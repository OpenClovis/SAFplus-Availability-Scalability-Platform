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
#include <stdlib.h>
#include <string.h>

#include <clCpmExtApi.h>
#include <clCksmApi.h>
#include <clParserApi.h>
#include <clIocIpi.h>
#include <clLogApi.h>
#include <clLogCommon.h>
#include <clCpmExtApi.h>
#include <clIocLogicalAddresses.h>
#include <clLogUtilApi.h>

#ifdef POSIX_BUILD
ClOsalSharedMutexFlagsT gClLogMutexMode = CL_OSAL_SHARED_POSIX_SEM;
#else
ClOsalSharedMutexFlagsT gClLogMutexMode = CL_OSAL_SHARED_SYSV_SEM;
#endif

const ClCharT   gShmPrefix[]            = "/CL_";
const ClCharT   gStreamScopeGlobal[]    = "LOG_STREAM_SCOPE_GLOBAL";
ClCharT         gSystemStreamName[]     = "LOG_SYSTEM";
ClCharT         gAppStreamName[]        = "LOG_APPLICATION";
ClCharT         gSystemStreamFile[]     = "clSystem.log";
ClCharT         gAppStreamFile[]        = "clApplication.log";
ClCharT         gSystemStreamFileLoc[]  = "0x1000000000004:/tmp";
ClCharT         gAppStreamFileLoc[]     = "0x1000000000004:/tmp";

ClLogStdStreamDataT stdStreamList[] = {
    {
        { (sizeof(gAppStreamName) - 1), "LOG_APPLICATION" },
        CL_LOG_STREAM_LOCAL,
        { 0, {0}},
        CL_HANDLE_INVALID_VALUE
    },
    {
        { (sizeof(gSystemStreamName) - 1), "LOG_SYSTEM" },
        CL_LOG_STREAM_LOCAL,
        { 0, {0}},
        CL_HANDLE_INVALID_VALUE
    },
};

ClLogASPCompMapT  aspCompMap[] =
{
    {"cpm"         , 1},
    {"logServer"   , 2},
    {"gmsServer"   , 3},
    {"eventServer" , 4},
    {"txnServer"   , 5},
    {"alarmServer" , 6},
    {"nameServer"  , 7},
    {"faultServer" , 8},
    {"corServer"   , 9},
    {"ckptServer"  , 10},
    {"cmServer"    , 11},
    {"snmpServer"  , 12},
};

ClUint32T nLogAspComps  = sizeof(aspCompMap) / sizeof(aspCompMap[0]);

ClUint32T nStdStream    = 0; 

typedef struct
{
  char* key;
  char  type;
  void* storage;
} ClStreamElementDef;

ClRcT
clLogFileLocationFindNGet(ClCharT    *recvFileLoc,
                          ClStringT  *destFileLoc)
{
    ClRcT  rc = CL_OK;
    ClCharT nodeStr[CL_MAX_NAME_LENGTH] = {0};
    ClCharT path[CL_MAX_NAME_LENGTH] = {0};
    SaNameT localName = {0};

    sscanf(recvFileLoc, "%[^:]:%s", nodeStr, path);
    if( ('.' == recvFileLoc[0]) && ('\0' == nodeStr[1]) )
    {
        rc = clCpmLocalNodeNameGet(&localName);
        if( CL_OK != rc )
        {
            clLogError("LOC","GET","Failed to get the local node rc[0x %x]", rc);
        }
        destFileLoc->length = localName.length + strlen(path) + 3;
        destFileLoc->pValue = clHeapCalloc(1, destFileLoc->length);
        if( NULL == destFileLoc->pValue )
        {
            clLogError("LOC","GET","Failed to allocate memory");
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        snprintf( destFileLoc->pValue, destFileLoc->length, "%.*s:%s",
                localName.length, localName.value, path);
    }
    else
    {
        destFileLoc->length = strlen(recvFileLoc) + 1;
        destFileLoc->pValue = clHeapCalloc(1, destFileLoc->length);
        if( NULL == destFileLoc->pValue )
        {
            clLogError("LOC","GET","Failed to allocate memory");
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }
        snprintf(destFileLoc->pValue, destFileLoc->length, "%s", recvFileLoc);
    }
    return CL_OK;
}

ClRcT 
clLogPerennialStreamsDataGet(ClLogStreamAttrIDLT  *pStreamAttr,
                             ClUint32T            nExpectedStream) 
{
    ClRcT	      rc        = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    ClParserPtrT  head      = NULL;
    ClCharT       *aspPath  = NULL;
    ClParserPtrT  fd        = NULL;
    ClParserPtrT  fd1       = NULL;
    ClParserPtrT  temp1     = NULL;
    ClUint32T     count     = 0;
    ClUint32T     i         = 0;

    aspPath = getenv("ASP_CONFIG");
    if( aspPath != NULL ) 
    {
        head = clParserOpenFile(aspPath, CL_LOG_DEFAULT_FILECONFIG);	
        if( head == NULL )
        {
            clLogWarning("SVR", "INI", 
                 "Log config file name has been changed from 'log.xml' to 'clLog.xml',"
                 "Please change the name in your config directory");
            head = clParserOpenFile(aspPath, "log.xml");
            if( head == NULL )
            {
                clLogError("SVR", "INI", "Config file clLog.xml is missing,"
                        "please edit that & proceed");
                return CL_LOG_RC(CL_ERR_NULL_POINTER);
            }
        }
    }
    else
    {
        clLogError("SVR","INI","ASP_CONFIG path is not set in the environment \n");
        return CL_LOG_RC(CL_ERR_DOESNT_EXIST);
    }

    if(NULL == head)
    {  
        CL_LOG_DEBUG_ERROR(("head tag is not proper, file %s/%s", 
                               aspPath, CL_LOG_DEFAULT_FILECONFIG));
        goto ParseError;
    }

    if(NULL == (fd = clParserChild(head, "log")))
    {
        CL_LOG_DEBUG_ERROR(("log tag is not proper, file %s/%s", 
                             aspPath, CL_LOG_DEFAULT_FILECONFIG));
        goto ParseError;
    }

    if(NULL == (fd = clParserChild(fd, "perennialStreamsData")))
    {
        CL_LOG_DEBUG_ERROR(("perennialStreamsData tag is not proper, file %s/%s", 
                            aspPath, CL_LOG_DEFAULT_FILECONFIG));
        goto ParseError;
    }

    if( NULL == (fd1= clParserChild(fd, "stream")))
    {
        CL_LOG_DEBUG_ERROR(("stream tag is not proper, file %s/%s",
                           aspPath, CL_LOG_DEFAULT_FILECONFIG));
        goto ParseError;
    }

    while( (NULL != fd1) && (count < nExpectedStream) )
    {
        if(NULL != (temp1 = clParserChild(fd1, "fileName")))
        {
            pStreamAttr[count].fileName.length = strlen(temp1->txt) + 1;
            pStreamAttr[count].fileName.pValue =
                clHeapCalloc(pStreamAttr[count].fileName.length,
                             sizeof(ClCharT));
            if( NULL == pStreamAttr[count].fileName.pValue )
            {
                CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
                goto ParseError;
            }

            snprintf(pStreamAttr[count].fileName.pValue, pStreamAttr[count].fileName.length, "%s", temp1->txt);
            CL_LOG_DEBUG_TRACE((" fileName: %s ",
                        pStreamAttr[count].fileName.pValue));
        }
        else
        {
            CL_LOG_DEBUG_ERROR(("fileName tag is not proper"));
            goto ParseError;
        }
        if(NULL != (temp1 = clParserChild(fd1, "fileLocation")))
        {
            rc = clLogFileLocationFindNGet(temp1->txt,
                    &pStreamAttr[count].fileLocation);
            if( CL_OK != rc )
            {
                clHeapFree(pStreamAttr[count].fileName.pValue);
                pStreamAttr[count].fileName.pValue = NULL;
                goto ParseError;
            }
            CL_LOG_DEBUG_TRACE((" fileLocation: %s ",
                        pStreamAttr[count].fileLocation.pValue));
        }
        else
        {
            CL_LOG_DEBUG_ERROR(("fileLocation tag is not proper"));
            clHeapFree(pStreamAttr[count].fileName.pValue);
            pStreamAttr[count].fileName.pValue = NULL;
            goto ParseError;
        }

        ClStreamElementDef elemArray[] = 
        {
            {"fileUnitSize",'d', (void*) &pStreamAttr[count].fileUnitSize},  /* I use 'd' for integer since that is like printf */
            {"recordSize",'d',   (void*) &pStreamAttr[count].recordSize},
            {"fileFullAction", 's',  (void*) &pStreamAttr[count].fileFullAction},
            {"maximumFilesRotated",'d',  (void*) &pStreamAttr[count].maxFilesRotated },
            {"flushFreq",'d', (void*) &pStreamAttr[count].flushFreq},
            {"flushInterval",'L', (void*) &pStreamAttr[count].flushInterval},
            {"syslog",'b', (void*) &pStreamAttr[count].syslog },
            {0,0,0}
        };

        /* Loop through all items, reading the value and inserting into the correct memory location */
        for (i=0; elemArray[i].key != 0; i++)
        {
            if(NULL != (temp1 = clParserChild(fd1,elemArray[i].key)))
            {
                if (elemArray[i].type == 'd')
                {
                    *((int*) elemArray[i].storage) = atoi(temp1->txt);
                }
                else if(elemArray[i].type == 'b')
                {
                    *((ClBoolT*)elemArray[i].storage) = (strncasecmp(temp1->txt, "yes", 3) == 0 ? CL_TRUE : CL_FALSE );
                }
                else if (elemArray[i].type == 's')
                  {
                    *((ClLogFileFullActionT *)elemArray[i].storage) = clLogStr2FileActionGet(temp1->txt);
                  }     
                else if (elemArray[i].type == 'L') 
                {
                    *((ClInt64T*)elemArray[i].storage) = (ClInt64T)strtoll(temp1->txt,  NULL, 10);
                 }
                else /* a type was specified that I do not support */
                {
                    CL_ASSERT(0);  /* This is a coding error.  The higher software can do nothing to fix it, so don't return an error, just halt */
                }
            }
            else if(strncmp(elemArray[i].key, "syslog", 6))
            {
                CL_LOG_DEBUG_ERROR(("%s tag is not correct in file %s/%s",elemArray[i].key, aspPath, CL_LOG_DEFAULT_FILECONFIG));
                clHeapFree(pStreamAttr[count].fileLocation.pValue);
                clHeapFree(pStreamAttr[count].fileName.pValue);
                pStreamAttr[count].fileLocation.pValue = NULL;
                pStreamAttr[count].fileName.pValue     = NULL;
                goto ParseError;
            }
        }

        pStreamAttr[count].haProperty         = 
            CL_LOG_DEFAULT_HA_PROPERTY;
        pStreamAttr[count].waterMark.lowLimit = 
            CL_LOG_DEFAULT_LOW_WATER_MARK;
        pStreamAttr[count].waterMark.lowLimit = 
            CL_LOG_DEFAULT_HIGH_WATER_MARK;

        count++;
        fd1 = fd1->next;
    }
    nStdStream = count;
    for( i = 0; i < nStdStream; i++ )
    {
        if( stdStreamList[i].streamScope == CL_LOG_STREAM_LOCAL )
        {
            memset(&stdStreamList[i].streamScopeNode, '\0', sizeof(SaNameT));
            rc = clCpmLocalNodeNameGet(&stdStreamList[i].streamScopeNode);
            if( CL_OK != rc )
            {
                clLogError("SEV","INI","Failed to get the local node rc[0x %x]", rc);
            }
        }
    }

    clParserFree(head);
    return CL_OK;
ParseError:
    for (i=0; i < count; i++) 
    {
        clHeapFree(pStreamAttr[i].fileLocation.pValue);
        clHeapFree(pStreamAttr[i].fileName.pValue);
        pStreamAttr[i].fileName.pValue = NULL;
        pStreamAttr[i].fileName.pValue = NULL;
    }
    clParserFree(head);
    return rc;
}

ClRcT
clLogMasterAddressGet(ClIocAddressT  *pIocAddress)
{
    ClRcT                 rc          = CL_OK;
    ClIocLogicalAddressT  logicalAddr = 0;
    ClIocNodeAddressT     nodeAddress = 0;
    ClRcT                 retryCnt    = 5;
    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pIocAddress);

    logicalAddr = CL_IOC_LOG_LOGICAL_ADDRESS;
    /*
     * if this call is made during transcient time of switchover/failover,
     * this may fail, so just retrying for thrice to ensure that master is 
     * not there 
     */
    rc = clIocMasterAddressGetExtended(logicalAddr,CL_IOC_LOG_PORT,&nodeAddress, retryCnt, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIocMasterAddressGet():"
                            "rc [0x %x]", rc));
        return rc;
    }
    pIocAddress->iocPhyAddress.nodeAddress = nodeAddress;
    pIocAddress->iocPhyAddress.portId = CL_IOC_LOG_PORT;
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamScopeGet(SaNameT            *pNodeName,
                    ClLogStreamScopeT  *pScope)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pNodeName);
    CL_ASSERT(NULL != pScope);

    CL_LOG_DEBUG_VERBOSE(("NodeName: %.*s", pNodeName->length,
                          pNodeName->value));
    if( !strncmp((const ClCharT *)pNodeName->value, gStreamScopeGlobal, pNodeName->length) )
    {
        *pScope = CL_LOG_STREAM_GLOBAL;
    }
    else
    {
        *pScope = CL_LOG_STREAM_LOCAL;
    }

    CL_LOG_DEBUG_TRACE(("Exit: scope:%d", *pScope));
    return rc;
}

ClInt32T
clLogStreamKeyCompare(ClCntKeyHandleT  key1,
                      ClCntKeyHandleT  key2)
{
    ClLogStreamKeyT  *pKey1 = (ClLogStreamKeyT *) key1;
    ClLogStreamKeyT  *pKey2 = (ClLogStreamKeyT *) key2;

    CL_LOG_DEBUG_TRACE(("Enter:"));

    CL_ASSERT(NULL != pKey1);
    CL_ASSERT(NULL != pKey2);

    CL_LOG_DEBUG_VERBOSE(("pKey1: %hu %.*s %hu %.*s", pKey1->streamName.length,
                          pKey1->streamName.length, pKey1->streamName.value,
                          pKey1->streamScopeNode.length, pKey1->streamScopeNode.length,
                          pKey1->streamScopeNode.value));
    CL_LOG_DEBUG_VERBOSE(("pKey1: %hu %.*s %hu %.*s", pKey2->streamName.length,
                          pKey2->streamName.length, pKey2->streamName.value,
                          pKey2->streamScopeNode.length, pKey2->streamScopeNode.length,
                          pKey2->streamScopeNode.value));

    if( pKey1->streamName.length != pKey2->streamName.length )
    {
        CL_LOG_DEBUG_TRACE(("Mismatched name length"));
        return -1;
    }

    if( strncmp((const ClCharT *)pKey1->streamName.value, (const ClCharT *)pKey2->streamName.value,
                pKey1->streamName.length)  )
    {
        CL_LOG_DEBUG_TRACE(("Mismatched stream name"));
        return -1;
    }

    if( pKey1->streamScopeNode.length != pKey2->streamScopeNode.length )
    {
        CL_LOG_DEBUG_TRACE(("Mismatched scopenode name length"));
        return -1;
    }

    if( strncmp((const ClCharT *)pKey1->streamScopeNode.value, (const ClCharT *)pKey2->streamScopeNode.value,
                pKey1->streamScopeNode.length)  )
    {
        CL_LOG_DEBUG_TRACE(("Mismatched scopenode name"));
        return -1;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return 0;
}

ClUint32T
clLogStreamHashFn(ClCntKeyHandleT key)
{
    return ((ClLogStreamKeyT *) key)->hash;
}


ClRcT
clLogStreamKeyCreate(SaNameT          *pStreamName,
                     SaNameT          *pNodeName,
                     ClUint32T        maxStreams,
                     ClLogStreamKeyT  **ppStreamKey)
{
    ClRcT      rc                              = CL_OK;
    ClCharT    pStream[2 * CL_MAX_NAME_LENGTH] = {0};
    ClUint32T  cksum                           = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pStreamName);
    CL_ASSERT(NULL != pNodeName);
    CL_ASSERT(NULL != ppStreamKey);
    CL_ASSERT(0 != maxStreams);

    CL_LOG_DEBUG_VERBOSE(("Stream: %hu %.*s %hu %.*s", pStreamName->length,
                          pStreamName->length, pStreamName->value, pNodeName->length,
                          pNodeName->length, pNodeName->value));

    memcpy(pStream, pStreamName->value, pStreamName->length);
    memcpy(pStream + pStreamName->length, pNodeName->value, pNodeName->length);

    rc = clCksm32bitCompute((ClUint8T *) pStream,
                            pStreamName->length + pNodeName->length, &cksum);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCksm32bitCompute(): rc[0x %x]", rc));
        return rc;
    }

    *ppStreamKey = clHeapCalloc(1, sizeof(ClLogStreamKeyT));
    if( *ppStreamKey == NULL )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    (*ppStreamKey)->streamName      = *pStreamName;
    (*ppStreamKey)->streamScopeNode = *pNodeName;
    (*ppStreamKey)->hash            = cksum % maxStreams;

    CL_LOG_DEBUG_VERBOSE(("Created Key: %hu %.*s %hu %.*s",
                          (*ppStreamKey)->streamName.length, (*ppStreamKey)->streamName.length,
                          (*ppStreamKey)->streamName.value,
                          (*ppStreamKey)->streamScopeNode.length,
                          (*ppStreamKey)->streamScopeNode.length,
                          (*ppStreamKey)->streamScopeNode.value));
    CL_LOG_DEBUG_TRACE(("Exit"));

    return rc;
}

void
clLogStreamKeyDestroy(ClLogStreamKeyT  *pStreamKey)
{
    CL_LOG_DEBUG_TRACE(("Enter"));

    clHeapFree(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return;
}

ClRcT
clLogShmGet(ClCharT   *shmName,
            ClInt32T  *pShmFd)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter: %s", shmName));

    CL_ASSERT(NULL != shmName);
    CL_ASSERT(NULL != pShmFd);

    CL_LOG_DEBUG_VERBOSE(("Share Memory Name: %s", shmName));

    rc = clOsalShmOpen_L(shmName, CL_LOG_SHM_EXCL_CREATE_FLAGS,
                         CL_LOG_SHM_MODE, pShmFd);
    if( CL_ERR_ALREADY_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        CL_LOG_DEBUG_TRACE(("Share Memory Segment Exists"));
        rc = clOsalShmOpen_L(shmName, CL_LOG_SHM_OPEN_FLAGS,
                             CL_LOG_SHM_MODE, pShmFd);
        if( CL_OK == rc )
        {
            clLogInfo("LOG", CL_LOG_CONTEXT_UNSPECIFIED, 
                "Shared memory [%s] has been opened", shmName); 
            return CL_LOG_RC(CL_ERR_ALREADY_EXIST); /* So no lock/cond is recreated */
        }
        return rc;
    }
    clLogInfo("LOG", CL_LOG_CONTEXT_UNSPECIFIED,  
             "Shared memory [%s] has been created", shmName); 

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return CL_OK;
}

ClRcT
clLogStreamShmSegInit(SaNameT                 *pStreamName,
                      ClCharT                 *pShmName,
                      ClInt32T                shmFd,
                      ClUint32T               shmSize,
                      ClUint16T               streamId,
                      ClIocMulticastAddressT  *pStreamMcastAddr,
                      ClUint32T               recordSize,
                      ClUint32T               flushFreq,
                      ClTimeT                 flushInterval,
                      ClUint16T               maxMsgs,
                      ClUint16T               maxComps,
                      ClLogStreamHeaderT      **ppSegHeader)
{
    ClRcT      rc      = CL_OK;
    ClUint32T  hdrSize = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(0 < shmSize);
    CL_ASSERT(NULL != pStreamMcastAddr);
    CL_ASSERT(NULL != ppSegHeader);

    CL_LOG_DEBUG_TRACE(("ShmSize: %u StreamId: %hu McastAddr: %lld "
                          "RecSize: %u FlushFreq: %u FlushInt: %lld",
                          shmSize, streamId, *pStreamMcastAddr, recordSize,
                          flushFreq, flushInterval));

    rc = clOsalFtruncate_L(shmFd, shmSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalFtruncate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMmap_L(0, shmSize, CL_LOG_MMAP_PROT_FLAGS, CL_LOG_MMAP_FLAGS,
                      shmFd, 0, (void **) ppSegHeader);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogPSharedMutexCreate(pShmName, *ppSegHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppSegHeader, shmSize), CL_OK);
        return rc;
    }

#ifndef POSIX_BUILD
    rc = clOsalMutexLock_L(&((*ppSegHeader)->shmLock));
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppSegHeader)->shmLock), CL_OK);
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppSegHeader, shmSize), CL_OK);
        return rc;
    }
#endif
    rc = clLogPSharedCondCreate(*ppSegHeader);
    if( CL_OK != rc )
    {
#ifndef POSIX_BUILD
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&((*ppSegHeader)->shmLock)), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&((*ppSegHeader)->shmLock)),
                       CL_OK);
#endif
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppSegHeader, shmSize), CL_OK);
        return rc;
    }

    rc = clOsalMutexInit_L(&((*ppSegHeader)->lock_for_join));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreateEx_L(): rc[0x %x]", rc));
#ifndef POSIX_BUILD
        CL_LOG_CLEANUP(clOsalCondDestroy_L(&((*ppSegHeader)->flushCond)), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&((*ppSegHeader)->shmLock)), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&((*ppSegHeader)->shmLock)),
                       CL_OK);
#endif
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppSegHeader, shmSize), CL_OK);
        return rc;
    }

    rc = clOsalCondInit(&((*ppSegHeader)->cond_for_join));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalCondInit_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&((*ppSegHeader)->lock_for_join)), CL_OK);
#ifndef POSIX_BUILD
        CL_LOG_CLEANUP(clOsalCondDestroy_L(&((*ppSegHeader)->flushCond)), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&((*ppSegHeader)->shmLock)), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&((*ppSegHeader)->shmLock)),
                       CL_OK);
#endif
        CL_LOG_CLEANUP(clOsalMunmap_L(*ppSegHeader, shmSize), CL_OK);
        return rc;
    }

    hdrSize = CL_LOG_HEADER_SIZE_GET(maxMsgs, maxComps);
    (*ppSegHeader)->streamId                            = streamId;
    (*ppSegHeader)->recordSize                          = recordSize;
    (*ppSegHeader)->recordIdx                           = 0;
    (*ppSegHeader)->startAck                            = 0;
    (*ppSegHeader)->flushCnt                            = 0;
    (*ppSegHeader)->numOverwrite                        = 0;
    (*ppSegHeader)->flushFreq                           = flushFreq;
    (*ppSegHeader)->flushInterval                       = flushInterval;
    (*ppSegHeader)->streamMcastAddr.iocMulticastAddress = *pStreamMcastAddr;
    (*ppSegHeader)->streamStatus                        = CL_LOG_STREAM_ACTIVE;
    (*ppSegHeader)->filter.severityFilter  = clLogDefaultStreamSeverityGet(pStreamName);
    (*ppSegHeader)->filter.msgIdSetLength  = 0;
    (*ppSegHeader)->filter.compIdSetLength = 0;
    (*ppSegHeader)->maxMsgs                = maxMsgs;
    (*ppSegHeader)->maxComps               = maxComps;
    (*ppSegHeader)->maxRecordCount         = (shmSize - hdrSize) / recordSize;
    (*ppSegHeader)->shmSize                = shmSize;
    (*ppSegHeader)->sequenceNum            = 1;
#ifndef POSIX_BUILD
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&((*ppSegHeader)->shmLock)), CL_OK);
#endif
    CL_LOG_DEBUG_TRACE(("Exit: %u", (*ppSegHeader)->maxRecordCount));
    return rc;
}

ClRcT
clLogFilterModify(ClLogStreamHeaderT  *pHeader,
                  ClLogFilterT        *pStreamFilter,
                  ClLogFilterFlagsT   flags)
{
    ClRcT      rc      = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pHeader);
    CL_ASSERT(NULL != pStreamFilter);

    CL_LOG_DEBUG_TRACE(("Severity: 0x%hx MsgSetLen: %hu CompSetLen: %hu",
                        pStreamFilter->severityFilter,
                        pStreamFilter->msgIdSetLength,
                        pStreamFilter->compIdSetLength));

    switch(flags)
    {
    case CL_LOG_FILTER_ASSIGN:
        {
            ClUint16T lastSetLength = pHeader->filter.msgIdSetLength;
            ClUint16T newSetLength = CL_MIN(pHeader->maxMsgs >> 3, pStreamFilter->msgIdSetLength);

            if(newSetLength  < lastSetLength)
                memset( pHeader + 1, 0, lastSetLength);

            pHeader->filter.severityFilter = pStreamFilter->severityFilter;
            pHeader->filter.msgIdSetLength = newSetLength;
            if(pStreamFilter->pMsgIdSet)
                memcpy(pHeader+1, pStreamFilter->pMsgIdSet, newSetLength);

            lastSetLength = pHeader->filter.compIdSetLength;
            newSetLength = CL_MIN(pHeader->maxComps >> 3, pStreamFilter->compIdSetLength);

            if(newSetLength < lastSetLength)
                memset( (ClUint8T*)(pHeader+1) + (pHeader->maxMsgs/CL_BITS_PER_BYTE+1),
                        0, lastSetLength);
            pHeader->filter.compIdSetLength = newSetLength;

            CL_LOG_DEBUG_VERBOSE(("Max MsgSetLen: %hu CompSetLen: %hu",
                                  pHeader->maxMsgs, pHeader->maxComps));
            CL_LOG_DEBUG_VERBOSE(("Assigned MsgSetLen: %hu CompSetLen: %hu",
                                  pHeader->filter.msgIdSetLength,
                                  pHeader->filter.compIdSetLength));

            pHeader->filter.pMsgIdSet  = NULL;
            pHeader->filter.pCompIdSet = NULL;

            if(pStreamFilter->pCompIdSet)
                memcpy( (ClUint8T *)(pHeader + 1) + (pHeader->maxMsgs/CL_BITS_PER_BYTE + 1),
                        pStreamFilter->pCompIdSet, pHeader->filter.compIdSetLength);
        }
        break;

    case CL_LOG_FILTER_MERGE_ADD:
        {
            ClUint32T i;
            ClUint8T *map = NULL;
            ClUint16T setLength = pStreamFilter->msgIdSetLength;

            pHeader->filter.severityFilter |= pStreamFilter->severityFilter;
            if(pStreamFilter->pMsgIdSet)
            {
                setLength = CL_MIN(pHeader->maxMsgs >> 3, setLength);
                pHeader->filter.msgIdSetLength = CL_MAX(pHeader->filter.msgIdSetLength, setLength);
                map = (ClUint8T*) (pHeader+1);
                for(i = 0; i < (setLength << 3); ++i)
                {
                    ClUint8T *newMap = pStreamFilter->pMsgIdSet+(i>>3);
                    if( (newMap[0] & (1<<(i&7))) )
                    {
                        map[i>>3] |= (1 << (i&7) );
                    }
                }
            }
            if(pStreamFilter->pCompIdSet)
            {
                setLength = CL_MIN(pHeader->maxComps >> 3, pStreamFilter->compIdSetLength);
                pHeader->filter.compIdSetLength = CL_MAX(pHeader->filter.compIdSetLength, setLength);
                map = (ClUint8T*)(pHeader+1) + (pHeader->maxMsgs/CL_BITS_PER_BYTE+1);
                for(i = 0; i <  (setLength << 3); ++i)
                {
                    ClUint8T *newMap = pStreamFilter->pCompIdSet+(i>>3);
                    if( ( newMap[0] & ( 1 << (i&7) ) ) )
                    {
                        map[i>>3] |= (1 << (i&7) );
                    }
                }
            }
        }
        break;

    case CL_LOG_FILTER_MERGE_DELETE:
        {
            ClUint16T setLength = pStreamFilter->msgIdSetLength;
            ClUint8T *map = NULL;
            ClUint32T i;
            pHeader->filter.severityFilter = 
                (pHeader->filter.severityFilter | pStreamFilter->severityFilter) ^ (pHeader->filter.severityFilter);
            if(pStreamFilter->pMsgIdSet)
            {
                map = (ClUint8T*)(pHeader+1);
                setLength = CL_MIN(pHeader->maxMsgs >> 3, pStreamFilter->msgIdSetLength);
                pHeader->filter.msgIdSetLength = CL_MAX(pHeader->filter.msgIdSetLength, setLength);
                for(i = 0; i < (setLength << 3); ++i)
                {
                    ClUint8T *newMap = pStreamFilter->pMsgIdSet + (i>>3);
                    if( (newMap[0] & ( 1 << (i&7) ) ) )
                    {
                        map[i>>3] ^= (1 << (i&7) );
                    }
                }
            }
            if(pStreamFilter->pCompIdSet)
            {
                map = (ClUint8T*)(pHeader + 1) + (pHeader->maxMsgs/CL_BITS_PER_BYTE + 1);
                setLength = CL_MIN(pHeader->maxComps >> 3, pStreamFilter->compIdSetLength);
                pHeader->filter.compIdSetLength = CL_MAX(pHeader->filter.compIdSetLength, setLength);
                for(i = 0; i < (setLength << 3); ++i)
                {
                    ClUint8T *newMap = pStreamFilter->pCompIdSet + (i>>3);
                    if( (newMap[0] & ( 1 << (i&7) ) ) )
                    {
                        map[i>>3] ^= (1 << (i&7) );
                    }
                }
            }
        }
        break;
        
    default:
        break;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFilterAssign(ClLogStreamHeaderT  *pHeader,
                  ClLogFilterT        *pStreamFilter)
{
    ClRcT      rc      = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pHeader);
    CL_ASSERT(NULL != pStreamFilter);

    CL_LOG_DEBUG_TRACE(("Severity: 0x%hx MsgSetLen: %hu CompSetLen: %hu",
                        pStreamFilter->severityFilter,
                        pStreamFilter->msgIdSetLength,
                        pStreamFilter->compIdSetLength));

    pHeader->filter.severityFilter = pStreamFilter->severityFilter;
    pHeader->filter.msgIdSetLength
        = (pStreamFilter->msgIdSetLength > (pHeader->maxMsgs >> 3))
        ? (pHeader->maxMsgs>>3) : pStreamFilter->msgIdSetLength;

    pHeader->filter.compIdSetLength
        = (pStreamFilter->compIdSetLength > (pHeader->maxComps>>3))
        ? (pHeader->maxComps>>3) : pStreamFilter->compIdSetLength;

    CL_LOG_DEBUG_VERBOSE(("Max MsgSetLen: %hu CompSetLen: %hu",
                          pHeader->maxMsgs, pHeader->maxComps));
    CL_LOG_DEBUG_VERBOSE(("Assigned MsgSetLen: %hu CompSetLen: %hu",
                          pHeader->filter.msgIdSetLength,
                          pHeader->filter.compIdSetLength));

    pHeader->filter.pMsgIdSet  = NULL;
    pHeader->filter.pCompIdSet = NULL;

    if(pStreamFilter->pMsgIdSet)
        memcpy(pHeader + 1, pStreamFilter->pMsgIdSet,
               pHeader->filter.msgIdSetLength);

    if(pStreamFilter->pCompIdSet)
        memcpy( (ClUint8T *)(pHeader + 1) + (pHeader->maxMsgs/CL_BITS_PER_BYTE + 1),
                pStreamFilter->pCompIdSet, pHeader->filter.compIdSetLength);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

#ifndef VXWORKS_BUILD
static ClRcT clLogCheckKey(ClCharT *pShmName, ClCharT *pFlusherName,ClUint32T *pBytes)
{
    ClUint32T key = 0;
    ClUint32T newKey = 0;
    ClInt32T index = 1;
    ClUint32T maxBytes = *pBytes;
    ClRcT rc = clCksm32bitCompute((ClUint8T*)pShmName, (ClUint32T)strlen(pShmName), &key);

    CL_ASSERT(rc == CL_OK && key);

    while (index++ < 100)
    {
        ClUint32T bytes = snprintf(pFlusherName, maxBytes, "%s_%d", (ClCharT*)pShmName, index);
        rc = clCksm32bitCompute((ClUint8T*)pFlusherName, bytes, &newKey);
        if(rc != CL_OK)
        {
            continue;
        }
        if(newKey == key)
            continue;

        *pBytes = bytes;
        goto found;
    }
    clDbgCodeError(CL_ERR_NOT_EXIST,("Newkey not found for flusher\n"));
    CL_ASSERT(0);

    found:
    return CL_OK;
}
#endif

ClRcT
clLogPSharedMutexCreate(ClCharT *pShmName, ClLogStreamHeaderT  *pHeader)
{
    ClRcT rc;
#ifndef POSIX_BUILD
    rc = clOsalProcessSharedMutexInit(&(pHeader->shmLock), CL_OSAL_SHARED_NORMAL, NULL, 0, 0);
#endif
    /* FOR POSIX, the below maps to a "sem_init" and would be done
     * only once during log shared segment create/init. as multiple 
     * sem_init calls are undefined.
     */

#ifndef VXWORKS_BUILD
    rc = clOsalProcessSharedMutexInit(&pHeader->sharedSem, gClLogMutexMode,
            (ClUint8T*)pShmName, strlen(pShmName), 1); /* unlocked*/
#else
    {
        ClOsalSemIdT sharedSemId = 0;
        ClCharT semName[CL_MAX_NAME_LENGTH];
        snprintf(semName, sizeof(semName)-1, "%s%s_sharedSem", pShmName[0] == '/' ? "":"/", pShmName);
        rc = clOsalSemCreate((ClUint8T*)semName, strlen(semName), &sharedSemId); 
    }
#endif
    if (rc == CL_OK)
    {
#ifndef VXWORKS_BUILD        
        ClCharT flushShmName[CL_MAX_NAME_LENGTH];
        ClUint32T bytes = 0;

        /*Make sure that atleast the key doesnt clash with shmName*/
        bytes = (ClUint32T)sizeof(flushShmName);
        clLogCheckKey(pShmName, flushShmName, &bytes);
        rc = clOsalProcessSharedMutexInit(&pHeader->flusherSem, gClLogMutexMode,
                (ClUint8T*)flushShmName, strlen(flushShmName), 0); /*LOCKED*/
#else
        ClOsalSemIdT sharedSemId = 0;
        ClCharT semName[CL_MAX_NAME_LENGTH];
        snprintf(semName, sizeof(semName)-1, "%s%s_flusherSem", pShmName[0] == '/' ? "":"/", pShmName);
        rc = clOsalSemCreate((ClUint8T*)semName, strlen(semName), &sharedSemId);
#endif
    }

    return rc; 
}

ClRcT
clLogPSharedCondCreate(ClLogStreamHeaderT *pHeader)
{
#ifndef POSIX_BUILD
    if( !(gClLogMutexMode & CL_OSAL_SHARED_POSIX_SEM) )
        return clOsalProcessSharedCondInit(&(pHeader->flushCond));
#endif
    return CL_OK;
}

ClRcT
clLogShmNameCreate(SaNameT    *pStreamName,
                   SaNameT    *pStreamScopeNode,
                   ClStringT  *pShmName)
{
    ClRcT       rc    = CL_OK;
    ClUint16T   nChar = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pStreamName);
    CL_ASSERT(NULL != pStreamScopeNode);
    CL_ASSERT(NULL != pShmName);

    CL_LOG_DEBUG_VERBOSE(("StreamName: %.*s",
                          pStreamName->length, pStreamName->value));
    CL_LOG_DEBUG_VERBOSE(("StreamNode: %.*s",
                          pStreamScopeNode->length, pStreamScopeNode->value));

    /*
     * shmName = /cl_pStreamName_pStreamScope_clIocLocalAddress 
     * nChar   => /cl_%s_%s_%d\0 = 7 + sizeof( ClUint32T) + NameLength +
     * scopeLength   
     */
    nChar = pStreamScopeNode->length + pStreamName->length + sizeof(ClUint32T) + 7;
    /*6 == "/cl_%s_%s\0"*/
    if( CL_MAX_NAME_LENGTH < nChar )
    {
        nChar = CL_MAX_NAME_LENGTH;
    }

    CL_LOG_DEBUG_VERBOSE(("nChar: %hu", nChar));
    pShmName->pValue = clHeapCalloc(nChar, sizeof(ClCharT));
    if( NULL == pShmName->pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }

    pShmName->length = nChar;
    nChar = snprintf(pShmName->pValue, nChar, "/CL_%.*s_%.*s_%d", pStreamScopeNode->length,
             pStreamScopeNode->value, pStreamName->length, pStreamName->value,
             clIocLocalAddressGet());
    pShmName->pValue[nChar] = '\0';

    CL_LOG_DEBUG_TRACE(("Exit: shmName: %s", pShmName->pValue));
    return rc;
}

ClRcT
clLogShmNameDestroy(ClStringT  *pShmName)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_ASSERT(NULL != pShmName);

    CL_LOG_DEBUG_VERBOSE(("StreamName: %s", pShmName->pValue));

    clHeapFree(pShmName->pValue);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogCompNamePrefixGet(SaNameT   *pCompName,
                       ClCharT   **ppCompPrefix)
{
    ClRcT      rc     = CL_OK;
    ClUint16T  cpmLen = strlen("cpm");

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pCompName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == ppCompPrefix), CL_LOG_RC(CL_ERR_NULL_POINTER));

    *ppCompPrefix = clHeapCalloc(pCompName->length + 1, sizeof(ClCharT));
    if( NULL == *ppCompPrefix )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    if( !memcmp(pCompName->value, "cpm", cpmLen) )
    {
        memcpy(*ppCompPrefix, "cpm", cpmLen);
        (*ppCompPrefix)[cpmLen + 1] = '\0';
    }
    else
    {
        sscanf((ClCharT *)pCompName->value, "%[^_]", *ppCompPrefix);
        (*ppCompPrefix)[pCompName->length] = '\0';
    }

    CL_LOG_DEBUG_TRACE(("Exit: compPrefix: %s", *ppCompPrefix));
    return rc;
}

ClRcT
clLogFileCreat(ClCharT  *fileName, ClLogFilePtrT  *pFp)
{
    ClRcT          rc = CL_OK;
    ClLogFilePtrT  fp = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter: fileName: %s", fileName));

    if( NULL != (fp = fopen(fileName, "r")) )
    {
        fclose(fp);
        CL_LOG_DEBUG_TRACE(("LogFile is already exist"));
        return CL_LOG_RC(CL_ERR_ALREADY_EXIST);
    }
    else if( NULL == (*pFp = fopen(fileName, "w")) )
    {
        if(errno == EACCES)
        {
            return CL_LOG_RC(CL_ERR_OP_NOT_PERMITTED);
        }
        else if(errno == ENOENT)
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }

        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOpen_L(ClCharT  *fileName, ClLogFilePtrT  *pFp)
{
    ClRcT  rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter: fileName: %s", fileName));

    if( NULL == (*pFp = fopen(fileName, "r+")) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileClose_L(ClLogFilePtrT fp)
{
    ClRcT  rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!fp) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    if( 0 != fclose(fp) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileWrite(ClLogFilePtrT  fp, void *pData, ClUint32T  size)
{
    ClRcT  rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!fp) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    if( 0 == (size = fwrite(pData, 1, size, fp)) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    fflush(fp);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileRead(ClLogFilePtrT  fp, void *pData, ClUint32T *pNumOfBytes)
{
    ClRcT  rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
   
    if(!fp) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    if( 0 == (*pNumOfBytes = fread(pData, *pNumOfBytes, 1, fp)) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileSeek(ClLogFilePtrT  fp, ClUint32T  offSet)
{
    ClRcT     rc  = CL_OK;
    ClUint32T err = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter: %u  %d", offSet, offSet));

    if(!fp) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    if( 0 != (err = fseek(fp, offSet, SEEK_SET)) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileNo(ClLogFilePtrT  fp, ClHandleT *fd)
{
    ClRcT  rc = CL_OK;
    ClInt32T fileFd = 0;
    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!fp || !fd)
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    fileFd = fileno(fp);
    if(fileFd == -1)
    {
        perror("fileno");
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }
    *fd = (ClHandleT)fileFd;
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileUnlink(const ClCharT *name)
{
    CL_LOG_DEBUG_TRACE(("Enter: %s", name));

    if( 0 != unlink(name) )
    {
        if( errno == ENOENT )
        {
            return CL_LOG_RC(CL_ERR_NOT_EXIST);
        }
        perror("unlink()");
        CL_LOG_DEBUG_ERROR(("clLogFileUnlink()")); 
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clLogSymLink(ClCharT *oldFileName, ClCharT  *newFileName)
{
    ClRcT rc = CL_OK;
    int ret;
    int dirPortion;
    char* o;
    char* n;
    CL_LOG_DEBUG_TRACE(("oldFileName: %s newFileName: %s ", oldFileName, newFileName));

    o = strrchr(oldFileName, '/');
    n = strrchr(newFileName, '/');

    dirPortion = n - newFileName;

    if (o && n && ((o - oldFileName) == dirPortion) && /* If they both have paths AND their paths are the same length */
                    (strncmp(oldFileName, newFileName, dirPortion) == 0)) /* AND the paths are the same then */
    {
        /* these paths are in the same directory so use a local symlink */
        ret = symlink(o + 1, newFileName);
    }
    else
    {
        ret = symlink(oldFileName, newFileName);
    }
    if ((ret != 0) && (errno != EEXIST))
    {
        char errorBuf[100];
        strerror_r(errno, errorBuf, 99);

        CL_LOG_DEBUG_ERROR(("symlink [%s] -> [%s] failed.  Error: [%s]",oldFileName,newFileName,errorBuf));
        return 1; //CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogReadLink(ClCharT   *softFileName, 
              ClCharT   *newFileName, 
              ClInt32T  *pFileNameLength)
{
    ClCharT path[CL_MAX_NAME_LENGTH] = {0};
    ClUint32T dirPortion = 0;
    ClCharT actualFileName[CL_MAX_NAME_LENGTH] = {0};
    ssize_t actualFileLen = 0;

    CL_LOG_DEBUG_TRACE(("fileName: %s fileNameLen : %d \n", softFileName, 
                        *pFileNameLength));
    if( (actualFileLen = readlink(softFileName, newFileName,
                                     *pFileNameLength - 1)) < 0 )
    {
        char errorBuf[100];
        strerror_r(errno, errorBuf, 99);

        CL_LOG_DEBUG_ERROR(("readlink [%s] with length [%d] failed.  Error: [%s]", softFileName, *pFileNameLength,errorBuf));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }

    *pFileNameLength = actualFileLen;
    newFileName[actualFileLen] = '\0';

    /*
     * If soft link is absolute path and actual file is relative path,
     * convert actual file to absolute path by reading soft link path
     */
    if (newFileName[0] != '/' && softFileName[0] == '/')
    {
        dirPortion = strrchr(softFileName, '/') - softFileName;

        // get path name for symlink file
        strncpy(path, softFileName, dirPortion);

        /*
         * Get full path for linked file
         */
        snprintf(actualFileName, CL_MAX_NAME_LENGTH, "%s/%s", path, newFileName);

        /*
         *
         */
        *pFileNameLength = strlen(actualFileName);
        newFileName = clHeapRealloc(newFileName, *pFileNameLength+1);
        newFileName[0] = '\0';
        strncat(newFileName, actualFileName, *pFileNameLength);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}    

ClRcT
clLogAddressForLocationGet(ClCharT        *pStr, 
                           ClIocAddressT  *pDestAddr,
                           ClBoolT        *pIsLogical)
{
    ClRcT           rc       = CL_OK;
    ClUint64T       address  = 0;
    ClCpmSlotInfoT  slotInfo = {0};

    CL_LOG_DEBUG_TRACE(("Enter: %s", pStr));

    *pIsLogical = CL_FALSE;
    if( ('*' == pStr[0]) && (1 == strlen(pStr)) )
    {
        pDestAddr->iocLogicalAddress =
            CL_IOC_LOG_LOGICAL_ADDRESS;
        CL_LOG_DEBUG_TRACE(("MulticastAddress: %llx ", address));
        *pIsLogical = CL_TRUE;
    }
    else
    {
        strncpy((ClCharT *)slotInfo.nodeName.value, pStr, CL_MAX_NAME_LENGTH-1);
        slotInfo.nodeName.length = strlen((const ClCharT *)slotInfo.nodeName.value);

        rc = clCpmSlotGet(CL_CPM_NODENAME, &slotInfo);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("FileLocation doesn't contain proper"
                                "format: nodeName:%s", pStr));
            return rc;
        }
        pDestAddr->iocPhyAddress.nodeAddress = slotInfo.nodeIocAddress;
        pDestAddr->iocPhyAddress.portId      = CL_IOC_LOG_PORT;
        CL_LOG_DEBUG_TRACE(("nodeAddress: %d", slotInfo.nodeIocAddress));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT 
clLogConfigDataGet(ClUint32T   *pMaxStreams,
                   ClUint32T   *pMaxComps,
                   ClUint32T   *pMaxShmPages,
                   ClUint32T   *pMaxRecordsInPacket) 
{

    ClRcT		  rc        = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    ClParserPtrT  head      = NULL;
    ClCharT       *aspPath = NULL;
    ClParserPtrT  fd        = NULL;
    ClParserPtrT  temp      = NULL;
    ClOsalSharedMutexFlagsT mutexMode = CL_OSAL_SHARED_SYSV_SEM;
    ClUint32T maxComps = (CL_LOG_COMPID_CLASS + 7) & ~7;
    aspPath = getenv("ASP_CONFIG");
    if( aspPath != NULL ) 
    {
        head = clParserOpenFile(aspPath, CL_LOG_DEFAULT_FILECONFIG);	
        if( head == NULL )
        {
            clLogWarning("SVR", "INI", 
                 "Log config file name has been changed from 'log.xml' to 'clLog.xml',"
                 "Please change the name in your config directory");
            head = clParserOpenFile(aspPath, "log.xml");
            if( head == NULL )
            {
                clLogError("SVR", "INI", "Config file clLog.xml is missing,"
                        "please edit that & proceed");
                return CL_LOG_RC(CL_ERR_NULL_POINTER);
            }
        }
    }
    else
    {
        CL_LOG_DEBUG_ERROR(("ASP_CONFIG path is not set in the environment"));
        return CL_LOG_RC(CL_ERR_DOESNT_EXIST);
    }

    if(NULL == (fd = clParserChild(head, "log")))
    {
        CL_LOG_DEBUG_ERROR(("Log Config xml is not proper: tag <log>"));
        goto ParseError;
    }

    if(NULL == (fd = clParserChild(fd, "logConfigData")))
    {
        CL_LOG_DEBUG_ERROR((
                    "Log Config xml is not proper: tag <logConfigData>"));
        goto ParseError;
    }
    
    if(NULL == (temp = clParserChild(fd, "maximumStreams")))
    {
        CL_LOG_DEBUG_ERROR((
                    "Log Config xml is not proper: tag <maximumStreams>"));
        goto ParseError;
    }
    if( NULL != pMaxStreams )
        *pMaxStreams = atoi(temp->txt);

    if(NULL == (temp = clParserChild(fd, "maximumComponents")))
    {
        CL_LOG_DEBUG_ERROR((
                    "Log Config xml is not proper: tag <maximumComponents>"));
        goto ParseError;
    }
    if( NULL != pMaxComps )
    {
        *pMaxComps = atoi(temp->txt);
        if(*pMaxComps < maxComps)
            *pMaxComps = maxComps;
        if(pMaxStreams)
            *pMaxStreams = CL_MAX(*pMaxStreams, *pMaxComps);
    }

    if(NULL == (temp = clParserChild(fd, "maximumSharedMemoryPages")))
    {
        CL_LOG_DEBUG_ERROR((
        "Log Config xml is not proper: tag <maximumSharedMemoryPages>"));
        goto ParseError;
    }
    if( NULL != pMaxShmPages )
        *pMaxShmPages = atoi(temp->txt);

    if(NULL == (temp = clParserChild(fd, "maximumRecordsInPacket")))
    {
        CL_LOG_DEBUG_ERROR((
        "Log Config xml is not proper: tag <maximumRecordsInPacket>"));
        goto ParseError;
    }
    if( NULL != pMaxRecordsInPacket )
        *pMaxRecordsInPacket = atoi(temp->txt);

    if( (temp = clParserChild(fd,"lockMode")) )
    {
        if(!strncasecmp(temp->txt,"sysv",4))
        {
            mutexMode = CL_OSAL_SHARED_SYSV_SEM;
        } 
        else if (!strncasecmp(temp->txt,"posix",5))
        {
            mutexMode = CL_OSAL_SHARED_POSIX_SEM;
        }
        else if(!strncasecmp(temp->txt,"mutex",5))
        {
            mutexMode = CL_OSAL_SHARED_NORMAL;
        }
        clLogStreamMutexModeSet(mutexMode);
    }
    clParserFree(head);
    return CL_OK;

ParseError:
    clParserFree(head);
    return rc;
}

ClLogStreamScopeT
clLogStr2StreamScopeGet(ClCharT  *pScope)
{
  if( NULL == pScope )
    {
      goto setDefaultScope;
    }
    if( !(strcmp(pScope, "GLOBAL")) )
    {
      return CL_LOG_STREAM_GLOBAL;
    }
    else if( !(strcmp(pScope, "LOCAL")) )
      {
        return CL_LOG_STREAM_LOCAL;
      }
 setDefaultScope:
    clLogMultiline(CL_LOG_SEV_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Scope value is not proper in the xml, setting to default [%s]\n"
                   "In clLog.xml, the value for tag streamScope is not proper\n"
                   "allowed values are LOCAL, GLOBAL", CL_LOG_DEFAULT_SCOPE_STR);
    return CL_LOG_DEFAULT_SCOPE;
}

ClLogFileFullActionT
clLogStr2FileActionGet(ClCharT  *pFileActionStr)
{
  if( NULL == pFileActionStr )
    {
      goto setDefaultFileAction;
    }   
  if( !(strcmp(pFileActionStr, "HALT") ) )
    {
    return CL_LOG_FILE_FULL_ACTION_HALT;
    }
  else if( !(strcmp(pFileActionStr, "WRAP") ))
    {
      return CL_LOG_FILE_FULL_ACTION_WRAP;
    }
  else if( !(strcmp(pFileActionStr, "ROTATE") ))
    {
      return CL_LOG_FILE_FULL_ACTION_ROTATE;
    }
 setDefaultFileAction:
  clLogMultiline(CL_LOG_SEV_WARNING, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, 
                 "FileFullAction is not proper, setting to default [%s]\n"
                 "In clLog.xml, the value for tag fileFullAction is not proper\n"
                 "allowed values are WRAP, HALT, ROTATE", CL_LOG_DEFAULT_FF_ACTION_STR);
  return CL_LOG_DEFAULT_FF_ACTION;
}

ClRcT
clLogFileIOVwrite(ClLogFilePtrT  fp,
                  struct iovec   *pIov,
                  ClUint32T      numRecords,
                  ClUint32T      *pNumBytes)
{
    ClRcT      rc       = CL_OK;
    ClHandleT  fd       = 0;
    
    rc = clLogFileNo(fp, &fd);
    if( CL_OK != rc )
    {
        return rc;
    }
    *pNumBytes = writev(fd, pIov, numRecords);
    if( *pNumBytes == -1 )
    {
        fprintf(stderr, "Failed to write the data into the file\n");
        return rc;
    }
    fsync(fd);
    return rc;
}

ClUint32T clLogDefaultStreamSeverityGet(SaNameT *pStreamName)
{
    static ClUint32T defaultStreamSeverity, customStreamSeverity;
    const ClCharT *sev = NULL;
    ClLogSeverityT severity = 0;

    if(!pStreamName) return CL_LOG_DEFAULT_SEVERITY_FILTER;

    if(!strncmp((const ClCharT*)pStreamName->value, gSystemStreamName,
                 pStreamName->length) 
       ||
       !strncmp((const ClCharT*)pStreamName->value, gAppStreamName,
                 pStreamName->length)
       )
    {
        if(!defaultStreamSeverity)
        {
            if(!(sev = getenv("CL_LOG_STREAM_SEVERITY")) ) 
            {
                defaultStreamSeverity = CL_LOG_DEFAULT_SEVERITY_FILTER;
                return defaultStreamSeverity;
            }
            severity = clLogSeverityGet(sev);
            defaultStreamSeverity = (1 << severity) - 1;
        }
        return defaultStreamSeverity;
    }
    if(!customStreamSeverity)
    {
        if(!(sev = getenv("CL_LOG_STREAM_SEVERITY_CUSTOM")) )
        {
            customStreamSeverity = CL_LOG_DEFAULT_SEVERITY_FILTER;
            return customStreamSeverity;
        }
        severity = clLogSeverityGet(sev);
        customStreamSeverity = (1 << severity) - 1;
    }
    return customStreamSeverity;
}
