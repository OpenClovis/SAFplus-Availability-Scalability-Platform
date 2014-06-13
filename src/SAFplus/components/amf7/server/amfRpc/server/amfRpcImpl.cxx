#include "amfRpc.hxx"
#include <clLogApi.hxx>
#include <clOsalApi.hxx>

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

  void amfRpcImpl::startComponent(const ::SAFplus::Rpc::amfRpc::StartComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StartComponentResponse* response)
  {
  //logInfo("RPC","SRT", "Starting component [%s] with command [%s]", request->name().c_str(),request->command().c_str());
  std::vector<std::string> env;
  try
    {
    Process p = executeProgram(request->name(), env);
    logInfo("OPS","SRT","Launched Component [%s] as [%s] with process id [%d]", request->name().c_str(),request->command().c_str(),p.pid);
    response->set_pid(p.pid);
    response->set_err(0);
    }
  catch (ProcessError& e)
    {
    logInfo("OPS","SRT","Failed to launch Component [%s] as [%s] with error [%s:%d]", request->name().c_str(),request->command().c_str(),strerror(e.osError),e.osError);
    response->set_pid(0);
    response->set_err(e.osError);
    }
  }

  void amfRpcImpl::stopComponent(const ::SAFplus::Rpc::amfRpc::StopComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StopComponentResponse* response)
  {
    //TODO: put your code here 
  }

}  // namespace amfRpc
}  // namespace Rpc
}  // namespace SAFplus
