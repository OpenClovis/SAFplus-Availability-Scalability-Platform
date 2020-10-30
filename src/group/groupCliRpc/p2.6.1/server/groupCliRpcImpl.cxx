#include "groupCliRpc.hxx"
#include <clGroupCli.hxx>

namespace SAFplus {
namespace Rpc {
namespace groupCliRpc {

  groupCliRpcImpl::groupCliRpcImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  groupCliRpcImpl::~groupCliRpcImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  void groupCliRpcImpl::getClusterView(const ::SAFplus::Rpc::groupCliRpc::GetClusterViewRequest* request,
                                ::SAFplus::Rpc::groupCliRpc::GetClusterViewResponse* response)
  {
     std::string view = SAFplus::getClusterView();
     response->set_clusterview(view);
  }

}  // namespace groupCliRpc
}  // namespace Rpc
}  // namespace SAFplus
