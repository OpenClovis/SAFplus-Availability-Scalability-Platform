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
 * File        : clCorTxnProcess.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements txn-agent functionality in COR
 *****************************************************************************/

/* INCLUDES */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clBitApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorUtilityApi.h>
#include <clCorApi.h>
#include <clCorTxnApi.h>
#include <clXdrApi.h>
#include <clRmdApi.h>
#include <netinet/in.h>


/* Internal Headers */
//#include <clCorOHLib.h>
#include "clCorObj.h"
#include "clCorNotify.h"
#include "clCorDmProtoType.h"
#include "clCorPvt.h"
#include "clCorLog.h"
#include "clCorDeltaSave.h"
#include "clCorTxnInterface.h"

#include "clCorTxnClientIpi.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


/* externs */
extern ClRcT clCorMoIdByteSwap(ClCorMOIdPtrT this);
extern ClRcT clCorAttrPathByteSwap(ClCorAttrPathT *this);
extern ClRcT corMOTreeClass2IdxConvert(ClCorMOIdPtrT moId);
extern ClUint8T gClCorCfgAppOHMask[];
extern _ClCorServerMutexT gCorMutexes;
extern ClInt32T clCorTestBasicSizeGet(ClUint32T type);

/* globals */
static ClCntHandleT     gCorBundleData;
static  ClRcT _clCorTxnProcessJob (
        CL_IN    ClCorTxnIdT  trans,
        CL_IN    ClCorTxnJobIdT jobId,
        CL_IN    void *cookie);

static ClRcT _clCorTxnObjAttrBitsGet(
        CL_IN    ClCorTxnIdT  trans,
        CL_IN    ClUint32T jobIdx,
        CL_IN    void *cookie);
         
static ClRcT 
_clCorRegularizeGetJobs(CL_IN ClCorMOIdPtrT   pMoId, 
                        CL_IN ClCorTxnObjJobNodeT      *pObjJobNode);

static ClRcT
clCorBundleContCreateAndDataStore(CL_IN    ClTxnTransactionHandleT txnHandle, 
                                   CL_IN    ClCorBundleDataPtrT pBundleData);

static ClRcT
clCorBundleConfigAndRunTimeJobAdd(CL_IN ClCntHandleT            jobCont, 
                                   CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                                   CL_IN ClCorTxnObjJobNodeT     *pObjJobNode,
                                   CL_IN ClCorTxnAttrSetJobNodeT *pAttrGetJobNode);

static ClRcT
clCorBundleRuntimeJobAdd( CL_OUT ClCntHandleT            *pRtAttrCnt,
                           CL_IN  ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                           CL_IN  ClCorTxnObjJobNodeT     *pObjJobNode,
                           CL_IN  ClCorTxnAttrSetJobNodeT *pAttrGetJobNodeT);
static ClInt32T
clCorBundleDataKeyComp(
    CL_IN ClCntKeyHandleT key1, 
    CL_IN ClCntKeyHandleT key2);

static void
clCorBundleDataDeleteFn(
   CL_IN ClCntKeyHandleT key,
   CL_IN ClCntDataHandleT data);

static ClRcT
clCorBundleDataAdd(CL_IN ClTxnTransactionHandleT txnHandle,
                    CL_IN ClCorBundleDataPtrT pBundleStoreData);

static ClUint32T _corIsMoIdInTxnList(
        CL_IN   ClCorTxnJobListT* pTxnList,
        CL_IN   ClUint32T nodeIdx,
        CL_IN   ClCorMOIdT moId,
        CL_IN   ClCorOpsT op
        );

static ClUint32T _corIsChildMoIdInTxnList(
        CL_IN   ClCorTxnJobListT* pTxnList,
        CL_IN   ClUint32T nodeIdx,
        CL_IN   ClCorMOIdT moId,
        CL_IN   ClCorOpsT op
        );

static ClRcT _clCorTxnValidateCreateJob(
                 CL_IN ClCorTxnJobListT* pTxnList,
                 CL_IN ClUint32T         nodeIdx,
                 CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
                 CL_IN ClCorMOIdT        moId);

static ClRcT _clCorTxnValidateDeleteJob(
                 CL_IN ClCorTxnJobListT* pTxnList,
                 CL_IN ClUint32T         nodeIdx,
                 CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
                 CL_IN ClCorMOIdT        moId);

static ClRcT _clCorTxnValidateSetJob(
                 CL_IN ClCorTxnJobHeaderNodeT* pJobHdrNode,
                 CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
                 CL_OUT ClCntHandleT* pCntHandle,
                 CL_IN const ClCorAddrPtrT pCompAddr);

static ClRcT _clCorTxnValidateCreateAndSetTxn(
                CL_IN ClCorTxnJobHeaderNodeT* pJobHdrNode);

static ClRcT _corValidateAttrInTxnJob(
                ClCorTxnIdT txnId, 
                ClCorTxnJobIdT jobId, 
                ClPtrT cookie);

static ClRcT
_corValidateAttrsCB(CORHashKey_h   key, 
                 CORHashValue_h pData,
                 void *         userData,
                 ClUint32T     dataLength);
static ClRcT
_corValidateInitializedAttrsCB(CORHashKey_h   key, 
                 CORHashValue_h pData,
                 void *         userData,
                 ClUint32T     dataLength);

static ClRcT _corAllocateInstanceId(ClCorMOIdPtrT moId, ClCorInstanceIdT* instId);

static ClRcT _corPrintInvalidAttrRange(ClCorAttrTypeT attrType, ClCorAttrIdT attrId, ClCharT* moIdName, void* pValue, ClRcT rc);

/* Function to validate the object against the maximum instance. */
static ClRcT clCorObjInstanceValidate(CL_IN ClCorMOIdPtrT moId);

static ClRcT   
_clCorInitializedAttrValueValidate ( CORClass_h classH, 
                                     ClUint32T  instanceId,
                                     ClUint32T  numAttrInit, 
                                     ClCorTxnObjJobNodeT *pAttrSetJobNode,
                                     ClCorOpsT op);

/**
 *  Routine which makes data changes in COR.
 *
 *  This function goes through the jobs in the transaction and 
 * makes the data changes (create/delete/set) in the COR.
 *                                                                        
 *  @param transId     Transaction identifier to process.
 *         attrBits    [OUT]: Bit map for which attr has changes.
 *         pOp         [OUT]: Logical or of all the ops done.
 *
 *  @returns CL_OK  - Success<br>
 *           COR_MO_BUSY - The Managed Object is in use or locked state<br>
 */
ClRcT
clCorTxnProcessTxn (
    CL_IN   ClCorTxnJobHeaderNodeT   *pTxnJobHdrNode)
{
    ClRcT               rc = CL_OK;

    CL_FUNC_ENTER();

    if (pTxnJobHdrNode == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL TXN!!"));
        CL_FUNC_EXIT();
        return(CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if((rc = clCorTxnJobWalk((ClCorTxnIdT)pTxnJobHdrNode, _clCorTxnProcessJob, NULL)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "TXN job Stream Processing Failed, rc = 0x%x", rc));
    }

    CL_FUNC_EXIT();
    return(rc);
}


/**
 *  Routine to process individual job items.
 *
 *  This function process individual work items from the transaction
 *   list. It also sets the change bits as applicable.
 *                                                                        
 *  @returns CL_OK  - Success<br>
 */
static ClRcT _clCorTxnProcessJob (
        CL_IN    ClCorTxnIdT         txnId,
        CL_IN    ClCorTxnJobIdT  jobId,
        CL_IN    void               *cookie)
{
    ClRcT          rc = CL_OK;
    ClCorOpsT      op = CL_COR_OP_RESERVED;
    ClCharT        moIdStr [CL_MAX_NAME_LENGTH] = {0};
    ClCorMOIdT     moId;
    
    CL_FUNC_ENTER();

    if ((txnId == 0) || (jobId == 0))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    if((rc = clCorTxnJobOperationGet(txnId, jobId, &op)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get Opeation from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }
  
    if((rc =  clCorTxnJobMoIdGet(txnId, &moId))!= CL_OK) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get MOId from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }
    
    switch (op)
    {
    case CL_COR_OP_CREATE_AND_SET:
    case CL_COR_OP_CREATE:
        {
           clOsalMutexLock(gCorMutexes.gCorServerMutex);   

           CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nCOR_OP_CREATE called\n"));
           if (moId.svcId == CL_COR_INVALID_SRVC_ID)
           {
               CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nOperation = MO OBJ Create:"));
	           rc = _corMOObjCreate (&moId);
		   	   if(rc == CL_OK)
			   		clCorDeltaDbStore(moId, NULL, CL_COR_DELTA_MO_CREATE);	
           }
           else
           {
               CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nOperation = MSO OBJ Create:"));
               rc = _corMSOObjCreate (&moId,
                                   moId.svcId);
		   	   if(rc == CL_OK)
			   		clCorDeltaDbStore(moId, NULL, CL_COR_DELTA_MSO_CREATE);	
           }

			
            if(rc != CL_OK)
            { 
                clLogError("TXN", "CRE", 
                        "Failed to create the object [%s]. rc[0x%x]",
                        _clCorMoIdStrGet(&moId, moIdStr), rc);
            }

		    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);	
            break;
        }
    case CL_COR_OP_SET:
        {
            clOsalMutexLock(gCorMutexes.gCorServerMutex);   
            DMContObjHandle_t   dmContObjHdl;
            ClCorAttrPathT  *pAttrPath = NULL;
            ClCorAttrIdT     attrId = 0;
            ClInt32T         index = 0;
            ClUint32T        size = 0;
            void            *pValue = NULL;
              
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nCOR_OP_SET called\n"));

            rc = moId2DMHandle(&moId, &dmContObjHdl.dmObjHandle); 
            if (CL_OK != rc)
            {
                  CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get DM Object Handle \n"));
                  goto handleError;
            }
               /* Set contained AttrPath */
            if (rc == CL_OK)
            {
                if((rc = clCorTxnJobAttrPathGet(txnId, jobId, &pAttrPath)) != CL_OK)
                {
                      CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                          ( "Could not get AttrPath from txn-job, rc = 0x%x \n", rc));
                      goto handleError;
                }
                else if((rc = _clCorTxnJobSetParamsGet(0,  /* Get in network order */
                                                         txnId,
                                                         jobId,
                                                         &attrId,
                                                         &index,
                                                         &pValue,
                                                         &size))!= CL_OK)
                {
                      CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                          ( "Could not get SET paramters from txn-job, rc = 0x%x \n", rc));
                      goto handleError;
                }
                    
                dmContObjHdl.pAttrPath = pAttrPath;

                rc = dmObjectAttrSet (&dmContObjHdl,
                                          attrId,
                                          index,
                                         pValue,
                                          size);
                if(rc == CL_OK)
                        clCorDeltaDbStore(moId, pAttrPath, CL_COR_DELTA_SET);	
            }
handleError:
            if(rc != CL_OK)
                clLogError("TXN", "SET", 
                    "Failed to set the attribute [0x%x] of MO[%s]. rc[0x%x]",
                    attrId, _clCorMoIdStrGet(&moId, moIdStr), rc);

		   	   clOsalMutexUnlock(gCorMutexes.gCorServerMutex);	
        }
        break;

    case CL_COR_OP_DELETE:
        clOsalMutexLock(gCorMutexes.gCorServerMutex);   

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nCOR_OP_DELETE called\n"));
        if (moId.svcId == CL_COR_INVALID_SRVC_ID)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nOperation = MO OBJ Delete:"));
            rc = _corMOObjDelete (&moId);
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nOperation = MSO OBJ Create:"));
            rc = _corMSOObjDelete (&moId);
        }
  

	    if(rc != CL_OK)
            clLogError("TXN", "DEL", "Failed to delete the object [%s]. rc[0x%x]",
                    _clCorMoIdStrGet(&moId, moIdStr), rc);

        clOsalMutexUnlock(gCorMutexes.gCorServerMutex);	
 	    break;
    default:
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "\nInvalid operation called\n"));
        return rc;
    }
 
   CL_FUNC_EXIT();
  return (rc);
}


#ifdef REL2
/* 
 * @TEMP: Following function needs to be provided by base module.
 */
ClRcT 
_clCorObjAttrSetPropogate(ClCorObjectHandleT this, 
                        ClCorAttrIdT attrId, 
                        void * value, 
                        ClUint32T size, 
                        ClUint32T flags)
{
  return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
}
#endif

ClRcT _corAllocateInstanceId(ClCorMOIdPtrT moId, ClCorInstanceIdT* instId)
{
    ObjTreeNode_h parent = NULL;
    ClCorClassTypeT classIdx = 0;
    MArrayVector_h group = NULL;
    ClRcT ret = CL_OK;
    ClCorMOIdT moIdh;
    ClRcT rc = CL_OK;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH];

    CL_FUNC_ENTER();

    moIdh = *moId;

    if ((rc = corMOTreeClass2IdxConvert(&moIdh)) != CL_OK)
    {
        clLogError( "TXN", "INA", 
            "Invalid depth specified. Failed to convert the class IDs to class Indexes for the MoId (%s) in the MO Tree. rc [0x%x]", 
            _clCorMoIdStrGet(moId, moIdStr), CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        CL_FUNC_EXIT();
        return(rc);
    }

    /* Get the parent object */
    parent = corObjTreeFindParent(objTree, moId);
    if (parent == NULL)
    {   
        clLogError( "TXN", "INA", 
            "Failed to find the parent for the moId specified : (%s). rc [0x%x]", 
            _clCorMoIdStrGet(moId, moIdStr), CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST));
    }
    
    /* Get the class index */
    classIdx = moIdh.node[moIdh.depth-1].type;
    
    group = corVectorGet(&(parent->groups), classIdx);

    if (group == NULL)
    {
        /* Change this to DEBUG_TRACE */
        clLog(CL_LOG_SEV_INFO, "TXN", "INA", 
            "No objects have been created for the MO class (%s)", 
            _clCorMoIdStrGet(moId, moIdStr));
        CL_FUNC_EXIT();
        goto AssignFirstSlot;
    }
    
    if ((group->nodes.head != NULL) && (group->nodes.tail != NULL) && (group->navigable == MARRAY_NAVIGABLE_NODE))
    {    
        ret = corVectorWalk(&(group->nodes), _mArrayFreeNodeGet, (void *) instId);
        if (ret != CL_OK)
        {
            if (ret == CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
            {
                clLog(CL_LOG_SEV_INFO, "TXN", "INA",
                    "Found the free slot: [%u] for the MO Path (%s)", 
                    *instId, _clCorMoIdStrGet(moId, moIdStr));
                CL_FUNC_EXIT();
                return CL_OK;
            }

            clLogError( "TXN", "INA", 
                "Failed to walk the COR Vector for MoId %s. rc [0x%x]", 
                _clCorMoIdStrGet(moId, moIdStr), ret);
            CL_FUNC_EXIT();
            return (ret);
        }

        (*instId)++;
        clLog(CL_LOG_SEV_INFO, "TXN", "INA",
            "Found the free slot: [%u] for the MO Path (%s)", *instId, _clCorMoIdStrGet(moId, moIdStr));
        
        CL_FUNC_EXIT();
        return (CL_OK);
    }
    else
    {
        goto AssignFirstSlot;
    }

AssignFirstSlot:

    clLog(CL_LOG_SEV_INFO, "TXN", "INA",
        "Assigning the free slot: 0 for the MO Path (%s).", _clCorMoIdStrGet(moId, moIdStr));
    *instId = 0;
    
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * Function to validate all operations of a given cor-txn-job
 */

ClRcT   clCorTxnRegularize(CL_IN ClCorTxnJobListT*       pTxnList,
                           CL_IN  ClUint32T              nodeIdx,
                           CL_OUT ClCntHandleT           *pCntHandle,
                           CL_IN  const ClCorAddrPtrT    pCompAddr)
{
    ClRcT                     rc  = CL_OK;
    ClCorMOIdT                moId;
    ClCorTxnObjJobNodeT      *pObjJobNode = NULL, *pTempObjJobNode = NULL;
    ClCorTxnJobHeaderNodeT   *pJobHdrNode = NULL;
    ClCorInstanceIdT         instId = CL_COR_INVALID_MO_INSTANCE;
    ClCharT                  moIdStr[CL_MAX_NAME_LENGTH];
    ClCorMOClassPathT        moPath = {{0}};

    CL_FUNC_ENTER();
    
    pJobHdrNode = pTxnList->pJobHdrNode[nodeIdx];

    if (NULL == pJobHdrNode)
    {
        clLogError( "TXN", "PRE", "Null argument passed for Job-Header");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
#if 0    
        rc = _clCorObjectHandleGet(&(pJobHdrNode->jobHdr.moId), &(pJobHdrNode->jobHdr.oh));
        if (rc != CL_OK)
        {
            clLogError("TXN", "PRE", "Failed to get the object handle from the moId. rc [0x%x]", rc);

            pJobHdrNode->jobHdr.jobStatus = rc;
            if(pJobHdrNode->head->job.op == CL_COR_OP_GET)
            {
                pJobHdrNode->jobHdr.isDeleted = CL_COR_TXN_JOB_HDR_OH_INVALID;
                rc = CL_OK;
            }

            return (rc);
        }
#endif

    if (pJobHdrNode->jobHdr.moId.node[pJobHdrNode->jobHdr.moId.depth-1].instance == CL_COR_INVALID_MO_INSTANCE)
    {
        rc = _corAllocateInstanceId(&(pJobHdrNode->jobHdr.moId), &instId);
        if (rc != CL_OK)
        {            
            clLogError("TXN", "PRE", "Failed to get the free instance id. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            pJobHdrNode->jobHdr.jobStatus = rc;
            return rc;
        }

        clLogInfo("TXN", "PRE", "Free Instance Id found : [%u]", instId);

        /* Update the instance Id */
        pJobHdrNode->jobHdr.moId.node[pJobHdrNode->jobHdr.moId.depth-1].instance = instId;
    }

    moId = pJobHdrNode->jobHdr.moId;

    /* Validate the MO class path given */
    rc = clCorMoIdToMoClassPathGet(&moId, &moPath);
    if (rc != CL_OK)
    {
        clLogError("TXN", "PRE", "Failed to get the Mo class path from MoId. rc [0x%x]", rc);
        return rc;
    }

    rc = _clCorMoClassPathValidate(&moPath, moId.svcId);
    if (rc != CL_OK)
    {
        clLogError("TXN", "PRE", "MO class path doesn't exist in COR. Invalid transaction.");
        return rc;
    }

    pObjJobNode = pJobHdrNode-> head;

    while(pObjJobNode)
    {
        switch(pObjJobNode->job.op)
        {
            case CL_COR_OP_CREATE:
            case CL_COR_OP_CREATE_AND_SET:
            {
                if (pObjJobNode->job.op == CL_COR_OP_CREATE_AND_SET)
                {
                    /* Validate the Create_And_Set and then validate the Create */
                    rc = _clCorTxnValidateCreateAndSetTxn(pJobHdrNode);
                    if (rc != CL_OK)
                    {

                        clLogError("TXN", "PRE", "Failed to validate the CREATE_AND_SET \
                                job for MO[%s]. rc [0x%x]", _clCorMoIdStrGet(&moId, moIdStr), rc);
                        CL_FUNC_EXIT();
                        return rc;
                    }
                }
                
                rc = _clCorTxnValidateCreateJob(pTxnList, nodeIdx, pObjJobNode, moId);
                if (rc != CL_OK)
                {
                    clLogError("TXN", "PRE", "Failed to validate the CREATE job for MO[%s]. rc [0x%x]", 
                            _clCorMoIdStrGet(&moId, moIdStr), rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            break;

            case CL_COR_OP_DELETE:
            {
                /* Validate the delete operation */
                rc = _clCorTxnValidateDeleteJob(pTxnList, nodeIdx, pObjJobNode, moId);
                if (rc != CL_OK)
                {
                    clLogError("TXN", "PRE", "Failed to validate the DELETE job for MO[%s]. rc [0x%x]", 
                            _clCorMoIdStrGet(&moId, moIdStr), rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            break;

            case CL_COR_OP_GET:
            {
                rc = _clCorRegularizeGetJobs(&moId, pObjJobNode); 
                if(CL_OK != rc || CL_OK != pObjJobNode->job.jobStatus)
                {
                    if(rc == CL_OK)
                        rc = pObjJobNode->job.jobStatus;

                    clLogError( "BUN", "PRE",
                        "Regularization of GET job failed for MoId %s. rc [0x%x]", 
                        _clCorMoIdStrGet(&moId, moIdStr), rc);

                    /* Do not want to break the regularizatio of other jobs */
                    pJobHdrNode->jobHdr.jobStatus = CL_COR_TXN_JOB_FAIL;
                    rc = CL_OK;
                    goto nextObjJob; 
                }
            }
            break;

            case CL_COR_OP_SET:
            {        
                clLogDebug("SER", "TXN", "Received SET job, validating set job");
                /* Validate the set operation */
                rc = _clCorTxnValidateSetJob(pJobHdrNode, pObjJobNode, pCntHandle, pCompAddr);
                if (rc != CL_OK)
                {
                    clLogError( "TXN", "PRE",
                        "Failed to validate the SET job for MO [%s]. rc [0x%x]", 
                        _clCorMoIdStrGet(&moId, moIdStr), rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
            break;
            
            default:
            {
                clLogError("TXN", "PRE", "Invalid op Type found while doing pre-processing.");
                CL_FUNC_EXIT();
                return CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_OP); 
            }
        }

nextObjJob:

        if((0 == pObjJobNode->job.numJobs) && (CL_COR_OP_SET == pObjJobNode->job.op))
        {
            pTempObjJobNode = pObjJobNode;

            /* pOjbJobNode is removed from the list and pObjJobNode is set to the next node */
             CL_COR_TXN_JOB_ADJUST(pJobHdrNode, pObjJobNode);

            /* Decrement the job count. */
            pJobHdrNode->jobHdr.numJobs--;

            /* if there are no jobs left in this header, then mark it as deleted to avoid its packing. */
            if(pJobHdrNode->jobHdr.numJobs == 0)
            {
                clLogDebug("TXN", "OPE", "Job is marked deleted");
                pJobHdrNode->jobHdr.isDeleted = CL_COR_TXN_JOB_HDR_DELETED;
            }

            /* Free the job node. */
            clHeapFree(pTempObjJobNode);            
        }
        else
            pObjJobNode = pObjJobNode-> next;  /* FIFO */
    }
    
    /* This will be reached only when all the jobs have been regularized.
       So set the status to true */
    pJobHdrNode->jobHdr.regStatus = CL_TRUE;
 
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT _clCorTxnValidateCreateJob(CL_IN ClCorTxnJobListT* pTxnList,
                                 CL_IN ClUint32T         nodeIdx,
                                 CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
                                 // coverity[pass_by_value]
                                 CL_IN ClCorMOIdT        moId)
{
    ClRcT                   rc = CL_OK;
    ClCharT                 moIdStr[CL_MAX_NAME_LENGTH];
    Byte_h objBuf = NULL;

    CL_FUNC_ENTER();
    
    if ((pObjJobNode->job.op == CL_COR_OP_CREATE) || (pObjJobNode->job.op == CL_COR_OP_CREATE_AND_SET))
    {
        if (pObjJobNode->job.op == CL_COR_OP_CREATE)
        {
            ClCorClassTypeT classId = 0;
            CORClass_h classH = NULL;

            clLogTrace("TXN", "PRE", "Get the class handle from the moId. Get all \
                    the attributes and check it is having any intialized attributes. ");

            rc = _corMoIdToClassGet(&moId, &classId);
            if (rc != CL_OK)
            {
                clLogError( "TXN", "PRE", "Failed to get the class Id from moId. rc [0x%x]", rc);
                pObjJobNode->job.jobStatus = rc;
                CL_FUNC_EXIT();
                return (rc);
            }

            /* Get the class handle */
            classH = dmClassGet(classId);
            if (classH == NULL)
            {
                clLogError( "TXN", "PRE", 
                    "Failed to get the class handle from the class id [0x%x]", classId);
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
            }            
            
            rc = HASH_ITR_COOKIE(classH->attrList, _corValidateInitializedAttrsCB, NULL);
            if (rc != CL_OK)
            {
                if (rc == CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED))
                {
                    clLogError( "TXN", "PRE",
                        "Invalid operation. The class contains initialized attributes. Need to use clCorObjectCreateAndSet() API");
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                }
                else
                {
                    clLogError( "TXN", "PRE", 
                        "Failed while checking for the Initialized attributes. rc [0x%x]", rc);
                    pObjJobNode->job.jobStatus = rc;
                    CL_FUNC_EXIT();
                    return rc;
                }
            }
        }
        
        if (moId.svcId == CL_COR_INVALID_SRVC_ID)
        {
            ClCorMOIdT moIdParent;
//            ClCorObjectHandleT objParent;
            ObjTreeNode_h moClassHandle = NULL;

#if 0            
            rc = _clCorMoIdValidateAgainstOHMask(&moId);
            if(rc != CL_OK)
            {
                clLogError("TXN", "PRE", 
                        "Failed while validating the MO against the OH MASK defined in <model>/clCorConfig.xml. This needs to be changed accordingly to create more objects. rc [0x%x]", 
                        rc);
                pObjJobNode->job.jobStatus = rc;
                CL_FUNC_EXIT();
                return rc;
            }
#endif           
            moClassHandle = corMOObjGet(&moId);
            if (moClassHandle != NULL)
            {
                clLogError("TXN", "PRE", "The MO object already exists. Check the MoId being used for creating the MO");
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MO_ALREADY_PRESENT);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_INST_ERR_MO_ALREADY_PRESENT));
            }

            moIdParent = moId;
            moIdParent.depth--;

            if (moIdParent.depth >= 1)
            {
//                rc = corMOObjHandleGet(&moIdParent, &objParent);
                rc = _clCorObjectValidate(&moIdParent);
                if (rc != CL_OK) 
                {
                    clLogTrace("TXN", "PRE", "Parent MO doesn't exist in the COR Object tree. \
                       Need to check whether its creation request is present in the current transaction ");

                    if (!_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdParent, pObjJobNode->job.op))
                    {
                        clLogError("TXN", "PRE", "Can't create the object \
                                [classId : 0x%x; instance : 0x%x]; Parent MO doesn't exist", 
                                moId.node[moId.depth-1].type, moId.node[moId.depth-1].instance);
                        pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST));
                     }
                
                     clLogInfo ( "TXN","PRE", "Parent and child creation is \
                                       part of the same transaction.");
                     rc = CL_OK;
                }

                clLogTrace("TXN", "PRE", "Check whether the delete request for \
                        the parent is already in the queue"); 

                if (_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdParent, CL_COR_OP_DELETE))
                {
                    clLogError("TXN", "PRE", ("Deleting the parent and creating a child (vice versa) \
                         in the same transaction is not a valid operation. "));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
                }
            }

            /* Check whether maximum instances reached for the MO-path */
            rc = clCorObjInstanceValidate(&moId);
            if(rc != CL_OK)
            {
                clLogError("TXN", "PRE", 
                        "This MO [%s] creation would exceed the maximum instances allowed for the MO class, so returning failure. rc[0x%x]", 
                        _clCorMoIdStrGet(&moId, moIdStr), rc);
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MAX_INSTANCE);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_INST_ERR_MAX_INSTANCE));
            }
        }
        else /* Validate the Mso's */
        {
            ObjTreeNode_h moClassHandle = NULL;
            CORObject_h msoClassHandle = NULL;
            ClCorMOIdT moIdParent;

            moClassHandle = corMOObjGet(&moId);
            if (moClassHandle == NULL)
            {
                clLogTrace("TXN", "PRE", "MO object doesn't exists. Need to check whether MO \
                        object creation is present in the transaction jobs list ");

                moIdParent = moId;
                clCorMoIdServiceSet(&moIdParent, CL_COR_INVALID_SRVC_ID);
    
                if (!_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdParent, CL_COR_OP_CREATE))
                {
                    clLogError("TXN", "PRE", "As the MO Object is not present in COR as \
                            well there is no creation request for the MO in the current transaction, \
                            so can't create MSO object[%s] ", _clCorMoIdStrGet(&moId, moIdStr));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST));
                }
            }

            /* Mo is either in COR or in the queue. Check whether MO deletion is in the queue before creating the Mso */
            moIdParent = moId;
            clCorMoIdServiceSet(&moIdParent, CL_COR_INVALID_SRVC_ID);
               
            if (_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdParent, CL_COR_OP_DELETE))
            {
                clLogError("TXN", "PRE", "Deleting the MO[%s] and creating a MSO[%s] in the same \
                        transaction is not a valid operation.", _clCorMoIdStrGet(&moIdParent, moIdStr), 
                        _clCorMoIdStrGet(&moId, moIdStr));
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
            }
           
            /* Check whether MO deletion is in the queue before trying to 
             * access the Mso objects.
             */
            if (moClassHandle != NULL)
            {
                objBuf = _corMoIdToDMBufGet(moIdParent); 
                if (objBuf == NULL)
                {
                    clLogError("TXN", "PRE", "Failed to get the DM buffer from the MoId [%s]. MO object doesn't exist.",
                            _clCorMoIdStrGet(&moIdParent, moIdStr));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST);
                    CL_FUNC_EXIT();
                    return CL_COR_SET_RC(CL_COR_INST_ERR_PARENT_MO_NOT_EXIST);
                }

                if ( (*(CORInstanceHdr_h) objBuf) & CL_COR_OBJ_TXN_STATE_DELETE_INPROGRESS )
                {
                    clLogError("TXN", "PRE", "MO object [%s] deletion is in-progress. Cannot create the MSO object.",
                        _clCorMoIdStrGet(&moIdParent, moIdStr));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                    CL_FUNC_EXIT();
                    return CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                }
            }

            msoClassHandle = corMSOObjGet(moClassHandle, moId.svcId);
            if ((msoClassHandle != NULL) && (msoClassHandle->dmObjH.classId != 0))
            {
                clLogError("TXN", "PRE", "The MSO [%s] already exists in the COR object tree. \
                        Check the MOId used for creating the MSO", _clCorMoIdStrGet(&moId, moIdStr));
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MSO_ALREADY_PRESENT);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_INST_ERR_MSO_ALREADY_PRESENT));
            }
        }
    }
    else
    {
        clLogError( "TXN", "PRE", 
            "Invalid OPERATION type specified [%d]", pObjJobNode->job.op);
        pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_OP);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_OP));
    }
             
    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT _clCorTxnValidateDeleteJob(CL_IN ClCorTxnJobListT* pTxnList,
                                 CL_IN ClUint32T         nodeIdx,
                                 CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
                                 // coverity[pass_by_value]
                                 CL_IN ClCorMOIdT        moId)
{
    ClUint32T i = 0;
    ClInt32T idx = -1;
    ObjTreeNode_h obj = NULL;
    ObjTreeNode_h objChild = NULL;
    ClCorMOIdT moIdChild;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH];

    CL_FUNC_ENTER();
    
    if (moId.svcId == CL_COR_INVALID_SRVC_ID)
    {
        if ((obj = corObjTreeNodeFind(objTree, &moId)) != NULL)
        {
            ClCorMOIdT moIdMso;
            CORObject_h msoClassHandle = NULL;

             /* Go through the loop and search for child MOs in the MOTree*/
             while ((objChild = mArrayNodeNextChildGet(obj, &i, &idx)))
             {
                 /* Need to check in the current transaction job list whether the child MO is in the
                    queue for delete */
                    
                 moIdChild = moId;
                 clCorMoIdAppend(&moIdChild, objChild->id, idx);
                   
                 if (!_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdChild, pObjJobNode->job.op))
                 {
                     clLogError( "TXN", "PRE", 
                        "Can't delete the object [classId : 0x%x; instance : 0x%x]; Child MO instances present.", 
                        moId.node[moId.depth-1].type, moId.node[moId.depth-1].instance);
                     pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_CHILD_MO_EXIST);
                     CL_FUNC_EXIT();
                     return (CL_COR_SET_RC(CL_COR_INST_ERR_CHILD_MO_EXIST));
                 }
             }
     
             /* Check whether any child MO creation is part of the transaction */
             if (_corIsChildMoIdInTxnList(pTxnList, nodeIdx, moId, CL_COR_OP_CREATE))
             {
                clLogError( "TXN", "PRE", 
                    "Can't delete the object. Child MO creation is already in the queue.");
                pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
             }
             
             /* Check whether both the Mso's are deleted or the deletion request is already in the queue */
             moIdMso = moId;
             clCorMoIdServiceSet(&moIdMso, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);

             msoClassHandle = corMSOObjGet(obj, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
             if ((msoClassHandle != NULL) && (msoClassHandle->dmObjH.classId != 0)) /* Mso is present */
             {
                /* MSo present, check whether the deletion request is in the queue */
                if (!_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdMso, CL_COR_OP_DELETE))
                {
                    clLogError( "TXN", "PRE", 
                        "MSO instances present, can't delete the object %s.", _clCorMoIdStrGet(&moId, moIdStr));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MSO_EXIST);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_INST_ERR_MSO_EXIST));
                }
             }
             else
             {
                /* Check whether the Mso creation request is in the queue */
                if (_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdMso, CL_COR_OP_CREATE))
                {
                    clLogError( "TXN", "PRE",
                        "Creating Mso [%s ]and deleting Mo [%s] is not a valid transaction.", 
                        _clCorMoIdStrGet(&moIdMso, moIdStr), _clCorMoIdStrGet(&moId, moIdStr));
                    pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
                }
             }

             moIdMso = moId;
             clCorMoIdServiceSet(&moIdMso, CL_COR_SVC_ID_ALARM_MANAGEMENT);

             msoClassHandle = corMSOObjGet(obj, CL_COR_SVC_ID_ALARM_MANAGEMENT);
             if ((msoClassHandle != NULL) && (msoClassHandle->dmObjH.classId != 0))
             {
                    /* MSo present, check whether the deletion request is in the queue */ 
                    if (!_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdMso, CL_COR_OP_DELETE))
                    {
                        clLogError( "TXN", "PRE", 
                            "MSO instances present [%s], can't delete the MO object [%s].", 
                            _clCorMoIdStrGet(&moIdMso, moIdStr), _clCorMoIdStrGet(&moId, moIdStr));
                        pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MSO_EXIST);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_INST_ERR_MSO_EXIST)); 
                    }
              }
              else
              {
                    /* Check whether the creation request is in the queue */
                    if (_corIsMoIdInTxnList(pTxnList, nodeIdx, moIdMso, CL_COR_OP_CREATE))
                    {
                        clLogError( "TXN", "PRE",
                            "Creating Mso [%s] and deleting Mo [%s] is not a valid transaction.", 
                            _clCorMoIdStrGet(&moIdMso, moIdStr), _clCorMoIdStrGet(&moId, moIdStr));
                        pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED));
                    }
              }
        }
        else
        {
            clLogError( "TXN", "PRE",
                "Invalid MoId %s passed. Unable to find the node in the objTree.",
                _clCorMoIdStrGet(&moId, moIdStr));
            pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }
    }
    else /* Validate the Mso's */
    {
        ObjTreeNode_h moClassHandle = NULL;
        CORObject_h msoClassHandle = NULL;

        moClassHandle = corMOObjGet(&moId);
        if (moClassHandle == NULL)
        {
            clLogError( "TXN", "PRE",
            "Failed to remove Mso (%s). MO object handle get failed.",
            _clCorMoIdStrGet(&moId, moIdStr));
            pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_NOT_EXIST));
        }

        msoClassHandle = corMSOObjGet(moClassHandle, moId.svcId);
        if ((msoClassHandle == NULL) || (msoClassHandle->dmObjH.classId == 0))
        {
            clLogError( "TXN", "PRE",
                "Mso (%s) doesn't exist.", _clCorMoIdStrGet(&moId, moIdStr));
            pObjJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_MSO_NOT_PRESENT);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_INST_ERR_MSO_NOT_PRESENT));
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
                
ClRcT _clCorTxnValidateSetJob(
    CL_IN ClCorTxnJobHeaderNodeT* pJobHdrNode,
    CL_IN ClCorTxnObjJobNodeT* pObjJobNode,
    CL_OUT ClCntHandleT* pCntHandle,
    CL_IN const ClCorAddrPtrT pCompAddr)
{
    ClRcT rc = CL_OK;
    CORAttr_h           attrH = NULL;
    ClCorAttrIdT        attrId = 0;
    ClUint32T           size = 0;
    ClCorTxnAttrSetJobNodeT  *pAttrSetJobNode = NULL;
    ClCorTxnAttrSetJobNodeT  *pAttrTempJobNode = NULL;
    ClCorMOIdT moId = {{{0}}};
    ClCorClassTypeT classId = 0;
    CORClass_h classH = NULL;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH];
    ClUint32T       numInitAttr = 0;

    CL_FUNC_ENTER();

    moId = pJobHdrNode->jobHdr.moId;
    pAttrSetJobNode = pObjJobNode->head;
    
    while(pAttrSetJobNode)
    {
        /* 1. Check whether he is creating the MO object or MSo object.
           2. Get the classId from the moId.
           3. Get the class handle from the class id.
           4. Get the attrH from the class handle.
         */

        _clCorMoIdStrGet(&moId, moIdStr);

        rc = _corMoIdToClassGet(&moId, &classId);
        if (rc != CL_OK)
        {
            clLogError( "TXN", "SET"
                "Failed to get the class id from moId (%s). rc [0x%x]", 
                moIdStr, rc);
            pObjJobNode->job.jobStatus = 
                pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
            return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }

        /* get the class handle */
        classH = dmClassGet(classId);
        if (classH == NULL)
        {
            clLogError( "TXN", "SET",
                "Failed to get the class handle from the classId (0x%x)", classId);
            pObjJobNode->job.jobStatus = 
                pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
            return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
        }

        /* get the attr Id */
        attrId = pAttrSetJobNode->job.attrId;

        /* get the attr handle */
        attrH = dmClassAttrInfoGet(classH, &(pObjJobNode->job.attrPath), attrId);
        if (NULL == attrH)
        {
            clLogError( "TXN", "SET",
                "Attribute Id 0x%x does not exist in the class Id 0x%x", attrId, classId);
            pObjJobNode->job.jobStatus = 
                pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_NOT_PRESENT);
            return (CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_NOT_PRESENT));
        }

        size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

        /* TODO: This code needs to be tested */
        if (pJobHdrNode->head->job.op != CL_COR_OP_CREATE_AND_SET)
        {
//            ClCorObjectHandleT objH = {{0}};

            /* check whether the object exists */
//            rc = _clCorObjectHandleGet(&moId, &objH);
            rc = _clCorObjectValidate(&moId);
            if (rc != CL_OK)
            {
                clLogError( "TXN", "SET",
                    "MO doesn't exist - (%s). rc [0x%x]", 
                    moIdStr, rc);
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
                return (CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
            }
        }

        if(attrH->userFlags & CL_COR_ATTR_OPERATIONAL) 
        {
            if (!(attrH->userFlags & CL_COR_ATTR_WRITABLE))
            {
                clLogError("TXN", "SET", "Set on operational attribute [%#x] that is not writable, rc [%#x]", 
                        attrId, CL_COR_SET_RC(CL_COR_ERR_ATTR_NON_WRITABLE_SET));
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_ATTR_NON_WRITABLE_SET);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_ATTR_NON_WRITABLE_SET));
            }
        }
        if(attrH->userFlags & CL_COR_ATTR_RUNTIME)
        {
            if(attrH->userFlags & CL_COR_ATTR_CACHED)
            {
                ClCorAddrT readOI = {0};
                rc = _clCorPrimaryOIGet(&pJobHdrNode->jobHdr.moId, &readOI);
                if(rc == CL_OK)
                {
                    clLogInfo ( "TXN", "SET",
                        "Transactional SET is called for the attrId [0x%x] which is a RUNTIME attribute \
                        by the Component [0x%x:0x%x]. The Primary OI registered for this is [0x%x:0x%x]", 
                        attrH->attrId, 
                        pCompAddr->nodeAddress, pCompAddr->portId,
                        readOI.nodeAddress, readOI.portId);

                    if((readOI.portId != pCompAddr->portId) 
                           || (readOI.nodeAddress != pCompAddr->nodeAddress))
                    {
                        clLog(CL_LOG_SEV_INFO, "TXN", "SET",
                            "Transactional SET is called for the RUNTIME attribute [0x%x] from non-Primary OI.\
                            rc [0x%x]",
                            attrH->attrId, rc);
                            
                        pObjJobNode->job.jobStatus = 
                            pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_RUNTIME_CACHED_SET);
                        CL_FUNC_EXIT();
                        return CL_COR_SET_RC(CL_COR_ERR_RUNTIME_CACHED_SET);
                    }
                }
            }
            else
            {
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_RUNTIME_ATTR_WRITE);
                clLogError("SER", "SET", "Set on a runtime non-cached attribute received, attrId [%d]", attrId);
                CL_FUNC_EXIT();
                return CL_COR_SET_RC(CL_COR_ERR_RUNTIME_ATTR_WRITE);
            }
        }

        /**
         * Set operation on the containment attribute is not allowed. It is a read-only
         * attribute to the user.
         */  
        if (attrH->attrType.type == CL_COR_CONTAINMENT_ATTR)
        {
            pObjJobNode->job.jobStatus = 
                pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_INVALID_SET);
            clLogError("TXN", "SET", 
                    "The set operation is performed on the containment attribute [0x%x]",
                    attrId);
            return CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_INVALID_SET);
        }

        /*
         * NOTE: The attr SET data in Txn is raw. Which means that no endian conversion has
         * been done on it. This has to be done now, after determining the actual attribute type.
         * Format the SET data in to Network (BIG-ENDIAN) order.
         */

        /* Correct the actual attribute size.  */
        if(attrH->attrType.type != CL_COR_ARRAY_ATTR &&
                attrH->attrType.type != CL_COR_ASSOCIATION_ATTR)
        {
            /**
             * The attribute job index for the simple attribute is always 
             * CL_COR_INVALID_ATTR_IDX.
             */  
            if (pAttrSetJobNode->job.index != CL_COR_INVALID_ATTR_IDX)
            {
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
                clLogMultiline(CL_LOG_ERROR, "TXN", "SET", 
                        "The index for a simple attribute [0x%x] is invalid."
                        "It should be always CL_COR_INVALID_ATTR_IDX (-1)", 
                        attrId);
                return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
            }

            /* Check if user passed more size */
            if(size < pAttrSetJobNode->job.size)
            {
              /*If the txn source is BIG-ENDIAN machine, move the value */
                if(pJobHdrNode->jobHdr.srcArch == CL_BIT_BIG_ENDIAN)
                {
                   ClUint8T *pDataLoc;
                   pDataLoc = (ClUint8T *)(pAttrSetJobNode->job.pValue)  +
                                    (pAttrSetJobNode->job.size - size);
                  
                   /* Move the data */
                    memmove((void *)pAttrSetJobNode->job.pValue,
                              (const void *)pDataLoc, size);
                }
                pAttrSetJobNode->job.size = size;
            }
            else if(size > pAttrSetJobNode->job.size)
            {
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                clLogError( "TXN", "SET", 
                    "Invalid size specified for attr Id [0x%x]", pAttrSetJobNode->job.attrId);
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE));
            }
        }
        else
        {
            if(attrH->attrType.type == CL_COR_ARRAY_ATTR)
            {
                ClUint32T  items    = attrH->attrValue.min;
                ClUint32T  elemSize = size/items;
                ClInt32T  index    = pAttrSetJobNode->job.index;
                ClUint32T  usrSize  = pAttrSetJobNode->job.size;

                /* If the index is -1, change it to 0 for common validation */
                if (index == CL_COR_INVALID_ATTR_IDX)
                    index = 0;

                if(index < CL_COR_INVALID_ATTR_IDX || index >= items) 
                {
                    pObjJobNode->job.jobStatus = 
                        pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
                    clLogError( "TXN", "PRE", 
                            "For the array attribute [0x%x] index [%d] passed is out of range [-1 to %d]", 
                            attrId, index, (items-1));
                    return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX));
                }


                size -= (index * elemSize);

                /*can not Set, if user buffer specified is greater than size left*/
                if(usrSize > size) 
                {
                    pObjJobNode->job.jobStatus = 
                        pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                    clLogError( "TXN", "SET",
                        "Invalid size specified for the attribute [0x%x] in the MO (%s), rc [0x%x]", 
                        attrId, moIdStr, rc);
                    CL_FUNC_EXIT();
                    return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE)); 
                }
                else
                { 
                  /* if user size is lesser than  native type.
                         e.g. 2 bytes for array of type ClUint32T */
                    if(usrSize < elemSize)
                    {
                        pObjJobNode->job.jobStatus = 
                            pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                        clLogError( "TXN", "SET",
                            "Invalid size specified for the attribute [0x%x] in the MO (%s), rc [0x%x]", 
                            attrId, moIdStr, rc);
                        CL_FUNC_EXIT();
                        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE)); 
                    }

                    /* user Size has to be multiples of native type. 
                        for e.g. for a Array of data type ClUint32T,
                        6 as size should not be allowed. 6 is
                        ok, if data type ClUint8T or ClUint16T */       
                    if((usrSize)%elemSize != 0)
                    {
                        pObjJobNode->job.jobStatus = 
                            pAttrSetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                        clLogError( "TXN", "SET", 
                            "Invalid size specified for the attribute [0x%x] in the MO (%s), rc [0x%x]", 
                            attrId, moIdStr, rc);                                
                        CL_FUNC_EXIT();
                        return(CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE)); 
                    }
                }
           }
       }

        /* Swap, if txn-src is little endian machine */
        if(pJobHdrNode->jobHdr.srcArch == CL_BIT_LITTLE_ENDIAN)
        {
            rc =  clCorObjAttrValSwap(pAttrSetJobNode->job.pValue,
                    pAttrSetJobNode->job.size,
                    attrH->attrType.type,
                    attrH->attrType.u.arrType);
            if(rc != CL_OK)
            { 
                clLogError( "TXN", "SET",
                    "Failed to do byte endian conversion on the value for attrId [0x%x]. rc: [0x%x]", 
                    attrH->attrId, rc);
                pObjJobNode->job.jobStatus =    
                    pAttrSetJobNode->job.jobStatus = rc;
                CL_FUNC_EXIT();
               return (rc);
            }
        }

        /* Write the unfilled attribute type information */
        pAttrSetJobNode->job.attrType = attrH->attrType.type;
        pAttrSetJobNode->job.arrDataType = attrH->attrType.u.arrType;

        clLogTrace("TXN", "OPE", "Value range validation");
        rc = dmObjectAttrValidate(attrH, (void *)pAttrSetJobNode->job.pValue);
        if(rc != CL_OK)
        {
            pObjJobNode->job.jobStatus = 
                pAttrSetJobNode->job.jobStatus = rc;            
            _corPrintInvalidAttrRange(
                attrH->attrType.type, attrId, moIdStr, pAttrSetJobNode->job.pValue, rc);
            CL_FUNC_EXIT();
            return rc;
        }

        if (attrH->userFlags & CL_COR_ATTR_INITIALIZED)
        {
            numInitAttr++;
            clLogDebug("TXN", "SET", "Found the intialized attribute no [%d]", numInitAttr);
        }

        /* If the attribute is an operational attribute */
        if(attrH->userFlags & CL_COR_ATTR_OPERATIONAL)
        { 
            rc = clCorBundleRuntimeJobAdd(pCntHandle, pJobHdrNode, pObjJobNode, pAttrSetJobNode);
            if(rc != CL_OK)
            {
                clLogError( "TXN", "SET",
                    "Failed while adding the operational attribute [0x%x]. rc[0x%x]", attrH->attrId, rc);
                pObjJobNode->job.jobStatus = 
                    pAttrSetJobNode->job.jobStatus = rc;
                CL_FUNC_EXIT();
                return rc;
            }
            
            /* Storing the job pointer. */
            pAttrTempJobNode = pAttrSetJobNode;

            /* Macro for adjusting the job pointers. */
            CL_COR_TXN_JOB_ADJUST(pObjJobNode, pAttrSetJobNode);

            /* Freeing the memory here. */
            clHeapFree(pAttrTempJobNode->job.pValue);
            clHeapFree(pAttrTempJobNode);

            /* Need to reduce the number of jobs */
            pObjJobNode->job.numJobs--;
        }
        else
            pAttrSetJobNode = pAttrSetJobNode->next;  /* FIFO */
    }

    /* Validate the uniqueness of the value of an initailized attribute amongst all 
     * the instance present for a given class.
     */
    if (numInitAttr > 0)
    {
        DMObjHandle_t dmObj = {0};

        if (pJobHdrNode->head->job.op != CL_COR_OP_CREATE_AND_SET)
        {
            rc = moId2DMHandle(&moId, &dmObj);
            if (rc != CL_OK)
            {
                clLogError("TXN", "SET", "Failed to get DM handle from MoId. rc [0x%x]", rc);
                pJobHdrNode->jobHdr.jobStatus = rc;
                return rc;
            }
        }

        rc = _clCorInitializedAttrValueValidate(classH, dmObj.instanceId, numInitAttr, 
                pObjJobNode, pJobHdrNode->head->job.op);
        if (CL_OK != rc)
        {
            clLogError("TXN", "SET", "Failed while validating the uniqueness "
                    "of the value of intialized attribute(s). rc [0x%x]", rc);
            pJobHdrNode->jobHdr.jobStatus = rc;
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT _corPrintInvalidAttrRange(
        ClCorAttrTypeT attrType, ClCorAttrIdT attrId, ClCharT* moIdName, void* pValue, ClRcT rc)
{
    CL_FUNC_ENTER();

    if (moIdName == NULL || pValue == NULL)
    {
        clLogError( "TXN", "SET", "NULL pointer passed.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    if (attrType == CL_COR_INT8)
    {
        clLogError("TXN", "SET",
            " Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%d], rc [0x%x] ", attrId, moIdName, *(ClInt8T*) pValue,  rc);
    }
    else if (attrType == CL_COR_UINT8)
    {
        clLogError( "TXN", "SET",
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO (%s). \
            Value specified is [%u]U, rc [0x%x] \n", attrId, moIdName, *(ClUint8T*) pValue,  rc);
    }
    else if (attrType == CL_COR_INT16)
    {
        clLogError( "TXN", "SET",
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO (%s). \
            Value specified is [%d], rc [0x%x] \n", attrId, moIdName, \
            (ClInt16T) CL_BIT_N2H16(*(ClInt16T*) pValue),  rc);
    }
    else if (attrType == CL_COR_UINT16)
    {
        clLogError( "TXN", "SET",
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%u]U, rc [0x%x] \n", attrId, moIdName, 
            (ClUint16T) CL_BIT_N2H16(*(ClUint16T*) pValue),  rc);
    }
    else if (attrType == CL_COR_INT32)
    {
        clLogError( "TXN", "SET", 
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%d], rc [0x%x] \n", attrId, moIdName, 
            (ClInt32T) CL_BIT_N2H32(*(ClInt32T*) pValue),  rc);
    }
    else if (attrType == CL_COR_UINT32)
    {
        clLogError("TXN", "SET", 
            " Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%u]U, rc [0x%x] ", attrId, moIdName, 
            (ClUint32T) CL_BIT_N2H32(*(ClUint32T*) pValue),  rc);
    }
    else if (attrType == CL_COR_INT64)
    {
        clLogError( "TXN", "SET", 
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%lld], rc [0x%x] \n", attrId, moIdName, 
            (ClInt64T) CL_BIT_N2H64(*(ClInt64T*) pValue),  rc);
    }
    else if (attrType == CL_COR_UINT64)
    {
        clLogError( "TXN", "SET", 
            "Data Manager Attribute Range Validation Failed for Attr [0x%x] in MO [%s]. \
            Value specified is [%llu]U, rc [0x%x] \n", attrId, moIdName, 
            (ClUint64T) CL_BIT_N2H64(*(ClUint64T*) pValue),  rc);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}
 
ClRcT _clCorTxnValidateCreateAndSetTxn(ClCorTxnJobHeaderNodeT* pJobHdrNode)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;
    ClCorClassTypeT classId = 0;
    CORClass_h classH = NULL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH] = {0};
    
    CL_FUNC_ENTER();
    
    /* Walk through all the jobs and check whether values are provided for all the 'Initialized' attributes. */
    moId = pJobHdrNode->jobHdr.moId;
   
    rc = _corMoIdToClassGet(&moId, &classId);
    if (rc != CL_OK)
    {
        clLogError( 
            "TXN", "PRE", "Failed to get the class id from moId [%s]. rc [0x%x]", 
            _clCorMoIdStrGet(&moId, moIdName),  rc);
        pJobHdrNode->jobHdr.jobStatus = rc;
        CL_FUNC_EXIT();
        return rc;
    }

    classH = dmClassGet(classId);
    if (classH == NULL)
    {
        clLogError( "TXN", "PRE", "Failed to get the class handle for classId [0x%x]\n", classId);
        CL_FUNC_EXIT();
        pJobHdrNode->jobHdr.jobStatus = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    rc = HASH_ITR_COOKIE(classH->attrList, _corValidateAttrsCB, (ClPtrT) pJobHdrNode);
    if(rc != CL_OK)
    {
        clLogError( "TXN", "PRE", 
                "Failed while validating the values for Initialized attributes in class [0x%x]. rc [0x%x]", 
                classId, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* The callback function for walking all the attributes of the class.
 * This will check if the attribute is Initialized or not and return the corresponding error code.
 */
ClRcT
_corValidateInitializedAttrsCB(CORHashKey_h   key, 
                 CORHashValue_h pData,
                 void *         userData,
                 ClUint32T     dataLength)
{
    CORAttr_h attrH = NULL;
    
    CL_FUNC_ENTER();

    attrH = (CORAttr_h) pData;

    if (attrH->attrType.type != CL_COR_CONTAINMENT_ATTR)
    {
        if ((attrH->userFlags & CL_COR_ATTR_CONFIG) && (attrH->userFlags & CL_COR_ATTR_INITIALIZED))
        {
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_TILL_REACHED));
        }
    }
                        
    CL_FUNC_EXIT();
    return (CL_OK);
}
                        
/*
 * Callback function that will be called for all the attributes found in the class.
 * This will check if the attribute is Initialized or not and check in the transaction job
 * if the value is provided for the 'Initialized' attribute.
 */
ClRcT
_corValidateAttrsCB(CORHashKey_h   key, 
                 CORHashValue_h pData,
                 void *         userData,
                 ClUint32T     dataLength)
{
    ClRcT rc = CL_OK;
    CORAttr_h attrH = NULL;
    ClCorTxnJobHeaderNodeT* pJobHdrNode = NULL;
    ClCorUserSetInfoT usrSetInfo = {0};
    ClCorTxnJobIdT jobId = 0;
    ClCorAttrPathT attrPath = {{{0}}};
    ClCharT         moIdStr[CL_MAX_NAME_LENGTH];

    CL_FUNC_ENTER();
    
    attrH = (CORAttr_h) pData;

    if (userData == NULL)
    {
        clLogError( "TXN", "PRE", "User specified argument is NULL");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    pJobHdrNode = (ClCorTxnJobHeaderNodeT*) userData;

    if ((attrH->attrType.type != CL_COR_CONTAINMENT_ATTR) && (attrH->attrType.type != CL_COR_ASSOCIATION_ATTR))
    {
        /* Validate only if it is array or simple attribute */
        if ((attrH->userFlags & CL_COR_ATTR_CONFIG) && (attrH->userFlags & CL_COR_ATTR_INITIALIZED))
        {
            rc = clCorTxnJobWalk((ClCorTxnIdT) pJobHdrNode, _corValidateAttrInTxnJob, (void *) &(attrH->attrId));
            if (rc != CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
            {
                if (rc != CL_OK)
                {
                    /* some other error has occurred */
                    clLogError( "TXN", "PRE", 
                        "Failed to validate the Initialized attribute [0x%x]. rc [0x%x]", attrH->attrId, rc);
                    CL_FUNC_EXIT();
                    return rc;
                }
                else
                {
                    clLogError("TXN", "PRE", 
                        "The Initialized Attribute [0x%x] is not present in the attribute list of create and set of MO [%s]", attrH->attrId, _clCorMoIdStrGet(&(pJobHdrNode->jobHdr.moId), moIdStr));

                    memset(&usrSetInfo, 0, sizeof(ClCorUserSetInfoT));

                    /* Insert the dummy attrPath */
                    clCorAttrPathInitialize(&attrPath);
                    
                    usrSetInfo.pAttrPath = &attrPath;
                    usrSetInfo.attrId = attrH->attrId;
                    usrSetInfo.index = -1;
                    usrSetInfo.size = 0;
                    usrSetInfo.value = NULL;
                    
                    rc = clCorTxnObjJobNodeInsert(pJobHdrNode, CL_COR_OP_SET, &usrSetInfo);
                    if (rc != CL_OK)
                    {
                        clLogError( "TXN", "PRE", 
                            "Error while inserting the failed job information into the txn job. rc [0x%x]", rc);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                    }
                  
                    rc = clCorTxnLastJobGet((ClCorTxnIdT) pJobHdrNode, &jobId);
                    if (rc != CL_OK)
                    {
                        clLogError( "TXN", "PRE",
                            "Error while getting the last job from the txn job. rc [0x%x]", rc);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                    }
                    
                    rc = clCorTxnJobStatusSet((ClCorTxnIdT) pJobHdrNode, jobId, CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                    if (rc != CL_OK)
                    {
                        clLogError( "TXN", "PRE", 
                            "Error while setting the job status. rc [0x%x]\n", rc);
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                    }

                    pJobHdrNode->jobHdr.jobStatus = CL_OK;

                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_NOT_INITIALIZED));
                }
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT _corValidateAttrInTxnJob(ClCorTxnIdT txnId, ClCorTxnJobIdT jobId, ClPtrT cookie)
{
    ClCorAttrIdT* pAttrId = NULL;
    ClCorAttrPathPtrT pAttrPath = NULL;
    ClCorOpsT      op;
    ClCorAttrIdT attrId = 0;
    ClInt32T index = 0;
    void* pValue = NULL;
    ClUint32T size = 0;
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    pAttrId = (ClCorAttrIdT *) cookie;

    if (pAttrId == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("AttrId passed is NULL"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if((rc = clCorTxnJobOperationGet(txnId, jobId, &op)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get Opeation from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }

    if (op == CL_COR_OP_SET)
    {
        if((rc = clCorTxnJobAttrPathGet(txnId, jobId, &pAttrPath)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
              ( "Could not get AttrPath from txn-job, rc = 0x%x \n", rc));
            CL_FUNC_EXIT();
            return rc;
        }
 
        /* Validating only the simple attributes */
        if (pAttrPath->depth == 0)
        {
            /* Getting the value in network format. So it won't change the endianism of the attribute value.
             * No need to convert the endianism of the value back.
             */
            rc = _clCorTxnJobSetParamsGet(0, txnId, jobId, &attrId, &index, &pValue, &size);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Could not get SET parameter from txn-job. rc [0x%x]", rc));
                CL_FUNC_EXIT();
                return rc;
            }

            if (attrId == (*pAttrId))
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Attr Initialized: attrId [0x%x]", (*pAttrId)));
                CL_FUNC_EXIT();
                return CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
  *   This function publishes the events for a transaction stream.
  *
  */
ClRcT  clCorTxnJobEventPublish(
           CL_IN   ClCorTxnJobHeaderNodeT *pJobHdrNode)
{ 
   ClRcT                    rc = CL_OK;
   ClCorTxnJobHeaderNodeT  *pNewJobHdrNode;
   ClCorTxnObjJobNodeT     *pNewObjJobNode;
   ClInt32T                 op;
   ClCorTxnObjJobNodeT     *pObjJobNode;
   ClUint32T                numJobs;
   ClCorAttrBitsT           attrBits = {{0}};
   ClBufferHandleT   msgHdl;
   ClUint8T                *pData;
   ClUint32T                size;
   ClCorMOIdT               tmpMoId;
   ClCharT                  moIdStr[CL_MAX_NAME_LENGTH];

  /**
    *  Event has to be published for every DM Object. This DM object could be a
    *  MO or a contained Object in a MO.
    *  This is because the granularity of change event subscription is to the 
    *  level of a contained object. All the changes (SET) to the object can be
    *  accomodated in one Event publish, as we have attribute change bitmap
    *  for a object.
    *
    */

    /*
     * Steps
     *  1) Generate one jobHeader per containment operation in pTxnStream. 
     *      i.e. Split pTxnStream into as many number of newJobHeader as number of objects (DM Object)
     *            in pTxnStream.
     *  2) For every newly generated stream out of pTxnStream, get the attribute bitmap for every set 
     *      request.
     *  3) Publish the event per newJobHeader, with newJobHeader as payload.
     */

     pObjJobNode = pJobHdrNode->head;
     numJobs     = pJobHdrNode->jobHdr.numJobs;

     while(numJobs && pObjJobNode)
     {
            /* Change the op type */
            if (pObjJobNode->job.op == CL_COR_OP_CREATE_AND_SET)
            {
                pObjJobNode->job.op = CL_COR_OP_CREATE;
            }
            
            /* Get a single stream for the object-job */
            if((rc = clCorTxnJobToJobExtract(pJobHdrNode, pObjJobNode, &pNewJobHdrNode)) != CL_OK)
            {
               /* Handle Error. */
                clLogError( "TXN", "EVT",
                    "Failed to extract a object job from the transaction. rc [0x%x]", rc);
                CL_FUNC_EXIT();
                return (rc);
            }
 
            /* Attribute Bits are needed, only if it is SET operation. */
            if(pObjJobNode->job.op == CL_COR_OP_SET)
            {
               if((rc = clCorTxnJobWalk((ClCorTxnIdT)pNewJobHdrNode,
                                   _clCorTxnObjAttrBitsGet,
                                      (void *) &attrBits)) != CL_OK)
               {
                    clLogError( "TXN", "EVT",
                        "Transaction Job Walk failed to get DM Class Attribute bits. rc [0x%x]", rc);
                    clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
                    CL_FUNC_EXIT();
                    return(rc);
               }
            }
 
            if((rc = clBufferCreate(&msgHdl)) != CL_OK)
            { 
                 clLogError( "TXN", "EVT", "Buffer Message Creation Failed. rc: 0x%x", rc);
                 clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
                 CL_FUNC_EXIT();
                 return (rc);
            }

            /* Pack the job into a buffer. */
            if((rc = clCorTxnJobStreamPack(pNewJobHdrNode, msgHdl)) != CL_OK)
            {
                 clLogError( "TXN", "EVT", "Failed to pack the transaction job. rc: 0x%x", rc);
                 clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
                 clBufferDelete(&msgHdl);
                 CL_FUNC_EXIT();
                 return (rc); 
            }
            
            if((rc = clBufferLengthGet(msgHdl, &size)) != CL_OK)
            {
                clLogError( "TXN", "EVT", "Failed to get the buffer length. rc: 0x%x", rc);
                clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
                clBufferDelete(&msgHdl);
                CL_FUNC_EXIT();
                return (rc);
            }

            if((rc = clBufferFlatten(msgHdl, &pData)) != CL_OK)
            {
                clLogError( "TXN", "EVT", "Failed to flatten the buffer message. rc: 0x%x", rc);
                clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
                clHeapFree(pData);
                clBufferDelete(&msgHdl);
                CL_FUNC_EXIT();
                return (rc);
            }

            /* Copy the MoId temporarily for debug messages */
            tmpMoId = pNewJobHdrNode->jobHdr.moId;

            pNewObjJobNode = pNewJobHdrNode->head;
            op             = pNewObjJobNode->job.op;

          /* All the data in publish has to be in Network Order. */
            if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
            { 
               op = CL_BIT_SWAP32(op);
               clCorMoIdByteSwap(&pNewJobHdrNode->jobHdr.moId);
               clCorAttrPathByteSwap(&pNewObjJobNode->job.attrPath);
            }
 
           /* Now publish the event. */ 
           if((rc = corEventPublish (op,
                                    &pNewJobHdrNode->jobHdr.moId,
                                       &pNewObjJobNode->job.attrPath,
                                         &attrBits,
                                           pData,
                                            size)) != CL_OK)
           {
                clLogError( "TXN", "EVT", "Failed to publish event for a obj-job of MO %s. rc 0x%x", 
                    _clCorMoIdStrGet(&tmpMoId, moIdStr), rc);
           }

           clLog(CL_LOG_SEV_INFO, "TXN", "EVT",
                    "Event published for a job of MO %s successfully", _clCorMoIdStrGet(&tmpMoId, moIdStr));

           clCorTxnJobHeaderNodeDelete(pNewJobHdrNode);
           clHeapFree(pData);
           clBufferDelete(&msgHdl);

           pObjJobNode = pObjJobNode->next;
           --numJobs;
    }
     
    return (rc);
}

/**
 *  This function updates the Attribute bits, for SET requests.
 *  These bits are required for event publish.
 *                                                                        
 *  @returns CL_OK  - Success<br>
 */
static ClRcT _clCorTxnObjAttrBitsGet (
        CL_IN    ClCorTxnIdT         txnId,
        CL_IN    ClCorTxnJobIdT      jobId,
        CL_IN    void               *cookie)
{
    ClRcT               rc = CL_OK;
    ClCorMOIdT          moId;
    ClCorAttrPathT     *pAttrPath;
    ClCorAttrListT      attrList;
    ClInt32T            index;
    ClUint32T           size;
    void               *pValue;
    ClCorAttrBitsT     *pAttrBits = (ClCorAttrBitsT  *)cookie;
    
    CL_FUNC_ENTER();

    if ((txnId == 0) || (jobId == 0))
    {
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }

    if((rc = clCorTxnJobMoIdGet(txnId, &moId)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get MoId from txnId, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }

    if((rc = clCorTxnJobAttrPathGet(txnId, jobId, &pAttrPath))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get AttrPath from txn-job, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }

    attrList.attrCnt = 1;
    if((rc = _clCorTxnJobSetParamsGet(0, txnId,
                             jobId, &attrList.attr[0],
                                  &index, &pValue, &size)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Could not get Set Parameters from job, rc = 0x%x", rc));
        CL_FUNC_EXIT();
        return(rc);
    }

    rc =  corObjAttrBitsGet (&moId,
                          pAttrPath,
                            &attrList,
                              pAttrBits);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT 
_clCorRegularizeGetJobs(CL_IN ClCorMOIdPtrT   pMoId, 
                        CL_IN ClCorTxnObjJobNodeT      *pObjJobNode)
{
    ClRcT                       rc                  = CL_OK;
    DMContObjHandle_t           dmContObjHdl        = {{0}};
    CORAttr_h                   attrH               = NULL;
    ClCorAttrIdT                attrId              = 0;
    ClUint32T                   size                = 0;
    ClCorTxnAttrSetJobNodeT     *pAttrGetJobNode    = NULL;
    ClUint32T                   count               = 0;
    ClUint32T                   items    = 0;
    ClUint32T                   elemSize = 0;
    ClInt32T                    index     = 0;
    ClUint32T                   usrSize  = 0;
    ClCharT                     moIdStr[CL_MAX_NAME_LENGTH];

    rc = moId2DMHandle(pMoId, &dmContObjHdl.dmObjHandle); 
    if (CL_OK != rc)
    {
        clLogError( "BUN", "PRE", 
            "Failed to get DM Object Handle for MoId %s", _clCorMoIdStrGet(pMoId, moIdStr));
        pObjJobNode->job.jobStatus = rc;
        /* Do not want to break the preprocessing. */
        return (CL_OK);
    }

    count = pObjJobNode->job.numJobs;
    pAttrGetJobNode = pObjJobNode->head;

    while(pAttrGetJobNode)
    {
        /* Set the attr path.*/
        dmContObjHdl.pAttrPath = &pObjJobNode->job.attrPath;
        attrId = pAttrGetJobNode->job.attrId;

        attrH = dmObjectAttrTypeGet(&dmContObjHdl, attrId);
        if (NULL == attrH)
        {
            rc = CL_COR_SET_RC(CL_COR_ERR_OBJ_ATTR_NOT_PRESENT);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, CL_LOG_MESSAGE_2_ATTR_DO_NOT_EXIST, 
                        attrId, dmContObjHdl.dmObjHandle.classId);
            clLogError( "BUN", "PRE", "Attribute Id [0x%x] does not exist", attrId);
            pObjJobNode->job.jobStatus = pAttrGetJobNode->job.jobStatus = rc;
            goto nextAttrInfo;
        }

        size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

        /* Check if user has passed exact size or not for simple or association attributes */
        if(attrH->attrType.type != CL_COR_ARRAY_ATTR 
            && attrH->attrType.type != CL_COR_ASSOCIATION_ATTR)
        {
            if(size != pAttrGetJobNode->job.size)
            {
                clLogError("BUN", "PRE", "For the MO [%s] attribute[0x%x] buffer size [%d] is not equal to the \
                        size expected[%d]", _clCorMoIdStrGet(pMoId, moIdStr), attrId, pAttrGetJobNode->job.size, size);
                pObjJobNode->job.jobStatus = 
                    pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                goto nextAttrInfo;
            }

            if(pAttrGetJobNode->job.index != CL_COR_INVALID_ATTR_IDX)
            {
                clLogError("BUN", "PRE", "For the MO[%s] index [%d] specified for non-array attribute [0x%x]",
                                    _clCorMoIdStrGet(pMoId, moIdStr), index, attrId);
                pObjJobNode->job.jobStatus =
                    pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
                goto nextAttrInfo;
            }
        }
        else
        {
            if(attrH->attrType.type == CL_COR_ARRAY_ATTR)
            {
                items    = attrH->attrValue.min;
            }
            else   
                items = attrH->attrValue.max;

            elemSize = size/items;
            index    = pAttrGetJobNode->job.index;
            usrSize  = pAttrGetJobNode->job.size;

            if(index == CL_COR_INVALID_ATTR_IDX)
                index = 0;

            if( (index < CL_COR_INVALID_ATTR_IDX) || (index >= items))
            {
                clLogError("BUN", "PRE", 
                        "For the MO[%s] , the index [%d] for the array attribute [0x%x] is out of range [-1 to %d]",
                        _clCorMoIdStrGet(pMoId, moIdStr), index, attrId, (items - 1));
                pObjJobNode->job.jobStatus =
                    pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_INDEX);
                goto nextAttrInfo;
            }
            
            size -= (index * elemSize);
            
            /*can not Set, if user buffer specified is greater than size left*/
            if(usrSize > size) 
            {
                pObjJobNode->job.jobStatus = 
                   pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                clLogError( "BUN", "PRE",
                    "For the MO[%s], invalid size passed for attrId [0x%x]. User passed Size [%d], Actual size [%d]", 
                    _clCorMoIdStrGet(pMoId, moIdStr), attrH->attrId, usrSize, size);
                goto nextAttrInfo;
            }
            else
            {
                /* if user size is lesser than  native type.
                     e.g. 2 bytes for array of type ClUint32T */
                if(usrSize < elemSize)
                {
                    pObjJobNode->job.jobStatus = 
                       pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                    clLogError( "BUN", "PRE",
                        "For the MO[%s] , buffer size passed is less than the basic element size for attrId [0x%x]", 
                        _clCorMoIdStrGet(pMoId, moIdStr),attrH->attrId);
                    goto nextAttrInfo;
                }

                /* user Size has to be multiples of native type. 
                    for e.g. for a Array of data type ClUint32T,
                    6 as size should not be allowed. 6 is
                    ok, if data type ClUint8T or ClUint16T */
                if((usrSize)%elemSize != 0)
                {
                    pObjJobNode->job.jobStatus = 
                       pAttrGetJobNode->job.jobStatus = CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                    clLogError( "BUN", "PRE", 
                        "For the MO[%s], buffer size passed is not exact multiple of basic \
                        element size for attrId [0x%x]", _clCorMoIdStrGet(pMoId, moIdStr), attrH->attrId);
                    goto nextAttrInfo;
                }
            }
        }

        /* Write the unfilled attribute type information */
        pAttrGetJobNode->job.attrType = attrH->attrType.type;
        pAttrGetJobNode->job.arrDataType = attrH->attrType.u.arrType;

nextAttrInfo:
        pAttrGetJobNode = pAttrGetJobNode-> next;  /* Get the next attr Info */
    }
       
    return rc;
}

/**
 * Function to store the seggregate the attribute for doing get.
 *
 * 1. The config attributes are obtained and stored locally.
 * 2. The runtime attributes are obtained from the OI and added to this buffer.
 */


ClRcT 
clCorBundleDataStore( CL_IN  ClCorTxnJobListT          *pBundleList,
                       CL_IN  ClTxnTransactionHandleT   txnHandle,
                       CL_IN  ClCorBundleDataPtrT      pBundleData,
                       CL_OUT ClCntHandleT              *pRtAttrCnt)
{
    ClRcT                   rc                  = CL_OK;
    ClUint32T               index               = 0;
    CORAttr_h               attrH               = NULL;
    ClCorAttrIdT            attrId              = 0;
    DMContObjHandle_t       dmContObjHdl        = {{0}};
    ClCorTxnObjJobNodeT     *pObjJobNode        = NULL;
    ClCorTxnAttrSetJobNodeT *pAttrGetJobNode    = NULL;
    ClCorTxnJobHeaderNodeT  *pJobHdrNode        = NULL;
    ClCharT                 moIdStr[CL_MAX_NAME_LENGTH] = {0};

    rc = clCorBundleContCreateAndDataStore(txnHandle, pBundleData);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the sessoin container. rc[0x%x]", rc));
        return rc;
    }

    while(index < pBundleList->count)
    {
        pJobHdrNode = pBundleList->pJobHdrNode[index++];

        if (NULL == pJobHdrNode)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Null argument for Job-Header"));
            return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
        }

        if(pJobHdrNode->jobHdr.isDeleted == CL_COR_TXN_JOB_HDR_OH_INVALID)
        {
            rc = clCorBundleInvalidJobAdd(*pBundleData, pJobHdrNode, pJobHdrNode->jobHdr.jobStatus);
            if(rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while storing the invalid OH jobs. rc[0x%x]", rc));
            if(CL_ERR_NO_MEMORY == CL_GET_ERROR_CODE(rc))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No memory to add the jobs. "));
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }

            continue; 
        }

        /* Add the header in the container*/
        pObjJobNode = pJobHdrNode-> head;

        if((rc = moId2DMHandle(&pJobHdrNode->jobHdr.moId, &dmContObjHdl.dmObjHandle)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the dm handle. rc[0x%x]", rc));
            /* Add this job in the container. */
            if(clCorBundleInvalidJobAdd(*pBundleData, pJobHdrNode, rc) != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the failed job. "));

            if(CL_ERR_NO_MEMORY == CL_GET_ERROR_CODE(rc))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No memory to add the jobs. "));
                return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
            }
            /* Go to the next job. */
            continue;    
        }

        while(pObjJobNode)
        {
            pAttrGetJobNode = pObjJobNode-> head;

            while(NULL != pAttrGetJobNode)
            {
                if(CL_OK != pAttrGetJobNode->job.jobStatus)
                {
                    rc = clCorBundleConfigAndRunTimeJobAdd (pBundleData->jobCont, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
                    if(CL_OK != rc)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Not able to add the sessoin job. rc[0x%x]", rc));
                    if(CL_ERR_NO_MEMORY == CL_GET_ERROR_CODE(rc))
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No memory to add the jobs. "));
                        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                    }

                    goto nextAttrJob;
                }

                dmContObjHdl.pAttrPath = &pObjJobNode->job.attrPath; 
                attrId = pAttrGetJobNode->job.attrId;

                attrH = dmObjectAttrTypeGet(&dmContObjHdl, attrId); 
                if(NULL == attrH)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("AttrId is not present[0x%x], ", attrId));
                    goto nextAttrJob;
                }

                /* If the attribute is config or runtime cached attribute add it after updating the attr value. */
                if((attrH->userFlags & CL_COR_ATTR_CONFIG) || 
                  ((attrH->userFlags & CL_COR_ATTR_RUNTIME) 
                   && (attrH->userFlags & CL_COR_ATTR_CACHED)))
                {
                    clLogTrace("BUN", "GET", "Getting the config attribute information for MO[%s], attrId [0x%x]",
                            _clCorMoIdStrGet(&pJobHdrNode->jobHdr.moId, moIdStr), attrId);

                    /* Get the attribute value for config attribues  */
                    memset(pAttrGetJobNode->job.pValue, 0, pAttrGetJobNode->job.size);
                    if((rc = dmObjectAttrGet(&dmContObjHdl, attrId, pAttrGetJobNode->job.index,
                                 pAttrGetJobNode->job.pValue, &pAttrGetJobNode->job.size )) != CL_OK)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Config attribute value get failed. rc[0x%x]", rc)); 
                        pAttrGetJobNode->job.jobStatus = rc; /* Incase of failure set the status. */ 
                    }
                
                    pObjJobNode->job.jobStatus = CL_OK;
                    rc = clCorBundleConfigAndRunTimeJobAdd (pBundleData->jobCont, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Not able to add the sessoin Info. rc[0x%x]", rc));
                        if(CL_ERR_NO_MEMORY == CL_GET_ERROR_CODE(rc))
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No Memory to add the jobs. "));
                            return rc;
                        }
                    }
                }
                else if((attrH->userFlags & CL_COR_ATTR_RUNTIME) || (attrH->userFlags & CL_COR_ATTR_OPERATIONAL))
                {
                    clLogTrace("BUN", "GET", "Getting the runtime/operational attribute information for MO[%s], attrId [0x%x]",
                            _clCorMoIdStrGet(&pJobHdrNode->jobHdr.moId, moIdStr), attrId);

                    rc = clCorBundleRuntimeJobAdd(pRtAttrCnt, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
                    if(CL_OK != rc)
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to add the runtime attribute information, rc[0x%x]", rc));
                        if(CL_ERR_NO_MEMORY == CL_GET_ERROR_CODE(rc))
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No Memory to add the jobs. "));
                            return rc;
                        }
                    }
                }
                else 
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the job information.: Invalid attribute type"));
                }
nextAttrJob: 
                pAttrGetJobNode = pAttrGetJobNode-> next; /* Next attribute info */ 
            }
            pObjJobNode = pObjJobNode-> next;  /* Next object info */
        }
    }  
    
    return rc;
}
    

/**
 *  Function to create the container and store the bundle
 *  information which will be used when runtime attribute data
 *  is obtained from the OIs.
 */

ClRcT
clCorBundleContCreateAndDataStore(CL_IN    ClTxnTransactionHandleT txnHandle, 
                                   CL_IN    ClCorBundleDataPtrT    pBundleData) 
{
        ClRcT       rc = CL_OK;
        ClCorBundleDataPtrT    pBundleStoreData = NULL;
        
        if(NULL == pBundleData)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL value passed."));
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }


        pBundleStoreData = clHeapAllocate(sizeof(ClCorBundleDataT));
        if(NULL == pBundleStoreData)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No memory. "));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pBundleStoreData, 0, sizeof(ClCorBundleDataT));

        pBundleStoreData->jobCont = pBundleData->jobCont = 0;

        rc = clCorBundleJobsContainerCreate(&(pBundleData->jobCont)); 
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the container for storing jobs. rc[0x%x]", rc));
            return rc;
        }
        
        memcpy(pBundleStoreData, pBundleData, sizeof(ClCorBundleDataT));

        rc = clCorBundleDataAdd(txnHandle, pBundleStoreData);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the bundle data in the container. rc[0x%x]", rc));
            return rc;
        }
        
        return rc;
} 


/**
 *  This function will add the runtime attribute jobs in the container.
 *  This container will be accessed before sending the jobs to the OIs.
 */


ClRcT
clCorBundleRuntimeJobAdd( CL_INOUT ClCntHandleT            *pRtAttrCnt,
                           CL_IN  ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                           CL_IN  ClCorTxnObjJobNodeT     *pObjJobNode,
                           CL_IN  ClCorTxnAttrSetJobNodeT *pAttrGetJobNode)
{
    ClRcT       rc = CL_OK;
    
    if(pRtAttrCnt == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL value for the container handle is passed."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    if(!(*pRtAttrCnt))
    {
             
        rc = clCorBundleJobsContainerCreate(pRtAttrCnt); 
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to create runtime attr job container. rc[0x%x]", rc));
            return rc;
        }
    }
    
    rc = clCorBundleAttrJobAdd(*pRtAttrCnt, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the runTime attribute info. "));
        return rc;
    }              

    return rc;
}


/**
 *  This function to store the jobs locally. This function will be called to store 
 *  all the config attribute jobs and those runtime attribute jobs whose regularization got failed.
 */

ClRcT
clCorBundleConfigAndRunTimeJobAdd( CL_IN ClCntHandleT            jobCont, 
                                    CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                                    CL_IN ClCorTxnObjJobNodeT     *pObjJobNode,
                                    CL_IN ClCorTxnAttrSetJobNodeT *pAttrGetJobNode)
{
    ClRcT       rc = CL_OK;

    rc = clCorBundleAttrJobAdd(jobCont, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the configuration attribute "));
        return rc;
    }              

    return rc;
}




/**
 * Actual function which will be called to store the jobs in the container.
 * This function will be called from different paths for adding the attribute jobs.
 */

ClRcT
clCorBundleAttrJobAdd(CL_IN ClCntHandleT            jobCont, 
                       CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                       CL_IN ClCorTxnObjJobNodeT     *pObjJobNode,
                       CL_IN ClCorTxnAttrSetJobNodeT *pAttrGetJobNode)
{
    ClRcT                   rc                  = CL_OK;
    ClCorTxnJobHeaderNodeT  *pTempJobHdr        = NULL;
    ClCorTxnObjJobNodeT     *pTempObjJobNode    = NULL;
    ClCorTxnAttrSetJobNodeT *pTempAttrJobNode   = NULL;

    CL_FUNC_ENTER();

    if (NULL == pJobHdrNode || NULL == pObjJobNode || NULL == pAttrGetJobNode)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("NULL pointer found..."));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    
    rc = clCntDataForKeyGet(jobCont, (ClCntKeyHandleT) &pJobHdrNode->jobHdr.moId, (ClCntDataHandleT *) &pTempJobHdr);
    if ((CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Not able to find the key: rc [0x%x]", rc));
    CL_FUNC_EXIT();
    return rc;
    }


    if(NULL == pTempJobHdr)
    {
        ClCorMOIdT* pMoId = NULL;

        rc = clCorTxnJobHeaderNodeCreate(&pTempJobHdr);
        if(CL_OK != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the header node. rc[0x%x]", rc));
            return rc;
        }

        memcpy(pTempJobHdr, pJobHdrNode, sizeof(ClCorTxnJobHeaderNodeT));
        pTempJobHdr->head = pTempJobHdr->tail = NULL;
        pTempJobHdr->jobHdr.numJobs = 0;

        rc = clCorMoIdAlloc(&pMoId);
        if (rc != CL_OK)
        {
            clLogError("TXN", "GET", "Failed to allocate memory.");
            return rc;
        }

        (*pMoId) = pJobHdrNode->jobHdr.moId;

#if 0        
        pObjH = clHeapAllocate(sizeof(ClCorObjectHandleT));
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while cloning the MoId. rc[0x%x]", rc));
            return rc;
        }
        memset(pObjH, 0, sizeof(ClCorObjectHandleT));

        memcpy(pObjH, &pJobHdrNode->jobHdr.oh, sizeof(ClCorObjectHandleT ));
#endif

        if((rc = clCntNodeAdd(jobCont, (ClCntKeyHandleT) pMoId, (ClCntDataHandleT) pTempJobHdr, NULL)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in container. rc[0x%x]", rc));
            return rc;
        }

        if((rc = clCorTxnObjJobTblCreate(&pTempJobHdr->objJobTblHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the job table. rc[0x%x]", rc));
            return rc;
        }
         
        rc = clCorTxnJobNodeAllocate(CL_COR_OP_GET, &pTempObjJobNode); 
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the job node. rc[0x%x]", rc));
            return rc;
        }
        
        memcpy(pTempObjJobNode, pObjJobNode, sizeof(ClCorTxnObjJobNodeT));
        pTempObjJobNode->next = pTempObjJobNode->prev = NULL;
        pTempObjJobNode->head = pTempObjJobNode->tail = NULL;
        pTempObjJobNode->job.numJobs = 0;
            
        if((rc = clCorTxnObjJobNodeAdd(pTempJobHdr, pTempObjJobNode)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in the job handle container. rc[0x%x]", rc));
            return rc;
        }

        CL_COR_TXN_OBJ_JOB_ADD(pTempJobHdr, pTempObjJobNode);

        pTempJobHdr->jobHdr.numJobs++;

        pTempAttrJobNode = clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
        if(NULL == pTempAttrJobNode)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the attr header. rc[0x%x]", rc));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        memset(pTempAttrJobNode, 0, sizeof(ClCorTxnAttrSetJobNodeT));
        
        /* copy everything blindly  */
        memcpy(pTempAttrJobNode, pAttrGetJobNode, sizeof(ClCorTxnAttrSetJobNodeT));
        
        /* Need to allocate and copy this value */
        pTempAttrJobNode->job.pValue = clHeapAllocate(pTempAttrJobNode->job.size);
        memset(pTempAttrJobNode->job.pValue, 0, pTempAttrJobNode->job.size);
        
        /* Copy the value */
        memcpy(pTempAttrJobNode->job.pValue, pAttrGetJobNode->job.pValue, pTempAttrJobNode->job.size);

        /* Resetting the next and previous pointers. */
        pTempAttrJobNode->next = pTempAttrJobNode->prev = NULL;

        /* Adding the attr set jobs. */
        CL_COR_BUNDLE_JOB_SORT_AND_ADD(pTempObjJobNode, pTempAttrJobNode);

        pTempObjJobNode->job.numJobs++;
    }
    else
    {
        /* Updating the job status  */
        pTempJobHdr->jobHdr.jobStatus = pJobHdrNode->jobHdr.jobStatus;

        if(pTempJobHdr->objJobTblHdl && 
                 ( clCorTxnObjJobNodeGet(pTempJobHdr, 
                            &pObjJobNode->job.attrPath, &pTempObjJobNode)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Not able to find the objHandle"));
            
            rc = clCorTxnJobNodeAllocate(CL_COR_OP_GET, &pTempObjJobNode); 
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the job node. rc[0x%x]", rc));
                return rc;
            }
            
            /* Copy everything blindly */
            memcpy(pTempObjJobNode, pObjJobNode, sizeof(ClCorTxnObjJobNodeT));

            /* Reseting the values which are not required.*/
            pTempObjJobNode->next = pTempObjJobNode->prev = NULL;
            pTempObjJobNode->head = pTempObjJobNode->tail = NULL;
            pTempObjJobNode->job.numJobs = 0;
                
            if((rc = clCorTxnObjJobNodeAdd(pTempJobHdr, pTempObjJobNode)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in the job handle container. rc[0x%x]", rc));
                return rc;
            }

            CL_COR_TXN_OBJ_JOB_ADD(pTempJobHdr, pTempObjJobNode);

            pTempJobHdr->jobHdr.numJobs++;
        }
        else
        {
            if (pTempObjJobNode != NULL)
                pTempObjJobNode->job.jobStatus = pObjJobNode->job.jobStatus;
            else
            {
                clLogError("BUN", "", 
                        "The pointer to the object job is found NULL. Looks like \
                        the job table was not initialized. ");
                return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            }
        }

        pTempAttrJobNode = clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
        if(NULL == pTempAttrJobNode)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the attr header. rc[0x%x]", rc));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }
        memset(pTempAttrJobNode, 0, sizeof(ClCorTxnAttrSetJobNodeT));
        

        memcpy(pTempAttrJobNode, pAttrGetJobNode, sizeof(ClCorTxnAttrSetJobNodeT));

        /* Need to allocate and copy this value */
        pTempAttrJobNode->job.pValue = clHeapAllocate(pTempAttrJobNode->job.size);
        memset(pTempAttrJobNode->job.pValue, 0, pTempAttrJobNode->job.size);
        
        memcpy(pTempAttrJobNode->job.pValue, pAttrGetJobNode->job.pValue, pTempAttrJobNode->job.size);

        /* Resetting the next and previous pointers. */
        pTempAttrJobNode->next = pTempAttrJobNode->prev = NULL;

        /* Adding the attr set jobs. */
        //CL_COR_TXN_ATTR_SET_JOB_ADD(pTempObjJobNode, pTempAttrJobNode);
        CL_COR_BUNDLE_JOB_SORT_AND_ADD(pTempObjJobNode, pTempAttrJobNode);

        pTempObjJobNode->job.numJobs++;
    }
    return rc;
}


/**
 *  This function will add the jobs before sending the jobs to the OI for 
 *  processing.
 */

ClRcT
clCorBundleTxnJobAdd( CL_IN ClTxnTransactionHandleT txnHandle,
                       CL_IN ClCntHandleT rtJobCont,
                       CL_IN ClCorBundleDataT sessionData)
{
    ClRcT                   rc              = CL_OK;
    ClUint32T               i               = 0;
    ClTxnJobHandleT         jobHandle       = {0};
    ClBufferHandleT         msgHdl          = 0;
    ClUint32T               length          = 0;
    ClUint8T                *pData          = NULL;
    ClUint32T               jobCount        = 0;
    ClCorTxnJobHeaderNodeT  *pRuntimeJobHdr = NULL;
    ClIocAddressT           compAddress     = {{0}};
    ClCorAddrT              addr            = {0};
    ClCntNodeHandleT        nodeHandle      = 0;
    ClCharT                 moIdStr[CL_MAX_NAME_LENGTH] = {0};
 
    CL_FUNC_ENTER();

    if(!rtJobCont)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cont is empty"));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    
    if((rc = clCntSizeGet(rtJobCont, &jobCount)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the runtime attribute container. rc[0x%x]", rc));
        return rc;
    }

    if((rc = clCntFirstNodeGet(rtJobCont, &nodeHandle)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to find the first node in the runtime attr container. rc[0x%x]",rc));
        return rc;
    }
     
    while((nodeHandle != 0) && (i < jobCount))
    {
        if((rc = clCntNodeUserDataGet(rtJobCont, nodeHandle, (ClCntDataHandleT *)&pRuntimeJobHdr)) != CL_OK)
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
            clLogError ( "BUN", "AJB", "No primary OI registered for the MO[%s]. rc[0x%x]", 
                    _clCorMoIdStrGet(&(pRuntimeJobHdr->jobHdr.moId), moIdStr), rc);

            /* Add the job which is not having an OI */
            rc = clCorBundleInvalidJobAdd(sessionData, pRuntimeJobHdr, rc);
            if(rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the invalid runtime attribute job. rc[0x%x]", rc));
            clBufferDelete(&msgHdl);
            goto nextRuntimeAttrJob;
        }
  
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Setting component node:0x%x portId:0x%x\n", 
                           addr.nodeAddress, 
                                addr.portId));

        if((rc = clCorTxnJobStreamPack(pRuntimeJobHdr, msgHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Cor-Txn Job Stream Unpack Failed. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(rtJobCont))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while finalizing the container."));
            CL_FUNC_EXIT();
            return (rc);
        }

        if((rc = clBufferLengthGet(msgHdl, &length)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Could not get buffer message length. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(rtJobCont))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
            CL_FUNC_EXIT();
            return (rc);
        }

        pData = NULL;
        if((rc = clBufferFlatten(msgHdl, &pData)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Flattening Failed. rc: 0x%x", rc));
            if(CL_OK !=  clCorBundleJobsContainerFinalize(rtJobCont))
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
            clBufferDelete(&msgHdl);
            CL_FUNC_EXIT();
            return (rc);
        }

        if(pData)
        {
            rc = clTxnJobAdd(txnHandle, 
                             (ClTxnJobDefnHandleT)pData, 
                             length, 
                             CL_COR_TXN_SERVICE_ID_READ,
                             &jobHandle);
            if (CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to register new job in txn-manager. rc:0x%x\n", rc));
                clHeapFree(pData);
                clBufferDelete(&msgHdl);
                goto nextRuntimeAttrJob;
            }
        }
       
        clHeapFree(pData);
        
        rc = clBufferDelete(&msgHdl);

        memcpy(&compAddress, &addr, sizeof(ClCorAddrT));

        rc = clTxnComponentSet(txnHandle, jobHandle, compAddress, CL_TXN_COMPONENT_CFG_1_PC_CAPABLE);
        if (CL_OK != rc)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                         CL_LOG_MESSAGE_1_SET_COMP_ADDRESS, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set component-address for this job. rc:0x%x\n", rc));
        }

nextRuntimeAttrJob:

        if((rc = clCntNextNodeGet(rtJobCont, nodeHandle, &nodeHandle)) != CL_OK)
        {
            if(CL_ERR_NOT_EXIST == rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the "));
                if(CL_OK !=  clCorBundleJobsContainerFinalize(rtJobCont))
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while clearing the container."));
                return rc;
            }
            return CL_OK;
        }
        /* increment the job count. */
        ++i;
    }  
    
    CL_FUNC_EXIT();
    return rc;
}


/**
 * This function will add the jobs in the local job list if the job got failed or 
 * there is no primary OI registered for this Job.
 */

ClRcT
clCorBundleInvalidJobAdd( CL_IN ClCorBundleDataT       sessionData,
                           CL_IN ClCorTxnJobHeaderNodeT  *pJobHdrNode,
                           CL_IN ClRcT                   status)
{
    ClRcT                   rc                  = CL_OK;
    ClCorTxnJobHeaderNodeT  *pTempJobHdrNode    = NULL;
    ClCorTxnObjJobNodeT     *pObjJobNode        = NULL, *pTempObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT *pAttrGetJobNode    = NULL, *pTempAttrJobNode = NULL;

    if(NULL == pJobHdrNode)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL parameter received. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCntDataForKeyGet(sessionData.jobCont, (ClCntKeyHandleT)&(pJobHdrNode->jobHdr.moId),
                                        (ClCntDataHandleT *) &pTempJobHdrNode);
    if ((CL_OK != rc) && (CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Not able to find the key: rc [0x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    
    if(NULL == pTempJobHdrNode) /* Header is not there */
    {
        ClCorMOIdT* pMoId = NULL;

        rc = clCorTxnJobHeaderNodeCreate(&pTempJobHdrNode);
        if(CL_OK != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the header node. rc[0x%x]", rc));
            return rc;
        }

        memcpy(pTempJobHdrNode, pJobHdrNode, sizeof(ClCorTxnJobHeaderNodeT));
        pTempJobHdrNode->head = pTempJobHdrNode->tail = NULL;
        pTempJobHdrNode->jobHdr.numJobs = 0;

        /* The whole job is invalid - as it contain only runtime attributes */
        pTempJobHdrNode->jobHdr.jobStatus = CL_COR_TXN_JOB_FAIL;

        rc = clCorMoIdAlloc(&pMoId);
        if (rc != CL_OK)
        {
            clLogError("TXN", "GET", "Failed to allocate memory.");
            return rc;
        }

        (*pMoId) = pJobHdrNode->jobHdr.moId;

#if 0        
        pObjH = clHeapAllocate(sizeof(ClCorObjectHandleT));
        if(CL_OK != rc)
        {   
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating object handle. rc[0x%x]", rc));
            return rc;
        }
        memset(pObjH, 0, sizeof(ClCorObjectHandleT));

        memcpy(pObjH, &pJobHdrNode->jobHdr.oh, sizeof(ClCorObjectHandleT));
#endif

        if((rc = clCntNodeAdd(sessionData.jobCont, (ClCntKeyHandleT) pMoId, (ClCntDataHandleT) pTempJobHdrNode, NULL)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in container. rc[0x%x]", rc));
            return rc;
        }

        if((rc = clCorTxnObjJobTblCreate(&pTempJobHdrNode->objJobTblHdl)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the job table. rc[0x%x]", rc));
            return rc;
        }
         
        pObjJobNode = pJobHdrNode->head;
        while(pObjJobNode)
        {
            rc = clCorTxnJobNodeAllocate(CL_COR_OP_GET, &pTempObjJobNode); 
            if(CL_OK != rc)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the job node. rc[0x%x]", rc));
                return rc;
            }
            
            memcpy(pTempObjJobNode, pObjJobNode, sizeof(ClCorTxnObjJobNodeT));
            pTempObjJobNode->next = pTempObjJobNode->prev = NULL;
            pTempObjJobNode->head = pTempObjJobNode->tail = NULL;
            pTempObjJobNode->job.numJobs = 0;
                
            pTempObjJobNode->job.jobStatus = status;

            if((rc = clCorTxnObjJobNodeAdd(pTempJobHdrNode, pTempObjJobNode)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in the job handle container. rc[0x%x]", rc));
                return rc;
            }

            CL_COR_TXN_OBJ_JOB_ADD(pTempJobHdrNode, pTempObjJobNode);

            pTempJobHdrNode->jobHdr.numJobs++;

            pAttrGetJobNode = pObjJobNode->head;
            while(pAttrGetJobNode)
            {
                pTempAttrJobNode = clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
                if(NULL == pTempAttrJobNode)
                {   
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the attr header. rc[0x%x]", rc));
                    return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                }
                memset(pTempAttrJobNode, 0, sizeof(ClCorTxnAttrSetJobNodeT));
                
                memcpy(pTempAttrJobNode, pAttrGetJobNode, sizeof(ClCorTxnAttrSetJobNodeT));
                /* Need to allocate and copy this value */
                pTempAttrJobNode->job.pValue = clHeapAllocate(pTempAttrJobNode->job.size);
                if(pTempAttrJobNode->job.pValue == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
                    return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                }

                memset(pTempAttrJobNode->job.pValue, 0, pTempAttrJobNode->job.size);
                /* Setting the status of the sub job */
                if(pAttrGetJobNode->job.jobStatus != CL_OK) 
                    pTempAttrJobNode->job.jobStatus = pAttrGetJobNode->job.jobStatus;
                else
                    pTempAttrJobNode->job.jobStatus = status;

                /* Resetting the next and previous pointers. */
                pTempAttrJobNode->next = pTempAttrJobNode->prev = NULL;

                /* Adding the attr set jobs. */
                CL_COR_BUNDLE_JOB_SORT_AND_ADD(pTempObjJobNode, pTempAttrJobNode);

                pTempObjJobNode->job.numJobs++;

                pAttrGetJobNode = pAttrGetJobNode->next; /* Get next sub job*/
            }
            pObjJobNode = pObjJobNode->next;    /* Get next job node*/
        }
    }
    else /* Found the header */
    {
        if(!pTempJobHdrNode->objJobTblHdl) /* Check if the job table is created */
        {
            if((rc = clCorTxnObjJobTblCreate(&pTempJobHdrNode->objJobTblHdl)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the job table. rc[0x%x]", rc));
                return rc;
            }
        }
    
        pObjJobNode = pJobHdrNode->head;
        while(pObjJobNode) /* There can be more than one jobs*/
        {
            if((rc = clCorTxnObjJobNodeGet(pTempJobHdrNode, &pObjJobNode->job.attrPath, &pTempObjJobNode)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Not able to find the obj job"));
                
                rc = clCorTxnJobNodeAllocate(CL_COR_OP_GET, &pTempObjJobNode); 
                if(CL_OK != rc)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the job node. rc[0x%x]", rc));
                    return rc;
                }
                
                /* Setting the job status */
                pTempObjJobNode->job.jobStatus = status;

                memcpy(pTempObjJobNode, pObjJobNode, sizeof(ClCorTxnObjJobNodeT));
                    
                pTempObjJobNode->next = pTempObjJobNode->prev = NULL;
                pTempObjJobNode->head = pTempObjJobNode->tail = NULL;
                pTempObjJobNode->job.numJobs = 0;

                if((rc = clCorTxnObjJobNodeAdd(pTempJobHdrNode, pTempObjJobNode)) != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the job in the job handle container. rc[0x%x]", rc));
                    return rc;
                }

                CL_COR_TXN_OBJ_JOB_ADD(pTempJobHdrNode, pTempObjJobNode);

                pTempJobHdrNode->jobHdr.numJobs++;
            }

            pAttrGetJobNode = pObjJobNode->head;
            while(pAttrGetJobNode) /* There can be more than one sub jobs.*/
            {
                pTempAttrJobNode = clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
                if(NULL == pTempAttrJobNode)
                {   
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the attr header. rc[0x%x]", rc));
                    return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                }
                memset(pTempAttrJobNode, 0, sizeof(ClCorTxnAttrSetJobNodeT));
                
                memcpy(pTempAttrJobNode, pAttrGetJobNode, sizeof(ClCorTxnAttrSetJobNodeT));

                /* Setting the sub job level status */
                if(pAttrGetJobNode->job.jobStatus != CL_OK) 
                    pTempAttrJobNode->job.jobStatus = pAttrGetJobNode->job.jobStatus;
                else
                    pTempAttrJobNode->job.jobStatus = status;

                /* Need to allocate and copy this value */
                pTempAttrJobNode->job.pValue = clHeapAllocate(pTempAttrJobNode->job.size);
                if(pTempAttrJobNode->job.pValue == NULL)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while allocating the memory. "));
                    return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
                }

                memset(pTempAttrJobNode->job.pValue, 0, pTempAttrJobNode->job.size);

                /* Resetting the next and previous pointers. */
                pTempAttrJobNode->next = pTempAttrJobNode->prev = NULL;

                /* Adding the attr set jobs. */
                CL_COR_BUNDLE_JOB_SORT_AND_ADD(pTempObjJobNode, pTempAttrJobNode);

                pTempObjJobNode->job.numJobs++;
                
                pAttrGetJobNode = pAttrGetJobNode->next; 
            }
            pObjJobNode = pObjJobNode->next;
        }
    }
    return rc;
}


/**
 * This function will be used to send the response to the client. This
 * function will send the jobs after getting the response from the OI 
 * or when there is a failure during processin of the bundle.
 */

ClRcT
clCorBundleResponseSend(ClCorBundleDataT bundleData)
{
    ClRcT                   rc              = CL_OK;
    ClBufferHandleT         inMsgHandle     = 0;
    ClCntNodeHandleT        nodeHandle      = 0;
    ClUint32T               jobCount        = 0;
    ClUint32T               count           = 0;
    ClCorTxnJobHeaderNodeT  *pJobHeaderNode = NULL;
    
    /* Assigning the message handle */
    inMsgHandle = bundleData.outMsgHandle;

    clLogTrace("BUN", "REP", "Sending the response for the bundle handle [%#llX]", 
            bundleData.txnId);

    rc = clXdrMarshallClHandleT(&bundleData.txnId, inMsgHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the handle. rc[0x%x]", rc));
        goto sendResponse;
    }
    
    if (bundleData.jobCont == 0)
    {
        clLogNotice("BUN", "REP", "The job container is not initialized till now.");

        jobCount = 0;
        rc = clXdrMarshallClUint32T(&jobCount, inMsgHandle,  0);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the job count. rc[0x%x]", rc));
        }
        goto sendResponse;
    }

    rc = clCntSizeGet(bundleData.jobCont, &jobCount);
    if(CL_OK != rc)
    {
        if(CL_ERR_INVALID_HANDLE != CL_GET_ERROR_CODE(rc))
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the container size. rc[0x%x]", rc));

        jobCount = 0;
        rc = clXdrMarshallClUint32T(&jobCount, inMsgHandle,  0);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the job count. rc[0x%x]", rc));
        }
        goto sendResponse;
    }

    if(jobCount == 0)
    {
        rc = CL_COR_SET_RC(CL_COR_ERR_ZERO_JOBS_BUNDLE);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("There are no jobs to send. rc[0x%x]", rc));

        rc = clXdrMarshallClUint32T(&jobCount, inMsgHandle,  0);
        if(CL_OK != rc)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the job count. rc[0x%x]", rc));

        goto sendResponse;;
    }

    rc = clXdrMarshallClUint32T(&jobCount, inMsgHandle,  0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the job count. rc[0x%x]", rc));
        goto sendResponse;
    }

    rc = clCntFirstNodeGet(bundleData.jobCont, &nodeHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the first JOB node. rc[0x%x]", rc));
        goto sendResponse;
    }

    count = jobCount;

    while(nodeHandle)
    {
        count-- ;

        rc = clCntNodeUserDataGet(bundleData.jobCont, nodeHandle, (ClCntDataHandleT *)&pJobHeaderNode);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the user data for the node. rc[0x%x]", rc));
            goto sendResponse;
        }

        rc = clCorTxnJobStreamPack(pJobHeaderNode, inMsgHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while packing the header information. rc[0x%x]", rc));
            goto sendResponse;
        }

        rc = clCntNextNodeGet(bundleData.jobCont, nodeHandle, &nodeHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Failed while getting the next node. rc[0x%x]", rc));
            goto sendResponse;
        }
    }

    if(count != 0)
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("All jobs are not updated with attribute values."));

sendResponse:

    clLogTrace("BUN", "RES", "Sending the Response to the client for the bundle handle[%#llx]. jobCount [%d]", 
            bundleData.txnId, jobCount);

    rc = clRmdSyncResponseSend(bundleData.clientAddr, inMsgHandle, CL_OK);
    if(CL_OK != rc)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while sending the response to client. rc[0x%x]", rc));

    clCorBundleJobsContainerFinalize(bundleData.jobCont);

    return rc;
}



/*****************************************************************************
    Container for storing the session data 
******************************************************************************/

/* Linked list for storing the session related data */

/**
 * Container for storing the bundle specific information.
 */

ClRcT clCorBundleDataContCreate()
{
    ClRcT rc = CL_OK;
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Creating the session container. "));
    rc = clCntThreadSafeLlistCreate(clCorBundleDataKeyComp,
                            clCorBundleDataDeleteFn,
                            clCorBundleDataDeleteFn,
                            CL_CNT_UNIQUE_KEY,
                            &gCorBundleData);
                            
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("gCorTxnJobInfo container creation failed. rc [0x%x]", rc));
        return rc;
    }
    
    return rc;
}


/**
 * Key compare function.
 */

ClInt32T clCorBundleDataKeyComp(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

/**
 * Delete callback function for the container.
 */

void clCorBundleDataDeleteFn(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    ClCorBundleDataPtrT pBundleData = NULL;

    pBundleData = (ClCorBundleDataPtrT) data;

    clHeapFree(pBundleData); 
}


/**
 * Container finalize function.
 */ 
ClRcT clCorBundleDataContFinalize()
{
    ClRcT rc = CL_OK;

    rc = clCntDelete((ClCntHandleT) gCorBundleData);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while deleting the sessionData container. rc [0x%x]", rc));
        return rc;
    }

    return rc;
}

/**
 * Function to add the bundle specific data in the container.
 */

ClRcT
clCorBundleDataAdd(CL_IN ClTxnTransactionHandleT   txnHandle,
                    CL_IN ClCorBundleDataPtrT      pBundleStoreData)
{
        ClRcT       rc = CL_OK;

        rc = clCntNodeAdd(gCorBundleData,(ClCntKeyHandleT) (ClWordT)txnHandle, (ClCntDataHandleT)pBundleStoreData, NULL);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the session Data. rc[0x%x]", rc));
            return rc;
        }

        return rc; 
}
                    
/**
 * Function to delete the node corresponding the the bundle.
 */

ClRcT
clCorBundleDataNodeDelete(CL_IN  ClTxnTransactionHandleT   txnHandle)
{
        ClRcT           rc = CL_OK;
        ClCntNodeHandleT   nodeHandle = 0;

        rc = clCntNodeFind(gCorBundleData,(ClCntKeyHandleT) (ClWordT)txnHandle, &nodeHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the session Data. rc[0x%x]", rc));
            return rc;
        }

        rc = clCntNodeDelete(gCorBundleData, nodeHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while removing the session data from the container. rc[0x%x]", rc));
            return rc;
        }
        return rc; 
}


/**
 * Function will get the data for the transaction. This function will be used to 
 * get the information in the transaction specific completion callback function.
 */

ClRcT
clCorBundleDataGet(CL_IN  ClTxnTransactionHandleT   txnHandle,
                    CL_OUT ClCorBundleDataPtrT      *pBundleStoreData)
{
        ClRcT               rc = CL_OK;
        ClCntNodeHandleT    nodeHandle = 0;

        if(NULL == pBundleStoreData)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

        rc = clCntNodeFind(gCorBundleData,(ClCntKeyHandleT) (ClWordT)txnHandle, &nodeHandle);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the session Data. rc[0x%x]", rc));
            return rc;
        }

        rc = clCntNodeUserDataGet(gCorBundleData, nodeHandle, (ClCntDataHandleT *)pBundleStoreData);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the user data. rc[0x%x]",rc));
            return rc;
        }

        return rc; 
}



/**
 * Function to aggregate the response from the OI after processing. This function will be called 
 * in the transaction completion callback function.
 */

ClRcT 
clCorBundleJobAggregate( CL_IN ClCorBundleDataT sessionData,
                          CL_IN ClCorTxnJobHeaderNodeT *pJobHdrNode)
{
    ClRcT                   rc = CL_OK;
    ClCorTxnObjJobNodeT     *pObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT *pAttrGetJobNode = NULL;

    if(NULL == pJobHdrNode)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Null pointer passed. "));
        return rc;
    }

    /* Only runtime jobs are there for this header. */
    pJobHdrNode->jobHdr.jobStatus = CL_OK;

    pObjJobNode = pJobHdrNode->head;
    while(pObjJobNode != NULL) /* Look for all the jobs */
    {
        pAttrGetJobNode = pObjJobNode->head;
        while(pAttrGetJobNode != NULL ) /* Look for all the sub jobs.*/
        {
            rc = clCorBundleAttrJobAdd(sessionData.jobCont, pJobHdrNode, pObjJobNode, pAttrGetJobNode);
            if(CL_OK != rc)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while adding the jobs in the response container.")); 

            pAttrGetJobNode = pAttrGetJobNode->next; /* Get next sub job */
        }
        pObjJobNode = pObjJobNode->next; /* Get next Obj job */
    }
    return rc;
}

/*****************************************************************************
*****************************************************************************/


/**
 * Function to validate the job in the regularize for create/delete jobs.
 */

ClUint32T _corIsMoIdInTxnList(ClCorTxnJobListT* pTxnList, 
                              ClUint32T nodeIdx, 
                              // coverity[pass_by_value]
                              ClCorMOIdT moId, 
                              ClCorOpsT op)
{
    ClUint32T i = 0;
    ClCorTxnObjJobNodeT* pObjJobNode = NULL;
    ClCorTxnJobHeaderNodeT* pTxnJobHdr = NULL;
   
    CL_FUNC_ENTER();
    
    /* Traverse through each moId based job list and find this moId is queued for 'op' */
    for (i=0; i<nodeIdx; i++)
    {
        pTxnJobHdr = pTxnList->pJobHdrNode[i];

        if (clCorMoIdCompare(&pTxnJobHdr->jobHdr.moId, &moId) == 0)
        {
            /* Walk through all the sub jobs and check whether the operation 'op' is scheduled for this moId */
            pObjJobNode = pTxnJobHdr->head;

            while (pObjJobNode)
            {
                if ((op == CL_COR_OP_CREATE) || (op == CL_COR_OP_CREATE_AND_SET))
                {
                    if ((pObjJobNode->job.op == CL_COR_OP_CREATE) || 
                            (pObjJobNode->job.op == CL_COR_OP_CREATE_AND_SET))
                    {
                        CL_FUNC_EXIT();
                        return 1;
                    }
                }
                else if (pObjJobNode->job.op == op)
                {
                    CL_FUNC_EXIT();
                    return 1;
                }
                
                pObjJobNode = pObjJobNode->next;
            }
        }
    }

    CL_FUNC_EXIT();
    return 0;
}

#if 0
/**
 * Function to validate the moid against the OH mask.
 */
ClRcT _clCorMoIdValidateAgainstOHMask(ClCorMOIdPtrT pMoId)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId;
    ClUint32T idx = 0;
    ClUint64T classVal = 0;
    ClUint64T instanceVal = 0;

    clCorMoIdInitialize(&moId);
    
    moId = *pMoId;
    if ((rc = corMOTreeClass2IdxConvert(&moId)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corMOTreeClass2IdxConvert failed rc => [0x%x]", rc));
        CL_FUNC_EXIT();
        return(rc);
    }

    for ( idx = 0; idx < moId.depth ; idx ++)
    {
        if(gClCorCfgAppOHMask[2*idx] == CL_COR_OH_MASK_END_MARKER)
            return CL_OK;

        classVal = (1<<gClCorCfgAppOHMask[2*idx]) - 1;
        if(moId.node[idx].type > classVal)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Object of class [0x%x] cannot be created. OH MASK Violation.", pMoId->node[idx].type));
            return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
        }
        
        instanceVal = (1<<gClCorCfgAppOHMask[2*idx +1]) - 1;
        if( moId.node[idx].instance > instanceVal)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("The object instance [0x%x] of the class [0x%x] cannot be created. OH Mask Violation.", 
                     pMoId->node[idx].instance, pMoId->node[idx].type));
            return CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
        }        
    }
    return CL_OK;
}
#endif

/**
 * Function to which will validate whether there is a child MO in the list.
 */

ClUint32T _corIsChildMoIdInTxnList(ClCorTxnJobListT* pTxnList, 
                                   ClUint32T nodeIdx,
                                   // coverity[pass_by_value]
                                   ClCorMOIdT moId, 
                                   ClCorOpsT op)
{
    ClUint32T i = 0;
    ClUint16T level = 0;
    ClCorTxnJobHeaderNodeT* pTxnJobHdr = NULL;
    ClCorTxnObjJobNodeT* pObjJobNode = NULL;
    ClCorMOIdT moIdTemp;

    CL_FUNC_ENTER();
    
    level = moId.depth;
    
    for (i=0; i<nodeIdx; i++)
    {
        pTxnJobHdr = pTxnList->pJobHdrNode[i];

        moIdTemp = pTxnJobHdr->jobHdr.moId;

        if (level < moIdTemp.depth)
        {
            clCorMoIdTruncate(&moIdTemp, level);
 
            if (clCorMoIdCompare(&moIdTemp, &moId) == 0)
            {
                /* Walk through all the sub jobs and check whether the operation 'op' is scheduled for this moId */
                pObjJobNode = pTxnJobHdr->head;

                while (pObjJobNode)
                {
                    if (pObjJobNode->job.op == op)
                    {
                        CL_FUNC_EXIT();
                        return 1;
                    }
                
                    pObjJobNode = pObjJobNode->next;
                }
            }          
        }
    }

    CL_FUNC_EXIT();
    return 0;
}



/**
 * This function will used in the object creation path. This will validate the MOId instance
 * against the maximum instance allowed for the MO.
 */
ClRcT clCorObjInstanceValidate(ClCorMOIdPtrT pMoId)
{
    ClRcT               rc = CL_OK;
	MArrayVector_h      vectorH = NULL;
	MOTreeNode_h        parentMO  = NULL;
	MOTreeNode_h        hMONode   = NULL;
	ClCorClassTypeT     classId  = 0; 
    ObjTreeNode_h       parent = NULL;
    ObjTreeNode_h       node = NULL;
    ClUint32T           grpId = 0;
    _CORMOClass_h       hMOClass = 0;
    ClCorInstanceIdT    instId = 0;
	ClCorMOClassPathT   tmpCorPath ;
    ClCharT             moIdStr[CL_MAX_NAME_LENGTH];

    
    if(NULL == pMoId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed for the MoId."));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    
	classId = pMoId->node[pMoId->depth-1].type;

    instId = clCorMoIdToInstanceGet(pMoId);

    clCorMoIdToMoClassPathGet(pMoId, &tmpCorPath);

    if(pMoId->svcId != CL_COR_INVALID_SRVC_ID)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Dont need to validate the Mso"));
        return CL_OK;
    }

    /* get the index list from the moTree and check if the classes
     * are present.
     */
    rc = mArrayId2Idx(moTree, (ClUint32T *) tmpCorPath.node, tmpCorPath.depth);
    if(rc != CL_OK)
    {
        CL_FUNC_EXIT();
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, (CL_COR_ERR_STR(CL_COR_MO_TREE_ERR_NODE_NOT_FOUND)));
        return rc;
    }

    /** 
     * Get the parent to proceed further.
     */
    parent = corObjTreeFindParent(objTree, pMoId);
    if(!parent)
    {
        /* parent unknown */
        CL_FUNC_EXIT();
        clLog(CL_LOG_SEV_NOTICE, "TXN", "PRE", "Parent MO doesn't exist in the \
            object tree for the MO (%s). ", _clCorMoIdStrGet(pMoId, moIdStr));
        return CL_OK;
    }

    /* check if node is there and then add 
    */
    node = mArrayNodeIdIdxNodeFind(parent, classId, instId);
    if(NULL == node)
    {
        grpId = tmpCorPath.node[tmpCorPath.depth-1];
        vectorH = mArrayGroupGet(parent, grpId);
        if(vectorH == NULL)
        {
            if(corMOInstaceValidate(pMoId) == CL_TRUE)
                return CL_OK;
            else 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("The Maximum instance is not valid. "));
                return CL_COR_SET_RC(CL_COR_INST_ERR_MAX_INSTANCE);
            }
        }	

        clCorMoIdToMoClassPathGet(pMoId, &tmpCorPath);
        if (NULL != (parentMO = corMOTreeFindParent(moTree, &tmpCorPath)))
        {
            if (NULL != (hMONode = mArrayNodeIdNodeFind(parentMO, classId)))
            {
                hMOClass = (_CORMOClass_h) hMONode->data;

                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("MaxInstance [%d], Current Instances [%d]", 
                             hMOClass->maxInstances, vectorH->numActiveNodes));

                if (hMOClass->maxInstances > 0)
                {
                    if((vectorH->numActiveNodes < hMOClass->maxInstances) && 
                            (instId < hMOClass->maxInstances) && (instId >= 0))
                    {
                        return CL_OK;
                    }
                }
                else
                {
                    if (instId >= 0)
                        return CL_OK;
                    else
                        return CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
                }
            }
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Object create validation failed: MO already present.")); 
        return CL_COR_SET_RC(CL_COR_INST_ERR_NODE_ALREADY_PRESENT); 
    }

	return CL_COR_SET_RC(CL_COR_INST_ERR_MAX_INSTANCE);	
}


/**
 * Function to validate the transaction job list against all the 
 * in-progress job.
 */ 

ClRcT _clCorTxnJobInprogressValidation(ClCorTxnJobListT *pTxnList)
{
    ClRcT               rc = CL_OK;
    Byte_h              objBuf = NULL;
    ClPtrT              *pHdr = NULL;
    ClCharT             moIdStr[CL_MAX_NAME_LENGTH] = {0};
    ClUint32T           index = 0;
    CORClass_h          pClassH = NULL;
    ClCorMOIdT          moId ;
    DMObjHandle_t       dmH = {0};
    CORInstanceHdr_t    hdr = 0;

    
    pHdr = clHeapAllocate(pTxnList->count * sizeof(CORInstanceHdr_h));
    if( NULL == pHdr )
    {
        clLogError("TXN", "PRE", "Failed while allocating the memory for the instance header list. ");
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    /**
     * This part of the function checks weather there are any jobs already in 
     * progress as a part of some other transaction.
     */ 
    for( ; index < pTxnList->count ; index++)
    {
        if(pTxnList->pJobHdrNode[index]->jobHdr.numJobs)
        {
            if(pTxnList->pJobHdrNode[index]->head->job.op != CL_COR_OP_CREATE &&
                    pTxnList->pJobHdrNode[index]->head->job.op != CL_COR_OP_CREATE_AND_SET)
            {
                clCorMoIdInitialize(&moId);
                moId = pTxnList->pJobHdrNode[index]->jobHdr.moId ;

                clLogTrace("TXN", "PRE", "Validating the lock for the MO[%s].", _clCorMoIdStrGet(&moId, moIdStr));
                
                rc = moId2DMHandle(&moId, &dmH);
                if(CL_OK != rc)
                {
                    clHeapFree(pHdr);
                    clLogError("TXN", "PRE", "Failed while getting the Dm handle for the MO[%s]. rc[0x%x]", moIdStr, rc);
                    return rc;
                } 

                DM_OH_GET_TYPE(dmH, pClassH);
                if(NULL == pClassH)
                {
                    clHeapFree(pHdr);
                    clLogError("TXN", "PRE", "Failed while getting the class handle for MO[%s]", moIdStr);
                    return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                }
            
                objBuf = corVectorGet(&(pClassH->objects), dmH.instanceId);
                if(NULL == objBuf)
                {
                    clHeapFree(pHdr);
                    clLogError("TXN", "PRE", "The object buffer is NULL for the MO [%s]", moIdStr);
                    return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
                }

                pHdr[index] = (CORInstanceHdr_h)objBuf;
                if( ((*(CORInstanceHdr_h)pHdr[index]) & CL_COR_OBJ_TXN_STATE_SET_INPROGRESS ) ||
                    ((*(CORInstanceHdr_h)pHdr[index]) & CL_COR_OBJ_TXN_STATE_DELETE_INPROGRESS) )
                {
                    clHeapFree(pHdr);
                    clLogInfo("TXN","PRE", "The MO [%s] has a transaction in progress. so retrying ", moIdStr);
                    return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
                }
            }
        }
        else
        {
            clLogDebug("TXN", "OPE", "There are no jobs in the header, only operational jobs present");
            return CL_OK;
        }
    }


    for ( index = 0 ; index < pTxnList->count ; index++ )
    {
        if(pTxnList->pJobHdrNode[index]->head->job.op != CL_COR_OP_CREATE &&
                pTxnList->pJobHdrNode[index]->head->job.op != CL_COR_OP_CREATE_AND_SET)
        {
            if (pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_SET)
                hdr = (CORInstanceHdr_t)((*(CORInstanceHdr_h)pHdr[index]) | CL_COR_OBJ_TXN_STATE_SET_INPROGRESS);
            else if (pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_DELETE)
                hdr = (CORInstanceHdr_t)((*(CORInstanceHdr_h)pHdr[index]) | CL_COR_OBJ_TXN_STATE_DELETE_INPROGRESS);

            memcpy(pHdr[index], &hdr, sizeof(CORInstanceHdr_t));
            hdr = 0;
        }
    }

    clHeapFree(pHdr);
    return rc;
} 


/**
 * Function to unset the in-progress transaction status to complete.
 */ 
ClRcT _clCorTxnJobInprogressUnset (ClCorTxnJobListT *pTxnList)
{
    ClRcT           rc = CL_OK;
    DMObjHandle_t   dmH = {0};
    CORClass_h      pClassH = NULL;
    Byte_h          objBuf = NULL;
    CORInstanceHdr_t hdr = CL_COR_OBJ_TXN_STATE_INVALID;
    ClCharT          moIdStr[CL_MAX_NAME_LENGTH] = {0};
    ClCorMOIdT       moId ;
    ClUint32T        index = 0;

    for( ; index < pTxnList->count ; index++)
    {
        if(pTxnList->pJobHdrNode[index]->jobHdr.numJobs)
        {
            if (pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_SET ||
                    pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_DELETE)
            {
                clCorMoIdInitialize(&moId);
                moId = pTxnList->pJobHdrNode[index]->jobHdr.moId ;

                clLogTrace("TXN", "PRE", "Unlocking the MO[%s] as the operation is done.", _clCorMoIdStrGet(&moId, moIdStr));
                
                rc = moId2DMHandle(&moId, &dmH);
                if(CL_OK != rc)
                {
                    if(pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_SET)
                        clLogError("TXN", "PRE", "Failed while getting the Dm handle for the MO[%s]. rc[0x%x]", moIdStr, rc);
                    continue;
                } 

                DM_OH_GET_TYPE(dmH, pClassH);
                if(NULL == pClassH)
                {
                    clLogNotice("TXN", "PRE", "Failed while getting the class handle for MO[%s] \
                            while clearing the status", moIdStr);
                    continue;
                }
            
                objBuf = corVectorGet(&(pClassH->objects), dmH.instanceId);
                if(NULL == objBuf)
                {
                    clLogError("TXN", "PRE", "The object buffer is NULL for the MO [%s]", moIdStr);
                    continue;
                }

                if (pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_SET)
                    hdr = (*(CORInstanceHdr_h) objBuf) & (~CL_COR_OBJ_TXN_STATE_SET_INPROGRESS);

                else if (pTxnList->pJobHdrNode[index]->head->job.op == CL_COR_OP_DELETE)
                    hdr = (*(CORInstanceHdr_h) objBuf) & (~CL_COR_OBJ_TXN_STATE_DELETE_INPROGRESS);

                memcpy(objBuf, &hdr, sizeof(CORInstanceHdr_t));
            }
        }
        else
        {
            clLogDebug("TXN", "OPE", "No jobs in job header, only operational jobs exists");
        }
    }

    return CL_OK;
}

static ClRcT _dmClassInitAttrGet(CORAttr_h attrH, Byte_h* buff)
{
    ClCorAttrInitInfoListT* pAttrInitList = (ClCorAttrInitInfoListT *) buff;
    ClCorAttrInitInfoT* pAttrInitInfo = NULL;

    if (attrH->userFlags & CL_COR_ATTR_INITIALIZED)
    {
        pAttrInitInfo = (pAttrInitList->pAttrInitInfo + pAttrInitList->numAttrs);

        pAttrInitInfo->attrId = attrH->attrId;
        pAttrInitInfo->offset = attrH->offset;
        pAttrInitInfo->type = attrH->attrType.type;
        pAttrInitInfo->dataType = attrH->attrType.u.arrType;
        pAttrInitInfo->size = dmClassAttrTypeSize(attrH->attrType, attrH->attrValue);

        pAttrInitInfo->pValue = clHeapAllocate(pAttrInitInfo->size);
        if (!pAttrInitInfo->pValue)
        {
            clLogError("TXN", "PRE", "Failed to allocate memory.");
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        memset(pAttrInitInfo->pValue, 0, pAttrInitInfo->size);

        pAttrInitList->numAttrs++;
    }

    return CL_OK;
}

/**
 *  Function to validate the value of initailzed attributes
 *  for their uniqueness amongst the various instances of the
 *  class.
 */
ClRcT   
_clCorInitializedAttrValueValidate ( CORClass_h classH, 
                                     ClUint32T instanceId,
                                     ClUint32T  numInitAttr,
                                     ClCorTxnObjJobNodeT *pObjJobNode,
                                     ClCorOpsT op)
{
    ClRcT                   rc = CL_OK;
    Byte_h                  objBuf = NULL;
    Byte_h                  tempBuf = NULL;
    ClUint32T               i = 0, j = 0, numOfMatches = 0;
    ClInt32T                index = 0;
    ClCntNodeHandleT        node = 0;
    ClCorTxnAttrSetJobNodeT *pAttrSetJobNode = NULL;
    ClCorAttrInitInfoListT  attrInitList = {0};

    if (classH->objCount == 0)
    {
        clLogNotice("TXN", "PRE", "There are no instances present to check the "
                "value of initailized attributes.");
        return CL_OK;
    }

    clLogDebug ("TXN", "PRE", "Inside the function to validate the uniqeness of "
            "the initialized attrbiute across all the instances. ");

    /* Fetch the initialized attr ids. */
    attrInitList.pAttrInitInfo = (ClCorAttrInitInfoT *) clHeapAllocate(classH->nAttrs * sizeof(ClCorAttrInitInfoT));
    if (!attrInitList.pAttrInitInfo)
    {
        clLogError("TXN", "PRE", "Failed to allocate memory.");
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }

    memset(attrInitList.pAttrInitInfo, 0, classH->nAttrs * sizeof(ClCorAttrInitInfoT));

    rc = dmClassAttrWalk(classH, NULL, _dmClassInitAttrGet, (Byte_h*) &attrInitList);
    if (rc != CL_OK)
    {
        clLogError("TXN", "PRE", "Failed to walk the class [0x%x] attributes. rc [0x%x]",
                    classH->classId, rc);
        clHeapFree(attrInitList.pAttrInitInfo);
        return rc; 
    }

    clLogNotice("TXN", "PRE", "No. of initialized attributes found in the class [%d] is [%u].",
            classH->classId, attrInitList.numAttrs);

    if ((op != CL_COR_OP_CREATE_AND_SET) &&
            (numInitAttr != attrInitList.numAttrs))
    {
        Byte_h objBuf = NULL;
        /* 
         * Populate the attributes values from COR only 
         * if it is a SET operation and the no. of init attributes 
         * set is not equal to the total no. of init attributes.
         * Otherwise it would be filled in the following loop.
         */

        objBuf = corVectorGet(&classH->objects, instanceId);
        if (!objBuf)
        {
            clLogError("TXN", "PRE", "Failed to get the object buffer for dm instance id : [%u]", instanceId);
            rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            goto handleError;
        }

        objBuf = objBuf + sizeof(CORInstanceHdr_t);

        for (i=0; i < attrInitList.numAttrs; i++)
        {
            ClCorAttrInitInfoT* pAttrInitInfo = NULL;

            pAttrInitInfo = (ClCorAttrInitInfoT *) (attrInitList.pAttrInitInfo + i);

            clLogTrace("TXN", "PRE", "Reading init attr [%d] info : offset : [%d], size : [%d].",
                    pAttrInitInfo->attrId, pAttrInitInfo->offset, pAttrInitInfo->size);

            memcpy(pAttrInitInfo->pValue, (objBuf + pAttrInitInfo->offset), pAttrInitInfo->size);
        }
    }

    pAttrSetJobNode = pObjJobNode->head;
    
    while (pAttrSetJobNode)
    {
        for (i=0; i < attrInitList.numAttrs; i++)
        {
            ClCorAttrInitInfoT* pAttrInitInfo = NULL;

            pAttrInitInfo = (attrInitList.pAttrInitInfo + i);

            if (pAttrInitInfo->attrId == pAttrSetJobNode->job.attrId)
            {
                /* Copy the value. */
                if (pAttrInitInfo->type == CL_COR_ARRAY_ATTR)
                {
                    index = (pAttrSetJobNode->job.index != CL_COR_INVALID_ATTR_IDX ? 
                        (pAttrSetJobNode->job.index * clCorTestBasicSizeGet(pAttrInitInfo->dataType)): 0);

                    memcpy((ClUint8T *) pAttrInitInfo->pValue + index, pAttrSetJobNode->job.pValue, pAttrSetJobNode->job.size);
                }
                else
                {
                    memcpy(pAttrInitInfo->pValue, pAttrSetJobNode->job.pValue, pAttrSetJobNode->job.size);
                }
            }
        }

        pAttrSetJobNode = pAttrSetJobNode->next;
    }

    for (i = 0; i < classH->recordId - 1; i++)
    {
        rc = COR_LLIST_FIND(classH->objFreeList, (ClWordT)i, node);
        if (rc == CL_OK)
        {
            clLogInfo("TXN", "PRE", "The instance [%d] is in the deleted object list.", i);
            continue;
        }

        if (op != CL_COR_OP_CREATE_AND_SET && i == instanceId)
        {
            /* Need not compare with the same object's initialized attributes values. 
             * Continue with the next instance.
             */
            continue;
        }

        objBuf = corVectorGet(&(classH->objects), i);
        if (objBuf == NULL)
        {
            clLogError("TXN", "PRE", "For the instance [%d[ got the instance buffer as NULL.", i);
            rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
            goto  handleError;
        }

        objBuf = (Byte_h) objBuf + sizeof(CORInstanceHdr_t);

        for (j = 0; j < attrInitList.numAttrs; j++)
        {
            ClCorAttrInitInfoT* pAttrInitInfo = (attrInitList.pAttrInitInfo + j);

            clLogTrace("TXN", "PRE", "The AttrId [%d], its offset is [%d], size [%d]", 
                    pAttrInitInfo->attrId, pAttrInitInfo->offset, pAttrInitInfo->size);

            tempBuf = objBuf + pAttrInitInfo->offset;

            if (memcmp (tempBuf, pAttrInitInfo->pValue, pAttrInitInfo->size) == 0)
            {
                clLogDebug("TXN", "PRE", 
                    "The value of an initialized attribute [0x%x]  is equal "
                    " to the same attribute of [%d] instance of the class.", pAttrInitInfo->attrId, i);
                numOfMatches++;
            }
            else
            {
                /* Initialized attribute's value doesn't match in this object instance. */
                break;
            }
        }
        
        if (numOfMatches == attrInitList.numAttrs)
        {
            clLogError("TXN", "PRE", "The initialized attribute(s) value(s) supplied for the "
                    "object is non-unique with the existing object instances.");
            rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
            goto handleError;
        }

        numOfMatches = 0;
    }

    rc = CL_OK;
    clLogDebug("TXN", "PRE", "The instance being created is having unique value"
            "for its initialized attributes.");

handleError:
    for (i=0; i < attrInitList.numAttrs; i++)
    {
        ClCorAttrInitInfoT* pAttrInitInfo = (attrInitList.pAttrInitInfo + i);
        clHeapFree(pAttrInitInfo->pValue);
    }

    clHeapFree(attrInitList.pAttrInitInfo);

    return rc;
}
