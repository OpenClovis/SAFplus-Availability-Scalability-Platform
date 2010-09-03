/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/templates/default/clSnmpAppMain.c $
 * $Author: bkpavan $
 * $Date: 2007/02/21 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provides a skeleton for writing a SAF aware component.
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
 
#include <signal.h>

/*
 * Basic ASP Includes.
 */

#include <clCommon.h>

/*
 * ASP Client Includes.
 */

#include <clDebugApi.h>

#include <saAmf.h>
#include <clCpmApi.h>
#include <clSnmpOp.h>
#include "clAppSubAgentConfig.h"

#include "clSnmpAppMain.h"


/******************************************************************************
 * Optional Features
 *****************************************************************************/


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


/******************************************************************************
 * Global Variables.
 *****************************************************************************/

SaAmfHandleT amfHandle;

ClBoolT unblockNow = CL_FALSE;

typedef void *(*taskStartFunction)(void *);

static ClInt32T snmpAgntInitDone = 0;

extern ClRcT processPortStChange( ClUint32T port, ClUint32T status);


int main(int argc, char *argv[])
{
    SaNameT                     appName;
    SaAmfCallbacksT             callbacks;
    SaVersionT                  version;
    ClIocPortT                  iocPort;
    ClRcT                       rc = SA_AIS_OK;
    SaSelectionObjectT          dispatch_fd;
    fd_set                      read_fds;
                                                                                                                         
/* Do the App intialization */
                                                                                                                         
/*  Do the CPM client init/Register */
    version.releaseCode = REL_CODE;
    version.majorVersion = MAJOR_VER;
    version.minorVersion = MINOR_VER;
                                                                                                                         
    callbacks.saAmfHealthcheckCallback = NULL;
    callbacks.saAmfComponentTerminateCallback = appTerminate;
    callbacks.saAmfCSISetCallback = clSnmpAgentAMFCSISet;
    callbacks.saAmfCSIRemoveCallback = clSnmpAgentAMFCSIRmv;
    callbacks.saAmfProtectionGroupTrackCallback = NULL;
    callbacks.saAmfProxiedComponentInstantiateCallback = NULL;
    callbacks.saAmfProxiedComponentCleanupCallback = NULL;
                                                                                                                         
    clEoMyEoIocPortGet(&iocPort);
    clprintf(CL_LOG_SEV_INFO, "Application Address 0x%x Port %x\n", clIocLocalAddressGet(), iocPort);
    rc = saAmfInitialize(&amfHandle, &callbacks, &version);
    if(SA_AIS_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Amf intialization failed, rc=[0x%x]", rc);
        return -1;
    }

    FD_ZERO(&read_fds);

    /*
     * Get the AMF dispatch FD for the callbacks
     */
    rc = saAmfSelectionObjectGet(amfHandle, &dispatch_fd); 
    if(SA_AIS_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "saAmfSelectionObjectGet() failed, rc=[0x%x]", rc);
        return -1;
    }
    
    FD_SET(dispatch_fd, &read_fds);

    rc = saAmfComponentNameGet(amfHandle, &appName);
    if(SA_AIS_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Component name get failed, rc=[0x%x]", rc);
        return -1;
    }
    rc = saAmfComponentRegister(amfHandle, &appName, NULL);
    if(SA_AIS_OK != rc)
    {
        clprintf(CL_LOG_SEV_ERROR, "Component register failed, rc=[0x%x]", rc);
        return -1;
    }
    clprintf(CL_LOG_SEV_INFO, "After clCpmClientRegister %x\n", rc);

    snmpInit();

    /*
     * Block on AMF dispatch file descriptor for callbacks
     */
    do
    {
        if( select(dispatch_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            clprintf(CL_LOG_SEV_ERROR, "Error in select()");
            perror("");
            break;
        }
        saAmfDispatch(amfHandle, SA_DISPATCH_ALL);
    }while(!unblockNow);      

    if((rc = saAmfFinalize(amfHandle)) != SA_AIS_OK)
    {
        clprintf(CL_LOG_SEV_ERROR, "AMF finalization error[0x%X]", rc);
    }

    clprintf(CL_LOG_SEV_INFO, "AMF Finalized");

    return 0;
}


void appTerminate(SaInvocationT invocation,
                        const SaNameT  *compName)
{
    SaAisErrorT  rc = SA_AIS_OK;
    clprintf(CL_LOG_SEV_INFO, "Inside appTerminate \n");
                                                                                                                             
    clprintf(CL_LOG_SEV_INFO, "Unregister with CPM before Exit ................. %.*s\n", compName->length, compName->value);
    rc = saAmfComponentUnregister(amfHandle, compName, NULL);
    clprintf(CL_LOG_SEV_INFO, "Finalize before Exit ................. %.*s\n", compName->length, compName->value);

    rc = saAmfResponse(amfHandle, invocation, SA_AIS_OK);

    unblockNow = CL_TRUE;
    return;
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

void
clSnmpAgentAMFCSISet(
        SaInvocationT         invocation,
        const SaNameT         *compName,
        SaAmfHAStateT         haState,
        SaAmfCSIDescriptorT   csiDescriptor)
{
    clprintf(CL_LOG_SEV_INFO, "Inside Function %s \n",__FUNCTION__);
                                                                                                                             
    if(haState == SA_AMF_HA_QUIESCING)
    {
        /*TODO make the quiescing complete call after the work assigned is done */
        clprintf(CL_LOG_SEV_INFO, "######## before saAmfCSIQuiescingComplete ########\n");
        saAmfCSIQuiescingComplete(amfHandle, invocation, SA_AIS_OK);
    }
    else
    {
        clprintf(CL_LOG_SEV_INFO, "######## before saAmfResponse ########\n");
        saAmfResponse(amfHandle, invocation, SA_AIS_OK);
    }
    return;
}


void
clSnmpAgentAMFCSIRmv(
        SaInvocationT         invocation,
        const SaNameT         *compName,
        const SaNameT         *csiName,
        SaAmfCSIFlagsT        csiFlags)
{
    clprintf(CL_LOG_SEV_INFO, "Inside Function %s \n",__FUNCTION__);
    /*TODO stop the work assigned before making the response done */
                                                                                                                             
    saAmfResponse(amfHandle, invocation, SA_AIS_OK);
                                                                                                                             
    return;
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
