/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : alarm
 * File        : clAlarmDefinitions.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file has alarm related definitions
 *
 *
 *****************************************************************************/

/**
 *  \file 
 *  \brief Header File of Alarm related Definitions
 *  \ingroup alarm_apis
 */

/**
 *  \addtogroup alarm_apis
 *  \{
 */

#ifndef _CL_ALARM_DEFINITIONS_H_
#define _CL_ALARM_DEFINITIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clVersion.h>
#include <clCorMetaData.h>

/****************************************************************************
 * Type/Constant definitions
 ****************************************************************************/

/**
 *  ID to specify whether the alarm is invalid or not.
 */

/****************************************************************************
 * Categories of Alarm
 ****************************************************************************/
/**
 *  Alarm event channel name. This is the channel on which the subscriber will
 *  be waiting for notifications. 
 */
#define ClAlarmEventName "CL_ALARM_EVENT_CHANNEL"

/**
 *  Event Type to be specifed while subscribing for the events
 *  published by alarm server.
 */
#define CL_ALARM_EVENT 1

/**
 * Enumeration defining the categories of alarms
 * support by alarm server.
 */
typedef enum{

    /**
     *  Category for alarms that are not valid
     */
     CL_ALARM_CATEGORY_INVALID             = 0,

    /**
     *  Category for alarms that are related to communication between to points.
     */
     CL_ALARM_CATEGORY_COMMUNICATIONS       = 1,

    /**
     *  Category for alarms that are related to degrations in quality of service.
     */
     CL_ALARM_CATEGORY_QUALITY_OF_SERVICE   = 2,

    /**
     *  Category for alarms that are related to software or processing error.
     */
     CL_ALARM_CATEGORY_PROCESSING_ERROR     = 3,

    /**
     *  Category for alarms that are related to equipment problems.
     */
     CL_ALARM_CATEGORY_EQUIPMENT            = 4,

    /**
     *  Category for alarms that are related to environmental conditions around the equipment.
     */
     CL_ALARM_CATEGORY_ENVIRONMENTAL        = 5,


}ClAlarmCategoryTypeT;

/****************************************************************************
 * Specific problem of Alarm
 ****************************************************************************/

/**
 * The type of an identifier to the specific problem of the alarm.
 * This information is not configured but is assigned a value at run-time
 * for segregation of alarms that have the same \e category and probable cause
 * but are different in their manifestation.
 */
typedef ClUint32T ClAlarmSpecificProblemT;

/**
 * The type of the handle for identifying the raised alarm.
 */
typedef ClUint32T ClAlarmHandleT;

/****************************************************************************
 * Severities of Alarm
 ****************************************************************************/

/**
 *  Enumeration to depict the severity of the alarm which
 *  is specified while modeling and also while publishing
 *  the alarm.
 */ 
typedef enum{

    /**
     * Alarm with invalid severity
     */
     CL_ALARM_SEVERITY_INVALID       = 0,

    /**
     * Alarm with severity level as \e critical.
     */
     CL_ALARM_SEVERITY_CRITICAL      = 1,

    /**
     *  Alarm with severity level as \e major.
     */
     CL_ALARM_SEVERITY_MAJOR         = 2,

    /**
     *  Alarm with severity level as \e minor.
     */
     CL_ALARM_SEVERITY_MINOR         = 3,

    /**
     *  Alarm with severity level \e warning.
     */
     CL_ALARM_SEVERITY_WARNING       = 4,

    /**
     *  Alarm with severity level \e indeterminate.
     */
     CL_ALARM_SEVERITY_INDETERMINATE = 5,

    /**
     *  Alarm with severity level as \e cleared.
     */
     CL_ALARM_SEVERITY_CLEAR         = 6

}ClAlarmSeverityTypeT;

/****************************************************************************
 * List of probable cause of alarms.
 ****************************************************************************/

/**
 * This enumeration defines all the probable causes of the
 * alarm based on the categories.
 */ 
typedef enum{
    /**
     * invalid cause
     */
    CL_ALARM_ID_INVALID =0,
    /**
     * Probable cause for alarms which can occur due to communication related
     * problems between two points.
     */

    /**
     * An error condition when there is no data in the communication channel.
     */ 
    CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL = 1,

    /**
     * An inability to locate the information in bit-delimiters.
     */ 
    CL_ALARM_PROB_CAUSE_LOSS_OF_FRAME,

    /**
     * Error in the bit-groups delimiters within the bit-stream.
     */ 
    CL_ALARM_PROB_CAUSE_FRAMING_ERROR,

    /**
     * An error has occured in the transmission between local node and adjacent node.
     */ 
    CL_ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR,

    /**
     * An error has occurred on the transmission channel beyond the adjacent node. 
     */ 
    CL_ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR,

    /**
     * Error occured while establishing the connection.
     */ 
    CL_ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR,

    /**
     * The quality of the singal has decreased.
     */ 
    CL_ALARM_PROB_CAUSE_DEGRADED_SIGNAL,

    /**
     * A failure in the subsystem which supports communication over telecommunication links.
     */ 
    CL_ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE,

    /**
     * The communication protocol has been voilated.
     */ 
    CL_ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR,

    /**
     * Problem detected in the local area network.
     */ 
    CL_ALARM_PROB_CAUSE_LAN_ERROR,

    /**
     * Probable cause of the alarm is data transmission error.
     */ 
    CL_ALARM_PROB_CAUSE_DTE,

    /**
     * Probable causes of alarms caused due to degradation in quality of service.
     */
    
    /**
     * The time delays between the query and its result is beyond the allowable limits. 
     */ 
    CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE,

    /**
     * The number of items to be processed has exceeded the specified limits. 
     */ 
    CL_ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED,

    /**
     * The bandwidth available for transmission has reduced. 
     */ 
    CL_ALARM_PROB_CAUSE_BANDWIDTH_REDUCED,

    /**
     * Number of repeat transmission is beyond the allowed limits. 
     */ 
    CL_ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE,

    /**
     * The limit allowed (or configured) has exceeded. 
     */ 
    CL_ALARM_PROB_CAUSE_THRESHOLD_CROSSED,

    /**
     * Service limit or service aggrement are outside the acceptable limits.
     */ 
    CL_ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED,

    /**
     * System is reaching its capacity or approaching it.
     */ 
    CL_ALARM_PROB_CAUSE_CONGESTION,

    /**
     * Probable cause of the alarm is the maximum capacity is already reached or about to be reached.
     */ 
    CL_ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY,

    /**
     * Probable causes of alarm caused due to software or processing errors.
     */
    
    /**
     * Probable cause of the alarm is storage capacity problem.
     */
    CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM,

    /**
     * There is a mismatch in the version of two communicating entities. 
     */
    CL_ALARM_PROB_CAUSE_VERSION_MISMATCH,

    /**
     * An error has caused the data to be incorrect.
     */ 
    CL_ALARM_PROB_CAUSE_CORRUPT_DATA,

    /**
     * CPU cycles limited has been exceeded its limit.
     */ 
    CL_ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED,

    /**
     * Probable cause of alarm is error in software.
     */ 
    CL_ALARM_PROB_CAUSE_SOFWARE_ERROR,

    /**
     * Probable cause of alarm is error in software program error.
     */ 
    CL_ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR,

    /**
     * Probable cause of alarm is software program abnormally terminated.
     */ 
    CL_ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED,

    /**
     * Format of the file is incorrect.
     */ 
    CL_ALARM_PROB_CAUSE_FILE_ERROR,

    /**
     * There is no program-addressable storage available.
     */ 
    CL_ALARM_PROB_CAUSE_OUT_OF_MEMORY,

    /**
     * An entity upon which the reporting object depends has become unreliable. 
     */ 
    CL_ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,

    /**
     * A failure is detected in the application subsystem failure. A subsystem can be a software
     * supporting the larger module.
     */ 
    CL_ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE,

    /**
     * The parameter for configuration or customization are given improperly.
     */ 
    CL_ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR,

    /**
     * Probable cause alarms caused due to equipment related problems.
     */
    
    /**
     * There is a problem with the power supply. 
     */ 
    CL_ALARM_PROB_CAUSE_POWER_PROBLEM,
    
    /**
     * The process which has timing constraints didn't complete with the specified
     * time or has completed but the results are not reliable.
     */ 
    CL_ALARM_PROB_CAUSE_TIMING_PROBLEM,
    
    /**
     * Internal machine error has occurred on the a Central processing Unit. 
     */ 
    CL_ALARM_PROB_CAUSE_PROCESSOR_PROBLEM,
    
    /**
     * An internal error has occured on a dataset or modem. 
     */ 
    CL_ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR,
    
    /**
     * Error occured while multiplexing the communication signal.
     */ 
    CL_ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM,
    
    /**
     * Probable cause of alarm is receiver failure.
     */ 
    CL_ALARM_PROB_CAUSE_RECEIVER_FAILURE,
    
    /**
     * Probable cause of alarm is transamitter failure.
     */ 
    CL_ALARM_PROB_CAUSE_TRANSMITTER_FAILURE,
    
    /**
     * Probable cause of alarm is receive failure.
     */ 
    CL_ALARM_PROB_CAUSE_RECEIVE_FAILURE,
    
    /**
     * Probable cause of alarm is transmission failure.
     */ 
    CL_ALARM_PROB_CAUSE_TRANSMIT_FAILURE,
    
    /**
     * Error has occured on a output device.
     */ 
    CL_ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR,
    
    /**
     * Probable cause of alarm is input device error.
     */ 
    CL_ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR,
    
    /**
     * Probable cause of alarm is input output device error.
     */ 
    CL_ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR,
    
    /**
     * Internal error in equipment malfunction for which no actual cause has been identified.
     */ 
    CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION,
    
    /**
     * Probable cause of alarm is adapter error.
     */ 
    CL_ALARM_PROB_CAUSE_ADAPTER_ERROR,

    /**
     * Probable cause of alarms caused due to environmental changes around the equipment.
     */

    /**
     * Probable cause of alarm is temperature unacceptable.
     */
    CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE,

    /**
     * Probable cause of alarm is humidity unacceptable around equipment.
     */
    CL_ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE,

    /**
     * Probable cause of alarm is heating or ventillation or cooling system problem.
     */
    CL_ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM,

    /**
     * Probable cause of alarm is fire around the equipment.
     */
    CL_ALARM_PROB_CAUSE_FIRE_DETECTED,

    /**
     * Probable cause of alarm is flood around equipment.
     */
    CL_ALARM_PROB_CAUSE_FLOOD_DETECTED,

    /**
     * Probable cause of alarm is toxic leak is detected around equipment.
     */
    CL_ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED,

    /**
     * A leak of gas or fluid is detected around equipment.
     */
    CL_ALARM_PROB_CAUSE_LEAK_DETECTED,

    /**
     * The fluid or gas pressure is unacceptable.
     */
    CL_ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE,

    /**
     * The vibration around the equipment has exceeded the limit.
     */
    CL_ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION,

    /**
     * Probable cause of alarm is exhaustion of the materail supply.
     */
    CL_ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED,

    /**
     * Error has occured in the pump which is used to trasfer fluid in the equipment.
     */
    CL_ALARM_PROB_CAUSE_PUMP_FAILURE,

    /**
     * Probable cause of alarm is enclosure door is open.
     */
    CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN
}ClAlarmProbableCauseT;


/**
 * The enumeration to depict the state of the alarm that is
 * into. It can be in any of the following state that is 
 * cleared, assert, suppressed or under soaking.
 */
typedef enum{
    /**
     * Invalid alarm state.
     */
    CL_ALARM_STATE_INVALID = -1,
    /**
     *  The alarm condition has cleared.
     */
    CL_ALARM_STATE_CLEAR   =   0,
    /**
     *  The alarm condition has occured.
     */
    CL_ALARM_STATE_ASSERT  =   1,
    /**
     * The alarm state is suppressed.
     */
    CL_ALARM_STATE_SUPPRESSED = 2,
    /**
     * The alarm state is under soaking.
     */
    CL_ALARM_STATE_UNDER_SOAKING = 3

}ClAlarmStateT;

/**
 * The \e ClAlarmInfoT data structure is used to store
 * the entire list of alarm attributes that
 * include probable cause, MOID, category,
 * sub-category, severity, and event time along with
 * additional information of the alarm. It also
 * ascertains whether the alarm is asserted or
 * cleared.
 */

typedef struct
{
    /**
     * Probable cause of the alarm.
     */
    ClAlarmProbableCauseT probCause;

    /**
     * Component name of the resource on which the alarm occurred.
     */
    ClNameT compName;

    /**
     * Resource on which the alarm occurred.
     */
    ClCorMOIdT moId;

    /**
     * Flag to indicate if the alarm was for assert or for clear.
     * If \e isAssert is set, then the alarm condition has occured.
     * If \e isAssert is zero, then the alarm has cleared.
     * indicates alarm has cleared.
     */
    ClAlarmStateT alarmState;

    /**
     * Category of the alarm.
     */
    ClAlarmCategoryTypeT category;

    /**
     * Specific-Problem of the alarm. This field adds further refinement
     * to the probable cause specified while raising the alarm.
     */
    ClAlarmSpecificProblemT specificProblem;

    /**
     * Severity of the alarm.
     */
    ClAlarmSeverityTypeT severity;

    /**
     * Time stamp of the alarm being raised.
     */
    ClTimeT    eventTime;

    /**
     * Length of the additional data about the alarm.
     */
    ClUint32T len;

    /**
     * Additional information about the alarm.
     */
    ClUint8T buff[1];

}ClAlarmInfoT;

/**
 * Pointer type for ClAlarmInfoT.
 */
typedef ClAlarmInfoT* ClAlarmInfoPtrT;

/**
 * The \e ClAlarmHandleInfoT data structure is used to store
 * the handle and the information of the alarm. The event which 
 * would be published would contain this information.
 */
typedef struct 
{
    /**
     * Unique alarm handle identifying an alarm
     */
    ClAlarmHandleT alarmHandle;
    /**
     * Alarm Information
     */
    ClAlarmInfoT   alarmInfo;
}ClAlarmHandleInfoT;


/** 
 * Enums for alarm fetch mode
 */ 
typedef enum
{
/**
 * There are two ways to get alarm notification for the alarm client:
 *   - The alarm can be reported by the application
 *     or any other component that is monitoring.
 */
    CL_ALARM_REPORT = 0,

/**   - The alarm client polls a resource on which the alarm
 *     may be raised.
 */
    CL_ALARM_POLL
}ClAlarmFetchModeT;

/**
 *  The enum enumerates the possible relationships between two alarms.
 */
typedef enum
{
/**
 * If there is only one alarm, then there is no relationship
 * that must be determined. If there is more than one alarm,
 * then the relationship between them must be determined.
 * The relationship can be either a logical OR or
 * a logical AND.
 */
    CL_ALARM_RULE_NO_RELATION =0,
/**
 * The alarm is raised or cleared, if at least one of the alarms
 * (specified in the modeling) exist.
 */
    CL_ALARM_RULE_LOGICAL_OR,
/**
 * The alarm is raised or cleared, only if all the alarms
 * (specified in the modeling) exist.
 */
    CL_ALARM_RULE_LOGICAL_AND,
/**
 *  There is no valid relation between the alarms
 *  (specified in the modeling).
 */
    CL_ALARM_RULE_RELATION_INVALID,
}ClAlarmRuleRelationT;


/** 
 * This structure contains the information about the alarm configured.
 * This has the probable cause and its specific problem.
 */
typedef struct {
    /**
     *  The \e probable cause of the alarm participating in the alarm rule.
     */
    ClAlarmProbableCauseT   probableCause;

    /**
     * The \e specific problem of the alarm participating in the generation rule.
     */
    ClAlarmSpecificProblemT specificProblem;
}ClAlarmRuleEntryT;

typedef ClAlarmRuleEntryT VDECL_VER(ClAlarmIdT, 4, 1, 0);

/**
 * This structure specifies the alarm rule relation
 * and the alarms IDs (probable cause) that participate in the alarm rule.
 */

typedef struct
{
    /**
     *  Alarm rule relation is a relationship between alarms.
     *  The relation can be \c CL_ALARM_RULE_NO_RELATION,
     * \c CL_ALARM_RULE_LOGICAL_OR, \c CL_ALARM_RULE_LOGICAL_AND.
     */
    ClAlarmRuleRelationT relation;

    /**
     *  The structure containing the probable cause and specific 
     *  problem of the alarms participating in the alarm rule.
     */
    ClAlarmRuleEntryT* alarmIds;
}ClAlarmRuleInfoT;


/**
 * This structure is used to store the alarm related
 * attributes including the category, sub-category, severity,
 * soaking time for assert and clear, probable cause of alarm,
 * and the generation and suppression rule for alarm.
 */

typedef struct ClAlarmProfile
{

    /**
     *  \e category to which the alarm belongs.
     */

     ClAlarmCategoryTypeT category;

    /**
     *   Probable Cause of the alarm.
     */
    ClAlarmProbableCauseT probCause;

    /**
     *  \e severity of the alarm.
     */
     ClAlarmSeverityTypeT severity;

    /**
     *  Indicates whether the alarm needs to be polled or not.
     */
    ClAlarmFetchModeT fetchMode;

    /**
     *  Soaking-time for the alarm to be asserted.
     */
    ClUint32T assertSoakingTime;

    /**
     *  Soaking-time for the alarm to be cleared.
     */
    ClUint32T clearSoakingTime;

    /**
     *  \e generationRule for the alarm.
     */
    ClAlarmRuleInfoT* generationRule;

    /**
     *  \e suppressionRule for the alarm.
     */
    ClAlarmRuleInfoT* suppressionRule;

    /**
     * \e specific problem for the alarm.
     */
    ClAlarmSpecificProblemT  specificProblem;
}ClAlarmProfileT;

/**
 * The structure captures the alarm profile of all alarms
 * that are modeled for each resource in the component.
 */

typedef struct ClAlarmComponentResAlarms
{

    /**
     * Resource \e MOID for which the alarm profiles are provided in the \e MoAlarms.
     */
    ClCorClassTypeT moClassID;

    /**
     * The frequency at which the resource will be
     * polled for the current status of all of its alarms.
     */
    ClUint32T pollingTime;

    /**
     * Pointer to ClAlarmProfileT structures of all alarms for the resource.
     */

    ClAlarmProfileT* MoAlarms;
}ClAlarmComponentResAlarmsT;

/**
 * Structure to contain the pending alarm information.
 */
typedef struct ClAlarmPendingAlmInfo
{
    /**
     * Pending alarm handle.
     */
    ClAlarmHandleT alarmHandle;

    /**
     * Pointer to the pending alarm information.
     */
    ClAlarmInfoPtrT pAlarmInfo;
}ClAlarmPendingAlmInfoT;

/**
 * Pointer type for ClAlarmPendingAlmInfoT.
 */
typedef ClAlarmPendingAlmInfoT* ClAlarmPendingAlmInfoPtrT;

/**
 * Structure to contain the information about the pending alarms.
 */
typedef struct ClAlarmPendingAlmList
{
    /**
     * The element will store the number of
     * pending alarms present in the pAlarmList 
     * array.
     */
    ClUint32T       noOfPendingAlarm;

    /**
     * The pointer to the structure which will store
     * the pointer to array of ClAlarmHandleInfoT structure
     * containing information about the pending alarms.
     */
    ClAlarmPendingAlmInfoPtrT  pAlarmList;
}ClAlarmPendingAlmListT;

/**
 * Pointer type for ClAlarmPendingAlmListT.
 */
typedef ClAlarmPendingAlmListT* ClAlarmPendingAlmListPtrT;

#ifdef __cplusplus
}
#endif

#endif /* _CL_ALARM_DEFINITIONS_H_ */

/** \} */

