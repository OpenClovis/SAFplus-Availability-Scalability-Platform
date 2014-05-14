#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>

using namespace SAFplus;

namespace SAFplusI
  {

  AmfOperations::getCompState(SAFplusAmf::Component* comp)
    {
    assert(comp);
    if (!comp->serviceUnit)
      {
      return // uninstantiated;
      }
    if (!comp->serviceUnit->node)
      {
      return // uninstantiated;
      }

    Handle nodeHdl = name.getHandle(comp->serviceUnit->node.name);

    if (nodeHdl == myHandle)  // Handle this request locally
      {
      }
    else  // RPC call
      {
      logInfo("OP","CMP","Request component state from node %s", comp->serviceUnit->node.name);
      }
    }


  };
