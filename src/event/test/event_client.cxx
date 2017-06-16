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
#define EVENT_CLIENT_PID 200

int main(int argc, char* argv[])
{
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;
    sic.iocPort     = 50;
    safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
    logSeverity = LOG_SEV_MAX;
    logInfo("FLT","CLT","********************Start msg server********************");
    safplusMsgServer.Start();
//    logInfo("FLT","CLT","********************Initial event lib********************");
//    faultInitialize();
    me = getProcessHandle(EVENT_CLIENT_PID,SAFplus::ASP_NODEADDR);
    //eventEntityHandle = getProcessHandle(FAULT_ENTITY_PID,SAFplus::ASP_NODEADDR);
    logInfo("FLT","CLT","********************Initial event client*********************");
    fc.eventInitialize(me);
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
    logInfo("FLT","CLT","********************Open local channel *********************");
    std::string localChannel = "testLocalchannel";
    std::string globalChannel = "testGlobalchannel";
    std::string localChannel1 = "testLocalchannel";

	fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);
	logInfo("FLT","CLT","********************Open global channel *********************");
	fc.eventChannelOpen(globalChannel,EventChannelScope::EVENT_GLOBAL_CHANNEL);
	sleep(1);
    logInfo("FLT","CLT","********************Open local channel *********************");
	fc.eventChannelOpen(localChannel,EventChannelScope::EVENT_LOCAL_CHANNEL);
	sleep(1);
}

