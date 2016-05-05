#include <clGroupIpi.hxx>
#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clSafplusMsgServer.hxx>

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;

int main(int argc,char *argv[])
  {
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;

  SAFplus::SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::GMS_STANDALONE_IOC_PORT;
  sic.msgQueueLen = MAX_MSGS;
  sic.msgThreads  = MAX_HANDLER_THREADS;
  SAFplus::safplusInitialize(SAFplus::LibDep::GRP | SAFplus::LibDep::IOC | SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG,sic);

  SAFplusI::GroupServer gs;
  gs.groupCommunicationPort = sic.iocPort;
  gs.init();

  /* Library should start it */
  SAFplus::safplusMsgServer.Start();
  while(1) { sleep(10000); }
  return 0;
  }
