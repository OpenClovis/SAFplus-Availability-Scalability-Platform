#include "amfAppRpc.hxx"
#include <clLogApi.hxx>
#include <clNameApi.hxx>

#include <clAmfIpi.hxx>

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
  SaNameT compName;
  saNameSet(compName,request->componentname());
  if (SAFplusI::amfSession)
    {
    uint tgt = request->target();
    if (tgt != SAFplusI::AMF_CSI_REMOVE_ONE)
      {
      SaAmfCSIDescriptorT csiDescriptor;  // TODO
      csiDescriptor.csiFlags = 0;
      csiDescriptor.csiName.length = 0;
      
      if (SAFplusI::amfSession->callbacks.saAmfCSISetCallback)
        {
        SAFplusI::amfSession->callbacks.saAmfCSISetCallback((SaInvocationT) request->invocation(),&compName,(SaAmfHAStateT) request->operation(), csiDescriptor);
        }
      }
    else if (SAFplusI::amfSession->callbacks.saAmfCSIRemoveCallback)
      {
      SaNameT csiName;
      SaAmfCSIFlagsT csiFlags = 0; // TODO
      csiName.length = 0;
      SAFplusI::amfSession->callbacks.saAmfCSIRemoveCallback((SaInvocationT) request->invocation(),&compName,&csiName,csiFlags);
      }
    }
  
  response->set_invocation(request->invocation());
  response->set_result(SA_AIS_OK);
  }

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
