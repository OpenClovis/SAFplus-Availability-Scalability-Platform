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
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clTimerApi.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clIdlApi.h>
#include <clDispatchApi.h>
#include <clVersion.h>
#include <clRmdIpi.h>
#include <clMsgQueue.h>
#include <clMsgCommon.h>
#include <clMsgDatabase.h>
#include <clMsgEo.h>
#include <clMsgIdl.h>
#include <clMsgDebugInternal.h>
#include <clMsgReceiver.h>
#include <clMsgFailover.h>


ClHandleDatabaseHandleT gClMsgQDatabase;
ClOsalMutexT gClLocalQsLock;


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
        clLogError("QUE", "INI", "Failed to intialize queue db mutex. error code [0x%x].", rc);
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


static ClInt32T clMsgMessageKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return 1;
}

 
static void clMsgDeleteCallbackFunc(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    return;
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

    
static void clMsgQueueEmpty(ClMsgQueueInfoT *pQInfo)
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


void clMsgQueueFree(ClBoolT qMoved, ClMsgQueueInfoT *pQInfo)
{
    ClRcT rc = CL_OK;
    ClNameT qName = {strlen("--Unknow--"), "--Unknow--"};
    ClUint32T i;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock); 
    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    (void)clMsgUnblockThreadsOfQueue(pQInfo);

    if(pQInfo->unlinkFlag == CL_FALSE && pQInfo->pQHashEntry != NULL)
    {
        memcpy(&qName, &pQInfo->pQHashEntry->qName, sizeof(ClNameT));
        if(qMoved == CL_FALSE)
        {
            CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
            CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
            rc = msgQueueInfoUpdateSend_SyncUnicast(&qName, CL_MSG_DATA_DEL, 0, NULL, NULL, NULL); 
            CL_OSAL_MUTEX_LOCK(&gClQueueDbLock); 
            CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
            if(rc !=  CL_OK)
                clLogError("QUE", "FREE", "Failed to send the [%.*s] delete update. error code [0x%x].", 
                           qName.length, qName.value, rc);
        }
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

    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock); 

    clLogDebug("QUE", "FREE", "Freed the queue [%.*s].", qName.length, qName.value);

    return;
}


static ClRcT clMsgQueueFreeByHandle(ClBoolT qMoved, SaMsgQueueHandleT queueHandle)
{
    ClRcT rc;
    ClMsgQueueInfoT *pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (ClPtrT *)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "FREE", "Failed at checkout the passed queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    clMsgQueueFree(qMoved, pQInfo);
    clLogDebug("QUE", "FREE", "Queue is freed through its handle."); 

    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    rc = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(rc != CL_OK)
        clLogError("QUE", "FREE", "Failed to checkin a queue handle. error code [0x%x].", rc);

    rc = clHandleDestroy(gClMsgQDatabase, queueHandle);
    if(rc != CL_OK)
        clLogError("QUE", "FREE", "Failed to destroy a queue handle. error code [0x%x].", rc);

error_out:
    return rc;

}


static ClRcT clMsgQueueDeleteTimerCallback(void *pParam)
{
    ClRcT rc;
    SaMsgQueueHandleT queueHandle = *(SaMsgQueueHandleT*)pParam;

    rc = clMsgQueueFreeByHandle(CL_FALSE, queueHandle);
    if(rc != CL_OK)
        clLogError("QUE", "TIMcb", "Failed to free a queue. error code [0x%x].", rc);

    return rc;
}


void clMsgFailoverClosedQMove(ClMsgQueueInfoT *pQInfo, ClIocNodeAddressT destNode)
{
    ClRcT rc;
    SaMsgQueueHandleT curHandle;
    ClIocPhysicalAddressT curOwnerAddr;
    ClIocPhysicalAddressT newOwnerAddr = {destNode, CL_IOC_MSG_PORT};
    SaMsgQueueCreationAttributesT tempQAttrs;
    SaMsgQueueHandleT qHandle;

    tempQAttrs.creationFlags = pQInfo->creationFlags;
    memcpy(tempQAttrs.size, pQInfo->size, sizeof(pQInfo->size));
    tempQAttrs.retentionTime = pQInfo->retentionTime;

    curHandle = pQInfo->pQHashEntry->qHandle;
    curOwnerAddr = pQInfo->pQHashEntry->compAddr;

    msgQueueInfoUpdateSend_internal(&pQInfo->pQHashEntry->qName, CL_MSG_DATA_UPD, 0, NULL, &newOwnerAddr, NULL); 

    /* NOTE : "newOwnerAddr" parameter is not needed for clMsgQueueAllocateThroughIdl(). But IDL will crash on passing NULL.
     * so passing "newOwnerAddr" just like that. It is not going to be used in the destination node of this RMD.
     * Similarly the "qHandle" is also not used. */
    rc = clMsgQueueAllocateThroughIdl(destNode, CL_FALSE, &pQInfo->pQHashEntry->qName, &newOwnerAddr, /* openFlags unused */ 0, &tempQAttrs, &qHandle);
    if(rc != CL_OK)
    {
        clLogError("QUE", "CLOS", "Failed to move the queue [%.*s] to node [0x%x]. error code [0x%x].", 
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, destNode, rc);
        goto error_out;
    }

    rc = clMsgQueueGetMessagesAndMove(pQInfo, destNode);
    if(rc != CL_OK)
    {
        clLogError("QUE", "CLOS", "Failed to move messages of queue [%.*s] to node [0x%x]. error code [0x%x].",
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, destNode, rc);
        if(rc == CL_GET_ERROR_CODE(CL_IOC_ERR_HOST_UNREACHABLE) 
           || 
           rc == CL_GET_ERROR_CODE(CL_IOC_ERR_COMP_UNREACHABLE))
        {
            rc = msgQueueInfoUpdateSend_internal(&pQInfo->pQHashEntry->qName, CL_MSG_DATA_UPD, curHandle, &curOwnerAddr, NULL, NULL);
            if(rc != CL_OK)
            {
                clLogError("QUE", "CLOS", "Failed to update the queue [%.*s]'s new address. error code [0x%x].", 
                        pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
            }
            else
            {
                clLogDebug("QUE", "CLOS", "Retaining queue [%.*s] with this node. i.e.[0x%x]. "
                        "Few messages would be lost while trying to move the queue.", 
                        pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, gClMyAspAddress);
            }
            goto error_out;
        }
    }

    clMsgFailoverQMovedInfoUpdateThroughIdl(destNode, &pQInfo->pQHashEntry->qName);

error_out:
    return;
}


ClRcT VDECL_VER(clMsgClientQueueClose, 4, 0, 0)(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc, retCode;
    ClMsgQueueInfoT *pQInfo;
    ClBoolT qDeleteFlag = CL_FALSE;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gFinBlockMutex);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (ClPtrT *)&pQInfo);
    if(rc != CL_OK)
    {
        clLogError("QUE", "CLOS", "Failed at checkout the passed queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock); 
    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_HANDLE);
        clLogError("QUE", "CLOS", "The Queue is already closed. error code [0x%x].",rc);
        goto error_out_1;
    }

    pQInfo->state = CL_MSG_QUEUE_CLOSED;
    pQInfo->closeTime = clOsalStopWatchTimeGet();

    clListDel(&pQInfo->pQInfoAtClt->list);
    clHeapFree(pQInfo->pQInfoAtClt);
    pQInfo->pQInfoAtClt = NULL;
    pQInfo->pOwner = NULL;

    clLogDebug("QUE", "CLOS", "Closed the queue [%.*s].", pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value);

    if(pQInfo->unlinkFlag == CL_TRUE || (pQInfo->creationFlags == 0 && pQInfo->retentionTime == 0))
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock); 

        clMsgQueueFree(CL_FALSE, pQInfo);
        clLogDebug("QUE", "CLOS", "Unlinked/zero-retention-time queue is freed.");
        qDeleteFlag = CL_TRUE;
        goto qHdl_chkin; 
    }
    else if(gMsgMoveStatus == MSG_MOVE_FIN_BLOCKED)
    {
        ClNameT qName;

        memcpy(&qName, &pQInfo->pQHashEntry->qName, sizeof(ClNameT));
        clMsgFailoverClosedQMove(pQInfo, gQMoveDestNode);

        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock); 

        clMsgQueueFree(CL_TRUE, pQInfo);
        clLogDebug("QUE", "CLOS", "Queue [%.*s] is closed, moved and freed.", qName.length, qName.value);
        qDeleteFlag = CL_TRUE;
        rc = clMsgFinBlockStatusSet(MSG_MOVE_DONE);
        if(rc != CL_OK)
            clLogError("QUE", "CLOS", "Failed to inform the queue closure. error code [0x%x].", rc);

        goto qHdl_chkin; 
    }
    else if(pQInfo->creationFlags == 0 && pQInfo->retentionTime != SA_TIME_MAX)
    {
        ClTimerTimeOutT timeout;

        clMsgTimeConvert(&timeout, pQInfo->retentionTime);

        clLogDebug("QUE", "CLOS", "Starting [%.*s]'s retention timer of [%lld ns].", 
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, pQInfo->retentionTime);

        rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT, 
                clMsgQueueDeleteTimerCallback, (void *)&pQInfo->pQHashEntry->qHandle, &pQInfo->timerHandle);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to create a timer for queue [%.*s]. error code [0x%x].", 
                    pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
        }
    }

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock); 

qHdl_chkin:
    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CLOS", "Failed to checkin the queue handle. error code [0x%x].", retCode);
    if(qDeleteFlag == CL_TRUE)
    {
        retCode = clHandleDestroy(gClMsgQDatabase, queueHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "CLOS", "Failed to destroy the queue handle. error code [0x%x].", retCode);
    }

error_out:
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gFinBlockMutex);

    return rc;
}


ClRcT VDECL_VER(clMsgQueueUnlink, 4, 0, 0)(ClNameT *pQName)
{
    ClRcT rc, retCode;
    ClMsgQueueRecordT *pTemp;
    SaMsgQueueHandleT queueHandle;
    ClMsgQueueInfoT *pQInfo;
    ClBoolT qDeleteFlag = CL_FALSE;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pTemp) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "UNL", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    queueHandle = pTemp->qHandle;

    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "UNL", "Failed to checkout queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);

    if(pQInfo->state == CL_MSG_QUEUE_CLOSED)
    {
        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        clMsgQueueFree(CL_FALSE, pQInfo);
        clLogDebug("QUE", "UNL", "Closed queue [%.*s] is unlinked and freed.", pQName->length, pQName->value);
        qDeleteFlag = CL_TRUE;
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    }
    else
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        /* Updation is must here. SAF says that, after unlink any process must be able to open a queue with same name. */
        rc = msgQueueInfoUpdateSend_internal(pQName, CL_MSG_DATA_DEL, 0, NULL, NULL, NULL); 
        if(rc != CL_OK)
            clLogError("QUE", "UNL", "Failed to update unlinking of queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        else
            pQInfo->unlinkFlag = CL_TRUE;

        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    }

    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "UNL", "Failed to checkin the queue handle. error code [0x%x].", retCode);
 
    if(qDeleteFlag == CL_TRUE)
    {
        retCode = clHandleDestroy(gClMsgQDatabase, queueHandle);
        if(retCode != CL_OK)
            clLogError("QUE", "UNL", "Failed to destroy the queue handle. error code [0x%x].", retCode);
    }
error_out:
    return rc;
}



ClRcT VDECL_VER(clMsgClientQueueUnlink, 4, 0, 0)(SaMsgHandleT msgHandle, ClNameT *pQName)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueRecordT *pTemp;
    ClMsgClientDetailsT *pClient;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("QUE", "UNL", "Failed to checkout client message handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pTemp) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "UNL", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }

    if(pTemp->compAddr.nodeAddress == 0)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("QUE", "UNL", "Queue [%.*s] is being moved to another node. Try unlinking later. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }
    else if(pTemp->compAddr.nodeAddress != gClMyAspAddress)
    {
        ClIocNodeAddressT node = pTemp->compAddr.nodeAddress;
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

        rc = clMsgQueueUnlinkThroughIdl(node, pQName);
 
        if(rc != CL_OK)
            clLogTrace("QUE", "UNL", "Failed to unlink queue [%.*s] on node [0x%x]. error code [0x%x].", pQName->length, pQName->value, node, rc);

        goto error_out_1;
    }
 
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = VDECL_VER(clMsgQueueUnlink, 4, 0, 0)(pQName);
    if(rc != CL_OK)
    {
        clLogTrace("QUE", "UNL", "Failed to unlink queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
    }

error_out_1:
    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "UNL", "Failed to checkin the message handle. error code [0x%x].", rc);
error_out:
    return rc;
}



ClRcT VDECL_VER(clMsgQueueAllocate, 4, 0, 0)(
        ClBoolT newQueue,
        ClNameT *pQName, 
        ClIocPhysicalAddressT *pCompAddress, 
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

    CL_MSG_SERVER_INIT_CHECK;

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

    if(newQueue == CL_TRUE)
    {
        rc = msgQueueInfoUpdateSend_SyncUnicast(pQName, CL_MSG_DATA_ADD, queueHandle, pCompAddress, NULL, &pQHashEntry);
        if(rc != CL_OK)
        {
            clLogError("QUE", "ALOC", "Failed to inform the new queue-name entry to all message servers. error code [0x%x].", rc);
            goto error_out_2;
        }
    }
    else
    {
        if(clMsgQNameEntryExists(pQName, &pQHashEntry) == CL_FALSE)
        {
            rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
            clLogError("QUE", "ALOC", "Failed to find queue entry in database. error code [0x%x].", rc);
            goto error_out_2;
        }
        pQHashEntry->qHandle = queueHandle;
    }

    rc = clOsalMutexInit(&pQInfo->qLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to initalize the [%.*s]'s mutex. error code [0x%x].", pQName->length, pQName->value, rc);
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
    pQInfo->pQHashEntry = pQHashEntry;
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
    if(newQueue == CL_TRUE)
    {
        retCode = clMsgQueueInfoUpdateSend(pQName, CL_MSG_DATA_DEL, 0, NULL, NULL, NULL); 
        if(retCode != CL_OK)
            clLogError("QUE", "ALOC", "Failed to send [%.*s] delete update. error code [0x%x].", pQName->length, pQName->value, retCode);
    }
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


ClRcT VDECL_VER(clMsgFailoverQMovedInfoUpdate, 4, 0, 0)(ClNameT *pQName)
{
    ClRcT rc, retCode;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT qHandle;
    ClIocPhysicalAddressT compAddr = {gClMyAspAddress, CL_IOC_MSG_PORT};
    ClBoolT qExists;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    qExists = clMsgQNameEntryExists(pQName, &pQEntry); 
    if(qExists == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "MOV", "Queue [%.*s] is already moved or deleted. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "MOV", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    
    pQInfo->state = CL_MSG_QUEUE_CLOSED;
    pQInfo->closeTime = clOsalStopWatchTimeGet();

    rc = msgQueueInfoUpdateSend_internal(pQName, CL_MSG_DATA_UPD, 0, &compAddr, NULL, NULL); 
    if(rc != CL_OK)
    {
        clLogError("QUE", "UPD", "Failed to update queue [%.*s]'s address [0x%x:0x%x]. error code [0x%x].", 
                pQName->length, pQName->value, compAddr.nodeAddress, compAddr.portId, rc);
        goto error_out_1;
    }

    if(pQInfo->creationFlags == 0 && pQInfo->retentionTime != SA_TIME_MAX)
    {
        ClTimerTimeOutT timeout;
        clMsgTimeConvert(&timeout, pQInfo->retentionTime);

        clLogDebug("QUE", "CLOS", "Starting [%.*s]'s retention timer of [%lld ns].", 
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, pQInfo->retentionTime);

        rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT, 
                clMsgQueueDeleteTimerCallback, (void *)&pQInfo->pQHashEntry->qHandle, &pQInfo->timerHandle);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to create a timer for queue [%.*s]. error code [0x%x].", 
                    pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
        }
    }

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CLOS", "Failed to checkin the queue handle. error code [0x%x].", retCode);
error_out:
    return rc;
}


static ClRcT clMsgQueueAttrCompare(SaMsgQueueCreationAttributesT *pQNewAttrs, ClMsgQueueInfoT *pQInfo)
{
    ClRcT rc = CL_OK;
    ClUint32T i;

    if(pQNewAttrs->creationFlags != pQInfo->creationFlags)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        clLogError("QUE", "MOV", "Passed creationFlag [%x] doesnt match the existing one [%x]. error code [0x%x]\n", 
                pQNewAttrs->creationFlags, pQInfo->creationFlags, rc);
        goto error_out;
    }

    for(i = 0; i < CL_MSG_QUEUE_PRIORITIES; i++)
    {
        if(pQNewAttrs->size[i] != pQInfo->size[i])
        {
            rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
            clLogError("QUE", "MOV", "Passed queue size [%llu] of priority-queue-[%d] doesnt match with current size [%llu]. error code [0x%x]\n", 
                    pQNewAttrs->size[i], i, pQInfo->size[i], rc);
            goto error_out;
        }
    }

error_out:
    return rc;
}



static ClRcT clMsgQueueOpen(ClMsgClientDetailsT *pClient, SaMsgQueueHandleT qHandle,
        SaMsgQueueCreationAttributesT *pCreationAttributes, SaMsgQueueOpenFlagsT openFlags)
{
    ClRcT rc = CL_OK, retCode;
    ClMsgClientQueueDetailsT *pClientQ;
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

    if(pQInfo->state == CL_MSG_QUEUE_OPEN)
    {
        rc = CL_MSG_RC(CL_ERR_INUSE);
        clLogError("QUE", "OPN", "Queue is already open. error code [0x%x].", rc);
        goto error_out_1;
    }

    if(pQInfo->timerHandle)
        clTimerDelete(&pQInfo->timerHandle);

    if((openFlags & SA_MSG_QUEUE_CREATE) && (pCreationAttributes != NULL))
    {
        rc = clMsgQueueAttrCompare(pCreationAttributes, pQInfo);
        if(rc != CL_OK)
        {
            clLogError("QUE", "OPN", "Queue [%.*s] create failed. error code [0x%x]\n", 
                    pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
            goto error_out_1;
        }
        pQInfo->retentionTime = pCreationAttributes->retentionTime;
    }

    if(openFlags & SA_MSG_QUEUE_EMPTY)
        clMsgQueueEmpty(pQInfo);

    pClientQ = (ClMsgClientQueueDetailsT *)clHeapAllocate(sizeof(ClMsgClientQueueDetailsT));
    if(pClientQ == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QUE", "OPN", "Failed to allocate %zd bytes. error code [0x%x].", sizeof(ClMsgClientQueueDetailsT), rc);
        goto error_out_1;
    }

    clListAddTail(&pClientQ->list, &pClient->qList);

    pClientQ->qHandle = qHandle;
    pQInfo->pOwner = pClient;
    pQInfo->openFlags = openFlags;
    pQInfo->pQInfoAtClt = pClientQ;

    pQInfo->state = CL_MSG_QUEUE_OPEN;

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "OPN", "Failed to checkin queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}



ClRcT VDECL_VER(clMsgQueueInfoGet, 4, 0, 0)(
        ClNameT *pQName, 
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pQNewAttrs)
{
    ClRcT rc, retCode;
    ClBoolT qExists;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    SaMsgQueueHandleT qHandle = 0;

    CL_MSG_SERVER_INIT_CHECK;

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

    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "MOV", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    if(pQInfo->state == CL_MSG_QUEUE_OPEN) 
    {
        rc = CL_MSG_RC(CL_ERR_INUSE);
        clLogError("QUE", "MOV", "Queue [%.*s] is in use. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }
   
    pQNewAttrs->creationFlags = pQInfo->creationFlags;
    memcpy(pQNewAttrs->size, pQInfo->size, sizeof(pQInfo->size));
    pQNewAttrs->retentionTime = pQInfo->retentionTime;

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    retCode = clHandleCheckin(gClMsgQDatabase, pQEntry->qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "MOV", "Failed to checkin [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, retCode);
error_out:
    return rc;
}


typedef struct {
    ClMsgQueueInfoT *pQInfo;
    ClIocPhysicalAddressT curOwnerAddr;
    ClIocPhysicalAddressT newOwnerAddr;
}ClMsgMoveThreadParamsT;


static void *clMsgMessageMoveThread(void *pParam)
{
    ClRcT rc;
    SaMsgQueueHandleT qHandle;
    ClMsgQueueInfoT *pQInfo = ((ClMsgMoveThreadParamsT*)pParam)->pQInfo;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    qHandle = pQInfo->pQHashEntry->qHandle;

    /* Here actually sending all the messages to the queue on new node. */
    rc = clMsgQueueGetMessagesAndMove(pQInfo, ((ClMsgMoveThreadParamsT*)pParam)->newOwnerAddr.nodeAddress);
    if(rc != CL_OK)
    {
        ClIocPhysicalAddressT curOwnerAddr = ((ClMsgMoveThreadParamsT*)pParam)->curOwnerAddr;

        clLogError("QUE", "MVTh", "Failed to move messages of queue [%.*s] to node [0x%x]. error code [0x%x].",
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, curOwnerAddr.nodeAddress, rc);

        if(rc == CL_GET_ERROR_CODE(CL_IOC_ERR_HOST_UNREACHABLE) 
           || 
           rc == CL_GET_ERROR_CODE(CL_IOC_ERR_COMP_UNREACHABLE))
        {
            rc = msgQueueInfoUpdateSend_internal(&pQInfo->pQHashEntry->qName, CL_MSG_DATA_UPD, qHandle, &curOwnerAddr, NULL, NULL);
            if(rc != CL_OK)
            {
                clLogError("QUE", "MVTh", "Failed to update the queue [%.*s]'s new address. error code [0x%x].", 
                        pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
            }
            else
            {
                clLogDebug("QUE", "MVTh", "Retaining queue [%.*s] with this node. i.e.[0x%x].", 
                        pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, gClMyAspAddress);
            }
            CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
            CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
            goto error_out;
        }
    }

    clMsgQueueEmpty(pQInfo);

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    /* Done with moving the messages. Now freeing the queue.*/
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    clMsgQueueFree(CL_TRUE, pQInfo);
    clLogDebug("QUE", "MVTh", "Queue [%.*s] and its messages are moved and then freed.",
            pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    rc = clHandleDestroy(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "MVTh", "Failed to destroy queue handle. error code [0x%x].", rc);

error_out:
    clHeapFree(pParam);

    return NULL;
}


ClRcT VDECL_VER(clMsgQueueMoveMessages, 4, 0, 0)(
        ClNameT *pQName,
        SaMsgQueueOpenFlagsT openFlags,
        ClIocPhysicalAddressT *pCompAddr
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClBoolT qExists;
    ClMsgQueueRecordT *pQEntry;
    ClMsgQueueInfoT *pQInfo;
    ClMsgMoveThreadParamsT *pMsgMoveParams;
    SaMsgQueueHandleT qHandle = 0;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    qExists = clMsgQNameEntryExists(pQName, &pQEntry);
    if(qExists == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("QUE", "MOV", "Queue [%.*s] is already moved or deleted. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    qHandle = pQEntry->qHandle;
    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void**)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "MOV", "Failed to checkout [%.*s]'s handle. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    if(pQEntry->compAddr.nodeAddress == 0)
    {
        rc = CL_MSG_RC(CL_ERR_INUSE);
        clLogError("MOV", "MSG", "Queue [%.*s] is already being moved. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }

    pMsgMoveParams = (ClMsgMoveThreadParamsT*)clHeapAllocate(sizeof(ClMsgMoveThreadParamsT));
    if(pMsgMoveParams == NULL)
    {
        clLogError("MOV", "MSG", "Failed to get RMD source address. error code [0x%x].", rc);
        goto error_out_1;
    }

    if(pQInfo->timerHandle != 0)
        clTimerDelete(&pQInfo->timerHandle);

    msgQueueInfoUpdateSend_internal(&pQEntry->qName, CL_MSG_DATA_UPD, 0, NULL, pCompAddr, NULL); 

    if(openFlags & SA_MSG_QUEUE_EMPTY)
        clMsgQueueEmpty(pQInfo);

    pMsgMoveParams->pQInfo = pQInfo;
    pMsgMoveParams->curOwnerAddr = pQEntry->compAddr;
    pMsgMoveParams->newOwnerAddr = *pCompAddr;

    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    clMsgMessageMoveThread(pMsgMoveParams);

    retCode = clHandleCheckin(gClMsgQDatabase, pQEntry->qHandle);
    if(retCode != CL_OK)
        clLogError("MOV", "MSG", "Failed to checkin queue handle. error code [0x%x].", retCode);

    return rc;


error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    retCode = clHandleCheckin(gClMsgQDatabase, pQEntry->qHandle);
    if(retCode != CL_OK)
        clLogError("MOV", "MSG", "Failed to checkin queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}


static ClRcT clMsgQueueMove(
        ClMsgClientDetailsT *pClient,
        ClIocNodeAddressT node, 
        ClNameT *pQName, 
        SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueHandleT *pQHandle)
{
    ClRcT rc, retCode;
    SaMsgQueueCreationAttributesT qAttrs;

    rc = clMsgQueueInfoGetThroughIdl(node, pQName, openFlags, &qAttrs);
    if(rc != CL_OK)
    {
        if(rc != CL_MSG_RC(CL_ERR_INUSE))
            clLogError("QUE", "MOV", "Failed to get queue [%.*s]'s information from node [0x%x]. error code [0x%x].",
                    pQName->length, pQName->value, node, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = VDECL_VER(clMsgQueueAllocate, 4, 0, 0)(CL_FALSE, pQName, &pClient->address, /* openFlags unused */ 0, &qAttrs, pQHandle);
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    if(rc != CL_OK)
    {
        clLogError("QUE", "MOV", "Failed to allocate queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    rc = clMsgQueueOpen(pClient, *pQHandle, pCreationAttributes, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "MOV", "Failed to initialize queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }

    rc = clMsgQueueMoveMessagesThroughIdl(node, pQName, openFlags, &pClient->address);
    if(rc != CL_OK)
    {
        clLogError("QUE", "MOV", "Failed to move messages of queue [%.*s], to node [0x%x]. error code [0x%x].", 
                pQName->length, pQName->value, node, rc);

        if(rc == CL_GET_ERROR_CODE(CL_IOC_ERR_HOST_UNREACHABLE) 
           || 
           rc == CL_GET_ERROR_CODE(CL_IOC_ERR_COMP_UNREACHABLE))
        {
            rc = clMsgQueueInfoUpdateSend(pQName, CL_MSG_DATA_UPD, 0, &pClient->address, NULL, NULL);
            if(rc != CL_OK)
            {
                clLogError("QUE", "MOV", "Failed to send queue [%.*s]'s new address. error code [0x%x].",
                        pQName->length, pQName->value, rc);
                goto error_out_1;
            }

            clLogDebug("QUE", "MOV", "Retaining queue [%.*s] with this node. i.e.[0x%x].", 
                    pQName->length, pQName->value, gClMyAspAddress);
        }
        else
        {
            goto error_out_1;
        }
    }
    else
    {
        rc = clMsgQueueInfoUpdateSend(pQName, CL_MSG_DATA_UPD, 0, &pClient->address, NULL, NULL);
        if(rc != CL_OK)
        {
            clLogError("QUE", "MOV", "Failed to send queue [%.*s]'s new address. error code [0x%x].",
                    pQName->length, pQName->value, rc);
            goto error_out_1;
        }
    }

    clLogDebug("QUE", "MOV", "Queue [%.*s] is successfully moved.", pQName->length, pQName->value);

    goto out;

error_out_1:
    retCode = clMsgQueueFreeByHandle(CL_FALSE, *pQHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "MOV", "Failed to free the queue. error code [0x%x].", retCode);
error_out:
out:
    return rc;
}



ClRcT VDECL_VER(clMsgClientQueueOpen, 4, 0, 0)(SaMsgHandleT msgHandle, 
        ClNameT *pQName, 
        SaMsgQueueCreationAttributesT *pCreationAttributes, 
        SaMsgQueueOpenFlagsT openFlags, 
        SaTimeT timeout, 
        SaMsgQueueHandleT *pQHandle)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueRecordT *pTemp;
    SaMsgQueueHandleT queueHandle = 0;
    ClBoolT qExists;
    ClMsgClientDetailsT *pClient;
    SaMsgQueueCreationAttributesT *pCrtAttrs = NULL;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to checkout the passed message handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    qExists = clMsgQNameEntryExists(pQName, &pTemp);
    if(qExists == CL_TRUE && pTemp->compAddr.nodeAddress != gClMyAspAddress)
    {
        ClIocNodeAddressT node = pTemp->compAddr.nodeAddress;
        ClIocPortT port = pTemp->compAddr.portId;

        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

        if(node == 0)
        {
            rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
            clLogError("QUE", "MOV", "Queue [%.*s] is already being moved. Please try again after some time. error code [0x%x]", 
                    pQName->length, pQName->value, rc);
            goto error_out_1;
        }

        rc = clMsgQueueMove(pClient, node, pQName, pCreationAttributes, openFlags, pQHandle);
        if(rc == CL_OK)
        {
            goto qMoved_out;
        }
        else if(rc == CL_MSG_RC(CL_ERR_INUSE))
        {
            clLogError("QUE", "OPEN", "Queue [%.*s] already exists. And it is in use by component [0x%x:0x%x]. error code [0x%x].",
                    pQName->length, pQName->value, node, port, rc);
            goto error_out_1;
        }
        else
        {
            clLogError("QUE", "OPEN", "Failed to move queue [%.*s] from node [%#x], port [%#x]. Error code [0x%x]. "
                    "Queue seems to be transitioning from the node. Try again", 
                    pQName->length, pQName->value, node, port, rc);
            rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
            goto error_out_1;
        }
    }

    if(qExists == CL_FALSE && !(openFlags & SA_MSG_QUEUE_CREATE))
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST); 
        clLogError("QUE", "OPEN", "Queue [%.*s] does not exist. Open needs [SA_MSG_QUEUE_CREATE] flag passed. error code [0x%x].",
                pQName->length, pQName->value, rc);
        goto error_out_1;
    }
    else if(qExists == CL_TRUE)
    {
        queueHandle = pTemp->qHandle;
        if(openFlags & SA_MSG_QUEUE_CREATE)
            pCrtAttrs = pCreationAttributes;
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    }
    else
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = VDECL_VER(clMsgQueueAllocate, 4, 0, 0)(CL_TRUE, pQName, &pClient->address, /* openFlags unused */ 0, pCreationAttributes, &queueHandle);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        if(rc != CL_OK)
        {
            clLogError("QUE", "OPEN", "Failed to create queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
            goto error_out_1;
        }
    }

    rc = clMsgQueueOpen(pClient, queueHandle, pCrtAttrs, openFlags);
    if(rc != CL_OK)
    {
        clLogError("QUE", "OPEN", "Failed to initialize queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out_1;
    }

    *pQHandle = queueHandle;

    clLogDebug("QUE", "OPEN", "Queue [%.*s] is successfully opened.", pQName->length, pQName->value);

error_out_1:
qMoved_out:
    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "OPEN", "Failed to checkin the message client handle. error code [0x%x].", rc);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgQueueStatusGet, 4, 0, 0)(
        const ClNameT *pQName,
        SaMsgQueueStatusT *pQueueStatus
        )
{
    ClRcT rc, retCode;
    SaMsgQueueHandleT qHandle;
    ClMsgQueueInfoT *pQInfo;
    ClMsgQueueRecordT *pQEntry;
    ClUint32T i = 0;

    CL_MSG_SERVER_INIT_CHECK;

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
        clLogError("QUE", "STAT", "Failed to checkin the queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgClientQueueStatusGet, 4, 0, 0)(
        SaMsgHandleT msgHandle,
        const ClNameT *pQName,
        SaMsgQueueStatusT *pQueueStatus
        )
{
    ClRcT rc;
    ClMsgQueueRecordT *pQEntry;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "STAT", "Queue [%.*s] does not exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    if(pQEntry->compAddr.nodeAddress == 0)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogError("QUE", "STAT", "Queue [%.*s] is being moved to another node. Try again later. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }
    else if(pQEntry->compAddr.nodeAddress != gClMyAspAddress)
    {
        ClIocNodeAddressT node = pQEntry->compAddr.nodeAddress;
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = clMsgQueueStatusGetThroughIdl(node, (ClNameT*)pQName, pQueueStatus);
        if(rc != CL_OK)
            clLogTrace("QUE", "STAT", "Failed to get status of queue [%.*s] on node [0x%x]. error code [0x%x].", pQName->length, pQName->value, node, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = VDECL_VER(clMsgQueueStatusGet, 4, 0, 0)(pQName, pQueueStatus);
    if(rc != CL_OK)
        clLogTrace("QUE", "STAT", "Failed to get the status of the queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);

error_out:
    return rc;
}



ClRcT VDECL_VER(clMsgClientQueueRetentionTimeSet, 4, 0, 0)(SaMsgQueueHandleT qHandle, SaTimeT *pRetenTime)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;

    CL_MSG_SERVER_INIT_CHECK;

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
                pQInfo->pQHashEntry->qName.length, pQInfo->pQHashEntry->qName.value, rc);
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


ClRcT VDECL_VER(clMsgClientMessageCancel, 4, 0, 0)(SaMsgQueueHandleT queueHandle)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgQueueInfoT *pQInfo;

    CL_MSG_SERVER_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    rc = clHandleCheckout(gClMsgQDatabase, queueHandle, (void**)&pQInfo);
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

    retCode = clHandleCheckin(gClMsgQDatabase, queueHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CANL", "Failed to checkin queue handle. error code [0x%x].", retCode);
error_out:
    return rc;
}
