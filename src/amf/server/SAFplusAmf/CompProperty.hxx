/* 
 * File PresenceState.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef COMPPROPERTY_HXX_
#define COMPPROPERTY_HXX_

#include "SAFplusAmfCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"

namespace SAFplusAmf
  {

    enum class CompProperty
      {
        sa_aware, proxied_preinstantiable, proxied_non_preinstantiable
      };
    std::ostream& operator<<(std::ostream& os, const CompProperty& e);
    std::istream& operator>>(std::istream& is, CompProperty& e);
    const char* c_str(const CompProperty& e);

    /*
     * This is the class that will handle the conversion for us.
     */
    class CompPropertyManager : public SAFplus::MgtEnumType<CompPropertyManager, CompProperty> {
        CompPropertyManager();  // private to prevent instantiation
    public:
        static const map_t en2str_map;  // This is the lookup table.
    };
}
/* namespace ::SAFplusAmf */
#endif /* COMPPROPERTY_HXX_ */