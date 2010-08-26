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
 * ModuleName  : gms                                                           
 * File        : clGmsEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file implements the standard EO template functions 
 * and data structures for GMS.
 *******************************************************************************/

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

ClHandleT  gGmsDebugReg = CL_HANDLE_INVALID_VALUE;
void *pluginHandle = NULL;
/*
   clGmsServerTerminate
   --------------------
   Terminates the GMS server instance and cleansup all the assosciations . 
 */
static ClRcT 
clGmsServerTerminate(
        const ClInvocationT invocation,
        const ClNameT* const  compName)
{
    ClRcT   rc = CL_OK;

    gmsGlobalInfo.opState = CL_GMS_STATE_SHUTING_DOWN;

    clLog(CRITICAL,GEN,NA,
            "Server Got Termination Request. Started Shutting Down...");


    rc = clEoClientUninstallTables (gmsGlobalInfo.gmsEoObject,
            CL_EO_SERVER_SYM_MOD(gAspFuncTable, GMS));
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "clEoClientUninstall failed with rc = 0x%x", rc);
    }

    rc = clCpmComponentUnregister(
            gmsGlobalInfo.cpmHandle, 
            compName, 
            NULL
            );
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "clCpmComponentUnregister failed with rc = 0x%x", rc);
    }

    rc = clCpmClientFinalize(gmsGlobalInfo.cpmHandle);

    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "clCpmClientFinalize failed with rc = 0x%x", rc);
    }


    rc = clDebugDeregister(gGmsDebugReg);
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "clDebugDeregister failed with rc = 0x%x", rc);
    }

    rc = clCpmResponse ( 
            gmsGlobalInfo.cpmHandle ,
            invocation  , 
            CL_OK 
            );
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "clCpmResponse failed with rc = 0x%x", rc);
    }

    /* Close the leader election algorithm dl if open */
#ifndef VXWORKS_BUILD 
    if (pluginHandle != NULL)
    {
        dlclose(pluginHandle);
    }
#endif
    /* We need to invoke openais finalize function instead of signal
     * handler here */
    totempg_finalize();

    /* Waiting for 10ms before invoking exit() */
    usleep(10000);
    clLog(CRITICAL,GEN,NA,
            "GMS server exiting");
    exit(0);
}





static ClRcT 
clGmsServerFinalize(void)
{
    ClRcT   rc = CL_OK;
    rc = clHandleDatabaseDestroy(contextHandleDatabase);
    if (rc != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "contextHandleDatabase destroy failed with Rc = 0x%x",rc);
    }

    return rc;
}





#define GMS_USAGE "asp_gms <xml config file>\n\0"


/*
   clGmsServerInitialize
   ----------------------
   Starting point for the GMS server instance , creates the ASP interface and
   necessary initializations required to work with the ASP
   infrastructure.Calls the clGmsServiceInitialize function to start the
   actual GMS service instance and finally blocks in the function. 
 */
   

static ClRcT 
clGmsServerInitialize(
        const ClUint32T argc,  
        ClCharT *argv[]
        )
{
	ClCpmCallbacksT	callbacks;
	ClVersionT      version;
	ClRcT	        rc = CL_OK;


    if (argc < 2){
        clLog (EMER,GEN,NA, "usage : %s", GMS_USAGE);
        exit(0);
    }

    /*  Do the CPM client init/Register */
	version.releaseCode = 'B';
	version.majorVersion = 0x01;
	version.minorVersion = 0x01;
	
	callbacks.appHealthCheck = NULL;
	callbacks.appTerminate = clGmsServerTerminate;
	callbacks.appCSISet = NULL;
	callbacks.appCSIRmv = NULL;
	callbacks.appProtectionGroupTrack = NULL;
	callbacks.appProxiedComponentInstantiate = NULL;
	callbacks.appProxiedComponentCleanup = NULL;
		
    gmsGlobalInfo.opState = CL_GMS_STATE_STARTING_UP;

    clLog(INFO,GEN,NA,
            CL_GMS_SERVER_STARTED);

    rc = clCpmClientInitialize(
            &gmsGlobalInfo.cpmHandle,
            &callbacks,
            &version
            );
    if( rc!= CL_OK )
    {
        clLog(EMER,GEN,NA,
                "CPM Client initialization Failed with rc [0x%x]. Booting Aborted",rc);
        exit(0);
    }

    rc = clCpmComponentNameGet(
            gmsGlobalInfo.cpmHandle,
            &gmsGlobalInfo.gmsComponentName
            );
    if( rc!= CL_OK )
    {
        clLog(EMER,GEN,NA,
                "ComponentNameGet Failed with rc [0x%x]. Booting Aborted",rc);
        exit(0);
    }


    /* Get the EOId of this servvice and use it to install the function pointers
     * that should called by the RMD */

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
    _clGmsServiceInitialize ( argc , argv );

    return rc;
}

/*
   clGmsServerStateChange 
   ----------------------
   Called by the Component manager to change the state of the GMS execution
   object . 
   */

static ClRcT 
clGmsServerStateChange(ClEoStateT eoState)
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
static ClRcT   
clGmsServerHealthCheck(
        ClEoSchedFeedBackT* const schFeedback
        )
{
    if (schFeedback == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }
	schFeedback->freq   = CL_EO_DEFAULT_POLL; 
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;
}


ClEoConfigT clEoConfig = {
    "GMS",                               /* name of the service */
    1,                	    	
    1,                	     
    CL_IOC_GMS_PORT,   	                 /* Service port for recieving reqs */
    CL_EO_USER_CLIENT_ID_START,           
    CL_EO_USE_THREAD_FOR_APP,            /* application is blocking         */
    clGmsServerInitialize,               /* service initialization func     */
    clGmsServerFinalize,   	             /* service finalize function       */  
    clGmsServerStateChange,              /* state change callback           */
    clGmsServerHealthCheck,              /* health checking callback        */  
    NULL,
    .needSerialization = CL_TRUE,
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
