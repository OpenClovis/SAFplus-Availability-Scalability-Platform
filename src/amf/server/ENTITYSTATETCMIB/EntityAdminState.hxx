/* 
 * File EntityAdminState.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef ENTITYADMINSTATE_HXX_
#define ENTITYADMINSTATE_HXX_
#include "ENTITYSTATETCMIBCommon.hxx"

#include "MgtEnumType.hxx"
#include <iostream>

namespace ENTITYSTATETCMIB
  {

    enum class EntityAdminState
      {
        unknown=1, locked=2, shuttingDown=3, unlocked=4
      };
    std::ostream& operator<<(std::ostream& os, const EntityAdminState& e);
    std::istream& operator>>(std::istream& is, EntityAdminState& e);
    const char* c_str(const EntityAdminState& e);

    /*
     * This is the class that will handle the conversion for us.
     */
    class EntityAdminStateManager : public SAFplus::MgtEnumType<EntityAdminStateManager, EntityAdminState> {
        EntityAdminStateManager();  // private to prevent instantiation
    public:
        static const map_t en2str_map;  // This is the lookup table.
    };
}
/* namespace ::ENTITYSTATETCMIB */
#endif /* ENTITYADMINSTATE_HXX_ */
