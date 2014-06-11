#include <clThreadPool.hxx>

using namespace SAFplus;

TaskQueue* ThreadPool::taskQueue = new TaskQueue();

Poolable::Poolable(UserCallbackT _fn, uint32_t timeLimit, bool _deleteWhenComplete): fn(_fn), executionTimeLimit(timeLimit), deleteWhenComplete(_deleteWhenComplete)
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
void Poolable::calculateExeTime()
{
  // calculate execution time and convert it to milisecond
  unsigned long long int executionTime = (endTime.tv_sec - startTime.tv_sec)*1000 + (endTime.tv_nsec - startTime.tv_nsec)/1000000L;
  printf("Total execution time [%llu]\n", executionTime);
  assert(executionTime <= executionTimeLimit); //assert if execution time is longer than the configured limit one  
  printf("The task executed on time\n");  
}

Poolable::~Poolable()
{
  printf("~Poolable() called\n");
}

ThreadPool::ThreadPool(short _minThreads, short _maxThreads): minThreads(_minThreads), maxThreads(_maxThreads)
{
  printf("ThreadPool::ThreadPool()\n");
  isStopped = false;
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
  for(int i=0;i<maxThreads;i++)
  {
    startThread();
  }  
}

/*void ThreadPool::run(Poolable* p, void* arg)
{
  printf("ThreadPool::run()\n");
  enqueue(p, arg);
}*/

void ThreadPool::run(Wakeable* p, void* arg)
{
  printf("ThreadPool::run()\n");
  enqueue(p, arg);
}

void ThreadPool::enqueue(Wakeable* p, void* arg)
{  
  pthread_t thid = pthread_self();
  printf("ThreadPool::enqueue()\n");
  TaskData* td = new TaskData(p, arg);
  printf("ThreadPool::enqueue(): lock the mutex\n");
  mutex.lock();
  printf("=======================================\n");
  td->p->wake(0, arg);
  printf("=======================================\n");
  taskQueue->push_back(td);
  printf("ThreadPool::enqueue(): notify\n");
  cond.notify_one();
  printf("ThreadPool::enqueue(): unlock the mutex\n");
  mutex.unlock();  
}

TaskData* ThreadPool::dequeue()
{  
  pthread_t thid = pthread_self();
  printf("ThreadPool::dequeue()\n");  
  printf("ThreadPool::dequeue(): lock the mutex\n");
  mutex.lock();
  printf("ThreadPool::dequeue(): if wait???\n");
  if (taskQueue->size() == 0)
  {    
    printf("ThreadPool::dequeue(): wait\n");
    cond.wait(mutex);
  }
  if (taskQueue->size() == 0) // in case of stopping the threadpool, the queue might not contain any item
  {
    mutex.unlock();
    return NULL;  
  }
  printf("ThreadPool::dequeue(): dequeue item 0\n");
  TaskData* td = taskQueue->at(0);
  taskQueue->erase(taskQueue->begin());
  printf("ThreadPool::dequeue(): unlock the mutex\n");
  mutex.unlock();
  return td;
}

void ThreadPool::runTask(void* arg)
{
  //pthread_t thid = pthread_self();
  printf("ThreadPool::runTask()\n");
  ThreadPool* tp = (ThreadPool*)arg;
  while (!tp->isStopped)
  {    
    TaskData* td = tp->dequeue();
    printf("ThreadPool::runTask(): execute user-defined func\n");
    if (td)
    {
      Poolable* p = dynamic_cast<Poolable*>(td->p);
      if (p!=NULL)
      {      
        p->calculateStartTime();
      } 
      td->p->wake(0,td->arg);
      if (p!=NULL)
      {
        p->calculateEndTime();
        p->calculateExeTime();
      }
      // Delete the object?      
      if (p!=NULL && p->deleteWhenComplete)
      {
        printf("Delete the poolable object\n");
        delete p;        
      }
      delete td;
    }
  }
  printf("ThreadPool::runTask(): exit runTask\n");
  return;
}

void ThreadPool::startThread()
{
  printf("ThreadPool::startThread()\n");
  pthread_t thid;
  pthread_create(&thid, NULL, (void* (*) (void*)) runTask, this);
}

ThreadPool::~ThreadPool()
{
  printf("~ThreadPool() called\n");  
  delete taskQueue;
}

