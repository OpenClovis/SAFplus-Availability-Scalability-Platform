/* 
 * File AlarmState.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtEnumType.hxx"
#include "SAFplusAlarmCommon.hxx"
#include <iostream>
#include "AlarmState.hxx"


namespace SAFplusAlarm
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const AlarmStateManager::map_t AlarmStateManager::en2str_map = {
            pair_t(AlarmState::INVALID, "INVALID"),
            pair_t(AlarmState::CLEAR, "CLEAR"),
            pair_t(AlarmState::SUPPRESSED, "SUPPRESSED"),
            pair_t(AlarmState::ASSERT_UNDER_SOAKING, "ASSERT_UNDER_SOAKING"),
            pair_t(AlarmState::CLEAR_UNDER_SOAKING, "CLEAR_UNDER_SOAKING"),
            pair_t(AlarmState::ASSERT, "ASSERT")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusAlarm::AlarmState &alarmState)
    {
        return AlarmStateManager::c_str(alarmState);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusAlarm::AlarmState &alarmState)
    {
        return os << AlarmStateManager::toString(alarmState);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusAlarm::AlarmState &alarmState)
    {
        std::string buf;
        is >> buf;
        alarmState = AlarmStateManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusAlarm */