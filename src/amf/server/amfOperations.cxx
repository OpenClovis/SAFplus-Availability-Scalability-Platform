#include <chrono>
#include <google/protobuf/stubs/common.h>
#include <clCommon.hxx>
#include <amfRpc.pb.hxx>
#include <amfRpc.hxx>
#include <amfAppRpc.hxx>
#include <clPortAllocator.hxx>
#include <clRpcChannel.hxx>
#include <SAFplusPBExt.pb.hxx>

#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clOsalApi.hxx>
#include <clAmfPolicyPlugin.hxx>
#include <clAmfIpi.hxx>

#include <SAFplusAmf/Component.hxx>
#include <SAFplusAmf/ComponentServiceInstance.hxx>
#include <SAFplusAmf/ServiceUnit.hxx>
#include <SAFplusAmf/ServiceGroup.hxx>
#include <SAFplusAmf/Data.hxx>
#include <SAFplusAmfModule.hxx>
#include <unordered_map>
#include <boost/range/algorithm.hpp>

//#ifdef SAFPLUS_AMF_LOG_NODE_REPRESENTATIVE
#include "../../log/clLogIpi.hxx"
//#endif

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

extern Handle           nodeHandle; //? The handle associated with this node
extern SAFplus::Fault gfault;
bool rebootFlag;
extern SAFplusAmf::SAFplusAmfModule cfg;

namespace SAFplus
  {
    extern SAFplusAmf::Recovery recommendedRecovery;
    extern bool suEarliestRanking(ServiceUnit* a, ServiceUnit* b);
    extern void updateStateDueToProcessDeath(SAFplusAmf::Component* comp);

    WorkOperationTracker::WorkOperationTracker(SAFplusAmf::Component* c,SAFplusAmf::ComponentServiceInstance* cwork,SAFplusAmf::ServiceInstance* work,uint statep, uint targetp)
    {
    comp = c; csi = cwork; si=work; state = statep; target = targetp;
    issueTime = nowMs();
    }


  // Move this service group and all contained elements to the specified state.
  ClRcT setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt, bool writeChanges)
    {
      //SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
    ClRcT rc = CL_OK;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
    //SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator endsu = sg->serviceUnits.value.end();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = sg->serviceUnits.listEnd();
    for (itsu = sg->serviceUnits.listBegin(); itsu != endsu; itsu++)
      {
      //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
      ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      const std::string& suName = su->name;
      if (su->adminState.value != tgt)
        {
          Node* node = su->node.value;
          if (tgt <= node->adminState.value)
          {
             logInfo("N+M","AUDIT","Setting service unit [%s] to admin state [%s]",suName.c_str(),c_str(tgt));
             if (!writeChanges)
               su->adminState.value = tgt; // do not change the beat, so, the AMF will not write changes again
             else
               su->adminState = tgt; // DO change the beat, so, the AMF will write changes next time
          }
        // TODO: transactional and event
        }       
      }
    if (sg->adminState.value != tgt)
     {
       logInfo("N+M","AUDIT","Setting service group [%s] to admin state [%s]",sg->name.value.c_str(),c_str(tgt));
       if (!writeChanges)
         sg->adminState.value = tgt; // do not change the beat, so, the AMF will not write changes again
       else
         sg->adminState = tgt; // DO change the beat, so, the AMF will write changes next time
     }
     else
       rc = CL_ERR_INVALID_STATE;

    if (rc == CL_OK && writeChanges)
      sg->adminState.write();

    return rc;

    }   

    // Move this node and all contained elements to the specified state.
  ClRcT setAdminState(SAFplusAmf::Node* node,SAFplusAmf::AdministrativeState tgt, bool writeChanges)
  {
    ClRcT rc = CL_OK;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = node->serviceUnits.listEnd();
    for (itsu = node->serviceUnits.listBegin(); itsu != endsu; itsu++)
      {
      ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      const std::string& suName = su->name;
      if (su->adminState.value != tgt)
        {
        ServiceGroup* sg = su->serviceGroup.value;
        if (tgt <= sg->adminState.value)
          {
            logInfo("N+M","AUDIT","Setting service unit [%s] to admin state [%s]",suName.c_str(),c_str(tgt));
            if (!writeChanges)
              su->adminState.value = tgt;
            else
              su->adminState = tgt;
          }
        // TODO: transactional and event
        }
      }
    if (node->adminState.value != tgt)
    {
      logInfo("N+M","AUDIT","Setting node [%s] to admin state [%s]",node->name.value.c_str(),c_str(tgt));
      if (!writeChanges)
        node->adminState.value = tgt;
      else
        node->adminState = tgt;
    }
    else
      rc = CL_ERR_INVALID_STATE;

    if (rc == CL_OK && writeChanges)
      node->adminState.write();

    return rc;
  }

  ClRcT setAdminState(SAFplusAmf::ServiceUnit* su,SAFplusAmf::AdministrativeState tgt, bool writeChanges)
  {
    ClRcT rc = CL_OK;    
    if (su->adminState.value != tgt)
    {
      logInfo("N+M","AUDIT","Setting su [%s] to admin state [%s]",su->name.value.c_str(),c_str(tgt));
      if (!writeChanges)
        su->adminState.value = tgt;
      else
        su->adminState = tgt;
    }   
    else
      rc = CL_ERR_INVALID_STATE;

    if (rc == CL_OK && writeChanges)
      su->adminState.write();

    return rc;
  }
  
  ClRcT setAdminState(SAFplusAmf::ServiceInstance* si,SAFplusAmf::AdministrativeState tgt, bool writeChanges)
  {
    ClRcT rc = CL_OK;    
    if (si->adminState.value != tgt)
    {
      logInfo("N+M","AUDIT","Setting si [%s] to admin state [%s]",si->name.value.c_str(),c_str(tgt));
      if (!writeChanges)
        si->adminState.value = tgt;
      else
        si->adminState = tgt;
    }
    else
      rc = CL_ERR_INVALID_STATE;

    if (rc == CL_OK && writeChanges)
      si->adminState.write();

    return rc;
  }

  // The effective administrative state is calculated by looking at the admin state of the containers: SU, SG, node, and application.  If any contain a "more restrictive" state, then this component inherits the more restrictive state.  The states from most to least restrictive is off, idle, on.  The Application object is optional.
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::Component* comp)
    {
    assert(comp);
    if (!comp->serviceUnit.value) comp->serviceUnit.updateReference(); // find the pointer if it is not resolved

    if ((!comp->serviceUnit.value)||(!comp->serviceUnit.value->node.value)||(!comp->serviceUnit.value->serviceGroup.value))  // This component is not properly hooked up to other entities; it must be off
      {
      logInfo("N+M","AUDIT","Component's [%s] entities are not fully connected",comp->name.value.c_str());
      return SAFplusAmf::AdministrativeState::off;
      }
    ServiceUnit* su = comp->serviceUnit.value;
    return effectiveAdminState(su);
    }

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceUnit* su)
    {
    assert(su);
    if ((!su->node.value)||(!su->serviceGroup.value))  // This component is not properly hooked up to other entities; is must be off
      {
      return SAFplusAmf::AdministrativeState::off;
      }
    Node* node = su->node.value;
    ServiceGroup* sg = su->serviceGroup.value;
    Application* app = sg->application;

    if ((su->adminState.value == SAFplusAmf::AdministrativeState::off) || (sg->adminState.value == SAFplusAmf::AdministrativeState::off)
      || (node->adminState.value == SAFplusAmf::AdministrativeState::off) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::off))
      return SAFplusAmf::AdministrativeState::off;

    if ((su->adminState.value == SAFplusAmf::AdministrativeState::idle) || (sg->adminState.value == SAFplusAmf::AdministrativeState::idle)
      || (node->adminState.value == SAFplusAmf::AdministrativeState::off) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::idle))
      return SAFplusAmf::AdministrativeState::idle;

    if ((su->adminState.value == SAFplusAmf::AdministrativeState::on) || (sg->adminState.value == SAFplusAmf::AdministrativeState::on)
      || (node->adminState.value == SAFplusAmf::AdministrativeState::off) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::on))
      return SAFplusAmf::AdministrativeState::on;
    }

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceGroup* sg)
    {
    assert(sg);
    Application* app = sg->application;

    if ((sg->adminState.value == SAFplusAmf::AdministrativeState::off) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::off))
      return SAFplusAmf::AdministrativeState::off;

    if ( (sg->adminState.value == SAFplusAmf::AdministrativeState::idle) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::idle))
      return SAFplusAmf::AdministrativeState::idle;

    if ((sg->adminState.value == SAFplusAmf::AdministrativeState::on) || (app && app->adminState.value == SAFplusAmf::AdministrativeState::on))
      return SAFplusAmf::AdministrativeState::on;
    }

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceInstance* si)
    {
    // If either SG or SI is off, admin state is off
    if (si->adminState.value == SAFplusAmf::AdministrativeState::off) return SAFplusAmf::AdministrativeState::off;

    ServiceGroup* sg = si->serviceGroup.value;

    SAFplusAmf::AdministrativeState ret;
    ret = effectiveAdminState(sg);

    if (ret == SAFplusAmf::AdministrativeState::off) return ret;
 
    // If either SG or SI is idle, admin state is idle
    if ((ret == SAFplusAmf::AdministrativeState::idle) || (si->adminState.value == SAFplusAmf::AdministrativeState::idle)) return SAFplusAmf::AdministrativeState::idle;
        
    // If its not off or idle, it must be on
    return SAFplusAmf::AdministrativeState::on;
    }    

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ComponentServiceInstance* csi)
  {    
    ServiceInstance* si = csi->serviceInstance.value;

    SAFplusAmf::AdministrativeState ret;
    ret = effectiveAdminState(si);

    return ret;  
  }

  bool ClAmfPolicyPlugin_1::initialize(SAFplus::AmfOperations* amfOperations,SAFplus::Fault* faultp)
    {
    amfOps = amfOperations;
    fault  = faultp;
    return true;
    }  
  
  inline bool operationPendingForComp(SAFplusAmf::Component* comp)
  {
    return (comp->pendingOperation != PendingOperation::none) ? true : false;
  }

  bool operationsPendingForSU(SAFplusAmf::ServiceUnit* su)
  {
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
    
    for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
    {
      Component* comp = dynamic_cast<Component*>(*itcomp);
      assert(comp);

      if(operationPendingForComp(comp) == true) return true; 
    }

    return false;
  }

  bool operationsPendingForSG(SAFplusAmf::ServiceGroup* sg)
  {
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = sg->serviceUnits.listEnd();
    for (itsu = sg->serviceUnits.listBegin(); itsu != endsu; itsu++)
    {
      //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
      SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      assert(su);

      if(operationsPendingForSU(su)) return true;    
    }

    return false;
  }

  bool operationsPendingForSI(SAFplusAmf::ServiceInstance* si)
  {
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itActiveSU;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endActiveSU = si->activeAssignments.listEnd();
    for (itActiveSU = si->activeAssignments.listBegin(); itActiveSU != endActiveSU; itActiveSU++)
    {
      SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itActiveSU);
      assert(su);
      
      if(operationsPendingForSU(su)) return true;
    }

    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itStandbySU;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endStandbySU = si->standbyAssignments.listEnd();
    for (itStandbySU = si->standbyAssignments.listBegin(); itStandbySU != endStandbySU; itStandbySU++)
    {
      SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itStandbySU);
      assert(su);
      
      if(operationsPendingForSU(su)) return true;
    }

    return false;
  }

  bool operationsPendingForNode(SAFplusAmf::Node* node)
  {
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = node->serviceUnits.listEnd();
    for (itsu = node->serviceUnits.listBegin(); itsu != endsu; itsu++)
    {
      SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      assert(su);

      if(operationsPendingForSU(su)) return true;
    }
    return false;
  }

  void AmfOperations::workOperationResponse(uint64_t invocation, uint32_t result)
    {
    if (1)
      {
      // if there is no invocation in the pendingWorkOperations, it means this is from proxied preinstantiable component response ==> no-op for this case      
      if (pendingWorkOperations.find(invocation) == pendingWorkOperations.end()) // saAmfResponse from proxied preinstantiable instatiate
      {
         logInfo("AMF","OPS","workOperationResponse from proxied preinstantiable instatiate. No-op");
         return;
      }
      WorkOperationTracker& wat = pendingWorkOperations.at(invocation);
      logInfo("AMF","OPS","Work Operation response on component [%s] invocation [%" PRIu64 "] result [%d]",wat.comp->name.value.c_str(),invocation, result);
      SAFplusAmf::Component* comp = wat.comp;
      if (comp->compProperty.value == SAFplusAmf::CompProperty::sa_aware && wat.state == (int)HighAvailabilityState::active)
      {
        assignWorkCallback(wat.comp);
      }
      if( wat.state == (int)HighAvailabilityState::active || wat.state == (int)HighAvailabilityState::standby)
      {
          AdministrativeState adminState = effectiveAdminState(comp->serviceUnit.value);
          if(adminState == AdministrativeState::on)
          {
              recommendedRecovery = Recovery::None;
          }
      }
      if ( wat.state == WorkOperationTracker::TerminationState)
        {
          if (result == SA_AIS_OK)
          {
            //wat.comp->pendingOperation = PendingOperation::none;
            //wat.comp->processId = 0;
            // DO update the comp's states because its process actually down now
            updateStateDueToProcessDeath(wat.comp);
          }
          else
          {
            wat.comp->presenceState = PresenceState::terminationFailed;
            // TODO: call abort maybe?
          }
        }
      else if ( wat.state <= (int) HighAvailabilityState::quiescing)
        {
        if (result == SA_AIS_OK)  // TODO: actually, I think I need to call into the redundancy model plugin to correctly process the result.
          {
          assert(wat.comp->pendingOperation == PendingOperation::workAssignment);  // No one should be issuing another operation until this one is aborted

          wat.comp->haState = (SAFplusAmf::HighAvailabilityState) wat.state; // TODO: won't work with multiple assignments of active and standby, for example
          // if (wat.si->assignmentState = AssignmentState::fullyAssigned;  // TODO: for now just make the SI happy to see something work

          // pending operation completed.  Clear it out
          wat.comp->pendingOperationExpiration.value.value = 0;
          wat.comp->pendingOperation = PendingOperation::none;
          }
        }
      else // work removal
        {
        if (result == SA_AIS_OK)  // TODO: actually, I think I need to call into the redundancy model plugin to correctly process the result.
          {
          assert(wat.comp->pendingOperation == PendingOperation::workRemoval);  // No one should be issuing another operation until this one is aborted

          wat.comp->haState = SAFplusAmf::HighAvailabilityState::idle;
          // if (wat.si->assignmentState = AssignmentState::fullyAssigned;  // TODO: for now just make the SI happy to see something work

          // pending operation completed.  Clear it out
          wat.comp->pendingOperationExpiration.value.value = 0;
          wat.comp->pendingOperation = PendingOperation::none;

          if(wat.comp->activeAssignments > 0)--wat.comp->activeAssignments;
          if(wat.comp->standbyAssignments > 0)--wat.comp->standbyAssignments;
          }
        }
      reportChange();
      }
    // TODO: crashes pendingWorkOperations.erase(invocation);
    }

  CompStatus AmfOperations::getCompState(SAFplusAmf::Component* comp)
    {
    assert(comp);
    if (!comp->serviceUnit)
      {
      return CompStatus::Uninstantiated; 
      }
    if (!comp->serviceUnit.value->node)
      {
      return CompStatus::Uninstantiated; 
      }

    try
      {
      Handle nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
      Handle amfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,nodeHdl.getNode());

      int pid = comp->processId.value;
      if (0) // nodeHdl == nodeHandle)  // Handle this request locally
        {
        if (pid == 0)
          {
          return CompStatus::Uninstantiated;
          }
        Process p(pid);
        try
          {
          std::string cmdline = p.getCmdline();
          // Some other process took that PID
          // TODO: if (cmdline != comp->commandLine)  return CompStatus::Uninstantiated;
          }
        catch (ProcessError& pe)
          {
          return CompStatus::Uninstantiated;
          }

        // TODO: Talk to the process to discover its state...
        return CompStatus::Instantiated;
        }
      else  // RPC call
        {
          //logInfo("OP","CMP","Request component [%s] state from node [%s]", comp->name.value.c_str(), comp->serviceUnit.value->node.value->name.value.c_str());
        if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable &&
            comp->presenceState.value == SAFplusAmf::PresenceState::instantiated) // TODO: with NonSAFComponent: don't care comp's pid???
         {
            return CompStatus::Instantiated; 
         }
        ProcessInfoRequest req;
        req.set_pid(pid);
        ProcessInfoResponse resp;
        amfInternalRpc->processInfo(amfHdl,&req, &resp);
        if (!resp.IsInitialized())
          {
            // RPC call is broken, should throw exception
            assert(0);
          }
        if (resp.command().size() == 0)
          {
            // There is no response from the destination because it's down
            // so treat it as stopped, not to assert
            //assert(0);
            resp.set_running(false);
          }
        logInfo("OP","CMP","Request component [%s] pid [%d] state from node [%s (%d)] returned [%s]", comp->name.value.c_str(), pid, comp->serviceUnit.value->node.value->name.value.c_str(), amfHdl.getNode(), resp.running() ? "running" : "stopped");

        if (resp.running()) 
          {
          // TODO: I need to compare the process command line with my command line to make sure that my proc didn't die and another reuse the pid
          return CompStatus::Instantiated;
          }
        else 
          {
            return CompStatus::Uninstantiated;
          }
        }
      }
    catch (NameException& e)
      {
        return CompStatus::Uninstantiated;
      }
    catch (Error& e)
      {
        if (e.clError == Error::DOES_NOT_EXIST) return CompStatus::Uninstantiated; // actually this could be the node does not exist (or hasn't been found yet by this AMF), so this may have to evolve into an "unknown" comp status, or we must make sure that an existing node is always found
        if (e.clError == Error::CONTAINER_DOES_NOT_EXIST) throw;
        throw; // TODO, any other errors?
      }
    }

    class StartCompResp:public SAFplus::Wakeable
    {
    public:
      StartCompResp(SAFplus::Wakeable* w, SAFplusAmf::Component* comp): w(w), comp(comp) {};
      virtual ~StartCompResp(){};
    SAFplus::Rpc::amfRpc::StartComponentResponse response;
    SAFplusAmf::Component* comp;
    SAFplus::Wakeable* w;

    void wake(int amt,void* cookie=NULL)
      {
      if (response.err() != 0)
        {
        comp->processId = 0;
        comp->lastError.value = strprintf("Process spawn failure [%s:%d]", strerror(response.err()));
        comp->presenceState = PresenceState::instantiationFailed;
        }
      else if (comp)
        {
        logInfo("---","---","set comp [%s] pid=%d",comp->name.value.c_str(), response.pid());
        comp->processId = response.pid();

       if ( comp->capabilityModel == CapabilityModel::not_preinstantiable)
          {
          Handle handle=getProcessHandle(comp->processId.value);
          name.set(comp->name, handle, NameRegistrar::MODE_NO_CHANGE); // I need to set the handle because the process itself will not do so.
        
          comp->presenceState.value  = PresenceState::instantiated;    // There are no transitional states in the not_preinstantiable (not SAF aware) case
          }
        else
          {
            comp->presenceState.value  = PresenceState::instantiating;   // When the component sets its handle, this will be marked instantiated.
          }

        }
      if (w) w->wake(1,comp);
      delete this;
      }
    };

  class CleanupCompResp:public SAFplus::Wakeable
    {
    public:
      CleanupCompResp(SAFplus::Wakeable* w, SAFplusAmf::Component* comp): w(w), comp(comp) {};
      virtual ~CleanupCompResp(){};
      SAFplus::Rpc::amfRpc::CleanupComponentResponse response;
      SAFplusAmf::Component* comp;
      SAFplus::Wakeable* w;

      void wake(int amt,void* cookie=NULL)
      {
      if (response.err() != 0)
        {
        comp->lastError.value = strprintf("Process spawn for cleaning up failure [%s:%d]", strerror(response.err()));
        }
      if (w) w->wake(1,comp);
      delete this;
      }
    };

  class WorkOperationResponseHandler:public SAFplus::Wakeable
    {
    public:
    WorkOperationResponseHandler(SAFplus::Wakeable* w, SAFplusAmf::Component* comp): w(w), comp(comp) {};
    virtual ~WorkOperationResponseHandler(){};
    //SAFplus::Rpc::amfAppRpc::WorkOperationResponse response;
    SAFplusAmf::Component* comp;
    SAFplus::Wakeable* w;

    void wake(int amt,void* cookie=NULL)
      {
#if 0
      if (response.err() != 0)
        {
        comp->processId.value = 0;
        comp->lastError.value = strprintf("Process spawn failure [%s:%d]", strerror(response.err()));
        comp->presence.value  = PresenceState::instantiationFailed;
        }
      else if (comp)
        {
        comp->processId.value = response.pid();
        comp->presence.value  = PresenceState::instantiating;
        }
#endif
      if (w) w->wake(1,comp);
      delete this;
      }
    };

    void AmfOperations::removeWork(SAFplusAmf::ServiceInstance* si,Wakeable& w)
    {
      SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::Container assignments = si->activeAssignments.value;     
      for (SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::Container::iterator itsu = assignments.begin(); itsu != assignments.end(); itsu++)
      {
         SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::Elem elem = *itsu;
         SAFplusAmf::ServiceUnit* su = elem.value;         
         removeWork(su,w);         
      }

      assignments = si->standbyAssignments.value;     
      for (SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::Container::iterator itsu = assignments.begin(); itsu != assignments.end(); itsu++)
      {
         SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::Elem elem = *itsu;
         SAFplusAmf::ServiceUnit* su = elem.value;
         removeWork(su,w);         
      }
    }
    
    void AmfOperations::removeWork(ServiceUnit* su, Wakeable& w)
    {
      logDebug("OPS","RMV.WRK","Removing works for su [%s]", su->name.value.c_str());
      assert(su);

      // Update the Service Instances: decrement assignment statistics, and remove this SU from the SI's assigned list.
      // SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator   it;
      //SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator   end = su->assignedServiceInstances.value.end();
      SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator it;
      SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator end = su->assignedServiceInstances.listEnd();
      std::unordered_map<ComponentServiceInstance*, std::vector<Component*>> csiRemovedComps;
      
      for (it = su->assignedServiceInstances.listBegin(); it != end; it++)
        {
          ServiceInstance* si = dynamic_cast<ServiceInstance*>(*it);
          assert(si);
          SAFplusAmf::ComponentServiceInstance* csi = NULL;
          SAFplus::MgtObject::Iterator itcsi;
          SAFplus::MgtObject::Iterator endcsi = si->componentServiceInstances.end();
          if (si->activeAssignments.find(su) != si->activeAssignments.value.end())
            {
              assert(si->numActiveAssignments.current > 0); // Data is inconsistent
              si->numActiveAssignments.current -= 1;
              si->activeAssignments.erase(su);

              for (itcsi = si->componentServiceInstances.begin(); itcsi != endcsi; itcsi++)
              {
                  std::vector<Component*> comps;
                  csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itcsi->second);
                    
                  SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->activeComponents.value;
                  std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator itvec = vec.begin();            
                  for (;itvec != vec.end(); itvec++)
                  {
                     SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itvec;
                     SAFplusAmf::Component* c = elem.value;                     
                     comps.push_back(c);                     
                  }
                  if (comps.size()>0)
                     csiRemovedComps[csi] = comps;
//                  csi->activeComponents.value.clear();
                  for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                  {
                      SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                      SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
                      for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
                      {
                          Component* comp = dynamic_cast<Component*>(*itcomp);
                          if((*iter)->name.value == comp->name.value)
                          {
                              csi->activeComponents.erase(*iter);
                          }

                      }
                  }
              }              
              si->isFullActiveAssignment= false;
            }

          if (si->standbyAssignments.find(su) != si->standbyAssignments.value.end())
            {
              assert(si->numStandbyAssignments.current > 0);  // Data is inconsistent
              si->numStandbyAssignments.current -= 1;
              si->standbyAssignments.erase(su);
              for (itcsi = si->componentServiceInstances.begin(); itcsi != endcsi; itcsi++)
                {
                    csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itcsi->second);
                    
                    std::vector<Component*> comps;
                                      
                    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->standbyComponents.value;
                    std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator itvec = vec.begin();            
                    for (;itvec != vec.end(); itvec++)
                    {
                       SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itvec;
                       SAFplusAmf::Component* c = elem.value;                       
                       comps.push_back(c);                    
                    }
                    if (comps.size()>0)
                       csiRemovedComps[csi] = comps;
//                    csi->standbyComponents.value.clear();
                    for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                    {
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
                        for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
                        {
                            Component* comp = dynamic_cast<Component*>(*itcomp);
                            if((*iter)->name.value == comp->name.value)
                            {
                                csi->standbyComponents.erase(*iter);
                            }

                        }
                    }
                }
              si->isFullStandbyAssignment= false;
            }
          if ((si->numActiveAssignments.current.value == 0)||(si->numStandbyAssignments.current.value == 0)) si->assignmentState = AssignmentState::partiallyAssigned;
          if ((si->numActiveAssignments.current.value == 0)&&(si->numStandbyAssignments.current.value == 0)) si->assignmentState = AssignmentState::unassigned;
        }

      // Update the Service Unit's statistics and SI list.
      su->assignedServiceInstances.clear();
      su->numActiveServiceInstances = 0;
      su->numStandbyServiceInstances = 0;

      // TODO: clear the CSI structures

      // Now actually remove the work from the relevant components
      SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
      SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
      //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
      //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
      for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
        {
          Component* comp = dynamic_cast<Component*>(*itcomp);
          assert(comp);   

          // In multi component service units, we need to remove the work from all the other components. 
          if (!((comp->presenceState == PresenceState::uninstantiated) || (comp->presenceState == PresenceState::instantiationFailed)))
            {
              if ( comp->capabilityModel == CapabilityModel::not_preinstantiable)  // except if it has no work we just kill it
                {
                  abort(comp, true, w);  // TODO, need to consolidate the wakeables
                }
              else
                {
                  // Nothing to do if already dead or never started
                  if (!((comp->presenceState == PresenceState::uninstantiated) || (comp->presenceState == PresenceState::instantiationFailed)))
                    {
                      SAFplus::Rpc::amfAppRpc::WorkOperationRequest request;
                      Handle hdl;
                      try
                        {
                          hdl = name.getHandle(comp->name);
                        }
                      catch (SAFplus::NameException& n)
                        {
                          logCritical("OPS","WRK","Component [%s] is not registered in the name service.  Cannot control it.", comp->name.value.c_str());
                          comp->lastError.value = "Component's name is not registered in the name service so cannot remove work cleanly";
                          // this one should be killed since it can't be controlled
                          abort(comp, true, w);  // TODO, need to consolidate the wakeables
                          // TODO ? anything to set in the comp's status?
                          continue; // Go to the next component
                        }

                      // Mark this component with a pending operation.  This will be cleared in the WorkOperationTracker, or by timeout in auditDiscovery
                      comp->pendingOperationExpiration.value.value = nowMs() + comp->timeouts.workRemoval;
                      comp->pendingOperation = PendingOperation::workRemoval;

                      request.set_componentname(comp->name.value.c_str());
                      request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
                      request.set_operation((uint32_t)SAFplusI::AMF_HA_OPERATION_REMOVE);
                      request.set_target(SA_AMF_CSI_TARGET_ALL);                 
                      if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
                      request.set_invocation(invocation++);
                      //request.set_csiname(csi->name.value);

                      // Now I need to fill up the key/value pairs from the CSI
                      request.clear_keyvaluepairs();

                      pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,nullptr,nullptr,(uint32_t)SAFplusI::AMF_HA_OPERATION_REMOVE,SA_AMF_CSI_TARGET_ALL);

                      // finding CSI for this component
                      bool csiFound = false;
                      for ( auto it = csiRemovedComps.begin(); it != csiRemovedComps.end(); ++it )
                      {
                         ComponentServiceInstance* csi = it->first;
                         std::vector<Component*>& vec =  it->second;
                         
                         for (std::vector<Component*>::iterator itvec = vec.begin();itvec != vec.end();itvec++)
                            {
                               Component* c = *itvec;
                               if (comp->name.value.compare(c->name.value) == 0)
                               {
                                   logDebug("OPS","WRK", "found work removed csi [%s] for comp [%s]", csi->name.value.c_str(), comp->name.value.c_str());
                                   request.set_csiname(csi->name.value);
                                   csiFound = true;
                                   break;
                               }
                           }
                         
                         if (csiFound) break;
                      }
                      amfAppRpc->workOperation(hdl, &request);                                            
                   }
                }
            }
        }
      // Free map     
      csiRemovedComps.clear();
      reportChange();
    }

  bool AmfOperations::suContainsSaAwareComp(SAFplusAmf::ServiceUnit* su)
  {
     SAFplus::MgtObject::Iterator itcomp;
     SAFplus::MgtObject::Iterator endcomp = su->components.end();
     for (itcomp = su->components.begin(); itcomp != endcomp; itcomp++)
     {                        
        Component* comp = dynamic_cast<SAFplusAmf::Component*>(itcomp->second);
        if (comp->compProperty.value == SAFplusAmf::CompProperty::sa_aware)
        {
           return true;
        }
     }
     return false;
  }

  bool AmfOperations::assignWork(ServiceUnit* su, ServiceInstance* si, HighAvailabilityState state,Wakeable& w)
    {
    ComponentServiceInstance* csi = nullptr;

    //assert(su->assignedServiceInstances.contains(si) == false);  // We can only assign a particular SI to a particular SU once.
    bool compAssigned = false;
    //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
    //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();

    for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
      {

      Component* comp = dynamic_cast<Component*>(*itcomp);
      assert(comp);
      if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable &&
          suContainsSaAwareComp(su))
        {
           //Don't count proxied preinstantiable because it never gets assignment
           continue;
        }
      assert(comp->pendingOperation == PendingOperation::none);  // We should not be adding work to a component if something else is happening to it.

      Handle hdl;
      if (comp->capabilityModel != CapabilityModel::not_preinstantiable)  // If the component is preinstantiable, it better be instantiated by now (work would not be assigned if it wasn't)
        {
          try
            {
              hdl = name.getHandle(comp->name);
            }
          catch (SAFplus::NameException& n)
            {
              logCritical("OPS","SRT","Component [%s] is not registered in the name service.  Cannot control it.", comp->name.value.c_str());
              comp->lastError.value = "Component's name is not registered in the name service so address cannot be determined.";
              assert(0);
              break; // TODO: right now go to the next component, however assignment should not occur on ANY components if all components are not accessible. 
            }
        }

      // TODO: let's add an extension to SAF in the SI which basically just says "ignore CSIs" and/or "assign all components to the same CSI".  This makes the model simpler

      // Let's find a CSI that we can apply to this component
      //SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator itcsi;
      //SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator endcsi = si->componentServiceInstances.value.end();
      SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itcsi;
      SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator endcsi = si->componentServiceInstances.listEnd();

      SAFplusAmf::ComponentServiceInstance* csi = NULL;
      for (itcsi = si->componentServiceInstances.listBegin(); itcsi != endcsi; itcsi++)
        {
          csi = *itcsi;
          if (!csi) continue;
          for (std::string csiType : comp->csiTypes.value ) {
            if((csi->type.value.length()>0) && (!(csi->type.value.compare(csiType)))) {
              goto found;// We found one!
            }
          }        
          // TODO: figure out number of assignments allowed if (csi->getComponent()) continue;  // We can't assign a CSI to > 1 component.
          // TODO validate CSI dependencies are assigned
          //break;
        }
    found:
      if (itcsi != endcsi)  // We found an assignable CSI and it is the variable "csi"
        {
        logInfo("OPS","SRT","Component [%s] handle [%" PRIx64 ":%" PRIx64 "] is being assigned [%s] work", comp->name.value.c_str(),hdl.id[0],hdl.id[1], c_str(state));
        if (comp->capabilityModel == CapabilityModel::not_preinstantiable)
          {
            if (state == HighAvailabilityState::active)
             start(comp,w);
          }
        else
          {
            // Mark this component with a pending operation.  This will be cleared in the WorkOperationTracker, or by timeout in auditDiscovery
            comp->pendingOperationExpiration.value.value = nowMs() + comp->timeouts.workAssignment;
            comp->pendingOperation = PendingOperation::workAssignment;
            SAFplus::Rpc::amfAppRpc::WorkOperationRequest request;
            request.set_componentname(comp->name.value.c_str());
            request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
            request.set_operation((uint32_t)state);
            request.set_target(SA_AMF_CSI_ADD_ONE);
            if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
            request.set_invocation(invocation++);
            request.set_csiname(csi->name.value);

            if (state == HighAvailabilityState::active)
              {
                csi->activeComponents.value.push_back(comp);  // Mark this CSI assigned to this component
                ++comp->activeAssignments;
                request.set_activecompname(comp->name.value); // assume that the SG model is 2N (1 active, 1 standby) or 1+N (one active, N standby)
              }
            else if (state == HighAvailabilityState::standby)
              {
                csi->standbyComponents.value.push_back(comp); // Mark this CSI assigned to this component
                ++comp->standbyAssignments;
                SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->activeComponents.value;
                std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator itvec = vec.begin();            
                if (itvec != vec.end())
                {
                   SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itvec;
                   SAFplusAmf::Component* c = elem.value;
                   request.set_activecompname(c->name.value);
                }
              }
            else if (state == HighAvailabilityState::idle)
              {
                assert(0);  // TODO (will this fn call work for work removal?
              }
            else if (state == HighAvailabilityState::quiescing)
              {
                assert(0);  // TODO (will this fn call work for work removal?
              }
          

            // Now I need to fill up the key/value pairs from the CSI
            request.clear_keyvaluepairs();
            SAFplus::MgtList<std::string>::Iterator it;
            for (it = csi->dataList.begin(); it != csi->dataList.end(); it++)
              {
                SAFplus::Rpc::amfAppRpc::KeyValuePairs* kvp = request.add_keyvaluepairs();
                assert(kvp);
                std::string val("val");
                //MgtObject *obj = it->second->getChildObject("val");
                MgtObject *obj2 = it->second;
                //MgtProv<std::string>* kv = dynamic_cast<MgtProv<std::string>*>(obj);
                SAFplusAmf::Data* kv2 =  dynamic_cast<SAFplusAmf::Data*>(obj2); 
                //kv->dbgDump();
                //obj2->dbgDump();
                //assert(kv);
                //kvp->set_thekey(kv->tag.c_str());  // it->first().c_str()
                //kvp->set_thevalue(kv->value.c_str());
                kvp->set_thekey(kv2->name.value.c_str());
                kvp->set_thevalue(kv2->val.value.c_str());
              }

            pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,csi,si,(uint32_t)state,SA_AMF_CSI_ADD_ONE);

            amfAppRpc->workOperation(hdl, &request);

            //assignWorkCallback(comp);
          }

          reportChange();
          //update isFullActiveAssignment and isFullStandbyAssignment for service instance
          si->isFullActiveAssignment.value = si->isFullStandbyAssignment.value = true;
          for(itcsi = si->componentServiceInstances.listBegin(); itcsi != endcsi; itcsi++)
          {
            ComponentServiceInstance* csi=dynamic_cast<ComponentServiceInstance*>(*itcsi);
            if(state == HighAvailabilityState::active && csi->activeComponents.value.empty())
            {
                si->isFullActiveAssignment=false;
                break;
            }
            if(state == HighAvailabilityState::standby&& csi->standbyComponents.value.empty())
            {
                si->isFullStandbyAssignment=false;
                break;
            }
          }
          compAssigned = true;
        }
      else
        {
        logInfo("OPS","SRT","Component [%s] handle [%" PRIx64 ":%" PRIx64 "] cannot be assigned work.  No valid Component Service Instance.", comp->name.value.c_str(),hdl.id[0],hdl.id[1]);
        }

      }
      if (compAssigned) {
        //assert(su->assignedServiceInstances.contains(si) == false);  // We can only assign a particular SI to a particular SU once.
        if(su->assignedServiceInstances.contains(si) == false)su->assignedServiceInstances.push_back(si);
        if (state == HighAvailabilityState::active)
        {
            su->numActiveServiceInstances.current.value=su->assignedServiceInstances.value.size();
        }
        if (state == HighAvailabilityState::standby)
        {
            su->numStandbyServiceInstances.current.value=su->assignedServiceInstances.value.size();
        }
      }
      return compAssigned;
    }


    // TODO: fixme there is no actual response because the application may quit within the callback.
  static  SAFplus::Rpc::amfAppRpc::TerminateResponse terminateResponse;

  void AmfOperations::stop(SAFplusAmf::Component* comp,Wakeable& w)
    {
    Handle hdl;
    if (comp->capabilityModel == CapabilityModel::not_preinstantiable)  // Cannot talk to the component, just kill it
      {
        abort(comp, true,w);
        return;
      }

    try
      {
        hdl = name.getHandle(comp->name);
      }
    catch (SAFplus::NameException& n)
      {
        logCritical("OPS","SRT","Component [%s] is not registered in the name service, so the AMF cannot communicate with it. Substituting kill for stop.", comp->name.value.c_str());
        comp->lastError.value = "Component's name is not registered in the name service so address cannot be determined.";
        abort(comp, true,w);
        return;
      }
        
    if (1)
      {
        // Mark this component with a pending operation.  This will be cleared in the WorkOperationTracker, or by timeout in auditDiscovery
        comp->pendingOperationExpiration.value.value = nowMs() + comp->timeouts.terminate;
        comp->pendingOperation = PendingOperation::shutdown;

        SAFplus::Rpc::amfAppRpc::TerminateRequest request;
        request.set_componentname(comp->name.value.c_str());
        request.set_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
        if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
        request.set_invocation(invocation++);
        pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,NULL,NULL,(uint32_t)WorkOperationTracker::TerminationState,0);

        comp->presenceState = PresenceState::terminating;
        try
          {
          amfAppRpc->terminate(hdl, &request,&terminateResponse, w);
          }
        catch (SAFplus::Error& e)
          {
            if (e.clError == Error::DOES_NOT_EXIST)
              {
                // OK, app terminated with out replying
              }
            else
              {
                throw;
              }
          }
      }

    }

  void AmfOperations::abort(SAFplusAmf::Component* comp, bool changePS, Wakeable& w)
    {
    Handle nodeHdl;
    Handle remoteAmfHdl;
    try
      {
      nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
      }
    catch (SAFplus::NameException& n)
      {
      logCritical("OPS","ABRT","AMF Entity [%s] is not registered in the name service.  Cannot stop processes on it.", comp->serviceUnit.value->node.value->name.value.c_str());
      comp->lastError.value = "Component's node is not registered in the name service so address cannot be determined.";
      if (&w) w.wake(1,(void*)comp);
      // Node's name is only not there if the node has never existed.
      comp->presenceState = PresenceState::uninstantiated;
      return;
      }

    if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
      {
        if (comp->processId.value)
          {
          logDebug("OPS","ABRT","sending signal [SIGTERM] to process [%d]", comp->processId.value);
          Process p(comp->processId.value);
          p.signal(SIGTERM);
          }
        else
          {
          logWarning("OPS","ABRT","Cannot stop AMF Entity [%s] since it has no associated process id.  If this process still exists, it will become orphaned.", comp->serviceUnit.value->node.value->name.value.c_str());

          }
        if(changePS == true)
        {
            comp->presenceState = PresenceState::uninstantiated;
        }
      }
    else  
      {
        remoteAmfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,nodeHdl.getNode());
        FaultState fs = fault.getFaultState(remoteAmfHdl);
        if (fs != FaultState::STATE_DOWN)
          {
          StopComponentRequest req;
          req.set_pid(comp->processId.value);
          //req.set_name(comp->name);
          amfInternalRpc->stopComponent(remoteAmfHdl,&req, nullptr);
          }
        else
          {
          comp->lastError.value = "Component's node is unavailable.";
          }
        comp->presenceState = PresenceState::uninstantiated;
        if (&w) w.wake(1,(void*)comp);
      }
    }

  bool compIsProxyReady(SAFplusAmf::Component* comp)
  {
    SAFplusAmf::Component* proxy = comp->proxy.value;    
  
    if (proxy && (proxy->readinessState.value == SAFplusAmf::ReadinessState::inService))
    {     
       MgtObject::Iterator itcsi;
       MgtObject::Iterator endcsi = cfg.safplusAmf.componentServiceInstanceList.end();
       for (itcsi = cfg.safplusAmf.componentServiceInstanceList.begin(); itcsi != endcsi; itcsi++)
        {
            ComponentServiceInstance* csi = dynamic_cast<ComponentServiceInstance*>(itcsi->second);
            //std::vector<SAFplus::MgtIdentifierList::Elem>& vec = csi->activeComponents.value;
            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->activeComponents.value;
            std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator itvec = vec.begin();            
            for(; itvec != vec.end(); itvec++)
            {
               SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itvec;
               SAFplusAmf::Component* c = elem.value;
               if (proxy->name.value.compare(c->name.value)==0)
               {
                  logDebug("OPS","PROXY.READY","found csi [%s] has active assignment for component [%s]", csi->name.value.c_str(), proxy->name.value.c_str());
                  return true;
               }
            }
        }
    }
    else
    {
      logInfo("OPS","PROXY.READY","proxied comp [%s] has no proxy or redinessState of [%s] is [%s]", comp->name.value.c_str(), proxy?proxy->name.value.c_str():"Unknown",proxy?SAFplusAmf::c_str(proxy->readinessState.value):"Unknown");
    }

    return false;
}

  void AmfOperations::start(SAFplusAmf::Component* comp,Wakeable& w)
    {
    assert(comp);
    comp->numInstantiationAttempts.value++;
    comp->lastInstantiation.value.value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (!comp->serviceUnit)
      {
      clDbgCodeError(1,("Can't start disconnected comp"));  // Because we don't know what node to start it on...
      comp->lastError.value = "Can't start disconnected component.";
      comp->operState = false;  // A configuration error isn't going to heal itself -- component needs manual intervention then repair
      if (&w) w.wake(1,(void*)comp);
      return;
      }
    if (!comp->serviceUnit.value->node)
      {
      clDbgCodeError(1,("Can't start disconnected comp"));
      comp->lastError.value = "Can't start disconnected component.";
      comp->operState = false;  // A configuration error isn't going to heal itself -- component needs manual intervention then repair
      if (&w) w.wake(1,(void*)comp);
      return;
      }

    try  // Is a component by that name already running?
      {
      Handle nodeHdl = name.getHandle(comp->name);
      // For now, if it is assume that its old data in the name service
      // In the future, wew will check the process's existence and attempt to connect to it.
      name.remove(comp->name);      
      }
    catch (SAFplus::NameException& n)
      {
    // This is the expected condition -- comp is not registered
      }

    Handle nodeHdl;
    try
      {
      nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
      }
    catch (SAFplus::NameException& n)
      {
      logCritical("OPS","SRT","AMF Entity [%s] is not registered in the name service.  Cannot start processes on it.", comp->serviceUnit.value->node.value->name.value.c_str());
      comp->lastError.value = "Component's node is not registered in the name service so address cannot be determined.";
      if (&w) w.wake(1,(void*)comp);
      return;
      }

    SAFplusAmf::Instantiate* inst = comp->getInstantiate();
    //assert(inst);
    if (!inst)  // Bad configuration
      {
      logWarning("OPS","SRT","Component [%s] has improper configuration (no instantiation information). Cannot start", comp->name.value.c_str());
      comp->operState = false;  // A configuration error isn't going to heal itself -- component needs manual intervention then repair
      comp->lastError.value = "No instantiation information";
      if (&w) w.wake(1,(void*)comp);
      return;
      }

    if ( comp->capabilityModel == CapabilityModel::not_preinstantiable)
        {
        comp->presenceState.value  = PresenceState::instantiated;
        // TODO: add the WORK key/value pairs to the environment variables
        }
    else 
        comp->presenceState.value  = PresenceState::instantiating;
    if (comp->compProperty.value == SAFplusAmf::CompProperty::sa_aware)
    {
      if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
      {
        std::vector<std::string> newEnv = comp->commandEnvironment.value;
        std::string strCompName("ASP_COMPNAME=");
        std::string strNodeName("ASP_NODENAME=");
        std::string strNodeAddr("ASP_NODEADDR=");
        std::string strPort("SAFPLUS_RECOMMENDED_MSG_PORT=");

        strCompName.append(comp->name);
        strNodeName.append(SAFplus::ASP_NODENAME);
        strNodeAddr.append(std::to_string(SAFplus::ASP_NODEADDR));
        int port = portAllocator.allocPort();
        strPort.append(std::to_string(port));
        newEnv.push_back(strCompName);
        newEnv.push_back(strNodeName);
        newEnv.push_back(strNodeAddr);
        newEnv.push_back(strPort);

        Process p = executeProgram(inst->command.value, newEnv,Process::InheritEnvironment);
        portAllocator.assignPort(port, p.pid);
        comp->processId = p.pid;

        createSymlink(inst->command.value, p.pid);

        // I need to set the handle because the process itself will not do so.
        if ( comp->capabilityModel == CapabilityModel::not_preinstantiable)
        {
          Handle handle=getProcessHandle(p.pid);
          name.set(comp->name, handle, NameRegistrar::MODE_NO_CHANGE);        
          comp->presenceState.value  = PresenceState::instantiated;    // There are no transitional states in the not_preinstantiable (not SAF aware) case
        }

        logInfo("OPS","SRT","Launching Component [%s] as [%s] locally with process id [%d], recommended port [%d]", comp->name.value.c_str(),inst->command.value.c_str(),p.pid,port);


        if (&w) w.wake(1,(void*)comp);
      }
      else  // RPC call
      {
        Handle remoteAmfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,nodeHdl.getNode());
        FaultState fs = fault.getFaultState(remoteAmfHdl);
        if (fs == FaultState::STATE_UP)
        {
          logInfo("OP","CMP","Request launch component [%s] as [%s] on node [%s]", comp->name.value.c_str(),inst->command.value.c_str(),comp->serviceUnit.value->node.value->name.value.c_str());

          StartComponentRequest req;
          req.set_name(comp->name.value.c_str());
          req.set_command(inst->command.value.c_str());
          StartCompResp* resp = new StartCompResp(&w,comp);
          amfInternalRpc->startComponent(remoteAmfHdl,&req, &resp->response,*resp);
        }
        else
        {
          comp->lastError.value = "Component's node is not up.";
          if (&w) w.wake(1,(void*)comp);
        }
      }
      reportChange();
    }
    else if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable)
    {
      if (!compIsProxyReady(comp))
      {
        logInfo("OPS","START","proxy is not ready, no operation for proxied start right now");
        return;
      }
      SAFplus::Rpc::amfAppRpc::ProxiedComponentInstantiateRequest request;
      Handle proxyHdl, hdl;
      SAFplusAmf::Component* proxy = comp->proxy.value;
      try
      {
         proxyHdl = name.getHandle(proxy->name);
      }
      catch (SAFplus::NameException& n)
      {
        logCritical("OPS","START","Component [%s] is not registered in the name service.  Cannot control it. Exception message [%s]", proxy->name.value.c_str(), n.what());
        comp->lastError.value = "Component's name is not registered in the name service so cannot cleanup proxied component cleanly";
        return;
      }
      try
      {
         hdl = name.getHandle(comp->name);
      }
      catch (SAFplus::NameException& n)
      {
        logInfo("OPS","START","Component [%s] is not registered in the name service. It'll register later. Exception message [%s]", comp->name.value.c_str(), n.what());
        //comp->lastError.value = "Component's name is not registered in the name service so cannot cleanup proxied component cleanly";

        //return;
      }
      request.set_componentname(comp->name.value.c_str());
      request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
      if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
      request.set_invocation(invocation++);
      amfAppRpc->proxiedComponentInstantiate(proxyHdl, &request);
      comp->launched = true;
    }
 }

 void AmfOperations::cleanup(SAFplusAmf::Component* comp,Wakeable& w)
 {
    SAFplusAmf::Cleanup* cleanup = comp->getCleanup();
    //if (!cleanup || cleanup->command.value.length()==0)  // no configuration
    if (!cleanup)  // no configuration
      {
      logInfo("OPS","CLE","Component [%s] has improper configuration (no cleanup information). Cannot clean", comp->name.value.c_str());
      return;
      }
    Handle nodeHdl;
    Handle remoteAmfHdl;
    try
    {
      nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
    }
    catch (SAFplus::NameException& n)
    {
      logCritical("OPS","CLE","AMF Entity [%s] is not registered in the name service.  Cannot cleanup it.", comp->serviceUnit.value->node.value->name.value.c_str());
      comp->lastError.value = "Component's node is not registered in the name service so address cannot be determined.";
      if (&w) w.wake(1,(void*)comp);
      // Node's name is only not there if the node has never existed.
      comp->presenceState = PresenceState::uninstantiated;
      return;
    }
   if (comp->compProperty.value == SAFplusAmf::CompProperty::sa_aware)
   {   
     if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
     {
        std::vector<std::string> newEnv = comp->commandEnvironment.value;
        std::string strCompName("ASP_COMPNAME=");
        strCompName.append(comp->name);
        newEnv.push_back(strCompName);

        if(cleanup->command.value.length()!=0)
        {
          logDebug("OPS","CLE","Cleaning up Component [%s] as [%s] locally", comp->name.value.c_str(),cleanup->command.value.c_str());
          int status = executeProgramWithTimeout(cleanup->command.value, newEnv,cleanup->timeout.value,Process::InheritEnvironment);
        }

        if (comp->processId.value>0)
        {
           logDebug("OPS", "CLE", "Sending SIGKILL signal to component [%s] with process id [%d]", comp->name.value.c_str(), comp->processId.value);
           kill(comp->processId.value, SIGKILL);
        }

        if (&w) w.wake(1,(void*)comp);
     }
     else  // RPC call
     {
       Handle remoteAmfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT,nodeHdl.getNode());
       FaultState fs = fault.getFaultState(remoteAmfHdl);
       if (fs == FaultState::STATE_UP)
       {
          logInfo("OP","CLE","Request cleanup component [%s] as [%s] on node [%s]", comp->name.value.c_str(),cleanup->command.value.c_str(),comp->serviceUnit.value->node.value->name.value.c_str());

          CleanupComponentRequest req;
          req.set_name(comp->name.value.c_str());
          req.set_command(cleanup->command.value.c_str());
          req.set_pid(comp->processId.value);
          req.set_timeout(cleanup->timeout.value);
          CleanupCompResp* resp = new CleanupCompResp(&w,comp);
          amfInternalRpc->cleanupComponent(remoteAmfHdl,&req, &resp->response,*resp);
       }
       else
       {
         comp->lastError.value = "Component's node is not up.";
         if (&w) w.wake(1,(void*)comp);
       }
     }
   }
   else if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable)
    {
      if (!compIsProxyReady(comp))
      {
        logInfo("OPS","CLEANUP","proxy is not ready, no operation for proxied cleanup right now");
        return;
      }
      SAFplus::Rpc::amfAppRpc::ProxiedComponentCleanupRequest request;
      Handle proxyHdl, hdl;
      SAFplusAmf::Component* proxy = comp->proxy.value;
      try
      {
         proxyHdl = name.getHandle(proxy->name);
      }
      catch (SAFplus::NameException& n)
      {
        logCritical("OPS","START","Component [%s] is not registered in the name service.  Cannot control it. Exception message [%s]", proxy->name.value.c_str(), n.what());
        comp->lastError.value = "Component's name is not registered in the name service so cannot cleanup proxied component cleanly";
        return;
      }
      try
      {
         hdl = name.getHandle(comp->name);
      }
      catch (SAFplus::NameException& n)
      {
        logCritical("OPS","START","Component [%s] is not registered in the name service.  Cannot control it. Exception message [%s]", comp->name.value.c_str(), n.what());
        comp->lastError.value = "Component's name is not registered in the name service so cannot cleanup proxied component cleanly";
        return;
      }
      request.set_componentname(comp->name.value.c_str());
      request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
      if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
      request.set_invocation(invocation++);
      amfAppRpc->proxiedComponentCleanup(proxyHdl, &request);
    }
  }

  void AmfOperations::start(SAFplusAmf::ServiceGroup* sg,Wakeable& w)
    {
    clDbgNotImplemented();
    // TODO: Service Group startup needs to be done in a manner consistent with the Redundancy Policy.
#if 0
    SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
    SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator endsu = sg->serviceUnits.value.end();
    for (itsu = sg->serviceUnits.value.begin(); itsu != endsu; itsu++)
      {
      //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
      ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      const std::string& suName = su->name;
      logInfo("N+M","AUDIT","Auditing su %s", suName.c_str());

      SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
      SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
      for (itcomp = su->components.value.begin(); itcomp != endcomp; itcomp++)
        {
        Component* comp = dynamic_cast<Component*>(*itcomp);
        SAFplusAmf::AdministrativeState eas = effectiveAdminState(comp);

        if ((comp->operState == false)&&(eas != SAFplusAmf::AdministrativeState::off))
          {
          logError("N+M","AUDIT","Component %s should be on but is not instantiated", comp->name.c_str());
          }
        if ((comp->operState)&&(eas == SAFplusAmf::AdministrativeState::off))
          {
          logError("N+M","AUDIT","Component %s should be off but is instantiated", comp->name.c_str());
          }
        }
      }
#endif
    }

    void AmfOperations::rebootNode(SAFplusAmf::Node* node, Wakeable& w)
    {
        logDebug("OPS","NODE.REBOOT","Rebooting node [%s]", node->name.value.c_str());
        if(!node)
        {
            return;
        }
        MgtIdentifierList<ServiceUnit*>::iterator itsu;
        for(itsu=node->serviceUnits.listBegin(); itsu != node->serviceUnits.listEnd(); ++itsu)
        {
            //remove each component in su of this node out of name service before restarting this node.
            ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
            MgtIdentifierList<Component*>::iterator itcomp=su->components.listBegin();
            MgtIdentifierList<Component*>::iterator endComp=su->components.listEnd();
            for(;itcomp!=endComp;++itcomp)
            {
                Component *comp=dynamic_cast<Component*>(*itcomp);
                try
                {
                    name.remove(comp->name);
                }
                catch(NameException& n)
                {
                    //comp is not registered
                    logCritical("OPS","SRT","AMF Entity [%s] is not registered in the name service.", comp->name.value.c_str());
                }
            }
        }
        Handle nodeHdl;
        try
        {
            nodeHdl = name.getHandle(node->name);
        }
        catch (SAFplus::NameException& n)
        {
            logCritical("OPS","SRT","AMF Entity [%s] is not registered in the name service.", node->name.value.c_str());
            return;
        }
        if(nodeHdl == nodeHandle)//handle locally
        {
            rebootFlag = true;
            gfault.registerEntity(nodeHandle,FaultState::STATE_DOWN);
        }
        else //remotely reboot node
        {
            Handle remoteAmfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT, nodeHdl.getNode());
            if(FaultState::STATE_UP == fault.getFaultState(remoteAmfHdl))
            {
                logInfo("OPS", "REB", "Reboot Node [%d]",nodeHdl.getNode());
                RebootNodeRequest req;
                RebootNodeResponse resp;
                amfInternalRpc->rebootNode(remoteAmfHdl,&req,&resp);
            }
        }
    }
    
    bool AmfOperations::compUpdateProxiedComponents(SAFplusAmf::Component* proxy, SAFplusAmf::ComponentServiceInstance* csi)
    {
        SAFplusAmf::PresenceState suState, compState;
        bool suUp, compNotUp, suDown, compNotDown;
        MgtObject::Iterator itcomp;
        MgtObject::Iterator endcomp = cfg.safplusAmf.componentList.end();
        for (itcomp = cfg.safplusAmf.componentList.begin(); itcomp != endcomp; itcomp++)
        {
           SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(itcomp->second);
           if (comp->proxyCSI.value.compare(csi->type.value)==0)
           {
              if (csi->activeComponents.value.size()>0)
              {
                 logInfo("OPS","UPD.PROXIED","registering proxy [%s] for component [%s]", proxy->name.value.c_str(),comp->name.value.c_str());
                 comp->proxy = proxy;
                 proxy->proxied.value.push_back(comp); // add the proxied component to the list of proxied components of its proxy
                 suState = comp->serviceUnit.value->presenceState.value;
                 compState = comp->presenceState.value;
                 logInfo("OPS","UPD.PROXIED","su [%s] presenceState [%s]; component [%s] presenceState [%s]", comp->serviceUnit.value->name.value.c_str(), SAFplusAmf::c_str(suState),comp->name.value.c_str(),SAFplusAmf::c_str(compState));
                 compNotUp = (compState == SAFplusAmf::PresenceState::uninstantiated) ||
                    (compState == SAFplusAmf::PresenceState::instantiating)  ||
                    (compState == SAFplusAmf::PresenceState::restarting);
                 compNotDown = (compState == SAFplusAmf::PresenceState::instantiated)   ||
                    (compState == SAFplusAmf::PresenceState::terminating)    ||
                    (compState == SAFplusAmf::PresenceState::restarting);
                 suUp      = (suState   == SAFplusAmf::PresenceState::instantiating)  ||
                    (suState   == SAFplusAmf::PresenceState::restarting)     ||
                    (suState   == SAFplusAmf::PresenceState::instantiated);
                 suDown      = (suState   == SAFplusAmf::PresenceState::uninstantiated) ||
                    (suState   == SAFplusAmf::PresenceState::terminating);
                 if ( (suUp | suDown) && compNotUp ) // starting a comp in s7 doesn't require its su up
                    {
                        logInfo("OPS","UPD.PROXIED","starting proxied component [%s] in presenceState [%s]", comp->name.value.c_str(),SAFplusAmf::c_str(compState));
                        start(comp);
                    }
                    
                 if ( suDown && compNotDown )
                    {
                        logInfo("OPS","UPD.PROXIED","stopping proxied component [%s] in presenceState [%s]", comp->name.value.c_str(),SAFplusAmf::c_str(compState));
                        stop(comp);
                    }
              }   
           }           
        }
    }

    void AmfOperations::assignWorkCallback(SAFplusAmf::Component* comp)
    {
        MgtObject::Iterator itcsi;
        MgtObject::Iterator endcsi = cfg.safplusAmf.componentServiceInstanceList.end();
        for (itcsi = cfg.safplusAmf.componentServiceInstanceList.begin(); itcsi != endcsi; itcsi++)
        {
            ComponentServiceInstance* csi = dynamic_cast<ComponentServiceInstance*>(itcsi->second);
            if (csi->isProxyCSI.value)
            {
                compUpdateProxiedComponents(comp,csi);
            }
        }
    }

    void AmfOperations::nodeRestart(SAFplusAmf::Node* node,Wakeable& w)
    {
        logDebug("OPS", "RESTART", "enter [nodeRestart] with param node name [%s]", node->name.value.c_str());
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = node->serviceUnits.listEnd();
        for (itsu = node->serviceUnits.listBegin(); itsu != endsu; itsu++)
        {
            ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
            serviceUnitRestart(su, w);
        }
    }

    void AmfOperations::serviceUnitRestart(SAFplusAmf::ServiceUnit* su,Wakeable& w)
    {
        logDebug("OPS", "RESTART", "enter [serviceUnitRestart] with param serviceUnit name [%s]", su->name.value.c_str());
        SAFplus::MgtObject::Iterator itcomp;
        SAFplus::MgtObject::Iterator endcomp = su->components.end();
        for (itcomp = su->components.begin(); itcomp != endcomp; itcomp++)
        {
            Component* comp = dynamic_cast<Component*>(itcomp->second);
            abort(comp, false, w);
        }
    }

    void AmfOperations::componentRestart(SAFplusAmf::Component* comp,Wakeable& w)
    {
        logDebug("OPS", "RESTART", "enter [componentRestart] with param component name [%s]", comp->name.value.c_str());
        abort(comp, false, w);
    }

    ClRcT AmfOperations::swapSI(std::string siName)
    {
        ClRcT rc = CL_OK;
        SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
        if (si == NULL)
        {
            logDebug("OPS","SWSI","si object is null for its name [%s]", siName.c_str());
            return CL_ERR_NOT_EXIST;
        }

        ServiceGroup* sg = si->serviceGroup.value;
        if(sg->autoAdjust == true)
        {
            logDebug("OPS", "SWSI", "Cannot swap SIs for SG [%s], which has auto adjust option enabled.", sg->name.value.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        if(si->adminState.value != SAFplusAmf::AdministrativeState::on)
        {
            logDebug("OPS", "SWSI", "serviceInstance [%s] admin state is not on", siName.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        if(si->getNumStandbyAssignments()->current.value == 0 || si->preferredStandbyAssignments.value == 0)
        {
            logDebug("OPS", "SWSI", "serviceInstance [%s] doesnt have standby assignments", siName.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        std::vector<ServiceUnit*> activeSU;
        std::vector<ServiceUnit*> standbySU;
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator beginSU;
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endSU = sg->serviceUnits.listEnd();
        for (beginSU = sg->serviceUnits.listBegin(); beginSU != endSU; beginSU++)
        {
            ServiceUnit* su = dynamic_cast<ServiceUnit*>(*beginSU);
            if(su->haState == HighAvailabilityState::active)
            {
                if(activeSU.size() != 0)
                {
                    continue;
                }
                activeSU.push_back(su);
            }
            else if(su->haState == HighAvailabilityState::standby)
            {
                if(standbySU.size() != 0)
                {
                    continue;
                }
                standbySU.push_back(su);
            }
        }

        if(activeSU.size() == 0 || standbySU.size() == 0)
        {
            logDebug("OPS", "SWSI", "SI [%s] doesn't have active or standby SUs", siName.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        if((activeSU.at(0))->presenceState.value == SAFplusAmf::PresenceState::instantiating ||
                (activeSU.at(0))->presenceState.value == SAFplusAmf::PresenceState::terminating ||
                (activeSU.at(0))->presenceState.value == SAFplusAmf::PresenceState::restarting)
        {
            logDebug("OPS", "SWSI", "Active SU [%s] has state [%s]. Returning try again for SI swap", (activeSU.at(0))->name.value.c_str(), c_str((activeSU.at(0))->presenceState.value));
            return CL_ERR_TRY_AGAIN;
        }

        if((standbySU.at(0))->presenceState.value == SAFplusAmf::PresenceState::instantiating ||
                (standbySU.at(0))->presenceState.value == SAFplusAmf::PresenceState::terminating ||
                (standbySU.at(0))->presenceState.value == SAFplusAmf::PresenceState::restarting)
        {
            logDebug("OPS", "SWSI", "Standby SU [%s] has state [%s]. Returning try again for SI swap", (standbySU.at(0))->name.value.c_str(), c_str((standbySU.at(0))->presenceState.value));
            return CL_ERR_TRY_AGAIN;
        }

        // actually swap si

        std::vector<ServiceUnit*>::iterator iterActiveSU;
        for(iterActiveSU = activeSU.begin(); iterActiveSU != activeSU.end(); iterActiveSU++)
        {
            ServiceUnit* su = *iterActiveSU;
            rc = removeThenAssignWork(si, su, su->haState, SAFplusAmf::HighAvailabilityState::standby);
        }

        std::vector<ServiceUnit*>::iterator iterStandbySU;
        for(iterStandbySU = standbySU.begin(); iterStandbySU != standbySU.end(); iterStandbySU++)
        {
            ServiceUnit* su = *iterStandbySU;
            rc = removeThenAssignWork(si, su, su->haState, SAFplusAmf::HighAvailabilityState::active);
        }

        return rc;
    }


    bool isThereStandbySU(const std::vector<ServiceUnit*>& suList)
      {
          std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
          for (; itsu != suList.end(); ++itsu)
          {
             SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
             if (su->numStandbyServiceInstances.current.value > 0)
                 return true;
          }
          return false;
      }

      uint32_t getActiveHighestRankSU(const std::vector<ServiceUnit*>& suList)
        {
            std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
            for (; itsu != suList.end(); ++itsu)
            {
               SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
               if (su->numActiveServiceInstances.current.value > 0)
                   return su->rank.value;
            }
            return 0;
        }

      uint32_t getStandbyHighestRankSU(const std::vector<ServiceUnit*>& suList)
          {
              std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
              for (; itsu != suList.end(); ++itsu)
              {
                 SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                 if (su->numStandbyServiceInstances.current.value > 0)
                     return su->rank.value;
              }
              return 0;
          }

      int getNumAssignedSUs(const std::vector<ServiceUnit*>& suList)
            {
                std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
                int count = 0;
                for (; itsu != suList.end(); ++itsu)
                {
                   SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                   if (su->numStandbyServiceInstances.current.value > 0 || su->numActiveServiceInstances.current.value > 0)
                       count++;
                }
                return count;
            }

    ClRcT AmfOperations::sgAdjust(const SAFplusAmf::ServiceGroup* sg)
      {
          logDebug("N+M","ADJUST","adjusting sg [%s]", sg->name.value.c_str());
          ClRcT rc = CL_OK;
          bool (*suOrder)(ServiceUnit* a, ServiceUnit* b) = suEarliestRanking;
          std::vector<SAFplusAmf::ServiceUnit*> sus;
          sus << sg->serviceUnits;
          if (!isThereStandbySU(sus) || getNumAssignedSUs(sus) == 1) // there are only active assignments or there is only one su in the list, so nothing to do
          {
             logDebug("N+M","ADJUST","sg [%s] doesn't need to be adjusted because there is no any standby SU or there is only one assigned SU", sg->name.value.c_str());
             rc = CL_ERR_NO_OP;
             return rc;
          }
          boost::sort(sus, suOrder);
          uint32_t activeHighestRankSU = getActiveHighestRankSU(sus);
          uint32_t standbyHighestRankSU = getStandbyHighestRankSU(sus);
          if ((activeHighestRankSU && activeHighestRankSU > standbyHighestRankSU) || (activeHighestRankSU == 0 && standbyHighestRankSU > 0))
          {
              logDebug("N+M","ADJUST","SU list of sg [%s] need to be adjusted", sg->name.value.c_str());
              std::vector<SAFplusAmf::ServiceUnit*>::iterator itsu = sus.begin();
              for (; itsu != sus.end(); ++itsu)
              {
                 SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                 // go one by one su to see if there is any su having hastate standby and having lower rank than active higher rank su,
                 // then remove their work by setting their admin state to idle
                 if (su->adminState.value == AdministrativeState::on)
                 {
                    su->adminState.value = AdministrativeState::idle;
                 }
              }
              boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
              for (itsu = sus.begin(); itsu != sus.end(); ++itsu)
              {
                 SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                 // then add their work by setting their admin state to on
                 if (su->adminState.value == AdministrativeState::idle)
                 {
                    su->adminState.value = AdministrativeState::on;
                 }
              }
          }
          else
          {
              logDebug("N+M","ADJUST","sg [%s] doesn't need to be adjusted because its SUs with ranks have appropriate hastate", sg->name.value.c_str());
          }
          return rc;
      }

    ClRcT AmfOperations::nodeErrorReport(SAFplusAmf::Node* node, bool gracefulSwitchover, bool shutdownAmf, bool restartAmf, bool rebootNode)
    {
        logDebug("OPS","NODE.ERR","Error reported for node [%s]", node->name.value.c_str());
        if (node->operState.value == false)
        {
            return CL_ERR_NO_OP;
        }
        CL_AMF_SET_O_STATE(node,false);
        //comp->haReadinessState == HighAvailabilityReadinessState::readyForAssignment
        //Terminate all components running in this node        
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
#define MAX_TRY 30
        int loopCount = 0;
        if (gracefulSwitchover)
        {
            if(shutdownAmf) pendingWorkOperations.clear();
            //nodeGracefulSwitchover = gracefulSwitchover;
            SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
            logDebug("OPS","NODE.ERR","Removing works for all components running in node [%s]", node->name.value.c_str());
            for (itsu = node->serviceUnits.listBegin(); itsu != node->serviceUnits.listEnd(); itsu++)
            {
                ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                //assert(su);
                if (!su) continue;
                // remove work first
                logDebug("OPS","NODE.ERR","Removing work assignments for all components running in su [%s]", su->name.value.c_str());
                if ((su->adminState.value != AdministrativeState::off) && (su->numActiveServiceInstances.current > 0 || su->numStandbyServiceInstances.current > 0))
                {
                    removeWork(su);
                }
            }
            logDebug("OPS","NODE.ERR","Terminating all components running in node [%s]", node->name.value.c_str());
            recommendedRecovery = SAFplusAmf::Recovery::None;
            for (itsu = node->serviceUnits.listBegin(); itsu != node->serviceUnits.listEnd(); itsu++)
            {
                ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                //assert(su);
                if (!su) continue;
                SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                for (itcomp = su->components.listBegin(); itcomp != su->components.listEnd(); itcomp++)
                {
                    Component* comp = dynamic_cast<Component*>(*itcomp);
                    //assert(comp);
                    if (!comp) continue;
                    //while (comp->haState.value != SAFplusAmf::HighAvailabilityState::idle && loopCount < MAX_TRY)
                    while ((comp->haState.value != SAFplusAmf::HighAvailabilityState::idle || comp->pendingOperation != PendingOperation::none) && loopCount < MAX_TRY)
                    {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                        loopCount++;
                    }
                    
                }
                
            }
            if (loopCount == MAX_TRY)
            {
                logWarning("OPS","NODE.ERR","one of components in node [%s] didn't complete csi remove", node->name.value.c_str());
                //nodeGracefulSwitchover = false;
                return CL_ERR_TRY_AGAIN;
            }
            for (itsu = node->serviceUnits.listBegin(); itsu != node->serviceUnits.listEnd(); itsu++)
            {  
                SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                for (itcomp = su->components.listBegin(); itcomp != su->components.listEnd(); itcomp++)
                {
                    Component* comp = dynamic_cast<Component*>(*itcomp);
                    //assert(comp);
                    if (!comp) continue;
                    logDebug("OPS","NODE.ERR","Terminating component [%s]", comp->name.value.c_str());
                    if (effectiveAdminState(comp) != AdministrativeState::off)
                    {
                       stop(comp);
                    }
                }
            }
            loopCount=0;
            for (itsu = node->serviceUnits.listBegin(); itsu != node->serviceUnits.listEnd(); itsu++)
            {
                ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                //assert(su);
                if (!su) continue;
                SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                for (itcomp = su->components.listBegin(); itcomp != su->components.listEnd(); itcomp++)
                {
                    Component* comp = dynamic_cast<Component*>(*itcomp);
                    //assert(comp);
                    if (!comp) continue;
                    while (comp->presenceState.value != SAFplusAmf::PresenceState::uninstantiated && loopCount < MAX_TRY)
                    {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                        loopCount++;
                    }
                    if (loopCount == MAX_TRY)
                    {
                        logWarning("OPS","NODE.ERR","Component [%s] doesn't complete its termination process in [%d] ms, so handling of node error report is not completed", comp->name.value.c_str(), MAX_TRY*100);
                        //nodeGracefulSwitchover = false;
                        return CL_ERR_UNSPECIFIED;
                    }
                    //updateStateDueToProcessDeath(comp); //was called in workOperationResponse
                }
            }
        }
        if (shutdownAmf)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
            Handle nodeHdl;
            try
            {
                nodeHdl = name.getHandle(node->name.value);
            }
            catch (NameException& ne)
            {
                logError("OPS", "NODE.REPORT", "Handle got exception for name [%s]", node->name.value.c_str());
                return CL_ERR_NOT_EXIST;
            }            
            if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
            {
                if (!restartAmf)
                {
                    char asp_restart_disable_file[CL_MAX_NAME_LENGTH];
                    snprintf(asp_restart_disable_file, CL_MAX_NAME_LENGTH-1, "%s/%s", (SAFplus::ASP_RUNDIR[0] != 0) ? SAFplus::ASP_RUNDIR : ".", ASP_RESTART_DISABLE_FILE);
                    FILE* fp = fopen(asp_restart_disable_file, "w");
                    if (fp)
                    {
                        fclose(fp);
                        logDebug("OPS","NODE.ERR","Shutdown amf by setting node [%s] fault state DOWN", node->name.value.c_str());
                        gfault.registerEntity(nodeHdl,FaultState::STATE_DOWN);
                    }
                    else
                    {
                       logError("OPS","SHUTDOWN.AMF","Opening file [%s] fail. Error code [%d], error text [%s]", asp_restart_disable_file, errno, strerror(errno));
                    }
                }
                else
                {
                    gfault.registerEntity(nodeHdl,FaultState::STATE_DOWN);
                }
            }            
            else
            {
                Handle remoteAmfHdl = getProcessHandle(SAFplusI::AMF_IOC_PORT, nodeHdl.getNode());
                if (FaultState::STATE_UP == fault.getFaultState(remoteAmfHdl))
                {
                    logInfo("OPS", "NODE.ERR", "Sending shutdown amf request to node [%d]",nodeHdl.getNode());
                    ShutdownAmfRequest req;
                    req.set_restartamf(restartAmf);
                    req.set_rebootnode(rebootNode);
                    ShutdownAmfResponse resp;
                    amfInternalRpc->shutdownAmf(remoteAmfHdl,&req,&resp);
                }
                else
                {
                    logError("OPS","NODE.ERR","Fault state of node [%s] is DOWN, cannot send shutdownAmf request to it", node->name.value.c_str());
                }
            }
        }
        if (rebootNode)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            this->rebootNode(node);
        }
        //nodeGracefulSwitchover = false;
#if 0
        loopCount = 0;
        while (node->presenceState.value != PresenceState::uninstantiated && loopCount < MAX_TRY)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            loopCount++;
        }
        nodeGracefulSwitchover = false;
        if (loopCount == MAX_TRY)
        {
            logWarning("OPS","NODE.ERR","Node [%s] (presenceState [%s]) doesn't complete the switchover in [%d] ms, so handling of node error report is not completed", node->name.value.c_str(), SAFplusAmf::c_str(node->presenceState.value), MAX_TRY*100);        
            return CL_ERR_UNSPECIFIED;
        }
#endif
        return CL_OK;
    }

    ClRcT AmfOperations::nodeErrorClear(SAFplusAmf::Node* node)
    {
        logDebug("OPS","NODE.ERR","Error clear for node [%s]", node->name.value.c_str());
        if (node->operState.value == true)
        {
            return CL_ERR_NO_OP;
        }
        CL_AMF_SET_O_STATE(node,true);
        return CL_OK;
    }

    ClRcT AmfOperations::assignSUtoSI(std::string siName, std::string activceSUName, std::string standbySUName)
    {
        logDebug("OPS","ASUI","assignSUtoSI: siName: %s, activeName: %s, standbyName: %s", siName.c_str(), activceSUName.c_str(), standbySUName.c_str());
        ClRcT rc = CL_OK;
        SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
        if (si == NULL)
        {
            logDebug("OPS","ASUI","si object is null for its name [%s]", siName.c_str());
            return CL_ERR_NOT_EXIST;
        }

        ServiceGroup* sg = si->serviceGroup.value;
        if(sg->autoAdjust == true)
        {
            logDebug("OPS", "ASUI", "Cannot swap SIs for SG [%s], which has auto adjust option enabled.", sg->name.value.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        if(si->adminState.value != SAFplusAmf::AdministrativeState::on)
        {
            logDebug("OPS", "ASUI", "serviceInstance [%s] admin state is not on", siName.c_str());
            return CL_ERR_BAD_OPERATION;
        }

        SAFplusAmf::ServiceUnit* activeSU = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[activceSUName]);
        SAFplusAmf::ServiceUnit* standbySU = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[standbySUName]);
        if (activeSU != NULL)
        {
            rc = removeThenAssignWork(si, activeSU, activeSU->haState, SAFplusAmf::HighAvailabilityState::active);
        }
        else
        {
            logDebug("---","---","activeSU wasn't existing");
        }
        if (standbySU != NULL)
        {
            rc = removeThenAssignWork(si, standbySU, standbySU->haState, SAFplusAmf::HighAvailabilityState::standby);
        }
        else
        {
            logDebug("---","---","standbySU wasn't existing");
        }

        return rc;
    }

    ClRcT AmfOperations::removeThenAssignWork(SAFplusAmf::ServiceInstance* si, SAFplusAmf::ServiceUnit* su, SAFplusAmf::HighAvailabilityState currentState, SAFplusAmf::HighAvailabilityState assignState)
    {
        ClRcT rc = CL_OK;
        if(currentState != assignState)
        {
            if(currentState == SAFplusAmf::HighAvailabilityState::standby)
            {
                if (si->standbyAssignments.find(su) != si->standbyAssignments.value.end())
                {
                    assert(si->numStandbyAssignments.current > 0);
                    si->numStandbyAssignments.current -= 1;
                    si->standbyAssignments.erase(su);

                    std::vector<Component *> comps;
                    SAFplus::MgtObject::Iterator beginCsi = si->componentServiceInstances.begin();
                    SAFplus::MgtObject::Iterator endCsi = si->componentServiceInstances.end();
                    for (auto itCsi{beginCsi}; itCsi != endCsi; itCsi++)
                    {
                        SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itCsi->second);

                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->standbyComponents.value;
                        std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator beginVec = vec.begin();
                        std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator endVec = vec.end();
                        for (auto itVec{beginVec}; itVec != endVec; itVec++)
                        {
                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itVec;
                            SAFplusAmf::Component* c = elem.value;

                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator beginComp = su->components.listBegin();
                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endComp = su->components.listEnd();
                            for (auto itComp{beginComp}; itComp != endComp; itComp++)
                            {
                                Component* comp = dynamic_cast<Component*>(*itComp);
                                if(comp->name.value == c->name.value)
                                {
                                    // same work removal in workOperationResponse function
                                    comp->haState = SAFplusAmf::HighAvailabilityState::idle;
                                    if(comp->activeAssignments > 0)
                                    {
                                        --comp->activeAssignments;
                                    }
                                    if(comp->standbyAssignments > 0)
                                    {
                                        --comp->standbyAssignments;
                                    }

                                    comps.push_back(comp);
                                }
                            }
                        }
                        for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                        {
                            csi->standbyComponents.erase(*iter);
                        }
                    }
                    si->isFullStandbyAssignment= false;
                }
                su->assignedServiceInstances.clear();
                su->numActiveServiceInstances = 0;
                su->numStandbyServiceInstances = 0;
                if(!si->activeAssignments.contains(su))
                {
                    si->activeAssignments.value.push_back(su);
                    si->getNumActiveAssignments()->current.value = si->activeAssignments.value.size();
                }
                assignWork(su, si, assignState);
            }
            else if(currentState == SAFplusAmf::HighAvailabilityState::active)
            {
                if (si->activeAssignments.find(su) != si->activeAssignments.value.end())
                {
                    si->numActiveAssignments.current -= 1;
                    si->activeAssignments.erase(su);

                    std::vector<Component *> comps;
                    SAFplus::MgtObject::Iterator beginCsi = si->componentServiceInstances.begin();
                    SAFplus::MgtObject::Iterator endCsi = si->componentServiceInstances.end();
                    for (auto itCsi{beginCsi}; itCsi != endCsi; itCsi++)
                    {
                        SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itCsi->second);

                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->activeComponents.value;
                        std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator beginVec = vec.begin();
                        std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator endVec = vec.end();
                        for (auto itVec{beginVec}; itVec != endVec; itVec++)
                        {
                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itVec;
                            SAFplusAmf::Component* c = elem.value;

                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator beginComp = su->components.listBegin();
                            SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endComp = su->components.listEnd();
                            for (auto itComp{beginComp}; itComp != endComp; itComp++)
                            {
                                Component* comp = dynamic_cast<Component*>(*itComp);
                                if(comp->name.value == c->name.value)
                                {
                                    // same work removal in workOperationResponse function
                                    comp->haState = SAFplusAmf::HighAvailabilityState::idle;
                                    if(comp->activeAssignments > 0)
                                    {
                                        --comp->activeAssignments;
                                    }
                                    if(comp->standbyAssignments > 0)
                                    {
                                        --comp->standbyAssignments;
                                    }

                                    comps.push_back(comp);
                                }
                            }
                        }
                        for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                        {
                            csi->activeComponents.erase(*iter);
                        }
                    }
                    si->isFullActiveAssignment= false;
                }
                su->assignedServiceInstances.clear();
                su->numActiveServiceInstances = 0;
                su->numStandbyServiceInstances = 0;

                // assign standby
                if(!si->standbyAssignments.contains(su))
                {
                    si->standbyAssignments.value.push_back(su);
                    si->getNumStandbyAssignments()->current.value = si->standbyAssignments.value.size();
                }
                assignWork(su, si, assignState);
            }
            else
            {
                logDebug("OPS","RMTAS","invalid currentState");
            }
        }
        else
        {
            logDebug("OPS","RMTAS","currentState == assignState");
        }

        return rc;

    }

    ClRcT AmfOperations::removeWorkWithoutAppRemove(SAFplusAmf::ServiceUnit* su)
    {
        ClRcT rc = CL_OK;
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator beginSI = su->assignedServiceInstances.listBegin();
        SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator endSI = su->assignedServiceInstances.listEnd();
        for (auto itSI{beginSI}; itSI != endSI; itSI++)
        {
            SAFplusAmf::ServiceInstance* si = dynamic_cast<ServiceInstance*>(*itSI);

            if (si->standbyAssignments.find(su) != si->standbyAssignments.value.end())
            {
                assert(si->numStandbyAssignments.current > 0);
                si->numStandbyAssignments.current -= 1;
                si->standbyAssignments.erase(su);

                std::vector<Component *> comps;
                SAFplus::MgtObject::Iterator beginCsi = si->componentServiceInstances.begin();
                SAFplus::MgtObject::Iterator endCsi = si->componentServiceInstances.end();
                for (auto itCsi{beginCsi}; itCsi != endCsi; itCsi++)
                {
                    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itCsi->second);

                    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->standbyComponents.value;
                    std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator beginVec = vec.begin();
                    std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator endVec = vec.end();
                    for (auto itVec{beginVec}; itVec != endVec; itVec++)
                    {
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itVec;
                        SAFplusAmf::Component* c = elem.value;

                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator beginComp = su->components.listBegin();
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endComp = su->components.listEnd();
                        for (auto itComp{beginComp}; itComp != endComp; itComp++)
                        {
                            Component* comp = dynamic_cast<Component*>(*itComp);
                            if(comp->name.value == c->name.value)
                            {
                                // same work removal in workOperationResponse function
                                comp->haState = SAFplusAmf::HighAvailabilityState::idle;
                                if(comp->activeAssignments > 0)
                                {
                                    --comp->activeAssignments;
                                }
                                if(comp->standbyAssignments > 0)
                                {
                                    --comp->standbyAssignments;
                                }

                                comps.push_back(comp);
                            }
                        }
                    }
                    for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                    {
                        csi->standbyComponents.erase(*iter);
                    }
                }
                si->isFullStandbyAssignment= false;
            }
            if (si->activeAssignments.find(su) != si->activeAssignments.value.end())
            {
                si->numActiveAssignments.current -= 1;
                si->activeAssignments.erase(su);

                std::vector<Component *> comps;
                SAFplus::MgtObject::Iterator beginCsi = si->componentServiceInstances.begin();
                SAFplus::MgtObject::Iterator endCsi = si->componentServiceInstances.end();
                for (auto itCsi{beginCsi}; itCsi != endCsi; itCsi++)
                {
                    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itCsi->second);

                    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = csi->activeComponents.value;
                    std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator beginVec = vec.begin();
                    std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator endVec = vec.end();
                    for (auto itVec{beginVec}; itVec != endVec; itVec++)
                    {
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itVec;
                        SAFplusAmf::Component* c = elem.value;

                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator beginComp = su->components.listBegin();
                        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endComp = su->components.listEnd();
                        for (auto itComp{beginComp}; itComp != endComp; itComp++)
                        {
                            Component* comp = dynamic_cast<Component*>(*itComp);
                            if(comp->name.value == c->name.value)
                            {
                                // same work removal in workOperationResponse function
                                comp->haState = SAFplusAmf::HighAvailabilityState::idle;
                                if(comp->activeAssignments > 0)
                                {
                                    --comp->activeAssignments;
                                }
                                if(comp->standbyAssignments > 0)
                                {
                                    --comp->standbyAssignments;
                                }

                                comps.push_back(comp);
                            }
                        }
                    }
                    for(auto iter{comps.begin()}; iter != comps.end(); ++iter)
                    {
                        csi->activeComponents.erase(*iter);
                    }
                }
                si->isFullActiveAssignment= false;
            }

            su->assignedServiceInstances.clear();
            su->numActiveServiceInstances = 0;
            su->numStandbyServiceInstances = 0;
        }

        return rc;

    }

    void createSymlink(const std::string& command, int pid) {
      char temp[CL_MAX_NAME_LENGTH];
      char fname[CL_MAX_NAME_LENGTH];
      strcpy(temp, command.c_str());
      char* cmd = strtok(temp, " ");
      snprintf(fname, CL_MAX_NAME_LENGTH, "asp.%d", pid);
      int result;
      if (!strncmp(cmd, "/", 1)) {
        result = symlink(cmd, fname);
      }
      else {
        char binDir[CL_MAX_NAME_LENGTH];
        getcwd(binDir, CL_MAX_NAME_LENGTH);
        strcat(binDir, "/");
        strcat(binDir, cmd);
        result = symlink(binDir, fname);
      }
      if (result) {
        logDebug("BIN", "SYMLK", "Can't create symlink with file name[%s] & command[%s]", fname, cmd);
      }
    }

  };
