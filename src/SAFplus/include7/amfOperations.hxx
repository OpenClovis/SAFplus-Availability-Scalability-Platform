#pragma once

namespace SAFplusAmf
  {
  class Component;
  }

namespace SAFplusI
  {
  enum class CompStatus
    {
    Uninstantiated = 0,
    Instantiated = 1
    };

  class AmfOperations
    {
    public:
      CompStatus getCompState(SAFplusAmf::Component* comp);
    };
  };
