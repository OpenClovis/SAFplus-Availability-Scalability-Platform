
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clTestApi.hxx>
#include <clMgtApi.hxx>
#include <clNameApi.hxx>

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
  char cwd[256];
  mgtCreate("/safplusLog/streamConfig/stream/test");
  std::string logname = mgtGet("/safplusLog/streamConfig/stream/test/name");
  std::string fileName = mgtGet("/safplusLog/streamConfig/stream/test/fileName");
  std::string fileLocation = mgtGet("/safplusLog/streamConfig/stream/test/fileLocation");
  printf("name:%s file:%s loc:%s\n",logname.c_str(),fileName.c_str(),fileLocation.c_str());

  mgtSet("/safplusLog/streamConfig/stream/test/fileName","testLog");
  mgtSet("/safplusLog/streamConfig/stream/test/fileLocation",getcwd(cwd,256));
  mgtSet("/safplusLog/streamConfig/stream/test/fileUnitSize","1024");
  mgtSet("/safplusLog/streamConfig/stream/test/maximumFilesRotated","2");

  Handle testLog = name.getHandle("test");
  printf("test stream handle is [%" PRIx64 ":%" PRIx64 "]\n", testLog.id[0], testLog.id[1]);
  clTest(("new stream has a handle"), testLog != INVALID_HDL,("handle is invalid"));
}

int main(int argc, char* argv[])
{
  SAFplus::logCompName = "testLog";
  clTestGroupInitialize(("log"));
  logInitialize();
  TestLog_basic();

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 86;

  safplusInitialize(SAFplus::LibDep::NAME | SAFplus::LibDep::MGT_ACCESS,sic);
  TestLog_mgt();
  clTestGroupFinalize(); 
}


