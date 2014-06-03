/*******************************************************************************
**
** FILE:
**   saClm.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Cluster Membership Service (CLM). It contains  
**   all the prototypes and type definitions required for CLM. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-CLM-B.04.01
**
** DATE: 
**   Wed Oct 08 2008 
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2008 by the Service Availability Forum. All rights reserved.
** 
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
** 
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/


#ifndef _SA_CLM_H
#define _SA_CLM_H

#include "saAis.h"
#include "saNtf.h"

#ifdef  __cplusplus
extern "C" {
#endif


typedef SaUint64T SaClmHandleT;
typedef SaUint32T SaClmNodeIdT;

#define SA_CLM_LOCAL_NODE_ID 0XFFFFFFFF
#define SA_CLM_MAX_ADDRESS_LENGTH 64

typedef enum {
    SA_CLM_AF_INET = 1,
    SA_CLM_AF_INET6 = 2
} SaClmNodeAddressFamilyT;

typedef enum {
    SA_CLM_NODE_NO_CHANGE = 1,
    SA_CLM_NODE_JOINED = 2,
    SA_CLM_NODE_LEFT = 3,
    SA_CLM_NODE_RECONFIGURED = 4,
    SA_CLM_NODE_UNLOCK = 5,
    SA_CLM_NODE_SHUTDOWN = 6
} SaClmClusterChangesT;

typedef enum {
    SA_CLM_CLUSTER_CHANGE_STATUS = 1,
    SA_CLM_ADMIN_STATE = 2
} SaClmStateT;

typedef enum {
    SA_CLM_ADMIN_UNLOCK = 1,
    SA_CLM_ADMIN_LOCK = 2,
    SA_CLM_ADMIN_SHUTDOWN = 3
} SaClmAdminOperationIdT;

typedef enum {
    SA_CLM_ADMIN_UNLOCKED = 1,
    SA_CLM_ADMIN_LOCKED = 2,
    SA_CLM_ADMIN_SHUTTING_DOWN = 3
} SaClmAdminStateT;

#ifdef SA_CLM_B03
typedef enum {
    SA_CLM_NODE_NAME = 1
} SaClmAdditionalInfoIdT;
#endif /* SA_CLM_B03 */

typedef enum {
    SA_CLM_ROOT_CAUSE_ENTITY = 1
} SaClmAdditionalInfoIdT_4;

typedef struct {
    SaClmNodeAddressFamilyT family;
    SaUint16T length;
    SaUint8T value[SA_CLM_MAX_ADDRESS_LENGTH];
} SaClmNodeAddressT;

typedef enum {
    SA_CLM_CHANGE_VALIDATE = 1,
    SA_CLM_CHANGE_START = 2,
    SA_CLM_CHANGE_ABORTED = 3,
    SA_CLM_CHANGE_COMPLETED = 4
} SaClmChangeStepT;

typedef enum {
    SA_CLM_CALLBACK_RESPONSE_OK = 1,
    SA_CLM_CALLBACK_RESPONSE_REJECTED= 2,
    SA_CLM_CALLBACK_RESPONSE_ERROR = 3
} SaClmResponseT;

typedef enum {
    SA_CLM_NTFID_NODE_JOIN = 0x065,
    SA_CLM_NTFID_NODE_LEAVE = 0x066,
    SA_CLM_NTFID_NODE_RECONFIG = 0x067,
    SA_CLM_NTFID_NODE_ADMIN_STATE = 0x068,
} SaClmNotificationMinorIdT;

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
typedef struct {
    SaClmNodeIdT      nodeId;
    SaClmNodeAddressT nodeAddress;
    SaNameT           nodeName;
    SaBoolT           member;
    SaTimeT           bootTimestamp;
    SaUint64T         initialViewNumber;
} SaClmClusterNodeT;

typedef struct {
    SaClmClusterNodeT    clusterNode;
    SaClmClusterChangesT clusterChange;
} SaClmClusterNotificationT;

typedef struct {
    SaUint64T                  viewNumber;
    SaUint32T                  numberOfItems;
    SaClmClusterNotificationT *notification;
} SaClmClusterNotificationBufferT;
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

typedef struct {
    SaClmNodeIdT nodeId;
    SaClmNodeAddressT nodeAddress;
    SaNameT nodeName;
    SaNameT executionEnvironment;
    SaBoolT member;
    SaTimeT bootTimestamp;
    SaUint64T initialViewNumber;
} SaClmClusterNodeT_4;

typedef struct {
    SaClmClusterNodeT_4 clusterNode;
    SaClmClusterChangesT clusterChange;
} SaClmClusterNotificationT_4;

typedef struct {
    SaUint64T viewNumber;
    SaUint32T numberOfItems;
    SaClmClusterNotificationT_4 *notification;
} SaClmClusterNotificationBufferT_4;

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
typedef void 
(*SaClmClusterTrackCallbackT) (
    const SaClmClusterNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error);

typedef void 
(*SaClmClusterNodeGetCallbackT) (
    SaInvocationT invocation,
    const SaClmClusterNodeT *clusterNode,
    SaAisErrorT error);
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

#if defined(SA_CLM_B01) || defined(SA_CLM_B02)
typedef struct {
    SaClmClusterNodeGetCallbackT saClmClusterNodeGetCallback;
    SaClmClusterTrackCallbackT saClmClusterTrackCallback;
} SaClmCallbacksT;
#endif /* SA_CLM_B01 || SA_CLM_B02 */

#ifdef SA_CLM_B03
typedef void 
(*SaClmClusterNodeEvictionCallbackT) (
    SaClmHandleT clmHandle,
    SaInvocationT invocation,
    SaClmAdminOperationIdT operationId);

typedef struct {
    SaClmClusterNodeGetCallbackT saClmClusterNodeGetCallback;
    SaClmClusterTrackCallbackT saClmClusterTrackCallback;
    SaClmClusterNodeEvictionCallbackT saClmClusterNodeEvictionCallback;
} SaClmCallbacksT_3;
#endif /* SA_CLM_B03 */

typedef void 
(*SaClmClusterNodeGetCallbackT_4)(
    SaInvocationT invocation,
    const SaClmClusterNodeT_4 *clusterNode,
    SaAisErrorT error);

typedef void 
(*SaClmClusterTrackCallbackT_4) (
    const SaClmClusterNotificationBufferT_4 *notificationBuffer,
    SaUint32T numberOfMembers,
    SaInvocationT invocation,
    const SaNameT *rootCauseEntity,
    const SaNtfCorrelationIdsT *correlationIds,
    SaClmChangeStepT step,
    SaTimeT timeSupervision,
    SaAisErrorT error);

typedef struct {
    SaClmClusterNodeGetCallbackT_4 saClmClusterNodeGetCallback;
    SaClmClusterTrackCallbackT_4   saClmClusterTrackCallback;
} SaClmCallbacksT_4;

/*************************************************/
/******** CLM API function declarations **********/
/*************************************************/
#if defined(SA_CLM_B01) || defined(SA_CLM_B02)
extern SaAisErrorT 
saClmInitialize(
    SaClmHandleT *clmHandle, 
    const SaClmCallbacksT *clmCallbacks,
    SaVersionT *version);
#endif /* SA_CLM_B01 || SA_CLM_B02 */

#ifdef SA_CLM_B03
extern SaAisErrorT 
saClmInitialize_3(
    SaClmHandleT *clmHandle, 
    const SaClmCallbacksT_3 *clmCallbacks,
    SaVersionT *version);
#endif /* SA_CLM_B03 */

extern SaAisErrorT 
saClmInitialize_4(
    SaClmHandleT *clmHandle, 
    const SaClmCallbacksT_4 *clmCallbacks,
    SaVersionT *version);


extern SaAisErrorT 
saClmSelectionObjectGet(
    SaClmHandleT clmHandle, 
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT
saClmDispatch(
    SaClmHandleT clmHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saClmFinalize(
    SaClmHandleT clmHandle);

#if defined(SA_CLM_B01) || defined(SA_CLM_B02) || defined(SA_CLM_B03)
extern SaAisErrorT 
saClmClusterTrack(
    SaClmHandleT clmHandle,
    SaUint8T trackFlags,
    SaClmClusterNotificationBufferT *notificationBuffer);

extern SaAisErrorT 
saClmClusterNodeGet(
    SaClmHandleT clmHandle,
    SaClmNodeIdT nodeId, 
    SaTimeT timeout,
    SaClmClusterNodeT *clusterNode);
#endif /* SA_CLM_B01 || SA_CLM_B02 || SA_CLM_B03 */

#if defined(SA_CLM_B02) || defined(SA_CLM_B03)
extern SaAisErrorT 
saClmClusterNotificationFree(
    SaClmHandleT clmHandle,
    SaClmClusterNotificationT *notification);
#endif /* SA_CLM_B02 || SA_CLM_B03 */

#ifdef SA_CLM_B03
extern SaAisErrorT
saClmResponse(
    SaClmHandleT clmHandle,
    SaInvocationT invocation,
    SaAisErrorT error);
#endif /* SA_CLM_B03 */

extern SaAisErrorT 
saClmClusterTrack_4(
    SaClmHandleT clmHandle,
    SaUint8T trackFlags,
    SaClmClusterNotificationBufferT_4 *notificationBuffer);

extern SaAisErrorT 
saClmClusterTrackStop(
    SaClmHandleT clmHandle);

extern SaAisErrorT 
saClmClusterNotificationFree_4(
    SaClmHandleT clmHandle,
    SaClmClusterNotificationT_4 *notification);

extern SaAisErrorT 
saClmClusterNodeGet_4(
    SaClmHandleT clmHandle,
    SaClmNodeIdT nodeId,
    SaTimeT timeout,
    SaClmClusterNodeT_4 *clusterNode);

extern SaAisErrorT
saClmClusterNodeGetAsync(
    SaClmHandleT clmHandle,
    SaInvocationT invocation,
    SaClmNodeIdT nodeId);

extern SaAisErrorT 
saClmResponse_4(
    SaClmHandleT clmHandle,
    SaInvocationT invocation,
    SaClmResponseT response);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_CLM_H */
