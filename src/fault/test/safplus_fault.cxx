#include <clFaultApi.hxx>
#include <clGlobals.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
#include <string>

using namespace std;
using namespace SAFplus;

//ClUint32T clAspLocalId = 0x1;
static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;

int main(int argc, char* argv[])
{
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 50;

  safplusInitialize(SAFplus::LibDep::LOG|SAFplus::LibDep::GRP ,sic);
  safplusMsgServer.Start();
  SAFplus::FaultServer fs;
  fs.init();


  SAFplus::Handle meHdl = Handle::create();
  FaultShmEntry fse;
  //test register fault entity
  fse.dependecyNum=0;
  //strncpy(fse.name,"test register fault entity",FAULT_NAME_LEN);
  fse.state=SAFplus::FaultState::STATE_UP; 
  fs.registerFaultEntity(&fse,meHdl,true);
  SAFplus::FaultState state = fs.getFaultState(meHdl);
  sleep(1000);
  //test update fault entity state
  fs.fsmServer.updateFaultHandleState(SAFplus::FaultState::STATE_DOWN,meHdl);
  state = fs.getFaultState(meHdl);
  sleep(1000);	
  //test process fault event in policy
  FaultEventData faultData;  
  //test process fault event in default policy
  fs.processFaultEvent(SAFplus::FaultPolicy::AMF,faultData,meHdl,meHdl);
  sleep(1000);
  //test process fault event in custom policy
  fs.processFaultEvent(SAFplus::FaultPolicy::Custom,faultData,meHdl,meHdl);
  sleep(1000);
  //test remove fault enity
  fs.removeFaultEntity(meHdl,false);


  while(1) { sleep(10000); }
  return 0;

}
