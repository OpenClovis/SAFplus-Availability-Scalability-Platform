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
 * File        : clTxnJobProcess.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains all the necessary processing done for a job
 * at the agent end. The agent receives job-definition and receives control
 * messages from transaction-manager. Agent correlates current-state of job
 * and takes required step.
 *
 *
 *****************************************************************************/

#include <clCommon.h>
#include <clDebugApi.h>
#include <clCommonErrors.h>
#include <clXdrApi.h>
#include <string.h>
#include <clTxnLog.h>
#include <xdrClTxnCmdT.h>

#include <clTxnStreamIpi.h>

#include "clTxnAgentIpi.h"

extern ClTxnAgentCfgT          *clTxnAgntCfg; 

/* Forward Declarations */
/*
static ClRcT 
_clTxnAgentCmpServiceWalkFn(
        CL_IN   ClCntKeyHandleT key, 
        CL_IN   ClCntDataHandleT compInst, 
        CL_IN   ClCntArgHandleT userArg, 
        CL_IN   ClUint32T dataLen);
*/

/*FIX FOR BUG : 4183 */
/* Basic functions for Prepare/Commit/Rollback for given
   Service registered
 */
static  ClRcT _clTxnAgentPrepare(
    CL_IN   ClTxnOperationT             *pTxnOp,
    CL_IN   ClTxnAgentCompServiceInfoT  *pCompService);

static  ClRcT _clTxnAgentCommit(
    CL_IN   ClTxnOperationT *pTxnOp,
    CL_IN   ClTxnAgentCompServiceInfoT *pCompService);

static  ClRcT _clTxnAgentRollback(
    CL_IN    ClTxnOperationT *pTxnOp,
    CL_IN    ClTxnAgentCompServiceInfoT *pCompService);

/*FIX FOR BUG : 4183 */
/*
   Basic functions to process each phase for all the services 
   registered for any component    
*/
static  ClRcT _clTxnAgentPrepareFor2PCC(
    CL_IN   ClTxnOperationT *pTxnOp);

static  ClRcT _clTxnAgentCommitFor2PCC(
    CL_IN   ClTxnOperationT *pTxnOp);

static  ClRcT _clTxnAgentRollbackFor2PCC(
    CL_IN   ClTxnOperationT *pTxnOp);

static  ClRcT _clTxnAgentCommitFor1PCC(
    CL_IN   ClTxnOperationT *pTxnOp);

/**
 * This function receives job-description from the remote transaction-client
 * requested by the application. 
 * This funciton is available as RMD
 */
ClRcT clTxnAgentTxnDefnReceive(
        CL_IN   ClTxnCmdT               cmd, 
        CL_IN   ClBufferHandleT  jobDefnMsg)
{
    ClRcT               rc              = CL_OK;
    ClTxnDefnT          *pTxnDefn;
    ClTxnAppJobDefnT    *pTxnAppJob;

    CL_FUNC_ENTER();
    /*
       Transaction-client sends txn-job containing job-description and necessary
       identification txn and job (known to txn-manager).
       Initialization of transaction-job at agent-side has following steps
       - Init transaction-context and extract job-description.
       - (TODO)Start timer required for fault-detection (txn-mgr failure).
    */

    rc = clTxnStreamTxnDataUnpack(jobDefnMsg, &pTxnDefn, &pTxnAppJob);

    if (CL_OK == rc)
    {
        /* Insert/Add this transaction to active-txn-list */
        rc = clTxnDbNewTxnDefnAdd(clTxnAgntCfg->activeTxnMap, pTxnDefn, 
                                  &(pTxnDefn->serverTxnId) );

        if ( CL_ERR_DUPLICATE == rc)
        {
            rc = CL_OK; /* Forcefully make it correct */
            clTxnDefnDelete(pTxnDefn);
            pTxnDefn = NULL;
            /* Retrieve the correct txn-defn matching txn-id */
            rc = clTxnDbTxnDefnGet(clTxnAgntCfg->activeTxnMap, 
                                   cmd.txnId, &pTxnDefn);
        }
        if (CL_OK == rc)
        {
            rc = clTxnNewAppJobAdd(pTxnDefn, pTxnAppJob);

            if (CL_OK != rc)
            {
                clLogMultiline(CL_LOG_ERROR, "AGT", NULL,  
                    "Error in adding JOB job[0x%x] to txn[0x%x:0x%x], rc [0x%x]\n"
                    "This can be because of rmd retries", 
                     pTxnAppJob->jobId.jobId, pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                     pTxnDefn->serverTxnId.txnId,  rc);
                rc = CL_OK;
            }
            else
            {
                pTxnAppJob->currentState = CL_TXN_STATE_PRE_INIT;
            }
        }
    }

    if (CL_OK != rc)
    {
        clLogError("AGT", "MTC", 
                "Failed to extract job-definitions from msg, rc[0x%x]", rc);
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to delete transaction-job definition as command issued by txn-server
 */
ClRcT clTxnAgentTxnDefnRemove(
        CL_IN   ClTxnCmdT           txnCmd)
{
    ClRcT               rc  =   CL_OK;
    ClTxnDefnT          *pTxnDefn;

    CL_FUNC_ENTER();

    /* Retrieve the txn-defn matching txn-id */
    rc = clTxnDbTxnDefnGet(clTxnAgntCfg->activeTxnMap, 
                           txnCmd.txnId, &pTxnDefn);

    if (CL_OK == rc)
    {
        rc = clTxnAppJobRemove(pTxnDefn, txnCmd.jobId);
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete txn-job. rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}


/**
 * Method doing transaction-management by coordinating control messages from 
 * transaction-manager with various comp-servces.
 */
ClRcT clTxnAgentProcessJob(
        CL_IN   ClTxnMessageHeaderT     *pMsgHdr,
        CL_IN   ClTxnCmdT               txnCmd,
        CL_IN   ClBufferHandleT         outMsg,
        CL_OUT  ClTxnCommHandleT        *pCommHandle)
{
    /* This function is called when a control message arrives for an 
       active txn from mgr.  */
    ClRcT               rc = CL_OK;
    ClTxnOperationT     txnOp;
    ClTxnCommHandleT    commHandle;
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "%s: Cmd received : %d\n", __FUNCTION__, txnCmd.cmd));
#endif

    CL_FUNC_ENTER();
    /* Procedure:
       1. Check and retrieve job-defn from the active-txn-map
       2. Check for services involved and retrieve initialize iterator
       3. Use switch-case to call respective callback function into 
          comp-service implementation
    */

    if(!pCommHandle)
        return CL_TXN_RC(CL_ERR_NULL_POINTER);

    /* Check if there are any services registered with transaction-agent */
    clTxnMutexLock(clTxnAgntCfg->actMtx);
    if (clTxnAgntCfg->agentCapability == CL_TXN_AGENT_NO_SERVICE_REGD)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No services are registed at this txn-agent. Aborting"));
        txnCmd.resp = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);

        rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                          pMsgHdr->srcAddr, 
                                          CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                          NULL, CL_TXN_RMD_DFLT_TIMEOUT,  
                                          CL_TXN_COMMON_ID, /* Value is 0x1 */
                                          &commHandle);
        if (CL_OK == rc)
        {
            rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);

            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to append cmd in the response with error [0x%x]",
                        rc); 
                clTxnMutexUnlock(clTxnAgntCfg->actMtx);
                return rc;
            }
            rc = clTxnCommIfcSessionRelease(commHandle);
            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to release session with error[0x%x]",
                        rc); 
                clTxnMutexUnlock(clTxnAgntCfg->actMtx);
                return rc;
            }
            *pCommHandle = commHandle;
        }
        CL_FUNC_EXIT();
        clTxnMutexUnlock(clTxnAgntCfg->actMtx);
        return (txnCmd.resp);
    }
    clTxnMutexUnlock(clTxnAgntCfg->actMtx);

    /*    txnOp.command = txnCmd.cmd; */
    /* No need to lock this, it is thread safe container */
    rc = clCntDataForKeyGet(clTxnAgntCfg->activeTxnMap, 
                            (ClCntKeyHandleT )&(txnCmd.txnId), 
                            (ClCntDataHandleT *) &(txnOp.pTxnDefn));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Invalid transaction-id or unknown txn txnId:0x%x rc:0x%x\n", 
                 txnCmd.txnId.txnId, rc));
        /* Write in output buffer for this error and continue */
        txnCmd.resp = CL_ERR_INVALID_HANDLE;

        rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                          pMsgHdr->srcAddr, 
                                          CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                          NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
                                          CL_TXN_COMMON_ID, /* Value is 0x1 */
                                          &commHandle);
        if (CL_OK == rc)
        {
            rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);

            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to append cmd in the response with error [0x%x]",
                        rc); 
                return rc;
            }
            rc = clTxnCommIfcSessionRelease(commHandle);
            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to release session with error[0x%x]",
                        rc); 
                return rc;
            }
            *pCommHandle = commHandle;
        }
        CL_FUNC_EXIT();
        return (txnCmd.resp);
    }

    /* Retrieve the specified job now... */
    rc = clCntDataForKeyGet(txnOp.pTxnDefn->jobList, (ClCntKeyHandleT ) &txnCmd.jobId, 
                            (ClCntDataHandleT *) &txnOp.pTxnJobDefn);

    if (CL_OK != rc)
    {
        /* Write response in output buffer and continue */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid job-id from server. jobId:0x%x rc:0x%x\n", 
                       txnCmd.jobId.jobId, rc));
        txnCmd.resp = CL_ERR_INVALID_HANDLE;

        rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                          pMsgHdr->srcAddr, 
                                          CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                          NULL,CL_TXN_RMD_DFLT_TIMEOUT, 
                                          CL_TXN_COMMON_ID, /* Value is 0x1 */
                                          &commHandle);
        if (CL_OK == rc)
        {
            rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);

            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to append cmd in the response with error [0x%x]",
                        rc); 
                return rc;
            }
            rc = clTxnCommIfcSessionRelease(commHandle);
            if (CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Failed to release session with error[0x%x]",
                        rc); 
                return rc;
            }
            *pCommHandle = commHandle;
        }
        CL_FUNC_EXIT();
        return (txnCmd.resp);
    }

    txnOp.retValue = CL_OK;
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "%s: Cmd received : %d\n", __FUNCTION__, txnCmd.cmd));
#endif

    /* Check if this agent is capable of processing 2PC transaction */
    if ( (txnCmd.cmd == CL_TXN_CMD_PREPARE) ||
         (txnCmd.cmd == CL_TXN_CMD_2PC_COMMIT) ||
         (txnCmd.cmd == CL_TXN_CMD_ROLLBACK) )
    {
        /* Txn-Cmd corresponds to a Two-Phase Commit Transaction */
        clTxnMutexLock(clTxnAgntCfg->actMtx);
        if ( (clTxnAgntCfg->agentCapability & CL_TXN_AGENT_SERVICE_2PC) != CL_TXN_AGENT_SERVICE_2PC)
        {
            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
            clLogError("AGT", NULL,
                    "No service registered for Two-Phase commit txn. Aborting...");
            txnCmd.resp = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);

            rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                              pMsgHdr->srcAddr, 
                                              CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                              NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
                                              CL_TXN_COMMON_ID, /* Value is 0x1 */
                                              &commHandle);
            if (CL_OK == rc)
            {
                rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);

                if (CL_OK != rc)
                {
                    clLogError("AGT", NULL,
                            "Failed to append cmd in the response with error [0x%x]",
                            rc); 
                    return rc;
                }
                rc = clTxnCommIfcSessionRelease(commHandle);
                if (CL_OK != rc)
                {
                    clLogError("AGT", NULL,
                            "Failed to release session with error[0x%x]",
                            rc); 
                    return rc;
                }
                *pCommHandle = commHandle;
            }
            CL_FUNC_EXIT();
            return (txnCmd.resp);
        }
        else
            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
    }
    else if (txnCmd.cmd == CL_TXN_CMD_1PC_COMMIT)
    {
        /* Txn-Cmd corresponds to a Single-Phase Commit Transaction */
        clTxnMutexLock(clTxnAgntCfg->actMtx);
        if ( (clTxnAgntCfg->agentCapability & CL_TXN_AGENT_SERVICE_1PC) != CL_TXN_AGENT_SERVICE_1PC)
        {
            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No service registered for Single-Phase commit txn. aborting"));
            clLogError("AGT", NULL,
                    "No service registered for read operation. Aborting...");
            txnCmd.resp = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);

            rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                              pMsgHdr->srcAddr, 
                                              CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                              NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
                                              CL_TXN_COMMON_ID, /* Value is 0x1 */
                                              &commHandle);
            if (CL_OK == rc)
            {
                rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);

                if (CL_OK != rc)
                {
                    clLogError("AGT", NULL,
                            "Failed to append cmd in the response with error [0x%x]",
                            rc); 
                    return rc;
                }
                rc = clTxnCommIfcSessionRelease(commHandle);
                if (CL_OK != rc)
                {
                    clLogError("AGT", NULL,
                            "Failed to release session with error[0x%x]",
                            rc); 
                    return rc;
                }
                *pCommHandle = commHandle;
            }
            CL_FUNC_EXIT();
            return (txnCmd.resp);
        }
        else
            clTxnMutexUnlock(clTxnAgntCfg->actMtx);
    }

    
/*FIX FOR BUG : 4183 */
/* According to the command of Job Definition recieved,process the each 
   phase for all the services
*/
    switch(txnCmd.cmd)
    {
        case CL_TXN_CMD_PREPARE:
            {
                rc = _clTxnAgentPrepareFor2PCC(&txnOp);
                if (rc == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
                {
                    /* This is a success case from txn-agent */
                    clLogError("AGT", NULL,
                            "No registered service for prepare cmd received");
                    txnOp.pTxnJobDefn->currentState = CL_TXN_STATE_PREPARED;
                    txnOp.retValue = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
                    rc = CL_OK;
                }
                else if (rc == CL_TXN_RC(CL_TXN_ERR_INVALID_CMP_CAPABILITY))
                {
                    /* Make it a failure */
                    txnOp.pTxnJobDefn->currentState = CL_TXN_STATE_MARKED_ROLLBACK;
                    txnOp.retValue = CL_TXN_RC(CL_TXN_ERR_INVALID_CMP_CAPABILITY);
                    rc = CL_OK;
                }
                else if(CL_OK == rc)
                {
                    clLogDebug("AGT", "ATM",
                            "PREPARE cmd successfull for job[0x%x] in txn[0x%x:0x%x], retCode from agent=[0x%x]", 
                            txnCmd.jobId.jobId,
                            txnCmd.txnId.txnMgrNodeAddress,
                            txnCmd.txnId.txnId, txnOp.retValue);
                }
            }
            break;

        case CL_TXN_CMD_1PC_COMMIT:
            {
#ifdef CL_TXN_DEBUG
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA: Calling the 1p commit callback\n"));
#endif
                rc = _clTxnAgentCommitFor1PCC(&txnOp);
#ifdef CL_TXN_DEBUG
                if(CL_OK == rc)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA: Successfully completed the single phase transaction"));
#endif
                if (rc == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
                {
                    txnOp.pTxnJobDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
                    txnOp.retValue = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                            ("No service registered for single phase, rc=[0x%x]", rc));
                    rc = CL_OK;
                }
            }
            break;         

        case CL_TXN_CMD_2PC_COMMIT:
            {
                rc = _clTxnAgentCommitFor2PCC(&txnOp);
#ifdef CL_TXN_DEBUG
                if(CL_OK == rc)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA: Successfully completed the 2 phase transaction"));
#endif
                if (rc == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
                {
                    txnOp.retValue = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                            ("No service registered for 2 phase, rc=[0x%x]", rc));
                    rc = CL_OK;
                    /* Txn-state is not modified here */
                }
                else if(CL_OK == rc)
                    clLogDebug("AGT", "ATM",
                            "COMMIT cmd successfull for job[0x%x] in txn[0x%x:0x%x]",
                            txnCmd.jobId.jobId,
                            txnCmd.txnId.txnMgrNodeAddress,
                            txnCmd.txnId.txnId);
            }
            break;

        case CL_TXN_CMD_ROLLBACK:
            {
                rc = _clTxnAgentRollbackFor2PCC(&txnOp);
#ifdef CL_TXN_DEBUG
                if(CL_OK == rc)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA: Successfully completed rollback for 2 phase transaction"));
#endif
                if (rc == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
                {
                    txnOp.retValue = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                            ("No service registered on rollback, rc=[0x%x]", rc));
                    rc = CL_OK;
                }
                else if(CL_OK == rc)
                    clLogDebug("AGT", "ATM",
                            "ROLLBACK cmd successfull for job[0x%x] in txn[0x%x:0x%x]",
                            txnCmd.jobId.jobId,
                            txnCmd.txnId.txnMgrNodeAddress,
                            txnCmd.txnId.txnId);
            }
            break;

        case CL_TXN_CMD_INVALID:
        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Invalid txn command rececived from txn-mgr:0x%x", txnCmd.cmd));
            rc = CL_ERR_INVALID_PARAMETER;
            break;
    } 

    if (CL_OK == rc)
    {
        txnCmd.resp = txnOp.retValue;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Fatal error while executing app call-back. rc:0x%x\n", rc));
        txnCmd.resp = CL_ERR_UNSPECIFIED;
    }
    
    rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_AGNT_RESP, 
                                      pMsgHdr->srcAddr, 
                                      CL_TXN_SERVICE_AGENT_RESP_RECV, 
                                      NULL, CL_TXN_RMD_DFLT_TIMEOUT, 
                                      CL_TXN_COMMON_ID, /* Value is 0x1 */
                                      &commHandle);
        
    if (CL_OK == rc)
    {
        /* Append payload only when prepare fails. */
    //    clLogDebug("AGT", "ATM",
     //           "Session creation successfull");
        
        if( (txnCmd.cmd == CL_TXN_CMD_PREPARE)  && (txnCmd.resp != CL_OK) ) 
        {
            /* Send the job-definition to the server as it got updated by the agent application*/ 
            ClBufferHandleT  msgHandle;
            ClUint8T                *pAppJob;
            ClUint32T               msgLen;
                
            rc = clBufferCreate(&msgHandle);
            rc = clXdrMarshallClUint32T(&(txnOp.pTxnJobDefn->appJobDefnSize),msgHandle,0);
            rc = clBufferNBytesWrite(msgHandle,
                                       (ClUint8T *)txnOp.pTxnJobDefn->appJobDefn, 
                                        txnOp.pTxnJobDefn->appJobDefnSize);
            if (CL_OK == rc)
                rc = clBufferLengthGet(msgHandle, &msgLen);
            
            pAppJob = (ClUint8T *) clHeapAllocate(msgLen);
            if (CL_OK == rc)
            {
                ClUint32T   tLen = msgLen;
                rc = clBufferNBytesRead(msgHandle, pAppJob, &tLen);
            }
 
            clLogDebug("AGT", NULL,
                    "Prepare failed for agent[0x%x:0x%x] with error[0x%x]. Appending payload to send to TM. TXNID [0x%x:0x%x]\n", 
                    pMsgHdr->srcAddr.nodeAddress,
                    pMsgHdr->srcAddr.portId,
                    txnCmd.resp,
                    txnCmd.txnId.txnMgrNodeAddress,
                    txnCmd.txnId.txnId);
            rc = clTxnCommIfcSessionAppendPayload(commHandle, &txnCmd, pAppJob,msgLen);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in appending payload while returning back from"
                         " TA to TM, rc=[0x%x]", rc));
            }
        
            clHeapFree(pAppJob);
            clBufferDelete(&msgHandle);
        }
        else /* Append cmd if Commit/Rollback passed/failed or Prepare passed */
        {
            txnCmd.resp = CL_OK; /* Rollback and commit is always successful */
            rc = clTxnCommIfcSessionAppendTxnCmd(commHandle, &txnCmd);
            if(CL_OK != rc)
            {
                clLogError("AGT", NULL,
                        "Error in appending payload while returning back from"
                         " Agent to Mgr, rc=[0x%x]", 
                         rc);
            }
            clLogInfo("AGT", NULL,
                    "Response for cmd[%d] successfully appended", txnCmd.cmd);
        }

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to write response to txn-mgr")); 
        }
        rc = clTxnCommIfcSessionRelease(commHandle);
        if (CL_OK != rc)
        {
            clLogError("AGT", NULL,
                    "Failed to release session with error[0x%x]",
                    rc); 
            return rc;
        }
        *pCommHandle = commHandle;
    } 
    else
    {
        clLogError("SER", NULL,
                "Error in creating session, rc=[0x%x]", rc);
    }


    CL_FUNC_EXIT();
    return (txnCmd.resp);
}

/*FIX FOR BUG : 4183 */
/**
 * Internal Function:
 * IPI to prepare all services registered with this transaction agent 
 * for given transaction/job.
 */
ClRcT _clTxnAgentPrepareFor2PCC(
        CL_IN   ClTxnOperationT     *pTxnOp)
{
    ClCntNodeHandleT            currNode;
    ClRcT                       retCode = CL_OK;
    ClTxnAgentCompServiceInfoT  *pCompService;
    ClUint8T                    matchingServiceFlag;
    ClUint8T                    invalidFlag;

    CL_FUNC_ENTER();
    
    if (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_PRE_INIT)
    {
        matchingServiceFlag = CL_FALSE;
        invalidFlag = CL_FALSE;
        retCode = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);
        while ( retCode == CL_OK )
        {
            ClCntNodeHandleT        node;
            ClRcT                   rc;

            /* Retrieve data-node for node */
            retCode = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                           (ClCntDataHandleT *)&pCompService);
            if (retCode == CL_OK)
            {
                if ( IS_MATCHING_SERVICE(pCompService, pTxnOp->pTxnJobDefn->serviceType) && 
                     IS_SERVICE_2PC_CAPABLE(pCompService) )
                {
                    matchingServiceFlag = CL_TRUE;
                    rc = _clTxnAgentPrepare(pTxnOp, pCompService);
                }
                else if (IS_MATCHING_SERVICE(pCompService, pTxnOp->pTxnJobDefn->serviceType) &&
                         IS_SERVICE_1PC_CAPABLE(pCompService) )
                {
                    invalidFlag = CL_TRUE;
                }
                node = currNode;

                retCode = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
            }
        }

        if (CL_ERR_NOT_EXIST == (CL_GET_ERROR_CODE(retCode)) )
        {
            retCode = CL_OK;
            if ( (CL_TRUE == matchingServiceFlag) && 
                 (CL_FALSE == invalidFlag) && (CL_OK == pTxnOp->retValue) )
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_PREPARED;
            }
            else if ( (CL_TRUE == matchingServiceFlag) && 
                      (CL_FALSE == invalidFlag) && (CL_OK != pTxnOp->retValue) )
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_MARKED_ROLLBACK;
            }
            else if ( (CL_FALSE == matchingServiceFlag) &&
                      (CL_FALSE == invalidFlag) )
            {
                retCode = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
            }
            else if (CL_TRUE == invalidFlag)
            {
                retCode = CL_TXN_RC(CL_TXN_ERR_INVALID_CMP_CAPABILITY);
            }
        }
    }
    else
    {
        switch (pTxnOp->pTxnJobDefn->currentState)
        {
            case CL_TXN_STATE_COMMITTED:
            case CL_TXN_STATE_PREPARED:
                pTxnOp->retValue = CL_OK;   /* Already prepared */
                break;

            case CL_TXN_STATE_MARKED_ROLLBACK:
            case CL_TXN_STATE_ROLLED_BACK:
                pTxnOp->retValue = CL_TXN_RC(CL_TXN_ERR_VALIDATE_FAILED); 
                break;

            default:
                pTxnOp->retValue = CL_TXN_RC(CL_TXN_ERR_TRANSACTION_FAILURE);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Invalid current state [0x%x] of txn-job[0x%x:0x%x, 0x%x] to prepare", 
                        pTxnOp->pTxnJobDefn->currentState,
                        pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                        pTxnOp->pTxnDefn->serverTxnId.txnId,
                        pTxnOp->pTxnJobDefn->jobId.jobId));
                    clDbgPause(); /* GAS temporary */
                break;
        }
    }
    
    CL_FUNC_EXIT();
    return (retCode);
}

/* FIX FOR BUG : 4183 */
/**
 * Internal Function:
 * IPI to call COMMIT function of all services registered with this
 * transaction-agent for given transaction-job
 */
ClRcT _clTxnAgentCommitFor2PCC(
        CL_IN   ClTxnOperationT     *pTxnOp)
{
    ClCntNodeHandleT            currNode;
    ClRcT                       retCode = CL_OK;
    ClTxnAgentCompServiceInfoT  *pCompService;
    ClUint8T                    matchingServiceFlag;
    
    CL_FUNC_ENTER();
    if (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_PREPARED)
    {
        matchingServiceFlag = CL_FALSE;
        retCode = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);
        while (retCode == CL_OK)
        {
            ClCntNodeHandleT        node;
            ClRcT                   rc = CL_OK;
            /* Retrieve data-node for node */
            retCode = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode, 
                                           (ClCntDataHandleT *)&pCompService);

            if (retCode == CL_OK)
            {
                if ( IS_MATCHING_SERVICE(pCompService, pTxnOp->pTxnJobDefn->serviceType) && 
                     IS_SERVICE_2PC_CAPABLE(pCompService) )
                {
                    matchingServiceFlag = CL_TRUE;
                    rc = _clTxnAgentCommit(pTxnOp, pCompService);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Error while committing, rc=[0x%x]", rc));
                    }
                }
                node = currNode;
                retCode = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in getting the user data wrt the component, rc=[0x%x]", rc));
            }
        }
    
        if (CL_ERR_NOT_EXIST == (CL_GET_ERROR_CODE(retCode)) )
        {
            retCode = CL_OK; /* All transactions are successful */
            if ( (matchingServiceFlag == CL_TRUE) && (CL_OK == pTxnOp->retValue) )
            {
                clLogInfo("AGT", NULL, "Transaction[0x%x:0x%x] is in state COMMITTED",
                        pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                        pTxnOp->pTxnDefn->serverTxnId.txnId);
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_COMMITTED;
            }
            else if ( (matchingServiceFlag == CL_TRUE) && (CL_OK != pTxnOp->retValue) )
            {
                clLogInfo("AGT", NULL, "Transaction[0x%x:0x%x] is in state UNKNOWN",
                        pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                        pTxnOp->pTxnDefn->serverTxnId.txnId);
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_UNKNOWN;
            }
            else
            {
                retCode = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
            }
        }
    }
    else
    {
        switch (pTxnOp->pTxnJobDefn->currentState)
        {
            case CL_TXN_STATE_COMMITTED:
                pTxnOp->retValue = CL_OK;
                break;

            default:
                pTxnOp->retValue = CL_TXN_RC(CL_TXN_ERR_TRANSACTION_FAILURE);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("Invalid current state [0x%x] of txn-job[0x%x:0x%x, 0x%x] to commit", 
                        pTxnOp->pTxnJobDefn->currentState,
                        pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                        pTxnOp->pTxnDefn->serverTxnId.txnId,
                        pTxnOp->pTxnJobDefn->jobId.jobId));
                break;
        }
    }
                    
    CL_FUNC_EXIT();
    return (retCode);    
}

/*FIX FOR BUG : 4183 */
/**
 * Internal Function:
 * IPI to rollback all services registered with this transaction-agent
 * for given transaction-job
 */
ClRcT _clTxnAgentRollbackFor2PCC(
        CL_IN   ClTxnOperationT     *pTxnOp)
{
    ClCntNodeHandleT            currNode;
    ClRcT                       retCode = CL_OK;
    ClTxnAgentCompServiceInfoT  *pCompService;
    ClUint8T                    matchingServiceFlag;
    
    CL_FUNC_ENTER();
    if ( (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_MARKED_ROLLBACK) ||
         (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_PREPARED) )
    {
        matchingServiceFlag = CL_FALSE;
        retCode = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);
        while (retCode == CL_OK)
        {
            ClCntNodeHandleT        node;
            ClRcT                   rc;

            /* Retrieve data-node for node */
            retCode = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                           (ClCntDataHandleT *)&pCompService);

            if (retCode == CL_OK)
            {
                if ( IS_MATCHING_SERVICE(pCompService, pTxnOp->pTxnJobDefn->serviceType) && 
                     IS_SERVICE_2PC_CAPABLE(pCompService) )
                {
                    matchingServiceFlag = CL_TRUE;
                    rc = _clTxnAgentRollback(pTxnOp, pCompService);
                }
                node = currNode;
                retCode = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
            }
        }

        if (CL_ERR_NOT_EXIST == (CL_GET_ERROR_CODE(retCode)) )
        {
            retCode = CL_OK;
            if ( (matchingServiceFlag == CL_TRUE) && (CL_OK == pTxnOp->retValue) )
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
            }
            else if ( (matchingServiceFlag == CL_TRUE) && (CL_OK != pTxnOp->retValue) )
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_UNKNOWN;
            }
            else
            {
                retCode = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
            }
        }
    }
    else
    {
        switch (pTxnOp->pTxnJobDefn->currentState)
        {
            case CL_TXN_STATE_ROLLED_BACK:
                pTxnOp->retValue = CL_OK;
                break;

            default:
                pTxnOp->retValue = CL_TXN_RC(CL_TXN_ERR_TRANSACTION_FAILURE);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Invalid current state [0x%x] of txn-job[0x%x:0x%x, 0x%x] to rollback", 
                    pTxnOp->pTxnJobDefn->currentState,
                    pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                    pTxnOp->pTxnDefn->serverTxnId.txnId,
                    pTxnOp->pTxnJobDefn->jobId.jobId));
                break;
        }
    }

    CL_FUNC_EXIT();
    return (retCode);    
}


/*FIX FOR BUG : 4183 */
ClRcT _clTxnAgentCommitFor1PCC(
        CL_IN   ClTxnOperationT     *pTxnOp)
{
    ClCntNodeHandleT                currNode;
    ClRcT                           retCode = CL_OK;
    ClTxnAgentCompServiceInfoT      *pCompService;
    ClUint8T                        matchingServiceFlag;

    CL_FUNC_ENTER();
    /*
       call _clTxnAgentPrepareFor2PCC to prepare all services capable
       of 2PC
            If successful, loop through the services to commit the 1PC capable service

                If successful, call _clTxnAgentCommitFor2PCC to commit all services
                capable of 2PC

                else call _clTxnAgentRollbackFor2PCC to rollback

            else call _clTxnAgentRollbackFor2PCC to rollback
    */
    
    pTxnOp->retValue = CL_OK;
    retCode = _clTxnAgentPrepareFor2PCC(pTxnOp);

    if (retCode == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
    {
        pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
        pTxnOp->retValue = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
        CL_FUNC_EXIT();
        return (CL_OK);
    }
    else if ( (retCode == CL_TXN_RC(CL_TXN_ERR_INVALID_CMP_CAPABILITY)) ||
              (retCode == CL_OK) )
    {
        /* If there is any 1-PC service matching service, retCode should have been 
           CL_TXN_ERR_INVALID_CMP_CAPABILITY. If retCode == CL_OK, then it means there
           is no 1-PC service matching service-id
        */
        if (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_PRE_INIT)
        {
            if (pTxnOp->retValue == CL_OK)
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_PREPARED;
            }
            else
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_MARKED_ROLLBACK;
            }
        }
    }
    else /* (retCode != CL_OK) */
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to prepare all services - internal error:0x%x", retCode));
        CL_FUNC_EXIT();
        return (retCode);
    }
    
    if (CL_TXN_STATE_PREPARED == pTxnOp->pTxnJobDefn->currentState)
    {
        /* Commit Phase for 1P Commit Capable service */
        pTxnOp->retValue = CL_OK;
        matchingServiceFlag = CL_FALSE;
        retCode = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);
        while ( retCode == CL_OK )
        {
            ClCntNodeHandleT        node;
            ClRcT                   rc;

            /* Retrieve data-node for node */
            retCode = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                           (ClCntDataHandleT *) &pCompService);
            if (retCode == CL_OK)
            {
                if ( IS_MATCHING_SERVICE(pCompService, pTxnOp->pTxnJobDefn->serviceType) && 
                     IS_SERVICE_1PC_CAPABLE(pCompService) )
                {
                    matchingServiceFlag = CL_TRUE;
                    rc = _clTxnAgentCommit(pTxnOp, pCompService);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Error in committing single phase"
                                 " transaction, rc=[0x%x]", rc));
                    }
#ifdef CL_TXN_DEBUG
                    else
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA:Successfully committed single phase transaction"));
#endif
                    break;
                }
                node = currNode;
                retCode = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
            }
        }

        if ( (CL_OK != retCode) &&
             (CL_ERR_NOT_EXIST != (CL_GET_ERROR_CODE(retCode)) ) )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("Failed to commit 1PC Service internal error:0x%x", retCode));
            pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_MARKED_ROLLBACK;
            retCode = CL_OK;
        }
        else
        {
            if ( (matchingServiceFlag == CL_TRUE) && (pTxnOp->retValue != CL_OK) )
            {
                pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_MARKED_ROLLBACK;
            }
        }
    }

    if ( (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_PREPARED) && 
         (CL_OK == pTxnOp->retValue) )
    {
        retCode = _clTxnAgentCommitFor2PCC(pTxnOp);

        if (retCode == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Committing 2P after single, no service registered for 2"
                     "phase transaction, rc=[0x%x]", retCode));
            retCode = CL_OK;
            pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_COMMITTED;
        }
    }

    if (pTxnOp->pTxnJobDefn->currentState == CL_TXN_STATE_MARKED_ROLLBACK)
    {
        ClRcT   tRc = pTxnOp->retValue;
        pTxnOp->retValue = CL_OK;
        retCode = _clTxnAgentRollbackFor2PCC(pTxnOp);
        if (retCode == CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE))
        {
            retCode = CL_OK;
            pTxnOp->pTxnJobDefn->currentState = CL_TXN_STATE_ROLLED_BACK;
        }
        pTxnOp->retValue = tRc;
    }

    CL_FUNC_EXIT();
    return (retCode);    
}



    
/*FIX FOR BUG : 4183 */
ClRcT _clTxnAgentPrepare(
        CL_IN   ClTxnOperationT *pTxnOp,
        CL_IN   ClTxnAgentCompServiceInfoT *pCompService)
{
    ClRcT   rc  =   CL_OK;

    CL_FUNC_ENTER();
    rc = pCompService->pCompCallbacks->fpTxnAgentJobPrepare(
        (ClTxnTransactionHandleT) (pTxnOp->pTxnDefn->serverTxnId.txnId),
        (ClTxnJobDefnHandleT) pTxnOp->pTxnJobDefn->appJobDefn,
        pTxnOp->pTxnJobDefn->appJobDefnSize,
        (ClTxnAgentCookieT *) &(pTxnOp->pTxnJobDefn->cookie) );

    if (CL_OK != rc)
    {
        if (pTxnOp->retValue == CL_OK)
            pTxnOp->retValue = rc;

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Failure to prepare/validate transaction[0x%x:0x%x] job[0x%x], rc:0x%x",
                pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                pTxnOp->pTxnDefn->serverTxnId.txnId,
                pTxnOp->pTxnJobDefn->jobId.jobId, rc)); 
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_TXN_AGENT_LIB, CL_TXN_LOG_0_JOB_VALIDATE_FAILED);
    }
  
    CL_FUNC_EXIT();
    return (rc);
}

        
/*FIX FOR BUG : 4183 */
ClRcT _clTxnAgentCommit(
        CL_IN   ClTxnOperationT             *pTxnOp,
        CL_IN   ClTxnAgentCompServiceInfoT  *pCompService)
{
    ClRcT   rc  =   CL_OK;
    CL_FUNC_ENTER();

    rc = pCompService->pCompCallbacks->fpTxnAgentJobCommit(
            (ClTxnTransactionHandleT) (pTxnOp->pTxnDefn->serverTxnId.txnId),
            (ClTxnJobDefnHandleT) pTxnOp->pTxnJobDefn->appJobDefn,
            pTxnOp->pTxnJobDefn->appJobDefnSize,
            (ClTxnAgentCookieT *) &(pTxnOp->pTxnJobDefn->cookie) );

    if (CL_OK != rc)
    {
        if (pTxnOp->retValue == CL_OK)
            pTxnOp->retValue = rc;

        clLogError("AGT", NULL,
                "Failure to commit transaction[0x%x:0x%x] having job [0x%x]\n", 
                 pTxnOp->pTxnDefn->serverTxnId.txnMgrNodeAddress,
                 pTxnOp->pTxnDefn->serverTxnId.txnId, 
                 pTxnOp->pTxnJobDefn->jobId.jobId);
    }
#if CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TA: Successfully committed the transaction\n"));
#endif

    CL_FUNC_EXIT();
    return (rc);
}


/*FIX FOR BUG : 4183 */
ClRcT _clTxnAgentRollback(
        CL_IN   ClTxnOperationT *pTxnOp,
        CL_IN   ClTxnAgentCompServiceInfoT *pCompService)
{
    ClRcT   rc  =   CL_OK;
    CL_FUNC_ENTER();
    
    rc = pCompService->pCompCallbacks->fpTxnAgentJobRollback(
        (ClTxnTransactionHandleT) (pTxnOp->pTxnDefn->serverTxnId.txnId), 
        (ClTxnJobDefnHandleT) pTxnOp->pTxnJobDefn->appJobDefn, 
        pTxnOp->pTxnJobDefn->appJobDefnSize,
        (ClTxnAgentCookieT *) &(pTxnOp->pTxnJobDefn->cookie) );

    if (CL_OK != rc)
    {
        if (pTxnOp->retValue == CL_OK) 
            pTxnOp->retValue = rc;
        
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failure to abort transaction(0x%x) job(0x%x)\n", 
                 pTxnOp->pTxnDefn->serverTxnId.txnId, 
                 pTxnOp->pTxnJobDefn->jobId.jobId));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_TXN_AGENT_LIB, CL_TXN_LOG_0_JOB_ROLLBACK_FAILED);
    }
    
    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnAgentReadJob(
        CL_IN       ClTxnCmdT           tCmd, 
        CL_IN       ClBufferHandleT     inMsgHandle,
        CL_INOUT    ClTxnCommHandleT    commHandle, 
        CL_IN       ClTxnStartStopT     startstop)
{
    ClRcT   rc  =   CL_OK;
    ClTxnDefnT          *pTxnDefn;
    ClTxnAppJobDefnT    *pTxnAppJob;
    ClCntNodeHandleT            currNode;
    ClRcT                       retCode = CL_OK;
    ClTxnAgentCompServiceInfoT  *pCompService;
    ClUint8T                    matchingServiceFlag;

    CL_FUNC_ENTER();

    rc = clTxnStreamTxnDataUnpack(inMsgHandle, &pTxnDefn, &pTxnAppJob);

    if(rc == CL_OK)
    {
        matchingServiceFlag = CL_FALSE;

        retCode = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);

        while (retCode == CL_OK)
        {
            ClCntNodeHandleT        node;
            /* Retrieve data-node for node */
            retCode = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode, 
                    (ClCntDataHandleT *)&pCompService);

            if (retCode == CL_OK)
            {
                if ( IS_MATCHING_SERVICE(pCompService, pTxnAppJob->serviceType) && 
                        IS_SERVICE_1PC_CAPABLE(pCompService) )
                {
                    matchingServiceFlag = CL_TRUE;
                    if(startstop & CL_TXN_START)
                    {
                        if(pCompService->pCompCallbacks->fpTxnAgentStart)
                        {
                            clLogDebug("AGT", "RDT", 
                                    "Calling start callback for transaction[0x%x:0x%x]",
                                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                    pTxnDefn->clientTxnId.txnId);
                            tCmd.resp = pCompService->pCompCallbacks->fpTxnAgentStart(
                                    (ClTxnTransactionHandleT) (pTxnDefn->clientTxnId.txnId),
                                    (ClTxnAgentCookieT *) &(pTxnAppJob->cookie) );
                            if(CL_OK != tCmd.resp)
                            {
                                clLogError("AGT", "RDT",
                                        "Error in start callback, tCmdRsp=[0x%x]", tCmd.resp);
                            }
                        }
                    }

                    clLogTrace("AGT", "RDT", "Calling callback for [%p]", (ClPtrT)pTxnDefn);
                    tCmd.resp = pCompService->pCompCallbacks->fpTxnAgentJobCommit(
                            (ClTxnTransactionHandleT) (pTxnDefn->clientTxnId.txnId),
                            (ClTxnJobDefnHandleT) pTxnAppJob->appJobDefn,
                            pTxnAppJob->appJobDefnSize,
                            (ClTxnAgentCookieT *) &(pTxnAppJob->cookie) );

                    if(tCmd.resp != CL_OK )
                    {
                        clLogError("AGT", NULL,
                                "Error while reading, tCmdRsp=[0x%x]", tCmd.resp);
                    }
                    if(startstop & CL_TXN_STOP)
                    {
                        if(pCompService->pCompCallbacks->fpTxnAgentStop)
                        {
                            clLogDebug("AGT", "RDT", 
                                    "Calling stop callback for transaction[0x%x:0x%x]",
                                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                    pTxnDefn->clientTxnId.txnId);
                            tCmd.resp = pCompService->pCompCallbacks->fpTxnAgentStop(
                                    (ClTxnTransactionHandleT) (pTxnDefn->clientTxnId.txnId),
                                    (ClTxnAgentCookieT *) &(pTxnAppJob->cookie) );
                            if(CL_OK != tCmd.resp)
                            {
                                clLogError("AGT", "RDT",
                                        "Error in stop callback, tCmdRsp=[0x%x]", tCmd.resp);
                            }
                        }
                    }
                }
                node = currNode;
                retCode = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in getting the user data wrt the component, retCode=[0x%x]", retCode));
            }
        }

        if (CL_ERR_NOT_EXIST == (CL_GET_ERROR_CODE(retCode)) )
        {
            retCode = CL_OK;
            if (matchingServiceFlag != CL_TRUE)
            {
                tCmd.resp = CL_TXN_RC(CL_TXN_ERR_NO_REGD_SERVICE);
            }
        }

        if(rc == CL_OK)
        {
            ClBufferHandleT         jobDefn;
            ClUint8T                *pJobDefnStr = NULL;
            ClUint32T               jobDefnLen;

            rc = clBufferCreate (&jobDefn);
            if (CL_OK == rc)
            {
                rc = clTxnStreamTxnDataPack(pTxnDefn, pTxnAppJob, jobDefn);
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
            rc = clTxnCommIfcSessionAppendPayload(commHandle, &tCmd, pJobDefnStr, jobDefnLen);
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Error in appending payload while returning back from"
                         " TA to TM, rc=[0x%x]", rc));
            }
            clHeapFree(pJobDefnStr);
            rc = clBufferDelete(&jobDefn);
        }
        clTxnDefnDelete(pTxnDefn);
        clTxnAppJobDelete(pTxnAppJob);
    }

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to write response to txn-mgr")); 
    }
    CL_FUNC_EXIT();
    return (CL_OK);    
}
ClRcT _clTxnAgentTxnStart(ClTxnCmdT tCmd)
{
    ClRcT           rc = CL_OK;
    ClUint8T        match = CL_FALSE;
    ClTxnOperationT txnOp = {0};
    ClCntNodeHandleT            currNode;
    ClTxnAgentCompServiceInfoT  *pCompService = NULL;

    
    rc = clCntDataForKeyGet(clTxnAgntCfg->activeTxnMap, 
                            (ClCntKeyHandleT )&(tCmd.txnId), 
                            (ClCntDataHandleT *) &(txnOp.pTxnDefn));
    if (CL_OK != rc)
    {
       clLogError("AGT", "MTA", 
                "Invalid transaction [0x%x:0x%x], rc[0x%x]", 
                 tCmd.txnId.txnMgrNodeAddress, 
                 tCmd.txnId.txnId, rc);
        /* Write in output buffer for this error and continue */
        tCmd.resp = CL_ERR_INVALID_HANDLE;
        return (rc);
    }

    /* Retrieve the specified job now... */
    rc = clCntDataForKeyGet(txnOp.pTxnDefn->jobList, (ClCntKeyHandleT ) &tCmd.jobId, 
                            (ClCntDataHandleT *) &txnOp.pTxnJobDefn);
    if(CL_OK != rc)
    {
        clLogError("AGT", "MTA",
                "Failed to get the job defn, rc=[0x%x]", rc);
        return rc;
    }
    
    CL_ASSERT(clTxnAgntCfg->compServiceMap);
    rc = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);

    while ( CL_OK == rc )
    {
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        rc = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                       (ClCntDataHandleT *)&pCompService);
        if (rc == CL_OK)
        {
            if ( IS_MATCHING_SERVICE(pCompService, txnOp.pTxnJobDefn->serviceType) && 
                 IS_SERVICE_2PC_CAPABLE(pCompService) )
            {
                match = CL_TRUE;

                CL_ASSERT(pCompService->pCompCallbacks);
                if(pCompService->pCompCallbacks->fpTxnAgentStart)
                {
                    rc = pCompService->pCompCallbacks->fpTxnAgentStart(
                        (ClTxnTransactionHandleT) (txnOp.pTxnDefn->serverTxnId.txnId),
                        (ClTxnAgentCookieT *) &(txnOp.pTxnJobDefn->cookie) );
                }
                else
                {
                    clLogDebug("AGT", "MTA",
                            "No transaction start callback provided. Proceeding without error");
                }

            }
            node = currNode;
            rc = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
        }
    }

    if(!match)
    {
        clLogError("AGT", "MTA",
                "No matching service found for service [%d]. Transaction[0x%x:0x%x] not started",
                txnOp.pTxnJobDefn->serviceType,
                tCmd.txnId.txnMgrNodeAddress,
                tCmd.txnId.txnId);
        rc = CL_ERR_NOT_EXIST;
    }
    else
        rc = CL_OK;

    return rc;
}
ClRcT _clTxnAgentTxnStop(ClTxnCmdT tCmd)
{
    ClRcT           rc = CL_OK;
    ClUint8T        match = CL_FALSE;
    ClTxnOperationT txnOp = {0};
    ClCntNodeHandleT            currNode;
    ClTxnAgentCompServiceInfoT  *pCompService = NULL;

    
    rc = clCntDataForKeyGet(clTxnAgntCfg->activeTxnMap, 
                            (ClCntKeyHandleT )&(tCmd.txnId), 
                            (ClCntDataHandleT *) &(txnOp.pTxnDefn));
    if (CL_OK != rc)
    {
       clLogError("AGT", "MTA", 
                "Invalid transaction [0x%x:0x%x], rc[0x%x]", 
                 tCmd.txnId.txnMgrNodeAddress, 
                 tCmd.txnId.txnId, rc);
        /* Write in output buffer for this error and continue */
        tCmd.resp = CL_ERR_INVALID_HANDLE;
        return (rc);
    }

    /* Retrieve the specified job now... */
    rc = clCntDataForKeyGet(txnOp.pTxnDefn->jobList, (ClCntKeyHandleT ) &tCmd.jobId, 
                            (ClCntDataHandleT *) &txnOp.pTxnJobDefn);
    if(CL_OK != rc)
    {
        clLogError("AGT", "MTA",
                "Failed to get the job defn, rc=[0x%x]", rc);
        return rc;
    }
    
    CL_ASSERT(clTxnAgntCfg->compServiceMap);
    rc = clCntFirstNodeGet(clTxnAgntCfg->compServiceMap, &currNode);

    while ( CL_OK == rc )
    {
        ClCntNodeHandleT        node;

        /* Retrieve data-node for node */
        rc = clCntNodeUserDataGet(clTxnAgntCfg->compServiceMap, currNode,
                                       (ClCntDataHandleT *)&pCompService);
        if (rc == CL_OK)
        {
            if ( IS_MATCHING_SERVICE(pCompService, txnOp.pTxnJobDefn->serviceType) && 
                 IS_SERVICE_2PC_CAPABLE(pCompService) )
            {
                match = CL_TRUE;

                CL_ASSERT(pCompService->pCompCallbacks);

                if(pCompService->pCompCallbacks->fpTxnAgentStop)
                {
                    rc = pCompService->pCompCallbacks->fpTxnAgentStop(
                        (ClTxnTransactionHandleT) (txnOp.pTxnDefn->serverTxnId.txnId),
                        (ClTxnAgentCookieT *) &(txnOp.pTxnJobDefn->cookie) );
                }
                else
                {
                    clLogDebug("AGT", "MTA",
                            "No transaction stop callback provided. Proceeding without error");
                }

            }
            node = currNode;
            rc = clCntNextNodeGet(clTxnAgntCfg->compServiceMap, node, &currNode);
        }
    }

    if(!match)
    {
        clLogError("AGT", "MTA",
                "No matching service found for service [%d]. Transaction[0x%x:0x%x] not stop",
                txnOp.pTxnJobDefn->serviceType,
                tCmd.txnId.txnMgrNodeAddress,
                tCmd.txnId.txnId);
        rc = CL_ERR_NOT_EXIST; /* Match not found, return not exists */
    }
    else
        rc = CL_OK; /* Match found, return CL_OK */

    return rc;
}
