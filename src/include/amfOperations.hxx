#pragma once
#include <boost/unordered_map.hpp>
#include <SAFplusAmf/AdministrativeState.hxx>
#include <SAFplusAmf/HighAvailabilityState.hxx>
namespace SAFplusAmf
  {
  class Component;
  class ServiceUnit;
  class ServiceGroup;
  class ServiceInstance;
  class ComponentServiceInstance;
  }

namespace SAFplus
  {

  class WorkOperationTracker
    {
    public:
    SAFplusAmf::Component* comp;
    SAFplusAmf::ComponentServiceInstance* csi;
    SAFplusAmf::ServiceInstance* si;
    uint state;
    uint target;
    uint64_t issueTime;
    WorkOperationTracker(SAFplusAmf::Component* c,SAFplusAmf::ComponentServiceInstance* cwork,SAFplusAmf::ServiceInstance* work,uint statep, uint targetp);
    WorkOperationTracker() { comp = nullptr; csi = nullptr; issueTime=0; state=0; target=0; }
    };

  typedef boost::unordered_map<uint64_t,WorkOperationTracker> PendingWorkOperationMap;
  

  namespace Rpc
    {
    namespace amfRpc
      {
      class amfRpc_Stub;
      };
    namespace amfAppRpc
      {
      class amfAppRpc_Stub;
      };
   };


  enum class CompStatus
    {
    Uninstantiated = 0,
    Instantiated = 1
    };

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::Component* comp);
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceUnit* su);
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceGroup* sg);
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceInstance* si);
  void setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt);


  class AmfOperations
    {
  public: // Don't use directly
    Rpc::amfRpc::amfRpc_Stub* amfInternalRpc;

    Rpc::amfAppRpc::amfAppRpc_Stub* amfAppRpc;
    uint64_t invocation;  // keeps track of the particular request being sent to the app (at the SAF level)
      bool changed;
    PendingWorkOperationMap pendingWorkOperations;

    AmfOperations()
    {
      changed = false;
    amfInternalRpc = nullptr;
    amfAppRpc = nullptr;
    // Make the invocation unique per AMF to ensure that failover or restart does not reuse invocation by accident (although, would reuse really matter?)
    invocation = (SAFplus::ASP_NODEADDR << 16) | SAFplus::iocPort;
    invocation <<= 32;  // TODO: should checkpointed value be used?
    }

  public:  // Public API
    //? Get the current component state from the node on which it is running
    CompStatus getCompState(SAFplusAmf::Component* comp);
    void start(SAFplusAmf::ServiceGroup* sg,Wakeable& w = *((Wakeable*)nullptr));
    void start(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));

    void assignWork(SAFplusAmf::ServiceUnit* su, SAFplusAmf::ServiceInstance* si, SAFplusAmf::HighAvailabilityState state,Wakeable& w = *((Wakeable*)nullptr));
    void removeWork(SAFplusAmf::ServiceInstance* si,Wakeable& w = *((Wakeable*)nullptr));
    void removeWork(SAFplusAmf::ServiceUnit* su, Wakeable& w = *((Wakeable*)nullptr));  // TODO: add indicator of how fast to remove work: kill -9, work removal or quiesce

    void workOperationResponse(uint64_t invocation, uint32_t result);

      //? Report that AMF state has changed.  This will cause the AMF to rerun its evaluation loop right away.  If you call another amfOperations API, you do not need to call this.
      void reportChange(void) { changed=true;}
    };
  };
