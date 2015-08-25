/* 
 * File ProcStats.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 
#include "SAFplusAmfCommon.hxx"

#include "ProcessStats.hxx"
#include <vector>
#include "MgtFactory.hxx"
#include "ProcStats.hxx"


namespace SAFplusAmf
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(ProcStats, /SAFplusAmf/safplusAmf/Component/procStats)

    ProcStats::ProcStats()
    {
        this->addChildObject(&failures, "failures");
        this->addChildObject(&cpuUtilization, "cpuUtilization");
        this->addChildObject(&memUtilization, "memUtilization");
        this->addChildObject(&pageFaults, "pageFaults");
        this->addChildObject(&numThreads, "numThreads");
        this->addChildObject(&residentMem, "residentMem");
        this->tag.assign("procStats");
    };

    std::vector<std::string>* ProcStats::getChildNames()
    {
        std::string childNames[] = { "failures", "cpuUtilization", "memUtilization", "pageFaults", "numThreads", "residentMem", "processState" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    ProcStats::~ProcStats()
    {
    };

}
/* namespace ::SAFplusAmf */