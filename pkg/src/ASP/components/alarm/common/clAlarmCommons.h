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
 * File        : clAlarmCommons.h
 *******************************************************************************/

/*****************************************************************************
 * Description :                                                                
 * This file provides the definitions for predefined log messages
 * for alarm component and also data containers common to
 * both client and server.
 *****************************************************************************/

#ifndef _CL_ALARM_COMMONS_H_
#define _CL_ALARM_COMMONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>
#include <clVersionApi.h>
#include <clAlarmDefinitions.h>

/** 
 *  Alarm RMD time out.
 */
#define CL_ALARM_RMDCALL_TIMEOUT 3000

/** 
 *  Alarm RMD retries.
 */
#define CL_ALARM_RMDCALL_RETRIES 2
                                                                                           
#define CL_ALARM_LOG_MAX_MSG 5    

extern ClCharT *clAlarmLogMsg[];

extern ClCharT *clAlarmCategoryString[];
extern ClCharT *clAlarmSeverityString[];
extern ClCharT *clAlarmProbableCauseString[];

#define CL_ALARM_CLIENT_LIB     "alarm_client"
#define CL_ALARM_SERVER_LIB        NULL 

/**
 *  Alarm client to owner RMD function IDs.
 */ 
#define CL_ALARM_COMP_ALARM_RAISE_IPI   CL_EO_GET_FULL_FN_NUM(CL_ALARM_CLIENT_TABLE_ID,1)
#define CL_ALARM_STATUS_GET_IPI         CL_EO_GET_FULL_FN_NUM(CL_ALARM_CLIENT_TABLE_ID,2)
#define CL_ALARM_PENDING_ALARMS_GET_IPI CL_EO_GET_FULL_FN_NUM(CL_ALARM_CLIENT_TABLE_ID,3)

#define ALARM_CLIENT_FUNC_ID(n)         CL_EO_GET_FULL_FN_NUM(CL_ALARM_CLIENT_TABLE_ID, n)


/* Alarm client to server RMD function IDs */
#define CL_ALARM_SERVER_API_HANDLER     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)
#define CL_ALARM_HANDLE_TO_INFO_GET     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)
#define CL_ALARM_HANDLE_FROM_INFO_GET   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3)
#define CL_ALARM_RESET                  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4)

#define ALARM_SERVER_FUNC_ID(n)      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, n)


#define    CL_ALARM_LOG_1_RMD             clAlarmLogMsg[0] /* Alarm log messages for RMD failure */
#define    CL_ALARM_LOG_1_REGISTER        clAlarmLogMsg[1] /* Alarm log messages when registration failed */
#define    CL_ALARM_LOG_1_DEREGISTER      clAlarmLogMsg[2] /* Alarm log messages when deregistration failed */
#define    CL_ALARM_LOG_1_INITIALIZATION  clAlarmLogMsg[3] /* Alarm log messages when initialization failed */
#define    CL_ALARM_LOG_1_FINALIZATION    clAlarmLogMsg[4] /* Alarm log messages when finalization failed */
    
extern ClUint8T
clAlarmCategory2BitmapTranslate(ClAlarmCategoryTypeT category);

extern ClInt32T clAlarmProbCauseStringToValueGet(ClCharT* str);
extern ClInt32T clAlarmSeverityStringToValueGet(ClCharT* str);
extern ClInt32T clAlarmCategoryStringToValueGet(ClCharT* str);

typedef struct
{
    ClVersionT version;
    ClAlarmInfoT alarmInfo;
}ClAlarmVersionInfoT;

/*
 * Internal Version related definitions
 */
extern ClVersionDatabaseT gAlarmClientToServerVersionDb;

#define clAlarmClientToServerVersionValidate(version) \
        do\
        { \
            if(clVersionVerify(&gAlarmClientToServerVersionDb, (&version) ) != CL_OK) \
            { \
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to validate the client version .")); \
                return CL_ALARM_ERR_VERSION_UNSUPPORTED;  \
            } \
        }while(0) 


#define CL_ALARM_VERSION                {'B', 0x1, 0x1}
#define CL_ALARM_VERSION_SET(version)     \
        do {                             \
            version.releaseCode = 'B' ; \
            version.majorVersion = 0x1; \
            version.minorVersion = 0x1; \
        }while(0)

/**
 * Macro to log the function entry. 
 */

#define CL_ALARM_FUNC_ENTER() \
do { \
    clLog(CL_LOG_SEV_TRACE,CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, "Entering [%s]", __FUNCTION__); \
}while(0)

/**
 * Macro to log the function exit. 
 */

#define CL_ALARM_FUNC_EXIT() \
do { \
    clLog(CL_LOG_SEV_TRACE,CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, "Leaving [%s]", __FUNCTION__); \
}while(0)


#ifdef __cplusplus
}
#endif


#endif /* _CL_ALARM_COMMONS_H_ */
