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
#include <clLogMaster.h>
#include <clLogFileOwner.h>
#include <clLogFileOwnerUtils.h>
#include <clLogFileOwnerEo.h>

ClRcT
clLogFileOwnerStreamMapPopulate(ClCntKeyHandleT   key,
                                 ClCntDataHandleT  data,
                                 ClCntArgHandleT   arg,
                                 ClUint32T         size)
{
    ClRcT                      rc           = CL_OK;
    ClLogStreamKeyT            *pStreamKey  = (ClLogStreamKeyT *) key;
    ClLogFileOwnerStreamDataT  *pStreamData = 
                            (ClLogFileOwnerStreamDataT *) data;
    ClBufferHandleT            msg          = (ClBufferHandleT) arg;
    ClLogStreamMapT            streamMap    = {{0}};

    CL_LOG_DEBUG_TRACE(("Enter"));

    streamMap.streamName = pStreamKey->streamName;
    streamMap.nodeName   = pStreamKey->streamScopeNode;
    streamMap.streamId   = pStreamData->streamId;

    rc = clLogStreamScopeGet(&pStreamKey->streamScopeNode,
                             &streamMap.streamScope);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clBufferNBytesWrite(msg, (ClUint8T *) &streamMap,
                             sizeof(ClLogStreamMapT));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamMapCopy(ClLogFileOwnerDataT  *pFileOwnerData,
                            ClUint32T            *pNumStreams,
                            ClLogStreamMapT      **ppStreams)
{
    ClRcT            rc  = CL_OK;
    ClBufferHandleT  msg = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clCntSizeGet(pFileOwnerData->hStreamTable, pNumStreams);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Num of streams: %d", *pNumStreams));
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clCntWalk(pFileOwnerData->hStreamTable, 
                   clLogFileOwnerStreamMapPopulate,
                   (ClCntArgHandleT) msg, sizeof(msg));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clBufferFlatten(msg, (ClUint8T **) ppStreams);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferFlatten(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
       
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileDataGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                          ClStringT               *fileName,
                          ClStringT               *fileLocation,
                          ClLogFileOwnerDataT    **ppFileOwnerData)
{
    ClRcT          rc        = CL_OK;
    ClLogFileKeyT  *pFileKey = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileKeyCreate(fileName, fileLocation, 
                            CL_LOG_STREAM_HDLR_MAX_FILES, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntDataForKeyGet(pFileOwnerEoEntry->hFileTable, 
                            (ClCntKeyHandleT) pFileKey,
                            (ClCntDataHandleT *) ppFileOwnerData);
    if( CL_OK != rc )   
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        clLogFileKeyDestroy(pFileKey);
        return rc;
    }

    clLogFileKeyDestroy(pFileKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileHdlrStreamDataGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                           ClStringT              *fileName,
                           ClStringT              *fileLocation,
                           ClUint32T              *pOperatingLvl,
                           ClLogStreamAttrIDLT    *pStreamAttr,
                           ClUint32T              *pNumStreams, 
                           ClLogStreamMapT        **ppStreams)
{
    ClRcT                 rc = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileDataGet(pFileOwnerEoEntry, fileName, 
                                   fileLocation, &pFileOwnerData);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    *pOperatingLvl = pFileOwnerData->pFileHeader->version;
    CL_LOG_DEBUG_TRACE(("Operating level : %d", *pOperatingLvl));
    CL_LOG_DEBUG_TRACE(("pFileOwnerData->hStreamTable: %p",
                (ClPtrT) pFileOwnerData->hStreamTable));
    rc = clLogFileOwnerStreamMapCopy(pFileOwnerData, pNumStreams, ppStreams); 
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogStreamAttributesCopy(&pFileOwnerData->streamAttr, 
                                   pStreamAttr, CL_FALSE);
    if( CL_OK != rc )
    {
        clHeapFree(ppStreams);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("fileName: %s length; %d", fileName->pValue,
                fileName->length));
    pStreamAttr->fileName.length     = fileName->length;
    pStreamAttr->fileLocation.length = fileLocation->length;
    pStreamAttr->fileName.pValue  = (ClCharT*) clHeapCalloc(fileName->length, sizeof(ClCharT));
    if( NULL == pStreamAttr->fileName.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(ppStreams);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           
    pStreamAttr->fileLocation.pValue  = (ClCharT*) clHeapCalloc(fileLocation->length, sizeof(ClCharT));
    if( NULL == pStreamAttr->fileLocation.pValue )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clHeapFree(pStreamAttr->fileName.pValue);
        clHeapFree(ppStreams);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }           

    memcpy(pStreamAttr->fileName.pValue, fileName->pValue,
           pStreamAttr->fileName.length);
    memcpy(pStreamAttr->fileLocation.pValue, fileLocation->pValue,
           pStreamAttr->fileLocation.length);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileRead(ClStringT            *pFileName,
                       ClStringT            *pFileLocation,
                       ClLogFileOwnerDataT  *pFileOwnerData,
                       ClUint32T            numRecords,
                       ClUint32T            fileCount, 
                       ClUint32T            startRecInFunit,
                       ClUint8T             *pLogRecords)
{
    ClRcT          rc            = CL_OK;
    ClCharT        *fileName     = NULL;
    ClUint32T      seekPosition  = 0;
    ClUint32T      maxRecInFunit = 0;
    ClCharT        *pSoftLink    = NULL;
    ClLogFilePtrT  readFp        = NULL;
    ClUint32T      numOfBytes    = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileNameGet(pFileName, pFileLocation, &fileName);
    if( CL_OK != rc )
    {
        return rc;
    }
    pSoftLink = (ClCharT*) clHeapCalloc(strlen(fileName) + 1, sizeof(ClCharT));
    if( NULL == pSoftLink )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return rc;
    }
    maxRecInFunit = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    snprintf(pSoftLink, strlen(fileName)+1, "%s.%d", fileName, fileCount);

    rc = clLogFileOpen_L(pSoftLink, &readFp);
    if( CL_OK != rc )
    {
        clHeapFree(pSoftLink);
        return rc;
    }
    seekPosition = startRecInFunit * pFileOwnerData->streamAttr.recordSize;
    rc = clLogFileSeek(readFp, seekPosition); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(readFp), CL_OK);
        clHeapFree(pSoftLink);
        return rc;
    }
    numOfBytes = numRecords * pFileOwnerData->streamAttr.recordSize;
    rc = clLogFileRead(readFp, pLogRecords, &numOfBytes); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(readFp), CL_OK);
        clHeapFree(pSoftLink);
        return rc;
    }
    clHeapFree(pSoftLink);
    CL_LOG_CLEANUP(clLogFileClose_L(readFp), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    (void)maxRecInFunit;
    return rc;
}

ClRcT
clLogFileOwnerRecordsCopy(ClStringT            *fileName,
                          ClStringT            *fileLocation, 
                          ClLogFileOwnerDataT  *pFileOwnerData,
                          ClUint32T            startRec, 
                          ClUint32T            numRecords,
                          ClUint32T            maxRecCount,
                          ClUint8T             **ppLogRecords)
{
    ClRcT      rc              = CL_OK;
    ClUint32T  maxRecInFunit   = 0;
    ClUint32T  numOfBytes      = 0;
    ClUint32T  startRecInFunit = 0;
    ClUint32T  fileCount       = 0;
    ClUint32T  remRecords      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
     
    maxRecInFunit   = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    startRecInFunit = startRec % maxRecInFunit;

    numOfBytes   = numRecords * pFileOwnerData->streamAttr.recordSize;
    *ppLogRecords = (ClUint8T*) clHeapCalloc(numOfBytes, sizeof(ClUint8T)); 
    if( NULL == *ppLogRecords )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    fileCount = 0; 
    if( CL_LOG_FILE_FULL_ACTION_ROTATE ==
            pFileOwnerData->streamAttr.fileFullAction)
    {
        fileCount  = startRec / maxRecInFunit;
        fileCount %= pFileOwnerData->streamAttr.maxFilesRotated;
    }

    if( (maxRecInFunit - startRecInFunit) > numRecords )
    {
        rc = clLogFileOwnerFileRead(fileName, fileLocation, pFileOwnerData,
                                    numRecords, fileCount, startRecInFunit,
                                    *ppLogRecords);
    }
    else
    {
        switch( pFileOwnerData->streamAttr.fileFullAction )
        {
            case CL_LOG_FILE_FULL_ACTION_HALT:
                {
                    break;
                }
            case CL_LOG_FILE_FULL_ACTION_WRAP:
                {
                    remRecords = ( maxRecInFunit - startRecInFunit );
                    rc = clLogFileOwnerFileRead(fileName, fileLocation, 
                                                pFileOwnerData, 
                                                remRecords, fileCount, 
                                                startRecInFunit, 
                                                *ppLogRecords);
                    if( CL_OK != rc )
                    {
                        break;
                    }
                    numOfBytes = remRecords *
                                 pFileOwnerData->streamAttr.recordSize; 
                    remRecords = numRecords - remRecords;
                    rc = clLogFileOwnerFileRead(fileName, fileLocation, 
                                                pFileOwnerData, remRecords,
                                                fileCount,  1, 
                                                 ((*ppLogRecords) + numOfBytes));
                    break;
                }
            case CL_LOG_FILE_FULL_ACTION_ROTATE:                
                {
                    remRecords = ( maxRecInFunit - startRecInFunit );
                    rc = clLogFileOwnerFileRead(fileName, fileLocation, 
                                                pFileOwnerData, remRecords,
                                                fileCount, startRecInFunit,  
                                                *ppLogRecords);
                    if( CL_OK != rc )
                    {
                        break;
                    }
                    fileCount++;
                    fileCount %= pFileOwnerData->streamAttr.maxFilesRotated;
                    numOfBytes = remRecords *
                                 pFileOwnerData->streamAttr.recordSize; 
                    remRecords = numRecords - remRecords;
                    rc = clLogFileOwnerFileRead(fileName, fileLocation, 
                                                 pFileOwnerData, remRecords, 
                                                 fileCount, 1,  
                                                 ((*ppLogRecords) + numOfBytes));
                    
                    break;
                }
        }
    }
    if( CL_OK != rc )
    {
        clHeapFree(*ppLogRecords);
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileHdlrNonArchiveRecordsGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                  ClStringT              *fileName,
                                  ClStringT              *fileLocation,
                                  ClUint64T              *pStartRec,
                                  ClUint32T              *pVersion,
                                  ClUint32T              *pNumRecords,  
                                  ClUint32T              *pBuffLength, 
                                  ClUint8T               **ppLogRecords)
{
   /*
    * This code has been commended. This code should be modified to 
    * behave with remSize.
    * This was written based on nextRec/lastSaved rec policy
    */
#if 0
    ClRcT                rc              = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T            numReqRecords   = 0;
    ClUint32T            maxRecCount     = 0;
    ClUint32T            maxRecInFunit   = 0;
    ClUint32T            iterationCnt    = 0;
    ClUint32T            startRec        = 0;
    ClUint32T            numRecords      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileDataGet(pFileOwnerEoEntry, fileName, 
                                   fileLocation, &pFileOwnerData);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( 0 == *pVersion )
    {
        *pVersion = pFileOwnerData->pFileHeader->version;
    }
    if( *pVersion != pFileOwnerData->pFileHeader->version )
    {
        CL_LOG_DEBUG_ERROR(("Operating level is not proper"));
        return CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
    }

    maxRecInFunit = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    maxRecCount   = maxRecInFunit;
    if( CL_LOG_FILE_FULL_ACTION_ROTATE == 
            pFileOwnerData->streamAttr.fileFullAction )
    {
        maxRecCount = maxRecInFunit *
            pFileOwnerData->streamAttr.maxFilesRotated;
    }

    if( (maxRecCount < *pNumRecords) || (0 == *pNumRecords) )
    {
        *pNumRecords  = maxRecCount;
    }
    numReqRecords = *pNumRecords;

    iterationCnt = *pStartRec >> 32;
    if( iterationCnt != pFileOwnerData->pFileHeader->iterationCnt )
    {
        startRec   = pFileOwnerData->pFileHeader->lastSavedRec;
        *pStartRec = pFileOwnerData->pFileHeader->iterationCnt;
        *pStartRec = *pStartRec << 32; 
        *pStartRec = *pStartRec | startRec;
    }
    else
    {
        startRec = (ClUint32T) *pStartRec;
    }

    if( CL_TRUE != pFileOwnerData->pFileHeader->overWriteFlag )
    {
        if( startRec >  pFileOwnerData->pFileHeader->nextRec )
        {
            numRecords = maxRecInFunit - (startRec % maxRecInFunit) +
                       (pFileOwnerData->pFileHeader->nextRec % maxRecInFunit);
        }
        else
        {
            numRecords = pFileOwnerData->pFileHeader->nextRec - startRec;
        }
        if( numReqRecords > numRecords)
        {
            numReqRecords = numRecords;
        }
    }

    if( numReqRecords == 0 )
    {
        CL_LOG_DEBUG_ERROR(("No records available"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clLogFileOwnerRecordsCopy(fileName, fileLocation, pFileOwnerData,
                                   startRec, numReqRecords, maxRecCount, 
                                    ppLogRecords);
    if( CL_OK != rc )
    {
        return rc;
    }

    *pBuffLength = numReqRecords * pFileOwnerData->streamAttr.recordSize;
    *pStartRec  += numReqRecords;
    *pNumRecords = numReqRecords;

    CL_LOG_DEBUG_TRACE(("Exit"));
    #endif 
    return CL_OK;
}

ClRcT
clLogFileHdlrArchiveRecordsGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                               ClStringT              *fileName,
                               ClStringT              *fileLocation,
                               ClUint32T              *pVersion,
                               ClUint32T              *pNumRecords,
                               ClUint32T              *pBuffLength, 
                               ClUint8T               **ppLogRecords)
{
   /*
    * This code has been commended. This code should be modified to 
    * behave with remSize.
    * This was written based on nextRec/lastSaved rec policy
    */
    #if 0
    ClRcT                rc              = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T            numReqRecords   = 0;
    ClUint32T            maxRecCount     = 0;
    ClUint32T            maxRecInFunit   = 0;
    ClUint32T            numRecords      = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileDataGet(pFileOwnerEoEntry, fileName, 
                                   fileLocation, &pFileOwnerData);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( 0 == *pVersion )
    {
        *pVersion = pFileOwnerData->pFileHeader->version;
    }
    if( *pVersion != pFileOwnerData->pFileHeader->version )
    {
        CL_LOG_DEBUG_ERROR(("Operating level is not proper"));
        return CL_LOG_RC(CL_ERR_VERSION_MISMATCH);
    }

    maxRecInFunit = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    maxRecCount   = maxRecInFunit;
    if( CL_LOG_FILE_FULL_ACTION_ROTATE == 
            pFileOwnerData->streamAttr.fileFullAction )
    {
        maxRecCount = maxRecInFunit *
            pFileOwnerData->streamAttr.maxFilesRotated;
    }

    if( (maxRecCount < *pNumRecords) || (0 == *pNumRecords) )
    {
        *pNumRecords  = maxRecCount;
    }
    numReqRecords = *pNumRecords;
    if( CL_TRUE != pFileOwnerData->pFileHeader->overWriteFlag )
    {
        if( pFileOwnerData->pFileHeader->lastSavedRec >  
            pFileOwnerData->pFileHeader->nextRec )
        {
            numRecords = maxRecInFunit -
                            (pFileOwnerData->pFileHeader->lastSavedRec %
                             maxRecInFunit) +
                           (pFileOwnerData->pFileHeader->nextRec %
                            maxRecInFunit);
            CL_LOG_DEBUG_TRACE(("Num records: %d  maxRecInFUnit%d nextRec %d", numRecords, maxRecInFunit,
                    pFileOwnerData->pFileHeader->nextRec));
        }
        else
        {
            numRecords = pFileOwnerData->pFileHeader->nextRec -
                            pFileOwnerData->pFileHeader->lastSavedRec;
        }
        if( numReqRecords > numRecords)
        {
            numReqRecords = numRecords;
        }
    }

    if( numReqRecords == 0 )
    {
        CL_LOG_DEBUG_ERROR(("No records available"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }
    rc = clLogFileOwnerRecordsCopy(fileName, fileLocation, pFileOwnerData,
                                   pFileOwnerData->pFileHeader->lastSavedRec, 
                                   numReqRecords, maxRecCount, ppLogRecords);
    if( CL_OK != rc )
    {
        return rc;
    }

    pFileOwnerData->pFileHeader->lastSavedRec += numReqRecords;
    pFileOwnerData->pFileHeader->lastSavedRec %= (2 * maxRecCount);
    pFileOwnerData->pFileHeader->overWriteFlag = CL_FALSE;

    *pBuffLength = numReqRecords * pFileOwnerData->streamAttr.recordSize;
    *pNumRecords = numReqRecords;

    CL_LOG_DEBUG_ERROR(("Exit: pNumRecords: %d", *pNumRecords));
    #endif 
    return CL_OK;
}

ClRcT
VDECL_VER(clLogFileHdlrFileOpen, 4, 0, 0)(ClStringT  *fileName,
                      ClStringT  *fileLocation,
                      ClUint32T  *pOperatingLvl)
{
    ClRcT                  rc                 = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClLogFileOwnerDataT    *pFileOwnerData    = NULL;
    ClLogFileKeyT          *pFileKey          = NULL;
    ClBoolT                fileOwner          = CL_FALSE;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogFileOwnerEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, fileLocation,
                                      &fileOwner);
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        /* 
         * This is not a fileOwner, so returning not exist
         */
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }
    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogFileKeyCreate(fileName, fileLocation, 
                            CL_LOG_STREAM_HDLR_MAX_FILES, &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);
        return rc;
    }
    rc = clCntDataForKeyGet(pFileOwnerEoEntry->hFileTable, 
                            (ClCntKeyHandleT) pFileKey, 
                            (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntDataForKeyGet(): rc[0x %x]", rc));
        clLogFileKeyDestroy(pFileKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);
    }
    *pOperatingLvl = pFileOwnerData->pFileHeader->version;

    clLogFileKeyDestroy(pFileKey);
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogFileHdlrFileMetaDataGet, 4,0,0)(ClStringT            *fileName,
                             ClStringT            *fileLocation,
                             ClUint32T            *pOperatingLvl, 
                             ClLogStreamAttrIDLT  *pStreamAttr,
                             ClUint32T            *pNumStreams, 
                             ClLogStreamMapT      **ppStreams)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClBoolT                 fileOwner           = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, fileLocation, &fileOwner);  
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("FileOwner for fileName: %s", fileName->pValue));

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogFileHdlrStreamDataGet(pFileOwnerEoEntry, fileName, fileLocation,
                                    pOperatingLvl, pStreamAttr, pNumStreams,
                                    ppStreams);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
VDECL_VER(clLogFileHdlrFileRecordsGet, 4, 0, 0)(ClStringT  *fileName,
                                                ClStringT  *fileLocation,
                                                ClBoolT    updateLastSaveRec,
                                                ClUint64T  *pStartRec,
                                                ClUint32T  *pVersion, 
                                                ClUint32T  *pNumRecords,
                                                ClUint32T  *pBuffLength, 
                                                ClUint8T   **ppLogRecords)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClBoolT                 fileOwner           = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( (!pStartRec) || (!pVersion) || (!pNumRecords) || (!pBuffLength) || (!ppLogRecords) )
    {
        clLogError("SVR", "HDLR", "One or more of the passed variable are NULL");
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }
    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, fileLocation, 
                                     &fileOwner);  
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_TRUE == updateLastSaveRec )
    {

        rc = clLogFileHdlrArchiveRecordsGet(pFileOwnerEoEntry, fileName, 
                                            fileLocation, pVersion, 
                                            pNumRecords, pBuffLength, ppLogRecords);
    }
    else
    {
        rc = clLogFileHdlrNonArchiveRecordsGet(pFileOwnerEoEntry, fileName,
                                               fileLocation, pStartRec,
                                               pVersion, pNumRecords,
                                               pBuffLength, ppLogRecords);
    }
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);
        return rc;
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
