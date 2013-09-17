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
 * File        : clAmsInvocation.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains definitions for invocation related AMS functions.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_INVOCATION_H_
#define _CL_AMS_INVOCATION_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>

/******************************************************************************
 * Global data structures 
 *****************************************************************************/

typedef enum
{
    CL_AMS_TERMINATE_CALLBACK               = 1,
    CL_AMS_CSI_SET_CALLBACK                 = 2,
    CL_AMS_CSI_RMV_CALLBACK                 = 3,
    CL_AMS_PG_TRACK_CALLBACK                = 4,
    CL_AMS_INSTANTIATE_CALLBACK             = 5,
    CL_AMS_PROXIED_INSTANTIATE_CALLBACK     = 6,
    CL_AMS_PROXIED_CLEANUP_CALLBACK         = 7,
    CL_AMS_CSI_QUIESCING_CALLBACK           = 8,
    CL_AMS_INSTANTIATE_REPLAY_CALLBACK      = 9,
    CL_AMS_TERMINATE_REPLAY_CALLBACK        = 10,
    CL_AMS_RECOVERY_REPLAY_CALLBACK         = 11,
    CL_AMS_MAX_CALLBACKS,
}ClAmsInvocationCmdT;

typedef struct 
{
    ClInvocationT       invocation;
    ClAmsInvocationCmdT cmd;
    SaNameT             compName;
    ClBoolT             csiTargetOne;
    ClAmsCSIT           *csi;
    SaNameT             csiName;
    ClBoolT             reassignCSI;
}ClAmsInvocationT;

typedef ClRcT (*ClAmsInvocationCallbackT)
        (ClAmsInvocationT *invocationData,ClPtrT arg);

/*-----------------------------------------------------------------------------
 * Interface functions exposed to rest of AMS for managing invocations
 *---------------------------------------------------------------------------*/

extern ClRcT clAmsInvocationListInstantiate(
        CL_INOUT  ClCntHandleT  *invocationList );

extern ClRcT clAmsInvocationListTerminate(ClCntHandleT *invocationList);

extern ClRcT clAmsInvocationCreate(
        CL_IN  ClAmsInvocationCmdT  cmd,
        CL_IN  ClAmsCompT  *comp,
        CL_IN  ClAmsCSIT  *csi,
        CL_OUT  ClInvocationT  *invocation);

extern ClRcT clAmsInvocationCreateExtended(
        CL_IN  ClAmsInvocationCmdT  cmd,
        CL_IN  ClAmsCompT  *comp,
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClBoolT    reassignCSI,
        CL_OUT  ClInvocationT  *invocation);

extern ClRcT clAmsInvocationGet(
        CL_IN  ClInvocationT  invocation,
        CL_INOUT  ClAmsInvocationT  *data );

extern ClRcT clAmsInvocationGetAndDelete(
        CL_IN  ClInvocationT  invocation,
        CL_INOUT  ClAmsInvocationT  *data );

extern ClRcT clAmsInvocationGetAndDeleteExtended(
        CL_IN  ClInvocationT  invocation,
        CL_INOUT  ClAmsInvocationT  *data,
        ClBoolT invocationUpdate );

extern ClRcT clAmsInvocationDeleteAll(
        CL_IN  ClAmsCompT  *comp );

extern ClRcT clAmsInvocationListWalk(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT (*fn)(ClAmsCompT *c, ClInvocationT i, ClRcT e, ClUint32T s),
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode,
        CL_IN   ClAmsInvocationCmdT invocationCmd);

/*-----------------------------------------------------------------------------
 * Interface functions to update CSIs and check status of pending operations
 *---------------------------------------------------------------------------*/

extern ClRcT clAmsInvocationUpdateCSI(
        ClAmsCompT *comp,
        ClAmsCSIT *csi,
        ClUint32T command);

extern ClRcT clAmsInvocationListUpdateCSIAll(ClBoolT updateCSI);

extern ClUint32T clAmsInvocationPendingForComp(
        ClAmsCompT *comp,
        ClAmsInvocationCmdT cmd);

extern ClUint32T clAmsInvocationSetPendingForComp(ClAmsCompT *comp);

extern ClUint32T clAmsInvocationsPendingForComp(
        ClAmsCompT *comp);

extern ClUint32T clAmsInvocationsPendingForSU(
        ClAmsSUT *su);

extern ClUint32T clAmsInvocationPendingForSU(
        ClAmsSUT *su, ClAmsInvocationCmdT cmd);

extern ClUint32T clAmsInvocationSetPendingForSU(ClAmsSUT *su);

extern ClUint32T clAmsInvocationsPendingForSI(
        ClAmsSIT *si,
        ClAmsSUT *su);

extern ClUint32T clAmsInvocationsSetPendingForSI(
        ClAmsSIT *si,
        ClAmsSUT *su);

extern ClBoolT 
clAmsInvocationPendingForCSI(ClAmsSUT *su, ClAmsCompT *comp, ClAmsCSIT *csi, ClAmsInvocationCmdT op);

extern ClBoolT
clAmsInvocationPendingForCompCSI(ClAmsCompT *comp, ClAmsCSIT *csi, ClAmsInvocationCmdT op);


/*-----------------------------------------------------------------------------
 * Internal functions
 *---------------------------------------------------------------------------*/

extern ClRcT   clAmsInvocationListAdd(
        CL_IN  ClCntHandleT  invocationList,
        CL_IN  ClAmsInvocationT  *invocationData );

extern ClRcT   clAmsInvocationListAddAll(
        CL_IN ClAmsInvocationT *invocationData);

extern ClRcT   clAmsInvocationListDelete(
        CL_IN  ClCntHandleT  invocationList,
        CL_IN  ClAmsInvocationT  *invocationData );

extern ClRcT   clAmsInvocationListDeleteAll(
        CL_IN  ClCntHandleT  invocationList,
        CL_IN  ClCharT  *compName );

extern ClRcT clAmsInvocationListDeleteAllInvocations(
        CL_IN  ClCntHandleT  listHandle );

/*-----------------------------------------------------------------------------
 * Internal container manipulation functions
 *---------------------------------------------------------------------------*/

extern ClRcT clAmsCntDeleteElement(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClCntDataHandleT  element ,
        CL_IN  ClCntKeyHandleT  keyHandle, 
        CL_IN  ClRcT (*clCntDeleteCallback) (
                        ClCntDataHandleT element,
                        ClCntDataHandleT data,
                        ClBoolT *match),
        CL_IN  ClUint32T  flag);

extern ClRcT clAmsCntMatchInvocationInstance(
        CL_IN  ClCntDataHandleT  element,
        CL_IN  ClCntDataHandleT  dataElement,
        CL_OUT  ClBoolT  *match);

extern ClRcT clAmsCntMatchInvocationInstanceName(
        CL_IN  ClCntDataHandleT  element,
        CL_IN  ClCntDataHandleT  dataElement,
        CL_OUT  ClBoolT  *match);

extern ClRcT clAmsCntFindInvocationElement(
        CL_IN  ClCntHandleT  listHandle,    
        CL_IN  ClInvocationT  invocation,
        CL_OUT  ClAmsInvocationT  *invocationData);

extern ClRcT clAmsInvocationFindCSI(
                                    ClAmsCompT *comp,
                                    ClAmsCSIT  *csi,
                                    ClUint16T  pendingOp,
                                    ClAmsInvocationT *invocationData);

extern ClRcT clAmsInvocationsPendingForSG(ClAmsSGT *sg);

extern ClRcT clAmsInvocationsPendingForNode(ClAmsNodeT *node);

extern ClRcT clAmsInvocationPendingForSI(ClAmsSIT *si);
                                    
extern ClRcT clAmsInvocationListWalkAll(
                                        ClAmsInvocationCallbackT callback,
                                        ClPtrT arg,
                                        ClBoolT continueOnFailure);

extern void clAmsInvocationLock(void);

extern void clAmsInvocationUnlock(void);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_INVOCATION_H_ */
