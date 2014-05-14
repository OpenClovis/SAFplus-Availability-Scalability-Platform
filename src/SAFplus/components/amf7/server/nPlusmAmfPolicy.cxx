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
    virtual void activeAudit(SAFplusAmf::SAFplusAmfRoot* root);
    virtual void standbyAudit(SAFplusAmf::SAFplusAmfRoot* root);
    };

  NplusMPolicy::NplusMPolicy()
    {
    }

  NplusMPolicy::~NplusMPolicy()
    {
    }
 
  void NplusMPolicy::activeAudit(SAFplusAmf::SAFplusAmfRoot* root)
    {
    logInfo("POL","N+M","Active audit");
    assert(root);
    SAFplusAmfRoot* cfg = (SAFplusAmfRoot*) root;

    MgtObject::Iterator it;

    for (it = cfg->serviceGroupList.begin();it != cfg->serviceGroupList.end(); it++)
      {
      ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
      const std::string& name = sg->name;

      logInfo("N+M","AUDIT","Auditing service group %s", name.c_str());
      if (1) // TODO: figure out if this Policy should control this Service group
        {
        MgtObject::Iterator itsu;
        MgtObject::Iterator endsu = sg->serviceUnits.end();
        for (itsu = sg->serviceUnits.begin(); itsu != endsu; itsu++)
          {
         ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
         const std::string& suName = su->name;
          logInfo("N+M","AUDIT","Auditing su %s", suName.c_str());

          MgtObject::Iterator itcomp;
          MgtObject::Iterator endcomp = su->components.end();
          for (itcomp = su->components.begin(); itcomp != endcomp; itcomp++)
            {
            std::string compName = itcomp->first;
            logInfo("N+M","AUDIT","Auditing comp %s ", compName.c_str());
            Component* comp = dynamic_cast<Component*>(itcomp->second);
            logInfo("N+M","AUDIT","Auditing component %s", comp->name.c_str());
            //ops->getCompState(comp);
            }
          }
        }

      }

    }
  
  void NplusMPolicy::standbyAudit(SAFplusAmf::SAFplusAmfRoot* root)
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
