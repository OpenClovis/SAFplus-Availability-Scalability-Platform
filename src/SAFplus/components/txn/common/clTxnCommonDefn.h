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
 * File        : clTxnCommonDefn.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This header files contains common definitions used in transaction
 * management. These definitions are shared among client, agent and
 * transaction manager.
 *
 *
 *****************************************************************************/


#ifndef _CL_TXN_COMMON_DEF_H
#define _CL_TXN_COMMON_DEF_H

#include <clCommon.h>
#include <clOsalApi.h>
#include <clIocApi.h>
#include <clEoApi.h>
#include <clRmdApi.h>

#include <clCntApi.h>
#include <clLogApi.h>

#include <clRmdIpi.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/**
 * Transaction Manager/Service RMD Function-Id for receiving client-request
 */
#define CL_TXN_SERVICE_CLIENT_REQ_RECV          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)

/**
 * Transaction Manager/Service RMD Function-Id for receiving agent-response.
 */
#define CL_TXN_SERVICE_AGENT_RESP_RECV          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)
/**
 * Transaction Agent RMD Function-Id for receiving debug CLI command through txn manager.
 */
#define CL_TXN_AGENT_MGR_DBG_MSG_RECV               CL_EO_GET_FULL_FN_NUM(CL_TXN_CLIENT_TABLE_ID, 3)

/**
 * Transaction Agent RMD Function-Id for receiving txn manager command.
 */
#define CL_TXN_AGENT_MGR_CMD_RECV               CL_EO_GET_FULL_FN_NUM(CL_TXN_CLIENT_TABLE_ID, 2)
/**
 * Transaction Client RMD Function-Id for receiving txn manager response.
 */
#define CL_TXN_CLIENT_MGR_RESP_RECV             CL_EO_GET_FULL_FN_NUM(CL_TXN_CLIENT_TABLE_ID, 1)

#define CL_TXN_COMP_INIT_STATE                  0x00U
#define CL_TXN_COMP_NOT_PREPARED                0x01U
#define CL_TXN_COMP_1PC_NOT_COMMITTED           0x02U
#define CL_TXN_COMP_2PC_NOT_COMMITTED           0x04U
#define CL_TXN_COMP_NOT_ROLLED_BACK             0x08U
#define CL_TXN_COMP_UNKNOWN_STATE               0x0cU
                                 

#define CL_TXN_NUM_BUCKETS                      10
#define CL_TXN_RMD_DFLT_TIMEOUT                 1000
#define CL_TXN_RMD_DFLT_RETRIES                 CL_RMD_DEFAULT_RETRIES 

#define CL_TXN_JOB_PROCESSING_TIME              CL_RMD_DEFAULT_TIMEOUT /* Milliseconds */

#define CL_TXN_JOB_READ_TIME                    CL_RMD_DEFAULT_TIMEOUT /* Milliseconds */

#define CL_TXN_PROCESSING_TIME                  (CL_TXN_JOB_PROCESSING_TIME * 5 + 1000U) /* Seconds */

#define CL_TXN_COMMON_ID                        0x1 /* tId used for session creation at server */


#define     CL_TXN_NULL_CHECK_RETURN(ptr, rc, msg)              \
    do {                                                        \
        if (NULL == ptr)                                        \
        {                                                       \
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, msg);                \
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_NO_MEMORY)      \
            {                                                   \
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, \
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED); \
            }                                                   \
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER)   \
            {                                                   \
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL, \
                           CL_LOG_MESSAGE_0_NULL_ARGUMENT); \
            }                                                   \
            CL_FUNC_EXIT();                                     \
            return (CL_TXN_RC(rc));                             \
        }                                                       \
    } while(0)

/* Macro to check for value and return */
#define     CL_TXN_RETURN_RC(rc, msg)                           \
    do {                                                        \
        if (CL_OK != rc)                                        \
        {                                                       \
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, msg);                          \
            CL_FUNC_EXIT();                                        \
            return (CL_TXN_RC(rc));                             \
        }                                                       \
        CL_FUNC_EXIT();                                            \
        return (CL_OK);                                         \
    } while(0)

/* Macro to check for error and return */
#define     CL_TXN_ERR_RET_ON_ERROR(rc, msg)                    \
    do {                                                        \
        if (CL_OK != rc)                                        \
        {                                                       \
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, msg);                          \
            CL_FUNC_EXIT();                                        \
            return (CL_TXN_RC(rc));                             \
        }                                                       \
    } while(0)

#define     CL_TXN_ERR_WAITING_FOR_RESPONSE     0x1


/*********************************************
 * This macro logs the contents of msgHeader
 * *******************************************/
#define clTxnDumpMsgHeader(msgHeader)                           \
    do {                                                        \
        clLogMultiline(CL_LOG_TRACE, NULL, NULL,\
                "Version\t[%c,0x%x,0x%x]\nmsgType\t[%d]\nsrcAddr\t[0x%x:0x%x]\n"\
                "msgCount\t[%d]\n", \
                msgHeader.version.releaseCode, msgHeader.version.majorVersion, msgHeader.version.minorVersion, \
                msgHeader.msgType, msgHeader.srcAddr.nodeAddress, msgHeader.srcAddr.portId, \
                msgHeader.msgCount);\
    }while(0)

#ifdef RECORD_TXN
#define clTxnRecordData(data, size, mtx, fd)     do { \
                fseek(fd, 0L, SEEK_END); \
                clLogInfo("SER", "DBG", "[%d] bytes out of [%d] recorded successfully", \
                        fwrite(data, 1, size, fd), size); \
}while(0)

/* Dont use this in functions where gTxnDbg.mtx is held */
#define CHECK_FD(fd) do { \
    if(feof(fd)) \
    { \
        clLogInfo("SER", "DBG", "End of file reached"); \
        return CL_ERR_NOT_EXIST; \
    } \
}while(0)
#endif

/* This macro takes a buffer handle from the debug cli function and writes a string to it
 */
#define clTxnCliDbgPrint(hdl, ... ) do { \
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
 
/* This macro takes a ClCharT handle and write to it the __VA_ARGS__ argument 
 */
#define clTxnSerDbgPrint(hdl, ... ) do { \
    ClCharT temp[1024]; \
    snprintf(temp, sizeof(temp) - 1, __VA_ARGS__); \
    *hdl = (ClCharT *)clHeapAllocate(strlen(temp)+1); \
    if(*hdl) \
    { \
        strncpy(*hdl, temp, strlen(temp)); \
    } \
}while(0)
/******************************************************************************
 *  Data Types 
 *****************************************************************************/

#ifdef RECORD_TXN

typedef struct
{
    FILE    *fd;
    ClOsalMutexIdT  mtx; /* This is required otherwise the replay functionality will change the file positioning */
}ClTxnDbgT;


#endif
/* The type of handle to job definition */
typedef ClPtrT  ClTxnJobDefnT;

/**
 *  Transaction manager and agent maintain last known state of each other.
 *  Each agent refresh the timer used for aborting current transaction if
 *  manager fails to respond within stipulated time.
 */
typedef enum 
{
    CL_TXN_STATE_PRE_INIT = 1,    /* This is for client-side usage */
    CL_TXN_STATE_ACTIVE,          /* This is for client-side usage */
    CL_TXN_STATE_PREPARING,
    CL_TXN_STATE_PREPARED,
    CL_TXN_STATE_COMMITTING,
    CL_TXN_STATE_COMMITTED,
    CL_TXN_STATE_MARKED_ROLLBACK,
    CL_TXN_STATE_ROLLING_BACK,
    CL_TXN_STATE_ROLLED_BACK,
    CL_TXN_STATE_RESTORED,
    CL_TXN_STATE_UNKNOWN
} ClTxnTransactionStateT;

/**
 * Txn-Manager issues following commands to txn-agents while coordinating
 * execution of an active txn-job.
 */
typedef enum 
{
    CL_TXN_CMD_INVALID      = 0,
    CL_TXN_CMD_PROCESS_TXN, /* Command used by client to mgr to process txn */
    CL_TXN_CMD_RESPONSE,
    CL_TXN_CMD_CANCEL_TXN,  /* Command used by client to mgr to cancel txn */
    CL_TXN_CMD_INIT,        /* Command used by mgr to agent to read job-defn */
    CL_TXN_CMD_PREPARE,     /* 2PC Protocol command to prepare */
    CL_TXN_CMD_1PC_COMMIT,  /* 1PC Protocol command to commit */
    CL_TXN_CMD_2PC_COMMIT,  /* 2PC Protocol command to commit */
    CL_TXN_CMD_ROLLBACK,    /* 2PC protocol command to rollback */
    CL_TXN_CMD_REMOVE_JOB,  /* Command to remove job-defn from agent-db */
    CL_TXN_CMD_READ_JOB     /* COR Specific */
} ClTxnMgmtCommandT;


/** A transaction need to be uniquely identified with in the system.Such a identification
 *  defines a context in which multiple jobs can be defined by the client.
 *
 *  For this purpose, a transaction-id is created as a combination of
 *  node-address where transaction-manager is running (which is responsible for
 *  assigning txn-id) and locally unique txn-id.
 */
typedef struct 
{
    ClIocNodeAddressT   txnMgrNodeAddress;
    ClUint32T           txnId;
} ClTxnTransactionIdT;


/** Transaction manager assigns unique Id to each job defined by the client.
 *  A job is identified with in the context of transaction-context defined earlier.
 */
typedef struct {
    ClTxnTransactionIdT unused_txnId;
    ClUint32T           jobId;
} ClTxnTransactionJobIdT;

/**
 * Following is the generic job definition used in transaction-management.
 */
typedef struct
{
    ClTxnTransactionJobIdT  jobId;
    ClTxnTransactionStateT  currentState;
    ClInt32T                serviceType;
    ClUint8T                jobCfg;
    ClUint32T               appJobDefnSize;
    ClTxnJobDefnT           appJobDefn;
    ClUint32T               compCount;
    ClCntHandleT            compList;       /* List of particapting components */
    ClHandleT               cookie;         /* Cookie used at the agent-end */
} ClTxnAppJobDefnT;

typedef ClTxnAppJobDefnT*   ClTxnAppJobDefnPtrT;

/**
 * Following is the definition of application component involved in
 * transaction. This information is supplied by client.
 */
typedef struct
{
    ClIocPhysicalAddressT   appCompAddress;     /* Physical address of component involved */
    ClUint8T                configMask;         /* Config associated with this comp in txn */
    ClUint32T               compIndex;          /* Indexing this component in the list */
} ClTxnAppComponentT;
/**
 * Following is the uniqe-Id to identify the agent where the 
 * transaction-job is failed.
 */
typedef struct
{
    ClTxnTransactionJobIdT  jobId;
    ClIocPhysicalAddressT   agentAddress;     /* Physical address of component involved */
}ClTxnAgentIdT;

/**
 * Following is the information returned by the agent where 
 * the transaction-job is failed.
 */
typedef struct 
{
    /* deepak : to check whether failedPhase is required */
    ClUint8T                failedPhase;
    ClUint8T*               agentJobDefn;
    ClUint32T               agentJobDefnSize;
    ClTxnAgentIdT           failedAgentId;      
}ClTxnAgentT;

/**
 * Common transaction definition used by all three sub-modules
 */
typedef struct
{
    ClTxnTransactionIdT     clientTxnId;
    ClTxnTransactionIdT     serverTxnId;
    ClUint32T               jobCount;
    ClTxnTransactionStateT  currentState;
    ClCntHandleT            jobList;    /* List of jobs */
    ClUint16T               txnCfg;
} ClTxnDefnT;

/**
 * Cookie used by server to track the transaction and agent 
 */
typedef struct
{
    ClIocPhysicalAddressT  addr;
    ClTxnTransactionIdT    tid;
}ClTxnAppCookieT;
typedef ClTxnDefnT*     ClTxnDefnPtrT;

/**
 * Type of Message exchanged among transaction client, server and agent
 */
typedef enum
{
    CL_TXN_MSG_CLIENT_REQ,
    CL_TXN_MSG_MGR_CMD,
    CL_TXN_MSG_MGR_RESP,            /* This proto will bypass rmd. This should solve condwait problem!!! */
    CL_TXN_MSG_AGNT_RESP,
    CL_TXN_MSG_CLIENT_REQ_TO_AGNT, /* COR SPECIFIC */
    CL_TXN_MSG_AGNT_RESP_TO_CLIENT, /* COR SPECIFIC */
    CL_TXN_MSG_COMP_STATUS_UPDATE
} ClTxnMessageTypeT;

/**
 * Message header exchanged between transaction-client, server and agent.
 */
typedef struct
{
    ClVersionT              version; /* 3 bytes */
    ClTxnMessageTypeT       msgType; /* 4 bytes */
    ClIocPhysicalAddressT   srcAddr; /* 8 bytes */ 
    ClIocPhysicalAddressT   destAddr; /* 8 bytes */
    ClRcT                   srcStatus; /* 4 bytes */
    ClUint32T               msgCount; /* 4 bytes */
    ClBufferHandleT         payload; /* 4 or 8 bytes but actually points to a huge one */
    ClUint32T               funcId; /* 4 bytes */
    ClRmdAsyncCallbackT     fpCallback; /* 4 or 8 bytes */
    ClUint32T               timeOut; /* 4 bytes */
    ClUint32T               sessionCount; /* 4 bytes */
    ClTxnTransactionIdT     tid; /* 8 bytes */
} ClTxnMessageHeaderT; /* Total 59 or 67 bytes */

/**
 * Structure containing information exchanged between txn-mgmt and agent 
 */
typedef struct
{
    ClTxnTransactionIdT     txnId;
    ClTxnTransactionJobIdT  jobId;
    ClTxnMgmtCommandT       cmd;
    ClRcT                   resp;
} ClTxnCmdT;

/**
 * Txn Communication Interface Session Details.
 */
typedef struct
{
    ClCntHandleT    txnCommSessionList;
    ClVersionT      txnSupportedVersion;
    ClUint32T       srcPortNo;
    ClOsalMutexIdT  txnCommMutex;
} ClTxnCommIfcT;

typedef ClCntHandleT   ClTxnCommHandleT;

typedef struct {
    ClIocPhysicalAddressT   destAddr;
    ClTxnMessageTypeT       msgType;
    ClUint32T               funcId;
    ClUint32T               tId;
} ClTxnCommSessionKeyT;
#define clTxnMutexCreate(mutex) do{                                    \
    ClRcT _rc = CL_OK;                                               \
    _rc = clOsalMutexCreate(mutex);                              \
        if(_rc != CL_OK ){                                       \
            clLogError(NULL, NULL,                               \
                    "Creating mutex failed [0x%x]",             \
                     _rc);                                      \
            CL_ASSERT(_rc == CL_OK );                            \
        }                                                        \
}while(0)

#define clTxnMutexCreateAndLock(mutex) do{                             \
    ClRcT _rc = CL_OK;                                               \
    _rc = clOsalMutexCreateAndLock(mutex);                       \
        if(_rc != CL_OK ){                                       \
            clLogError(NULL, NULL,                               \
                    "Creating and locking mutex failed [0x%x]", \
                     _rc);                                      \
            CL_ASSERT(_rc == CL_OK );                            \
        }                                                        \
}while(0)                                                       

#define clTxnMutexLock( mutex ) do {                                 \
    ClRcT _rc = CL_OK;                                               \
    _rc = clOsalMutexLock(mutex);                                \
        if(_rc != CL_OK ){                                       \
            clLogError(NULL, NULL,                               \
                    "Locking mutex failed [0x%x]",              \
                     _rc);                                      \
            CL_ASSERT(_rc == CL_OK );                            \
        }                                                       \
}while(0)                                                       

#define clTxnMutexUnlock( mutex ) do {                               \
    ClRcT _rc = CL_OK;                                               \
        _rc = clOsalMutexUnlock(mutex);                              \
            if(_rc != CL_OK ){                                       \
                clLogError(NULL, NULL,                               \
                        "Unlocking mutex failed [0x%x]",			 \
                         _rc);                                      \
                CL_ASSERT(_rc == CL_OK );                            \
            }                                                        \
}while(0)                                                      

#define clTxnMutexDelete(mutex) do{                                    \
    ClRcT _rc = CL_OK;                                               \
    _rc = clOsalMutexDelete(mutex);                              \
        if(_rc != CL_OK ){                                       \
            clLogError(NULL, NULL,                               \
                    "Deleting mutex failed [0x%x]",             \
                     _rc);                                      \
            CL_ASSERT(_rc == CL_OK );                            \
        }                                                        \
}while(0)                                                       


/******************************************************************************
 *                          Function Declaration                              *
 ******************************************************************************/
/**
 * Compare given two Transaction-Id
 */
extern ClInt32T  clTxnUtilTxnIdCompare(
        CL_IN   ClTxnTransactionIdT     txnId_1, 
        CL_IN   ClTxnTransactionIdT     txnId_2);

/**
 * Compare given two Transaction Job-Id
 */
extern ClInt32T  clTxnUtilJobIdCompare(
        CL_IN   ClTxnTransactionJobIdT  jobId_1,
        CL_IN   ClTxnTransactionJobIdT  jobId_2);

/* Communication related APIs */
extern ClRcT clTxnCommIfcInit(
        CL_IN   ClVersionT  *pSupportedVersion);

extern ClRcT clTxnCommIfcFini();

extern ClRcT clTxnCommIfcNewSessionCreate(
        CL_IN   ClTxnMessageTypeT       msgType, 
        CL_IN   ClIocPhysicalAddressT   destAddr,
        CL_IN   ClUint32T               funcId,
        CL_IN   ClRmdAsyncCallbackT     fpCallback,
        CL_IN   ClUint32T               timeOut,
        CL_IN   ClUint32T               tId,
        CL_OUT  ClTxnCommHandleT        *pHandle);

extern ClRcT clTxnCommIfcSessionAppendTxnCmd(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClTxnCmdT               *pTxnCmd);

extern ClRcT clTxnCommIfcSessionAppendPayload(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClTxnCmdT               *pTxnCmd,
        CL_IN   ClUint8T                *pMsg,
        CL_IN   ClUint32T               len);

extern ClRcT clTxnCommIfcSessionCancel(
        CL_IN   ClTxnCommHandleT        commHandle);

extern ClRcT clTxnCommIfcSessionClose(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClUint8T                syncFlag);

extern ClRcT clTxnCommIfcSessionCloseSync(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClUint8T                syncFlag,
        CL_OUT  ClBufferHandleT         *pOutMsg);

extern ClRcT clTxnCommIfcReadMessage(
        CL_IN   ClTxnCommHandleT        commHandle,
        CL_IN   ClBufferHandleT         outMsg);


extern ClRcT clTxnCommIfcSessionRelease(
        CL_IN   ClTxnCommHandleT    commHandle);

extern ClRcT clTxnCommIfcAllSessionClose();

extern ClRcT clTxnCommIfcAllSessionCancel();

extern void clTxnCommCookieFree(
        CL_IN ClPtrT cookie);

extern ClRcT clTxnClientProcessMsg(
        CL_IN ClBufferHandleT inMsg);

extern ClRcT  clTxnCommIfcDeferSend(
        CL_IN ClTxnCommHandleT commHandle,
        CL_IN ClRmdResponseContextHandleT rmdDeferHdl,
        CL_IN ClBufferHandleT             outMsg);

#ifdef __cplusplus
}
#endif

#endif
