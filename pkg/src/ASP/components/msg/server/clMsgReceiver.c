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
#include <clBufferApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clVersion.h>
#include <clTaskPool.h>
#include <clRmdIpi.h>
#include <clMsgEo.h>
#include <clMsgReceiver.h>
#include <clMsgDatabase.h>
#include <clMsgGroupDatabase.h>
#include <clMsgSender.h>
#include <clMsgCommon.h>
#include <clMsgDebugInternal.h>
#include <clMsgIdl.h>


typedef struct {
    SaMsgMessageT *pMessage;
    SaTimeT sendTime;
    ClHandleT replyId;
} ClMsgReceivedMessageDetailsT;


ClHandleDatabaseHandleT gMsgReplyDetailsDb;

/***********************************************************************************************************************/

ClRcT clMsgReplyReceived(SaMsgMessageT *pMessage, SaTimeT sendTime, SaMsgSenderIdT senderHandle, SaTimeT timeout)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgWaitingSenderDetailsT *pReplyInfo;

    rc = clHandleCheckout(gMsgSenderDatabase, senderHandle, (void **)&pReplyInfo);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "GOT-REPLY", "Failed to get the sender details. error code [0x%x].", rc);
        clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pReplyInfo->mutex);
    if(pReplyInfo->pRecvMessage->size != 0 && pReplyInfo->pRecvMessage->size < pMessage->size)
    {
        rc = CL_MSG_RC(CL_ERR_NO_SPACE);
        pReplyInfo->pRecvMessage->size = pMessage->size;
        clLogCritical("MSG", "GOT-REPLY", "The reply buffer provided is very small. error code [0x%x].", rc);
        clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
        goto error_out_1;
    }
    else if(pReplyInfo->pRecvMessage->size == 0)
    {
        ClUint8T *pTemp;
        pTemp = (ClUint8T*)clHeapAllocate(pMessage->size);
        if(pTemp == NULL)
        {
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogCritical("MSG", "GOT-REPLY", "Failed to allocate memory of size %llu bytes. error code [0x%x].", pMessage->size, rc);
            clLogCritical("MSG", "GOT-REPLY", "Dropping the received packet.");
            goto error_out_1;
        }
        clHeapFree(pReplyInfo->pRecvMessage->data);
        pReplyInfo->pRecvMessage->data = pTemp; 
    }
    
    pReplyInfo->pRecvMessage->type = pMessage->type;
    pReplyInfo->pRecvMessage->version = pMessage->version;
    pReplyInfo->pRecvMessage->size = pMessage->size;
    memcpy(pReplyInfo->pRecvMessage->senderName, pMessage->senderName, sizeof(ClNameT));
    memcpy(pReplyInfo->pRecvMessage->data, pMessage->data, pMessage->size);
    pReplyInfo->pRecvMessage->priority = pMessage->priority;

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
        clLogError("RPL", "DEL", "Failed to checkin replier handle. error code [0x%x].", retCode);

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

    pSenderInfo->senderNode = srcAddr.nodeAddress;
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
        clLogError("MSG", "QUE", "Failed to checkin just created replier handle. error code [0x%x].", retCode);
    error_out_1:
    retCode = clHandleDestroy(gMsgReplyDetailsDb, replyHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "QUE", "Failed to destroy the reply handle. error code [0x%x].", rc);
    error_out:
    out:
    return rc;
}



ClRcT clMsgQueueTheLocalMessage(
        ClMsgMessageSendTypeT sendType,
        SaMsgQueueHandleT qHandle,
        SaMsgMessageT *pMessage,
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


    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "QUE", "Failed to get the handle for queue. error code [0x%x].", rc);
        clLogCritical("MSG", "QUE", "Dropping the received packet.");
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    priority = pMessage->priority;
    if(pQInfo->size[priority] < pQInfo->usedSize[priority] + pMessage->size)
    {
        rc = CL_MSG_RC(CL_ERR_BUFFER_OVERRUN);
        clLogCritical("MSG", "QUE", "The priority queue is full. error code [0x%x].", rc); 
        clLogCritical("MSG", "QUE", "Dropping the received packet.");
        goto error_out_1;
    }

    pRecvInfo = (ClMsgReceivedMessageDetailsT*)clHeapAllocate(sizeof(ClMsgReceivedMessageDetailsT));
    if(pRecvInfo == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogCritical("MSG", "QUE", "Failed to allcoate memory of %zd bytes. error code [0x%x].", sizeof(ClMsgReceivedMessageDetailsT), rc);
        clLogCritical("MSG", "QUE", "Dropping the received packet.");
        goto error_out_1;
    }
        
    rc = clMsgMessageToMessageCopy(&pRecvInfo->pMessage, pMessage);
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

    clOsalCondSignal(pQInfo->qCondVar);

    ++pQInfo->numberOfMessages[priority];
    pQInfo->usedSize[priority] = pQInfo->usedSize[priority] + pMessage->size;

    clLogTrace("MSG", "QUE", "Queued a message of size [%llu] to queue [%.*s].", 
            pMessage->size, pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value);

    if((pQInfo->state == CL_MSG_QUEUE_OPEN) 
       &&
       pQInfo->pOwner
       &&
       (pQInfo->openFlags & SA_MSG_QUEUE_RECEIVE_CALLBACK))
        clMsgCallClientsMessageReceiveCallback(&pQInfo->pOwner->address, pQInfo->pOwner->cltHandle, qHandle); 

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
        clLogError("MSG", "QUE", "Failed to checkin the queue handle to database. error code [0x%x].", retCode);
error_out:
    return rc;
}


static ClRcT clMsgQueueMessageToGroup(ClMsgMessageSendTypeT sendType, ClMsgGroupRecordT *pGroup, SaMsgMessageT *pMessage, SaTimeT sendTime, ClHandleT senderHandle, SaTimeT timeout)
{
    ClRcT rc = CL_OK;
    ClListHeadT *pQList;
    register ClListHeadT *pTemp;
    ClMsgGroupsQueueDetailsT *pQueue;

    switch(pGroup->policy)
    {
        case SA_MSG_QUEUE_GROUP_BROADCAST:
            pQList = &pGroup->qList;
            CL_LIST_FOR_EACH(pTemp, pQList)
            {
                pQueue = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
                if(pQueue->pQueueDetails->compAddr.nodeAddress == gClMyAspAddress)
                {
                    rc = clMsgQueueTheLocalMessage(sendType, pQueue->pQueueDetails->qHandle, pMessage, sendTime, senderHandle, timeout);
                    if(rc != CL_OK)
                        clLogError("MSG", "RCVD", "Failed to queue a message to queue [%.*s]. error code [0x%x].", 
                                pQueue->pQueueDetails->qName.length, pQueue->pQueueDetails->qName.value, rc);
                }
            }
            break;
        default:
            clLogCritical("MSG", "RCVD", "No Message should make it here.");
            break;
    }

    return rc;
}


ClRcT VDECL_VER(clMsgMessageReceived, 4, 0, 0)(ClMsgMessageSendTypeT sendType, ClNameT *pDest, SaMsgMessageT *pMessage, SaTimeT sendTime, ClHandleT senderHandle, SaTimeT timeout)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRecordT *pGroup;
    ClMsgQueueRecordT *pQueue;


    CL_MSG_SERVER_INIT_CHECK;

    /* Reply for saMsgSendReceiveMessage() is received. */
    if(sendType == CL_MSG_REPLY_SEND)
        return clMsgReplyReceived(pMessage, sendTime, senderHandle, timeout);

    /* Queuing the message to the local queue on this node. */
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    if(clMsgQNameEntryExists(pDest, &pQueue) == CL_TRUE)
    {
        SaMsgQueueHandleT qHandle = pQueue->qHandle;
        rc = clMsgQueueTheLocalMessage(sendType, qHandle, pMessage, sendTime, senderHandle, timeout);
        goto q_done_out;
    }


    /* Queuing the message in the Group's queues. */
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);

    if(clMsgGroupEntryExists(pDest, &pGroup) == CL_TRUE)
    {
        CL_OSAL_MUTEX_LOCK(&pGroup->groupLock);
        rc = clMsgQueueMessageToGroup(sendType, pGroup, pMessage, sendTime, senderHandle, timeout);
        if(rc != CL_OK)
            clLogError("MSG", "RCVD", "Failed to queue the message to group [%.*s].", pGroup->name.length, pGroup->name.value);
        CL_OSAL_MUTEX_UNLOCK(&pGroup->groupLock);
        goto group_done_out;
    }


    /* There is no queue or queue-group on this node to consume this message. */
    rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
    clLogCritical("MSG", "RCV", "No queue/queue-group with name [%.*s]. error code [0x%x].", pDest->length, pDest->value, rc);
    clLogCritical("MSG", "RCV", "Dropping a received packet.");

group_done_out:
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
q_done_out:
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    return rc;
}

/***********************************************************************************************************************/

ClRcT  VDECL_VER(clMsgClientMessageGet, 4, 0, 0)(SaMsgQueueHandleT queueHandle,
        SaMsgMessageT *pTempMessage,
        SaTimeT *pSendTime,
        SaMsgSenderIdT *pSenderId,
        SaTimeT timeout)
{
    ClRcT rc = CL_OK;
    ClRcT retCode;
    ClCntNodeHandleT nodeHandle = NULL;
    SaMsgMessageT *pRecvMessage;
    ClUint32T i=0;
    ClBoolT condVarFlag = CL_FALSE;
    ClTimerTimeOutT tempTimeout;
    ClMsgQueueInfoT *pQInfo = NULL;
    ClMsgReceivedMessageDetailsT *pRecvInfo;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("MSG", "GET", "Failed to checkout the queue handle. error code [0x%x].",rc);
        goto error_out;
    }
    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

get_message:
    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; i++)
    {
        rc = clCntFirstNodeGet(pQInfo->pPriorityContainer[i], &nodeHandle);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            continue;
        if(rc != CL_OK)
        {
            clLogError("MSG", "RCV", "Failed to get the first node from the message queue of priority %d. error code [0x%x].", i, rc);
            continue;
        }

        rc = clCntNodeUserDataGet(pQInfo->pPriorityContainer[i], nodeHandle, (ClCntDataHandleT*)&pRecvInfo);
        if(rc != CL_OK)
        {
            clLogError("MSG", "RCV", "Failed to get the user-data from the first node. error code 0x%x", rc);
            continue;
        }
        
        pRecvMessage = pRecvInfo->pMessage;

        if(pTempMessage->size != 0 && pTempMessage->size < pRecvMessage->size)
        {
            pTempMessage->size = pRecvMessage->size;
            rc = CL_MSG_RC(CL_ERR_NO_SPACE);
            clLogError("MSG", "RCV", "The buffer provided by the client is not enough to hold the received message. error code [0x%x].",rc);
            goto error_out_1;
        }
        else if(pTempMessage->size == 0)
        {
            ClUint8T *pTemp;
            pTemp = (ClUint8T*)clHeapAllocate(pRecvMessage->size);
            if(pTemp == NULL)
            {
                rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
                clLogError("MSG", "RCV", "Failed to allocate %llu bytes of memory. error code [0x%x].", pRecvMessage->size, rc);
                goto error_out_1;
            }
            /* IDL gives pTempMessage->data allocated and size would be 0. so freeing it for new allocation of different size. */
            clHeapFree(pTempMessage->data);
            pTempMessage->data = pTemp;
        }

        pTempMessage->type = pRecvMessage->type;
        pTempMessage->version = pRecvMessage->version;
        pTempMessage->size = pRecvMessage->size;
        pTempMessage->priority = pRecvMessage->priority;
        memcpy(pTempMessage->senderName, pRecvMessage->senderName, sizeof(ClNameT));
        memcpy(pTempMessage->data, pRecvMessage->data, pRecvMessage->size);

        /*
         * Just adding an inconsistent queue size report armor which shouldn't be anyway hit.
         */
        if(pQInfo->usedSize[i] >= pRecvMessage->size)
            pQInfo->usedSize[i] =  pQInfo->usedSize[i] - pRecvMessage->size;
        else
        {
            clLogWarning("QUE", "GET", "MSG queue used size [%lld] is lesser than message size [%lld] "
                         "for queue handle [%llx]", pQInfo->usedSize[i], pRecvMessage->size, queueHandle);
            pQInfo->usedSize[i] = 0;
        }

        if(pQInfo->numberOfMessages[i] > 0)
            --pQInfo->numberOfMessages[i];

        clMsgMessageFree(pRecvMessage);

        retCode = clCntNodeDelete(pQInfo->pPriorityContainer[i], nodeHandle);
        if(retCode != CL_OK)
            clLogError("MSG", "RCV", "Failed to delete a node from container. error code [0x%x].", retCode);

        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

        *pSendTime = pRecvInfo->sendTime;
        *pSenderId = pRecvInfo->replyId;

        clHeapFree(pRecvInfo);

        goto out;
    }

    /*
     * Bail out immediately if timeout is 0 and queue is empty. with timeout
     */
    if(!timeout)
    {
        rc = CL_MSG_RC(CL_ERR_TIMEOUT);
        clLogDebug("MSG", "RCV", "No messages found in the queue for [%.*s]",
                   pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value);
        goto error_out_1;
    }

    if(condVarFlag == CL_FALSE)
    {
        ClBoolT monitorDisabled = CL_FALSE;

        condVarFlag = CL_TRUE;

        clMsgTimeConvert(&tempTimeout, timeout);

        pQInfo->numThreadsBlocked++;
        if((!tempTimeout.tsSec && !tempTimeout.tsMilliSec)
           || 
           (tempTimeout.tsSec*2 >= CL_TASKPOOL_MONITOR_INTERVAL))
        {
            monitorDisabled = CL_TRUE;
            clTaskPoolMonitorDisable();
        }
        rc = clOsalCondWait(pQInfo->qCondVar, &pQInfo->qLock, tempTimeout);
        if(monitorDisabled)
        {
            monitorDisabled = CL_FALSE;
            clTaskPoolMonitorEnable();
        }
        if(pQInfo->numThreadsBlocked == 0)
        {
            rc = CL_MSG_RC(CL_ERR_INTERRUPT);
            clLogInfo("MSG", "RCV", "[%s] is unblocked/cancled by the application. error code [0x%x].",__FUNCTION__, rc);
            goto error_out_1;
        }
        pQInfo->numThreadsBlocked--;

        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)            
        {
            clLogInfo("MSG", "RCV", "Message get timed-out, while waiting for message in [%.*s]. timeout [%lld ns]. error code [0x%x].", 
                    pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, timeout, rc);
            goto error_out_1;
        }
        if(rc != CL_OK)
        {
            clLogError("MSG", "RCV", "Failed at Cond Wait for getting a message. error code [0x%x].", rc);
            goto error_out_1;
        }
        goto get_message;
    }

    rc = CL_MSG_RC(CL_ERR_NOT_EXIST);
    
error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
out:
    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("MSG", "RCV", "Failed to checkin the queue handle. error code [0x%x].", rc);
error_out:
    return rc; 
}

/***********************************************************************************************************/

ClRcT clMsgQueueGetMessagesAndMove(
                                   ClMsgQueueInfoT *pQInfo,
                                   ClIocNodeAddressT destNodeAddr
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
                    clLogError("MSG", "RTV", "Failed to checkin replier handle. error code [0x%x].", rc);
                rc = clHandleDestroy(gMsgReplyDetailsDb, pRecvInfo->replyId);
                if(rc != CL_OK)
                    clLogError("MSG", "RTV", "Failed to destroy replier handle. error code [0x%x].", rc);
            }

            rc = clMsgSendMessage_idl(CL_MSG_SEND, destNodeAddr, &pQInfo->pQHashEntry->qName, pRecvInfo->pMessage,
                                      pRecvInfo->sendTime, senderInfo.senderHandle, 0, senderInfo.timeout); 
            if(rc != CL_OK)
            {
                clLogError("MSG", "RTV", "Failed to move message of queue [%.*s]. error code [0x%x].", 
                           pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
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
