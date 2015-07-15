
#include <clPluginApi.hxx>
#include <clMgtApi.hxx>
#include <clFaultApi.hxx>
#include <clFaultServerIpi.hxx>

namespace SAFplus
{
  enum
    {
    FAULT_POLICY_PLUGIN_ID = 0x53843923,
    FAULT_POLICY_PLUGIN_VER = 1
    };

class FaultPolicyPlugin_1:public ClPlugin
{
  public:
  //? All plugins have a unique policy identifier.  Fault reports can specify a policy id when issued to direct the handling exclusively to this plugin.  Otherwise, plugins will be called in an arbitrary order, until one plugin "accepts" the fault report.
    SAFplus::FaultPolicy policyId;
  //? Initialize your fault policy plugin
    virtual void initialize(FaultServer* faultServer)=0;
  //? The fault server calls your plugin with an incoming fault and you handle it.  If your plugin decides that this fault changes the status of a handle, call faultServer->setFaultState(...).  Return whether you exclusively handled the fault (true), or whether you want to allow other fault handlers a chance to analyze this fault (false).
    virtual bool processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultReporter, SAFplus::Handle faultEntity,int countFaultEvent) = 0;
    //virtual FaultAction processIocNotification(ClIocNotificationIdT eventId, ClIocNodeAddressT nodeAddress, ClIocPortT portId) = 0;
    //virtual FaultAction processMsgTransportNotification(uint_t nodeAddress, uint_t portId,...) = 0;
    FaultPolicyPlugin_1(FaultPolicyPlugin_1 const&) = delete;
    FaultPolicyPlugin_1& operator=(FaultPolicyPlugin_1 const&) = delete;
  protected:  // Only constructable from your derived class from within the .so
    FaultPolicyPlugin_1()
    {
    
    }
};
}
