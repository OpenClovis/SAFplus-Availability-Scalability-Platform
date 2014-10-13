#include "amfRpc.hxx"
#include <clLogApi.hxx>
#include <clOsalApi.hxx>
#include <string>
#include <sstream>

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
  std::vector<std::string> env;

  std::stringstream ssNodeAddr;
  std::string strCompName("ASP_COMPNAME=");
  std::string strNodeName("ASP_NODENAME=");
  std::string strNodeAddr("ASP_NODEADDR=");

  strCompName.append(request->name());
  env.push_back(strCompName);

  strNodeName.append(SAFplus::ASP_NODENAME);
  env.push_back(strNodeName);

  ssNodeAddr<<SAFplus::ASP_NODEADDR;
  env.push_back(strNodeAddr.append(ssNodeAddr.str()));

  try
    {
    char temp[200];
    Process p = executeProgram(request->command().c_str(), env,Process::InheritEnvironment);
    logInfo("OPS","SRT","Launched Component [%s] as [%s] with process id [%d], working directory [%s]", request->name().c_str(),request->command().c_str(),p.pid,getcwd(temp,200));
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

  void amfRpcImpl::processInfo(const ::SAFplus::Rpc::amfRpc::ProcessInfoRequest* request,
                                ::SAFplus::Rpc::amfRpc::ProcessInfoResponse* response)
  {
  assert(request->has_pid());
  Process p(request->pid());
  try
    {
    std::string cmdline = p.getCmdline();
    response->set_command(cmdline);
    response->set_running(1);
    }
  catch(ProcessError& e)
    {
    response->set_command("");
    response->set_running(0);
    }
  }

  void amfRpcImpl::processFailed(const ::SAFplus::Rpc::amfRpc::ProcessFailedRequest* request,
                                ::SAFplus::Rpc::amfRpc::ProcessFailedResponse* response)
  {
    //TODO: put your code here 
  logInfo("OPS","SRT","Process failed notification");
  response->set_err(0);
  }

}  // namespace amfRpc
}  // namespace Rpc
}  // namespace SAFplus
