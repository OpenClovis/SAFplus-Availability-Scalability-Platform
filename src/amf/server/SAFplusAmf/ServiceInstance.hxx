/* 
 * File ServiceInstance.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef SERVICEINSTANCE_HXX_
#define SERVICEINSTANCE_HXX_

#include "EntityId.hxx"
#include "AssignmentState.hxx"
#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifier.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "clMgtList.hxx"
#include "clMgtHistoryStat.hxx"
#include "ServiceGroup.hxx"
#include "clMgtIdentifierList.hxx"
#include "ComponentServiceInstance.hxx"
#include "ServiceUnit.hxx"
#include "SAFplusAmfCommon.hxx"
#include <cstdint>
#include <vector>
#include "AdministrativeState.hxx"

namespace SAFplusAmf
  {

    class ServiceInstance : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(ServiceInstance);

    public:

        /*
         * Does the operator want this entity to be off, idle, or in service?
         */
        SAFplus::MgtProv<::SAFplusAmf::AdministrativeState> adminState;

        /*
         * The assignment state of a service instance indicates whether the service represented by this service instance is being provided or not by some service unit.
         */
        SAFplus::MgtProv<::SAFplusAmf::AssignmentState> assignmentState;

        /*
         * What is the optimal number of Service Units that should be given an active assignment for this work?  Note that the SA-Forum requires this field to be 1 for 2N, N+M, N-Way, and no redundancy models (see SAI-AIS-AMF-B.04.01@3.2.3.2 table 11).  However SAFplus allows this field to be set to any value for these models.
         */
        SAFplus::MgtProv<::uint32_t> preferredActiveAssignments;

        /*
         * What is the optimal number of Service Units that should be given a standby assignment for this work?  Note that the SA-Forum requires this field to be 1 for 2N, and N+M, redundancy models (see SAI-AIS-AMF-B.04.01@3.2.3.2 table 11).  However SAFplus allows this field to be set to any value for these models.  This field must be 0 for N-Way Active and No-redundancy models since these models do not have standby apps.
         */
        SAFplus::MgtProv<::uint32_t> preferredStandbyAssignments;

        /*
         * Lower rank is instantiated before higher; but rank 0 means 'don't care'.  This field indicates priority but does not guarantee ordering. That is, it is NOT true that all rank 1 entities will be finished before rank 2 is initiated (use dependencies for that).
         */
        SAFplus::MgtProv<::uint32_t> rank;

        /*
         * This work is assigned active to these service units.  Depending on the redundancy model, it 
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ServiceUnit*> activeAssignments;

        /*
         * This work is assigned active to these service units
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ServiceUnit*> standbyAssignments;

        /*
         * Component Service Instances in this Service group
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ComponentServiceInstance*> componentServiceInstances;
        SAFplus::MgtIdentifier<SAFplusAmf::ServiceGroup*> serviceGroup;
        SAFplus::MgtHistoryStat<int> numActiveAssignments;
        SAFplus::MgtHistoryStat<int> numStandbyAssignments;
        ::uint32_t numQuiescedAssignments;
        ::uint32_t numQuiescingAssignments;

        SAFplus::MgtProv<bool> isFullActiveAssignment;//true when all component service instances of this service instance are assigned to all components of an active service unit completely
        SAFplus::MgtProv<bool> isFullStandbyAssignment;//true when all component service instances of this service instance are assigned to all components of a standby service unit completely

        /*
         * An abstract definition of the amount of work this node can handle.  Nodes can be assigned capacities for arbitrarily chosen strings (MEM or CPU, for example).  Service Instances can be assigned 'weights' and the sum of the weights of service instances assigned active or standby on this node cannot exceed these values.
         */
        SAFplus::MgtList<std::string> activeWeightList;

        /*
         * An abstract definition of the amount of work this node can handle.  Nodes can be assigned capacities for arbitrarily chosen strings (MEM or CPU, for example).  Service Instances can be assigned 'weights' and the sum of the weights of service instances assigned active or standby on this node cannot exceed these values.
         */
        SAFplus::MgtList<std::string> standbyWeightList;

    public:
        ServiceInstance();
        ServiceInstance(const std::string& nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/adminState
         */
        ::SAFplusAmf::AdministrativeState getAdminState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/adminState
         */
        void setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/assignmentState
         */
        ::SAFplusAmf::AssignmentState getAssignmentState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/assignmentState
         */
        void setAssignmentState(::SAFplusAmf::AssignmentState &assignmentStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/preferredActiveAssignments
         */
        ::uint32_t getPreferredActiveAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/preferredActiveAssignments
         */
        void setPreferredActiveAssignments(::uint32_t preferredActiveAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/preferredStandbyAssignments
         */
        ::uint32_t getPreferredStandbyAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/preferredStandbyAssignments
         */
        void setPreferredStandbyAssignments(::uint32_t preferredStandbyAssignmentsValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/rank
         */
        ::uint32_t getRank();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/rank
         */
        void setRank(::uint32_t rankValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/serviceGroup
         */
        SAFplusAmf::ServiceGroup* getServiceGroup();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/serviceGroup
         */
        void setServiceGroup(SAFplusAmf::ServiceGroup* serviceGroupValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/numActiveAssignments
         */
        SAFplus::MgtHistoryStat<int>* getNumActiveAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/numActiveAssignments
         */
        void addNumActiveAssignments(SAFplus::MgtHistoryStat<int> *numActiveAssignmentsValue);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/numStandbyAssignments
         */
        SAFplus::MgtHistoryStat<int>* getNumStandbyAssignments();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/ServiceInstance/numStandbyAssignments
         */
        void addNumStandbyAssignments(SAFplus::MgtHistoryStat<int> *numStandbyAssignmentsValue);
        ~ServiceInstance();

    };
}
/* namespace ::SAFplusAmf */
#endif /* SERVICEINSTANCE_HXX_ */
