/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : txn                                                           
 * File        : clTxnClientImpl.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains transaction-client side implementation for communication
 * and transaction execution. This implementation is internal to transaction client
 *
 *
 *****************************************************************************/

#include <string.h>

#include <clDebugApi.h>
#include <clCntApi.h>
#include <clVersionApi.h>

#include <clTxnErrors.h>

#include <clTxnCommonDefn.h>
#include <clTxnDb.h>
#include <clTxnStreamIpi.h>
#include <xdrClTxnMessageHeaderIDLT.h>
#include <xdrClTxnCmdT.h>
#include <clTxnXdrCommon.h>

#include "clTxnClientIpi.h"

extern ClTxnClientInfoT*    clTxnClientInfo;

/**
 * List of supported version (application client)
 */
extern ClVersionT   clTxnClientVersionSupported[]; 

/**
 * Version Database
 */
extern ClVersionDatabaseT clTxnClientVersionDb; 

/****************************************************************************
                    Internal (Private) Functions
****************************************************************************/

/**
 * Internal function. This function is responsible for releasing/freeing 
 * a completed transaction
 */
ClRcT clTxnTransactionFinalize(
        CL_IN ClTxnDefnT    *pTxnDefn)
{
    ClRcT rc = CL_OK;
    ClUint16T   txnId;

    CL_FUNC_ENTER();

    if (NULL == pTxnDefn)
    {
        txnId = -1;
        rc = CL_ERR_NULL_POINTER;
        goto error;
    }

    txnId = pTxnDefn->clientTxnId.txnId;

    rc = clTxnDefnDelete(pTxnDefn);

error:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to release memory allocated for transaction txnId:0x%x. rc:0x%x\n", txnId, rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT 
clTxnClientProcessMsg(ClBufferHandleT  inMsg) 

{
    ClRcT   rc  = CL_OK;
    ClTxnMessageHeaderT     msgHeader = {{0}};
    ClTxnMessageHeaderIDLT  msgHeadIDL = {{0}};
    ClTxnFailedTxnT        *pFailedTxn = NULL;
    int msgCnt;
    

    CL_FUNC_ENTER();

    rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(inMsg, &msgHeadIDL);
    if(CL_OK != rc)
    {
        clLogError("CLT", NULL,
                "Error in unmarshalling msg header, rc=[0x%x]", rc);
        return rc;
    }
    _clTxnCopyIDLToMsgHead(&msgHeader, &msgHeadIDL);
    CL_ASSERT(msgHeader.msgCount);

    rc = clVersionVerify (&clTxnClientVersionDb, &(msgHeader.version));
    if (CL_OK != rc)
    {
        clLogError("CLT", NULL, 
                "Un-supported version [0x%x,%c,%c]. rc:0x%x\n", 
                rc, msgHeader.version.releaseCode,
                msgHeader.version.majorVersion,
                msgHeader.version.minorVersion);
        CL_FUNC_EXIT();
        return (CL_GET_ERROR_CODE(rc));
    }
    clLog(CL_LOG_DEBUG, "CLT", NULL, 
            "Received [%d] messages from server", msgHeader.msgCount);

    msgCnt = msgHeader.msgCount;
    while ( (CL_OK == rc) && (msgCnt > 0) )
    {
        ClTxnCmdT   tCmd;

        msgCnt--;

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(inMsg, &tCmd);
        switch (tCmd.cmd)
        {
            case CL_TXN_CMD_RESPONSE:
                {
                    ClTxnDefnT  *pTxnDefn = NULL;

                    /* Lock before accessing */
                    clTxnMutexLock(clTxnClientInfo->txnMutex);
                    rc = clTxnDbTxnDefnGet(
                            clTxnClientInfo->activeTxnDb, 
                            tCmd.txnId, 
                            &pTxnDefn);
                    if (CL_OK != rc)
                    {
                        /* NO transaction exist. Chances are that it has already been processed */
                        clLogTrace("CLT", "MTC",
                                "Transaction[0x%x:0x%x] doesnt exist",
                                tCmd.txnId.txnMgrNodeAddress,
                                tCmd.txnId.txnId);
                        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                        rc = CL_OK; /* Return CL_OK */
                        break;
                    }
                    clLog(CL_LOG_DEBUG,"CLT", NULL, 
                            "Received response [0x%x] from server for transaction with mgrAddress:txnId as [0x%x:0x%x]", 
                            tCmd.resp, 
                            tCmd.txnId.txnMgrNodeAddress, 
                            tCmd.txnId.txnId); 
                    if(tCmd.resp == CL_TXN_STATE_ROLLED_BACK)
                    {
                        pFailedTxn = (ClTxnFailedTxnT *) clHeapAllocate(sizeof(ClTxnFailedTxnT));
                        if(NULL == pFailedTxn)
                        {
                            clLog(CL_LOG_CRITICAL, "CLT", NULL,
                                    "Memory allocation failed while trying to allocate the failedAgentList");
                            clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                            break;

                        }
                        clTxnAgentListCreate( &(pFailedTxn->failedAgentList));
                        clTxnStreamTxnAgentDataUnpack(inMsg,
                                &(pFailedTxn->failedAgentList),
                                &(pFailedTxn->failedAgentCount));
                        pFailedTxn->fpTxnAgentCallback = NULL;
                        clLog(CL_LOG_DEBUG, "CLT", NULL, 
                                "Transaction rolledback. Failed agent count [%d]\n", 
                                pFailedTxn->failedAgentCount);

                        rc = clCntNodeAdd(clTxnClientInfo->failedTxnDb, 
                                (ClCntKeyHandleT) pTxnDefn, 
                                (ClCntDataHandleT) pFailedTxn, 0);
                        if(CL_OK != rc)
                        {
                            clLogError("CLT", NULL, 
                                    "Failed in adding to the failed db, key[0x%x:0x%x], rc[0x%x]", 
                                    pTxnDefn->clientTxnId.txnMgrNodeAddress, 
                                    pTxnDefn->clientTxnId.txnId, rc);
                        }

                    }

                    /* Verify a valid transaction state */
                    if ((tCmd.resp >= CL_TXN_STATE_PRE_INIT)&&(tCmd.resp < CL_TXN_STATE_UNKNOWN))
                      pTxnDefn->currentState = tCmd.resp;
                    else
                    {
                        clLogWarning("CLT","MTC","Invalid transaction state [0x%x] in reply",tCmd.resp);
                        pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;
                    }
                    

                    if (pTxnDefn->txnCfg & CL_TXN_TRANSACTION_RUN_SYNC)
                    {
                        clLogDebug("CLT", "MTC",
                                "Received response for sync call for transaction [0x%x:0x%x]",
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId);
                        clLogDebug("CLT", "MTC",
                                "Transaction[0x%x:0x%x] successfully processed. Broadcasting signal",
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId);
                        rc = clOsalCondBroadcast(clTxnClientInfo->txnClientCondVar);
                        CL_ASSERT(CL_OK == rc);
                        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                    }
                    else 
                    {
                        ClRcT   status = CL_OK;

                        /*  Call application provided callback fn */
                        clLogDebug("CLT", "MTC",
                                "Received response for async call for transaction with txnId[0x%x:0x%x]. Invoking callback function", 
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId);
                        if(!clTxnClientInfo->fpTxnCompletionCallback)
                        {
                            clLogNotice("CLT", "MTC",
                                    "For transaction[0x%x:0x%x] with config [%d], application callback is NULL",
                                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                    pTxnDefn->clientTxnId.txnId,
                                    pTxnDefn->txnCfg);
                            clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                            break;
                        }

                        if (pTxnDefn->currentState == CL_TXN_STATE_COMMITTED)
                        {
                            status = CL_OK;
                        }
                        else if (pTxnDefn->currentState == CL_TXN_STATE_ROLLED_BACK)
                        {
                            status = CL_TXN_ERR_TRANSACTION_ROLLED_BACK;
                        }
                        else
                        {
                            status = CL_TXN_ERR_TRANSACTION_FAILURE;
                            clLogError("CLT", NULL,
                                "Transaction [0x%x:0x%x] state is [%d], marking it FAILED",
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId,
                                pTxnDefn->currentState);

                        } 

                        clLogInfo("CLT", NULL,
                                "Transaction having txnId[0x%x:0x%x] is [%s], invoking callback",
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId,
                                (CL_OK == status)?"COMMITTED":
                                ((CL_TXN_ERR_TRANSACTION_ROLLED_BACK == status)?
                                 "ROLLEDBACK":"in an UNKNOWN state"));

                        /* Unlock and call application callback */
                        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                        clTxnClientInfo->fpTxnCompletionCallback(
                                (ClTxnTransactionHandleT)(ClWordT)pTxnDefn, 
                                CL_TXN_RC(status));
                    }
                }
                break;
            default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid comamnd received 0x%x", tCmd.cmd));
                rc = CL_ERR_INVALID_PARAMETER;
                break;
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/* Called from rmdSendTimerFunc.  Maybe when RMDs time out? */
void clTxnClientResMsgReceive(
        ClRcT            retCode, 
        ClPtrT           pCookie, 
        ClBufferHandleT  inMsg, 
        ClBufferHandleT  outMsg)
{
    ClRcT   rc = CL_OK;
    ClTxnTransactionIdT *pTid = NULL;

    if (!pCookie)
    {
        clLogError("CLT", "MTC", "NULL pointer passed in pCookie.");
        return; 
    }

    pTid = (ClTxnTransactionIdT *)pCookie;

    clLogDebug("CLT", "MTC",
            "Received reply from server for txn[0x%x:0x%x], retCode [0x%x]", 
            pTid->txnMgrNodeAddress,
            pTid->txnId,
            retCode);

    if(retCode == CL_OK)
    {
        rc = clTxnClientProcessMsg(outMsg);
    }
    /* Mostly a timeout error. Wake up calling thread */ 
    else
    {
        ClTxnDefnT  *pTxnDefn = NULL;


        clLogError("CLT", "MTC",
                "Error condition [0x%x], waking up transaction[0x%x:0x%x]",
                retCode,
                pTid->txnMgrNodeAddress,
                pTid->txnId);

        /* Extract transaction from activeTxnDb and wake up the threads */
        clTxnMutexLock(clTxnClientInfo->txnMutex);
        rc = clTxnDbTxnDefnGet(
                clTxnClientInfo->activeTxnDb, 
                *pTid, 
                &pTxnDefn);
        if (CL_OK != rc)
        {
            /* NO transaction exist. Chances are that it has already been processed */
            clLogError("CLT", "MTC",
                    "Transaction[0x%x:0x%x] doesnt exist",
                    pTid->txnMgrNodeAddress,
                    pTid->txnId);
            clTxnMutexUnlock(clTxnClientInfo->txnMutex);
            goto free_buf;
        }

        pTxnDefn->currentState = CL_TXN_STATE_UNKNOWN;
        

        if (pTxnDefn->txnCfg & CL_TXN_TRANSACTION_RUN_SYNC)
        {
            clLogDebug("CLT", "MTC",
                    "Received response for sync call for transaction [0x%x:0x%x]",
                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                    pTxnDefn->clientTxnId.txnId);
            clLogDebug("CLT", "MTC",
                    "Transaction[0x%x:0x%x] successfully processed. Broadcasting signal",
                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                    pTxnDefn->clientTxnId.txnId);
            rc = clOsalCondBroadcast(clTxnClientInfo->txnClientCondVar);
            CL_ASSERT(CL_OK == rc);
            clTxnMutexUnlock(clTxnClientInfo->txnMutex);
        }
        else 
        {
            /*  Call application provided callback fn */
            clLogDebug("CLT", "MTC",
                    "Received response for async call for transaction with txnId[0x%x:0x%x]. Invoking callback function", 
                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                    pTxnDefn->clientTxnId.txnId);
            if(!clTxnClientInfo->fpTxnCompletionCallback)
            {
                clLogNotice("CLT", "MTC",
                        "For transaction[0x%x:0x%x] with config [%d], application callback is NULL",
                        pTxnDefn->clientTxnId.txnMgrNodeAddress,
                        pTxnDefn->clientTxnId.txnId,
                        pTxnDefn->txnCfg);
                clTxnMutexUnlock(clTxnClientInfo->txnMutex);
            }
            else
            {
                /* Unlock and call the application callback */
                clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                clLogInfo("CLT", "MTC",
                        "Calling application callback for txn[0x%x:0x%x]", 
                        pTxnDefn->clientTxnId.txnMgrNodeAddress,
                        pTxnDefn->clientTxnId.txnId);
                clTxnClientInfo->fpTxnCompletionCallback(
                        (ClTxnTransactionHandleT)(ClWordT)pTxnDefn, 
                         CL_TXN_RC(CL_TXN_ERR_TRANSACTION_FAILURE));
            }
        }
    }


    if(CL_OK != rc)
    {
        clLogError("CLT", "MTC",
                "Processing msg from server failed with error [0x%x]", 
                rc);
    }
free_buf:
    if(pTid)
        clHeapFree(pTid);
    clBufferDelete(&outMsg);

}

ClRcT clTxnTransactionRunSync(
        CL_IN   ClTxnDefnT      *pTxnDefn)
{
    ClRcT       rc  = CL_OK;

    CL_FUNC_ENTER();

    clTxnMutexLock(clTxnClientInfo->txnMutex);
    pTxnDefn->txnCfg |= CL_TXN_TRANSACTION_RUN_SYNC;
    clTxnMutexUnlock(clTxnClientInfo->txnMutex);

    rc = clTxnTransactionRunAsync(pTxnDefn);

    if (CL_OK == rc)
    {
        ClTimerTimeOutT timeOut = {
            .tsSec      = CL_TXN_PROCESSING_TIME, 
            .tsMilliSec = 0x0
        };
        /* Lock the mutex and wait on conditional var */
        clTxnMutexLock(clTxnClientInfo->txnMutex);
        while ( pTxnDefn->currentState == CL_TXN_STATE_ACTIVE )
        {
            clLogTrace("CLT", "MTC", 
                    "Waiting for response from server for txn[0x%x:0x%x]", 
                        pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId);
            rc = clOsalCondWait(clTxnClientInfo->txnClientCondVar, 
                                clTxnClientInfo->txnMutex, timeOut);
            if (CL_OK != rc)
            {
                clLogError("CLT", "MTC", 
                    "Error while waiting for response from txn-server for txn[0x%x:0x%x], rc[0x%x]. Aborting", 
                     pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId, rc);
                break;
            }
        }
        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
    }
    else
        clLogError("CLT", "MTC",
                "Failed to complete transaction successfully with error [0x%x]",
                rc);

    if (CL_OK == rc)
    {
        if (pTxnDefn->currentState == CL_TXN_STATE_COMMITTED)
        {
            rc = CL_OK;
        }
        else if (pTxnDefn->currentState == CL_TXN_STATE_ROLLED_BACK)
        {
            rc = CL_TXN_ERR_TRANSACTION_ROLLED_BACK;
        }
        else
        {
            rc = CL_TXN_ERR_TRANSACTION_FAILURE;
            clLogError("CLT", "MTC", "Transaction [0x%x:0x%x] state is [%d], returning transaction failure",
                                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                                pTxnDefn->clientTxnId.txnId,
                                pTxnDefn->currentState);
        }
        clLogInfo("CLT", "MTC", 
                "Transaction[0x%x:0x%x] is at state[%s], returning rc[0x%x]",
                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                pTxnDefn->clientTxnId.txnId,
                clTxnStateGet(pTxnDefn->currentState), rc);
    } 
    
    CL_FUNC_EXIT();
    return (rc);
}


/**
  * Private function for processing txn-request and initiating txn-manaegment
  */
ClRcT clTxnTransactionRunAsync(
        CL_IN   ClTxnDefnT  *pTxnDefn)
{
    ClRcT                   rc = CL_OK;

    ClIocAddressT           mgrAddress = {
        .iocPhyAddress.nodeAddress = clIocLocalAddressGet(),
        .iocPhyAddress.portId = CL_IOC_TXN_PORT,
    };

    ClTxnCmdT   txnCmd  = {
        .txnId.txnId                = pTxnDefn->clientTxnId.txnId,
        .txnId.txnMgrNodeAddress    = pTxnDefn->clientTxnId.txnMgrNodeAddress,
        .jobId                      = {{0, 0}, 0},
        .cmd                        = CL_TXN_CMD_PROCESS_TXN,
        .resp                       = CL_OK
    };

    ClBufferHandleT     cfgMsgHandle;
    ClTxnCommHandleT    commHandle;
    ClUint8T            *pTxnCfg = NULL;
    ClUint32T           txnCfgLen;

    CL_FUNC_ENTER();

    /* Lock this transaction now */
    clTxnMutexLock(clTxnClientInfo->txnMutex);
    /* Check the currents status of this transaction */ 
    if (pTxnDefn->currentState != CL_TXN_STATE_PRE_INIT) 
    {
        rc = CL_TXN_RC(CL_ERR_INVALID_STATE);
        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
        return rc;
    }
    else
    {
        pTxnDefn->currentState = CL_TXN_STATE_ACTIVE;
    }
    /* Unlock */
    clTxnMutexUnlock(clTxnClientInfo->txnMutex);

    /* Pack configuration along with app-job-defn information and invoke transaction */
    if ( CL_OK == (rc = clBufferCreate(&cfgMsgHandle)) )
        rc = clTxnStreamTxnCfgInfoPack(pTxnDefn, cfgMsgHandle);

    if (CL_OK == rc)
        rc = clBufferLengthGet(cfgMsgHandle, &txnCfgLen);

    if (CL_OK == rc)
    {
        pTxnCfg = (ClUint8T *) clHeapAllocate(txnCfgLen);

        if (pTxnCfg == NULL)
            rc = CL_ERR_NO_MEMORY;
        else
            memset(pTxnCfg, 0, txnCfgLen);
    }

    if (CL_OK == rc)
    {
        ClUint32T   tLen = txnCfgLen;
        rc = clBufferNBytesRead(cfgMsgHandle, pTxnCfg, &tLen);
    }

    if (CL_OK != rc)
    {
        clHeapFree(pTxnCfg);
        clBufferDelete(&cfgMsgHandle);
        clLogError("CLT", "CTM", 
                "Failed to prepare for txn-cfg stream,  rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return (CL_GET_ERROR_CODE(rc));
    } 
    
    rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_CLIENT_REQ, 
                                      mgrAddress.iocPhyAddress, 
                                      CL_TXN_SERVICE_CLIENT_REQ_RECV, 
                                      clTxnClientResMsgReceive, 
                                      CL_TXN_PROCESSING_TIME, 
                                      txnCmd.txnId.txnId, 
                                      &commHandle);
    if(CL_OK != rc)
    {
        clLogError("CLT", "CTM",
                "Failed to create new session having msgType[%d], funcId[%d], address[0x%x:0x%x] with error[0x%x]",
                CL_TXN_MSG_CLIENT_REQ, CL_TXN_SERVICE_CLIENT_REQ_RECV, 
                mgrAddress.iocPhyAddress.nodeAddress, mgrAddress.iocPhyAddress.portId, 
                rc); 
        clHeapFree(pTxnCfg);
        clBufferDelete(&cfgMsgHandle);
        return rc;
    }

    rc = clTxnCommIfcSessionAppendPayload(commHandle, &txnCmd, pTxnCfg, txnCfgLen);
    if(CL_OK != rc)
    {
        clLogError("CLT", "CTM",
                "Failed to append payload with error[0x%x] during transaction start", rc); 
        clHeapFree(pTxnCfg);
        clBufferDelete(&cfgMsgHandle);
        return rc;
    }

    rc = clTxnCommIfcSessionRelease(commHandle);
    if(CL_OK != rc)
    {
        clLogError("CLT", "CTM",
                "Failed to release session with error[0x%x] during transaction start", rc); 
        clHeapFree(pTxnCfg);
        clBufferDelete(&cfgMsgHandle);
        return rc;
    }

    clLogTrace("CLT", "CTM", 
        "Sending command to txn-server for txn[0x%x:0x%x] using comm-handle[%p]", 
        pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId, 
        (void *)commHandle); 
    rc = clTxnCommIfcSessionClose(commHandle, CL_FALSE);
    if(CL_OK != rc)
    {
        clLogError("CLT", "CTM",
                "Failed while closing session for txn[0x%x:0x%x] for processing with error[0x%x]", 
                 pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId, rc);
    }
    else
        clLogInfo("CLT", "CTM",
                "Successfully closed session for txn[0x%x:0x%x]", 
                 pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId);

    if(CL_OK != clTxnCommIfcSessionCancel(commHandle))
    {
        clLogError("CLT", NULL, "Failed to cancel session,");
    }

    clHeapFree(pTxnCfg);
    clBufferDelete(&cfgMsgHandle);

    CL_FUNC_EXIT();
    return (rc);
}

        
/**
  * Specific to COR
  */
ClRcT clTxnReadJobAsync(
        CL_IN   ClTxnDefnT  *pTxnDefn,
        ClTxnFailedTxnT        *pFailedTxn)
{
    ClRcT               rc = CL_OK;
    ClRcT               retCode = CL_OK;
    ClCntNodeHandleT    jobCurrNode;
    ClTxnCommHandleT    commHandle;

    CL_FUNC_ENTER();

    clLogTrace("CLT", "RDT",
             "Sending data to agent . JOB COUNT : %d", pTxnDefn->jobCount);
    retCode = clCntFirstNodeGet(pTxnDefn->jobList, &jobCurrNode);
    while ( (retCode == CL_OK) && (CL_OK == rc) )
    {
        ClBufferHandleT         jobDefn;
        ClUint8T                *pJobDefnStr = NULL;
        ClUint32T               jobDefnLen;
        ClRcT                   compRetCode = CL_OK;
        ClCntNodeHandleT        currNode;
        ClCntNodeHandleT        jobNode;
        ClTxnAppJobDefnT        *pTxnJob = NULL;
        
        retCode = clCntNodeUserDataGet( pTxnDefn->jobList, jobCurrNode, 
                                   (ClCntDataHandleT *)&pTxnJob);
        rc = clBufferCreate (&jobDefn);

        if (CL_OK == rc)
        {
            rc = clTxnStreamTxnDataPack(pTxnDefn, pTxnJob, jobDefn);
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


        clLogTrace("CMN", NULL,
                 "[%s]. COMP COUNT : %d", 
                __FUNCTION__, pTxnJob->compCount);
        compRetCode = clCntFirstNodeGet(pTxnJob->compList, &currNode);
        while ( (compRetCode == CL_OK) && (CL_OK == rc) )
        {
            ClTxnAppComponentT      *pAppComp;
            ClCntNodeHandleT        node;

            /* Retrieve data-node for node */
            compRetCode = clCntNodeUserDataGet( pTxnJob->compList, currNode, 
                                       (ClCntDataHandleT *)&pAppComp);
            if (CL_OK == compRetCode)
            {
                ClTxnCmdT   cmd = {
                    .txnId = pTxnDefn->clientTxnId,
                    .jobId = pTxnJob->jobId,
                    .cmd   = CL_TXN_CMD_READ_JOB,
                };
    
                if (CL_OK == rc)
                {
                    rc = clTxnCommIfcNewSessionCreate(CL_TXN_MSG_CLIENT_REQ_TO_AGNT, 
                                                        pAppComp->appCompAddress, 
                                                        CL_TXN_AGENT_MGR_CMD_RECV, 
                                                        clTxnAgentJobMsgReceive, 
                                                        CL_TXN_JOB_READ_TIME ,
                                                        CL_TXN_COMMON_ID, /* Value is 0x1 */ 
                                                        &commHandle);
                }

                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Sending txn-defn to agent 0x%x:0x%x, rc:0x%x", 
                                pAppComp->appCompAddress.nodeAddress, 
                                pAppComp->appCompAddress.portId, rc));
                if (CL_OK == rc)
                {
                    /* Pack job-defn and send to agent */
                    rc = clTxnCommIfcSessionAppendPayload(
                            commHandle, 
                            &cmd, 
                            pJobDefnStr, 
                            jobDefnLen);
                    rc = clTxnCommIfcSessionRelease(commHandle);
                    if(CL_OK == rc)
                    {
                        ClTxnAgentT     *pFailedAgentInfo = NULL;

                        pFailedAgentInfo = (ClTxnAgentT *) clHeapAllocate(sizeof(ClTxnAgentT));
                        if(pFailedAgentInfo == NULL)
                        {
                            clLogTrace("TXN", "", "Failed to allocate memory.");
                            return CL_TXN_RC(CL_ERR_NO_MEMORY);
                        }

                        memset(pFailedAgentInfo, 0, sizeof(ClTxnAgentT));
                        pFailedAgentInfo->agentJobDefnSize = 0;
                        pFailedAgentInfo->agentJobDefn = NULL;
                        pFailedAgentInfo->failedPhase = CL_TXN_ERR_WAITING_FOR_RESPONSE;
                        pFailedAgentInfo->failedAgentId.jobId = pTxnJob->jobId;
                        pFailedAgentInfo->failedAgentId.agentAddress = pAppComp->appCompAddress;
                        
                        rc = clCntNodeAdd(pFailedTxn->failedAgentList,
                                (ClCntKeyHandleT) &(pFailedAgentInfo->failedAgentId),
                                (ClCntDataHandleT) pFailedAgentInfo, 0);

                        if(rc ==CL_OK)
                        {
                            pFailedTxn->failedAgentCount++;
                            pFailedTxn->failedAgentRespCount++;
                            clLogTrace("CLT", NULL, 
                                    "Response count : [%d]", 
                                    pFailedTxn->failedAgentRespCount);
                        }
                         
                    }
                }
                else
                {
                    clLogError("CLT", NULL, 
                            "Failed to open comm-channel for dest [0x%x:0x%x]", 
                            pAppComp->appCompAddress.nodeAddress, 
                            pAppComp->appCompAddress.portId);
                }
                node = currNode; 
                compRetCode = clCntNextNodeGet( pTxnJob->compList, node, &currNode);
            }
        }

        clHeapFree(pJobDefnStr);
        clBufferDelete (&jobDefn);

        jobNode = jobCurrNode; 
        retCode = clCntNextNodeGet(pTxnDefn->jobList, jobNode, &jobCurrNode);
        clLogTrace("CLT", "RDT", 
                "RETCODE : [0x%x], rc [0x%x] for job [0x%x]", retCode, rc, pTxnJob->jobId.jobId);
    }
    
    if(rc == CL_OK)
    {
        rc = clTxnCommIfcAllSessionClose();
    }
    if(CL_OK != rc)
        clLogError("CLT", NULL,
                "Error in closing the session, rc=[0x%x]", rc);

   
    CL_FUNC_EXIT();
    return (rc);
}

void clTxnAgentJobMsgReceive(
        CL_IN   ClRcT           retCode,
        CL_IN   void            *pCookie,
        CL_IN   ClBufferHandleT  inMsg, 
        CL_OUT  ClBufferHandleT  outMsg)
{
    ClRcT               rc  =   CL_OK;
    ClTxnMessageHeaderT     msgHeader = {{0}};
    ClTxnMessageHeaderIDLT     msgHeadIDL = {{0}};
    ClBufferHandleT  cookieMsg;
    int msgCnt;
    
    cookieMsg = (ClBufferHandleT)pCookie;

    CL_FUNC_ENTER();
    

    clLogTrace("CLT", "ATC",
            "Return value [0x%x]", rc);
    if(retCode == CL_OK)
    {

        rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(outMsg, &msgHeadIDL);
        if(CL_OK != rc)
        {
            clLogError("CLT", NULL, 
                    "Error in unmarshalling message header, rc [0x%x]", rc);
            return;
        }

        _clTxnCopyIDLToMsgHead(&msgHeader, &msgHeadIDL);
        CL_ASSERT(msgHeader.msgCount);

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Received 0x%x messages from agent ", msgHeader.msgCount));
        if(CL_OK == rc)
        clLogTrace("CLT", NULL, 
                "[%d] Reponse(s) received from agent[0x%x:0x%x]", 
                msgHeader.msgCount,
                msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId);

        msgCnt = msgHeader.msgCount;
        
        while ( (CL_OK == rc) && (msgCnt > 0) )
        {
            ClTxnCmdT   tCmd;

            msgCnt--;

            rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(outMsg, &tCmd);
            switch (tCmd.cmd)
            {
                case CL_TXN_CMD_READ_JOB:
                    {
                        ClTxnDefnT          *pTxnDefn;
                        ClTxnDefnT          *pTxnTmpHandle;
                        ClTxnAppJobDefnT    *pTxnAppJob;
                        rc = clTxnStreamTxnDataUnpack(outMsg, &pTxnDefn, &pTxnAppJob);

                        if(rc == CL_OK)
                        {
                            ClTxnFailedTxnT        *pFailedTxn;
                            ClTxnAgentT     *pFailedAgentInfo = NULL;
                            ClTxnAgentIdT   tmpAgentId = {
                                .jobId = pTxnAppJob->jobId,
                                .agentAddress = msgHeader.srcAddr
                            };
                            /* Get the transaction handle using its ID as an index into the activeTxnDb */ 
                            rc = clTxnDbTxnDefnGet(clTxnClientInfo->activeTxnDb, pTxnDefn->clientTxnId, &pTxnTmpHandle);
                            if(rc == CL_OK)
                            {
                                /* Now look for any failed transactions stored under this handle */
                                rc = clCntDataForKeyGet(clTxnClientInfo->failedTxnDb,
                                        (ClCntKeyHandleT) pTxnTmpHandle,
                                        (ClCntDataHandleT *)&pFailedTxn);
                                if(rc == CL_OK)
                                {
                                    if ( CL_OK == (rc = clOsalMutexLock(clTxnClientInfo->txnReadMutex)) )
                                    {
                                        clLogTrace("CLT", 
                                        NULL, 
                                        "Reply received, failed agent response count [%d]", 
                                        pFailedTxn->failedAgentRespCount);
                                        if (pFailedTxn->failedAgentRespCount != 0x0)
                                        {
                                            pFailedTxn->failedAgentRespCount--;
                                        }

                                        rc = clCntDataForKeyGet(pFailedTxn->failedAgentList,
                                                (ClCntKeyHandleT) &tmpAgentId,
                                                (ClCntDataHandleT *)&pFailedAgentInfo);
                                        
                                        if(rc == CL_OK)
                                        {
                                            pFailedAgentInfo->agentJobDefn = 
                                                (ClUint8T *) clHeapAllocate(pTxnAppJob->appJobDefnSize);

                                            if(!pFailedAgentInfo->agentJobDefn)
                                               goto free_buffer; 

                                            memcpy(pFailedAgentInfo->agentJobDefn,
                                                    (ClUint8T*)pTxnAppJob->appJobDefn,
                                                    pTxnAppJob->appJobDefnSize);
                                            pFailedAgentInfo->agentJobDefnSize = pTxnAppJob->appJobDefnSize;
                                            pFailedAgentInfo->failedPhase = tCmd.resp;
                                        }
                                        /* If nobody failed, then we are going to call the transaction completion callback
                                           to notify the user that the TXN is done, and then delete it */
                                        if(pFailedTxn->failedAgentRespCount == 0)
                                        {
                                            if(rc == CL_OK)
                                            {
                                                clLogTrace("CLT", NULL, "Reply received, calling callback");
                                                pFailedTxn->fpTxnAgentCallback(
                                                        (ClTxnTransactionHandleT)(ClWordT)pTxnTmpHandle, CL_OK );
                                                 rc = clTxnJobInfoListDel((ClTxnTransactionHandleT)(ClWordT)pTxnTmpHandle);
                                            }
                                        }
                                        rc = clOsalMutexUnlock(clTxnClientInfo->txnReadMutex);
                                
                                    }
                                }
                            }
                            else
                            {
                                clLogDebug("CLT", NULL,
                                "Delayed messages from agent[0x%x:0x%x]."
                                "Dropping replies\n",
                                msgHeader.srcAddr.nodeAddress, 
                                msgHeader.srcAddr.portId); 
                            }
                        }
                        else
                        {
                                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while unpacking the  messages "));
                        }
                        clTxnDefnDelete(pTxnDefn);
                        clTxnAppJobDelete(pTxnAppJob);
                    }
                    break;
                default:
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid comamnd received 0x%x", tCmd.cmd));
                    rc = CL_ERR_INVALID_PARAMETER;
                    break;
            }
        }
    }
    else
    {
        ClRcT                      rc  =   CL_OK;
        ClTxnFailedTxnT            *pFailedTxn = NULL;
        ClCntNodeHandleT           currNode;
        ClTxnDefnT                 *pTxnDefn = NULL;
        ClTxnMessageHeaderT        cookieMsgHeader = {{0}};
        ClTxnMessageHeaderIDLT     msgHeadIDL = {{0}};

        rc = VDECL_VER(clXdrUnmarshallClTxnMessageHeaderIDLT, 4, 0, 0)(cookieMsg, &msgHeadIDL);
        if(CL_OK != rc)
        {
            clLogError("CLT", NULL, 
                    "Error in unmarshalling message header, rc [0x%x]", rc);
            return;
        }
        _clTxnCopyIDLToMsgHead(&cookieMsgHeader, &msgHeadIDL);
        CL_ASSERT(cookieMsgHeader.msgCount);

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Received [0x%x] messages from agent ", cookieMsgHeader.msgCount));
        clLogTrace("CLT", NULL, 
                "Agent [0x%x:0x%x] did not respond within the timeout",
                cookieMsgHeader.destAddr.nodeAddress, 
                cookieMsgHeader.destAddr.portId); 

        while ( (CL_OK == rc) && (cookieMsgHeader.msgCount > 0) )
        {
            ClTxnCmdT   tCmd;

            cookieMsgHeader.msgCount--;

            rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(cookieMsg, &tCmd);
            switch (tCmd.cmd)
            {
                case CL_TXN_CMD_READ_JOB:

                    rc = clTxnDbTxnDefnGet(clTxnClientInfo->activeTxnDb, tCmd.txnId, &pTxnDefn);
                    if(rc == CL_OK)
                    {
                        rc = clCntDataForKeyGet(clTxnClientInfo->failedTxnDb,
                                (ClCntKeyHandleT) pTxnDefn,
                                (ClCntDataHandleT *)&pFailedTxn);
                        if(rc == CL_OK)
                        {

                            if ( CL_OK == (rc = clOsalMutexLock(clTxnClientInfo->txnReadMutex)) )
                            {
                                if (pFailedTxn->failedAgentRespCount != 0x0)
                                {
                                    pFailedTxn->failedAgentRespCount = 0x0;

                                    if(rc == CL_OK)
                                        rc = clCntFirstNodeGet(pFailedTxn->failedAgentList, &currNode);
                                    while ( CL_OK == rc )
                                    {
                                        ClTxnAgentT     *pFailedAgentInfo = NULL;
                                        ClCntNodeHandleT        node;

                                        /* Retrieve data-node for node */
                                        rc = clCntNodeUserDataGet( pFailedTxn->failedAgentList, currNode, 
                                                (ClCntDataHandleT *)&pFailedAgentInfo);
                                        if(pFailedAgentInfo->failedPhase == CL_TXN_ERR_WAITING_FOR_RESPONSE)
                                        {
                                            ClTxnAppJobDefnT    *pTxnJob = NULL;

                                            pFailedAgentInfo->failedPhase = CL_ERR_TIMEOUT;
                                            rc = clCntDataForKeyGet(pTxnDefn->jobList,
                                                    (ClCntKeyHandleT) &(pFailedAgentInfo->failedAgentId.jobId),
                                                    (ClCntDataHandleT *)&pTxnJob);
                                            pFailedAgentInfo->agentJobDefnSize = pTxnJob->appJobDefnSize;
                                            pFailedAgentInfo->agentJobDefn = (ClUint8T*)clHeapAllocate(pTxnJob->appJobDefnSize);
                                            if(pFailedAgentInfo->agentJobDefn)
                                                memcpy(pFailedAgentInfo->agentJobDefn,(ClUint8T*)pTxnJob->appJobDefn,pTxnJob->appJobDefnSize);
                                        }

                                        node = currNode; 
                                        rc = clCntNextNodeGet( pFailedTxn->failedAgentList, node, &currNode);
                                        clLogTrace("CLT", NULL, "Next node get from failedAgentList");
                                    }

                                    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
                                    {
                                        clLogTrace("CLT", NULL, "Timeout received. Calling callback");
                                        pFailedTxn->fpTxnAgentCallback((ClTxnTransactionHandleT)(ClWordT)pTxnDefn, CL_OK );
                                        rc = clTxnJobInfoListDel((ClTxnTransactionHandleT)(ClWordT)pTxnDefn);
                                    }
                                }
                                rc = clOsalMutexUnlock(clTxnClientInfo->txnReadMutex);
                            }
                        }
                    }
                    else
                    {
                        clLogTrace("CLT", NULL, 
                                "Delayed timer invoke,which cannot be processed ");
                    }

                    break;
                default:
                        break;
            }
        }

    }
    if(rc != CL_OK) 
    {
        clLogError("CLT", NULL, 
                "Error in RMD callback function, rc [0x%x]", rc);
    }

free_buffer:
    clBufferDelete (&outMsg);
    clBufferDelete (&cookieMsg);

    CL_FUNC_EXIT();
}
