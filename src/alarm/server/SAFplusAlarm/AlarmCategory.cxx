/* 
 * File AlarmCategory.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtEnumType.hxx"
#include "SAFplusAlarmCommon.hxx"
#include <iostream>
#include "AlarmCategory.hxx"


namespace SAFplusAlarm
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const AlarmCategoryManager::map_t AlarmCategoryManager::en2str_map = {
            pair_t(AlarmCategory::INVALID, "INVALID"),
            pair_t(AlarmCategory::COMMUNICATIONS, "COMMUNICATIONS"),
            pair_t(AlarmCategory::QUALITYOFSERVICE, "QUALITYOFSERVICE"),
            pair_t(AlarmCategory::PROCESSINGERROR, "PROCESSINGERROR"),
            pair_t(AlarmCategory::EQUIPMENT, "EQUIPMENT"),
            pair_t(AlarmCategory::ENVIROMENTAL, "ENVIROMENTAL")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::SAFplusAlarm::AlarmCategory &alarmCategory)
    {
        return AlarmCategoryManager::c_str(alarmCategory);
    };

    std::ostream& operator<<(std::ostream &os, const ::SAFplusAlarm::AlarmCategory &alarmCategory)
    {
        return os << AlarmCategoryManager::toString(alarmCategory);
    };

    std::istream& operator>>(std::istream &is, ::SAFplusAlarm::AlarmCategory &alarmCategory)
    {
        std::string buf;
        is >> buf;
        alarmCategory = AlarmCategoryManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusAlarm */