/*******************************************************************************
 * ModuleName  : Demo
 * $File: //depot/dev/main/Andromeda/IDE/models/Demo/templates/clCompAppMain.c $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#include <stdio.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clIocServices.h>

#include <clRmdApi.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clOampRtApi.h>
#include <clProvApi.h>
#include <clAlarmApi.h>

#include <clEoApi.h>
#include <clCpmApi.h>
#include <clIdlApi.h>
#include <string.h>

#include "./clCompAppMain.h"

#if HAS_EO_SERVICES

extern ClRcT idlClientInstall(void);

#endif

ClCpmHandleT		cpmHandle;
ClRcT clCompAppTerminate(ClInvocationT invocation,
			const ClNameT  *compName)
{
    ClRcT rc;
    printf("Inside %s \n", __FUNCTION__);
	/* Do the App Finalization */
    printf("Unregister with CPM before Exit ................. %s\n", compName->value);
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);
    printf("Finalize before Exit ................. %s\n", compName->value);
    rc = clCpmClientFinalize(cpmHandle);
    clCpmResponse(cpmHandle, invocation, CL_OK);

    return CL_OK;
}

ClRcT
clCompAppAMFCSISet(
        ClInvocationT         invocation,
        const ClNameT         *compName,
        ClAmsHAStateT         haState,
        ClAmsCSIDescriptorT   csiDescriptor)
{
    CL_DEBUG_PRINT (CL_DEBUG_TRACE,("Inside Function %s \n",__FUNCTION__));
                                                                                                                             
    if(haState == CL_AMS_HA_STATE_QUIESCING)
    {
	/*TODO make the quiescing complete call after the work assigned is done */
        clOsalPrintf("######## before clCpmCSIQuiescingComplete ########\n");
        clCpmCSIQuiescingComplete(cpmHandle, invocation, CL_OK);
    }
    else
    {
        clOsalPrintf("######## before clCpmResponse ########\n");
        clCpmResponse(cpmHandle, invocation, CL_OK);
    }
                                                                                                                             
    return CL_OK;
}
                                                                                                                             
ClRcT
clCompAppAMFCSIRemove(
        ClInvocationT         invocation,
        const ClNameT         *compName,
        const ClNameT         *csiName,
        ClAmsCSIFlagsT        csiFlags)
{
    CL_DEBUG_PRINT (CL_DEBUG_TRACE,("Inside Function %s \n",__FUNCTION__));
    /*TODO stop the work assigned before making the response done */
                                                                                                                             
    clCpmResponse(cpmHandle, invocation, CL_OK);
                                                                                                                             
    return CL_OK;
}

ClRcT clCompAppInitialize(ClUint32T argc,  ClCharT *argv[])
{
    ClNameT			appName;
	ClCpmCallbacksT	callbacks;
	ClVersionT		version;
	ClIocPortT			iocPort;
	ClRcT	rc = CL_OK;

    /* Do the App intialization */

    /*  Do the CPM client init/Register */
	version.releaseCode = 'B';
	version.majorVersion = 01;
	version.minorVersion = 01;
	
	callbacks.appHealthCheck = NULL;
	callbacks.appTerminate = clCompAppTerminate;
	callbacks.appCSISet = clCompAppAMFCSISet;
	callbacks.appCSIRmv = clCompAppAMFCSIRemove;
	callbacks.appProtectionGroupTrack = NULL;
	callbacks.appProxiedComponentInstantiate = NULL;
	callbacks.appProxiedComponentCleanup = NULL;
		
	clEoMyEoIocPortGet(&iocPort);
	printf("Application Address 0x%x Port %x\n", clIocLocalAddressGet(), iocPort);
	rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
	printf("After clCpmClientInitialize %d\t %x\n", cpmHandle, rc);
	rc = clCpmComponentNameGet(cpmHandle, &appName);
	printf("After clCpmComponentNameGet %d\t %s\n", cpmHandle, appName.value);
	rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
	printf("After clCpmClientRegister %x\n", rc);
        
#if HAS_EO_SERVICES
	idlClientInstall();
#endif
    /* Block and use the main thread if required other wise return */
    
    /*TODO: Application code should come here*/ 
    
    /*if main thread usage policy == CL_EO_USE_THREAD_FOR_APP return from
    this function only after app finishes*/ 
    
    /*if main thread usage policy == CL_EO_USE_THREAD_FOR_RECV return from
    this function immediately after doing some application initialize stuff*/
    
    return CL_OK;
}

ClRcT clCompAppFinalize()
{
    
   
    return CL_OK;
}

ClRcT 	clCompAppStateChange(ClEoStateT eoState)
{
    /* Application state change */
    return CL_OK;
}

ClRcT   clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    /* Modify following as per App requirement*/
    schFeedback->freq   = CL_EO_BUSY_POLL; 
    schFeedback->status = CL_CPM_EO_ALIVE;

    return CL_OK;
}

ClEoConfigT clEoConfig = {
    COMP_EO_NAME,       /* EO Name*/
    COMP_EO_THREAD_PRIORITY,              /* EO Thread Priority */
    COMP_EO_NUM_THREAD,              /* No of EO thread needed */
    COMP_IOC_PORT,         /* Required Ioc Port */
    COMP_EO_USER_CLIENT_ID, 
    COMP_EO_USE_THREAD_MODEL, /* Whether to use main thread for eo Recv or not */
    clCompAppInitialize,  /* Function CallBack  to initialize the Application */
    clCompAppFinalize,    /* Function Callback to Terminate the Application */ 
    clCompAppStateChange, /* Function Callback to change the Application state */
    clCompAppHealthCheck, /* Function Callback to change the Application state */
};

/* You may force default libraries here by using CL_TRUE or CL_FALSE or use
 * appropriate configuration generated by ClovisWorks in ./clCompAppMain.h
 * 
 * The first 6 basic libraries are mandatory. */

ClUint8T clEoBasicLibs[] = {
    COMP_EO_BASICLIB_OSAL,			/* osal */
    COMP_EO_BASICLIB_TIMER,			/* timer */    
    COMP_EO_BASICLIB_BUFFER,			/* buffer */
    COMP_EO_BASICLIB_IOC,			/* ioc */
    COMP_EO_BASICLIB_RMD,			/* rmd */
    COMP_EO_BASICLIB_EO,			/* eo */
    COMP_EO_BASICLIB_OM,			/* om */
    COMP_EO_BASICLIB_HAL,			/* hal */
    COMP_EO_BASICLIB_DBAL,			/* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    COMP_EO_CLIENTLIB_COR,			/* cor */
    COMP_EO_CLIENTLIB_CM,				/* cm */
    COMP_EO_CLIENTLIB_NAME,			/* name */
    COMP_EO_CLIENTLIB_LOG,			/* log */
    COMP_EO_CLIENTLIB_TRACE,			/* trace */
    COMP_EO_CLIENTLIB_DIAG,			/* diag */
    COMP_EO_CLIENTLIB_TXN,			/* txn */
    CL_FALSE,			    /* NA */
    COMP_EO_CLIENTLIB_PROV,			    /* Prov */
    COMP_EO_CLIENTLIB_ALARM,			/* alarm */
    COMP_EO_CLIENTLIB_DEBUG,			/* debug */
    COMP_EO_CLIENTLIB_GMS				/* gms */
};

