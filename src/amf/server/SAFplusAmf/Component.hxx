/* Replace to src/amf/server/SAFplusAmf */
/* 
 * File Component.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef COMPONENT_HXX_
#define COMPONENT_HXX_

#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include <string>
#include "Recovery.hxx"
#include <cstdint>
#include "Date.hxx"
#include "PresenceState.hxx"
#include "CompProperty.hxx"
#include "Cleanup.hxx"
#include "clMgtHistoryStat.hxx"
#include "ProcStats.hxx"
#include "Instantiate.hxx"
#include "EntityId.hxx"
#include "clMgtProvList.hxx"
#include "HighAvailabilityReadinessState.hxx"
#include "Terminate.hxx"
#include "ServiceUnit.hxx"
#include "ReadinessState.hxx"
#include "Timeouts.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "HighAvailabilityState.hxx"
#include "PendingOperation.hxx"
#include "clMgtIdentifier.hxx"
#include "clMgtProv.hxx"
#include "CapabilityModel.hxx"

namespace SAFplusAmf
  {

    class Component : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(Component);

    public:
        SAFplus::MgtProv<::SAFplusAmf::PresenceState> presenceState;

        SAFplus::MgtProv<::SAFplusAmf::CompProperty> compProperty;

        /*
         * This is defined by the SA-Forum AMF B.04.01 specification section 3.5.  Options allowing just 1 assignment are a subset of the x assignment options.  To limit the capability to 1 assignment, change the saAmfCompNumMaxActiveCSIs and saAmfCompNumMaxStandbyCSIs fields.  Options disallowing standby assignments are implemented by setting saAmfCompNumMaxStandbyCSIs.  Choose 'non-pre-instantiable', saAmfCompNumMaxStandbyCSIs=0 for applications that are not SAF-aware
         */
        SAFplus::MgtProv<::SAFplusAmf::CapabilityModel> capabilityModel;

        /*
         * Maximum number of active work assignments this component can handle.
         */
        SAFplus::MgtProv<::uint32_t> maxActiveAssignments;

	/*
         * Indicate when start this compoent
         */
        SAFplus::MgtProv<::uint32_t> instantiateLevel;
        /*
         * Maximum number of standby work assignments this component can handle.
         */
        SAFplus::MgtProv<::uint32_t> maxStandbyAssignments;

        /*
         * Currently assigned work.
         */
        SAFplus::MgtProvList<std::string> assignedWork;
        //SAFplus::MgtIdentifierList<::SAFplusAmf::Component*> activeComponents; //this is the right declaration

        /*
         * True is enabled, False is disabled.  To move from False to True a 'repair' action must occur.
         */
        SAFplus::MgtProv<bool> operState;
        SAFplus::MgtProv<::SAFplusAmf::ReadinessState> readinessState;

        /*
         * This state field covers ALL work assignments.  If this field is set to notReadyForAssignment then SAFplus will not assign work.  So the application can call saAmfHAReadinessStateSet() upon startup (or any other time) to enable or disable work assignments.
         */
        SAFplus::MgtProv<::SAFplusAmf::HighAvailabilityReadinessState> haReadinessState;
        SAFplus::MgtProv<::SAFplusAmf::HighAvailabilityState> haState;

        /*
         * Compatible SA-Forum API version
         */
        SAFplus::MgtProv<std::string> safVersion;

        /*
         * Information about the type of component, as defined in sec 7.4.8 of AMF-B.04.01.  This is C type: SaAmfCompCategoryT, and its value is set based on other configuration (namely the component capabilities and whether a proxy is defined)
         */
        SAFplus::MgtProv<::uint32_t> compCategory;

        /*
         * What software installation bundle does this component come from
         */
        SAFplus::MgtProv<std::string> swBundle;

        /*
         * List of environment variables in the form '<VARIABLE>=<VALUE>
<VARIABLE2>=<VALUE2>
' the form the environment in which this component should be started
         */
        SAFplus::MgtProvList<std::string> commandEnvironment;

        /*
         * How many times to attempt to instantiate this entity without delay.  If the number of instantiation attempts exceeds both this and the max delayed instantiations field, the fault will be elevated to the Service Unit level.
         */
        SAFplus::MgtProv<::uint32_t> maxInstantInstantiations;

        /*
         * How many times to attempt to instantiate this entity after an initial delay.  If the number of instantiation attempts exceeds both this and the max instant instantiations field, the fault will be elevated to the Service Unit level.
         */
        SAFplus::MgtProv<::uint32_t> maxDelayedInstantiations;

        /*
         * The number of times this application has been attempted to be instantiated
         */
        SAFplus::MgtProv<::uint32_t> numInstantiationAttempts;

        /*
         * If this component remains instantiated for this length of time (in milliseconds), the component is deemed to be successfully instantiated and the numInstantiationAttempts field is zeroed.
         */
        SAFplus::MgtProv<::uint32_t> instantiationSuccessDuration;

        /*
         * The last time an instantiation attempt occurred
         */
        SAFplus::MgtProv<SAFplusTypes::Date> lastInstantiation;

        /*
         * How long to delay between instantiation attempts
         */
        SAFplus::MgtProv<::uint32_t> delayBetweenInstantiation;
        SAFplus::MgtIdentifier<ServiceUnit*> serviceUnit;        
        //recovery in config file
        SAFplus::MgtProv<::SAFplusAmf::Recovery> recovery;
        //status of recovery
        SAFplus::MgtProv<::SAFplusAmf::Recovery> currentRecovery;
        /*
         * Set to true if this component can be restarted on failure, without this event registering as a fault
         */
        SAFplus::MgtProv<bool> restartable;

        /*
         * The component listed here is this component's proxy.
         */
        SAFplus::MgtIdentifier<Component*> proxy;
        //SAFplus::MgtProv<std::string> proxy;

        SAFplus::MgtProvList<std::string> csiTypes;
        SAFplus::MgtProv<std::string> proxyCSI;

        /*
         * This component is the proxy for the components listed here.
         */
        //SAFplus::MgtProvList<std::string> proxied;
        SAFplus::MgtIdentifierList<::SAFplusAmf::Component*> proxied;
         
        /*
        Am I launched?        
        */
        bool launched; // this is for proxied component only

        /*
         * The process id of this component (if it is instantiated as a process), or 0
         */
        SAFplus::MgtProv<::int32_t> processId;

        /*
         * The last error generated by operations on this component
         */
        SAFplus::MgtProv<std::string> lastError;

        /*
         * The system is currently attempting the following operation on the component
         */
        SAFplus::MgtProv<::SAFplusAmf::PendingOperation> pendingOperation;

        /*
         * When the system will give up on the pending operation, in milliseconds since the epoch
         */
        SAFplus::MgtProv<SAFplusTypes::Date> pendingOperationExpiration;
        SAFplusAmf::ProcStats procStats;
        SAFplus::MgtHistoryStat<int> activeAssignments;
        SAFplus::MgtHistoryStat<int> standbyAssignments;
        SAFplusAmf::Instantiate instantiate;
        SAFplusAmf::Terminate terminate;
        SAFplusAmf::Cleanup cleanup;
        SAFplusAmf::Timeouts timeouts;
        SAFplus::MgtHistoryStat<int> restartCount;

    public:
        Component();
        Component(const std::string& nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/presenceState
         */
        ::SAFplusAmf::PresenceState getPresenceState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/presenceState
         */
        void setPresenceState(::SAFplusAmf::PresenceState &presenceStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/capabilityModel
         */
        ::SAFplusAmf::CapabilityModel getCapabilityModel();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/capabilityModel
         */
        void setCapabilityModel(::SAFplusAmf::CapabilityModel &capabilityModelValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxActiveAssignments
         */
        ::uint32_t getMaxActiveAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxActiveAssignments
         */
        void setMaxActiveAssignments(::uint32_t maxActiveAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

	/*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiateLevel
         */
        ::uint32_t getInstantiateLevel();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiateLevel
         */
        void setInstantiateLevel(::uint32_t instantiateLevelValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxStandbyAssignments
         */
        ::uint32_t getMaxStandbyAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxStandbyAssignments
         */
        void setMaxStandbyAssignments(::uint32_t maxStandbyAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/operState
         */
        bool getOperState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/operState
         */
        void setOperState(bool operStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/readinessState
         */
        ::SAFplusAmf::ReadinessState getReadinessState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/readinessState
         */
        void setReadinessState(::SAFplusAmf::ReadinessState &readinessStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/haReadinessState
         */
        ::SAFplusAmf::HighAvailabilityReadinessState getHaReadinessState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/haReadinessState
         */
        void setHaReadinessState(::SAFplusAmf::HighAvailabilityReadinessState &haReadinessStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/haState
         */
        ::SAFplusAmf::HighAvailabilityState getHaState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/haState
         */
        void setHaState(::SAFplusAmf::HighAvailabilityState &haStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/safVersion
         */
        std::string getSafVersion();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/safVersion
         */
        void setSafVersion(std::string safVersionValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/compCategory
         */
        ::uint32_t getCompCategory();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/compCategory
         */
        void setCompCategory(::uint32_t compCategoryValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/swBundle
         */
        std::string getSwBundle();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/swBundle
         */
        void setSwBundle(std::string swBundleValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxInstantInstantiations
         */
        ::uint32_t getMaxInstantInstantiations();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxInstantInstantiations
         */
        void setMaxInstantInstantiations(::uint32_t maxInstantInstantiationsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxDelayedInstantiations
         */
        ::uint32_t getMaxDelayedInstantiations();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/maxDelayedInstantiations
         */
        void setMaxDelayedInstantiations(::uint32_t maxDelayedInstantiationsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/numInstantiationAttempts
         */
        ::uint32_t getNumInstantiationAttempts();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/numInstantiationAttempts
         */
        void setNumInstantiationAttempts(::uint32_t numInstantiationAttemptsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiationSuccessDuration
         */
        ::uint32_t getInstantiationSuccessDuration();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiationSuccessDuration
         */
        void setInstantiationSuccessDuration(::uint32_t instantiationSuccessDurationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/lastInstantiation
         */
        SAFplusTypes::Date getLastInstantiation();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/lastInstantiation
         */
        void setLastInstantiation(SAFplusTypes::Date &lastInstantiationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/delayBetweenInstantiation
         */
        ::uint32_t getDelayBetweenInstantiation();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/delayBetweenInstantiation
         */
        void setDelayBetweenInstantiation(::uint32_t delayBetweenInstantiationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/serviceUnit
         */
        ServiceUnit* getServiceUnit();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/serviceUnit
         */
        void setServiceUnit(ServiceUnit* serviceUnitValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/NonSafComponent/proxy
         */
        Component* getProxy();

        /*
         * XPATH: XPATH: /SAFplusAmf/safplusAmf/NonSafComponent/proxy
         */
        void setProxy(Component* proxyValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/recovery
         */
        ::SAFplusAmf::Recovery getRecovery();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/recovery
         */
        void setRecovery(::SAFplusAmf::Recovery &recoveryValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/restartable
         */
        bool getRestartable();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/restartable
         */
        void setRestartable(bool restartableValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/proxy
         */
        //std::string getProxy();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/proxy
         */
        //void setProxy(std::string proxyValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/processId
         */
        ::int32_t getProcessId();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/processId
         */
        void setProcessId(::int32_t processIdValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/lastError
         */
        std::string getLastError();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/lastError
         */
        void setLastError(std::string lastErrorValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperation
         */
        ::SAFplusAmf::PendingOperation getPendingOperation();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperation
         */
        void setPendingOperation(::SAFplusAmf::PendingOperation &pendingOperationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperationExpiration
         */
        SAFplusTypes::Date getPendingOperationExpiration();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperationExpiration
         */
        void setPendingOperationExpiration(SAFplusTypes::Date &pendingOperationExpirationValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/procStats
         */
        SAFplusAmf::ProcStats* getProcStats();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/procStats
         */
        void addProcStats(SAFplusAmf::ProcStats *procStatsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/activeAssignments
         */
        SAFplus::MgtHistoryStat<int>* getActiveAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/activeAssignments
         */
        void addActiveAssignments(SAFplus::MgtHistoryStat<int> *activeAssignmentsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/standbyAssignments
         */
        SAFplus::MgtHistoryStat<int>* getStandbyAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/standbyAssignments
         */
        void addStandbyAssignments(SAFplus::MgtHistoryStat<int> *standbyAssignmentsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiate
         */
        SAFplusAmf::Instantiate* getInstantiate();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/instantiate
         */
        void addInstantiate(SAFplusAmf::Instantiate *instantiateValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/terminate
         */
        SAFplusAmf::Terminate* getTerminate();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/terminate
         */
        void addTerminate(SAFplusAmf::Terminate *terminateValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/cleanup
         */
        SAFplusAmf::Cleanup* getCleanup();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/cleanup
         */
        void addCleanup(SAFplusAmf::Cleanup *cleanupValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts
         */
        SAFplusAmf::Timeouts* getTimeouts();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts
         */
        void addTimeouts(SAFplusAmf::Timeouts *timeoutsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/restartCount
         */
        SAFplus::MgtHistoryStat<int>* getRestartCount();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Component/restartCount
         */
        void addRestartCount(SAFplus::MgtHistoryStat<int> *restartCountValue);
        ~Component();

    };
}
/* namespace ::SAFplusAmf */
#endif /* COMPONENT_HXX_ */
