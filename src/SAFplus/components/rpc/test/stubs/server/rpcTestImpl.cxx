#include "rpcTestImpl.hxx"

namespace SAFplus {
namespace Rpc {
namespace rpcTest {

  rpcTestImpl::rpcTestImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  rpcTestImpl::~rpcTestImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  static int count=0;

  void rpcTestImpl::testGetRpcMethod(::google::protobuf::RpcController* controller,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response,
                                ::google::protobuf::Closure* done)
  {
    // Add your implementation of the RPC method by hand
    SAFplus::Rpc::rpcTest::DataResult* dr = response->mutable_dataresult();
    dr->set_name("testGetRpcMethod");
    dr->set_status(count);
    count++;

    done->Run(); // DO NOT removed this line!!! 
  }

  void rpcTestImpl::testGetRpcMethod2(::google::protobuf::RpcController* controller,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response,
                                ::google::protobuf::Closure* done)
  {
    //TODO: put your code here 
    SAFplus::Rpc::rpcTest::DataResult* dr = response->mutable_dataresult();
    dr->set_name("testGetRpcMethod2");
    dr->set_status(count);
    count++;

    done->Run(); // DO NOT removed this line!!! 
  }

  void rpcTestImpl::testGetRpcMethod3(::google::protobuf::RpcController* controller,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response,
                                ::google::protobuf::Closure* done)
  {
    //TODO: put your code here 
    SAFplus::Rpc::rpcTest::DataResult* dr = response->mutable_dataresult();
    dr->set_name("testGetRpcMethod3");
    dr->set_status(count);
    count++;

    done->Run(); // DO NOT removed this line!!! 
  }

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
