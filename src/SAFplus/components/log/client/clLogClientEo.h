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
#ifndef _CL_LOG_CLIENT_EO_H_
#define _CL_LOG_CLIENT_EO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>
#include <clHandleApi.h>
#include <clLogOsal.h>
#include <clLogClientCommon.h>

#define CL_LOG_CLIENT_EO_ENTRY_KEY  CL_CID_LOG

typedef struct
{
    ClHandleDatabaseHandleT  hClntHandleDB;
    ClCntHandleT             hClntStreamTable;
    ClOsalMutexId_LT         clntStreamTblLock;
    ClCntHandleT             hStreamHandlerTable;
    ClOsalMutexId_LT         streamHandlerTblLock;
    ClUint32T                compId;
    ClUint32T                initCount;
    ClIdlHandleT             hClntIdl;
    ClUint32T                clientId;
    ClUint32T                maxStreams;
}ClLogClntEoDataT;

ClRcT
clLogClntEoEntryInstall(ClEoExecutionObjT  *pEoObj,
                        ClLogClntEoDataT   **ppClntEoEntry);

extern ClRcT
clLogClntEoEntryCreate(ClLogClntEoDataT  **ppClntEoEntry);

ClRcT
clLogClntEoEntryDelete(ClLogClntEoDataT **ppClntEoEntry);

ClRcT
clLogClntEoUninstall(ClLogClntEoDataT   **ppClntEoEntry);

ClRcT
clLogClntEoEntryGet(ClLogClntEoDataT  **ppClntEoEntry);

#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_CLIENT_EO_H_ */
