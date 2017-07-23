/* 
 * File TimerType.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtEnumType.hxx"
#include "SAFplusAlarmCommon.hxx"
#include <iostream>
#include "TimerType.hxx"


namespace SAFplusAlarm
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const TimerTypeManager::map_t TimerTypeManager::en2str_map = {
            pair_t(TimerType::INVALID, "INVALID"),
            pair_t(TimerType::ASSERT, "ASSERT"),
            pair_t(TimerType::CLEAR, "CLEAR")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusAlarm::TimerType &timerType)
    {
        return TimerTypeManager::c_str(timerType);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusAlarm::TimerType &timerType)
    {
        return os << TimerTypeManager::toString(timerType);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusAlarm::TimerType &timerType)
    {
        std::string buf;
        is >> buf;
        timerType = TimerTypeManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusAlarm */
