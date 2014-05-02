#pragma once
#include <string>
#include <clMgtObject.hxx>

namespace SAFplusAmf
{
  class ComponentServiceInstance;
  class ServiceInstance;
  class Component;
  class ServiceGroup;
  class ServiceUnit;
  
  
  void demarshall(const std::string& obj,SAFplus::ClMgtObject* context, ComponentServiceInstance*& result);
  void demarshall(const std::string& obj,SAFplus::ClMgtObject* context, ServiceInstance*& result);
  void demarshall(const std::string& obj,SAFplus::ClMgtObject* context, Component*& result);
  void demarshall(const std::string& obj,SAFplus::ClMgtObject* context, ServiceGroup*& result);
  void demarshall(const std::string& obj,SAFplus::ClMgtObject* context, ServiceUnit*& result);
};


