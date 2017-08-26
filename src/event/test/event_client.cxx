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
std::string globalChannel1 = "ALARM_CHANNEL_NAME";
void testAllFeature();
#define EVENT_CLIENT_PID 50

void eventCallback(const std::string& channelName,const EventChannelScope& scope,const std::string& data,const int& length)
{
	logDebug("EVT", "MSG", "callback Receive event from event channel with id [%s]", channelName.c_str());
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
	std::cout<<"DANGLE:handle client:"<<me.id[0]<<":"<<me.id[1]<<std::endl;
	fc.eventInitialize(me,&eventCallback);
	//fc.registerFault();
	testAllFeature();
	int count = 0;
	//while(1)
	{
	  //count++;
	//	sleep(1);
	//	if(count > 30) break;
	}




	return 0;
}


void testAllFeature()
{

	logInfo("FLT","CLT","********************Test Open global channel *********************");
	fc.eventChannelOpen(globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	logInfo("FLT","CLT","********************Test publisher global channel *********************");
	fc.eventChannelPublish(globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);
	logInfo("FLT","CLT","********************Test subscriber global channel *********************");
	fc.eventChannelSubscriber(globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);
	std::string mytest = "Ming Giang Test Event publish!";
	logInfo("FLT","CLT","********************Test publish data  *********************");
	fc.eventPublish(mytest,mytest.length(),globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);
  logInfo("FLT","CLT","********************Test unSubscriber global channel*********************");
  fc.eventChannelUnSubscriber(globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
  sleep(1);
  logInfo("FLT","CLT","********************Test eventChannelClose global channel *********************");
  fc.eventChannelClose(globalChannel1,EventChannelScope::EVENT_GLOBAL_CHANNEL);
  sleep(1);
 }
