/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/templates/default/clSnmpAppMain.c $
 * $Author: bkpavan $
 * $Date: 2007/02/21 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
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
#include <signal.h>

/*
 * Basic ASP Includes.
 */

#include <clCommon.h>
#include <clOsalApi.h>
#include <clIocServices.h>

/*
 * ASP Client Includes.
 */

#include <clDebugApi.h>

#include <clEoApi.h>
#include <clCpmApi.h>
#include <clSnmpOp.h>
#include "clAppSubAgentConfig.h"

/*
 * ---BEGIN_APPLICATION_CODE---
 */
 
#include "clSnmpAppMain.h"

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

extern ClRcT idlClientInstall(void);

#endif

/*
 * Subagent configuration file name <AGENT_CONFIG>.conf
 */
#define AGENT_CONFIG "snmp_subagent"

/*
 * This template has a few default clprintfs. These can be disabled by 
 * changing clprintf to a null function
 */

#define clprintf(severity, ...)   clAppLog(CL_LOG_HANDLE_APP, severity, 10, \
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

ClCpmHandleT cpmHandle;

ClEoConfigT clEoConfig = {
    COMP_EO_NAME,       /* EO Name*/
    CL_OSAL_THREAD_PRI_MEDIUM,              /* EO Thread Priority */
    COMP_EO_NUM_THREAD,              /* No of EO thread needed */
    COMP_IOC_PORT,         /* Required Ioc Port */
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV, /* Whether to use main thread for eo Recv or not */
    snmpInitialize,  /* Function CallBack  to initialize the Application */
    snmpFinalize,    /* Function Callback to Terminate the Application */
    snmpStateChange, /* Function Callback to change the Application state */
    snmpHealthCheck, /* Function Callback to change the Application state */
    NULL,
    CL_FALSE,
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
    COMP_EO_CLIENTLIB_GMS       /* Lib: Cluster/Group Membership Service    */
};
                                                                               

typedef void *(*taskStartFunction)(void *);

static ClInt32T snmpAgntInitDone = 0;

extern ClRcT processPortStChange( ClUint32T port, ClUint32T status);

ClRcT   snmpInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClNameT                     appName;
    ClCpmCallbacksT callbacks;
    ClVersionT              version;
    ClIocPortT                      iocPort;
    ClRcT   rc = CL_OK;
                                                                                                                         
/* Do the App intialization */
                                                                                                                         
/*  Do the CPM client init/Register */
    version.releaseCode = REL_CODE;
    version.majorVersion = MAJOR_VER;
    version.minorVersion = MINOR_VER;
                                                                                                                         
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = appTerminate;
    callbacks.appCSISet = clSnmpAgentAMFCSISet;
    callbacks.appCSIRmv = clSnmpAgentAMFCSIRmv;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;
                                                                                                                         
    clEoMyEoIocPortGet(&iocPort);
    clprintf(CL_LOG_SEV_INFO, "Application Address 0x%x Port %x\n", clIocLocalAddressGet(), iocPort);
    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Amf initialization failed, rc=[0x%x]", rc);
        return rc;
    }
    rc = clCpmComponentNameGet(cpmHandle, &appName);
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Component name get failed, rc=[0x%x]", rc);
        return rc;
    }
    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Component register failed, rc=[0x%x]", rc);
        return rc;
    }
    clprintf(CL_LOG_SEV_INFO, "After clCpmClientRegister %x\n", rc);

    snmpInit();
    return CL_OK;
}

ClRcT   snmpFinalize()
{
    ClRcT   rc = CL_OK;
    /* Finalize snmp op */ 
    rc = clSnmpMutexFini();
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Snmp op finalize failed, rc=[0x%x]", rc);
        return rc;
    }
    rc = clMedFinalize(gMedSnmpHdl);
    return rc;
}

ClRcT appTerminate(ClInvocationT invocation,
                        const ClNameT  *compName)
{
    ClRcT rc;
    clprintf(CL_LOG_SEV_INFO, "Inside appTerminate \n");
                                                                                                                             
    clprintf(CL_LOG_SEV_INFO, "Unregister with CPM before Exit ................. %.*s\n", compName->length, compName->value);
    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);
    clprintf(CL_LOG_SEV_INFO, "Finalize before Exit ................. %.*s\n", compName->length, compName->value);
    rc = clCpmClientFinalize(cpmHandle);
    clprintf(CL_LOG_SEV_INFO, "After Finalize ................. %.*s\n", compName->length, compName->value);
    rc = clCpmResponse(cpmHandle, invocation, CL_OK);
    return CL_OK;
}

ClRcT   snmpStateChange(ClEoStateT eoState)
{
    return CL_OK;
}

ClRcT   snmpHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    schFeedback->freq = CL_EO_DONT_POLL;
    schFeedback->status = CL_CPM_EO_ALIVE;
    return CL_OK;   
}

ClRcT
clSnmpAgentAMFCSISet(
        ClInvocationT         invocation,
        const ClNameT         *compName,
        ClAmsHAStateT         haState,
        ClAmsCSIDescriptorT   csiDescriptor)
{
    clprintf(CL_LOG_SEV_INFO, "Inside Function %s \n",__FUNCTION__);
                                                                                                                             
    if(haState == CL_AMS_HA_STATE_QUIESCING)
    {
    /*TODO make the quiescing complete call after the work assigned is done */
        clprintf(CL_LOG_SEV_INFO, "######## before clCpmCSIQuiescingComplete ########\n");
        clCpmCSIQuiescingComplete(cpmHandle, invocation, CL_OK);
    }
    else
    {
        clprintf(CL_LOG_SEV_INFO, "######## before clCpmResponse ########\n");
        clCpmResponse(cpmHandle, invocation, CL_OK);
    }
    return CL_OK;
}


ClRcT
clSnmpAgentAMFCSIRmv(
        ClInvocationT         invocation,
        const ClNameT         *compName,
        const ClNameT         *csiName,
        ClAmsCSIFlagsT        csiFlags)
{
    clprintf(CL_LOG_SEV_INFO, "Inside Function %s \n",__FUNCTION__);
    /*TODO stop the work assigned before making the response done */
                                                                                                                             
    clCpmResponse(cpmHandle, invocation, CL_OK);
                                                                                                                             
    return CL_OK;
}



/*************************************************************************/
/* NAME:  snmpInit                                                        */
/* DESCRIPTION:  Inits the SNMP.                                          */
/* @Params:                                                              */
/* @Return:                                                              */
/*************************************************************************/
ClRcT snmpInit(void)
{
    ClRcT   rc = CL_OK;
    if (snmpAgntInitDone)
        return (CL_ERR_NOT_INITIALIZED);


    /* Add snmpinit function to create mutex */
    rc = clSnmpMutexInit();
    if(CL_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Snmp Init failed, rc=[0x%x]", rc);
        return rc;
    }
    
    clOsalTaskCreateDetached ("clovisSnmpMain", CL_OSAL_SCHED_OTHER, CL_OSAL_THREAD_PRI_NOT_APPLICABLE,
                        65536, (taskStartFunction)clovisSnmpMain,
                        NULL);
    return rc;
}

static int keep_running;

RETSIGTYPE
stop_server(ClInt32T a) {
    keep_running = 0;
}

void clovisSnmpMain(void)
{
    ClInt32T agentx_subagent = AGENT;

    if ( CL_OK != initMM() )
    {
        return;
    }
    clprintf(CL_LOG_SEV_INFO, "\n SNMP Init Done\n");

    snmp_enable_stderrlog();

    /* we're an agentx subagent? */
    /* make us a agentx client. */
    if(agentx_subagent == AGENT)
    {
        netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
        netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_NO_ROOT_ACCESS, 1);
        netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DISABLE_PERSISTENT_LOAD, 1);
    }

    /* initialize the agent library */
    init_agent(AGENT_CONFIG);

    /* initialize mib code here */

    /*Call the entry point for the MIB specific code. Every MIB sub-agent
     * should implement a function called init_appMIB from which it calls the
     * MIB specific init functions generated by mib2c*/
    init_appMIB();  

    /* example-demon will be used to read example-demon.conf files. */
    init_snmp(AGENT_CONFIG);

    /* In case we recevie a request to stop (kill -TERM or kill -INT) */
    keep_running = 1;
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

    /*snmp_log(LOG_INFO,"example-demon is up and running.\n");*/

    /* your main loop here... */
    while(keep_running) {
        /* if you use select(), see snmp_select_info() in snmp_api(3) */
        /*     --- OR ---  */
        agent_check_and_process(1); /* 0 == don't block */
    }
    /* at shutdown time */
    snmp_shutdown(AGENT_CONFIG);
    /*SOCK_CLEANUP;*/
}
