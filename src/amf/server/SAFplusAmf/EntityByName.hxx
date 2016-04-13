/* 
 * File EntityByName.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef ENTITYBYNAME_HXX_
#define ENTITYBYNAME_HXX_

#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "clTransaction.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "SAFplusAmfCommon.hxx"
#include <vector>

namespace SAFplusAmf
  {

    class EntityByName : public SAFplus::MgtContainer {

        /* Apply MGT object factory */
        MGT_REGISTER(EntityByName);

    public:
        SAFplus::MgtProv<std::string> name;
        SAFplus::MgtProv<std::string> entity;

    public:
        EntityByName();
        EntityByName(const std::string& nameValue);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityByName/name
         */
        std::string getName();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityByName/name
         */
        void setName(std::string nameValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityByName/entity
         */
        std::string getEntity();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityByName/entity
         */
        void setEntity(std::string entityValue, SAFplus::Transaction &t=SAFplus::NO_TXN);
        ~EntityByName();

    };
}
/* namespace ::SAFplusAmf */
#endif /* ENTITYBYNAME_HXX_ */
