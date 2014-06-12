#include "amfRpc.hxx"

namespace SAFplus {
namespace Rpc {
namespace amfRpc {

  amfRpcImpl::amfRpcImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  amfRpcImpl::~amfRpcImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  void amfRpcImpl::startComponent(SAFplus::Handle destination,
                                const ::SAFplus::Rpc::amfRpc::StartComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StartComponentResponse* response,
                                SAFplus::Wakeable& wakeable)
  {
    //TODO: put your code here 

    wakeable.wake(1, (void*) response); // DO NOT removed this line!!! 
  }

  void amfRpcImpl::stopComponent(SAFplus::Handle destination,
                                const ::SAFplus::Rpc::amfRpc::StopComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StopComponentResponse* response,
                                SAFplus::Wakeable& wakeable)
  {
    //TODO: put your code here 

    wakeable.wake(1, (void*) response); // DO NOT removed this line!!! 
  }

}  // namespace amfRpc
}  // namespace Rpc
}  // namespace SAFplus
