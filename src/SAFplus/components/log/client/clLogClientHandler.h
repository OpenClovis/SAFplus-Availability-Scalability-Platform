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
#ifndef _CL_LOG_CLIENT_HANDLER_H_
#define _CL_LOG_CLIENT_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clBitmapApi.h>
#include <clLogCommon.h>    
#include <clLogClientEo.h>    

typedef  enum
{
    CL_LOG_CLNTHDLR_INACTIVE     = 1,
    CL_LOG_CLNTHDLR_PERSISTING   = 2,
    CL_LOG_CLNTHDLR_DEREGISTERED = 3
}ClLogClntHdlrStatusT;

typedef struct
{
    ClBitmapHandleT       hStreamBitmap;
    SaNameT               streamName;
    ClLogStreamScopeT     streamScope;
    SaNameT               nodeName;
    ClCntHandleT          hFlusherTable;     
    ClLogClntHdlrStatusT  status;
    ClUint32T             persistingCnt;
} ClLogClntHandlerNodeT;

typedef struct
{
    ClIocNodeAddressT  srcAddr;
    ClUint32T          seqNum;
    ClHandleT          hFlushCookie; 
    ClUint32T          refCount;
    ClUint32T          hash;
}ClLogClntFlushKeyT;

typedef struct
{
    ClCntNodeHandleT    hFlushNode;
    ClLogClntFlushKeyT  key;
    ClUint32T           numRecords;
    ClUint8T            *pRecords;
} ClLogClntHdlrRecvDataT;

extern ClRcT
clLogClntHandlerRegister(ClLogHandleT              hLog,
                         SaNameT                   *pStreamName,
                         ClLogStreamScopeT         streamScope,
                         SaNameT                   *pNodeName,
                         ClLogStreamHandlerFlagsT  handlerFlags,
                         ClIocMulticastAddressT    mcastAddr,
                         ClLogStreamHandleT        *phStream);

extern ClRcT
clLogClntHandlerEntryGet(ClLogClntEoDataT        *pClntEoEntry,
                         SaNameT                 *pStreamName,
                         ClLogStreamScopeT       streamScope, 
                         SaNameT                 *pNodeName,
                         ClIocMulticastAddressT  mcastAddr,
                         ClCntNodeHandleT        *phHandlerNode,
                         ClBoolT                 *pAddedEntry);

extern ClRcT
clLogClntHandlerEntryAdd(ClCntHandleT            hStreamHandlerTable,
                         ClIocMulticastAddressT  *pMcastAddr,
                         SaNameT                 *pStreamName,
                         ClLogStreamScopeT       streamScope,
                         SaNameT                 *pNodeName,
                         ClCntNodeHandleT        *phHandlerNode);
extern ClRcT
clLogStreamMcastRegister(ClIocMulticastAddressT  *pStreamMcastAddr);

extern ClRcT
clLogStreamMcastReceive(ClIocCommPortHandleT   commPort,
                        ClIocMulticastAddressT streamMcastAddr);

extern ClRcT
clLogClntPackageReceived(ClBufferHandleT         msg,
                         ClUint8T                protoType, 
                         ClIocMulticastAddressT  mcastAddr,
                         ClBoolT                 *pExit);

extern ClRcT
clLogClntHandlerDeregister(ClLogStreamHandleT  hStream, 
                           ClLogHandleT        *phLog);


#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_CLIENT_HANDLER_H_ */
