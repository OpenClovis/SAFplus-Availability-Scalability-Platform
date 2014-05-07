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
  
  class NplusMPolicy:public ClAmfPolicyPlugin_1
    {
  public:
    NplusMPolicy();
    ~NplusMPolicy();
    virtual void activeAudit(ClMgtObject* root);
    virtual void standbyAudit(ClMgtObject* root);
    
    };

  NplusMPolicy::NplusMPolicy()
    {
    }

  NplusMPolicy::~NplusMPolicy()
    {
    }
 
  void NplusMPolicy::activeAudit(ClMgtObject* root)
    {
    logInfo("POL","N+M","Active audit");
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
           
        logInfo("N+M","AUDIT","Auditing service group %s", name.c_str());
        if (1) // TODO: figure out if this Policy should control this Service group
          {
          ClMgtObjectMap::iterator endsu = sg->serviceUnits.end();
          for (itsu = cfg->sg->serviceUnits.begin(); itsu != endsu; itsu++)
            {
            vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) itsu->second;
            int temp = objs->size();
            for(int i = 0; i < temp; i++)
              {
              ServiceUnit* su = dynamic_cast<ServiceUnit*>((*objs)[i]);

              ClMgtObjectMap::iterator endcomp = su->components.end();
              for (itcomp = su->components.begin(); itcomp != endsu; itcomp++)
                {
                vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) itcomp->second;
                int temp = objs->size();
                for(int i = 0; i < temp; i++)
                  {
                  Component* comp = dynamic_cast<Component*>((*objs)[i]);
                  
                  
                  }
                }
          


              }      
            }
          
          }
        
        }
      }
    }
  
  void NplusMPolicy::standbyAudit(ClMgtObject* root)
    {
    logInfo("POL","CUSTOM","Standby audit");
    }
  
  static NplusMPolicy api;
  };

extern "C" ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
  {
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = CL_AMF_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = CL_AMF_POLICY_PLUGIN_VER;

  SAFplus::api.policyId = SAFplus::AmfRedundancyPolicy::NplusM;

  // return it
  return (ClPlugin*) &SAFplus::api;
  }
