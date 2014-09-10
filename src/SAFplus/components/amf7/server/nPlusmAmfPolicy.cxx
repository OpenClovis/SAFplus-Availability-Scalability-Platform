#include <chrono>
#include <clAmfPolicyPlugin.hxx>
#include <clLogApi.hxx>
#include <clProcessApi.hxx>
#include <clThreadApi.hxx>
#include <clAmfApi.hxx>
#include <vector>
#include <boost/range/algorithm.hpp>

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

  class Poolable: public Wakeable
    {
    public:
    uint_t   executionTimeLimit;
    };

  class ServiceGroupPolicyExecution: public Poolable
    {
    public:
    ServiceGroupPolicyExecution(ServiceGroup* svcgrp,AmfOperations* ops):sg(svcgrp),amfOps(ops) {} 

    ServiceGroup* sg;
    AmfOperations* amfOps;

    virtual void wake(int amt,void* cookie=NULL);

    void start();
    void stop();

    // helper functions
    int start(ServiceUnit* s,Wakeable& w);
    };

  NplusMPolicy::NplusMPolicy()
    {
    }

  NplusMPolicy::~NplusMPolicy()
    {
    }

  void ServiceGroupPolicyExecution::wake(int amt,void* cookie)
    {
    start();
    }

  bool suEarliestRanking(ServiceUnit* a, ServiceUnit* b)
  {
  assert(a);
  assert(b);
  if (b->rank.value == 0) return true;
  if (a->rank.value == 0) return false;
  return (a->rank.value < b->rank.value);
  }

  int ServiceGroupPolicyExecution::start(ServiceUnit* su,Wakeable& w)
    {
    int ret=0;
    SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
    SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
    for (itcomp = su->components.value.begin(); itcomp != endcomp; itcomp++)
      {
      Component* comp = dynamic_cast<Component*>(*itcomp);
      if (comp->processId)
        {
        logDebug("N+M","STRT","Not starting [%s]. Its already started as pid [%d].",comp->name.c_str(),comp->processId.value);
        continue;
        }
      if (comp->operState == false)
        {
        logDebug("N+M","STRT","Not starting [%s]. It must be repaired.",comp->name.c_str(),comp->processId.value);
        continue;
        }
      if (comp->numInstantiationAttempts.value >= comp->maxInstantInstantiations + comp->maxDelayedInstantiations)
        {
        logInfo("N+M","STRT","Faulting [%s]. It has exceeded its startup attempts [%d].",comp->name.c_str(),comp->maxInstantInstantiations + comp->maxDelayedInstantiations);
        comp->operState = false;
        comp->numInstantiationAttempts = 0;
        continue;
        }
      uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
// (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count()/std::chrono::milliseconds(1);
      if ((comp->numInstantiationAttempts.value >= comp->maxInstantInstantiations)&&(curTime < comp->delayBetweenInstantiation.value + comp->lastInstantiation.value.value))
        {
        logDebug("N+M","STRT","Not starting [%s]. Must wait [%lu] more milliseconds.",comp->name.c_str(),comp->delayBetweenInstantiation + comp->lastInstantiation.value.value - curTime);
        continue;
        }

      logInfo("N+M","STRT","Starting component [%s]", comp->name.c_str());
      CompStatus status = amfOps->getCompState(comp);

      SAFplusAmf::AdministrativeState eas = effectiveAdminState(comp);
      assert(eas != SAFplusAmf::AdministrativeState::off); // Do not call this API if the component is administratively OFF!
      amfOps->start(comp,w);
      ret++;
      }
    return ret;
    }

  void ServiceGroupPolicyExecution::start()
    {
      const std::string& name = sg->name;

      logInfo("N+M","STRT","Starting service group %s", name.c_str());
      if (1) // TODO: figure out if this Policy should control this Service group
        {
        std::vector<SAFplusAmf::ServiceUnit*> sus(sg->serviceUnits.value.begin(),sg->serviceUnits.value.end());

        // Sort the SUs by rank so we start them up in the proper order.
        boost::sort(sus,suEarliestRanking);

        int numActive = 0;  // TODO: figure this out, so start can be called from a half-started situation.
        int numStandby = 0; // TODO: figure this out, so start can be called from a half-started situation.
        int numIdle = 0;    // TODO: figure this out, so start can be called from a half-started situation.

        int totalStarted = numActive + numStandby + numIdle;
        int preferredTotal = sg->preferredNumActiveServiceUnits.value + sg->preferredNumStandbyServiceUnits.value + sg->preferredNumIdleServiceUnits.value;

        std::vector<SAFplusAmf::ServiceUnit*>::iterator itsu;
        int waits=0;
        int curRank = -1;
        ThreadSem waitSem;
        for (itsu = sus.begin(); itsu != sus.end(); itsu++)
          {
          if (totalStarted >= preferredTotal) break;  // we have started enough
          ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
          // If we moved up to a new rank, we must first wait for the old rank to come up.  This is part of the definition of SU ranks
          if (su->rank.value != curRank)  
            {
            waitSem.lock(waits);  // This code works in the initial case because waits is 0, so no locking happens
            waits = 0;
            curRank = su->rank.value;
            }
          const std::string& suName = su->name;
          if (su->adminState != AdministrativeState::off)
            {
            logInfo("N+M","STRT","Starting su %s", suName.c_str());
            waits += start(su,waitSem);  // When started, "wake" will be called on the waitSem
            totalStarted++;
            }
          }
        waitSem.lock(waits);  // if waits is 0, lock is no-op
        }
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

  bool running(SAFplusAmf::PresenceState p)
  {
  return (   (p == SAFplusAmf::PresenceState::instantiating)
          || (p == SAFplusAmf::PresenceState::instantiated)
          || (p == SAFplusAmf::PresenceState::terminating)
          || (p == SAFplusAmf::PresenceState::restarting)
          || (p == SAFplusAmf::PresenceState::terminationFailed));
  }

  void NplusMPolicy::activeAudit(SAFplusAmf::SAFplusAmfRoot* root)
    {
    auditDiscovery(root);
    auditOperation(root);
    }

  // Second step in the audit is to do something to heal any discrepencies.
  void NplusMPolicy::auditOperation(SAFplusAmf::SAFplusAmfRoot* root)
    {
    bool startSg;
    logInfo("POL","N+M","Active audit: Operation phase");
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
            if (comp->operState == true) // false means that the component needs repair before we will deal with it.
              {
              if (running(comp->presence.value))
                {
                if (eas == SAFplusAmf::AdministrativeState::off)
                  {
                  logError("N+M","AUDIT","Component %s should be off but is instantiated", comp->name.c_str());
                  }
                else
                  {
                  logDebug("N+M","AUDIT","Component %s process [%s.%d] is [%d]",comp->name.c_str(), su->node.value->name.c_str(), comp->processId.value, (int) comp->presence.value);
                  }
                }
              else
                {
                if (eas != SAFplusAmf::AdministrativeState::off)
                  {
                  logError("N+M","AUDIT","Component %s could be on but is not instantiated", comp->name.c_str());
                  startSg=true;
                  }
                }
              }
            }
          }
        }
      if (startSg)
        {
        ServiceGroupPolicyExecution go(sg,amfOps);
        go.start(); // TODO: this will be put in a thread pool...
        }
      }

    }


  // First step in the audit is to update the current state of every entity to match the reality.
  void NplusMPolicy::auditDiscovery(SAFplusAmf::SAFplusAmfRoot* root)
    {
    logInfo("POL","N+M","Active audit: Discovery phase");
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
            logInfo("N+M","AUDIT","Component [%s]: operState [%s]", comp->name.c_str(), comp->operState.value ? "enabled" : "faulted");
            if (running(comp->presence))  // If I think its running, let's check it out.
              {
              CompStatus status = amfOps->getCompState(comp);
              if (status == CompStatus::Uninstantiated)  // database shows should be running but actually no process is there.  I should update DB.
                {
                comp->presence = PresenceState::uninstantiated;
                comp->processId = 0;
                }
              }

            // SAI-AIS-AMF-B.04.01.pdf sec 3.2.2.3
            if ((comp->operState == false) || (su->readinessState == SAFplusAmf::ReadinessState::outOfService))
              {
              comp->readinessState = SAFplusAmf::ReadinessState::outOfService;
              }
            if ((comp->operState == true) && (su->readinessState == SAFplusAmf::ReadinessState::inService))
              {
              comp->readinessState = SAFplusAmf::ReadinessState::inService;
              }
            if ((comp->operState == true) && (su->readinessState == SAFplusAmf::ReadinessState::stopping))
              {
              comp->readinessState = SAFplusAmf::ReadinessState::stopping;
              }

#if 0  // This was a guess
            if ((status == CompStatus::Uninstantiated)&&(comp->operState))
              {
              // TODO: Should be changed transactionally
              comp->readinessState = SAFplusAmf::ReadinessState::outOfService;
              // 3.2.2.5 if comp sets this, AMF must remove assignment or change to standby comp->haReadinessState = SAFplusAmf::HighAvailabilityReadinessState::notReadyForAssignment;
              comp->haState = SAFplusAmf::HighAvailabilityState::idle;
              // Notification?
              }
            if ((status == CompStatus::Instantiated)&&(!comp->operState))
              {
              // TODO: Should be changed transactionally
              comp->readinessState = SAFplusAmf::ReadinessState::inService;
              // Notification?
              }
#endif
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
