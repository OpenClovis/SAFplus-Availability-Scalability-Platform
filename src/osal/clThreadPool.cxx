#include <clThreadPool.hxx>
#include <clLogApi.hxx>


using namespace SAFplus;

Poolable::PoolableList Poolable::poolableList;

Poolable::Poolable(UserCallbackT _fn, void* _arg, uint32_t timeLimit, bool _deleteWhenComplete): fn(_fn), arg(_arg), executionTimeLimit(timeLimit), deleteWhenComplete(_deleteWhenComplete)
{
  structId = Poolable::STRUCT_ID;
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
  // assert(executionTime <= executionTimeLimit); //assert if execution time is longer than the configured limit one // TODO: No, you can't measure the thread duration and assert INSIDE the thread.  That defeats the purpose of finding threads that are stuck in p->wake.  We need a separate thread that periodically looks at the run length of the other threads.
}
bool Poolable::isDeleteWhenComplete()
{
  return deleteWhenComplete;
}

Poolable::~Poolable()
{
    structId = 0xdeadbeef;
}

ThreadPool::ThreadPool(): minThreads(0), maxThreads(0), numCurrentThreads(0), numIdleThreads(0), isStopped(false)
{
}

ThreadPool::ThreadPool(short _minThreads, short _maxThreads): minThreads(_minThreads), maxThreads(_maxThreads), numCurrentThreads(0), numIdleThreads(0), isStopped(false)
{
  start();
}

void ThreadPool::init(uint_t _minThreads, uint_t _maxThreads)
{
  minThreads = _minThreads; 
  maxThreads = _maxThreads;
  start();
}

void ThreadPool::stop()
{
  logTrace("THRPOOL","STOP", "ThreadPool::stop enter");
  mutex.lock();
  isStopped = true;
  // Notify all threads to stop waiting if any and then exit
  logInfo("THRPOOL","STOP", "Notify all threads to stop running");
  cond.notify_all();
  mutex.unlock();
  // TODO: join the threads here instead of sleeping
  sleep(5);
}

void ThreadPool::start()
{
  logTrace("THRPOOL","START", "ThreadPool::start enter");
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
  logTrace("THRPOOL","RUN", "ThreadPool::run enter");
  enqueue(p);
}

void ThreadPool::run(Wakeable* wk, void* arg)
{
  logTrace("THRPOOL","RUN", "ThreadPool::run enter");
  WakeableHelper* pwh = allocWakeableHelper(wk,arg);
  enqueue(pwh);
}

void ThreadPool::enqueue(Poolable* p)
{
  logTrace("THRPOOL","ENQ", "ThreadPool::enqueue enter");
  if ((numIdleThreads == 0) && (numCurrentThreads <= maxThreads))
  {
    logDebug("THRPOOL","ENQ", "Creating a new thread to execute job because all current threads are busy.  Current num threads [%d], max allowed [%d]", numCurrentThreads, maxThreads);
    startThread();
  }
  mutex.lock();
  Poolable::poolableList.push_back(*p);
  cond.notify_one();
  mutex.unlock();
}

void Poolable::complete()
{
}

Poolable* ThreadPool::dequeue()
{
  logTrace("THRPOOL","DEQ", "ThreadPool::dequeue enter");
  mutex.lock();
  if (Poolable::poolableList.size() == 0)
  {
    logTrace("THRPOOL","DEQ", "No task in queue. Wait...");
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
  logTrace("THRPOOL","RUNTSK", "ThreadPool::runTask enter");
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
    Poolable* p = tp->dequeue();  // blocking
    if (!p)
      continue;

    ts.working = true;
    tp->mutex.lock();
    tp->numIdleThreads--;
    tp->mutex.unlock();
    WakeableHelper* wh = dynamic_cast<WakeableHelper*>(p);
    if (!wh) // it's Poolable object
    {
      logTrace("THRPOOL","RUNTSK", "Execute user-defined func of Poolable object");
      assert(p->structId == Poolable::STRUCT_ID);
      p->calculateStartTime();
      p->wake(0, p->arg);
      p->calculateEndTime();
      p->calculateExecTime(); 
      p->complete();
      if (p->isDeleteWhenComplete())
      {
        logTrace("THRPOOL","RUNTSK", "Delete the poolable object");
        delete p;
      }
    }
    else // Wakeable object
    {
      logTrace("THRPOOL","RUNTSK", "Execute user-defined func of Wakeable object");
      wh->wk->wake(0, wh->arg);
      wh->wk=NULL;
      
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
  logTrace("THRPOOL","RUNTSK", "exit runTask");
  return;
}

void ThreadPool::startThread()
{
  logTrace("THRPOOL","STARTTHR", "startThread enter");
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

WakeableHelper* ThreadPool::allocWakeableHelper(Wakeable* wk, void* arg)
{
  if (unusedWakeableHelperList == nullptr)
    {
    WakeableHelper* pwh = new WakeableHelper(wk, arg);
    return pwh;
    }
  else 
    {
      WakeableHelper* pwh = unusedWakeableHelperList;
      unusedWakeableHelperList = pwh->next;
      pwh->next = nullptr;
      pwh->wk = wk;
      pwh->arg = arg;
    }  

#if 0
  for (WHList::iterator it=whList.begin(); it != whList.end(); ++it)
  {
    if ((*it)->wk==NULL)
    {
      return *it;
    }
  }
  return NULL;
#endif

}

void ThreadPool::deleteWakeableHelper(WakeableHelper* wh)
  {
    if (unusedWakeableHelperList == nullptr)
    {
      unusedWakeableHelperList = wh;
      wh->next = nullptr;
    }
    else
      {
        wh->next = unusedWakeableHelperList;
        unusedWakeableHelperList = wh;
      }
    //delete wh;
  }

void* ThreadPool::timerThreadFunc(void* arg)
{
  logTrace("THRPOOL","TIMERFUNC", "timerThreadFunc enter");
  ThreadPool* tp = (ThreadPool*) arg;
  while(!tp->isStopped)
  {
    sleep(SAFplusI::ThreadPoolTimerInterval);
    tp->checkAndReleaseThread();
  }
  return NULL;
}

void ThreadPool::checkAndReleaseThread()
{
  logTrace("THRPOOL","RLS", "checkAndReleaseThread enter: numCurrentThreads [%d]", numCurrentThreads);
  int nRunningThreads = numCurrentThreads;
  for(ThreadHashMap::iterator iter=threadMap.begin(); iter!=threadMap.end()&&nRunningThreads>minThreads; iter++)
  {
    pthread_t threadId = iter->first;
    //printf("checkAndReleaseThread(): threadId [%lu]\n", threadId);
    ThreadState& ts = iter->second;
    if (!ts.working)
    {
      struct timespec now;
      int ret = clock_gettime(CLOCK_MONOTONIC, &now);
      //printf("errno [%d]\n", errno);
      assert(ret==0);
      unsigned long long int idleTime = now.tv_sec - ts.idleTimestamp.tv_sec + (now.tv_nsec - ts.idleTimestamp.tv_nsec)/1000000000L; // calculating idle time in second
      logTrace("THRPOOL","RLS","idle time from not working [%llu]", idleTime);
      if (idleTime >= SAFplusI::ThreadPoolIdleTimeLimit)
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
  stop();
  WakeableHelper* iter = unusedWakeableHelperList;
  while(iter)
    {
      WakeableHelper* tmp = iter;
      iter = iter->next;
      delete tmp;
    }
}
