/* 
 * File ServiceUnit.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "EntityId.hxx"
#include "HighAvailabilityReadinessState.hxx"
#include "clMgtIdentifierList.hxx"
#include "PresenceState.hxx"
#include "MgtFactory.hxx"
#include "Component.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifier.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "clMgtHistoryStat.hxx"
#include "ServiceGroup.hxx"
#include "ReadinessState.hxx"
#include "Node.hxx"
#include "ServiceInstance.hxx"
#include "SAFplusAmfCommon.hxx"
#include <cstdint>
#include <vector>
#include "AdministrativeState.hxx"
#include "HighAvailabilityState.hxx"
#include "ServiceUnit.hxx"

using namespace SAFplusAmf;

namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(ServiceUnit, /SAFplusAmf/safplusAmf/ServiceUnit)

    ServiceUnit::ServiceUnit(): adminState("adminState"), rank("rank"), failover("failover"), preinstantiable("preinstantiable"), saAmfSUHostNodeOrNodeGroup("saAmfSUHostNodeOrNodeGroup"), presenceState("presenceState"), readinessState("readinessState"), haReadinessState("haReadinessState"), haState("haState"), operState("operState"), assignedServiceInstances("assignedServiceInstances"), components("components"), node("node"), serviceGroup("serviceGroup"), probationTime("probationTime")
    {
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&rank, "rank");
        this->addChildObject(&failover, "failover");
        this->addChildObject(&preinstantiable, "preinstantiable");
        preinstantiable.config = false;
        this->addChildObject(&saAmfSUHostNodeOrNodeGroup, "saAmfSUHostNodeOrNodeGroup");
        this->addChildObject(&presenceState, "presenceState");
        presenceState.config = false;
        this->addChildObject(&readinessState, "readinessState");
        readinessState.config = false;
        this->addChildObject(&haReadinessState, "haReadinessState");
        haReadinessState.config = false;
        this->addChildObject(&haState, "haState");
        haState.config = false;
        this->addChildObject(&operState, "operState");
        operState.config = false;
        this->addChildObject(&assignedServiceInstances, "assignedServiceInstances");
        assignedServiceInstances.config = false;
        this->addChildObject(&components, "components");
        this->addChildObject(&node, "node");
        this->addChildObject(&serviceGroup, "serviceGroup");
        this->addChildObject(&probationTime, "probationTime");
        this->addChildObject(&numActiveServiceInstances, "numActiveServiceInstances");
        numActiveServiceInstances.config = false;
        this->addChildObject(&numStandbyServiceInstances, "numStandbyServiceInstances");
        numStandbyServiceInstances.config = false;
        this->addChildObject(&restartCount, "restartCount");
        restartCount.config = false;
        this->tag.assign("ServiceUnit");
    };

    ServiceUnit::ServiceUnit(std::string nameValue): adminState("adminState"), rank("rank"), failover("failover"), preinstantiable("preinstantiable"), saAmfSUHostNodeOrNodeGroup("saAmfSUHostNodeOrNodeGroup"), presenceState("presenceState"), readinessState("readinessState"), haReadinessState("haReadinessState"), haState("haState"), operState("operState"), assignedServiceInstances("assignedServiceInstances"), components("components"), node("node"), serviceGroup("serviceGroup"), probationTime("probationTime")
    {
        this->name.value =  nameValue;
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&rank, "rank");
        this->addChildObject(&failover, "failover");
        this->addChildObject(&preinstantiable, "preinstantiable");
        preinstantiable.config = false;
        this->addChildObject(&saAmfSUHostNodeOrNodeGroup, "saAmfSUHostNodeOrNodeGroup");
        this->addChildObject(&presenceState, "presenceState");
        presenceState.config = false;
        this->addChildObject(&readinessState, "readinessState");
        readinessState.config = false;
        this->addChildObject(&haReadinessState, "haReadinessState");
        haReadinessState.config = false;
        this->addChildObject(&haState, "haState");
        haState.config = false;
        this->addChildObject(&operState, "operState");
        operState.config = false;
        this->addChildObject(&assignedServiceInstances, "assignedServiceInstances");
        assignedServiceInstances.config = false;
        this->addChildObject(&components, "components");
        this->addChildObject(&node, "node");
        this->addChildObject(&serviceGroup, "serviceGroup");
        this->addChildObject(&probationTime, "probationTime");
        this->addChildObject(&numActiveServiceInstances, "numActiveServiceInstances");
        numActiveServiceInstances.config = false;
        this->addChildObject(&numStandbyServiceInstances, "numStandbyServiceInstances");
        numStandbyServiceInstances.config = false;
        this->addChildObject(&restartCount, "restartCount");
        restartCount.config = false;
        this->tag.assign("ServiceUnit");
    };

    std::vector<std::string> ServiceUnit::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* ServiceUnit::getChildNames()
    {
        std::string childNames[] = { "name", "id", "adminState", "rank", "failover", "preinstantiable", "saAmfSUHostNodeOrNodeGroup", "presenceState", "readinessState", "haReadinessState", "haState", "operState", "assignedServiceInstances", "numActiveServiceInstances", "numStandbyServiceInstances", "restartCount", "components", "node", "serviceGroup", "probationTime" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/adminState
     */
    ::SAFplusAmf::AdministrativeState ServiceUnit::getAdminState()
    {
        return this->adminState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/adminState
     */
    void ServiceUnit::setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->adminState.value = adminStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState>(&(adminState.value),adminStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/rank
     */
    ::uint32_t ServiceUnit::getRank()
    {
        return this->rank.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/rank
     */
    void ServiceUnit::setRank(::uint32_t rankValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->rank.value = rankValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(rank.value),rankValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/failover
     */
    bool ServiceUnit::getFailover()
    {
        return this->failover.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/failover
     */
    void ServiceUnit::setFailover(bool failoverValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->failover.value = failoverValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(failover.value),failoverValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/preinstantiable
     */
    bool ServiceUnit::getPreinstantiable()
    {
        return this->preinstantiable.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/preinstantiable
     */
    void ServiceUnit::setPreinstantiable(bool preinstantiableValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->preinstantiable.value = preinstantiableValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(preinstantiable.value),preinstantiableValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/saAmfSUHostNodeOrNodeGroup
     */
    std::string ServiceUnit::getSaAmfSUHostNodeOrNodeGroup()
    {
        return this->saAmfSUHostNodeOrNodeGroup.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/saAmfSUHostNodeOrNodeGroup
     */
    void ServiceUnit::setSaAmfSUHostNodeOrNodeGroup(std::string saAmfSUHostNodeOrNodeGroupValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->saAmfSUHostNodeOrNodeGroup.value = saAmfSUHostNodeOrNodeGroupValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(saAmfSUHostNodeOrNodeGroup.value),saAmfSUHostNodeOrNodeGroupValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/presenceState
     */
    ::SAFplusAmf::PresenceState ServiceUnit::getPresenceState()
    {
        return this->presenceState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/presenceState
     */
    void ServiceUnit::setPresenceState(::SAFplusAmf::PresenceState &presenceStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->presenceState.value = presenceStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState>(&(presenceState.value),presenceStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/readinessState
     */
    ::SAFplusAmf::ReadinessState ServiceUnit::getReadinessState()
    {
        return this->readinessState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/readinessState
     */
    void ServiceUnit::setReadinessState(::SAFplusAmf::ReadinessState &readinessStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->readinessState.value = readinessStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::ReadinessState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::ReadinessState>(&(readinessState.value),readinessStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/haReadinessState
     */
    ::SAFplusAmf::HighAvailabilityReadinessState ServiceUnit::getHaReadinessState()
    {
        return this->haReadinessState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/haReadinessState
     */
    void ServiceUnit::setHaReadinessState(::SAFplusAmf::HighAvailabilityReadinessState &haReadinessStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->haReadinessState.value = haReadinessStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityReadinessState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityReadinessState>(&(haReadinessState.value),haReadinessStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/haState
     */
    ::SAFplusAmf::HighAvailabilityState ServiceUnit::getHaState()
    {
        return this->haState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/haState
     */
    void ServiceUnit::setHaState(::SAFplusAmf::HighAvailabilityState &haStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->haState.value = haStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::HighAvailabilityState>(&(haState.value),haStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/operState
     */
    bool ServiceUnit::getOperState()
    {
        return this->operState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/operState
     */
    void ServiceUnit::setOperState(bool operStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->operState.value = operStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(operState.value),operStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/assignedServiceInstances
     */
    std::vector<SAFplusAmf::ServiceInstance*> ServiceUnit::getAssignedServiceInstances()
    {
        return this->assignedServiceInstances.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/assignedServiceInstances
     */
    void ServiceUnit::setAssignedServiceInstances(SAFplusAmf::ServiceInstance* assignedServiceInstancesValue)
    {
        this->assignedServiceInstances.value.push_back(assignedServiceInstancesValue);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/components
     */
    std::vector<SAFplusAmf::Component*> ServiceUnit::getComponents()
    {
        return this->components.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/components
     */
    void ServiceUnit::setComponents(SAFplusAmf::Component* componentsValue)
    {
        this->components.value.push_back(componentsValue);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/node
     */
    SAFplusAmf::Node* ServiceUnit::getNode()
    {
        return this->node.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/node
     */
    void ServiceUnit::setNode(SAFplusAmf::Node* nodeValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->node.value = nodeValue;
        else
        {
            SAFplus::SimpleTxnOperation<SAFplusAmf::Node*> *opt = new SAFplus::SimpleTxnOperation<SAFplusAmf::Node*>(&(node.value),nodeValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/serviceGroup
     */
    SAFplusAmf::ServiceGroup* ServiceUnit::getServiceGroup()
    {
        return this->serviceGroup.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/serviceGroup
     */
    void ServiceUnit::setServiceGroup(SAFplusAmf::ServiceGroup* serviceGroupValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->serviceGroup.value = serviceGroupValue;
        else
        {
            SAFplus::SimpleTxnOperation<SAFplusAmf::ServiceGroup*> *opt = new SAFplus::SimpleTxnOperation<SAFplusAmf::ServiceGroup*>(&(serviceGroup.value),serviceGroupValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/probationTime
     */
    ::uint32_t ServiceUnit::getProbationTime()
    {
        return this->probationTime.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/probationTime
     */
    void ServiceUnit::setProbationTime(::uint32_t probationTimeValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->probationTime.value = probationTimeValue;
        else
        {
            SAFplus::SimpleTxnOperation<::uint32_t> *opt = new SAFplus::SimpleTxnOperation<::uint32_t>(&(probationTime.value),probationTimeValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/numActiveServiceInstances
     */
    SAFplus::MgtHistoryStat<int>* ServiceUnit::getNumActiveServiceInstances()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("numActiveServiceInstances"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/numActiveServiceInstances
     */
    void ServiceUnit::addNumActiveServiceInstances(SAFplus::MgtHistoryStat<int> *numActiveServiceInstancesValue)
    {
        this->addChildObject(numActiveServiceInstancesValue, "numActiveServiceInstances");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/numStandbyServiceInstances
     */
    SAFplus::MgtHistoryStat<int>* ServiceUnit::getNumStandbyServiceInstances()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("numStandbyServiceInstances"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/numStandbyServiceInstances
     */
    void ServiceUnit::addNumStandbyServiceInstances(SAFplus::MgtHistoryStat<int> *numStandbyServiceInstancesValue)
    {
        this->addChildObject(numStandbyServiceInstancesValue, "numStandbyServiceInstances");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/restartCount
     */
    SAFplus::MgtHistoryStat<int>* ServiceUnit::getRestartCount()
    {
        return dynamic_cast<SAFplus::MgtHistoryStat<int>*>(this->getChildObject("restartCount"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/ServiceUnit/restartCount
     */
    void ServiceUnit::addRestartCount(SAFplus::MgtHistoryStat<int> *restartCountValue)
    {
        this->addChildObject(restartCountValue, "restartCount");
    };

    ServiceUnit::~ServiceUnit()
    {
    };

}
/* namespace ::SAFplusAmf */
