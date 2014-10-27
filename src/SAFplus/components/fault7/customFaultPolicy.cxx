#include <clFaultPolicyPlugin.hxx>
#include <clFaultApi.hxx>
#include <clLogApi.hxx>
#include <vector>


using namespace std;
using namespace SAFplus;


namespace SAFplus
{
    class CustomFaultPolicy:public FaultPolicyPlugin
    {
      public:
        CustomFaultPolicy();
        ~CustomFaultPolicy();
        virtual void processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultEntity);
    };

    CustomFaultPolicy::CustomFaultPolicy()
    {

    }

    CustomFaultPolicy::~CustomFaultPolicy()
    {

    }
 
    void CustomFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultEntity)
    {
        logInfo("POL","CUSTOM","process fault event");
        //assert(root);
        //TODO
    }
    static CustomFaultPolicy api;
}

extern "C" ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = SAFplus::FAULT_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = SAFplus::FAULT_POLICY_PLUGIN_VER;

  SAFplus::api.policyId = SAFplus::FaultPolicy::AMF;

  // return it
  return (ClPlugin*) &SAFplus::api;
}

