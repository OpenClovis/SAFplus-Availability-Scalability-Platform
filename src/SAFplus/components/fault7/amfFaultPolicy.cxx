#include <clFaultPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <vector>
#include <clFaultApi.hxx>


using namespace std;
using namespace SAFplus;


namespace SAFplus
{
    class AmfFaultPolicy:public FaultPolicyPlugin
    {
      public:
    	AmfFaultPolicy();
        ~AmfFaultPolicy();
        virtual void processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultEntity);
    };

    AmfFaultPolicy::AmfFaultPolicy()
    {

    }

    AmfFaultPolicy::~AmfFaultPolicy()
    {

    }
 
    void AmfFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultEntity)
    {
        logInfo("POL","AMF","process fault event");
        //assert(root);
        //TODO
    }
    static AmfFaultPolicy api;
}

extern "C" ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = FAULT_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = FAULT_POLICY_PLUGIN_VER;
  SAFplus::api.policyId = SAFplus::FaultPolicy::Custom;

  // return it
  return (ClPlugin*) &SAFplus::api;
}
