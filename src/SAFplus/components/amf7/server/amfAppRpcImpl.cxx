#include "amfAppRpc/amfAppRpc.hxx"
#include <clLogApi.hxx>
#include <clNameApi.hxx>
//#include <clAmfIpi.hxx>
#include <amfOperations.hxx>
#include "amfAppRpcImplAmfSide.hxx"

namespace SAFplus {

namespace Rpc {
namespace amfAppRpc {

  amfAppRpcImplAmfSide::amfAppRpcImplAmfSide(AmfOperations* amfOps)
  {
    amfOperations = amfOps;
  }

  amfAppRpcImplAmfSide::~amfAppRpcImplAmfSide()
  {
    //TODO: Auto-generated destructor stub
  }

  void amfAppRpcImplAmfSide::heartbeat(const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                                ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response)
  {
    logInfo("AMF","RPC", "heartbeat");
  }

  void amfAppRpcImplAmfSide::terminate(const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                                ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response)
  {
  logInfo("AMF","RPC", "terminate");
  dbgAssert(0); // Should never be received by an AMF
  }

  void amfAppRpcImplAmfSide::workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request)
    {
    logError("AMF","RPC","work Operation");
    dbgAssert(0); // Should never be received by an AMF
    }

  void amfAppRpcImplAmfSide::workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request)
  {
    //TODO: put your code here 
  logInfo("AMF","RPC","work Operation response");

  amfOperations->workOperationResponse(request->invocation(), request->result());
  }

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
