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
    {"resourcetest2", 100, ManagedResource0AlmProfile},

    {"",0,NULL}
};
std::string myresourceId = "resourcetest2";

static unsigned int MAX_MSGS = 25;
static unsigned int MAX_HANDLER_THREADS = 2;
SafplusInitializationConfiguration sic;
SAFplus::Handle alarmServerHandle;
SAFplus::Handle alarmClientHandle;
Alarm alarmClient;
void testSummary();
void unit_test_Raise_Alarm();
void raiseAlarm_Assert(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
void raiseAlarm_Clear(const int indexProfile = 0, const AlarmSeverity& severity = AlarmSeverity::MINOR);
//tc00
void test_Assert_Clear();
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
#define ALARM_CLIENT_PID 52
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
  clTestGroupInitialize(("Alarm TestSuite: alarm client. Please waiting........\n"));
  testSummary();
  unit_test_Raise_Alarm();
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
  logInfo(ALARM, ALARM_ENTITY, "raiseAlarm_Assert:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  int retry = 0;
  while(retry < RETRY)
  {
    if(CL_ERR_INVALID_HANDLE == alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::ASSERT))
    {
      retry++;
      std::cout<<"Server is not stable send back assert!retry:"<<retry<<std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }else break;
  }
}
void raiseAlarm_Clear(const int indexProfile, const AlarmSeverity& severity)
{
  int indexApp = 0;
  std::ostringstream os;
  os<<"index:"<<indexProfile<<" "<<appAlarms[indexApp].MoAlarms[indexProfile].probCause<<" "<< severity;
  logInfo(ALARM, ALARM_ENTITY, "raiseAlarm_Clear:%s %s",appAlarms[indexApp].resourceId.c_str(),os.str().c_str());
  int retry = 0;
  while(retry < RETRY)
  {
    if(CL_ERR_INVALID_HANDLE == alarmClient.raiseAlarm(appAlarms[indexApp].resourceId.c_str(), appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause, severity, 0, AlarmState::CLEAR))
    {
      retry++;
      std::cout<<"Server is not stable send back clear!retry:"<<retry<<std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }else break;
  }
}
/*
 * test case 0
 * pre condition: raise alarm and receive callback
 * result: alarm is raised,then clear alarm
 */
void test_Assert_Clear()
{
  g_isCallBack = false;
  raiseAlarm_Assert(0);
  sleep(1);
  raiseAlarm_Clear(0);
}
/*
 * test case 1
 * pre condition: raise alarm with period of assert soaking time
 * result: after a period assert soaking time, alarm is raised,then clear alarm
 */
void test_Assert_Soaking_Time()
{
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
  raiseAlarm_Assert(1);
  raiseAlarm_Clear(1);
}
/*
 * test case 5
 * pre condition: raise alarm assert assert time < clear time
 * result: after a period assert soaking time, raise clear alarm=> no alarm raised, status=clear
 */void test_Assert_Lower_Clear_Soaking_Time_OR()
{
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
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
  g_isCallBack = false;
  raiseAlarm_Assert(6);
  sleep(6);
  raiseAlarm_Clear(6);
  sleep(11);
}
void unit_test_Raise_Alarm()
{
    //test case 0
    test_Assert_Clear();
    //clTest(("TC00:expect alarm raised for test_Assert_Clear()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
    //test case 1
    test_Assert_Soaking_Time();
    //clTest(("TC01:expect alarm raised for test_Assert_Soaking_Time()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
    //tc2
    test_Clear_Soaking_Time();
    //clTest(("TC02:expect alarm raised for test_Clear_Soaking_Time()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc3
    test_Assert_Clear_Soaking_Time();
    //clTest(("TC03:expect alarm raised for test_Assert_Clear_Soaking_Time()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc4
    test_Assert_Greater_Clear_Soaking_Time_zero();
    //clTest(("TC04:expect alarm raised for test_Assert_Greater_Clear_Soaking_Time_zero()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc5
    test_Assert_Lower_Clear_Soaking_Time_OR();
    //clTest(("TC05:expect alarm raised for test_Assert_Lower_Clear_Soaking_Time_OR()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc6
    test_Assert_Greater_Clear_Soaking_Time_OR();
    //clTest(("TC06:expect alarm raised for test_Assert_Greater_Clear_Soaking_Time_OR()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc7
    test_Assert_Raise_Clear_Soaking_Time_OR();
    //clTest(("TC07:expect alarm raised for test_Assert_Raise_Clear_Soaking_Time_OR()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
    //tc8
    test_Assert_Clear_Soaking_Time_Suppression_AND();
    //clTest(("TC08:expect alarm raised for test_Assert_Clear_Soaking_Time_Suppression_AND()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc9
    test_Assert_Clear_Soaking_Time_Suppression_OR_09();
    //clTest(("TC09:expect alarm raised for test_Assert_Clear_Soaking_Time_Suppression_OR_09()"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
    //tc10
    test_Assert_Clear_Soaking_Time_Suppression_AND_10();
    //clTest(("TC10:expect alarm raised for test_Assert_Clear_Soaking_Time_Suppression_AND_10()"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));

}
/*
 * test case 11
 * pre condition: , ,assert-clear change severity,assert_soaking_time clear,assert-clearsoaking time
 * result:
 */
void testSummary()
{
  int countTotalCritical = 0;
  int countTotalMajor = 0;
  int countTotalMinor = 0;
  int countTotalWarning = 0;
  int countTotalClear = 0;

  int countClearCritical = 0;
  int countClearMajor = 0;
  int countClearMinor = 0;
  int countClearWarning = 0;
  int countClearClear = 0;

  //raise alarm assert-clear 1 1
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::CRITICAL);
  countTotalCritical++;//critical
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary01"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(0,AlarmSeverity::CRITICAL);
  countClearCritical++;//critical


  //countTotal=1,CRITICAL:countClear=1
  //assert-assert-changed-severity-clear
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::CRITICAL);
  countTotalCritical++;
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary02"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(0,AlarmSeverity::MAJOR);
  countClearCritical++;


  //countTotal=1,CRITICAL:countClear=1
  //assert-assert-changed-severity-clear
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::CRITICAL);
  countTotalMajor++;
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary03"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::MAJOR);
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary03_1"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(0,AlarmSeverity::MAJOR);
  countClearMajor++;


  //countTotal=1,CRITICAL:countClear=1
  //assert-assert-changed-severity-clear
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::CRITICAL);
  countTotalMajor++;
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary04"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  g_isCallBack = false;
  raiseAlarm_Assert(0,AlarmSeverity::MAJOR);
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary04_1"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(0,AlarmSeverity::MAJOR);
  sleep(1);
  raiseAlarm_Clear(0,AlarmSeverity::MAJOR);
  countClearMajor++;


  //countTotal=1,CRITICAL:countClear=1
  //assert-soaking time-changed-severity-clear,no changed count severity
  g_isCallBack = false;
  raiseAlarm_Assert(1,AlarmSeverity::CRITICAL);
  //clTest(("TC Summary:expect alarm raised for testSummary05"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
  countTotalCritical++;
  sleep(1);
  raiseAlarm_Clear(1,AlarmSeverity::MAJOR);
  countClearCritical++;
  sleep(1);

  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert
  g_isCallBack = false;
  raiseAlarm_Assert(2,AlarmSeverity::CRITICAL);
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary06"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(2,AlarmSeverity::CRITICAL);
  countTotalCritical++;
  countClearCritical++;
  sleep(11);


  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert
  g_isCallBack = false;
  raiseAlarm_Assert(3,AlarmSeverity::CRITICAL);
  sleep(1);
  //clTest(("TC Summary:expect alarm raised for testSummary07"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  countTotalCritical++;
  countClearCritical++;
  sleep(11);

  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert-soaking time
  g_isCallBack = false;
  raiseAlarm_Assert(3,AlarmSeverity::CRITICAL);
  sleep(11);
  //clTest(("TC Summary:expect alarm raised for testSummary08"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  countTotalCritical++;
  countClearCritical++;
  sleep(11);


  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert
  g_isCallBack = false;
  raiseAlarm_Assert(3,AlarmSeverity::CRITICAL);
  sleep(11);
  //clTest(("TC Summary:expect alarm raised for testSummary09"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  sleep(11);
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  sleep(11);
  countTotalCritical++;
  countClearCritical++;

  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert
  g_isCallBack = false;
  raiseAlarm_Assert(3,AlarmSeverity::CRITICAL);
  sleep(11);
  //clTest(("TC Summary:expect alarm raised for testSummary10"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  g_isCallBack = false;
  raiseAlarm_Assert(3,AlarmSeverity::CRITICAL);
  sleep(11);
  //clTest(("TC Summary:expect alarm raised for testSummary10_1"),g_isCallBack==false , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  sleep(11);
  raiseAlarm_Clear(3,AlarmSeverity::MAJOR);
  sleep(11);
  countTotalCritical++;
  countClearCritical++;


  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  //raise assert soaking time-changed-severity-clear after assert
  g_isCallBack = false;
  raiseAlarm_Assert(4,AlarmSeverity::CRITICAL);
  sleep(6);
  //clTest(("TC Summary:expect alarm raised for testSummary11"),g_isCallBack==true , ("g_isCallBack is %d", g_isCallBack));
  raiseAlarm_Clear(4,AlarmSeverity::MAJOR);
  countTotalCritical++;
  countClearCritical++;
  sleep(13);


  //countTotal=1,CRITICAL:countClear=1 MAJOR:countClear=1
  raiseAlarm_Clear(5,AlarmSeverity::MAJOR);
  sleep(8);
  std::cout<<"\n********************************************\n";
  std::cout<<"Total critical:"<<countTotalCritical<<" Total cleared:"<<countClearCritical<<std::endl;
  std::cout<<"Total major:"<<countTotalMajor<<" Total cleared:"<<countClearMajor<<std::endl;
  std::cout<<"Total minor:"<<countTotalMinor<<" Total cleared:"<<countClearMinor<<std::endl;
  std::cout<<"Total warning:"<<countTotalWarning<<" Total cleared:"<<countClearWarning<<std::endl;
  std::cout<<"Total clear:"<<countTotalClear<<" Total cleared:"<<countClearClear<<std::endl;
  std::cout<<"********************************************\n";
}


