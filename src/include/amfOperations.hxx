#pragma once
#include <clFaultApi.hxx>
#include <boost/unordered_map.hpp>
#include <SAFplusAmf/AdministrativeState.hxx>
#include <SAFplusAmf/HighAvailabilityState.hxx>
#include <clThreadApi.hxx>

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
   
    enum ExtendedState
      {
      TerminationState = 0xff00
      };

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
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ComponentServiceInstance* csi);
  ClRcT setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt, bool writeChanges=false);
  ClRcT setAdminState(SAFplusAmf::Node* node,SAFplusAmf::AdministrativeState tgt, bool writeChanges=false);
  ClRcT setAdminState(SAFplusAmf::ServiceUnit* su,SAFplusAmf::AdministrativeState tgt, bool writeChanges=false);
  ClRcT setAdminState(SAFplusAmf::ServiceInstance* si,SAFplusAmf::AdministrativeState tgt, bool writeChanges=false);
  inline bool operationPendingForComp(SAFplusAmf::Component* comp);
  bool operationsPendingForSU(SAFplusAmf::ServiceUnit* su);
  bool operationsPendingForSG(SAFplusAmf::ServiceGroup* sg);
  bool operationsPendingForSI(SAFplusAmf::ServiceInstance* si);
  bool operationsPendingForNode(SAFplusAmf::Node* node);
  void createSymlink(const std::string& command, int pid);

  class AmfOperations
    {
  protected:
      void assignWorkCallback(SAFplusAmf::Component* comp);
      bool compUpdateProxiedComponents(SAFplusAmf::Component* proxy, SAFplusAmf::ComponentServiceInstance* csi);
  public: // Don't use directly
      Rpc::amfRpc::amfRpc_Stub* amfInternalRpc;

      Rpc::amfAppRpc::amfAppRpc_Stub* amfAppRpc;
      uint64_t invocation;  // keeps track of the particular request being sent to the app (at the SAF level)
      bool changed;
      PendingWorkOperationMap pendingWorkOperations;
      Fault fault;
      //bool nodeGracefulSwitchover;
      SAFplus::ProcSem mutex;

      AmfOperations()
      {
        changed = false;
        amfInternalRpc = nullptr;
        amfAppRpc = nullptr;
        // Make the invocation unique per AMF to ensure that failover or restart does not reuse invocation by accident (although, would reuse really matter?)
        invocation = (SAFplus::ASP_NODEADDR << 16) | SAFplus::iocPort;
        invocation <<= 32;  // TODO: should checkpointed value be used?
        //fault.init();
        //nodeGracefulSwitchover = false;
        mutex.init("MgtObject",1);
      }

  public:  // Public API
    //? Get the current component state from the node on which it is running
    CompStatus getCompState(SAFplusAmf::Component* comp);
    void start(SAFplusAmf::ServiceGroup* sg,Wakeable& w = *((Wakeable*)nullptr));
    void start(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));
    void abort(SAFplusAmf::Component* comp, bool changePS = true, Wakeable& w = *((Wakeable*)nullptr));  // Stops a component via a signal (without removing work)
    void stop(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));  // Stops a component via terminate RPC
    void cleanup(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));  // Stops a component via terminate RPC
    void rebootNode(SAFplusAmf::Node* node, Wakeable& w = *((Wakeable*)nullptr));

    void assignWork(SAFplusAmf::ServiceUnit* su, SAFplusAmf::ServiceInstance* si, SAFplusAmf::HighAvailabilityState state,Wakeable& w = *((Wakeable*)nullptr));
    void removeWork(SAFplusAmf::ServiceInstance* si,Wakeable& w = *((Wakeable*)nullptr));
    void removeWork(SAFplusAmf::ServiceUnit* su, Wakeable& w = *((Wakeable*)nullptr));  // TODO: add indicator of how fast to remove work: kill -9, work removal or quiesce

    void workOperationResponse(uint64_t invocation, uint32_t result);

      //? Report that AMF state has changed.  This will cause the AMF to rerun its evaluation loop right away.  If you call another amfOperations API, you do not need to call this.
    void reportChange(void) { changed=true;}
    bool suContainsSaAwareComp(SAFplusAmf::ServiceUnit* su);

    void nodeRestart(SAFplusAmf::Node* node,Wakeable& w = *((Wakeable*)nullptr));
    void serviceUnitRestart(SAFplusAmf::ServiceUnit* su,Wakeable& w = *((Wakeable*)nullptr));
    void componentRestart(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));

    ClRcT swapSI(std::string siName);

    ClRcT sgAdjust(const SAFplusAmf::ServiceGroup* sg);

    ClRcT nodeErrorReport(SAFplusAmf::Node* node, bool gracefulSwitchover, bool shutdownAmf, bool restartAmf, bool rebootNode);
    ClRcT nodeErrorClear(SAFplusAmf::Node* node);

    ClRcT removeThenAssignWork(SAFplusAmf::ServiceInstance* si, SAFplusAmf::ServiceUnit* su, SAFplusAmf::HighAvailabilityState currentState, SAFplusAmf::HighAvailabilityState assignState);
    ClRcT assignSUtoSI(std::string siName, std::string activceSUName, std::string standbySUName);
    ClRcT removeWorkWithoutAppRemove(SAFplusAmf::ServiceUnit* su);


    };
  };
