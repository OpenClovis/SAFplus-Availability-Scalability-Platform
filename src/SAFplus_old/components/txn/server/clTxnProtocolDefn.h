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
 * ModuleName  : txn                                                           
 * File        : clTxnProtocolDefn.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module declares data-structures and functions related to 
 * transaction (Two-Phase Commit) protocol.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_PROTOCOL_DEFN_H_
#define _CL_TXN_PROTOCOL_DEFN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clSmTemplateApi.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/


/******************************************************************************
 *  Data Types 
 *****************************************************************************/

typedef ClPtrT          ClTxnExecContextT;

typedef ClPtrT          ClTxnJobExecContextT;

/**
 *  Transaction manager and agent maintain last known state of each other.
 *  Each agent refresh the timer used for aborting current transaction if
 *  manager fails to respond within stipulated time.
 */
typedef enum 
{
    CL_TXN_SM_STATE_INIT,
    CL_TXN_SM_STATE_PREPARING,
    CL_TXN_SM_STATE_COMMITTING_1PC,
    CL_TXN_SM_STATE_COMMITTING_2PC,
    CL_TXN_SM_STATE_ROLLING_BACK,
    CL_TXN_SM_STATE_COMPLETE,
    CL_TXN_SM_STATE_MAX
} ClTxnSmStateT;

typedef enum 
{
    CL_TXN_JOB_SM_STATE_PRE_INIT,
    CL_TXN_JOB_SM_STATE_INIT,
    CL_TXN_JOB_SM_STATE_PREPARING,
    CL_TXN_JOB_SM_STATE_PREPARED,
    CL_TXN_JOB_SM_STATE_COMMITTING_1PC,
    CL_TXN_JOB_SM_STATE_COMMITTING_2PC,
    CL_TXN_JOB_SM_STATE_COMMITTED_1PC,
    CL_TXN_JOB_SM_STATE_COMMITTED_2PC,
    CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK,
    CL_TXN_JOB_SM_STATE_ROLLING_BACK,
    CL_TXN_JOB_SM_STATE_ROLLED_BACK,
    CL_TXN_JOB_SM_STATE_FAILED,
    CL_TXN_JOB_SM_STATE_MAX
} ClTxnJobSmStateT;

#define CL_TXN_JOB_SM_STATE_COMMITTED       CL_TXN_JOB_SM_STATE_COMMITTED_2PC

/**
 * List of transaction protocol messages exchanged between transaction-service and
 * various agents
 */
typedef enum
{
    CL_TXN_REQ_INVALID       = 0,
    CL_TXN_REQ_INIT,
    CL_TXN_REQ_PREPARE,
    CL_TXN_RESP_SUCCESS,
    CL_TXN_RESP_FAILURE,
    CL_TXN_REQ_CANCEL,
    CL_TXN_EVENT_MAX
} ClTxnSmEventsT;

typedef enum
{
    CL_TXN_JOB_EVENT_INVALID        =   0,
    CL_TXN_JOB_EVENT_INIT,
    CL_TXN_JOB_EVENT_PREPARE,
    CL_TXN_JOB_RESP_SUCCESS,
    CL_TXN_JOB_RESP_FAILURE,
    CL_TXN_JOB_EVENT_PREPARE_FOR_ROLLBACK,
    CL_TXN_JOB_EVENT_COMMIT_1PC,
    CL_TXN_JOB_EVENT_COMMIT_2PC,
    CL_TXN_JOB_EVENT_ROLLBACK,
    CL_TXN_JOB_EVENT_RESTART,
    CL_TXN_JOB_EVENT_MAX
} ClTxnJobSmEventsT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

extern ClRcT clTxnExecTemplateInitialize();
extern ClRcT clTxnExecTemplateFinalize();

extern ClRcT clTxnNewExecContextCreate(
        CL_OUT  ClTxnExecContextT   *pNewExecCntxt);

extern ClRcT clTxnExecContextDelete(
        CL_IN   ClTxnExecContextT   execContext);

extern ClRcT clTxnSmPostEvent(
        CL_IN   ClTxnExecContextT   execContext, 
        CL_IN   ClSmEventPtrT       evt);

extern ClRcT clTxnSmProcessEvents(
        CL_IN   ClTxnExecContextT   execContext);

extern ClRcT clTxnJobExecTemplateInitialize();

extern ClRcT clTxnJobExecTemplateFinalilze();

extern ClRcT clTxnJobNewExecContextCreate(
        CL_IN   ClTxnJobExecContextT    *pNewExecCntxt);

extern ClRcT clTxnJobExecContextDelete(
        CL_IN   ClTxnJobExecContextT    execContext);

extern ClRcT clTxnJobSmProcessEvent(
        CL_IN   ClTxnJobExecContextT    execContext, 
        CL_IN   ClSmEventPtrT           evt);

extern ClCharT* clTxnJobEventIdGet(ClUint16T);
extern ClCharT* clTxnJobStateGet(ClUint16T);
extern ClCharT* clTxnTMStateGet(ClUint16T);
extern ClCharT* clTxnTMEventIdGet(ClUint16T);
#if 0
/**
 * Initializes protocol internal state-machine and 
 * data-structure.
 */
ClRcT clTxnProtocolInit(CL_OUT ClTxnProtocolTemplateHandleT *pTxnProtocolTmplt);

/**
 * Finalizes protocol internal state-machine and data-structures
 */
ClRcT clTxnProtocolFini(CL_IN ClTxnProtocolTemplateHandleT txnProtocolTmplt);

/**
 * Creates a new instance of protocol handler
 */
ClRcT clTxnProtocolInstanceCreate(CL_IN ClTxnProtocolTemplateHandleT txnProtocolTmplt, 
                                  CL_OUT ClTxnProtocolHandleT *txnProtocolHndl);

/**
 * Deletes an instance of protocol handler
 */
ClRcT clTxnProtocolInstanceFinalize(CL_IN ClTxnProtocolHandleT txnProtocolHndl);

/**
 * Runs an instantiated protocol handler
 */
ClRcT clTxnProtocolInstanceRun(CL_IN ClTxnProtocolHandleT txnProtocolHndl);

/**
 * Posts an event to an active protocol.
 */
ClRcT clTxnProtocolInstancePostEvent(CL_IN ClTxnProtocolHandleT txnProtocolHndl, 
                                     CL_IN ClSmEventPtrT pEvent);

#endif
#ifdef __cplusplus
}
#endif

#endif /* _CL_TXN_PROTOCOL_DEFN_H_ */
