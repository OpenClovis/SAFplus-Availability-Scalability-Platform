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
#include <xdrClMsgGroupSyncupRecordT.h>


ClOsalMutexT gClGroupDbLock;
static ClUint32T gClNumMsgGroups;

static void clMsgDeleteAllQueuesOfGroup(ClMsgGroupRecordT *pGroup);
static void clMsgGroupMembershipChangeInformTrackers(ClMsgGroupRecordT *pGroupEntry, ClNameT *pQueueName);


/****************************************************************************/

#define CL_MSG_QUE_GROUP_BITS          (5)
#define CL_MSG_QUE_GROUP_BUCKETS       (1 << CL_MSG_QUE_GROUP_BITS)
#define CL_MSG_QUE_GROUP_MASK          (CL_MSG_QUE_GROUP_BUCKETS - 1)


static struct hashStruct *ppMsgQGroupHashTable[CL_MSG_QUE_GROUP_BUCKETS];


static __inline__ ClUint32T clMsgGroupHash(const ClNameT *pQGroupName)
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


ClBoolT clMsgGroupEntryExists(const ClNameT *pQGroupName, ClMsgGroupRecordT **ppQGroupEntry)
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

ClRcT clMsgNewGroupAddInternal(ClMsgGroupSyncupRecordT *pGroupInfo, ClMsgGroupRecordT **ppGroupEntry)
{
    ClRcT rc;
    ClRcT retCode;
    ClMsgGroupRecordT *pGroupEntry;

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
                pGroupInfo->name.length, pGroupInfo->name.value, rc);
        goto error_out;
    }

    memcpy(&pGroupEntry->name, &pGroupInfo->name, sizeof(ClNameT));
    pGroupEntry->policy = pGroupInfo->policy;
    pGroupEntry->pRRNext = NULL; 
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
        clLogError("GRP", "ADD", "Failed to destroye the group [%.*s]'s mutex lock. error code [0x%x].",
                pGroupInfo->name.length, pGroupInfo->name.value, retCode);
error_out:
    clHeapFree(pGroupEntry);
    return rc;
}


static ClRcT clMsgNewGroupAdd(ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy, ClMsgGroupRecordT **ppGroupEntry)
{
    ClRcT rc;
    ClMsgGroupSyncupRecordT recvdRecord;

    if(clMsgGroupEntryExists(pGroupName, ppGroupEntry) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogWarning("GRP", "ADD", "A group with name [%.*s] already exists. error code [0x%x].",pGroupName->length, pGroupName->value, rc);
        goto error_out;
    }

    memcpy(&recvdRecord.name, pGroupName, sizeof(ClNameT));
    recvdRecord.policy = policy;

    rc = clMsgNewGroupAddInternal(&recvdRecord, ppGroupEntry);

error_out:
    return rc;
}

    
static ClRcT clMsgGroupDelete(ClNameT *pGroupName)
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



ClRcT VDECL_VER(clMsgGroupDatabaseUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy)
{
    ClRcT rc;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogError("GRP", "UDB", "Failed to get the RMD source address. error code [0x%x].", rc);
        goto error_out;
    }
    if(srcAddr.nodeAddress == gClMyAspAddress)
        goto out;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
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
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
out:
    return rc;
}

ClRcT clMsgGroupInfoUpdateSend(ClMsgSyncActionT syncupType, ClNameT *pGroupName, SaMsgQueueGroupPolicyT policy)
{
    ClRcT rc;

    /* Updating all the message server's database except my own. */
    rc = clMsgGroupUpdateSendThroughIdl(syncupType, pGroupName, policy);
    if(rc != CL_OK)
    {
        clLogError("GRP","UPD", "Failed to update a new-group addition or old-group deletion with other nodes. error code [0x%x].",rc);
        goto error_out;
    }

    /* Updating this message server's database. */
    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
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
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
    return rc;
}



/****************************************************************************************/


ClBoolT clMsgDoesQExistInGroup(ClMsgGroupRecordT *pQGroupEntry, const ClNameT *pQueueName, 
        ClMsgQueuesGroupDetailsT **ppGListEntry, ClMsgGroupsQueueDetailsT **ppQListEntry)
{
    register ClListHeadT *pTemp = NULL;
    ClMsgGroupsQueueDetailsT *pQListEntry;
    ClMsgQueuesGroupDetailsT *pGListEntry;

    CL_LIST_FOR_EACH(pTemp, &pQGroupEntry->qList)
    {
        pQListEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
        if(pQListEntry->pQueueDetails->qName.length == pQueueName->length &&
                memcmp(pQListEntry->pQueueDetails->qName.value, pQueueName->value, pQueueName->length) == 0)
        {
            if(ppQListEntry != NULL)
                *ppQListEntry = pQListEntry;
            if(ppGListEntry != NULL)
            {
                CL_LIST_FOR_EACH(pTemp, &pQListEntry->pQueueDetails->groupList)
                {
                    pGListEntry = CL_LIST_ENTRY(pTemp, ClMsgQueuesGroupDetailsT, list);
                    if(pGListEntry->pGroupDetails->name.length == pQGroupEntry->name.length &&
                            memcmp(pGListEntry->pGroupDetails->name.value, pQGroupEntry->name.value, pQGroupEntry->name.length) == 0)
                    {
                        *ppGListEntry = pGListEntry;
                        goto out;
                    }

                }
                clLogCritical("GRP", "Q-FIND", "There is some inconsistency in the database.");
            }
out:
            return CL_TRUE;
        }
    }

    return CL_FALSE;
}


ClRcT clMsgAddQueueToGroup(ClNameT *pGroupName, ClNameT *pQueueName)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRecordT *pGroupEntry;
    ClMsgQueueRecordT *pQueueEntry;
    ClMsgGroupsQueueDetailsT *pGQueueTemp;
    ClMsgQueuesGroupDetailsT *pQGroupTemp;

    if(clMsgGroupEntryExists(pGroupName, &pGroupEntry) == CL_FALSE)
    {   
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "ADDQ", "Message group with the %.*s name doesnot exist. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    } 
    CL_OSAL_MUTEX_LOCK(&pGroupEntry->groupLock);

    if(clMsgQNameEntryExists(pQueueName, &pQueueEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "ADDQ", "Message queue with the name %.*s doesnot exist. error code [0x%x].", pQueueName->length, pQueueName->value, rc);
        goto error_out_1;
    } 

    if(clMsgDoesQExistInGroup(pGroupEntry, pQueueName, NULL, NULL) == CL_TRUE)
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

    pQGroupTemp = (ClMsgQueuesGroupDetailsT *)clHeapAllocate(sizeof(*pQGroupTemp));
    if(pQGroupTemp == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("GRP", "ADDQ", "Failed to allocate memory of %zd bytes. error code [0x%x].", sizeof(*pQGroupTemp), rc);
        goto error_out_2;
    }

    pGQueueTemp->pQueueDetails = pQueueEntry;
    pGQueueTemp->change = SA_MSG_QUEUE_GROUP_ADDED;
    pQGroupTemp->pGroupDetails = pGroupEntry;

    /* Updating the round robin informatoin if the queue was a last queue of the group. */
    if(CL_LIST_HEAD_EMPTY(&pGroupEntry->qList) && 
            (pGroupEntry->policy == SA_MSG_QUEUE_GROUP_ROUND_ROBIN || 
             (pGroupEntry->policy == SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN && pQueueEntry->compAddr.nodeAddress == gClMyAspAddress)))
        pGroupEntry->pRRNext = &pGQueueTemp->list;
 
    clListAddTail(&pGQueueTemp->list, &pGroupEntry->qList);
    clListAddTail(&pQGroupTemp->list, &pQueueEntry->groupList);


    /*Inform the addition of queue to all the track enabled processes only on this machine. */
    clMsgGroupMembershipChangeInformTrackers(pGroupEntry, pQueueName);

    clLogDebug("GRP", "ADDQ", "Queue [%.*s] added to the group [%.*s].",
            pQueueEntry->qName.length, pQueueEntry->qName.value,
            pGroupEntry->name.length, pGroupEntry->name.value);

    goto out;

error_out_2:
    clHeapFree(pGQueueTemp);
error_out_1:
out:
    CL_OSAL_MUTEX_UNLOCK(&pGroupEntry->groupLock);
error_out:
    return rc;
}

static ClRcT clMsgDelQueueFromGroup(ClNameT *pGroupName, ClNameT *pQueueName)
{
    ClRcT rc = CL_OK;
    ClMsgGroupRecordT *pGroupEntry;
    ClMsgGroupsQueueDetailsT *pQListEntry = NULL;
    ClMsgQueuesGroupDetailsT *pGListEntry = NULL;


    if(clMsgGroupEntryExists(pGroupName, &pGroupEntry) == CL_FALSE)
    {   
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "QRM", "Message group with the %.*s name doesnot exist. error code [0x%x].", pGroupName->length, pGroupName->value, rc);
        goto error_out;
    } 
    CL_OSAL_MUTEX_LOCK(&pGroupEntry->groupLock);

    if(clMsgDoesQExistInGroup(pGroupEntry, pQueueName, &pGListEntry, &pQListEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogError("GRP", "QRM", "Message queue doesnot exist in the Message Queue Group. error code [0x%x].", rc);
        goto error_out_1;
    }


    /* Updating the round-robin-next if the entry pRRNext entry is the one which is getting deleted. */
    if(pGroupEntry->pRRNext == &pQListEntry->list)
    {
        clMsgRRNextMemberGet(pGroupEntry, NULL);
        if(pGroupEntry->pRRNext == &pQListEntry->list)
            pGroupEntry->pRRNext = NULL;
    }

    /* Marking the entry as removed. This is for infroming all, who has enabled tracking for the group. */
    pQListEntry->change = SA_MSG_QUEUE_GROUP_REMOVED;

    /*Inform the group membership changes to all the track enabled processes on this node. */
    clMsgGroupMembershipChangeInformTrackers(pGroupEntry, pQueueName);

    /* Earlier it was just marked for removal. Now, here it is actually being removed. */
    /* Removing Queue from the Gourp list*/
    clListDel(&pQListEntry->list);
    clHeapFree(pQListEntry);
    /* Removing Group from the Queue List */
    clListDel(&pGListEntry->list);
    clHeapFree(pGListEntry);

    clLogDebug("GRP", "QRM", "Queue [%.*s] deleted from group [%.*s].",
            pQueueName->length, pQueueName->value,
            pGroupEntry->name.length, pGroupEntry->name.value);

error_out_1:
    CL_OSAL_MUTEX_UNLOCK(&pGroupEntry->groupLock);

error_out:
    return rc;
}


ClRcT clMsgGroupMembershipInfoSend(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName)
{
    ClRcT rc;

    rc = clMsgGroupMembershipChangeInfoSendThroughIdl(syncupType, pGroupName, pQueueName);
    if(rc != CL_OK)
    {
        clLogError("GRP", "USND", "Failed to send the group update through IDL. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
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
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
    return rc;
}


ClRcT VDECL_VER(clMsgGroupMembershipUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClNameT *pGroupName, ClNameT *pQueueName)
{
    ClRcT rc;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogError("GRP", "MBR", "Failed to get the RMD source address. error code [0x%x].", rc);
        goto error_out;
    }
    if(srcAddr.nodeAddress == gClMyAspAddress)
        goto out;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);
    switch(syncupType)
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
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
out:
    return rc;
}


static void clMsgDeleteAllQueuesOfGroup(ClMsgGroupRecordT *pGroup)
{
    register ClListHeadT *pGroupL = NULL;
    register ClListHeadT *pQueueL = NULL;
    ClMsgQueuesGroupDetailsT *pGroupEntry;
    ClMsgGroupsQueueDetailsT *pQueueEntry;

    while(!CL_LIST_HEAD_EMPTY(&pGroup->qList))
    {
        pQueueL = pGroup->qList.pNext;
        pQueueEntry = CL_LIST_ENTRY(pQueueL, ClMsgGroupsQueueDetailsT, list);
        CL_LIST_FOR_EACH(pGroupL, &pQueueEntry->pQueueDetails->groupList)
        {
            pGroupEntry = CL_LIST_ENTRY(pGroupL, ClMsgQueuesGroupDetailsT, list);
            if(pGroupEntry->pGroupDetails->name.length == pGroup->name.length && 
                    memcmp(pGroupEntry->pGroupDetails->name.value, pGroup->name.value, pGroup->name.length) == 0)
            {
                clListDel(&pGroupEntry->list);
                clHeapFree(pGroupEntry);
                break;
            }
        }
        clListDel(&pQueueEntry->list);
        clHeapFree(pQueueEntry);
    }
    
    clLogDebug("GRP", "QDEL", "Deleted all queues of group [%.*s].",  pGroup->name.length, pGroup->name.value);
    return;
}


void clMsgDeleteAllGroupsOfQueue(ClMsgQueueRecordT *pQueue)
{
    register ClListHeadT *pGroupL = NULL;
    register ClListHeadT *pQueueL = NULL;
    ClMsgQueuesGroupDetailsT *pGroupEntry;
    ClMsgGroupsQueueDetailsT *pQueueEntry;

    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);

    while(!CL_LIST_HEAD_EMPTY(&pQueue->groupList))
    {
        pGroupL = pQueue->groupList.pNext;
        pGroupEntry = CL_LIST_ENTRY(pGroupL, ClMsgQueuesGroupDetailsT, list);
        CL_LIST_FOR_EACH(pQueueL, &pGroupEntry->pGroupDetails->qList)
        {
            pQueueEntry = CL_LIST_ENTRY(pQueueL, ClMsgGroupsQueueDetailsT, list);
            if(pQueueEntry->pQueueDetails->qName.length == pQueue->qName.length && 
               memcmp(pQueueEntry->pQueueDetails->qName.value, pQueue->qName.value, pQueue->qName.length) == 0)
            {
                /*
                 * Update the round robin next entry if the current entry is getting deleted.
                 */
                if(pGroupEntry->pGroupDetails->pRRNext == &pQueueEntry->list)
                {
                    clMsgRRNextMemberGet(pGroupEntry->pGroupDetails, NULL);
                    if(pGroupEntry->pGroupDetails->pRRNext == &pQueueEntry->list)
                        pGroupEntry->pGroupDetails->pRRNext = NULL;
                }
                clListDel(&pQueueEntry->list);
                clHeapFree(pQueueEntry);
                break;
            }
        }
        clListDel(&pGroupEntry->list);
        clHeapFree(pGroupEntry);
    }

    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    
    clLogDebug("GRP", "GDEL", "Deleted all groups of queue [%.*s].",  pQueue->qName.length, pQueue->qName.value);
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
        memcpy(&((*ppData)[i].member.queueName), &pQueueTemp->pQueueDetails->qName, sizeof(ClNameT));
        (*ppData)[i].change = pQueueTemp->change; 
        pQueueTemp->change = SA_MSG_QUEUE_GROUP_NO_CHANGE;
    }

    clLogDebug("GRP", "QUE", "Packing all [%d] queues of group [%.*s].",
            count, pMsgGroup->name.length, pMsgGroup->name.value);

    *pCount = count;

error_out:
    return rc;
}


static ClRcT clMsgOnlyChangedEntriesOfGroupGet(ClMsgGroupRecordT *pMsgGroup, ClNameT *pQueueName, SaMsgQueueGroupNotificationT **ppData)
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
        if(pQueueTemp->pQueueDetails->qName.length == pQueueName->length || 
                memcpy(pQueueTemp->pQueueDetails->qName.value, pQueueName->value, pQueueName->length) == 0)
        {
            memcpy(&(*ppData)->member.queueName, pQueueName, sizeof(ClNameT));
            (*ppData)->change = pQueueTemp->change;
            pQueueTemp->change = SA_MSG_QUEUE_GROUP_NO_CHANGE;
            break;
        }
    }

    clLogDebug("GRP", "QUE", "Packing only status-changed queues of group [%.*s].", pMsgGroup->name.length, pMsgGroup->name.value);

error_out:
    return rc;
}


static void clMsgGroupMembershipChangeInformTrackers(ClMsgGroupRecordT *pGroupEntry, ClNameT *pQueueName)
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



/****************************************************************************************/
/* The following function is for computing the whom to send next in case of Global/Local
 * Round Robin. */

ClRcT clMsgRRNextMemberGet(ClMsgGroupRecordT *pMsgGroup, ClMsgQueueRecordT **pQueue)
{
    ClRcT rc = CL_MSG_RC(CL_ERR_QUEUE_NOT_AVAILABLE);
    register ClListHeadT *pTemp;
    ClMsgQueueRecordT *pTempQueue;
    ClListHeadT *pMsgQList;
    ClMsgGroupsQueueDetailsT *pMsgQEntry;

    if(pMsgGroup->pRRNext == NULL)
        goto error_out;

    pMsgQList = &pMsgGroup->qList;
    pTemp = pMsgGroup->pRRNext;

    do {
        switch(pMsgGroup->policy)
        {
            case SA_MSG_QUEUE_GROUP_ROUND_ROBIN:
                pTemp = pTemp->pNext;
                if(pTemp == pMsgQList) pTemp = pTemp->pNext;
                pMsgQEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
                if(pTemp == pMsgGroup->pRRNext && pMsgQEntry->pQueueDetails->compAddr.nodeAddress == 0)
                {
                    /* There is only one queue in the group. And it is also being moved to another node. */
                    rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
                    clLogError("GRP", "GRR", "Queue [%.*s] is being moved. Try again later. error code [0x%x].", 
                            pMsgQEntry->pQueueDetails->qName.length, pMsgQEntry->pQueueDetails->qName.value, rc);
                    goto error_out;
                }
                break;
            case SA_MSG_QUEUE_GROUP_LOCAL_ROUND_ROBIN:
                do {
                    pTemp = pTemp->pNext;
                    if(pTemp == pMsgQList) pTemp = pTemp->pNext;
                    pMsgQEntry = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
                } /* The "do-while" loop will continue till it finds the queue with my own node address or till it reaches the first where queue it started. */ 
                while(pMsgQEntry->pQueueDetails->compAddr.nodeAddress != gClMyAspAddress && pTemp != pMsgGroup->pRRNext);

                /* The following "if" and "else-if" are needed for the cases, when the above "do-while" loop comes out with "pTemp == pMsgGroup->pRRNext". */
                if(pMsgQEntry->pQueueDetails->compAddr.nodeAddress == 0)
                {
                    /* There is only one queue in the group from this node. And it is also being moved to another node. */
                    rc = CL_MSG_RC(CL_ERR_TRY_AGAIN);
                    clLogError("GRP", "LRR", "Queue [%.*s] is being moved. Try again later. error code [0x%x].",
                            pMsgQEntry->pQueueDetails->qName.length, pMsgQEntry->pQueueDetails->qName.value, rc);
                    goto error_out;
                } 
                else if(pMsgQEntry->pQueueDetails->compAddr.nodeAddress != gClMyAspAddress)
                {
                    /* There was only one queue in the group from this node, which just got moved. */
                    rc = CL_MSG_RC(CL_ERR_QUEUE_NOT_AVAILABLE);
                    clLogError("GRP", "LRR", "Queue [%.*s] is moved. No queues in the group. error code [0x%x].",
                            pMsgQEntry->pQueueDetails->qName.length, pMsgQEntry->pQueueDetails->qName.value, rc);
                    pTemp = NULL;
                    goto error_out;
                }
                break;
            default:
                rc = CL_MSG_RC(CL_ERR_QUEUE_NOT_AVAILABLE);
                goto error_out;
        }

        rc = CL_OK;

        pMsgQEntry = CL_LIST_ENTRY((pMsgGroup->pRRNext), ClMsgGroupsQueueDetailsT, list);
        pTempQueue = pMsgQEntry->pQueueDetails;

        pMsgGroup->pRRNext = pTemp;

        if(pQueue == NULL) break;

    } while(pTempQueue->compAddr.nodeAddress == 0);

    if(pQueue != NULL)
        *pQueue = pTempQueue;

error_out:
    return rc;
}



ClRcT clMsgGroupLocalBestNextMemberGet(ClMsgGroupRecordT *pMsgGroup, ClMsgQueueRecordT **pQueue, SaUint8T priority)
{
    ClRcT rc = CL_MSG_RC(CL_ERR_QUEUE_NOT_AVAILABLE), retCode;
    register ClListHeadT *pTemp;
    ClListHeadT *pMsgQList;
    ClMsgGroupsQueueDetailsT *pQDetails;
    ClMsgQueueRecordT *pBestQueueRecord = NULL;
    SaSizeT bestMsgQueueSize = 0;
    ClMsgQueueInfoT *pQInfo;

    pMsgQList = &pMsgGroup->qList;
    CL_OSAL_MUTEX_LOCK(&gClLocalQsLock);
    CL_LIST_FOR_EACH(pTemp, pMsgQList)
    {
        pQDetails = CL_LIST_ENTRY(pTemp, ClMsgGroupsQueueDetailsT, list);
        rc = clHandleCheckout(gClMsgQDatabase, pQDetails->pQueueDetails->qHandle, (void **)&pQInfo);
        if(pQDetails->pQueueDetails->compAddr.nodeAddress != gClMyAspAddress)
            continue;
        if(rc != CL_OK)
            continue;
        if((pQInfo->size[priority] - pQInfo->usedSize[priority]) > bestMsgQueueSize)
        {
            bestMsgQueueSize = pQInfo->size[priority] - pQInfo->usedSize[priority];
            pBestQueueRecord = pQDetails->pQueueDetails;
        }
        retCode = clHandleCheckin(gClMsgQDatabase, pQDetails->pQueueDetails->qHandle);
        if(retCode != CL_OK)
            clLogError("GRP", "LBEST", "Failed to checkin queue handle. error code [0x%x].", retCode);
    }
    CL_OSAL_MUTEX_UNLOCK(&gClLocalQsLock);
    *pQueue = pBestQueueRecord;
    return rc;
}

/*********************************************************************************/

ClRcT clMsgGroupDatabasePack(ClBufferHandleT message)
{
    ClRcT rc;
    ClUint32T i;
    ClUint32T numEntries;
    register struct hashStruct *pTemp;
    ClMsgGroupRecordT *pGroupEntry;
    ClMsgGroupSyncupRecordT tempGroup;

    numEntries = htonl(gClNumMsgGroups);
    rc = clBufferNBytesWrite(message, (ClUint8T*)&numEntries, sizeof(numEntries));
    if(rc != CL_OK)
    {
        clLogCritical("DB", "PACK", "Failed to write the number of entries to buffer. error code [0x%x].", rc);
        goto error_out;
    }

    clLogDebug("DB", "PACK", "Number of groups in the system [%d].", gClNumMsgGroups);

    for(i = 0 ; i < CL_MSG_QUE_GROUP_BUCKETS; i++)
    {
        if(ppMsgQGroupHashTable[i] == NULL)
            continue;
        for(pTemp = ppMsgQGroupHashTable[i]; pTemp ; pTemp = pTemp->pNext)
        {
            pGroupEntry = hashEntry(pTemp, ClMsgGroupRecordT, hash);

            memset(&tempGroup, 0, sizeof(tempGroup));

            memcpy(&tempGroup.name, &pGroupEntry->name, sizeof(ClNameT));
            tempGroup.policy = pGroupEntry->policy;

            clLogTrace("DB", "PACK", "Group [%.*s] policy [%d].", 
                    tempGroup.name.length, tempGroup.name.value, tempGroup.policy);

            rc = VDECL_VER(clXdrMarshallClMsgGroupSyncupRecordT, 4, 0, 0)((void*)&tempGroup, message, 0);
            if(rc != CL_OK)
            {
                clLogError("DB", "PACK", "Failed to marshall the Group Info. error code [0x%x].", rc);
                goto error_out;
            }
        }
    }

error_out:
    return rc;
}


ClRcT clMsgGroupDatabaseSyncup(ClBufferHandleT message)
{
    ClRcT rc;
    ClUint32T i;
    ClUint32T numEntries = 0;
    ClUint32T tempSize;
    ClMsgGroupSyncupRecordT tempGroup;

    tempSize = sizeof(numEntries);
    rc = clBufferNBytesRead(message, (ClUint8T*)&numEntries, &tempSize);
    CL_ASSERT(rc == CL_OK);

    numEntries = ntohl(numEntries);

    clLogDebug("DB", "UPD", "Number of groups for update [%d].", numEntries);

    for(i = 0; i < numEntries; i++)
    {
        memset(&tempGroup, 0, sizeof(tempGroup));

        rc = VDECL_VER(clXdrUnmarshallClMsgGroupSyncupRecordT, 4, 0, 0)(message, (void*)&tempGroup);
        if(rc != CL_OK)
        {
            clLogError("DB", "UPD", "Faild to unmarshal group infromation. error code [0x%x].", rc);
            CL_ASSERT(0);
        }

        clLogTrace("DB", "UPD", "Group [%.*s], policy [%d].", 
                tempGroup.name.length, tempGroup.name.value, tempGroup.policy);

        if(clMsgGroupEntryExists(&tempGroup.name, NULL) == CL_FALSE)
        {
            rc = clMsgNewGroupAddInternal(&tempGroup, NULL);
            if(rc != CL_OK)
            {
                clLogError("DB", "UPD", "Failed to add a new group entry [%.*s]. error code [0x%x].",
                        tempGroup.name.length, tempGroup.name.value, rc);
                CL_ASSERT(0);
            }
        }
    }

    return CL_OK;
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
        clOsalPrintf("[%.*s] ",pQueue->pQueueDetails->qName.length, pQueue->pQueueDetails->qName.value);
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
    ClMsgGroupsQueueDetailsT *pRRTemp;
    ClMsgQueueRecordT *pRRQueue;
#define LINE_LEN 76
    char msgLn[LINE_LEN+1] = {0};

    memset(&msgLn, '=', LINE_LEN);

    clOsalPrintf("\n\n%s\n", msgLn);
    clOsalPrintf("%-40s %6s %11s %11s %-40s\n", "Group", "Policy", "RRNext", "Trackers","Queues");
    clOsalPrintf("%s\n", msgLn);
    for(key = 0; key < CL_MSG_QUE_GROUP_BUCKETS; key++)
    {
        for(pTemp = ppMsgQGroupHashTable[key];pTemp; pTemp = pTemp->pNext)
        {
             pGroupEntry = hashEntry(pTemp, ClMsgGroupRecordT, hash);
             pRRTemp = CL_LIST_ENTRY(pGroupEntry->pRRNext, ClMsgGroupsQueueDetailsT, list);
             pRRQueue = pRRTemp->pQueueDetails;

             clOsalPrintf("%-40s %6d [0x%x:0x%x] ",
                     pGroupEntry->name.length, pGroupEntry->name.value,
                     pGroupEntry->policy,
                     pRRQueue->compAddr.nodeAddress, pRRQueue->compAddr.portId);
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
