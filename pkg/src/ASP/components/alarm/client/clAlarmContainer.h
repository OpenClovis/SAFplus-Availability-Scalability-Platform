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
 * File        : clAlarmContainer.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * 
 * This module contains alarm contains alarm container related data structures
 * and api's.
 *
 *****************************************************************************/

#ifndef _CL_ALARM_CONTAINER_H_
#define _CL_ALARM_CONTAINER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clAlarmDefinitions.h>

/****************************************************************************
 * Constants 
 ***************************************************************************/ 
#define CL_ALARM_MAX_RES_ATTR_SUBS_ID 2
#define CL_ALARM_CONFIG_RETRIES       5

/*
 * This container will hold the configuration related information per alarm
 */
typedef struct clAlarmContInfo
{
	ClUint64T generationRule;
	ClAlarmRuleRelationT generationRuleRelation;
	ClUint64T suppressionRule;
	ClAlarmRuleRelationT suppressionRuleRelation;
	ClAlarmCategoryTypeT alarmCategory;
	ClAlarmProbableCauseT probCause;
    ClAlarmSpecificProblemT specProb;
	ClUint64T affectedAlarms;
}ClAlarmContInfoT;

/*
 * This container will hold the configuration related information for a unique
 * combination of classid and service id.
 */
typedef struct clAlarmResCntData
{
	ClHandleT omHandle;
	ClCorObjectHandleT hMSOObj;
	ClUint32T pollingInterval;
	ClUint8T msoToBePolled;
	ClUint32T numofValidAlarms;
	ClAlarmContInfoT *alarmContInfo;
} ClAlarmResCntDataT;


/*****************************************************************************
 *  Functions
 *****************************************************************************/


/*
 * This function will fill in the ClAlarmResCntDataT after retrieving the 
 * information from COR. Going ahead this api will use the bulk read functionality
 * to get the information from COR
 */
ClRcT clAlarmStaticInfoCntAdd(ClCorMOIdPtrT pMoId, ClAlarmResCntDataT *resInfo);

/*
 * This function will fetch the specific data from the container.
 */
ClRcT clAlarmStaticInfoCntDataGet(ClCorMOIdPtrT pMoId, 
                                    ClUint32T attrId,
                                    ClUint32T idx, 
                                    void* value,
									ClUint32T lockApplied);

/*
 * This function will fetch the object handle correspoding to the given MOID.
 */
ClRcT clAlarmStaticInfoCntMoIdToHandleGet(ClCorMOIdPtrT pMoId,ClCorObjectHandleT* hMSOObj);

/*
 * This function will find if the specified container exists.
 */
ClRcT clAlarmStaticInfoCntFind(ClCorMOIdPtrT pMoId, ClUint32T lockApplied);

/*
 * This function will get the number of configured alarms.
 */
ClRcT clAlarmGetNumOfConfiguredAlarms(ClCorMOIdPtrT pMoId, ClUint32T *ptrIndex,ClCorObjectHandleT hMSOObj);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_ALARM_CONTAINER_H_ */
