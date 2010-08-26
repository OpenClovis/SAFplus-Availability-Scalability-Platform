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
#include <clEoApi.h>
#include <clCpmExtApi.h>
#include <clLogFileOwner.h>
#include <clLogFileOwnerDeputy.h>
#include <clLogFileEvt.h>
#include <clLogMaster.h>
#include <clLogOsal.h>

ClRcT
clLogFileOwnerEoDataInit(ClLogFileOwnerEoDataT  **ppFileOwnerEoEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    *ppFileOwnerEoEntry = clHeapCalloc(1, sizeof(ClLogFileOwnerEoDataT));
    if( NULL == *ppFileOwnerEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clOsalMutexInit_L(&(*ppFileOwnerEoEntry)->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        clHeapFree(*ppFileOwnerEoEntry);
        return rc;
    }

    rc = clCpmLocalNodeNameGet(&((*ppFileOwnerEoEntry)->nodeName));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmLocalNodeNameGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppFileOwnerEoEntry)->fileTableLock),
                       CL_OK);
        clHeapFree(*ppFileOwnerEoEntry);
        return rc;
    }
    (*ppFileOwnerEoEntry)->hStreamDB  = CL_HANDLE_INVALID_VALUE;
    (*ppFileOwnerEoEntry)->hFileTable = CL_HANDLE_INVALID_VALUE;
    (*ppFileOwnerEoEntry)->hLog       = CL_HANDLE_INVALID_VALUE;
    (*ppFileOwnerEoEntry)->activeCnt  = 0;
    (*ppFileOwnerEoEntry)->terminate  = CL_FALSE;
    rc = clEoPrivateDataSet(pEoObj, CL_LOG_STREAM_HDLR_EO_ENTRY_KEY,
                            (void *) *ppFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppFileOwnerEoEntry)->fileTableLock),
                       CL_OK);
        clHeapFree(*ppFileOwnerEoEntry);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerEoDataFinalize(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry)
{
    ClRcT             rc            = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    clLogWarning("FOW", "DEL", "FileOwner eo is getting deleted");
    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != pFileOwnerEoEntry->hFileTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerEoEntry->hFileTable), CL_OK);
    }
    if( CL_HANDLE_INVALID_VALUE != pFileOwnerEoEntry->hStreamDB )
    {
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(pFileOwnerEoEntry->hStreamDB),
                       CL_OK);
    }
    pFileOwnerEoEntry->status = CL_LOG_FILEOWNER_STATE_CLOSED;
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
            CL_OK);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pFileOwnerEoEntry->fileTableLock),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerEoEntryGet(ClLogFileOwnerEoDataT  **ppFileOwnerEoEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_STREAM_HDLR_EO_ENTRY_KEY, 
                            (void **) ppFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

 /*
  * This function will be called after initialize with event.
  */
/*
 * Open the channel for subscribing for streamCreation event.
 * Create the lock varible for the fileTable.
 * Initialize the streamHandlerEoEntry.
 * Set the data.
 */
ClRcT
clLogFileOwnerBootup(ClBoolT  logRestart)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileOwnerStateRecover(pFileOwnerEoEntry, logRestart);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileOwnerEoDataFinalize(pFileOwnerEoEntry),
                CL_OK);
        return rc;
    }

    rc = clLogFileOwnerEventSubscribe();
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileOwnerEoDataFinalize(pFileOwnerEoEntry),
                CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerEoDataFree(void)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != pFileOwnerEoEntry->hLog )
    {
        CL_LOG_CLEANUP(clLogFinalize(pFileOwnerEoEntry->hLog), CL_OK);
        pFileOwnerEoEntry->hLog = CL_HANDLE_INVALID_VALUE;
    }
    clHeapFree(pFileOwnerEoEntry);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerShutdown(void)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };
    ClInt32T retries = 0;
    ClBoolT lockDestroyed = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    pFileOwnerEoEntry->terminate = CL_TRUE;
    while(pFileOwnerEoEntry->status == CL_LOG_FILEOWNER_STATE_ACTIVE
          &&
          pFileOwnerEoEntry->activeCnt > 0 
          &&
          retries++ < 6)
    {
        clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock);
        clOsalTaskDelay(delay);
        clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    }
    pFileOwnerEoEntry->activeCnt = 1;
    CL_LOG_CLEANUP(clLogFileOwnerFileTableDestroyWithLock(pFileOwnerEoEntry, 
                                                          CL_HANDLE_INVALID_VALUE,
                                                          &lockDestroyed),
                   CL_OK);
    pFileOwnerEoEntry->activeCnt = 0;
    if(!lockDestroyed)
        clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock);
 
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


ClRcT
clLogFileOwnerEoEntryUpdate(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry)
{
    ClRcT            rc           = CL_OK;
    ClLogCallbacksT  logCallbacks = {0};
    ClVersionT       version      = {'B', 0x01, 0x01};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clHandleDatabaseCreate(NULL, &pFileOwnerEoEntry->hStreamDB);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleDatabaseCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntHashtblCreate(CL_LOG_STREAM_HDLR_MAX_FILES,
                            clLogFileKeyCompare, clLogFileKeyHashFn, 
                            clLogFileTableDeleteCb, clLogFileTableDeleteCb,
                            CL_CNT_UNIQUE_KEY, 
                            &pFileOwnerEoEntry->hFileTable);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtbleCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(pFileOwnerEoEntry->hStreamDB),
                       CL_OK);
        return rc;
    }

    logCallbacks.clLogRecordDeliveryCb = clLogFileOwnerRecordDeliverCb;
    rc = clLogInitialize(&pFileOwnerEoEntry->hLog, &logCallbacks, &version);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogInitialize(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerEoEntry->hFileTable), CL_OK);
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(pFileOwnerEoEntry->hStreamDB),
                       CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
