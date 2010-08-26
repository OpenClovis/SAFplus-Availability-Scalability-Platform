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
 * File        : clCorTxnClient.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements client utilities for COR-Transaction Management
 *****************************************************************************/

/* INCLUDES */

#include <string.h> 
#include <clCommon.h> 
#include <clOsalApi.h> 
#include <clBitApi.h> 
#include <clTxnApi.h>
#include <clLogApi.h>
#include <clDebugApi.h> 
#include <clCorErrors.h> 
#include <clCorTxnApi.h>
#include <clCorLog.h>
#include "clCorTxnClientIpi.h"
#include <clCorClient.h>
#include <clCorRMDWrap.h>
#include <clCorApi.h>
#include <clHandleApi.h>

/* Container to store the failed job information */
extern ClCntHandleT gCorTxnInfo;

/* Forward Declaration */
static ClRcT _clCorTxnJobWalk(ClCorTxnJobHeaderNodeT*, ClCorTxnFuncT, void *cookie);

/* Function to reset pointer. Need to be done before finalizing the bundle. */
extern ClRcT
_clCorBundleResetPointer(ClCorBundleInfoPtrT pBundleInfo);

/* Bundle Mutex */
extern ClOsalMutexIdT   gCorBundleMutex;

/* Bundle handle Destroy */
extern ClRcT
clCorBundleHandleDestroy(  CL_OUT  ClHandleT dBhandle);

/**
 *  Walk all jobs on the list.
 *
 *  This function invokes the user supplied routine
 * on each of the jobs of the given transaction.
 *
 *  @param pThis	COR transaction descriptor.
 *  @param funcPtr	Pointer to user callback function
 *  @param flag	walk options, Fifo = 0, Lifo = 1
 *  @param cookie	shall be a parameter of the user callback function
 *
 *  @returns CL_OK  - Success<br 
 */
ClRcT  
clCorTxnJobWalk(
    CL_IN   ClCorTxnIdT         pThis, 
    CL_IN   ClCorTxnFuncT       funcPtr, 
    CL_IN   void               *cookie)
{
    ClRcT  rc  =  CL_OK;

    CL_FUNC_ENTER(); 


    if ( (funcPtr == NULL) || (pThis == 0) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
  
    if ((rc = _clCorTxnJobWalk((ClCorTxnJobHeaderNodeT*)pThis, funcPtr, cookie)) != CL_OK)
    {
        if (rc == CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Job walk terminated. rc [0x%x]\n", rc));
        else        
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Job Walk failed. rc [0x%x]\n", rc));
    } 
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This function commits a given transaction.
 */
ClRcT  clCorTxnSessionCommit(
    CL_IN   ClCorTxnSessionIdT    txnSessionId)
{
    ClRcT           rc      = CL_OK;
    ClCorTxnSessionT        txnSession;

    CL_FUNC_ENTER(); 

    if (txnSessionId == NULL)
    {
        clLogWarning("COR", "TXN", "Transaction Session Id is NULL.");
        return CL_OK;
    }

    txnSession.txnSessionId    = txnSessionId;
    txnSession.txnMode  = CL_COR_TXN_MODE_COMPLEX;

    /*
       Before invoking COR-Server to commit the transaction, 
       pack the locally available transaction information
    */
    rc = _clCorTxnSessionCommit(txnSession);
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This function cancels a given transaction.
 */
ClRcT clCorTxnSessionCancel(
    CL_IN   ClCorTxnSessionIdT     txnSessionId)
{
    ClRcT              rc = CL_OK;

    CL_FUNC_ENTER();

    rc = clCorTxnSessionFinalize(txnSessionId); 
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nFailed to delete the session. rc [0x%x]\n", rc));
        return rc;
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCorTxnSessionFinalize(
    CL_IN   ClCorTxnSessionIdT     txnSessionId)
{
    ClRcT              rc = CL_OK;
    ClCorTxnSessionT  corTxnSession = {txnSessionId, CL_COR_TXN_MODE_COMPLEX};

    CL_FUNC_ENTER();

    if (txnSessionId == NULL)
    {
        clLogTrace("COR", "TXN", "Transaction Session Id is NULL.");
        return CL_OK;
    }

    rc = clCorTxnSessionDelete(corTxnSession);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nFailed to delete the session. rc [0x%x]\n", rc));
    }

    rc = clCorTxnFailedJobCleanUp(txnSessionId);

    if (rc != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to cleanup the container : gCorTxnInfo. rc [0x%x]", rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal Method.
 *
 * This function invokes user supplied method for cor-txn job/operation on a given
 * object.
 */
ClRcT  
_clCorTxnJobWalk(
    CL_IN   ClCorTxnJobHeaderNodeT*    pThis, 
    CL_IN   ClCorTxnFuncT              funcPtr, 
    CL_IN   void                      *cookie)
{ 
    ClRcT  rc  =  CL_OK;
    ClCorTxnObjJobNodeT      *pObjJobNode;
    ClCorTxnAttrSetJobNodeT  *pAttrSetJobNode;
    ClUint16T                 objJobId = 0;
    ClUint16T                 attrSetJobId = 0;
   
    ClCorTxnJobIdT        jobId = 0;
    
    CL_COR_FUNC_ENTER("TXN", "WLK"); 
    
    if ( (funcPtr == NULL) || (pThis == NULL) )
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "WLK", "Invalid input parameter");
        CL_COR_FUNC_EXIT("TXN", "WLK");
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    } 
    
    pObjJobNode = pThis-> head;
  
    if(pObjJobNode == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "WLK", "ZERO obj-jobs in the txn-job.");
        CL_COR_FUNC_EXIT("TXN", "WLK");
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));   
    }
    
    /* Traverse thru' all the nodes and call the user supplied input routine */ 
    while(pObjJobNode)
    {
        ++objJobId;
        CL_COR_TXN_OBJ_JOB_ID_SET(jobId, objJobId);
      
        /* CREATE or DELETE job. Does not need furhter description. */
        if((pObjJobNode->job.op != CL_COR_OP_SET) && (pObjJobNode->job.op != CL_COR_OP_GET))
        { 
  
           CL_COR_TXN_ATTRSET_JOB_ID_SET(jobId, 0);
           rc = funcPtr((ClCorTxnIdT)pThis, (ClCorTxnJobIdT)jobId, cookie);
           if(rc != CL_OK)
           {
              if (rc == CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
              {
                clLog(CL_LOG_SEV_TRACE, "TXN", "WLK", "Job walk is terminated by the user. rc [0x%x]", rc);
                CL_COR_FUNC_EXIT("TXN", "WLK");
                return rc;
              }
             
              clLog(CL_LOG_SEV_ERROR, "TXN", "WLK", "User supplied input routine in JobWalk failed. rc [0x%x]", rc);
              CL_COR_FUNC_EXIT("TXN", "WLK");
              return rc;
           }
        }
        else
        {
              pAttrSetJobNode = pObjJobNode-> head;
              attrSetJobId = 0;

              if(pAttrSetJobNode == NULL)
              {
                  clLog(CL_LOG_SEV_ERROR, "TXN", "WLK", "ZERO SET jobs in the obj-job");
                  CL_COR_FUNC_EXIT("TXN", "WLK");
                  return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
              }

              while(pAttrSetJobNode)
              {
                    ++attrSetJobId;
                    CL_COR_TXN_ATTRSET_JOB_ID_SET(jobId, attrSetJobId);
                    rc = funcPtr((ClCorTxnIdT)pThis, (ClCorTxnJobIdT)jobId, cookie);
                    if(rc != CL_OK)
                    {
                        if (rc == CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
                        {
                            clLog(CL_LOG_SEV_TRACE, "TXN", "WLK", "Job walk is terminated by the user. rc [0x%x]", rc);
                            CL_COR_FUNC_EXIT("TXN", "WLK");
                            return rc;
                        }
                  
                        clLog(CL_LOG_SEV_ERROR, "TXN", "WLK", "User supplied input routine failed in Job Walk, rc [0x%x]", rc);
                        CL_COR_FUNC_EXIT("TXN", "WLK");
                        return rc;
                    }

                    pAttrSetJobNode = pAttrSetJobNode-> next;  /* FIFO */
             }
        }
            pObjJobNode = pObjJobNode-> next;  /* FIFO */
    }
    
    CL_COR_FUNC_EXIT("TXN", "WLK");
    return (CL_OK); 
}


/**
 *  Return cor-txnId  given COR notification data.
 *
 *  This function returns reference to the txn-Id after
 *  copying the data from txn.
 *
 *  @param jobDefn   jobDefn   Handle which is passed to Commit callout of txn agent.
 *         ClSizeT   size      size  which is passed to Commit callout of txn agent.
 *         pTransH  [OUT] TXN handle is retunrd here.
 *
 *  @returns CL_OK on success<br>
 *       CL_NU
 */
ClRcT
clCorTxnJobHandleToCorTxnIdGet(
       CL_IN    ClTxnJobDefnHandleT   jobDefn,
       CL_IN    ClSizeT               size, 
       CL_OUT   ClCorTxnIdT          *pTxnId)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT   msgHdl;
    ClCorTxnJobHeaderNodeT  *pJobHdrNode;


    if((rc = clBufferCreate(&msgHdl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed "));
        return (rc);
    }    
    if((rc = clBufferNBytesWrite(msgHdl, jobDefn, size))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Write Failed "));
        return (rc);
    }

    if(( rc = clCorTxnJobStreamUnpack(msgHdl, &pJobHdrNode))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Cor-Txn Job Stream Unpack Failed "));
        return (rc);
    }

    if ((rc = clBufferDelete(&msgHdl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Deletion Failed. rc [0x%x]\n", rc));
        return (rc);
    }

   /* Update the Tid */
    *pTxnId =  (ClCorTxnIdT )pJobHdrNode;
 
    CL_FUNC_EXIT();
    return (rc);
}

/*
 *
 * This API frees the memory for the transaction, which was allocated when
 * clCorTxnJobHandleToCorTxnIdGet or clCorEventToCorTxnIdGet was called. 
 */
ClRcT
clCorTxnIdTxnFree(
       CL_IN    ClCorTxnIdT    corTxnId)
{
    if(!corTxnId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
    clCorTxnJobHeaderNodeDelete((ClCorTxnJobHeaderNodeT *)corTxnId);
  return (CL_OK);
}

/*
 *  API to get the operation type from jobId.
 *
 */
ClRcT  clCorTxnJobOperationGet(
         CL_IN     ClCorTxnIdT    txnId,
         CL_IN     ClCorTxnJobIdT jobId,
         CL_OUT    ClCorOpsT          *op)
{ 
  ClRcT                 rc = CL_OK;
  ClUint16T             objJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  
   if(!op)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
 
  if(!txnId || !jobId) 
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   } 
 
   objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);

   pObjJobNode = jobHeaderNode->head;

  /* Job Id starts from 1. */

   while((--objJobId) && pObjJobNode)
   {
     pObjJobNode = pObjJobNode->next;
   }
 
   if(pObjJobNode)  
      *op = pObjJobNode->job.op;
   else
    rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); /* Internal Error */
  
  return (rc);
}


/*
 *  API to get the moId form txnId.
 *
 */
ClRcT  clCorTxnJobMoIdGet(
         CL_IN     ClCorTxnIdT  txnId,
         CL_OUT    ClCorMOIdT  *pMOId)
{ 

   ClCorTxnJobHeaderNodeT  *pJobHeader = (ClCorTxnJobHeaderNodeT *)txnId;

   if(!pMOId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   /* Write the object handle */
       *pMOId = pJobHeader->jobHdr.moId;
   return (CL_OK);
}


/*
 *  API to get the objectHandle form txnId.
 *
 */
ClRcT  clCorTxnJobObjectHandleGet(
         CL_IN     ClCorTxnIdT  txnId,
         CL_OUT    ClCorObjectHandleT  *pObjHandle)
{ 
   ClCorTxnJobHeaderNodeT  *pJobHeader = (ClCorTxnJobHeaderNodeT *)txnId;

   if(!pObjHandle)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   /* Write the object handle */
       *pObjHandle = pJobHeader->jobHdr.oh;

   return (CL_OK);
}


/*
 *  API to get the OM class form txnId.
 *
 */
ClRcT  clCorTxnJobOmClassIdGet(
         CL_IN     ClCorTxnIdT  txnId,
         CL_OUT    ClCorClassTypeT  *pOmClassId)
{ 

   ClCorTxnJobHeaderNodeT  *pJobHeader = (ClCorTxnJobHeaderNodeT *)txnId;

   if(NULL == pOmClassId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   /* Write the OM Class Id */
   *pOmClassId = pJobHeader->jobHdr.omClassId;
   return (CL_OK);
}

/*
 *  IPI to set the value of the attribute in network format given the txnId and jobId.
 */
ClRcT clCorBundleAttrValueSet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_IN   ClPtrT               *pValue)
{  

    ClRcT                   rc                  = CL_OK;
    ClUint16T               objJobId            = 0;
    ClUint16T               attrSetJobId        = 0;
    ClCorTxnJobHeaderNodeT  *jobHeaderNode      = (ClCorTxnJobHeaderNodeT *)txnId;
    ClCorTxnObjJobNodeT     *pObjJobNode        = NULL;
    ClCorTxnAttrSetJobNodeT *pAttrSetJobNode    = NULL;
  
    if(!pValue)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
                                                                                                                        
    if(!txnId || !jobId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }

    objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);
    attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId);
 
    if(attrSetJobId == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Job Id passed is not that of a SET Job"));
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
    }

    pObjJobNode = jobHeaderNode->head;

    /* Job Id starts from 1. */
    while((--objJobId) && pObjJobNode)
    {
        pObjJobNode = pObjJobNode->next;
    }

    if(pObjJobNode)
    {
        pAttrSetJobNode = pObjJobNode->head;
        /* Job Id starts from 1. */
        while((--attrSetJobId) && pAttrSetJobNode)
        {
            pAttrSetJobNode = pAttrSetJobNode->next;
        }
    }
 
    if(pAttrSetJobNode)  
    {
        memcpy(pAttrSetJobNode->job.pValue, pValue, pAttrSetJobNode->job.size);
        if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
        {
            /* convert endianness. */
            if((rc =  clCorObjAttrValSwap ( pAttrSetJobNode->job.pValue,
                                            pAttrSetJobNode->job.size,
                                              pAttrSetJobNode->job.attrType,
                                                pAttrSetJobNode->job.arrDataType))!= CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to do endian conversion on user Data. "));
                return (rc);
            }
        }
    }
    else
        rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); /* Internal Error */

   return (CL_OK);
}

/*
 *  API to get the SET Parameters, if the operation is SET.
 *  @param: Flag = 0, get the value in Network order.
 *          Flag = 1, get the value in Host order
 *  The pValue returned here, would be in Network Order.
 */
ClRcT _clCorTxnJobSetParamsGet(
          CL_IN   ClUint8T             flag,
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_OUT  ClCorAttrIdT        *pAttrId,
          CL_OUT  ClInt32T            *pIndex,
          CL_OUT  void               **pValue,
          CL_OUT  ClUint32T           *pSize)
{  

  ClRcT                 rc = CL_OK;
  ClUint16T             objJobId = 0;
  ClUint16T             attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  ClCorTxnAttrSetJobNodeT  *pAttrSetJobNode = NULL;
  
 if(!pAttrId || !pIndex || !pValue || !pSize )
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId || !jobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);
   attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId);
 
   if(attrSetJobId == 0)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Job Id passed is not that of a SET Job"));
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
   }

   pObjJobNode = jobHeaderNode->head;

  /* Job Id starts from 1. */
   while((--objJobId) && pObjJobNode)
   {
     pObjJobNode = pObjJobNode->next;
   }

   if(pObjJobNode)
   {
      pAttrSetJobNode = pObjJobNode->head;
     /* Job Id starts from 1. */
       while((--attrSetJobId) && pAttrSetJobNode)
       {
         pAttrSetJobNode = pAttrSetJobNode->next;
       }
   }
 
   if(pAttrSetJobNode)  
   {
      *pAttrId = pAttrSetJobNode->job.attrId;
      *pIndex =  pAttrSetJobNode->job.index;
      *pSize  =  pAttrSetJobNode->job.size;

      /* Get the value in host format. */
      if(flag == 1)
      {  
          if(CL_BIT_LITTLE_ENDIAN == clBitBlByteEndianGet())
          {
              /* convert endianness. */
               if((rc =  clCorObjAttrValSwap(pAttrSetJobNode->job.pValue,
                                                pAttrSetJobNode->job.size,
                                                  pAttrSetJobNode->job.attrType,
                                                    pAttrSetJobNode->job.arrDataType))!= CL_OK)
               {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to do endian conversion on user Data. "));
                    return (rc);
               }
          }
       }

       *pValue = pAttrSetJobNode->job.pValue;
   }
   else
    rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); /* Internal Error */
    
   return (CL_OK);
}

/*
 *  API to get the SET Parameters, if the operation is SET.
 *  The pValue returned is in Host Order.
 */
ClRcT clCorTxnJobSetParamsGet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_OUT  ClCorAttrIdT        *pAttrId,
          CL_OUT  ClInt32T            *pIndex,
          CL_OUT  void               **pValue,
          CL_OUT  ClUint32T           *pSize)
{  

  ClRcT                 rc = CL_OK;

  if(!pAttrId || !pIndex || !pValue || !pSize )
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId || !jobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

     rc = _clCorTxnJobSetParamsGet(1, txnId, jobId, pAttrId, pIndex, pValue, pSize);
   return (rc);
}


/*
 *  API to get the attribute Type information. 
 */
ClRcT clCorTxnJobAttributeTypeGet(
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId, 
          CL_OUT  ClCorAttrTypeT      *pAttrType,
          CL_OUT  ClCorTypeT          *pAttrDataType)
{  

  ClRcT                     rc = CL_OK;
  ClUint16T                 objJobId = 0;
  ClUint16T                 attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  ClCorTxnAttrSetJobNodeT  *pAttrSetJobNode = NULL;

  if(!pAttrType || !pAttrDataType)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId || !jobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);
   attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId);

   pObjJobNode = jobHeaderNode->head;

   if(attrSetJobId == 0)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Job Id passed is not that of a SET Job"));
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
   }
  /* Job Id starts from 1. */
   while((--objJobId) && pObjJobNode)
   {
     pObjJobNode = pObjJobNode->next;
   }

   if(pObjJobNode)
   {
      pAttrSetJobNode = pObjJobNode->head;
     /* Job Id starts from 1. */
       while((--attrSetJobId) && pAttrSetJobNode)
       {
         pAttrSetJobNode = pAttrSetJobNode->next;
       }
   }
 
   if(pAttrSetJobNode)  
   {
      if(pAttrSetJobNode->job.attrType != CL_COR_ARRAY_ATTR && 
              pAttrSetJobNode->job.attrType != CL_COR_ASSOCIATION_ATTR)
      {
          *pAttrType = CL_COR_SIMPLE_ATTR;
          *pAttrDataType = pAttrSetJobNode->job.attrType;
      }
      else if(pAttrSetJobNode->job.attrType == CL_COR_ARRAY_ATTR)
      {
          *pAttrType = CL_COR_ARRAY_ATTR; 
          *pAttrDataType = pAttrSetJobNode->job.arrDataType;
      }
      else
      { 
         /* This is association attribute */
          *pAttrType = pAttrSetJobNode->job.attrType;
          *pAttrDataType = CL_COR_INVALID_DATA_TYPE;
      }
   }
   else
     rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); /* Internal Error */

   return (rc);
}




/*
 *  API to get the AttrPath, given the Job handle.
 *
 */
ClRcT  clCorTxnJobAttrPathGet(
          CL_IN     ClCorTxnIdT         txnId,
          CL_IN     ClCorTxnJobIdT      jobId,
          CL_OUT    ClCorAttrPathT    **pAttrPath)
{ 
  ClRcT                 rc = CL_OK;
  ClUint16T             objJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  

  if(!pAttrPath)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId || !jobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }


   objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);

   pObjJobNode = jobHeaderNode->head;
  
   if (NULL == pObjJobNode)
   {
       clLogError("COR", "APG", "The Object job obtained is NULL");
       return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
   }

  /* Job Id starts from 1. */
   while((--objJobId) && pObjJobNode)
   {
     pObjJobNode = pObjJobNode->next;
   }
 
   if((pObjJobNode != NULL) 
           && (pObjJobNode->job.op != CL_COR_OP_SET)
           && (pObjJobNode->job.op != CL_COR_OP_GET))
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Job Id passed is not that of a SET Job"));
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
   }

   if(pObjJobNode)  
      *pAttrPath = &pObjJobNode->job.attrPath;
   else
    rc = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR); /* Internal Error */
  return (rc);
}

/*
 *  API to get first Job in Txn.
 *
 */
ClRcT  clCorTxnFirstJobGet(
         CL_IN     ClCorTxnIdT      txnId,
         CL_OUT    ClCorTxnJobIdT  *pJobId)
{ 
  ClRcT                    rc = CL_OK;
  ClUint16T                objJobId = 0;
  ClUint16T                attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT  *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT     *pObjJobNode = NULL;
  
  if(!pJobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   if((pObjJobNode = jobHeaderNode->head) == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Transaction has NO jobs. "));
      return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
   }

   ++objJobId;
   /** Set the objJobId **/
    *pJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId); 

  /* checking if job is a SET Job. */
   if(pObjJobNode->head && pObjJobNode->job.op == CL_COR_OP_SET)
   {
      ++attrSetJobId;
     *pJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId); 
   }
   return (rc);
}


/*
 *  API to get Last Job in Txn.
 *
 */
ClRcT  clCorTxnLastJobGet(
         CL_IN     ClCorTxnIdT      txnId,
         CL_OUT    ClCorTxnJobIdT  *pJobId)
{ 
  ClRcT                 rc = CL_OK;
  ClUint16T             objJobId = 0;
  ClUint16T             attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  
   if(!pJobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
   if(!txnId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }

   if((pObjJobNode = jobHeaderNode->head) == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Transaction has NO jobs. "));
      return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
   }

   /* Get the last job */
   if ((pObjJobNode = jobHeaderNode->tail) == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Transaction has NO jobs. "));
      return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
   }
   
  /* numJobs equals the job id of last job. */
   objJobId = jobHeaderNode->jobHdr.numJobs;

  /** Set the objJobId **/
    *pJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId); 

  /* checking if job is a SET Job. */
   if(pObjJobNode->tail && pObjJobNode->job.op == CL_COR_OP_SET)
   {
      attrSetJobId = pObjJobNode->job.numJobs; /* numJobs equals the job Id of last job. */
     *pJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId); 
   }

  return (rc);
}

/*
 *  API to get Next Job in Txn.
 *
 */
ClRcT  clCorTxnNextJobGet(
         CL_IN     ClCorTxnIdT      txnId,
         CL_IN     ClCorTxnJobIdT   currentJobId,
         CL_OUT    ClCorTxnJobIdT  *pNextJobId)
{ 
  ClRcT                     rc = CL_OK;
  ClUint16T                 i = 0;
  ClUint16T                 objJobId = 0;
  ClUint16T                 attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  
   if(!txnId && !pNextJobId)
   {
      /* Error Condition */
   }

   if(!pNextJobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
   }
                                                                                                                            
  if(!txnId || !currentJobId)
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid input parameter"));
        return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
   }
 
   if((pObjJobNode = jobHeaderNode->head) == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Transaction has NO jobs. "));
      return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
   }
   
    objJobId  =  i = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(currentJobId);
    attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(currentJobId);

     if(i <= jobHeaderNode->jobHdr.numJobs)
     {
         if(i == jobHeaderNode->jobHdr.numJobs)
         {
            pObjJobNode = jobHeaderNode->tail;
            if(pObjJobNode->job.op != CL_COR_OP_SET)
            {
               return (CL_COR_SET_RC(CL_COR_TXN_ERR_LAST_JOB));
            }

         }
         else
         {
             pObjJobNode = jobHeaderNode->head;
             while((--i) && pObjJobNode)
             {
                pObjJobNode = pObjJobNode->next;
             }
         }
     }
     else
     {
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
     }
  
    if (pObjJobNode == NULL)
    {
        clLogError("COR", "NJG", 
                "The object job found as NULL while finding next attribute job");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

     if (pObjJobNode->job.op == CL_COR_OP_SET)
     {
          if(attrSetJobId <= pObjJobNode->job.numJobs)
          { 
              if(attrSetJobId != pObjJobNode->job.numJobs)
              {
                 ++attrSetJobId;
                 *pNextJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId);
                  return (rc);
              }
             else if(pObjJobNode == jobHeaderNode->tail)
                  {
                      return (CL_COR_SET_RC(CL_COR_TXN_ERR_LAST_JOB));
                  }
          } 
          else 
          {
             return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
          }
 
     }
       pObjJobNode = pObjJobNode->next;
     ++objJobId;
     if(pObjJobNode->job.op != CL_COR_OP_SET)
     {

        *pNextJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, 0);
     }
     else
     {
        *pNextJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, 1);
     }
   return (CL_OK);
}


/*
 *  API to get Previous Job Id in Txn.
 *
 */
ClRcT  clCorTxnPreviousJobGet(
         CL_IN     ClCorTxnIdT      txnId,
         CL_IN     ClCorTxnJobIdT   currentJobId,
         CL_OUT    ClCorTxnJobIdT  *pPrevJobId)
{ 
  ClRcT                     rc = CL_OK;
  ClUint16T                 i = 0;
  ClUint16T                 objJobId = 0;
  ClUint16T                 attrSetJobId = 0;
  ClCorTxnJobHeaderNodeT   *jobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
  ClCorTxnObjJobNodeT      *pObjJobNode = NULL;
  
    if(!pPrevJobId)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n NULL argument "));
         return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }
                                                                                                                            
    if(!txnId || !currentJobId)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Invalid input parameter"));
         return (CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM));
    }
  
    if((pObjJobNode = jobHeaderNode->head) == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Transaction has NO jobs. "));
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }

    objJobId  =  i = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(currentJobId);
    attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(currentJobId);

     if(i <= jobHeaderNode->jobHdr.numJobs)
     {
         if(i == 1)    /* This is first objJob*/
         {
            pObjJobNode = jobHeaderNode->head;
            if(pObjJobNode->job.op != CL_COR_OP_SET)   /* First job*/
            {
               return (CL_COR_SET_RC(CL_COR_TXN_ERR_FIRST_JOB));
            }

         }
         else
         {
             pObjJobNode = jobHeaderNode->head;
             while((--i) && pObjJobNode)
             {
                pObjJobNode = pObjJobNode->next;
             }
         }
     }
     else
     {
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
     }

     if (pObjJobNode == NULL)
     {
         clLogError("COR", "PJG", 
                 "The object job got null while finding the previous job. ");
         return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
     }
  
     if(pObjJobNode->job.op == CL_COR_OP_SET)
     {
          if(attrSetJobId <= pObjJobNode->job.numJobs)
          { 
              if(attrSetJobId != 1)  /* This is not first attrSetJob*/
              {
                 --attrSetJobId;
                 *pPrevJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId);
                  return (rc);
              }
             else if(pObjJobNode == jobHeaderNode->head) 
                  {
                       return (CL_COR_SET_RC(CL_COR_TXN_ERR_FIRST_JOB));
                  }
          } 
          else 
          {
             return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
          }
 
     }
       pObjJobNode = pObjJobNode->prev;
     --objJobId;
     if(pObjJobNode->job.op != CL_COR_OP_SET)
     {

        *pPrevJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, 0);
     }
     else
     {
         attrSetJobId = pObjJobNode->job.numJobs;
        *pPrevJobId = CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId);
     }
   return (CL_OK);
}

/*
 * To set the status of a particular job
 */

ClRcT clCorTxnJobStatusSet(
                    CL_IN   ClCorTxnIdT         txnId,
                    CL_IN   ClCorTxnJobIdT      jobId,
                    CL_IN   ClUint32T           jobStatus)
{ 
    ClUint16T                 objJobId = 0;
    ClUint16T                 attrSetJobId = 0;
    ClCorTxnJobHeaderNodeT    *pJobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
    ClCorTxnObjJobNodeT       *pObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT   *pAttrSetJobNode = NULL;

    CL_FUNC_ENTER();
   
    if ( (pObjJobNode = pJobHeaderNode->head) == NULL)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "JOB",
            "Transaction [%p] has no jobs", txnId);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }

    /** Retrieve the job id and sub job id */
    
    objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);
    attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId);
 
    /* Set the job header status */
    if (jobStatus != CL_OK)
    {
        pJobHeaderNode->jobHdr.jobStatus = CL_COR_TXN_JOB_FAIL;
    }

    if (objJobId <= pJobHeaderNode->jobHdr.numJobs)
    {
        /* Traverse to the required job */
        while ( (--objJobId) && pObjJobNode)
        {
            pObjJobNode = pObjJobNode->next;
        }

        /* Now pObjJobNode points to the required job */
        if (pObjJobNode != NULL)
        {
            /* Set the job status */
            if (jobStatus != CL_OK)
            {
                pObjJobNode->job.jobStatus = jobStatus;
            }
            
            if (pObjJobNode->job.op == CL_COR_OP_SET || pObjJobNode->job.op == CL_COR_OP_GET)
            {
                pAttrSetJobNode = pObjJobNode->head;

                if ((attrSetJobId <= pObjJobNode->job.numJobs) && (attrSetJobId != 0))
                {
                    while ( (--attrSetJobId) && pAttrSetJobNode)
                    {
                        pAttrSetJobNode = pAttrSetJobNode->next;
                    }

                    if (pAttrSetJobNode != NULL)
                    {
                        /* Set the sub job status */
                        pAttrSetJobNode->job.jobStatus = jobStatus;
                    }
                    else
                    {
                        clLog(CL_LOG_SEV_INFO, "TXN", "JOB",
                            "Error while accessing the attribute sub-job.");
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
                    }
                }
                else
                {
                    clLog(CL_LOG_SEV_INFO, "TXN", "JOB",
                        "Invalid Job Id Passed. Error while getting attribute sub-job id.");
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
                }
            }
        }
        else
        {
            clLog(CL_LOG_SEV_ERROR, "TXN", "JOB",
                "Error while accessing the job in the transaction");
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
        }
    }
    else
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "JOB",
            "Invalid Job Id [0x%x] passed", objJobId);
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 * To get the status of a particular job
 */

ClRcT clCorTxnJobStatusGet(
                    CL_IN       ClCorTxnIdT         txnId,
                    CL_IN       ClCorTxnJobIdT      jobId,
                    CL_OUT      ClUint32T*          jobStatus)
{ 
    ClUint16T                 objJobId = 0;
    ClUint16T                 attrSetJobId = 0;
    ClCorTxnJobHeaderNodeT    *pJobHeaderNode = (ClCorTxnJobHeaderNodeT *)txnId;
    ClCorTxnObjJobNodeT       *pObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT   *pAttrSetJobNode = NULL;

    CL_FUNC_ENTER();
   
    if ( (pObjJobNode = pJobHeaderNode->head) == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nTransaction has no jobs\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }

    /* Retrieve the job id and sub job id */
    
    objJobId = CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId);
    attrSetJobId = CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId);
 
    if (objJobId <= pJobHeaderNode->jobHdr.numJobs)
    {
        /* Traverse to the required job */
        while ( (--objJobId) && pObjJobNode)
        {
            pObjJobNode = pObjJobNode->next;
        }

        /* Now pObjJobNode points to the required job */
        if (pObjJobNode != NULL)
        {
            if (pObjJobNode->job.op == CL_COR_OP_SET)
            {
                pAttrSetJobNode = pObjJobNode->head;

                if ((attrSetJobId <= pObjJobNode->job.numJobs) && (attrSetJobId != 0))
                {
                    while ( (--attrSetJobId) && pAttrSetJobNode)
                    {
                        pAttrSetJobNode = pAttrSetJobNode->next;
                    }

                    if (pAttrSetJobNode != NULL)
                    {
                        /* Get the sub job status */
                        *jobStatus = pAttrSetJobNode->job.jobStatus;
                    }
                    else
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nJob doesn't exist. Error while accessing the attrSetSubJob.\n"));
                        CL_FUNC_EXIT();
                        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
                    }
                }
                else
                {
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nInvalid Job Id Passed. Error while getting attrSetJobId.\n"));
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
                }
            }
            else
            {
                /* Get the job status */
                *jobStatus = pObjJobNode->job.jobStatus;
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nJob doesn't exist. Error while getting jobId.\n"));
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nInvalid Job Id passed\n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_INVALID_JOB_ID));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* To retrieve the failed jobs from the container. Memory is allocated by client.
 */

ClRcT clCorTxnFailedJobGet(ClCorTxnSessionIdT txnSessionId, ClCorTxnInfoT *pPrevTxnInfo, ClCorTxnInfoT *pNextTxnInfo)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT node = 0;
    ClCorTxnInfoStoreT *pCorTxnInfoStore = NULL, *pTempCorTxnInfoStr = NULL;

    CL_FUNC_ENTER();

    pCorTxnInfoStore = (ClCorTxnInfoStoreT *) clHeapAllocate(sizeof(ClCorTxnInfoStoreT));
    
    if (pCorTxnInfoStore == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNo memory to allocate. rc [0x%x]\n", rc));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
    }
    memset(pCorTxnInfoStore, 0, sizeof(ClCorTxnInfoStoreT));

    /* Copy the session id */
    pCorTxnInfoStore->txnSessionId = txnSessionId;

    if (pPrevTxnInfo == NULL)
    {
        /* Get the first job */

        pCorTxnInfoStore->op = CL_COR_TXN_INFO_FIRST_GET;
        
        if ((rc = clCntNodeFind(gCorTxnInfo, (ClCntKeyHandleT)pCorTxnInfoStore, &node)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNo job exists. with this key. rc [0x%x]\n", rc));
            clHeapFree(pCorTxnInfoStore);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_NOT_EXIST));
        }

        if ((rc = clCntNodeUserKeyGet(gCorTxnInfo, node, (ClCntKeyHandleT *) &pTempCorTxnInfoStr)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNot able to get the data from the node. rc [0x%x]\n", rc));
            clHeapFree(pCorTxnInfoStore);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_GET));
        }
    }
    else
    {
        /* Get the next job */
        pCorTxnInfoStore->op = CL_COR_TXN_INFO_NEXT_GET;
        
        /* copy the txninfo information */
        memcpy(&pCorTxnInfoStore->txnInfo, pPrevTxnInfo, sizeof(ClCorTxnInfoT));
        
        if ((rc = clCntNodeFind(gCorTxnInfo, (ClCntKeyHandleT) pCorTxnInfoStore, &node)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nUnable to find the node. rc [0x%x]\n", rc));
            clHeapFree(pCorTxnInfoStore);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_GET));
        }
        
        if ((rc = clCntNextNodeGet(gCorTxnInfo, node, &node)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("\nUnable to find the next node. rc [0x%x]\n", rc));
            clHeapFree(pCorTxnInfoStore);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_NOT_EXIST));
        }

        if ((rc = clCntNodeUserKeyGet (gCorTxnInfo, node, (ClCntKeyHandleT *) &pTempCorTxnInfoStr)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nUnable to get the data from the node. rc [0x%x]\n", rc));
            clHeapFree(pCorTxnInfoStore);
            CL_FUNC_EXIT();
            return (CL_COR_SET_RC(CL_COR_TXN_ERR_FAILED_JOB_GET));
        }
    }
    
    memcpy(pNextTxnInfo, &pTempCorTxnInfoStr->txnInfo, sizeof(ClCorTxnInfoT));

    clHeapFree(pCorTxnInfoStore);

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clCorTxnJobDefnHandleUpdate(ClTxnJobDefnHandleT jobDefnHandle, ClCorTxnIdT corTxnId)
{
    ClRcT rc = CL_OK;
    ClUint8T *pData;
    ClBufferHandleT msgHdl;
    ClUint32T length;
    ClCorTxnJobHeaderNodeT *pCorTxnHdrNode = NULL;

    pCorTxnHdrNode = (ClCorTxnJobHeaderNodeT *) corTxnId;
    
    if((rc = clBufferCreate(&msgHdl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed. rc: 0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

     if((rc = clCorTxnJobStreamPack(pCorTxnHdrNode, msgHdl)) != CL_OK)
     {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Cor-Txn Job Stream pack Failed. rc: 0x%x", rc));
        clBufferDelete(&msgHdl);
        CL_FUNC_EXIT();
        return (rc);
     }

     if((rc = clBufferLengthGet(msgHdl, &length)) != CL_OK)
     {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Could not get buffer message length. rc: 0x%x", rc));
        clBufferDelete(&msgHdl);
        CL_FUNC_EXIT();
        return (rc);
     }

     pData = NULL;
     if((rc = clBufferFlatten(msgHdl, &pData)) != CL_OK)
     {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Flattening Failed. rc: 0x%x", rc));
        clBufferDelete(&msgHdl);
        CL_FUNC_EXIT();
        return (rc);
     }

    memcpy((void *) jobDefnHandle, (const void *) pData, length);

    clHeapFree(pData);
    clBufferDelete(&msgHdl);

    CL_FUNC_EXIT();
    return (CL_OK);
}


/**
 * Function to create the bundle.
 */

ClRcT
clCorBundleInitialize( CL_OUT ClCorBundleHandlePtrT pBundleHandle, 
                       CL_IN ClCorBundleConfigPtrT  pBundleConfig)
{
    ClRcT   rc = CL_OK;

    rc = _clCorBundleInitialize(pBundleHandle, pBundleConfig);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to initialize the bundle. rc[0x%x]", rc));
        return rc;
    }
    return rc;
}


/**
 * Function to apply the bundle asynchronously
 */

ClRcT clCorBundleApplyAsync (
                CL_IN ClCorBundleHandleT    bundleHandle,
                CL_IN const ClCorBundleCallbackPtrT    bundleCallback,
                CL_IN const ClPtrT                          userArg)
{
    ClRcT   rc = CL_OK;

    if (NULL == bundleCallback)
    {
        clLogError("BUN", "APL", 
                "The pointer to the callback is passed NULL for an async bundle.");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = _clCorBundleStart(bundleHandle, bundleCallback, userArg);
    if(rc != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while starting the bundle. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}


/**
 * Function to apply the bundle synchronously
 */

ClRcT clCorBundleApply ( CL_IN ClCorBundleHandleT  bundleHandle)
{
    ClRcT   rc = CL_OK;

    rc = _clCorBundleStart(bundleHandle, NULL, NULL);
    if(rc != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while starting the bundle. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}

typedef struct {
    ClOsalMutexIdT          corAttrShowMutex;
    ClOsalCondIdT           corAttrShowCond;
} BundleApplySyncCallbackDataT;
ClRcT clCorBundleApplySyncCallback(ClCorBundleHandleT  bundleHandle, ClPtrT usrArg);

ClRcT clCorBundleApplySync(CL_IN ClCorBundleHandleT  bundleHandle)
{
    BundleApplySyncCallbackDataT callbackData;
    ClRcT   rc = CL_OK;
    ClTimerTimeOutT  delay = {0,0};
    /* delay.tsMilliSec = timeout; */
    
    rc = clOsalMutexCreate(&callbackData.corAttrShowMutex);
    assert(rc==CL_OK);
    rc = clOsalCondCreate(&callbackData.corAttrShowCond);
    assert(rc==CL_OK);

    clOsalMutexLock(callbackData.corAttrShowMutex);

    rc = _clCorBundleStart(bundleHandle, clCorBundleApplySyncCallback, (ClPtrT*) &callbackData);
    if(rc != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while starting the bundle. rc[0x%x]", rc));
        return rc;
    }

    rc = clOsalCondWait(callbackData.corAttrShowCond, callbackData.corAttrShowMutex, delay);
    
    clOsalMutexUnlock(callbackData.corAttrShowMutex);
    
    clOsalCondDelete(callbackData.corAttrShowCond);
    clOsalMutexDelete(callbackData.corAttrShowMutex);
    
    //    clCorBundleFinalize(bundleHandle);
    return rc;
}

ClRcT clCorBundleApplySyncCallback(ClCorBundleHandleT  bundleHandle, ClPtrT usrArg)
{
    BundleApplySyncCallbackDataT* opInfo = (BundleApplySyncCallbackDataT*) usrArg;
    clOsalMutexLock(opInfo->corAttrShowMutex);
    clOsalCondSignal(opInfo->corAttrShowCond);
    clOsalMutexUnlock(opInfo->corAttrShowMutex);
    return CL_OK;
}


/**
 *  Function to finalize the bundle.
 */

ClRcT clCorBundleFinalize (
                CL_IN ClCorBundleHandleT bundleHandle)
{
    ClRcT   rc = CL_OK;

    rc = _clCorBundleFinalize(bundleHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while finalizing the bundle. rc[0x%x]", rc));
        return rc;
    }

    return rc;
}


