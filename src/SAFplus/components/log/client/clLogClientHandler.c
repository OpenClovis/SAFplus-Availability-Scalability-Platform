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

#include <clCpmExtApi.h>
#include <clIocServices.h>
#include <clIocErrors.h>
#include <ipi/clIocIpi.h>
#include <clLogClientHandler.h>
#include <clLogClientHandle.h>
#include <LogPortStreamOwnerClient.h>

static ClRcT
clLogClntHdlrsStateUpdate(ClBitmapHandleT  hBitmap,
                          ClUint32T        bitNum, 
                          void             *pCookie);

static ClRcT
clLogClntHandlersNotify(ClBitmapHandleT  hBitmap,
                        ClUint32T        bitNum, 
                        void             *pCookie);
static ClRcT
clLogClntHdlrNodeChkNDestroy(ClLogClntEoDataT        *pClntEoEntry, 
                             ClLogClntHandlerNodeT   *pClntHdlrData, 
                             ClIocMulticastAddressT  streamMcastAddr);
static ClRcT
clLogClntHdlrPersistCntDecrement(ClLogClntEoDataT        *pClntEoEntry, 
                                 ClIocMulticastAddressT  streamMcastAddr);

ClInt32T
clLogClntFlushKeyCompare(ClCntKeyHandleT  key1, ClCntKeyHandleT  key2)
{
    ClLogClntFlushKeyT  *pKey1 = (ClLogClntFlushKeyT *) key1;
    ClLogClntFlushKeyT  *pKey2 = (ClLogClntFlushKeyT *) key2;

    CL_LOG_DEBUG_TRACE(("Enter-Exit"));

    return (pKey2->seqNum - pKey1->seqNum);

}

ClUint32T
clLogClntFlushKeyHashFn(ClCntKeyHandleT  key)
{
    return (((ClLogClntFlushKeyT *)key)->hash); 
}

void
clLogClntFlushKeyDeleteCb(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree((ClLogClntFlushKeyT *) userKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClInt32T
clLogMcastAddrCompare(ClCntKeyHandleT  key1, ClCntKeyHandleT  key2)
{
    return (*(ClIocMulticastAddressT *) key2 - 
            *(ClIocMulticastAddressT *) key1);
}

ClUint32T
clLogStreamHandlerHashFn(ClCntKeyHandleT  key)
{
    ClLogClntEoDataT  *pClntEoEntry    = NULL;
    ClRcT             rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return ~0;
    }

    return ( (ClUint32T) ((*(ClIocMulticastAddressT *)key) % pClntEoEntry->maxStreams) );
}

void
clLogStreamHandlerDeleteCb(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    ClLogClntHandlerNodeT  *pData = (ClLogClntHandlerNodeT *) userData;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_CLEANUP(clBitmapDestroy(pData->hStreamBitmap), CL_OK);
    clHeapFree(pData);
    clHeapFree((ClIocMulticastAddressT *) userKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}
ClRcT
clLogClntHandlerRegister(ClLogHandleT              hLog,
                         SaNameT                   *pStreamName,
                         ClLogStreamScopeT         streamScope,
                         SaNameT                   *pNodeName,
                         ClLogStreamHandlerFlagsT  handlerFlags,
                         ClIocMulticastAddressT    streamMcastAddr,
                         ClLogStreamHandleT        *phStream)
{
    ClRcT                  rc               = CL_OK;
    ClBoolT                addedEntry       = CL_FALSE;
    ClBoolT                addedTable       = CL_FALSE;
    ClCntNodeHandleT       hClntHandlerNode = CL_HANDLE_INVALID_VALUE;
    ClLogClntHandlerNodeT  *pUserData       = NULL;
    ClLogClntEoDataT       *pClntEoEntry    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("trying to take lock  %p",
                (ClPtrT) &pClntEoEntry->streamHandlerTblLock));
    rc = clOsalMutexLock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;            
    }
    CL_LOG_DEBUG_TRACE(("Got a lock  %p",
                (ClPtrT) &pClntEoEntry->streamHandlerTblLock));

    if( CL_HANDLE_INVALID_VALUE == pClntEoEntry->hStreamHandlerTable )
    {
        rc = clCntHashtblCreate(pClntEoEntry->maxStreams, 
                                clLogMcastAddrCompare,
                                clLogStreamHandlerHashFn,
                                clLogStreamHandlerDeleteCb,
                                clLogStreamHandlerDeleteCb,
                                CL_CNT_UNIQUE_KEY,
                                &(pClntEoEntry->hStreamHandlerTable));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntHashtblCreate(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                           CL_OK);
            return rc;
        }
        addedTable = CL_TRUE;
    }

    rc = clLogClntHandlerEntryGet(pClntEoEntry, pStreamName, streamScope,
                                  pNodeName, streamMcastAddr,
                                  &hClntHandlerNode, &addedEntry);
    if( CL_OK != rc)
    {
        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hStreamHandlerTable),
                           CL_OK);
            pClntEoEntry->hStreamHandlerTable = CL_HANDLE_INVALID_VALUE;
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
        return rc;
    }
    
    rc = clLogClntHandleHandlerHandleCreate(hLog, hClntHandlerNode, 
                                            streamMcastAddr, handlerFlags, 
                                            phStream);
    if( CL_OK != rc)
    {
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hStreamHandlerTable,
                                           hClntHandlerNode), CL_OK);
        }
        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hStreamHandlerTable),
                           CL_OK);
            pClntEoEntry->hStreamHandlerTable = CL_HANDLE_INVALID_VALUE;
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
        return rc;
    }

    rc = clCntNodeUserDataGet(pClntEoEntry->hStreamHandlerTable,
                              hClntHandlerNode,
                              (ClCntDataHandleT *) &pUserData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB,
                                             *phStream), CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hStreamHandlerTable,
                                           hClntHandlerNode), CL_OK);
        }
        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hStreamHandlerTable),
                           CL_OK);
            pClntEoEntry->hStreamHandlerTable = CL_HANDLE_INVALID_VALUE;
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
        return rc;
    }
    
    rc = clBitmapBitSet(pUserData->hStreamBitmap, *phStream);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB,
                                       *phStream), CL_OK);
        *phStream = CL_HANDLE_INVALID_VALUE;
        if( CL_TRUE == addedEntry )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pClntEoEntry->hStreamHandlerTable,
                                           hClntHandlerNode), CL_OK);
        }

        if( CL_TRUE == addedTable )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hStreamHandlerTable),
                           CL_OK);
            pClntEoEntry->hStreamHandlerTable = CL_HANDLE_INVALID_VALUE;
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Released the lock %p",
                (ClPtrT) &pClntEoEntry->streamHandlerTblLock));
    rc = clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock(): rc[0x %x]", rc));
        return rc;            
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/* - clLogClntHandlerEntryGet()
 * - Searches for the handler entry in the client table,
 * - creates if it doesn't exist. 
 */
ClRcT
clLogClntHandlerEntryGet(ClLogClntEoDataT        *pClntEoEntry,
                         SaNameT                 *pStreamName,
                         ClLogStreamScopeT       streamScope, 
                         SaNameT                 *pNodeName,
                         ClIocMulticastAddressT  mcastAddr,
                         ClCntNodeHandleT        *phHandlerNode,
                         ClBoolT                 *pAddedEntry)
{
    ClRcT  rc = CL_OK;
    ClIocMulticastAddressT  *pKey = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pAddedEntry = CL_FALSE;
    
    pKey = (ClIocMulticastAddressT*)clHeapCalloc(1, sizeof(ClIocMulticastAddressT));
    if( NULL == pKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    *pKey = mcastAddr;
    
    rc = clCntNodeFind(pClntEoEntry->hStreamHandlerTable,
                       (ClCntKeyHandleT) pKey, phHandlerNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clLogClntHandlerEntryAdd(pClntEoEntry->hStreamHandlerTable,
                                      pKey, pStreamName, streamScope,
                                      pNodeName, phHandlerNode);
        if( CL_OK != rc )
        {
            return rc;
        }
        *pAddedEntry = CL_TRUE;
        return rc;
    }
    clHeapFree(pKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHandlerEntryAdd(ClCntHandleT            hStreamHandlerTable,
                         ClIocMulticastAddressT  *pStreamMcastAddr,
                         SaNameT                 *pStreamName,
                         ClLogStreamScopeT       streamScope,
                         SaNameT                 *pNodeName,
                         ClCntNodeHandleT        *phHandlerNode)
{
    ClRcT                   rc            = CL_OK;
    ClLogClntHandlerNodeT   *pHandlerData = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    pHandlerData = (ClLogClntHandlerNodeT*)clHeapCalloc(1, sizeof(ClLogClntHandlerNodeT));
    if( NULL == pHandlerData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clBitmapCreate(&(pHandlerData->hStreamBitmap), 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        clHeapFree(pHandlerData);
        return rc;
    }
    pHandlerData->streamName = *pStreamName;
    pHandlerData->streamScope = streamScope;
    if( CL_LOG_STREAM_LOCAL == streamScope )
    {
        pHandlerData->nodeName = *pNodeName;
    }
    pHandlerData->status        = CL_LOG_CLNTHDLR_INACTIVE;
    pHandlerData->persistingCnt = 0;
    
    rc = clCntNodeAddAndNodeGet(hStreamHandlerTable,
                                (ClCntKeyHandleT) pStreamMcastAddr,  
                                (ClCntDataHandleT) pHandlerData,
                                NULL, phHandlerNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pHandlerData->hStreamBitmap), CL_OK);
        clHeapFree(pHandlerData);
        return rc;
    }

    rc = clLogStreamMcastRegister(pStreamMcastAddr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamMcastRegister(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntNodeDelete(hStreamHandlerTable, *phHandlerNode),
                        CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamMcastDeregister(ClIocMulticastAddressT  streamMcastAddr)
{
    ClRcT                rc             = CL_OK;
    ClIocMcastUserInfoT  multiCastInfo  = {0};
    ClIocPortT           myPort        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
        
    rc = clEoMyEoIocPortGet(&myPort);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoIocPortGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("PortNum of Application: %d", myPort));
    multiCastInfo.mcastAddr                = streamMcastAddr;
    multiCastInfo.physicalAddr.nodeAddress = clIocLocalAddressGet();
    multiCastInfo.physicalAddr.portId      = myPort;

    rc = clIocMulticastDeregister(&multiCastInfo);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("Multicast deregister failed for %llx",
                    streamMcastAddr));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamMcastRegister(ClIocMulticastAddressT  *pStreamMcastAddr)
{
    ClRcT             rc          = CL_OK;
    ClIocMcastUserInfoT  multiCastInfo = {0};
    ClIocPortT        myPort      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
        
    rc = clEoMyEoIocPortGet(&myPort);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoIocPortGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("PortNum of Application: %d", myPort));
    multiCastInfo.mcastAddr                = *pStreamMcastAddr;
    multiCastInfo.physicalAddr.nodeAddress = clIocLocalAddressGet();
    multiCastInfo.physicalAddr.portId      = myPort;

    rc = clIocMulticastRegister(&multiCastInfo);
        if( CL_OK != rc )
        {
        CL_LOG_DEBUG_ERROR(("clIocMulticastRegister(): rc[0x %x]", rc));
            return rc;
        }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHdlrAllHandlesClose(ClLogClntEoDataT        *pClntEoEntry, 
                             ClLogClntHandlerNodeT   *pClntHdlrData,
                             ClIocMulticastAddressT  streamMcastAddr)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBitmapWalkUnlocked(pClntHdlrData->hStreamBitmap,
                              clLogClntHdlrsStateUpdate, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapWalk(): rc[0x %x]", rc));
    }
    CL_LOG_CLEANUP(clLogStreamMcastDeregister(streamMcastAddr), CL_OK);
    CL_LOG_CLEANUP(clLogClntHdlrNodeChkNDestroy(pClntEoEntry, pClntHdlrData,
                                                streamMcastAddr), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogClntFileHdlrDataReceive, 4, 0, 0)(
                             ClIocMulticastAddressT  streamMcastAddr, 
                             ClUint32T               seqNum, 
                             ClIocNodeAddressT       srcAddr,
                             ClHandleT               hFlusherCookie,
                             ClUint32T               numRecords,
                             ClUint32T               buffLen,
                             ClUint8T                *pRecords)
{
    ClRcT                  rc            = CL_OK;
    ClLogClntEoDataT       *pClntEoEntry = NULL;
    ClLogClntHandlerNodeT  *pStreamNode  = NULL;
    ClLogClntFlushKeyT      key           = {0};
    ClCntNodeHandleT        hFlushNode    = CL_HANDLE_INVALID_VALUE;
    ClLogClntHdlrRecvDataT  recvData      = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Trying take lock: %p ", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));
    rc = clOsalMutexLock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;            
    }
    CL_LOG_DEBUG_TRACE(("Got a lock : %p", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));

    rc = clCntDataForKeyGet(pClntEoEntry->hStreamHandlerTable, 
                            (ClCntKeyHandleT) &streamMcastAddr,
                            (ClCntDataHandleT *) &pStreamNode);
    if( CL_OK != rc)
    {			
        CL_LOG_DEBUG_TRACE(("Entry for McastAddr %llx does not exist", streamMcastAddr));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                             CL_OK);
        return rc;
    }
    if( (CL_IOC_RESERVED_ADDRESS == srcAddr) && (0 == seqNum) ) 
    {
        /*
         * This is a explicit close notification from streamOwner for
         * streamClose, so we are making all handlers handler as inactive,
         * so that no more record delivary callback will be called on those 
         * handles.
         */
         rc = clLogClntHdlrAllHandlesClose(pClntEoEntry, pStreamNode, 
                                           streamMcastAddr);
         CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                             CL_OK);
         return rc;
    }
    if( CL_LOG_CLNTHDLR_DEREGISTERED == pStreamNode->status )
    {
        CL_LOG_DEBUG_ERROR(("StreamClosed already, no more persistence"));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                             CL_OK);
        return CL_OK;
    }
    
    ++pStreamNode->persistingCnt;
    pStreamNode->status = CL_LOG_CLNTHDLR_PERSISTING;
    /*
     * Removing the Node&Add and getting the nodeHandle passing to the
     * fileowner, after receiving getting the node from container, it seems,
     * its unneccessary here, for each owner allocate memory and give the
     * pointer as seqnum to him, once you recieve the pointer deallocates the
     * memory
     */
    key.srcAddr      = srcAddr;
    key.seqNum       = seqNum;
    key.hFlushCookie = hFlusherCookie;
    key.hash         = srcAddr % CL_LOG_MAX_FLUSHERS;

    /*
     * Cookie to the fileOwner handles walk 
     */
    recvData.hFlushNode = hFlushNode;
    recvData.numRecords = numRecords;
    recvData.pRecords   = pRecords;
    recvData.key        = key;

    CL_LOG_DEBUG_TRACE(("Released lock : %p", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                   CL_OK);

    /*
     * Walk thru the list of streamHandles, call the appropriate Record
     * delivary callbacks
     */
    rc = clBitmapWalkUnlocked(pStreamNode->hStreamBitmap, clLogClntHandlersNotify,
                              (void *) &recvData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapWalk(): rc[0x %x]", rc));
    }
    
    CL_LOG_CLEANUP(clLogClntHdlrPersistCntDecrement(pClntEoEntry, streamMcastAddr),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogClntHdlrsStateUpdate(ClBitmapHandleT  hBitmap,
                          ClUint32T        bitNum, 
                          void             *pCookie)
{
    ClRcT                          rc             = CL_OK;
    ClLogClntEoDataT               *pClntEoEntry  = NULL;
    ClLogStreamHandlerHandleDataT  *pData         = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, bitNum,
                          (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }
    pData->status = CL_FALSE;

    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogClntHandlersNotify(ClBitmapHandleT  hBitmap,
                        ClUint32T        bitNum, 
                        void             *pCookie)
{
    ClRcT                          rc             = CL_OK;
    ClLogClntEoDataT               *pClntEoEntry  = NULL;
    ClLogStreamHandlerHandleDataT  *pData         = NULL;
    ClLogInitHandleDataT           *pInitData     = NULL;
    ClLogClntHdlrRecvDataT         *pRecvData     = (ClLogClntHdlrRecvDataT*) pCookie;
    ClLogClntFlushKeyT             *pKey          = NULL; 

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, bitNum,
                          (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, pData->hLog,
                          (void **) (&pInitData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                       CL_OK);
        return rc;
    }
    if( CL_LOG_HANDLER_WILL_ACK == pData->handlerFlag )
    {
        pKey = (ClLogClntFlushKeyT*) clHeapCalloc(1, sizeof(ClLogClntFlushKeyT));
        if( NULL == pKey )
        {
            CL_LOG_DEBUG_ERROR(("Allocation failed rc[0x %x]", rc));
            CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                    CL_OK);
            return rc;
        }
        *pKey = pRecvData->key;
    }

    if( (NULL != pInitData->pCallbacks) && 
        (NULL != pInitData->pCallbacks->clLogRecordDeliveryCb) )
    {
        CL_LOG_DEBUG_TRACE(("numRecords: %u seqNum : %p",
                    pRecvData->numRecords, 
		    (ClPtrT) pRecvData->hFlushNode));
                        
        pInitData->pCallbacks->clLogRecordDeliveryCb( 
                                            (ClLogStreamHandleT) bitNum,
                                            (ClUint64T)(ClWordT)pKey,
                                            pRecvData->numRecords, 
                                            pRecvData->pRecords);
    }
    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, pData->hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                       CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHandlerDataForHandleGet(ClLogClntEoDataT          *pClntEoEntry,
                                 ClLogStreamHandleT        hStream,
                                 ClLogHandleT              *phLog,
                                 ClLogStreamHandlerFlagsT  *pHandlerFlags,
                                 ClCntNodeHandleT          *phHandlerNode,
                                 ClIocMulticastAddressT    *pStreamMcastAddr)
{
    ClRcT                          rc     = CL_OK;
    ClLogStreamHandlerHandleDataT  *pData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream,
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;            
    }

    if( CL_LOG_HANDLER_HANDLE != pData->type )
    {
        CL_LOG_DEBUG_ERROR(("Passed handle has wrong type"));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                       CL_OK);
        return CL_LOG_RC(CL_ERR_INVALID_HANDLE);
    }
    *phLog            = pData->hLog;
    *pHandlerFlags    = pData->handlerFlag;
    *phHandlerNode    = pData->hClntHandlerNode;
    *pStreamMcastAddr = pData->streamMcastAddr;

    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                       CL_OK);
    if( CL_FALSE == pData->status )
    {
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHandlerDeregister(ClLogStreamHandleT  hStream,
                           ClLogHandleT        *phLog)
{
    ClRcT                     rc              = CL_OK;
    ClUint32T                 nBits           = 0;
    ClIdlHandleT              hClntIdl        = {0};
    SaNameT                   streamName      = {0};
    ClLogStreamScopeT         streamScope     = CL_LOG_STREAM_LOCAL;
    SaNameT                   nodeName        = {0};
    ClLogStreamHandlerFlagsT  handlerFlag     = 0;
    ClCntNodeHandleT          hHandlerNode    = CL_HANDLE_INVALID_VALUE;
    ClIocAddressT             destAddr        = {{0}};
    ClLogClntEoDataT          *pClntEoEntry   = NULL;
    ClLogClntHandlerNodeT     *hHandler       = NULL;
    ClIocMulticastAddressT    streamMcastAddr = 0;
    ClCpmSlotInfoT            slotInfo        = {0};


    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntHandlerDataForHandleGet(pClntEoEntry, hStream, phLog, 
                                          &handlerFlag, &hHandlerNode,
                                          &streamMcastAddr);
    if( CL_LOG_RC(CL_ERR_NOT_EXIST) == rc )
    {
        /*
         * Multicast has been closed aleady by DataReceive function.
         * it invoked the close function.Just destroy the handle.
         */
        CL_LOG_CLEANUP(clLogClntHandleHandlerHandleRemove(hStream), CL_OK);
        return CL_OK;
    }
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Acquired lock : %p", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));
    rc = clOsalMutexLock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;            
    }
    CL_LOG_DEBUG_TRACE(("Got a lock : %p", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));

    rc = clCntDataForKeyGet(pClntEoEntry->hStreamHandlerTable,
                            (ClCntKeyHandleT) &streamMcastAddr,
                            (ClCntDataHandleT *) &hHandler);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                CL_OK);
        return rc;
    }

    streamName   = hHandler->streamName;
    streamScope  = hHandler->streamScope;
    nodeName     = hHandler->nodeName;

    rc = clBitmapBitClear(hHandler->hStreamBitmap, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                CL_OK);
        return rc;
    }

    rc = clBitmapNumBitsSet(hHandler->hStreamBitmap, &nBits);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBitmapBitSet(hHandler->hStreamBitmap, hStream),
                CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                CL_OK);
        return rc;
    }
    if( 0 == nBits )
    {
        CL_LOG_CLEANUP(clLogStreamMcastDeregister(streamMcastAddr), CL_OK);
        CL_LOG_CLEANUP(clLogClntHdlrNodeChkNDestroy(pClntEoEntry, hHandler,
                                                    streamMcastAddr), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Released lock : %p", (ClPtrT)
                &pClntEoEntry->streamHandlerTblLock));
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock), CL_OK);

    CL_LOG_CLEANUP(clLogClntHandleHandlerHandleRemove(hStream), CL_OK);

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        nodeName.length = strlen(gStreamScopeGlobal);
        memcpy(nodeName.value, gStreamScopeGlobal, nodeName.length);

        rc = clLogMasterAddressGet(&destAddr);
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    else
    {
        ClBoolT getAddress = CL_TRUE;
        ClUint32T i = 0;
        /* Its a hack to avoid timed out while shutting down the node
         * by the time, COR will not availble , cpm to cor communication will
         * be timed out, so for perrennial steams, directly talk to local guy
         * as we knew about already 
         */
        for(i = 0; i < nStdStream; i++ ) 
        {
            if( !strncmp((const ClCharT *)stdStreamList[i].streamName.value,
                        (const ClCharT *)streamName.value, streamName.length) )
            {
                destAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet(); 
                destAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
                getAddress = CL_FALSE; 
                break;
            }
        }
        if( getAddress )
        {
            slotInfo.nodeName.length = nodeName.length;
            memcpy(slotInfo.nodeName.value, nodeName.value, nodeName.length);

            rc = clCpmSlotGet(CL_CPM_NODENAME, &slotInfo);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clCpmSlotGet(): rc[0x %x]", rc));
                return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
            }
            destAddr.iocPhyAddress.nodeAddress = slotInfo.nodeIocAddress;
            destAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
        }
    }

    rc = clLogClntIdlHandleInitialize(destAddr, &hClntIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = VDECL_VER(clLogStreamOwnerHandlerDeregisterClientSync, 4, 0, 0)(hClntIdl, &streamName, streamScope,
                                           &nodeName, handlerFlag,
                                           clIocLocalAddressGet(), pClntEoEntry->compId);
    if( (CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)) )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogStreamOwnerHandlerDeregister, 4, 0, 0)(): rc[0x %x]",
                    rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);
        return rc;
    }
    rc = CL_OK;

    CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntHdlrPersistCntDecrement(ClLogClntEoDataT        *pClntEoEntry, 
                                 ClIocMulticastAddressT  streamMcastAddr)
{
    ClRcT                  rc             = CL_OK;
    ClLogClntHandlerNodeT  *pClntHdlrData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(); rc[0x %x]", rc));
        return rc;
    }

    rc = clCntDataForKeyGet(pClntEoEntry->hStreamHandlerTable, 
                           (ClCntKeyHandleT) &streamMcastAddr, 
                           (ClCntDataHandleT *) &pClntHdlrData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc[0x %x]", rc));
        return rc;
    }
    --pClntHdlrData->persistingCnt;

    if( CL_LOG_CLNTHDLR_DEREGISTERED == pClntHdlrData->status )
    {
        clLogClntHdlrNodeChkNDestroy(pClntEoEntry, pClntHdlrData, 
                                     streamMcastAddr);
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
            CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
    
static ClRcT
clLogClntHdlrNodeChkNDestroy(ClLogClntEoDataT        *pClntEoEntry, 
                             ClLogClntHandlerNodeT   *pClntHdlrData, 
                             ClIocMulticastAddressT  streamMcastAddr)
{
    ClRcT      rc   = CL_OK;
    ClUint32T  size = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_CLNTHDLR_DEREGISTERED != pClntHdlrData->status )
    {
        pClntHdlrData->status = CL_LOG_CLNTHDLR_DEREGISTERED;
    }

    CL_LOG_DEBUG_TRACE(("Persisting cnt: %d", pClntHdlrData->persistingCnt));
    if( 0 == pClntHdlrData->persistingCnt )
    {
        rc = clCntAllNodesForKeyDelete(pClntEoEntry->hStreamHandlerTable,
                                       (ClCntDataHandleT) &streamMcastAddr);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]\n", rc));
            return rc;
        }
        rc = clCntSizeGet(pClntEoEntry->hStreamHandlerTable, &size);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]\n", rc));
            return rc;
        }

        if( 0 == size )
        {
            CL_LOG_CLEANUP(clCntDelete(pClntEoEntry->hStreamHandlerTable), CL_OK);
            pClntEoEntry->hStreamHandlerTable = CL_HANDLE_INVALID_VALUE;
        }
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
