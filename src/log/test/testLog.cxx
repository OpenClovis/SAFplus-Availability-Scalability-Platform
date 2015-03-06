
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;


void TestLog_basic(void)
{
  logEmergency("LOG","TST","Test Emergency Log");
  logEmergency("LOG","TST","Test Emergency Log2");
  logAlert("LOG","TST","Test Alert Log");

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

void TestLog_mgt(void)
{

}

int main(int argc, char* argv[])
{
  SAFplus::logCompName = "testLog";
  //clTestGroupInitialize(("Osal"));
  logInitialize();
  TestLog_basic();
  TestLog_mgt();
  //clTestGroupFinalize(); 
}


