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
 * ModuleName  : ckpt                                                          
 * File        : clCkptMain.c
 *******************************************************************************/

/*******************************************************************************
* Description :                                                              
*   This file contains Checkpoint EO related implementation
*
*
*****************************************************************************/
/* Standard Inculdes */
#include <string.h>  

/* ASP Includes */
#include <clEoApi.h>
#include <clDebugApi.h>
#include <clCpmApi.h>

/* CKPT Includes */
#include <clEoQueue.h>
#include <clCkptUtils.h>
#include <clCkptSvr.h>
#include "clCkptSvrIpi.h"
#include "clCkptMaster.h"
#include "clCkptDs.h"

extern CkptSvrCbT  *gCkptSvr;

static ClRcT   ckptSvcInitialize(ClUint32T argc, ClCharT *argv[]);
static ClRcT   ckptSvcFinalize();
static ClRcT   ckptSvcStateChange(ClEoStateT eoState);
static ClRcT   ckptSvcHealthCheck(ClEoSchedFeedBackT* schFeedback);
static ClRcT   ckptTerminate(ClInvocationT invocation, const ClNameT  *compName);

ClVersionT  gVersion;

/*
 * Ckpt EO configuration related parameters.
 */
 
ClEoConfigT clEoConfig = 
{
    "CKP",                      /* EO Name */
    1,                          /* Thread priority */ 
    CL_CKPT_MAX_NUM_THREADS,   /* No of EO threads */
    CL_IOC_CKPT_PORT,           /* Ioc port no. */
    CL_EO_USER_CLIENT_ID_START, /* How many clients (inluding ASP) 
                                   are needed */
    CL_EO_USE_THREAD_FOR_RECV,  /*  Whether to use main thread for 
                                   EO Recv or not. */
    ckptSvcInitialize,          /* Ckpt server ebtry point.*/
    ckptSvcFinalize,            /* Callback for ckpt finalization. */
    ckptSvcStateChange,         /* Callback for ckpt state change */
    ckptSvcHealthCheck,         /* Callback for ckpt health check. */
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


/*
 * Checkpoint server entry point. This will be called after ckpt EO creation.
 */
 
ClRcT   ckptSvcInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClNameT		     appName   = {0};
	ClCpmCallbacksT	 callbacks = {0};
	ClIocPortT       iocPort   = 0;
    ClCpmHandleT     cpmHandle = 0;
	ClRcT	         rc        = CL_OK;

    /*
     * Set the supported version.
     */
	gVersion.releaseCode = 'B';
	gVersion.majorVersion = 0x01;
	gVersion.minorVersion = 0x01;
	
    /*
     * Fill the cpm's callback structure.
     */
	callbacks.appHealthCheck                 = NULL;
	callbacks.appTerminate                   = ckptTerminate;
	callbacks.appCSISet                      = NULL;
	callbacks.appCSIRmv                      = NULL;
	callbacks.appProtectionGroupTrack        = NULL;
	callbacks.appProxiedComponentInstantiate = NULL;
	callbacks.appProxiedComponentCleanup     = NULL;

    /*
     * Get the port Id from IOC.
     */
	clEoMyEoIocPortGet(&iocPort);

    clCkptLeakyBucketInitialize();

    /*
     * Initialize the cpm client library.
     */
	rc = clCpmClientInitialize(&cpmHandle, &callbacks, &gVersion);

    /*
     * Get the component name from cpm.
     */
    rc = clCpmComponentNameGet(cpmHandle, &appName);
 
    /*
     * Initialize ckpt server.
     */
    clCkptSvrInitialize();

    /*
     * Register ckpt server with cpm.
     */
    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
    
    gCkptSvr->cpmHdl = cpmHandle;

    /*
     * Obtain the component id from cpm. This will be used while registering
     * ckpt master address with TL.
     */
    clCpmComponentIdGet(gCkptSvr->cpmHdl, &appName, &gCkptSvr->compId);

    /*
     * Register with debug server.
     */
    ckptDebugRegister(gCkptSvr->eoHdl);
    
    return CL_OK;
}



/*
 * Ckpt finalize function.
 */
 
ClRcT ckptTerminate(ClInvocationT invocation,
			const ClNameT  *compName)
{
    ClRcT     rc     = CL_OK;
    ClHandleT cpmHdl = 0;

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
    rc = clCpmComponentUnregister(gCkptSvr->cpmHdl, compName, NULL);
    
    cpmHdl = gCkptSvr->cpmHdl;

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

    /*
     * Finalize cpm client library.
     */
    rc = clCpmClientFinalize(cpmHdl);
    clCpmResponse(cpmHdl, invocation, CL_OK);
    return CL_OK;
}



/*
 * Not required.
 */
 
ClRcT   ckptSvcFinalize()
{
    return CL_OK;
}



/*
 * Ckpt server's state change callback function.
 */
 
ClRcT   ckptSvcStateChange(ClEoStateT eoState)
{
    return CL_OK;
}



/*
 * Ckpt server's health check callback function.
 */
 
ClRcT   ckptSvcHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    schFeedback->freq = CL_EO_DONT_POLL;
    return CL_OK;
}
