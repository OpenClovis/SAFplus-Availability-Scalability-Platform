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
    //TODO: put your code here
    SAFplus::Rpc::rpcTest::DataResult *dataResult = response->mutable_dataresult();
    dataResult->set_name("testGetRpcMethod");
    dataResult->set_status(1);
  }

  void rpcTestImpl::testGetRpcMethod2(const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response)
  {
    //TODO: put your code here
    SAFplus::Rpc::rpcTest::DataResult *dataResult = response->mutable_dataresult();
    dataResult->set_name("testGetRpcMethod2");
    dataResult->set_status(2);
  }

  void rpcTestImpl::testGetRpcMethod3(const ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request* request,
                                ::SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response)
  {
    //TODO: put your code here
    SAFplus::Rpc::rpcTest::DataResult *dataResult = response->mutable_dataresult();
    dataResult->set_name("testGetRpcMethod3");
    dataResult->set_status(3);
  }

  void rpcTestImpl::workOperation(const ::SAFplus::Rpc::rpcTest::WorkOperationRequest* request)
  {
    //TODO: put your code here
  }

  void rpcTestImpl::workOperationResponse(const ::SAFplus::Rpc::rpcTest::WorkOperationResponseRequest* request)
  {
    //TODO: put your code here
  }

}  // namespace rpcTest
}  // namespace Rpc
}  // namespace SAFplus
