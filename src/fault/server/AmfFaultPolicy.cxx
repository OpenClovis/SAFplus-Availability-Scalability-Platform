#include <clFaultPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <vector>
#include <FaultSharedMem.hxx>

using namespace std;
using namespace SAFplus;

namespace SAFplus
{
    class AmfFaultPolicy:public FaultPolicyPlugin_1
    {
      public:
      FaultServer* faultServer;
        AmfFaultPolicy();
        ~AmfFaultPolicy();

      virtual void initialize(FaultServer* faultServer);

      virtual bool processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent);
//        virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId);

    };
    AmfFaultPolicy::AmfFaultPolicy()
    {
      policyId = FaultPolicy::AMF;
    }

    AmfFaultPolicy::~AmfFaultPolicy()
    {

    }

  void AmfFaultPolicy::initialize(FaultServer* fs)
  {
    faultServer = fs;
  }

    bool AmfFaultPolicy::processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter,SAFplus::Handle faultEntity,int countFaultEvent)
    {
      logInfo("POL","AMF","Received fault report from [%" PRIx64 ":%" PRIx64 "]:  Entity [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).  Current fault count is [%d] ", faultReporter.id[0], faultReporter.id[1], faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess(), countFaultEvent);

        // If this fault comes from an AMF
      if (faultReporter.getPort() == SAFplusI::AMF_IOC_PORT)
        {
          if ((faultEntity.getPort() == 0)&&(fault.cause == AlarmProbableCause::RECEIVER_FAILURE))  // port==0 means that this fault concerns the whole node, its a Keep-alive failure
            {
            assert(faultServer);
            faultServer->setFaultState(faultEntity,FaultState::STATE_DOWN);
            logWarning("FLT","POL","Entity DOWN [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).", faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess());
            return true;          
            }
          else if (fault.cause == AlarmProbableCause::RECEIVER_FAILURE)  // Trust RPC timeouts reported by the AMF
            {
              // TODO: wait for a few timeouts
            assert(faultServer);
            faultServer->setFaultState(faultEntity,FaultState::STATE_DOWN);
            logWarning("FLT","POL","Entity DOWN [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).", faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess());  
            return true;          
            }
        // If this fault comes from an AMF and its telling me that there was a crash the trust it because the local AMF monitors its local processes.
          else if (fault.cause == AlarmProbableCause::SOFTWARE_ERROR)
            {
            if (fault.state == AlarmState::ASSERT)
              {
                assert(faultServer);
                faultServer->setFaultState(faultEntity,FaultState::STATE_DOWN);
                logWarning("FLT","POL","Entity DOWN [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).", faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess());
                return true;
              }
            
            if (fault.state == AlarmState::CLEAR)
              {  // Probably never going to happen.  Program will die and AMF will start a new one, reregistering the handle
              logInfo("FLT","POL","Entity UP [%" PRIx64 ":%" PRIx64 "] (on node [%d], port [%d]).", faultEntity.id[0], faultEntity.id[1], faultEntity.getNode(), faultEntity.getProcess());
              faultServer->setFaultState(faultEntity,FaultState::STATE_UP);
              return true;
              }
            }
        }

#if 0
        // If I get multiple reports from other entities, then start believing them
        if (countFaultEvent>=2)
        {
          // TODO
        }
#endif

        return false;
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
    SAFplus::api.policyId = SAFplus::FaultPolicy::AMF;
    // return it
    return (ClPlugin*) &SAFplus::api;
}
