/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/templates/default/clProxyCompAppMain.h $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
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
                                                                                                                             

#ifndef CL_COMP_APP_MAIN
#define CL_COMP_APP_MAIN

#ifdef __cplusplus
extern "C" {
#endif

#include "./clCompCfg.h"

#ifndef COMP_NAME
#error "COMP_NAME is not defined. Bad or missing ./clCompCfg.h"
#endif

/******************************************************************************
 * Utility macros
 *****************************************************************************/
                                                                                                                             
#define STRING_HA_STATE(S)                                                  \
(   ((S) == CL_AMS_HA_STATE_ACTIVE)             ? "Active" :                \
    ((S) == CL_AMS_HA_STATE_STANDBY)            ? "Standby" :               \
    ((S) == CL_AMS_HA_STATE_QUIESCED)           ? "Quiesced" :              \
    ((S) == CL_AMS_HA_STATE_QUIESCING)          ? "Quiescing" :             \
    ((S) == CL_AMS_HA_STATE_NONE)               ? "None" :                  \
                                                  "Unknown" )

#define STRING_CSI_FLAGS(S)                                                 \
(   ((S) == CL_AMS_CSI_FLAG_ADD_ONE)            ? "Add One" :               \
    ((S) == CL_AMS_CSI_FLAG_TARGET_ONE)         ? "Target One" :            \
    ((S) == CL_AMS_CSI_FLAG_TARGET_ALL)         ? "Target All" :            \
                                                  "Unknown" )

ClRcT clCompAppTerminate(ClInvocationT invocation, const ClNameT  *compName);

ClRcT clCompAppAMFCSISet( ClInvocationT         invocation,
        const ClNameT         *compName,
        ClAmsHAStateT         haState,
        ClAmsCSIDescriptorT   csiDescriptor);

ClRcT clCompAppAMFCSIRemove( ClInvocationT         invocation,
        const ClNameT         *compName,
        const ClNameT         *csiName,
        ClAmsCSIFlagsT        csiFlags);

ClRcT clCompAppInitialize(ClUint32T argc,  ClCharT *argv[]);
ClRcT clCompAppFinalize();

ClRcT   clCompAppStateChange(ClEoStateT eoState);

ClRcT   clCompAppHealthCheck(ClEoSchedFeedBackT* schFeedback);

ClRcT clProxiedCompInstantiate(
    ClInvocationT       invocation,
    const ClNameT       *proxiedCompName);

ClRcT clProxiedCompCleanup(
    ClInvocationT       invocation,
    const ClNameT       *proxiedCompName);

/******************************************************************************
 * Utility functions
 *****************************************************************************/
                                                                                                                             
ClRcT
clCompAppAMFPrintCSI(
    ClAmsCSIDescriptorT csiDescriptor,
    ClAmsHAStateT haState);
#ifdef __cplusplus
}
#endif
#endif
