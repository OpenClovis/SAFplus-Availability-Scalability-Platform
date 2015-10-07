#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Timer.hxx"
#include <clLogApi.hxx>

ClUint32T clAspLocalId = 0x1;
ClBoolT   gIsNodeRepresentative = CL_TRUE;
using namespace SAFplus;
using namespace std;
#define CL_OK                    0x00


static ClRcT testTimerCallback1(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 1 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 1 end");
  return CL_OK;
}

static ClRcT testTimerCallback2(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 2 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 2 end");


  return CL_OK;
}
static ClRcT testTimerCallback3(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 3 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 3 end");

  return CL_OK;
}

static ClRcT testTimerCallback4(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 4 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 4 end");

  return CL_OK;
}
static ClRcT testTimerCallback5(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 5 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 5 end");

  return CL_OK;
}
static ClRcT testTimerCallback6(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 6 begin");
  sleep(2);
  logInfo("TIMER7","CREATE", "testTimer Callback 6 end");

  return CL_OK;
}
static ClRcT testTimerCallback7(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 7 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 7 end");

  return CL_OK;
}
static ClRcT testTimerCallback8(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 8 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 8 end");

  return CL_OK;
}
static ClRcT testTimerCallback9(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 9 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 9 end");

  return CL_OK;
}
static ClRcT testTimerCallback10(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 10 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 10 end");

  return CL_OK;
}
static ClRcT testTimerCallback11(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 11 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 11 end");
  return CL_OK;
}
static ClRcT testTimerCallback12(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 12 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 12 end");
  return CL_OK;
}
static ClRcT testTimerCallback13(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 13 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 13 end");
  return CL_OK;
}
static ClRcT testTimerCallback14(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 14 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 14 end");
  return CL_OK;
}
static ClRcT testTimerCallback15(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 15 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 15 end");
  return CL_OK;
}
static ClRcT testTimerCallback16(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 16 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 16 end");
  return CL_OK;
}
static ClRcT testTimerCallback17(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 17 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 17 end");
  return CL_OK;
}
static ClRcT testTimerCallback18(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 18 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 18 end");
  return CL_OK;
}
static ClRcT testTimerCallback19(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 19 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 19 end");
  return CL_OK;
}
static ClRcT testTimerCallback20(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 20 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 20 end");
  return CL_OK;
}
static ClRcT testTimerCallback21(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 21 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 21 end");
  return CL_OK;
}
static ClRcT testTimerCallback22(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 22 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 22 end");
  return CL_OK;
}
static ClRcT testTimerCallback23(void *unused)
{
  logInfo("TIMER7","CREATE", "testTimer Callback 23 begin");
  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer Callback 23 end");
  return CL_OK;
}

int main(int argc, char* argv[])
{
  ClRcT rc;
  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;
  if ((rc = timerInitialize(NULL)) != CL_OK)
  {
    cout << "testTimer initialize false \n";
    return rc;
  }
  static TimerTimeOutT testTimeout;
  testTimeout.tsMilliSec=0;
  testTimeout.tsSec=1;
  Timer timerTest23;
/*
  Timer timerTest1(testTimeout, TimerTypeT::TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback1, NULL);
  Timer timerTest2(testTimeout, TimerTypeT::TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback2, NULL);
  Timer timerTest3(testTimeout, TimerTypeT::TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback3, NULL);
  Timer timerTest4(testTimeout, TimerTypeT::TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback4, NULL);
  Timer timerTest5(testTimeout, TimerTypeT::TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback5, NULL);
  Timer timerTest6(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback6, NULL);
  Timer timerTest7(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback7, NULL);
  Timer timerTest8(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback8, NULL);
  Timer timerTest9(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback9, NULL);
  Timer timerTest10(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback10, NULL);
  Timer timerTest11(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback11, NULL);
  Timer timerTest12(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback12, NULL);
  Timer timerTest13(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback13, NULL);
  Timer timerTest14(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback14, NULL);
  Timer timerTest15(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback15, NULL);
  Timer timerTest16(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback16, NULL);
  Timer *timerTest17= new Timer(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback17, NULL);
  Timer timerTest18(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback18, NULL);
  Timer timerTest19(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback19, NULL);
  Timer timerTest20(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback20, NULL);
  Timer timerTest21(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback21, NULL);
  Timer timerTest22(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback22, NULL);
*/
  timerTest23.timerCreate(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,testTimerCallback23, NULL);
/*
  rc= timerTest1.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest2.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest3.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest4.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);
  rc= timerTest5.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);
  rc= timerTest6.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest7.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest8.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest9.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest10.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest11.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest12.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest13.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest14.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest15.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest16.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest17->timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer1 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest18.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer2 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest19.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest20.timerStart();
  if( CL_OK != rc )
  {
    cout << "testTimer3 start false \n";
    return rc;
  }
  sleep(1);

  rc= timerTest21.timerStart();
  if( CL_OK != rc )
  {   cout << "testTimer3 start false \n";
  return rc;
  }
  sleep(1);

  rc= timerTest22.timerStart();
  if( CL_OK != rc )
  {   cout << "testTimer3 start false \n";
  return rc;
  }sleep(1);


  rc= timerTest23.timerStart();
  if( CL_OK != rc )
  {   cout << "testTimer3 start false \n";
  return rc;
  }

  sleep(3);
  logInfo("TIMER7","CREATE", "testTimer 17 stop");
  timerTest17->timerStop();
  logInfo("TIMER7","CREATE", "testTimer 17 delete");
  delete timerTest17;

  if(argc > 1)
  {
    clAspLocalId = atoi(argv[1]);
  }
*/
  while(1)
  {
    sleep(1);
  }
}



