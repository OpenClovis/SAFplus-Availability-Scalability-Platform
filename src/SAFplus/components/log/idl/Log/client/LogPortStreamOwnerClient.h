
/*********************************************************************
* ModuleName  : idl
*********************************************************************/
/*********************************************************************
* Description : This file contains the declartions for client stub
*               routines
*     
* THIS FILE IS AUTO-GENERATED BY OPENCLOVIS IDE. EDIT THIS FILE AT
* YOUR OWN RISK. ANY CHANGE TO THIS FILE WILL BE OVERWRITTEN ON
* RE-GENERATION.
*     
*********************************************************************/
#ifndef _LOG_PORT_STREAM_OWNER_CLIENT_H_
#define _LOG_PORT_STREAM_OWNER_CLIENT_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <clXdrApi.h>
#include <clIdlApi.h>
#include "../clientIDDefinitions.h"
#include "xdrClLogFilterT.h"
#include "xdrClLogStreamAttrIDLT.h"
#include "xdrClLogCompDataT.h"
#include "xdrClLogStreamScopeT.h"


typedef void (*LogClLogStreamOwnerStreamOpenAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN ClUint8T  logOpenFlags, CL_IN ClUint32T  nodeAddr, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT* pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_INOUT ClUint32T* compId, CL_INOUT ClLogStreamAttrIDLT_4_0_0* pStreamAttr, CL_OUT ClUint64T* pStreamMastAddr, CL_OUT ClLogFilterT_4_0_0* pStreamFilter, CL_OUT ClUint32T* pAckerCnt, CL_OUT ClUint32T* pNonAckerCnt, CL_INOUT ClUint16T* pStreamId, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerStreamOpenAsyncCallbackT_4_0_0 LogClLogStreamOwnerStreamOpenAsyncCallbackT;

ClRcT clLogStreamOwnerStreamOpenClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN ClUint8T  logOpenFlags, CL_IN ClUint32T  nodeAddr, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT* pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_INOUT ClUint32T* compId, CL_INOUT ClLogStreamAttrIDLT_4_0_0* pStreamAttr, CL_OUT ClUint64T* pStreamMastAddr, CL_OUT ClLogFilterT_4_0_0* pStreamFilter, CL_OUT ClUint32T* pAckerCnt, CL_OUT ClUint32T* pNonAckerCnt, CL_INOUT ClUint16T* pStreamId,CL_IN LogClLogStreamOwnerStreamOpenAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);

typedef void (*LogClLogStreamOwnerStreamCloseAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint32T  nodeAddress, CL_IN ClUint32T  compId, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerStreamCloseAsyncCallbackT_4_0_0 LogClLogStreamOwnerStreamCloseAsyncCallbackT;

ClRcT clLogStreamOwnerStreamCloseClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint32T  nodeAddress, CL_IN ClUint32T  compId,CL_IN LogClLogStreamOwnerStreamCloseAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);

typedef void (*LogClLogStreamOwnerFilterSetAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  filterFlags, CL_IN ClLogFilterT_4_0_0* pFilter, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerFilterSetAsyncCallbackT_4_0_0 LogClLogStreamOwnerFilterSetAsyncCallbackT;

ClRcT clLogStreamOwnerFilterSetClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  filterFlags, CL_IN ClLogFilterT_4_0_0* pFilter,CL_IN LogClLogStreamOwnerFilterSetAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);

ClRcT clLogStreamOwnerHandlerRegisterClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClUint32T pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T handlerFlags, CL_IN ClUint32T localAddr, CL_IN ClUint32T compId);

typedef void (*LogClLogStreamOwnerHandlerRegisterAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  handlerFlags, CL_IN ClUint32T  localAddr, CL_IN ClUint32T  compId, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerHandlerRegisterAsyncCallbackT_4_0_0 LogClLogStreamOwnerHandlerRegisterAsyncCallbackT;

ClRcT clLogStreamOwnerHandlerRegisterClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  handlerFlags, CL_IN ClUint32T  localAddr, CL_IN ClUint32T  compId,CL_IN LogClLogStreamOwnerHandlerRegisterAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);

ClRcT clLogStreamOwnerStreamMcastGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_OUT ClUint64T* mcastAddr);

ClRcT clLogStreamOwnerHandlerDeregisterClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T handlerFlags, CL_IN ClUint32T localAddr, CL_IN ClUint32T compId);

typedef void (*LogClLogStreamOwnerHandlerDeregisterAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  handlerFlags, CL_IN ClUint32T  localAddr, CL_IN ClUint32T  compId, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerHandlerDeregisterAsyncCallbackT_4_0_0 LogClLogStreamOwnerHandlerDeregisterAsyncCallbackT;

ClRcT clLogStreamOwnerHandlerDeregisterClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_IN ClUint8T  handlerFlags, CL_IN ClUint32T  localAddr, CL_IN ClUint32T  compId,CL_IN LogClLogStreamOwnerHandlerDeregisterAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);

ClRcT clLogStreamOwnerFilterGetClientSync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_OUT ClLogFilterT_4_0_0* pFilter);

typedef void (*LogClLogStreamOwnerFilterGetAsyncCallbackT_4_0_0) (CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_OUT ClLogFilterT_4_0_0* pFilter, CL_IN ClRcT rc, CL_IN void* pCookie);

typedef LogClLogStreamOwnerFilterGetAsyncCallbackT_4_0_0 LogClLogStreamOwnerFilterGetAsyncCallbackT;

ClRcT clLogStreamOwnerFilterGetClientAsync_4_0_0(CL_IN ClIdlHandleT handle, CL_IN SaNameT* pStreamName, CL_IN ClLogStreamScopeT  pStreamScope, CL_IN SaNameT* pStreamScopeNode, CL_OUT ClLogFilterT_4_0_0* pFilter,CL_IN LogClLogStreamOwnerFilterGetAsyncCallbackT_4_0_0 fpAsyncCallback, CL_IN void *cookie);


#ifdef __cplusplus
}
#endif
#endif /*_LOG_PORT_STREAM_OWNER_CLIENT_H_*/
