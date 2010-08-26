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
 * ModuleName  : message
 * File        : clMsgMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clRmdIpi.h>
#include <clIdlApi.h>
#include <clLogApi.h>
#include <clMsgCommon.h>
#include <clMsgDatabase.h>
#include <clMsgGroupDatabase.h>
#include <clMsgSender.h>
#include <clMsgEo.h>
#include <clCpmApi.h>
#include <clMsgDebugInternal.h>

#include <msgIdlServer.h>
#include <msgIdlClientCallsFromServerServer.h>
#include <msgIdlClientCallsFromServerClient.h>
#include <msgCltClientCallsFromServerToClientClient.h>

static ClOsalMutexT idlMutex;
static ClIdlHandleObjT gIdlUcastObj;
static ClIdlHandleObjT gIdlBcastObj;
static ClIdlHandleT gIdlUcastHandle;
static ClIdlHandleT gIdlBcastHandle;


ClRcT clMsgQUpdateSendThroughIdl(ClMsgSyncActionT syncAct, ClNameT *pQName, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr)
{
    ClRcT rc;

    clLogTrace("IDL", "QUPD", "Sending queue syncup data. action [%d], queue [%.*s].",
                syncAct, pQName->length, pQName->value);

    rc = VDECL_VER(clMsgQDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncAct, pQName, pCompAddress, pNewOwnerAddr, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "QUPD", "Failed to broadcast queue syncup data. action [%d], queue [%.*s]. error code [0x%x].", 
                syncAct, pQName->length, pQName->value, rc);
    }

    return rc;
}

ClRcT msgQUpdateSend_unicastSync_idl(ClIocNodeAddressT node, ClMsgSyncActionT syncAct, ClNameT *pQName, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = node;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;
    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "QUPD", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }


    clLogTrace("IDL", "QUPD", "Sending queue syncup data. action [%d], queue [%.*s].",
                syncAct, pQName->length, pQName->value);

    rc = VDECL_VER(clMsgQDatabaseUpdateClientSync, 4, 0, 0)(idlHandle, syncAct, pQName, pCompAddress, pNewOwnerAddr);
    if(rc != CL_OK)
    {
        clLogError("IDL", "QUPD", "Failed to broadcast queue syncup data. action [%d], queue [%.*s]. error code [0x%x].", 
                syncAct, pQName->length, pQName->value, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgGroupUpdateSendThroughIdl(
        ClMsgSyncActionT syncAct,
        ClNameT *pGroupName,
        SaMsgQueueGroupPolicyT policy)
{
    ClRcT rc;

    clLogTrace("IDL", "GUPD", "Sending group syncup data. action [%d], group [%.*s], policy [%d].",
            syncAct, pGroupName->length, pGroupName->value, policy);

    rc = VDECL_VER(clMsgGroupDatabaseUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncAct, pGroupName, policy, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "GUPD", "Failed to send group syncup data. action [%d], group [%.*s], policy [%d]. error code [0x%x].", 
                syncAct, pGroupName->length, pGroupName->value, policy, rc);
    }

    return rc;
}


ClRcT clMsgGroupMembershipChangeInfoSendThroughIdl(ClMsgSyncActionT syncAct, ClNameT *pGroupName, ClNameT *pQueueName)
{
    ClRcT rc;

    clLogTrace("IDL", "MUPD", "Sending membership syncup data. action [%d], group [%.*s], queue [%.*s].",
            syncAct, pGroupName->length, pGroupName->value, pQueueName->length, pQueueName->value);

    rc = VDECL_VER(clMsgGroupMembershipUpdateClientAsync, 4, 0, 0)(gIdlBcastHandle, syncAct, pGroupName, pQueueName, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "MUPD", "Failed to broadcast membership syncup data. action [%d], group [%.*s], queue [%.*s]. error code [0x%x].",
                syncAct, pGroupName->length, pGroupName->value, pQueueName->length, pQueueName->value, rc);
    }

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


ClRcT clMsgCallClientsMessageReceiveCallback(ClIocPhysicalAddressT *pCompAddr,
        SaMsgHandleT clientHandle,
        SaMsgQueueHandleT qHandle
        )
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = *pCompAddr;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "RCVcb", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "RCVcb", "Calling message received callback for client [0x%x:0x%x].", pCompAddr->nodeAddress, pCompAddr->portId);

    rc = VDECL_VER(clMsgClientsMessageReceiveCallbackClientAsync, 4, 0, 0)(idlHandle, clientHandle, qHandle, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "RCVcb", "Failed to make an Async RMD to client. error code [0x%x].", rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


void clMsgReceivedCallback(ClIdlHandleT idlHandle, ClUint32T sendType, ClNameT *pDest, SaMsgMessageT *pMessage, ClInt64T sendTime, ClHandleT senderHandle, ClInt64T timeout, ClRcT retCode, void* pCookie)
{
    ClRcT rc;
    ClIdlHandleT *pIdlHandle = (ClIdlHandleT*)pCookie;

    rc = clMsgMessageReceivedResponseSend_4_0_0(*pIdlHandle, retCode);
    if(rc != CL_OK)
        clLogError("IDL", "SNDCb", "Failed to send the defered RMD response. error code [0x%x].", rc);
        
    clHeapFree(pIdlHandle);
    if(pMessage)
    {
        if(pMessage->senderName)
            clHeapFree(pMessage->senderName);
        if(pMessage->data)
            clHeapFree(pMessage->data);
    }
}


ClRcT clMsgSendMessage_idl(ClMsgMessageSendTypeT sendType,
        ClIocNodeAddressT nodeAddr,
        ClNameT *pName,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaMsgAckFlagsT ackFlag,
        SaTimeT timeout)
{
    ClRcT rc, retCode;
    ClIdlHandleT *pIdlHandle = NULL;
    ClIdlHandleT idlHandle = 0;
    ClIdlHandleObjT idlObj = {0};

    if(nodeAddr == CL_IOC_BROADCAST_ADDRESS)
    {
        rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(gIdlBcastHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
        if(rc != CL_OK)
            clLogError("IDL", "BCAST", "Failed to broadcast a message. error code [0x%x].", rc);
        return rc;
    }

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = nodeAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "SND", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    if(ackFlag == SA_MSG_MESSAGE_DELIVERED_ACK)
    {
        pIdlHandle = (ClIdlHandleT*) clHeapAllocate(sizeof(*pIdlHandle));
        if(pIdlHandle == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogError("IDL", "SND", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(*pIdlHandle), rc);
            goto out;
        }

        rc = clMsgIdlIdlSyncDefer(pIdlHandle);
        if(rc != CL_OK)
        {
            clLogError("IDL", "SND", "Failed to defer the RMD response. error code [0x%x].", rc);
            goto error_out_1;
        }

        clLogTrace("IDL", "SND", "Sending a message to message server on node [0x%x].", nodeAddr);

        rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(idlHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, clMsgReceivedCallback, pIdlHandle);
        if(rc == CL_OK)
            goto out;

        clLogError("IDL", "SND", "Failed to send a message to message server on node [0x%x]. error code [0x%x].", nodeAddr, rc);
        retCode = clMsgMessageReceivedResponseSend_4_0_0(*pIdlHandle, rc);
        if(retCode != CL_OK)
            clLogError("IDL", "SND", "Failed to send the defered RMD response. error code [0x%x].", retCode);

error_out_1:
        clHeapFree(pIdlHandle);
    }
    else
    {
        clLogTrace("IDL", "SND", "Sending a message to message server on node [0x%x].", nodeAddr);

        rc = VDECL_VER(clMsgMessageReceivedClientAsync, 4, 0, 0)(idlHandle, sendType, pName, pMessage, sendTime, senderHandle, timeout, NULL, NULL);
        if(rc != CL_OK)
            clLogError("IDL", "SND", "Failed to send a message to message server on node [0x%x]. error code [0x%x].", nodeAddr, rc);
    }

    out:
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgQueueInfoGetThroughIdl(
        ClIocNodeAddressT nodeAddr,
        ClNameT *pName,
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pQMvInfo)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = nodeAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "MOV", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "MOV", "Sending a request to message server on node [0x%x] for moving queue [%.*s].", nodeAddr, pName->length, pName->value);

    rc = VDECL_VER(clMsgQueueInfoGetClientSync, 4, 0, 0)(idlHandle, pName, openFlags, pQMvInfo);
    if(rc != CL_OK)
    {
        clLogError("IDL", "MOV", "Failed to get queue [%.*s]'s infromation. error code [0x%x].", pName->length, pName->value, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgQueueMoveMessagesThroughIdl(
        ClIocNodeAddressT nodeAddr,
        ClNameT *pName,
        SaMsgQueueOpenFlagsT openFlags,
        ClIocPhysicalAddressT *pCompAddr)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));

    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = nodeAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "MOV", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "MOV", "Sending a request to message service on node [0x%x] for moving the messages of queue [%.*s].", nodeAddr, pName->length, pName->value);

    rc = VDECL_VER(clMsgQueueMoveMessagesClientSync, 4, 0, 0)(idlHandle, pName, openFlags, pCompAddr);
    if(rc != CL_OK)
    {
        clLogError("IDL", "MOV", "Move messages RMD for queue [%.*s] failed. error code [0x%x].", pName->length, pName->value, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgQueueAllocateThroughIdl(
        ClIocNodeAddressT destNode,
        ClBoolT newQ,
        ClNameT *pQName,
        ClIocPhysicalAddressT *pCompAddress, 
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttrs,
        SaMsgQueueHandleT *pQHandle)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;
    
    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = destNode;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "ALOC", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "ALOC", "Allocate-requeust for queue [%.*s] on node [%d].", pQName->length, pQName->value, destNode);

    rc = VDECL_VER(clMsgQueueAllocateClientSync, 4, 0, 0)(idlHandle, newQ, pQName, pCompAddress, openFlags, pCreationAttrs, pQHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "ALOC", "Queue [%.*s] allocation failed on node [%d]. error code [0x%x].", pQName->length, pQName->value, destNode, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgFailoverQMovedInfoUpdateThroughIdl(ClIocNodeAddressT destNode, ClNameT *pQName)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = destNode;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "QUPD", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "QUPD", "Update queue [%.*s] on node [%d].", pQName->length, pQName->value, destNode);

    rc = VDECL_VER(clMsgFailoverQMovedInfoUpdateClientAsync, 4, 0, 0)(idlHandle, pQName, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("IDL", "QUPD", "Failed to update queue [%.*s] on node [%d]. error code [0x%x].", pQName->length, pQName->value, destNode, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgQueueUnlinkThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pQName)
{
    ClRcT rc;
    ClIdlHandleT idlHandle = 0;
    ClIdlHandleObjT idlObj = {0};

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = nodeAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "UNL", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "UNL", "Sending [%.*s] unlink request to message server on node [0x%x].", pQName->length, pQName->value, nodeAddr);

    rc = VDECL_VER(clMsgQueueUnlinkClientSync, 4, 0, 0)(idlHandle, pQName);
    if(rc != CL_OK)
        clLogError("IDL", "UNL", "Failed to send a message. error code [0x%x].", rc);

    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}


ClRcT clMsgQueueStatusGetThroughIdl(ClIocNodeAddressT nodeAddr, ClNameT *pQName, SaMsgQueueStatusT *pQueueStatus)
{
    ClRcT rc;
    ClIdlHandleT idlHandle = 0;
    ClIdlHandleObjT idlObj = {0};

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = nodeAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("IDL", "STAT", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("IDL", "STAT", "Sending [%.*s] status get request to message server on node [0x%x].", pQName->length, pQName->value, nodeAddr);

    rc = VDECL_VER(clMsgQueueStatusGetClientSync, 4, 0, 0)(idlHandle, pQName, pQueueStatus);
    if(rc != CL_OK)
        clLogError("IDL", "STAT", "Failed to send a message. error code [0x%x].", rc);

    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}



ClRcT clMsgGetDatabasesThroughIdl(ClIocNodeAddressT masterAddr, ClUint8T **ppData, ClUint32T *pSize)
{
    ClRcT rc;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress.nodeAddress = masterAddr;
    idlObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("GET", "DBs", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("GET", "DBs", "Sending request to [%d] for getting the databases.", masterAddr);

    rc = VDECL_VER(clMsgGetDatabasesClientSync, 4, 0, 0)(idlHandle, ppData, pSize);
    if(rc != CL_OK)
    {
        clLogError("GET", "DBs", "Failed to get the databases from [%d]. error code [0x%x].", masterAddr, rc);
    }
    clIdlHandleFinalize(idlHandle);

error_out:
    return rc;
}

ClRcT clMsgSrvIdlInitialize(void)
{
    ClRcT rc;
    ClRcT retCode;

    clOsalMutexInit(&idlMutex);

    CL_OSAL_MUTEX_LOCK(&idlMutex);

    gIdlUcastObj.objId = clObjId_ClIdlHandleObjT; 
    gIdlUcastObj.address.addressType = CL_IDL_ADDRESSTYPE_IOC;
    gIdlUcastObj.address.address.iocAddress.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS; 
    gIdlUcastObj.address.address.iocAddress.iocPhyAddress.portId = CL_IOC_MSG_PORT;
    gIdlUcastObj.flags = 0; 
    gIdlUcastObj.options.timeout = CL_RMD_DEFAULT_TIMEOUT;
    gIdlUcastObj.options.retries = 3; 
    gIdlUcastObj.options.priority = CL_IOC_DEFAULT_PRIORITY;

    rc = clIdlHandleInitialize(&gIdlUcastObj, &gIdlUcastHandle);
    if(rc != CL_OK)
    {    
        clLogError("MSG", "IDL", "Failed to initialize the the IDL. error code [0x%x].", rc); 
        goto error_out;
    }    

    memcpy(&gIdlBcastObj, &gIdlUcastObj, sizeof(gIdlBcastObj));
    gIdlBcastObj.options.priority = CL_IOC_ORDERED_PRIORITY;
    rc = clIdlHandleInitialize(&gIdlBcastObj, &gIdlBcastHandle);
    if(rc != CL_OK)
    {    
        clLogError("MSG", "IDL", "Failed to initialize the the IDL. error code [0x%x].", rc); 
        goto error_out_1;
    } 

    CL_OSAL_MUTEX_UNLOCK(&idlMutex);
    goto done_out;

error_out_1:
    retCode = clIdlHandleFinalize(gIdlUcastHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "IDL", "Failed to do the finalize the IDL handle. error code [0x%x].", rc);
error_out:
    CL_OSAL_MUTEX_UNLOCK(&idlMutex);
    clOsalMutexDestroy(&idlMutex);
    
done_out:
    return rc;
}


ClRcT clMsgSrvIdlFinalize(void)
{
    ClRcT rc;

    rc = clIdlHandleFinalize(gIdlUcastHandle);
    if(rc == CL_OK)
        rc = clIdlHandleFinalize(gIdlBcastHandle);

    clOsalMutexDestroy(&idlMutex);

    return rc;
}
