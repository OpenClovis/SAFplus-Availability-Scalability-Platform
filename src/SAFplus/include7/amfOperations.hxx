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
  enum class CompStatus
    {
    Uninstantiated = 0,
    Instantiated = 1
    };

  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::Component* comp);
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceUnit* su);
  SAFplusAmf::AdministrativeState effectiveAdminState(SAFplusAmf::ServiceGroup* sg);
  void setAdminState(SAFplusAmf::ServiceGroup* sg,SAFplusAmf::AdministrativeState tgt);

  void start(SAFplusAmf::ServiceGroup* sg);

  class AmfOperations
    {
    public:
      //? Get the current component state from the node on which it is running
    CompStatus getCompState(SAFplusAmf::Component* comp);
    };
  };
