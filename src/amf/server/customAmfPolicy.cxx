#include <clAmfPolicyPlugin.hxx>
#include <clLogIpi.hxx>
#include <clAmfApi.hxx>
#include <vector>

#include <SAFplusAmfModule.hxx>
#include <ServiceGroup.hxx>

using namespace std;
using namespace SAFplus;
using namespace SAFplusAmf;

namespace SAFplus
  {
  
  class CustomPolicy:public ClAmfPolicyPlugin_1
    {
  public:
    CustomPolicy();
    ~CustomPolicy();
    virtual void activeAudit(SAFplusAmf::SAFplusAmfModule* root);
    virtual void standbyAudit(SAFplusAmf::SAFplusAmfModule* root);
    virtual void compFaultReport(Component* comp, const SAFplusAmf::Recovery recommRecovery = SAFplusAmf::Recovery::NoRecommendation);
    
    };

  CustomPolicy::CustomPolicy()
    {
    }

  CustomPolicy::~CustomPolicy()
    {
    }
 
  void CustomPolicy::activeAudit(SAFplusAmf::SAFplusAmfModule* root)
    {
    logInfo("POL","CUSTOM","Active audit");
    assert(root);
    SAFplusAmfModule* cfg = (SAFplusAmfModule*) root;

    MgtObject::Iterator it;

    for (it = cfg->safplusAmf.serviceGroupList.begin();it !=cfg->safplusAmf.serviceGroupList.end(); it++)
      {
      ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
      const std::string& name = sg->name;
           
      logInfo("CUSTOM","AUDIT","Auditing service group %s", name.c_str());
        
      }
    }
  
  void CustomPolicy::standbyAudit(SAFplusAmfModule* root)
    {
    logInfo("POL","CUSTOM","Standby audit");
    }
  
  void CustomPolicy::compFaultReport(Component* comp, const SAFplusAmf::Recovery recommRecovery)
  {

  }

  static CustomPolicy api;
  }

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
  {
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = CL_AMF_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = CL_AMF_POLICY_PLUGIN_VER;

  SAFplus::api.policyId = SAFplus::AmfRedundancyPolicy::Custom;

  // return it
  return (ClPlugin*) &SAFplus::api;
  }
