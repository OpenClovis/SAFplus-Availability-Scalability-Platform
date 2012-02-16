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
 * File        : clTxnEngine.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This source file contains transaction processing engine for executing
 * all active transactions.
 *
 *
 *****************************************************************************/

#include <unistd.h>
#include <string.h>

#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clVersionApi.h>

#include <clTxnApi.h>
#include <clTxnErrors.h>

#include <xdrClTxnTransactionIdT.h>
#include <xdrClTxnTransactionJobIdT.h>
#include <xdrClTxnCmdT.h>
#include <xdrClTxnMessageHeaderIDLT.h>
#include <clTxnXdrCommon.h>

#include "clTxnServiceIpi.h"

#include "clTxnEngine.h"
#include "clTxnRecovery.h"

/* Forward Declarations (Private Functions) */

static ClRcT clTxnJobInfoAdd(
        ClTxnActiveTxnT         *pActiveTxn,
        ClBufferHandleT  msg,
        ClTxnTransactionJobIdT  jobId,
        ClTxnMgmtCommandT       cmd,
        ClIocPhysicalAddressT   appCompAddress,
        ClTxnActiveJobT         *pActiveJob,
        ClUint8T                phase,
        ClUint8T                noServNTimeout); 
static void _clTxnEngineRun(void *t);

static ClRcT _clTxnTxnExecContextCreate( 
        CL_IN   ClTxnDefnT              *pTxnDefn,
        CL_IN   ClIocPhysicalAddressT   clientAddr,
        CL_IN   ClBufferHandleT         outMsg, 
        CL_IN   ClUint8T                cli);

static ClRcT _clTxnEngineTxnStart(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg);

static ClRcT _clTxnEngineTxnCancel(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg);

static ClRcT _clTxnEngineTxnJobProcess(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg);

static ClRcT _clTxnTimerExpire(void *data);
static ClRcT    clTxnCompListAdd(
        ClTxnActiveJobT         *pActiveJob,
        ClIocPhysicalAddressT   *pAddr);

static ClRcT 
_clTxnEngineRecoveryLogUpdate(
        CL_IN   ClTxnActiveJobT     *pActiveJob);

static void 
_clTxnEngineReqDelFn(
        CL_IN   ClQueueDataT    data);

static void 
_clTxnEngineDequeueCallBack(ClQueueDataT userData);

static void _clTxnActiveTxnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

static void _clTxnActiveTxnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);
static ClInt32T _clTxnActiveTxnKeyCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

static ClUint32T  _clTxnActveTxnHashFn(
        ClCntKeyHandleT key);

static ClRcT _clTxnWalkActiveMap(
        CL_IN ClTxnActiveTxnT       *pActiveTxn,
        CL_IN ClIocPhysicalAddressT *pAddr);

static ClRcT clTxnRecoveryCleanup(
        ClTxnActiveTxnT *pActiveTxn);

ClTxnActiveJobT* clTxnGetActiveJob(ClTxnActiveTxnT* context, ClTxnAppJobDefnT* job)
{
    /* Make sure no one switcheroo the job count */
    CL_ASSERT(context->activeJobListLen == context->pTxnDefn->jobCount);
    /* Make sure that the job is in the correct range */
    CL_ASSERT(job->jobId.jobId < context->pTxnDefn->jobCount);

    return &(context->pActiveJobList[job->jobId.jobId]);
}

ClTxnActiveJobT* clTxnGetActiveJobById(ClTxnActiveTxnT* context, int idx)
{
    /* Make sure no one switcheroo the job count */
    CL_ASSERT(context->activeJobListLen == context->pTxnDefn->jobCount);
    /* Make sure that the job is in the correct range */
    CL_ASSERT(idx < context->pTxnDefn->jobCount);

    return &(context->pActiveJobList[idx]);
}



extern ClTxnServiceT    *clTxnServiceCfg;
extern ClVersionDatabaseT clTxnMgmtVersionDb;
#ifdef CL_TXN_DEBUG
typedef enum ClTxnOperationT {
        CL_TXN_APP_ADD,
            CL_TXN_APP_SUBTRACT
} ClTxnOperationT;
typedef struct clTxnAppJobDef
{
    ClTxnOperationT     opType;
    ClUint32T           arrIndex;
    ClUint32T           newValue;
} ClTxnAppJobT;
#endif

static ClTxnCntrlMsgT* AllocTxnCntrlMsg()
{
    ClTxnCntrlMsgT* pTxnCntrl = (ClTxnCntrlMsgT*) clHeapAllocate(sizeof(ClTxnCntrlMsgT));
    CL_ASSERT(pTxnCntrl);
    
    memset(pTxnCntrl, 0, sizeof(ClTxnCntrlMsgT));
    pTxnCntrl->structId = ClTxnCntrlMsgStructId;
    pTxnCntrl->guard    = ClTxnCntrlMsgStructId;
    return pTxnCntrl;
}

static void ReleaseTxnCntrlMsg(ClTxnCntrlMsgT* pTxnCntrl)
{
    CL_ASSERT(pTxnCntrl->structId == ClTxnCntrlMsgStructId);
    CL_ASSERT(pTxnCntrl->guard == ClTxnCntrlMsgStructId);
    clHeapFree(pTxnCntrl);
}

/** 
 * Transaction Core Engine Initialization Routine
 * 
 * This module contains transaction processing engine providing 
 * interface to execute/operate on active transactions.  
 * 
 * Transaction Engine initialize queue for server-thread to 
 */
ClRcT clTxnEngineInitialize()
{
    ClRcT   rc = CL_OK;

    CL_FUNC_ENTER();
    
    /* Initialize the queue(s) for receiving response and user-commands on 
       active transactions. 
    */
    if ( NULL == clTxnServiceCfg )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Un-initialized transaction service. NULL clTxnServiceCfg instance\n"));
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    clTxnServiceCfg->pTxnEngine = (ClTxnEngineT *) clHeapAllocate(sizeof(ClTxnEngineT));
    if (clTxnServiceCfg->pTxnEngine == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for txn-engine.\n"));
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }

    memset(clTxnServiceCfg->pTxnEngine, 0, sizeof(ClTxnEngineT));

    /* Create active-txn database */
    rc = clCntHashtblCreate(CL_TXN_ENGINE_ACTIVE_TXN_BUCKET_SIZE,
                            _clTxnActiveTxnKeyCompare, 
                            _clTxnActveTxnHashFn, 
                            _clTxnActiveTxnDelete, _clTxnActiveTxnDestroy, 
                            CL_CNT_UNIQUE_KEY,
                            &(clTxnServiceCfg->pTxnEngine->activeTxnMap) );

    /* Create request/response queues for receiving user request & agent response 
     */
    if (CL_OK == rc)
    {
        rc = clQueueCreate(0, _clTxnEngineDequeueCallBack, _clTxnEngineReqDelFn, 
                              &(clTxnServiceCfg->pTxnEngine->txnReqQueue));
        if(CL_OK != rc)
        {
            clLogError("SER", "ENG", 
                    "Failed to create ReqQueue, rc [0x%x]", rc);
        }
    }

    /* Initialize protocol state-machine for transaction processing */
    if (CL_OK == rc)
    {
        rc = clTxnExecTemplateInitialize();
    }

    /* Initialilze protocol state-machine for job processing */
    if (CL_OK == rc)
    {
        rc = clTxnJobExecTemplateInitialize();
    }

    /* Create mutex used for synchronization purpose */
    if (CL_OK == rc)
    {
        rc = clOsalMutexCreate(&(clTxnServiceCfg->pTxnEngine->txnEngineMutex));
    }

    /* Create conditional variable required for inter-thread signaling */
    if (CL_OK == rc)
    {
        rc = clOsalCondCreate(&(clTxnServiceCfg->pTxnEngine->txnCondVar));
    }

    if (CL_OK == rc)
    {
        rc = clTxnRecoveryLibInit();
    }

    /*
       Create task for executing all active transactions and processing responses
     */
    if (CL_OK == rc)
    {
        rc = clOsalTaskCreateAttached("TxnTask", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, 
                              ((void *(*)(void *))_clTxnEngineRun), 
                               NULL, &(clTxnServiceCfg->pTxnEngine->txnEngineTask));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize transaction engine. rc:0x%x", rc));
        /* FIXME: Remove allocate memory */
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}



/**
 * Transaction Engine finalization routine. 
 */
ClRcT clTxnEngineFinalize()
{
    ClRcT   rc = CL_OK;
    ClTxnCntrlMsgT      *pTxnCntrlMsg;

    CL_FUNC_ENTER();

    /* Check current status of active transactions.. and abort them as required */
    /* 
       - Delete conditional variable
       - Finalize active-txn database
       - Remove all queues
    */

    pTxnCntrlMsg = AllocTxnCntrlMsg();
    if ( pTxnCntrlMsg != NULL )
    {
        pTxnCntrlMsg->cmd = CL_TXN_ENGINE_TERMINATE;
        if ( (rc = clOsalMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex)) == CL_OK)
        {
            rc = clQueueNodeInsert(clTxnServiceCfg->pTxnEngine->txnReqQueue, 
                              (ClQueueDataT) pTxnCntrlMsg);
            if (CL_OK == rc)
                clOsalCondBroadcast(clTxnServiceCfg->pTxnEngine->txnCondVar);
            clOsalMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        }
    }
    else
        rc = CL_ERR_NO_MEMORY;

    if (CL_OK == rc)
    {
        /* Wait for a transaction-thread to complete pending tasks */
        if ( clOsalMutexLock (clTxnServiceCfg->pTxnEngine->txnEngineMutex) == CL_OK)
        {
            ClTimerTimeOutT timeOut = {
                .tsSec      = 1, 
                .tsMilliSec = 0
            };
            rc = clOsalCondWait(clTxnServiceCfg->pTxnEngine->txnCondVar, 
                                clTxnServiceCfg->pTxnEngine->txnEngineMutex, timeOut);
            if (CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(rc))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction thread failed to terminate. Aborting"));
            }

            clOsalMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        }
    }

    /* This will cleanup history db as well */
    clCntDelete (clTxnServiceCfg->pTxnEngine->activeTxnMap);

    rc = clTxnRecoveryLibFini();

    clOsalTaskDelete (clTxnServiceCfg->pTxnEngine->txnEngineTask);
    clOsalMutexDelete (clTxnServiceCfg->pTxnEngine->txnEngineMutex);
    clOsalCondDelete (clTxnServiceCfg->pTxnEngine->txnCondVar);

    clQueueDelete(&(clTxnServiceCfg->pTxnEngine->txnReqQueue));


    clTxnJobExecTemplateFinalilze();
    clTxnExecTemplateFinalize();
    clHeapFree (clTxnServiceCfg->pTxnEngine);

    CL_FUNC_EXIT();
    return (rc);
}

/** 
 * Receives new transaction for processing from user.
 * New transaction received will be in PRE-INIT state (as defined by the client).
 * Txn Engine creates instance of Active-Txn and initilizes protocol hndl for
 * all jobs.
 */
ClRcT clTxnEngineNewTxnReceive(
        CL_IN   ClTxnDefnT              *pNewTxn,
        CL_IN   ClIocPhysicalAddressT   clientAddr,
        CL_IN   ClUint32T               recoveryMode,
        CL_IN   ClBufferHandleT         outMsg, 
        CL_IN   ClUint8T                cli)
{
    ClRcT           rc  = CL_OK;
    ClTxnCntrlMsgT  *pNewCntrlMsg;

    CL_FUNC_ENTER();

    /* Create/store the new transaction in the queue and notify the task */
    rc = _clTxnTxnExecContextCreate(pNewTxn, clientAddr, outMsg, cli);
    if ( (CL_OK == rc) && (recoveryMode == CL_FALSE) )
    {
        /* Create txn recovery session in case of new txn */ 
        rc = clTxnRecoverySessionInit(pNewTxn->serverTxnId);
    }
    
    if (CL_OK == rc)
    {
        /* Post Cntrl-Msg to the txn-task for new transaction processing. */
        pNewCntrlMsg = AllocTxnCntrlMsg();
        if (pNewCntrlMsg != NULL)
        {
            pNewCntrlMsg->txnId = pNewTxn->serverTxnId;
            pNewCntrlMsg->cmd   = CL_TXN_ENGINE_TRANSACTION_START;
            if ( (rc = clOsalMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex)) == CL_OK)
            {
                clLogDebug("SER", "ENG", "Adding txn[0x%x:0x%x] to ReqQueue to start", 
                        pNewCntrlMsg->txnId.txnMgrNodeAddress,
                        pNewCntrlMsg->txnId.txnId);
                rc = clQueueNodeInsert(clTxnServiceCfg->pTxnEngine->txnReqQueue, 
                                  (ClQueueDataT) pNewCntrlMsg);
                /* Send notification */
                if (CL_OK == rc)
                {
                    clLogDebug("SER", "CTM",
                            "Queued transaction with txnId[0x%x:0x%x]. Signalling engine thread to do further processing!",
                            pNewCntrlMsg->txnId.txnMgrNodeAddress,
                            pNewCntrlMsg->txnId.txnId);
                    clOsalCondBroadcast(clTxnServiceCfg->pTxnEngine->txnCondVar);
                }
                clOsalMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
            }
        }
        else
        {
            rc = CL_ERR_NO_MEMORY;
        }
    }

    if (CL_OK != rc) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Failed to create execution-context for new transaction execution. rc:0x%x\n", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Receives response from various agents after processing txn-cmds
 */
ClRcT clTxnEngineAgentResponseReceive(
        CL_IN   ClTxnMessageHeaderT     msgHeader,
        CL_IN   ClBufferHandleT  msg)
{
    ClRcT                   rc = CL_OK;

    CL_FUNC_ENTER();

    clLogDebug("SER", "ATM",
            "Received [%d] messages from agent at [0x%x:0x%x]", 
            msgHeader.msgCount,
            msgHeader.srcAddr.nodeAddress,
            msgHeader.srcAddr.portId);

    while ( (CL_OK == rc) && (msgHeader.msgCount > 0) )
    {
        ClTxnCmdT           agentResp;
        ClTxnActiveTxnT     *pActiveTxn = NULL;
        ClTxnActiveJobT     *pActiveJob = NULL;
        ClTxnAppComponentT  *pAppComp = NULL;

        msgHeader.msgCount--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(msg, &agentResp);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Error in unmarshalling the msg, rc=[0x%x]", rc));
        }

        /*
           Extract agent response and update for each transaction
           and job-resonse.
           If job had received all responses, post cntrl-msg
        */

        /* Lock */
        if ( CL_OK == rc )
        {
             clTxnMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        }

        if (CL_OK == rc)
        {
            rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                                    (ClCntKeyHandleT) &(agentResp.txnId),
                                    (ClCntDataHandleT *) &pActiveTxn);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in getting active txn, mgrAdd:txnId: [0x%x:0x%x]",
                         agentResp.txnId.txnMgrNodeAddress, agentResp.txnId.txnId));
                goto label_error1;
            }
            pActiveJob = &(pActiveTxn->pActiveJobList[agentResp.jobId.jobId]);
            rc = clCntDataForKeyGet (pActiveJob->pAppJobDefn->compList, 
                                     (ClCntKeyHandleT) &(msgHeader.srcAddr), 
                                     (ClCntDataHandleT *) &pAppComp);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in getting compIndex for txn with txnId %d\n",
                         agentResp.txnId.txnId));
                goto label_error1;
            }
           clLogDebug("SER", NULL, 
               "Received cmd [%d] for transaction with txnId[0x%x:0x%x] and jobId[0x%x] from agent[0x%x:0x%x]", 
                agentResp.cmd, 
                agentResp.txnId.txnMgrNodeAddress, agentResp.txnId.txnId, 
                agentResp.jobId.jobId, msgHeader.srcAddr.nodeAddress, 
                msgHeader.srcAddr.portId);
        }

        if (CL_OK == rc)
        {
            //pActiveJob = &(pActiveTxn->pActiveJobList[agentResp.jobId.jobId]);
            if (pActiveJob->agentRespCount == 0x0U)
            {
                clLogInfo("SER", NULL,
                        "Timeout has already occurred for agent [0x%x:0x%x] processing job[0x%x]. Dropping packets", 
                        msgHeader.srcAddr.nodeAddress,
                        msgHeader.srcAddr.portId,
                        agentResp.jobId.jobId);
                rc = CL_ERR_TIMEOUT;
            }
        }

        if (CL_OK == rc)
        { 
            if ( (pActiveJob->pAgntRespList[pAppComp->compIndex].status == CL_TXN_ERR_WAITING_FOR_RESPONSE) &&
                 (pActiveJob->pAgntRespList[pAppComp->compIndex].cmd == agentResp.cmd) )
            {
                clLogDebug("SER", "ENG", 
                        "Received response for txn-job[0x%x:0x%x-0x%x] from agent [0x%x:0x%x]", 
                         pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                         pActiveTxn->pTxnDefn->serverTxnId.txnId, pActiveJob->pAppJobDefn->jobId.jobId, 
                         msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId);

                pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd = agentResp.resp;
                /* Building up the Job Info List */
                
                if ( (agentResp.resp == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE)) &&
                     (pActiveTxn->pTxnDefn->txnCfg & CL_TXN_CONFIG_IGNORE_NO_SERVICE_REGD_AGENT) )
                {
                    rc = clTxnJobInfoAdd(pActiveTxn,
                                    msg,
                                    agentResp.jobId, 
                                    agentResp.cmd, 
                                    pAppComp->appCompAddress,
                                    pActiveJob,
                                    agentResp.cmd,
                                    CL_TRUE); 
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Error in adding AJD into the Job Info List,"
                                 " rc = [0x%x]", rc));
                    }
#ifdef CL_TXN_DEBUG
                    else
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM:Job failed, no regd service, jobId"
                                ": %d, added into JIL\n",
                                agentResp.jobId.jobId));
                    }
#endif
                }
                /* Add the job definition recieved from the prepare-failed agent-comp
                   to the failed-agent list*/
                else if(((pActiveJob->pAgntRespList[pAppComp->compIndex].respRcvd) != CL_OK) &&
                                    ( agentResp.cmd == CL_TXN_CMD_PREPARE))
                {
                    clLogDebug("SER", NULL, 
                            "Prepare failed, adding to failed joblist. Response from OI at node[0x%x] portId[0x%x]", 
                            pAppComp->appCompAddress.nodeAddress,
                            pAppComp->appCompAddress.portId
                            );
                    rc = clTxnJobInfoAdd(pActiveTxn,
                                    msg,
                                    agentResp.jobId, 
                                    agentResp.cmd, 
                                    pAppComp->appCompAddress,
                                    pActiveJob,
                                    agentResp.cmd,
                                    CL_FALSE); 
                    if(CL_OK != rc)
                    {
                        clLogError("SER", NULL,
                                "Prepare failed. Error in adding AJD into the Job Info List, rc [0x%x]", rc);
                    }
#ifdef CL_TXN_DEBUG
                    else
                    {
                        clLogDebug("SER", NULL,
                                 "Job with jobId [0x%x] failed at prepare."
                                ", Successfully added into JIL", agentResp.jobId.jobId);
                    }
#endif
                }
                pActiveJob->agentRespCount--;
                pActiveJob->pAgntRespList[pAppComp->compIndex].status = CL_OK;
                pActiveJob->rcvdStatus |= agentResp.resp;   /* Consolidated response */

                rc = clTxnRecoveryReceivedResp(
                       pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                       pActiveJob->pAppJobDefn->jobId, 
                       pAppComp->appCompAddress, agentResp.cmd, 
                       agentResp.resp);
            }
            else
            {
                rc = CL_ERR_TIMEOUT;
            }
        }
        if ( (CL_OK == rc) && (pActiveJob->agentRespCount == 0) )
        {
            ClTxnCntrlMsgT  *pNewCntrlMsg;

            pNewCntrlMsg = AllocTxnCntrlMsg();
            /* Post cntrl-msg to engine */
            if (pNewCntrlMsg != NULL)
            {
                pNewCntrlMsg->cmd = CL_TXN_ENGINE_TXN_JOB_RESP_RCVD;
                pNewCntrlMsg->txnId = agentResp.txnId;
                pNewCntrlMsg->jobId = agentResp.jobId;
#ifdef CL_TXN_DEBUG
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: All responses received from agent"
                        " and added into JIL, adding the job into the Job queue\n"));
#endif
                clLogDebug("SER", "ENG", "Adding txn[0x%x:0x%x] to ReqQueue to process job", 
                        pNewCntrlMsg->txnId.txnMgrNodeAddress,
                        pNewCntrlMsg->txnId.txnId);
                rc = clQueueNodeInsert(clTxnServiceCfg->pTxnEngine->txnReqQueue, 
                                  (ClQueueDataT) pNewCntrlMsg);
                /* Send notification */
                if (CL_OK == rc)
                {
                    rc = clOsalCondBroadcast(clTxnServiceCfg->pTxnEngine->txnCondVar);
                    clLogDebug("SER", NULL, 
                            "Job[0x%x] processed for txn[0x%x:0x%x]. Signalling the engine thread with cmd[%d]", 
                            agentResp.jobId.jobId,
                            agentResp.txnId.txnMgrNodeAddress,
                            agentResp.txnId.txnId,
                            pNewCntrlMsg->cmd);
                }
            }
            else
            {
                rc = CL_ERR_NO_MEMORY;
            }
        }
label_error1:
        if (CL_OK != rc)
        {
            clLogError("SER", NULL, 
                    "Failed to process response from agent[0x%x:0x%x] for "\
                     "txn-job[0x%x:0x%x-0x%x] agent-resp-count:%d rc:0x%x", 
                     msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId, 
                     agentResp.txnId.txnMgrNodeAddress, agentResp.txnId.txnId, 
                     agentResp.jobId.jobId, 
                     (pActiveJob == NULL ? -1 : pActiveJob->agentRespCount), rc);
            rc = CL_GET_ERROR_CODE(rc);
        }

        /* Release lock */
        clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

        if (rc == CL_ERR_TIMEOUT)
        {
            rc = CL_OK; /* To continue processing of remaining msgs */
        }
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to process all of agent response. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: Create execution-context for transaction processing.
 */
static ClRcT 
_clTxnTxnExecContextCreate(
        CL_IN   ClTxnDefnT              *pTxnDefn,
        CL_IN   ClIocPhysicalAddressT   clientAddr,
        CL_IN   ClBufferHandleT         outMsg, 
        CL_IN   ClUint8T                cli)
{
    ClRcT               rc  = CL_OK;
    ClTxnActiveTxnT     *pNewTxnExecCntxt;
    ClCntNodeHandleT    jobDefnNode;

    CL_FUNC_ENTER();

    pNewTxnExecCntxt = (ClTxnActiveTxnT *) clHeapAllocate(sizeof(ClTxnActiveTxnT));
    if (pNewTxnExecCntxt == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for new txn exec-context\n"));
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }
    memset(pNewTxnExecCntxt, 0, sizeof(ClTxnActiveTxnT));

    pNewTxnExecCntxt->pTxnDefn = pTxnDefn;
    pNewTxnExecCntxt->pTxnDefn->currentState = CL_TXN_STATE_PRE_INIT;
    pNewTxnExecCntxt->clientAddr = clientAddr;
    pNewTxnExecCntxt->pActiveJobList = 
        (ClTxnActiveJobT *) clHeapAllocate(sizeof(ClTxnActiveJobT) * pTxnDefn->jobCount);
    if(!pNewTxnExecCntxt->pActiveJobList)
    {
        clLogError("SER", "CTM",
                "Error in allocating memory of size [%zd]", 
                sizeof(ClTxnActiveJobT) * pTxnDefn->jobCount);
        return CL_ERR_NO_MEMORY;
    }

    pNewTxnExecCntxt->activeJobListLen = pTxnDefn->jobCount;

    /* Initilize the count of failing agents and the list*/
    pNewTxnExecCntxt->failedAgentCount = 0U;
    rc = clTxnAgentListCreate(&(pNewTxnExecCntxt->failedAgentList));
    if (NULL == pNewTxnExecCntxt->pActiveJobList)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for job-exec-context for new txn\n"));
        clHeapFree(pNewTxnExecCntxt);
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }
    memset( pNewTxnExecCntxt->pActiveJobList, 0, 
            (sizeof(ClTxnActiveJobT) * pTxnDefn->jobCount));
    clLogTrace("SER", "ENG", "Initializing [%d] active-job exec-context", 
            pTxnDefn->jobCount);


    rc = clCntFirstNodeGet( pTxnDefn->jobList, &jobDefnNode);
    while (CL_OK == rc)
    {
        ClCntNodeHandleT    tNode;
        ClTxnAppJobDefnT    *pTxnJob;

        rc = clCntNodeUserDataGet(pTxnDefn->jobList, jobDefnNode, (ClCntDataHandleT *)&pTxnJob);

        if (CL_OK == rc)
        {
            ClTxnActiveJobT     *pTxnActiveJob;

/*            check this is required*/
            /*memcpy(&(pTxnJob->jobId.txnId), &(pTxnDefn->serverTxnId), 
                    sizeof(ClTxnTransactionIdT));*/
            pTxnActiveJob  = clTxnGetActiveJob(pNewTxnExecCntxt,pTxnJob);

            pTxnActiveJob->pAppJobDefn = pTxnJob;
            pTxnActiveJob->pAppJobDefn->currentState = CL_TXN_STATE_PRE_INIT;
            pTxnActiveJob->pParentTxn = pNewTxnExecCntxt;

            if (pTxnJob->compCount == 0x0)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Zorro component job. Invalid transaction. Aborting"));
                rc = CL_ERR_INVALID_PARAMETER;
                break;
            }

            pTxnActiveJob->pAgntRespList = 
                clHeapAllocate(sizeof(ClTxnAgentResponseT) * pTxnJob->compCount);
            if (pTxnActiveJob->pAgntRespList != NULL)
            {
                memset(pTxnActiveJob->pAgntRespList, 0, 
                       (sizeof(ClTxnAgentResponseT) * pTxnJob->compCount) );
                rc = clTxnJobNewExecContextCreate( &(pTxnActiveJob->jobExecContext) );
                clLogDebug("SER", NULL, 
                       "Result of creating jobexec context [%p], rc [0x%x] for jobId [0x%x]", 
                       (ClPtrT)pTxnActiveJob->jobExecContext, rc, pTxnJob->jobId.jobId);
            }
            else
            {
                rc = CL_ERR_NO_MEMORY;
            }

        }

        if (CL_OK == rc)
        {
            tNode = jobDefnNode;
            if (CL_OK != clCntNextNodeGet(pTxnDefn->jobList, tNode, &jobDefnNode))
                break;  /* End of list */
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create job-exec context. rc:0x%x\n", rc));
        }
    }

    if (CL_OK == rc)
    {
        /* Store this pNewTxnExecCntxt indexed by txn-id for further retrieval */ 
        rc = clTxnNewExecContextCreate(&(pNewTxnExecCntxt->txnExecContext));

        if (CL_OK == rc)
        {
            ClUint32T   count = 0;

            clTxnMutexLock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);

            rc = clCntSizeGet(clTxnServiceCfg->pTxnEngine->activeTxnMap, &count);
            if (CL_OK == rc)
            {

                clLogTrace("SER", "CTM", "Before scheduling new transaction[0x%x:0x%x], size of active map[%d]", 
                                pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                                pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnId, count);

                /* Dont defer reply if the invocation is cli based.
                 * */
                if(!cli) 
                {
                    /* Defer reply */
                    rc = clRmdResponseDefer(&pNewTxnExecCntxt->rmdDeferHdl);
                    if(CL_OK != rc)
                    {
                        clLogError("SER", NULL,
                                "Failed to defer rmd for transaction [0x%x:0x%x] with error [0x%x]",
                                pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                                pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnId, rc);
                        clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
                        return rc;
                    }
                    /* Defer outMsgHandler, the output needs to be written to this */
                    pNewTxnExecCntxt->deferOutMsg = outMsg;
                    clLogDebug("SER", NULL, 
                            "Deferred rmd for transaction [0x%x:0x%x]. Adding to activeTxnMap", 
                            pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                            pNewTxnExecCntxt->pTxnDefn->serverTxnId.txnId);
                }
                rc = clCntNodeAdd(clTxnServiceCfg->pTxnEngine->activeTxnMap, 
                              (ClCntKeyHandleT) &(pNewTxnExecCntxt->pTxnDefn->serverTxnId),
                              (ClCntDataHandleT) pNewTxnExecCntxt, NULL);
            }
            clTxnMutexUnlock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        }
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create exec-context for transaction[0x%x:0x%x]. rc:0x%x\n", 
                       pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId, rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Function: Entry-point of txn-engine task thread
 */
static void 
_clTxnEngineRun(void *t)
{
    ClRcT       rc      =   CL_OK;
    ClCharT     flag    =   0x1;

    ClTimerTimeOutT timeOut = {
        .tsSec      = 0x0, 
        .tsMilliSec = 0x0
    };

    CL_FUNC_ENTER();

    clLogDebug("SER", "ENG", "Starting transaction worker thread");

    if(clTxnServiceCfg->retVal)
    {
        rc = clTxnServiceCkptRecoveryLogRestore();
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to retrieve active txn of previous run from ckpt. rc:0x%x", rc));
        }
        else
        {
            rc = clTxnRecoveryInitiate();
        }
    }
    else
        clLogDebug("SER", NULL,
                "Checkpoint does not exist, no recovery action taken");

    while (flag)
    {
        ClUint32T           reqCount;
        ClTxnCntrlMsgT      *pTxnCntrlMsg = NULL;
        /* To abort the processing if junk comes in */
        ClBoolT             badTxn = CL_FALSE;

        /* Not strictly necessary due to GAS rc check below, but safe */
        reqCount = 0;
        
        clTxnMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

        rc = clQueueSizeGet(clTxnServiceCfg->pTxnEngine->txnReqQueue, &reqCount);
        while ( (rc == CL_OK) && (reqCount == 0) )
        {
            clLogDebug("SER", NULL,
                    "Sleeping on cond-var...");
            rc = clOsalCondWait(clTxnServiceCfg->pTxnEngine->txnCondVar, 
                                clTxnServiceCfg->pTxnEngine->txnEngineMutex, timeOut);
            if (CL_OK == rc)
            {
                rc = clQueueSizeGet(clTxnServiceCfg->pTxnEngine->txnReqQueue, &reqCount);
            }
        }

        if (rc)
        {            
        /* Case where rc != CL_OK is not handled  */
        clLogError("SER", "ENG", "clQueueSizeGet or clOsalCondWait returned an error rc [0x%x]", rc);
        clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        continue;
        }

        pTxnCntrlMsg = 0;

        /* If rc wasn't checked, and pTxnCntrlMsg wasn't zeroed, and reqCount wasn't
           zeroed, then we handle the same pTxnCntrlMsg twice.
           Of course its probably been freed, so we have heap corruption.
         */
        
        clLogDebug("SER", "ENG", "Pending request count is [0x%x] in the ReqQueue", reqCount);
        if (reqCount > 0)
        {

            rc = clQueueNodeDelete(clTxnServiceCfg->pTxnEngine->txnReqQueue, (ClQueueDataT *)&pTxnCntrlMsg);
            if(CL_OK != rc)
            {
                clLogError("SER", "ENG", 
                        "Failed to dequeue node, rc [0x%x]", rc);
                clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
                continue;
            }
            clLogDebug("SER", "ENG", "Extract txn[0x%x:0x%x] from ReqQueue to process", 
                    pTxnCntrlMsg->txnId.txnMgrNodeAddress,
                    pTxnCntrlMsg->txnId.txnId);
                
        }

        /* Code below requires pTxnCntrlMsg was set in the if block above */
        CL_ASSERT(reqCount > 0);        
        
        clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

        clLogDebug("SER", "ENG", 
                "Processing txn[0x%x;0x%x] cmd[0x%x]", 
                pTxnCntrlMsg->txnId.txnMgrNodeAddress, pTxnCntrlMsg->txnId.txnId, 
                pTxnCntrlMsg->cmd);

        switch (pTxnCntrlMsg->cmd)
        {
            case CL_TXN_ENGINE_TRANSACTION_START:
                /* Start the transaction */
                clLogDebug("SER", "ENG", "To start new transaction");
                rc = _clTxnEngineTxnStart(pTxnCntrlMsg);
                break;

            case CL_TXN_ENGINE_TRANSACTION_CANCEL:
                /* Cancel this transaction */
                clLogDebug("SER", "ENG", "To cancel a transaction");
                rc = _clTxnEngineTxnCancel(pTxnCntrlMsg);
                break;

            case CL_TXN_ENGINE_TXN_JOB_RESP_RCVD:
                /* Received all response for a given job. proceed further */
                clLogDebug("SER", "ENG", "Received response from agent");
                rc = _clTxnEngineTxnJobProcess(pTxnCntrlMsg);
                break;

            case CL_TXN_ENGINE_TERMINATE:
                /* Need to terminate this thread/task. */
                flag = 0x0;
                break;

            default:
                clDbgCodeError(rc,("Invalid transaction command [%d]", pTxnCntrlMsg->cmd));
                badTxn = CL_TRUE;                
                break;
        }
        
        /* This frees the bad transaction.  I should really put it on a "bad"
           queue for analysis by a person */
        if (badTxn)
            {
                ReleaseTxnCntrlMsg(pTxnCntrlMsg);
                continue;
            }
        
        if (flag) 
        {
            /* CloseSession only on preparing/committing/rolling back. Delete the activeTxn after closing.
               For reponse to the client, deferred response is sent in _clTxnFini */

            ClTxnActiveTxnT     *pActiveTxn = NULL;
            clLogDebug("SER", NULL,
                    "Closing all session. Sending processing cmd(s) to agent(s)");
            rc = clTxnCommIfcAllSessionClose();
            if(CL_OK != rc)
            {
                clLogError("SER", NULL,
                        "Error in closing all sessions, rc=[0x%x]", rc);
            }

            clTxnMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

            rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                                    (ClCntKeyHandleT) &(pTxnCntrlMsg->txnId),
                                    (ClCntDataHandleT *) &pActiveTxn);
            if (   (CL_OK == rc) &&
                (  (pActiveTxn->pTxnDefn->currentState == CL_TXN_STATE_COMMITTED) || 
                   (pActiveTxn->pTxnDefn->currentState == CL_TXN_STATE_ROLLED_BACK) ||
                   (pActiveTxn->pTxnDefn->currentState == CL_TXN_STATE_UNKNOWN)
                )
               )
            {
                clLogDebug("SER", NULL, 
                        "Deleting txndefn with txnid [0x%x:0x%x] from activeTxnMap", 
                        pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                        pActiveTxn->pTxnDefn->serverTxnId.txnId);
                /* Remove entry from the active Map */
                rc = clCntAllNodesForKeyDelete(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                                      (ClCntKeyHandleT) &(pActiveTxn->pTxnDefn->serverTxnId));
                if(CL_OK != rc)
                {
                    clLogError("SER", NULL,
                            "Deleting txn information from activeTxnMap failed with error [0x%x]", 
                            rc);
                }

            }

            clTxnMutexUnlock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        }
        /* Free the ReqQueue node */
        ReleaseTxnCntrlMsg(pTxnCntrlMsg);
    }
    
    /* Just inform the main thread that its all over */
    /* Dont think anybody is waiting for this signal */

    clTxnMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

    clOsalCondSignal(clTxnServiceCfg->pTxnEngine->txnCondVar);

    clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);


    CL_FUNC_EXIT();
}

static ClRcT _clTxnEngineTxnStart(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg)
{
    ClRcT   rc  =   CL_OK;
    ClTxnActiveTxnT     *pActiveTxn = NULL;
    ClSmEventT          txnSmEvent;

    CL_FUNC_ENTER();

    /* Lock */

    clTxnMutexLock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);

    rc = clCntDataForKeyGet (clTxnServiceCfg->pTxnEngine->activeTxnMap, 
                             (ClCntKeyHandleT) &(pTxnCtrlMsg->txnId), 
                             (ClCntDataHandleT *) &pActiveTxn);
    if(CL_OK != rc)
    {
        clLogError("SER", "ENG",
                "Data for key[0x%x:0x%x] failed with error [0x%x]", 
                pTxnCtrlMsg->txnId.txnMgrNodeAddress,
                pTxnCtrlMsg->txnId.txnId,
                rc);
    }
   clTxnMutexUnlock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);

    if ( (CL_OK != rc) || ( NULL == pActiveTxn) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction Id:0x%x", pTxnCtrlMsg->txnId.txnId));

        if ((rc == CL_OK)&&(pActiveTxn == NULL))
        {
            clDbgCodeError(rc,("Strange condition when retrieving txn info...needs analysis."));
        }        
        return (CL_RC(CL_CID_TXN,CL_ERR_INVALID_PARAMETER));
    }

    /* Post events (INIT and PREPARE) to transaction sm instance */
    txnSmEvent.eventId = CL_TXN_REQ_INIT;
    txnSmEvent.payload = (ClPtrT) pActiveTxn;

    rc = clTxnSmPostEvent( pActiveTxn->txnExecContext, &txnSmEvent);

    if (CL_OK == rc)
    {
        clLogDebug("SER", "ENG", "Completed transaction INIT, going for PREPARE");
        txnSmEvent.eventId = CL_TXN_REQ_PREPARE;
        /* TxnSmPostEvent should not have touched the txnSmEvent object */
        CL_ASSERT(txnSmEvent.payload == (ClPtrT) pActiveTxn);

        rc = clTxnSmPostEvent( pActiveTxn->txnExecContext, &txnSmEvent);
        if (rc != CL_OK)
        {
            clDbgCodeError(rc,("Possibly not-considered case where we have init event success, but prepare failure."));
        }        
    }

    if (CL_OK == rc)
    {
        rc = clTxnSmProcessEvents( pActiveTxn->txnExecContext );
    }

    if (CL_OK != rc)
    {
        clLogError("SER", NULL, "Failed to initiate transaction processing, rc [0x%x]"
                , rc);
        rc = CL_GET_ERROR_CODE(rc);
        /* Cleaning recovery data */
        clLogDebug("SER", NULL, 
                "Cleaning up txn data");
        if(CL_OK != (rc = clTxnRecoveryCleanup(pActiveTxn)))
        {
            clLog(CL_LOG_ERROR, "SER", NULL,
                    "Cleaning up txn data failed, rc [0x%x]", 
                    rc);
                    
        }

    }

    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT _clTxnEngineTxnCancel(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    /* FIXME: Post event (CANCEL_TXN) to transaction sm-instance */
    clDbgCodeError(rc,("Cancel transaction not implemented."));
    /* What is left behind when transactions are not cancelled and how can that affect other transactions?
       How soon are transaction IDs reused?
    */
    
    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT _clTxnEngineTxnJobProcess(
        CL_IN   ClTxnCntrlMsgT  *pTxnCtrlMsg)
{
    ClRcT               rc  =   CL_OK;
    ClTxnActiveTxnT     *pActiveTxn = NULL;
    ClTxnActiveJobT     *pActiveJob = NULL;
    ClSmEventT          jobSmEvent;

    CL_FUNC_ENTER();

    /* Lock */
    clTxnMutexLock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);

    rc = clCntDataForKeyGet (clTxnServiceCfg->pTxnEngine->activeTxnMap, 
                             (ClCntKeyHandleT) &(pTxnCtrlMsg->txnId), 
                             (ClCntDataHandleT *) &pActiveTxn);
    if(CL_OK != rc)
    {
        clLogError("SER", "ENG",
                "Data for key[0x%x:0x%x] failed with error [0x%x]", 
                pTxnCtrlMsg->txnId.txnMgrNodeAddress,
                pTxnCtrlMsg->txnId.txnId,
                rc);
    }


    if ( (CL_OK != rc) || ( NULL == pActiveTxn) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction Id 0x%x:0x%x or failed to retrieve txn-defn rc 0x%x", 
                        pTxnCtrlMsg->txnId.txnMgrNodeAddress, pTxnCtrlMsg->txnId.txnId, rc));
        CL_FUNC_EXIT();
        clTxnMutexUnlock (clTxnServiceCfg->pTxnEngine->txnEngineMutex);
        if ((rc == CL_OK)&&(pActiveTxn == NULL))
        {
            clDbgCodeError(rc,("Strange condition when retrieving txn info...needs analysis."));            
        }        
        return (CL_RC(CL_CID_TXN,CL_ERR_INVALID_PARAMETER));
    }

    CL_ASSERT(pTxnCtrlMsg->jobId.jobId < pActiveTxn->activeJobListLen);
    CL_ASSERT(pActiveTxn->activeJobListLen == pActiveTxn->pTxnDefn->jobCount); /* jobCount seems to be the limit the rest of code uses */

    pActiveJob = clTxnGetActiveJobById(pActiveTxn,pTxnCtrlMsg->jobId.jobId);
    CL_ASSERT(pActiveJob);    
    
    jobSmEvent.eventId = (pActiveJob->rcvdStatus == CL_OK) ? CL_TXN_JOB_RESP_SUCCESS : CL_TXN_JOB_RESP_FAILURE;
    jobSmEvent.payload = (ClPtrT) pActiveJob;
    pActiveJob->rcvdStatus = CL_OK; /* resetting. */

    clTxnMutexUnlock (clTxnServiceCfg->pTxnEngine->txnEngineMutex); /* Protecting rcvdStatus */

    clLogTrace("SER", "ENG", "Posting event[%s] to txn/job [0x%x:0x%x, 0x%x]", 
                   (jobSmEvent.eventId == CL_TXN_JOB_RESP_FAILURE)?"FAILURE":
                   ((jobSmEvent.eventId == CL_TXN_JOB_RESP_SUCCESS)?"SUCCESS":"UNKNOWN"),
                   pTxnCtrlMsg->txnId.txnMgrNodeAddress, 
                   pTxnCtrlMsg->txnId.txnId, pTxnCtrlMsg->jobId.jobId);

#ifdef CL_TXN_DEBUG
    if(jobSmEvent.eventId == CL_TXN_JOB_RESP_FAILURE)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Failure, posting event to the JobSM \n"));
    else
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Success, posting event to the JobSM \n"));
#endif

    rc = clTxnJobSmProcessEvent(pActiveJob->jobExecContext, &jobSmEvent);

    if (CL_OK == rc)
    {
        rc = clTxnSmProcessEvents(pActiveJob->pParentTxn->txnExecContext);
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Failed to process job-event. rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* This is used for calling within the async callback function when a timeout occurs */
static ClRcT _clTxnTimerExpire(void *data)
{
    ClRcT               rc  =   CL_OK;
    //ClTxnCntrlMsgT    *pNewCntrlMsg;
    ClTxnAppCookieT     cookie;
    ClTxnActiveTxnT     *pActiveTxn = NULL;

    CL_FUNC_ENTER();
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,(
            "Entering %s\n", __FUNCTION__));
#endif

    if(data)
    {
        cookie.tid = ((ClTxnAppCookieT *)data)->tid;
        cookie.addr = ((ClTxnAppCookieT *)data)->addr;
    }

    clTxnMutexLock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);
    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                            (ClCntKeyHandleT) &(cookie.tid),
                            (ClCntDataHandleT *) &pActiveTxn);
    if(CL_OK != rc)
    {
        clLogError("SER", NULL,
                "Error in getting active txn, mgrAdd:txnId: [0x%x:0x%x]",
                 cookie.tid.txnMgrNodeAddress, cookie.tid.txnId);
        goto unlock_mutex_expire;
    }
    rc = _clTxnWalkActiveMap(pActiveTxn, &cookie.addr); 
    if(CL_OK != rc)
    {
        goto unlock_mutex_expire;
    }

unlock_mutex_expire:
    clTxnMutexUnlock(clTxnServiceCfg->pTxnEngine->txnEngineMutex);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal API
 * This function updates the recovery-logs for specific components which failed
 * to reply within the stipulated time-interval
 */
static ClRcT 
_clTxnEngineRecoveryLogUpdate(
        CL_IN   ClTxnActiveJobT     *pActiveJob)
{
    ClRcT                   rc  = CL_OK;
    ClUint32T               index;

    CL_FUNC_ENTER();

    for (index = 0x0; index < pActiveJob->pAppJobDefn->compCount; index++)
    {
        if (pActiveJob->pAgntRespList[index].status == CL_TXN_ERR_WAITING_FOR_RESPONSE)
        {
            rc = clTxnRecoveryReceivedResp(pActiveJob->pParentTxn->pTxnDefn->serverTxnId, 
                                           pActiveJob->pAppJobDefn->jobId, 
                                           pActiveJob->pAgntRespList[index].agentAddr, 
                                           pActiveJob->pAgntRespList[index].cmd, 
                                           CL_ERR_TIMEOUT);
            pActiveJob->pAgntRespList[index].status = CL_OK;

            clLogTrace("SER", "ENG", 
                    "[%dth] component [0x%x:0x%x] failed to respond. TxnLog Update rc [0x%x]", 
                     index, pActiveJob->pAgntRespList[index].agentAddr.nodeAddress, 
                     pActiveJob->pAgntRespList[index].agentAddr.portId, rc);
        }
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update recovery log. rc:0x%x returning CL_OK", rc));
        rc = CL_OK;
    }

    CL_FUNC_EXIT();
    return (rc);
}

static void 
_clTxnEngineReqDelFn(
        CL_IN   ClQueueDataT    data)
{
    clHeapFree((ClPtrT)data);
}
static void 
_clTxnEngineDequeueCallBack(
        CL_IN ClQueueDataT userData)
{
    /* Noop */
}

/**
 * Internal function to delete/clean-up active-job definition/instance
 */
static void _clTxnActiveTxnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnActiveTxnT     *pActiveTxn;
    ClUint32T           index;

    CL_FUNC_ENTER();

    pActiveTxn = (ClTxnActiveTxnT *) userData;

    /* Delete all associated objects of this active-txn */
    for (index = 0; index < pActiveTxn->pTxnDefn->jobCount; index++)
    {
        ClTxnActiveJobT *pActiveJob = &(pActiveTxn->pActiveJobList[index]);
        
        pActiveJob->pParentTxn = NULL;
        clHeapFree(pActiveJob->pAgntRespList);
        clTxnJobExecContextDelete(pActiveJob->jobExecContext);
        //clTimerDelete (&(pActiveJob->txnJobTimerHandle) );
        /* clTxnAppJobDelete(pActiveJob->pAppJobDefn); */
    }

    clHeapFree(pActiveTxn->pActiveJobList);
    clTxnExecContextDelete(pActiveTxn->txnExecContext);
    pActiveTxn->pParentTxn = NULL;
    //clCntAllNodesDelete(pActiveTxn->failedAgentList); /* This is not required when deleting the entire list */
    clCntDelete(pActiveTxn->failedAgentList);

    /* Always delete the history db entry */
    clTxnDbTxnDefnRemove(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb,
            (pActiveTxn->pTxnDefn->serverTxnId));
    /* clTxnDefnDelete(pActiveTxn->pTxnDefn); */

    clHeapFree(pActiveTxn);

    CL_FUNC_EXIT();
}
static void _clTxnActiveTxnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnActiveTxnT     *pActiveTxn;
    ClUint32T           index;

    CL_FUNC_ENTER();

    pActiveTxn = (ClTxnActiveTxnT *) userData;

    /* Delete all associated objects of this active-txn */
    if(pActiveTxn)
    {
        for (index = 0; index < pActiveTxn->pTxnDefn->jobCount; index++)
        {
            ClTxnActiveJobT *pActiveJob = &(pActiveTxn->pActiveJobList[index]);
            
            pActiveJob->pParentTxn = NULL;
            clHeapFree(pActiveJob->pAgntRespList);
            clTxnJobExecContextDelete(pActiveJob->jobExecContext);
            //clTimerDelete (&(pActiveJob->txnJobTimerHandle) );
            /* clTxnAppJobDelete(pActiveJob->pAppJobDefn); */
        }

        clHeapFree(pActiveTxn->pActiveJobList);
        clTxnExecContextDelete(pActiveTxn->txnExecContext);
        pActiveTxn->pParentTxn = NULL;
        //clCntAllNodesDelete(pActiveTxn->failedAgentList); /* This is not required when deleting the entire list */
        clCntDelete(pActiveTxn->failedAgentList);

        /*Deleting node of history db. Should be deleting for ROLLBACK ? */ 
        if( pActiveTxn->pTxnDefn->currentState == CL_TXN_STATE_COMMITTED) 
            clTxnDbTxnDefnRemove(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb,
                    (pActiveTxn->pTxnDefn->serverTxnId));
        /* clTxnDefnDelete(pActiveTxn->pTxnDefn); */

        clHeapFree(pActiveTxn);
    }

    CL_FUNC_EXIT();
}

/**
 * Internal Function
 * Key compare function used while managing container of job-definitions.
 */
static ClInt32T _clTxnActiveTxnKeyCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2)
{
    return clTxnUtilTxnIdCompare( (*(ClTxnTransactionIdT *)key1), 
                                  (*(ClTxnTransactionIdT *)key2));
}

static ClUint32T  _clTxnActveTxnHashFn(
        ClCntKeyHandleT key)
{
    ClTxnTransactionIdT     *pTxnId;

    pTxnId = (ClTxnTransactionIdT *) key;

    return (pTxnId->txnId % CL_TXN_ENGINE_ACTIVE_TXN_BUCKET_SIZE);
}
/* Description: This function adds the AJD into the Job Info List. 
   For transaction that timed out or had no regd service, the AJD is taken
   from the TM's active map. Also on commit and rollback, the AJD is updated
   for this phase. This inturn implies that, eventually there will not be any AJDs in
   prepare phase that is passed to the client*/
static ClRcT 
clTxnJobInfoAdd(ClTxnActiveTxnT         *pActiveTxn,
                ClBufferHandleT  msg,
                ClTxnTransactionJobIdT  jobId,
                ClTxnMgmtCommandT       cmd,
                ClIocPhysicalAddressT   appCompAddress,
                ClTxnActiveJobT         *pActiveJob,
                ClUint8T                phase,
                ClUint8T                noServNTimeout) /* If set, it
                                                           indicates that the
                                                           job had no
                                                           service registered
                                                           or a timeout
                                                           occurred. The
                                                           action is same in
                                                           both cases */
{
    ClRcT           rc = CL_OK;
    ClTxnAgentT     *pFailedAgentInfo = NULL;
    ClUint8T        tempPhase = 0;

#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Command : %d\n", cmd));
#endif
    /* TODO: CLEANUP: This condition along with the code inside can be removed
     * */
    if((cmd == CL_TXN_CMD_ROLLBACK || cmd == CL_TXN_CMD_2PC_COMMIT) 
            && (CL_TRUE != noServNTimeout)) /* Not required for single
                                                  phase since it doesnt have a
                                                  prepare or rollback phase */
    {
        ClTxnAgentIdT       agentId;
         ClCntNodeHandleT   nodeHandle;
        
        agentId.agentAddress = appCompAddress; 
        agentId.jobId = jobId;
        
        rc = clCntDataForKeyGet(pActiveTxn->failedAgentList, 
                                (ClCntKeyHandleT) &agentId,
                                (ClCntDataHandleT *) &pFailedAgentInfo);
        if(rc == CL_OK)
        {
            tempPhase = pFailedAgentInfo->failedPhase;
            pFailedAgentInfo->agentJobDefnSize = 0U;
//            clHeapFree((void *)pFailedAgentInfo->agentJobDefn);

            /* Transient, Should update the AJD here */
            rc = clCntNodeFind(pActiveTxn->failedAgentList,
                                (ClCntKeyHandleT)&agentId,
                                &nodeHandle);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in searching the node with jobId %d\n",
                         agentId.jobId.jobId));
                CL_ASSERT(CL_OK == rc);
            }
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: After finding the node to delete :\n"));
#endif

#if 1 
            rc = clCntNodeDelete(pActiveTxn->failedAgentList,
                                nodeHandle);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in deleting the node with jobId %d\n",
                         agentId.jobId.jobId));
                CL_ASSERT(CL_OK == rc);
            }
#endif
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: After deleting the node : \n"));
#endif
#ifdef CL_TXN_DEBUG
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM: Successfully deleted the Job Info with id %d\n", 
                    agentId.jobId.jobId));
#endif
            pActiveTxn->failedAgentCount--;
        }

#ifdef CL_TXN_DEBUG
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TM:Something is wrong, the AJD should exist at this"
                    " point in prepare phase\n"));
            CL_ASSERT(CL_OK == rc);
        }
#endif
    }

    pFailedAgentInfo = (ClTxnAgentT *)clHeapAllocate(sizeof(ClTxnAgentT));
    if(NULL == pFailedAgentInfo)
    {
        return CL_TXN_RC(CL_ERR_NO_MEMORY);
    }
    memset(pFailedAgentInfo, 0, sizeof(ClTxnAgentT));
    pFailedAgentInfo->failedPhase |= (phase | tempPhase);
    pFailedAgentInfo->failedAgentId.agentAddress = appCompAddress; 
    pFailedAgentInfo->failedAgentId.jobId = jobId;
    if(CL_TRUE == noServNTimeout) 
    {
        pFailedAgentInfo->agentJobDefnSize = pActiveJob->pAppJobDefn->appJobDefnSize;
        pFailedAgentInfo->agentJobDefn = (ClUint8T *)clHeapAllocate( pFailedAgentInfo->agentJobDefnSize);
        if(pFailedAgentInfo->agentJobDefn)
            memcpy(pFailedAgentInfo->agentJobDefn, 
                   pActiveJob->pAppJobDefn->appJobDefn, 
                   pFailedAgentInfo->agentJobDefnSize); 
        else
            return CL_TXN_RC(CL_ERR_NO_MEMORY);
#ifdef CL_TXN_DEBUG
        {
            ClTxnAppJobT    *pAppJob;
            pAppJob = (ClTxnAppJobT *) pFailedAgentInfo->agentJobDefn ;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "After adding into JIL, Optype : %d, Index : %d, Value : %d\n",
                    pAppJob->opType, pAppJob->arrIndex, pAppJob->newValue));
        }
#endif
       if(cmd == CL_TXN_CMD_PREPARE || cmd == CL_TXN_CMD_1PC_COMMIT)
       {
            rc = clCntNodeAdd(pActiveTxn->failedAgentList, 
                          (ClCntKeyHandleT) &(pFailedAgentInfo->failedAgentId), 
                          (ClCntDataHandleT) pFailedAgentInfo, 0);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in adding to the Job Info List, rc=[0x%x]\n", rc));
                goto label_error1;
            
            }
            pActiveTxn->failedAgentCount++;
       }
    }
    else
    { 
        rc = clXdrUnmarshallClUint32T(msg,&(pFailedAgentInfo->agentJobDefnSize));
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Error in unmarshalling job defn size, rc=[0x%x]\n", rc));
            goto label_error1;
        }
        pFailedAgentInfo->agentJobDefn = 
            (ClUint8T *) clHeapAllocate(pFailedAgentInfo->agentJobDefnSize);
        if(NULL == (pFailedAgentInfo->agentJobDefn))
        {
            return CL_TXN_RC(CL_ERR_NO_MEMORY);
        }
        memset(pFailedAgentInfo->agentJobDefn, 0,
                pFailedAgentInfo->agentJobDefnSize);
        rc = clBufferNBytesRead(msg,(ClUint8T *) pFailedAgentInfo->agentJobDefn,
                &(pFailedAgentInfo->agentJobDefnSize));
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Error in reading buffer message, rc=[0x%x]\n", rc));
            goto label_error1;
        }
        rc = clCntNodeAdd(pActiveTxn->failedAgentList, 
                      (ClCntKeyHandleT) &(pFailedAgentInfo->failedAgentId), 
                      (ClCntDataHandleT) pFailedAgentInfo, 0);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Error in adding to the Job Info List, rc=[0x%x]\n", rc));
            goto label_error1;
        }
        pActiveTxn->failedAgentCount++;
    }
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( 
            "TM:Incrementing the agent count value %d, phase : %d\n", 
            pActiveTxn->failedAgentCount, pFailedAgentInfo->failedPhase));
#endif
label_error1:
    return rc;
}

/*j
static ClRcT 
_clTxnCompRemove(ClCntHandleT compList,
                 ClPtrT       pData,
                 ClIocPhysicalAddressT phyAddr)
{
    ClRcT       rc = CL_OK;
    ClUint32T   i = 0;
    ClTxnAppCookieT *pAppCookie = NULL;

    pAppCookie = (ClTxnAppCookieT *)pData;

    while(i < pAppCookie->count)
    {
        if(!memcmp(&(pAppCookie->pAgentList[i].agentAddress), &phyAddr, 
                    sizeof(ClIocPhysicalAddressT))) 
        {
            rc = clCntAllNodesForKeyDelete(compList, (ClCntKeyHandleT)&phyAddr);
            if(CL_OK != rc)
            {
                clLogError("SER", NULL,
                        "Failed to remove component[0x%x:0x%x] from job[0x%x]",
                        phyAddr.nodeAddress,
                        phyAddr.portId,
                        pAppCookie->pAgentList[i].jobId.jobId);
            }

        }
        i++;
    }

    return rc;
}
*/

static ClRcT    
clTxnCompListAdd(ClTxnActiveJobT       *pActiveJob,
                 ClIocPhysicalAddressT *pAddr)
{
    ClRcT       rc = CL_OK;
    ClUint32T   index;


    clLogTrace("SER", NULL,
            "Timeout occurred: Updating status for [%d] components", 
            pActiveJob->pAppJobDefn->compCount);
    /* Deleting from the active job if present */
    rc = clCntAllNodesForKeyDelete(pActiveJob->pAppJobDefn->compList, 
            (ClCntKeyHandleT)pAddr);
    if(CL_OK == rc)
    {
        clLogInfo("SER", NULL,
                "Removed agent [0x%x:0x%x] from job[0x%x] in transaction[0x%x:0x%x]",
                pAddr->nodeAddress, pAddr->portId,
                pActiveJob->pAppJobDefn->jobId.jobId,
                pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId);
        /* This makes the other packets to drop. 
         * Dont do the below if you still want to wait for the response from other 
         * agents of the job, just search for the specific comp and add into JIL
         */
        if (pActiveJob->rcvdStatus == CL_OK)
        {
            pActiveJob->rcvdStatus = CL_ERR_TIMEOUT;
        }
        pActiveJob->agentRespCount = 0x0; 
        for (index = 0x0; index < pActiveJob->pAppJobDefn->compCount; index++)
        {
            clLogTrace("SER", NULL,
                    "Status of component [%d]", 
                    pActiveJob->pAgntRespList[index].status);
            if (pActiveJob->pAgntRespList[index].status == CL_TXN_ERR_WAITING_FOR_RESPONSE)
            {
                clLogInfo("SER", NULL,
                        "Agent running at node [0x%x] with port [0x%x] did not respond within the specified time", 
                        pActiveJob->pAgntRespList[index].agentAddr.nodeAddress,
                        pActiveJob->pAgntRespList[index].agentAddr.portId);
                
                /* Define CL_TXN_WAIT_MODE if for an agent death in a job, responses from
                 * other agents needs to be processes 
                 */
#ifdef CL_TXN_WAIT_MODE 
                if(!memcmp(pActiveJob->pAgntRespList[index].agentAddr,
                            pAddr, sizeof(ClIocPhysicalAddressT)))
                {
#endif
                    rc = clTxnJobInfoAdd(
                            pActiveJob->pParentTxn,
                            0,
                            pActiveJob->pAppJobDefn->jobId, 
                            pActiveJob->pAgntRespList[index].cmd,
                            pActiveJob->pAgntRespList[index].agentAddr,
                            pActiveJob,
                            pActiveJob->pAgntRespList[index].cmd,
                            CL_TRUE); 
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Error in adding AJD into the Job Info List,"
                                 " rc = [0x%x]", rc));
                    }
#ifdef CL_TXN_WAIT_MODE 
                }
#endif
            }
        }
        /********************Till here **********************/
    }
    return rc;
}

static ClRcT _clTxnAgentRespRecv(
        ClBufferHandleT  inMsgHandle) 
{
    ClRcT                   rc  = CL_OK;
    ClVersionT              ver;
    ClTxnMessageHeaderT     msgHeader = {{0}};
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}};

    CL_FUNC_ENTER();

    rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(inMsgHandle, &msgHeadIDL);
    if(CL_OK != rc)
    {
        clLogError("AGT", NULL,
                "Error in unmarshalling msgHeadIDL, rc [0x%x]", rc);
        return rc;
    }

    _clTxnCopyIDLToMsgHead(&msgHeader, &msgHeadIDL);

    if(!msgHeader.msgCount)
    {
        clLogNotice("SER", "ATM",
                "Response received with invalid msgCount [%d], dropping response", 
                msgHeader.msgCount);
        /* Invalid response, dont process just drop them */
        return CL_OK;
    }

    memcpy(&ver, &(msgHeader.version), sizeof(ClVersionT));

    /* Do version management and reject un-supported version */
    clLogDebug("SER", "ATM",
            "My version information [%c:0x%x:0x%x], msgCount [%d]", 
            clTxnMgmtVersionDb.versionsSupported[0].releaseCode,
            clTxnMgmtVersionDb.versionsSupported[0].majorVersion,
            clTxnMgmtVersionDb.versionsSupported[0].minorVersion, msgHeader.msgCount);

    clTxnDumpMsgHeader(msgHeader);
    
    rc = clVersionVerify (&clTxnMgmtVersionDb, &(msgHeader.version));
    if (CL_OK != rc)
    {
        /* FIXME: Need to propogate this information to all pending transaction */
        clLogError("SER", NULL,
                "Response from agent [0x%x:0x%x] received. Version[%c:0x%x:0x%x] passed is not supported, rc [0x%x]. Use supported version[%c:0x%x:0x%x]", 
                msgHeader.srcAddr.nodeAddress,
                msgHeader.srcAddr.portId,
                ver.releaseCode,
                ver.majorVersion,
                ver.minorVersion,
                msgHeader.version.releaseCode,
                msgHeader.version.majorVersion,
                msgHeader.version.minorVersion,
                rc);

        return (CL_TXN_RC(rc));
    }

    /* Check/Validate message-type */
    switch (msgHeader.msgType)
    {
        case CL_TXN_MSG_AGNT_RESP:
            clLogDebug("SER", NULL,
                       "Received response from txn-agent at [0x%x:0x%x]", 
                       msgHeader.srcAddr.nodeAddress, 
                       msgHeader.srcAddr.portId);
            rc = clTxnEngineAgentResponseReceive(msgHeader, inMsgHandle);
            break;

        default:
            clLogError("SER", NULL,
                    "Invalid msg-type received [0x%x]", 
                    msgHeader.msgType);
            rc = CL_ERR_INVALID_PARAMETER;
            break;
    }

    if (CL_OK != rc)
    {
        clLogError("SER", NULL, 
                "Failed to process received message from agent, rc [0x%x]", rc);
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}
/* When retCode is itself not CL_OK, it means that there is no msg to process.
   This condition is very similar to timeout condition which existed earlier.
   This is also similar to making all the pending responses in that job invalid and 
   proceeding with the job
 */
void clTxnEngineAgentAsyncResponseRecv(
        CL_IN ClRcT retCode,
        CL_IN ClPtrT cookie,         
        CL_IN ClBufferHandleT inMsg,
        CL_IN ClBufferHandleT outMsg)

{
    ClRcT   rc = CL_OK;
    ClUint32T   length = 0;

    if(!clTxnServiceCfg)
    {
        clLogNotice("SER", "ATM",
                "txn engine finalized, clTxnServiceCfg is NULL");
        goto free_buff;
    }
    if(CL_OK == retCode)
    {
        /* Detect if the buffer handle is valid */
        rc = clBufferLengthGet(outMsg, &length);
        if(CL_OK != rc)
        {
            clLogError("SER", NULL,
                    "Error in getting the length of the buffer, rc=[0x%x]", 
                    rc);
            goto free_buff;
        }
        clLogTrace("SER", NULL,
                "Length of the buffer [%d]", length);
        if(!length)
        {
            clLogInfo("SER", NULL,
                     "Buffer length is 0, most likeliest possibility is that the response was already processed. This happens when the system is under stress and the agent returns multiple response in a single payload, txn[0x%x:0x%x], agent[0x%x:0x%x]", 
                     ((ClTxnAppCookieT *)cookie)->tid.txnMgrNodeAddress,
                     ((ClTxnAppCookieT *)cookie)->tid.txnId,
                     ((ClTxnAppCookieT *)cookie)->addr.nodeAddress,
                     ((ClTxnAppCookieT *)cookie)->addr.portId);
            goto free_buff;
        }
        /* Response sent by Agent. Process the request */
        rc = _clTxnAgentRespRecv(outMsg);
        if(CL_OK != rc)
        {
            clLogError("SER", "ATM",
                    "Failed to process agent response with error [0x%x]", rc);
            goto free_buff;

        }

    }
    /* A timeout occurred or the request was not successfully processed */
   
    else /* (CL_ERR_TIMEOUT == CL_GET_ERROR_CODE(retCode) && CL_OK != retCode) */
    {
        clLogInfo("SER", NULL,
                "One of the participating component of the transaction died, rc=[0x%x]", 
                retCode);
        rc = _clTxnTimerExpire(cookie);
        if(CL_OK != rc)
        {
            clLogError("SER", NULL,
                    "Error in handling component death with error [0x%x]", rc);
        }
        

    }
free_buff:
    clTxnCommCookieFree(cookie);    
    clBufferDelete(&outMsg);
    return;
}


static ClRcT 
_clTxnWalkActiveMap(CL_IN ClTxnActiveTxnT *pActiveTxn,
                    CL_IN ClIocPhysicalAddressT   *pAddr)

{
    ClRcT rc = CL_OK;
    ClUint32T   count = 0;
    ClCntNodeHandleT    jobDefnNode = NULL, nextNode = NULL;
    ClTxnCntrlMsgT      *pNewCntrlMsg = NULL;


    if(!pActiveTxn || !pAddr)
    {
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    rc = clCntFirstNodeGet(pActiveTxn->pTxnDefn->jobList, &jobDefnNode);
    while (CL_OK == rc)
    {
        ClTxnAppJobDefnT    *pTxnJob;

        rc = clCntNodeUserDataGet(pActiveTxn->pTxnDefn->jobList, 
                jobDefnNode, (ClCntDataHandleT *)&pTxnJob);
        if (CL_OK == rc)
        {
            ClTxnActiveJobT     *pActiveJob = NULL;

            pActiveJob = clTxnGetActiveJob(pActiveTxn, pTxnJob);

            if ( (CL_OK == rc) && (pActiveJob->agentRespCount > 0x0) )
            {
                clLogInfo("SER", NULL, 
                     "Received time-out error for txn[0x%x:0x%x-0x%x] during phase[%d]", 
                      pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                      pActiveJob->pParentTxn->pTxnDefn->serverTxnId.txnId, 
                      pActiveJob->pAppJobDefn->jobId.jobId, 
                      pActiveJob->pAppJobDefn->currentState);

                /* This transaction has failed waiting for response from an agent */
                
                /* Have to update the failedList here also */
                CL_DEBUG_PRINT(CL_DEBUG_INFO, 
                        ("Adding into the failed job list only when the job timesout at preparing state"));
                /* Fix for 6440.
                   When comparing against CL_TXN_SM_STATE_PREPARING, the condition never becomes true since this state is used for transaction state and not job state  */
                if(pActiveJob->pAppJobDefn->currentState == CL_TXN_STATE_PREPARING)
                {
                    clLogDebug("SER", NULL,
                            "Updating failed agent list");
                    rc = clTxnCompListAdd(pActiveJob, pAddr);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Error in adding complist into the agent response list, rc=[0x%x]\n", rc));
                        return rc;
                    }
                }

                /********************************************/


                /* Update the transaction-recovery logs */
                rc = _clTxnEngineRecoveryLogUpdate(pActiveJob);
                if(CL_OK != rc)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("Error in updating recovery log, rc=[0x%x]\n", rc));
                    return rc;
                }

                pNewCntrlMsg = AllocTxnCntrlMsg();
                if (pNewCntrlMsg != NULL)
                {
                    pNewCntrlMsg->txnId = pActiveJob->pParentTxn->pTxnDefn->serverTxnId;
                    pNewCntrlMsg->jobId = pActiveJob->pAppJobDefn->jobId;
                    pNewCntrlMsg->cmd   = CL_TXN_ENGINE_TXN_JOB_RESP_RCVD;

                    clLogDebug("SER", NULL,
                            "Agent timeout, updating req queue for further processing");
                    rc = clQueueNodeInsert(clTxnServiceCfg->pTxnEngine->txnReqQueue,
                                           (ClQueueDataT) pNewCntrlMsg);
                    
                    if(CL_OK != rc)
                    {
                        clLogError("SER", "ATM",
                                "Failed to add into the job queue with error [0x%x]", rc);
                        break;
                    }
                    count++;
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for txn cntrl-msg for time-out"));
                    rc = CL_ERR_NO_MEMORY;
                }
            }
            rc = clCntNextNodeGet(pActiveTxn->pTxnDefn->jobList, 
                                  jobDefnNode, 
                                  &nextNode);
            if(CL_OK != rc)
            {
                clLogDebug("SER", NULL, 
                        "Next node from the job list failed, rc =[0x%x]", rc);
            }
            jobDefnNode = nextNode;
        }
    }
    /* Send notification */
    clLogDebug("SER", "ATM",
            "Sending notification to txn engine to start processing [%d] jobs",
            count);
    clOsalCondBroadcast(clTxnServiceCfg->pTxnEngine->txnCondVar);
    return CL_OK;
}

static ClRcT 
clTxnRecoveryCleanup(ClTxnActiveTxnT *pActiveTxn)
{
    ClRcT   rc = CL_OK;
    ClTxnCommHandleT    commHandle;
    ClTxnCmdT   cmd = {{0}};

    if (!pActiveTxn || !pActiveTxn->pTxnDefn)
    {
        clLogError("SER", NULL,  
            "Null arguement in msg-payload. Cannot retrieve active-txn");
        return CL_ERR_NULL_POINTER;
    }

    cmd.txnId = pActiveTxn->pTxnDefn->clientTxnId;
    cmd.resp  = CL_TXN_STATE_UNKNOWN; 
    cmd.cmd   = CL_TXN_CMD_RESPONSE;

    rc = clTxnCommIfcNewSessionCreate(
            CL_TXN_MSG_MGR_RESP, 
            pActiveTxn->clientAddr, 
            CL_TXN_CLIENT_MGR_RESP_RECV, 
            NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
            CL_TXN_COMMON_ID, /* Value is 0x1 */ 
            &commHandle);

    clLogDebug("SER", NULL, 
            "Sending response to client [0x%x:0x%x]", 
            pActiveTxn->clientAddr.nodeAddress, 
            pActiveTxn->clientAddr.portId);

    /* Always COMMITTED or ROLLED_BACK at this stage */
    clLog(CL_LOG_INFO, "SER", NULL,
            "Transaction with txnId [0x%x:0x%x] is at UNKNOWN state", 
            pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
            pActiveTxn->pTxnDefn->serverTxnId.txnId);
    pActiveTxn->pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;

    rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &cmd);
    if (CL_OK == rc)
    {
        clLogTrace("SER", "ENG", 
                "Checkpointing state of txn [0x%x:0x%x]", 
                pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                pActiveTxn->pTxnDefn->serverTxnId.txnId);
    }
    /* The recovery db alongwith the checkpoint is removed at all cases now */
    clLogDebug("SER", "ENG", 
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

    clLogDebug("SER", NULL,
            "Sending response of transaction[0x%x:0x%x] to client", 
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
    return rc;
}



ClRcT VerifyTxnCntrlMsg(ClTxnCntrlMsgT* t)
{
    ClRcT rc = CL_RC(CL_CID_TXN,CL_ERR_INVALID_PARAMETER);
    
    if ((t->cmd<CL_TXN_ENGINE_TRANSACTION_START) || (t->cmd>CL_TXN_ENGINE_TERMINATE))
    {
        clDbgCodeError(rc, ("Transaction command is out of range"));
        return rc;        
    }

    /* Should t->txnId and t->jobId.txnId be the same? */
    
    
    return CL_OK;    
}
