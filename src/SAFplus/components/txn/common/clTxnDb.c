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
 * File        : clTxnDb.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains implementation of database implementation of transaction
 * information.
 *
 *
 *****************************************************************************/

#include <string.h>

#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clCntApi.h>

#include <clTxnCommonDefn.h>
#include <clTxnDb.h>
#include <clTxnErrors.h>

/*****************************************************************************
 *  Internal Functions  (Forwrd Declaration)                                 *
 *****************************************************************************/
/**
 * This is key-compare function used by container for storing active-txn 
 */
static ClInt32T _clTxnTxnIdCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

/**
 * This is hash-fn used to store active-txn in a hash-map
 */
static ClUint32T _clTxnTxnIdHashFn(
        CL_IN   ClCntKeyHandleT     key);

/**
 * Callback function for deleting an entry of transaction-definition
 */
static void _clTxnTxnDefnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

/**
 * Callback function for destroying an entry. This is called when a cnt is deleted *
 */
static void _clTxnTxnDefnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);
/**
 * This is key-compare function used by container for storing comp-services
 */
static ClInt32T _clTxnCmpKeyCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

/**
 * This is function for deleting an entry of failed-agent information.
 */
static void 
_clTxnAgentDelFn(
        CL_IN   ClCntKeyHandleT     key, 
        CL_IN   ClCntDataHandleT    data);
/* This function is the destroy callback for the agent list */
static void 
_clTxnAgentDestroy(
        CL_IN   ClCntKeyHandleT     key, 
        CL_IN   ClCntDataHandleT    data);

/**
 * This is key-compare function used by container for comparing Agent-Component address.
 */
static ClInt32T 
_clTxnUtilCmpKeyCompare(CL_IN ClCntKeyHandleT key1,
                               CL_IN ClCntKeyHandleT key2);

/**
 * This is key-compare function used by container for storing failed-agent information
 */
static ClInt32T  
_clTxnAgentCmpFn(
        CL_IN   ClCntKeyHandleT key1, 
        CL_IN   ClCntKeyHandleT key2);

/**
 * Callback function to delete an entry in job-specific component-list.
 * This list consists of component-address and sequence-no.
 */
static void _clTxnCompEntryDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

/**
 * Destroy callback function for the complist
 */
static void _clTxnCompEntryDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);
/**
 * This is key-compare function used by container library for storing job-definitions
 */
static ClInt32T _clTxnJobKeyCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2);

/**
 * Hash function used to manage hash-map of job-definitions.
static ClUint32T _clTxnJobIdHashFn(
        CL_IN   ClCntKeyHandleT     key);
 */

/**
 * Callback function to delete single job-definition
 */
static void _clTxnJobDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);

/**
 * Destroy callback function 
 */
static void _clTxnJobDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData);
/**
 * Creates new transaction definition instance
 */
ClRcT clTxnNewTxnDfnCreate(
        CL_OUT  ClTxnDefnPtrT    *pNewTxnDef)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();
    if (NULL == pNewTxnDef)
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }
    
    *pNewTxnDef = (ClTxnDefnT *) clHeapCalloc(1,sizeof(ClTxnDefnT));

    if (NULL == *pNewTxnDef)
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }

    memset(*pNewTxnDef, 0, sizeof(ClTxnDefnT));

    /* Initialize data-structure for storing job and component details */
    rc = clCntLlistCreate(_clTxnJobKeyCompare,
                          _clTxnJobDelete, _clTxnJobDestroy,
                          CL_CNT_UNIQUE_KEY, 
                          (ClCntHandleT *) &( (*pNewTxnDef)->jobList));

    if (CL_OK != rc)
    {
        clHeapFree(*pNewTxnDef);
        *pNewTxnDef = NULL;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Delete transaction definition
 */
ClRcT clTxnDefnDelete(
        CL_IN   ClTxnDefnT    *pTxnDefn)
{
    CL_FUNC_ENTER();

    /* Clean-up all other memory allocated with this transaction */

    if (NULL != pTxnDefn)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
            ("Removing transaction definition. txnId:0x%x:0x%x", 
             pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pTxnDefn->serverTxnId.txnId));
        if(pTxnDefn->jobList)
        {
            //clCntAllNodesDelete(pTxnDefn->jobList); /* This can be removed by giving the destroy callback */
            clCntDelete(pTxnDefn->jobList);
        }
        else
        {
            clDbgCodeError(0,("Where did the jobList go?"));            
        }

        clHeapFree(pTxnDefn);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Add newly created job to transaction.
 * FIXME: THIS IS NOT MULTITHREAD SAFE
 */
ClRcT clTxnNewAppJobAdd(
        CL_IN   ClTxnDefnT          *pTxnDefn, 
        CL_IN   ClTxnAppJobDefnT    *pNewTxnJob)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    if ( (NULL == pTxnDefn) || (NULL == pNewTxnJob) )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inserting new job for txn %p\n", (ClPtrT) pTxnDefn));

    /* With RC2 enhancements, generate unique id for this job and update accordingly */
    clLogDebug("CMN", NULL,  
            "PID[%d], Adding job JOBID : {jobid: [0x%x] ]} for txn with TXNID: { txnId: [0x%x], mgrAdd : [0x%x]}",
            (int)getpid(),
            pNewTxnJob->jobId.jobId,
            pTxnDefn->clientTxnId.txnId,
            pTxnDefn->clientTxnId.txnMgrNodeAddress);

    rc = clCntNodeAdd(pTxnDefn->jobList, 
                      (ClCntKeyHandleT) &(pNewTxnJob->jobId), 
                      (ClCntDataHandleT) pNewTxnJob, 0);
    if (CL_OK != rc)
    {
        rc = CL_GET_ERROR_CODE(rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Adding to the job list failed, [0x%x]", rc));
        return rc;
    }
    pTxnDefn->jobCount++;
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Remove an existing job from transaction.
 * FIXME: THIS IS NOT MULTITHREAD SAFE
 */
ClRcT clTxnAppJobRemove(
        CL_IN   ClTxnDefnT              *pTxnDefn, 
        CL_IN   ClTxnTransactionJobIdT  jobId)
{
    CL_FUNC_ENTER();

    if (NULL == pTxnDefn)
    {
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    clCntAllNodesForKeyDelete(pTxnDefn->jobList, 
                              (ClCntKeyHandleT) &jobId);
    /* Following has to be done atomically */
    pTxnDefn->jobCount--;


    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 * Create newly transaction-job definition
 */
ClRcT clTxnNewAppJobCreate(
        CL_OUT  ClTxnAppJobDefnPtrT  *pNewTxnJob)
{
    ClRcT               rc  = CL_OK;
    ClTxnAppJobDefnT    *pTxnJob;

    CL_FUNC_ENTER();

    if (NULL == pNewTxnJob)
    {
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    *pNewTxnJob = NULL;
    
    pTxnJob = (ClTxnAppJobDefnT *)clHeapAllocate( sizeof(ClTxnAppJobDefnT));
    if (NULL == pTxnJob)
    {
        CL_FUNC_EXIT();
        return CL_ERR_NO_MEMORY;
    }

    memset(pTxnJob, 0, sizeof(ClTxnAppJobDefnT));
    rc = clCntLlistCreate(_clTxnCmpKeyCompare,
                          _clTxnCompEntryDelete, _clTxnCompEntryDestroy,
                          CL_CNT_UNIQUE_KEY,
                          (ClCntHandleT *) &(pTxnJob->compList) );

    if (CL_OK != rc)
    {
        clHeapFree(pTxnJob);
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Creating new txn-job %p, compList:%p", 
                        (ClPtrT) pTxnJob, (ClPtrT) pTxnJob->compList));
        *pNewTxnJob = pTxnJob;
    }

    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

/**
 * Delete/free transaction-job definition
 */
ClRcT clTxnAppJobDelete(
        CL_IN   ClTxnAppJobDefnT *pTxnAppJob)
{
    CL_FUNC_ENTER();

    if (NULL == pTxnAppJob)
    {
        CL_FUNC_EXIT();
        clDbgCodeError(CL_ERR_NULL_POINTER, ("Null pointer on the job list"));
        return CL_ERR_NULL_POINTER;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting txn-job %p", (ClPtrT) pTxnAppJob));

    if(pTxnAppJob->compList)
    {
        //clCntAllNodesDelete(pTxnAppJob->compList);
        clCntDelete(pTxnAppJob->compList);
    }

    if (pTxnAppJob->appJobDefn)
        clHeapFree( (ClPtrT) pTxnAppJob->appJobDefn);
    
    memset(pTxnAppJob, 0, sizeof(ClTxnAppJobDefnT));  /* to crash uses after delete */
    clHeapFree(pTxnAppJob);

    return (CL_OK);
}

/**
 * Adds information about participating component in the transaction for 
 * the given job
 * FIXMETHIS IS NOT MULTITHREAD SAFE
 */
ClRcT clTxnAppJobComponentAdd(
        CL_IN   ClTxnAppJobDefnT        *pTxnJobDefn, 
        CL_IN   ClIocPhysicalAddressT   txnCompAddress, 
        CL_IN   ClUint8T                configMask)
{
    ClRcT                   rc  = CL_OK;
    ClTxnAppComponentT      *pNewComp = NULL;

    CL_FUNC_ENTER();

    if (NULL == pTxnJobDefn)
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    pNewComp = (ClTxnAppComponentT *) clHeapCalloc(1,sizeof(ClTxnAppComponentT));
    if (NULL == pNewComp)
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NO_MEMORY);
    }

    pNewComp->appCompAddress = txnCompAddress;
    pNewComp->configMask = configMask;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Adding new component to txn-job %p\n", (ClPtrT) pTxnJobDefn));

    rc = clCntNodeAdd(pTxnJobDefn->compList,
                      (ClCntKeyHandleT) &(pNewComp->appCompAddress),
                      (ClCntDataHandleT) pNewComp, 0);
    if (CL_OK == rc)
    {
        pTxnJobDefn->compCount++;
    }
    else
    {
        clHeapFree(pNewComp);
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Initializes transaction database for maintaining active transactions
 */
ClRcT clTxnDbInit(
        CL_OUT ClTxnDbHandleT  *pTxnDb)
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();

    if (NULL == pTxnDb)
    {
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    rc = clCntThreadSafeHashtblCreate(CL_TXN_NUM_BUCKETS, 
                            _clTxnTxnIdCompare, 
                            _clTxnTxnIdHashFn, 
                            _clTxnTxnDefnDelete, _clTxnTxnDefnDestroy, 
                            CL_CNT_UNIQUE_KEY, 
                            pTxnDb);
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Finalizes transaction database
 */
ClRcT clTxnDbFini(
        CL_IN   ClTxnDbHandleT  txnDb)
{
    CL_FUNC_ENTER();

    /* TODO: Check for active transactions, if any */
    clCntDelete(txnDb);

    CL_FUNC_EXIT();
    return CL_OK;
}

/**
 * Adds a new definition of transaction to txn-database
 */
ClRcT clTxnDbNewTxnDefnAdd(
        CL_IN   ClTxnDbHandleT      txnDb,
        CL_IN   ClTxnDefnT          *pNewTxn,
        CL_IN   ClTxnTransactionIdT *pTxnId)
{
    ClRcT       rc  = CL_OK;
    ClTxnDefnT  *pTxnDef;
    CL_FUNC_ENTER();

    rc = clCntDataForKeyGet(txnDb, (ClCntKeyHandleT) pTxnId, 
                            (ClCntDataHandleT *) &pTxnDef);
    if (CL_OK != rc)
    {
        rc = clCntNodeAdd(txnDb, 
                          (ClCntKeyHandleT) pTxnId, 
                          (ClCntDataHandleT) pNewTxn, 0);
    }
    else
    {
        rc = CL_ERR_DUPLICATE;
    }

    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}
/**
 * Remove a transaction definition from txn-database.
 */
extern ClRcT clTxnDbTxnDefnRemove(
        CL_IN   ClTxnDbHandleT      txnDb,
        CL_IN   ClTxnTransactionIdT txnId)
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    rc = clCntAllNodesForKeyDelete(txnDb, (ClCntKeyHandleT) &txnId);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Retrieve txn-definition using txn-id as index
 */
ClRcT clTxnDbTxnDefnGet(
        CL_IN   ClTxnDbHandleT      txnDb, 
        CL_IN   ClTxnTransactionIdT txnId, 
        CL_OUT  ClTxnDefnT          **pTxnDefn)
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();

    if (NULL == pTxnDefn)
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    rc = clCntDataForKeyGet(txnDb, (ClCntKeyHandleT) &txnId, 
                            (ClCntDataHandleT *) pTxnDefn);

    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

ClRcT clTxnAgentListCreate(ClCntHandleT *pAgentCntHandle)
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();
    rc = clCntLlistCreate(_clTxnAgentCmpFn, _clTxnAgentDelFn, 
                                            _clTxnAgentDestroy,
                                            CL_CNT_UNIQUE_KEY,
                                            pAgentCntHandle);
    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

/****************************************************************************
 *                  Internal (Private) Functions                           *
 ***************************************************************************/
/**
 * Key compare function used to manage hash-map of active transactions.
 */
ClInt32T _clTxnTxnIdCompare(CL_IN ClCntKeyHandleT key1, 
                            CL_IN ClCntKeyHandleT key2)
{

    return clTxnUtilTxnIdCompare( (*(ClTxnTransactionIdT *)key1), 
                                  (*(ClTxnTransactionIdT *)key2));

}

/**
 * Hash function used to manage hash-map of active transactions.
 */
ClUint32T _clTxnTxnIdHashFn(CL_IN ClCntKeyHandleT key)
{
    ClTxnTransactionIdT *pTxnId;
    CL_FUNC_ENTER();

    pTxnId = (ClTxnTransactionIdT *) key;
    if (NULL == pTxnId)
        return 0;

    CL_FUNC_EXIT();

    return ( ((ClTxnTransactionIdT *)key)->txnId % CL_TXN_NUM_BUCKETS);
}

/**
 * Callback for deletion of a transaction
 */
void _clTxnTxnDefnDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnDefnT      *pTxnDefn;

    pTxnDefn = (ClTxnDefnT *) userData;
    if(pTxnDefn) 
        clTxnDefnDelete(pTxnDefn);

}
void _clTxnTxnDefnDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnDefnT      *pTxnDefn;

    pTxnDefn = (ClTxnDefnT *) userData;
    if(pTxnDefn) 
        clTxnDefnDelete(pTxnDefn);
}
/**
 * Key Compare function used while managing addresses application 
 * components for a job.
 */
ClInt32T _clTxnCmpKeyCompare(CL_IN ClCntKeyHandleT key1, 
                             CL_IN ClCntKeyHandleT key2)
{
    ClIocAddressT   *c1, *c2;
    CL_FUNC_ENTER();
    c1 = (ClIocAddressT *)key1;
    c2 = (ClIocAddressT *)key2;

    CL_FUNC_EXIT();
    return (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress == 0) ? (c1->iocPhyAddress.portId - c2->iocPhyAddress.portId) : (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress);
}

/**
 * Callback function to delete an entry in job-specific component-list.
 * This list consists of component-address and sequence-no.
 */
void _clTxnCompEntryDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting an entry in job-spec-comp-list\n"));
    clHeapFree((ClTxnAppComponentT*) userData);
}

/**
 * Destroy callback function for the complist 
 */
void _clTxnCompEntryDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting an entry in job-spec-comp-list\n"));
    if (userData) clHeapFree((ClTxnAppComponentT*) userData);
}
/**
 * Key compare function used while managing container of job-definitions.
 */
ClInt32T _clTxnJobKeyCompare(
        CL_IN   ClCntKeyHandleT     key1, 
        CL_IN   ClCntKeyHandleT     key2)
{
    return clTxnUtilJobIdCompare ( (*(ClTxnTransactionJobIdT *)key1), 
                                   (*(ClTxnTransactionJobIdT *)key2));
}

/**
 * Callback function to delete single job-definition
 */
void _clTxnJobDelete(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnAppJobDefnT    *pTxnJob;
    pTxnJob = (ClTxnAppJobDefnT *) userData;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting single job-definition\n"));
    clTxnAppJobDelete(pTxnJob);
}
void _clTxnJobDestroy(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData)
{
    ClTxnAppJobDefnT    *pTxnJob;
    pTxnJob = (ClTxnAppJobDefnT *) userData;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Deleting single job-definition\n"));
    clTxnAppJobDelete(pTxnJob);
}

/**
 * Callback function to compare failed-agent unique-Id
 */
static ClInt32T  
_clTxnAgentCmpFn(
        CL_IN   ClCntKeyHandleT key1, 
        CL_IN   ClCntKeyHandleT key2)
{
    ClTxnAgentIdT *pAgentId1 = (ClTxnAgentIdT *)key1;
    ClTxnAgentIdT *pAgentId2 = (ClTxnAgentIdT *)key2;
    ClInt32T cmp = 0;

    cmp = clTxnUtilJobIdCompare(pAgentId1->jobId,pAgentId2->jobId);
    if(cmp == 0)
       cmp = _clTxnUtilCmpKeyCompare((ClCntKeyHandleT)&(pAgentId1->agentAddress),(ClCntKeyHandleT)&(pAgentId2->agentAddress)); 

    return (cmp);
}

/**
 * Key Compare function for agent-component addresses .
*/
static ClInt32T 
_clTxnUtilCmpKeyCompare(CL_IN ClCntKeyHandleT key1,
                               CL_IN ClCntKeyHandleT key2)
{
    ClIocAddressT   *c1, *c2;
    CL_FUNC_ENTER();
    c1 = (ClIocAddressT *)key1;
    c2 = (ClIocAddressT *)key2;
 
    CL_FUNC_EXIT();
    return (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress == 0) ? (c1->iocPhyAddress.portId - c2->iocPhyAddress.portId)         : (c1->iocPhyAddress.nodeAddress - c2->iocPhyAddress.nodeAddress);
}
                                                 
/**
 * Delete function for failed-agent-info entry from the failed-agents-list container .
*/
static void 
_clTxnAgentDelFn(
        CL_IN   ClCntKeyHandleT     key, 
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnAgentT     *pFailedAgentInfo = (ClTxnAgentT *)data;
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Inside %s\n", __FUNCTION__));
#endif
    if(pFailedAgentInfo)
    {
        if(pFailedAgentInfo->agentJobDefn)
            clHeapFree(pFailedAgentInfo->agentJobDefn);
        clHeapFree(pFailedAgentInfo);
    }
}

static void 
_clTxnAgentDestroy(
        CL_IN   ClCntKeyHandleT     key, 
        CL_IN   ClCntDataHandleT    data)
{
    ClTxnAgentT     *pFailedAgentInfo = (ClTxnAgentT *)data;
#ifdef CL_TXN_DEBUG
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Inside %s\n", __FUNCTION__));
#endif
    if(pFailedAgentInfo)
    {
        if(pFailedAgentInfo->agentJobDefn)
            clHeapFree(pFailedAgentInfo->agentJobDefn);
        clHeapFree(pFailedAgentInfo);
    }
}


