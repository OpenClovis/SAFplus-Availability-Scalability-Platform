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
#include <string.h>
#include <clLogDebug.h>
#include <clLogCommon.h>
#include <clLogOsal.h>
#include <clLogClientEo.h>
#include <AppServer.h>
#include <AppClient.h>
#include <LogClient.h>

/*FIXME: This declaration has to go, only this fn is required from handle*/
extern void clLogHandleCleanupCb(void *pData);

ClRcT
clLogClntEoEntryInstall(ClEoExecutionObjT  *pEoObj,
                        ClLogClntEoDataT   **ppClntEoEntry)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryCreate(ppClntEoEntry);
    if( CL_OK != rc)
    {
        return rc;
    }

    rc = clAppClientInstall();
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clAppClientInstall(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogClntEoEntryDelete(ppClntEoEntry), CL_OK);
        return rc;
    }

    rc = clLogClientTableRegister(CL_IOC_LOG_PORT);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClientTableRegister: rc[0x%x]", rc));
        clAppClientUninstall();
        CL_LOG_CLEANUP(clLogClntEoEntryDelete(ppClntEoEntry), CL_OK);
        return rc;
    }

    rc = clAppClientTableRegister(pEoObj->eoPort);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clAppClientTableRegister: rc[0x%x]", rc));
        clLogClientTableDeregister();
        clAppClientUninstall();
        CL_LOG_CLEANUP(clLogClntEoEntryDelete(ppClntEoEntry), CL_OK);
        return rc;
    }

    rc = clEoPrivateDataSet(pEoObj, CL_LOG_CLIENT_EO_ENTRY_KEY,
                            *ppClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc));
        clAppClientTableDeregister();
        clLogClientTableDeregister();
        CL_LOG_CLEANUP(clAppClientUninstall(), CL_OK);
        CL_LOG_CLEANUP(clLogClntEoEntryDelete(ppClntEoEntry), CL_OK);
        return rc;
    }

#if 0
        CL_LOG_CLEANUP(
            clEoPrivateDataDelete(pEoObj, CL_LOG_CLIENT_EO_ENTRY_KEY), CL_OK);
#endif

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntEoEntryCreate(ClLogClntEoDataT  **ppClntEoEntry)
{
    ClRcT           rc         = CL_OK;
    ClLogCompInfoT  compInfo   = {{0}};
    ClIdlHandleT    hClntIdl   = CL_HANDLE_INVALID_VALUE;
    ClIocAddressT   localAddr  = {{0}};
    ClUint32T       clientId   = 0;
    ClUint32T       maxStreams = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
#ifdef NO_SAF

#else
    rc = clLogClntCompInfoGet(&compInfo);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntCompNameChkNCompIdGet(&compInfo.compName, &clientId);
    if( CL_OK != rc )
    {
        return rc;
    }
#endif
    localAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    localAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
    rc = clLogClntIdlHandleInitialize(localAddr, &hClntIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    *ppClntEoEntry = (ClLogClntEoDataT*) clHeapCalloc(1, sizeof(ClLogClntEoDataT));
    if( NULL == *ppClntEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&hClntIdl), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clOsalMutexErrorCheckInit((&(*ppClntEoEntry)->clntStreamTblLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexInitEx(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&hClntIdl), CL_OK);
        clHeapFree(*ppClntEoEntry);
        *ppClntEoEntry = NULL;
        return rc;
    }

    rc = clOsalMutexInit_L((&(*ppClntEoEntry)->streamHandlerTblLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexInitEx(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&((*ppClntEoEntry)->clntStreamTblLock)),
                                 CL_OK);
        CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&hClntIdl), CL_OK);
        clHeapFree(*ppClntEoEntry);
        *ppClntEoEntry = NULL;
        return rc;
    }

    rc = clHandleDatabaseCreate(clLogHandleCleanupCb,
                                &(*ppClntEoEntry)->hClntHandleDB);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&((*ppClntEoEntry)->streamHandlerTblLock)),
                                 CL_OK);
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&((*ppClntEoEntry)->clntStreamTblLock)),
                                 CL_OK);
        CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&hClntIdl), CL_OK);
        clHeapFree(*ppClntEoEntry);
        *ppClntEoEntry = NULL;
        return rc;
    }

    (*ppClntEoEntry)->hClntStreamTable = CL_HANDLE_INVALID_VALUE;
    (*ppClntEoEntry)->compId           = compInfo.compId;
    (*ppClntEoEntry)->hClntIdl         = hClntIdl;
    (*ppClntEoEntry)->initCount        = 0;
    (*ppClntEoEntry)->clientId         = clientId;

    rc = clLogConfigDataGet(&maxStreams, NULL, NULL, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&((*ppClntEoEntry)->streamHandlerTblLock)),
                                 CL_OK);
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&((*ppClntEoEntry)->clntStreamTblLock)),
                                 CL_OK);
        CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&hClntIdl), CL_OK);
        clHeapFree(*ppClntEoEntry);
        *ppClntEoEntry = NULL;
        return rc;
    }
    
    (*ppClntEoEntry)->maxStreams = maxStreams;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntEoEntryDelete(ClLogClntEoDataT **ppClntEoEntry)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&(*ppClntEoEntry)->clntStreamTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != (*ppClntEoEntry)->hClntStreamTable )
    {
        CL_LOG_CLEANUP(clCntDelete((*ppClntEoEntry)->hClntStreamTable), CL_OK);
        (*ppClntEoEntry)->hClntStreamTable = CL_HANDLE_INVALID_VALUE;
    }

    if( CL_HANDLE_INVALID_VALUE != (*ppClntEoEntry)->hClntHandleDB )
    {
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(
                           (*ppClntEoEntry)->hClntHandleDB), CL_OK);
        (*ppClntEoEntry)->hClntHandleDB = CL_HANDLE_INVALID_VALUE;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(*ppClntEoEntry)->clntStreamTblLock),
            CL_OK);

    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppClntEoEntry)->clntStreamTblLock),
                   CL_OK);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppClntEoEntry)->streamHandlerTblLock),
                   CL_OK);
    CL_LOG_CLEANUP(clLogClntIdlHandleFinalize(&((*ppClntEoEntry)->hClntIdl)),
                   CL_OK);
    memset(*ppClntEoEntry, '\0', sizeof(ClLogClntEoDataT));
    clHeapFree(*ppClntEoEntry);
    *ppClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntEoUninstall(ClLogClntEoDataT   **ppClntEoEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    clAppClientTableDeregister();
    clLogClientTableDeregister();
    CL_LOG_CLEANUP(clAppClientUninstall(), CL_OK);
#if 0
    CL_LOG_CLEANUP(
        clEoPrivateDataDelete(pEoObj, CL_LOG_CLIENT_EO_ENTRY_KEY), CL_OK);
#endif
    CL_LOG_CLEANUP(clLogClntEoEntryDelete(ppClntEoEntry), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntEoEntryGet(ClLogClntEoDataT  **ppClntEoEntry)
{
    ClRcT               rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_CLIENT_EO_ENTRY_KEY,
                            (void **) ppClntEoEntry);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
