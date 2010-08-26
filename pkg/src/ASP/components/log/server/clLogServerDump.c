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
#include <clOsalApi.h>
#include <clHandleApi.h>
#include <clEoApi.h>
#include <clBitmapApi.h>

#include <clLogServerDump.h>
#include <clLogDebug.h>
#include <clLogSvrCommon.h>
#include <clLogStreamOwnerEo.h>
#include <clLogStreamOwner.h>
#include <xdrClLogCompKeyT.h>
#include <clLogSvrEo.h>
#include <clLogOsal.h>

void 
clLogStreamOwnerAttributesPrint(ClLogStreamAttrIDLT  *pStreamAttr,
                                ClDebugPrintHandleT  *msg)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clDebugPrint(*msg, "fileName       : %s\n", pStreamAttr->fileName.pValue);
    clDebugPrint(*msg, "fileNameLength : %d\n", pStreamAttr->fileName.length);
    clDebugPrint(*msg, "fileLocation   : %s\n", pStreamAttr->fileLocation.pValue);
    clDebugPrint(*msg, "fileLocationLen: %d\n", pStreamAttr->fileName.length);
    clDebugPrint(*msg, "FileUnitSize   : %d\n", pStreamAttr->fileUnitSize);
    clDebugPrint(*msg, "recordSize     : %d\n", pStreamAttr->recordSize);
    clDebugPrint(*msg, "maxFilesRotated: %d\n", pStreamAttr->maxFilesRotated);
    clDebugPrint(*msg, "fileFullAction : %d\n", pStreamAttr->fileFullAction);
    clDebugPrint(*msg, "haProperty     : %d\n", pStreamAttr->haProperty);
    clDebugPrint(*msg, "waterMark(low) : %lld\n", pStreamAttr->waterMark.lowLimit);
    clDebugPrint(*msg, "waterMark(high): %lld\n", pStreamAttr->waterMark.highLimit);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogCompTableWalkForPrint(ClCntKeyHandleT   key,
                           ClCntDataHandleT  data,
                           ClCntArgHandleT   arg,
                           ClUint32T         size)
{
    ClRcT                rc        = CL_OK;
    ClLogCompKeyT        *pCompKey = (ClLogCompKeyT *) key;
    ClUint32T            *pData    = (ClUint32T *) data;
    ClDebugPrintHandleT  msg       = *((ClDebugPrintHandleT *) arg);
        
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    clDebugPrint(msg, "NodeAddress:  %d \n", pCompKey->nodeAddr);
    clDebugPrint(msg, "CompId     :  %d \n", pCompKey->compId);
    clDebugPrint(msg, "RefCount   :  %d \n", *pData);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSOTableWalkForPrint(ClCntKeyHandleT   key,
                         ClCntDataHandleT  data,
                         ClCntArgHandleT   arg,
                         ClUint32T         size)
{
    ClRcT                  rc                = CL_OK;
    ClLogStreamOwnerDataT  *pStreamOwnerData = (ClLogStreamOwnerDataT *) data;
    ClLogStreamKeyT        *pStreamKey       = (ClLogStreamKeyT *) key;
    ClDebugPrintHandleT    msg               = *((ClDebugPrintHandleT *) arg);

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pStreamOwnerData->nodeLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    clDebugPrint(msg, "StreamKey:\n");
    clDebugPrint(msg, "StreamName    : %s\n", pStreamKey->streamName.value);
    clDebugPrint(msg, "StreamNode    : %s\n", pStreamKey->streamScopeNode.value);
    clDebugPrint(msg, "streamNameLen : %d\n", pStreamKey->streamName.length);
    clDebugPrint(msg, "streamScopeLen: %d\n", pStreamKey->streamScopeNode.length);
    clDebugPrint(msg, "--------------------------------------------------\n");
    clDebugPrint(msg, "StreamOwnerData:\n");
    clDebugPrint(msg, "streamId      : %d\n", pStreamOwnerData->streamId);
    clDebugPrint(msg, "MulticastAddr : %lld\n", pStreamOwnerData->streamMcastAddr);
    clDebugPrint(msg, "nodeStatus    : %d\n", pStreamOwnerData->nodeStatus);
    clDebugPrint(msg, "dataSetId     : %d\n", pStreamOwnerData->dsId);
    clDebugPrint(msg, "isNewStream   : %d\n", pStreamOwnerData->isNewStream);
    clLogStreamOwnerAttributesPrint(&pStreamOwnerData->streamAttr, arg);
    rc = clCntWalk(pStreamOwnerData->hCompTable, clLogCompTableWalkForPrint,
                   arg, 0);
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pStreamOwnerData->nodeLock), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static void
clLogStreamOwnerEoDataPrint(ClLogSOEoDataT       *pSoEoEntry,
                            ClDebugPrintHandleT  *msg)
{
    ClUint8T   *pDsIdMap = NULL;  
    ClUint32T  length    = 0;
    ClUint32T  nBytes    = 0;
    ClUint32T  count     = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    clDebugPrint(*msg, "G StreamTable  : %p\n", (void *)
            pSoEoEntry->hGStreamOwnerTable);
    clDebugPrint(*msg, "L StreamTable  : %p\n", (void *)
            pSoEoEntry->hLStreamOwnerTable);
    clDebugPrint(*msg, "G StreamMutex  : %p\n", (void *)
            &pSoEoEntry->gStreamTableLock);
    clDebugPrint(*msg, "L StreamMutex  : %p\n", (void *)
            &pSoEoEntry->lStreamTableLock);
    clDebugPrint(*msg, "G StreamTable  : %#llX\n", 
            pSoEoEntry->hCkpt);
    clDebugPrint(*msg, "Num of dataSets: %d\n", pSoEoEntry->dsIdCnt);
    clDebugPrint(*msg, "DataSet Ids:\n");
    clBitmap2BufferGet(pSoEoEntry->hDsIdMap, &length, &pDsIdMap);
    if( NULL != pDsIdMap )
    {
        nBytes = length / CL_BITS_PER_BYTE;
        nBytes++;
        for( count = 0; count < nBytes; count++)
        {
            clDebugPrint(*msg, "%d |", *(pDsIdMap + count));
        }
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return ; 
}

void
clLogStreamOwnerDataDump(ClCharT **retval)
{
    ClRcT                  rc              = CL_OK;
    ClDebugPrintHandleT    msg             = 0;
    ClLogSOEoDataT         *pSoEoEntry     = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    clDebugPrintInitialize(&msg);

    rc = clLogStreamOwnerEoEntryGet(&pSoEoEntry, &pCommonEoEntry);
    if( CL_OK != rc )
    {
        return ;
    }    
    rc = clLogSOLock(pSoEoEntry, CL_LOG_STREAM_GLOBAL);
    if( CL_OK != rc )
    {
        return ;
    }
    clLogStreamOwnerEoDataPrint(pSoEoEntry, &msg);

    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hGStreamOwnerTable )
    {
        rc = clCntWalk(pSoEoEntry->hGStreamOwnerTable, clLogSOTableWalkForPrint, 
                &msg, 0);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_GLOBAL), CL_OK);
            return ;
        }
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_GLOBAL), CL_OK);

    rc = clLogSOLock(pSoEoEntry, CL_LOG_STREAM_LOCAL);
    if( CL_OK != rc )
    {
        return ;
    }
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hLStreamOwnerTable )
    {
        rc = clCntWalk(pSoEoEntry->hLStreamOwnerTable, clLogSOTableWalkForPrint, 
                &msg, 0);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_LOCAL), CL_OK);
            return ;
        }
    }
    CL_LOG_CLEANUP(clLogSOUnlock(pSoEoEntry, CL_LOG_STREAM_LOCAL), CL_OK);

    clDebugPrintFinalize(&msg, retval);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}



/* Server Eo Data Dump */
ClRcT
clLogSvrEoDataDump(ClCharT  **ret)
{
    ClRcT                rc           = CL_OK;
    ClDebugPrintHandleT  msg          = 0;
    ClLogSvrEoDataT      *pSvrEoEntry = NULL;

	CL_LOG_DEBUG_TRACE(("Enter"));

    clDebugPrintInitialize(&msg);
    
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x%x]", rc));
        return rc;
    }
    
    clDebugPrint(msg, "\nServer EO data \n");

    clDebugPrint(msg, "Cpm Handle: %llu\n", pSvrEoEntry->hCpm);

    clDebugPrint(msg, "Log Component id: %ud\n", pSvrEoEntry->logCompId);

    clDebugPrint(msg, "Handle to Server Stream table: %p\n",
                  pSvrEoEntry->hSvrStreamTable);

#ifndef __KERNEL__
    clDebugPrint(msg, "Table Lock id: %p\n", (void *) &pSvrEoEntry->svrStreamTableLock);
#else
    clDebugPrint(msg, "Table Lock id: %ud\n", &pSvrEoEntry->svrStreamTableLock);
#endif

    clDebugPrint(msg, "Next valid Dataset id: %ud\n", pSvrEoEntry->nextDsId);
    clDebugPrint(msg, "Bitmap Handle: %p\n", (void *) pSvrEoEntry->hDsIdMap);
    clDebugPrint(msg, "Water mark for flushing - limit: %u\n", pSvrEoEntry->maxFlushLimit);

    clDebugPrintFinalize(&msg, ret);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}
