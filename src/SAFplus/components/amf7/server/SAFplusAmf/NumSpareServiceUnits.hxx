/* 
 * File NumSpareServiceUnits.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef NUMSPARESERVICEUNITS_HXX_
#define NUMSPARESERVICEUNITS_HXX_
#include "SAFplusAmfCommon.hxx"

#include <vector>
#include "MgtFactory.hxx"
#include "IntStatistic.hxx"

namespace SAFplusAmf
  {

    class NumSpareServiceUnits : public SAFplusTypes::IntStatistic {

        /* Apply MGT object factory */
        MGT_REGISTER(NumSpareServiceUnits);

    public:

    public:
        NumSpareServiceUnits();
        std::vector<std::string>* getChildNames();
        ~NumSpareServiceUnits();

    };
}
/* namespace SAFplusAmf */
#endif /* NUMSPARESERVICEUNITS_HXX_ */