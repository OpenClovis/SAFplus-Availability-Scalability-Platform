
#include <clPluginApi.hxx>
#include <clMgtApi.hxx>
#include <clFaultApi.hxx>

//using namespace SAFplusI;
namespace SAFplus
{

class FaultPolicyPlugin:public ClPlugin
{
  public:
    SAFplus::FaultPolicy policyId;
    virtual FaultAction processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter, SAFplus::Handle faultEntity,int countFaultEvent) = 0;
    //virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId) = 0;
    //virtual FaultAction processMsgTransportNotification(uint_t nodeAddress, uint_t portId,...) = 0;
    FaultPolicyPlugin(FaultPolicyPlugin const&) = delete;
    FaultPolicyPlugin& operator=(FaultPolicyPlugin const&) = delete;
  protected:  // Only constructable from your derived class from within the .so
    FaultPolicyPlugin()
    {
    
    }
};
}
