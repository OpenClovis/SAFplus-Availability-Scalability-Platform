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
 * File        : clTxnErrors.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains all relevant definitions of errors conditions for
 * transaction management.
 *
 *
 *****************************************************************************/



/**
 *  \file
 *  \ingroup group34
 */

/**
 *  \addtogroup group34
 *  \{
 */




#ifndef _CL_TXN_ERRORS_H_
#define _CL_TXN_ERRORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clCommonErrors.h>

/******************************************************************************
 * ERROR/RETURN CODE HANDLING MACROS
 *****************************************************************************/
/**
 * Macro to set transaction component-id to return code.
 */
#define CL_TXN_RC(ERR_ID)      (CL_RC(CL_CID_TXN, ERR_ID))

/**
 * Offset for transaction specific error-id.
 */
#define CL_TXN_SPECIFIC_ERR_ID_OFFSET  CL_ERR_COMMON_MAX

/******************************************************************************
 * ERROR CODES
 *****************************************************************************/
/**
 * Error code representing validation failure.
 * This value is 0x100
 */
#define CL_TXN_ERR_VALIDATE_FAILED              0x100

/**
 * Error code representing commit failure.
 * This value is 0x101
 */
#define CL_TXN_ERR_TRANSACTION_ROLLED_BACK      0x101 

/**
 * Error code representing transaction failure.
 * This value is 0x102
 */
#define CL_TXN_ERR_TRANSACTION_FAILURE          0x102

/**
 * Error code representing no services registered at a transaction-agent.
 * This value is 0x103
 */
#define CL_TXN_ERR_NO_REGD_SERVICE              0x103

/**
 * Error code representing invalid/mis-match of component and agent capability.
 * This value is 0x104
 */
#define CL_TXN_ERR_INVALID_CMP_CAPABILITY       0x104

/**
 * Error code representing failed agent count is void.  
 * This value is 0x105
 */
#define CL_TXN_ERR_AGENT_FAILED_VOID            0x105

/**
 * Error code representing failed agent count is void.  
 * This value is 0x106
 */
#define CL_TXN_ERR_NO_JOBS                      0x106

/**
 * Error code representing failed agent count is void.  
 * This value is 0x107
 */
#define CL_TXN_ERR_NO_COMPONENTS                0x107



#ifdef __cplusplus
}
#endif

#endif  /* _CL_TXN_ERRORS_H_ */


/** \} */
