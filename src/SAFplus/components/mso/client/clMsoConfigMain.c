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
 * ModuleName  : mso 
 * File        : clMsoConfigMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the Mso configuration library implementation. 
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>

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
#include <clCorErrors.h>
#include <clProvApi.h>

#include <clMsoConfig.h>
#include "clMsoConfigUtils.h"

#ifdef MORE_CODE_COVERAGE
#include "clCodeCoveApi.h"
#endif

/**
 * Application callback functions for Transaction.
 */
extern ClProvTxnCallbacksT clProvTxnCallbacks;

/**
 * Mso configuration Library Initalize flag.
 */
ClUint32T       gMsoConfigInit   = CL_FALSE;
ClOsalMutexIdT  gClMsoConfigMutex;
ClCntHandleT gMsoConfigHandle = 0;
ClTxnAgentServiceHandleT gMsoConfigWriteTxnHandle = 0;
ClTxnAgentServiceHandleT gMsoConfigReadTxnHandle = 0;

static ClTxnAgentCallbacksT gMsoConfigWriteTxnCallbacks   =
{     
    .fpTxnAgentStart = clMsoConfigTxnStart,
    .fpTxnAgentJobPrepare = clMsoConfigTxnValidate,
    .fpTxnAgentJobCommit = clMsoConfigTxnUpdate,
    .fpTxnAgentJobRollback = clMsoConfigTxnRollback,
    .fpTxnAgentStop = clMsoConfigTxnEnd
};

static ClTxnAgentCallbacksT gMsoConfigReadTxnCallbacks   =
{       
    .fpTxnAgentStart = clMsoConfigTxnStart,
    .fpTxnAgentJobPrepare = NULL,
    .fpTxnAgentJobCommit = clMsoConfigTxnRead,
    .fpTxnAgentJobRollback = NULL,
    .fpTxnAgentStop = clMsoConfigTxnEnd
};

ClRcT
clMsoConfigTxnRegistration()
{
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();

    /* Registration for modify operation  */
    rc = clTxnAgentServiceRegister( CL_COR_TXN_SERVICE_ID_WRITE, gMsoConfigWriteTxnCallbacks,
            &gMsoConfigWriteTxnHandle);
    if(CL_OK != rc)
    {    
        clLogError("MSOCONFIG", "INI", "Failed while registering txn agent write callbacks. rc [0x%x]", rc);
        return rc;
    }    

    /* Registration for read operation  */
    rc = clTxnAgentServiceRegister( CL_COR_TXN_SERVICE_ID_READ, gMsoConfigReadTxnCallbacks,
            &gMsoConfigReadTxnHandle);
    if(CL_OK != rc)
    {    
        clLogError("MSOCONFIG", "INI", "Failed while registering txn agent read callbacks. rc [0x%x]", rc);
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK; 
}

ClRcT
clMsoConfigTxnUnRegister()
{
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();

    /* Registration for modify operation  */
    rc = clTxnAgentServiceUnRegister(gMsoConfigWriteTxnHandle);
    if(CL_OK != rc)
    {    
        clLogError("MSOCONFIG", "INI", "Failed while unregistering txn agent write callbacks. rc [0x%x]", rc);
        return rc;
    }    

    /* Registration for read operation  */
    rc = clTxnAgentServiceUnRegister(gMsoConfigReadTxnHandle);
    if(CL_OK != rc)
    {    
        clLogError("MSOCONFIG", "INI", "Failed while unregistering txn agent read callbacks. rc [0x%x]", rc);
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT clMsoConfigTxnStart(
                ClTxnTransactionHandleT txnHandle,
                ClTxnAgentCookieT*      pCookie)
{
    if (clProvTxnCallbacks.fpProvTxnStart != NULL)
    {
        /* Call the user supplied callback function */
        (void) clProvTxnCallbacks.fpProvTxnStart(txnHandle);
    }
    else
    {
        clLogInfo("MSOCONFIG", "TXN", "User has not specified the Txn-Start callback function.");
    }    

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clMsoConfigTxnEnd(
                ClTxnTransactionHandleT txnHandle,
                ClTxnAgentCookieT*      pCookie)
{
    if (clProvTxnCallbacks.fpProvTxnEnd != NULL)
    {
        /* Call the user supplied callback function */
        (void) clProvTxnCallbacks.fpProvTxnEnd(txnHandle);
    }
    else
    {
        clLogInfo("MSOCONFIG", "TXN", "User has not specified the Txn-End callback function.");
    }    

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT        
clMsoConfigTxnValidate( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie )
{
    ClCorTxnIdT             corTxnId    = 0;
    ClCorMOIdT              moId = {{{0}}};
    ClRcT                   rc = CL_OK;
    ClMsoConfigCallbacksT*  pMsoConfigCB = NULL;
    ClCorServiceIdT         svcId = -1;
    ClCntNodeHandleT        nodeHandle = 0;

    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
    if(rc != CL_OK)
    {
        clLogError("MSOCONFIG", "VALIDATE", "Failed while getting cor-txn Id. rc[0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "VALIDATE", "Failed to get the MoId from Transaction Job. rc [0x%x]", rc);
        return rc;
    }

    svcId = clCorMoIdServiceGet(&moId);

    rc = clCntNodeFind(gMsoConfigHandle, (ClCntKeyHandleT) (ClWordT) svcId, &nodeHandle);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "VALIDATE", "Service for Id [%d] is not registered. rc [0x%x]", svcId, rc);
        return rc;
    }

    rc = clCntNodeUserDataGet(gMsoConfigHandle, nodeHandle, (ClCntDataHandleT *) &pMsoConfigCB);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "VALIDATE", "Failed to get the callbacks for svcId [%d], rc [0x%x]", svcId, rc);
        return rc;
    }

    /* Invoke the callback registered by prov. */
    if (pMsoConfigCB->fpMsoJobPrepare)
    {
        rc = pMsoConfigCB->fpMsoJobPrepare(txnHandle, jobDefn, jobDefnSize, corTxnId);
        if (rc != CL_OK)
        {
            clCorTxnIdTxnFree(corTxnId);
            clLogError("MSOCONFIG", "VALIDATE", "Mso service callback failed. rc [0x%x]", rc);
            return rc;
        }
    }
    else
    {
        clLogInfo("MSOCONFIG", "VALIDATE", "TXN Validate callback not registered for svcId [%d].", svcId);
    }

    clCorTxnIdTxnFree(corTxnId);

    return CL_OK;
}

ClRcT        
clMsoConfigTxnUpdate(   CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie )
{
    ClCorTxnIdT             corTxnId    = 0;
    ClCorMOIdT              moId = {{{0}}};
    ClRcT                   rc = CL_OK;
    ClMsoConfigCallbacksT*  pMsoConfigCB = NULL;
    ClCorServiceIdT         svcId = -1;
    ClCntNodeHandleT        nodeHandle = 0;

    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
    if(rc != CL_OK)
    {
        clLogError("MSOCONFIG", "UPDATE", "Failed while getting cor-txn Id. rc[0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "UPDATE", "Failed to get the MoId from Transaction Job. rc [0x%x]", rc);
        clCorTxnIdTxnFree(corTxnId);
        return rc;
    }

    svcId = clCorMoIdServiceGet(&moId);

    rc = clCntNodeFind(gMsoConfigHandle, (ClCntKeyHandleT) (ClWordT) svcId, &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "UPDATE", "Service for Id [%d] is not registered. rc [0x%x]", svcId, rc);
        clCorTxnIdTxnFree(corTxnId);
        return rc;
    }

    rc = clCntNodeUserDataGet(gMsoConfigHandle, nodeHandle, (ClCntDataHandleT *) &pMsoConfigCB);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "UPDATE", "Failed to get the callbacks for svcId [%d], rc [0x%x]", svcId, rc);
        clCorTxnIdTxnFree(corTxnId);
        return rc;
    }

    /* Invoke the callback registered by prov. */
    if (pMsoConfigCB->fpMsoJobCommit)
    {
        rc = pMsoConfigCB->fpMsoJobCommit(txnHandle, jobDefn, jobDefnSize, corTxnId);
        if (rc != CL_OK)
        {
            clCorTxnIdTxnFree(corTxnId);
            clLogError("MSOCONFIG", "UPDATE", "Mso service callback failed. rc [0x%x]", rc);
            return rc;
        }
    }
    else
    {
        clLogInfo("MSOCONFIG", "UPDATE", "TXN Update callback not registered for svcId [%d].", svcId);
    }

    clCorTxnIdTxnFree(corTxnId);

    return CL_OK;
}

ClRcT        
clMsoConfigTxnRollback( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie )
{
    ClCorTxnIdT             corTxnId    = 0;
    ClCorMOIdT              moId = {{{0}}};
    ClRcT                   rc = CL_OK;
    ClMsoConfigCallbacksT*  pMsoConfigCB = NULL;
    ClCorServiceIdT         svcId = -1;
    ClCntNodeHandleT        nodeHandle = 0;

    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
    if(rc != CL_OK)
    {
        clLogError("MSOCONFIG", "ROLLBACK", "Failed while getting cor-txn Id. rc[0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "ROLLBACK", "Failed to get the MoId from Transaction Job. rc [0x%x]", rc);
        return rc;
    }

    svcId = clCorMoIdServiceGet(&moId);

    rc = clCntNodeFind(gMsoConfigHandle, (ClCntKeyHandleT) (ClWordT) svcId, &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "ROLLBACK", "Service for Id [%d] is not registered. rc [0x%x]", svcId, rc);
        clCorTxnIdTxnFree(corTxnId);
        return rc;
    }

    rc = clCntNodeUserDataGet(gMsoConfigHandle, nodeHandle, (ClCntDataHandleT *) &pMsoConfigCB);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "ROLLBACK", "Failed to get the callbacks for svcId [%d], rc [0x%x]", svcId, rc);
        return rc;
    }

    /* Invoke the callback registered by prov. */
    if (pMsoConfigCB->fpMsoJobRollback)
    {
        rc = pMsoConfigCB->fpMsoJobRollback(txnHandle, jobDefn, jobDefnSize, corTxnId);
        if (rc != CL_OK)
        {
            clCorTxnIdTxnFree(corTxnId);
            clLogError("MSOCONFIG", "ROLLBACK", "Mso service callback failed. rc [0x%x]", rc);
            return rc;
        }
    }
    else
    {
        clLogInfo("MSOCONFIG", "ROLLBACK", "TXN Rollback callback is not registered for svcId [%d].", svcId);
    }

    clCorTxnIdTxnFree(corTxnId);

    return CL_OK;
}

ClRcT        
clMsoConfigTxnRead( CL_IN       ClTxnTransactionHandleT txnHandle,
                        CL_IN       ClTxnJobDefnHandleT     jobDefn,
                        CL_IN       ClUint32T               jobDefnSize,
                        CL_INOUT    ClTxnAgentCookieT* pCookie )
{
    ClCorTxnIdT             corTxnId    = 0;
    ClCorMOIdT              moId = {{{0}}};
    ClRcT                   rc = CL_OK;
    ClMsoConfigCallbacksT*  pMsoConfigCB = NULL;
    ClCorServiceIdT         svcId = -1;
    ClCntNodeHandleT        nodeHandle = 0;

    rc = clCorTxnJobHandleToCorTxnIdGet(jobDefn, jobDefnSize, &corTxnId);
    if(rc != CL_OK)
    {
        clLogError("MSOCONFIG", "READ", "Failed while getting cor-txn Id. rc[0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "READ", "Failed to get the MoId from Transaction Job. rc [0x%x]", rc);
        return rc;
    }

    svcId = clCorMoIdServiceGet(&moId);

    rc = clCntNodeFind(gMsoConfigHandle, (ClCntKeyHandleT) (ClWordT) svcId, &nodeHandle);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "READ", "Service for Id [%d] is not registered. rc [0x%x]", svcId, rc);
        return rc;
    }

    rc = clCntNodeUserDataGet(gMsoConfigHandle, nodeHandle, (ClCntDataHandleT *) &pMsoConfigCB);
    if (rc != CL_OK)
    {
        clCorTxnIdTxnFree(corTxnId);
        clLogError("MSOCONFIG", "READ", "Failed to get the callbacks for svcId [%d], rc [0x%x]", svcId, rc);
        return rc;
    }

    /* Invoke the callback registered by prov. */
    if (pMsoConfigCB->fpMsoJobRead)
    {
        rc = pMsoConfigCB->fpMsoJobRead(txnHandle, jobDefn, jobDefnSize, corTxnId);
        if (rc != CL_OK)
        {
            clCorTxnIdTxnFree(corTxnId);
            clLogError("MSOCONFIG", "READ", "Mso service callback failed. rc [0x%x]", rc);
            return rc;
        }
    }
    else
    {
        clLogInfo("MSOCONFIG", "READ", "TXN Read callback is not registered for svcId [%d].", svcId);
    }

    clCorTxnIdTxnFree(corTxnId);

    return CL_OK;
}

ClRcT clMsoLibInitialize()
{
    ClRcT rc = CL_OK;
    /*
     * Check for delayed initialization of OM since the EO
     * isnt created during basic lib initialization phase of OM.
     * thereby preventing RMDs in that phase.
     */
    if(gClOIConfig.oiDBReload)
    {
        rc = clOmLibInitializeExtended();
        if(rc != CL_OK)
        {
            clLogError("MSOCONFIG", "INI", "Initializing OM with OI config reload enabled returned [%#x]", rc);
            return rc;
        }
    }
            
    rc = clMsoConfigTxnRegistration();
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "INI", "Failed to register the Txn agent callbacks. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntHashtblCreate(CL_MSO_NUM_BUCKETS, 
            _clMsoDataCmpFunc,
            _clMsoDataHashFunc,
            _clMsoDataDeleteFunc,
            _clMsoDataDeleteFunc,
            CL_CNT_UNIQUE_KEY,
            &gMsoConfigHandle);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "INI", "Failed to create mso config hash table. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT clMsoLibFinalize()
{
    ClRcT rc = CL_OK;

    rc = clMsoConfigTxnUnRegister();
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "FINALIZE", "Failed to unregister txn-callbacks. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntDelete(gMsoConfigHandle);
    if (rc != CL_OK)
    {
        clLogError("MSOCONFIG", "FINALIZE", "Failed to delete mso config data table. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT clMsoConfigRegister(ClCorServiceIdT svcId, ClMsoConfigCallbacksT msoCallbacks)
{
    ClRcT rc = CL_OK;
    ClMsoConfigCallbacksT* pMsoCallbacks = NULL;

    /* Store this information into hash-table */
    pMsoCallbacks = clHeapAllocate(sizeof(ClMsoConfigCallbacksT));
    if (!pMsoCallbacks)
    {
        clLogError("MSOCONFIG", "CBREG", "Failed to allocate memory");
        return CL_MSO_RC(CL_ERR_NO_MEMORY);
    }

    memcpy(pMsoCallbacks, &msoCallbacks, sizeof(ClMsoConfigCallbacksT));

    rc = clCntNodeAdd(gMsoConfigHandle, (ClCntKeyHandleT) (ClWordT) svcId, (ClCntDataHandleT) pMsoCallbacks, NULL);
    if (rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE)
    {
        clLogError("MSOCONFIG", "CBREG", "Failed to add mso config data for svcId [%d]. rc [0x%x]", svcId, rc);
        return rc;
    }

    return CL_OK;
}

ClInt32T _clMsoDataCmpFunc(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT) key1 - (ClWordT) key2);
}

ClUint32T _clMsoDataHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT) key & (CL_MSO_NUM_BUCKETS - 1));
}

void _clMsoDataDeleteFunc(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    clHeapFree((void *) data);
}
