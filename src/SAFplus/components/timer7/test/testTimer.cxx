#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "clTimer7.hxx"
#include <clLogApi.hxx>

ClUint32T clAspLocalId = 0x1;
ClBoolT   gIsNodeRepresentative = CL_TRUE;
using namespace SAFplus;
using namespace std;

static ClRcT testTimerCallback1(void *unused)
{
    cout << "Usage: testTimer Callback 1 begin \n";
    sleep(120);
    cout << "Usage: testTimer Callback 1 end \n";
    return CL_OK;
}

static ClRcT testTimerCallback2(void *unused)
{
    cout << "Usage: testTimer Callback 2 begin \n";
    sleep(120);
    cout << "Usage: testTimer Callback 2 end \n";

    return CL_OK;
}
static ClRcT testTimerCallback3(void *unused)
{
    cout << "Usage: testTimer Callback 3 begin \n";
    sleep(120);
    cout << "Usage: testTimer Callback 3 end \n";

    return CL_OK;
}
int main(int argc, char* argv[])
{
  ClRcT rc;

  logInitialize();
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  printf("initialize Timer 7 \n ");
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK || (rc = timerInitialize(NULL)) != CL_OK)
  {
      cout << "testTimer initialize false \n";
      return rc;
  }
  static ClTimerTimeOutT testTimeout = { .tsSec = 130, .tsMilliSec = 0 };
  Timer timerTest1(testTimeout, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT,testTimerCallback1, NULL);
  Timer timerTest2(testTimeout, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT,testTimerCallback2, NULL);
  Timer timerTest3(testTimeout, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT,testTimerCallback3, NULL);

  cout << "testTimer create \n";
  printf("test timer start \n");
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

  if(argc > 1)
  {
    clAspLocalId = atoi(argv[1]);
  }

  while(1)
  {
    ;
  }
}



