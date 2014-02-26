
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#define this_error_indicates_missing_parens_around_string(...) __VA_ARGS__

// I need ot override the output of the test macros because Osal is so fundamental that the test routines use it!
#define clTestPrintAt(__file, __line, __function, x) do { printf( this_error_indicates_missing_parens_around_string x); } while(0)
#define clTestGroupInitialize(name) printf("Running test group %s\n", name)
#define clTestGroupFinalize() do { } while(0)

//#include <clTestApi.h>


using namespace SAFplus;


void TestLog_basic(void)
{
  //clTestCaseStart(("Basic Log Test"));

  logEmergency("LOG","TST","Test Emergency Log");
  logEmergency("LOG","TST","Test Emergency Log2");
  logAlert("LOG","TST","Test Alert Log");
  
  //clTestCaseEnd((" "));
  logSeverity = LOG_SEV_CRITICAL;
  logCritical("LOG","TST","Test critical Log");
  logError("LOG","TST","this log should not appear");
  logSeverity = LOG_SEV_ERROR;
  logError("LOG","TST","this log should appear");
  logDebug("LOG","TST","this log should not appear");

  logSeverity = LOG_SEV_TRACE;
  logError("LOG","TST","this log should appear");
  logDebug("LOG","TST","this log should appear");
  
}



int main(int argc, char* argv[])
{
  SAFplus::logCompName = "testLog";
  //clTestGroupInitialize(("Osal"));
  logInitialize();
  TestLog_basic();
  //clTestGroupFinalize(); 
}


