/* 
 * File LifecycleState.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtEnumType.hxx"
#include "SAFplusTypesCommon.hxx"
#include <iostream>
#include "LifecycleState.hxx"


namespace SAFplusTypes
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const LifecycleStateManager::map_t LifecycleStateManager::en2str_map = {};
    // workaround for fixing double free or corruption of event (nowhere uses this class, so why issue here???)
    /*{
            pair_t(LifecycleState::start, "start"),
            pair_t(LifecycleState::idle, "idle"),
            pair_t(LifecycleState::stop, "stop")
    };*/ // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusTypes::LifecycleState &lifecycleState)
    {
        return LifecycleStateManager::c_str(lifecycleState);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusTypes::LifecycleState &lifecycleState)
    {
        return os << LifecycleStateManager::toString(lifecycleState);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusTypes::LifecycleState &lifecycleState)
    {
        std::string buf;
        is >> buf;
        lifecycleState = LifecycleStateManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusTypes */
