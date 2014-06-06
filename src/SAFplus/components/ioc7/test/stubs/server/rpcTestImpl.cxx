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
    //TODO: put your code here 

    // Add your implementation of the RPC method by hand
    SAFplus::Rpc::rpcTest::DataResult* dr = response->mutable_dataresult();
    dr->set_name("serverName");
    dr->set_status(count);
    //response->set_has_dataresult();
    count++;


    done->Run(); // DO NOT removed this line!!! 
  }

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
