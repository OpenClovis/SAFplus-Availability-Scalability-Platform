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
 * ModuleName  : message
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


/*#include <arpa/inet.h>*/
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocErrors.h>
#include <clRmdErrors.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>

#include <clMsgCommon.h>
#include <clMsgDebugCli.h>
#include <clMsgEo.h>
#include <clMsgIdl.h>
#include <clMsgQueue.h>
#include <clMsgReceiver.h>
#include <clMsgCkptServer.h>
#include <clMsgGroupDatabase.h>
#include <clMsgFailover.h>

#include <msgCltClient.h>
#include <msgIdlClient.h>
#include <msgIdlServer.h>
#include <msgCltSrvServer.h>
#include <msgCltSrvClient.h>

/* POSIX Includes */
#include <assert.h>
#include <errno.h>

/* Basic SAFplus Includes */
#include <clCommon.h>

/* SAFplus Client Includes */
#include <clLogApi.h>
#include <clCpmApi.h>
#include <saAmf.h>


/* Global Declarations */
SaAmfHandleT amfHandle;

ClBoolT unblockNow = CL_FALSE;

SaNameT      appName = {0};

int errorExit(SaAisErrorT rc)
{        
    exit(-1);
    return -1;
}

/* Function Declarations */
static ClRcT initializeAmf(void);
static ClHandleT gMsgNotificationHandle;
ClInt32T gClMsgSvcRefCnt;
ClHandleDatabaseHandleT gMsgClientHandleDb;
ClOsalMutexT gClMsgFinalizeLock;
ClOsalCondT  gClMsgFinalizeCond;
static void safTerminate(SaInvocationT invocation, const SaNameT *compName);
static void clMsgRegisterWithCpm(void);
static void clMsgNotificationReceiveCallback(ClIocNotificationIdT event, ClPtrT pArg, ClIocAddressT *pAddr);
static void *clMsgCachedCkptInitAsync(void *pParam);
static int safMsgFinalize(ClBoolT *pLockStatus);
static void dispatchLoop(void);

ClEoConfigT clEoConfig = {
    (ClOsalThreadPriorityT) 1,                              /* EO Thread Priority */
    10,                             /* No of EO thread needed */
    CL_IOC_MSG_PORT,                /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,      /* Whether to use main thread for eo Recv or not */
    NULL,                           /* Function CallBack to initialize the Application */
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


ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

    /* Connect to the SAF cluster */

    rc = initializeAmf();
    if(rc != CL_OK)
    {
       clLogCritical(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                     "MSG: msgInitialize failed [0x%X]\n\r", rc);
       return rc;
    }

    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();

    /* Now finalize my connection with the SAF cluster */

   saAmfFinalize(amfHandle);

   return 0;
}

static ClRcT initializeAmf(void)
{
    ClRcT         rc = CL_OK; 
    ClRcT         retCode;
    ClIocPhysicalAddressT notificationForComp = { CL_IOC_BROADCAST_ADDRESS, 0};
    
    clLogCompName = (ClCharT*) "MSG"; /* Override generated eo name with a short name for our server */
    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);
  
    clMsgRegisterWithCpm();
     
    if(gClMsgInit == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_INITIALIZED);
        clLogError("MSG", "INI", "The Message Service is already initialized. error code [0x%x].", rc);
        goto error_out;
    }
    
    gLocalAddress = clIocLocalAddressGet();
    gLocalPortId = CL_IOC_MSG_PORT;

    /* Initializing a database to maintain client information */
    rc = clHandleDatabaseCreate((void (*)(void*))NULL, &gMsgClientHandleDb);
     
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the handle database. error code [0x%x].", rc);
        goto error_out;
    }
    
    /* Initializing IDL for server to server and server to client communication. */
    rc = clMsgCommIdlInitialize();
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
   
    rc = clMsgReceiverDatabaseInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize \"receiver database\". error code [0x%x].", rc);
        goto error_out_4;
    }

    rc = clOsalMutexInit(&gClMsgFinalizeLock);
    CL_ASSERT(rc == CL_OK);
    
    rc = clOsalCondInit(&gClMsgFinalizeCond);
    CL_ASSERT(rc == CL_OK);

    /* Initializing the Group-Information */
    rc = clOsalMutexInit(&gClGroupDbLock);
    CL_ASSERT(rc == CL_OK);

    rc = clCpmNotificationCallbackInstall(notificationForComp, &clMsgNotificationReceiveCallback, NULL, &gMsgNotificationHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install the notification callback function. error code [0x%x].", rc);
        goto error_out_5;
    }
   
    /* Initializing the IDL generated code. */
    rc = clMsgIdlClientInstall();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install Server Table. error code [0x%x].", rc);
        goto error_out_6;
    }

    rc = clMsgIdlClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register Server Table. error code [0x%x].", rc);
        goto error_out_7;
    }

    rc = clMsgCltClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register Client Table. error code [0x%x].", rc);
        goto error_out_8;
    }

    rc = clMsgCltSrvClientInstall();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to install Client Server Table. error code [0x%x].", rc);
        goto error_out_9;
    }
  
    rc = clMsgCltSrvClientTableRegister(CL_IOC_MSG_PORT);
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to register Client Server Table. error code [0x%x].", rc);
        goto error_out_10;
    }

    rc = clMsgFinalizeBlockInit();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize the msg-finalize blocker. error code [0x%x].", rc);
        goto error_out_11;
    }
   
    clMsgDebugCliRegister();

    rc = clOsalTaskCreateDetached("MsgCkptInitAsync", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0,
                                 clMsgCachedCkptInitAsync, NULL);
    CL_ASSERT(rc == CL_OK);

    goto out;
error_out_11:
    retCode = clMsgCltSrvClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Client Table. error code [0x%x].", retCode);
error_out_10:
    retCode = clMsgCltSrvClientUninstall();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
error_out_9:
    retCode = clMsgCltClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Client Table. error code [0x%x].", retCode);
error_out_8:
    retCode = clMsgIdlClientTableDeregister();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to deregister Server Table. error code [0x%x].", retCode);
error_out_7:
    retCode = clMsgIdlClientUninstall();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the just opened handle database. error code [0x%x].", retCode);
error_out_6:
    retCode = clCpmNotificationCallbackUninstall(&gMsgNotificationHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to uninstall the notification callback function. error code [0x%x].", retCode);
error_out_5:
    retCode = clOsalMutexDestroy(&gClGroupDbLock);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the group db mutex. error code [0x%x].", retCode);

    retCode = clOsalCondDestroy(&gClMsgFinalizeCond);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the finalization condition. error code [0x%x].", retCode);

    retCode = clOsalMutexDestroy(&gClMsgFinalizeLock);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the finalization mutex. error code [0x%x].", retCode);

    clMsgReceiverDatabaseFin();

error_out_4:
    retCode = clMsgQueueFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to finalize the queue databases. error code [0x%x].", retCode);
error_out_3:
    clMsgCommIdlFinalize();
error_out_2:
    retCode = clHandleDatabaseDestroy(gMsgClientHandleDb);
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the handle database. error code [0x%x].", retCode);
error_out:
out:
    return rc;  
    
}//end of intializeAmf

static void safTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    SaAisErrorT rc = SA_AIS_OK;
    if(gClMsgInit)
    {
        ClBoolT lockStatus = CL_TRUE;
        ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = 0};
        clOsalMutexLock(&gClMsgFinalizeLock);
        while(gClMsgSvcRefCnt > 0)
        {
            clOsalCondWait(&gClMsgFinalizeCond, &gClMsgFinalizeLock, timeout);
        }
        safMsgFinalize(&lockStatus);
        if(lockStatus)
        {
            clOsalMutexUnlock(&gClMsgFinalizeLock);
        }
    }
    rc = saAmfComponentUnregister(amfHandle, compName, NULL);
    clCpmClientFinalize(amfHandle);
    //clCpmResponse(cpmHandle, invocation, CL_OK);
    saAmfResponse(amfHandle, invocation, SA_AIS_OK);
    
    return;
}


static void clMsgRegisterWithCpm(void)
{
    SaAisErrorT rc = SA_AIS_OK;
    SaAmfCallbacksT    callbacks = {0};
    SaVersionT  version = {0};

    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
                                                                                                                             
    callbacks.saAmfHealthcheckCallback = NULL;
    callbacks.saAmfComponentTerminateCallback = safTerminate;
    callbacks.saAmfCSISetCallback = NULL;
    callbacks.saAmfCSIRemoveCallback = NULL;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
    

    rc = saAmfInitialize(&amfHandle, &callbacks, &version);
    if( rc != SA_AIS_OK)
    {
         clLogError("MSG", "INI", "saAmfInitialize failed with  error code [0x%x].", rc);
         return ;
    }


    rc = saAmfComponentNameGet(amfHandle, &appName);

    rc = saAmfComponentRegister(amfHandle, &appName, NULL);
}


static void clMsgNotificationReceiveCallback(ClIocNotificationIdT event, ClPtrT pArg, ClIocAddressT *pAddr)
{
    
    clOsalMutexLock(&gClMsgFinalizeLock);
    if(!gClMsgInit)
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
       event == CL_IOC_NODE_LEAVE_NOTIFICATION || 
       event == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
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

static void *clMsgCachedCkptInitAsync(void *pParam)
{
    ClRcT  rc = CL_OK;
    ClRcT retCode;
    /* Initialize cached ckpt for MSG queue & MSG queue group */
    rc = clMsgQCkptInitialize();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to initialize cached checkpoints. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clMsgQCkptSynch();
    if(rc != CL_OK)
    {
        clLogError("MSG", "INI", "Failed to synchronize cached checkpoints. error code [0x%x].", rc);
        goto error_out_1;
    }

    gClMsgInit = CL_TRUE;

    return NULL;
error_out_1:
    retCode = clMsgQCkptFinalize();
    if(retCode != CL_OK)
        clLogError("MSG", "INI", "clMsgQCkptFinalize(): error code [0x%x].", retCode);
error_out:
    return NULL;
}


/*
 * Called with the msg finalize lock held.
 */
static int safMsgFinalize(ClBoolT *pLockStatus)
{
    ClRcT  rc = CL_OK;
    if(pLockStatus && !*pLockStatus) 
        return CL_MSG_RC(CL_ERR_INVALID_STATE);

    CL_MSG_INIT_CHECK(rc);
    if( rc != CL_OK)
    {
         return rc;
    }    
   
    gClMsgInit = CL_FALSE;

    if(pLockStatus)
    {
        *pLockStatus = CL_FALSE;
        clOsalMutexUnlock(&gClMsgFinalizeLock);
    }

    clMsgFinalizeBlocker();

    rc = clMsgFinalizeBlockFin();
    if(rc != CL_OK)
        clLogError("MSG", "INI", "Failed to cleanup the msg-finalize blocker. error code [0x%x].", rc);

    clMsgDebugCliDeregister();

    clMsgCommIdlFinalize();

    /* Finalize the IDL generated code. */
    rc = clMsgCltClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Client Table. error code [0x%x].", rc);

    rc = clMsgIdlClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Server Table. error code [0x%x].", rc);

    clMsgIdlClientUninstall();

    rc = clMsgCltSrvClientTableDeregister();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to deregister Client Server Table. error code [0x%x].", rc);

    clMsgCltSrvClientUninstall();

    /* Finalize cached ckpt for MSG queue & MSG queue group */
    rc = clMsgQCkptFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "clMsgQCkptFinalize(): error code [0x%x].", rc);

    /* Finalize database for maintaining queues. */
    clMsgReceiverDatabaseFin();

    rc = clMsgQueueFinalize();
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to finalize queue databases. error code [0x%x].", rc);

    rc = clCpmNotificationCallbackUninstall(&gMsgNotificationHandle);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to uninstall the notification callback function. error code [0x%x].", rc);

    rc = clOsalMutexDestroy(&gClGroupDbLock);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the group db lock. error code [0x%x].", rc);

    rc = clHandleDatabaseDestroy(gMsgClientHandleDb);
    if(rc != CL_OK)
        clLogError("MSG", "FIN", "Failed to destroy the client handle database. error code [0x%x].", rc);

    return rc;
}


/* dispatch loop  */
void dispatchLoop(void)
{        
    SaAisErrorT         rc = SA_AIS_OK;
    SaSelectionObjectT amf_dispatch_fd;
    int maxFd;
    fd_set read_fds;

    /*
     * Get the AMF dispatch FD for the callbacks
    */
    if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
        errorExit(rc);

    maxFd = amf_dispatch_fd;  /* maxFd = max(amf_dispatch_fd,ckpt_dispatch_fd); */
    do
    {
       FD_ZERO(&read_fds);
       FD_SET(amf_dispatch_fd, &read_fds);
       
       if( select(maxFd + 1, &read_fds, NULL, NULL, NULL) < 0)
       {
          char errorStr[80];
          int err = errno;
          if (EINTR == err) continue;

          errorStr[0] = 0; /* just in case strerror does not fill it in */
          strerror_r(err, errorStr, 79);
          break;
       }
       if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
    }while(!unblockNow);      
}

