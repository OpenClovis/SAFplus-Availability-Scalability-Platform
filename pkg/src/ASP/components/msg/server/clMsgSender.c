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
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clVersion.h>
#include <clTaskPool.h>
#include <saAis.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgEo.h>
#include <clMsgDatabase.h>
#include <clMsgGroupDatabase.h>
#include <clMsgQueue.h>
#include <clMsgReceiver.h>
#include <clMsgIdl.h>
#include <clMsgDebugInternal.h>


ClHandleDatabaseHandleT gMsgSenderDatabase;


static ClRcT clMsgGroupMessageSend(ClMsgMessageSendTypeT sendType,
        ClMsgGroupRecordT *pGroup,
        ClNameT *pDest,
        SaMsgMessageT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaMsgAckFlagsT ackFlag,
        SaTimeT timeout)
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pQueue;
    SaMsgQueueHandleT qHandle;
    ClIocNodeAddressT destNode;
    ClIocPortT destPort;

    switch(pGroup->policy)
    {
        case SA_MSG_QUEUE_GROUP_ROUND_ROBIN:
            rc = clMsgRRNextMemberGet(pGroup, &pQueue);
            if(rc != CL_OK)
            {
                clLogError("GRP", "SND", "Failed to send to group [%.*s]. error code [0x%x].", pGroup->name.length, pGroup->name.value, rc);
                goto error_out;
            }
            destNode = pQueue->compAddr.nodeAddress;
            destPort = pQueue->compAddr.portId;
            qHandle = pQueue->qHandle;

            if(destNode != gClMyAspAddress)
            {
                rc = clMsgSendMessage_idl(sendType, destNode, &pQueue->qName, pMessage, sendTime, senderHandle, ackFlag, timeout);
                if(rc != CL_OK)
                {
                    clLogError("MSG", "CMS", "Failed to send the message to node [0x%x]. error code [0x%x].", destNode, rc);
                    goto error_out;
                }
            }
            else
            {
                rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
                if(rc != CL_OK)
                {
                    clLogError("MSG", "CMS", "Failed to queue the message to component [0x%x] on this node. error code [0x%x].", destPort, rc);
                    goto error_out;
                }
            }
            break;
        case SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN:
            rc = clMsgRRNextMemberGet(pGroup, &pQueue);
            if(rc != CL_OK)
            {
                clLogError("MSG", "CMS", "Failed to get next member of group [%.*s]. error code [0x%x].", pGroup->name.length, pGroup->name.value, rc);
                goto error_out;
            }
            qHandle = pQueue->qHandle;
            rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
            break;
        case SA_MSG_QUEUE_GROUP_LOCAL_BEST_QUEUE:
            rc = clMsgGroupLocalBestNextMemberGet(pGroup, &pQueue, pMessage->priority);
            if(rc != CL_OK)
            {
                clLogError("MSG", "CMS", "Failed to get next member of group [%.*s]. error code [0x%x].", pGroup->name.length, pGroup->name.value, rc);
                goto error_out;
            }
            qHandle = pQueue->qHandle;
            rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
            break;
        case SA_MSG_QUEUE_GROUP_BROADCAST:
            rc = clMsgSendMessage_idl(sendType, CL_IOC_BROADCAST_ADDRESS, pDest, pMessage, sendTime, senderHandle, 0, timeout);
            if(rc != CL_OK)
                clLogError("MSG", "CMS", "Failed to send a message to the message servers. error code [0x%x].", rc);

            break;
    }

error_out:
    return rc;
}



static ClRcT clMsgMessageSend(ClMsgMessageSendTypeT sendType, ClNameT *pDest, SaMsgMessageT *pMessage, SaTimeT sendTime, ClHandleT senderHandle, SaMsgAckFlagsT ackFlag, SaTimeT timeout)
{
    ClRcT rc;
    ClMsgQueueRecordT *pQueue = NULL;
    ClMsgGroupRecordT *pGroup = NULL;


    /* Sending to Message Queue */
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    
    if(clMsgQNameEntryExists(pDest, &pQueue) == CL_TRUE) {
        SaMsgQueueHandleT qHandle = pQueue->qHandle;
        ClIocNodeAddressT nodeAddress = pQueue->compAddr.nodeAddress;
        if(nodeAddress == 0)
        {
            rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
            clLogError("MSG", "SND", "Queue [%.*s] is being moved to another node. error code [0x%x].", pDest->length, pDest->value, rc);
            goto q_done_out;
        }

        if(nodeAddress == gClMyAspAddress)
        {
            /* Queuing the message as the destination queue is on the same machine. */
            rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
        }
        else
        {
            /* Sending the message as the destination queue is on the some other machine. */
            rc = clMsgSendMessage_idl(sendType, nodeAddress, pDest, pMessage, sendTime, senderHandle, ackFlag, timeout);
        }

        if(rc != CL_OK)
            clLogError("MSG", "SND", "Failed to send to the destination queue. error code [0x%x].", rc);

        goto q_done_out;
    }



    /* Sending to Message Queue Group */
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);

    if(clMsgGroupEntryExists(pDest, &pGroup) == CL_TRUE) { 
        CL_OSAL_MUTEX_LOCK(&pGroup->groupLock);

        rc = clMsgGroupMessageSend(sendType, pGroup, pDest, pMessage, sendTime, senderHandle, ackFlag, timeout);

        CL_OSAL_MUTEX_UNLOCK(&pGroup->groupLock);

        if(rc != CL_OK)
            clLogTrace("MSG", "SND", "Failed to send a message of size [%llu] to [%.*s]. error code [0x%x].", pMessage->size, pDest->length, pDest->value, rc);

        goto group_done_out;
    }



    /* Couldn't find the destination queue. */
    rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
    clLogError("MSG", "SND", "Queue or Queue Group [%.*s] doesnt exists. error code [0x%x].",
            pDest->length, pDest->value, rc);

group_done_out:
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
q_done_out:
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    return rc;
}



ClRcT VDECL_VER(clMsgClientMessageSend, 4, 0, 0)(SaMsgHandleT msgHandle, ClNameT *pDest, SaMsgMessageT *pMessage, SaTimeT sendTime, SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgClientDetailsT *pClient;


    CL_MSG_SERVER_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SND", "Failed to checkout the passed message handle. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clMsgMessageSend(CL_MSG_SEND, pDest, pMessage, sendTime, 0, SA_MSG_MESSAGE_DELIVERED_ACK, timeout);
    if(rc != CL_OK)
        clLogError("MSG", "SND", "Failed to send a message of size [%llu] to [%.*s]. error code [0x%x].", pMessage->size, pDest->length, pDest->value, rc);

    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SND", "Failed to checkin the client handle. error code [0x%x].", retCode);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgClientMessageSendReceive, 4, 0, 0)(SaMsgHandleT msgHandle,
        ClNameT *pDest,
        SaMsgMessageT *pSendMessage,
        SaTimeT sendTime,
        SaMsgMessageT *pRecvMessage,
        SaTimeT *pReplierSendTime,
        SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgClientDetailsT *pClient;
    ClHandleT senderHandle;
    ClMsgWaitingSenderDetailsT *pSender;
    ClTimerTimeOutT tempTime;
    ClMsgGroupRecordT *pGroup;
    ClBoolT monitorDisabled = CL_FALSE;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to chechout the passed message handle. error code [0x%x].",rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    if(clMsgGroupEntryExists(pDest, &pGroup) == CL_TRUE && pGroup->policy == SA_MSG_QUEUE_GROUP_BROADCAST)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("MSG", "SendRecv", "The destination is a group and the policy is SA_MSG_QUEUE_GROUP_BROADCAST. error code [0x%x].",rc);
        goto error_out_1;
    }
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);

    rc = clHandleCreate(gMsgSenderDatabase, sizeof(ClMsgWaitingSenderDetailsT), &senderHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create an Id for the sender. error code [0x%x].",rc);
        goto error_out_1;
    }

    rc = clHandleCheckout(gMsgSenderDatabase, senderHandle, (void **)&pSender);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to checkout the sender id/handle. error code [0x%x].",rc);
        goto error_out_2;
    }
    
    rc = clOsalMutexInit(&pSender->mutex);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create a sender mutex lock. error code [0x%x].", rc);
        goto error_out_3;
    }

    CL_OSAL_MUTEX_LOCK(&pSender->mutex);
    rc = clOsalCondCreate(&pSender->condVar);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create a sender condtional variable. error code [0x%x].", rc);
        goto error_out_4;
    }

    pSender->pRecvMessage = pRecvMessage;
    pSender->pReplierSendTime = pReplierSendTime;

    rc = clMsgMessageSend(CL_MSG_SEND_RECV, pDest, pSendMessage, sendTime, senderHandle, 0, timeout);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to send the message to the destination. error code [0x%x].",rc);
        goto error_out_5;
    }

    clMsgTimeConvert(&tempTime, timeout);

    if((!tempTime.tsSec && !tempTime.tsMilliSec)
       ||
       (tempTime.tsSec * 2 >= CL_TASKPOOL_MONITOR_INTERVAL))
    {
        monitorDisabled = CL_TRUE;
        clTaskPoolMonitorDisable();
    }
    rc = clOsalCondWait(pSender->condVar, &pSender->mutex, tempTime);
    if(monitorDisabled)
    {
        clTaskPoolMonitorEnable();
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        clLogError("MSG", "SendRecv", "Message sending timed out. timeout is [%lld ns]. error code [0x%x].", timeout, rc);
    else if(rc != CL_OK)
        clLogError("MSG", "SendRecv", "Failed at Conditional Wait. error code [0x%x].", rc);
    else
        rc = pSender->replierRc;

error_out_5:
    retCode = clOsalCondDelete(pSender->condVar);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to delete sender conditional variable. error code [0x%x].", retCode);
error_out_4:
    CL_OSAL_MUTEX_UNLOCK(&pSender->mutex);
    retCode = clOsalMutexDestroy(&pSender->mutex); 
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to delete sender mutex. error code [0x%x].", retCode);
error_out_3:
    retCode = clHandleCheckin(gMsgSenderDatabase, senderHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to checkin sender handle. error code [0x%x].", retCode);
error_out_2:
    retCode = clHandleDestroy(gMsgSenderDatabase, senderHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to destroy sender handle. error code [0x%x].", retCode);
error_out_1:
    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to checkin message client handle. error code [0x%x].", retCode);
error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgClientMessageReply, 4, 0, 0)(SaMsgHandleT msgHandle, SaMsgMessageT *pMessage, SaTimeT sendTime, ClHandleT senderId, SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClNameT dummyName = {0, {0}};
    ClMsgReplyDetailsT *pReplyInfo;
    ClMsgClientDetailsT *pClient;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to chechout the passed message handle. error code [0x%x].",rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgReplyDetailsDb, senderId, (void **)&pReplyInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to chechout the passed message handle. error code [0x%x].",rc);
        goto error_out_1;
    }
    
    if(pReplyInfo->timerHandle)
    {
        rc = clTimerDelete(&pReplyInfo->timerHandle);
        if(rc != CL_OK)
            clLogError("MSG", "REPLY", "Failed to delete the replier's timer. error code [0x%x].", rc);
    }

    if(pReplyInfo->senderNode == gClMyAspAddress)
    {
        rc = clMsgReplyReceived(pMessage, sendTime, pReplyInfo->senderHandle, timeout);
    }
    else
    {
        /* FIXME : Passing address of dummyName, just not to crash in IDL, as IDL expects non NULL parameter. */
        rc = clMsgSendMessage_idl(CL_MSG_REPLY_SEND, pReplyInfo->senderNode, &dummyName, pMessage, sendTime, pReplyInfo->senderHandle, 0, timeout);
    }
    if(rc != CL_OK)
        clLogError("MSG", "REPLY", "Failed to send the reply to the destination. error code [0x%x].",rc);

    retCode = clHandleCheckin(gMsgReplyDetailsDb, senderId);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to checkin replier handle. error code [0x%x].", retCode);

    retCode = clHandleDestroy(gMsgReplyDetailsDb, senderId);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to destroy replier handle. error code [0x%x].", retCode);

    error_out_1:
    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to checkin message client handle. error code [0x%x].", retCode);

    error_out:
    return rc;
}


ClRcT clMsgSenderDatabaseInit(void)
{
    ClRcT rc;

    rc = clHandleDatabaseCreate(NULL, &gMsgSenderDatabase);
    if(rc != CL_OK)
        clLogError("MSG", "INI", "Failed to create a handle database for message-client-init-handle. error code [0x%x].", rc);

    return rc;
}


void clMsgSenderDatabaseFin(void)
{
    ClRcT rc;

    rc = clHandleDatabaseDestroy(gMsgSenderDatabase);
    if(rc != CL_OK)
        clLogError("MSG", "INI", "Failed to destroy the the sender database. error code [0x%x].", rc);

    return;
}
