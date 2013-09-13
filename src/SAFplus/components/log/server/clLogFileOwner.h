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
#ifndef _CL_LOG_STREAM_HDLR_H_
#define _CL_LOG_STREAM_HDLR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clHandleApi.h>
#include <clCntApi.h>
#include <clLogApi.h>
#include <clLogCommon.h>    
#include <clLogFileOwnerEo.h>
#include <xdrClLogStreamAttrIDLT.h>
#include <xdrClLogCompDataT.h>

#define  CL_LOG_STREAM_HDLR_MAX_FILES    128    
    
#define  CL_LOG_STREAM_HDLR_MAX_STREAMS  128    

#define  CL_LOG_FILE_HDR_INIT_PAGES      10

#define  CL_LOG_FILEOWNER_CFG_FILE_SIZE   (CL_LOG_FILE_HDR_INIT_PAGES * getpagesize())    
 
#define  CL_LOG_DEFAULT_CFG_FILE_STRING  "OpenClovis_Log_Configfile"    

#define  CL_LOG_DEFAULT_FILE_STRING      "OpenClovis_Logfile"    

#define  CL_LOG_DEFAULT_FILE_STRING_RESTART "OpenClovis_Logfile (Created on log server restart)"

#define  CL_LOG_FILEOWNER_STREAM_HEAD     320    

#define  CL_LOG_FILEOWNER_FILE_HEAD       322        

#define  CL_LOG_FILEOWNER_COMP_HEAD       324
    
#define  CL_LOG_FILEOWNER_NEXT_ENTRY      326

#define  CL_LOG_FILEOWNER_CFG_HDR_SIZE    CL_LOG_FILEOWNER_NEXT_ENTRY 

#define  CL_LOG_MAX_RECCOUNT_GET(streamAttr) \
         (streamAttr.fileUnitSize / streamAttr.recordSize)

#define  CL_LOG_CURR_FUNIT_CNT_GET(pFileOwnerData) \
         ((pFileOwnerData->pFileHeader->nextRec - 1)/ \
          CL_LOG_MAX_RECCOUNT_GET(pFileOwnerData->streamAttr))

#define  CL_LOG_OPERATING_LEVEL_INC(pFileHeader) \
         pFileHeader->version += 5

#define  CL_LOG_MAX_RECNUM_CALC(streamAttr, maxNumRecords) \
    {\
        maxNumRecords = CL_LOG_MAX_RECCOUNT_GET(streamAttr);\
        if( CL_LOG_FILE_FULL_ACTION_ROTATE == streamAttr.fileFullAction ) \
        {\
            maxNumRecords = (maxNumRecords) * (streamAttr.maxFilesRotated); \
        }\
    }while(0)

#define  CL_LOG_HDR_RECORD_SIZE(streamAttr)  streamAttr.recordSize

typedef enum
{
    CL_LOG_FILE_TYPE_ASCII = 1,
    CL_LOG_FILE_TYPE_BINARY = 2, 
    CL_LOG_FILE_TYPE_INVALID = 3
}ClLogFileUnitTypeT;
    
/*
 * Structure about file config file.
 */
typedef struct
{
    ClUint32T             nextRec;
    ClUint32T             remSize;
    ClUint32T             nOverwritten;
    ClBoolT               overWriteFlag;
    ClUint32T             deleteCnt;
    ClUint32T             version;
    ClUint32T             currFileUnitCnt;
    ClUint32T             maxHdrSize;
    ClUint32T             remHdrSize;
    ClUint32T             numPages;
    ClLogFileUnitTypeT    fileType;
    ClUint16T             nextEntry;
    ClUint16T             streamOffSet; 
    ClUint16T             fileOffSet;
    ClUint16T             compOffSet;
}ClLogFileHeaderT;

typedef enum 
{
    CL_LOG_FILEOWNER_STATUS_INIT   = 1,
    CL_LOG_FILEOWNER_STATUS_WIP    = 2,
    CL_LOG_FILEOWNER_STATUS_CLOSED = 3, 
}ClLogFileOwnerStatusT;

typedef struct
{
    ClLogFileHeaderT     *pFileHeader;
    ClLogStreamAttrIDLT  streamAttr;
    ClLogFilePtrT        fileUnitPtr;
//    ClUint32T            currFileUnitCnt;
    ClCntHandleT         hStreamTable;
    ClOsalMutexId_LT     fileEntryLock;
}ClLogFileOwnerDataT;

typedef struct
{
    ClUint16T              streamId;
    ClHandleT              hFileOwner;   
    ClLogFileOwnerStatusT  status;
}ClLogFileOwnerStreamDataT;

typedef struct
{
    ClCntNodeHandleT  hFileNode;
    ClCntNodeHandleT  hStreamNode;
    ClUint32T         activeCnt;
} ClLogFileOwnerHdlrDataT;

extern ClRcT
clLogFileOwnerStreamCloseEvent(SaNameT              *pStreamName,
                                ClLogStreamScopeT    streamScope,
                                SaNameT              *pStreamScopeNode);
extern ClRcT
clLogFileOwnerCompAddEvent(ClLogCompDataT  *pCompData);

extern void
clLogFileOwnerRecordDeliverCb(ClHandleT  hStream,
                               ClUint64T  sequenceNum,
                               ClUint32T  numRecords,
                               ClPtrT     logRecords);
extern void
clLogFileTableDeleteCb(ClCntKeyHandleT   key, 
                       ClCntDataHandleT  data);
extern ClRcT
clLogFileOwnerFileEntryGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                            ClLogStreamAttrIDLT     *pStreamAttr, 
                            ClBoolT                 restart, 
                            ClCntNodeHandleT        *phFileNode, 
                            ClBoolT                 *pEntryAdd);
extern ClRcT
clLogFileOwnerStreamEntryGet(ClLogFileOwnerDataT  *pFileOwnerData, 
                              SaNameT               *pStreamName, 
                              SaNameT               *pStreamScopeNode, 
                              ClCntNodeHandleT      *phStreamNode,
                              ClBoolT               *pEntryAdd);
extern ClRcT
clLogFileOwnerHdlCreate(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                         ClHandleT             hFileOwner, 
                         ClCntNodeHandleT      hFileNode, 
                         ClCntNodeHandleT      hStreamNode);
extern ClRcT
clLogFileOwnerLocationVerify(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                             ClStringT              *pFileLocation,
                             ClBoolT                *pFileOwner);
extern ClRcT
clLogFileOwnerStreamClose(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                           SaNameT                 *pStreamName,
                           ClLogStreamScopeT       streamScope,
                           SaNameT                 *pStreamScopeNode);
extern ClRcT
clLogFileOwnerHandlerRegister(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                               ClHandleT               hFileOwner, 
                               SaNameT                 *pStreamName,
                               ClLogStreamScopeT       streamScope,
                               SaNameT                 *pStreamScopeNode,
                               ClUint16T               streamId,
                               ClCntNodeHandleT        hFileNode,
                               ClCntNodeHandleT        hStreamNode,
                               ClBoolT                 logRestart);

extern ClRcT
clLogFileOwnerFileEntryCntDecrement(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                    ClLogFileOwnerDataT    *pFileOwnerData, 
                                    ClCntNodeHandleT       hFileNode,
                                    ClHandleT              hFileOwer);
extern ClRcT
clLogFileOwnerStreamEntryRemove(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                ClCntNodeHandleT        hFileNode, 
                                SaNameT                *pStreamName,
                                ClLogStreamScopeT      streamScope, 
                                SaNameT                *pStreamScopeNode);
extern ClRcT
clLogFileOwnerFileTableDestroy(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                ClCntNodeHandleT        hFileNode);

extern ClRcT
clLogFileOwnerFileTableDestroyWithLock(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                       ClCntNodeHandleT        hFileNode,
                                       ClBoolT                 *pLockDestroyed);

extern ClRcT
clLogFileOwnerStreamEntryChkNGet(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry,
                                 ClCntNodeHandleT       hFileNode, 
                                 SaNameT                *pStreamName,
                                 ClLogStreamScopeT      streamScope, 
                                 SaNameT                *pStreamScopeNode,
                                 ClCntNodeHandleT       *phStreamNode);
extern ClRcT
clLogFileOwnerStreamChkNDestroy(ClLogFileOwnerEoDataT  *pFileOwnerEoEntry, 
                                ClLogFileOwnerDataT    *pFileOwnerData, 
                                ClHandleT              hFileOwner, 
                                ClBoolT                decrement);
extern ClRcT
clLogFileOwnerEntryFindNPersist(ClStringT   *fileName,
                                ClStringT   *fileLocation,
                                ClUint32T   numRecords, 
                                ClUint8T    *pRecords);
#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_COMMON_H_ */
