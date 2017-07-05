#include <string>
#include <clTestApi.hxx>

#include <iostream>
#include <clGlobals.hxx>

#include <clMsgPortsAndTypes.hxx>

#include <clSafplusMsgServer.hxx>
#include <clAlarmServerApi.hxx>

using namespace std;
using namespace SAFplus;
static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;

int main(int argc, char* argv[])
{
  logEchoToFd = 1; // echo logs to stdout for debugging
  logSeverity = LOG_SEV_DEBUG;
  SafplusInitializationConfiguration sic;
  sic.iocPort = SAFplusI::ALARM_IOC_PORT;
  safplusInitialize(SAFplus::LibSet::MSG | SAFplus::LibDep::LOG | SAFplus::LibDep::IOC | SAFplus::LibDep::GRP, sic);
  safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();
  SAFplus::AlarmServer as;
  logInfo("FLT", "CLT", "Initial alarm server");
  as.initialize();
  sleep(2);
  while (1)
  {
    sleep(10000);
  }
  return 0;
}
