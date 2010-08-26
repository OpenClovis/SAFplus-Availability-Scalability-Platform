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
 * File        : clAlarmServerEngine.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#ifndef _CL_ALARM_SERVER_ENGINE_H_
#define _CL_ALARM_SERVER_ENGINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clAlarmDefinitions.h>

/****************************************************************************
 * Constants and macro definitions
 ***************************************************************************/

/***************************************************************************
 * forward declaration of functions belonging to the server engine.
 **************************************************************************/

/* Added new parameters to fill the outMsgHandle with alarm handle and
 *  to pass alarmInfo to the container */
ClRcT 
clAlarmServerEngineAlarmSuppressionCheck (ClCorObjectHandleT hMSOObj, 
                                          ClUint32T idx, 
                                          ClUint8T almRuleMatches,
                                          ClAlarmInfoT *pAlarmInfo,
                                          ClAlarmProcessT* pAlarmValues,
                                          ClBufferHandleT outMsgHandle);

ClRcT 
clAlarmServerEngineAlmToBeSuppressed      (ClCorObjectHandleT hCorHdl, 
                                           ClUint32T idx,
                                           ClAlarmProcessT* pAlarmValues);

ClRcT 
asEnginePefSet                            (void* pData, void *cookie);


ClRcT 
asEnginePEFAttrProcess                    (ClCorObjectHandleT hMSOObj, 
                                           ClUint32T idx, 
                                           ClUint8T isAssert,
										   ClUint64T suppressedAlmBM, 
                                           ClAlarmInfoT* pAlarmInfo);

/**************************************************************************/
typedef struct
{
    ClAlarmProbableCauseT    probCauseId;
    ClUint8T     isPEFpresent; 
    ClCorMOIdT   moID;
	ClUint8T alarmState;
}PefInfoT; /* parent Entity field info structure */

#endif /* _CL_ALARM_SERVER_H_ */
