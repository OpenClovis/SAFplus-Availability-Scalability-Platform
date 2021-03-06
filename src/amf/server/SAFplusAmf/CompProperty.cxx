/* 
 * File CompProperty.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "SAFplusAmfCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"
#include "CompProperty.hxx"


namespace SAFplusAmf
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const CompPropertyManager::map_t CompPropertyManager::en2str_map = {
            pair_t(CompProperty::sa_aware, "sa_aware"),
            pair_t(CompProperty::proxied_preinstantiable, "proxied_preinstantiable"),
            pair_t(CompProperty::proxied_non_preinstantiable, "proxied_non_preinstantiable")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusAmf::CompProperty &compProperty)
    {
        return CompPropertyManager::c_str(compProperty);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusAmf::CompProperty &compProperty)
    {
        return os << CompPropertyManager::toString(compProperty);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusAmf::CompProperty &compProperty)
    {
        std::string buf;
        is >> buf;
        compProperty = CompPropertyManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusAmf */
