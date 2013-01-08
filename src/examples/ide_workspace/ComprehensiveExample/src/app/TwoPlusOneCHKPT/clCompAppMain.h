/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/templates/default/clCompAppMain.h $
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
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include "./clCompCfg.h"

/*
 * ---BEGIN_APPLICATION_CODE---
 */
 
// add extra includes here...

/*
 * ---END_APPLICATION_CODE---
 */

#ifndef COMP_NAME
#error "COMP_NAME is not defined. Bad or missing ./clCompCfg.h"
#endif

/******************************************************************************
 * Utility macros
 *****************************************************************************/

#define STRING_HA_STATE(S)                                                  \
(   ((S) == SA_AMF_HA_ACTIVE)             ? "Active" :                \
    ((S) == SA_AMF_HA_STANDBY)            ? "Standby" :               \
    ((S) == SA_AMF_HA_QUIESCED)           ? "Quiesced" :              \
    ((S) == SA_AMF_HA_QUIESCING)          ? "Quiescing" :             \
                                            "Unknown" )

#define STRING_CSI_FLAGS(S)                                                 \
(   ((S) & SA_AMF_CSI_ADD_ONE)            ? "Add One" :               \
    ((S) & SA_AMF_CSI_TARGET_ONE)         ? "Target One" :            \
    ((S) & SA_AMF_CSI_TARGET_ALL)         ? "Target All" :            \
                                                  "Unknown" )

/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/

ClRcT clCompAppInitialize(ClUint32T argc, ClCharT *argv[]);

ClRcT clCompAppFinalize();

ClRcT clCompAppStateChange(ClEoStateT eoState);

ClRcT clCompAppHealthCheck(ClEoSchedFeedBackT *schFeedback);

void clCompAppTerminate(SaInvocationT invocation, const SaNameT *compName);

/******************************************************************************
 * Application Work Assignment Functions
 *****************************************************************************/

void clCompAppAMFCSISet(SaInvocationT       invocation,
                        const SaNameT       *compName,
                        SaAmfHAStateT       haState,
                        SaAmfCSIDescriptorT csiDescriptor);

void clCompAppAMFCSIRemove(SaInvocationT  invocation,
                           const SaNameT  *compName,
                           const SaNameT  *csiName,
                           SaAmfCSIFlagsT csiFlags);

/******************************************************************************
 * Utility functions 
 *****************************************************************************/

void clCompAppAMFPrintCSI(SaAmfCSIDescriptorT csiDescriptor,
                          SaAmfHAStateT haState);

/*
 * ---BEGIN_APPLICATION_CODE---
 */
 
// add custom functions here...

/*
 * ---END_APPLICATION_CODE---
 */

#ifdef __cplusplus
}
#endif

#endif // CL_COMP_APP_MAIN
