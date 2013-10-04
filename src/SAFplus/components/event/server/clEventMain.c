/*
 * Copyright (C) 2002-2013 OpenClovis Solutions Inc.  All Rights Reserved.
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
 * ModuleName  : event
 * File        : clEventMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for EM - Non-EOnization & association with CPM, debug, etc.
 *****************************************************************************/
#include <saAmf.h>
#include <clSafUtils.h>
#define __SERVER__
#include "clCpmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clRmdApi.h"
#include <clEventClientIpi.h>
#include "clEventServerIpi.h"
#include "clEventUtilsIpi.h"

#include "clEventCkptIpi.h"     /* For Check Pointing Service */

#undef __CLIENT__
#include "clEventServerFuncTable.h"

#define EVENT_LOG_AREA		"EVT"
#define EVENT_LOG_CTX_EVENT_INI	"INI"
#define EVENT_LOG_CTX_EVENT_FIN	"FIN"

static SaAmfHandleT gClEvtAmfHandle;

ClBoolT unblockNow = CL_FALSE;

void clEventTerminate(SaInvocationT invocation, const SaNameT *compName);

ClRcT initializeAmf()
{
    SaNameT             appName;      
    SaAmfCallbacksT     callbacks;
    SaVersionT          version;
    ClIocPortT          iocPort;
    SaAisErrorT         rc = SA_AIS_OK;

    version.releaseCode  = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.saAmfHealthcheckCallback          = NULL; /* rarely necessary because SAFplus monitors the process */
    callbacks.saAmfComponentTerminateCallback   = clEventTerminate;
    callbacks.saAmfCSISetCallback               = NULL;
    callbacks.saAmfCSIRemoveCallback            = NULL;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
    callbacks.saAmfProxiedComponentInstantiateCallback = NULL;
    callbacks.saAmfProxiedComponentCleanupCallback = NULL;

    clEoMyEoIocPortGet(&iocPort);
    rc = saAmfInitialize(&gClEvtAmfHandle, &callbacks, &version);
    if(rc != SA_AIS_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
                   CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, " CPM Library", rc);
        return clSafToClovisError(rc);
    }
    saAmfComponentNameGet(gClEvtAmfHandle, &appName);
    saAmfComponentRegister(gClEvtAmfHandle, &appName, NULL);

    return CL_OK;
    
}

/*****************************************************************/

/*
 * This function will do following steps 
 * 1. Create DB for ECH/G 
 * 2. Create DB for ECH/L 
 * 3. Create mutex to protect ECH/G DB 
 * 4. Create mutex to protect ECH/L DB 
 * 5. Register with CPM for component failure notification. 
 * 6. Intialize the global variable to indicate EM is done. 
 */
ClRcT clEvtInitialize(ClInt32T argc, ClCharT *argv[])
{

    ClRcT rc = CL_OK;

    rc = initializeAmf();
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: Amf Initialization failed [0x%X]\n\r", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    CL_FUNC_ENTER();
    rc = clEvtChannelDBInit();
    rc = clEoMyEoObjectGet(&gEvtHead.evtEOId);
    rc = clEoClientInstallTables(gEvtHead.evtEOId, CL_EO_SERVER_SYM_MOD(gAspFuncTable, EVT));
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: Installing Native table failed [0x%X]\n\r",
                      rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
                   CL_LOG_MESSAGE_1_FUNC_TABLE_INSTALL_FAILED, rc);
        clCntDelete(gEvtMasterECHHandle);
        clEvtChannelDBClean();
        CL_FUNC_EXIT();
        return rc;
    }
    rc = clEventLibTableInit(); 
    if(rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: Installing Native Server to Server table failed [0x%X]\n\r",
                      rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
                   CL_LOG_MESSAGE_1_FUNC_TABLE_INSTALL_FAILED, rc);
        clCntDelete(gEvtMasterECHHandle);
        clEvtChannelDBClean();
        CL_FUNC_EXIT();
        return rc;
    }
    rc = clEventDebugRegister(gEvtHead.evtEOId);
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: Debug Register failed [0x%X]\n\r", rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
                   CL_LOG_MESSAGE_1_DBG_REGISTER_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }
    /*
     ** Initialize the Handle Database.
     */
    rc = clEvtHandleDatabaseInit();
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: Handle Database Init failed [0x%X]\n\r", rc);
        CL_FUNC_EXIT();
        return rc;
    }

#ifdef CKPT_ENABLED
    /*
     ** Initialize the Check Pointing Library.
     */
    rc = clEvtCkptInit();
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: CKPT Init failed [0x%X]\n\r", rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL,
                   CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "Checkpoint Library",
                   rc);
        CL_FUNC_EXIT();
        return rc;
    }
#endif

    gEvtInitDone = CL_EVT_TRUE;

    CL_FUNC_EXIT();

    return CL_OK;
}

/*****************************************************************************/

/*
 * This function clean up the information which is it has 
 * 1. Deregister with CPM. 
 * 2. Deregister with debug. 
 * 3. Deregister with CKPT
 * 4. Release the resources like Handle Databases, etc.
 */

ClRcT clEvtFinalize()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();
    CL_EVT_INIT_DONE_VALIDATION();
    rc = clEoClientUninstallTables(gEvtHead.evtEOId, CL_EO_SERVER_SYM_MOD(gAspFuncTable, EVT));  
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_FIN,
                      "Event: EO Client Uninstall failed [0x%X]\n\r", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    rc = clEventDebugDeregister(gEvtHead.evtEOId);
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_FIN,
                      "Event: Debug Deregister failed [0x%X]\n\r", rc);
        CL_FUNC_EXIT();
        return rc;
    }
    /*
     ** Handle Database Cleanup.
    */
    rc = clEvtHandleDatabaseExit();
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_FIN,
                      "Event: Handle Database Cleanup failed [0x%X]\n\r",
                      rc);
        CL_FUNC_EXIT();
        return rc;
    }

#ifdef CKPT_ENABLED
    /*
     ** Check Pointing Related Cleanup.
     */
    clEvtCkptExit();
    if (rc != CL_OK)
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_FIN,
                      "Event Ckpt Exit failed [0x%X]\n\r", rc);
        CL_FUNC_EXIT();
        return rc;
    }
#endif

    CL_FUNC_EXIT();
    return CL_OK;
}

void clEventTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    clEvtFinalize();

    /* No need to check error messages b/c cannot do anything about the errors anyway... am shutting down */
    saAmfComponentUnregister(gClEvtAmfHandle, compName, NULL);
    saAmfResponse(gClEvtAmfHandle, invocation, SA_AIS_OK);
    unblockNow = CL_TRUE;
}

/*****************************************************************************/
ClEoConfigT clEoConfig = {
    1,                          /* EO Thread Priority */
    1,                          /* No of EO thread needed */
    CL_IOC_EVENT_PORT,          /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,  /* Whether to use main thread for eo Recv or
                                 * not */
    NULL,                       /* Function CallBack to initialize the
                                 * Application */
    NULL,                       /* Function Callback to Terminate the
                                 * Application */
    NULL,                       /* Function Callback to change the Application
                                 * state */
    NULL,                       /* Function Callback to change the Application
                                 * state */
    NULL,
    .needSerialization = CL_TRUE
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
    CL_FALSE,                    /* gms */
    CL_FALSE,                    /* pm */
};


void dispatchLoop(void);
int  errorExit(SaAisErrorT rc);

/******************************************************************************
 * Service Life Cycle Management Functions
 *****************************************************************************/


ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    ClRcT rc = CL_OK;

    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);
    
    rc = clEvtInitialize(argc,argv);
    if( rc != CL_OK )
    {
        clLogCritical(EVENT_LOG_AREA,EVENT_LOG_CTX_EVENT_INI,
                      "Event: clEvtInitialize failed [0x%X]\n\r", rc);
        return rc;
    }
            
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();
        
    saAmfFinalize(gClEvtAmfHandle);
     
    return 0;
}

int errorExit(SaAisErrorT rc)
{        
  
    exit(-1);
    return -1;
}

void dispatchLoop(void)
{        
    SaAisErrorT         rc = SA_AIS_OK;
    SaSelectionObjectT amf_dispatch_fd;
    int maxFd;
    fd_set read_fds;
  
    /*
    * Get the AMF dispatch FD for the callbacks
    */
    if ( (rc = saAmfSelectionObjectGet(gClEvtAmfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
       errorExit(rc);
    
    maxFd = amf_dispatch_fd;  
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
       if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(gClEvtAmfHandle, SA_DISPATCH_ALL);
      
    }while(!unblockNow);      
}






