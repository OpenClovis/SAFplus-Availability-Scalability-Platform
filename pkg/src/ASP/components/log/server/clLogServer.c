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
#include <clXdrApi.h>
#include <clCkptApi.h>
#include <clHandleApi.h>
#include <ipi/clHandleIpi.h>
#include <clCpmExtApi.h>

#include <clLogErrors.h>
#include <clLogCommon.h>

#include <clLogServer.h>
#include <clLogSvrEo.h>
#include <clLogSvrCommon.h>
#include <clLogFlusher.h>
#include <clLogGms.h>
#include <clLogSvrCkpt.h>
#include <clLogFileEvt.h>
#include <clLogStreamOwnerCkpt.h>
#include <clLogFileOwnerEo.h>
#include <clLogMasterCkpt.h>
#include <clLogMaster.h>
#include <LogServer.h>
#include <LogPortStreamOwnerClient.h>
#include <LogPortMasterClient.h>
#include <LogPortSvrServer.h>
#include <AppclientPortclientClient.h>
#include <xdrClLogCompDataT.h>
#include <clLogOsal.h>

static ClRcT
clLogSvrCompEntryAdd(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                     CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                     CL_IN  ClLogSvrStreamDataT    *pSvrStreamData,
   	                 CL_IN  ClUint32T              componentId,
                     CL_IN  ClIocPortT             portId);

static ClRcT
clLogSvrCompRefCountIncrement(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                              CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                              CL_IN  ClCntNodeHandleT       hSvrStreamNode,
			                  CL_IN  ClUint32T              componentId,
                              CL_IN  ClIocPortT             portId);

static ClRcT
clLogSvrCompEntrySearch(CL_IN  ClLogSvrEoDataT     *pSvrEoEntry,
                        CL_IN  ClCntNodeHandleT    svrStreamNode,
                        CL_IN  ClUint32T           componentId);


static ClRcT
clLogSvrCompRefCountDecrement(CL_IN   ClLogSvrEoDataT     *pSvrEoEntry,
                              CL_IN   ClCntNodeHandleT    svrStreamNode,
                              CL_IN   ClUint32T           componentId,
                              CL_OUT  ClUint16T           *pTableStatus);

static ClRcT
clLogSvrFlusherCheckNStart(ClLogSvrEoDataT         *pSvrEoEntry,
                           ClCntNodeHandleT        hSvrStreamNode,
                           ClUint16T               *pStreamId,
                           ClUint32T               componentId,
                           ClIocMulticastAddressT  *pStreamMcastAddr,
                           ClLogFilterT            *pStreamFilter,
                           ClLogStreamAttrIDLT     *pStreamAttr,
                           ClStringT               *pShmName,
                           ClUint32T               *pShmSize);

static ClRcT
clLogSvrShmAndFlusherClose(ClLogSvrStreamDataT    *pSvrStreamData);

static ClRcT
clLogSvrFlusherClose(CL_IN ClLogSvrStreamDataT  *pSvrStreamData);


static ClRcT
clLogSvrSOStreamClose(ClNameT            *pStreamName,
                      ClLogStreamScopeT  streamScope,
                      ClNameT            *pStreamScopeNode,
                      ClUint32T          compId,
                      ClIocNodeAddressT  localAddress);


static ClRcT
clLogSvrClientIdlHandleInitialize(ClIocPortT    portId,
                                  ClIdlHandleT  *phLogIdl);

static ClRcT
clLogSvrFilterSetClientInformCb(ClCntKeyHandleT   key,
                               ClCntDataHandleT  data,
                               ClCntArgHandleT   arg,
                               ClUint32T         size);

static ClRcT
clLogSvrStdStreamShmCreate(ClStringT               *pShmName,
                           ClUint32T               shmSize,
                           ClLogStreamAttrIDLT     *pStreamAttr,
                           ClUint16T               streamId,
                           ClIocMulticastAddressT  *pStreamMcastAddr,
                           ClLogFilterT            *pFilter,
                           ClLogStreamHeaderT      **ppStreamHeader);
static ClRcT
clLogSvrMasterCompListUpdate(void);

static ClRcT
clLogSvrLocalFileOwnerNFlusherStart(void);

static ClRcT 
clLogSvrStreamEntryUpdate(ClLogSvrEoDataT         *pSvrEoEntry,
                          ClNameT                 *pStreamName,
                          ClNameT                 *pStreamScopeNode,
                          ClCntNodeHandleT        hSvrStreamNode,
                          ClUint32T               compId, 
			    	      ClLogStreamAttrIDLT     *pStreamAttr,
                          ClIocMulticastAddressT  streamMcastAddr,
                          ClLogFilterT            *pStreamFilter,
                          ClUint32T               ackerCnt, 
                          ClUint32T               nonAckerCnt, 
			   		      ClUint16T				  streamId,
                          ClStringT               *pShmName,
                          ClUint32T               *pShmSize);
static  ClRcT
clLogSvrInitialEntryAdd(ClLogSvrEoDataT        *pSvrEoEntry,
                        ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                        ClNameT                *pStreamName,
                        ClNameT                *pStreamScopeNode,
                        ClUint32T              compId,
                        ClIocPortT             portId,
                        ClBoolT                addCompEntry,
                        ClCntNodeHandleT       *phSvrStreamNode);
static ClRcT
clLogLocalFlusherSetup(ClNameT              *pStreamName,
                       ClNameT              *pStreamScopeNode, 
                       ClLogStreamAttrIDLT  *pStreamAttr);

ClRcT
clLogSvrPrecreatedStreamsOpen(void);

/******************************** Log Svr *************************************/

ClRcT
clLogFlusherHdlDeleteCb(ClHandleDatabaseHandleT  hFlusherDB, 
                        ClHandleT                hFlusher,
                        ClPtrT                   pCookie)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    CL_LOG_CLEANUP(clLogFlusherCookieHandleDestroy(hFlusher, CL_FALSE),
                    CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrEoDataFree(void)
{
    ClLogSvrEoDataT  *pSvrEoEntry = NULL;
    ClRcT            rc           = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_CLEANUP(clHandleWalk(pSvrEoEntry->hFlusherDB, 
                                clLogFlusherHdlDeleteCb, 
                                NULL), CL_OK);
    if( CL_HANDLE_INVALID_VALUE != pSvrEoEntry->hFlusherDB )
    {
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(pSvrEoEntry->hFlusherDB),
                CL_OK);
    }
    
    /*
     * Dont free the log server eo entry as it could be parallely in used
     clHeapFree(pSvrEoEntry);
    */
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrBootup(ClBoolT  *pLogRestart)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClEoExecutionObjT      *pEoObj            = NULL;
    ClLogFileOwnerEoDataT  *pFileEoEntry      = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrCommonEoDataGet(): rc[0x %x]", rc));
        return rc;
    }

    pSvrEoEntry = clHeapCalloc(1, sizeof(ClLogSvrEoDataT));
    if( NULL == pSvrEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clLogSvrEoDataInit(pSvrEoEntry, pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoDataInit(): rc[0x %x]", rc));
        clHeapFree(pSvrEoEntry);
        return rc;
    }

    rc = clEoPrivateDataSet(pEoObj, CL_LOG_SERVER_EO_ENTRY_KEY, pSvrEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        clHeapFree(pSvrEoEntry);
        return rc;
    }

    rc = clLogSvrCkptGet(pSvrEoEntry, pSvrCommonEoEntry, pLogRestart);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogCkptLibInit(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        clHeapFree(pSvrEoEntry);
        return rc;
    }

    /* Initialize the fileowner here itself */
    rc = clLogFileOwnerEoDataInit(&pFileEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrEoDataFinalize(), CL_OK);
        clHeapFree(pSvrEoEntry);
        return rc;
    }
    pFileEoEntry->status = CL_LOG_FILEOWNER_STATE_INACTIVE;
    /*
     * Avoiding log loosing in the initial phase 
     */ 
        /*
         * If it is restart, no need to parse the xml and do anything, 
         * it is required in case of fresh start
         */
    clLogSvrLocalFileOwnerNFlusherStart();

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrStreamTableGet(CL_IN ClLogSvrEoDataT  *pSvrEoEntry)
{
    ClRcT   rc = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE == pSvrEoEntry->hSvrStreamTable)
    {
        rc = clCntHashtblCreate(pSvrCommonEoEntry->maxStreams,
                                clLogStreamKeyCompare, clLogStreamHashFn,
                                clLogSvrStreamEntryDeleteCb,
                                clLogSvrStreamEntryDeleteCb, CL_CNT_UNIQUE_KEY,
                                &(pSvrEoEntry->hSvrStreamTable));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntHashtblCreate(): rc[0x %x]", rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClInt32T
clLogSvrCompKeyCompare(ClCntKeyHandleT key1,
                       ClCntKeyHandleT key2)
{
    ClLogSvrCompKeyT  *pKey1 = (ClLogSvrCompKeyT *) key1;
    ClLogSvrCompKeyT  *pKey2 = (ClLogSvrCompKeyT *) key2;

    CL_LOG_DEBUG_TRACE(("Enter: %u %u", pKey1->componentId, pKey2->componentId));

    return (pKey1->componentId - pKey2->componentId);
}

ClUint32T
clLogSvrCompHashFn(ClCntKeyHandleT    userKey)
{
    ClLogSvrCompKeyT  *pKey = (ClLogSvrCompKeyT *) userKey;

    CL_LOG_DEBUG_TRACE(("Enter: %u", pKey->componentId));

    return (pKey->hash);
}

void
clLogSvrCompDeleteCb(ClCntKeyHandleT   userKey,
                     ClCntDataHandleT  userData)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree((void *) userData);
    clHeapFree((void *) userKey);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogSvrSOSOResponseCleanup(ClLogSvrEoDataT  *pSvrEoEntry,
                            ClCntNodeHandleT  hSvrStreamNode,
                            ClUint32T         compId)
{
    ClRcT  rc = CL_OK;
    ClBoolT  tableEmpty = CL_FALSE;
    
    rc = clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId, &tableEmpty);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrCompRefCountDecrement(): rc[0x %x]", rc));
    }

    if( CL_TRUE == tableEmpty )
    {
        rc = clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
        }
    }
    
    return rc;
}

ClRcT
clLogSvrSOSOResponseProcess(ClNameT                 *pStreamName,
                            ClLogStreamScopeT       *pStreamScope,
			    	    	ClNameT                 *pStreamScopeNode,
					     	ClUint32T        		*pCompId,
			    	     	ClLogStreamAttrIDLT     *pStreamAttr,
                       		ClIocMulticastAddressT  *pStreamMcastAddr,
			   		 		ClLogFilterT 			*pStreamFilter,
                            ClUint32T               ackerCnt, 
                            ClUint32T               nonAckerCnt, 
			   		 		ClUint16T				*pStreamId,
					        ClRcT 					retCode,
                            ClStringT               *pShmName,
                            ClUint32T               *pShmSize)
{
    ClRcT                rc              = CL_OK;
    ClLogSvrEoDataT      *pSvrEoEntry    = NULL;
    ClCntNodeHandleT     hSvrStreamNode  = CL_HANDLE_INVALID_VALUE;
    ClBoolT              addedEntry      = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode, 
                                CL_FALSE, &hSvrStreamNode, &addedEntry);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return rc;
    }

    rc = clLogSvrCompEntrySearch(pSvrEoEntry, hSvrStreamNode, *pCompId);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return rc;
    }
	if(CL_OK != retCode )
	{
        CL_LOG_CLEANUP(clLogSvrSOSOResponseCleanup(pSvrEoEntry, hSvrStreamNode,
                                                   *pCompId), CL_OK);
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return retCode;
	}
    
    rc = clLogSvrStreamEntryUpdate(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                   hSvrStreamNode, *pCompId, pStreamAttr, 
                                   *pStreamMcastAddr, pStreamFilter, ackerCnt,
                                   nonAckerCnt, *pStreamId, pShmName,
                                   pShmSize);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrSOSOResponseCleanup(pSvrEoEntry, hSvrStreamNode,
                                                   *pCompId), CL_OK);
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return retCode;
    }

    rc = clLogSvrStreamCheckpoint(pSvrEoEntry, hSvrStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrSOSOResponseCleanup(pSvrEoEntry, hSvrStreamNode,
                                                   *pCompId), CL_OK);
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(pShmName), CL_OK);
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return rc;
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogSvrSOSOResponse(CL_OUT     ClIdlHandleT            hLogIdl,
                     CL_OUT     ClLogStreamOpenFlagsT   streamOpenFlags,
                     CL_OUT     ClIocNodeAddressT       nodeAddr,
			    	 CL_INOUT	ClNameT                 *pStreamName,
                     CL_INOUT   ClLogStreamScopeT       *pStreamScope,
			    	 CL_INOUT	ClNameT                 *pStreamScopeNode,
					 CL_INOUT  	ClUint32T        		*pCompId,
			    	 CL_INOUT  	ClLogStreamAttrIDLT     *pStreamAttr,
                     CL_IN 		ClIocMulticastAddressT  *pStreamMcastAddr,
			   		 CL_IN		ClLogFilterT 			*pStreamFilter,
                     CL_IN      ClUint32T               *pAckerCnt, 
                     CL_IN      ClUint32T               *pNonAckerCnt, 
			   		 CL_IN		ClUint16T				*pStreamId,
					 CL_IN      ClRcT 					retCode,
					 CL_IN      ClPtrT 					cookie)
{
    ClRcT                   rc       = CL_OK;
    ClStringT               shmName  = {0};
    ClUint32T               shmSize  = 0;
    ClLogStreamOpenCookieT  *pCookie = (ClLogStreamOpenCookieT *) cookie;

    CL_LOG_DEBUG_TRACE(("Enter: retCode: %x", retCode));

    CL_ASSERT(NULL != pCookie);
    rc = clLogSvrSOSOResponseProcess(pStreamName, pStreamScope, 
                                     pStreamScopeNode, pCompId, 
                                     pStreamAttr, pStreamMcastAddr, pStreamFilter, 
                                     *pAckerCnt, *pNonAckerCnt, pStreamId, 
                                     retCode, &shmName, &shmSize);
    if( CL_OK != retCode )
    {
        rc = retCode;
    }
    if( CL_LOG_DEFAULT_COMPID != pCookie->compId )
    {
        clLogDebug("SVR", "OPE", "Sending response back to client rc [0x %x] for stream [%.*s]", 
                retCode, pStreamName->length, pStreamName->value);
        if( rc == CL_OK )
        {
            rc = VDECL_VER(clLogSvrStreamOpenResponseSend, 4, 0, 0)(pCookie->hDeferIdl, rc, 
                                                shmName, shmSize);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrStreamOpenResponseSend, 4, 0, 0)(): rc[0x %x]", rc));
            }   
        }
        else
        {
            shmName.length = 0;
            shmName.pValue = NULL;
            rc = VDECL_VER(clLogSvrStreamOpenResponseSend, 4, 0, 0)(pCookie->hDeferIdl, rc, 
                                                shmName, 0);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrStreamOpenResponseSend, 4, 0, 0)(): rc[0x %x]", rc));
            }   
        }
    }
    else
    {
       clHeapFree(shmName.pValue);
       clLogSvrMasterCompListUpdate();
    }

    /*
     * Idl Asyn callback has allocated memory for the following 
     * varibles, so its users responsilbilty to free the memory, 
     * if the Callback retCode is CL_OK. 
     */
    if( retCode == CL_OK )
    {
        if( NULL != pStreamAttr->fileName.pValue )
        {
            clHeapFree(pStreamAttr->fileName.pValue);
            pStreamAttr->fileName.pValue     = NULL;
        }
        if( NULL != pStreamAttr->fileLocation.pValue )
        {
            clHeapFree(pStreamAttr->fileLocation.pValue);
            pStreamAttr->fileLocation.pValue = NULL;
        }
        if( NULL != pStreamFilter->pMsgIdSet )
        {
            clHeapFree(pStreamFilter->pMsgIdSet);
            pStreamFilter->pMsgIdSet = NULL;
        }
        if( NULL != pStreamFilter->pCompIdSet )
        {
            clHeapFree(pStreamFilter->pCompIdSet);
            pStreamFilter->pCompIdSet = NULL;
        }
    }
    clHeapFree(pCookie);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

static ClRcT
clLogSvrCompEntrySearch(ClLogSvrEoDataT     *pSvrEoEntry,
                        ClCntNodeHandleT    hSvrStreamNode,
                        ClUint32T           componentId)
{
    ClRcT                   rc                = CL_OK;
    ClLogSvrStreamDataT     *pSvrStreamData   = NULL;
    ClCntNodeHandleT        hCompNode         = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCompKeyT        svrCompKey        = {0};
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
     }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    svrCompKey.componentId = componentId;
    svrCompKey.hash        = componentId % pSvrCommonEoEntry->maxComponents;
    rc = clCntNodeFind(pSvrStreamData->hComponentTable,
                       (ClCntKeyHandleT) &svrCompKey, &hCompNode);

    CL_LOG_DEBUG_TRACE(("Exit: rc [0x %x]", rc));
    return rc;
}

static ClRcT
clLogSvrCompRefCountDecrement(ClLogSvrEoDataT   *pSvrEoEntry,
                              ClCntNodeHandleT  hSvrStreamNode,
                              ClUint32T         componentId,
                              ClBoolT           *pTableEmpty)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCompDataT      *pCompData         = NULL;
    ClLogSvrCompKeyT       svrCompKey         = {0};
    ClUint32T              tableSize          = 0;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
     }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }

    svrCompKey.componentId = componentId;
    svrCompKey.hash        = componentId % pSvrCommonEoEntry->maxComponents;
    rc = clCntDataForKeyGet(pSvrStreamData->hComponentTable,
                            (ClCntKeyHandleT) &svrCompKey,
                            (ClCntDataHandleT *) &pCompData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc[0x %x]", rc));
        return rc;
    }

    pCompData->refCount--;
    if( 0 == pCompData->refCount )
    {
        rc = clCntAllNodesForKeyDelete(pSvrStreamData->hComponentTable,
                                       (ClCntKeyHandleT) &svrCompKey);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]", rc));
            return rc;
        }

        rc = clCntSizeGet(pSvrStreamData->hComponentTable, &tableSize);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
            return rc;
        }

        if( 0 == tableSize )
        {
            *pTableEmpty = CL_TRUE;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrShmAndFlusherClose(ClLogSvrStreamDataT  *pSvrStreamData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrFlusherClose(pSvrStreamData);

    if( (rc == CL_OK) && (NULL != pSvrStreamData->pStreamHeader) )
    {
        CL_LOG_DEBUG_TRACE(("Cleaning up the shared memory ...%p",
                (ClPtrT) pSvrStreamData->pStreamHeader));
#ifdef VXWORKS_BUILD
        clLogServerStreamMutexDestroy(pSvrStreamData);
        clLogServerStreamFlusherMutexDestroy(pSvrStreamData);
#endif
        rc = clOsalMunmap_L(pSvrStreamData->pStreamHeader,
                pSvrStreamData->pStreamHeader->shmSize);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalMunmap(): rc[0x %x]", rc));
        }

        rc = clOsalShmUnlink_L(pSvrStreamData->shmName.pValue);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalShmDelete(): rc[0x %x]", rc));
        }
        pSvrStreamData->pStreamHeader = NULL;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrFlusherCheckNStart(ClLogSvrEoDataT         *pSvrEoEntry,
                           ClCntNodeHandleT        hSvrStreamNode,
                           ClUint16T               *pStreamId,
                           ClUint32T               componentId,
                           ClIocMulticastAddressT  *pStreamMcastAddr,
                           ClLogFilterT            *pStreamFilter,
                           ClLogStreamAttrIDLT     *pStreamAttr,
                           ClStringT               *pShmName,
                           ClUint32T               *pShmSize)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClUint32T              headerSize         = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                            (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_SVR_SHMSIZE_GET(pSvrCommonEoEntry->numShmPages,
                           *pShmSize);
    if( NULL == pSvrStreamData->pStreamHeader )
    {
        if( CL_LOG_DEFAULT_COMPID != componentId )
        {
             rc = clLogShmCreateAndFill(pShmName, *pShmSize, *pStreamId,
                                        componentId, pStreamMcastAddr, pStreamFilter,
                                        pStreamAttr, &pSvrStreamData->pStreamHeader);
           if( CL_OK != rc )
           {   
               return rc;
           }
        }
        else
        {
            rc = clLogSvrStdStreamShmCreate(pShmName, *pShmSize, pStreamAttr,
                                            *pStreamId, pStreamMcastAddr,
                                            pStreamFilter, &pSvrStreamData->pStreamHeader);
            if( CL_OK != rc )
            {
                return rc;
            }
            pSvrStreamData->pStreamHeader->streamMcastAddr.iocMulticastAddress
                = *pStreamMcastAddr;
            pSvrStreamData->pStreamHeader->streamId = *pStreamId;
        }
        headerSize                     =
            CL_LOG_HEADER_SIZE_GET(pSvrCommonEoEntry->maxMsgs,
                    pSvrCommonEoEntry->maxComponents);
        pSvrStreamData->pStreamRecords = 
            ((ClUint8T *) pSvrStreamData->pStreamHeader) + headerSize;
        clLogDebug(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, "Starting the flusher for shm [%.*s]", pSvrStreamData->shmName.length,
                pSvrStreamData->shmName.pValue);
        rc = clLogSvrFlusherThreadCreateNStart(pSvrStreamData, 
                                               &pSvrStreamData->flusherId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrFlusherThreadCreateNStart(): rc[0x %x]", rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrShmNameCreate(ClNameT    *pStreamName,
                      ClNameT    *pStreamScopeNode,
                      ClStringT  *pShmName)
{
    ClRcT       rc      = CL_OK;
    ClUint16T   nChar   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    nChar = pStreamScopeNode->length + pStreamName->length + 7;
                                           /*6 = "/cl_%s_%s\0"*/
    (pShmName)->pValue = clHeapCalloc(nChar, sizeof(ClCharT));
    if( NULL == (pShmName)->pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }
    (pShmName)->length = nChar;
    snprintf(pShmName->pValue, nChar - 1, "/cl_%s_%s", 
                pStreamScopeNode->value, pStreamName->value);
    pShmName->pValue[nChar - 1] = '\0';

    CL_LOG_DEBUG_TRACE(("Exit: shmName: %s", pShmName->pValue));
    return rc;
}

ClRcT
clLogSvrShmNameDelete(ClStringT  *pShmName)
{

    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree(pShmName->pValue);
    pShmName->pValue = NULL;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}


static ClRcT
clLogSvrSOStreamClose(ClNameT            *pStreamName,
                      ClLogStreamScopeT  streamScope,
                      ClNameT            *pStreamScopeNode,
                      ClUint32T          compId,
                      ClIocNodeAddressT  localAddress)
{
    ClRcT         rc      = CL_OK;
    ClIdlHandleT  hLogIdl = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrIdlHandleInitialize(streamScope, &hLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrIdlHandleInitialize(): rc[0x %x]", rc));
        return rc;
    }
    rc = VDECL_VER(clLogStreamOwnerStreamCloseClientAsync, 4, 0, 0)(
            hLogIdl, pStreamName, streamScope, pStreamScopeNode, 
            localAddress, compId, NULL, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogStreamOwnerStreamCloseClientAsync, 4, 0, 0)():"
                          " rc[0x %x]", rc));
    }
    rc = clIdlHandleFinalize(hLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleFinalize(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogSvrStreamClose, 4, 0, 0)(
                    CL_IN  ClNameT            *pStreamName,
                    CL_IN  ClLogStreamScopeT  streamScope,
                    CL_IN  ClNameT            *pStreamScopeNode,
                    CL_IN  ClUint32T          compId)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClCntNodeHandleT       hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClBoolT                tableEmpty         = CL_FALSE;
    ClIocNodeAddressT      localAddress       = 0;
    ClLogStreamKeyT        *pStreamKey        = NULL;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( 0 == (localAddress = clIocLocalAddressGet()) )
    {
        CL_LOG_DEBUG_ERROR(("clIocLocalAddressGet() failed"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        return rc;
    }
    if( CL_FALSE == pSvrEoEntry->logInit )
    {
        CL_LOG_DEBUG_ERROR(("Log Server is not ready to provide the service"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&): rc[0x %x]", rc));
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        return rc;
    }
    
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pSvrCommonEoEntry->maxStreams, &pStreamKey);
    if( rc != CL_OK )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamKeyCreate(): rc[0x %x]", rc));
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        return rc;
    }

    rc = clCntDataForKeyGet(pSvrEoEntry->hSvrStreamTable, (ClCntKeyHandleT)
                            pStreamKey, (ClCntDataHandleT *)&pSvrStreamData);
    if( rc != CL_OK )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeFind(): rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        /*Log the error FIXME*/
        return rc;
    }

    if( 0 == pSvrStreamData->ackersCount + pSvrStreamData->nonAckersCount )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                       CL_OK);
        clLogInfo("LOG", "STM", "Stream didn't get register");
        sleep(2);/* 
                  * This is to avoid loosing records when streamopen, logwrite,
                  * streamclose happens immediately without delay
                  */
        rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&): rc[0x %x]", rc));
            clLogStreamKeyDestroy(pStreamKey);
            clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                                  compId, localAddress);
            return rc;
        }
    }

    rc = clCntNodeFind(pSvrEoEntry->hSvrStreamTable, 
                       (ClCntKeyHandleT) pStreamKey, &hSvrStreamNode);
    if( rc != CL_OK )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeFind(): rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        /* Log the error FIXME*/
        return rc;
    }

    clLogStreamKeyDestroy(pStreamKey);

	rc = clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                                       &tableEmpty);
    if( rc != CL_OK )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrCompRefCountDecrement(): rc[0x %x]", rc));
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
        return rc;
    }
    if( CL_TRUE == tableEmpty )
    {
        rc = clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
            clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
            clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
            return rc;
        }
    }
    else
    {
        rc = clLogSvrStreamCheckpoint(pSvrEoEntry, hSvrStreamNode);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrStreamCheckpoint(): rc[0x %x]", rc));
            clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
            clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                              compId, localAddress);
            return rc;
        }
    }

    rc = clLogSvrSOStreamClose(pStreamName, streamScope, pStreamScopeNode,
                               compId, localAddress);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrSOStreamClose(): rc[0x %x]", rc));
        clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
        return rc;
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

ClRcT
clLogSvrIdlHandleInitialize(CL_IN   ClLogStreamScopeT  streamScope,
                            CL_OUT  ClIdlHandleT       *phLogIdl)
{
    ClRcT          rc         = CL_OK;
    ClIocAddressT  iocAddress = {{0}};

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        rc = clLogMasterAddressGet(&iocAddress);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogMasterAddrGet(): rc[0x %x]", rc));
            return rc;
        }
        clLogDebug("SVR", "ADD", "Current master address [%d]", iocAddress.iocPhyAddress.nodeAddress);
    }
    else
    {
        iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
        iocAddress.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
    }
    rc = clLogIdlHandleInitialize(iocAddress, phLogIdl);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

static ClRcT
clLogSvrCompEntryAdd(ClLogSvrEoDataT        *pSvrEoEntry,
                     ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                     ClLogSvrStreamDataT    *pSvrStreamData,
 	                 ClUint32T              componentId,
                     ClIocPortT             portId)
{
    ClRcT              rc         = CL_OK;
    ClLogSvrCompDataT  *pCompData = NULL;
    ClLogSvrCompKeyT   *pCompKey  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pCompKey = clHeapCalloc(1, sizeof(ClLogSvrCompKeyT));
    if( NULL == pCompKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    pCompData = clHeapCalloc(1, sizeof(ClLogSvrCompDataT));
    if( NULL == pCompData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        clHeapFree(pCompKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    pCompKey->componentId = componentId;
    pCompKey->hash        = componentId % pSvrCommonEoEntry->maxComponents;
    pCompData->refCount   = 1;
    pCompData->portId     = portId;
    rc = clCntNodeAdd(pSvrStreamData->hComponentTable,
                      (ClCntKeyHandleT) pCompKey, (ClCntDataHandleT) pCompData,
                       NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAdd(): rc[0x %x]", rc));
        clHeapFree(pCompData);
        clHeapFree(pCompKey);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrCompRefCountIncrement(ClLogSvrEoDataT        *pSvrEoEntry,
                              ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                              ClCntNodeHandleT       hSvrStreamNode,
			                  ClUint32T              componentId,
                              ClIocPortT             portId)
{
    ClRcT                rc              = CL_OK;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;
    ClLogSvrCompKeyT     svrCompKey      = {0};
    ClLogSvrCompDataT    *pCompData      = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                             (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    
    svrCompKey.componentId = componentId;
    svrCompKey.hash        = componentId % pSvrCommonEoEntry->maxComponents;
    rc = clCntDataForKeyGet(pSvrStreamData->hComponentTable,
                            (ClCntKeyHandleT ) &svrCompKey,
                            (ClCntDataHandleT *) &pCompData);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clLogSvrCompEntryAdd(pSvrEoEntry, pSvrCommonEoEntry,
                                  pSvrStreamData, componentId, portId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrCompEntryAdd(): rc[0x %x]", rc));
        }
    }
    else if( CL_OK == rc )
    {
        pCompData->refCount++;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrStreamEntryAdd(ClLogSvrEoDataT        *pSvrEoEntry,
                       ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                       ClLogStreamKeyT        *pStreamKey,
                       ClStringT              *pShmName,
                       ClCntNodeHandleT       *phSvrStreamNode)
{
    ClRcT                rc              = CL_OK;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pSvrStreamData = clHeapCalloc(1, sizeof(ClLogSvrStreamDataT));
    if( NULL == pSvrStreamData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }

    rc = clCntHashtblCreate(pSvrCommonEoEntry->maxComponents,
                            clLogSvrCompKeyCompare, clLogSvrCompHashFn,
                            clLogSvrCompDeleteCb, clLogSvrCompDeleteCb,
                            CL_CNT_UNIQUE_KEY,
                            &(pSvrStreamData->hComponentTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtblCreate(): rc[0x %x]", rc));
        clHeapFree(pSvrStreamData);
        return rc;
    }

    pSvrStreamData->dsId           = CL_LOG_INVALID_DSID;
    pSvrStreamData->pStreamHeader  = NULL;
    pSvrStreamData->shmName.length = 0;
    pSvrStreamData->shmName.pValue = NULL;
    pSvrStreamData->pStreamRecords = NULL;
    pSvrStreamData->flusherId      = 0;
    rc = clLogServerStreamSharedMutexInit(pSvrStreamData, pShmName);

    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clOsalProcessSharedMutexInit: rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pSvrStreamData->hComponentTable), CL_OK);
        clHeapFree(pSvrStreamData);
        return rc;
    }

    rc = clCntNodeAddAndNodeGet(pSvrEoEntry->hSvrStreamTable, 
                                (ClCntKeyHandleT) pStreamKey,
                                (ClCntDataHandleT) pSvrStreamData, NULL, 
                                phSvrStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pSvrStreamData->hComponentTable), CL_OK);
        CL_LOG_CLEANUP(clLogServerStreamMutexDestroy(pSvrStreamData),CL_OK);
        clHeapFree(pSvrStreamData);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


ClRcT
clLogSvrStreamEntryGet(CL_IN   ClLogSvrEoDataT   *pSvrEoEntry,
		               CL_IN   ClNameT           *pStreamName,
		               CL_IN   ClNameT           *pStreamScopeNode,
                       CL_IN   ClBoolT           createFlag,
		               CL_OUT  ClCntNodeHandleT  *phSvrStreamNode,
                       CL_OUT  ClBoolT           *pAddedEntry)
{
    ClRcT                  rc                 = CL_OK;
    ClLogStreamKeyT        *pStreamKey        = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pAddedEntry = CL_FALSE;
    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pSvrCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogStreamKeyCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntNodeFind(pSvrEoEntry->hSvrStreamTable, 
                       (ClCntKeyHandleT) pStreamKey, phSvrStreamNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        if( CL_TRUE == createFlag )
        {
            ClStringT shmName = {0};
            rc = clLogShmNameCreate(pStreamName,pStreamScopeNode,&shmName);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogStreamEntryAdd(): rc[0x %x]", rc));
                clLogStreamKeyDestroy(pStreamKey);
                return rc;
            }
            rc = clLogSvrStreamEntryAdd(pSvrEoEntry, pSvrCommonEoEntry,
                                        pStreamKey,&shmName,phSvrStreamNode);
            clHeapFree(shmName.pValue);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogStreamEntryAdd(): rc[0x %x]", rc));
                clLogStreamKeyDestroy(pStreamKey);
            }
            else
            {
                *pAddedEntry = CL_TRUE;
            }
 
            return rc;
        }
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogSvrStreamEntryDeleteCb(ClCntKeyHandleT   userKey,
                            ClCntDataHandleT  userData)
{
    ClRcT                rc            = CL_OK;
    ClLogStreamKeyT      *pKey         = (ClLogStreamKeyT *) userKey;
    ClLogSvrStreamDataT  *pStreamData  = (ClLogSvrStreamDataT *) userData;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( NULL == pKey )
    {
        CL_LOG_DEBUG_ERROR(("streamKey is NULL"));
        return;
    }

    if( NULL == pStreamData )
    {
        CL_LOG_DEBUG_ERROR(("streamData is NULL"));
        return;
    }

    if( CL_HANDLE_INVALID_VALUE !=  pStreamData->hComponentTable )
    {
        rc = clCntDelete(pStreamData->hComponentTable);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntDelete(): rc[0x %x]", rc));
        }
        pStreamData->hComponentTable = CL_HANDLE_INVALID_VALUE;
    }

    if( NULL != pStreamData->shmName.pValue )
    {
        rc = clLogSvrShmAndFlusherClose(pStreamData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrShmAndFlusherClose(): rc[0x %x]", rc));
        }
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(&pStreamData->shmName), CL_OK);
        pStreamData->shmName.pValue = NULL;
    }
    if( NULL != pStreamData->fileName.pValue )
        clHeapFree(pStreamData->fileName.pValue);
    if( NULL != pStreamData->fileLocation.pValue )
        clHeapFree(pStreamData->fileLocation.pValue);

    if( CL_LOG_INVALID_DSID != pStreamData->dsId )
    {
        CL_LOG_CLEANUP(clLogSvrCkptDataSetDelete(pStreamData->dsId),
                   CL_OK);
    }
    clHeapFree((ClLogSvrStreamDataT *) userData);
    clLogStreamKeyDestroy((ClLogStreamKeyT *) userKey);
    
    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return ;
}


ClRcT
VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)(
                   CL_IN   ClNameT                *pStreamName,
                   CL_IN   ClLogStreamScopeT      streamScope,
                   CL_IN   ClNameT                *pStreamScopeNode,
                   CL_IN   ClLogStreamAttrIDLT    *pStreamAttr,
                   CL_IN   ClLogStreamOpenFlagsT  streamOpenFlags,
                   CL_IN   ClUint32T              compId,
                   CL_IN   ClIocPortT             portId,
                   CL_OUT  ClStringT              *pShmName,
                   CL_OUT  ClUint32T              *pShmSize)
{
    ClRcT                   rc                 = CL_OK;
    ClLogSvrEoDataT         *pSvrEoEntry       = NULL;
    ClLogSvrCommonEoDataT   *pSvrCommonEoEntry = NULL;
    ClCntNodeHandleT        hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClIdlHandleT            hLogIdl            = CL_HANDLE_INVALID_VALUE;
    ClIdlHandleT            hIdlDefer          = CL_HANDLE_INVALID_VALUE;
    ClIocMulticastAddressT  streamMcastAddr    = CL_IOC_RESERVED_ADDRESS;
    ClIocNodeAddressT       localAddress       = CL_IOC_RESERVED_ADDRESS;
    ClLogFilterT            streamFilter       = {0};
    ClUint16T               streamId           = 0;
    ClBoolT                 tableEmpty         = CL_FALSE;
    ClLogStreamOpenCookieT  *pCookie           = NULL;
    ClLogStreamAttrIDLT     streamAttr         = {{0}};
    ClUint32T               ackerCnt           = 0;
    ClUint32T               nonAckerCnt        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pShmName), CL_LOG_RC(CL_ERR_NULL_POINTER));

    localAddress = clIocLocalAddressGet();
    if( CL_IOC_RESERVED_ADDRESS == localAddress )
    {
        CL_LOG_DEBUG_ERROR(("clIocLocalAddressGet() failed"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_FALSE == pSvrEoEntry->logInit )
    {
        CL_LOG_DEBUG_ERROR(("Log Server is not ready to provide the service"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSvrInitialEntryAdd(pSvrEoEntry, pSvrCommonEoEntry, pStreamName,
                                 pStreamScopeNode, compId, portId, CL_TRUE,
                                 &hSvrStreamNode);
    if( CL_OK != rc )
    {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                                         CL_OK);
            return rc;
    }

    rc = clLogSvrIdlHandleInitialize(streamScope, &hLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(
            clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                                          &tableEmpty), 
            CL_OK);
        if( CL_TRUE == tableEmpty )
        {
            CL_LOG_CLEANUP(
                clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    if( CL_LOG_STREAM_CREATE == streamOpenFlags )
    {
        rc = clLogStreamAttributesCopy(pStreamAttr, &streamAttr, CL_TRUE);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            CL_LOG_CLEANUP(
                    clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                        &tableEmpty), 
                    CL_OK);
            if( CL_TRUE == tableEmpty )
            {
                CL_LOG_CLEANUP(
                        clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                        CL_OK);
            }
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
            return rc;
        }
    }

    pCookie = clHeapCalloc(1, sizeof(ClLogStreamOpenCookieT));
    if( NULL == pCookie )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        clHeapFree(streamAttr.fileName.pValue);
        clHeapFree(streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        CL_LOG_CLEANUP(
            clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                                          &tableEmpty), 
            CL_OK);
        if( CL_TRUE == tableEmpty )
        {
            CL_LOG_CLEANUP(
                clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    if( CL_LOG_DEFAULT_COMPID != compId )
    {
        rc = clLogIdlSyncDefer(&hIdlDefer);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clIdlSyncDefer(): rc[0x %x]", rc));
            clHeapFree(pCookie);
            clHeapFree(streamAttr.fileName.pValue);
            clHeapFree(streamAttr.fileLocation.pValue);
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            CL_LOG_CLEANUP(
                    clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                        &tableEmpty), 
                    CL_OK);
            if( CL_TRUE == tableEmpty )
            {
                CL_LOG_CLEANUP(
                        clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                        CL_OK);
            }
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
            return rc;
        }
        pCookie->hDeferIdl = hIdlDefer;
    }
    pCookie->compId    = compId;

    clLogDebug("SVR", "OPE", "Making call to streamOwner for [%.*s]", 
            pStreamName->length, pStreamName->value);
    rc = VDECL_VER(clLogStreamOwnerStreamOpenClientAsync, 4, 0, 0)(hLogIdl, streamOpenFlags, 
                                               localAddress, pStreamName,
                                               &streamScope, pStreamScopeNode,
                                               &compId, &streamAttr, 
                                               &streamMcastAddr, &streamFilter,
                                               &ackerCnt, &nonAckerCnt, &streamId,
                                               clLogSvrSOSOResponse, (void *) pCookie);
    if( CL_OK != rc )
    {
        clHeapFree(pCookie);
        clHeapFree(streamAttr.fileName.pValue);
        clHeapFree(streamAttr.fileLocation.pValue);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        CL_LOG_CLEANUP(
            clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId,
                                          &tableEmpty), 
            CL_OK);
        if( CL_TRUE == tableEmpty )
        {
            CL_LOG_CLEANUP(
                clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                CL_OK);
        }
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        if( CL_LOG_DEFAULT_COMPID != compId )
        {
            pShmName->length = 0;
            pShmName->pValue = NULL;
            CL_LOG_CLEANUP(VDECL_VER(clLogSvrStreamOpenResponseSend, 4, 0, 0)(hIdlDefer, rc,
                                                      *pShmName, 0),
                       CL_OK);
        }
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


static ClRcT
clLogSvrFlusherClose(CL_IN ClLogSvrStreamDataT  *pSvrStreamData)
{
    ClRcT               rc       = CL_OK;
    ClLogStreamHeaderT  *pHeader = pSvrStreamData->pStreamHeader;
    //ClTimerTimeOutT     time     = {0, 100L};

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    CL_LOG_PARAM_CHK((NULL == pHeader), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogServerStreamMutexLock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&): rc[0x %x]", rc));
        return rc;
    }

    if( (pHeader->streamStatus != CL_LOG_STREAM_THREAD_EXIT) &&
        (pHeader->streamStatus != CL_LOG_STREAM_CLOSE) )
    {
        pHeader->streamStatus  = CL_LOG_STREAM_CLOSE;
        pHeader->flushInterval = 1000000000;
    }
    else
    {
        CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pSvrStreamData), CL_OK);
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }
    
    CL_LOG_DEBUG_TRACE(("signalling for the threadid : %llu\n",
                        pSvrStreamData->flusherId));
#ifndef POSIX_BUILD
    rc = clLogServerStreamSignalFlusher(pSvrStreamData,&pHeader->flushCond);
#else
    rc = clLogServerStreamSignalFlusher(pSvrStreamData, NULL);
#endif
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("CL_LOG_LOCK(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pSvrStreamData), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Joining the thread: %llu ",
                        pSvrStreamData->flusherId));

    if( CL_LOG_STREAM_THREAD_EXIT != pHeader->streamStatus )
    {
        rc = clLogServerStreamMutexUnlock(pSvrStreamData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(): rc[0x %x]", rc));
        }
        rc = clOsalTaskJoin(pSvrStreamData->flusherId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalTaskJoin(): rc[0x %x]", rc));
            return CL_LOG_RC(CL_ERR_INVALID_STATE);
        }
    }
    else
    {
        rc = clLogServerStreamMutexUnlock(pSvrStreamData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(): rc[0x %x]", rc));
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrFlusherThreadCreateNStart(ClLogSvrStreamDataT  *pSvrStreamData,
                                  ClOsalTaskIdT        *pTaskId)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalTaskCreateAttached(pSvrStreamData->shmName.pValue, CL_OSAL_SCHED_OTHER,
                                  CL_OSAL_THREAD_PRI_NOT_APPLICABLE, CL_OSAL_MIN_STACK_SIZE, 
                                  clLogFlusherStart, pSvrStreamData, pTaskId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalJoinableTaskCreate(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrShmMapAndGet(ClOsalShmIdT  fd, 
                     ClUint8T      **ppStreamHeader)
{
    ClRcT                  rc                 = CL_OK;
    ClUint32T              shmSize            = 0;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_SVR_SHMSIZE_GET(pSvrCommonEoEntry->numShmPages, shmSize);

    rc = clOsalMmap_L(0, shmSize, CL_LOG_MMAP_PROT_FLAGS, CL_LOG_MMAP_FLAGS,
                      fd, 0, (void *) ppStreamHeader);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrDebugSeverityGet(ClNameT *pStreamName,
                         ClNameT *pStreamScopeNode,
                         ClLogSeverityFilterT *pSeverityFilter)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       svrStreamNode      = CL_HANDLE_INVALID_VALUE;
    ClBoolT                addedEntry         = CL_FALSE;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClNameT streamScopeNode = {0};
    ClLogSeverityFilterT severityFilter = 0;
    ClInt32T i;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!pStreamName || !pSeverityFilter) return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);

    if(!pStreamScopeNode)
    {
        pStreamScopeNode = &streamScopeNode;
        clCpmLocalNodeNameGet(pStreamScopeNode);
    }
            
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_FALSE, &svrStreamNode, &addedEntry );
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamEntryGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == svrStreamNode )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFilterSet() failed: Unknown streamKey"));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clLogServerStreamMutexLock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    
    severityFilter = pSvrStreamData->pStreamHeader->filter.severityFilter;

    rc = clLogServerStreamMutexUnlock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
    }

    /*
     * Change the severity filter mask to a severity.
     */
    for(i = CL_LOG_SEV_TRACE; i >= CL_LOG_SEV_EMERGENCY; --i)
    {
        if(severityFilter & ( 1 << (i-1)))
        {
            severityFilter = i;
            break;
        }
    }

    if(i < CL_LOG_SEV_EMERGENCY) severityFilter = 0;

    *pSeverityFilter = severityFilter;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

/*****************Filter Set*****************************/

ClRcT
clLogSvrDebugFilterSet(ClNameT *pStreamName,
                       ClNameT *pStreamScopeNode,
                       ClLogFilterT *pFilter,
                       ClLogFilterFlagsT flags)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       svrStreamNode      = CL_HANDLE_INVALID_VALUE;
    ClBoolT                addedEntry         = CL_FALSE;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClNameT streamScopeNode = {0};
    ClLogSvrFilterCbDataT  filterCbData       = {0};
    ClLogStreamScopeT streamScope = CL_LOG_STREAM_LOCAL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!pStreamName || !pFilter) return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);

    if(!pStreamScopeNode)
    {
        pStreamScopeNode = &streamScopeNode;
        clCpmLocalNodeNameGet(pStreamScopeNode);
    }
            
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_FALSE, &svrStreamNode, &addedEntry );
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamEntryGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == svrStreamNode )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFilterSet() failed: Unknown streamKey"));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clLogServerStreamMutexLock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    
    rc = clLogFilterModify(pSvrStreamData->pStreamHeader, pFilter, flags);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFilterUpdate(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pSvrStreamData),
                       CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    
    rc = clLogServerStreamMutexUnlock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
    }

    filterCbData.pStreamName      = pStreamName;
    filterCbData.pStreamScope     = &streamScope;
    filterCbData.pStreamScopeNode = pStreamScopeNode;
    filterCbData.pFilter          = pFilter;

    rc = clCntWalkFailSafe(pSvrStreamData->hComponentTable, 
                           clLogSvrFilterSetClientInformCb, &filterCbData,
                           sizeof(filterCbData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]\n", rc));
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}


ClRcT
VDECL_VER(clLogSvrFilterSet, 4, 0, 0)(
                  ClNameT            *pStreamName,
                  ClLogStreamScopeT  streamScope,
                  ClNameT            *pStreamScopeNode,
                  ClLogFilterT       *pFilter)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       svrStreamNode      = CL_HANDLE_INVALID_VALUE;
    ClBoolT                addedEntry         = CL_FALSE;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrFilterCbDataT  filterCbData       = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_FALSE, &svrStreamNode, &addedEntry );
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamEntryGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == svrStreamNode )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFilterSet() failed: Unknown streamKey"));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, svrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    if(!pSvrStreamData->pStreamHeader)
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return CL_LOG_RC(CL_ERR_NOT_INITIALIZED);
    }

    rc = clLogServerStreamMutexLock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    
    rc = clLogFilterAssign(pSvrStreamData->pStreamHeader, pFilter);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFilterUpdate(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clLogServerStreamMutexUnlock(pSvrStreamData),
                       CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    
    rc = clLogServerStreamMutexUnlock(pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
    }

    filterCbData.pStreamName      = pStreamName;
    filterCbData.pStreamScope     = &streamScope;
    filterCbData.pStreamScopeNode = pStreamScopeNode;
    filterCbData.pFilter          = pFilter;

    rc = clCntWalkFailSafe(pSvrStreamData->hComponentTable, 
                           clLogSvrFilterSetClientInformCb, &filterCbData,
                           sizeof(filterCbData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]\n", rc));
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrFilterSetClientInformCb(ClCntKeyHandleT   key,
                               ClCntDataHandleT  data,
                               ClCntArgHandleT   arg,
                               ClUint32T         size)
{
    ClRcT                  rc             = CL_OK;
    ClLogSvrCompDataT      *pData         = (ClLogSvrCompDataT *) data;
    ClIdlHandleT           hLogIdl        = CL_HANDLE_INVALID_VALUE;
    ClLogSvrFilterCbDataT *pFilterCbData = (ClLogSvrFilterCbDataT*)arg;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrClientIdlHandleInitialize(pData->portId, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }


    rc = VDECL_VER(clLogClientFilterSetNotifyClientAsync, 4, 0, 0)(hLogIdl, *pFilterCbData->pStreamName,
                                                                   *pFilterCbData->pStreamScope, 
                                                                   *pFilterCbData->pStreamScopeNode,
                                                                   *pFilterCbData->pFilter, NULL, NULL); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntFilterSetClientAsync(): rc[0x %x]", rc));
    }
    
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrClientIdlHandleInitialize(ClIocPortT    portId,
                                  ClIdlHandleT  *phLogIdl)
{
    ClRcT              rc         = CL_OK;
    ClIdlHandleObjT    idlObj     = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT      address    = {0};
    ClIocAddressT      iocAddress = {{0}};

    CL_LOG_DEBUG_TRACE(("Enter"));

    iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    iocAddress.iocPhyAddress.portId      = portId;
    address.addressType                  = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress           = iocAddress;
    idlObj.address                       = address;
    idlObj.flags                         = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout               = CL_LOG_SVR_DEFAULT_TIMEOUT;
    idlObj.options.priority              = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries               = CL_LOG_SVR_DEFAULT_RETRY;

    rc = clIdlHandleInitialize(&idlObj, phLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit: NodeAddr: %u portId: %d",
                        iocAddress.iocPhyAddress.nodeAddress,
                        iocAddress.iocPhyAddress.portId));
    return rc;
}

ClRcT
clLogShmCreateAndFill(ClStringT               *pShmName, 
                      ClUint32T               shmSize, 
                      ClUint16T               streamId,
                      ClUint32T               componentId,
                      ClIocMulticastAddressT  *pStreamMcastAddr,
                      ClLogFilterT            *pStreamFilter,
                      ClLogStreamAttrIDLT     *pStreamAttr,
                      ClLogStreamHeaderT      **ppStreamHeader)
{
    ClRcT      rc      = CL_OK;
    ClInt32T   shmFd   = CL_LOG_INVALID_FD;
    ClUint32T  maxComp = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogConfigDataGet(NULL, &maxComp, NULL, NULL);
    if( CL_OK != rc ) 
    {
        CL_LOG_DEBUG_ERROR(("clLogConfigDataGet: rc[0x %x]", rc));
        return rc;
    }

    rc = clLogShmGet(pShmName->pValue, &shmFd);
    if( CL_OK != rc ) 
    {
        if( CL_ERR_ALREADY_EXIST == CL_GET_ERROR_CODE(rc) )
        {
            CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);
        }
        CL_LOG_DEBUG_ERROR(("Shared segment is already exist: rc[0x %x]",
                    rc));
        return rc;
    }

    rc = clLogStreamShmSegInit(pShmName->pValue, shmFd, shmSize, streamId,
                               pStreamMcastAddr, pStreamAttr->recordSize, 
                               pStreamAttr->flushFreq,
                               pStreamAttr->flushInterval, 
                               CL_LOG_MAX_MSGS, maxComp, 
                               ppStreamHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);
    rc = clLogFilterAssign(*ppStreamHeader, pStreamFilter);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppStreamHeader, shmSize), CL_OK);
        return rc;

    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogGlobalCkptDestroy(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSOEoDataT         *pSoEoEntry        = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_CLEANUP(clLogMasterCkptDestroy(), CL_OK);

    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hCkpt )
    {
      CL_LOG_CLEANUP(clCkptCheckpointClose(pSoEoEntry->hCkpt), CL_OK);
      pSoEoEntry->hCkpt = CL_HANDLE_INVALID_VALUE;
    }

    CL_LOG_CLEANUP(clCkptFinalize(pSvrCommonEoEntry->hSvrCkpt), CL_OK);
    pSvrCommonEoEntry->hSvrCkpt = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogGlobalCkptGet(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSOEoDataT         *pSoEoEntry        = NULL;
    ClBoolT                createCkpt         = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamOwnerGlobalCkptGet(pSoEoEntry, pSvrCommonEoEntry,
                                       &createCkpt);
    if( CL_OK != rc )
    {
        return rc;
    }    
    rc = clLogMasterCkptGet();
    if( CL_OK != rc )
    {
        if( CL_TRUE == createCkpt )
        {
           CL_LOG_CLEANUP(clCkptCheckpointClose(pSoEoEntry->hCkpt), CL_OK);
        }
        pSoEoEntry->hCkpt = CL_HANDLE_INVALID_VALUE;
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrTimerDeleteNStart(ClLogSvrEoDataT  *pSvrEoEntry,
                          void             *pData)
{
    ClRcT            rc      = CL_OK;
    ClTimerTimeOutT  timeout = {0, 1000};

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_HANDLE_INVALID_VALUE != pSvrEoEntry->hTimer )
    {
        rc = clTimerDelete(&pSvrEoEntry->hTimer);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clTimerDelete(): rc[0x %x]", rc));
            return rc;
        }
        pSvrEoEntry->hTimer = CL_HANDLE_INVALID_VALUE;
        CL_LOG_DEBUG_TRACE(("\n++++++++++timer deleted successfully\n "));
    }
    /*
     * Before restarting the timer, check for log server exit status.
     */
    if(gClLogSvrExiting)
        return rc;

    rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT,
                               CL_TIMER_SEPARATE_CONTEXT,
                               clLogTimerCallback, 
                               pData, &pSvrEoEntry->hTimer);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clTimerCreateAndStart(): rc[0x %x]", rc));
        return rc;
    }
    clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
      "Timer has been started...");

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogTimerCallback(void *pData)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClBoolT                logRestart         = *(ClBoolT *) pData;
    ClUint32T              errIndex           = 0;
    ClIocNodeAddressT      localAddress       = CL_IOC_RESERVED_ADDRESS;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClVersionT             ckptVersion        = gCkptVersion;

    CL_LOG_DEBUG_TRACE(("Enter"));

    clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
               "Timer callback started...");
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_FALSE == pSvrEoEntry->ckptInit )
    {
        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Initializing client [CKPT]...");
        rc = clCkptInitialize(&pSvrCommonEoEntry->hSvrCkpt, NULL, &ckptVersion);
        if( CL_OK != rc )
        {
            rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clCkptInitialize failed rc[0x %x]", rc));
            }
            return rc;
        }
        pSvrEoEntry->ckptInit = CL_TRUE;
    }

    if( CL_FALSE == pSvrEoEntry->gmsInit )
    {
        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Initializing client [GMS]...");
        rc = clLogGmsInit();
        if( CL_OK != rc )
        {
            rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogGmsInit(): rc[0x %x]", rc));
                CL_LOG_CLEANUP(clCkptFinalize(pSvrCommonEoEntry->hSvrCkpt), CL_OK);
            }
            return rc;
        }
        pSvrEoEntry->gmsInit = CL_TRUE;
    }

    if( CL_FALSE == pSvrEoEntry->evtInit )
    {
        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Initializing client [EVENT]...");
        rc = clLogEventInitialize(pSvrCommonEoEntry);
        if( CL_OK != rc )
        {
            rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogEvtInit(): rc[0x %x]", rc));
                CL_LOG_CLEANUP(clLogGmsFinalize(), CL_OK);
                CL_LOG_CLEANUP(clCkptFinalize(pSvrCommonEoEntry->hSvrCkpt), CL_OK);
            }
            return rc;
        }
        pSvrEoEntry->evtInit  = CL_TRUE;
    }
        
    if( CL_FALSE == pSvrEoEntry->ckptOpen )
    {
        localAddress = clIocLocalAddressGet();
        if( (pSvrCommonEoEntry->masterAddr == localAddress) || 
            (pSvrCommonEoEntry->deputyAddr == localAddress) )
        {
            rc = clLogGlobalCkptGet();
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogGlobalCkptGet(): rc[0x %x]", rc));
                rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
                return rc;
            }
        }
        else if(clCpmIsSCCapable())
        {
            /*
             * Preboot/initialize the stream owner and master.
             */
            rc = clLogStreamOwnerGlobalBootup();
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogStreamOwnerGlobalBootup(): rc[0x %x]", rc));
                rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
                return rc;
            }    
            rc = clLogMasterBootup();
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogMasterInit(): rc[0x %x]", rc));
                rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
                return rc;
            }
            rc = clLogGlobalCkptGet();
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogGlobalCkptGet(): rc[0x %x]", rc));
                rc = clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
                return rc;
            }
        }
        else
        {
            CL_LOG_CLEANUP(clCkptFinalize(pSvrCommonEoEntry->hSvrCkpt), CL_OK);
        }
        pSvrEoEntry->ckptOpen = CL_TRUE;
    }

    CL_LOG_DEBUG_TRACE(("\n++++++++++after Global Ckpt get\n "));
    if( CL_TRUE == logRestart )
    {    
        pSvrEoEntry->logInit = CL_TRUE;
    }
    clLogNotice(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                "FileOwner is booting up...");
    rc = clLogFileOwnerBootup(logRestart);
    if( CL_OK != rc )
    {
        clLogCritical("LOG", "INI", 
                      "Unrecoverable error. Log file owner boot up failed. Log server exiting...");
        exit(127);
    }
    if( CL_FALSE == logRestart )
    {
        pSvrEoEntry->logInit = CL_TRUE;
        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Opening the perennial streams...");
        rc = clLogSvrPerennialStreamsOpen(&errIndex);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clLogSvrStdStreamClose(errIndex), CL_OK);
            CL_LOG_CLEANUP(clLogFileOwnerShutdown(), CL_OK);
            /* CL_LOG_CLEANUP(clLogFileOwnerEoDataFree(), CL_OK);*/
            CL_LOG_CLEANUP(clLogGlobalCkptDestroy(), CL_OK);
            CL_LOG_CLEANUP(clLogEvtFinalize(CL_FALSE), CL_OK);
            CL_LOG_CLEANUP(clLogGmsFinalize(), CL_OK);
            pSvrEoEntry->logInit = CL_FALSE;
            pSvrEoEntry->ckptOpen = CL_FALSE;
            pSvrEoEntry->ckptInit = CL_FALSE;
            pSvrEoEntry->gmsInit = CL_FALSE;
            pSvrEoEntry->evtInit = CL_FALSE;
            clLogSvrTimerDeleteNStart(pSvrEoEntry, pData);
            return rc;
        }
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                  "Perennial streams(default) streams have been opened");

        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Creating the pre-created streams...");
        rc = clLogSvrPrecreatedStreamsOpen();
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrPrecreatedStreamsOpen(): rc[0x %x]", rc));
        }
    }

    CL_LOG_DEBUG_TRACE(("\n++++++++++before the condition\n "));
    if( CL_HANDLE_INVALID_VALUE != pSvrEoEntry->hTimer )
    {
        rc = clTimerDelete(&pSvrEoEntry->hTimer);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clTimerDelete(): rc[0x %x]", rc));
            return rc;
        }
        pSvrEoEntry->hTimer = CL_HANDLE_INVALID_VALUE;
        clLogDebug(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Timer has been deleted");
    }
    clHeapFree(pData);

    clLogNotice(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Log server fully up");

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogSvrStreamHandleFlagsUpdate, 4, 0, 0)(
                                ClNameT                   *pStreamName,
                                ClLogStreamScopeT         streamScope,
                                ClNameT                   *pStreamScopeNode,
                                ClLogStreamHandlerFlagsT  handlerFlags,
                                ClBoolT                   flagsSet)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClBoolT                addedEntry         = CL_FALSE;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter: %d", flagsSet));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_FALSE == pSvrEoEntry->logInit )
    {
        clLogDebug("SVR", "SHU", "LogService is shutting down...");
        return CL_OK;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_FALSE, &hSvrStreamNode, &addedEntry );
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrStreamEntryGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }
    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                              (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock), CL_OK);
        return rc;
    }

    if( CL_TRUE == flagsSet) 
    {
        if( CL_LOG_HANDLER_WILL_ACK == handlerFlags )
        {
            pSvrStreamData->ackersCount++;
        }
        else
        {
            pSvrStreamData->nonAckersCount++;
        }
    }
    else
    {
        if( CL_LOG_HANDLER_WILL_ACK == handlerFlags )
        {
            pSvrStreamData->ackersCount--;
        }
        else
        {
            pSvrStreamData->nonAckersCount--;
        }
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrPerennialStreamsOpen(ClUint32T  *pErrIndex)
{
    ClRcT                rc            = CL_OK;
    ClStringT            shmName       = {0};
    ClUint32T            shmSize       = 0;
    ClUint32T            count         = 0;
    ClLogStreamAttrIDLT  streamAttr[2] = {{{0}}};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogPerennialStreamsDataGet(streamAttr, (ClUint32T)
            sizeof(streamAttr)/ sizeof(streamAttr[0]));
    if( CL_OK != rc )
    {
        return rc;
    }
    for( count = 0; count < nStdStream; count++ )
    {
       rc = VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)(
                               &stdStreamList[count].streamName,
                               stdStreamList[count].streamScope,
                               &stdStreamList[count].streamScopeNode,
                               &streamAttr[count], CL_LOG_STREAM_CREATE, 
                               CL_LOG_DEFAULT_COMPID,
                               CL_LOG_DEFAULT_PORTID,
                               &shmName, &shmSize);
       if( CL_OK != rc )
       {
           *pErrIndex = count;
       }
    }
    for(count = 0; count < nStdStream; count++)
    {
        clHeapFree(streamAttr[count].fileName.pValue);
        clHeapFree(streamAttr[count].fileLocation.pValue);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrPrecreatedStreamsOpen(void)
{
    ClRcT                rc                                         = CL_OK;
    ClStringT            shmName                                    = {0};
    ClUint32T            shmSize                                    = 0;
    ClUint32T            count                                      = 0;
    ClUint32T            i                                          = 0;
    ClLogStreamDataT     *streamAttr[CL_LOG_MAX_PRECREATED_STREAMS] = {0};
    ClNameT              lNodeName                                  = {0};
    ClNameT              gNodeName                                  = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogPrecreatedStreamsDataGet(streamAttr, &count);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogPrecreatedStreamsDataGet() : rc[0x %x]\n", rc));
        return rc;
    }

    rc = clCpmLocalNodeNameGet(&lNodeName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmLocalNodeNameGet(): rc[0x %x]", rc));
        return rc;
    }
    gNodeName.length = strlen(gStreamScopeGlobal);
    strncpy(gNodeName.value, gStreamScopeGlobal, gNodeName.length);

    for( i = 0; i < count; i++ )
    {
        if( CL_LOG_STREAM_LOCAL == streamAttr[i]->streamScope )
        {
            rc = VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)(
                                   &streamAttr[i]->streamName,
                                   streamAttr[i]->streamScope,
                                   &lNodeName,
                                   &streamAttr[i]->streamAttr, 
                                   CL_LOG_STREAM_CREATE, 
                                   CL_LOG_DEFAULT_COMPID,
                                   CL_LOG_DEFAULT_PORTID,
                                   &shmName, &shmSize);
        }
        else if( CL_LOG_STREAM_GLOBAL == streamAttr[i]->streamScope )
        {
            rc = VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)(
                                   &streamAttr[i]->streamName,
                                   streamAttr[i]->streamScope,
                                   &gNodeName,
                                   &streamAttr[i]->streamAttr, 
                                   CL_LOG_STREAM_CREATE, 
                                   CL_LOG_DEFAULT_COMPID,
                                   CL_LOG_DEFAULT_PORTID,
                                   &shmName, &shmSize);
        }
        else
        {
            CL_LOG_DEBUG_ERROR(("Unknown scope: scope[0x %x]\n",
                                streamAttr[i]->streamScope));
        }
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)() : rc[0x %x]\n", rc));
        }
            clHeapFree(streamAttr[i]->streamAttr.fileName.pValue);
            clHeapFree(streamAttr[i]->streamAttr.fileLocation.pValue);
            clHeapFree(streamAttr[i]);
        }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrStdStreamClose(ClUint32T  tblSize)
{
    ClRcT      rc    = CL_OK;
    ClUint32T  count = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    for( count = 0; count < tblSize; count++ )
    {
       rc = VDECL_VER(clLogSvrStreamClose, 4, 0, 0)(
                                &stdStreamList[count].streamName,
                                stdStreamList[count].streamScope,
                                &stdStreamList[count].streamScopeNode,
                                CL_LOG_DEFAULT_COMPID);
       if( CL_OK != rc )
       {
           return rc;
       }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrStdStreamShmCreate(ClStringT               *pShmName,
                           ClUint32T               shmSize,
                           ClLogStreamAttrIDLT     *pStreamAttr,
                           ClUint16T               streamId,
                           ClIocMulticastAddressT  *pStreamMcastAddr,
                           ClLogFilterT            *pFilter,
                           ClLogStreamHeaderT      **ppStreamHeader)
{
    ClRcT      rc          = CL_OK;
    ClInt32T   shmFd       = CL_LOG_INVALID_FD;
    ClUint32T  maxRecCount = 0;
    ClUint32T  headerSize  = 0;
    ClUint32T  maxComp     = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogConfigDataGet(NULL, &maxComp, NULL, NULL);
    if( CL_OK != rc ) 
    {
        CL_LOG_DEBUG_ERROR(("clLogConfigDataGet: rc[0x %x]", rc));
        return rc;
    }

    rc = clLogShmGet(pShmName->pValue, &shmFd);
    if( (CL_OK != rc) && ( CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST) )
    {
        return rc;
    }

    headerSize  = CL_LOG_HEADER_SIZE_GET(CL_LOG_MAX_MSGS, maxComp);
    maxRecCount = ( shmSize - headerSize ) / pStreamAttr->recordSize;
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST )
    {
        struct stat statbuf;
        ClInt32T tries = 0;
        /*
         * Delay for a parallel ftruncate to complete on the pre-created stream
         * to be safe w.r.t a SIGBUS
         */
        memset(&statbuf, 0, sizeof(statbuf));
        while(tries++ < 3 && !fstat(shmFd, &statbuf))
        {
            if(statbuf.st_size < shmSize)
            {
                sleep(1);
            }
            else break;
        }
        if(!statbuf.st_size)
        {
            /*
             * The standard log stream segment has suddenly become inaccessible.
             */
            CL_ASSERT(0);
        }
        rc = clOsalMmap_L(0, shmSize, CL_LOG_MMAP_PROT_FLAGS, CL_LOG_MMAP_FLAGS, 
                          shmFd, 0, (void **) ppStreamHeader);
    }
    else
    {
        rc = clLogStreamShmSegInit(pShmName->pValue, shmFd, shmSize, streamId, pStreamMcastAddr,
                                   pStreamAttr->recordSize, 
                                   pStreamAttr->flushFreq,
                                   pStreamAttr->flushInterval, 
                                   CL_LOG_MAX_MSGS, maxComp, 
                                   ppStreamHeader);
    }
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogSvrMasterCompListUpdate(void)
{
    ClRcT             rc          = CL_OK;
    ClLogCompDataT    *pCompData  = NULL;
    ClUint32T         count       = 0;
    ClUint32T         compLen     = 0;
    ClIdlHandleT      hIdl        = CL_HANDLE_INVALID_VALUE;
    ClUint32T         localAddr   = 0;
    ClNameT           nodeName    = {0};
    static ClUint32T  numStream   = 1;   

    CL_LOG_DEBUG_TRACE(("Enter"));

    localAddr = clIocLocalAddressGet();
    localAddr = localAddr << 16;

    rc = clCpmLocalNodeNameGet(&nodeName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmLocalNodeNameGet() : rc[0x %x]\n", rc));
        return rc;
    }

    pCompData = clHeapCalloc(nLogAspComps, sizeof(ClLogCompDataT));
    if( NULL == pCompData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }
    for( count = 0; count < nLogAspComps; count++ )
    {
        compLen = strlen(aspCompMap[count].pCompName);
        snprintf(pCompData[count].compName.value, sizeof(pCompData[count].compName.value), "%.*s_%.*s",
                compLen, aspCompMap[count].pCompName, nodeName.length,
                nodeName.value);
        pCompData[count].compName.length = strlen(pCompData[count].compName.value);
        pCompData[count].clientId        = localAddr | aspCompMap[count].clntId;
    }

    if( numStream == nStdStream )
    {
        CL_LOG_CLEANUP(clLogCompAddEvtPublish(&pCompData[1].compName, localAddr), CL_OK);
    }
    else if( 1 == numStream )
    {
        rc = clLogSvrIdlHandleInitialize(CL_LOG_STREAM_GLOBAL, &hIdl);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrIdlHandleInitialize(): rc[0x %x]", rc));
            clHeapFree(pCompData);
            return rc;
        }
        rc = VDECL_VER(clLogMasterCompListNotifyClientAsync, 4, 0, 0)(hIdl, nLogAspComps, pCompData,
                                                  NULL, NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterCompListNotifyClientAsync, 4, 0, 0)(): rc[0x %x]", rc));
            clHeapFree(pCompData);
            CL_LOG_CLEANUP(clIdlHandleFinalize(hIdl), CL_OK);
            return rc;
        }
        CL_LOG_CLEANUP(clIdlHandleFinalize(hIdl), CL_OK);
    }
    ++numStream;
    clHeapFree(pCompData);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


static ClRcT
clLogSvrLocalFileOwnerNFlusherStart(void)
{
    ClRcT                rc            = CL_OK;
    ClLogStreamAttrIDLT  streamAttr[2] = {{{0}}};
    ClInt32T             count         = 0;

    /* Read the xml file & fetch the local streams with local fileOwners */
    rc = clLogPerennialStreamsDataGet(streamAttr, (ClUint32T)
                                    sizeof(streamAttr)/ sizeof(streamAttr[0]));
    if( CL_OK != rc )
    {
        return rc;
    }

    for( count = 0; count < nStdStream; count++ )
    {
        /*
         * Stream is already LOCAL stream, hard coded internally & if this
         * stream is intended for local file, then go ahead set it up the 
         * flusher & fileowner stuff
         */
        if( clIocLocalAddressGet() ==
                clLogFileOwnerAddressFetch(&streamAttr[count].fileLocation))
        {
            rc = clLogFileOwnerStreamCreateEvent(&stdStreamList[count].streamName,
                    stdStreamList[count].streamScope,
                    &stdStreamList[count].streamScopeNode,
                    0/*streamId */, &streamAttr[count], CL_FALSE);
            if( CL_OK != rc )
            {
                clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_FO_INIT, 
                        "Server boot up, file owner entry create failed rc[0x %x]",
                        rc);
                goto attrFree;
            }
            /* Set the flusher for all local stream with local files */
            rc = clLogLocalFlusherSetup(&stdStreamList[count].streamName,
                    &stdStreamList[count].streamScopeNode,
                    &streamAttr[count]);
            if( CL_OK != rc )
            {
                goto attrFree;
            }
        }
    }
attrFree:    
    for(count = 0; count < nStdStream; count++)
    {
        clHeapFree(streamAttr[count].fileName.pValue);
        clHeapFree(streamAttr[count].fileLocation.pValue);
    }

    return rc;
}

static  ClRcT
clLogSvrInitialEntryAdd(ClLogSvrEoDataT        *pSvrEoEntry,
                        ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                        ClNameT                *pStreamName,
                        ClNameT                *pStreamScopeNode,
                        ClUint32T              compId,
                        ClIocPortT             portId,
                        ClBoolT                addCompEntry,
                        ClCntNodeHandleT       *phSvrStreamNode)
{
    ClRcT             rc             = CL_OK;
    ClBoolT           addedEntry     = CL_FALSE;

    /* If the table is not yet created, creating here */
    if( CL_HANDLE_INVALID_VALUE == pSvrEoEntry->hSvrStreamTable )
    {
        rc = clCntHashtblCreate(pSvrCommonEoEntry->maxStreams,
                                clLogStreamKeyCompare, clLogStreamHashFn,
                                clLogSvrStreamEntryDeleteCb,
                                clLogSvrStreamEntryDeleteCb, CL_CNT_UNIQUE_KEY,
                                &(pSvrEoEntry->hSvrStreamTable));
        if( CL_OK != rc )
        {
            clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, 
                    "Log server boot up failed to create a hash table rc[0x %x]", rc);
            return rc;
        }
    }

    rc = clLogSvrStreamEntryGet(pSvrEoEntry, pStreamName, pStreamScopeNode,
                                CL_TRUE, phSvrStreamNode, &addedEntry);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, "Log server bootup"
                "failed to create server entries for stream rc[0x %x]", rc);
        return rc;
    }

    if( addCompEntry == CL_TRUE )
    {
        rc = clLogSvrCompRefCountIncrement(pSvrEoEntry, pSvrCommonEoEntry,
                *phSvrStreamNode, compId, portId);
        if( CL_OK != rc )
        {
            if( CL_TRUE == addedEntry )
            {
                CL_LOG_CLEANUP(clCntNodeDelete(pSvrEoEntry->hSvrStreamTable,
                            *phSvrStreamNode), CL_OK);
            }
            return rc;
        }
    }

    return CL_OK;
}

static ClRcT
clLogLocalFlusherSetup(ClNameT              *pStreamName,
                       ClNameT              *pStreamScopeNode, 
                       ClLogStreamAttrIDLT  *pStreamAttr)
                       
{
    ClRcT                  rc                 = CL_OK;
    ClStringT              shmName            = {0};
    ClUint32T              shmSize            = 0;
    ClLogFilterT           filter             = {0};
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;

    /* Take out the svr eo entry */
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        clLogError("SVR", "BOO", "Failed to fetch server eo entry rc [0x %x]",
                rc);
        return rc;
    }
    /* Lock the svr stream table */
    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }
    /* Add the initial entry */
    rc = clLogSvrInitialEntryAdd(pSvrEoEntry, pSvrCommonEoEntry, pStreamName, 
            pStreamScopeNode, CL_LOG_DEFAULT_COMPID, CL_LOG_DEFAULT_PORTID, CL_FALSE, 
            &hSvrStreamNode);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, 
                "Svr initial boot up failed rc[0x %x]", rc);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                CL_OK);
        return rc;
    }
    /* update the svn stream entry with all initial entries*/ 
    rc = clLogSvrStreamEntryUpdate(pSvrEoEntry, pStreamName, pStreamScopeNode, 
            hSvrStreamNode, CL_LOG_DEFAULT_COMPID, pStreamAttr,
            0, &filter, 0, 0, 0, &shmName, &shmSize);
    if( CL_OK != rc )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, 
                "Svr initial boot up failed rc[0x %x]", rc);
        CL_LOG_CLEANUP(
                clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode),
                CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
            CL_OK);

    return rc;
}

static ClRcT
clLogFileKeyCopy(ClLogSvrStreamDataT  *pSvrStreamData, 
                 ClLogStreamAttrIDLT  *pStreamAttr)
{
    ClRcT  rc = CL_OK;

    pSvrStreamData->fileName.pValue =
    clHeapCalloc(pStreamAttr->fileName.length, sizeof(ClCharT));
    if( NULL == pSvrStreamData->fileName.pValue )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, 
          "Failed to allocate memory rc[0x %x]", rc);
        return rc;
    }
    pSvrStreamData->fileLocation.pValue = 
    clHeapCalloc(pStreamAttr->fileLocation.length, sizeof(ClCharT));
    if( NULL == pSvrStreamData->fileName.pValue )
    {
        clLogError(CL_LOG_AREA_SVR, CL_LOG_CTX_BOOTUP, 
          "Failed to allocate memory rc[0x %x]", rc);
        clHeapFree(pSvrStreamData->fileName.pValue);
        return rc;
    }
    pSvrStreamData->fileName.length = pStreamAttr->fileName.length;
    pSvrStreamData->fileLocation.length = pStreamAttr->fileLocation.length;
       
    strncpy(pSvrStreamData->fileName.pValue, pStreamAttr->fileName.pValue, 
            pStreamAttr->fileName.length);
    strncpy(pSvrStreamData->fileLocation.pValue,
    pStreamAttr->fileLocation.pValue, pStreamAttr->fileLocation.length);

    return CL_OK;
}

static ClRcT 
clLogSvrStreamEntryUpdate(ClLogSvrEoDataT         *pSvrEoEntry,
                          ClNameT                 *pStreamName,
                          ClNameT                 *pStreamScopeNode,
                          ClCntNodeHandleT        hSvrStreamNode,
                          ClUint32T               compId, 
			    	      ClLogStreamAttrIDLT     *pStreamAttr,
                          ClIocMulticastAddressT  streamMcastAddr,
                          ClLogFilterT            *pStreamFilter,
                          ClUint32T               ackerCnt, 
                          ClUint32T               nonAckerCnt, 
			   		      ClUint16T				  streamId,
                          ClStringT               *pShmName,
                          ClUint32T               *pShmSize)
{
    ClRcT                rc              = CL_OK;
    ClLogSvrStreamDataT  *pSvrStreamData = NULL;

    rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                            (ClCntDataHandleT *) &pSvrStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    pSvrStreamData->nonAckersCount = nonAckerCnt;
    pSvrStreamData->ackersCount    = ackerCnt;

//    if( streamMcastAddr == 0 )
    {
        /* assign if the multicast is not specified */
        pSvrStreamData->fileOwnerAddr =
        clLogFileOwnerAddressFetch(&pStreamAttr->fileLocation);
    }

    if( 0 == pSvrStreamData->shmName.length )
    {
        rc = clLogShmNameCreate(pStreamName, pStreamScopeNode, pShmName);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogSvrShmNameCreate(): rc[0x %x]", rc));
            return rc;
        }
        pSvrStreamData->shmName.length = pShmName->length;
        pSvrStreamData->shmName.pValue = clHeapCalloc(pShmName->length,
                                                      sizeof(ClCharT));
        if( NULL == pSvrStreamData->shmName.pValue )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            CL_LOG_CLEANUP(clLogSvrShmNameDelete(pShmName), CL_OK);
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        memcpy(pSvrStreamData->shmName.pValue, pShmName->pValue,
                pShmName->length);
    }
    else
    {
        pShmName->length = pSvrStreamData->shmName.length;
        pShmName->pValue = clHeapCalloc(pShmName->length, sizeof(ClCharT));
        if( NULL == pShmName->pValue )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            return rc;
        }
        memcpy(pShmName->pValue, pSvrStreamData->shmName.pValue, pShmName->length);
    }
    rc = clLogFileKeyCopy(pSvrStreamData, pStreamAttr);
    if( CL_OK != rc )
    {
       CL_LOG_CLEANUP(clLogSvrShmNameDelete(pShmName), CL_OK);
       return rc;
    }
    rc = clLogSvrFlusherCheckNStart(pSvrEoEntry, hSvrStreamNode, 
                                    &streamId, compId, &streamMcastAddr,
                                    pStreamFilter, pStreamAttr, pShmName,
                                    pShmSize);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrShmNameDelete(pShmName), CL_OK);
        return rc;
    }

    return rc;
}
