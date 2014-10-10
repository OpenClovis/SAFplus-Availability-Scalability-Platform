#include "SAFplusAmfCommon.hxx"
#include "SAFplusAmf.hxx"
#include "ComponentServiceInstance.hxx"
#include "ServiceInstance.hxx"
#include "Component.hxx"
#include "ServiceGroup.hxx"
#include "Node.hxx"

using namespace SAFplusAmf;
using namespace SAFplus;


namespace SAFplusAmf
  {
  unsigned short int amfId = 1;

  unsigned short int getAmfId()
    {
    //GAS TODO: thread safety
    unsigned short int ret = amfId;
    amfId++;
    return ret;
    }

  Node* createNode(const char* nam, const SAFplusAmf::AdministrativeState& adminState, bool autoRepair, bool failFastOnInstantiationFailure, bool failFastOnCleanupFailure)
    {
    Node* ret = new Node(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id = getAmfId();
    ret->adminState.value = adminState;
    ret->autoRepair = autoRepair;
    ret->failFastOnInstantiationFailure = failFastOnInstantiationFailure;
    ret->failFastOnCleanupFailure = failFastOnCleanupFailure;
    ret->operState.value = true;  // created ready to run...

    return ret;
    }


  ServiceGroup* createServiceGroup(const char* nam, const SAFplusAmf::AdministrativeState& adminState, bool autoRepair, bool autoAdjust, SaTimeT autoAdjustInterval,unsigned int preferredNumActiveServiceUnits,unsigned int preferredNumStandbyServiceUnits,unsigned int preferredNumIdleServiceUnits,unsigned int maxActiveWorkAssignments,unsigned int maxStandbyWorkAssignments )
    {
    ServiceGroup* ret = new ServiceGroup(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.value                = adminState;
    ret->autoRepair                      = autoRepair;
    ret->autoAdjust                      = autoAdjust;
    ret->autoAdjustInterval              = autoAdjustInterval;
    ret->preferredNumActiveServiceUnits  = preferredNumActiveServiceUnits;
    ret->preferredNumStandbyServiceUnits = preferredNumStandbyServiceUnits;
    ret->preferredNumIdleServiceUnits    = preferredNumIdleServiceUnits;
    ret->maxActiveWorkAssignments        = maxActiveWorkAssignments;
    ret->maxStandbyWorkAssignments       = maxStandbyWorkAssignments;

    return ret;
    }


  ServiceInstance* createServiceInstance(const char* nam, const SAFplusAmf::AdministrativeState& adminState, int rank,int actives=1, int standbys=1)
    {
    ServiceInstance* ret = new ServiceInstance(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.value                = adminState;
    ret->rank                            = rank;
    ret->addStandbyAssignments(new StandbyAssignments());
    ret->addActiveAssignments(new ActiveAssignments());
    ret->preferredActiveAssignments      = actives;
    ret->preferredStandbyAssignments      = standbys;
    return ret;
    }

  ComponentServiceInstance* createComponentServiceInstance(const char* nam)
    {
    ComponentServiceInstance* ret = new ComponentServiceInstance(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    return ret;
    }


  ServiceUnit* createServiceUnit(const char* nam, const SAFplusAmf::AdministrativeState& adminState, int rank, bool failover)
    {
    ServiceUnit* ret = new ServiceUnit(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.value                = adminState;
    ret->rank                            = rank;
    ret->failover                        = failover;
    ret->preinstantiable                 = true;

    // Initial condition values
    ret->haReadinessState                = HighAvailabilityReadinessState::notReadyForAssignment;
    ret->haState                         = HighAvailabilityState::idle;
    ret->presenceState                   = PresenceState::uninstantiated;
    ret->readinessState                  = ReadinessState::outOfService;
    ret->operState                       = true; // created ready to run...
    return ret;
    }


  Component* createComponent(const char* nam, SAFplusAmf::CapabilityModel capabilityModel,unsigned int maxActiveAssignments,unsigned int maxStandbyAssignments,std::string safVersion, unsigned int compCategory,const std::string& swBundle,const std::string& env,unsigned int maxInstantInstantiations,unsigned int maxDelayedInstantiations,unsigned int delayBetweenInstantiation,SAFplusAmf::Recovery recovery,bool restartable,const std::string& proxy,const std::string& proxied)
    {
    Component* ret = new Component(nam);
    ret->name = nam;  // TBD: ctor should set
    ret->id = getAmfId();
    ret->capabilityModel = capabilityModel;
    ret->maxActiveAssignments = maxActiveAssignments;
    ret->maxStandbyAssignments = maxStandbyAssignments;
    ret->safVersion = safVersion;
    ret->compCategory = compCategory;
    ret->swBundle = swBundle;
    // GAS TODO: ClMgtProvList needs code accessors: ret->commandEnvironment = env;
    ret->maxInstantInstantiations = maxInstantInstantiations;
    ret->maxDelayedInstantiations = maxDelayedInstantiations;
    ret->delayBetweenInstantiation = delayBetweenInstantiation;
    ret->recovery = recovery;
    ret->restartable = restartable;
    ret->proxy = proxy;
    // GAS TODO: ClMgtProvList needs code accessors: ret->proxied =
    // proxied;

    // Initial condition values
    ret->operState.value = true;  // created ready to run...
    ret->numInstantiationAttempts.value = 0;
    ret->lastInstantiation.value.value = 0;
    ret->presence.value = PresenceState::uninstantiated;

    return ret;
    }

  void createTestDataSet(SAFplusAmfRoot* self)
    {
    SaTimeT st = 0;
    //SAFplusAmf::AdministrativeState as;
    //as.value = 2;
    Node* node[2];
    node[0]  = createNode("sc0",SAFplusAmf::AdministrativeState::on,true,false,false);
    node[1]  = createNode("sc1",SAFplusAmf::AdministrativeState::on,true,false,false);
    ServiceGroup* sg = createServiceGroup("sg0",SAFplusAmf::AdministrativeState::on,true,false,st,1,1,1,1,1);
    Component* comp[2];
    comp[0] = createComponent("c0",SAFplusAmf::CapabilityModel::x_active_or_y_standby,1,1,"B.01.01",1,"testBundle.tgz","TEST_ENV=1\nTEST_ENV2=2",1,1,10000,SAFplusAmf::Recovery::Restart,true,"","");
    Instantiate* inst = new Instantiate();
    inst->command.value = "./test0 arg1 arg2";
    inst->timeout.value = 30000;
    comp[0]->addChildObject(inst);

    comp[1] = createComponent("c1",SAFplusAmf::CapabilityModel::x_active_or_y_standby,1,1,"B.01.01",1,"testBundle.tgz","TEST_ENV=1\nTEST_ENV2=2",0,2,20000,SAFplusAmf::Recovery::Restart,true,"","");
    Instantiate* inst1 = new Instantiate();
    inst1->command.value = "./test1 arg1 arg2";
    inst1->timeout.value = 30000;
    comp[1]->addChildObject(inst);


    ServiceUnit* su[2];
    su[0] = createServiceUnit("su0", SAFplusAmf::AdministrativeState::on,0,true);
    su[1] = createServiceUnit("su1", SAFplusAmf::AdministrativeState::on,0,true);
    //su[1] = createServiceUnit(const char* name, const SAFplusAmf::AdministrativeState& adminState, int rank, bool failover);
    ServiceInstance* si;
    si = createServiceInstance("si", SAFplusAmf::AdministrativeState::on,0);

    ComponentServiceInstance* csi;
    csi = createComponentServiceInstance("csi");

    // Put the elements in their type-lookup arrays
    self->nodeList.addChildObject(node[0]);
    self->serviceGroupList.addChildObject(sg);
    self->componentList.addChildObject(comp[0]);
    self->serviceInstanceList.addChildObject(si);
    self->componentServiceInstanceList.addChildObject(csi);

    // Connect the elements
    node[0]->serviceUnits.value.push_back(su[0]);
    sg->serviceUnits.value.push_back(su[0]);
    su[0]->components.value.push_back(comp[0]);
    sg->serviceInstances.value.push_back(si);
    si->serviceGroup.value = sg;
    comp[0]->serviceUnit.value = su[0];
    su[0]->node.value = node[0];
    su[0]->serviceGroup.value = sg;
    //su[1]->node.value = node[1];
    csi->serviceInstance.value = si;
    si->componentServiceInstances.value.push_back(csi);    

#if 1
    // Handle the secondary elements
    self->nodeList.addChildObject(node[1]);
    self->componentList.addChildObject(comp[1]);
    //self->componentServiceInstanceList.addChildObject(csi);
    // connect the secondary elements
    node[1]->serviceUnits.value.push_back(su[1]);
    sg->serviceUnits.value.push_back(su[1]);
    su[1]->components.value.push_back(comp[1]);

    comp[1]->serviceUnit.value = su[1];
    su[1]->node.value = node[1];
    su[1]->serviceGroup.value = sg;
#endif
    }

  void deXMLize(const std::string& obj,MgtObject* context, ComponentServiceInstance*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (ComponentServiceInstance*) found;
      // verify that it really is a CSI
      return;
      }
    else  // I have to create a placeholder.  The real one will be destored onto this one.
      {
      ComponentServiceInstance* csi;
      csi = new ComponentServiceInstance(obj);
      root->componentServiceInstanceList.addChildObject(csi);
      root->entityByNameList.addChildObject(csi);
      result = csi;
      }
  
    }

  void deXMLize(const std::string& obj,MgtObject* context, ServiceInstance*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (ServiceInstance*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      ServiceInstance* elem;
      elem = new ServiceInstance(obj);
      root->serviceInstanceList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      }
  
    }

  void deXMLize(const std::string& obj,MgtObject* context, Component*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (Component*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      Component* elem;
      elem = new Component(obj);
      root->componentList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      } 
    }

  void deXMLize(const std::string& obj,MgtObject* context, ServiceGroup*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (ServiceGroup*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      ServiceGroup* elem;
      elem = new ServiceGroup(obj);
      root->serviceGroupList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      } 
    }

  void deXMLize(const std::string& obj,MgtObject* context, ServiceUnit*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (ServiceUnit*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      ServiceUnit* elem;
      elem = new ServiceUnit(obj);
      root->serviceUnitList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      } 
    }

  void deXMLize(const std::string& obj,MgtObject* context, Application*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (Application*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      Application* elem;
      elem = new Application(obj);
      root->applicationList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      } 
    }

  void deXMLize(const std::string& obj,MgtObject* context, Node*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    MgtObject* found = root->deepFind(obj);
    if (found)
      {
      result = (Node*) found;
      // verify that it really is a SI
      return;
      }
    else  // I have to create a placeholder.  The real one will be deXMLizeed onto this one eventually
      {
      Node* elem;
      elem = new Node(obj);
      root->nodeList.addChildObject(elem);
      root->entityByNameList.addChildObject(elem);
      result = elem;
      } 
    }
 
 
  void deXMLize(const char* obj,SAFplus::MgtObject* context, Node*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, ComponentServiceInstance*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceInstance*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);    
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, Component*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceGroup*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceUnit*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::MgtObject* context, Application*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }


  }
