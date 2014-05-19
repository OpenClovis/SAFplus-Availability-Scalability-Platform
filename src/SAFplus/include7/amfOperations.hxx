#pragma once

namespace SAFplusAmf
  {
  class Component;
  }

namespace SAFplus
  {
  enum class CompStatus
    {
    Uninstantiated = 0,
    Instantiated = 1
    };

  class AmfOperations
    {
    public:
      //? Get the current component state from the node on which it is running
      CompStatus getCompState(SAFplusAmf::Component* comp);
    };
  };
