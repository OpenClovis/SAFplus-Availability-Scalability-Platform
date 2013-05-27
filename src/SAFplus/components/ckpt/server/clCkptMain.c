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
 * This file is autogenerated by OpenClovis IDE, 
 */
/*******************************************************************************
 * ModuleName  : ckpt                                                          
 * File        : clCkptMain.c
 *******************************************************************************/

/*******************************************************************************
* Description :                                                              
*   This file contains Checkpoint Non-EO related implementation
*
*
*****************************************************************************/
/* Standard Inculdes */
#include <string.h>  

/* POSIX Includes */
#include <assert.h>
#include <errno.h>

/* Basic SAFplus Includes */
#include <clCommon.h>

/* ASP Includes */
#include <clDebugApi.h>
#include <clCpmApi.h>
#include <saAmf.h>

/* CKPT Includes */
#include <clEoQueue.h>
#include <clCkptUtils.h>
#include <clCkptSvr.h>
#include "clCkptSvrIpi.h"
#include "clCkptMaster.h"
#include "clCkptDs.h"

extern CkptSvrCbT  *gCkptSvr;


SaVersionT  gVersion;






/*
 * Ckpt server specific log messages.
 */
 
ClCharT *clCkptLogMsg[]=
{
    "CheckPoint created  %s",
    "CheckPoint closed  %s",
    "CheckPoint Deleted %s",
    "Section Created %s",
    "Section Deleted %s",
    "Checkpoint written  %s",
    "Section overwritten  %s",
    "Checkpoint read  %s",
    "%s NACK received from Node[0x%x:0x%x] - Supported Version {'%c', 0x%x, 0x%x}",
    "Improper data :%s",
    "NULL"
};

ClEoConfigT clEoConfig = 
{
    1,                          /* Thread priority */ 
    CL_CKPT_MAX_NUM_THREADS,   /* No of EO threads */
    CL_IOC_CKPT_PORT,           /* Ioc port no. */
    CL_EO_USER_CLIENT_ID_START, /* How many clients (inluding ASP) 
                                   are needed */
    CL_EO_USE_THREAD_FOR_RECV,  /*  Whether to use main thread for 
                                   EO Recv or not. */
    NULL,          /* Ckpt server ebtry point.*/
    NULL,            /* Callback for ckpt finalization. */
    NULL,         /* Callback for ckpt state change */
    NULL,         /* Callback for ckpt health check. */
    NULL
};



/* 
 * What basic and client libraries do we need to use? 
 */
 
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,            /* osal */
    CL_TRUE,            /* timer */
    CL_TRUE,            /* buffer */
    CL_TRUE,            /* ioc */
    CL_TRUE,            /* rmd */
    CL_TRUE,            /* eo */
    CL_FALSE,           /* om */
    CL_FALSE,           /* hal */
    CL_TRUE,            /* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_FALSE,            /* cor */
    CL_FALSE,            /* cm */
    CL_FALSE,            /* name */
    CL_TRUE,             /* log */
    CL_FALSE,            /* trace */
    CL_FALSE,            /* diag */
    CL_FALSE,            /* txn */
    CL_FALSE,            /* hpi */
    CL_FALSE,            /* cli */
    CL_FALSE,            /* alarm */
    CL_TRUE,             /* debug */
    CL_TRUE,              /* gms */
    CL_FALSE,             /*pm*/
};
/* The application should fill these functions */
static void ckptTerminate(SaInvocationT invocation, const SaNameT *compName);


/* Utility functions */
void initializeAmf(void);
void dispatchLoop(void);
int  errorExit(SaAisErrorT rc);

/******************************************************************************
 * Global Variables.
 *****************************************************************************/

/* The process ID is stored in this variable.  This is only used in our logging. */
pid_t        mypid;

/* Access to the SAF AMF framework occurs through this handle */
SaAmfHandleT amfHandle;

/* This process's SAF name */
SaNameT      appName = {0};


ClBoolT unblockNow = CL_FALSE;

/* Declare other global variables here. */


/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/


ClInt32T main(ClInt32T argc, ClCharT *argv[])
{
    SaAisErrorT rc = SA_AIS_OK;

    /* Connect to the SAF cluster */
    initializeAmf();
	
   
    /* Do the application specific initialization here. */
    
    /* Block on AMF dispatch file descriptor for callbacks.
       When this function returns its time to quit. */
    dispatchLoop();
    
    
    /* Now finalize my connection with the SAF cluster */
    rc = saAmfFinalize(amfHandle);
      

    return 0;
}



void initializeAmf(void)
{
    SaAmfCallbacksT     callbacks;
   
    ClNameT             appName1;
    ClIocPortT          iocPort=0;
    SaAisErrorT         rc = SA_AIS_OK;
    /*This function overrides the default EO Configuaration */
    clAppConfigure(&clEoConfig,clEoBasicLibs,clEoClientLibs);
    /*
     * Initialize and register with SAFplus AMF. 'version' specifies the
     * version of AMF with which this application would like to
     * interface. 'callbacks' is used to register the callbacks this
     * component expects to receive.
     */
     /*
     * Set the supported version.
     */
     gVersion.releaseCode = 'B';
     gVersion.majorVersion = 0x01;
     gVersion.minorVersion = 0x01;
   
     /*
     * Fill the AMS's callback structure.
     */
     callbacks.saAmfHealthcheckCallback          = NULL; /* rarely necessary because SAFplus monitors the process */
     callbacks.saAmfComponentTerminateCallback   = ckptTerminate;
     callbacks.saAmfCSISetCallback               = NULL;
     callbacks.saAmfCSIRemoveCallback            = NULL;
     callbacks.saAmfProtectionGroupTrackCallback = NULL;
     
     /*
     * Get the port Id from IOC.
     */
     clEoMyEoIocPortGet(&iocPort);

     
        
     /* Initialize AMF client library. */
     rc = saAmfInitialize(&amfHandle, &callbacks, &gVersion);
       
     /*
     * Get the component name from AMF.
     */

     rc = saAmfComponentNameGet(amfHandle, &appName);
      /*
     * Initialize ckpt server.
     */

     clCkptLeakyBucketInitialize();

     clCkptSvrInitialize();
     

     rc = saAmfComponentRegister(amfHandle, &appName, NULL);
    
     gCkptSvr->cpmHdl = amfHandle;

     /*
     * Obtain the component id from cpm. This will be used while registering
     * ckpt master address with TL.
     */
     appName1.length = appName.length;
     memcpy(appName1.value,appName.value,appName.length);
     clCpmComponentIdGet(gCkptSvr->cpmHdl, &appName1, &gCkptSvr->compId);

     /*
     * Register with debug server.
     */
     ckptDebugRegister(gCkptSvr->eoHdl);
    
    
}

/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

void ckptTerminate(SaInvocationT invocation, const SaNameT *compName)
{
    SaAisErrorT rc = SA_AIS_OK;
    
    CL_DEBUG_PRINT(CL_DEBUG_WARN,("Checkpoint service is stopping."));
    /*
     * Deregister with debug server.
     */
    ckptDebugDeregister(gCkptSvr->eoHdl);
    
    /*
     * Wait for all the other threads to finalize  
     */
     clEoQueuesQuiesce();

     /*
     * Deregister with cpm.
     */

    rc = saAmfComponentUnregister(amfHandle, compName, NULL);
    
    /*
     * Cleanup the persistent DB information.
     */

    /*
    Feature not supported -- see bug 6017 -- don't forget to uncomment ckptDataBackupInitialize()!
    ckptDataBackupFinalize();
    */

    /*
     * Cleanup resources taken by ckpt server.
     */
    ckptShutDown();

    /* Ok tell SAFplus that we handled it properly */
   


    saAmfResponse(amfHandle, invocation, SA_AIS_OK);
    unblockNow = CL_TRUE;
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

  /* This boilerplate code includes an example of how to simultaneously
     dispatch for 2 services (in this case AMF and CKPT).  But since checkpoint
     is not initialized or used, it is commented out */
  /* SaSelectionObjectT ckpt_dispatch_fd; */

  /*
   * Get the AMF dispatch FD for the callbacks
   */
  if ( (rc = saAmfSelectionObjectGet(amfHandle, &amf_dispatch_fd)) != SA_AIS_OK)
    errorExit(rc);
  /* if ( (rc = saCkptSelectionObjectGet(ckptLibraryHandle, &ckpt_dispatch_fd)) != SA_AIS_OK)
       errorExit(rc); */
    
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
          //clprintf (CL_LOG_SEV_ERROR, "Error [%d] during dispatch loop select() call: [%s]",err,errorStr);
          break;
        }
      if (FD_ISSET(amf_dispatch_fd,&read_fds)) saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
      /* if (FD_ISSET(ckpt_dispatch_fd,&read_fds)) saCkptDispatch(ckptLibraryHandle, SA_DISPATCH_ALL); */
    }while(!unblockNow);      
}






