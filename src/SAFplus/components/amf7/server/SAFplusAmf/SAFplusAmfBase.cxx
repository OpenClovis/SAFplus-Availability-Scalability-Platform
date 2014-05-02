#include "SAFplusAmfCommon.hxx"
#include "SAFplusAmf.hxx"
#include "ComponentServiceInstance.hxx"
#include "ServiceInstance.hxx"
#include "Component.hxx"
#include "ServiceGroup.hxx"

using namespace SAFplusAmf;
using namespace SAFplus;


namespace SAFplusAmf
{
void demarshall(const std::string& obj,ClMgtObject* context, ComponentServiceInstance*& result)
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
      root->componentServiceInstanceList.addEntry(csi);
      root->entityByNameList.addEntry(csi);
      result = csi;
    }
  
}

void demarshall(const std::string& obj,ClMgtObject* context, ServiceInstance*& result)
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
  else  // I have to create a placeholder.  The real one will be demarshalled onto this one eventually
    {
      ServiceInstance* elem;
      elem = new ServiceInstance(obj);
      root->serviceInstanceList.addEntry(elem);
      root->entityByNameList.addEntry(elem);
      result = elem;
    }
  
}

void demarshall(const std::string& obj,ClMgtObject* context, Component*& result)
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
  else  // I have to create a placeholder.  The real one will be demarshalled onto this one eventually
    {
      Component* elem;
      elem = new Component(obj);
      root->componentList.addEntry(elem);
      root->entityByNameList.addEntry(elem);
      result = elem;
    } 
}

void demarshall(const std::string& obj,ClMgtObject* context, ServiceGroup*& result)
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
  else  // I have to create a placeholder.  The real one will be demarshalled onto this one eventually
    {
      ServiceGroup* elem;
      elem = new ServiceGroup(obj);
      root->serviceGroupList.addEntry(elem);
      root->entityByNameList.addEntry(elem);
      result = elem;
    } 
}

 void demarshall(const std::string& obj,ClMgtObject* context, ServiceUnit*& result)
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
  else  // I have to create a placeholder.  The real one will be demarshalled onto this one eventually
    {
      ServiceUnit* elem;
      elem = new ServiceUnit(obj);
      root->serviceUnitList.addEntry(elem);
      root->entityByNameList.addEntry(elem);
      result = elem;
    } 
}
 


}
