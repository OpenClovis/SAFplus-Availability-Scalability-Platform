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
#ifndef _CL_LOG_CLIENT_HANDLE_H_
#define _CL_LOG_CLIENT_HANDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clHandleApi.h>
#include <clCntApi.h>
#include <clBitmapApi.h>
#include <clLogApi.h>
#include <clLogClientEo.h>
#include <clLogClntFileHdlr.h>

typedef enum
{
    CL_LOG_INIT_HANDLE    = 1,
    CL_LOG_STREAM_HANDLE  = 2,
    CL_LOG_HANDLER_HANDLE = 3,
    CL_LOG_FILE_HANDLE    = 4
} ClLogHandleTypeT;

typedef struct
{
    ClLogHandleTypeT  type;
    ClLogCallbacksT   *pCallbacks;
    ClHandleT         hDispatch;
    ClBitmapHandleT   hStreamBitmap;
    ClBitmapHandleT   hFileBitmap;
} ClLogInitHandleDataT;

typedef struct
{
    ClLogHandleTypeT  type;
    ClHandleT         hLog;
    ClCntNodeHandleT  hClntStreamNode;
    ClLogStreamHandleT         hStream;
} ClLogStreamHandleDataT;

typedef struct
{
    ClLogHandleTypeT          type;
    ClLogHandleT              hLog;
    ClCntNodeHandleT          hClntHandlerNode;
    ClLogStreamHandlerFlagsT  handlerFlag;
    ClBoolT                   status;
    ClIocMulticastAddressT    streamMcastAddr;
} ClLogStreamHandlerHandleDataT;

typedef struct
{
    ClLogHandleTypeT   type;
    ClBoolT            isDelete;
    ClUint64T          startRead;
    ClUint32T          operatingLvl;
    ClLogHandleT       hLog;
    ClLogClntFileKeyT  *pFileKey;
}ClLogClntFileHdlrInfoT;

typedef struct
{
    ClInvocationT       invocation;
    ClLogStreamHandleT  hStream;
    ClRcT               rc;
} ClLogStreamOpenCbDataT;

typedef struct
{
    ClLogStreamHandleT  hStream;
    ClLogFilterT        filter;
} ClLogFilterSetCbDataT;


typedef enum
{
   CL_LOG_STREAM_OPEN_CB = 1,
   CL_LOG_FILTER_SET_CB  = 2,
}ClLogCallbackTypeT;

ClRcT
clLogHandleCheck(ClLogHandleT      hLog,
                 ClLogHandleTypeT  type);
ClRcT
clLogHandleDBGet(ClLogClntEoDataT  **ppClntEoEntry,
                 ClBoolT           *pAddedEntry);
ClRcT
clLogHandleInitHandleCreate(const ClLogCallbacksT  *pCallbacks,
                            ClLogHandleT           *phLog,
                            ClBoolT                *addedEntry);

ClRcT
clLogHandleInitHandleInitialize(ClHandleDatabaseHandleT  hClntHandleDB,
                                ClLogHandleT             hLog,
                                const ClLogCallbacksT    *pCallbacks);
void
clLogHandleCleanupCb(void  *pData);

ClRcT
clLogHandleStreamHandleCreate(ClHandleT         hLog,
                              ClCntNodeHandleT  hClntStreamNode,
                              ClLogHandleT     *phStream);
ClRcT
clLogHandleStreamDataInitialize(ClHandleDatabaseHandleT  hClntHandleDB,
                                ClLogHandleT             hLog,
                                ClLogStreamHandleT       hStream,
                                ClCntNodeHandleT         hClntStreamNode);
ClRcT
clLogHandleInitHandleStreamAdd(ClLogHandleT        hLog,
                               ClLogStreamHandleT  hStream);
ClRcT
clLogHandleStreamHandleClose(ClLogStreamHandleT  hStream,
                             ClBoolT             notifySvr);
ClRcT
clLogHandleInitHandleStreamRemove(ClLogHandleT        hLog,
                                  ClLogStreamHandleT  hStream);
ClRcT
clLogHandleOpenStreamCloseCb(ClBitmapHandleT  hBitmap,
                             ClUint32T        bitNum,
                             void             *pCookie);

ClRcT
clLogHandleInitHandleHandlerRemove(ClLogHandleT        hLog,
                                   ClLogStreamHandleT  hStream);
ClRcT
clLogHandleInitHandleDestroy(ClLogHandleT  hLog);

ClRcT
clLogClntHandleHandlerHandleCreate(ClLogHandleT              hLog,
                                   ClCntNodeHandleT          hClntHandlerNode,
                                   ClIocMulticastAddressT    streamMcastAddr,
                                   ClLogStreamHandlerFlagsT  handlerFlag,
                                   ClLogStreamHandleT        *phStream);
ClRcT
clLogHandleInitHandleHandlerAdd(ClLogHandleT        hLog,
                                ClLogStreamHandleT  hStream);

ClRcT
clLogClntHandleHandlerHandleRemove(ClLogStreamHandleT  hHandler);

ClRcT
clLogClntHandleTypeGet(ClLogHandleT      hLog,
                       ClLogHandleTypeT  *pType);

extern ClRcT
clLogInitHandleCheckNCbValidate(ClLogHandleT hLog);
#ifdef __cplusplus
}
#endif

#endif /* _CL_LOG_CLIENT_HANDLE_H_ */
  
