/*******************************************************************************
**
** FILE:
**   saNtf.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum Notification Service (NTF). 
**   It contains all of the prototypes and type definitions required 
**   for user API functions
**   
** SPECIFICATION VERSION:
**   SAI-AIS-NTF-A.03.01
**
** DATE: 
**   Wed Oct 08 2008
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS.
**
** Copyright 2008 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/

#ifndef _SA_NTF_H
#define _SA_NTF_H

#include <saAis.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef SaUint64T SaNtfHandleT;
typedef SaUint64T SaNtfNotificationHandleT;
typedef SaUint64T SaNtfNotificationFilterHandleT;
typedef SaUint64T SaNtfReadHandleT;


typedef enum {
    SA_NTF_TYPE_OBJECT_CREATE_DELETE = 0x1000,
    SA_NTF_TYPE_ATTRIBUTE_CHANGE = 0x2000,
    SA_NTF_TYPE_STATE_CHANGE = 0x3000,
    SA_NTF_TYPE_ALARM = 0x4000,
    SA_NTF_TYPE_SECURITY_ALARM = 0x5000,
    SA_NTF_TYPE_MISCELLANEOUS = 0x6000
} SaNtfNotificationTypeT;

/*  
 * Event types enum, these are only generic
 * types as defined by the X.73x standards  
 */
#define SA_NTF_NOTIFICATIONS_TYPE_MASK 0xF000

typedef enum {
    SA_NTF_OBJECT_NOTIFICATIONS_START = SA_NTF_TYPE_OBJECT_CREATE_DELETE,
    SA_NTF_OBJECT_CREATION,
    SA_NTF_OBJECT_DELETION,

    SA_NTF_ATTRIBUTE_NOTIFICATIONS_START = SA_NTF_TYPE_ATTRIBUTE_CHANGE,
    SA_NTF_ATTRIBUTE_ADDED,
    SA_NTF_ATTRIBUTE_REMOVED,
    SA_NTF_ATTRIBUTE_CHANGED,
    SA_NTF_ATTRIBUTE_RESET,

    SA_NTF_STATE_CHANGE_NOTIFICATIONS_START = SA_NTF_TYPE_STATE_CHANGE,
    SA_NTF_OBJECT_STATE_CHANGE,

    SA_NTF_ALARM_NOTIFICATIONS_START = SA_NTF_TYPE_ALARM,
    SA_NTF_ALARM_COMMUNICATION,
    SA_NTF_ALARM_QOS,
    SA_NTF_ALARM_PROCESSING,
    SA_NTF_ALARM_EQUIPMENT,
    SA_NTF_ALARM_ENVIRONMENT,

    SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START = SA_NTF_TYPE_SECURITY_ALARM,
    SA_NTF_INTEGRITY_VIOLATION,
    SA_NTF_OPERATION_VIOLATION,
    SA_NTF_PHYSICAL_VIOLATION,
    SA_NTF_SECURITY_SERVICE_VIOLATION,
    SA_NTF_TIME_VIOLATION, 

    SA_NTF_MISCELLANEOUS_NOTIFICATIONS_START = SA_NTF_TYPE_MISCELLANEOUS,
    SA_NTF_APPLICATION_EVENT,
    SA_NTF_ADMIN_OPERATION_START,
    SA_NTF_ADMIN_OPERATION_END,
    SA_NTF_CONFIG_UPDATE_START,
    SA_NTF_CONFIG_UPDATE_END,
    SA_NTF_ERROR_REPORT,
    SA_NTF_ERROR_CLEAR,
    SA_NTF_HPI_EVENT_RESOURCE,
    SA_NTF_HPI_EVENT_SENSOR,
    SA_NTF_HPI_EVENT_WATCHDOG,
    SA_NTF_HPI_EVENT_DIMI,
    SA_NTF_HPI_EVENT_FUMI,
    SA_NTF_HPI_EVENT_OTHER
} SaNtfEventTypeT;

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
typedef enum {
    SA_NTF_TYPE_OBJECT_CREATE_DELETE_BIT = 0x0001,
    SA_NTF_TYPE_ATTRIBUTE_CHANGE_BIT = 0x0002,
    SA_NTF_TYPE_STATE_CHANGE_BIT = 0x0004,
    SA_NTF_TYPE_ALARM_BIT = 0x0008,
    SA_NTF_TYPE_SECURITY_ALARM_BIT = 0x0010
} SaNtfNotificationTypeBitsT;
#endif /* SA_NTF_A01 || SA_NTF_A02 */

#define SA_NTF_OBJECT_CREATION_BIT                      0x01
#define SA_NTF_OBJECT_DELETION_BIT                      0x02
#define SA_NTF_ATTRIBUTE_ADDED_BIT                      0x04
#define SA_NTF_ATTRIBUTE_REMOVED_BIT                    0x08
#define SA_NTF_ATTRIBUTE_CHANGED_BIT                    0x10
#define SA_NTF_ATTRIBUTE_RESET_BIT                      0x20
#define SA_NTF_OBJECT_STATE_CHANGE_BIT                  0x40
#define SA_NTF_ALARM_COMMUNICATION_BIT                  0x80
#define SA_NTF_ALARM_QOS_BIT                           0x100
#define SA_NTF_ALARM_PROCESSING_BIT                    0x200
#define SA_NTF_ALARM_EQUIPMENT_BIT                     0x400
#define SA_NTF_ALARM_ENVIRONMENT_BIT                   0x800
#define SA_NTF_INTEGRITY_VIOLATION_BIT                0x1000
#define SA_NTF_OPERATION_VIOLATION_BIT                0x2000
#define SA_NTF_PHYSICAL_VIOLATION_BIT                 0x4000
#define SA_NTF_SECURITY_SERVICE_VIOLATION_BIT         0x8000
#define SA_NTF_TIME_VIOLATION_BIT                    0x10000
#define SA_NTF_ADMIN_OPERATION_START_BIT             0x20000
#define SA_NTF_ADMIN_OPERATION_END_BIT               0x40000
#define SA_NTF_CONFIG_UPDATE_START_BIT               0x80000
#define SA_NTF_CONFIG_UPDATE_END_BIT                0x100000
#define SA_NTF_ERROR_REPORT_BIT                     0x200000
#define SA_NTF_ERROR_CLEAR_BIT 		            0x400000
#define SA_NTF_HPI_EVENT_RESOURCE_BIT               0x800000
#define SA_NTF_HPI_EVENT_SENSOR_BIT                0x1000000
#define SA_NTF_HPI_EVENT_WATCHDOG_BIT              0x2000000
#define SA_NTF_HPI_EVENT_DIMI_BIT                  0x4000000
#define SA_NTF_HPI_EVENT_FUMI_BIT                  0x8000000
#define SA_NTF_HPI_EVENT_OTHER                    0x10000000
#define SA_NTF_APPLICATION_EVENT_BIT          0x100000000000

typedef SaUint64T SaNtfEventTypeBitmapT;

typedef struct { 
    SaUint32T vendorId; 
    SaUint16T majorId;
    SaUint16T minorId;
} SaNtfClassIdT; 

#define SA_NTF_VENDOR_ID_SAF 18568

typedef SaUint16T SaNtfElementIdT;

typedef SaUint64T SaNtfIdentifierT;

#define SA_NTF_IDENTIFIER_UNUSED   ((SaNtfIdentifierT) 0LL)

typedef struct {
    SaNtfIdentifierT rootCorrelationId;
    SaNtfIdentifierT parentCorrelationId;
    SaNtfIdentifierT notificationId;
} SaNtfCorrelationIdsT;

/* Initializing this enum to start with value 1, as the earlier value
 * of 0 was causing issue during marshalling/unmarshalling the unused value types */
typedef enum {
    SA_NTF_VALUE_UINT8 = 1,     /* 1 byte long - unsigned int */
    SA_NTF_VALUE_INT8,      /* 1 byte long - signed int */
    SA_NTF_VALUE_UINT16,    /* 2 bytes long - unsigned int */
    SA_NTF_VALUE_INT16,     /* 2 bytes long - signed int */
    SA_NTF_VALUE_UINT32,    /* 4 bytes long - unsigned int */
    SA_NTF_VALUE_INT32,     /* 4 bytes long - signed int */
    SA_NTF_VALUE_FLOAT,     /* 4 bytes long - float */
    SA_NTF_VALUE_UINT64,    /* 8 bytes long - unsigned int */
    SA_NTF_VALUE_INT64,     /* 8 bytes long - signed int */
    SA_NTF_VALUE_DOUBLE,    /* 8 bytes long - double */
    SA_NTF_VALUE_LDAP_NAME, /* SaNameT type */
    SA_NTF_VALUE_STRING,    /* '\0' terminated char array (UTF-8
                               encoded) */
    SA_NTF_VALUE_IPADDRESS, /* IPv4 or IPv6 address as '\0' terminated
                               char array */
    SA_NTF_VALUE_BINARY,    /* Binary data stored in bytes - number of
                               bytes stored separately */
    SA_NTF_VALUE_ARRAY      /* Array of some data type - size of elements
                               and number of elements stored separately */
} SaNtfValueTypeT;

typedef union {
    SaUint8T  uint8Val;    /* SA_NTF_VALUE_UINT8 */
    SaInt8T   int8Val;      /* SA_NTF_VALUE_INT8 */
    SaUint16T uint16Val;  /* SA_NTF_VALUE_UINT16 */
    SaInt16T  int16Val;    /* SA_NTF_VALUE_INT16 */
    SaUint32T uint32Val;  /* SA_NTF_VALUE_UINT32 */
    SaInt32T  int32Val;    /* SA_NTF_VALUE_INT32 */
    SaFloatT  floatVal;    /* SA_NTF_VALUE_FLOAT */
    SaUint64T uint64Val;  /* SA_NTF_VALUE_UINT64 */
    SaInt64T  int64Val;    /* SA_NTF_VALUE_INT64 */
    SaDoubleT doubleVal;  /* SA_NTF_VALUE_DOUBLE */

    /* 
     * This struct can represent variable length fields like        
     * LDAP names, strings, IP addresses and binary data. 
     * It may only be used in conjunction with the data type values 
     * SA_NTF_VALUE_LDAP_NAME, SA_NTF_VALUE_STRING, 
     * SA_NTF_VALUE_IPADDRESS and SA_NTF_VALUE_BINARY.
     * This field shall not be directly accessed. 
     * To initialise this structure and to set a pointer to the real data   
     * use saNtfPtrValAllocate(). The function saNtfPtrValGet() shall be used
     * for retrieval of the real data.
     */
    struct {
        SaUint16T dataOffset;
        SaUint16T dataSize;
    } ptrVal;

    /* 
     * This struct represents sets of data of identical type        
     * like notification identifiers, attributes  etc. 
     * It may only be used in conjunction with the data type value 
     * SA_NTF_VALUE_ARRAY. Functions        
     * SaNtfArrayValAllocate() or SaNtfArrayValGet() shall be used to       
     * get a pointer for accessing the real data. Direct access is not allowed.
     */

    struct {
        SaUint16T arrayOffset;
        SaUint16T numElements;
        SaUint16T elementSize;
    } arrayVal;

} SaNtfValueT;

typedef struct {
    SaNtfElementIdT infoId;
    /* API user is expected to define this field    */
    SaNtfValueTypeT infoType;
    SaNtfValueT infoValue;
} SaNtfAdditionalInfoT;

typedef struct {
    SaNtfEventTypeT *eventType;
    SaNameT *notificationObject;
    SaNameT *notifyingObject;
    SaNtfClassIdT *notificationClassId;
    SaTimeT *eventTime;
    SaUint16T numCorrelatedNotifications;
    SaUint16T lengthAdditionalText;
    SaUint16T numAdditionalInfo;
    SaNtfIdentifierT *notificationId;
    SaNtfIdentifierT *correlatedNotifications;
    SaStringT additionalText;
    SaNtfAdditionalInfoT *additionalInfo;
} SaNtfNotificationHeaderT;

typedef enum {
    SA_NTF_OBJECT_OPERATION = 0,
    SA_NTF_UNKNOWN_OPERATION = 1,
    SA_NTF_MANAGEMENT_OPERATION = 2
} SaNtfSourceIndicatorT;

typedef struct {
    SaNtfElementIdT attributeId;
    SaNtfValueTypeT attributeType;
    SaBoolT oldAttributePresent;
    SaNtfValueT oldAttributeValue;
    SaNtfValueT newAttributeValue;
} SaNtfAttributeChangeT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
    SaUint16T numAttributes;
    SaNtfSourceIndicatorT *sourceIndicator;
    SaNtfAttributeChangeT *changedAttributes;
} SaNtfAttributeChangeNotificationT;

typedef struct {
    SaNtfElementIdT attributeId;
    SaNtfValueTypeT attributeType;
    SaNtfValueT attributeValue;
} SaNtfAttributeT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
    SaUint16T numAttributes;
    SaNtfSourceIndicatorT *sourceIndicator;
    SaNtfAttributeT *objectAttributes;
} SaNtfObjectCreateDeleteNotificationT;

typedef struct {
    SaNtfElementIdT stateId;
    SaBoolT oldStatePresent;
    SaUint16T oldState;
    SaUint16T newState;
} SaNtfStateChangeT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
    SaUint16T numStateChanges;
    SaNtfSourceIndicatorT *sourceIndicator;
    SaNtfStateChangeT *changedStates;
} SaNtfStateChangeNotificationT;

typedef enum {
    SA_NTF_ADAPTER_ERROR,
    SA_NTF_APPLICATION_SUBSYSTEM_FAILURE,
    SA_NTF_BANDWIDTH_REDUCED,
    SA_NTF_CALL_ESTABLISHMENT_ERROR,
    SA_NTF_COMMUNICATIONS_PROTOCOL_ERROR,
    SA_NTF_COMMUNICATIONS_SUBSYSTEM_FAILURE,
    SA_NTF_CONFIGURATION_OR_CUSTOMIZATION_ERROR,
    SA_NTF_CONGESTION,
    SA_NTF_CORRUPT_DATA,
    SA_NTF_CPU_CYCLES_LIMIT_EXCEEDED,
    SA_NTF_DATASET_OR_MODEM_ERROR,
    SA_NTF_DEGRADED_SIGNAL,
    SA_NTF_D_T_E,
    SA_NTF_ENCLOSURE_DOOR_OPEN,
    SA_NTF_EQUIPMENT_MALFUNCTION,
    SA_NTF_EXCESSIVE_VIBRATION,
    SA_NTF_FILE_ERROR,
    SA_NTF_FIRE_DETECTED,
    SA_NTF_FLOOD_DETECTED,
    SA_NTF_FRAMING_ERROR,
    SA_NTF_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM,
    SA_NTF_HUMIDITY_UNACCEPTABLE,
    SA_NTF_INPUT_OUTPUT_DEVICE_ERROR,
    SA_NTF_INPUT_DEVICE_ERROR,
    SA_NTF_L_A_N_ERROR,
    SA_NTF_LEAK_DETECTED,
    SA_NTF_LOCAL_NODE_TRANSMISSION_ERROR,
    SA_NTF_LOSS_OF_FRAME,
    SA_NTF_LOSS_OF_SIGNAL,
    SA_NTF_MATERIAL_SUPPLY_EXHAUSTED,
    SA_NTF_MULTIPLEXER_PROBLEM,
    SA_NTF_OUT_OF_MEMORY,
    SA_NTF_OUTPUT_DEVICE_ERROR,
    SA_NTF_PERFORMANCE_DEGRADED,
    SA_NTF_POWER_PROBLEM,
    SA_NTF_PRESSURE_UNACCEPTABLE,
    SA_NTF_PROCESSOR_PROBLEM,
    SA_NTF_PUMP_FAILURE,
    SA_NTF_QUEUE_SIZE_EXCEEDED,
    SA_NTF_RECEIVE_FAILURE,
    SA_NTF_RECEIVER_FAILURE,
    SA_NTF_REMOTE_NODE_TRANSMISSION_ERROR,
    SA_NTF_RESOURCE_AT_OR_NEARING_CAPACITY,
    SA_NTF_RESPONSE_TIME_EXCESSIVE,
    SA_NTF_RETRANSMISSION_RATE_EXCESSIVE,
    SA_NTF_SOFTWARE_ERROR,
    SA_NTF_SOFTWARE_PROGRAM_ABNORMALLY_TERMINATED,
    SA_NTF_SOFTWARE_PROGRAM_ERROR,
    SA_NTF_STORAGE_CAPACITY_PROBLEM,
    SA_NTF_TEMPERATURE_UNACCEPTABLE,
    SA_NTF_THRESHOLD_CROSSED,
    SA_NTF_TIMING_PROBLEM,
    SA_NTF_TOXIC_LEAK_DETECTED,
    SA_NTF_TRANSMIT_FAILURE,
    SA_NTF_TRANSMITTER_FAILURE,
    SA_NTF_UNDERLYING_RESOURCE_UNAVAILABLE,
    SA_NTF_VERSION_MISMATCH,
    SA_NTF_AUTHENTICATION_FAILURE,
    SA_NTF_BREACH_OF_CONFIDENTIALITY,
    SA_NTF_CABLE_TAMPER,
    SA_NTF_DELAYED_INFORMATION,
    SA_NTF_DENIAL_OF_SERVICE,
    SA_NTF_DUPLICATE_INFORMATION,
    SA_NTF_INFORMATION_MISSING,
    SA_NTF_INFORMATION_MODIFICATION_DETECTED,
    SA_NTF_INFORMATION_OUT_OF_SEQUENCE,
    SA_NTF_INTRUSION_DETECTION,
    SA_NTF_KEY_EXPIRED,
    SA_NTF_NON_REPUDIATION_FAILURE,
    SA_NTF_OUT_OF_HOURS_ACTIVITY,
    SA_NTF_OUT_OF_SERVICE,
    SA_NTF_PROCEDURAL_ERROR,
    SA_NTF_UNAUTHORIZED_ACCESS_ATTEMPT,
    SA_NTF_UNEXPECTED_INFORMATION,
    SA_NTF_UNSPECIFIED_REASON
} SaNtfProbableCauseT;

typedef struct {
    SaNtfElementIdT problemId;
    SaNtfClassIdT problemClassId;
    SaNtfValueTypeT problemType;
    SaNtfValueT problemValue;
} SaNtfSpecificProblemT;

typedef enum {
    SA_NTF_SEVERITY_CLEARED, /* alarm notification, only */
    SA_NTF_SEVERITY_INDETERMINATE,
    SA_NTF_SEVERITY_WARNING,
    SA_NTF_SEVERITY_MINOR,
    SA_NTF_SEVERITY_MAJOR,
    SA_NTF_SEVERITY_CRITICAL
} SaNtfSeverityT;

typedef enum {
    SA_NTF_TREND_MORE_SEVERE,
    SA_NTF_TREND_NO_CHANGE,
    SA_NTF_TREND_LESS_SEVERE
} SaNtfSeverityTrendT;

typedef struct {
    SaNtfElementIdT thresholdId;
    SaNtfValueTypeT thresholdValueType;
    SaNtfValueT thresholdValue;
    SaNtfValueT thresholdHysteresis;
    /* This has to be of the same type as threshold */
    SaNtfValueT observedValue;
    SaTimeT armTime;
} SaNtfThresholdInformationT;

typedef struct {
    SaNtfElementIdT actionId;
    SaNtfValueTypeT actionValueType;
    SaNtfValueT actionValue;
} SaNtfProposedRepairActionT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
    SaUint16T numSpecificProblems;
    SaUint16T numMonitoredAttributes;
    SaUint16T numProposedRepairActions;
    SaNtfProbableCauseT *probableCause;
    SaNtfSpecificProblemT *specificProblems;
    SaNtfSeverityT *perceivedSeverity;
    SaNtfSeverityTrendT *trend;
    SaNtfThresholdInformationT *thresholdInformation;
    SaNtfAttributeT *monitoredAttributes;
    SaNtfProposedRepairActionT *proposedRepairActions;
} SaNtfAlarmNotificationT;

typedef struct {
    SaNtfValueTypeT valueType;
    SaNtfValueT value;
} SaNtfSecurityAlarmDetectorT;

typedef struct {
    SaNtfValueTypeT valueType;
    SaNtfValueT value;
} SaNtfServiceUserT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
    SaNtfProbableCauseT *probableCause;
    SaNtfSeverityT *severity;
    SaNtfSecurityAlarmDetectorT *securityAlarmDetector;
    SaNtfServiceUserT*serviceUser;
    SaNtfServiceUserT *serviceProvider;
} SaNtfSecurityAlarmNotificationT;

typedef struct {
    SaNtfNotificationHandleT notificationHandle;
    SaNtfNotificationHeaderT notificationHeader;
} SaNtfMiscellaneousNotificationT;

/* 
 * Default for API user when not sure how much memory is in sum needed to 
 * accommodate the variable size data 
 */
#define SA_NTF_ALLOC_SYSTEM_LIMIT    (-1)

typedef SaUint32T SaNtfSubscriptionIdT; 

typedef struct {
    SaUint16T numEventTypes;
    SaNtfEventTypeT *eventTypes;
    SaUint16T numNotificationObjects;
    SaNameT *notificationObjects;
    SaUint16T numNotifyingObjects;
    SaNameT *notifyingObjects;
    SaUint16T numNotificationClassIds;
    SaNtfClassIdT *notificationClassIds;
} SaNtfNotificationFilterHeaderT;

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numSourceIndicators;
    SaNtfSourceIndicatorT *sourceIndicators;
} SaNtfObjectCreateDeleteNotificationFilterT;

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numSourceIndicators;
    SaNtfSourceIndicatorT *sourceIndicators;
} SaNtfAttributeChangeNotificationFilterT;

#ifdef SA_NTF_A01
typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numSourceIndicators;
    SaNtfSourceIndicatorT *sourceIndicators;
    SaUint16T numStateChanges;
    SaNtfStateChangeT *changedStates;
} SaNtfStateChangeNotificationFilterT;
#endif /* SA_NTF_A01 */

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numSourceIndicators;
    SaNtfSourceIndicatorT *sourceIndicators;
    SaUint16T numStateChanges;
    SaNtfElementIdT *stateId;
} SaNtfStateChangeNotificationFilterT_2;

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numProbableCauses;
    SaUint16T numPerceivedSeverities;
    SaUint16T numTrends;
    SaNtfProbableCauseT *probableCauses;
    SaNtfSeverityT *perceivedSeverities;
    SaNtfSeverityTrendT *trends;
} SaNtfAlarmNotificationFilterT;

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
    SaUint16T numProbableCauses;
    SaUint16T numSeverities;
    SaUint16T numSecurityAlarmDetectors;
    SaUint16T numServiceUsers;
    SaUint16T numServiceProviders;
    SaNtfProbableCauseT *probableCauses;
    SaNtfSeverityT *severities;
    SaNtfSecurityAlarmDetectorT *securityAlarmDetectors;
    SaNtfServiceUserT *serviceUsers;
    SaNtfServiceUserT *serviceProviders;
} SaNtfSecurityAlarmNotificationFilterT;

typedef struct {
    SaNtfNotificationFilterHandleT notificationFilterHandle;
    SaNtfNotificationFilterHeaderT notificationFilterHeader;
} SaNtfMiscellaneousNotificationFilterT;

typedef enum {
    SA_NTF_SEARCH_BEFORE_OR_AT_TIME = 1,
    SA_NTF_SEARCH_AT_TIME = 2,
    SA_NTF_SEARCH_AT_OR_AFTER_TIME = 3,
    SA_NTF_SEARCH_BEFORE_TIME = 4,
    SA_NTF_SEARCH_AFTER_TIME = 5,
    SA_NTF_SEARCH_NOTIFICATION_ID = 6,
    SA_NTF_SEARCH_ONLY_FILTER = 7
} SaNtfSearchModeT;

typedef struct {
    SaNtfSearchModeT searchMode;
    SaTimeT eventTime;
    /* 
     * event time (relevant only if searchMode is one of SA_NTF_SEARCH_*_TIME) 
     */

    SaNtfIdentifierT notificationId;
    /* 
     * notification ID (relevant only if searchMode is 
     * SA_NTF_SEARCH_NOTIFICATION_ID) 
     */
} SaNtfSearchCriteriaT;

typedef enum {
    SA_NTF_SEARCH_OLDER = 1,
    SA_NTF_SEARCH_YOUNGER = 2
} SaNtfSearchDirectionT;

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
typedef struct {
    SaNtfNotificationFilterHandleT objectCreateDeleteFilterHandle;
    SaNtfNotificationFilterHandleT attributeChangeFilterHandle;
    SaNtfNotificationFilterHandleT stateChangeFilterHandle;
    SaNtfNotificationFilterHandleT alarmFilterHandle;
    SaNtfNotificationFilterHandleT securityAlarmFilterHandle;
} SaNtfNotificationTypeFilterHandlesT;
#endif /* SA_NTF_A01 || SA_NTF_A02 */

typedef struct {
    SaNtfNotificationFilterHandleT objectCreateDeleteFilterHandle;
    SaNtfNotificationFilterHandleT attributeChangeFilterHandle;
    SaNtfNotificationFilterHandleT stateChangeFilterHandle;
    SaNtfNotificationFilterHandleT alarmFilterHandle;
    SaNtfNotificationFilterHandleT securityAlarmFilterHandle;
    SaNtfNotificationFilterHandleT miscellaneousFilterHandle;
} SaNtfNotificationTypeFilterHandlesT_3;

#define SA_NTF_FILTER_HANDLE_NULL       ((SaNtfNotificationFilterHandleT) 0)

typedef struct {
    SaNtfNotificationTypeT notificationType;
    union {
        SaNtfObjectCreateDeleteNotificationT objectCreateDeleteNotification;
        SaNtfAttributeChangeNotificationT    attributeChangeNotification;
        SaNtfStateChangeNotificationT        stateChangeNotification;
        SaNtfAlarmNotificationT              alarmNotification;
        SaNtfSecurityAlarmNotificationT      securityAlarmNotification;
    } notification;
} SaNtfNotificationsT;

typedef enum {
    SA_NTF_STATIC_FILTER_STATE = 1,
    SA_NTF_SUBSCRIBER_STATE = 2,
} SaNtfStateT;

typedef enum {
    SA_NTF_STATIC_FILTER_STATE_INACTIVE = 1,
    SA_NTF_STATIC_FILTER_STATE_ACTIVE = 2,
} SaNtfStaticFilterStateT;

typedef enum {
    SA_NTF_SUBSCRIBER_STATE_FORWARD_NOT_OK = 1,
    SA_NTF_SUBSCRIBER_STATE_FORWARD_OK = 2,
} SaNtfSubscriberStateT;

typedef enum {
    SA_NTF_NTFID_STATIC_FILTER_ACTIVATED = 0x065,
    SA_NTF_NTFID_STATIC_FILTER_DEACTIVATED = 0x066,
    SA_NTF_NTFID_CONSUMER_SLOW = 0x067,
    SA_NTF_NTFID_CONSUMER_FAST_ENOUGH = 0x068
} SaNtfNotificationMinorIdT;

typedef void 
(*SaNtfNotificationCallbackT)(
    SaNtfSubscriptionIdT subscriptionId,
    const SaNtfNotificationsT *notification);

typedef void 
(*SaNtfNotificationDiscardedCallbackT)(
    SaNtfSubscriptionIdT subscriptionId,
    SaNtfNotificationTypeT notificationType,
    SaUint32T numberDiscarded,
    const SaNtfIdentifierT *discardedNotificationIdentifiers);

#ifdef SA_NTF_A01
typedef struct {
    SaNtfNotificationCallbackT saNtfNotificationCallback;
    SaNtfNotificationDiscardedCallbackT saNtfNotificationDiscardedCallback;
} SaNtfCallbacksT;

extern SaAisErrorT
saNtfInitialize(
    SaNtfHandleT *ntfHandle,
    const SaNtfCallbacksT *ntfCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saNtfLocalizedMessageFree(
    SaStringT message);

extern SaAisErrorT 
saNtfStateChangeNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfStateChangeNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numSourceIndicators,
    SaUint32T numChangedStates);

extern SaAisErrorT 
saNtfNotificationUnsubscribe(
    SaNtfSubscriptionIdT subscriptionId);

extern SaAisErrorT 
saNtfNotificationReadInitialize(
    SaNtfSearchCriteriaT searchCriteria,
    const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
    SaNtfReadHandleT *readHandle);
#endif /* SA_NTF_A01 */

#ifdef SA_NTF_A02
typedef void 
(*SaNtfStaticSuppressionFilterSetCallbackT)(
    SaNtfHandleT ntfHandle,
    SaUint16T notificationTypeBitmap);

typedef struct {
    SaNtfNotificationCallbackT saNtfNotificationCallback;
    SaNtfNotificationDiscardedCallbackT saNtfNotificationDiscardedCallback;
    SaNtfStaticSuppressionFilterSetCallbackT
        saNtfStaticSuppressionFilterSetCallback;
} SaNtfCallbacksT_2;

extern SaAisErrorT
saNtfInitialize_2(
    SaNtfHandleT *ntfHandle,
    const SaNtfCallbacksT_2 *ntfCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saNtfNotificationReadInitialize_2(
    const SaNtfSearchCriteriaT *searchCriteria,
    const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
    SaNtfReadHandleT *readHandle);
#endif /* SA_NTF_A02 */

#if defined(SA_NTF_A01) || defined(SA_NTF_A02)
extern SaAisErrorT 
saNtfNotificationSubscribe(
    const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
    SaNtfSubscriptionIdT subscriptionId);
#endif /* SA_NTF_A01 || SA_NTF_A02 */

typedef void 
(*SaNtfStaticSuppressionFilterSetCallbackT_3)(
    SaNtfHandleT ntfHandle,
    SaNtfEventTypeBitmapT eventTypeBitmap);

typedef struct {
    SaNtfNotificationCallbackT saNtfNotificationCallback;
    SaNtfNotificationDiscardedCallbackT saNtfNotificationDiscardedCallback;
    SaNtfStaticSuppressionFilterSetCallbackT_3 saNtfStaticSuppressionFilterSetCallback;
} SaNtfCallbacksT_3;

extern SaAisErrorT 
saNtfInitialize_3(
    SaNtfHandleT *ntfHandle,
    const SaNtfCallbacksT_3 *ntfCallbacks,
    SaVersionT *version);

extern SaAisErrorT
saNtfSelectionObjectGet(
    SaNtfHandleT ntfHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saNtfDispatch(
    SaNtfHandleT ntfHandle,
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saNtfFinalize(
    SaNtfHandleT ntfHandle);

extern SaAisErrorT 
saNtfObjectCreateDeleteNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfObjectCreateDeleteNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numAttributes,
    SaInt16T variableDataSize);

extern SaAisErrorT 
saNtfAttributeChangeNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAttributeChangeNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numAttributes,
    SaInt16T variableDataSize);

extern SaAisErrorT 
saNtfStateChangeNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfStateChangeNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numStateChanges,
    SaInt16T variableDataSize);

extern SaAisErrorT 
saNtfAlarmNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAlarmNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaUint16T numSpecificProblems,
    SaUint16T numMonitoredAttributes,
    SaUint16T numProposedRepairActions,
    SaInt16T variableDataSize);

extern SaAisErrorT 
saNtfSecurityAlarmNotificationAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfSecurityAlarmNotificationT *notification,
    SaUint16T numCorrelatedNotifications,
    SaUint16T lengthAdditionalText,
    SaUint16T numAdditionalInfo,
    SaInt16T variableDataSize);    

extern SaAisErrorT 
saNtfPtrValAllocate(
    SaNtfNotificationHandleT notificationHandle,
    SaUint16T dataSize,
    void **dataPtr,
    SaNtfValueT *value);

extern SaAisErrorT 
saNtfArrayValAllocate(
    SaNtfNotificationHandleT notificationHandle,
    SaUint16T numElements,
    SaUint16T elementSize,
    void **arrayPtr,
    SaNtfValueT *value);

extern SaAisErrorT 
saNtfNotificationSend(
    SaNtfNotificationHandleT notificationHandle);

extern SaAisErrorT 
saNtfNotificationFree(
    SaNtfNotificationHandleT notificationHandle);

extern SaAisErrorT
saNtfVariableDataSizeGet(
    SaNtfNotificationHandleT notificationHandle,
    SaUint16T *variableDataSpaceAvailable);

extern SaAisErrorT 
saNtfLocalizedMessageGet(
    SaNtfNotificationHandleT notificationHandle,
    SaStringT *message);

extern SaAisErrorT 
saNtfLocalizedMessageFree_2(
    SaNtfHandleT ntfHandle,
    SaStringT message);

extern SaAisErrorT saNtfPtrValGet(
    SaNtfNotificationHandleT notificationHandle,
    SaNtfValueT *value,
    void **dataPtr,
    SaUint16T *dataSize);

extern SaAisErrorT 
saNtfArrayValGet(
    SaNtfNotificationHandleT notificationHandle,
    SaNtfValueT *value,
    void **arrayPtr,
    SaUint16T *numElements,
    SaUint16T *elementSize);

extern SaAisErrorT 
saNtfObjectCreateDeleteNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint16T numSourceIndicators);

extern SaAisErrorT 
saNtfAttributeChangeNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAttributeChangeNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numSourceIndicators);

extern SaAisErrorT 
saNtfStateChangeNotificationFilterAllocate_2( 
    SaNtfHandleT ntfHandle,
    SaNtfStateChangeNotificationFilterT_2 *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numSourceIndicators,
    SaUint32T numChangedStates);

extern SaAisErrorT 
saNtfAlarmNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfAlarmNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numProbableCauses,
    SaUint32T numPerceivedSeverities,
    SaUint32T numTrends);

extern SaAisErrorT 
saNtfSecurityAlarmNotificationFilterAllocate( 
    SaNtfHandleT ntfHandle,
    SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
    SaUint16T numEventTypes,
    SaUint16T numNotificationObjects,
    SaUint16T numNotifyingObjects,
    SaUint16T numNotificationClassIds,
    SaUint32T numProbableCauses,
    SaUint32T numSeverities,
    SaUint32T numSecurityAlarmDetectors,
    SaUint32T numServiceUsers,
    SaUint32T numServiceProviders);

extern SaAisErrorT 
saNtfNotificationFilterFree(
    SaNtfNotificationFilterHandleT notificationFilterHandle);

extern SaAisErrorT 
saNtfNotificationSubscribe_3(
    const SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles,
    SaNtfSubscriptionIdT subscriptionId);

extern SaAisErrorT 
saNtfNotificationReadInitialize_3(
   const SaNtfSearchCriteriaT *searchCriteria,
   const SaNtfNotificationTypeFilterHandlesT_3 *notificationFilterHandles,
   SaNtfReadHandleT *readHandle);

extern SaAisErrorT 
saNtfNotificationUnsubscribe_2(
    SaNtfHandleT ntfHandle,
    SaNtfSubscriptionIdT subscriptionId);

extern SaAisErrorT 
saNtfNotificationReadNext(
    SaNtfReadHandleT readHandle,
    SaNtfSearchDirectionT searchDirection,
    SaNtfNotificationsT *notification);

extern 
SaAisErrorT saNtfNotificationReadFinalize(
        SaNtfReadHandleT readHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SA_NTF_H */

