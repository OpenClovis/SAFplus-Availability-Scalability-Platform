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
 * ModuleName  : cor
 * File        : clCorClientInit.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the cor client lib initilization
 *                and finalization routines.
 *****************************************************************************/

/* INCLUDES */
#undef __SERVER__
#define __CLIENT__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clEoApi.h>
#include <clHandleApi.h>
#include <clBitApi.h>
#include <clCorApi.h>

/** Cor Internal Headers **/
#include <clCorTxnApi.h>
#include <clCorRMDWrap.h> 
#include <clCorClient.h>
#include "clCorTxnClientIpi.h"
#include "xdrClIocAddressIDLT.h"

#include <clCorServerFuncTable.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

/* GLOBALs */

extern ClRcT
clTxnCommitRmd (ClUint32T data,
        ClBufferHandleT  inMsgHandle, ClBufferHandleT  outMsgHandle);

ClVersionT gCorClientToServerVersionT[] = {{'B', 0x1, 0x1}};
ClVersionDatabaseT gCorClientToServerVersionDb = {1, gCorClientToServerVersionT};
ClHandleDatabaseHandleT gCorDbHandle = 0;
ClOsalMutexIdT   gCorBundleMutex = 0;

/* STATIC DEFINITIONS */

static ClCorMOIdPtrT  pwd;


/**
 *  Attach the COR component into the EO.
 *
 *  This API attaches the COR into each new EO when it is instantiated.
 *  It is called from the EO create callout.
 *                                                                        
 *  @param EOHandle : Handle to the EO Object
 *
 *  @returns CL_OK  - Success<br>
 *    On failure, error returned by #clEoClientInstall.
 */
ClRcT 
clCorClientInitialize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT* EOHandle = NULL;

    CL_FUNC_ENTER();

    rc= clEoMyEoObjectGet(&EOHandle);
    if (CL_OK != rc)
    {
        clLogError("COR", "INT", "Failed to get the Eo data. rc[0x%x]", rc);
        return rc;
    }

    rc = clEoClientTableRegister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, COR), CL_IOC_COR_PORT);
    if(CL_OK != rc)
    {
        clLogError("COR", "INT", "COR EO client table register failed with [%#x]", rc);
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "EOId passed = %p", (void *)EOHandle));

    if (EOHandle == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clCorClientInitialize: NULL EO Object Handle\n"));
        CL_FUNC_EXIT();
        return (CL_COR_ERR_NULL_PTR);
    }

    if ((rc = clCorMoIdAlloc (&pwd)) != CL_OK)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clCorClientInitialize: Could not allocate moID\n"));
        CL_FUNC_EXIT();
        return (rc);
    }
    
	if ((rc = clEoPrivateDataSet(EOHandle,
                          CL_EO_COR_SERVER_COOKIE_ID,
                          (void *)pwd)) != CL_OK)
	{
		CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clCorClientInitialize: Could not set default corPath\n"));
    	CL_FUNC_EXIT();
    	return (rc);
	}

    if ((rc = clCorTxnInfoStoreContainerCreate()) != CL_OK)
    {
        /* Unable to create the container in the client to store txn failed job information */
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "clCorClientInitialize: Could not create container to store the failed job information."));
        CL_FUNC_EXIT();
        return rc;
    }

    if ((rc = clCorCliTxnIdMapCntCreate()) != CL_OK)
    {
        clLogError("CLN", "INI", "Failed to create the container to store txnId name to txnId mapping. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    if ((rc = clCorCliTxnFailedJobLlistCreate()) != CL_OK)
    {
        clLogError("COR", "INI", 
                        "Failed to create the linked list to store the failed job node mapping. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clHandleDatabaseCreate(NULL, &gCorDbHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the handle database. rc[0x%x]", rc));
        return rc;
    }

    rc = clOsalMutexCreate(&gCorBundleMutex);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the mutex. rc[0x%x]", rc));
        return rc;
    }

    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  Detach the COR component into the EO.
 *
 *  This API detaches the COR from the EO when it is being deleted.
 *  It is called from the EO delete callout.
 *                                                                        
 *  @param EOHandle : Handle to the EO Object
 *
 *  @returns CL_OK  - Success<br>
 *    On failure, error returned by #EOClientInstall.
 */
ClRcT 
clCorClientFinalize()
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT* EOHandle;

    CL_FUNC_ENTER();
    
    rc = clEoMyEoObjectGet(&EOHandle);
    if (CL_OK != rc)
    {
        clLogError("COR", "FIN",
                "Failed while getting the Eo data. rc[0x%x]", rc);
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "EOId passed = %p", (void *)EOHandle));

    if (EOHandle == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ( "clCorClientFinalize: NULL EO Object Handle\n"));
        CL_FUNC_EXIT();
        return (CL_COR_ERR_NULL_PTR);
    }

    /* Free the  moId malloced during clCorClientInitialize */
    clCorMoIdFree(pwd);

    /* Delete the container created to store the txn failed job information */
    rc = clCorTxnInfoStoreContainerFinalize();
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorClientFinalize: Failed to finalize the container. rc [0x%x]", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Delete the container created to store the txn id name to txn id mapping */
    rc = clCorCliTxnIdMapCntFinalize();
    if (rc != CL_OK)
    {
        clLogError("CLI", "FIN", 
            "Failed to finalize the container created to store txn-id name to txn-id mapping. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    /* Delete the linked list created to store the txn job info node mapping */
    rc = clCorCliTxnFailedJobLlistFinalize();
    if (rc != CL_OK)
    {
        clLogError("COR", "INI",
            "Failed to finalize the linked list to store the txn job info node mapping. rc [0x%x]", rc);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = clOsalMutexDelete(gCorBundleMutex);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the mutex. rc[0x%x]", rc));
        return rc;
    }

    rc = clHandleDatabaseDestroy(gCorDbHandle);
    if(CL_OK != rc)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while deleting the database handle. rc[0x%x]", rc));

    clEoClientTableDeregister(CL_EO_CLIENT_SYM_MOD(gAspFuncTable, COR), CL_IOC_COR_PORT);

    CL_FUNC_EXIT();
    return rc;
}

