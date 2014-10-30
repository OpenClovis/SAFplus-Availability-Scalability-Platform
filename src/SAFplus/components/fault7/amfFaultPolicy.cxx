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
        logInfo("POL","AMF","received fault event of fault Entity with processId [%d] , node [id] ", faultEntity.getProcess(),faultEntity.getNode());
        logInfo("POL","AMF","fault event data severity [%d] , cause [%d] , catagory [%d] , state [%d] ", fault.severity,fault.cause,fault.category,fault.alarmState);

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
