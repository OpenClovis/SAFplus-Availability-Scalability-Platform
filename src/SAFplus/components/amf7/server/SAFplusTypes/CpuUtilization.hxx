/* 
 * File CpuUtilization.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef CPUUTILIZATION_HXX_
#define CPUUTILIZATION_HXX_
#include "SAFplusTypesCommon.hxx"

#include "DecStatistic.hxx"
#include <vector>
#include "MgtFactory.hxx"

namespace SAFplusTypes
  {

    class CpuUtilization : public DecStatistic {

        /* Apply MGT object factory */
        MGT_REGISTER(CpuUtilization);

    public:

    public:
        CpuUtilization();
        std::vector<std::string>* getChildNames();
        ~CpuUtilization();

    };
}
/* namespace SAFplusTypes */
#endif /* CPUUTILIZATION_HXX_ */