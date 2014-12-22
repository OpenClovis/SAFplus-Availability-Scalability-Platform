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
        virtual FaultAction processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent);
        virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);

    };

    CustomFaultPolicy::CustomFaultPolicy()
    {

    }
    CustomFaultPolicy::~CustomFaultPolicy()
    {

    }
    FaultAction CustomFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent)
    {
    	logInfo("POL","CUS","Received fault event of fault Entity with processId [%d] , node [%d] fault count [%d] ", faultEntity.getProcess(),faultEntity.getNode(),countFaultEvent);
        logInfo("POL","CUS","Default plugin : Process fault event");
        //assert(root);
        //TODO
        if (countFaultEvent>=2)
        {
        	return FaultAction::ACTION_STOP;
        }
        return FaultAction::ACTION_IGNORE;
    }
    FaultAction CustomFaultPolicy::processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId)
    {
        logInfo("POL","CUSTOM","process ioc notification");
        return FaultAction::ACTION_STOP;
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

