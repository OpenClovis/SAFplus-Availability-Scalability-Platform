/* 
 * File EntityById.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef ENTITYBYID_HXX_
#define ENTITYBYID_HXX_

#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "clTransaction.hxx"
#include <string>
#include "clMgtProv.hxx"
#include "SAFplusAmfCommon.hxx"
#include <cstdint>
#include <vector>

namespace SAFplusAmf
  {

    class EntityById : public SAFplus::MgtContainer {

        /* Apply MGT object factory */
        MGT_REGISTER(EntityById);

    public:
        SAFplus::MgtProv<::uint16_t> id;
        SAFplus::MgtProv<std::string> entity;

    public:
        EntityById();
        EntityById(::uint16_t idValue);
        EntityById(const std::string& nameParam);
        std::vector<std::string> getKeys();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityById/id
         */
        ::uint16_t getId();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityById/id
         */
        void setId(::uint16_t idValue, SAFplus::Transaction &t=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityById/entity
         */
        std::string getEntity();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/EntityById/entity
         */
        void setEntity(std::string entityValue, SAFplus::Transaction &t=SAFplus::NO_TXN);
        ~EntityById();

    };
}
/* namespace ::SAFplusAmf */
#endif /* ENTITYBYID_HXX_ */
