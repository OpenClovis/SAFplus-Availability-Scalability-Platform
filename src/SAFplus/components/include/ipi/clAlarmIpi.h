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
 * ModuleName  : include
 * File        : clAlarmIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the interfaces for Alarm used internally by other ASP
 * componets.  
 *
 *
 *****************************************************************************/

#ifndef _CL_ALARM_IPI_H_
#define _CL_ALARM_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clAlarmDefinitions.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
/** 
 *  Maximum number of alarms for an alarm MSO.
 */
#define CL_ALARM_MAX_ALARMS 64

/** 
 *  Macro for the attribute used to inform SNMP whether an alarm 
 *  strucuture in the MSO is a valid entry or not.
 */
#define CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY 1 


/****************************************************************************
 * Assigning enum values to the alarm class Ids & attributes which
 * are a part of alarm MSO.
 ****************************************************************************/
typedef enum{
	CL_ALARM_COR_CLASS_ID_START=0,
	
	CL_ALARM_BASE_CLASS,
	CL_ALARM_INFO_CLASS,
	CL_ALARM_INFO_CONT_CLASS,

	CL_ALARM_COR_CLASS_ID_END,
}ClAlarmMsoClassIdT;

typedef enum
{
	CL_ALARM_COR_ATTR_START =CL_ALARM_COR_CLASS_ID_END,
	
	CL_ALARM_SIMPLE_ATTR_START,
	CL_ALARM_POLLING_INTERVAL,
	CL_ALARM_PUBLISHED_ALARMS, 
	CL_ALARM_SUPPRESSED_ALARMS,
	CL_ALARM_MSO_TO_BE_POLLED,
	CL_ALARM_PARENT_ENTITY_FAILED,
	CL_ALARM_ALMS_AFTER_SOAKING_BITMAP,
	CL_ALARM_SIMPLE_ATTR_END ,

	CL_ALARM_ARRAY_ATTR_START,
	CL_ALARM_ID,
	CL_ALARM_ARRAY_ATTR_END,

	CL_ALARM_CONT_ATTR_START,
	CL_ALARM_PROBABLE_CAUSE,
	CL_ALARM_CATEGORY,
	CL_ALARM_SPECIFIC_PROBLEM,
	CL_ALARM_SEVERITY,
	CL_ALARM_AFFECTED_ALARMS,
	CL_ALARM_GENERATION_RULE,
	CL_ALARM_SUPPRESSION_RULE,
	CL_ALARM_GENERATE_RULE_RELATION,
	CL_ALARM_SUPPRESSION_RULE_RELATION,
	CL_ALARM_ACTIVE,
	CL_ALARM_SUSPEND,
	CL_ALARM_ENABLE,
	CL_ALARM_CLEAR,
	CL_ALARM_EVENT_TIME,
	CL_ALARM_CONT_ATTR_END,

	CL_ALARM_COR_ATTR_END
}ClAlarmMsoAttrIdT;

/**
 * This structure is used for polling. Alarm client does polling
 * by calling the \e fpAlarmObjectPoll virtual function of the application-specific
 * OM class. You need to override this function
 * by an application-specific function that should have the functionality
 * to query the alarms.
 * This structure is passed as one of the arguments in the virtual
 * function of the application-specific alarm OM class.
 */

typedef struct ClAlarmToPoll
{

    /**
     * Probable cause of the alarm. Alarm client sets this attribute.
     */
    ClAlarmProbableCauseT    probCause;

    /**
     * \e alarmActive indicates whether the alarm is asserted or not. This needs to be
     * set by the user code. If alarm status is asserted, set this to 1
     * and if the alarm status is cleared, set it to 0. Device Object needs to
     *  set it to 1 if the alarm exists.
     */
    ClAlarmStateT alarmState;

}ClAlarmPollInfoT;

/******************************************************************************
 *  Data Types 
 *****************************************************************************/


/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 * Function to be used by the fault server to get the resource MOId
 * while providing the alarm handle.
 */ 

ClRcT clAlarmClientResourceInfoGet (CL_IN ClAlarmHandleT alarmHandle, 
                           CL_OUT  ClAlarmInfoPtrT* const ppAlarmInfo) ;
#ifdef __cplusplus
}
#endif

#endif /* _CL_ALARM_IPI_H_ */
