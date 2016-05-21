/* 
 * File Application.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef APPLICATION_HXX_
#define APPLICATION_HXX_

#include "EntityId.hxx"
#include "MgtFactory.hxx"
#include "clTransaction.hxx"
#include "clMgtIdentifierList.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "clMgtHistoryStat.hxx"
#include "ServiceGroup.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>
#include "AdministrativeState.hxx"

namespace SAFplusAmf
  {

    class Application : public EntityId {

        /* Apply MGT object factory */
        MGT_REGISTER(Application);

    public:

        /*
         * Does the operator want this entity to be off, idle, or in service?
         */
        SAFplus::MgtProv<::SAFplusAmf::AdministrativeState> adminState;

        /*
         * Service Groups in this Application
         */
        SAFplus::MgtIdentifierList<::SAFplusAmf::ServiceGroup*> serviceGroups;

        /*
         * SAFplus Extension: To the greatest extent possible, all Service Groups in this application will be Active (or standby) on the same node.  This will only be not true if service groups are not configured to run on the same nodes.
         */
        SAFplus::MgtProv<bool> keepTogether;
        SAFplus::MgtHistoryStat<int> numServiceGroups;

    public:
        Application();
        Application(const std::string& nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/adminState
         */
        ::SAFplusAmf::AdministrativeState getAdminState();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/adminState
         */
        void setAdminState(::SAFplusAmf::AdministrativeState &adminStateValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/keepTogether
         */
        bool getKeepTogether();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/keepTogether
         */
        void setKeepTogether(bool keepTogetherValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/numServiceGroups
         */
        SAFplus::MgtHistoryStat<int>* getNumServiceGroups();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Application/numServiceGroups
         */
        void addNumServiceGroups(SAFplus::MgtHistoryStat<int> *numServiceGroupsValue);
        ~Application();

    };
}
/* namespace ::SAFplusAmf */
#endif /* APPLICATION_HXX_ */
