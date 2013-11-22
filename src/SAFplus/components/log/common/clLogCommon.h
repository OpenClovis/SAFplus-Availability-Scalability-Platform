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
 /*******************************************************************************
 * File        : clLogCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This header contains all the macros shared by the client and the server.
 *
 *
 *
  *****************************************************************************/

#ifndef _CL_LOG_COMMON_H_
#define _CL_LOG_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <fcntl.h>
#include <sys/uio.h>
#include <clIocApi.h>

#include <xdrClLogStreamAttrIDLT.h>
#include <clCntApi.h>
#include <clEoApi.h>
#include <clLogApi.h>
#include <clLogErrors.h>
#include <clLogDebug.h>
#include <clLogOsal.h> /*FIXME: This will go */

  /* Default values for the XML file */
#define  CL_LOG_DEFAULT_SCOPE         CL_LOG_STREAM_GLOBAL
#define  CL_LOG_DEFAULT_SCOPE_STR     "GLOBAL"
#define  CL_LOG_DEFAULT_FF_ACTION_STR "WRAP"
#define  CL_LOG_DEFAULT_FF_ACTION     CL_LOG_FILE_FULL_ACTION_WRAP
#define CL_LOG_COMPID_CLASS 0x3FF
#define LOG_ASCII_ENDIAN_LEN 1
#define LOG_ASCII_SEV_LEN 2
#define LOG_ASCII_HDR_LEN 4
#define LOG_ASCII_DATA_LEN 10
#define _LOG_STR(L) #L
#define __LOG_STR(L) _LOG_STR(L)
#define LOG_ASCII_ENDIAN_FMT "%"__LOG_STR(LOG_ASCII_ENDIAN_LEN)"u"
#define LOG_ASCII_SEV_FMT "%"__LOG_STR(LOG_ASCII_SEV_LEN)"u"
#define LOG_ASCII_HDR_FMT LOG_ASCII_ENDIAN_FMT LOG_ASCII_SEV_FMT
#define LOG_ASCII_HDR_LEN_FMT "%"__LOG_STR(LOG_ASCII_HDR_LEN)"u"
#define LOG_ASCII_DATA_LEN_FMT "%"__LOG_STR(LOG_ASCII_DATA_LEN)"u"
#define LOG_ASCII_DATA_LEN_DELIMITER_FMT LOG_ASCII_DATA_LEN_FMT __LOG_DATA_DELIMITER
#define LOG_DATA_DELIMITER_LEN (ClUint32T)(sizeof(__LOG_DATA_DELIMITER)-1)
#define LOG_DATA_DELIMITER_FMT "%*c%*c"
#define __LOG_DATA_DELIMITER "##"
#define LOG_ASCII_METADATA_LEN ( LOG_ASCII_ENDIAN_LEN + LOG_ASCII_SEV_LEN + LOG_ASCII_HDR_LEN + LOG_ASCII_DATA_LEN + LOG_DATA_DELIMITER_LEN)    
#define LOG_ASCII_MIN_REC_SIZE (LOG_ASCII_METADATA_LEN + 10)

/*
 * Usage: CL_LOG_PARAM_CHK((NULL == ptr), CL_ERR_NULL_PTR)
 */
#define CL_LOG_PARAM_CHK(cond, err)                     \
    do                                                  \
    {                                                   \
        if( (cond) )                                    \
        {                                               \
            CL_LOG_DEBUG_ERROR((#cond " returning"));   \
            return (err);                               \
        }                                               \
    }while( 0 )

/*
 * Usage: CL_LOG_CLEANUP((clBitmapDestroy(ptr)), CL_OK)
 */
#define CL_LOG_CLEANUP(fn, expect)                          \
    do                                                      \
    {                                                       \
        ClRcT ret = (fn);                                   \
        if( ret != (expect) )                               \
        {                                                   \
            CL_LOG_DEBUG_ERROR ((#fn " rc[0x %x]", ret));   \
        }                                                   \
    }while( 0 )

#define CL_LOG_RECORD_WRITE_INPROGRESS   1
#define CL_LOG_RECORD_WRITE_COMPLETE    2
#define CL_LOG_STREAM_HEADER_UPDATE_INPROGRESS 1 
#define CL_LOG_STREAM_HEADER_UPDATE_COMPLETE 2 
#define CL_LOG_STREAM_HEADER_STRUCT_ID      12374 
#define CL_LOG_RECORD_HEADER_SIZE       20
#define CL_LOG_MAX_RETRIES              3
/* FIXME: The Data below should be read from configuration */
#define CL_LOG_MAX_FLUSHERS             256
#define CL_LOG_MAX_MSGS                 1024
#define CL_LOG_DEFAULT_SEVERITY         (CL_LOG_SEV_DEBUG)
#define CL_LOG_DEFAULT_SEVERITY_FILTER  ((1 << CL_LOG_DEFAULT_SEVERITY)-1)
/*End of FIXME */

#define CL_LOG_SHM_MODE                (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CL_LOG_SHM_OPEN_FLAGS          (O_RDWR)
#define CL_LOG_SHM_CREATE_FLAGS        (O_RDWR | O_CREAT)
#define CL_LOG_SHM_EXCL_CREATE_FLAGS   (O_RDWR | O_CREAT | O_EXCL)
#ifdef MAP_POPULATE
#define CL_LOG_MMAP_FLAGS              (MAP_SHARED | MAP_POPULATE)
#else
#define CL_LOG_MMAP_FLAGS              (MAP_SHARED)
#endif
#define CL_LOG_MMAP_PROT_FLAGS         (PROT_READ | PROT_WRITE)

#define  CL_LOG_DEFAULT_FILE_SIZE          20 * 4096
#define  CL_LOG_DEFAULT_RECORD_SIZE        300
#define  CL_LOG_DEFAULT_HA_PROPERTY        CL_FALSE
#define  CL_LOG_DEFAULT_FILE_FULL_ACTION   CL_LOG_FILE_FULL_ACTION_WRAP
#define  CL_LOG_DEFAULT_MAX_FILES_ROTATED  0
#define  CL_LOG_DEFAULT_FLUSH_FREQ         64
#define  CL_LOG_DEFAULT_FLUSH_INTERVAL     10000000000
#define  CL_LOG_DEFAULT_LOW_WATER_MARK     20
#define  CL_LOG_DEFAULT_HIGH_WATER_MARK    80
#define  CL_LOG_INVALID_FD                 -1
#define  CL_LOG_DEFAULT_STREAMID           ~0x0
#define  CL_LOG_DEFAULT_MCASTADDR          ~0x0
#define CL_LOG_DEFAULT_FILECONFIG 		"clLog.xml"

#define CL_LOG_HEADER_SIZE_GET(maxMsgs, maxComps)\
        sizeof(ClLogStreamHeaderT) + \
        (maxMsgs / CL_BITS_PER_BYTE + 1) + \
        (maxComps / CL_BITS_PER_BYTE + 1) \

#define  CL_LOG_MCAST_PROTO       0x36    
#define  CL_LOG_MCAST_CLOSE       0x37    

#if 0
#define  clHeapAllocate         clOsalMalloc
#define  clHeapCalloc(x,y)      clOsalCalloc((x) * (y))
#define  clHeapFree             clOsalFree    
   
#define  ClBufferHandleT        ClBufferMessageHandleT
    
#define  clBufferCreate         clBufferMessageCreate
#define  clBufferDelete         clBufferMessageDelete
#define  clBufferClear          clBufferMessageClear
#define  clBufferNBytesWrite    clBufferMessageNBytesWrite
#define  clBufferNBytesRead     clBufferMessageNBytesRead
#define  clBufferLengthGet      clBufferMessageLengthGet
#endif
typedef FILE *                ClLogFilePtrT;
extern ClBoolT gClLogSvrExiting;
typedef enum
{
    CL_LOG_STREAM_HALT = 0,
    CL_LOG_STREAM_CLOSE,
    CL_LOG_STREAM_ACTIVE, 
    CL_LOG_STREAM_THREAD_EXIT
} ClLogStreamStatusT;

typedef struct
{
    ClUint16T           streamId;
    ClUint16T           struct_id; //This Field is initialized with CL_LOG_STREAM_HEADER_STRUCT_ID and shall not modify it during runtime
    ClUint32T           recordSize;
    ClUint32T           recordIdx;
    ClUint32T           startAck;
    ClUint32T           flushCnt;
    ClUint32T           numOverwrite;
    ClUint32T           flushFreq;
    ClTimeT             flushInterval;
    ClIocAddressT       streamMcastAddr;
    ClLogStreamStatusT  streamStatus;
    ClOsalMutexId_LT    lock_for_join;
    ClOsalCondId_LT     cond_for_join;
#ifndef POSIX_BUILD
    ClOsalMutexId_LT    shmLock;
    ClOsalCondId_LT     flushCond;
#endif
    ClLogFilterT        filter;
    ClUint16T           maxMsgs;
    ClUint16T           maxComps;
    ClUint32T           maxRecordCount;
    ClUint32T           shmSize;
    ClUint64T           sequenceNum;
#ifndef VXWORKS_BUILD
    ClOsalMutexId_LT    sharedSem;
    ClOsalMutexId_LT    flusherSem;
#endif
    ClUint8T            update_status;
    /* The following members are only to used to reset above member variables
     * when stream header corruption is detected
     */
    ClUint16T           streamId_sec;
    ClUint32T           recordSize_sec;
    ClUint32T           flushFreq_sec;
    ClTimeT             flushInterval_sec;
    ClIocAddressT       streamMcastAddr_sec;
    ClLogStreamStatusT  streamStatus_sec;
    ClLogFilterT        filter_sec;
    ClUint16T           maxMsgs_sec;
    ClUint16T           maxComps_sec;
    ClUint32T           maxRecordCount_sec;
    ClUint32T           shmSize_sec;
} ClLogStreamHeaderT;

typedef struct
{
    ClNameT    streamName;
    ClNameT    streamScopeNode;
    ClUint32T  hash;
} ClLogStreamKeyT;

typedef struct
{
    ClNameT              streamName;
    ClLogStreamScopeT    streamScope;
    ClNameT              streamScopeNode;
    ClLogStreamHandleT   hStream;
} ClLogStdStreamDataT;

typedef struct
{
    ClCharT    *pCompName;
    ClUint16T  clntId; 
} ClLogASPCompMapT;

typedef enum 
{
    CL_LOG_SEM_MODE = 1,
    CL_LOG_MUTEX_MODE = 2,
}ClLogLockModeT;

extern const ClCharT        gStreamScopeGlobal[];
extern ClLogStdStreamDataT  stdStreamList[];
extern ClLogASPCompMapT     aspCompMap[];

extern ClUint32T            nStdStream;
extern ClUint32T            nLogAspComps;
struct ClLogClntStreamData;
struct ClLogSvrStreamData;
ClRcT 
clLogPerennialStreamsDataGet(ClLogStreamAttrIDLT  *pStreamAttr,
                             ClUint32T            nExpected);
    
ClRcT 
clLogConfigDataGet(ClUint32T   *pMaxStreams,
                   ClUint32T   *pMaxComps,
                   ClUint32T   *pMaxShmPages,
                   ClUint32T   *pMaxRecordsInPacket);
                   
ClRcT
clLogMasterAddressGet(ClIocAddressT  *pMasterAddr);

ClRcT
clLogStreamScopeGet(ClNameT            *pNodeName,
                    ClLogStreamScopeT  *pScope);

ClInt32T
clLogStreamKeyCompare(ClCntKeyHandleT  key1,
                      ClCntKeyHandleT  key2);

ClUint32T
clLogStreamHashFn(ClCntKeyHandleT key);

ClRcT
clLogStreamKeyCreate(ClNameT          *pStreamName,
                     ClNameT          *pNodeName,
                     ClUint32T        maxStreams,
                     ClLogStreamKeyT  **ppStreamKey);

void
clLogStreamKeyDestroy(ClLogStreamKeyT  *pStreamKey);

ClRcT
clLogShmGet(ClCharT   *shmName,
            ClInt32T  *pShmFd);

ClRcT
clLogStreamShmSegInit(ClNameT                 *pStreamName,
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
                      ClLogStreamHeaderT      **ppSegHeader);

void clLogStreamHeaderReset(ClLogStreamHeaderT *pStreamHeader); 

ClRcT
clLogFilterAssign(ClLogStreamHeaderT  *pHeader,
                  ClLogFilterT        *pStreamFilter);

ClRcT
clLogFilterModify(ClLogStreamHeaderT  *pHeader,
                  ClLogFilterT        *pStreamFilter,
                  ClLogFilterFlagsT    flags);

ClRcT
clLogPSharedMutexCreate(ClCharT *pShmName, ClLogStreamHeaderT  *pHeader);

ClRcT
clLogPSharedCondCreate(ClLogStreamHeaderT *pHeader);

ClRcT
clLogShmNameCreate(ClNameT    *pStreamName,
                   ClNameT    *pStreamScopeNode,
                   ClStringT  *pShmName);

ClRcT
clLogShmNameDestroy(ClStringT  *pStreamName);

ClRcT
clLogCompNamePrefixGet(ClNameT   *pCompName,
                       ClCharT   **ppCompPrefix);

ClRcT
clLogFileCreat(ClCharT  *fileName, ClLogFilePtrT  *pFp);

ClRcT
clLogFileOpen_L(ClCharT  *fileName, ClLogFilePtrT  *pFp);
    
ClRcT
clLogFileClose_L(ClLogFilePtrT fp);

ClRcT
clLogFileWrite(ClLogFilePtrT  fp, void *pData, ClUint32T  size);

ClRcT
clLogFileIOVwrite(ClLogFilePtrT  fp,
                  struct iovec   *pIov,
                  ClUint32T      numRecords,
                  ClUint32T      *pNumOfBytes);

ClRcT
clLogFileRead(ClLogFilePtrT  fp, void *pData, ClUint32T *pNumOfBytes);

ClRcT
clLogFileSeek(ClLogFilePtrT  fp, ClUint32T  offSet);

ClRcT
clLogFileNo(ClLogFilePtrT  fp, ClHandleT *fd);

ClRcT
clLogFileUnlink(const ClCharT *name);

ClRcT
clLogSymLink(ClCharT *oldFileName, ClCharT  *newFileName);

ClRcT
clLogReadLink(ClCharT   *softFileName, 
              ClCharT   *newFileName, 
              ClInt32T *pFileNameLength);
ClRcT
clLogAddressForLocationGet(ClCharT        *pStr, 
                           ClIocAddressT  *pDestAddr,
                           ClBoolT        *pIsLogical);
extern ClLogStreamScopeT
clLogStr2StreamScopeGet(ClCharT  *str);

extern ClLogFileFullActionT
clLogStr2FileActionGet(ClCharT *str);

ClRcT clLogStreamMutexModeSet(ClOsalSharedMutexFlagsT mode);

ClRcT clLogServerStreamSharedMutexInit(struct ClLogSvrStreamData *pStreamData, ClStringT *pShmName);

ClRcT clLogClientStreamSharedMutexInit(struct ClLogClntStreamData *pStreamData, ClStringT *pShmName);

ClRcT clLogClientStreamMutexLock(struct ClLogClntStreamData *pClntData);

ClRcT clLogClientStreamSignalFlusher(struct ClLogClntStreamData *pClntData,
                                     ClOsalCondIdT cond
                                     );

ClRcT clLogClientStreamMutexUnlock(struct ClLogClntStreamData *pClntData);

ClRcT clLogServerStreamMutexLock(struct ClLogSvrStreamData *pSvrData);

ClRcT clLogServerStreamMutexLockFlusher(struct ClLogSvrStreamData *pSvrData);

ClRcT clLogServerStreamMutexUnlock(struct ClLogSvrStreamData *pSvrData);

ClRcT clLogServerStreamCondWait(struct ClLogSvrStreamData *pSvrData,
                                ClOsalCondIdT cond,
                                ClTimerTimeOutT timeout
                                );

ClRcT clLogServerStreamSignalFlusher(struct ClLogSvrStreamData *pSvrData,
                                     ClOsalCondIdT cond
                                     );

ClRcT clLogClientStreamMutexDestroy(struct ClLogClntStreamData *pClntData);

ClRcT clLogClientStreamFlusherMutexDestroy(struct ClLogClntStreamData *pClntData);

ClRcT clLogServerStreamMutexDestroy(struct ClLogSvrStreamData *pSvrData);

ClRcT clLogServerStreamFlusherMutexDestroy(struct ClLogSvrStreamData *pSvrData);

ClRcT
clLogFileLocationFindNGet(ClCharT    *recvFileLoc,
                          ClStringT  *destFileLoc);
ClLogLockModeT 
clLogLockModeGet(void);

#ifdef VXWORKS_BUILD
ClRcT
clLogSharedSemGet(const ClCharT *pShmName, const ClCharT *pSuffix, ClOsalSemIdT *pSemId);
#endif

ClUint32T
clLogDefaultStreamSeverityGet(ClNameT *pStreamName);

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_COMMON_H_*/
