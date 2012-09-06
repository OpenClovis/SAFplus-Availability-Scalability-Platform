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
#include <string.h>
#include <sys/stat.h>
#include <LogPortSvrClient.h>

#include <clLogApi.h>
#include <clLogApiExt.h>
#include <clLogErrors.h>
#include <clLogOsal.h>
#include <clLogDebug.h>
#include <clLogCommon.h>
#include <clLogClientEo.h>
#include <clLogClientHandle.h>
#include <clLogClientStream.h>
#include <clLogClient.h>

static ClRcT
clLogClntFilterPass(ClLogSeverityT      severity,
                    ClUint16T           msgId,
                    ClUint32T           compId,
                    ClLogStreamHeaderT  *pStreamHeader);

static ClRcT
clLogClientMsgArgCopy(ClUint16T   msgId,
                      va_list     args,
                      ClUint32T   bufLength,
                      ClUint8T    *pBuffer);

ClRcT
clLogClntStreamOpen(ClLogHandleT        hLog,
                    ClNameT             *pStreamName,
                    ClNameT             *pNodeName,
                    ClStringT           *pShmName,
                    ClUint32T           shmSize,
                    ClLogStreamHandleT  *phStream)
{
    ClRcT                 rc              = CL_OK;
    ClLogClntEoDataT      *pClntEoEntry   = NULL;
    ClBoolT               addedTable      = CL_FALSE;
    ClBoolT               addedEntry      = CL_FALSE;
    ClCntNodeHandleT      hClntStreamNode = CL_HANDLE_INVALID_VALUE;
    ClLogClntStreamDataT  *pUserData      = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pNodeName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pShmName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((sizeof(ClLogStreamHeaderT) > shmSize),
                      CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((NULL == phStream), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&(pClntEoEntry->clntStreamTblLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE == pClntEoEntry->hClntStreamTable )
    {
        rc = clCntHashtblCreate(pClntEoEntry->maxStreams,
                                clLogStreamKeyCompare,
                                clLogStreamHashFn,
                                clLogClntStreamDeleteCb,
                                clLogClntStreamDeleteCb,
                                CL_CNT_UNIQUE_KEY,
                                &(pClntEoEntry->hClntStreamTable));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntHashtblCreate(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(
                clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),CL_OK);
            return rc;
        }
        addedTable = CL_TRUE;
        CL_LOG_DEBUG_VERBOSE(("Created the HashTable"));
    }

    rc = clLogClntStreamEntryGet(pClntEoEntry, pStreamName, pNodeName,
                                 pShmName, shmSize, &hClntStreamNode,
                                 &addedEntry);
    if( CL_OK != rc)
    {
        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hClntStreamTable), CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),
                       CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Got the stream entry"));

    rc = clLogHandleStreamHandleCreate(hLog, hClntStreamNode, phStream);
    if( CL_OK != rc)
    {
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hClntStreamTable,
                                           hClntStreamNode), CL_OK);
        }
        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hClntStreamTable), CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),
                       CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Created the streamhandle"));

    rc = clCntNodeUserDataGet(pClntEoEntry->hClntStreamTable, hClntStreamNode,
                              (ClCntDataHandleT *) &pUserData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB,
                                             *phStream), CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hClntStreamTable,
                                           hClntStreamNode), CL_OK);
        }

        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hClntStreamTable), CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),
                       CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Got stream entry"));

    rc = clBitmapBitSet(pUserData->hStreamBitmap, CL_HDL_IDX(*phStream));
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB, *phStream),
                       CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hClntStreamTable,
                                           hClntStreamNode), CL_OK);
        }

        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hClntStreamTable), CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),
                       CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Updated stream bitmap"));

    rc = clLogHandleInitHandleStreamAdd(hLog, *phStream);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBitmapBitClear(pUserData->hStreamBitmap, CL_HDL_IDX(*phStream)),
                       CL_OK);
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB, *phStream),
                       CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hClntStreamTable,
                                           hClntStreamNode), CL_OK);
        }

        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hClntStreamTable), CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock)),
                       CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Updated init handle"));

    rc = clOsalMutexUnlock_L(&(pClntEoEntry->clntStreamTblLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogClntStreamDeleteCb(ClCntKeyHandleT   userKey,
                        ClCntDataHandleT  userData)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_CLEANUP(
        clLogClntStreamDataCleanup((ClLogClntStreamDataT *) userData), CL_OK);
    clHeapFree((ClLogClntStreamDataT *) userData);
    clLogStreamKeyDestroy((ClLogStreamKeyT *) userKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
}

ClRcT
clLogClntStreamDataCleanup(ClLogClntStreamDataT *pData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));


    rc = clBitmapDestroy(pData->hStreamBitmap);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapDestroy(): rc[0x %x]", rc));
    }

#ifdef VXWORKS_BUILD
    clLogClientStreamMutexDestroy(pData);
    clLogClientStreamFlusherMutexDestroy(pData);
#endif

    rc = clOsalMunmap_L(pData->pStreamHeader, pData->pStreamHeader->shmSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMunmap(): rc[0x %x]", rc));
    }

    clHeapFree(pData->shmName.pValue);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamEntryGet(ClLogClntEoDataT  *pClntEoEntry,
                        ClNameT           *pStreamName,
                        ClNameT           *pNodeName,
                        ClStringT         *pShmName,
                        ClUint32T         shmSize,
                        ClCntNodeHandleT  *phStreamNode,
                        ClBoolT           *pAddedEntry)
{
    ClRcT             rc            = CL_OK;
    ClLogStreamKeyT   *pStreamKey   = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pAddedEntry = CL_FALSE;

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pNodeName,
                              pClntEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeFind(pClntEoEntry->hClntStreamTable,
                       (ClCntKeyHandleT) pStreamKey,
                       phStreamNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clLogClntStreamEntryAdd(pClntEoEntry->hClntStreamTable,
                                     pStreamKey, pShmName, shmSize,
                                     phStreamNode);
        if( CL_OK != rc )
        {
            clLogStreamKeyDestroy(pStreamKey);
        }
        else
        {
            *pAddedEntry = CL_TRUE;
        }
        return rc;
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamEntryAdd(ClCntHandleT       hClntTable,
                        ClLogStreamKeyT    *pStreamKey,
                        ClStringT          *pShmName,
                        ClUint32T          shmSize,
                        ClCntNodeHandleT   *phStreamNode)
{
    ClRcT                  rc           = CL_OK;
    ClLogClntStreamDataT   *pStreamData = NULL;
    ClInt32T               fd           = 0;
    ClUint32T              headerSize   = 0;
    ClInt32T               tries        = 0;
    struct stat statbuf;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pStreamData = clHeapCalloc(1, sizeof(ClLogClntStreamDataT));
    if( pStreamData == NULL )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    
    pStreamData->shmName.pValue = clHeapCalloc(pShmName->length,
                                               sizeof(ClCharT));
    if( NULL == pStreamData->shmName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(pStreamData);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    memcpy(pStreamData->shmName.pValue, pShmName->pValue, pShmName->length);
    CL_LOG_DEBUG_VERBOSE(("Opening Shared Memory Segment: %s",
                          pStreamData->shmName.pValue));

    rc = clOsalShmOpen_L(pStreamData->shmName.pValue, CL_LOG_SHM_OPEN_FLAGS,
                         CL_LOG_SHM_MODE, &fd);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalShmOpen(): rc[0x %x]", rc));
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        return rc;
    }
    /*
     * Check to be safe w.r.t SIGBUS in case a parallel ftruncate from log server is in progress
     */
    memset(&statbuf, 0, sizeof(statbuf));
    while(tries++ < 3 && !fstat(fd, &statbuf))
    {
        if(statbuf.st_size < shmSize)
        {
            sleep(1);
        }
        else break;
    }
    if(!statbuf.st_size)
    {
        CL_LOG_DEBUG_ERROR(("fstat on shared segment with size 0"));
        CL_LOG_CLEANUP(clOsalShmClose_L(fd), CL_OK);
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        return CL_LOG_RC(CL_ERR_LIBRARY);
    }
    rc = clOsalMmap_L(NULL, shmSize, CL_LOG_MMAP_PROT_FLAGS, CL_LOG_MMAP_FLAGS,
                      fd, 0, (void **) &pStreamData->pStreamHeader);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalShmClose_L(fd), CL_OK);
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        return rc;
    }
    CL_LOG_CLEANUP(clOsalShmClose_L(fd), CL_OK);
    CL_LOG_DEBUG_VERBOSE(("Mmapped Shared Memory Segment"));

    pStreamData->pStreamHeader->shmSize = shmSize;
    headerSize                  =
        CL_LOG_HEADER_SIZE_GET(pStreamData->pStreamHeader->maxMsgs, 
                               pStreamData->pStreamHeader->maxComps);
    pStreamData->pStreamRecords = ((ClUint8T *) (pStreamData->pStreamHeader)) +
        headerSize;
    CL_LOG_DEBUG_VERBOSE(("pStreamHeader : %p",
                          (void *) pStreamData->pStreamHeader));
    CL_LOG_DEBUG_VERBOSE(("msgMap: %p",
                          (void *) (pStreamData->pStreamHeader + 1)));
    CL_LOG_DEBUG_VERBOSE(("pStreamRecords: %p", pStreamData->pStreamRecords));

    rc = clBitmapCreate(&(pStreamData->hStreamBitmap), 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMunmap_L(pStreamData->pStreamHeader, shmSize),
                       CL_OK);
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        return rc;
    }

    /*
     * Create the shared lock for the sem.
     */
    rc = clLogClientStreamSharedMutexInit(pStreamData, pShmName);

    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalProcessSharedMutexInit(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pStreamData->hStreamBitmap), CL_OK);
        CL_LOG_CLEANUP(clOsalMunmap_L(pStreamData->pStreamHeader, shmSize),
                       CL_OK);
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        return rc;
    }

    rc = clCntNodeAddAndNodeGet(hClntTable,(ClCntKeyHandleT) pStreamKey,
                                (ClCntDataHandleT) pStreamData, NULL,
                                phStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pStreamData->hStreamBitmap), CL_OK);
        CL_LOG_CLEANUP(clLogClientStreamMutexDestroy(pStreamData), CL_OK);
        CL_LOG_CLEANUP(clOsalMunmap_L(pStreamData->pStreamHeader, shmSize), CL_OK);
        clHeapFree(pStreamData->shmName.pValue);
        clHeapFree(pStreamData);
        pStreamData = NULL;
        
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamWriteWithHeader(ClLogClntEoDataT    *pClntEoEntry,
                               ClLogSeverityT      severity,
                               ClUint16T           serviceId,
                               ClUint16T           msgId,
                               ClCharT             *pMsgHeader,
                               va_list             args,
                               ClCntNodeHandleT    hClntStreamNode)
{
    ClRcT                 rc              = CL_OK;
    ClLogClntStreamDataT  *pClntData      = NULL;
    ClLogStreamHeaderT    *pStreamHeader  = NULL;
    ClUint8T              *pStreamRecords = NULL;
    ClUint32T             nUnAcked        = 0;
    ClUint8T              *pBuffer        = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    CL_LOG_DEBUG_VERBOSE(("Severity: %d ServiceId: %hu MsgId: %hu CompId: %u",
                          severity, serviceId, msgId, pClntEoEntry->compId));

    rc = clCntNodeUserDataGet(pClntEoEntry->hClntStreamTable,  hClntStreamNode,
                              (ClCntDataHandleT *) &pClntData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    pStreamHeader  = pClntData->pStreamHeader;
    pStreamRecords = pClntData->pStreamRecords;
    CL_LOG_DEBUG_TRACE(("Hdr: %p Rec: %p", (void *) pStreamHeader,
                          pStreamRecords));

    rc = clLogClientStreamMutexLock(pClntData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_LOG_STREAM_ACTIVE != pStreamHeader->streamStatus )
    {
        CL_LOG_CLEANUP(clLogClientStreamMutexUnlock(pClntData), CL_OK);
        return (CL_LOG_STREAM_HALT == pStreamHeader->streamStatus)
               ? CL_LOG_RC(CL_LOG_ERR_FILE_FULL)
               : CL_LOG_RC(CL_ERR_NO_RESOURCE);
    }

    rc = clLogClntFilterPass(severity, msgId, pClntEoEntry->compId,
                             pStreamHeader);
    if( CL_OK != rc )
    {
        if( CL_ERR_BAD_OPERATION == CL_GET_ERROR_CODE(rc) )
        {
            rc = CL_OK; /* Its a filtered out record, not an error */
        }
        
        CL_LOG_CLEANUP(clLogClientStreamMutexUnlock(pClntData), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Passed the Filter criteria: %u",
                pStreamHeader->maxRecordCount));

    pBuffer = pStreamRecords + (pStreamHeader->recordSize *
              (pStreamHeader->recordIdx % pStreamHeader->maxRecordCount));
    rc = clLogClientMsgWriteWithHeader(severity, pStreamHeader->streamId, serviceId,
                                       msgId, pClntEoEntry->clientId, pStreamHeader->sequenceNum,
                                       pMsgHeader, args,
                                       pStreamHeader->recordSize, pBuffer);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogClientStreamMutexUnlock(pClntData), CL_OK);
        return rc;
    }
    ++pStreamHeader->sequenceNum;
    nUnAcked = abs(pStreamHeader->recordIdx - pStreamHeader->startAck);
    if( nUnAcked ==  pStreamHeader->maxRecordCount )
    {
        ++pStreamHeader->numOverwrite;
        ++pStreamHeader->startAck;
        pStreamHeader->startAck %= (2 * pStreamHeader->maxRecordCount);
        CL_LOG_DEBUG_TRACE(("In Overwriting mode...total: %u startAck: %u",
             pStreamHeader->numOverwrite, pStreamHeader->startAck));
    }
    ++pStreamHeader->recordIdx;
    pStreamHeader->recordIdx %= (2 * pStreamHeader->maxRecordCount);
    ++pStreamHeader->flushCnt;
    CL_LOG_DEBUG_TRACE(("recordIdx: %u startAck: %u",
                          pStreamHeader->recordIdx, pStreamHeader->startAck));

    if( (0 != pStreamHeader->flushFreq) &&
        (pStreamHeader->flushCnt == pStreamHeader->flushFreq) )
    {
        CL_LOG_DEBUG_TRACE(("Signaling @ %u", pStreamHeader->recordIdx));
        pStreamHeader->flushCnt = 0;
#ifndef POSIX_BUILD
        rc = clLogClientStreamSignalFlusher(pClntData,&(pStreamHeader->flushCond));
#else
        rc = clLogClientStreamSignalFlusher(pClntData, NULL);
#endif
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalCondSignal(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogClientStreamMutexUnlock(
                                                        pClntData),
                           CL_OK);
            return rc;
        }
    }

    rc = clLogClientStreamMutexUnlock(pClntData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamWrite(ClLogClntEoDataT    *pClntEoEntry,
                     ClLogSeverityT      severity,
                     ClUint16T           serviceId,
                     ClUint16T           msgId,
                     va_list             args,
                     ClCntNodeHandleT    hClntStreamNode)
{
    return clLogClntStreamWriteWithHeader(pClntEoEntry, severity, serviceId, msgId, NULL, 
                                          args, hClntStreamNode);
}

static ClRcT
clLogClntFilterPass(ClLogSeverityT      severity,
                    ClUint16T           msgId,
                    ClUint32T           compId,
                    ClLogStreamHeaderT  *pStreamHeader)
{
    ClRcT      rc          = CL_OK;
    ClUint16T  byteNum     = 0;
    ClUint8T   bitNum      = 0;
    ClUint8T   *pMsgIdSet  = NULL;
    ClUint8T   *pCompIdSet = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (CL_LOG_SEV_EMERGENCY > severity) || (CL_LOG_SEV_MAX < severity) )
    {
        CL_LOG_DEBUG_ERROR(("Passed Severity is invalid: passed: %d", severity));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    if( !(pStreamHeader->filter.severityFilter & (1 << (severity - 1))) )
    {
        CL_LOG_DEBUG_TRACE(("Failed Severity filter: %hu",
                            pStreamHeader->filter.severityFilter));
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    if( 0 != pStreamHeader->filter.msgIdSetLength )
    {
        byteNum = msgId / CL_BITS_PER_BYTE;
        bitNum  = msgId % CL_BITS_PER_BYTE;

        if( byteNum > pStreamHeader->filter.msgIdSetLength  )
        {
            CL_LOG_DEBUG_VERBOSE(("msgId not in set: byteNum: %hu", byteNum));
            return CL_LOG_RC(CL_ERR_BAD_OPERATION);
        }

        pMsgIdSet = (ClUint8T *) (pStreamHeader + 1);
        CL_LOG_DEBUG_VERBOSE(("pMsgIdSet start: %p", pMsgIdSet));
        if( pMsgIdSet[byteNum] & (1 << bitNum) )
        {
            CL_LOG_DEBUG_ERROR(("msgId masked byte: %hu bit: %u", byteNum,
                                bitNum));
            return CL_LOG_RC(CL_ERR_BAD_OPERATION);
        }
    }

    if( 0 != pStreamHeader->filter.compIdSetLength )
    {
        compId  = compId & CL_LOG_COMPID_CLASS;
        byteNum = compId / CL_BITS_PER_BYTE;
        bitNum  = compId % CL_BITS_PER_BYTE;
        if( byteNum < pStreamHeader->filter.compIdSetLength  )
        {
            pCompIdSet = (ClUint8T *) (pStreamHeader + 1);
            pCompIdSet += (pStreamHeader->maxMsgs/CL_BITS_PER_BYTE + 1);
            CL_LOG_DEBUG_VERBOSE(("pCompIdSet start: %p", pCompIdSet));
            if( pCompIdSet[byteNum]& (1 << bitNum) )
            {
                CL_LOG_DEBUG_ERROR(("compId masked byte: %hu bit: %u",
                                    byteNum, bitNum));
                return CL_LOG_RC(CL_ERR_BAD_OPERATION);
            }
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogClientMsgArgCopy(ClUint16T   msgId,
                      va_list     args,
                      ClUint32T   bufLength,
                      ClUint8T    *pBuffer)
{
    ClRcT  rc = CL_OK;

    switch(msgId)
    {
        case CL_LOG_MSGID_BUFFER:
        {
            ClUint32T  dataLen = 0;
            ClPtrT     pData   = NULL;

            dataLen = va_arg(args, ClUint32T);
            pData   = va_arg(args, void *);
            if( NULL == pData )
            {
                return CL_LOG_RC(CL_ERR_NULL_POINTER);
            }

            if( bufLength > sizeof(ClUint32T) )
            {
                memcpy(pBuffer, &dataLen, sizeof(ClUint32T));
                pBuffer   += sizeof(ClUint32T);
                bufLength -= sizeof(ClUint32T);

                memcpy(pBuffer,pData,
                       (dataLen < bufLength) ? dataLen : bufLength);
            }
            break;
        }

        case CL_LOG_MSGID_PRINTF_FMT:
        {
            ClInt32T   nBytes  = 0;
            ClCharT   *pFmtStr = va_arg(args, ClCharT *);
            ClUint32T tempLen  = bufLength - 2; /* In case of ascii, one for \n one for '\0' */

            if( (nBytes = (vsnprintf((char *) pBuffer, tempLen  , pFmtStr,
                            args))) < 0 )
            {
                rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                return rc;
            }
            pBuffer[CL_MIN(nBytes, tempLen)] = '\n';
            pBuffer[CL_MIN(nBytes + 1, tempLen + 1)] = 0;
            break;
        }

        default:
        {
            ClUint16T  tag        = 0;
            ClUint16T  valLength  = 0;
            ClPtrT     *pVal     = NULL;

            while( bufLength > sizeof(tag) )
            {
                tag = va_arg(args, ClUint32T);
                CL_LOG_DEBUG_VERBOSE(("tag: %hu", tag));
                memcpy(pBuffer, &tag, sizeof(tag));
                pBuffer   += sizeof(tag);
                bufLength -= sizeof(tag);
                if( CL_LOG_TAG_TERMINATE == tag )
                {
                    break; /*out of while */
                }

                if( bufLength > sizeof(valLength) )
                {
                    valLength = va_arg(args, ClUint32T);
                    CL_LOG_DEBUG_VERBOSE(("valLength: %hu", valLength));
                    memcpy(pBuffer, &valLength, sizeof(valLength));
                    pBuffer   += sizeof(valLength);
                    bufLength -= sizeof(valLength);
                }
                else
                {
                    break; /* out of while */
                }

                pVal = va_arg(args, void *);
                if( NULL == pVal )
                {
                    return CL_LOG_RC(CL_ERR_NULL_POINTER);
                }
                if( bufLength >= valLength )
                {
                    memcpy(pBuffer, pVal, valLength);
                    pBuffer   += valLength;
                    bufLength -= valLength;
                }
                else
                {
                    memcpy(pBuffer - sizeof(valLength), 
                           &bufLength, sizeof(bufLength));
                    memcpy(pBuffer, pVal, bufLength);
                    pBuffer   += bufLength;
                    bufLength  = 0;
                    break; /* out of while */
                }
            }
            break;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit:\n"));

    return rc;
}

ClRcT
clLogClientMsgWriteWithHeader(ClLogSeverityT     severity,
                              ClUint16T          streamId,
                              ClUint16T          serviceId,
                              ClUint16T          msgId,
                              ClUint32T          clientId,
                              ClUint64T          sequenceNum,
                              ClCharT            *pMsgHeader,
                              va_list            args,
                              ClUint32T          recSize,
                              ClUint8T           *pRecord)
{
#define __UPDATE_REC_SIZE do {                  \
        if(recSize < nbytes )                   \
            recSize = nbytes;                   \
        recSize -= nbytes;                      \
        if(!recSize)                            \
            return CL_OK;                       \
    }while(0)

    ClRcT             rc            = CL_OK;
    ClTimeT           timeStamp     = 0;
    ClUint8T          *pRecStart    = pRecord;
    ClUint16T         tmp           = 1;
    ClUint8T          endian        = *(ClUint8T *) &tmp;
    CL_LOG_DEBUG_TRACE(("Enter: recSize: %u @ %p", recSize, pRecord));

    memset(pRecord, 0, recSize);
    if( CL_LOG_MSGID_PRINTF_FMT == msgId )
    {
        ClInt32T nbytes = 0;

        if(recSize < LOG_ASCII_MIN_REC_SIZE ) /* just a minimum record size taking care of the headers*/
        {
            printf("LOG record size has to be minimum [%d] bytes. Got [%d] bytes\n", 
                   LOG_ASCII_MIN_REC_SIZE, recSize);
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        /*
         * In ASCII logging, we dont need header, so just keeping only the
         * required fields
         */
        nbytes = snprintf((ClCharT *)pRecord, recSize, LOG_ASCII_HDR_FMT, endian, severity & 0x1f); 
        pRecord += nbytes;
        __UPDATE_REC_SIZE;
        {
            ClCharT *pSeverity = clLogSeverityStrGet(severity);
            ClCharT c = 0;
            ClInt32T hdrLen = 0;
            ClInt32T len = 0;
            va_list argsCopy;
            ClCharT *pFmtStr ;
            va_copy(argsCopy, args);
            pFmtStr = va_arg(argsCopy, ClCharT *);
            if(pMsgHeader && pMsgHeader[0])
            {
                hdrLen = snprintf(&c, 1, "%s.%05lld : %6s) ",
                                  pMsgHeader, sequenceNum, pSeverity ? pSeverity : "DEBUG");
                if(hdrLen < 0) hdrLen = 0;
            }
            len = vsnprintf(&c, 1, pFmtStr, argsCopy);
            va_end(argsCopy);
            if(len < 0) len = 0;
            hdrLen = CL_MIN(hdrLen, recSize - LOG_ASCII_HDR_LEN - LOG_ASCII_DATA_LEN - 1);
            len = CL_MIN(len, recSize - LOG_ASCII_HDR_LEN - LOG_ASCII_DATA_LEN - hdrLen - 1);
            nbytes = snprintf((ClCharT*)pRecord, recSize - 1, LOG_ASCII_HDR_LEN_FMT, hdrLen);
            pRecord += nbytes;
            __UPDATE_REC_SIZE;
            nbytes = snprintf((ClCharT*)pRecord, recSize - 1, LOG_ASCII_DATA_LEN_DELIMITER_FMT, len);
            pRecord += nbytes;
            __UPDATE_REC_SIZE;
            if(pMsgHeader && pMsgHeader[0])
            {
                nbytes = snprintf((ClCharT*)pRecord, recSize - 1, "%s.%05lld : %6s) ",
                                  pMsgHeader, sequenceNum, pSeverity ? pSeverity : "DEBUG");
                if(nbytes < 0) nbytes = 0;
                pRecord += nbytes;
                __UPDATE_REC_SIZE;
            }
            pFmtStr = va_arg(args, ClCharT *);
            nbytes = vsnprintf((ClCharT*)pRecord, recSize - 1, pFmtStr, args);
            if(nbytes < 0) nbytes = 0;
        }
    }
    else
    {
        CL_LOG_DEBUG_VERBOSE(("endian: %u", endian));
        memcpy(pRecord, &endian, sizeof(ClUint8T));
        pRecord += sizeof(ClUint8T);

        CL_LOG_DEBUG_VERBOSE(("severity: %u", severity));
        memcpy(pRecord, &severity, sizeof(ClLogSeverityT));
        pRecord += sizeof(ClLogSeverityT);

        CL_LOG_DEBUG_VERBOSE(("streamId: %hu", streamId));
        memcpy(pRecord, &streamId, sizeof(ClUint16T));
        pRecord += sizeof(streamId);

        CL_LOG_DEBUG_VERBOSE(("clientId: %u", clientId));
        memcpy(pRecord, &clientId, sizeof(ClUint32T));
        pRecord += sizeof(ClUint32T);

        CL_LOG_DEBUG_VERBOSE(("serviceId: %hu", serviceId));
        memcpy(pRecord, &serviceId, sizeof(ClUint16T));
        pRecord += sizeof(ClUint16T);

        CL_LOG_DEBUG_VERBOSE(("msgId: %hu", msgId));
        memcpy(pRecord, &msgId, sizeof(ClUint16T));
        pRecord += sizeof(ClUint16T);

        rc = clOsalNanoTimeGet_L(&timeStamp);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogTimeOfDayGet(): rc[0x %x]", rc));
            return rc;
        }
        memcpy(pRecord, &timeStamp, sizeof(ClTimeT));
        pRecord += sizeof(ClTimeT);
        CL_LOG_DEBUG_VERBOSE(("timeStamp: %lld", timeStamp));
        
        memcpy(pRecord, &sequenceNum, sizeof(sequenceNum));
        pRecord += sizeof(sequenceNum);
        CL_LOG_DEBUG_VERBOSE(("sequenceNum: %lld", sequenceNum));

        rc = clLogClientMsgArgCopy(msgId, args, recSize - (pRecord - pRecStart), pRecord);
    }

    CL_LOG_DEBUG_TRACE(("Exit: rc: [Ox %x]", rc));
    return rc;

#undef __UPDATE_REC_SIZE
}

ClRcT
clLogClientMsgWrite(ClLogSeverityT     severity,
                    ClUint16T          streamId,
                    ClUint16T          serviceId,
                    ClUint16T          msgId,
                    ClUint32T          clientId,
                    va_list            args,
                    ClUint32T          recSize,
                    ClUint8T           *pRecord)
{
    return clLogClientMsgWriteWithHeader(severity, streamId, serviceId, msgId, clientId, 0, NULL, 
                                         args, recSize, pRecord);
}

ClRcT
clLogClntStreamCloseLocked(ClLogClntEoDataT *pClntEoEntry,
                           ClCntNodeHandleT    hClntStreamNode,
                           ClLogStreamHandleT  hStream,
                           ClBoolT             notifySvr)
{
    ClRcT                 rc            = CL_OK;
    ClLogStreamKeyT       *pUserKey     = NULL;
    ClLogClntStreamDataT  *pStreamData  = NULL;
    ClUint32T             nBits         = 0;
    ClLogStreamKeyT       key           = {{0}};
    ClLogStreamScopeT     scope         = CL_LOG_STREAM_LOCAL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserKeyGet(pClntEoEntry->hClntStreamTable, hClntStreamNode,
                             (ClCntKeyHandleT *) &pUserKey);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]", rc));
        return rc;
    }

    key = *pUserKey;
    rc = clCntDataForKeyGet(pClntEoEntry->hClntStreamTable,
                            (ClCntKeyHandleT) pUserKey,
                            (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeForKeyGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamScopeGet(&(key.streamScopeNode), &scope);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clBitmapBitClear(pStreamData->hStreamBitmap, CL_HDL_IDX(hStream));
    if( CL_OK != rc)
    {
        return rc;
    }

    rc = clBitmapNumBitsSet(pStreamData->hStreamBitmap, &nBits);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapNumBitsSet(): rc[0x %x]", rc));
        return rc;
    }

    if( 0 == nBits )
    {
        CL_LOG_DEBUG_VERBOSE(("Deleting the node for: %.*s",
            key.streamScopeNode.length, key.streamScopeNode.value));
        rc = clCntNodeDelete(pClntEoEntry->hClntStreamTable, hClntStreamNode);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
            return rc;
        }
    }

    /* Async without response at-most-once */
    if( CL_TRUE == notifySvr )
    {
        CL_LOG_DEBUG_VERBOSE(("To server to close: %.*s",
            key.streamScopeNode.length, key.streamScopeNode.value));

        rc = VDECL_VER(clLogSvrStreamCloseClientAsync, 4, 0, 0)(pClntEoEntry->hClntIdl,
                                            &(key.streamName),
                                            scope,
                                            &(key.streamScopeNode),
                                            pClntEoEntry->compId,
                                            NULL,
                                            NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrStreamCloseClientAsync, 4, 0, 0)(); rc[0x %x]"
                                , rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamClose(ClCntNodeHandleT    hClntStreamNode,
                     ClLogStreamHandleT  hStream,
                     ClBoolT             notifySvr)
{
    ClRcT                 rc            = CL_OK;
    ClLogClntEoDataT      *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&(pClntEoEntry->clntStreamTblLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogClntStreamCloseLocked(pClntEoEntry, hClntStreamNode, hStream, notifySvr);

    clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

