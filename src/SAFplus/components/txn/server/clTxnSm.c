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
 * File        : clTxnSm.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This sub-module contains core of transaction management - 2 Phase Commit
 * protocol implementation. This module understands well-defined
 * used for communication with various agents. Each active transaction would
 * have an associated state-machine defined in this sub-module.
 *
 *
 *****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clSmExtendedApi.h>

#include <clTxnApi.h>
#include <clTxnErrors.h>

#include <clTxnStreamIpi.h>

#include "clTxnEngine.h"
#include "clTxnProtocolDefn.h"
#include "clTxnServiceIpi.h"
#include "clTxnRecovery.h"


static ClRcT _clTxnInit(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnPrepare(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxn1PCCommit(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxn2PCCommit(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnFini(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static  ClPtrT          clTxnExecTmplt;

extern ClTxnServiceT    *clTxnServiceCfg;

/**
 * This function creates protocol state-machine for transaction processing.
 */
ClRcT clTxnExecTemplateInitialize()
{
    ClRcT   rc  = CL_OK;
    ClSmStatePtrT       tmpStatePtr;
    ClSmTransitionPtrT  tmpTransPtr;
    ClSmTemplatePtrT    pSmTemplate;

    CL_FUNC_ENTER();

    rc = clSmTypeCreate ( (ClUint16T) CL_TXN_SM_STATE_MAX, &pSmTemplate);
    if (CL_OK != rc)
        goto error;

    /* This is a initialize state for transaction processing to begin */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_INIT, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where PREPARE cmd has been sent and response is
       awaited */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_PREPARING, _clTxnPrepare, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where COMMIT for all 1-PC jobs has been sent
       and response is awaited */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_COMMITTING_1PC, _clTxn1PCCommit, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where COMMIT for all jobs has been sent
       and response is awaited
    */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_COMMITTING_2PC, _clTxn2PCCommit, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where transaction is being aborted
       and ROLLBACK cmd has been sent. */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_ROLLING_BACK, _clTxnRollback, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a final state of transaction where transaction is completed */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_SM_STATE_COMPLETE, _clTxnFini, NULL);
    if (CL_OK != rc)
        goto error;

    /* Define transitions on various events */

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_INIT, 
                               CL_TXN_REQ_INIT, 
                               CL_TXN_SM_STATE_INIT, _clTxnInit, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* This transition corresponding user (external) request to process 
       transaction. The SM reaches transient state of PREPARING after
       sending cmds to various agents */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_INIT, 
                               CL_TXN_REQ_PREPARE, 
                               CL_TXN_SM_STATE_PREPARING, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* The SM transitions to commit phase once success (CL_OK) is received
       from various agents */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_PREPARING, 
                               CL_TXN_RESP_SUCCESS,
                               CL_TXN_SM_STATE_COMMITTING_1PC, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* The SM transitions to abort phase if preparation phase results in failure */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_PREPARING, 
                               CL_TXN_RESP_FAILURE,
                               CL_TXN_SM_STATE_ROLLING_BACK, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* On arrival of success (CL_OK) from 1-PC capable components, the SM transitions
       to transient state of committing all 2-PC capable components */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_COMMITTING_1PC,
                               CL_TXN_RESP_SUCCESS,
                               CL_TXN_SM_STATE_COMMITTING_2PC, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* If, while committing 1PC component, failure is observed, reach rolling back state
    */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_COMMITTING_1PC, 
                               CL_TXN_RESP_FAILURE,
                               CL_TXN_SM_STATE_ROLLING_BACK, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* On arrival of success (CL_OK) from commit phase, transaction reaches final/stable
       state */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_COMMITTING_2PC, 
                               CL_TXN_RESP_SUCCESS, 
                               CL_TXN_SM_STATE_COMPLETE, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* If, while committing 2PC, if failure is observed, then reach complete-state
       and wait for recovery thread to take action.
    */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_COMMITTING_2PC,
                               CL_TXN_RESP_FAILURE,
                               CL_TXN_SM_STATE_COMPLETE, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* On arrival of failure from commit phase, transaction reaches final state while 
       marking (log) for inconsistent state of system */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_ROLLING_BACK, 
                               CL_TXN_RESP_SUCCESS, 
                               CL_TXN_SM_STATE_COMPLETE, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* If while rolling back, failure is observed, reach complete and wait for 
       recovery thread to take action.
    */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, CL_TXN_SM_STATE_ROLLING_BACK,
                               CL_TXN_RESP_FAILURE,
                               CL_TXN_SM_STATE_COMPLETE, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* Set initial state */
    rc = clSmTypeInitStateSet(pSmTemplate, CL_TXN_SM_STATE_INIT);
    if (CL_OK != rc)
        goto error;

    clTxnExecTmplt = pSmTemplate; 

error:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create txn-exec template. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
        /* FIXME: Do clean up */
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This function finalizes transaction-processing sm-type
 */
ClRcT clTxnExecTemplateFinalize()
{
    ClRcT   rc  =   CL_OK;

    CL_FUNC_ENTER();

    rc = clSmTypeDelete(clTxnExecTmplt);

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete txn-exec template. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Creates new transaction exec context.
 */
ClRcT clTxnNewExecContextCreate(
        CL_OUT  ClTxnExecContextT   *pNewExecCntxt)
{
    ClRcT                   rc  = CL_OK;
    ClExSmInstancePtrT      exSmInstance;

    CL_FUNC_ENTER();

    /* Create new exec-context */ 
    rc = clEsmInstanceCreate(clTxnExecTmplt, 
                              &exSmInstance); 

    if (CL_OK == rc) 
        *pNewExecCntxt = (ClTxnExecContextT) exSmInstance;
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create txn exec context. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
} 

ClRcT clTxnExecContextDelete(
        CL_IN   ClTxnExecContextT   txnExecContext)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    rc = clEsmInstanceDelete( (ClExSmInstancePtrT )txnExecContext);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Post event to transaction state-machine for the given context.
 */
ClRcT clTxnSmPostEvent(
        CL_IN   ClTxnExecContextT   execContext, 
        CL_IN   ClSmEventPtrT       evt)
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();

    rc = clEsmInstanceEventAdd( (ClExSmInstancePtrT) execContext, evt);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnSmProcessEvents(
        CL_IN   ClTxnExecContextT   execContext)
{
    ClRcT   rc = CL_OK;
    CL_FUNC_ENTER();

    rc = clEsmInstanceProcessEvents((ClExSmInstancePtrT) execContext);

    if (CL_OK != rc)
    {
        if (CL_ERR_INVALID_STATE == CL_GET_ERROR_CODE(rc) )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Event not defined for current-state 0x%x of SM", 
                            ((ClExSmInstancePtrT) execContext)->fsm->current->type));
            rc = CL_OK; /* Ignore this for the moment */
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Failed to process event for TxnSm at state:0x%x, rc:0x%x", 
                            ((ClExSmInstancePtrT) execContext)->fsm->current->type, rc));
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}
/**
 * Internal Function: This is the entry function to transaction initialization phase.
 * As transaction enters this state, it requests all protocol handlers for all related
 * involved jobs to initialize.
 */
static ClRcT 
_clTxnInit(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           index;
    ClTxnActiveTxnT     *pActiveTxn;

    CL_FUNC_ENTER();

    /* Retrieve Active-Txn instance from msg and post INIT event to
       all job-sm
       FIXME: Initialize for transaction logs generation and storage.
     */
    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    } 
    
    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_ACTIVE;
    pActiveTxn->pendingRespCount = 0;
    pActiveTxn->rcvdStatus = CL_OK; /* Resetting */

    for (index = 0; (CL_OK == rc) && (index < pActiveTxn->pTxnDefn->jobCount); index++)
    {
        ClTxnActiveJobT     *pActiveJob;
        /* Post INIT cmd to job-state-machine */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_EVENT_INIT,
            .payload = (ClPtrT) &(pActiveTxn->pActiveJobList[index])
        };

        pActiveJob = &(pActiveTxn->pActiveJobList[index]);
        rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvt);
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to process for all active-job of this txn:0x%x, rc:0x%x", 
                 pActiveTxn->pTxnDefn->serverTxnId.txnId, rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function. When transaction protocol receives user-command to 
 * run/commit the transaction, it transitions from INIT-state to PREPARING-state
 * and requests each job to prepare for txn-completion (according to 2-PC commit protocol).
 */
static ClRcT 
_clTxnPrepare(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           index;
    ClTxnActiveTxnT     *pActiveTxn;

    CL_FUNC_ENTER();

    /* Retrieve Active-Txn instance from msg and post PREPARE event to
       all job-sm
    */
    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    if (pActiveTxn->pendingRespCount != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Something is wrong as pendingRespCount supposed to be zero :0x%x", 
                 pActiveTxn->pendingRespCount)); 
    
    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_PREPARING;
    pActiveTxn->pendingRespCount = pActiveTxn->pTxnDefn->jobCount;
    pActiveTxn->rcvdStatus = CL_OK;   /* Resetting */

    for (index = 0; (CL_OK == rc) && (index < pActiveTxn->pTxnDefn->jobCount); index++)
    {
        ClTxnActiveJobT     *pActiveJob;
        /* Post PREPARE cmd to job-state-machine */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_EVENT_PREPARE,
            .payload = (ClPtrT) &(pActiveTxn->pActiveJobList[index])
        };

        pActiveJob = &(pActiveTxn->pActiveJobList[index]);

        rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvt);
        if(CL_OK != rc)
        {
            clLogError("SER", "CTM", 
                    "Processing event failed for job[0x%x] with error [0x%x]",
                    pActiveJob->pAppJobDefn->jobId.jobId,
                    rc);
        }
    }

    if (CL_OK == rc)
    {
        clLogTrace("SER", "TSM", "Checkpointing state of txn [0x%x:0x%x]", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
    }
    clLogDebug("SER", NULL,
            "Txn[0x%x:0x%x] is at state[PREPARING]",
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
            pActiveTxn->pTxnDefn->serverTxnId.txnId);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: When transaction completes PREPARE-phase, in order to support
 * 1-PC capable nodes (with certain restrictions), enters state of 1-PC commit
 * to commit such nodes before proceeding further.
 */
static ClRcT 
_clTxn1PCCommit(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           index;
    ClTxnActiveTxnT     *pActiveTxn;

    CL_FUNC_ENTER();

    /* 
       Retrieve Active-Txn from msg and check txn-config
       If no job exists for 1-PC, then move to 2-PC Commit stat

       Else, post COMMIT_1PC to those jobs which contains 1-PC capable
       nodes.
    */
    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    if (pActiveTxn->pendingRespCount != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Something is wrong as pendingRespCount supposed to be zero :0x%x", 
                 pActiveTxn->pendingRespCount)); 

    if ( ! (pActiveTxn->pTxnDefn->txnCfg & (CL_TXN_COMPONENT_CFG_1_PC_CAPABLE << 0x8)) )
    {
        ClSmEventT  txnSelfEvt = {
            .eventId    =   CL_TXN_RESP_SUCCESS,
            .payload    =   msg->payload
        };

        clLogTrace("SER", "TSM", 
                "Transaction[0x%x:0x%x] has no 1-PC Job. Skipping", 
                 pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                 pActiveTxn->pTxnDefn->serverTxnId.txnId); 

        rc = clTxnSmPostEvent(pActiveTxn->txnExecContext, &txnSelfEvt);

        CL_FUNC_EXIT();
        return (rc);
    }

    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_COMMITTING;
    pActiveTxn->pendingRespCount = 0;
    pActiveTxn->rcvdStatus = CL_OK; /* Resetting */

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Transaction [0x%x:0x%x] going in for 1PC-COMMIT PHASE", 
             pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pActiveTxn->pTxnDefn->serverTxnId.txnId));

    for (index = 0; (CL_OK == rc) && (index < pActiveTxn->pTxnDefn->jobCount); index++)
    {
        ClTxnActiveJobT     *pActiveJob;
        /* Post COMMIT_1PC cmd to job-state-machine */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_EVENT_COMMIT_1PC,
            .payload = (ClPtrT) &(pActiveTxn->pActiveJobList[index])
        };

        pActiveJob = &(pActiveTxn->pActiveJobList[index]);

        if (pActiveJob->pAppJobDefn->jobCfg & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE)
        { 
            rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvt); 

            if (CL_OK == rc) 
               pActiveTxn->pendingRespCount++;
        }
    }

    if (CL_OK == rc)
    {
        clLogTrace("SER", "TSM", "Checkpointing state of txn [0x%x:0x%x]", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: After successful PREPARE-phase and successful commit of any
 * 1-PC capable nodes, transaction enters 2-PC committing state. In this state,
 * it sends out commands to all nodes to complete (commit) the transaction.
 */
static ClRcT 
_clTxn2PCCommit(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           index;
    ClTxnActiveTxnT     *pActiveTxn;

    CL_FUNC_ENTER();
    
    /* Retrieve Active-Txn from msg and check txn-config.
       Post COMMIT_2PC to all jobs.
    */

    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    if (pActiveTxn->pendingRespCount != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Something is wrong as pendingRespCount supposed to be zero :0x%x", 
                 pActiveTxn->pendingRespCount));

    clLogDebug("SER", "TSM", 
            "Transaction [0x%x:0x%x] going in for 2PC-COMMIT PHASE", 
             pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pActiveTxn->pTxnDefn->serverTxnId.txnId);

    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_COMMITTING;
    pActiveTxn->pendingRespCount = pActiveTxn->pTxnDefn->jobCount;
    pActiveTxn->rcvdStatus = CL_OK;     /* Resetting */

    for (index = 0; (CL_OK == rc) && (index < pActiveTxn->pTxnDefn->jobCount); index++)
    {
        ClTxnActiveJobT     *pActiveJob;
        /* Post COMMIT_2PC cmd to job-state-machine */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_EVENT_COMMIT_2PC,
            .payload = (ClPtrT) &(pActiveTxn->pActiveJobList[index])
        };

        pActiveJob = &(pActiveTxn->pActiveJobList[index]);

        rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvt);
    }

    if (CL_OK == rc)
    {
        clLogTrace("SER", "TSM", "Checkpointing state of txn [0x%x:0x%x]", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
    }
    clLogDebug("SER", NULL,
            "Txn[0x%x:0x%x] is at state[COMMITTING]",
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
            pActiveTxn->pTxnDefn->serverTxnId.txnId);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: If PREPARE-phase fails or (1-PC commit fails), transaction
 * enters rollback-mode. In this state, transaction requests all nodes to 
 * rollback (temporary) changes done to the system.
 */
static ClRcT 
_clTxnRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClUint32T           index;
    ClTxnActiveTxnT     *pActiveTxn;

    CL_FUNC_ENTER();

#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Rolling back the Txn SM\n"));
#endif
    /* Retrieve Active-Txn from msg and post ABORT to all jobs. */
    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    if (pActiveTxn->pendingRespCount != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Something is wrong as pendingRespCount supposed to be zero :0x%x", 
                 pActiveTxn->pendingRespCount));

    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_ROLLING_BACK;
    pActiveTxn->pendingRespCount = pActiveTxn->pTxnDefn->jobCount;
    pActiveTxn->rcvdStatus = CL_OK;     /* Resetting */

    for (index = 0; (CL_OK == rc) && (index < pActiveTxn->pTxnDefn->jobCount); index++)
    {
        ClTxnActiveJobT     *pActiveJob;
        /* Post COMMIT_2PC cmd to job-state-machine */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_EVENT_ROLLBACK,
            .payload = (ClPtrT) &(pActiveTxn->pActiveJobList[index])
        };

        pActiveJob = &(pActiveTxn->pActiveJobList[index]);

        rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvt);
    }

    if (CL_OK == rc)
    {
        clLogTrace("SER", "TSM", 
                "Checkpointing state of txn [0x%x:0x%x]", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
    }
    clLogDebug("SER", NULL,
            "Txn[0x%x:0x%x] is at state[ROLLBACK]",
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
            pActiveTxn->pTxnDefn->serverTxnId.txnId);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: Transaction enters FINI state for its completion 
 * (either via commit or rollback). In this case, it does the clean-up
 * and also finalizes transaction logs.
 */
static ClRcT
_clTxnFini(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClTxnActiveTxnT     *pActiveTxn;
    ClTxnCommHandleT    commHandle;
    CL_FUNC_ENTER();

    /* Retrieve Active-Txn from msg and post ABORT to all jobs. */
    pActiveTxn = (ClTxnActiveTxnT *) msg->payload;
    if (NULL == pActiveTxn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Null arguement in msg-payload. Cannot retrieve active-txn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    if (pActiveTxn->pendingRespCount != 0)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Something is wrong as pendingRespCount supposed to be zero :0x%x", 
                 pActiveTxn->pendingRespCount));
    
    rc = clTxnCommIfcNewSessionCreate(
            CL_TXN_MSG_MGR_RESP, 
            pActiveTxn->clientAddr, 
            CL_TXN_CLIENT_MGR_RESP_RECV, 
            NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
            CL_TXN_COMMON_ID, /* Value is 0x1 */ 
            &commHandle);

    if ( (prevState->type == CL_TXN_SM_STATE_COMMITTING_2PC) )
    {
        ClTxnCmdT   cmd = {
            .txnId = pActiveTxn->pTxnDefn->clientTxnId,
            .jobId = {{0,0}, 0},
           // .resp  = (msg->eventId == CL_TXN_RESP_SUCCESS)?CL_TXN_STATE_COMMITTED:CL_TXN_STATE_UNKNOWN,
            .resp  = CL_TXN_STATE_COMMITTED, /* A transaction cannot fail. There is no UNKNOWN state. Commit and Rollback is assumed to be always successful */

            .cmd   = CL_TXN_CMD_RESPONSE
        };
        /* Always COMMITTED or ROLLED_BACK at this stage */
        clLog(CL_LOG_INFO, "SER", NULL,
                "Transaction with txnId [0x%x] COMMITTED", 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_COMMITTED;
        /*
        if( msg->eventId == CL_TXN_RESP_SUCCESS)
        {
            pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_COMMITTED;
            clLog(CL_LOG_INFO, "SER", NULL,
                    "Successfully completed the transaction mgrAddr:txnID [0x%x:0x%x]: COMMITTED", 
                   pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                    pActiveTxn->pTxnDefn->serverTxnId.txnId); 
        }
        else
            pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;
            */

        rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
        if (CL_OK == rc)
        {
            clLogTrace("SER", "TSM", 
                    "Checkpointing state of txn [0x%x:0x%x]", 
                    pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                    pActiveTxn->pTxnDefn->serverTxnId.txnId);
            /* This is not required and it is immediately deleted in clTxnRecoverySessionFini */
            //rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
        }
    }
    else if ( (prevState->type == CL_TXN_SM_STATE_ROLLING_BACK) )
    {
        ClTxnCmdT   cmd = {
            .txnId = pActiveTxn->pTxnDefn->clientTxnId,
            .jobId = {{0,0}, 0},
            //.resp  =  (msg->eventId == CL_TXN_RESP_SUCCESS)?CL_TXN_STATE_ROLLED_BACK:CL_TXN_STATE_UNKNOWN,
            .resp  =  CL_TXN_STATE_ROLLED_BACK, /* There is no UNKNOWN state. If Prepare succeeds, then Commit and Rollback are assumed to be always successful */
            .cmd   = CL_TXN_CMD_RESPONSE
        };
        ClBufferHandleT  agentList;
        ClUint32T               agentListLen;
        ClUint8T                *pAgentListStr = NULL;

        clLog(CL_LOG_INFO, "SER", NULL,
                "Transaction with txnId [0x%x:0x%x] ROLLED BACK", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        rc = clBufferCreate (&agentList);
        if(CL_OK != rc)
        {
            clLog(CL_LOG_ERROR, "SER", NULL,
                    "Error in creating buffer while trying to send response to the client rc [0x%x]", rc);
            return rc;
        }
        clLog(CL_LOG_DEBUG, "SER", NULL,
                "Packing job info list to send to client for transaction[0x%x:0x%x], failed count[%d]",
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                 pActiveTxn->pTxnDefn->serverTxnId.txnId,
                pActiveTxn->failedAgentCount);
        clTxnStreamTxnAgentDataPack(&(pActiveTxn->failedAgentCount),pActiveTxn->failedAgentList,agentList);
        rc = clBufferLengthGet(agentList, &agentListLen);

        if (CL_OK == rc)
        {
           pAgentListStr = (ClUint8T *) clHeapAllocate(agentListLen);
            if (pAgentListStr == NULL)
                rc = CL_ERR_NO_MEMORY;
            else
                memset(pAgentListStr, 0, agentListLen);
        }

        if (CL_OK == rc)
        {
            ClUint32T   tLen = agentListLen;
            rc = clBufferNBytesRead(agentList, (ClUint8T *)pAgentListStr, &tLen);
        }

        if (CL_OK != rc)
        {
            clHeapFree(pAgentListStr);
            clBufferDelete(&agentList);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize buffer for job-defn. rc:0x%x", rc));
            CL_FUNC_EXIT();
            return CL_GET_ERROR_CODE(rc);
        }
        /* Always COMMITTED or ROLLED_BACK at this stage */
        pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
/*
        if( msg->eventId == CL_TXN_RESP_SUCCESS)
        {
            pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
        clLog(CL_LOG_INFO, "SER", NULL,
                "Successfully completed the transaction with mgrAdd:txnId[0x%x:0x%x]: ROLLEDBACK", 
                       pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveTxn->pTxnDefn->serverTxnId.txnId);
        }
        else
            pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;
*/
        //rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
        rc = clTxnCommIfcSessionAppendPayload(commHandle, &cmd, pAgentListStr, agentListLen);
        if (CL_OK == rc)
        {
            clLog(CL_LOG_INFO, "SER", NULL, 
                    "Removing checkpointed txndefn [0x%x:0x%x] from Recovery Log", 
                    pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                    pActiveTxn->pTxnDefn->serverTxnId.txnId);
            /* Dont update here since this will be deleted in clTxnRecoverySessionFini() */
            //rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
        }
    clHeapFree(pAgentListStr);
    clBufferDelete(&agentList);
    }
    else 
    {
        clLog(CL_LOG_ERROR, "SER", NULL,
                "Invalid prevState [%d] for txn[0x%x:0x%x]. Should be either COMMITTING or ROLLING_BACK",
                prevState->type, 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        ClTxnCmdT   cmd = {
            .txnId = pActiveTxn->pTxnDefn->clientTxnId,
            .jobId = {{0,0}, 0},
            .resp  = CL_TXN_STATE_UNKNOWN,
            .cmd   = CL_TXN_CMD_RESPONSE
        };
        pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;

        rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
        if (CL_OK == rc)
        {
            clLogTrace("SER", "TSM", 
                    "Checkpointing state of txn [0x%x:0x%x]", 
                    pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                    pActiveTxn->pTxnDefn->serverTxnId.txnId);
            /* This is not required and it is immediately deleted in clTxnRecoverySessionFini */
            //rc = clTxnServiceCkptRecoveryLogCheckpoint(pActiveTxn->pTxnDefn->serverTxnId);
        }
    }
    /* The recovery db alongwith the checkpoint is removed at all cases now */
    clLogDebug("SER", NULL,
            "Cleaning recovery session of transaction[0x%x:0x%x]", 
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
            pActiveTxn->pTxnDefn->serverTxnId.txnId);
    if(CL_OK != clTxnRecoverySessionFini(pActiveTxn->pTxnDefn->serverTxnId))
    {
        clLog(CL_LOG_ERROR, "SER", NULL,
                "Cleaning up Recovery db failed [0x%x]", 
                rc);
                
    }
    rc = clTxnCommIfcSessionRelease(commHandle);
    if(CL_OK != rc)
    {
        clLogError("SER", NULL,
                "Failed to release session with error [0x%x]",
                rc);
        return rc;
    }

    clLogDebug("SER", "TSM", 
            "Sending response to client [0x%x:0x%x] for txn[0x%x:0x%x]", 
            pActiveTxn->clientAddr.nodeAddress, 
            pActiveTxn->clientAddr.portId, 
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
            pActiveTxn->pTxnDefn->serverTxnId.txnId);
    rc = clTxnCommIfcDeferSend(commHandle, pActiveTxn->rmdDeferHdl, pActiveTxn->deferOutMsg);
    if(CL_OK != rc)
    {
        clLogError("SER", NULL,
                "Failed while defer send with error [0x%x] for txn[0x%x:0x%x]",
                rc,
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
        return rc;
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClCharT* clTxnTMStateGet(ClUint16T state)
{
    switch(state)
    {
        case  CL_TXN_SM_STATE_PREPARING:
            return "PREPARING";
        case  CL_TXN_SM_STATE_COMMITTING_1PC:
            return "COMMITTING_1PC";
        case  CL_TXN_SM_STATE_COMMITTING_2PC:
            return "COMMITTING_2PC";
        case  CL_TXN_SM_STATE_ROLLING_BACK:
            return "ROLLING_BACK";
        case  CL_TXN_SM_STATE_COMPLETE:
            return "COMPLETE";
        default:
            return "INVALID_STATE";
    }
}
ClCharT* clTxnTMEventIdGet(ClUint16T eId)
{
    switch(eId)
    {
        case CL_TXN_REQ_INIT:
            return "INIT";
        case CL_TXN_REQ_PREPARE:
            return "PREPARE";
        case CL_TXN_RESP_SUCCESS:
            return "SUCCESS";
        case CL_TXN_RESP_FAILURE:
            return "FAILURE";
        case CL_TXN_REQ_CANCEL:
            return "CANCEL";
        default:
            return "INVALID_ID";
    }
}
