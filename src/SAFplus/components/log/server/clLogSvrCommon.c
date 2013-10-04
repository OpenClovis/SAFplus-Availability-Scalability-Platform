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
#include <ctype.h>

#include <clCksmApi.h>
#include <clCpmExtApi.h>
#include <clCkptExtApi.h>
#include <clParserApi.h>

#include <clLogSvrCommon.h>
#include <clLogSvrEo.h>
#include <clLogErrors.h>

#include <LogServer.h>
#include <LogClient.h>
#include <clLogFileEvt.h>

const ClVersionT  gCkptVersion = {'B', 0x1, 0x1};
const ClVersionT  gLogVersion  = {'B', 0x1, 0x1};

ClRcT
clLogSvrCommonDataInit(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClEoExecutionObjT      *pEoObj            = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    pSvrCommonEoEntry = clHeapCalloc(1, sizeof(ClLogSvrCommonEoDataT));
    if( NULL == pSvrCommonEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    pSvrCommonEoEntry->masterAddr    = CL_IOC_RESERVED_ADDRESS;
    pSvrCommonEoEntry->deputyAddr    = CL_IOC_RESERVED_ADDRESS;
    pSvrCommonEoEntry->hSvrCkpt      = CL_HANDLE_INVALID_VALUE;
    pSvrCommonEoEntry->hLibCkpt      = CL_HANDLE_INVALID_VALUE;
    /* FIXME: Get the values of maxStreams,maxComponents and shmSize from config*/
    rc = clLogConfigDataGet(&pSvrCommonEoEntry->maxStreams, 
                            &pSvrCommonEoEntry->maxComponents,
                            &pSvrCommonEoEntry->numShmPages, NULL);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogConfigDataGet(): rc[0x %x]", rc));
        clHeapFree(pSvrCommonEoEntry);
        return rc;
    }
    pSvrCommonEoEntry->maxMsgs       = CL_LOG_MAX_MSGS;

    rc = clCkptLibraryInitialize(&pSvrCommonEoEntry->hLibCkpt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryInitialize(): rc[0x %x]", rc));
        clHeapFree(pSvrCommonEoEntry);
        return rc;
    }

    rc = clLogClientInstall();
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClientInstall(): rc[0x%x]", rc));
        CL_LOG_CLEANUP(clCkptLibraryFinalize(pSvrCommonEoEntry->hLibCkpt), CL_OK);
        clHeapFree(pSvrCommonEoEntry);
        return rc;
    }

    rc = clLogClientTableRegister(CL_IOC_LOG_PORT);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClientTableRegister: rc[0x%x]", rc));
        CL_LOG_CLEANUP(clLogClientUninstall(), CL_OK);
        CL_LOG_CLEANUP(clCkptLibraryFinalize(pSvrCommonEoEntry->hLibCkpt), CL_OK);
        clHeapFree(pSvrCommonEoEntry);
        return rc;
    }
    rc = clOsalMutexInit(&pSvrCommonEoEntry->lock);
    if(rc != CL_OK)
    {
        CL_LOG_DEBUG_ERROR(("log commoneo lock initialize failed with [%#x]", rc));
        CL_LOG_CLEANUP(clLogClientUninstall(), CL_OK);
        CL_LOG_CLEANUP(clCkptLibraryFinalize(pSvrCommonEoEntry->hLibCkpt), CL_OK);
        clHeapFree(pSvrCommonEoEntry);
        return rc;
    }
    pSvrCommonEoEntry->flags |= __LOG_EO_INIT;
    rc = clEoPrivateDataSet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                            pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc)); 
        CL_LOG_CLEANUP(clLogClientTableDeregister(), CL_OK);
        CL_LOG_CLEANUP(clLogClientUninstall(), CL_OK);
        CL_LOG_CLEANUP(clCkptLibraryFinalize(pSvrCommonEoEntry->hLibCkpt), CL_OK);
        clOsalMutexDestroy(&pSvrCommonEoEntry->lock);
        clHeapFree(pSvrCommonEoEntry);
    }
    
    CL_LOG_DEBUG_TRACE(("Exit: rc [0x %x]", rc));
    return rc;
}

ClRcT
clLogSvrCommonDataFinalize(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_CLEANUP(clLogClientTableDeregister(), CL_OK);
    CL_LOG_CLEANUP(clLogClientUninstall(), CL_OK);

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x%x]", rc));
        return rc;
    }
    CL_LOG_CLEANUP(clLogSvrCkptFinalize(pSvrCommonEoEntry), CL_OK);

    clOsalMutexLock(&pSvrCommonEoEntry->lock);
    pSvrCommonEoEntry->flags = 0;
    clOsalMutexUnlock(&pSvrCommonEoEntry->lock);
    /*
     * Avoid freeing the common eodata cookie entry as it could be in use.
    clHeapFree(pSvrCommonEoEntry);
    */
    
    CL_LOG_DEBUG_TRACE(("Exit: rc [0x %x]", rc));
    return rc;
}

ClRcT
clLogSvrCkptFinalize(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry)
{
    ClRcT  rc = CL_OK;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

#if 0
    rc = clCkptFinalize(pSvrCommonEoEntry->hSvrCkpt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptFinalize(): rc[0x %x]", rc));
    }
#endif

    rc = clCkptLibraryFinalize(pSvrCommonEoEntry->hLibCkpt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCkptLibraryFinalize(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * Function - clLogStreamAttributesCopy()
 *  it will copy the variables from stored attr to passed attr.
 *  it will copy the filePath, if copyFilePath is CL_TRUE.
 */
ClRcT
clLogStreamAttributesCopy(ClLogStreamAttrIDLT  *pStoredAttr,
                          ClLogStreamAttrIDLT  *pPassedAttr, 
                          ClBoolT              copyFilePath)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(( "Enter"));
    
    CL_LOG_PARAM_CHK((NULL == pStoredAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pPassedAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));

    *pPassedAttr = *pStoredAttr;
    if( CL_TRUE == copyFilePath )
    {
        if( (NULL == pStoredAttr->fileName.pValue) || 
            (NULL == pStoredAttr->fileLocation.pValue) )
        {
            CL_LOG_DEBUG_ERROR(("FileURL or FileName are NULL pointer"));
            return CL_LOG_RC(CL_ERR_NULL_POINTER);
        }    

        pPassedAttr->fileName.pValue  
            = clHeapCalloc(pStoredAttr->fileName.length, sizeof(ClCharT));
        if( NULL == pPassedAttr->fileName.pValue )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }           
        pPassedAttr->fileLocation.pValue  
            = clHeapCalloc(pStoredAttr->fileLocation.length, sizeof(ClCharT));
        if( NULL == pPassedAttr->fileLocation.pValue )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            clHeapFree(pPassedAttr->fileName.pValue);
            return CL_LOG_RC(CL_ERR_NO_MEMORY);
        }           

        memcpy(pPassedAttr->fileName.pValue, pStoredAttr->fileName.pValue,
               pPassedAttr->fileName.length);
        memcpy(pPassedAttr->fileLocation.pValue,
               pStoredAttr->fileLocation.pValue,
               pPassedAttr->fileLocation.length);
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/* 
 * Function - clLogAttributesMatch()
 *  Compare each field of passed streamAttributes with stored attributes.
 *  it will compare the filePath, if compareFilePath is CL_TRUE.
 */
ClRcT
clLogAttributesMatch(ClLogStreamAttrIDLT  *pStoredAttr,
                     ClLogStreamAttrIDLT  *pPassedAttr,
                     ClBoolT              compareFilePath)
{
    ClRcT rc = CL_OK;

    CL_LOG_DEBUG_TRACE(( "Enter"));

    CL_LOG_PARAM_CHK((NULL == pStoredAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pPassedAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));
    
    if( CL_TRUE == compareFilePath )
    {
        if( strcmp(pStoredAttr->fileName.pValue, 
                   pPassedAttr->fileName.pValue) )
        {
            CL_LOG_DEBUG_ERROR(("Stored: fileName: %s Passed: %s", 
                                pStoredAttr->fileName.pValue, 
                                pPassedAttr->fileName.pValue)); 
            CL_LOG_DEBUG_ERROR(("FileName is not Proper"));
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }    
        if( strcmp(pStoredAttr->fileLocation.pValue,
                   pPassedAttr->fileLocation.pValue) )
        {
            CL_LOG_DEBUG_ERROR(("Stored: fileLocation: %s Passed: %s", 
                                 pStoredAttr->fileLocation.pValue,
                                 pPassedAttr->fileLocation.pValue)); 
            CL_LOG_DEBUG_ERROR(("FileLocation is not Proper"));
            return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
        }    
    }

    if( pStoredAttr->fileUnitSize != pPassedAttr->fileUnitSize)
    {
        CL_LOG_DEBUG_ERROR(("MaxFileSize is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %d Passed: %d",
                    pPassedAttr->fileUnitSize, 
                    pStoredAttr->fileUnitSize));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    if( pStoredAttr->recordSize != pPassedAttr->recordSize)
    {
        CL_LOG_DEBUG_ERROR(("MaxRecordSize is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %d Passed: %d",
                    pPassedAttr->recordSize, pStoredAttr->recordSize));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    #if 0
    if( pStoredAttr->haProperty != pPassedAttr->haProperty )
    {
        CL_LOG_DEBUG_ERROR(("haProperty is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %d Passed: %d",
                    pPassedAttr->haProperty, pStoredAttr->haProperty));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    #endif
    if( pStoredAttr->fileFullAction != pPassedAttr->fileFullAction )
    {    
        CL_LOG_DEBUG_ERROR(("logFileFullAction is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %d Passed: %d",
                           pPassedAttr->fileFullAction, 
                           pStoredAttr->fileFullAction));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    if( pStoredAttr->maxFilesRotated != pPassedAttr->maxFilesRotated )
    {
        CL_LOG_DEBUG_ERROR(("MaxFilesRotated is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %d Passed: %d",
                    pPassedAttr->maxFilesRotated, 
                    pStoredAttr->maxFilesRotated));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    #if 0
    if( pStoredAttr->flushInterval != pPassedAttr->flushInterval )
    {    
        CL_LOG_DEBUG_ERROR(("flushInterval is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %lld Passed: %lld",
                            pPassedAttr->flushInterval, 
                            pStoredAttr->flushInterval));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }   
    #endif
    if( (pStoredAttr->waterMark.lowLimit != pPassedAttr->waterMark.lowLimit)
            &&
            (pStoredAttr->waterMark.highLimit != pPassedAttr->waterMark.highLimit)
      )
    {    
        CL_LOG_DEBUG_ERROR(("highWatermark is not proper"));
        CL_LOG_DEBUG_ERROR(("Stored: %llu Passed: %llu",
                           pPassedAttr->waterMark.lowLimit,
                           pStoredAttr->waterMark.lowLimit));
        CL_LOG_DEBUG_ERROR(("Stored: %llu Passed: %llu",
                           pPassedAttr->waterMark.highLimit,
                           pStoredAttr->waterMark.highLimit));
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    }    
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}    

ClRcT
clLogIdlHandleInitialize(ClIocAddressT  destAddr, 
                         ClIdlHandleT   *phLogIdl)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK; 

    CL_LOG_DEBUG_TRACE(("Enter"));

    address.addressType        = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress = destAddr;
    idlObj.address             = address;
    idlObj.flags               = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout     = CL_LOG_SVR_DEFAULT_TIMEOUT;
    idlObj.options.priority    = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries     = CL_LOG_SVR_DEFAULT_RETRY;

    rc = clIdlHandleInitialize(&idlObj, phLogIdl);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIdlHandleInitialize(): rc[0x %x]", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


ClRcT
clLogServerSerialiser(ClUint32T  dsId,
                      ClAddrT    *pBuffer,
                      ClUint32T  *pSize,
                      ClPtrT     cookie)
{
    ClRcT           rc  = CL_OK;
    ClBufferHandleT msg = (ClBufferHandleT)cookie;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferLengthGet(msg, pSize);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x %x]", rc));
        return rc;
    }        
    *pBuffer = clHeapCalloc(*pSize, sizeof(ClUint8T));
    if( NULL == *pBuffer )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return rc;
    }    
    rc = clBufferNBytesRead(msg, (ClUint8T *)*pBuffer, pSize); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x %x]", rc));
        clHeapFree(*pBuffer);
        return rc;
    }    
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    


ClRcT 
clLogPrecreatedStreamsDataGet(ClLogStreamDataT     *pStreamAttr[],
                              ClUint32T            *pNumStreams) 
{

    ClRcT		         rc          = CL_OK;
    ClParserPtrT         head        = NULL;
    ClCharT              *aspPath    = NULL;
    ClParserPtrT         fd          = NULL;
    ClParserPtrT         fd1         = NULL;
    ClParserPtrT         temp        = NULL;
    ClUint32T            count       = 0;
    ClLogStreamDataT     *pData      = NULL;

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
            if( NULL == head )
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
    if(NULL != head)
    {
        if(NULL != (fd = clParserChild(head, "log")))
        {
          if(NULL != (fd = clParserChild(fd, "precreatedStreamsData")))
          {
            if( NULL != (fd1 = clParserChild(fd, "stream")))
            {
            while( NULL != fd1 )
            {
                pStreamAttr[count] = NULL;
                pStreamAttr[count] = clHeapCalloc(sizeof(ClLogStreamDataT), 
                                                      sizeof(ClCharT));
                pData = pStreamAttr[count];
                if( NULL == pData )
                {
                    CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                    rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
                    goto ParseError;
                }
                snprintf((ClCharT *) ((pData->streamName).value), sizeof((pData->streamName).value), "%s", fd1->attr[1]);
                (pData->streamName).length = strlen((const ClCharT *) ((pData->streamName).value));

                if(NULL != (temp = clParserChild(fd1, "streamScope")))
                {
                  pData->streamScope = clLogStr2StreamScopeGet(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }
                
                if(NULL != (temp = clParserChild(fd1, "syslog")) )
                {
                    pData->streamAttr.syslog = (strncasecmp(temp->txt, "yes", 3)==0 ? CL_TRUE : CL_FALSE );
                }

                if(NULL != (temp = clParserChild(fd1, "fileName")))
                {
                    (pData->streamAttr).fileName.length = strlen(temp->txt) + 1;
                    (pData->streamAttr).fileName.pValue = 
                        clHeapCalloc((pData->streamAttr).fileName.length,
                            sizeof(ClCharT));
                    if( NULL == (pData->streamAttr).fileName.pValue )
                    {
                        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
                        (pData->streamAttr).fileName.length = 0;
                        goto ParseError;
                    }
                    snprintf((pData->streamAttr).fileName.pValue, (pData->streamAttr).fileName.length, "%s", temp->txt);
                    CL_LOG_DEBUG_TRACE((" fileLocation: %s ",
                                (pData->streamAttr).fileName.pValue));
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1, "fileLocation")))
                {
                     rc = clLogFileLocationFindNGet(temp->txt,
                                &((pData->streamAttr).fileLocation));
                     if( CL_OK != rc )
                    {
                        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
                        (pData->streamAttr).fileLocation.length = 0;
                        goto ParseError;
                    }
                    CL_LOG_DEBUG_TRACE((" fileLocation: %s ",
                                (pData->streamAttr).fileLocation.pValue));
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1,"fileUnitSize")))
                {
                    (pData->streamAttr).fileUnitSize = atoi(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1,"recordSize")))
                {
                    (pData->streamAttr).recordSize = atoi(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1, "fileFullAction")))
                {
                  (pData->streamAttr).fileFullAction = clLogStr2FileActionGet(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1, "maximumFilesRotated")))
                {
                    (pData->streamAttr).maxFilesRotated = atoi(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1, "flushFreq")))
                {
                    (pData->streamAttr).flushFreq = atoi(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }

                if(NULL != (temp = clParserChild(fd1, "flushInterval")))
                {
                    (pData->streamAttr).flushInterval = atoi(temp->txt);
                    rc = CL_OK;
                }
                else
                {
                    rc = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
                    goto ParseError;
                }
                (pData->streamAttr).haProperty      = CL_LOG_DEFAULT_HA_PROPERTY;
                (pData->streamAttr).waterMark.lowLimit = 
                                                CL_LOG_DEFAULT_LOW_WATER_MARK;
                (pData->streamAttr).waterMark.lowLimit = 
                                                CL_LOG_DEFAULT_HIGH_WATER_MARK;
                if( CL_LOG_MAX_PRECREATED_STREAMS == count )
                {
                    CL_LOG_DEBUG_ERROR(("XML parsing cannot be continued...Read"
                    " %d streams", CL_LOG_MAX_PRECREATED_STREAMS));
                    *pNumStreams = count;
                    goto ParseError;
                    
                }
                fd1 = fd1->next;
                count++;
            }
            }
          }
        }
    }
ParseError:
    clParserFree(head);
    if( CL_OK != rc )
    {
        //freeing up the last allocated data        
        pData = pStreamAttr[count];
        if( NULL != pData )
        {
            if( 0 != (pData->streamAttr).fileName.length )
            {
                clHeapFree((pData->streamAttr).fileName.pValue);
            }
            if( 0 != (pData->streamAttr).fileLocation.length )
            {
                clHeapFree((pData->streamAttr).fileLocation.pValue);
            }
            clHeapFree(pData);
        }
    }
    *pNumStreams = count;

    return rc;
}

ClRcT 
clLogSvrMutexModeSet(void)
{

    ClRcT		  rc        = CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    ClParserPtrT  head      = NULL;
    ClCharT       *aspPath  = NULL;
    ClParserPtrT  fd        = NULL;
    ClParserPtrT  temp      = NULL;
    ClOsalSharedMutexFlagsT mutexMode = CL_OSAL_SHARED_SYSV_SEM;

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
    
    if( (temp = clParserChild(fd,"lockMode")) )
    {
        if(!strncasecmp(temp->txt,"sysv",4))
        {
            mutexMode = CL_OSAL_SHARED_SYSV_SEM;
        }
        else if(!strncasecmp(temp->txt,"posix",5))
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

ClIocNodeAddressT 
clLogFileOwnerAddressFetch(ClStringT  *pFileLocation)
{
    ClRcT                  rc                          = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry          = NULL;
    ClIocNodeAddressT      localAddr                   =
        clIocLocalAddressGet(); 
    SaNameT                nodeName                    = {0};
    ClCharT                nodeStr[CL_MAX_NAME_LENGTH] = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCpmLocalNodeNameGet(&nodeName);
    if( CL_OK != rc )
    {
        return rc;
    }

    sscanf(pFileLocation->pValue, "%[^:]", nodeStr);
    CL_LOG_DEBUG_TRACE(("Nodestr extracted from fileLocation: %s", nodeStr));

    if( ('*' == nodeStr[0]) && ('\0' == nodeStr[1]) )
    {
        return pSvrCommonEoEntry->masterAddr;
    }
    else if( nodeName.length == strlen(nodeStr) 
             && 
             !(strncmp((const ClCharT *)nodeName.value, nodeStr, nodeName.length)) )
    {
        return localAddr;
    }
    else
    {
        clLogCritical(CL_LOG_AREA_SVR, CL_LOG_CTX_SVR_INIT, 
                "Could not find fileowner address, filelocation [%.*s] is not proper", 
                pFileLocation->length, pFileLocation->pValue);
    }
    /* Nothing matched, return reserved address which is ivalid */
    return CL_IOC_RESERVED_ADDRESS;
}
