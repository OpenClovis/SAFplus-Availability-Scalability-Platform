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
    ret->Name = nam;  // TBD: ctor should set
    ret->id = getAmfId();
    ret->adminState.Value = adminState;
    ret->autoRepair = autoRepair;
    ret->failFastOnInstantiationFailure = failFastOnInstantiationFailure;
    ret->failFastOnCleanupFailure = failFastOnCleanupFailure;
    return ret;
    }


  ServiceGroup* createServiceGroup(const char* nam, const SAFplusAmf::AdministrativeState& adminState, bool autoRepair, bool autoAdjust, SaTimeT autoAdjustInterval,unsigned int preferredNumActiveServiceUnits,unsigned int preferredNumStandbyServiceUnits,unsigned int preferredNumIdleServiceUnits,unsigned int maxActiveWorkAssignments,unsigned int maxStandbyWorkAssignments )
    {
    ServiceGroup* ret = new ServiceGroup(nam);
    ret->Name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.Value                = adminState;
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


  ServiceInstance* createServiceInstance(const char* nam, const SAFplusAmf::AdministrativeState& adminState, int rank)
    {
    ServiceInstance* ret = new ServiceInstance(nam);
    ret->Name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.Value                = adminState;
    ret->rank                            = rank;
    return ret;
    }

  ComponentServiceInstance* createComponentServiceInstance(const char* nam)
    {
    ComponentServiceInstance* ret = new ComponentServiceInstance(nam);
    ret->Name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    return ret;
    }


  ServiceUnit* createServiceUnit(const char* nam, const SAFplusAmf::AdministrativeState& adminState, int rank, bool failover)
    {
    ServiceUnit* ret = new ServiceUnit(nam);
    ret->Name = nam;  // TBD: ctor should set
    ret->id                              = getAmfId();
    ret->adminState.Value                = adminState;
    ret->rank                            = rank;
    ret->failover                        = failover;
    return ret;
    }


  Component* createComponent(const char* nam, SAFplusAmf::CapabilityModel capabilityModel,unsigned int maxActiveAssignments,unsigned int maxStandbyAssignments,std::string safVersion, unsigned int compCategory,const std::string& swBundle,const std::string& env,unsigned int maxInstantInstantiations,unsigned int maxDelayedInstantiations,unsigned int delayBetweenInstantiation,SAFplusAmf::Recovery recovery,bool restartable,const std::string& proxy,const std::string& proxied)
    {
    Component* ret = new Component(nam);
    ret->Name = nam;  // TBD: ctor should set
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
    return ret;
    }

  void createTestDataSet(SAFplusAmfRoot* self)
    {
    SaTimeT st = 0;
    //SAFplusAmf::AdministrativeState as;
    //as.Value = 2;
    Node* node[2];
    node[0]  = createNode("ctrl0",SAFplusAmf::AdministrativeState::on,true,false,false);
    ServiceGroup* sg = createServiceGroup("sg0",SAFplusAmf::AdministrativeState::on,true,false,st,1,1,1,1,1);
    Component* comp[2];
    comp[0] = createComponent("c0",SAFplusAmf::CapabilityModel::x_active_or_y_standby,1,1,"B.01.01",1,"testBundle.tgz","TEST_ENV=1\nTEST_ENV2=2",2,2,2000,SAFplusAmf::Recovery::Restart,true,"","");
    //comp[1] = createComponent("c1",SAFplusAmf::CapabilityModel::x_active_or_y_standby,1,1,"B.01.01",1,"testBundle.tgz","TEST_ENV=1\nTEST_ENV2=2",2,2,2000,SAFplusAmf::Recovery::Restart,true,"","");
    ServiceUnit* su[2];
    su[0] = createServiceUnit("su0", SAFplusAmf::AdministrativeState::on,0,true);
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
    node[0]->serviceUnits.addChildObject(su[0]);
    sg->serviceUnits.addChildObject(su[0]);
    su[0]->components.addChildObject(comp[0]);
    
    sg->serviceInstances.addChildObject(si);
    si->serviceGroup.Value = sg;
   
    csi->serviceInstance.Value = si;
    si->componentServiceInstances.addChildObject(csi);    
    }

  void deXMLize(const std::string& obj,ClMgtObject* context, ComponentServiceInstance*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, ServiceInstance*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, Component*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, ServiceGroup*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, ServiceUnit*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, Application*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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

  void deXMLize(const std::string& obj,ClMgtObject* context, Node*& result)
    {
    SAFplusAmfRoot* root = (SAFplusAmfRoot*) context->root();
    assert(root);
    ClMgtObject* found = root->deepFind(obj);
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
 
 
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Node*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ComponentServiceInstance*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceInstance*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);    
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Component*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceGroup*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceUnit*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Application*& result)
    {
    // TODO: Implement directly for efficiency
    deXMLize(std::string(obj),context,result);   
    }


  }
