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
 * File        : clTxnClientIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains internal APIs available. It will also define additional
 * APIs available for testing and validation.
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_CLIENT_IPI_H
#define _CL_TXN_CLIENT_IPI_H

#include <clCommon.h>

#include <clTxnApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

#define CL_TXN_AGENT_JOB_DEFN_RECEIVE       5

#define CL_TXN_AGENT_JOB_DEFN_RECEIVE_FN    CL_EO_GET_FULL_FN_NUM(CL_TXN_CLIENT_TABLE_ID, \
                                                                  CL_TXN_AGENT_JOB_DEFN_RECEIVE)

#define CL_TXN_TRANSACTION_RUN_SYNC         0x8000U

/* This is used to write all the __VA_ARGS__ into the hdl supplied.
 * This hdl is eventually written to retStr of the debug cli exposed fptr.
 */
#define clTxnClientCliDbgPrint(hdl, ... ) do { \
    ClRcT retCode = CL_OK; \
    ClCharT temp[1024]; \
    sprintf(temp, __VA_ARGS__); \
    retCode = clBufferNBytesWrite(hdl, (ClUint8T *)temp, strlen((char *)temp)); \
    if(CL_OK != retCode) \
    {\
        clLogError("TXN", "CLT", \
                "Failed to write data in buffer, rc [0x%x]", retCode); \
    } \
}while(0)
/******************************************************************************
 *  Data Types 
 *****************************************************************************/

/******************************************************************************
 *  Callback Functions
 *****************************************************************************/

/******************************************************************************
 *  Data Structures
 *****************************************************************************/
/**
 * This structure maintains information of a transaction client initialized by
 * application component.
 */
typedef struct
{
    ClTxnTransactionCompletionCallbackT     fpTxnCompletionCallback;
    ClUint16T                               txnIdCounter;
    ClOsalMutexIdT                          txnMutex;
    ClOsalCondIdT                           txnClientCondVar;
    ClTxnDbHandleT                          activeTxnDb;
    ClTxnDbHandleT                          failedTxnDb;
    ClOsalMutexIdT                          txnReadMutex;
    ClHandleT                               dbgHandle;
} ClTxnClientInfoT;

/**
 * Following is the list of failed-agents for a transaction.
 */
typedef struct
{
    ClUint32T                   failedAgentCount;
    ClCntHandleT                failedAgentList;
    ClUint32T                   failedAgentRespCount;
    ClTxnTransactionCompletionCallbackT     fpTxnAgentCallback;
}ClTxnFailedTxnT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/
/**
 * Debug API to validate a given transaction-handle
 */
extern ClRcT clTxnTxnHandleValidate(
        CL_IN   ClTxnTransactionHandleT  tHandle);

/**
 * Debug API to validate a given job handle
 */
extern ClRcT clTxnJobHandleValidate(
        CL_IN   ClTxnJobHandleT tHandle);

/**
 * IPI to form transaction request and run it synchronously
 */
extern ClRcT clTxnTransactionRunSync(
        CL_IN   ClTxnDefnT      *pTxnDefn);

/**
 * IPI to form transaction request and run it asynchronously
 */
extern ClRcT clTxnTransactionRunAsync(
        CL_IN   ClTxnDefnT      *pTxnDefn);

/**
 * Specific to COR
 */
extern ClRcT clTxnReadJobAsync(
        CL_IN   ClTxnDefnT      *pTxnDefn,
        CL_IN   ClTxnFailedTxnT        *pFailedTxn);


/* Forward Declarations */
extern void clTxnAgentJobMsgReceive(
        CL_IN   ClRcT           retCode,
        CL_IN   void            *pCookie,
        CL_IN   ClBufferHandleT  inMsg, 
        CL_OUT  ClBufferHandleT  outMsg);

/* Forward Declarations */
void clTxnClientResMsgReceive(
        ClRcT            retCode, 
        ClPtrT           pCookie, 
        ClBufferHandleT  inMsg, 
        ClBufferHandleT  outMsg);
/**
 * IPI to finalize a transaction
 */
extern ClRcT clTxnTransactionFinalize(
        CL_IN   ClTxnDefnT      *pTxnDefn);


#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _CL_TXN_CLIENT_IPI_H */
