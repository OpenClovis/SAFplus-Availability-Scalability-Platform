#include <clFaultPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <vector>
#include <FaultSharedMem.hxx>

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
//        virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);

    };
    AmfFaultPolicy::AmfFaultPolicy()
    {

    }

    AmfFaultPolicy::~AmfFaultPolicy()
    {

    }

    FaultAction AmfFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent)
    {
      logInfo("POL","AMF","Received fault from [%" PRIx64 ":%" PRIx64 "]:  Entity [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).  Current fault count is [%d] ", faultReporter.id[0], faultReporter.id[1], faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess(), countFaultEvent);

        // If this fault comes from an AMF and its telling me that there was a crash the trust it because the local AMF monitors its local processes.
      if ((faultReporter.getPort() == SAFplusI::AMF_IOC_PORT)&&(fault.cause == SAFplus::AlarmProbableCause::ALARM_PROB_CAUSE_SOFTWARE_ERROR))
          {
            if ((fault.alarmState == SAFplus::AlarmState::ALARM_STATE_ASSERT)&&(fault.severity >= SAFplus::AlarmSeverity::ALARM_SEVERITY_MAJOR))
              return FaultAction::ACTION_STOP;
          }

        // If I get multiple reports from other entities, then start believing them
        if (countFaultEvent>=2)
        {
            return FaultAction::ACTION_STOP;
        }
        return FaultAction::ACTION_IGNORE;
    }

#if 0
    FaultAction AmfFaultPolicy::processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId)
    {
        logInfo("POL","AMF","Process ioc notification");
        //TODO :
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
