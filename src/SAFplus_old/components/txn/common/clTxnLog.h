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
 * File        : clTxnLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This header file contains definitions of log messages adapted in transaction
 * management module.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_LOG_H_
#define _CL_TXN_LOG_H_

#include <clLogApi.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClCharT  *clTxnLogMsg[];

#define         CL_TXN_LOG_0_INVALID_JOB_RECEIVED           clTxnLogMsg[0]  /* "Invalid job description received"   */
#define         CL_TXN_LOG_0_JOB_VALIDATE_FAILED            clTxnLogMsg[1]  /* "Transaction Job Validation Failed"  */
#define         CL_TXN_LOG_0_JOB_COMMIT_FAILED              clTxnLogMsg[2]  /* "Transaction Job Commit Failed"      */
#define         CL_TXN_LOG_0_JOB_ROLLBACK_FAILED            clTxnLogMsg[3]  /* "Transaction Job Rollback Failed"    */
#define         CL_TXN_LOG_0_JOB_FINALIZE_FAILED            clTxnLogMsg[4]  /* "Transaction Job Finalize Failed"    */
#define         CL_TXN_LOG_1_TRANSACTION_FAILED             clTxnLogMsg[5]  /* "Transaction [Id:0x%x] failed"       */
#define         CL_TXN_LOG_1_INVALID_STATE_TRANSACTION      clTxnLogMsg[6]  /* "Transaction [Id:0x%x] received in invalid state"    */


#define CL_TXN_AGENT_LIB            "Txn-Agent"

#ifdef __cplusplus
}
#endif

#endif
