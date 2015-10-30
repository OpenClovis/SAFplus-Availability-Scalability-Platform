/* 
 * File FaultProbableCause.hxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#pragma once
#ifndef FAULTPROBABLECAUSE_HXX_
#define FAULTPROBABLECAUSE_HXX_

#include "FaultEnumsCommon.hxx"
#include <iostream>
#include "MgtEnumType.hxx"

namespace FaultEnums
  {

    enum class FaultProbableCause
      {
        ALARM_ID_INVALID, ALARM_PROB_CAUSE_LOSS_OF_SIGNAL, ALARM_PROB_CAUSE_LOSS_OF_FRAME, ALARM_PROB_CAUSE_FRAMING_ERROR, ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR, ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR, ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR, ALARM_PROB_CAUSE_DEGRADED_SIGNAL, ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE, ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR, ALARM_PROB_CAUSE_LAN_ERROR, ALARM_PROB_CAUSE_DTE, ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE, ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED, ALARM_PROB_CAUSE_BANDWIDTH_REDUCED, ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE, ALARM_PROB_CAUSE_THRESHOLD_CROSSED, ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED, ALARM_PROB_CAUSE_CONGESTION, ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY, ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM, ALARM_PROB_CAUSE_VERSION_MISMATCH, ALARM_PROB_CAUSE_CORRUPT_DATA, ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED, ALARM_PROB_CAUSE_SOFTWARE_ERROR, ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR, ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED, ALARM_PROB_CAUSE_FILE_ERROR, ALARM_PROB_CAUSE_OUT_OF_MEMORY, ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE, ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE, ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR, ALARM_PROB_CAUSE_POWER_PROBLEM, ALARM_PROB_CAUSE_TIMING_PROBLEM, ALARM_PROB_CAUSE_PROCESSOR_PROBLEM, ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR, ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM, ALARM_PROB_CAUSE_RECEIVER_FAILURE, ALARM_PROB_CAUSE_TRANSMITTER_FAILURE, ALARM_PROB_CAUSE_RECEIVE_FAILURE, ALARM_PROB_CAUSE_TRANSMIT_FAILURE, ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR, ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR, ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR, ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION, ALARM_PROB_CAUSE_ADAPTER_ERROR, ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE, ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE, ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM, ALARM_PROB_CAUSE_FIRE_DETECTED, ALARM_PROB_CAUSE_FLOOD_DETECTED, ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED, ALARM_PROB_CAUSE_LEAK_DETECTED, ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE, ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION, ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED, ALARM_PROB_CAUSE_PUMP_FAILURE, ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN
      };
    std::ostream& operator<<(std::ostream& os, const FaultProbableCause& e);
    std::istream& operator>>(std::istream& is, FaultProbableCause& e);
    const char* c_str(const FaultProbableCause& e);

    /*
     * This is the class that will handle the conversion for us.
     */
    class FaultProbableCauseManager : public SAFplus::MgtEnumType<FaultProbableCauseManager, FaultProbableCause> {
        FaultProbableCauseManager();  // private to prevent instantiation
    public:
        static const map_t en2str_map;  // This is the lookup table.
    };
}
/* namespace ::FaultEnums */
#endif /* FAULTPROBABLECAUSE_HXX_ */