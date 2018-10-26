/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/templates/default/clSnmpAppMain.h $
 * $Author: pushparaj $
 * $Date: 2007/05/17 $
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

#ifndef CL_SNMP_APP_MAIN
#define CL_SNMP_APP_MAIN

#include "clCompCfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EO_NAME "snmpSubAgent"
#define REL_CODE    'B'
#define MAJOR_VER   0x01
#define MINOR_VER   0x01

/*
 * if ClEoApplicationTypeT set to CL_EO_USE_THREAD_FOR_APP, then appInitialize
 * should block. and  when appFinalize is called, application should do its
 * cleanup and unblock the main thread
 *
 * If set ClEoApplicationTypeT to CL_EO_USE_THREAD_FOR_APP, then appInitialize
 * whould do application Initialize and return. eoRecvLoop will be called from
 * main, once the application initialize returns
 */
ClRcT   snmpInitialize(ClUint32T argc, ClCharT *argv[]);
                                                                                                                             
/*
 * This function should cleanup the application
 * If it is using main thread, then it should return
 */
ClRcT   snmpFinalize();
                                                                                                                             
/*
 * This function is required to change the application state mainly to
 * suspend/Resume its operation
 */
ClRcT   snmpStateChange(ClEoStateT eoState);


/*
 * This function will be called periodically to check the EO health. User need
 * to fill in the structure as per it health
 */
                                                                                                                             
ClRcT   snmpHealthCheck(ClEoSchedFeedBackT* schFeedback);

/*
 * clCompAppTerminate
 * ------------------
 * This function is invoked when the application is to be terminated.
 */
void appTerminate(SaInvocationT invocation, const SaNameT  *compName);

/*
 * clCompAppAMFCSISet
 * ------------------
 * This function is invoked when a CSI assignment is made or the state
 * of a CSI is changed.
 */
void clSnmpAgentAMFCSISet(
        SaInvocationT         invocation,
        const SaNameT         *compName,
        SaAmfHAStateT         haState,
        SaAmfCSIDescriptorT   csiDescriptor);

/*
 * clCompAppAMFCSIRemove
 * ---------------------
 * This function is invoked when a CSI assignment is to be removed.
 */
void
clSnmpAgentAMFCSIRmv(
        SaInvocationT         invocation,
        const SaNameT         *compName,
        const SaNameT         *csiName,
        SaAmfCSIFlagsT        csiFlags);

/*
 * snmpInit
 * DESCRIPTION:  Inits the SNMP.
 * @Params:
 * @Return:
 */
ClRcT snmpInit(void);

extern void clovisSnmpMain(void );
#define AGENT 1

#ifdef __cplusplus
}
#endif

#endif
