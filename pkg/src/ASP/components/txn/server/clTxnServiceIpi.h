/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*******************************************************************************
 * ModuleName  : txn                                                           
 * $File: //depot/dev/main/Andromeda/Cauvery/ASP/components/txn/server/clTxnServiceIpi.h $
 * $Author: deepak $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module defines internal data-structures and methods used by
 * transaction management service
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_SERVICE_IPI_H
#define _CL_TXN_SERVICE_IPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clOsalApi.h>

#include <clEoApi.h>
#include <clCkptApi.h>

#include <clTxnCommonDefn.h>

#include <clTxnDb.h>

#include "clTxnProtocolDefn.h"
#include "clTxnEngine.h"
#include "clTxnRecovery.h"

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

#define         CL_TXN_CKPT_NAME                    "TxnServiceCkpt"
#define         CL_TXN_CKPT_COUNT                   1
#define         CL_TXN_CKPT_DATASET_COUNT           0   /* Actually there is no max-limit */
#define         CL_TXN_SERVICE_DATA_SET             1
#define         CL_TXN_RECOVERY_DATA_SET            2

#define         CL_TXN_CKPT_TXN_DATA_SET_ID_OFFSET  100


#define         CL_TXN_RESTORE                      0x1
#define         CL_TXN_COMMIT                       0x2
/******************************************************************************
 *  Data Types 
 *****************************************************************************/

typedef ClPtrT   ClTxnServiceHandleT;


typedef struct
{
    ClTxnEngineT                    *pTxnEngine;            /* Txn Core Engine Instance */
    ClCkptSvcHdlT                   txnCkptHandle;          /* Txn Ckpt Session Handle  */
    ClTxnRecoveryT                  *pTxnRecoveryContext;   /* Maintains txn recovery logs */

    ClOsalMutexIdT                  txnServiceMutex;        /* For protecting service txn-id */
    ClUint32T                       txnIdCounter;           /* For server-side txn-Id generation */
    ClUint8T                        dsblCkpt;               /* For disabling checkpoint */
    ClBoolT                         retVal;
} ClTxnServiceT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/* External functions (only if transaction service is linked to main-application) */

ClRcT 
_clTxnServiceClientReqProcess(
        CL_IN   ClTxnMessageHeaderT     msgHeader, 
        CL_IN   ClBufferHandleT  inMsgHandle,
        CL_IN   ClBufferHandleT  outMsgHandle, 
        CL_IN   ClUint8T         cli); /* indicates if it is a cli invocation */
/**
 * Initializes transaction service by installing RMD callout functions and initializing
 * internal data-structure and protocol handlers.
 */
extern ClRcT clTxnServiceInitialize(
        CL_IN ClEoExecutionObjT *pEoObj, 
        CL_IN ClEoClientIdT clientId, 
        CL_OUT ClTxnServiceHandleT *pTxnSrvcHandle);

/**
 * Finalize transaction service by terminating all on-going transaction processing.
 */
extern ClRcT clTxnServiceFinalize(
        CL_IN ClTxnServiceHandleT txnSrvcHandle, 
        CL_IN ClEoClientIdT clientId);

/**
 * Function to initialize transaction check-point service.
 */
extern ClRcT clTxnServiceCkptInitialize();

/**
 * Function to finalize transaction check-point service
 */
extern ClRcT clTxnServiceCkptFinalize();

/**
 * API to checkpoint current state of transaction-service 
 * (Checkpoint transaction application state)
 */
extern ClRcT clTxnServiceCkptAppStateCheckpoint();

/**
 * API to restore status of transaction-service application 
 * from check-point
 */
extern ClRcT clTxnServiceCkptAppStateRestore();

/**
 * API to checkpoint recovery-log of an active transaction.
 */
extern ClRcT clTxnServiceCkptRecoveryLogCheckpoint(
        CL_IN   ClTxnTransactionIdT     txnId);
/**
 * API to delete checkpoint recovery-log of an active transaction.
 */
extern ClRcT clTxnServiceCkptRecoveryLogCheckpointDelete(
        CL_IN   ClTxnTransactionIdT     txnId);

/**
 * API to restore recovery logs of previously active transactions
 * from check-point
 */
extern ClRcT clTxnServiceCkptRecoveryLogRestore();

/**
 * API to checkpoint newly created transaction
 */
extern ClRcT clTxnServiceCkptNewTxnCheckpoint(
        CL_IN   ClTxnDefnT  *pTxnDefn);

/**
 * API to restore definition of a transaction from check-point data-set
 */
extern ClRcT clTxnServiceCkptTxnRestore(
        CL_IN   ClTxnTransactionIdT     serverTxnId);

/**
 * API to delete checkpoint data-set for given transaction
 */
extern ClRcT clTxnServiceCkptTxnDelete(
        CL_IN   ClTxnTransactionIdT     serverTxnId);
#ifdef __cplusplus
}
#endif

#endif /* _CL_TXN_SERVICE_IPI_H */
