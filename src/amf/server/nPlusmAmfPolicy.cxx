#include <chrono>
#include <clAmfPolicyPlugin.hxx>
#include <clLogIpi.hxx>
#include <clProcessApi.hxx>
#include <clThreadApi.hxx>
#include <clNameApi.hxx>
#include <clAmfApi.hxx>
#include <vector>
#include <boost/range/algorithm.hpp>

#include <SAFplusAmfModule.hxx>
#include <ServiceGroup.hxx>

typedef boost::unordered_map<SAFplusAmf::Node*, int> UninstantiatedCountMap;

using namespace std;
using namespace SAFplus;
using namespace SAFplusAmf;


namespace SAFplus
  {

  SAFplusAmf::Recovery recommendedRecovery = SAFplusAmf::Recovery::None;
  const char* oper_str(bool val) { if (val) return "enabled"; else return "disabled"; }
  bool compareEntityRecoveryScope(Recovery a, Recovery b);
  void updateStateDueToProcessDeath(SAFplusAmf::Component* comp);
  //ClRcT sgAdjust(const SAFplusAmf::ServiceGroup* sg);
  //ClRcT nodeErrorReport(SAFplusAmf::Node* node, bool shutdownAmf = false, bool rebootNode = false);

class NplusMPolicy:public ClAmfPolicyPlugin_1
    {
  public:
    NplusMPolicy();
    ~NplusMPolicy();
    virtual void activeAudit(SAFplusAmf::SAFplusAmfModule* root);
    virtual void standbyAudit(SAFplusAmf::SAFplusAmfModule* root);
    virtual void compFaultReport(Component* comp, const SAFplusAmf::Recovery recommRecovery = SAFplusAmf::Recovery::NoRecommendation);
    //SAFplusAmf::Recovery recommendedRecovery;
    Component* processedComp;

  protected:
    void auditOperation(SAFplusAmf::SAFplusAmfModule* root);
    void auditDiscovery(SAFplusAmf::SAFplusAmfModule* root);
    void computeCompRecoveryAction(Component* comp, Recovery* recovery, bool* escalation);
    void computeSURecoveryAction(ServiceUnit* su, Component* faultyComp, Recovery* recovery, bool* escalation);
    void postProcessForSURecoveryAction(ServiceUnit* su, Component* faultyComp, Recovery* recovery, bool* escalation);
    void computeNodeRecoveryAction(Node* node, Recovery* recovery, bool* escalation);
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
        //recommendedRecovery=SAFplusAmf::Recovery::None;
        processedComp = NULL;
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

  bool siEarliestRanking(ServiceInstance* a, ServiceInstance* b)
  {
  assert(a);
  assert(b);
  if (b->rank.value == 0) return true;
  if (a->rank.value == 0) return false;
  return (a->rank.value < b->rank.value);
  }

  bool EarliestLevel(Component* a, Component* b)
  {
	  assert(a);
	  assert(b);
	  if (b->instantiateLevel.value == 0) return true;
	  if(a->instantiateLevel.value == 0) return false;
	  return (a->instantiateLevel.value < b->instantiateLevel.value);
  }

  int ServiceGroupPolicyExecution::start(ServiceUnit* su,Wakeable& w)
    {
    int ret=0;
    std::vector<SAFplusAmf::Component*> sortedComps;
    std::vector<SAFplusAmf::Component*>::iterator itcomp, endcomp;

    sortedComps << su->components;
    boost::sort(sortedComps, EarliestLevel);
    endcomp = sortedComps.end();
    for (itcomp = sortedComps.begin(); itcomp != endcomp; itcomp++)
      {
      Component* comp = *itcomp;
      if (!comp->serviceUnit.value)
        {
          comp->serviceUnit.updateReference(); // find the pointer if it is not resolved         
        }
      if (comp->serviceUnit.value != su)
        {
          logAlert("N+M","STRT","Model inconsistency.  Service Unit [%s] contains component [%s], but component does not refer to the Service Unit.  Please fix the model, using service unit's data for now.",su->name.value.c_str(),comp->name.value.c_str());
          comp->serviceUnit.value = su;
        }

      if (comp->processId)
        {
        logDebug("N+M","STRT","Not starting [%s]. Its already started as pid [%d].",comp->name.value.c_str(),comp->processId.value);
        continue;
        }
      else if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable &&
               comp->presenceState.value == PresenceState::instantiated)
      {
         logDebug("N+M","STRT","Not starting [%s]. Its a proxied preinstantiable component already started",comp->name.value.c_str());
         continue;
      }
      if (comp->operState == false)
        {
        logDebug("N+M","STRT","Not starting [%s]. It must be repaired.",comp->name.value.c_str());
        continue;
        }
      if (comp->capabilityModel == CapabilityModel::not_preinstantiable)
        {
        logDebug("N+M","STRT","Not starting [%s]. Its not preinstantiable.",comp->name.value.c_str());
        continue;
        }

      if (comp->numInstantiationAttempts.value >= comp->maxInstantInstantiations + comp->maxDelayedInstantiations)
        {
          logInfo("N+M","STRT","Faulting [%s]. Number of instantiation Attempts [%d] has exceeded its configured maximum [%d].",comp->name.value.c_str(),comp->numInstantiationAttempts.value, comp->maxInstantInstantiations + comp->maxDelayedInstantiations);
        comp->operState = false;
        comp->numInstantiationAttempts = 0;
        continue;
        }
      uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
// (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count()/std::chrono::milliseconds(1);
      if ((comp->numInstantiationAttempts.value >= comp->maxInstantInstantiations)&&(curTime < comp->delayBetweenInstantiation.value + comp->lastInstantiation.value.value))
        {
        logDebug("N+M","STRT","Not starting [%s]. Must wait [%" PRIu64 "] more milliseconds.",comp->name.value.c_str(),comp->delayBetweenInstantiation + comp->lastInstantiation.value.value - curTime);
        continue;
        }
	int minInstantiateLevel = comp->instantiateLevel;
      logInfo("N+M","STRT","Starting component [%s], with instantiateLevel = %d", comp->name.value.c_str(), minInstantiateLevel);
      try
        {
        CompStatus status = amfOps->getCompState(comp);

        SAFplusAmf::AdministrativeState eas = effectiveAdminState(comp);
        assert(eas != SAFplusAmf::AdministrativeState::off); // Do not call this API if the component is administratively OFF!
        amfOps->start(comp,w);        
        if (comp->compProperty.value == SAFplusAmf::CompProperty::sa_aware)
          ret++;
        }
      catch (Error& e)  // Can't talk to the node and the fault manager entry does not exist.
        {
          // TODO: cleanup data about this node
        }
      }
    return ret;
    }

  void ServiceGroupPolicyExecution::start()
    {
      const std::string& name = sg->name;

      logInfo("N+M","STRT","Starting service group [%s]", name.c_str());
      if (1) // TODO: figure out if this Policy should control this Service group
        {
        std::vector<SAFplusAmf::ServiceUnit*> sus;//(sg->serviceUnits.value.begin(),sg->serviceUnits.value.end());
        sus << sg->serviceUnits;


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
          if (totalStarted >= preferredTotal)
            {
              logInfo("N+M","STRT","Already started [%d] out of [%d] service units in service group [%s], ", totalStarted, preferredTotal, name.c_str());
            break;  // we have started enough
            }
          ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
          // If we moved up to a new rank, we must first wait for the old rank to come up.  This is part of the definition of SU ranks
          if (su->rank.value != curRank)  
            {
            waitSem.lock(waits);  // This code works in the initial case because waits is 0, so no locking happens
            waits = 0;
            curRank = su->rank.value;
            }
          const std::string& suName = su->name;
          if (su->node.value->presenceState == SAFplusAmf::PresenceState::instantiated)
            {
              if (su->adminState != AdministrativeState::off)
                { // if su->presenceState != ...
                  logInfo("N+M","STRT","Starting service unit [%s]", suName.c_str());
                  waits += start(su,waitSem);  // When started, "wake" will be called on the waitSem
                  totalStarted++;
                }
              else
                {
                  logInfo("N+M","STRT","Not starting service unit [%s] admin state is [%s]", suName.c_str(),c_str(su->adminState.value));
                }
            }
          else
            {
              logInfo("N+M","STRT","Not starting service unit [%s] node [%s] is not instantiated", suName.c_str(),su->node.value->name.value.c_str());
            }
          }
        waitSem.lock(waits);  // if waits is 0, lock is no-op
        }
    }


  bool running(SAFplusAmf::PresenceState p)
  {
  return (   (p == SAFplusAmf::PresenceState::instantiating)
          || (p == SAFplusAmf::PresenceState::instantiated)
          || (p == SAFplusAmf::PresenceState::terminating)
          || (p == SAFplusAmf::PresenceState::restarting)
          || (p == SAFplusAmf::PresenceState::terminationFailed));
  }

  void NplusMPolicy::activeAudit(SAFplusAmf::SAFplusAmfModule* root)
    {
    auditDiscovery(root);
    auditOperation(root);
    }

  ServiceUnit* findAssignableServiceUnit(std::vector<SAFplusAmf::ServiceUnit*>& candidates,SAFplusAmf::ServiceInstance* si, HighAvailabilityState tgtState)
    {
    std::vector<SAFplusAmf::ServiceUnit*>::iterator i;
    for (i = candidates.begin(); i != candidates.end(); i++)
      {
      bool assignable = true;
      ServiceUnit* su = *i;
      assert(su);
      if (su->presenceState.value != SAFplusAmf::PresenceState::instantiated) continue;
      // We can only assign a particular SI to a particular SU once, and it can't be in "repair needed" state
      if ((su->assignedServiceInstances.contains(si) == false ||
           (tgtState == HighAvailabilityState::active && !si->isFullActiveAssignment.value && !si->standbyAssignments.contains(su)) ||
           (tgtState == HighAvailabilityState::standby && !si->isFullStandbyAssignment.value && !si->activeAssignments.contains(su))) &&
           (su->operState == true))
        {
          // TODO: add a text field in the SU that describes why it is not assignable... generate a string from the component iterator and fill that field.

          //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
        //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
        for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
          {
          Component* comp = dynamic_cast<Component*>(*itcomp);
          assert(comp);
          if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable)
            {
               //Don't count proxied preinstantiable because it never gets assignment
               continue;
            }          
          if (comp->operState.value == false) {assignable = false; break; }
          if (comp->capabilityModel == CapabilityModel::not_preinstantiable)  // If the component is not preinstantiable, there are basically no requirements on it to be assignable -- it doesn't even have to be running.
            {
            }
          else if ((comp->presenceState.value == PresenceState::instantiated) && (comp->readinessState.value == ReadinessState::inService) && (comp->haReadinessState == HighAvailabilityReadinessState::readyForAssignment) && (comp->haState != HighAvailabilityState::quiescing) && (comp->pendingOperation == PendingOperation::none))
            {
              // Now check the component's capability model
              if (comp->capabilityModel == CapabilityModel::x_active_or_y_standby)  // Can't take both active and standby assignments
                {
                  if ((tgtState == HighAvailabilityState::active) && (comp->haState == HighAvailabilityState::standby)) assignable = false;
                  if ((tgtState == HighAvailabilityState::standby) && (comp->haState == HighAvailabilityState::active)) assignable = false;
                }

              // Check # of assignments against the maximum.
              if ((tgtState == HighAvailabilityState::active) && (comp->activeAssignments.current.value >= comp->maxActiveAssignments)) assignable = false;
              if ((tgtState == HighAvailabilityState::standby) && (comp->standbyAssignments.current >= comp->maxStandbyAssignments )) assignable = false;

            // candidate
            }
          else
            {
            assignable = false;
            break;
            }
          }
        if (assignable) return su;
        }
      }
    return nullptr;
    }

  ServiceUnit* findPromotableServiceUnit(std::vector<SAFplusAmf::ServiceUnit*>& candidates,MgtList<std::string>& weights, HighAvailabilityState tgtState)
    {
    std::vector<SAFplusAmf::ServiceUnit*>::iterator i;
    for (i = candidates.begin(); i != candidates.end(); i++)
      {
      bool assignable = true;
      ServiceUnit* su = *i;
      assert(su);
      if (su->operState == true)
        {
          // TODO: add a text field in the SU that describes why it is not assignable... generate a string from the component iterator and fill that field.

        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
        //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
        //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
        for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
          {
          Component* comp = dynamic_cast<Component*>(*itcomp);
          assert(comp);
          if ((comp->operState.value == true) && (comp->readinessState.value == ReadinessState::inService) && (comp->haReadinessState == HighAvailabilityReadinessState::readyForAssignment) && (comp->haState != HighAvailabilityState::quiescing) && (comp->pendingOperation == PendingOperation::none))
            {
              // Check # of assignments against the maximum.
              /*if ((tgtState == HighAvailabilityState::active) && (comp->activeAssignments.current.value >= comp->maxActiveAssignments)) assignable = false;
              if ((tgtState == HighAvailabilityState::standby) && (comp->standbyAssignments.current >= comp->maxStandbyAssignments )) assignable = false;
              */
              if (tgtState == HighAvailabilityState::active)
              {
                  if(comp->activeAssignments.current.value >= comp->maxActiveAssignments) assignable = false;
                  else
                  {
                      assignable = true;
                      break;
                  }
              }
              if (tgtState == HighAvailabilityState::standby)
              {
                  if(comp->standbyAssignments.current >= comp->maxStandbyAssignments ) assignable = false;
                  else
                  {
                      assignable = true;
                      break;
                  }
              }
            // candidate
            }
          else 
            {
            assignable = false;
            break;
            }
          }
        if (assignable) return su;
        }
      }
    return nullptr;
    }



  // Second step in the audit is to do something to heal any discrepencies.
  void NplusMPolicy::auditOperation(SAFplusAmf::SAFplusAmfModule* root)
  {
      bool startSg;
      logInfo("POL","N+M","Active audit: Operation phase");
      assert(root);
      SAFplusAmfModule* cfg = (SAFplusAmfModule*) root;
      MgtObject::Iterator it;
      for (it = cfg->safplusAmf.serviceGroupList.begin();it != cfg->safplusAmf.serviceGroupList.end(); it++)
      {
          startSg=false;
          ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
          const std::string& name = sg->name;

          // TODO: figure out if this Policy should control this Service group

          logInfo("N+M","AUDIT","Auditing service group [%s]", name.c_str());

          // Look for Service Units that need to be started up
          if (1)
          {

              SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu;
              //SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator endsu = sg->serviceUnits.listEnd();
              for (itsu = sg->serviceUnits.listBegin(); itsu != sg->serviceUnits.listEnd(); itsu++)
              {
                  //ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
                  ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
                  assert(su);
                  const std::string& suName = su->name;

                  Node* node = su->node;
                  bool nodeExists  = false;
                  if (node)
                  {
                      nodeExists = (node->presenceState == PresenceState::instantiated);  //(node->stats.upTime > 0); // TODO: use the presence state
                  }

                  uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                  if(!(su->compRestartCount==0) && (curTime - su->lastCompRestart.value.value >= sg->componentRestart.getDuration()))
                  {
                      su->compRestartCount=0;
                  }
                  if(!(su->restartCount==0) && (curTime - su->lastRestart.value.value >= sg->serviceUnitRestart.getDuration()))
                  {
                      su->restartCount=0;
                  }
                  if(!(node->serviceUnitFailureEscalationPolicy.failureCount==0) &&
                          (curTime - node->lastSUFailure.value.value >= node->serviceUnitFailureEscalationPolicy.getDuration()))
                  {
                      node->serviceUnitFailureEscalationPolicy.failureCount=0;
                  }

                  logInfo("N+M","AUDIT","Auditing service unit [%s] on [%s (%s)]: Operational State [%s] AdminState [%s] PresenceState [%s] ReadinessState [%s] HA State [%s] HA Readiness [%s] ", suName.c_str(),node ? node->name.value.c_str() : "unattached", node ? c_str(node->presenceState.value): "N/A", oper_str(su->operState.value), c_str(su->adminState.value), c_str(su->presenceState.value), c_str(su->readinessState.value), c_str(su->haState.value), c_str(su->haReadinessState.value) );

                  SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp;
                  SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endcomp = su->components.listEnd();
                  //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   itcomp;
                  //SAFplus::MgtProvList<SAFplusAmf::Component*>::ContainerType::iterator   endcomp = su->components.value.end();
                  for (itcomp = su->components.listBegin(); itcomp != endcomp; itcomp++)
                  {
                      Component* comp = dynamic_cast<Component*>(*itcomp);
                      logInfo("N+M","AUDIT","Auditing component [%s], compProperty [%s] on [%s.%s] pid [%d]: Operational State [%s] PresenceState [%s] ReadinessState [%s] HA State [%s] HA Readiness [%s] Pending Operation [%s] (expires in: [%d ms]) instantiation attempts [%d]",comp->name.value.c_str(),c_str(comp->compProperty.value),node ? node->name.value.c_str(): "unattached",suName.c_str(), comp->processId.value,
                              oper_str(comp->operState.value), c_str(comp->presenceState.value), c_str(comp->readinessState.value),
                              c_str(comp->haState.value), c_str(comp->haReadinessState.value), c_str(comp->pendingOperation.value), ((comp->pendingOperationExpiration.value.value>0) ? (int) (comp->pendingOperationExpiration.value.value - curTime): 0), comp->numInstantiationAttempts.value);
                      if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable &&
                              amfOps->suContainsSaAwareComp(su))
                      {
                          //Don't count proxied preinstantiable because it never gets assignment according to safplus 6.0
                          logInfo("N+M","AUDIT","Don't count proxied preinstantiable for component [%s] because it must be instantiated after its proxy gets assignment", comp->name.value.c_str());
                          continue;
                      }
                      SAFplusAmf::AdministrativeState eas = effectiveAdminState(comp);
                      /*logInfo("N+M","AUDIT","Auditing component [%s] on [%s.%s] pid [%d]: Operational State [%s] PresenceState [%s] ReadinessState [%s] HA State [%s] HA Readiness [%s] Pending Operation [%s] (expires in: [%d ms]) instantiation attempts [%d]",comp->name.value.c_str(),node ? node->name.value.c_str(): "unattached",suName.c_str(), comp->processId.value,
            oper_str(comp->operState.value), c_str(comp->presenceState.value), c_str(comp->readinessState.value),
            c_str(comp->haState.value), c_str(comp->haReadinessState.value), c_str(comp->pendingOperation.value), ((comp->pendingOperationExpiration.value.value>0) ? (int) (comp->pendingOperationExpiration.value.value - curTime): 0), comp->numInstantiationAttempts.value);*/
                      if (comp->operState == true) // false means that the component needs repair before we will deal with it.
                      {
                          if (running(comp->presenceState.value))
                          {
                              if (eas == SAFplusAmf::AdministrativeState::off)
                              {
                                  logError("N+M","AUDIT","Component [%s] should be off but is instantiated", comp->name.value.c_str());
                                  amfOps->stop(comp);
                              }
                              else
                              {
                                  char timeString[80];
                                  //struct tm * timeptr;
                                  time_t rawtime = comp->lastInstantiation.value.value / 1000;  // /1000 converts ms to sec.
                                  //timeinfo = localtime(&rawtime);
                                  strftime(timeString,80,"%c",localtime(&rawtime));
                                  logDebug("N+M","AUDIT","Component [%s] process [%s.%d] is [%s].  Instantiated since [%s].",comp->name.value.c_str(), su->node.value->name.value.c_str(), comp->processId.value, c_str(comp->presenceState.value),timeString);

                                  if (comp->presenceState == PresenceState::instantiating)
                                  {
                                      uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                      if (curTime - comp->lastInstantiation.value.value >= comp->getInstantiate()->timeout.value)
                                      {
                                          logError("N+M","AUDIT","Component [%s] process [%s:%d] never registered with AMF after instantiation.  Killing it.", comp->name.value.c_str(), node->name.value.c_str(),comp->processId.value);
                                          // Process is not responding after instantiation.  Kill it.  When we discover it is dead, we will update the database and the next step (restart) will happen.
                                          amfOps->abort(comp);
                                      }
                                  }
                              }
                          }
                          else
                          {
                              if (nodeExists)  // Make sure that components are running if they can be.
                              {
                                  if (comp->presenceState == PresenceState::instantiating)
                                  {
                                      uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                      if (curTime - comp->lastInstantiation.value.value >= comp->getInstantiate()->timeout.value)
                                      {
                                          logError("N+M","AUDIT","Component [%s] process [%s:%d] never registered with AMF after instantiation.  Killing it.", comp->name.value.c_str(), node->name.value.c_str(),comp->processId.value);
                                          // Process is not responding after instantiation.  Kill it.  When we discover it is dead, we will update the database and the next step (restart) will happen.
                                          amfOps->abort(comp);
                                      }
                                  }
                                  else
                                  {
                                      if (comp->capabilityModel == CapabilityModel::not_preinstantiable)
                                      {
                                          // TODO: in this case I need to look at the work to determine if this component should be instantiated but is not
                                          //startSg=true;
                                      }
                                      else if (eas != SAFplusAmf::AdministrativeState::off)
                                      {
                                          if((recommendedRecovery==SAFplusAmf::Recovery::NodeSwitchover
                                              || recommendedRecovery==SAFplusAmf::Recovery::NodeFailover
                                              || recommendedRecovery==SAFplusAmf::Recovery::NodeFailfast) && processedComp)
                                          {
                                              startSg=false;
                                          }
                                          else
                                          {
                                              logError("N+M","AUDIT","Component [%s] could be on but is not instantiated", comp->name.value.c_str());
                                              startSg = true;
                                          }
                                      }
                                  }
                              }
                              else
                              {
                                  // TODO: if the node does not exist, make sure that the component's state is properly set
                              }
                          }
                      }
                  }
              }
          }


          // Look for Service Instances that need assignment
          if (1)
          {
              // TODO: pick the SU sort algorithm based on SG policy
              bool (*suOrder)(ServiceUnit* a, ServiceUnit* b) = suEarliestRanking;

              // Sort the SUs by rank so we assign them in proper order.
              // TODO: I don't like this constant resorting... we should have a sorted list in the SG.
              std::vector<SAFplusAmf::ServiceUnit*> sus;
              sus << sg->serviceUnits; // (sg->serviceUnits.begin(),sg->serviceUnits.end());
              if (sg->autoAdjust) // SU active assignment based on its rank is only applied when the flag autoAdjust is on
              {
                  boost::sort(sus,suOrder);
              }
              // Go through in rank order so that the most important SIs are assigned first
              std::vector<SAFplusAmf::ServiceInstance*> sis; //(sg->serviceInstances.listBegin(),sg->serviceInstances.listEnd());
              sis << sg->serviceInstances;
              boost::sort(sis,siEarliestRanking);

              std::vector<SAFplusAmf::ServiceInstance*>::iterator itsi;
              for (itsi = sis.begin(); itsi != sis.end(); itsi++)
              {
                  ServiceInstance* si = dynamic_cast<ServiceInstance*>(*itsi);
                  if (!si) continue;

                  SAFplusAmf::AdministrativeState eas = effectiveAdminState(si);

                  if (eas == AdministrativeState::on)
                  {
                      // Handle promotion of standby to active
                      if ( (recommendedRecovery!= SAFplusAmf::Recovery::CompRestart && recommendedRecovery!= SAFplusAmf::Recovery::SuRestart) &&
                           (si->numActiveAssignments.current < si->preferredActiveAssignments) && (si->numStandbyAssignments.current > 0))
                      {
                          // Sort the SUs by rank so we promote them in proper order.
                          // TODO: I don't like this constant resorting... we should have a sorted list in the SG.
                          logInfo("N+M","AUDIT","Attempting to promote standby service instance [%s] assignment to active",si->name.value.c_str());
                          std::vector<SAFplusAmf::ServiceUnit*> standbySus; //(si->standbyAssignments.value.begin(),si->standbyAssignments.value.end());
                          standbySus << si->standbyAssignments;
                          boost::sort(standbySus,suOrder);
                          ServiceUnit* su = findPromotableServiceUnit(standbySus,si->activeWeightList,HighAvailabilityState::active);
                          if (su)
                          {
                              if(si->standbyAssignments.contains(su))
                              {
                                  si->standbyAssignments.erase(su);
                                  si->getNumStandbyAssignments()->current.value = si->standbyAssignments.value.size();
                              }
                              if(su->assignedServiceInstances.contains(si))
                              {
                                  su->assignedServiceInstances.erase(si);
                              }
                              // TODO: remove other SI standby assignments from this SU if the component capability model is OR
                              if(!si->activeAssignments.contains(su))
                              {
                                  si->activeAssignments.value.push_back(su);  // assign the si to the su
                                  si->getNumActiveAssignments()->current.value = si->activeAssignments.value.size();
                              }

                              SAFplus::MgtObject::Iterator itcsi;
                              SAFplus::MgtObject::Iterator endcsi = si->componentServiceInstances.end();
                              for (itcsi = si->componentServiceInstances.begin(); itcsi != endcsi; itcsi++)
                              {
                                  SAFplusAmf::ComponentServiceInstance *csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itcsi->second);
                                  //csi->standbyComponents.value.clear();
                                  // remove the old active comp only from its CSI
                                  if (csi->standbyComponents.contains(processedComp))
                                  {
                                      logDebug("N+M","AUDIT","Erase faulty component [%s] from standbyComponents of csi [%s]", processedComp->name.value.c_str(),csi->name.value.c_str());
                                      csi->standbyComponents.erase(processedComp);
                                      break;
                                  }
                              }

                              amfOps->assignWork(su,si,HighAvailabilityState::active);
                          }
                      }
                  }

                  // We want to assign but for some reason it is not.
                  if ((eas == AdministrativeState::on) && (si->assignmentState != AssignmentState::fullyAssigned))
                  {
                      logInfo("N+M","AUDIT","Service Instance [%s] should be fully assigned but is [%s]. Current active assignments [%d], targeting [%d]", si->name.value.c_str(),c_str(si->assignmentState),(int) si->getNumActiveAssignments()->current.value,(int) si->preferredActiveAssignments.value);

                      //if (1)
                      if (recommendedRecovery != SAFplusAmf::Recovery::NodeFailfast)
                      {
                          for (int cnt = si->getNumActiveAssignments()->current.value; cnt < si->preferredActiveAssignments /*|| !si->isFullActiveAssignment.value*/; cnt++)
                          {
                              ServiceUnit* su = findAssignableServiceUnit(sus,si,HighAvailabilityState::active);
                              if (su)
                              {
                                  // TODO: assert(si is not already assigned to this su)
                                  /*si->activeAssignments.value.push_back(su);  // assign the si to the su
                  si->getNumActiveAssignments()->current.value++;
                  */
                                  if(!si->activeAssignments.contains(su))
                                  {
                                      si->activeAssignments.value.push_back(su);
                                      si->getNumActiveAssignments()->current.value = si->activeAssignments.value.size();
                                  }
                                  amfOps->assignWork(su,si,HighAvailabilityState::active);
                                  boost::sort(sus,suOrder);  // Sort order may have changed based on the assignment.
                              }
                              else
                              {
                                  logInfo("N+M","AUDIT","Service Instance [%s] cannot be assigned %dth active.  No available service units.", si->name.value.c_str(),cnt);
                                  break;
                              }
                          }
                      }
                      if (si->getNumActiveAssignments()->current.value > 0 && si->isFullActiveAssignment)  // If there is at least 1 active, try to assign the standbys
                      {
                          for (int cnt = si->getNumStandbyAssignments()->current.value; cnt < si->preferredStandbyAssignments /*|| (!si->isFullStandbyAssignment.value )*/; cnt++)
                          {
                              ServiceUnit* su = findAssignableServiceUnit(sus,si,HighAvailabilityState::standby);

                              if((recommendedRecovery==SAFplusAmf::Recovery::NodeSwitchover
                                  || recommendedRecovery==SAFplusAmf::Recovery::NodeFailover
                                  || recommendedRecovery==SAFplusAmf::Recovery::NodeFailfast)
                                      && processedComp && su && processedComp->serviceUnit.value->name.value.compare(su->name.value) == 0 )
                              {
                                  continue;
                              }
                              if (su)
                              {
                                  // TODO: assert(si is not already assigned to this su)
                                  /*si->standbyAssignments.value.push_back(su);  // assign the si to the su
                  si->getNumStandbyAssignments()->current.value++;
                  */
                                  if(!si->standbyAssignments.contains(su))
                                  {
                                      si->standbyAssignments.value.push_back(su);
                                      si->getNumStandbyAssignments()->current.value = si->standbyAssignments.value.size();
                                  }
                                  amfOps->assignWork(su,si,HighAvailabilityState::standby);
                                  boost::sort(sus,suOrder);  // Sort order may have changed based on the assignment.
                              }
                              else
                              {
                                  logInfo("N+M","AUDIT","Service Instance [%s] cannot be assigned %dth standby.  No available service units.", si->name.value.c_str(),cnt);
                                  break;
                              }
                          }
                      }
                  }

                  else if ((eas == AdministrativeState::off || eas == AdministrativeState::idle) && (si->assignmentState != AssignmentState::unassigned))
                  {
                      logInfo("N+M","AUDIT","Service Instance [%s] should be unassigned but is [%s].", si->name.value.c_str(),c_str(si->assignmentState));
                      amfOps->removeWork(si);
                  }
                  else
                  {
                      logInfo("N+M","AUDIT","Service Instance [%s]: admin state [%s]. assignment state [%s].  Assignments: active [%d] standby [%d]. ", si->name.value.c_str(),c_str(si->adminState.value), c_str(si->assignmentState),(int)si->getNumActiveAssignments()->current.value,(int) si->getNumStandbyAssignments()->current.value  );
                  }

              }
          }

#if 0
          // Look for Components that need assignment
          if (1)
          {
              // Go through in rank order so that the most important SIs are assigned first
              std::vector<SAFplusAmf::ServiceInstance*> sis(sg->serviceInstances.value.begin(),sg->serviceInstances.value.end());
              boost::sort(sis,siEarliestRanking);

              std::vector<SAFplusAmf::ServiceInstance*>::iterator itsi;
              for (itsi = sis.begin(); itsi != sis.end(); itsi++)
              {
                  ServiceInstance* si = dynamic_cast<ServiceInstance*>(*itsi);
                  SAFplusAmf::AdministrativeState eas = effectiveAdminState(si);

                  if (eas == AdministrativeState::on)
                  {
                      //logInfo("N+M","AUDIT","Service Instance [%s] should be fully assigned but is [%s]. Current active assignments [%ld], targeting [%d]", si->name.value.c_str(),c_str(si->assignmentState),si->getNumActiveAssignments()->current.value, si->preferredActiveAssignments.value);

                      if (1)
                      {
                          for (int cnt = si->getNumActiveAssignments()->current.value; cnt < si->preferredActiveAssignments; cnt++)
                          {
                              ServiceUnit* su = findAssignableServiceUnit(sus,si,HighAvailabilityState::active);
                              if (su)
                              {
                                  // TODO: assert(si is not already assigned to this su)
                                  si->activeAssignments.value.push_back(su);  // assign the si to the su
                                  si->getNumActiveAssignments()->current.value++;
                                  amfOps->assignWork(su,si,HighAvailabilityState::active);
                                  boost::sort(sus,suOrder);  // Sort order may have changed based on the assignment.
                              }
                              else
                              {
                                  logInfo("N+M","AUDIT","Service Instance [%s] cannot be assigned %dth active.  No available service units.", si->name.value.c_str(),cnt);
                                  break;
                              }
                          }
                      }
                      if (si->getNumActiveAssignments()->current.value > 0)  // If there is at least 1 active, try to assign the standbys
                      {
                          for (int cnt = si->getNumStandbyAssignments()->current.value; cnt < si->preferredStandbyAssignments; cnt++)
                          {
                              ServiceUnit* su = findAssignableServiceUnit(sus,si->standbyWeightList,HighAvailabilityState::standby);
                              if (su)
                              {
                                  // TODO: assert(si is not already assigned to this su)
                                  si->standbyAssignments.value.push_back(su);  // assign the si to the su
                                  si->getNumStandbyAssignments()->current.value++;

                                  amfOps->assignWork(su,si,HighAvailabilityState::standby);
                                  boost::sort(sus,suOrder);  // Sort order may have changed based on the assignment.
                              }
                              else
                              {
                                  logInfo("N+M","AUDIT","Service Instance [%s] cannot be assigned %dth standby.  No available service units.", si->name.value.c_str(),cnt);
                                  break;
                              }
                          }
                      }
                  }

                  else if ((eas == AdministrativeState::off) && (si->assignmentState != AssignmentState::unassigned))
                  {
                      logInfo("N+M","AUDIT","Service Instance [%s] should be unassigned but is [%s].", si->name.value.c_str(),c_str(si->assignmentState));
                      amfOps->removeWork(si);
                  }
                  else
                  {
                      logInfo("N+M","AUDIT","Service Instance [%s]: admin state [%s]. assignment state [%s].  Assignments: active [%ld] standby [%ld]. ", si->name.value.c_str(),c_str(si->adminState.value), c_str(si->assignmentState), si->getNumActiveAssignments()->current.value,si->getNumStandbyAssignments()->current.value  );
                  }

              }
          }
#endif

          if(startSg)
          {
              ServiceGroupPolicyExecution go(sg,amfOps);
              go.start(); // TODO: this will be put in a thread pool...
          }
          else
          {
              if((recommendedRecovery==SAFplusAmf::Recovery::NodeSwitchover
                  ||recommendedRecovery==SAFplusAmf::Recovery::NodeFailover
                  ||recommendedRecovery==SAFplusAmf::Recovery::NodeFailfast)&& processedComp)
              {
                  amfOps->rebootNode(processedComp->serviceUnit.value->node.value);
                  processedComp =NULL;
              }
          }
      }
  }
  
#if 0
  bool isThereStandbySU(const std::vector<ServiceUnit*>& suList)
  {
      std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
      for (; itsu != suList.end(); ++itsu)
      {
         SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
         if (su->numStandbyServiceInstances.current.value > 0)
             return true;
      }
      return false;
  }

  uint32_t getActiveHighestRankSU(const std::vector<ServiceUnit*>& suList)
    {
        std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
        for (; itsu != suList.end(); ++itsu)
        {
           SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
           if (su->numActiveServiceInstances.current.value > 0)
               return su->rank.value;
        }
        return 0;
    }

  uint32_t getStandbyHighestRankSU(const std::vector<ServiceUnit*>& suList)
      {
          std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
          for (; itsu != suList.end(); ++itsu)
          {
             SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
             if (su->numStandbyServiceInstances.current.value > 0)
                 return su->rank.value;
          }
          return 0;
      }

  int getNumAssignedSUs(const std::vector<ServiceUnit*>& suList)
        {
            std::vector<SAFplusAmf::ServiceUnit*>::const_iterator itsu = suList.begin();
            int count = 0;
            for (; itsu != suList.end(); ++itsu)
            {
               SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
               if (su->numStandbyServiceInstances.current.value > 0 || su->numActiveServiceInstances.current.value > 0)
                   count++;
            }
            return count;
        }

  ClRcT sgAdjust(const SAFplusAmf::ServiceGroup* sg)
  {
      logDebug("N+M","ADJUST","adjusting sg [%s]", sg->name.value.c_str());
      ClRcT rc = CL_OK;
      bool (*suOrder)(ServiceUnit* a, ServiceUnit* b) = suEarliestRanking;
      std::vector<SAFplusAmf::ServiceUnit*> sus;
      sus << sg->serviceUnits;
      if (!isThereStandbySU(sus) || getNumAssignedSUs(sus) == 1) // there are only active assignments or there is only one su in the list, so nothing to do
      {
         logDebug("N+M","ADJUST","sg [%s] doesn't need to be adjusted because there is no any standby SU or there is only one assigned SU", sg->name.value.c_str());
         rc = CL_ERR_NO_OP;
         return rc;
      }
      boost::sort(sus, suOrder);
      uint32_t activeHighestRankSU = getActiveHighestRankSU(sus);
      uint32_t standbyHighestRankSU = getStandbyHighestRankSU(sus);      
      if ((activeHighestRankSU && activeHighestRankSU > standbyHighestRankSU) || (activeHighestRankSU == 0 && standbyHighestRankSU > 0))
      {
          logDebug("N+M","ADJUST","SU list of sg [%s] need to be adjusted", sg->name.value.c_str());
          std::vector<SAFplusAmf::ServiceUnit*>::iterator itsu = sus.begin();
          for (; itsu != sus.end(); ++itsu)
          {
             SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
             // go one by one su to see if there is any su having hastate standby and having lower rank than active higher rank su,
             // then remove their work by setting their admin state to idle
             if (su->adminState.value == AdministrativeState::on)
             {
                su->adminState.value = AdministrativeState::idle;
             }
          }
          boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
          for (itsu = sus.begin(); itsu != sus.end(); ++itsu)
          {
             SAFplusAmf::ServiceUnit* su = dynamic_cast<ServiceUnit*>(*itsu);
             // then add their work by setting their admin state to on
             if (su->adminState.value == AdministrativeState::idle)
             {
                su->adminState.value = AdministrativeState::on;
             }
          }
      }
      else
      {
          logDebug("N+M","ADJUST","sg [%s] doesn't need to be adjusted because its SUs with ranks have appropriate hastate", sg->name.value.c_str());
      }
      return rc;
  }
#endif

  void updateStateDueToProcessDeath(SAFplusAmf::Component* comp)
  {
    assert(comp);
    logDebug("POL","CUSTOM","updating states of component [%s] due to its process dead", comp->name.value.c_str());
    // Reset component's basic state to dead
    comp->presenceState = PresenceState::uninstantiated;
    comp->activeAssignments = 0;
    comp->standbyAssignments = 0;
    comp->assignedWork = "";
    comp->readinessState = ReadinessState::outOfService;
    // right now, only the customer changes this; with presence uninstantiated, this comp obviously can't take an assignment: comp->haReadinessState = HighAvailabilityReadinessState::notReadyForAssignment;
    comp->haState = HighAvailabilityState::idle;
    comp->pendingOperation = PendingOperation::none;
    comp->pendingOperationExpiration.value.value = 0;
    comp->processId = 0;
    SAFplus::name.set(comp->name,INVALID_HDL,NameRegistrar::MODE_NO_CHANGE);  // remove the handle in the name service because the component is dead
    comp->currentRecovery = Recovery::None;
    comp->serviceUnit.value->currentRecovery = Recovery::None;
    comp->serviceUnit.value->node.value->currentRecovery = Recovery::None;
    comp->launched = false;
    /* update states of my proxied components */
    if (comp->compProperty.value == CompProperty::sa_aware)
    {
      logDebug("COMP","DEAD","update states of my proxied components if any");
      SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Container& vec = comp->proxied.value;
      std::vector<SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem>::iterator itvec = vec.begin();            
      for(; itvec != vec.end(); itvec++)
      {
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::Elem elem = *itvec;
        SAFplusAmf::Component* c = elem.value;
        updateStateDueToProcessDeath(c);
      }
    }

    SAFplusAmf::ServiceUnit* su = comp->serviceUnit.value;
    SAFplusAmf::ServiceGroup* sg = su->serviceGroup.value;
    
    // I need to remove any CSIs that were assigned to this component.
    //SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator itsi;
    //SAFplus::MgtProvList<SAFplusAmf::ServiceInstance*>::ContainerType::iterator endsi = sg->serviceInstances.value.end();
    SAFplus::MgtObject::Iterator itsi;
    SAFplus::MgtObject::Iterator endsi = sg->serviceInstances.end();
    for (itsi = sg->serviceInstances.begin();itsi != endsi; itsi++)
      {
        ServiceInstance* si = dynamic_cast<ServiceInstance*> (itsi->second);
      const std::string& name = si->name;

      SAFplus::MgtObject::Iterator itcsi;
      SAFplus::MgtObject::Iterator endcsi = si->componentServiceInstances.end();
    //SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator itcsi;
    //SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*>::ContainerType::iterator endcsi = si->componentServiceInstances.value.end();

      SAFplusAmf::ComponentServiceInstance* csi = NULL;
      bool found = false;
      bool isPartiallyAssignment = false;
      // Look for which CSI is assigned to the dead component and remove the assignment.
      for (itcsi = si->componentServiceInstances.begin(); itcsi != endcsi; itcsi++)
        {
          csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*> (itcsi->second);
        if (!csi || !(csi->type == comp->csiType.value)) continue;
        found = csi->activeComponents.erase(comp);
        if(found){
            isPartiallyAssignment=true;
            si->isFullActiveAssignment=false;
        }
        found = csi->standbyComponents.erase(comp);
        if(found){
            isPartiallyAssignment=true;
            si->isFullStandbyAssignment=false;
        }
#if 0
        if (1)
          {
            found |= csi->activeComponents.erase(comp);

          std::vector<SAFplusAmf::Component*>& vec = csi->activeComponents.value;
          if (std::find(vec.begin(), vec.end(), comp) != vec.end())
            {
            found = true;
            vec.erase(std::remove(vec.begin(),vec.end(), comp), vec.end());
            }
          }
        if (1)
          {
          std::vector<SAFplusAmf::Component*>& vec = csi->standbyComponents.value;
          if (std::find(vec.begin(), vec.end(), comp) != vec.end())
            {
            found = true;
            vec.erase(std::remove(vec.begin(),vec.end(), comp), vec.end());
            }
          }
#endif
        }
      if (isPartiallyAssignment) si->assignmentState = AssignmentState::partiallyAssigned;// TODO: or it could be unassigned...
      }

  }

  // First step in the audit is to update the current state of every entity to match the reality.
  void NplusMPolicy::auditDiscovery(SAFplusAmf::SAFplusAmfModule* root)
  {
      logInfo("POL","N+M","Active audit: Discovery phase");
      assert(root);
      UninstantiatedCountMap uninstCountMap;
      SAFplusAmfModule* cfg = (SAFplusAmfModule*) root;

      MgtObject::Iterator it;
      for (it = cfg->safplusAmf.serviceGroupList.begin();it != cfg->safplusAmf.serviceGroupList.end(); it++)
      {
          ServiceGroup* sg = dynamic_cast<ServiceGroup*> (it->second);
          const std::string& name = sg->name;

          logInfo("N+M","AUDIT","Auditing service group [%s]", name.c_str());
          if (1) // TODO: figure out if this Policy should control this Service group
          {
              SAFplus::MgtObject::Iterator itsu = sg->serviceUnits.begin();
              SAFplus::MgtObject::Iterator endsu = sg->serviceUnits.end();
              for (; itsu != endsu; ++itsu)
              {
                  uint readyForAssignment;
                  readyForAssignment = 0;
                  ServiceUnit* su = dynamic_cast<ServiceUnit*>(itsu->second);
                  Node* suNode = su->node.value;
                  const std::string& suName = su->name;
                  logInfo("N+M","AUDIT","Auditing service unit [%s]", suName.c_str());

                  // Calculate readiness state SAI-AIS-AMF-B.04.01.pdf sec 3.2.1.4
                  ReadinessState rs = su->readinessState.value;
                  // out of service if needs repair
                  if ((suNode == nullptr) || (su->operState.value == false)||(suNode->operState.value == false)
                          // or any related entity is adminstratively off
                          || (su->adminState.value != AdministrativeState::on) || (suNode->adminState.value != AdministrativeState::on) || (sg->adminState.value != AdministrativeState::on)
                          || ((nullptr!=sg->application.value) && (sg->application.value->adminState != AdministrativeState::on))
                          // or its presence state is neither instantiated nor restarting,
                          || ((su->presenceState.value != PresenceState::instantiated) && (su->presenceState.value != PresenceState::restarting))
                          // TODO: or the service unit contains contained components, and their configured container CSI is not assigned active or quiescing to any container component on the node that contains the service unit.
                          )
                  {
                      rs = ReadinessState::outOfService;
                  }

                  // in service if does not need repair
                  else if ((suNode)&&(su->operState == true)&&(suNode->operState == true)
                           // and administratively on
                           && (su->adminState == AdministrativeState::on) && (suNode->adminState == AdministrativeState::on) && (sg->adminState == AdministrativeState::on)
                           && (!sg->application.value || (sg->application.value->adminState == AdministrativeState::on))
                           // and its presence state is either instantiated or restarting,
                           && ((su->presenceState == PresenceState::instantiated) || (su->presenceState == PresenceState::restarting)))
                  {
                      rs = ReadinessState::inService;
                  }
                  else
                  {
                      // TODO: stopping
                  }

                  if (rs != su->readinessState.value)
                  {
                      logInfo("N+M","AUDIT","Readiness state of Service Unit [%s] changed from [%s] to [%s]", su->name.value.c_str(),c_str(su->readinessState),c_str(rs));
                      su->readinessState.value = rs;
                      amfOps->reportChange();
                      // TODO event?
                  }


                  int numComps = 0;
                  // count up the presence state of each component so I can infer the presence state of the SU
                  int presenceCounts[((int)PresenceState::terminationFailed)+1];
                  int haCounts[((int)HighAvailabilityState::quiescing)+1];
                  for (int j = 0; j<((int)PresenceState::terminationFailed)+1;j++) presenceCounts[j] = 0;
                  for (int j = 0; j<((int)HighAvailabilityState::quiescing)+1;j++) haCounts[j] = 0;

                  SAFplus::MgtObject::Iterator itcomp;
                  SAFplus::MgtObject::Iterator endcomp = su->components.end();
                  for (itcomp = su->components.begin(); itcomp != endcomp; itcomp++)
                  {
                      Component* comp = dynamic_cast<Component*>(itcomp->second);
                      if (comp->compProperty.value == SAFplusAmf::CompProperty::proxied_preinstantiable &&
                              !comp->launched &&
                              amfOps->suContainsSaAwareComp(su))
                      {
                          //Don't count proxied preinstantiable because it must be instantiated after its proxy gets assignment
                          continue;
                      }
                      numComps++;
                      logInfo("N+M","AUDIT","Component [%s]: operState [%s]", comp->name.value.c_str(), comp->operState.value ? "enabled" : "faulted");
                      if (!running(comp->presenceState))
                      {
                          if (comp->processId.value) comp->processId.value=0;  // TODO: should we check the node to see if there is a process?
                          //  TODO: see if its in the name service
                      }
                      else // If I think its running, let's check it out.
                      {
                          CompStatus status = amfOps->getCompState(comp);

                          // Node died
                          //if ((su->readinessState.value == ReadinessState::outOfService)&&(comp->presenceState !=  PresenceState::uninstantiated))
                          if ((su->node.value->presenceState.value == PresenceState::uninstantiated)&&(comp->presenceState !=  PresenceState::uninstantiated))
                          {
                              logInfo("N+M","DSC","SU went out of service during Component [%s] instantiation.", comp->name.value.c_str());
                              updateStateDueToProcessDeath(comp);
                          }

                          // If someone shuts off the SG or SU, then we want to reset certain fields in the component
                          if ((su->adminState == AdministrativeState::off)||(sg->adminState == AdministrativeState::off))
                          {
                              if (comp->numInstantiationAttempts != 0) { comp->numInstantiationAttempts = 0; amfOps->reportChange(); }   // Reset the instantiation attempts to reflect the user's resetting of the app.  If we don't do this, a bunch of start/stops in a row could trigger the "maxInstantiations" timer to fault the component.
                          }

                          // In the instantiating case, the process may not have even been started yet.  So to detect failure, we must check both that it has reported a PID and that it is currently uninstantiated
                          if ((effectiveAdminState(comp) == AdministrativeState::on) && ((status == CompStatus::Uninstantiated) && ((comp->processId.value > 0) || (comp->presenceState != PresenceState::instantiating))))  // database shows should be running but actually no process is there.  I should update DB.
                          {
                              try
                              {
                                  Handle compHdl = ::name.getHandle(comp->name);
                                  // Actually it throws NameException: assert(compHdl != INVALID_HDL);  // AMF MUST register this component before it does anything else with it so the name must exist.
                                  fault->notify(compHdl,FaultEventData(AlarmState::ALARM_STATE_ASSERT,AlarmCategory::ALARM_CATEGORY_PROCESSING_ERROR,AlarmSeverity::ALARM_SEVERITY_MAJOR, AlarmProbableCause::ALARM_PROB_CAUSE_SOFTWARE_ERROR));
                                  // TODO: it may be better to have the AMF react to fault manager's notification instead of doing it preemptively here
                              }
                              catch(NameException& ne)
                              {
                                  logWarning("N+M","AUDIT","Component [%s] is marked as running with uninstantiated state and no name registration (no handle).  It may have died at startup.", comp->name.value.c_str());
                              }
                              compFaultReport(comp);//we must have more steps to handle for other comps in the same su in node switchover case
                          }
                          else if (comp->presenceState == PresenceState::instantiating)  // If the component is in the instantiating state, look for it to register with the AMF
                          {
                              Handle compHandle=INVALID_HDL;

                              try
                              {
                                  RefObjMapPair p = SAFplus::name.get(comp->name);  // The way a component "registers" is that it puts its name in the name service.
                                  compHandle = p.first;
                              }
                              catch(NameException& n)
                              {
                                  // compHandle=INVALID_HDL; I'd do this but its already set.
                              }

                              if (compHandle != INVALID_HDL) // TODO: what other things do we need to do for registration?
                              {
                                  comp->presenceState = PresenceState::instantiated;
                                  if (comp->compProperty.value == CompProperty::proxied_preinstantiable)
                                  {
                                      readyForAssignment++;
                                  }
                                  logDebug("M+N","AUDIT","comp name [%s] presence state changes to [%s], readyForAssignment [%d]", comp->name.value.c_str(),c_str(comp->presenceState.value), readyForAssignment);
                                  fault->registerEntity(compHandle ,FaultState::STATE_UP);
                              }
                              else
                              {
                                  uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                  if (curTime - comp->lastInstantiation.value.value >= comp->getInstantiate()->timeout.value)
                                  {
                                      // logError("N+M","DSC","Component [%s] never registered with AMF after instantiation.", comp->name.value.c_str());
                                      // Process is not responding after instantiation.  We'll catch and kill it in the operation's phase... nothing to do here
                                  }
                                  else
                                  {
                                      logInfo("N+M","DSC","Component [%s] waiting [%lu] more milliseconds for instantiation.", comp->name.value.c_str(),(long unsigned int) (comp->getInstantiate()->timeout.value - (curTime - comp->lastInstantiation.value.value)));
                                  }
                              }


                          }
                          else if (comp->presenceState == PresenceState::instantiated)
                          {
                              // If the component has been instantiated for long enough, reset the instantiation attempts.

                              uint64_t curTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                              if ((curTime - comp->lastInstantiation.value.value >= comp->instantiationSuccessDuration.value)&&(comp->numInstantiationAttempts.value != 0))
                              {
                                  comp->numInstantiationAttempts.value = 0;
                                  amfOps->reportChange();
                              }

                              if (su->adminState.value == AdministrativeState::on && comp->haReadinessState == HighAvailabilityReadinessState::readyForAssignment)
                              {
                                  readyForAssignment++;
                                  logDebug("M+N","AUDIT","comp name [%s] haReadinessState [%s]", comp->name.value.c_str(),c_str(comp->haReadinessState.value));
                                  logDebug("M+N","AUDIT","readyForAssignment [%d]", readyForAssignment);
                              }
                              else if (su->adminState.value == AdministrativeState::idle)
                              {
                                  comp->haReadinessState.value = HighAvailabilityReadinessState::notReadyForAssignment;
                                  logDebug("M+N","AUDIT","comp name [%s] haReadinessState [%s]", comp->name.value.c_str(),c_str(comp->haReadinessState.value));
                                  logDebug("M+N","AUDIT","readyForAssignment [%d]", readyForAssignment);
                              }
                              else if (su->adminState.value == AdministrativeState::on)
                              {
                                  comp->haReadinessState.value = HighAvailabilityReadinessState::readyForAssignment;
                                  logDebug("M+N","AUDIT","comp name [%s] haReadinessState [%s]", comp->name.value.c_str(),c_str(comp->haReadinessState.value));
                                  logDebug("M+N","AUDIT","readyForAssignment [%d]", readyForAssignment);
                              }
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

                      // From the point of view of the Service Unit, a non_preinstantiable component is always "instantiated" (ready for work assignment), because the act of assigning active work is what instantiates it
                      if (comp->capabilityModel == CapabilityModel::not_preinstantiable)
                      {
                          presenceCounts[(int)PresenceState::instantiated]++;
                          assert(((int)comp->haState.value <= (int)HighAvailabilityState::quiescing)&&((int)comp->haState.value >= (int)HighAvailabilityState::active));
                          haCounts[(int)comp->haState.value]++;
                      }
                      else
                      {
                          assert(((int)comp->presenceState.value <= ((int)PresenceState::terminationFailed))&&((int)comp->presenceState.value >= ((int)PresenceState::uninstantiated)));
                          presenceCounts[(int)comp->presenceState.value]++;
                          assert(((int)comp->haState.value <= (int)HighAvailabilityState::quiescing)&&((int)comp->haState.value >= (int)HighAvailabilityState::active));
                          haCounts[(int)comp->haState.value]++;
                      }
                      //amfOps->reportChange();
                  }


                  // High Availability state calculation (TODO: validate logic against AMF spec)
                  HighAvailabilityState ha = su->haState; // .value;

                  if (haCounts[(int)HighAvailabilityState::quiescing] > 0)  // If any component is quiescing, the Service Unit's HA state is quiescing
                  {
                      ha = HighAvailabilityState::quiescing;
                  }
                  else if (haCounts[(int)HighAvailabilityState::active] == numComps)  // If all components have an active assignment, SU is active
                  {
                      ha = HighAvailabilityState::active;
                  }
                  else if (haCounts[(int)HighAvailabilityState::standby] == numComps)  // If all components have a standby assignment, SU is standby
                  {
                      ha = HighAvailabilityState::standby;
                  }
                  else ha = HighAvailabilityState::idle;

                  if (ha != su->haState.value)
                  {
                      // high availability state changed.
                      logInfo("N+M","AUDIT","High Availability state of Service Unit [%s] changed from [%s (%d)] to [%s (%d)]", su->name.value.c_str(),c_str(su->haState.value),(int) su->haState.value, c_str(ha), (int) ha);
                      su->haState = ha;
                      amfOps->reportChange();

                      // TODO: Event?
                  }


                  // SAI-AIS-AMF-B.04.01.pdf sec 3.2.1.1, presence state calculation
                  PresenceState ps = su->presenceState.value;

                  // Start by seeing if we are stopping the system...
                  AdministrativeState nodeAs = AdministrativeState::off;  // If there is no node, then it is effectively administratively off
                  if (suNode) nodeAs = suNode->adminState;
                  if ((su->adminState == AdministrativeState::off) || (nodeAs == AdministrativeState::off) || (sg->adminState == AdministrativeState::off))
                  {
                      if ((ps == PresenceState::instantiating) || (ps == PresenceState::instantiated) || (ps == PresenceState::restarting))
                      {
                          ps = PresenceState::terminating;
                      }
                  }

                  if (presenceCounts[(int)PresenceState::uninstantiated] == numComps)  // When all components are uninstantiated, the service unit is uninstantiated.
                  {
                      ps = PresenceState::uninstantiated;
                  }
                  else if (presenceCounts[(int)PresenceState::instantiating] > 0) // When the first component moves to instantiating, the service unit also becomes instantiating.
                  {
                      ps = PresenceState::instantiating;
                  }
                  else if (presenceCounts[(int)PresenceState::instantiated] == numComps) // When all pre-instantiable components of a service unit enter the instantiated state, the service unit becomes instantiated
                  {
                      ps = PresenceState::instantiated;
                  }
                  else if (presenceCounts[(int)PresenceState::instantiationFailed] > 0)  // If, after all possible retries, a component cannot be instantiated, the presence state of the component is set to instantiation-failed, and the presence state of the service unit is also set to instantiation-failed.
                  {
                      ps = PresenceState::instantiationFailed;
                  }
                  else if (presenceCounts[(int)PresenceState::terminating] > 0)  // When the first component of an already instantiated service unit becomes terminating, the service unit becomes terminating.
                  {
                      ps = PresenceState::terminating;
                  }
                  else if (presenceCounts[(int)PresenceState::terminationFailed] > 0)  // If the Availability Management Framework fails to terminate a component, the presence state of the component is set to termination-failed and the presence state of the service unit is also set to termination-failed.
                  {
                      ps = PresenceState::terminationFailed;
                  }
                  else if (presenceCounts[(int)PresenceState::restarting] == numComps) // When all components enter the restarting state, the service unit become restarting.  However, if only some components are restarting, the service unit is still instantiated.
                  {
                      ps = PresenceState::restarting;
                  }

                  if (ps != su->presenceState.value)
                  {
                      // Presence state changed.
                      logInfo("N+M","AUDIT","Presence state of Service Unit [%s] changed from [%s (%d)] to [%s (%d)]", su->name.value.c_str(),c_str(su->presenceState.value),(int) su->presenceState.value, c_str(ps), (int) ps);
                      su->presenceState = ps;
                      if (ps == PresenceState::uninstantiated)
                      {
                          if (uninstCountMap.find(suNode) != uninstCountMap.end())
                          {
                              int count = uninstCountMap[suNode];
                              uninstCountMap[suNode] = ++count;
                              //logInfo("N+M","AUDIT","increase [%d]", uninstCountMap[suNode]);
                          }
                          else
                          {
                              int count = 1;
                              //logInfo("N+M","AUDIT","add new pair (%s->%d) to the map", suNode->name.value.c_str(), count);
                              uninstCountMap[suNode] = count;
                          }
                      }
                      amfOps->reportChange();

                      // TODO: Event?
                  }


                  // Now address SU's haReadinessState.  AMF B04.01 states that the haReadiness state should be per SU/SI combination.  It is the ability of this SU to accept a particular piece of work.  At this point I am going to reject this as unnecessary complexity and have a single haReadinessState per SU.  Having work that cannot be applied to particular SUs is problematic because the AMF has no way (other than guessing) to determine which work can be assigned to which SU.  If work can't be assigned to a SU, it should not be in that SG in the first place (create a separate SG for that work).
                  HighAvailabilityReadinessState hrs; // = su->haReadinessState;
                  if (readyForAssignment == numComps) hrs = HighAvailabilityReadinessState::readyForAssignment;
                  else hrs = HighAvailabilityReadinessState::notReadyForAssignment;
                  if (hrs != su->haReadinessState)
                  {
                      logInfo("N+M","AUDIT","High availability readiness state of Service Unit [%s] changed from [%s (%d)] to [%s (%d)]", su->name.value.c_str(),c_str(su->haReadinessState.value),(int) su->haReadinessState.value, c_str(hrs), (int) hrs);
                      su->haReadinessState.value = hrs;

                      if (hrs == HighAvailabilityReadinessState::notReadyForAssignment)
                      {
                          if ((su->numActiveServiceInstances.current > 0) || (su->numStandbyServiceInstances.current > 0))
                          {
                              amfOps->removeWork(su);
                          }
                      }

                  }

              }
          }

          MgtObject::Iterator itsi;
          MgtObject::Iterator endsi = sg->serviceInstances.end();
          for (itsi = sg->serviceInstances.begin(); itsi != endsi; itsi++)
          {
              SAFplusAmf::ServiceInstance* si = dynamic_cast<ServiceInstance*>(itsi->second);
              logInfo("N+M","AUDIT","Auditing service instance [%s]", si->name.value.c_str());

              //si->getNumActiveAssignments()->current.value = 0;  // TODO set this correctly
              //si->getNumStandbyAssignments()->current.value = 0; // TODO set this correctly
              SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itcsi;
              SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator endcsi = si->componentServiceInstances.listEnd();
              si->isFullActiveAssignment = true;
              si->isFullStandbyAssignment = true;
              for(itcsi = si->componentServiceInstances.listBegin(); itcsi != endcsi; ++itcsi)
              {
                  ComponentServiceInstance* csi = dynamic_cast<ComponentServiceInstance*>(*itcsi);
                  if(csi->activeComponents.value.empty() == true) si->isFullActiveAssignment=false;
                  if(csi->standbyComponents.value.empty() == true) si->isFullStandbyAssignment=false;
                  if(!si->isFullActiveAssignment.value && !si->isFullStandbyAssignment.value)break;
              }
              //AssignmentState as = si->assignmentState;
              AssignmentState as;
              //if (wat.si->assignmentState = AssignmentState::fullyAssigned;  // TODO: for now just make the SI happy to see something work
              if(si->getNumActiveAssignments()->current.value == si->getPreferredActiveAssignments() && si->isFullActiveAssignment.value &&
                      si->getNumStandbyAssignments()->current.value == si->getPreferredStandbyAssignments() && si->isFullStandbyAssignment.value)
              {
                  as = AssignmentState::fullyAssigned;
              }
              else if(si->getNumActiveAssignments()->current.value == 0 &&
                      si->getNumStandbyAssignments()->current.value == 0)
              {
                  as = AssignmentState::unassigned;
              }
              else as = AssignmentState::partiallyAssigned;
              if (as != si->assignmentState)
              {
                  logInfo("N+M","AUDIT","Assignment state of service instance [%s] changed from [%s (%d)] to [%s (%d)]", si->name.value.c_str(),c_str(si->assignmentState.value),(int) si->assignmentState.value, c_str(as), (int) as);
                  si->assignmentState = as;
                  amfOps->reportChange();
              }
          }


      }
      UninstantiatedCountMap::iterator itMap = uninstCountMap.begin();
      for(; itMap != uninstCountMap.end(); itMap++)
      {
          Node* node = itMap->first;
          int count = itMap->second;
          if (node->serviceUnits.value.size() == count)
          {
              PresenceState ps = PresenceState::uninstantiated;
              logInfo("N+M","AUDIT","Presence state of Node [%s] changed from [%s (%d)] to [%s (%d)]", node->name.value.c_str(),c_str(node->presenceState.value),(int) node->presenceState.value, c_str(ps), (int) ps);
              node->presenceState = ps;
          }
      }
      // free the map
      /*for(itMap = uninstCountMap.begin(); itMap != uninstCountMap.end(); itMap++)
      {
          int* count = (int*)itMap->second;
          if (count) {
             logInfo("N+M","AUDIT","deleting (%s->%d) from the map", (Node*)itMap->first->name.value.c_str(), *count);
             delete count;
          }
          else
             logInfo("N+M","AUDIT", "null pointer");
      }*/
      uninstCountMap.clear();
  }

  void NplusMPolicy::standbyAudit(SAFplusAmf::SAFplusAmfModule* root)
    {
    logInfo("POL","CUSTOM","Standby audit");
    }
  static NplusMPolicy api;
  void NplusMPolicy::compFaultReport(Component* comp, const Recovery recommRecovery)
  {
      logDebug("POL","N+M","Component failure reported for component [%s], recommended recovery [%s], current recovery [%s]", comp->name.value.c_str(),c_str(recommRecovery),c_str(recommendedRecovery));
      bool escalation = false;
      processedComp = comp;
      if (0) //(amfOps->nodeGracefulSwitchover)
      {          
          recommendedRecovery = Recovery::None;          
      }
      else
      {
          recommendedRecovery = recommRecovery;

          computeCompRecoveryAction(comp, &recommendedRecovery, &escalation);

          logDebug("POL","N+M","Fault on component [%s], recommended recovery [%s], escalation [%s]", comp->name.value.c_str(),c_str(recommendedRecovery), escalation?"Yes":"No");

          switch(recommendedRecovery)
          {
              case Recovery::CompRestart:
                  comp->serviceUnit.value->lastCompRestart.value.value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                  break;
              case Recovery::SuRestart:
              case Recovery::CompFailover:
                  computeSURecoveryAction(comp->serviceUnit.value, comp, &recommendedRecovery, &escalation);
                  postProcessForSURecoveryAction(comp->serviceUnit.value, comp, &recommendedRecovery, &escalation);
                  break;
              case Recovery::NodeSwitchover:
              case Recovery::NodeFailover:
              case Recovery::NodeFailfast:
                  computeNodeRecoveryAction(comp->serviceUnit.value->node.value, &recommendedRecovery, &escalation);
                  break;
          }
      }
      if(recommendedRecovery!=SAFplusAmf::Recovery::CompRestart)
      {
          MgtIdentifierList<Component*>::iterator itcomp = comp->serviceUnit.value->components.listBegin();
          MgtIdentifierList<Component*>::iterator end = comp->serviceUnit.value->components.listEnd();
          for(;itcomp != end;++itcomp)
          {
              Component* iterComp = dynamic_cast<Component*>(*itcomp);
              if(comp->name.value.compare(iterComp->name.value)!=0)
              {
                  amfOps->stop(iterComp);
              }
              amfOps->cleanup(iterComp);
              updateStateDueToProcessDeath(iterComp);
          }
      }
      else
      {
          amfOps->cleanup(comp);
          if(recommendedRecovery != SAFplusAmf::Recovery::NodeFailfast)updateStateDueToProcessDeath(comp);
      }
      logDebug("POL","N+M","processing faulty component [%s], recommended recovery [%s], escalation [%s]", comp->name.value.c_str(),c_str(recommendedRecovery), escalation?"Yes":"No");
  }

  void NplusMPolicy::computeCompRecoveryAction(Component* comp, Recovery* recovery, bool* escalation)
  {
      AdministrativeState adminState;
      Recovery computedRecovery = *recovery;
      ServiceUnit* su = comp->serviceUnit.value;
      assert(su);
      ServiceGroup* sg = su->serviceGroup.value;
      assert(sg);
      Node* node = su->node.value;
      assert(node);
      bool match = false;

      if(computedRecovery == Recovery::NoRecommendation)
      {
          computedRecovery = comp->recovery.value;
          logInfo("POL","COMPUTE","1 computedRecovery[%s] configRecovery[%s] currentRecovery[%s]",c_str(computedRecovery),c_str(comp->recovery.value), c_str(comp->currentRecovery.value));
          if(computedRecovery == Recovery::NoRecommendation)
          {
              computedRecovery = comp->restartable.value ?
                                  Recovery::CompRestart :
                                  Recovery::CompFailover;
          }
      }

    adminState = effectiveAdminState(comp->serviceUnit.value);
    if(adminState == AdministrativeState::idle)
    {
        if ( (computedRecovery == Recovery::NoRecommendation) ||
             (computedRecovery == Recovery::CompRestart)      ||
             (computedRecovery == Recovery::SuRestart) )
        {
            computedRecovery = Recovery::CompFailover;
        }
    }

         /*
     * -ve
     * If this is an external fault, it is only allowed if it has a larger scope
     * than any existing fault recovery affecting the component.
     */
    if ( *escalation == false )
    {
        if ( compareEntityRecoveryScope(comp->currentRecovery.value, computedRecovery) ||
             compareEntityRecoveryScope(su->currentRecovery.value, computedRecovery) ||
             compareEntityRecoveryScope(node->currentRecovery.value, computedRecovery))
        {
            computedRecovery = Recovery::None;
        }
    }

    /*
     * +ve
     * If this is an escalated fault, make sure it is atleast at the same level
     * as the current recovery.
     */

    else
    {
        if ( compareEntityRecoveryScope(comp->currentRecovery.value, computedRecovery))
        {
            computedRecovery = comp->currentRecovery.value;
        }

        if ( compareEntityRecoveryScope(su->currentRecovery.value, computedRecovery))
        {
            computedRecovery = su->currentRecovery.value;
        }

        if ( compareEntityRecoveryScope(node->currentRecovery.value, computedRecovery))
        {
            computedRecovery = node->currentRecovery.value;
        }
    }
      if ( computedRecovery == Recovery::CompRestart )
    {
        
        match = true;
        if ( comp->restartable.value)
        {
            ++su->compRestartCount;
            if(su->compRestartCount > sg->componentRestart.maximum.value)
            {
                computedRecovery = Recovery::SuRestart;
                *escalation = true;
            }
        }
        else
        {
            computedRecovery = Recovery::SuRestart;
            *escalation = true;
        }
    }
    if(computedRecovery == Recovery::SuRestart || computedRecovery == Recovery::CompFailover ||
        computedRecovery == Recovery::NodeSwitchover || computedRecovery == Recovery::NodeFailover ||
        computedRecovery == Recovery::NodeFailfast || computedRecovery == Recovery::ClusterReset)
      {
          match = true;
      }

    if(!*escalation && computedRecovery == comp->currentRecovery.value)
    {
        match = false;
    }

    if(!match)computedRecovery = Recovery::None;

    *recovery = computedRecovery;

    if(computedRecovery != Recovery::None)
    {
        comp->currentRecovery.value = computedRecovery;
    }
  }

  void NplusMPolicy::computeSURecoveryAction(ServiceUnit* su, Component* faultyComp, Recovery* recovery, bool* escalation )
  {
      bool match = false;
      assert(su);
      assert(faultyComp);
      ServiceGroup* sg = su->serviceGroup.value;
      assert(sg);
      Node* node = su->node.value;
      assert(node);
      //Pre compute SU recovery action
      if(su->presenceState == PresenceState::uninstantiated)
      {
          logDebug("AMS", "FLT-SU", "Fault on SU[%s]: Presence State[%s]. Ignore fault ...",su->name.value.c_str(),c_str(su->presenceState.value));
          return;
      }
      if(*recovery == Recovery::CompFailover)
      {
          bool foundOtherSu = false;
          SAFplus::MgtObject::Iterator itsu = sg->serviceUnits.begin();
          SAFplus::MgtObject::Iterator endsu = sg->serviceUnits.end();
          for(; itsu != endsu; itsu++ )
          {
              ServiceUnit* foundSu = dynamic_cast<ServiceUnit*>(itsu->second);
              if(su == foundSu)continue;
              if(foundSu->operState && foundSu->presenceState == PresenceState::instantiated)
              {
                  foundOtherSu = true;
                  break;
              }
          }

          /*
         * SU failover with restart
         */
          if(!foundOtherSu && su->adminState == AdministrativeState::on)
          {
              *recovery = Recovery::SuRestart;
              *escalation = true;
          }
      }
      //End pre compute SU recovery action

      if ( (*recovery == Recovery::NoRecommendation) ||
         (*recovery == Recovery::CompRestart) )
      {
        match = true;

        *recovery = su->restartable.value ?
                        Recovery::SuRestart :
                        Recovery::CompFailover;
      }

      if ( *escalation == false )
    {
        if ( compareEntityRecoveryScope(su->currentRecovery.value, *recovery) ||
             compareEntityRecoveryScope(node->currentRecovery.value, *recovery) )
        {
            *recovery = Recovery::None;
        }
    }

    if ( *recovery == Recovery::SuRestart )
    {
        match = CL_TRUE;

        if ( su->restartable.value )// isRestartable stil not implemented on ide yet.
        {
            ++su->restartCount;
            if ( su->restartCount > sg->serviceUnitRestart.maximum )
            {
                *recovery = Recovery::CompFailover;
                *escalation = true;
            }
        }
        else
        {
            *recovery = Recovery::CompFailover;
            *escalation = true;
        }
    }

     if(*recovery == Recovery::CompFailover)
     {
        match = true;
        ++node->serviceUnitFailureEscalationPolicy.failureCount;
        if(node->serviceUnitFailureEscalationPolicy.failureCount > node->serviceUnitFailureEscalationPolicy.maximum.value)
        {
            *recovery = Recovery::NodeFailover;
            *escalation = true;
        }
     }

    if ( *recovery == Recovery::NodeSwitchover
    || *recovery == Recovery::NodeFailover
    || *recovery == Recovery::NodeFailfast
    || *recovery == Recovery::ClusterReset)
    {
        match = true;
    }

    if ( !*escalation && *recovery == su->currentRecovery.value )
    {
        match = false;
    }

    if ( !match )
    {
        *recovery = Recovery::None;
    }

    if ( *recovery != Recovery::None )
    {
        su->currentRecovery = *recovery;
    }
  }

void NplusMPolicy::postProcessForSURecoveryAction(ServiceUnit* su, Component* faultyComp, Recovery* recovery, bool* escalation)
{
     /*
     * Stop all possible timers that could be running for the SU
     * and clear the invocation list.
     */

    //AMS_CALL ( clAmsEntityClearOps((ClAmsEntityT *)su) );

    /*
     * Delete pending reassign ops against this SU.
     */
    //clAmsPeSUSIReassignEntryDelete(su);
    /*
     * Clear the entity stack operations.
     */
    //clAmsEntityOpsClear((ClAmsEntityT*)su, &su->status.entity);

    switch(*recovery)
    {
        case Recovery::SuRestart:
            su->lastRestart.value.value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            break;
        case Recovery::CompFailover:
            su->node.value->lastSUFailure.value.value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            break;
        case Recovery::NodeSwitchover:
        case Recovery::NodeFailover:
        case Recovery::NodeFailfast:
            computeNodeRecoveryAction(su->node.value, recovery, escalation);
            break;
    }
}

void NplusMPolicy::computeNodeRecoveryAction(Node* node, Recovery* recovery, bool* escalation)
{
    bool match = false;
    if(!node->operState.value )
    {
        match = true;
        *recovery = Recovery::None;
    }

    if(*recovery == Recovery::NoRecommendation)
    {
        match = true;
        if(node->restartable.value)
        {
            *recovery = Recovery::NodeFailover;
        }
        else
        {
            *recovery = Recovery::ClusterReset;
            *escalation = true;
        }
    }

    if(!*escalation)
    {
        if(compareEntityRecoveryScope(node->currentRecovery.value, *recovery))
        {
            *recovery = Recovery::None;
        }
    }

    if(*recovery == Recovery::NodeSwitchover
    || *recovery == Recovery::NodeFailover
    || *recovery == Recovery::NodeFailfast
    || *recovery == Recovery::ClusterReset)
    {
        match = true;
    }

    if ( *recovery == node->currentRecovery.value )
    {
        match = false;
    }

    if ( !match )
    {
        *recovery = Recovery::None;
    }

    if ( *recovery != Recovery::None )
    {
        node->currentRecovery = *recovery;
    }
}

bool compareEntityRecoveryScope(Recovery a, Recovery b)
{
    if ( a == b )
    {
        return true;
    }

    switch ( a )
    {
        case Recovery::None:
        {
            break;
        }

        case Recovery::CompRestart:
        {
            if ( b == Recovery::None )
            {
                return true;
            }

            break;
        }

        case Recovery::SuRestart:
        {
            if ( (b == Recovery::None)            ||
                 (b == Recovery::CompRestart) )
            {
                return true;
            }

            break;
        }

        case Recovery::CompFailover:
        //case CL_AMS_RECOVERY_INTERNALLY_RECOVERED:
        {
            if ( (b == Recovery::None)            ||
                 (b == Recovery::CompRestart) )
            {
                return true;
            }
            break;
        }

        case Recovery::NodeSwitchover:
        {
            if ( (b == Recovery::None)           ||
                 (b == Recovery::CompRestart)    ||
                 (b == Recovery::SuRestart)      ||
                 (b == Recovery::CompFailover)   ||
                 //(b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == Recovery::ApplicationRestart) )
            {
                return true;
            }
            break;
        }

        case Recovery::NodeFailover:
        {
            if ( (b == Recovery::None)           ||
                 (b == Recovery::CompRestart)    ||
                 (b == Recovery::SuRestart)      ||
                 (b == Recovery::CompFailover)   ||
                 //(b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == Recovery::NodeSwitchover) ||
                 (b == Recovery::ApplicationRestart) )
            {
                return true;
            }
            break;
        }

        case Recovery::NodeFailfast:
        {
            if ( (b == Recovery::None)           ||
                 (b == Recovery::CompRestart)    ||
                 (b == Recovery::SuRestart)      ||
                 (b == Recovery::CompFailover)   ||
                 //(b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == Recovery::NodeSwitchover) ||
                 (b == Recovery::NodeFailover)   ||
                 (b == Recovery::ApplicationRestart) )
            {
                return true;
            }
            break;
        }

        //case CL_AMS_RECOVERY_NODE_HALT:
        //{
            /*
             * Supercedes everything else.
             */
        //    return CL_TRUE;
        //}
        //break;

        case Recovery::ApplicationRestart:
        {
            if ( (b == Recovery::None)           ||
                 (b == Recovery::CompRestart)    ||
                 (b == Recovery::SuRestart)      ||
                 (b == Recovery::CompFailover)   ||
                 //(b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == Recovery::NodeSwitchover) ||
                 (b == Recovery::NodeFailover)   ||
                 (b == Recovery::NodeFailfast) )
            {
                return true;
            }
            break;
        }

        case Recovery::ClusterReset:
        default:
        {
            return false;
        }
    }

    return false;
}

  };

  

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
  {
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = CL_AMF_POLICY_PLUGIN_ID;
  SAFplus::api.pluginVersion    = CL_AMF_POLICY_PLUGIN_VER;

  SAFplus::api.policyId = SAFplus::AmfRedundancyPolicy::NplusM;

  // return it
  return (ClPlugin*) &SAFplus::api;
  }
