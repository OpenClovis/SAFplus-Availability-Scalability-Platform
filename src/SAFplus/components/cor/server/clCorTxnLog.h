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
 * ModuleName  : cor
 * File        : clCorTxnLog.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Implements logging feature for COR defined Transactions
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_TXN_LOG_H_
#define _CL_COR_TXN_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCommon.h>
#include <clCorTxnApi.h>

/* DEFINES */

/* ENUMs */

/* PROTOTYPES */

/**
 * API to initialize COR-Txn Logging
 */
ClRcT clCorTxnLogInit(
    CL_IN   ClUint32T       maxLogs);

/**
 * API to finalize COR-Txn Logging
 */
ClRcT clCorTxnLogFini(void);

/**
 * API to add new transaction into logging 
 */
/* TODO - IDL */
#if 0
ClRcT clCorTxnLogAdd(
    CL_IN   ClCorTxnJobStreamT *txnStreamH);
#endif

/**
 * API to show all transactions
 */
ClRcT clCorTxnLogShow(ClUint32T verbose);


#ifdef __cplusplus
}
#endif

#endif  /*  _CL_COR_TXN_LOG_H_ */
