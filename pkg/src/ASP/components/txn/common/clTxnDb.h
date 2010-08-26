/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : txn                                                           
 * File        : clTxnDb.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains IPI definition of transaction information database.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_DB_H
#define _CL_TXN_DB_H


#include <clCommon.h>
#include <clTxnCommonDefn.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef ClCntHandleT       ClTxnDbHandleT;

extern ClRcT clTxnNewTxnDfnCreate(
        CL_OUT  ClTxnDefnPtrT   *pNewTxnDef);

/**
 * Delete transaction definition
 */
extern ClRcT clTxnDefnDelete(
        CL_IN   ClTxnDefnT    *pTxnDefn);

/**
 * Add newly created job to transaction.
 */
extern ClRcT clTxnNewAppJobAdd(
        CL_IN   ClTxnDefnT          *pTxnDefn, 
        CL_IN   ClTxnAppJobDefnT    *pNewTxnJob);

/**
 * Remove an existing job from transaction.
 */
extern ClRcT clTxnAppJobRemove(
        CL_IN   ClTxnDefnT              *pTxnDefn, 
        CL_IN   ClTxnTransactionJobIdT  jobId);

/**
 * Create newly transaction-job definition
 */
extern ClRcT clTxnNewAppJobCreate(
        CL_OUT  ClTxnAppJobDefnPtrT  *pNewTxnJob);

/**
 * Delete/free transaction-job definition
 */
extern ClRcT clTxnAppJobDelete(
        CL_IN   ClTxnAppJobDefnT *pTxnAppJob);

/**
 * Append new participating component information 
 * for transaction-job
 */
extern ClRcT clTxnAppJobComponentAdd(
        CL_IN   ClTxnAppJobDefnT        *pTxnJobDefn, 
        CL_IN   ClIocPhysicalAddressT   txnCompAddress, 
        CL_IN   ClUint8T                configMask);


ClRcT clTxnAgentListCreate(ClCntHandleT *pAgentCntHandle);

/**
 * Initialize txn-db for maintaining transaction information.
 */
extern ClRcT clTxnDbInit(
        CL_OUT  ClTxnDbHandleT      *pTxnDb);

/**
 * Finalize txn-db and free all allocated memory
 */
extern ClRcT clTxnDbFini(
        CL_IN   ClTxnDbHandleT      txnDb);

/**
 * Add new transaction definition to txn-database.
 */
extern ClRcT clTxnDbNewTxnDefnAdd(
        CL_IN   ClTxnDbHandleT      txnDb, 
        CL_IN   ClTxnDefnT          *pTxnDefn,
        CL_IN   ClTxnTransactionIdT *pTxnId);
/**
 * Remove a transaction definition from txn-database.
 */
extern ClRcT clTxnDbTxnDefnRemove(
        CL_IN   ClTxnDbHandleT      txnDb,
        CL_IN   ClTxnTransactionIdT txnId);

/**
 * Retrieve txn-definition using txn-id as index
 */
extern ClRcT clTxnDbTxnDefnGet(
        CL_IN   ClTxnDbHandleT      txnDb, 
        CL_IN   ClTxnTransactionIdT txnId, 
        CL_OUT  ClTxnDefnT          **pTxnDefn);

#ifdef __cplusplus
}
#endif

#endif
