/* 
 * File ServiceInstance.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef SERVICEINSTANCE_HXX_
#define SERVICEINSTANCE_HXX_
#include "SAFplusAmfCommon.hxx"

#include "AssignmentState.hxx"
#include <string>
#include "StandbyAssignments.hxx"
#include "ActiveWeightKey.hxx"
#include "ComponentServiceInstance.hxx"
#include "clMgtList.hxx"
#include "MgtFactory.hxx"
#include "ActiveAssignments.hxx"
#include "AdministrativeState.hxx"
#include "clMgtProv.hxx"
#include "StandbyAssignments.hxx"
#include "ServiceGroup.hxx"
#include <vector>
#include "ActiveAssignments.hxx"
#include "StandbyWeightKey.hxx"
#include "EntityId.hxx"
#include "clMgtProvList.hxx"

namespace SAFplusAmf
  {

    class ServiceInstance : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(ServiceInstance);

    public:

        /*
         * Does the operator want this entity to be off, idle, or in service?
         */
        SAFplus::MgtProv<SAFplusAmf::AdministrativeState> adminState;

        /*
         * The assignment state of a service instance indicates whether the service represented by this service instance is being provided or not by some service unit.
         */
        SAFplus::MgtProv<SAFplusAmf::AssignmentState> assignmentState;

        /*
         * Lower rank is instantiated before higher; but rank 0 means 'don't care'.
         */
        SAFplus::MgtProv<unsigned int> rank;

        /*
         * Component Service Instances in this Service group
         */
        SAFplus::MgtProvList<SAFplusAmf::ComponentServiceInstance*> componentServiceInstances;
        SAFplus::MgtProv<SAFplusAmf::ServiceGroup*> serviceGroup;

        /*
         * An abstract definition of the amount of work this node can handle.  Nodes can be assigned capacities for arbitrarily chosen strings (MEM or CPU, for example).  Service Instances can be assigned 'weights' and the sum of the weights of service instances assigned active or standby on this node cannot exceed these values.
         */
        SAFplus::MgtList<SAFplus::ActiveWeightKey> activeWeightList;

        /*
         * An abstract definition of the amount of work this node can handle.  Nodes can be assigned capacities for arbitrarily chosen strings (MEM or CPU, for example).  Service Instances can be assigned 'weights' and the sum of the weights of service instances assigned active or standby on this node cannot exceed these values.
         */
        SAFplus::MgtList<SAFplus::StandbyWeightKey> standbyWeightList;

    public:
        ServiceInstance();
        ServiceInstance(std::string myNameValue);
        void toString(std::stringstream &xmlString);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/adminState
         */
        SAFplusAmf::AdministrativeState getAdminState();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/adminState
         */
        void setAdminState(SAFplusAmf::AdministrativeState adminStateValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/assignmentState
         */
        SAFplusAmf::AssignmentState getAssignmentState();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/assignmentState
         */
        void setAssignmentState(SAFplusAmf::AssignmentState assignmentStateValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/rank
         */
        unsigned int getRank();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/rank
         */
        void setRank(unsigned int rankValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/componentServiceInstances
         */
        std::vector<SAFplusAmf::ComponentServiceInstance*> getComponentServiceInstances();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/componentServiceInstances
         */
        void setComponentServiceInstances(SAFplusAmf::ComponentServiceInstance* componentServiceInstancesValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/serviceGroup
         */
        SAFplusAmf::ServiceGroup* getServiceGroup();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/serviceGroup
         */
        void setServiceGroup(SAFplusAmf::ServiceGroup* serviceGroupValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/activeAssignments
         */
        SAFplusAmf::ActiveAssignments* getActiveAssignments();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/activeAssignments
         */
        void addActiveAssignments(SAFplusAmf::ActiveAssignments *activeAssignmentsValue);

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/standbyAssignments
         */
        SAFplusAmf::StandbyAssignments* getStandbyAssignments();

        /*
         * XPATH: /SAFplusAmf/ServiceInstance/standbyAssignments
         */
        void addStandbyAssignments(SAFplusAmf::StandbyAssignments *standbyAssignmentsValue);
        ~ServiceInstance();

    };
}
/* namespace SAFplusAmf */
#endif /* SERVICEINSTANCE_HXX_ */
