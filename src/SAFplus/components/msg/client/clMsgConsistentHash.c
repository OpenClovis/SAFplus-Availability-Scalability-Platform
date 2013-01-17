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
#include <clMsgCommon.h>
#include <clMsgCkptData.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clMsgApi.h>
#include <clMsgCkptClient.h>
#include <clMsgConsistentHash.h>

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
    ClInt32T nodes;
    ClInt32T hashesPerNode;
    MsgNodeHashesT *groupNodeHashes;
}ClMsgGroupHashesT;

static ClMsgGroupHashesT *gGroupHashes = NULL;

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
    ClCharT key[CL_MAX_NAME_LENGTH * 2];

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
            unsigned char md5Digest[16];
            ClInt32T h;
            snprintf(key, sizeof(key), "%d-%d", i, j);
            clMD5Compute((unsigned char*)key, strlen(key), md5Digest);
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
ClRcT clMsgQueueGroupHashInit(ClInt32T nodes, ClInt32T hashesPerNode)
{
    ClRcT rc =  CL_OK;
    ClMsgGroupHashesT *pGroupHashes;

    pGroupHashes = (ClMsgGroupHashesT *)clHeapAllocate(sizeof(ClMsgGroupHashesT));
    if(pGroupHashes == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NO_MEMORY);
        clLogError("QGH", "INIT", "Failed to allocate %u bytes of memory. error code [0x%x].",
                (ClUint32T) sizeof(ClMsgGroupHashesT), rc);
        goto error_out;
    }

    pGroupHashes->nodes = nodes;
    pGroupHashes->hashesPerNode = hashesPerNode;

    rc = msgNodeHashesInit(nodes, hashesPerNode, &pGroupHashes->groupNodeHashes);
    if (rc != CL_OK)
    {
        clLogError("QGH", "INIT", "Failed to initialize consistent hash. error code [0x%x].",
                  rc);
        clHeapFree(pGroupHashes);
        goto error_out;
    }

    gGroupHashes = pGroupHashes;
error_out:
    return rc;
}

/*
 * Free memory allocated for consistent hash of a group
 */
ClRcT clMsgQueueGroupHashFinalize()
{
    ClRcT rc = CL_OK;
    ClMsgGroupHashesT *pGroupHashes = gGroupHashes;

    if(pGroupHashes == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogWarning("QGH", "DEL", "Consistent hash is not initialized. error code [0x%x]", rc);
        goto error_out;
    }

    if(pGroupHashes->groupNodeHashes)
        clHeapFree(pGroupHashes->groupNodeHashes);

    clHeapFree(pGroupHashes);
    gGroupHashes = NULL;

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
    ClMsgGroupHashesT *pGroupHashes = gGroupHashes;
    ClMsgQGroupCkptDataT qGroupData = {{0}};
    MsgNodeHashesT *queueGroupHash = NULL;
    ClInt32T hashes = __MSG_QUEUE_GROUP_HASHES;

    if(pGroupHashes == NULL)
    {
        rc = CL_MSG_RC(CL_ERR_NOT_INITIALIZED);
        clLogWarning("QGH", "DEL", "Consistent hash is not initialized. error code [0x%x]", rc);
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

    hashes = CL_MIN(pGroupHashes->nodes, qGroupData.numberOfQueues) * pGroupHashes->hashesPerNode;
    queueGroupHash = msgNodeHashesGet((unsigned char*)key, keyLen,
                                      pGroupHashes->groupNodeHashes, hashes);

    /* Handle incorrect index */
    if ((queueGroupHash->nodeIndex < 0) || (queueGroupHash->nodeIndex >= qGroupData.numberOfQueues))
    {
        rc = CL_MSG_RC(CL_ERR_BAD_OPERATION);
        clLogError("QGH", "SEND", "Incorrect node index [%d] with key [%.*s] and number of nodes [%d]. error code [0x%x]",
                                   queueGroupHash->nodeIndex,
                                   keyLen, key,
                                   qGroupData.numberOfQueues, rc);
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

