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
 * ModuleName  : cor
 * File        : clCorTxnImpl.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements transaction and job definitions/instances in COR-Client
 *****************************************************************************/

/* INCLUDES */
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clOsalApi.h>
#include <clCntApi.h>
#include <clBitApi.h>
#include <clXdrApi.h>
#include <clHandleApi.h>

#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorTxnApi.h>
#include <clCorLog.h>

#include <clCorClient.h>
#include <clCorRMDWrap.h>

/* Internal Headers*/
#include "clCorTxnClientIpi.h"
#include <clCorPvt.h>
#include <xdrClCorObjectHandleIDLT.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* Globals */
#define CL_COR_CLIENT_TXN_SESSION_RETRY_TIME 100
/* Container to store the transaction failed job information */
ClCntHandleT gCorTxnInfo = 0;
extern ClHandleDatabaseHandleT  gCorDbHandle;

extern  ClOsalMutexIdT  gCorBundleMutex;

/* Forward declaration */

static ClInt32T _clCorTxnTreeCompare(
    CL_IN       ClCntKeyHandleT         oh1,
    CL_IN       ClCntKeyHandleT         oh2);

static ClInt32T _clCorTxnListCompare(
    CL_IN       ClCntKeyHandleT         oh1,
    CL_IN       ClCntKeyHandleT         oh2);

static void _clCorTxnJobHeaderNodeDelete(
    CL_IN       ClCntKeyHandleT         key,
    CL_IN       ClCntDataHandleT        data);

static ClRcT _clCorTxnJobPack(
    CL_IN       ClCntKeyHandleT         key, 
    CL_IN       ClCntDataHandleT        data, 
    CL_IN       ClCntArgHandleT         userArg, 
    CL_IN       ClUint32T               len);

static ClRcT _clCorTxnObjHdlExtract(
    CL_IN       ClCntKeyHandleT         key, 
    CL_IN       ClCntDataHandleT        data, 
    CL_IN       ClCntArgHandleT         userArg, 
    CL_IN       ClUint32T               len);

static ClRcT _clCorTxnResponseProcess(
    CL_IN   ClCorTxnSessionT        txnSession,
    CL_IN   ClBufferHandleT         outMsgHandle);

static ClRcT clCorTxnRetrieveFailedJobs(
    CL_IN   ClCorTxnSessionT        txnSession, 
    CL_IN   ClBufferHandleT  outMsgHandle);

static ClRcT clCorTxnProcessFailedJobs(
    CL_IN   ClCorTxnSessionT txnSession, 
    CL_IN   ClCorTxnJobHeaderNodeT *pTxnJobHdrNode);

static ClInt32T clCorTxnInfoStoreKeyCompFn(
    CL_IN   ClCntKeyHandleT         key1, 
    CL_IN   ClCntDataHandleT        key2);

static ClUint32T clCorTxnInfoStoreHashGenFn(
    CL_IN   ClCntKeyHandleT         key);

static void clCorTxnInfoStoreDestroyFn(
    CL_IN   ClCntKeyHandleT         userKey, 
    CL_IN   ClCntDataHandleT        userData);

static void clCorTxnInfoStoreDeleteFn(
    CL_IN   ClCntKeyHandleT         userKey, 
    CL_IN   ClCntDataHandleT        userData);

static ClRcT _clCorTxnJobValidate(
        ClCntKeyHandleT key,
        ClCntDataHandleT data,
        ClCntArgHandleT userArg,
        ClUint32T len);

static void  
clCorProcessResponseCB( CL_IN ClRcT rc,
                        CL_IN ClPtrT pCookie,
                        CL_IN ClBufferHandleT inMsgHandle,
                        CL_OUT ClBufferHandleT outMsgHandle);

static ClRcT
clCorBundleUpdate( CL_IN   ClCorBundleInfoPtrT    pBundleInfo,
                    CL_IN   ClCorTxnJobHeaderNodeT  *pJobHeaderNode);

static ClRcT
_clCorBundleUpdateData ( CL_IN       ClCorTxnJobHeaderNodeT    *pJobHeaderNode,
                        CL_IN       ClCorTxnObjJobNodeT       *pObjJobRecv,
                        CL_INOUT    ClCorTxnObjJobNodeT       *pObjJobNative);

/**
 * Create the handle and store in the data.
 */
static ClRcT     
clCorBundleHandleStore( CL_IN ClCorBundleInfoPtrT pBundleInfo,
                         CL_OUT ClHandleT *dBHandle);

/**
 * Destroy the entry of the handle from the handle database.
 */
static ClRcT
clCorBundleHandleDestroy(  CL_OUT  ClHandleT dBhandle);

/**
 * This is a common fuction called after making the rmd in bundle. For sync bundle 
 * this function is called just after getting the response. For async bundle, this 
 * is called in the callback function.
 */ 
static ClRcT _clCorBundleJobResponseProcess(ClBufferHandleT *pOutMsgHandle,
                                    ClCorBundleInfoPtrT *pBundleInfo,
                                    ClCorBundleHandlePtrT pBundleHandle);

static ClRcT
corValidateDuplicateSetJob(ClCorTxnObjJobNodeT* pObjJobNode, ClCorUserSetInfoT* pUsrSetInfo);

static ClRcT
corValidateDuplicateObjJob(ClCorTxnJobHeaderNodeT* pTxnJobHdrNode, ClCorOpsT op);

/**
 * Internal Function
 *
 * This method creates new COR-Txn job Header defining
 * all operations on a given COR-Object.
 */
ClRcT
clCorTxnJobHeaderNodeCreate(CL_OUT ClCorTxnJobHeaderNodeT  **pJobHdrNode)
{
    CL_FUNC_ENTER();

    /* Create cor-txn description object */
    *pJobHdrNode = (ClCorTxnJobHeaderNodeT *) clHeapAllocate(sizeof(ClCorTxnJobHeaderNodeT));
    if (NULL == *pJobHdrNode)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                     CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset(*pJobHdrNode, 0, sizeof(ClCorTxnJobHeaderNodeT));

    /* Initialize the moId */
    clCorMoIdInitialize(&((*pJobHdrNode)->jobHdr.moId));

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 *  Internal Function
 *
 *  This function removed the first attrSetJob from the obj-Job.
 *
 *  @returns CL_OK  - if a job is found and successfuly removed.
 *   ERROR if transaction does not have any jobs. Other error codes
 *   otherwise.
 */
ClRcT     
clCorTxnFirstAttrSetJobNodeRemove(
    CL_IN   ClCorTxnObjJobNodeT *pThis)
{
    ClRcT           rc      = CL_OK;
    ClCorTxnAttrSetJobNodeT* tmpPtr;
    ClCorTxnAttrSetJobNodeT* nextJob;

    CL_FUNC_ENTER();

    if (pThis->head == NULL)
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS);
    }

    tmpPtr = pThis->head;
    if (pThis->tail == pThis->head)
    {
        pThis->tail = pThis->head = NULL;
    }
    else
    {
        nextJob = (pThis->head)->next;
        nextJob->prev = NULL;
        pThis->head   = nextJob;
    } 
    /* First free the value buffer */
     clHeapFree(tmpPtr->job.pValue);
 
    /* Free the AttrSet Job */
     clHeapFree(tmpPtr);

    CL_FUNC_EXIT();
  return(rc);
}


/**
 *  Internal Function
 *
 *  This function removed the first obj job from the transaction
 *
 *  @returns CL_OK  - if a job is found and successfuly removed.
 *   ERROR if transaction does not have any jobs. Other error codes
 *   otherwise.
 */
ClRcT     
clCorTxnFirstObjJobNodeRemove(
    CL_IN   ClCorTxnJobHeaderNodeT *pThis)
{
    ClRcT           rc      = CL_OK;
    ClCorTxnObjJobNodeT* tmpPtr;
    ClCorTxnObjJobNodeT* nextJob;

    CL_FUNC_ENTER();

    if (pThis->head == NULL)
    {
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS);
    }

    tmpPtr = pThis->head;
    if (pThis->tail == pThis->head)
    {
        pThis->tail = pThis->head = NULL;
    }
    else
    {
        nextJob = (pThis->head)->next;
        nextJob->prev = NULL;
        pThis->head   = nextJob;
    } 
    while (CL_OK == clCorTxnFirstAttrSetJobNodeRemove(tmpPtr));

    clHeapFree(tmpPtr);
    CL_FUNC_EXIT();
    return(rc);
}


/**
 * Internal Method
 *
 * This method deletes a COR-Txn Object Header along with all 
 * operations associated with it
 */
ClRcT
clCorTxnJobHeaderNodeDelete(CL_IN ClCorTxnJobHeaderNodeT *pJobHdrNode)
{
    CL_FUNC_ENTER();

    /* Delete all jobs and release pJobHdrNode */
    if (NULL == pJobHdrNode)
    {
        return (CL_OK);
    }

    /* Remove all jobs in this Txn-Header object */
    while (CL_OK == clCorTxnFirstObjJobNodeRemove(pJobHdrNode));

    /* Delete the container, if there is one.*/
    if(pJobHdrNode->objJobTblHdl)
      clCntDelete(pJobHdrNode->objJobTblHdl);

    if (pJobHdrNode->jobHdr.oh)
        clHeapFree(pJobHdrNode->jobHdr.oh);

    clHeapFree(pJobHdrNode);

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 * Internal Method
 *
 * This function creates and initializes new transaction by allocating locally unique
 * transaction-id for this client to define single/multi operation transaction.
 */
typedef struct ClCorTxnHandle
{
    ClCntHandleT tree;
    ClCntHandleT list;
}ClCorTxnHandleT;

ClRcT
clCorTxnNewSessionCreate(ClCorTxnSessionT   *pTxnSession)
{
    ClRcT           rc = CL_OK;
    ClCorTxnHandleT* pTxnHandle = NULL;

    CL_FUNC_ENTER();

    if (NULL == pTxnSession)
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if (CL_COR_TXN_MODE_SIMPLE == pTxnSession->txnMode)
    {
        /* Single operation transaction - ClCorTxnJobHeaderNodeT* is the txn-id */
        rc = clCorTxnJobHeaderNodeCreate((ClCorTxnJobHeaderNodeT **)&pTxnSession->txnSessionId);
    }
    else
    {
        pTxnHandle = (ClCorTxnHandleT *) clHeapAllocate(sizeof(ClCorTxnHandleT));
        if (!pTxnHandle)
        {
            clLogError("COR", "TXN", "Failed to allocate memory.");
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        /* Multi operation transaction - Create llist for cor-txn descriptor objects */
        rc = clCntRbtreeCreate (_clCorTxnTreeCompare,
                               _clCorTxnJobHeaderNodeDelete,
                               NULL, CL_CNT_UNIQUE_KEY, 
                               (ClCntHandleT *) &(pTxnHandle->tree));

        rc = clCntLlistCreate (_clCorTxnListCompare,
                                NULL, NULL, CL_CNT_UNIQUE_KEY, 
                               (ClCntHandleT *) &(pTxnHandle->list));

        /* pTxnHandle acts as the transaction-id */
        pTxnSession->txnSessionId = pTxnHandle;
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                      ("Failed to create llist for cor-txn-descr. rc:0x%x\n", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Method
 *
 * Deletes an transaction-session
 */
ClRcT
clCorTxnSessionDelete(ClCorTxnSessionT txnSession)
{
    ClRcT       rc = CL_OK;

    CL_FUNC_ENTER();

    if (txnSession.txnSessionId == NULL)
    { 
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    if (CL_COR_TXN_MODE_SIMPLE == txnSession.txnMode)
    {
        /* Single operation transaction */
        rc = clCorTxnJobHeaderNodeDelete(txnSession.txnSessionId);
    }
    else
    {
        ClCorTxnHandleT* pTxnHandle = (ClCorTxnHandleT *) txnSession.txnSessionId;

        clCntDelete(pTxnHandle->tree);
        clCntDelete(pTxnHandle->list);

        clHeapFree(pTxnHandle);
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Method
 *
 * This method returns cor-txn description for all operations on a object identified
 * by txnObject. If such a object does not exists, then new instance would be created.
 */
ClRcT 
clCorTxnJobForObjectGet(
    // coverity[pass_by_value]
    CL_IN     ClCorTxnObjectHandleT     txnObject, 
    CL_IN     ClCorTxnSessionT          *pTxnSession,
    CL_OUT    ClCorTxnJobHeaderNodeT*          *pJobHdrNode)
{
    ClRcT                   rc                  = CL_OK;
    ClCorTxnHandleT*        pCorTxnJobHdrList   = NULL;

    CL_FUNC_ENTER();

    /* Retrieve list of cor-txn descriptions for this cor-session */
    pCorTxnJobHdrList = (ClCorTxnHandleT *) pTxnSession->txnSessionId;

    rc = clCntDataForKeyGet(pCorTxnJobHdrList->tree, (ClCntKeyHandleT) &(txnObject), 
                            (ClCntDataHandleT *)pJobHdrNode);
    if ( (CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction-session or id. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    if (NULL == *pJobHdrNode)
    {
        ClCorTxnObjectHandleT       *pCorTxnObject;
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("No entry found for this object-handle. \
                              Creating new Cor-Txn Header"));

        rc = clCorTxnJobHeaderNodeCreate(pJobHdrNode);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create new COR-Txn Header. rc:0x%x\n", rc));
            CL_FUNC_EXIT();
            return (rc);
        }

        pCorTxnObject = (ClCorTxnObjectHandleT *)clHeapAllocate(sizeof(ClCorTxnObjectHandleT));
        if (NULL == pCorTxnObject)
        {
             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                            CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
             CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
              
             /* Delete the header */
             clCorTxnJobHeaderNodeDelete(*pJobHdrNode);
             return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        }

        memset(pCorTxnObject, 0, sizeof(ClCorTxnObjectHandleT));
        memcpy((void *)pCorTxnObject, (const void *)&txnObject, sizeof(ClCorTxnObjectHandleT));

        (*pJobHdrNode)->jobHdr.moId = txnObject.obj.moId;
        (*pJobHdrNode)->jobHdr.jobStatus = CL_COR_TXN_JOB_PASS;

#if 0        
        /* Keep this pJobHdrNode in hash-map indexed by oh */
        if (txnObject.type == CL_COR_TXN_OBJECT_MOID)
        {
            (*pJobHdrNode)->jobHdr.moId = txnObject.obj.moId;
        }
        else
        {
            rc = clCorObjectHandleToMoIdGet(txnObject.obj.oh, 
                    &((*pJobHdrNode)->jobHdr.moId), NULL);
            if (rc != CL_OK)
            {
                clLogError("COR", "TXN", "Failed to get the MoId from Object Handle. rc [0x%x]", rc);
                return rc;
            }

            pCorTxnObject->type = CL_COR_TXN_OBJECT_MOID;
        }
#endif

        rc = clCntNodeAdd(pCorTxnJobHdrList->list, (ClCntKeyHandleT) pCorTxnObject, 
                (ClCntDataHandleT) *pJobHdrNode, NULL);
        if (rc != CL_OK)
        {
            clLogError("COR", "JAD", "Failed to add the Job Header to the container. rc [0x%x]", rc);
            clCorTxnJobHeaderNodeDelete(*pJobHdrNode);
            clHeapFree(pCorTxnObject);
            return rc;
        }

        rc = clCntNodeAdd(pCorTxnJobHdrList->tree, (ClCntKeyHandleT) pCorTxnObject, 
                (ClCntDataHandleT) *pJobHdrNode, NULL);
        if (rc != CL_OK)
        {
            clLogError("COR", "JAD", "Failed to add the Job Header to the container. rc [0x%x]", rc);
            clCorTxnJobHeaderNodeDelete(*pJobHdrNode);
            clHeapFree(pCorTxnObject);
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Allocate memory for new cor-job
 *
 *  This function allocates a job. The user only needs to allocate
 * job. The freeing will be taken care of when the job is added to
 * transaction
 *
 *  @param op: operation for which job is allocated
 *         pNewJob: [OUT] allocated job is returned here.
 *
 *  @returns CL_OK  - Success<br 
 */
ClRcT     
clCorTxnJobNodeAllocate(
    CL_IN   ClCorOpsT             op,   /* operation type */
    CL_OUT  ClCorTxnObjJobNodeT* *pNewJob /* OUT */)
{
    CL_FUNC_ENTER();

    if (pNewJob == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n Null arguements \n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if(op != CL_COR_OP_CREATE &&
         op != CL_COR_OP_CREATE_AND_SET &&
            op != CL_COR_OP_SET  &&
                op != CL_COR_OP_GET  &&
                    op!= CL_COR_OP_DELETE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Improper Operation Type \n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_OP));
    }

    if ((*pNewJob = (ClCorTxnObjJobNodeT*) 
            clHeapAllocate(sizeof(ClCorTxnObjJobNodeT))) == NULL)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset( *pNewJob, 0, sizeof(ClCorTxnObjJobNodeT));
    
   /* initialize *pNewJob */
    (*pNewJob)-> job.op = op; 
    (*pNewJob)-> next = NULL;
    (*pNewJob)-> prev = NULL;

   /* initialize Attribute Path */
     clCorAttrPathInitialize(&((*pNewJob)->job.attrPath));

     /* Set the job status */
     (*pNewJob)->job.jobStatus = CL_OK;

    CL_FUNC_EXIT();
    return(CL_OK);
}


/**
 *  This function gets the Job Header from corTxnSession, with MoId as key.
 *  If not found, a new Job Header is created.
 *  
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorTxnJobHeaderNodeGet(
    CL_IN     ClCorTxnSessionT        *pTxnSession,
    // coverity[pass_by_value]
    CL_IN     ClCorTxnObjectHandleT    txnObject,
    CL_OUT    ClCorTxnJobHeaderNodeT **pJobHdrNode)
{
    ClRcT                    rc      = CL_OK;
    ClCorTxnJobHeaderNodeT  *pThis   = NULL;

    CL_FUNC_ENTER();

    if ( (NULL == pJobHdrNode) || (NULL == pTxnSession) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL job pointer"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    /* Check if, the transaction session is already initialized. */
     if(pTxnSession->txnSessionId == 0)
     {
         rc = clCorTxnNewSessionCreate(pTxnSession);
         if (CL_OK != rc)
         {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get/create session. rc:0x%x\n", rc));
            CL_FUNC_EXIT();
            return (rc);
         }
     }

    if (CL_COR_TXN_MODE_SIMPLE == pTxnSession->txnMode)
    {
        /* Single operation transaction */
        pThis = (ClCorTxnJobHeaderNodeT*) pTxnSession->txnSessionId;
        pThis->jobHdr.moId = txnObject.obj.moId;
        pThis->jobHdr.jobStatus = CL_COR_TXN_JOB_PASS;

#if 0
        /* Keep this pJobHdrNode in hash-map indexed by MOId/Oh */
        if (txnObject.type == CL_COR_TXN_OBJECT_MOID)
        {
            pThis->jobHdr.moId = txnObject.obj.moId;
        }
        else
        {
            rc = clCorObjectHandleToMoIdGet(txnObject.obj.oh, 
                    &(pThis->jobHdr.moId), NULL);
            if (rc != CL_OK)
            {
                clLogError("COR", "TXN", "Failed to get the MoId from Object Handle. rc [0x%x]", rc);
                return rc;
            }
        }
#endif

    }
    else if (CL_COR_TXN_MODE_COMPLEX == pTxnSession->txnMode)
    {
        /* Multi-operation transaction */
        rc = clCorTxnJobForObjectGet(txnObject, pTxnSession, &pThis);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get Cor-Txn Header for given object\n"));
            CL_FUNC_EXIT();
            return (rc);
        }
    }
    else
    {
        /* pTxnSession->txnMode got corrupted */
        CL_ASSERT(0);
    }

    /* Update the transaction description. */
    *pJobHdrNode = pThis;  

    CL_FUNC_EXIT();
    return(CL_OK);
}



/**
 * This function communicates with COR-Server to commit the transactions
 * identified by txnSession parameter.
 */
ClRcT _clCorTxnSessionCommit(ClCorTxnSessionT txnSession)
{
    ClRcT                       rc          = CL_OK;
    ClBufferHandleT      inMsgHandle;
    ClBufferHandleT      outMsgHandle;
    ClUint32T                   count;
    ClVersionT 					version;

    CL_COR_VERSION_SET(version);

    CL_FUNC_ENTER();

    if (CL_COR_TXN_MODE_SIMPLE == txnSession.txnMode)
    {
        ClCorTxnJobHeaderNodeT  *pJobHdrNode;

        /*
           Single operation transaction. txnSession. txnSessionId is ClCorTxnJobHeaderNodeT* 
           - Pack/Stream COR-Txn Job Header object.
           - Fill message-buffer with txnInfo and packed stream of COR-Txn Headers.
        */
        if ((rc = clBufferCreate (&inMsgHandle)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed.  rc:0x%x", rc));
            clCorTxnSessionDelete(txnSession);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        }

        if((rc = clXdrMarshallClVersionT((void *)&version, inMsgHandle, 0)) != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
           clCorTxnSessionDelete(txnSession);
           clBufferDelete(&inMsgHandle);
           CL_FUNC_EXIT();
           return (rc);
        }
        /* Simple transaction. count = 1*/
        count = 1;
        if((rc = clXdrMarshallClUint32T((void *)&count,inMsgHandle, 0)) != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
           clCorTxnSessionDelete(txnSession);
           clBufferDelete(&inMsgHandle);
           CL_FUNC_EXIT();
           return (rc);
        }

        pJobHdrNode = (ClCorTxnJobHeaderNodeT *)txnSession.txnSessionId;
 
     /* Set the source endianness */
        pJobHdrNode->jobHdr.srcArch = clBitBlByteEndianGet();
        pJobHdrNode->jobHdr.regStatus = CL_FALSE;
          
        if((rc = clCorTxnJobStreamPack(pJobHdrNode, inMsgHandle)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to pack transaction-descriptor object. rc:0x%x\n", rc));
            clCorTxnSessionDelete(txnSession);
            clBufferDelete(&inMsgHandle);
            CL_FUNC_EXIT();
            return (rc);
        }

    }
    else
    {
        /*  Note : On error conditions for complex transaction commit, the user is expected
         *  to call clCorTxnSessionFinalize, which will internally call clCorTxnSessionDelete.
         *  Hence there is no need of deleting the transaction-session here.
         */

        ClCorTxnHandleT*    pCorTxnJobHdrMap = NULL;

        /* Multi operation transaction. 
           Look up for list of ClCorTxnJobHeaderNodeT objects using txnSession.txnSessionId
         */
        pCorTxnJobHdrMap = (ClCorTxnHandleT *) txnSession.txnSessionId;

        if((rc = clCntSizeGet(pCorTxnJobHdrMap->list, &(count))) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction-session. rc:0x%x\n", rc));
            CL_FUNC_EXIT();
            return (rc);
        }
 
        if (0 == count)
        {
            CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Zero transaction jobs\n"));
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
        }

        if ((rc = clBufferCreate (&inMsgHandle))!= CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create message-buffer. rc:0x%x\n", rc));
            CL_FUNC_EXIT();
            return (rc);
        }

        if((rc = clXdrMarshallClVersionT((void *)&version, inMsgHandle, 0)) != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
           clBufferDelete(&inMsgHandle);
           CL_FUNC_EXIT();
           return (rc);
        }

        if((rc = clXdrMarshallClUint32T((void *)&count,inMsgHandle, 0)) != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
           clBufferDelete(&inMsgHandle);
           CL_FUNC_EXIT();
           return (rc);
        }

        rc = clCntWalk(pCorTxnJobHdrMap->list, _clCorTxnJobValidate, 0, 0);
        if (CL_OK != rc)
        {
            /* Transaction validation failed */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Transaction validation got failed. rc [0x%x]\n", rc));
            clBufferDelete(&inMsgHandle);
            CL_FUNC_EXIT();
            return (rc);
        }
        
        rc = clCntWalk(pCorTxnJobHdrMap->list, _clCorTxnJobPack, 
                       (ClCntArgHandleT) inMsgHandle, sizeof(ClBufferHandleT));
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to include all transaction-jobs in msg-buffer. rc:0x%x\n", rc));
            clBufferDelete(&inMsgHandle);
            CL_FUNC_EXIT();
            return (rc);
        }
    }

    /* Create output buffer - contains list of OH */
    if((rc = clBufferCreate (&outMsgHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create message-buffer. rc:0x%x\n", rc));
        if (CL_COR_TXN_MODE_SIMPLE == txnSession.txnMode)
            clCorTxnSessionDelete(txnSession);
        clBufferDelete (&inMsgHandle);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Invoke COR_TRANSACTION_OPS RMD-function at the server */
     COR_CALL_RMD_SYNC_WITH_MSG(COR_TRANSACTION_OPS, inMsgHandle, outMsgHandle, rc);
    
    /* Extract MO-Id and OH from the buffer */
      if (CL_OK == rc)
      {
         /** 
          * Extract the object-handle from the response and update the OUT parameters/cookie.
          * This is because the object creation request might be having CL_COR_INVALID_MO_INSTANCE
          * which requires the COR server to allocate instance-id.
          */
         _clCorTxnResponseProcess(txnSession, outMsgHandle);

         /* Delete the session */
         clCorTxnSessionDelete(txnSession);
      }
      else
      {
          clLogError("COR", "TXN", "Transaction RMD failed rc:0x%x", rc);
  
        /* Check whether it is a simple transaction */
        if (txnSession.txnMode == CL_COR_TXN_MODE_SIMPLE)
        {
            /* No need to store the failed jobs into the container */
            clCorTxnSessionDelete(txnSession);
        }
        else
        {
            /* Retrieve the failed jobs */
            clCorTxnRetrieveFailedJobs(txnSession, outMsgHandle);

            /* The cor client needs to call clCorTxnSessionFinalize to free the memory allocated for txnSession->txnSessionId */
        }
      }

    clBufferDelete(&inMsgHandle);
    clBufferDelete(&outMsgHandle);

    CL_FUNC_EXIT();
    return (rc);
}

/* Txn Failed jobs related functions */

ClRcT clCorTxnFailedJobCleanUp(ClCorTxnSessionIdT txnSession)
{
    ClRcT rc = CL_OK;
    ClCorTxnInfoStoreT corTxnInfoStore = {0};
    ClCntNodeHandleT node = 0;

    CL_FUNC_ENTER();
    
    if (txnSession == NULL)
    {
        clLogTrace("COR", "TXN", "Transaction Session Id is NULL.");
        return CL_OK;
    }

    corTxnInfoStore.op = CL_COR_TXN_INFO_CLEAN;
    corTxnInfoStore.txnSessionId = txnSession;
    
    if (gCorTxnInfo)
    {
        while ((rc = clCntNodeFind(gCorTxnInfo, (ClCntKeyHandleT) &corTxnInfoStore, &node)) == CL_OK)
        {
            rc = clCntNodeDelete(gCorTxnInfo, node);

            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nFailed to delete the node. rc [0x%x]\n", rc));
                CL_FUNC_EXIT();
                return rc;
            }
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nContainer not initialized. rc [0x%x]\n", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/***************************************************
 * Hash table generation logic                     *
 ***************************************************/
 
ClRcT clCorTxnInfoStoreContainerCreate()
{
    ClRcT rc = CL_OK;

    rc = clCntHashtblCreate(CL_COR_TXN_INFO_NUM_BUCKETS,
                            clCorTxnInfoStoreKeyCompFn,
                            clCorTxnInfoStoreHashGenFn,
                            clCorTxnInfoStoreDeleteFn,
                            clCorTxnInfoStoreDestroyFn,
                            CL_CNT_UNIQUE_KEY,
                            &gCorTxnInfo);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container creation failed. rc [0x%x]", rc));
        return rc;
    }
    
    return rc;
}

ClRcT clCorTxnInfoStoreContainerFinalize()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
   
    if (gCorTxnInfo)
    {
        rc = clCntDelete(gCorTxnInfo);
        if (rc != CL_OK)
        {
            /* Unable to delete the container */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nUnable to delete the container. rc [0x%x]\n", rc));
            CL_FUNC_EXIT();
            return rc;
        }

        gCorTxnInfo = 0;
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\ngCorTxnInfo container is not initialized.\n"));
    }
    
    CL_FUNC_EXIT();
    return CL_OK;
}

static ClInt32T clCorTxnInfoStoreKeyCompFn(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClCorTxnInfoStoreT *pCorTxnInfoStore1 = (ClCorTxnInfoStoreT *) key1;
    ClCorTxnInfoStoreT *pCorTxnInfoStore2 = (ClCorTxnInfoStoreT *) key2;

    ClCorTxnInfoT *pCorTxnInfo1 = &(pCorTxnInfoStore1->txnInfo);
    ClCorTxnInfoT *pCorTxnInfo2 = &(pCorTxnInfoStore2->txnInfo);
 
    if (pCorTxnInfoStore1->txnSessionId == pCorTxnInfoStore2->txnSessionId)
    {
        if ((pCorTxnInfoStore2->op == CL_COR_TXN_INFO_FIRST_GET) || (pCorTxnInfoStore2->op == CL_COR_TXN_INFO_CLEAN))
        {
            return 0;
        }
        
        if (clCorMoIdCompare(&pCorTxnInfo1->moId, &pCorTxnInfo2->moId) == 0 )
        {
            if ( ((pCorTxnInfo1->opType & CL_COR_OP_CREATE) && (pCorTxnInfo2->opType & CL_COR_OP_CREATE)) ||
                 ((pCorTxnInfo1->opType & CL_COR_OP_DELETE) && (pCorTxnInfo2->opType & CL_COR_OP_DELETE))
            )
            {
                return 0;
            }
            else /* Compare attrpath */
            {
                if (clCorAttrPathCompare(&pCorTxnInfo1->attrPath, &pCorTxnInfo2->attrPath) == 0)
                {
                    if (pCorTxnInfo1->attrId == pCorTxnInfo2->attrId)
                    {
                        return 0;
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    return 1;
                }
                
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to add the failed job : Txn-Handle mismatch."));
        return 0;
    }

}

static ClUint32T clCorTxnInfoStoreHashGenFn(ClCntKeyHandleT key)
{
    ClUint32T hashkey = 0;
    ClCorTxnInfoStoreT *pCorTxnInfoStore = (ClCorTxnInfoStoreT *) key;
    ClCorTxnSessionIdT hashval = pCorTxnInfoStore->txnSessionId;

    hashkey = (ClWordT)hashval % CL_COR_TXN_INFO_NUM_BUCKETS;
    return (hashkey);
}

static void clCorTxnInfoStoreDeleteFn(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *)userKey);
}

static void clCorTxnInfoStoreDestroyFn(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *)userKey);
}


static ClRcT clCorTxnRetrieveFailedJobs(
            ClCorTxnSessionT txnSession, 
            ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint32T jobCount = 0, cnt = 0;
    ClCorTxnJobHeaderNodeT* pCorTxnJobHdr = NULL;

    CL_FUNC_ENTER();

    if((rc = clXdrUnmarshallClUint32T(outMsgHandle, (void *)&jobCount)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to unmarshall ClUint32T  rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }                        
    
    if (jobCount == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nNo failed jobs to process. jobCount [%u]. rc [0x%x]\n", jobCount, rc));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }
    
    cnt = 0;
    while (cnt < jobCount)
    {
        if ((rc = clCorTxnJobStreamUnpack(outMsgHandle, &pCorTxnJobHdr)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nError while unmarshalling txn info. rc [0x%x]\n", rc));
            CL_FUNC_EXIT();
            return rc;        
        }

        rc = clCorTxnProcessFailedJobs(txnSession, pCorTxnJobHdr);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error while processing the failed jobs. rc [0x%x]", rc));
            clCorTxnJobHeaderNodeDelete(pCorTxnJobHdr);
            CL_FUNC_EXIT();
            return rc;
        }

        clCorTxnJobHeaderNodeDelete(pCorTxnJobHdr);
        cnt++;                          
    }

    CL_FUNC_EXIT();
    return rc;
}


/**
 * Function to add the failed jobs in the container. 
 */ 

static ClRcT _clCorTxnFailedJobCntAdd (ClCorTxnSessionT txnSession, 
                                ClCorTxnJobHeaderNodeT *pTxnJobHdrNode, 
                                ClCorTxnObjJobNodeT *pTxnObjJobNode, 
                                ClCorTxnAttrSetJobNodeT *pTxnAttrJobNode)
{
    ClRcT rc = CL_OK;
    ClCorTxnInfoStoreT *pTxnInfoStore = NULL;
    ClCorTxnInfoT txnInfo = {0};

    if ((NULL == pTxnJobHdrNode) || (NULL == pTxnObjJobNode))
    {
        clLogError("COR", "FJA", "The pointer to cor-txn job node is NULL");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    txnInfo.moId = pTxnJobHdrNode->jobHdr.moId;
    if (pTxnObjJobNode->job.op == CL_COR_OP_CREATE_AND_SET)
        txnInfo.opType = CL_COR_OP_CREATE;
    else
        txnInfo.opType = pTxnObjJobNode->job.op;

    if (pTxnAttrJobNode == NULL)
    {
        if (CL_OK != pTxnJobHdrNode->jobHdr.jobStatus)
            txnInfo.jobStatus = pTxnJobHdrNode->jobHdr.jobStatus;
        else
            txnInfo.jobStatus = pTxnObjJobNode->job.jobStatus;
    }
    else
    {
        txnInfo.jobStatus = pTxnAttrJobNode->job.jobStatus;
        txnInfo.attrId = pTxnAttrJobNode->job.attrId;
        txnInfo.attrPath = pTxnObjJobNode->job.attrPath;
    }

    pTxnInfoStore = clHeapAllocate(sizeof(ClCorTxnInfoStoreT));

    if (pTxnInfoStore == NULL)
    {
        clLogError("COR", "FJA", "Failed to allocate memory for failed-job-container key. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }

    memset(pTxnInfoStore, 0, sizeof(ClCorTxnInfoStoreT));
    
    /* Put the txnSessionId into txnInfoStore */
    pTxnInfoStore->op = CL_COR_TXN_INFO_ADD;
    pTxnInfoStore->txnSessionId = txnSession.txnSessionId;

    /* Copy the transaction information */
    memcpy(&pTxnInfoStore->txnInfo, &txnInfo, sizeof(ClCorTxnInfoT));

    rc = clCntNodeAdd(gCorTxnInfo, (ClCntKeyHandleT)pTxnInfoStore, 0, NULL);
    if (rc != CL_OK)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE)
        {
            clLogError("COR", "FJA", "Failed to add data into container. rc [0x%x]", rc);
            clHeapFree(pTxnInfoStore);
            CL_FUNC_EXIT();
            return rc;
        }
        else
        {
            clLogTrace("COR", "FJA", "Adding the duplicate data. rc [0x%x]", rc);
            clHeapFree(pTxnInfoStore);
        }
    }

    return rc;
}

/**
 * Function to processing  the failed jobs.
 */ 
static ClRcT clCorTxnProcessFailedJobs(ClCorTxnSessionT txnSession, ClCorTxnJobHeaderNodeT *pTxnJobHdrNode)
{
    ClRcT rc = CL_OK;
    ClCorTxnObjJobNodeT* pTxnObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT* pTxnAttrSetJobNode = NULL;
    ClUint32T cnta=0, cntb=0; 

    CL_FUNC_ENTER();

    if (pTxnJobHdrNode == NULL)
    {
        clLogError("COR", "FJA", "No jobs found. Header is NULL.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    cnta = pTxnJobHdrNode->jobHdr.numJobs;

    if ( (0 == cnta) || (pTxnObjJobNode = pTxnJobHdrNode->head) == NULL)
    {
        clLogError("COR", "FJA", "There are no sub-jobs added to the header. rc[0x%x]", rc);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }

    if ( CL_OK != pTxnJobHdrNode->jobHdr.jobStatus) 
    {
        if (pTxnObjJobNode->job.op == CL_COR_OP_DELETE || 
                pTxnObjJobNode->job.op == CL_COR_OP_SET)
        {
            clLogInfo("COR", "FJA", "The header node is not proper to add into the falied job container.");
            return CL_OK;
        }

        rc = _clCorTxnFailedJobCntAdd(txnSession, pTxnJobHdrNode, pTxnObjJobNode, NULL);
        if (CL_OK != rc)
        {
            clLogError("COR", "FJA", "Failed while adding the failed-job into the container. rc[0x%x]", rc);
            return rc;
        }
    }

    while ((cnta--) && pTxnObjJobNode)
    {
        if (pTxnObjJobNode->job.jobStatus != CL_OK )
        {
            if (pTxnObjJobNode->job.op != CL_COR_OP_SET)
            {
                rc = _clCorTxnFailedJobCntAdd(txnSession, pTxnJobHdrNode, pTxnObjJobNode, NULL);
                if(CL_OK != rc)
                {
                    clLogError("COR", "FJA", 
                            "Failed while adding the create/createAndSet job in the failed-job-container. rc[0x%x]", rc);
                    return rc;
                }
            }
            else
            {   
                /* Process the sub jobs */
                cntb = pTxnObjJobNode->job.numJobs;
            
                if (((pTxnAttrSetJobNode =  pTxnObjJobNode->head) == NULL) || (cntb == 0))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nAttrSet sub job not found. rc [0x%x]\n", rc));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
                }

                while ((cntb--) && pTxnAttrSetJobNode)
                {
                    if (pTxnAttrSetJobNode->job.jobStatus != CL_OK)
                    {
                        rc = _clCorTxnFailedJobCntAdd(txnSession, pTxnJobHdrNode, 
                                pTxnObjJobNode, pTxnAttrSetJobNode); 
                        if (CL_OK != rc)
                        {
                            clLogError("COR", "FJA", "Failed while adding the failed set job in the container. rc[0x%x]", rc);
                            return rc;
                        }
                    }
                    pTxnAttrSetJobNode = pTxnAttrSetJobNode->next;
                }
            }
        }

        pTxnObjJobNode = pTxnObjJobNode->next;
    }

    CL_FUNC_EXIT();
    return rc;
}

/*
 * This function is used to verify whether both the SET and DELETE jobs are 
 * part of the same transaction.
 */
static ClRcT _clCorTxnJobValidate(
        ClCntKeyHandleT key,
        ClCntDataHandleT data,
        ClCntArgHandleT userArg,
        ClUint32T len)
{
    ClRcT rc = CL_OK;
    ClCorTxnJobHeaderNodeT *pTxnJobHdrNode = NULL;
    ClInt32T cnta = 0;
    ClInt32T hasSet = 0, hasDelete = 0;
    ClCorTxnObjJobNodeT* pTxnObjJobNode = NULL;
    
    CL_FUNC_ENTER();
    
    pTxnJobHdrNode = (ClCorTxnJobHeaderNodeT *) data;
    
    if (pTxnJobHdrNode == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNo jobs found. Header is NULL.\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    cnta = pTxnJobHdrNode->jobHdr.numJobs;
    if (cnta > 1000)  /* GAS: Temporary sanity check */
    {
        clDbgPause();        
    }
    
    
    
    if ((pTxnObjJobNode = pTxnJobHdrNode->head) == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNo jobs found.\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    while ( (cnta--) && pTxnObjJobNode)
    {
        if (pTxnObjJobNode->job.op == CL_COR_OP_SET)
        {
            hasSet = 1;
        }
        else if (pTxnObjJobNode->job.op == CL_COR_OP_DELETE)
        {
            hasDelete = 1;
        }

        if ((hasSet == 1) && (hasDelete == 1))
        {
            /* Both set and delete cannot be in the same transaction for a particular object */
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid operation : Both set and delete operation for a MO cannot be in same transaction."));
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_STATE));
        }
        
        pTxnObjJobNode = pTxnObjJobNode->next;
    }

    if ((cnta != -1) || (pTxnObjJobNode != NULL))
    {
        clDbgPause();        
    }    
    /* CL_ASSERT(cnta == -1); Do the job count and node list agree */
    /*CL_ASSERT(pTxnObjJobNode == NULL); Do the job count and node list agree */    

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Method.
 * This method used with container-walk used to pack COR-Txn Header object
 * into COR-Txn Stream
 */

static  ClRcT _clCorTxnJobPack(
    CL_IN   ClCntKeyHandleT     key, 
    CL_IN   ClCntDataHandleT    data, 
    CL_IN   ClCntArgHandleT     userArg, 
    CL_IN   ClUint32T           len)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT     inMsgHandle;
    ClCorTxnJobHeaderNodeT    *pJobHdrNode;

    CL_FUNC_ENTER();

    inMsgHandle = (ClBufferHandleT) userArg;
    pJobHdrNode = (ClCorTxnJobHeaderNodeT *)data;

   /* Set the source endianness */
     pJobHdrNode->jobHdr.srcArch = clBitBlByteEndianGet();
     pJobHdrNode->jobHdr.regStatus = CL_FALSE;

    rc = clCorTxnJobStreamPack(pJobHdrNode, inMsgHandle); 
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to pack transaction-job. rc:0x%x\n", rc));
    }

    CL_FUNC_EXIT();
  return (rc);
}

/**
 * Internal Method.
 * This method used with container-walk for extracting object handles for CREATE operations
 *  in a complex transaction.
 */

static  ClRcT _clCorTxnObjHdlExtract(
    CL_IN   ClCntKeyHandleT     key, 
    CL_IN   ClCntDataHandleT    data, 
    CL_IN   ClCntArgHandleT     userArg, 
    CL_IN   ClUint32T           len)
{
    ClRcT                      rc = CL_OK;
    ClCorTxnJobHeaderNodeT    *pCorTxn = NULL;
    ClBufferHandleT            outMsgHandle;

    CL_FUNC_ENTER();

    outMsgHandle = (ClBufferHandleT) userArg;
    pCorTxn = (ClCorTxnJobHeaderNodeT *)data;

    /* Check if the job is a CREATE job only. */
    if( (pCorTxn->head->job.op == CL_COR_OP_CREATE || pCorTxn->head->job.op == CL_COR_OP_CREATE_AND_SET) && 
            pCorTxn->pReturnHdl != NULL)
    {
       VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIdl = {0};

        rc = VDECL_VER(clXdrUnmarshallClCorObjectHandleIDLT, 4, 1, 0)(outMsgHandle, (void *) &objHIdl);
        if (rc != CL_OK)
        {
            clLogError("COR", "TXN", "Failed to unmarshall ClCorObjectHandleIDLT_4_1_0. rc [0x%x]", rc);
            return rc;
        }
        
        *(pCorTxn->pReturnHdl) = objHIdl.oh;
    }
    else if (pCorTxn->head->job.op == CL_COR_OP_DELETE)
    {
        clCorObjectHandleFree((ClCorObjectHandleT *) &(pCorTxn->pReturnHdl));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Internal method
 *
 * This method processes the response from the transaction-commit to retrieve Object-handle
 * of required MO-Ids
 */
static ClRcT _clCorTxnResponseProcess(
    CL_IN   ClCorTxnSessionT        txnSession,
    CL_IN   ClBufferHandleT         outMsgHandle)
{
    ClRcT               rc          = CL_OK;
    ClCorObjectHandleT  *pReturnHdl = NULL;
    ClCorTxnJobHeaderNodeT* pCorTxn = NULL;

    CL_FUNC_ENTER();
    
    if (CL_COR_TXN_MODE_SIMPLE == txnSession.txnMode)
    {
        pCorTxn = (ClCorTxnJobHeaderNodeT *) txnSession.txnSessionId;
        pReturnHdl = pCorTxn->pReturnHdl;

        /* Check if the job is a CREATE job only. */
        if( (pCorTxn->head->job.op == CL_COR_OP_CREATE || pCorTxn->head->job.op == CL_COR_OP_CREATE_AND_SET) && 
            pReturnHdl != NULL)
        {
           VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0) objHIdl = {0};

            rc = VDECL_VER(clXdrUnmarshallClCorObjectHandleIDLT, 4, 1, 0)(outMsgHandle, (void *) &objHIdl);
            if (rc != CL_OK)
            {
                clLogError("COR", "TXN", "Failed to unmarshall ClCorObjectHandleIDLT_4_1_0. rc [0x%x]", rc);
                return rc;
            }
            
            *pReturnHdl = objHIdl.oh;
        }
        else if ((pCorTxn->head->job.op == CL_COR_OP_DELETE) && pReturnHdl)
        {
            /* Free the object handle. */
            clCorObjectHandleFree((ClCorObjectHandleT *) &pReturnHdl);
        }
    }
    else if (CL_COR_TXN_MODE_COMPLEX == txnSession.txnMode)
    {
        ClCorTxnHandleT* pCorTxnHandle = (ClCorTxnHandleT *) txnSession.txnSessionId;

        rc = clCntWalk(pCorTxnHandle->list, _clCorTxnObjHdlExtract, outMsgHandle, 0);
        if (rc != CL_OK)
        {
            clLogError("COR", "TXN", "Failed to get the object-handle. rc [0x%x]", rc);
            return rc;
        }
    }
    else /* Invalid transaction mode */
    {
        CL_ASSERT(0);        
    }
    
    CL_FUNC_EXIT();
    return (CL_OK);
}


/* Transaction related show functionality */

/*
 * op related strings:
 * need to match with the op enum.
 */
char *gJobOp[] = {
    "CL_COR_OP_RESERVED" ,
    "CL_COR_OP_CREATE" ,
    "CL_COR_OP_SET" ,
    "CL_COR_OP_DELETE"
};


/**
 *  Return operation string given the op enum
 *
 *  This function returns the operation string 
 * for the given op.
 *
 *  @param op: operation
 *
 *  @returns string representing operation.
 */
char *
clCorTxnOpToStringGet(
    CL_IN   ClCorOpsT   op)
{
    int i = 0;

    CL_FUNC_ENTER();

    while (op != 0)
    {
        i++;
        if (op & 0x1)
            break;
        op /=2;
    }

    if (i < sizeof (gJobOp)/4)
        return (&(gJobOp[i][0]));

    return ("UNKNOWN");

    CL_FUNC_EXIT();
}

/**
 *  Prints details of TXN job.
 *
 *  This function prints details of a TXN job.
 *
 *  @param trans : transaction Id.
 *         jobPtr: job handle.
 *         jobIdx: index of the job in the transaction.
 *         verbose: indicates how much detail to print.
 *
 *  @returns N/A
 */
ClRcT
clCorTxnJobShow(
    CL_IN   ClCorTxnIdT          trans, 
    CL_IN   ClCorTxnJobIdT       jobId, 
    CL_IN   void                *cookie)
{
    ClCorOpsT op;
    CL_FUNC_ENTER();

    clCorTxnJobOperationGet(trans, jobId, &op); 
    clOsalPrintf("\t\t\tOperation ......... %s\n",
                  clCorTxnOpToStringGet(op));

    switch (op)
    {
        case CL_COR_OP_CREATE:
        break;

        case CL_COR_OP_SET:
             {
              ClCorAttrPathT *pAttrPath;
              ClCorAttrIdT    attrId;
              ClInt32T        index;
              void           *pValue;
              ClUint32T       size;
    
              clCorTxnJobAttrPathGet(trans, jobId, &pAttrPath);
         
              if(pAttrPath)
                {
                   clOsalPrintf("\t\t\tAttrPath ............");
                   clCorAttrPathShow(pAttrPath);
                }
         
                 clCorTxnJobSetParamsGet(trans, jobId, &attrId, &index,
                                                  &pValue, &size);
                 clOsalPrintf("\t\t\tAttrId ............ 0x%08x\n",
                          attrId);
                 clOsalPrintf("\t\t\tValue ............. 0x%08d\n",
                         *(ClUint32T *)pValue);
                 clOsalPrintf("\t\t\tSize .............. 0x%08x\n\n",
                           size);
             }
             break;
        case CL_COR_OP_DELETE:
            break;
        default:
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "invalid operation called\n"));
            break;
    }
        
    CL_FUNC_EXIT();
    return CL_OK;
}


/**
 *  Print details of a transaction.
 *
 *  This function prints the details of a transaction given its ID.
 *
 *  @param pThis: transaction ID.
 *
 *  @returns N/A
 */
void
clCorTxnShow(
    CL_IN   ClCorTxnSessionT *pTxnSession)
{ 
    ClCorMOIdT        moId;
    ClCorServiceIdT   srvcId;
    ClRcT             rc = CL_OK;

    CL_FUNC_ENTER();

    if(pTxnSession->txnMode == CL_COR_TXN_MODE_SIMPLE)
    {
        /* Simple Txn. */
       
        clOsalPrintf("\n\t[ TxnId .......... %p ]\n", pTxnSession->txnSessionId);
        clOsalPrintf("\tNum Jobs ....... %d\n", ((ClCorTxnJobHeaderNodeT *)(pTxnSession->txnSessionId))->jobHdr.numJobs);
        clOsalPrintf("\tMOId     ....... \n");

        if((((ClCorTxnJobHeaderNodeT *)(pTxnSession->txnSessionId))->jobHdr.moId.depth) == 0)
        {
            rc = clCorObjectHandleToMoIdGet(((ClCorTxnJobHeaderNodeT *)(pTxnSession->txnSessionId))->jobHdr.oh, 
                    &moId, &srvcId);
            if (rc != CL_OK)
            {
                clLogError("COR", "DBG", "Failed to get the MoId from object handle. rc [0x%x]", rc);
                return;
            }
        }
        else 
        {
          moId = ((ClCorTxnJobHeaderNodeT *)(pTxnSession->txnSessionId))->jobHdr.moId;
        }
        clCorMoIdShow(&moId);

        /* Walk the jobs */
        clCorTxnJobWalk((ClCorTxnIdT)pTxnSession->txnSessionId, clCorTxnJobShow, 0);
    }
    else
    {
       /* Complex Txn. */
        ClCntNodeHandleT cntNodeHdl;
        ClCorTxnIdT corTxnId;
        clCntFirstNodeGet(pTxnSession->txnSessionId,  &cntNodeHdl); 
        clCntNodeUserDataGet(pTxnSession->txnSessionId, cntNodeHdl, (ClCntNodeHandleT*)&corTxnId);
 
       do{      
           clOsalPrintf("\n\t[ TxnId .......... %p ]\n", corTxnId);
           clOsalPrintf("\tNum Jobs ....... %d\n", ((ClCorTxnJobHeaderNodeT *)(corTxnId))->jobHdr.numJobs);
           clOsalPrintf("\tMOId     ....... ");
 
           if((((ClCorTxnJobHeaderNodeT *)corTxnId)->jobHdr.moId.depth) == 0 )
           {
                rc = clCorObjectHandleToMoIdGet(((ClCorTxnJobHeaderNodeT *)corTxnId)->jobHdr.oh, &moId, &srvcId);
                if (rc != CL_OK)
                {
                    clLogError("COR", "DBG", "Failed to get the MoId from object handle. rc [0x%x] ", rc);
                    return;
                }
           }
           else 
           {
              moId = ((ClCorTxnJobHeaderNodeT *)corTxnId)->jobHdr.moId;
           }

           clCorMoIdShow(&moId);

           /* Walk all the jobs */
           clCorTxnJobWalk(corTxnId, clCorTxnJobShow, 0);
         }
         while(CL_OK == clCntNextNodeGet(pTxnSession->txnSessionId, cntNodeHdl, &cntNodeHdl)  &&
               CL_OK == clCntNodeUserDataGet(pTxnSession->txnSessionId, cntNodeHdl, (ClCntNodeHandleT*)&corTxnId));
    }
   
    CL_FUNC_EXIT();
}

static ClInt32T
_clCorTxnListCompare(
    CL_IN   ClCntKeyHandleT     oh1,
    CL_IN   ClCntKeyHandleT     oh2)
{
    ClCorTxnObjectHandleT   *pTxnObject1;
    ClCorTxnObjectHandleT   *pTxnObject2;

    CL_FUNC_ENTER();

    pTxnObject1 = (ClCorTxnObjectHandleT *)oh1;
    pTxnObject2 = (ClCorTxnObjectHandleT *)oh2;

    /* Based on the content of key, do MO-Id compare or OH compare */
    if (pTxnObject1->type == pTxnObject2->type)
    {
        if (pTxnObject1->type == CL_COR_TXN_OBJECT_MOID)
        {
            if(memcmp((const void *)&(pTxnObject1->obj.moId), (const void *)&(pTxnObject2->obj.moId), 
                        sizeof(ClCorMOIdT)))
               return 1;
            else
                return 0;
        }
        else
        {
            ClUint16T size1 = 0;
            ClUint16T size2 = 0;

            clCorObjectHandleSizeGet(pTxnObject1->obj.oh, &size1);
            clCorObjectHandleSizeGet(pTxnObject2->obj.oh, &size2);

            if (size1 == size2)
            {
                if ( memcpy((void *) pTxnObject1->obj.oh, (void *) pTxnObject2->obj.oh, size1))
                    return 1;
                else
                    return 0;
            }
            else
                return 1;
        }
    }

    CL_FUNC_EXIT();
    return 1;
}

/**
 * Internal Method
 *
 * This method compares two ClCorTxnObjectHandleT based on its type.
 * Used for container key-compare requirements
 */

static ClInt32T
_clCorTxnTreeCompare(
    CL_IN   ClCntKeyHandleT     oh1,
    CL_IN   ClCntKeyHandleT     oh2)
{
    ClCorTxnObjectHandleT   *pTxnObject1;
    ClCorTxnObjectHandleT   *pTxnObject2;

    CL_FUNC_ENTER();

    pTxnObject1 = (ClCorTxnObjectHandleT *)oh1;
    pTxnObject2 = (ClCorTxnObjectHandleT *)oh2;

    /* Based on the content of key, do MO-Id compare or OH compare */
    if (pTxnObject1->type == pTxnObject2->type)
    {
        if (pTxnObject1->type == CL_COR_TXN_OBJECT_MOID)
        {
            return clCorMoIdSortCompare(&(pTxnObject1->obj.moId), &(pTxnObject2->obj.moId));
#if 0
            if(memcmp((const void *)&(pTxnObject1->obj.moId), (const void *)&(pTxnObject2->obj.moId), 
                        sizeof(ClCorMOIdT)))
               return 1;
            else
                return 0;
#endif
        }
        else
        {
            ClUint16T size1 = 0;
            ClUint16T size2 = 0;

            clCorObjectHandleSizeGet(pTxnObject1->obj.oh, &size1);
            clCorObjectHandleSizeGet(pTxnObject2->obj.oh, &size2);

            if (size1 == size2)
            {
                if ( memcpy((void *) pTxnObject1->obj.oh, (void *) pTxnObject2->obj.oh, size1))
                    return 1;
                else
                    return 0;
            }
            else
                return 1;
        }
    }

    CL_FUNC_EXIT();
    return 1;
}

/**
 * Internal Method
 *
 * This method deletes an entry of container of COR-Txn Job Header objects.
 */

static  void 
_clCorTxnJobHeaderNodeDelete(
    CL_IN       ClCntKeyHandleT     key,
    CL_IN       ClCntDataHandleT    data)
{
    clCorTxnJobHeaderNodeDelete((ClCorTxnJobHeaderNodeT *)data);
    clHeapFree((ClInt8T *)key);   /* COR-Txn Object */
}

static  void 
_clCorTxnJobHeaderNodeDestroy(
    CL_IN       ClCntKeyHandleT     key,
    CL_IN       ClCntDataHandleT    data)
{
    clCorTxnJobHeaderNodeDelete((ClCorTxnJobHeaderNodeT *)data);
    clHeapFree((ClInt8T *)key);   /* COR-Txn Object */
}

static ClRcT
corValidateDuplicateSetJob(ClCorTxnObjJobNodeT* pObjJobNode, ClCorUserSetInfoT* pUsrSetInfo)
{
    ClCorTxnAttrSetJobNodeT* pAttrSetJobNode = NULL;

    pAttrSetJobNode = pObjJobNode->head;

    while (pAttrSetJobNode != NULL)
    {
        if ((pAttrSetJobNode->job.attrId == pUsrSetInfo->attrId) && 
                ((pAttrSetJobNode->job.index == pUsrSetInfo->index) && (pAttrSetJobNode->job.size == pUsrSetInfo->size)))
        {
            /* Attribute already present in this job */
            clLogError("COR", "JAD", 
                    "SET job for this attribute [0x%x] with index [0x%x] and size [0x%x] is already present in this transaction. "
                    "Duplicate SET job addition detected. rc [0x%x]", 
                    pUsrSetInfo->attrId, pUsrSetInfo->index, pUsrSetInfo->size, CL_COR_SET_RC(CL_COR_ERR_DUPLICATE));
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
        }

        pAttrSetJobNode = pAttrSetJobNode->next;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

static ClRcT
corValidateDuplicateObjJob(ClCorTxnJobHeaderNodeT* pTxnJobHdrNode, ClCorOpsT op)
{
    ClCorTxnObjJobNodeT* pTxnObjJobNode = NULL;

    pTxnObjJobNode = pTxnJobHdrNode->head;

    while (pTxnObjJobNode != NULL)
    {
        /* Check for the duplicate obj job */
        if (pTxnObjJobNode->job.op == op)
        {
            clLogError("COR", "JAD", "Duplicate jobs with same operation detected for the same object. "
                    "Failed to add the job to the transaction. rc [0x%x]", CL_COR_SET_RC(CL_COR_ERR_DUPLICATE));
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
        }

        pTxnObjJobNode = pTxnObjJobNode->next;
    }
    
    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 *  Function to insert a Job into a transaction.
 */

ClRcT
clCorTxnObjJobNodeInsert(ClCorTxnJobHeaderNodeT* pJobHdrNode, ClCorOpsT op, void *jobData)
{
    ClRcT                 rc      = CL_OK;
    ClCorTxnObjJobNodeT*      pObjJobNode = 0;

    CL_FUNC_ENTER();
    if (pJobHdrNode == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL jobHeader passed."));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
   switch(op)
    {
        case CL_COR_OP_CREATE_AND_SET:
        case CL_COR_OP_CREATE:
        {
           /* Allocate New job.*/
              rc = clCorTxnJobNodeAllocate(op, &pObjJobNode);
              if(rc != CL_OK)
              {
                 CL_FUNC_EXIT();
                 return (rc);
              }
 
           /* Add this object job to the job-header */
              CL_COR_TXN_OBJ_JOB_ADD(pJobHdrNode, pObjJobNode);
  
          /* Update the Job Header Size. */
              pJobHdrNode->jobHdr.numJobs++;
          break;
        }

        case CL_COR_OP_DELETE:
        {
            rc = corValidateDuplicateObjJob(pJobHdrNode, CL_COR_OP_DELETE);
            if (rc != CL_OK)
            {
                clLogError("COR", "JAD", "Failed to add the DELETE job to this transaction. "
                        "Duplicate DELETE jobs detected for the same object. rc [0x%x]", rc);
                CL_FUNC_EXIT();
                return rc;
            }
            
            /* Allocate New job.*/
            rc = clCorTxnJobNodeAllocate(CL_COR_OP_DELETE, &pObjJobNode);
            if(rc != CL_OK)
            {
                CL_FUNC_EXIT();
                return (rc);
            }

            /* Add this object job to the job-header */
            CL_COR_TXN_OBJ_JOB_ADD(pJobHdrNode, pObjJobNode);
 
            /* Update the Job Header Size. */    
            pJobHdrNode->jobHdr.numJobs++;
            
            break;
       }

       case CL_COR_OP_SET:
       {
         ClUint32T    size;
         ClCorTxnAttrSetJobNodeT  *pAttrSetJobNode = NULL;
         ClCorUserSetInfoT        *pUsrSetInfo = (ClCorUserSetInfoT *)jobData;
 
         if(pUsrSetInfo == NULL)
         {
            /* Put ERROR messages */
               return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
         }
  
       /* Steps
          1) Check if Simple txn or Complex.
          2) if complex, look for ClCorTxnObjJobNodeT in jobTable with attrPath as key.
          3) if clCorTxnObjJobNodeT is not found, create a new one and Add the ClCorTxnObjJobNodeT to the txn
                and into the Job Table also.
          4) Add the Set Jobs to the ClCorTxnObjJobNodeT.
       */

                if((pJobHdrNode->objJobTblHdl &&             /* COMPLEX TXN */
                       (clCorTxnObjJobNodeGet(pJobHdrNode,
                                pUsrSetInfo->pAttrPath, &pObjJobNode) != CL_OK))
                                            ||
                               (!pJobHdrNode->objJobTblHdl))  /* SIMPLE TXN */
                {
                  /* Object Job not found. Allocating new job.*/
                   if((rc =  clCorTxnJobNodeAllocate(CL_COR_OP_SET, &pObjJobNode)) != CL_OK)
                   { 
                         CL_FUNC_EXIT();
                         return (rc);
                   }
 
                  /* Set the key, before adding to container */
                    pObjJobNode->job.attrPath = *(pUsrSetInfo->pAttrPath);
            
                  /* If Complex, make an entry to jobTable */
                   if(pJobHdrNode->objJobTblHdl)
                   {
                       if((rc = clCorTxnObjJobNodeAdd(pJobHdrNode, pObjJobNode)) != CL_OK)
                       {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                              ( "\n clCorTxnObjJobInsert: Could not add obj-job to txnJobHeader "));
                            CL_FUNC_EXIT();
                            return (rc);
                       }
                   }
                   
                 /* chain the job to jobHeader */
                    CL_COR_TXN_OBJ_JOB_ADD(pJobHdrNode, pObjJobNode);
                  
                  /* update job header size. */
                   pJobHdrNode->jobHdr.numJobs++;
                }

                if (pObjJobNode == NULL)
                {
                    clLogError("COR", "JNI", 
                            "The Object job node found NULL while adding the set job.");
                    return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                }

                /* Traverse the Obj job and check whether the attribute is already present */
                rc = corValidateDuplicateSetJob(pObjJobNode, pUsrSetInfo);
                if (rc != CL_OK)
                {
                    clLogError("COR", "JNI", 
                            "SET job for this attribute [0x%x] with index [0x%x] and size [0x%x] is already present in this transaction. "
                            "rc [0x%x]",
                            pUsrSetInfo->attrId, pUsrSetInfo->index, pUsrSetInfo->size, rc);
                    CL_FUNC_EXIT();
                    return rc;
                }

               /* Allocate Set Job information. */
                  size = sizeof(ClCorTxnAttrSetJobNodeT); /* + pUsrSetInfo->size; */
                  pAttrSetJobNode =  clHeapAllocate(size);
                  if(!pAttrSetJobNode)
                  {
	             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
	                                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                     CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
                     return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                  }

                  memset(pAttrSetJobNode, 0, size);
                 
                  pAttrSetJobNode->job.attrId = pUsrSetInfo->attrId;
                  pAttrSetJobNode->job.index = pUsrSetInfo->index;
                  pAttrSetJobNode->job.size = pUsrSetInfo->size;

               /* Allocate Set Job information. */
                  pAttrSetJobNode->job.pValue =  clHeapAllocate(pUsrSetInfo->size);
                  if(!pAttrSetJobNode->job.pValue)
                  {
	                 clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
	                                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                     CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MEM_ALLOC_FAIL));
                     return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
                  }
                  memset(pAttrSetJobNode->job.pValue, 0, pUsrSetInfo->size);

                  /* copy the actual data */
                  memcpy((void *)pAttrSetJobNode->job.pValue, 
                            (const void *)pUsrSetInfo->value,
                                            pUsrSetInfo->size);

                 /* Set the job status information */
                 pAttrSetJobNode->job.jobStatus = CL_OK;

                 /* Add pAttrSetJobNode to the pObjJobNode */
                   CL_COR_TXN_ATTR_SET_JOB_ADD(pObjJobNode, pAttrSetJobNode);
              
                /* Update the job count */    
                   pObjJobNode->job.numJobs++;
         break;
        }
       case CL_COR_OP_GET:
       {
            ClUint32T    size;
            ClCorTxnAttrSetJobNodeT  *pAttrGetJobNode = NULL;
            ClCorUserSetInfoT        *pUsrGetInfo = (ClCorUserSetInfoT *)jobData;
 
            if(pUsrGetInfo == NULL)
            {
                /* Put ERROR messages */
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }
  
            /* Steps
            1) Look for ClCorTxnObjJobNodeT in jobTable with attrPath as key.
            2) if clCorTxnJobT is not found, create a new one and Add the ClCorTxnObjJobNodeT to the txn,
               after that add the entry into jobTable.
            3) chain clCorTxnAttrSetJobT to the clCorTxnJobT.
            */

            if((pJobHdrNode->objJobTblHdl &&   
                   (clCorTxnObjJobNodeGet(pJobHdrNode,
                            pUsrGetInfo->pAttrPath, &pObjJobNode) != CL_OK)))
            {
               /* Object Job not found. Allocating new job.*/
               if((rc =  clCorTxnJobNodeAllocate(CL_COR_OP_GET, &pObjJobNode)) != CL_OK)
               { 
                     CL_FUNC_EXIT();
                     return (rc);
               }

              /* Set the key, before adding to container */
                pObjJobNode->job.attrPath = *(pUsrGetInfo->pAttrPath);
        
              /* If Complex, make an entry to jobTable */
               if(pJobHdrNode->objJobTblHdl)
               {
                   if((rc = clCorTxnObjJobNodeAdd(pJobHdrNode, pObjJobNode)) != CL_OK)
                   {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                          ( "\n clCorTxnObjJobInsert: Could not add obj-job to txnJobHeader "));
                        CL_FUNC_EXIT();
                        return (rc);
                   }
               }
               
               /* chain the job to jobHeader */
               CL_COR_TXN_OBJ_JOB_ADD(pJobHdrNode, pObjJobNode);
              
               /* update job header size. */
               pJobHdrNode->jobHdr.numJobs++;
            }

            if (pObjJobNode == NULL)
            {
                clLogError("COR", "JNI", 
                        "The Object job node found NULL while adding the get job.");
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }

            /* Allocate Set Job information. */
            size = sizeof(ClCorTxnAttrSetJobNodeT); /* + pUsrGetInfo->size; */
            pAttrGetJobNode =  clHeapAllocate(size);
            if(!pAttrGetJobNode)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
                                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
                return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
            }

            memset(pAttrGetJobNode, 0, size);
             
            pAttrGetJobNode->job.attrId = pUsrGetInfo->attrId;
            pAttrGetJobNode->job.size = pUsrGetInfo->size;
            pAttrGetJobNode->job.index = pUsrGetInfo->index;

            /* Allocate assign the pointer to this Value pointer */
            pAttrGetJobNode->job.pValue =  pUsrGetInfo->value;
            pAttrGetJobNode->job.pJobStatus = pUsrGetInfo->pJobStatus;

            /* Filling the default error status of this job. */
            if(pAttrGetJobNode->job.pJobStatus != NULL)
            *pAttrGetJobNode->job.pJobStatus = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_TIMED_OUT);
            else
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
                                CL_LOG_MESSAGE_0_NULL_ARGUMENT);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed for the status of the job."));
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }

            if(!pAttrGetJobNode->job.pValue)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName, 
                                CL_LOG_MESSAGE_0_NULL_ARGUMENT);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_NULL_PTR_STR));
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }

            /* Add pAttrSetJobNode to the pObjJobNode */
            CL_COR_BUNDLE_JOB_SORT_AND_ADD(pObjJobNode, pAttrGetJobNode);
          
            /* Update the job count */    
            pObjJobNode->job.numJobs++;
            break;
       }
       default:
       {
            rc = CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_OP);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ( "\n clCorTxnObjJobInsert: Invalid Transaction operation. rc [0x%x] ", rc));
       } 
        
    } /* end of switch */ 

  CL_FUNC_EXIT();
  return(rc);
}


/* Transaction Job  Hash Table */
#define CL_COR_TXN_JOB_TABLE_BUCKETS   16

/*
 *  AttrPath to Txn Job reference get.
 */
ClRcT clCorTxnObjJobNodeGet(ClCorTxnJobHeaderNodeT *pJobHdrNode, ClCorAttrPathT *pAttrPath, ClCorTxnObjJobNodeT **pObjJobNode)
{
     ClRcT rc = CL_OK;
     rc = clCntDataForKeyGet(pJobHdrNode->objJobTblHdl, (ClCntKeyHandleT)pAttrPath, (ClCntDataHandleT *)pObjJobNode);
    return (rc);
}

/*
 *  Add ClCorTxnObjJobNodeT entry to HASH TABLE
 */
ClRcT clCorTxnObjJobNodeAdd(ClCorTxnJobHeaderNodeT *pJobHdrNode, ClCorTxnObjJobNodeT *pObjJobNode)
{
    ClRcT rc = CL_OK;

    ClCntHandleT  objJobTblHdl = pJobHdrNode->objJobTblHdl;
    rc = clCntNodeAdd(objJobTblHdl, (ClCntKeyHandleT)&pObjJobNode->job.attrPath, (ClCntDataHandleT)pObjJobNode, NULL);

    return (rc);
}


/*
 * Hash function for txnJobTable 
 * Here hash value is determined using XOR, byte by byte on entire stream.
 */
static ClUint32T _clCorTxnObjJobTblHashFn(ClCntKeyHandleT key)
{
    /* The key is AttrPath here */
    ClCorAttrPathPtrT  pAttrPath = (ClCorAttrPathPtrT)key;
    ClUint32T       *pTmp = NULL;
    ClUint32T       hashVal = 0;
    ClUint16T       len = 0;

    if (pAttrPath == NULL)
    {
            clLogError("COR", "HFN", "Attribute path found as NULL");
            return 0; 
    }
    pTmp = (ClUint32T *)pAttrPath;
    len  = pAttrPath->depth * (sizeof(ClCorAttrIdIdxPairT)/sizeof(ClUint32T));
	
    while(len--)
    {
	    hashVal = (*pTmp++ ^ (hashVal << 1));
    }

    return (hashVal % CL_COR_TXN_JOB_TABLE_BUCKETS);
}

static void _clCorTxnObjJobTblDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
     /* The data is ClCorTxnObjJobNodeT. Just free it. */
     /*	clHeapFree((void *) userData); */
}

static ClInt32T _clCorTxnObjJobTblHashKeyCmp(ClCntKeyHandleT key1, 
			ClCntKeyHandleT key2)
{
    ClCorAttrPathPtrT pAttrPath1 = (ClCorAttrPathPtrT)key1;
    ClCorAttrPathPtrT pAttrPath2 = (ClCorAttrPathPtrT)key2;

    /* Compare the paths */
      return (clCorAttrPathCompare(pAttrPath1, pAttrPath2));
}

ClRcT clCorTxnObjJobTblCreate(ClCntHandleT *pContHandle)
{

    ClRcT rc;

    rc = clCntHashtblCreate(CL_COR_TXN_JOB_TABLE_BUCKETS,
                                                        _clCorTxnObjJobTblHashKeyCmp, 
                                                        _clCorTxnObjJobTblHashFn, 
                                                        _clCorTxnObjJobTblDelete, 
                                                        NULL, 
                                                        CL_CNT_UNIQUE_KEY,
                                                        pContHandle);
    if(CL_OK != rc)
      {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Object Job-table creation Failed rc:0x%x", rc));
      }
    return (rc);
}


/**
 * Internal function to initailze the bundle.
 */

ClRcT _clCorBundleInitialize(CL_OUT ClCorBundleHandlePtrT pHandle,
                             CL_IN  ClCorBundleConfigPtrT pBundleConfig)
{
    ClRcT                   rc = CL_OK;
    ClCorBundleInfoPtrT    pHandleInfo = NULL;
    
    if((NULL == pHandle) || (NULL == pBundleConfig))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pHandleInfo = (ClCorBundleInfoPtrT) clHeapAllocate(sizeof(ClCorBundleInfoT));
    if(pHandleInfo == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory "));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    memset(pHandleInfo, 0, sizeof(ClCorBundleInfoT));

    /* Container for storing the get jobs */
    rc = clCntRbtreeCreate (_clCorTxnTreeCompare,
                           _clCorTxnJobHeaderNodeDelete,
                           _clCorTxnJobHeaderNodeDestroy, CL_CNT_UNIQUE_KEY, 
                           (ClCntHandleT *) &(pHandleInfo->jobStoreCont));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to create the rbtree container. rc[0x%x] ", rc));
        clHeapFree(pHandleInfo);
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_INIT_FAILURE);
        CL_FUNC_EXIT();
        return rc;
    }

#if 0
    rc = clCntLlistCreate (_clCorTxnListCompare,
                           _clCorTxnJobHeaderNodeDelete,
                           _clCorTxnJobHeaderNodeDestroy, CL_CNT_UNIQUE_KEY, 
                           (ClCntHandleT *) &(pHandleInfo->jobList));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to create the list container. rc[0x%x] ", rc));
        clHeapFree(pHandleInfo);
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_INIT_FAILURE);
        CL_FUNC_EXIT();
        return rc;
    }
#endif

    pHandleInfo->bundleType = pBundleConfig->bundleType;

    clOsalMutexLock(gCorBundleMutex);

    if((rc = clCorBundleHandleStore(pHandleInfo, pHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to store the handle in the Handle Db. rc:0x%x", rc));
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_INIT_FAILURE);
        clHeapFree(pHandleInfo);
        clOsalMutexUnlock(gCorBundleMutex);
        CL_FUNC_EXIT();
        return (rc);
    }

    clOsalMutexUnlock(gCorBundleMutex);

    CL_FUNC_EXIT();
    return rc;
}



/**
 * Function to add the jobs which are part of read bundle.
 */

ClRcT 
clCorBundleJobHeaderNodeGet(
    CL_IN       ClCorBundleInfoPtrT pBundleInfo,
    // coverity[pass_by_value]
    CL_IN       ClCorTxnObjectHandleT  bundleObject,
    CL_OUT      ClCorTxnJobHeaderNodeT  **pJobHdrNode)
{
    ClRcT                       rc          = CL_OK;
    ClCntHandleT                corBundleHdrList = 0;

    CL_FUNC_ENTER();

    corBundleHdrList = pBundleInfo->jobStoreCont;

    rc = clCntDataForKeyGet(corBundleHdrList, (ClCntKeyHandleT) &(bundleObject), 
                            (ClCntDataHandleT *)pJobHdrNode);
    if ( (CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid transaction-bundle or id. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
        return (rc);
    }

    if (NULL == *pJobHdrNode)
    {
        ClCorTxnObjectHandleT       *pCorBundleObject;
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("No entry found for this object-handle. \
                              Creating new Cor-Bundle Header"));

        rc = clCorTxnJobHeaderNodeCreate(pJobHdrNode);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create new COR-Bundle Header. rc:0x%x\n", rc));
            CL_FUNC_EXIT();
            return (rc);
        }

        pCorBundleObject = (ClCorTxnObjectHandleT *)clHeapAllocate(sizeof(ClCorTxnObjectHandleT));

        if (NULL == pCorBundleObject)
        {

             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                            CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
             CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MEM_ALLOC_FAIL));
              
             /* Delete the header */
             clCorTxnJobHeaderNodeDelete(*pJobHdrNode);
             return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        }

        memset(pCorBundleObject, 0, sizeof(ClCorTxnObjectHandleT));
        memcpy((void *)pCorBundleObject, (const void *)&bundleObject, sizeof(ClCorTxnObjectHandleT));

        /* Copy the MoId */
        (*pJobHdrNode)->jobHdr.moId = bundleObject.obj.moId;

        rc = clCntNodeAdd(corBundleHdrList, (ClCntKeyHandleT) pCorBundleObject, 
                          (ClCntDataHandleT) *pJobHdrNode, NULL);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle header."));
             /* Delete the header */
             clCorTxnJobHeaderNodeDelete(*pJobHdrNode);
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return rc;
}


/**
 * Internal function to send the bundle request to server.
 */
ClRcT _clCorBundleStart ( 
            CL_IN ClCorBundleHandleT               bundleHandle,
            CL_IN const ClCorBundleCallbackPtrT    bundleCallback,
            CL_IN const ClPtrT                      userArg)
{
    ClRcT                       rc = CL_OK;
    ClBufferHandleT             inMsgHandle = 0;
    ClUint32T                   count = 0;
    ClVersionT 					version = {0};
    ClCorBundleInfoPtrT         pBundleInfo = NULL;
    ClBufferHandleT             outMsgHandle  = 0;

    CL_COR_VERSION_SET(version);

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Inside the bundle apply function. "));

    clOsalMutexLock(gCorBundleMutex);

    rc = clCorBundleHandleGet(bundleHandle, &pBundleInfo);
    if(rc != CL_OK)
    {
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        clOsalMutexUnlock(gCorBundleMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the bundle handle. rc[0x%x]", rc));
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);

    if(NULL == pBundleInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("bundle handle obtained is NULL."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(CL_COR_BUNDLE_IN_EXECUTION == pBundleInfo->bundleStatus)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Bundle in progress. Waiting for the response from server for this bundle."));
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_IN_EXECUTION);
    }

    /* Assigning the user argument */
    pBundleInfo->userArg = userArg;

    /* Assigning the function pointer to be called after completion of bundle */
    pBundleInfo->funcPtr = bundleCallback;

    clOsalMutexLock(gCorBundleMutex);

    /* Store this bundle in the handle library*/
    rc = clCorBundleHandleUpdate(bundleHandle, pBundleInfo);
    if(CL_OK != rc)
    {   
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        clOsalMutexUnlock(gCorBundleMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while storing the bundle handle in the handle Db. rc[0x%x]", rc));
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);

    if((rc = clCntSizeGet(pBundleInfo->jobStoreCont, &(count))) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (" Invalid bundle handle passed. rc:0x%x\n", rc));
        if(CL_ERR_INVALID_HANDLE == CL_GET_ERROR_CODE(rc))
            rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
        else
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        CL_FUNC_EXIT();
        return (rc);
    }

    if (0 == count)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Zero bundle jobs\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_ZERO_JOBS_BUNDLE));
    }

    if ((rc = clBufferCreate (&inMsgHandle))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create message-buffer. rc:0x%x\n", rc));
        CL_FUNC_EXIT();
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        return (rc);
    }

    if((rc = clXdrMarshallClVersionT((void *)&version, inMsgHandle, 0)) != CL_OK)
    {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
       clBufferDelete(&inMsgHandle);
       CL_FUNC_EXIT();
       rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
       return (rc);
    }

    if((rc = clXdrMarshallClHandleT((void *)&bundleHandle,inMsgHandle, 0)) != CL_OK)
    {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal Handle rc:0x%x", rc));
       clBufferDelete(&inMsgHandle);
       CL_FUNC_EXIT();
       rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
       return (rc);
    }

    if((rc = clXdrMarshallClUint32T((void *)&count,inMsgHandle, 0)) != CL_OK)
    {
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClUint32T rc:0x%x", rc));
       clBufferDelete(&inMsgHandle);
       CL_FUNC_EXIT();
       rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
       return (rc);
    }


    rc = clCntWalk(pBundleInfo->jobStoreCont, _clCorTxnJobPack, 
                   (ClCntArgHandleT) inMsgHandle, sizeof(ClBufferHandleT));
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to include all transaction-jobs in msg-buffer. rc:0x%x\n", rc));
        if(CL_ERR_INVALID_HANDLE == CL_GET_ERROR_CODE(rc))
            rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
        else
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        clBufferDelete(&inMsgHandle);
        CL_FUNC_EXIT();
        return (rc);
    }

    clOsalMutexLock(gCorBundleMutex);

    /* Changing the bundle status */
    pBundleInfo->bundleStatus = CL_COR_BUNDLE_IN_EXECUTION;

    /* Store this bundle in the handle library*/
    rc = clCorBundleHandleUpdate(bundleHandle, pBundleInfo);
    if(CL_OK != rc)
    {   
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
        clOsalMutexUnlock(gCorBundleMutex);
        clBufferDelete(&inMsgHandle);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while storing the bundle handle in the handle Db. rc[0x%x]", rc));
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);

    clLogTrace("COR", "BUN", "Starting the bundle for the bundle handle [%#llX]", bundleHandle);

    rc = clBufferCreate(&outMsgHandle);
    if(CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while allocating the message handle for sending the request to server. rc[0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        goto handleError;
    }

    if (NULL != pBundleInfo->funcPtr)
    {
        /* Invoke RMD-function at the server */
        COR_CALL_RMD_ASYNC_WITH_MSG(COR_EO_BUNDLE_OP, inMsgHandle, 
                outMsgHandle, clCorProcessResponseCB, rc);  
        if(rc != CL_OK)
        {
            clBufferDelete(&outMsgHandle);
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
            clLogError("COR", "BUN", "Failed while sending request for asynchronous bundle. rc[0x%x]", rc);
            goto handleError;
        }
    }
    else
    {
        COR_CALL_RMD_SYNC_WITH_MSG(COR_EO_BUNDLE_OP, inMsgHandle, outMsgHandle, rc);
        if (CL_OK != rc)
        {
            clBufferDelete(&outMsgHandle);
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
            clLogError("COR", "BUN", "Failed while getting the response for synchronous bundle. rc[0x%x]", rc);
            goto handleError;
        }
    
        clLogTrace("COR", "BUN", "Got the response of the sync bundle handle [%#llx]", bundleHandle);


        rc = _clCorBundleJobResponseProcess(&outMsgHandle, &pBundleInfo, NULL);
        if(CL_OK != rc)
        {
            clLogError("COR", "BUN", "Failed while processing the response from \
                    the server for the bundle hande [%#llx]. rc[0x%x]", bundleHandle, rc);
            rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
            goto handleError;
        }
    }
    
    return rc;

handleError:

    clOsalMutexLock(gCorBundleMutex);

    /* Changing the bundle status */
    pBundleInfo->bundleStatus = CL_COR_BUNDLE_INITIALIZED;

    /* Store this bundle in the handle library*/
    rc = clCorBundleHandleUpdate(bundleHandle, pBundleInfo);
    if (rc != CL_OK)
    {
        clLogError("COR", "BUN", "Failed to store the bundle handle. rc [0x%x]", rc);
        clOsalMutexUnlock(gCorBundleMutex);
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);
    
    return rc;
}

/**
 *  Function to destroy the bundle handle.
 */

ClRcT
clCorBundleHandleDestroy(CL_OUT  ClHandleT dBhandle)
{
    ClRcT   rc = CL_OK;

    rc = clHandleDestroy(gCorDbHandle, dBhandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while destroying the handle. rc[0x%x]", rc));
        return rc;
    }

    clLogTrace("COR", "BUN", "Destroyed the handle [%#llX] Successfully.", dBhandle);
    return rc;
}


/**
 *  Function to create the handle and store the bundle Info structure in its data part.
 */
ClRcT     
clCorBundleHandleStore( CL_IN ClCorBundleInfoPtrT pBundleInfo,
                         CL_OUT ClHandleT *dBHandle)
{
    ClRcT       rc = CL_OK;
    ClCorBundleInfoPtrT *tempHandle = NULL;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,(" Creating and storing the handle. "));

    rc = clHandleCreate(gCorDbHandle, sizeof(ClHandleT), dBHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the handle. rc[0x%x]", rc));
        return rc;
    }

    clLogTrace("COR", "BUN", "Got the bundle handle [%#llX] from handle library", *dBHandle);

    rc = clHandleCheckout(gCorDbHandle, *dBHandle, (void **)&tempHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing checkout of the handle. rc[0x%x]", rc));
        return rc;
    }
    
    /* Assigning the value */
    *tempHandle = pBundleInfo;
    
    rc = clHandleCheckin(gCorDbHandle, *dBHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while checking in the handle. rc[0x%x]", rc));
        return rc;   
    }
    
    return rc;
}



/**
 *  Function to store the handle.
 */
ClRcT     
clCorBundleHandleUpdate( CL_IN ClHandleT dBHandle,
                CL_IN ClCorBundleInfoPtrT pBundleInfo)
{
    ClRcT       rc = CL_OK;
    ClCorBundleInfoPtrT *tempHandle = NULL;

    clLogTrace("COR", "BUN", " Update the bundle handle [%#llX]. ", dBHandle);

    rc = clHandleCheckout(gCorDbHandle, dBHandle, (void **)&tempHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing checkout of the handle. rc[0x%x]", rc));
        return rc;
    }
    
    /* Assigning the value */
    *tempHandle = pBundleInfo;
    
    rc = clHandleCheckin(gCorDbHandle, dBHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while checking in the handle. rc[0x%x]", rc));
        return rc;   
    }
    
    return rc;
}

/**
 * Container walk callback function which will make the data pointer to NULL.
 */
ClRcT _clCorBundleUpdatePointers( ClCntKeyHandleT userKey, 
                                   ClCntDataHandleT userData,
                                   ClCntArgHandleT userArg, 
                                   ClUint32T dataLength)
{
    ClRcT                   rc              = CL_OK;
    ClCorTxnJobHeaderNodeT  *pBundleJobHdr = (ClCorTxnJobHeaderNodeT *)userData;
    ClCorTxnObjJobNodeT     *pObjJobNode    = NULL;
    ClCorTxnAttrSetJobNodeT *pAttrJobNode   = NULL;

    if(NULL == pBundleJobHdr)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while updating the local pointers. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pObjJobNode = pBundleJobHdr->head;
    while(pObjJobNode)
    {
        pAttrJobNode = pObjJobNode->head;
        while(pAttrJobNode)
        {
            /* Update the jobs. */
            pAttrJobNode->job.pValue = NULL;
            
            /* Goto next sub-job*/
            pAttrJobNode = pAttrJobNode->next;
        }
        
        /* Goto next job */
        pObjJobNode = pObjJobNode->next;
    }
    
    return rc;
}



/**
 *  Function to reset the pointers before finalizing the jobs.
 */
ClRcT
_clCorBundleResetPointer(ClCorBundleInfoPtrT pBundleInfo)
{
    ClRcT       rc = CL_OK;


    rc = clCntWalk(pBundleInfo->jobStoreCont, _clCorBundleUpdatePointers, NULL, 0);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed while updating the pointers. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}


/**
 * Function to get the bundle handle.
 */
ClRcT     
clCorBundleHandleGet( CL_IN  ClHandleT dBhandle,
                       CL_OUT ClCorBundleInfoPtrT *pBundleInfo)
{
    ClRcT       rc = CL_OK;
    ClCorBundleInfoPtrT *tempHandle = NULL;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Inside the handle get function. "));

    rc = clHandleCheckout(gCorDbHandle, dBhandle, (void **)&tempHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing checkout of the handle. rc[0x%x]", rc));
        return rc;
    }
    
    /* Assigning the actual handle */
    *pBundleInfo = *tempHandle;

    rc = clHandleCheckin(gCorDbHandle, dBhandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while checking in the handle. rc[0x%x]", rc));
        return rc;   
    }
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Got the bundle handle. "));
    return rc;
}

/**
 *  Function to finalize the bundle.
 */

ClRcT _clCorBundleFinalize (
                CL_IN ClCorBundleHandleT bundleHandle)
{
    ClRcT   rc = CL_OK;
    ClCorBundleInfoPtrT pBundleInfo = NULL;

    clOsalMutexLock(gCorBundleMutex);

    /* Getting the bundle data */
    rc = clCorBundleHandleGet(bundleHandle, &pBundleInfo);
    if( CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to get the handle from handle Db. rc[0x%x]", rc));
        clOsalMutexUnlock(gCorBundleMutex);
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_FINALIZE);
        return rc;
    }

    if(NULL == pBundleInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Bundle Info obtained is a NULL pointer. "));
        clOsalMutexUnlock(gCorBundleMutex);
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* Check if the bundle is still in progress. If so return failure. */
    if(CL_COR_BUNDLE_IN_EXECUTION == pBundleInfo->bundleStatus)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot finalizing this bundle as it is still in progress."));
        clOsalMutexUnlock(gCorBundleMutex);
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_IN_EXECUTION);
    }

    /* Delete the bundle handle from the handle library. */
    rc = clCorBundleHandleDestroy(bundleHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while destroying the handle. rc[0x%x]", rc));
        clOsalMutexUnlock(gCorBundleMutex);
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_FINALIZE);
        return rc;
    }

    clOsalMutexUnlock(gCorBundleMutex);
 
    if(NULL == pBundleInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Null pointer is obtained from the handle database. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

   
    /* Reset the pointer before deleting the jobs. */
    rc = _clCorBundleResetPointer(pBundleInfo);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing resetting the pointers. rc[0x%x]", rc));
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_FINALIZE);
        return rc;
    }


    /* Free the job container. */
    rc = clCntDelete(pBundleInfo->jobStoreCont);

    /* Free the bundle info structure. */
    if(rc == CL_OK)
        clHeapFree(pBundleInfo);
    else
        rc = CL_COR_SET_RC(CL_COR_ERR_BUNDLE_FINALIZE);

    /* Invalidate the bundle. */
    bundleHandle = CL_HANDLE_INVALID_VALUE;

    return rc;
}


/**
 *  This function is used to process the response for the bundle. This is a common function which would
 *  be used to process the response in sync as well as in async version.
 */

ClRcT _clCorBundleJobResponseProcess(ClBufferHandleT *pOutMsgHandle,
                                    ClCorBundleInfoPtrT *pBundleInfo,
                                    ClCorBundleHandlePtrT pBundleHandle)
{
    ClRcT                   rc                  = CL_OK;
    ClUint32T               count               = 0;
    ClCorTxnJobHeaderNodeT  *pJobHeaderNode     = NULL;
    ClCorBundleHandleT      bundleHandle        = 0;

    clLogTrace("COR", "BUN", "Inside the client installed function for bundle .");
    
    rc = clXdrUnmarshallClHandleT(*pOutMsgHandle, &bundleHandle);  
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while unmarshalling the handle. rc[0x%x]", rc));
        clBufferDelete(pOutMsgHandle);
        return rc;
    }

    clLogTrace("COR", "BUN", "The response for the bundle handle[%#llx] has arrived ", bundleHandle);

    if( CL_HANDLE_INVALID_VALUE == bundleHandle)
    {
        clLogError("COR", "BUN", "Invalid handle obtained. [0x%x], [0x%x]", 
                    (ClInt32T)bundleHandle, (ClInt32T)CL_HANDLE_INVALID_VALUE);
        clBufferDelete(pOutMsgHandle);
        return CL_COR_SET_RC(CL_COR_ERR_INVALID_HANDLE);
    }

    clOsalMutexLock(gCorBundleMutex);

    /* Getting the bundle data */
    rc = clCorBundleHandleGet(bundleHandle, pBundleInfo);
    if( CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed to get the information corresponding to \
                bundle handle [%#llx] from handle Database. rc[0x%x]", bundleHandle, rc);
        clBufferDelete(pOutMsgHandle);
        clOsalMutexUnlock(gCorBundleMutex);
        return rc;
    }
    
    clOsalMutexUnlock(gCorBundleMutex);

    if(NULL != pBundleHandle)
        *pBundleHandle = bundleHandle;

    rc = clXdrUnmarshallClUint32T(*pOutMsgHandle, &count);
    if(CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while unmarshalling the count for bundle [%#llx]. rc[0x%x]", 
                bundleHandle, rc);
        goto handleError;
    }

    if( 0 >= count)
    {
        clLogError("COR", "BUN", "No job got processed at the server for the bundle handle [%#llx]. ", bundleHandle);
        rc = CL_COR_SET_RC(CL_COR_ERR_ZERO_JOBS_BUNDLE);
        goto handleError;
    }

    /* Get the No. of jobs in the native job list. So after updating data or job status from the received jobs, for all the 
     * jobs which were not sent by server, reset the value and status pointers.
    */
    
    while (0 < count)
    {
        rc = clCorTxnJobStreamUnpack(*pOutMsgHandle, &pJobHeaderNode);
        if(CL_OK != rc)
        {
            clLogError("COR", "BUN", "Failed while unpacking the job obtained for bundle handle [%#llx]. rc[0x%x]", 
                    bundleHandle, rc);
            goto handleError;
        }

        /* Update the local buffers*/
        rc = clCorBundleUpdate(*pBundleInfo, pJobHeaderNode);
        if (CL_OK != rc)
        {
            clLogError("COR", "BUN", "Failed while updating the local buffers for bundle handle [%#llx]. rc[0x%x]", 
                    bundleHandle, rc);
        }

        /* Deleting the header node as its job is done. */
        rc = clCorTxnJobHeaderNodeDelete(pJobHeaderNode);
        if (CL_OK != rc)
        {
            clLogError("COR", "BUN", "Failed while freeing the header node for \
                    the bundle handle [%#llx]. rc[0x%x]", bundleHandle, rc);
            goto handleError;
        }

        /*Decrementing the count.*/
        count--;
    }

handleError:
    
    clBufferDelete(pOutMsgHandle);

    clOsalMutexLock(gCorBundleMutex);

    /* Change the status of the bundle as initialized */
    (*pBundleInfo)->bundleStatus = CL_COR_BUNDLE_INITIALIZED;

    /* Store this bundle in the handle library*/
    rc = clCorBundleHandleUpdate(bundleHandle, *pBundleInfo);
    if(CL_OK != rc)
    {   
        clLogError("COR", "BUN", "Failed while updating the handle information for the \
                bundle handle [%#llx] in the handle Database. rc[0x%x]", bundleHandle, rc);
        clOsalMutexUnlock(gCorBundleMutex);
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
    }

    clOsalMutexUnlock(gCorBundleMutex);

    return rc;
}

 
/**
 * The function which is going to clean the bundle state in the case of
 * the COR server is down due to some reason.
 */ 

ClRcT _clCorBundleStateReset( ClBufferHandleT inMsgHandle, 
                              ClCorBundleInfoPtrT *pBundleInfo,
                              ClCorBundleHandlePtrT pBundleHandle)
{
    ClRcT                   rc = CL_OK;
    ClVersionT              version = {0};
    
    if (NULL == pBundleInfo || NULL == pBundleHandle)
    {
        clLogError("COR", "BUN", "The null pointer passed for the bundle information");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clXdrUnmarshallClVersionT(inMsgHandle, &version);
    if (CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while unmarshalling the version information");
        return rc;
    }

    rc = clXdrUnmarshallClHandleT(inMsgHandle, pBundleHandle);
    if (CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while unmarshalling the bundle handle. rc[0x%x]", rc);
        return rc;
    }
     

    clLogTrace("COR", "BUN", "The bundle handle obtained is [%#llx]", *pBundleHandle);

    rc = clCorBundleHandleGet(*pBundleHandle, pBundleInfo); 
    if (CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while getting the bundle information. rc[0x%x]", rc);
        return rc;
    }

    /**
     * Reset the state of the bundle now to initialized so as to allow 
     * another bundle apply after this mishap.
     */
    clOsalMutexLock(gCorBundleMutex);

    /* Change the status of the bundle as initialized */
    (*pBundleInfo)->bundleStatus = CL_COR_BUNDLE_INITIALIZED;

    /* Store this bundle in the handle library*/
    rc = clCorBundleHandleUpdate(*pBundleHandle, *pBundleInfo);
    if(CL_OK != rc)
    {   
        clLogError("COR", "BUN", "Failed while updating the handle information for the \
                bundle handle [%#llx] in the handle Database. rc[0x%x]", *pBundleHandle, rc);
        clOsalMutexUnlock(gCorBundleMutex);
        return CL_COR_SET_RC(CL_COR_ERR_BUNDLE_APPLY_FAILURE);
    }

    clOsalMutexUnlock(gCorBundleMutex);

    return rc;
}


/**
 * The COR clients EO callback function which would be called by 
 * the server after accomodating all the bundle jobs.
 */

static void
clCorProcessResponseCB( CL_IN ClRcT rc,
                        CL_IN ClPtrT pCookie,
                        CL_IN ClBufferHandleT inMsgHandle,
                        CL_OUT ClBufferHandleT outMsgHandle)
{
    ClCorBundleHandleT     bundleHandle       = CL_HANDLE_INVALID_VALUE;
    ClCorBundleInfoPtrT    pBundleInfo        = NULL;
    
    if (CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while getting the response for bundle. rc[0x%x]", rc);
        clBufferDelete(&outMsgHandle);
        /* Function call to update the bundle information in the case of COR server unavailability. */
        rc = _clCorBundleStateReset(inMsgHandle, &pBundleInfo, &bundleHandle);
        if (CL_OK != rc)
            return;
        goto handleError;
    }

    /* This is a common function used in both sync and async version. */
    rc = _clCorBundleJobResponseProcess(&outMsgHandle, &pBundleInfo, &bundleHandle);
    if (CL_OK != rc)
    {
        clLogError("COR", "BUN", "Failed while processing the response from the server. rc[0x%x]", rc);
    }

handleError:

    if(pBundleInfo->funcPtr != NULL)
    {

        rc = pBundleInfo->funcPtr(bundleHandle, pBundleInfo->userArg );
        if(rc != CL_OK)
        {
            clLogError("COR", "BUN", "Failed while calling the bundle callback function for \
                    the bundle handle[%#llx]. rc[0x%x]", bundleHandle, rc);
            return ;
        }
    }
    
    clLogTrace("COR", "BUN", "Leaving the bundle response process function. ");

    return ;
} /* End of clCorProcessResponseCB*/
            


/**
 *  Function to update the local buffer and status of each jobs of the bundle.
 */

ClRcT
clCorBundleUpdate( CL_IN   ClCorBundleInfoPtrT    pBundleInfo,
                    CL_IN   ClCorTxnJobHeaderNodeT  *pJobHeaderNode)
{
    ClRcT                       rc                  = CL_OK;
    ClCorTxnObjJobNodeT         *pObjJobNode        = NULL, *pTempObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT     *pAttrGetJob        = NULL, *pTempAttrGetJob = NULL;
    ClCorTxnJobHeaderNodeT      *pTempJobHeader     = NULL;
    ClUint32T                   size                = -1;    
    ClCorTxnObjectHandleT       bundleObject        = {0};

    CL_FUNC_ENTER();
   
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Inside bundle Update function. "));

    if(NULL == pBundleInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The bundle handle is NULL. "));
        return rc;
    }

    bundleObject.type = CL_COR_TXN_OBJECT_MOID;
    bundleObject.obj.moId = pJobHeaderNode->jobHdr.moId;

    rc = clCntSizeGet(pBundleInfo->jobStoreCont , &size);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of container. rc[0x%x]", rc));
        return rc;
    }

    if(size <= 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No jobs in for the bundle. rc[0x%x]", rc));
        return rc;
    }


    if((rc = clCntDataForKeyGet(pBundleInfo->jobStoreCont, 
            (ClCntKeyHandleT)&bundleObject,(ClCntDataHandleT *) &pTempJobHeader)) != CL_OK)
    {
        if(CL_OK != rc && (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The job header is not present in the container. rc[0x%x]", rc));
            return rc;
        }
    }

    pObjJobNode = pJobHeaderNode->head;

    while(pObjJobNode != NULL)
    {
        /* Get the job based on the attrPath of the source job and work on that. */
        rc = clCorTxnObjJobNodeGet(pTempJobHeader, &pObjJobNode->job.attrPath, &pTempObjJobNode);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the job-node. rc0[0x%x]", rc));
            return rc;
        }

        pTempAttrGetJob = pTempObjJobNode->head;

        pAttrGetJob = pObjJobNode->head;

        rc = _clCorBundleUpdateData( pJobHeaderNode, pObjJobNode, pTempObjJobNode);
        if(rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Updating the incomplete job failed. rc[0x%x]", rc));
    
        /* Got to the next job */
        pObjJobNode = pObjJobNode->next;
    }

    CL_FUNC_EXIT();

    return rc;
}


/**
 *  Updating the incomplete jobs. 
 */

ClRcT
_clCorBundleUpdateData ( CL_IN       ClCorTxnJobHeaderNodeT  *pJobHeaderNode,
                         CL_IN       ClCorTxnObjJobNodeT     *pObjJobRecv,
                         CL_INOUT    ClCorTxnObjJobNodeT     *pObjJobNative)
{
    ClRcT                   rc = CL_OK;
    ClCorTxnAttrSetJobNodeT *pAttrJobRecv = NULL, *pAttrJobNative = NULL;


    if((pJobHeaderNode == NULL) || (pObjJobNative == NULL) || (pObjJobRecv == NULL))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pAttrJobRecv  = pObjJobRecv->head;
    pAttrJobNative = pObjJobNative->head;

    while((pAttrJobNative != NULL)) 
    {
        if((pAttrJobRecv != NULL ) && (pAttrJobNative->job.attrId == pAttrJobRecv->job.attrId))
        {
            if(pAttrJobRecv->job.jobStatus != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE ,("Adding the failed job. attrId [0x%x]. rc[0x%x]", 
                                pAttrJobRecv->job.attrId, pAttrJobRecv->job.jobStatus));

                /* Updating the jobStatus */
                *pAttrJobNative->job.pJobStatus = pAttrJobRecv->job.jobStatus;

            }        
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Filling the attribute data and jobStatus. attrId[0x%x]", pAttrJobRecv->job.attrId));

                if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
                {
                    rc = clCorObjAttrValSwap(pAttrJobRecv->job.pValue, pAttrJobRecv->job.size, 
                                pAttrJobRecv->job.attrType, pAttrJobRecv->job.arrDataType);
                    if(CL_OK != rc)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while converting the data into host format."));
                }

                if(pAttrJobNative->job.size != pAttrJobRecv->job.size)
                {  
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid size .. for attrId[0x%x]: size[passed: %d received: %d]\n", 
                                pAttrJobNative->job.attrId, pAttrJobNative->job.size, pAttrJobRecv->job.size));
                    return CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                }
                else
                    memcpy(pAttrJobNative->job.pValue, pAttrJobRecv->job.pValue, pAttrJobRecv->job.size);

                /* Resetting the pointer to the value which will be helpful while finalizing */
                *pAttrJobNative->job.pJobStatus = CL_OK;
            }

            pAttrJobRecv = pAttrJobRecv->next;
        }

        pAttrJobNative = pAttrJobNative->next;
    }
    
    return rc;
}
