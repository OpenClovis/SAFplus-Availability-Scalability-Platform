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

  SaNameT compName;
  SAFplus::saNameSet(&compName,request->componentname().c_str());

  // TODO: add work quiesce logic in here so AMF does not have to micromanage the termination.
  // TODO: protect against double termination requests

  // remove myself from the name server.
  name.set(SAFplus::ASP_COMPNAME,INVALID_HDL,NameRegistrar::MODE_NO_CHANGE);
  
  if ((SAFplusI::amfSession)&&(SAFplusI::amfSession->callbacks.saAmfComponentTerminateCallback))
    {
      SAFplusI::amfSession->callbacks.saAmfComponentTerminateCallback((SaInvocationT) request->invocation(),&compName);
    }
  else
    {
      logWarning("AMF","RPC", "Terminate called but application has no handler.  Exiting...");
      exit(0);
    }

  // No response expected.  App should simply stop
  }

  void amfAppRpcImpl::workOperation(const ::SAFplus::Rpc::amfAppRpc::WorkOperationRequest* request)
    {
    ScopedAllocation allocated;
    Handle hdl;

    if (request->componenthandle().size() != 1)
      {
      logWarning("AMF","RPC","Bad work operation, garbage component handle");
      return;  
      }
    
    memcpy(&hdl,request->componenthandle().Get(0).c_str(),sizeof(Handle));
    logInfo("AMF","RPC","Work Operation on component [%" PRIx64 ":%" PRIx64 "]",hdl.id[0],hdl.id[1]);
    SaNameT compName;
    SAFplus::saNameSet(&compName,request->componentname().c_str());
    if (SAFplusI::amfSession)
      {
      uint tgt = request->target();
      SaAmfHAStateT haState = (SaAmfHAStateT) request->operation();
      if ((int)haState == SAFplusI::AMF_HA_OPERATION_REMOVE)
        {
          if (tgt == SA_AMF_CSI_TARGET_ALL)
            {
                if (SAFplusI::amfSession->callbacks.saAmfCSIRemoveCallback)
                {
                    SaNameT csiName;
                    SaAmfCSIFlagsT csiFlags = 0;
                    strcpy((char*)csiName.value,request->csiname().c_str());
                    csiName.length = request->csiname().length()+1;                    
                    SAFplusI::amfSession->callbacks.saAmfCSIRemoveCallback((SaInvocationT) request->invocation(),&compName,&csiName,csiFlags);
                }
            }
          else   
            {
              // TODO
              assert(0);
            }
        }
      else if (tgt != SAFplusI::AMF_CSI_REMOVE_ONE)
        {
        SaAmfCSIDescriptorT csiDescriptor;
        csiDescriptor.csiFlags = tgt;
        strcpy((char*)csiDescriptor.csiName.value,request->csiname().c_str());
        csiDescriptor.csiName.length = request->csiname().length()+1;
        if (haState == SA_AMF_HA_ACTIVE)
          {
          csiDescriptor.csiStateDescriptor.activeDescriptor.transitionDescriptor = SA_AMF_CSI_NEW_ASSIGN; // TODO
          strcpy((char*)csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.value, request->activecompname().c_str());
          csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.length = request->activecompname().length()+1;
          }
        else if (haState == SA_AMF_HA_STANDBY)
          {
          strcpy((char*)csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.value, request->activecompname().c_str());
          csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.length = request->activecompname().length()+1;
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
      }
    }

  void amfAppRpcImpl::workOperationResponse(const ::SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest* request)
  {
    //TODO: put your code here 
  logInfo("AMF","RPC","work Operation response");
  }

  void amfAppRpcImpl::proxiedComponentInstantiate(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest* request)
  {
    if (SAFplusI::amfSession->callbacks.saAmfProxiedComponentInstantiateCallback)
      {
        SaNameT compName;
        SAFplus::saNameSet(&compName,request->componentname().c_str());
        SAFplusI::amfSession->callbacks.saAmfProxiedComponentInstantiateCallback((SaInvocationT) request->invocation(),&compName);
      }
  }

  void amfAppRpcImpl::proxiedComponentCleanup(const ::SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest* request)
  {
    if (SAFplusI::amfSession->callbacks.saAmfProxiedComponentCleanupCallback)
      {
        SaNameT compName;
        SAFplus::saNameSet(&compName,request->componentname().c_str());
        SAFplusI::amfSession->callbacks.saAmfProxiedComponentCleanupCallback((SaInvocationT) request->invocation(),&compName);
      }
  }

}  // namespace amfAppRpc
}  // namespace Rpc
}  // namespace SAFplus
