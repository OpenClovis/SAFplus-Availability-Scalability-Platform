#include <clLogApi.hxx>
#include "amfRpcImpl.hxx"

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

  void amfRpcImpl::startComponent(::google::protobuf::RpcController* controller,
                                const ::SAFplus::Rpc::amfRpc::StartComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StartComponentResponse* response,
                                ::google::protobuf::Closure* done)
    {
    //TODO: put your code here 
    logInfo("RPC","SVR", "Starting component [%s] with command [%s]", request->name().c_str(), request->command().c_str());
    response->set_err(1);
    response->set_pid(0);
    done->Run(); // DO NOT removed this line!!! 
    }

  void amfRpcImpl::stopComponent(::google::protobuf::RpcController* controller,
                                const ::SAFplus::Rpc::amfRpc::StopComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StopComponentResponse* response,
                                ::google::protobuf::Closure* done)
  {
    //TODO: put your code here 
    logInfo("RPC","SVR", "Stopping component running as pid [%d]", request->pid());
    response->set_err(1);
    done->Run(); // DO NOT removed this line!!! 
  }

}  // namespace amfRpc
}  // namespace Rpc
}  // namespace SAFplus
