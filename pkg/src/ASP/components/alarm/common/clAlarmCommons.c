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
 * File        : clAlarmCommons.c
 *******************************************************************************/

/**************************************************************************
 * Description :                                                                
 * This module has functions that are used by both the alarm              
 * client and server for logging messages                                 
 **************************************************************************/

/* system includes */

/* ASP includes */
#include <clCommon.h>
#include <clDebugApi.h>

/* alarm includes */
#include <clAlarmCommons.h>

#define CL_ALARM_CATEGORY_ENVIRONMENTAL_INTERNAL       0x01<<0
#define CL_ALARM_CATEGORY_EQUIPMENT_INTERNAL           0x01<<1
#define CL_ALARM_CATEGORY_PROCESSING_ERROR_INTERNAL    0x01<<2
#define CL_ALARM_CATEGORY_QUALITY_OF_SERVICE_INTERNAL  0x01<<3
#define CL_ALARM_CATEGORY_COMMUNICATIONS_INTERNAL      0x01<<4

/*
 *  This contains the definition of 
 *  the alarm log messages
 */
ClCharT *clAlarmLogMsg[CL_ALARM_LOG_MAX_MSG]={
    "Rmd failed while calling [%s]",
    "Registration of alarm component with [%s] failed",
    "Deregistration of alarm component with [%s] failed",
    "Initialization of [%s] failed",
    "Finalization of [%s] failed"
};

ClVersionT gAlarmClientToServerVersionT[] = {{'B', 0x1, 0x1}};
ClVersionDatabaseT gAlarmClientToServerVersionDb = {1, gAlarmClientToServerVersionT};

ClCharT *clAlarmCategoryString[]={
    "CL_ALARM_CATEGORY_UNKNOWN",
    "CL_ALARM_CATEGORY_COMMUNICATIONS",
    "CL_ALARM_CATEGORY_QUALITY_OF_SERVICE",
    "CL_ALARM_CATEGORY_PROCESSING_ERROR",
    "CL_ALARM_CATEGORY_EQUIPMENT",
    "CL_ALARM_CATEGORY_ENVIRONMENTAL"
};

ClCharT *clAlarmSeverityString[]={
    "CL_ALARM_SEVERITY_UNKNOWN",
    "CL_ALARM_SEVERITY_CRITICAL",
    "CL_ALARM_SEVERITY_MAJOR",
    "CL_ALARM_SEVERITY_MINOR",
    "CL_ALARM_SEVERITY_WARNING",
    "CL_ALARM_SEVERITY_INDETERMINATE",
    "CL_ALARM_SEVERITY_CLEAR"
};


ClCharT* clAlarmProbableCauseString[]={
    "CL_ALARM_PROB_CAUSE_UNKNOWN",
    /**
     * Probable cause for Communication related alarms
     */
    "CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL",
    "CL_ALARM_PROB_CAUSE_LOSS_OF_FRAME",
    "CL_ALARM_PROB_CAUSE_FRAMING_ERROR",
    "CL_ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR",
    "CL_ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR",
    "CL_ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR",
    "CL_ALARM_PROB_CAUSE_DEGRADED_SIGNAL",
    "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE",
    "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR",
    "CL_ALARM_PROB_CAUSE_LAN_ERROR",
    "CL_ALARM_PROB_CAUSE_DTE",
    /**
     * Probable cause for Quality of Service related alarms
     */
    "CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE",
    "CL_ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED",
    "CL_ALARM_PROB_CAUSE_BANDWIDTH_REDUCED",
    "CL_ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE",
    "CL_ALARM_PROB_CAUSE_THRESHOLD_CROSSED",
    "CL_ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED",
    "CL_ALARM_PROB_CAUSE_CONGESTION",
    "CL_ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY",
    /**
     * Probable cause for Processing Error related alarms
     */
    "CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM",
    "CL_ALARM_PROB_CAUSE_VERSION_MISMATCH",
    "CL_ALARM_PROB_CAUSE_CORRUPT_DATA",
    "CL_ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED",
    "CL_ALARM_PROB_CAUSE_SOFWARE_ERROR",
    "CL_ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR",
    "CL_ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED",
    "CL_ALARM_PROB_CAUSE_FILE_ERROR",
    "CL_ALARM_PROB_CAUSE_OUT_OF_MEMORY",
    "CL_ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE",
    "CL_ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE",
    "CL_ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR",
    /**
     * Probable cause for Equipment related alarms
     */
    "CL_ALARM_PROB_CAUSE_POWER_PROBLEM",
    "CL_ALARM_PROB_CAUSE_TIMING_PROBLEM",
    "CL_ALARM_PROB_CAUSE_PROCESSOR_PROBLEM",
    "CL_ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR",
    "CL_ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM",
    "CL_ALARM_PROB_CAUSE_RECEIVER_FAILURE",
    "CL_ALARM_PROB_CAUSE_TRANSMITTER_FAILURE",
    "CL_ALARM_PROB_CAUSE_RECEIVE_FAILURE",
    "CL_ALARM_PROB_CAUSE_TRANSMIT_FAILURE",
    "CL_ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR",
    "CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION",
    "CL_ALARM_PROB_CAUSE_ADAPTER_ERROR",
    /**
     * Probable cause for Environmental related alarms
     */
    "CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM",
    "CL_ALARM_PROB_CAUSE_FIRE_DETECTED",
    "CL_ALARM_PROB_CAUSE_FLOOD_DETECTED",
    "CL_ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED",
    "CL_ALARM_PROB_CAUSE_LEAK_DETECTED",
    "CL_ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE",
    "CL_ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION",
    "CL_ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED",
    "CL_ALARM_PROB_CAUSE_PUMP_FAILURE",
    "CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN"
};

ClUint32T gClAlarmProbCauseStringCount = sizeof(clAlarmProbableCauseString)/sizeof(clAlarmProbableCauseString[0]);
ClUint32T gClAlarmSeverityStringCount = sizeof(clAlarmSeverityString)/sizeof(clAlarmSeverityString[0]);
ClUint32T gClAlarmCategoryStringCount = sizeof(clAlarmCategoryString)/sizeof(clAlarmCategoryString[0]);

ClUint8T
clAlarmCategory2BitmapTranslate(ClAlarmCategoryTypeT category){
    
    switch(category)
    {
       case CL_ALARM_CATEGORY_COMMUNICATIONS:
		    return CL_ALARM_CATEGORY_COMMUNICATIONS_INTERNAL;

       case CL_ALARM_CATEGORY_QUALITY_OF_SERVICE:
		    return CL_ALARM_CATEGORY_QUALITY_OF_SERVICE_INTERNAL;

       case CL_ALARM_CATEGORY_PROCESSING_ERROR:
            return CL_ALARM_CATEGORY_PROCESSING_ERROR_INTERNAL; 

       case CL_ALARM_CATEGORY_EQUIPMENT:
			return CL_ALARM_CATEGORY_EQUIPMENT_INTERNAL;

       case CL_ALARM_CATEGORY_ENVIRONMENTAL:
			return CL_ALARM_CATEGORY_ENVIRONMENTAL_INTERNAL;

       default :
	        return CL_OK;
    }
}

ClInt32T clAlarmProbCauseStringToValueGet(ClCharT* str)
{
    ClInt32T i = 0;

    for (i=0; i < gClAlarmProbCauseStringCount; i++)
    {
        if (! strcmp(clAlarmProbableCauseString[i], str))
            return i;
    }

    return CL_ALARM_ID_INVALID;
}

ClInt32T clAlarmSeverityStringToValueGet(ClCharT* str)
{
    ClInt32T i = 0;

    for (i=0; i < gClAlarmSeverityStringCount; i++)
    {
        if (! strcmp(clAlarmSeverityString[i], str))
            return i;
    }

    return CL_ALARM_SEVERITY_INVALID;
}

ClInt32T clAlarmCategoryStringToValueGet(ClCharT* str)
{
    ClInt32T i = 0;

    for (i=0; i < gClAlarmCategoryStringCount; i++)
    {
        if (! strcmp(clAlarmCategoryString[i], str))
            return i;
    }

    return CL_ALARM_CATEGORY_INVALID;
}
