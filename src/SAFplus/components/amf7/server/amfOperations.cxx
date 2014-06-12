#include <google/protobuf/stubs/common.h>

#include "amfRpc/amfRpc.pb.h"
#include "stubs/server/amfRpcImpl.hxx"
#include <clRpcChannel.hxx>

#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clOsalApi.hxx>
#include <clAmfPolicyPlugin.hxx>

#include <SAFplusAmf/Component.hxx>
#include <SAFplusAmf/ServiceUnit.hxx>
#include <SAFplusAmf/ServiceGroup.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace SAFplusAmf;
using namespace SAFplus::Rpc::amfRpc;

extern Handle           nodeHandle; //? The handle associated with this node
namespace SAFplus
  {

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



  bool ClAmfPolicyPlugin_1::initialize(SAFplus::AmfOperations* amfOperations)
    {
    amfOps = amfOperations;
    return true;
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

    Handle nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);

    if (nodeHdl == nodeHandle)  // Handle this request locally
      {
      int pid = comp->processId;
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
      logInfo("OP","CMP","Request component state from node %s", comp->serviceUnit.value->node.name.c_str());
      return CompStatus::Uninstantiated;
      }
    }

  class StartCompResp:public google::protobuf::Closure
    {
    public:
    SAFplus::Rpc::amfRpc::StartComponentResponse response;
    SAFplusAmf::Component* comp;
    SAFplus::Wakeable* w;

    virtual void Run()
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
        }
      if (w) w->wake(1,comp);
      delete this;
      }
    };

  void AmfOperations::start(SAFplusAmf::Component* comp,Wakeable& w)
    {
    assert(comp);
    if (!comp->serviceUnit)
      {
      clDbgCodeError(1,("Can't start disconnected comp"));
      }
    if (!comp->serviceUnit.value->node)
      {
      clDbgCodeError(1,("Can't start disconnected comp"));
      }

    Handle nodeHdl = name.getHandle(comp->serviceUnit.value->node.value->name);

    if (nodeHdl == nodeHandle)  // Handle this request locally
      {
      SAFplusAmf::Instantiate* inst = comp->getInstantiate();
      assert(inst);
      Process p = executeProgram(inst->command.value, comp->commandEnvironment.value);
      comp->processId.value = p.pid;
      logInfo("OPS","SRT","Launching Component [%s] as [%s] with process id [%d]", comp->name.c_str(),inst->command.value.c_str(),p.pid);
      if (&w) w.wake(1,(void*)comp);
      }
    else  // RPC call
      {
      logInfo("OP","CMP","Request component start on node %s", comp->serviceUnit.value->node.name.c_str());
      SAFplus::Rpc::RpcChannel channel(&safplusMsgServer, nodeHdl);
      channel->setMsgType(AMF_REQ_HANDLER_TYPE, AMF_REPLY_HANDLER_TYPE);
      amfRpc_Stub service(channel);
      StartComponentRequest req;
      StartCompResp* respData = new respData(w,comp);     
      service.startComponent(NULL,&req, &respData.response, &respData);  // TODO: what happens in a RPC call timeout?

      //service.startComponent(&RpcDestination(iocAddress),&req, &respData.response, &respData);  // TODO: what happens in a RPC call timeout?
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
