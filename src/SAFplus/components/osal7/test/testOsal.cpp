#include <boost/thread.hpp>
#include <clTestApi.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clCommon.hxx>
#include <clOsalApi.hxx>

using namespace SAFplus;


void TestProcSem_oneProcess(void)
{
}

void TestProcSem_basic(void)
{
  clTestCaseStart(("Basic ProcSem"));
  if (1)
    {
    ProcSem s1(1,1);
    s1.lock();  /* Should NOT hang here */
    }

  if (1)
    {
    ProcSem s1(1,0);
    clTest(("try_lock returns False when sem is 0"), s1.try_lock()==0, (" "));
    s1.unlock();                 // test "giving"
    clTest(("make sure try_lock correctly takes the sem."), s1.try_lock()==1, (" "));
    clTest(("make sure it can only take once."), s1.try_lock()==0, (" "));
    s1.unlock(5);                // test giving > 1    
    clTest(("lock count > 1"), s1.try_lock()==1, (" "));  // now sem is 4

    clTest(("take too many"), s1.try_lock(5)==0, (" "));  // taking too many!
    clTest(("take just enough"), s1.try_lock(4)==1, (" ")); // taking just enough!

    /* s1.lock();  // should hang here */
    }
  
  clTestCaseEnd((" "));
  
}

class SleepWaker
{
public:
  unsigned long int delay;
  Wakeable* wake;
  const char* printThis;
  SleepWaker(Wakeable&w,unsigned long int dly,const char *print):printThis(print) { wake=&w; delay=dly; };
  void operator()()
  {
        boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
        printf(printThis);
	wake->wake(1,NULL);
  }
};

void testCondition(void)
{
  Mutex m;
  ThreadCondition c;
 
  clTestCaseStart(("Basic Condition"));

  if (1)
    { 
      ScopedLock<> lock(m);
      boost::thread(SleepWaker(c,1000,"Kicking the condition\n"));
      clTest(("condition kicker"), c.timed_wait(m,2000)==1, (" "));
    }

  if (1)
    { 
      ScopedLock<> lock(m);
      boost::thread(SleepWaker(c,2000,"Not Kicking the condition soon enough\n"));
      clTest(("condition abort before kicker"), c.timed_wait(m,1000)==0, (" "));
      clTest(("condition kicker"), c.timed_wait(m,2000)==1, (" "));  // If I don't wait until the thread quits it will be accessing out of scope stack vars...
    }

  clTestCaseEnd((" "));

}

void testProcSem(void)
{
  TestProcSem_basic();
  TestProcSem_oneProcess();
}

void testProcess(void)
{
  clTestCaseStart(("Process Information"));
  int pid = getpid();

  Process proc(pid);
  std::string s = proc.getCmdline();
  printf("This process' command line: %s\n", s.c_str());
  clTest(("command line matches"), s.find("testOsal") != std::string::npos, ("")); 
  clTestCaseEnd((" "));
}



int main(int argc, char* argv[])
{
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging  
  utilsInitialize();

  clTestGroupInitialize(("Osal"));
  testProcSem();
  testCondition();
  testProcess();
  clTestGroupFinalize(); 
}


