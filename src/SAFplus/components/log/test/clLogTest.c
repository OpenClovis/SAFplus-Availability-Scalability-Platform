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
#include <clHeapApi.h>
#include <clHandleApi.h>
#include <clLogTest.h>

ClTcLogParamsT  gTestInit;

#define  CL_LOG_TST_TIME_INTVAL     1000000000   /*1 sec */

#define  CL_LOG_DEFAULT_SERVICE_ID  10

#define  CL_LOG_MSGID_TLV           2 
#define  CL_LOG_VERSION { 'B', 0x01, 0x01 }

ClRcT
clTcLogSvcInit(ClLogHandleT  *phLog)
{
    ClRcT       rc      = CL_OK;
    ClVersionT  version = CL_LOG_VERSION;

    memcpy(&gTestInit.version, &version, sizeof(ClVersionT)); 
    rc = clLogInitialize(phLog, NULL, &gTestInit.version);
    if( CL_OK != rc )
    {
        /* coder can't help it */
        return  rc;
    }
    return CL_OK;
}

ClRcT
clTcLogStreamOpen(ClLogHandleT      hLog,
                  ClCharT           *pStreamName,
                  ClLogStreamScopeT streamScope, 
                  ClCharT           *fileName,
                  ClCharT           *fileLocation,
                  ClLogOpenParamsT  *pOpenParams,
                  ClUint32T         flushFreq,
                  ClLogTstDataT     *pLogTestData)
{
    ClRcT                   rc         = CL_OK;
    ClLogStreamAttributesT  streamAttr = {0};
    SaNameT                 streamName = {0};

    /* initializing the stream attributes with the given values */
    rc = clLogTestStreamAttributesInit(fileName, fileLocation,
                                       pOpenParams->fileSize,
                                       pOpenParams->recordSize,
                                       pOpenParams->fileAction,
                                       pOpenParams->maxFiles,
                                       flushFreq, 
                                       CL_LOG_TST_TIME_INTVAL, 
                                       gTestInit.waterMark,
                                       &streamAttr);
    if( CL_OK != rc )
    {
        return rc;
    }

    streamName.length = strlen(pStreamName);
    memcpy(streamName.value, pStreamName, streamName.length);
    /* open the stream */
    rc = clLogStreamOpen(hLog, streamName, streamScope, 
                         &streamAttr, CL_LOG_STREAM_CREATE, 0,
                         &pLogTestData->hStream);

    /* irrespective RC free the mem alloc */
    clHeapFree(streamAttr.fileLocation);
    clHeapFree(streamAttr.fileName);

    return rc;
}

ClRcT
clLogTestStreamAttributesInit(const ClCharT                 *fileName,
                              const ClCharT                 *fileLocation,
                              ClUint32T               fileUnitSize,
                              ClUint32T               recordSize,
                              ClLogFileFullActionT    fileFullAction,
                              ClUint32T               maxFilesRotated,
                              ClUint32T               flushFreq,
                              ClTimeT                 flushInterval,
                              ClWaterMarkT            waterMark,
                              ClLogStreamAttributesT  *pStreamAttr)
{

    if( NULL != fileName )
    {
        /* copying the filename */
        if( NULL == ( pStreamAttr->fileName = (ClCharT*) clHeapCalloc(1, strlen(fileName)+1)))
        {
            return CL_ERR_NO_MEMORY;
        }
        strcpy(pStreamAttr->fileName, fileName);
    }
    if( NULL != fileLocation)
    {
       if( NULL == ( pStreamAttr->fileLocation = (ClCharT*) clHeapCalloc(1, strlen(fileLocation)+1)))
        {
            clHeapFree(pStreamAttr->fileName);
            return CL_ERR_NO_MEMORY;
        }        
       strncpy(pStreamAttr->fileLocation, fileLocation, strlen(fileLocation));
    }
    pStreamAttr->haProperty         = 0;
    pStreamAttr->fileUnitSize       = fileUnitSize;
    pStreamAttr->recordSize         = recordSize;
    pStreamAttr->fileFullAction     = fileFullAction;
    pStreamAttr->maxFilesRotated    = maxFilesRotated;
    pStreamAttr->flushFreq          = flushFreq;
    pStreamAttr->flushInterval      = flushInterval;
    pStreamAttr->waterMark          = waterMark;

    return CL_OK;
}    

ClRcT
clTcLogStreamClose(ClLogTstDataT  *pTestData)
{
    ClRcT  rc = CL_OK;

    rc = clLogStreamClose(pTestData->hStream);

    return rc;
}

ClRcT
clTcLogSvcFinalize(ClLogHandleT hLog)
{
    ClRcT  rc = CL_OK;

    rc = clLogFinalize(hLog);
    gTestInit.hLog = CL_HANDLE_INVALID_VALUE;

    return rc;
}

ClRcT
clTcLogWrite(ClLogTstDataT    *pLogTestData, 
             ClLogWriteTypeT  writeType,
             ClLogSeverityT   severity, 
             ClCharT          *pLogStr)
{
    ClRcT      rc  = CL_OK;
    ClUint32T  num = 32;

    switch(writeType)
    {
        case CL_LOG_TST_BUFFER:
            rc = clLogWriteAsync(pLogTestData->hStream, severity,
                                 CL_LOG_DEFAULT_SERVICE_ID,
                                 CL_LOG_MSGID_BUFFER, 
                                 pLogStr, strlen(pLogStr));
            break;
        case CL_LOG_TST_ASCII:
            rc = clLogWriteAsync(pLogTestData->hStream, severity,
                                 CL_LOG_DEFAULT_SERVICE_ID,
                                 CL_LOG_MSGID_PRINTF_FMT, 
                                 pLogStr);
            break;
        case CL_LOG_TST_TLV:
            rc = clLogWriteAsync(pLogTestData->hStream, severity,
                                 CL_LOG_DEFAULT_SERVICE_ID,
                                 CL_LOG_MSGID_TLV, 
                                 CL_LOG_TLV_STRING(pLogStr),
                                 CL_LOG_TLV_UINT32(num),
                                 CL_LOG_TAG_TERMINATE);
            break;
    }

    return rc;
}

