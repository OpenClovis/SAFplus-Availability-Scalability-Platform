/* 
 * File Cluster.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 
#include "SAFplusAmfCommon.hxx"

#include <string>
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "AdministrativeState.hxx"
#include "EntityId.hxx"
#include "Cluster.hxx"


namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(Cluster, /SAFplusAmf/Cluster)

    Cluster::Cluster(): adminState("adminState"), startupAssignmentDelay("startupAssignmentDelay")
    {
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&startupAssignmentDelay, "startupAssignmentDelay");
        this->name.assign("Cluster");
    };

    Cluster::Cluster(std::string myNameValue): adminState("adminState"), startupAssignmentDelay("startupAssignmentDelay")
    {
        this->myName.value =  myNameValue;
        this->addChildObject(&adminState, "adminState");
        this->addChildObject(&startupAssignmentDelay, "startupAssignmentDelay");
        this->name.assign("Cluster");
    };

    void Cluster::toString(std::stringstream &xmlString)
    {
        /* TODO:  */
    };

    std::vector<std::string> Cluster::getKeys()
    {
        std::string keyNames[] = { "myName" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* Cluster::getChildNames()
    {
        std::string childNames[] = { "myName", "id", "adminState", "startupAssignmentDelay" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/Cluster/adminState
     */
    SAFplusAmf::AdministrativeState Cluster::getAdminState()
    {
        return this->adminState.value;
    };

    /*
     * XPATH: /SAFplusAmf/Cluster/adminState
     */
    void Cluster::setAdminState(SAFplusAmf::AdministrativeState adminStateValue)
    {
        this->adminState.value = adminStateValue;
    };

    /*
     * XPATH: /SAFplusAmf/Cluster/startupAssignmentDelay
     */
    SaTimeT Cluster::getStartupAssignmentDelay()
    {
        return this->startupAssignmentDelay.value;
    };

    /*
     * XPATH: /SAFplusAmf/Cluster/startupAssignmentDelay
     */
    void Cluster::setStartupAssignmentDelay(SaTimeT startupAssignmentDelayValue)
    {
        this->startupAssignmentDelay.value = startupAssignmentDelayValue;
    };

    Cluster::~Cluster()
    {
    };

}
/* namespace SAFplusAmf */