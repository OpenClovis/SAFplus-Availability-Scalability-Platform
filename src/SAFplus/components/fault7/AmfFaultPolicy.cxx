#include <clFaultPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <vector>
#include <clFaultIpi.hxx>




using namespace std;
using namespace SAFplus;


namespace SAFplus
{
    class AmfFaultPolicy:public FaultPolicyPlugin
    {
      public:
    	AmfFaultPolicy();
        ~AmfFaultPolicy();
        virtual FaultAction processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent);
        //virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);

    };

    AmfFaultPolicy::AmfFaultPolicy()
    {

    }

    AmfFaultPolicy::~AmfFaultPolicy()
    {

    }
 
    FaultAction AmfFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent)
    {
        logInfo("POL","AMF","Received fault event of fault Entity with processId [%d] , node [%d] fault count [%d] ", faultEntity.getProcess(),faultEntity.getNode(),countFaultEvent);
        logInfo("POL","AMF","Default plugin : Process fault event");
        if (countFaultEvent>=2)
        {
          	return FaultAction::ACTION_STOP;
        }
        return FaultAction::ACTION_IGNORE;
    }
#if 0
    FaultAction AmfFaultPolicy::processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId)
    {
        logInfo("POL","CUSTOM","process ioc notification");
        return FaultAction::ACTION_STOP;
    }
#endif
    static AmfFaultPolicy api;
}

extern "C" ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = FAULT_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = FAULT_POLICY_PLUGIN_VER;
  SAFplus::api.policyId = SAFplus::FaultPolicy::Custom;

  // return it
  return (ClPlugin*) &SAFplus::api;
}
