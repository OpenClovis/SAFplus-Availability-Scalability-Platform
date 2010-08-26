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


#include <netinet/in.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clList.h>
#include <clBufferApi.h>
#include <clDebugApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clVersion.h>
#include <saMsg.h>
#include <clMsgCommon.h>
#include <clMsgEo.h>
#include <clMsgDatabase.h>
#include <clMsgGroupDatabase.h>
#include <clMsgIdl.h>
#include <clRmdIpi.h>
#include <clMsgDebugInternal.h>
#include <xdrClMsgQueueSyncupRecordT.h>
#include <xdrClMsgGroupSyncupRecordT.h>


ClOsalMutexT gClQueueDbLock;
static ClUint32T gClNumMsgQueues;


/*****************************************************************************/


struct hashStruct *ppMsgNodeQHashTable[CL_MSG_NODE_Q_BUCKETS];

static ClRcT clMsgNodeQEntryAdd(ClMsgQueueRecordT *pHashEntry)
{
    ClUint32T key = clMsgNodeQHash(pHashEntry->compAddr.nodeAddress);
    return hashAdd(ppMsgNodeQHashTable, key, &pHashEntry->qNodeHash);
}

static void clMsgNodeQEntryDel(ClMsgQueueRecordT *pHashEntry)
{
    hashDel(&pHashEntry->qNodeHash);
}


/*****************************************************************************/


#define CL_MSG_QNAME_BUCKET_BITS  (6)
#define CL_MSG_QNAME_BUCKETS      (1 << CL_MSG_QNAME_BUCKET_BITS)
#define CL_MSG_QNAME_MASK         (CL_MSG_QNAME_BUCKETS - 1)

static struct hashStruct *ppMsgQNameHashTable[CL_MSG_QNAME_BUCKETS];


static __inline__ ClUint32T clMsgQNameHash(const ClNameT *pQName)
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

ClBoolT clMsgQNameEntryExists(const ClNameT *pQName, ClMsgQueueRecordT **ppQNameEntry)
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



static ClRcT clMsgQEntryAdd(ClNameT *pName, ClIocPhysicalAddressT *pCompAddr, SaMsgQueueHandleT queueHandle, ClMsgQueueRecordT **ppMsgQEntry)
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

    memcpy(&pTemp->qName, pName, sizeof(ClNameT));
    pTemp->compAddr = *pCompAddr;
    pTemp->qHandle = queueHandle;

    rc = clMsgQNameEntryAdd(pTemp);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ADD", "Failed to add the Queue Entry. error code [0x%x].", rc);
        goto error_out_1;
    }

    rc = clMsgNodeQEntryAdd(pTemp);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ADD", "Failed to add to the Node Queue Table. error code [0x%x].", rc);
        goto error_out_2;
    }

    CL_LIST_HEAD_INIT(&pTemp->groupList);

    if(ppMsgQEntry != NULL)
        *ppMsgQEntry = pTemp;

    clLogDebug("QUE", "ADD", "Added Queue [%.*s] by component [0x%x:0x%x].", 
            pTemp->qName.length, pTemp->qName.value, pTemp->compAddr.nodeAddress, pTemp->compAddr.portId);

    gClNumMsgQueues++;
    goto out;

error_out_2:
    clMsgQNameEntryDel(pTemp);
error_out_1:
    clHeapFree(pTemp);
error_out:
out:
    return rc;
}


static ClRcT clMsgQEntryDel(ClNameT *pQName)
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pTemp;

    if(clMsgQNameEntryExists(pQName, &pTemp) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogMultiline(CL_LOG_SEV_ERROR, "QUE", "DEL", "Queue [%.*s] does not exist. error code [0x%x].\n"
                "But returning [Success] as the out come of the operation is going to be the same.",
                pQName->length, pQName->value, rc);
        rc = CL_OK;
        goto out;
    }

    clMsgDeleteAllGroupsOfQueue(pTemp);
    
    clMsgNodeQEntryDel(pTemp);

    clMsgQNameEntryDel(pTemp);

    gClNumMsgQueues--;

    clLogDebug("QUE", "DEL", "Deleted queue [%.*s] of component [0x%x:0x%x].", 
            pTemp->qName.length, pTemp->qName.value, pTemp->compAddr.nodeAddress, pTemp->compAddr.portId);

    clHeapFree(pTemp);

out:
    return rc;
}


static ClRcT clMsgQEntryUpd(ClNameT *pQName, ClIocPhysicalAddressT *pCompAddr, ClIocPhysicalAddressT *pNewOwner)
{
    ClRcT rc = CL_OK;
    ClMsgQueueRecordT *pTemp;

    if(clMsgQNameEntryExists(pQName, &pTemp) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_DOESNT_EXIST);
        clLogMultiline(CL_LOG_SEV_ERROR, "QUE", "UPD", "Queue [%.*s] does not exist. error code [0x%x].",
                pQName->length, pQName->value, rc);
        goto out;
    }

    /* If the RESET and NEW-OWNER-ADDR packets are received out of sequence and the new owners address
     * is already updated when the RESET packet comes then the routine will return immediately without
     * RESETing the address. */

    if(!pCompAddr || !pCompAddr->nodeAddress || !pCompAddr->portId)
    { 
        /* it is in here since the RESET packet is received. */
        if(pNewOwner && pNewOwner->nodeAddress == pTemp->compAddr.nodeAddress && pNewOwner->portId == pTemp->compAddr.portId)
        {
            /* it is here because the the NEW-OWNER-ADDR is already updated. so just returning from here. */
            clLogDebug("QUE", "UPD", "Queue [%.*s] is already updated and having new address [0x%x:0x%x].",
                    pQName->length, pQName->value, pTemp->compAddr.nodeAddress, pTemp->compAddr.portId);
            goto out;
        }
    }

    clMsgNodeQEntryDel(pTemp);

    if(pCompAddr && pCompAddr->nodeAddress && pCompAddr->portId)
    {
        pTemp->compAddr = *pCompAddr;
        clLogDebug("QUE", "UPD", "Queue [%.*s] is moved to component [0x%x:0x%x].", 
                pQName->length, pQName->value, pTemp->compAddr.nodeAddress, pTemp->compAddr.portId);
    }
    else
    {
        clLogDebug("QUE", "UPD", "Queue [%.*s] is being moved from [0x%x:0x%x] component.", 
                pQName->length, pQName->value, pTemp->compAddr.nodeAddress, pTemp->compAddr.portId);
        pTemp->compAddr.nodeAddress = 0;
        pTemp->compAddr.portId = 0;
    }

    clMsgNodeQEntryAdd(pTemp);

out:
    return rc;
}


ClRcT VDECL_VER(clMsgQDatabaseUpdate, 4, 0, 0)(ClMsgSyncActionT syncupType, ClNameT *pQName, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwner)
{
    ClRcT rc;
    ClIocPhysicalAddressT srcAddr;

    CL_MSG_SERVER_INIT_CHECK;

    rc = clRmdSourceAddressGet(&srcAddr);
    if(rc != CL_OK)
    {
        clLogCritical("MSG", "DBU", "Failed to get the RMD source address. error code [0x%x].", rc);
        goto error_out;
    }
    if(srcAddr.nodeAddress == gClMyAspAddress)
        goto out;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            rc = clMsgQEntryAdd(pQName, pCompAddress, 0, NULL);
            break;
        case CL_MSG_DATA_DEL:
            rc = clMsgQEntryDel(pQName);
            break;
        case CL_MSG_DATA_UPD:
            rc = clMsgQEntryUpd(pQName, pCompAddress, pNewOwner);
    }
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
out:
    return rc;
}


ClRcT msgQueueInfoUpdateSend_internal(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue)
{
    ClRcT rc;

    /* Updating all the message server's database except my own. */
    rc = clMsgQUpdateSendThroughIdl(syncupType, pQName, pCompAddress, pNewOwnerAddr); 
    if(rc != CL_OK)
    {
        clLogError("QUE", "UPD", "Failed to update a new-queue addition or old-group deletion with other nodes. error code [0x%x].",rc);
        goto error_out;
    }

    /* Updating this message server's database. */
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            rc = clMsgQEntryAdd(pQName, pCompAddress, qHandle, pQueue);
            break;
        case CL_MSG_DATA_DEL:
            rc = clMsgQEntryDel(pQName);
            break;
        case CL_MSG_DATA_UPD:
            rc = clMsgQEntryUpd(pQName, pCompAddress, pNewOwnerAddr);
    }

error_out:
    return rc;
}


ClRcT clMsgQueueInfoUpdateSend(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle, ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue)
{
    ClRcT rc;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    rc = msgQueueInfoUpdateSend_internal(pQName, syncupType, qHandle, pCompAddress, pNewOwnerAddr, pQueue);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    return rc;
}

static ClRcT msgQueueInfoUpdateSend_SyncUnicastInternal(ClNameT *pQName, ClMsgSyncActionT syncupType, 
                                                        SaMsgQueueHandleT qHandle, ClIocPhysicalAddressT *pCompAddress, 
                                                        ClIocPhysicalAddressT *pNewOwnerAddr, ClMsgQueueRecordT **pQueue)
{
    ClRcT rc = CL_OK;
    ClUint32T i;
    ClUint32T numNodes = 0;
    ClIocNodeAddressT *pNodes = NULL;

    rc = clIocTotalNeighborEntryGet(&numNodes);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to get the number of neighbors in ASP system. error code [0x%x].", rc);
        goto error_out;
    }

    pNodes = (ClIocNodeAddressT *)clHeapAllocate(sizeof(ClIocNodeAddressT) * numNodes);
    if(pNodes == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QUE", "ALOC", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(ClIocNodeAddressT) * numNodes, rc);
        goto error_out;
    }

    rc = clIocNeighborListGet(&numNodes, pNodes);
    if(rc != CL_OK)
    {
        clLogError("QUE", "ALOC", "Failed to get the neighbor node addresses. error code [0x%x].", rc);
        goto error_out;
    }

    for(i = 0 ; i < numNodes; i++)
    {
        if(pNodes[i] == gClMyAspAddress) continue;

        rc = msgQUpdateSend_unicastSync_idl(pNodes[i], syncupType, pQName, pCompAddress, pNewOwnerAddr); 
        if(rc != CL_OK && rc != CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE) 
           && 
           rc != CL_IOC_RC(CL_IOC_ERR_HOST_UNREACHABLE) 
           && 
           CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_INITIALIZED)
        {
            clLogError("QUE", "ALOC", "Failed to inform about new queue [%.*s] to node [%d]. error code [0x%x].", pQName->length, pQName->value, pNodes[i], rc);
            clHeapFree(pNodes);
            goto error_out;
        }
    }

    clHeapFree(pNodes);
    rc = CL_OK;

error_out:
    return rc;
}

/*
 * Should be called without queuedb lock held.
 */
ClRcT msgQueueInfoUpdateSend_SyncUnicast(ClNameT *pQName, ClMsgSyncActionT syncupType, SaMsgQueueHandleT qHandle, 
                                         ClIocPhysicalAddressT *pCompAddress, ClIocPhysicalAddressT *pNewOwnerAddr, 
                                         ClMsgQueueRecordT **pQueue)
{
    ClRcT rc = CL_OK;

    rc = msgQueueInfoUpdateSend_SyncUnicastInternal(pQName, syncupType, qHandle, pCompAddress, pNewOwnerAddr, pQueue);
    if(rc != CL_OK)
        return rc;

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);

    /* Updating this message server's database. */
    switch(syncupType)
    {
        case CL_MSG_DATA_ADD:
            rc = clMsgQEntryAdd(pQName, pCompAddress, qHandle, pQueue);
            break;
        case CL_MSG_DATA_DEL:
            rc = clMsgQEntryDel(pQName);
            break;
        case CL_MSG_DATA_UPD:
            rc = clMsgQEntryUpd(pQName, pCompAddress, pNewOwnerAddr);
            break;
    }

    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
    return rc;
}


/*************************************************************************************/
/* Cleans up node/component specific information, which just died.           */
 

void clMsgNodeLeftCleanup(ClIocAddressT *pAddr)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgNodeQHash(pAddr->iocPhyAddress.nodeAddress);

    clLogDebug("NOD", "LEFT", "Node [0x%x] left the cluster. Cleaning up its info on this node.", 
            pAddr->iocPhyAddress.nodeAddress);

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    for(pTemp = ppMsgNodeQHashTable[key]; pTemp; pTemp = ppMsgNodeQHashTable[key])
    {
        ClMsgQueueRecordT *pQueueEntry = hashEntry(pTemp, ClMsgQueueRecordT, qNodeHash);
        if(pQueueEntry->compAddr.nodeAddress == pAddr->iocPhyAddress.nodeAddress)
        {
            clMsgQEntryDel(&pQueueEntry->qName);
            clLogTrace("NOD", "LEFT", "Cleaning up info of queue [%.*s].",
                    pQueueEntry->qName.length, pQueueEntry->qName.value);
        }
    }
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);
}
     

/*************************************************************************************/
/* The following code is to update the database of a node, which is just coming up.*/
 

ClRcT VDECL_VER(clMsgGetDatabases, 4, 0, 0)(ClUint8T **ppData, ClUint32T *pSize)
{
    ClRcT rc;
    ClRcT retCode;
    ClUint32T i;
    ClUint32T size;
    ClBufferHandleT message;
    ClMsgQueueSyncupRecordT tempQueue;
    ClMsgGroupSyncupRecordT tempGroup;
    register struct hashStruct *pTemp;
    ClMsgQueueRecordT *pMsgNodeQEntry;
    ClUint32T numEntries;
    register ClListHeadT *pGroupTemp;
    ClListHeadT *pQGroupHead;
    ClMsgQueuesGroupDetailsT *pGroupEntry;


    CL_MSG_SERVER_INIT_CHECK;

    clLogTrace("DB", "PACK", "Got a request for complete database info.");

    rc = clBufferCreate(&message);
    if(rc != CL_OK)
    {
        clLogCritical("DB", "PACK", "Failed to get the Database info. error code [0x%x].", rc);
        goto error_out;
    }

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);


    numEntries = htonl(gClNumMsgQueues);
    rc = clBufferNBytesWrite(message, (ClUint8T*)&numEntries, sizeof(numEntries));
    if(rc != CL_OK)
    {
        clLogCritical("DB", "PACK", "Failed to write the number of entries to buffer. error code [0x%x].", rc);
        goto check_other_nodes;
    }

    clLogDebug("DB", "PACK", "Number of open queues [%d]", gClNumMsgQueues);

    for(i = 0; i < CL_MSG_NODE_Q_BUCKETS ; i++)
    {
        if(ppMsgNodeQHashTable[i] == NULL)
            continue;

        for(pTemp = ppMsgNodeQHashTable[i]; pTemp; pTemp = pTemp->pNext)
        {
            pMsgNodeQEntry = hashEntry(pTemp, ClMsgQueueRecordT, qNodeHash);

            /* counting number of groups this queue has joined. */
            pQGroupHead = &pMsgNodeQEntry->groupList;
            numEntries = 0;
            CL_LIST_FOR_EACH(pGroupTemp, pQGroupHead)
                numEntries++;

            /* Packing the queue information. */
            memset(&tempQueue, 0, sizeof(tempQueue));
            
            memcpy(&tempQueue.qName, &pMsgNodeQEntry->qName, sizeof(ClNameT));
            tempQueue.compAddr.nodeAddress = pMsgNodeQEntry->compAddr.nodeAddress;
            tempQueue.compAddr.portId = pMsgNodeQEntry->compAddr.portId;
            tempQueue.numGroups = numEntries;

            rc = VDECL_VER(clXdrMarshallClMsgQueueSyncupRecordT, 4, 0, 0)((void*)&tempQueue, message, 0);
            if(rc != CL_OK)
            {
                clLogError("DB", "PACK", "Failed to write the queue info into a buffer. error code [0x%x].", rc);
                goto check_other_nodes;
            }

            clLogTrace("DB", "PACK", "Queue [%.*s] component [0x%x:0x%x]", 
                    tempQueue.qName.length, tempQueue.qName.value, tempQueue.compAddr.nodeAddress, tempQueue.compAddr.portId);

            /* Packing all the groups of the queue information. */
            CL_LIST_FOR_EACH(pGroupTemp, pQGroupHead)
            {
                pGroupEntry = CL_LIST_ENTRY(pGroupTemp, ClMsgQueuesGroupDetailsT, list);

                memset(&tempGroup, 0, sizeof(tempGroup));

                memcpy(&tempGroup.name, &pGroupEntry->pGroupDetails->name, sizeof(ClNameT));
                tempGroup.policy = pGroupEntry->pGroupDetails->policy;

                rc = VDECL_VER(clXdrMarshallClMsgGroupSyncupRecordT, 4, 0, 0)((void*)&tempGroup, message, 0); 
                if(rc != CL_OK)
                {
                    clLogError("DB", "PACK", "Failed to write the queue info into a buffer. error code [0x%x].", rc);
                    goto check_other_nodes;
                }

                clLogTrace("DB", "PACK", "\t- Group [%.*s] policy [%d].", 
                        tempGroup.name.length, tempGroup.name.value, tempGroup.policy);
            }
        }
    }

    /* Packing information about all the groups */
    rc = clMsgGroupDatabasePack(message);
    if(rc != CL_OK)
    {
        clLogError("DB", "PACK", "Failed to pack the group db. error code [0x%x].", rc);
        goto check_other_nodes;
    }

    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    rc = clBufferLengthGet(message, &size);
    if(rc != CL_OK)
    {
        clLogError("DB", "PACK", "Failed to get the length of the buffer data. error code [0x%x].", rc);
        goto error_out;
    }

    *ppData = (ClUint8T *)clHeapAllocate(size);
    if(*ppData == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("DB", "PACK", "Failed to allocate %d bytes of memory. error code [0x%x]", size, rc);
        goto error_out;
    }

    rc = clBufferNBytesRead(message, (ClUint8T*)*ppData, &size);
    if(rc != CL_OK)
    {
        clHeapFree(ppData);
        clLogError("DB", "PACK", "Failed to read the data from the buffer. error code [0x%x].",rc);
        goto error_out;
    }

    *pSize = size;
        
    retCode = clBufferDelete(&message);
    if(retCode != CL_OK)
        clLogError("DB", "PACK", "Failed to delete the buffer message. error code [0x%x].", retCode);

    clLogInfo("DB", "PACK", "Packed [%d] bytes of msg group data", size);

    return rc;

check_other_nodes:
    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

error_out:
    retCode = clBufferDelete(&message);
    if(retCode != CL_OK)
        clLogError("DB", "PACK", "Failed to delete the buffer message. error code [0x%x].", retCode);

    /*FIXME : Need to write a function to see which node can send this info, as this function has failed. */
    return rc;
}



ClRcT clMsgUpdateDatabases(ClUint8T *pTempData, ClUint32T size)
{
    ClRcT rc = CL_OK;
    ClUint32T i, j;
    ClMsgQueueRecordT *pMsgQEntry;
    ClMsgGroupRecordT *pQGroupEntry;
    ClMsgQueueSyncupRecordT tempQueue;
    ClMsgGroupSyncupRecordT tempGroup;
    ClUint32T numEntries = 0;
    ClUint32T tempSize;
    ClBufferHandleT message;

    clLogTrace("DB", "UPD", "Updating my own databases.");

    CL_OSAL_MUTEX_LOCK(&gClQueueDbLock);
    CL_OSAL_MUTEX_LOCK(&gClGroupDbLock);

    rc = clBufferCreate(&message);
    CL_ASSERT(rc == CL_OK);

    rc = clBufferNBytesWrite(message, pTempData, size);
    CL_ASSERT(rc == CL_OK);

    tempSize = sizeof(numEntries);
    rc = clBufferNBytesRead(message, (ClUint8T*)&numEntries, &tempSize);
    CL_ASSERT(rc == CL_OK);

    numEntries = ntohl(numEntries);

    for(j = 0; j < numEntries; j++)
    {
        memset(&tempQueue, 0, sizeof(tempQueue));

        rc = VDECL_VER(clXdrUnmarshallClMsgQueueSyncupRecordT, 4, 0, 0)(message, (void*)&tempQueue);
        if(rc != CL_OK)
        {
            clLogError("DB", "UPD", "Failed to unmarshall Queue info. error code [0x%x].",rc);
            CL_ASSERT(0);
        }

        if(clMsgQNameEntryExists(&tempQueue.qName, &pMsgQEntry) == CL_FALSE)
        {
            rc = clMsgQEntryAdd(&tempQueue.qName, &tempQueue.compAddr, 0, &pMsgQEntry);
            if(rc != CL_OK)
            {
                clLogError("DB", "UPD", "Failed to add a Queue [%.*s] to database. error code [0x%x].", tempQueue.qName.length, tempQueue.qName.value, rc);
                CL_ASSERT(0);
            }
        }

        clLogTrace("DB", "UPD", "Queue [%.*s] component [0x%x:0x%x].", 
                tempQueue.qName.length, tempQueue.qName.value, tempQueue.compAddr.nodeAddress, tempQueue.compAddr.portId);

        if(tempQueue.numGroups == 0)
            goto next_queue_info;


        for(i = 0 ; i < tempQueue.numGroups; i++)
        {
            memset(&tempGroup, 0, sizeof(tempGroup));

            rc = VDECL_VER(clXdrUnmarshallClMsgGroupSyncupRecordT, 4, 0, 0)(message, (void*)&tempGroup);
            if(rc != CL_OK)
            {
                clLogError("DB", "UPD", "Failed to unmarshall Queue's Group info. error code [0x%x].",rc);
                CL_ASSERT(0);
            }

            if(clMsgGroupEntryExists(&tempGroup.name, &pQGroupEntry) == CL_FALSE)
            {
                rc = clMsgNewGroupAddInternal(&tempGroup, &pQGroupEntry);
                if(rc != CL_OK)
                {
                    clLogError("DB", "UPD", "Failed to add an entry of a message group [%.*s]. error code [0x%x].", 
                            tempGroup.name.length, tempGroup.name.value, rc);
                    CL_ASSERT(0);
                }

                clMsgAddQueueToGroup(&pQGroupEntry->name, &pMsgQEntry->qName);
            }
            else if(clMsgDoesQExistInGroup(pQGroupEntry, &pMsgQEntry->qName, NULL, NULL) == CL_FALSE)
            {
                clMsgAddQueueToGroup(&pQGroupEntry->name, &pMsgQEntry->qName);
            }

            clLogTrace("DB", "UPD", "\t- Group [%.*s] policy [%d].", tempGroup.name.length, tempGroup.name.value, tempGroup.policy);
        }

next_queue_info:
        continue;
    }

    clMsgGroupDatabaseSyncup(message);

    CL_OSAL_MUTEX_UNLOCK(&gClGroupDbLock);
    CL_OSAL_MUTEX_UNLOCK(&gClQueueDbLock);

    clBufferDelete(&message);

    return rc;
}



/****************************************************************************************/

void clMsgQueueDatabaseShow(void)
{
    register struct hashStruct *pTemp;
    ClUint32T key;
    ClUint32T flag = 0;
#define LINE_LEN 76
    char msgLn[LINE_LEN+1] = {0};

    memset(&msgLn, '=', LINE_LEN);

    clOsalPrintf("\n\n%s\n", msgLn);
    clOsalPrintf("%-40s \t %6s \t %s\n", "QueueName", "Q-Hdl", "Node:Comp");
    clOsalPrintf("%s\n", msgLn);

    for(key = 0; key < CL_MSG_QNAME_BUCKETS ; key++)
    {
        for(pTemp = ppMsgQNameHashTable[key];pTemp; pTemp = pTemp->pNext)
        {
             ClMsgQueueRecordT *pMsgQEntry = hashEntry(pTemp, ClMsgQueueRecordT, qNameHash);
             clOsalPrintf("%-40s \t %6llu \t 0x%x:0x%x\n",
                     /*pMsgQEntry->qName.length, */pMsgQEntry->qName.value,
                     pMsgQEntry->qHandle,
                     pMsgQEntry->compAddr.nodeAddress,
                     pMsgQEntry->compAddr.portId);
             flag = 1;
        }
    }
    if(flag == 0)
    {
        clOsalPrintf("No record present in the Message-Queue-Name's Database.\n");
    }
    clOsalPrintf("%s\n\n", msgLn);
}
