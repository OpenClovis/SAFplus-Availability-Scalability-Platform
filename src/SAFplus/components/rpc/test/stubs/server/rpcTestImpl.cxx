#include "rpcTest.hxx"

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

  void rpcTestImpl::testGetRpcMethod(const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response)
  {
      response->mutable_dataresult()->set_name("Hello 1");
      response->mutable_dataresult()->set_status(1);
    //TODO: put your code here 
  }

  void rpcTestImpl::testGetRpcMethod2(const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response)
  {
      response->mutable_dataresult()->set_name("Hello 2");
      response->mutable_dataresult()->set_status(2);
    //TODO: put your code here 
  }

  void rpcTestImpl::testGetRpcMethod3(const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response)
  {
      response->mutable_dataresult()->set_name("Hello 3");
      response->mutable_dataresult()->set_status(3);
    //TODO: put your code here 
  }

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
