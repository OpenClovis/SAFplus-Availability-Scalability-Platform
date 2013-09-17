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
#include <clLogCommon.h>
#include <clLogSvrCommon.h>
#include <clLogServer.h>

extern const SaNameT gDefaultStreamName;
extern const ClCharT gStreamScopeGlobal[];
extern const ClCharT gLogDefaultShmName[];

#define CL_LOG_DEFAULT_STREAMS     128

static ClRcT
clLogSvrStreamDefaultEntryCreate(ClLogSvrStreamDataT  **ppStreamData);

static ClRcT
clLogSvrStreamDataFree(ClLogSvrStreamDataT  *pStreamData);

ClRcT
clLogSvrDefaultStreamCreate(void)
{
    ClRcT                  rc              = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry    = NULL;
    ClLogStreamKeyT        *pStreamKey     = NULL;
    SaNameT                scopeNode       = {0};
    ClLogSvrStreamDataT    *pStreamData    = NULL;

    CL_LOG_DEBUG_ERROR(("Enter"));
    
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pSvrEoEntry->hSvrStreamTable )
    {
        rc = clCntHashtblCreate(pCommonEoEntry->maxStreams,
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

    scopeNode.length = strlen(gStreamScopeGlobal);
    memcpy(scopeNode.value, gStreamScopeGlobal, scopeNode.length);
    
    rc = clLogStreamKeyCreate((SaNameT *) &gDefaultStreamName, &scopeNode,
                               CL_LOG_DEFAULT_STREAMS, &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogSvrStreamDefaultEntryCreate(&pStreamData);
    if( CL_OK != rc )
    {
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    rc = clCntNodeAdd(pSvrEoEntry->hSvrStreamTable, 
                      (ClCntKeyHandleT) pStreamKey, 
                      (ClCntDataHandleT) pStreamData, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogSvrStreamDataFree(pStreamData), CL_OK);
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    rc = clLogSvrFlusherThreadCreateNStart(pStreamData,
                                           &pStreamData->flusherId);
    if( CL_OK != rc )
    {
        /* will internally delete the key and streamData also */
        CL_LOG_CLEANUP(clCntAllNodesForKeyDelete(pSvrEoEntry->hSvrStreamTable, 
                                                 (ClCntKeyHandleT) pStreamKey),
                       CL_OK);
        return rc;
    }
    
    CL_LOG_DEBUG_ERROR(("Exit"));
    return rc;
}

static ClRcT
clLogSvrStreamDataFree(ClLogSvrStreamDataT  *pStreamData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_ERROR(("Enter"));

    clHeapFree(pStreamData->shmName.pValue);
    clHeapFree(pStreamData);

    CL_LOG_DEBUG_ERROR(("Exit"));
    return rc;
}

static ClRcT
clLogSvrStreamDefaultEntryCreate(ClLogSvrStreamDataT  **ppStreamData)
{
    ClRcT               rc             = CL_OK;
    ClLogStreamHeaderT  *pStreamHeader = NULL;
    ClUint32T           maxMsgBytes    = 0;
    ClUint32T           maxCompBytes   = 0;
    ClUint32T           headerSize     = 0;
    ClBoolT             createShm      = 0;

    CL_LOG_DEBUG_ERROR(("Enter"));
   
    (*ppStreamData) = clHeapCalloc(1, sizeof(ClLogSvrStreamDataT)); 
    if( NULL == *ppStreamData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    (*ppStreamData)->dsId           = CL_LOG_INVALID_DSID;
    (*ppStreamData)->shmName.length = strlen(gLogDefaultShmName) + 1;
    (*ppStreamData)->shmName.pValue = clHeapCalloc(
                    (*ppStreamData)->shmName.length, sizeof(ClCharT));
    if( NULL == *ppStreamData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(*ppStreamData);
        return rc;
    }
    memcpy((*ppStreamData)->shmName.pValue, gLogDefaultShmName,
            (*ppStreamData)->shmName.length);    

    rc = clLogDefaultShmOpen(&(*ppStreamData)->shmName, &pStreamHeader, 
                             &createShm);
    if( CL_OK != rc )
    {
        clHeapFree((*ppStreamData)->shmName.pValue);
        clHeapFree(*ppStreamData);
        return rc;
    }
    (*ppStreamData)->pStreamHeader   = pStreamHeader;

    maxMsgBytes  = CL_LOG_DEFAULT_MAX_MSGS / CL_LOG_BITS_IN_BYTE + 1;
    maxCompBytes = CL_LOG_DEFAULT_MAX_COMPS / CL_LOG_BITS_IN_BYTE + 1;
    headerSize   = maxCompBytes + maxCompBytes + sizeof(ClLogStreamHeaderT);
    (*ppStreamData)->pStreamRecords  = ((ClUint8T *) pStreamHeader) + headerSize;
    
    CL_LOG_DEBUG_ERROR(("Exit"));
    return rc;
}

ClRcT
clLogSvrDefaultStreamDestroy(void)
{
    clOsalPrintf("Alert ! Not yet implemented \n");
    return CL_OK;
}
