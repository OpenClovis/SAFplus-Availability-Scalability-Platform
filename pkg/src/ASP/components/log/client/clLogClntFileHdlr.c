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
#include <clCpmExtApi.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clLogCommon.h>
#include <clLogClientHandle.h>
#include <clLogClientEo.h>
#include <clLogClntFileHdlr.h>
#include <LogPortFileHdlrClient.h>
#include <LogPortMasterClient.h>
#include <clIocLogicalAddresses.h>

static ClBoolT  updateVersion = CL_FALSE;

ClRcT
clLogClntFileKeyCreate(ClCharT           *fileName,
                       ClCharT           *fileLocation,
                       ClLogClntFileKeyT  **pFileKey)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pFileKey = clHeapCalloc(1, sizeof(ClLogClntFileKeyT));
    if( NULL == *pFileKey )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*pFileKey)->fileName.length = strlen(fileName) + 1;
    (*pFileKey)->fileName.pValue = clHeapCalloc((*pFileKey)->fileName.length,
                                                sizeof(ClCharT));
    if( NULL == (*pFileKey)->fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(*pFileKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*pFileKey)->fileLocation.length = strlen(fileLocation) + 1;
    (*pFileKey)->fileLocation.pValue = clHeapCalloc((*pFileKey)->fileLocation.length,
                                                    sizeof(ClCharT));
    if( NULL == (*pFileKey)->fileLocation.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree((*pFileKey)->fileName.pValue);
        clHeapFree(*pFileKey);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    strncpy((*pFileKey)->fileName.pValue, fileName,
            (*pFileKey)->fileName.length);
    strncpy((*pFileKey)->fileLocation.pValue, fileLocation,
            (*pFileKey)->fileLocation.length);
          
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogClntFileKeyDestroy(ClLogClntFileKeyT  *pFileKey)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree(pFileKey->fileLocation.pValue);
    clHeapFree(pFileKey->fileName.pValue);
    clHeapFree(pFileKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

static ClRcT
clLogFileHdlrHandleCreate(ClLogHandleT       hLog,
                          ClBoolT            isDelete, 
                          ClUint32T          operatingLvl, 
                          ClLogClntFileKeyT  *pFileKey,
                          ClHandleT          *phFile)
{
    ClRcT                   rc            = CL_OK;
    ClBoolT                 entryAdd      = CL_FALSE;
    ClLogClntFileHdlrInfoT  *pData        = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCreate(pClntEoEntry->hClntHandleDB, 
                        sizeof(ClLogClntFileHdlrInfoT), phFile);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCreate(); rc[0x %x]", rc));
        if( CL_TRUE == entryAdd )
        {
            CL_LOG_CLEANUP(clHandleDatabaseDestroy(pClntEoEntry->hClntHandleDB),
                           CL_OK);
        }
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, *phFile, 
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB, *phFile),
                       CL_OK);
        if( CL_TRUE == entryAdd )
        {
            CL_LOG_CLEANUP(clHandleDatabaseDestroy(pClntEoEntry->hClntHandleDB),
                           CL_OK);
        }
        return rc;
    }
    pData->type         = CL_LOG_FILE_HANDLE;
    pData->isDelete     = isDelete;
    pData->startRead    = 0;
    pData->operatingLvl = operatingLvl;
    pData->hLog         = hLog;
    pData->pFileKey     = pFileKey; 

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, *phFile);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB, *phFile),
                       CL_OK);
        if( CL_TRUE == entryAdd )
        {
            CL_LOG_CLEANUP(clHandleDatabaseDestroy(pClntEoEntry->hClntHandleDB),
                           CL_OK);
        }
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogFileHdlrFileHandleDelete(ClLogFileHandleT  hFileHdlr,
                              ClLogHandleT      *phLog)
{
    ClRcT               rc            = CL_OK;
    ClLogClntFileHdlrInfoT  *pData        = NULL;
    ClLogClntEoDataT    *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hFileHdlr,
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    *phLog = pData->hLog;
    clLogClntFileKeyDestroy(pData->pFileKey); 

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hFileHdlr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(); rc[0x %x]", rc));
    }
    CL_LOG_CLEANUP(clHandleDestroy(pClntEoEntry->hClntHandleDB, hFileHdlr), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogInitHandleBitmapFileAdd(ClLogHandleT      hLog, 
                             ClLogFileHandleT  hFileHdlr)
{
    ClRcT                  rc            = CL_OK;
    ClLogInitHandleDataT  *pData         = NULL;
    ClLogClntEoDataT      *pClntEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
        
    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pData)); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]\n", rc));
        return rc;
    }

    rc = clBitmapBitSet(pData->hFileBitmap, hFileHdlr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                       CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogInitHandleBitmapFileRemove(ClLogHandleT      hLog, 
                                ClLogFileHandleT  hFileHdlr)
{
    ClRcT                  rc            = CL_OK;
    ClLogInitHandleDataT  *pData         = NULL;
    ClLogClntEoDataT      *pClntEoEntry  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
        
    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog,
                          (void **) (&pData)); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]\n", rc));
        return rc;
    }

    rc = clBitmapBitClear(pData->hStreamBitmap, hFileHdlr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapBitSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog),
                       CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
} 

ClRcT
clLogFileHdlrAddressGet(ClStringT      *pFileLocation,
                        ClIocAddressT  *pFileHdlrAddr)
{
    ClRcT          rc                          = CL_OK;
    ClIocAddressT  address                     = {{0}};
    ClCharT        nodeStr[CL_MAX_NAME_LENGTH] = {0};
    ClBoolT        isLogical                   = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    sscanf(pFileLocation->pValue, "%[^:]", nodeStr);

    rc = clLogAddressForLocationGet(nodeStr, &address, &isLogical);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_TRUE == isLogical )
    {
        rc = clLogMasterAddressGet(pFileHdlrAddr);
    }
    else
    {
       pFileHdlrAddr->iocPhyAddress.nodeAddress =
           address.iocPhyAddress.nodeAddress;
       pFileHdlrAddr->iocPhyAddress.portId      = 
           address.iocPhyAddress.portId;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogFileHdlrFileLocationVerify(ClCharT  *fileLocation)
{
    ClRcT                 rc                          = CL_OK;
    ClIocAddressT         address                     = {{0}};
    ClStatusT              status                      = 0;
    ClIocLogicalAddressT  logLogicalAddr              = 0;
    ClCharT               nodeStr[CL_MAX_NAME_LENGTH] = {0};
    ClBoolT               isLogical                   = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    sscanf(fileLocation, "%[^:]", nodeStr);

    rc = clLogAddressForLocationGet(nodeStr, &address, &isLogical);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_TRUE == isLogical )
    {
        logLogicalAddr = CL_IOC_LOG_LOGICAL_ADDRESS;
        if( logLogicalAddr == address.iocLogicalAddress )
        {
            return CL_OK;
        }
    }
    else
    {
        rc = clCpmNodeStatusGet( address.iocPhyAddress.nodeAddress,
                                       &status); 
        if( status == CL_STATUS_UP )
        {
            return CL_OK;
        }
    }
    CL_LOG_DEBUG_ERROR(("fileLocation doesn't have proper address"));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT
clLogFileHdlrHandleInfoGet(ClLogFileHandleT   hFileHdlr,
                           ClBoolT            *pIsDelete, 
                           ClUint32T          *pVersion, 
                           ClUint64T          *pStartRec, 
                           ClLogClntFileKeyT  **ppFileKey)
{
    ClRcT                   rc            = CL_OK;
    ClLogClntFileHdlrInfoT  *pData        = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hFileHdlr,
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }
    
    *ppFileKey = pData->pFileKey;
    *pIsDelete = pData->isDelete;
    *pStartRec = pData->startRead;
    *pVersion  = pData->operatingLvl;

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hFileHdlr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(); rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntFileHdlrHandleUpdate(ClLogFileHandleT   hFileHdlr,
                              ClUint32T          version, 
                              ClUint64T          startRec)
{
    ClRcT               rc            = CL_OK;
    ClLogClntFileHdlrInfoT  *pData        = NULL;
    ClLogClntEoDataT    *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hFileHdlr, 
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }
    
    pData->operatingLvl = version;
    pData->startRead    = startRec;
    
    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hFileHdlr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(); rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileHdlrStreamAttributesCopy(ClLogStreamAttrIDLT     *pSource,
                                  ClLogStreamAttributesT  *pDest)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pDest->fileUnitSize    = pSource->fileUnitSize;
    pDest->recordSize      = pSource->recordSize;
    pDest->haProperty      = pSource->haProperty;
    pDest->fileFullAction  = pSource->fileFullAction;
    pDest->maxFilesRotated = pSource->maxFilesRotated;
    pDest->flushFreq       = pSource->flushFreq;
    pDest->flushInterval   = pSource->flushInterval;
    pDest->waterMark       = pSource->waterMark;
    pDest->fileName     
        = clHeapCalloc(pSource->fileName.length, sizeof(ClCharT));
    if( NULL == pDest->fileName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           
    pDest->fileLocation  
        = clHeapCalloc(pSource->fileLocation.length, sizeof(ClCharT));
    if( NULL == pDest->fileLocation )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(pDest->fileName);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           
    memcpy(pDest->fileName, pSource->fileName.pValue, pSource->fileName.length);
    memcpy(pDest->fileLocation, pSource->fileLocation.pValue, 
           pSource->fileLocation.length);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamListEntryGet(ClBufferHandleT      msg,
                            ClLogStreamAttrIDLT  *pStreamAttr,
                            ClLogStreamInfoT     *pLogStream)
{
    ClRcT      rc           = CL_OK;
    ClUint32T  acitveStream = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint32T(msg, &acitveStream);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( 0 == acitveStream )
    {
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    rc = clXdrUnmarshallClNameT(msg, &pLogStream->streamName);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClNameT(msg, &pLogStream->streamScopeNode);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClUint32T(msg, &pLogStream->streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClUint32T(msg, &pLogStream->streamId);
    if( CL_OK != rc )
    {
        return rc;
    }
            
    rc = clLogFileHdlrStreamAttributesCopy(pStreamAttr, &pLogStream->streamAttr);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamListGet(ClBufferHandleT   msg,
                       ClLogStreamInfoT  *pLogStreams)
{
    ClRcT                rc           = CL_OK;
    ClLogStreamInfoT     *pTempBuffer = pLogStreams;
    ClUint32T            count        = 0;
    ClUint32T            strmCnt      = 0;
    ClUint32T            numFiles     = 0;
    ClUint32T            numStreams   = 0;
    ClUint32T            numStreamsUnpacked = 0;
    ClLogStreamAttrIDLT  streamAttr   = {{0}};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint32T(msg, &numFiles);
    if( CL_OK != rc )
    {
        return rc;
    }
    for( count = 0; count < numFiles; count++)
    {
        memset(&streamAttr, '\0', sizeof(ClLogStreamAttrIDLT));
        rc = VDECL_VER(clXdrUnmarshallClLogStreamAttrIDLT, 4, 0, 0)(msg, &streamAttr);
        if( CL_OK != rc )
        {
            break;
        }

        if(streamAttr.fileName.pValue)
            clHeapFree(streamAttr.fileName.pValue);

        if(streamAttr.fileLocation.pValue)
            clHeapFree(streamAttr.fileLocation.pValue);

        rc = clXdrUnmarshallClStringT(msg, &streamAttr.fileName); 
        if( CL_OK != rc )
        {
            break;
        }
        rc = clXdrUnmarshallClStringT(msg, &streamAttr.fileLocation);
        if( CL_OK != rc )
        {
            clHeapFree(streamAttr.fileName.pValue);
            break;
        }

        rc = clXdrUnmarshallClUint32T(msg, &numStreams);
        if( CL_OK != rc )
        {
            clHeapFree(streamAttr.fileLocation.pValue);
            clHeapFree(streamAttr.fileName.pValue);
            break;
        }

        for(strmCnt = 0; strmCnt < numStreams; strmCnt++)
        {
            rc = clLogClntStreamListEntryGet(msg, &streamAttr, pTempBuffer);
            if(CL_LOG_RC(CL_ERR_INVALID_PARAMETER) == rc )
            {
                rc = CL_OK;
                continue;
            }
            if( CL_OK != rc )
            {
                clHeapFree(streamAttr.fileLocation.pValue);
                clHeapFree(streamAttr.fileName.pValue);
                return rc;
            }
            pTempBuffer += 1;
            ++numStreamsUnpacked;
        }

        clHeapFree(streamAttr.fileLocation.pValue);
        clHeapFree(streamAttr.fileName.pValue);
    }
    /* Any failure, cleanup the filled Entries */
    if( CL_OK != rc )
    {
       for( strmCnt = 0; strmCnt < numStreamsUnpacked; strmCnt++) 
       {
         pTempBuffer = pLogStreams + strmCnt; 
         clHeapFree(pTempBuffer->streamAttr.fileName);
         clHeapFree(pTempBuffer->streamAttr.fileLocation);
         pTempBuffer->streamAttr.fileName = pTempBuffer->streamAttr.fileLocation = NULL;
         
       }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStreamListUnpack(ClUint32T         numStreams,
                          ClUint32T         buffLen,
                          ClUint8T          *pBuffer,
                          ClLogStreamInfoT  **ppLogStreams)
{
    ClRcT            rc  = CL_OK;
    ClBufferHandleT  msg = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *ppLogStreams = clHeapCalloc(numStreams, sizeof(ClLogStreamInfoT));
    if( NULL == *ppLogStreams )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        clHeapFree(*ppLogStreams);
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, buffLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        clHeapFree(*ppLogStreams);
        return rc;
    }

    rc = clLogClntStreamListGet(msg, *ppLogStreams);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        clHeapFree(*ppLogStreams);
        return rc;
    }

    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
   
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntTimePrint(ClTimeT  timetp)
{
    ClTimerTimeOutT  time                        = {0};
    time_t           timeInSec                   = 0;
    struct tm        brokenTime                  = {0};
    ClCharT          timeStr[CL_MAX_NAME_LENGTH] = {0};

    time.tsSec  = (timetp / 1000000000);
    time.tsMilliSec = (timetp % 1000000000) / 1000000;
    timeInSec  += time.tsSec + time.tsMilliSec / 1000; 
    localtime_r(&timeInSec, &brokenTime);
    if( 0 == strftime(timeStr, CL_MAX_NAME_LENGTH , "%Y-%m-%dT%H:%M:%S",
                &brokenTime) )
    {
         CL_LOG_DEBUG_ERROR(("strftime() failed"));
         return CL_OK;
    }
    CL_LOG_DEBUG_ERROR(("Time: %s", timeStr));
    return CL_OK;
}

ClRcT
clLogClntFileHdlrTimeStampGet(ClUint32T   numRecords, 
                              ClUint32T   buffLen,
                              ClUint8T    *pRecords,
                              ClTimeT     *pStartTime,
                              ClTimeT     *pEndTime)
{
    ClRcT      rc                = CL_OK;
    ClUint32T  recordSize        = 0;
    ClUint32T  offSet            = 0;
    ClUint8T   *pBuffer          = pRecords;
    ClTimeT    timeStamp         = 0;
    ClUint32T  count             = 0;
    ClUint32T  timeInSec         = 0;
    ClUint32T  timeInMilliSec    = 0;
    ClUint32T  startTimSec       = 0xFFFFFFFE;
    ClUint32T  startTimeMilliSec = 0xFFFFFFFE;
    ClUint32T  endTimSec         = 0;
    ClUint32T  endTimeMilliSec   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pEndTime   = CL_LOG_TIME_MIN_VALUE;
    *pStartTime = CL_LOG_TIME_MIN_VALUE; 
    recordSize  = buffLen / numRecords;
    offSet      = CL_LOG_CLNT_TIMESTAMP_OFFSET;
    for( count = 0; count < numRecords; count++ )
    {
        pBuffer = pRecords + (count * recordSize) + offSet;
        memcpy(&timeStamp, pBuffer, sizeof(ClTimeT));

        timeInSec = timeStamp / 1000000000L;
        timeInMilliSec = (timeStamp % 1000000000L) / 1000000L;
        
        if( (timeInSec < startTimSec) || 
            (timeInMilliSec < startTimeMilliSec) )
        {
            *pStartTime       = timeStamp;
            startTimSec       = timeInSec;
            startTimeMilliSec = timeInMilliSec;
        }
        if( (timeInSec > endTimSec) || 
            (timeInMilliSec > endTimeMilliSec) )
        {
            *pEndTime       = timeStamp;
            endTimSec       = timeInSec;
            endTimeMilliSec = timeInMilliSec;
        }
    }

    clLogClntTimePrint(*pStartTime);
    clLogClntTimePrint(*pEndTime);

    CL_LOG_DEBUG_TRACE(("StartTime: %lld  EndTime: %lld", *pStartTime,
                *pEndTime));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOpen(ClLogHandleT      hLog,
              ClCharT           *fileName,
              ClCharT           *fileLocation,
              ClBoolT           isDelete, 
              ClLogFileHandleT  *phFile)
{
    ClRcT              rc            = CL_OK;
    ClLogClntFileKeyT  *pFileKey     = NULL;
    ClUint32T          operatingLvl  = 0;
    ClIocAddressT      address       = {{0}};
    ClIdlHandleT       hLogIdl       = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == fileName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == fileLocation), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == phFile), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntFileKeyCreate(fileName, fileLocation, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileHdlrAddressGet(&pFileKey->fileLocation, &address);
    if( CL_OK != rc )
    {
        clLogClntFileKeyDestroy(pFileKey);
        return rc;
    }
    CL_LOG_DEBUG_ERROR(("FileOwner's Address: %d",
                address.iocPhyAddress.nodeAddress));

    rc = clLogClntIdlHandleInitialize(address, &hLogIdl);
    if( CL_OK != rc )
    {
        clLogClntFileKeyDestroy(pFileKey);
        return rc;
    }

    rc = VDECL_VER(clLogFileHdlrFileOpenClientSync, 4, 0, 0)(hLogIdl, &pFileKey->fileName, 
                               &pFileKey->fileLocation, &operatingLvl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogFileHdlrFileOpen, 4, 0, 0)(): rc[0x %x]", rc));
        clLogClntFileKeyDestroy(pFileKey);
        return rc;
    }

    rc = clLogFileHdlrHandleCreate(hLog, isDelete, operatingLvl, pFileKey, phFile);
    if( CL_OK != rc )
    {
        clLogClntFileKeyDestroy(pFileKey);
        return rc;
    }

    rc = clLogHandleInitHandleStreamAdd(hLog, *phFile);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileHdlrFileHandleDelete(*phFile, &hLog), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileClose(ClLogFileHandleT  hFileHdlr)
{
    ClRcT             rc   = CL_OK;
    ClLogFileHandleT  hLog = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileHdlrFileHandleDelete(hFileHdlr, &hLog);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_CLEANUP(clLogInitHandleBitmapFileRemove(hLog, hFileHdlr), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileMetaDataGet(ClLogFileHandleT        hFileHdlr,
                     ClLogStreamAttributesT  *pStreamAttr,
                     ClUint32T               *pNumStreams,
                     ClLogStreamMapT         **ppLogStreams)
{
    ClRcT                rc            = CL_OK;
    ClIocAddressT        address       = {{0}}; 
    ClIdlHandleT         hLogIdl       = CL_HANDLE_INVALID_VALUE;
    ClLogClntFileKeyT    *pFileKey     = NULL;
    ClLogStreamAttrIDLT  streamAttrIdl = {{0}};

    ClBoolT              isDelete      = CL_FALSE;
    ClUint32T            version       = 0;
    ClUint64T            startRec      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pNumStreams), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppLogStreams), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogFileHdlrHandleInfoGet(hFileHdlr, &isDelete, &version, 
                                    &startRec, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileHdlrAddressGet(&pFileKey->fileLocation, &address);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_DEBUG_ERROR(("FileOwner's Address: %d",
                address.iocPhyAddress.nodeAddress));

    rc = clLogClntIdlHandleInitialize(address, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_ERROR(("fileName: %s fileLoc: %s", pFileKey->fileName.pValue, 
                                                    pFileKey->fileLocation.pValue));
    
    rc = VDECL_VER(clLogFileHdlrFileMetaDataGetClientSync, 4, 0, 0)(hLogIdl, &pFileKey->fileName,
                                      &pFileKey->fileLocation,
                                      &version, &streamAttrIdl, 
                                      pNumStreams, ppLogStreams); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFileHdlrFileMetaDataGet(): rc[0x %x]",
                            rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_ERROR(("NumStreams: %d", *pNumStreams));

    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
    {
        rc = clLogClntFileHdlrHandleUpdate(hFileHdlr, version, 0);
        if( CL_OK != rc )
        {
            clHeapFree(streamAttrIdl.fileLocation.pValue);
            clHeapFree(streamAttrIdl.fileName.pValue);
            clHeapFree(ppLogStreams);
            return rc;
        }
        updateVersion = CL_TRUE;
    }

    rc = clLogFileHdlrStreamAttributesCopy(&streamAttrIdl, pStreamAttr);
    if( CL_OK != rc )
    {
        clHeapFree(streamAttrIdl.fileLocation.pValue);
        clHeapFree(streamAttrIdl.fileName.pValue);
        clHeapFree(ppLogStreams);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileRecordsGet(ClLogFileHandleT  hFileHdlr,
                    ClTimeT           *pStartTime,
                    ClTimeT           *pEndTime, 
                    ClUint32T         *pNumRecords,
                    ClPtrT            *pLogRecords)
{
    ClRcT              rc        = CL_OK;
    ClIocAddressT      address   = {{0}};
    ClIdlHandleT       hLogIdl   = CL_HANDLE_INVALID_VALUE;
    ClLogClntFileKeyT  *pFileKey = NULL;
    ClBoolT            isDelete  = CL_FALSE;
    ClUint32T          version   = 0;
    ClUint64T          startRec  = 0;
    ClUint32T          buffLen   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pNumRecords), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pLogRecords), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogFileHdlrHandleInfoGet(hFileHdlr, &isDelete, &version, 
                                    &startRec, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileHdlrAddressGet(&pFileKey->fileLocation, &address);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntIdlHandleInitialize(address, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_ERROR(("fileName: %s fileLoc: %s", pFileKey->fileName.pValue, 
                                                    pFileKey->fileLocation.pValue));

    CL_LOG_DEBUG_ERROR(("Version: %d isDelete: %d startRec: %llu", version, 
                                                   isDelete, startRec));
    
    rc = VDECL_VER(clLogFileHdlrFileRecordsGetClientSync, 4, 0, 0)(hLogIdl, &pFileKey->fileName,
                                     &pFileKey->fileLocation, isDelete, 
                                     &startRec, &version, pNumRecords, &buffLen, 
                                     (ClUint8T **) pLogRecords); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrFileHdlrFileMetaDataGet(): rc[0x %x]",
                            rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("NumRecords: %d", *pNumRecords));
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
    if( (CL_FALSE == isDelete))
    {
       rc = clLogClntFileHdlrHandleUpdate(hFileHdlr, version, startRec); 
       if( CL_OK != rc )
       {
           return rc;
       }
       updateVersion = CL_TRUE;
    }

    rc = clLogClntFileHdlrTimeStampGet(*pNumRecords, buffLen, *pLogRecords, 
                                       pStartTime, pEndTime);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntFileHdlrTimeStampGet(): rc[0x %x]",
                    rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamListGet(ClLogHandleT      hLog,
                   ClUint32T         *pNumStreams,
                   ClLogStreamInfoT  **ppLogStreams)
{
    ClRcT          rc       = CL_OK;
    ClIocAddressT  address  = {{0}};
    ClIdlHandleT   hLogIdl  = CL_HANDLE_INVALID_VALUE;
    ClUint8T       *pBuffer = NULL;
    ClUint32T      buffLen  = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pNumStreams), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppLogStreams), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogMasterAddressGet(&address);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntIdlHandleInitialize(address, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = VDECL_VER(clLogMasterStreamListGetClientSync, 4, 0, 0)(hLogIdl, pNumStreams, &buffLen, &pBuffer); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterStreamListGet, 4, 0, 0)(): rc[0x %x]",
                            rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("NumRecords: %d", *pNumStreams));
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);

    if(!*pNumStreams)
    {
        if(pBuffer)
            clHeapFree(pBuffer);
        if(ppLogStreams)
            *ppLogStreams = NULL;
        return CL_OK;
    }

    rc = clLogClntStreamListUnpack(*pNumStreams, buffLen, pBuffer,
                                    ppLogStreams);
    if( CL_OK != rc )
    {
        clHeapFree(pBuffer);
        return rc;
    }
    clHeapFree(pBuffer);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
