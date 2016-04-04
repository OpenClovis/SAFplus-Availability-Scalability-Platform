#include <amfAppRpc.hxx>

namespace SAFplus {

  class AmfOperations;

namespace Rpc {
namespace amfAppRpc {


class amfAppRpcImplAmfSide : public amfAppRpc 
  {
  public:
  amfAppRpcImplAmfSide(AmfOperations* amfOps);
  ~amfAppRpcImplAmfSide();

  AmfOperations* amfOperations;

  // implements amfAppRpcImpl ----------------------------------------------
  void heartbeat(const ::SAFplus::Rpc::amfAppRpc::HeartbeatRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::HeartbeatResponse* response);
  void terminate(const ::SAFplus::Rpc::amfAppRpc::TerminateRequest* request,
                       ::SAFplus::Rpc::amfAppRpc::TerminateResponse* response);
  void workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request);
  void workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request);
  };

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
