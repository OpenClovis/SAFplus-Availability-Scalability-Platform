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
 * File        : clMsgSender.c
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clVersion.h>
#include <clTaskPool.h>
#include <saAis.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgQueue.h>
#include <clMsgReceiver.h>
#include <clMsgSender.h>
#include <clMsgIdl.h>
#include <clMsgDebugInternal.h>
#include <clMsgCkptClient.h>


ClHandleDatabaseHandleT gMsgSenderDatabase;

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

ClRcT clMsgReplyReceived(ClMsgMessageIovecT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgWaitingSenderDetailsT *pReplyInfo;

    if ((pMessage->numIovecs != 1) || (pMessage->pIovec == NULL))
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogCritical("MSG", "GOT-REPLY", "Invalid parameters.");
        goto error_out;
    }

    rc = clHandleCheckout(gMsgSenderDatabase, senderHandle, (void **)&pReplyInfo);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "GOT-REPLY", "Failed to get the sender details. error code [0x%x].", rc);
        clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pReplyInfo->mutex);
    if(pReplyInfo->pRecvMessage->size != 0 && pReplyInfo->pRecvMessage->size < pMessage->pIovec[0].iov_len)
    {
        rc = CL_MSG_RC(CL_ERR_NO_SPACE);
        pReplyInfo->pRecvMessage->size = pMessage->pIovec[0].iov_len;
        clLogCritical("MSG", "GOT-REPLY", "The reply buffer provided is very small. error code [0x%x].", rc);
        clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
        goto error_out_1;
    }
    else if(pReplyInfo->pRecvMessage->size == 0)
    {
        ClUint8T *pTemp;
        pTemp = (ClUint8T*)clHeapAllocate(pMessage->pIovec[0].iov_len);
        if(pTemp == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("MSG", "GOT-REPLY", "Failed to allocate memory of size %u bytes. error code [0x%x].", (unsigned int)pMessage->pIovec[0].iov_len, rc);
            clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
            goto error_out_1;
        }
        clHeapFree(pReplyInfo->pRecvMessage->data);
        pReplyInfo->pRecvMessage->data = pTemp;
    }

    pReplyInfo->pRecvMessage->type = pMessage->type;
    pReplyInfo->pRecvMessage->version = pMessage->version;
    pReplyInfo->pRecvMessage->priority = pMessage->priority;
    memcpy(pReplyInfo->pRecvMessage->senderName, pMessage->senderName, sizeof(SaNameT));
    memcpy(pReplyInfo->pRecvMessage->data, pMessage->pIovec[0].iov_base, pMessage->pIovec[0].iov_len);
    pReplyInfo->pRecvMessage->size = pMessage->pIovec[0].iov_len;

    *pReplyInfo->pReplierSendTime = sendTime;

error_out_1:
    pReplyInfo->replierRc = rc;

    clOsalCondSignal(pReplyInfo->condVar);
    CL_OSAL_MUTEX_UNLOCK(&pReplyInfo->mutex);

    retCode = clHandleCheckin(gMsgSenderDatabase, senderHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "GOT-REPLY", "Failed to checkin a handle to sender db. error code [0x%x].", retCode);
error_out:
    return rc;
}

static ClRcT clMsgMessageSend(ClIocAddressT * pDestAddr, 
                         ClMsgMessageSendTypeT sendType, 
                         SaNameT *pDest, 
                         ClMsgMessageIovecT *pMessage,
                         SaTimeT sendTime, 
                         ClHandleT senderHandle, 
                         SaTimeT timeout,
                         ClBoolT isSync,
                         SaMsgAckFlagsT ackFlag,
                         MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
                         void *cookie)
{
    ClRcT rc;
    ClMsgQueueRecordT *pQueue = NULL;
    ClMsgMessageIovecT *pTempMessage = NULL;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    clLogDebug("MSG", "SND", "Sending message to component [0x%x,0x%x].",
               pDestAddr->iocPhyAddress.nodeAddress, pDestAddr->iocPhyAddress.portId);

    if((pDestAddr->iocPhyAddress.nodeAddress == gLocalAddress)
            && (pDestAddr->iocPhyAddress.portId == gLocalPortId))
    {
        if(clMsgQNameEntryExists(pDest, &pQueue) == CL_FALSE)
        {
            rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
            clLogError("MSG", "SND", "Queue [%.*s] doesnt exists. error code [0x%x].",
                            pDest->length, pDest->value, rc);
            goto out;
        }

        SaMsgQueueHandleT qHandle = pQueue->qHandle;

        /* Allocate memory to queue message on the same machine */
        rc = clMsgIovecToIovecCopy(&pTempMessage, pMessage);
        if(rc != CL_OK)
        {
            clLogCritical("MSG", "SND", "Failed to copy the message to the local memory.");
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            goto out;
        }

        /* Queuing the message as the destination queue is on the same machine. */
        rc = clMsgQueueTheLocalMessage(sendType, qHandle, pTempMessage, sendTime, senderHandle, timeout);
        if(rc != CL_OK)
        {
            for (ClUint32T i = 0; i < pTempMessage->numIovecs; i++)
            {
                if (pTempMessage->pIovec[i].iov_base)
                    clHeapFree(pTempMessage->pIovec[i].iov_base);
            }
        }
        if (pTempMessage)
        {
            if (pTempMessage->senderName)
                clHeapFree(pTempMessage->senderName);
            if (pTempMessage->pIovec)
                clHeapFree(pTempMessage->pIovec);
            clHeapFree(pTempMessage);
        }

        if (ackFlag == SA_MSG_MESSAGE_DELIVERED_ACK)
        {
            fpAsyncCallback(0
                            , sendType, pDest, NULL, sendTime
                            , senderHandle, timeout, rc, cookie);
        }
    }
    else
    {
        /* Sending the message as the destination queue is on the some other machine. */
        rc = clMsgSendMessage_idl(sendType, pDestAddr->iocPhyAddress, pDest, pMessage, sendTime, senderHandle, timeout, isSync, ackFlag, fpAsyncCallback, cookie);
    }

out:
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    return rc;
}

ClRcT clMsgClientMessageSend(ClIocAddressT *pDestAddr, 
                             ClIocAddressT *pServerAddr, 
                             SaNameT *pDest, 
                             ClMsgMessageIovecT *pMessage,
                             SaTimeT sendTime, 
                             SaTimeT timeout,
                             ClBoolT isSync,
                             SaMsgAckFlagsT ackFlag,
                             MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
                             void *cookie)
{
    ClRcT rc = CL_OK;
    ClUint32T i;
    SaNameT tempDest;
    ClIocAddressT queueAddr;
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    ClMsgQueueCkptDataT queueData = {{0}};

    if(pDestAddr->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
    {
        if (clMsgQGroupCkptDataGet((SaNameT*)pDest, &qGroupData) == CL_OK)
        {
            for (i = 0; i < qGroupData.numberOfQueues; i++)
            {
                SaNameT *queueName = (SaNameT *)(qGroupData.pQueueList + i);

                saNameCopy(&tempDest, (SaNameT *)queueName);

                if(clMsgQCkptExists((SaNameT*)&tempDest, &queueData))
                {
                    if (queueData.qAddress.nodeAddress != 0)
                    {
                        queueAddr.iocPhyAddress = queueData.qAddress;

                        rc = clMsgMessageSend(&queueAddr, CL_MSG_SEND, &tempDest, pMessage, sendTime, 0, timeout, CL_FALSE, ackFlag, fpAsyncCallback, cookie);
                        if(rc != CL_OK)
                        {
                            clLogError("MSG", "SND", "Failed to send [%u] messages to [%.*s]. error code [0x%x].", pMessage->numIovecs, pDest->length, pDest->value, rc);
                            goto error_out;
                        }
                    }
                    if (queueData.qServerAddress.nodeAddress != 0)
                    {
                        queueAddr.iocPhyAddress = queueData.qServerAddress;

                        rc = clMsgMessageSend(&queueAddr, CL_MSG_SEND, &tempDest, pMessage, sendTime, 0, timeout, CL_FALSE, 0, NULL, NULL);
                        if(rc != CL_OK)
                        {
                            clLogError("MSG", "SND", "Failed to send [%u] messages to [%.*s]. error code [0x%x].", pMessage->numIovecs, pDest->length, pDest->value, rc);
                            goto error_out;
                        }
                    }
                }
            }
            clMsgQGroupCkptDataFree(&qGroupData);
        }
        else
        {
            rc = CL_ERR_DOESNT_EXIST;
            clLogError("MSG", "SND", "Message queue group [%.*s] does not exist. error code [0x%x].", pDest->length, pDest->value, rc);
            goto error_out;
        }

    }
    else
    {
        if (pDestAddr->iocPhyAddress.nodeAddress != 0)
        {
            rc = clMsgMessageSend(pDestAddr, CL_MSG_SEND, pDest, pMessage, sendTime, 0, timeout, isSync, ackFlag, fpAsyncCallback, cookie);
            if(rc != CL_OK)
            {
                clLogError("MSG", "SND", "Failed to send [%u] messages to [%.*s]. error code [0x%x].", pMessage->numIovecs, pDest->length, pDest->value, rc);
                goto error_out;
            }
        }

        if (pServerAddr->iocPhyAddress.nodeAddress != 0)
        {
            rc = clMsgMessageSend(pServerAddr, CL_MSG_SEND, pDest, pMessage, sendTime, 0, timeout, CL_FALSE, 0, NULL, NULL);
            if(rc != CL_OK)
            {
                clLogError("MSG", "SND", "Failed to send [%u] messages to [%.*s]. error code [0x%x].", pMessage->numIovecs, pDest->length, pDest->value, rc);
                goto error_out;
            }
        }
    }

error_out:
    return rc;
}

ClRcT clMsgClientMessageSendReceive(ClIocAddressT * pDestAddr,
                             ClIocAddressT * pServerAddr,
                             SaNameT *pDest,
                             SaMsgMessageT *pSendMessage,
                             SaTimeT sendTime,
                             SaMsgMessageT *pRecvMessage,
                             SaTimeT *pReplierSendTime,
                             SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClHandleT senderHandle;
    ClMsgWaitingSenderDetailsT *pSender;
    ClTimerTimeOutT tempTime;
    ClBoolT monitorDisabled = CL_FALSE;

    ClMsgMessageIovecT msgVector;
    struct iovec iovec = {0};
    iovec.iov_base = (void*)pSendMessage->data;
    iovec.iov_len = pSendMessage->size;

    msgVector.type = pSendMessage->type;
    msgVector.version = pSendMessage->version;
    msgVector.senderName = pSendMessage->senderName;
    msgVector.priority = pSendMessage->priority;
    msgVector.pIovec = &iovec;
    msgVector.numIovecs = 1;

    if(pDestAddr->iocPhyAddress.nodeAddress == 0)
    {
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("MSG", "SND", "Persistent queue [%.*s] is closed. error code [0x%x].", pDest->length, pDest->value, rc);
        goto error_out_1;
    }

    if(pDestAddr->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(rc,("The destination is a group and the policy is SA_MSG_QUEUE_GROUP_BROADCAST. error code [0x%x].",rc));
        goto error_out_1;
    }

    /* GAS: handle creation is a linear search through an handle array.  Can a pointer be used here instead of a handle? */
    rc = clHandleCreate(gMsgSenderDatabase, sizeof(ClMsgWaitingSenderDetailsT), &senderHandle);
    if(rc != CL_OK)
    {
        clDbgResourceLimitExceeded(clDbgHandleResource,clDbgSafMessageHandleResource,("Failed to create an Id for the sender. error code [0x%x].",rc));
        goto error_out;
    }

    rc = clHandleCheckout(gMsgSenderDatabase, senderHandle, (void **)&pSender);
    if(rc != CL_OK)  // I just created the handle so this must work...
    {
        clLogError("MSG", "SendRecv", "Failed to checkout the sender id/handle. error code [0x%x].",rc);
        goto error_out_1;
    }

    /* GAS: Mutex & cond are relatively expensive resources to create.  Do we really need 1 per send/recv?
       I see now that this is used due to the use of  1-thread per send/recv.  This must be changed to use
       a queue of recv threads because we can't be creating a thread for every send and recv.
     */
    rc = clOsalMutexInit(&pSender->mutex);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create a sender mutex lock. error code [0x%x].", rc);
        goto error_out_2;
    }

    CL_OSAL_MUTEX_LOCK(&pSender->mutex);
    rc = clOsalCondCreate(&pSender->condVar);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to create a sender conditional variable. error code [0x%x].", rc);
        goto error_out_3;
    }

    pSender->pRecvMessage = pRecvMessage;
    pSender->pReplierSendTime = pReplierSendTime;

    /* Send the message to the destination */
    rc = clMsgMessageSend(pDestAddr, CL_MSG_SEND_RECV, pDest, &msgVector, sendTime, senderHandle, timeout, CL_FALSE, 0, NULL, NULL);
    if(rc != CL_OK)
    {
        clLogError("MSG", "SendRecv", "Failed to send the message to the destination. error code [0x%x].",rc);
        goto error_out_4;
    }

    /* Echo the message to the controlling server for persistent queues... */
    if (pServerAddr->iocPhyAddress.nodeAddress != 0)
    {
        rc = clMsgMessageSend(pServerAddr, CL_MSG_SEND, pDest, &msgVector, sendTime, senderHandle, timeout, CL_FALSE, 0, NULL, NULL);
        if(rc != CL_OK)
        {
            clLogError("MSG", "SendRecv", "Failed to send the message to the destination. error code [0x%x].",rc);
            goto error_out_4;
        }
    }

    /* The the user passed a timeout that is larger then our task pool
       monitoring interval then we need to temporarily disable the monitor
       before waiting */
    clMsgTimeConvert(&tempTime, timeout);
    if((!tempTime.tsSec && !tempTime.tsMilliSec)
       ||
       (tempTime.tsSec * 2 >= CL_TASKPOOL_MONITOR_INTERVAL))
    {
        monitorDisabled = CL_TRUE;
        clTaskPoolMonitorDisable();
    }

    /* Wait for the reply to come in */
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

error_out_4:
    retCode = clOsalCondDelete(pSender->condVar);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to delete sender conditional variable. error code [0x%x].", retCode);
error_out_3:
    CL_OSAL_MUTEX_UNLOCK(&pSender->mutex);
    retCode = clOsalMutexDestroy(&pSender->mutex); 
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to delete sender mutex. error code [0x%x].", retCode);
error_out_2:
    retCode = clHandleCheckin(gMsgSenderDatabase, senderHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to check-in sender handle. error code [0x%x].", retCode);
error_out_1:
    retCode = clHandleDestroy(gMsgSenderDatabase, senderHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "SendRecv", "Failed to destroy sender handle. error code [0x%x].", retCode);
error_out:
    return rc;
}


ClRcT clMsgClientMessageReply(SaMsgMessageT *pMessage,
                              SaTimeT sendTime, 
                              ClHandleT senderId, 
                              SaTimeT timeout,
                              ClBoolT isSync,
                              SaMsgAckFlagsT ackFlag,
                              MsgCltSrvClMsgMessageReceivedAsyncCallbackT fpAsyncCallback,
                              void *cookie)
{
    ClRcT rc;
    ClRcT retCode;
    SaNameT dummyName = {0, {0}};
    ClMsgReplyDetailsT *pReplyInfo;

    ClMsgMessageIovecT msgVector;
    struct iovec iovec = {0};
    iovec.iov_base = (void*)pMessage->data;
    iovec.iov_len = pMessage->size;

    msgVector.type = pMessage->type;
    msgVector.version = pMessage->version;
    msgVector.senderName = pMessage->senderName;
    msgVector.priority = pMessage->priority;
    msgVector.pIovec = &iovec;
    msgVector.numIovecs = 1;

    rc = clHandleCheckout(gMsgReplyDetailsDb, senderId, (void **)&pReplyInfo);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REPLY", "Failed to check-out the passed message handle. error code [0x%x].",rc);
        goto error_out;
    }
    
    if(pReplyInfo->timerHandle)
    {
        rc = clTimerDelete(&pReplyInfo->timerHandle);
        if(rc != CL_OK)
            clLogError("MSG", "REPLY", "Failed to delete the replier's timer. error code [0x%x].", rc);
    }

    if((pReplyInfo->senderAddr.nodeAddress == gLocalAddress)
        && (pReplyInfo->senderAddr.portId == gLocalPortId))
    {
        /* Allocate memory to store reply message when sending msg locally. */
        rc = clMsgReplyReceived(&msgVector, sendTime, pReplyInfo->senderHandle, timeout);

        if (ackFlag == SA_MSG_MESSAGE_DELIVERED_ACK)
        {
            fpAsyncCallback(0
                            , CL_MSG_REPLY_SEND, &dummyName, NULL, sendTime
                            , pReplyInfo->senderHandle, timeout, rc, cookie);
        }
    }
    else
    {
        rc = clMsgSendMessage_idl(CL_MSG_REPLY_SEND, pReplyInfo->senderAddr, &dummyName, &msgVector, sendTime, pReplyInfo->senderHandle,  timeout, isSync, ackFlag, fpAsyncCallback, cookie);
    }
    if(rc != CL_OK)
        clLogError("MSG", "REPLY", "Failed to send the reply to the destination. error code [0x%x].",rc);

    retCode = clHandleCheckin(gMsgReplyDetailsDb, senderId);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to check-in replier handle. error code [0x%x].", retCode);

    retCode = clHandleDestroy(gMsgReplyDetailsDb, senderId);
    if(retCode != CL_OK)
        clLogError("MSG", "REPLY", "Failed to destroy replier handle. error code [0x%x].", retCode);

    error_out:
    return rc;
}
