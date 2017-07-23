#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
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
    {"resourcetest", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest";

static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmServerHandle;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
void testhandMasking();
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
  logInfo(ALARM, "CLT", "********************Start msg server********************");
  safplusMsgServer.Start();
  logInfo(ALARM, "CLT", "********************Initial alarm lib********************");
  alarmClientHandle = getProcessHandle(ALARM_CLIENT_PID, SAFplus::ASP_NODEADDR);
  logInfo("FLT", "CLT", "********************Initial alarm client*********************");
  alarmClient.initialize(alarmClientHandle);
  alarmClient.createAlarmProfile();
  alarmClient.subscriber();
  clTestGroupInitialize(("Alarm TestSuite parent: Please waiting........"));
  sleep(5);
  testhandMasking();
  alarmClient.unSubscriber();
  alarmClient.deleteAlarmProfile();
  boost::this_thread::sleep_for( boost::chrono::seconds(21));
  clTestGroupFinalize();
  return 0;
}
//make sure indexProfile is in valid range
void raiseAlarm_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  std::ostringstream os;
  os<<"index:"<<indexProfile<<" "<<appAlarms[indexApp].MoAlarms[indexProfile].probCause<<" "<< severity;
  logDebug(ALARM, ALARM_ENTITY, "raiseAlarm_Assert:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::ASSERT);
}
void raiseAlarm_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  std::ostringstream os;
  os<<"index:"<<indexProfile<<" "<<appAlarms[indexApp].MoAlarms[indexProfile].probCause<<" "<< severity;
  logDebug(ALARM, ALARM_ENTITY, "raiseAlarm_Clear:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::CLEAR);
}

void testhandMasking()
{
  //tc0 hand masking with alarm_client_parent run first alarm_client_child
  g_isCallBack=false;
  raiseAlarm_Assert();
  sleep(3);
  clTest(("TC00:expect alarm raised for raiseAlarm_Assert()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  sleep(20);
  raiseAlarm_Clear();

}


