#include <clAmfPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <clAmfApi.hxx>
#include <vector>

#include <SAFplusAmf.hxx>
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
    virtual void activeAudit(SAFplusAmf::SAFplusAmfRoot* root);
    virtual void standbyAudit(SAFplusAmf::SAFplusAmfRoot* root);
    
    };

  CustomPolicy::CustomPolicy()
    {
    }

  CustomPolicy::~CustomPolicy()
    {
    }
 
  void CustomPolicy::activeAudit(SAFplusAmf::SAFplusAmfRoot* root)
    {
    logInfo("POL","CUSTOM","Active audit");
    assert(root);
    SAFplusAmfRoot* cfg = (SAFplusAmfRoot*) root;

    MgtObject::Iterator it;

    for (it = cfg->serviceGroupList.begin();it !=cfg->serviceGroupList.end(); it++)
      {
      ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
      const std::string& name = sg->name;
           
      logInfo("CUSTOM","AUDIT","Auditing service group %s", name.c_str());
        
      }
    }
  
  void CustomPolicy::standbyAudit(SAFplusAmfRoot* root)
    {
    logInfo("POL","CUSTOM","Standby audit");
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
