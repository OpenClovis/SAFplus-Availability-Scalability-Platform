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
 * File        : clAlarmServerAlarmUtil.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains alarm Service related APIs
 *
 *
 *****************************************************************************/

#ifndef _CL_ALARM_UTIL_H_
#define _CL_ALARM_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCorApi.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clDebugApi.h>
#include <clAlarmDefinitions.h>
#include <clAlarmErrors.h>

typedef struct ClAlarmProcess
{
    /* Alarm MSO Bitmaps */
    ClUint64T suppressedAlarms;
    ClUint64T publishedAlarms;
    ClUint8T  parentEntityFailed;
    ClUint64T afterSoakingBM;

    /* Alarm Info */
    ClUint32T probableCause;
    ClUint32T specificProblem;
    ClUint64T supRule;
    ClUint32T supRuleRel;
    ClUint8T  alarmSuspend;
    ClUint8T  alarmEnable;
    ClUint8T  category;
    ClUint8T  severity;
    ClUint8T  contAttrValidEntry;
} ClAlarmProcessT;

typedef struct ClAlarmAttrInfo
{
    ClUint32T attrId;
    ClUint32T index;
    void* pValue;
    ClUint32T size;
} ClAlarmAttrInfoT;

typedef struct ClAlarmAttrList
{
    ClUint32T numOfAttr;
    ClAlarmAttrInfoT attrInfo[1];
}ClAlarmAttrListT;

typedef ClAlarmAttrListT* ClAlarmAttrListPtrT;

extern ClRcT
clAlarmAttrValueGet(ClCorObjectHandleT hMSOObj, ClAlarmAttrListT* pAlarmAttrList); 

extern ClRcT 
clAlarmAttrValueSet(ClCorObjectHandleT hMSOObj, ClAlarmAttrListT* pAttrList); 

extern ClRcT
clAlarmUtilAlmIdxGet(ClCorObjectHandleT objH,  
                   ClAlarmRuleEntryT alarmKey,
                   ClUint32T* index);

ClRcT _clAlarmResetChildWalk(void* pData, void* pCookie);

#ifdef __cplusplus
}
#endif

                                                                                
#endif /* CL_ALARM_UTIL_H */
