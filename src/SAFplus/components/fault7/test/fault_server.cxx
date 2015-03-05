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
  logSeverity = LOG_SEV_DEBUG;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::FLT_IOC_PORT;
  safplusInitialize(SAFplus::LibSet::MSG|SAFplus::LibDep::LOG|SAFplus::LibDep::IOC|SAFplus::LibDep::GRP ,sic);
  //safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();
  SAFplus::FaultServer fs;
  logInfo("FLT","CLT","Initial fault server");
  fs.init();
  sleep(2);
  fs.fsmServer.removeAll();
  while(1) { sleep(10000); }
  return 0;

}
