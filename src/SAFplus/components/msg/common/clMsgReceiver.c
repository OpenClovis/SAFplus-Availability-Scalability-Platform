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
 * File        : clMsgReceiver.c
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBufferApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clVersion.h>
#include <clTaskPool.h>
#include <clRmdIpi.h>
#include <clMsgReceiver.h>
#include <clMsgDebugInternal.h>
#include <clMsgIdl.h>

#include <msgCltClientCallsFromServerToClientServer.h>

ClHandleDatabaseHandleT gMsgReplyDetailsDb;

/***********************************************************************************************************************/

static ClRcT clMsgDeleteDetailsStoredForReplier(void *pParam)
{
    ClRcT rc, retCode;
    ClMsgReplyDetailsT *pSenderInfo;
    ClHandleT replyHandle = *(ClHandleT *)pParam;

    rc = clHandleCheckout(gMsgReplyDetailsDb, replyHandle, (void**)&pSenderInfo);
    if(rc != CL_OK)
    {
        clLogError("RPL", "DEL", "Failed to checkout replier handle. error code [0x%x].", rc);
        goto error_out;
    }
    if(pSenderInfo->timerHandle)
    {
        rc = clTimerDelete(&pSenderInfo->timerHandle);
        if(rc != CL_OK)
            clLogError("RPL", "DEL", "Failed to delete replier timer. error code [0x%x].", rc);
    }
    retCode = clHandleCheckin(gMsgReplyDetailsDb, replyHandle);
    if(retCode != CL_OK)
        clLogError("RPL", "DEL", "Failed to check-in replier handle. error code [0x%x].", retCode);

    retCode = clHandleDestroy(gMsgReplyDetailsDb, replyHandle);
    if(retCode != CL_OK)
        clLogError("RPL", "DEL", "Failed to destroy replier handle. error code [0x%x].", retCode);

    error_out:
    return rc;
}


static ClRcT clMsgStoreDetailsForReplier(SaNameT *pSenderName, ClHandleT senderHandle, SaTimeT timeout, ClHandleT **ppReplierId)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgReplyDetailsT *pSenderInfo;
    ClHandleT replyHandle;
    ClTimerTimeOutT tempTimeout = {0};
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to get the RMD originator's address. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCreate(gMsgReplyDetailsDb, sizeof(ClMsgReplyDetailsT), &replyHandle);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to create a Sender Id for reply. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gMsgReplyDetailsDb, replyHandle, (void**)&pSenderInfo);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to checkout a handle for storing the sender info. error code [0x%x].", rc);
        goto error_out_1;
    }

    pSenderInfo->senderAddr = srcAddr;
    pSenderInfo->senderHandle = senderHandle;
    pSenderInfo->timeout = timeout;
    pSenderInfo->myOwnHandle = replyHandle; 

    if(timeout != SA_TIME_MAX && timeout/1000 < CL_MAX_TIMEOUT)
    {
        clMsgTimeConvert(&tempTimeout, timeout);

        rc = clTimerCreateAndStart(tempTimeout, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT, 
                                   clMsgDeleteDetailsStoredForReplier, (void*)&pSenderInfo->myOwnHandle, &pSenderInfo->timerHandle);
        if(rc != CL_OK)
        {
            clLogCritical("MSG", "QUE", "Failed to start a timer for replier. error code [0x%x].", rc);
            goto error_out_2;
        }
    }

    rc = clHandleCheckin(gMsgReplyDetailsDb, replyHandle);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to checkin the handle to replier database. error code [0x%x]", rc);
        goto error_out_3;
    }

    *ppReplierId = &pSenderInfo->myOwnHandle;

    goto out;

    error_out_3:
    if(pSenderInfo->timerHandle)
    {
        retCode = clTimerDelete(&pSenderInfo->timerHandle);
        if(retCode != CL_OK)
            clLogError("MSG", "QUE", "Failed to delete the timer. error code [0x%x].", retCode);
    }
    error_out_2:
    retCode = clHandleCheckin(gMsgReplyDetailsDb, replyHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QUE", "Failed to check-in just created replier handle. error code [0x%x].", retCode);
    error_out_1:
    retCode = clHandleDestroy(gMsgReplyDetailsDb, replyHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QUE", "Failed to destroy the reply handle. error code [0x%x].", retCode);
    error_out:
    out:
    return rc;
}


ClRcT clMsgMessageReceiveCallback(SaMsgQueueHandleT qHandle);
ClRcT clMsgQueueTheLocalMessage(
        ClMsgMessageSendTypeT sendType,
        SaMsgQueueHandleT qHandle,
        ClMsgMessageIovecT *pMessage,
        SaTimeT sendTime,
        ClHandleT senderHandle,
        SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;
    ClMsgReceivedMessageDetailsT *pRecvInfo;
    ClUint32T priority;
    ClHandleT *pReplierHandle = NULL;
    ClUint32T i;
    SaSizeT msgSize = 0;

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to get the handle for queue. error code [0x%x].", rc);
        clLogCritical("MSG", "QUE", "Dropping the received packet.");
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    for (i = 0; i < pMessage->numIovecs; i++)
    {
        msgSize += pMessage->pIovec[i].iov_len;
    }

    priority = pMessage->priority;
    if(pQInfo->size[priority] < pQInfo->usedSize[priority] + msgSize)
    {
        rc = CL_MSG_RC(CL_ERR_BUFFER_OVERRUN);
        clLogCritical("MSG", "QUE", "The priority queue is full. error code [0x%x].", rc); 
        clLogCritical("MSG", "QUE", "Dropping the received packet.");
        goto error_out_1;
    }
    for (i = 0; i < pMessage->numIovecs; i++)
    {
        pRecvInfo = (ClMsgReceivedMessageDetailsT*)clHeapAllocate(sizeof(ClMsgReceivedMessageDetailsT));
        if(pRecvInfo == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("MSG", "QUE", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(ClMsgReceivedMessageDetailsT), rc);
            clLogCritical("MSG", "QUE", "Dropping the received packet.");
            goto error_out_1;
        }

        rc = clMsgIovecToMessageCopy(&pRecvInfo->pMessage, pMessage, i);
        if(rc != CL_OK)
        {
            clLogCritical("MSG", "QUE", "Failed to copy the message to the local memory.");
            clLogCritical("MSG", "QUE", "Dropping the received packet.");
            goto error_out_2;
        }

        pRecvInfo->replyId = 0;

        if(senderHandle != 0)
        {
            rc = clMsgStoreDetailsForReplier(pMessage->senderName, senderHandle, timeout, &pReplierHandle);
            if(rc != CL_OK)
            {
                clLogCritical("MSG", "QUE", "Dropping the received packet.");
                goto error_out_3;
            }
            pRecvInfo->replyId = *pReplierHandle;
        }

        CL_MSG_SEND_TIME_GET(pRecvInfo->sendTime);

        rc = clCntNodeAdd(pQInfo->pPriorityContainer[priority], NULL, (ClCntDataHandleT)pRecvInfo, NULL);
        if(rc != CL_OK)
        {
            clLogCritical("MSG", "QUE", "Failed to add the new message to the message queue with priority %d. error code [0x%x].", priority, rc);
            clLogCritical("MSG", "QUE", "Dropping the received packet.");
            goto error_out_4;
        }
    }

    clOsalCondSignal(pQInfo->qCondVar);
    /*
     * TODO 
     * As we don't support iovec msg receives yet, and we are queueing each msg iovec,
     * it might be justified to do --
     * pQInfo->numberOfMessages[priority] += pMessage->numIovecs; 
     */
    ++pQInfo->numberOfMessages[priority];
    pQInfo->usedSize[priority] = pQInfo->usedSize[priority] + msgSize;

    clLogTrace("MSG", "QUE", "Queued a message of size [%llu] to queue [%.*s].", 
            msgSize, pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value);

    if((pQInfo->state == CL_MSG_QUEUE_OPEN) 
       &&
       (pQInfo->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK))
    {
        for (i = 0; i < pMessage->numIovecs; i++)
            clMsgMessageReceiveCallback(qHandle);
    }

    goto out;

error_out_4:
    if(senderHandle != 0)
    {
        retCode = clMsgDeleteDetailsStoredForReplier((void*)pReplierHandle);
        if(retCode != CL_OK)
            clLogError("MSG", "QUE", "Failed to delete data stored for replier. error code [0x%x].", retCode);
    }
error_out_3:
    clMsgMessageFree(pRecvInfo->pMessage);
error_out_2:
    clHeapFree(pRecvInfo);
error_out_1:
out:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QUE", "Failed to check-in the queue handle to database. error code [0x%x].", retCode);
error_out:
    return rc;
}

ClRcT clMsgReplyReceived(ClMsgMessageIovecT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout);
ClRcT VDECL_VER(clMsgMessageReceived, 4, 0, 0)(ClMsgMessageSendTypeT sendType, ClNameT *pDest, ClMsgMessageIovecT *pMessage, SaTimeT sendTime, ClHandleT senderHandle, SaTimeT timeout)
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pQueue;
    ClUint32T i;

    CL_MSG_INIT_CHECK;

    if ((pMessage == NULL) || (pMessage->pIovec == NULL))
    {
        rc = CL_MSG_RC(CL_ERR_NULL_POINTER);
        clLogCritical("MSG", "RCV", "Passing NULL pointer. error code [0x%x].", rc);
        goto out;
    }

    ClIocPhysicalAddressT srcAddr;
    rc = clRmdSourceAddressGet(&srcAddr);
    clLogTrace("MSG", "RCV", "Queue [%.*s] receiving [%d] messages from [0x%x,0x%x].",
               pDest->length, pDest->value,
               (ClInt32T)pMessage->numIovecs, srcAddr.nodeAddress, srcAddr.portId);

    /* Reply for saMsgSendReceiveMessage() is received. */
    if(sendType == CL_MSG_REPLY_SEND)
    {
        rc = clMsgReplyReceived(pMessage, sendTime, senderHandle, timeout);

        /* Free iov_base allocated by the IDL */
        for (i = 0; i < pMessage->numIovecs; i++)
        {
            if (pMessage->pIovec[i].iov_base)
            {
                clHeapFree(pMessage->pIovec[i].iov_base);
                pMessage->pIovec[i].iov_base = NULL;
            }
        }
        goto out;
    }
    /* Queuing the message to the local queue on this node. */
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    if(clMsgQNameEntryExists(pDest, &pQueue) == CL_TRUE)
    {
        SaMsgQueueHandleT qHandle = pQueue->qHandle;
        rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
        goto q_done_out;
    }

    /* There is no queue or queue-group on this node to consume this message. */
    rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
    clLogCritical("MSG", "RCV", "No queue/queue-group with name [%.*s]. error code [0x%x].", pDest->length, pDest->value, rc);
    clLogCritical("MSG", "RCV", "Dropping a received packet.");

q_done_out:
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    if (rc != CL_OK)
    {
        for (i = 0; i < pMessage->numIovecs; i++)
        {
            if (pMessage->pIovec[i].iov_base)
            {
                clHeapFree(pMessage->pIovec[i].iov_base);
                pMessage->pIovec[i].iov_base = NULL;
            }
        }
    }
out:
    return rc;
}

/***********************************************************************************************************/
ClRcT clMsgQueueGetMessagesAndMove(
                                   ClMsgQueueInfoT *pQInfo,
                                   ClIocPhysicalAddressT destCompAddr
                                   )
{
    ClRcT rc = CL_OK;
    ClUint32T i;
    ClMsgReceivedMessageDetailsT *pRecvInfo;
    ClMsgReplyDetailsT *pSendersInfo;
    ClMsgReplyDetailsT senderInfo;
    ClCntNodeHandleT nodeHandle = NULL;
    ClCntNodeHandleT nextNodeHandle = NULL;


    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; i++)
    {
        rc = clCntFirstNodeGet(pQInfo->pPriorityContainer[i], &nodeHandle);

        while(CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            if(rc != CL_OK)
            {
                clLogCritical("MSG", "RTV", "Failed to get a message from a queue of priority %d. error code [0x%x].", i, rc);
                clLogCritical("MSG", "RTV", "Couldnt retrieve packet for moving. Dropping a packet.");
                goto error_next_entry;
            }

            rc = clCntNodeUserDataGet(pQInfo->pPriorityContainer[i], nodeHandle, (ClCntDataHandleT*)&pRecvInfo);
            if(rc != CL_OK)
            {
                clLogCritical("MSG", "RTV", "Failed to get the user-data from the first node. error code 0x%x", rc);
                clLogCritical("MSG", "RTV", "Couldnt retrieve packet for moving. Dropping a packet.");
                goto error_next_entry;
            }

            memset(&senderInfo, 0, sizeof(senderInfo));
            if(pRecvInfo->replyId != 0)
            {
                rc = clHandleCheckout(gMsgReplyDetailsDb, pRecvInfo->replyId, (void**)&pSendersInfo);
                if(rc != CL_OK)
                {
                    clLogCritical("MSG", "RTV", "Failed to get the user-data from the first node. error code 0x%x", rc);
                    clLogCritical("MSG", "RTV", "Couldnt retrieve packet for moving. Dropping a packet.");
                    goto error_continue;
                }
                if(pSendersInfo->timerHandle)
                {
                    rc = clTimerDelete(&pSendersInfo->timerHandle);
                    if(rc != CL_OK)
                        clLogError("MSG", "RTV", "Failed to delete the timer started for a send-receive message. error code [0x%x].", rc);
                }
                memcpy(&senderInfo, pSendersInfo, sizeof(senderInfo));

                rc = clHandleCheckin(gMsgReplyDetailsDb, pRecvInfo->replyId);
                if(rc != CL_OK)
                    clLogError("MSG", "RTV", "Failed to check-in replier handle. error code [0x%x].", rc);
                rc = clHandleDestroy(gMsgReplyDetailsDb, pRecvInfo->replyId);
                if(rc != CL_OK)
                    clLogError("MSG", "RTV", "Failed to destroy replier handle. error code [0x%x].", rc);
            }

            ClMsgMessageIovecT msgVector;
            struct iovec iovec = {0};
            iovec.iov_base = (void*)pRecvInfo->pMessage->data;
            iovec.iov_len = pRecvInfo->pMessage->size;

            msgVector.type = pRecvInfo->pMessage->type;
            msgVector.version = pRecvInfo->pMessage->version;
            msgVector.senderName = pRecvInfo->pMessage->senderName;
            msgVector.priority = pRecvInfo->pMessage->priority;
            msgVector.pIovec = &iovec;
            msgVector.numIovecs = 1;

            rc = clMsgSendMessage_idl(CL_MSG_SEND, destCompAddr, &pQInfo->pQueueEntry->qName, &msgVector,
                                      pRecvInfo->sendTime, senderInfo.senderHandle, senderInfo.timeout, CL_TRUE, 0, NULL, NULL);
            if(rc != CL_OK)
            {
                clLogError("MSG", "RTV", "Failed to move message of queue [%.*s]. error code [0x%x].", 
                           pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, rc);
                goto error_out;
            }

            error_continue:
            clMsgMessageFree(pRecvInfo->pMessage);
            clHeapFree(pRecvInfo);

            error_next_entry:
            rc = clCntNextNodeGet(pQInfo->pPriorityContainer[i], nodeHandle, &nextNodeHandle);
            clCntNodeDelete(pQInfo->pPriorityContainer[i], nodeHandle);
            nodeHandle = nextNodeHandle;
        }
        rc = CL_OK;
    }

    error_out:
    return rc;
}

/***********************************************************************************************************/

ClRcT clMsgReceiverDatabaseInit(void)
{
    ClRcT rc;

    rc = clHandleDatabaseCreate(NULL, &gMsgReplyDetailsDb);
    if(rc != CL_OK)
        clLogError("MSG", "INI", "Failed to create a handle database for message-client-init-handle. error code [0x%x].", rc);

    return rc;
}


void clMsgReceiverDatabaseFin(void)
{
    ClRcT rc;

    rc = clHandleDatabaseDestroy(gMsgReplyDetailsDb);
    if(rc != CL_OK)
        clLogError("RDb", "FIN", "Failed to destroy the reply handle database. error code [0x%x].",rc);

    return;
}

