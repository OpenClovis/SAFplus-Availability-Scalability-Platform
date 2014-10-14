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

/**
 * This header file contains definition related to CPM-AMS
 * interaction.
 */

#ifndef _CL_CPM_AMS_H_
#define _CL_CPM_AMS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCkptApi.h>

#include <clCmIpi.h>
#include <clCpmIpi.h>
#include <clAmsEntities.h>
#include <clAmsInvocation.h>

#define CL_AMS_INSTANTIATE_MODE_ACTIVE           1
#define CL_AMS_INSTANTIATE_MODE_STANDBY          1<<1
#define CL_AMS_INSTANTIATE_USE_CHECKPOINT        1<<2

#define CL_AMS_TERMINATE_MODE_GRACEFUL           1
#define CL_AMS_TERMINATE_MODE_IMMEDIATE          1<<1
#define CL_AMS_TERMINATE_MODE_SC_ONLY            1<<2

#define CL_AMS_STATE_CHANGE_GRACEFUL             1
#define CL_AMS_STATE_CHANGE_IMMEDIATE            1<<1
#define CL_AMS_STATE_CHANGE_ACTIVE_TO_STANDBY    1<<2
#define CL_AMS_STATE_CHANGE_STANDBY_TO_ACTIVE    1<<3
#define CL_AMS_STATE_CHANGE_RESET                1<<4
#define CL_AMS_STATE_CHANGE_USE_CHECKPOINT       1<<5
    
/**
 * This structure is used to store invocations of AMF callbacks in
 * container. The invocation data needs to be allocated and freed by
 * the user. This container is thread safe as the access is locked
 * appropriately
 */
typedef struct
{
    ClInvocationT invocation;
    void *data;
    ClUint32T flags;
    ClTimeT createdTime; //Store invocation created timestamp
}ClCpmInvocationT;

typedef struct ClCpmMgmtComp 
{
    ClCharT compName[CL_MAX_NAME_LENGTH];
    ClAmsCompPropertyT compProperty;
    ClCpmCompProcessRelT compProcessRel;
    ClCharT instantiationCMD[CL_MAX_NAME_LENGTH];
    ClCharT *argv[CL_MAX_NAME_LENGTH];
} ClCpmMgmtCompT;

typedef enum
{
    CL_CPM_NODE_LEAVING = 1,
    CL_CPM_NODE_LEFT    = 2,
}ClCpmNodeLeaveT;

/**
 * The AMS to CPM calls on the server side.
 */
typedef ClRcT
(*ClCpmComponentInstantiateT) (CL_IN ClCharT *compName,
                               CL_IN ClCharT *proxyCompName,
                               CL_IN ClUint64T instantiateCookie,
                               CL_IN ClCharT *nodeName,
                               CL_IN ClIocPhysicalAddressT *srcAddress,
                               CL_IN ClUint32T rmdNumber);

typedef ClRcT
(*ClCpmComponentTerminateT) (CL_IN ClCharT *compName,
                             CL_IN ClCharT *proxyCompName,
                             CL_IN ClCharT *nodeName,
                             CL_IN ClIocPhysicalAddressT *srcAddress,
                             CL_IN ClUint32T rmdNumber);

typedef ClRcT
(*ClCpmComponentCleanupT) (CL_IN ClCharT *compName,
                           CL_IN ClCharT *proxyCompName,
                           CL_IN ClCharT *nodeName,
                           CL_IN ClIocPhysicalAddressT *srcAddress,
                           CL_IN ClUint32T rmdNumber,
                           CL_IN ClCpmCompRequestTypeT requestType);

typedef ClRcT
(*ClCpmComponentRestartT) (CL_IN ClCharT *compName,
                           CL_IN ClCharT *proxyCompName,
                           CL_IN ClCharT *nodeName,
                           CL_IN ClIocPhysicalAddressT *srcAddress,
                           CL_IN ClUint32T rmdNumber);

typedef ClRcT
(*ClCpmComponentCSISetT) (CL_IN ClCharT *compName,
                          CL_IN ClCharT *proxyCompName,
                          CL_IN ClCharT *nodeName,
                          CL_IN ClInvocationT invocation,
                          CL_IN ClAmsHAStateT haState,
                          CL_IN ClAmsCSIDescriptorT csiDescriptor);

typedef ClRcT
(*ClCpmComponentCSIRmvT) (CL_IN ClCharT *compName,
                          CL_IN ClCharT *proxyCompName,
                          CL_IN ClCharT *nodeName,
                          CL_IN ClInvocationT invocation,
                          CL_IN ClNameT *csiName,
                          CL_IN ClAmsCSIFlagsT csiFlags);

typedef ClRcT
(*ClCpmComponentPGTrackT) (CL_IN ClIocAddressT iocAddress,
                           CL_IN ClCpmHandleT cpmHandle,
                           CL_IN ClNameT csiName,
                           CL_IN ClAmsPGNotificationBufferT *notificationBuffer,
                           CL_IN ClUint32T numberOfMembers,
                           CL_IN ClUint32T error);
    
typedef ClRcT (*ClCpmNodeDepartureAllowedT) (CL_IN ClNameT *nodeName, CL_IN ClCpmNodeLeaveT nodeLeave);

typedef ClRcT (*ClCpmIocAddressForNodeGetT) (CL_IN ClNameT *nodeName,
                                             CL_OUT ClIocAddressT *pIocAddress);

typedef ClRcT (*ClCpmNodeNameForNodeAddressGetT) (CL_IN ClIocNodeAddressT nodeAddress,
                                                  CL_OUT ClNameT *pNodeName);

typedef ClRcT (*ClCpmNodeFailFastT) (CL_IN ClNameT *nodeName, CL_IN ClBoolT isASPAware);

typedef ClRcT (*ClCpmNodeFailOverT) (CL_IN ClNameT *nodeName, CL_IN ClBoolT isASPAware);

typedef  ClRcT (*ClCpmNodeFailoverRestartT) (CL_IN ClNameT *nodeName, CL_IN ClBoolT isASPAware);

typedef ClRcT (*ClCpmEntityAddT) (CL_IN ClAmsEntityRefT *entityRef);

typedef ClRcT (*ClCpmEntityRmvT) (CL_IN ClAmsEntityRefT *entityRef);

typedef ClRcT (*ClCpmEntitySetConfigT) (CL_IN ClAmsEntityConfigT *entityConfig,
                                        CL_IN ClUint64T bitMask);

typedef ClRcT (*ClCpmNodeHaltT)(CL_IN ClNameT *nodeName, ClBoolT aspAware);

typedef ClRcT (*ClCpmClusterResetT)(void);

typedef struct
{
    ClCpmComponentInstantiateT compInstantiate;
    ClCpmComponentTerminateT compTerminate;
    ClCpmComponentCleanupT compCleanup;
    ClCpmComponentRestartT compRestart;
    ClCpmComponentCSISetT compCSISet;
    ClCpmComponentCSIRmvT compCSIRmv;
    ClCpmComponentPGTrackT compPGTrack;
    ClCpmNodeDepartureAllowedT nodeDepartureAllowed;
    ClCpmIocAddressForNodeGetT cpmIocAddressForNodeGet;
    ClCpmNodeNameForNodeAddressGetT cpmNodeNameForNodeAddressGet;
    ClCpmNodeFailFastT cpmNodeFailFast;
    ClCpmNodeFailOverT cpmNodeFailOver;
    ClCpmNodeFailoverRestartT cpmNodeFailOverRestart;
    ClCpmEntityAddT cpmEntityAdd;
    ClCpmEntityRmvT cpmEntityRmv;
    ClCpmEntitySetConfigT cpmEntitySetConfig;
    ClCpmNodeHaltT cpmNodeHalt;
    ClCpmClusterResetT cpmClusterReset;
}ClCpmAmsToCpmCallT;

/**
 * The CPM to AMS calls on the server side.
 */

/**
 * Get the HA State from AMS
 */
typedef ClRcT (*ClAmsCompHAStateGetT) (CL_IN ClNameT *compName,
                                       CL_IN ClNameT *csiName,
                                       CL_OUT ClAmsHAStateT *haState);

/**
 * Inform AMS that the quiescing complete callback has been received
 * from the component.
 */
typedef ClRcT (*ClAmsQuiescingCompleteT) (CL_IN ClInvocationT invocation,
                                          CL_IN ClRcT retCode);

typedef ClRcT
(*ClAmsComponentErrorReportT) (CL_IN const ClNameT *compName,
                               CL_IN ClTimeT errorDetectionTime,
                               CL_IN ClAmsLocalRecoveryT recommendedRecovery,
                               CL_IN ClUint32T alarmHandle,
                               CL_IN ClUint64T instantiateCookie);

/**
 * Inform CSI operation response to AMS.
 */
typedef ClRcT (*ClAmsCSIOperationResponseT) (CL_IN ClInvocationT invocation,
                                             CL_IN ClRcT retCode);

/**
 * Inform component operation response to AMS.
 */
typedef ClRcT
(*ClAmsComponentOperationResponseT)(CL_IN ClNameT compName,
                                    CL_IN ClCpmCompRequestTypeT requestType,
                                    CL_IN ClRcT retCode);

/**
 * Inform AMS that the node is ready to provide service.
 */
typedef ClRcT (*ClAmsNodeJoinT) (CL_IN ClNameT *nodeName);

/**
 * Inform AMS that node is leaving/left the cluster.
 */
typedef ClRcT (*ClAmsNodeLeaveT) (CL_IN ClNameT *nodeName,
                                  CL_IN ClCpmNodeLeaveT nodeLeave,
                                  CL_IN ClBoolT scFailover);

/**
 * Inform AMS that protection group track request has been received
 * from the component.
 */
typedef ClRcT
(*ClAmsPgTrackT) (CL_IN ClIocAddressT iocAddress,
                  CL_IN ClCpmHandleT cpmHandle,
                  CL_IN ClNameT *csiName,
                  CL_IN ClUint8T trackFlags,
                  CL_OUT ClAmsPGNotificationBufferT *notificationBuffer);

/**
 * Inform AMS that protection group track stop request has been
 * received from the component.
 */
typedef ClRcT (*ClAmsPgTrackStopT) (CL_IN ClIocAddressT iocAddress,
                                    CL_IN ClCpmHandleT cpmHandle,
                                    CL_IN ClNameT *csiName);

/**
 * Inform AMS that checkpoint server is ready to provide service.
 */
typedef ClRcT (*ClAmsCkptServerReady) (ClCkptHdlT ckptHandle, ClUint32T mode);

/**
 * Inform AMS that event server is ready to provide service.
 */
typedef ClRcT (*ClAmsSAEventServerReady) (ClBoolT eventServerReady);

/**
 * Inform AMS to change state from active to standby or vice-versa.
 */
typedef ClRcT (*ClAmsSAAmsStateChange) (ClUint32T mode);

/*
 * Request to AMS to sync up CPMs node table.
 */
typedef ClRcT (*ClAmsSANodeAdd)(const ClCharT *node);

typedef ClRcT (*ClAmsSANodeRestart)(const ClNameT *nodeName, ClBoolT graceful);

typedef struct
{
    ClAmsCompHAStateGetT compHAStateGet;
    ClAmsQuiescingCompleteT csiQuiescingComplete;
    ClAmsComponentErrorReportT compErrorReport;
    ClAmsCSIOperationResponseT csiOperationComplete;
    ClAmsComponentOperationResponseT compOperationComplete;
    ClAmsNodeJoinT nodeJoin;
    ClAmsNodeLeaveT nodeLeave;
    ClAmsPgTrackT pgTrack;
    ClAmsPgTrackStopT pgTrackStop;
    ClAmsCkptServerReady ckptServerReady;
    ClAmsSAAmsStateChange amsStateChange;
    ClAmsSAEventServerReady eventServerReady;
    ClAmsSANodeAdd  nodeAdd;
    ClAmsSANodeRestart nodeRestart;
}ClCpmCpmToAmsCallT;

/**
 * Some helper functions used for CPM-AMS interaction.
 */

extern ClRcT cpmInvocationAdd(ClUint32T cbType,
                              void *data,
                              ClInvocationT *invocationId,
                              ClUint32T flags);

extern ClRcT cpmInvocationAddKey(ClUint32T cbType,
                                 void *data,
                                 ClInvocationT invocationId,
                                 ClUint32T flags);

extern ClRcT cpmInvocationGet(ClInvocationT invocationId,
                              ClUint32T *cbType,
                              void **data);

extern ClRcT cpmInvocationGetWithLock(ClInvocationT invocationId,
                                      ClUint32T *cbType,
                                      void **data);

/**
 * Delete all the invocation entries for a given component name 
 */
extern ClRcT cpmInvocationClearCompInvocation(ClNameT *compName);

/**
 * Delete a specific invocation entry 
 */
extern ClRcT cpmInvocationDeleteInvocation(ClInvocationT invocationId);

extern ClRcT cpmReplayInvocationAdd(ClUint32T cbType, const ClCharT *pComp, 
                                    const ClCharT *pNode, ClBoolT *pResponsePending);

extern ClRcT cpmReplayInvocationsGet(ClAmsInvocationT ***ppInvocations, 
                                     ClUint32T *pNumInvocations, ClBoolT canDelete);

extern ClRcT cpmReplayInvocations(ClBoolT canDelete);

extern ClRcT _cpmComponentCSIRmv(ClCharT *compName,
                                 ClCharT *proxyCompName,
                                 ClCharT *nodeName,
                                 ClInvocationT invocation,
                                 ClNameT *csiName,
                                 ClAmsCSIFlagsT csiFlags);

extern ClRcT _cpmComponentCSISet(ClCharT *compName,
                                 ClCharT *proxyCompName,
                                 ClCharT *nodeName,
                                 ClInvocationT invocation,
                                 ClAmsHAStateT haState,
                                 ClAmsCSIDescriptorT csiDescriptor);

extern ClRcT clCpmAmsToCpmInitialize(CL_IN ClCpmAmsToCpmCallT **callback);

extern void clCpmAmsToCpmFree(CL_IN ClCpmAmsToCpmCallT *callback);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_AMS_H_ */
