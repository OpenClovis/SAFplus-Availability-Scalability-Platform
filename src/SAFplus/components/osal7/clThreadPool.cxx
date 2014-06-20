#include <clThreadPool.hxx>
#include <signal.h>

using namespace SAFplus;

Poolable::PoolableList Poolable::poolableList; 

Poolable::Poolable(UserCallbackT _fn, void* _arg, uint32_t timeLimit, bool _deleteWhenComplete): fn(_fn), arg(_arg), executionTimeLimit(timeLimit), deleteWhenComplete(_deleteWhenComplete)
{
}

void Poolable::calculateStartTime()
{
  int ret = clock_gettime(CLOCK_MONOTONIC, &startTime);
  printf("errno [%d]\n", errno);
  assert(ret==0);
}
void Poolable::calculateEndTime()
{
  int ret = clock_gettime(CLOCK_MONOTONIC, &endTime);
  printf("errno [%d]\n", errno);
  assert(ret==0);
}
void Poolable::calculateExecTime()
{
  // calculate execution time and convert it to milisecond
  unsigned long long int executionTime = (endTime.tv_sec - startTime.tv_sec)*1000 + (endTime.tv_nsec - startTime.tv_nsec)/1000000L;
  printf("Total execution time [%llu]\n", executionTime);
  assert(executionTime <= executionTimeLimit); //assert if execution time is longer than the configured limit one  
  printf("The task executed on time\n");  
}
bool Poolable::isDeleteWhenComplete()
{
  return deleteWhenComplete;
}
Poolable::~Poolable()
{
  printf("~Poolable() called\n");
}

ThreadPool::ThreadPool(short _minThreads, short _maxThreads): minThreads(_minThreads), maxThreads(_maxThreads), numCurrentThreads(0), numIdleThreads(0), isStopped(false)
{
  printf("ThreadPool::ThreadPool()\n");  
  start();
}

void ThreadPool::stop()
{
  printf("ThreadPool::stop()\n");
  mutex.lock();
  isStopped = true;
  // Notify all threads to stop waiting if any and then exit
  cond.notify_all(); 
  mutex.unlock();
}

void ThreadPool::start()
{
  printf("ThreadPool::start()\n");
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
  printf("ThreadPool::run()\n");
  enqueue(p);
}

void ThreadPool::run(Wakeable* wk, void* arg)
{
  printf("ThreadPool::runw()\n");
  WakeableHelper* pwh = getUnusedWHElement();
  if (pwh)
  {
    printf("Reuse helper object in list\n");
    pwh->wk=wk;
    pwh->arg=arg;
  }
  else
  {
    printf("No unused helper obj. Create a new one and add it to list\n");
    pwh = new WakeableHelper(wk, arg);
    whList.push_back(pwh);
  }
  enqueue(pwh);
}

void ThreadPool::enqueue(Poolable* p)
{  
  printf("ThreadPool::enqueue()\n");
  if (numIdleThreads == 0 && numCurrentThreads <= maxThreads)
  {
    printf("Creating a new thread to execute job because all current threads are busy\n");
    startThread();
  }
  printf("ThreadPool::enqueue(): lock the mutex\n");
  mutex.lock();  
  Poolable::poolableList.push_back(*p);
  printf("ThreadPool::enqueue(): notify\n");
  cond.notify_one();
  printf("ThreadPool::enqueue(): unlock the mutex\n");
  mutex.unlock();  
}

Poolable* ThreadPool::dequeue()
{  
  printf("ThreadPool::dequeue()\n");  
  printf("ThreadPool::dequeue(): lock the mutex\n");
  mutex.lock();
  printf("ThreadPool::dequeue(): if wait???\n");
  if (Poolable::poolableList.size() == 0)
  {    
    printf("ThreadPool::dequeue(): wait\n");
    cond.wait(mutex);
  }
  if (Poolable::poolableList.size() == 0) // in case of stopping the threadpool, the queue might not contain any item
  {
    mutex.unlock();
    return NULL;  
  }
  printf("ThreadPool::dequeue(): dequeue item 0\n");
  Poolable* p = &Poolable::poolableList.front();
  Poolable::poolableList.pop_front();
  printf("ThreadPool::dequeue(): unlock the mutex\n");
  mutex.unlock();
  return p;
}

void ThreadPool::runTask(void* arg)
{
  printf("ThreadPool::runTask()\n");  
  ThreadPool* tp = (ThreadPool*)arg;
  pthread_t thid = pthread_self();
  printf("runTask(): threadid [%lu]\n", thid);
  tp->mutex.lock();
  tp->cond.wait(tp->mutex);
  ThreadHashMap::iterator contents = tp->threadMap.find(thid);
  assert(contents != tp->threadMap.end());  
  tp->mutex.unlock();
  ThreadState& ts = contents->second;
  printf("runTask(): enter loop\n");
  while (!tp->isStopped)
  { 
    if (ts.quitAllowed)
    {
      printf("runTask(): I have to quit immediately\n");      
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
      printf("ThreadPool::runTask(): execute user-defined func of Poolable object\n");      
      p->wake(0, p->arg);      
      p->calculateStartTime();
      p->calculateEndTime();
      p->calculateExecTime();
      if (p->isDeleteWhenComplete())
      {
        printf("Delete the poolable object\n");
        delete p;        
      }          
    }
    else // Wakeable object
    {
      printf("ThreadPool::runTask(): execute user-defined func of WakeableHelper object[%p]\n", wh);
      ts.working = true;
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
  printf("ThreadPool::runTask(): exit runTask\n");
  return;
}

void ThreadPool::startThread()
{
  printf("ThreadPool::startThread()\n");
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
  printf("startThread(): insert to map\n");  
  threadMap.insert(mp);
  cond.notify_one();
  mutex.unlock();
}

WakeableHelper* ThreadPool::getUnusedWHElement()
{
  for (WHList::iterator it=whList.begin(); it != whList.end(); ++it)
  {
    printf("getUnusedWHElement(): [%p]\n", (Wakeable*)(*it));
    if ((*it)->wk==NULL)
    { 
      return *it;
    }
  }
  return NULL;
}

void* ThreadPool::timerThreadFunc(void* arg)
{
  printf("Enter timerThreadFunc()\n");
  ThreadPool* tp = (ThreadPool*) arg;
  while(!tp->isStopped)
  {
    printf("checkAndReleaseThread(): invoking checkAndReleaseThread\n");
    tp->checkAndReleaseThread();
    sleep(TIMER_INTERVAL);
  }
  return NULL;
}

void ThreadPool::checkAndReleaseThread()
{  
  printf("Enter checkAndReleaseThread(): numCurrentThreads [%d]\n", numCurrentThreads);
  int nRunningThreads = numCurrentThreads;
  bool quitNotify = false;
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
      printf("checkAndReleaseThread(): idle time from not working [%llu]\n", idleTime);
      if (idleTime >= THREAD_IDLE_TIME_LIMIT)
      {
        printf("checkAndReleaseThread(): allow thread [%lu] to quit\n", threadId);
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
        quitNotify = true;
        nRunningThreads--;      
      }
    }
  }
  if (quitNotify) 
    cond.notify_all();
  printf("Leave checkAndReleaseThread()\n");
}

ThreadPool::~ThreadPool()
{
  printf("~ThreadPool() called. Deallocate mem for helper object list [%d]\n", (int)whList.size());  
  for (WHList::iterator it=whList.begin(); it != whList.end(); ++it)
  {
    delete (*it);
  }
}
