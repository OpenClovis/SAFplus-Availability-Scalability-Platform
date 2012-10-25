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
 * ModuleName  : amf                                                           
 * File        : clAmsPolicyEngine.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This header file contains prototypes for Policy Engine Functions. These
 * definitions are AMS internal.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_POLICY_ENGINE_H_
#define _CL_AMS_POLICY_ENGINE_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clAmsErrors.h>
#include <clAmsTypes.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>
#include <clAms.h>
#include <clAmsInvocation.h>

/******************************************************************************
 * Policy Engine Defines
 *****************************************************************************/

#define CL_AMS_SET_R_STATE(ENTITY, STATE)                                   \
{                                                                           \
    if ( (ENTITY)->status.readinessState != (STATE) )                       \
    {                                                                       \
        /*                                                                  \
         * Pre-change notification                                          \
         */                                                                 \
                                                                            \
        AMS_ENTITY_LOG((ENTITY), CL_AMS_MGMT_SUB_AREA_STATE_CHANGE, CL_DEBUG_TRACE,\
                ("State Change: Entity [%s] Readiness State (%s -> %s)\n",  \
                 (ENTITY)->config.entity.name.value,                        \
                 CL_AMS_STRING_R_STATE((ENTITY)->status.readinessState),    \
                 CL_AMS_STRING_R_STATE((STATE)) ));                         \
                                                                            \
        (ENTITY)->status.readinessState = STATE;                            \
                                                                            \
        /*                                                                  \
         * Post-change notification                                         \
         */                                                                 \
    }                                                                       \
}

#define CL_AMS_SET_O_STATE(ENTITY, STATE)                                  do \
{                                                                       \
    if ( (ENTITY)->status.operState != (STATE) )                        \
    {                                                                   \
        ClAmsOperStateT lastOperState = (ENTITY)->status.operState;     \
        /*                                                              \
         * Pre-change notification                                      \
         */                                                             \
        AMS_ENTITY_LOG( (ENTITY), CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, \
                ("State Change: Entity [%s] Operational State (%s -> %s)\n", \
                 (ENTITY)->config.entity.name.value,                    \
                 CL_AMS_STRING_O_STATE((ENTITY)->status.operState),     \
                 CL_AMS_STRING_O_STATE((STATE)) ));                     \
        (ENTITY)->status.operState = STATE;                             \
                                                                        \
        /*                                                              \
         * Post-change notification                                     \
         */                                                             \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_NODE )  \
        {                                                               \
            clAmsPeNodeUpdateReadinessState((ClAmsNodeT *)(ENTITY));    \
        }                                                               \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_SU )    \
        {                                                               \
            clAmsPeSUUpdateReadinessState((ClAmsSUT *)(ENTITY));        \
        }                                                               \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_COMP )  \
        {                                                               \
            clAmsPeCompUpdateReadinessState((ClAmsCompT *)(ENTITY));    \
        }                                                               \
        clAmsOperStateNotificationPublish(&(ENTITY)->config.entity, lastOperState, (STATE)); \
    }                                                                   \
} while(0)

#define CL_AMS_SET_P_STATE(ENTITY, STATE)                                   \
{                                                                           \
    if ( (ENTITY)->status.presenceState != (STATE) )                        \
    {                                                                       \
        /*                                                                  \
         * Pre-change notification                                          \
         */                                                                 \
                                                                            \
        AMS_ENTITY_LOG((ENTITY), CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE,\
                ("State Change: Entity [%s] Presence State (%s -> %s)\n",   \
                 (ENTITY)->config.entity.name.value,                        \
                 CL_AMS_STRING_P_STATE((ENTITY)->status.presenceState),     \
                 CL_AMS_STRING_P_STATE((STATE)) ));                         \
                                                                            \
        (ENTITY)->status.presenceState = STATE;                             \
                                                                            \
        /*                                                                  \
         * Post-change notification                                         \
         */                                                                 \
                                                                            \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_SU )        \
        {                                                                   \
            clAmsPeSUUpdateReadinessState((ClAmsSUT *)(ENTITY));            \
        }                                                                   \
                                                                            \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_COMP )      \
        {                                                                   \
            clAmsPeCompUpdateReadinessState((ClAmsCompT *)(ENTITY));        \
        }                                                                   \
    }                                                                       \
}

#define CL_AMS_SET_A_STATE(ENTITY, STATE)                                   do \
{                                                                       \
    if ( (ENTITY)->config.adminState != (STATE) )                       \
    {                                                                   \
        ClAmsAdminStateT lastAdminState = (ENTITY)->config.adminState ; \
        /*                                                              \
         * Pre-change notification                                      \
         */                                                             \
                                                                        \
        AMS_ENTITY_LOG((ENTITY), CL_AMS_MGMT_SUB_AREA_STATE_CHANGE, CL_DEBUG_TRACE, \
                ("State Change: Entity [%s] Admin State (%s -> %s)\n",  \
                 (ENTITY)->config.entity.name.value,                    \
                 CL_AMS_STRING_A_STATE((ENTITY)->config.adminState),    \
                 CL_AMS_STRING_A_STATE((STATE)) ));                     \
                                                                        \
        (ENTITY)->config.adminState = STATE;                            \
                                                                        \
        /*                                                              \
         * Post-change notification                                     \
         */                                                             \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_SG )    \
        {                                                               \
            clAmsPeSGUpdateReadinessState((ClAmsSGT *)(ENTITY));        \
        }                                                               \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_NODE )  \
        {                                                               \
            clAmsPeNodeUpdateReadinessState((ClAmsNodeT *)(ENTITY));    \
        }                                                               \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_SU )    \
        {                                                               \
            clAmsPeSUUpdateReadinessState((ClAmsSUT *)(ENTITY));        \
        }                                                               \
                                                                        \
        if ( (ENTITY)->config.entity.type == CL_AMS_ENTITY_TYPE_COMP )  \
        {                                                               \
            clAmsPeCompUpdateReadinessState((ClAmsCompT *)(ENTITY));    \
        }                                                               \
        clAmsAdminStateNotificationPublish(&(ENTITY)->config.entity, lastAdminState, (STATE)); \
    }                                                                   \
} while(0)

#define CL_AMS_NOTIFICATION_PUBLISH(entity, targetEntityRef, lastHAState, ntfType, \
                                    switchoverMode)  do {               \
    if( !( (switchoverMode) & CL_AMS_ENTITY_SWITCHOVER_REPLAY) )        \
    {                                                                   \
        ClAmsNotificationDescriptorT notification = {0};                \
                                                                        \
        if(clAmsNotificationEventPayloadSet((const ClAmsEntityT*)entity, \
                                            (const ClAmsEntityRefT*)targetEntityRef, \
                                            lastHAState,                \
                                            ntfType,                    \
                                            &notification) == CL_OK)    \
        {                                                               \
            clAmsNotificationEventPublish(&notification);               \
        }                                                               \
    }                                                                   \
}while(0)

#define CL_AMS_SET_H_STATE(ENTITY, ENTITYREF, STATE, SWITCHOVERMODE)    \
do{                                                                     \
    if ( (ENTITYREF)->haState != (STATE) )                              \
    {                                                                   \
        ClAmsHAStateT lastHAState = (ENTITYREF)->haState;               \
        /*                                                              \
         * Pre-change notification                                      \
         */                                                             \
        AMS_ENTITY_LOG((ENTITY), CL_AMS_MGMT_SUB_AREA_STATE_CHANGE, CL_DEBUG_TRACE, \
                ("State Change: Entity [%s] / Entity [%s] HA State (%s -> %s)\n", \
                 (ENTITY)->config.entity.name.value,                    \
                 (ENTITYREF)->entityRef.entity.name.value,              \
                 CL_AMS_STRING_H_STATE(lastHAState),                    \
                 CL_AMS_STRING_H_STATE((STATE)) ));                     \
                                                                        \
        (ENTITYREF)->haState = (STATE);                                 \
        if ( (ENTITYREF)->entityRef.entity.type == CL_AMS_ENTITY_TYPE_SU ) \
        {                                                               \
            CL_AMS_NOTIFICATION_PUBLISH(ENTITY,                         \
                                        (ClAmsEntityRefT *)ENTITYREF,   \
                                        lastHAState,                    \
                                        CL_AMS_NOTIFICATION_SU_HA_STATE_CHANGE, \
                                        SWITCHOVERMODE);                \
        }                                                               \
        if( (ENTITYREF)->entityRef.entity.type == CL_AMS_ENTITY_TYPE_COMP) \
        {                                                               \
            CL_AMS_NOTIFICATION_PUBLISH(ENTITY, (ClAmsEntityRefT*)ENTITYREF, \
                                        lastHAState,                    \
                                        CL_AMS_NOTIFICATION_COMP_HA_STATE_CHANGE, \
                                        SWITCHOVERMODE);                \
        }                                                               \
    }                                                                   \
}while(0)

#define CL_AMS_SET_EPOCH(ent) do {              \
    (ent)->status.entity.epoch = time(NULL);    \
}while(0)                                       

#define CL_AMS_RESET_EPOCH(ent) do {            \
    (ent)->status.entity.epoch = 0;             \
}while(0)

#define CL_AMS_ENTITY_SWITCHOVER_GRACEFUL   0x1
#define CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE  0x2
#define CL_AMS_ENTITY_SWITCHOVER_FAST       0x4
#define CL_AMS_ENTITY_SWITCHOVER_SU         0x8
#define CL_AMS_ENTITY_SWITCHOVER_SWAP       0x10
#define CL_AMS_ENTITY_SWITCHOVER_REPLAY     0x20
#define CL_AMS_ENTITY_SWITCHOVER_CONTROLLER 0x40

typedef struct ClAmsSUAdjustList
{
    ClAmsSUT *su;
    ClListHeadT list;
}ClAmsSUAdjustListT;

/******************************************************************************
 * Main entry and exit points to the AMS policy engine. 
 *****************************************************************************/

extern ClRcT clAmsPeClusterInstantiate(
        CL_IN ClAmsT *ams);

extern ClRcT clAmsPeClusterTerminate(
        CL_IN ClAmsT *ams);

extern ClRcT clAmsPeClusterTerminateCallback_Step1(
        CL_IN ClAmsT *ams);

extern ClRcT clAmsPeClusterTerminateCallback_Step2(
        CL_IN ClAmsT *ams);

extern ClRcT clAmsPeClusterFaultReport(
        CL_IN    ClAmsT *ams,
        CL_INOUT ClAmsLocalRecoveryT *recovery,
        CL_INOUT ClUint32T *escalation);

extern ClRcT clAmsPePreprocessDb(
        CL_IN ClAmsT *ams);

/******************************************************************************
 * SG Functions
 *****************************************************************************/

/*
 * ----- Administrative Functions -----
 */

extern ClRcT clAmsPeSGUnlock(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGUnlockCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeSGLockAssignment(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGLockAssignmentCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeSGLockInstantiation(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGLockInstantiationCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeSGShutdown(
        CL_IN       ClAmsSGT            *sg); 

extern ClRcT clAmsPeSGShutdownCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeSGHasAssignments(
        CL_IN       ClAmsSGT            *sg);

/*
 * ----- External Events -----
 */

extern ClRcT clAmsPeSGInstantiate(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGInstantiateTimeout(
        CL_IN       ClAmsEntityTimerT   *timer);

extern ClRcT clAmsPeSGAdjustTimeout(
        CL_IN       ClAmsEntityTimerT   *timer);

extern ClRcT clAmsPeSGAdjustProbationTimeout(
        CL_IN       ClAmsEntityTimerT   *timer);

extern ClRcT clAmsPeSGTerminate(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGTerminateCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClRcT               error);

/*
 * ----- AMS Actions for SG -----
 */

extern ClRcT clAmsPeSGEvaluateWork(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGInstantiateSUs(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGAssignSUs(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGAssignSUMPlusN(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGFindSIForActiveAssignment(
        CL_IN       ClAmsSGT            *sg,
        CL_OUT      ClAmsSIT            **targetSI);

extern ClRcT clAmsPeSGFindSIForStandbyAssignment(
        CL_IN       ClAmsSGT            *sg,
        CL_OUT      ClAmsSIT            **targetSI,
        CL_IN       ClAmsSIT            **scannedSIList,
        CL_IN       ClUint32T           numScannedSIs);

extern ClRcT clAmsPeSGFindSUForActiveAssignmentMPlusN(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClAmsSUT            **su,
        CL_IN       ClAmsSIT            *si);

extern ClRcT clAmsPeSGFindSUForStandbyAssignmentMPlusN(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClAmsSUT            **su,
        CL_IN       ClAmsSIT            *si);

extern ClUint32T clAmsPeSGComputeMaxActiveSU(
        CL_IN       ClAmsSGT            *sg);

extern ClUint32T clAmsPeSGComputeMaxStandbySU(
        CL_IN       ClAmsSGT            *sg);

extern ClUint32T clAmsPeSGColocationCheckFails(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClAmsSUT            *su1,
        CL_IN       ClAmsSUT            *su2);

extern ClRcT clAmsPeSGRemoveWork(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           switchoverMode);

extern ClRcT clAmsPeSGUpdateSIDependents(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGComputeAdminState(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClAmsAdminStateT    *adminState);

extern ClRcT clAmsPeSGUpdateReadinessState(
        CL_IN       ClAmsSGT            *sg);

extern ClRcT clAmsPeSGIsInstantiated(
        CL_IN       ClAmsSGT            *sg);

/******************************************************************************
 * Node Functions
 *****************************************************************************/

/*
 * ----- Administrative Functions -----
 */

extern ClRcT clAmsPeNodeUnlock(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeUnlockCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeNodeLockAssignment(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeLockAssignmentCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeNodeLockInstantiation(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeLockInstantiationCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeNodeShutdown(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeShutdownCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeNodeRestart(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           switchoverMode);

extern ClRcT clAmsPeNodeRestartCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeNodeRepaired(
        CL_IN       ClAmsNodeT        *node);

/*
 * ----- External Events -----
 */

extern ClRcT clAmsPeNodeJoinCluster(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeHasLeftCluster(
                                       CL_IN       ClAmsNodeT          *node,
                                       CL_IN       ClBoolT scFailover);

extern ClRcT clAmsPeNodeHasLeftClusterCallback_Step1(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeHasLeftClusterCallback_Step2(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeIsLeavingCluster(
                                         CL_IN       ClAmsNodeT          *node,
                                         CL_IN       ClBoolT scFailover);

extern ClRcT clAmsPeNodeIsLeavingClusterCallback_Step1(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeIsLeavingClusterCallback_Step2(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeColdplugJoinCluster(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeFaultReport(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClAmsCompT          *faultyComp,
        CL_INOUT    ClAmsLocalRecoveryT *recovery,
        CL_OUT      ClUint32T           *escalation);

extern ClRcT clAmsPeNodeFaultReportProcess(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClAmsCompT          *faultyComp,
        CL_INOUT    ClAmsLocalRecoveryT *recovery,
        CL_OUT      ClUint32T           *escalation);

extern ClRcT clAmsPeNodeFaultCallback_Step1(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeFaultCallback_Step2(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

/*
 * ----- AMS Actions for Node -----
 */

extern ClRcT clAmsPeNodeInstantiate(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeInstantiateCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeTerminate(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeTerminateCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClRcT               error);

extern ClRcT clAmsPeNodeEvaluateWork(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeSwitchoverWork(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           switchoverMode);

extern ClRcT clAmsPeNodeSwitchoverCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error,
        CL_IN       ClUint32T           switchoverMode);

extern ClRcT clAmsPeNodeRemoveWork(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeSUFailoverTimeout( 
        CL_IN       ClAmsEntityTimerT   *timer); 

extern ClRcT clAmsPeNodeIsInstantiable(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeIsInstantiable2(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeComputeAdminState(
        CL_IN       ClAmsNodeT          *node,
        CL_OUT      ClAmsAdminStateT    *adminState);

extern ClRcT clAmsPeNodeUpdateReadinessState(
        CL_IN       ClAmsNodeT          *node);

extern ClRcT clAmsPeNodeComputeRecoveryAction(
        CL_IN       ClAmsNodeT          *node,
        CL_INOUT    ClAmsLocalRecoveryT *recovery,
        CL_OUT      ClUint32T           *escalation);

extern ClRcT clAmsPeNodeReset(
        CL_IN       ClAmsNodeT          *node);

/******************************************************************************
 * SU Functions
 *****************************************************************************/

/*
 * ----- Administrative Functions -----
 */

extern ClRcT clAmsPeSUUnlock(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUUnlockCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSULockAssignment(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSULockAssignmentCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSULockInstantiation(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSULockInstantiationCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSUShutdown(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUShutdownCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURestart(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURestartCallback_Step1(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSURestartCallback_Step2(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSUFaultReport(
        CL_IN       ClAmsSUT *su,
        CL_IN       ClAmsCompT *faultyComp,
        CL_INOUT    ClAmsLocalRecoveryT *recovery,
        CL_INOUT    ClUint32T *escalation);

extern ClRcT clAmsPeSUFaultCallback_Step1(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSUFaultCallback_Step2(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSURepaired(
        CL_IN   ClAmsSUT *su);

/*
 * ----- AMS Actions: SU Lifecycle Management -----
 */

extern ClRcT clAmsPeSUInstantiate(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUInstantiateCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeSUInstantiateError(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeSUTerminate(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUTerminateCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeSUCleanup(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUCleanupCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeSUCleanupError(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error);

/*
 * ----- AMS Actions: Work Assignment -----
 */

extern ClRcT clAmsPeSUEvaluateWork(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUAssignWorkAgain(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUSwitchoverWorkByComponent(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUSwitchoverWorkBySI(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUSwitchoverWork(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUSwitchoverCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURemoveWork(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURemoveCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

/*
 * ----- AMS Actions: Work Assignment -----
 */

extern ClRcT clAmsPeSUAssignSI(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsPeSUAssignSICallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeSUAssignSIDependencyCallback(
        CL_IN   ClAmsSUT      *su,
        CL_IN   ClAmsCSIT     *csi,
        CL_IN   ClAmsHAStateT haState,
        CL_IN   ClUint32T     switchoverMode,
        CL_IN   ClBoolT       reassignCSI);

extern ClRcT clAmsPeSUQuiesceSI(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUQuiesceSIGracefullyCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUQuiesceSIImmediatelyCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURemoveSI(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSURemoveSICallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSUFindCompForCSIAssignment(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsCSIT *csi, 
        CL_IN   ClAmsHAStateT haState,
        CL_OUT  ClAmsCompT **foundComp);

extern ClRcT clAmsPeSUFindCompWithCSIAssignment(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsCSIT *csi, 
        CL_OUT  ClAmsCompT **foundComp);

extern ClRcT clAmsPeSUFindCompAndCSIWithCSIAssignment(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsCSIT *csi, 
        CL_OUT  ClAmsCompT **foundComp,
        CL_OUT  ClAmsEntityRefT **csiRef);

/*
 * ----- AMS Actions: -----
 */

extern ClRcT clAmsPeSUCompRestartTimeout( 
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeSUSURestartTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeSUProbationTimeout(
                                       CL_IN ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeSUAssignmentTimeout(
                                       CL_IN ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeSUComputeAdminState(
        CL_IN   ClAmsSUT *su,
        CL_OUT  ClAmsAdminStateT *adminState);

extern ClRcT clAmsPeCompComputeAdminState(
        CL_IN ClAmsCompT *comp,
        CL_OUT ClAmsAdminStateT *adminState);

extern ClRcT clAmsPeSUComputeReadinessState(
        CL_IN   ClAmsSUT *su,
        CL_OUT  ClAmsReadinessStateT *suState);

extern ClRcT clAmsPeSUUpdateReadinessState(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUComputeRecoveryAction(
        CL_IN   ClAmsSUT *su,
        CL_INOUT ClAmsLocalRecoveryT *recovery,
        CL_OUT  ClUint32T *escalation);

extern ClRcT clAmsPeSUReset(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUIsInstantiable(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUIsAssignable(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkInstantiable(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkUninstantiable(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkInstantiated(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkUninstantiated(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkReady(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkNotReady(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkAssigned(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkUnassigned(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkTerminated(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkRestarting(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkFaulty(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUMarkRepaired(
        CL_IN   ClAmsSUT *su);

extern ClRcT clAmsPeSUHasProxiedComponents(
        CL_IN   ClAmsSUT *su);

extern ClUint32T clAmsPeSUComputeLoad(
        CL_IN   ClAmsSUT *su);

/******************************************************************************
 * SI Functions
 *****************************************************************************/

/*
 * ----- Administrative Functions -----
 */

extern ClRcT clAmsPeSIUnlock(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIUnlockCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSILockAssignment(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSILockAssignmentCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeSILockInstantiation(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIShutdown(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIShutdownCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T error);

/*
 * ----- AMS Actions -----
 */

extern ClRcT clAmsPeSIEvaluateWork(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSISwitchoverWork(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSISwitchoverCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSIRemoveWork(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSIRemoveCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeSIUpdateDependents(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIIsActiveAssignable(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIIsActiveAssignable2(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIIsStandbyAssignable(
        CL_IN   ClAmsSIT *si);

extern ClRcT clAmsPeSIComputeAdminState(
        CL_IN   ClAmsSIT *si,
        CL_OUT  ClAmsAdminStateT *adminState);

/******************************************************************************
 * Component Functions
 *****************************************************************************/

/*
 * ----- Administrative Functions -----
 */

extern ClRcT clAmsPeCompRestart(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompRestartCallback_Step1(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClUint32T           error);

extern ClRcT clAmsPeCompRestartCallback_Step2(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClUint32T           error);

/*
 * ----- External Events -----
 */

extern ClRcT clAmsPeCompFaultReport(
        CL_IN   ClAmsCompT *comp,
        CL_OUT  ClAmsLocalRecoveryT *recovery,
        CL_OUT  ClUint32T *escalation);

extern ClRcT clAmsPeCompFaultCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClUint32T error);

extern ClRcT clAmsPeCompComputeRecoveryAction(
        CL_IN       ClAmsCompT *comp,
        CL_INOUT    ClAmsLocalRecoveryT *recovery,
        CL_OUT      ClUint32T *escalation);

extern ClRcT clAmsPeCompComputeSwitchoverMode(
        CL_IN       ClAmsCompT *comp,
        CL_INOUT    ClUint32T *switchoverMode);

/*
 * ----- AMS Actions: Component Lifecycle Management -----
 */

extern ClRcT clAmsPeCompInstantiate(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompInstantiate2(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompInstantiateDelayTimeout(
        CL_IN ClAmsEntityTimerT  *timer);

extern ClRcT clAmsPeCompInstantiateCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompInstantiateError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompInstantiateTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompProxiedCompInstantiateTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompProxiedCompInstantiateError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompTerminate(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompShutdown(
                                 CL_IN   ClAmsCompT *comp,
                                 CL_OUT  ClBoolT *pResponsePending);

extern ClRcT clAmsPeCompTerminateCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompTerminateTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompTerminateError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompCleanup(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompCleanupCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompCleanupError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompCleanupTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompProxiedCompCleanupCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompProxiedCompCleanupError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompProxiedCompCleanupTimeout(
        CL_IN   ClAmsEntityTimerT *timer);


/*
 * ----- Work Assignment -----
 */

extern ClRcT clAmsPeCompRemoveWork(
        ClAmsCompT *comp,
        ClUint32T switchoverMode);

extern ClRcT clAmsPeCompSwitchoverWork(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClUint32T switchoverMode,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsPeCompSwitchoverCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompReassignWork(
        CL_IN      ClAmsCompT *comp,
        CL_INOUT   ClAmsSUT   **activeSU,
        CL_IN      ClListHeadT *siList,
        CL_OUT     ClBoolT *pAllCSIsReassigned);

/*
 * ----- CSI Assignment -----
 */

extern ClRcT clAmsPeCompAssignCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsPeCompAssignCSIAgain(
        CL_IN   ClAmsCompT *comp);


extern ClRcT clAmsPeCompAssignCSIExtended(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsHAStateT haState,
        CL_IN   ClBoolT  reassignCSI);

extern ClRcT clAmsPeCompReassignCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCompT *oldcomp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsPeCompReassignAllCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCompT *oldcomp,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsPeCompAssignCSICallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClInvocationT invocation,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompAssignCSITimeout(
        CL_IN   ClAmsEntityTimerT  *timer);

extern ClRcT clAmsPeCompAssignCSIError(
        CL_IN   ClAmsCompT  *comp,
        CL_IN   ClRcT error);

extern ClRcT clAmsPeCompQuiesceCSIGracefully(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompQuiesceCSIGracefullyExtended(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode,
        CL_IN   ClBoolT reassignCSI);

extern ClRcT clAmsPeCompQuiesceCSIImmediately(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompQuiesceCSIImmediatelyExtended(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode,
        CL_IN   ClBoolT reassignCSI);

extern ClRcT clAmsPeCompQuiescingCompleteCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClInvocationT invocation,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompQuiescingCSICallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClInvocationT invocation,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompQuiescingCompleteTimeout(
        CL_IN   ClAmsEntityTimerT  *timer);

extern ClRcT clAmsPeCompQuiesceCSIGracefullyError(
        CL_IN ClAmsCompT  *comp,
        CL_IN ClRcT error);

extern ClRcT clAmsPeCompRemoveCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompRemoveCSICallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClInvocationT invocation,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCompRemoveCSITimeout(
        CL_IN   ClAmsEntityTimerT  *timer);

extern ClRcT clAmsPeCompRemoveCSIError(
        CL_IN ClAmsCompT  *comp,
        CL_IN ClRcT error);

extern ClRcT clAmsPeCompUpdateProxiedComponents(
        CL_IN   ClAmsCompT *proxy,
        CL_IN   ClAmsCompCSIRefT *proxyCSIRef);

/*
 * ----- Utility Functions -----
 */

extern ClRcT clAmsPeCompComputeReadinessState(
        CL_IN   ClAmsCompT *comp,
        CL_OUT  ClAmsReadinessStateT *compState);

extern ClRcT clAmsPeCompUpdateReadinessState(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompIsProxyReady(
        CL_IN   ClAmsCompT *comp);

extern ClRcT clAmsPeCompReset(
        CL_IN ClAmsCompT *comp);

extern ClRcT clAmsPeCompAmStartTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompAmStopTimeout(
        CL_IN   ClAmsEntityTimerT *timer);

extern ClRcT clAmsPeCompAmStartCallback(
        CL_IN       ClAmsEntityT *entity,
        CL_IN       ClInvocationT invocation,
        CL_IN       ClRcT error);

extern ClRcT clAmsPeCompAmStopCallback(
        CL_IN       ClAmsEntityT *entity,
        CL_IN       ClInvocationT invocation,
        CL_IN       ClRcT error);

/******************************************************************************
 * CSI Functions
 *****************************************************************************/

/*
 * ----- Utility Functions -----
 */

extern ClRcT clAmsPeCSITransitionHAState(
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT oldState,
        CL_IN   ClAmsHAStateT newState);

extern ClRcT clAmsPeCSITransitionHAStateExtended(
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT oldState,
        CL_IN   ClAmsHAStateT newState,
        CL_IN   ClUint32T switchoverMode);

extern ClRcT clAmsPeCSIComputeAdminState(
        CL_IN ClAmsCSIT *csi,
        CL_OUT ClAmsAdminStateT *adminState);

/******************************************************************************
 * Generic Entity Functions
 *****************************************************************************/

/*
 * clAmsPeEntityUnlock
 * -------------------
 * This event informs the AMS PE that the entity is now administratively
 * unlocked and can be used.
 */

extern ClRcT clAmsPeEntityUnlock(
        CL_IN       ClAmsEntityT        *entity);


/*
 * clAmsPeEntityLockInstantiate
 * ----------------------------
 * This event informs the AMS PE that the entity is now administratively
 * locked and cannot be instantiated. If the entity is in use, it is
 * gracefully removed.
 */

extern ClRcT clAmsPeEntityLockInstantiate(
        CL_IN       ClAmsEntityT        *entity);

/*
 * clAmsPeEntityLockAssignment
 * ---------------------------
 * This event informs the AMS PE that the entity is now administratively
 * locked and cannot be assigned for use. (It can be instantiated) If the 
 * entity is in use, it is gracefully removed.
 */

extern ClRcT clAmsPeEntityLockAssignment(
        CL_IN       ClAmsEntityT        *entity);

extern ClRcT clAmsPeSUForceLockOperation(
                                         CL_IN       ClAmsSUT        *entity,
                                         CL_IN       ClBoolT lock);

extern ClRcT clAmsPeSUForceLockInstantiationOperation(
                                                      CL_IN       ClAmsSUT        *entity);

/*
 * clAmsPeEntityShutdown
 * ---------------------
 * This event informs the AMS PE that the entity should be shutdown
 * gracefully.
 */

extern ClRcT clAmsPeEntityShutdown(
        CL_IN       ClAmsEntityT        *entity);

extern ClRcT clAmsPeEntityShutdownWithRestart(
        CL_IN       ClAmsEntityT        *entity);

/*
 * clAmsPeEntityRestart
 * --------------------
 * This event informs the AMS PE that the entity under consideration
 * should be restarted. A restart includes one of the below sequences
 * on the concerned entity:
 *
 * terminate + instantiate           : graceful termination
 * cleanup + instantiate             : forced termination due to fault
 * terminate + cleanup + instantiate : fault during graceful termination
 */

extern ClRcT clAmsPeEntityRestart(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClUint32T           mode);


/*
 * clAmsPeEntityRepaired
 * ---------------------
 * This event informs the AMS PE that the entity can be considered ready 
 * again for use. 
 */

extern ClRcT clAmsPeEntityRepaired(
        CL_IN       ClAmsEntityT        *entity);

extern ClRcT clAmsPeSISwap( CL_IN ClAmsSIT *si);

extern ClRcT clAmsPeSGAdjust(CL_IN ClAmsSGT *sg, 
                             CL_IN ClUint32T enable);

extern ClRcT clAmsPeSGAutoAdjust(CL_IN ClAmsSGT *sg);

/*-----------------------------------------------------------------------------
 * Other management API functions
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeEntityInstantiate
 * ------------------------
 * This event informs the AMS PE that an AMS entity in the database should 
 * be started.  This function can only be invoked for components, SUs and 
 * nodes, ie all entities that have a presence state.
 *
 * Note: This fn is only for testing. Instantiation of entities is normally
 * decided by AMS.
 */

extern ClRcT clAmsPeEntityInstantiate(
        CL_IN       ClAmsEntityT        *entity);

/*
 * clAmsPeEntityTerminate
 * ----------------------
 * This event informs the AMS PE that the entity under consideration
 * should be terminated.
 *
 * Note: This fn is only for testing. Termination of entities is normally
 * decided by AMS.
 */

extern ClRcT clAmsPeEntityTerminate(
        CL_IN       ClAmsEntityT       *entity);


/*
 * clAmsPeEntitySwitchover
 * -----------------------
 * This event informs the AMS PE that the workloads assigned to the
 * entity under consideration should be switchedover. The entity
 * under consideration is restarted if permitted.
 */

extern ClRcT clAmsPeEntitySwitchover(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClUint32T           switchoverMode);

/*
 * clAmsPeEntityFaultReport
 * ------------------------
 * This event informs the AMS PE that a fault is being reported for the 
 * entity under consideration. This function would typically be called
 * from the event API with the appropriate entity and recovery params.
 *
 * A fault reported via the AMS event API can be only against a component 
 * or a node.
 */

extern ClRcT clAmsPeEntityFaultReport(
        CL_IN       ClAmsEntityT                *entity,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation);

extern ClRcT clAmsPeEntityComputeFaultEscalation(
        CL_IN       ClAmsEntityT                *entity);

extern ClRcT clAmsPeEntityRecoveryScopeLarger(
        CL_IN       ClAmsRecoveryT a,
        CL_IN       ClAmsRecoveryT b);

extern ClRcT clAmsPeReplayCSI(
                               CL_IN ClAmsCompT *comp,
                               CL_IN ClAmsInvocationT *invocationData,
                               CL_IN ClBoolT scFailover);

extern ClRcT clAmsPeSGRealignSU(
                                CL_IN ClAmsSGT *sg,
                                CL_IN ClAmsSUT *su,
                                CL_OUT ClAmsHAStateT *newHAState);

extern ClRcT
clAmsPeEntityOpsReplay(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClBoolT recovery);

extern ClRcT
clAmsPeEntityOpReplay(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op, ClBoolT recovery);

extern ClRcT
clAmsPeSUSwitchoverReplay(ClAmsSUT *su, ClAmsSUT *activeSU, ClUint32T error, ClUint32T switchoverMode);

extern ClRcT
clAmsPeSUSwitchoverRemoveReplay(ClAmsSUT *su, ClUint32T error, ClUint32T switchoverMode);

extern ClBoolT clAmsPeCheckSUReassignOp(ClAmsSUT *su, ClAmsSIT *si, ClBoolT deleteEntry);

extern ClRcT clAmsPeDeleteSUReassignOp(ClAmsSUT *su);

extern ClRcT clAmsPeSUSIReassignEntryDelete(ClAmsSUT *su);

extern ClRcT clAmsPeSIReassignEntryDelete(ClListHeadT *siList);

extern ClRcT clAmsPeSISUReassignEntryDelete(ClAmsSIT *si);

extern ClRcT clAmsPeAddReassignOp(ClAmsSIT *targetSI, ClAmsSUT *targetSU);

extern ClBoolT clAmsPeCheckDependencySIInNode(ClAmsSIT *si, ClAmsSUT *su, ClUint32T level);

extern ClRcT
clAmsPeSGCheckSUHigherRank(ClAmsSGT *sg, ClAmsSUT *su,ClAmsSIT *si);

extern ClRcT clAmsPeSGCheckSUAssignmentDelay(ClAmsSGT *sg);

extern ClRcT
clAmsPeSUSwitchoverWorkActiveAndStandby(ClAmsSUT *su, ClListHeadT *standbyList, ClUint32T mode);

static __inline__ ClBoolT clAmsPeSIReassignMatch(ClAmsSIT *si,
                                                 ClListHeadT *siList)
{
    ClListHeadT *iter = NULL;

    if(!siList) return CL_FALSE;

    CL_LIST_FOR_EACH(iter, siList)
    {
        ClAmsSIReassignEntryT *reassignEntry = CL_LIST_ENTRY(iter, ClAmsSIReassignEntryT, list);
        if(reassignEntry->si == si)
            return CL_TRUE;
    }

    return CL_FALSE;
}

static __inline__ ClRcT clAmsPeSIReassignEntryListDelete(ClListHeadT *siList)
{
    if(!siList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    while(!CL_LIST_HEAD_EMPTY(siList))
    {
        ClListHeadT *pNext = siList->pNext;
        ClAmsSIReassignEntryT *reassignEntry = CL_LIST_ENTRY(pNext, ClAmsSIReassignEntryT, list);
        clListDel(pNext);
        clHeapFree(reassignEntry);
    }
    return CL_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_POLICY_ENGINE_H_ */
