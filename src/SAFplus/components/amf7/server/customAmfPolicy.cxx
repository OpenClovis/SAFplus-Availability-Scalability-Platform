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
    virtual void activeAudit(ClMgtObject* root);
    virtual void standbyAudit(ClMgtObject* root);
    
  };

  CustomPolicy::CustomPolicy()
  {
  }

   CustomPolicy::~CustomPolicy()
  {
  }
 
 void CustomPolicy::activeAudit(ClMgtObject* root)
 {
   logInfo("POL","CUSTOM","Active audit");
   assert(root);
   SAFplusAmfRoot* cfg = (SAFplusAmfRoot*) root;

   ClMgtObjectMap::iterator it;

   for (it = cfg->serviceGroupList.begin();it != cfg->serviceGroupList.end(); it++)
     {
       vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) it->second;
       int nObjs = objs->size();
       assert(nObjs <= 1);  // There better not be multiple AMF entities with the same name
       for(int i = 0; i < nObjs; i++)
         {
           ServiceGroup* sg = (ServiceGroup*) (*objs)[i];
           const std::string& name = sg->name.Value;
           
           logInfo("CUSTOM","AUDIT","Auditing service group %s", name.c_str());
         }
     }
 }
  
 void CustomPolicy::standbyAudit(ClMgtObject* root)
 {
   logInfo("POL","CUSTOM","Standby audit");
 }
  
  CustomPolicy api;
}

extern "C" ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
{
    // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

    // Initialize the pluginData structure
  SAFplus::api.pluginId         = CL_AMF_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = CL_AMF_POLICY_PLUGIN_VER;

  SAFplus::api.policyId == SAFplus::AmfRedundancyPolicy::Custom;

    // return it
    return (ClPlugin*) &SAFplus::api;
}
