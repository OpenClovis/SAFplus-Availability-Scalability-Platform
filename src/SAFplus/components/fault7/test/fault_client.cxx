#include <clFaultIpi.hxx>
#include <clGlobals.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
#include <string>

using namespace std;
using namespace SAFplus;

//ClUint32T clAspLocalId = 0x1;
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;
ClBoolT   gIsNodeRepresentative = false;

int main(int argc, char* argv[])
{
  SAFplus::ASP_NODEADDR = 1;
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 60;

  safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);

  //safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  faultInitialize();

  SAFplus::Handle me = Handle::create();
  Fault fc;
  ClIocAddress server;
  server.iocPhyAddress.nodeAddress= SAFplus::ASP_NODEADDR;
  server.iocPhyAddress.portId=  50;
  fc.init(me,server,sic.iocPort,BLOCK);
  FaultEventData faultData;
  faultData.alarmState=SAFplusI::AlarmStateT::ALARM_STATE_INVALID;
  faultData.category=SAFplusI::AlarmCategoryTypeT::ALARM_CATEGORY_COMMUNICATIONS;
  faultData.cause=SAFplusI::AlarmProbableCauseT::ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
  faultData.severity=SAFplusI::AlarmSeverityTypeT::ALARM_SEVERITY_MINOR;

  fc.sendFaultEventMessage(me,SAFplusI::FaultMessageTypeT::MSG_ENTITY_FAULT,SAFplus::FaultPolicy::Custom,faultData);




  while(1) { sleep(10000); }
  return 0;

}
