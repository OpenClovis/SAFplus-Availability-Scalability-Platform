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

  Header file of SA Forum AIS AMF APIs (SAI-AIS-B.01.00.09)
  compiled on 21SEP2004 by sayandeb.saha@motorola.com.

*/

#ifndef _SA_AMF_H_
#define _SA_AMF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <saAis.h>
#include <saNtf.h>

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
   SaUint32T number;
} SaAmfCSIAttributeListT;

typedef struct {
   SaAmfCSIFlagsT csiFlags;
   SaNameT csiName;
   SaAmfCSIStateDescriptorT csiStateDescriptor;
   SaAmfCSIAttributeListT csiAttr;
} SaAmfCSIDescriptorT;

typedef struct {
   SaNameT compName;
   SaAmfHAStateT haState;
   SaUint32T rank;
} SaAmfProtectionGroupMemberT;

typedef enum {
   SA_AMF_PROTECTION_GROUP_NO_CHANGE = 1,
   SA_AMF_PROTECTION_GROUP_ADDED = 2,
   SA_AMF_PROTECTION_GROUP_REMOVED = 3,
   SA_AMF_PROTECTION_GROUP_STATE_CHANGE = 4
} SaAmfProtectionGroupChangesT;

typedef struct {
   SaAmfProtectionGroupMemberT member;
   SaAmfProtectionGroupChangesT change;
} SaAmfProtectionGroupNotificationT;

typedef struct {
   SaUint32T numberOfItems;
   SaAmfProtectionGroupNotificationT *notification;
} SaAmfProtectionGroupNotificationBufferT;

typedef enum {
   SA_AMF_NO_RECOMMENDATION = 1,
   SA_AMF_COMPONENT_RESTART = 2,
   SA_AMF_COMPONENT_FAILOVER = 3,
   SA_AMF_NODE_SWITCHOVER = 4,
   SA_AMF_NODE_FAILOVER = 5,
   SA_AMF_NODE_FAILFAST = 6,
   SA_AMF_CLUSTER_RESET =7
} SaAmfRecommendedRecoveryT;

typedef void 
(*SaAmfHealthcheckCallbackT)(SaInvocationT invocation,
                             const SaNameT *compName,
                             SaAmfHealthcheckKeyT *healthcheckKey);

typedef void 
(*SaAmfComponentTerminateCallbackT)(SaInvocationT invocation,
                                    const SaNameT *compName);

typedef void 
(*SaAmfCSISetCallbackT)(SaInvocationT invocation,
                        const SaNameT *compName,
                        SaAmfHAStateT haState,
                        SaAmfCSIDescriptorT csiDescriptor);

typedef void 
(*SaAmfCSIRemoveCallbackT)(SaInvocationT invocation,
                           const SaNameT *compName,
                           const SaNameT *csiName,
                           SaAmfCSIFlagsT csiFlags);

typedef void 
(*SaAmfProtectionGroupTrackCallbackT)(const SaNameT *csiName,
                                      SaAmfProtectionGroupNotificationBufferT *notificationBuffer,
                                      SaUint32T numberOfMembers,
                                      SaAisErrorT error);

typedef void 
(*SaAmfProxiedComponentInstantiateCallbackT)(SaInvocationT invocation,
                                       const SaNameT *proxiedCompName);

typedef void 
(*SaAmfProxiedComponentCleanupCallbackT)(SaInvocationT invocation,
                                         const SaNameT *proxiedCompName);

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
saAmfInitialize(SaAmfHandleT *amfHandle, const SaAmfCallbacksT *amfCallbacks,
                SaVersionT *version);
    extern SaAisErrorT 
saAmfSelectionObjectGet(SaAmfHandleT amfHandle, 
                        SaSelectionObjectT *selectionObject);
    extern SaAisErrorT 
saAmfDispatch(SaAmfHandleT amfHandle, SaDispatchFlagsT dispatchFlags);
    extern SaAisErrorT 
saAmfFinalize(SaAmfHandleT amfHandle);

    extern SaAisErrorT 
saAmfComponentRegister(SaAmfHandleT amfHandle,
                       const SaNameT *compName, const SaNameT *proxyCompName);
    extern SaAisErrorT 
saAmfComponentUnregister(SaAmfHandleT amfHandle,
                         const SaNameT *compName, 
                         const SaNameT *proxyCompName);

    extern SaAisErrorT 
saAmfComponentNameGet(SaAmfHandleT amfHandle, SaNameT *compName);

    extern SaAisErrorT 
saAmfPmStart(SaAmfHandleT amfHandle,
             const SaNameT *compName,
             SaUint64T processId,
             SaInt32T descendentsTreeDepth,
             SaAmfPmErrorsT pmErrors,
             SaAmfRecommendedRecoveryT recommendedRecovery);

    extern SaAisErrorT 
saAmfPmStop(SaAmfHandleT amfHandle,
            const SaNameT *compName,
            SaAmfPmStopQualifierT stopQualifier,
            SaInt64T processId,
            SaAmfPmErrorsT pmErrors);

    extern SaAisErrorT
saAmfHealthcheckStart(SaAmfHandleT amfHandle, 
                      const SaNameT *compName,
                      const SaAmfHealthcheckKeyT *healthcheckKey,
                      SaAmfHealthcheckInvocationT invocationType,
                      SaAmfRecommendedRecoveryT recommendedRecovery);

    extern SaAisErrorT 
saAmfHealthcheckConfirm(SaAmfHandleT amfHandle,
                        const SaNameT *compName,
                        const SaAmfHealthcheckKeyT *healthcheckKey,
                        SaAisErrorT healthcheckResult);

    extern SaAisErrorT 
saAmfHealthcheckStop(SaAmfHandleT amfHandle,
                     const SaNameT *compName,
                     const SaAmfHealthcheckKeyT *healthcheckKey);

    extern SaAisErrorT 
saAmfCSIQuiescingComplete(SaAmfHandleT amfHandle,
                          SaInvocationT invocation,
                          SaAisErrorT error);

    extern SaAisErrorT 
saAmfHAStateGet(SaAmfHandleT amfHandle,
                const SaNameT *compName,
                const SaNameT *csiName,
                SaAmfHAStateT *haState);

    extern SaAisErrorT 
saAmfProtectionGroupTrack(SaAmfHandleT amfHandle,
                          const SaNameT *csiName,
                          SaUint8T trackFlags,
                          SaAmfProtectionGroupNotificationBufferT *notificationBuffer);

    extern SaAisErrorT 
saAmfProtectionGroupTrackStop(SaAmfHandleT amfHandle,
                              const SaNameT *csiName);

    extern SaAisErrorT 
saAmfComponentErrorReport(SaAmfHandleT amfHandle,
                          const SaNameT *erroneousComponent,
                          SaTimeT errorDetectionTime,
                          SaAmfRecommendedRecoveryT recommendedRecovery,
                          SaNtfIdentifierT ntfIdentifier);
    extern SaAisErrorT 
saAmfComponentErrorClear(SaAmfHandleT amfHandle,
                         const SaNameT *compName,
                         SaNtfIdentifierT ntfIdentifier);

    extern SaAisErrorT 
saAmfResponse(SaAmfHandleT amfHandle,
              SaInvocationT invocation,
              SaAisErrorT error);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_AMF_H_ */

