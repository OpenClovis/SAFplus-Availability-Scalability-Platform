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
 * ModuleName  : message
 * File        : clMsgIdl.c
 *******************************************************************************/
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clMsgDebugInternal.h>
#include <clMsgCommon.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clMsgIdl.h>
#include <clMsgCkptData.h>
#include <clMsgIocClient.h>

#include <msgCltSrvClientCallsFromClientToClientServerClient.h>
#include <msgCltSrvClientCallsFromClientToClientServerServer.h>
#include <msgCltClientCallsFromServerToClientClient.h>
#include <msgIdlClientCallsFromServerClient.h>
#include <msgIdlClientCallsFromClientClient.h>

ClIdlHandleObjT gIdlUcastObj;
ClIdlHandleObjT gIdlBcastObj;
ClIdlHandleT gIdlUcastHandle;
ClIdlHandleT gIdlBcastHandle;

ClRcT clMsgCommIdlInitialize(void)
{
    ClRcT rc;
    ClRcT retCode;

    gIdlUcastObj.objId = clObjId_ClIdlHandleObjT; 
    gIdlUcastObj.address.addressType = CL_IDL_ADDRESSTYPE_IOC;
    gIdlUcastObj.address.address.iocAddress.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    gIdlUcastObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;
    gIdlUcastObj.flags = 0; 
    gIdlUcastObj.options.timeout = CL_RMD_TIMEOUT_FOREVER;
    gIdlUcastObj.options.retries = 3; 
    gIdlUcastObj.options.priority = CL_IOC_DEFAULT_PRIORITY;

    rc = clIdlHandleInitialize(&gIdlUcastObj, &gIdlUcastHandle);
    if(rc != CL_OK)
    {    
        clLogError("MSG", "IDL", "Failed to initialize the the IDL. error code [0x%x].", rc); 
        goto error_out;
    }    

    memcpy(&gIdlBcastObj, &gIdlUcastObj, sizeof(gIdlBcastObj));
    gIdlBcastObj.address.address.iocAddress.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    gIdlBcastObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;
    gIdlBcastObj.options.priority = CL_IOC_ORDERED_PRIORITY;
    rc = clIdlHandleInitialize(&gIdlBcastObj, &gIdlBcastHandle);
    if(rc != CL_OK)
    {    
        clLogError("MSG", "IDL", "Failed to initialize the the IDL. error code [0x%x].", rc); 
        goto error_out_1;
    } 

    goto done_out;

error_out_1:
    retCode = clIdlHandleFinalize(gIdlUcastHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "IDL", "Failed to do the finalize the IDL handle. error code [0x%x].", retCode);
error_out:
done_out:
    return rc;
}


void clMsgCommIdlFinalize(void)
{
    ClRcT rc;

    rc = clIdlHandleFinalize(gIdlUcastHandle);
    if(rc != CL_OK)
        clLogError("MSG", "IDL", "Failed to do the finalize the IDL handle. error code [0x%x].", rc);

    rc = clIdlHandleFinalize(gIdlBcastHandle);
    if(rc != CL_OK)
        clLogError("MSG", "IDL", "Failed to do the finalize the IDL handle. error code [0x%x].", rc);

}

ClRcT clMsgSendMessage_idl(ClMsgMessageSendTypeT sendType,
        ClIocPhysicalAddressT compAddr,
        ClNameT *pName,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout,
        ClBoolT isSync,
        SaMsgAckFlagsT ackFlag,
        MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
        void *cookie)
 
{
    ClRcT rc = CL_OK;
    ClIdlHandleT idlHandle = 0;
    ClIdlHandleObjT idlObj = {0};

    if(compAddr.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
    {
        rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(gIdlBcastHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
        if(rc != CL_OK)
            clLogError("IDL", "BCAST", "Failed to broadcast a message. error code [0x%x].", rc);
        return rc;
    }

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = compAddr;

    if (pMessage->priority == SA_MSG_MESSAGE_HIGHEST_PRIORITY)
    {
        idlObj.options.priority = CL_IOC_HIGH_PRIORITY;
    }

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "SND", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "SND", "Sending a message to component [0x%x,0x%x].", compAddr.nodeAddress, compAddr.portId);

    if (isSync == CL_TRUE)
    {
        rc = VDECL_VER(clMsgMessageReceivedClientSync, 4, 0, 0)(idlHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout);
        if(rc != CL_OK)
            clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
    }
    else
    {
        if(ackFlag == SA_MSG_MESSAGE_DELIVERED_ACK)
        {
            rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(idlHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, fpAsyncCallback, cookie);
            if(rc != CL_OK)
                clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
        }
        else
        {
            rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(idlHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
            if(rc != CL_OK)
                clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
        }
    }

    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}

typedef struct
{
    ClMsgMessageSendTypeT sendType;
    ClNameT *pName;
    SaMsgMessageT *pMessage;
    SaTimeT sendTime;
    ClHandleT senderHandle;
    SaTimeT timeout;
    void *cookie;
    MsgCltSrvClMsgMessageReceivedAsyncCallbackT func;
} ClMsgIocSendAsyncRecordT;

static void clMsgIocSendAsyncCallbackFunc(ClRcT rc, 
                             void *pCookie)
{
    ClMsgIocSendAsyncRecordT *pRecord = (ClMsgIocSendAsyncRecordT *) pCookie;

    ((MsgCltSrvClMsgMessageReceivedAsyncCallbackT)(pRecord->func))(
                          0,
                          pRecord->sendType,
                          pRecord->pName,
                          pRecord->pMessage,
                          pRecord->sendTime,
                          pRecord->senderHandle,
                          pRecord->timeout,
                          rc, pRecord->cookie);
    clHeapFree(pCookie);
    return;
}

ClRcT clMsgSendMessage_IocSend(ClMsgMessageSendTypeT sendType,
        ClIocPhysicalAddressT compAddr,
        ClNameT *pName,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout,
        ClBoolT isSync,
        SaMsgAckFlagsT ackFlag,
        MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
        void *cookie)
{
    ClRcT rc = CL_OK;

    if(compAddr.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
    {
        rc = clMsgIocSendAsync((ClIocAddressT *)&compAddr, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
        if(rc != CL_OK)
            clLogError("IDL", "BCAST", "Failed to broadcast a message. error code [0x%x].", rc);
        return rc;
    }

    clLogTrace("IDL", "SND", "Sending a message to component [0x%x,0x%x].", compAddr.nodeAddress, compAddr.portId);

    if (isSync == CL_TRUE)
    {
        rc = clMsgIocSendSync((ClIocAddressT *)&compAddr, sendType, pName, pMessage, sendTime, senderHandle, timeout);
        if(rc != CL_OK)
            clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
    }
    else
    {
        if(ackFlag == SA_MSG_MESSAGE_DELIVERED_ACK)
        {
            ClMsgIocSendAsyncRecordT *pCookie = (ClMsgIocSendAsyncRecordT *) clHeapAllocate(sizeof(ClMsgIocSendAsyncRecordT));
            if(pCookie == NULL)
            {
                rc = CL_ERR_NO_MEMORY;
                goto error_out;
            }
            
            pCookie->func = (MsgCltSrvClMsgMessageReceivedAsyncCallbackT) fpAsyncCallback;
            pCookie->cookie = cookie;
            pCookie->sendType = sendType;
            pCookie->pName = pName;
            pCookie->pMessage = pMessage;
            pCookie->sendTime = sendTime;
            pCookie->senderHandle = senderHandle;
            pCookie->timeout = timeout;

            rc = clMsgIocSendAsync((ClIocAddressT *)&compAddr, sendType, pName, pMessage, sendTime, senderHandle, timeout, &clMsgIocSendAsyncCallbackFunc, pCookie);
            if(rc != CL_OK)
                clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
        }
        else
        {
            rc = clMsgIocSendAsync((ClIocAddressT *)&compAddr, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
            if(rc != CL_OK)
                clLogError("IDL", "SND", "Failed to send a message to component [0x%x,0x%x]. error code [0x%x].", compAddr.nodeAddress, compAddr.portId, rc);
        }
    }

error_out:
    return rc;
}

ClBoolT clMsgQCkptExists(const ClNameT *pQName, ClMsgQueueCkptDataT *pQueueData);
ClRcT clMsgQueueUnlinkToServer(ClNameT *pQName)
{
    ClRcT rc;
    /* Look up msg queue in the cached checkpoint */
    ClMsgQueueCkptDataT queueData;
    if(clMsgQCkptExists((ClNameT *)pQName, &queueData) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("MSG", "UNL", "Failed to get the message queue information.");
        goto error_out;
    }

    /* Get Ioc address of the given MSG queue */
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    if ((queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT) &&
            (queueData.qServerAddress.nodeAddress != 0))
    {
        idlObj.address.address.iocAddress.iocPhyAddress = queueData.qServerAddress;
    }

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "UNL", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = VDECL_VER(clMsgQueueUnlinkClientSync, 4, 0, 0)(idlHandle, (ClNameT *)pQName);
    if(rc != CL_OK)
    {
        clLogError("MSG", "UNL", "Failed to close the message queue at message server. error code [0x%x].", rc);
    }

    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}

ClRcT clMsgCallClientsTrackCallback(ClIocPhysicalAddressT compAddr,
        ClNameT *pGroupName,
        SaMsgHandleT appHandle,
        SaMsgQueueGroupNotificationBufferT *pData)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;
    
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = compAddr;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "TRCb", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "TRCb", "Calling track callback for client [0x%x:0x%x].", compAddr.nodeAddress, compAddr.portId);

    rc = VDECL_VER(clMsgClientsTrackCallbackClientAsync, 4, 0, 0)(idlHandle, appHandle, pGroupName, pData, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "TRCb", "Failed to make an Async RMD to client. error code [0x%x].", rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}

ClRcT clMsgQueueAllocateThroughIdl(
        ClIocPhysicalAddressT destNode,
        ClNameT *pQName,
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttrs,
        SaMsgQueueHandleT *pQHandle)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;
    
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = destNode;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "ALOC", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "ALOC", "Allocate-request for queue [%.*s] on node [%d].", pQName->length, pQName->value, destNode.nodeAddress);

    rc = VDECL_VER(clMsgQueueAllocateClientSync, 4, 0, 0)(idlHandle, pQName, openFlags, pCreationAttrs, pQHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "ALOC", "Queue [%.*s] allocation failed on node [%d]. error code [0x%x].", pQName->length, pQName->value, destNode.nodeAddress, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}

ClRcT clMsgMessageGet_Idl(
        ClIocPhysicalAddressT destNode,
        ClNameT *pQName,
        SaTimeT timeout)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;
    
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = destNode;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "GET", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "GET", "Get message request for queue [%.*s] on node [%d].", pQName->length, pQName->value, destNode.nodeAddress);

    rc = VDECL_VER(clMsgMessageGetClientSync, 4, 0, 0)(idlHandle, pQName, timeout);
    if(rc != CL_OK)
    {
        clLogError("IDL", "GET", "Queue [%.*s] message get failed on node [%d]. error code [0x%x].", pQName->length, pQName->value, destNode.nodeAddress, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}
