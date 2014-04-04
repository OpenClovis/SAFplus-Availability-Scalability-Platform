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
#include<string.h>
#include<netinet/in.h>
#include <syslog.h>
#include <clHandleApi.h>
#include <clIocApi.h>
#include <ipi/clHandleIpi.h>

#include <clLogApi.h>
#include <clLogErrors.h>
#include <clLogSvrCommon.h>
#include <clLogMaster.h>
#include <clLogSvrEo.h>
#include <clLogFileOwner.h>
#include <clLogFileOwnerEo.h>
#include <clLogFileOwnerUtils.h>
#include <clLogFileEvt.h>
#include <clLogOsal.h>
#include <xdrClLogCompDataT.h>

const ClCharT  *gClLogErrorAsciiRecord =
                "Error:this is a ascii file, found non ascii message";

const ClCharT  *CL_LOG_LATEST_STR_LINK = ".latest";                

#define CL_LOG_TIMESTR_LEN 26

static ClRcT
clLogFileOwnerRecordPersist(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                             ClCntNodeHandleT        hFileNode, 
                             ClUint32T               numRecords, 
                             ClUint8T                *pRecords);
static ClRcT
clLogFileRecordsPersist(ClLogFileOwnerEoDataT   *pFileOwnerEoEntry,
                        ClCntHandleT            hFileNode,
                        ClLogFileOwnerDataT     *pFileOwnerData,
                        ClUint32T               numBytes,
                        ClUint32T               numRecords,
                        ClUint8T                *pRecords);

static ClRcT
clLogFileOwnerOldFileDelete(ClCharT    *fileName, 
                            ClUint32T  fileCount);

/*
 * Cleanup function
 *  - Close the filePtr. 
 *  - Unmap the fileHeader.
 *  - Delete the streamTable.
 *  - Free the fileName and fileLocation in streamAttr.
 *  - Free the data
 *  - Free the key.
 */
void
clLogFileTableDeleteCb(ClCntKeyHandleT   key, 
                       ClCntDataHandleT  data)
{
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T             fileMaxCnt     = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pFileOwnerData = (ClLogFileOwnerDataT *) data;
    
    clOsalMutexLock(&pFileOwnerData->fileEntryLock);
    if( NULL != pFileOwnerData->fileUnitPtr )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(pFileOwnerData->fileUnitPtr), CL_OK);
        pFileOwnerData->fileUnitPtr = NULL;
    }
    
    CL_LOG_CLEANUP(clOsalMunmap_L(pFileOwnerData->pFileHeader, 
                               CL_LOG_FILEOWNER_CFG_FILE_SIZE), CL_OK);
    
    pFileOwnerData->pFileHeader = NULL;
    clOsalMutexUnlock(&pFileOwnerData->fileEntryLock);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pFileOwnerData->fileEntryLock),
            CL_OK);
    if( CL_HANDLE_INVALID_VALUE != pFileOwnerData->hStreamTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerData->hStreamTable), CL_OK);
        pFileOwnerData->hStreamTable = CL_HANDLE_INVALID_VALUE;
    }

    fileMaxCnt = pFileOwnerData->streamAttr.maxFilesRotated;
    clHeapFree(pFileOwnerData);

    if( CL_HANDLE_INVALID_VALUE  != key )
    {
        CL_LOG_CLEANUP(clLogFileOwnerSoftLinkDelete((ClLogFileKeyT *) key,
                    fileMaxCnt),
                CL_OK);

        clLogFileKeyDestroy((ClLogFileKeyT *) key);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

void
clLogFileOwnerStreamDataDeleteCb(ClCntKeyHandleT   key,
                                 ClCntDataHandleT  data)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClLogFileOwnerStreamDataT  *pData;
    
    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pData = (ClLogFileOwnerStreamDataT *) data;

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return ;
    }
    CL_LOG_DEBUG_TRACE(("hFileOwner: %#llX", pData->hFileOwner));
    if( CL_HANDLE_INVALID_VALUE != pData->hFileOwner )
    {
        clLogHandlerDeregister(pData->hFileOwner);
        CL_LOG_CLEANUP(clHandleDestroy(pFileOwnerEoEntry->hStreamDB,
                    pData->hFileOwner), CL_OK);
        pData->hFileOwner = CL_HANDLE_INVALID_VALUE;
    }
    clHeapFree(pData);
    clHeapFree((ClLogStreamKeyT *) key);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

/*
 * Function - clLogFileOwnerOwnerVerify()
 *  - Extract the address info from fileLocation.
 *  - Chk localAddress as fileOwner 
 *  - Return CL_TRUE if owner Else CL_FALSE.
 */
ClRcT
clLogFileOwnerLocationVerify(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                             ClStringT              *pFileLocation,
                             ClBoolT                *pFileOwner)
{
    ClRcT                  rc                          = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry          = NULL;
    ClIocNodeAddressT      localAddr                   = CL_IOC_RESERVED_ADDRESS;
    ClCharT                nodeStr[CL_MAX_NAME_LENGTH] = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    *pFileOwner = CL_FALSE;
    sscanf(pFileLocation->pValue, "%[^:]", nodeStr);
    CL_LOG_DEBUG_TRACE(("Nodestr extracted from fileLocation: %s", nodeStr));

    if( ('*' == nodeStr[0]) && ('\0' == nodeStr[1]) )
    {
        localAddr = clIocLocalAddressGet();
        if( localAddr == pSvrCommonEoEntry->masterAddr )
        {
            *pFileOwner = CL_TRUE;
        }
    }
    else if( pFileOwnerEoEntry->nodeName.length == strlen(nodeStr)
             &&
             !(strncmp((const ClCharT *)pFileOwnerEoEntry->nodeName.value, nodeStr,
                       pFileOwnerEoEntry->nodeName.length)) )
    {
        *pFileOwner = CL_TRUE;
    }

    CL_LOG_DEBUG_TRACE(("Exit: %d", *pFileOwner));
    return rc;
}


ClRcT
clLogFileOwnerCfgFileMapNPopulate(ClHandleT            fd,
                                   ClLogStreamAttrIDLT  *pStreamAttr, 
                                   ClLogFileHeaderT     **ppHeader)
{
    ClRcT      rc      = CL_OK;
    ClUint16T  num     = 1;
    ClCharT    *pTemp  = NULL;
    ClUint16T  endMark = 0;
    ClUint8T   endian  = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalFtruncate_L(fd, CL_LOG_FILEOWNER_CFG_FILE_SIZE);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clOsalMmap_L(0, CL_LOG_FILEOWNER_CFG_FILE_SIZE, CL_LOG_MMAP_PROT_FLAGS, 
                      CL_LOG_MMAP_FLAGS, fd, 0, (void **) ppHeader);
    if( CL_OK != rc )
    {
       CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
       return rc;
    }

    (*ppHeader)->nextRec        = 0;
    (*ppHeader)->remSize        = 0;
    (*ppHeader)->nOverwritten   = 0;
    (*ppHeader)->overWriteFlag  = CL_FALSE;
    (*ppHeader)->deleteCnt      = 0;
    (*ppHeader)->currFileUnitCnt  = 0;
    (*ppHeader)->maxHdrSize     = CL_LOG_FILEOWNER_CFG_FILE_SIZE;
//    (*ppHeader)->remHdrSize     = (*ppHeader)->maxHdrSize - sizeof(ClLogFileHeaderT);
    (*ppHeader)->numPages       = CL_LOG_FILE_HDR_INIT_PAGES;
    (*ppHeader)->fileType       = CL_LOG_FILE_TYPE_INVALID;
    (*ppHeader)->streamOffSet   = CL_LOG_FILEOWNER_STREAM_HEAD;
    (*ppHeader)->fileOffSet     = CL_LOG_FILEOWNER_FILE_HEAD;
    (*ppHeader)->compOffSet     = CL_LOG_FILEOWNER_COMP_HEAD;
    (*ppHeader)->nextEntry      = CL_LOG_FILEOWNER_NEXT_ENTRY;
    
    pTemp = (ClCharT *) (*ppHeader);

    memcpy( (pTemp + 256), CL_LOG_DEFAULT_CFG_FILE_STRING,
            strlen(CL_LOG_DEFAULT_CFG_FILE_STRING) + 1);

    endian = *(ClCharT *) &num;
    memcpy((pTemp + 287),  (ClCharT *) &endian, sizeof(endian));
    //(*ppHeader)->fileUnitSize    = pStreamAttr->fileUnitSize;
    memcpy((pTemp + 288), &pStreamAttr->fileUnitSize, 
            sizeof(pStreamAttr->fileUnitSize));
    //(*ppHeader)->recordSize      = pStreamAttr->recordSize;
    memcpy((pTemp + 292), &pStreamAttr->recordSize, 
              sizeof(pStreamAttr->recordSize));
    //(*ppHeader)->fileFullAction  = pStreamAttr->fileFullAction;
    memcpy((pTemp + 296), &pStreamAttr->fileFullAction, 
              sizeof(pStreamAttr->fileFullAction));
    //(*ppHeader)->maxFilesRotated = pStreamAttr->maxFilesRotated;
    memcpy((pTemp + 300), &pStreamAttr->maxFilesRotated, 
              sizeof(pStreamAttr->maxFilesRotated));
    //(*ppHeader)->waterMark.lowLimit = pStreamAttr->waterMark.lowLimit;
    memcpy((pTemp + 304), &pStreamAttr->waterMark.lowLimit, 
              sizeof(pStreamAttr->waterMark.lowLimit));
    //(*ppHeader)->waterMark.lowLimit = pStreamAttr->waterMark.lowLimit;
    memcpy((pTemp + 312), &pStreamAttr->waterMark.highLimit, 
              sizeof(pStreamAttr->waterMark.highLimit));
   // CL_LOG_FILEOWNER_STREAM_HEAD = 320 
    memcpy((pTemp + CL_LOG_FILEOWNER_STREAM_HEAD), (ClCharT *) &endMark, 
            sizeof(endMark));
   // CL_LOG_FILEOWNER_STREAM_HEAD = 322 
    memcpy((pTemp + CL_LOG_FILEOWNER_FILE_HEAD), (ClCharT *) &endMark, 
            sizeof(endMark));
   // CL_LOG_FILEOWNER_STREAM_HEAD = 324 
    memcpy((pTemp + CL_LOG_FILEOWNER_COMP_HEAD), (ClCharT *) &endMark, 
            sizeof(endMark));
    (*ppHeader)->remHdrSize = (*ppHeader)->maxHdrSize - CL_LOG_FILEOWNER_CFG_HDR_SIZE; 

    clOsalMsync(*ppHeader, CL_LOG_FILEOWNER_CFG_FILE_SIZE, MS_SYNC);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCfgFileCreateNPopulate(ClCharT              *pTimeStr, 
                                      ClCharT              *fileName,
                                      ClLogStreamAttrIDLT  *pStreamAttr, 
                                      ClLogFileHeaderT     **ppFileHeader)
{
    ClRcT          rc             = CL_OK;
    ClCharT        *pSoftLinkName = NULL;
    ClHandleT      fd             = CL_HANDLE_INVALID_VALUE;
    ClLogFilePtrT  fp             = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pSoftLinkName = (ClCharT*) clHeapCalloc(strlen(fileName) + 5, sizeof(ClCharT));
    if( NULL == pSoftLinkName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(); rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    snprintf(pSoftLinkName, strlen(fileName) + 5, "%s.cfg", fileName);

    do
    {
        rc = clLogFileOwnerSoftLinkCreate(fileName, pSoftLinkName, pTimeStr,
                                           CL_LOG_FILE_CNT_MAX);
        if( CL_OK == rc )
        {
            break;
        }
        clLogFileOwnerTimeGet(pTimeStr, 1);
    }while( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST));
    if( CL_OK != rc )
    {
        clHeapFree(pSoftLinkName);
        return rc;
    }

    rc = clLogFileOpen_L(pSoftLinkName, &fp);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    rc = clLogFileNo(fp, &fd);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }

    rc = clLogFileOwnerCfgFileMapNPopulate(fd, pStreamAttr, ppFileHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }

    CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
    clHeapFree(pSoftLinkName);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCfgFileDelete(ClCharT           *fileName,
                             ClLogFileHeaderT  *pHeader) 
{
    ClRcT    rc             = CL_OK;
    ClCharT  *pSoftLinkName = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pSoftLinkName = (ClCharT*) clHeapCalloc(strlen(fileName) + 5, sizeof(ClCharT));
    if( NULL == pSoftLinkName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(); rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    snprintf(pSoftLinkName, strlen(fileName) + 5, "%s.cfg", fileName);

    CL_LOG_CLEANUP(clOsalMunmap_L(pHeader, CL_LOG_FILEOWNER_CFG_FILE_SIZE),
                   CL_OK);
    CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
    clHeapFree(pSoftLinkName);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT
clLogLatestSoftlinkUpdate(ClCharT  *pSoftLinkName,
                          ClCharT  *fileName)
{
   ClRcT    softLinkLen = strlen(fileName) + strlen(CL_LOG_LATEST_STR_LINK) + 1;
   ClCharT  latestLink[softLinkLen];

   snprintf(latestLink, softLinkLen, "%s%s", fileName, CL_LOG_LATEST_STR_LINK);

   clLogFileUnlink(latestLink);
   clLogSymLink(pSoftLinkName, latestLink);

   return CL_OK;
}

ClRcT
logFileOwnerLogFileCreateNPopulate(ClCharT        *pTimeStr, 
                                   ClCharT        *fileName,
                                   ClUint32T      recordSize, 
                                   ClUint32T      fileCount, 
                                   ClLogFilePtrT  *pFp,
                                   ClBoolT restartFlag)
{
    ClRcT     rc             = CL_OK;
    ClCharT   *pSoftLinkName = NULL;
    ClCharT   *pRecord       = NULL;
    ClUint32T len            = 0;
    ClUint32T  fileCountLen  = 0;
    ClUint32T  softLinkLen   = 0;
    ClUint8T   tempVar       = 0;

    CL_LOG_DEBUG_TRACE(("Enter: currFileUnitCnt: %u", fileCount));
    
    fileCountLen = snprintf((ClCharT*)&tempVar, 1, "%u", fileCount);

    softLinkLen = strlen(fileName) + 1 + fileCountLen + 1;
    
    pSoftLinkName = (ClCharT*) clHeapAllocate(softLinkLen);
    if( NULL == pSoftLinkName )
    {
        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(); rc[0x %x]", rc));
        return rc;
    }
    snprintf(pSoftLinkName, softLinkLen, "%s.%u", fileName, fileCount);

    do
    {
        rc = clLogFileOwnerSoftLinkCreate(fileName, pSoftLinkName, pTimeStr,
                                           fileCount);
        if( CL_OK == rc )
        {
            break;
        }
        clLogFileOwnerTimeGet(pTimeStr, 1);
    }while( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST));
    if( CL_OK != rc )
    {
        clHeapFree(pSoftLinkName);
        return rc;
    }
    rc = clLogFileOpen_L(pSoftLinkName, pFp);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    
    /* fill the default record */
    pRecord = (ClCharT*) clHeapCalloc(recordSize, sizeof(ClCharT));
    if( NULL == pRecord )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(*pFp), CL_OK);
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    if(!restartFlag)
    {
        len = strlen(CL_LOG_DEFAULT_FILE_STRING);
        snprintf(pRecord, recordSize, "%s", CL_LOG_DEFAULT_FILE_STRING);
    }
    else
    {
        len = strlen(CL_LOG_DEFAULT_FILE_STRING_RESTART);
        snprintf(pRecord, recordSize, "%s", CL_LOG_DEFAULT_FILE_STRING_RESTART);
    }
    /*Last 1 Bye is reserved for record write in-progress indicator */
    memset(pRecord + len, ' ', recordSize - len - 2); 
    pRecord[recordSize - 2]='\n'; 
    rc = clLogFileWrite(*pFp, pRecord, recordSize-1);
//    fprintf(*pFp, "%.*s\n", recordSize, CL_LOG_DEFAULT_FILE_STRING); 
    if( CL_OK != rc )
    {
        clHeapFree(pRecord);
        CL_LOG_CLEANUP(clLogFileClose_L(*pFp), CL_OK);
        CL_LOG_CLEANUP(clLogFileUnlink(pSoftLinkName), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    /* Just ignoring the error, it will not harm anything */
    // clLogFileSeek(*pFp, recordSize); 
    /*
     * Modify the latest link point to the current file unit 
     */
    clLogLatestSoftlinkUpdate(pSoftLinkName, fileName);
    clHeapFree(pRecord);
    clHeapFree(pSoftLinkName);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerLogFileCreateNPopulate(ClCharT        *pTimeStr, 
                                     ClCharT        *fileName,
                                     ClUint32T      recordSize, 
                                     ClUint32T      fileCount, 
                                     ClLogFilePtrT  *pFp)
{
    return logFileOwnerLogFileCreateNPopulate(pTimeStr, fileName, recordSize,
                                              fileCount, pFp, CL_FALSE);
}

ClRcT
clLogFileOwnerFileOpenNPopulate(ClCharT               *fileName,
                                 ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT          rc             = CL_OK;
    ClCharT        *pSoftLinkName = NULL;
    ClLogFilePtrT  fp             = NULL;
    ClHandleT      fd             = CL_HANDLE_INVALID_VALUE;
    ClUint32T      nextRec        = 0;
    ClUint32T      maxRecInFunit  = 0;
    ClUint32T      lastFileUnitCnt = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pSoftLinkName = (ClCharT*)clHeapCalloc(strlen(fileName) + 5, sizeof(ClCharT));
    if( NULL == pSoftLinkName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(); rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    snprintf(pSoftLinkName, strlen(fileName) + 5, "%s.cfg", fileName);
    rc = clLogFileOpen_L(pSoftLinkName, &fp);
    if( CL_OK != rc ) 
    {
        clHeapFree(pSoftLinkName);
        return rc;
    }
    rc = clLogFileNo(fp, &fd);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }

    rc = clOsalMmap_L(0, CL_LOG_FILEOWNER_CFG_FILE_SIZE, CL_LOG_MMAP_PROT_FLAGS, 
                      CL_LOG_MMAP_FLAGS, fd, 0, 
                      (void **) &pFileOwnerData->pFileHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        clHeapFree(pSoftLinkName);
        CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
        return rc;
    }
    CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);

    memset(pSoftLinkName, '\0', strlen(fileName) + 5);

    if( CL_LOG_FILE_FULL_ACTION_ROTATE ==
            pFileOwnerData->streamAttr.fileFullAction )
    {
        if(!(lastFileUnitCnt = pFileOwnerData->pFileHeader->currFileUnitCnt))
        {
            lastFileUnitCnt = pFileOwnerData->streamAttr.maxFilesRotated;
        }
        if(lastFileUnitCnt)
            --lastFileUnitCnt;
    }

    snprintf(pSoftLinkName, strlen(fileName) + 5, "%s.%d", fileName, lastFileUnitCnt);
    rc = clLogFileOpen_L(pSoftLinkName, &pFileOwnerData->fileUnitPtr);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMunmap_L(pFileOwnerData->pFileHeader, 
                                      CL_LOG_FILEOWNER_CFG_FILE_SIZE), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    maxRecInFunit = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    nextRec = pFileOwnerData->streamAttr.fileUnitSize - pFileOwnerData->pFileHeader->remSize;
    rc = clLogFileSeek(pFileOwnerData->fileUnitPtr, nextRec);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(pFileOwnerData->fileUnitPtr), CL_OK);
        pFileOwnerData->fileUnitPtr = NULL;
        CL_LOG_CLEANUP(clOsalMunmap_L(pFileOwnerData->pFileHeader, 
                                      CL_LOG_FILEOWNER_CFG_FILE_SIZE), CL_OK);
        clHeapFree(pSoftLinkName);
        return rc;
    }
    clHeapFree(pSoftLinkName);

    CL_LOG_DEBUG_TRACE(("Exit"));
    (void)maxRecInFunit;
    return rc;
}

ClRcT
clLogFileOwnerFileCreateNPopulate(ClCharT               *fileName,
                                  ClLogStreamAttrIDLT   *pStreamAttr,
                                  ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT      rc                          = CL_OK;
    ClCharT    timeStr[CL_MAX_NAME_LENGTH] = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogFileOwnerTimeGet(timeStr, 0);
    if( CL_OK != rc )
    {
        return rc;            
    }
    rc = clLogFileOwnerCfgFileCreateNPopulate(timeStr, fileName, pStreamAttr, 
                                              &pFileOwnerData->pFileHeader);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileOwnerLogFileCreateNPopulate(timeStr, fileName, 
                                              pStreamAttr->recordSize,
                                              pFileOwnerData->pFileHeader->currFileUnitCnt, 
                                              &pFileOwnerData->fileUnitPtr);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileOwnerCfgFileDelete(fileName,
                                     pFileOwnerData->pFileHeader), CL_OK); 
        return rc;
    }
    pFileOwnerData->pFileHeader->remSize = pFileOwnerData->streamAttr.fileUnitSize - 
                                           pFileOwnerData->streamAttr.recordSize;
    pFileOwnerData->pFileHeader->nextRec = 1;                                           
    if( CL_LOG_FILE_FULL_ACTION_ROTATE == pStreamAttr->fileFullAction )
    {
        pFileOwnerData->pFileHeader->currFileUnitCnt++;
        pFileOwnerData->pFileHeader->currFileUnitCnt %= pStreamAttr->maxFilesRotated;
    }
    /* Publish an Event for File Creation Event*/
    rc = clLogFileCreationEvent(&pStreamAttr->fileName, &pStreamAttr->fileLocation);

    clOsalMsync(pFileOwnerData->pFileHeader, CL_LOG_FILEOWNER_CFG_FILE_SIZE, MS_SYNC);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerNewFunitRotate(ClCharT *fileName,
                             ClLogStreamAttrIDLT  *pStreamAttr,
                             ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT                  rc                          = CL_OK;
    ClCharT                timeStr[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *pSoftLinkName = NULL;
    ClLogFilePtrT          fp = NULL;
    ClHandleT fd = 0;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!fileName || !pFileOwnerData || !pStreamAttr)
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);

    pSoftLinkName = (ClCharT*) clHeapCalloc(strlen(fileName) + 5, sizeof(ClCharT));
    if( NULL == pSoftLinkName )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(); rc[0x %x]", rc));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    snprintf(pSoftLinkName, strlen(fileName) + 5, "%s.cfg", fileName);
    rc = clLogFileOpen_L(pSoftLinkName, &fp);
    if( CL_OK != rc ) 
    {
        clHeapFree(pSoftLinkName);
        return rc;
    }

    clHeapFree(pSoftLinkName);
    rc = clLogFileNo(fp, &fd);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        return rc;
    }
    
    rc = clOsalMmap_L(0, CL_LOG_FILEOWNER_CFG_FILE_SIZE, CL_LOG_MMAP_PROT_FLAGS, 
                      CL_LOG_MMAP_FLAGS, fd, 0, 
                      (void **) &pFileOwnerData->pFileHeader);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);
        CL_LOG_DEBUG_ERROR(("clOsalMmap(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_CLEANUP(clLogFileClose_L(fp), CL_OK);

    if( pFileOwnerData->pFileHeader->deleteCnt == 
        pFileOwnerData->pFileHeader->currFileUnitCnt )
    {
        rc = clLogFileOwnerOldFileDelete(fileName, 
                pFileOwnerData->pFileHeader->deleteCnt);
        if( CL_OK != rc )
        {
            return rc;
        }
        CL_LOG_DEBUG_TRACE(("Deleting this file: %s", fileName));
        /* 
         * Move the lastSavedRec into nextFile.
         * NextRec wont be in that range.
         */
        pFileOwnerData->pFileHeader->deleteCnt++;
        if(pFileOwnerData->streamAttr.maxFilesRotated > 0)
            pFileOwnerData->pFileHeader->deleteCnt %=
                pFileOwnerData->streamAttr.maxFilesRotated;
        else
            pFileOwnerData->pFileHeader->deleteCnt = 0;
    }

    rc = clLogFileClosureEvent(&pStreamAttr->fileName, &pStreamAttr->fileLocation);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clLogFileOwnerTimeGet(timeStr, 0);
    if( CL_OK != rc )
    {
        return rc;            
    }
    rc = logFileOwnerLogFileCreateNPopulate(timeStr, fileName,
                                            pFileOwnerData->streamAttr.recordSize,
                                            pFileOwnerData->pFileHeader->currFileUnitCnt,
                                            &pFileOwnerData->fileUnitPtr,
                                            CL_TRUE);
    if( CL_OK != rc )
    {
        return rc;
    }

    /* New file unit has been created, so this file will have max-1 records 
     * resetting the rem size to be max -1 records. 
     */
    pFileOwnerData->pFileHeader->remSize = pFileOwnerData->streamAttr.fileUnitSize - 
                                            pFileOwnerData->streamAttr.recordSize;
    pFileOwnerData->pFileHeader->nextRec = 1;                                            
    if(pFileOwnerData->streamAttr.maxFilesRotated > 0)
    {
        pFileOwnerData->pFileHeader->currFileUnitCnt++;
        pFileOwnerData->pFileHeader->currFileUnitCnt %=
            pFileOwnerData->streamAttr.maxFilesRotated;
    }

    clOsalMsync(pFileOwnerData->pFileHeader, CL_LOG_FILEOWNER_CFG_FILE_SIZE, MS_SYNC);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryInit(ClLogFileOwnerDataT  *pFileOwnerData,
                            ClLogStreamAttrIDLT  *pStreamAttr,
                            ClBoolT              restart,
                            ClUint32T            operatingLvl)
{
    ClRcT    rc        = CL_OK;
    ClCharT  *fileName = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clLogFileNameForStreamAttrGet(pStreamAttr, &fileName);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_TRUE == restart )
    {
        rc = clLogFileOwnerFileOpenNPopulate(fileName, 
                                             pFileOwnerData);
        if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
        {
            /*
            rc = clLogFileOwnerFileCreateNPopulate(fileName, pStreamAttr,
                                                   pFileOwnerData);
            */
            rc = clLogFileOwnerNewFunitRotate(fileName, pStreamAttr, pFileOwnerData);
        }
    }
    else
    {
        rc = clLogFileOwnerFileCreateNPopulate(fileName, pStreamAttr,
                                                pFileOwnerData);
        if( CL_OK != rc )
        {
            clHeapFree(fileName);
            return rc;
        }
        pFileOwnerData->pFileHeader->version = operatingLvl;
        CL_LOG_OPERATING_LEVEL_INC(pFileOwnerData->pFileHeader);
    }
    clHeapFree(fileName);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryFree(ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_CLEANUP(clOsalMunmap_L(pFileOwnerData->pFileHeader, 
                                  CL_LOG_FILEOWNER_CFG_FILE_SIZE), CL_OK);
    CL_LOG_CLEANUP(clLogFileClose_L(pFileOwnerData->fileUnitPtr), CL_OK);
    
    pFileOwnerData->fileUnitPtr = NULL;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamAttrModifyNCopy(ClLogStreamAttrIDLT  *pStreamAttr, 
                                    ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( 0 == pStreamAttr->fileUnitSize )
    {
        pStreamAttr->fileUnitSize = (~pStreamAttr->fileUnitSize) / 2;
        if( CL_LOG_FILE_FULL_ACTION_ROTATE == pStreamAttr->fileFullAction )
        {
            pStreamAttr->maxFilesRotated = 1;
        }
    }
    if( CL_LOG_FILE_FULL_ACTION_ROTATE == pStreamAttr->fileFullAction )
    {
        if( 0 == pStreamAttr->maxFilesRotated )
        {
            pStreamAttr->maxFilesRotated = ((~pStreamAttr->maxFilesRotated) /
                                            (pStreamAttr->fileUnitSize * 2));
        }
    }
    rc = clLogStreamAttributesCopy(pStreamAttr, &pFileOwnerData->streamAttr,
                                   CL_FALSE);
    if( CL_OK != rc )
    {
        return rc;
    }
    pFileOwnerData->streamAttr.fileName.length     = 0;
    pFileOwnerData->streamAttr.fileName.pValue     = NULL;
    pFileOwnerData->streamAttr.fileLocation.length = 0;
    pFileOwnerData->streamAttr.fileLocation.pValue = NULL;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryAdd(ClCntHandleT         hFileTable,
                            ClLogFileKeyT        *pFileKey, 
                            ClLogStreamAttrIDLT  *pStreamAttr, 
                            ClBoolT              restart, 
                            ClCntNodeHandleT     *phFileNode) 
{
    ClRcT                 rc               = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T            maxRecInFunit     = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    pFileOwnerData = (ClLogFileOwnerDataT*) clHeapCalloc(1, sizeof(ClLogFileOwnerDataT));
    if( NULL == pFileOwnerData )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc(): rc[0x %x]", rc));
        return rc;
    }
    rc = clCntHashtblCreate(CL_LOG_STREAM_HDLR_MAX_FILES,
                            clLogStreamKeyCompare, clLogStreamHashFn,
                            clLogFileOwnerStreamDataDeleteCb,
                            clLogFileOwnerStreamDataDeleteCb, CL_CNT_UNIQUE_KEY,
                            &(pFileOwnerData->hStreamTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashtbleCreate(): rc[0x %x]\n", rc));
        clHeapFree(pFileOwnerData);
        return rc;
    }
    CL_LOG_DEBUG_TRACE(("Created streamTable  %p", 
			(ClPtrT) pFileOwnerData->hStreamTable));
    rc = clOsalMutexInit_L(&(pFileOwnerData->fileEntryLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexInitEx_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerData->hStreamTable), CL_OK);
        clHeapFree(pFileOwnerData);
        return rc;
    }

    rc = clLogFileOwnerStreamAttrModifyNCopy(pStreamAttr, pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pFileOwnerData->fileEntryLock),
                CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerData->hStreamTable), CL_OK);
        clHeapFree(pFileOwnerData);
        return rc;
    }

    maxRecInFunit = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    /*FIXME this should be calculated based on passed streamAttr*/
    pStreamAttr->waterMark.highLimit = (90 * maxRecInFunit) / 100;
    pFileOwnerData->streamAttr.waterMark.highLimit =
        pStreamAttr->waterMark.highLimit;
    
    rc = clLogFileOwnerFileEntryInit(pFileOwnerData, pStreamAttr,
                                     restart, 0);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pFileOwnerData->fileEntryLock),
                CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerData->hStreamTable), CL_OK);
        clHeapFree(pFileOwnerData);
        return rc;
    }

    rc = clCntNodeAddAndNodeGet(hFileTable, (ClCntKeyHandleT) pFileKey,
                                (ClCntDataHandleT) pFileOwnerData, NULL,
                                 phFileNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeAddAndNodeGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clLogFileOwnerFileEntryFree(pFileOwnerData), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pFileOwnerData->fileEntryLock),
                CL_OK);
        CL_LOG_CLEANUP(clCntDelete(pFileOwnerData->hStreamTable), CL_OK);
        clHeapFree(pFileOwnerData);
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                           ClLogStreamAttrIDLT    *pStreamAttr, 
                           ClBoolT                restart, 
                           ClCntNodeHandleT       *phFileNode, 
                           ClBoolT                *pEntryAdd)
{
    ClRcT                rc              = CL_OK;
    ClLogFileKeyT        *pFileKey       = NULL;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T            numStreams      = 0;
    ClUint32T            operatingLvl    = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pEntryAdd = CL_FALSE;
    rc = clLogFileKeyCreate(&(pStreamAttr->fileName),
                            &(pStreamAttr->fileLocation),
                            CL_LOG_STREAM_HDLR_MAX_FILES, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeFind(pFileOwnerEoEntry->hFileTable, 
                       (ClCntKeyHandleT) pFileKey, phFileNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        rc = clLogFileOwnerFileEntryAdd(pFileOwnerEoEntry->hFileTable, 
                                         pFileKey, pStreamAttr, restart,
                                         phFileNode);
        if( CL_OK != rc )
        {
            clLogFileKeyDestroy(pFileKey);
            return rc;
        }

        *pEntryAdd = CL_TRUE;
        return rc;
    }
    else if( CL_OK == rc )
    {
        rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, 
                                  *phFileNode,
                                  (ClCntDataHandleT *) &pFileOwnerData);
        if( CL_OK != rc )   
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
            clLogFileKeyDestroy(pFileKey);
            return rc;
        }

        rc = clCntSizeGet(pFileOwnerData->hStreamTable, &numStreams); 
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]", rc));
            clLogFileKeyDestroy(pFileKey);
            return rc;
        }
        if( 0 == numStreams )
        {
            /*
             * StreamOpen - Close again Open, Filentry still be there,
             * but it has to be reinitialize data with new attributes
             * change the operating level also.
             * This we are doing to provide service for archiver.
             */
            rc = clOsalMutexLock_L(&pFileOwnerData->fileEntryLock);
            if( CL_OK != rc )
            {
                clLogFileKeyDestroy(pFileKey);
                return rc;
            }
            CL_LOG_CLEANUP(clLogFileClose_L(pFileOwnerData->fileUnitPtr),
                           CL_OK);
            pFileOwnerData->fileUnitPtr = NULL;
            CL_LOG_CLEANUP(clOsalMsync_L(pFileOwnerData->pFileHeader, 
                               CL_LOG_FILEOWNER_CFG_FILE_SIZE, MS_SYNC),
                           CL_OK);
            operatingLvl = pFileOwnerData->pFileHeader->version;
            CL_LOG_CLEANUP(clOsalMunmap_L(pFileOwnerData->pFileHeader, 
                               CL_LOG_FILEOWNER_CFG_FILE_SIZE), CL_OK);
            CL_LOG_CLEANUP(clLogFileOwnerSoftLinkDelete(pFileKey,
                        pFileOwnerData->streamAttr.maxFilesRotated),
                           CL_OK);
            rc = clLogFileOwnerStreamAttrModifyNCopy(pStreamAttr, 
                                                     pFileOwnerData);
            if( CL_OK != rc )
            {
                clLogFileKeyDestroy(pFileKey);
                CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock),
                                                   CL_OK);
                return rc;
            }
            rc = clLogFileOwnerFileEntryInit(pFileOwnerData, pStreamAttr, 
                                             CL_FALSE, operatingLvl);
            if( CL_OK != rc )
            {
                clLogFileKeyDestroy(pFileKey);
                CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock),
                                                   CL_OK);
                return rc;
            }
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock),
                                               CL_OK);
        }
    }
    clLogFileKeyDestroy(pFileKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerHdlCreate(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                        ClHandleT              hFileOwner, 
                        ClCntNodeHandleT       hFileNode,
                        ClCntNodeHandleT       hStreamNode)
{
    ClRcT                    rc     = CL_OK;
    ClLogFileOwnerHdlrDataT  *pData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clHandleCreateSpecifiedHandle(pFileOwnerEoEntry->hStreamDB, 
                                       sizeof(ClLogFileOwnerHdlrDataT), 
                                       hFileOwner);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCreateSpecifiedHandle(): rc[0x %x]",
                                                        rc));
        return rc;
    }

    rc = clHandleCheckout(pFileOwnerEoEntry->hStreamDB, hFileOwner, 
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDestroy(pFileOwnerEoEntry->hStreamDB,
                                       hFileOwner), CL_OK);
        return rc;
    }

    //*pData = hFileNode;
    pData->hFileNode   = hFileNode;
    pData->hStreamNode = hStreamNode;
    pData->activeCnt   = 0;

    rc = clHandleCheckin(pFileOwnerEoEntry->hStreamDB, hFileOwner);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDestroy(pFileOwnerEoEntry->hStreamDB,
                                       hFileOwner), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamEntryGet(ClLogFileOwnerDataT  *pFileOwnerData, 
                              SaNameT               *pStreamName, 
                              SaNameT               *pStreamScopeNode, 
                              ClCntNodeHandleT      *phStreamNode,
                              ClBoolT               *pEntryAdd)
{
    ClRcT                       rc           = CL_OK;
    ClLogStreamKeyT             *pStreamKey  = NULL;
    ClLogFileOwnerStreamDataT  *pStreamData = NULL; 

    CL_LOG_DEBUG_TRACE(("Enter"));

    *pEntryAdd = CL_FALSE;
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              CL_LOG_STREAM_HDLR_MAX_STREAMS,
                              &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeFind(pFileOwnerData->hStreamTable, 
                       (ClCntKeyHandleT) pStreamKey, phStreamNode);
    if( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        pStreamData = (ClLogFileOwnerStreamDataT*) clHeapCalloc(1, sizeof(ClLogFileOwnerStreamDataT)); 
        if( NULL == pStreamData )
        {
            CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }
        pStreamData->status = CL_LOG_FILEOWNER_STATUS_WIP;
        rc = clCntNodeAddAndNodeGet(pFileOwnerData->hStreamTable, 
                                    (ClCntKeyHandleT) pStreamKey, 
                                    (ClCntDataHandleT) pStreamData, 
                                    NULL, phStreamNode);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeAddAndGet(): rc[0x %x]", rc));
            clHeapFree(pStreamData);
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }

        *pEntryAdd = CL_TRUE;
        return rc;
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCfgFileStreamInfoUpdate(ClLogFileOwnerDataT  *pFileOwnerData, 
                                       SaNameT               *pStreamName,
                                       ClUint16T             streamId)
{
    ClRcT             rc           = CL_OK;
    ClLogFileHeaderT  *pHeader     = pFileOwnerData->pFileHeader;
    ClUint8T          *pStart      = (ClUint8T *) pHeader;
    ClUint16T         streamOffSet = 0;
    ClUint8T          *pAddr       = NULL;
    ClUint16T         endMark      = 0;
    ClUint16T         entryLen     = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    entryLen = sizeof(endMark) + sizeof(pStreamName->length) + 
             pStreamName->length + sizeof(streamId); 
    if( pHeader->remHdrSize >  entryLen )
    {
	    pAddr  = pStart + pHeader->streamOffSet;

	    memcpy((ClCharT *)&streamOffSet, pAddr, sizeof(streamOffSet));
	    while(streamOffSet != 0 )
	    {
		    pAddr = pStart + streamOffSet;
		    memcpy((ClCharT *)&streamOffSet, pAddr, sizeof(streamOffSet));
	    }
	    memcpy(pAddr, (ClCharT *) &pHeader->nextEntry, sizeof(streamOffSet));

	    pAddr = pStart + pHeader->nextEntry;
	    memcpy(pAddr, (ClCharT *) &endMark, sizeof(endMark));
	    pAddr += sizeof(endMark);

	    memcpy(pAddr, (ClCharT *) &pStreamName->length, sizeof(pStreamName->length));
	    pAddr += sizeof(pStreamName->length);

	    memcpy(pAddr, pStreamName->value, pStreamName->length);
	    pAddr += pStreamName->length;

	    memcpy(pAddr, (ClCharT *) &streamId, sizeof(streamId));
	    pHeader->nextEntry += entryLen; 
	    pHeader->remHdrSize -= entryLen;
    }
    else
    {
	   /* The initial pages are alreay over, so keep allocation one page when it hits this area */
        clLogWarning("FOW", "STR", "Initial pages are alreay over, allocation extra page");
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCfgFileFileInfoUpdate(ClLogFileOwnerDataT  *pFileOwnerData, 
                                     ClStringT             *pFileName,
                                     ClStringT             *pFileLocation)
{
    ClRcT             rc           = CL_OK;
    ClLogFileHeaderT  *pHeader     = pFileOwnerData->pFileHeader;
    ClUint8T          *pStart      = (ClUint8T *) pHeader;
    ClUint16T         fileOffSet   = 0;
    ClUint8T          *pAddr       = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pAddr = pStart + pHeader->fileOffSet;
    
    memcpy((ClCharT *)&fileOffSet, pAddr, sizeof(fileOffSet));
    while(fileOffSet != 0 )
    {
        pAddr += fileOffSet;
        memcpy((ClCharT *)&fileOffSet, pAddr, sizeof(fileOffSet));
    }
    memcpy(pAddr, (ClCharT *) &pHeader->nextEntry,
                    sizeof(fileOffSet));
    pAddr += sizeof(fileOffSet);
    memcpy(pAddr, &pFileName->length, sizeof(pFileName->length));
    pAddr += sizeof(pFileName->length);
    memcpy(pAddr, pFileName->pValue, pFileName->length);
    pAddr += pFileName->length;
    memcpy(pAddr, &pFileLocation->length, sizeof(pFileLocation->length));
    pAddr += sizeof(pFileLocation->length);
    memcpy(pAddr, pFileLocation->pValue, pFileLocation->length);
    pAddr += pFileLocation->length;
    pHeader->nextEntry += sizeof(fileOffSet) + sizeof(pFileName->length) +
                          pFileName->length + sizeof(pFileLocation->length)
                          + pFileLocation->length ;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}


static ClRcT
clLogFileOwnerOldFileDelete(ClCharT    *fileName, 
                            ClUint32T  fileCount)
{
    ClRcT      rc                                   = CL_OK;
    ClCharT    *pSoftLinkName                       = NULL;
    ClCharT    *pActualFile                         = NULL;
    ClInt32T   fileNameLen                          = 0;
    ClUint32T  fileCountLen                         = 0;
    ClUint32T  softLinkLen                          = 0;
    ClUint8T   tempVar                              = 0;

    CL_LOG_DEBUG_TRACE(("Enter: fileCount: %u", fileCount));
    
    fileCountLen = snprintf((ClCharT*)&tempVar, 1, "%u", fileCount);

    softLinkLen = strlen(fileName) + 1 + fileCountLen + 1;
    
    pSoftLinkName = (ClCharT*) clHeapAllocate(softLinkLen);
    if( NULL == pSoftLinkName )
    {
        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(); rc[0x %x]", rc));
        return rc;
    }
    snprintf(pSoftLinkName, softLinkLen, "%s.%u", fileName, fileCount);
    
    fileNameLen = strlen(fileName) + 1 + CL_LOG_TIMESTR_LEN + 1; 

    pActualFile = (ClCharT*) clHeapAllocate(fileNameLen);
    if( NULL == pActualFile )
    {
        clHeapFree(pSoftLinkName);
        rc = CL_LOG_RC(CL_ERR_NO_MEMORY);
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(); rc[0x %x]", rc));
        return rc;
    }
    
    rc = clLogReadLink(pSoftLinkName, pActualFile, &fileNameLen );
    if( CL_OK != rc )
    {
        clHeapFree(pSoftLinkName);
        clHeapFree(pActualFile);
        return rc;
    }
    pActualFile[fileNameLen] = '\0';
    
    rc = clLogFileUnlink(pSoftLinkName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogFileUnlink(): rc[0x %x]", rc));
        clHeapFree(pSoftLinkName);
        clHeapFree(pActualFile);
        return rc;
    }
    CL_LOG_CLEANUP(clLogFileUnlink(pActualFile), CL_OK);

    clHeapFree(pSoftLinkName);
    clHeapFree(pActualFile);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerWaterMarkChkNPublish(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                    ClCntNodeHandleT        hFileNode, 
                                    ClLogFileOwnerDataT    *pFileOwnerData)
{
    ClRcT          rc        = CL_OK;
    ClLogFileKeyT  *pFileKey = NULL;
    ClUint32T      nextRec   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserKeyGet(pFileOwnerEoEntry->hFileTable, hFileNode, 
                              (ClCntDataHandleT *) &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    
    nextRec = (pFileOwnerData->streamAttr.fileUnitSize - pFileOwnerData->pFileHeader->remSize) / 
              pFileOwnerData->streamAttr.recordSize;
    if( nextRec >= pFileOwnerData->streamAttr.waterMark.highLimit )
    {
        /*Crossed the limit so publish the event */
        CL_LOG_DEBUG_TRACE(("Publishing Event for water mark: %llu \n",
                pFileOwnerData->streamAttr.waterMark.highLimit));
        rc = clLogHighWatermarkEvent(&pFileKey->fileName,
                                     &pFileKey->fileLocation);
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

/*
 * We could just fire the syslog with a severity of DEBUG instead of mapping it.
 * since the log messages of ASP itself contains severity info. but it could impact syslog
 * severity filters
 */

static void doSyslog(ClLogSeverityT logSev, const ClCharT *buf, ClInt32T len)
{
#define SYSLOG_SEV_DEFAULT (LOG_INFO) /*which is the default in var/log/messages for most linux systems */
    static struct ClSyslogSeverityMap
    {
        const ClLogSeverityT severity;
        ClInt32T syslogSev;
    } clSyslogSeverityMap[] = {
        {CL_LOG_SEV_EMERGENCY, LOG_EMERG},
        {CL_LOG_SEV_ALERT, LOG_ALERT},
        {CL_LOG_SEV_CRITICAL, LOG_CRIT},
        {CL_LOG_SEV_ERROR, LOG_ERR},
        {CL_LOG_SEV_WARNING, LOG_WARNING},
        {CL_LOG_SEV_NOTICE, LOG_NOTICE},
        {CL_LOG_SEV_INFO, LOG_INFO},
        {CL_LOG_SEV_DEBUG, SYSLOG_SEV_DEFAULT},
        {CL_LOG_SEV_TRACE, SYSLOG_SEV_DEFAULT},
        {(ClLogSeverityT)0, 0},
    };
    static ClBoolT syslogSevDisabled = (ClBoolT)-1;
    register ClInt32T i;
    ClLogSeverityT severity;
    ClInt32T syslogSev = SYSLOG_SEV_DEFAULT; 

    if(syslogSevDisabled == (ClBoolT)-1)
    {
        syslogSevDisabled = clParseEnvBoolean("ASP_SYSLOG_SEVERITY_DISABLED");
    }

    if(!syslogSevDisabled)
    {
        for(i = 0; (severity = clSyslogSeverityMap[i].severity); ++i)
        {
            if(severity == logSev)
            {
                syslogSev = clSyslogSeverityMap[i].syslogSev;
                break;
            }
        }
    } 
    syslog(syslogSev, "%.*s", len, buf);
#undef SYSLOG_SEV_DEFAULT
}

ClRcT
clLogFileOwnerFileWrite(ClLogFileOwnerDataT  *pFileOwnerData,
                        ClUint32T            recordSize,
                        ClUint32T            *pNumRecords,
                        ClUint8T             *pRecords)
{
    ClRcT      rc         = CL_OK;
    ClUint8T   endian     = 0;
    ClLogSeverityT severity;
    ClUint8T *pRecordIter = NULL;
    struct iovec iov[*pNumRecords];
    ClUint32T  idx        = 0;
    ClUint32T count        = 0;
    ClInt32T  numBytes   = 0;
    ClBoolT syslogEnabled = pFileOwnerData->streamAttr.syslog;

    memset(&severity,0,sizeof(ClLogSeverityT));

    if(!pFileOwnerData->fileUnitPtr) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    if( pFileOwnerData->pFileHeader->fileType == CL_LOG_FILE_TYPE_INVALID )
    {
        /* File type is not yet decied to find out that */
        memcpy(&endian, pRecords, sizeof(endian));
        pFileOwnerData->pFileHeader->fileType = 
            ( endian == '1' || endian == '0' ) ? CL_LOG_FILE_TYPE_ASCII:
            CL_LOG_FILE_TYPE_BINARY;
    }
    for(count = 0; count < *pNumRecords; count++)
    {
        memcpy(&endian, pRecords, sizeof(endian));
        if( pFileOwnerData->pFileHeader->fileType == 
            CL_LOG_FILE_TYPE_ASCII )
        {
            pRecordIter = pRecords + LOG_ASCII_ENDIAN_LEN;
            sscanf((ClCharT*)pRecordIter, LOG_ASCII_SEV_FMT, (ClUint32T*)&severity);
            pRecordIter += LOG_ASCII_SEV_LEN;
            if( (endian == '1' || endian == '0') && (severity > 0  && severity <= CL_LOG_SEV_MAX) )

            {
                ClUint32T hdrLen = 0, len = 0, choppedLen = 0;
                sscanf((char*)pRecordIter, LOG_ASCII_HDR_LEN_FMT, &hdrLen);
                pRecordIter += LOG_ASCII_HDR_LEN;
                sscanf((char*)pRecordIter, LOG_ASCII_DATA_LEN_FMT LOG_DATA_DELIMITER_FMT, &len);
                pRecordIter += LOG_ASCII_DATA_LEN + LOG_DATA_DELIMITER_LEN; 
                iov[idx].iov_base = (char*)pRecordIter;
                /*Last 1 Bye is reserved for record write in-progress indicator */
                iov[idx].iov_len  = CL_MIN(hdrLen + len + 1, recordSize - LOG_ASCII_METADATA_LEN - 1);
                choppedLen = strlen((ClCharT *)iov[idx].iov_base) + 1;
                /* Ensure that the record is CR terminated.
                   All ASCII records should have a \n from the client, but
                   may not if the message len was chopped during the send 
                */
                if (iov[idx].iov_len > choppedLen)
                {
                    iov[idx].iov_len = choppedLen - 1;
                }
                *((ClCharT *)iov[idx].iov_base + iov[idx].iov_len - 1) = '\n';
                if(syslogEnabled) 
                {
                    doSyslog(severity, (const ClCharT*)iov[idx].iov_base, iov[idx].iov_len);
                }
            }
            else
            {
                iov[idx].iov_base = (void *)gClLogErrorAsciiRecord;
                iov[idx].iov_len  = strlen(gClLogErrorAsciiRecord);
            }
            idx++;
        }
        else
        {
            if( (endian != '1') && (endian != '0') )
            {
                /*
                 * found a binary record,  
                 */
                iov[idx].iov_base = (char*)pRecords;
                /*Last 1 Bye is reserved for record write in-progress indicator */
                iov[idx].iov_len  = recordSize - 1; 
                idx++;
            }
        }
        pRecords += recordSize;
    }
    *pNumRecords = idx;
    rc = clLogFileIOVwrite(pFileOwnerData->fileUnitPtr, iov, *pNumRecords, &numBytes);
    if( CL_OK != rc )
    {
        return rc;
    }
    pFileOwnerData->pFileHeader->remSize -= numBytes;
    pFileOwnerData->pFileHeader->nextRec += *pNumRecords;

    return rc;
}

ClRcT
clLogFileOwnerNewFunitCreate(ClCntNodeHandleT      hFileNode,
                             ClLogFileOwnerDataT  *pFileOwnerData)
{
    ClRcT                  rc                          = CL_OK;
    ClLogFileKeyT          *pFileKey                   = NULL;
    ClCharT                *fileName                   = NULL;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry          = NULL;
    ClUint32T              maxRecInFunit               = 0;
    ClCharT                timeStr[CL_MAX_NAME_LENGTH] = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    maxRecInFunit  = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    rc = clCntNodeUserKeyGet(pFileOwnerEoEntry->hFileTable, hFileNode, 
                            (ClCntDataHandleT *) &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogFileOwnerFileNameGet(&pFileKey->fileName, 
                                    &pFileKey->fileLocation, &fileName);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( pFileOwnerData->pFileHeader->deleteCnt == 
            pFileOwnerData->pFileHeader->currFileUnitCnt )
    {
        rc = clLogFileOwnerOldFileDelete(fileName, 
                pFileOwnerData->pFileHeader->deleteCnt);
        if( CL_OK != rc )
        {
            clHeapFree(fileName);
            return rc;
        }
        CL_LOG_DEBUG_TRACE(("Deleting this file: %s", fileName));
        /* 
         * Move the lastSavedRec into nextFile.
         * NextRec wont be in that range.
         */
        pFileOwnerData->pFileHeader->deleteCnt++;
        pFileOwnerData->pFileHeader->deleteCnt %=
            pFileOwnerData->streamAttr.maxFilesRotated;
    }
    rc = clLogFileClosureEvent(&pFileKey->fileName, &pFileKey->fileLocation);
    if( CL_OK != rc )
    {
        clHeapFree(fileName);
        return rc;
    }
    rc = clLogFileOwnerTimeGet(timeStr, 0);
    if( CL_OK != rc )
    {
        clHeapFree(fileName);
        return rc;            
    }
    rc = clLogFileOwnerLogFileCreateNPopulate(timeStr, fileName,
                            pFileOwnerData->streamAttr.recordSize,
                            pFileOwnerData->pFileHeader->currFileUnitCnt,
                            &pFileOwnerData->fileUnitPtr);
    if( CL_OK != rc )
    {
        clHeapFree(fileName);
        return rc;
    }

    /* New file unit has been created, so this file will have max-1 records 
     * resetting the rem size to be max -1 records. 
     */
    pFileOwnerData->pFileHeader->remSize = pFileOwnerData->streamAttr.fileUnitSize - 
                                            pFileOwnerData->streamAttr.recordSize;
    pFileOwnerData->pFileHeader->nextRec = 1;                                            
    pFileOwnerData->pFileHeader->currFileUnitCnt++;
    pFileOwnerData->pFileHeader->currFileUnitCnt %=
        pFileOwnerData->streamAttr.maxFilesRotated;
    clHeapFree(fileName);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    (void)maxRecInFunit;
    return rc;
}

ClRcT
clLogFileOwnerFileFullAction(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                             ClCntNodeHandleT       hFileNode, 
                             ClLogFileOwnerDataT    *pFileOwnerData, 
                             ClUint32T              numRecords, 
                             ClUint32T              recordSize, 
                             ClUint8T               *pRecords)
{
    ClRcT          rc            = CL_OK;
    ClLogFileKeyT  *pFileKey     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if(!pFileOwnerData->fileUnitPtr) return CL_LOG_RC(CL_ERR_NOT_EXIST);

    rc = clCntNodeUserKeyGet(pFileOwnerEoEntry->hFileTable, hFileNode, 
                            (ClCntDataHandleT *) &pFileKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogFileUnitFullEvent(&pFileKey->fileName, &pFileKey->fileLocation);
    if( CL_OK != rc )
    {
        return rc;
    }
    switch( pFileOwnerData->streamAttr.fileFullAction )
    {
        case CL_LOG_FILE_FULL_ACTION_HALT:
        {
            /*
             * Mark file as FULL and return ERROR also
             */
            return CL_LOG_RC(CL_LOG_ERR_FILE_FULL);
        }
        case CL_LOG_FILE_FULL_ACTION_WRAP:
        {
            /*
             * fSeek this fd into firstRecord of this file.
             */
            clLogDebug("FILEOWN", "WRITE", 
                      "Seeking to the first record position: %p offset : %d", 
                      (ClPtrT) pFileOwnerData->fileUnitPtr, recordSize);
            rc = clLogFileSeek(pFileOwnerData->fileUnitPtr, 
                               recordSize);
            if( CL_OK != rc )
            {
                return rc;
            }
            /*
             * one default record has been written at start of the file 
             * resetting the remaining size to be (max - 1) records
             */
            pFileOwnerData->pFileHeader->remSize = pFileOwnerData->streamAttr.fileUnitSize - pFileOwnerData->streamAttr.recordSize;
            pFileOwnerData->pFileHeader->nextRec = 1;
            break;
        }
        case CL_LOG_FILE_FULL_ACTION_ROTATE:
        {
            /*
             * close the currFileUnitFd..
             * move the nextFd to currFileUnitFd.
             * make nextFileUnitFd is invalid value.
             */
            rc = clLogFileClose_L(pFileOwnerData->fileUnitPtr);
            if( CL_OK != rc )
            {
                return rc;
            }
            pFileOwnerData->fileUnitPtr = NULL;
            rc = clLogFileOwnerNewFunitCreate(hFileNode, pFileOwnerData);
            if( CL_OK != rc )
            {
                return rc;
            }
            break;
        }
    }
    /* Write into the current fileFd. */
    
    rc = clLogFileRecordsPersist(pFileOwnerEoEntry, hFileNode, pFileOwnerData,
                                 numRecords * recordSize, numRecords, pRecords);

    if(pFileOwnerData->pFileHeader)
        clOsalMsync(pFileOwnerData->pFileHeader, CL_LOG_FILEOWNER_CFG_FILE_SIZE, MS_SYNC);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerOverwriteChkNSet(ClLogFileHeaderT  *pFileHeader,
                               ClUint32T         numRecords, 
                               ClUint32T         maxNumRecords,
                               ClUint32T         *pnOverWritten)
{
#if 0
    ClRcT      rc           = CL_OK;
    ClUint32T  nOverWritten = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( pFileHeader->nextRec > pFileHeader->lastSavedRec )
    {
        if( ((pFileHeader->nextRec + numRecords) - pFileHeader->lastSavedRec) >=
            maxNumRecords )
        {
            nOverWritten = numRecords - (maxNumRecords + pFileHeader->lastSavedRec -
                             pFileHeader->nextRec);
        }
    }
    else
    {
        if( (pFileHeader->lastSavedRec - pFileHeader->nextRec) >=
             numRecords )
        {
            nOverWritten = numRecords - (pFileHeader->lastSavedRec -
                           pFileHeader->nextRec) ;
        }
    }
    *pnOverWritten  = nOverWritten;
    #endif

    CL_LOG_DEBUG_TRACE(("Exit"));
    return CL_OK;
}

ClRcT
clLogFileOwnerRecordPersist_old(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                            ClCntNodeHandleT        hFileNode, 
                            ClUint32T               numRecords, 
                            ClUint8T                *pRecords)
{
#if 0
    ClRcT                rc              = CL_OK;
    ClUint32T            maxNumRecords   = 0;
    ClUint32T            maxRecInFunit   = 0; 
    ClUint8T             *pTempRecords   = NULL;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClUint32T            numOfFreeSlots  = 0;
    ClUint32T            numRemRecords   = 0;
    ClUint32T            nOverwritten    = 0;

    CL_LOG_DEBUG_TRACE(("Enter: NumRecords: %d", numRecords));

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode, 
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    
    CL_LOG_MAX_RECNUM_CALC(pFileOwnerData->streamAttr, maxNumRecords);

    maxRecInFunit  = CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr);
    if( CL_FALSE == pFileOwnerData->pFileHeader->overWriteFlag )
    {
        rc = clLogFileOwnerOverwriteChkNSet(pFileOwnerData->pFileHeader,
                                            numRecords, maxNumRecords, 
                                            &nOverwritten);
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    numOfFreeSlots = maxRecInFunit - (pFileOwnerData->pFileHeader->nextRec % maxRecInFunit);  
    if( 0 == pFileOwnerData->pFileHeader->nextRec % maxRecInFunit ) 
    {
        /*
         * First phase of write, nextRec reaches the end of file, then next
         * iteration, u may not know , it overflows the fileSize.
         * so this check.
         */
        rc = clLogFileOwnerFileFullAction(pFileOwnerEoEntry, hFileNode,
                                          pFileOwnerData, maxNumRecords,
                                          numRecords, 
                                          pFileOwnerData->streamAttr.recordSize, 
                                          pRecords); 
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    else if( numOfFreeSlots >= numRecords )
    {
        rc = clLogFileOwnerFileWrite(pFileOwnerData,
                                     pFileOwnerData->streamAttr.recordSize, 
                                     &numRecords, pRecords);
        if( CL_OK != rc )
        {
            return rc;
        }
        pFileOwnerData->pFileHeader->nextRec += numRecords;
        pFileOwnerData->pFileHeader->nextRec %= (2 * maxNumRecords);
        if( CL_TRUE == pFileOwnerData->pFileHeader->overWriteFlag )
        {
            pFileOwnerData->pFileHeader->lastSavedRec += numRecords;
            pFileOwnerData->pFileHeader->lastSavedRec %= (2 * maxNumRecords);
        }
    }
    else
    {
        numRemRecords = numRecords - numOfFreeSlots;
        if( 0 != numOfFreeSlots )
        {
            rc = clLogFileOwnerFileWrite(pFileOwnerData, 
                                         pFileOwnerData->streamAttr.recordSize, 
                                         &numOfFreeSlots, pRecords); 
            if( CL_OK != rc )
            {
                return rc;
            }
            pFileOwnerData->pFileHeader->nextRec += numOfFreeSlots;
            pFileOwnerData->pFileHeader->nextRec %= (2 * maxNumRecords);
            if( CL_TRUE == pFileOwnerData->pFileHeader->overWriteFlag )
            {
                pFileOwnerData->pFileHeader->lastSavedRec += numOfFreeSlots;
                pFileOwnerData->pFileHeader->lastSavedRec %= (2 * maxNumRecords);
            }
        }
        pTempRecords = pRecords + (pFileOwnerData->streamAttr.recordSize * 
                                   numOfFreeSlots); 
        rc = clLogFileOwnerFileFullAction(pFileOwnerEoEntry, hFileNode,
                                          pFileOwnerData, 
                                          maxNumRecords, numRemRecords, 
                                          pFileOwnerData->streamAttr.recordSize, 
                                          pTempRecords); 
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    if( 0 != nOverwritten )
    {
        pFileOwnerData->pFileHeader->overWriteFlag = CL_TRUE;
        pFileOwnerData->pFileHeader->lastSavedRec += nOverwritten;
        pFileOwnerData->pFileHeader->lastSavedRec %= (2 * maxNumRecords);
        pFileOwnerData->pFileHeader->nOverwritten += nOverwritten;
        ++pFileOwnerData->pFileHeader->iterationCnt;
    }
    rc = clLogFileOwnerWaterMarkChkNPublish(pFileOwnerEoEntry, hFileNode, 
                                             pFileOwnerData);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    #endif
    return CL_OK;
}

ClRcT
clLogFileOwnerFileNodeGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                           ClHandleT               hStream,
                           ClCntNodeHandleT        *phFileNode)
{
    ClRcT                   rc     = CL_OK;
    ClLogFileOwnerHdlrDataT *pData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clHandleCheckout(pFileOwnerEoEntry->hStreamDB, hStream, 
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }
//    *phFileNode = *pData;
    *phFileNode = pData->hFileNode;
    pData->activeCnt++;
    
    CL_LOG_CLEANUP(clHandleCheckin(pFileOwnerEoEntry->hStreamDB,
                                   hStream), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

void
clLogFileOwnerRecordDeliverCb(ClHandleT  hStream,
                              ClUint64T  sequenceNum,
                              ClUint32T  numRecords,
                              ClPtrT     pRecords)
{
    ClRcT                   rc                = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClCntNodeHandleT        hFileNode         = CL_HANDLE_INVALID_VALUE;
    ClUint32T               activeCnt         = 0;
    ClLogFileOwnerDataT    *pFileOwnerData    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return ;
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return ;
    }
    activeCnt = pFileOwnerEoEntry->activeCnt;
    if( CL_LOG_FILEOWNER_STATE_CLOSED == pFileOwnerEoEntry->status
        ||
        CL_TRUE == pFileOwnerEoEntry->terminate)
    {
        CL_LOG_DEBUG_ERROR(("Log FileOwner is shutting down..."));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                CL_OK);
        return ; 
    }

    ++activeCnt;
    pFileOwnerEoEntry->status = CL_LOG_FILEOWNER_STATE_ACTIVE;
    rc = clLogFileOwnerFileNodeGet(pFileOwnerEoEntry, hStream, &hFileNode); 
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                           CL_OK);
        return ;
    }

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode,
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                           CL_OK);
        return ;
    }
    rc = clOsalMutexLock_L(&pFileOwnerData->fileEntryLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                           CL_OK);
        return ;
    }
    pFileOwnerEoEntry->activeCnt = activeCnt;
    CL_LOG_DEBUG_TRACE(("streamHdlr active cnt: %d  status %d ",
                pFileOwnerEoEntry->activeCnt, pFileOwnerEoEntry->status));
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);

    rc = clLogFileOwnerRecordPersist(pFileOwnerEoEntry, hFileNode,
                                      numRecords, (ClUint8T *) pRecords);
    if( (CL_OK != rc) && (CL_GET_ERROR_CODE(rc) != CL_LOG_ERR_FILE_FULL) )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock), 
                                     CL_OK);
        clLogFileOwnerFileEntryCntDecrement(pFileOwnerEoEntry, pFileOwnerData, 
                                            hFileNode, hStream);
        return ;
    }

    if( CL_LOG_ERR_FILE_FULL == CL_GET_ERROR_CODE(rc) )
    {
        /* Sending explicitly O ack, to notify FILE FULL */
        rc = clLogHandlerRecordAck(hStream, sequenceNum, 0);
    }
    else
    {
        rc = clLogHandlerRecordAck(hStream, sequenceNum, numRecords);
    }
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("clLogHandlerRecordAck(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock), 
                   CL_OK);
        clLogFileOwnerFileEntryCntDecrement(pFileOwnerEoEntry, pFileOwnerData, 
                                            hFileNode, hStream);
        return ;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock), 
                   CL_OK);
    clLogFileOwnerFileEntryCntDecrement(pFileOwnerEoEntry, pFileOwnerData, 
                                        hFileNode, hStream);


    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogFileOwnerStreamOpen(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                          SaNameT                 *pStreamName,
                          ClLogStreamScopeT       streamScope,
                          SaNameT                 *pStreamScopeNode,
                          ClUint16T               streamId,
                          ClLogStreamAttrIDLT     *pStreamAttr, 
                          ClCntNodeHandleT        *phFileNode,
                          ClCntNodeHandleT        *phStreamNode)
{
    ClRcT                 rc               = CL_OK;
    ClBoolT               entryAdd         = CL_FALSE;
    ClBoolT               streamEntryAdd   = CL_FALSE;
    ClBoolT               restart          = CL_FALSE;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogFileOwnerFileEntryGet(pFileOwnerEoEntry, pStreamAttr,
                                    restart, phFileNode, &entryAdd);
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
    /*
     * Uncommenting the following check for the following scenario
     * N local streams going to one remote & same file. If one node going down
     * will not affect fileOwner as he is not listening to any node death
     * event.
     */
    if( CL_FALSE == streamEntryAdd )
    {
        return CL_LOG_RC(CL_ERR_ALREADY_EXIST);
    }
                        
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerHandlerRegister(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                              ClHandleT               hFileOwner, 
                              SaNameT                 *pStreamName,
                              ClLogStreamScopeT       streamScope,
                              SaNameT                 *pStreamScopeNode,
                              ClUint16T               streamId,
                              ClCntNodeHandleT        hFileNode,
                              ClCntNodeHandleT        hStreamNode,
                              ClBoolT                 logRestart)
{
    ClRcT                       rc               = CL_OK;
    ClLogFileOwnerStreamDataT  *pStreamData     = NULL;
    ClLogFileOwnerDataT        *pFileOwnerData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode,
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clCntNodeUserDataGet(pFileOwnerData->hStreamTable, hStreamNode,
                              (ClCntDataHandleT *) &pStreamData);
    if( CL_OK != rc )
    {
        /* this case should not arise, something went wrong */
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    if( pStreamData->status == CL_LOG_FILEOWNER_STATUS_CLOSED )
    {
        return CL_LOG_RC(CL_ERR_NOT_EXIST);
    }

    pStreamData->hFileOwner = hFileOwner;
    pStreamData->streamId   = streamId;

    rc = clLogFileOwnerHdlCreate(pFileOwnerEoEntry, hFileOwner,
                                 hFileNode, hStreamNode); 
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_FALSE == logRestart )
    {
        rc = clLogFileOwnerCfgFileStreamInfoUpdate(pFileOwnerData, 
                                                   pStreamName,
                                                   streamId);
        if( CL_OK != rc )
        {
            /* 
             *  Cleanup of above handle will happen in the calling function
             */
            return rc;
        }
    }
    pStreamData->status = CL_LOG_FILEOWNER_STATUS_INIT;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamEntryChkNGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                 ClCntNodeHandleT       hFileNode, 
                                 SaNameT                *pStreamName,
                                 ClLogStreamScopeT      streamScope, 
                                 SaNameT                *pStreamScopeNode,
                                 ClCntNodeHandleT       *phStreamNode)
{
    ClRcT                 rc             = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClLogStreamKeyT      *pStreamKey     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode,
                             (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              CL_LOG_STREAM_HDLR_MAX_STREAMS,
                              &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCntNodeFind(pFileOwnerData->hStreamTable, 
                       (ClCntKeyHandleT) pStreamKey, phStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeFind(): rc[0x %x]", rc));
        clLogStreamKeyDestroy(pStreamKey);
        return rc;
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamEntryRemove(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                ClCntNodeHandleT       hFileNode, 
                                SaNameT                *pStreamName,
                                ClLogStreamScopeT      streamScope, 
                                SaNameT                *pStreamScopeNode)
{
    ClRcT                rc              = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    ClLogStreamKeyT      *pStreamKey     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode,
                             (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              CL_LOG_STREAM_HDLR_MAX_STREAMS,
                              &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCntAllNodesForKeyDelete(pFileOwnerData->hStreamTable, 
                                   (ClCntKeyHandleT) pStreamKey);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntAllNodesForKeyDelete(): rc[0x %x]", rc));
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamCreateEvent(SaNameT              *pStreamName,
                                ClLogStreamScopeT    streamScope,
                                SaNameT              *pStreamScopeNode,
                                ClUint16T            streamId,
                                ClLogStreamAttrIDLT  *pStreamAttr,
                                ClBoolT              doHandlerRegister)
{
    ClRcT                   rc                = CL_OK;
    ClRcT                   retCode           = CL_OK;
    ClBoolT                 fileOwner         = CL_FALSE;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClCntNodeHandleT        hFileNode         = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT        hStreamNode       = CL_HANDLE_INVALID_VALUE;
    ClHandleT               hFileOwner        = CL_HANDLE_INVALID_VALUE;
    ClBoolT                 logHandlerRegister = doHandlerRegister;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamAttr), CL_LOG_RC(CL_ERR_NULL_POINTER));

    clLogDebug(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
              "Create Event for stream: %.*s scope: %.*s", 
               pStreamName->length, pStreamName->value,
              pStreamScopeNode->length, pStreamScopeNode->value);
    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clLogFileOwnerLocationVerify(pFileOwnerEoEntry, &pStreamAttr->fileLocation, &fileOwner);  
    if( (CL_OK != rc) || (CL_FALSE == fileOwner) )
    {
        return rc;
    }
    clLogDebug(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, " %d is fileOwner for fileName %s", clIocLocalAddressGet(), pStreamAttr->fileName.pValue);
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

    rc = clLogFileOwnerStreamOpen(pFileOwnerEoEntry, pStreamName,
                                   streamScope, pStreamScopeNode, streamId, 
                                   pStreamAttr, &hFileNode, &hStreamNode);

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);
    /* 
     * Uncommenting the following check, as it is required for the following
     * scenario. Stream is opened by some other node and the stream owner gone
     * down and recreating the stream entries as its local stream, again
     * getting the stream create event as it didn't delete those stream
     * information. so avoiding handler register to avoid for the same
     * fileowner one handle is fine enough to receive the data
     */
    if( CL_ERR_ALREADY_EXIST == CL_GET_ERROR_CODE(rc) )
    {
        /*
         * StreamEntry already registered with Handler.
         */
        clLogInfo(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
                  "Stream entry [%.*s:%.*s] already exist", 
                  pStreamScopeNode->length, pStreamScopeNode->value,
                  pStreamName->length, pStreamName->value); 

        return CL_OK;
    }
    if( CL_OK != rc )
    {
        return rc;
    }

    if(streamScope == CL_LOG_STREAM_LOCAL)
        logHandlerRegister = CL_FALSE;

    if( CL_TRUE == logHandlerRegister)
    {
        /* No acknowledgements to reduce the traffic */
        retCode = clLogHandlerRegister(pFileOwnerEoEntry->hLog, *pStreamName, 
                                       streamScope, *pStreamScopeNode, 0, &hFileOwner);
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( (CL_OK != rc) || (CL_OK != retCode) )
    {
        clDbgPause();
        CL_LOG_CLEANUP(clLogFileOwnerStreamEntryRemove(pFileOwnerEoEntry, 
                                                       hFileNode, pStreamName,
                                                       streamScope,
                                                       pStreamScopeNode), CL_OK);
        if( CL_OK == rc )
        {
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                    CL_OK);
        }
        return retCode;
    }
    rc = clLogFileOwnerStreamEntryChkNGet(pFileOwnerEoEntry, hFileNode,
                                          pStreamName, streamScope,
                                          pStreamScopeNode, &hStreamNode);
    if( CL_OK != rc )
    {
        clDbgPause();
        if(logHandlerRegister)
            clLogHandlerDeregister(hFileOwner);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                CL_OK);
        return rc;
    }

    if(logHandlerRegister)
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
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamCloseEvent(SaNameT             *pStreamName,
                                ClLogStreamScopeT  streamScope,
                                SaNameT            *pStreamScopeNode)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pStreamName), CL_LOG_RC(CL_ERR_NULL_POINTER));
    CL_LOG_PARAM_CHK((NULL == pStreamScopeNode), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_FALSE == pFileOwnerEoEntry->status )
    {
        CL_LOG_DEBUG_ERROR(("Log FileOwner is shutting down..."));
        return CL_OK;
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    rc = clLogFileOwnerStreamClose(pFileOwnerEoEntry, pStreamName, 
                                   streamScope, pStreamScopeNode); 

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
            CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;

}

ClRcT
clLogFileOwnerStreamClose(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                          SaNameT                *pStreamName,
                          ClLogStreamScopeT      streamScope,
                          SaNameT                *pStreamScopeNode)
{
    ClRcT                      rc              = CL_OK;
    ClLogFileOwnerDataT        *pFileOwnerData = NULL;
    ClCntNodeHandleT           hNode           = CL_HANDLE_INVALID_VALUE;
    ClLogStreamKeyT            *pStreamKey     = NULL;
    ClCntNodeHandleT           hStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClLogFileOwnerStreamDataT  *pStreamData    = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_DEBUG_TRACE(("Closing Stream: %s scope: %s", pStreamName->value,
                         pStreamScopeNode->value));

    rc = clLogStreamKeyCreate(pStreamName, pStreamScopeNode,
                              CL_LOG_STREAM_HDLR_MAX_STREAMS,
                              &pStreamKey);
    if( CL_OK != rc )
    {
        return rc;
    }

    if (pFileOwnerEoEntry->hFileTable)
      {
        /* Iterate through the list of files */

        rc = clCntFirstNodeGet(pFileOwnerEoEntry->hFileTable, &hNode);
        if( CL_OK != rc )
          {
            CL_LOG_DEBUG_TRACE(("clCntFirstNodeGet(): rc[0x %x]", rc));
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
          }
        while( CL_HANDLE_INVALID_VALUE != hNode )
          {
            pFileOwnerData = NULL;
            rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, 
                                      hNode, 
                                      (ClCntDataHandleT *) &pFileOwnerData);
            if( CL_OK != rc )
              {
                CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
                clLogStreamKeyDestroy(pStreamKey);
                return rc;
              }

            /* For each file, see if it has this stream */
            rc = clCntNodeFind(pFileOwnerData->hStreamTable, 
                               (ClCntKeyHandleT) pStreamKey, &hStreamNode); 
            if( CL_OK == rc )
              {
                break;  /* It does */
              }
            if( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST )
              {
                CL_LOG_DEBUG_ERROR(("clCntNodeFind(): rc[0x %x]", rc));
                clLogStreamKeyDestroy(pStreamKey);
                return rc;
              }
            rc = clCntNextNodeGet(pFileOwnerEoEntry->hFileTable, hNode, &hNode);
            if( CL_OK != rc )
              {
                CL_LOG_DEBUG_TRACE(("clCntNextNodeGet(): rc[0x %x]", rc));
                hNode = CL_HANDLE_INVALID_VALUE;
              }
          }
        if( CL_OK != rc )
          {
            CL_LOG_DEBUG_TRACE(("Stream doesn't exist"));
            clLogStreamKeyDestroy(pStreamKey);
            return CL_OK;
          }
      }

    /* Set the stream status to closed */
    if( (NULL != pFileOwnerData) && (CL_HANDLE_INVALID_VALUE != hStreamNode) )
    {
        rc = clCntNodeUserDataGet(pFileOwnerData->hStreamTable, hStreamNode, 
                                  (ClCntDataHandleT *) &pStreamData );
        if( CL_OK != rc )
        {
            /* This case should not arise here */
            CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }
        if( CL_LOG_FILEOWNER_STATUS_WIP == pStreamData->status )
        {
            pStreamData->status = CL_LOG_FILEOWNER_STATUS_CLOSED;
            clLogStreamKeyDestroy(pStreamKey);
            return rc;
        }
        pStreamData->status = CL_LOG_FILEOWNER_STATUS_CLOSED;

        if( pStreamData->hFileOwner != CL_HANDLE_INVALID_VALUE )
        {
        CL_LOG_CLEANUP(clLogFileOwnerStreamChkNDestroy(
                                pFileOwnerEoEntry, pFileOwnerData, 
                                pStreamData->hFileOwner, CL_FALSE),
                       CL_OK);
        }
        else
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileOwnerData->hStreamTable,
                                       hStreamNode), CL_OK);
        }
    }
    clLogStreamKeyDestroy(pStreamKey);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerStreamChkNDestroy(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                ClLogFileOwnerDataT    *pFileOwnerData, 
                                ClHandleT              hFileOwner, 
                                ClBoolT                decrement)
{
    ClRcT                      rc           = CL_OK;
    ClLogFileOwnerHdlrDataT    *pData       = NULL;
    ClUint32T                  activeCnt    = 0;
    ClCntNodeHandleT           hStreamNode  = CL_HANDLE_INVALID_VALUE;
    ClLogFileOwnerStreamDataT  *pStreamData = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clHandleCheckout(pFileOwnerEoEntry->hStreamDB, hFileOwner, 
                          (void **) &pData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_TRUE == decrement )
    {
        pData->activeCnt--;
    }
    activeCnt   = pData->activeCnt;
    hStreamNode = pData->hStreamNode;
    
    CL_LOG_CLEANUP(clHandleCheckin(pFileOwnerEoEntry->hStreamDB,
                                   hFileOwner), CL_OK);
    CL_LOG_DEBUG_TRACE(("handle : %#llX activeCnt: %d close status: %d", 
                        hFileOwner, activeCnt, decrement));
    if( 0 == activeCnt )
    {
        rc = clCntNodeUserDataGet(pFileOwnerData->hStreamTable, hStreamNode,
                                  (ClCntDataHandleT *) &pStreamData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
            return rc;
        }
        if( CL_LOG_FILEOWNER_STATUS_CLOSED == pStreamData->status ) 
        {
            CL_LOG_CLEANUP(clCntNodeDelete(pFileOwnerData->hStreamTable,
                                           hStreamNode), CL_OK);
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCfgFileCompDataUpdate(ClLogFileOwnerDataT  *pFileOwnerData, 
                                     SaNameT               *pCompName,
                                     ClUint32T             clientId)
{
    ClRcT             rc         = CL_OK;
    ClLogFileHeaderT  *pHeader   = pFileOwnerData->pFileHeader;
    ClUint8T          *pStart    = (ClUint8T *) pHeader;
    ClUint16T         compOffSet = 0;
    ClUint8T          *pAddr     = NULL;
    ClUint16T         endMark    = 0;
    ClUint16T         entryLen   = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));

    entryLen = sizeof(endMark) + sizeof(pCompName->length) +
        sizeof(clientId) + pCompName->length;
    if( pHeader->remHdrSize > entryLen )
    {
        pAddr = pStart + pHeader->compOffSet;
    
        memcpy((ClCharT *) &compOffSet, pAddr, sizeof(compOffSet));
        while(compOffSet != 0 )
        {
            pAddr = pStart + compOffSet;
            memcpy((ClCharT *) &compOffSet, pAddr, sizeof(compOffSet));
        }
        memcpy(pAddr, (ClCharT *) &pHeader->nextEntry, sizeof(pHeader->nextEntry));

        pAddr = pStart + pHeader->nextEntry;
        memcpy(pAddr, (ClCharT *) &endMark, sizeof(endMark));
        pAddr += sizeof(endMark);
    
        memcpy(pAddr, (ClCharT *) &pCompName->length, sizeof(pCompName->length));
        pAddr += sizeof(pCompName->length);

        memcpy(pAddr, pCompName->value, pCompName->length);
        pAddr += pCompName->length;

        memcpy(pAddr, (ClCharT *) &clientId, sizeof(clientId));
        pHeader->nextEntry += entryLen; 
        pHeader->remHdrSize -= entryLen;
    }
    else
    {
        static ClInt32T warn;
        if(warn++ < 5 )
        {
            /* The initial pages are alreay over, so keep allocation one page when it hits this area */
            clLogWarning("FOW", "CMP", "Comp Data shared config file FULL");
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCompAdd(ClCntKeyHandleT   key,
                       ClCntDataHandleT  data,
                       ClCntArgHandleT   arg,
                       ClUint32T         size)
{
    ClRcT                 rc               = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = (ClLogFileOwnerDataT *) data;
    ClLogCompDataT        *pCompData       = (ClLogCompDataT *) arg;
    SaNameT               nodeName         = {0};
    SaNameT               compName         = {0};
    SaNameT               compPrefix       = {0};
    ClUint32T             count            = 0;
    ClUint32T             compLen          = 0;
    ClUint32T             clientId         = 0;

    CL_LOG_DEBUG_TRACE(("Enter:"));

    sscanf((ClCharT *) pCompData->compName.value, "%[^_]_%s", compPrefix.value, (ClCharT *) nodeName.value);
    nodeName.length   = strlen((const ClCharT *)nodeName.value);
    compPrefix.length = strlen((const ClCharT *)compPrefix.value);

    if( !(strncmp((const ClCharT *)compPrefix.value, aspCompMap[1].pCompName,
                  compPrefix.length)) )
    {
        /* ASP components, so by default we can add */
        for( count = 0; count < nLogAspComps; count++ )
        {
            compLen = strlen(aspCompMap[count].pCompName);
            snprintf((ClCharT *)compName.value, sizeof(compName.value), "%.*s_%.*s", compLen,
                    aspCompMap[count].pCompName, nodeName.length,
                    (const ClCharT *)nodeName.value);
            compName.length = strlen((const ClCharT *)compName.value);
            clientId        = pCompData->clientId | aspCompMap[count].clntId;

            CL_LOG_DEBUG_TRACE(("compName: %s and clientId : %d \n", compName.value,
                    clientId));
            rc = clLogFileOwnerCfgFileCompDataUpdate(pFileOwnerData,
                                                      &compName,
                                                      clientId);
        }
    }
    else
    {
        rc = clLogFileOwnerCfgFileCompDataUpdate(pFileOwnerData,
                                                  &pCompData->compName,
                                                  pCompData->clientId);
    }
    if( CL_OK != rc )
    {
        /* 
         * Still continues to put in other cfg files, so no error. 
         */
        CL_LOG_DEBUG_ERROR(("Failed to add the compId"));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerCompAddEvent(ClLogCompDataT  *pCompData)
{
    ClRcT                   rc                  = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    CL_LOG_PARAM_CHK((NULL == pCompData), CL_LOG_RC(CL_ERR_NULL_POINTER));

    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE != pFileOwnerEoEntry->hFileTable )
    {
        rc = clCntWalk(pFileOwnerEoEntry->hFileTable, clLogFileOwnerCompAdd, 
                  (ClCntArgHandleT) pCompData, sizeof(pCompData)); 
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntWalk(): rc[0x %x]", rc));
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                CL_OK);
            return rc;
        }
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
            CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogFileOwnerFileEntryCntDecrement(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                    ClLogFileOwnerDataT    *pFileOwnerData, 
                                    ClCntNodeHandleT       hFileNode,
                                    ClHandleT              hFileOwer)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE != hFileOwer )
    {
        CL_LOG_CLEANUP(clLogFileOwnerStreamChkNDestroy(pFileOwnerEoEntry, 
                    pFileOwnerData, 
                    hFileOwer, CL_TRUE), 
                CL_OK);
    }

    --pFileOwnerEoEntry->activeCnt;
    CL_LOG_DEBUG_TRACE(("active cnt: %d", pFileOwnerEoEntry->activeCnt));
    if( CL_LOG_FILEOWNER_STATE_ACTIVE == pFileOwnerEoEntry->status )
    {
       pFileOwnerEoEntry->status = CL_LOG_FILEOWNER_STATE_INACTIVE; 
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                   CL_OK);
    if( CL_LOG_FILEOWNER_STATE_CLOSED == pFileOwnerEoEntry->status )
    {
        CL_LOG_CLEANUP(clLogFileOwnerFileTableDestroy(pFileOwnerEoEntry,
                    hFileNode), CL_OK);
    }

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogFileOwnerFileTableDestroyWithLock(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                       ClCntNodeHandleT        hFileNode,
                                       ClBoolT                 *pLockDestroyed)
{
    ClRcT  rc = CL_OK;

    if(!pLockDestroyed) 
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( CL_LOG_FILEOWNER_STATE_CLOSED != pFileOwnerEoEntry->status )
    {
        pFileOwnerEoEntry->status = CL_LOG_FILEOWNER_STATE_CLOSED;
    }
    if( CL_HANDLE_INVALID_VALUE != hFileNode )
    {
        CL_LOG_CLEANUP(clCntNodeDelete(pFileOwnerEoEntry->hFileTable, 
                                       hFileNode), CL_OK);
    }
    CL_LOG_DEBUG_TRACE(("FileOwner active cnt: %d",
                pFileOwnerEoEntry->activeCnt));
    if( 0 == pFileOwnerEoEntry->activeCnt )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                   CL_OK);

        CL_LOG_CLEANUP(clLogFileOwnerEoDataFinalize(pFileOwnerEoEntry),
                CL_OK);
        *pLockDestroyed = CL_TRUE;
        return CL_OK;
    }

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogFileOwnerFileTableDestroy(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                               ClCntNodeHandleT        hFileNode)
{
    ClRcT  rc = CL_OK;
    ClBoolT lockDestroyed = CL_FALSE;

    CL_LOG_CLEANUP(clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock), CL_OK);

    rc = clLogFileOwnerFileTableDestroyWithLock(pFileOwnerEoEntry,
                                                hFileNode,
                                                &lockDestroyed);
    
    if(!lockDestroyed)
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                       CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogFileOwnerEntryFindNPersist(ClStringT   *fileName,
                                ClStringT   *fileLocation,
                                ClUint32T   numRecords, 
                                ClUint8T    *pRecords)
{
    ClRcT                  rc                 = CL_OK;
    ClLogFileOwnerEoDataT  *pFileOwnerEoEntry = NULL;
    ClLogFileKeyT          *pFileKey          = NULL;
    ClCntNodeHandleT       hFileNode          = CL_HANDLE_INVALID_VALUE;
    ClUint32T              activeCnt          = 0;
    ClLogFileOwnerDataT   *pFileOwnerData     = NULL;

    clLogDebug(CL_LOG_AREA_FILE_OWNER, CL_LOG_CTX_FO_INIT, 
             "Persisting records [%d] for file [%.*s] [%.*s]", 
             numRecords, fileName->length, fileName->pValue, 
             fileLocation->length, fileLocation->pValue);
    /* Get the eo entry */
    rc = clLogFileOwnerEoEntryGet(&pFileOwnerEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    /*
     * If the status is closed, this is the stopping for no more persistence
    */
    if( CL_LOG_FILEOWNER_STATE_CLOSED == pFileOwnerEoEntry->status )
    {
        clLogWarning("FOW", "PER", "FileOwner eo has been already deleted");
        return CL_OK;
    }
    if( CL_HANDLE_INVALID_VALUE == pFileOwnerEoEntry->hFileTable )
    {
        clLogInfo("FOW", "PER", "Log not yet open");
        return CL_OK;
    }
    
    /* Create the filekey from filelocation */
    rc = clLogFileKeyCreate(fileName, fileLocation,
            CL_LOG_STREAM_HDLR_MAX_FILES, &pFileKey);
    if( CL_OK != rc )
    {
        return rc;
    }
    /* Take the outer table lock */
    rc = clOsalMutexLock_L(&pFileOwnerEoEntry->fileTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock(): rc[0x %x]", rc));
        clLogFileKeyDestroy(pFileKey);
        return rc;
    }
    /* Num of files are being get overwritten */
    activeCnt = pFileOwnerEoEntry->activeCnt;

    /* am going inside to flush */
    ++activeCnt;
    pFileOwnerEoEntry->status = CL_LOG_FILEOWNER_STATE_ACTIVE;
    rc = clCntNodeFind(pFileOwnerEoEntry->hFileTable, 
                       (ClCntKeyHandleT) pFileKey, &hFileNode);
    if( CL_OK != rc )
    {
        clLogFileKeyDestroy(pFileKey);
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                CL_OK);
        return rc;
    }
    /* free the key as no more required */
    clLogFileKeyDestroy(pFileKey);
    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode,
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                           CL_OK);
        return rc;
    }
    /* Take the fileEntry lock */
    rc = clOsalMutexLock_L(&pFileOwnerData->fileEntryLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                           CL_OK);
        return rc;
    }
    pFileOwnerEoEntry->activeCnt = activeCnt;
    CL_LOG_DEBUG_TRACE(("streamHdlr active cnt: %d  status %d ",
                pFileOwnerEoEntry->activeCnt, pFileOwnerEoEntry->status));
    /* Leave outer table lock */
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerEoEntry->fileTableLock),
                                 CL_OK);
    /* go and persist the data */
    rc = clLogFileOwnerRecordPersist(pFileOwnerEoEntry, hFileNode,
                                     numRecords, (ClUint8T *) pRecords);
    if( (CL_OK != rc) && (CL_LOG_ERR_FILE_FULL != CL_GET_ERROR_CODE(rc)) )
    {
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock), 
                                     CL_OK);
        clLogFileOwnerFileEntryCntDecrement(pFileOwnerEoEntry, pFileOwnerData, 
                                            hFileNode, CL_HANDLE_INVALID_VALUE);
        return rc;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pFileOwnerData->fileEntryLock), 
                   CL_OK);
    clLogFileOwnerFileEntryCntDecrement(pFileOwnerEoEntry, pFileOwnerData, 
                                        hFileNode, CL_HANDLE_INVALID_VALUE);
    return rc;
}

static ClRcT
clLogFileOwnerRecordPersist(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                            ClCntNodeHandleT        hFileNode, 
                            ClUint32T               numRecords, 
                            ClUint8T                *pRecords)
{
    ClRcT                rc              = CL_OK;
    ClLogFileOwnerDataT  *pFileOwnerData = NULL;
    
    rc = clCntNodeUserDataGet(pFileOwnerEoEntry->hFileTable, hFileNode, 
                              (ClCntDataHandleT *) &pFileOwnerData);
    if( CL_OK != rc )
    {
        clLogError("FILEOWN", "WRITE", "Failed to get the file data from file table rc [0x %x]", rc);
        return rc;
    }
    /* Check the file has been over written or Are these records are going to over write
     * the records
     */
    if( CL_FALSE == pFileOwnerData->pFileHeader->overWriteFlag )
    {
        /* Need to write */
    }

    rc = clLogFileRecordsPersist(pFileOwnerEoEntry, hFileNode, pFileOwnerData, 
                                 numRecords * pFileOwnerData->streamAttr.recordSize,
                                 numRecords, pRecords);
    /*
     * Need to update the last saved rec, this will be part of archiver
     */

    return CL_OK;
}

static ClRcT
clLogFileRecordsPersist(ClLogFileOwnerEoDataT   *pFileOwnerEoEntry,
                        ClCntHandleT            hFileNode,
                        ClLogFileOwnerDataT     *pFileOwnerData,
                        ClUint32T               numBytes,
                        ClUint32T               numRecords,
                        ClUint8T                *pRecords)
{
    ClRcT      rc          = CL_OK;
    ClUint32T  remNumRecs  = 0;
    ClUint32T  numRemBytes = 0;

    clLogTrace("FILEOWN", "WRITE", "[%d] records, [%d] bytes to be written", numRecords,numBytes);
    
    if( pFileOwnerData->pFileHeader->remSize < numBytes )
    {
        remNumRecs = pFileOwnerData->pFileHeader->remSize / pFileOwnerData->streamAttr.recordSize; 
        if( remNumRecs < 1 )
        {
            /* remNumRecs is Zero, the file has become full, need to take action
             * based on file full action policy rotate/wrap/halt 
             */
             rc = clLogFileOwnerFileFullAction(pFileOwnerEoEntry, hFileNode, pFileOwnerData,
                                               numRecords, pFileOwnerData->streamAttr.recordSize,
                                               pRecords);
             if( CL_OK != rc )
             {
                clLogError("FILEOWN", "WRITE", "Failed to do file full action rc[0x %x]", rc);
                return rc;
             }
        }
        else
        {
           /* spilt the num of records to be written and write into two calls */
           numRemBytes = remNumRecs * pFileOwnerData->streamAttr.recordSize;
           rc = clLogFileRecordsPersist(pFileOwnerEoEntry, hFileNode, pFileOwnerData,
                                        numRemBytes, remNumRecs, pRecords);
           if( CL_OK != rc )
           {
                return rc;
           }
           /* Write the remaining records into the file, this two calls are because of
            * variable length recordsize of ASCII record
            */
           rc = clLogFileRecordsPersist(pFileOwnerEoEntry, hFileNode, pFileOwnerData,
                                        (numBytes - numRemBytes), (numRecords - remNumRecs),
                                        pRecords + numRemBytes);
           if( CL_OK != rc )
           {
              return rc;
           }
        }
    }        
    else
    {
        rc = clLogFileOwnerFileWrite(pFileOwnerData, pFileOwnerData->streamAttr.recordSize,
                                &numRecords, pRecords);
        if( CL_OK != rc )
        {
            clLogError("FILEOWN", "WRITE", "Failed to write [%d] records into the file rc[0x %x]", 
                        numRecords, rc);
        }
    }
    return rc;
}

