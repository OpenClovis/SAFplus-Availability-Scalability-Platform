/* 
 * File PresenceState.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "SAFplusAmfCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"
#include "PresenceState.hxx"


namespace SAFplusAmf
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const PresenceStateManager::map_t PresenceStateManager::en2str_map = {
            pair_t(PresenceState::uninstantiated, "uninstantiated"),
            pair_t(PresenceState::instantiating, "instantiating"),
            pair_t(PresenceState::instantiated, "instantiated"),
            pair_t(PresenceState::terminating, "terminating"),
            pair_t(PresenceState::restarting, "restarting"),
            pair_t(PresenceState::instantiationFailed, "instantiationFailed"),
            pair_t(PresenceState::terminationFailed, "terminationFailed")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusAmf::PresenceState &presenceState)
    {
        return PresenceStateManager::c_str(presenceState);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusAmf::PresenceState &presenceState)
    {
        return os << PresenceStateManager::toString(presenceState);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusAmf::PresenceState &presenceState)
    {
        std::string buf;
        is >> buf;
        presenceState = PresenceStateManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusAmf */
