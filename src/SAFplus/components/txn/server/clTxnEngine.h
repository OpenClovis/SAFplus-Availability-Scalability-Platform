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
 * File        : clTxnEngine.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module is the core transaction engine definition
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_ENGINE_H_
#define _CL_TXN_ENGINE_H_

#include <clCommon.h>
#include <clIocApi.h>

#include <clTxnCommonDefn.h>
#include <clTxnDb.h>
#include <clTxnProtocolDefn.h>

#ifdef __cplusplus
extern "C" {
#endif

    
#define CL_TXN_ENGINE_ACTIVE_TXN_BUCKET_SIZE    10

typedef struct {
    ClIocPhysicalAddressT   agentAddr;
    ClRcT                   status;
    ClTxnMgmtCommandT       cmd;
    ClRcT                   respRcvd;
} ClTxnAgentResponseT;

typedef enum {
    CL_TXN_ENGINE_TRANSACTION_START,        /* Request to start new transaction */
    CL_TXN_ENGINE_TRANSACTION_CANCEL,       /* Request to cancel a running transaction */
    CL_TXN_ENGINE_TXN_JOB_RESP_RCVD,        /* To proceed on a ginve job after receiving all responses */
    CL_TXN_ENGINE_TERMINATE                 /* During termination of txn-server */
} ClTxnEngineCmdT;

/* Control Message exechanged between Transaction service and
   transaction core engine
 */
typedef struct {
    ClUint32T                   structId;
    ClTxnEngineCmdT             cmd;
    ClTxnTransactionIdT         txnId;      /* Reference txn-id */
    ClTxnTransactionJobIdT      jobId;      /* Reference job-id */
    ClUint32T                   guard;
} ClTxnCntrlMsgT;


    enum
    {
        ClTxnCntrlMsgStructId = 0x67834012
    };


    ClRcT VerifyTxnCntrlMsg(ClTxnCntrlMsgT* txnMsg);

/* Forward declaration */
typedef struct activeTxn        ClTxnActiveTxnT;
typedef struct activeJob        ClTxnActiveJobT;

/* Active job definition type */
struct activeJob {
    ClTxnActiveTxnT             *pParentTxn;        /* reference to txn containing this job */
    ClTxnAppJobDefnT            *pAppJobDefn;
    ClTxnJobExecContextT        jobExecContext;
    ClUint32T                   agentRespCount;
    ClRcT                       rcvdStatus;
    ClTxnAgentResponseT         *pAgntRespList;
    ClTimerHandleT              txnJobTimerHandle;
};

/* Active transaction definition type */
struct activeTxn {
    ClTxnDefnT                  *pTxnDefn;
    ClTxnExecContextT           txnExecContext;
    ClTxnActiveJobT             *pActiveJobList;
    ClUint32T                   activeJobListLen;  /* length of pActiveJobList for verification purposes */
    ClUint32T                   pendingRespCount;
    ClRcT                       rcvdStatus;
    ClTxnActiveTxnT             *pParentTxn;            /* Parent txn reference, in case of nested txn */
    ClIocPhysicalAddressT       clientAddr;
    ClUint32T                   failedAgentCount;       /* Count of the failed agets*/
    ClCntHandleT                failedAgentList;       /* List of failed Agent Components */
    ClRmdResponseContextHandleT rmdDeferHdl;            /* defer handle */
    ClBufferHandleT             deferOutMsg;            /* Defer outMsgHandle, response to client is written here */
};

typedef struct {
    ClCntHandleT                activeTxnMap;       /* Active Txn Map/List */
    ClOsalTaskIdT               txnEngineTask;      /* Task-Id of txn-engine */
    ClQueueT                    txnReqQueue;        /* Q for txn-requests */
    ClOsalMutexIdT              txnEngineMutex;     /* Mutex for operating on active-txn Map */
    ClOsalCondIdT               txnCondVar;         /* conditional-variable */
} ClTxnEngineT;

/**
 * Initialize transaction processing engine
 */
extern ClRcT clTxnEngineInitialize();

/**
 * Delete/Finalize transaction processing engine.
 */
extern ClRcT clTxnEngineFinalize();

/**
 * Receive new transaction for processing/completion
 */
extern ClRcT clTxnEngineNewTxnReceive(
        CL_IN   ClTxnDefnT              *pNewTxn,
        CL_IN   ClIocPhysicalAddressT   clientAddr,
        CL_IN   ClUint32T               recoveryMode,
        CL_IN   ClBufferHandleT         outMsg,
        CL_IN   ClUint8T                cli); /* Indicates whether its a cli command */

/**
 * Receive response from transaction agent after processing
 * commands for given transaction/job
 */
extern ClRcT clTxnEngineAgentResponseReceive(
        CL_IN   ClTxnMessageHeaderT     msgHeader,
        CL_IN   ClBufferHandleT  msgHandle);

/**
 * Callback that receives the async request sent to the Agent  
 */
extern void clTxnEngineAgentAsyncResponseRecv(
        CL_IN ClRcT rc,
        CL_IN ClPtrT cookie,
        CL_IN ClBufferHandleT inMsg,
        CL_IN ClBufferHandleT outMsg);

#ifdef __cplusplus
}
#endif

#endif /* _CL_TXN_ENGINE_H_ */

