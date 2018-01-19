#include <EventClient.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>
#include <string>
#include <clTestApi.hxx>
using namespace std;
using namespace SAFplus;

//ClUint32T clAspLocalId = 0x1;
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;
ClBoolT   gIsNodeRepresentative = false;
SafplusInitializationConfiguration sic;
SAFplus::Handle eventEntityHandle;
SAFplus::Handle me;
EventClient fc;
#define TEST_EVENT_GLOBAL_CHANNEL "ALARM_CHANNEL_NAME"
void testAllFeature();
#define EVENT_CLIENT_PID 56

void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  logDebug("EVT", "MSG", "************************************************************************" );
  logDebug("EVT", "MSG", "Receive event from event channel with name [%s]", channelName.c_str());
  logDebug("EVT", "MSG", "Event data [%s]", data.c_str());
  logDebug("EVT", "MSG", "************************************************************************" );
  //fc.eventPublish(data, data.length(), TEST_EVENT_GLOBAL_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
}


int main(int argc, char* argv[])
{
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;
    sic.iocPort     = EVENT_CLIENT_PID;
    safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
    clTestGroupInitialize(("Alarm TestSuite: alarm client. Please waiting........\n"));
    logSeverity = LOG_SEV_MAX;
    logInfo("FLT","CLT","********************Start msg server********************");
    safplusMsgServer.Start();
    me = getProcessHandle(EVENT_CLIENT_PID,SAFplus::ASP_NODEADDR);
    logInfo("FLT","CLT","********************Initial event client*********************");
    fc.eventInitialize(me,&eventCallback);
    fc.eventChannelSubscriber(TEST_EVENT_GLOBAL_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
    fc.eventChannelPublish(TEST_EVENT_GLOBAL_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
   //testAllFeature();

    logInfo("FLT","CLT","******************** please wait *********************");
    int count = 0;
    while(1)
    {
        sleep(3);
        if(count > 1) break;
        count++;
    }
    fc.eventChannelUnPublish(TEST_EVENT_GLOBAL_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
    fc.eventChannelUnSubscriber(TEST_EVENT_GLOBAL_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
    logInfo("FLT","CLT","********************end program*********************");
    clTestGroupFinalize();
    return 0;
}

void testAllFeature()
{
  std::string testdata = "send messsage to server";
  logInfo("FLT","CLT","********************Open local channel *********************");
  fc.eventPublish(testdata,testdata.length(),TEST_EVENT_GLOBAL_CHANNEL,EventChannelScope::EVENT_LOCAL_CHANNEL);
}

