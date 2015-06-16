
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clTestApi.hxx>
#include <clMgtApi.hxx>

using namespace SAFplus;


void TestLog_basic(void)
{
  logSeverity = LOG_SEV_DEBUG;
  clTestCaseStart(("LOG-BAS-FNC.TC001: issue some logs"));
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
  clTestSuccess(("logs issued without failure"));
  clTestCaseEnd((" "));
}

void TestLog_mgt(void)
{

}

int main(int argc, char* argv[])
{
  SAFplus::logCompName = "testLog";
  clTestGroupInitialize(("log"));
  logInitialize();
  TestLog_basic();
  TestLog_mgt();
  clTestGroupFinalize(); 
}


