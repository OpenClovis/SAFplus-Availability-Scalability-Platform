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
 *//*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : message
 * File        : clMsgIdlHandle.c
 *******************************************************************************/
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clMsgCommon.h>
#include <clIdlApi.h>
#include <clLogApi.h>
#include <clRmdIpi.h>
#include <clMsgQueue.h>
#include <clMsgCkptServer.h>
#include <clMsgDebugInternal.h>
#include <clMsgFailover.h>
#include <clOsalApi.h>
#include <clMsgReceiver.h>
#include <clTaskPool.h>

ClRcT VDECL_VER(clMsgQueueUnlink, 4, 0, 0)(ClNameT *pQName)
{
    ClRcT rc=CL_OK;  /* Return OK even if the message queue does not exist, because the end state is the same */
    ClMsgQueueCkptDataT queueData;
    ClBoolT isExist = clMsgQCkptExists((ClNameT *)pQName, &queueData);

    CL_MSG_INIT_CHECK;

    if(queueData.creationFlags == SA_MSG_QUEUE_PERSISTENT)
    {
        rc = clMsgQueueDestroy((ClNameT *)pQName);
        if(rc != CL_OK)
        {
            clLogError("MSG", "QUL", "Failed to destroy msg queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
            goto error_out;
        }
    }

    if (isExist)
    {
        rc = clMsgQCkptDataUpdate(CL_MSG_DATA_DEL, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            clLogError("MSG", "QUL", "Failed to update msg queue database. error code [0x%x].", rc);
            goto error_out;
        }
    }

error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueOpen, 4, 0, 0)(ClNameT *pQName, 
        SaMsgQueueCreationAttributesT *pCreationAttributes, 
        SaMsgQueueOpenFlagsT openFlags)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT srcAddr;
    ClMsgQueueCkptDataT queueData;
    ClBoolT isExist = clMsgQCkptExists((ClNameT *)pQName, &queueData);
    SaMsgQueueHandleT qHandle;

    CL_MSG_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "OPEN", "Failed to get the RMD originator's address. error code [0x%x].", rc);
        goto error_out;
    }

    if ((openFlags & SA_MSG_QUEUE_CREATE)
       && (pCreationAttributes->creationFlags == SA_MSG_QUEUE_PERSISTENT))
    {
        CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
        CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
        rc = clMsgQueueAllocate((ClNameT *)pQName, /* openFlags unused */ 0, pCreationAttributes, &qHandle);
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        if(rc != CL_OK)
        {
            clLogError("QUE", "OPEN", "Failed to allocate queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
            goto error_out;
        }
    }

    queueData.qName = *pQName;
    queueData.qAddress = srcAddr;
    queueData.state = CL_MSG_QUEUE_OPEN;
    if (openFlags & SA_MSG_QUEUE_CREATE)
    {
        queueData.creationFlags = pCreationAttributes->creationFlags;
        if (pCreationAttributes->creationFlags == SA_MSG_QUEUE_PERSISTENT)
        {
            queueData.qServerAddress.nodeAddress = gLocalAddress;
            queueData.qServerAddress.portId = CL_IOC_MSG_PORT;
        }
        else
        {
            queueData.qServerAddress.nodeAddress = 0;
        }
    }

    if (isExist)
    {
        rc = clMsgQCkptDataUpdate(CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            clLogError("MSG", "OPEN", "Failed to update msg queue database. error code [0x%x].", rc);
            goto error_out;
        }
    }
    else
    {
        rc = clMsgQCkptDataUpdate(CL_MSG_DATA_ADD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            clLogError("MSG", "OPEN", "Failed to update msg queue database. error code [0x%x].", rc);
            goto error_out;
        }
    }

error_out:
    return rc;
}

static ClRcT clMsgQueueDeleteTimerCallback(void *pParam)
{
    ClRcT rc;
    SaMsgQueueHandleT qHandle = *(SaMsgQueueHandleT*)pParam;
    ClMsgQueueInfoT *pQInfo;
    ClNameT qName;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (ClPtrT *)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "TIMcb", "Failed at checkout the passed queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    clNameCopy(&qName, &pQInfo->pQueueEntry->qName);
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    clMsgQueueFree(pQInfo);
    clLogDebug("QUE", "TIMcb", "Queue is freed through its handle."); 

    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    rc = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "TIMcb", "Failed to checkin a queue handle. error code [0x%x].", rc);

    rc = clHandleDestroy(gClMsgQDatabase, qHandle);
    if(rc != CL_OK)
        clLogError("QUE", "TIMcb", "Failed to destroy a queue handle. error code [0x%x].", rc);

    /* Remove the message queue out of the database */
    clMsgQEntryDel((ClNameT *)&qName);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    VDECL_VER(clMsgQueueUnlink, 4, 0, 0)((ClNameT *)&qName);

error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueRetentionClose, 4, 0, 0)(const ClNameT *pQName)
{
    ClRcT rc = CL_OK, retCode;
    ClMsgQueueCkptDataT queueData;
    ClTimerTimeOutT timeout;
    ClMsgQueueRecordT *pQEntry;
    SaMsgQueueHandleT qHandle;
    ClMsgQueueInfoT *pQInfo;

    CL_MSG_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gFinBlockMutex);
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    /* Look up msg queue in the cached checkpoint */
    if(clMsgQCkptExists((ClNameT *)pQName, &queueData) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_ERR_DOESNT_EXIST;
        clLogError("MSG", "CLOS", "Failed to get the message queue information. error code [0x%x].", rc);
        goto error_out;
    }

    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "CLOS", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    qHandle = pQEntry->qHandle;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clHandleCheckout(gClMsgQDatabase, qHandle, (void **)&pQInfo);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
        clLogError("QUE", "CLOS", "Failed to checkout queue handle. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pQInfo->qLock);
    if(pQInfo->retentionTime != SA_TIME_MAX)
    {
        clMsgTimeConvert(&timeout, pQInfo->retentionTime);
        rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT, 
                clMsgQueueDeleteTimerCallback, (void *)&pQInfo->pQueueEntry->qHandle, &pQInfo->timerHandle);
        if(rc != CL_OK)
        {
            clLogError("QUE", "CLOS", "Failed to create a timer for queue [%.*s]. error code [0x%x].", 
                    pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, rc);
        }
    }
    CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

    if(gMsgMoveStatus == MSG_MOVE_FIN_BLOCKED)
    {
        queueData.qAddress.nodeAddress = 0;
        queueData.qAddress.portId = CL_IOC_MSG_PORT;
        rc = clMsgQCkptDataUpdate(CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
            clLogError("MSG", "CLOS", "Failed to update msg queue database. error code [0x%x].", rc);
            goto error_out;
        }

        clMsgToDestQueueMove(gQMoveDestNode, (ClNameT *)pQName);
        clMsgFinBlockStatusSet(MSG_MOVE_DONE);

        /* Set nodeAddress with new address after moving queue */
        queueData.qAddress.nodeAddress = gQMoveDestNode;
        queueData.qAddress.portId = CL_IOC_MSG_PORT;
        queueData.state = CL_MSG_QUEUE_CLOSED;
        rc = clMsgQCkptDataUpdate(CL_MSG_DATA_UPD, &queueData, CL_TRUE);
        if(rc != CL_OK)
        {
            CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
            clLogError("MSG", "CLOS", "Failed to update msg queue database. error code [0x%x].", rc);
            goto error_out;
        }
    }

    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);

    retCode = clHandleCheckin(gClMsgQDatabase, qHandle);
    if(retCode != CL_OK)
        clLogError("QUE", "CLOS", "Failed to checkin the queue handle. error code [0x%x].", retCode);

    clLogDebug("QUE", "CLOS", "Finished [%.*s]'s retention timer of [%lld ns].", 
                pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, pQInfo->retentionTime);

error_out:
    CL_OSAL_MUTEX_UNLOCK(&gFinBlockMutex);
    return rc;
}

ClRcT VDECL_VER(clMsgQueuePersistRedundancy, 4, 0, 0)(const ClNameT *pQName, ClIocPhysicalAddressT srcAddr, ClBoolT isDelete)
{
    ClRcT rc = CL_OK;

    CL_MSG_INIT_CHECK;

    /* Look up msg queue in the cached checkpoint */
    ClMsgQueueCkptDataT queueData;
    if(clMsgQCkptExists((ClNameT *)pQName, &queueData) == CL_FALSE)
    {
        rc = CL_ERR_DOESNT_EXIST;
        clLogError("MSG", "REDUN", "Failed to get the message queue information. error code [0x%x].", rc);
        goto error_out;
    }

    queueData.qServerAddress.nodeAddress = 0;
    queueData.qServerAddress.portId = CL_IOC_MSG_PORT;
    rc = clMsgQCkptDataUpdate(CL_MSG_DATA_UPD, &queueData, CL_TRUE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REDUN", "Failed to update msg queue database. error code [0x%x].", rc);
        goto error_out;
    }

    rc = clMsgToLocalQueueMove(srcAddr, (ClNameT *) pQName, isDelete);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REDUN", "Failed to move messages from the remote queue [%.*s]. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    queueData.qServerAddress.nodeAddress = gLocalAddress;
    queueData.qServerAddress.portId = CL_IOC_MSG_PORT;
    rc = clMsgQCkptDataUpdate(CL_MSG_DATA_UPD, &queueData, CL_TRUE);
    if(rc != CL_OK)
    {
        clLogError("MSG", "REDUN", "Failed to update msg queue database. error code [0x%x].", rc);
        goto error_out;
    }

error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQDatabaseUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClMsgQueueCkptDataT *queueData, ClBoolT updateCkpt)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "UPD", "Failed to get the RMD originator's address. error code [0x%x].", rc);
        goto error_out;
    }

    /* Return if destination address equals to source address */
    if ((srcAddr.nodeAddress == gLocalAddress)
        && (srcAddr.portId == CL_IOC_MSG_PORT))
        goto out;

    rc = clMsgQCkptDataUpdate(syncupType, queueData, updateCkpt);

error_out:
out:
    return rc;
}

ClRcT VDECL_VER(clMsgGroupDatabaseUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy, ClIocPhysicalAddressT qGroupAddress,ClBoolT updateCkpt)
{
    ClRcT rc;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogError("GRP", "UDB", "Failed to get the RMD source address. error code [0x%x].", rc);
        goto error_out;
    }
    if ((srcAddr.nodeAddress == gLocalAddress)
        && (srcAddr.portId == CL_IOC_MSG_PORT))
        goto out;

    rc = clMsgQGroupCkptDataUpdate(syncupType, pGroupName, policy, qGroupAddress, updateCkpt);

error_out:
out:
    return rc;
}

ClRcT VDECL_VER(clMsgGroupMembershipUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName, ClBoolT updateCkpt)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogError("GRP", "MBR", "Failed to get the RMD source address. error code [0x%x].", rc);
        goto error_out;
    }
    if ((srcAddr.nodeAddress == gLocalAddress)
        && (srcAddr.portId == CL_IOC_MSG_PORT))
        goto out;

    rc = clMsgQGroupMembershipCkptDataUpdate(syncupType, pGroupName, pQueueName, updateCkpt);

error_out:
out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueAllocate, 4, 0, 0)(
        ClNameT *pQName, 
        SaMsgQueueOpenFlagsT openFlags,
        SaMsgQueueCreationAttributesT *pCreationAttributes,
        SaMsgQueueHandleT *pQueueHandle)
{
    return clMsgQueueAllocate(pQName, openFlags, pCreationAttributes, pQueueHandle);
}

ClRcT VDECL_VER(clMsgMessageGet, 4, 0, 0)(const ClNameT *pQName, SaTimeT timeout)
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
    SaMsgQueueHandleT queueHandle;
    ClMsgQueueRecordT *pQEntry;

    CL_MSG_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    if(clMsgQNameEntryExists(pQName, &pQEntry) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("QUE", "CLOS", "Queue [%.*s] doesn't exist. error code [0x%x].", pQName->length, pQName->value, rc);
        goto error_out;
    }

    queueHandle = pQEntry->qHandle;

    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

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
            clLogError("MSG", "GET", "Failed to get the first node from the message queue of priority %d. error code [0x%x].", i, rc);
            continue;
        }

        rc = clCntNodeUserDataGet(pQInfo->pPriorityContainer[i], nodeHandle, (ClCntDataHandleT*)&pRecvInfo);
        if(rc != CL_OK)
        {
            clLogError("MSG", "GET", "Failed to get the user-data from the first node. error code 0x%x", rc);
            continue;
        }

        pRecvMessage = pRecvInfo->pMessage;

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
            clLogError("MSG", "GET", "Failed to delete a node from container. error code [0x%x].", retCode);

        CL_OSAL_MUTEX_UNLOCK(&pQInfo->qLock);

        clHeapFree(pRecvInfo);

        goto out;
    }

    /*
     * Bail out immediately if timeout is 0 and queue is empty. with timeout
     */
    if(!timeout)
    {
        rc = CL_MSG_RC(CL_ERR_TIMEOUT);
        clLogDebug("MSG", "GET", "No messages found in the queue for [%.*s]",
                   pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value);
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
            clLogInfo("MSG", "RCV", "[%s] is unblocked/canceled by the application. error code [0x%x].",__FUNCTION__, rc);
            goto error_out_1;
        }
        pQInfo->numThreadsBlocked--;

        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
        {
            clLogInfo("MSG", "RCV", "Message get timed-out, while waiting for message in [%.*s]. timeout [%lld ns]. error code [0x%x].", 
                    pQInfo->pQueueEntry->qName.length, pQInfo->pQueueEntry->qName.value, timeout, rc);
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
        clLogError("MSG", "RCV", "Failed to checkin the queue handle. error code [0x%x].", retCode);

error_out:
    return rc;
}
