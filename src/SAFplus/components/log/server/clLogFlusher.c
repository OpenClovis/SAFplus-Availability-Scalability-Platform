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
#include <clLogFlusher.h>
#include <clLogServer.h>
#include <clLogOsal.h>
#include <AppclientPortclientClient.h>
#include <clHandleIpi.h>
#include <clLogFileOwner.h>
#include <clLogCommon.h>

#ifndef POSIX_BUILD

typedef struct ClLogFlushBuffer
{
    ClUint8T *pRecord;
    ClUint32T numRecords;
}ClLogFlushBufferT;

typedef struct ClLogFlushRecord
{
    ClLogFlushBufferT *pBuffs;
    ClUint32T numBufs;
    ClUint32T recordSize;
    ClStringT fileName;
    ClStringT fileLocation;
    ClInt32T  multicast;
    ClIocAddressT mcastAddr;
    ClUint32T ackersCount;
    ClUint32T fileOwnerAddr;
    ClUint32T seqNum;
}ClLogFlushRecordT;

static ClRcT
clLogFlusherRecordsGetMcast(ClLogSvrStreamDataT  *pStreamData,
                            ClUint32T            nRecords,
                            ClLogFlushRecordT    *pFlushRecord);

#else

static ClRcT
clLogFlusherRecordsMcast(ClLogSvrStreamDataT  *pStreamData,
                         ClUint32T            nRecords);
#endif

static ClRcT
clLogFlusherRecordsFlush(ClLogSvrStreamDataT  *pStreamData);

static ClRcT
clLogFlusherCookieHandleCreate(ClUint32T       numRecords,
                               ClTimerHandleT  *phTimer, 
                               ClHandleT       *phFlusher);

ClRcT
clLogFlusherCondSignal(ClLogStreamHeaderT  *pHeader);


void *
clLogFlushIntervalThread(void *pData)
{
    ClRcT                rc           = CL_OK;
    ClLogSvrStreamDataT  *pStreamData = (ClLogSvrStreamDataT*) pData;
    ClTimerTimeOutT      timeout      = {0, 0};

    if( !pStreamData ) return NULL; 

    /* calculate the time out */
    clLogDebug(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP,
            "Flusher interval thread started for stream [%.*s]", pStreamData->shmName.length, pStreamData->shmName.pValue);
    timeout.tsSec = pStreamData->pStreamHeader->flushInterval / (1000L * 1000L * 1000L);
    timeout.tsMilliSec
        = (pStreamData->pStreamHeader->flushInterval % (1000L * 1000L * 1000L)) / (1000L * 1000L);
    
    /*
     * Making sure that we dont sleep forever to work on the flush
     * frequency which would making logging slow if log writes are slow.
    */
    if(!timeout.tsSec && !timeout.tsMilliSec)
    {
        timeout.tsMilliSec = 10;
    }

    clLogDebug(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, "timeout: %u sec %u millisec", timeout.tsSec, timeout.tsMilliSec);
    rc  = clOsalMutexLock_L(&pStreamData->flushIntervalMutex);
    if( CL_OK != rc )
    {
        pStreamData->flushIntervalThreadStatus =
        CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE;
        return NULL;
    }
    while( pStreamData->flushIntervalThreadStatus ==
            CL_LOG_FLUSH_INTERVAL_STATUS_ACTIVE )
    {
        rc = clOsalCondWait_L(&pStreamData->flushIntervalCondvar,
                &pStreamData->flushIntervalMutex, timeout);
        if( (CL_OK != rc) && (CL_GET_ERROR_CODE(rc) != CL_ERR_TIMEOUT) )
        {
            rc = CL_OK;
            goto exitThread;
        }
        /* timeout happened, do this job again go & sleep */
        if( (CL_LOG_STREAM_ACTIVE == pStreamData->pStreamHeader->streamStatus) && 
            (0 < abs(pStreamData->pStreamHeader->recordIdx -
                     pStreamData->pStreamHeader->startAck)) )
        {
            extern ClOsalSharedMutexFlagsT gClLogMutexMode;
            /*
             * Not checking error, incase of error, 1 will be set 
             */
            if(gClLogMutexMode == CL_OSAL_SHARED_SYSV_SEM)
            {
#ifndef VXWORKS_BUILD
                ClInt32T  semCnt = 0;
                clOsalMutexValueGet_L(&pStreamData->pStreamHeader->flusherSem, &semCnt);
                if(semCnt < 10)
                    clOsalMutexValueSet_L(&pStreamData->pStreamHeader->flusherSem, ++semCnt);
#endif
            }
            else
            {
#ifndef POSIX_BUILD
                clLogServerStreamSignalFlusher(pStreamData, &pStreamData->pStreamHeader->flushCond);
#else
                clLogServerStreamSignalFlusher(pStreamData, NULL);
#endif
            }
        }
    }
exitThread:    
    pStreamData->flushIntervalThreadStatus =
        CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE;
    clOsalMutexUnlock_L(&pStreamData->flushIntervalMutex);
    return NULL;
}

ClRcT
clLogFlushIntervalThreadJoin(ClLogSvrStreamDataT  *pStreamData, 
                             ClOsalTaskIdT        taskId)
{
    ClRcT  rc = CL_OK;

    rc = clOsalMutexLock_L(&pStreamData->flushIntervalMutex);
    if( CL_OK != rc )
    {
        pStreamData->flushIntervalThreadStatus =
            CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE;
        clLogError("SVR", "FLU", "Failed acquire the lock rc [0x %x]", rc);
        goto exitOnError;
    }
    if( pStreamData->flushIntervalThreadStatus !=
            CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE )
    {
        /* still that thread is there */
        pStreamData->flushIntervalThreadStatus =
            CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE;
        /* Unlock & signal the thread */
        clOsalCondSignal_L(&pStreamData->flushIntervalCondvar);
        clOsalMutexUnlock_L(&pStreamData->flushIntervalMutex);
        /* Join the thread*/
        clLogDebug("SVR", "FLU", "Joining thread [%llu]", taskId);
        rc = clOsalTaskJoin(taskId);
        if( CL_OK != rc )
        {
            clLogError("SVR", "FLU", "Joining flush interval thread failed rc[0x %x]" ,rc);
        }
    }
    else
    {
        /* Unlock & signal the thread */
        clOsalMutexUnlock_L(&pStreamData->flushIntervalMutex);
    }
    /* so flushThread is done with its work, delete the mutex & taskId */
exitOnError:
    clOsalCondDestroy_L(&pStreamData->flushIntervalCondvar);
    clOsalMutexDestroy_L(&pStreamData->flushIntervalMutex);
    return rc; 
}
        
ClRcT
clLogFlushIntervalThreadCreate(ClLogSvrStreamDataT  *pSvrStreamData, 
                               ClOsalTaskIdT        *pTaskId)
{
    ClRcT  rc = CL_OK;

    /* create a mutex for this thread, no need to destroy this mutex as
     * memory is not allocated for this mutex */
    rc = clOsalMutexInit_L(&pSvrStreamData->flushIntervalMutex);
    if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Mutex creation failed rc [0x %x]", rc);
        return rc;
    }
    rc = clOsalCondInit_L(&pSvrStreamData->flushIntervalCondvar);
    if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Mutex creation failed rc [0x %x]", rc);
        clOsalMutexDestroy_L(&pSvrStreamData->flushIntervalMutex);
        return rc;
    }
    pSvrStreamData->flushIntervalThreadStatus =
        CL_LOG_FLUSH_INTERVAL_STATUS_ACTIVE;
    rc = clOsalTaskCreateAttached(pSvrStreamData->shmName.pValue,
            CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
            CL_OSAL_MIN_STACK_SIZE, clLogFlushIntervalThread, 
            pSvrStreamData, pTaskId);
    if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Creating thread for flush interval fails rc[0x %x]",
                rc);
        clOsalCondDestroy(&pSvrStreamData->flushIntervalCondvar);
        clOsalMutexDestroy_L(&pSvrStreamData->flushIntervalMutex);
        return rc;
    }
    return rc;
}

/*
 * This is the main flusher therad function. This is where the flusher action
 * starts.
 */
void*
clLogFlusherStart(void  *pData)
{
    ClRcT              rc             = CL_OK;
    ClLogSvrStreamDataT  *pStreamData = (ClLogSvrStreamDataT*) pData;
    ClLogStreamHeaderT   *pHeader     = pStreamData->pStreamHeader;
    ClTimerTimeOutT      timeout      = {0, 0};
    ClOsalTaskIdT        taskId       = CL_HANDLE_INVALID_VALUE;

    clLogDebug(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP,
            "Flusher started for stream [%.*s]", pStreamData->shmName.length, pStreamData->shmName.pValue);
    timeout.tsSec = pHeader->flushInterval / (1000L * 1000L * 1000L);
    timeout.tsMilliSec
        = (pHeader->flushInterval % (1000L * 1000L * 1000L)) / (1000L * 1000L);

    clLogDebug("LOG", "FLS", "timeout: %u sec %u millisec", timeout.tsSec, timeout.tsMilliSec);

    rc = clLogServerStreamMutexLockFlusher(pStreamData);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "CL_LOG_LOCK(): rc[0x %x]", rc);
        pHeader->streamStatus = CL_LOG_STREAM_THREAD_EXIT;
        return NULL;
    }
    if ((CL_LOG_STREAM_HEADER_STRUCT_ID != pHeader->struct_id) || (CL_LOG_STREAM_HEADER_UPDATE_COMPLETE != pHeader->update_status))
    {/* Stream Header is corrupted so reset Header parameters */
       clLogStreamHeaderReset(pHeader); 
    }
    /*
     * Create a thread for flush interval
     */
    if( CL_LOG_SEM_MODE == clLogLockModeGet() )
    {
        rc = clLogFlushIntervalThreadCreate(pStreamData, &taskId);
        if( CL_OK != rc )
        {
            clLogWarning("SVR", "FLU", "Flusher interval thread create failed"
                    "flushFrequency function will still work..continuing...");
            rc = CL_OK;
            taskId = CL_HANDLE_INVALID_VALUE;
        }
    }

    while( gClLogSvrExiting == CL_FALSE )
    {
        CL_LOG_DEBUG_TRACE(("startIdx in server: %d  startAck  %d \n",
                    pHeader->recordIdx, pHeader->startAck));
        do
        {
            CL_LOG_DEBUG_TRACE(("recordIdx: %u startAck: %u  cnt %d",
                        pHeader->recordIdx, pHeader->startAck, 
                        pStreamData->ackersCount +
                        pStreamData->nonAckersCount));
#ifndef POSIX_BUILD
            rc = clLogServerStreamCondWait(pStreamData,
                                           &pHeader->flushCond,
                                           timeout);
#else
            rc = clLogServerStreamCondWait(pStreamData,
                                           NULL,
                                           timeout);
#endif
            if( gClLogSvrExiting == CL_TRUE 
                    || ( (CL_OK != CL_GET_ERROR_CODE(rc)) && 
                        (CL_ERR_TIMEOUT != CL_GET_ERROR_CODE(rc))) )
            { /* Log service is exiting or Stream is closed so come out of polling loop */
                pHeader->update_status = CL_LOG_STREAM_HEADER_UPDATE_INPROGRESS;
                pHeader->streamStatus = CL_LOG_STREAM_CLOSE;
            }
        }while( (CL_LOG_STREAM_CLOSE != pHeader->streamStatus) &&
                (0 == abs(pHeader->recordIdx - pHeader->startAck)));

        if( gClLogSvrExiting || CL_LOG_STREAM_CLOSE == pHeader->streamStatus )
        {
            clLogInfo("SVR", "FLU", "Stream status: CLOSE...Exiting flusher [%lu] "
                    " [%lu]", (unsigned long)pStreamData->flusherId, (long unsigned int)pthread_self());
            pHeader->streamStatus = CL_LOG_STREAM_THREAD_EXIT;
            pHeader->update_status = CL_LOG_STREAM_HEADER_UPDATE_COMPLETE;
            goto exitFlusher;
        }
        rc = clLogFlusherRecordsFlush(pStreamData);
        if( CL_OK != rc )
        {
            if(pHeader->streamStatus == CL_LOG_STREAM_CLOSE)
            {
                break;
            }
        }    

    }
    pHeader->streamStatus = CL_LOG_STREAM_THREAD_EXIT;
exitFlusher:

    if( CL_HANDLE_INVALID_VALUE != taskId )
    {
        /* Flusher interval thread has been created, so signal to that guy and
         * wait on the variable 
         */
        clLogFlushIntervalThreadJoin(pStreamData, taskId);
        taskId = CL_HANDLE_INVALID_VALUE;
    }
    clLogInfo("SVR", "FLU", "Flusher for stream [%.*s] is exiting..Id [%llu]", 
            pStreamData->shmName.length, pStreamData->shmName.pValue,
            pStreamData->flusherId);
    rc = clLogServerStreamMutexUnlock(pStreamData);
    if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Faild to unlock the stream");
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return NULL;
}    

ClRcT
clLogFlusherCondSignal(ClLogStreamHeaderT  *pHeader)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pHeader->lock_for_join);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clOsalMutexLock_L(): rc[0x %x]", rc);
        return rc;
    }    

    pHeader->streamStatus = CL_LOG_STREAM_THREAD_EXIT;
    rc = clOsalCondSignal_L(&pHeader->cond_for_join);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clOsalMutexLock_L(): rc[0x %x]", rc);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pHeader->lock_for_join), CL_OK);
        return rc;
    }    

    rc = clOsalMutexUnlock_L(&pHeader->lock_for_join);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clOsalMutexUnlock_L(): rc[0x %x]", rc);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Called with the log server flusher stream lock. held
 */

#ifndef POSIX_BUILD

static void logRecordsFlush(ClLogFlushRecordT *pFlushRecord)
{
    ClRcT rc = CL_OK;
    ClIdlHandleT idlHdl = CL_HANDLE_INVALID_VALUE;
    ClIocNodeAddressT localAddr = clIocLocalAddressGet();
    ClUint32T seqNum = 0;
    ClUint32T i = 0;

    seqNum = pFlushRecord->seqNum;
    if(pFlushRecord->multicast)
    {
        rc = clLogIdlHandleInitialize(pFlushRecord->mcastAddr, &idlHdl);
        if( CL_OK != rc )
        {
            goto out_free;
        }
    }

    for(i = 0; i < pFlushRecord->numBufs; ++i)
    {
        ClUint32T numRecords = pFlushRecord->pBuffs[i].numRecords;
        ClUint8T* pRecord = pFlushRecord->pBuffs[i].pRecord;
        ClUint32T buffLen = pFlushRecord->recordSize * numRecords;
        ClTimerHandleT hTimer = CL_HANDLE_INVALID_VALUE;
        ClHandleT hFlusher = CL_HANDLE_INVALID_VALUE;
        if(pFlushRecord->multicast > 0)
        {
            if(pFlushRecord->ackersCount > 0)
            {
                rc = clLogFlusherCookieHandleCreate(numRecords, &hTimer, &hFlusher);
                if(rc != CL_OK)
                    goto out_free;
            }
            rc = VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)(idlHdl,
                                                                             pFlushRecord->mcastAddr.iocMulticastAddress,
                                                                             seqNum,
                                                                             localAddr,
                                                                             hFlusher, numRecords,
                                                                             buffLen, pRecord,
                                                                             NULL, 0);
            if(rc != CL_OK)
            {
                if(hFlusher)
                {
                    CL_LOG_CLEANUP(clLogFlusherCookieHandleDestroy(hFlusher, CL_FALSE), CL_OK);
                }
            }
        }
        else
        {
            if(pFlushRecord->fileOwnerAddr == localAddr)
            {
                rc = clLogFileOwnerEntryFindNPersist(&pFlushRecord->fileName,
                                                     &pFlushRecord->fileLocation,
                                                     numRecords, pRecord);
                if(rc != CL_OK)
                {
                    /* 
                     * do nothing
                     */
                }
            }
        }
        seqNum += numRecords;
        clHeapFree(pRecord);
    }

    out_free:
    {
        ClUint32T j;
        for(j = i; j < pFlushRecord->numBufs; ++j)
        {
            if(pFlushRecord->pBuffs[j].pRecord)
                clHeapFree(pFlushRecord->pBuffs[j].pRecord);
        }
    }
    if(idlHdl)
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(idlHdl), CL_OK);
    }
    if(pFlushRecord->fileLocation.pValue)
    {
        clHeapFree(pFlushRecord->fileLocation.pValue);
        pFlushRecord->fileLocation.pValue = NULL;
        pFlushRecord->fileLocation.length = 0;
    }
    if(pFlushRecord->fileName.pValue)
    {
        clHeapFree(pFlushRecord->fileName.pValue);
        pFlushRecord->fileName.pValue = NULL;
        pFlushRecord->fileName.length = 0;
    }
    if(pFlushRecord->pBuffs)
    {
        clHeapFree(pFlushRecord->pBuffs);
        pFlushRecord->pBuffs = NULL;
    }
}

static ClRcT
clLogFlusherRecordsFlush(ClLogSvrStreamDataT  *pStreamData)
{    
    ClRcT     rc        = CL_OK;
    ClLogStreamHeaderT     *pHeader           = pStreamData->pStreamHeader;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClUint32T              nFlushableRecords  = 0;
    ClUint32T              nFlushedRecords    = 0;
    ClLogFlushRecordT      flushRecord = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (pStreamData->fileOwnerAddr != clIocLocalAddressGet()) && 
        (0 == pStreamData->ackersCount + pStreamData->nonAckersCount) )
    {
        clLogInfo("LOG", "FLS", "No one has registered for the Stream");
        return CL_OK;
    }    

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }            



    CL_LOG_DEBUG_TRACE((" recordIdx: %d startAck : %d \n", pHeader->recordIdx,
                pHeader->startAck));

    if (pHeader->recordIdx < pHeader->startAck)
    {  /* Ring buffer wraparound Case */
        nFlushableRecords = (pHeader->recordIdx + pHeader->maxRecordCount) - pHeader->startAck;
    }
    else
    {
        nFlushableRecords = pHeader->recordIdx - pHeader->startAck;
    }    

    flushRecord.fileName.pValue = (ClCharT*) clHeapCalloc(1, pStreamData->fileName.length+1);
    CL_ASSERT(flushRecord.fileName.pValue != NULL);
    flushRecord.fileName.length = pStreamData->fileName.length;
    memcpy(flushRecord.fileName.pValue, pStreamData->fileName.pValue, flushRecord.fileName.length);
    
    flushRecord.fileLocation.pValue = (ClCharT*) clHeapCalloc(1, pStreamData->fileLocation.length+1);
    CL_ASSERT(flushRecord.fileLocation.pValue != NULL);
    flushRecord.fileLocation.length = pStreamData->fileLocation.length;
    memcpy(flushRecord.fileLocation.pValue, pStreamData->fileLocation.pValue,
           flushRecord.fileLocation.length);

    flushRecord.fileOwnerAddr = pStreamData->fileOwnerAddr;
    flushRecord.seqNum = pStreamData->seqNum;
    flushRecord.multicast = -1;
    flushRecord.recordSize = pHeader->recordSize;

    while( nFlushableRecords > 0 )
    {
        nFlushedRecords = (nFlushableRecords > pSvrEoEntry->maxFlushLimit)
            ? pSvrEoEntry->maxFlushLimit : nFlushableRecords;

        rc = clLogFlusherRecordsGetMcast(pStreamData, nFlushedRecords, &flushRecord);
        if( CL_OK == rc )
        {
            pHeader->update_status = CL_LOG_STREAM_HEADER_UPDATE_INPROGRESS;
            pHeader->startAck += nFlushedRecords;
            pHeader->startAck %= (pHeader->maxRecordCount);
            pStreamData->seqNum += nFlushedRecords;
            nFlushableRecords -= nFlushedRecords;
            CL_LOG_DEBUG_TRACE(("startAck: %u remaining: %u",pHeader->startAck, nFlushableRecords));
            /* FIXME: put the number of overwritten records in log */
            if( 0 != pHeader->numOverwrite )
            {
                pHeader->numDroppedRecords = pHeader->numOverwrite;
                pHeader->numOverwrite = 0;
                CL_LOG_DEBUG_TRACE(("Log buffer full. [%d] records have been dropped", pHeader->numDroppedRecords));
            }
            pHeader->update_status = CL_LOG_STREAM_HEADER_UPDATE_COMPLETE;
        }
        else
        { 
            break;
        }
    }

    rc = clLogServerStreamMutexUnlock(pStreamData);
	if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Failed to unlock the stream");
    }

    logRecordsFlush(&flushRecord);
    clLogServerStreamMutexLockFlusher(pStreamData);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    

#else

static ClRcT
clLogFlusherRecordsFlush(ClLogSvrStreamDataT  *pStreamData)
{    
    ClRcT     rc        = CL_OK;
    ClLogStreamHeaderT     *pHeader           = pStreamData->pStreamHeader;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClUint32T              nFlushableRecords  = 0;
    ClUint32T              nFlushedRecords    = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (pStreamData->fileOwnerAddr != clIocLocalAddressGet()) && 
        (0 == pStreamData->ackersCount + pStreamData->nonAckersCount) )
    {
        clLogInfo("LOG", "FLS", "No one has registered for the Stream");
        return CL_OK;
    }    

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    

    CL_LOG_DEBUG_TRACE((" recordIdx: %d startAck : %d \n", pHeader->recordIdx,
                pHeader->startAck));

    if (pHeader->recordIdx < pHeader->startAck)
    {
        nFlushableRecords = (pHeader->recordIdx + pHeader->maxRecordCount) - pHeader->startAck;
    }
    else
    {
        nFlushableRecords = abs( pHeader->recordIdx - pHeader->startAck );
    }    

    while( nFlushableRecords > 0 )
    {
        nFlushedRecords = (nFlushableRecords > pSvrEoEntry->maxFlushLimit)
            ? pSvrEoEntry->maxFlushLimit : nFlushableRecords;

        rc = clLogFlusherRecordsMcast(pStreamData, nFlushedRecords);
        if( CL_OK == rc )
        {
            pHeader->startAck += nFlushedRecords;
            pHeader->startAck %= (pHeader->maxRecordCount);
            pStreamData->seqNum += nFlushedRecords;
            nFlushableRecords -= nFlushedRecords;
            CL_LOG_DEBUG_TRACE(("startAck: %u remaining: %u",
                        pHeader->startAck, nFlushableRecords));
            /* FIXME: put the number of overwritten records in log */
            if( 0 != pHeader->numOverwrite )
            {
                CL_LOG_DEBUG_TRACE((" %d records have been dropped",
                            pHeader->numOverwrite));
            }
            pHeader->numOverwrite = 0;
        }
        else
        {
            return CL_OK;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    
#endif

ClRcT
clLogFlusherTimerCallback(void  *pData)
{
    ClHandleT  hFlusher = *(ClHandleT *)(ClWordT)pData;

    CL_LOG_DEBUG_TRACE(("Enter"));

    clLogInfo("LOG", "FLS", "Timer expired");

    clLogFlusherCookieHandleDestroy(hFlusher, CL_FALSE);

    clHeapFree(pData);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    

ClRcT
clLogFlusherCookieHandleDestroy(ClHandleT  hFlusher, 
                                ClBoolT    timerExpired)
{
    ClRcT     rc   = CL_OK;
    ClLogFlushCookieT  *pFlushCookie = NULL;
    ClLogSvrEoDataT    *pSvrEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    /*
     * FIXME:
     * Unable to flush this set of records but will this be true in
     * future also, DON'T know
     * Also need to reset the startAck otherwise it will not  enter
     * the cond_wait
     */
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if(  CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleValidate(pSvrEoEntry->hFlusherDB, hFlusher);
    if( CL_OK != rc )
    {
        return rc;/*Flusher handle has already been destroyed*/
    }

    rc = clHandleCheckout(pSvrEoEntry->hFlusherDB, hFlusher, 
                          (void **) &pFlushCookie);

    if( (CL_TRUE == timerExpired) && (CL_OK != rc) )
    {
        clLogTrace("LOG", "FLS", "Timer has already destroyed the handle");
        return CL_OK;
    }    
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "Flusher handle checkout failed : "
                   "rc[0x %x]", rc);
        return rc;
    }

    if( CL_FALSE == timerExpired )
    {
        clLogWarning("LOG", "FLS", "Didn't get ack for %d records",
                     pFlushCookie->numRecords);
    }    
    CL_LOG_CLEANUP(clTimerDelete(&pFlushCookie->hTimer), CL_OK);

    rc = clHandleCheckin(pSvrEoEntry->hFlusherDB, hFlusher);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clHandleCheckin(): rc[0x %x]", rc);
    }    
    CL_LOG_CLEANUP(clHandleDestroy(pSvrEoEntry->hFlusherDB, hFlusher), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

static ClRcT
clLogFlusherCookieHandleCreate(ClUint32T       numRecords,
                               ClTimerHandleT  *phTimer, 
                               ClHandleT       *phFlusher)
{
    ClRcT     rc   = CL_OK;
    ClLogFlushCookieT  *pFlushCookie = NULL;
    ClLogSvrEoDataT    *pSvrEoEntry  = NULL;
    ClTimerTimeOutT    timeout       = {0, 8000L};
    ClHandleT          *pTimerArg    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if(  CL_OK != rc )
    {
        return rc;
    }    
    if( CL_FALSE == pSvrEoEntry->logInit )
    {
        clLogError("LOG", "FLS", "Log Service has terminated...");
        return CL_OK;
    }
    CL_LOG_DEBUG_TRACE(("hFlusherDB: %p ", (ClPtrT) pSvrEoEntry->hFlusherDB));

    rc = clHandleCreate(pSvrEoEntry->hFlusherDB, sizeof(ClLogFlushCookieT), 
                        phFlusher);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clHandleCreate(): rc[0x %x]", rc);
        return rc;
    }    
    pTimerArg = (ClHandleT*) clHeapCalloc(1, sizeof(ClHandleT));

    if( NULL == pTimerArg )
    {
    	clLogError("LOG", "FLS", "clHeapCalloc() : rc[0x %x]", rc);
        CL_LOG_CLEANUP(clHandleDestroy(pSvrEoEntry->hFlusherDB, *phFlusher),
                       CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    *pTimerArg = *phFlusher;

    rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT, 
                               CL_TIMER_SEPARATE_CONTEXT, 
                               clLogFlusherTimerCallback,
                               (ClPtrT)(ClWordT) pTimerArg, phTimer);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clTimerCreate(): rc[0x %x]", rc);
        CL_LOG_CLEANUP(clHandleDestroy(pSvrEoEntry->hFlusherDB, *phFlusher),
                       CL_OK);
        clHeapFree(pTimerArg);
        return rc;
    }    

    rc = clHandleCheckout(pSvrEoEntry->hFlusherDB, *phFlusher, 
            (void **) &pFlushCookie);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clHandleCheckout(): rc[0x %x]", rc);
        CL_LOG_CLEANUP(clTimerDelete(phTimer), CL_OK);
        CL_LOG_CLEANUP(clHandleDestroy(pSvrEoEntry->hFlusherDB, *phFlusher),
                CL_OK);
        return rc;
    }    
    pFlushCookie->numRecords = numRecords;
    pFlushCookie->hTimer     = *phTimer;
    rc = clHandleCheckin(pSvrEoEntry->hFlusherDB, *phFlusher);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clHandleCheckin(): rc[0x %x]", rc);
        CL_LOG_CLEANUP(clTimerDelete(phTimer), CL_OK);
        CL_LOG_CLEANUP(clHandleDestroy(pSvrEoEntry->hFlusherDB, *phFlusher),
                CL_OK);
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

#ifndef POSIX_BUILD
static void clLogVerifyAndFlushRecords(ClUint8T *pBuffer, ClLogStreamHeaderT *pHeader,
                                       ClLogFlushRecordT *pFlushRecord, ClUint32T nRecords)
{
    ClUint32T           recordIndex  = 0;
    ClUint32T           wipIdx = 0;
    ClUint32T           validRecordCount= 0;
    ClUint32T           size = 0;
    

    CL_ASSERT(pBuffer != NULL);
    CL_ASSERT(pHeader != NULL);
    CL_ASSERT(pFlushRecord != NULL);
    wipIdx = pHeader->recordSize - 1;

    /*Validate records until bad record found and flush only valid records to avoid memory corrupiton */
    for (validRecordCount =0, recordIndex = 0; recordIndex < nRecords; recordIndex++)
    {
        if ((pBuffer + (recordIndex * pHeader->recordSize))[wipIdx]  == CL_LOG_RECORD_WRITE_COMPLETE)
        { /* If the Record is Valid Continue to check for next record validity */
            validRecordCount++;
            (pBuffer + (recordIndex * pHeader->recordSize))[wipIdx] = '\0'; //To mask this byte in the logs.
        }
        else
        { /* If Record is Invalid, then skip all remaining records  */
            clLogAlert("SVR", "FLU", "Invalid Record is Found, So Skipping remainging Records");
            break;
        }
    }
    if (validRecordCount)
    {
        size = (pFlushRecord->pBuffs[pFlushRecord->numBufs].numRecords) * (pHeader->recordSize); 
        memcpy(pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord + size, pBuffer, (validRecordCount *  pHeader->recordSize));
        pFlushRecord->pBuffs[pFlushRecord->numBufs].numRecords += validRecordCount;
        CL_LOG_DEBUG_VERBOSE(("Copied from: %p to %u", pBuffer, validRecordCount * pHeader->recordSize));
    }
}

static ClRcT
clLogFlusherRecordsGetMcast(ClLogSvrStreamDataT  *pStreamData,
                            ClUint32T            nRecords,
                            ClLogFlushRecordT    *pFlushRecord)
{
    ClRcT      rc       = CL_OK;
    ClLogStreamHeaderT  *pHeader  = pStreamData->pStreamHeader;
    ClUint8T            *pRecords = pStreamData->pStreamRecords;
    ClUint32T           startIdx  = 0;
    ClUint32T           buffLen   = 0;
    ClIocNodeAddressT   localAddr = 0;
    ClUint8T            *pBuffer  = NULL;
    ClUint32T           firstBatch = 0;
    ClBoolT             doMulticast = CL_FALSE;
    ClUint32T           secondBatch = 0;

    if ((CL_LOG_STREAM_HEADER_STRUCT_ID != pHeader->struct_id) || (CL_LOG_STREAM_HEADER_UPDATE_COMPLETE != pHeader->update_status))
    {/* Stream Header is corrupted so reset Header parameters */
       clLogStreamHeaderReset(pHeader); 
    }
    if(pFlushRecord->multicast < 0 )
    {
        doMulticast = ( (0 < (pStreamData->ackersCount + pStreamData->nonAckersCount)) &&
                        (pHeader->streamMcastAddr.iocMulticastAddress != 0) )? CL_TRUE: CL_FALSE;

        if( (pStreamData->ackersCount + pStreamData->nonAckersCount) == 1 && 
            (pStreamData->fileOwnerAddr == clIocLocalAddressGet()) )
        {
            doMulticast = CL_FALSE;
        }
        pFlushRecord->multicast = doMulticast;
        pFlushRecord->mcastAddr = pHeader->streamMcastAddr;
        pFlushRecord->ackersCount = pStreamData->ackersCount;
    }
    else
    {
        doMulticast = (pFlushRecord->multicast ? CL_TRUE : CL_FALSE) ;
    }

    localAddr = clIocLocalAddressGet();
    if((!doMulticast) && (pStreamData->fileOwnerAddr != localAddr))
    { /*Nobody is interested in these records and they are not for me then skip them */
      /*  clLogDebug("SVR", "FLU", "Nobody is Interested in These records, So skipping them");*/
        return rc;
    }

    startIdx = pHeader->startAck % pHeader->maxRecordCount;
    if(nRecords > pHeader->maxRecordCount)
        nRecords = pHeader->maxRecordCount;

    CL_ASSERT(pHeader->recordSize < 4*1024);  // Sanity check the log record size
    buffLen = nRecords * pHeader->recordSize;

    clLogTrace(CL_LOG_AREA_SVR, "FLU", "startIdx: %u maxRec: %u nRecords: %u startIdx: %d recordIdx: %d", startIdx,
               pHeader->maxRecordCount, nRecords, pHeader->startAck, pHeader->recordIdx);
    /* FirstBatch is from startIdx towards maxRecordCount and SecondBatch is from 0 to startIdx 
     * SecondBatch is only valid if number of records are greater than (maxRecordCount - startIdx)
     */
    if ( (startIdx + nRecords) <= pHeader->maxRecordCount )
    {
        firstBatch = nRecords;
        secondBatch = 0;
    }
    else
    {
        firstBatch = pHeader->maxRecordCount - startIdx;
        secondBatch = nRecords + startIdx - pHeader->maxRecordCount;
    }
    /* Computed firstBatch and secondBatch number of records, now verify and flush them */
    pBuffer = pRecords + (startIdx * pHeader->recordSize);

    pFlushRecord->pBuffs = (ClLogFlushBufferT*) clHeapRealloc(pFlushRecord->pBuffs, (pFlushRecord->numBufs+1)*sizeof(*pFlushRecord->pBuffs));
    CL_ASSERT(pFlushRecord->pBuffs != NULL);
    memset(pFlushRecord->pBuffs+pFlushRecord->numBufs, 0, sizeof(*pFlushRecord->pBuffs));
    pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord = (ClUint8T*) clHeapCalloc(sizeof(ClUint8T), buffLen);
    CL_ASSERT(pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord != NULL);

    pFlushRecord->pBuffs[pFlushRecord->numBufs].numRecords = 0;
    if (firstBatch)
    {
        clLogVerifyAndFlushRecords(pBuffer, pHeader, pFlushRecord, firstBatch);
    }
    if (secondBatch)
    {
        pBuffer = pRecords;
        clLogVerifyAndFlushRecords(pBuffer, pHeader, pFlushRecord, secondBatch);
    }
    pFlushRecord->numBufs++;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

#else

/*
 * Called with the log server flusher stream lock. held.
 */
static ClRcT
clLogFlusherRecordsMcast(ClLogSvrStreamDataT  *pStreamData,
                         ClUint32T            nRecords)
{
    ClRcT      rc       = CL_OK;
    ClLogStreamHeaderT  *pHeader  = pStreamData->pStreamHeader;
    ClUint8T            *pRecords = pStreamData->pStreamRecords;
    ClUint32T           startIdx  = 0;
    ClUint32T           buffLen   = 0;
    ClHandleT           hFlusher  = CL_HANDLE_INVALID_VALUE;     
    ClTimerHandleT      hTimer    = CL_HANDLE_INVALID_VALUE;  
    ClIdlHandleT        hIdlHdl   = CL_HANDLE_INVALID_VALUE;
    ClIocNodeAddressT   localAddr = 0;
    ClUint8T            *pBuffer  = NULL;
    ClUint32T           size      = 0;
    ClUint32T           firstBatch = 0;
    ClUint32T           secondPatch = 0;
    ClBoolT             doMulticast = 
        ( (0 < (pStreamData->ackersCount + pStreamData->nonAckersCount)) &&
          (pHeader->streamMcastAddr.iocMulticastAddress != 0) )? CL_TRUE: CL_FALSE;

    clLogDebug("SVR", "FLU", "Enter: nRecords: %u seqNum: %u fileOwnerAddr:[%d]", nRecords,
            pStreamData->seqNum, pStreamData->fileOwnerAddr);

    if( (pStreamData->ackersCount + pStreamData->nonAckersCount) == 1 && 
        (pStreamData->fileOwnerAddr == clIocLocalAddressGet()) )
    {
        doMulticast = CL_FALSE;
    }
    /* 
     * If some fileowner has registered for this stream, if then multicast,
     * otherwise just send it to local guy 
     */
    if( CL_TRUE == doMulticast )
    {
        rc = clLogIdlHandleInitialize(pHeader->streamMcastAddr, &hIdlHdl);
        if( CL_OK != rc )
        {
            return rc;
        }
        if( pStreamData->ackersCount > 0 )
        {
            rc = clLogFlusherCookieHandleCreate(nRecords, &hTimer, &hFlusher);
            if( CL_OK != rc )
            {
                CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);
                return rc;
            }
            CL_LOG_DEBUG_TRACE(("hFlusher: %#llX", hFlusher));
        }
    }

    localAddr = clIocLocalAddressGet();
    startIdx = pHeader->startAck % pHeader->maxRecordCount;
    if(nRecords > pHeader->maxRecordCount)
        nRecords = pHeader->maxRecordCount;

    buffLen = nRecords * pHeader->recordSize;
    /*
     * Drop the server stream mutex unlock now and grab it again after file owner writes
     * and before exiting out of the function
     */
    clLogDebug(CL_LOG_AREA_SVR, "FLU", "startIdx: %u maxRec: %u nRecords: %u startIdx: %d recordIdx: %d", startIdx,
                    pHeader->maxRecordCount, nRecords, pHeader->startAck, pHeader->recordIdx);
    if( (startIdx + nRecords) <= pHeader->maxRecordCount )
    {
        pBuffer = pRecords + (startIdx * pHeader->recordSize);
        if( pStreamData->fileOwnerAddr == localAddr ) 
        {
            /* File onwer function which flushes the data into file */
            rc = clLogFileOwnerEntryFindNPersist(&pStreamData->fileName, 
                    &pStreamData->fileLocation, nRecords, pBuffer);
            if( CL_OK != rc )
            {
                return rc;
            }
        }
        if( doMulticast == CL_TRUE )
        {
            rc = VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)(hIdlHdl, 
                    pHeader->streamMcastAddr.iocMulticastAddress, 
                    pStreamData->seqNum, localAddr, 
                    hFlusher, nRecords, buffLen, pBuffer,
                    NULL, 0);
            CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);
        }
        CL_LOG_DEBUG_VERBOSE(("Copied from: %p to %u", pRecords + startIdx,
                    nRecords * pHeader->recordSize));
    }
    else
    {
        CL_LOG_DEBUG_TRACE(("startIdx: %u maxRec: %u nRecords: %u", startIdx,
                    pHeader->maxRecordCount, nRecords));
        firstBatch = pHeader->maxRecordCount - startIdx;
        pBuffer = pRecords + (startIdx * pHeader->recordSize);
        secondPatch = nRecords - firstBatch;
        if( pStreamData->fileOwnerAddr == localAddr )
        {
            /*
             *  Make two calls to fileowner function, so that no need to
             *  allocate memory, it just direct copy from shared memory 
             */
            rc = clLogFileOwnerEntryFindNPersist(&pStreamData->fileName, 
                    &pStreamData->fileLocation, firstBatch, pBuffer);
            if( CL_OK != rc )
            {
                return rc;
            }
            rc = clLogFileOwnerEntryFindNPersist(&pStreamData->fileName, 
                    &pStreamData->fileLocation, secondPatch, pRecords);
            if( CL_OK != rc )
            {
                return rc;
            }
        }
        if( doMulticast == CL_TRUE )
        {
            pBuffer = clHeapCalloc(buffLen, sizeof(ClUint8T));
            if( NULL == pBuffer )
            {
                clLogError("LOG", "FLS", "clHeapCalloc() : rc[0x %x]", rc);
                CL_LOG_CLEANUP(clLogFlusherCookieHandleDestroy(hFlusher, CL_FALSE), CL_OK);
                CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);
                return CL_LOG_RC(CL_ERR_NO_MEMORY);
            }
            size = firstBatch * pHeader->recordSize;
            memcpy(pBuffer, pRecords + (startIdx * pHeader->recordSize), size);
            memcpy(pBuffer + size, pRecords, buffLen - size); 
            rc = VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)(hIdlHdl, 
                    pHeader->streamMcastAddr.iocMulticastAddress, 
                    pStreamData->seqNum, localAddr, 
                    hFlusher, nRecords, buffLen, pBuffer,
                    NULL, 0);
            clHeapFree(pBuffer);
            CL_LOG_CLEANUP(clIdlHandleFinalize(hIdlHdl), CL_OK);
        }
    }
    if( (doMulticast == CL_TRUE) && (CL_OK != rc) )
    {
        clLogError("LOG", "FLS", "VDECL_VER(clLogClntFileHdlrDataReceiveClientAsync, 4, 0, 0)(): "
                "rc[0x %x]", rc);
        CL_LOG_CLEANUP(clLogFlusherCookieHandleDestroy(hFlusher, CL_FALSE), CL_OK);
        return rc;
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

#endif    

ClRcT
VDECL_VER(clLogHandlerSvrAckSend, 4, 0, 0)(
                       SaNameT            *pStreamName,
                       SaNameT            *pStreamScopeNode,
                       ClLogStreamScopeT  streamScope,
                       ClUint32T          seqeunceNum,
                       ClUint32T          numRecords, 
                       ClHandleT          hFlusher)
{
    ClRcT     rc       = CL_OK;
    ClLogSvrEoDataT      *pSvrEoEntry    = NULL;
    ClCntNodeHandleT     hSvrStreamNode  = CL_HANDLE_INVALID_VALUE;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;
    ClBoolT              addedEntry      = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_DEBUG_TRACE(("seqeunceNum: %d numRecords: %d hFlusher: %#llX", 
                        seqeunceNum, numRecords, hFlusher));
    CL_LOG_CLEANUP(clLogFlusherCookieHandleDestroy(hFlusher, CL_TRUE), CL_OK);
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clLogSvrEoEntryGet(): rc[0x %x]", rc);
        return rc;
    }    
    if( CL_FALSE == pSvrEoEntry->logInit )
    {
        clLogError("LOG", "FLS", "Log Server is shutting down...");
        return CL_OK;
    }   
    if( 0 != numRecords )
    {
        CL_LOG_DEBUG_TRACE(("Exit"));
        return CL_OK;
    }    

    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_FALSE, &hSvrStreamNode, &addedEntry);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clLogSvrStreamEntryGet(): rc[0x %x]", rc);
        return rc;
    }    

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                              (ClCntDataHandleT *)&pSvrStreamData);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clCntNodeUserDataGet(): rc[0x %x]", rc);
        return rc;
    }

    if( NULL == pSvrStreamData->pStreamHeader )
    {
        clLogError("LOG", "FLS", "pStreamHeader is NULL\n");
        return rc;            
    }    

    rc = clLogServerStreamMutexLock(pSvrStreamData);
    if( CL_OK != rc )
    {
        clLogError("LOG", "FLS", "clOsalMutexLock_L(): rc[0x %x]", rc);
        return rc;
    }    
    pSvrStreamData->pStreamHeader->streamStatus = CL_LOG_STREAM_HALT;

    CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pSvrStreamData),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

static ClRcT
clLogFlusherExternalRecordsGetMcast(ClLogSvrStreamDataT  *pStreamData,
                            ClUint32T            nRecords,
                            ClLogFlushRecordT    *pFlushRecord,
                            ClUint8T            *pRecords)
{
    ClRcT      rc       = CL_OK;
    ClLogStreamHeaderT  *pHeader  = pStreamData->pStreamHeader;
    ClUint32T           startIdx  = 0;
    ClUint32T           buffLen   = 0;
    ClIocNodeAddressT   localAddr = 0;
    ClUint8T            *pBuffer  = NULL;
    ClBoolT             doMulticast = CL_FALSE;    
    if(pFlushRecord->multicast < 0 )
    {
        doMulticast = ( (0 < (pStreamData->ackersCount + pStreamData->nonAckersCount)) &&
                        (pHeader->streamMcastAddr.iocMulticastAddress != 0) )? CL_TRUE: CL_FALSE;

        if( (pStreamData->ackersCount + pStreamData->nonAckersCount) == 1 &&
            (pStreamData->fileOwnerAddr == clIocLocalAddressGet()) )
        {
            doMulticast = CL_FALSE;
        }
        pFlushRecord->multicast = doMulticast;
        pFlushRecord->mcastAddr = pHeader->streamMcastAddr;
        pFlushRecord->ackersCount = pStreamData->ackersCount;

    }
    else
    {
        doMulticast = (pFlushRecord->multicast ? CL_TRUE : CL_FALSE) ;
    }
    localAddr = clIocLocalAddressGet();

    buffLen = nRecords * pHeader->recordSize;
    pBuffer = pRecords;
    pFlushRecord->pBuffs = (ClLogFlushBufferT*) clHeapRealloc(pFlushRecord->pBuffs,
                                         (pFlushRecord->numBufs+1) * sizeof(*pFlushRecord->pBuffs));
    CL_ASSERT(pFlushRecord->pBuffs != NULL);
    memset(pFlushRecord->pBuffs + pFlushRecord->numBufs, 0, sizeof(*pFlushRecord->pBuffs));
    pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord = (ClUint8T*) clHeapCalloc(sizeof(ClUint8T), buffLen);
    CL_ASSERT(pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord != NULL);
    memcpy(pFlushRecord->pBuffs[pFlushRecord->numBufs].pRecord,
           pBuffer, buffLen);
    pFlushRecord->pBuffs[pFlushRecord->numBufs++].numRecords = nRecords;
    CL_LOG_DEBUG_VERBOSE(("Copied from: %p to %u", pRecords + (startIdx*pHeader->recordSize),
                          nRecords * pHeader->recordSize));
    CL_LOG_DEBUG_TRACE(("Exit"));
    (void)localAddr;
    return rc;
}

ClRcT
clLogFlusherExternalRecordsFlush(ClLogSvrStreamDataT  *pStreamData,
		                             ClUint8T          *pRecord)
{	
    ClRcT     rc        = CL_OK;
    ClLogStreamHeaderT     *pHeader           = pStreamData->pStreamHeader;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    //ClUint32T              nFlushableRecords  = 0;
    ClUint32T              nFlushedRecords    = 1;
    ClLogFlushRecordT      flushRecord = {0};
    CL_LOG_DEBUG_TRACE(("Enter"));
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE((" recordIdx: %d startAck : %d \n", pHeader->recordIdx, pHeader->startAck));
    flushRecord.fileName.pValue = (ClCharT*) clHeapCalloc(1, pStreamData->fileName.length+1);
    CL_ASSERT(flushRecord.fileName.pValue != NULL);
    flushRecord.fileName.length = pStreamData->fileName.length;
    memcpy(flushRecord.fileName.pValue, pStreamData->fileName.pValue, flushRecord.fileName.length);
    flushRecord.fileLocation.pValue = (ClCharT*) clHeapCalloc(1, pStreamData->fileLocation.length+1);
    CL_ASSERT(flushRecord.fileLocation.pValue != NULL);
    flushRecord.fileLocation.length = pStreamData->fileLocation.length;
    memcpy(flushRecord.fileLocation.pValue, pStreamData->fileLocation.pValue, flushRecord.fileLocation.length);

    flushRecord.fileOwnerAddr = pStreamData->fileOwnerAddr;
    flushRecord.seqNum = pStreamData->seqNum;
    flushRecord.multicast = -1;
    flushRecord.recordSize = pHeader->recordSize;
    rc = clLogFlusherExternalRecordsGetMcast(pStreamData, nFlushedRecords, &flushRecord,pRecord);
    rc = clLogServerStreamMutexUnlock(pStreamData);
	if( CL_OK != rc )
    {
        clLogError("SVR", "FLU", "Faild to unlock the stream");
    }
    logRecordsFlush(&flushRecord);
    clLogServerStreamMutexLockFlusher(pStreamData);
    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}


