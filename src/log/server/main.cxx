#include <syslog.h>

#include <vector>
using namespace std;

#include <clMgtRoot.hxx>
#include <clMgtApi.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clLogIpi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clGlobals.hxx>

using namespace SAFplus;
using namespace SAFplusI;

class MgtMsgHandler : public SAFplus::MsgHandler
{
  public:
    virtual void msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      Mgt::Msg::MsgMgt mgtMsgReq;
      mgtMsgReq.ParseFromArray(msg, msglen);
      MgtRoot::getInstance()->mgtMessageHandler.msgHandler(from, svr, msg, msglen, cookie);
    }
};
MgtMsgHandler msghandle;


int main(int argc, char* argv[])
{
  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::LOG_IOC_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 10;

  safplusInitialize(SAFplus::LibDep::MSG|SAFplus::LibDep::GRP|SAFplus::LibDep::NAME, sic);

  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  safplusMsgServer.registerHandler(SAFplusI::CL_MGT_MSG_TYPE,&msghandle,NULL);

  logServerInitialize();

  // Log processing Loop
  while(1)
    {
      logServerProcessing();
    }

}
