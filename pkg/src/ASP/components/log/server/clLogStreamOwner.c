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
#include <stdlib.h>

#include <clCkptExtApi.h>
#include <clBitmapApi.h>
#include <clOsalErrors.h>

#include <clLogErrors.h>
#include <clLogCommon.h>
#include <clLogSvrCommon.h>
#include <clLogFileEvt.h>
#include <clLogStreamOwner.h>
#include <clLogStreamOwnerCkpt.h>

#include <LogServer.h>
#include <LogPortMasterClient.h>
#include <LogPortStreamOwnerServer.h>
#include <xdrClLogCompKeyT.h>

extern const ClCharT soSecPrefix[];

static 
ClBoolT clLogMasterLocation(ClStringT  *fileLocation)
{
    ClCharT nodeStr[CL_MAX_NAME_LENGTH] = {0};
    ClBoolT isMaster = CL_FALSE;

    sscanf(fileLocation->pValue, "%[^:]", nodeStr);
    isMaster =  (nodeStr[0] == '*' && nodeStr[1] == '\0') ? CL_TRUE: CL_FALSE;
    return  isMaster;
}

#define  CL_LOG_PERRINNIAL_LOCAL_STREAM(compId, scope, fileLocation)\
     ((scope == CL_LOG_STREAM_LOCAL) && (clIocLocalAddressGet() ==\
            clLogFileOwnerAddressFetch(&fileLocation)) &&\
      (!clLogMasterLocation(&fileLocation)) && (compId ==\
            CL_LOG_DEFAULT_COMPID))

static ClRcT
clLogStreamOwnerEntryDelete(ClLogSOEoDataT     *pSoEoEntry,
                            ClLogStreamScopeT  streamScope,
                            ClCntNodeHandleT   hStreamOwnerNode);
/*************************CompTable Related Callback Functions ******************/

static ClUint32T
clLogCompTblHashFn(ClCntKeyHandleT key)
{
    return ((ClLogCompKeyT *) key)->hash;
}    

static ClInt32T
clLogCompKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClLogCompKeyT  *pCompKey1 = (ClLogCompKeyT *) key1;
    ClLogCompKeyT  *pCompKey2 = (ClLogCompKeyT *) key2;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    if( pCompKey1->nodeAddr == pCompKey2->nodeAddr )
    {
        if( pCompKey1->compId == pCompKey2->compId )
        {
            return 0;
        }    
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return 1;
}    

static void
clLogCompTblDeleteCb(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    ClLogCompKeyT    *pCompKey  = (ClLogCompKeyT *) key;
    ClLogSOCompDataT *pCompData = (ClLogSOCompDataT *) data;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
         
    CL_LOG_DEBUG_TRACE(("its getting deleted ************** %d  %x \n",
            pCompKey->nodeAddr, pCompKey->compId));
    clHeapFree(pCompData);
    clHeapFree(pCompKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}    

/*****************StreamTable Related Callback Functions *************/
static void
clLogStreamOwnerDeleteCb(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    ClLogStreamKeyT        *pKey             = (ClLogStreamKeyT *) key;
    ClLogStreamOwnerDataT  *pStreamOwnerData = (ClLogStreamOwnerDataT *) data;
    ClRcT                  rc                = CL_OK;
   
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    if( CL_FALSE == pStreamOwnerData->condDelete )
    {
        rc = clOsalCondDestroy_L(&pStreamOwnerData->nodeCond); 
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalCondDelete(): rc[0x %x]", rc));
            return ;
        }
    }
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pStreamOwnerData->nodeLock), CL_OK);
    
    clLogStreamOwnerFilterFinalize(&pStreamOwnerData->streamFilter);
    
    CL_LOG_CLEANUP(clCntDelete(pStreamOwnerData->hCompTable), CL_OK);
    pStreamOwnerData->hCompTable = CL_HANDLE_INVALID_VALUE;
    
    clHeapFree(pStreamOwnerData->streamAttr.fileName.pValue);
    clHeapFree(pStreamOwnerData->streamAttr.fileLocation.pValue);

    clHeapFree(pStreamOwnerData);
    clHeapFree(pKey);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}    

/********************************* STREAMOPEN **********************************/
/*
 *
 */
ClRcT
clLogStreamOwnerOpenCleanup(ClLogSOCookieT  *pCookie)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClUint32T              tableSize         = 0;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clLogSOLock(pSoEoEntry, pCookie->scope);
    if( CL_OK != rc )
    {
        return rc;
    }    
    hStreamTable = (CL_LOG_STREAM_GLOBAL == pCookie->scope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeUserDataGet(hStreamTable, pCookie->hStreamNode,
                             (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        clLogSOUnlock(pSoEoEntry, pCookie->scope);
        return rc;
    }    
    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        /* nothing can be done, coz this itself is a cleanup function */
        clLogSOUnlock(pSoEoEntry, pCookie->scope);
        return rc;
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, pCookie->scope), CL_OK);

    rc = clLogStreamOwnerCompEntryDelete(pStreamOwnerData, pCookie->nodeAddr, 
                                         pCookie->compId, &tableSize);
    pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_UN_INIT;
    clOsalCondBroadcast(&pStreamOwnerData->nodeCond);

    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    if( 0 == tableSize )
    {
        CL_LOG_CLEANUP(clLogStreamOwnerEntryChkNDelete(pSoEoEntry,
                    pCookie->scope,
                    pCookie->hStreamNode), 
                CL_OK);
    }

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerMasterClose(ClNameT              *pStreamName,
                            ClNameT              *pStreamScopeNode,
                            ClLogStreamAttrIDLT  *pStreamAttr)
{
    ClRcT      rc           = CL_OK;
    ClStringT  fileName     = {0}; 
    ClStringT  fileLocation = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    fileName.length = pStreamAttr->fileName.length;
    fileName.pValue = clHeapCalloc(fileName.length, sizeof(ClCharT));
    if( NULL == fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    fileLocation.length = pStreamAttr->fileLocation.length;
    fileLocation.pValue = clHeapCalloc(fileLocation.length, sizeof(ClCharT));
    if( NULL == fileLocation.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        clHeapFree(fileName.pValue);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    
    rc = clLogStreamOwnerCloseMasterNotify(pStreamName, pStreamScopeNode, 
                                           &fileName, &fileLocation);

    clHeapFree(fileLocation.pValue);
    clHeapFree(fileName.pValue);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerStreamHdlrEntryDelete(ClLogStreamOwnerDataT     *pStreamOwnerData,
                                      ClIocNodeAddressT         nodeAddr,
                                      ClUint32T                 compId,
                                      ClLogStreamHandlerFlagsT  flags, 
                                      ClUint32T                 *pTableSize)
{
    ClRcT                  rc              = CL_OK;
    ClLogCompKeyT          compKey         = {0};
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSOCompDataT       *pData          = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoEntry->maxComponents;
    rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable,
                            (ClCntKeyHandleT) &compKey, 
                            (ClCntDataHandleT *) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForkeyGet(): rc[0x %x]", rc));
        return rc;
    }
    /* Decrement the refCount */
    if( CL_LOG_HANDLER_WILL_ACK == flags )
    {
        --pData->ackerCnt;
        --pStreamOwnerData->ackerCnt;
    }
    else
    {
        --pData->nonAckerCnt;
        --pStreamOwnerData->nonAckerCnt;
    }

    if( (0 == pData->refCount) && (0 == pData->ackerCnt) && 
        (0 == pData->nonAckerCnt)  )
    {
        rc = clCntAllNodesForKeyDelete(pStreamOwnerData->hCompTable,
                                       (ClCntKeyHandleT) &compKey);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]",
                        rc));
            return rc;
        }    
    }
    /* 
     *  Checking the size, and returning EMPTY,if none of the components are
     *  there 
     */
    rc = clCntSizeGet(pStreamOwnerData->hCompTable, pTableSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerCompEntryDelete()
 *  - Get the refCount of component.  
 *  - Decrement the refCount;
 *  - If refCount == 0, remove the component Entry.
 */
ClRcT
clLogStreamOwnerCompEntryDelete(ClLogStreamOwnerDataT  *pStreamOwnerData,
                                ClIocNodeAddressT      nodeAddr,
                                ClUint32T              compId,
                                ClUint32T              *pTableSize)
{
    ClRcT                  rc              = CL_OK;
    ClLogCompKeyT          compKey         = {0};
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSOCompDataT       *pData          = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoEntry->maxComponents;
    rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable,
                            (ClCntKeyHandleT) &compKey, 
                            (ClCntDataHandleT *) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForkeyGet(): rc[0x %x]", rc));
        return rc;
    }
    /* Decrement the refCount */
    --pData->refCount;
    --pStreamOwnerData->openCnt;

    if( (0 == pData->refCount) && (0 == pData->ackerCnt) && 
        (0 == pData->nonAckerCnt)  )
    {
        rc = clCntAllNodesForKeyDelete(pStreamOwnerData->hCompTable,
                                       (ClCntKeyHandleT) &compKey);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]",
                        rc));
            return rc;
        }    
    }
    /* 
     *  Checking the size, and returning EMPTY,if none of the components are
     *  there 
     */
    rc = clCntSizeGet(pStreamOwnerData->hCompTable, pTableSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerCkptInfoGet(ClCntHandleT      hStreamTable,
                            ClCntNodeHandleT  hStreamOwnerNode,
                            ClCkptSectionIdT  *pSecId,
                            ClUint32T         *pDsId)
{
    ClRcT                  rc                = CL_OK;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserKeyGet(hStreamTable, hStreamOwnerNode,
                             (ClCntKeyHandleT *) &pStreamKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clCntNodeUserDataGet(hStreamTable, hStreamOwnerNode, 
                              (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }

    if( !(memcmp(pStreamKey->streamScopeNode.value, gStreamScopeGlobal,
                 pStreamKey->streamScopeNode.length)) )
    {
        if( CL_FALSE == pStreamOwnerData->isNewStream )
        {
            ClUint32T  prefixLen = strlen(soSecPrefix);

            pSecId->idLen = pStreamKey->streamName.length + prefixLen;
            pSecId->id    = clHeapCalloc(pSecId->idLen, sizeof(ClCharT)); 
            if( NULL == pSecId->id )
            {
                CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
                return CL_LOG_RC(CL_ERR_NO_MEMORY);
            }    
            memcpy(pSecId->id, soSecPrefix, prefixLen);
            memcpy(pSecId->id + prefixLen, pStreamKey->streamName.value,
                   pStreamKey->streamName.length); 
        }
    }    
    else
    {
        if( CL_LOG_INVALID_DSID != pStreamOwnerData->dsId )
        {
            *pDsId = pStreamOwnerData->dsId;
        }
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/***************** SCOPE Related Functions ********************/
ClRcT
clLogSOLock(ClLogSOEoDataT     *pSoEoEntry, 
            ClLogStreamScopeT  streamScope)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        rc = clOsalMutexLock_L(&pSoEoEntry->gStreamTableLock);
    }    
    else
    {
        rc = clOsalMutexLock_L(&pSoEoEntry->lStreamTableLock);
    }    

    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clOsalMutexLock(): rc[0x %x]", rc));
    }    
            
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

ClRcT
clLogSOUnlock(ClLogSOEoDataT    *pSoEoEntry,
              ClLogStreamScopeT streamScope)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        rc = clOsalMutexUnlock_L(&pSoEoEntry->gStreamTableLock);
    }    
    else
    {
        rc = clOsalMutexUnlock_L(&pSoEoEntry->lStreamTableLock);
    }    

    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clOsalMutexUnlock(): rc[0x %x]", rc));
    }    
            
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

/********************************Close Functions *************************/
/*
 * Function - clLogStreamOwnerCloseMasterNotify()
 * - Inform the master about the closure of this stream.
 */

ClRcT
clLogStreamOwnerCloseMasterNotify(ClNameT    *pStreamName,
                                  ClNameT    *pStreamScopeNode, 
                                  ClStringT  *pFileName,
                                  ClStringT  *pFileLocation)
{
    ClRcT              rc          = CL_OK;
    ClIdlHandleT       hLogIdl     = CL_HANDLE_INVALID_VALUE;
    ClIocAddressT      masterAddr  = {{0}};
    ClLogStreamScopeT  streamScope = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterAddressGet(&masterAddr);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clLogIdlHandleInitialize(masterAddr, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamScopeGet(pStreamScopeNode, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = VDECL_VER(clLogMasterStreamCloseNotifyClientAsync, 4, 0, 0)(hLogIdl, pFileName, 
                                                 pFileLocation, pStreamName,
                                                 streamScope, pStreamScopeNode,
                                                 NULL, NULL);
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerEntryDelete()
 *  Just delete the node from the table
 */
static ClRcT
clLogStreamOwnerEntryDelete(ClLogSOEoDataT     *pSoEoEntry,
                            ClLogStreamScopeT  streamScope,
                            ClCntNodeHandleT   hStreamOwnerNode)
{
    ClRcT         rc           = CL_OK;    
    ClCntHandleT  hStreamTable = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    /* Delete callback will free the streamOwnerData and Key */
    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeDelete(hStreamTable, hStreamOwnerNode); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCntNodeDelete(): rc[0x %x]", rc));
    }
        
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerEntryCleanup()
 */
ClRcT
clLogStreamOwnerEntryCleanup(ClLogSOEoDataT         *pSoEoEntry, 
                             ClLogStreamOwnerDataT  *pStreamOwnerData,
                             ClLogStreamScopeT      streamScope, 
                             ClCntNodeHandleT       hStreamOwnerNode)
{
    ClRcT             rc                = CL_OK;
    ClCkptSectionIdT  secId             = {0};
    ClUint32T         dsId              = 0;
    ClCntHandleT      hStreamTable = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clLogStreamOwnerCkptInfoGet(hStreamTable, hStreamOwnerNode, 
                                     &secId, &dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clLogStreamOwnerCkptInfoGet(): rc[0x %x]", rc));
        return rc;
    }
    /*
     *  I am the last one, so peacefully remove this Entry
     *  Callback function will internally, cleanuping the entry
     */
    rc = clCntNodeDelete(hStreamTable, hStreamOwnerNode); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
        return rc;
    }    

    clLogStreamOwnerCkptDelete(pSoEoEntry, &secId, dsId);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    
/*
 * Function - clLogStreamOwnerEntryChkNDelete()
 * - Take streamTable lock. 
 * - Get the corresponding StreamOwnerData for streamOwnerNode.
 * - Decrement the refCount of compId in the compData.
 * - If refCount is 0, remove the compEntry.
 * - Check the table size,
 * - If size == 0, remove the node Entry.
 */
ClRcT
clLogStreamOwnerEntryChkNDelete(ClLogSOEoDataT     *pSoEoEntry,
                                ClLogStreamScopeT  streamScope,
                                ClCntNodeHandleT   hStreamOwnerNode)
{
    ClRcT                  rc                = CL_OK;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionIdT       secId             = {0};
    ClUint32T              dsId              = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSOLock(pSoEoEntry, streamScope);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clLogSOLock(): rc[0x %x]", rc));
        return rc;
    }    
    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeUserDataGet(hStreamTable, hStreamOwnerNode,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCntNodeUserDataGet(): rc[0x %x]", rc));
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }    
    rc = clLogStreamOwnerCkptInfoGet(hStreamTable, hStreamOwnerNode, 
                                     &secId, &dsId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clLogStreamOwnerCkptInfoGet(): rc[0x %x]", rc));
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clOsalMutexLock_L(): rc[0x %x]", rc));
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }    
    pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_UN_INIT;
    /*
     * None of componentEntries exist in the entry, so remove
     * Try to delete the condition varible, if it gives error,
     * so somebody is waiting on this varibale. so just return CL_OK
     */
    rc = clOsalCondDestroy_L(&pStreamOwnerData->nodeCond);
    if( CL_OSAL_ERR_CONDITION_DELETE == rc )
    {
        CL_LOG_DEBUG_TRACE(("clOsalCondDelete(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalCondSignal_L(&pStreamOwnerData->nodeCond), CL_OK);
        clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }
    else if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalCondDestroy_L(): rc[0x %x]", rc));
        clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }
    /* resetting the value */
    pStreamOwnerData->condDelete = CL_TRUE;
    clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
    /*
     *  I am the last one, so peacefully remove this Entry
     *  Callback function will internally, cleanuping the entry
     */
    rc = clCntNodeDelete(hStreamTable, hStreamOwnerNode); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
        clLogSOUnlock(pSoEoEntry, streamScope);
        return rc;
    }    

    clLogStreamOwnerCkptDelete(pSoEoEntry, &secId, dsId);
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerInfoCopy(ClLogStreamOwnerDataT   *pStreamOwnerData, 
                         ClStringT               *pFileName,
                         ClStringT               *pFileLocation,
                         ClIocMulticastAddressT  *pStreamMcastAddr,
                         ClUint16T               *pStreamId)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    pFileName->length = pStreamOwnerData->streamAttr.fileName.length;
    pFileName->pValue = clHeapCalloc(pFileName->length, sizeof(ClCharT));
    if( NULL == pFileName->pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    pFileLocation->length = pStreamOwnerData->streamAttr.fileLocation.length;
    pFileLocation->pValue = clHeapCalloc(pFileLocation->length, sizeof(ClCharT));
    if( NULL == pFileLocation->pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        clHeapFree(pFileName->pValue);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    memcpy(pFileName->pValue, pStreamOwnerData->streamAttr.fileName.pValue, 
           pFileName->length);
    memcpy(pFileLocation->pValue, 
           pStreamOwnerData->streamAttr.fileLocation.pValue, 
           pFileLocation->length);
    *pStreamMcastAddr = pStreamOwnerData->streamMcastAddr;
    *pStreamId        = pStreamOwnerData->streamId;

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerStreamClose()
 *   takes care of the following scenarios.
 * Test cases :-
 *  - t1. No Entry for this stream.
 *  - t2. No Component Entry
 *  - t3. Just decrement refCount of compId.
 *  - t4. Just delete the compId Entry.
 *  - t5. Last compId - Remove the Entry.
 *  - t6. Trying to remove the NodeEntry, some other guy waiting on that.
 */
ClRcT
VDECL_VER(clLogStreamOwnerStreamClose, 4, 0, 0)(
                            ClNameT            *pStreamName,
                            ClLogStreamScopeT  streamScope,
                            ClNameT            *pStreamScopeNode,
                            ClIocNodeAddressT  nodeAddr, 
                            ClUint32T          compId)
{
    ClRcT                   rc                 = CL_OK;
    ClLogSOEoDataT          *pSoEoEntry        = NULL;
    ClLogSvrCommonEoDataT   *pCommonEoEntry    = NULL;
    ClCntNodeHandleT        hStreamOwnerNode   = CL_HANDLE_INVALID_VALUE;
    ClIocMulticastAddressT  multiCastAddr      = 0;
    ClUint16T               streamId           = 0;
    ClCntHandleT            hStreamTable       = CL_HANDLE_INVALID_VALUE;
    ClLogStreamOwnerDataT   *pStreamOwnerData  = NULL;  
    ClLogStreamKeyT         *pStreamKey        = NULL;
    ClStringT               fileName           = {0};
    ClStringT               fileLocation       = {0};
    ClUint32T               tableSize          = 0;
    ClIocMulticastAddressT  streamMcastAddr    = 0;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }    

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        /* return for t1 */
        clLogSOUnlock(pSoEoEntry, streamScope);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }    
    rc = clCntNodeFind(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntNodeHandleT *) &hStreamOwnerNode);
    if( CL_OK != rc )
    {
        clLogSOUnlock(pSoEoEntry, streamScope);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }    

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        clLogSOUnlock(pSoEoEntry, streamScope);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clLogStreamOwnerCompEntryDelete(pStreamOwnerData, nodeAddr, 
                                         compId, &tableSize);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        clLogSOUnlock(pSoEoEntry, streamScope);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }

    rc = clLogStreamOwnerInfoCopy(pStreamOwnerData, &fileName, &fileLocation, 
                                  &multiCastAddr, &streamId);
    
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        clLogSOUnlock(pSoEoEntry, streamScope);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }    

    if( 0 == pStreamOwnerData->openCnt )
    {
        /* this will not get called */
        streamMcastAddr = pStreamOwnerData->streamMcastAddr;
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);

        CL_LOG_CLEANUP(clLogStreamOwnerEntryCleanup(pSoEoEntry, pStreamOwnerData, 
                                          streamScope, hStreamOwnerNode),
                       CL_OK);

        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

        CL_LOG_DEBUG_TRACE(("****rc from Stream Owner while streamClose: rc[0x%x]",
                    rc));

        /* Entry got removed, so notifying the master abt stream Deletion */
        CL_LOG_CLEANUP(clLogStreamOwnerCloseMasterNotify(pStreamName,
                                                         pStreamScopeNode, 
                                                         &fileName,
                                                         &fileLocation),
                                                         CL_OK);
        
        CL_LOG_CLEANUP(clLogStreamOwnerMulticastCloseNotify(streamMcastAddr),
                       CL_OK);

        /* Publish the event abt closure of Stream */
        if( ! CL_LOG_PERRINNIAL_LOCAL_STREAM(compId, streamScope, fileLocation))
        {
            CL_LOG_CLEANUP(clLogStreamCloseEvtPublish(pStreamName, streamScope, 
                        pStreamScopeNode), CL_OK);
        }

        clHeapFree(fileName.pValue);
        clHeapFree(fileLocation.pValue);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    /* Checkpoint node Info */
    clLogStreamOwnerCheckpoint(pSoEoEntry, streamScope, hStreamOwnerNode, 
                               pStreamKey);
    clHeapFree(fileName.pValue);
    clHeapFree(fileLocation.pValue);
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

/******************************* OPEN ********************************/
void 
clLogStreamOwnerFilterFinalize(ClLogFilterInfoT  *pFilter)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_BM_INVALID_BITMAP_HANDLE != pFilter->hCompIdMap )
    { 
        CL_LOG_CLEANUP(clBitmapDestroy(pFilter->hCompIdMap), CL_OK);
        pFilter->hCompIdMap = CL_BM_INVALID_BITMAP_HANDLE;
    }
    if( CL_BM_INVALID_BITMAP_HANDLE != pFilter->hMsgIdMap )
    {
        CL_LOG_CLEANUP(clBitmapDestroy(pFilter->hMsgIdMap), CL_OK);
        pFilter->hMsgIdMap = CL_BM_INVALID_BITMAP_HANDLE;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogStreamOwnerFilterInit(ClLogFilterInfoT  *pFilter)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pFilter->severityFilter = clLogDefaultStreamSeverityGet();
    rc = clBitmapCreate(&pFilter->hMsgIdMap, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBitmapCreate(&pFilter->hCompIdMap, CL_LOG_COMPID_CLASS);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pFilter->hMsgIdMap), CL_OK);
        pFilter->hMsgIdMap = CL_BM_INVALID_BITMAP_HANDLE;
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerEvtPublish(ClNameT                *pStreamName,
                           ClLogStreamScopeT      streamScope,
                           ClNameT                *pStreamScopeNode,
                           ClLogStreamOwnerDataT  *pStreamOwnerData)
{
    ClRcT                rc         = CL_OK;
    ClLogStreamInfoIDLT  streamInfo = {{0}}; 
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    streamInfo.streamName  = *pStreamName; 
    streamInfo.streamScope = streamScope;
    streamInfo.nodeName    = *pStreamScopeNode;
    streamInfo.streamId    = pStreamOwnerData->streamId;
    rc = clLogStreamAttributesCopy(&pStreamOwnerData->streamAttr,
                                   &streamInfo.streamAttr, CL_TRUE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamCreationEvtPublish(&streamInfo);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR((
                    "clLogStreamCreationEvtPublish() failed to publish open event for stream : [%.*s] rc[0x %x]",
                    pStreamName->length, pStreamName->value, rc));
    }

    clHeapFree(streamInfo.streamAttr.fileName.pValue);
    clHeapFree(streamInfo.streamAttr.fileLocation.pValue);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerEntryUpdate(ClLogSOEoDataT          *pSoEoEntry,
                            ClNameT                 *pStreamName,
                            ClLogStreamScopeT       streamScope,
                            ClNameT                 *pStreamScopeNode,
                            ClLogStreamOwnerDataT   *pStreamOwnerData,
                            ClLogStreamAttrIDLT     *pStreamAttr,
                            ClIocMulticastAddressT  multiCastAddr,
                            ClLogFilterT            *pFilter, 
                            ClUint16T               streamId,
                            ClUint32T               compId)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pStreamOwnerData->streamMcastAddr             = multiCastAddr;
    pStreamOwnerData->streamFilter.severityFilter = clLogDefaultStreamSeverityGet();
    pStreamOwnerData->streamId                    = streamId;
    pStreamOwnerData->nodeStatus                  = CL_LOG_NODE_STATUS_INIT;
    rc = clLogFilterFormatConvert(&pStreamOwnerData->streamFilter, pFilter);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogStreamAttributesCopy(pStreamAttr, 
                                   &pStreamOwnerData->streamAttr, CL_TRUE);
    if( CL_OK != rc )
    {
        clHeapFree(pFilter->pCompIdSet);
        clHeapFree(pFilter->pMsgIdSet);
        return rc;
    }    
    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
       rc = clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, pStreamName, 
                                             pStreamScopeNode,
                                             pStreamOwnerData);  
    }    
    else
    {
        rc = clLogStreamOwnerLocalCheckpoint(pSoEoEntry, pStreamName,
                                             pStreamScopeNode, 
                                             pStreamOwnerData);
    }    
    if( CL_OK != rc )
    {
        clHeapFree(pFilter->pCompIdSet);
        clHeapFree(pFilter->pMsgIdSet);
        return rc;
    }
    if( ! CL_LOG_PERRINNIAL_LOCAL_STREAM(compId, streamScope, pStreamAttr->fileLocation) )
    {
        rc = clLogStreamOwnerEvtPublish(pStreamName, streamScope, pStreamScopeNode, 
                pStreamOwnerData);
        if( CL_OK != rc )
        {
            clHeapFree(pFilter->pCompIdSet);
            clHeapFree(pFilter->pMsgIdSet);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerMAVGResponseProcess(ClNameT                 *pStreamName,
                                    ClNameT                 *pStreamScopeNode,
                                    ClLogStreamAttrIDLT     *pStreamAttr,
                                    ClIocMulticastAddressT  *pStreamMcastAddr,
                                    ClUint16T               *pStreamId,
                                    ClLogFilterT            *pFilter, 
                                    ClUint32T               *pAckerCnt,
                                    ClUint32T               *pNonAckerCnt, 
                                    ClRcT                   retCode,
                                    ClLogSOCookieT          *pCookie)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClLogStreamOpenFlagsT  openFlags         = 0;
    ClCntNodeHandleT       hStreamOwnerNode  = CL_HANDLE_INVALID_VALUE;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClUint16T              refCount          = 0;
    ClBoolT                entryAdd          = 0; 
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clLogSOLock(pSoEoEntry, pCookie->scope);
    if( CL_OK != rc )
    {
        return rc;
    }    
    CL_LOG_DEBUG_VERBOSE(("maxStreams: %d \n", pCommonEoEntry->maxStreams));
    rc = clLogStreamOwnerEntryGet(pSoEoEntry, openFlags, pStreamName, 
                                  pCookie->scope, pStreamScopeNode, 
                                  &hStreamOwnerNode,
                                  &pStreamOwnerData, &entryAdd);
    if( CL_OK != rc )
    {
        clLogSOUnlock(pSoEoEntry, pCookie->scope);
        return rc;
    }    

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        clLogSOUnlock(pSoEoEntry, pCookie->scope);
        return rc;
    }    
    if( CL_LOG_STREAM_LOCAL == pCookie->scope )
    {
        rc = clLogStreamOwnerCkptDsIdGet(pSoEoEntry, &pStreamOwnerData->dsId);
        if( CL_OK != rc )
        {
            pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_UN_INIT;
            clOsalCondBroadcast(&pStreamOwnerData->nodeCond);
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
            clLogSOUnlock(pSoEoEntry, pCookie->scope);
            return rc;
        }
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, pCookie->scope), CL_OK);

    rc = clLogStreamOwnerCompRefCountGet(pSoEoEntry, pCookie->scope, 
                                         hStreamOwnerNode, pCookie->nodeAddr,
                                         pCookie->compId, &refCount);
    if( (CL_OK != rc) || ( 0 == refCount) )
    {
        pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_UN_INIT;
        clOsalCondBroadcast(&pStreamOwnerData->nodeCond);
        /* Explicitly returning this error */
       clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
       return CL_LOG_RC(CL_ERR_NOT_EXIST); 
    }    

    rc = clLogStreamOwnerEntryUpdate(pSoEoEntry, pStreamName, pCookie->scope,
                                     pStreamScopeNode, pStreamOwnerData, 
                                     pStreamAttr, *pStreamMcastAddr,
                                     pFilter, *pStreamId, pCookie->compId);
    *pAckerCnt    = pStreamOwnerData->ackerCnt;
    *pNonAckerCnt = pStreamOwnerData->nonAckerCnt;
    clOsalCondBroadcast(&pStreamOwnerData->nodeCond);
    clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
    
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

void
clLogStreamOwnerMAVGResponse(ClIdlHandleT            hLogIdl,
                             ClLogStreamAttrIDLT     *pStreamAttr,
                             ClNameT                 *pStreamName,
                             ClLogStreamScopeT       *pStreamScope, 
                             ClNameT                 *pStreamScopeNode,
                             ClUint16T               *pStreamId,
                             ClIocMulticastAddressT  *pStreamMcastAddr,
                             ClRcT                   retCode,
                             ClPtrT                  pData)
{
    ClRcT           rc          = CL_OK;
    ClLogSOCookieT  *pCookie    = (ClLogSOCookieT *) pData;
    ClLogFilterT    filter      = {0};
    ClUint32T       ackerCnt    = 0;
    ClUint32T       nonAckerCnt = 0;

    CL_LOG_DEBUG_TRACE(("Enter: streamId %d  retCode: %x", *pStreamId, retCode));

    CL_ASSERT(NULL != pCookie);
    if( retCode == CL_OK )
    {
        rc = clLogStreamOwnerMAVGResponseProcess(pStreamName, pStreamScopeNode,
                                                 pStreamAttr, pStreamMcastAddr,
                                                 pStreamId, &filter, &ackerCnt, 
                                                 &nonAckerCnt, retCode, 
                                                 pCookie);
        if( CL_OK != rc )
        {
            retCode = rc;
            /* call master to close the stream */
            CL_LOG_CLEANUP(
               clLogStreamOwnerMasterClose(pStreamName, pStreamScopeNode, 
                                           pStreamAttr), 
               CL_OK);
        }
    }
    if( CL_OK != retCode )
    {
        /* In master, open failed, so reverting back to old */
       rc = clLogStreamOwnerOpenCleanup(pCookie);
    }
    clLogDebug("SOW", "OPE", "Sending the open response back to server retcode [0x %x]"
            "streamName: [%.*s]", retCode, pStreamName->length, pStreamName->value);
    if( CL_OK == retCode )
    {
        VDECL_VER(clLogStreamOwnerStreamOpenResponseSend, 4, 0, 0)(pCookie->hDeferIdl, retCode,
                                               *pStreamName, *pStreamScope, 
                                               *pStreamScopeNode, pCookie->compId,
                                               *pStreamAttr, *pStreamMcastAddr,
                                               filter, ackerCnt, nonAckerCnt,
                                               *pStreamId);
    }   
    else
    {
        clHeapFree(pStreamAttr->fileName.pValue);
        pStreamAttr->fileName.pValue     = NULL;
        clHeapFree(pStreamAttr->fileLocation.pValue);
        pStreamAttr->fileLocation.pValue = NULL;
        filter.pMsgIdSet = NULL;
        filter.pCompIdSet= NULL;
        VDECL_VER(clLogStreamOwnerStreamOpenResponseSend, 4, 0, 0)(pCookie->hDeferIdl, retCode,
                                               *pStreamName, *pStreamScope, 
                                               *pStreamScopeNode, pCookie->compId,
                                               *pStreamAttr, *pStreamMcastAddr,
                                               filter, ackerCnt, nonAckerCnt,
                                               *pStreamId);
    }
    clHeapFree(pCookie);

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return;
}    
/*
 * Function - clLogStreamOwnerInitStateUpdate()
 *  - Get the corresspoding the data for node.
 *  - If CREATE flags specified, match the attributes.
 *  - Copy the varibles into output varibles.
 */
ClRcT
clLogStreamOwnerAttrVerifyNGet(ClLogSOEoDataT          *pSoEoEntry,
                               ClLogStreamOpenFlagsT   openFlags,
                               ClLogStreamOwnerDataT   *pStreamOwnerData,
                               ClLogStreamAttrIDLT     *pStreamAttr,
                               ClIocMulticastAddressT  *pStreamMcastAddr,
                               ClLogFilterT            *pStreamFilter,
                               ClUint32T               *pAckerCnt, 
                               ClUint32T               *pNonAckerCnt, 
                               ClUint16T               *pStreamId)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_STREAM_CREATE == openFlags )
    {
        rc = clLogAttributesMatch(&pStreamOwnerData->streamAttr, pStreamAttr, 
                                  CL_TRUE);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("This streamEntry is already exist"));
            return CL_LOG_RC(CL_ERR_ALREADY_EXIST);
        }    
        /* 
         * As we need to copy the attributes, in order to avoid the leak freeing
         * the memory, in case of open case, attributes will be NULL.
         */
        clHeapFree(pStreamAttr->fileName.pValue);
        clHeapFree(pStreamAttr->fileLocation.pValue);
    }    

    *pStreamMcastAddr = pStreamOwnerData->streamMcastAddr;
    *pStreamId        = pStreamOwnerData->streamId;
    *pAckerCnt        = pStreamOwnerData->ackerCnt;
    *pNonAckerCnt     = pStreamOwnerData->nonAckerCnt;
    rc = clLogFilterFormatConvert(&pStreamOwnerData->streamFilter, 
                                  pStreamFilter);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogStreamAttributesCopy(&pStreamOwnerData->streamAttr, 
                                   pStreamAttr, CL_TRUE);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerOpenInfoSend()
 *  - Get the current logMasterAddress.
 *  - Initialize the idlHandle for communication with master.
 *  - Allocate memory for cookie.
 *  - Defere the call.
 *  - Populate the cookie.
 *  - Make Call to master (async call)
 *  - do cleanup.
 */

ClRcT
clLogStreamOwnerMasterOpen(ClLogSOEoDataT         *pSoEoEntry,
                           ClIocNodeAddressT      nodeAddr,
                           ClLogStreamScopeT      streamScope,
                           ClCntNodeHandleT       hStreamOwnerNode,
                           ClUint32T              *pCompId,
                           ClNameT                *pStreamName,
                           ClNameT                *pStreamScopeNode,
                           ClLogStreamAttrIDLT    *pStreamAttr)
{
    ClRcT                   rc            = CL_OK;
    ClIdlHandleT            hLogIdl       = CL_HANDLE_INVALID_VALUE;
    ClIocAddressT           masterAddr    = {{0}};
    ClLogSOCookieT          *pCookie      = NULL;
    ClIocMulticastAddressT  multiCastAddr = 0;
    ClUint16T               streamId      = 0;
    ClLogFilterT            filter        = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterAddressGet(&masterAddr);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogIdlHandleInitialize(masterAddr, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }    

    pCookie = clHeapCalloc(1, sizeof(ClLogSOCookieT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
        
    rc = clLogIdlSyncDefer(&(pCookie->hDeferIdl)); 
    if( CL_OK != rc )
    {        
        CL_LOG_DEBUG_ERROR(( "cllogEoIdlSyncDefer(): rc[0x %x]", rc));
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }        
    pCookie->compId      = *pCompId;
    pCookie->nodeAddr    = nodeAddr;
    pCookie->scope       = streamScope; 
    pCookie->hStreamNode = hStreamOwnerNode;
    clLogDebug("SOW", "OPE", "Making call to master for stream open [%.*s]", 
            pStreamName->length, pStreamName->value); 
    rc = VDECL_VER(clLogMasterAttrVerifyNGetClientAsync, 4, 0, 0)(hLogIdl, pStreamAttr, pStreamName,
                                              &streamScope, pStreamScopeNode, 
                                              &streamId, &multiCastAddr,
                                              clLogStreamOwnerMAVGResponse,
                                              pCookie);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamOwnerMAVGResponse(): rc[0x %x]", rc));
        VDECL_VER(clLogStreamOwnerStreamOpenResponseSend, 4, 0, 0)(pCookie->hDeferIdl, rc,
                                               *pStreamName, streamScope,
                                               *pStreamScopeNode, *pCompId,  
                                               *pStreamAttr, multiCastAddr, 
                                               filter, 0, 0, streamId);
        clHeapFree(pCookie);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }    
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
    /*
     * The memory which was allocated by IDL. As we are defering the call,
     * and idl generated code is not freeing the allocated memory. 
     FIXME - dont know, problem with idl or us..Freeing the memory
     */
    clHeapFree(pStreamAttr->fileName.pValue);
    clHeapFree(pStreamAttr->fileLocation.pValue);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamDoesCompIdExist()
 *  - Get the corresponding streamTable based on streamScope.
 *  - Get the streamOwnerData for the streamOwnerNode.
 *  - Check the compId Exist.
 *  - Return the refCount, if exist.
 */

ClRcT
clLogStreamOwnerCompRefCountGet(ClLogSOEoDataT     *pSoEoEntry,
                                ClLogStreamScopeT  streamScope,
                                ClCntNodeHandleT   hStreamNode,
                                ClIocNodeAddressT  nodeAddr,
                                ClUint32T          compId,
                                ClUint16T          *pRefCount)
{ 
    ClRcT                  rc                = CL_OK;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClLogCompKeyT          compKey           = {0};
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogSOCompDataT       *pData            = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeUserDataGet(hStreamTable, hStreamNode, 
                              (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }    
    
    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoEntry->maxComponents;
    rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable,
                            (ClCntKeyHandleT) &compKey,
                            (ClCntDataHandleT *) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForkeyGet(): rc[0x %x]", rc));
        return rc;
    }
    *pRefCount = pData->refCount;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerEntryProcess()
 * if WIP wait on condVar.
 * Check compId exist
 * If not exist - Return the NOT_EXIST(use case ? ).
 * if refCount == 0, remove the compId Entry.
 *  
 */

ClRcT
clLogStreamOwnerEntryProcess(ClLogSOEoDataT          *pSoEoEntry, 
                             ClLogStreamOpenFlagsT   openFlags,
                             ClIocNodeAddressT       nodeAddr,
                             ClLogStreamOwnerDataT   *pStreamOwnerData,
                             ClCntNodeHandleT        hStreamOwnerNode,
                             ClUint32T               *pCompId,
                             ClNameT                 *pStreamName,
                             ClLogStreamScopeT       streamScope,
                             ClNameT                 *pStreamScopeNode,
                             ClLogStreamAttrIDLT     *pStreamAttr,
                             ClIocMulticastAddressT  *pStreamMcastAddr,
                             ClLogFilterT            *pStreamFilter,
                             ClUint32T               *pAckerCnt, 
                             ClUint32T               *pNonAckerCnt, 
                             ClUint16T               *pStreamId)
{
    ClRcT            rc       = CL_OK;
    ClUint16T        refCount =  0;
    ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = 0 };
    CL_LOG_DEBUG_TRACE(("Enter"));

    while( CL_LOG_NODE_STATUS_WIP == pStreamOwnerData->nodeStatus)
    {
        /*
         * The same stream is already being in the creation process,
         * wait for 100 ms and check the status and go out.
         */
        rc = clOsalCondWait_L(&pStreamOwnerData->nodeCond, 
                              &pStreamOwnerData->nodeLock, 
                              timeout);
        if( (CL_GET_ERROR_CODE(rc) != CL_ERR_TIMEOUT) && (CL_OK != rc) )
        {
           CL_LOG_DEBUG_ERROR(("clOsalCondWait(); rc[0x %x]", rc));
           return rc;
        }    
    }    
    
    rc = clLogStreamOwnerCompRefCountGet(pSoEoEntry, streamScope,
                                         hStreamOwnerNode, nodeAddr,
                                         *pCompId, &refCount);
    if( (CL_OK != rc) ||( 0 == refCount) )
    {
       /* 
        *  Entry has to be removed from compTable,  
        *  The calling function will do that
        *  (use case compDown, after it has
        *  sent open request to Master)
        */
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }    

    if( pStreamOwnerData->nodeStatus == CL_LOG_NODE_STATUS_UN_INIT
        ||
        pStreamOwnerData->nodeStatus == CL_LOG_NODE_STATUS_REINIT)
    {
        if( openFlags != CL_LOG_STREAM_CREATE )
        {
            /*
             * use case 
             */
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }    
        pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_WIP;
        rc = clLogStreamOwnerMasterOpen(pSoEoEntry, nodeAddr,
                                        streamScope, hStreamOwnerNode,
                                        pCompId, pStreamName,
                                        pStreamScopeNode, pStreamAttr);
        if( CL_OK != rc )
        {
            pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_UN_INIT;
            return rc;
        }    
    }    
    else
    {
        rc = clLogStreamOwnerAttrVerifyNGet(pSoEoEntry, openFlags, 
                                            pStreamOwnerData, pStreamAttr,
                                            pStreamMcastAddr, pStreamFilter,
                                            pAckerCnt, pNonAckerCnt, pStreamId);
        if( CL_OK != rc )
        {
            return rc;
        }    
        if( CL_LOG_STREAM_GLOBAL == streamScope )
        {
            clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, pStreamName,
                                             pStreamScopeNode, pStreamOwnerData);
        }    
        else
        {
            clLogStreamOwnerLocalCheckpoint(pSoEoEntry, pStreamName, 
                                            pStreamScopeNode,
                                            pStreamOwnerData);
        }    
    }    
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerCompEntryAdd()
 * - Check the compId exist or not.
 * - If not exist, add the Entry.
 * - Increment the refCount.
 */
ClRcT
clLogStreamOwnerStreamHdlrEntryAdd(ClLogStreamOwnerDataT     *pStreamOwnerData,
                                   ClIocNodeAddressT         nodeAddr,
                                   ClUint32T                 compId,
                                   ClLogStreamHandlerFlagsT  flags)
{
    ClRcT                  rc              = CL_OK;
    ClLogCompKeyT          compKey         = {0};
    ClLogCompKeyT          *pCompKey       = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSOCompDataT       *pData          = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter"));
    
    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoEntry->maxComponents;
    CL_LOG_DEBUG_TRACE(("nodeAddr: %d  compId: %d ", nodeAddr, compId));
    rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable, 
                            (ClCntKeyHandleT) &compKey, 
                            (ClCntDataHandleT *) &pData);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        /* Particular compId not exist */
        pCompKey = clHeapCalloc(1, sizeof(ClLogCompKeyT));
        if( NULL == pCompKey )
        {
            CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }    
        *pCompKey = compKey;

        pData = clHeapCalloc(1, sizeof(ClLogSOCompDataT));
        if( NULL == pData )
        {
            CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
            clHeapFree(pCompKey);
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }    
        rc = clCntNodeAdd(pStreamOwnerData->hCompTable, 
                          (ClCntKeyHandleT) pCompKey,
                          (ClCntDataHandleT) pData, NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clCntNodeAdd(): rc[0x %x]", rc));
            clHeapFree(pData);
            clHeapFree(pCompKey);
            return rc;
        }    
    }    
    else if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(
                ("an entry does not exists for the stream: compId [%u] rc [0x %x]",
                 compId, rc));
        return rc;
    }
    /* Exist, Just increment the refCount */
    if( CL_LOG_HANDLER_WILL_ACK == flags )
    {
        ++pData->ackerCnt;
        ++pStreamOwnerData->ackerCnt;
    }
    else
    {
        ++pData->nonAckerCnt;
        ++pStreamOwnerData->nonAckerCnt;
    }

    CL_LOG_DEBUG_TRACE(( "Exit: rc[0x%x]", rc));
    return rc;
}    

/*
 * Function - clLogStreamOwnerCompEntryAdd()
 * - Check the compId exist or not.
 * - If not exist, add the Entry.
 * - Increment the refCount.
 */
ClRcT
clLogStreamOwnerCompEntryAdd(ClLogStreamOwnerDataT  *pStreamOwnerData,
                             ClIocNodeAddressT      nodeAddr,
                             ClUint32T              compId)
{
    ClRcT                  rc              = CL_OK;
    ClLogCompKeyT          compKey         = {0};
    ClLogCompKeyT          *pCompKey       = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSOCompDataT       *pData          = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter"));
    
    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoEntry->maxComponents;
    CL_LOG_DEBUG_TRACE(("nodeAddr: %d  compId: %d ", nodeAddr, compId));
    rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable, 
                            (ClCntKeyHandleT) &compKey, 
                            (ClCntDataHandleT *) &pData);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        /* Particular compId not exist */
        pCompKey = clHeapCalloc(1, sizeof(ClLogCompKeyT));
        if( NULL == pCompKey )
        {
            CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }    
        *pCompKey = compKey;

        pData = clHeapCalloc(1, sizeof(ClLogSOCompDataT));
        if( NULL == pData )
        {
            CL_LOG_DEBUG_ERROR(( "clHeapCalloc()"));
            clHeapFree(pCompKey);
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }    
        rc = clCntNodeAdd(pStreamOwnerData->hCompTable, 
                          (ClCntKeyHandleT) pCompKey,
                          (ClCntDataHandleT) pData, NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clCntNodeAdd(): rc[0x %x]", rc));
            clHeapFree(pData);
            clHeapFree(pCompKey);
            return rc;
        }    
    }    
    else if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("Getting data for comp table failed rc[0x %x] compId is %u",
                                    rc ,compId));
        return rc;
    }
    /* Exist, Just increment the refCount */
    ++pData->refCount;
    ++pStreamOwnerData->openCnt;
    if(pData->refCount > 1)
    {
        --pData->refCount;
        --pStreamOwnerData->openCnt;
        if(pStreamOwnerData->nodeStatus == CL_LOG_NODE_STATUS_INIT)
        {
            pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_REINIT;
        }
    }

    CL_LOG_DEBUG_TRACE(( "Exit: rc[0x%x]", rc));
    return rc;
}    

/*
 * clLogStreamOwnerEntryAdd()
 * - allocte memory for streamOwner data.
 * - Create the list for component table.
 * - Add the entry to the table.
 */
ClRcT
clLogStreamOwnerEntryAdd(ClCntHandleT       hStreamTable,
                         ClLogStreamScopeT  streamScope,
                         ClLogStreamKeyT    *pStreamKey,
                         ClCntNodeHandleT   *phStreamOwnerNode)
{
    ClRcT                  rc                = CL_OK;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    pStreamOwnerData = clHeapCalloc(1, sizeof(ClLogStreamOwnerDataT)); 
    if( NULL == pStreamOwnerData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }    

    rc = clCntHashtblCreate(pCommonEoEntry->maxStreams, clLogCompKeyCompare, 
                            clLogCompTblHashFn, clLogCompTblDeleteCb,
                            clLogCompTblDeleteCb, CL_CNT_UNIQUE_KEY,
                            &(pStreamOwnerData->hCompTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtblCreate: rc[0x %x]", rc));
        clHeapFree(pStreamOwnerData);
        return rc;
    }    

    pStreamOwnerData->nodeStatus  = CL_LOG_NODE_STATUS_UN_INIT;
    pStreamOwnerData->dsId        = CL_LOG_INVALID_DSID;
    pStreamOwnerData->isNewStream = CL_TRUE;
    pStreamOwnerData->condDelete  = CL_FALSE;
    pStreamOwnerData->openCnt     = 0;
    pStreamOwnerData->ackerCnt    = 0;
    pStreamOwnerData->nonAckerCnt = 0;
    rc = clOsalCondInit_L(&pStreamOwnerData->nodeCond);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalCondCreate(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pStreamOwnerData->hCompTable), CL_OK);
        clHeapFree(pStreamOwnerData);
        return rc;
    }    
    rc = clOsalMutexInit_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalCondDestroy_L(&pStreamOwnerData->nodeCond), CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pStreamOwnerData->hCompTable), CL_OK);
        clHeapFree(pStreamOwnerData);
        return rc;
    }    
    rc = clLogStreamOwnerFilterInit(&pStreamOwnerData->streamFilter);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pStreamOwnerData->nodeLock), CL_OK);
        CL_LOG_CLEANUP(clOsalCondDestroy_L(&pStreamOwnerData->nodeCond), CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pStreamOwnerData->hCompTable), CL_OK);
        clHeapFree(pStreamOwnerData);
        return rc;
    }

    rc = clCntNodeAddAndNodeGet(hStreamTable, (ClCntKeyHandleT) pStreamKey, 
                                (ClCntDataHandleT) pStreamOwnerData, NULL,
                                phStreamOwnerNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        clLogStreamOwnerFilterFinalize(&pStreamOwnerData->streamFilter);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pStreamOwnerData->nodeLock), CL_OK);
        CL_LOG_CLEANUP(clOsalCondDestroy_L(&pStreamOwnerData->nodeCond), CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pStreamOwnerData->hCompTable), CL_OK);
        clHeapFree(pStreamOwnerData);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerEntryGet()
 *  - Search the table with key - streamName & streamScope.
 *  - If the Entry Exist, just get the node.
 *  - Else
 *     - Allocate memory for streamOwnerData.
 *     - Create the compTable. 
 *     - Initialize all variables with default values.
 *  - Return the nodeHandle.
 */
ClRcT
clLogStreamOwnerEntryGet(ClLogSOEoDataT         *pSoEoEntry,
                         ClLogStreamOpenFlagsT  openFlags,
                         ClNameT                *pStreamName, 
                         ClLogStreamScopeT      streamScope,
                         ClNameT                *pStreamScopeNode,
                         ClCntNodeHandleT       *phStreamOwnerNode,
                         ClLogStreamOwnerDataT  **ppStreamOwnerData,
                         ClBoolT                *pAddedEntry) 
{
    ClRcT                  rc              = CL_OK;
    ClLogStreamKeyT        *pStreamKey     = NULL;
    ClCntHandleT           hStreamTable    = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter: %d", streamScope));

    *pAddedEntry = CL_FALSE;
    rc = clLogStreamOwnerEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode, 
                              pCommonEoEntry->maxStreams,
                              &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }        

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntNodeFind(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                       phStreamOwnerNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        if( CL_LOG_STREAM_CREATE != openFlags )
        {
            CL_LOG_DEBUG_ERROR(("openFlags are not proper"));
            clLogStreamKeyDestroy(pStreamKey);
            /* Explicitly retruning Error to avoid container error to be returned*/
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }    
        rc = clLogStreamOwnerEntryAdd(hStreamTable, streamScope, pStreamKey,
                                      phStreamOwnerNode);
        if( CL_OK != rc )
        {
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }    
        *pAddedEntry = CL_TRUE;
    }    
    else
    {
        clLogStreamKeyDestroy(pStreamKey);
    }    

    rc = clCntNodeUserDataGet(hStreamTable, *phStreamOwnerNode, 
                              (ClCntDataHandleT *) ppStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clCntNodeUserDataGet(): rc[0x %x]", rc));
        if( CL_TRUE == *pAddedEntry )
        {
            CL_LOG_CLEANUP(
                    clCntNodeDelete(hStreamTable, *phStreamOwnerNode), 
                    CL_OK );
        }
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerEoEntryGet(ClLogSOEoDataT         **ppSoEoEntry,
                           ClLogSvrCommonEoDataT  **ppCommonEoEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    if( NULL != ppSoEoEntry )
    {
        rc = clEoPrivateDataGet(pEoObj, CL_LOG_STREAM_OWNER_EO_ENTRY_KEY, 
                                (void **) ppSoEoEntry); 
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
            return rc;
        }    
    }

    if( NULL != ppCommonEoEntry )
    {
        rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                                (void **) ppCommonEoEntry); 
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
        }    
    }

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

ClRcT
clLogStreamOwnerStreamTableGet(ClLogSOEoDataT     **ppSoEoEntry, 
                               ClLogStreamScopeT  streamScope)
{
    ClRcT                  rc             = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(( "Enter"));
    
    rc = clLogStreamOwnerEoEntryGet(ppSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_LOG_STREAM_GLOBAL != streamScope )
    {
        rc = clOsalMutexLock_L(&(*ppSoEoEntry)->lStreamTableLock);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clOsalMutexLock(): rc[0x %x]", rc));
            return rc;
        }
        if( CL_HANDLE_INVALID_VALUE ==
                         (*ppSoEoEntry)->hLStreamOwnerTable )
        {
            rc = clCntHashtblCreate(pCommonEoEntry->maxStreams, 
                                    clLogStreamKeyCompare, 
                                    clLogStreamHashFn,
                                    clLogStreamOwnerDeleteCb,
                                    clLogStreamOwnerDeleteCb,
                                    CL_CNT_UNIQUE_KEY,
                                    &(*ppSoEoEntry)->hLStreamOwnerTable);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(( "clCntHashtblCreate(): rc[0x %x]", rc));
            }    
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(*ppSoEoEntry)->lStreamTableLock),
                       CL_OK);
    }
    else
    {
        rc = clOsalMutexLock_L(&(*ppSoEoEntry)->gStreamTableLock);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(( "clOsalMutexLock(): rc[0x %x]", rc));
            return rc;
        }
        if( CL_HANDLE_INVALID_VALUE == 
                (*ppSoEoEntry)->hGStreamOwnerTable )
        {
            rc = clCntHashtblCreate(pCommonEoEntry->maxStreams, 
                                    clLogStreamKeyCompare, 
                                    clLogStreamHashFn,
                                    clLogStreamOwnerDeleteCb,
                                    clLogStreamOwnerDeleteCb,
                                    CL_CNT_UNIQUE_KEY,
                                    &(*ppSoEoEntry)->hGStreamOwnerTable);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(( "clCntHashtblCreate(): rc[0x %x]", rc));
            }    
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&(*ppSoEoEntry)->gStreamTableLock), 
                       CL_OK);
    }

    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}

/*
 * Function clLogStreamOwnerStreamOpen()
 * - Validate all inout and out parameters.
 * - Get the streamOwner Table.
 * - Take table lock.
 * - Get the Entry from StreamOwner Table.
 * - Take node lock.
 * - Unlock the table lock.
 * - Process the node Entry
 * - Unlock the node.
 *
 * --Test Cases
 *  - create Flag
 *     -- first entry.
 *  - Open Flag
 *    -- Entry Doesnot Exist
 *  Exist-
 *     -- Increment compId refCount.
 *     -- Adding new compId
 *     -- Matching the attributes.
 *  
 */
ClRcT
VDECL_VER(clLogStreamOwnerStreamOpen, 4, 0, 0)(
                           ClLogStreamOpenFlagsT   openFlags,
                           ClIocNodeAddressT       nodeAddr,
                           ClNameT                 *pStreamName,
                           ClLogStreamScopeT       *pStreamScope, 
                           ClNameT                 *pStreamScopeNode,
                           ClUint32T               *pCompId,
                           ClLogStreamAttrIDLT     *pStreamAttr,
                           ClIocMulticastAddressT  *pStreamMcastAddr,
                           ClLogFilterT            *pStreamFilter,
                           ClUint32T               *pAckerCnt,
                           ClUint32T               *pNonAckerCnt, 
                           ClUint16T               *pStreamId)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry        = NULL;
    ClCntNodeHandleT       hStreamOwnerNode   = CL_HANDLE_INVALID_VALUE;
    ClBoolT                addedEntry         = CL_FALSE;  
    ClUint32T              tableSize          = 0;
    ClLogStreamOwnerDataT  *pStreamOwnerData  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter %d", *pStreamId));
    
    CL_LOG_PARAM_CHK(NULL == pCompId, CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK(NULL == pStreamName, CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK(NULL == pStreamScopeNode, CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK(NULL == pStreamMcastAddr, CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK(NULL == pStreamFilter, CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK(NULL == pStreamId, CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerStreamTableGet(&pSoEoEntry, *pStreamScope);
    if( CL_OK != rc )
    {
        return rc;
    }    
    CL_LOG_DEBUG_TRACE(("streamName; %s streamScope: %s\n",
                pStreamName->value, pStreamScopeNode->value));
    rc = clLogSOLock(pSoEoEntry, *pStreamScope);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clLogStreamOwnerEntryGet(pSoEoEntry, openFlags, pStreamName, 
                                  *pStreamScope, pStreamScopeNode, &hStreamOwnerNode,
                                  &pStreamOwnerData, &addedEntry);
    if( CL_OK != rc )
    {
        /* Entry does not exist, OPEN flag */
        clLogSOUnlock(pSoEoEntry, *pStreamScope);
        return rc;
    }    
    
    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        if( CL_TRUE == addedEntry )
        {
           CL_LOG_CLEANUP(
                   clLogStreamOwnerEntryDelete(pSoEoEntry, *pStreamScope,
                                               hStreamOwnerNode),
                   CL_OK); 
        }    
        clLogSOUnlock(pSoEoEntry, *pStreamScope);
        return rc;
    }    

    rc = clLogStreamOwnerCompEntryAdd(pStreamOwnerData, nodeAddr, *pCompId);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
        if( CL_TRUE == addedEntry )
        {
           CL_LOG_CLEANUP(
                   clLogStreamOwnerEntryDelete(pSoEoEntry, *pStreamScope,
                                               hStreamOwnerNode),
                   CL_OK); 
        }    
        clLogSOUnlock(pSoEoEntry, *pStreamScope);
        return rc;
    }    
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, *pStreamScope), CL_OK);
    /* 
     * Internally will make call to master, In case any failure,
     * unlock nodeMutex. and return INVALID_STATE
     */
    rc = clLogStreamOwnerEntryProcess(pSoEoEntry, openFlags, nodeAddr, pStreamOwnerData,
                                      hStreamOwnerNode, pCompId, pStreamName, 
                                      *pStreamScope, pStreamScopeNode, pStreamAttr,
                                      pStreamMcastAddr, pStreamFilter,
                                      pAckerCnt, pNonAckerCnt, pStreamId);
    if( CL_OK != rc )
    {
        /* 
         * Should not be any error check, coz on success it may return 
         * this CL_LOG_ERR_STREAM_DELETE
         */
        if(pStreamOwnerData->nodeStatus == CL_LOG_NODE_STATUS_REINIT)
        {
            pStreamOwnerData->nodeStatus = CL_LOG_NODE_STATUS_INIT;
        }
        CL_LOG_CLEANUP(
                clLogStreamOwnerCompEntryDelete(pStreamOwnerData, nodeAddr,  
                                                *pCompId, &tableSize), 
                CL_OK);
        clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
        if( 0 == tableSize )
        {
            CL_LOG_CLEANUP(
                clLogStreamOwnerEntryChkNDelete(pSoEoEntry, *pStreamScope, 
                                                hStreamOwnerNode),
                CL_OK);
        }
        return rc;
    }    
    /*
     * Everything went successfull, so no problem, unlock and exit.
     * Internally making async call, so holding mutex doesn't matter.
     */
    clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

#define  clStreamOwnerEntryDump(pSoEoEntry)\
    clOsalPrintf(" LINE: %d HT: %p LT: %p hl %p ll %p\n", __LINE__, \
                pSoEoEntry->hGStreamOwnerTable, pSoEoEntry->hLStreamOwnerTable, pSoEoEntry->gStreamTableLock,\
                  pSoEoEntry->lStreamTableLock);

/**************************FILTER SET *******************/
/*
 * Function - clLogStreamOwnerFilterSet()
 *  Find the streamTable using streamScope.  
 *  Find the Entry using pStreamKey
 *  Change the filter. 
 *  Go thru the compList, and propagate the info other servers.
 */
ClRcT
VDECL_VER(clLogStreamOwnerFilterSet, 4, 0, 0)(
                          ClNameT            *pStreamName,
                          ClLogStreamScopeT  streamScope,
                          ClNameT            *pStreamScopeNode,
                          ClLogFilterFlagsT  filterFlags,
                          ClLogFilterT       *pLogFilter)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey); 
    if( CL_OK != rc ) goto abort;

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    if( hStreamTable == CL_HANDLE_INVALID_VALUE )
    {
        clLogError(CL_LOG_AREA_STREAM_OWNER, CL_LOG_CTX_SO_FILTERSET, 
                   "LogServer is not yet ready to change the filter, please try again");
        
        rc = CL_LOG_RC(CL_ERR_TRY_AGAIN);
        goto abort;
    }
    rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc ) goto abort;

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc ) goto abort;

    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    rc = clLogStreamOwnerDataUpdate(pSoEoEntry, pStreamKey, pStreamOwnerData, 
                                    streamScope, filterFlags, pLogFilter);
    
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    goto done;
    
  abort:
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
  done:
    if (pStreamKey) clLogStreamKeyDestroy(pStreamKey);
    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

/*
 * Function - clLogStreamOwnerDataUpdate()
 *  Modify the filter according to filter flags specified.
 *  Walk thru the compList table, and propagate the filter 
 *  to all the servers.
 */
ClRcT
clLogStreamOwnerDataUpdate(ClLogSOEoDataT         *pSoEoEntry,
                           ClLogStreamKeyT        *pStreamKey,
                           ClLogStreamOwnerDataT  *pStreamOwnerData, 
                           ClLogStreamScopeT      streamScope,
                           ClLogFilterFlagsT      filterFlags,
                           ClLogFilterT           *pLogFilter)
{
    ClRcT                 rc       = CL_OK;
    ClLogSOCommonCookieT  *pCookie = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerFilterModify(filterFlags, pStreamOwnerData, 
                                      pLogFilter);
    if( CL_OK != rc )
    {
        return rc;
    }
    pCookie = clHeapCalloc(1, sizeof(ClLogSOCommonCookieT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    pCookie->sendFilter    = CL_TRUE; 
    pCookie->pStreamKey    = pStreamKey;
    pCookie->pStreamFilter = &pStreamOwnerData->streamFilter;
    rc = clCntWalk(pStreamOwnerData->hCompTable, clLogStreamOwnerSvrIntimate, 
                   (ClCntArgHandleT) pCookie, sizeof(ClLogSOCommonCookieT));
    if( CL_OK != rc )
    {
        /*
         * If it is failure on this do we need to revert back the filter
         * modifications.
         */
        clHeapFree(pCookie);
        return rc;
    }

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, &pStreamKey->streamName,
                                         &pStreamKey->streamScopeNode, 
                                         pStreamOwnerData);
    }    
    else
    {
        clLogStreamOwnerLocalCheckpoint(pSoEoEntry, &pStreamKey->streamName,
                                        &pStreamKey->streamScopeNode,
                                        pStreamOwnerData);
    }    
    clHeapFree(pCookie);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

/*
 * Function - clLogStreamOwnerFilterModify()
 *   CL_LOG_FILTER_ASSIGN:
 *      - Destory the bitmap. 
 *      - Create the bitmap with LEN = hMsgIdMapLen; 
 *      - Walk thru the hMsgIdMap and Set the bit in bitmap, if it is set.
 *  CL_LOG_FILTER_MERGE_ADD:
 *      - len(storedBitmap) < hMsgIdMapLen
 *         - Increse the length of bitmap.
 *      - Walk thru the hMsgIdMap.
 *        if( isBitSet(hMsgIdMap) )
 *          set(stroedMap).
 *  CL_LOG_FILTER_MERGE_DELETE:
 *      - length(storedBitmap) < hMsgIdMapLen
 *         - Increase the length of bitmap.
 *      - Walk thru hMsgIdMap.
 *          if( isBitSet(storedBitmap) & isBitSet(hMsgIdMap) )
 *             clear(storedBitmap);
 *          else
 *            bitset(storedBitmap);
 *
 */ 
ClRcT
clLogStreamOwnerFilterModify(ClLogFilterFlagsT      filterFlags,
                             ClLogStreamOwnerDataT  *pStreamOwnerData,  
                             ClLogFilterT           *pNewFilter)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    switch(filterFlags)
    {
        case CL_LOG_FILTER_ASSIGN:
            {
                rc = clLogStreamOwnerFilterAssign(pNewFilter->msgIdSetLength, 
                                                  pNewFilter->pMsgIdSet, 
                                                  &pStreamOwnerData->streamFilter.hMsgIdMap);
                if( CL_OK != rc )
                {
                    return rc;
                }
                rc = clLogStreamOwnerFilterAssign(pNewFilter->compIdSetLength, 
                                                  pNewFilter->pCompIdSet, 
                                                  &pStreamOwnerData->streamFilter.hCompIdMap);
                pStreamOwnerData->streamFilter.severityFilter =
                            pNewFilter->severityFilter;   
                break;
            }
        case CL_LOG_FILTER_MERGE_ADD:
            {
                rc = clLogFilterMergeAdd(pNewFilter->msgIdSetLength, 
                                       pNewFilter->pMsgIdSet, 
                                       &pStreamOwnerData->streamFilter.hMsgIdMap);
                if( CL_OK != rc )
                {
                    return rc;
                }
                rc = clLogFilterMergeAdd(pNewFilter->compIdSetLength, 
                                       pNewFilter->pCompIdSet, 
                                       &pStreamOwnerData->streamFilter.hCompIdMap);
                pStreamOwnerData->streamFilter.severityFilter = 
                            pStreamOwnerData->streamFilter.severityFilter |
                            pNewFilter->severityFilter;
                break;
            }
        case CL_LOG_FILTER_MERGE_DELETE:            
            {
                rc = clLogFilterMergeDelete(pNewFilter->msgIdSetLength, 
                                       pNewFilter->pMsgIdSet, 
                                       pStreamOwnerData->streamFilter.hMsgIdMap);
                if( CL_OK != rc )
                {
                    return rc;
                }
                rc = clLogFilterMergeDelete(pNewFilter->compIdSetLength, 
                                       pNewFilter->pCompIdSet, 
                                       pStreamOwnerData->streamFilter.hCompIdMap);
                pStreamOwnerData->streamFilter.severityFilter = 
                      ( pStreamOwnerData->streamFilter.severityFilter & 
                       pNewFilter->severityFilter ) ^
                      pStreamOwnerData->streamFilter.severityFilter;
                break;
            }
        default:
           CL_LOG_DEBUG_ERROR(("Invalid filter flags"));
           return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * Function - clLogFilterAssign()
 *  - Destory the old bitmap.
 *  - Create new bit of length = logMsgIdSetLength * bitsperbyte.
 *  - Walk thru the passed msgIdSetMap.
 *  - Set the bit in storedbitmap, if it is set in passed map.
 */
ClRcT
clLogStreamOwnerFilterAssign(ClUint32T        numBytes,
                             ClUint8T         *pSetMap,
                             ClBitmapHandleT  *phBitmap)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  nBits     = 0;
    ClUint32T  bitNum    = 0;
    ClUint32T  bitIdx    = 0;
    ClUint32T  byteIdx   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBitmapDestroy(*phBitmap);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapDestroy(): rc[0x %x]", rc));
        return rc;
    }
    nBits = numBytes * CL_BITS_PER_BYTE;
    rc = clBitmapCreate(phBitmap, nBits);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        return rc;
    }

    for( bitNum = 0 ; bitNum < nBits; bitNum++)
    {
        byteIdx = bitNum / CL_BITS_PER_BYTE;
        bitIdx  = bitNum % CL_BITS_PER_BYTE;
        if( *(pSetMap + byteIdx) & (0x1 << bitIdx) )
        {
            rc = clBitmapBitSet(*phBitmap, bitNum);
            if( CL_OK != rc )
            {
                /* 
                 * it is not necessary to destroy the bitmap
                 */
                CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
                return rc;
            }
        }
    }
         
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * Function - clLogFilterMergeAdd()
 *  - Walk thru the passed msgIdSetMap.
 *  - Set the bit in storedbitmap, if it is set in passed map.
 */
ClRcT
clLogFilterMergeAdd(ClUint32T        numBytes,
                    ClUint8T         *pSetMap,
                    ClBitmapHandleT  *phBitmap)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  nBits     = 0;
    ClUint32T  bitNum    = 0;
    ClUint32T  bitIdx    = 0;
    ClUint32T  byteIdx   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    nBits = numBytes * CL_BITS_PER_BYTE;
    for( bitNum = 0 ; bitNum < nBits; bitNum++)
    {
        byteIdx = bitNum / CL_BITS_PER_BYTE;
        bitIdx  = bitNum % CL_BITS_PER_BYTE;
        if( *(pSetMap + byteIdx) & (0x1 << bitIdx) )
        {
            rc = clBitmapBitSet(*phBitmap, bitNum);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
                return rc;
            }
        }
    }
         
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFilterMergeDelete(ClUint32T        numBytes,
                       ClUint8T         *pSetMap,
                       ClBitmapHandleT  hBitmap)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  nBits     = 0;
    ClUint32T  bitNum    = 0;
    ClRcT      retCode   = CL_OK;
    ClInt32T   isSet     = 0;
    ClUint32T  byteIdx   = 0;
    ClUint32T  bitIdx    = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    nBits = numBytes * CL_BITS_PER_BYTE;
    for( bitNum = 0 ; bitNum < nBits; bitNum++)
    {
        byteIdx = bitNum / CL_BITS_PER_BYTE;
        bitIdx  = bitNum % CL_BITS_PER_BYTE;
        isSet = clBitmapIsBitSet(hBitmap, bitNum, &retCode); 
        if( CL_OK == retCode )
        {
            if( (isSet) && (*(pSetMap + byteIdx) & ( 0x1 << bitIdx )) )
            {
                rc = clBitmapBitClear(hBitmap, bitNum);
                if( CL_OK != rc )
                {
                    CL_LOG_DEBUG_ERROR(("clBitmapBitClear(): rc[0x %x]", rc));
                    return rc;
                }
            }
        }
#if 0
        if( *(pSetMap + byteIdx) & (0x1 << bitIdx) )
        {
            rc = clBitmapBitSet(hBitmap, bitNum);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
                return rc;
            }
        }
#endif
    }
         
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * Function - clLogFilterFormatConvert()
 * 
 */
ClRcT
clLogFilterFormatConvert(ClLogFilterInfoT  *pStoredFilter,
                         ClLogFilterT      *pPassedFilter)
{
    ClRcT      rc         = CL_OK;
    ClUint32T  numOfBytes = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    pPassedFilter->severityFilter = pStoredFilter->severityFilter;
    rc = clBitmap2BufferGet(pStoredFilter->hMsgIdMap, &numOfBytes, 
                           &pPassedFilter->pMsgIdSet);
    if( CL_OK != rc )
    {
        return rc;
    }
    pPassedFilter->msgIdSetLength  = (ClUint16T) numOfBytes;
    numOfBytes = 0;
    rc = clBitmap2BufferGet(pStoredFilter->hCompIdMap, &numOfBytes, 
                            &pPassedFilter->pCompIdSet);
    pPassedFilter->compIdSetLength = (ClUint16T) numOfBytes;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
        
/* Function clLogSOEODataSet()
 *  - Setting StreamOwnerGlobal Data in th Eo
 */
ClRcT
clLogStreamOwnerLocalBootup(void)
{
    ClRcT           rc          = CL_OK;
    ClLogSOEoDataT  *pSoEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerLocalEoDataInit(&pSoEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    } 

    rc = clLogSOLocalCkptGet(pSoEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogStreamOwnerLocalEoDataFinalize(pSoEoEntry), 
                       CL_OK);
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
} 

/* Function clLogStreamOwnerEoDataFree() 
 *  - Setting StreamOwnerGlobal Data in th Eo
 */
ClRcT
clLogStreamOwnerEoDataFree(void)
{
    ClRcT           rc          = CL_OK;
    ClLogSOEoDataT  *pSoEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    clHeapFree(pSoEoEntry);                   

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    
/* Function clLogSOEODataSet()
 *  - Setting StreamOwnerGlobal Data in th Eo
 */
ClRcT
clLogStreamOwnerLocalShutdown(void)
{
    ClRcT                  rc              = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_CLEANUP(clLogStreamOwnerLocalEoDataFinalize(pSoEoEntry),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

/*
 * Function - clLogStreamOwnerGlobalInit()
 *  - Global streamOwner related info will be initialized.
 */
ClRcT
clLogStreamOwnerGlobalBootup(void) 
{
    ClRcT                  rc              = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSOEoDataT         *pSoEoEntry     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexInit_L(&pSoEoEntry->gStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        return rc;
    }
    pSoEoEntry->hGStreamOwnerTable = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerGlobalShutdown(void)
{
    ClRcT                  rc              = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    /*
     * If table itself is not created, this node wont be master or deputy so
     * just returning from here 
     */
    if( CL_HANDLE_INVALID_VALUE == pSoEoEntry->hGStreamOwnerTable )
    {
        return CL_OK;
    }

    rc = clOsalMutexLock_L(&pSoEoEntry->gStreamTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hGStreamOwnerTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pSoEoEntry->hGStreamOwnerTable), CL_OK);
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSoEoEntry->gStreamTableLock), CL_OK);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSoEoEntry->gStreamTableLock), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogStreamOwnerFilterGet, 4, 0, 0)(
                          ClNameT            *pStreamName,
                          ClLogStreamScopeT  streamScope,
                          ClNameT            *pStreamScopeNode, 
                          ClLogFilterT       *pFilter)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClUint32T length = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    pFilter->severityFilter = pStreamOwnerData->streamFilter.severityFilter;

    rc = clBitmap2BufferGet(pStreamOwnerData->streamFilter.hMsgIdMap, 
                            &length,
                            &pFilter->pMsgIdSet);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }

    pFilter->msgIdSetLength = length;

    rc = clBitmap2BufferGet(pStreamOwnerData->streamFilter.hCompIdMap, 
                            &length,
                            &pFilter->pCompIdSet);
    if( CL_OK != rc )
    {
        clHeapFree(pFilter->pMsgIdSet);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }

    pFilter->compIdSetLength = length;

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

ClRcT
VDECL_VER(clLogStreamOwnerStreamMcastGet, 4, 0, 0)(
                               ClNameT                 *pStreamName,
                               ClLogStreamScopeT       streamScope,
                               ClNameT                 *pStreamScopeNode,
                               ClIocMulticastAddressT  *pMcastAddr) 
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode),
            CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    if(!hStreamTable
       ||
       (rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                                (ClCntDataHandleT *) &pStreamOwnerData)) != CL_OK)
    {
        /*
         * If the stream handle isnt present, then the stream isnt restored yet or hasn't been
         * opened. 
         */
        if(!hStreamTable)
        {
            rc = CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    *pMcastAddr = pStreamOwnerData->streamMcastAddr;
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    clLogStreamKeyDestroy(pStreamKey);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

ClRcT
VDECL_VER(clLogStreamOwnerHandlerRegister, 4, 0, 0)(
                                ClNameT                   *pStreamName,
                                ClLogStreamScopeT         streamScope,
                                ClNameT                   *pStreamScopeNode,
                                ClLogStreamHandlerFlagsT  handlerFlags,
                                ClIocNodeAddressT         nodeAddr, 
                                ClUint32T                 compId)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClLogSOCommonCookieT   *pCookie          = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode),
            CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    
    if( CL_FALSE == pSoEoEntry->status )
    {
        CL_LOG_DEBUG_ERROR(("Log Server is not ready to provide the"
                            "service"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }
    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(); rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    if( 0 == pStreamOwnerData->openCnt )
    {
        CL_LOG_DEBUG_ERROR(("StreamEntry doesn't exist"));
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }
    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    pCookie = clHeapCalloc(1, sizeof(ClLogSOCommonCookieT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    pCookie->sendFilter   = CL_FALSE;
    pCookie->pStreamKey   = pStreamKey;
    pCookie->handlerFlags = handlerFlags;
    pCookie->setFlags     = CL_TRUE;
    pCookie->nodeAddr     = 0;
    rc = clCntWalk(pStreamOwnerData->hCompTable, clLogStreamOwnerSvrIntimate, 
                   (ClCntArgHandleT) pCookie, sizeof(ClLogSOCommonCookieT));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        clHeapFree(pCookie);
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }
    clLogStreamKeyDestroy(pStreamKey);

    rc = clLogStreamOwnerStreamHdlrEntryAdd(pStreamOwnerData, nodeAddr, compId,
                                            handlerFlags);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        clHeapFree(pCookie);
        return rc;
    }
    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
       rc = clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, pStreamName, 
                                             pStreamScopeNode,
                                             pStreamOwnerData);  
    }    
    else
    {
        rc = clLogStreamOwnerLocalCheckpoint(pSoEoEntry, pStreamName,
                                             pStreamScopeNode, 
                                             pStreamOwnerData);
    }    

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    
    /*
     * After failover from master, deputy loses node death event, by that time
     * deputy has not been recovered its state by reading checkpoint.
     * found here, and cleaning up the entry. This is only for master.
     */
    if( 0 != pCookie->nodeAddr )
    {
        clLogStreamOwnerNodedownStateUpdate(pCookie->nodeAddr);
    }
    clHeapFree(pCookie);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

ClRcT
VDECL_VER(clLogStreamOwnerHandlerDeregister, 4, 0, 0)(
                                  ClNameT                   *pStreamName,
                                  ClLogStreamScopeT         streamScope,
                                  ClNameT                   *pStreamScopeNode,
                                  ClLogStreamHandlerFlagsT  handlerFlags,
                                  ClIocNodeAddressT         nodeAddr,
                                  ClUint32T                 compId)
{
    ClRcT                  rc                = CL_OK;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntHandleT           hStreamTable      = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pCommonEoEntry   = NULL;
    ClLogStreamKeyT        *pStreamKey       = NULL;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClLogSOCommonCookieT   *pCookie          = NULL;
    ClCntNodeHandleT       hStreamOwnerNode  = CL_HANDLE_INVALID_VALUE;
    ClUint32T              tableSize         = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode),
            CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }    
    if( CL_FALSE == pSoEoEntry->status )
    {
        CL_LOG_DEBUG_ERROR(("Log Server is shutting down..."));
        return CL_OK;
    }
    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }

    hStreamTable = (CL_LOG_STREAM_GLOBAL == streamScope)
                   ? pSoEoEntry->hGStreamOwnerTable 
                   : pSoEoEntry->hLStreamOwnerTable ;
    rc = clCntDataForKeyGet(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamOwnerData);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    rc = clCntNodeFind(hStreamTable, (ClCntKeyHandleT) pStreamKey,
                       (ClCntNodeHandleT *) &hStreamOwnerNode);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }    
    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    rc = clLogStreamOwnerStreamHdlrEntryDelete(pStreamOwnerData, nodeAddr,
                                               compId, handlerFlags, &tableSize);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }

    if( 0 == tableSize )
    {
        /*
         * Last deregister, Delete the node, no need to propagate to other
         * servers, by this time they might have closed this stream
         */
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        CL_LOG_CLEANUP(
                clLogStreamOwnerEntryChkNDelete(pSoEoEntry, streamScope, 
                                                hStreamOwnerNode),
                CL_OK);
        return rc;
    }

    pCookie = clHeapCalloc(1, sizeof(ClLogSOCommonCookieT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    pCookie->sendFilter   = CL_FALSE;
    pCookie->pStreamKey   = pStreamKey;
    pCookie->handlerFlags = handlerFlags;
    pCookie->setFlags     = CL_FALSE;
    rc = clCntWalk(pStreamOwnerData->hCompTable, clLogStreamOwnerSvrIntimate, 
                   (ClCntArgHandleT) pCookie, sizeof(ClLogSOCommonCookieT));
    if( CL_OK != rc )
    {
        clHeapFree(pCookie);
        clLogStreamKeyDestroy(pStreamKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }
    clHeapFree(pCookie);
    clLogStreamKeyDestroy(pStreamKey);

#if 0
    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
       rc = clLogStreamOwnerGlobalCheckpoint(pSoEoEntry, pStreamName, 
                                             pStreamScopeNode,
                                             pStreamOwnerData);  
    }    
    else
    {
        rc = clLogStreamOwnerLocalCheckpoint(pSoEoEntry, pStreamName,
                                             pStreamScopeNode, 
                                             pStreamOwnerData);
    }    
#endif
    
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return  rc;
}

ClRcT
clLogStreamOwnerTableWalk(ClCntHandleT       hStreamTable,
                          ClLogStreamScopeT  streamScope,
                          ClIocNodeAddressT  nodeAddr,
                          ClUint32T          compId)
{
    ClRcT                  rc                = CL_OK;
    ClLogCompKeyT          compKey           = {0};
    ClLogSvrCommonEoDataT  *pCommonEoData    = NULL;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;
    ClCntNodeHandleT       hSvrStreamNode    = CL_HANDLE_INVALID_VALUE;
    ClCntDataHandleT       hNextNode         = CL_HANDLE_INVALID_VALUE;
    ClLogStreamOwnerDataT  *pStreamOwnerData = NULL;
    ClUint32T              *pData            = NULL;
    ClUint32T              size              = 0;
    ClLogStreamKeyT       *pStreamKey        = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamOwnerEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSOLock(pSoEoEntry, streamScope); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSOLock(): rc[0x %x]", rc));
        return rc;
    }    
    compKey.nodeAddr = nodeAddr;
    compKey.compId   = compId;
    compKey.hash     = nodeAddr % pCommonEoData->maxComponents;

    rc = clCntFirstNodeGet(hStreamTable, &hSvrStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("clCntFirstNodeGet(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
        return rc;
    }
    while( CL_HANDLE_INVALID_VALUE != hSvrStreamNode )
    {
        hNextNode = CL_HANDLE_INVALID_VALUE;

        rc = clCntNodeUserDataGet(hStreamTable, hSvrStreamNode,
                (ClCntDataHandleT *) &pStreamOwnerData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_TRACE(("clCntNodeUserDataGet(); rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
            return rc;
        }
        rc = clCntNodeUserKeyGet(hStreamTable, hSvrStreamNode, 
                                 (ClCntDataHandleT *) &pStreamKey);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_TRACE(("clCntNodeUserKeyGet(); rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
            return rc;
        }
        clCntNextNodeGet(hStreamTable, hSvrStreamNode, &hNextNode);

        rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
            return rc;
        }

        rc = clCntDataForKeyGet(pStreamOwnerData->hCompTable, 
                (ClCntKeyHandleT) &compKey, 
                (ClCntDataHandleT *) &pData);
        if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
        {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
            hSvrStreamNode = hNextNode;
            continue;
        }
        if( CL_LOG_NODE_STATUS_WIP == pStreamOwnerData->nodeStatus )
        {
            *pData = 0;
        }
        else
        {
            rc = clCntAllNodesForKeyDelete(pStreamOwnerData->hCompTable,
                    (ClCntKeyHandleT) &compKey);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]",
                            rc));
            }
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        rc = clCntSizeGet(pStreamOwnerData->hCompTable, &size);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]\n", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
            return rc;
        }
        if( size == 0)
        {
            rc = clCntNodeDelete(hStreamTable, hSvrStreamNode);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]\n", rc));
                CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
                return rc;

            }
        }
        else
        {
            rc = clLogStreamOwnerCheckpoint(pSoEoEntry, streamScope,
                                            hSvrStreamNode, pStreamKey);
            if( CL_OK != rc )
            {
                CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);
                return rc;
            }
        }
        hSvrStreamNode = hNextNode;
    }

    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, streamScope), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}
    
ClRcT
clLogStreamOwnerCompdownStateUpdate(ClIocNodeAddressT  nodeAddr, 
                                    ClUint32T          compId)
{
    ClLogSOEoDataT     *pSoEoEntry = NULL;
    ClRcT              rc          = CL_OK;
    ClLogStreamScopeT  streamScope = CL_LOG_STREAM_GLOBAL; 

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamOwnerEoEntryGet(): rc[0x %x]",
                        rc));
        return rc;
    }    

    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hGStreamOwnerTable )
    {
        rc = clLogStreamOwnerTableWalk(pSoEoEntry->hGStreamOwnerTable, 
                                       streamScope, nodeAddr, compId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogStreamOwnerTableWalk() GLOBAL: rc[0x %x]",
                            rc));
            return rc;
        }
    }
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hLStreamOwnerTable )
    {
        streamScope = CL_LOG_STREAM_LOCAL;
        rc = clLogStreamOwnerTableWalk(pSoEoEntry->hLStreamOwnerTable, 
                                       streamScope, nodeAddr, compId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogStreamOwnerTableWalk() LOCAL: rc[0x %x]",
                            rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSOTableNodeDeathWalk(ClCntKeyHandleT   key,
                          ClCntDataHandleT  data,
                          ClCntArgHandleT   arg,
                          ClUint32T         size)
{
    ClRcT                  rc                = CL_OK;
    ClLogCompKeyT          *pCompKey         = NULL;
    ClCntNodeHandleT       hNode             = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT       hNextNode         = CL_HANDLE_INVALID_VALUE;
    ClLogStreamKeyT        *pStreamKey       = (ClLogStreamKeyT *) key;
    ClLogStreamOwnerDataT  *pStreamOwnerData = (ClLogStreamOwnerDataT *) data;
    ClLogStreamScopeT      streamScope       = 0;
    ClLogSOEoDataT         *pSoEoEntry       = NULL;

    ClIocNodeAddressT nodeAddr = (ClIocNodeAddressT) (ClWordT) arg;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogStreamScopeGet(&pStreamKey->streamScopeNode, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCntFirstNodeGet(pStreamOwnerData->hCompTable, &hNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("clCntFirstNodeGet(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
        return rc;
    }
    while( CL_HANDLE_INVALID_VALUE != hNode )
    {
        hNextNode = CL_HANDLE_INVALID_VALUE;
        rc = clCntNodeUserKeyGet(pStreamOwnerData->hCompTable, hNode, 
                                 (ClCntKeyHandleT *) &pCompKey);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);
            return rc;
        }
        /* 
         *  Explicitly not checking error code, coz if it is only one node, 
         *  this call will fail, but we have to delete the node
         */  
        clCntNextNodeGet(pStreamOwnerData->hCompTable, hNode, &hNextNode);

        if( nodeAddr == pCompKey->nodeAddr )
        {
            CL_LOG_CLEANUP(
                clCntNodeDelete(pStreamOwnerData->hCompTable, hNode),
                CL_OK);
            CL_LOG_DEBUG_TRACE(("NodeAddress %d is getting deleted", nodeAddr));
        }
        hNode = hNextNode;
    }
#if 0
    clLogStreamOwnerCheckpoint(pSoEoEntry, streamScope, hStreamOwnerNode, 
                               pStreamKey);
#endif

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerNodedownStateUpdate(ClIocNodeAddressT  nodeAddr) 
{
    ClLogSOEoDataT  *pSoEoEntry = NULL;
    ClRcT           rc          = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }    
    /* 
     * If table is not there, no need to walk inside to do anything
     * this is also another way of telling that this node is not master or
     * deputy to deal with Global table 
     */
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hGStreamOwnerTable )
    {
        rc = clLogSOLock(pSoEoEntry, CL_LOG_STREAM_GLOBAL);
        if( CL_OK != rc )
        {
            return rc;
        }
        CL_LOG_DEBUG_TRACE(("Node Address %d is going down", nodeAddr));
        rc = clCntWalk(pSoEoEntry->hGStreamOwnerTable, clLogSOTableNodeDeathWalk, 
                (ClCntArgHandleT)(ClWordT)(ClLogHandleT) nodeAddr, 
                sizeof(nodeAddr));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_GLOBAL), CL_OK);
            return rc;
        }
        CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_GLOBAL), CL_OK);
    }

    rc = clLogSOLock(pSoEoEntry, CL_LOG_STREAM_LOCAL);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hLStreamOwnerTable )
    {
        rc = clCntWalk(pSoEoEntry->hLStreamOwnerTable, clLogSOTableNodeDeathWalk, 
                      (ClCntArgHandleT)(ClWordT)nodeAddr, sizeof(nodeAddr));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_LOCAL), CL_OK);
            return rc;
        }
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_LOCAL), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

#if 0
ClRcT
clLogStreamOwnerHdlrCntUpdate(ClLogStreamOwnerDataT     *pStreamOwnerData,
                              ClIocNodeAddressT         nodeAddr, 
                              ClLogStreamHandlerFlagsT  handlerFlags,
                              ClBoolT                   setFlags)
{
    ClRcT      rc        = CL_OK;
    ClUint64T  ackerCnt  = 0;
    ClUint64T  ackerByte = 0;
    ClUint64T  nodeByte  = 0;
    ClUint64T  temp      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    if( CL_TRUE == setFlags )
    {
        if( CL_LOG_HANDLER_WILL_ACK == handlerFlags )
        {
            ackerCnt = pStreamOwnerData->ackerCnt;
            nodeByte = ackerCnt >> 48;
            if( !(nodeByte & (1 << (nodeAddr - 1))) )
            {
                nodeByte =  nodeByte | (1 << (nodeAddr -1));
            }
            ackerByte = ackerCnt & (0xF << ((nodeAddr - 1) * 4));
            ++ackerByte;
            nodeByte  = nodeByte << 48;
            ackerCnt  = ackerByte | nodeByte;
            pStreamOwnerData->ackerCnt = ackerCnt; 
        }
        else
        {
            ackerCnt = pStreamOwnerData->nonAckerCnt;
            nodeByte = ackerCnt >> 48;
            if( !(nodeByte & (1 << (nodeAddr - 1))) )
            {
                nodeByte =  nodeByte | (1 << (nodeAddr -1));
            }
            ackerByte = ackerCnt & (0xF << ((nodeAddr - 1) * 4));
            ++ackerByte;
            nodeByte  = nodeByte << 48;
            ackerCnt  = ackerByte | nodeByte;
            pStreamOwnerData->nonAckerCnt = ackerCnt; 
        }
    }
    else
    {
        if( CL_LOG_HANDLER_WILL_ACK == handlerFlags )
        {
            ackerCnt = pStreamOwnerData->ackerCnt;
            nodeByte = ackerCnt >> 48;
            if( !(nodeByte & (1 << (nodeAddr - 1))) )
            {
                CL_LOG_DEBUG_ERROR(("No more deregister"));
                return CL_OK;
            }
            ackerByte = ackerCnt & (0xF << ((nodeAddr - 1) * 4));
            temp = ackerByte - 1;
            if( temp == 0 )
            {
                nodeByte = ~(nodeByte << 48);
                ackerCnt &= nodeByte;
            }
            ackerCnt  &= (ackerByte ^ (~temp));
            pStreamOwnerData->ackerCnt = ackerCnt; 
        }
        else
        {
            ackerCnt = pStreamOwnerData->nonAckerCnt;
            nodeByte = ackerCnt >> 48;
            if( !(nodeByte & (1 << (nodeAddr - 1))) )
            {
                CL_LOG_DEBUG_ERROR(("No more deregister"));
                return CL_OK;
            }
            ackerByte = ackerCnt & (0xF << ((nodeAddr - 1) * 4));
            temp = ackerByte - 1;
            if( temp == 0 )
            {
                nodeByte = ~(nodeByte << 48);
                ackerCnt &= nodeByte;
            }
            ackerCnt  &= (ackerByte ^ (~temp));
            pStreamOwnerData->nonAckerCnt = ackerCnt; 
        }
    }
    CL_LOG_DEBUG_ERROR(("Acker cnt: %llx  NonAckerCnt: %llx",
                pStreamOwnerData->ackerCnt, pStreamOwnerData->nonAckerCnt));
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerHdlrCntClear(ClLogStreamOwnerDataT  *pStreamOwnerData,
                             ClIocNodeAddressT      nodeAddr)
{
    ClRcT      rc        = CL_OK;
    ClUint64T  ackerCnt  = 0;
    ClUint64T  ackerByte = 0;
    ClUint64T  nodeByte  = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    ackerCnt = pStreamOwnerData->ackerCnt;
    nodeByte = ackerCnt >> 48;
    if( (nodeByte & (1 << (nodeAddr - 1))) )
    {
        ackerByte = ackerCnt & (~(0xF << ((nodeAddr - 1) * 4)));
        nodeByte  = (nodeByte & ~(1 << (nodeAddr -1)));
        ackerCnt  = ackerCnt & ackerByte;
        nodeByte  = nodeByte << 48;
        ackerCnt  = ackerCnt & (~nodeByte);
        pStreamOwnerData->ackerCnt = ackerCnt;
    }
    ackerCnt = pStreamOwnerData->nonAckerCnt;
    nodeByte = ackerCnt >> 48;
    if( (nodeByte & (1 << (nodeAddr - 1))) )
    {
        ackerByte = ackerCnt & (~(0xF << ((nodeAddr - 1) * 4)));
        nodeByte  = (nodeByte & ~(1 << (nodeAddr -1)));
        ackerCnt  = ackerCnt & ackerByte;
        nodeByte  = nodeByte << 48;
        ackerByte = ackerCnt & (~nodeByte);
        pStreamOwnerData->nonAckerCnt = ackerCnt;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
#endif
