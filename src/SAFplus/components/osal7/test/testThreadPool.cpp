#include <clThreadPool.hxx>
#include <typeinfo>

using namespace SAFplus;

class MyPoolable: public Poolable 
{
public:
  MyPoolable(UserCallbackT fn=NULL, uint32_t timeLimit=30000, bool deleteWhenComplete=false): Poolable(fn, timeLimit, deleteWhenComplete) {}
  virtual void wake(int amt, void* cookie)
  {
    printf("MyPoolable wake()\n");
    if (fn)
    {
      fn(cookie);
    }
    else
    {
      printf("Executing my own code here\n");
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
  MyPoolable p(&foo);
  MyPoolable p2(&foo2);
  MyPoolable p3(&foo3);
  MyPoolable p4;
  MyWakeable mywk;
  ThreadPool pool(0, 4);
  printf("Calling pool.run\n");
  pool.run(&p, NULL);
  pool.run(&p2, NULL);
  pool.run(&p3, NULL);  
  pool.run(&mywk, NULL);
  pool.run(&p4, NULL);
  MyPoolable* pl = new MyPoolable(NULL,3,true);
  pool.run(pl, NULL);
  sleep(2);
  pool.stop();
  sleep(5);

  return 0;
}

