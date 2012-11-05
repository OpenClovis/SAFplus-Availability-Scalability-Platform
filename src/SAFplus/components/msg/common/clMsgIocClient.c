#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clDebugApi.h>
#include <clXdrApi.h>
#include <clMsgIocClient.h>

#include <xdrSaMsgMessageT.h>

/* 
 * Internal definitions 
 */
#define CL_MSG_IOC_CALL_SYNC 0
#define CL_MSG_IOC_CALL_ASYNC 1
#define CL_MSG_IOC_TIMEOUT 100000

#define NUMBER_OF_MSG_IOC_BUCKETS 256
#define MSG_IOC_BUCKETS_MASK (NUMBER_OF_MSG_IOC_BUCKETS - 1)

typedef struct
{
    ClUint64T msgId;
    ClOsalMutexIdT msgSendLock;
    ClCntHandleT msgSendCntHandle;
}ClMsgIocSvcT;

typedef struct
{
    ClTimerHandleT timerID;
    void *cookie;
    MsgIocMessageSendAsyncCallbackT func;
} ClMsgAsyncRecordT;

typedef struct
{
    ClOsalCondT     syncCond;
    ClRcT retVal;
} ClMsgSyncRecordT;

typedef struct
{
    ClUint8T flag;        /* synchronous or asynchronous */
    union
    {
        ClMsgSyncRecordT syncRec;
        ClMsgAsyncRecordT asyncRec;
    } recType;

} ClMsgIocRecordT;

/* 
 * Static function prototypes 
 */
static ClRcT clMsgSendDataMarshall(ClBufferHandleT message,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout,
                     ClUint64T msgId,
                     ClUint8T syncType
                     );
static ClRcT msgIocAsyncTimerFunc(void *pData);
static ClUint32T msgIocHashFunction(ClCntKeyHandleT key);
static ClRcT clMsgIocReplyHandle(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr);
static void clMsgIocReplyProtoInstall(void);
static ClUint32T msgIocHashFunction(ClCntKeyHandleT key);
static ClInt32T msgIocKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void msgIocHashDeleteCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData);
static void msgIocHashDestroyCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData);
static void clMsgIocSendAsyncReplyHandle(ClRcT rc, 
                                 MsgIocMessageSendAsyncCallbackT fpAsyncCallback, 
                                 void *pCookie);

/* 
 * Global variables
 */
ClMsgIocSvcT gMsgSendSvr;

/* 
 * API implementation
 */
ClRcT clMsgIocCltInitialize()
{
    ClRcT rc = CL_OK;

    /* Initialize msgId */
    gMsgSendSvr.msgId = 0;

    /* Initialize mutex for MSG IOC send */
    rc = clOsalMutexCreate(&gMsgSendSvr.msgSendLock);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clDbgCodeError(rc, ("Cannot initialize mutex for MSG IOC send."));
        goto out1;
    }

    /* Initialize hash table to store send record*/
    rc = clCntHashtblCreate(NUMBER_OF_MSG_IOC_BUCKETS, msgIocKeyCompare,
                           msgIocHashFunction, msgIocHashDeleteCallBack,
                           msgIocHashDestroyCallBack, CL_CNT_UNIQUE_KEY,
                           &gMsgSendSvr.msgSendCntHandle);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clDbgCodeError(rc, ("Cannot initialize hash table to store MSG IOC send record."));
        goto out2;
    }

    clMsgIocReplyProtoInstall();

    return rc;
out2:
    clOsalMutexDelete(gMsgSendSvr.msgSendLock);
out1:    
    return rc;
}

ClRcT clMsgIocCltFinalize()
{
    ClRcT rc;

    rc = clOsalMutexDelete(gMsgSendSvr.msgSendLock);
    if(rc != CL_OK)
        clLogError("MSG", "IOC_FIN", "clOsalMutexDelete(): error code [0x%x].", rc);

    rc = clCntDelete(gMsgSendSvr.msgSendCntHandle);
    if(rc != CL_OK)
        clLogError("MSG", "IOC_FIN", "clCntDelete(): error code [0x%x].", rc);

    return rc;
}

/* This function is used to send messages synchronously to server */
ClRcT clMsgIocSendSync(ClIocAddressT * pDestAddr,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout
                    )
{
    ClRcT rc = CL_OK;
    ClRcT rec;
    ClUint8T protoType = CL_IOC_SAF_MSG_REQUEST_PROTO;
    ClIocSendOptionT sendOption = { 0 };
    ClBufferHandleT inMsg = NULL;
    ClEoExecutionObjT *pThis = NULL;
    ClMsgIocRecordT sendRecord = {0};
    ClUint64T msgId;
    ClUint8T syncType = CL_MSG_IOC_CALL_SYNC;
    ClInt32T msgIocTimeout = CL_MSG_IOC_TIMEOUT;
    ClTimerTimeOutT condTsTimeout;

    condTsTimeout.tsSec = msgIocTimeout / 1000;
    condTsTimeout.tsMilliSec = msgIocTimeout % 1000;

    // Get EO object
    rc = clEoMyEoObjectGet(&pThis);
    if (rc != CL_OK)
    {
        clDbgCodeError(rc, ("Cannot get EO object."));
        goto out1;
    }

    /* Set Ioc send option */
    sendOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sendOption.priority = CL_IOC_DEFAULT_PRIORITY;
    sendOption.timeout = msgIocTimeout;

    if (pMessage->priority == SA_MSG_MESSAGE_HIGHEST_PRIORITY)
    {
        sendOption.priority = CL_IOC_HIGH_PRIORITY;
    }

    /* Create buffer for input message */
    rc = clBufferCreate(&inMsg);
    if (CL_OK != rc)
    {
        clDbgResourceLimitExceeded(clDbgMemoryResource, 0, ("Out of memory"));
        goto out1;
    }

    rec = clOsalMutexLock(gMsgSendSvr.msgSendLock);
    CL_ASSERT(rec == CL_OK);

    /* Get msgId */
    msgId = gMsgSendSvr.msgId++;

    /* Marshall send data */
    rc = clMsgSendDataMarshall(inMsg, sendType, pDestination, pMessage, sendTime, senderHandle, timeout, msgId, syncType);
    if (CL_OK != rc)
    {
        clDbgCodeError(rc, ("Cannot marshal send data."));
        goto out2;
    }

    /* Create a send record */
    sendRecord.flag = sendType;
    sendRecord.recType.syncRec.retVal = CL_OK;
    rec = clOsalCondInit(&sendRecord.recType.syncRec.syncCond);
    CL_ASSERT(rec == CL_OK);

    /* Add send record into database */
    rc = clCntNodeAdd(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId,
                     (ClCntDataHandleT) &sendRecord, NULL);    
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_SYNC", "clCntNodeAdd(): error code [0x%x].", rc);
        goto out2;
    }

    /* Call clIocSend to send message to the destination */
    rc = clIocSend(pThis->commObj, inMsg, protoType, pDestAddr, &sendOption);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_SYNC", "clIocSend(): error code [0x%x].", rc);
        goto out3;
    }

    /* Wait until remote function finish running or timeout*/
    rc = clOsalCondWait(&sendRecord.recType.syncRec.syncCond,
                           gMsgSendSvr.msgSendLock, condTsTimeout);
    if (rc == CL_OK)
    {
        rc = sendRecord.recType.syncRec.retVal;
    }

out3:
    clCntAllNodesForKeyDelete(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId);
out2:
    clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
    clBufferDelete(&inMsg);
out1:
    return rc;
}

/* This function is used to send messages asynchronously to server */
ClRcT clMsgIocSendAsync(ClIocAddressT * pDestAddr,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout,
                     MsgIocMessageSendAsyncCallbackT fpAsyncCallback,
                     void *cookie
                    )
{
    ClRcT rc = CL_OK;
    ClRcT rec;
    ClUint8T protoType = CL_IOC_SAF_MSG_REQUEST_PROTO;
    ClIocSendOptionT sendOption = { 0 };
    ClBufferHandleT inMsg = NULL;
    ClEoExecutionObjT *pThis = NULL;
    ClMsgIocRecordT *pSendRecord = NULL;
    ClUint64T msgId;    
    ClUint8T syncType = CL_MSG_IOC_CALL_ASYNC;
    ClInt32T msgIocTimeout = CL_MSG_IOC_TIMEOUT;
    ClTimerTimeOutT condTsTimeout;

    condTsTimeout.tsSec = msgIocTimeout / 1000;
    condTsTimeout.tsMilliSec = msgIocTimeout % 1000;

    // Get EO object
    rc = clEoMyEoObjectGet(&pThis);
    if (rc != CL_OK)
    {
        clDbgCodeError(rc, ("Cannot get EO object."));
        goto out1;
    }

    /* Set Ioc send option */
    sendOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sendOption.priority = CL_IOC_DEFAULT_PRIORITY;
    sendOption.timeout = timeout;

    if (pMessage->priority == SA_MSG_MESSAGE_HIGHEST_PRIORITY)
    {
        sendOption.priority = CL_IOC_HIGH_PRIORITY;
    }

    /* Create buffer for input message */
    rc = clBufferCreate(&inMsg);
    if (CL_OK != rc)
    {
        clDbgResourceLimitExceeded(clDbgMemoryResource, 0, ("Out of memory"));
        goto out1;
    }

    rec = clOsalMutexLock(gMsgSendSvr.msgSendLock);
    CL_ASSERT(rec == CL_OK);

    /* Get msgId */
    msgId = gMsgSendSvr.msgId++;

    /* Marshall send data */
    rc = clMsgSendDataMarshall(inMsg, sendType, pDestination, pMessage, sendTime, senderHandle, timeout, msgId, syncType);
    if (CL_OK != rc)
    {
        clDbgCodeError(rc, ("Cannot marshal send data."));
        goto out2;
    }

    /* Create a send record */
    pSendRecord = clHeapAllocate((ClUint32T) sizeof(ClMsgIocRecordT));
    if(pSendRecord == NULL)
    {
        rc = CL_ERR_NO_MEMORY;
        clDbgResourceLimitExceeded(clDbgMemoryResource, 0, ("Out of memory"));
        goto out2;
    }

    pSendRecord->flag = sendType;
    pSendRecord->recType.asyncRec.func = fpAsyncCallback;
    pSendRecord->recType.asyncRec.cookie = cookie;

    /* Create timer to handle async call timeout */
    rc = clTimerCreate(condTsTimeout, CL_TIMER_REPETITIVE,
                      CL_TIMER_SEPARATE_CONTEXT, msgIocAsyncTimerFunc,
                      (ClPtrT)(ClWordT)msgId,
                      &pSendRecord->recType.asyncRec.timerID);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_ASYNC", "clTimerCreate(): error code [0x%x].", rc);
        goto out3;
    }

    /* Add send record into database */
    rc = clCntNodeAdd(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId,
                     (ClCntDataHandleT) pSendRecord, NULL);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_ASYNC", "clCntNodeAdd(): error code [0x%x].", rc);
        goto out4;
    }

    /* Call clIocSend to send message to the destination */
    rc = clIocSend(pThis->commObj, inMsg, protoType, pDestAddr, &sendOption);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_ASYNC", "clIocSend(): error code [0x%x].", rc);
        goto out5;
    }

    /* start timer for async call timeout event */
    rc = clTimerStart(pSendRecord->recType.asyncRec.timerID);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "SEND_ASYNC", "clTimerStart(): error code [0x%x].", rc);
        goto out5;
    }

    clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
    clBufferDelete(&inMsg);
    return rc;

out5:
    clCntAllNodesForKeyDelete(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId);
out4:
    clTimerDeleteAsync(&pSendRecord->recType.asyncRec.timerID);
out3:
    clHeapFree(pSendRecord);
out2:
    clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
    clBufferDelete(&inMsg);
out1:
    return rc;
}

/* 
 * static function implementation
 */

/* Function to marshal MSG IOC send data */
static ClRcT clMsgSendDataMarshall(ClBufferHandleT message,
                     ClMsgMessageSendTypeT sendType,
                     const ClNameT *pDestination,
                     const SaMsgMessageT *pMessage,
                     ClInt64T sendTime,
                     ClHandleT senderHandle,
                     ClInt64T timeout,
                     ClUint64T msgId,
                     ClUint8T syncType
                     )
{
    ClRcT rc = CL_OK;

    rc = clXdrMarshallClUint32T(&(sendType), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClNameT((void *)pDestination, message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallSaMsgMessageT_4_0_0((void *)pMessage, message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClInt64T(&(sendTime), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClHandleT(&(senderHandle), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClInt64T(&(timeout), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClUint64T(&(msgId), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clXdrMarshallClUint8T(&(syncType), message, 0);
    if (CL_OK != rc)
    {
        return rc;
    }

    return rc;
}

/* Function to handle timeout for asynchronous call */
static ClRcT msgIocAsyncTimerFunc(void *pData)
{
    ClCntNodeHandleT                nodeHandle;
    ClMsgIocRecordT                 *record = NULL;
    ClRcT                           rc = CL_OK;
    MsgIocMessageSendAsyncCallbackT fpTempPtr;
    void                            *cookie;
    ClUint64T                       msgId = (ClUint32T)(ClWordT) pData;

    if (pData == NULL)
    {
        rc = CL_ERR_INVALID_BUFFER;
        clDbgCodeError(rc, ("Error invalid buffer."));
        return rc;
    }

    rc = clOsalMutexLock(gMsgSendSvr.msgSendLock);
    CL_ASSERT(rc == CL_OK);

    rc = clCntNodeFind(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId,
                       &nodeHandle);
    if (rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(gMsgSendSvr.msgSendCntHandle, nodeHandle,
                                  (ClCntDataHandleT *) &record);
        if ((rc == CL_OK) && (record))
        {
            /* delete timer */
            clTimerDeleteAsync(&record->recType.asyncRec.timerID);
            
            if (record->recType.asyncRec.func)
            {
                fpTempPtr = record->recType.asyncRec.func;
                cookie = record->recType.asyncRec.cookie;

                rc = clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
                CL_ASSERT(rc == CL_OK);
                /* call callback function with CL_ERR_TIMEOUT*/
                clMsgIocSendAsyncReplyHandle(CL_ERR_TIMEOUT, fpTempPtr, cookie);
                rc = clOsalMutexLock(gMsgSendSvr.msgSendLock);
                CL_ASSERT(rc == CL_OK);
            }
            
            /* delete MSG IOC send record */
            clCntNodeDelete(gMsgSendSvr.msgSendCntHandle,
                                     nodeHandle);
        }
    }

    rc = clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
    CL_ASSERT(rc == CL_OK);
    return CL_OK;

}

static void clMsgIocSendAsyncReplyHandle(ClRcT rc, 
                                 MsgIocMessageSendAsyncCallbackT fpAsyncCallback, 
                                 void *pCookie)
{
    ((MsgIocMessageSendAsyncCallbackT)(fpAsyncCallback))(rc, pCookie);
    clHeapFree(pCookie);
    return;
}

static ClRcT clMsgIocReplyHandle(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr)
{
    ClRcT                  rc = CL_OK;
    ClRcT                  rec;
    ClInt64T               msgId;
    ClUint8T               syncType;
    ClCntNodeHandleT       nodeHandle;
    ClMsgIocRecordT        *record = NULL;

    /* Get return value */
    rc = clXdrUnmarshallClUint32T( eoRecvMsg,&(rec));
    if (CL_OK != rc)
    {
        goto out1;
    }

    /* Get msgId */
    rc = clXdrUnmarshallClInt64T( eoRecvMsg,&(msgId));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallClUint8T( eoRecvMsg,&(syncType));
    if (CL_OK != rc)
    {
        goto out1;
    }

    if (syncType == CL_MSG_IOC_CALL_ASYNC)
    {
        rc = clOsalMutexLock(gMsgSendSvr.msgSendLock);
        CL_ASSERT(rc == CL_OK);
    }

    rc = clCntNodeFind(gMsgSendSvr.msgSendCntHandle, (ClPtrT)(ClWordT)msgId,
                       &nodeHandle);
    if (rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(gMsgSendSvr.msgSendCntHandle, nodeHandle,
                                  (ClCntDataHandleT *) &record);
        if ((rc == CL_OK) && (record))
        {
            /* Async case */
            if (record->flag == CL_MSG_IOC_CALL_ASYNC)
            {
                clTimerDeleteAsync(&record->recType.asyncRec.timerID);

                if (record->recType.asyncRec.func)
                {
                    MsgIocMessageSendAsyncCallbackT fpTempPtr;
                    void                            *cookie;
                    
                    fpTempPtr = record->recType.asyncRec.func;
                    cookie = record->recType.asyncRec.cookie;

                    rc = clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
                    CL_ASSERT(rec == CL_OK);
                    /* call callback function */
                    clMsgIocSendAsyncReplyHandle(rec, fpTempPtr, cookie);
                    rc = clOsalMutexLock(gMsgSendSvr.msgSendLock);
                    CL_ASSERT(rec == CL_OK);
                }
                
                /* delete MSG IOC send record */
                clCntNodeDelete(gMsgSendSvr.msgSendCntHandle,
                                         nodeHandle);
            }
            else /* Synch case */
            {
                /* Set return value */
                record->recType.syncRec.retVal = rec;
                
                /* Wake the caller*/
                rc = clOsalCondSignal(&record->recType.syncRec.syncCond);
                CL_ASSERT(rc == CL_OK); 
            }
        }
    }

    if (syncType == CL_MSG_IOC_CALL_ASYNC)
    {
        rc = clOsalMutexUnlock(gMsgSendSvr.msgSendLock);
        CL_ASSERT(rc == CL_OK); 
    }

out1:
    return rc;
}

static void clMsgIocReplyProtoInstall(void)
{
    ClEoProtoDefT eoProtoDef = 
    {
        CL_IOC_SAF_MSG_REPLY_PROTO,
        "Msg reply from MSG server",
        clMsgIocReplyHandle,
        NULL,
        CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE
    };

    clEoProtoSwitch(&eoProtoDef);
} 
/*
 * Hash table to store MSG IOC record
 */

static ClUint32T msgIocHashFunction(ClCntKeyHandleT key)
{
    return (  ((ClWordT)key) & MSG_IOC_BUCKETS_MASK);
}

static ClInt32T msgIocKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

static void msgIocHashDeleteCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData)
{
    ClMsgIocRecordT *rec = (ClMsgIocRecordT *) userData;
    if (userData)
    {
        if (rec->flag == CL_MSG_IOC_CALL_ASYNC)
        {
            if (rec->recType.asyncRec.timerID)
            {
                clTimerDeleteAsync(&rec->recType.asyncRec.timerID);
            }

            clHeapFree(rec);
        }
        else
        {
            /* Before destroying the condition variable, make sure it is not being used in clOsalCondWait(). */
            clOsalCondSignal(&rec->recType.syncRec.syncCond);

            clOsalCondDestroy(&rec->recType.syncRec.syncCond);
        }
    }
}

static void msgIocHashDestroyCallBack(ClCntKeyHandleT userKey,
                                   ClCntDataHandleT userData)
{
    msgIocHashDeleteCallBack(userKey,userData);
}


