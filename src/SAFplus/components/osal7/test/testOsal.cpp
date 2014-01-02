
#include <clThreadApi.hpp>

#define this_error_indicates_missing_parens_around_string(...) __VA_ARGS__

// I need ot override the output of the test macros because Osal is so fundamental that the test routines use it!
#define clTestPrintAt(__file, __line, __function, x) do { printf( this_error_indicates_missing_parens_around_string x); } while(0)
#define clTestGroupInitialize(name) printf("Running test group %s\n", name)
#define clTestGroupFinalize() do { } while(0)

#include <clTestApi.h>


using namespace SAFplus;


void TestProcSemT_oneProcess(void)
{
}

void TestProcSemT_basic(void)
{
  clTestCaseStart(("Basic ProcSemT"));
  if (1)
    {
    ProcSemT s1(1,1);
    s1.lock();  /* Should NOT hang here */
    }

  if (1)
    {
    ProcSemT s1(1,0);
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


void testProcSemT(void)
{
  TestProcSemT_basic();
  TestProcSemT_oneProcess();
}



int main(int argc, char* argv[])
{
  clTestGroupInitialize(("Osal"));
  testProcSemT();
  clTestGroupFinalize(); 
}


