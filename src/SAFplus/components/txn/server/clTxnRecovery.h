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
 * File        : clTxnRecovery.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module is the definitions used by transaction-recovery implementation
 *
 *
 *****************************************************************************/


#ifndef _CL_TXN_RECOVERY_H_
#define _CL_TXN_RECOVERY_H_

#include <clCommon.h>

#include <clTxnCommonDefn.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_TXN_ACTIVITY_CMD_SENT        (0x1)
#define CL_TXN_ACTIVITY_APP_RESP_CLOK   (0x2)
#define CL_TXN_ACTIVITY_APP_RESP_ERR    (0x4)
#define CL_TXN_ACTIVITY_APP_TIMEOUT     (0x8)

/* Settings for PREPARE-phase */
#define CL_TXN_PREPARE_CMD_SENT         (CL_TXN_ACTIVITY_CMD_SENT       << 0x0)
#define CL_TXN_PREPARE_APP_RESP_CLOK    (CL_TXN_ACTIVITY_APP_RESP_CLOK  << 0x0)
#define CL_TXN_PREPARE_APP_RESP_ERR     (CL_TXN_ACTIVITY_APP_RESP_ERR   << 0x0)
#define CL_TXN_PREPARE_APP_TIMEOUT      (CL_TXN_ACTIVITY_APP_TIMEOUT    << 0x0)

/* Settings for COMMIT-phase */
#define CL_TXN_COMMIT_CMD_SENT          (CL_TXN_ACTIVITY_CMD_SENT       << 0x4)
#define CL_TXN_COMMIT_APP_RESP_CLOK     (CL_TXN_ACTIVITY_APP_RESP_CLOK  << 0x4)
#define CL_TXN_COMMIT_APP_RESP_ERR      (CL_TXN_ACTIVITY_APP_RESP_ERR   << 0x4)
#define CL_TXN_COMMIT_APP_TIMEOUT       (CL_TXN_ACTIVITY_APP_TIMEOUT    << 0x4)


/* Settings for ROLLBACK-phase */
#define CL_TXN_ROLLBACK_CMD_SENT          (CL_TXN_ACTIVITY_CMD_SENT         << 0x8)
#define CL_TXN_ROLLBACK_APP_RESP_CLOK     (CL_TXN_ACTIVITY_APP_RESP_CLOK    << 0x8)
#define CL_TXN_ROLLBACK_APP_RESP_ERR      (CL_TXN_ACTIVITY_APP_RESP_ERR     << 0x8)
#define CL_TXN_ROLLBACK_APP_TIMEOUT       (CL_TXN_ACTIVITY_APP_TIMEOUT      << 0x8)

#define CL_TXN_VALID_LOG                  (0x1 << 0xC)

#define CL_TXN_RECOVERY_MAX_RETRY           0x2
/* Activity log of agent involved in a transaction-job, indexed by jobId.jobId */
typedef struct {
    ClRcT           appRc;          /* App returned error code */
    ClUint32T       activityLog;    /* Activity log maintained using above definitions */
} ClTxnAgentLogT;

typedef struct {
    ClIocPhysicalAddressT   compAddress;        /* Component address        */
    ClUint32T               compCfg;            /* Component configuration  */
    ClTxnAgentLogT          *pTxnAgentLog;      /* Indexed using job-id     */
} ClTxnComponentInfoT;

/* Entry for an active transaction */
typedef struct {
    ClTxnTransactionIdT     txnId;              /* Used during restoration  */
    ClIocPhysicalAddressT   clientAddr;
    ClTxnTransactionStateT  currentState;       /* State of txn in case of failure or from ckpt */
    ClUint32T               compCount;
    ClCntHandleT            txnCompList;        /* Entry for each component involved in txn */
    /* Temporary pointer */
    ClTxnDefnT              *pTxnDefn;
    ClUint32T               retryCount;
    ClUint8T                recoveryMode;       /* CL_TRUE - RESTORED, CL_FALSE - NEW   */
} ClTxnRecoveryLogT;

/* Structure for context of transaction recovery */
typedef struct {
    ClTxnDbHandleT          txnHistoryDb;
    ClCntHandleT            txnRecoveryDb;
    ClOsalMutexIdT          txnRecoveryMutex;
} ClTxnRecoveryT;

/* Initialize transaction recovery lib */
extern ClRcT clTxnRecoveryLibInit();

/* Finalize transaction recovery lib */
extern ClRcT clTxnRecoveryLibFini();

/* Initialize for failure recovery and txn activity */
extern ClRcT clTxnRecoverySessionInit(
        CL_IN   ClTxnTransactionIdT     serverTxnId);
  
extern ClRcT clTxnRecoverySessionUpdate(
        CL_IN   ClTxnDefnT              *pTxnDefn);

extern ClRcT clTxnRecoveryInitiate();

/* Set the participation of this component in transaction-job */
extern ClRcT clTxnRecoverySetComponent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress, 
        CL_IN   ClUint32T               compCfg);

/* Make a note of the fact that PREPARE cmd has been sent to this agent 
   for given transaction job
*/
extern ClRcT clTxnRecoveryPrepareCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress);


/* Process the received response from agent for transaction-job */
extern ClRcT clTxnRecoveryReceivedResp(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId, 
        CL_IN   ClIocPhysicalAddressT   compAddress, 
        CL_IN   ClTxnMgmtCommandT       cmd, 
        CL_IN   ClRcT                   resp);

/* Make a note of the fact that COMMIT cmd has been sent to this agent
   for given transaction job
*/
extern ClRcT clTxnRecoveryCommitCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress);

/* Make a note of the fact that ROLLBACK cmd has been sent to this agent
   for given transaction job
*/
extern ClRcT clTxnRecoveryRollbackCmdSent(
        CL_IN   ClTxnTransactionIdT     serverTxnId, 
        CL_IN   ClTxnTransactionJobIdT  jobId,
        CL_IN   ClIocPhysicalAddressT   compAddress);

/* API to update transaction state */
extern ClRcT clTxnRecoveryTxnFailed(
        CL_IN   ClTxnTransactionIdT     serverTxnId,
        CL_IN   ClTxnTransactionStateT  txnState);

/* Finalize the failure recovery and txn activity */
extern ClRcT clTxnRecoverySessionFini(
        CL_IN   ClTxnTransactionIdT     serverTxnId);

/* Routine to pack transaction logs */
extern ClRcT clTxnRecoveryLogPack(
        CL_IN   ClTxnTransactionIdT     *pTxnId, 
        CL_IN   ClBufferHandleT  msgHandle);

/* Routine to unpack transaction logs */
extern ClRcT clTxnRecoveryLogUnpack(
        CL_IN   ClBufferHandleT  msgHandle);

extern ClRcT clTxnRecoveryLogShow(
        CL_IN   ClTxnTransactionIdT     txnId,
        CL_IN   ClBufferHandleT  msgHandle);

#ifdef __cplusplus
}
#endif

#endif  /* _CL_TXN_RECOVERY_H_ */
