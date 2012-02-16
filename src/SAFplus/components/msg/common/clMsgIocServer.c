#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clDebugApi.h>
#include <clXdrApi.h>
#include <clMsgIocServer.h>
#include <xdrSaMsgMessageT.h>

#include <msgCltSrvClientCallsFromClientToClientServerServer.h>

static ClRcT clMsgReplyDataMarshall(ClBufferHandleT message,
                     ClRcT ret,
                     ClUint64T msgId,
                     ClUint8T syncType
                     )
{
    ClRcT rc = CL_OK;
    
    rc = clXdrMarshallClUint32T(&(ret), message, 0);
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

/* This function handles msg send request from clients */
static ClRcT clMsgIocRequestHandle(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr)
{
    ClRcT          rc = CL_OK;
    ClRcT          ret;
    ClUint32T      sendType;
    ClNameT        pDestination;
    SaMsgMessageT  pMessage;
    ClInt64T       sendTime;
    ClHandleT      senderHandle;
    ClInt64T       timeout;
    ClInt64T       msgId;
    ClUint8T       syncType;

    memset(&(pDestination), 0, sizeof(ClNameT));
    memset(&(pMessage), 0, sizeof(SaMsgMessageT));
    memset(&(senderHandle), 0, sizeof(ClHandleT));

    rc = clXdrUnmarshallClUint32T( eoRecvMsg,&(sendType));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallClNameT( eoRecvMsg,&(pDestination));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallSaMsgMessageT_4_0_0( eoRecvMsg,&(pMessage));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallClInt64T( eoRecvMsg,&(sendTime));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallClHandleT( eoRecvMsg,&(senderHandle));
    if (CL_OK != rc)
    {
        goto out1;
    }

    rc = clXdrUnmarshallClInt64T( eoRecvMsg,&(timeout));
    if (CL_OK != rc)
    {
        goto out1;
    }

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

    /* Call remote function */
    ret = VDECL_VER(clMsgMessageReceived, 4, 0, 0)(sendType, &(pDestination), &(pMessage), sendTime, senderHandle, timeout);

    /* Prepare to send return value to the caller */
    ClIocSendOptionT sendOption = { 0 };
    ClBufferHandleT replyMsg = NULL;    
    ClUint8T replyProto = CL_IOC_SAF_MSG_REPLY_PROTO;

    /* Set Ioc send option */
    sendOption.msgOption = CL_IOC_PERSISTENT_MSG;
    sendOption.priority = CL_IOC_DEFAULT_PRIORITY;
    sendOption.timeout = 10000;

    /* Create buffer for input message */
    rc = clBufferCreate(&replyMsg);
    if (CL_OK != rc)
    {
        clDbgResourceLimitExceeded(clDbgMemoryResource, 0, ("Out of memory"));
        goto out1;
    }

    /* Marshall reply data */
    rc = clMsgReplyDataMarshall(replyMsg, ret, msgId, syncType);
    if (CL_OK != rc)
    {
        clDbgCodeError(rc, ("Cannot marshal reply data."));
        goto out2;
    }

    /* Send return value to the caller */
    rc = clIocSend(pThis->commObj, replyMsg, replyProto,(ClIocAddressT *) &srcAddr, &sendOption);
    if (CL_OK != CL_GET_ERROR_CODE(rc))
    {
        clLogError("MSG", "REPLY", "clIocSend(): error code [0x%x].", rc);
    }

out2:
    clBufferDelete(&replyMsg);
out1:
    return rc;
}

/* This function is used to register Ioc MSG Send protocol to the EO */
static void clMsgIocRequestProtoInstall(void)
{
    ClEoProtoDefT eoProtoDef = 
    {
        CL_IOC_SAF_MSG_REQUEST_PROTO,
        "Msg send to MSG server",
        clMsgIocRequestHandle,
        NULL,
        CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE
    };

    clEoProtoSwitch(&eoProtoDef);
} 

void clMsgIocSvrInitialize()
{
    clMsgIocRequestProtoInstall();
}
