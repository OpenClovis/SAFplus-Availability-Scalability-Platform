#include <chrono>
#include <google/protobuf/stubs/common.h>
#include <clCommon.hxx>
#include <amfRpc.pb.hxx>
#include <amfRpc.hxx>
#include <amfAppRpc.hxx>
#include <clPortAllocator.hxx>
#include <clRpcChannel.hxx>

#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clOsalApi.hxx>
#include <clAmfPolicyPlugin.hxx>
#include <clAmfIpi.hxx>
#include <clAmfNotification.hxx>
#include <SAFplusAmf/Component.hxx>
#include <SAFplusAmf/ComponentServiceInstance.hxx>
#include <SAFplusAmf/ServiceUnit.hxx>
#include <SAFplusAmf/ServiceGroup.hxx>
#include <SAFplusAmf/Data.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

extern Handle           nodeHandle; //? The handle associated with this node
namespace SAFplus
  {
EventClient evtClient;
    WorkOperationTracker::WorkOperationTracker(SAFplusAmf::Component* c,SAFplusAmf::ComponentServiceInstance* cwork,SAFplusAmf::ServiceInstance* work,uint statep, uint targetp)
    {
    comp = c; csi = cwork; si=work; state = statep; target = targetp;
    issueTime = nowMs();
    }


  // Move this service group and all contained elements to the specified state.
  void setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt)
    {
      //SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
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
        logInfo("N+M","AUDIT","Setting service unit [%s] to admin state [%s]",suName.c_str(),c_str(tgt));
        su->adminState.value = tgt;
        // TODO: transactional and event
        }
      }
      if (sg->adminState.value != tgt)
        {
        logInfo("N+M","AUDIT","Setting service group [%s] to admin state [%s]",sg->name.value.c_str(),c_str(tgt));
        sg->adminState.value = tgt;
        }
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

    bool ClAmfPolicyPlugin_1::initialize(SAFplus::AmfOperations* amfOperations,SAFplus::Fault* faultp)
    {
    amfOps = amfOperations;
    fault  = faultp;
    return true;
    }

  void AmfOperations::workOperationResponse(uint64_t invocation, uint32_t result)
    {
    if (1)
      {
      WorkOperationTracker& wat = pendingWorkOperations.at(invocation);
      logInfo("AMF","OPS","Work Operation response on component [%s] invocation [%" PRIu64 "] result [%d]",wat.comp->name.value.c_str(),invocation, result);
      if ( wat.state == WorkOperationTracker::TerminationState)
        {
          if (result == SA_AIS_OK)
          {
            wat.comp->processId = 0;
            // Nothing to do, we'll update the AMF state when we see the process actually down
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
	if(!(wat.comp->haState == (SAFplusAmf::HighAvailabilityState) wat.state))
	{
	std::string pEventData = createEventNotification(wat.comp->name.value, "highAvaiabilityState", c_str(wat.comp->haState.value), c_str((SAFplusAmf::HighAvailabilityState) wat.state));
	//logInfo("AMF", "OPS", "pEventData : %s", pEventData.c_str());
	evtClient.eventPublish(pEventData,pEventData.length(), CL_CPM_EVENT_CHANNEL_NAME, EventChannelScope::EVENT_GLOBAL_CHANNEL);
        wat.comp->haState = (SAFplusAmf::HighAvailabilityState) wat.state; // TODO: won't work with multiple assignments of active and standby, for example
	}

           //proxy-proxied support feature
          if((wat.comp->compCategory & SA_AMF_COMP_PROXIED)){} //wat.comp->processId = proxied_pid;
          logInfo("MAIN","INIT","AmfOperations::workOperationResponse wat.csi [%s] wat.csi->isProxyCSI.value [%d] wat.comp[%s] pid[%d] ",wat.csi->name.value.c_str(),wat.csi->isProxyCSI.value,wat.comp->name.value.c_str(),wat.comp->processId.value);
          if(wat.csi->isProxyCSI.value)
          {
			  //assert(SAFplusI::amfSession);
			  ServiceUnit* su = wat.comp->getServiceUnit();
			  SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
			  SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
			  for(itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
			  {
				  Component* comp= dynamic_cast<SAFplusAmf::Component*> (*itcomp);
				  if(!(comp->proxyCSIType.value.compare(wat.csi->type.value)))
				  {
					  if(wat.state == (int)HighAvailabilityState::active)
					  {
						  PresenceState suPsState, compPsState;
						  bool compUp, compNotUp, compNotDown, suUp, suDown;
						  //TODO: register wat.comp for proxiedComp
						  wat.comp->proxied.pushBackValue(comp->name.value);
						  comp->proxy = wat.comp->name.value;
						  compPsState = comp->presenceState.value;
						  suPsState = su->presenceState.value;
						  
						  logInfo("WORK", "RESPONSE", "comp [%s] : PresencState[%s] comp->proxy [%s]",comp->name.value.c_str(), c_str(compPsState),comp->proxy.value.c_str());
						  logInfo("WORK", "RESPONSE", "SU [%s] : PresencState[%s]", su->name.value.c_str(), c_str(suPsState));
						  
						  compUp = (compPsState == PresenceState::instantiated);
						  compNotUp = (compPsState == PresenceState::instantiating) || (compPsState == PresenceState::uninstantiated) ||(compPsState == PresenceState::restarting);
						  compNotDown = (compPsState == PresenceState::instantiated) || (compPsState == PresenceState::restarting) || (compPsState == PresenceState::terminating);
						  suUp = (suPsState == PresenceState::instantiated) || (suPsState == PresenceState::instantiating) || (suPsState == PresenceState::restarting);
						  suDown = (suPsState == PresenceState::uninstantiated) || (suPsState == PresenceState::terminating);
						  //instantiate proxy comp
						  logInfo("WORK", "RESPONSE", "workOperationResponse compUp[%d], compNotUp[%d], compNotDown[%d], suUp[%d], suDown[%d]", compUp, compNotUp, compNotDown, suUp, suDown);
						  if((comp->compCategory.value & SA_AMF_COMP_PROXIED))
						  {
							  if(suUp && compNotUp)
							  {
								   std::vector<std::string> newEnv = comp->commandEnvironment.value;
								 
								  Handle hdl;
								  if (comp->capabilityModel != CapabilityModel::not_preinstantiable)  // If the component is preinstantiable, it better be instantiated by now (work would not be assigned if it wasn't)
									{
									  try
										{
										  hdl = name.getHandle(comp->proxy);
										}
									  catch (SAFplus::NameException& n)
										{
										  logCritical("OPS","SRT","Component [%s] is not registered in the name service.  Cannot control it.", comp->name.value.c_str());
										  comp->lastError.value = "Component's name is not registered in the name service so address cannot be determined.";
										  assert(0);
										  break; // TODO: right now go to the next component, however assignment should not occur on ANY components if all components are not accessible. 
										}
									}
								  SAFplus::Rpc::amfAppRpc::WorkOperationRequest request;
								  request.set_componentname(comp->name.value.c_str());
								  
								  request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
								  request.set_operation((uint32_t)wat.state);
								  request.set_target(SA_AMF_PROXIED_INST_CB);
								  if ((this->invocation & 0xFFFFFFFF) == 0xFFFFFFFF) this->invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
								  request.set_invocation(this->invocation++);
								  SAFplusAmf::ComponentServiceInstance* csi = NULL;
								  SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itcsi;
								  SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator endcsi = wat.si->componentServiceInstances.listEnd();
								  for (itcsi = wat.si->componentServiceInstances.listBegin(); itcsi != endcsi; itcsi++)
									{
										csi = *itcsi;
										if (!csi) continue;
										//proxy-proxied support feature
										if(!(csi->type.value.compare(comp->csiType.value))) break;
										//End proxy-proxied support feature
										// TODO: figure out number of assignments allowed if (csi->getComponent()) continue;  // We can't assign a CSI to > 1 component.
										// TODO validate CSI dependencies are assigned
										//break;  // We found one!
									}
									logInfo("WORK", "RESPONSE", "workOperationResponse Comp [%s] presenceState [%s] csi [%s] csi->activeComponents.value.push_back(comp);", comp->name.value.c_str(), c_str(comp->presenceState.value), csi->name.value.c_str()); 
									//csi->activeComponents.value.push_back(comp);  // Mark this CSI assigned to this component
									comp->presenceState.value = PresenceState::instantiated;
									comp->numInstantiationAttempts.value++;
									comp->lastInstantiation.value.value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
									comp->pendingOperationExpiration.value.value = nowMs() + comp->timeouts.workAssignment;
									comp->pendingOperation = PendingOperation::workAssignment;
									logInfo("WORK", "RESPONSE", "workOperationResponse Comp [%s] PID [%d] presenceState [%s] ", comp->name.value.c_str(),comp->processId.value, c_str(comp->presenceState.value)); 
								  pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,csi,wat.si,(uint32_t)wat.state,SA_AMF_CSI_ADD_ONE);
								  amfAppRpc->workOperation(hdl, &request);
							  }
						  }
					  }
				  }
			  }
			  
		  }
		  //End proxy-proxied support feature
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

      int pid = comp->processId;
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
            // RPC call is not implemented -- probably code regeneration issue
            assert(0);
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
        comp->processId.value = 0;
        comp->lastError.value = strprintf("Process spawn failure [%s:%d]", strerror(response.err()));
        comp->presenceState.value  = PresenceState::instantiationFailed;
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
    auto standby = si->getNumStandbyAssignments();
    auto active = si->getNumActiveAssignments();
    
    
    }

    void AmfOperations::removeWork(ServiceUnit* su, Wakeable& w)
    {
      assert(su);

      // Update the Service Instances: decrement assignment statistics, and remove this SU from the SI's assigned list.
      // SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator   it;
      //SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator   end = su->assignedServiceInstances.value.end();
      SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator it;
      SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator end = su->assignedServiceInstances.listEnd();
      for (it = su->assignedServiceInstances.listBegin(); it != end; it++)
        {
          ServiceInstance* si = dynamic_cast<ServiceInstance*>(*it);
          assert(si);
          if (si->activeAssignments.find(su) != si->activeAssignments.value.end())
            {
              assert(si->numActiveAssignments.current > 0); // Data is inconsistent
              si->numActiveAssignments.current -= 1;
              si->activeAssignments.erase(su);
            }

          if (si->standbyAssignments.find(su) != si->standbyAssignments.value.end())
            {
              assert(si->numStandbyAssignments.current > 0);  // Data is inconsistent
              si->numStandbyAssignments.current -= 1;
              si->standbyAssignments.erase(su);
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
                  abort(comp, w);  // TODO, need to consolidate the wakeables
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
                          abort(comp,w);  // TODO, need to consolidate the wakeables
                          // TODO ? anything to set in the comp's status?
                          continue; // Go to the next component
                        }

                      // Mark this component with a pending operation.  This will be cleared in the WorkOperationTracker, or by timeout in auditDiscovery
                      comp->pendingOperationExpiration.value.value = nowMs() + comp->timeouts.workAssignment;
                      comp->pendingOperation = PendingOperation::workRemoval;

                      request.set_componentname(comp->name.value.c_str());
                      request.set_componenthandle(0,(const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
                      request.set_operation((uint32_t)SAFplusI::AMF_HA_OPERATION_REMOVE);
                      request.set_target(SA_AMF_CSI_TARGET_ALL);
                      if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
                      request.set_invocation(invocation++);

                      // Now I need to fill up the key/value pairs from the CSI
                      request.clear_keyvaluepairs();

                      pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,nullptr,nullptr,(uint32_t)SAFplusI::AMF_HA_OPERATION_REMOVE,SA_AMF_CSI_TARGET_ALL);

                      amfAppRpc->workOperation(hdl, &request);
                    }
                }
            }
        }
      reportChange();
    }

  void AmfOperations::assignWork(ServiceUnit* su, ServiceInstance* si, HighAvailabilityState state,Wakeable& w)
    {
    SAFplus::Rpc::amfAppRpc::WorkOperationRequest request;
    ComponentServiceInstance* csi = nullptr;

    assert(su->assignedServiceInstances.contains(si) == false);  // We can only assign a particular SI to a particular SU once.

    su->assignedServiceInstances.push_back(si);
    if (state == HighAvailabilityState::active) su->numActiveServiceInstances.current.value+=1;
    if (state == HighAvailabilityState::standby) su->numStandbyServiceInstances.current.value+=1;

    //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
    //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();

    for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
      {

      Component* comp = dynamic_cast<Component*>(*itcomp);
      assert(comp);
      //proxy-proxied support feature
      assert(comp->pendingOperation == PendingOperation::none);  // We should not be adding work to a component if something else is happening to it.
      if((comp->compCategory & SA_AMF_COMP_PROXIED))
        {
			//TODO:maybe checking pending operation or reverse something as number active/standby assignments of si if any
			logInfo("OPS","SRT","No need to assign proxied component [%s] work", comp->name.value.c_str());
			continue;
		}
	  //End proxy-proxied support feature
      Handle hdl;
      if (comp->capabilityModel != CapabilityModel::not_preinstantiable)  // If the component is preinstantiable, it better be instantiated by now (work would not be assigned if it wasn't)
        {
          try
            {
				if((comp->compCategory & SA_AMF_COMP_SA_AWARE))
				{
					hdl = name.getHandle(comp->name);
				}
				/*else if ((comp->compCategory & SA_AMF_COMP_PROXIED))
				{
					hdl = name.getHandle(comp->proxy);
				}*/
              //logInfo("OPS","SRT","name.getHandle(comp->name) Component [%s] hdl[%" PRIx64 ":%" PRIx64 "] with proxy [%s] can be assigned work", comp->name.value.c_str(),hdl.id[0],hdl.id[1],comp->proxy.value.c_str());
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
            //proxy-proxied support feature
			if(!(csi->type.value.compare(comp->proxyCSIType.value)) || !(csi->type.value.compare(comp->csiType.value))) break;
		    //End proxy-proxied support feature
		    // TODO: figure out number of assignments allowed if (csi->getComponent()) continue;  // We can't assign a CSI to > 1 component.
		    // TODO validate CSI dependencies are assigned
		    //break;  // We found one!
        }

      if (itcsi != endcsi)  // We found an assignable CSI and it is the variable "csi"
        {
		if((comp->compCategory & SA_AMF_COMP_PROXIED))
		{
			logInfo("OPS","SRT","no need to assign Component [%s] csi", comp->name.value.c_str());
			continue;
		}
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

            request.set_componentname(comp->name.value.c_str());

             request.add_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.

            request.set_operation((uint32_t)state);
            request.set_target(SA_AMF_CSI_ADD_ONE);
            if ((invocation & 0xFFFFFFFF) == 0xFFFFFFFF) invocation &= 0xFFFFFFFF00000000ULL;  // Don't let increasing invocation numbers overwrite the node or port... ofc this'll never happen 4 billion invocations? :-)
            request.set_invocation(invocation++);

            if (state == HighAvailabilityState::active)
              {
                csi->activeComponents.value.push_back(comp);  // Mark this CSI assigned to this component
              }
            else if (state == HighAvailabilityState::standby)
              {
                csi->standbyComponents.value.push_back(comp); // Mark this CSI assigned to this component
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
          }

        reportChange();
        }
      else
        {
        logInfo("OPS","SRT","Component [%s] handle [%" PRIx64 ":%" PRIx64 "] cannot be assigned work.  No valid Component Service Instance.", comp->name.value.c_str(),hdl.id[0],hdl.id[1]);
        }

      }

    }


    // TODO: fixme there is no actual response because the application may quit within the callback.
  static  SAFplus::Rpc::amfAppRpc::TerminateResponse terminateResponse;

  void AmfOperations::stop(SAFplusAmf::Component* comp,Wakeable& w)
    {
    Handle hdl;
    if (comp->capabilityModel == CapabilityModel::not_preinstantiable)  // Cannot talk to the component, just kill it
      {
        abort(comp,w);
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
        abort(comp,w);
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

  void AmfOperations::abort(SAFplusAmf::Component* comp,Wakeable& w)
    {
    Handle nodeHdl;
    Handle remoteAmfHdl;
    try
      {
      nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
      }
    catch (SAFplus::NameException& n)
      {
      logCritical("OPS","SRT","AMF Entity [%s] is not registered in the name service.  Cannot stop processes on it.", comp->serviceUnit.value->node.value->name.value.c_str());
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
          Process p(comp->processId.value);
          p.signal(SIGTERM);
          }
        else
          {
          logWarning("OPS","SRT","Cannot stop AMF Entity [%s] since it has no associated process id.  If this process still exists, it will become orphaned.", comp->serviceUnit.value->node.value->name.value.c_str());

          }
        comp->presenceState = PresenceState::uninstantiated;
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
      Process p(0);
      if (!(comp->compCategory & SA_AMF_COMP_PROXIED))
      {
		  std::string strCompName("ASP_COMPNAME=");
		  std::string strNodeName("ASP_NODENAME=");
		  std::string strNodeAddr("ASP_NODEADDR=");
		  std::string strPort("SAFPLUS_RECOMMENDED_MSG_PORT=");

		  strCompName.append(comp->name);
		  strNodeName.append(SAFplus::ASP_NODENAME);
		  strNodeAddr.append(std::to_string(SAFplus::ASP_NODEADDR));
		  
		  strPort.append(std::to_string(port));
		  newEnv.push_back(strCompName);
		  newEnv.push_back(strNodeName);
		  newEnv.push_back(strNodeAddr);
		  newEnv.push_back(strPort);
		  p = executeProgram(inst->command.value, newEnv,Process::InheritEnvironment);
          portAllocator.assignPort(port, p.pid);
          comp->processId.value = p.pid;
	  }
	  
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


 void AmfOperations::cleanup(SAFplusAmf::Component* comp,Wakeable& w)
 {
    SAFplusAmf::Cleanup* cleanup = comp->getCleanup();
    if (!cleanup || cleanup->command.value.length()==0)  // no configuration
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
   if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
   {
      std::vector<std::string> newEnv = comp->commandEnvironment.value;
      std::string strCompName("ASP_COMPNAME=");
      strCompName.append(comp->name);
      newEnv.push_back(strCompName);

      logDebug("OPS","CLE","Cleaning up Component [%s] as [%s] locally", comp->name.value.c_str(),cleanup->command.value.c_str());
      int status = executeProgramWithTimeout(cleanup->command.value, newEnv,cleanup->timeout.value,Process::InheritEnvironment);

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

  };
