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
static EventClient fc;
#define TEST_EVENT_CHANNEL_NAME "ALARM_CHANNEL_NAME"
void testAllFeature();
#define EVENT_CLIENT_PID 55

void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
  logDebug("EVT", "MSG", "***********************************************");
	logDebug("EVT", "MSG", "Receive event from event channel with name [%s]", channelName.c_str());
	logDebug("EVT", "MSG", "Event data [%s]", data.c_str());
	logDebug("EVT", "MSG", "***********************************************");
}

int main(int argc, char* argv[])
{
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;
    sic.iocPort     = EVENT_CLIENT_PID;
    clTestGroupInitialize(("Alarm TestSuite: alarm client. Please waiting........\n"));
    safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
    logSeverity = LOG_SEV_MAX;
    logInfo("FLT","CLT","********************Start msg server********************");
    safplusMsgServer.Start();
    me = Handle::create(); // This is the handle for this specific alarm server
    logInfo("FLT","CLT","********************Initial event client*********************");
    fc.eventInitialize(me,&eventCallback);
    fc.eventChannelOpen(TEST_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_LOCAL_CHANNEL);
    fc.eventChannelPublish(TEST_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_LOCAL_CHANNEL);
    fc.eventChannelSubscriber(TEST_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_LOCAL_CHANNEL);
    logInfo("FLT","CLT","please waiting 1 minute...............................");
    int count = 0;
    while(1)
    {
      sleep(60);
      std::string testdata = "test send messsage to server";
      logInfo("FLT","CLT","********************send message *********************");
      fc.eventPublish(testdata,testdata.length(),TEST_EVENT_CHANNEL_NAME,EventChannelScope::EVENT_LOCAL_CHANNEL);
      if(count > 20)   break;
      count++;
    }
    fc.eventChannelClose(TEST_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_LOCAL_CHANNEL);
    logInfo("FLT","CLT","********************end program*********************");
    clTestGroupFinalize();
    return 0;
}

void testAllFeature()
{

    std::string localChannel = "testLocalchannel";
    std::string globalChannel = "testGlobalchannel";
    std::string localChannel1 = "testLocalchannel";
    std::string testEventData = "this is the test event";

    logInfo("FLT","CLT","********************Open local channel *********************");
    fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
    sleep(1);
    logInfo("FLT","CLT","********************Subscriber local channel *********************");
    fc.eventPublish(testEventData,22,localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
    sleep(1);
    while(1)
    {
       sleep(1);
    }
}
