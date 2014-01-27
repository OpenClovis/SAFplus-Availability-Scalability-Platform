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
 * ModuleName  : gms                                                           
 * File        : clGmsEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file implements the standard NON-EO template functions 
 * and data structures for GMS.
 *******************************************************************************/
#include <saAmf.h>
#define __SERVER__
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clCpmApi.h>
#include <clIocServices.h>
#include <clEoConfigApi.h>
#include <clSAClientSelect.h>

#include <clGms.h>
#include <clGmsRmdServer.h>
#include <clGmsCli.h>
#include <clGmsCommon.h>
#include <clGmsErrors.h>
#include <clGmsEngine.h>
#include <clLogApi.h>
#include <clGmsLog.h>
#include <clGmsEoFunc.h>


#undef __CLIENT__
#include <clGmsServerFuncTable.h>


static ClHandleT  gGmsDebugReg = CL_HANDLE_INVALID_VALUE;
static SaAmfHandleT amfHandle;
#define GMS_USAGE "safplus_gms <xml config file>\n\0"
/*
   clGmsServerTerminate
   --------------------
   Terminates the GMS server instance and cleansup all the assosciations . 
*/
static void clGmsServerTerminate(SaInvocationT invocation, const SaNameT *compName);
static ClRcT clGmsServerStateChange(ClEoStateT eoState);
static ClRcT clGmsServerHealthCheck(ClEoSchedFeedBackT* const schFeedback);

ClEoConfigT clEoConfig = {
    (ClOsalThreadPriorityT)1,                	    	
    1,                	     
    CL_IOC_GMS_PORT,   	                 /* Service port for recieving reqs */
    CL_EO_USER_CLIENT_ID_START,           
    CL_EO_USE_THREAD_FOR_RECV,            /* application is blocking         */
    NULL,                                /* service initialization func     */
    NULL,               	             /* service finalize function       */  
    clGmsServerStateChange,              /* state change callback           */
    clGmsServerHealthCheck,              /* health checking callback        */  
    NULL,
    CL_TRUE,
    };




ClUint8T clEoBasicLibs[] = {
    CL_TRUE,                    /* osal */
    CL_TRUE,                    /* timer */
    CL_TRUE,                    /* buffer */
    CL_TRUE,                    /* ioc */
    CL_TRUE,                    /* rmd */
    CL_TRUE,                    /* eo */
    CL_FALSE,                   /* om */
    CL_FALSE,                   /* hal */
    CL_FALSE,                   /* dbal */
};

ClUint8T clEoClientLibs[] = {
    CL_TRUE,                    /* cor */
    CL_FALSE,                   /* cm */
    CL_FALSE,                   /* name */
    CL_TRUE,                   /* log */
    CL_FALSE,                   /* trace */
    CL_FALSE,                   /* diag */
    CL_FALSE,                   /* txn */
    CL_FALSE,                   /* hpi */
    CL_FALSE,                   /* cli */
    CL_FALSE,                   /* alarm */
    CL_TRUE,                    /* debug */
    CL_TRUE,                    /* gms */
    CL_FALSE,                    /* pm */
};





/* Utility functions */
ClRcT initializeAmf(void);
void dispatchLoop(void);
int  errorExit(SaAisErrorT rc);



/******************************************************************************
 * Global Variables.
 *****************************************************************************/


/* Access to the SAF AMF framework occurs through this handle */


/* This process's SAF name */
SaNameT      appName = {0};


ClBoolT unblockNow = CL_FALSE;

/* Declare other global variables here. */


/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/


int main(int argc, char *argv[])
{
    ClRcT   rc = CL_OK;
    if (argc < 2){
        clLog (EMER,GEN,NA, "usage : %s", GMS_USAGE);
        exit(0);
    }

    /* Connect to the SAF cluster */
    rc = initializeAmf();
    if( rc != CL_OK)
    {
        clLogCritical(GEN,NA,
                      "GMS: gmsInitialize failed [0x%X]\n\r", rc);
        return rc;
    }
    _clGmsServiceInitialize ( argc , argv );

    
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();
    
    saAmfFinalize(amfHandle); 

    return 0;
}
/*
   clGmsServerStateChange 
   ----------------------
   Called by the Component manager to change the state of the GMS execution
   object . 
   */
ClRcT clGmsServerStateChange(ClEoStateT eoState)
{
    clLog(CRITICAL,GEN,NA,
            "Server Got State Change Request ");
    return CL_OK;
}

/*
   clGmsServerHealthCheck 
   -----------------------
   health check callback called periodically by Component Manager status of
   the server BUSY , ALIVE is sent back to the component manager . 
*/
ClRcT  clGmsServerHealthCheck(ClEoSchedFeedBackT* const schFeedback)
{
    if (schFeedback == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }
	schFeedback->freq   = CL_EO_DEFAULT_POLL; 
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;
}

/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

void clGmsServerTerminate(SaInvocationT invocation, const SaNameT *compName)
{

    ClRcT   rc = CL_OK;

    gmsGlobalInfo.opState = CL_GMS_STATE_SHUTING_DOWN;

    clLog(CRITICAL,GEN,NA, "Got termination request for component [%.*s]. Started Shutting Down...", compName->length,compName->value);
    
    rc = clEoClientUninstallTables (gmsGlobalInfo.gmsEoObject, CL_EO_SERVER_SYM_MOD(gAspFuncTable, GMS));
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA, "clEoClientUninstall failed with rc = 0x%x", rc);
    }
    /*
     * Unregister with AMF and respond to AMF saying whether the
     * termination was successful or not.
     */
    

    rc = clDebugDeregister(gGmsDebugReg);
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA, "clDebugDeregister failed with rc = 0x%x", rc);
    }

    /* Close the leader election algorithm dl if open */
#ifndef VXWORKS_BUILD 
    if (pluginHandle != NULL)
    {
        dlclose(pluginHandle);
    }
#endif

    if(gClTotemRunning)
    {
        /* We need to invoke openais finalize function instead of signal
         * handler here */
        totempg_finalize();

        /* Waiting for 10ms before invoking exit() */
        usleep(10000);
    }
    clLog(CRITICAL,GEN,NA,
          "GMS server exiting");

    rc = clHandleDatabaseDestroy(contextHandleDatabase);
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "contextHandleDatabase destroy failed with Rc = 0x%x",rc);
    }
    rc = saAmfComponentUnregister(amfHandle, compName, NULL);
    
    if(rc != SA_AIS_OK) 
    {
        clLog(ERROR,GEN,NA,
              "saAmfComponentUnregister failed with rc = 0x%x", rc);     
    }

    rc = saAmfFinalize(amfHandle);

    if (rc != SA_AIS_OK)
    {
        clLog(ERROR,GEN,NA,
              "saAmfFinalize failed with rc = 0x%x", rc);
    }
    /* Ok tell SAFplus that we handled it properly */
    rc = saAmfResponse(amfHandle, invocation, SA_AIS_OK);
    
    if (rc != SA_AIS_OK)
    {
        clLog(ERROR,GEN,NA,
              "clCpmResponse failed with rc = 0x%x", rc);
    }
    unblockNow = CL_TRUE;
}


/******************************************************************************
 * Utility functions 
 *****************************************************************************/
/* This simple helper function just prints an error and quits */
int errorExit(SaAisErrorT rc)
{        
    exit(-1);
    return -1;
}

/*
   initializeAmf
   ----------------------
   Starting point for the GMS server instance , creates the ASP interface and
   necessary initializations required to work with the ASP
   infrastructure.Calls the clGmsServiceInitialize function to start the
   actual GMS service instance and finally blocks in the function. 
 */

ClRcT initializeAmf(void)
{
    SaAmfCallbacksT     callbacks;
    SaVersionT          version;
    ClRcT	        rc = CL_OK;

    clLogCompName = (ClCharT*)"GMS"; /* Override generated eo name with a short name for our server */
    /* this function overrides the default EO configuration */
    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs); 

    /*  Do the CPM client init/Register */
    version.releaseCode  = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.saAmfHealthcheckCallback          = NULL; 
    callbacks.saAmfComponentTerminateCallback   = clGmsServerTerminate;
    callbacks.saAmfCSISetCallback               = NULL;
    callbacks.saAmfCSIRemoveCallback            = NULL;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;

    gmsGlobalInfo.opState = CL_GMS_STATE_STARTING_UP;

    clLog(INFO,GEN,NA,
            CL_GMS_SERVER_STARTED);
        
    /* Initialize AMF client library. */
    rc = saAmfInitialize(&amfHandle, &callbacks, &version);
    if(rc!= SA_AIS_OK)
    {
        clLog(EMER,GEN,NA,
                "Sa AMF initialization Failed with rc [0x%x]. Booting Aborted",rc);
        exit(0);
        
    }
    gmsGlobalInfo.cpmHandle=amfHandle;
    /*
     * Now register the component with AMF. At this point it is
     * ready to provide service, i.e. take work assignments.
     */

    rc = saAmfComponentNameGet(amfHandle, &appName);
    if(rc != SA_AIS_OK)
    {
         clLog(EMER,GEN,NA,
                "saAmfomponentNameGet Failed with rc [0x%x]. Booting Aborted",rc);
        exit(0);
      
    }
    gmsGlobalInfo.gmsComponentName.length=appName.length;
    memcpy(gmsGlobalInfo.gmsComponentName.value,appName.value,appName.length);

    rc = clEoMyEoObjectGet(&gmsGlobalInfo.gmsEoObject);
    if (rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
               "clEoMyEoObjectGet Failed with rc [0x%x]. Booting Aborted",rc);
        exit(0);
    }
    
    rc = clDebugPromptSet(GMS_COMMAND_PROMPT);
    if( CL_OK == rc )
    {
        clLog(NOTICE,GEN,NA,
            "GMS Server registering with debug service");
        rc = clDebugRegister(
                gmsCliCommandsList, 
                (int)(sizeof(gmsCliCommandsList)/sizeof(ClDebugFuncEntryT)), 
                &gGmsDebugReg);
    }
    if( rc != CL_OK )
    {
        clLog(ERROR,GEN,NA,
            "Failed to register with debug server, error [0x%x]. Debug shell access disabled",
            rc);
    }

    rc = clEoClientInstallTables (
            gmsGlobalInfo.gmsEoObject,
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, GMS));

    if ( rc != CL_OK )
    {
        clLog (EMER,GEN,NA,
                "Eo client install failed with rc [0x%x]. Booting aborted",rc);
        exit(0);
    }

    /* This function never returns the exit is done by causing a signal from
     *  the Terminate function */
   return CL_OK;
    
}

void dispatchLoop(void)
{        
  SaAisErrorT         rc = SA_AIS_OK;
  SaSelectionObjectT amf_dispatch_fd;
  int maxFd;
  fd_set read_fds;

  
  if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
    errorExit(rc);
    
  maxFd = amf_dispatch_fd;  /* maxFd = max(amf_dispatch_fd,ckpt_dispatch_fd); */
  do
    {
      FD_ZERO(&read_fds);
      FD_SET(amf_dispatch_fd, &read_fds);
      /* FD_SET(ckpt_dispatch_fd, &read_fds); */
        
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
      /* if (FD_ISSET(ckpt_dispatch_fd,&read_fds)) saCkptDispatch(ckptLibraryHandle, SA_DISPATCH_ALL); */
    }while(!unblockNow);      
}




