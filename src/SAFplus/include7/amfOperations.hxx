#pragma once

#include <SAFplusAmf/AdministrativeState.hxx>
namespace SAFplusAmf
  {
  class Component;
  class ServiceUnit;
  class ServiceGroup;
  }

namespace SAFplus
  {
  namespace Rpc
    {
  namespace amfRpc
      {
      class amfRpc_Stub;
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
  void setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt);


  class AmfOperations
    {
  public: // Don't use directly
    Rpc::amfRpc::amfRpc_Stub* amfInternalRpc;

  public:  // Public API
    //? Get the current component state from the node on which it is running
    CompStatus getCompState(SAFplusAmf::Component* comp);
    void start(SAFplusAmf::ServiceGroup* sg,Wakeable& w = *((Wakeable*)nullptr));
    void start(SAFplusAmf::Component* comp,Wakeable& w = *((Wakeable*)nullptr));
    };
  };
