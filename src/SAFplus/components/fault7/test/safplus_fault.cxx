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
ClBoolT   gIsNodeRepresentative = CL_TRUE;

int main(int argc, char* argv[])
{
  SAFplus::ASP_NODEADDR = 1;
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 50;

  safplusInitialize(SAFplus::LibDep::IOC | SAFplus::LibDep::LOG |SAFplus::LibDep::MSG ,sic);

  //safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  SAFplus::FaultServer fs;

  fs.init();


  SAFplus::Handle me = Handle::create();
  FaultShmEntry fse;
  //test register fault entity
  fse.dependecyNum=0;
  strncpy(fse.name,"test register fault entity",FAULT_NAME_LEN);
  fse.state=SAFplus::FaultState::STATE_UP;
  fs.RegisterFaultEntity(&fse,me,false);
  SAFplus::FaultState state = fs.getFaultState(me);

  //test update fault entity state
  fs.fsmServer.updateFaultHandleState(SAFplus::FaultState::STATE_DOWN,me);
  state = fs.getFaultState(me);

  //test process fault event in policy
  FaultEventData faultData;
  //test process fault event in default policy
  fs.processFaultEvent(SAFplus::FaultPolicy::AMF,faultData,me);
  //test process fault event in custom policy
  fs.processFaultEvent(SAFplus::FaultPolicy::Custom,faultData,me);

  //test remove fault enity
  fs.removeFaultEntity(me,false);
  state = fs.getFaultState(me);



  while(1) { sleep(10000); }
  return 0;

}
