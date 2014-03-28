#ifndef groupMessageHandler_hxx
#define groupMessageHandler_hxx

#include <clSafplusMsgServer.hxx>
#include "groupServer.hxx"
#include <clGlobals.hxx>
#include <clLogApi.hxx>

class GroupMessageHandler:public SAFplus::MsgHandler
{
  public:
    void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
};

#endif // groupMessageHandler_hxx
