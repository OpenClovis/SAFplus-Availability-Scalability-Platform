#include "rpcTest.hxx"

namespace SAFplus {
namespace Rpc {
namespace rpcTest {

namespace {

const ::google::protobuf::ServiceDescriptor* rpcTest_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_Rpc_rpcTest_2eproto() {
  protobuf_AddDesc_rpcTest_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "rpcTest.proto");
  GOOGLE_CHECK(file != NULL);
  rpcTest_descriptor_ = file->service(0);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_Rpc_rpcTest_2eproto);
}

}  // namespace
rpcTest::~rpcTest() {}

const ::google::protobuf::ServiceDescriptor* rpcTest::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return rpcTest_descriptor_;
}

const ::google::protobuf::ServiceDescriptor* rpcTest::GetDescriptor() {
  protobuf_AssignDescriptorsOnce();
  return rpcTest_descriptor_;
}

void rpcTest::testGetRpcMethod(SAFplus::Handle destination,
                         const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest*,
                         ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse*,
                         SAFplus::Wakeable& wakeable) {
  logError("RPC","SVR","Method testGetRpcMethod() not implemented.");
  wakeable.wake(1, (void*)nullptr); // DO NOT removed this line!!! 
}

void rpcTest::testGetRpcMethod2(SAFplus::Handle destination,
                         const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request*,
                         ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response*,
                         SAFplus::Wakeable& wakeable) {
  logError("RPC","SVR","Method testGetRpcMethod2() not implemented.");
  wakeable.wake(1, (void*)nullptr); // DO NOT removed this line!!! 
}

void rpcTest::testGetRpcMethod3(SAFplus::Handle destination,
                         const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request*,
                         ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response*,
                         SAFplus::Wakeable& wakeable) {
  logError("RPC","SVR","Method testGetRpcMethod3() not implemented.");
  wakeable.wake(1, (void*)nullptr); // DO NOT removed this line!!! 
}

void rpcTest::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                             SAFplus::Handle destination,
                             const ::google::protobuf::Message* request,
                             ::google::protobuf::Message* response,
                             SAFplus::Wakeable& wakeable) {
  GOOGLE_DCHECK_EQ(method->service(), rpcTest_descriptor_);
  switch(method->index()) {
    case 0:
      testGetRpcMethod(destination,
             ::google::protobuf::down_cast<const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest*>(request),
             ::google::protobuf::down_cast< ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse*>(response),
             wakeable);
      break;
    case 1:
      testGetRpcMethod2(destination,
             ::google::protobuf::down_cast<const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request*>(request),
             ::google::protobuf::down_cast< ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response*>(response),
             wakeable);
      break;
    case 2:
      testGetRpcMethod3(destination,
             ::google::protobuf::down_cast<const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request*>(request),
             ::google::protobuf::down_cast< ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response*>(response),
             wakeable);
      break;
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      break;
  }
}

const ::google::protobuf::Message& rpcTest::GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest::default_instance();
    case 1:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request::default_instance();
    case 2:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);
  }
}

const ::google::protobuf::Message& rpcTest::GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse::default_instance();
    case 1:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response::default_instance();
    case 2:
      return ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);
  }
}

rpcTest_Stub::rpcTest_Stub(SAFplus::Rpc::RpcChannel* channel)
  : channel_(channel), owns_channel_(false) {}
rpcTest_Stub::rpcTest_Stub(
    SAFplus::Rpc::RpcChannel* channel,
    ::google::protobuf::Service::ChannelOwnership ownership)
  : channel_(channel),
    owns_channel_(ownership == ::google::protobuf::Service::STUB_OWNS_CHANNEL) {}
rpcTest_Stub::~rpcTest_Stub() {
  if (owns_channel_) delete channel_;
}

void rpcTest_Stub::testGetRpcMethod(SAFplus::Handle destination,
                              const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest* request,
                              ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response,
                              SAFplus::Wakeable& wakeable) {
  channel_->CallMethod(descriptor()->method(0),
                       destination, request, response, wakeable);
}
void rpcTest_Stub::testGetRpcMethod2(SAFplus::Handle destination,
                              const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request* request,
                              ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response,
                              SAFplus::Wakeable& wakeable) {
  channel_->CallMethod(descriptor()->method(1),
                       destination, request, response, wakeable);
}
void rpcTest_Stub::testGetRpcMethod3(SAFplus::Handle destination,
                              const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request* request,
                              ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response,
                              SAFplus::Wakeable& wakeable) {
  channel_->CallMethod(descriptor()->method(2),
                       destination, request, response, wakeable);
}

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
