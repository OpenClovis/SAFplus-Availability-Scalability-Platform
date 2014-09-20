#include "amfAppRpc.hxx"
#include <clLogApi.hxx>

namespace SAFplus {
namespace Rpc {
namespace amfAppRpc {

  amfAppRpcImpl::amfAppRpcImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  amfAppRpcImpl::~amfAppRpcImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  void amfAppRpcImpl::heartbeat(const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                                ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response)
  {
  logInfo("AMF","RPC", "heartbeat");
    //TODO: put your code here 
  }

  void amfAppRpcImpl::terminate(const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                                ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response)
  {
  logInfo("AMF","RPC", "terminate");
    //TODO: put your code here 
  }

  void amfAppRpcImpl::workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request,
                                ::SAFplus::Rpc::amfAppRpc::WorkOperationResponse* response)
  {
  logInfo("AMF","RPC","work Operation");
    //TODO: put your code here 
  }

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
