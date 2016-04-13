/* 
 * File Stats.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtFactory.hxx"
#include "clMgtContainer.hxx"
#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "Load.hxx"
#include "SAFplusAmfCommon.hxx"
#include <cstdint>
#include <vector>
#include "Stats.hxx"

using namespace SAFplusAmf;

namespace SAFplusAmf
  {

    Stats::Stats(): SAFplus::MgtContainer("stats"), upTime("upTime"), bootTime("bootTime")
    {
        this->addChildObject(&upTime, "upTime");
        upTime.config = false;
        this->addChildObject(&bootTime, "bootTime");
        bootTime.config = false;
        this->addChildObject(&load, "load");
        load.config = false;
    };

    std::vector<std::string>* Stats::getChildNames()
    {
        std::string childNames[] = { "load", "upTime", "bootTime" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/upTime
     */
    ::uint64_t Stats::getUpTime()
    {
        return this->upTime.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/upTime
     */
    void Stats::setUpTime(::uint64_t upTimeValue, SAFplus::Transaction &txn)
    {
        this->upTime.set(upTimeValue,txn);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/bootTime
     */
    ::uint64_t Stats::getBootTime()
    {
        return this->bootTime.value;
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/bootTime
     */
    void Stats::setBootTime(::uint64_t bootTimeValue, SAFplus::Transaction &txn)
    {
        this->bootTime.set(bootTimeValue,txn);
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/load
     */
    SAFplusAmf::Load* Stats::getLoad()
    {
        return dynamic_cast<Load*>(this->getChildObject("load"));
    };

    /*
     * XPATH: /SAFplusAmf/safplusAmf/Node/stats/load
     */
    void Stats::addLoad(SAFplusAmf::Load *loadValue)
    {
        this->addChildObject(loadValue, "load");
    };

    Stats::~Stats()
    {
    };

}
/* namespace ::SAFplusAmf */
