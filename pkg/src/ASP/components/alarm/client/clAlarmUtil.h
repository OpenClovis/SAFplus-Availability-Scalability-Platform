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
 * File        : clAlarmUtil.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains alarm Service related APIs
 *****************************************************************************/

#ifndef _CL_ALARM_UTIL_H_
#define _CL_ALARM_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorApi.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clDebugApi.h>
#include <clEventApi.h>
#include <clAlarmDefinitions.h>


extern ClRcT 
GetAlarmUniqueId(ClUint32T *alarmId);
    
extern ClRcT
GetAlmAttrValue(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, ClUint32T attrId,
                ClUint32T idx, void* value, ClUint32T* size, ClUint32T lockApplied);

extern ClRcT 
SetAlmAttrValue(ClCorMOIdPtrT pMoId,ClCorTxnSessionIdT *ptxnSessionId,ClCorObjectHandleT hMSOObj, ClUint32T attrId,
                ClUint32T idx, void* value, ClUint32T size);


extern ClRcT
alarmUtilAlmIdxGet(ClCorMOIdPtrT pMoId,
                   ClCorObjectHandleT objH,  
                   ClAlarmRuleEntryT alarmKey,
                   ClUint32T* index,
				   ClUint32T lockApplied );

extern ClAlarmComponentResAlarmsT appAlarms[];

extern ClRcT clAlarmSoakTimeStatusGet(ClUint32T *pValue);

extern ClRcT clAlarmPollingStatusGet(ClUint32T *pValue);

extern ClRcT 
clAlarmCheckIfPrimaryOI(ClCorMOIdPtrT pMoId, ClUint32T* primaryOI);

/* Function to unregister the route list of COR. */
extern ClRcT _clAlarmOIUnregister();

/* Funtion to count the no. of resesources to be managed. */
ClRcT _clAlarmGetAlarmsCountOnResource (ClCorMOIdPtrT pMoId, ClUint32T *pExpectedIndex);

#ifdef __cplusplus
}
#endif 
                                                                                
#endif /* _CL_ALARM_UTIL_H_ */


