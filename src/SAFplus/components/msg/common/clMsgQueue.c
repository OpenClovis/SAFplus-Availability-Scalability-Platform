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
 * File        : clMsgQueue.c
 *******************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clMsgQueue.h>
#include <clMsgCommon.h>
#include <clMsgDebugInternal.h>
#include <clIdlApi.h>
#include <clRmdIpi.h>
#include <clMsgReceiver.h>
#include <clMsgIdl.h>

#include <msgCltSrvClientCallsFromClientToClientServerClient.h>

ClHandleDatabaseHandleT gClMsgQDatabase;
ClOsalMutexT gClLocalQsLock;
ClOsalMutexT gClQueueDbLock;

ClRcT clMsgQueueInitialize(void)
{
    ClRcT rc, retCode;

    rc = clHandleDatabaseCreate(NULL, &gClMsgQDatabase);
    if(rc != CL_OK)
    {
        clLogError("QUE", "INI", "Failed to create a Queue Handle database. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clOsalMutexInit(&gClLocalQsLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "INI", "Failed to initialize the \"all local queues' lock\". error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clOsalMutexInit(&gClQueueDbLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "INI", "Failed to initialize queue db mutex. error code [0x%x].", rc);
        goto error_out_2;
    }

    goto out;

error_out_2:
    retCode = clOsalMutexDestroy(&gClLocalQsLock);
    if(retCode != CL_OK)
        clLogError("QUE", "INI", "Failed to destroy the local queues' mutex. error code [0x%x].", retCode);
error_out_1:
    retCode = clHandleDatabaseDestroy(gClMsgQDatabase);
    if(retCode != CL_OK)
        clLogError("QUE", "INI", "Failed to delete the Queue Handle database. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}


ClRcT clMsgQueueFinalize(void)
{
    ClRcT rc;
    
    rc = clOsalMutexDestroy(&gClQueueDbLock);
    if(rc != CL_OK)
        clLogError("QUE", "FIN", "Failed to destroy queue db mutex. error code [0x%x].", rc);
    
    rc = clOsalMutexDestroy(&gClLocalQsLock);
    if(rc != CL_OK)
        clLogError("QUE", "FIN", "Failed to destroy the local queues' mutex. error code [0x%x].", rc);

    rc = clHandleDatabaseDestroy(gClMsgQDatabase);
    if(rc != CL_OK)
        clLogError("QUE", "FIN", "Failed to delete the Queue Handle database. error code [0x%x].", rc);
    
    return rc;
}

static ClRcT clMsgUnblockThreadsOfQueue(ClMsgQueueInfoT *pQInfo)
{
    ClRcT rc;

    if(pQInfo->numThreadsBlocked != 0)
    {
        pQInfo->numThreadsBlocked = 0;
        rc = clOsalCondBroadcast(pQInfo->qCondVar);
    }
    else
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
    }

    return rc;
}

void clMsgQueueEmpty(ClMsgQueueInfoT *pQInfo)
{
    ClRcT rc;
    ClCntNodeHandleT nodeHandle = NULL, nextNodeHandle = NULL;
    ClUint32T i;
    ClRcT retCode;

    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; i++)
    {
        rc = clCntFirstNodeGet(pQInfo->pPriorityContainer[i], &nodeHandle);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            continue;

        do {
            rc = clCntNextNodeGet(pQInfo->pPriorityContainer[i], nodeHandle, &nextNodeHandle);
            if(CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST && rc != CL_OK)
            {
                clLogError("QUE", "EMT", "Failed to get the next node from container. error code [0x%x].", rc);
                break;
            }

            retCode = clCntNodeDelete(pQInfo->pPriorityContainer[i], nodeHandle);
            if(retCode != CL_OK)
                clLogError("QUE", "EMT", "Failed to delete a node from container. error code [0x%x].", retCode);

            nodeHandle = nextNodeHandle;
        }while(CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST);
    }
}


void clMsgQueueFree(ClMsgQueueInfoT *pQInfo)
{
    ClRcT rc = CL_OK;
    SaNameT qName = {strlen("--Unknow--"), "--Unknow--"};
    ClUint32T i;

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    (void)clMsgUnblockThreadsOfQueue(pQInfo);

    if(pQInfo->unlinkFlag == CL_FALSE && pQInfo->pQueueEntry != NULL)
    {
        saNameCopy(&qName, &pQInfo->pQueueEntry->qName);
    }
    
    if(pQInfo->timerHandle != 0)
    {
        rc = clTimerDelete(&pQInfo->timerHandle);
        if(rc != CL_OK)
            clLogError("QUE", "FREE", "Failed to delete [%.*s]'s timer handle. error code [0x%x].", 
                       qName.length, qName.value, rc);
    }
    
    clMsgQueueEmpty(pQInfo);

    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES ; i++)
    {
        rc = clCntDelete(pQInfo->pPriorityContainer[i]);
        if(rc != CL_OK)
            clLogError("QUE", "FREE", "Failed to delete the [%.*s]'s [%d] priority container. error code [0x%x].", 
                       qName.length, qName.value, i, rc);
        else
            pQInfo->pPriorityContainer[i] = 0;
    }

    rc = clOsalCondDelete(pQInfo->qCondVar);
    if(rc != CL_OK)
        clLogError("QUE",  "FREE", "Failed to delete the [%.*s]'s condtional variable. error code [0x%x].",
                   qName.length, qName.value, rc);

    pQInfo->qCondVar = 0;

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    rc = clOsalMutexDestroy(&pQInfo->qLock);
    if(rc != CL_OK)
        clLogError("QUE",  "FREE", "Failed to delete the [%.*s]'s lock. error code [0x%x].",
                   qName.length, qName.value, rc);

    clLogDebug("QUE", "FREE", "Freed the queue [%.*s].", qName.length, qName.value);

    return;
}


ClRcT clMsgQueueFreeByHandle(SaMsgQueueHandleT qHandle)
{
    ClRcT rc;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (ClPtrT *)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "FREE", "Failed at checkout the passed queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    clMsgQueueFree(pQInfo);
    clLogDebug("QUE", "FREE", "Queue is freed through its handle."); 

    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    rc = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "FREE", "Failed to checkin a queue handle. error code [0x%x].", rc);

    rc = clHandleDestroy(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "FREE", "Failed to destroy a queue handle. error code [0x%x].", rc);

error_out:
    return rc;

}

ClRcT clMsgQueueOpen(SaMsgQueueHandleT qHandle,
        SaMsgQueueOpenFlagsT openFlags)
{
    ClRcT rc = CL_OK, retCode;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "OPN", "Failed to checkout a queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    if(pQInfo->timerHandle)
        clTimerDelete(&pQInfo->timerHandle);

    if(openFlags & SA_MSG_QUEUE_EMPTY)
        clMsgQueueEmpty(pQInfo);

    pQInfo->openFlags = openFlags;
    pQInfo->state = CL_MSG_QUEUE_OPEN;

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "OPN", "Failed to check-in queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}

ClRcT clMsgQueueStatusGet(SaMsgQueueHandleT qHandle,
        SaMsgQueueStatusT *pQueueStatus)
{
    ClRcT rc, retCode;
    ClMsgQueueInfoT *pQInfo;
    ClUint32T i = 0;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "STAT", "Failed to checkout the queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    pQueueStatus->creationFlags = pQInfo->creationFlags;
    pQueueStatus->retentionTime = pQInfo->retentionTime;
    pQueueStatus->closeTime     = (clOsalStopWatchTimeGet() - pQInfo->closeTime) * 1000 ;

    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; ++i)
    {
        pQueueStatus->saMsgQueueUsage[i].queueSize = pQInfo->size[i];
        pQueueStatus->saMsgQueueUsage[i].queueUsed = pQInfo->usedSize[i];
        pQueueStatus->saMsgQueueUsage[i].numberOfMessages = pQInfo->numberOfMessages[i];
    }

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "STAT", "Failed to check-in the queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueStatusGet, 4, 0, 0)(
        SaNameT *pQName,
        SaMsgQueueStatusT *pQueueStatus
        )
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pQEntry;
    SaMsgQueueHandleT qHandle;

    CL_MSG_INIT_CHECK(rc);
    if( rc != CL_OK)
    {
       goto error_out;
    }
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "STAT", "Queue [%.*s] does not exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clMsgQueueStatusGet(qHandle, pQueueStatus);
    if(rc != CL_OK)
        clLogTrace("QUE", "STAT", "Failed to get the status of the queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);

error_out:
    return rc;
}

ClRcT clMsgQueueRetentionTimeSet(SaMsgQueueHandleT qHandle, SaTimeT *pRetenTime)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "RET", "Failed to checkout the queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_HANDLE);
        clLogError("QUE", "RET", "Queue [%.*s] is in closed state. Cannot change retention time. error code [0x%x].", 
                pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, rc);
        goto error_out_1;
    }

    if(pQInfo->creationFlags != 0)
    {
        rc = CL_MSG_RC(CL_ERR_BAD_OPERATION);
        clLogError("QUE", "RET", "Retention time can be set for only for non-persistent queues. error code [0x%x].", rc);
        goto error_out_1;
    }

    pQInfo->retentionTime = *pRetenTime;

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "RET", "Failed to checkin queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}


ClRcT clMsgMessageCancel(SaMsgQueueHandleT qHandle)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("MSG", "CANL", "Failed to checkout the passed handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    rc = clMsgUnblockThreadsOfQueue(pQInfo);

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CANL", "Failed to checkin queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueInfoGet, 4, 0, 0)(
        SaNameT *pQName, 
        SaMsgQueueCreationAttributesT *pQNewAttrs)
{
    ClRcT rc, retCode;
    ClBoolT qExists;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT qHandle = 0;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    qExists = clMsgQNameEntryExists(pQName, &pQEntry);
    if(qExists == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "MOV", "Queue [%.*s] is already moved or deleted. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "MOV", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    pQNewAttrs->creationFlags = pQInfo->creationFlags;
    memcpy(pQNewAttrs->size, pQInfo->size, sizeof(pQInfo->size));
    pQNewAttrs->retentionTime = pQInfo->retentionTime;

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    retCode = clHandleCheckin(gClMsgQDatabase, pQEntry->qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "MOV", "Failed to check-in [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, retCode);
error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueMoveMessages, 4, 0, 0)(
        SaNameT *pQName,
        SaMsgQueueOpenFlagsT openFlags,
        ClBoolT qDelete
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClBoolT qExists;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT qHandle = 0;
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogCritical("QUE", "MOV", "Failed to get the RMD originator's address. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    qExists = clMsgQNameEntryExists(pQName, &pQEntry);
    if(qExists == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("QUE", "MOV", "Queue [%.*s] is already moved or deleted. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        clLogError("QUE", "MOV", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    if(pQInfo->timerHandle != 0)
        clTimerDelete(&pQInfo->timerHandle);

    /* Here actually sending all the messages to the queue on new node. */
    rc = clMsgQueueGetMessagesAndMove(pQInfo, srcAddr);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        clLogError("QUE", "MOV", "Failed to send [%.*s]'s messages. error code [0x%x].", pQName->length, pQName->value, rc);
        qDelete = CL_FALSE;
        goto error_out_1;
    }

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

error_out_1:
    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "MOV", "Failed to check-in queue handle. error code [0x%x].", retCode);

    /* Done with moving the messages. Now freeing the queue.*/
    if (qDelete)
    {
        clMsgQueueFree(pQInfo);
        clLogDebug("QUE", "MOV", "Queue [%.*s] and its messages are moved and then freed.",
                pQName->length, pQName->value);

        retCode = clHandleDestroy(gClMsgQDatabase, qHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "CLOS", "Failed to destroy the queue handle. error code [0x%x].", retCode);

        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

        clMsgQEntryDel((SaNameT *)pQName);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    }
    else
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    }

error_out:
    return rc;
}

ClRcT clMsgToDestQueueMove(ClIocNodeAddressT destNode, SaNameT *pQName)
{
    ClRcT rc;
    SaMsgQueueCreationAttributesT tempQAttrs;
    ClBoolT qExists;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT qHandle = 0;
    SaMsgQueueHandleT qTempHandle = 0;
    ClIocPhysicalAddressT destComp;
    destComp.nodeAddress = destNode;
    destComp.portId = CL_IOC_MSG_PORT;

    qExists = clMsgQNameEntryExists(pQName, &pQEntry);
    if(qExists == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("QUE", "CLOS", "Queue [%.*s] is already moved or deleted. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        clLogError("QUE", "CLOS", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    if(pQInfo->timerHandle)
        clTimerDelete(&pQInfo->timerHandle);

    tempQAttrs.creationFlags = pQInfo->creationFlags;
    memcpy(tempQAttrs.size, pQInfo->size, sizeof(pQInfo->size));
    tempQAttrs.retentionTime = pQInfo->retentionTime;

    rc = clMsgQueueAllocateThroughIdl(destComp, pQName, /* openFlags unused */ 0, &tempQAttrs, &qTempHandle);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        clLogError("QUE", "CLOS", "Failed to move the queue [%.*s] to node [0x%x]. error code [0x%x].", 
                pQName->length, pQName->value, destNode, rc);
        goto error_out_1;
    }

    rc = clMsgQueueGetMessagesAndMove(pQInfo, destComp);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        clLogError("QUE", "CLOS", "Failed to move messages of queue [%.*s] to node [0x%x]. error code [0x%x].",
                pQName->length, pQName->value, destNode, rc);
        goto error_out_1;
    }

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

error_out_1:
    rc = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "CLOS", "Failed to check-in queue handle. error code [0x%x].", rc);

    /* Done with moving the messages. Now freeing the queue.*/
    clMsgQueueFree(pQInfo);
    clLogDebug("QUE", "CLOS", "Queue [%.*s] and its messages are moved and then freed.",
                  pQName->length, pQName->value);

    rc = clHandleDestroy(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "CLOS", "Failed to destroy the queue handle. error code [0x%x].", rc);

    clMsgQEntryDel((SaNameT *)pQName);
error_out:
    return rc;
}

ClRcT clMsgToLocalQueueMove(ClIocPhysicalAddressT srcAddr, SaNameT * pQName, ClBoolT qDelete)
{
    ClRcT rc = CL_OK;
    SaMsgQueueHandleT qHandle;
    SaMsgQueueCreationAttributesT qAttrs;
    ClIdlHandleObjT idlObj = {0};
    ClIdlHandleT idlHandle = 0;

    memcpy(&idlObj, &gIdlUcastObj, sizeof(idlObj));
    idlObj.address.address.iocAddress.iocPhyAddress = srcAddr;

    rc = clIdlHandleInitialize(&idlObj, &idlHandle);
    if(rc != CL_OK)
    {
        clLogError("MSG", "MOVE", "Failed to initialize the IDL handle. error code [0x%x].", rc);
        goto error_out;
    }

    /* Copy creation attributes from remote queue */
    rc = VDECL_VER(clMsgQueueInfoGetClientSync, 4, 0, 0)(idlHandle, (SaNameT *) pQName, &qAttrs);
    if(rc != CL_OK)
    {
        clLogError("MSG", "MOVE", "Failed to get queue [%.*s]'s information. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out1;
    }

    /* Allocate a new msg queue */
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clMsgQueueAllocate((SaNameT *)pQName, /* openFlags unused */ 0, &qAttrs, &qHandle);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    if(rc != CL_OK)
    {
        clLogError("MSG", "MOVE", "Failed to allocate queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out1;
    }

    rc = VDECL_VER(clMsgQueueMoveMessagesClientSync, 4, 0, 0)(idlHandle, (SaNameT *) pQName, 0, qDelete);
    if(rc != CL_OK)
    {
        clLogError("MSG", "MOVE", "Failed to move messages from the remote queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out2;
    }

    clIdlHandleFinalize(idlHandle);
    goto out;

error_out2:
    clMsgQueueFreeByHandle(qHandle);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    clMsgQEntryDel((SaNameT *)pQName);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
error_out1:
    clIdlHandleFinalize(idlHandle);
error_out:
out:
    return rc;
}

static ClInt32T clMsgMessageKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return 1;
}

static void clMsgDeleteCallbackFunc(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    return;
}

ClRcT clMsgQueueAllocate(
        SaNameT *pQName, 
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueHandleT *pQueueHandle)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT queueHandle;
    ClMsgQueueRecordT *pQHashEntry;
    ClUint32T j;

    rc = clHandleCreate(gClMsgQDatabase, sizeof(ClMsgQueueInfoT), &queueHandle);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to create a queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to checkout a queue handle. error code [0x%x].", rc);
        goto error_out_1;
    }

    memset(pQInfo, 0, sizeof(ClMsgQueueInfoT));
    pQInfo->state = CL_MSG_QUEUE_CREATED;

    if(clMsgQNameEntryExists(pQName, &pQHashEntry) == CL_FALSE)
    {
        rc = clMsgQEntryAdd(pQName, queueHandle, &pQHashEntry);
        if(rc != CL_OK)
        {
            clLogError("QUE", "ALOC", "Failed to inform the new queue-name entry to all message servers. error code [0x%x].", rc);
            goto error_out_2;
        }
    }
    else
    {
        pQHashEntry->qHandle = queueHandle;
    }

    rc = clOsalMutexInit(&pQInfo->qLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to initialize the [%.*s]'s mutex. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_3;
    }

    rc = clOsalCondCreate(&pQInfo->qCondVar);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to create [%.*s]'s conditional variable. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_4;
    }

    for(j = 0 ; j < CL_MSG_QUEUE_PRIORITIES ; j++)
    {
        rc = clCntLlistCreate(clMsgMessageKeyCompare, clMsgDeleteCallbackFunc, clMsgDeleteCallbackFunc, CL_CNT_NON_UNIQUE_KEY, &pQInfo->pPriorityContainer[j]);
        if(rc != CL_OK) {
            clLogError("QUE", "ALOC", "Failed to create the priority message queues for [%.*s]. error code [0x%x]",
                    pQName->length, pQName->value, rc);
            goto error_out_5;
        }
    }
    
    pQInfo->creationFlags = pCreationAttributes->creationFlags;
    memcpy(pQInfo->size, pCreationAttributes->size, sizeof(pQInfo->size));
    pQInfo->retentionTime = pCreationAttributes->retentionTime;
    pQInfo->pQueueEntry = pQHashEntry;
    pQInfo->numThreadsBlocked = 0;

    rc = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to checkin the change for [%.*s]. error code [0x%x.", pQName->length, pQName->value, rc);
        goto error_out_6;
    }

    *pQueueHandle = queueHandle;

    goto out;

error_out_6:
    if(pQInfo->timerHandle != 0)
    {
        retCode = clTimerDelete(&pQInfo->timerHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "ALOC", "Failed to delete the [%.*s]'s timer. error code [0x%x].", pQName->length, pQName->value, retCode);
    }
    
error_out_5:
    for( ; j > 0; j--)
         clCntDelete(pQInfo->pPriorityContainer[j-1]);

    retCode = clOsalCondDelete(pQInfo->qCondVar);
    if(retCode != CL_OK)
        clLogError("QUE", "ALOC", "Failed to delete [%.*s]'s conditional variable. error code [0x%x].", pQName->length, pQName->value, retCode);

error_out_4:
    retCode = clOsalMutexDestroy(&pQInfo->qLock);
    if(retCode != CL_OK)
        clLogError("QUE", "ALOC", "Failed to delete [%.*s]'s mutex lock. error code [0x%x].", pQName->length, pQName->value, retCode);
error_out_3:
    pQHashEntry->qHandle = 0;
error_out_2:
    pQInfo->state = CL_MSG_QUEUE_CLOSED;

    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "ALOC", "Failed to checkin [%.*s]'s queue handle. error code [0x%x].", pQName->length, pQName->value, retCode);

error_out_1:
    retCode = clHandleDestroy(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "ALOC", "Failed to destroy [%.*s]'s queue handle. error code [0x%x].", pQName->length, pQName->value, retCode);
error_out:
out:
    return rc;
}

ClRcT clMsgQueueDestroy(SaNameT * pQName)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueRecordT *pQEntry;
    SaMsgQueueHandleT qHandle;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "UNL", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    qHandle = pQEntry->qHandle;
    
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        clLogError("QUE", "UNL", "Failed to checkout queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    clMsgQueueFree(pQInfo);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    clMsgQEntryDel(pQName);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "UNL", "Failed to checkin the queue handle. error code [0x%x].", retCode);

    retCode = clHandleDestroy(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "UNL", "Failed to destroy the queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}


/**************************** Queue Entry IPIs ****************************/

#define CL_MSG_QNAME_BUCKET_BITS  (6)
#define CL_MSG_QNAME_BUCKETS      (1 << CL_MSG_QNAME_BUCKET_BITS)
#define CL_MSG_QNAME_MASK         (CL_MSG_QNAME_BUCKETS - 1)

static struct hashStruct *ppMsgQNameHashTable[CL_MSG_QNAME_BUCKETS];


static __inline__ ClUint32T clMsgQNameHash(const SaNameT *pQName)
{
    return (ClUint32T)((ClUint32T)pQName->value[0] & CL_MSG_QNAME_MASK);
}

static ClRcT clMsgQNameEntryAdd(ClMsgQueueRecordT *pHashEntry)
{
    ClUint32T key = clMsgQNameHash(&pHashEntry->qName);
    return hashAdd(ppMsgQNameHashTable, key, &pHashEntry->qNameHash);
}

static __inline__ void clMsgQNameEntryDel(ClMsgQueueRecordT *pHashEntry)
{
    hashDel(&pHashEntry->qNameHash);
}

ClBoolT clMsgQNameEntryExists(const SaNameT *pQName, ClMsgQueueRecordT **ppQNameEntry)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgQNameHash(pQName);

    for(pTemp = ppMsgQNameHashTable[key];  pTemp; pTemp = pTemp->pNext)
    {
        ClMsgQueueRecordT *pMsgQEntry = hashEntry(pTemp, ClMsgQueueRecordT, qNameHash);
        if(pQName->length == pMsgQEntry->qName.length && memcmp(pMsgQEntry->qName.value, pQName->value, pQName->length) == 0)
        {
            if(ppQNameEntry != NULL)
                *ppQNameEntry = pMsgQEntry;
            return CL_TRUE;
        }
    }
    return CL_FALSE;
}
/****************************************************************************/

ClRcT clMsgQEntryAdd(SaNameT *pName, SaMsgQueueHandleT queueHandle, ClMsgQueueRecordT **ppMsgQEntry)
{
    ClRcT rc;
    ClMsgQueueRecordT *pTemp;

    if(clMsgQNameEntryExists(pName, NULL) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogError("QUE", "ADD", "A queue with name [%.*s] already exists. error code [0x%x].",pName->length, pName->value, rc);
        goto error_out;
    }

    pTemp = (ClMsgQueueRecordT *) clHeapAllocate(sizeof(ClMsgQueueRecordT));
    if(pTemp == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QUE", "ADD", "Failed to allocate %zd bytes of memory. error code [0x%x].", 
                sizeof(ClMsgQueueRecordT), rc);
        goto error_out;
    }

    memcpy(&pTemp->qName, pName, sizeof(SaNameT));
    pTemp->qHandle = queueHandle;

    rc = clMsgQNameEntryAdd(pTemp);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ADD", "Failed to add the Queue Entry. error code [0x%x].", rc);
        goto error_out_1;
    }

    if(ppMsgQEntry != NULL)
        *ppMsgQEntry = pTemp;

    goto out;

error_out_1:
    clHeapFree(pTemp);
error_out:
out:
    return rc;
}


void clMsgQEntryDel(SaNameT *pQName)
{
    ClMsgQueueRecordT *pTemp;

    if(clMsgQNameEntryExists(pQName, &pTemp) == CL_FALSE)
    {
        clLogMultiline(CL_LOG_SEV_ERROR, "QUE", "DEL", "Queue [%.*s] does not exist.\n"
                "But returning [Success] as the out come of the operation is going to be the same.",
                pQName->length, pQName->value);
        goto out;
    }
    
    clMsgQNameEntryDel(pTemp);

    clHeapFree(pTemp);

out:
    return;
}

