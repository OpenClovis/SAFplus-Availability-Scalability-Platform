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
 * File        : clCorTxnJobStream.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements stream (pack/unpack) utilities for COR-Transaction
 *****************************************************************************/
/* INCLUDES */
#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clBitApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorErrors.h>
#include <clCorLog.h>
#include <clCorTxnClientIpi.h>

/* IDL  */
#include <xdrClCorTxnJobHeaderT.h>
#include <xdrClCorTxnObjJobT.h>
#include <xdrClCorTxnAttrSetJobT.h>


#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif


/* Forward declaration */

/**
 *  Pack/Stream a transaction.
 *
 *  This routine packs/streams a transaction into continguous buffer.
 *  This buffer can be sent across CPU boundaries for processing.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorTxnJobStreamPack(
        CL_IN    ClCorTxnJobHeaderNodeT*    pThis, 
        CL_OUT   ClBufferHandleT     msgHdl)
{
    ClRcT rc = CL_OK;
    ClCorTxnObjJobNodeT     *pObjJobNodeS;
    ClCorTxnAttrSetJobNodeT *pAttrSetJobNodeS;
    ClUint32T i, j;

    CL_FUNC_ENTER();

    if(pThis == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL argument passed. \n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_NULL_PTR));
    }


    
    if((rc = VDECL_VER(clXdrMarshallClCorTxnJobHeaderT, 4, 0, 0)((void *)&pThis->jobHdr, msgHdl, 0)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClCorTxnJobHeaderT rc:0x%x", rc));
        return (rc);
    }
 
    pObjJobNodeS  = pThis->head;
    i = pThis->jobHdr.numJobs;
 
    if(pObjJobNodeS == NULL || i == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ZERO obj-jobs in the txn-job . \n"));
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
    }
 
    /* Pack all the ClCorTxnObjJob */
    while (pObjJobNodeS)
    {
        --i;
        if((rc = VDECL_VER(clXdrMarshallClCorTxnObjJobT, 4, 0, 0)((void *)&pObjJobNodeS->job, msgHdl, 0)) != CL_OK)
        {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClCorTxnObjJobT rc:0x%x", rc));
           return (rc);
        }
 
        /* Check if the object has SET or GET jobs */
        if(((pObjJobNodeS->job.op) == CL_COR_OP_SET )|| ((pObjJobNodeS->job.op) == CL_COR_OP_GET))
         {
           /* Pack all the ClCorTxnAttrSetJobT */
             pAttrSetJobNodeS  = pObjJobNodeS->head;
             j = pObjJobNodeS->job.numJobs;

            if(pAttrSetJobNodeS == NULL || j == 0)
             {
                 CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ZERO SET jobs in the obj-job . \n"));
                 CL_FUNC_EXIT();
                 return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
             }

             while(pAttrSetJobNodeS)
             { 
                  --j;
                 if((rc = VDECL_VER(clXdrMarshallClCorTxnAttrSetJobT, 4, 0, 0)((void *)&(pAttrSetJobNodeS->job), msgHdl, 0)) != CL_OK)
                  {
                     CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Failed to Marshal ClCorTxnAttrSetJobT rc:0x%x", rc));
                     return (rc);
                  }

                 /* Move Next */
                   pAttrSetJobNodeS = pAttrSetJobNodeS->next;
             }
             CL_ASSERT(j == 0);	
         }
         pObjJobNodeS = pObjJobNodeS->next;
    }
    
    CL_ASSERT(i == 0);	

    CL_FUNC_EXIT();
  return(rc);
}


/**
 *  Unpack a transaction stream.
 *
 *  This routine unpacks a transaction stream. The stream is unpacked
 *  "in place".
 *
 *  @param pTxnStream: stream to unpack.
 *
 *  @returns CL_OK  - Success<br 
 */

ClRcT 
clCorTxnJobStreamUnpack(
    CL_IN   ClBufferHandleT    inMsgHdl,
    CL_OUT  ClCorTxnJobHeaderNodeT  **pJobHeaderNode
    )
{
    ClRcT rc = CL_OK;
    ClCorTxnJobHeaderNodeT     *pJobHdrNode = NULL;
    ClCorTxnObjJobNodeT        *pObjJobNode = NULL;
    ClCorTxnAttrSetJobNodeT    *pAttrSetJobNode = NULL;
    ClUint32T i;
    ClUint32T j;

    CL_FUNC_ENTER();
 
    rc = clCorTxnJobHeaderNodeCreate(&pJobHdrNode);
    if(rc != CL_OK)
    {
         clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "Failed to create cor-txn job Header. rc [0x%x]", rc);
         CL_FUNC_EXIT();
         return (rc);
    }
 
    if((rc = VDECL_VER(clXdrUnmarshallClCorTxnJobHeaderT, 4, 0, 0)(inMsgHdl, (void *)&pJobHdrNode->jobHdr)) != CL_OK)
    {
         clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "Failed to unmarshall ClCorTxnJobHeaderT. rc:0x%x", rc);
         CL_FUNC_EXIT();
         return (rc);
    }

    /* Save the number of jobs */
    i = pJobHdrNode->jobHdr.numJobs;

    if(i == 0)
    {
        clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "Transaction-job contains 0 obj-jobs.");
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));   
    } 
 
    /* 
     * Resets the transaction data so that we can 
     * add packed jobs afresh 
     */
    pJobHdrNode->head = pJobHdrNode->tail = NULL;
    pJobHdrNode->jobHdr.numJobs = 0;

    while(i)
    {
        --i;
 
        pObjJobNode = (ClCorTxnObjJobNodeT *)clHeapAllocate(sizeof(ClCorTxnObjJobNodeT));
        if(!pObjJobNode)
        {
             clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                     CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
             clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", CL_COR_ERR_STR_MEM_ALLOC_FAIL);
             return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
        } 
 
        if((rc = VDECL_VER(clXdrUnmarshallClCorTxnObjJobT, 4, 0, 0)(inMsgHdl, (void *)&pObjJobNode->job)) != CL_OK)
        {
           clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "Failed to unmarshall ClCorTxnObjJobT. rc:0x%x", rc);
           CL_FUNC_EXIT();
           return (rc);
        }

       /* Add the job */
        CL_COR_TXN_OBJ_JOB_ADD(pJobHdrNode, pObjJobNode); 

        j = pObjJobNode->job.numJobs;

        /* Reset the pointers */
        pObjJobNode->head = pObjJobNode->tail = NULL;
        pObjJobNode->job.numJobs = 0;

        /* Unpack the SET jobs, if any */
        if(pObjJobNode->job.op == CL_COR_OP_SET || pObjJobNode->job.op == CL_COR_OP_GET)
        {

           if(j == 0)
           {
                clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "ZERO SET jobs in the obj-job.");
                CL_FUNC_EXIT();
                return (CL_COR_SET_RC(CL_COR_TXN_ERR_ZERO_JOBS));
           }

           while(j)
           {
             --j;
   
              pAttrSetJobNode = (ClCorTxnAttrSetJobNodeT *)clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
              if(!pAttrSetJobNode)
              {
                  clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                     CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                  clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", CL_COR_ERR_STR_MEM_ALLOC_FAIL);
                  CL_FUNC_EXIT();
                  return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
              } 

              if((rc = VDECL_VER(clXdrUnmarshallClCorTxnAttrSetJobT, 4, 0, 0)(inMsgHdl, (void *)&pAttrSetJobNode->job)) != CL_OK)
              {
                   clLog(CL_LOG_SEV_ERROR, "TXN", "JOB", "Failed to unmarshall ClCorTxnAttrSetJobT  rc:0x%x", rc);
                   CL_FUNC_EXIT();
                   return (rc);
              }

             /* Reset the prev/next pointers */
              pAttrSetJobNode->next = NULL;
              pAttrSetJobNode->prev = NULL;

             /* Add the job */
              CL_COR_TXN_ATTR_SET_JOB_ADD(pObjJobNode, pAttrSetJobNode);
 
           /* Update size in object Job */
              pObjJobNode->job.numJobs++;

           }
        }

      /* Update size in txn Job Header */
         pJobHdrNode->jobHdr.numJobs++;
    }
  
    /* update the pointer given by user */
    *pJobHeaderNode = pJobHdrNode;
    CL_FUNC_EXIT();
    return(rc);
}

/** 
  *  Function to generate transaction stream for the given object job in
  *  in the object job in the txn.
  *
  */
ClRcT  clCorTxnJobToJobExtract(
              CL_IN   ClCorTxnJobHeaderNodeT   *pJobHdrNode,
              CL_IN   ClCorTxnObjJobNodeT      *pObjJobNode,
              CL_OUT  ClCorTxnJobHeaderNodeT  **pRetNode)
{
   ClRcT rc = CL_OK;
   ClCorTxnJobHeaderNodeT *pNewJobHdrNode;
   ClCorTxnObjJobNodeT    *pNewObjJobNode;

      CL_FUNC_ENTER();
 
       /* New Job Header */
       pNewJobHdrNode = (ClCorTxnJobHeaderNodeT *)
                          clHeapAllocate(sizeof(ClCorTxnJobHeaderNodeT));
       if(!pNewJobHdrNode)
       {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                     CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
          CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
          CL_FUNC_EXIT();
          return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
       }
       memset((void *)pNewJobHdrNode, 0, sizeof(ClCorTxnJobHeaderNodeT));

       /* New Obj Job */
       pNewObjJobNode = (ClCorTxnObjJobNodeT *)
                           clHeapAllocate(sizeof(ClCorTxnObjJobNodeT));
       if(!pNewObjJobNode)
       {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                     CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
          CL_DEBUG_PRINT(CL_DEBUG_TRACE, (CL_COR_ERR_STR_MEM_ALLOC_FAIL));
          CL_FUNC_EXIT();
          return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
       }
       memset((void *)pNewObjJobNode, 0, sizeof(ClCorTxnObjJobNodeT));
 
      pNewJobHdrNode->jobHdr = pJobHdrNode->jobHdr;

      /* Make the object handle NULL */
      pNewJobHdrNode->jobHdr.oh = NULL;
      pNewJobHdrNode->jobHdr.ohSize = 0;

      pNewObjJobNode->job = pObjJobNode->job;

     /* Set the txn size and numJobs */
        pNewJobHdrNode->jobHdr.numJobs  = 1;
        pNewObjJobNode->job.numJobs = 0;

    /* Chain the jobs  */
       CL_COR_TXN_OBJ_JOB_ADD(pNewJobHdrNode, pNewObjJobNode);
 
   /* Are there any SET jobs */
      if(pObjJobNode->job.op == CL_COR_OP_SET)
      {
          ClUint32T  count = 0;
          ClCorTxnAttrSetJobNodeT *pSetJobNode;
          ClCorTxnAttrSetJobNodeT *pNewSetJobNode;

          pSetJobNode = pObjJobNode->head;
          count       = pObjJobNode->job.numJobs;

          while(pSetJobNode && count)
          {
              pNewSetJobNode = (ClCorTxnAttrSetJobNodeT *)
                           clHeapAllocate(sizeof(ClCorTxnAttrSetJobNodeT));
              if(!pNewSetJobNode)
              {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                                         CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                   CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
                   CL_FUNC_EXIT();
                   return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
              }
              memset((void *)pNewSetJobNode, 0, sizeof(ClCorTxnAttrSetJobNodeT));
              pNewSetJobNode->job = pSetJobNode->job;

              pNewSetJobNode->job.pValue = (ClUint8T *)
                           clHeapAllocate(pNewSetJobNode->job.size);
              if(!pNewSetJobNode)
              {
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, gCorClientLibName,
                                         CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                   CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( CL_COR_ERR_STR_MEM_ALLOC_FAIL));
                   CL_FUNC_EXIT();
                   return (CL_COR_SET_RC(CL_COR_ERR_NO_MEM));
              }

              memcpy((void *)pNewSetJobNode->job.pValue, 
                     (const void*)pSetJobNode->job.pValue, pSetJobNode->job.size); 

              /* Chain the jobs  */
                CL_COR_TXN_ATTR_SET_JOB_ADD(pNewObjJobNode, pNewSetJobNode);

             /* increment count */
               ++(pNewObjJobNode->job.numJobs);
               pSetJobNode = pSetJobNode->next;
               --count;
          }

      }
   /* update the return job. */
    *pRetNode = pNewJobHdrNode;

   CL_FUNC_EXIT();
  return (rc);
}
