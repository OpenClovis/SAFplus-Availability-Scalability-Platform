// Generated by the protocol buffer compiler.
#pragma once
#include <string>

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <clRpcService.hxx>
#include "amfAppRpc.pb.hxx"

namespace SAFplus {
  namespace Rpc {
    class RpcChannel;
  }
}

namespace SAFplus {
namespace Rpc {
namespace amfAppRpc {
class amfAppRpc_Stub;

class amfAppRpc : public SAFplus::Rpc::RpcService {
 protected:
  // This class should be treated as an abstract interface.
  inline amfAppRpc() {};
 public:
  virtual ~amfAppRpc();

  typedef amfAppRpc_Stub Stub;

  static const ::google::protobuf::ServiceDescriptor* descriptor();


  // implements amfAppRpcImpl ----------------------------------------------
  virtual void heartbeat(const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response);
  virtual void terminate(const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response);
  virtual void workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request);
  virtual void workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request);
  virtual void proxiedComponentInstantiate(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest* request);
  virtual void proxiedComponentCleanup(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest* request);

  // implements amfAppRpc ------------------------------------------
  virtual void heartbeat(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response,
                       SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));
  virtual void terminate(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response,
                       SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));
  virtual void workOperation(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request);
  virtual void workOperationResponse(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request);
  virtual void proxiedComponentInstantiate(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest* request);
  virtual void proxiedComponentCleanup(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest* request);


  const ::google::protobuf::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  SAFplus::Handle destination,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));
  const ::google::protobuf::Message& GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const;
  const ::google::protobuf::Message& GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(amfAppRpc);
};

class amfAppRpc_Stub : public amfAppRpc {
 public:
  amfAppRpc_Stub(SAFplus::Rpc::RpcChannel* channel);
  amfAppRpc_Stub(SAFplus::Rpc::RpcChannel* channel,
                   ::google::protobuf::Service::ChannelOwnership ownership);
  ~amfAppRpc_Stub();

  inline SAFplus::Rpc::RpcChannel* channel() { return channel_; }


  // implements amfAppRpc ------------------------------------------
  void heartbeat(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response,
                       SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));
  void terminate(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response,
                       SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));
  void workOperation(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request);
  void workOperationResponse(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request);
  void proxiedComponentInstantiate(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest* request);
  void proxiedComponentCleanup(SAFplus::Handle destination,
                       const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest* request);
 private:
  SAFplus::Rpc::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(amfAppRpc_Stub);
};

class amfAppRpcImpl : public amfAppRpc {
 public:
  amfAppRpcImpl();
  ~amfAppRpcImpl();


  // implements amfAppRpcImpl ----------------------------------------------
  void heartbeat(const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response);
  void terminate(const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response);
  void workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request);
  void workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request);
  void proxiedComponentInstantiate(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest* request);
  void proxiedComponentCleanup(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest* request);
};

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
