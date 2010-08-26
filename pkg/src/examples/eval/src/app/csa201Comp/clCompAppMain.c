/******************************************************************************
 *
 * clCompAppMain.c
 *
 ***************************** Description ************************************
 *
 * This file provides a skeleton for writing a SAF aware component. Application
 * specific code should be added between the ---BEGIN_APPLICATION_CODE--- and
 * ---END_APPLICATION_CODE--- separators.
 *
 * Template Version: 1.0
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

/*
 * POSIX Includes.
 */
 
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Basic ASP Includes.
 */

#include <clCommon.h>
#include <clOsalApi.h>
#include <clIocServices.h>

/*
 * ASP Client Includes.
 */

#include <clRmdApi.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clOampRtApi.h>
#include <clProvApi.h>
#include <clAlarmApi.h>

#include <clEoApi.h>
#include <saAmf.h>
#include <clCpmApi.h>
#include <clIdlApi.h>


/*
 * ---BEGIN_APPLICATION_CODE---
 */
 
#include "clCompAppMain.h"
#include "../ev/ev.h"

/*
 * ---END_APPLICATION_CODE---
 */

/******************************************************************************
 * Optional Features
 *****************************************************************************/

/*
 * This is necessary if the component wishes to provide a service that will
 * be used by other components.
 */

#if HAS_EO_SERVICES

extern ClRcT csa201CompEO0ClientInstall(void);

#endif

/*
 * This template has a few default clprintfs. These can be disabled by 
 * changing clprintf to a null function
 */
 
#define clprintf(severity, ...)   clAppLog(gEvalLogStream, severity, 10, \
				  CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,\
				  __VA_ARGS__)

/*
 * ---BEGIN_APPLICATION_CODE---
 */

/*
 * ---END_APPLICATION_CODE---
 */

/******************************************************************************
 * Global Variables.
 *****************************************************************************/

ClPidT mypid;
SaAmfHandleT cpmHandle;
ClLogStreamHandleT  gEvalLogStream = CL_HANDLE_INVALID_VALUE;

static int running = 0;
static int exiting = 0;

static char*
show_progress(void)
{
    static char bar[] = "          .";
    static int progress = 0;

    /* Show a little progress bar */
    return &bar[sizeof(bar)-2-(progress++)%(sizeof(bar)-2)];
}


/*
 * Description of this EO
 */

ClEoConfigT clEoConfig =
{
    COMP_EO_NAME,               /* EO Name                                  */
    COMP_EO_THREAD_PRIORITY,    /* EO Thread Priority                       */
    COMP_EO_NUM_THREAD,         /* No of EO thread needed                   */
    COMP_IOC_PORT,              /* Required Ioc Port                        */
    COMP_EO_USER_CLIENT_ID, 
    COMP_EO_USE_THREAD_MODEL,   /* Thread Model                             */
    clCompAppInitialize,        /* Application Initialize Callback          */
    clCompAppFinalize,          /* Application Terminate Callback           */
    clCompAppStateChange,       /* Application State Change Callback        */
    clCompAppHealthCheck,       /* Application Health Check Callback        */
};

/*
 * Basic libraries used by this EO. The first 6 libraries are mandatory, the
 * others can be enabled or disabled by setting to CL_TRUE or CL_FALSE.
 */

ClUint8T clEoBasicLibs[] =
{
    COMP_EO_BASICLIB_OSAL,      /* Lib: Operating System Adaptation Layer   */
    COMP_EO_BASICLIB_TIMER,     /* Lib: Timer                               */
    COMP_EO_BASICLIB_BUFFER,    /* Lib: Buffer Management                   */
    COMP_EO_BASICLIB_IOC,       /* Lib: Intelligent Object Communication    */
    COMP_EO_BASICLIB_RMD,       /* Lib: Remote Method Dispatch              */
    COMP_EO_BASICLIB_EO,        /* Lib: Execution Object                    */
    COMP_EO_BASICLIB_OM,        /* Lib: Object Management                   */
    COMP_EO_BASICLIB_HAL,       /* Lib: Hardware Adaptation Layer           */
    COMP_EO_BASICLIB_DBAL,      /* Lib: Database Adaptation Layer           */
};

/*
 * Client libraries used by this EO. All are optional and can be enabled
 * or disabled by setting to CL_TRUE or CL_FALSE.
 */

ClUint8T clEoClientLibs[] =
{
    COMP_EO_CLIENTLIB_COR,      /* Lib: Common Object Repository            */
    COMP_EO_CLIENTLIB_CM,       /* Lib: Chassis Management                  */
    COMP_EO_CLIENTLIB_NAME,     /* Lib: Name Service                        */
    COMP_EO_CLIENTLIB_LOG,      /* Lib: Log Service                         */
    COMP_EO_CLIENTLIB_TRACE,    /* Lib: Trace Service                       */
    COMP_EO_CLIENTLIB_DIAG,     /* Lib: Diagnostics                         */
    COMP_EO_CLIENTLIB_TXN,      /* Lib: Transaction Management              */
    CL_FALSE,                   /* NA */
    COMP_EO_CLIENTLIB_PROV,     /* Lib: Provisioning Management             */
    COMP_EO_CLIENTLIB_ALARM,    /* Lib: Alarm Management                    */
    COMP_EO_CLIENTLIB_DEBUG,    /* Lib: Debug Service                       */
    COMP_EO_CLIENTLIB_GMS,      /* Lib: Cluster/Group Membership Service    */
    COMP_EO_CLIENTLIB_PM        /* Lib: Performance Management              */
};

/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/

/*
 * clCompAppInitialize
 * -------------------
 * This function is invoked when the application is to be initialized.
 */

ClRcT
clCompAppInitialize(
    ClUint32T argc,
    ClCharT *argv[])
{
    SaNameT             appName;
    SaAmfCallbacksT     callbacks;
    SaVersionT          version;
    ClIocPortT          iocPort;
    ClRcT               rc = CL_OK;
    SaAisErrorT		saRc = SA_AIS_OK;

    /*
     * Get the pid for the process and store it in global variable.
     */

    mypid = getpid();

    /*
     * Initialize and register with CPM. 'version' specifies the version of
     * AMF with which this application would like to interface. 'callbacks'
     * is used to register the callbacks this component expects to receive.
     */

    version.releaseCode                         = 'B';
    version.majorVersion                        = 01;
    version.minorVersion                        = 01;
    
    callbacks.saAmfHealthcheckCallback          = NULL;
    callbacks.saAmfComponentTerminateCallback   = clCompAppTerminate;
    callbacks.saAmfCSISetCallback		= clCompAppAMFCSISet;
    callbacks.saAmfCSIRemoveCallback		= clCompAppAMFCSIRemove;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
        
    /*
     * Get IOC Address, Port and Name. Register with AMF.
     */

    clEoMyEoIocPortGet(&iocPort);

    if ( (saRc = saAmfInitialize(&cpmHandle, &callbacks, &version))  != SA_AIS_OK) 
        goto errorexit;

    /*
     * If this component will provide a service, register it now.
     */

#if HAS_EO_SERVICES

extern ClRcT csa201CompEO0ClientInstall(void);

#endif

    /*
     * Do the application specific initialization here.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    // ...

    clprintf(CL_LOG_SEV_INFO,"csa201: Initializing and registering with CPM...\n");

    /*
     * ---END_APPLICATION_CODE---
     */

    /*
     * Now register the component with AMF. At this point it is
     * ready to provide service, i.e. take work assignments.
     */

    if ( (saRc = saAmfComponentNameGet(cpmHandle, &appName)) != SA_AIS_OK) 
        goto errorexit;
    if ( (saRc = saAmfComponentRegister(cpmHandle, &appName, NULL)) != SA_AIS_OK) 
        goto errorexit;
    /* Set up console redirection for demo purposes */
    clEvalAppLogStreamOpen((ClCharT *)appName.value , &gEvalLogStream);

    /*
     * Print out standard information for this component.
     */

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%ld]. Initializing\n", appName.value, mypid);
    clprintf (CL_LOG_SEV_INFO, "   IOC Address             : 0x%x\n", clIocLocalAddressGet());
    clprintf (CL_LOG_SEV_INFO, "   IOC Port                : 0x%x\n", iocPort);

    /*
     * This is where the application code starts. If the main thread usage
     * policy is CL_EO_USE_THREAD_FOR_APP, then return from this fn only 
     * after the application terminates. If the main thread usage policy is
     * CL_EO_USE_THREAD_FOR_RECV, then return from this fn after doing the
     * application specific initialization and registration.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    // ...


    clprintf(CL_LOG_SEV_INFO,"csa201: Instantiated as component instance %s.\n", appName.value);

    clprintf(CL_LOG_SEV_INFO,"%s: Waiting for CSI assignment...\n", appName.value);

    while (!exiting)
    {
        if (running)
        {
            clprintf(CL_LOG_SEV_INFO,"csa201: Hello World! %s\n", show_progress());
        }
        sleep(1);
    }

    /*
     * ---END_APPLICATION_CODE---
     */

    return rc;

errorexit:

    clprintf (CL_LOG_SEV_ERROR, "Component [%s] : PID [%ld]. Initialization error [0x%x]\n",
              appName.value, mypid, rc);

    return rc;
}

/*
 * clCompAppFinalize
 * -----------------
 * This function is invoked when the application is to be terminated.
 */

ClRcT clCompAppFinalize()
{

    SaAisErrorT  saRc = SA_AIS_OK;

    clprintf(CL_LOG_SEV_INFO,"csa201: Finalize called.\n");
    /* Finalizing CPM client library */
    if ((saRc = saAmfFinalize(cpmHandle)) != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_INFO,"csa201: Failed [0x%x] to finalize cpm client library\n", saRc);
        return saRc;
    }

    return CL_OK;
}

/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */

void
clCompAppTerminate(
    SaInvocationT       invocation,
    const SaNameT       *compName)
{
    ClRcT rc = CL_OK;
    SaAisErrorT  saRc = SA_AIS_OK;

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%ld]. Terminating\n",
              compName->value, mypid);

    /*
     * ---BEGIN_APPLICATION_CODE--- 
     */

    // ...
    exiting = 1;

    /*
     * ---END_APPLICATION_CODE---
     */
    
    /*
     * Unregister with AMF and send back a response
     */

    if ( (saRc = saAmfComponentUnregister(cpmHandle, compName, NULL)) != SA_AIS_OK )
        goto errorexit;
    if ( (saRc = saAmfFinalize(cpmHandle)) != SA_AIS_OK )
        goto errorexit;

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%ld]. Terminated\n", compName->value, mypid);
    saAmfResponse(cpmHandle, invocation, SA_AIS_OK);
    clEvalAppLogStreamClose(gEvalLogStream);

    return ;

errorexit:

    clprintf (CL_LOG_SEV_ERROR, "Component [%s] : PID [%ld]. Termination error [0x%x]\n",
              compName->value, mypid, rc);

    return ;
}

/*
 * clCompAppStateChange
 * ---------------------
 * This function is invoked to change the state of an EO.
 */

ClRcT
clCompAppStateChange(
    ClEoStateT eoState)
{
    switch (eoState)
    {
        case CL_EO_STATE_SUSPEND:
        {
            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
            clprintf(CL_LOG_SEV_INFO,"csa201: Suspending...\n");
            running = 0;


            /*
             * ---END_APPLICATION_CODE---
             */

            break;
        }

        case CL_EO_STATE_RESUME:
        {
            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
            clprintf(CL_LOG_SEV_INFO,"csa201: Resuming...\n");
            running = 1;

            /*
             * ---END_APPLICATION_CODE---
             */

            break;
        }
        
        default:
        {
            break;
        }
    }
 
    return CL_OK;
}

/*
 * clCompAppHealthCheck
 * --------------------
 * This function is invoked to perform a healthcheck on the application. The
 * health check logic is application specific.
 */

ClRcT
clCompAppHealthCheck(
    ClEoSchedFeedBackT* schFeedback)
{
    /*
     * Add code for application specific health check below. The defaults
     * indicate EO is healthy and polling interval is unaltered.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */
    
    schFeedback->freq   = CL_EO_DEFAULT_POLL; 
    schFeedback->status = CL_CPM_EO_ALIVE;

    /*
     * ---END_APPLICATION_CODE---
     */

    return CL_OK;
}

/******************************************************************************
 * Application Work Assignment Functions
 *****************************************************************************/

/*
 * clCompAppAMFCSISet
 * ------------------
 * This function is invoked when a CSI assignment is made or the state
 * of a CSI is changed.
 */

void 
clCompAppAMFCSISet(
    SaInvocationT       invocation,
    const SaNameT       *compName,
    SaAmfHAStateT       haState,
    SaAmfCSIDescriptorT csiDescriptor)
{
    /*
     * ---BEGIN_APPLICATION_CODE--- 
     */

    // ...

    /*
     * ---END_APPLICATION_CODE---
     */

    /*
     * Print information about the CSI Set
     */

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%ld]. CSI Set Received\n", 
              compName->value, mypid);

    clCompAppAMFPrintCSI(csiDescriptor, haState);

    /*
     * Take appropriate action based on state
     */

    switch ( haState )
    {
        case SA_AMF_HA_ACTIVE:
        {
            /*
             * AMF has requested application to take the active HA state 
             * for the CSI.
             */

            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
	        clprintf(CL_LOG_SEV_INFO,"csa201: ACTIVE state requested; activating service\n");
        	running = 1;

            /*
             * ---END_APPLICATION_CODE---
             */

            saAmfResponse(cpmHandle, invocation, SA_AIS_OK);
            break;
        }

        case SA_AMF_HA_STANDBY:
        {
            /*
             * AMF has requested application to take the standby HA state 
             * for this CSI.
             */

            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
	        clprintf(CL_LOG_SEV_INFO,"csa201: New state is not the ACTIVE; deactivating service\n");
        	running = 0;

            /*
             * ---END_APPLICATION_CODE---
             */

            saAmfResponse(cpmHandle, invocation, SA_AIS_OK);
            break;
        }

        case SA_AMF_HA_QUIESCED:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active or quiescing HA state. The application 
             * must stop work associated with the CSI immediately.
             */

            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
            clprintf(CL_LOG_SEV_INFO,"csa201: Acknowledging new state\n");

            /*
             * ---END_APPLICATION_CODE---
             */

            saAmfResponse(cpmHandle, invocation, SA_AIS_OK);
            break;
        }

        case SA_AMF_HA_QUIESCING:
        {
            /*
             * AMF has requested application to quiesce the CSI currently
             * assigned the active HA state. The application must stop work
             * associated with the CSI gracefully and not accept any new
             * workloads while the work is being terminated.
             */

            /*
             * ---BEGIN_APPLICATION_CODE---
             */

            // ...
            clprintf(CL_LOG_SEV_INFO,"csa201: Signaling completion of QUIESCING\n");


            /*
             * ---END_APPLICATION_CODE---
             */

            saAmfCSIQuiescingComplete(cpmHandle, invocation, SA_AIS_OK);
            break;
        }

        default:
        {
            /*
             * Should never happen. Ignore.
             */
        }
    }

    return ;
}

/*
 * clCompAppAMFCSIRemove
 * ---------------------
 * This function is invoked when a CSI assignment is to be removed.
 */

void
clCompAppAMFCSIRemove(
    SaInvocationT       invocation,
    const SaNameT       *compName,
    const SaNameT       *csiName,
    SaAmfCSIFlagsT      csiFlags)
{
    /*
     * Print information about the CSI Remove
     */

    clprintf (CL_LOG_SEV_INFO, "Component [%s] : PID [%ld]. CSI Remove Received\n", 
              compName->value, mypid);

    clprintf (CL_LOG_SEV_INFO, "   CSI                     : %s\n", csiName->value);
    clprintf (CL_LOG_SEV_INFO, "   CSI Flags               : 0x%d\n", csiFlags);

    /*
     * Add application specific logic for removing the work for this CSI.
     */

    /*
     * ---BEGIN_APPLICATION_CODE---
     */

    // ...

    /*
     * ---END_APPLICATION_CODE---
     */

    saAmfResponse(cpmHandle, invocation, SA_AIS_OK);
    return ;
}

/******************************************************************************
 * Utility functions 
 *****************************************************************************/

/*
 * clCompAppAMFPrintCSI
 * --------------------
 * Print information received in a CSI set request.
 */

ClRcT
clCompAppAMFPrintCSI(
    SaAmfCSIDescriptorT csiDescriptor,
    SaAmfHAStateT haState)
{
    clprintf (CL_LOG_SEV_INFO, "   CSI                     : %s\n", 
            csiDescriptor.csiName.value);
    clprintf (CL_LOG_SEV_INFO, "   HA State                : %s\n",
            STRING_HA_STATE(haState));
    clprintf (CL_LOG_SEV_INFO, "   CSI Flags               : 0x%d\n",
            csiDescriptor.csiFlags);
    clprintf (CL_LOG_SEV_INFO, "   Active Descriptor       : \n");
    clprintf (CL_LOG_SEV_INFO, "       Active Component    : %s\n",
        csiDescriptor.csiStateDescriptor.activeDescriptor.activeCompName.value);
    clprintf (CL_LOG_SEV_INFO, "   Standby Descriptor      : \n");
    clprintf (CL_LOG_SEV_INFO, "       Standby Rank        : %d\n",
        csiDescriptor.csiStateDescriptor.standbyDescriptor.standbyRank);
    clprintf (CL_LOG_SEV_INFO, "       Active Component    : %s\n",
        csiDescriptor.csiStateDescriptor.standbyDescriptor.activeCompName.value);

    clprintf (CL_LOG_SEV_INFO, "   Name Value Pairs        : \n");
    for (ClUint32T i = 0; i < csiDescriptor.csiAttr.number; i++)
    {
/*        clprintf (CL_LOG_SEV_INFO, "       Name            : %s\n",
                csiDescriptor.csiAttr.attr[i].attrName);
        clprintf (CL_LOG_SEV_INFO, "       Value           : %s\n",
                csiDescriptor.csiAttr.attr[i].attrValue);*/
    }

    return CL_OK;
}

