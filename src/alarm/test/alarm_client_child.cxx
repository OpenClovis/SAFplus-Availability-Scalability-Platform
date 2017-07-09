#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>
#include <clCkptApi.hxx>
#include <clTestApi.hxx>
#include <StorageAlarmData.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include "test_child.hxx"

using namespace std;
using namespace SAFplus;

void tressTest(int eventNum);
void deRegisterTestCase();
static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
ClBoolT gIsNodeRepresentative = false;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
void registerTestCase();
void testAllFeature();
void raiseAllAlarm();
void raiseAlarm_Assert_Clear();
void raiseAlarm_Assert(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
void raiseAlarm_Clear(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
#define ALARM_CLIENT_PID 51
void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  logDebug("EVT", "MSG", "***********************************************************");
  logDebug("EVT", "MSG", "app Receive event from event channel with id [%s]", channelName.c_str());
  logDebug("EVT", "MSG", "app Event data [%s]", data.c_str());
  logDebug("EVT", "MSG", "***********************************************************");
}

int main(int argc, char* argv[])
{
  logEchoToFd = 1; // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;
  sic.iocPort = ALARM_CLIENT_PID;
  safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  logSeverity = LOG_SEV_MAX;
  logInfo(ALARM, "CLT", "********************Start msg server********************");
  safplusMsgServer.Start();
  logInfo(ALARM, "CLT", "********************Initial alarm lib********************");
  alarmClientHandle = getProcessHandle(ALARM_CLIENT_PID, SAFplus::ASP_NODEADDR);
  logInfo("FLT", "CLT", "********************Initial alarm client*********************");
  alarmClient.initialize(alarmClientHandle);
  alarmClient.subscriber();
  registerTestCase();
  sleep(5);
  testAllFeature();
  sleep(20);
  alarmClient.unSubscriber();
  deRegisterTestCase();
  while (0)
  {
    sleep(1);
  }
  return 0;
}
void registerTestCase()
{
  alarmClient.createAlarmProfile();
}
void deRegisterTestCase()
{
  alarmClient.deleteAlarmProfile();
}
void raiseAllAlarm()
{
  int indexApp = 0;
  int indexProfile = 0;
  if (nullptr != appAlarms)
  {
    while (appAlarms[indexApp].resourceId.compare(myresourceId) != 0)
    {
      if (appAlarms[indexApp].resourceId.size() == 0)
      {
        break; //end array
      }
      indexApp++;
    }
    if (nullptr != appAlarms[indexApp].MoAlarms)
    {
      while (nullptr != appAlarms[indexApp].MoAlarms)
      {
        sleep(1);
        alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, AlarmSeverity::MINOR, 0, AlarmState::ASSERT);
        alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, AlarmSeverity::MINOR, 0, AlarmState::CLEAR);

        if (AlarmCategory::INVALID == appAlarms[indexApp].MoAlarms[indexProfile].category)
        {
          logDebug(ALARM, ALARM_ENTITY, "Profile size is %d", indexProfile);
          break;
        }
        indexProfile++;
      } //while
    } //if
  } //if
}
void raiseAlarm_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  //int indexProfile = 0;
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::ASSERT);
}
void raiseAlarm_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::CLEAR);
}
void raiseAlarm_Assert_Assert()
{
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
  sleep(2);
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
}
void raiseAlarm_Assert_Clear()
{
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
  sleep(2);
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
}
void raiseAlarm_Assert_Clear_Assert()
{
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
  sleep(2);
  raiseAlarm_Clear(0, AlarmSeverity::MINOR);
  sleep(2);
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
}

void raiseAlarm_Assert_Clear_Assert_Assert()
{
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
  sleep(2);
  raiseAlarm_Clear(0, AlarmSeverity::CRITICAL);
  sleep(2);
  raiseAlarm_Assert(0, AlarmSeverity::MAJOR);
  sleep(2);
  raiseAlarm_Assert(0, AlarmSeverity::MINOR);
}
void testAllFeature()
{
  //test hand masking
  sleep(5);
  raiseAlarm_Assert();
  sleep(5);
  raiseAlarm_Clear();
  sleep(20);
  raiseAlarm_Assert(1);
  sleep(5);
  raiseAlarm_Clear(1);
}

void tressTest(int eventNum)
{
}

void deRegisterTest()
{
  /*logInfo("FLT","CLT","Deregister fault entity");
   fc.deRegister(faultEntityHandle);
   sleep(2);
   logInfo("FLT","CLT","Deregister fault client");
   fc.deRegister();*/
}

