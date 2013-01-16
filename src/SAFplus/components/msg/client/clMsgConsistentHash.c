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
 * File        : clMsgConsistentHash.c
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include <clMsgApiExt.h>
#include <clMD5Api.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clMsgCommon.h>
#include <clMsgCkptData.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clMsgApi.h>
#include <clMsgCkptClient.h>

#define __MSG_QUEUE_GROUP_NODES (10)
#define __MSG_QUEUE_GROUP_HASHES_PER_NODE (100)
#define __MSG_QUEUE_GROUP_HASHES ( __MSG_QUEUE_GROUP_NODES * __MSG_QUEUE_GROUP_HASHES_PER_NODE )
#define __DIGEST_TO_HASH(digest, index) (\
( (digest)[3 + (index)*4] << 24 ) | \
( (digest)[2 + (index)*4] << 16 ) | \
( (digest)[1 + (index)*4] << 8 ) | \
( (digest)[0 + (index)*4] ) \
)

typedef struct
{
    ClInt32T nodeIndex;
    ClUint32T nodeHash;
} MsgNodeHashesT;

typedef struct
{
    struct hashStruct hash;
    ClNameT name;
    ClInt32T nodes;
    ClInt32T hashesPerNode;
    MsgNodeHashesT *groupNodeHashes;
}ClMsgGroupHashesT;


/********************************************************************/
/* Database to store consistent hash rings for MSG Queue Group      */
/********************************************************************/
#define CL_MSG_GROUP_HASH_BITS          (5)
#define CL_MSG_GROUP_HASH_BUCKETS       (1 << CL_MSG_GROUP_HASH_BITS)
#define CL_MSG_GROUP_HASH_MASK          (CL_MSG_GROUP_HASH_BUCKETS - 1)

static struct hashStruct *ppMsgQGroupHashTable[CL_MSG_GROUP_HASH_BUCKETS];

static __inline__ ClUint32T clMsgGroupHash(const ClNameT *pQGroupName)
{
    return (ClUint32T)((ClUint32T)pQGroupName->value[0] & CL_MSG_GROUP_HASH_MASK);
}

static ClRcT clMsgGroupEntryAdd(ClMsgGroupHashesT *pGroupHashEntry)
{
    ClUint32T key = clMsgGroupHash(&pGroupHashEntry->name);
    return hashAdd(ppMsgQGroupHashTable, key, &pGroupHashEntry->hash);
}

static __inline__ void clMsgGroupEntryDel(ClMsgGroupHashesT *pGroupHashEntry)
{
    hashDel(&pGroupHashEntry->hash);
}

ClBoolT clMsgGroupEntryExists(const ClNameT *pQGroupName, ClMsgGroupHashesT **ppQGroupEntry)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clMsgGroupHash(pQGroupName);

    for(pTemp = ppMsgQGroupHashTable[key];  pTemp; pTemp = pTemp->pNext)
    {
        ClMsgGroupHashesT *pMsgQGroupEntry = hashEntry(pTemp, ClMsgGroupHashesT, hash);
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
/********************************************************************/
/* END: Database to store consistent hash rings for MSG Queue Group */
/********************************************************************/

/***********************************************************************************************/

static int msgNodeHashSortCmp(const void *a, const void *b)
{
    const MsgNodeHashesT *hash1 = a;
    const MsgNodeHashesT *hash2 = b;
    if(hash1->nodeHash < hash2->nodeHash) return -1;
    if(hash1->nodeHash > hash2->nodeHash) return 1;
    return 0;
}

static ClRcT msgNodeHashesCreate(ClInt32T nodes, ClInt32T hashesPerNode, MsgNodeHashesT *nodeHashes)
{
    ClRcT rc = CL_OK;
    ClInt32T i, j;
    ClInt32T hashes = nodes * hashesPerNode;
    ClInt32T scaledHashesPerNode = 0;
    ClInt32T hashIndex = 0;

    if(hashes <= 0 || hashesPerNode < 4)
    {
        rc = CL_MSG_RC(CL_ERR_INVALID_PARAMETER);
        goto out;
    }

    /*
     * 4 keys per md5 digest of 16 bytes
     */
    scaledHashesPerNode = hashesPerNode >> 2;
    for(i = 0; i < nodes; ++i)
    {
        for(j = 0; j < scaledHashesPerNode; ++j)
        {
            char *key = NULL;
            unsigned char md5Digest[16];
            ClInt32T h;
            asprintf(&key, "%d-%d", i, j);
            clMD5Compute((unsigned char*)key, strlen(key), md5Digest);
            free(key);
            for(h = 0; h < 4; ++h)
            {
                /*
                 * hashIndex = i*hashesPerNode + j*4 + h
                 */
                nodeHashes[hashIndex].nodeHash = __DIGEST_TO_HASH(md5Digest, h);
                nodeHashes[hashIndex].nodeIndex = i;
                ++hashIndex;
            }
        }
    }

    CL_ASSERT(hashIndex <= hashes);
    qsort(nodeHashes, hashIndex, sizeof(*nodeHashes), msgNodeHashSortCmp);

out:
    return rc;
}

static ClRcT msgNodeHashesInit(ClInt32T nodes, ClInt32T hashesPerNode,
                                    MsgNodeHashesT **ppNodeHashes)
{
    ClRcT rc = CL_OK;
    MsgNodeHashesT *nodeHashes = NULL;
    ClInt32T hashes = nodes * hashesPerNode;
    *ppNodeHashes = NULL;

    if(!hashes)
    {
        nodes = __MSG_QUEUE_GROUP_NODES;
        hashesPerNode = __MSG_QUEUE_GROUP_HASHES_PER_NODE;
        hashes = __MSG_QUEUE_GROUP_HASHES;
    }

    nodeHashes = clHeapCalloc(hashes, sizeof(*nodeHashes));
    if(nodeHashes == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QGH", "INIT", "Failed to allocate %u bytes of memory. error code [0x%x].",
                (ClUint32T) sizeof(MsgNodeHashesT), rc);
        goto error_out;
    }

    rc = msgNodeHashesCreate(nodes, hashesPerNode, nodeHashes);
    if (rc != CL_OK)
    {
        clLogError("QGH", "INIT", "Failed to create node hashes. error code [0x%x].", rc);
        clHeapFree(nodeHashes);
        goto error_out;
    }

    *ppNodeHashes = nodeHashes;

error_out:
    return rc;
}

static ClUint32T msgNodeHashesKeyToHash(unsigned char *key, ClInt32T keylen)
{
    unsigned char digest[16];
    clMD5Compute(key, keylen, digest);
    return __DIGEST_TO_HASH(digest, 0);
}

static MsgNodeHashesT *msgNodeHashesGet(unsigned char *key, ClInt32T keylen,
                                        MsgNodeHashesT *nodeHashes, ClInt32T numHashes)
{
    ClUint32T hash = msgNodeHashesKeyToHash(key, keylen);
    ClInt32T left = 0;
    ClInt32T right = numHashes - 1;
    ClInt32T mid = 0, mid2 = 0;
    ClInt32T fallbackIndex = 0; /* fall to the first index when out of range*/
    // printf("Hash for key [%.*s] = [%#x]\n", keylen, key, hash);
    while(left <= right)
    {
        mid = (left + right) >> 1;
        mid2 = (mid > 0) ? mid - 1 : 0;

        if(mid == mid2)
            return &nodeHashes[fallbackIndex];

        if(hash <= nodeHashes[mid].nodeHash)
        {
            if(hash == nodeHashes[mid].nodeHash)
                return &nodeHashes[mid];
            if(hash > nodeHashes[mid2].nodeHash)
                return &nodeHashes[mid];
            right = mid-1;
        }
        else
        {
            left = mid+1;
        }
    }
    return &nodeHashes[fallbackIndex];
}

/*
 * Initialize a consistent hash ring for a MSG queue group
 * and allocate a MsgQueueGroupHashesT to store the hash for receiver lookup
 */
ClRcT clMsgQueueGroupHashInit(const SaNameT *group, ClInt32T nodes, ClInt32T hashesPerNode)
{
    ClRcT rc =  CL_OK;
    ClMsgGroupHashesT *pGroupEntry;

    if(clMsgGroupEntryExists((ClNameT *)group, NULL) == CL_TRUE)
    {
        rc = CL_MSG_RC(CL_ERR_ALREADY_EXIST);
        clLogError("QGH", "INIT", "Consistent hash ring for Message Queue Group with name [%.*s] already exists. error code [0x%x].", group->length, group->value, rc);
        goto error_out;
    }

    pGroupEntry = (ClMsgGroupHashesT *)clHeapAllocate(sizeof(ClMsgGroupHashesT));
    if(pGroupEntry == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QGH", "INIT", "Failed to allocate %u bytes of memory. error code [0x%x].",
                (ClUint32T) sizeof(ClMsgGroupHashesT), rc);
        goto error_out;
    }

    clNameCopy(&pGroupEntry->name, (ClNameT *) group);
    pGroupEntry->nodes = nodes;
    pGroupEntry->hashesPerNode = hashesPerNode;

    rc = msgNodeHashesInit(nodes, hashesPerNode, &pGroupEntry->groupNodeHashes);
    if (rc != CL_OK)
    {
        clLogError("QGH", "INIT", "Failed to initialize consistent hash. error code [0x%x].",
                  rc);
        clHeapFree(pGroupEntry);
        goto error_out;
    }

    rc = clMsgGroupEntryAdd(pGroupEntry);
    if(rc != CL_OK)
    {
        clLogError("QGH", "INIT", "Failed to add entry to the group hash table. error code [0x%x].", rc);
        clHeapFree(pGroupEntry);
        goto error_out;
    }
error_out:
    return rc;
}

/*
 * Free memory allocated for consistent hash of a group
 */
ClRcT clMsgQueueGroupHashFinalize(const SaNameT *group)
{
    ClRcT rc = CL_OK;
    ClMsgGroupHashesT *pGroupEntry;

    if(clMsgGroupEntryExists((ClNameT *)group, &pGroupEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogWarning("QGH", "DEL", "Consistent hashes for group [%.*s] is not initialized. error code [0x%x]", group->length, group->value, rc);
        goto error_out;
    }

    if(pGroupEntry->groupNodeHashes)
        clHeapFree(pGroupEntry->groupNodeHashes);

    clMsgGroupEntryDel(pGroupEntry);
    clHeapFree(pGroupEntry);

error_out:
    return rc;
}

static SaAisErrorT clMsgQueueGroupSendWithKey(ClBoolT isSync,
                                       SaMsgHandleT msgHandle,
                                       SaInvocationT invocation,
                                       const SaNameT *group, SaMsgMessageT *message,
                                       ClCharT *key, ClInt32T keyLen,
                                       SaTimeT timeout,
                                       SaMsgAckFlagsT ackFlags)
{
    ClRcT rc = CL_OK;
    ClMsgGroupHashesT *pGroupEntry = NULL;
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    MsgNodeHashesT *queueGroupHash = NULL;
    ClInt32T hashes = __MSG_QUEUE_GROUP_HASHES;

    if(clMsgGroupEntryExists((ClNameT *)group, &pGroupEntry) == CL_FALSE)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogError("QGH", "SEND", "Consistent hashes for group [%.*s] is not initialized. error code [0x%x]", group->length, group->value, rc);
        goto error_out;
    }

    rc = clMsgQGroupCkptDataGet((ClNameT*)group, &qGroupData);
    if (rc != CL_OK)
    {
        clLogError("QGH", "SEND", "Group [%.*s] does not exist. error code [0x%x]", group->length, group->value, rc);
        goto error_out;
    }

    if (qGroupData.numberOfQueues == 0)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_EXIST);
        clLogError("QGH", "SEND", "There is no MSG queue in Group [%.*s]. error code [0x%x]", group->length, group->value, rc);
        goto error_out;
    }

    hashes = CL_MIN(pGroupEntry->nodes, qGroupData.numberOfQueues) * pGroupEntry->hashesPerNode;
    queueGroupHash = msgNodeHashesGet((unsigned char*)key, keyLen,
                                          pGroupEntry->groupNodeHashes, hashes);

    /* Handle incorrect index */
    if ((queueGroupHash->nodeIndex < 0) || (queueGroupHash->nodeIndex >= qGroupData.numberOfQueues))
    {
        rc = CL_MSG_RC(CL_ERR_BAD_OPERATION);
        clLogError("QGH", "SEND", "Incorrect node index [%d]. error code [0x%x]", queueGroupHash->nodeIndex, rc);
        goto error_out;
    }

    /* Find target based on the consistent hash ring */
    ClNameT *pQName = &qGroupData.pQueueList[queueGroupHash->nodeIndex];

    if (isSync)
        return saMsgMessageSend(msgHandle, (SaNameT *)pQName, message, timeout);
    else
        return saMsgMessageSendAsync(msgHandle, invocation, (SaNameT *)pQName, message, ackFlags);

error_out:
    return CL_MSG_SA_RC(rc);
}

/*
 * Lookup and message to the receiver in the queue group that map with the key
 */
SaAisErrorT clMsgQueueGroupSendWithKeySynch(SaMsgHandleT msgHandle,
                                                   const SaNameT *group, SaMsgMessageT *message,
                                                   ClCharT *key, ClInt32T keyLen,
                                                   SaTimeT timeout)
{
    return clMsgQueueGroupSendWithKey(CL_TRUE, msgHandle, 0, group, message, key, keyLen, timeout, 0);
}
SaAisErrorT clMsgQueueGroupSendWithKeyAsync(SaMsgHandleT msgHandle,
                                                   SaInvocationT invocation,
                                                   const SaNameT *group, SaMsgMessageT *message,
                                                   ClCharT *key, ClInt32T keyLen,
                                                   SaMsgAckFlagsT ackFlags)
{
    return clMsgQueueGroupSendWithKey(CL_FALSE, msgHandle, invocation, group, message, key, keyLen, 0, ackFlags);
}

