/*******************************************************************************
**
** FILE:
**   SaAmf.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum Availability Management Framework (AMF). 
**   It contains all of the prototypes and type definitions required 
**   for user and admin API functions
**   
** SPECIFICATION VERSION:
**   SAI-AIS-AMF-B.04.01
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

#ifndef _SA_AMF_H
#define _SA_AMF_H

#include "saAis.h"
#include "saNtf.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaAmfHandleT;

#define SA_AMF_PM_ZERO_EXIT 0x1
#define SA_AMF_PM_NON_ZERO_EXIT 0x2
#define SA_AMF_PM_ABNORMAL_END 0x4

typedef SaUint32T SaAmfPmErrorsT;

typedef enum {
    SA_AMF_PM_PROC = 1,
    SA_AMF_PM_PROC_AND_DESCENDENTS = 2,
    SA_AMF_PM_ALL_PROCESSES = 3
} SaAmfPmStopQualifierT;

typedef enum {
    SA_AMF_HEALTHCHECK_AMF_INVOKED = 1,
    SA_AMF_HEALTHCHECK_COMPONENT_INVOKED= 2
} SaAmfHealthcheckInvocationT;

#define SA_AMF_HEALTHCHECK_KEY_MAX 32

typedef struct {
    SaUint8T key[SA_AMF_HEALTHCHECK_KEY_MAX];
    SaUint16T keyLen;
} SaAmfHealthcheckKeyT;

typedef enum {
    SA_AMF_HA_ACTIVE = 1,
    SA_AMF_HA_STANDBY = 2,
    SA_AMF_HA_QUIESCED = 3,
    SA_AMF_HA_QUIESCING = 4
} SaAmfHAStateT;

typedef enum {							
    SA_AMF_READINESS_OUT_OF_SERVICE = 1,
    SA_AMF_READINESS_IN_SERVICE = 2,
    SA_AMF_READINESS_STOPPING = 3
} SaAmfReadinessStateT;

typedef enum {							
    SA_AMF_PRESENCE_UNINSTANTIATED = 1,
    SA_AMF_PRESENCE_INSTANTIATING = 2,
    SA_AMF_PRESENCE_INSTANTIATED = 3,
    SA_AMF_PRESENCE_TERMINATING = 4,
    SA_AMF_PRESENCE_RESTARTING = 5,
    SA_AMF_PRESENCE_INSTANTIATION_FAILED = 6,
    SA_AMF_PRESENCE_TERMINATION_FAILED = 7
} SaAmfPresenceStateT;

typedef enum {							
    SA_AMF_OPERATIONAL_ENABLED = 1,
    SA_AMF_OPERATIONAL_DISABLED = 2
} SaAmfOperationalStateT;

typedef enum {							
    SA_AMF_ADMIN_UNLOCKED =1,
    SA_AMF_ADMIN_LOCKED = 2,
    SA_AMF_ADMIN_LOCKED_INSTANTIATION = 3,
    SA_AMF_ADMIN_SHUTTING_DOWN = 4
} SaAmfAdminStateT;

typedef enum {							
    SA_AMF_ASSIGNMENT_UNASSIGNED=1,
    SA_AMF_ASSIGNMENT_FULLY_ASSIGNED=2,
    SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED=3
} SaAmfAssignmentStateT;

typedef enum {
    SA_AMF_HARS_READY_FOR_ASSIGNMENT = 1,
    SA_AMF_HARS_READY_FOR_ACTIVE_DEGRADED = 2,
    SA_AMF_HARS_NOT_READY_FOR_ACTIVE = 3,
    SA_AMF_HARS_NOT_READY_FOR_ASSIGNMENT = 4
} SaAmfHAReadinessStateT;

typedef enum {							
    SA_AMF_PROXY_STATUS_UNPROXIED = 1,
    SA_AMF_PROXY_STATUS_PROXIED = 2
} SaAmfProxyStatusT;

typedef enum {							
    SA_AMF_READINESS_STATE =1,
    SA_AMF_HA_STATE = 2,
    SA_AMF_PRESENCE_STATE = 3,
    SA_AMF_OP_STATE = 4,
    SA_AMF_ADMIN_STATE = 5,
    SA_AMF_ASSIGNMENT_STATE = 6,
    SA_AMF_PROXY_STATUS = 7,
    SA_AMF_HA_READINESS_STATE = 8
} SaAmfStateT;

#define SA_AMF_CSI_ADD_ONE 0X1
#define SA_AMF_CSI_TARGET_ONE 0X2
#define SA_AMF_CSI_TARGET_ALL 0X4

typedef SaUint32T SaAmfCSIFlagsT;

typedef enum {
    SA_AMF_CSI_NEW_ASSIGN = 1,
    SA_AMF_CSI_QUIESCED = 2,
    SA_AMF_CSI_NOT_QUIESCED = 3,
    SA_AMF_CSI_STILL_ACTIVE = 4
} SaAmfCSITransitionDescriptorT;

typedef struct {
    SaAmfCSITransitionDescriptorT transitionDescriptor;
    SaNameT activeCompName;
} SaAmfCSIActiveDescriptorT;

typedef struct {
    SaNameT activeCompName;
    SaUint32T standbyRank;
} SaAmfCSIStandbyDescriptorT;

typedef union {
    SaAmfCSIActiveDescriptorT  activeDescriptor;
    SaAmfCSIStandbyDescriptorT standbyDescriptor;
} SaAmfCSIStateDescriptorT;

typedef struct {
    SaUint8T *attrName;
    SaUint8T *attrValue;
} SaAmfCSIAttributeT;

typedef struct {
    SaAmfCSIAttributeT *attr;
    SaUint32T          number;
} SaAmfCSIAttributeListT;

typedef struct {
    SaAmfCSIFlagsT           csiFlags;
    SaNameT                  csiName;
    SaAmfCSIStateDescriptorT csiStateDescriptor;
    SaAmfCSIAttributeListT   csiAttr;
} SaAmfCSIDescriptorT;

typedef enum {
    SA_AMF_PROTECTION_GROUP_NO_CHANGE = 1,
    SA_AMF_PROTECTION_GROUP_ADDED = 2,
    SA_AMF_PROTECTION_GROUP_REMOVED = 3,
    SA_AMF_PROTECTION_GROUP_STATE_CHANGE = 4
} SaAmfProtectionGroupChangesT;

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
typedef struct {
    SaNameT       compName;
    SaAmfHAStateT haState;
    SaUint32T     rank;
} SaAmfProtectionGroupMemberT;

typedef struct {
    SaAmfProtectionGroupMemberT   member;
    SaAmfProtectionGroupChangesT  change;
} SaAmfProtectionGroupNotificationT;

typedef struct {
    SaUint32T                          numberOfItems;
    SaAmfProtectionGroupNotificationT  *notification;
} SaAmfProtectionGroupNotificationBufferT;
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

typedef struct {
    SaNameT                compName;
    SaAmfHAStateT          haState;
    SaAmfHAReadinessStateT haReadinessState;
    SaUint32T              rank;
} SaAmfProtectionGroupMemberT_4;

typedef struct {
    SaAmfProtectionGroupMemberT_4   member;
    SaAmfProtectionGroupChangesT    change;
} SaAmfProtectionGroupNotificationT_4;

typedef struct {
    SaUint32T                             numberOfItems;
    SaAmfProtectionGroupNotificationT_4  *notification;
} SaAmfProtectionGroupNotificationBufferT_4;

typedef enum {
    SA_AMF_NO_RECOMMENDATION = 1,
    SA_AMF_COMPONENT_RESTART = 2,
    SA_AMF_COMPONENT_FAILOVER = 3,
    SA_AMF_NODE_SWITCHOVER = 4,
    SA_AMF_NODE_FAILOVER = 5,
    SA_AMF_NODE_FAILFAST = 6,
    SA_AMF_CLUSTER_RESET = 7,
    SA_AMF_APPLICATION_RESTART = 8,
    SA_AMF_CONTAINER_RESTART = 9
} SaAmfRecommendedRecoveryT;

#define SA_AMF_COMP_SA_AWARE    0x0001
#define SA_AMF_COMP_PROXY       0x0002
#define SA_AMF_COMP_PROXIED     0x0004
#define SA_AMF_COMP_LOCAL       0x0008
#define SA_AMF_COMP_CONTAINER   0x0010
#define SA_AMF_COMP_CONTAINED   0x0020
#define SA_AMF_COMP_PROXIED_NPI 0x0040
typedef SaUint32T saAmfCompCategoryT;

typedef enum {								
    SA_AMF_2N_REDUNDANCY_MODEL = 1,
    SA_AMF_NPM_REDUNDANCY_MODEL =2,
    SA_AMF_N_WAY_REDUNDANCY_MODEL = 3,
    SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL = 4,
    SA_AMF_NO_REDUNDANCY_MODEL= 5
} saAmfRedundancyModelT;

typedef enum {								
    SA_AMF_COMP_X_ACTIVE_AND_Y_STANDBY = 1,
    SA_AMF_COMP_X_ACTIVE_OR_Y_STANDBY = 2,
    SA_AMF_COMP_ONE_ACTIVE_OR_Y_STANDBY = 3,
    SA_AMF_COMP_ONE_ACTIVE_OR_ONE_STANDBY = 4,
    SA_AMF_COMP_X_ACTIVE = 5,
    SA_AMF_COMP_1_ACTIVE = 6,
    SA_AMF_COMP_NON_PRE_INSTANTIABLE = 7
} saAmfCompCapabilityModelT;

typedef enum {								
    SA_AMF_NODE_NAME = 1,
    SA_AMF_SI_NAME = 2,
    SA_AMF_MAINTENANCE_CAMPAIGN_DN = 3,
    SA_AMF_AI_RECOMMENDED_RECOVERY = 4,
    SA_AMF_AI_APPLIED_RECOVERY = 5
} SaAmfAdditionalInfoIdT;

typedef enum {
    /* alarms */
    SA_AMF_NTFID_COMP_INSTANTIATION_FAILED = 0x02,
    SA_AMF_NTFID_COMP_CLEANUP_FAILED = 0x03,
    SA_AMF_NTFID_CLUSTER_RESET = 0x04,
    SA_AMF_NTFID_SI_UNASSIGNED = 0x05,
    SA_AMF_NTFID_COMP_UNPROXIED = 0x06,
    /* state change */
    SA_AMF_NTFID_NODE_ADMIN_STATE = 0x065,
    SA_AMF_NTFID_SU_ADMIN_STATE = 0x066,
    SA_AMF_NTFID_SG_ADMIN_STATE = 0x067,
    SA_AMF_NTFID_SI_ADMIN_STATE = 0x068,
    SA_AMF_NTFID_APP_ADMIN_STATE = 0x069,
    SA_AMF_NTFID_CLUSTER_ADMIN_STATE = 0x06A,
    SA_AMF_NTFID_NODE_OP_STATE = 0x06B,
    SA_AMF_NTFID_SU_OP_STATE = 0x06C,
    SA_AMF_NTFID_SU_PRESENCE_STATE = 0x06D,
    SA_AMF_NTFID_SU_SI_HA_STATE = 0x06E,
    SA_AMF_NTFID_SI_ASSIGNMENT_STATE = 0x06F,
    SA_AMF_NTFID_COMP_PROXY_STATUS = 0x070,
    SA_AMF_NTFID_SU_SI_HA_READINESS_STATE = 0x071,
    /* miscellaneous */
    SA_AMF_NTFID_ERROR_REPORT = 0x0191,
    SA_AMF_NTFID_ERROR_CLEAR = 0x0192
} SaAmfNotificationMinorIdT;


/*************************************************/
/************ Defs for AMF Admin API *************/
/*************************************************/

typedef enum {
    SA_AMF_ADMIN_UNLOCK = 1,
    SA_AMF_ADMIN_LOCK = 2,
    SA_AMF_ADMIN_LOCK_INSTANTIATION = 3,
    SA_AMF_ADMIN_UNLOCK_INSTANTIATION = 4,
    SA_AMF_ADMIN_SHUTDOWN = 5,
    SA_AMF_ADMIN_RESTART = 6,
    SA_AMF_ADMIN_SI_SWAP = 7,
    SA_AMF_ADMIN_SG_ADJUST = 8,
    SA_AMF_ADMIN_REPAIRED = 9,
    SA_AMF_ADMIN_EAM_START = 10,
    SA_AMF_ADMIN_EAM_STOP = 11
} SaAmfAdminOperationIdT;


/*************************************************/
/******** AMF API function declarations **********/
/*************************************************/

typedef void 
(*SaAmfHealthcheckCallbackT)(
    SaInvocationT invocation,
    const SaNameT *compName,
    SaAmfHealthcheckKeyT *healthcheckKey);

typedef void 
(*SaAmfComponentTerminateCallbackT)(
    SaInvocationT invocation,
    const SaNameT *compName);

typedef void 
(*SaAmfCSISetCallbackT)(
    SaInvocationT invocation,
    const SaNameT *compName,
    SaAmfHAStateT haState,
    SaAmfCSIDescriptorT csiDescriptor);

typedef void 
(*SaAmfCSIRemoveCallbackT)(
    SaInvocationT invocation,
    const SaNameT *compName,
    const SaNameT *csiName,
    SaAmfCSIFlagsT csiFlags);

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
typedef void 
(*SaAmfProtectionGroupTrackCallbackT)(
    const SaNameT *csiName,
    SaAmfProtectionGroupNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error);
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

typedef void 
(*SaAmfProtectionGroupTrackCallbackT_4)(
    const SaNameT *csiName,
    SaAmfProtectionGroupNotificationBufferT_4 *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error);

typedef void 
(*SaAmfProxiedComponentInstantiateCallbackT)(
    SaInvocationT invocation,
    const SaNameT *proxiedCompName);

typedef void 
(*SaAmfProxiedComponentCleanupCallbackT)(
    SaInvocationT invocation,
    const SaNameT *proxiedCompName);

typedef void 
(*SaAmfContainedComponentInstantiateCallbackT)(
    SaInvocationT invocation,
    const SaNameT *containedCompName);

typedef void 
(*SaAmfContainedComponentCleanupCallbackT)(
    SaInvocationT invocation,
    const SaNameT *containedCompName);

#if defined(SA_AMF_B01) || defined(SA_AMF_B02)
typedef struct {
   SaAmfHealthcheckCallbackT                 saAmfHealthcheckCallback;
   SaAmfComponentTerminateCallbackT          saAmfComponentTerminateCallback;
   SaAmfCSISetCallbackT                      saAmfCSISetCallback;
   SaAmfCSIRemoveCallbackT                   saAmfCSIRemoveCallback;
   SaAmfProtectionGroupTrackCallbackT        saAmfProtectionGroupTrackCallback;
   SaAmfProxiedComponentInstantiateCallbackT saAmfProxiedComponentInstantiateCallback;
   SaAmfProxiedComponentCleanupCallbackT     saAmfProxiedComponentCleanupCallback;
} SaAmfCallbacksT;

extern SaAisErrorT 
saAmfInitialize(
    SaAmfHandleT *amfHandle, 
    const SaAmfCallbacksT *amfCallbacks,
    SaVersionT *version);

extern SaAisErrorT 
saAmfPmStart(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    SaUint64T processId,
    SaInt32T descendentsTreeDepth,
    SaAmfPmErrorsT pmErrors,
    SaAmfRecommendedRecoveryT recommendedRecovery);
#endif /* SA_AMF_B01 || SA_AMF_B02 */

#ifdef SA_AMF_B03 
typedef struct {
    SaAmfHealthcheckCallbackT                 saAmfHealthcheckCallback;
    SaAmfComponentTerminateCallbackT          saAmfComponentTerminateCallback;
    SaAmfCSISetCallbackT                      saAmfCSISetCallback;
    SaAmfCSIRemoveCallbackT                   saAmfCSIRemoveCallback;
    SaAmfProtectionGroupTrackCallbackT        saAmfProtectionGroupTrackCallback;
    SaAmfProxiedComponentInstantiateCallbackT saAmfProxiedComponentInstantiateCallback;
    SaAmfProxiedComponentCleanupCallbackT     saAmfProxiedComponentCleanupCallback;
    SaAmfContainedComponentInstantiateCallbackT saAmfContaintedComponentInstantiateCallback;
    SaAmfContainedComponentCleanupCallbackT saAmfContaintedComponentCleanupCallback;
} SaAmfCallbacksT_3;

extern SaAisErrorT 
saAmfInitialize_3(
    SaAmfHandleT *amfHandle, 
    const SaAmfCallbacksT_3 *amfCallbacks,
    SaVersionT *version);
#endif /* SA_AMF_B03 */

#if defined(SA_AMF_B01) || defined(SA_AMF_B02) || defined(SA_AMF_B03)
extern SaAisErrorT 
saAmfComponentUnregister(
    SaAmfHandleT amfHandle,
    const SaNameT *compName, 
    const SaNameT *proxyCompName);

extern SaAisErrorT 
saAmfProtectionGroupTrack(
    SaAmfHandleT amfHandle,
    const SaNameT *csiName,
    SaUint8T trackFlags,
    SaAmfProtectionGroupNotificationBufferT *notificationBuffer);

extern SaAisErrorT 
saAmfProtectionGroupNotificationFree(
    SaAmfHandleT amfHandle,
    SaAmfProtectionGroupNotificationT *notification);

extern SaAisErrorT 
saAmfComponentErrorReport(
    SaAmfHandleT amfHandle,
    const SaNameT *erroneousComponent,
    SaTimeT errorDetectionTime,
    SaAmfRecommendedRecoveryT recommendedRecovery,
    SaNtfIdentifierT ntfIdentifier);

extern SaAisErrorT 
saAmfComponentErrorClear(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    SaNtfIdentifierT ntfIdentifier);

extern SaAisErrorT 
saAmfResponse(
    SaAmfHandleT amfHandle,
    SaInvocationT invocation,
    SaAisErrorT error);
#endif /* SA_AMF_B01 || SA_AMF_B02 || SA_AMF_B03 */

typedef struct {
    SaAmfHealthcheckCallbackT                   saAmfHealthcheckCallback;
    SaAmfComponentTerminateCallbackT            saAmfComponentTerminateCallback;
    SaAmfCSISetCallbackT                        saAmfCSISetCallback;
    SaAmfCSIRemoveCallbackT                     saAmfCSIRemoveCallback;
    SaAmfProtectionGroupTrackCallbackT_4        saAmfProtectionGroupTrackCallback;
    SaAmfProxiedComponentInstantiateCallbackT   saAmfProxiedComponentInstantiateCallback;
    SaAmfProxiedComponentCleanupCallbackT       saAmfProxiedComponentCleanupCallback;
    SaAmfContainedComponentInstantiateCallbackT saAmfContaintedComponentInstantiateCallback;
    SaAmfContainedComponentCleanupCallbackT     saAmfContaintedComponentCleanupCallback;
} SaAmfCallbacksT_4;

extern SaAisErrorT 
saAmfInitialize_4(
    SaAmfHandleT *amfHandle, 
    const SaAmfCallbacksT_4 *amfCallbacks,
    SaVersionT *version);


extern SaAisErrorT 
saAmfSelectionObjectGet(
    SaAmfHandleT amfHandle, 
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saAmfDispatch(
    SaAmfHandleT amfHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saAmfFinalize(
    SaAmfHandleT amfHandle);

extern SaAisErrorT 
saAmfComponentRegister(
    SaAmfHandleT amfHandle,
    const SaNameT *compName, 
    const SaNameT *proxyCompName);

extern SaAisErrorT 
saAmfComponentNameGet(
    SaAmfHandleT amfHandle, 
    SaNameT *compName);

extern SaAisErrorT 
saAmfPmStart_3(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    SaInt64T processId,
    SaInt32T descendentsTreeDepth,
    SaAmfPmErrorsT pmErrors,
    SaAmfRecommendedRecoveryT recommendedRecovery);

extern SaAisErrorT 
saAmfPmStop(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    SaAmfPmStopQualifierT stopQualifier,
    SaInt64T processId,
    SaAmfPmErrorsT pmErrors);

extern SaAisErrorT
saAmfHealthcheckStart(
    SaAmfHandleT amfHandle, 
    const SaNameT *compName,
    const SaAmfHealthcheckKeyT *healthcheckKey,
    SaAmfHealthcheckInvocationT invocationType,
    SaAmfRecommendedRecoveryT recommendedRecovery);

extern SaAisErrorT 
saAmfHealthcheckConfirm(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    const SaAmfHealthcheckKeyT *healthcheckKey,
    SaAisErrorT healthcheckResult);

extern SaAisErrorT 
saAmfHealthcheckStop(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    const SaAmfHealthcheckKeyT *healthcheckKey);

extern SaAisErrorT 
saAmfCSIQuiescingComplete(
    SaAmfHandleT amfHandle,
    SaInvocationT invocation,
    SaAisErrorT error);

extern SaAisErrorT 
saAmfHAReadinessStateSet(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    const SaNameT *csiName,
    SaAmfHAReadinessStateT haReadinessState,
    SaNtfCorrelationIdsT *correlationIds
);

extern SaAisErrorT 
saAmfHAStateGet(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    const SaNameT *csiName,
    SaAmfHAStateT *haState);


extern SaAisErrorT 
saAmfProtectionGroupTrack_4(
    SaAmfHandleT amfHandle,
    const SaNameT *csiName,
    SaUint8T trackFlags,
    SaAmfProtectionGroupNotificationBufferT_4 *notificationBuffer);

extern SaAisErrorT 
saAmfProtectionGroupTrackStop(
    SaAmfHandleT amfHandle,
    const SaNameT *csiName);

extern SaAisErrorT 
saAmfProtectionGroupNotificationFree_4(
    SaAmfHandleT amfHandle,
    SaAmfProtectionGroupNotificationT_4 *notification);

extern SaAisErrorT 
saAmfComponentErrorReport_4(
    SaAmfHandleT amfHandle,
    const SaNameT *erroneousComponent,
    SaTimeT errorDetectionTime,
    SaAmfRecommendedRecoveryT recommendedRecovery,
    SaNtfCorrelationIdsT *correlationIds
);

extern SaAisErrorT 
saAmfComponentErrorClear_4(
    SaAmfHandleT amfHandle,
    const SaNameT *compName,
    SaNtfCorrelationIdsT *correlationIds
);

extern SaAisErrorT 
saAmfResponse_4(
    SaAmfHandleT amfHandle,
    SaInvocationT invocation,
    SaNtfCorrelationIdsT *correlationIds,
    SaAisErrorT error
);

#ifdef  __cplusplus
}
#endif
#endif  /* _SA_AMF_H */
