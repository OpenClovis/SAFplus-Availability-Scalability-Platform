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
 * ModuleName  : prov
 * File        : clProvMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the Provision Library implementation.
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>  /* strcpy is needed for eo name */

/* ASP Includes */
#include <clCommon.h>
#include <clVersionApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clOmApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorMetaData.h>
#include <clCorTxnApi.h>
#include <clDebugApi.h>
#include <clOampRtApi.h>
#include <clMsoConfig.h>
#include <clProvErrors.h>
#include <clProvOmApi.h>
#include <clProvMainIpi.h>
#include <clProvLogMsgIpi.h>
#include <clCorErrors.h>


#ifdef MORE_CODE_COVERAGE
#include "clCodeCoveApi.h"
#endif

/**
 *          G L O B A L       V A R I A L E S
 */

static ClVersionT versionsSupported[] = {
    {'B', 0x01, 0x01}
};

static ClVersionDatabaseT versionDatabase = {
    sizeof(versionsSupported) / sizeof(ClVersionT),
    versionsSupported
};

/* Global definition of OM Class table. */
extern ClOmClassControlBlockT pAppOmClassTbl[];
/* Count of the no. of OM class structures. */
extern ClUint32T appOmClassCnt;

/**
 * Application callback functions for Transaction.
 */
extern ClProvTxnCallbacksT clProvTxnCallbacks;

extern ClMsoConfigCallbacksT gClProvMsoCallbacks;

/**
 * Provision Library Initalize falg
 */
ClUint32T       gProvInit   = CL_FALSE;

ClCntHandleT    gClProvOmHandleInfo;

ClOsalMutexIdT  gClProvMutex;

ClCorMOIdListT* pProvMoIdList = NULL;

ClInt32T
clProvKeyCompare( ClCntKeyHandleT key1,
                  ClCntKeyHandleT key2 )
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

/**
 *  This function will deallocate the memroy which is allocated to store
 *  OM handle
 */
void
clProvUserDelete( ClCntKeyHandleT userKey,
                  ClCntDataHandleT userData )
{
    ClRcT           rc      = CL_OK;
    ClHandleT       handle  = (ClHandleT)(ClWordT)userKey;
    ClCorMOIdPtrT   phMoId;

    rc = clOmOmHandleToMoIdGet( handle, &phMoId );
    CL_PROV_CHECK_RC0_LOG1( rc, CL_OK, CL_LOG_ERROR, CL_PROV_LOG_1_MOID_GET, rc );

    rc = clOmObjectDelete( handle, 1, NULL, 0 );
    if (rc != CL_OK)
    {
        clLogError("PRV", "OMD", "Failed to remove the OM object. rc [0x%x]", rc);
    }

    /* Remove the MoId to OmId mapping */
    rc = clOmMoIdToOmIdMapRemove(phMoId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "OMD", "Failed to remove the MoId to Om Id mapping. rc [0x%x]", rc);
    }

    return;
}

/**
 *  This function will deallocate the memroy which is allocated to store
 *  OM handle. As well it deregister from COR. This will happen only when 
 *  clProvFinalize function is getting called. 
 */
void
clProvUserDestroy( ClCntKeyHandleT userKey,
                   ClCntDataHandleT userData )
{
    ClRcT           rc      = CL_OK;
    ClHandleT       handle  = (ClHandleT)(ClWordT)userKey;
    ClCorMOIdPtrT   phMoId;

    rc = clOmOmHandleToMoIdGet( handle, &phMoId );
    CL_PROV_CHECK_RC0_LOG1( rc, CL_OK, CL_LOG_ERROR, CL_PROV_LOG_1_MOID_GET, rc );

    rc = clOmObjectDelete( handle, 1, NULL, 0 );
    if (rc != CL_OK)
    {
        clLogError("PRV", "FIN", "Failed to delete the OM object. rc [0x%x]", rc);
    }

    /* Remove the MoId to OmId mapping */
    rc = clOmMoIdToOmIdMapRemove(phMoId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "FIN", "Failed to remove the MoId to Om Id mapping. rc [0x%x]", rc);
    }

    return;
}

/**
 *  1. If the service ID is different then return with out doin any thing.
 *     This will happen when the transation happens for Alarm.
 *  2. Find out the operation. (Create, Delete, Set, RollBack)
 *  3. If it is Create Operation.
 *     3.1 Needs to do the bellow steps only @ for UPDATE txn state. This function will be called
 *         for update, validate, postValidate states.
 *     3.2 Create the OM object for corresponding MOID add the object handle to ProvOM Db. This needs to be deleted
 *         MOID is deleted or component is down.
 *     3.3 Map the MOID and the generated OM handle. Further in the flow, OM handle can be found using MOID.
 *  4. If it is Delete Operation.
 *     4.1 Get the OM handle form MOID.
 *     4.2 Delete OM handle from ProvOM db and delete the OM handle.
 *  5. If it is Set Operation.
 *     5.1 Get the OM object from the MOID to OM map table.
 *     5.2 Depend on the txn state call the appropriate function.
 *  6.If it is RollBack Function needs to call the Rollback Call back function.
 */

ClRcT clProvTxnListJobWalk( ClCorTxnIdT     corTxnId,
                            ClCorTxnJobIdT  jobId,
                            void           *arg )
{
    ClRcT               rc          = CL_OK;
    ClCorMOIdT          moId;
    ClCorMOIdPtrT       pMoId = NULL;
    ClCorOpsT           corOp       = CL_COR_OP_RESERVED;
    ClPtrT               pOmObj      = NULL;
    ClCorAttrTypeT      attrType    = 0; 
    ClCorTypeT          attrDataType= 0;
    ClCorAttrIdT        attrId      = 0;    
    void*               pValue      = NULL;
    ClUint32T           valueSize   = 0;
    ClInt32T            index       = 0;
    ClCorMOServiceIdT   provSerId   = 0;
    ClCorAttrPathPtrT   attrPath    = NULL;
    ClProvTxnDataT      *pProvTxnData = NULL;
    ClProvTxnListJobWalkDataPtrT pProvTxnListWalkData = (ClProvTxnListJobWalkDataPtrT)arg;   

    CL_FUNC_ENTER();

    if(!pProvTxnListWalkData)
        return CL_PROV_RC(CL_ERR_INVALID_PARAMETER);

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the moid during job walk. rc[0x%x]", rc));
        return rc;
    }

    provSerId = clCorMoIdServiceGet(&moId);

    /** 
     * If it is for other then provisioning then return OK
     */
    if ( CL_COR_SVC_ID_PROVISIONING_MANAGEMENT != provSerId )
    {
        return CL_OK;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &corOp);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the Op type. rc[0x%x]", rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("INSIDE PROV JOB WALK , Op is [%x], txnState [0x%x]", corOp, 
                                   pProvTxnListWalkData->txnJobData.op));

    if(!(pProvTxnListWalkData->txnDataEntries & 3))
    {
        pProvTxnListWalkData->pTxnDataList = clHeapRealloc(pProvTxnListWalkData->pTxnDataList,
                                                           sizeof(*pProvTxnListWalkData->pTxnDataList) *
                                                           (pProvTxnListWalkData->txnDataEntries + 4));
        CL_ASSERT(pProvTxnListWalkData->pTxnDataList != NULL);
        memset(pProvTxnListWalkData->pTxnDataList + pProvTxnListWalkData->txnDataEntries,
               0,
               sizeof(*pProvTxnListWalkData->pTxnDataList) * 4);
    }
    pProvTxnData = pProvTxnListWalkData->pTxnDataList + pProvTxnListWalkData->txnDataEntries;
    pProvTxnData->jobId = jobId;

    switch ( ( ClUint32T )corOp )
    {
    case CL_COR_OP_CREATE_AND_SET:
    case CL_COR_OP_CREATE:
        rc = clCorMoIdAlloc(&pMoId);
        CL_ASSERT( rc == CL_OK);
        memcpy(pMoId, &moId, sizeof(*pMoId));

        rc = clOmMoIdToOmObjectReferenceGet( pMoId, &pOmObj );
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
            pMoId = NULL;
        }
        CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

        pProvTxnData->provCmd = corOp;
        pProvTxnData->pMoId = pMoId;
        pProvTxnData->attrId = CL_COR_INVALID_ATTR_ID;
        break;
        
    case CL_COR_OP_DELETE:
        rc = clCorMoIdAlloc(&pMoId);
        CL_ASSERT(rc == CL_OK);
        memcpy(pMoId, &moId, sizeof(*pMoId));

        rc = clOmMoIdToOmObjectReferenceGet(pMoId, &pOmObj );
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

        pProvTxnData->provCmd = corOp;
        pProvTxnData->pMoId   = pMoId;
        pProvTxnData->attrId  = CL_COR_INVALID_ATTR_ID;
        break;        

    case CL_COR_OP_SET:

        rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &attrPath);
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        /* This will convert the attribute value to host format.
         * Need to change the value to network format after validation.
         */
        rc = clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &valueSize);

        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        rc = clCorMoIdAlloc(&pMoId);
        CL_ASSERT(rc == CL_OK);
        memcpy(pMoId, &moId, sizeof(*pMoId));

        rc = clOmMoIdToOmObjectReferenceGet( pMoId, &pOmObj );
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        rc = clCorTxnJobAttributeTypeGet(corTxnId, jobId, &attrType, &attrDataType);
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );
        
        pProvTxnData->provCmd    = CL_COR_OP_SET;
        pProvTxnData->pMoId      = pMoId;
        pProvTxnData->attrPath   = attrPath;
        pProvTxnData->attrType   = attrType;
        pProvTxnData->attrDataType = attrDataType;
        pProvTxnData->attrId     = attrId;
        pProvTxnData->pProvData  = pValue;
        pProvTxnData->size       = valueSize;
        pProvTxnData->index      = index;
        break;
        
    case CL_COR_OP_GET:
        rc = clCorMoIdAlloc(&pMoId);
        CL_ASSERT(rc == CL_OK);
        memcpy(pMoId, &moId, sizeof(*pMoId));

        rc = clOmMoIdToOmObjectReferenceGet( &moId, &pOmObj );
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

        rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &attrPath);
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_MOID_OM_GET, rc );

        rc = clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &valueSize);
        if(rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_ATTR_INFO_GET, rc );

        /* Update the type of the attribute. */
        rc = clCorTxnJobAttributeTypeGet(corTxnId, jobId, &attrType, &attrDataType);
        if (rc != CL_OK)
        {
            clCorMoIdFree(pMoId);
        }
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_ATTR_INFO_GET, rc );

        pProvTxnData->provCmd    = CL_COR_OP_GET;
        pProvTxnData->pMoId      = pMoId;
        pProvTxnData->attrPath   = attrPath;
        pProvTxnData->attrType   = attrType;
        pProvTxnData->attrDataType = attrDataType;
        pProvTxnData->attrId     = attrId;
        pProvTxnData->size       = valueSize;
        pProvTxnData->index      = index;
        /* Allocate the memory to be assigned in the read callback function */
        pProvTxnData->pProvData = clHeapCalloc(1, valueSize);
        CL_ASSERT(pProvTxnData->pProvData != NULL);
        break;

    default:
        return CL_OK;
    }
    
    ++pProvTxnListWalkData->txnDataEntries;
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT   clProvTxnJobWalk( ClCorTxnIdT     corTxnId,
                          ClCorTxnJobIdT  jobId,
                          void           *arg )
{
    ClRcT               rc          = CL_OK;
    ClCorMOIdT          moId;
    ClCorOpsT           corOp       = CL_COR_OP_RESERVED;
    ClPtrT               pOmObj      = NULL;
    ClCorAttrTypeT      attrType    = 0; 
    ClCorTypeT          attrDataType= 0;
    ClCorAttrIdT        attrId      = 0;    
    void*               pValue      = NULL;
    ClUint32T           valueSize   = 0;
    ClInt32T            index       = 0;
    ClCorMOServiceIdT   provSerId   = 0;
    ClCorAttrPathPtrT   attrPath    = NULL;
    ClProvTxnDataT      provTxnData = {0};
    ClUint32T           jobStatus   = 0; /* To know the status of the job in the rollback state */
    ClProvTxnJobWalkDataPtrT pProvTxnWalkData = (ClProvTxnJobWalkDataPtrT)arg;   

    CL_FUNC_ENTER();


    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the moid during job walk. rc[0x%x]", rc));
        return rc;
    }

    provSerId = clCorMoIdServiceGet(&moId);

    /** 
     * If it is for other then provisioning then return OK
     */
    if ( CL_COR_SVC_ID_PROVISIONING_MANAGEMENT != provSerId )
    {
        return CL_OK;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &corOp);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the Op type. rc[0x%x]", rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("INSIDE PROV JOB WALK , Op is [%x], txnState [0x%x]", corOp, pProvTxnWalkData->op));
    
    switch ( ( ClUint32T )corOp )
    {
      case CL_COR_OP_CREATE_AND_SET:
      case CL_COR_OP_CREATE:
        
        rc = clOmMoIdToOmObjectReferenceGet( &moId, &pOmObj );

        CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

        if (CL_PROV_VALIDATE_STATE == pProvTxnWalkData->op)  
        {
            provTxnData.provCmd = corOp;
            provTxnData.pMoId = &moId;
            provTxnData.attrId = CL_COR_INVALID_ATTR_ID;

            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvValidate(pOmObj, pProvTxnWalkData->handle, &provTxnData);
            CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                    CL_PROV_LOG_1_CREATE_VALIDATE, rc );
        }
        else if (CL_PROV_ROLLBACK_STATE == pProvTxnWalkData->op )
        {
            provTxnData.provCmd = corOp;
            provTxnData.pMoId = &moId;
            provTxnData.attrId = CL_COR_INVALID_ATTR_ID;

            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvRollback(pOmObj, pProvTxnWalkData->handle, &provTxnData );

            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                    CL_PROV_LOG_1_CREATE_ROLLBACK, rc );

            if (CL_OK == clCorTxnJobStatusGet(corTxnId, jobId, &jobStatus))
            {
                if (jobStatus != CL_OK)
                {
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE)); 
                }
            }    
        }
        else if ( CL_PROV_UPDATE_STATE == pProvTxnWalkData->op )
        {
            provTxnData.provCmd    = corOp;
            provTxnData.pMoId      = &moId;
            provTxnData.attrId     = CL_COR_INVALID_ATTR_ID;

            rc = ( ( CL_OM_PROV_CLASS * ) pOmObj )->clProvUpdate( pOmObj, pProvTxnWalkData->handle, &provTxnData );
            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                    CL_PROV_LOG_1_CREATE_UPDATE, rc );
        }
        break;
        
      case CL_COR_OP_DELETE:
        rc = clOmMoIdToOmObjectReferenceGet( &moId, &pOmObj );

        CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
            CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

        if ( CL_PROV_VALIDATE_STATE == pProvTxnWalkData->op )
        {
            provTxnData.provCmd = corOp;
            provTxnData.pMoId   = &moId;
            provTxnData.attrId  = CL_COR_INVALID_ATTR_ID;

            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvValidate(pOmObj, pProvTxnWalkData->handle, &provTxnData );

            CL_PROV_CHECK_RC_UPDATE( rc, CL_OK, corTxnId, jobId);
            CL_PROV_CHECK_RC1_LOG1(rc, CL_OK, rc, CL_LOG_ERROR,
                    CL_PROV_LOG_1_DELETE_VALIDATE, rc);
        }
        else if ( CL_PROV_ROLLBACK_STATE == pProvTxnWalkData->op)
        {
            provTxnData.provCmd = corOp;
            provTxnData.pMoId   = &moId;
            provTxnData.attrId  = CL_COR_INVALID_ATTR_ID;

            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvRollback(pOmObj, pProvTxnWalkData->handle, &provTxnData );

            CL_PROV_CHECK_RC1_LOG1(rc, CL_OK, rc, CL_LOG_ERROR,
                    CL_PROV_LOG_1_DELETE_ROLLBACK, rc);
        }
        else if ( CL_PROV_UPDATE_STATE == pProvTxnWalkData->op ) 
        {
            provTxnData.provCmd = corOp;
            provTxnData.pMoId   = &moId;
            provTxnData.attrId  = CL_COR_INVALID_ATTR_ID;

            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvUpdate(pOmObj, pProvTxnWalkData->handle, &provTxnData );

            CL_PROV_CHECK_RC1_LOG1(rc, CL_OK, rc, CL_LOG_ERROR,
                    CL_PROV_LOG_1_DELETE_UPDATE, rc);
        }
        break;        

      case CL_COR_OP_SET:

        rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &attrPath);
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        /* This will convert the attribute value to host format.
         * Need to change the value to network format after validation.
         */
        rc = clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &valueSize);

        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        rc = clOmMoIdToOmObjectReferenceGet( &moId, &pOmObj );
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );

        rc = clCorTxnJobAttributeTypeGet(corTxnId, jobId, &attrType, &attrDataType);
        CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);
        CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_PREVALIDATE, rc );
        
        provTxnData.provCmd    = CL_COR_OP_SET;
        provTxnData.pMoId      = &moId;
        provTxnData.attrPath   = attrPath;
        provTxnData.attrType   = attrType;
        provTxnData.attrDataType = attrDataType;
        provTxnData.attrId     = attrId;
        provTxnData.pProvData  = pValue;
        provTxnData.size       = valueSize;
        provTxnData.index      = index;

        switch ( pProvTxnWalkData->op )
        {
          case CL_PROV_VALIDATE_STATE:
            rc = ( ( CL_OM_PROV_CLASS * )pOmObj )->clProvValidate( pOmObj, pProvTxnWalkData->handle, &provTxnData );
            
            /* Update the attr value in network format */
            clCorBundleAttrValueSet(corTxnId, jobId, provTxnData.pProvData);
            
            CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                    CL_PROV_LOG_1_SET_VALIDATE, rc );
            break;

          case CL_PROV_UPDATE_STATE:
            rc = ( ( CL_OM_PROV_CLASS * )pOmObj )->clProvUpdate( pOmObj, pProvTxnWalkData->handle, &provTxnData );

            /* Update the attr value in network format */
            clCorBundleAttrValueSet(corTxnId, jobId, provTxnData.pProvData);
            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                    CL_PROV_LOG_1_SET_UPDATE, rc );
            break;             

          case CL_PROV_ROLLBACK_STATE:
               
            rc = ( ( CL_OM_PROV_CLASS * )pOmObj )->clProvRollback( pOmObj, pProvTxnWalkData->handle, &provTxnData );
            
            /* Update the attr value in network format */
            clCorBundleAttrValueSet(corTxnId, jobId, provTxnData.pProvData);

            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR,
                                CL_PROV_LOG_1_SET_ROLLBACK, rc );

            if (CL_OK == clCorTxnJobStatusGet(corTxnId, jobId, &jobStatus))
            {
                if (jobStatus != CL_OK)
                {
                    CL_FUNC_EXIT();
                    return (CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE)); 
                }
            }

            break;

        default:
            break;
        }
        break;
        
        case CL_COR_OP_GET:
        {
            rc = clOmMoIdToOmObjectReferenceGet( &moId, &pOmObj );
            CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                    CL_PROV_LOG_1_OM_OBJ_REF_GET, rc );

            rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &attrPath);
            CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                    CL_PROV_LOG_1_MOID_OM_GET, rc );

            rc = clCorTxnJobSetParamsGet(corTxnId, jobId, &attrId, &index, &pValue, &valueSize);
            CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);            
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                    CL_PROV_LOG_1_ATTR_INFO_GET, rc );

            rc = clCorTxnJobAttributeTypeGet(corTxnId, jobId, &attrType, &attrDataType);
            CL_PROV_CHECK_RC_UPDATE(rc, CL_OK, corTxnId, jobId);
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                                    CL_PROV_LOG_1_ATTR_INFO_GET, rc );

            provTxnData.provCmd    = CL_COR_OP_GET;
            provTxnData.pMoId      = &moId;
            provTxnData.attrPath   = attrPath;
            provTxnData.attrType   = attrType;
            provTxnData.attrDataType = attrDataType;
            provTxnData.attrId     = attrId;
            provTxnData.size       = valueSize;
            provTxnData.index      = index;
            
            /* Allocate the memory to be assigned in the read callback function */
            provTxnData.pProvData = clHeapAllocate(valueSize);
            if(NULL == provTxnData.pProvData)
            {
                clLogError("PRV", "JWK", "Failed while allocating the memory for \
                        attribute value with attrId [0x%x].", attrId ); 
                rc = CL_PROV_RC(CL_ERR_NO_MEMORY);
                clCorTxnJobStatusSet(corTxnId, jobId, rc);
                return CL_OK;
            }

            memset(provTxnData.pProvData, 0, valueSize);

            if(( ( CL_OM_PROV_CLASS * )pOmObj )->clProvRead != NULL)
                rc = ( ( CL_OM_PROV_CLASS * )pOmObj )->clProvRead( pOmObj, pProvTxnWalkData->handle, &provTxnData );
            else
            {
                clLogError("PRV", "JWK", "The read callback for the MO is not defined. rc[0x%x]", rc);
                rc = CL_PROV_RC(CL_ERR_NULL_POINTER);
            }

            if(rc != CL_OK)
                rc = CL_COR_SET_RC(CL_COR_ERR_GET_DATA_NOT_FOUND);

            /* Update the status for this job. */
            clCorTxnJobStatusSet(corTxnId, jobId, rc);
            
            if(rc == CL_OK)
            {
                rc = clCorBundleAttrValueSet(corTxnId, jobId, provTxnData.pProvData);
                if(CL_OK != rc)
                {
                    clLogError("PRV", "GET", "Failed while setting the value of the attribute [0x%x]. rc[0x%x]", attrId, rc);
                } 
            }

            /* Free the memory allocated earlier. */
            clHeapFree(provTxnData.pProvData);
            rc = CL_OK;
        }
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

static ClRcT clProvTxnObjectInvoke( ClCorTxnIdT txnId, ClPtrT pOmObj, ClProvTxnListJobWalkDataPtrT pTxnData)
{
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;
    ClProvTxnStateT txnState = pTxnData->txnJobData.op;
    ClInt32T i;

    CL_FUNC_ENTER();

    if(!pTxnData->txnDataEntries || !pTxnData->pTxnDataList)
        return rc;

    switch(txnState)
    {

    case CL_PROV_READ_STATE:
        if(( ( CL_OM_PROV_CLASS * )pOmObj )->clProvObjectRead)
            rc = ( ( CL_OM_PROV_CLASS * )pOmObj )->clProvObjectRead( pOmObj, 
                                                                     pTxnData->txnJobData.handle, 
                                                                     pTxnData->pTxnDataList,
                                                                     pTxnData->txnDataEntries);
        break;
    case CL_PROV_VALIDATE_STATE:
        if(( ( CL_OM_PROV_CLASS * )pOmObj )->clProvObjectValidate)
        {
            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvObjectValidate(pOmObj, 
                                                                     pTxnData->txnJobData.handle, 
                                                                     pTxnData->pTxnDataList,
                                                                     pTxnData->txnDataEntries);
        }
        break;
        
    case CL_PROV_UPDATE_STATE:
        if(( ( CL_OM_PROV_CLASS * )pOmObj )->clProvObjectUpdate)
        {
            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvObjectUpdate(pOmObj, 
                                                                   pTxnData->txnJobData.handle, 
                                                                   pTxnData->pTxnDataList,
                                                                   pTxnData->txnDataEntries);
        }
        break;

    case CL_PROV_ROLLBACK_STATE:
        if(( ( CL_OM_PROV_CLASS * )pOmObj )->clProvObjectRollback)
        {
            rc = ((CL_OM_PROV_CLASS *) pOmObj)->clProvObjectRollback(pOmObj, 
                                                                     pTxnData->txnJobData.handle, 
                                                                     pTxnData->pTxnDataList,
                                                                     pTxnData->txnDataEntries);
        }
        break;

    default:
        break;
    }

    /*
     * Prologue/cleanup after the callback.
     */
    for(i = 0; i < pTxnData->txnDataEntries; ++i)
    {
        ClProvTxnDataT *pProvTxnData = pTxnData->pTxnDataList + i;
        ClCorOpsT corOp = pProvTxnData->provCmd;
        
        switch ( ( ClUint32T )corOp )
        {
        case CL_COR_OP_SET:
            
            /* Update the attr value in network format */
            clCorBundleAttrValueSet(txnId, pProvTxnData->jobId, pProvTxnData->pProvData);
            break;

        case CL_COR_OP_GET:
            /* Update the status for this job. */
            clCorTxnJobStatusSet(txnId, pProvTxnData->jobId, rc);
            if(rc == CL_OK)
            {
                retCode = clCorBundleAttrValueSet(txnId, pProvTxnData->jobId, pProvTxnData->pProvData);
                if(CL_OK != retCode)
                {
                    clLogError("PRV", "GET", 
                               "Failed while setting the value of the attribute [0x%x]. rc[0x%x]", 
                               pProvTxnData->attrId, retCode);
                } 
            }
            clHeapFree(pProvTxnData->pProvData);
            break;
        default:
            break;
        }
        clCorMoIdFree(pProvTxnData->pMoId);
    }

    clHeapFree(pTxnData->pTxnDataList);
    pTxnData->pTxnDataList = NULL;
    pTxnData->txnDataEntries = 0;
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clProvTxnStart(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_INOUT    ClTxnAgentCookieT*      pCookie)
{
    CL_FUNC_ENTER();

    if (clProvTxnCallbacks.fpProvTxnStart != NULL)
    {
        /* Call the user supplied callback function */
        (void) clProvTxnCallbacks.fpProvTxnStart(txnHandle);
    }
    else
    {
        clLogInfo("PRV", "TXN", "User has not specified the Txn-Start callback function.");
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clProvTxnEnd(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_INOUT    ClTxnAgentCookieT*      pCookie)
{
    CL_FUNC_ENTER();

    if (clProvTxnCallbacks.fpProvTxnEnd != NULL)
    {
        (void) clProvTxnCallbacks.fpProvTxnEnd(txnHandle);
    }
    else
    {
        clLogInfo("PRV", "TXN", "User has not specified the Transaction End callback function.");
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT
clProvTxnValidate( CL_IN       ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId) 
{
    ClRcT                   rc          = CL_OK;
    ClCorTxnJobIdT          jobId       = 0;
    ClCorOpsT               op          = 0;
    ClProvTxnListJobWalkDataT txnData = {{0}};
    ClCorMOIdT              moId        = {{{0}}};
    ClPtrT                  pOmObj      = NULL;
    ClRcT                   retCode     = CL_OK;

    CL_FUNC_ENTER();

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "VAL", "Failed to get the MoId from Transaction Job. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorTxnFirstJobGet(corTxnId, &jobId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the first job. rc [0x%x]", rc));
        return rc;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &op);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the operation type. rc [0x%x]", rc));
        return rc;
    }
    
    if ((op == CL_COR_OP_CREATE_AND_SET) || (op == CL_COR_OP_CREATE))
    {   
        rc = clProvOmObjectCreate(corTxnId, &pOmObj);
        if (rc != CL_OK)
        {
            createError:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create the om object. rc [0x%x]", rc));
            clCorTxnJobStatusSet(corTxnId, jobId, rc);
            
            if (clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
            }
            
            return rc;
        }
    }
    else
    {
        rc = clOmMoIdToOmObjectReferenceGet(&moId, &pOmObj);
        if (rc != CL_OK)
        {
            ClNameT moidName = {0};
            (void)clCorMoIdToMoIdNameGet(&moId, &moidName);
            /*
             * It could be created from builtin mib implementation in COR.
             * Try adding a reference.
             */
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            {
                clLogInfo("PRV", "INI", "Adding OM reference for MoId [%.*s]",
                          moidName.length, moidName.value);

                rc = clProvOmObjectCreate(corTxnId, &pOmObj);
                if(rc != CL_OK)
                    goto createError;
            }
            else
            {
                clLogError("PRV", "INI", "OM object for MOID [%.*s] returned [%#x]", 
                           moidName.length, moidName.value, rc);
                return rc;
            }
        }
    }

    txnData.txnJobData.op = CL_PROV_VALIDATE_STATE;
    txnData.txnJobData.handle = txnHandle;

    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart != NULL)
    {
        (void) (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart(
                                                                  &moId, txnData.txnJobData.handle));
    }

    if ( ((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectValidate )
    {
        txnData.pTxnDataList = NULL;
        txnData.txnDataEntries = 0;
        retCode = clCorTxnJobWalk(corTxnId, clProvTxnListJobWalk, (void *)&txnData);
        if(retCode == CL_OK)
        {
            clProvTxnObjectInvoke(corTxnId, pOmObj, &txnData);
        }
    }
    else
    {
        retCode = clCorTxnJobWalk(corTxnId, clProvTxnJobWalk, (void *)&txnData.txnJobData);
    }

    if (retCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Job got failed rc [0x%x]. Updating the txn handle..", retCode));
        
        if ((rc = clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
        }
    }

    CL_FUNC_EXIT();
    return retCode;
}

ClRcT clProvOmObjectCreate(ClCorTxnIdT corTxnId, void** ppOmObj)
{
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_OK;
    ClOmClassTypeT omClass = 0;
    ClHandleT handle = 0;
    ClCorMOIdT moId;
    ClPtrT       pOmObj = NULL;
   
    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the moId"));
        return rc;
    }
    
    /* Get the OM classId from the job header. */
    rc = clCorTxnJobOmClassIdGet(corTxnId, &omClass);
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_GET_OM_CLASS, rc );

    retCode = clOmObjectCreate( omClass, 1, &handle, &pOmObj, ( void* )&moId, sizeof( ClCorMOIdT ) );
    if((retCode != CL_OK) && (pOmObj == NULL))
    {
        CL_FUNC_EXIT();
        clLogError("PRV", "TXN", (ClCharT*) CL_PROV_LOG_1_OM_OBJ_CREATE);
        return retCode;
    }

    clOsalMutexLock( gClProvMutex );
    rc = clCntNodeAdd( gClProvOmHandleInfo, ( ClCntKeyHandleT )(ClWordT)handle, 0, NULL );
    clOsalMutexUnlock( gClProvMutex );

    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_CONTAINER_ADD, rc );

    rc = clOmMoIdToOmIdMapInsert( &moId, handle );
    
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_MAP_MOID_OM, rc );

    *ppOmObj = pOmObj;

    return retCode;
}

ClRcT
clProvTxnUpdate( CL_IN       ClTxnTransactionHandleT txnHandle,
                 CL_IN       ClTxnJobDefnHandleT     jobDefn,
                 CL_IN       ClUint32T               jobDefnSize,
                 CL_IN       ClCorTxnIdT             corTxnId)
{
    ClRcT                   rc          = CL_OK;
    ClCorTxnJobIdT          jobId       = 0;
    ClProvTxnListJobWalkDataT   txnData     = {{0}}; 
    ClCorMOIdT              moId        = {{{0}}};
    ClPtrT                  pOmObj      = NULL;
    ClHandleT               handle      = 0;
    ClRcT                   retCode     = CL_OK;
    ClCorOpsT               op          = 0;

    CL_FUNC_ENTER();

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "RLB", "Failed to get the moId from Txn Id. rc [0x%x]", rc);
        return rc;
    }
    
    rc = clOmMoIdToOmObjectReferenceGet(&moId, &pOmObj);
    if (rc != CL_OK)
    {
        clLogError("PRV", "RLB", "Failed to get the OM Object reference from the MoId. rc [0x%x]", rc);
        return rc;
    }

    txnData.txnJobData.op = CL_PROV_UPDATE_STATE;
    txnData.txnJobData.handle = txnHandle;

    if ( ((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectUpdate )
    {
        txnData.pTxnDataList = NULL;
        txnData.txnDataEntries = 0;
        retCode = clCorTxnJobWalk(corTxnId, clProvTxnListJobWalk, (void *)&txnData);
        if(retCode == CL_OK)
        {
            clProvTxnObjectInvoke(corTxnId, pOmObj, &txnData);
        }
    }
    else
    {
        retCode = clCorTxnJobWalk(corTxnId, clProvTxnJobWalk, (void *)&txnData.txnJobData);
    }

    if (retCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Job got failed rc [0x%x]. Updating the txn handle..", retCode));
        
        if((rc = clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId)) != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
        }
    }

    /* Call end of Object operations callback */
    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd != NULL)
    {
        (void) (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd(
                                                                &moId, txnData.txnJobData.handle));
    }

    /* Delete the OM object */
    rc = clCorTxnFirstJobGet(corTxnId, &jobId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the first job. rc [0x%x]", rc));
        return retCode;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &op);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the operation type. rc [0x%x]", rc));
        return retCode;
    }
 
    if (op == CL_COR_OP_DELETE)
    {
        rc = clOmMoIdToOmHandleGet( &moId, &handle );
        if (rc != CL_OK)
        {
            clLogError("PRV", "UPD", CL_PROV_LOG_1_OM_HANDLE_GET, rc);
            return retCode; 
        }

        clOsalMutexLock( gClProvMutex );
        ( void )clCntAllNodesForKeyDelete( gClProvOmHandleInfo, ( ClCntKeyHandleT ) (ClWordT) handle );
        clOsalMutexUnlock( gClProvMutex );
    }

    CL_FUNC_EXIT();
    return retCode;
}

ClRcT
clProvTxnRollback( CL_IN       ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId)
{
    ClRcT                   rc          = CL_OK;
    ClRcT                   retCode     = CL_OK;
    ClProvTxnListJobWalkDataT   txnData     = {{0}}; 
    ClCorTxnJobIdT          jobId       = 0;
    ClCorOpsT               op          = 0;
    ClCorMOIdT              moId        = {{{0}}};
    ClPtrT                  pOmObj      = NULL;

    CL_FUNC_ENTER();

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "RLB", "Failed to get the moId from Txn Id. rc [0x%x]", rc);
        return rc;
    }
    
    rc = clOmMoIdToOmObjectReferenceGet(&moId, &pOmObj);
    if (rc != CL_OK)
    {
        clLogError("PRV", "RLB", "Failed to get the OM Object reference from the MoId. rc [0x%x]", rc);
        return rc;
    }

    txnData.txnJobData.op = CL_PROV_ROLLBACK_STATE;
    txnData.txnJobData.handle = txnHandle;

    if ( ((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectRollback )
    {
        txnData.pTxnDataList = NULL;
        txnData.txnDataEntries = 0;
        retCode = clCorTxnJobWalk(corTxnId, clProvTxnListJobWalk, (void *)&txnData);
        if(retCode == CL_OK)
        {
            clProvTxnObjectInvoke(corTxnId, pOmObj, &txnData);
        }
    }
    else
    {
        retCode = clCorTxnJobWalk((ClCorTxnIdT) corTxnId, clProvTxnJobWalk, (void *)&txnData.txnJobData);
    }

    if (retCode != CL_OK)
    { 
        if (CL_GET_ERROR_CODE(retCode) == CL_COR_TXN_ERR_JOB_WALK_TERMINATE)
        {
            clLogInfo("PRV", "TXN", "Txn rollback got terminated. rc [0x%x]", retCode);
            retCode = CL_OK;
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Job got failed rc [0x%x]. Updating the txn handle..", retCode));
        
            if ((rc = clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
            }
        }   
    }
 
    /* Call end of Object operations callback */
    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd != NULL)
    {
        (void) (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd(
                                                                &moId, txnData.txnJobData.handle));
    }

    rc = clCorTxnFirstJobGet(corTxnId, &jobId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the first job. rc [0x%x]", rc));
        clCorTxnIdTxnFree(corTxnId);
        return retCode;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &op);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the operation type. rc [0x%x]", rc));
        return retCode;
    }
        
    if ((op == CL_COR_OP_CREATE_AND_SET) || (op == CL_COR_OP_CREATE))
    {   
        rc = clProvOmObjectDelete(corTxnId);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the om object. rc [0x%x]", rc));
            return retCode;
        }
    }
    
    CL_FUNC_EXIT();
    return ( retCode );
}

ClRcT clProvOmObjectDelete(ClCorTxnIdT corTxnId)
{
    ClCorMOIdT moId;
    ClRcT rc = CL_OK;
    ClHandleT handle;

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the moId. rc [0x%x]\n", rc));
        return rc;
    }

    rc = clOmMoIdToOmHandleGet( &moId, &handle );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                           CL_PROV_LOG_1_OM_HANDLE_GET, rc );

    clOsalMutexLock( gClProvMutex );
    ( void )clCntAllNodesForKeyDelete( gClProvOmHandleInfo, ( ClCntKeyHandleT )(ClWordT)handle );
    clOsalMutexUnlock( gClProvMutex );

    return rc;
}

ClRcT
clProvReadSession( CL_IN       ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corSessionId)
{
    ClRcT                   rc              = CL_OK;
    ClProvTxnListJobWalkDataT   provJobWalk     = {{0}};
    ClRcT                   retCode         = CL_OK;
    ClPtrT                  pOmObj          = NULL;
    ClCorMOIdT              moId            = {{{0}}};

    CL_FUNC_ENTER();

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Inside read callback "));

    rc = clCorTxnJobMoIdGet(corSessionId, &moId);
    if (rc != CL_OK)
    {
        clLogError("PRV", "RLB", "Failed to get the moId from Txn Id. rc [0x%x]", rc);
        return rc;
    }
    
    rc = clOmMoIdToOmObjectReferenceGet(&moId, &pOmObj);
    if (rc != CL_OK)
    {
        ClNameT moidName = {0};
        (void)clCorMoIdToMoIdNameGet(&moId, &moidName);
        /*
         * It could be created from builtin mib implementation in COR.
         * Try adding a reference.
         */
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            clLogInfo("PRV", "INI", "Adding OM reference for MoId [%.*s]",
                      moidName.length, moidName.value);
            rc = clProvOmObjectCreate(corSessionId, &pOmObj);
            if(rc != CL_OK)
            {
                clLogError("PRV", "RLB", "Failed to create OM object for MoId [%.*s]. rc [0x%x]",
                        moidName.length, moidName.value, rc);
                return rc;
            }
        }
        else
        {
            clLogError("PRV", "INI", "OM object for MOID [%.*s] returned [%#x]", 
                       moidName.length, moidName.value, rc);
            return rc;
        }
    }
 
    /*Initialize the handle. */
    provJobWalk.txnJobData.handle = txnHandle;
    provJobWalk.txnJobData.op = CL_PROV_READ_STATE;

    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart != NULL)
    {
        (void) (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart(
                    &moId, provJobWalk.txnJobData.handle));
    }

    if ( ((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectRead )
    {
        provJobWalk.pTxnDataList = NULL;
        provJobWalk.txnDataEntries = 0;
        retCode = clCorTxnJobWalk(corSessionId, clProvTxnListJobWalk, (void *)&provJobWalk);
        if(retCode == CL_OK)
        {
            clProvTxnObjectInvoke(corSessionId, pOmObj, &provJobWalk);
        }
    }
    else
    {
        retCode = clCorTxnJobWalk((ClCorTxnIdT) corSessionId, clProvTxnJobWalk, (void *)&provJobWalk.txnJobData);
    }

    if (CL_OK != retCode)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Job got failed rc [0x%x]. Updating the txn handle..", retCode));
    }
    
    /* Updating the job */
    if ((rc = clCorTxnJobDefnHandleUpdate(jobDefn, corSessionId)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to update the txn handle. rc [0x%x]", rc));
    }

    /* Call the object-end callback function */
    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd != NULL)
    {
        (void) ((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd(
                &moId, provJobWalk.txnJobData.handle);
    }

    CL_FUNC_EXIT();
    return ( retCode );
}

/***************************************************************************************************
                                P R O V     Base OM Class 
 ***************************************************************************************************/
ClRcT
clProvOMClassConstructor( void* pThis,
                        void* pUsrData,
                        ClUint32T usrDataLen )
{
    return CL_OK;
}

ClRcT
clProvOMClassDestructor( void* pThis,
                       void* pUsrData,
                       ClUint32T usrDataLen )
{
    return CL_OK;
}


ClRcT clProvVersionCheck(ClVersionT* pVersion)
{
    ClRcT rc = CL_OK;

    if(NULL == pVersion)
    {
        return CL_PROV_RC(CL_ERR_NULL_POINTER);
    }

    /*
     * Verify the version information 
     */
    rc = clVersionVerify(&versionDatabase, pVersion);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, CL_PROV_LIB_NAME,
                   CL_LOG_MESSAGE_0_VERSION_MISMATCH);
    }
    
    return rc;
}

/** This function is called when component is terminated.
 *  1. Delete the mutex.
 *  2. Delete the OM handle container.
 *  3. Delete the Prov OM Base object.
 *  4. Deregister with TXN.
 */
ClRcT
clProvFinalize()
{
    ClRcT   rc  = CL_OK;

    CL_FUNC_ENTER();

    if ( CL_FALSE == gProvInit )
    {
        CL_FUNC_EXIT();
        return CL_PROV_RC(CL_PROV_ERR_NOT_INITIALIZED);
    }

    clOsalMutexDelete( gClProvMutex );
    clCntDelete( gClProvOmHandleInfo );

    clProvTxnDeregister();

    rc = clProvOmClassDelete();
    clLogWrite( CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, CL_PROV_LIB_NAME,
                CL_PROV_LOG_0_PROV_CLEAN_DONE );
    
    /* Remove the global resources list. */
    clHeapFree(pProvMoIdList);

    gProvInit = CL_FALSE;
    CL_FUNC_EXIT();
    return rc;
}

/** This function is called for the resource which is representented as wildcard.
 *  1. Create the OM for the given MoId and add it into OM db. 
 *  2. walk through the attribute of given object.
 */
ClRcT
clProvWildCardCorObjWalk( void* pData , void *cookie)
{
    ClRcT               rc              = CL_OK;
    ClCorObjectHandleT  corObjHandle    = *( ClCorObjectHandleT* )pData; 
    ClCorMOIdT          fullMoId;
    ClCorServiceIdT     srvcId          = CL_COR_SVC_ID_PROVISIONING_MANAGEMENT;

    CL_FUNC_ENTER();

    rc = clCorMoIdInitialize( &fullMoId );

    rc = clCorObjectHandleToMoIdGet( corObjHandle, &fullMoId, &srvcId );
    if (rc != CL_OK)
    {
        clLogError("PRV", "PRE", "Failed to get MoId from Object Handle. rc [0x%x]", rc);
        clCorObjectHandleFree(&corObjHandle);
        return rc;
    }

    if (srvcId != CL_COR_SVC_ID_PROVISIONING_MANAGEMENT)
    {
        clLogTrace("PRV", "PRE", "Not a Provisioning MSO. No need for pre-provisioning");
        clCorObjectHandleFree(&corObjHandle);
        return CL_OK;
    }

    rc = clProvOmObjectPrepare( &fullMoId );
    if (rc != CL_OK)
    {
        clLogError("PRV", "PRE", "Failed to configure the OM object. rc [0x%x]", rc);
        clCorObjectHandleFree(&corObjHandle);
        return rc;
    }

    clCorObjectHandleFree(&corObjHandle);

    CL_FUNC_EXIT();
    return rc;
}

/** This function is getting called when component is instanciated.
 *  1. Create container which is used to store OM handle.
 *  2. Create a mutex to protect the above container.
 *  3. Get the resource information which is there in the rt.xml.
 *  4. Create the prov base OM object.
 *  5. Get the Node level string MOID.
 *  6. If the resource is wildcard.
 *     6.1 Then walk through the object which are created.
 *  7. If the resource is non wildcarded.
 *     7.1 Then walk through the attribute for given Moid.
 *  8. Register with TXN agent to involve in the transaction.
 */
ClRcT
clProvInitialize()
{
    ClRcT                   rc              = CL_OK;
    ClCorMOIdT              fullMoId        = {{{0}}};    
    ClCorMOIdPtrT           rootMoId        = NULL;    
    ClUint32T               i               = 0;
    ClCorAddrT              provAddr        = {0};        
    ClOampRtResourceArrayT  resourcesArray = {0};    

    CL_FUNC_ENTER();

    rc = clCntLlistCreate( clProvKeyCompare, clProvUserDelete, clProvUserDestroy, CL_CNT_UNIQUE_KEY,
                           &gClProvOmHandleInfo );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_CRITICAL,
                            CL_PROV_LOG_1_CREATING_CONTAINER, rc );

    rc = clOsalMutexCreate( &gClProvMutex );
    if ( CL_OK != rc )
    {
        return rc;
    }

    rc = clMsoConfigResourceInfoGet(&provAddr, &resourcesArray);
    if (rc != CL_OK)
    {
        clLogError("PROV", "INI", "Failed to get the resource information. rc [0x%x]", rc);
        return rc;
    }

    rc = clProvOmClassCreate();
    if ( CL_OK != rc )
    {
        return rc;
    }

    rc = clMsoConfigRegister(CL_COR_SVC_ID_PROVISIONING_MANAGEMENT, gClProvMsoCallbacks);
    if (CL_OK != rc )
    {
        clLogError("PROV", "INI", "Failed to register prov callbacks with mso config library. rc [0x%x]", rc);
        return rc;
    }

    if (resourcesArray.noOfResources > 0)
    {
        /* Call the Transaction-Start callback */
        if (clProvTxnCallbacks.fpProvTxnStart != NULL)
        {
            /* Call the user supplied callback function */
            (void) clProvTxnCallbacks.fpProvTxnStart(0);
        }
        else
        {
            clLogInfo("PRV", "PRE", "User has not specified the Txn-Start callback function.");
        }

        /* Allocate prov resource list which is the global array 
         * which stores all the prov resources configured for this 
         * application. 
         */
        pProvMoIdList = clHeapAllocate(sizeof(ClCorMOIdListT) + resourcesArray.noOfResources * sizeof(ClCorMOIdT));
        if (!pProvMoIdList)
        {
            clLogError("PRV", "INI", "Failed to allocate memory.");
            return CL_PROV_RC(CL_ERR_NO_MEMORY);
        }

        memset(pProvMoIdList, 0, sizeof(ClCorMOIdListT) + resourcesArray.noOfResources * sizeof(ClCorMOIdT));

        for ( i = 0; i < resourcesArray.noOfResources; i++ )
        {
            rc = clCorMoIdInitialize( &fullMoId );
            CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_CRITICAL,
                                    CL_PROV_LOG_1_MOID_INIT, rc );


            rc = clProvProvisionOmCreateAndRuleAdd( &resourcesArray.pResources[i].resourceName,
                                                    &fullMoId, &provAddr,
                                                    resourcesArray.pResources[i].objCreateFlag,
                                                    resourcesArray.pResources[i].autoCreateFlag,
                                                    resourcesArray.pResources[i].primaryOIFlag,
                                                    resourcesArray.pResources[i].wildCardFlag );
            if ( CL_OK != rc )
            {
                /* Continue with the next resource */
                continue;
            }            
           
            if ( CL_TRUE == resourcesArray.pResources[i].wildCardFlag )
            {
                /**
                 * Before walking the wildcard moID needs to set the service ID as invalide
                 * This is the function requirement. by clCorObjectWalk funciton
                 */
                rc = clCorMoIdServiceSet( &fullMoId, CL_COR_INVALID_SVC_ID );

                rc = clCorMoIdAlloc(&rootMoId);
                rc = clCorMoIdInitialize(rootMoId);

                rc = clCorObjectWalk( rootMoId, &fullMoId, clProvWildCardCorObjWalk, CL_COR_MSO_WALK, NULL);
                rc = clCorMoIdFree(rootMoId);

                /*
                 * Add this rule so that if the object is created later we will get the notification
                 */
                rc = clCorMoIdServiceSet(&fullMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
                if (rc != CL_OK)
                {
                    clLogError("PRV", "INI", "Failed to set the service-id in the MoId. rc [0x%x]", rc);
                    return rc;
                }

                rc = clProvRuleAdd( &fullMoId, &provAddr );
                if ( CL_OK != rc )
                {
                    clLogError("PRV", "INI", "Failed to register as the OI for the MO. rc [0x%x]", rc);
                    return rc;
                }

                /* Register the primary OI. */
                if( CL_TRUE == resourcesArray.pResources[i].primaryOIFlag)
                {
                    clLogTrace("PRV", "INI", "Setting the OI with addr [0x%x: 0x%x] as the primary OI",
                    provAddr.nodeAddress, provAddr.portId);

                    rc = clCorPrimaryOISet(&fullMoId, &provAddr);
                    if(CL_OK != rc)
                    {
                        clLogError("PRV", "INT", "Primary-OI registration failed. rc [0x%x]", rc);
                        return rc;
                    }
                }
            }
        }

        if (clProvTxnCallbacks.fpProvTxnEnd != NULL)
        {
            (void) clProvTxnCallbacks.fpProvTxnEnd(0);
        }
        else
        {
            clLogInfo("PRV", "TXN", "User has not supplied the Transaction End callback function.");
        }
    }

    if( resourcesArray.noOfResources != 0)
        clHeapFree( resourcesArray.pResources );

    clLogNotice("PRV", "INT", CL_PROV_LOG_0_PROV_INIT_DONE );

    gProvInit = CL_TRUE;
    CL_FUNC_EXIT();
    return CL_OK;
}

/** 
 * This function is used to handle the array attributes
 */
void
clProvArrayAttrInfoGet( ClCorAttrDefT* pAttrDef,
                        ClUint32T* pDataLen )
{
    ClUint32T   arrDataType = 0;
    ClUint32T   noOfElement = 0;

    noOfElement = pAttrDef->u.attrInfo.maxElement;
    arrDataType = pAttrDef->u.attrInfo.arrDataType;
    switch ( arrDataType )
    {
      case CL_COR_INT8:
      case CL_COR_UINT8:
        *pDataLen = sizeof( ClUint8T ) * noOfElement;
        break;
      case CL_COR_INT16:
      case CL_COR_UINT16:
        *pDataLen = sizeof( ClUint16T ) * noOfElement;
        break;
      case CL_COR_INT32:
      case CL_COR_UINT32:
        *pDataLen = sizeof( ClUint32T ) * noOfElement;
        break;

      case CL_COR_INT64:
      case CL_COR_UINT64:
        *pDataLen = sizeof( ClUint64T ) * noOfElement;
        break;
    }
    return;
}

void
clProvObjAttrFree(void *cookie)
{
    ClProvObjAttrArgsT *pArg = cookie;
    if(pArg)
    {
        ClUint32T i;
        for(i = 0; i < pArg->txnDataEntries; ++i)
        {
            if(pArg->pProvTxnData[i].attrPath)
                clCorAttrPathFree(pArg->pProvTxnData[i].attrPath);
            if(pArg->pProvTxnData[i].pProvData)
                clHeapFree(pArg->pProvTxnData[i].pProvData);
        }
        if(pArg->pProvTxnData)
            clHeapFree(pArg->pProvTxnData);
    }
}

ClRcT
clProvObjAttrWalk(ClCorAttrPathPtrT pAttrPath,
                  ClCorAttrIdT attrId,
                  ClCorAttrTypeT attrType,
                  ClCorTypeT attrDataType,
                  void* value,
                  ClUint32T size,
                  ClCorAttrFlagT attrFlag,
                  void* cookie )
{
    ClProvObjAttrArgsT *pArg =  cookie;
    ClUint32T       currentTxnDataEntries = 0;
    ClCorAttrPathPtrT pObjAttrPath = NULL;
    void *pObjAttrValue = NULL;
    ClRcT rc = CL_OK;

    if(!pArg)
        return CL_PROV_RC(CL_ERR_INVALID_PARAMETER);

    /* Preprovisioning is done only for config attributes.*/
    if(!(attrFlag & CL_COR_ATTR_CONFIG))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Only Config attributes are applied while pre-provisioning."));
        return CL_OK;
    }

    currentTxnDataEntries = pArg->txnDataEntries;

    pArg->pProvTxnData = clHeapRealloc(pArg->pProvTxnData, (currentTxnDataEntries+1)*
                                       sizeof(*pArg->pProvTxnData));
    CL_ASSERT(pArg->pProvTxnData != NULL);
    memset(pArg->pProvTxnData + currentTxnDataEntries, 0, sizeof(*pArg->pProvTxnData));

    if(pAttrPath)
    {
        rc = clCorAttrPathAlloc(&pObjAttrPath);
        if(rc != CL_OK)
            return rc;
        memcpy(pObjAttrPath, pAttrPath, sizeof(*pObjAttrPath));
    }
    if(value)
    {
        pObjAttrValue = clHeapCalloc(1, size);
        CL_ASSERT(pObjAttrValue != NULL);
        memcpy(pObjAttrValue, value, size);
    }

    pArg->pProvTxnData[currentTxnDataEntries].provCmd    = CL_COR_OP_SET;
    pArg->pProvTxnData[currentTxnDataEntries].pMoId      = pArg->pMoId;
    pArg->pProvTxnData[currentTxnDataEntries].attrPath   = pObjAttrPath;
    pArg->pProvTxnData[currentTxnDataEntries].attrType   = attrType;
    pArg->pProvTxnData[currentTxnDataEntries].attrId     = attrId;
    pArg->pProvTxnData[currentTxnDataEntries].pProvData  = pObjAttrValue;
    pArg->pProvTxnData[currentTxnDataEntries].size       = size;
    pArg->pProvTxnData[currentTxnDataEntries++].index      = 0;
    pArg->txnDataEntries = currentTxnDataEntries;

    return CL_OK;
}

/** This function a call back function, this called for each attribute of given MoId.
 *  1. Get the OM object reference for given MoId.
 *  2. Then call following functions of application.
 *  3. Validate, Update, PostValidate.
 */
ClRcT
clProvAttrWalk( ClCorAttrPathPtrT pAttrPath,
                ClCorAttrIdT attrId,
                ClCorAttrTypeT attrType,
                ClCorTypeT attrDataType,
                void* value,
                ClUint32T size,
                ClCorAttrFlagT attrFlag,
                void* cookie )
{
    ClRcT           rc          = CL_OK;
    ClCorMOIdPtrT   moId        = ( ClCorMOIdPtrT )cookie;
    void*           pOmProvObj  = NULL;
    ClProvTxnDataT  provTxnData;
    ClRcT           retCode     = CL_OK;

    /* Preprovisioning is done only for config attributes.*/
    if(!(attrFlag & CL_COR_ATTR_CONFIG))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Only Config attributes are applied while pre-provisioning."));
        return CL_OK;
    }

    rc = clOmMoIdToOmObjectReferenceGet( moId, &pOmProvObj );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_MOID_OM_GET, rc );

    provTxnData.provCmd    = CL_COR_OP_SET;
    provTxnData.pMoId      = moId;
    provTxnData.attrPath   = pAttrPath;
    provTxnData.attrType   = attrType;
    provTxnData.attrId     = attrId;
    provTxnData.pProvData  = value;
    provTxnData.size       = size;
    provTxnData.index      = 0;

    /* First call validate function */
    retCode = ( ( CL_OM_PROV_CLASS * )pOmProvObj )->clProvValidate( pOmProvObj, 0, &provTxnData );
    if (retCode != CL_OK)
    {
        clLogError("PRV", "VAL", CL_PROV_LOG_1_SET_VALIDATE, retCode);

        /* Application's VALIDATE callback failed, call ROLLBACK */

        retCode = ( ( CL_OM_PROV_CLASS * )pOmProvObj )->clProvRollback( pOmProvObj, 0, &provTxnData );
        if (retCode != CL_OK)
        {
            clLogError("PRV", "VAL", CL_PROV_LOG_1_SET_ROLLBACK, retCode);
        }

        CL_FUNC_EXIT();
        return CL_OK;
    }

    /* Validate got passed, call the application's UPDATE callback */
    CL_PROV_CHECK_RC1_LOG1( retCode, CL_OK, retCode, CL_LOG_INFORMATIONAL,
                            CL_PROV_LOG_1_SET_UPDATE, retCode );

    /* Call update */
    retCode = ( ( CL_OM_PROV_CLASS * )pOmProvObj )->clProvUpdate( pOmProvObj, 0, &provTxnData );
    if (retCode != CL_OK)
    {
        clLogInfo("PRV", "PRE", CL_PROV_LOG_1_SET_UPDATE, retCode);
    }

    return CL_OK;
}

