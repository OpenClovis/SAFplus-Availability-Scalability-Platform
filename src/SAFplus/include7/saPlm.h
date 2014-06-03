/*******************************************************************************
**
** FILE:
**   SaMsg.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Platform Management Service (PLM). It contains all of 
**   the prototypes and type definitions required for PLM. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-PLM-A.01.01
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

#ifndef _SA_PLM_H
#define _SA_PLM_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaPlmHandleT;
typedef SaUint64T SaPlmEntityGroupHandleT;

typedef enum {
    SA_PLM_HE_ADMIN_UNLOCKED = 1,
    SA_PLM_HE_ADMIN_LOCKED = 2,
    SA_PLM_HE_ADMIN_LOCKED_INACTIVE = 3,
    SA_PLM_HE_ADMIN_SHUTTING_DOWN = 4
} SaPlmHEAdminStateT;

typedef enum {
    SA_PLM_EE_ADMIN_UNLOCKED = 1,
    SA_PLM_EE_ADMIN_LOCKED = 2,
    SA_PLM_EE_ADMIN_LOCKED_INSTANTIATION = 3,
    SA_PLM_EE_ADMIN_SHUTTING_DOWN = 4
} SaPlmEEAdminStateT;

typedef enum {
    SA_PLM_OPERATIONAL_ENABLED = 1,
    SA_PLM_OPERATIONAL_DISABLED = 2
} SaPlmOperationalStateT;

typedef enum {
    SA_PLM_HE_PRESENCE_NOT_PRESENT = 1,
    SA_PLM_HE_PRESENCE_INACTIVE = 2,
    SA_PLM_HE_PRESENCE_ACTIVATING = 3,
    SA_PLM_HE_PRESENCE_ACTIVE = 4,
    SA_PLM_HE_PRESENCE_DEACTIVATING = 5
} SaPlmHEPresenceStateT;

typedef enum {
    SA_PLM_EE_PRESENCE_UNINSTANTIATED = 1,
    SA_PLM_EE_PRESENCE_INSTANTIATING = 2,
    SA_PLM_EE_PRESENCE_INSTANTIATED = 3,
    SA_PLM_EE_PRESENCE_TERMINATING = 4,
    SA_PLM_EE_PRESENCE_INSTANTIATION_FAILED = 5,
    SA_PLM_EE_PRESENCE_TERMINATION_FAILED = 6
} SaPlmEEPresenceStateT;

typedef enum {
    SA_PLM_READINESS_OUT_OF_SERVICE = 1,
    SA_PLM_READINESS_IN_SERVICE = 2,
    SA_PLM_READINESS_STOPPING = 3
} SaPlmReadinessStateT;

#define SA_PLM_RF_MANAGEMENT_LOST             0x00001
#define SA_PLM_RF_ADMIN_OPERATION_PENDING     0x00002
#define SA_PLM_RF_ISOLATE_PENDING             0x00004
#define SA_PLM_RF_DEPENDENCY                  0x00100
#define SA_PLM_RF_IMMINENT_FAILURE            0x00200
#define SA_PLM_RF_DEPENDENCY_IMMINENT_FAILURE 0x00400
typedef SaUint64T SaPlmReadinessFlagsT;

typedef struct {
    SaPlmReadinessStateT readinessState;
    SaPlmReadinessFlagsT readinessFlags;
} SaPlmReadinessStatusT;

typedef enum {
    SA_PLM_RI_FAILURE = 1,
    SA_PLM_RI_IMMINENT_FAILURE = 2,
    SA_PLM_RI_FAILURE_CLEARED = 101,
    SA_PLM_RI_IMMINENT_FAILURE_CLEARED = 102
} SaPlmReadinessImpactT;

typedef enum {
    SA_PLM_DP_REJECT_NOT_OOS = 1,
    SA_PLM_DP_VALIDATE = 2,
    SA_PLM_DP_UNCONDITIONAL = 3
} SaPlmHEDeactivationPolicyT;

typedef enum {
    SA_PLM_GROUP_SINGLE_ENTITY = 1,
    SA_PLM_GROUP_SUBTREE = 2,
    SA_PLM_GROUP_SUBTREE_HES_ONLY = 3,
    SA_PLM_GROUP_SUBTREE_EES_ONLY = 4
} SaPlmGroupOptionsT;

typedef enum {
    SA_PLM_GROUP_NO_CHANGE = 1,
    SA_PLM_GROUP_MEMBER_ADDED = 2,
    SA_PLM_GROUP_MEMBER_REMOVED = 3,
    SA_PLM_GROUP_MEMBER_READINESS_CHANGE = 4
} SaPlmGroupChangesT;

typedef enum {
    SA_PLM_CHANGE_VALIDATE = 1,
    SA_PLM_CHANGE_START = 2,
    SA_PLM_CHANGE_ABORTED = 3,
    SA_PLM_CHANGE_COMPLETED = 4
} SaPlmChangeStepT;

typedef enum {
        /* Causes that may trigger all change steps */
    SA_PLM_CAUSE_HE_DEACTIVATION = 1,
    SA_PLM_CAUSE_LOCK = 2,
        /* Causes that may trigger only START and COMPLETED steps */
    SA_PLM_CAUSE_SHUTDOWN = 101,
        /* Causes that only trigger a COMPLETED step */
    SA_PLM_CAUSE_GROUP_CHANGE = 201,
    SA_PLM_CAUSE_MANAGEMENT_LOST = 202,
    SA_PLM_CAUSE_MANAGEMENT_REGAINED = 203,
    SA_PLM_CAUSE_FAILURE = 204,
    SA_PLM_CAUSE_FAILURE_CLEARED = 205,
    SA_PLM_CAUSE_IMMINENT_FAILURE = 206,
    SA_PLM_CAUSE_IMMINENT_FAILURE_CLEARED = 207,
    SA_PLM_CAUSE_UNLOCKED = 208,
    SA_PLM_CAUSE_HE_ACTIVATED = 209,
    SA_PLM_CAUSE_HE_RESET = 210,
    SA_PLM_CAUSE_EE_INSTANTIATED = 211,
    SA_PLM_CAUSE_EE_UNINSTANTIATED = 212,
    SA_PLM_CAUSE_EE_RESTART = 213
} SaPlmTrackCauseT;

typedef struct {
    SaPlmGroupChangesT change;
    SaNameT entityName;
    SaPlmReadinessStatusT currentReadinessStatus;
    SaPlmReadinessStatusT expectedReadinessStatus;
    SaNtfIdentifierT plmNotificationId;
} SaPlmReadinessTrackedEntityT;

typedef struct {
    SaUint32T numberOfEntities;
    SaPlmReadinessTrackedEntityT *entities;
} SaPlmReadinessTrackedEntitiesT;

typedef enum {
    SA_PLM_CALLBACK_RESPONSE_OK = 1,
    SA_PLM_CALLBACK_RESPONSE_REJECTED = 2,
    SA_PLM_CALLBACK_RESPONSE_ERROR = 3
} SaPlmReadinessTrackResponseT;

typedef enum {
    SA_PLM_NTFID_HE_ALARM = 0x01,
    SA_PLM_NTFID_EE_ALARM = 0x02,
    SA_PLM_NTFID_HE_SEC_ALARM = 0x03,
    SA_PLM_NTFID_EE_SEC_ALARM = 0x04,
    SA_PLM_NTFID_UNMAPPED_HE_ALARM = 0x05,
    SA_PLM_NTFID_STATE_CHANGE_ROOT = 0x65,
    SA_PLM_NTFID_STATE_CHANGE_DEP = 0x66,
    SA_PLM_NTFID_HPI_NORMAL_MSB = 0x201,
    SA_PLM_NTFID_HPI_NORMAL_LSB = 0x202,
    SA_PLM_NTFID_HPI_XDR = 0x203
} SaPlmNotificationMinorIdT;

typedef enum {
    SA_PLM_AI_ENTITY_PATH = 1,
    SA_PLM_AI_ROOT_OBJECT = 2,
    SA_PLM_AI_HPI_DOMAIN_ID = 3,
    SA_PLM_AI_HPI_EVENT_DATA = 4,
    SA_PLM_AI_HPI_RDR_DATA = 5,
    SA_PLM_AI_HPI_RPT_DATA = 6
} SaPlmAdditionalInfoIdT;

typedef enum {
    SA_PLM_HE_ADMIN_STATE = 1,
    SA_PLM_EE_ADMIN_STATE = 2,
    SA_PLM_OPERATIONAL_STATE = 3,
    SA_PLM_HE_PRESENCE_STATE = 4,
    SA_PLM_EE_PRESENCE_STATE = 5,
    SA_PLM_READINESS_STATE = 6,
    SA_PLM_READINESS_FLAGS = 7
} SaPlmStateT;

typedef void 
(*SaPlmReadinessTrackCallbackT)(
    SaPlmEntityGroupHandleT entityGroupHandle,
    SaUint64T trackCookie,
    SaInvocationT invocation,
    SaPlmTrackCauseT cause,
    const SaNameT *rootCauseEntity,
    SaNtfIdentifierT rootCorrelationId,
    const SaPlmReadinessTrackedEntitiesT *trackedEntities,
    SaPlmChangeStepT step,
    SaAisErrorT error);

typedef struct {
    SaPlmReadinessTrackCallbackT saPlmReadinessTrackCallback;
} SaPlmCallbacksT;

extern SaAisErrorT
saPlmInitialize(
    SaPlmHandleT *plmHandle,
    const SaPlmCallbacksT *plmCallbacks,
    SaVersionT *version);

extern SaAisErrorT
saPlmSelectionObjectGet(
    SaPlmHandleT plmHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT
saPlmDispatch(
    SaPlmHandleT plmHandle,
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saPlmFinalize(
    SaPlmHandleT plmHandle);

extern SaAisErrorT 
saPlmEntityGroupCreate(
    SaPlmHandleT plmHandle,
    SaPlmEntityGroupHandleT *entityGroupHandle);

extern SaAisErrorT
saPlmEntityGroupAdd(
    SaPlmEntityGroupHandleT entityGroupHandle,
    const SaNameT *entityNames,
    SaUint32T entityNamesNumber,
    SaPlmGroupOptionsT options);

extern SaAisErrorT
saPlmEntityGroupRemove(
    SaPlmEntityGroupHandleT entityGroupHandle,
    const SaNameT *entityNames,
    SaUint32T entityNamesNumber);

extern SaAisErrorT
saPlmEntityGroupDelete(
    SaPlmEntityGroupHandleT entityGroupHandle);

extern SaAisErrorT
saPlmReadinessTrack(
    SaPlmEntityGroupHandleT entityGroupHandle,
    SaUint8T trackFlags,
    SaUint64T trackCookie,
    SaPlmReadinessTrackedEntitiesT *trackedEntities);

extern SaAisErrorT
saPlmReadinessTrackResponse(
    SaPlmEntityGroupHandleT entityGroupHandle,
    SaInvocationT invocation,
    SaPlmReadinessTrackResponseT response);

extern SaAisErrorT
saPlmReadinessTrackStop(
    SaPlmEntityGroupHandleT entityGroupHandle);

extern SaAisErrorT
saPlmReadinessNotificationFree(
    SaPlmEntityGroupHandleT entityGroupHandle,
    SaPlmReadinessTrackedEntityT *entities);

extern SaAisErrorT
saPlmEntityReadinessImpact(
    SaPlmHandleT plmHandle,
    const SaNameT *impactedEntity,
    SaPlmReadinessImpactT impact,
    SaNtfCorrelationIdsT *correlationIds);

#ifdef  __cplusplus
}
#endif
#endif  /* _SA_PLM_H */
