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
 * File        : clTxnJobSm.c
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

#include <clTxnCommonDefn.h>
#include <clTxnStreamIpi.h>

#include "clTxnEngine.h"
#include "clTxnProtocolDefn.h"
#include "clTxnRecovery.h"

/**
 * Forward declaration : Entry & transition functions
 */
static ClRcT _clTxnJob1PCCommitted(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobCommitted(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobRolledBack(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobFailed(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobDefnDistribute(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobPrepareForCommit(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobPrepared(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobCommit1PC(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobCommit2PC(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobPrepareForRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static ClRcT _clTxnJobRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg);

static  ClPtrT       clTxnJobExecTmplt;

/**
 * This function creates protocol state-machine type for job processing.
 */
ClRcT clTxnJobExecTemplateInitialize()
{
    ClRcT               rc  =   CL_OK;
    ClSmStatePtrT       tmpStatePtr;
    ClSmTransitionPtrT  tmpTransPtr;
    ClSmTemplatePtrT    pSmTemplate;

    CL_FUNC_ENTER();

    rc = clSmTypeCreate ( (ClUint16T) CL_TXN_JOB_SM_STATE_MAX, &pSmTemplate);
    if (CL_OK != rc)
        goto error;

    /* This is the initialization state after creating sm-instance for each
       active job to be executed.  */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_PRE_INIT, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_INIT, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is the transient state where PREPARE command has been sent 
       and response is being waited */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_PREPARING, _clTxnJobPrepareForCommit, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where transaction-job has been marked for
       ROLLBACK.  */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where all involved agents have prepared
       for execution of transaction-job. */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_PREPARED, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where 1-PC capable component instance is
       being committed */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_COMMITTING_1PC, _clTxnJobCommit1PC, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where 1-PC capable component instance is 
       committed */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_COMMITTED_1PC, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This is a transient state where the job is being committed */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_COMMITTING_2PC, _clTxnJobCommit2PC, NULL);
    if (CL_OK != rc)
        goto error;

    /* This state signifies a consistent state of the system where 
       job is committed. */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_COMMITTED, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This state signifies a transient state where ABORT cmd is being sent to 
       various agents and response is awaited.  */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_ROLLING_BACK, _clTxnJobRollback, NULL);
    if (CL_OK != rc)
        goto error;

    /* This state signifies a consistent state of system where transaction 
       is successfully aborted.  */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_ROLLED_BACK, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* This state signifies failure of transaction for this job where either 
       ROLLBACK did not succeed or COMMIT did not succeed even though 
       PREPARE was successful */
    SM_TYPE_STATE_CREATE(rc, pSmTemplate, CL_TXN_JOB_EVENT_MAX, tmpStatePtr, 
                         CL_TXN_JOB_SM_STATE_FAILED, NULL, NULL);
    if (CL_OK != rc)
        goto error;

    /* Define transitions for this job state-machine */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PRE_INIT, CL_TXN_JOB_EVENT_INIT, 
                               CL_TXN_JOB_SM_STATE_INIT, _clTxnJobDefnDistribute, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_INIT, CL_TXN_JOB_EVENT_PREPARE, 
                               CL_TXN_JOB_SM_STATE_PREPARING, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARING, CL_TXN_JOB_RESP_SUCCESS, 
                               CL_TXN_JOB_SM_STATE_PREPARED, _clTxnJobPrepared, tmpTransPtr);
    if (CL_OK != rc)
        goto error;
    
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARING, CL_TXN_JOB_RESP_FAILURE, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, _clTxnJobPrepareForRollback, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARING, CL_TXN_JOB_EVENT_PREPARE_FOR_ROLLBACK, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, _clTxnJobPrepareForRollback, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate,
                               CL_TXN_JOB_SM_STATE_PREPARED, CL_TXN_JOB_EVENT_COMMIT_1PC, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_1PC, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate,
                               CL_TXN_JOB_SM_STATE_PREPARED, CL_TXN_JOB_RESP_SUCCESS, 
                               CL_TXN_JOB_SM_STATE_PREPARED, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARED, CL_TXN_JOB_EVENT_COMMIT_2PC, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_2PC, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARED, CL_TXN_JOB_EVENT_PREPARE_FOR_ROLLBACK, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, _clTxnJobPrepareForRollback, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_PREPARED, CL_TXN_JOB_EVENT_ROLLBACK, 
                               CL_TXN_JOB_SM_STATE_ROLLING_BACK, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_1PC, CL_TXN_JOB_RESP_SUCCESS, 
                               CL_TXN_JOB_SM_STATE_COMMITTED_1PC, _clTxnJob1PCCommitted, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_1PC, CL_TXN_JOB_RESP_FAILURE, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, _clTxnJobPrepareForRollback, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTED_1PC, CL_TXN_JOB_EVENT_COMMIT_2PC, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_2PC, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;
/* Transient: Defining this state transition to go from Committed 1pc to
*  rolling back state */
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTED_1PC,
                               CL_TXN_JOB_EVENT_ROLLBACK, 
                               CL_TXN_JOB_SM_STATE_ROLLING_BACK, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;
    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_2PC, CL_TXN_JOB_RESP_SUCCESS, 
                               CL_TXN_JOB_SM_STATE_COMMITTED, _clTxnJobCommitted, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_COMMITTING_2PC, CL_TXN_JOB_RESP_FAILURE, 
                               CL_TXN_JOB_SM_STATE_FAILED, _clTxnJobFailed, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, CL_TXN_JOB_EVENT_ROLLBACK, 
                               CL_TXN_JOB_SM_STATE_ROLLING_BACK, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK, CL_TXN_JOB_EVENT_RESTART, 
                               CL_TXN_JOB_SM_STATE_INIT, NULL, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_ROLLING_BACK, CL_TXN_JOB_RESP_SUCCESS, 
                               CL_TXN_JOB_SM_STATE_ROLLED_BACK, _clTxnJobRolledBack, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    SM_STATE_TRANSITION_CREATE(rc, pSmTemplate, 
                               CL_TXN_JOB_SM_STATE_ROLLING_BACK, CL_TXN_JOB_RESP_FAILURE, 
                               CL_TXN_JOB_SM_STATE_FAILED, _clTxnJobFailed, tmpTransPtr);
    if (CL_OK != rc)
        goto error;

    /* Set the initial state */
    rc = clSmTypeInitStateSet(pSmTemplate, CL_TXN_JOB_SM_STATE_PRE_INIT);
    if (CL_OK != rc)
        goto error;

    clTxnJobExecTmplt = pSmTemplate;

error:

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create txn-job exec-template. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
        /* FIXME: Do clean up */
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This function finalizes job-processing execution-template
 */
ClRcT clTxnJobExecTemplateFinalilze()
{
    ClRcT   rc  =   CL_OK;

    CL_FUNC_ENTER();

    rc = clSmTypeDelete((ClSmTemplatePtrT)clTxnJobExecTmplt);

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete job-exec template. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnJobNewExecContextCreate(
        CL_IN   ClTxnJobExecContextT    *pNewExecCntxt)
{
    ClRcT                   rc  =   CL_OK;
    ClExSmInstancePtrT      exSmInstance;

    CL_FUNC_ENTER();

    /* Create new exec-context */ 
    rc = clEsmInstanceCreate((ClSmTemplatePtrT) clTxnJobExecTmplt, 
                              &exSmInstance); 

    if (CL_OK == rc) 
    {
        *pNewExecCntxt = (ClTxnJobExecContextT) exSmInstance;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create job-exec context. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }


    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnJobExecContextDelete(
        CL_IN   ClTxnJobExecContextT    execContext)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    rc = clEsmInstanceDelete( (ClExSmInstancePtrT ) execContext);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnJobSmProcessEvent(
        CL_IN   ClTxnJobExecContextT    execContext, 
        CL_IN   ClSmEventPtrT           evt)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    /* FIXME */
    rc = clEsmInstanceEventAdd( (ClExSmInstancePtrT) execContext, evt);

    if (CL_OK == rc)
    {
        clLogDebug("SER", NULL,
                "Successfully added event[%s] into Job SM [%s]", 
                clTxnJobEventIdGet(evt->eventId),
                clTxnJobStateGet(((ClExSmInstancePtrT) execContext)->fsm->current->type));
        rc = clEsmInstanceProcessEvents( (ClExSmInstancePtrT) execContext);
    }

    
    if (CL_OK != rc)
    {
        if (CL_ERR_INVALID_STATE == CL_GET_ERROR_CODE(rc) )
        {
            clLogError("SER", NULL,
                           "Event [%s] not defined for current-state [%s] of Job SM", 
                            clTxnJobEventIdGet(evt->eventId), 
                            clTxnJobStateGet(((ClExSmInstancePtrT) execContext)->fsm->current->type));
            rc = CL_OK; /* Ignore this for the moment */
        }
        else
        {
            clLogError("SER", "CTM", 
                    "Failed to process event [%s] for Job SM at state [%s], rc [0x%x]", 
                    clTxnJobEventIdGet(evt->eventId), 
                    clTxnJobStateGet(((ClExSmInstancePtrT) execContext)->fsm->current->type), rc);
        }

    }
    else
        clLogDebug("SER", NULL,
                "Event [%s] successfully processed for Job SM at state [%s]",
                clTxnJobEventIdGet(evt->eventId),
                clTxnJobStateGet(((ClExSmInstancePtrT) execContext)->fsm->current->type));

    CL_FUNC_EXIT();
    return (rc);
}

/******************************************************************************
 *                 Internal Functions (Entry/Exit/Transition Fns)             *
 ******************************************************************************/

/**
 * Transition Function : Sm is transitioning into MARKED_FOR_ROLLBACK
 */
static ClRcT 
_clTxnJobPrepareForRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClTxnActiveJobT     *pActiveJob;

    CL_FUNC_ENTER();
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n_clTxnJobPrepareForRollback\n"));
#endif
    /*
       State-machine enters MARKED_ROLLBACK when failure is detected during 
       prepare-phase or while committing 1-PC capable components.
       Following tasks need to be done
       - Update txn-logs
       - Prepare for either complete rollback or restarting txn
       - Inform txn-sm with error msg
    */

#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Job Preparing for rollback\n"));
#endif
    pActiveJob = (ClTxnActiveJobT *) msg->payload;

    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid msg or payload, null arguement\n"));
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

#if 0
    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }

#endif

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_ROLLING_BACK;

    if ( (pActiveJob->pParentTxn != NULL) && 
         (msg->eventId == CL_TXN_JOB_RESP_FAILURE) )
    {
        /* 
           In the parent transaction, update the response received.
           If the pendingRespCount is zero, then post message based
           on success or failure of this phase of processing.
        */
        pActiveJob->pParentTxn->pendingRespCount--;
        pActiveJob->pParentTxn->rcvdStatus = 1;     /* FIXME: Set more meaningful error-code */
        if (pActiveJob->pParentTxn->pendingRespCount == 0)
        {
            ClSmEventT  jobSmEvt = {
                .eventId = CL_TXN_RESP_FAILURE,
                .payload = (ClPtrT) (pActiveJob->pParentTxn)
            };

#ifdef CL_TXN_DEBUG
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Posting failure event to the txn SM \n"));
#endif
            rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transition function from preparing-state to prepared-state
 */
static ClRcT 
_clTxnJobPrepared(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClTxnActiveJobT     *pActiveJob;

#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "%s, Prepared now\n", __FUNCTION__));
#endif
    CL_FUNC_ENTER();
    /*
       State-machine enters PREPARED-state, when all participating components 
       respond positively (CL_OK) for PREPARE cmd. Following tasks need to be done -
       - Update txn-log for successful completion of PREPARE phase.
       - Inform txn-sm with CL_OK msg
    */

    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }
#if 0

    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }
#endif

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_PREPARED;

    clLogDebug("SER", NULL, "Active-Job [0x%x] is PREPARED", pActiveJob->pAppJobDefn->jobId.jobId);


    if (pActiveJob->pParentTxn != NULL)
    {
        /* 
           In the parent transaction, update the response received.
           If the pendingRespCount is zero, then post message based
           on success or failure of this phase of processing.
        */
        pActiveJob->pParentTxn->pendingRespCount--;
        pActiveJob->pParentTxn->rcvdStatus |= CL_OK;

        clLogDebug("SER", "JSM", 
            "For parent-txn [0x%x:0x%x], pendingRespCount [0x%x], rcvdStatus [0x%x]", 
             pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
             pActiveJob->pParentTxn->pendingRespCount, 
             pActiveJob->pParentTxn->rcvdStatus);

        if (pActiveJob->pParentTxn->pendingRespCount == 0)
        {
            ClSmEventT  jobSmEvt = {
                .eventId = CL_TXN_RESP_SUCCESS,
                .payload = (ClPtrT) (pActiveJob->pParentTxn)
            };

            if (pActiveJob->pParentTxn->rcvdStatus != CL_OK)
                jobSmEvt.eventId = CL_TXN_RESP_FAILURE;

            rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
            clLogDebug("SER", "JSM", 
                    "Result of posting event to txn-sm from job-sm, rc [0x%x]", rc);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transition function for committing-1pc-state to Committed-1PC-state
 */
static ClRcT 
_clTxnJob1PCCommitted(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc  = CL_OK;
    ClTxnActiveJobT         *pActiveJob;

    CL_FUNC_ENTER();
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCommited1PC\n"));
#endif
    /*
       State-machine enters 1PC-Committed state, when this job has one of components
       capable of 1-PC only, and it has successfully completed COMMIT operation.

       - Update txn-log for successful completion of PREPARE phase.
       - Inform txn-sm with CL_OK msg
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

#if 0
    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }
#endif

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_COMMITTING;

    if (pActiveJob->pParentTxn != NULL)
    {
        /* 
           In the parent transaction, update the response received.
           If the pendingRespCount is zero, then post message based
           on success or failure of this phase of processing.
        */
        pActiveJob->pParentTxn->pendingRespCount--;
        pActiveJob->pParentTxn->rcvdStatus |= CL_OK;
        if (pActiveJob->pParentTxn->pendingRespCount == 0)
        {
            ClSmEventT  jobSmEvt = {
                .eventId = CL_TXN_RESP_SUCCESS,
                .payload = (ClPtrT) (pActiveJob->pParentTxn)
            };

            if (pActiveJob->pParentTxn->rcvdStatus != CL_OK)
                jobSmEvt.eventId = CL_TXN_RESP_FAILURE;

            /* Call ClTxnSm::Post Event */
            rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transition function for committing to 2PC-Committed-state
 */
static ClRcT 
_clTxnJobCommitted(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClTxnActiveJobT         *pActiveJob;

    CL_FUNC_ENTER();
    /*
       State-machine enters 2PC-Committed state, when all participating components
       complete COMMIT phase and return ack successfully. Following task need to be done -

       - Update txn-log for successful completion of COMMIT phase.
       - Inform txn-sm with CL_OK msg
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (!pActiveJob || !pActiveJob->pParentTxn 
            || !pActiveJob->pAppJobDefn || !pActiveJob->pParentTxn->pTxnDefn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

#if 0
    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }
#endif

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_COMMITTED;

    /* 
       In the parent transaction, update the response received.
       If the pendingRespCount is zero, then post message based
       on success or failure of this phase of processing.
    */
    pActiveJob->pParentTxn->pendingRespCount--;
    pActiveJob->pParentTxn->rcvdStatus |= CL_OK;
    clLogDebug("SER", NULL,
            "Job having jobId[0x%x] completed",
            pActiveJob->pAppJobDefn->jobId.jobId);
    if (pActiveJob->pParentTxn->pendingRespCount == 0)
    {
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_RESP_SUCCESS,
            .payload = (ClPtrT) (pActiveJob->pParentTxn)
        };

        if (pActiveJob->pParentTxn->rcvdStatus != CL_OK)
            jobSmEvt.eventId = CL_TXN_RESP_FAILURE;

        /* Call ClTxnSm::Post Event */
        clLogDebug("SER", NULL,
                "All jobs corresponding to transaction [0x%x:0x%x] has be processed. Posting event[%s] to Txn SM at state [%s]",
                pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId,
                clTxnJobEventIdGet(jobSmEvt.eventId),
                clTxnTMStateGet(pActiveJob->pParentTxn->pTxnDefn->currentState));
        rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
    }
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transition function for rolling-back to ROLLED_BACK state
 */
static ClRcT 
_clTxnJobRolledBack(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc      = CL_OK;
    ClTxnActiveJobT     *pActiveJob;

    CL_FUNC_ENTER();
#ifdef  CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: inside %s, job rolled back\n", __FUNCTION__));
#endif
    /*
       State-machine enters rolled-back state, when all participating components
       successfully abort current transaction and return ack successfully. 
       Following task need to be done -

       - Update txn-log for successful completion of ABORT phase.
       - Inform txn-sm with CL_OK msg
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (!pActiveJob || !pActiveJob->pParentTxn 
            || !pActiveJob->pAppJobDefn || !pActiveJob->pParentTxn->pTxnDefn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

#if 0
    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }
#endif

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_ROLLED_BACK;

    /* 
       In the parent transaction, update the response received.
       If the pendingRespCount is zero, then post message based
       on success or failure of this phase of processing.
    */
    pActiveJob->pParentTxn->pendingRespCount--;
    pActiveJob->pParentTxn->rcvdStatus |= CL_OK;
    if (pActiveJob->pParentTxn->pendingRespCount == 0)
    {
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_RESP_SUCCESS,
            .payload = (ClPtrT) (pActiveJob->pParentTxn)
        };

        if (pActiveJob->pParentTxn->rcvdStatus != CL_OK)
            jobSmEvt.eventId = CL_TXN_RESP_FAILURE;

        /* Call ClTxnSm::Post Event */
        clLogDebug("SER", NULL, "Posting event[%s] to Txn SM",
                clTxnJobEventIdGet(jobSmEvt.eventId)); 
        rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transition function for FAILED state (FIXME)
 */
static ClRcT 
_clTxnJobFailed(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT               rc  = CL_OK;
    ClTxnActiveJobT     *pActiveJob;

    CL_FUNC_ENTER();
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Inside %s\n, rolling back failed\n", __FUNCTION__));
#endif
    /*
       State-machine enters FAILED state, when either there was a fatal error 
       while executing transaction - either component failed to COMMIT or
       failed to ABORT transaction successfully. 

       Following task need to be done -
       - Update txn-log for successful completion of ABORT phase.
       - Inform txn-sm with CL_OK msg
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

#if 0
    rc = clTimerStop(pActiveJob->txnJobTimerHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to stop the timer for txn-job[0x%x:0x%x-0x%x], rc:0x%x", 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                       pActiveJob->pAppJobDefn->jobId.jobId, rc));
        rc = CL_OK;     /* Ignoring it now */
    }

#endif
    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_UNKNOWN;

    if (pActiveJob->pParentTxn != NULL)
    {
        /* 
           In the parent transaction, update the response received.
           If the pendingRespCount is zero, then post message based
           on success or failure of this phase of processing.
        */
        pActiveJob->pParentTxn->pendingRespCount--;
        if ( msg->eventId == CL_TXN_JOB_RESP_SUCCESS) 
        {
            pActiveJob->pParentTxn->rcvdStatus |= CL_OK;
        }
        else if ( msg->eventId == CL_TXN_JOB_RESP_FAILURE)
        {
            pActiveJob->pParentTxn->rcvdStatus = 1;
        }

        if (pActiveJob->pParentTxn->pendingRespCount == 0)
        {
            ClSmEventT  jobSmEvt = {
                .eventId = CL_TXN_RESP_SUCCESS,
                .payload = (ClPtrT) (pActiveJob->pParentTxn)
            };

            if (pActiveJob->pParentTxn->rcvdStatus != CL_OK)
                jobSmEvt.eventId = CL_TXN_RESP_FAILURE;

            /* Call ClTxnSm::Post Event */
#ifdef CL_TXN_DEBUG
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Posting %s event to the TxnSM\n",
                    jobSmEvt.eventId ==
                    CL_TXN_RESP_SUCCESS?"Success":"Failure"));
#endif
            rc = clTxnSmPostEvent(pActiveJob->pParentTxn->txnExecContext, &jobSmEvt);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/******************************************************************************
 *                 Internal Functions (Transition Fn)                         *
 ******************************************************************************/
/**
 * Transition function from pre-init to init-state
 */
static ClRcT 
_clTxnJobDefnDistribute(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClRcT                   retCode = CL_OK;
    ClTxnActiveJobT         *pActiveJob;
    ClCntNodeHandleT        currNode;
    ClBufferHandleT  jobDefn;
    ClUint32T               index   = 0;
    ClUint8T                *pJobDefnStr = NULL;
    ClUint32T               jobDefnLen;

    CL_FUNC_ENTER();
    /*
       As state-machine enters INIT state, following check task need to be done.
       - Initialize communication interface/IDL (if necessary)
       - Distribute job definitions to all involved/participating components
       - Initialize for transaction logs generation and storage.
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    rc = clBufferCreate (&jobDefn);

    if (CL_OK == rc)
    {
        rc = clTxnStreamTxnDataPack(pActiveJob->pParentTxn->pTxnDefn, 
                                    pActiveJob->pAppJobDefn, jobDefn);
    }

    if (CL_OK != rc)
    {
        clBufferDelete (&jobDefn);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to pack txn/job for agents. rc:0x%x", rc));
        CL_FUNC_EXIT();
        return CL_GET_ERROR_CODE(rc);
    }

    rc = clBufferLengthGet(jobDefn, &jobDefnLen);

    if (CL_OK == rc)
    {
        pJobDefnStr = (ClUint8T *) clHeapAllocate(jobDefnLen);
        if (pJobDefnStr == NULL)
            rc = CL_ERR_NO_MEMORY;
        else
            memset(pJobDefnStr, 0, jobDefnLen);
    }

    if (CL_OK == rc)
    {
        ClUint32T   tLen = jobDefnLen;
        rc = clBufferNBytesRead(jobDefn, (ClUint8T *)pJobDefnStr, &tLen);
    }

    if (CL_OK != rc)
    {
        clHeapFree(pJobDefnStr);
        clBufferDelete(&jobDefn);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize buffer for job-defn. rc:0x%x", rc));
        CL_FUNC_EXIT();
        return CL_GET_ERROR_CODE(rc);
    }

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_ACTIVE;

    retCode = clCntFirstNodeGet(pActiveJob->pAppJobDefn->compList, &currNode);
    while ( (retCode == CL_OK) && (CL_OK == rc) )
    {
        ClTxnAppComponentT      *pAppComp;
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        retCode = clCntNodeUserDataGet( pActiveJob->pAppJobDefn->compList, currNode, 
                                   (ClCntDataHandleT *)&pAppComp);
        if (CL_OK == retCode)
        {
            ClTxnCmdT   cmd = {
                .txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId,
                .jobId = pActiveJob->pAppJobDefn->jobId,
                .cmd   = CL_TXN_CMD_INIT,
            };
            ClTxnCommHandleT    commHandle;

            rc = clTxnRecoverySetComponent(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                           pActiveJob->pAppJobDefn->jobId,
                                           pAppComp->appCompAddress, 
                                           pAppComp->configMask);

            if (CL_OK == rc)
            {
                rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_MGR_CMD, 
                                                  pAppComp->appCompAddress, 
                                                  CL_TXN_AGENT_MGR_CMD_RECV, 
                                                  clTxnEngineAgentAsyncResponseRecv, 
                                                  CL_TXN_JOB_PROCESSING_TIME, 
                                                  CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                  &commHandle);
            }

            clLogDebug("SER", "JSM", 
                    "Sending txn-defn to agent [0x%x:0x%x], rc [0x%x]", 
                            pAppComp->appCompAddress.nodeAddress, 
                            pAppComp->appCompAddress.portId, rc);
            if (CL_OK == rc)
            {
                /* Pack job-defn and send to agent */
                rc = clTxnCommIfcSessionAppendPayload(commHandle, &cmd, pJobDefnStr, jobDefnLen);
                rc = clTxnCommIfcSessionRelease(commHandle);
                pActiveJob->pAgntRespList[index].agentAddr = pAppComp->appCompAddress;
                pActiveJob->pAgntRespList[index].status = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                pActiveJob->pAgntRespList[index].cmd = CL_TXN_CMD_INVALID;
                pActiveJob->pAgntRespList[index].respRcvd = CL_OK;
                pAppComp->compIndex = index;
                index++;
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to open comm-channel with dest 0x%x:0x%x", 
                                pAppComp->appCompAddress.nodeAddress, 
                                pAppComp->appCompAddress.portId));
            }
            node = currNode; 
            retCode = clCntNextNodeGet( pActiveJob->pAppJobDefn->compList, node, &currNode);
        }
    }

    clBufferDelete (&jobDefn);
    clHeapFree(pJobDefnStr);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Entry state for PREPARING_STATE
 */
static ClRcT 
_clTxnJobPrepareForCommit(
        CL_IN       ClSmStatePtrT   prevState, 
        CL_INOUT    ClSmStatePtrT   *nextState, 
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClRcT                   retCode = CL_OK;
    ClTxnActiveJobT         *pActiveJob;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();
    /*
       As state-machine is leaving INIT state and entering PREPARING state, 
       following task need to be done -
       - Send PREPARE command to all the participating components
       - Generate log message for this job for having sent PREPARE cmd to agents.
    */

    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    } 
    
    clLogInfo("SER", NULL,
            "Preparing for commit for job[0x%x]",
            pActiveJob->pAppJobDefn->jobId.jobId);
    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_PREPARING;
    pActiveJob->agentRespCount = 0;
    
    /* Prepare txn-command for all participating components of this job */
    retCode = clCntFirstNodeGet(pActiveJob->pAppJobDefn->compList, &currNode);
    
    while ( (CL_OK == retCode) && (CL_OK == rc) )
    {
        ClTxnAppComponentT      *pAppComp;
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        retCode = clCntNodeUserDataGet( pActiveJob->pAppJobDefn->compList, currNode, 
                                   (ClCntDataHandleT *)&pAppComp);
        if (CL_OK == retCode)
        {
        
            /* multiple 1 pc */ 
            if (!(pAppComp->configMask & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE) )
            {
                /* Include this component for PREPARE phase */
                ClTxnCmdT   cmd = {
                    .txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId,
                    .jobId = pActiveJob->pAppJobDefn->jobId,
                    .cmd   = CL_TXN_CMD_PREPARE
                };

                ClTxnCommHandleT    commHandle;

                rc = clTxnCommIfcNewSessionCreate( CL_TXN_MSG_MGR_CMD, 
                                                    pAppComp->appCompAddress, 
                                                    CL_TXN_AGENT_MGR_CMD_RECV, 
                                                    clTxnEngineAgentAsyncResponseRecv, 
                                                    CL_TXN_JOB_PROCESSING_TIME,
                                                    CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                    &commHandle);
                clLogDebug("SER", "MTA", 
                        "Sending CL_TXN_CMD_PREPARE to agent [0x%x:0x%x], rc [0x%x]", 
                        pAppComp->appCompAddress.nodeAddress, 
                        pAppComp->appCompAddress.portId, rc);
                if (CL_OK == rc)
                {
                
                    rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
                    if(CL_OK != rc)
                    {
                        clLogError("SER", NULL,
                                "Preparing for commit failed, error in appending cmd for key, rc [0x%x]",
                                rc);
                    }
                    rc = clTxnCommIfcSessionRelease(commHandle);
                    pActiveJob->agentRespCount++;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].status = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd = CL_OK;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].cmd = CL_TXN_CMD_PREPARE;
                    
                    rc = clTxnRecoveryPrepareCmdSent(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                                     pActiveJob->pAppJobDefn->jobId,
                                                     pAppComp->appCompAddress);
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                                  ("Failed to open comm-channel with dest 0x%x:0x%x", 
                                   pAppComp->appCompAddress.nodeAddress, 
                                   pAppComp->appCompAddress.portId));
                }
            }
            node = currNode; 
            retCode = clCntNextNodeGet( pActiveJob->pAppJobDefn->compList, node, &currNode);
        }
    }

    if (pActiveJob->agentRespCount == 0)
    {
        /* Post CL_TXN_JOB_RESP_SUCCESS event to self */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_RESP_SUCCESS,
            .payload = pActiveJob
        }; 
        
#ifdef CL_TXN_DEBUG
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: In prepare for 1PC, addig success event to Job SM\n"));
#endif
        rc = clEsmInstanceEventAdd( (ClExSmInstancePtrT) pActiveJob->jobExecContext, 
                                    &jobSmEvt);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to post CL_TXN_JOB_RESP_SUCCESS msg to self. rc:0x%x", rc));
        }
              
    }

    /*
    else
    {
        rc = clTimerStart(pActiveJob->txnJobTimerHandle);
    }
 
    */
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Entry function for commiting 1-pc components
 */
static ClRcT 
_clTxnJobCommit1PC(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClRcT                   retCode = CL_OK;
    ClTxnActiveJobT         *pActiveJob;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();
    /*
       Once PREPARE phase is completed successfully, transaction could initiate COMMIT
       in 1-PC capable components (if there are any).
       Task to be done
       - Send COMMIT cmd to pre-selected 1-PC capable component
       - Update txn logs
    */
    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_COMMITTING;
    pActiveJob->agentRespCount = 0;

    clLogDebug("SER", "JSM", 
            "Active-Txn/Job [0x%x:0x%x][0x%x] in COMMITTING PHASE", 
             pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
             pActiveJob->pAppJobDefn->jobId.jobId);
    
    /* Prepare txn-command for all participating components of this job */
    retCode = clCntFirstNodeGet(pActiveJob->pAppJobDefn->compList, &currNode);
    
    while ( (CL_OK == retCode) && (CL_OK == rc) )
    {
        ClTxnAppComponentT      *pAppComp;
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        retCode = clCntNodeUserDataGet( pActiveJob->pAppJobDefn->compList, currNode, 
                                  (ClCntDataHandleT *)&pAppComp);
        if (CL_OK == retCode)
        {
            /* multiple 1 pc */ 
            if ( pAppComp->configMask & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE )
            {
                /* Include this component for COMMIT-1PC phase */
                ClTxnCmdT   cmd = {
                    .txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId,
                    .jobId = pActiveJob->pAppJobDefn->jobId,
                    .cmd   = CL_TXN_CMD_1PC_COMMIT,
                };
                ClTxnCommHandleT    commHandle;

                rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_MGR_CMD, 
                                                  pAppComp->appCompAddress, 
                                                  CL_TXN_AGENT_MGR_CMD_RECV, 
                                                  clTxnEngineAgentAsyncResponseRecv, 
                                                  CL_TXN_JOB_PROCESSING_TIME, 
                                                  CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                  &commHandle);
                if (CL_OK == rc)
                {
                    rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
                    rc = clTxnCommIfcSessionRelease(commHandle);
                    pActiveJob->agentRespCount++;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].status = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd = CL_OK;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].cmd = CL_TXN_CMD_1PC_COMMIT;
                    rc = clTxnRecoveryCommitCmdSent(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                                    pActiveJob->pAppJobDefn->jobId,
                                                    pAppComp->appCompAddress);
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                                  ("Failed to open comm-channel with dest 0x%x:0x%x", 
                                   pAppComp->appCompAddress.nodeAddress, 
                                   pAppComp->appCompAddress.portId));
                }
                /* Transient: Allow multiple single phase in a single JOb */
                break;  /* As only one such component could exists */
            }
            node = currNode; 
            retCode = clCntNextNodeGet( pActiveJob->pAppJobDefn->compList, node, &currNode);
        }
    }

    if (pActiveJob->agentRespCount == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No 1-PC components for this job"));
        /* FIXME */
    }
#if 0
    else
    {
        /* Start timer for detecting failure of COMMIT-1PC PHASE */
        rc = clTimerStart(pActiveJob->txnJobTimerHandle);
    }
#endif

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Entry function for commiting 2-PC components.
 */
static ClRcT 
_clTxnJobCommit2PC(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClRcT                   retCode = CL_OK;
    ClTxnActiveJobT         *pActiveJob;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();

    /*
       Once PREPARE phase is completed successfully, transaction could initiate COMMIT
       in participating components
       Task to be done
       - Send COMMIT cmd to all participating components
       - Update txn logs
    */

    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_COMMITTING;
    pActiveJob->agentRespCount = 0;
    
    /* Prepare txn-command for all participating components of this job */
    retCode = clCntFirstNodeGet(pActiveJob->pAppJobDefn->compList, &currNode);
    
    while ( (CL_OK == retCode) && (CL_OK == rc) )
    {
        ClTxnAppComponentT      *pAppComp;
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        retCode = clCntNodeUserDataGet( pActiveJob->pAppJobDefn->compList, currNode, 
                                   (ClCntDataHandleT *)&pAppComp);
        if (CL_OK == retCode)
        {

            /* multiple 1 pc */ 
            if ( !(pAppComp->configMask & (CL_TXN_COMPONENT_CFG_1_PC_CAPABLE | 
                                           CL_TXN_COMPONENT_CFG_FOR_VERIFY_PHASE) ))
            {
                /* Include this component for COMMIT-2PC phase */
                ClTxnCmdT   cmd = {
                    .txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId,
                    .jobId = pActiveJob->pAppJobDefn->jobId,
                    .cmd   = CL_TXN_CMD_2PC_COMMIT,
                };

                ClTxnCommHandleT    commHandle;

                rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_MGR_CMD, 
                                                  pAppComp->appCompAddress, 
                                                  CL_TXN_AGENT_MGR_CMD_RECV, 
                                                  clTxnEngineAgentAsyncResponseRecv, 
                                                  CL_TXN_JOB_PROCESSING_TIME, 
                                                  CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                  &commHandle);
                clLogDebug("SER", "JSM", 
                        "Sending CL_TXN_CMD_COMMIT to agent [0x%x:0x%x], rc [0x%x]", 
                                pAppComp->appCompAddress.nodeAddress, 
                                pAppComp->appCompAddress.portId, rc);
                if (CL_OK == rc)
                {
                    rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
                    rc = clTxnCommIfcSessionRelease(commHandle);
                    pActiveJob->agentRespCount++;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].status = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd = CL_OK;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].cmd = CL_TXN_CMD_2PC_COMMIT;
                    rc = clTxnRecoveryCommitCmdSent(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                                    pActiveJob->pAppJobDefn->jobId,
                                                    pAppComp->appCompAddress);
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                                  ("Failed to open comm-channel with dest 0x%x:0x%x", 
                                   pAppComp->appCompAddress.nodeAddress, 
                                   pAppComp->appCompAddress.portId));
                }
            }
            node = currNode; 
            retCode = clCntNextNodeGet( pActiveJob->pAppJobDefn->compList, node, &currNode);
        }
    }

    if (pActiveJob->agentRespCount == 0)
    {
        /* Post CL_TXN_JOB_RESP_SUCCESS event to self */
        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_RESP_SUCCESS,
            .payload = pActiveJob
        }; 
        
        rc = clEsmInstanceEventAdd( (ClExSmInstancePtrT) pActiveJob->jobExecContext, 
                                    &jobSmEvt);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to post CL_TXN_JOB_RESP_SUCCESS msg to self. rc:0x%x", rc));
        }
    }
#if 0
    else
    {
        /* Start the timer for detecting failure of COMMIT-2PC PHASE */
        rc = clTimerStart(pActiveJob->txnJobTimerHandle);
    }
#endif

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Entry function for rolling back state
 */
static ClRcT 
_clTxnJobRollback(
        CL_IN       ClSmStatePtrT   prevState,
        CL_INOUT    ClSmStatePtrT   *nextState,
        CL_IN       ClSmEventPtrT   msg)
{
    ClRcT                   rc      = CL_OK;
    ClRcT                   retCode = CL_OK;
    ClTxnActiveJobT         *pActiveJob;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Rolling back the job\n"));
#endif
    /*
       State-machine transitions from marked-for-rollback to Rolling back using this
       transition function, 
       Task to be done -
       - Send ABORT cmd to all participating components
       - Update txn logs
    */

    pActiveJob = (ClTxnActiveJobT *) msg->payload;
    if (NULL == pActiveJob)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument or invalid active-job defn\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    pActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_ROLLING_BACK;
    pActiveJob->agentRespCount = 0;
    
    /* Prepare txn-command for all participating components of this job */
    retCode = clCntFirstNodeGet(pActiveJob->pAppJobDefn->compList, &currNode);
 
    while ( (CL_OK == retCode) && (CL_OK == rc) )
    {
        ClTxnAppComponentT      *pAppComp;
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        retCode = clCntNodeUserDataGet( pActiveJob->pAppJobDefn->compList, currNode, 
                                   (ClCntDataHandleT *)&pAppComp);
        if (CL_OK == retCode)
        {
            /* multiple 1 pc */ 
            if ( !(pAppComp->configMask & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE) )
            {
                /* Include this component for Rollback phase */
                ClTxnCmdT   cmd = {
                    .txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId,
                    .jobId = pActiveJob->pAppJobDefn->jobId,
                    .cmd   = CL_TXN_CMD_ROLLBACK,
                };
                ClTxnCommHandleT    commHandle;

                rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_MGR_CMD, 
                                                  pAppComp->appCompAddress, 
                                                  CL_TXN_AGENT_MGR_CMD_RECV, 
                                                  clTxnEngineAgentAsyncResponseRecv, 
                                                  CL_TXN_JOB_PROCESSING_TIME, 
                                                  CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                  &commHandle);
                clLogDebug("SER", "JSM", 
                        "Sending CL_TXN_CMD_ROLLBACK to agent [0x%x:0x%x], rc [0x%x]", 
                                pAppComp->appCompAddress.nodeAddress, 
                                pAppComp->appCompAddress.portId, rc);

                if (CL_OK == rc)
                {
                    rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
                    rc = clTxnCommIfcSessionRelease(commHandle);
                    pActiveJob->agentRespCount++;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].status = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd = CL_OK;
                    pActiveJob->pAgntRespList[pAppComp->compIndex].cmd = CL_TXN_CMD_ROLLBACK;
                    rc = clTxnRecoveryRollbackCmdSent(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                                      pActiveJob->pAppJobDefn->jobId,
                                                      pAppComp->appCompAddress);
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                                  ("Failed to open comm-channel with dest 0x%x:0x%x", 
                                   pAppComp->appCompAddress.nodeAddress, 
                                   pAppComp->appCompAddress.portId));
                }
            }
#ifdef CL_TXN_DEBUG
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Getting next comp, current agent count : %d\n",
                    pActiveJob->agentRespCount));
#endif
            node = currNode; 
            retCode = clCntNextNodeGet( pActiveJob->pAppJobDefn->compList, node, &currNode);
        }
    }

#if 0
    if (pActiveJob->agentRespCount > 0x0)
    {
        /* Start the timer for detecting failure for ROLLBACK PHASE */
        rc = clTimerStart(pActiveJob->txnJobTimerHandle);
    }
#endif
    if (pActiveJob->agentRespCount <= 0x0)
    {

        ClSmEventT  jobSmEvt = {
            .eventId = CL_TXN_JOB_RESP_FAILURE,
            .payload = pActiveJob
        }; 
        
#ifdef CL_TXN_DEBUG
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: In rollback, adding failure event to Job SM\n"));
#endif
        rc = clEsmInstanceEventAdd( (ClExSmInstancePtrT) pActiveJob->jobExecContext, 
                                    &jobSmEvt);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to post CL_TXN_JOB_RESP_FAILURE msg to self. rc:0x%x", rc));
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}
ClCharT* clTxnJobEventIdGet(ClUint16T eId)
{
    switch(eId)
    {
        case CL_TXN_JOB_EVENT_INIT:
            return "INIT";
        case CL_TXN_JOB_EVENT_PREPARE:
            return "PREPARE";
        case CL_TXN_JOB_RESP_SUCCESS:
            return "SUCCESS";
        case CL_TXN_JOB_RESP_FAILURE:
            return "FAILURE";
        case CL_TXN_JOB_EVENT_PREPARE_FOR_ROLLBACK:
            return "PREPARE_4_ROLLBACK";
        case CL_TXN_JOB_EVENT_COMMIT_1PC:
            return "COMMIT_1PC";
        case CL_TXN_JOB_EVENT_COMMIT_2PC:
            return "COMMIT_2PC";
        case CL_TXN_JOB_EVENT_ROLLBACK:
            return "ROLLBACK";
        case CL_TXN_JOB_EVENT_RESTART:
            return "RESTART";
        default:
            return "INVALID_EVENT";
    }

}
ClCharT* clTxnJobStateGet(ClUint16T state)
{
    switch(state)
    {
        case CL_TXN_JOB_SM_STATE_PRE_INIT:
            return "PRE_INIT";
        case CL_TXN_JOB_SM_STATE_INIT:
            return "INIT";
        case CL_TXN_JOB_SM_STATE_PREPARING:
            return "PREPARING";
        case CL_TXN_JOB_SM_STATE_PREPARED:
            return "PREPARED";
        case CL_TXN_JOB_SM_STATE_COMMITTING_1PC:
            return "COMMITTING_1PC";
        case CL_TXN_JOB_SM_STATE_COMMITTING_2PC:
            return "COMMITTING_2PC";
        case CL_TXN_JOB_SM_STATE_COMMITTED_1PC:
            return "COMMITTED_1PC";
        case CL_TXN_JOB_SM_STATE_COMMITTED_2PC:
            return "COMMITTED_2PC";
        case CL_TXN_JOB_SM_STATE_MARKED_ROLLBACK:
            return "MARKED_ROLLBACK";
        case CL_TXN_JOB_SM_STATE_ROLLING_BACK:
            return "ROLLING_BACK";
        case CL_TXN_JOB_SM_STATE_ROLLED_BACK:
            return "ROLLED_BACK";
        case CL_TXN_JOB_SM_STATE_FAILED:
            return "FAILED";
        default:
            return "INVALID_STATE";
    }
}
