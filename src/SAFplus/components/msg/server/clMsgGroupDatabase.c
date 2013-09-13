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


#include <clArchHeaders.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clList.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clVersion.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgEo.h>
#include <clMsgGroupDatabase.h>
#include <clMsgIdl.h>
#include <clCpmApi.h>
#include <clRmdIpi.h>
#include <clMsgDebugInternal.h>


ClOsalMutexT gClGroupDbLock;
static ClUint32T gClNumMsgGroups;

static void clMsgDeleteAllQueuesOfGroup(ClMsgGroupRecordT *pGroup);
static void clMsgGroupMembershipChangeInformTrackers(ClMsgGroupRecordT *pGroupEntry, SaNameT *pQueueName);


/****************************************************************************/

#define CL_MSG_QUE_GROUP_BITS          (5)
#define CL_MSG_QUE_GROUP_BUCKETS       (1 << CL_MSG_QUE_GROUP_BITS)
#define CL_MSG_QUE_GROUP_MASK          (CL_MSG_QUE_GROUP_BUCKETS - 1)


static struct hashStruct *ppMsgQGroupHashTable[CL_MSG_QUE_GROUP_BUCKETS];


static __inline__ ClUint32T clMsgGroupHash(const SaNameT *pQGroupName)
{
    return (ClUint32T)((ClUint32T)pQGroupName->value[0] & CL_MSG_QUE_GROUP_MASK);
}


static ClRcT clMsgGroupEntryAdd(ClMsgGroupRecordT *pGroupHashEntry)
{
    ClUint32T key = clMsgGroupHash(&pGroupHashEntry->name);
    return hashAdd(ppMsgQGroupHashTable, key, &pGroupHashEntry->hash);
}


static __inline__ void clMsgGroupEntryDel(ClMsgGroupRecordT *pGroupHashEntry)
{
    hashDel(&pGroupHashEntry->hash);
}


ClBoolT clMsgGroupEntryExists(const SaNameT *pQGroupName, ClMsgGroupRecordT **ppQGroupEntry)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgGroupHash(pQGroupName);

    for(pTemp = ppMsgQGroupHashTable[key];  pTemp; pTemp = pTemp->pNext)
    {   
        ClMsgGroupRecordT *pMsgQGroupEntry = hashEntry(pTemp, ClMsgGroupRecordT, hash);
        if(pQGroupName->length == pMsgQGroupEntry->name.length && 
                memcmp(pMsgQGroupEntry->name.value, pQGroupName->value, pQGroupName->length) == 0)
        {
            if(ppQGroupEntry != NULL)
                *ppQGroupEntry = pMsgQGroupEntry;
            return CL_TRUE;
        } 
    }   
    return CL_FALSE;
}

/***********************************************************************************************/

static ClRcT clMsgNewGroupAdd(SaNameT *pGroupName, SaMsgQueueGroupPolicyT policy, ClMsgGroupRecordT **ppGroupEntry)
{
    ClRcT rc, retCode;
    ClMsgGroupRecordT *pGroupEntry;
    
    if(clMsgGroupEntryExists(pGroupName, ppGroupEntry) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogWarning("GRP", "ADD", "A group with name [%.*s] already exists. error code [0x%x].",pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    pGroupEntry = (ClMsgGroupRecordT *)clHeapAllocate(sizeof(ClMsgGroupRecordT));
    if(pGroupEntry == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("GRP", "ADD", "Failed to allocate %u bytes of memory. error code [0x%x].", 
                (ClUint32T) sizeof(ClMsgGroupRecordT), rc);
        goto error_out;
    }

    memset(pGroupEntry, 0, sizeof(ClMsgGroupRecordT));

    rc = clOsalMutexInit(&pGroupEntry->groupLock);
    if(rc != CL_OK)
    {
        clLogError("GRP", "ADD", "Failed to create group [%.*s]'s lock. error code [0x%x].",
                pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    saNameCopy(&pGroupEntry->name, pGroupName);
    pGroupEntry->policy = policy;
    CL_LIST_HEAD_INIT(&pGroupEntry->qList);
    CL_LIST_HEAD_INIT(&pGroupEntry->trackList);

    rc = clMsgGroupEntryAdd(pGroupEntry); 
    if(rc != CL_OK)
    {
        clLogError("GRP", "ADD", "Failed to add entry to the group hash table. error code [0x%x].", rc);
        goto error_out_1;
    }

    if(ppGroupEntry != NULL)
        *ppGroupEntry = pGroupEntry;

    gClNumMsgGroups++;

    clLogDebug("GRP", "ADD", "Group [%.*s], policy [%d] Added.", pGroupEntry->name.length, pGroupEntry->name.value, pGroupEntry->policy);
    return rc;
    
error_out_1:
    retCode = clOsalMutexDestroy(&pGroupEntry->groupLock);
    if(retCode != CL_OK)
        clLogError("GRP", "ADD", "Failed to destroy the group [%.*s]'s mutex lock. error code [0x%x].",
                pGroupName->length, pGroupName->value, retCode);

error_out:
    return rc;
}

static ClRcT clMsgGroupDelete(SaNameT *pGroupName)
{
    ClRcT rc;
    ClMsgGroupRecordT *pGroupEntry;
    ClListHeadT *pListHead;
    ClMsgGroupTrackListEntryT *pTrackRecord; 
    register ClListHeadT *pTemp;

    if(clMsgGroupEntryExists(pGroupName, &pGroupEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogWarning("GRP", "DEL", "Group [%.*s] doesnot exist. error code [0x%x]", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    if(!CL_LIST_HEAD_EMPTY(&pGroupEntry->qList))
    {
        rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
        clLogWarning("GRP", "DEL", "Group [%.*s] is not empty, so cannot delete it now. error code [0x%x]", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    clMsgDeleteAllQueuesOfGroup(pGroupEntry);

    pListHead = &pGroupEntry->trackList;
    while(!CL_LIST_HEAD_EMPTY(pListHead))
    {
        pTemp = pListHead->pNext;
        pTrackRecord = CL_LIST_ENTRY(pTemp, ClMsgGroupTrackListEntryT,list);
        clListDel(&pTrackRecord->list);
    }

    clMsgGroupEntryDel(pGroupEntry);

    clLogDebug("GRP", "DEL", "Group [%.*s] policy [%d] deleted.", pGroupEntry->name.length, pGroupEntry->name.value, pGroupEntry->policy);

    rc = clOsalMutexDestroy(&pGroupEntry->groupLock);
    if(rc != CL_OK)
        clLogError("GRP", "DEL", "Failed to destroy the [%.*s]'s lock. error code [0x%x].", pGroupName->length, pGroupName->value, rc);

    clHeapFree(pGroupEntry);

    gClNumMsgGroups--;
error_out:
    return rc;
}

ClRcT clMsgGroupInfoUpdate(ClMsgSyncActionT syncupType, SaNameT *pGroupName, SaMsgQueueGroupPolicyT policy)
{
    ClRcT rc = CL_OK;

    /* Updating this message server's database. */
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            rc = clMsgNewGroupAdd(pGroupName, policy, NULL); 
            break;
        case CL_MSG_DATA_DEL : 
            rc = clMsgGroupDelete(pGroupName);
            if(rc != CL_OK)
            {
                clLogError("GRP", "ADD", "Failed to delete a group [%.*s] from database. error code [0x%x].",
                        pGroupName->length, pGroupName->value, rc);
            }
            break;
        default:
            break;
    }
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    return rc;
}

/****************************************************************************************/


ClBoolT clMsgDoesQExistInGroup(ClMsgGroupRecordT *pQGroupEntry, const SaNameT *pQueueName, 
        ClMsgGroupsQueueDetailsT **ppQListEntry)
{
    register ClListHeadT *pTemp = NULL;
    ClMsgGroupsQueueDetailsT *pQListEntry;

    CL_LIST_FOR_EACH(pTemp, &pQGroupEntry->qList)
    {
        pQListEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
        if(pQListEntry->qName.length == pQueueName->length &&
                memcmp(pQListEntry->qName.value, pQueueName->value, pQueueName->length) == 0)
        {
            if(ppQListEntry != NULL)
                *ppQListEntry = pQListEntry;
            return CL_TRUE;
        }
    }

    return CL_FALSE;
}


ClRcT clMsgAddQueueToGroup(SaNameT *pGroupName, SaNameT *pQueueName)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRecordT *pGroupEntry;
    ClMsgGroupsQueueDetailsT *pGQueueTemp;

    if(clMsgGroupEntryExists(pGroupName, &pGroupEntry) == CL_FALSE)
    {   
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "ADDQ", "Message group with the %.*s name does not exist. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    } 
    CL_OSAL_MUTEX_LOCK(&pGroupEntry->groupLock);

    if(clMsgDoesQExistInGroup(pGroupEntry, pQueueName, NULL) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogError("GRP", "ADDQ", "Message queue already exists in the Message Queue Group. error code [0x%x].", rc);
        goto error_out_1;
    }

    pGQueueTemp = (ClMsgGroupsQueueDetailsT *)clHeapAllocate(sizeof(*pGQueueTemp));
    if(pGQueueTemp == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("GRP", "ADDQ", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(*pGQueueTemp), rc);
        goto error_out_1;
    }

    saNameCopy(&pGQueueTemp->qName, pQueueName);
    pGQueueTemp->change = SA_MSG_QUEUE_GROUP_ADDED;

    clListAddTail(&pGQueueTemp->list, &pGroupEntry->qList);

    /*Inform the addition of queue to all the track enabled processes only on this machine. */
    clMsgGroupMembershipChangeInformTrackers(pGroupEntry, pQueueName);

    clLogDebug("GRP", "ADDQ", "Queue [%.*s] added to the group [%.*s].",
            pQueueName->length, pQueueName->value,
            pGroupEntry->name.length, pGroupEntry->name.value);

    goto out;

error_out_1:
out:
    CL_OSAL_MUTEX_UNLOCK(&pGroupEntry->groupLock);
error_out:
    return rc;
}

static ClRcT clMsgDelQueueFromGroup(SaNameT *pGroupName, SaNameT *pQueueName)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRecordT *pGroupEntry;
    ClMsgGroupsQueueDetailsT *pQListEntry = NULL;


    if(clMsgGroupEntryExists(pGroupName, &pGroupEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogWarning("GRP", "QRM", "Message group with the %.*s name doesnot exist. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }
    CL_OSAL_MUTEX_LOCK(&pGroupEntry->groupLock);

    if(clMsgDoesQExistInGroup(pGroupEntry, pQueueName, &pQListEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "QRM", "Message queue doesnot exist in the Message Queue Group. error code [0x%x].", rc);
        goto error_out_1;
    }

    /* Marking the entry as removed. This is for informing all, who has enabled tracking for the group. */
    pQListEntry->change = SA_MSG_QUEUE_GROUP_REMOVED;

    /*Inform the group membership changes to all the track enabled processes on this node. */
    clMsgGroupMembershipChangeInformTrackers(pGroupEntry, pQueueName);

    /* Earlier it was just marked for removal. Now, here it is actually being removed. */
    /* Removing Queue from the Gourp list*/
    clListDel(&pQListEntry->list);
    clHeapFree(pQListEntry);

    clLogDebug("GRP", "QRM", "Queue [%.*s] deleted from group [%.*s].",
            pQueueName->length, pQueueName->value,
            pGroupEntry->name.length, pGroupEntry->name.value);

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pGroupEntry->groupLock);

error_out:
    return rc;
}


ClRcT clMsgGroupMembershipInfoSend(ClMsgSyncActionT syncupType, SaNameT *pGroupName, SaNameT *pQueueName)
{
    ClRcT rc = CL_OK;

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    switch (syncupType)
    {
        case CL_MSG_DATA_ADD:
            rc = clMsgAddQueueToGroup(pGroupName, pQueueName);
            if(rc != CL_OK)
            {
                clLogError("GRP", "UPD", "Failed to add [%.*s] queue to the [%.*s] group. error code [0x%x].",
                        pQueueName->length, pQueueName->value, pGroupName->length, pGroupName->value, rc);
            }
            break;
        case CL_MSG_DATA_DEL:
            rc = clMsgDelQueueFromGroup(pGroupName, pQueueName);
            if(rc != CL_OK)
            {
                clLogError("GRP", "UPD", "Failed to delete [%.*s] queue from [%.*s] group. error code [0x%x].",
                        pQueueName->length, pQueueName->value, pGroupName->length, pGroupName->value, rc);
            }
            break;
        default:
            break;
    }
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);

    return rc;
}

static void clMsgDeleteAllQueuesOfGroup(ClMsgGroupRecordT *pGroup)
{
    register ClListHeadT *pQueueL = NULL;
    ClMsgGroupsQueueDetailsT *pQueueEntry;

    while(!CL_LIST_HEAD_EMPTY(&pGroup->qList))
    {
        pQueueL = pGroup->qList.pNext;
        pQueueEntry = CL_LIST_ENTRY(pQueueL, ClMsgGroupsQueueDetailsT, list);
        clListDel(&pQueueEntry->list);
        clHeapFree(pQueueEntry);
    }

    clLogDebug("GRP", "QDEL", "Deleted all queues of group [%.*s].",  pGroup->name.length, pGroup->name.value);
    return;
}


/*********************************************************************************/


ClRcT clMsgAllQueuesOfGroupGet(ClMsgGroupRecordT *pMsgGroup, SaMsgQueueGroupNotificationT **ppData, ClUint32T *pCount)
{
    ClRcT rc = CL_OK;
    register ClListHeadT *pTemp;
    ClListHeadT *pMsgQList;
    ClMsgGroupsQueueDetailsT *pQueueTemp;
    ClUint32T count = 0;
    ClUint32T i;

    pMsgQList = &pMsgGroup->qList;
    CL_LIST_FOR_EACH(pTemp, pMsgQList) count++;

    *ppData = (SaMsgQueueGroupNotificationT *)clHeapAllocate(count * sizeof(SaMsgQueueGroupNotificationT));
    if(*ppData == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("GRP", "TRK", "Failed to allocate %zd bytes of memory. error code [0x%x].", 
                (count * sizeof(SaMsgQueueGroupNotificationT)), rc);
        goto error_out;
    }

    memset(*ppData, 0, sizeof(SaMsgQueueGroupNotificationT) * count);

    for(pTemp = pMsgQList->pNext, i = 0 ; pTemp != pMsgQList && i < count; pTemp = pTemp->pNext, i++)
    {
        pQueueTemp = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT , list);
        memcpy(&((*ppData)[i].member.queueName), &pQueueTemp->qName, sizeof(SaNameT));
        (*ppData)[i].change = pQueueTemp->change; 
        pQueueTemp->change = SA_MSG_QUEUE_GROUP_NO_CHANGE;
    }

    clLogDebug("GRP", "QUE", "Packing all [%d] queues of group [%.*s].",
            count, pMsgGroup->name.length, pMsgGroup->name.value);

    *pCount = count;

error_out:
    return rc;
}


static ClRcT clMsgOnlyChangedEntriesOfGroupGet(ClMsgGroupRecordT *pMsgGroup, SaNameT *pQueueName, SaMsgQueueGroupNotificationT **ppData)
{
    ClRcT rc = CL_OK;
    register ClListHeadT *pTemp;
    ClListHeadT *pQList;
    ClMsgGroupsQueueDetailsT *pQueueTemp;

    *ppData = (SaMsgQueueGroupNotificationT *)clHeapAllocate(sizeof(SaMsgQueueGroupNotificationT));
    if(*ppData == NULL)
    {
        rc= CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("GRP", "TRK", "Failed to allocate %zd bytes of memory, error code [0x%x].", sizeof(SaMsgQueueGroupNotificationT), rc);
        goto error_out;
    }

    pQList = &pMsgGroup->qList;
    CL_LIST_FOR_EACH(pTemp, pQList)
    {
        pQueueTemp = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT , list); 
        if(pQueueTemp->qName.length == pQueueName->length || 
                memcpy(pQueueTemp->qName.value, pQueueName->value, pQueueName->length) == 0)
        {
            memcpy(&(*ppData)->member.queueName, pQueueName, sizeof(SaNameT));
            (*ppData)->change = pQueueTemp->change;
            pQueueTemp->change = SA_MSG_QUEUE_GROUP_NO_CHANGE;
            break;
        }
    }

    clLogDebug("GRP", "QUE", "Packing only status-changed queues of group [%.*s].", pMsgGroup->name.length, pMsgGroup->name.value);

error_out:
    return rc;
}


static void clMsgGroupMembershipChangeInformTrackers(ClMsgGroupRecordT *pGroupEntry, SaNameT *pQueueName)
{
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pTrackList;
    ClMsgGroupTrackListEntryT *pTracker;
    ClUint32T changeCount = 0;
    SaMsgQueueGroupNotificationBufferT changesData = {0};
    SaMsgQueueGroupNotificationBufferT changesOnlyData = {0};

    pTrackList = &pGroupEntry->trackList;
    CL_LIST_FOR_EACH(pTemp, pTrackList)
    {
        pTracker = CL_LIST_ENTRY(pTemp, ClMsgGroupTrackListEntryT, list);
        switch(pTracker->trackType)
        {
            case SA_TRACK_CHANGES:
                if(changesData.notification == NULL)
                {
                    clMsgAllQueuesOfGroupGet(pGroupEntry, &changesData.notification, &changeCount);
                    changesData.numberOfItems = changeCount;
                    changesData.queueGroupPolicy = pGroupEntry->policy;
                }
                clLogTrace("GRP", "TRK", "Calling the callback for track-changes for group [%.*s].",
                        pGroupEntry->name.length, pGroupEntry->name.value);
                clMsgCallClientsTrackCallback(pTracker->compAddr, &pGroupEntry->name, pTracker->appHandle, &changesData);
                break;
            case SA_TRACK_CHANGES_ONLY:
                if(changesOnlyData.notification == NULL)
                {
                    clMsgOnlyChangedEntriesOfGroupGet(pGroupEntry, pQueueName, &changesOnlyData.notification);
                    changesData.numberOfItems = 1;
                    changesData.queueGroupPolicy = pGroupEntry->policy;
                }
                clLogTrace("GRP", "TRK", "Calling the callback for track-changes-only for group [%.*s].",
                        pGroupEntry->name.length, pGroupEntry->name.value);
                clMsgCallClientsTrackCallback(pTracker->compAddr, &pGroupEntry->name, pTracker->appHandle, &changesOnlyData);
                break;
            case SA_TRACK_CURRENT:
                goto out;
        }
    }

    if(changesData.notification != NULL)
        clHeapFree(changesData.notification);
    if(changesOnlyData.notification != NULL)
        clHeapFree(changesOnlyData.notification);

out:
    return;
}


/*********************************************************************************/

void clMsgGroupQueuesShow(ClMsgGroupRecordT *pGroup)
{
    ClListHeadT *pQList;
    register ClListHeadT *pTemp;
    ClMsgGroupsQueueDetailsT *pQueue;

    pQList = &pGroup->qList;
    CL_LIST_FOR_EACH(pTemp, pQList)
    {
        pQueue = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
        clOsalPrintf("[%.*s] ",pQueue->qName.length, pQueue->qName.value);
    }
}

void clMsgGroupTrackersShow(ClMsgGroupRecordT *pGroup)
{
    ClListHeadT *pTrackList;
    register ClListHeadT *pTemp;
    ClMsgGroupTrackListEntryT *pTracker;

    pTrackList = &pGroup->trackList;
    CL_LIST_FOR_EACH(pTemp, pTrackList)
    {
         pTracker = CL_LIST_ENTRY(pTemp, ClMsgGroupTrackListEntryT, list);
         clOsalPrintf("[0x%x:0x%x] ", pTracker->compAddr.nodeAddress, pTracker->compAddr.portId);
    }
}


void clMsgGroupDatabaseShow(void)
{
    register struct hashStruct *pTemp;
    ClUint32T key;
    ClUint32T flag = 0;
    ClMsgGroupRecordT *pGroupEntry;
#define LINE_LEN 76
    char msgLn[LINE_LEN+1] = {0};

    memset(&msgLn, '=', LINE_LEN);

    clOsalPrintf("\n\n%s\n", msgLn);
    clOsalPrintf("%-40s %6s %11s %-40s\n", "Group", "Policy", "Trackers","Queues");
    clOsalPrintf("%s\n", msgLn);
    for(key = 0; key < CL_MSG_QUE_GROUP_BUCKETS; key++)
    {
        for(pTemp = ppMsgQGroupHashTable[key];pTemp; pTemp = pTemp->pNext)
        {
             pGroupEntry = hashEntry(pTemp, ClMsgGroupRecordT, hash);

             clOsalPrintf("%-40s %6d ",
                     pGroupEntry->name.length, pGroupEntry->name.value,
                     pGroupEntry->policy);
             clMsgGroupTrackersShow(pGroupEntry);
             clMsgGroupQueuesShow(pGroupEntry);
             clOsalPrintf("\n");
             flag = 1;
        }
    }
    if(flag == 0)
        clOsalPrintf("No record present in the Group-Name's Database.\n");
    clOsalPrintf("%s\n\n", msgLn);
}


/*********************************************************************************/
