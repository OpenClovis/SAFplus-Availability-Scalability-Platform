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
 * File        : clMsgEo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This module contains all the house keeping functionality
 *          for MS - EOnization & association with CPM, debug, etc.
 *****************************************************************************/


#include <clCommon.h>
#include <clList.h>
#include <clHandleApi.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clVersion.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgEo.h>
#include <clMsgGroupDatabase.h>
#include <clMsgIdl.h>
#include <clCpmApi.h>
#include <clMsgDebugInternal.h>
#include <clMsgCkptServer.h>


ClRcT VDECL_VER(clMsgQueueGroupCreate, 4, 0, 0)(ClNameT *pGroupName, SaMsgQueueGroupPolicyT qPolicy)
{
    ClRcT rc;
    ClRcT retCode;

    CL_MSG_INIT_CHECK;

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    if(clMsgGroupEntryExists(pGroupName, NULL) == CL_TRUE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogError("GRP", "CRT", "Message Queue Group with name [%.*s] already exists. error code [0x%x].", pGroupName->length, pGroupName->value, rc);

        /* Remove MSG queue group if it does not exist in the cached checkpoint */
        ClMsgQGroupCkptDataT qGroupData;
        if(clMsgQGroupCkptExists((ClNameT*)pGroupName, &qGroupData) == CL_FALSE)
        {
            goto error_out_1;
        }

        goto error_out;
    }
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);

    rc = clMsgGroupInfoUpdate(CL_MSG_DATA_ADD, pGroupName, qPolicy);
    if(rc != CL_OK)
    {
        clLogError("GRP", "CRT", "Fail to add Message Queue Group with name [%.*s] into the database. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    ClIocPhysicalAddressT qGroupAddress;
    qGroupAddress.nodeAddress = gLocalAddress;
    qGroupAddress.portId = CL_IOC_MSG_PORT;

    /* Add group into the cached checkpoint */
    rc = clMsgQGroupCkptDataUpdate(CL_MSG_DATA_ADD, (ClNameT *)pGroupName, qPolicy, qGroupAddress, CL_TRUE);
    if (rc != CL_OK)
    {
        clLogError("GRP", "CRT", "Fail to add Message Queue Group with name [%.*s] into the cached checkpoint. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out_1;
    }

    clLogTrace("GRP", "CRT", "Created a Message Queue Group with name [%.*s].", pGroupName->length, pGroupName->value);

    goto out;
error_out_1:
    retCode = clMsgGroupInfoUpdate(CL_MSG_DATA_DEL, pGroupName, 0);
    if(retCode != CL_OK)
        clLogError("GRP", "CRT", "Fail to remove Message Queue Group with name [%.*s] from the database. error code [0x%x].", pGroupName->length, pGroupName->value, retCode);
out:
error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgQueueGroupInsert, 4, 0, 0)(
        ClNameT *pGroupName,
        ClNameT *pQName
        )
{
    ClRcT rc;
    ClRcT retCode;

    CL_MSG_INIT_CHECK;

    rc = clMsgGroupMembershipInfoSend(CL_MSG_DATA_ADD, pGroupName, pQName);
    if(rc != CL_OK)
    {
        clLogError("GRP", "QIN", "Failed to insert Message Queue [%.*s] to Message Queue Group [%.*s]. error code [0x%x]."
                     , pQName->length, pQName->value
                     , pGroupName->length, pGroupName->value
                     , rc);
        goto error_out;
    }

    rc = clMsgQGroupMembershipCkptDataUpdate(CL_MSG_DATA_ADD, pGroupName, pQName, CL_TRUE);
    if(rc != CL_OK)
    {
        clLogError("GRP", "QIN", "Failed to update the MSG queue group cached checkpoint. error code [0x%x].", rc);
        goto error_out_1;
    }

    clLogTrace("GRP", "QIN", "Inserted Message Queue [%.*s] to Message Queue Group [%.*s]."
                             , pQName->length, pQName->value
                             , pGroupName->length, pGroupName->value);

    goto out;
error_out_1:
    retCode = clMsgGroupMembershipInfoSend(CL_MSG_DATA_DEL, pGroupName, pQName);
    if(retCode != CL_OK)
        clLogError("GRP", "QIN", "Fail to remove Message Queue Group with name [%.*s] from the database. error code [0x%x].", pGroupName->length, pGroupName->value, retCode);
out:
error_out:
    return rc;
}

ClRcT VDECL_VER(clMsgQueueGroupRemove, 4, 0, 0)(
        ClNameT *pGroupName,
        ClNameT *pQName
        )
{
    ClRcT rc;

    CL_MSG_INIT_CHECK;

    rc  = clMsgGroupMembershipInfoSend(CL_MSG_DATA_DEL, pGroupName, pQName);
    if(rc != CL_OK)
    {
        clLogError("GRP", "QRM", "Failed to delete a queue from the group. error code [0x%x].", rc);
    }

    rc = clMsgQGroupMembershipCkptDataUpdate(CL_MSG_DATA_DEL, pGroupName, pQName, CL_TRUE);
    if(rc != CL_OK)
    {
        clLogError("GRP", "QRM", "Failed to update the MSG queue group cached checkpoint. error code [0x%x].", rc);
        goto error_out;
    }

    clLogTrace("GRP", "QRM", "Removed Message Queue [%.*s] from Message Queue Group [%.*s]."
                             , pQName->length, pQName->value
                             , pGroupName->length, pGroupName->value);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgQueueGroupDelete, 4, 0, 0)(
        const ClNameT *pGroupName
        )
{
    ClRcT rc;

    CL_MSG_INIT_CHECK;

    rc = clMsgGroupInfoUpdate(CL_MSG_DATA_DEL, (ClNameT*)pGroupName, 0);
    if(rc != CL_OK)
    {
        CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
        if(clMsgGroupEntryExists(pGroupName, NULL) == CL_TRUE)
        {
            CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
            /* 
             * MSG queue group is not removed from the cached checkpoint
             * if it cannot be deleted completely.
             */
            clLogError("GRP", "DEL", "Failed to inform deletion of group-name [%.*s]. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
            goto error_out;
        }
        CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    }

    ClIocPhysicalAddressT qGroupAddress = {0};
    rc = clMsgQGroupCkptDataUpdate(CL_MSG_DATA_DEL, (ClNameT *)pGroupName, 0, qGroupAddress, CL_TRUE);
    if(rc != CL_OK)
    {
        clLogError("GRP", "DEL", "Fail to remove Message Queue Group with name %.*s from the cached checkpoint. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    clLogTrace("GRP", "DEL", "Deleted Message Queue Group with name %.*s.", pGroupName->length, pGroupName->value);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgQueueGroupTrack, 4, 0, 0)(
        SaMsgHandleT appHandle,
        SaMsgHandleT msgHandle,
        const ClNameT *pGroupName,
        ClUint8T trackFlags,
        SaMsgQueueGroupNotificationBufferT *pNotificationBuffer
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgGroupRecordT *pMsgGroup;
    ClMsgGroupTrackListEntryT *pTrackEntry = NULL;
    ClMsgClientDetailsT *pClient;
    SaMsgQueueGroupNotificationT *pTempNotif;
    ClListHeadT *pTrackListHead;
    register ClListHeadT *pTemp;

    CL_MSG_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {   
        clLogError("GRP", "TRK", "Failed at Handle checkout for the client. Probably invalid Message client handle is passed. error code [0x%x].", rc);
        goto error_out;
    }  

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    if(clMsgGroupEntryExists(pGroupName, &pMsgGroup) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "TRK", "Message group with the name %.*s doesnot exists. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out_1;
    }

    CL_OSAL_MUTEX_LOCK(&pMsgGroup->groupLock);
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);

    if(trackFlags & SA_TRACK_CHANGES || trackFlags & SA_TRACK_CHANGES_ONLY )
    {
        pTrackListHead = &pMsgGroup->trackList;
        CL_LIST_FOR_EACH(pTemp, pTrackListHead)
        {
            pTrackEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupTrackListEntryT, list);
            if(pTrackEntry->clientHandle == msgHandle && 
                    pTrackEntry->compAddr.nodeAddress == pClient->address.nodeAddress && 
                    pTrackEntry->compAddr.portId == pClient->address.portId)
            {
                if(trackFlags & SA_TRACK_CHANGES_ONLY)
                    pTrackEntry->trackType = SA_TRACK_CHANGES_ONLY;
                else
                    pTrackEntry->trackType = SA_TRACK_CHANGES;
                goto nextTrackFlag;
            }
        }

        pTrackEntry = (ClMsgGroupTrackListEntryT*)clHeapAllocate(sizeof(ClMsgGroupTrackListEntryT));
        if(pTrackEntry == NULL)
        {
            CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);
            rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
            clLogError("GRP", "TRK", "Failed to allocate %u bytes of memory. error code [0x%x].", 
                    (ClUint32T) sizeof(ClMsgGroupTrackListEntryT), rc);
            goto error_out_1;
        }   

        if(trackFlags & SA_TRACK_CHANGES_ONLY)
            pTrackEntry->trackType = SA_TRACK_CHANGES_ONLY;
        else
            pTrackEntry->trackType = SA_TRACK_CHANGES;

        pTrackEntry->compAddr = pClient->address;
        pTrackEntry->appHandle = appHandle;
        pTrackEntry->clientHandle = msgHandle;
        pTrackEntry->pMsgGroup = pMsgGroup;

        clListAddTail(&pTrackEntry->list, &pMsgGroup->trackList);
        clListAddTail(&pTrackEntry->clientList, &pClient->trackList);
    }

nextTrackFlag:
    if(trackFlags & SA_TRACK_CURRENT)
    {
        ClUint32T count;

        rc = clMsgAllQueuesOfGroupGet(pMsgGroup, &pTempNotif, &count);
        if(rc != CL_OK)
        {
            CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);
            clLogError("GRP", "TRK", "Failed to get all the group members. error code [0x%x].", rc);
            goto error_out_1;
        }

        if(pNotificationBuffer->numberOfItems != 0 && pNotificationBuffer->numberOfItems < count)
        {
            pNotificationBuffer->numberOfItems = count;
            rc = CL_MSG_RC(CL_ERR_NO_SPACE);
            clLogError("GRP", "TRK", "Passed space is small for the data. passed %d and required is %d. error code [0x%x].",
                    pNotificationBuffer->numberOfItems, count, rc);
            /* Note : Sending rc = CL_OK, because the client side IDL wants rc to be CL_OK to
             * copy the "count" to the Notification buffer passed by client.*/
            rc = CL_OK;
            CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);
            goto error_out_2;

        }
        else if(pNotificationBuffer->numberOfItems == 0 )
        {
            if(pNotificationBuffer->notification)
                clHeapFree(pNotificationBuffer->notification);
            pNotificationBuffer->notification = pTempNotif;
        }
        else
        {
            memcpy(pNotificationBuffer->notification, pTempNotif, count * sizeof(SaMsgQueueGroupNotificationT));
            clHeapFree(pTempNotif);
        }
        pNotificationBuffer->numberOfItems = count;
        pNotificationBuffer->queueGroupPolicy = pMsgGroup->policy;
    }

    CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);
    goto out;

error_out_2:
    clHeapFree(pTempNotif);
error_out_1:
out:
    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("GRP", "QRM", "Failed to checkin the message client handle. error code [0x%x].", retCode);
error_out:
    return rc;
}


ClRcT clMsgQueueGroupTrackStopInternal(
        ClMsgClientDetailsT *pClient,
        SaMsgHandleT msgHandle,
        const ClNameT *pGroupName
        )
{
    ClRcT rc;
    register ClListHeadT *pTemp;
    ClMsgGroupRecordT *pMsgGroup;
    ClListHeadT *pTrackListHead;

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);

    if(clMsgGroupEntryExists(pGroupName, &pMsgGroup) == CL_FALSE)
    {
        CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "TST", "Message group with the name %.*s doesnot exists. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&pMsgGroup->groupLock);
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);

    pTrackListHead = &pMsgGroup->trackList;
    CL_LIST_FOR_EACH(pTemp, pTrackListHead)
    {
        ClMsgGroupTrackListEntryT *pTrackEntry = hashEntry(pTemp, ClMsgGroupTrackListEntryT, list);
        if(pTrackEntry->clientHandle == msgHandle && 
                pTrackEntry->compAddr.nodeAddress == pClient->address.nodeAddress && 
                pTrackEntry->compAddr.portId == pClient->address.portId)
        {
            clListDel(&pTrackEntry->list);
            clListDel(&pTrackEntry->clientList);
            clHeapFree(pTrackEntry);
            CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);
            rc = CL_OK;
            goto out;
        }
    }

    CL_OSAL_MUTEX_UNLOCK(&pMsgGroup->groupLock);

    rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
    clLogError("GRP", "TST", "There is no track enabled by this process. error code [0x%x].", rc);

out:
error_out:
    return rc;
}



ClRcT VDECL_VER(clMsgQueueGroupTrackStop, 4, 0, 0)(
        SaMsgHandleT msgHandle,
        const ClNameT *pGroupName
        )
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgClientDetailsT *pClient;

    CL_MSG_INIT_CHECK;

    rc = clHandleCheckout(gMsgClientHandleDb, msgHandle, (void**)&pClient);
    if(rc != CL_OK)
    {   
        clLogError("GRP", "TST", "Failed at Handle checkout for the client. Probably invalid Message client handle is passed. error code [0x%x].", rc);
        goto error_out;
    } 

    rc = clMsgQueueGroupTrackStopInternal(pClient, msgHandle, pGroupName);
    if(rc != CL_OK)
    {
        clLogError("GRP", "TST", "Failed to stop the tracking of group [%.*s] for client [0x%x]. error code [0x%x].",
                pGroupName->length, pGroupName->value, pClient->address.portId, rc);
    }

    retCode = clHandleCheckin(gMsgClientHandleDb, msgHandle);
    if(retCode != CL_OK)
        clLogError("GRP", "TST", "Failed to checkin the message client handle. error code [0x%x].", retCode);

error_out:
    return rc;
}
