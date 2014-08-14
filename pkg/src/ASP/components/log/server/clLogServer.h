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
#ifndef _CL_LOG_SERVER_H_
#define _CL_LOG_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clLogApi.h>
#include <clLogSvrEo.h>    
#include <xdrClLogStreamAttrIDLT.h>
    
#define CL_LOG_SVR_SHMSIZE_GET(numPages, shmSize) \
    do{\
        ClInt32T pageSize = 0; \
        rc      = clOsalPageSizeGet_L(&pageSize);\
        shmSize = numPages * pageSize;\
    }while(0)
        


#define  CL_LOG_DEFAULT_PORTID              0xFFFFFFFE

typedef enum
{
    CL_LOG_FLUSH_INTERVAL_STATUS_ACTIVE = 1, 
    CL_LOG_FLUSH_INTERVAL_STATUS_CLOSE  = 2,
} ClLogFlushIntervalStatusT;
    
struct ClLogSvrStreamData
{
    ClUint32T           dsId;
    ClCntHandleT        hComponentTable;
    ClStringT           shmName;
    ClLogStreamHeaderT  *pStreamHeader;
    ClUint8T            *pStreamRecords;
    ClOsalTaskIdT       flusherId;
    ClUint32T           seqNum;
    ClUint32T           ackersCount;
    ClUint32T           nonAckersCount;
    ClIocNodeAddressT   fileOwnerAddr;
    ClStringT           fileName;
    ClStringT           fileLocation;
    ClLogFlushIntervalStatusT flushIntervalThreadStatus;
    ClOsalCondId_LT     flushIntervalCondvar;
    ClOsalMutexId_LT    flushIntervalMutex;
#ifdef VXWORKS_BUILD
    /*
     * Cache shared sem ids for named sems
     */
    ClOsalSemIdT        sharedSemId;
    ClOsalSemIdT        flusherSemId;
#endif    
};

typedef struct ClLogSvrStreamData ClLogSvrStreamDataT;

typedef struct
{
    ClUint32T   componentId;
    ClUint32T  hash;
} ClLogSvrCompKeyT;

typedef struct
{
    ClUint32T   refCount;
    ClIocPortT  portId;
}ClLogSvrCompDataT;

typedef struct
{
    ClNameT            *pStreamName;
    ClLogStreamScopeT  *pStreamScope;
    ClNameT            *pStreamScopeNode;
    ClLogFilterT       *pFilter;
}ClLogSvrFilterCbDataT;

typedef struct
{
    ClIdlHandleT  hDeferIdl;
    ClUint32T     compId;
}ClLogStreamOpenCookieT;

extern ClRcT
clLogSvrDataInit(void);

extern ClRcT
clLogTimerCreate(void);

extern ClRcT
clLogSvrPerennialStreamsOpen(ClUint32T  *pErrIndex);

extern ClRcT
clLogSvrStdStreamClose(ClUint32T  tblSize);

extern ClRcT
clLogSvrBootup(ClBoolT  *pLogRestart);

extern ClRcT
clLogGlobalCkptGet(void);

extern ClRcT
clLogTimerCallback(CL_IN  void  *pData);

extern ClRcT
clLogSvrStreamTableGet(CL_IN ClLogSvrEoDataT  *pSvrEoEntry);

extern ClRcT 
clLogSvrStreamEntryGet(CL_IN   ClLogSvrEoDataT   *pSvrEoEntry,
		               CL_IN   ClNameT           *pStreamName,
		               CL_IN   ClNameT           *pStreamScopeNode,
                       CL_IN   ClBoolT           createFlag,
		               CL_OUT  ClCntNodeHandleT  *pSvrStreamNode,
                       CL_OUT  ClBoolT           *pAddedEntry);
extern ClRcT
clLogSvrStreamTableCreate(CL_IN  ClLogSvrEoDataT   *pSvrEoData);

extern ClRcT
clLogSvrShmNameCreate(CL_IN   ClNameT    *pStreamName,
                      CL_IN   ClNameT    *pStreamScopeNode,
                      CL_OUT  ClStringT  *pShmName);

extern ClRcT
clLogSvrShmNameDelete(ClStringT  *pShmName);

extern ClRcT
clLogSvrShmMapAndGet(CL_IN  ClOsalShmIdT  fd, 
                     CL_IN  ClUint8T      **pStreamHeader);

extern ClRcT
clLogSvrFlusherThreadCreateNStart(ClLogSvrStreamDataT  *pSvrStreamData,
                                  ClOsalTaskIdT        *pTaskId);

extern  ClRcT
clLogSvrStreamEntryAdd(CL_IN   ClLogSvrEoDataT        *pSvrEoEntry,
                       CL_IN   ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                       CL_IN   ClLogStreamKeyT        *pStreamKey,
                       CL_IN   ClStringT              *pShmName,
                       CL_OUT  ClCntNodeHandleT       *pSvrNode);
extern  ClRcT
clLogSvrIdlHandleInitialize(CL_IN  ClLogStreamScopeT  streamScope,
                            CL_OUT ClIdlHandleT       *phLogIdl);
extern ClRcT
clLogShmCreateAndFill(ClStringT               *pShmName, 
                      ClUint32T               shmSize, 
                      ClUint16T               streamId,
                      ClUint32T               componentId,
                      ClIocMulticastAddressT  *pStreamMcastAddr,
                      ClLogFilterT            *pStreamFilter,
                      ClLogStreamAttrIDLT     *pStreamAttr,
                      ClLogStreamHeaderT      **ppStreamHeader);
extern ClRcT
clLogShmMapAndFill(CL_IN   ClOsalShmIdT            fd,
                   CL_IN   ClUint16T               *pStreamId,
                   CL_IN   ClLogFilterT            *pStreamFilter,
                   CL_IN   ClIocMulticastAddressT  *pStreamMCASTAddr,
                   CL_IN   ClLogStreamAttrIDLT     *pStreamAttr,
                   CL_IN   ClUint32T               shmSize,
                   CL_OUT  ClPtrT                  *ppStreamHeader);
                 
extern void
clLogSvrStreamEntryDeleteCb(ClCntKeyHandleT   userKey,
                            ClCntDataHandleT  userData);
extern ClRcT
clLogSvrTimerDeleteNStart(ClLogSvrEoDataT  *pSvrEoEntry,
                          void             *pData);

extern ClRcT
clLogSvrEoDataFree(void);

extern ClRcT
clLogSvrDebugFilterSet(ClNameT *pStreamName, ClNameT *pStreamNodeName, ClLogFilterT *pFilter,
                       ClLogFilterFlagsT flags);

extern ClRcT
clLogSvrDebugSeverityGet(ClNameT *pStreamName, ClNameT *pStreamNodeName, ClLogSeverityFilterT *pSeverityFilter);

extern ClRcT
clLogSvrCompRefCountIncrement(CL_IN  ClLogSvrEoDataT        *pSvrEoEntry,
                              CL_IN  ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                              CL_IN  ClCntNodeHandleT       hSvrStreamNode,
			                  CL_IN  ClUint32T              componentId,
                              CL_IN  ClIocPortT             portId);

extern ClRcT
clLogSvrCompRefCountDecrement(CL_IN   ClLogSvrEoDataT     *pSvrEoEntry,
                              CL_IN   ClCntNodeHandleT    svrStreamNode,
                              CL_IN   ClUint32T           componentId,
                              CL_OUT  ClUint16T           *pTableStatus);

#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_SERVER_H_*/
