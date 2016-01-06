/* 
 * File AlarmCategory.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "FaultEnumsCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"
#include "AlarmCategory.hxx"


namespace FaultEnums
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const AlarmCategoryManager::map_t AlarmCategoryManager::en2str_map = {
            pair_t(AlarmCategory::ALARM_CATEGORY_INVALID, "ALARM_CATEGORY_INVALID"),
            pair_t(AlarmCategory::ALARM_CATEGORY_COMMUNICATIONS, "ALARM_CATEGORY_COMMUNICATIONS"),
            pair_t(AlarmCategory::ALARM_CATEGORY_QUALITY_OF_SERVICE, "ALARM_CATEGORY_QUALITY_OF_SERVICE"),
            pair_t(AlarmCategory::ALARM_CATEGORY_PROCESSING_ERROR, "ALARM_CATEGORY_PROCESSING_ERROR"),
            pair_t(AlarmCategory::ALARM_CATEGORY_EQUIPMENT, "ALARM_CATEGORY_EQUIPMENT"),
            pair_t(AlarmCategory::ALARM_CATEGORY_ENVIRONMENTAL, "ALARM_CATEGORY_ENVIRONMENTAL"),
            pair_t(AlarmCategory::ALARM_CATEGORY_COUNT, "ALARM_CATEGORY_COUNT")
    }; // uses c++11 initializer lists 

    const char* c_str(const ::FaultEnums::AlarmCategory &alarmCategory)
    {
        return AlarmCategoryManager::c_str(alarmCategory);
    };

    std::ostream& operator<<(std::ostream &os, const ::FaultEnums::AlarmCategory &alarmCategory)
    {
        return os << AlarmCategoryManager::toString(alarmCategory);
    };

    std::istream& operator>>(std::istream &is, ::FaultEnums::AlarmCategory &alarmCategory)
    {
        std::string buf;
        is >> buf;
        alarmCategory = AlarmCategoryManager::toEnum(buf);
        return is;
    };

}
/* namespace ::FaultEnums */
