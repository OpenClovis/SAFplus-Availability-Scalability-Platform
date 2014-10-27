#include <clFaultIpi.hxx>
#include <clGlobals.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>
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


  faultInitialize();

  SAFplus::Handle me = Handle::create();
  SAFplus::Fault fc;
  ClIocAddress server;
  server.iocPhyAddress.nodeAddress=1;
  server.iocPhyAddress.portId=50;

  fc.init(me,server,sic.iocPort,BLOCK);


  while(1) { sleep(10000); }
  return 0;

}
