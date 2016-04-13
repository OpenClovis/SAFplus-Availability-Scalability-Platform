/* 
 * File ServiceGroup.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef SERVICEGROUP_HXX_
#define SERVICEGROUP_HXX_

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

namespace SAFplusAmf
  {

    class ServiceGroup : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(ServiceGroup);

    public:

        /*
         * Does the operator want this entity to be off, idle, or in service?
         */
        SAFplus::MgtProv<::SAFplusAmf::AdministrativeState> adminState;

        /*
         * Automatically attempt to bring this entity back into a healthy state if its operational state becomes disabled.  A 'false' value will cause the system to wait for operator intervention (via the repair API) before attempting to restart this entity.
         */
        SAFplus::MgtProv<bool> autoRepair;

        /*
         * Match this service group as closely as possible to the preferred high availability configuration.  For example, if the preferred active comes online, 'fail-back' to it.  Another example is if a new work assignment is provisioned, the system could remove an existing standby assignment so the new active can be provisioned.
         */
        SAFplus::MgtProv<bool> autoAdjust;

        /*
         * The time between checks to see if adjustment is needed.
         */
        SAFplus::MgtProv<SaTimeT> autoAdjustInterval;

        /*
         * What is the optimal number of active Service Units for this Service Group?
         */
        SAFplus::MgtProv<::uint32_t> preferredNumActiveServiceUnits;

        /*
         * What is the optimal number of standby Service Units for this Service Group?
         */
        SAFplus::MgtProv<::uint32_t> preferredNumStandbyServiceUnits;

        /*
         * An idle service unit is running but is not assigned active or standby.  This concept is functionally equivalent to the saAmfSGNumPrefInserviceSUs since Active+Standby+Idle = Inservice
         */
        SAFplus::MgtProv<::uint32_t> preferredNumIdleServiceUnits;

        /*
         * The maximum number of active work assignments that can be placed on a single service unit (and therefore component/process) simultaneously.
         */
        SAFplus::MgtProv<::uint32_t> maxActiveWorkAssignments;

        /*
         * The maximum number of standby work assignments that can be placed on a single service unit (and therefore component/process) simultaneously.
         */
        SAFplus::MgtProv<::uint32_t> maxStandbyWorkAssignments;

        /*
         * Service Units in this Service Group
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ServiceUnit*> serviceUnits;

        /*
         * Service Instances (work) in this Service group
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ServiceInstance*> serviceInstances;
        SAFplus::MgtIdentifier<::SAFplusAmf::Application*> application;
        SAFplusAmf::ComponentRestart componentRestart;
        SAFplusAmf::ServiceUnitRestart serviceUnitRestart;
        SAFplus::MgtHistoryStat<int> numAssignedServiceUnits;
        SAFplus::MgtHistoryStat<int> numIdleServiceUnits;
        SAFplus::MgtHistoryStat<int> numSpareServiceUnits;

    public:
        ServiceGroup();
        ServiceGroup(const std::string& nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/adminState
         */
        ::SAFplusAmf::AdministrativeState getAdminState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/adminState
         */
        void setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoRepair
         */
        bool getAutoRepair();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoRepair
         */
        void setAutoRepair(bool autoRepairValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjust
         */
        bool getAutoAdjust();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjust
         */
        void setAutoAdjust(bool autoAdjustValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjustInterval
         */
        SaTimeT getAutoAdjustInterval();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/autoAdjustInterval
         */
        void setAutoAdjustInterval(SaTimeT &autoAdjustIntervalValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumActiveServiceUnits
         */
        ::uint32_t getPreferredNumActiveServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumActiveServiceUnits
         */
        void setPreferredNumActiveServiceUnits(::uint32_t preferredNumActiveServiceUnitsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumStandbyServiceUnits
         */
        ::uint32_t getPreferredNumStandbyServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumStandbyServiceUnits
         */
        void setPreferredNumStandbyServiceUnits(::uint32_t preferredNumStandbyServiceUnitsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumIdleServiceUnits
         */
        ::uint32_t getPreferredNumIdleServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/preferredNumIdleServiceUnits
         */
        void setPreferredNumIdleServiceUnits(::uint32_t preferredNumIdleServiceUnitsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxActiveWorkAssignments
         */
        ::uint32_t getMaxActiveWorkAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxActiveWorkAssignments
         */
        void setMaxActiveWorkAssignments(::uint32_t maxActiveWorkAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxStandbyWorkAssignments
         */
        ::uint32_t getMaxStandbyWorkAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/maxStandbyWorkAssignments
         */
        void setMaxStandbyWorkAssignments(::uint32_t maxStandbyWorkAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnits
         */
        std::vector<::SAFplusAmf::ServiceUnit*> getServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnits
         */
        void setServiceUnits(::SAFplusAmf::ServiceUnit* serviceUnitsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceInstances
         */
        std::vector<::SAFplusAmf::ServiceInstance*> getServiceInstances();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceInstances
         */
        void setServiceInstances(::SAFplusAmf::ServiceInstance* serviceInstancesValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/application
         */
        ::SAFplusAmf::Application* getApplication();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/application
         */
        void setApplication(::SAFplusAmf::Application* applicationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/componentRestart
         */
        SAFplusAmf::ComponentRestart* getComponentRestart();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/componentRestart
         */
        void addComponentRestart(SAFplusAmf::ComponentRestart *componentRestartValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnitRestart
         */
        SAFplusAmf::ServiceUnitRestart* getServiceUnitRestart();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/serviceUnitRestart
         */
        void addServiceUnitRestart(SAFplusAmf::ServiceUnitRestart *serviceUnitRestartValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numAssignedServiceUnits
         */
        SAFplus::MgtHistoryStat<int>* getNumAssignedServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numAssignedServiceUnits
         */
        void addNumAssignedServiceUnits(SAFplus::MgtHistoryStat<int> *numAssignedServiceUnitsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numIdleServiceUnits
         */
        SAFplus::MgtHistoryStat<int>* getNumIdleServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numIdleServiceUnits
         */
        void addNumIdleServiceUnits(SAFplus::MgtHistoryStat<int> *numIdleServiceUnitsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numSpareServiceUnits
         */
        SAFplus::MgtHistoryStat<int>* getNumSpareServiceUnits();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceGroup/numSpareServiceUnits
         */
        void addNumSpareServiceUnits(SAFplus::MgtHistoryStat<int> *numSpareServiceUnitsValue);
        ~ServiceGroup();

    };
}
/* namespace ::SAFplusAmf */
#endif /* SERVICEGROUP_HXX_ */
