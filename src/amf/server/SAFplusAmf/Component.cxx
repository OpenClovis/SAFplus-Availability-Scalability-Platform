/* 
 * File Component.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include <string>
#include "Recovery.hxx"
#include <cstdint>
#include "Date.hxx"
#include "PresenceState.hxx"
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
#include "Component.hxx"

using namespace SAFplusTypes;
using namespace  std;
using namespace SAFplusAmf;

namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(Component, /SAFplusAmf/safplusAmf/Component)

    Component::Component(): presenceState("presenceState",::SAFplusAmf::PresenceState::uninstantiated), capabilityModel("capabilityModel"), maxActiveAssignments("maxActiveAssignments",1), maxStandbyAssignments("maxStandbyAssignments",1), assignedWork("assignedWork"), operState("operState",true), readinessState("readinessState",::SAFplusAmf::ReadinessState::outOfService), haReadinessState("haReadinessState",::SAFplusAmf::HighAvailabilityReadinessState::readyForAssignment), haState("haState",::SAFplusAmf::HighAvailabilityState::idle), safVersion("safVersion",std::string("B.04.01")), compCategory("compCategory"), swBundle("swBundle"), commandEnvironment("commandEnvironment"), maxInstantInstantiations("maxInstantInstantiations",1), maxDelayedInstantiations("maxDelayedInstantiations",1), numInstantiationAttempts("numInstantiationAttempts"), instantiationSuccessDuration("instantiationSuccessDuration",30000), lastInstantiation("lastInstantiation"), delayBetweenInstantiation("delayBetweenInstantiation",10000), serviceUnit("serviceUnit"), recovery("recovery",Recovery::NoRecommendation), currentRecovery("currentRecovery", Recovery::None), restartable("restartable",true), proxy("proxy"), proxied("proxied"), processId("processId",0), lastError("lastError"), pendingOperation("pendingOperation"), pendingOperationExpiration("pendingOperationExpiration",Date(0)), csiType("csiType")
    {
        this->addChildObject(&presenceState, "presenceState");
        // We allow presenceState to be stored and read to/from database to do the failover, comment the followings:
        //presenceState.config = true;
        //presenceState.settable = true;
        //presenceState.loadDb = true;
        //presenceState.replicated = true;
        this->addChildObject(&capabilityModel, "capabilityModel");
        this->addChildObject(&maxActiveAssignments, "maxActiveAssignments");
        this->addChildObject(&maxStandbyAssignments, "maxStandbyAssignments");
        this->addChildObject(&assignedWork, "assignedWork");
        assignedWork.config = false;
        assignedWork.settable = false;
        assignedWork.loadDb = false;
        assignedWork.replicated = false;
        this->addChildObject(&operState, "operState");
        operState.config = false;
        operState.settable = true;
        operState.loadDb = false;
        operState.replicated = false;
        this->addChildObject(&readinessState, "readinessState");
        readinessState.config = false;
        readinessState.settable = false;
        readinessState.loadDb = false;
        readinessState.replicated = false;
        this->addChildObject(&haReadinessState, "haReadinessState");
        haReadinessState.config = false;
        haReadinessState.settable = false;
        haReadinessState.loadDb = false;
        haReadinessState.replicated = false;
        this->addChildObject(&haState, "haState");
        haState.config = false;
        haState.settable = false;
        haState.loadDb = false;
        haState.replicated = false;
        this->addChildObject(&safVersion, "safVersion");
        safVersion.config = false;
        safVersion.settable = false;
        safVersion.loadDb = false;
        safVersion.replicated = false;
        this->addChildObject(&compCategory, "compCategory");
        compCategory.config = false;
        compCategory.settable = false;
        compCategory.loadDb = false;
        compCategory.replicated = false;
        this->addChildObject(&csiType, "csiType");
        this->addChildObject(&swBundle, "swBundle");
        swBundle.config = false;
        swBundle.settable = false;
        swBundle.loadDb = false;
        swBundle.replicated = false;
        this->addChildObject(&commandEnvironment, "commandEnvironment");
        this->addChildObject(&maxInstantInstantiations, "maxInstantInstantiations");
        this->addChildObject(&maxDelayedInstantiations, "maxDelayedInstantiations");
        this->addChildObject(&numInstantiationAttempts, "numInstantiationAttempts");
        numInstantiationAttempts.config = false;
        numInstantiationAttempts.settable = false;
        numInstantiationAttempts.loadDb = false;
        numInstantiationAttempts.replicated = false;
        this->addChildObject(&instantiationSuccessDuration, "instantiationSuccessDuration");
        this->addChildObject(&lastInstantiation, "lastInstantiation");
        lastInstantiation.config = false;
        lastInstantiation.settable = false;
        lastInstantiation.loadDb = false;
        lastInstantiation.replicated = false;
        this->addChildObject(&delayBetweenInstantiation, "delayBetweenInstantiation");
        this->addChildObject(&serviceUnit, "serviceUnit");
        this->addChildObject(&recovery, "recovery");
        this->addChildObject(&currentRecovery, "currentRecovery");
        this->addChildObject(&restartable, "restartable");
        this->addChildObject(&proxy, "proxy");
        this->addChildObject(&proxied, "proxied");
        this->addChildObject(&processId, "processId");
        // We allow processId to be stored and read to/from database to do the failover, comment the followings:
        //processId.config = true;
        //processId.settable = true;
        //processId.loadDb = true;
        //processId.replicated = true;
        this->addChildObject(&lastError, "lastError");
        lastError.config = false;
        lastError.settable = false;
        lastError.loadDb = false;
        lastError.replicated = false;
        this->addChildObject(&pendingOperation, "pendingOperation");
        pendingOperation.config = false;
        pendingOperation.settable = false;
        pendingOperation.loadDb = false;
        pendingOperation.replicated = false;
        this->addChildObject(&pendingOperationExpiration, "pendingOperationExpiration");
        pendingOperationExpiration.config = false;
        pendingOperationExpiration.settable = false;
        pendingOperationExpiration.loadDb = false;
        pendingOperationExpiration.replicated = false;
        this->addChildObject(&procStats, "procStats");
        procStats.config = false;
        procStats.settable = false;
        procStats.loadDb = false;
        procStats.replicated = false;
        this->addChildObject(&activeAssignments, "activeAssignments");
        activeAssignments.config = false;
        activeAssignments.settable = false;
        activeAssignments.loadDb = false;
        activeAssignments.replicated = false;
        this->addChildObject(&standbyAssignments, "standbyAssignments");
        standbyAssignments.config = false;
        standbyAssignments.settable = false;
        standbyAssignments.loadDb = false;
        standbyAssignments.replicated = false;
        this->addChildObject(&instantiate, "instantiate");
        this->addChildObject(&terminate, "terminate");
        this->addChildObject(&cleanup, "cleanup");
        this->addChildObject(&timeouts, "timeouts");
        this->addChildObject(&restartCount, "restartCount");
        restartCount.config = false;
        restartCount.settable = false;
        restartCount.loadDb = false;
        restartCount.replicated = false;
        this->tag.assign("Component");
        presenceState = ::SAFplusAmf::PresenceState::uninstantiated;
        maxActiveAssignments = 1;
        maxStandbyAssignments = 1;
        operState = true;
        readinessState = ::SAFplusAmf::ReadinessState::outOfService;
        haReadinessState = ::SAFplusAmf::HighAvailabilityReadinessState::readyForAssignment;
        haState = ::SAFplusAmf::HighAvailabilityState::idle;
        safVersion = std::string("B.04.01");
        maxInstantInstantiations = 1;
        maxDelayedInstantiations = 1;
        instantiationSuccessDuration = 30000;
        delayBetweenInstantiation = 10000;
        restartable = true;
        processId = 0;
        pendingOperationExpiration = Date(0);
    };

    Component::Component(const std::string& nameValue): presenceState("presenceState",::SAFplusAmf::PresenceState::uninstantiated), capabilityModel("capabilityModel"), maxActiveAssignments("maxActiveAssignments",1), maxStandbyAssignments("maxStandbyAssignments",1), assignedWork("assignedWork"), operState("operState",true), readinessState("readinessState",::SAFplusAmf::ReadinessState::outOfService), haReadinessState("haReadinessState",::SAFplusAmf::HighAvailabilityReadinessState::readyForAssignment), haState("haState",::SAFplusAmf::HighAvailabilityState::idle), safVersion("safVersion",std::string("B.04.01")), compCategory("compCategory"), swBundle("swBundle"), commandEnvironment("commandEnvironment"), maxInstantInstantiations("maxInstantInstantiations",1), maxDelayedInstantiations("maxDelayedInstantiations",1), numInstantiationAttempts("numInstantiationAttempts"), instantiationSuccessDuration("instantiationSuccessDuration",30000), lastInstantiation("lastInstantiation"), delayBetweenInstantiation("delayBetweenInstantiation",10000), serviceUnit("serviceUnit"), recovery("recovery",SAFplusAmf::Recovery::NoRecommendation), currentRecovery("currentRecovery", Recovery::None), restartable("restartable",true), proxy("proxy"), proxied("proxied"), processId("processId",0), lastError("lastError"), pendingOperation("pendingOperation"), pendingOperationExpiration("pendingOperationExpiration",Date(0)), csiType("csiType")
    {
        this->name.value =  nameValue;
        this->addChildObject(&presenceState, "presenceState");
        // We allow presenceState to be stored and read to/from database to do the failover, comment the followings:
        //presenceState.config = false;
        //presenceState.settable = false;
        //presenceState.loadDb = false;
        //presenceState.replicated = false;
        this->addChildObject(&capabilityModel, "capabilityModel");
        this->addChildObject(&maxActiveAssignments, "maxActiveAssignments");
        this->addChildObject(&maxStandbyAssignments, "maxStandbyAssignments");
        this->addChildObject(&assignedWork, "assignedWork");
        assignedWork.config = false;
        assignedWork.settable = false;
        assignedWork.loadDb = false;
        assignedWork.replicated = false;
        this->addChildObject(&operState, "operState");
        operState.config = false;
        operState.settable = true;
        operState.loadDb = false;
        operState.replicated = false;
        this->addChildObject(&readinessState, "readinessState");
        readinessState.config = false;
        readinessState.settable = false;
        readinessState.loadDb = false;
        readinessState.replicated = false;
        this->addChildObject(&haReadinessState, "haReadinessState");
        haReadinessState.config = false;
        haReadinessState.settable = false;
        haReadinessState.loadDb = false;
        haReadinessState.replicated = false;
        this->addChildObject(&haState, "haState");
        haState.config = false;
        haState.settable = false;
        haState.loadDb = false;
        haState.replicated = false;
        this->addChildObject(&safVersion, "safVersion");
        safVersion.config = false;
        safVersion.settable = false;
        safVersion.loadDb = false;
        safVersion.replicated = false;
        this->addChildObject(&compCategory, "compCategory");
        compCategory.config = false;
        compCategory.settable = false;
        compCategory.loadDb = false;
        compCategory.replicated = false;
        this->addChildObject(&csiType, "csiType");
        this->addChildObject(&swBundle, "swBundle");
        swBundle.config = false;
        swBundle.settable = false;
        swBundle.loadDb = false;
        swBundle.replicated = false;
        this->addChildObject(&commandEnvironment, "commandEnvironment");
        this->addChildObject(&maxInstantInstantiations, "maxInstantInstantiations");
        this->addChildObject(&maxDelayedInstantiations, "maxDelayedInstantiations");
        this->addChildObject(&numInstantiationAttempts, "numInstantiationAttempts");
        numInstantiationAttempts.config = false;
        numInstantiationAttempts.settable = false;
        numInstantiationAttempts.loadDb = false;
        numInstantiationAttempts.replicated = false;
        this->addChildObject(&instantiationSuccessDuration, "instantiationSuccessDuration");
        this->addChildObject(&lastInstantiation, "lastInstantiation");
        lastInstantiation.config = false;
        lastInstantiation.settable = false;
        lastInstantiation.loadDb = false;
        lastInstantiation.replicated = false;
        this->addChildObject(&delayBetweenInstantiation, "delayBetweenInstantiation");
        this->addChildObject(&serviceUnit, "serviceUnit");
        this->addChildObject(&recovery, "recovery");
        this->addChildObject(&currentRecovery, "currentRecovery");
        this->addChildObject(&restartable, "restartable");
        this->addChildObject(&proxy, "proxy");
        this->addChildObject(&proxied, "proxied");
        this->addChildObject(&processId, "processId");
        // We allow processId to be stored and read to/from database to do the failover, comment the followings:
        //processId.config = false;
        //processId.settable = false;
        //processId.loadDb = false;
        //processId.replicated = true;
        this->addChildObject(&lastError, "lastError");
        lastError.config = false;
        lastError.settable = false;
        lastError.loadDb = false;
        lastError.replicated = false;
        this->addChildObject(&pendingOperation, "pendingOperation");
        pendingOperation.config = false;
        pendingOperation.settable = false;
        pendingOperation.loadDb = false;
        pendingOperation.replicated = false;
        this->addChildObject(&pendingOperationExpiration, "pendingOperationExpiration");
        pendingOperationExpiration.config = false;
        pendingOperationExpiration.settable = false;
        pendingOperationExpiration.loadDb = false;
        pendingOperationExpiration.replicated = false;
        this->addChildObject(&procStats, "procStats");
        procStats.config = false;
        procStats.settable = false;
        procStats.loadDb = false;
        procStats.replicated = false;
        this->addChildObject(&activeAssignments, "activeAssignments");
        activeAssignments.config = false;
        activeAssignments.settable = false;
        activeAssignments.loadDb = false;
        activeAssignments.replicated = false;
        this->addChildObject(&standbyAssignments, "standbyAssignments");
        standbyAssignments.config = false;
        standbyAssignments.settable = false;
        standbyAssignments.loadDb = false;
        standbyAssignments.replicated = false;
        this->addChildObject(&instantiate, "instantiate");
        this->addChildObject(&terminate, "terminate");
        this->addChildObject(&cleanup, "cleanup");
        this->addChildObject(&timeouts, "timeouts");
        this->addChildObject(&restartCount, "restartCount");
        restartCount.config = false;
        restartCount.settable = false;
        restartCount.loadDb = false;
        restartCount.replicated = false;
        this->tag.assign("Component");
    };

    std::vector<std::string> Component::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* Component::getChildNames()
    {
        std::string childNames[] = { "name", "id", "procStats", "presenceState", "capabilityModel", "maxActiveAssignments", "maxStandbyAssignments", "activeAssignments", "standbyAssignments", "assignedWork", "operState", "readinessState", "haReadinessState", "haState", "safVersion", "compCategory", "swBundle", "commandEnvironment", "instantiate", "terminate", "cleanup", "maxInstantInstantiations", "maxDelayedInstantiations", "numInstantiationAttempts", "instantiationSuccessDuration", "lastInstantiation", "delayBetweenInstantiation", "timeouts", "serviceUnit", "recovery", "restartable", "restartCount", "proxy", "proxied", "processId", "lastError", "pendingOperation", "pendingOperationExpiration", "csiType" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/presenceState
     */
    ::SAFplusAmf::PresenceState Component::getPresenceState()
    {
        return this->presenceState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/presenceState
     */
    void Component::setPresenceState(::SAFplusAmf::PresenceState &presenceStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->presenceState.value = presenceStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState>(&(presenceState.value),presenceStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/capabilityModel
     */
    ::SAFplusAmf::CapabilityModel Component::getCapabilityModel()
    {
        return this->capabilityModel.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/capabilityModel
     */
    void Component::setCapabilityModel(::SAFplusAmf::CapabilityModel &capabilityModelValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->capabilityModel.value = capabilityModelValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::CapabilityModel> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::CapabilityModel>(&(capabilityModel.value),capabilityModelValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxActiveAssignments
     */
    ::uint32_t Component::getMaxActiveAssignments()
    {
        return this->maxActiveAssignments.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxActiveAssignments
     */
    void Component::setMaxActiveAssignments(::uint32_t maxActiveAssignmentsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxActiveAssignments.value = maxActiveAssignmentsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxActiveAssignments.value),maxActiveAssignmentsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxStandbyAssignments
     */
    ::uint32_t Component::getMaxStandbyAssignments()
    {
        return this->maxStandbyAssignments.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxStandbyAssignments
     */
    void Component::setMaxStandbyAssignments(::uint32_t maxStandbyAssignmentsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxStandbyAssignments.value = maxStandbyAssignmentsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxStandbyAssignments.value),maxStandbyAssignmentsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/operState
     */
    bool Component::getOperState()
    {
        return this->operState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/operState
     */
    void Component::setOperState(bool operStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->operState.value = operStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(operState.value),operStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/readinessState
     */
    ::SAFplusAmf::ReadinessState Component::getReadinessState()
    {
        return this->readinessState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/readinessState
     */
    void Component::setReadinessState(::SAFplusAmf::ReadinessState &readinessStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->readinessState.value = readinessStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::ReadinessState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::ReadinessState>(&(readinessState.value),readinessStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/haReadinessState
     */
    ::SAFplusAmf::HighAvailabilityReadinessState Component::getHaReadinessState()
    {
        return this->haReadinessState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/haReadinessState
     */
    void Component::setHaReadinessState(::SAFplusAmf::HighAvailabilityReadinessState &haReadinessStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->haReadinessState.value = haReadinessStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityReadinessState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityReadinessState>(&(haReadinessState.value),haReadinessStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/haState
     */
    ::SAFplusAmf::HighAvailabilityState Component::getHaState()
    {
        return this->haState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/haState
     */
    void Component::setHaState(::SAFplusAmf::HighAvailabilityState &haStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->haState.value = haStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityState>(&(haState.value),haStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/safVersion
     */
    std::string Component::getSafVersion()
    {
        return this->safVersion.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/safVersion
     */
    void Component::setSafVersion(std::string safVersionValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->safVersion.value = safVersionValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(safVersion.value),safVersionValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/compCategory
     */
    ::uint32_t Component::getCompCategory()
    {
        return this->compCategory.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/compCategory
     */
    void Component::setCompCategory(::uint32_t compCategoryValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->compCategory.value = compCategoryValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(compCategory.value),compCategoryValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/swBundle
     */
    std::string Component::getSwBundle()
    {
        return this->swBundle.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/swBundle
     */
    void Component::setSwBundle(std::string swBundleValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->swBundle.value = swBundleValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(swBundle.value),swBundleValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxInstantInstantiations
     */
    ::uint32_t Component::getMaxInstantInstantiations()
    {
        return this->maxInstantInstantiations.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxInstantInstantiations
     */
    void Component::setMaxInstantInstantiations(::uint32_t maxInstantInstantiationsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxInstantInstantiations.value = maxInstantInstantiationsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxInstantInstantiations.value),maxInstantInstantiationsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxDelayedInstantiations
     */
    ::uint32_t Component::getMaxDelayedInstantiations()
    {
        return this->maxDelayedInstantiations.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/maxDelayedInstantiations
     */
    void Component::setMaxDelayedInstantiations(::uint32_t maxDelayedInstantiationsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->maxDelayedInstantiations.value = maxDelayedInstantiationsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(maxDelayedInstantiations.value),maxDelayedInstantiationsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/numInstantiationAttempts
     */
    ::uint32_t Component::getNumInstantiationAttempts()
    {
        return this->numInstantiationAttempts.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/numInstantiationAttempts
     */
    void Component::setNumInstantiationAttempts(::uint32_t numInstantiationAttemptsValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->numInstantiationAttempts.value = numInstantiationAttemptsValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(numInstantiationAttempts.value),numInstantiationAttemptsValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/instantiationSuccessDuration
     */
    ::uint32_t Component::getInstantiationSuccessDuration()
    {
        return this->instantiationSuccessDuration.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/instantiationSuccessDuration
     */
    void Component::setInstantiationSuccessDuration(::uint32_t instantiationSuccessDurationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->instantiationSuccessDuration.value = instantiationSuccessDurationValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(instantiationSuccessDuration.value),instantiationSuccessDurationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/lastInstantiation
     */
    SAFplusTypes::Date Component::getLastInstantiation()
    {
        return this->lastInstantiation.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/lastInstantiation
     */
    void Component::setLastInstantiation(SAFplusTypes::Date &lastInstantiationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->lastInstantiation.value = lastInstantiationValue;
        else
        {
            SAFplus::SimpleTxnOperation<SAFplusTypes::Date> *opt = new SAFplus::SimpleTxnOperation<SAFplusTypes::Date>(&(lastInstantiation.value),lastInstantiationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/delayBetweenInstantiation
     */
    ::uint32_t Component::getDelayBetweenInstantiation()
    {
        return this->delayBetweenInstantiation.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/delayBetweenInstantiation
     */
    void Component::setDelayBetweenInstantiation(::uint32_t delayBetweenInstantiationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->delayBetweenInstantiation.value = delayBetweenInstantiationValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(delayBetweenInstantiation.value),delayBetweenInstantiationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/serviceUnit
     */
    ServiceUnit* Component::getServiceUnit()
    {
        return this->serviceUnit.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/serviceUnit
     */
    void Component::setServiceUnit(ServiceUnit* serviceUnitValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->serviceUnit.value = serviceUnitValue;
        else
        {
            SAFplus::SimpleTxnOperation<ServiceUnit*> *opt = new SAFplus::SimpleTxnOperation<ServiceUnit*>(&(serviceUnit.value),serviceUnitValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/recovery
     */
    ::SAFplusAmf::Recovery Component::getRecovery()
    {
        return this->recovery.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/recovery
     */
    void Component::setRecovery(::SAFplusAmf::Recovery &recoveryValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->recovery.value = recoveryValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::Recovery> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::Recovery>(&(recovery.value),recoveryValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/restartable
     */
    bool Component::getRestartable()
    {
        return this->restartable.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/restartable
     */
    void Component::setRestartable(bool restartableValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->restartable.value = restartableValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(restartable.value),restartableValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/proxy
     */
    std::string Component::getProxy()
    {
        return this->proxy.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/proxy
     */
    void Component::setProxy(std::string proxyValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->proxy.value = proxyValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(proxy.value),proxyValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/processId
     */
    ::int32_t Component::getProcessId()
    {
        return this->processId.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/processId
     */
    void Component::setProcessId(::int32_t processIdValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->processId.value = processIdValue;
        else
        {
            SAFplus::SimpleTxnOperation<::int32_t> *opt = new SAFplus::SimpleTxnOperation<::int32_t>(&(processId.value),processIdValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/lastError
     */
    std::string Component::getLastError()
    {
        return this->lastError.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/lastError
     */
    void Component::setLastError(std::string lastErrorValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->lastError.value = lastErrorValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(lastError.value),lastErrorValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperation
     */
    ::SAFplusAmf::PendingOperation Component::getPendingOperation()
    {
        return this->pendingOperation.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperation
     */
    void Component::setPendingOperation(::SAFplusAmf::PendingOperation &pendingOperationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->pendingOperation.value = pendingOperationValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::PendingOperation> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::PendingOperation>(&(pendingOperation.value),pendingOperationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperationExpiration
     */
    SAFplusTypes::Date Component::getPendingOperationExpiration()
    {
        return this->pendingOperationExpiration.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/pendingOperationExpiration
     */
    void Component::setPendingOperationExpiration(SAFplusTypes::Date &pendingOperationExpirationValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->pendingOperationExpiration.value = pendingOperationExpirationValue;
        else
        {
            SAFplus::SimpleTxnOperation<SAFplusTypes::Date> *opt = new SAFplus::SimpleTxnOperation<SAFplusTypes::Date>(&(pendingOperationExpiration.value),pendingOperationExpirationValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/procStats
     */
    SAFplusAmf::ProcStats* Component::getProcStats()
    {
        return dynamic_cast<ProcStats*>(this->getChildObject("procStats"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/procStats
     */
    void Component::addProcStats(SAFplusAmf::ProcStats *procStatsValue)
    {
        this->addChildObject(procStatsValue, "procStats");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/activeAssignments
     */
    SAFplus::MgtHistoryStat<int>* Component::getActiveAssignments()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("activeAssignments"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/activeAssignments
     */
    void Component::addActiveAssignments(SAFplus::MgtHistoryStat<int> *activeAssignmentsValue)
    {
        this->addChildObject(activeAssignmentsValue, "activeAssignments");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/standbyAssignments
     */
    SAFplus::MgtHistoryStat<int>* Component::getStandbyAssignments()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("standbyAssignments"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/standbyAssignments
     */
    void Component::addStandbyAssignments(SAFplus::MgtHistoryStat<int> *standbyAssignmentsValue)
    {
        this->addChildObject(standbyAssignmentsValue, "standbyAssignments");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/instantiate
     */
    SAFplusAmf::Instantiate* Component::getInstantiate()
    {
        return dynamic_cast<Instantiate*>(this->getChildObject("instantiate"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/instantiate
     */
    void Component::addInstantiate(SAFplusAmf::Instantiate *instantiateValue)
    {
        this->addChildObject(instantiateValue, "instantiate");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/terminate
     */
    SAFplusAmf::Terminate* Component::getTerminate()
    {
        return dynamic_cast<Terminate*>(this->getChildObject("terminate"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/terminate
     */
    void Component::addTerminate(SAFplusAmf::Terminate *terminateValue)
    {
        this->addChildObject(terminateValue, "terminate");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/cleanup
     */
    SAFplusAmf::Cleanup* Component::getCleanup()
    {
        return dynamic_cast<Cleanup*>(this->getChildObject("cleanup"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/cleanup
     */
    void Component::addCleanup(SAFplusAmf::Cleanup *cleanupValue)
    {
        this->addChildObject(cleanupValue, "cleanup");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts
     */
    SAFplusAmf::Timeouts* Component::getTimeouts()
    {
        return dynamic_cast<Timeouts*>(this->getChildObject("timeouts"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/timeouts
     */
    void Component::addTimeouts(SAFplusAmf::Timeouts *timeoutsValue)
    {
        this->addChildObject(timeoutsValue, "timeouts");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/restartCount
     */
    SAFplus::MgtHistoryStat<int>* Component::getRestartCount()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("restartCount"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Component/restartCount
     */
    void Component::addRestartCount(SAFplus::MgtHistoryStat<int> *restartCountValue)
    {
        this->addChildObject(restartCountValue, "restartCount");
    };

    Component::~Component()
    {
    };

}
/* namespace ::SAFplusAmf */
