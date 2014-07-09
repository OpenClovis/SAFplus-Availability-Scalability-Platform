#include <clThreadPool.hxx>
#include <clCommon.hxx>

using namespace SAFplus;

class MyPoolable: public Poolable 
{
public:
  MyPoolable(UserCallbackT fn=NULL, void* arg=NULL, uint32_t timeLimit=30000, bool deleteWhenComplete=false): Poolable(fn, arg, timeLimit, deleteWhenComplete) {}
  virtual void wake(int amt, void* cookie)
  {
    logInfo("THRPOOL", "TEST", "MyPoolable wake()");
    if (fn)
    {
      fn(arg);
      sleep(1);
    }
    else
    {
      //printf("Put your own code here\n");
      sleep(1);
    }
  }
  ~MyPoolable()
   {
     //printf("~MyPoolable() called\n");
   }
};

class MyWakeable: public Wakeable
{
public:
  virtual void wake(int amt, void* cookie)
  {
    logInfo("THRPOOL", "TEST", "MyWakeable wake()");
  }
};

//ThreadPool pool(0, 4);

uint32_t foo(void* invocation)
{
  logInfo("THRPOOL", "TEST", "My task running");
  return 0;
}

uint32_t foo2(void* invocation)
{
  logInfo("THRPOOL", "TEST", "My task 2 running");
  return 0;
}

uint32_t foo3(void* invocation)
{
  logInfo("THRPOOL", "TEST", "My task 3 running");
  return 0;
}

uint32_t foo4(void* invocation)
{
  logInfo("THRPOOL", "TEST", "My task 4 running");
  return 0;
}

int main()
{
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_TRACE;
  logDebug("THRPOOL", "TEST", "Main started");
  
#if 0
  MyWakeable mywk2;  
  MyPoolable p(&foo);
  MyPoolable p2(&foo2);
  MyPoolable p3(&foo3);
  MyPoolable p4;
  MyWakeable mywk;
  ThreadPool pool(2, 10);
  pool.run(&mywk2, NULL);
  pool.run(&p);
  pool.run(&p2);
  pool.run(&p3);  
  pool.run(&mywk, NULL);
  pool.run(&p4);
  MyPoolable* pl = new MyPoolable(NULL,NULL,6000,true);  
  pool.run(pl);  
  MyWakeable mywk3;
  pool.run(&mywk3, NULL);
  sleep(120);
#endif
#if 1
  ThreadPool pool(1000, 1500);
  MyPoolable p[10000];
  for(int i=0;i<10000;i++)
  {
    p[i] = MyPoolable(&foo);
    pool.run(&p[i]);
  }
  sleep(180);
#endif  
  pool.stop();
  sleep(10);

  return 0;
}

