/*******************************************************************************
 * ModuleName  : com
 * File        : clCompAppMain.h
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

/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/

ClRcT 
clCompAppInitialize(
        ClUint32T           argc,
        ClCharT             *argv[]);

ClRcT
clCompAppFinalize();

ClRcT
clCompAppStateChange(
        ClEoStateT          eoState);

ClRcT
clCompAppHealthCheck(
        ClEoSchedFeedBackT* schFeedback);

void 
clCompAppTerminate(
        SaInvocationT       invocation,
        const SaNameT       *compName);

/******************************************************************************
 * Application Work Assignment Functions
 *****************************************************************************/

void 
clCompAppAMFCSISet(
        SaInvocationT       invocation,
        const SaNameT       *compName,
        SaAmfHAStateT       haState,
        SaAmfCSIDescriptorT csiDescriptor);

void 
clCompAppAMFCSIRemove(
        SaInvocationT       invocation,
        const SaNameT       *compName,
        const SaNameT       *csiName,
        SaAmfCSIFlagsT      csiFlags);

/******************************************************************************
 * Utility functions 
 *****************************************************************************/

ClRcT
clCompAppAMFPrintCSI(
    SaAmfCSIDescriptorT csiDescriptor,
    SaAmfHAStateT haState);
#ifdef __cplusplus
}
#endif

#endif // CL_COMP_APP_MAIN
