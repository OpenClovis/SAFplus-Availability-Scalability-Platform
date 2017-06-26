#include <EventClient.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>
#include <string>

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

void testAllFeature();
#define EVENT_CLIENT_PID 50

void eventCallback(uintcw_t channelId,EventChannelScope scope,std::string data,int length)
{
	logDebug("EVT", "MSG", "Receive event from event channel with id [%ld]", channelId);
	logDebug("EVT", "MSG", "Event data [%s]", data.c_str());

}
int main(int argc, char* argv[])
{
	logEchoToFd = 1;  // echo logs to stdout for debugging
	logSeverity = LOG_SEV_MAX;
	sic.iocPort     = 50;
	safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
	logSeverity = LOG_SEV_MAX;
	logInfo("FLT","CLT","********************Start msg server********************");
	safplusMsgServer.Start();

	me = getProcessHandle(EVENT_CLIENT_PID,SAFplus::ASP_NODEADDR);
	//eventEntityHandle = getProcessHandle(FAULT_ENTITY_PID,SAFplus::ASP_NODEADDR);
	logInfo("FLT","CLT","********************Initial event client*********************");
	fc.eventInitialize(me,eventCallback);
	//fc.registerFault();
	testAllFeature();
	sleep(1);
	while(0)
	{
		sleep(10000);
	}
	return 0;
}


void testAllFeature()
{

	std::string localChannel = "testLocalchannel";
	std::string globalChannel = "testGlobalchannel";
	std::string localChannel1 = "testLocalchannel";
	std::string testEventData = "this is the test event";

	logInfo("FLT","CLT","********************Test Open local channel *********************");
	fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test Open global channel *********************");
	fc.eventChannelOpen(globalChannel,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test Open local channel (duplicate) *********************");
	fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test Open global channel (duplicate) *********************");
	fc.eventChannelOpen(globalChannel,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test subscriber local channel *********************");
	fc.eventChannelSubscriber(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test subscriber local channel (Duplicate) *********************");
	fc.eventChannelSubscriber(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	//	logInfo("FLT","CLT","********************Test publish local channel without publisher *********************");
	//	fc.eventChannelSubscriberRpc(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	//	sleep(1);

	logInfo("FLT","CLT","********************Test publisher local channel *********************");
	fc.eventChannelPublish(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test publisher local channel (Duplicate) *********************");
	fc.eventChannelPublish(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test public event local channel *********************");
	fc.eventPublish(testEventData,22,localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test unSubscriber local channel *********************");
	fc.eventChannelUnSubscriber(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test unSubscriber local channel (Duplicate) *********************");
	fc.eventChannelUnSubscriber(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	logInfo("FLT","CLT","********************Test close local channel *********************");
	fc.eventChannelClose(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);
	logInfo("FLT","CLT","********************Test Reopen local channel *********************");
	fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);
	logInfo("FLT","CLT","********************Subscriber local channel *********************");
	fc.eventChannelSubscriber(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);

	while(1)
	{
		sleep(1);
	}





}

