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
 * File        : clTxnDebugIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This module defines data-structures used for debugging using CLI
 *
 *
 *****************************************************************************/

#ifndef _CL_TXN_DEBUG_IPI_H
#define _CL_TXN_DEBUG_IPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clTxnApi.h>
#include <clTxnAgentApi.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

#define     CL_TXN_DEBUG_SERVICE   0
#define     CL_MAX_DATA_SIZE       10

#define     CL_TXN_CLI_DISPLAY_SHORT_SEP        \
                            "-------------------------------------------------------------------------------------\n"
#define     CL_TXN_CLI_DISPLAY_SHORT_SEP_SIZE   \
                            (strlen(CL_TXN_CLI_DISPLAY_SHORT_SEP))

#define     CL_TXN_CLI_DISPLAY_TXN_LIST_HEADER  \
                            "   Server TxnId   |   Client TxnId   |   Server TxnHandle   |    Txn-Status\n"
#define     CL_TXN_CLI_DISPLAY_TXN_LIST_HEADER_SIZE \
                            (strlen(CL_TXN_CLI_DISPLAY_TXN_LIST_HEADER))


#define     CL_TXN_CLI_DISPLAY_LONG_SEP        \
                            "----------------------------------------------------------------\n"
#define     CL_TXN_CLI_DISPLAY_LONG_SEP_SIZE   \
                            (strlen(CL_TXN_CLI_DISPLAY_LONG_SEP))

#define     CL_TXN_CLI_DISPLAY_TXN_INFO_HEADER  \
                            "|     Job-Id     |    Status    |       Participant List       |\n"
#define     CL_TXN_CLI_DISPLAY_TXN_INFO_HEADER_SIZE \
                            (strlen(CL_TXN_CLI_DISPLAY_TXN_INFO_HEADER))

#define     CL_TXN_CLI_DISPLAY_TXN_NO_COMPONENT          \
                            "|                |              |            N/A               |\n"
#define     CL_TXN_CLI_DISPLAY_TXN_NO_COMPONENT_SIZE    \
                            (strlen(CL_TXN_CLI_DISPLAY_TXN_NO_COMPONENT))

/******************************************************************************
 *  Data Types 
 *****************************************************************************/
typedef enum dummyOperation
{
    CL_TXN_DUMMY_OP_ADD         = 1,
    CL_TXN_DUMMY_OP_SUBTRACT    = 2
} ClTxnDummyOperation;

typedef struct clTxnDebugDummyJob 
{
    ClTxnDummyOperation     op;
    ClUint32T               value;
    ClUint32T               index;
} ClTxnDebugDummyJobT;

typedef struct clTxnDebugShadowMemory
{
    ClUint32T               prevValue;
    ClUint32T               newValue;
} ClTxnDebugShadowMemoryT;

ClRcT clTxnDebugRegister(ClEoExecutionObjT *pEoObj, ClNameT appName);

ClRcT clTxnDebugUnregister(ClEoExecutionObjT *pEoObj);
/* Initialize txn-client */
ClRcT clCliTxnInit(int argc, char **argv, char **retStr);

/* Create a dummy transaction for debugging purpose */
ClRcT clCliTxnTransactionCreate(int argc, char **argv, char **retStr);

/* Add a dummy job for debugging purpose */
ClRcT clCliTxnDummyJobAdd(int argc, char **argv, char **retStr);

/* Delete the job from transaction */
ClRcT clCliTxnDummyJobDelete(int argc, char **argv, char **retStr);

/* Set component for transaction-job */
ClRcT clCliTxnComponentSet(int argc, char **argv, char **retStr);

/* Run a given transaction */
ClRcT clCliTxnTransactionRun(int argc, char **argv, char **retStr);

/* Show Active transactions */
ClRcT clCliShowActiveTransactions(int argc, char **argv, char **retStr);

/* Show details of a transaction */
ClRcT clCliShowTransaction(int argc, char **argv, char **retStr);

/* Show activities of a transaction */
ClRcT clCliShowTransactionActivity(int argc, char **argv, char **retStr);

/* Show data used for test-transaction */
ClRcT clCliShowTestData(int argc, char **argv, char **retStr);

/* Show data used for test-transaction */
ClRcT clCliShowAgentState(int argc, char **argv, char **retStr);

/* Show txn history */
ClRcT clCliShowTxnHistory(int argc, char **argv, char **retStr);

/* Plays transaction */
ClRcT clCliTxnPlay(int argc, char **argv, char **retStr);


/* Agent-Debug Prepare callback */
ClRcT clTxnAgentStart(
        CL_IN       ClTxnTransactionHandleT txnHandle, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie);

ClRcT clTxnAgentStop(
        CL_IN       ClTxnTransactionHandleT txnHandle, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie);

ClRcT clTxnAgentDebugJobPrepare(
        CL_IN       ClTxnTransactionHandleT txnHandle, 
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie);

/* Agent-Debug COmmit Callback */
ClRcT clTxnAgentDebugJobCommit(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie);

/* Agent-debug rollback callback */
ClRcT clTxnAgentDebugRollback(
        CL_IN       ClTxnTransactionHandleT txnHandle, 
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie);

#ifdef __cplusplus
}
#endif

#endif /* _CL_TXN_DEBUG_IPI_H */
