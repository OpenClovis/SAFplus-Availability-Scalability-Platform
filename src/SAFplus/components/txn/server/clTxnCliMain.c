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
 * File        : clTxnCliMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>


#include <clDebugApi.h>
#include <clCpmApi.h>
#include <clBufferApi.h>

#include <clTxnErrors.h>

#include <xdrClTxnMessageHeaderIDLT.h>

#include <clTxnCommonDefn.h>
#include "clTxnServiceIpi.h"
#include "clTxnDebugIpi.h"
#include "clTxnRecovery.h"

#ifdef RECORD_TXN
/* Unmashalling functions declared here */
#include <clTxnStreamIpi.h>
#include <xdrClTxnCmdT.h>
#include <time.h>
#endif


/* Forward declarations */
ClRcT _clCliAppendActiveTxn(CL_IN ClCntKeyHandleT userKey, 
                            CL_IN ClCntDataHandleT userData, 
                            CL_IN ClCntArgHandleT userArg, 
                            CL_IN ClUint32T len);

ClRcT _clCliAppendJob(CL_IN ClCntKeyHandleT userKey, 
                      CL_IN ClCntDataHandleT userData, 
                      CL_IN ClCntArgHandleT userArg, 
                      CL_IN ClUint32T len);

ClRcT _clCliAppendCompList(CL_IN ClCntKeyHandleT userKey, 
                           CL_IN ClCntDataHandleT userData, 
                           CL_IN ClCntArgHandleT userArg, 
                           CL_IN ClUint32T dataLen);

/* Global Data and externs */
ClHandleT   gTxnDebugReg = CL_HANDLE_INVALID_VALUE;
static ClUint32T                    *gSharedData;
static ClTxnClientHandleT           gTxnCliClientHandle;
static ClTxnAgentServiceHandleT     gTxnCliAgentHandle;
static ClCharT                      gInitFlag;

extern ClTxnServiceT                *clTxnServiceCfg;

#ifdef RECORD_TXN
extern ClTxnDbgT gTxnDbg;
static ClRcT   
_clTxnReadRecords(ClTxnTransactionIdT *pTxnId, 
                  ClBufferHandleT output);

static ClRcT   
__clTxnReadRecord(ClBufferHandleT output,
                 ClTxnTransactionIdT *pTxnId, 
                 ClUint8T *pTrue);

ClRcT _clTxnCliJobWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen);

ClRcT _clTxnCliCompWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen);

static ClRcT   
__clTxnReadBuff(ClBufferHandleT output,
                 ClTxnTransactionIdT *pTxnId); 
#endif
static ClDebugFuncEntryT gClTxnCliFuncTable[] = 
{
    {(ClDebugCallbackT)clCliTxnInit,                "init", 
                                                    "Initialize Txn for test application"},

    {(ClDebugCallbackT)clCliTxnTransactionCreate,   "createTxn", 
                                                    "Create a new transaction"},

    {(ClDebugCallbackT)clCliTxnDummyJobAdd,         "addJob", 
                                                    "Add a job to a transaction"},

    {(ClDebugCallbackT)clCliTxnDummyJobDelete,      "deleteJob", 
                                                    "Delete the job from transaction"},

    {(ClDebugCallbackT)clCliTxnComponentSet,        "setComponent", 
                                                    "Set component for transaction-job"},

    {(ClDebugCallbackT)clCliTxnTransactionRun,      "runTxn", 
                                                    "Run a given transaction"},

    {(ClDebugCallbackT)clCliShowActiveTransactions, "showActiveTxn", 
                                                    "Show Active transactions"},

    {(ClDebugCallbackT)clCliShowTransaction,        "showTxnDetails", 
                                                    "Show details of a transaction"},

    {(ClDebugCallbackT)clCliShowTransactionActivity,"showTxnActivity", 
                                                    "Show activity of a transaction"},

    {(ClDebugCallbackT)clCliShowTestData,           "showTestData", 
                                                    "Show data used for test-transaction"},
    
    {(ClDebugCallbackT)clCliShowAgentState,           "showAgentState", 
                                                    "Show agent details"},

    {(ClDebugCallbackT)clCliShowTxnHistory,           "showTxnHistory", 
                                                    "Show transaction history"},
    {(ClDebugCallbackT)clCliTxnPlay,                "txnPlay", 
                                                    "Play stored transaction"}
};

ClDebugModEntryT clModTab[] = 
{
    {"TXN", "TXN", gClTxnCliFuncTable, "Transaction Service commands"},
    {"", "", 0, ""}
};

static ClTxnAgentCallbacksT gAgentDebugCallback  = {
    (ClTxnAgentTxnStartCallbackT) clTxnAgentStart,
    (ClTxnAgentTxnJobPrepareCallbackT) clTxnAgentDebugJobPrepare,
    (ClTxnAgentTxnJobCommitCallbackT)  clTxnAgentDebugJobCommit,
    (ClTxnAgentTxnJobRollbackCallbackT) clTxnAgentDebugRollback,
    (ClTxnAgentTxnStopCallbackT) clTxnAgentStop,
};

static char*    gCliTxnStatusStr[] = {
    "",
    "CL_TXN_TRANSACTION_PRE_INIT",
    "CL_TXN_TRANSACTION_ACTIVE",
    "CL_TXN_TRANSACTION_PREPARING",
    "CL_TXN_TRANSACTION_PREPARED",
    "CL_TXN_TRANSACTION_COMMITTING",
    "CL_TXN_TRANSACTION_COMMITTED",
    "CL_TXN_TRANSACTION_MARKED_ROLLBACK",
    "CL_TXN_TRANSACTION_ROLLING_BACK",
    "CL_TXN_TRANSACTION_ROLLED_BACK",
    "CL_TXN_TRANSACTION_RESTORED",
    "CL_TXN_TRANSACTION_UNKNOWN",
};

/**
 * Debug Register - to be called during application initialization.
 */
ClRcT clTxnDebugRegister(ClEoExecutionObjT *pEoObj, 
                         /* Suppressing coverity warning for pass by value with below comment */
                         // coverity[pass_by_value]
                         ClNameT appName)
{
    ClRcT rc = CL_OK;

    rc = clDebugPromptSet("TXN");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }
    rc = clDebugRegister(gClTxnCliFuncTable, 
                         sizeof(gClTxnCliFuncTable)/sizeof(gClTxnCliFuncTable[0]), 
                         &gTxnDebugReg);
    gInitFlag = 0;
    return (rc);
}

/**
 * Debug Un-register - to be called during application termination.
 */
ClRcT clTxnDebugUnregister(ClEoExecutionObjT *pEoObj)
{
    /* Call Client, agent finalize functions */
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside finalize function gInitFlag: 0x%x\n", gInitFlag));
    if (gInitFlag)
    {
        clTxnClientFinalize(gTxnCliClientHandle);
        clTxnAgentServiceUnRegister(gTxnCliAgentHandle);
        clTxnAgentFinalize();
        gInitFlag = 0;
    }
    return clDebugDeregister(gTxnDebugReg);
}

/**
  * Internal function to produce output.
  */
ClRcT clCliTxnStrPrint(char *str, char **retStr)
{
    *retStr = (char *) clHeapAllocate(strlen(str) + 1);
    if (NULL != (*retStr))
        sprintf(*retStr, str);

    return (CL_OK);
}

/**
  * Callback function used by client
  */
ClRcT clCliTxnClientCallbackFn(ClTxnTransactionHandleT txnHandle, ClRcT retCode)
{
    ClRcT rc = CL_OK;
    /* FIXME: Show the details here */
    return (rc);
}
/* Start callback */
ClRcT clTxnAgentStart(
        CL_IN       ClTxnTransactionHandleT     txnHandle, 
        CL_INOUT    ClTxnAgentCookieT           *pCookie)
{
    clLogTrace("SER", "CLI",
            "Transaction[%lld] start",
            txnHandle);
    return CL_OK;
}

/* Stop callback */
ClRcT clTxnAgentStop(
        CL_IN       ClTxnTransactionHandleT     txnHandle, 
        CL_INOUT    ClTxnAgentCookieT           *pCookie)
{
    clLogTrace("SER", "CLI",
            "Transaction[%lld] stop",
            txnHandle);
    return CL_OK;
}
/* Agent-Debug Prepare Callback */
ClRcT clTxnAgentDebugJobPrepare(
        CL_IN       ClTxnTransactionHandleT     txnHandle, 
        CL_IN       ClTxnJobDefnHandleT         jobDefn, 
        CL_IN       ClUint32T                   jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT           *pCookie)
{
    ClRcT rc = CL_OK;
    ClTxnDebugDummyJobT     *pJobDefn;
    ClTxnDebugShadowMemoryT *pShadowMem;

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Callback to do job-prepare\n"));

    pJobDefn = (ClTxnDebugDummyJobT *) jobDefn;
    if (pJobDefn->index > CL_MAX_DATA_SIZE)
    {
        return (CL_ERR_INVALID_PARAMETER);
    }

    pShadowMem = (ClTxnDebugShadowMemoryT *) clHeapAllocate(sizeof(ClTxnDebugShadowMemoryT));
    *pCookie = (ClTxnAgentCookieT) pShadowMem;

    pShadowMem->prevValue = gSharedData[pJobDefn->index];
    switch (pJobDefn->op)
    {
        case CL_TXN_DUMMY_OP_ADD :
            pShadowMem->newValue = gSharedData[pJobDefn->index] + pJobDefn->value;
            break;

        case CL_TXN_DUMMY_OP_SUBTRACT:
            pShadowMem->newValue = gSharedData[pJobDefn->index] - pJobDefn->value;
            break;

        default:
            return (CL_ERR_INVALID_PARAMETER);
    }

    return (rc);
}

/* Agent-Debug COmmit Callback */
ClRcT clTxnAgentDebugJobCommit(
        CL_IN       ClTxnTransactionHandleT     txnHandle, 
        CL_IN       ClTxnJobDefnHandleT         jobDefn, 
        CL_IN       ClUint32T                   jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT           *pCookie)
{
    ClRcT rc = CL_OK;
    ClTxnDebugDummyJobT     *pJobDefn;
    ClTxnDebugShadowMemoryT *pShadowMem;

    pJobDefn = (ClTxnDebugDummyJobT *) jobDefn;
    pShadowMem = (ClTxnDebugShadowMemoryT *) *pCookie;
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Callback to do job-commit\n"));

    gSharedData[pJobDefn->index] = pShadowMem->newValue;

    return (rc);
}

/* Agent-debug rollback callback */
ClRcT clTxnAgentDebugRollback(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_IN       ClTxnJobDefnHandleT     jobDefn, 
        CL_IN       ClUint32T               jobDefnSize, 
        CL_INOUT    ClTxnAgentCookieT       *pCookie)
{
    ClRcT rc = CL_OK;
    ClTxnDebugDummyJobT     *pJobDefn;
    ClTxnDebugShadowMemoryT *pShadowMem;

    pJobDefn = (ClTxnDebugDummyJobT *) jobDefn;
    pShadowMem = (ClTxnDebugShadowMemoryT *) *pCookie;
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Callback to do job-rollback\n"));
    if (pJobDefn->index > CL_MAX_DATA_SIZE)
    {
        return (CL_OK);
    }

    gSharedData[pJobDefn->index] = pShadowMem->prevValue;
    return (rc);
}

/***********************************************************************************
 *                  CLI Command Implementations                                    *
 **********************************************************************************/
/**
  * Initialize transaction-service debug mode. Initialize client and dummy agent.
  */
ClRcT clCliTxnInit(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClUint32T   i;
    ClEoExecutionObjT   *pEoObj;
    ClVersionT version = {
        .releaseCode = 'B',
        .majorVersion = 1,
        .minorVersion = 1
    };

    CL_FUNC_ENTER();

    if (gInitFlag == 1)
    {
        clCliTxnStrPrint("Txn-Cli Interface is already initialized\n", retStr);
        return (rc);
    }

    if (CL_OK != clEoMyEoObjectGet(&pEoObj) )
    {
        clCliTxnStrPrint("Unable to retrieve My-Eo\n", retStr);
        return (CL_OK);
    }

    rc = clTxnClientInitialize(&version, 
                (ClTxnTransactionCompletionCallbackT) clCliTxnClientCallbackFn, 
                &gTxnCliClientHandle);
    if (CL_OK != rc)
    {
        clCliTxnStrPrint("Client Initialization Failed\n", retStr);
        return (CL_OK);
    }

    gSharedData = (ClUint32T *) clHeapAllocate(sizeof(ClUint32T) * CL_MAX_DATA_SIZE);
    for (i = 0; i < CL_MAX_DATA_SIZE; i++)
        gSharedData[i] = i;

    /* Initialize agent */
    rc = clTxnAgentInitialize(pEoObj, &version);
    if (CL_OK != rc)
    {
        clCliTxnStrPrint("Agent Initialization Failed\n", retStr);
        return (CL_OK);
    }

    rc = clTxnAgentServiceRegister(CL_TXN_DEBUG_SERVICE, gAgentDebugCallback, &gTxnCliAgentHandle);
    if (CL_OK != rc)
        clCliTxnStrPrint("Not able to register service with agent\n", retStr);
    else
        clCliTxnStrPrint("Initializatin Done\n", retStr);

    gInitFlag = 1;
    CL_FUNC_EXIT();
    return (CL_OK);
}


/* Create a dummy transaction for debugging purpose */
ClRcT clCliTxnTransactionCreate(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClTxnTransactionHandleT     txnHandle;
    ClUint32T       txnConfig = 0;

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }

    rc = clTxnTransactionCreate(txnConfig, (ClTxnTransactionHandleT *)&txnHandle);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Rc value of creating txn rc:0x%x\n", rc));


    /* Show transaction-id (number assigned) */
    if (CL_OK == rc)
    {
        *retStr = (char *) clHeapAllocate(100);
        snprintf(*retStr, 100, "New transaction created. Txn-Handle: %#llX\n", txnHandle);
    }
    else
    {
        clCliTxnStrPrint("Failed to create transaction\n", retStr);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* Add a dummy job for debugging purpose */
ClRcT clCliTxnDummyJobAdd(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClTxnTransactionHandleT txnHandle;
    ClTxnJobHandleT         jobHandle;
    ClTxnDebugDummyJobT     jobDefn;

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }

    /* Collect txn_id, job_operation_type, job_operation_arrayIndex */
    if (argc != 5)
    {
        clCliTxnStrPrint("Usage: addJob <txn_id> <op_type> <array_index> <value>\n"
                         "\ttxn_handle [HEX] - Transaction-Handle\n"
                         "\top_type [DEC] - [1:ADD], [2:SUBTRACT]\n"
                         "\tarray_index [DEC] - Index into test array\n"
                         "\tvalue [DEC] - New value used for operation\n", retStr);
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    sscanf(argv[1], "%p", (ClPtrT *)(&txnHandle));
    jobDefn.op = atoi(argv[2]);
    jobDefn.index = atoi(argv[3]);
    jobDefn.value = atoi(argv[4]);
    if ( (jobDefn.op > 2) || (jobDefn.op < 1) )
    {
        clCliTxnStrPrint("Usage: addJob <txn_id> <op_type> <array_index> <value>\n"
                         "\ttxn_handle [HEX] - Transaction-Handle\n"
                         "\top_type [DEC] - [1:ADD], [2:SUBTRACT]\n"
                         "\tarray_index [DEC] - Index into test array\n"
                         "\tvalue [DEC] - New value used for operation\n", retStr);
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    rc = clTxnJobAdd(txnHandle, 
                     (ClTxnJobDefnHandleT) &jobDefn, 
                     sizeof(ClTxnDebugDummyJobT), CL_TXN_DEBUG_SERVICE,
                     &jobHandle);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Rc value of adding job rc:0x%x\n", rc));

    /* Show result */
    if (CL_OK == rc)
    {
        *retStr = (char *) clHeapAllocate(100);
        snprintf(*retStr, 100, "New Job added. Job-Handle = %p\n", (ClPtrT)jobHandle);
    }
    else
    {
        clCliTxnStrPrint("Failed to add job: Invalid Txn-Handle\n", retStr);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* Delete the job from transaction */
ClRcT clCliTxnDummyJobDelete(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClTxnJobHandleT         jobHandle;
    ClTxnTransactionHandleT txnHandle;

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }

    if (argc != 3)
    {
        clCliTxnStrPrint("Usage: deleteJob <txn_id> <job_id>\n"
                         "\ttxn_id [HEX] - Transaction Handle\n"
                         "\tjob_id [HEX] - Job Handle\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Collect txn-id and job-id */
    sscanf(argv[1], "%p", (ClPtrT *) &txnHandle);
    sscanf(argv[2], "%p", (ClPtrT *) &jobHandle);

    rc = clTxnJobRemove(txnHandle, jobHandle); 
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Rc value of deleting job rc:0x%x\n", rc));

    /* Show results */
    if (CL_OK == rc)
        clCliTxnStrPrint("Job deleted\n", retStr);
    else
        clCliTxnStrPrint("Error while deleting job: Invalid job or txn handle\n",
                         retStr);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* Set component for transaction-job */
ClRcT clCliTxnComponentSet(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClTxnTransactionHandleT     txnHandle;
    ClTxnJobHandleT             jobHandle;
    ClIocAddressT               txnCompAddr;

    ClNameT                     compName = {
        .length = CL_MAX_NAME_LENGTH
    };

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }
    /* Collect txn-id, job-id  & ioc-address and orderNo */
    if (argc != 4)
    {
        clCliTxnStrPrint("Usage: setComponent <txn_handle> <job_handle> <comp_name>\n"
                         "\ttxn_handle [HEX] - Transaction Handle\n"
                         "\tjob_handle [HEX] - Job Handle\n"
                         "\tcomp_name [NAME] - Component Name\n", retStr);
        CL_FUNC_EXIT();
        return CL_OK;
    }

    memset(&(compName.value[0]), '\0', compName.length);

    sscanf(argv[1], "%p", (ClPtrT *)&txnHandle);
    sscanf(argv[2], "%p", (ClPtrT *)&jobHandle);
    sscanf(argv[3], "%s", &(compName.value[0]));

    rc = clCpmComponentAddressGet(clIocLocalAddressGet(), &compName, &txnCompAddr);

    rc = clTxnComponentSet( txnHandle, jobHandle, txnCompAddr, 0);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Rc value of setting component rc:0x%x\n", rc));

    if (CL_OK != rc)
        clCliTxnStrPrint("Failed to add component-address to txn-job\n", retStr);
    else
        clCliTxnStrPrint("Successfully added component addresss to txn-job\n", retStr);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* Run a given transaction */
ClRcT clCliTxnTransactionRun(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClTxnTransactionHandleT txnHandle;

    CL_FUNC_ENTER();
    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }

    if (argc != 2)
    {
        clCliTxnStrPrint("Usage: runTxn <txn_handle>\n"
                         "\ttxn_handle [HEX] - Transaction-Handle\n", retStr);
        CL_FUNC_EXIT();
        return (CL_OK);
    }

    /* Collect txn-id */
    sscanf(argv[1], "%p", (ClPtrT *)&txnHandle);


    rc = clTxnTransactionStartAsync( txnHandle);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Result of transaction is 0x%x comp-code:0x%x \n", rc, (CL_GET_CID(rc))));
    if (CL_OK != rc)
    {
        clCliTxnStrPrint("Transaction failed\n", retStr);
    }
    else
    {
        clCliTxnStrPrint("Transaction completed successfully\n", retStr);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/* Show transactions at the server end */
ClRcT clCliShowActiveTransactions(int argc, char **argv, char **retStr)
{
    ClRcT rc = CL_OK;
    ClUint32T   outputLen;
    ClUint32T   txnCount;
    ClBufferHandleT  output;
    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }
    /* - Set a walk function for displaying each transaction at the server-end
       - Put the header here. Header must contain
         | TxnId | Status | Num-of-jobs |
    */
    do {
        rc = clBufferCreate(&output);
        if (CL_OK != rc)
            break;

        /* Get the count of transaction */
        rc = clCntSizeGet(clTxnServiceCfg->pTxnEngine->activeTxnMap, &txnCount);
        if (CL_OK != rc)
            break;

        rc = clBufferNBytesWrite(output, (ClUint8T *) CL_TXN_CLI_DISPLAY_TXN_LIST_HEADER, 
                                        CL_TXN_CLI_DISPLAY_TXN_LIST_HEADER_SIZE);
        if (CL_OK != rc)
            break;

        rc = clBufferNBytesWrite(output, 
                                    (ClUint8T *) CL_TXN_CLI_DISPLAY_SHORT_SEP,
                                    CL_TXN_CLI_DISPLAY_SHORT_SEP_SIZE);
        if (CL_OK != rc)
            break;

        rc = clCntWalk(clTxnServiceCfg->pTxnEngine->activeTxnMap, _clCliAppendActiveTxn, 
                       (ClCntArgHandleT) output, sizeof(ClBufferHandleT));
        if (CL_OK != rc)
            break;

        if (txnCount)
        {
            rc = clBufferNBytesWrite(output, 
                                        (ClUint8T *) CL_TXN_CLI_DISPLAY_SHORT_SEP,
                                        CL_TXN_CLI_DISPLAY_SHORT_SEP_SIZE);
            if (CL_OK != rc)
                break;
        }

        rc = clBufferLengthGet(output, &outputLen);
        if (CL_OK != rc || (outputLen <= 0))
        {
            clTxnSerDbgPrint(retStr,
                    "Error in getting length of the out buffer");
            break;
        }

        *retStr = (char *) clHeapAllocate(outputLen + 1);
        memset(*retStr, '\0', outputLen + 1);
        rc = clBufferNBytesRead(output, (ClUint8T *) *retStr, &outputLen);
        if (CL_OK != rc)
            break;

    } while (0);

    clBufferDelete(&output);


    CL_FUNC_EXIT();
    return (rc);
}

/* Show details of a given transaction at the server end */
ClRcT clCliShowTransaction(int argc, char **argv, char **retStr)
{
    ClRcT                       rc = CL_OK;
    ClTxnActiveTxnT             *pActiveTxn = NULL;
    ClTxnTransactionIdT         txnId = {0};
    ClBufferHandleT             output;
    ClUint32T                   outputLen;
    ClCharT                     str[1024];

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Collect txn-id */
    if (argc != 2)
    {
        clCliTxnStrPrint("Usage: showTxnDetails <txnId>\n"
                         "\ttxnId [HEX] - Server transaction Id\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Collect txn-id */
    sscanf(argv[1], "%x", &(txnId.txnId)); /* if txnHandle is used directly, 
                                            core dumps can occur in the case of
                                            non-existent txns */

    txnId.txnMgrNodeAddress = clIocLocalAddressGet();

    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                            (ClCntKeyHandleT)&txnId, 
                            (ClCntDataHandleT*)&pActiveTxn);
    if(CL_OK != rc)
    {
       clTxnSerDbgPrint(retStr, "Failed to get txn[0x%x:0x%x] from activeDb, rc [0x%x]", 
               txnId.txnMgrNodeAddress, txnId.txnId, rc);
       return rc;
    }
    
    rc = clBufferCreate(&output);

    if (CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to create out-msg buffer. rc [0x%x]", 
                rc);
        return (rc);
    }

    str[0] = '\0';
    sprintf(str, "Transaction Description:\n"               \
                 "\tServer TxnId\t:\t0x%x:0x%x\n"           \
                 "\tClient TxnId\t:\t0x%x:0x%x\n"           \
                 "\tCurrentState\t:\t%s\n"                  \
                 "\tJob Count\t:\t0x%x\n"                   \
                 "\tConfiguration\t:\t0x%x\n", pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                 pActiveTxn->pTxnDefn->serverTxnId.txnId, pActiveTxn->pTxnDefn->clientTxnId.txnMgrNodeAddress,
                 pActiveTxn->pTxnDefn->clientTxnId.txnId, gCliTxnStatusStr[pActiveTxn->pTxnDefn->currentState], 
                 pActiveTxn->pTxnDefn->jobCount, (pActiveTxn->pTxnDefn->txnCfg & 0xf));

    clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));

    rc = clCntWalk(pActiveTxn->pTxnDefn->jobList, _clCliAppendJob, (ClCntArgHandleT)output, 
              sizeof(ClBufferHandleT));

    rc = clBufferLengthGet(output, &outputLen);

    if(CL_OK != rc || (outputLen <= 0))
    {
        clTxnSerDbgPrint(retStr, "Buffer length get failed, rc [0x%x]", rc);
        clBufferDelete(&output);
        return  rc;
    }

    *retStr = (char *) clHeapCalloc(1,outputLen + 1);
    if(!*retStr)
    {
        clTxnSerDbgPrint(retStr, "Failed to allocate memory of [%d]bytes", 
                outputLen + 1);
        clBufferDelete(&output);
        return rc;
    }

    memset(*retStr, '\0', outputLen + 1);
    rc = clBufferNBytesRead(output, (ClUint8T *) *retStr, &outputLen);

    clBufferDelete(&output);

    CL_FUNC_EXIT();
    return (rc);
}

/* Show details of transaction activity at the server end */
ClRcT clCliShowTransactionActivity(int argc, char **argv, char **retStr)
{
    ClRcT                       rc = CL_OK;
    ClTxnActiveTxnT             *pActiveTxn = NULL;
    ClTxnTransactionIdT         txnId = {0};
    ClBufferHandleT             output;
    ClUint32T                   outputLen;
    ClCharT                     str[1024];

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Collect txn-id */
    if (argc != 2)
    {
        clCliTxnStrPrint("Usage: showTxnActivity <txnId>\n"
                         "\ttxnId[HEX] - Server transaction Id\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    /* Collect txn-id */
    sscanf(argv[1], "%x", &(txnId.txnId)); /* if txnHandle is used directly, 
                                            core dumps can occur in the case of
                                            non-existent txns */

    txnId.txnMgrNodeAddress = clIocLocalAddressGet();

    rc = clCntDataForKeyGet(clTxnServiceCfg->pTxnEngine->activeTxnMap,
                            (ClCntKeyHandleT)&txnId, 
                            (ClCntDataHandleT*)&pActiveTxn);
    if(CL_OK != rc)
    {
       clTxnSerDbgPrint(retStr, "Failed to get txn[0x%x:0x%x] from activeDb, rc [0x%x]", 
               txnId.txnMgrNodeAddress, txnId.txnId, rc);
       return rc;
    }
    
    rc = clBufferCreate(&output);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create out-msg buffer. rc:0x%x", rc));
        CL_FUNC_EXIT();
        return (rc);
    }

    str[0] = '\0';
    sprintf(str, "Transaction Activity:\n"               \
                 "\tServer TxnId\t:\t0x%x:0x%x\n"           \
                 "\tClient TxnId\t:\t0x%x:0x%x\n"           \
                 "\tCurrentState\t:\t%s\n"                  \
                 "\tJob Count\t:\t0x%x\n"                   \
                 "\tConfiguration\t:\t0x%x\n", pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
                 pActiveTxn->pTxnDefn->serverTxnId.txnId, pActiveTxn->pTxnDefn->clientTxnId.txnMgrNodeAddress,
                 pActiveTxn->pTxnDefn->clientTxnId.txnId, gCliTxnStatusStr[pActiveTxn->pTxnDefn->currentState], 
                 pActiveTxn->pTxnDefn->jobCount, (pActiveTxn->pTxnDefn->txnCfg & 0xf));

    clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));

    /* Retrieve txn-log and append here */
    rc = clTxnRecoveryLogShow(pActiveTxn->pTxnDefn->serverTxnId, output);

    rc = clBufferLengthGet(output, &outputLen);
    if(CL_OK != rc || (outputLen <= 0))
    {
        clTxnSerDbgPrint(retStr, "Buffer length get failed, rc [0x%x]", rc);
        clBufferDelete(&output);
        return  rc;
    }

    *retStr = (char *) clHeapCalloc(1,outputLen + 1);
    if(!*retStr)
    {
        clTxnSerDbgPrint(retStr, "Failed to allocate memory of [%d]bytes", 
                outputLen + 1);
        clBufferDelete(&output);
        return rc;
    }

    memset(*retStr, 0, outputLen + 1);
    rc = clBufferNBytesRead(output, (ClUint8T *) *retStr, &outputLen);

    clBufferDelete(&output);

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT clCliShowTxnHistory(int argc, char **argv, char **retStr)
{
    ClRcT               rc = CL_OK;

#ifndef RECORD_TXN

    clCliTxnStrPrint("This feature is enabled if only txn server is compiled with RECORD_TXN", retStr);
    return (rc);

#else
    ClTxnTransactionIdT *pTxnId = NULL; 
    ClBufferHandleT     output = NULL;     
    ClUint32T           outputLen = 0;
    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }


    /* Collect txn-id */
    if (argc > 2)
    {
        clCliTxnStrPrint("Usage: showTxnHistory [txnId]\n"
                         "\ttxnId[HEX] - Client transaction Id\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }

    rc = clBufferCreate(&output);
    if(CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to create buffer, rc [0x%x]", rc);
        return rc;
    }
    if(1 == argc)
    {
        /* Display all stored transactions */
        pTxnId = NULL;
    }
    else
    {
        /* Display only the specific transaction */
        pTxnId = clHeapAllocate(sizeof(ClTxnTransactionIdT));
        if(!pTxnId)
        {
            clLogError("CLI", "DBG",
                    "Error in allocating memory");
            clBufferDelete(&output);
            return CL_ERR_NO_MEMORY;
        }
        sscanf(argv[1], "%x", &pTxnId->txnId);
        pTxnId->txnMgrNodeAddress = clIocLocalAddressGet();
    }

    rc = _clTxnReadRecords(pTxnId, output);
    if(CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to read records, rc [0x%x]", rc);
        goto free_buff;
    }
     
    rc = clBufferLengthGet(output, &outputLen); 
    if(CL_OK != rc || (outputLen <= 0))
    {
        clTxnSerDbgPrint(retStr, "Failed to get buffer length, rc [0x%x], leng [%d]", 
                rc, outputLen);
        goto free_buff;
    }
    
    *retStr = (char *) clHeapAllocate(outputLen + 1); 
    if(CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to allocate buffer of length [%d], rc [0x%x]", 
                outputLen+1, rc);
        goto free_buff;
    }
    memset(*retStr, '\0', outputLen + 1); 
    rc = clBufferNBytesRead(output, (ClUint8T *) *retStr, &outputLen);
    if(CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to read from buffer, rc [0x%x]", rc);
        goto free_buff;
    }

free_buff: 
    clBufferDelete(&output);
    clHeapFree(pTxnId);

    CL_FUNC_EXIT();
    return (rc);
#endif
}
ClRcT clCliTxnPlay(int argc, char **argv, char **retStr)
{
    ClRcT               rc = CL_OK;

#ifndef RECORD_TXN

    clCliTxnStrPrint("This feature is enabled if only txn server is compiled with RECORD_TXN", retStr);
    return (rc);

#else
    ClTxnTransactionIdT *pTxnId = NULL; 
    ClBufferHandleT     output = NULL;     
    ClTxnMessageHeaderT msgHeader;
    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (rc);
    }


    /* Collect txn-id */
    if (argc != 2)
    {
        clCliTxnStrPrint("Usage: txnPlay [txnId]\n"
                         "\ttxnId[HEX] - Client transaction Id\n", retStr);
        CL_FUNC_EXIT();
        return (rc);
    }
    

    rc = clBufferCreate(&output);
    if(CL_OK != rc)
    {
        clTxnSerDbgPrint(retStr, "Failed to create buffer, rc [0x%x]", rc);
        return rc;
    }

    /* Display only the specific transaction */
    pTxnId = clHeapAllocate(sizeof(ClTxnTransactionIdT));
    if(!pTxnId)
    {
        clLogError("CLI", "DBG",
                "Error in allocating memory");
        clBufferDelete(&output);
        return CL_ERR_NO_MEMORY;
    }
    sscanf(argv[1], "%x", &pTxnId->txnId);
    pTxnId->txnMgrNodeAddress = clIocLocalAddressGet();

    clTxnMutexLock(gTxnDbg.mtx);
    
    /* Rewind the file position indicator */
    rewind(gTxnDbg.fd);
    /* Find the transaction from the file */
    while(!feof(gTxnDbg.fd))
    {
        rc = __clTxnReadBuff(output, pTxnId);
        if(CL_OK == rc)
        {
            ClBufferHandleT outHandle = NULL;

            clLogInfo("CLI", "DBG",
                    "Found transaction [0x%x:0x%x]",
                    pTxnId->txnMgrNodeAddress,
                    pTxnId->txnId);

            rc = clBufferCreate(&outHandle);
            if(CL_OK != rc)
            {
                clLogError("CLI", "DBG",
                        "Error in buffer creation, rc [0x%x}", rc);
                clTxnSerDbgPrint(retStr, 
                        "Error in buffer creation, rc [0x%x}", rc);
                clTxnMutexUnlock(gTxnDbg.mtx);
                clBufferDelete(&output);
                clHeapFree(pTxnId);
                return rc;
            }
            memset(&msgHeader, 0, sizeof(ClTxnMessageHeaderT));


            /* Unlock here else the lock will be help foreever */
            clTxnMutexUnlock(gTxnDbg.mtx);

            /* Calling eo func callback doesnt work, no prints come in the CLI, return status is 60000 */
            rc = clXdrUnmarshallClTxnMessageHeaderT(output, &msgHeader);
            if(CL_OK != rc)
            {
                clLogError("CLI", "DBG",
                        "Error in unmarshalling message header, rc [0x%x]", rc);
                clBufferDelete(&output);
                clBufferDelete(&outHandle);
                clHeapFree(pTxnId);
                return rc;
            }
            /* Replay transaction */
            rc = _clTxnServiceClientReqProcess(msgHeader, output, outHandle, 1); 
            if(CL_OK != rc)
            {
                clTxnSerDbgPrint(retStr, "Error in replaying transaction [0x%x:0x%x], rc [0x%x]",
                        pTxnId->txnMgrNodeAddress, pTxnId->txnId, rc);
                clBufferDelete(&output);
                clBufferDelete(&outHandle);
                clHeapFree(pTxnId);
                return rc;

            }
            clLogInfo("CLI", "DBG", "Transaction [0x%x:0x%x] successfully queued for replaying",
                    pTxnId->txnMgrNodeAddress, pTxnId->txnId);
            clTxnSerDbgPrint(retStr, "Transaction [0x%x:0x%x] successfully queued for replaying \n",
                    pTxnId->txnMgrNodeAddress, pTxnId->txnId);
            clBufferDelete(&output);
            clBufferDelete(&outHandle);
            clHeapFree(pTxnId);
            return CL_OK;

        }
       else
       {
            /* Error for end of file reached */
            if(feof(gTxnDbg.fd))
                clTxnSerDbgPrint(retStr, "No records found, end of file reached");

            /* exits from the while loop after this and returns from the function */
       }
    }

    clTxnMutexUnlock(gTxnDbg.mtx);
    clBufferDelete(&output);
    clHeapFree(pTxnId);

    CL_FUNC_EXIT();
    return (rc);
#endif
}
#ifdef RECORD_TXN

static ClRcT   
_clTxnReadRecords(ClTxnTransactionIdT *pTxnId, 
                  ClBufferHandleT output)
{
    ClRcT   rc = CL_OK;
    ClUint8T    true = 0;
    ClBufferHandleT tempOut = NULL; /* temporary handle */
    ClUint32T   len = 0;

    clTxnMutexLock(gTxnDbg.mtx); /* Lock before proceeding to access fd */

    rewind(gTxnDbg.fd); /* Take the position indicator to the beginning of the file */
    if(feof(gTxnDbg.fd))
    {
        clLogError("CLI", "DBG",
                "End of file reached");
        clTxnMutexUnlock(gTxnDbg.mtx);
        return CL_ERR_NULL_POINTER;
    }

    rc = clBufferCreate(&tempOut); 
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in buffer creation, rc [0x%x]", rc);
        clTxnMutexUnlock(gTxnDbg.mtx);
        return rc;
    }

    while(!feof(gTxnDbg.fd) && !true)
    {
        if(pTxnId)
        {
            /* If reading one txn, the intermediate results are not required */
            rc = clBufferClear(tempOut);
            if(CL_OK != rc)
            {
                clLogInfo("CLI", "DBG",
                        "Error in clearing buffer, rc [0x%x]", rc);
                goto free_buff;
            }
        }
        rc = __clTxnReadRecord(tempOut, pTxnId, &true);
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Failed to read record rc [0x%x]", rc);
            goto free_buff;
        }
    }
        
    if(true || !pTxnId) 
    {
        rc = clBufferLengthGet(tempOut, &len);
        if(CL_OK != rc || (len <= 0) )
        {
            clLogError("CLI", "DBG",
                    "Failed to get lenth of buffer, rc [0x%x]", rc);
            goto free_buff;
        }
        rc = clBufferToBufferCopy(tempOut, 0, output, len);
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Failed to read record for txnId [0x%x]", rc);
            goto free_buff;
        }
    }

free_buff:
    clTxnMutexUnlock(gTxnDbg.mtx);
    clBufferDelete(&tempOut);

    return rc;
}

static ClRcT   
__clTxnReadRecord(ClBufferHandleT output,
                 ClTxnTransactionIdT *pTxnId, 
                 ClUint8T *pTrue)
{
    ClRcT               rc = CL_OK;
    ClUint32T           i = 0;
    ClTimeT             tstamp = 0;
    ClCharT             str[512] = {0};
    ClUint32T           len = 0;
    ClUint8T            *flatbuff = NULL;
    ClTxnMessageHeaderT msgHeader;
    ClTxnDefnT          *pTxnDefn = NULL;
    ClBufferHandleT     tempbuff = NULL;
    ClUint32T           hr = 0, min = 0, ss = 0;

    memset(&msgHeader, 0, sizeof(ClTxnMessageHeaderT));


    rc = clBufferCreate(&tempbuff);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in tempbuff creation, rc [0x%x]", rc);
        return rc;
    }

    /* Read timestamp */
    
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of time stamp from txnRecords", 
            fread(&tstamp, 1, sizeof(ClTimeT), gTxnDbg.fd));
    if(feof(gTxnDbg.fd))
    {
        clBufferDelete(&tempbuff);
        return CL_OK; /* return OK for this since this is the last record */ 
    }

    clLogInfo("CLI", "DBG",
            "time stamp read[%lld]", tstamp);
            
    /* Getting some problem with clOsalStopWatchTimeGet(), might have to use clOsalTimeOfDayGet() */
    tstamp = (ClTimeT)(tstamp/1000000); /* Convert to seconds */

    min = tstamp/60;

    ss = tstamp%60;
    hr = min/60;
    min = min%60;
    
#define START_MARKER "========================================"
    clLogInfo("CLI", "DBG",  "\n\nTime stamp [%lld], [%d:%d:%d]\n", 
            tstamp, hr, min, ss);
    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str) - 1, "\n"START_MARKER"\nTime stamp [%d:%d:%d]\n", 
            hr, min, ss);

    rc = clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Failed to write into buffer, rc [0x%x]", rc);
        clBufferDelete(&tempbuff);
        return rc;
    }

    /* Read buffer length */
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of length field from txnRecords", 
            fread(&len, 1, sizeof(ClUint32T), gTxnDbg.fd));
    if(feof(gTxnDbg.fd))
    {
        clBufferDelete(&tempbuff);
        return CL_ERR_NOT_EXIST;
    }

    clLogInfo("CLI", "DBG",
            "Length of msgheader received from file [%d]", len);
    if(len <= 0)
    {
        clLogInfo("CLI", "DBG",
                "No msgheader to read");
        clBufferDelete(&tempbuff);
        return rc;
    }
    flatbuff = clHeapAllocate(len);
    if(!flatbuff)
    {
        clLogError("CLI", "DBG",
                "Error in allocating memory");
        clBufferDelete(&tempbuff);
        return CL_ERR_NO_MEMORY;
    }
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of length field from txnRecords", 
            fread(flatbuff, 1, len, gTxnDbg.fd));
    rc = clBufferNBytesWrite(tempbuff, flatbuff, len);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in writing [%d] bytes to tempbuff, rc [0x%x]",
                len, rc);
        goto free_buff;
    }


    rc = clXdrUnmarshallClTxnMessageHeaderT(tempbuff, &msgHeader);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in unmarshalling message header, rc [0x%x]", rc);
        goto free_buff;
    }
    clLogInfo("CLI", "DBG", "Message Header\n\tVersion [%c, 0x%x, 0x%x]\n"
            "\tSource Address [0x%x, 0x%x]\n"
            "\tMessage Count [%d]\n", msgHeader.version.releaseCode, 
            msgHeader.version.majorVersion, msgHeader.version.minorVersion, 
            msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId,
            msgHeader.msgCount);
    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str) - 1, "Message Header\n\tVersion [%c, 0x%x, 0x%x]\n"
            "\tSource Address [0x%x, 0x%x]\n"
            "\tMessage Count [%d]\n", msgHeader.version.releaseCode, 
            msgHeader.version.majorVersion, msgHeader.version.minorVersion, 
            msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId,
            msgHeader.msgCount);
    rc = clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Failed to write into buffer, rc [0x%x]", rc);
        goto free_buff;
    }


    /* Read txnDefn */
    for(i = 0; i < (msgHeader.msgCount) && !(*pTrue); i++)
    {
        ClTxnCmdT   tCmd;

        pTxnDefn = NULL;
        memset(&tCmd, 0, sizeof(ClTxnCmdT));

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(tempbuff, &tCmd); /* Just skip this */
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Error in unmarshalling cmd, rc [0x%x]", rc);
            goto free_buff;
        }
        
        rc = clTxnStreamTxnCfgInfoUnpack(tempbuff, &pTxnDefn);
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Error in getting txndefn from the buffer, rc [0x%x]", rc);
            goto free_buff;
        }
        if(pTxnId)
        {
            if(!memcmp(pTxnId, &pTxnDefn->clientTxnId, sizeof(ClTxnTransactionIdT)))
            {
                *pTrue = CL_TRUE;
                clLogInfo("CLI", "DBG",
                        "Found transaction [0x%x:0x%x] to show details", 
                        pTxnId->txnMgrNodeAddress, 
                        pTxnId->txnId);
            }
            else
            {
                /* Not this record, return from here */
                rc = CL_OK;
                goto free_buff;
            }
        
        }
        clLogInfo("CLI", "DBG", "Txn Defn\n\tClientId [0x%x:0x%x]\n"
                "\tServerId [0x%x:0x%x]\n"
                "\tState [%d]\n" 
                "\tJob count [%d]", 
                pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId,
                pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId,
                pTxnDefn->currentState, pTxnDefn->jobCount);
        memset(str, 0, sizeof(str));
        snprintf(str, sizeof(str) - 1, "Txn Defn\n\tClientId [0x%x:0x%x]\n"
                "\tServerId [0x%x:0x%x]\n"
                "\tState [%d]\n" 
                "\tJob count [%d]\n", 
                pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId,
                pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId,
                pTxnDefn->currentState, pTxnDefn->jobCount);
        rc = clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Failed to write into buffer, rc [0x%x]", rc);
            goto free_buff;
        }

        rc = clCntWalk(pTxnDefn->jobList, _clTxnCliJobWalk, 
                  (ClCntArgHandleT )output, sizeof(ClBufferHandleT));
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Failed to walk joblist, rc [0x%x]", rc);
            goto free_buff;
        }

    }
free_buff:
    clBufferDelete(&tempbuff);
    clHeapFree(flatbuff);
    return rc;
}
static ClRcT   
__clTxnReadBuff(ClBufferHandleT output,
                 ClTxnTransactionIdT *pTxnId) 
{
    ClRcT               rc = CL_OK;
    ClUint32T           i = 0;
    ClTimeT             tstamp = 0;
    ClUint32T           len = 0;
    ClUint8T            *flatbuff = NULL;
    ClTxnMessageHeaderT msgHeader;
    ClTxnDefnT          *pTxnDefn = NULL;
    ClBufferHandleT     tempbuff = NULL;

    memset(&msgHeader, 0, sizeof(ClTxnMessageHeaderT));


    if(!pTxnId)
    {
        clLogError("CLI", "DBG",
                "txnId passed is NULL");
        return CL_ERR_NULL_POINTER;
    }
    rc = clBufferCreate(&tempbuff);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in tempbuff creation, rc [0x%x]", rc);
        return rc;
    }

    /* Read timestamp */
    
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of time stamp from txnRecords", 
            fread(&tstamp, 1, sizeof(ClTimeT), gTxnDbg.fd));
    CHECK_FD(gTxnDbg.fd);

    clLogInfo("CLI", "DBG",
            "time stamp read[%lld]", tstamp);
            
    /* Read buffer length */
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of length field from txnRecords", 
            fread(&len, 1, sizeof(ClUint32T), gTxnDbg.fd));
    CHECK_FD(gTxnDbg.fd);

    clLogInfo("CLI", "DBG",
            "Length of msgheader received from file [%d]", len);
    if(len <= 0)
    {
        clLogInfo("CLI", "DBG",
                "No msgheader to read");
        clBufferDelete(&tempbuff);
        return CL_ERR_DOESNT_EXIST;
    }
    flatbuff = clHeapAllocate(len);
    if(!flatbuff)
    {
        clLogError("CLI", "DBG",
                "Error in allocating memory");
        clBufferDelete(&tempbuff);
        return CL_ERR_NO_MEMORY;
    }
    clLogInfo("CLI", "DBG", "Read [%zd]bytes of length field from txnRecords", 
            fread(flatbuff, 1, len, gTxnDbg.fd));

    rc = clBufferNBytesWrite(tempbuff, flatbuff, len);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in writing [%d] bytes to tempbuff, rc [0x%x]",
                len, rc);
        goto free_buff;
    }


    rc = clXdrUnmarshallClTxnMessageHeaderT(tempbuff, &msgHeader);
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Error in unmarshalling message header, rc [0x%x]", rc);
        goto free_buff;
    }
    clLogInfo("CLI", "DBG", "Message Header\n\tVersion [%c, 0x%x, 0x%x]\n"
            "\tSource Address [0x%x, 0x%x]\n"
            "\tMessage Count [%d]\n", msgHeader.version.releaseCode, 
            msgHeader.version.majorVersion, msgHeader.version.minorVersion, 
            msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId,
            msgHeader.msgCount);

    /* This is returned when there are no matching txn */
    rc = CL_ERR_NOT_EXIST;
    /* Read txnDefn */
    for(i = 0; i < msgHeader.msgCount; i++)
    {
        ClTxnCmdT   tCmd;

        pTxnDefn = NULL;
        memset(&tCmd, 0, sizeof(ClTxnCmdT));

        rc = VDECL_VER(clXdrUnmarshallClTxnCmdT, 4, 0, 0)(tempbuff, &tCmd); /* Just skip this */
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Error in unmarshalling cmd, rc [0x%x]", rc);
            goto free_buff;
        }
        
        rc = clTxnStreamTxnCfgInfoUnpack(tempbuff, &pTxnDefn);
        if(CL_OK != rc)
        {
            clLogError("CLI", "DBG",
                    "Error in getting txndefn from the buffer, rc [0x%x]", rc);
            goto free_buff;
        }
        clLogInfo("CLI", "DBG", "Txn Defn\n\tclientId [0x%x:0x%x]\n"
                "\tserverId [0x%x:0x%x]\n"
                "\tState [%d]\n" 
                "\tJob Count [%d]", 
                pTxnDefn->clientTxnId.txnMgrNodeAddress, pTxnDefn->clientTxnId.txnId,
                pTxnDefn->serverTxnId.txnMgrNodeAddress, pTxnDefn->serverTxnId.txnId,
                pTxnDefn->currentState, pTxnDefn->jobCount);
        if(!memcmp(pTxnId, &pTxnDefn->clientTxnId, sizeof(ClTxnTransactionIdT)))
        {
            rc = clBufferToBufferCopy(tempbuff, 0, output, len);
            if(CL_OK != rc)
            {
                clLogError("CLI", "DBG",
                        "Error in buffer to buffer copy, rc [0x%x]", rc);
                goto free_buff;
            }
            rc = CL_OK;
            goto free_buff;
        }
        else
            rc = CL_ERR_NOT_EXIST;
    }
free_buff:
    clBufferDelete(&tempbuff);
    clHeapFree(flatbuff);
    return rc;
}
ClRcT _clTxnCliJobWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen)
{
    ClRcT   rc = CL_OK;
    ClCharT     str[256] = {0};
    ClBufferHandleT output = NULL;
    ClTxnAppJobDefnT   *pJobDefn = NULL; 

    output = (ClBufferHandleT)userArg;
    pJobDefn = (ClTxnAppJobDefnT *)userData;


    clLogInfo("CLI", "DBG", "Job Defn\n\tJobId [0x%x]\n"
            "\tState [%d]\n" 
            "\tApp Job Size [%d]\n" 
            "\tComp Count [%d]", 
            pJobDefn->jobId.jobId, pJobDefn->currentState,
            pJobDefn->appJobDefnSize, pJobDefn->compCount);

    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str) - 1, "Job Defn\n\tJobId [0x%x]\n"
            "\tState [%d]\n" 
            "\tApp Job Size [%d]\n" 
            "\tComp Count [%d]\n", 
            pJobDefn->jobId.jobId, pJobDefn->currentState,
            pJobDefn->appJobDefnSize, pJobDefn->compCount);

    rc = clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Failed to write into buffer, rc [0x%x]", rc);
        return rc;
    }

      
    rc = clCntWalk(pJobDefn->compList, _clTxnCliCompWalk, 
              (ClCntArgHandleT )output, sizeof(ClBufferHandleT));
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Failed to walk compList, rc [0x%x]", rc);
        return rc;
    }

    

    return rc;
}
ClRcT _clTxnCliCompWalk(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen)
{
    ClRcT   rc = CL_OK;
    ClCharT     str[256] = {0};
    ClBufferHandleT output = NULL;
    ClTxnComponentInfoT *pCompInfo = NULL; 

    output = (ClBufferHandleT)userArg;
    pCompInfo = (ClTxnComponentInfoT*)userData;

    clLogInfo("CLI", "DBG", "CompInfo\n\tAddress[0x%x:0x%x]\n",
            pCompInfo->compAddress.nodeAddress,
            pCompInfo->compAddress.portId);

    memset(str, 0, sizeof(str));
    snprintf(str, sizeof(str) - 1, "CompInfo\n\tAddress[0x%x:0x%x]\n",
            pCompInfo->compAddress.nodeAddress,
            pCompInfo->compAddress.portId);

    rc = clBufferNBytesWrite(output, (ClUint8T *)str, strlen(str));
    if(CL_OK != rc)
    {
        clLogError("CLI", "DBG",
                "Failed to write into buffer, rc [0x%x]", rc);
        return rc;
    }

    return rc;
}
#endif
ClRcT clCliShowTestData(int argc, char **argv, char **retStr)
{
    ClBufferHandleT  output;
    char                    *outStr;
    ClUint32T               count;

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (CL_OK);
    }
    clBufferCreate(&output);

    outStr = (char *) clHeapCalloc(1,200);
    snprintf(outStr, 200, " Item-No\t\tData-Value\n");
    clBufferNBytesWrite(output, (ClUint8T *)outStr, strlen(outStr));

    for (count = 0; count < CL_MAX_DATA_SIZE; count++)
    {
        snprintf(outStr, 200, "  %d  \t\t  %d\n", count, gSharedData[count]);
        clBufferNBytesWrite(output, (ClUint8T *)outStr, strlen(outStr));
    }
    clHeapFree(outStr);

    clBufferLengthGet(output, &count);

    *retStr = (char *) clHeapAllocate(count + 1);
    memset(*retStr, '\0', (count + 1));
    clBufferNBytesRead(output, (ClUint8T *)*retStr, &count);

    clBufferDelete(&output);

    return (CL_OK);
}

ClRcT clCliShowAgentState(int argc, char **argv, char **retStr)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT         output;
    ClUint32T               count;
    ClIocAddressT           txnCompAddr;
    ClNameT                     compName = {
        .length = CL_MAX_NAME_LENGTH
    };
    ClRmdOptionsT           rmdOptions = CL_RMD_DEFAULT_OPTIONS;
    ClUint32T               rmdFlag;

    CL_FUNC_ENTER();

    if (gInitFlag != 1)
    {
        clCliTxnStrPrint("Txn Interface is not initialized\n", retStr);
        return (CL_OK);
    }
    if (argc != 2)
    {
        clCliTxnStrPrint("Usage: showAgentState <comp_name>\n"
                         "\tcomp_name [NAME] - Component Name\n", retStr);
        CL_FUNC_EXIT();
        return CL_OK;
    }

    memset(&(compName.value[0]), '\0', compName.length);
    sscanf(argv[1], "%s", &(compName.value[0]));
    rc = clCpmComponentAddressGet(clIocLocalAddressGet(), &compName, &txnCompAddr);

    if(rc == CL_OK)
    {
        rc = clBufferCreate(&output);

        if (CL_OK == rc)
        {
        
            rmdFlag = CL_RMD_CALL_ATMOST_ONCE | CL_RMD_CALL_NEED_REPLY;
            rmdOptions.timeout = CL_TXN_RMD_DFLT_TIMEOUT;
            rmdOptions.retries = CL_TXN_RMD_DFLT_RETRIES;
            rmdOptions.priority = CL_RMD_DEFAULT_PRIORITY;
        
            rc = clRmdWithMsg(txnCompAddr,CL_TXN_AGENT_MGR_DBG_MSG_RECV,
                                0,
                                (ClBufferHandleT) output, 
                                rmdFlag,
                                &rmdOptions, 
                                NULL);
            if(rc == CL_OK)
            {
                
                rc = clBufferLengthGet(output, &count);
                if(CL_OK != rc || (count <= 0))
                {
                    clTxnSerDbgPrint(retStr, "Failed to get length of buffer, rc [0x%x]", rc);
                    return rc;
                }

                *retStr = (char *) clHeapAllocate(count + 1);
                if(*retStr)
                {
                    memset(*retStr, '\0', (count + 1));
                    rc = clXdrUnmarshallArrayClUint8T(output,*retStr,count);
                    if (rc != CL_OK)
                    {
                        clTxnSerDbgPrint(retStr, "Failed to unmarshall ClUint8T array. rc [0x%x]", rc);
                        return CL_OK;
                    }
                }
            }
            else
            {
                clTxnSerDbgPrint(retStr, 
                        "Rmd to agent[0x%x:0x%x] failed with error [0x%x]", 
                        txnCompAddr.iocPhyAddress.nodeAddress,
                        txnCompAddr.iocPhyAddress.portId, rc);
            }
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to create out-msg buffer. rc:0x%x", rc));
        }
    }
    else
    {
        clTxnSerDbgPrint(retStr, " Component is not valid, rc [0x%x]", rc);
    }
    clBufferDelete(&output);

    CL_FUNC_EXIT();
    return (rc);
}

/* Internal (private) functions */
ClRcT _clCliAppendActiveTxn(CL_IN ClCntKeyHandleT userKey, 
                            CL_IN ClCntDataHandleT userData, 
                            CL_IN ClCntArgHandleT userArg, 
                            CL_IN ClUint32T len)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT  outMsgHandle;
    ClTxnActiveTxnT         *pActiveTxn;
    ClCharT                 pOutMsg[1024];
    CL_FUNC_ENTER();

    outMsgHandle = (ClBufferHandleT) userArg;

    pActiveTxn = (ClTxnActiveTxnT *) userData;

    if (NULL == pActiveTxn)
    {
        CL_FUNC_EXIT();
        return (rc);
    }

    pOutMsg[0] = '\0';
    sprintf(pOutMsg,
             "    0x%x:0x%x \t|     0x%x:0x%x \t|      %p  \t|     %30s \n",
             pActiveTxn->pTxnDefn->serverTxnId.txnMgrNodeAddress, 
             pActiveTxn->pTxnDefn->serverTxnId.txnId, 
             pActiveTxn->pTxnDefn->clientTxnId.txnMgrNodeAddress, 
             pActiveTxn->pTxnDefn->clientTxnId.txnId, 
             (void *)pActiveTxn->pTxnDefn, gCliTxnStatusStr[pActiveTxn->pTxnDefn->currentState]);

    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *)pOutMsg, 
                                    strlen((char *)pOutMsg));
    CL_FUNC_EXIT();
    return(rc);
}

ClRcT _clCliAppendJob(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen)
{
    ClRcT                   rc = CL_OK;
    ClBufferHandleT  outMsgHandle;
    ClCharT                 str[1024];
    ClTxnAppJobDefnT        *pAppJobDefn;

    CL_FUNC_ENTER();
    outMsgHandle = (ClBufferHandleT) userArg;

    pAppJobDefn = (ClTxnAppJobDefnT *) userData;

    str[0] = '\0';
    sprintf(str, "\n\tJob Details:\n"               \
                 "\t\tJob-Id      \t:\t0x%x\n"  \
                 "\t\tService Type\t:\t0x%x\n"      \
                 "\t\tConfiguration\t:\t0x%x\n"     \
                 "\t\tCurrent State\t:\t%s\n"       \
                 "\t\tComponent Cnt\t:\t0x%x\n"     \
                 "\t\tComponent List\t:\n", 
                 pAppJobDefn->jobId.jobId,
                 pAppJobDefn->serviceType, pAppJobDefn->jobCfg, 
                 gCliTxnStatusStr[pAppJobDefn->currentState], pAppJobDefn->compCount);

    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *)str, strlen(str) ); 

    /* Prepare to display list of components */ 
    rc = clCntWalk(pAppJobDefn->compList, _clCliAppendCompList, 
                  (ClCntArgHandleT )outMsgHandle, sizeof(ClBufferHandleT));

    CL_FUNC_EXIT();
    return (rc);
}

/**
 * Internal API
 * This function appends component address to the message-buffer used for
 * display purpose.
 */
ClRcT _clCliAppendCompList(
        CL_IN   ClCntKeyHandleT     userKey, 
        CL_IN   ClCntDataHandleT    userData, 
        CL_IN   ClCntArgHandleT     userArg, 
        CL_IN   ClUint32T           dataLen)
{
    ClRcT                   rc  = CL_OK;
    ClTxnAppComponentT      *pAppComp;
    ClBufferHandleT  outMsgHandle;
    ClCharT                 str[512];

    CL_FUNC_ENTER();

    outMsgHandle = (ClBufferHandleT) userArg;
    pAppComp = (ClTxnAppComponentT *) userData;

    str[0] = '\0';
    sprintf(str, "\t\t\tAddr\t:\t[0x%x:0x%x]\n"     \
                 "\t\t\tConfig\t:\t0x%x\n", pAppComp->appCompAddress.nodeAddress,
                 pAppComp->appCompAddress.portId, pAppComp->configMask);

    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *)str, strlen(str));

    CL_FUNC_EXIT();
    return (rc);
}



