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

  void amfAppRpcImpl::workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request)
    {
    ScopedAllocation allocated;
    Handle hdl;
    memcpy(&hdl,request->componenthandle().c_str(),sizeof(Handle));
    logInfo("AMF","RPC","Work Operation on component [%lx.%lx]",hdl.id[0],hdl.id[1]);
    SaNameT compName;
    SAFplus::saNameSet(&compName,request->componentname().c_str());
    if (SAFplusI::amfSession)
      {
      uint tgt = request->target();
      if (tgt != SAFplusI::AMF_CSI_REMOVE_ONE)
        {
        SaAmfHAStateT haState = (SaAmfHAStateT) request->operation();
        SaAmfCSIDescriptorT csiDescriptor;  // TODO
        csiDescriptor.csiFlags = tgt;
        csiDescriptor.csiName.length = 0;
        if (haState == SA_AMF_HA_ACTIVE)
          {
          csiDescriptor.csiStateDescriptor.activeDescriptor.transitionDescriptor = SA_AMF_CSI_NEW_ASSIGN; // TODO
          csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.length = 0;
          }
        else if (haState == SA_AMF_HA_STANDBY)
          {
          csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.length = 0;  // TODO
          csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank = 0;  // TODO
          }

        csiDescriptor.csiAttr.number = request->keyvaluepairs_size();
        if (csiDescriptor.csiAttr.number > SAFplusI::AmfMaxComponentServiceInstanceKeyValuePairs)  // Sanity check this value to be robust even if AMF sends me garbage.
          {
          csiDescriptor.csiAttr.number = 0;
          csiDescriptor.csiAttr.attr = NULL;
          }
        else
          {
          allocated = malloc(csiDescriptor.csiAttr.number * sizeof(SaAmfCSIAttributeT));
          csiDescriptor.csiAttr.attr = (SaAmfCSIAttributeT*) allocated.memory;
          for (int i = 0; i<csiDescriptor.csiAttr.number; i++)
            {
            const SAFplus::Rpc::amfAppRpc::KeyValuePairs& kvp = request->keyvaluepairs(i);
            csiDescriptor.csiAttr.attr[i].attrName = (SaUint8T*) kvp.thekey().c_str();
            csiDescriptor.csiAttr.attr[i].attrValue = (SaUint8T*) kvp.thevalue().c_str();
            }
          }

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
    }

  void amfAppRpcImpl::workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request)
  {
    //TODO: put your code here 
  logInfo("AMF","RPC","work Operation response");
  }

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
