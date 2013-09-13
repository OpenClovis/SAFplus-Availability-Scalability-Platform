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
#include <clCksmApi.h>
#include <clBitmapApi.h>

#include <clLogSvrCommon.h>
#include <clLogMaster.h>
#include <clLogMasterEo.h>
#include <clLogMasterCkpt.h>
#include <clLogFileEvt.h>
#include <xdrClLogCompDataT.h>
#include <clCpmExtApi.h>

ClUint32T  gStreamCnt = 0;

static ClRcT
clLogMasterFileEntryGetNMatch(ClLogMasterEoDataT   *pMasterEoEntry,
                              ClLogStreamAttrIDLT  *pStreamAttr,
                              ClCntNodeHandleT     *phFileNode,
                              ClBoolT              *pAddCkptEntry);

static ClRcT
clLogMasterFileEntryAdd(ClLogMasterEoDataT   *pMasterEoEntry,
                        ClLogFileKeyT        *pKey,
                        ClLogStreamAttrIDLT  *pStreamAttr,
                        ClCntNodeHandleT     *phFileNode);


static ClRcT
clLogMasterFileStreamAttrGet(ClLogMasterEoDataT      *pMasterEoEntry,
                             ClCntNodeHandleT        hFileNode,
                             SaNameT                 *pStreamName,
                             SaNameT                 *pStreamScopeNode,
                             ClUint16T               *pStreamId,
                             ClIocMulticastAddressT  *pStreamMcastAddr);

static ClRcT
clLogMasterFileStreamEntryAdd(ClUint16T           streamId, 
                              ClLogFileDataT      *pFileData,
                              ClLogStreamKeyT     *pStreamKey,
                              ClCntNodeHandleT    *phStreamNode);

static ClRcT
clLogMasterFileStreamAttrObtain(ClLogMasterEoDataT      *pMasterEoEntry,
                                ClLogMasterStreamDataT  *pStreamData,
                                ClUint16T               *pStreamId,
                                ClIocMulticastAddressT  *pStreamMcastAddr,
                                ClLogFileDataT          *pFileData);


static ClRcT
clLogMasterFileEntryRemove(ClLogMasterEoDataT      *pMasterEoEntry,
                           ClCntNodeHandleT        hFileNode,
                           ClIocMulticastAddressT  *pStreamMcastAddr);
ClRcT
clLogMasterBootup(void)
{
    ClRcT              rc              = CL_OK;
    ClLogMasterEoDataT *pMasterEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoDataInit(&pMasterEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterShutdown(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogMasterEoDataT     *pMasterEoEntry    = NULL;
    ClIocNodeAddressT      localAddr          = 0;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    clOsalMutexLock(&pSvrCommonEoEntry->lock);
    if(!(pSvrCommonEoEntry->flags & __LOG_EO_INIT)
       ||
       !(pSvrCommonEoEntry->flags & __LOG_EO_MASTER_INIT))
    {
        clOsalMutexUnlock(&pSvrCommonEoEntry->lock);
        return rc;
    }
    pSvrCommonEoEntry->flags &= ~__LOG_EO_MASTER_INIT;
    clOsalMutexUnlock(&pSvrCommonEoEntry->lock);

    localAddr = clIocLocalAddressGet();
    if( (localAddr == pSvrCommonEoEntry->masterAddr) ||
        (localAddr == pSvrCommonEoEntry->deputyAddr) ||
        clCpmIsSCCapable())
    {
        rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
        if( CL_OK != rc)
        {
            CL_LOG_DEBUG_ERROR(("clLogMasterEoEntryGet(): rc[0x %x]", rc));
            return rc;
        }
#if 0
        /* this is because of while shutting down, those ckpt not be there so
         * to avoid errors
         */
        CL_LOG_CLEANUP(clCkptCheckpointClose(pMasterEoEntry->hCkpt), CL_OK);
#endif

        CL_LOG_CLEANUP(clLogMasterEoDataFinalize(pMasterEoEntry), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterAttrVerifyNGet, 4, 0, 0)(
                          ClLogStreamAttrIDLT     *pStreamAttr,
                          SaNameT                 *pStreamName,
                          ClLogStreamScopeT       *pStreamScope,
                          SaNameT                 *pStreamScopeNode,
                          ClUint16T               *pStreamId,
                          ClIocMulticastAddressT  *pStreamMcastAddr)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;
    ClCntNodeHandleT    hFileNode       = CL_HANDLE_INVALID_VALUE;
    ClBoolT             addCkptEntry    = CL_TRUE;

    CL_LOG_DEBUG_TRACE(("Enter: %d", *pStreamId));

    CL_LOG_PARAM_CHK((NULL == pStreamAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode),
                      CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamId), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamMcastAddr),
                      CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&clOsalMutexLock(): rc[0x%x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE == pMasterEoEntry->hMasterFileTable )
    {
        rc = clLogMasterEoEntrySet(pMasterEoEntry);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(
                clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
            return rc;
        }
    }

    rc = clLogMasterFileEntryGetNMatch(pMasterEoEntry, pStreamAttr, &hFileNode,
                                       &addCkptEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileEntryGetNMatch(): rc[0x%x]", rc));
        CL_LOG_CLEANUP(
            clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
        return rc;
    }

    rc = clLogMasterFileStreamAttrGet(pMasterEoEntry, hFileNode, pStreamName,
                                      pStreamScopeNode, pStreamId,
                                      pStreamMcastAddr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterFileStreamAttrGet(): rc[0x %x]", rc));
        if( CL_TRUE == addCkptEntry )
        {
            CL_LOG_CLEANUP(
                clLogMasterFileEntryRemove(pMasterEoEntry, hFileNode,
                                           CL_IOC_RESERVED_ADDRESS), 
                CL_OK);
        }
        CL_LOG_CLEANUP(
            clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
        return rc;
    }

    rc = clLogMasterDataCheckpoint(pMasterEoEntry, hFileNode, addCkptEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterDataCheckpoint: rc[0x %x]", rc));
        if( CL_TRUE == addCkptEntry )
        {
            CL_LOG_CLEANUP(
                clLogMasterFileEntryRemove(pMasterEoEntry, hFileNode,
                                           pStreamMcastAddr),
                CL_OK);
        }
        CL_LOG_CLEANUP(
            clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
        return rc;
    }

    rc = clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc [0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClInt32T
clLogFileKeyCompare(ClCntKeyHandleT  key1,
                    ClCntKeyHandleT  key2)
{
    ClLogFileKeyT  *pKey1 = (ClLogFileKeyT *) key1;
    ClLogFileKeyT  *pKey2 = (ClLogFileKeyT *) key2;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pKey1), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pKey2), CL_LOG_RC(CL_ERR_NULL_POINTER));

    if( pKey1->fileLocation.length != pKey2->fileLocation.length )
    {
        CL_LOG_DEBUG_TRACE(("fileLocation.length MISMATCH"));
        return -1;
    }

    if( pKey1->fileName.length != pKey2->fileName.length )
    {
        CL_LOG_DEBUG_TRACE(("fileName.length MISMATCH"));
        return -1;
    }

    if( 0 != strncmp(pKey1->fileLocation.pValue, pKey2->fileLocation.pValue,
                     pKey1->fileLocation.length) )
    {
        CL_LOG_DEBUG_TRACE(("fileLocation MISMATCH"));
        return -1;
    }

    if( 0 != strncmp(pKey1->fileName.pValue, pKey2->fileName.pValue,
                     pKey1->fileName.length) )
    {
            CL_LOG_DEBUG_TRACE(("fileName MISMATCH"));
            return -1;
    }

    CL_LOG_DEBUG_TRACE(("Exit MATCH"));
    return 0;
}

ClUint32T
clLogFileKeyHashFn(ClCntKeyHandleT  key)
{
    CL_LOG_DEBUG_TRACE(("Enter - Exit"));

    return ((ClLogFileKeyT *) key)->hash ;
}

void
clLogMasterFileEntryDeleteCb(ClCntKeyHandleT   key,
                             ClCntDataHandleT  data)
{
    ClLogFileKeyT   *pKey  = (ClLogFileKeyT *) key;
    ClLogFileDataT  *pData = (ClLogFileDataT *) data;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( NULL != pData )
    {
        CL_LOG_CLEANUP(clCntDelete(pData->hStreamTable), CL_OK);
        clHeapFree(pData);
    }

    if( NULL != pKey )
    {
        clLogFileKeyDestroy(pKey);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

void
clLogFileKeyDestroy(ClLogFileKeyT  *pKey)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    if( NULL != pKey )
    {
        clHeapFree(pKey->fileLocation.pValue);
        clHeapFree(pKey->fileName.pValue);
        clHeapFree(pKey);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
  return;
}

static ClRcT
clLogMasterFileEntryGetNMatch(ClLogMasterEoDataT   *pMasterEoEntry,
                              ClLogStreamAttrIDLT  *pStreamAttr,
                              ClCntNodeHandleT     *phFileNode,
                              ClBoolT              *pAddCkptEntry)
{
    ClRcT           rc            = CL_OK;
    ClLogFileKeyT   *pKey         = NULL;
    ClLogFileDataT  *pLogFileData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileKeyCreate(&(pStreamAttr->fileName),
                            &(pStreamAttr->fileLocation),
                            pMasterEoEntry->maxFiles, &pKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeFind(pMasterEoEntry->hMasterFileTable,
                       (ClCntKeyHandleT) pKey, phFileNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clLogMasterFileEntryAdd(pMasterEoEntry, pKey, pStreamAttr,
                                     phFileNode);
        if( CL_OK != rc )
        {
            *pAddCkptEntry = CL_FALSE;
            clLogFileKeyDestroy(pKey);
            return rc;
        }
        else
        {
            *pAddCkptEntry = CL_TRUE;
            return rc;
        }
    }
    else if( CL_GET_ERROR_CODE(rc) != CL_OK )
    {
        *pAddCkptEntry = CL_FALSE;
        clLogFileKeyDestroy(pKey);
        return rc;
    }

    *pAddCkptEntry = CL_FALSE;
    clLogFileKeyDestroy(pKey);

    rc = clCntNodeUserDataGet(pMasterEoEntry->hMasterFileTable,
                              *phFileNode, (ClCntDataHandleT *) &pLogFileData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Before AttrMatch nActiveStreams = %d--",
            pLogFileData->nActiveStreams));
    
    if( 0 == pLogFileData->nActiveStreams )
    {
        rc = clLogStreamAttributesCopy(pStreamAttr, &pLogFileData->streamAttr, CL_FALSE);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogStreamAttributesCopy(): rc[0x %x]", rc));
            return rc;
        }
        pLogFileData->streamAttr.fileName.length     = 0;
        pLogFileData->streamAttr.fileName.pValue     = NULL;
        pLogFileData->streamAttr.fileLocation.length = 0;
        pLogFileData->streamAttr.fileLocation.pValue = NULL;
    }
    else
    {
        rc = clLogAttributesMatch(&(pLogFileData->streamAttr), pStreamAttr,
                                   CL_FALSE);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("FileAlready exist"));
            return CL_LOG_RC(CL_ERR_ALREADY_EXIST);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterCompKeyCreate(ClCharT             *pCompPrefix,
                         ClUint32T            maxComps,
                         ClLogMasterCompKeyT  **ppKey)
{
    ClRcT      rc   = CL_OK;
    ClUint32T  hash = 0;

    CL_LOG_DEBUG_TRACE(("Enter: name: %s", pCompPrefix));

    CL_LOG_PARAM_CHK((NULL == pCompPrefix), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppKey), CL_LOG_RC(CL_ERR_NULL_POINTER));

    clCksm32bitCompute((ClUint8T *) pCompPrefix, strlen(pCompPrefix), &hash);

    hash %= maxComps;

    *ppKey = clHeapCalloc(1, sizeof(ClLogMasterCompKeyT));
    if( NULL == *ppKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*ppKey)->pCompPrefix = pCompPrefix;
    (*ppKey)->hash        = hash;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterCompKeyDestroy(ClLogMasterCompKeyT  *pCompKey)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter: name: %s", pCompKey->pCompPrefix));

    clHeapFree(pCompKey->pCompPrefix);
    clHeapFree(pCompKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileKeyCreate(ClStringT      *pFileName,
                   ClStringT      *pFileLocation,
                   ClUint32T      maxFiles,
                   ClLogFileKeyT  **ppKey)
{
    ClRcT      rc        = CL_OK;
    ClCharT    *cksmKey  = NULL;
    ClUint32T  hash      = 0;
    ClUint32T  tempLen   = 0;

    CL_LOG_DEBUG_TRACE(("Enter: namelen: %u locationlen: %u",
                       pFileName->length, pFileLocation->length));

    CL_LOG_PARAM_CHK((NULL == pFileName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pFileLocation), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppKey), CL_LOG_RC(CL_ERR_NULL_POINTER));

    tempLen = pFileName->length + pFileLocation->length;
    cksmKey = clHeapCalloc(tempLen+1, sizeof(ClCharT));

    if( NULL == cksmKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    snprintf(cksmKey, tempLen+1,"%.*s%.*s", pFileLocation->length, 
             pFileLocation->pValue, pFileName->length, pFileName->pValue);
    clCksm32bitCompute((ClUint8T *) cksmKey, tempLen, &hash);
    hash %= maxFiles;
    clHeapFree(cksmKey);

    *ppKey = clHeapCalloc(1, sizeof(ClLogFileKeyT));
    if( NULL == *ppKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*ppKey)->fileName.pValue
        = clHeapCalloc(pFileName->length+1, sizeof(ClCharT));
    if( NULL == (*ppKey)->fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(*ppKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*ppKey)->fileLocation.pValue
        = clHeapCalloc(pFileLocation->length+1, sizeof(ClCharT));
    if( NULL == (*ppKey)->fileLocation.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree((*ppKey)->fileName.pValue);
        clHeapFree(*ppKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*ppKey)->fileName.length = pFileName->length;
    (*ppKey)->fileLocation.length = pFileLocation->length;
    strncpy((*ppKey)->fileName.pValue, pFileName->pValue, pFileName->length);
    strncpy((*ppKey)->fileLocation.pValue, pFileLocation->pValue,
            pFileLocation->length);

    /* The string wont be null terminated if length is less/source not 
     * null terminated
     */
    (*ppKey)->fileName.pValue[pFileName->length] = 0;
    (*ppKey)->fileLocation.pValue[pFileLocation->length] = 0;

    (*ppKey)->hash = hash;

    CL_LOG_DEBUG_TRACE(("Exit [%s] [%s]", (*ppKey)->fileLocation.pValue,
                       (*ppKey)->fileName.pValue));
    return rc;
}

static ClRcT
clLogMasterFileEntryAdd(ClLogMasterEoDataT   *pMasterEoEntry,
                        ClLogFileKeyT        *pKey,
                        ClLogStreamAttrIDLT  *pStreamAttr,
                        ClCntNodeHandleT     *phFileNode)
{
    ClRcT                  rc              = CL_OK;
    ClLogFileDataT         *pData          = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pMasterEoEntry), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pKey), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogMasterEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    pData = clHeapCalloc(1, sizeof(ClLogFileDataT));
    if( NULL == pData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clCntHashtblCreate(pCommonEoEntry->maxStreams,
                            clLogStreamKeyCompare, clLogStreamHashFn,
                            clLogMasterStreamEntryDeleteCb,
                            clLogMasterStreamEntryDeleteCb, CL_CNT_UNIQUE_KEY,
                            &(pData->hStreamTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtbleCreate(): rc[0x %x]\n", rc));
        clHeapFree(pData);
        return rc;
    }

    rc = clLogStreamAttributesCopy(pStreamAttr, &pData->streamAttr, CL_FALSE);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clCntDelete(pData->hStreamTable), CL_OK);
        clHeapFree(pData);
        return rc;
    }
    pData->streamAttr.fileName.pValue     = NULL;
    pData->streamAttr.fileLocation.pValue = NULL;
    pData->streamAttr.fileName.length     = 0;
    pData->streamAttr.fileLocation.length = 0;

    pData->nActiveStreams                 = 0;

    rc = clCntNodeAddAndNodeGet(pMasterEoEntry->hMasterFileTable,
                                (ClCntKeyHandleT) pKey,
                                (ClCntDataHandleT) pData, NULL, phFileNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pData->hStreamTable), CL_OK);
        clHeapFree(pData);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogMasterStreamEntryDeleteCb(ClCntKeyHandleT   key,
                               ClCntDataHandleT  data)
{
    ClLogStreamKeyT         *pKey  = (ClLogStreamKeyT *) key;
    ClLogMasterStreamDataT  *pData = (ClLogMasterStreamDataT *) data;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( NULL != pData )
    {
        clHeapFree(pData);
    }

    if( NULL != pKey )
    {
        clHeapFree(pKey);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

static ClRcT
clLogMasterFileStreamAttrGet(ClLogMasterEoDataT      *pMasterEoEntry,
                             ClCntNodeHandleT        hFileNode,
                             SaNameT                 *pStreamName,
                             SaNameT                 *pStreamScopeNode,
                             ClUint16T               *pStreamId,
                             ClIocMulticastAddressT  *pStreamMcastAddr)
{
    ClRcT                   rc               = CL_OK;
    ClLogFileDataT          *pFileData       = NULL;
    ClCntNodeHandleT        hStreamNode      = CL_HANDLE_INVALID_VALUE;
    ClLogStreamKeyT         *pStreamKey      = NULL;
    ClLogMasterStreamDataT  *pStreamData     = NULL;
    ClBoolT                 streamEntryAdded = CL_FALSE;
    ClLogSvrCommonEoDataT   *pCommonEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCntNodeUserDataGet(pMasterEoEntry->hMasterFileTable, hFileNode,
                              (ClCntDataHandleT *) &pFileData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCntNodeFind(pFileData->hStreamTable, (ClCntKeyHandleT) pStreamKey,
                       &hStreamNode);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        rc = clLogMasterFileStreamEntryAdd(*pStreamId, pFileData, pStreamKey,
                                           &hStreamNode);
        if( CL_OK != rc )
        {
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }
        streamEntryAdded = CL_TRUE;
    }

    rc = clCntNodeUserDataGet(pFileData->hStreamTable, hStreamNode,
                              (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        if( CL_TRUE == streamEntryAdded )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileData->hStreamTable,
                                           (ClCntKeyHandleT) pStreamKey),
                           CL_OK);
        }
        else
        {
            clLogStreamKeyDestroy(pStreamKey);
        }
        return rc;
    }

    rc = clLogMasterFileStreamAttrObtain(pMasterEoEntry, pStreamData,
                                         pStreamId, pStreamMcastAddr, pFileData);
    if( CL_OK != rc )
    {
        if( CL_TRUE == streamEntryAdded )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileData->hStreamTable,
                                           (ClCntKeyHandleT) pStreamKey),
                           CL_OK);
        }
        else
        {
            clLogStreamKeyDestroy(pStreamKey);
        }
        return rc;
    }

    if( CL_FALSE == streamEntryAdded )
    {
        clLogStreamKeyDestroy(pStreamKey);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileStreamEntryAdd(ClUint16T           streamId, 
                              ClLogFileDataT      *pFileData,
                              ClLogStreamKeyT     *pStreamKey,
                              ClCntNodeHandleT    *phStreamNode)
{
    ClRcT                   rc           = CL_OK;
    ClLogMasterStreamDataT  *pStreamData = NULL;
    ClUint32T               count        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pStreamData = clHeapCalloc(1, sizeof(ClLogMasterStreamDataT));
    if( NULL == pStreamData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    pStreamData->streamMcastAddr = CL_IOC_RESERVED_ADDRESS;
    pStreamData->streamId        = streamId;
    if (0 == pStreamData->streamId)
    {
        for (count = 0; count < nStdStream; count++)
        {
            if (!strncmp((const ClCharT *) pStreamKey->streamName.value, (const ClCharT *) stdStreamList[count].streamName.value,
                            pStreamKey->streamName.length))
            {
                pStreamData->streamId = count + 1;
                break;
            }
        }
    }
    rc = clCntNodeAddAndNodeGet(pFileData->hStreamTable,
                                (ClCntKeyHandleT) pStreamKey,
                                (ClCntDataHandleT) pStreamData,
                                NULL, phStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        clHeapFree(pStreamData);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterFileStreamAttrObtain(ClLogMasterEoDataT      *pMasterEoEntry,
                                ClLogMasterStreamDataT  *pStreamData,
                                ClUint16T               *pStreamId,
                                ClIocMulticastAddressT  *pStreamMcastAddr,
                                ClLogFileDataT          *pFileData)
{
    ClRcT      rc     = CL_OK;
    ClUint32T  bitNum = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( 0 == pStreamData->streamId )
    {
        pStreamData->streamId = ++(pMasterEoEntry->nextStreamId);
    }

    if( CL_IOC_RESERVED_ADDRESS == pStreamData->streamMcastAddr )
    {
        rc = clBitmapNextClearBitSetNGet(pMasterEoEntry->hAllocedAddrMap,
                                         pMasterEoEntry->numMcastAddr,
                                         &bitNum);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapGetNSetNextClearBit(): rc[0x %x]",
                                rc));
            return rc;
        }
        pStreamData->streamMcastAddr = pMasterEoEntry->startMcastAddr +
                                       (ClIocMulticastAddressT) bitNum;
        pFileData->nActiveStreams    += 1;
    }

    *pStreamMcastAddr = pStreamData->streamMcastAddr;
    *pStreamId        = pStreamData->streamId;

    CL_LOG_DEBUG_TRACE(("Exit: Addr: %lld Id: %hd",
                        *pStreamMcastAddr, *pStreamId));
    return rc;
}

static ClRcT
clLogMasterFileEntryRemove(ClLogMasterEoDataT      *pMasterEoEntry,
                           ClCntNodeHandleT        hFileNode,
                           ClIocMulticastAddressT  *pStreamMcastAddr)
{
    ClRcT      rc     = CL_OK;
    ClUint32T  bitNum = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeDelete(pMasterEoEntry->hMasterFileTable, hFileNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_IOC_RESERVED_ADDRESS != pStreamMcastAddr )
    {
        bitNum = *pStreamMcastAddr - pMasterEoEntry->startMcastAddr;
        CL_LOG_CLEANUP((clBitmapBitClear(pMasterEoEntry->hAllocedAddrMap,
                                         bitNum)), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterStreamCloseNotify, 4, 0, 0)(
                             ClStringT          *pFileName,
                             ClStringT          *pFileLocation,
                             SaNameT            *pStreamName,
                             ClLogStreamScopeT  streamScope,
                             SaNameT            *pStreamScopeNode)
{
    ClRcT                   rc              = CL_OK;
    ClLogMasterEoDataT      *pMasterEoEntry = NULL;
    ClLogSvrCommonEoDataT   *pCommonEoEntry = NULL;
    ClCntNodeHandleT        hFileNode       = CL_HANDLE_INVALID_VALUE;
    ClLogFileKeyT           *pFileKey       = NULL;
    ClLogFileDataT          *pFileData      = NULL;
    ClLogStreamKeyT         *pStreamKey     = NULL;
    ClLogMasterStreamDataT  *pStreamData    = NULL;
    ClUint32T               bitNum          = 0;
    ClIocMulticastAddressT  addr            = CL_IOC_RESERVED_ADDRESS;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pFileName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pFileLocation), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode),
                      CL_LOG_RC(CL_ERR_NULL_POINTER));

    /*
     * Check if log server is exiting and back off.
     */
    if(gClLogSvrExiting)
        return rc;

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pCommonEoEntry);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&): rc [0x%x]", rc));
        return rc;
    }

    rc = clLogFileKeyCreate(pFileName, pFileLocation,
                            pMasterEoEntry->maxFiles, &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        return rc;
    }

    rc = clCntDataForKeyGet(pMasterEoEntry->hMasterFileTable,
                            (ClCntKeyHandleT) pFileKey,
                            (ClCntDataHandleT *) &pFileData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc [0x%x]", rc));
        clLogFileKeyDestroy(pFileKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        return rc;
    }

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              pCommonEoEntry->maxStreams, &pStreamKey);
    if( CL_OK != rc )
    {
        clLogFileKeyDestroy(pFileKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        return rc;
    }

    rc = clCntDataForKeyGet(pFileData->hStreamTable,
                            (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc [0x%x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        clLogFileKeyDestroy(pFileKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        return rc;
    }

    bitNum = pStreamData->streamMcastAddr - pMasterEoEntry->startMcastAddr;
    rc = clBitmapBitClear(pMasterEoEntry->hAllocedAddrMap, bitNum);
    if( CL_OK == rc )
    {
        CL_LOG_DEBUG_TRACE(("Unusing Mcast Addr: %u", bitNum));
        addr = pStreamData->streamMcastAddr;
        pStreamData->streamMcastAddr = CL_IOC_RESERVED_ADDRESS;
        pFileData->nActiveStreams--;
        CL_LOG_DEBUG_TRACE(("\n In closeNotify nActiveStreams = %d--", 
                    pFileData->nActiveStreams));

        rc = clCntNodeFind(pMasterEoEntry->hMasterFileTable,
                           (ClCntKeyHandleT) pFileKey, &hFileNode);
        if( CL_OK == rc )
        {
            rc = clLogMasterDataCheckpoint(pMasterEoEntry, hFileNode,
                                           CL_FALSE);
        }
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clBitmapBitSet(pMasterEoEntry->hAllocedAddrMap,
                             bitNum), CL_OK);
            pStreamData->streamMcastAddr = addr;
        }
    }

    clLogStreamKeyDestroy(pStreamKey);
    clLogFileKeyDestroy(pFileKey);
    rc = clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexUnlock_L(&): rc [0x%x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));

    return rc;
}

ClRcT
clLogMasterStreamInfoGet(ClCntKeyHandleT   key,
                         ClCntDataHandleT  data,
                         ClCntArgHandleT   arg, 
                         ClUint32T         size)
{
    ClRcT                   rc           = CL_OK;
    ClLogMasterStreamDataT  *pStreamData = (ClLogMasterStreamDataT *) data;
    ClLogStreamKeyT         *pStreamKey  = (ClLogStreamKeyT *) key;
    ClBufferHandleT         msg          = (ClBufferHandleT) arg;
    ClLogStreamScopeT       streamScope  = 0;
    ClUint32T               validEntry   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_IOC_RESERVED_ADDRESS == pStreamData->streamMcastAddr )
    {
        rc = clXdrMarshallClUint32T(&validEntry, msg, 0);
        if( CL_OK != rc )
        {
            return rc;
        }
        return CL_OK;
    }
    validEntry = 1;
    rc = clXdrMarshallClUint32T(&validEntry, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrMarshallSaNameT(&pStreamKey->streamName, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrMarshallSaNameT(&pStreamKey->streamScopeNode, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogStreamScopeGet(&pStreamKey->streamScopeNode, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrMarshallClUint32T(&streamScope, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrMarshallClUint16T(&pStreamData->streamId, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
    clLogDebug(CL_LOG_AREA_MASTER, CL_LOG_CTX_MAS_INIT, 
             "Packing the streamInfo [%.*s:%.*s]", 
             pStreamKey->streamName.length, pStreamKey->streamName.value, 
             pStreamKey->streamScopeNode.length, pStreamKey->streamScopeNode.value);
    gStreamCnt++;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterTableWalk(ClCntKeyHandleT   key,
                     ClCntDataHandleT  data,
                     ClCntArgHandleT   arg,
                     ClUint32T         size)
{
    ClRcT            rc         = CL_OK;
    ClLogFileDataT   *pFileData = (ClLogFileDataT *) data;
    ClLogFileKeyT    *pFileKey  = (ClLogFileKeyT *) key;
    ClBufferHandleT  msg        = (ClBufferHandleT) arg;
    ClUint32T        numStreams = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = VDECL_VER(clXdrMarshallClLogStreamAttrIDLT, 4, 0, 0)(&pFileData->streamAttr, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrMarshallClLogStreamAttrIDLT, 4, 0, 0)(): rc[0x %x]",
                    rc));
        return rc;
    }
    rc = clXdrMarshallClStringT(&pFileKey->fileName, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClStringT(&pFileKey->fileLocation, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x %x]", rc));
        return rc;
    }
    rc = clCntSizeGet(pFileData->hStreamTable, &numStreams);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clXdrMarshallClUint32T(&numStreams, msg, 0);
    if( CL_OK != rc )
    {
        return rc;
    }
        
    rc = clCntWalk(pFileData->hStreamTable, clLogMasterStreamInfoGet,
                   (ClCntArgHandleT) msg, sizeof(msg));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterStreamListGet, 4, 0, 0)(ClUint32T  *pNumStreams, 
                         ClUint32T  *pBuffLength,
                         ClUint8T   **ppBuffer)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;
    ClBufferHandleT     msg             = CL_HANDLE_INVALID_VALUE;
    ClUint32T           numFiles        = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pNumStreams), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pBuffLength), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppBuffer), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(&): rc [0x%x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE == pMasterEoEntry->hMasterFileTable )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }
    rc = clCntSizeGet(pMasterEoEntry->hMasterFileTable, &numFiles);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clXdrMarshallClUint32T(&numFiles, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    gStreamCnt = 0;
    rc = clCntWalk(pMasterEoEntry->hMasterFileTable, clLogMasterTableWalk,
                   (ClCntArgHandleT) msg, sizeof(msg));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                       CL_OK);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    *pNumStreams = gStreamCnt;
    gStreamCnt = 0;
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                   CL_OK);

    rc = clBufferLengthGet(msg, pBuffLength);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
#if 0
    rc = clBufferFlatten(msg, ppBuffer);
#endif
    *ppBuffer = clHeapCalloc(*pBuffLength, sizeof(ClCharT));
    if( NULL == *ppBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    rc = clBufferNBytesRead(msg, (ClUint8T *) *ppBuffer, pBuffLength);
    if( CL_OK != rc )
    {
        clHeapFree(*ppBuffer);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
            
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClInt32T
clLogMasterCompKeyCompare(ClCntKeyHandleT  key1,
                          ClCntKeyHandleT  key2)
{
    ClLogMasterCompKeyT  *pKey1 = (ClLogMasterCompKeyT *) key1;
    ClLogMasterCompKeyT  *pKey2 = (ClLogMasterCompKeyT *) key2;
    ClUint32T            length = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    length = strlen(pKey1->pCompPrefix);
    if( length == strlen(pKey2->pCompPrefix) )
    {
        if( !(strncmp(pKey1->pCompPrefix, pKey2->pCompPrefix, length)) )
        {
            return 0;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return -1;
}

ClUint32T
clLogMasterCompHashFn(ClCntKeyHandleT  key)
{
    CL_LOG_DEBUG_TRACE(("Enter - Exit"));

    return ((ClLogMasterCompKeyT *) key)->hash ;
}

void
clLogMasterCompEntryDeleteCb(ClCntKeyHandleT   key,
                             ClCntDataHandleT  data)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree((ClLogCompDataT *) data );
    clLogMasterCompKeyDestroy((ClLogMasterCompKeyT *) key);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogMasterCompEntryGet(ClLogMasterEoDataT     *pMasterEoEntry,
                        ClLogSvrCommonEoDataT  *pCommonEoEntry, 
                        ClLogMasterCompKeyT    *pCompKey,
                        SaNameT                *pCompName, 
                        ClUint32T              *pClientId,
                        ClBoolT                restart)
{
    ClRcT           rc           = CL_OK;
    ClUint32T       clientId     = 0;
    ClBoolT         createdTable = CL_FALSE;
    ClLogCompDataT  *pCompData   = NULL;
    ClBoolT         publisheEvt  = CL_FALSE;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_HANDLE_INVALID_VALUE == pMasterEoEntry->hCompTable) 
    {
        rc = clCntHashtblCreate(pCommonEoEntry->maxComponents,
                                clLogMasterCompKeyCompare, 
                                clLogMasterCompHashFn,
                                clLogMasterCompEntryDeleteCb,
                                clLogMasterCompEntryDeleteCb,
                                CL_CNT_UNIQUE_KEY,
                                &(pMasterEoEntry->hCompTable));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntHashtbleCreate(): rc[0x %x]\n", rc));
            return rc;
        }
        createdTable = CL_TRUE;
    }
    rc = clCntDataForKeyGet(pMasterEoEntry->hCompTable, 
                            (ClCntKeyHandleT) pCompKey, 
                            (ClCntDataHandleT *) &pCompData);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        if( 0 == *pClientId )
        {
            clientId    = ++pMasterEoEntry->nextCompId;
            publisheEvt = CL_TRUE;
        }
        else
        {
            clientId    = *pClientId;
        }

        pCompData = clHeapCalloc(1, sizeof(ClLogCompDataT));
        if( NULL == pCompData )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            if( CL_TRUE == createdTable )
            {
                CL_LOG_CLEANUP(clCntDelete(pMasterEoEntry->hCompTable),
                        CL_OK);
                pMasterEoEntry->hCompTable = CL_HANDLE_INVALID_VALUE;
            }
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        
        pCompData->compName = *pCompName;
        pCompData->clientId = clientId;
        rc = clCntNodeAdd(pMasterEoEntry->hCompTable, 
                          (ClCntKeyHandleT) pCompKey, 
                          (ClCntDataHandleT) pCompData, NULL);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeAdd(): rc[0x %x]", rc));
            clHeapFree(pCompData);
            clLogMasterCompKeyDestroy(pCompKey);
            if( CL_TRUE == createdTable )
            {
                CL_LOG_CLEANUP(clCntDelete(pMasterEoEntry->hCompTable),
                        CL_OK);
                pMasterEoEntry->hCompTable = CL_HANDLE_INVALID_VALUE;
            }
            return rc;
        }
        *pClientId = clientId;

        if( CL_FALSE == restart )
        {
            rc = clLogMasterCompDataCheckpoint(pMasterEoEntry, pCompData);
            if( CL_OK != rc )
            {
                CL_LOG_CLEANUP(clCntAllNodesForKeyDelete(pMasterEoEntry->hCompTable, 
                            (ClCntDataHandleT)
                            pCompKey), CL_OK);
                if( CL_TRUE == createdTable )
                {
                    CL_LOG_CLEANUP(clCntDelete(pMasterEoEntry->hCompTable),
                            CL_OK);
                    pMasterEoEntry->hCompTable = CL_HANDLE_INVALID_VALUE;
                }
                return rc; 
            }
        }

        if( CL_TRUE == publisheEvt )
        {
            rc = clLogCompAddEvtPublish(pCompName, *pClientId);
            if( CL_OK != rc )
            {
                /* 
                 *  Do we do revert back. without compEntry also we can still
                 * provide the service. 
                 */
            }
        }
    }
    else if( CL_OK == rc )
    {
        clLogMasterCompKeyDestroy(pCompKey);
    }
     
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT 
clLogMasterCompNameGet(SaNameT  *pCompName, 
                       ClCharT  **ppCompPrefix)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pCompName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppCompPrefix), CL_LOG_RC(CL_ERR_NULL_POINTER));

    *ppCompPrefix = clHeapCalloc(pCompName->length + 1, sizeof(ClCharT));
    if( NULL == *ppCompPrefix )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    memcpy(*ppCompPrefix, pCompName->value, pCompName->length);
    (*ppCompPrefix)[pCompName->length] = '\0';

    CL_LOG_DEBUG_TRACE(("Exit: compPrefix: %s", *ppCompPrefix));
    return rc;
}

ClRcT
clLogMasterCompEntryUpdate(SaNameT    *pCompName,
                           ClUint32T  *pClientId,
                           ClBoolT    restart)
{
    ClRcT                  rc              = CL_OK;
    ClCharT                *pCompPrefix    = NULL;
    ClLogMasterEoDataT     *pMasterEoEntry = NULL;
    ClLogMasterCompKeyT    *pMasterCompKey = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL== pCompName), CL_ERR_NULL_POINTER);
    CL_LOG_PARAM_CHK((NULL== pClientId), CL_ERR_NULL_POINTER);

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogMasterCompNameGet(pCompName, &pCompPrefix);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogMasterCompKeyCreate(pCompPrefix, pCommonEoEntry->maxComponents, 
                                  &pMasterCompKey);
    if( CL_OK != rc )
    {
        clHeapFree(pCompPrefix);
        return rc;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterCompTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L() : rc[0x %x]", rc));
        clLogMasterCompKeyDestroy(pMasterCompKey);
        return rc;
    }

    rc = clLogMasterCompEntryGet(pMasterEoEntry, pCommonEoEntry, pMasterCompKey,
                                 pCompName, pClientId, restart);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                       CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                   CL_OK);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterCompIdChkNGet, 4, 0, 0)(SaNameT    *pCompName,
                                             ClUint32T  *pClientId)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL== pCompName), CL_ERR_NULL_POINTER);
    CL_LOG_PARAM_CHK((NULL== pClientId), CL_ERR_NULL_POINTER);

    rc = clLogMasterCompEntryUpdate(pCompName, pClientId, CL_FALSE);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterCompEntryPack(ClCntKeyHandleT   key,
                         ClCntDataHandleT  data,
                         ClCntArgHandleT   arg,
                         ClUint32T         size)
{
    ClLogCompDataT   *pCompData = (ClLogCompDataT *) data;
    ClBufferHandleT  msg        = (ClBufferHandleT) arg;
    ClRcT            rc         = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = VDECL_VER(clXdrMarshallClLogCompDataT, 4, 0, 0)(pCompData, msg, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrMarshallClLogCompDataT, 4, 0, 0)(): rc[0x %x]", rc));
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterCompListGet, 4, 0, 0)(ClUint32T  *pNumEntries,
                                           ClUint32T  *pBuffLen, 
                                           ClUint8T   **ppCompData)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;
    ClBufferHandleT     msg             = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pMasterEoEntry->hCompTable )
    {
        clLogMultiline(CL_LOG_SEV_DEBUG, CL_LOG_AREA_MASTER, CL_LOG_CONTEXT_UNSPECIFIED, 
                     "Component table have not been created yet\n"
                      "It does not harm this execution, continuing...");
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterCompTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L() : rc[0x %x]", rc));
        return rc;
    }

    rc = clCntSizeGet(pMasterEoEntry->hCompTable, pNumEntries);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                       CL_OK);
        return rc;
    }

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                       CL_OK);
        return rc;
    }

    rc = clCntWalk(pMasterEoEntry->hCompTable, clLogMasterCompEntryPack,
                   (ClCntArgHandleT) msg, sizeof(msg));
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                       CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
                   CL_OK);

    rc = clBufferLengthGet(msg, pBuffLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    *ppCompData = clHeapCalloc(*pBuffLen, sizeof(ClUint8T)); 
    if( NULL == *ppCompData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clBufferNBytesRead(msg, *ppCompData, pBuffLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        clHeapFree(*ppCompData);
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogMasterCompListNotify, 4, 0, 0)(ClUint32T       numEntries, 
                          ClLogCompDataT  *pCompData)
{

    ClRcT      rc    = CL_OK;
    ClUint32T  count = 0;

    CL_LOG_DEBUG_TRACE(("Enter: %d", numEntries));

    for( count = 0; count < numEntries ; count++)
    {
        rc = VDECL_VER(clLogMasterCompIdChkNGet, 4, 0, 0)(&(pCompData[count].compName),
                                                          &(pCompData[count].clientId));
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterCompIdChkNGet, 4, 0, 0)(): rc[0x %x]", rc));
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogMasterStreamEntryChkNUnset(ClCntKeyHandleT  key,
                                ClCntDataHandleT data,
                                ClCntArgHandleT  arg,
                                ClUint32T        size)
{
    ClLogStreamKeyT         *pStreamKey  = (ClLogStreamKeyT *) key;
    ClLogMasterStreamDataT  *pStreamData = (ClLogMasterStreamDataT *) data;
    ClLogMasterWalkArgT     *pArg        = (ClLogMasterWalkArgT *) arg;
    ClUint32T               bitNum       = 0;
    ClLogMasterEoDataT      *pMasterEoEntry = NULL;
    ClRcT                   rc              = CL_OK;

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc )
    {
        return rc;
    }
    if (pStreamKey->streamScopeNode.length == pArg->pNodeName->length
                    && !strncmp((const ClCharT *) pStreamKey->streamScopeNode.value, (const ClCharT *) pArg->pNodeName->value,
                                    pArg->pNodeName->length))
    {
        /*
         * This particular stream is created by this going node
         * Hence marking this as invalid 
         */
        clLogDebug(CL_LOG_AREA_MASTER, CL_LOG_CTX_FO_INIT, "Invalidating the stream [%.*s:%.*s]", pStreamKey->streamScopeNode.length,
                        pStreamKey->streamScopeNode.value, pStreamKey->streamName.length, pStreamKey->streamName.value);
        bitNum = pStreamData->streamMcastAddr - pMasterEoEntry->startMcastAddr;
        rc = clBitmapBitClear(pMasterEoEntry->hAllocedAddrMap, bitNum);
        if (CL_OK != rc)
        {
            clLogWarning(CL_LOG_AREA_MASTER, CL_LOG_CTX_FO_INIT, "Failed to clear the bit [%d]", bitNum);
        }
        pStreamData->streamMcastAddr = CL_IOC_RESERVED_ADDRESS;
        ++pArg->numStreams;
        pArg->flag = CL_TRUE;
    }
    return CL_OK;
}

static
ClRcT
clLogMasterEntryNodedownUpdate(ClLogFileDataT   *pFileData, 
                               SaNameT          *pNodeName, 
                               ClBoolT          *pFlag)
{
    ClRcT  rc = CL_OK;
    ClLogMasterWalkArgT  walkArg;

    walkArg.pNodeName  = pNodeName;
    walkArg.flag       = CL_FALSE;
    walkArg.numStreams = 0;
    rc = clCntWalk(pFileData->hStreamTable, clLogMasterStreamEntryChkNUnset,
                   (ClCntArgHandleT) &walkArg, sizeof(walkArg));
    if( CL_OK != rc )
    {
        return CL_OK;
    }
    pFileData->nActiveStreams -= walkArg.numStreams;
    *pFlag = walkArg.flag;

    return CL_OK;
}

ClRcT clLogNodeDownMasterDBUpdate(SaNameT*  nodeName)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;
    ClCntNodeHandleT    node            = CL_HANDLE_INVALID_VALUE;
    ClLogFileDataT      *pFileData      = NULL;
    ClBoolT             updatedFlag     = CL_FALSE;
    ClLogSvrCommonEoDataT *pSvrCommonEoEntry = NULL;

    clLogDebug(CL_LOG_AREA_MASTER, CL_LOG_CTX_FO_INIT, "Received node down event for node name [%.*s]", nodeName->length, nodeName->value);
    rc = clLogMasterEoEntryGet(&pMasterEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc)
    {
        if (CL_GET_ERROR_CODE(rc) ==  CL_ERR_NOT_EXIST)
          clLogDebug(CL_LOG_AREA_MASTER, CL_LOG_CTX_FO_INIT, "I cannot be master, so nothing to do");            
        return rc;
    }
    if( clIocLocalAddressGet() != pSvrCommonEoEntry->masterAddr )
    {
        return CL_OK;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( pMasterEoEntry->hMasterFileTable == CL_HANDLE_INVALID_VALUE )
    {
        /* not initialized yet, just return from here */
         CL_LOG_CLEANUP(
                 clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
         return rc;
    }
    /* Go thru the list of local streams which has come from this particular
     * node, mark those as invalid as streamowner & opener is no more valid on
     * that node 
     */
     rc = clCntFirstNodeGet(pMasterEoEntry->hMasterFileTable, &node);
     if( CL_OK != rc )
     {
         CL_LOG_CLEANUP(
                 clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);
         return rc;
     }
     while( node != CL_HANDLE_INVALID_VALUE )
     {
         rc = clCntNodeUserDataGet(pMasterEoEntry->hMasterFileTable, node, (ClCntDataHandleT *)&pFileData);
         if( CL_OK != rc )
         {
             break;
         }
         rc = clLogMasterEntryNodedownUpdate(pFileData, nodeName, &updatedFlag);
         if( CL_OK != rc )
         {
             /* Just keep on continue to look for some other entries */
         }
         if( updatedFlag == CL_TRUE )
         {
            clLogMasterDataCheckpoint(pMasterEoEntry, node,
                                           CL_FALSE);
         }
         rc = clCntNextNodeGet(pMasterEoEntry->hMasterFileTable, node, &node);
         if( CL_OK != rc )
         {
             rc  = CL_OK;
             break;
         }
     }
    CL_LOG_CLEANUP(
          clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock), CL_OK);

    return CL_OK;
}
