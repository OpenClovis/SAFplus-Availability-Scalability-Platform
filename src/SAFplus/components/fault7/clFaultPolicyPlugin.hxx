
#include <clPluginApi.hxx>
#include <clMgtApi.hxx>
#include <clFaultApi.hxx>

using namespace SAFplusI;
namespace SAFplus
{

class FaultPolicyPlugin:public ClPlugin
{
  public:
    SAFplus::FaultPolicy policyId;
    virtual void processFaultEvent(SAFplus::FaultEventData fault,SAFplus::Handle faultEntity) = 0;
    FaultPolicyPlugin(FaultPolicyPlugin const&) = delete;
    FaultPolicyPlugin& operator=(FaultPolicyPlugin const&) = delete;
  protected:  // Only constructable from your derived class from within the .so
    FaultPolicyPlugin()
    {
    
    }
};
}
