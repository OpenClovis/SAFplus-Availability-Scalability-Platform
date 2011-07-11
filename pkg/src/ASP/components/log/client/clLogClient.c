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
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include <clVersionApi.h>
#include <clCpmExtApi.h>
#include <xdrClLogStreamAttrIDLT.h>
#include <LogPortSvrClient.h>
#include <LogPortStreamOwnerClient.h>
#include <clLogErrors.h>
#include <clLogCommon.h>
#include <clLogDebug.h>
#include <clLogClientEo.h>
#include <clLogClientHandle.h>
#include <clLogClientHandler.h>
#include <clLogClientStream.h>
#include <clLogClient.h>
#include <LogPortMasterClient.h>
#include <clLogApiExt.h>

/* Supported Client Version */
static ClVersionT gLogClntVersionsSupported[] = {CL_LOG_CLIENT_VERSION};

/* Supported Client Version database */
static ClVersionDatabaseT gLogClntVersionDb = {
    sizeof(gLogClntVersionsSupported) / sizeof(ClVersionT),
    gLogClntVersionsSupported
};

static ClRcT
clLogStreamAttributesCopy(ClLogStreamAttributesT  *pCreateAttr,
                          ClLogStreamAttrIDLT     *pAttrIDL);
static ClRcT
clLogStreamNameValidate(ClNameT  *pStreamName);

static ClRcT
clLogStreamFileParamValidate(ClCharT  *fileName,
                             ClCharT  *fileLocation);

static ClRcT
clLogStreamAttrValidate(ClLogStreamAttributesT  *pAttr);

static ClRcT
clLogStreamOpenParamValidate(ClLogHandleT            hLog,
                             ClNameT                 *pStreamName,
                             ClLogStreamScopeT       streamScope,
                             ClLogStreamAttributesT  *pStreamAttr,
                             ClLogStreamOpenFlagsT   streamOpenFlags,
                             ClTimeT                 timeout,
                             ClLogStreamHandleT      *phStream);

ClCharT*
clLogSeverityStrGet(ClLogSeverityT severity)
{
    if( severity == CL_LOG_SEV_EMERGENCY )
    {
        return "EMRGN";
    }
    else if( severity == CL_LOG_SEV_ALERT )
    {
        return "ALERT";
    }
    else if( severity == CL_LOG_SEV_CRITICAL )
    {
        return "CRITIC";
    }
    else if( severity == CL_LOG_SEV_ERROR )
    {
        return "ERROR";
    }
    else if( severity == CL_LOG_SEV_WARNING )
    {
        return "WARN";
    }
    else if( severity == CL_LOG_SEV_NOTICE )
    {
        return "NOTICE";
    }
    else if( severity == CL_LOG_SEV_INFO )
    {
        return "INFO";
    }
    else if( severity == CL_LOG_SEV_DEBUG )
    {
        return "DEBUG";
    }
    else if( severity == CL_LOG_SEV_TRACE )
    {
        return "TRACE";
    }
    return "DEBUG";
}

static ClRcT
clLogStreamAttributesCopy(ClLogStreamAttributesT  *pCreateAttr,
                          ClLogStreamAttrIDLT     *pAttrIDL)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pCreateAttr);
    CL_ASSERT(NULL != pAttrIDL);

    pAttrIDL->fileName.length     = strlen(pCreateAttr->fileName) + 1;
    pAttrIDL->fileLocation.length = strlen(pCreateAttr->fileLocation) + 1;
    pAttrIDL->fileUnitSize        = pCreateAttr->fileUnitSize;
    pAttrIDL->recordSize          = pCreateAttr->recordSize;
    pAttrIDL->haProperty          = pCreateAttr->haProperty;
    pAttrIDL->fileFullAction      = pCreateAttr->fileFullAction;
    pAttrIDL->maxFilesRotated     = pCreateAttr->maxFilesRotated;
    pAttrIDL->flushFreq           = pCreateAttr->flushFreq;
    pAttrIDL->flushInterval       = pCreateAttr->flushInterval;
    pAttrIDL->waterMark           = pCreateAttr->waterMark;
    pAttrIDL->syslog              = pCreateAttr->syslog;
    pAttrIDL->fileName.pValue
        = clHeapCalloc(pAttrIDL->fileName.length, sizeof(ClCharT));
    if( NULL == pAttrIDL->fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clLogFileLocationFindNGet(pCreateAttr->fileLocation,
            &pAttrIDL->fileLocation);
    if( CL_OK != rc )
    {
        clHeapFree(pAttrIDL->fileName.pValue);
        pAttrIDL->fileName.pValue = NULL;
        return rc;
    }
    memcpy(pAttrIDL->fileName.pValue, pCreateAttr->fileName,
           pAttrIDL->fileName.length);

    CL_LOG_DEBUG_TRACE(("Exit: %s %s", pAttrIDL->fileName.pValue,
                        pAttrIDL->fileLocation.pValue));
    return rc;
}

static ClRcT
clLogStreamNameValidate(ClNameT  *pStreamName)
{
    ClRcT      rc = CL_OK;
    ClUint32T  i  = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pStreamName);

    if( (CL_LOG_STREAM_NAME_MAX_LENGTH < pStreamName->length)  || 
        (0 == pStreamName->length) )
    {
        CL_LOG_DEBUG_ERROR(("Invalid length for streamName: %d",
                            pStreamName->length));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    for( i = 0; i < pStreamName->length; i++ )
    {
        if( (!isalnum(pStreamName->value[i])) && ('_' != pStreamName->value[i])
             && ('-' != pStreamName->value[i]) )
        {
            CL_LOG_DEBUG_ERROR(("Invalid Name: %.*s", pStreamName->length,
                                pStreamName->value));
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit: StreamName: %.*s", pStreamName->length,
                        pStreamName->value));
    return rc;
}

static ClRcT
clLogStreamFileParamValidate(ClCharT  *fileName,
                             ClCharT  *fileLocation)
{
    ClRcT      rc  = CL_OK;
    ClUint32T  i   = 0;
    ClUint32T  len = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != fileName);
    CL_ASSERT(NULL != fileLocation);

    len = strlen(fileName);

    if(0 == len)
    {
        clLogError("LOG", NULL, "%s", "File name should not be blank");
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    for( i = 0; i < len; i++ )
    {
        if( (!isalnum(fileName[i])) && ('_' != fileName[i])
            && ('-' != fileName[i]) && ('.' != fileName[i]) )
        {
            CL_LOG_DEBUG_ERROR(("Invalid fileName: %.*s", i, fileName));
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
    }
    CL_LOG_DEBUG_VERBOSE(("fileName: %s", fileName));

    len = strlen(fileLocation);
    for( i = 0; i < len; i++ )
    {
        if( !isprint(fileLocation[i]) )
        {
            CL_LOG_DEBUG_ERROR(("Invalid fileLoc: %.*s", i, fileLocation));
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
    }
    CL_LOG_DEBUG_VERBOSE(("fileLoc: %s", fileLocation));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogStreamAttrValidate(ClLogStreamAttributesT  *pAttr)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pAttr);

    CL_LOG_PARAM_CHK((NULL == pAttr->fileName),
                      CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((NULL == pAttr->fileLocation),
                      CL_LOG_RC(CL_ERR_INVALID_PARAMETER));

    rc = clLogStreamFileParamValidate(pAttr->fileName, pAttr->fileLocation);
    if( CL_OK != rc )
    {
        return rc;
    }

    if(CL_LOG_RECORD_HEADER_SIZE >= pAttr->recordSize)
    {
        clLogError("LOG", NULL, "%s", "Record size should be greater than "
                                       "the record header size (20 bytes)");
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    if( (0 != pAttr->fileUnitSize) && 
        ((pAttr->fileUnitSize / pAttr->recordSize) < 2) )
    {
        CL_LOG_DEBUG_ERROR(("FileSize should be twice greater than"
                            "RecordSize"));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_VERBOSE(("recordSize: %u", pAttr->recordSize));

    if( (0 != pAttr->fileUnitSize) &&
        (pAttr->fileUnitSize < pAttr->recordSize) )
    {
        CL_LOG_DEBUG_ERROR(("recordSize > fileSize: %u", pAttr->fileUnitSize));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    CL_LOG_DEBUG_VERBOSE(("fileUnitSize: %u", pAttr->fileUnitSize));

    if( (pAttr->fileFullAction != CL_LOG_FILE_FULL_ACTION_ROTATE) &&
        (pAttr->fileFullAction != CL_LOG_FILE_FULL_ACTION_WRAP)   &&
        (pAttr->fileFullAction != CL_LOG_FILE_FULL_ACTION_HALT))
    {
        CL_LOG_DEBUG_ERROR(("Invalid fileFullAct: %d", pAttr->fileFullAction));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    CL_LOG_DEBUG_VERBOSE(("fileFullAct: %u", pAttr->fileFullAction));

    CL_LOG_PARAM_CHK((0 > pAttr->flushInterval), CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    
    /* Flow frequency and flush interval both can't be 0 */
    if( (0 == pAttr->flushFreq) && (0 == pAttr->flushInterval) )
    {
        CL_LOG_DEBUG_ERROR(("Invalid Flush Parameter: freq: %u interval: %lld",
                            pAttr->flushFreq, pAttr->flushInterval));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    if( 0 ==  pAttr->flushFreq )
    {
        pAttr->flushInterval = ((pAttr->flushInterval/1000000L) < 500)
                                ? (500 * 1000 * 1000L)
                                : pAttr->flushInterval;
    }
     
    CL_LOG_DEBUG_VERBOSE(("Freq: %u Interval: %lld", pAttr->flushFreq,
                          pAttr->flushInterval));

    CL_LOG_PARAM_CHK((100 < (pAttr->waterMark).highLimit), CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((100 < (pAttr->waterMark).lowLimit), CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((((pAttr->waterMark).lowLimit) > (pAttr->waterMark).highLimit), 
                       CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((0 != pAttr->haProperty) && (1 != pAttr->haProperty), 
                     CL_LOG_RC(CL_ERR_INVALID_PARAMETER));

    if((CL_LOG_FILE_FULL_ACTION_WRAP == pAttr->fileFullAction || 
            CL_LOG_FILE_FULL_ACTION_HALT == pAttr->fileFullAction) 
            && (0 != pAttr->maxFilesRotated))
    {
        pAttr->maxFilesRotated = 0;
        clLogInfo("LOG","STM", "%s", "Overriding 'maxFilesRotated' attribute "
                "of stream to '0' in case of WRAP or HALT");
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogStreamOpenParamValidate(ClLogHandleT            hLog,
                             ClNameT                 *pStreamName,
                             ClLogStreamScopeT       streamScope,
                             ClLogStreamAttributesT  *pStreamAttr,
                             ClLogStreamOpenFlagsT   streamOpenFlags,
                             ClTimeT                 timeout,
                             ClLogStreamHandleT      *phStream)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamNameValidate(pStreamName);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( (CL_LOG_STREAM_LOCAL  != streamScope) &&
        (CL_LOG_STREAM_GLOBAL != streamScope) )
    {
        CL_LOG_DEBUG_ERROR(("Invalid scope: %d", streamScope));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    CL_LOG_DEBUG_VERBOSE(("streamScope: %d", streamScope));

    if( (0 != streamOpenFlags) && (CL_LOG_STREAM_CREATE != streamOpenFlags) )
    {
        CL_LOG_DEBUG_ERROR(("Invalid open flags: %d", streamOpenFlags));
        return CL_LOG_RC(CL_ERR_BAD_FLAG);
    }
    CL_LOG_DEBUG_VERBOSE(("streamOpenFlags: %d", streamScope));

    if( (0 == streamOpenFlags) && (NULL != pStreamAttr) )
    {
        CL_LOG_DEBUG_ERROR(("Attribtes specified with open"));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    if( (streamOpenFlags == CL_LOG_STREAM_CREATE) && (NULL == pStreamAttr) )
    {
        CL_LOG_DEBUG_ERROR(("Stream Attributes should be specified"));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    if( NULL != pStreamAttr )
    {
        rc = clLogStreamAttrValidate(pStreamAttr);
        if( CL_OK != rc )
        {
            return rc;
        }
    }

    CL_LOG_PARAM_CHK((0 > timeout), CL_LOG_RC(CL_ERR_INVALID_PARAMETER));
    CL_LOG_PARAM_CHK((NULL == phStream), CL_LOG_RC(CL_ERR_NULL_POINTER));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogInitialize(ClLogHandleT           *phLog,
                const ClLogCallbacksT  *pLogCallbacks,
                ClVersionT             *pVersion)
{
    ClRcT       rc          = CL_OK;
    ClBoolT     firstHandle = CL_FALSE;
    ClIocPortT  port        = 0;      

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == phLog), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pVersion), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clVersionVerify(&gLogClntVersionDb, pVersion);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clVersionVerify(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogHandleInitHandleCreate(pLogCallbacks, phLog, &firstHandle);
    if( CL_OK != rc )
    {
        return rc;
    }

    clEoMyEoIocPortGet(&port);
    if( (CL_TRUE == firstHandle) && (port != CL_IOC_LOG_PORT) )
    {
        rc = clLogClntStdStreamOpen(*phLog);
        /*
         * Initialize the sys and app streams assuming clLogLibInitialize wasn't called.
         */
        if(rc == CL_OK)
        {
            CL_LOG_HANDLE_APP = stdStreamList[0].hStream;
            CL_LOG_HANDLE_SYS = stdStreamList[1].hStream;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntMasterCompIdAdd(ClLogClntEoDataT  *pClntEoEntry)
{
    ClRcT           rc           = CL_OK;
    ClIdlHandleT    hIdl         = CL_HANDLE_INVALID_VALUE;
    ClUint32T       clientId     = 0;
    ClIocAddressT   mastAddr     = {{0}};
    ClNameT         compName     = {0};
    static ClBoolT  masterNotify = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_TRUE == masterNotify )
    {
        /* 
         * Client has already sent the compData to the master
         */
        return CL_OK;
    }
    rc = clLogMasterAddressGet(&mastAddr);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCpmComponentNameGet(0, &compName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCpmComponentNameGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogClntIdlHandleInitialize(mastAddr, &hIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    clientId = pClntEoEntry->clientId;
    rc = VDECL_VER(clLogMasterCompIdChkNGetClientSync, 4, 0, 0)(hIdl, &compName, &clientId);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(hIdl), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hIdl), CL_OK);
    pClntEoEntry->clientId = clientId;
    masterNotify = CL_TRUE;

    CL_LOG_DEBUG_VERBOSE(("clientId: %d", clientId));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOpen(ClLogHandleT            hLog,
                /* Suppressing coverity warning for pass by value with below comment */
                // coverity[pass_by_value]
                ClNameT                 streamName,
                ClLogStreamScopeT       streamScope,
                ClLogStreamAttributesT  *pStreamAttr,
                ClLogStreamOpenFlagsT   streamOpenFlags,
                ClTimeT                 timeout,
                ClLogStreamHandleT      *phStream)
{
    ClRcT                rc            = CL_OK;
    ClLogClntEoDataT     *pClntEoEntry = NULL;
    ClNameT              nodeName      = {0};
    ClStringT            shmName       = {0};
    ClIdlAddressT        server        = {0};
    ClIdlHandleObjT      idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClLogStreamAttrIDLT  streamAttrIDL = {{0}};
    ClIocPortT           appPort       = 0;
    ClUint32T            shmSize       = 0;
    ClTimerTimeOutT      delay         = {.tsSec = 2, .tsMilliSec = 0};
    ClInt32T maxTries = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogStreamOpenParamValidate(hLog, &streamName, streamScope,
                                      pStreamAttr, streamOpenFlags,
                                      timeout, phStream);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_LOG_STREAM_LOCAL == streamScope )
    {
        rc = clCpmLocalNodeNameGet(&nodeName);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCpmLocalNodeNameGet(): rc[0x %x]", rc));
            return rc;
        }
    }
    else
    {
        nodeName.length = strlen(gStreamScopeGlobal);
        strncpy(nodeName.value, gStreamScopeGlobal, nodeName.length);
        if(pStreamAttr 
           && 
           (streamOpenFlags & CL_LOG_STREAM_CREATE)
           &&
           pStreamAttr->fileLocation[0] != '*'
           &&
           pStreamAttr->fileLocation[0] != '.')
        {
            /*
             * Safe copy instead of a sscanf regex
             */
            ClCharT *pLoc = strchr(pStreamAttr->fileLocation, ':');
            if(pLoc)
            {
                strncpy(nodeName.value, 
                        pStreamAttr->fileLocation,
                        (nodeName.length = CL_MIN(sizeof(nodeName.value)-1, pLoc - pStreamAttr->fileLocation))
                        );
                nodeName.value[nodeName.length] = 0;
            }
        }
    }
    CL_LOG_DEBUG_VERBOSE(("nodeName: %.*s", nodeName.length, nodeName.value));

    rc = clEoMyEoIocPortGet(&appPort);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoIocPortGet(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("Application Port: %u", appPort));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( 0 != timeout )
    {
        server.addressType      = CL_IDL_ADDRESSTYPE_IOC;
        server.address.iocAddress.iocPhyAddress.nodeAddress
            = clIocLocalAddressGet();
        server.address.iocAddress.iocPhyAddress.portId = CL_IOC_LOG_PORT;
        idlObj.address          = server;
        idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
        idlObj.options.timeout  = timeout;
        idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
        idlObj.options.retries  = CL_LOG_CLIENT_DEFAULT_RETRIES; /* 0 */

        rc = clIdlHandleUpdate(pClntEoEntry->hClntIdl, &idlObj);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
            return rc;
        }
    }

    if( NULL != pStreamAttr )
    {
        rc = clLogStreamAttributesCopy(pStreamAttr, &streamAttrIDL);
        if( CL_OK != rc )
        {
            return rc;
        }
    }

    /* Make the sync call to the Server*/
    clLogDebug("LOG", "OPE", "Sending stream [%.*s] open call to server", streamName.length, streamName.value);
    do
    {
        rc = VDECL_VER(clLogSvrStreamOpenClientSync, 4, 0, 0)(pClntEoEntry->hClntIdl, &streamName, streamScope,
                                                              &nodeName, &streamAttrIDL, streamOpenFlags,
                                                              pClntEoEntry->compId, appPort, &shmName, &shmSize);
    }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN 
           && 
           ++maxTries < 5 
           && 
           clOsalTaskDelay(delay) == CL_OK);

    if( NULL != pStreamAttr )
    {
        clHeapFree(streamAttrIDL.fileName.pValue);
        clHeapFree(streamAttrIDL.fileLocation.pValue);
    }
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrStreamOpen, 4, 0, 0)(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_DEBUG_VERBOSE(("shmName: %s shmSize: %u", shmName.pValue, shmSize));

    rc = clLogClntMasterCompIdAdd(pClntEoEntry);
    if( CL_OK != rc )
    {
        clHeapFree(shmName.pValue);
        return rc;
    }

    rc = clLogClntSSOResponseProcess(hLog, &streamName, &nodeName,
                                     &shmName, shmSize, phStream);

    clHeapFree(shmName.pValue);

    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntSSOResponseProcess(ClHandleT  hLog,
                            ClNameT    *pStreamName,
                            ClNameT    *pNodeName,
                            ClStringT  *pShmName,
                            ClUint32T  shmSize,
                            ClHandleT  *phStream)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntStreamOpen(hLog, pStreamName, pNodeName, pShmName,
                             shmSize, phStream);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogWriteAsync(ClLogStreamHandleT   hStream,
                ClLogSeverityT       logSeverity,
                ClUint16T            serviceId,
                ClUint16T            msgId,
                ...)
{
    ClRcT    rc = CL_OK;
    va_list  args;

    CL_LOG_DEBUG_TRACE(("Enter"));

    va_start(args, msgId);

    rc = clLogVWriteAsync(hStream, logSeverity, serviceId, msgId, args);

    va_end(args);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogWriteWithHeader(ClLogStreamHandleT   hStream,
                     ClLogSeverityT       logSeverity,
                     ClUint16T            serviceId,
                     ClUint16T            msgId,
                     ClCharT              *pMsgHeader,
                     ...)
{
    ClRcT    rc = CL_OK;
    va_list  args;

    CL_LOG_DEBUG_TRACE(("Enter"));

    va_start(args, pMsgHeader);

    rc = clLogVWriteAsyncWithHeader(hStream, logSeverity, serviceId, msgId, pMsgHeader, args);

    va_end(args);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogWriteAsyncWithContextHeader(ClLogStreamHandleT   hStream,
                                 ClLogSeverityT       logSeverity,
                                 const ClCharT        *pArea,
                                 const ClCharT        *pContext,
                                 ClUint16T            serviceId,
                                 ClUint16T            msgId,
                                 ...)
{
    ClRcT    rc = CL_OK;
    ClCharT msgHeader[CL_MAX_NAME_LENGTH];
    va_list  args;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (rc = clLogHeaderGetWithContext(pArea, pContext, msgHeader, (ClUint32T)sizeof(msgHeader)) ) != CL_OK)
    {
        msgHeader[0] = 0;
    }

    va_start(args, msgId);

    rc = clLogVWriteAsyncWithHeader(hStream, logSeverity, serviceId, msgId, msgHeader, args);

    va_end(args);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogWriteAsyncWithHeader(ClLogStreamHandleT   hStream,
                          ClLogSeverityT       logSeverity,
                          ClUint16T            serviceId,
                          ClUint16T            msgId,
                          ...)
{
    ClRcT    rc = CL_OK;
    ClCharT msgHeader[CL_MAX_NAME_LENGTH];
    va_list  args;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (rc = clLogHeaderGetWithContext(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                                        msgHeader, (ClUint32T)sizeof(msgHeader)) ) != CL_OK)
    {
        msgHeader[0] = 0;
    }

    va_start(args, msgId);

    rc = clLogVWriteAsyncWithHeader(hStream, logSeverity, serviceId, msgId, msgHeader, args);

    va_end(args);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogVWriteAsyncWithHeader(ClLogStreamHandleT  hStream,
                           ClLogSeverityT      severity,
                           ClUint16T           serviceId,
                           ClUint16T           msgId,
                           ClCharT             *pMsgHeader,
                           va_list             args)
{
    ClRcT                   rc            = CL_OK;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClLogStreamHandleDataT  *pInfo        = NULL;
    ClBoolT unlock = CL_TRUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pClntEoEntry->hClntHandleDB )
    {
        CL_LOG_DEBUG_ERROR(("Log library has been already finalized"));
        return CL_OK;
    }

    rc = clLogHandleCheck(hStream, CL_LOG_STREAM_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pClntEoEntry->clntStreamTblLock);
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_INUSE)
    {
        /*
         * Same thread trying to hold the lock:
         * a potential log loop. So we avoid taking the lock.
         */
        unlock = CL_FALSE;
        rc = CL_OK;
    }

    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clntStreamTblLock(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        if(unlock)
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                           CL_OK);
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }


    rc = clLogClntStreamWriteWithHeader(pClntEoEntry, severity, serviceId,
                                        msgId, pMsgHeader, args, pInfo->hClntStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                CL_OK);
        if(unlock)
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                           CL_OK);
        return rc;
    }


    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        if(unlock)
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                           CL_OK);
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    if(unlock)
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                       CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogVWriteAsync(ClLogStreamHandleT  hStream,
                 ClLogSeverityT      severity,
                 ClUint16T           serviceId,
                 ClUint16T           msgId,
                 va_list             args)
{
    return clLogVWriteAsyncWithHeader(hStream, severity, serviceId, msgId, NULL, args);
}

ClRcT
clLogFilterSet(ClLogStreamHandleT  hStream,
               ClLogFilterFlagsT   filterFlags,
               ClLogFilterT        filter)
{
    ClRcT                   rc            = CL_OK;
    ClLogStreamScopeT       scope         = 0;
    ClLogStreamHandleDataT  *pInfo        = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClLogStreamKeyT         *pUserKey     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogHandleCheck(hStream, CL_LOG_STREAM_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntNodeUserKeyGet(pClntEoEntry->hClntStreamTable,
                             pInfo->hClntStreamNode,
                             (ClCntKeyHandleT *) &pUserKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserKeyGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                       CL_OK);
        return rc;
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogStreamScopeGet(&pUserKey->streamScopeNode, &scope);
    if( CL_OK != rc )
    {
        clLogError("STREAM", "FILTERSET", "Failed get stream scope. rc [0x%x]", rc);
        return rc;
    }

    rc = clLogStreamFilterSet(&pUserKey->streamName, 
                            scope,
                            &pUserKey->streamScopeNode,
                            filterFlags,
                            filter);
    if (rc != CL_OK)
    {
        clLogError("STREAM", "FILTERSET", "Failed to set filter using stream info. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}


ClRcT clLogStreamFilterSet(ClNameT                    *pStreamName, 
                                ClLogStreamScopeT     streamScope,
                                ClNameT               *pStreamScopeNode,
                                ClLogFilterFlagsT     filterFlags,
                                ClLogFilterT          filter)
{
    ClRcT rc = CL_OK;
    ClIocAddressT           iocAddr       = {{0}};
    ClIdlHandleObjT         idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlHandleT            hSvrIdl       = CL_HANDLE_INVALID_VALUE;
    ClIdlAddressT           address       = {0};

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        rc =  clLogMasterAddressGet(&iocAddr);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCpmMasterAddressGet(): rc[0x %x]", rc));
	    return rc;
        }
    }
    else
    {
        rc = clCpmIocAddressForNodeGet(*pStreamScopeNode, &iocAddr);
        if (rc != CL_OK)
        {
            clLogError("STREAM", "FILTERSET", "Failed to get node address from node name. rc [0x%x]", rc);
            return rc;
        }
    }

    CL_LOG_DEBUG_VERBOSE(("NodeAddr: %u", iocAddr.iocPhyAddress.nodeAddress));

    address.addressType     = CL_IDL_ADDRESSTYPE_IOC;
    address.address.iocAddress.iocPhyAddress.nodeAddress
                            = iocAddr.iocPhyAddress.nodeAddress;
    address.address.iocAddress.iocPhyAddress.portId
                            = CL_IOC_LOG_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = CL_LOG_CLIENT_DEFULT_TIMEOUT;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = CL_LOG_CLIENT_DEFAULT_RETRIES;

    rc = clIdlHandleInitialize(&idlObj, &hSvrIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_VERBOSE(("streamName: %.*s", pStreamName->length,
                         pStreamName->value));
    CL_LOG_DEBUG_VERBOSE(("scopeNode: %.*s", pStreamScopeNode->length,
                          pStreamScopeNode->value));

    rc = VDECL_VER(clLogStreamOwnerFilterSetClientAsync, 4, 0, 0)(hSvrIdl, pStreamName,
             streamScope, pStreamScopeNode, filterFlags, &filter, NULL, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogSvrFilterSetClientAsync, 4, 0, 0)(): rc[0x%x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hSvrIdl), CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clIdlHandleFinalize(hSvrIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamFilterGet(ClNameT                  *pStreamName,
                        ClLogStreamScopeT     streamScope,
                        ClNameT               *pStreamScopeNode,
                        ClLogFilterT          *pFilter)
{
    ClRcT rc = CL_OK;
    ClIocAddressT           iocAddr       = {{0}};
    ClIdlHandleObjT         idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlHandleT            hSvrIdl       = CL_HANDLE_INVALID_VALUE;
    ClIdlAddressT           address       = {0};

    if (!pFilter)
    {
        clLogError("STREAM", "FILTERGET", "NULL parameter passed: pFilter");
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }

    if( CL_LOG_STREAM_GLOBAL == streamScope )
    {
        rc =  clLogMasterAddressGet(&iocAddr);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCpmMasterAddressGet(): rc[0x %x]", rc));
	    return rc;
        }
    }
    else
    {
        rc = clCpmIocAddressForNodeGet(*pStreamScopeNode, &iocAddr);
        if (rc != CL_OK)
        {
            clLogError("STREAM", "FILTERGET", "Failed to get node address from node name. rc [0x%x]", rc);
            return rc;
        }
    }

    CL_LOG_DEBUG_VERBOSE(("NodeAddr: %u", iocAddr.iocPhyAddress.nodeAddress));

    address.addressType     = CL_IDL_ADDRESSTYPE_IOC;
    address.address.iocAddress.iocPhyAddress.nodeAddress
                            = iocAddr.iocPhyAddress.nodeAddress;
    address.address.iocAddress.iocPhyAddress.portId
                            = CL_IOC_LOG_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = CL_LOG_CLIENT_DEFULT_TIMEOUT;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = CL_LOG_CLIENT_DEFAULT_RETRIES;

    rc = clIdlHandleInitialize(&idlObj, &hSvrIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_VERBOSE(("streamName: %.*s", pStreamName->length,
                         pStreamName->value));
    CL_LOG_DEBUG_VERBOSE(("scopeNode: %.*s", pStreamScopeNode->length,
                          pStreamScopeNode->value));

    rc = VDECL_VER(clLogStreamOwnerFilterGetClientSync, 4, 0, 0)(hSvrIdl, pStreamName,
            streamScope, pStreamScopeNode, pFilter);
    if (CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogStreamOwnerFilterGetClientSync, 4, 0, 0)(): rc[0x%x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hSvrIdl), CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clIdlHandleFinalize(hSvrIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamClose(ClLogStreamHandleT hStream)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogHandleCheck(hStream, CL_LOG_STREAM_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogHandleStreamHandleClose(hStream, CL_TRUE);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFinalize(ClLogHandleT  hLog)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter: %#llX",  hLog));

    rc = clLogHandleCheck(hLog, CL_LOG_INIT_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogHandleInitHandleDestroy(hLog);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandlerRegister(ClLogHandleT              hLog,
                     /* Suppressing coverity warning for pass by value with below comment */
                     // coverity[pass_by_value]
                     ClNameT                   streamName,
                     ClLogStreamScopeT         streamScope,
                     /* Suppressing coverity warning for pass by value with below comment */
                     // coverity[pass_by_value]
                     ClNameT                   nodeName,
                     ClLogStreamHandlerFlagsT  handlerFlags,
                     ClLogHandleT              *phStream)
{
    ClRcT                   rc            = CL_OK;
    ClIocMulticastAddressT  mcastAddr     = {0};
    ClIdlHandleT            hClntIdl      = {0};
    ClIdlHandleObjT         idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClIocAddressT           destAddr      = {{0}};
    ClUint16T               retryCnt      = 0;
    ClIocLogicalAddressT    localAddr     = 0;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClCpmSlotInfoT          slotInfo      = {0};

    CL_LOG_DEBUG_TRACE(("Enter: %#llX",  hLog));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogClntHandlerRegisterParamValidate(hLog, &streamName, streamScope,
                                               &nodeName, handlerFlags,
                                               phStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntHandlerRegisterParamValidate(): rc[0x %x]", rc));
        return rc;
    }

    localAddr = clIocLocalAddressGet();
    if( CL_LOG_STREAM_LOCAL == streamScope )
    {
        slotInfo.nodeName.length = nodeName.length;
        memcpy(slotInfo.nodeName.value, nodeName.value, nodeName.length);

        rc = clCpmSlotGet(CL_CPM_NODENAME, &slotInfo);
        if( CL_OK != rc )
        {
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }
        destAddr.iocPhyAddress.nodeAddress = slotInfo.nodeIocAddress;
        destAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;
    }
    else
    {
       nodeName.length = strlen(gStreamScopeGlobal);
       memcpy(nodeName.value, gStreamScopeGlobal, nodeName.length);
       rc = clLogMasterAddressGet(&destAddr);
       if( CL_OK != rc )
       {
           return rc;
       }
    }

    idlObj.address.addressType        = CL_IDL_ADDRESSTYPE_IOC;
    idlObj.address.address.iocAddress = destAddr;
    idlObj.flags                      = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout            = 7000; 
    idlObj.options.priority           = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries            = 0; 

    rc = clIdlHandleInitialize(&idlObj, &hClntIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    do
    {
        rc = VDECL_VER(clLogStreamOwnerStreamMcastGetClientSync, 4, 0, 0)(hClntIdl, &streamName, streamScope,
                                            &nodeName, &mcastAddr);
        /*FIXME - returning 14 sometimes, eventhough its registering in server */
        if( CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc))
        {
            rc = CL_OK;
            usleep(100000);
        }
        else 
        {
            break;
        }
    }while((retryCnt++ < 3));
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);
        return rc;
    }
    
    rc = clLogClntHandlerRegister(hLog, &streamName, streamScope,
                                  &nodeName, handlerFlags, mcastAddr,
                                  phStream);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);
        return rc;
    }
    rc = VDECL_VER(clLogStreamOwnerHandlerRegisterClientAsync, 4, 0, 0)(hClntIdl, &streamName,
                                                    streamScope, &nodeName,
                                                    handlerFlags, localAddr, 
                                                    pClntEoEntry->compId,
                                                    NULL, 0);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogClntHandlerDeregister(*phStream, &hLog), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);
        return rc;
    }

    rc = clLogHandleInitHandleStreamAdd(hLog, *phStream);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(VDECL_VER(clLogStreamOwnerHandlerDeregisterClientAsync, 4, 0, 0)(hClntIdl,
                       &streamName, streamScope, &nodeName, handlerFlags, 
                       localAddr, pClntEoEntry->compId, NULL, 0), CL_OK);
        CL_LOG_CLEANUP(clLogClntHandlerDeregister(*phStream, &hLog), CL_OK);
        CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit: %#llX",  *phStream));
    return rc;
}

/* - clLogClntHandlerRegisterParamValidate
 * - Validates the parameters passed to the handler register function
 */
ClRcT
clLogClntHandlerRegisterParamValidate(ClLogHandleT              hLog,
                                      ClNameT                   *pStreamName,
                                      ClLogStreamScopeT         streamScope,
                                      ClNameT                   *pNodeName,
                                      ClLogStreamHandlerFlagsT  handlerFlags,
                                      ClLogStreamHandleT        *phStream)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogInitHandleCheckNCbValidate(hLog);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamNameValidate(pStreamName);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_LOG_STREAM_LOCAL == streamScope )
    {
        rc = clLogStreamNameValidate(pNodeName);
        if( CL_OK != rc )
        {
            return rc;
        }
    }

    if( (streamScope != CL_LOG_STREAM_LOCAL) &&
        (streamScope != CL_LOG_STREAM_GLOBAL) )
    {
        CL_LOG_DEBUG_ERROR(("Invalid scope"));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    if( NULL == phStream )
    {
        CL_LOG_DEBUG_ERROR(("NULL pointer passed"));
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }
    
    if( (0 != handlerFlags) && (CL_LOG_HANDLER_WILL_ACK != handlerFlags) )
    {
        CL_LOG_DEBUG_ERROR(("bad flag passed"));
        return CL_LOG_RC(CL_ERR_BAD_FLAG);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/* - clLogHandlerRecordAck
 * - Acknowledges the receipt of Log Records to the sender.
 */
ClRcT
clLogHandlerRecordAck(ClLogStreamHandleT  hStream,
                      ClUint64T           seqNum,
                      ClUint32T           numRecords)
{
    ClRcT                          rc                  = CL_OK;
    ClIdlHandleT                   hClntIdl            = CL_HANDLE_INVALID_VALUE;
    ClIocAddressT                  destAddr            = {{0}};
    ClLogClntEoDataT               *pClntEoEntry       = NULL;
    ClLogStreamHandlerHandleDataT  *pData              = NULL;
    ClLogClntHandlerNodeT          *pStreamHandlerData = NULL;
    ClLogClntFlushKeyT             *pKey               = (ClLogClntFlushKeyT *) (ClWordT) seqNum; 
    ClNameT                        nodeName            = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( NULL == pKey ) 
    {
        CL_LOG_DEBUG_WARN(("This handle has not been registered with ACK Flag"));
        return CL_OK;
    }
    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
	clHeapFree(pKey);
        return rc;
    }

    rc = clOsalMutexLock_L(&pClntEoEntry->streamHandlerTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
	clHeapFree(pKey);
        return rc;            
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream, (void *) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
	clHeapFree(pKey);
        return rc;
    }

    rc = clCntDataForKeyGet(pClntEoEntry->hStreamHandlerTable,
                            (ClCntKeyHandleT) &pData->streamMcastAddr, 
                            (ClCntDataHandleT *) &pStreamHandlerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout: rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream), 
                       CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
	clHeapFree(pKey);
        return rc;
    }

    destAddr.iocPhyAddress.nodeAddress = pKey->srcAddr; 
    destAddr.iocPhyAddress.portId      = CL_IOC_LOG_PORT;

    rc = clLogClntIdlHandleInitialize(destAddr, &hClntIdl);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                       CL_OK);
	clHeapFree(pKey);
        return rc;
    }

    if( CL_LOG_STREAM_LOCAL == pStreamHandlerData->streamScope )
    {
        nodeName = pStreamHandlerData->nodeName;
    }
    else
    {
        nodeName.length = strlen(gStreamScopeGlobal);
        strncpy(nodeName.value, gStreamScopeGlobal, nodeName.length);
    }

    rc = VDECL_VER(clLogHandlerSvrAckSendClientAsync, 4, 0, 0)(hClntIdl,
                                           &pStreamHandlerData->streamName,
                                           &nodeName, 
                                           pStreamHandlerData->streamScope,
                                           pKey->seqNum, numRecords,
                                           pKey->hFlushCookie, NULL, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogHandlerAckSendClientSync(): rc[0x %x]",
                            rc));
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hClntIdl), CL_OK);

    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream), CL_OK);

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->streamHandlerTblLock),
                    CL_OK);
	clHeapFree(pKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/* - clLogHandlerDeregister
 * - Deregisters the calling process as handler for the specified stream. */
ClRcT
clLogHandlerDeregister(ClLogStreamHandleT  hStream)
{
    ClRcT         rc   = CL_OK;
    ClLogHandleT  hLog = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogHandleCheck(hStream, CL_LOG_HANDLER_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }
    CL_LOG_CLEANUP(clLogClntHandlerDeregister(hStream, &hLog), CL_OK);
    CL_LOG_CLEANUP(clLogHandleInitHandleHandlerRemove(hLog, hStream), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT clLogClntFilterSetCb(ClBitmapHandleT  hBitmap,
                           ClUint32T        bitNum,
                           void             *pCookie)
{
    ClRcT                   rc            = CL_OK;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClLogStreamHandleDataT  *pStream      = NULL;
    ClLogInitHandleDataT    *pInit        = NULL;
    ClLogFilterT            *pFilter      = (ClLogFilterT *) pCookie;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, bitNum,
                          (void **) (&pStream));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]\n", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, pStream->hLog,
                          (void **) (&pInit));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]\n", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                       CL_OK);
        return rc;
    }

    if( (NULL != pInit->pCallbacks) &&
        (NULL != pInit->pCallbacks->clLogFilterSetCb) )
    {
        pInit->pCallbacks->clLogFilterSetCb(pStream->hStream, *pFilter);
    }

    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, pStream->hLog),
                   CL_OK);
    CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, bitNum),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogClientFilterSetNotify, 4, 0, 0)(
                           /* Suppressing coverity warning for pass by value with below comment */
                           // coverity[pass_by_value]
                           ClNameT            streamName,
                           ClLogStreamScopeT  streamScope,
                           ClNameT            nodeName,
                           ClLogFilterT       filter)
{
    ClRcT                 rc            = CL_OK;
    ClLogClntEoDataT      *pClntEoEntry = NULL;
    ClLogStreamKeyT       *pStreamKey   = NULL;
    ClLogClntStreamDataT  *pStreamData  = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamKeyCreate(&streamName, &nodeName,
                              pClntEoEntry->maxStreams,
                              (ClLogStreamKeyT **) &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntDataForKeyGet(pClntEoEntry->hClntStreamTable,
                            (ClCntKeyHandleT) pStreamKey,
                            (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    clLogStreamKeyDestroy(pStreamKey);

    rc = clBitmapWalk(pStreamData->hStreamBitmap, clLogClntFilterSetCb,
                      (void *) &filter);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStdStreamOpen(ClLogHandleT hLog)
{
    ClRcT                   rc            = CL_OK;
    ClUint32T               i             = 0;
    ClStringT               shmName       = {0, 0};
    ClInt32T                shmFd         = -1;
    ClLogStreamHeaderT      *pHdr         = NULL;
    ClIocMulticastAddressT  mcastAddr     = CL_LOG_DEFAULT_MCASTADDR;
    ClUint32T               shmSize       = 0; 
    ClLogStreamAttrIDLT     streamAttr[2] = {{{0}}};
    ClInt32T                pageSize      = 0;
    ClUint32T               shmPages      = 0;
    ClUint32T               maxComp       = 0;
    ClUint32T               maxStreams    = 0;
    ClUint32T               maxLimit      = 0;
    ClUint32T               numRecords    = 0;

    CL_LOG_DEBUG_TRACE(("Enter: %#llX", hLog));

    rc = clLogPerennialStreamsDataGet(streamAttr, (ClUint32T)
            (sizeof(streamAttr)/sizeof(streamAttr[0])));
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clOsalPageSizeGet_L(&pageSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalPageSizeGet_L(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogConfigDataGet(&maxStreams, &maxComp, &shmPages, &maxLimit);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    shmSize = shmPages * pageSize;

    for( i = 0; i < nStdStream; ++i )
    {
        numRecords = shmSize / streamAttr[i].recordSize;
        if( numRecords < streamAttr[i].flushFreq )
        {
            clLogMultiline(CL_LOG_SEV_WARNING, CL_LOG_AREA_UNSPECIFIED,
                    CL_LOG_CONTEXT_UNSPECIFIED, 
                    "Wrong configuration. Shared memory can hold [%d] records\n"
                    "flush frequency [%d] exceeds this value\n"
                    "Log Service may overwrite [%d] records", 
                    numRecords, streamAttr[i].flushFreq, 
                    streamAttr[i].flushFreq - numRecords);
        }
    }
    
    for( i = 0; i < nStdStream; ++i )
    {
        pHdr = NULL;
        rc = clLogShmNameCreate(&(stdStreamList[i].streamName),
                                &(stdStreamList[i].streamScopeNode),
                                &shmName);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clLogClntStdStreamClose(i), CL_OK);
            return rc;
        }

        rc = clLogShmGet(shmName.pValue, &shmFd);
        if( (CL_OK != rc) && (CL_ERR_ALREADY_EXIST != CL_GET_ERROR_CODE(rc)) )
        {
            CL_LOG_CLEANUP(clLogShmNameDestroy(&shmName), CL_OK);
            CL_LOG_CLEANUP(clLogClntStdStreamClose(i), CL_OK);
            return rc;
        }

        if( CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST )
        {
            rc = clLogStreamShmSegInit(shmName.pValue,
                     shmFd,
                     shmSize,
                     i + 1, 
                     &mcastAddr,
                     streamAttr[i].recordSize,
                     streamAttr[i].flushFreq,
                     streamAttr[i].flushInterval,
                     CL_LOG_MAX_MSGS,
                     maxComp,
                     &pHdr);
            if( CL_OK != rc )
            {
                CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);
                CL_LOG_CLEANUP(clLogShmNameDestroy(&shmName), CL_OK);
                CL_LOG_CLEANUP(clLogClntStdStreamClose(i), CL_OK);
                return rc;
            }
     	}
        CL_LOG_CLEANUP(clOsalShmClose_L(shmFd), CL_OK);

        if( NULL != pHdr )
        {
            CL_LOG_CLEANUP(clOsalMunmap_L(pHdr, pHdr->shmSize), CL_OK);
            pHdr = NULL;
        }

        rc = clLogClntStreamOpen(hLog, &(stdStreamList[i].streamName),
                                 &(stdStreamList[i].streamScopeNode),
                                 &shmName,
                                 shmSize,
                                 &(stdStreamList[i].hStream));
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clLogShmNameDestroy(&shmName), CL_OK);
            CL_LOG_CLEANUP(clLogClntStdStreamClose(i), CL_OK);
            return rc;
        }

        rc = clLogHandleInitHandleStreamRemove(hLog, stdStreamList[i].hStream);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clLogShmNameDestroy(&shmName), CL_OK);
            CL_LOG_CLEANUP(clLogClntStdStreamClose(i), CL_OK);
            return rc;
        }
        CL_LOG_CLEANUP(clLogShmNameDestroy(&shmName), CL_OK);
    }
    for(i = 0; i < nStdStream; i++)
    {
        clHeapFree(streamAttr[i].fileName.pValue);
        clHeapFree(streamAttr[i].fileLocation.pValue);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogClntStdStreamClose(ClUint32T  nStream)
{
    ClRcT               rc      = CL_OK;
    ClUint32T           i       = 0;;

    CL_LOG_DEBUG_TRACE(("Enter: %u", nStream));

    for( i = 0; i < nStream; ++i )
    {
        CL_LOG_CLEANUP(clLogHandleStreamHandleClose(stdStreamList[i].hStream,
                       CL_FALSE), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSeverityFilterGet(ClLogStreamHandleT    hStream,
              ClLogSeverityFilterT  *pSeverity)

{
    ClRcT                   rc            = CL_OK;
    ClLogClntStreamDataT    *pClntData    = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClLogStreamHandleDataT  *pInfo        = NULL;

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pClntEoEntry->hClntHandleDB )
    {
        CL_LOG_DEBUG_ERROR(("Log library has been already finalized"));
        return CL_OK;
    }
    rc = clLogHandleCheck(hStream, CL_LOG_STREAM_HANDLE);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hStream,
                          (void **) (&pInfo));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pClntEoEntry->clntStreamTblLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clntStreamTblLock(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                CL_OK);
        return rc;
    }
    rc = clCntNodeUserDataGet(pClntEoEntry->hClntStreamTable,  pInfo->hClntStreamNode,
                              (ClCntDataHandleT *) &pClntData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                   CL_OK);
        CL_LOG_CLEANUP(clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream),
                CL_OK);
        return rc;
    }
    *pSeverity = pClntData->pStreamHeader->filter.severityFilter;
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pClntEoEntry->clntStreamTblLock),
                   CL_OK);

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }
    return rc;
}
