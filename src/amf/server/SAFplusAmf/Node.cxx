/* 
 * File Node.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "EntityId.hxx"
#include "PresenceState.hxx"
#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifierList.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "clMgtList.hxx"
#include "ServiceUnit.hxx"
#include "Stats.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "AdministrativeState.hxx"
#include "ServiceUnitFailureEscalationPolicy.hxx"
#include "Node.hxx"

using namespace  std;
using namespace SAFplusAmf;

namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(Node, /SAFplusAmf/safplusAmf/Node)

    Node::Node(): presenceState("presenceState",::SAFplusAmf::PresenceState::uninstantiated), adminState("adminState",::SAFplusAmf::AdministrativeState::on), operState("operState"), autoRepair("autoRepair"), failFastOnInstantiationFailure("failFastOnInstantiationFailure"), failFastOnCleanupFailure("failFastOnCleanupFailure"), disableAssignmentOn("disableAssignmentOn"), serviceUnits("serviceUnits"), capacityList("capacity")
    {
        this->addChildObject(&presenceState, "presenceState");
        presenceState.config = false;
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&operState, "operState");
        operState.config = false;
        this->addChildObject(&autoRepair, "autoRepair");
        this->addChildObject(&failFastOnInstantiationFailure, "failFastOnInstantiationFailure");
        this->addChildObject(&failFastOnCleanupFailure, "failFastOnCleanupFailure");
        this->addChildObject(&disableAssignmentOn, "disableAssignmentOn");
        this->addChildObject(&serviceUnits, "serviceUnits");
        this->addChildObject(&stats, "stats");
        stats.config = false;
        this->addChildObject(&serviceUnitFailureEscalationPolicy, "serviceUnitFailureEscalationPolicy");
        this->addChildObject(&capacityList, "capacity");
        capacityList.childXpath="/SAFplusAmf/safplusAmf/Node/capacity";
        capacityList.setListKey("resource");
        this->tag.assign("Node");
        presenceState = ::SAFplusAmf::PresenceState::uninstantiated;
        adminState = ::SAFplusAmf::AdministrativeState::on;
    };

    Node::Node(const std::string& nameValue): presenceState("presenceState",::SAFplusAmf::PresenceState::uninstantiated), adminState("adminState",::SAFplusAmf::AdministrativeState::on), operState("operState"), autoRepair("autoRepair"), failFastOnInstantiationFailure("failFastOnInstantiationFailure"), failFastOnCleanupFailure("failFastOnCleanupFailure"), disableAssignmentOn("disableAssignmentOn"), serviceUnits("serviceUnits"), capacityList("capacity")
    {
        this->name.value =  nameValue;
        this->addChildObject(&presenceState, "presenceState");
        presenceState.config = false;
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&operState, "operState");
        operState.config = false;
        this->addChildObject(&autoRepair, "autoRepair");
        this->addChildObject(&failFastOnInstantiationFailure, "failFastOnInstantiationFailure");
        this->addChildObject(&failFastOnCleanupFailure, "failFastOnCleanupFailure");
        this->addChildObject(&disableAssignmentOn, "disableAssignmentOn");
        this->addChildObject(&serviceUnits, "serviceUnits");
        this->addChildObject(&stats, "stats");
        stats.config = false;
        this->addChildObject(&serviceUnitFailureEscalationPolicy, "serviceUnitFailureEscalationPolicy");
        this->addChildObject(&capacityList, "capacity");
        capacityList.childXpath="/SAFplusAmf/safplusAmf/Node/capacity";
        capacityList.setListKey("resource");
        this->tag.assign("Node");
    };

    std::vector<std::string> Node::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* Node::getChildNames()
    {
        std::string childNames[] = { "name", "id", "stats", "presenceState", "adminState", "operState", "capacity", "serviceUnitFailureEscalationPolicy", "autoRepair", "failFastOnInstantiationFailure", "failFastOnCleanupFailure", "disableAssignmentOn", "serviceUnits" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/presenceState
     */
    ::SAFplusAmf::PresenceState Node::getPresenceState()
    {
        return this->presenceState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/presenceState
     */
    void Node::setPresenceState(::SAFplusAmf::PresenceState &presenceStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->presenceState.value = presenceStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::PresenceState>(&(presenceState.value),presenceStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/adminState
     */
    ::SAFplusAmf::AdministrativeState Node::getAdminState()
    {
        return this->adminState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/adminState
     */
    void Node::setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->adminState.value = adminStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState> *opt = new SAFplus::SimpleTxnOperation<::SAFplusAmf::AdministrativeState>(&(adminState.value),adminStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/operState
     */
    bool Node::getOperState()
    {
        return this->operState.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/operState
     */
    void Node::setOperState(bool operStateValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->operState.value = operStateValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(operState.value),operStateValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/autoRepair
     */
    bool Node::getAutoRepair()
    {
        return this->autoRepair.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/autoRepair
     */
    void Node::setAutoRepair(bool autoRepairValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->autoRepair.value = autoRepairValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(autoRepair.value),autoRepairValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/failFastOnInstantiationFailure
     */
    bool Node::getFailFastOnInstantiationFailure()
    {
        return this->failFastOnInstantiationFailure.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/failFastOnInstantiationFailure
     */
    void Node::setFailFastOnInstantiationFailure(bool failFastOnInstantiationFailureValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->failFastOnInstantiationFailure.value = failFastOnInstantiationFailureValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(failFastOnInstantiationFailure.value),failFastOnInstantiationFailureValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/failFastOnCleanupFailure
     */
    bool Node::getFailFastOnCleanupFailure()
    {
        return this->failFastOnCleanupFailure.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/failFastOnCleanupFailure
     */
    void Node::setFailFastOnCleanupFailure(bool failFastOnCleanupFailureValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->failFastOnCleanupFailure.value = failFastOnCleanupFailureValue;
        else
        {
            SAFplus::SimpleTxnOperation<bool> *opt = new SAFplus::SimpleTxnOperation<bool>(&(failFastOnCleanupFailure.value),failFastOnCleanupFailureValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/disableAssignmentOn
     */
    std::string Node::getDisableAssignmentOn()
    {
        return this->disableAssignmentOn.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/disableAssignmentOn
     */
    void Node::setDisableAssignmentOn(std::string disableAssignmentOnValue, SAFplus::Transaction &t)
    {
        if(&t == &SAFplus::NO_TXN) this->disableAssignmentOn.value = disableAssignmentOnValue;
        else
        {
            SAFplus::SimpleTxnOperation<std::string> *opt = new SAFplus::SimpleTxnOperation<std::string>(&(disableAssignmentOn.value),disableAssignmentOnValue);
            t.addOperation(opt);
        }
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/serviceUnits
     */
    std::vector<::SAFplusAmf::ServiceUnit*> Node::getServiceUnits()
    {
        return this->serviceUnits.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/serviceUnits
     */
    void Node::setServiceUnits(::SAFplusAmf::ServiceUnit* serviceUnitsValue)
    {
        this->serviceUnits.value.push_back(serviceUnitsValue);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats
     */
    SAFplusAmf::Stats* Node::getStats()
    {
        return dynamic_cast<Stats*>(this->getChildObject("stats"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats
     */
    void Node::addStats(SAFplusAmf::Stats *statsValue)
    {
        this->addChildObject(statsValue, "stats");
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/serviceUnitFailureEscalationPolicy
     */
    SAFplusAmf::ServiceUnitFailureEscalationPolicy* Node::getServiceUnitFailureEscalationPolicy()
    {
        return dynamic_cast<ServiceUnitFailureEscalationPolicy*>(this->getChildObject("serviceUnitFailureEscalationPolicy"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/serviceUnitFailureEscalationPolicy
     */
    void Node::addServiceUnitFailureEscalationPolicy(SAFplusAmf::ServiceUnitFailureEscalationPolicy *serviceUnitFailureEscalationPolicyValue)
    {
        this->addChildObject(serviceUnitFailureEscalationPolicyValue, "serviceUnitFailureEscalationPolicy");
    };

    Node::~Node()
    {
    };

}
/* namespace ::SAFplusAmf */
