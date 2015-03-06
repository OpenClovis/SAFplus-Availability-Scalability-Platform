#include <clGroupIpi.hxx>
#include <clGlobals.hxx>
#include <clIocPortList.hxx>
#include <clSafplusMsgServer.hxx>

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;

int main(int argc,char *argv[])
  {
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging
  SAFplus::logSeverity = SAFplus::LOG_SEV_MAX;

  SAFplus::safplusInitialize(SAFplus::LibDep::GRP | SAFplus::LibDep::IOC | SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG);

  SAFplus::safplusMsgServer.init(SAFplusI::GMS_IOC_PORT, MAX_MSGS, MAX_HANDLER_THREADS);

  SAFplusI::GroupServer gs;
  gs.init();

  /* Library should start it */
  SAFplus::safplusMsgServer.Start();
  while(1) { sleep(10000); }
  return 0;
  }
