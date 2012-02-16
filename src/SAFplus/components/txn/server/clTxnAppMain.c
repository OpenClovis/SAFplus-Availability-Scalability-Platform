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
 * File        : clTxnAppMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


/*******************************************************************************
 * Description :                                                                
 *******************************************************************************/


#include <stdio.h>

#include <clCommon.h>
#include <clDebugApi.h>

#include <clOsalApi.h>
#include <clCpmApi.h>
#include <clIocServices.h>
#include <clEoConfigApi.h>
#include <clCkptExtApi.h>

#include <clTxnLog.h>

#include "clTxnServiceIpi.h"
#include "clTxnDebugIpi.h"

extern ClUint32T  remoteId;
extern ClUint32T  myId;

ClCpmHandleT		gCpmHandle;

/* Forward Declarations */

ClRcT clTxnAppTerminate(ClInvocationT invocation,
			const ClNameT  *compName)
{
    ClEoExecutionObjT   *pEoObj;
    ClRcT rc = CL_OK;
    CL_FUNC_ENTER();

    /* Do the App Finalization */
    rc = clEoMyEoObjectGet(&pEoObj);
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to get EO Object rc:0x%x\n", rc));


    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Calling finalize/un-register functions\n"));
    clTxnDebugUnregister(pEoObj);
    clTxnServiceFinalize(0, CL_EO_NATIVE_COMPONENT_TABLE_ID);


    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Unregister with CPM before Exit ................. %s\n", 
                        compName->value));
    rc = clCpmComponentUnregister(gCpmHandle, compName, NULL);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Finalize before Exit ................. %s\n", 
                        compName->value));
    rc = clCpmClientFinalize(gCpmHandle);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("After Finalize ................. %s\n", 
                         compName->value));
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, NULL, CL_LOG_MESSAGE_1_SERVICE_STOPPED, "Transaction");

    clCpmResponse(gCpmHandle, invocation, CL_OK);
    CL_TXN_RETURN_RC(rc, ("Failed to properly terminate application rc:0x%x\n", rc));
}


ClRcT clTxnAppInitialize(ClUint32T argc,  ClCharT *argv[])
{
    ClNameT             appName;
    ClCpmCallbacksT     callbacks;
    ClVersionT          version;
    ClIocPortT          iocPort;
    ClEoExecutionObjT   *pEoObj;
    ClTxnServiceHandleT txnServiceHandle;
    ClRcT               rc          = CL_OK;

    CL_FUNC_ENTER();

    /* Do the App intialization */
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("clTxnAppMain::clTxnAppInitialize() - Init Server\n"));
    rc = clEoMyEoObjectGet(&pEoObj);
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to get EO Object 0x%x\n", rc));


    /* Initialize transaction-service */
    rc = clTxnServiceInitialize(pEoObj,
                                CL_EO_NATIVE_COMPONENT_TABLE_ID, 
                                &txnServiceHandle);
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to initialize transaction-service 0x%x\n", rc));

    /*  Do the CPM client init/Register */
    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = clTxnAppTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;
            
    rc = clEoMyEoIocPortGet(&iocPort);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Application Address 0x%x Port %x\n", 
                         clIocLocalAddressGet(), iocPort));
    rc = clCpmClientInitialize(&gCpmHandle, &callbacks, &version);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("After clCpmClientInitialize %#llX\t %x\n", gCpmHandle, rc));
    rc = clCpmComponentNameGet(gCpmHandle, &appName);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("After clCpmComponentNameGet %#llX\t %s\n", 
                         gCpmHandle, appName.value));
    rc = clCpmComponentRegister(gCpmHandle, &appName, NULL);
    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("After clCpmClientRegister %x\n", rc));

    rc = clTxnDebugRegister(pEoObj, appName);
    CL_TXN_ERR_RET_ON_ERROR(rc, ("Failed to initialize debug-service rc:0x%x\n", rc));

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_NOTICE, NULL, CL_LOG_MESSAGE_1_SERVICE_STARTED, "Transaction");

#if 0
    clDebugCli("TXN");
#endif
    
/* Block and use the main thread if required other wise return */
    CL_TXN_RETURN_RC(rc, ("Failed to initialize transaction-service rc:0x%x\n", rc));
}

/**
 * Callback for EO finalize
 */
ClRcT clTxnAppFinalize()
{
    return CL_OK;
}


ClRcT 	clTxnAppStateChange(ClEoStateT eoState)
{
    CL_FUNC_ENTER();
    /* Application state change */

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT   clTxnAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    CL_FUNC_ENTER();
    /* Modify following as per App requirement*/
    schFeedback->freq   = CL_EO_DEFAULT_POLL; 
    schFeedback->status = CL_CPM_EO_ALIVE;

    CL_FUNC_EXIT();
    return CL_OK;
}

ClEoConfigT clEoConfig = {
    "TXN",                      /* EO Name*/
    CL_OSAL_THREAD_PRI_MEDIUM,  /* EO Thread Priority */
    4,                          /* No of EO thread needed */
    CL_IOC_TXN_PORT,            /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START, 
    CL_EO_USE_THREAD_FOR_RECV,  /* Whether to use main thread for eo Recv or not */
    clTxnAppInitialize,         /* Function CallBack  to initialize the Application */
    clTxnAppFinalize,           /* Function Callback to Terminate the Application */ 
    clTxnAppStateChange,        /* Function Callback to change the Application state */
    clTxnAppHealthCheck,        /* Function Callback to change the Application state */
    NULL
};

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,			/* osal */
    CL_TRUE,			/* timer */
    CL_TRUE,			/* buffer */
    CL_TRUE,			/* ioc */
    CL_TRUE,			/* rmd */
    CL_TRUE,			/* eo */
    CL_FALSE,			/* om */
    CL_FALSE,			/* hal */
    CL_TRUE,			/* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_TRUE,			/* cor */
    CL_FALSE,			/* cm */
    CL_FALSE,			/* name */
    CL_TRUE,			/* log */
    CL_FALSE,			/* trace */
    CL_FALSE,			/* diag */
    CL_TRUE,			/* txn */
    CL_FALSE,			/* hpi */
    CL_FALSE,			/* cli */
    CL_FALSE,			/* alarm */
    CL_TRUE,			/* debug */
    CL_FALSE,			/* gms */
    CL_FALSE,			/* pm */
};




