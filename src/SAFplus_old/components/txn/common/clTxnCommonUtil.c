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
 * File        : clTxnCommonUtil.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains common utility functions used by transaction management
 *
 *
 *****************************************************************************/


#include <string.h>

#include <clDebugApi.h>

#include <clTxnApi.h>
#include "clTxnCommonDefn.h"

/**
 * Initialization for transaction debug/trace logs
 */
ClCharT     *clTxnLogMsg[] =
{
    "Invalid job description received",                     /* CL_TXN_LOG_0_INVALID_JOB_RECEIVED        */
    "Transaction Job Validation Failed",                    /* CL_TXN_LOG_0_JOB_VALIDATE_FAILED         */
    "Transaction Job Commit Failed",                        /* CL_TXN_LOG_0_JOB_COMMIT_FAILED           */
    "Transaction Job Rollback Failed",                      /* CL_TXN_LOG_0_JOB_ROLLBACK_FAILED         */
    "Transaction Job Finalize Failed",                      /* CL_TXN_LOG_0_JOB_FINALIZE_FAILED         */
    "Transaction [Id:0x%x] failed",                         /* CL_TXN_LOG_1_TRANSACTION_FAILED          */
    "Transaction [Id:0x%x] received in invalid state"       /* CL_TXN_LOG_1_INVALID_STATE_TRANSACTION   */
};

/**
 * Compare given two transaction id
 */
ClInt32T  clTxnUtilTxnIdCompare(
        CL_IN   ClTxnTransactionIdT     txnId_1, 
        CL_IN   ClTxnTransactionIdT     txnId_2)
{
    ClInt32T    cmp = 0;

    cmp = txnId_1.txnMgrNodeAddress - txnId_2.txnMgrNodeAddress;

    if (cmp == 0)
        cmp =  txnId_1.txnId - txnId_2.txnId;

    return (cmp); 
}

/**
 * Compare given two transaction job-id
 */
ClInt32T  clTxnUtilJobIdCompare(
        CL_IN   ClTxnTransactionJobIdT  jobId_1,
        CL_IN   ClTxnTransactionJobIdT  jobId_2)
{
    ClInt32T cmp = 0;

#if 0  /* Job id field is NOT used */
    cmp = clTxnUtilTxnIdCompare ( jobId_1.txnId, jobId_2.txnId);
#endif
    if (cmp == 0)
        cmp = jobId_1.jobId - jobId_2.jobId; 
    
    return (cmp);
}
ClCharT* clTxnStateGet(
        CL_IN ClUint32T state)
{
    switch(state)
    {
        case CL_TXN_STATE_PRE_INIT:
            return "PRE-INIT";
        case CL_TXN_STATE_ACTIVE:
            return "ACTIVE";
        case CL_TXN_STATE_PREPARING:
            return "PREPARING";
        case CL_TXN_STATE_PREPARED:
            return "PREPARED";
        case CL_TXN_STATE_COMMITTING:
            return "COMMITTING";
        case CL_TXN_STATE_COMMITTED:
            return "COMMITTED";
        case CL_TXN_STATE_MARKED_ROLLBACK:
            return "MARKED ROLLBACK";
        case CL_TXN_STATE_ROLLING_BACK:
            return "ROLLING BACK";
        case CL_TXN_STATE_ROLLED_BACK:
            return "ROLLED BACK";
        case CL_TXN_STATE_RESTORED:
            return "RESTORED";
        case CL_TXN_STATE_UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID STATE";
    }
}


