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
 * File        : clTxnStreamIpi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains necessary implementation used by the txnClient
 * txnMgr modules for streaming (packing/unpacking) data for transaction
 * processing.
 *
 *
 *****************************************************************************/
#include <string.h>

#include <clDebugApi.h>

#include <clTxnCommonDefn.h>
#include <clTxnDb.h>

#include <xdrClTxnAppComponentT.h>
#include <xdrClTxnAppJobDefnT.h>
#include <xdrClTxnDefnT.h>
#include <xdrClTxnTransactionIdT.h>
#include <xdrClTxnTransactionJobIdT.h>
#include <xdrClTxnTransactionStateT.h>
#include <xdrClTxnAgentIdT.h>
#include <xdrClTxnAgentT.h>
#include "clTxnStreamIpi.h"

/* Callback function for container-walk */
ClRcT _clTxnStreamCfgJobInfoPack(
        CL_IN ClCntKeyHandleT   userKey, 
        CL_IN ClCntDataHandleT  userData, 
        CL_IN ClCntArgHandleT   userArg, 
        CL_IN ClUint32T         len);

ClRcT _clTxnStreamCfgCompInfoPack(
        CL_IN ClCntKeyHandleT   userKey, 
        CL_IN ClCntDataHandleT  dataKey, 
        CL_IN ClCntArgHandleT   userArg, 
        CL_IN ClUint32T         len);

ClRcT _clTxnStreamAgentInfoPack(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    dataKey, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           len);
/**
 * Packs job definition. This API is used by Transaction Server Library to 
 * pack data for sending it to associated transaction agents.
 */
ClRcT clTxnStreamTxnDataPack(
        CL_IN   ClTxnDefnT              *pTxnDefn,
        CL_IN   ClTxnAppJobDefnT        *pTxnJobDefn,
        CL_IN   ClBufferHandleT  msgHandle)
{
    ClRcT       rc          = CL_OK;

    CL_FUNC_ENTER();
    if ( (NULL == pTxnJobDefn) )
    {
        rc = CL_ERR_NULL_POINTER;
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = VDECL_VER(clXdrMarshallClTxnDefnT, 4, 0, 0)( (ClPtrT) pTxnDefn, msgHandle, 0);

    if (CL_OK == rc)
    { 
        rc = VDECL_VER(clXdrMarshallClTxnAppJobDefnT, 4, 0, 0)( (ClPtrT)pTxnJobDefn, msgHandle, 0);
    }

    if (CL_OK == rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Writing 0x%x of app job-defn\n", (int) pTxnJobDefn->appJobDefnSize));
        rc = clBufferNBytesWrite(msgHandle,
                                    (ClUint8T *) pTxnJobDefn->appJobDefn, 
                                    pTxnJobDefn->appJobDefnSize);
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to pack txn and job definition. rc:0x%x\n", rc));
    }

    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

/**
 * Unpacks transaction definition along with associated job details.
 * This API is used by transaction agents to retrieve transaction
 * details from the message receieved from client.
 */
ClRcT clTxnStreamTxnDataUnpack(
        CL_IN   ClBufferHandleT  msgHandle, 
        CL_OUT  ClTxnDefnT              **pNewTxnDefn,
        CL_OUT  ClTxnAppJobDefnT        **pNewTxnJobDefn)
{
    ClRcT               rc              = CL_OK;
    ClUint32T           offSet          = 0;
    ClTxnDefnT          *pTxnDefn       = NULL;
    ClTxnAppJobDefnT    *pTxnJobDefn    = NULL;


    CL_FUNC_ENTER();
    if ( (NULL == pNewTxnDefn) || (NULL == pNewTxnJobDefn) )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    rc = clTxnNewTxnDfnCreate(&pTxnDefn);
    if (CL_OK != rc)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Read txn-definition */
    rc = VDECL_VER(clXdrUnmarshallClTxnDefnT, 4, 0, 0)(msgHandle, pTxnDefn);
    if(CL_OK != rc)
    {
        clLogError("CMN", NULL,
                "Failed to unmarshall txn definition, rc=[0x%x]", rc);
        clTxnDefnDelete(pTxnDefn);
        return rc;
        
    }
    pTxnDefn->jobCount = 0;
    rc = clTxnNewAppJobCreate(&pTxnJobDefn);
    if (CL_OK != rc)
    {
        clLogError("CMN", NULL,
                "Failed to create application job, rc=[0x%x]", rc);
        clTxnDefnDelete(pTxnDefn);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Read txn-job definition */
    rc = VDECL_VER(clXdrUnmarshallClTxnAppJobDefnT, 4, 0, 0)(msgHandle, pTxnJobDefn);
    if(CL_OK != rc)
    {
        clLogError("CMN", NULL,
                "Failed to unmarshall application job defnition, rc=[0x%x]", rc);
        clTxnDefnDelete(pTxnDefn);
        clTxnAppJobDelete(pTxnJobDefn);
        return rc;
    }
    /* Read application job description */
    pTxnJobDefn->appJobDefn = (ClTxnJobDefnT) clHeapAllocate(pTxnJobDefn->appJobDefnSize);
    if(!pTxnJobDefn->appJobDefn)
    {
        clLogError("CMN", NULL,
                "Memory allocation failed");
        clTxnDefnDelete(pTxnDefn);
        clTxnAppJobDelete(pTxnJobDefn);
        rc = CL_ERR_NO_MEMORY;
        return rc;
    }
    offSet  = pTxnJobDefn->appJobDefnSize;
    clLogTrace("CMN", NULL, 
            "Retrieving app-job defn of size [0x%x]", offSet);
    rc = clBufferNBytesRead(msgHandle, (ClUint8T *) pTxnJobDefn->appJobDefn, &offSet);
    if((CL_OK != rc)||(offSet != pTxnJobDefn->appJobDefnSize))
    {
        clLogError("CMN", NULL,
                "Failed in reading application job defn, rc=[0x%x]", rc);
        clTxnDefnDelete(pTxnDefn);
        clTxnAppJobDelete(pTxnJobDefn);
        return rc;
    }

    *pNewTxnDefn = pTxnDefn;
    *pNewTxnJobDefn = pTxnJobDefn;

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This API packs configuration information required to run the transaction
 * by transaction management service. User specifies various configuration
 * details such as component-list etc via transaction-client API.
 * These details are packed and sent to transaction-service for completing
 * the intended work.
 */
ClRcT clTxnStreamTxnCfgInfoPack(
        CL_IN   ClTxnDefnT              *pTxnDefn, 
        CL_OUT  ClBufferHandleT  msgHandle)
{
    ClRcT                   rc = CL_OK;

    CL_FUNC_ENTER();
    if ( NULL == pTxnDefn )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    rc = VDECL_VER(clXdrMarshallClTxnDefnT, 4, 0, 0)( (ClPtrT) pTxnDefn, msgHandle, 0);
    if (CL_OK != rc)
        goto error;

    /* Walk along the component-list and pack information */
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Packing 0x%x jobs for txn-id 0x%x\n", pTxnDefn->jobCount, 
                                    pTxnDefn->clientTxnId.txnId));
    rc = clCntWalk(pTxnDefn->jobList, _clTxnStreamCfgJobInfoPack, 
                   (ClCntArgHandleT) msgHandle, sizeof(ClBufferHandleT));

error:
    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

/**
 * Internal function to pack mapping of job-id and component-address.
 */
ClRcT _clTxnStreamCfgJobInfoPack(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           len)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT  msgHandle;
    ClTxnAppJobDefnT        *pTxnAppJob;

    CL_FUNC_ENTER();

    msgHandle = (ClBufferHandleT ) userArg;
    pTxnAppJob = (ClTxnAppJobDefnT *) userData;

    if ( NULL == pTxnAppJob )
    {
        CL_FUNC_EXIT();
        return CL_ERR_NULL_POINTER;
    }

    rc = VDECL_VER(clXdrMarshallClTxnAppJobDefnT, 4, 0, 0)( (ClPtrT) pTxnAppJob, msgHandle, 0);

    if (CL_OK == rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Writing %d of app job-defn\n", pTxnAppJob->appJobDefnSize));
        rc = clBufferNBytesWrite(msgHandle,
                                    (ClUint8T *) pTxnAppJob->appJobDefn, 
                                    pTxnAppJob->appJobDefnSize);
    }

    if (CL_OK == rc)
    {
        /* Walk through the list of components */
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Packing 0x%x components of job 0x%x\n", 
                        pTxnAppJob->compCount, pTxnAppJob->jobId.jobId));
        rc = clCntWalk(pTxnAppJob->compList, _clTxnStreamCfgCompInfoPack, 
                       (ClCntArgHandleT) msgHandle, sizeof(ClBufferHandleT));
    }

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to pack job-defn for job 0x%x\n", pTxnAppJob->jobId.jobId));
        rc = CL_GET_ERROR_CODE(rc);
    }

    CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

ClRcT _clTxnStreamCfgCompInfoPack(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    dataKey, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           len)
{
    ClRcT rc = CL_OK;
    ClTxnAppComponentT      *pTxnComp;
    ClBufferHandleT  msgHandle;
    CL_FUNC_ENTER();

    pTxnComp = (ClTxnAppComponentT *) dataKey;
    msgHandle = (ClBufferHandleT ) userArg;

    if ( NULL == pTxnComp )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Adding component 0x%x:0x%x\n", 
                   pTxnComp->appCompAddress.nodeAddress, pTxnComp->appCompAddress.portId));
    rc = VDECL_VER(clXdrMarshallClTxnAppComponentT, 4, 0, 0)( (ClPtrT) pTxnComp, msgHandle, 0);

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * This API unpacks configuration information required to run the transaction.
 * Transaction management service uses this API to extract details from the
 * message sent from txn-client.
 */
ClRcT clTxnStreamTxnCfgInfoUnpack(
        CL_IN   ClBufferHandleT  msgHandle, 
        CL_OUT  ClTxnDefnPtrT           *pNewTxnDefn)
{
    ClRcT           rc          = CL_OK;
    ClTxnDefnT      *pTxnDefn   = NULL;
    ClUint32T       offSet = 0;
    ClUint32T       jobIncr, compIncr, compCount = 0, jobCount = 0, offset = 0;

    CL_FUNC_ENTER();

    if (NULL == pNewTxnDefn)
    {
        rc = CL_ERR_NULL_POINTER;
        CL_FUNC_EXIT();
        return (rc);
    }

    *pNewTxnDefn = 0;  /* Clear it out in case there is an error */
    
    rc = clTxnNewTxnDfnCreate(&pTxnDefn);
    if (CL_OK != rc)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = VDECL_VER(clXdrUnmarshallClTxnDefnT, 4, 0, 0)(msgHandle, (ClPtrT) pTxnDefn);
    if(CL_OK != rc)
    {
        clLogError("SER", NULL,
                "Failed to unmarshall txndefn, rc [0x%x]", rc);
        goto free_buff;
    }

    jobCount = pTxnDefn->jobCount;
    pTxnDefn->jobCount = 0;
    clLogDebug("SER", NULL, 
            "Extracting transaction [0x%x] job-count [0x%x]", 
            pTxnDefn->clientTxnId.txnId, jobCount);
    
    /* Iterator over all jobs and extract component details */
    for (jobIncr = 0; jobIncr < jobCount; jobIncr++)
    {
        ClTxnAppJobDefnT        *pNewJobDefn = NULL;

        rc = clTxnNewAppJobCreate(&pNewJobDefn);
        if(CL_OK != rc)
        {
            clLogError("SER", NULL,
                    "Failed while creating job defn, rc [0x%x]", rc);
            goto free_buff;
        }
        rc = VDECL_VER(clXdrUnmarshallClTxnAppJobDefnT, 4, 0, 0)(msgHandle, (ClPtrT) pNewJobDefn);
        if(CL_OK != rc)
        {
            clLogError("SER", NULL,
                    "Failed to unmarshall job defn, rc [0x%x]", rc);
            goto free_buff;
        }

        pNewJobDefn->appJobDefn = (ClTxnJobDefnT) clHeapAllocate(pNewJobDefn->appJobDefnSize);
        if(!pNewJobDefn->appJobDefn)
        {
            clLogCritical("SER", "CTM",
                    "Failed to allocate memory of size [%d] for application job",
                    pNewJobDefn->appJobDefnSize);
            goto free_buff;
        }
        offSet  = pNewJobDefn->appJobDefnSize;
        clLogDebug("SER", NULL,
                "Retrieving app-job defn of size [0x%x]", offSet);
        rc = clBufferNBytesRead(msgHandle, (ClUint8T *) pNewJobDefn->appJobDefn, &offSet);
        if(CL_OK != rc)
        {
            clLogError("SER", "CTM",
                    "Failed read [%d] bytes of application payload, rc [0x%x]", 
                    offset, rc);
            goto free_buff;
        }
        CL_ASSERT(offSet == pNewJobDefn->appJobDefnSize);            

        compCount = pNewJobDefn->compCount;
        pNewJobDefn->compCount = 0;

        for (compIncr = 0; compIncr < compCount ; compIncr++)
        {
            ClTxnAppComponentT compInfo;

            memset(&compInfo, 0, sizeof(ClTxnAppComponentT));
            rc = VDECL_VER(clXdrUnmarshallClTxnAppComponentT, 4, 0, 0)(msgHandle, (ClPtrT) &compInfo);
            if (CL_OK != rc)
            {
                clLogError("SER", NULL,
                        "Failed to unmarshall compInfo, rc [0x%x]", rc);
                goto free_buff;
            }
            /* Add new component details to job-component-list */
            rc = clTxnAppJobComponentAdd(pNewJobDefn, compInfo.appCompAddress, 
                                         compInfo.configMask);
            if(CL_OK != rc)
            {
                clLogError("SER", NULL,
                        "Failed to add compInfo into complist, rc [0x%x]", rc);
                goto free_buff;
            }
        }

        rc = clTxnNewAppJobAdd(pTxnDefn, pNewJobDefn);
        if(CL_OK != rc)
        {
            clLogError("SER", NULL,
                    "Failed to add job defn into txn defn, rc [0x%x]", rc);
            goto free_buff;
        }
    }

free_buff:
    if (CL_OK != rc)
    {
        /* Do clean-up of all memory allocations done so far */
        clTxnDefnDelete(pTxnDefn);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
            ("Failed to extract configuration information rc:0x%x\n", rc));
    }
    else
    {
        *pNewTxnDefn = pTxnDefn;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Packs job definition. This API is used by Transaction Server Library to 
 * pack data for sending it to associated transaction agents.
 */
ClRcT clTxnStreamTxnAgentDataPack(
        CL_IN   ClUint32T               *pFailedAgentCount,
        CL_IN   ClCntHandleT            failedAgentList,
        CL_IN   ClBufferHandleT  msgHandle)
{
    ClRcT       rc          = CL_OK;
    rc = clXdrMarshallClUint32T(pFailedAgentCount,msgHandle,0);
    if (CL_OK != rc)
    {
        return rc;
    }
    
    rc = clCntWalk(failedAgentList, _clTxnStreamAgentInfoPack, 
                   (ClCntArgHandleT) msgHandle, sizeof(ClBufferHandleT));



    CL_FUNC_ENTER();

     CL_FUNC_EXIT();
    return (CL_GET_ERROR_CODE(rc));
}

ClRcT _clTxnStreamAgentInfoPack(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    dataKey, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           len)
{
    ClRcT rc = CL_OK;
    ClTxnAgentT             *pAgentInfo;
    ClBufferHandleT  msgHandle;
    CL_FUNC_ENTER();

    pAgentInfo = (ClTxnAgentT *) dataKey;
    msgHandle = (ClBufferHandleT ) userArg;

    if ( NULL == pAgentInfo )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }
    rc = VDECL_VER(clXdrMarshallClTxnAgentT, 4, 0, 0)((ClPtrT)pAgentInfo, msgHandle, 0);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error in packing response structure, rc=[0x%x]\n", rc));
    }
    /*

    rc = clBufferNBytesWrite(msgHandle,
                                    (ClUint8T *) pAgentInfo->agentJobDefn, 
                                    pAgentInfo->agentJobDefnSize);
                                    */

    CL_FUNC_EXIT();
    return (rc);
}


/**
 * Unpacks transaction definition along with associated job details.
 * This API is used by transaction agents to retrieve transaction
 * details from the message receieved from client.
 */
ClRcT clTxnStreamTxnAgentDataUnpack(
        CL_OUT   ClBufferHandleT  msgHandle,
        CL_OUT   ClCntHandleT            *pFailedAgentList,
        CL_OUT   ClUint32T               *pFailedAgentCount)
{
    ClRcT               rc              = CL_OK;
    ClUint32T           count = 0;


    CL_FUNC_ENTER();
    if ( (NULL == pFailedAgentCount) || (NULL == pFailedAgentList) )
    {
        CL_FUNC_EXIT();
        return (CL_ERR_NULL_POINTER);
    }

    rc = clXdrUnmarshallClUint32T(msgHandle,(ClPtrT)pFailedAgentCount);
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,( "Agent Count : %d\n", *pFailedAgentCount));
    for(count = 0;count < *(pFailedAgentCount); ++count )
    {
        ClTxnAgentT     *pFailedAgentInfo = NULL;

        pFailedAgentInfo = (ClTxnAgentT *)clHeapAllocate(sizeof(ClTxnAgentT));
        CL_ASSERT(NULL != pFailedAgentInfo);
        rc = VDECL_VER(clXdrUnmarshallClTxnAgentT, 4, 0, 0)(msgHandle,(ClPtrT)pFailedAgentInfo);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Failed while unmarshalling the failed agent info"
                    ": %x\n", rc));
            return rc;
        }
#ifdef CL_TXN_DEBUG
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("TC:Agent Job Defn : %p, Size : %d\n",
                pFailedAgentInfo->agentJobDefn,
                pFailedAgentInfo->agentJobDefnSize));
#endif
        
        /*
        pFailedAgentInfo->agentJobDefn =
                                    (ClTxnJobDefnT) clHeapAllocate(pFailedAgentInfo->agentJobDefnSize);
        CL_ASSERT(0 != pFailedAgentInfo->agentJobDefn);
        
        rc = clBufferNBytesRead(msgHandle,(ClUint8T *) pFailedAgentInfo->agentJobDefn,
                                        &(pFailedAgentInfo->agentJobDefnSize));
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Failed while unmarshalling the job defn"
                    ": %x\n", rc));
            return rc;
        }
        */

        rc = clCntNodeAdd(*pFailedAgentList,
                                        (ClCntKeyHandleT) &(pFailedAgentInfo->failedAgentId),
                                        (ClCntDataHandleT) pFailedAgentInfo, 0);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Failed while adding the node into the container "
                    ": %x\n", rc));
            return rc;
        }
                                                            
        
    }
    
    CL_FUNC_EXIT();
    return (rc);
}


