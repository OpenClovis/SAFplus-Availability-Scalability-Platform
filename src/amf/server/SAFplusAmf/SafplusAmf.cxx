/* 
 * File SafplusAmf.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "EntityByIdKey.hxx"
#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtList.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "SafplusAmf.hxx"


namespace SAFplusAmf
  {

    SafplusAmf::SafplusAmf(): SAFplus::MgtContainer("safplusAmf"), healthCheckPeriod("healthCheckPeriod",SaTimeT(0)), healthCheckMaxSilence("healthCheckMaxSilence",SaTimeT(0)), clusterList("Cluster"), nodeList("Node"), serviceGroupList("ServiceGroup"), componentList("Component"), componentServiceInstanceList("ComponentServiceInstance"), serviceInstanceList("ServiceInstance"), serviceUnitList("ServiceUnit"), applicationList("Application"), entityByNameList("EntityByName"), entityByIdList("EntityById")
    {
        this->addChildObject(&healthCheckPeriod, "healthCheckPeriod");
        this->addChildObject(&healthCheckMaxSilence, "healthCheckMaxSilence");
        this->addChildObject(&clusterList, "Cluster");
        this->addChildObject(&nodeList, "Node");
        this->addChildObject(&serviceGroupList, "ServiceGroup");
        this->addChildObject(&componentList, "Component");
        this->addChildObject(&componentServiceInstanceList, "ComponentServiceInstance");
        this->addChildObject(&serviceInstanceList, "ServiceInstance");
        this->addChildObject(&serviceUnitList, "ServiceUnit");
        this->addChildObject(&applicationList, "Application");
        this->addChildObject(&entityByNameList, "EntityByName");
        this->addChildObject(&entityByIdList, "EntityById");
        clusterList.childXpath="/SAFplusAmf/safplusAmf/Cluster";
        clusterList.setListKey("name");
        nodeList.childXpath="/SAFplusAmf/safplusAmf/Node";
        nodeList.setListKey("name");
        serviceGroupList.childXpath="/SAFplusAmf/safplusAmf/ServiceGroup";
        serviceGroupList.setListKey("name");
        componentList.childXpath="/SAFplusAmf/safplusAmf/Component";
        componentList.setListKey("name");
        componentServiceInstanceList.childXpath="/SAFplusAmf/safplusAmf/ComponentServiceInstance";
        componentServiceInstanceList.setListKey("name");
        serviceInstanceList.childXpath="/SAFplusAmf/safplusAmf/ServiceInstance";
        serviceInstanceList.setListKey("name");
        serviceUnitList.childXpath="/SAFplusAmf/safplusAmf/ServiceUnit";
        serviceUnitList.setListKey("name");
        applicationList.childXpath="/SAFplusAmf/safplusAmf/Application";
        applicationList.setListKey("name");
        entityByNameList.childXpath="/SAFplusAmf/safplusAmf/EntityByName";
        entityByNameList.setListKey("name");
        entityByIdList.childXpath="/SAFplusAmf/safplusAmf/EntityById";
        entityByIdList.setListKey("id");
        healthCheckPeriod = SaTimeT(0);
        healthCheckMaxSilence = SaTimeT(0);
    };

    std::vector<std::string>* SafplusAmf::getChildNames()
    {
        std::string childNames[] = { "Cluster", "Node", "ServiceGroup", "Component", "ComponentServiceInstance", "ServiceInstance", "ServiceUnit", "Application", "EntityByName", "EntityById", "healthCheckPeriod", "healthCheckMaxSilence" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/healthCheckPeriod
     */
    SaTimeT SafplusAmf::getHealthCheckPeriod()
    {
        return this->healthCheckPeriod.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/healthCheckPeriod
     */
    void SafplusAmf::setHealthCheckPeriod(SaTimeT &healthCheckPeriodValue, SAFplus::Transaction &txn)
    {
        this->healthCheckPeriod.set(healthCheckPeriodValue,txn);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/healthCheckMaxSilence
     */
    SaTimeT SafplusAmf::getHealthCheckMaxSilence()
    {
        return this->healthCheckMaxSilence.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/healthCheckMaxSilence
     */
    void SafplusAmf::setHealthCheckMaxSilence(SaTimeT &healthCheckMaxSilenceValue, SAFplus::Transaction &txn)
    {
        this->healthCheckMaxSilence.set(healthCheckMaxSilenceValue,txn);
    };

    SafplusAmf::~SafplusAmf()
    {
    };

}
/* namespace ::SAFplusAmf */
