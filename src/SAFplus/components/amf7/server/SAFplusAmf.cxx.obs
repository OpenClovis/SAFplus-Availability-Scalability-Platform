#include "SAFplusAmf.hxx"
#include "Node.hxx"
//using namespace SAFplus;
using namespace SAFplusAmf;
#include <clMgtObject.hxx>

SAFplusAmfRoot::SAFplusAmfRoot():ClMgtObject("SAFplusAmf"),
                                 clusterList("cluster"), nodeList("node"),serviceGroupList("serviceGroup"),componentList("component"),componentServiceInstanceList("componentServiceInstance"),serviceInstanceList("serviceInstance"),serviceUnitList("serviceUnit"),applicationList("application"),entityByNameList("entityByName"),entityByIdList("entityById")
{
  this->addChildObject(&clusterList,"cluster");
  this->addChildObject(&nodeList,"node");
  this->addChildObject(&serviceGroupList,"serviceGroup");
  this->addChildObject(&componentList,"component");
  this->addChildObject(&componentServiceInstanceList,"componentServiceInstance");
  this->addChildObject(&serviceInstanceList,"serviceInstance");
  this->addChildObject(&serviceUnitList,"serviceUnit");
  this->addChildObject(&applicationList,"application");

  this->addChildObject(&entityByNameList,"entityByName");
  this->addChildObject(&entityByIdList,"entityById");
}

unsigned short int amfId = 1;

unsigned short int getAmfId()
{
  //GAS TODO: thread safety
  unsigned short int ret = amfId;
  amfId++;
  return ret;
}

Node* createNode(const char* name, const SAFplusAmf::AdministrativeState& adminState, bool autoRepair, bool failFastOnInstantiationFailure, bool failFastOnCleanupFailure)
{
  Node* ret = new Node(name);
  ret->id = getAmfId();
  ret->adminState.Value = adminState;
  ret->autoRepair = autoRepair;
  ret->failFastOnInstantiationFailure = failFastOnInstantiationFailure;
  ret->failFastOnCleanupFailure = failFastOnCleanupFailure;
  return ret;
}


ServiceGroup* createServiceGroup(const char* name, const SAFplusAmf::AdministrativeState& adminState, bool autoRepair, bool autoAdjust,SAFplusTypes::SaTimeT autoAdjustInterval,unsigned int preferredNumActiveServiceUnits,unsigned int preferredNumStandbyServiceUnits,unsigned int preferredNumIdleServiceUnits,unsigned int maxActiveWorkAssignments,unsigned int maxStandbyWorkAssignments )
{
  ServiceGroup* ret = new ServiceGroup(name);
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

ServiceGroup* createComponent(const char* name, int capabilityModel,unsigned int maxActiveAssignments,unsigned int maxStandbyAssignments,std::string safVersion, unsigned int compCategory,const std::string& swBundle,const std::string& env,unsigned int maxInstantInstantiations,unsigned int maxDelayedInstantiations,unsigned int delayBetweenInstantiation,int recovery,bool restartable,const std::string& proxy,const std::string& proxied)
{
  
}

void SAFplusAmfRoot::load(ClMgtDatabase *db)
{
#if 1  // For Testing:  By hand initialization:

  //SAFplusAmf::AdministrativeState as;
  //as.Value = 2;
  Node* node = createNode("ctrl0",SAFplusAmf::AdministrativeState::on,true,false,false);
  


#else  // Actually load from DB
#endif
}
