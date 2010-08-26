/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : txn                                                           
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/txn/server/clTxnRecoveryImpl.c $
 * $Author: anil $
 * $Date: 2007/04/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This source file contains transaction recovery implementation
 *
 *
 *****************************************************************************/

#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>

#include <clTxnCommonDefn.h>
#include <clTxnDb.h>

#include <xdrClTxnTransactionIdT.h>
#include <xdrClIocPhysicalAddressT.h>

#include "clTxnServiceIpi.h"
#include "clTxnEngine.h"
#include "clTxnRecovery.h"

/* Forward declaration */
static ClInt32T _clTxnTxnIdCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

static void _clTxnRecoveryLogEntryDel(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data);

static void _clTxnRecoveryLogEntryDestroy(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data);
static ClInt32T _clTxnCmpKeyCompare(
        CL_IN ClCntKeyHandleT key1, 
        CL_IN ClCntKeyHandleT key2);

static void _clTxnCompLogDel(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data);

static void _clTxnCompLogDestroy(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data);
extern ClTxnServiceT    *clTxnServiceCfg;

/* Initialize transaction recovery lib */
ClRcT clTxnRecoveryLibInit()
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    /* Initialize recovery library */
    if (clTxnServiceCfg == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction service is not initialized"));
        CL_FUNC_EXIT();
        return CL_ERR_NOT_INITIALIZED;
    }


    clTxnServiceCfg->pTxnRecoveryContext = (ClTxnRecoveryT *) clHeapAllocate (sizeof (ClTxnRecoveryT) );

    if (clTxnServiceCfg->pTxnRecoveryContext == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory for txn-recovery context"));
        CL_FUNC_EXIT();
        return CL_ERR_NO_MEMORY;
    }

    memset(clTxnServiceCfg->pTxnRecoveryContext, 0, sizeof (ClTxnRecoveryT));

    /* Create container (llist) for maintaining txn-log */
    rc = clCntLlistCreate(_clTxnTxnIdCompare, _clTxnRecoveryLogEntryDel, 
                          _clTxnRecoveryLogEntryDestroy, CL_CNT_UNIQUE_KEY, 
                          &(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb));

    if (CL_OK == rc)
    {
        rc = clOsalMutexCreate ( &(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex) );
    }

    if (CL_OK == rc)
    {
        rc = clTxnDbInit( &(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb) );
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Failed to create structure for storing txn recovery-log. rc:0x%x", rc));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* Finalize transaction recovery lib */
ClRcT clTxnRecoveryLibFini()
{
    ClRcT           rc = CL_OK;
    ClUint32T       outStandingTxn;

    CL_FUNC_ENTER();


    rc = clCntSizeGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, &outStandingTxn);

    /* Remove history db */
    clTxnDbFini(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb);
    /* Remove all nodes from container and delete it */
    //rc = clCntAllNodesDelete(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb);
    rc = clCntDelete(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb);

    rc = clOsalMutexDelete (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (outStandingTxn != 0x0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Outstanding transactions.. saving for future"));
        /* There were some transactions outstanding */
        rc = CL_ERR_TRY_AGAIN;
    }

    clHeapFree(clTxnServiceCfg->pTxnRecoveryContext);

    CL_FUNC_EXIT();
    return (rc);
}

/* Initialize for failure recovery and txn activity */
ClRcT clTxnRecoverySessionInit(
        CL_IN   ClTxnTransactionIdT     serverTxnId)
{
    ClRcT               rc = CL_OK;
    ClTxnActiveTxnT     *pActiveTxn;
    ClTxnRecoveryLogT   *pTxnRecoveryLog;

    CL_FUNC_ENTER();

    /* Create recovery log for newly create txn */
    pTxnRecoveryLog = (ClTxnRecoveryLogT *) clHeapAllocate(sizeof(ClTxnRecoveryLogT));
    if (pTxnRecoveryLog == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("No memory"));
    }

    if (CL_OK == rc)
    {
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                                (ClCntKeyHandleT) &(serverTxnId),
                                (ClCntDataHandleT *) &pActiveTxn);
    }

    if (CL_OK == rc)
    {
        memset(pTxnRecoveryLog, 0, sizeof(ClTxnRecoveryLogT));

        pTxnRecoveryLog->pTxnDefn = pActiveTxn->pTxnDefn;
        /*pTxnRecoveryLog->pActiveTxn = pActiveTxn;*/
        pTxnRecoveryLog->currentState = CL_TXN_STATE_ACTIVE;
        pTxnRecoveryLog->txnId = serverTxnId;
        pTxnRecoveryLog->clientAddr = pActiveTxn->clientAddr;
        pTxnRecoveryLog->recoveryMode = CL_FALSE;
        /* Find unique set of components involved in this transaction */
        /* Incrementally fill-in this structure */
        rc = clCntLlistCreate(_clTxnCmpKeyCompare, _clTxnCompLogDel, 
                              _clTxnCompLogDestroy, CL_CNT_UNIQUE_KEY, 
                              &(pTxnRecoveryLog->txnCompList));

        /* Lock */
        if (CL_OK == rc)
        {
            rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
        }

        if (CL_OK == rc)
        {
            rc = clCntNodeAdd(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                              (ClCntKeyHandleT) &(pTxnRecoveryLog->pTxnDefn->serverTxnId), 
                              (ClCntDataHandleT) pTxnRecoveryLog, NULL);

            /* Make an entry in the history-db. First remove existing, if any */
            clTxnDbTxnDefnRemove(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb, 
                                 serverTxnId);

            rc = clTxnDbNewTxnDefnAdd(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb, 
                                 pTxnRecoveryLog->pTxnDefn, 
                                 &(pTxnRecoveryLog->pTxnDefn->serverTxnId));

            /* Unlock */
            clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
        }
    }

    if (CL_OK == rc)
    {
        /* Create checkpoint for this transaction */
        rc = clTxnServiceCkptNewTxnCheckpoint(pTxnRecoveryLog->pTxnDefn);
    }

    /* Checkpoint transaction recovery log */
    if (CL_OK == rc)
    {
        rc = clTxnServiceCkptRecoveryLogCheckpoint(pTxnRecoveryLog->pTxnDefn->serverTxnId);
    }

    if (rc != CL_OK)
    {
        clLogCritical("SER", NULL,"clTxnRecoverySessionInit failed, rc [0x%x]", rc);        
    }    
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to update the restored recovery log session with latest exec context
 * 
 * NOTE: No need protect txnRecoveryDb as this function is controlled by 
 * clTxnRecoveryInitiate();
 */
ClRcT clTxnRecoverySessionUpdate(
    CL_IN   ClTxnDefnT      *pTxnDefn)
{
    ClRcT                   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;

    CL_FUNC_ENTER();

    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
            (ClCntKeyHandleT) &(pTxnDefn->serverTxnId), (ClCntDataHandleT *)&pTxnRecoveryLog);

    if (CL_OK == rc)
    {
        pTxnRecoveryLog->currentState = CL_TXN_STATE_ACTIVE;
        pTxnRecoveryLog->pTxnDefn = pTxnDefn;

        /* Make an entry in the history-db. First remove existing, if any */
        clTxnDbTxnDefnRemove(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb, 
                             pTxnDefn->serverTxnId);

        rc = clTxnDbNewTxnDefnAdd(clTxnServiceCfg->pTxnRecoveryContext->txnHistoryDb, 
                             pTxnDefn, &(pTxnDefn->serverTxnId));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Failed to update the exec context for txn[0x%x:0x%x]", 
                    pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* Set the partipation of this component in transactin job */
ClRcT clTxnRecoverySetComponent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress, 
        CL_IN   ClUint32T               compCfg)
{
    ClRcT                   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClTxnComponentInfoT     *pTxnAppComp;

    CL_FUNC_ENTER();

    /* Retrieve pTxnRecoveryLog for this serverTxnId */
    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        /* Retrieve pTxnAppComp for this compAddress, if does not exists,
           create one
        */
        rc = clCntDataForKeyGet(pTxnRecoveryLog->txnCompList, 
                                (ClCntKeyHandleT) &compAddress, 
                                (ClCntDataHandleT *)&pTxnAppComp);
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            pTxnAppComp = (ClTxnComponentInfoT *)clHeapAllocate(sizeof (ClTxnComponentInfoT));
            if (pTxnAppComp != NULL)
            {
                pTxnAppComp->compAddress = compAddress;
                pTxnAppComp->compCfg = compCfg;
                pTxnAppComp->pTxnAgentLog = (ClTxnAgentLogT *) 
                    clHeapAllocate(sizeof(ClTxnAgentLogT) * pTxnRecoveryLog->pTxnDefn->jobCount);
                if (pTxnAppComp->pTxnAgentLog != NULL)
                {
                    memset(pTxnAppComp->pTxnAgentLog, 0, 
                           sizeof(ClTxnAgentLogT) * pTxnRecoveryLog->pTxnDefn->jobCount);
                    rc = clCntNodeAdd(pTxnRecoveryLog->txnCompList, 
                                      (ClCntKeyHandleT) &(pTxnAppComp->compAddress), 
                                      (ClCntDataHandleT) pTxnAppComp, NULL);
                    pTxnRecoveryLog->compCount++;
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                    rc = CL_ERR_NO_MEMORY;
                }
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
                rc = CL_ERR_NO_MEMORY;
            }
        }
    }

    if (CL_OK == rc)
    {
        pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= CL_TXN_VALID_LOG;
    }


    CL_FUNC_EXIT();
    return (rc);
}

/* Make a note of the fact that PREPARE cmd has been sent to this agent 
   for given transaction job
*/
ClRcT clTxnRecoveryPrepareCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress)
{
    ClRcT   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClTxnComponentInfoT     *pTxnAppComp;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        /* Retrieve pTxnRecoveryLog for this serverTxnId */
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        /* Retrieve pTxnAppComp for this compAddress, if does not exists,
           create one
        */
        rc = clCntDataForKeyGet(pTxnRecoveryLog->txnCompList, 
                                (ClCntKeyHandleT) &compAddress, 
                                (ClCntDataHandleT *)&pTxnAppComp);
    }

    if (CL_OK == rc)
    {
        pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= CL_TXN_PREPARE_CMD_SENT;
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnRecoveryReceivedResp(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId, 
        CL_IN   ClIocPhysicalAddressT   compAddress, 
        CL_IN   ClTxnMgmtCommandT       cmd, 
        CL_IN   ClRcT                   retCode)
{
    ClRcT                   rc      = CL_OK;
    ClUint32T               shift   = 0x0;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClTxnComponentInfoT     *pTxnAppComp;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        /* Retrieve pTxnRecoveryLog for this serverTxnId */
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        /* Retrieve pTxnAppComp for this compAddress, if does not exists,
           create one
        */
        rc = clCntDataForKeyGet(pTxnRecoveryLog->txnCompList, 
                                (ClCntKeyHandleT) &compAddress, 
                                (ClCntDataHandleT *)&pTxnAppComp);
    }

    switch (cmd)
    {
        case CL_TXN_CMD_PREPARE:
            shift = 0x0;
            break;

        case CL_TXN_CMD_1PC_COMMIT:
        case CL_TXN_CMD_2PC_COMMIT:
            shift = 0x4;
            break;

        case CL_TXN_CMD_ROLLBACK:
            shift = 0x8;
            break;

        default:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid cmd 0x%x to process agent response", cmd));
            rc = CL_ERR_INVALID_PARAMETER;
            break;
    }
    

    if (CL_OK == rc)
    {
        if (retCode == CL_OK) 
        {
            pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= (CL_TXN_ACTIVITY_APP_RESP_CLOK << shift);
        }
        else if (CL_GET_ERROR_CODE(retCode) == CL_ERR_TIMEOUT)
        {
            pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= (CL_TXN_ACTIVITY_APP_TIMEOUT << shift);
        }
        else
        {
            pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= (CL_TXN_ACTIVITY_APP_RESP_ERR << shift);
            pTxnAppComp->pTxnAgentLog[jobId.jobId].appRc = retCode;
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clTxnRecoveryCommitCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress)
{
    ClRcT   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClTxnComponentInfoT     *pTxnAppComp;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    if (CL_OK == rc)
    {
        /* Retrieve pTxnRecoveryLog for this serverTxnId */
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        /* Retrieve pTxnAppComp for this compAddress, if does not exists,
           create one
        */
        rc = clCntDataForKeyGet(pTxnRecoveryLog->txnCompList, 
                                (ClCntKeyHandleT) &compAddress, 
                                (ClCntDataHandleT *)&pTxnAppComp);
    }

    if (CL_OK == rc)
    {
        pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= CL_TXN_COMMIT_CMD_SENT;
    }

    CL_FUNC_EXIT();
    return (rc);
}
  
ClRcT clTxnRecoveryRollbackCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress)
{
    ClRcT   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClTxnComponentInfoT     *pTxnAppComp;

    CL_FUNC_ENTER();

    /* Retrieve pTxnRecoveryLog for this serverTxnId */
    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        /* Retrieve pTxnAppComp for this compAddress, if does not exists,
           create one
        */
        rc = clCntDataForKeyGet(pTxnRecoveryLog->txnCompList, 
                                (ClCntKeyHandleT) &compAddress, 
                                (ClCntDataHandleT *)&pTxnAppComp);
    }

    if (CL_OK == rc)
    {
        pTxnAppComp->pTxnAgentLog[jobId.jobId].activityLog |= CL_TXN_ROLLBACK_CMD_SENT;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to update transaction recovery log instance with failure
 * information
 */
ClRcT clTxnRecoveryTxnFailed(
        CL_IN   ClTxnTransactionIdT     serverTxnId,
        CL_IN   ClTxnTransactionStateT  txnState)
{
    ClRcT                   rc = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        /* Retrieve pTxnRecoveryLog for this serverTxnId */
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) &serverTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    if (CL_OK == rc)
    {
        CL_ASSERT((txnState >= CL_TXN_STATE_PRE_INIT)&&(txnState <=CL_TXN_STATE_UNKNOWN ));
        pTxnRecoveryLog->currentState = txnState;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to initiate recovery of failed transaction.
 * Transaction could be found failed in this session or from
 * previous run of application
 */
ClRcT clTxnRecoveryInitiate()
{
    ClRcT               rc = CL_OK;
    ClCntNodeHandleT    currNode;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    { 
        rc = clCntFirstNodeGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, &currNode);
    }

    while (CL_OK == rc)
    {
        ClCntNodeHandleT    tNode;
        ClTxnRecoveryLogT   *pTxnRecoveryLog;

        rc = clCntNodeUserDataGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, currNode, 
                                  (ClCntDataHandleT *) &pTxnRecoveryLog);
        
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to retrieve recovery log. aborting. rc:0x%x", rc));
            break;
        }

        if (pTxnRecoveryLog->recoveryMode == CL_TRUE)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ("Transaction [0x%x:0x%x] had failed to complete. trying to restore", 
                 pTxnRecoveryLog->txnId.txnMgrNodeAddress, pTxnRecoveryLog->txnId.txnId));

            if (pTxnRecoveryLog->pTxnDefn == NULL)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Txn Defn for txnId[0x%x:0x%x] need to be restored", 
                               pTxnRecoveryLog->txnId.txnMgrNodeAddress, 
                               pTxnRecoveryLog->txnId.txnId));

                rc = clTxnServiceCkptTxnRestore(pTxnRecoveryLog->txnId);
                if(CL_OK != rc)
                {
                    clLogNotice("SER", NULL,
                            "Failed to recover incomplete transaction[0x%x:0x%x]", 
                            pTxnRecoveryLog->txnId.txnMgrNodeAddress, 
                            pTxnRecoveryLog->txnId.txnId);
                    /* Delete recovery checkpoint */
                    clTxnServiceCkptRecoveryLogCheckpointDelete(pTxnRecoveryLog->txnId);

                    /* Delete recovery log for this txn */
                    clCntAllNodesForKeyDelete(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                                              (ClCntKeyHandleT) &(pTxnRecoveryLog->txnId));
                }
                else
                {
                    /* Txn defn is successfully read from ckpt and put into txnRecoveryDb.
                     * Read the defn from txnRecoveryDb.
                     */
                    clLogDebug("SER", "", "Reading txn defn from the recovery db for txnId : [0x%x:0x%x]",
                            pTxnRecoveryLog->txnId.txnMgrNodeAddress,
                            pTxnRecoveryLog->txnId.txnId);

                    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                        (ClCntKeyHandleT) &(pTxnRecoveryLog->txnId), (ClCntDataHandleT *)&pTxnRecoveryLog);
                    if (rc != CL_OK)
                    {
                        clLogError("SERVER", "", "Failed get txn defn from recovery db. rc [0x%x]", rc);
                    }
                }
            }

            /* Add only if txn was successfully read from the recovery ckpt db */
            if(CL_OK == rc)
            {
                clLogDebug("SER", "", "Replaying transaction for txnId : [0x%x:0x%x]",
                        pTxnRecoveryLog->txnId.txnMgrNodeAddress,
                        pTxnRecoveryLog->txnId.txnId);

                rc = clTxnEngineNewTxnReceive(pTxnRecoveryLog->pTxnDefn, 
                                          pTxnRecoveryLog->clientAddr, CL_TRUE, 0, 0);
                pTxnRecoveryLog->recoveryMode = CL_FALSE;   /* declare this to be new one! */
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Result of resuming txn after restart. rc:0x%x", rc));
            }
        }

        tNode = currNode;
        rc = clCntNextNodeGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, tNode, &currNode);
    }

    clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        rc = CL_OK;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to process all failed txn. rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}
  

/* Finalize the failure recovery and txn activity */
ClRcT clTxnRecoverySessionFini(
        CL_IN   ClTxnTransactionIdT     serverTxnId)
{
    ClRcT   rc = CL_OK;

    CL_FUNC_ENTER(); 
    
    /* Call API to delete txndefn data-set */ 
    rc = clTxnServiceCkptTxnDelete(serverTxnId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete txn-defn checkpoint data-set. rc:0x%x", rc));
    }

    rc = clTxnServiceCkptRecoveryLogCheckpointDelete(serverTxnId);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete Recovery-Log checkpoint data-set. rc:0x%x", rc));
    }

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        /* Finalize recovery log for this txn */
        clCntAllNodesForKeyDelete(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                                  (ClCntKeyHandleT) &serverTxnId);

        clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* Routine to pack transaction logs */
ClRcT clTxnRecoveryLogPack(
        CL_IN   ClTxnTransactionIdT     *pTxnId,
        CL_IN   ClBufferHandleT  msgHandle)
{
    ClRcT   rc = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();

    rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_OK == rc)
    {
        /* Retrieve pTxnRecoveryLog for this serverTxnId */
        rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                (ClCntKeyHandleT) pTxnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

    }

    if (CL_OK == rc)
    { 
        rc = VDECL_VER(clXdrMarshallClTxnTransactionIdT, 4, 0, 0)(
                &(pTxnRecoveryLog->pTxnDefn->serverTxnId), msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        rc = clXdrMarshallClUint32T(&(pTxnRecoveryLog->pTxnDefn->currentState), 
                                    msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        rc = VDECL_VER(clXdrMarshallClIocPhysicalAddressT, 4, 0, 0)(&(pTxnRecoveryLog->clientAddr), 
                                                msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        rc = clXdrMarshallClUint32T(&(pTxnRecoveryLog->compCount), msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        rc = clXdrMarshallClUint32T(&(pTxnRecoveryLog->pTxnDefn->jobCount), msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        rc = clCntFirstNodeGet(pTxnRecoveryLog->txnCompList, &currNode);
    }

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClTxnComponentInfoT     *pTxnComp;

        rc = clCntNodeUserDataGet(pTxnRecoveryLog->txnCompList, currNode, 
                                  (ClCntDataHandleT *)&pTxnComp);
        if (CL_OK == rc)
        {
            rc = VDECL_VER(clXdrMarshallClIocPhysicalAddressT, 4, 0, 0)( &(pTxnComp->compAddress), 
                                                     msgHandle, 0);
        }

        if (CL_OK == rc)
        {
            rc = clXdrMarshallClUint32T( &(pTxnComp->compCfg), msgHandle, 0);
        }

        if (CL_OK == rc)
        {
            /* FIXME: Improve using xdr */
            rc = clBufferNBytesWrite(msgHandle, 
                    (ClUint8T *)pTxnComp->pTxnAgentLog, 
                    (sizeof(ClTxnAgentLogT) * pTxnRecoveryLog->pTxnDefn->jobCount) );
        }

        tNode = currNode;
        rc = clCntNextNodeGet(pTxnRecoveryLog->txnCompList, tNode, &currNode);
    }
    clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        rc = CL_OK;
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create transaction state checkpoint, rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* Routine to unpack transaction logs */
ClRcT clTxnRecoveryLogUnpack(
        CL_IN   ClBufferHandleT  msgHandle)
{
    ClRcT                   rc  = CL_OK;
    ClTxnTransactionIdT     txnId = {0};
    ClTxnTransactionStateT  txnState = CL_TXN_STATE_PRE_INIT;
    ClTxnRecoveryLogT       *pTxnRecoveryLog = NULL;
    ClUint32T               index = 0;
    ClUint32T               jobCount = 0;

    CL_FUNC_ENTER();

    rc = VDECL_VER(clXdrUnmarshallClTxnTransactionIdT, 4, 0, 0)(msgHandle, &txnId);

    if (CL_OK == rc)
    {
        rc = clXdrUnmarshallClUint32T(msgHandle, &txnState);
    }

    if ( (CL_OK == rc) && ( (txnState == CL_TXN_STATE_COMMITTED) ||
                            (txnState == CL_TXN_STATE_ROLLED_BACK) ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO, 
            ("Transaction [0x%x:0x%x] had reached stable-state. Ignoring it", 
             txnId.txnMgrNodeAddress, txnId.txnId));
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    /* Create recovery log for newly create txn */
    pTxnRecoveryLog = (ClTxnRecoveryLogT *) clHeapAllocate(sizeof(ClTxnRecoveryLogT));
    if (pTxnRecoveryLog == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clLogError("SER", NULL,
                "Error in memory allocation");
        return rc;
    }
    memset(pTxnRecoveryLog, 0, sizeof(ClTxnRecoveryLogT));

    CL_ASSERT((txnState >= CL_TXN_STATE_PRE_INIT)&&(txnState <=CL_TXN_STATE_UNKNOWN ));
    pTxnRecoveryLog->currentState = txnState;
    pTxnRecoveryLog->txnId = txnId;
    pTxnRecoveryLog->recoveryMode = CL_TRUE;

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to restore transaction definition. rc:0x%x, Aborting", rc));
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    rc = VDECL_VER(clXdrUnmarshallClIocPhysicalAddressT, 4, 0, 0)(msgHandle, &(pTxnRecoveryLog->clientAddr));

    if (CL_OK == rc)
    { 
        rc = clXdrUnmarshallClUint32T(msgHandle, &(pTxnRecoveryLog->compCount));
    }

    if (CL_OK == rc)
    {
        rc = clCntLlistCreate(_clTxnCmpKeyCompare, _clTxnCompLogDel, 
                              _clTxnCompLogDestroy, CL_CNT_UNIQUE_KEY, 
                              &(pTxnRecoveryLog->txnCompList));
    }

    rc = clXdrUnmarshallClUint32T(msgHandle, &jobCount);

    for (index = 0; (CL_OK == rc) && (index < pTxnRecoveryLog->compCount); index++)
    {
        ClTxnComponentInfoT     *pTxnComp;
        ClUint32T               size = sizeof(ClTxnAgentLogT) * jobCount;

        pTxnComp = (ClTxnComponentInfoT *)clHeapAllocate(sizeof(ClTxnComponentInfoT));
        if (pTxnComp == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
            rc = CL_ERR_NO_MEMORY;
            break;
        }
        memset(pTxnComp, 0, sizeof(ClTxnComponentInfoT));

        pTxnComp->pTxnAgentLog = (ClTxnAgentLogT *) clHeapAllocate(size);
        if (pTxnComp->pTxnAgentLog == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
            rc = CL_ERR_NO_MEMORY;
            break;
        }
        memset(pTxnComp->pTxnAgentLog, 0, size);

        rc = VDECL_VER(clXdrUnmarshallClIocPhysicalAddressT, 4, 0, 0)( msgHandle, &(pTxnComp->compAddress) );
        if (CL_OK != rc)
            break; 
        
        rc = clXdrUnmarshallClUint32T( msgHandle, &(pTxnComp->compCfg) );
        if (CL_OK != rc)
            break;


        rc = clBufferNBytesRead(msgHandle, (ClUint8T *) pTxnComp->pTxnAgentLog, &size);
        if (CL_OK != rc)
            break;

        rc = clCntNodeAdd(pTxnRecoveryLog->txnCompList, 
                          (ClCntKeyHandleT) &(pTxnComp->compAddress), 
                          (ClCntDataHandleT) pTxnComp, NULL);
    }

    if (CL_OK == rc)
    {
        /* Lock */
        rc = clOsalMutexLock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);

        if (CL_OK == rc)
        {
            rc = clCntNodeAdd(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
                              (ClCntKeyHandleT) &(pTxnRecoveryLog->txnId), 
                              (ClCntDataHandleT) pTxnRecoveryLog, NULL);
            /* Unlock */
            clOsalMutexUnlock (clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryMutex);
        }

        CL_DEBUG_PRINT(CL_DEBUG_INFO, 
            ("Restored txn-log of txn[0x%x:0x%x], state:0x%x, jobCount:0x%x, compCount:0x%x, rc:0x%x",
                    txnId.txnMgrNodeAddress, txnId.txnId, txnState, 
                    jobCount, pTxnRecoveryLog->compCount, rc));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to extract txn-log from ckpt. rc:0x%x", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Key compare function used to manage hash-map of active transactions.
 */
static ClInt32T _clTxnTxnIdCompare(
        CL_IN ClCntKeyHandleT key1, 
        CL_IN ClCntKeyHandleT key2)
{

    return clTxnUtilTxnIdCompare( (*(ClTxnTransactionIdT *)key1), 
                                  (*(ClTxnTransactionIdT *)key2));

}

static ClInt32T _clTxnCmpKeyCompare(
        CL_IN ClCntKeyHandleT key1, 
        CL_IN ClCntKeyHandleT key2)
{
    ClIocAddressT   *c1, *c2;
    CL_FUNC_ENTER();
    c1 = (ClIocAddressT *)key1;
    c2 = (ClIocAddressT *)key2;

    CL_FUNC_EXIT();
    return (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress == 0) ? (c1->iocPhyAddress.portId - c2->iocPhyAddress.portId) : (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress);
}

static void _clTxnRecoveryLogEntryDel(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnRecoveryLogT   *pTxnRecoveryLog = (ClTxnRecoveryLogT *)data;

    pTxnRecoveryLog->pTxnDefn = NULL;

    clCntDelete(pTxnRecoveryLog->txnCompList);

    clHeapFree(pTxnRecoveryLog);
}

static void _clTxnRecoveryLogEntryDestroy(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnRecoveryLogT   *pTxnRecoveryLog = (ClTxnRecoveryLogT *)data;

    pTxnRecoveryLog->pTxnDefn = NULL;

    clCntDelete(pTxnRecoveryLog->txnCompList);

    clHeapFree(pTxnRecoveryLog);
}
static void _clTxnCompLogDel(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnComponentInfoT     *pTxnAppComp;

    pTxnAppComp = (ClTxnComponentInfoT *) data;

    clHeapFree(pTxnAppComp->pTxnAgentLog);
    clHeapFree(pTxnAppComp);
}

static void _clTxnCompLogDestroy(
        CL_IN   ClCntKeyHandleT     key,
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnComponentInfoT     *pTxnAppComp;

    pTxnAppComp = (ClTxnComponentInfoT *) data;

    clHeapFree(pTxnAppComp->pTxnAgentLog);
    clHeapFree(pTxnAppComp);
}

/**
 * Appends transaction log for display purpose
 */
ClRcT clTxnRecoveryLogShow(
        CL_IN   ClTxnTransactionIdT     txnId,
        CL_IN   ClBufferHandleT  msgHandle)
{
    ClRcT                   rc  = CL_OK;
    ClTxnRecoveryLogT       *pTxnRecoveryLog;
    ClCntNodeHandleT        currNode;

    ClCharT                 str[1024];

    CL_FUNC_ENTER();

    /* Retrieve pTxnRecoveryLog for this serverTxnId */
    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnRecoveryContext->txnRecoveryDb, 
            (ClCntKeyHandleT) &txnId, (ClCntDataHandleT *)&pTxnRecoveryLog);

    str[0] = '\0';
    if (CL_OK != rc)
    {
        sprintf(str, "\n\tINVALID TXN_ID or NO TXN LOG\n");
        clBufferNBytesWrite(msgHandle, (ClUint8T *)str, strlen(str));
        CL_FUNC_EXIT();
        return CL_OK;
    }

    sprintf(str, "\n\tRecovery State\t:\n"  \
                 "\t\tRetry \t:\t0x%x\n", pTxnRecoveryLog->retryCount);

    rc = clBufferNBytesWrite(msgHandle, (ClUint8T *)str, strlen(str));


    /* Walk through all jobs of this transaction */
    rc = clCntFirstNodeGet (pTxnRecoveryLog->txnCompList, &currNode);

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClTxnComponentInfoT     *pTxnAppComp;

        rc = clCntNodeUserDataGet(pTxnRecoveryLog->txnCompList, currNode, 
                                  (ClCntDataHandleT *)&pTxnAppComp);

        if (CL_OK == rc)
        {
            ClUint32T   index;
            str[0] = '\0';
            sprintf(str, "\n\tExecution State\t\t:\tComponent[0x%x:0x%x]\n"     \
                         "\t\tComp Cfg\t:\t0x%x\n", 
                        pTxnAppComp->compAddress.nodeAddress, 
                        pTxnAppComp->compAddress.portId, 
                        pTxnAppComp->compCfg);
            rc = clBufferNBytesWrite(msgHandle, (ClUint8T *)str, strlen(str));
            for (index = 0; index < pTxnRecoveryLog->pTxnDefn->jobCount; index++)
            {
                ClUint32T   log = pTxnAppComp->pTxnAgentLog[index].activityLog;
                ClRcT       retCode = pTxnAppComp->pTxnAgentLog[index].appRc;

                str[0] = '\0';
                if (log & CL_TXN_VALID_LOG)
                {
                    sprintf(str, "\n\t\tJob-Id\t:\t0x%x\n"          \
                                 "\t\tPREPARE-PHASE\t:\t%s\n"       \
                                 "\t\t\tRC:\t:\t%s RetCode[0x%x]\n" \
                                 "\t\tCOMMIT-PHASE\t:\t%s\n"        \
                                 "\t\t\tRC:\t:\t%s RetCode[0x%x]\n" \
                                 "\t\tROLLBACK-PHASE\t:\t%s\n"      \
                                 "\t\t\tRC:\t:\t%s RetCode[0x%x]\n",
                                 index, 
                                 (log & CL_TXN_PREPARE_CMD_SENT) ? "CMD_SENT" : "CMD_NOT_SENT", 
                                 (log & CL_TXN_PREPARE_APP_TIMEOUT) ? "TIMEOUT" : (log & CL_TXN_PREPARE_APP_RESP_ERR) ? "FALUIRE" : (log & CL_TXN_PREPARE_APP_RESP_CLOK) ? "SUCCESS" : "NOT AVAILABLE", retCode,
                                 (log & CL_TXN_COMMIT_CMD_SENT) ? "CMD_SENT" : "CMD_NOT_SENT", 
                                 (log & CL_TXN_COMMIT_APP_TIMEOUT) ? "TIMEOUT" : (log & CL_TXN_COMMIT_APP_RESP_ERR) ? "FALUIRE" : (log & CL_TXN_COMMIT_APP_RESP_CLOK) ? "SUCCESS" : "NOT AVAILABLE", retCode,
                                 (log & CL_TXN_ROLLBACK_CMD_SENT) ? "CMD_SENT" : "CMD_NOT_SENT", 
                                 (log & CL_TXN_ROLLBACK_APP_TIMEOUT) ? "TIMEOUT" : (log & CL_TXN_ROLLBACK_APP_RESP_ERR) ? "FALUIRE" : (log & CL_TXN_ROLLBACK_APP_RESP_CLOK) ? "SUCCESS" : "NOT AVAILABLE", retCode);
                }
                else
                {
                    sprintf(str, "\n\t\tJob-Id\t:\t0x%x - INVALID", index);
                }
                rc = clBufferNBytesWrite(msgHandle, (ClUint8T *)str, strlen(str));
            }

            if (CL_OK == rc)
            {
                tNode = currNode;
                rc = clCntNextNodeGet (pTxnRecoveryLog->txnCompList, tNode, &currNode);
            }
        }
    }

    CL_FUNC_EXIT();
    return (rc);

}
