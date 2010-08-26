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
 * File        : clAlarmPayloadCont.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * 
 * This module contains alarm Service related APIs
 *
 *****************************************************************************/

#ifndef _CL_ALARM_PAYLOAD_CNT_H_
#define _CL_ALARM_PAYLOAD_CNT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clAlarmDefinitions.h>

/****************************************************************************
 * Constants 
 ***************************************************************************/ 
/*
 * This structure will form the data part of the payload container. This
 * container is required to store the payload information of the alarm  
 * which is being raised. This information will have to be stored if the
 * alarm is being suppressed because of heirarchical masking because once
 * it gets raised after the parent alarm is cleared all the rest of the 
 * alarm information is retrieved from COR except for the alarm payload.
 */
typedef struct clAlarmPayload
{
	ClUint32T len;
    ClUint8T buff[1];
}ClAlarmPayloadCntT;

/*
 * This key uniquely identifies the payload information. The key
 * is a combination of moid, probable cause and specific problem.
 */
typedef struct clAlarmPayloadKey
{
    ClAlarmProbableCauseT probCause;
    ClAlarmSpecificProblemT specificProblem;
    ClCorMOIdT moId;
}ClAlarmPayloadCntKeyT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/


/*
 * The payload container creation routine.
 */
ClRcT clAlarmPayloadCntListCreate();

/*
 * The payload container compare routine.
 */
ClInt32T clAlarmPayloadCntCompare(
    ClCntKeyHandleT key1, 
    ClCntKeyHandleT key2);

/*
 * The payload container delete routine.
 */
void clAlarmPayloadCntEntryDeleteCallback(
    ClCntKeyHandleT key, 
    ClCntDataHandleT userData);

/*
 * The payload container destroy routine.
 */
void clAlarmPayloadCntEntryDestroyCallback(
    ClCntKeyHandleT key, 
    ClCntDataHandleT userData);

/*
 * The payload container add routine.
 */
ClRcT clAlarmPayloadCntAdd(ClAlarmInfoT *pAlarmInfo);

/*
 * The payload container key specific delete routine.
 */
ClRcT clAlarmPayloadCntDelete(ClCorMOIdPtrT pMoId, ClAlarmProbableCauseT probCause, ClAlarmSpecificProblemT specificProblem);

/*
 * The payload container delete routine.
 */
ClRcT clAlarmPayloadCntDeleteAll(void);

/*
 * The payload container DataGet routine.
 */
ClRcT clAlarmPayloadCntDataGet(ClAlarmProbableCauseT probCause, 
                                    ClAlarmSpecificProblemT specificProblem,
                                    ClCorMOIdPtrT pMoId, 
                                    ClAlarmPayloadCntT **payloadInfo);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_ALARM_PAYLOAD_CNT_H_ */
