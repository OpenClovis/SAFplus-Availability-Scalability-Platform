#pragma once
#include <string>
#include <clMgtObject.hxx>
#include <clMgtList.hxx>

namespace SAFplusAmf
  {
  class Application;
  class ComponentServiceInstance;
  class ServiceInstance;
  class Component;
  class ServiceGroup;
  class ServiceUnit;
  class Node;

  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, ComponentServiceInstance*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, ServiceInstance*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, Component*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, ServiceGroup*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, ServiceUnit*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, Application*& result);
  void deXMLize(const std::string& obj,SAFplus::MgtObject* context, Node*& result);

  void deXMLize(const char* obj,SAFplus::MgtObject* context, ComponentServiceInstance*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceInstance*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, Component*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceGroup*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, ServiceUnit*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, Application*& result);
  void deXMLize(const char* obj,SAFplus::MgtObject* context, Node*& result);

  };

