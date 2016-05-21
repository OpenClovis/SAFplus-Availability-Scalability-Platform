/* 
 * File ServiceGroup.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "EntityId.hxx"
#include "clMgtIdentifierList.hxx"
#include "ComponentRestart.hxx"
#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifier.hxx"
#include "ServiceUnitRestart.hxx"
#include <string>
#include "Application.hxx"
#include "clMgtProv.hxx"
#include "clMgtHistoryStat.hxx"
#include "ServiceInstance.hxx"
#include "ServiceUnit.hxx"
#include "SAFplusAmfCommon.hxx"
#include <cstdint>
#include <vector>
#include "AdministrativeState.hxx"
#include "ServiceGroup.hxx"

using namespace  std;
using namespace SAFplusAmf;

namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(ServiceGroup, /SAFplusAmf/safplusAmf/ServiceGroup)

    ServiceGroup::ServiceGroup(): adminState("adminState",::SAFplusAmf::AdministrativeState::on), autoRepair("autoRepair"), autoAdjust("autoAdjust",false), autoAdjustInterval("autoAdjustInterval"), preferredNumActiveServiceUnits("preferredNumActiveServiceUnits",1), preferredNumStandbyServiceUnits("preferredNumStandbyServiceUnits",1), preferredNumIdleServiceUnits("preferredNumIdleServiceUnits",0), maxActiveWorkAssignments("maxActiveWorkAssignments"), maxStandbyWorkAssignments("maxStandbyWorkAssignments"), serviceUnits("serviceUnits"), serviceInstances("serviceInstances"), application("application")
    {
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&autoRepair, "autoRepair");
        this->addChildObject(&autoAdjust, "autoAdjust");
        this->addChildObject(&autoAdjustInterval, "autoAdjustInterval");
        this->addChildObject(&preferredNumActiveServiceUnits, "preferredNumActiveServiceUnits");
        this->addChildObject(&preferredNumStandbyServiceUnits, "preferredNumStandbyServiceUnits");
        this->addChildObject(&preferredNumIdleServiceUnits, "preferredNumIdleServiceUnits");
        this->addChildObject(&maxActiveWorkAssignments, "maxActiveWorkAssignments");
        this->addChildObject(&maxStandbyWorkAssignments, "maxStandbyWorkAssignments");
        this->addChildObject(&serviceUnits, "serviceUnits");
        this->addChildObject(&serviceInstances, "serviceInstances");
        this->addChildObject(&application, "application");
        this->addChildObject(&componentRestart, "componentRestart");
        this->addChildObject(&serviceUnitRestart, "serviceUnitRestart");
        this->addChildObject(&numAssignedServiceUnits, "numAssignedServiceUnits");
        numAssignedServiceUnits.config = false;
        numAssignedServiceUnits.settable = false;
        numAssignedServiceUnits.loadDb = false;
        numAssignedServiceUnits.replicated = false;
        this->addChildObject(&numIdleServiceUnits, "numIdleServiceUnits");
        numIdleServiceUnits.config = false;
        numIdleServiceUnits.settable = false;
        numIdleServiceUnits.loadDb = false;
        numIdleServiceUnits.replicated = false;
        this->addChildObject(&numSpareServiceUnits, "numSpareServiceUnits");
        numSpareServiceUnits.config = false;
        numSpareServiceUnits.settable = false;
        numSpareServiceUnits.loadDb = false;
        numSpareServiceUnits.replicated = false;
        this->tag.assign("ServiceGroup");
        adminState = ::SAFplusAmf::AdministrativeState::on;
        autoAdjust = false;
        preferredNumActiveServiceUnits = 1;
        preferredNumStandbyServiceUnits = 1;
        preferredNumIdleServiceUnits = 0;
    };

    ServiceGroup::ServiceGroup(const std::string& nameValue): adminState("adminState",::SAFplusAmf::AdministrativeState::on), autoRepair("autoRepair"), autoAdjust("autoAdjust",false), autoAdjustInterval("autoAdjustInterval"), preferredNumActiveServiceUnits("preferredNumActiveServiceUnits",1), preferredNumStandbyServiceUnits("preferredNumStandbyServiceUnits",1), preferredNumIdleServiceUnits("preferredNumIdleServiceUnits",0), maxActiveWorkAssignments("maxActiveWorkAssignments"), maxStandbyWorkAssignments("maxStandbyWorkAssignments"), serviceUnits("serviceUnits"), serviceInstances("serviceInstances"), application("application")
    {
        this->name.value =  nameValue;
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&autoRepair, "autoRepair");
        this->addChildObject(&autoAdjust, "autoAdjust");
        this->addChildObject(&autoAdjustInterval, "autoAdjustInterval");
        this->addChildObject(&preferredNumActiveServiceUnits, "preferredNumActiveServiceUnits");
        this->addChildObject(&preferredNumStandbyServiceUnits, "preferredNumStandbyServiceUnits");
        this->addChildObject(&preferredNumIdleServiceUnits, "preferredNumIdleServiceUnits");
        this->addChildObject(&maxActiveWorkAssignments, "maxActiveWorkAssignments");
        this->addChildObject(&maxStandbyWorkAssignments, "maxStandbyWorkAssignments");
        this->addChildObject(&serviceUnits, "serviceUnits");
        this->addChildObject(&serviceInstances, "serviceInstances");
        this->addChildObject(&application, "application");
        this->addChildObject(&componentRestart, "componentRestart");
        this->addChildObject(&serviceUnitRestart, "serviceUnitRestart");
        this->addChildObject(&numAssignedServiceUnits, "numAssignedServiceUnits");
        numAssignedServiceUnits.config = false;
        numAssignedServiceUnits.settable = false;
        numAssignedServiceUnits.loadDb = false;
        numAssignedServiceUnits.replicated = false;
        this->addChildObject(&numIdleServiceUnits, "numIdleServiceUnits");
        numIdleServiceUnits.config = false;
        numIdleServiceUnits.settable = false;
        numIdleServiceUnits.loadDb = false;
        numIdleServiceUnits.replicated = false;
        this->addChildObject(&numSpareServiceUnits, "numSpareServiceUnits");
        numSpareServiceUnits.config = false;
        numSpareServiceUnits.settable = false;
        numSpareServiceUnits.loadDb = false;
        numSpareServiceUnits.replicated = false;
        this->tag.assign("ServiceGroup");
    };

    std::vector<std::string> ServiceGroup::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* ServiceGroup::getChildNames()
    {
        std::string childNames[] = { "name", "id", "adminState", "autoRepair", "autoAdjust", "autoAdjustInterval", "preferredNumActiveServiceUnits", "preferredNumStandbyServiceUnits", "preferredNumIdleServiceUnits", "maxActiveWorkAssignments", "maxStandbyWorkAssignments", "componentRestart", "serviceUnitRestart", "numAssignedServiceUnits", "numIdleServiceUnits", "numSpareServiceUnits", "serviceUnits", "serviceInstances", "application" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/adminState
     */
    ::SAFplusAmf::AdministrativeState ServiceGroup::getAdminState()
    {
        return this->adminState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/adminState
     */
    void ServiceGroup::setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->adminState.value = adminStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState>(&(adminState.value),adminStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoRepair
     */
    bool ServiceGroup::getAutoRepair()
    {
        return this->autoRepair.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoRepair
     */
    void ServiceGroup::setAutoRepair(bool autoRepairValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->autoRepair.value = autoRepairValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(autoRepair.value),autoRepairValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjust
     */
    bool ServiceGroup::getAutoAdjust()
    {
        return this->autoAdjust.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjust
     */
    void ServiceGroup::setAutoAdjust(bool autoAdjustValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->autoAdjust.value = autoAdjustValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(autoAdjust.value),autoAdjustValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjustInterval
     */
    SaTimeT ServiceGroup::getAutoAdjustInterval()
    {
        return this->autoAdjustInterval.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjustInterval
     */
    void ServiceGroup::setAutoAdjustInterval(SaTimeT &autoAdjustIntervalValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->autoAdjustInterval.value = autoAdjustIntervalValue;
        else
        {
            SAFplus::SimpleTxnOperation<SaTimeT> *opt = new SAFplus::SimpleTxnOperation<SaTimeT>(&(autoAdjustInterval.value),autoAdjustIntervalValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumActiveServiceUnits
     */
    ::uint32_t ServiceGroup::getPreferredNumActiveServiceUnits()
    {
        return this->preferredNumActiveServiceUnits.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumActiveServiceUnits
     */
    void ServiceGroup::setPreferredNumActiveServiceUnits(::uint32_t preferredNumActiveServiceUnitsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->preferredNumActiveServiceUnits.value = preferredNumActiveServiceUnitsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(preferredNumActiveServiceUnits.value),preferredNumActiveServiceUnitsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumStandbyServiceUnits
     */
    ::uint32_t ServiceGroup::getPreferredNumStandbyServiceUnits()
    {
        return this->preferredNumStandbyServiceUnits.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumStandbyServiceUnits
     */
    void ServiceGroup::setPreferredNumStandbyServiceUnits(::uint32_t preferredNumStandbyServiceUnitsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->preferredNumStandbyServiceUnits.value = preferredNumStandbyServiceUnitsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(preferredNumStandbyServiceUnits.value),preferredNumStandbyServiceUnitsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumIdleServiceUnits
     */
    ::uint32_t ServiceGroup::getPreferredNumIdleServiceUnits()
    {
        return this->preferredNumIdleServiceUnits.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumIdleServiceUnits
     */
    void ServiceGroup::setPreferredNumIdleServiceUnits(::uint32_t preferredNumIdleServiceUnitsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->preferredNumIdleServiceUnits.value = preferredNumIdleServiceUnitsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(preferredNumIdleServiceUnits.value),preferredNumIdleServiceUnitsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxActiveWorkAssignments
     */
    ::uint32_t ServiceGroup::getMaxActiveWorkAssignments()
    {
        return this->maxActiveWorkAssignments.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxActiveWorkAssignments
     */
    void ServiceGroup::setMaxActiveWorkAssignments(::uint32_t maxActiveWorkAssignmentsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxActiveWorkAssignments.value = maxActiveWorkAssignmentsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxActiveWorkAssignments.value),maxActiveWorkAssignmentsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxStandbyWorkAssignments
     */
    ::uint32_t ServiceGroup::getMaxStandbyWorkAssignments()
    {
        return this->maxStandbyWorkAssignments.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxStandbyWorkAssignments
     */
    void ServiceGroup::setMaxStandbyWorkAssignments(::uint32_t maxStandbyWorkAssignmentsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxStandbyWorkAssignments.value = maxStandbyWorkAssignmentsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxStandbyWorkAssignments.value),maxStandbyWorkAssignmentsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/application
     */
    ::SAFplusAmf::Application* ServiceGroup::getApplication()
    {
        return this->application.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/application
     */
    void ServiceGroup::setApplication(::SAFplusAmf::Application* applicationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->application.value = applicationValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::Application*> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::Application*>(&(application.value),applicationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/componentRestart
     */
    SAFplusAmf::ComponentRestart* ServiceGroup::getComponentRestart()
    {
        return dynamic_cast<ComponentRestart*>(this->getChildObject("componentRestart"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/componentRestart
     */
    void ServiceGroup::addComponentRestart(SAFplusAmf::ComponentRestart *componentRestartValue)
    {
        this->addChildObject(componentRestartValue, "componentRestart");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnitRestart
     */
    SAFplusAmf::ServiceUnitRestart* ServiceGroup::getServiceUnitRestart()
    {
        return dynamic_cast<ServiceUnitRestart*>(this->getChildObject("serviceUnitRestart"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnitRestart
     */
    void ServiceGroup::addServiceUnitRestart(SAFplusAmf::ServiceUnitRestart *serviceUnitRestartValue)
    {
        this->addChildObject(serviceUnitRestartValue, "serviceUnitRestart");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numAssignedServiceUnits
     */
    SAFplus::MgtHistoryStat<int>* ServiceGroup::getNumAssignedServiceUnits()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("numAssignedServiceUnits"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numAssignedServiceUnits
     */
    void ServiceGroup::addNumAssignedServiceUnits(SAFplus::MgtHistoryStat<int> *numAssignedServiceUnitsValue)
    {
        this->addChildObject(numAssignedServiceUnitsValue, "numAssignedServiceUnits");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numIdleServiceUnits
     */
    SAFplus::MgtHistoryStat<int>* ServiceGroup::getNumIdleServiceUnits()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("numIdleServiceUnits"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numIdleServiceUnits
     */
    void ServiceGroup::addNumIdleServiceUnits(SAFplus::MgtHistoryStat<int> *numIdleServiceUnitsValue)
    {
        this->addChildObject(numIdleServiceUnitsValue, "numIdleServiceUnits");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numSpareServiceUnits
     */
    SAFplus::MgtHistoryStat<int>* ServiceGroup::getNumSpareServiceUnits()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("numSpareServiceUnits"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numSpareServiceUnits
     */
    void ServiceGroup::addNumSpareServiceUnits(SAFplus::MgtHistoryStat<int> *numSpareServiceUnitsValue)
    {
        this->addChildObject(numSpareServiceUnitsValue, "numSpareServiceUnits");
    };

    ServiceGroup::~ServiceGroup()
    {
    };

}
/* namespace ::SAFplusAmf */
