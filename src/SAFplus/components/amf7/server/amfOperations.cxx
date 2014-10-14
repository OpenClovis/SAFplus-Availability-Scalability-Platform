#include <chrono>
#include <google/protobuf/stubs/common.h>
#include <saAmf.h>

#include "amfRpc/amfRpc.pb.hxx"
#include "amfRpc/amfRpc.hxx"
#include "amfAppRpc/amfAppRpc.hxx"
#include <clRpcChannel.hxx>

#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clOsalApi.hxx>
#include <clAmfPolicyPlugin.hxx>

#include <SAFplusAmf/Component.hxx>
#include <SAFplusAmf/ComponentServiceInstance.hxx>
#include <SAFplusAmf/ServiceUnit.hxx>
#include <SAFplusAmf/ServiceGroup.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

extern Handle           nodeHandle; //? The handle associated with this node
namespace SAFplus
  {

    WorkOperationTracker::WorkOperationTracker(SAFplusAmf::Component* c,SAFplusAmf::ComponentServiceInstance* cwork,SAFplusAmf::ServiceInstance* work,uint statep, uint targetp)
    {
    comp = c; csi = cwork; si=work; state = statep; target = targetp;
    issueTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }


  // Move this service group and all contained elements to the specified state.
  void setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt)
    {
    SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
    SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator endsu = sg->serviceUnits.value.end();
    for (itsu = sg->serviceUnits.value.begin(); itsu != endsu; itsu++)
      {
      //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
      ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
      const std::string& suName = su->name;
      if (su->adminState.value != tgt)
        {
        logInfo("N+M","AUDIT","Setting service unit [%s] to admin state [%d]",suName.c_str(),tgt);
        su->adminState.value = tgt;
        // TODO: transactional and event
        }
      }
      if (sg->adminState.value != tgt)
        {
        logInfo("N+M","AUDIT","Setting service group [%s] to admin state [%d]",sg->name.c_str(),tgt);
        sg->adminState.value = tgt;
        }
    }

  // The effective administrative state is calculated by looking at the admin state of the containers: SU, SG, node, and application.  If any contain a "more restrictive" state, then this component inherits the more restrictive state.  The states from most to least restrictive is off, idle, on.  The Application object is optional.
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::Component* comp)
    {
    assert(comp);
    if ((!comp->serviceUnit.value)||(!comp->serviceUnit.value->node.value)||(!comp->serviceUnit.value->serviceGroup.value))  // This component is not properly hooked up to other entities; is must be off
      {
      logInfo("N+M","AUDIT","Component [%s] entity group is not properly configured",comp->name.c_str());
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

  bool ClAmfPolicyPlugin_1::initialize(SAFplus::AmfOperations* amfOperations)
    {
    amfOps = amfOperations;
    return true;
    }

  void AmfOperations::workOperationResponse(uint64_t invocation, uint32_t result)
    {
    if (1)
      {
      WorkOperationTracker& wat = pendingWorkOperations.at(invocation);
      logInfo("AMF","OPS","Work Operation response on component [%s] invocation [%lx] result [%d]",wat.comp->name.c_str(),invocation, result);

      if ( wat.state <= (int) HighAvailabilityState::quiescing)
        {
        if (result == SA_AIS_OK)  // TODO: actually, I think I need to call into the redundancy model plugin to correctly process the result.
          {
          wat.comp->haState = (SAFplusAmf::HighAvailabilityState) wat.state; // TODO: won't work with multiple assignments of active and standby, for example
          wat.si->assignmentState = AssignmentState::fullyAssigned;  // TODO: for now just make the SI happy to see something work
          }
        }
      else // work removal
        {
        }

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
        logInfo("OP","CMP","Request component [%s] state from node [%s]", comp->name.c_str(), comp->serviceUnit.value->node.value->name.c_str());

        ProcessInfoRequest req;
        req.set_pid(pid);
        ProcessInfoResponse resp;
        amfInternalRpc->processInfo(nodeHdl,&req, &resp);
        if (resp.running()) 
          {
          // TODO: I need to compare the process command line with my command line to make sure that my proc didn't die and another reuse the pid
          return CompStatus::Instantiated;
          }
        else return CompStatus::Uninstantiated;
        }
      }
    catch (NameException& e)
      {
      return CompStatus::Uninstantiated;
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
        comp->presence.value  = PresenceState::instantiationFailed;
        }
      else if (comp)
        {
        comp->processId.value = response.pid();
        comp->presence.value  = PresenceState::instantiating;
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
    SAFplusAmf::StandbyAssignments* standby = si->getStandbyAssignments();
    SAFplusAmf::ActiveAssignments* active = si->getActiveAssignments();
    
    
    }

  void AmfOperations::assignWork(ServiceUnit* su, ServiceInstance* si, HighAvailabilityState state,Wakeable& w)
    {
    SAFplus::Rpc::amfAppRpc::WorkOperationRequest request;
    // TODO fill request
    Component* comp = nullptr; // TODO: su down to comp
    ComponentServiceInstance* csi = nullptr;

    SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
    SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
    for (itcomp = su->components.value.begin(); itcomp != endcomp; itcomp++)
      {
      Component* comp = dynamic_cast<Component*>(*itcomp);
      assert(comp);

      Handle hdl;
      try
          {
          hdl = name.getHandle(comp->name);
          }
      catch (SAFplus::NameException& n)
          {
          logCritical("OPS","SRT","Component [%s] is not registered in the name service.  Cannot control it.", comp->name.c_str());
          comp->lastError.value = "Component's name is not registered in the name service so address cannot be determined.";
          break; // TODO: right now go to the next component, however assignment should not occur on ANY components if all components are not accessible. 
          }

      // TODO: let's add an extension to SAF in the SI which basically just says "ignore CSIs" and/or "assign all components to the same CSI".  This makes the model simpler

      // Let's find a CSI that we can apply to this component
      SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator itcsi;
      SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator endcsi = si->componentServiceInstances.value.end();

      SAFplusAmf::ComponentServiceInstance* csi = NULL;
      for (itcsi = si->componentServiceInstances.value.begin(); itcsi != endcsi; itcsi++)
        {
        csi = *itcsi;
        if (!csi) continue;
        // TODO: figure out number of assignments allowed if (csi->getComponent()) continue;  // We can't assign a CSI to > 1 component.
        // TODO validate CSI dependencies are assigned
        break;  // We found one!
        }

      if (itcsi != endcsi)  // We found an assignable CSI and it is the variable "csi"
        {
        logInfo("OPS","SRT","Component [%s] handle [%lx.%lx] is being assigned work", comp->name.c_str(),hdl.id[0],hdl.id[1]);
        request.set_componentname(comp->name.c_str());
        request.set_componenthandle((const char*) &hdl, sizeof(Handle)); // [libprotobuf ERROR google/protobuf/wire_format.cc:1053] String field contains invalid UTF-8 data when serializing a protocol buffer. Use the 'bytes' type if you intend to send raw bytes.
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
          MgtProv<std::string>* kv = dynamic_cast<MgtProv<std::string>*>(it->second); 
          kvp->set_thekey(kv->name.c_str());  // it->first().c_str()
          kvp->set_thevalue(kv->value.c_str());
          }

        pendingWorkOperations[request.invocation()] = WorkOperationTracker(comp,csi,si,(uint32_t)state,SA_AMF_CSI_ADD_ONE);

        amfAppRpc->workOperation(hdl, &request);
        }
      else
        {
        logInfo("OPS","SRT","Component [%s] handle [%lx.%lx] cannot be assigned work.  No valid Component Service Instance.", comp->name.c_str(),hdl.id[0],hdl.id[1]);
        }

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

    Handle nodeHdl;
    try
      {
      nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);
      }
    catch (SAFplus::NameException& n)
      {
      logCritical("OPS","SRT","AMF Entity [%s] is not registered in the name service.  Cannot start processes on it.", comp->serviceUnit.value->node.value->name.c_str());
      comp->lastError.value = "Component's node is not registered in the name service so address cannot be determined.";
      if (&w) w.wake(1,(void*)comp);
      return;
      }

    SAFplusAmf::Instantiate* inst = comp->getInstantiate();
    //assert(inst);
    if (!inst)  // Bad configuration
      {
      logWarning("OPS","SRT","Component [%s] has improper configuration (no instantiation information). Cannot start", comp->name.c_str());
      comp->operState = false;  // A configuration error isn't going to heal itself -- component needs manual intervention then repair
      comp->lastError.value = "No instantiation information";
      if (&w) w.wake(1,(void*)comp);
      return;
      }
    else if (nodeHdl == nodeHandle)  // Handle this request locally.  This is an optimization.  The RPC call will also work locally.
      {
      comp->presence.value  = PresenceState::instantiating;
      std::vector<std::string> newEnv = comp->commandEnvironment.value;
      std::string strCompName("ASP_COMPNAME=");
      std::string strNodeName("ASP_NODENAME=");
      std::string strNodeAddr("ASP_NODEADDR=");
      strCompName.append(comp->name);
      strNodeName.append(SAFplus::ASP_NODENAME);
      strNodeAddr.append(std::to_string(SAFplus::ASP_NODEADDR));
      newEnv.push_back(strCompName);
      newEnv.push_back(strNodeName);
      newEnv.push_back(strNodeAddr);
      Process p = executeProgram(inst->command.value, newEnv,Process::InheritEnvironment);
      comp->processId.value = p.pid;

      logInfo("OPS","SRT","Launching Component [%s] as [%s] locally with process id [%d]", comp->name.c_str(),inst->command.value.c_str(),p.pid);
      if (&w) w.wake(1,(void*)comp);
      }
    else  // RPC call
      {
      logInfo("OP","CMP","Request launch component [%s] as [%s] on node [%s]", comp->name.c_str(),inst->command.value.c_str(),comp->serviceUnit.value->node.value->name.c_str());

#if 0
      SAFplus::Rpc::RpcChannel channel(&safplusMsgServer, nodeHdl);
      channel->setMsgType(AMF_REQ_HANDLER_TYPE, AMF_REPLY_HANDLER_TYPE);
      amfRpc_Stub service(channel);
      StartComponentRequest req;
      StartCompResp respData(w,comp);
      service.startComponent(INVALID_HDL, &req, &respData.response, respData);  // TODO: what happens in a RPC call timeout?
#endif
      StartComponentRequest req;
      req.set_name(comp->name.c_str());
      req.set_command(inst->command.value.c_str());
      StartCompResp* resp = new StartCompResp(&w,comp);
      amfInternalRpc->startComponent(nodeHdl,&req, &resp->response,*resp);
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
