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

  void rpcTestImpl::testGetRpcMethod(SAFplus::Handle destination,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response,
                                SAFplus::Wakeable& wakeable)
  {
    //TODO: put your code here 
    std::cout << "testGetRpcMethod!" << std::endl;

    wakeable.wake(1, (void*) response); // DO NOT removed this line!!! 
  }

  void rpcTestImpl::testGetRpcMethod2(SAFplus::Handle destination,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response,
                                SAFplus::Wakeable& wakeable)
  {
    //TODO: put your code here 
    std::cout << "testGetRpcMethod2!" << std::endl;

    wakeable.wake(1, (void*) response); // DO NOT removed this line!!! 
  }

  void rpcTestImpl::testGetRpcMethod3(SAFplus::Handle destination,
                                const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response,
                                SAFplus::Wakeable& wakeable)
  {
    //TODO: put your code here 
    std::cout << "testGetRpcMethod3!" << std::endl;

    wakeable.wake(1, (void*) response); // DO NOT removed this line!!! 
  }

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
