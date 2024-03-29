#include <clThreadPool.hxx>
#include <clLogIpi.hxx>


using namespace SAFplus;

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

ThreadPool::ThreadPool(): minThreads(0), maxThreads(0), numCurrentThreads(0), numIdleThreads(0), isStopped(true), unusedWakeableHelperList(nullptr)
{
checker = 0; // TODO
}

ThreadPool::ThreadPool(short _minThreads, short _maxThreads): minThreads(_minThreads), maxThreads(_maxThreads), numCurrentThreads(0), numIdleThreads(0), isStopped(true), unusedWakeableHelperList(nullptr)
{
  checker = 0; // TODO
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
  if(!isStopped)
  {
    isStopped = true;
    // Notify all threads to stop waiting if any and then exit
    logInfo("THRPOOL","STOP", "Notify all threads to stop running");
    cond.notify_all();
    checkerCond.notify_all();
    mutex.unlock();

    pthread_join(checker,NULL);  // wait for the checker thread to finish

    // join the threads to make sure they have all completed
    // I can't hold mutex during the join because the threads grab it when quitting, but they do not modify threadMap.
    for(ThreadHashMap::iterator iter=threadMap.begin(); iter!=threadMap.end(); iter++)
    {
      pthread_t threadId = iter->first;
      ThreadState& ts = iter->second;
      if (!ts.finished)
        pthread_join(threadId,NULL); // TODO: after a while we should give up and kill it
    }
    threadMap.clear();
  }
  else
  {
    mutex.unlock();
  }
   
}

void ThreadPool::start()
{
  if (isStopped)
  {
    isStopped=false;
    logTrace("THRPOOL","START", "ThreadPool::start enter");
    for(int i=0;i<minThreads;i++)
    {
      startThread();
    }
    pthread_create(&checker, NULL, timerThreadFunc, this);
  }
}

void ThreadPool::run(Poolable* p)
{
  //logTrace("THRPOOL","RUN", "ThreadPool::run enter");
  //immediatelyRun(p);
  enqueue(p);
}


    //? [GDB only] verify that all items in the queue are Poolable objects
bool ThreadPool::dbgValidateQueue()
{
  mutex.lock();
  for (PoolableList::iterator it = poolableList.begin(); it != poolableList.end(); it++)
    {
      Poolable& p = *it;
      assert(&p);
      assert(p.structId == Poolable::STRUCT_ID);
      assert(p.deleteWhenComplete == false);  // temporary
    }

  mutex.unlock();
  return true;
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
  bool startNewThread = false;
  mutex.lock();
  if ((numIdleThreads == 0) && (numCurrentThreads <= maxThreads))
  {
    mutex.unlock();
    logDebug("THRPOOL","ENQ", "Creating a new thread to execute job because all current threads are busy.  Current num threads [%d], max allowed [%d]", numCurrentThreads, maxThreads);
    startThread();
    startNewThread = true;
  }
  if (startNewThread)
  {
    mutex.lock();
  }
  poolableList.push_back(*p);
  cond.notify_one();
  mutex.unlock();
}

void Poolable::complete()
{
}

Poolable* ThreadPool::dequeue()
{
  //logTrace("THRPOOL","DEQ", "ThreadPool::dequeue enter");
  mutex.lock();
  if (poolableList.empty())
  {
    logTrace("THRPOOL","DEQ", "No task in queue. Wait...");
    cond.wait(mutex);
  }
  if (poolableList.empty()) // in case of stopping the threadpool, the queue might not contain any item
  {
    mutex.unlock();
    return NULL;
  }
  Poolable* p = &poolableList.front();
  poolableList.pop_front();
  mutex.unlock();
  return p;
}

void ThreadPool::immediatelyRun(Poolable*p)
{
    WakeableHelper* wh = dynamic_cast<WakeableHelper*>(p);
    if (!wh) // it's Poolable object
    {
      //logTrace("THRPOOL","RUNTSK", "Execute user-defined func of Poolable object");
      assert(p->structId == Poolable::STRUCT_ID);
      p->calculateStartTime();
      p->wake(0, p->arg);
      p->calculateEndTime();
      p->calculateExecTime(); 
      bool tmp = p->isDeleteWhenComplete(); 
      p->complete();   // Its possible that complete() will release p so I can't use it after this line
      if (tmp)  // Unless delete is requested, then I will delete it.
      {
        assert(0);  // this feature is not used
        logTrace("THRPOOL","RUNTSK", "Delete the poolable object");
        delete p;
      }
    }
    else // Wakeable object
    {
      assert(0); // temporary
      logTrace("THRPOOL","RUNTSK", "Execute user-defined func of Wakeable object");
      wh->wk->wake(0, wh->arg);
      wh->wk=NULL;
      deleteWakeableHelper(wh);
    }
}

void ThreadPool::runTask(void* arg)
{
  logTrace("THRPOOL","RUNTSK", "ThreadPool::runTask enter");
  ThreadPool* tp = (ThreadPool*)arg;
  pthread_t thid = pthread_self();
  tp->mutex.lock();
  ThreadHashMap::iterator contents = tp->threadMap.find(thid);
  //assert(contents != tp->threadMap.end());
  if (contents == tp->threadMap.end())
  {
     logWarning("THRPOOL","RUNTSK", "thread [%lu] has no map's contents", thid);
     return;     
  }
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

    tp->mutex.lock();
    tp->numIdleThreads--;
    ts.working = true;
    tp->mutex.unlock();
    tp->immediatelyRun(p);
    tp->mutex.lock();
    ts.working = false;
    tp->mutex.unlock();
    int ret = clock_gettime(CLOCK_MONOTONIC, &ts.idleTimestamp);
    assert(ret==0);
    tp->mutex.lock();
    tp->numIdleThreads++;
    tp->mutex.unlock();
  }
  tp->mutex.lock();
  // We can't remove this thread from the map from inside the thread, or join() won't be called and the thread will zombie
  //tp->threadMap.erase(contents); // Remove this thread element from the map and exit
  tp->numCurrentThreads--;
  tp->numIdleThreads--;
  ts.zombie = true;
  tp->mutex.unlock();
  logTrace("THRPOOL","RUNTSK", "exit runTask");
  return;
}

void ThreadPool::startThread()
{
  logTrace("THR","POOL", "startThread enter");
  pthread_t thid;
  mutex.lock();
  pthread_create(&thid, NULL, (void* (*) (void*)) runTask, this);
  // If we detach, we can't join to make sure it is cleaned up: pthread_detach(thid);
  numIdleThreads++;
  numCurrentThreads++;
  ThreadState ts(false,false);
  int ret = clock_gettime(CLOCK_MONOTONIC, &ts.idleTimestamp);
  assert(ret==0);
  ThreadMapPair mp(thid, ts);
  threadMap.insert(mp);
  mutex.unlock();  // Don't unlock before the insert or the new thread might run before we put its details in the map
}

WakeableHelper* ThreadPool::allocWakeableHelper(Wakeable* wk, void* arg)
{
  WakeableHelper* pwh = nullptr;
  mutex.lock();
  if (unusedWakeableHelperList == nullptr)
    {
    pwh = new WakeableHelper(wk, arg);
    }
  else 
    {
      WakeableHelper* pwh = unusedWakeableHelperList;
      unusedWakeableHelperList = pwh->next;
      pwh->next = nullptr;
      pwh->wk = wk;
      pwh->arg = arg;
    }  
  mutex.unlock();
  return pwh;

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
  mutex.lock();
    if (unusedWakeableHelperList == nullptr)
    {
      wh->next = nullptr;
      unusedWakeableHelperList = wh;
      
    }
    else
      {
        wh->next = unusedWakeableHelperList;
        unusedWakeableHelperList = wh;
      }
    //delete wh;
    mutex.unlock();
  }

void* ThreadPool::timerThreadFunc(void* arg)
{
  logTrace("THRPOOL","TIMERFUNC", "timerThreadFunc enter");
  ThreadPool* tp = (ThreadPool*) arg;
  tp->mutex.lock();
  while(!tp->isStopped)
  {
    tp->checkerCond.timed_wait(tp->mutex,SAFplusI::ThreadPoolTimerInterval * 1000);
    tp->checkAndReleaseThread();
  }
  tp->mutex.unlock();
  return NULL;
}

void ThreadPool::checkAndReleaseThread()
{
  logTrace("THRPOOL","RLS", "checkAndReleaseThread enter: numCurrentThreads [%d]", numCurrentThreads);
  int nRunningThreads = numCurrentThreads;
  ThreadHashMap::iterator eraseMe = threadMap.end();
  for(ThreadHashMap::iterator iter=threadMap.begin(); iter!=threadMap.end()&&nRunningThreads>minThreads; iter++)
  {
    // I can't erase the element the iterator is visiting, so if I need to erase I'll set this variable and wait for the iter to advance, executing the erase at the top of the next loop.
    if (eraseMe != threadMap.end()) { threadMap.erase(eraseMe); eraseMe = threadMap.end(); }
    pthread_t threadId = iter->first;
    //printf("checkAndReleaseThread(): threadId [%lu]\n", threadId);
    ThreadState& ts = iter->second;
    if (ts.zombie)
      {
        pthread_join(threadId,NULL);  // clean up thread tracker in OS (stop zombie threads)
        ts.finished = true;
        eraseMe = iter;        
      }
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
          logWarning("THRPOOL","RLS","WARNING. terminating thread [%lu]", threadId);
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
