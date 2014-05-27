#include <clAmfPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <clProcessApi.hxx>
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

  protected:
    void auditOperation(SAFplusAmf::SAFplusAmfRoot* root);
    void auditDiscovery(SAFplusAmf::SAFplusAmfRoot* root);
    };

  NplusMPolicy::NplusMPolicy()
    {
    }

  NplusMPolicy::~NplusMPolicy()
    {
    }

#if 0 
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
            amfOps->getCompState(comp);
            }
          }
        }

      }

    }
#endif
  void NplusMPolicy::activeAudit(SAFplusAmf::SAFplusAmfRoot* root)
    {
    auditDiscovery(root);
    auditOperation(root);
    }

  // Second step in the audit is to do something to heal any discrepencies.
  void NplusMPolicy::auditOperation(SAFplusAmf::SAFplusAmfRoot* root)
    {
    bool startSg;
    logInfo("POL","N+M","Active audit");
    assert(root);
    SAFplusAmfRoot* cfg = (SAFplusAmfRoot*) root;

    MgtObject::Iterator it;
    for (it = cfg->serviceGroupList.begin();it != cfg->serviceGroupList.end(); it++)
      {
      startSg=false;
      ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
      const std::string& name = sg->name;

      logInfo("N+M","AUDIT","Auditing service group %s", name.c_str());
      if (1) // TODO: figure out if this Policy should control this Service group
        {
        SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
        SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator endsu = sg->serviceUnits.value.end();
        for (itsu = sg->serviceUnits.value.begin(); itsu != endsu; itsu++)
          {
          //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
          ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
          const std::string& suName = su->name;
          logInfo("N+M","AUDIT","Auditing su %s", suName.c_str());

          SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
          SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
          for (itcomp = su->components.value.begin(); itcomp != endcomp; itcomp++)
            {
            Component* comp = dynamic_cast<Component*>(*itcomp);
            SAFplusAmf::AdministrativeState eas = effectiveAdminState(comp);

            if ((comp->operState == false)&&(eas != SAFplusAmf::AdministrativeState::off))
              {
              logError("N+M","AUDIT","Component %s should be on but is not instantiated", comp->name.c_str());
              amfOps->start(comp); // TODO, remove this and call policy specific SG start so the comp can be started based on the policy
              }
            if ((comp->operState)&&(eas == SAFplusAmf::AdministrativeState::off))
              {
              logError("N+M","AUDIT","Component %s should be off but is instantiated", comp->name.c_str());
              }
            }
          }
        }
      //if (startSg) amfOps->start(sg);
      }

    }


  // First step in the audit is to update the current state of every entity to match the reality.
  void NplusMPolicy::auditDiscovery(SAFplusAmf::SAFplusAmfRoot* root)
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
        SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator itsu;
        SAFplus::MgtProvList<SAFplusAmf::ServiceUnit*>::ContainerType::iterator endsu = sg->serviceUnits.value.end();
        for (itsu = sg->serviceUnits.value.begin(); itsu != endsu; itsu++)
          {
          //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
          ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
          const std::string& suName = su->name;
          logInfo("N+M","AUDIT","Auditing su %s", suName.c_str());

          SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
          SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
          for (itcomp = su->components.value.begin(); itcomp != endcomp; itcomp++)
            {
            Component* comp = dynamic_cast<Component*>(*itcomp);
            logInfo("N+M","AUDIT","Auditing component %s", comp->name.c_str());
            CompStatus status = amfOps->getCompState(comp);
            if ((status == CompStatus::Uninstantiated)&&(comp->operState))
              {
              // TODO: Should be changed transactionally
              comp->operState = false;
              comp->readinessState = SAFplusAmf::ReadinessState::outOfService;
              comp->haReadinessState = SAFplusAmf::HighAvailabilityReadinessState::notReadyForAssignment;
              comp->haState = SAFplusAmf::HighAvailabilityState::idle;
              // Notification?
              }
            if ((status == CompStatus::Instantiated)&&(!comp->operState))
              {
              // TODO: Should be changed transactionally
              comp->operState = true;
              comp->readinessState = SAFplusAmf::ReadinessState::inService;
              // Notification?
              }
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
