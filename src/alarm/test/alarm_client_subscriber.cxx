#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
#include <thread>
#include <string>
#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCkptApi.hxx>
#include <clTestApi.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include "test.hxx"
#include <clTestApi.hxx>

using namespace std;
using namespace SAFplus;

AlarmComponentResAlarms appAlarms [] =
{
    {"resourcetest_subscriber", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest_subscriber";

static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmServerHandle;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
#define ALARM_CLIENT_PID 60
bool g_isCallBack = false;
void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  logInfo("EVT", "MSG", "***********************************************************");
  logInfo("EVT", "MSG", "app Event channel[%s] data [%s]",channelName.c_str(), data.c_str());
  logInfo("EVT", "MSG", "***********************************************************");
  g_isCallBack = true;
}


int main(int argc, char* argv[])
{
  //init
  logEchoToFd = 1; // echo logs to stdout for debugging
  logSeverity = SAFplus::LOG_SEV_MAX;
  sic.iocPort = ALARM_CLIENT_PID;
  safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  clTestGroupInitialize(("Alarm TestSuite: alarm client. Please waiting........\n"));
  logInfo(ALARM, "CLT", "********************Start msg server********************");
  safplusMsgServer.Start();
  logInfo(ALARM, "CLT", "********************Initial alarm lib********************");
  alarmClientHandle = getProcessHandle(ALARM_CLIENT_PID, SAFplus::ASP_NODEADDR);
  logInfo("FLT", "CLT", "********************Initial alarm client*********************");
  alarmClient.initialize(alarmClientHandle);
  alarmClient.subscriber();
  sleep(1);
  while(true)
  {
    boost::this_thread::sleep_for( boost::chrono::seconds(1));
  }
  alarmClient.unSubscriber();
  clTestGroupFinalize();
  return 0;
}
