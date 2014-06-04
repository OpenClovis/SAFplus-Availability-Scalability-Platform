#pragma once

#include <google/protobuf/service.h>
#include "clSafplusMsgServer.hxx"

namespace SAFplusI
{

class AmfRpcChannel : public google::protobuf::RpcChannel, public SAFplus::MsgHandler
  {
public:
  AmfRpcChannel(SAFplus::MsgServer *svr);
  virtual
  ~AmfRpcChannel();
  void CallMethod(const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done);

  void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
public:
  SAFplus::MsgServer *svr;
  
  //Msg index
  int msgId;
  };

}
