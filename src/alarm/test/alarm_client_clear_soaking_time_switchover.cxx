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
    {"resourcetest5", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest5";

static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmServerHandle;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
void test_clear_soaking_time_switchover();
void raiseAlarm_Assert(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
void raiseAlarm_Clear(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
#define ALARM_CLIENT_PID 53
bool g_isCallBack = false;
void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  logDebug("EVT", "MSG", "***********************************************************");
  logDebug("EVT", "MSG", "app Event channel[%s] data [%s]",channelName.c_str(), data.c_str());
  logDebug("EVT", "MSG", "***********************************************************");
  g_isCallBack = true;
}
int main(int argc, char* argv[])
{
  //init
  logEchoToFd = 1; // echo logs to stdout for debugging
  logSeverity = SAFplus::LOG_SEV_MAX;
  sic.iocPort = ALARM_CLIENT_PID;
  safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  clTestGroupInitialize(("Alarm TestSuite client: Please waiting........"));
  logInfo(ALARM, "CLT", "********************Start msg server********************");
  safplusMsgServer.Start();
  logInfo(ALARM, "CLT", "********************Initial alarm lib********************");
  alarmClientHandle = getProcessHandle(ALARM_CLIENT_PID, SAFplus::ASP_NODEADDR);
  logInfo("FLT", "CLT", "********************Initial alarm client*********************");
  alarmClient.initialize(alarmClientHandle);
  alarmClient.createAlarmProfile();
  alarmClient.subscriber();
  sleep(5);
  test_clear_soaking_time_switchover();
  alarmClient.unSubscriber();
  alarmClient.deleteAlarmProfile();
  clTestGroupFinalize();
  return 0;
}
//make sure indexProfile is in valid range
void raiseAlarm_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  std::ostringstream os;
  os<<"index:"<<indexProfile<<" "<<appAlarms[indexApp].MoAlarms[indexProfile].probCause<<" "<< severity;
  logInfo(ALARM, ALARM_ENTITY, "raiseAlarm_Assert:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  if(CL_ERR_INVALID_HANDLE == alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::ASSERT))
  {
    logInfo(ALARM, ALARM_ENTITY,"Server is not stable!");
  }
}
void raiseAlarm_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  std::ostringstream os;
  os<<"index:"<<indexProfile<<" "<<appAlarms[indexApp].MoAlarms[indexProfile].probCause<<" "<< severity;
  logInfo(ALARM, ALARM_ENTITY, "raiseAlarm_Clear:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  if(CL_ERR_INVALID_HANDLE == alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::CLEAR))
  {
    logInfo(ALARM, ALARM_ENTITY,"Server is not stable!");
  }
}

void test_clear_soaking_time_switchover()
{
  //alarm raise after that clear when switch over
  g_isCallBack=false;
  raiseAlarm_Assert(9);
  sleep(1);
  clTest(("TCtest_clear_soaking_time_switchover:expect alarm raised for raiseAlarm_Assert()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(9);
  sleep(41);
}

