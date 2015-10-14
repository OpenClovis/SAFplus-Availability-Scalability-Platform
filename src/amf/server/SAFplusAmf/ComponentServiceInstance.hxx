/* 
 * File ComponentServiceInstance.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef COMPONENTSERVICEINSTANCE_HXX_
#define COMPONENTSERVICEINSTANCE_HXX_

#include "EntityId.hxx"
#include "clMgtIdentifier.hxx"
#include "MgtFactory.hxx"
#include "clMgtProvList.hxx"
#include "Component.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifierList.hxx"
#include <string>
#include "clMgtList.hxx"
#include "ServiceInstance.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>

namespace SAFplusAmf
  {

    class ComponentServiceInstance : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(ComponentServiceInstance);

    public:

        /*
         * A protection group for a specific component service instance is the group of components to which the component service instance has been assigned
         */
        SAFplus::MgtProvList<std::string> protectionGroup;

        /*
         * 
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ComponentServiceInstance*> dependencies;
        SAFplus::MgtIdentifier<ServiceInstance*> serviceInstance;

        /*
         * This work is assigned standby to these components
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::Component*> standbyComponents;

        /*
         * This work is assigned active to these components
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::Component*> activeComponents;

        /*
         * Arbitrary data that defines the work needed to be done.
         */
        SAFplus::MgtList<std::string> dataList;

    public:
        ComponentServiceInstance();
        ComponentServiceInstance(std::string nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/protectionGroup
         */
        std::vector<std::string> getProtectionGroup();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/protectionGroup
         */
        void setProtectionGroup(std::string protectionGroupValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/dependencies
         */
        std::vector<::SAFplusAmf::ComponentServiceInstance*> getDependencies();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/dependencies
         */
        void setDependencies(::SAFplusAmf::ComponentServiceInstance* dependenciesValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/serviceInstance
         */
        ServiceInstance* getServiceInstance();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/serviceInstance
         */
        void setServiceInstance(ServiceInstance* serviceInstanceValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/standbyComponents
         */
        std::vector<::SAFplusAmf::Component*> getStandbyComponents();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/standbyComponents
         */
        void setStandbyComponents(::SAFplusAmf::Component* standbyComponentsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/activeComponents
         */
        std::vector<::SAFplusAmf::Component*> getActiveComponents();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ComponentServiceInstance/activeComponents
         */
        void setActiveComponents(::SAFplusAmf::Component* activeComponentsValue);
        ~ComponentServiceInstance();

    };
}
/* namespace ::SAFplusAmf */
#endif /* COMPONENTSERVICEINSTANCE_HXX_ */
