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
 * ModuleName  : fault                                                         
 * File        : clFaultClientServerCommons.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This module has functions that are used by both the fault              
 * client and server                                                      
 *******************************************************************************/

/* system includes */

/* ASP includes */

/* fault includes */
#include <clFaultErrorId.h>
#include <clFaultDefinitions.h>
#include "clFaultDS.h"
#include "clFaultLog.h"
#include <clVersionApi.h>
#include <clCorUtilityApi.h>
#include <clCorApi.h>
#include <clCpmApi.h>
#include <clDebugApi.h>

ClVersionT gFaultClientToServerVersionT[] = {{'B', 0x1, 0x1}};
ClVersionDatabaseT gFaultClientToServerVersionDb = {1, gFaultClientToServerVersionT};


/*
 *  This contains the definition of 
 *  the fault log messages
 */
ClCharT *clFaultLogMsg[CL_FAULT_LOG_MAX_MSG]={
    "Rmd failed while calling [%s]",
    "Error in mutex    creation/locking/unlocking in [%s]",
    "Registration of fault component with [%s] failed",
    "Deregistration of fault component with [%s] failed",
    "Initialization of [%s] failed",
    "Finalization of [%s] failed"
};

ClCharT *clFaultCategoryString[]={
    "CL_ALARM_CATEGORY_UNKNOWN",
    "CL_ALARM_CATEGORY_COMMUNICATIONS",
    "CL_ALARM_CATEGORY_QUALITY_OF_SERVICE",
    "CL_ALARM_CATEGORY_PROCESSING_ERROR",
    "CL_ALARM_CATEGORY_EQUIPMENT",
    "CL_ALARM_CATEGORY_ENVIRONMENTAL"
};
ClCharT *clFaultSeverityString[]={
    "CL_ALARM_SEVERITY_UNKNOWN",
    "CL_ALARM_SEVERITY_CRITICAL",
    "CL_ALARM_SEVERITY_MAJOR",
    "CL_ALARM_SEVERITY_MINOR",
    "CL_ALARM_SEVERITY_WARNING",
    "CL_ALARM_SEVERITY_INDETERMINATE",
    "CL_ALARM_SEVERITY_CLEAR"
};
ClCharT *clFaultProbableCauseString[]={
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

/*
 *  converting from the internal 
 *  category value to corresponding 
 *  index 
 */
   
ClRcT 
clFaultCategory2IndexTranslate(ClAlarmCategoryTypeT category, ClUint8T *pCatIndex){
    
    switch(category)
    {
       case CL_FM_CAT_COM_INTERNAL:
            *pCatIndex = CL_FM_CAT_COM_X;
            break;

       case CL_FM_CAT_QOS_INTERNAL:
            *pCatIndex = CL_FM_CAT_QOS_X;
            break;

       case CL_FM_CAT_PROC_INTERNAL:
            *pCatIndex = CL_FM_CAT_PROC_X;
            break;

       case CL_FM_CAT_EQUIP_INTERNAL:
            *pCatIndex = CL_FM_CAT_EQUIP_X;
            break;

       case CL_FM_CAT_ENV_INTERNAL:
            *pCatIndex = CL_FM_CAT_ENV_X;
            break;

       default:
            return CL_FAULT_RC(CL_ERR_INVALID_PARAMETER);
             
    }
    return CL_OK;
}


/*
 *  converting from the internal 
 *  severity value to corresponding 
 *  index 
 */

ClRcT 
clFaultSeverity2IndexTranslate(ClAlarmSeverityTypeT severity, ClUint8T *pSevIndex){
    
    switch(severity)
    {
       case CL_FM_SEV_IN_INTERNAL:
            *pSevIndex = CL_FM_SEV_IN_X;
            break;

       case CL_FM_SEV_WR_INTERNAL:
            *pSevIndex = CL_FM_SEV_WR_X;
            break;

       case CL_FM_SEV_MN_INTERNAL:
            *pSevIndex = CL_FM_SEV_MN_X;
            break;

       case CL_FM_SEV_MJ_INTERNAL:
            *pSevIndex = CL_FM_SEV_MJ_X;
            break;

       case CL_FM_SEV_CR_INTERNAL:
            *pSevIndex = CL_FM_SEV_CR_X;
            break;

       case CL_FM_SEV_CL_INTERNAL:
            *pSevIndex = CL_FM_SEV_CL_X;
            break;

       default:
             return CL_FAULT_RC(CL_ERR_INVALID_PARAMETER);
    }
    return CL_OK;
}

/*
 *  converting from the default 
 *  category value to corresponding 
 *  internal value 
 */

ClRcT 
clFaultCategory2InternalTranslate(ClAlarmCategoryTypeT category){
    
    switch(category)
    {
       case CL_ALARM_CATEGORY_COMMUNICATIONS :
            return CL_FM_CAT_COM_INTERNAL;

       case CL_ALARM_CATEGORY_QUALITY_OF_SERVICE :
            return CL_FM_CAT_QOS_INTERNAL;

       case CL_ALARM_CATEGORY_PROCESSING_ERROR:
            return CL_FM_CAT_PROC_INTERNAL;

       case CL_ALARM_CATEGORY_EQUIPMENT :
            return CL_FM_CAT_EQUIP_INTERNAL;

       case CL_ALARM_CATEGORY_ENVIRONMENTAL :
            return CL_FM_CAT_ENV_INTERNAL;
      
       default :
            return CL_OK;

    }
}


/*
 *  converting from the default 
 *  severity value to corresponding 
 *  internal value 
 */

ClRcT 
clFaultSeverity2InternalTranslate(ClAlarmSeverityTypeT severity){
    
    switch(severity)
    {
       case CL_ALARM_SEVERITY_INDETERMINATE:
            return CL_FM_SEV_IN_INTERNAL;

       case CL_ALARM_SEVERITY_WARNING:
            return CL_FM_SEV_WR_INTERNAL;

       case CL_ALARM_SEVERITY_MINOR:
            return CL_FM_SEV_MN_INTERNAL;

       case CL_ALARM_SEVERITY_MAJOR:
            return CL_FM_SEV_MJ_INTERNAL;

       case CL_ALARM_SEVERITY_CRITICAL:
            return CL_FM_SEV_CR_INTERNAL;

       case CL_ALARM_SEVERITY_CLEAR:
            return CL_FM_SEV_CL_INTERNAL;

       default :
            return CL_OK;
    }
}

/*
 *  converting back the internal 
 *  category value to corresponding 
 *  default value 
 */

ClRcT 
clFaultInternal2CategoryTranslate(ClAlarmCategoryTypeT category){
    
    switch(category)
    {
       case CL_FM_CAT_COM_INTERNAL:
            return CL_ALARM_CATEGORY_COMMUNICATIONS;

       case CL_FM_CAT_QOS_INTERNAL:
            return CL_ALARM_CATEGORY_QUALITY_OF_SERVICE;

       case CL_FM_CAT_PROC_INTERNAL:
            return CL_ALARM_CATEGORY_PROCESSING_ERROR;

       case CL_FM_CAT_EQUIP_INTERNAL:
            return CL_ALARM_CATEGORY_EQUIPMENT;

       case CL_FM_CAT_ENV_INTERNAL:
            return CL_ALARM_CATEGORY_ENVIRONMENTAL;

       default :
            return CL_FAULT_ERR_INTERNAL;
    }
    return CL_OK;
}


/*
 *  converting back the internal 
 *  severity value to corresponding 
 *  default value 
 */

ClRcT 
clFaultInternal2SeverityTranslate(ClAlarmSeverityTypeT severity){
    
    switch(severity)
    {
       case CL_FM_SEV_IN_INTERNAL:
            return CL_ALARM_SEVERITY_INDETERMINATE;

       case CL_FM_SEV_WR_INTERNAL:
            return CL_ALARM_SEVERITY_WARNING;

       case CL_FM_SEV_MN_INTERNAL:
            return CL_ALARM_SEVERITY_MINOR;

       case CL_FM_SEV_MJ_INTERNAL:
            return CL_ALARM_SEVERITY_MAJOR;

       case CL_FM_SEV_CR_INTERNAL:
            return CL_ALARM_SEVERITY_CRITICAL;

       case CL_FM_SEV_CL_INTERNAL:
            return CL_ALARM_SEVERITY_CLEAR;
       default :
            return CL_FAULT_ERR_INTERNAL;
    }
    return CL_OK;
}
/*
 *  validating the category value 
 */
ClRcT 
clFaultValidateCategory(ClAlarmCategoryTypeT category){
    

   switch(category)
       {
           case CL_ALARM_CATEGORY_COMMUNICATIONS: break;
           case CL_ALARM_CATEGORY_QUALITY_OF_SERVICE: break;
           case CL_ALARM_CATEGORY_PROCESSING_ERROR: break;
           case CL_ALARM_CATEGORY_EQUIPMENT: break;
           case CL_ALARM_CATEGORY_ENVIRONMENTAL: break;
        default:
            return CL_FAULT_ERR_INVALID_CATEGORY;
       }

       return CL_OK;
}


/*
 *  validating the severity value 
 */

ClRcT 
clFaultValidateSeverity(ClAlarmSeverityTypeT severity){

   switch(severity)
       {
           case CL_ALARM_SEVERITY_CRITICAL: break;
           case CL_ALARM_SEVERITY_MAJOR: break;
           case CL_ALARM_SEVERITY_MINOR: break;
           case CL_ALARM_SEVERITY_WARNING: break;
           case CL_ALARM_SEVERITY_INDETERMINATE: break;
           case CL_ALARM_SEVERITY_CLEAR: break;
        default:
            return CL_FAULT_ERR_INVALID_SEVERITY;
       }

       return CL_OK;   
}

