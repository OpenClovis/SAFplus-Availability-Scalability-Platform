/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : amf
 * File        : clAmsUtils.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains utility definitions required by AMS.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_UTILS_H_
#define _CL_AMS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clAmsErrors.h>
#include <clAmsMgmtCommon.h>

/******************************************************************************
 * Debug Defines
 *****************************************************************************/

extern char *clAmsFormatMsg(const char *fmt, ...);
extern void clAmsLogMsgClient( const ClUint32T level,  char *buffer);

/******************************************************************************
 * Common Error Checking Defines
 *****************************************************************************/

#define AMS_CLIENT_LOG(LEVEL, MSG)              \
{                                               \
  clAmsLogMsgClient( LEVEL, clAmsFormatMsg MSG );    \
}

#ifndef AMS_SERVER
#define AMS_LOG(LEVEL, MSG) AMS_CLIENT_LOG(LEVEL,MSG)
#endif

#define AMS_CHECK_BAD_SANAME(name)                          \
{                                                           \
    if ( (name).length > CL_MAX_NAME_LENGTH )               \
    {                                                       \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                      \
            ("ALERT [%s:%d] : Invalid SaNameT structure\n", \
             __FUNCTION__, __LINE__));                      \
        rc = CL_ERR_BUFFER_OVERRUN;                         \
        goto exitfn;                                        \
    }                                                       \
}

#define AMS_CHECK_ENTITY_TYPE(type)                             \
{                                                               \
    if ( (type) > CL_AMS_ENTITY_TYPE_MAX )                      \
    {                                                           \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                          \
                ("ERROR: Invalid entity type = %d\n", type));   \
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);            \
    }                                                           \
}

#define AMS_CHECK_ENTITY_TYPE_AND_EXIT(type)                    \
{                                                               \
    if ( (type) > CL_AMS_ENTITY_TYPE_MAX )                      \
    {                                                           \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                          \
                ("ERROR: Invalid entity type = %d\n", type));   \
        rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);             \
        goto exitfn;                                            \
    }                                                           \
}

#define AMS_CHECK_RC_ERROR(fn)                  \
{                                               \
    rc = (fn);                                  \
                                                \
    if ( (rc) != CL_OK )                        \
    {                                           \
        goto exitfn;                            \
    }                                           \
}

#define AMS_CHECK_RC_UNLOCK(fn)                  do {   \
    rc = (fn);                                          \
    if ( (rc) != CL_OK )                                \
    {                                                   \
        goto out_unlock;                                \
    }                                                   \
}while(0)

#define AMS_CHECKPTR_SILENT(x)                  \
{                                               \
    if ( (x) != CL_FALSE )                      \
    {                                           \
        return CL_AMS_RC(CL_ERR_NULL_POINTER);  \
    }                                           \
}

#define AMS_CHECK_NO_MEMORY(x)                                          \
{                                                                       \
    if ( (x) == NULL )                                                  \
    {                                                                   \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expression (%s) is True. No Memory\n",    \
             __FUNCTION__, __LINE__, #x));                              \
        return CL_AMS_RC(CL_ERR_NO_MEMORY);                             \
    }                                                                   \
}

#define AMS_CHECK_NO_MEMORY_AND_EXIT(x)                                 \
{                                                                       \
    if ( (x) == NULL )                                                  \
    {                                                                   \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expression (%s) is True. No Memory\n",    \
             __FUNCTION__, __LINE__, #x));                              \
        rc =  CL_ERR_NO_MEMORY;                                         \
        goto exitfn;                                                    \
    }                                                                   \
}

#define AMS_CHECKPTR_AND_EXIT(x)                                        \
{                                                                       \
    if ( (x) != CL_FALSE )                                              \
    {                                                                   \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expression (%s) is True. Null Pointer\n", \
             __FUNCTION__, __LINE__, #x));                              \
        rc = CL_ERR_NULL_POINTER;                                       \
        goto exitfn;                                                    \
    }                                                                   \
}

#define AMS_CHECKPTR(x)                                                 \
{                                                                       \
    if ( (x) != CL_FALSE )                                              \
    {                                                                   \
        AMS_CLIENT_LOG(CL_LOG_SEV_ERROR,                                  \
            ("ALERT [%s:%d] : Expression (%s) is True. Null Pointer\n", \
             __FUNCTION__, __LINE__, #x));                              \
        return CL_AMS_RC(CL_ERR_NULL_POINTER);                          \
    }                                                                   \
}

/******************************************************************************
 * Utility Functions
 *****************************************************************************/

#define AMS_MIN(x,y) ( (x) < (y) ) ? (x) : (y);
#define AMS_MAX(x,y) ( (x) > (y) ) ? (x) : (y);

#define clAmsFreeMemory(mPtr)                   \
{                                               \
    if ( (mPtr) !=NULL )                        \
    {                                           \
        clHeapFree(mPtr);                       \
    }                                           \
    mPtr = NULL;                                \
}
 
/******************************************************************************
 * Printing Related Defines
 *****************************************************************************/

#define CL_AMS_COLUMN_1_WIDTH       45
#define CL_AMS_COLUMN_2_WIDTH       35
#define CL_AMS_COLUMN_1_DELIMITER   "--------------------------------------"
#define CL_AMS_COLUMN_2_DELIMITER   "---------------------------------"
#define CL_AMS_DELIMITER            \
 "==========================================================================="

#define CL_AMS_PRINT_TWO_COL(A,B,C)                                 \
{                                                                   \
    clOsalPrintf("%-*s | ", CL_AMS_COLUMN_1_WIDTH, A);              \
    clOsalPrintf(B, C);                                             \
    clOsalPrintf("\n");                                             \
    if ( debugPrintFP != NULL )                                     \
    {                                                               \
        fprintf(debugPrintFP,"%-*s | ", CL_AMS_COLUMN_1_WIDTH, A);  \
        fprintf(debugPrintFP,B, C);                                 \
        fprintf(debugPrintFP,"\n");                                 \
    }                                                               \
}

#define CL_AMS_PRINT_DELIMITER()                        \
{                                                       \
    clOsalPrintf("%s\n", CL_AMS_DELIMITER);             \
    if ( debugPrintFP != NULL )                         \
    {                                                   \
        fprintf(debugPrintFP,"%s\n", CL_AMS_DELIMITER); \
    }                                                   \
}

#define CL_AMS_PRINT_EMPTY_LINE()               \
{                                               \
    clOsalPrintf("\n\n");                       \
    if ( debugPrintFP != NULL )                 \
    {                                           \
        fprintf(debugPrintFP,"\n\n");           \
    }                                           \
}


#define CL_AMS_PRINT_SUMMARY    1
#define CL_AMS_PRINT_DETAILS    2

#define CL_AMS_PRINT_HEADER(TITLE,FORMAT,STRING)    \
{                                                   \
    CL_AMS_PRINT_DELIMITER();                       \
    CL_AMS_PRINT_TWO_COL(TITLE, FORMAT, STRING);    \
    CL_AMS_PRINT_DELIMITER();                       \
}

#define CL_AMS_PRINT_OPEN_TAG(tag)              \
{                                               \
    fprintf(debugPrintFP, "<%s>\n", tag);       \
}

#define CL_AMS_PRINT_CLOSE_TAG(tag)             \
{                                               \
    fprintf(debugPrintFP, "</%s>\n", tag);      \
}

#define CL_AMS_PRINT_TAG_ATTR(tag, s, value)                    \
{                                                               \
    fprintf(debugPrintFP, "<%s value=\""s"\"/>\n", tag, value); \
}

#define CL_AMS_PRINT_TAG_VALUE(tag, s, value)                   \
{                                                               \
    fprintf(debugPrintFP, "<%s>"s"</%s>\n", tag, value, tag);   \
}

#define CL_AMS_PRINT_OPEN_TAG_ATTR(tag, s, value)               \
{                                                               \
    fprintf(debugPrintFP, "<%s value=\""s"\">\n", tag, value);  \
}

/******************************************************************************
 * Strings for AMS Types
 *****************************************************************************/

#define CL_AMS_STRING_BOOLEAN(S)                ( (S) ? "True" : "False" )

#define CL_AMS_STRING_SERVICE_STATE(S)                                  \
(   ((S) == CL_AMS_SERVICE_STATE_RUNNING ) ?    "Running" :             \
    ((S) == CL_AMS_SERVICE_STATE_STOPPED)         ? "Stopped" :         \
    ((S) == CL_AMS_SERVICE_STATE_STARTINGUP)      ? "Starting Up" :     \
    ((S) == CL_AMS_SERVICE_STATE_SHUTTINGDOWN)    ? "Shutting Down" :   \
    ((S) == CL_AMS_SERVICE_STATE_UNAVAILABLE)     ? "Unavailable" :     \
    ((S) == CL_AMS_SERVICE_STATE_HOT_STANDBY)     ? "Hot standby" :     \
    ((S) == CL_AMS_SERVICE_STATE_NONE)            ? "None" :            \
                                                  "Unknown" )

#define CL_AMS_STRING_INSTANTIATE_MODE(S)                           \
(   ((S)&CL_AMS_INSTANTIATE_MODE_ACTIVE)       ? "Active Mode" :    \
    ((S)&CL_AMS_INSTANTIATE_MODE_STANDBY)      ? "Standby Mode":    \
                                                  "Unknown" )

#define CL_AMS_STRING_A_STATE(S)                                        \
(   ((S) == CL_AMS_ADMIN_STATE_UNLOCKED)        ? "Unlocked" :          \
    ((S) == CL_AMS_ADMIN_STATE_LOCKED_A)        ? "Locked Assignment" : \
    ((S) == CL_AMS_ADMIN_STATE_LOCKED_I)        ? "Locked Instantiation" : \
    ((S) == CL_AMS_ADMIN_STATE_SHUTTINGDOWN)    ? "Shutting Down" :     \
    ((S) == CL_AMS_ADMIN_STATE_SHUTTINGDOWN_RESTART)    ? "Shutting Down with Restart" :     \
    ((S) == CL_AMS_ADMIN_STATE_NONE)            ? "None" :              \
                                                  "Unknown" )

#define CL_AMS_STRING_O_STATE(S)                                            \
(   ((S) == CL_AMS_OPER_STATE_ENABLED)          ? "Enabled" :               \
    ((S) == CL_AMS_OPER_STATE_DISABLED)         ? "Disabled" :              \
    ((S) == CL_AMS_OPER_STATE_NONE)             ? "None" :                  \
                                                  "Unknown" )

#define CL_AMS_STRING_P_STATE(S)                                        \
(   ((S) == CL_AMS_PRESENCE_STATE_UNINSTANTIATED) ? "Uninstantiated" :  \
    ((S) == CL_AMS_PRESENCE_STATE_INSTANTIATING)  ? "Instantiating" :   \
    ((S) == CL_AMS_PRESENCE_STATE_INSTANTIATED)   ? "Instantiated" :    \
    ((S) == CL_AMS_PRESENCE_STATE_TERMINATING)    ? "Terminating" :     \
    ((S) == CL_AMS_PRESENCE_STATE_RESTARTING)     ? "Restarting" :      \
    ((S) == CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) ? "Instantiation Failed" : \
    ((S) == CL_AMS_PRESENCE_STATE_TERMINATION_FAILED) ? "Termination Failed" : \
    ((S) == CL_AMS_PRESENCE_STATE_FAULT)          ? "Fault" :           \
    ((S) == CL_AMS_PRESENCE_STATE_FAULT_WTR)      ? "Fault WTR" :       \
    ((S) == CL_AMS_PRESENCE_STATE_FAULT_WTC)      ? "Fault WTC" :       \
    ((S) == CL_AMS_PRESENCE_STATE_NONE)           ? "None" :            \
                                                    "Unknown" )

#define CL_AMS_STRING_R_STATE(S)                                            \
(   ((S) == CL_AMS_READINESS_STATE_INSERVICE)   ? "In Service" :            \
    ((S) == CL_AMS_READINESS_STATE_STOPPING)    ? "Stopping" :              \
    ((S) == CL_AMS_READINESS_STATE_OUTOFSERVICE)? "Out of Service" :        \
    ((S) == CL_AMS_READINESS_STATE_NONE)        ? "None" :                  \
                                                  "Unknown" )

#define CL_AMS_STRING_H_STATE(S)                                            \
(   ((ClAmsHAStateT)(S) == CL_AMS_HA_STATE_ACTIVE)             ? "Active" :                \
    ((ClAmsHAStateT)(S) == CL_AMS_HA_STATE_STANDBY)            ? "Standby" :               \
    ((ClAmsHAStateT)(S) == CL_AMS_HA_STATE_QUIESCED)           ? "Quiesced" :              \
    ((ClAmsHAStateT)(S) == CL_AMS_HA_STATE_QUIESCING)          ? "Quiescing" :             \
    ((ClAmsHAStateT)(S) == CL_AMS_HA_STATE_NONE)               ? "None" :                  \
                                                  "Unknown" )

#define CL_AMS_STRING_TIMER(S)                                          \
(   ((S) == CL_AMS_NODE_TIMER_SUFAILOVER)            ? "Node-SUFailover" : \
    ((S) == CL_AMS_SG_TIMER_INSTANTIATE)             ? "SG-Instantiate" : \
    ((S) == CL_AMS_SG_TIMER_ADJUST)                  ? "SG-Adjust" :         \
    ((S) == CL_AMS_SG_TIMER_ADJUST_PROBATION)        ? "SG-Adjust-Probation" :    \
    ((S) == CL_AMS_SU_TIMER_SURESTART)               ? "SU-SURestart" : \
    ((S) == CL_AMS_SU_TIMER_PROBATION)               ? "SU-SUProbation" : \
    ((S) == CL_AMS_SU_TIMER_COMPRESTART)             ? "SU-CompRestart" : \
    ((S) == CL_AMS_SU_TIMER_ASSIGNMENT)             ? "SU-Assignment-Delay" : \
    ((S) == CL_AMS_COMP_TIMER_INSTANTIATE)           ? "Comp-Instantiate" : \
    ((S) == CL_AMS_COMP_TIMER_INSTANTIATEDELAY)      ? "Comp-InstantiateDelay" : \
    ((S) == CL_AMS_COMP_TIMER_TERMINATE)             ? "Comp-Terminate" : \
    ((S) == CL_AMS_COMP_TIMER_CLEANUP)               ? "Comp-Cleanup" : \
    ((S) == CL_AMS_COMP_TIMER_AMSTART)               ? "Comp-AMStart" : \
    ((S) == CL_AMS_COMP_TIMER_AMSTOP)                ? "Comp-AMStop" :  \
    ((S) == CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE)     ? "Comp-QuiescingComplete" : \
    ((S) == CL_AMS_COMP_TIMER_CSISET)                ? "Comp-CSISet" :  \
    ((S) == CL_AMS_COMP_TIMER_CSIREMOVE)             ? "Comp-CSIRemove" : \
    ((S) == CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE)? "Comp-ProxiedCompInstantiate": \
    ((S) == CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP)    ? "Comp-ProxiedCompCleanup": \
                                                  "Unknown" )

#define CL_AMS_STRING_NODE_CLASSTYPE(S)                                     \
(   ((S) == CL_AMS_NODE_CLASS_A)                ? "Class A" :               \
    ((S) == CL_AMS_NODE_CLASS_B)                ? "Class B" :               \
    ((S) == CL_AMS_NODE_CLASS_C)                ? "Class C" :               \
    ((S) == CL_AMS_NODE_CLASS_D)                ? "Class D" :               \
    ((S) == CL_AMS_NODE_CLASS_NONE)             ? "None" :                  \
                                                  "Unknown" )

#define CL_AMS_STRING_NODE_ISCLUSTERMEMBER(S)                               \
(   ((S) == CL_AMS_NODE_IS_CLUSTER_MEMBER)      ? "True" :                  \
    ((S) == CL_AMS_NODE_IS_LEAVING_CLUSTER)     ? "Leaving" :               \
                                                  "False" )

#define CL_AMS_STRING_SG_REDUNDANCY_MODEL(S)                                \
(   ((S) == CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY) ? "No Redundancy" :   \
    ((S) == CL_AMS_SG_REDUNDANCY_MODEL_TWO_N)         ? "2N (1+1)" :        \
    ((S) == CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N)      ? "M + N" :           \
    ((S) == CL_AMS_SG_REDUNDANCY_MODEL_N_WAY)         ? "N-Way" :           \
    ((S) == CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE)  ? "N-Way-Active" :    \
    ((S) == CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM)        ? "CUSTOM" : \
                                                        "Unknown" )

#define CL_AMS_STRING_SG_LOADING_STRATEGY(S)                                      \
(   ((S) == CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU)   ? "Least SI per SU" :   \
    ((S) == CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED) ? "Least SU Assigned" : \
    ((S) == CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU) ? "Least Load per SU" : \
    ((S) == CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE)  ? "By SI Perference" :  \
    ((S) == CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED)      ? "User Defined" :      \
                                                            "Unknown" )

#define CL_AMS_STRING_RECOVERY(S)                                                   \
(   ((S) == CL_AMS_RECOVERY_NO_RECOMMENDATION)    ? "No Recommendation" :           \
    ((S) == CL_AMS_RECOVERY_COMP_RESTART)         ? "Component Restart" :           \
    ((S) == CL_AMS_RECOVERY_COMP_FAILOVER)        ? "Component Failover" :          \
    ((S) == CL_AMS_RECOVERY_NODE_SWITCHOVER)      ? "Node Switchover" :             \
    ((S) == CL_AMS_RECOVERY_NODE_FAILOVER)        ? "Node Failover" :               \
    ((S) == CL_AMS_RECOVERY_NODE_FAILFAST)        ? "Node Failfast" :               \
    ((S) == CL_AMS_RECOVERY_APP_RESTART)          ? "Application Restart" :         \
    ((S) == CL_AMS_RECOVERY_CLUSTER_RESET)        ? "Cluster Restart" :             \
    ((S) == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ? "Internally Recovered" :        \
    ((S) == CL_AMS_RECOVERY_SU_RESTART)           ? "SU Restart" :                  \
    ((S) == CL_AMS_RECOVERY_NONE)                 ? "None" :                        \
    ((S) == CL_AMS_EXTERNAL_RECOVERY_RESET)       ? "External Component Reset" :    \
    ((S) == CL_AMS_EXTERNAL_RECOVERY_REBOOT)      ? "External Component Reboot" :   \
    ((S) == CL_AMS_EXTERNAL_RECOVERY_POWER_ON)    ? "External Component PowerOn" :  \
    ((S) == CL_AMS_EXTERNAL_RECOVERY_POWER_OFF)   ? "External Component PowerOff" : \
    ((S) == CL_AMS_RECOVERY_NODE_HALT)            ? "Node halt" :                   \
                                                    "Unknown" )

#define CL_AMS_STRING_COMP_PROPERTY(S)                                      \
(   ((S) == CL_AMS_COMP_PROPERTY_SA_AWARE)                          ?       \
                                        "SA Aware" :                        \
    ((S) == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE)           ?       \
                                        "Proxied Preinstantiable":          \
    ((S) == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)       ?       \
                                        "Proxied Non Preinstantiable":      \
    ((S) == CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)   ?       \
                                        "Non Proxied Non Preinstantiable":  \
                                        "Unknown" )

#define CL_AMS_STRING_COMP_CAP(S)                                                    \
(   ((S) == CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY)     ? "X-Active AND Y-Standby" : \
    ((S) == CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY)      ? "X-Active OR Y-Standby" :  \
    ((S) == CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY)    ? "1-Active OR Y-Standby" :  \
    ((S) == CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY)  ? "1-Active OR 1-Standby" :  \
    ((S) == CL_AMS_COMP_CAP_X_ACTIVE)                   ? "X-Active" :               \
    ((S) == CL_AMS_COMP_CAP_ONE_ACTIVE)                 ? "1-Active" :               \
    ((S) == CL_AMS_COMP_CAP_NON_PREINSTANTIABLE)        ? "Non Preinstantiable" :    \
                                                          "Unknown" )

#define CL_AMS_STRING_CSI_FLAGS(S)                                          \
(   ((S) & CL_AMS_CSI_FLAG_ADD_ONE)        ? "ADD_ONE" :                   \
    ((S) & CL_AMS_CSI_FLAG_TARGET_ONE)     ? "TARGET_ONE" :                \
    ((S) & CL_AMS_CSI_FLAG_TARGET_ALL)     ? "TARGET_ALL" :                \
                                              "Unknown" )

#define CL_AMS_STRING_CSI_TRAN_DESCR(S)                                     \
(   ((S) == CL_AMS_CSI_NEW_ASSIGN)        ? "NEW_ASSIGN" :                  \
    ((S) == CL_AMS_CSI_QUIESCED)          ? "CSI_QUIESCED" :                \
    ((S) == CL_AMS_CSI_NOT_QUIESCED)      ? "CSI_NOT_QUIESCED" :            \
    ((S) == CL_AMS_CSI_STILL_ACTIVE)      ? "CSI_STILL_ACTIVE" :            \
                                            "Unknown" )

#define CL_AMS_STRING_ENTITY_TYPE(S)                        \
(   ((S) == CL_AMS_ENTITY_TYPE_ENTITY )      ? "entity" :   \
    ((S) == CL_AMS_ENTITY_TYPE_NODE)         ? "node" :     \
    ((S) == CL_AMS_ENTITY_TYPE_SG)           ? "sg" :       \
    ((S) == CL_AMS_ENTITY_TYPE_SU)           ? "su" :       \
    ((S) == CL_AMS_ENTITY_TYPE_SI)           ? "si" :       \
    ((S) == CL_AMS_ENTITY_TYPE_COMP)         ? "comp" :     \
    ((S) == CL_AMS_ENTITY_TYPE_CSI)          ? "csi" :      \
    ((S) == CL_AMS_ENTITY_TYPE_CLUSTER)      ? "cluster" :  \
                                               "unknown" )

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_UTILS_H_ */
