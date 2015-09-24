/* 
 * File Stats.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef STATS_HXX_
#define STATS_HXX_
#include "SAFplusAmfCommon.hxx"

#include "Load.hxx"
#include "Load.hxx"
#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include <cstdint>

namespace SAFplusAmf
  {

    class Stats : public SAFplus::MgtContainer {

        /* Apply MGT object factory */
        MGT_REGISTER(Stats);

    public:

        /*
         * Number of seconds this node has been running
         */
        SAFplus::MgtProv<::uint64_t> upTime;

        /*
         * Date (in seconds since the epoch) this node booted
         */
        SAFplus::MgtProv<::uint64_t> bootTime;
        SAFplusAmf::Load load;

    public:
        Stats();
        std::vector<std::string>* getChildNames();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/upTime
         */
        ::uint64_t getUpTime();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/upTime
         */
        void setUpTime(::uint64_t upTimeValue, SAFplus::Transaction &txn=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/bootTime
         */
        ::uint64_t getBootTime();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/bootTime
         */
        void setBootTime(::uint64_t bootTimeValue, SAFplus::Transaction &txn=SAFplus::NO_TXN);

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/load
         */
        SAFplusAmf::Load* getLoad();

        /*
         * XPATH: /SAFplusAmf/safplusAmf/Node/stats/load
         */
        void addLoad(SAFplusAmf::Load *loadValue);
        ~Stats();

    };
}
/* namespace ::SAFplusAmf */
#endif /* STATS_HXX_ */