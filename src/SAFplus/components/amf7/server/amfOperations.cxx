#include <amfOperations.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clOsalApi.hxx>

#include <SAFplusAmf/Component.hxx>
using namespace SAFplusAmf;
using namespace SAFplus;

extern Handle           nodeHandle; //? The handle associated with this node
namespace SAFplus
  {

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

    Handle nodeHdl = name.getHandle(comp->serviceUnit.value->node.name);

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


  };
