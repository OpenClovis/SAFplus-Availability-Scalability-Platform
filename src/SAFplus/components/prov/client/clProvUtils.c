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
 * ModuleName  : prov
 * File        : clProvUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the Provision Library implementation.
 *****************************************************************************/


/* Standard Inculdes */
#include <string.h>  /* strcpy is needed for eo name */
#include <ctype.h>

/* ASP Includes */
#include <clCommon.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clOmApi.h>
#include <clCorErrors.h>
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

#ifdef MORE_CODE_COVERAGE
#include "clCodeCoveApi.h"
#endif


#define PROV_OM_CLASS_NAME           "provOMClass"
#define PROV_OM_CLASS_VERSION        0x001
#define PROV_OM_CLASS_MAX_SLOTS      1
#define MAX_OBJ 256

extern ClRcT    clCorNIClassIdGet( char* name,
                                   ClCorClassTypeT* pClassId );

extern ClCharT*                gProvLogMsgDb[];
extern ClCorMOIdListT* pProvMoIdList;

ClTxnAgentServiceHandleT    gProvTxnRegisterHandle = 0;
ClTxnAgentServiceHandleT    gProvSessionRegisterHandle = 0;

/**
 * Application callback functions for Transaction.
 */
extern ClProvTxnCallbacksT clProvTxnCallbacks;

/*
 * Prov OM class Table
 */
ClOmClassControlBlockT      omProvClassTbl[]    =
{
    {
    PROV_OM_CLASS_NAME,         /* object */
    sizeof( CL_OM_PROV_CLASS ), /* size */
    CL_OM_BASE_CLASS_TYPE,      /* extends from */
    clProvOMClassConstructor,   /* constructor */
    clProvOMClassDestructor,    /* destructor */
    NULL,                       /* pointer to methods struct */
    PROV_OM_CLASS_VERSION,      /* version */
    0,                          /* Instance table ptr */
    MAX_OBJ,                    /* Maximum number of classes */
    0,                          /* cur instance count */
    PROV_OM_CLASS_MAX_SLOTS,    /* max slots */
    CL_OM_PROV_CLASS_TYPE       /* my class type */
    }
};

ClMsoConfigCallbacksT gClProvMsoCallbacks = {
    .fpMsoJobPrepare = clProvTxnValidate,
    .fpMsoJobCommit  = clProvTxnUpdate,
    .fpMsoJobRollback = clProvTxnRollback,
    .fpMsoJobRead = clProvReadSession
};

/* 
 * This function used to deregister with transation agent
 */
void
clProvTxnDeregister()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    rc = clTxnAgentServiceUnRegister(gProvTxnRegisterHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deregistration of the transaction agent . rc[0x%x]", rc));
        return ;
    }

    rc = clTxnAgentServiceUnRegister(gProvSessionRegisterHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deregistration of the transaction agent . rc[0x%x]", rc));
        return ;
    }

    CL_FUNC_EXIT();
    return;
}


/* This function is called when the resource object needs to be precreated by component is comming up.
 *  1. Find out if the object is already created. (This can be created by North bound or componet which already
 *     Instanciated)
 *  2. If the object is already created then do nothing, simply return CL_OK.
 *  3. If it is not create then create the object for provisoning. 
 *  4. Register with COR to know any change in the object.
 */
ClRcT
clProvIfCreateFlagIsTrue( ClCorMOIdPtrT pFullMoId,
                          ClCorAddrT* pProvAddr ,
                          ClUint32T   primaryOIFlag,
                          ClBoolT     autoCreateFlag)
{
    ClRcT               rc  = CL_OK;
    ClCorObjectHandleT  corObjHandle = NULL;    
    CL_FUNC_ENTER();

    if (!autoCreateFlag)
    {
        rc = clCorObjectHandleGet(pFullMoId, &corObjHandle);
        if (rc != CL_OK)
        {
            rc = clCorObjectCreate( NULL, pFullMoId, NULL);
            if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MO_ALREADY_PRESENT))
            {
                clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT *) CL_PROV_LOG_1_CREATING_OBJECT, rc);
                CL_FUNC_EXIT();
                return CL_PROV_RC(CL_PROV_INTERNAL_ERROR);
            }
        }
        else
            clCorObjectHandleFree(&corObjHandle);
    }

    rc = clCorMoIdServiceSet( pFullMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR, CL_PROV_LOG_1_SERVICE_SET,
                            rc );
        
    if (!autoCreateFlag)
    {
        rc = clCorObjectHandleGet(pFullMoId, &corObjHandle);
        if (rc != CL_OK)
        {
            rc = clCorObjectCreate( NULL, pFullMoId, NULL );
            if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MSO_ALREADY_PRESENT))
            {
                clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT *) CL_PROV_LOG_1_CREATING_OBJECT, rc);
                CL_FUNC_EXIT();
                return CL_PROV_RC(CL_PROV_INTERNAL_ERROR);
            }
        }
        else
            clCorObjectHandleFree(&corObjHandle);
    }

    rc = clProvOmObjectPrepare( pFullMoId );

    /*
     * This should not called before creating MSO. In that case, if the application has only one thred it 
     * will time out. Because clcorobjectcreate function will do sync RMD to cor. then cor will initiate the 
     * transation 
     */
    rc = clProvRuleAdd( pFullMoId, pProvAddr );
    if ( CL_OK != rc )
    {
        clLogError("PRV", "PRE", "Failed to register for the MO as the OI. rc [0x%x]", rc);
        return rc;
    }

    /* Register the primary OI. */
    if( CL_TRUE == primaryOIFlag)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Is primaryOI flag is %d, addr [0x%x: 0x%x] ",
                                       primaryOIFlag, pProvAddr->nodeAddress, pProvAddr->portId));

        rc = clCorPrimaryOISet(pFullMoId, pProvAddr);
        if(CL_OK != rc)
        {
            clLogError("PRV", "PRE", "Primary OI registration failed. rc [0x%x]", rc);
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/* This function is called when the resource object not precreated by component is comming up.
 * 1. Register with COR to know any change in the object.
 */
ClRcT
clProvIfCreateFlagIsFalse( ClCorMOIdPtrT pFullMoId,
                           ClCorAddrT* pProvAddr,
                           ClUint32T   primaryOIFlag )
{
    ClRcT               rc  = CL_OK, ret = CL_OK;
    ClCorObjectHandleT  corObjHandle = NULL;
    ClCorMOIdT          tempMoId;

    CL_FUNC_ENTER();

    ( void )clCorMoIdInitialize( &tempMoId );
    memcpy( &tempMoId, pFullMoId, sizeof( ClCorMOIdT ) );

    ( void )clCorMoIdServiceSet( &tempMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT );

    /*
     * If we expect object will be created by default then of the object is not created then return error.
     */
    rc = clCorObjectHandleGet( pFullMoId, &corObjHandle );
    if ( CL_OK != rc )
    {
        ret = CL_PROV_RC(CL_PROV_INTERNAL_ERROR);
        goto handleError;
    }    
    else
        clCorObjectHandleFree(&corObjHandle);

    ( void )clCorMoIdServiceSet( pFullMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT );

    /*
     * If we expect object will be created by default then of the object is not created then return error.
     */
    rc = clCorObjectHandleGet( pFullMoId, &corObjHandle );
    if(CL_OK != rc)
    {
        ret = CL_PROV_RC(CL_PROV_INTERNAL_ERROR);
        goto handleError;
    }
    else
        clCorObjectHandleFree(&corObjHandle);

    rc = clProvOmObjectPrepare( pFullMoId );

handleError:
    /*
     * Add this rule so that if the object is created later, component will get the notification.
     */
    rc = clProvRuleAdd( &tempMoId, pProvAddr );
    if(CL_OK != rc)
    {
        clLogError("PRV", "PRE", "Failed to register for the MO as the OI. rc [0x%x]", rc);
        return rc;
    }

    /* Register the primary OI. */
    if( CL_TRUE == primaryOIFlag)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Is PriamryOI flag is %d, addr [0x%x: 0x%x] ", 
                        primaryOIFlag, pProvAddr->nodeAddress, pProvAddr->portId));

        rc = clCorPrimaryOISet(&tempMoId, pProvAddr);
        if ( CL_OK != rc )
        {
                clLogError("PRV", "PRE", "Primary-OI set failed. rc [0x%x]", rc);
                return rc; 
        }
    }

    CL_FUNC_EXIT();
    return ret;
}

ClRcT
clProvObjectCreate(ClCorMOIdPtrT pMoId, ClCorAttributeValueListPtrT attrList, ClCorObjectHandleT* pHandle)
{
    ClRcT rc = CL_OK;
    ClCorMOIdPtrT pTempMoId = NULL;
    ClCorObjectHandleT moHandle = NULL;
    ClCorObjectHandleT msoHandle = NULL;
    ClCorTxnSessionIdT tid = 0;
    ClCorAddrT provAddr = {0};

    CL_FUNC_ENTER();
    
    /* Create the MO object */
    if(NULL == pMoId)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nNULL parameter passed\n"));
        CL_FUNC_EXIT();
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    rc = clCorMoIdClone(pMoId, &pTempMoId);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while cloning the MoId. rc[0x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clCorMoIdServiceSet(pTempMoId, CL_COR_INVALID_SRVC_ID);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the service id of the MoId. rc [0x%x]", rc));
        clCorMoIdFree(pTempMoId);
        CL_FUNC_EXIT();
        return rc;
    }
    
    tid = 0;
    
    if (CL_OK != clCorObjectHandleGet(pTempMoId, &moHandle))
    {
        rc = clCorObjectCreate(&tid, pTempMoId, NULL);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to add MO object creation in txn. rc [0x%x]", rc));
            clCorMoIdFree(pTempMoId);
            CL_FUNC_EXIT();
            return rc;
        }        
    }
    else
        clCorObjectHandleFree(&moHandle);

    rc = clCorMoIdServiceSet(pTempMoId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the service id of the MoId. rc [0x%x]\n", rc));
        clCorMoIdFree(pTempMoId);
        clCorTxnSessionFinalize(tid);
        CL_FUNC_EXIT();
        return rc;
    }

    if (CL_OK != clCorObjectHandleGet(pTempMoId, &msoHandle))
    {
        rc = clCorObjectCreateAndSet(&tid, pTempMoId, attrList, &msoHandle);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("Failed to add create and set for the MSO object in txn. rc [0x%x]", rc));
            clCorMoIdFree(pTempMoId);
            clCorTxnSessionFinalize(tid);
            CL_FUNC_EXIT();
            return (rc);
        }

        rc = clCorTxnSessionCommit(tid);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to commit the transaction. rc [0x%x]", rc));
            clCorObjectHandleFree(&msoHandle);
            clCorMoIdFree(pTempMoId);
            clCorTxnSessionFinalize(tid);
            CL_FUNC_EXIT();
            return rc;            
        }

        provAddr.nodeAddress = clIocLocalAddressGet();
        rc = clEoMyEoIocPortGet(&provAddr.portId);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the IOC port. rc [0x%x]", rc));
            clCorObjectHandleFree(&msoHandle);
            clCorMoIdFree(pTempMoId);
            CL_FUNC_EXIT();
            return rc;
        }
        
        rc = clCorOIRegister(pTempMoId, &provAddr);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to register the OI with COR. rc [0x%x]", rc));
            clCorObjectHandleFree(&msoHandle);
            clCorMoIdFree(pTempMoId);
            CL_FUNC_EXIT();
            return rc;
        }
    }
    
    clCorMoIdFree(pTempMoId);

    /* Assign the prov mso handle */
    *pHandle = msoHandle;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/*
 * To delete the Mo and MSo objects.
 */

ClRcT clProvObjectDelete(ClCorObjectHandleT pHandle)
{
    ClRcT rc = CL_OK;
    ClCorTxnSessionIdT tid = 0;
    ClCorMOIdPtrT pMoId = NULL;
    ClCorObjectHandleT objHandle = NULL;
    ClCorServiceIdT svcId = 0;
    ClUint32T i = 0;

    CL_FUNC_ENTER();

    rc = clCorMoIdAlloc(&pMoId);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate MoId. rc [0x%x]", rc));
        CL_FUNC_EXIT();
        return rc;
    }
    
    rc = clCorObjectHandleToMoIdGet(pHandle, pMoId, &svcId);
    if (rc != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the MoId from object handle. rc [0x%x]", rc));
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return rc;
    }

    for (i = CL_COR_SVC_ID_DEFAULT + 1; i < CL_COR_SVC_ID_MAX; i++)
    {
        rc = clCorMoIdServiceSet(pMoId, (ClCorMOServiceIdT) i);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the service Id in the MoId. rc [0x%x]", rc));
            clCorMoIdFree(pMoId);
            CL_FUNC_EXIT();
            return rc;
        }

        rc = clCorObjectHandleGet(pMoId, &objHandle);
        if ((rc == CL_OK) && (CL_COR_SVC_ID_PROVISIONING_MANAGEMENT != i))
        {
            /* Some other mso object exists. 
             * Delete the provisioning mso and exit.
             */
            /* Free the object handle */
            clCorObjectHandleFree(&objHandle);

            rc = clCorMoIdServiceSet(pMoId, (ClCorMOServiceIdT) CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the service Id in the MoId. rc [0x%x]", rc));
                clCorMoIdFree(pMoId);
                CL_FUNC_EXIT();
                return rc;
            }

            rc = clCorObjectHandleGet(pMoId, &objHandle);
            if (rc == CL_OK)
            {
                rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, objHandle);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the MSo object. rc [0x%x]", rc));
                    clCorMoIdFree(pMoId);
                    CL_FUNC_EXIT();
                    return rc;
                }
            }

            clCorMoIdFree(pMoId);
            CL_FUNC_EXIT();
            return CL_OK;
        }
    }
    
    tid = 0;
    
    rc = clCorMoIdServiceSet(pMoId, (ClCorMOServiceIdT) CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the service Id in the MoId. rc [0x%x]", rc));
        clCorMoIdFree(pMoId);
        CL_FUNC_EXIT();
        return rc;
    }
   
    rc = clCorObjectHandleGet(pMoId, &objHandle);
    if (rc == CL_OK)
    {
        rc = clCorObjectDelete(&tid, objHandle);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed delete the prov MSO object. rc [0x%x]\n", rc));
            clCorMoIdFree(pMoId);
            CL_FUNC_EXIT();
            return rc;
        }
    }
    
    rc = clCorMoIdServiceSet(pMoId, CL_COR_INVALID_SRVC_ID);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to set the svc id. rc [0x%x]", rc));
        clCorMoIdFree(pMoId);
        clCorTxnSessionFinalize(tid);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clCorObjectHandleGet(pMoId, &objHandle);
    if (rc == CL_OK)
    {
        rc = clCorObjectDelete(&tid, objHandle);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to delete the object. rc [0x%x]", rc));
            clCorMoIdFree(pMoId);   
            clCorTxnSessionFinalize(tid);
            CL_FUNC_EXIT();
            return rc;
        }
    }

    rc = clCorTxnSessionCommit(tid);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to commit the transaction. rc [0x%x]", rc));
        clCorMoIdFree(pMoId);
        clCorTxnSessionFinalize(tid);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Free the handle given by the user */
    clCorObjectHandleFree(&pHandle);

    clCorMoIdFree(pMoId);
    CL_FUNC_EXIT();
    return CL_OK;
}

/* 
 * This function is used to delete the PROV base object
 */
ClRcT
clProvOmClassDelete()
{
    ClRcT                   rc              = CL_OK;
    ClOmClassControlBlockT* pOmClassEntry   = NULL;

    CL_FUNC_ENTER();

    pOmClassEntry = &omProvClassTbl[0];
    rc = clOmClassFinalize( pOmClassEntry, pOmClassEntry->eMyClassType );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_ERROR, CL_PROV_LOG_1_PROV_FINALIZE,
                            rc );

    CL_FUNC_EXIT();
    return CL_OK;
}


/**************************************************************************************************
  Utility functions which are expected to implemented in other components
 ***************************************************************************************************/
ClRcT
clCpmComponentHandle( ClCpmHandleT* pCpmHandle )
{
    *pCpmHandle = 0;
    return CL_OK;
}

/* 
 * This function is used to get the slot MOID
 */
ClRcT
clCorNodeAddressToMoIdGet( ClIocNodeAddressT iocNodeAddress,
                           ClCorMOIdPtrT moId )
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();

    rc = clCorLogicalSlotToMoIdGet( iocNodeAddress, moId );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_NODE_MOID_GET_FROM_LOGICAL_SLOT, rc );

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
clProvRuleAdd( ClCorMOIdPtrT fullMoId,
               ClCorAddrT* pProvAddr )
{
    ClRcT   rc  = CL_OK;
    CL_FUNC_ENTER();

    rc = clCorOIRegister( fullMoId, pProvAddr );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_PROV_SERVICE_SET, rc );
    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 1. This function creates the Provision OM base class.
 2. Prov OM base class has to be created. Then only derived 
    Application OM class can be created. 
 */
ClRcT
clProvOmClassCreate()
{
    ClRcT                   rc              = CL_OK;
    ClOmClassControlBlockT* pOmClassEntry   = NULL;

    CL_FUNC_ENTER();

    pOmClassEntry = &omProvClassTbl[0];
    rc = clOmClassInitialize( pOmClassEntry, pOmClassEntry->eMyClassType, pOmClassEntry->maxObjInst,
                              pOmClassEntry->maxSlots );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, rc, CL_LOG_INFORMATIONAL,
                            CL_PROV_LOG_1_PROV_OM_INIT, rc );
    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT
clProvProvisionOmCreateAndRuleAdd( SaNameT* pResourceStringMoId,
                                   ClCorMOIdPtrT pFullMoId,
                                   ClCorAddrT* pProvAddr,
                                   ClUint32T createFlag,
                                   ClBoolT   autoCreateFlag,
                                   ClUint32T primaryOIFlag,
                                   ClUint32T wildCardFlag )
{
    ClRcT               rc          = CL_OK;
    ClCorMOClassPathT   moPath;

    CL_FUNC_ENTER();

    rc = clCorMoIdNameToMoIdGet( pResourceStringMoId, pFullMoId );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_MOID_STRING_MOID, rc );

    /*
     * If given Prov MSO is not associated for given resource then don't add the rule 
     * silently supperss the information.
     * Some case alarm is associated with some resource but for the same resource prov may not
     * be associated. In this case prov should not register for those resource with COR.
     */
    rc = clCorMoIdToMoClassPathGet( pFullMoId, &moPath );
    rc = clCorMSOClassExist( &moPath, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT );
    if ( CL_OK != rc )
    {
        return rc;
    }

    /*
     * Add to prov resource list 
     */
    memcpy(&pProvMoIdList->moId[pProvMoIdList->moIdCnt], pFullMoId, sizeof(ClCorMOIdT));
    clCorMoIdServiceSet(&pProvMoIdList->moId[pProvMoIdList->moIdCnt], CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    pProvMoIdList->moIdCnt++;

    if ( CL_TRUE == wildCardFlag )
    {
        return CL_OK;
    }

    if ( CL_FALSE == createFlag )
    {
        rc = clProvIfCreateFlagIsFalse( pFullMoId, pProvAddr, primaryOIFlag );
        if ( CL_OK != rc )
        {
            return rc;
        }
    }
    else
    {
        rc = clProvIfCreateFlagIsTrue( pFullMoId, pProvAddr, primaryOIFlag, autoCreateFlag );
        if ( CL_OK != rc )
        {
            return rc;
        }
    }

    CL_FUNC_EXIT();
    return rc;
}

/*
 *  This function used to generate the OM for MOID and store it in OM db
 *  1. Get the OM class type from MOID.
 *  2. Get the OM class from the OM class type.
 *  3. Create the OM object for given MOID
 *  4. Then add OM object handle to prov OM db.
 *  5. Insert the OM object into MoId and OM Object table.
 */
ClRcT
clProvOmObjectPrepare( ClCorMOIdPtrT pFullMoId)
{
    ClCorClassTypeT moClassType = 0;
    ClOmClassTypeT  omClass     = 0;    
    ClHandleT       handle;
    void*           pOmObj      = NULL;
    ClRcT           rc          = CL_OK;
    ClProvTxnDataT  provTxnData = {0};
    ClRcT           retCode     = CL_OK;
    ClCorObjectHandleT corObjHandle = NULL;

    CL_FUNC_ENTER();

    rc = clCorMoIdToClassGet( pFullMoId, CL_COR_MO_CLASS_GET, &moClassType );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_MO_CLASS_GET, rc );

    rc = clOmClassFromInfoModelGet( moClassType, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
                                    &omClass );

    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_GET_OM_CLASS, rc );

    retCode = clOmObjectCreate( omClass, 1, &handle, &pOmObj, ( void* )pFullMoId, sizeof( ClCorMOIdT ) );
    if (NULL == pOmObj)
    {
        CL_PROV_CHECK_RC1_EAQUAL_LOG0( pOmObj, NULL, retCode, CL_LOG_ERROR,
                                       CL_PROV_LOG_1_OM_OBJ_CREATE );
        CL_FUNC_EXIT();
        return retCode;
    }

    clOsalMutexLock( gClProvMutex );
    rc = clCntNodeAdd( gClProvOmHandleInfo, ( ClCntKeyHandleT )(ClWordT)handle, 0, NULL );
    clOsalMutexUnlock( gClProvMutex );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_INFORMATIONAL,
                            CL_PROV_LOG_1_CONTAINER_ADD, rc );

    rc = clOmMoIdToOmIdMapInsert( pFullMoId, handle );
    CL_PROV_CHECK_RC1_LOG1( rc, CL_OK, CL_PROV_RC(CL_PROV_INTERNAL_ERROR), CL_LOG_ERROR,
                            CL_PROV_LOG_1_MAP_MOID_OM, rc );

    provTxnData.provCmd    = CL_COR_OP_CREATE;
    provTxnData.pMoId      = pFullMoId;
    provTxnData.attrId     = CL_COR_INVALID_ATTR_ID;

    /* Call the application's Object Start callback function */

    if ((((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart) != NULL)
    {
        (void) (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectStart(
                                                                  pFullMoId, 0));
    }

    /* Call the Validate callback */
    if( ( (CL_OM_PROV_CLASS *)pOmObj)->clProvObjectValidate )
    {
        retCode = ( (CL_OM_PROV_CLASS *)pOmObj)->clProvObjectValidate(pOmObj, 0, &provTxnData, 1);
    }
    else if( ( (CL_OM_PROV_CLASS *)pOmObj)->clProvValidate )
    {
        retCode = ((CL_OM_PROV_CLASS *) pOmObj)->clProvValidate(pOmObj, 0, &provTxnData);
    }
    else
    {
        retCode = CL_OK;
    }

    if (retCode != CL_OK)
    {
        clLogError("PRV", "VAL", CL_PROV_LOG_1_CREATE_VALIDATE, retCode);

        /* Call the rollback callback */
        if( ( (CL_OM_PROV_CLASS * )pOmObj )->clProvObjectRollback )
        {
            retCode =  ( (CL_OM_PROV_CLASS *)pOmObj)->clProvObjectRollback( pOmObj, 0, &provTxnData, 1);
        }
        else if ( ( (CL_OM_PROV_CLASS *)pOmObj)->clProvRollback )
        {
            retCode = ( ( CL_OM_PROV_CLASS * ) pOmObj )->clProvRollback( pOmObj, 0, &provTxnData );
        }
        else
        {
            retCode = CL_OK;
        }

        if (retCode != CL_OK)
        {
            clLogError("PRV", "VAL", CL_PROV_LOG_1_CREATE_ROLLBACK, retCode);
        }

        /* Call the application's Object End callback function */
        if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd != NULL)
        {
            (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd(
                                                             pFullMoId, 0));
        }

        /* Remove the OM object */
        if ((rc = clOmObjectByReferenceDelete(pOmObj)) != CL_OK)
        {
            clLogError("OMC", "PRE", "Failed to delete the OM object. rc [0x%x]", rc);
        }

        /* Remove the MoId to OM Id mapping */
        if ((rc = clOmMoIdToOmIdMapRemove(pFullMoId)) != CL_OK)
        {
            clLogError("PRV", "PRE", "Failed to remove the MoId to OM Id mapping. rc [0x%x]", rc);
            CL_FUNC_EXIT();
            return rc;
        }

        CL_FUNC_EXIT();
        return (CL_OK);
    }

    /* Validate is successful, call the update callback function */
    if( ( (CL_OM_PROV_CLASS * )pOmObj)->clProvObjectUpdate )
    {
        retCode =  ( (CL_OM_PROV_CLASS *)pOmObj )->clProvObjectUpdate( pOmObj, 0, &provTxnData, 1);
    }
    else if ( ( (CL_OM_PROV_CLASS *)pOmObj)->clProvUpdate )
    {
        retCode = ( ( CL_OM_PROV_CLASS * ) pOmObj )->clProvUpdate( pOmObj, 0, &provTxnData );
    }
    else
    {
        retCode = CL_OK;
    }

    if (retCode != CL_OK)
    {
        clLogError("PRV", "PRE", CL_PROV_LOG_1_CREATE_UPDATE, rc);
    }

    /* Get the object handle */
    rc = clCorMoIdToObjectHandleGet(pFullMoId, &corObjHandle);
    if (rc != CL_OK)
    {
        clLogError("PRV", "PRE", "Failed to get Object Handle from MoId. rc [0x%x]", rc);
        return rc;
    }

    /* Do pre-provisioning for the attributes present in the object */
    if( ( (CL_OM_PROV_CLASS*)pOmObj)->clProvObjectValidate
        &&
        ( (CL_OM_PROV_CLASS *)pOmObj)->clProvObjectUpdate)
    {
        ClProvObjAttrArgsT provObjAttrArgs = {0};
        provObjAttrArgs.pMoId = pFullMoId;
        rc = clCorObjectAttributeWalk( corObjHandle, NULL, clProvObjAttrWalk, &provObjAttrArgs);
        if(rc == CL_OK && provObjAttrArgs.txnDataEntries > 0)
        {
            rc = ((CL_OM_PROV_CLASS*)pOmObj)->clProvObjectValidate(pOmObj, 0, 
                                                                   provObjAttrArgs.pProvTxnData,
                                                                   provObjAttrArgs.txnDataEntries);
            if(rc == CL_OK)
            {
                rc = ((CL_OM_PROV_CLASS*)pOmObj)->clProvObjectUpdate(pOmObj, 0,
                                                                     provObjAttrArgs.pProvTxnData,
                                                                     provObjAttrArgs.txnDataEntries);
            }
            else
            {
                if( ( (CL_OM_PROV_CLASS*)pOmObj)->clProvObjectRollback != NULL)
                {
                    rc =  ( (CL_OM_PROV_CLASS*)pOmObj)->clProvObjectRollback(pOmObj, 0,
                                                                             provObjAttrArgs.pProvTxnData,
                                                                             provObjAttrArgs.txnDataEntries);
                }
            }
        }
        clProvObjAttrFree(&provObjAttrArgs);
    }
    else
    {
        rc = clCorObjectAttributeWalk( corObjHandle, NULL, clProvAttrWalk, pFullMoId );
    }

    /* Calling Object-End callback function */
    if (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd != NULL)
    {
        (((CL_OM_PROV_CLASS *) pOmObj )->clProvObjectEnd(
                                                         pFullMoId, 0));
    }

    clCorObjectHandleFree(&corObjHandle);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**
 * This function is used to un-register for all the MOs for which the registration was happened
 * during initialization phase. It will un-register for all the resource entires of the resource
 * table (rt) xml file.
 */ 
ClRcT _clProvOIUnregister()
{
    ClRcT                   rc = CL_OK;
    ClCorAddrT              provAddr = {0};
    ClCorMOIdT              moId ;
    ClUint32T               index = 0;
    ClOampRtResourceArrayT  resourcesArray = {0};    

    rc = clMsoConfigResourceInfoGet( &provAddr, &resourcesArray );
    if (CL_OK != rc)
    {
        clLogError("PRV","URG", "Failed while getting the resource information. rc[0x%x] ", rc);
        return rc;
    }

    for (index = 0; index < resourcesArray.noOfResources ; index++)
    {
        clCorMoIdInitialize(&moId);

        rc = clCorMoIdNameToMoIdGet(&(resourcesArray.pResources[index].resourceName), &moId);
        if (CL_OK != rc)
        {
            clLogError("PRV", "URG", "Failed to get the MOID from the give moid name[%s]. rc[0x%x]", 
                    resourcesArray.pResources[index].resourceName.value, rc);
            continue;
        }

        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);

        rc = clCorOIUnregister(&moId, &provAddr);
        if (CL_OK != rc)
        {
            clLogError("PRV", "URG", 
                    "Failed while unregistering the route entry for resource [%s], rc[0x%x]", 
                    resourcesArray.pResources[index].resourceName.value, rc);
            continue;
        }
    }


    if (resourcesArray.noOfResources != 0)
        clHeapFree(resourcesArray.pResources);

    return rc;
}

ClRcT clProvResourcesGet(ClCorMOIdListT** ppMoIdList)
{
    if (!pProvMoIdList)
    {
        clLogDebug("PRV", "RESLISTGET", "No prov resources configured.");
        return CL_OK;
    }

    *ppMoIdList = clHeapAllocate(sizeof(ClCorMOIdListT) + pProvMoIdList->moIdCnt * sizeof(ClCorMOIdT));
    if (!(*ppMoIdList))
    {
        clLogError("PRV", "RESLISTGET", "Failed to allocate memory.");
        return CL_PROV_RC(CL_ERR_NO_MEMORY);
    }

    memcpy(*ppMoIdList, pProvMoIdList, sizeof(ClCorMOIdListT) + pProvMoIdList->moIdCnt * sizeof(ClCorMOIdT));

    return CL_OK;
}
