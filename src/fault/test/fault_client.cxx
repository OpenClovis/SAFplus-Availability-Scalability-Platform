#include <clFaultApi.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>
#include <string>

using namespace std;
using namespace SAFplus;

//ClUint32T clAspLocalId = 0x1;
void tressTest(int eventNum);
void deRegisterTest();
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;
ClBoolT   gIsNodeRepresentative = false;
SafplusInitializationConfiguration sic;
SAFplus::Handle faultEntityHandle;
SAFplus::Handle me;
Fault fc;

void testAllFeature();
#define FAULT_CLIENT_PID 200
#define FAULT_ENTITY_PID 201
int main(int argc, char* argv[])
{
    logEchoToFd = 1;  // echo logs to stdout for debugging
    logSeverity = LOG_SEV_MAX;
    sic.iocPort     = 50;
    safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);
    logSeverity = LOG_SEV_MAX;
    logInfo("FLT","CLT","********************Start msg server********************");
    safplusMsgServer.Start();
    logInfo("FLT","CLT","********************Initial fault lib********************");
    faultInitialize();
    me = getProcessHandle(FAULT_CLIENT_PID,SAFplus::ASP_NODEADDR);
    faultEntityHandle = getProcessHandle(FAULT_ENTITY_PID,SAFplus::ASP_NODEADDR);
    logInfo("FLT","CLT","********************Initial fault client*********************");
    fc.init(me,INVALID_HDL,sic.iocPort,BLOCK);
    //fc.registerFault();
    testAllFeature();
    tressTest(5);
    sleep(1);
    deRegisterTest();
    while(0)
    {
        sleep(10000);
    }
    return 0;
}

void testAllFeature()
{

    sleep(2);
    FaultState state = fc.getFaultState(me);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    // Register other fault entity
    logInfo("FLT","CLT","Register fault entity");
    sleep(2);
    fc.registerEntity(faultEntityHandle,FaultState::STATE_DOWN);
    sleep(2);
    FaultState state1 = fc.getFaultState(faultEntityHandle);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state1)]);
    sleep(2);
    logInfo("FLT","CLT","Update fault entity");
    fc.registerEntity(faultEntityHandle,FaultState::STATE_UP);
    sleep(2);
    state = fc.getFaultState(faultEntityHandle);
    logInfo("FLT","CLT","Get current fault state after updated in shared memory [%s]", strFaultEntityState[int(state)]);
    sleep(10);
    FaultEventData faultData;
    faultData.alarmState=SAFplus::AlarmState::ALARM_STATE_INVALID;
    faultData.category=SAFplus::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS;
    faultData.cause=SAFplus::AlarmProbableCause::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
    faultData.severity=SAFplus::AlarmSeverity::ALARM_SEVERITY_MINOR;
    state = fc.getFaultState(me);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    logInfo("FLT","CLT","Send fault event to local fault server");
    fc.notify(faultData,SAFplus::FaultPolicy::Custom);
    sleep(10);
    logInfo("FLT","CLT","Send fault event to fault server");
    fc.notify(faultData,SAFplus::FaultPolicy::AMF);
    sleep(10);
    logInfo("FLT","CLT","Get current fault state in shared memory [%s]", strFaultEntityState[int(state)]);
    state = fc.getFaultState(me);
    logInfo("FLT","CLT","Send fault event of fault entity fault server");
    fc.notify(faultEntityHandle,faultData,SAFplus::FaultPolicy::Custom);
    sleep(1);
}

void tressTest(int eventNum)
{
    for (int i = 0; i < eventNum; i ++)
    {
        FaultEventData faultData;
        faultData.alarmState=SAFplus::AlarmState::ALARM_STATE_INVALID;
        faultData.category=SAFplus::AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS;
        faultData.cause=SAFplus::AlarmProbableCause::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
        faultData.severity=SAFplus::AlarmSeverity::ALARM_SEVERITY_MINOR;
        fc.notify(faultData,FaultPolicy::AMF);
        sleep(1);
    }
}

void deRegisterTest()
{
    logInfo("FLT","CLT","Deregister fault entity");
    fc.deRegister(faultEntityHandle);
    sleep(2);
    logInfo("FLT","CLT","Deregister fault client");
    fc.deRegister();
}
