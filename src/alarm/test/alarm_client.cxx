#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
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

using namespace std;
using namespace SAFplus;

void tressTest(int eventNum);
void deRegisterTestCase();
static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmServerHandle;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
void registerTestCase();
void testAllFeature();
void raiseAllAlarm();
void raiseAlarm_Assert_Clear();
void raiseAlarm_Assert(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
void raiseAlarm_Clear(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);

void test_Assert_Soaking_Time();
void test_Clear_Soaking_Time();
void test_Assert_Clear_Soaking_Time();
void test_Assert_Greater_Clear_Soaking_Time_zero();
void test_Assert_Lower_Clear_Soaking_Time_OR();
void test_Assert_Greater_Clear_Soaking_Time_OR();
void test_Assert_Raise_Clear_Soaking_Time_OR();
void test_Assert_Clear_Soaking_Time_Suppression_AND();
void test_Assert_Clear_Soaking_Time_Suppression_OR_09();
void test_Assert_Clear_Soaking_Time_Suppression_AND_10();
#define ALARM_CLIENT_PID 50

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
  sic.iocPort = 50;
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
  int count = 0;
  while (1)
  {
    sleep(1);
    count++;
    if (count > 30) break;
  }
  alarmClient.unSubscriber();
  deRegisterTestCase();
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
//make sure indexProfile is in valid range
void raiseAlarm_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::ASSERT);
}
void raiseAlarm_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::CLEAR);
}
void raiseAlarm_Assert_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  raiseAlarm_Assert(indexProfile, severity);
  sleep(2);
  raiseAlarm_Assert(indexProfile, severity);
}

void raiseAlarm_Assert_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  raiseAlarm_Assert(indexProfile, severity);
  sleep(2);
  raiseAlarm_Assert(indexProfile, severity);
}
void raiseAlarm_Assert_Clear_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  raiseAlarm_Assert(indexProfile, severity);
  sleep(2);
  raiseAlarm_Clear(indexProfile, severity);
  sleep(2);
  raiseAlarm_Assert(indexProfile, severity);
}

void raiseAlarm_Assert_Clear_Assert_Assert(const int indexProfile, const AlarmSeverity& severity)
{
  raiseAlarm_Assert(indexProfile, severity);
  sleep(2);
  raiseAlarm_Clear(indexProfile, severity);
  sleep(2);
  raiseAlarm_Assert(indexProfile, severity);
  sleep(2);
  raiseAlarm_Assert(indexProfile, severity);
}
/*
 * test case 1
 * pre condition: raise alarm with period of assert soaking time
 * result: after a period assert soaking time, alarm is raised,then clear alarm
 */
void test_Assert_Soaking_Time()
{
  raiseAlarm_Assert(1);
  sleep(11);
  raiseAlarm_Clear(1);
}
/*
 * test case 2
 * pre condition: raise alarm assert
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Clear_Soaking_Time()
{
  raiseAlarm_Assert(3);
  raiseAlarm_Clear(3);
  sleep(11);
}
/*
 * test case 3
 * pre condition: raise alarm assert
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Assert_Clear_Soaking_Time()
{
  raiseAlarm_Assert(3);
  raiseAlarm_Clear(3);
  sleep(11);
}
/*
 * test case 4
 * pre condition: raise alarm assert time > clear time == 0
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Assert_Greater_Clear_Soaking_Time_zero()
{
  raiseAlarm_Assert(1);
  raiseAlarm_Clear(1);
}
/*
 * test case 5
 * pre condition: raise alarm assert assert time < clear time
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */void test_Assert_Lower_Clear_Soaking_Time_OR()
{
  raiseAlarm_Assert(4);
  sleep(1);
  raiseAlarm_Clear(4);
  sleep(11);
}

/*
 * test case 6
 * pre condition: raise alarm assert time > clear time
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Assert_Greater_Clear_Soaking_Time_OR()
{
  raiseAlarm_Assert(5);
  raiseAlarm_Clear(5);
  sleep(11);
}
/*
 * test case 7
 * pre condition: raise alarm assert time > clear time
 * result: after a period assert soaking time, wait for raise alarm,send clear alarm: expect alarm raised, status=clear
 */
void test_Assert_Raise_Clear_Soaking_Time_OR()
{
  raiseAlarm_Assert(4);
  sleep(10);
  raiseAlarm_Clear(4);
  sleep(11);
}
/*
 * test case 8
 * pre condition: raise alarm assert time > clear time
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Assert_Clear_Soaking_Time_Suppression_AND()
{
  raiseAlarm_Assert(6);
  raiseAlarm_Clear(6);
  sleep(11);
}
/*
 * test case 9
 * pre condition: raise alarm assert time > clear time
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */
void test_Assert_Clear_Soaking_Time_Suppression_OR_09()
{
  raiseAlarm_Assert(7);
  raiseAlarm_Clear(7);
  sleep(11);
}
/*
 * test case 10
 * pre condition: raise alarm assert time > clear time
 * result: after a period assert soaking time,raise alarm, raise clear alarm=> alarm raised, status=clear
 */
void test_Assert_Clear_Soaking_Time_Suppression_AND_10()
{
  raiseAlarm_Assert(6);
  sleep(6);
  raiseAlarm_Clear(6);
  sleep(11);
}
void testAllFeature()
{
  //tc0 hand masking
  raiseAlarm_Assert();
  sleep(20);
  raiseAlarm_Clear();
  //not suppress
  raiseAlarm_Assert(1);
  sleep(20);
  raiseAlarm_Clear(1);
  //tc1
  test_Assert_Soaking_Time();
  //tc2
  test_Clear_Soaking_Time();
  //tc3
  test_Assert_Clear_Soaking_Time();
  //tc4
  test_Assert_Greater_Clear_Soaking_Time_zero();
  //tc5
  test_Assert_Lower_Clear_Soaking_Time_OR();
  //tc6
  test_Assert_Greater_Clear_Soaking_Time_OR();
  //tc7
  test_Assert_Raise_Clear_Soaking_Time_OR();
  //tc8
  test_Assert_Clear_Soaking_Time_Suppression_AND();
  //tc9
  test_Assert_Clear_Soaking_Time_Suppression_OR_09();
  //tc10
  test_Assert_Clear_Soaking_Time_Suppression_AND_10();
}

void tressTest(int eventNum)
{
}
