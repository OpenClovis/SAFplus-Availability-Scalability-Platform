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
 * ModuleName  : txn                                                           
 * File        : clTxnAgentIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  This file defines data-structure and internal functions of transaction-agents.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_AGENT_IPI_H
#define _CL_TXN_AGENT_IPI_H

#include <clCommon.h>
#include <clCntApi.h>

#include <clTxnAgentApi.h>
#include <clTxnCommonDefn.h>
#include <clTxnDb.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
#define CL_TXN_AGENT_NO_SERVICE_REGD    0x0
#define CL_TXN_AGENT_SERVICE_2PC        0x1 
#define CL_TXN_AGENT_SERVICE_1PC        0x2 

#define IS_SERVICE_1PC_CAPABLE(pService)    ( (pService)->serviceCapability == CL_TXN_AGENT_SERVICE_1PC )

#define IS_SERVICE_2PC_CAPABLE(pService)    ( (pService)->serviceCapability == CL_TXN_AGENT_SERVICE_2PC )

#define IS_MATCHING_SERVICE(pService, jobService)                           \
        ( ( (pService)->serviceType == CL_TXN_SERVICE_TYPE_WILDCARD) ||     \
          ( (jobService) == CL_TXN_SERVICE_TYPE_WILDCARD) ||                \
          ( (pService)->serviceType == (jobService) ) )

#define     CL_TXN_AGT_CLI_DISP_JOB_LIST_HEADER  \
                            "   TxnId\t|   JobId\t|  Job-Status\n"
#define     CL_TXN_AGT_CLI_DISP_JOB_LIST_HEADER_SIZE \
                            (strlen(CL_TXN_AGT_CLI_DISP_JOB_LIST_HEADER))


#define     CL_TXN_AGT_CLI_DISP_SHORT_SEP        \
                            "----------------------------------------------------------------\n"
#define     CL_TXN_AGT_CLI_DISP_SHORT_SEP_SIZE   \
                            (strlen(CL_TXN_AGT_CLI_DISP_SHORT_SEP))

#define     CL_TXN_AGT_CLI_DISP_SRC_INFO_HEADER  \
                            "   Service-Id\t|    Capability\n"
#define     CL_TXN_AGT_CLI_DISP_SRC_INFO_HEADER_SIZE \
                            (strlen(CL_TXN_AGT_CLI_DISP_SRC_INFO_HEADER))

#define     CL_TXN_AGT_CLI_DISP_NEWLINE  \
                            "\n\n\n"
#define     CL_TXN_AGT_CLI_DISP_NEWLINE_SIZE \
                            (strlen(CL_TXN_AGT_CLI_DISP_NEWLINE))



/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/**
 * Agent uses following structure to maintain information regarding 
 * individual services of component.
 */
typedef struct
{
    ClInt32T                serviceType;
    ClTxnAgentCallbacksT    *pCompCallbacks;
    ClUint8T                serviceCapability;
/* Define any additional configuration declared by component
       during registration.
    */
} ClTxnAgentCompServiceInfoT;


typedef struct
{
    ClTxnDbHandleT          activeTxnMap;   /* Hash-map of active txn */
    ClCntHandleT            compServiceMap; /* Hash-map of service registration */
    ClUint8T                agentCapability;
    ClOsalMutexIdT			actMtx; 
} ClTxnAgentCfgT;

/**
 * Common command definition used by agent.
 */
typedef struct txnCommand
{
    ClTxnDefnT              *pTxnDefn;
    ClTxnAppJobDefnT        *pTxnJobDefn;
/*    ClTxnMgmtCommandT       command;*/
    ClRcT                   retValue;
} ClTxnOperationT;

/**
 * Enum to indicate start, stop 
 */
typedef enum
{
    CL_TXN_DEFAULT = -1,
    CL_TXN_PHASE   = 0,
    CL_TXN_START   = 1,
    CL_TXN_STOP    = 2,
    CL_TXN_END     
}ClTxnStartStopT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 * This function processes an active transaction-job when a control msg recived 
 * from transaction-manager. 
 * Based on control msg received from mgr, it calls respective service implementation
 * as registered.
 */
extern ClRcT clTxnAgentProcessJob(
        CL_IN   ClTxnMessageHeaderT     *pMsgHdr,
        CL_IN   ClTxnCmdT               tCmd,
        CL_IN   ClBufferHandleT         outMsgHdl,
        CL_OUT  ClTxnCommHandleT        *pCommHandle);

extern ClRcT clTxnAgentTxnDefnReceive(
        CL_IN   ClTxnCmdT               tCmd, 
        CL_IN   ClBufferHandleT  jobDefnMsg);

extern ClRcT clTxnAgentReadJob(
        CL_IN       ClTxnCmdT               tCmd, 
        CL_IN       ClBufferHandleT         inJobDefnMsg,
        CL_INOUT    ClTxnCommHandleT        pCommHandle,
        CL_IN       ClTxnStartStopT         startstop);

extern ClRcT clTxnAgentTxnDefnRemove(
        CL_IN   ClTxnCmdT           txnCmd);

extern ClRcT _clTxnAgentTxnStart(
        CL_IN ClTxnCmdT tCmd);

extern ClRcT _clTxnAgentTxnStop(
        CL_IN ClTxnCmdT tCmd);

extern ClRcT  clTxnAgentTableRegister(
        CL_IN ClEoExecutionObjT *pThis);

#ifdef __cplusplus
}
#endif

#endif /* _CL_TXN_AGENT_IPI_H */

