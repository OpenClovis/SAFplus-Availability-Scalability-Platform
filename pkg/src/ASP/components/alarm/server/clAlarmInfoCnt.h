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
 * File        : clAlarmInfoCnt.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains alarm info container related APIs
 *****************************************************************************/

#ifndef _CL_ALARM_INFO_CNT_H_
#define _CL_ALARM_INFO_CNT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clAlarmDefinitions.h>
#include <clAlarmErrors.h>


extern ClRcT
clAlarmAlarmInfoCntCreate(void);

extern ClRcT
clAlarmAlarmInfoCntDestroy(void);

extern ClRcT
clAlarmAlarmInfoAdd(ClAlarmInfoT *pAlarmInfo,ClAlarmHandleT *pHandle);

extern ClRcT
clAlarmAlarmInfoDelete(ClAlarmHandleT handle);

extern ClRcT
clAlarmAlarmInfoGet(ClAlarmHandleT handle,ClAlarmInfoT **pAlarmInfo);

extern ClRcT
clAlarmNumAlarmInfoEntriesGet(ClUint32T *pNumEntries);

extern ClRcT
clAlarmDatabaseHandlesGet(void *buf,ClUint32T length);

extern ClRcT
clAlarmDatabaseToBufferCopy(
    ClCntKeyHandleT     key,
    ClCntDataHandleT    pData,
    void                *dummy,
    ClUint32T           dataLength);

extern ClRcT
clAlarmAlarmInfoToHandleGet(
    ClAlarmInfoT *pAlarmInfo,
    ClAlarmHandleT *pAlarmHandle);

#ifdef __cplusplus
}
#endif


                                                                                
#endif /* CL_ALARM_INFO_CNT_H */
