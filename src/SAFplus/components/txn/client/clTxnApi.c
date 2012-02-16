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
 * File        : clTxnApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This is the client API implementation used to access transaction 
 * management services.
 *
 *
 *****************************************************************************/

#undef __SERVER__
#define __CLIENT__
#include <string.h>

#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clVersionApi.h>

#include <clTxnApi.h>
#include <clTxnErrors.h>
#include <clTxnDb.h>

#include "clTxnClientIpi.h"
#include <clTxnServerFuncTable.h>



/****************************************************************************
                    Global Variable Definitions
 ***************************************************************************/

ClTxnClientInfoT    *clTxnClientInfo = NULL;

/**
 * List of supported version (application client)
 */
ClVersionT   clTxnClientVersionSupported[] = {
    { 'B', 1, 1 }
};

/**
 * Version Database
 */
ClVersionDatabaseT clTxnClientVersionDb = {
    sizeof(clTxnClientVersionSupported) / sizeof(ClVersionT), 
    clTxnClientVersionSupported
};


ClRcT clTxnActiveDbWalk( 
        ClUint32T argc, 
        ClCharT *argv[], 
        ClCharT **retStr );

static ClDebugFuncEntryT _clTxnClientDbgCliFuncList[] = {
    {clTxnActiveDbWalk, "showActiveDb", "show all the entry of the activeDb"},
};
static char*    gTxnStatusStr[] = {
    "",
    "CL_TXN_TRANSACTION_PRE_INIT",
    "CL_TXN_TRANSACTION_ACTIVE",
    "CL_TXN_TRANSACTION_PREPARING",
    "CL_TXN_TRANSACTION_PREPARED",
    "CL_TXN_TRANSACTION_COMMITTING",
    "CL_TXN_TRANSACTION_COMMITTED",
    "CL_TXN_TRANSACTION_MARKED_ROLLBACK",
    "CL_TXN_TRANSACTION_ROLLING_BACK",
    "CL_TXN_TRANSACTION_ROLLED_BACK",
    "CL_TXN_TRANSACTION_RESTORED",
    "CL_TXN_TRANSACTION_UNKNOWN",
};
/**
 * This is Handle-compare function used by container for storing active-txn 
 */
static ClInt32T _clTxnTxnHandleCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

void _clTxnFailedTxnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

void _clTxnFailedTxnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

ClRcT _clTxnWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen);

/**
 * This function registers debug cli for txn client 
 */
ClRcT 
clTxnClientDebugCliRegister(CL_OUT ClHandleT *pDbgHandle );

/**
 * This function unregisters debug cli for txn client 
 */
ClRcT 
clTxnClientDebugCliUnregister();

/**
 * Initialize client library for transaction management.
 *
 * Application need to call this API in order to initialize the library 
 * before starting any transaction. This API needs version and callback
 * function handlers.
 *
 *
 */
ClRcT clTxnClientInitialize(
        CL_IN   ClVersionT                              *pVersion, 
        CL_IN   ClTxnTransactionCompletionCallbackT     fpTxnCallback,
        CL_OUT  ClTxnClientHandleT                      *pTxnHandle)
{
    ClRcT           rc = CL_OK;

    /*
       This client library initialization must do following
       - Allocate memory for keeping current transaction sessions
       - Initialize packing/unpacking routines required for streaming jobs
       - Communicate with Transaction Manager (well-known comm-port address) &
         register for new client
       - Update the EO context's private data for transaction client with
         details of callback function (TODO)
    */
    CL_FUNC_ENTER();

    if ( NULL != clTxnClientInfo )
    {
        rc = CL_ERR_INITIALIZED;
        goto error;
    }

    if ( (NULL == pVersion) || (NULL == pTxnHandle) )
    {
        rc = CL_ERR_NULL_POINTER;
        goto error;
    }
    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, TXN), CL_IOC_TXN_PORT);
    if(CL_OK != rc)
    {
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clVersionVerify (&clTxnClientVersionDb, pVersion);
    if (CL_OK != rc)
        goto error;

    clTxnClientInfo = (ClTxnClientInfoT *) clHeapAllocate(sizeof(ClTxnClientInfoT));
    if (NULL == clTxnClientInfo)
    {
        rc = CL_ERR_NO_MEMORY;
        goto error;
    }

    clTxnClientInfo->fpTxnCompletionCallback = (ClTxnTransactionCompletionCallbackT)fpTxnCallback; 

    clTxnClientInfo->txnIdCounter = 0x0U;
    if (clOsalMutexCreate ( &(clTxnClientInfo->txnMutex) ) )
    {
        /* TO free clTxnClientInfo memory allocated */
        clHeapFree(clTxnClientInfo);
        clTxnClientInfo = NULL;
        rc = CL_ERR_UNSPECIFIED;
    }

    rc = clOsalMutexCreate ( &(clTxnClientInfo->txnReadMutex) ) ;

    if (CL_OK == rc)
    {
        rc = clOsalCondCreate(&(clTxnClientInfo->txnClientCondVar));
    }

    if (CL_OK == rc)
    {
        rc = clTxnDbInit( &(clTxnClientInfo->activeTxnDb) );
    

    }
    if (CL_OK == rc)
    {
        rc = clCntLlistCreate( _clTxnTxnHandleCompare, 
                                _clTxnFailedTxnDelete, _clTxnFailedTxnDestroy, 
                                CL_CNT_UNIQUE_KEY, 
                                &(clTxnClientInfo->failedTxnDb));
    }
 
    if (CL_OK == rc)
    {
        rc = clTxnCommIfcInit(&(clTxnClientVersionSupported[0]));
    }


    clTxnClientDebugCliRegister ( &(clTxnClientInfo->dbgHandle) );
error:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to initialize transaction-service rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to finalize transaction client-library
 */
ClRcT clTxnClientFinalize(
        CL_IN ClTxnClientHandleT txnClientHandle)
{
    ClRcT               rc = CL_OK;
    ClEoExecutionObjT   *pEOObj;

    CL_FUNC_ENTER();

    if (clTxnClientInfo != NULL)
    {
        /* FIXME: Release all allocated memory */
        /* FIXME: Make a note not to receive further requests */

        clOsalMutexDelete(clTxnClientInfo->txnMutex);
        clOsalMutexDelete(clTxnClientInfo->txnReadMutex);

        clOsalCondDelete (clTxnClientInfo->txnClientCondVar);
        clTxnClientDebugCliUnregister();

        clTxnDbFini (clTxnClientInfo->activeTxnDb);

        clCntDelete(clTxnClientInfo->failedTxnDb);
        
        clTxnCommIfcFini();

        clHeapFree(clTxnClientInfo);
        if (clEoMyEoObjectGet(&pEOObj) == CL_OK)
        {
            clEoClientUninstall(pEOObj, CL_TXN_CLIENT_TABLE_ID);
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to create a new transaction.
 */
ClRcT clTxnTransactionCreate(
        CL_IN   ClTxnConfigMaskT        txnConfig, 
        CL_OUT  ClTxnTransactionHandleT *pHandle)
{
    ClRcT           rc                  = CL_OK;
    ClTxnDefnT      *pNewTxn;
    /*
       Creating transaction involves preparing storage area for
       - transaction description
       - list of job descriptions
       - transaction configuration
       - List of components per job involved in the transaction completion
    */
    CL_FUNC_ENTER();

    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    if (NULL == pHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    rc = clTxnNewTxnDfnCreate(&pNewTxn);
    
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate/create new transaction definition. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return CL_TXN_RC(rc);
    }

    if ( (rc = clOsalMutexLock(clTxnClientInfo->txnMutex)) == CL_OK )
    {
        pNewTxn->clientTxnId.txnId = clTxnClientInfo->txnIdCounter++;
        pNewTxn->clientTxnId.txnMgrNodeAddress = clIocLocalAddressGet();    /* Client Address itself */
        if (clOsalMutexUnlock(clTxnClientInfo->txnMutex) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock\n"));
            rc = CL_ERR_UNSPECIFIED;
            goto error;
        }
    }

#if 0
    /* Invoke txn-manager to create new txn-session */
    rc = clTxnSessionCreate(clTxnClientInfo->txnServerIdlHandle, &(pNewTxn->txnId));
    if (CL_OK != rc)
        goto error;
#endif

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("New transaction %p created txn-id 0x%x:0x%x\n", (ClPtrT)pNewTxn, 
                    pNewTxn->clientTxnId.txnMgrNodeAddress, pNewTxn->clientTxnId.txnId));


    /* Instead of pointer to txnId, make handle from the transaction-defn itself */
    *pHandle = (ClTxnTransactionHandleT)(ClWordT) pNewTxn;
    pNewTxn->currentState = CL_TXN_STATE_PRE_INIT;
    pNewTxn->txnCfg = txnConfig;

    rc = clTxnDbNewTxnDefnAdd(clTxnClientInfo->activeTxnDb, pNewTxn, 
                              &(pNewTxn->clientTxnId));

error:
    /* Do necessary clean-up */
    if (CL_OK != rc)
    {
        clTxnDefnDelete(pNewTxn);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create new transaction-session[0x%x:0x%x], rc [0x%x]\n", 
                    pNewTxn->clientTxnId.txnMgrNodeAddress,
                     pNewTxn->clientTxnId.txnId,
                    rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return rc;
}

/**
 * API to start a fully defined transaction
 */
ClRcT clTxnTransactionStart(
        CL_IN ClTxnTransactionHandleT tHandle)
{
    ClRcT                   rc          = CL_OK;
    ClTxnDefnT              *pTxnDefn   = NULL;
    ClCntNodeHandleT        currNode;

    CL_FUNC_ENTER();

    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnDefn = (ClTxnDefnT *)(ClWordT)tHandle;
    if ( NULL == pTxnDefn )
    {
        rc = CL_ERR_NULL_POINTER;
        clLogError("CLT", NULL,
                "Transaction handle passed is invalid");
        goto error;
    }
    if(pTxnDefn->jobCount == 0x0)
    {
        rc = CL_TXN_RC(CL_TXN_ERR_NO_JOBS);
        goto error;
    }
    
    rc = clCntFirstNodeGet(pTxnDefn->jobList, &currNode);

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClTxnAppJobDefnT    *pTxnJobDefn = NULL;

        rc = clCntNodeUserDataGet(pTxnDefn->jobList, currNode, 
                                  (ClCntDataHandleT *)&pTxnJobDefn);
        
        if (pTxnJobDefn->compCount == 0x0)
        {
            rc = CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS);
            break;
        }

        tNode = currNode;
        rc = clCntNextNodeGet(pTxnDefn->jobList, tNode, &currNode);
    }
    
    if (rc == CL_TXN_RC(CL_TXN_ERR_NO_JOBS) || rc == CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS))
    {
        goto error;
    }

    clLogTrace("CLT", NULL, "Running txn [0x%x:0x%x]", 
            pTxnDefn->clientTxnId.txnMgrNodeAddress,
            pTxnDefn->clientTxnId.txnId);
    rc = clTxnTransactionRunSync(pTxnDefn);
    if (CL_OK != rc)
    {
       clLogError("CLT", NULL,
               "Error while processing transaction[0x%x:0x%x],  rc[0x%x]", 
               pTxnDefn->clientTxnId.txnMgrNodeAddress,
               pTxnDefn->clientTxnId.txnId,
               rc);
        rc = CL_TXN_RC(rc);
    }
    
error:
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to cancel an active transaction
 */
ClRcT clTxnTransactionCancel(
        CL_IN ClTxnTransactionHandleT txnHandle)
{
    ClRcT                   rc          = CL_OK;
    ClTxnDefnT              *pTxnDefn   = NULL;

    CL_FUNC_ENTER();

    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnDefn = (ClTxnDefnT *)(ClWordT)txnHandle;
    if (NULL == pTxnDefn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguement"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    if (pTxnDefn->currentState == CL_TXN_STATE_ACTIVE)
    {
        /* FIXME: Send a request to txn-processor to cancel this txn */
    }

    /* Do local clean-up */
    if (clTxnTransactionFinalize(pTxnDefn) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to clean-up transaction-session at client."));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to cancel the transaction. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to asynchronously start a fully transaction.
 */
ClRcT clTxnTransactionStartAsync(
        CL_IN ClTxnTransactionHandleT tHandle)
{
    ClRcT                   rc          = CL_OK;
    ClTxnDefnT              *pTxnDefn;
    ClCntNodeHandleT        currNode;
    /*
       This is a non-blocking call to run the transaction in the background.
       The processing stages involved is same as blocking version (mentioned above)
       but it would be make Async-RMD with reply to the transaction manager.
       The callback function would be of transaction client library, which
       eventually makes callback to application client
       
       Validation Required:
       - Check if application has supplied callback function handler. If it is not
         available, then async call should return error
    */
    CL_FUNC_ENTER();

    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    if (NULL == clTxnClientInfo->fpTxnCompletionCallback)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Callback function missing."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_INVALID_PARAMETER);
    }

    pTxnDefn = (ClTxnDefnT *) (ClWordT)tHandle;
    if (NULL == pTxnDefn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction-handle or null-pointer"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    if(pTxnDefn->jobCount == 0x0)
    {
        rc = CL_TXN_RC(CL_TXN_ERR_NO_JOBS);
        goto error;
    }

    rc = clCntFirstNodeGet(pTxnDefn->jobList, &currNode);

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClTxnAppJobDefnT    *pTxnJobDefn = NULL;

        rc = clCntNodeUserDataGet(pTxnDefn->jobList, currNode, 
                                  (ClCntDataHandleT *)&pTxnJobDefn);
        if (pTxnJobDefn->compCount == 0x0)
        {
            rc =  CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS);
            break;
        }

        tNode = currNode;
        rc = clCntNextNodeGet(pTxnDefn->jobList, tNode, &currNode);
    }
 
    if (rc == CL_TXN_RC(CL_TXN_ERR_NO_JOBS) || rc == CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS))
    {
        goto error;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Running txn:%p\n", (ClPtrT) pTxnDefn));
    rc = clTxnTransactionRunAsync(pTxnDefn);

error:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to queue transaction for processing. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * API to add a new job to an existance transaction.
 */
ClRcT clTxnJobAdd(
        CL_IN ClTxnTransactionHandleT tHandle, 
        CL_IN ClTxnJobDefnHandleT     jobDefn, 
        CL_IN ClUint32T               jobDefnSize, 
        CL_IN ClInt32T                serviceType,
        CL_OUT ClTxnJobHandleT        *pJobHandle)
{
    ClRcT rc = CL_OK;
    ClTxnAppJobDefnT    *pNewTxnJob = NULL;
    ClTxnDefnT          *pTxnDefn;
    ClUint8T            *pAppJobDefn;

    CL_FUNC_ENTER();
    /*
       This API updates the transaction identified by tHandle with new job definition.
       It involves following steps -
       - Access transaction related information from EO private data
       - Access current transaction-details along with already updated job-details.
       - Update the new job at the end of queue.
       - Assign an identification to the new job and return the same.
    */
    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnDefn = (ClTxnDefnT *) (ClWordT)tHandle;
    pAppJobDefn = (ClUint8T *) jobDefn;

    if ( (NULL == pTxnDefn) || (NULL == pAppJobDefn) || (NULL == pJobHandle) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguements"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    /* (Future) FIXME: There is no way in this scheme to identify duplication of job */

    rc = clTxnNewAppJobCreate(&pNewTxnJob);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create txn-job in database. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
        goto error_2;
    }

    /* FIXME: check if this is required!!!
       memcpy(&(pNewTxnJob->jobId.txnId), &(pTxnDefn->clientTxnId), sizeof(ClTxnTransactionIdT) ); 
     */
    /* Acquire lock */
    if ( (rc = clOsalMutexLock (clTxnClientInfo->txnMutex)) == CL_OK)
    {
        if (pTxnDefn->currentState == CL_TXN_STATE_PRE_INIT)
        {
            pNewTxnJob->jobId.jobId = pTxnDefn->jobCount;
            rc = clTxnNewAppJobAdd(pTxnDefn, pNewTxnJob);
            if(CL_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid state of the transaction\n"));
            rc = CL_ERR_INVALID_STATE;
        }

        /* Release lock */
        clOsalMutexUnlock (clTxnClientInfo->txnMutex);
    }

    if (CL_OK != rc)
        goto error_1;

    pNewTxnJob->appJobDefn = clHeapAllocate(jobDefnSize);
    if (0 == pNewTxnJob->appJobDefn) /* Check for malloc success */
    {
        /* FIXME: This has serious error as, new job-defn gets deleted, 
           while txn maintains stale pointer
        */
        rc = CL_ERR_NO_MEMORY;
        goto error_1;
    }

    memcpy ( (ClUint8T *)pNewTxnJob->appJobDefn, pAppJobDefn, jobDefnSize);
    pNewTxnJob->appJobDefnSize = jobDefnSize;
    pNewTxnJob->serviceType = serviceType;

    /* RC2 Change: Job-handle is same as pointer to newly created job */
    *pJobHandle = (ClTxnJobHandleT) pNewTxnJob;

error_1:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to add new job to transaction rc:0x%x\n", rc));
        clTxnAppJobDelete(pNewTxnJob);
        rc = CL_TXN_RC(rc);
    }

error_2:
    CL_FUNC_EXIT();
    return rc;
}

/**
 * API to delete/remove an existing job
 */
ClRcT clTxnJobRemove(
        CL_IN ClTxnTransactionHandleT tHandle, 
        CL_IN ClTxnJobHandleT jobHandle)
{
    ClRcT rc = CL_OK;
    ClTxnDefnT              *pTxnDefn;
    ClTxnAppJobDefnT        *pTxnJobDefn;

    CL_FUNC_ENTER();
    /*
       This API updates the transaction identified by tHandle by removing specified job.
       It invovlves following steps -
       - Access transaction related information from EO private data
       - Access current transaction-details along with job-list
       - Remove the jon from the list and return appropriately.
    */
    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnDefn = (ClTxnDefnT *) (ClWordT)tHandle;
    pTxnJobDefn = (ClTxnAppJobDefnT *) jobHandle;
    if ( (NULL == pTxnDefn) || (NULL == pTxnJobDefn) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguements"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    if ( (rc = clOsalMutexLock(clTxnClientInfo->txnMutex)) != CL_OK)
        goto error;

    /* Check the currents status of this transaction */
    if (pTxnDefn->currentState == CL_TXN_STATE_PRE_INIT)
    {
        /* Check if this job definition is part of the given transaction or not */ 
        /* Delete locally */ 
        /* FIXME: This has serious error, as same job-id gets assigned to two 
           different jobs of same txn
        */
        clTxnAppJobRemove(pTxnDefn, pTxnJobDefn->jobId); 
        /* This is required here as clTxnAppJobRemove calls clTxnAppJobDelete internally in the callback function */
        /*clTxnAppJobDelete(pTxnJobDefn);*/
    }
    else
        rc = CL_ERR_INVALID_STATE;

    if (clOsalMutexUnlock (clTxnClientInfo->txnMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock on this transaction-client.\n"));
    }


error:
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the transacion-job. rc:0x%x\n", rc));
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}


/**
 * API to add new component for a transaction-job. This component would be involved
 * in transaction in all phases to complete the intended operation.
 * It is expected that this component has transaction-agent integrated and provides
 * necessary functionality (behavior) appropriate for Two-Phase Commit Protocol.
 */
ClRcT clTxnComponentSet(
        CL_IN ClTxnTransactionHandleT   txnHandle,
        CL_IN ClTxnJobHandleT           tJobHandle, 
        CL_IN ClIocAddressT             txnCompAddress, 
        CL_IN ClTxnCompConfigMaskT      configMask)
{
    ClRcT rc = CL_OK;
    ClTxnDefnT              *pTxnDefn;
    ClTxnAppJobDefnT        *pTxnJobDefn;

    CL_FUNC_ENTER();
    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnJobDefn = (ClTxnAppJobDefnT *)tJobHandle;
    pTxnDefn    = (ClTxnDefnT *) (ClWordT)txnHandle;

    if ( (NULL == pTxnDefn) || (NULL == pTxnJobDefn) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null arguments\n"));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }


    /* FIXME: Validate/Check for transaction-handle and job-handle match */

    /* Acquire lock here */
    if (clOsalMutexLock (clTxnClientInfo->txnMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to acquire lock. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return CL_TXN_RC(rc);
    }

    /* Check the currents status of this transaction */
    if (pTxnDefn->currentState == CL_TXN_STATE_PRE_INIT) 
    {
        switch(configMask)
        {
            case CL_TXN_COMPONENT_CFG_1_PC_CAPABLE:
                {
                    if((pTxnJobDefn->jobCfg & CL_TXN_COMPONENT_CFG_2_PC_CAPABLE) ||
                       (pTxnDefn->txnCfg & (CL_TXN_COMPONENT_CFG_2_PC_CAPABLE << 0x8)))
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Both single and 2P components cannot"
                                 " participate in a single transaction\n"));
                        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                        return CL_ERR_INVALID_PARAMETER;
                    }
                    break;
                }
            case CL_TXN_COMPONENT_CFG_2_PC_CAPABLE:
                {
                    if((pTxnJobDefn->jobCfg & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE) ||
                       (pTxnDefn->txnCfg & (CL_TXN_COMPONENT_CFG_1_PC_CAPABLE << 0x8)))
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("Both single and 2P components cannot"
                                 " participating in a single transaction\n"));
                        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                        return CL_ERR_INVALID_PARAMETER;
                    }
                    break;
                }
            default:
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("Both single and 2P components cannot"
                             " participating in a single transaction\n"));
                    clTxnMutexUnlock(clTxnClientInfo->txnMutex);
                    return CL_ERR_INVALID_PARAMETER;
                 }

        }

        rc = clTxnAppJobComponentAdd(pTxnJobDefn, txnCompAddress.iocPhyAddress, 
                                     configMask);
        if(rc == CL_OK)
        {
            /*
            pTxnJobDefn->jobCfg |= (configMask & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE) ;
            pTxnDefn->txnCfg |=  ((configMask & CL_TXN_COMPONENT_CFG_1_PC_CAPABLE)<< 0x8);
            */
            pTxnJobDefn->jobCfg |= configMask ;
            pTxnDefn->txnCfg |=  (configMask << 0x8);
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("Unable to add component into the table, rc=[0x%x]",
                     rc));
            clTxnMutexUnlock(clTxnClientInfo->txnMutex);
            return rc;
        }
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
            ("Result of setting component[0x%x:0x%x] to txn[0x%x:0x%x] jobId[0x%x] rc:0x%x", 
             txnCompAddress.iocPhyAddress.nodeAddress, 
             txnCompAddress.iocPhyAddress.portId, 
             pTxnDefn->clientTxnId.txnMgrNodeAddress, 
             pTxnDefn->clientTxnId.txnId, 
             pTxnJobDefn->jobId.jobId, rc));
    
    }
    else
    {
        /* FIXME: Do logging as well */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Invalid status of transaction for setting participating component-address\n"));
        rc = CL_ERR_INVALID_STATE;
    }

    /* Release lock here */
    if (clOsalMutexUnlock (clTxnClientInfo->txnMutex) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to release lock.\n"));
    }


    CL_TXN_RETURN_RC(rc, ("Failed to add new component for txn rc:0x%x\n", rc));
}

/*****************************************************************************
 *                              Debug APIs                                   *
 *****************************************************************************/

/**
 * API to validate a given transaction-handle
 */
ClRcT clTxnTxnHandleValidate(
        CL_IN ClTxnTransactionHandleT   tHandle)
{
    ClTxnDefnT      *pTxnDefn;

    CL_FUNC_ENTER();

    pTxnDefn = (ClTxnDefnT *)(ClWordT)tHandle;
    if (NULL == pTxnDefn)
    {
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }
    /* FIXME: Find additional validation process */
    return CL_OK;
}

/**
 * API to validate a given job handle
 */
ClRcT clTxnJobHandleValidate(
        CL_IN ClTxnJobHandleT           tHandle)
{
    ClTxnAppJobDefnT        *pTxnJobDefn;

    CL_FUNC_ENTER();
    pTxnJobDefn = (ClTxnAppJobDefnT *) tHandle;

    if (NULL == pTxnJobDefn)
    {
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }

    /* FIXME: Find additional validation process */
    return CL_OK;
}
/***************************************************************************
***********************Apis for getting back and deleting the **************
*************************agent response list *******************************
****************************************************************************/

ClRcT   clTxnJobInfoGet(
        CL_IN   ClTxnTransactionHandleT txnHandle,
        CL_IN   ClCntNodeHandleT        currentNodeHandle,
        CL_OUT  ClCntNodeHandleT        *pNextNodeHandle,
        CL_OUT  ClTxnAgentRespT         *pAgentResp)
{
    ClRcT               rc = CL_OK;
    ClTxnFailedTxnT     *pAgentListNode = NULL;
    ClTxnAgentT         *pFailedAgent = NULL;
    ClCntNodeHandleT    *pTempHandle = NULL;
    ClTxnDefnT          *pTxnDefn = NULL;

    if(0 == txnHandle)
    {
        return CL_TXN_RC(CL_ERR_INVALID_HANDLE);
    }
    if(NULL == pNextNodeHandle)
    {
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }
    if(NULL == pAgentResp)
    {
        return CL_TXN_RC(CL_ERR_NULL_POINTER);
    }
    clTxnMutexLock(clTxnClientInfo->txnMutex);
    rc = clCntDataForKeyGet(clTxnClientInfo->failedTxnDb,
                             (ClCntKeyHandleT) (ClWordT)txnHandle,
                             (ClCntDataHandleT *)&pAgentListNode);
    if(CL_OK != rc)
    {
        clTxnMutexUnlock(clTxnClientInfo->txnMutex);
        clLog(CL_LOG_INFO, "TXC", "FRD", 
                "There are no failed jobs in the transaction, it is successful");
        return rc;
    }
    clTxnMutexUnlock(clTxnClientInfo->txnMutex);
     
#ifdef CL_TXN_DEBUG
   CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "TC:failed agent count value : %d\n", pAgentListNode->failedAgentCount));
#endif
    if(0 == pAgentListNode->failedAgentCount)
    {
        clLog(CL_LOG_ERROR, "TXC", "FRD", 
                "Failed agent count SHOULD not be zero, possible corruption");
        return CL_TXN_RC(CL_TXN_ERR_AGENT_FAILED_VOID); 
    }
    if(0 == currentNodeHandle)
    {
        rc = clCntFirstNodeGet(pAgentListNode->failedAgentList,
                           (ClCntNodeHandleT *) &pTempHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("First node get from failed agent list failed: rc=[0x%x]\n", rc));
            return rc;
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,( "TC:Next node get\n"));
        rc = clCntNextNodeGet(pAgentListNode->failedAgentList,
                             (ClCntNodeHandleT) currentNodeHandle,
                             (ClCntNodeHandleT *)&pTempHandle);
        if(CL_OK != rc)
        {
           if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
                return CL_TXN_RC(CL_ERR_NOT_EXIST);
           else
           {
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Next node get from failed agent list failed for handle [%p], rc=[0x%x]\n", 
                         (ClPtrT)currentNodeHandle, rc));
               return rc;
           }
        }

    }
   *pNextNodeHandle = (ClCntNodeHandleT)pTempHandle;
    rc = clCntNodeUserDataGet(pAgentListNode->failedAgentList,
                              (ClCntNodeHandleT)*pNextNodeHandle,
                              (ClCntDataHandleT *)&pFailedAgent);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error in getting user data in failed agent list with handle"
                  "[%p]: rc =[0x%x]\n",(ClPtrT) *pNextNodeHandle, rc));
        return rc;
    }
    
    pTxnDefn = (ClTxnDefnT *)(ClWordT)txnHandle;
    /* This is required to get the job handle value */
    rc = clCntDataForKeyGet(pTxnDefn->jobList,
                       (ClCntKeyHandleT) &(pFailedAgent->failedAgentId.jobId),
                       (ClCntDataHandleT *)&(pAgentResp->jobHandle));
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error in getting job handle in the active job list: rc=[0x%x]", rc));
        return rc;
    }
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("TC:job id %x, txn id : %x, handle :%p, failed phase : %d\n",
            pFailedAgent->failedAgentId.jobId.jobId,
            pFailedAgent->failedAgentId.jobId.txnId.txnId,
            (ClPtrT)pAgentResp->jobHandle,
            pAgentResp->failedPhase));
#endif

    pAgentResp->failedPhase = pFailedAgent->failedPhase;
    pAgentResp->agentJobDefn = (ClTxnJobDefnHandleT)pFailedAgent->agentJobDefn;
    pAgentResp->agentJobDefnSize = pFailedAgent->agentJobDefnSize;
    pAgentResp->iocPhyAddress = pFailedAgent->failedAgentId.agentAddress;
    return CL_OK; 
}

/* This deletes the failed transaction db */
ClRcT   clTxnJobInfoListDel(
        ClTxnTransactionHandleT txnHandle)
{
    ClRcT   rc = CL_OK;
    ClCntNodeHandleT    delHandle = 0;
    ClTxnDefnT          *pTxnDefn = NULL;

    if(0 == txnHandle)
        return CL_TXN_RC(CL_ERR_INVALID_HANDLE);

    pTxnDefn = (ClTxnDefnT *)(ClWordT)txnHandle;
    clTxnMutexLock(clTxnClientInfo->txnMutex);

    /* Deleting the failed transaction node from the failed txn db*/ 
    if(CL_OK == clCntNodeFind(clTxnClientInfo->failedTxnDb,
                        (ClCntKeyHandleT) (ClWordT)pTxnDefn, &delHandle))
    {
        rc =  clCntNodeDelete(clTxnClientInfo->failedTxnDb,
                              delHandle);
        if(CL_OK != rc)
        {
            clLogDebug("CLT", "MTC",
                    "Failed to delete node corresponding to txn[0x%x:0x%x] from failedTxnDb",
                    pTxnDefn->clientTxnId.txnMgrNodeAddress,
                    pTxnDefn->clientTxnId.txnId);
        }
        else
            clLogNotice("CLT", "MTC",
                    "Successfully deleted txn defn from failedTxnDb");
    }
    
    /* Deleting the txn node from the active transaction db */
    rc = clTxnDbTxnDefnRemove(clTxnClientInfo->activeTxnDb, pTxnDefn->clientTxnId);
    if(CL_OK != rc)
    {
        clLogError("CLT", NULL, 
                "Unable to delete the node in active txndb with txnid [0x%x]: rc=[0x%x]",
                 pTxnDefn->clientTxnId.txnId, rc);
    }

    clTxnMutexUnlock(clTxnClientInfo->txnMutex);
    return rc;
}
/* Specific to COR */
ClRcT clTxnReadAgentJobsAsync(
        CL_IN ClTxnTransactionHandleT tHandle,
        CL_IN ClTxnTransactionCompletionCallbackT pTxnReadJobCallback)
{
    ClRcT                   rc          = CL_OK;
    ClTxnDefnT              *pTxnDefn   = NULL;
    ClCntNodeHandleT        currNode;
    ClTxnFailedTxnT        *pFailedTxn;

    CL_FUNC_ENTER();

    if (NULL == clTxnClientInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction client library not initialized."));
        CL_FUNC_EXIT();
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    pTxnDefn = (ClTxnDefnT *) (ClWordT)tHandle;
    if ( NULL == pTxnDefn )
    {
        rc = CL_ERR_NULL_POINTER;
        goto error;
    }
    if(pTxnDefn->jobCount == 0x0)
    {
        rc = CL_TXN_RC(CL_TXN_ERR_NO_JOBS);
        goto error;
    }
    
    rc = clCntFirstNodeGet(pTxnDefn->jobList, &currNode);

    while (CL_OK == rc)
    {
        ClCntNodeHandleT        tNode;
        ClTxnAppJobDefnT    *pTxnJobDefn = NULL;

        rc = clCntNodeUserDataGet(pTxnDefn->jobList, currNode, 
                                  (ClCntDataHandleT *)&pTxnJobDefn);
        
        if (pTxnJobDefn->compCount == 0x0)
        {
            rc = CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS);
            break;
        }

        tNode = currNode;
        rc = clCntNextNodeGet(pTxnDefn->jobList, tNode, &currNode);
    }
    
    if (rc == CL_TXN_RC(CL_TXN_ERR_NO_JOBS) || rc == CL_TXN_RC(CL_TXN_ERR_NO_COMPONENTS))
    {
        goto error;
    }
    if(pTxnReadJobCallback == NULL)
    {
        rc = CL_TXN_RC(CL_ERR_INVALID_PARAMETER);
        goto error;
    }

    pFailedTxn = (ClTxnFailedTxnT *) clHeapAllocate(sizeof(ClTxnFailedTxnT));
    if(pFailedTxn == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clLogError("CLT", NULL, 
                "Memory allocation failed");
        goto error;
    }
    memset(pFailedTxn, 0, sizeof(ClTxnFailedTxnT));
    rc = clTxnAgentListCreate( &(pFailedTxn->failedAgentList));
    if(rc != CL_OK)
    {
        clHeapFree(pFailedTxn);
        goto error;
    }
   

    if ( CL_OK == (rc = clOsalMutexLock(clTxnClientInfo->txnReadMutex)) )
    {
        clLogDebug("CLT", NULL,
                "Adding to read db, key[0x%x:0x%x]", 
                pTxnDefn->clientTxnId.txnMgrNodeAddress,
                pTxnDefn->clientTxnId.txnId);
        rc = clCntNodeAdd(clTxnClientInfo->failedTxnDb, 
            (ClCntKeyHandleT) pTxnDefn, 
            (ClCntDataHandleT) pFailedTxn, 0);
        if(rc != CL_OK)
        {
            rc = clCntDelete(pFailedTxn->failedAgentList);
            clHeapFree(pFailedTxn);
            clTxnMutexUnlock(clTxnClientInfo->txnReadMutex);
            goto error;
        }

        clTxnMutexUnlock(clTxnClientInfo->txnReadMutex);
    }
   
    pFailedTxn->fpTxnAgentCallback = pTxnReadJobCallback;
    
    clLogDebug("CLT", NULL, "Running txn:[0x%x:0x%x]", 
            pTxnDefn->clientTxnId.txnMgrNodeAddress,
            pTxnDefn->clientTxnId.txnId);
    rc = clTxnReadJobAsync(pTxnDefn,pFailedTxn);
    if(rc != CL_OK)
    {
        clHeapFree(pFailedTxn);
        goto error;
    }

error:
    if (CL_OK != rc)
    {
        clLogError("CLT", NULL, 
                "Error while processing transaction, rc[0x%x]", rc);
        rc = CL_TXN_RC(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Key compare function used to manage hash-map of active transactions.
 */
ClInt32T _clTxnTxnHandleCompare(CL_IN ClCntKeyHandleT key1, 
                            CL_IN ClCntKeyHandleT key2)
{

    return clTxnUtilTxnIdCompare( ((ClTxnDefnT *)key1)->clientTxnId, 
                                  ((ClTxnDefnT *)key2)->clientTxnId);

}
/**
 * Callback for deletion of a failed transaction job list
 */
void _clTxnFailedTxnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnFailedTxnT      *pFailedTxn;

    pFailedTxn = (ClTxnFailedTxnT *) userData;

    if (NULL != pFailedTxn)
    {
        clCntDelete(pFailedTxn->failedAgentList);
        clHeapFree(pFailedTxn);
    }
}


void _clTxnFailedTxnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnFailedTxnT      *pFailedTxn;

    pFailedTxn = (ClTxnFailedTxnT *) userData;

    if (NULL != pFailedTxn)
    {
        clCntDelete(pFailedTxn->failedAgentList);
        clHeapFree(pFailedTxn);
    }
}

/**
 * Function to register the CLI commands that can be used for testing.
 */
ClRcT 
clTxnClientDebugCliRegister(CL_OUT ClHandleT *pDbgHandle )
{
    ClRcT rc = CL_OK;
    ClNameT cliName = {0};
    ClCharT debugPrompt[15] = {0};
    
    clLogTrace ( "TXN", "CLI", "Registering the COR CLI commands for the component [%s]", cliName.value );
    /* Assuming that the component name is not taking whole of 256 bytes of cliName.value */
    strcat ( debugPrompt, "TXN_CLI" );
    clLog ( CL_LOG_SEV_TRACE, "TXN", "CLI", "The CLI command prompt is [%s]", debugPrompt );
    /* Registering the debug cli */
    rc  =   clDebugRegister ( _clTxnClientDbgCliFuncList, 
                            sizeof ( _clTxnClientDbgCliFuncList )/sizeof ( ClDebugFuncEntryT ),
                            pDbgHandle );
    if ( CL_OK != rc )
    {
        clLogError ( "TXN", "CLI", "Failed to register the function pointers" );
        return rc; 
    }

    /* Setting the debug prompt.  */
    rc = clDebugPromptSet ( debugPrompt );
    if ( CL_OK != rc )
    {
        clLogError ( "TXN", "CLI", "Failed while setting the prompt. rc[0x%x]", rc );
        return rc;
    }

    clLogInfo ( "TXN", "CLI", "Successfully completed the CLI registration [%s]", debugPrompt );

    return rc;
}
/**
 * Function to unregister the CLI commands that can be used for testing.
 */
ClRcT 
clTxnClientDebugCliUnregister()
{
    ClRcT   rc = CL_OK;

    /* Registering the debug cli */
    rc  =   clDebugDeregister (clTxnClientInfo->dbgHandle);
    if ( CL_OK != rc )
    {
        clLogError ( "TXN", "CLI", "Failed to unregister debug cli" );
    }
    else
        clLogDebug("TXN", "CLI", "Successfully unregistered debug client");
    return rc;
}

ClRcT 
clTxnActiveDbWalk( ClUint32T argc, ClCharT *argv[], ClCharT **retStr )
{
    ClRcT               rc = CL_OK;
    ClCharT             errStr[1000];
    ClUint32T           i = 0;
    ClBufferHandleT     outMsg = 0;
    ClUint32T           outLen = 0;

    if (NULL == clTxnClientInfo)
    {
        clLogError("TXN", "CLI", "Txn library is not initialized.");
        return CL_TXN_RC(CL_ERR_NOT_INITIALIZED);
    }

    rc = clBufferCreate(&outMsg);
    if(CL_OK != rc)
    {
        clLogError("TXN", "CLI", "Failed to create a buffer. rc [0x%x]", rc);
        return rc;
    }
    
    clLogDebug("TXN", "CLI",
            "Inside %s, argc [%d]",
            __FUNCTION__, argc);

    if(argc > 1)
    {
        for(i = 1; i < argc; i++)
        {
            ClTxnDefnT  *pTxnDefn;
            ClTxnTransactionIdT tId;
            rc = sscanf(argv[i], "%x", &(tId.txnId)); 

            if ( (!rc) || (EOF == rc))
            {
                sprintf(errStr, "%s", "sscanf failed");
                goto return_retStr;
            }
            

            tId.txnMgrNodeAddress = clIocLocalAddressGet();

            clLogDebug("TXN", "CLI", "TxnId : 0x%x", tId.txnId);

            rc = clCntDataForKeyGet(clTxnClientInfo->activeTxnDb, 
                    (ClCntKeyHandleT)&tId, 
                    (ClCntDataHandleT )&pTxnDefn);
            if(CL_OK != rc)
            {
                sprintf(errStr, "Data get for key failed, rc [0x%x]", rc);
                goto return_retStr;
            }
            clTxnClientCliDbgPrint(outMsg, 
                    "Transaction Description:\n"               
                     "\tClient TxnId\t:\t0x%x:0x%x\n"           
                     "\tCurrentState\t:\t%s\n"                  
                     "\tJob Count\t:\t0x%x\n"                   
                     "\tConfiguration\t:\t%s\n",  
                     pTxnDefn->clientTxnId.txnMgrNodeAddress,
                     pTxnDefn->clientTxnId.txnId, 
                     gTxnStatusStr[pTxnDefn->currentState], 
                     pTxnDefn->jobCount, (pTxnDefn->txnCfg)?"SYNC":"ASYNC");
        }  
    }
    else
    {
        ClUint32T    cntLength = 0;
        clLogDebug("TXN", "CLI",
                "Walking activeTxnDb");

        clCntSizeGet(clTxnClientInfo->activeTxnDb, &cntLength);
        clLogDebug("TXN", "CLI",
                "Walking activeTxnDb, size[%d]", cntLength);

        rc = clCntWalk(clTxnClientInfo->activeTxnDb, 
                _clTxnWalk,
                outMsg, 
                sizeof(ClBufferHandleT));
        if(CL_OK != rc)
        {
            sprintf(errStr, "Walk on active Db failed, rc[0x%x]", rc);
            goto return_retStr;
        }
    }

    rc = clBufferLengthGet(outMsg, &outLen);
    if(CL_OK != rc)
    {
        sprintf(errStr, "Buffer length get failed, rc[0x%x]", rc);
        goto return_retStr;
    }
    
    clLogDebug("TXN", "CLI",
            "Length of buffer[%d]", outLen);
    *retStr = (ClCharT *)clHeapCalloc(1, outLen + 1);
    if(!*retStr)
    {
        sprintf(errStr, "could not allocate memory, rc[0x%x]", rc);
        goto return_retStr;
    }

    rc = clBufferNBytesRead(outMsg, (ClUint8T *) *retStr, &outLen);
    if(CL_OK != rc)
    {
        sprintf(errStr, "Buffer read failed, rc[0x%x]", rc);
        goto return_retStr;
    }

    clLogDebug("TXN", "CLI",
            "retStr [%s]", *retStr);
    clBufferDelete(&outMsg);
    return rc;

return_retStr:
    clTxnClientCliDbgPrint(outMsg,
            "[%s]\nUsage: showActiveDb [txnId]...\n"
            "txnId [HEX] : Id of a transaction\n"
            "If no txnId is given, all the existing entries will be displayed.\n"
            "If more than one txnId is given, then all the corresponding transaction info is displayed.", errStr);
    rc = clBufferLengthGet(outMsg, &outLen);
    if(CL_OK != rc)
    {
        sprintf(errStr, "Buffer length get failed, rc[0x%x]", rc);
        return rc;
    }
    
    *retStr = (ClCharT *)clHeapCalloc(1, outLen + 1);
    if(!*retStr)
    {
        sprintf(errStr, "could not allocate memory, rc[0x%x]", rc);
        return rc;
    }

    rc = clBufferNBytesRead(outMsg, (ClUint8T *) *retStr, &outLen);
    if(CL_OK != rc)
    {
        sprintf(errStr, "Buffer read failed, rc[0x%x]", rc);
        return rc;
    }

    clBufferDelete(&outMsg);
    return rc;
}

ClRcT _clTxnWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen)
{
    ClRcT   rc = CL_OK;
    ClBufferHandleT outMsg;
    ClTxnDefnT  *pTxnDefn = NULL;

    clLogDebug("TXN", "CLI",
            "INside cnt walk function");
    pTxnDefn = (ClTxnDefnT *)userData;
    outMsg = (ClBufferHandleT)userArg;

    if(!pTxnDefn)
    {
        return CL_ERR_NULL_POINTER;
    }
    clTxnClientCliDbgPrint(outMsg,  
            "Transaction Description:\n"               \
             "\tClient TxnId\t:\t0x%x:0x%x\n"           \
             "\tCurrentState\t:\t%s\n"                  \
             "\tJob Count\t:\t0x%x\n"                   \
             "\tConfiguration\t:\t%s\n",  
             pTxnDefn->clientTxnId.txnMgrNodeAddress,
             pTxnDefn->clientTxnId.txnId, 
             gTxnStatusStr[pTxnDefn->currentState], 
             pTxnDefn->jobCount, (pTxnDefn->txnCfg)?"SYNC":"ASYNC");
    return rc;
}
