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
#include <clIdlApi.h>
#include <clLogFileOwner.h>
#include <clLogServer.h>
#include <clLogMaster.h>
#include <clLogMasterEo.h>
#include <clLogFileOwnerEo.h>
#include <LogPortMasterClient.h>

ClRcT
clLogFileOwnerStreamReopen(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                            ClNameT                 *pStreamName,
                            ClLogStreamScopeT       streamScope, 
                            ClNameT                 *pStreamScopeNode,
                            ClUint16T               streamId,
                            ClLogStreamAttrIDLT     *pStreamAttr, 
                            ClBoolT                 logRestart,
                            ClCntNodeHandleT        *phFileNode,
                            ClCntNodeHandleT        *phStreamNode)
{
    ClRcT                 rc               = CL_OK;
    ClBoolT               entryAdd         = CL_FALSE;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClBoolT               streamEntryAdd   = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileEntryGet(pFileOwnerEoEntry, pStreamAttr,
                                     logRestart, phFileNode, &entryAdd);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, *phFileNode,
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        if( CL_TRUE == entryAdd )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileOwnerEoEntry->hFileTable,
                                           *phFileNode), CL_OK);
        }
        return rc;
    }

    rc = clLogFileOwnerStreamEntryGet(pFileOwnerData, pStreamName, 
                                       pStreamScopeNode, phStreamNode, 
                                       &streamEntryAdd);
    if( CL_OK != rc )
    {
        if( CL_TRUE == entryAdd )
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileOwnerEoEntry->hFileTable,
                                           *phFileNode), CL_OK);
        }
        return rc;
    }
#if 0
    if( CL_FALSE == streamEntryAdd )
    {
        CL_LOG_DEBUG_ERROR(("StreamEntry is already exist"));
        return CL_LOG_RC(CL_ERR_ALREADY_EXIST);
    }
#endif
                        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamEntryAdd(ClNameT             *pStreamName,
                             ClLogStreamScopeT    streamScope, 
                             ClNameT              *pStreamScopeNode,
                             ClUint16T            streamId,
                             ClLogStreamAttrIDLT  *pStreamAttr, 
                             ClBoolT              logRestart)
{
    ClRcT                   rc                  = CL_OK;
    ClRcT                   retCode             = CL_OK;
    ClBoolT                 fileOwner           = CL_FALSE;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClCntNodeHandleT        hFileNode           = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT        hStreamNode         = CL_HANDLE_INVALID_VALUE;
    ClHandleT               hFileOwner         = CL_HANDLE_INVALID_VALUE;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, &pStreamAttr->fileLocation,
                                      &fileOwner);  
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE == pFileOwnerEoEntry->hFileTable )
    {
        rc = clLogFileOwnerEoEntryUpdate(pFileOwnerEoEntry);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                           CL_OK);
            return rc;
        }
    }

    rc = clLogFileOwnerStreamReopen(pFileOwnerEoEntry, pStreamName,
                                    streamScope, pStreamScopeNode, streamId,
                                    pStreamAttr, logRestart, &hFileNode, 
                                    &hStreamNode);

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                   CL_OK);
    if( CL_OK != rc )
    {
        return rc;
    }

    if(streamScope != CL_LOG_STREAM_LOCAL)
    {
        retCode = clLogHandlerRegister(pFileOwnerEoEntry->hLog, *pStreamName, 
                                       streamScope, *pStreamScopeNode,
                                       0, &hFileOwner);
    }
    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( (CL_OK != rc) || (CL_OK != retCode) )
    {
        CL_LOG_CLEANUP(clLogFileOwnerStreamEntryRemove(pFileOwnerEoEntry, 
                                                       hFileNode, pStreamName,
                                                       streamScope,
                                                       pStreamScopeNode), CL_OK);
        if( CL_OK == rc )
        {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                           CL_OK);
        }
        return rc;
    }
    rc = clLogFileOwnerStreamEntryChkNGet(pFileOwnerEoEntry, hFileNode,
                                          pStreamName, streamScope,
                                          pStreamScopeNode, &hStreamNode);
    if( CL_OK != rc )
    {
        if(streamScope != CL_LOG_STREAM_LOCAL)
            clLogHandlerDeregister(hFileOwner);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);
        return rc;
    }

    if(streamScope != CL_LOG_STREAM_LOCAL)
    {
        rc = clLogFileOwnerHandlerRegister(pFileOwnerEoEntry, hFileOwner, 
                                           pStreamName, streamScope,
                                           pStreamScopeNode, streamId,
                                           hFileNode, hStreamNode, CL_FALSE);
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                           CL_OK);
            CL_LOG_CLEANUP(clLogFileOwnerStreamEntryRemove(pFileOwnerEoEntry, 
                                                           hFileNode, pStreamName, 
                                                           streamScope,
                                                           pStreamScopeNode), CL_OK);
            return rc;
        }
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                   CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamListEntryGet(ClBufferHandleT      msg,
                                  ClBoolT              logRestart, 
                                  ClLogStreamAttrIDLT  *pStreamAttr)
{
    ClRcT              rc              = CL_OK;
    ClNameT            streamName      = {0};
    ClNameT            streamScopeNode = {0};
    ClLogStreamScopeT  streamScope     = 0;
    ClUint16T          streamId        = 0;
    ClUint32T          validEntry      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clXdrUnmarshallClUint32T(msg, &validEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( 0 == validEntry )
    {
        return CL_OK;
    }
        
    rc = clXdrUnmarshallClNameT(msg, &streamName);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClNameT(msg, &streamScopeNode);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClUint32T(msg, &streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clXdrUnmarshallClUint16T(msg, &streamId);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileOwnerStreamEntryAdd(&streamName, streamScope, 
                                       &streamScopeNode, streamId,
                                       pStreamAttr, logRestart);
    if( CL_OK != rc )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryUnpack(ClBoolT          logRestart, 
                               ClBufferHandleT  msg)
{
    ClRcT                rc         = CL_OK;
    ClUint32T            count      = 0;
    ClUint32T            strmCnt    = 0;
    ClUint32T            numFiles   = 0;
    ClLogStreamAttrIDLT  streamAttr = {{0}};
    ClUint32T            numStreams = 0;

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
            rc = clLogFileOwnerStreamListEntryGet(msg, logRestart, 
                                                   &streamAttr);
            if( CL_OK != rc )
            {
                break;
            }
        }

        clHeapFree(streamAttr.fileLocation.pValue);
        clHeapFree(streamAttr.fileName.pValue);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileTableRecreate(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                 ClBoolT                 logRestart, 
                                 ClUint32T               numStreams,
                                 ClUint32T               buffLen,
                                 ClUint8T                *pBuffer)
{
    ClRcT            rc  = CL_OK;
    ClBufferHandleT  msg = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, buffLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clLogFileOwnerFileEntryUnpack(logRestart, msg);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCompEntriesAdd(ClUint32T  numEntries,
                              ClUint32T  buffLen,
                              ClUint8T   *pCompData) 
{
    ClRcT            rc       = CL_OK;
    ClUint32T        count    = 0;
    ClLogCompDataT   compData = {{0}};
    ClBufferHandleT  msg      = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(); rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pCompData, buffLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    for( count = 0; count < numEntries; count++ )
    {
        rc = VDECL_VER(clXdrUnmarshallClLogCompDataT, 4, 0, 0)(msg, &compData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrUnmarshallClLogCompDataT, 4, 0, 0)(): rc[0x %x]",
                    rc));
            CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
            return rc;
        }
        rc = clLogFileOwnerCompAddEvent(&compData);
        if( CL_OK != rc )
        {
            /*
             * Just log the error, carry on to the table 
             */
            CL_LOG_DEBUG_ERROR(("clLogFileOwnerCompAddEvent(): rc[0x %x]",
                        rc));
        }
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStateRecover(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                            ClBoolT                 logRestart)
{
    ClRcT         rc         = CL_OK;
    ClIdlHandleT  hLogIdl    = CL_HANDLE_INVALID_VALUE;
    ClUint8T      *pBuffer   = NULL;
    ClUint32T     buffLen    = 0;
    ClUint32T     numEntries = 0;
    ClUint32T     retryCnt   = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec =  50 };
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrIdlHandleInitialize(CL_LOG_STREAM_GLOBAL, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }

    do
    {
        rc = VDECL_VER(clLogMasterStreamListGetClientSync, 4, 0, 0)(hLogIdl, &numEntries, &buffLen, &pBuffer);
        if( CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc) )
        {
            clOsalTaskDelay(delay);
        }
        else
        {
            break;
        }
    }while( retryCnt++ < 3 );
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        if( CL_FALSE == logRestart )
        {
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            return CL_OK;
        }
    }
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterStreamListGet, 4, 0, 0)(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Num of streams: %d ", numEntries));

    rc = clLogFileOwnerFileTableRecreate(pFileOwnerEoEntry, logRestart,
                                          numEntries, buffLen, pBuffer);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        clHeapFree(pBuffer);
        return rc;
    }
    clHeapFree(pBuffer);

    if( CL_FALSE == logRestart )
    {
        retryCnt = 0;
        do
        {
            rc = VDECL_VER(clLogMasterCompListGetClientSync, 4, 0, 0)(hLogIdl, &numEntries, &buffLen, &pBuffer);
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
            {
                clOsalTaskDelay(delay);
            }
            else
            {
                break;
            }
        } while(retryCnt++ < 3);

        if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            /*its temporay fix..clean solution is needed.FIXME */
            if( CL_FALSE == logRestart )
            {
                CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
                return CL_OK;
            }
        }
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterCompListGet, 4, 0, 0)(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
            return rc; 
        }
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        CL_LOG_DEBUG_TRACE(("Num of components: %d ", numEntries));
        rc = clLogFileOwnerCompEntriesAdd(numEntries, buffLen, pBuffer);
        if( CL_OK != rc )
        {
            clHeapFree(pBuffer);
            return rc;
        }
        clHeapFree(pBuffer);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamEntryReopen(ClCntKeyHandleT   key, 
                                 ClCntDataHandleT  data,
                                 ClCntArgHandleT   arg,
                                 ClUint32T         size)
{
    ClRcT                   rc           = CL_OK;
    ClLogMasterStreamDataT  *pStreamData = (ClLogMasterStreamDataT *) data;
    ClLogStreamKeyT         *pStreamKey  = (ClLogStreamKeyT *) key;
    ClLogStreamScopeT       streamScope  = 0;
    ClLogStreamAttrIDLT     *pStreamAttr = (ClLogStreamAttrIDLT *) arg;

    CL_LOG_DEBUG_TRACE(("Enter"));

    /* Explicity not checking, allowing to create as many entries as it can */
    if( CL_IOC_RESERVED_ADDRESS != pStreamData->streamMcastAddr )
    {
        clLogInfo(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT,
                  "Recreating files for stream Name: %.*s streamScopeNode: %.*s\n", 
                   pStreamKey->streamName.length, pStreamKey->streamName.value, 
                   pStreamKey->streamScopeNode.length, pStreamKey->streamScopeNode.value);
        rc = clLogStreamScopeGet(&pStreamKey->streamScopeNode, &streamScope);
        if( CL_OK != rc )
        {
            return rc;            
        }
        rc = clLogFileOwnerStreamCreateEvent(&pStreamKey->streamName, streamScope, 
                                         &pStreamKey->streamScopeNode,
                                         pStreamData->streamId, pStreamAttr,
                                         CL_TRUE);
        if( rc != CL_OK )
        {
            rc = CL_OK;
            /* 
             * this particular stream doesn't exist, so marking this as
             * invalid 
             */
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStateRecreate(ClCntKeyHandleT   key,
                             ClCntDataHandleT  data,
                             ClCntArgHandleT   arg,
                             ClUint32T         size)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileDataT          *pFileData          = (ClLogFileDataT *) data;
    ClLogFileKeyT           *pFileKey           = (ClLogFileKeyT *) key;
    ClBoolT                 fileOwner           = CL_FALSE;
    ClLogStreamAttrIDLT     streamAttr          = {{0}};
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, &pFileKey->fileLocation,
                                      &fileOwner);
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        return rc;
    }

    rc = clLogStreamAttributesCopy(&pFileData->streamAttr, &streamAttr,
                                   CL_FALSE);
    if( CL_OK != rc )
    {
        return rc;
    }
    streamAttr.fileName.length     = pFileKey->fileName.length;
    streamAttr.fileLocation.length = pFileKey->fileLocation.length;
    streamAttr.fileName.pValue  
        = clHeapCalloc(pFileKey->fileName.length, sizeof(ClCharT));
    if( NULL == streamAttr.fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           
    streamAttr.fileLocation.pValue  
        = clHeapCalloc(pFileKey->fileLocation.length, sizeof(ClCharT));
    if( NULL == streamAttr.fileLocation.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(streamAttr.fileName.pValue);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           

    memcpy(streamAttr.fileName.pValue, pFileKey->fileName.pValue,
           streamAttr.fileName.length);
    memcpy(streamAttr.fileLocation.pValue, pFileKey->fileLocation.pValue,
           streamAttr.fileLocation.length);
    
    rc = clCntWalk(pFileData->hStreamTable, clLogFileOwnerStreamEntryReopen,
                   &streamAttr, sizeof(streamAttr));

    if (streamAttr.fileName.pValue != NULL)
    {
        clHeapFree(streamAttr.fileName.pValue);
        streamAttr.fileName.pValue = NULL;
    }

    if (streamAttr.fileLocation.pValue != NULL)
      {
        clHeapFree(streamAttr.fileLocation.pValue);
        streamAttr.fileLocation.pValue = NULL;
      }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


ClRcT
clLogFileOwnerMasterStateRecover(void)
{
    ClRcT               rc              = CL_OK;
    ClLogMasterEoDataT  *pMasterEoEntry = NULL;
    ClUint8T            *pBuffer        = NULL;
    ClUint32T           buffLen         = 0;
    ClUint32T           numEntries      = 0;
    ClIdlHandleT        hLogIdl         = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(&pMasterEoEntry, NULL);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc [0x%x]", rc));
        return rc;
    }

    rc = clCntWalk(pMasterEoEntry->hMasterFileTable,
                   clLogFileOwnerStateRecreate, NULL, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
                CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
            CL_OK);

    rc = clLogSvrIdlHandleInitialize(CL_LOG_STREAM_GLOBAL, &hLogIdl);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = VDECL_VER(clLogMasterCompListGetClientSync, 4, 0, 0)(hLogIdl, &numEntries, &buffLen, &pBuffer);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        /*its temporay fix..clean solution is needed.FIXME */
        CL_LOG_DEBUG_ERROR(("No components availbale at Master"));
        return CL_OK;
    }
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clLogMasterCompListGet, 4, 0, 0)(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
        return rc; 
    }
    CL_LOG_CLEANUP(clIdlHandleFinalize(hLogIdl), CL_OK);
    CL_LOG_DEBUG_TRACE(("Num of components: %d ", numEntries));
    rc = clLogFileOwnerCompEntriesAdd(numEntries, buffLen, pBuffer);
    if( CL_OK != rc )
    {
        clHeapFree(pBuffer);
        return rc;
    }
    clHeapFree(pBuffer);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
