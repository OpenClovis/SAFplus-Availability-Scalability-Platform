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
#ifndef _CL_LOG_SVR_EO_H_
#define _CL_LOG_SVR_EO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmapApi.h>
#include <clLogSvrCommon.h>

#define CL_LOG_SERVER_EO_ENTRY_KEY 2

typedef struct
{
    ClHandleT                hCpm;
    ClUint32T                logCompId;
    ClCntHandleT             hSvrStreamTable;
    ClOsalMutexId_LT         svrStreamTableLock;
    ClUint32T                nextDsId;
    ClBitmapHandleT          hDsIdMap;
    ClUint32T                maxFlushLimit;
    ClTimerHandleT           hTimer;
    ClHandleDatabaseHandleT  hFlusherDB;
    ClBoolT                  gmsInit;
    ClBoolT                  evtInit;
    ClBoolT                  ckptInit;
    ClBoolT                  ckptOpen;
    ClBoolT                  logInit;
}ClLogSvrEoDataT;

extern ClRcT
clLogSvrEoDataInit(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                   CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry);
extern ClRcT
clLogSvrEoDataFinalize(void);

extern ClRcT
clLogSvrEoEntryGet(CL_OUT  ClLogSvrEoDataT        **ppSvrEoData,
                   CL_OUT  ClLogSvrCommonEoDataT  **ppSvrCommonEoEntry);

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_SVR_EO_H_*/
