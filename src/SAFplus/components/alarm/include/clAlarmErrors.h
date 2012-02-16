/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : alarm
 * File        : clAlarmErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * Description: This file contains alarm service related error codes
 *
 *
 *****************************************************************************/

/**
 *  \file 
 *  \brief Header file of Alarm Service related Error codes
 *  \ingroup alarm_apis
 */

/**
 *  \addtogroup alarm_apis
 *  \{
 */

#ifndef _CL_ALARM_ERRORS_H_
#define _CL_ALARM_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/

/**
 * Alarm service related error codes 
 */

/** 
 *  The alarm category is invalid.
 */
#define CL_ALARM_ERR_INVALID_CAT                0x100 
    
/** 
 *  Client Library version error.
 */
#define CL_ALARM_ERR_VERSION_UNSUPPORTED        0x101

/** 
 *  The alarm severity is invalid.
 */
#define CL_ALARM_ERR_INVALID_SEVERITY           0x102
    
/** 
 *  The probable cause of alarm is not present.
 */
#define CL_ALARM_ERR_PROB_CAUSE_NOT_PRESENT     0x103
    
/** 
 *  The Generation Rule of alarm is invalid.
 */
#define CL_ALARM_ERR_GEN_RULE_NOT_VALID         0x104
    
/** 
 *  The Suppression Rule of alarm is invalid.
 */
#define CL_ALARM_ERR_SUPP_RULE_NOT_VALID        0x105
    
/** 
 *  The OM class creation of alarm failed.
 */
#define CL_ALARM_ERR_OM_CREATE_FAILED           0x106
    
/** 
 *  Internal error related to alarms.
 */
#define CL_ALARM_ERR_INTERNAL_ERROR             0x107

/**
 * The event initialization failure.
 */
#define CL_ALARM_ERR_EVT_INIT                   0x108

/**
 * The event channel open failure for alarm event channel.
 */
#define CL_ALARM_ERR_EVT_CHANNEL_OPEN           0x109

/**
 * Failed while subscribing for the event.
 */
#define CL_ALARM_ERR_EVT_SUBSCRIBE              0x10a

/**
 * The event initialization had been done already.
 */
#define CL_ALARM_ERR_EVT_SUBSCRIBE_AGAIN        0x10b

/**
 * The event channel close failure while unsubscribing for the event.
 */
#define CL_ALARM_ERR_EVT_CHANNEL_CLOSE          0x10c

/**
 * The event finalize failure while unsubscribing for the event.
 */
#define CL_ALARM_ERR_EVT_FINALIZE               0x10d

/**
 * The managed object Identifier is invalid.
 */
#define CL_ALARM_ERR_INVALID_MOID               0x10e

/**
 * The managed object doesn't have any owner registered in COR.
 */
#define CL_ALARM_ERR_NO_OWNER                   0x10f



/******************************************************************************
 * ERROR/RETRUN CODE HANDLING MACROS
 *****************************************************************************/
/**
 * It appends the component ID with the error code.
 */ 
#define CL_ALARM_RC(ERROR_CODE)                 CL_RC(CL_CID_ALARMS, (ERROR_CODE))

/**
 * The pointer passed is NULL.
 */
#define CL_ALARM_ERR_NULL_POINTER               CL_ERR_NULL_POINTER

/**
 * There is no memory left with the application. 
 */
#define CL_ALARM_ERR_NO_MEMORY                  CL_ERR_NO_MEMORY

/**
 * The parameter passed in invalid 
 */
#define CL_ALARM_ERR_INVALID_PARAM              CL_ERR_INVALID_PARAMETER

#ifdef __cplusplus
}
#endif

#endif /* _CL_ALARM_ERRORS_H_ */

/** \} */
