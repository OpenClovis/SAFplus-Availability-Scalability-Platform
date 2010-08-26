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
 * File        : clTxnStreamIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module contains necessary implementation used by the txnClient
 * txnMgr modules for streaming (packing/unpacking) data for transaction
 * processing.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_STREAM_IPI_H
#define _CL_TXN_STREAM_IPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

#include <clCntApi.h>
#include <clBufferApi.h>

#include <clTxnCommonDefn.h>



/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 * Packs Transaction details along with job-description associated with it
 * and returns a buffer. This IPI is used by the transaction-client 
 * for distributing transaction-job descriptions to various agents.
 *
 */
extern ClRcT clTxnStreamTxnDataPack(
        CL_IN   ClTxnDefnT              *pTxnDefn, 
        CL_IN   ClTxnAppJobDefnT        *pTxnAppJob, 
        CL_IN   ClBufferHandleT  msgHandle);

/**
 * Unpacks transaction details from the byte-stream. This IPI is used by the 
 * transaction-agents to extract transaction-job description from clients.
 *
 */
extern ClRcT clTxnStreamTxnDataUnpack(
        CL_IN   ClBufferHandleT  msgHandle, 
        CL_OUT  ClTxnDefnT              **pTxnDefn, 
        CL_OUT  ClTxnAppJobDefnT        **pTxnAppJob);
/**
 * Packs Transaction details along with job-description associated with it
 * and returns a buffer. This IPI is used by the transaction-client 
 * for distributing transaction-job descriptions to various agents.
 *
 */
extern ClRcT clTxnStreamTxnAgentDataPack(
        CL_IN   ClUint32T               *failedAgentCount,
        CL_IN   ClCntHandleT            failedAgentList,
        CL_IN   ClBufferHandleT  msgHandle);

/**
 * Unpacks transaction details from the byte-stream. This IPI is used by the 
 * transaction-agents to extract transaction-job description from clients.
 *
 */
extern ClRcT clTxnStreamTxnAgentDataUnpack(
        CL_OUT   ClBufferHandleT  msgHandle,
        CL_OUT   ClCntHandleT            *pFailedAgentList,
        CL_OUT   ClUint32T               *failedAgentCount);


/**
 * Packs configuration information required to run a given transaction such as
 * components involved, order and additional user provided information
 * for customization run-time behavior of transaction management.
 * This API is used by transaction-client while controlling transaction processing
 * at server.
 */
extern ClRcT clTxnStreamTxnCfgInfoPack(
        CL_IN   ClTxnDefnT              *pTxnDefn, 
        CL_IN   ClBufferHandleT  msgHandle);

/**
 * Unpacks configuration information required to run a given transaction.
 * This API is used by transaction manager to extract configuration information
 * sent by txn-client to run the transaction.
 */
extern ClRcT clTxnStreamTxnCfgInfoUnpack(
        CL_IN   ClBufferHandleT  msgHandle, 
        CL_OUT  ClTxnDefnT              **pTxnDefn);

/**
 * Get the transactional state 
 */
extern ClCharT* clTxnStateGet(
        CL_IN ClUint32T state);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _CL_TXN_STREAM_IPI_H  */
