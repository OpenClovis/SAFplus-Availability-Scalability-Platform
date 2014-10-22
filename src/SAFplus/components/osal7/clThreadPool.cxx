#include <clThreadPool.hxx>
#include <clLogApi.hxx>


using namespace SAFplus;

Poolable::PoolableList Poolable::poolableList;

Poolable::Poolable(UserCallbackT _fn, void* _arg, uint32_t timeLimit, bool _deleteWhenComplete): fn(_fn), arg(_arg), executionTimeLimit(timeLimit), deleteWhenComplete(_deleteWhenComplete)
{
}

void Poolable::calculateStartTime()
{
  int ret = clock_gettime(CLOCK_MONOTONIC, &startTime);
  assert(ret==0);
}
void Poolable::calculateEndTime()
{
  int ret = clock_gettime(CLOCK_MONOTONIC, &endTime);
  assert(ret==0);
}
void Poolable::calculateExecTime()
{
  // calculate execution time and convert it to milisecond
  unsigned long long int executionTime = (endTime.tv_sec - startTime.tv_sec)*1000 + (endTime.tv_nsec - startTime.tv_nsec)/1000000L;
  assert(executionTime <= executionTimeLimit); //assert if execution time is longer than the configured limit one
}
bool Poolable::isDeleteWhenComplete()
{
  return deleteWhenComplete;
}
Poolable::~Poolable()
{
}

ThreadPool::ThreadPool(short _minThreads, short _maxThreads): minThreads(_minThreads), maxThreads(_maxThreads), numCurrentThreads(0), numIdleThreads(0), isStopped(false)
{
  start();
}

void ThreadPool::stop()
{
  logDebug("THRPOOL","STOP", "ThreadPool::stop enter");
  mutex.lock();
  isStopped = true;
  // Notify all threads to stop waiting if any and then exit
  logInfo("THRPOOL","STOP", "Notify all threads to stop running");
  cond.notify_all();
  mutex.unlock();
}

void ThreadPool::start()
{
  logDebug("THRPOOL","START", "ThreadPool::start enter");
  for(int i=0;i<minThreads;i++)
  {
    startThread();
  }
  pthread_t thid;
  pthread_create(&thid, NULL, timerThreadFunc, this);
  pthread_detach(thid);
}

void ThreadPool::run(Poolable* p)
{
  logDebug("THRPOOL","RUN", "ThreadPool::run enter");
  enqueue(p);
}

void ThreadPool::run(Wakeable* wk, void* arg)
{
  logDebug("THRPOOL","RUN", "ThreadPool::run enter");
  WakeableHelper* pwh = getUnusedWHElement();
  if (pwh)
  {
    pwh->wk=wk;
    pwh->arg=arg;
  }
  else
  {
    pwh = new WakeableHelper(wk, arg);
    whList.push_back(pwh);
  }
  enqueue(pwh);
}

void ThreadPool::enqueue(Poolable* p)
{
  logDebug("THRPOOL","ENQ", "ThreadPool::enqueue enter");
  if (numIdleThreads == 0 && numCurrentThreads < maxThreads)
  {
    logDebug("THRPOOL","ENQ", "Creating a new thread to execute job because all current threads are busy");
    startThread();
  }
  mutex.lock();
  Poolable::poolableList.push_back(*p);
  cond.notify_one();
  mutex.unlock();
}

Poolable* ThreadPool::dequeue()
{
  logDebug("THRPOOL","DEQ", "ThreadPool::dequeue enter");
  mutex.lock();
  if (Poolable::poolableList.size() == 0)
  {
    logDebug("THRPOOL","DEQ", "No task in queue. Wait...");
    cond.wait(mutex);
  }
  if (Poolable::poolableList.size() == 0) // in case of stopping the threadpool, the queue might not contain any item
  {
    mutex.unlock();
    return NULL;
  }
  Poolable* p = &Poolable::poolableList.front();
  Poolable::poolableList.pop_front();
  mutex.unlock();
  return p;
}

void ThreadPool::runTask(void* arg)
{
  logDebug("THRPOOL","RUNTSK", "ThreadPool::runTask enter");
  ThreadPool* tp = (ThreadPool*)arg;
  pthread_t thid = pthread_self();
  tp->mutex.lock();
  tp->cond.wait(tp->mutex);
  ThreadHashMap::iterator contents = tp->threadMap.find(thid);
  assert(contents != tp->threadMap.end());
  tp->mutex.unlock();
  ThreadState& ts = contents->second;
  while (!tp->isStopped)
  {
    if (ts.quitAllowed)
    {
      logInfo("THRPOOL","RUNTSK", "Thread [%lu] has to quit immediately", thid);
      break;
    }
    Poolable* p = tp->dequeue();
    if (!p)
      continue;
    ts.working = true;
    tp->mutex.lock();
    tp->numIdleThreads--;
    tp->mutex.unlock();
    WakeableHelper* wh = dynamic_cast<WakeableHelper*>(p);
    if (!wh) // it's Poolable object
    {
      logDebug("THRPOOL","RUNTSK", "Execute user-defined func of Poolable object");
      p->wake(0, p->arg);
      p->calculateStartTime();
      p->calculateEndTime();
      p->calculateExecTime();
      if (p->isDeleteWhenComplete())
      {
        logDebug("THRPOOL","RUNTSK", "Delete the poolable object");
        delete p;
      }
    }
    else // Wakeable object
    {
      logDebug("THRPOOL","RUNTSK", "Execute user-defined func of Wakeable object");
      wh->wk->wake(0, wh->arg);
      wh->wk=NULL;
      //delete wh;
    }
    ts.working = false;
    int ret = clock_gettime(CLOCK_MONOTONIC, &ts.idleTimestamp);
    assert(ret==0);
    tp->mutex.lock();
    tp->numIdleThreads++;
    tp->mutex.unlock();
  }
  tp->mutex.lock();
  tp->threadMap.erase(contents); // Remove this thread element from the map and exit
  tp->numCurrentThreads--;
  tp->mutex.unlock();
  logDebug("THRPOOL","RUNTSK", "exit runTask");
  return;
}

void ThreadPool::startThread()
{
  logDebug("THRPOOL","STARTTHR", "startThread enter");
  numIdleThreads++;
  pthread_t thid;
  pthread_create(&thid, NULL, (void* (*) (void*)) runTask, this);
  pthread_detach(thid);
  numCurrentThreads++;
  mutex.lock();
  ThreadState ts(false,false);
  int ret = clock_gettime(CLOCK_MONOTONIC, &ts.idleTimestamp);
  assert(ret==0);
  ThreadMapPair mp(thid, ts);
  threadMap.insert(mp);
  cond.notify_one();
  mutex.unlock();
}

WakeableHelper* ThreadPool::getUnusedWHElement()
{
  for (WHList::iterator it=whList.begin(); it != whList.end(); ++it)
  {
    if ((*it)->wk==NULL)
    {
      return *it;
    }
  }
  return NULL;
}

void* ThreadPool::timerThreadFunc(void* arg)
{
  logDebug("THRPOOL","TIMERFUNC", "timerThreadFunc enter");
  ThreadPool* tp = (ThreadPool*) arg;
  while(!tp->isStopped)
  {
    sleep(TIMER_INTERVAL);
    tp->checkAndReleaseThread();
  }
  return NULL;
}

void ThreadPool::checkAndReleaseThread()
{
  logDebug("THRPOOL","RLS", "checkAndReleaseThread enter: numCurrentThreads [%d]", numCurrentThreads);
  int nRunningThreads = numCurrentThreads;
  for(ThreadHashMap::iterator iter=threadMap.begin(); iter!=threadMap.end()&&nRunningThreads>minThreads; iter++)
  {
    pthread_t threadId = iter->first;
    printf("checkAndReleaseThread(): threadId [%lu]\n", threadId);
    ThreadState& ts = iter->second;
    if (!ts.working)
    {
      struct timespec now;
      int ret = clock_gettime(CLOCK_MONOTONIC, &now);
      printf("errno [%d]\n", errno);
      assert(ret==0);
      unsigned long long int idleTime = now.tv_sec - ts.idleTimestamp.tv_sec + (now.tv_nsec - ts.idleTimestamp.tv_nsec)/1000000000L; // calculating idle time in second
      logDebug("THRPOOL","RLS","idle time from not working [%llu]", idleTime);
      if (idleTime >= THREAD_IDLE_TIME_LIMIT)
      {
        logDebug("THRPOOL","RLS","allow thread [%lu] to quit", threadId);
        #if 0
        ret = pthread_kill(threadId, SIGKILL);
        if (ret==0)
        {
          numCurrentThreads--;
        }
        else
        {
          printf("WARNING. terminating thread\n");
        }
        #endif
        ts.quitAllowed = true;
        nRunningThreads--;
      }
    }
  }
  if (nRunningThreads<numCurrentThreads)
    cond.notify_all();
  //printf("Leave checkAndReleaseThread()\n");
}

ThreadPool::~ThreadPool()
{
  logDebug("THRPOOL","DES","Deallocate mem for helper object list. size[%d]", (int)whList.size());
  for (WHList::iterator it=whList.begin(); it != whList.end(); ++it)
  {
    delete (*it);
  }
}
