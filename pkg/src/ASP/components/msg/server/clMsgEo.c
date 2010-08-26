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
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


/*#include <arpa/inet.h>*/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocErrors.h>
#include <clRmdErrors.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>

#include <clMsgCommon.h>
#include <clMsgDatabase.h>
#include <clMsgDebugCli.h>
#include <clMsgEo.h>
#include <clMsgIdl.h>
#include <clMsgReceiver.h>
#include <clMsgQueue.h>
#include <clMsgFailover.h>

#include <msgCltClient.h>
#include <msgIdlClient.h>
#include <msgIdlServer.h>


ClCpmHandleT cpmHandle;

static ClRcT clMsgInitialize(ClUint32T argc, ClCharT *argv[]);
static ClRcT clMsgFinalize(void);

ClEoConfigT clEoConfig = {
    "MSG",                          /* EO Name */
    1,                              /* EO Thread Priority */
    10,                             /* No of EO thread needed */
    CL_IOC_MSG_PORT,                /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,      /* Whether to use main thread for eo Recv or not */
    clMsgInitialize,                /* Function CallBack to initialize the Application */
    NULL,                           /* Function Callback to Terminate the Application */
    NULL /*clMsgStateChange*/,      /* Function Callback to change the Application state */
    NULL /*clMsgHealthCheck*/,      /* Function Callback to check the Application health */
    NULL,
    /*.needSerialization = CL_TRUE */
};

/*
 * What basic and client libraries do we need to use? 
 */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,                    /* osal */
    CL_TRUE,                    /* timer */
    CL_TRUE,                    /* buffer */
    CL_TRUE,                    /* ioc */
    CL_TRUE,                    /* rmd */
    CL_TRUE,                    /* eo */
    CL_FALSE,                   /* om */
    CL_FALSE,                   /* hal */
    CL_TRUE,                    /* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_FALSE,                   /* cor */
    CL_FALSE,                   /* cm */
    CL_FALSE,                   /* name */
    CL_TRUE,                    /* log */
    CL_FALSE,                   /* trace */
    CL_FALSE,                   /* diag */
    CL_FALSE,                   /* txn */
    CL_FALSE,                   /* hpi */
    CL_FALSE,                   /* cli */
    CL_FALSE,                   /* alarm */
    CL_TRUE,                    /* debug */
    CL_FALSE,                   /* gms */
    CL_FALSE,                   /* pm */
};



ClBoolT gClMsgSrvInit = CL_FALSE;
static ClHandleT gMsgNotificationHandle;
ClInt32T gClMsgSvcRefCnt;
ClHandleDatabaseHandleT gMsgClientHandleDb;
ClIocNodeAddressT gClMyAspAddress;
ClOsalMutexT gClMsgFinalizeLock;
ClOsalCondT  gClMsgFinalizeCond;

static ClRcT clMsgTerminate(ClInvocationT invocation,
                            const ClNameT *compName)
{
    if(gClMsgSrvInit)
    {
        ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = 0};
        clOsalMutexLock(&gClMsgFinalizeLock);
        while(gClMsgSvcRefCnt > 0)
        {
            clOsalCondWait(&gClMsgFinalizeCond, &gClMsgFinalizeLock, timeout);
        }
        clMsgFinalize();
        clOsalMutexUnlock(&gClMsgFinalizeLock);
    }
    clCpmComponentUnregister(cpmHandle, compName, NULL);
    clCpmClientFinalize(cpmHandle);
    clCpmResponse(cpmHandle, invocation, CL_OK);
    
    return CL_OK;
}


static void clMsgRegisterWithCpm(void)
{
    ClRcT rc;
    ClNameT            appName = {0};
    ClCpmCallbacksT    callbacks = {0};
    ClVersionT  version = {0};

    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
                                                                                                                             
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = clMsgTerminate;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
    CL_ASSERT(rc == CL_OK);

    rc = clCpmComponentNameGet(cpmHandle, &appName);
    CL_ASSERT(rc == CL_OK);

    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
    CL_ASSERT(rc == CL_OK);
}


static ClRcT clMsgGetDatabasesAndUpdate(void)
{
    ClRcT rc;
    ClIocNodeAddressT masterAddr;
    ClUint8T *pData = NULL;
    ClUint32T size = 0;
    ClTimerTimeOutT delay = CL_MSG_DEFAULT_DELAY;
    ClUint32T retries = 0;

    rc = clCpmMasterAddressGet(&masterAddr);
    if(rc != CL_OK)
    {
        clLogError("UPD", "DBs", "Failed to get the CPM master node address. error code [0x%x].", rc);
        goto error_out;
    }

    if(masterAddr == gClMyAspAddress)
    {
        rc = CL_OK;
        goto out;
    }

    do {
        rc = clMsgGetDatabasesThroughIdl(masterAddr, &pData, &size);
    }while((rc == CL_MSG_RC(CL_ERR_NOT_INITIALIZED) ||
                rc == CL_EO_RC(CL_EO_ERR_FUNC_NOT_REGISTERED) ||
                rc == CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE) ||
                rc == CL_RMD_RC(CL_IOC_ERR_COMP_UNREACHABLE)) &&
            ++retries < CL_MSG_DEFAULT_RETRIES &&
            clOsalTaskDelay(delay) == CL_OK);
    if(rc != CL_OK)
    {
        clLogError("UPD", "DBs", "Failed to pull the data from [%d]. error code [0x%x].", masterAddr, rc);
        goto error_out;
    }

    rc = clMsgUpdateDatabases(pData, size);
    if(rc != CL_OK)
        clLogError("UPD", "DBs", "Failed to the message databases received from master [%d]. error code [0x%x].", masterAddr, rc);

    clHeapFree(pData);

error_out:
out:
    return rc;
}


static void clMsgNotificationReceiveCallback(ClIocNotificationIdT event, ClPtrT pArg, ClIocAddressT *pAddr)
{
    clOsalMutexLock(&gClMsgFinalizeLock);
    if(!gClMsgSrvInit)
    {
        /*
         * Msg server already finalized. skip it.
         */
        clOsalMutexUnlock(&gClMsgFinalizeLock);
        return;
    }
    ++gClMsgSvcRefCnt;
    clOsalMutexUnlock(&gClMsgFinalizeLock);

    if((event == CL_IOC_COMP_DEATH_NOTIFICATION && pAddr->iocPhyAddress.portId == CL_IOC_MSG_PORT) ||
            event == CL_IOC_NODE_LEAVE_NOTIFICATION)
    {
        clMsgNodeLeftCleanup(pAddr);
    }
    else if(event == CL_IOC_COMP_DEATH_NOTIFICATION)
    {
        clMsgCompLeftCleanup(pAddr);
    }

    clOsalMutexLock(&gClMsgFinalizeLock);
    --gClMsgSvcRefCnt;
    clOsalCondSignal(&gClMsgFinalizeCond);
    clOsalMutexUnlock(&gClMsgFinalizeLock);

    return;
}


static ClRcT clMsgInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClRcT rc;
    ClRcT retCode;
    ClIocPhysicalAddressT notificationForComp = { CL_IOC_BROADCAST_ADDRESS, 0};

    /* Checking if message service is already initialized */
    if(gClMsgSrvInit == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_INITIALIZED);
        clLogError("MSG", "INI", "The Message Service is already initialized. error code [0x%x].", rc);
        goto error_out;
    }

    gClMyAspAddress = clIocLocalAddressGet();

    /* Initializing a database to maintain client information */
    rc = clHandleDatabaseCreate((void (*)(void*))NULL, &gMsgClientHandleDb);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the handle database. error code [0x%x].", rc);
        goto error_out;
    }

    /* Initializing IDL for server to server and server to client communitcation. */
    rc = clMsgSrvIdlInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the IDL. error code [0x%x].", rc);
        goto error_out_2;
    }

    /* Initializing a database for maintaining queues. */
    rc = clMsgQueueInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the queue databases. error code [0x%x].", rc);
        goto error_out_3;
    }

    rc = clOsalMutexInit(&gClMsgFinalizeLock);

    CL_ASSERT(rc == CL_OK);

    rc = clOsalCondInit(&gClMsgFinalizeCond);

    CL_ASSERT(rc == CL_OK);

    /* Initializing the Group-Information */
    rc = clOsalMutexInit(&gClGroupDbLock);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the \"group db lock\". error code [0x%x].", rc);
        goto error_out_4;
    }

    rc = clMsgSenderDatabaseInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the \"sender database\". error code [0x%x].", rc);
        goto error_out_5;
    }

    rc = clMsgReceiverDatabaseInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize \"receiver database\". error code [0x%x].", rc);
        goto error_out_6;
    }

    rc = clCpmNotificationCallbackInstall(notificationForComp, &clMsgNotificationReceiveCallback, NULL, &gMsgNotificationHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install the notification callback function. error code [0x%x].", rc);
        goto error_out_7;
    }

    /* Initializing the IDL generated code. */
    rc = clMsgIdlClientInstall();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install Server Table. error code [0x%x].", rc);
        goto error_out_8;
    }

    rc = clMsgIdlClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register Server Table. error code [0x%x].", rc);
        goto error_out_9;
    }

    rc = clMsgCltClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register Client Table. error code [0x%x].", rc);
        goto error_out_10;
    }

    clMsgDebugCliRegister();

    clMsgRegisterWithCpm();

    rc = clMsgFinalizeBlockInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the msg-finalize blocker. error code [0x%x].", rc);
        goto error_out_11;
    }

    rc = clMsgGetDatabasesAndUpdate();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to Get Databases and update. error code [0x%x].", rc);
        goto error_out_12;
    }

    gClMsgSrvInit = CL_TRUE;

    goto out;

error_out_12:
    retCode = clMsgFinalizeBlockFin();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to cleanup the msg-finalize blocker. error code [0x%x].", retCode);
error_out_11:
    retCode = clMsgCltClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Client Table. error code [0x%x].", retCode);
error_out_10:
    retCode = clMsgIdlClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Server Table. error code [0x%x].", retCode);
error_out_9:
    retCode = clMsgIdlClientUninstall();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to Uninstall Server Table. error code [0x%x].", retCode);
error_out_8:
    retCode = clCpmNotificationCallbackUninstall(&gMsgNotificationHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to uninstall the notification callback . error code [0x%x].", retCode);
error_out_7:
    clMsgReceiverDatabaseFin();
error_out_6:
    clMsgSenderDatabaseFin();
error_out_5:
    retCode = clOsalMutexDestroy(&gClGroupDbLock);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the group db mutex. error code [0x%x].", retCode);
error_out_4:
    retCode = clMsgQueueFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize the queue databases. error code [0x%x].", retCode);
error_out_3:
    retCode = clMsgSrvIdlFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize IDL. error code [0x%x].", retCode);
error_out_2:
    retCode = clHandleDatabaseDestroy(gMsgClientHandleDb);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the handle database. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}


static ClRcT clMsgFinalize(void)
{
    ClRcT rc = CL_OK;

    CL_MSG_SERVER_INIT_CHECK;

    clMsgFinalizeBlocker();

    rc = clMsgFinalizeBlockFin();
    if(rc != CL_OK)
        clLogError("MSG", "INI", "Failed to cleanup the msg-finalize blocker. error code [0x%x].", rc);

    clMsgDebugCliDeregister();
    
    rc = clMsgCltClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister the Server Table. error code [0x%x].", rc);

    rc = clMsgIdlClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Server Table. error code [0x%x].", rc);

    clMsgIdlClientUninstall();

    rc = clCpmNotificationCallbackUninstall(&gMsgNotificationHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to uninstall the notification callback function. error code [0x%x].", rc);

    clMsgReceiverDatabaseFin();

    clMsgSenderDatabaseFin();

    rc = clOsalMutexDestroy(&gClGroupDbLock);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the group db lock. error code [0x%x].", rc);

    rc = clMsgQueueFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize queue databases. error code [0x%x].", rc);

    rc = clMsgSrvIdlFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize IDL. error code [0x%x].", rc); 

    rc = clHandleDatabaseDestroy(gMsgClientHandleDb);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the client handle database. error code [0x%x].", rc);


    gClMsgSrvInit = CL_FALSE;
    goto out;

out:
    return rc;
}

