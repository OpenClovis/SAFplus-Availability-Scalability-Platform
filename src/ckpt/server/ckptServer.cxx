#include <clCommon.hxx>
#include <clGlobals.hxx>

using namespace SAFplus;

SAFplus::SafplusMsgServer safplusMsgServer(ComponentId::Checkpoint);

bool quit=false;


class CkptServerMsgHandler:public MsgHandlerI
  {
  public:
    virtual ~CkptServerMsgHandler();
    virtual void MsgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
  };


CkptServerMsgHandler* handler;

void ckptServer(void)
{
  handler = new CkptServerMsgHandler();
  
  safplusMsgServer.registerHandler(MsgProtocols::Checkpoint, handler, 0);
}


CkptServerMsgHandler::~CkptServerMsgHandler()
{
  safplusMsgServer.removeHandler(MsgProtocols::Checkpoint);
}

void CkptServerMsgHandler::MsgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  clLogInfo("received message of length [%d] from [%d]", msglen, from);
}


void ckptServerQuit(void)
{
  quit = true;
  safplusMsgServer.removeHandler(MsgProtocols::Checkpoint);
  CkptServerMsgHandler* tmp = handler;
  handler = nullptr;
  delete tmp;
}
