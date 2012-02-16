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
 * ModuleName  : cor
 * File        : clCorTxnInterface.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements interface between COR and Transaction Management module
 *****************************************************************************/

#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clOsalApi.h>
#include <clTaskPool.h>
#include <clXdrApi.h>
#include <clCorApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clCorClient.h>
#include <ipi/clRmdIpi.h>

#include <clTxnApi.h>
#include <clTxnAgentApi.h>

#include <clCorTxnInterface.h>
#include <clCorTxnLog.h>
#include <clCorRmDefs.h>
#include <clCorLog.h>
#include <clCorPvt.h>
#include <clCorTreeDefs.h>
#include <xdrClCorObjectHandleIDLT.h>


/* E X T E R N   D E F I N I T I O N */
extern ClRcT _clCorOmClassFromInfoModelGet(ClCorClassTypeT moClass, 
            ClCorServiceIdT svcId, ClOmClassTypeT *pOmClass);

extern ClCorSyncStateT pCorSyncState;
extern _ClCorServerMutexT gCorMutexes;
extern ClCorInitStageT    gCorInitStage;


/* Define (static) global variables */
ClCorTxnConnectionT          *gCorTxnInstance;
ClUint32T                    gCorTxnRequestCount = 0;


/* Internal Functions - Forward declarations */
static ClRcT
clCorTxnJobInfoContainerCreate(ClCntHandleT *pTxnInfoCont);

static ClRcT
clCorTxnJobInfoContainerFinalize(ClCntHandleT corTxnJobInfo);

static ClRcT 
clCorTxnPrepareCallback(
    CL_IN       ClTxnTransactionHandleT     txnHandle,
    CL_IN       ClTxnJobDefnHandleT         jobDefn, 
    CL_IN       ClUint32T                   jobDefnSize,
    CL_INOUT    ClTxnAgentCookieT          *pCookie);

static ClRcT 
clCorTxnCommitCallback(
    CL_IN       ClTxnTransactionHandleT txnHandle,
    CL_IN       ClTxnJobDefnHandleT     jobDefn, 
    CL_IN       ClUint32T               jobDefnSize, 
    CL_INOUT    ClTxnAgentCookieT       *pCookie);

static ClRcT 
clCorTxnRollbackCallback(
    CL_IN       ClTxnTransactionHandleT txnHandle,
    CL_IN       ClTxnJobDefnHandleT     jobDefn, 
    CL_IN       ClUint32T               jobDefnSize, 
    CL_INOUT    ClTxnAgentCookieT       *pCookie);

static ClRcT 
clCorTxnUnpack(
    CL_IN       ClBufferHandleT     msgHandle,
    CL_IN       ClUint32T                  txnJobCount,
    CL_OUT      ClCorTxnJobListT       *pTxnList);

static ClRcT 
clCorTxnPreProcess(
    CL_INOUT    ClCorTxnJobListT       *pTxnList,
    CL_IN       ClBufferHandleT     outMsgHandle,
    CL_OUT      ClCntHandleT            *pContHandle,
    CL_IN       const ClCorAddrPtrT      pCompAddr);

static ClRcT 
clCorTxnTxnJobAdd(
    CL_IN       ClTxnTransactionHandleT     txnHandle,
    CL_IN       ClCorTxnJobListT         *pTxnList, 
    CL_IN       ClCntHandleT             opJobContHdl);

static ClRcT 
clCorTxnEventPublish(
    CL_IN       ClCorTxnJobListT         *pTxnList);

ClRcT 
clCorTxnJobListFree(
    CL_IN       ClCorTxnJobListT         *pTxnList);

ClCorTxnJobListT *
clCorTxnJobListAlloc(ClUint32T count);

static ClRcT clCorTxnProcessFailedTxn(
    CL_IN       ClTxnTransactionHandleT     txnHandle, 
    CL_IN       ClBufferHandleT      outMsgHandle);

static ClRcT 
clCorTxnProcessFailedJobs(
    CL_IN       ClCorTxnJobHeaderNodeT      *pTxnJobHdrNode, 
    CL_IN       ClBufferHandleT      outMsgHandle);

static ClRcT 
clCorTxnMergeJobDefns(
    CL_OUT      ClCorTxnJobHeaderNodeT*     pDestTxnJobHdr, 
    CL_IN       ClCorTxnJobHeaderNodeT*     pSrcTxnJobHdr);
    
static ClRcT 
clCorTxnJobPack(
    CL_IN       ClCntKeyHandleT             key,
    CL_IN       ClCntDataHandleT            data,
    CL_IN       ClCntArgHandleT             userArg,
    CL_IN       ClUint32T                   len);

static ClRcT 
clCorTxnRetrieveJobInfo(
    CL_IN       ClCntHandleT         corTxnJobInfo, 
    CL_IN       ClBufferHandleT      outMsgHandle);

/* Callback functions for transaction-agent */
static ClTxnAgentCallbacksT gCorTxnCallbacks = {
    .fpTxnAgentStart = NULL,
    .fpTxnAgentJobPrepare   = clCorTxnPrepareCallback,
    .fpTxnAgentJobCommit    = clCorTxnCommitCallback,
    .fpTxnAgentJobRollback  = clCorTxnRollbackCallback,
    .fpTxnAgentStop = NULL
};


/**
 * Function to decrement the transaction request 
 * count which was incremented while processing the transation.
 */
static 
void _clCorTxnRequestCountDecrement()
{
    /* Decrement the transaction reqeust counter */
    clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
    gCorTxnRequestCount--;
    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

    return;
}

static ClInt32T 
clCorTxnJobInfoKeyCompFn(
    CL_IN       ClCntKeyHandleT         key1, 
    CL_IN       ClCntKeyHandleT         key2);

static void 
clCorTxnJobInfoDeleteFn(
    CL_IN       ClCntKeyHandleT         key, 
    CL_IN       ClCntDataHandleT        data);


static void 
clCorTxnCompletionCallback (
   CL_IN ClTxnTransactionHandleT txnHandle,
   CL_IN ClRcT                   retCode);

static ClRcT 
clCorTxnOperationalJobsAdd (
    CL_IN ClTxnTransactionHandleT txnHandle,
    CL_IN ClCntHandleT            opJobContHdl);
/**
 * Initializes COR-Transaction Interface for creating new transactions
 * and jobs.
 */
ClRcT clCorTxnInterfaceInit()
{
    ClRcT                   rc = CL_OK;
    ClVersionT              txnVersion = {
                                 .releaseCode    = 'B',
                                 .majorVersion   = 1,
                                 .minorVersion   = 1};
    CL_FUNC_ENTER();

    gCorTxnInstance = (ClCorTxnConnectionT *) clHeapAllocate(sizeof(ClCorTxnConnectionT));
    if (NULL == gCorTxnInstance)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_ERR_NO_MEMORY;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate memory"));
        CL_FUNC_EXIT();
        return (rc);
    }
    memset(gCorTxnInstance, 0, sizeof(ClCorTxnConnectionT));

    /* Initialize transaction-client */
    rc = clTxnClientInitialize(&txnVersion, NULL, &(gCorTxnInstance->corTxnClientHandle));
    if (CL_OK != rc)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
				CL_LOG_MESSAGE_2_INIT,"transaction-client", rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize transaction-client. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = clTxnAgentServiceRegister(CL_COR_INVALID_SRVC_ID, gCorTxnCallbacks, 
                                   &(gCorTxnInstance->corTxnServiceHandle));
    if (CL_OK != rc)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
				CL_LOG_MESSAGE_1_REGISTER_WITH_TRANSACTION_AGENT, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to register COR-Service with transaction-agent. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }


    /* FIXME: Initialize Txn-Log */
     /* TODO - IDL */
#if 0
       rc = clCorTxnLogInit(10);
#endif

    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Initialization done\n"));
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Finalize function for Cor-Txn Interface implementation.
 */
ClRcT clCorTxnInterfaceFini()
{
    ClRcT       rc = CL_OK;
    CL_FUNC_ENTER();
/* TODO - IDL */
#if 0
    clCorTxnLogFini();
#endif

    clTxnAgentServiceUnRegister(gCorTxnInstance->corTxnServiceHandle);

    clTxnClientFinalize(gCorTxnInstance->corTxnClientHandle);

    clHeapFree(gCorTxnInstance);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT VDECL(clCorTransactionConvertOp) (
    CL_IN   ClEoDataT               cData, 
    CL_IN   ClBufferHandleT  inMsgHandle, 
    CL_IN   ClBufferHandleT  outMsgHandle)
{
    ClRcT                    rc = CL_OK;
    ClCorTxnJobListT        *pTxnList = NULL;
    ClUint32T                count = 0;
    ClTxnTransactionHandleT  txnHandle = 0;
	ClVersionT 				 version = {0};
    ClCntHandleT             corTxnJobInfo = 0;
    ClCntHandleT             opAttrCont = 0;
    ClCorAddrT               compAddr = {0};

    CL_FUNC_ENTER();

    /** 
     * Changes for Bug #6055.
     * This snippet of code increments the trasaction job counter which will be 
     * decremented either in the error path or after completion of the txn request.
     * It also restrict the COR server active to take up any transaction request
     * while the synchronisation with the standby cor server is in process. If the txn
     * request counter is greater than zero then the sync-up request would be retried by
     * the slave COR.
     */

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("TXN", "PRE", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    rc = clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
    CL_ASSERT(rc==CL_OK);
    
    if(pCorSyncState == CL_COR_SYNC_STATE_INPROGRESS)
    {
	    clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "COR slave syncup in progress .... retry again ..");
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
	    return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }
    
    /* Incrementing the txn request count.*/
    gCorTxnRequestCount++;

    clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);

     clLog(CL_LOG_SEV_INFO, "TXN", "PRE", "Preparing to start a transaction.");
     
     if((rc = clXdrUnmarshallClVersionT(inMsgHandle, (void *)&version)) != CL_OK)
     {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
         clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", 
            "Failed to unmarshall ClVersionT which is passed to the transaction.  rc [0x%x]", rc);
         CL_FUNC_EXIT();
         return (rc);
     }

	/* Client To Server Version Check */
	clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
	{
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}
     
    /* Get the count first */
    if((rc = clXdrUnmarshallClUint32T(inMsgHandle, (void *)&count)) != CL_OK)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", 
            "Failed to unmarshall ClUint32T which is passed to the transaction.  rc: 0x%x", rc);
        CL_FUNC_EXIT();
        return (rc);
    }

    if(count == 0)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Transaction contains 0 jobs. Unable to initiate the transaction.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS)); 
    }

    pTxnList = clCorTxnJobListAlloc(count);
    if(pTxnList == NULL)
    {
        _clCorTxnRequestCountDecrement();
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    
     /* Unpack the transaction stream into different - txn-jobs */ 
    if((rc = clCorTxnUnpack(inMsgHandle, count, pTxnList)) != CL_OK)
    {
        /* Function to decrement the transaction request count.*/
          _clCorTxnRequestCountDecrement();
          clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to unpack the Txn data. rc [0x%x]", rc);
          clHeapFree(pTxnList);
          CL_FUNC_EXIT();
          return (rc);
    }

    rc = clRmdSourceAddressGet(&compAddr);
    if(CL_OK != rc)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to get the client address. rc[0x%x]", rc);
        clCorTxnJobListFree(pTxnList);
        /* Cannot do anything here. */
        CL_FUNC_EXIT();
        return rc;
    }

    /* Unpack the transaction and update the unfilled moIds/handles. Attribute Types etc */ 
    if((rc =  clCorTxnPreProcess(pTxnList, outMsgHandle, &opAttrCont, &compAddr)) != CL_OK)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        clLogError("TXN", "PRE", "Failed while Pre-validating the COR-TXN job. rc [0x%x]", rc);
        clCorTxnJobListFree(pTxnList);
        CL_FUNC_EXIT();
        return (rc);
    } 

    /** 
     *  Validate weather this transaction has a job which is a part of 
     *  of some other in-progress transaction.
     */
    clOsalMutexLock(gCorMutexes.gCorTxnValidation);
    rc = _clCorTxnJobInprogressValidation(pTxnList);
    if(CL_OK != rc)
    {
        if(CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN) == rc)
            clLogNotice("TXN", "PRE", "One of the transaction job  in the job-list has its \
                    MO under process in some other in-progress transaction. So retrying this transaction. ");
        else
            clLogError("TXN", "PRE", "Failed while doing transaction job inprogress validation. rc[0x%x]", rc);
        clCorTxnJobListFree(pTxnList);
        clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);
        _clCorTxnRequestCountDecrement();
        return rc;
    }
    clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);

    /* Open a new Txn Session */
    rc = clCorTxnTxnSessionOpen(&txnHandle);
    if (CL_OK != rc)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        /* Unset the in-progress flag for the jobs of the transaction job.*/
        clOsalMutexLock(gCorMutexes.gCorTxnValidation);
        _clCorTxnJobInprogressUnset(pTxnList);
        clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);
        /* Failed to open/create transaction */
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to open txn-session. rc [0x%x]", rc);
        clCorTxnJobListFree(pTxnList);
        CL_FUNC_EXIT();
        return (rc);
    }

    clLog(CL_LOG_SEV_INFO, "TXN", "PRE",
            "Created the transaction [%#llX] in the Txn-Manager.",  txnHandle);

    /* Register Jobs with txn-manager */
    rc = clCorTxnTxnJobAdd(txnHandle, pTxnList, opAttrCont);
    
    if (CL_OK != rc)
    {
        /* Function to decrement the transaction request count.*/
        _clCorTxnRequestCountDecrement();
        /* Unset the in-progress flag for the jobs of the transaction job.*/
        clOsalMutexLock(gCorMutexes.gCorTxnValidation);
        _clCorTxnJobInprogressUnset(pTxnList);
        clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to add cor-job to transaction client. rc [0x%x]", rc);
        clCorTxnJobListFree(pTxnList);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Start the transaction (COMMIT) */
    rc = clCorTxnTxnSessionClose(txnHandle);

    if (CL_OK != rc)
    {
        ClRcT txnRc = rc;

        /* Unset the in-progress flag for the jobs of the transaction job.*/
        clOsalMutexLock(gCorMutexes.gCorTxnValidation);
         _clCorTxnJobInprogressUnset(pTxnList);
        clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);

        if(CL_TXN_RC(CL_TXN_ERR_NO_JOBS) == rc)
        {
            clLog(CL_LOG_SEV_TRACE, "TXN", "COM", "No jobs to process in the transaction. rc[0x%x]", rc);
            clCorTxnJobListFree(pTxnList);
            return CL_OK;
        }

        if (CL_TXN_RC(CL_TXN_ERR_TRANSACTION_FAILURE) == rc)
        {
            clLogError("CTI", "COM", "Transaction got failed in txn client. rc [0x%x].", rc);
            goto cleanup;            
        }

        clLog(CL_LOG_SEV_ERROR, "TXN", "COM", "Transaction got failed rc [0x%x]", rc);

        clLog(CL_LOG_SEV_INFO, "TXN", "COM", "Retrieving the failed job information from Txn-Manager.");
                
        /* Create the container to store the failed job information */
        rc = clCorTxnJobInfoContainerCreate(&corTxnJobInfo);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, 
                "TXN", "FJP", "Failed to create container to store failed job information. rc [0x%x]", rc);
            goto cleanup;            
        }

        rc = clCorTxnProcessFailedTxn(txnHandle, corTxnJobInfo);
        if ( rc != CL_OK ) 
        {            
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP", "Error while processing the failed txn. rc [0x%x]", rc);
            clCorTxnJobInfoContainerFinalize(corTxnJobInfo);
            goto cleanup;
        }
    
        rc = clCorTxnRetrieveJobInfo(corTxnJobInfo, outMsgHandle);
        if ( rc != CL_OK ) 
        {            
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP", 
                    "Error while retrieving the failed jobs from the container. rc [0x%x]", rc);
            clCorTxnJobInfoContainerFinalize(corTxnJobInfo);
            goto cleanup;
       }
    
        rc = clCorTxnJobInfoContainerFinalize(corTxnJobInfo);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FPJ", 
                    "Failed to remove the failed job information from the container. rc [0x%x]", rc);
            goto cleanup;            
        }

        rc = txnRc;
        goto cleanup;       
    }

    clLog(CL_LOG_SEV_INFO, "TXN", "COM",
            "Committed the transaction [%#llX] successfully",  txnHandle);

    clOsalMutexLock(gCorMutexes.gCorTxnValidation);
    _clCorTxnJobInprogressUnset(pTxnList);
    clOsalMutexUnlock(gCorMutexes.gCorTxnValidation);

    /* Publish the events for this transaction */
    if((rc = clCorTxnEventPublish(pTxnList)) != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "EVT", "Failed to publish event for the transaction. rc [0x%x]", rc);
        goto cleanup;
    }

    clLog(CL_LOG_SEV_INFO, "TXN", "COM",
            "All the events are published successfully for the transaction [%#llX]", txnHandle);

    /* If everything got pass */
 cleanup:    
    clTxnJobInfoListDel(txnHandle);
    clCorTxnJobListFree(pTxnList);
    
    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorTxnProcessFailedTxn(ClTxnTransactionHandleT txnHandle, ClCntHandleT corTxnJobInfo)
{

    ClTxnAgentRespT agentResp = {0};
    ClCntNodeHandleT previous = 0;
    ClCntNodeHandleT next = 0;
    ClRcT rc = CL_OK;                
    ClBufferHandleT msgHdl;
    ClCorTxnJobHeaderNodeT *pTxnJobHdrNode;
                    
    /* Get the failed jobs from Transaction Manager */

    rc = clTxnJobInfoGet(txnHandle,
                previous,
                &next,
                &agentResp);
                    
    if (rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            /* Unable to get the first failed job from transaction manager */
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP", 
                "Not able to get the first failed job from Txn Manager for txn [%#llX]. rc [0x%x]", 
                txnHandle, rc);
            CL_FUNC_EXIT();
            return (CL_OK);
        }
        
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Error while getting the failed jobs from Txn Manager for txn [%#llX]. rc:[0x%x]\n", txnHandle, rc);
        CL_FUNC_EXIT();
        return (rc);
    }
    
    while ((agentResp.agentJobDefn != 0) && (agentResp.agentJobDefnSize != 0))
    {
        if ((rc = clBufferCreate(&msgHdl)) != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "Error while creating the buffer message. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return rc;
        }
        
        if ((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *) agentResp.agentJobDefn, agentResp.agentJobDefnSize)) != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "Error while writing into the buffer message. rc [0x%x]", rc);
            clBufferDelete(&msgHdl);
            CL_FUNC_EXIT();
            return rc;
        }

        if ((rc = clCorTxnJobStreamUnpack(msgHdl, &pTxnJobHdrNode)) != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "Error while unpacking the job stream. rc [0x%x]", rc);
            clBufferDelete(&msgHdl);
            CL_FUNC_EXIT();
            return rc;
        }
                   
        /* Remove the message handle */
        clBufferDelete(&msgHdl);
                    
        if (pTxnJobHdrNode->jobHdr.jobStatus == CL_COR_TXN_JOB_FAIL)
        {
            /* Resetting the jobStatus of the job-header. */
            pTxnJobHdrNode->jobHdr.jobStatus = CL_OK;

            /* Process the failed jobs */
            rc = clCorTxnProcessFailedJobs(pTxnJobHdrNode, corTxnJobInfo);
            if (rc != CL_OK)
            {
                clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                    "\nError while processing the failed jobs. rc [0x%x]", rc);
                clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
                CL_FUNC_EXIT();
                return (rc);
            }
        }
        else 
            clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        
        previous = next;
        next = 0;

        rc = clTxnJobInfoGet(txnHandle,
                                    previous,
                                    &next,
                                    &agentResp);
                    
        if (rc != CL_OK)
        {
            if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
            {
                clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                    "Error while getting the failed jobs from Txn Manager for txn [%#llX]. rc [0x%x]", 
                    txnHandle, rc);
                CL_FUNC_EXIT();
                return (rc);
            }

            CL_FUNC_EXIT();
            return (CL_OK);
        }
    }
    
    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorTxnRetrieveJobInfo(ClCntHandleT corTxnJobInfo, ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T count = 0;

    CL_FUNC_ENTER();
   
    clBufferClear(outMsgHandle);

    if((rc = clCntSizeGet(corTxnJobInfo, &(count))) != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Invalid container handle specified to read the failed job information. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Marshal count into outMsgHandle */
    if((rc = clXdrMarshallClUint32T((void *)&count,outMsgHandle, 0)) != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP", 
            "Failed to marshall ClUint32T. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return (rc);
    }
                                
    rc = clCntWalk(corTxnJobInfo, clCorTxnJobPack,
                        (ClCntArgHandleT) outMsgHandle, sizeof(ClBufferHandleT));
    if (CL_OK != rc)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Failed to include all failed transaction-jobs in msg-buffer. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return (rc);
    }
    
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clCorTxnJobPack(
                CL_IN   ClCntKeyHandleT     key,
                CL_IN   ClCntDataHandleT    data,
                CL_IN   ClCntArgHandleT     userArg,
                CL_IN   ClUint32T           len)
{
    ClRcT rc = CL_OK;
    ClCorTxnJobHeaderNodeT* pCorTxnJobHdr;
    ClBufferHandleT outMsgHandle;
    
    pCorTxnJobHdr = (ClCorTxnJobHeaderNodeT *) data;
    outMsgHandle = (ClBufferHandleT) userArg;

    /* Pack the job */
    rc = clCorTxnJobStreamPack(pCorTxnJobHdr, outMsgHandle);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to pack the job. rc [0x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    
    CL_FUNC_EXIT();
    return rc;
}
            
ClRcT clCorTxnProcessFailedJobs(ClCorTxnJobHeaderNodeT *pTxnJobHdrNode, ClCntHandleT corTxnJobInfo)
{
    ClRcT rc = CL_OK;
    ClCorTxnJobHeaderNodeT* pTempTxnJobHdr = NULL;
    ClCorMOIdPtrT moId = NULL;

    CL_FUNC_ENTER();

    if (pTxnJobHdrNode == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP", 
            "No jobs found. Header is NULL. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }
    
    rc = clCorMoIdAlloc(&moId);
    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Failed to allocate memory for MoId. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    
    memcpy(moId, &pTxnJobHdrNode->jobHdr.moId, sizeof(ClCorMOIdT));
    
    rc = clCntDataForKeyGet(corTxnJobInfo, (ClCntKeyHandleT) moId, (ClCntDataHandleT *) &pTempTxnJobHdr);

    if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",   
            "The container handle to retrieve the failed job information is invalid. rc [0x%x]", rc);
        clCorMoIdFree(moId);
        CL_FUNC_EXIT();
        return rc;
    }

    if (pTempTxnJobHdr == NULL)
    {
        /* If the key (moId) does not exist */
        rc = clCntNodeAdd(corTxnJobInfo, (ClCntKeyHandleT) moId, (ClCntDataHandleT) pTxnJobHdrNode, NULL);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "Unable to add the failed job node into the container. rc [0x%x]", rc);
            clCorMoIdFree(moId);
            CL_FUNC_EXIT();
            return rc;
        }
    }
    else
    {
        /* If the key (moId) already exists */
        /* PTempTxnJobHdr will point to the existing job definition in the container */
        /* Need to compare both and set the status */
        rc = clCorTxnMergeJobDefns(pTempTxnJobHdr, pTxnJobHdrNode);
        if (rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "Unable to merge two failed job definitions. rc [0x%x]", rc);
            clCorMoIdFree(moId);
            CL_FUNC_EXIT();
            return rc;
        }

        /* Delete the moId and the new job. Since it is not added into container */
        clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        clCorMoIdFree(moId);
    }   

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clCorTxnMergeJobDefns(ClCorTxnJobHeaderNodeT* pDestTxnJobHdr, ClCorTxnJobHeaderNodeT* pSrcTxnJobHdr)
{
    ClRcT rc = CL_OK;
    ClCorTxnObjJobNodeT* pTxnObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT* pTxnAttrSetJobNode = NULL;
    ClUint32T cnta=0, cntb=0, jobId=0, attrSetJobId=0;
    ClCorTxnJobIdT jobSubJobId = 0;

    CL_FUNC_ENTER();
    
    if (clCorMoIdCompare(&pDestTxnJobHdr->jobHdr.moId, &pSrcTxnJobHdr->jobHdr.moId) == 0)
    {
        /* If both the jobs are same */
        cnta = pSrcTxnJobHdr->jobHdr.numJobs;
        
        if ((pTxnObjJobNode = pSrcTxnJobHdr->head) == NULL)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                "No jobs found to merge. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
        }

        jobId = 0;

        while ((cnta--) && pTxnObjJobNode)
        {
            jobId++;

            if (pTxnObjJobNode->job.jobStatus != CL_OK)
            {
                if (pTxnObjJobNode->job.op != CL_COR_OP_SET)
                {
                    jobSubJobId = CL_COR_TXN_FORM_JOB_ID(jobId, 0);

                    clCorTxnJobStatusSet((ClCorTxnIdT) pDestTxnJobHdr, jobSubJobId, pTxnObjJobNode->job.jobStatus);
                    pDestTxnJobHdr->jobHdr.jobStatus = CL_OK;
                }
                else /* If it contains sub jobs */
                {
                    cntb = pTxnObjJobNode->job.numJobs;

                    if (((pTxnAttrSetJobNode =  pTxnObjJobNode->head) == NULL) || (cntb == 0))
                    {
                        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
                            "AttrSet sub job not found. rc [0x%x]", rc);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
                    }

                    attrSetJobId = 0;

                    while ((cntb--) && pTxnAttrSetJobNode)
                    {
                        attrSetJobId++;
                
                        if (pTxnAttrSetJobNode->job.jobStatus != CL_OK)
                        {
                            jobSubJobId = CL_COR_TXN_FORM_JOB_ID(jobId, attrSetJobId);

                            clCorTxnJobStatusSet((ClCorTxnIdT) pDestTxnJobHdr, jobSubJobId, pTxnAttrSetJobNode->job.jobStatus);
                            pDestTxnJobHdr->jobHdr.jobStatus = CL_OK;
                        }

                        pTxnAttrSetJobNode = pTxnAttrSetJobNode->next;
                    }
                }
            }

            pTxnObjJobNode = pTxnObjJobNode->next;
        }
    }
    else
    {
        /* both the jobs are not same, cannot merge */
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Both the jobs passed to merge are not same. Failed to merge the job definitions");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }
    
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Key compare callback function for the bundle container.
 */


ClInt32T 
clCorBundleJobContKeyCompFn(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return clCorMoIdSortCompare((ClCorMOIdPtrT) key1, (ClCorMOIdPtrT) key2);

#if 0
    ClCorMOIdT* pMoId1 = NULL;
    ClCorMOIdT* pMoId2 = NULL;

    pMoId1 = (ClCorMOIdT *) key1;
    pMoId2 = (ClCorMOIdT *) key2;

    if(clCorMoIdCompare(pMoId1, pMoId2) == 0)
        return 0;
    else
        return 1;
#endif
}

/**
 * Delete callback for the bundle container.
 */

void
clCorBundleJobContDeleteFn(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    clCorMoIdFree((ClCorMOIdPtrT) key);
    clCorTxnJobHeaderNodeDelete((ClCorTxnJobHeaderNodeT *) data);

#if 0
    ClCorTxnJobHeaderNodeT  *jobHdr = NULL;
    ClCorMOIdT* pMoId = NULL;

    pMoId = (ClCorMOIdT *) key;
    jobHdr = (ClCorTxnJobHeaderNodeT *) data;

    clCorMoIdFree(pMoId);
    clCorTxnJobHeaderNodeDelete(jobHdr); /* Delete the whole job information */
#endif
}

ClRcT clCorBundleJobsContainerCreate(ClCntHandleT *pBundleInfoCont)
{
    ClRcT       rc = CL_OK;

    rc = clCntRbtreeCreate ( clCorBundleJobContKeyCompFn,
                            clCorBundleJobContDeleteFn,
                            NULL,
                            CL_CNT_UNIQUE_KEY,
                            pBundleInfoCont);
#if 0
    rc = clCntLlistCreate ( clCorBundleJobContKeyCompFn,
                            clCorBundleJobContDeleteFn,
                            NULL,
                            CL_CNT_UNIQUE_KEY,
                            pBundleInfoCont);
#endif                       
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("gCorTxnJobInfo container creation failed. rc [0x%x]", rc));
        return rc;
    }
    
    return rc;
}




/* Linked list table generation */

ClRcT clCorTxnJobInfoContainerCreate(ClCntHandleT* pCorTxnJobInfo)
{
    ClRcT rc = CL_OK;

    rc = clCntRbtreeCreate(clCorTxnJobInfoKeyCompFn,
                            clCorTxnJobInfoDeleteFn,
                            NULL,
                            CL_CNT_UNIQUE_KEY,
                            pCorTxnJobInfo);
#if 0
    rc = clCntLlistCreate(clCorTxnJobInfoKeyCompFn,
                            clCorTxnJobInfoDeleteFn,
                            NULL,
                            CL_CNT_UNIQUE_KEY,
                            pCorTxnJobInfo);
#endif
                            
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("gCorTxnJobInfo container creation failed. rc [0x%x]", rc));
        return rc;
    }
    
    return rc;
}

ClInt32T clCorTxnJobInfoKeyCompFn(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return clCorMoIdSortCompare((ClCorMOIdPtrT) key1, (ClCorMOIdPtrT) key2);
#if 0
    ClCorMOIdPtrT moId1, moId2;
    
    moId1 = (ClCorMOIdPtrT) key1;
    moId2 = (ClCorMOIdPtrT) key2;

    if (clCorMoIdCompare(moId1, moId2) == 0)
    {
        return 0;
    }
    else 
    {
        return 1;
    }

    return 1;
#endif
}

void clCorTxnJobInfoDeleteFn(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    clCorMoIdFree((ClCorMOIdPtrT) key);
    clCorTxnJobHeaderNodeDelete((ClCorTxnJobHeaderNodeT *) data);

#if 0
    ClCorMOIdPtrT moId;
    ClCorTxnJobHeaderNodeT* jobHdr;

    moId = (ClCorMOIdPtrT) key;
    jobHdr = (ClCorTxnJobHeaderNodeT *) data;

    clCorMoIdFree(moId); /* Delete the moId */
    clCorTxnJobHeaderNodeDelete(jobHdr); /* Delete the whole job information */
#endif
}

ClRcT clCorBundleJobsContainerFinalize(ClCntHandleT bundleInfoCont)
{
    ClRcT   rc = CL_OK;

    if(bundleInfoCont != 0)
        rc = clCorTxnJobInfoContainerFinalize(bundleInfoCont);
    
    return rc;
}



ClRcT clCorTxnJobInfoContainerFinalize(ClCntHandleT corTxnJobInfo)
{
    ClRcT rc = CL_OK;

    rc = clCntAllNodesDelete((ClCntHandleT) corTxnJobInfo);

    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Unable to delete all the nodes of the failed job container. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntDelete((ClCntHandleT) corTxnJobInfo);

    if (rc != CL_OK)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "FJP",
            "Unable to delete the failed job info container. rc [0x%x]", rc);
        return rc;
    }

    return rc;
}

/* =========== Transaction-Client Internal Functions =========== */

ClRcT clCorTxnTxnSessionOpen(
    CL_IN       ClTxnTransactionHandleT     *pTxnHandle)
{
    ClRcT                       rc          = CL_OK;
    CL_FUNC_ENTER();

    /* Create a new transaction and copy the 
       returned transaction handle as session-handle 
     */
    rc = clTxnTransactionCreate(0, pTxnHandle);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorTxnTxnSessionClose(
    CL_IN       ClTxnTransactionHandleT   txnHandle)
{
    ClRcT                       rc          = CL_OK;

    CL_FUNC_ENTER();

    clTaskPoolMonitorDisable();

    rc = clTxnTransactionStart(txnHandle);

    clTaskPoolMonitorEnable();

    /* Function to decrement the transaction request count.*/
    _clCorTxnRequestCountDecrement();

    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Function to start the bundle for get operation.
 */

ClRcT clCorGetBundleStart(
    CL_IN       ClTxnTransactionHandleT   txnHandle)
{
    ClRcT                       rc          = CL_OK;

    CL_FUNC_ENTER();

    rc = clTxnReadAgentJobsAsync(txnHandle, clCorTxnCompletionCallback);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to start the get operation. rc[0x%x]", rc));
        return rc;
    }

    CL_FUNC_EXIT();
    return (rc);
}



ClRcT clCorTxnTxnSessionCancel(
    CL_IN       ClTxnTransactionHandleT   txnHandle)
{
    ClRcT               rc = CL_OK;

    CL_FUNC_ENTER();

    rc = clTxnTransactionCancel(txnHandle);

        CL_FUNC_EXIT();
        return (rc);
    }

/**
 *
 * While committing an active transaction, all operations on a object (MO)
 * would be converted into a single job.
 */


static ClRcT 
clCorTxnTxnJobAdd(
    CL_IN       ClTxnTransactionHandleT     txnHandle,
    CL_IN       ClCorTxnJobListT         *pTxnList,
    CL_IN       ClCntHandleT              opJobContHdl)
{

    ClRcT           rc     = CL_OK;
    ClUint32T       i      = 0;
    ClCorCommInfoT          *pCompList = NULL;
    ClTxnJobHandleT          jobHandle = 0;
    ClUint32T                compCnt = 0;
    ClBufferHandleT          msgHdl = 0;
    ClUint32T                length = 0;
    ClUint8T                *pData =  NULL;
    ClCharT                 moIdStr[CL_MAX_NAME_LENGTH];
 
    CL_FUNC_ENTER();

    if(!pTxnList)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
            "NULL argument passed for pTxnList");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    while (i < pTxnList->count)
    {
         if(CL_COR_TXN_JOB_HDR_DELETED == pTxnList->pJobHdrNode[i]->jobHdr.isDeleted )
         {
             i++;
             continue ;
         }

         if((rc = clBufferCreate(&msgHdl)) != CL_OK)
         {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Buffer Message Creation Failed. rc: 0x%x", rc);
            CL_FUNC_EXIT();
            return (rc);
         }

         if((rc = clCorTxnJobStreamPack(pTxnList->pJobHdrNode[i], msgHdl)) != CL_OK)
         {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Cor-Txn Job Stream pack Failed. rc: 0x%x", rc);
            CL_FUNC_EXIT();
            return (rc);
         }

         if((rc = clBufferLengthGet(msgHdl, &length)) != CL_OK)
         {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Could not get buffer message length. rc: 0x%x", rc);
            CL_FUNC_EXIT();
            return (rc);
         }


         if((rc = clBufferFlatten(msgHdl, &pData)) != CL_OK)
         {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Buffer Message Flattening Failed. rc: 0x%x", rc);
            CL_FUNC_EXIT();
            return (rc);
         }
       
         if(pData)
         {
             rc = clTxnJobAdd(txnHandle, 
                       (ClTxnJobDefnHandleT)pData, 
                          length, 
                            CL_COR_TXN_SERVICE_ID_WRITE,     /* FIXME: Check for proper usage of service-id */
                              &jobHandle);
             if (CL_OK != rc)
             {
                clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                    "Failed to register new job in txn-manager. rc [0x%x]", rc);
                break;
             }
         }
         clHeapFree(pData);
         rc = clBufferDelete(&msgHdl);

        /* Set the participating components for this job */
         pCompList = (ClCorCommInfoT *) clHeapAllocate( (sizeof(ClCorCommInfoT) * 
                                                  CL_COR_TXN_MAX_COMPONENTS) );
         if (NULL == pCompList)
         {
              rc = CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
              clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
              CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MEM_ALLOC_FAIL));
              CL_FUNC_EXIT();
              break;
          }
          memset(pCompList, 0, (sizeof(ClCorCommInfoT) * CL_COR_TXN_MAX_COMPONENTS));

          compCnt = CL_COR_TXN_MAX_COMPONENTS;
          rc = rmRouteGet(&(pTxnList->pJobHdrNode[i]->jobHdr.moId), pCompList, &compCnt);
          if (CL_OK != rc)
          {
              clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Failed to look-up routes for given object (%s). rc [0x%x]", 
                _clCorMoIdStrGet(&(pTxnList->pJobHdrNode[i]->jobHdr.moId), moIdStr), rc);
              clHeapFree(pCompList);
              break;
           }
  
          while (compCnt > 0)
          {
                 ClIocAddressT   compAddress;
                 memcpy(&(compAddress.iocPhyAddress),  &(pCompList[compCnt - 1].addr), 
                        sizeof(ClIocPhysicalAddressT));

                 clLog(CL_LOG_SEV_INFO, "TXN", "PRE",
                    "Adding the component [node:0x%x portId:0x%x] as OI for txn-job [%p]",
                    compAddress.iocPhyAddress.nodeAddress, 
                    compAddress.iocPhyAddress.portId,
                    (ClPtrT) jobHandle);

                 rc = clTxnComponentSet(txnHandle, jobHandle, compAddress, CL_TXN_COMPONENT_CFG_2_PC_CAPABLE);
                 if (CL_OK != rc)
                 {
	                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
	                                 CL_LOG_MESSAGE_1_SET_COMP_ADDRESS, rc);
                    clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                        "Failed to add component [node:0x%x;portId:0x%x] as OI for job [%p]. rc [0x%x]",
                        compAddress.iocPhyAddress.nodeAddress,
                        compAddress.iocPhyAddress.portId,
                        (ClPtrT) jobHandle,
                        rc);
                    break;
                 }
                 
                compCnt--;
          }
          
        clHeapFree(pCompList);

        /* increment the job count. */
        ++i;
    } /* while (i <= pTxnList->count) */


    /* Only if there are set jobs of operation attributes*/
    if(opJobContHdl != 0)
    {
        rc = clCorTxnOperationalJobsAdd(txnHandle, opJobContHdl);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Failed while adding the jobs");
        }
        
        rc = clCorTxnJobInfoContainerFinalize(opJobContHdl);
        if(rc != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE",
                "Failed while finalizing the operational attribute job container. rc[0x%x]", rc);
            return rc;
        }
    }
    
    CL_FUNC_EXIT();
    return (rc);
}


/**
 *  Add the Operational jobs. 
 */

ClRcT clCorTxnOperationalJobsAdd(CL_IN ClTxnTransactionHandleT txnHandle,
                                 CL_IN ClCntHandleT            opJobContHdl)
{
    ClRcT                       rc          = CL_OK;
    ClCorAddrT                  addr            = {0};
    ClTxnJobHandleT             jobHandle       = 0;
    ClBufferHandleT             msgHdl          = 0;
    ClUint32T                   length          = 0;
    ClUint8T                    *pData          = NULL;
    ClUint32T                   jobCount        , i = 0;
    ClCntNodeHandleT            nodeHandle      = 0;
    ClCorTxnJobHeaderNodeT      *pRuntimeJobHdr = NULL;
    ClIocAddressT               compAddress     = {{0}};


    if(opJobContHdl == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* Add the operational attribute jobs if any */
    if((rc = clCntSizeGet(opJobContHdl, &jobCount)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the container. rc[0x%x]", rc));
        return rc;
    }

    if(jobCount == 0)
    {
        /* nothing to be done. return ok.*/
        return CL_OK;
    }


    if((rc = clCntFirstNodeGet(opJobContHdl, &nodeHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to find the first node of container. rc[0x%x]",rc));
        return rc;
    }
     
    while((nodeHandle != 0) && (i < jobCount))
    {
        if((rc = clCntNodeUserDataGet(opJobContHdl, nodeHandle, (ClCntDataHandleT *)&pRuntimeJobHdr)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the user data. rc[0x%x]", rc));
            return rc;
        }

        if((rc = clBufferCreate(&msgHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed. rc: 0x%x", rc));
            CL_FUNC_EXIT();
            return (rc);
        }
       
        /* Get the participating components for this job */
        memset(&compAddress, 0, sizeof(ClCorAddrT));

        rc = _clCorPrimaryOIGet (&(pRuntimeJobHdr->jobHdr.moId), &addr);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to look-up routes for given object. rc:0x%x\n", rc));
            clBufferDelete(&msgHdl);
            goto nextAttrJob;
        }
  
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Setting component node:0x%x portId:0x%x\n", 
                           addr.nodeAddress, 
                                addr.portId));

        if((rc = clCorTxnJobStreamPack(pRuntimeJobHdr, msgHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Cor-Txn Job Stream Unpack Failed. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(opJobContHdl))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while finalizing the container."));
            CL_FUNC_EXIT();
            return (rc);
        }

        if((rc = clBufferLengthGet(msgHdl, &length)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Could not get buffer message length. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(opJobContHdl))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
            CL_FUNC_EXIT();
            return (rc);
        }

        pData = NULL;
        if((rc = clBufferFlatten(msgHdl, &pData)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Flattening Failed. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(opJobContHdl))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
            clBufferDelete(&msgHdl);
            CL_FUNC_EXIT();
            return (rc);
        }

        if(pData)
        {
            rc = clTxnJobAdd ( txnHandle, 
                              (ClTxnJobDefnHandleT)pData, 
                              length, 
                              CL_COR_TXN_SERVICE_ID_WRITE,     /* Service Id for 2-pc */
                              &jobHandle);
            if (CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to register new job in txn-manager. rc:0x%x\n", rc));
                goto nextAttrJob;
            }
        }
        clHeapFree(pData); 
        rc = clBufferDelete(&msgHdl);

        memcpy(&compAddress, &addr, sizeof(ClCorAddrT));

        rc = clTxnComponentSet(txnHandle, jobHandle, compAddress, CL_TXN_COMPONENT_CFG_2_PC_CAPABLE);
        if (CL_OK != rc)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                         CL_LOG_MESSAGE_1_SET_COMP_ADDRESS, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set component-address for this job. rc:0x%x\n", rc));
        }

nextAttrJob:

        if((rc = clCntNextNodeGet(opJobContHdl, nodeHandle, &nodeHandle)) != CL_OK)
        {
            if(CL_ERR_NOT_EXIST == rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the "));
                if(CL_OK !=  clCorBundleJobsContainerFinalize(opJobContHdl))
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
                return rc;
            }
            return CL_OK;
        }
        /* increment the job count. */
        ++i;
    }  
    
    /* Free the container. */
    if(CL_OK !=  clCorBundleJobsContainerFinalize(opJobContHdl))
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));

    return rc;
}
static ClRcT 
clCorTxnUnpack(
    CL_IN       ClBufferHandleT     msgHandle,
    CL_IN       ClUint32T                  txnJobCount,
    CL_OUT      ClCorTxnJobListT       *pTxnList)
{
    ClRcT           rc     = CL_OK;
    ClUint32T       i      = 0;
 
    CL_FUNC_ENTER();

    if(pTxnList == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "NULL argument passed.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    pTxnList->count = txnJobCount;
    while (i < txnJobCount)
    {
      /* Unpack the stream. */
        if((rc = clCorTxnJobStreamUnpack(msgHandle, &(pTxnList->pJobHdrNode[i]))) != CL_OK)
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "Failed to unpack the txn JobSteam. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return (rc);
        }
      /* increment the count */
        ++i;
    }
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Preprocess the transaction data. 
 *
 * The functionalities are
 * ==> Updates the empty object Handles and MoId (keys ) in each ClCorTxnJobStreamT.      
 * ==> Transaction job validation.
 * ==> Updates the attribute Type information for SET jobs.                               N
 * ==> Does the endian conversion for the payload(value ) and                             N
 *     also corrects the size of payload (if user has passed more size).
 * ==> Writes the returned object Handle (for CREATE request) to buffer.                  
 */

static ClRcT 
clCorTxnPreProcess( CL_INOUT    ClCorTxnJobListT    *pTxnList,
                    CL_IN       ClBufferHandleT     outMsgHandle,
                    CL_OUT      ClCntHandleT        *pCntHandle,
                    CL_IN       const ClCorAddrPtrT  pCompAddr )
{

    ClRcT           rc     = CL_OK;
    ClUint32T       i      = 0;
    ClCorClassTypeT classId = 0;
    ClCorMOIdT      moId  ;
    ClCorMOClassPathT moClsPath = {{0}};
    ClUint32T count = 0;
 
    CL_FUNC_ENTER();

    if(pTxnList == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "PRE", "NULL argument passed in pTxnList.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    while (i < pTxnList->count)
    {
       /* Fill the unfilled information and also format the value in Network order. */
       
       /* If the job is failing in the Regularization phase, write down the dummy object handle
        * into the outMsgHandle and Marshall the failed information.
        */

        /* TODO : Need to add failed job information for CREATE / DELETE also. */
        if((rc = clCorTxnRegularize(pTxnList, i, pCntHandle, pCompAddr)) != CL_OK) 
        {
            ClRcT tmpRc = CL_OK;
           
            /* Store the rc value locally to return to the caller */
            tmpRc = rc;
            
            clLogError( "TXN", "PRE", " Failed while prevalidating the cor-txn job. rc[0x%x]", rc);
            
            clLogTrace("TXN", "PRE", "Packing the failed job information which can be retrieved at client.");

            clBufferClear(outMsgHandle);

            count = 1;
            
            /* Marshal count into outMsgHandle */
            if((rc = clXdrMarshallClUint32T((void *)&count, outMsgHandle, 0)) != CL_OK)
            {
                clLogError("TXN", "PRE", "Failed to pack the job count in the buffer. rc [0x%x]", rc);
                CL_FUNC_EXIT();
                return (tmpRc);
            }
        
            rc = clCorTxnJobStreamPack(pTxnList->pJobHdrNode[i], outMsgHandle);
            if (rc != CL_OK)
            {
                clLogError("TXN", "PRE", "Failed to pack the job in the buffer. rc [0x%x]", rc);
                CL_FUNC_EXIT();
                return tmpRc;
            }
            
            CL_FUNC_EXIT();
            /* Return the value got from the Regularization function */
            return (tmpRc);
        }
    
        /* If rc == CL_OK */
        /* Write the object handle to outBuff, if the operation is CREATE */
        if((pTxnList->pJobHdrNode[i]->jobHdr.isDeleted != CL_COR_TXN_JOB_HDR_DELETED))
        {
            if ((pTxnList->pJobHdrNode[i]->head->job.op == CL_COR_OP_CREATE) || 
                (pTxnList->pJobHdrNode[i]->head->job.op == CL_COR_OP_CREATE_AND_SET))
            {
                VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIdl = {0};

                rc = clCorMoIdToObjectHandleGet(&(pTxnList->pJobHdrNode[i]->jobHdr.moId), 
                        &(objHIdl.oh));
                if (rc != CL_OK)
                {
                    clLogError("TXN", "PRE", "Failed to get Object Handle from MoId. rc [0x%x]", rc);
                    return rc;
                }

                rc = clCorObjectHandleSizeGet(objHIdl.oh, &(objHIdl.ohSize));
                if (rc != CL_OK)
                {
                    clLogError("TXN", "PRE", "Failed to get Object Handle size. rc [0x%x]", rc);
                    return rc;
                }

                rc = VDECL_VER(clXdrMarshallClCorObjectHandleIDLT, 4, 1, 0)((void *) &objHIdl, outMsgHandle, 0);
                if (rc != CL_OK)
                {
                    clLogError("TXN", "PRE", "Failed to marshall ClCorObjectHandleIDLT_4_1_0. rc [0x%x]", rc);
                    return rc;
                }

                clCorObjectHandleFree(&(objHIdl.oh));
            }

            clCorMoIdInitialize(&moId);
            moId = pTxnList->pJobHdrNode[i]->jobHdr.moId;
            if(moId.svcId != CL_COR_INVALID_SVC_ID)
            {
                clCorMoIdServiceSet(&moId, CL_COR_INVALID_SVC_ID);

                clCorMoIdToMoClassPathGet(&moId, &moClsPath);
                if ((rc = corMOTreeClassGet (&moClsPath, moId.svcId, &classId)) != CL_OK)
                {
                    clLogError( "TXN", "PRE", "Could'nt get the classId for the supplied \
                            moId/svcId combination. rc[0x%x]", rc);
                    CL_FUNC_EXIT();
                    return (rc);
                }

                clCorMoIdServiceSet(&moId, pTxnList->pJobHdrNode[i]->jobHdr.moId.svcId);

                rc = _clCorOmClassFromInfoModelGet(classId, 
                        moId.svcId, &pTxnList->pJobHdrNode[i]->jobHdr.omClassId);
                if(CL_OK != rc)
                {
                    clLogTrace("TXN", "PRE", "The OM class Id information is not present in COR for \
                        the MSO class[0x%x]. It might not have been configured \
                        in the ASP model. rc[0x%x], ", classId, rc);
                    pTxnList->pJobHdrNode[i]->jobHdr.omClassId = 0;
                    rc = CL_OK;
                }
            }
        }
        else
        {
            clDbgPause(); /* GAS temporary */
        }

        /* increment count */
         ++i;
    }
    
    CL_FUNC_EXIT();
    return (rc);
}

ClCorTxnJobListT *
clCorTxnJobListAlloc(ClUint32T count)
{
    ClUint32T size =  sizeof(ClCorTxnJobListT) + ((count-1)*sizeof(ClCorTxnJobHeaderNodeT *)) + sizeof(ClUint32T);
    CL_ASSERT(count>0);
    
    ClCorTxnJobListT * pTxnList = (ClCorTxnJobListT *)clHeapAllocate(size);
    
    if(pTxnList == NULL)
    {
        clLog(CL_LOG_SEV_TRACE, "TXN", "PRE", "Failed to allocate memory");
        return NULL;
    }
    memset(pTxnList, 0, size);
    *((ClUint32T*)(((char*)pTxnList)+size-sizeof(ClUint32T))) = ClCorTxnJobListStructId;  /* Add a guard */
    pTxnList->structId = ClCorTxnJobListStructId;
    pTxnList->count = count;
    return pTxnList;
}


/* Function to free the contents of pTxnList*/
ClRcT
clCorTxnJobListFree(
    CL_IN       ClCorTxnJobListT         *pTxnList)
{
    CL_ASSERT(pTxnList);
    
    ClUint32T   count = 0;
    ClUint32T size =  sizeof(ClCorTxnJobListT) + ((pTxnList->count-1)*sizeof(ClCorTxnJobHeaderNodeT *)) + sizeof(ClUint32T);
 
    CL_ASSERT(pTxnList->structId == ClCorTxnJobListStructId);
    CL_ASSERT(*((ClUint32T*)(((char*)pTxnList)+size-sizeof(ClUint32T))) == ClCorTxnJobListStructId);  /* check the guard */

   /* Free all the txn jobs. */
    count = 0;
    while (count < pTxnList->count)
    {
       clCorTxnJobHeaderNodeDelete(pTxnList->pJobHdrNode[count]);
        ++count;
    }
    
    CL_ASSERT(*((ClUint32T*)(((char*)pTxnList)+size-sizeof(ClUint32T))) == ClCorTxnJobListStructId);  /* check the guard */

    /* Clear before freeing to cause problems if read-after-free */
    memset(pTxnList,0xa,size);    
    clHeapFree(pTxnList);
  return (CL_OK);
}

static ClRcT clCorTxnEventPublish(
              CL_IN       ClCorTxnJobListT    *pTxnList)
{

    ClRcT           rc     = CL_OK;
    ClUint32T       i      = 0;
 
    CL_FUNC_ENTER();

    while (i < pTxnList->count)
    
    {
       if((rc = clCorTxnJobEventPublish(pTxnList->pJobHdrNode[i])) != CL_OK)
       {
             clLog(CL_LOG_SEV_ERROR, "TXN", "EVT",
                "Failed to publish the event for a jobHeader, rc [0x%x]", rc);
              CL_FUNC_EXIT();
             return (rc);
       }
      /* increment the count */
        ++i;
    }

    CL_FUNC_EXIT();
    return (rc);
}


/* =========== Transaction-Agent Callback Functions =========== */
ClRcT clCorTxnPrepareCallback(
        CL_IN       ClTxnTransactionHandleT     txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize,
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT               rc          = CL_OK;

   /* We dont need to validate now.. since we have already validated.
      May be need to put Locks.
     */
    return (rc);
}

/**
 * Transaction Agent callback to commit current transaction
 */
ClRcT clCorTxnCommitCallback(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT                    rc = CL_OK;
    ClBufferHandleT   msgHdl;
    ClCorTxnJobHeaderNodeT  *pTxnJobHdrNode;  

    CL_FUNC_ENTER();

    if((rc = clBufferCreate(&msgHdl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed. rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }
 
    if((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *)jobDefn, jobDefnSize)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Write Failed. rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    if((rc = clCorTxnJobStreamUnpack(msgHdl, &pTxnJobHdrNode)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Read Failed. rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    clBufferDelete(&msgHdl);

    clTaskPoolMonitorDisable();

    /* Now process the stream. */
    if((rc = clCorTxnProcessTxn(pTxnJobHdrNode)) != CL_OK)
    {
        clTaskPoolMonitorEnable();
        clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n cor-txn Update Falied !!!!! rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    clTaskPoolMonitorEnable();

    clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Transaction Agent callback to rollback current transaction.
 */
ClRcT clCorTxnRollbackCallback(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT       rc = CL_OK;
    CL_FUNC_ENTER();

    /* FIXME: COR to implement or call rollback for this job */
    rc = CL_OK;

    CL_FUNC_EXIT();
    return (rc);
}


/* EO callback function for the get bundle. */
ClRcT
VDECL(_clCorBundleOp) (CL_IN ClEoDataT data,
                 CL_IN ClBufferHandleT inMsgHandle,
                 CL_OUT ClBufferHandleT outMsgHandle)
{
    ClRcT                        rc          = CL_OK;
    ClVersionT                   version     = {0};
    ClCntHandleT                 rtAttrCont  = 0;
    ClCorBundleDataT             bundleData = {0};
    ClCorTxnJobListT             *pTxnList   = NULL;
    ClTxnTransactionHandleT      txnHandle   = 0;
    ClUint32T                    count       = 0, rtJobCount = 0;
    
    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("BUN", "EXP", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    rc = clXdrUnmarshallClVersionT(inMsgHandle, (void *)&version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while unmarshalling the version information. rc[0x%x]", rc));
        /* Cannot do any thing here. */
        return rc;
    }
       
    rc = clXdrUnmarshallClHandleT(inMsgHandle, (void *) &bundleData.txnId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while unmarshalling the client's bundle handle. rc[0x%x]", rc));
        /* Cannot do anything here */
        CL_FUNC_EXIT();
        return rc;
    }
 
    clLogTrace("BUN", "REQ", "Got the request for the bundle handle [%#llX]", bundleData.txnId);

    /* Getting the rmd's defer handle*/
    rc = clRmdResponseDefer(&bundleData.clientAddr);
    if(CL_OK != rc)
    {
        clLogError("BUN", "GET", 
                "Failed while getting the rmd defer handle for bundle handle[%#llX]. rc[0x%x]",
                bundleData.txnId, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Storing the message handle to be used while sending the response back to client.*/
    bundleData.outMsgHandle = outMsgHandle;

    /* Get the count first */
    if((rc = clXdrUnmarshallClUint32T(inMsgHandle, (void *)&count)) != CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to unmarshall ClUint32T  rc: 0x%x", rc));
         clCorBundleResponseSend(bundleData); 
         CL_FUNC_EXIT();
         return (rc);
    }

    if(0 == count)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ZERO Transaction Jobs . \n"));
        clCorBundleResponseSend(bundleData); 
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_ZERO_JOBS_BUNDLE)); 
    }

    /* Client To Server Version Check */
    clCorClientToServerVersionValidate(version, rc);
    if(rc != CL_OK)
    {
        clCorBundleResponseSend(bundleData); 
        return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
    }
 
    pTxnList = clCorTxnJobListAlloc(count);
    if(NULL == pTxnList)
    {
        clCorBundleResponseSend(bundleData); 
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    
    /* Unpack the transaction stream into different - txn-jobs */ 
    if((rc = clCorTxnUnpack(inMsgHandle, count, pTxnList)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to unpack the Txn data. rc: 0x%x", rc));
        clCorBundleResponseSend(bundleData); 
        clHeapFree(pTxnList);
        CL_FUNC_EXIT();
        return (rc);
    }

    if((rc = clCorTxnPreProcess(pTxnList, outMsgHandle, NULL, NULL)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while preprocessing the get jobs. rc[0x%x]", rc));
        clCorBundleResponseSend(bundleData); 
        clCorTxnJobListFree(pTxnList);
        return rc;
    }

    /* Assigning the job Type */
    bundleData.jobType = CL_COR_JOB_TYPE_READ;

    /* Open a new Txn Session */
    if((rc = clCorTxnTxnSessionOpen(&txnHandle)) != CL_OK)
    {
        /* Failed to open/create transaction */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to open txn-bundle. rc:0x%x \n", rc));
        clCorBundleResponseSend(bundleData); 
        clCorTxnJobListFree(pTxnList);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* This function will saparate the config and the runtime attributes into different containers */ 
    rc = clCorBundleDataStore(pTxnList, txnHandle, &bundleData, &rtAttrCont);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while storing the bundle Data. rc[0x%x]", rc));
        clCorBundleResponseSend(bundleData); 
        clCorTxnJobListFree(pTxnList);
        clCorBundleDataNodeDelete(txnHandle);
        return rc;
    }

    if(rtAttrCont == 0)
    {
       rtJobCount = 0;
    }
    else
    {
        rc = clCntSizeGet(rtAttrCont, &rtJobCount);
        if(rc != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_TRACE,("No runtime Job in this request. "));
           rtJobCount = 0;
        }
    }

    /* If there are any runtime attr jobs then only start the transaction */
    if(rtJobCount > 0)
    {
        clLogTrace("BUN", "GET", "It contains runtime jobs [%d]", rtJobCount);

        /* Register Jobs with txn-manager */
        rc = clCorBundleTxnJobAdd(txnHandle, rtAttrCont, bundleData);
        if (CL_OK != rc)
        {
            /* Send the config and failed jobs only. */
            clCorBundleResponseSend(bundleData);
            clCorBundleDataNodeDelete(txnHandle);
            clCorBundleJobsContainerFinalize(rtAttrCont);
            clCorTxnJobListFree(pTxnList);
            clLogError("BUN", "GET", "Failed to add cor-job in transaction manager. rc:0x%x\n", rc);
            CL_FUNC_EXIT();
            return (rc);
        }

        /* Start the bundle */
        rc = clCorGetBundleStart(txnHandle);
        if(CL_OK != rc)
        {
            /* Send the config and failed jobs only. */
            clCorBundleResponseSend(bundleData);
            clCorBundleDataNodeDelete(txnHandle);
            clCorBundleJobsContainerFinalize(rtAttrCont);
            clCorTxnJobListFree(pTxnList);
            clLogError("BUN", "GET", "Failed to start the transaction. rc[0x%x]", rc);
            return rc;
        }

        clLogTrace("BUN", "GET", "Started the bundle asyn transaction with txn handle [%#llX]", txnHandle);

        /* Remove all the runtime attribute jobs from container and delete it.*/
        if(CL_OK !=  clCorBundleJobsContainerFinalize(rtAttrCont))
            clLogError("BUN", "GET", "Failed while clearing the runtime job container. ");
    } 
    else
    {
        clLogTrace("BUN", "GET", "Sending only configuration attribute jobs. ");

        /* Contains only config attributes, send them to client.*/
        rc = clCorBundleResponseSend(bundleData);
        if(CL_OK != rc)
            clLogError("BUN", "GET", "Failed while sending only the configuration attr values. rc[0x%x]", rc);
            
        /* Remove the node specific to this bundle from the container */
        rc = clCorBundleDataNodeDelete(txnHandle);
        if(CL_OK != rc)
            clLogError("BUN", "GET", "Failed while Deleting the bundle data node. rc[0x%x]",rc);

        if(CL_OK !=  clCorBundleJobsContainerFinalize(rtAttrCont))
            clLogError("BUN", "GET", "Failed while clearing the runtime job container. ");

        /* Txn client function to delete the txn handle deletion. */
        clTxnJobInfoListDel(txnHandle);
    }
    
    clCorTxnJobListFree(pTxnList);
    return rc;
}


/**
 * The transaction client's callback function which will be called once transaction client 
 * is done with processing of the trasanction. 
 */
void 
clCorTxnCompletionCallback (CL_IN ClTxnTransactionHandleT txnHandle,
                            CL_IN ClRcT                   retCode)
{
    ClRcT                   rc              = CL_OK;
    ClCorBundleDataPtrT     pBundleData    = NULL;    
    ClTxnAgentRespT         agentResp       = {0};
    ClCntNodeHandleT        previous        = 0;
    ClCntNodeHandleT        next            = 0;
    ClCorTxnJobHeaderNodeT  *pTxnJobHdrNode = NULL;
    ClBufferHandleT         msgHdl          = 0;

    CL_FUNC_ENTER();
    
    /* Get the bundle data for this transaction */
    if((rc = clCorBundleDataGet(txnHandle, &pBundleData) ) != CL_OK)
    {
        /* Can not send response as unable to find bundle data. */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the bundle data. Undefined state !!!! . rc[0x%x]",rc));
        return ;
    }

    if(NULL == pBundleData )
    {
        /* Can not send response as bundle data found is NULL. */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,(" bundle Data is NULL. Undefined state. !!!!  . rc[0x%x]", CL_COR_SET_RC(CL_COR_ERR_NULL_PTR)));
        return ;
    }
                    
    /* Get the job from transaction client. */
    rc = clTxnJobInfoGet(txnHandle,
                previous,
                &next,
                &agentResp);
                    
    if (CL_OK != rc)
    {
        /* Unable to get the first failed job from transaction manager */
        if ( CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc) )
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nUnable to get failed job from Transaction. rc [0x%x]\n", rc));
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nUnable to get the failed job. rc:[0x%x]\n", rc));
        goto sendBundleResponse;
    }
    
    while ((agentResp.agentJobDefn != 0) && (agentResp.agentJobDefnSize != 0))
    {
        if ((rc = clBufferCreate(&msgHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while creating the buffer message. rc:[0x%x]\n", rc));
            goto sendBundleResponse;
        }
        
        if ((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *) agentResp.agentJobDefn, agentResp.agentJobDefnSize)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while writing into the buffer message. rc:[0x%x]\n", rc));
            clBufferDelete(&msgHdl);
            goto sendBundleResponse;
        }
        
        /* Unpack the job. */
        if ((rc = clCorTxnJobStreamUnpack(msgHdl, &pTxnJobHdrNode)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while unpacking the stream. rc:[0x%x]\n", rc));
            clBufferDelete(&msgHdl);
            goto sendBundleResponse;
        }
                   
        /* Remove the message handle */
        clBufferDelete(&msgHdl);
                    
        /* Aggregate all the jobs in the job container */
        if(agentResp.failedPhase != 0)
        {
            /* Set a generic return code for all the job whose job status is not set. */
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_TIMED_OUT);

            rc = clCorBundleInvalidJobAdd(*pBundleData, pTxnJobHdrNode, rc);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nError while storing the jobs. rc [0x%x]\n", rc));
                clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
                goto sendBundleResponse;
            }
        }
        else /* If get on this job was success. */
        {
            rc = clCorBundleJobAggregate(*pBundleData, pTxnJobHdrNode);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nError while storing the jobs. rc [0x%x]\n", rc));
                clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
                goto sendBundleResponse;
            }
        }

        /* Done with the job. delete it */
        rc = clCorTxnJobHeaderNodeDelete(pTxnJobHdrNode);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the txn job. rc[0x%x]", rc));
            goto sendBundleResponse;
        }

        previous = next;
        next = 0;
        
        /* get the next job. */
        rc = clTxnJobInfoGet (txnHandle,
                             previous,
                             &next,
                             &agentResp);
                    
        if (CL_OK != rc)
        {
            if ( CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while getting the failed jobs from transaction manager. rc:[0x%x]\n", rc));
            }
            else
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("No more failed jobs. rc[0x%x]", rc));
            goto sendBundleResponse;
        }
    }
    
sendBundleResponse:


    /* Send the response to the server */
    if((rc = clCorBundleResponseSend(*pBundleData)) != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while sending the reponse to the client. rc[0x%x]", rc));
    
    /* Remove the node specific to this bundle from the container */
    if((rc = clCorBundleDataNodeDelete(txnHandle) != CL_OK))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while Deleting the bundle data node. rc[0x%x]",rc));
    }

    CL_FUNC_EXIT();
    return;
}
