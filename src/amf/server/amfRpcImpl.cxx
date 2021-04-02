#include "amfRpc.hxx"
#include "clPortAllocator.hxx"
#include <clLogApi.hxx>
#include <clOsalApi.hxx>
#include <clFaultApi.hxx>
#include <string>
#include <sstream>

extern SAFplus::Handle nodeHandle; //? The handle associated with this node
extern SAFplus::Fault gfault;
bool rebootFlag;

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
  std::stringstream ssPort;
  std::string strCompName("ASP_COMPNAME=");
  std::string strNodeName("ASP_NODENAME=");
  std::string strNodeAddr("ASP_NODEADDR=");
  std::string strPort("SAFPLUS_RECOMMENDED_MSG_PORT=");

  strCompName.append(request->name());
  env.push_back(strCompName);

  strNodeName.append(SAFplus::ASP_NODENAME);
  env.push_back(strNodeName);

  ssNodeAddr<<SAFplus::ASP_NODEADDR;
  env.push_back(strNodeAddr.append(ssNodeAddr.str()));

  int port = SAFplusI::portAllocator.allocPort();
  ssPort<<port;
  env.push_back(strPort.append(ssPort.str()));
  
  try
    {
    char temp[200];
    Process p = executeProgram(request->command().c_str(), env,Process::InheritEnvironment);
    SAFplusI::portAllocator.assignPort(port, p.pid);
    logInfo("OPS","SRT","Launched Component [%s] as [%s] with process id [%d] and recommended msg port [%d], working directory [%s]", request->name().c_str(),request->command().c_str(),p.pid,port,getcwd(temp,200));
    response->set_pid(p.pid);
    response->set_err(0);
    }
  catch (ProcessError& e)
    {
    logInfo("OPS","SRT","Failed to launch Component [%s] as [%s] with error [%s:%d]", request->name().c_str(),request->command().c_str(),strerror(e.osError),e.osError);
    SAFplusI::portAllocator.releasePort(port);
    response->set_pid(0);
    response->set_err(e.osError);
    }

  }

  void amfRpcImpl::stopComponent(const ::SAFplus::Rpc::amfRpc::StopComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::StopComponentResponse* response)
  {
    int32_t compPid = request->pid();
    logDebug("OPS","STOP","sending signal [SIGTERM] to process [%d]", compPid);
    Process p(compPid);
    p.signal(SIGTERM);
  }

  void amfRpcImpl::cleanupComponent(const ::SAFplus::Rpc::amfRpc::CleanupComponentRequest* request,
                                ::SAFplus::Rpc::amfRpc::CleanupComponentResponse* response)
  {

  std::vector<std::string> env;
  std::string strCompName("ASP_COMPNAME=");
  strCompName.append(request->name());
  env.push_back(strCompName);
  try
  {
    logDebug("OPS","CLE","Cleaning up Component [%s] as [%s]", request->name().c_str(),request->command().c_str());
    int status = executeProgramWithTimeout(request->command().c_str(), env,request->timeout(), Process::InheritEnvironment);
    response->set_err(status);
    if (status)
      {
        logError("OPS", "CLE", "Cleanup command [%s] returned error [%d] for Component [%s]", request->command().c_str(), status, request->name().c_str());
      }
    if (request->pid()>0)
    {
       logDebug("OPS", "CLE", "Sending SIGKILL signal to component [%s] with process id [%d]", request->name().c_str(), request->pid());
       kill(request->pid(), SIGKILL);
    }
  }
  catch (ProcessError& e)
  {
    logInfo("OPS","CLE","Failed to cleanup Component [%s] as [%s] with error [%s:%d]", request->name().c_str(),request->command().c_str(),strerror(e.osError),e.osError);
    response->set_err(e.osError);
  }
 }


  void amfRpcImpl::nodeInfo(const ::SAFplus::Rpc::amfRpc::NodeInfoRequest* request,
                                ::SAFplus::Rpc::amfRpc::NodeInfoResponse* response)
  {
    response->set_time(nowMs());
  }

  void amfRpcImpl::processInfo(const ::SAFplus::Rpc::amfRpc::ProcessInfoRequest* request,
                                ::SAFplus::Rpc::amfRpc::ProcessInfoResponse* response)
  {
    //DbgAssert(request->has_pid());
  if (!request->has_pid())  // Improperly formatted RPC call sent -- its asking for processInfo but the process is not specified
    {
      response->set_command(" ");  // indicate that the call was valid
      response->set_running(0);
    }
  Process p(request->pid());
  try
    {
    std::string cmdline = p.getCmdline();
    if (cmdline.size() == 0) cmdline = " ";
    response->set_command(cmdline);
    response->set_running(1);
    }
  catch(ProcessError& e)
    {
      SAFplusI::portAllocator.releasePortByPid(request->pid());
      response->set_command(" ");
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

  void amfRpcImpl::rebootNode(const ::SAFplus::Rpc::amfRpc::RebootNodeRequest* request,
                       ::SAFplus::Rpc::amfRpc::RebootNodeResponse* response)
  {
	  rebootFlag = true;
	  gfault.registerEntity(nodeHandle,FaultState::STATE_DOWN);
	  response->set_err(0);
  }

  void amfRpcImpl::shutdownAmf(const ::SAFplus::Rpc::amfRpc::ShutdownAmfRequest* request,
                                ::SAFplus::Rpc::amfRpc::ShutdownAmfResponse* response)
  {
      char asp_restart_disable_file[CL_MAX_NAME_LENGTH];
      snprintf(asp_restart_disable_file, CL_MAX_NAME_LENGTH-1, "%s/%s", (SAFplus::ASP_RUNDIR[0] != 0) ? SAFplus::ASP_RUNDIR : ".", ASP_RESTART_DISABLE_FILE);
      logDebug("OPS","SHUTDOWN.AMF","Opening file [%s]", asp_restart_disable_file);
      FILE* fp = fopen(asp_restart_disable_file, "w");
      if (fp)
      {
          fclose(fp);
          logDebug("OPS","SHUTDOWN.AMF","Shutdown amf by setting node [%d] fault state DOWN", nodeHandle.getNode());
          gfault.registerEntity(nodeHandle,FaultState::STATE_DOWN);
      }
      else
      {
          logError("OPS","SHUTDOWN.AMF","Opening file [%s] fail. Error code [%d], error text [%s]", asp_restart_disable_file, errno, strerror(errno));
      }
  }

}  // namespace amfRpc
}  // namespace Rpc
}  // namespace SAFplus
