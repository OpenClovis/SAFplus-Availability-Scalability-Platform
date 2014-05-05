#pragma once
#include <string>
#include <clMgtObject.hxx>


namespace SAFplusAmf
  {
  class Application;
  class ComponentServiceInstance;
  class ServiceInstance;
  class Component;
  class ServiceGroup;
  class ServiceUnit;
  class Node;  
    
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, ComponentServiceInstance*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, ServiceInstance*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, Component*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, ServiceGroup*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, ServiceUnit*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, Application*& result);
  void deXMLize(const std::string& obj,SAFplus::ClMgtObject* context, Node*& result);

  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ComponentServiceInstance*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceInstance*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Component*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceGroup*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, ServiceUnit*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Application*& result);
  void deXMLize(const char* obj,SAFplus::ClMgtObject* context, Node*& result);

  };

