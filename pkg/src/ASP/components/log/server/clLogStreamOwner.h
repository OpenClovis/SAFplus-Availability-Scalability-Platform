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
/*******************************************************************************
 * File        : clLogStreamOwner.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * FIXME: This header has the structure defined for log message 
 *
 *
 *****************************************************************************/

#ifndef _CL_LOG_STREAMOWNER_H_
#define _CL_LOG_STREAMOWNER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmapApi.h>    
    

#include <clLogCommon.h>    
#include <clLogSvrCommon.h>    
#include <clLogStreamOwnerEo.h>    
#include <xdrClLogStreamAttrIDLT.h>

#define CL_LOG_SO_DSID_START          1
    
typedef enum
{
    CL_LOG_NODE_STATUS_UN_INIT = 1,
    CL_LOG_NODE_STATUS_WIP     = 2,
    CL_LOG_NODE_STATUS_INIT    = 3,
    CL_LOG_NODE_STATUS_REINIT  = 4,
}ClLogNodeStatusT;    
    
typedef struct
{
    ClLogSeverityFilterT  severityFilter;
    ClBitmapHandleT       hMsgIdMap;
    ClBitmapHandleT       hCompIdMap;
}ClLogFilterInfoT;

typedef struct 
{
    ClUint16T         streamId;
    ClLogFilterInfoT        streamFilter;
    ClIocMulticastAddressT  streamMcastAddr;
    ClLogStreamAttrIDLT     streamAttr;
    ClCntHandleT            hCompTable;
    ClLogNodeStatusT        nodeStatus;
    ClOsalMutexId_LT        nodeLock;
    ClOsalCondId_LT         nodeCond;
    ClUint32T               dsId;
    ClBoolT                 isNewStream;
    ClBoolT                 condDelete;
    ClUint32T               openCnt;
    ClUint32T               ackerCnt;
    ClUint32T               nonAckerCnt;
}ClLogStreamOwnerDataT;    

typedef struct
{
    ClIocNodeAddressT  nodeAddr;
    ClUint32T          compId;
    ClUint32T          hash;
}ClLogCompKeyT;

typedef struct
{
    ClUint32T  ackerCnt;
    ClUint32T  refCount;
    ClUint32T  nonAckerCnt;
}ClLogSOCompDataT;

typedef struct
{
    ClHandleT          hDeferIdl;
    ClIocNodeAddressT  nodeAddr;
    ClUint32T          compId;
    ClCntNodeHandleT   hStreamNode;
    ClLogStreamScopeT scope;
}ClLogSOCookieT;    

typedef struct
{
    ClBoolT                   sendFilter;
    ClLogStreamKeyT           *pStreamKey; 
    ClLogFilterInfoT          *pStreamFilter;
    ClLogStreamHandlerFlagsT  handlerFlags;
    ClBoolT                   setFlags;
    ClIocNodeAddressT         nodeAddr;
}ClLogSOCommonCookieT;

extern ClRcT
clLogStreamOwnerEoEntryGet(ClLogSOEoDataT         **ppSoEoEntry,
                           ClLogSvrCommonEoDataT  **ppCommonEoEntry);

extern ClRcT
clLogStreamOwnerLocalBootup(void);

    
extern ClRcT
clLogStreamOwnerLocalShutdown(void);

extern ClRcT
clLogStreamOwnerEoDataFree(void);

extern ClRcT
clLogStreamOwnerGlobalBootup(void);

extern ClRcT
clLogStreamOwnerGlobalShutdown(void);

extern ClRcT
clLogStreamOwnerStreamTableGet(ClLogSOEoDataT     **ppSoEoData, 
                               ClLogStreamScopeT  streamScope);

extern ClRcT
clLogStreamOwnerEntryProcess(ClLogSOEoDataT          *pSoEoData,
                             ClLogStreamOpenFlagsT   openFlags,
                             ClIocNodeAddressT       nodeAddr,
                             ClLogStreamOwnerDataT   *pStreamOwnerData,
                             ClCntNodeHandleT        hStreamOwnerNode,
                             ClUint32T               *pCompId,
                             ClNameT                 *pStreamName,
                             ClLogStreamScopeT       streamScope,
                             ClNameT                 *pStreamScopeNode,
                             ClLogStreamAttrIDLT     *pStreamAttr,
                             ClIocMulticastAddressT  *pMultiCastAddr,
                             ClLogFilterT            *pStreamFilter,
                             ClUint32T               *pAckerCnt,
                             ClUint32T               *pNonAckerCnt, 
                             ClUint16T               *pStreamId);
    
extern ClRcT
clLogStreamOwnerAttrVerifyNGet(ClLogSOEoDataT          *pSoEoData,
                               ClLogStreamOpenFlagsT   openFlags,
                               ClLogStreamOwnerDataT   *pStreamOwnerData,
                               ClLogStreamAttrIDLT     *pStreamAttr,
                               ClIocMulticastAddressT  *pMultiCastAddr,
                               ClLogFilterT            *pStreamFilter,
                               ClUint32T               *pAckerCnt,
                               ClUint32T               *pNonAckerCnt, 
                               ClUint16T               *pStreamId);
extern ClRcT
clLogStreamOwnerOpenMasterOpen(ClLogSOEoDataT         *pSoEoData,
                               ClIocNodeAddressT      nodeAddr,
                               ClLogStreamScopeT      streamScope,
                               ClLogStreamOwnerDataT  *pStreamOwnerData,
                               ClCntNodeHandleT       hStreamOwnerNode,
                               ClUint32T              *pCompId,
                               ClNameT                *pStreamName,
                               ClNameT                *pStreamScope,
                               ClLogStreamAttrIDLT    *pStreamAttr);
extern ClRcT
clLogStreamOwnerEntryUpdate(ClLogSOEoDataT          *pSoEoData,
                            ClNameT                 *pStreamName,
                            ClLogStreamScopeT       streamScope,
                            ClNameT                 *pStreamScope,
                            ClLogStreamOwnerDataT   *pStreamOwnerData,
                            ClLogStreamAttrIDLT     *pStreamAttr,
                            ClIocMulticastAddressT  multiCastAddr,
                            ClLogFilterT            *pStreamFilter,
                            ClUint16T               streamId,
                            ClUint32T               compId);
extern ClRcT
clLogStreamOwnerEntryGet(ClLogSOEoDataT         *pSoEoData,
                         ClLogStreamOpenFlagsT  openFlags,
                         ClNameT                *pStreamName, 
                         ClLogStreamScopeT      streamScope,
                         ClNameT                *pStreamScope,
                         ClCntNodeHandleT       *phStreamOwnerNode,
                         ClLogStreamOwnerDataT  **ppStreamOwnerData,
                         ClBoolT                *pEntryAdd); 
extern ClRcT
clLogStreamOwnerEntryAdd(ClCntHandleT       hStreamOwnerTable,
                         ClLogStreamScopeT  streamScope,
                         ClLogStreamKeyT    *pStreamKey,
                         ClCntNodeHandleT   *phStreamOwnerNode);

extern ClRcT
clLogStreamOwnerCompRefCountGet(ClLogSOEoDataT     *pSoEoData,
                                ClLogStreamScopeT  streamScope,
                                ClCntNodeHandleT   hStreamNode,
                                ClIocNodeAddressT  nodeAddr,
                                ClUint32T          compId,
                                ClUint16T          *pRefCount);
extern ClRcT
clLogStreamOwnerCompEntryDelete(ClLogStreamOwnerDataT  *pStreamOwnerData, 
                                ClIocNodeAddressT      nodeAddr,
                                ClUint32T              compId,
                                ClUint32T              *pTableSize);
extern ClRcT
clLogStreamOwnerEntryChkNDelete(ClLogSOEoDataT     *pSoEoEntry,
                                ClLogStreamScopeT  streamScope,
                                ClCntNodeHandleT   hStreamOwnerNode);

extern ClRcT
clLogStreamOwnerMAVGResponseProcess(ClNameT                *pStreamName,
                                    ClNameT                *pStreamScope,
                                    ClLogStreamAttrIDLT    *pStreamAttr,
                                    ClIocMulticastAddressT *pMultiCastAddr,
                                    ClUint16T              *pStreamId,
                                    ClLogFilterT           *pFilter, 
                                    ClUint32T               *pAckerCnt,
                                    ClUint32T               *pNonAckerCnt, 
                                    ClRcT                  retCode,
                                    ClLogSOCookieT         *pCookie);

extern ClRcT
clLogStreamOwnerInfoCopy(ClLogStreamOwnerDataT   *pStreamOwnerData, 
                         ClStringT               *pFileName,
                         ClStringT               *pFileLocation,
                         ClIocMulticastAddressT  *pStreamMcastAddr,
                         ClUint16T               *pStreamId);

extern ClRcT
clLogStreamOwnerNodeGet(ClLogSOEoDataT  *pSoEoData,
                        ClLogStreamScopeT      streamScope,
                        ClLogStreamKeyT  *pStreamKey,
                        ClCntNodeHandleT *phStreamOwnerNode);

extern ClRcT
clLogSOUnlock(ClLogSOEoDataT  *pSoEoData,
              ClLogStreamScopeT      streamScope);
extern ClRcT
clLogSOLock(ClLogSOEoDataT *pSoEoData, 
              ClLogStreamScopeT      streamScope);

extern ClRcT
clLogIdlHandleInitialize(ClIocAddressT mastAddr, ClHandleT *phdl);

extern ClRcT
clLogStreamOwnerCompEntryAdd(ClLogStreamOwnerDataT  *pStreamOwnerData,
                             ClIocNodeAddressT      nodeAddr,
                             ClUint32T              compId);
extern ClRcT
clLogStreamOwnerOpenCleanup(ClNameT *pStreamName, ClNameT *pStreamScopeNode, ClLogSOCookieT  *pCookie);

extern void 
clLogStreamOwnerFilterFinalize(ClLogFilterInfoT  *pFilter);

extern ClRcT
clLogStreamOwnerFilterInit(ClLogFilterInfoT  *pFilter);

extern ClRcT
clLogStreamOwnerDataUpdate(ClLogSOEoDataT         *pSoEoEntry,
                           ClLogStreamKeyT        *pStreamKey,
                           ClLogStreamOwnerDataT  *pStreamOwnerData, 
                           ClLogStreamScopeT      streamScope,
                           ClLogFilterFlagsT      filterFlags,
                           ClLogFilterT           *pLogFilter);
extern ClRcT
clLogStreamOwnerSvrIntimate(ClCntKeyHandleT   key,
                            ClCntDataHandleT  data, 
                            ClCntArgHandleT   arg,
                            ClUint32T         size);
extern ClRcT
clLogStreamOwnerFilterModify(ClLogFilterFlagsT      filterFlags,
                             ClLogStreamOwnerDataT  *pStreamOwnerData,  
                             ClLogFilterT           *pNewFilter);
extern ClRcT
clLogStreamOwnerFilterAssign(ClUint32T        numBytes,
                             ClUint8T         *pSetMap,
                             ClBitmapHandleT  *phBitmap);

    
extern ClRcT
clLogFilterMergeAdd(ClUint32T        numBytes,
                    ClUint8T         *pSetMap,
                    ClBitmapHandleT  *phBitmap);

extern ClRcT
clLogFilterMergeDelete(ClUint32T        numBytes,
                       ClUint8T         *pSetMap,
                       ClBitmapHandleT  hBitmap);

extern ClRcT
clLogFilterFormatConvert(ClLogFilterInfoT  *pStoredFilter,
                         ClLogFilterT      *pPassedFilter);
extern ClRcT
clLogStreamOwnerCloseMasterNotify(ClNameT    *pStreamName,
                                  ClNameT    *pStreamScopeNode, 
                                  ClStringT  *pFileName,
                                  ClStringT  *pFileLocation);
extern ClRcT
clLogStreamOwnerCompdownStateUpdate(ClIocNodeAddressT  nodeAddr, 
                                    ClUint32T          compId);

extern ClRcT
clLogStreamOwnerNodedownStateUpdate(ClIocNodeAddressT  nodeAddr); 

extern ClRcT
clLogStreamOwnerHdlrCntUpdate(ClLogStreamOwnerDataT     *pStreamOwnerData,
                              ClIocNodeAddressT         nodeAddr, 
                              ClLogStreamHandlerFlagsT  handlerFlags,
                              ClBoolT                   setFlags);
extern ClRcT
clLogStreamOwnerHdlrCntClear(ClLogStreamOwnerDataT  *pStreamOwnerData,
                             ClIocNodeAddressT      nodeAddr);
extern ClRcT
clLogStreamOwnerMulticastCloseNotify(ClIocMulticastAddressT  streamMcastAddr);

#ifdef __cplusplus
}
#endif
#endif /*_CL_LOG_SERVER_H_*/
