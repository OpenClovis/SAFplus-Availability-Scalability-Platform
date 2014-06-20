#include <clThreadPool.hxx>
#include <typeinfo>

using namespace SAFplus;

class MyPoolable: public Poolable 
{
public:
  MyPoolable(UserCallbackT fn=NULL, void* arg=NULL, uint32_t timeLimit=30000, bool deleteWhenComplete=false): Poolable(fn, arg, timeLimit, deleteWhenComplete) {}
  virtual void wake(int amt, void* cookie)
  {
    printf("MyPoolable wake()\n");
    if (fn)
    {
      fn(arg);
      sleep(5);
    }
    else
    {
      printf("Put your own code here\n");
      sleep(5);
    }
  }
  ~MyPoolable()
   {
     printf("~MyPoolable() called\n");
   }
};

class MyWakeable: public Wakeable
{
public:
  virtual void wake(int amt, void* cookie)
  {
    printf("MyWakeable wake()\n");
  }
};

//ThreadPool pool(0, 4);

uint32_t foo(void* invocation)
{
  printf("My task running\n");
  return 0;
}

uint32_t foo2(void* invocation)
{
  printf("My task 2 running\n");
  return 0;
}

uint32_t foo3(void* invocation)
{
  printf("My task 3 running\n");
  return 0;
}

uint32_t foo4(void* invocation)
{
  printf("My task 4 running\n");
  return 0;
}

int main()
{
  printf("Main started\n");
  MyWakeable mywk2;  
  MyPoolable p(&foo);
  MyPoolable p2(&foo2);
  MyPoolable p3(&foo3);
  MyPoolable p4;
  MyWakeable mywk;
  ThreadPool pool(2, 10);
  printf("Calling pool.run\n");
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
  pool.stop();
  sleep(5);

  return 0;
}

