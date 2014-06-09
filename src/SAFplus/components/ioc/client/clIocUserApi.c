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
/*
 * 
 *   copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
 * 
 *   The source code for this program is not published or otherwise divested
 *   of its trade secrets, irrespective of what has been deposited with  the
 *   U.S. Copyright office.
 * 
 *   No part of the source code  for this  program may  be use,  reproduced,
 *   modified, transmitted, transcribed, stored  in a retrieval  system,  or
 *   translated, in any form or by  any  means,  without  the prior  written
 *   permission of OpenClovis Inc
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : ioc                                                           
 * File        : clIocUserApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains all the API definations. These functions do the
 * "ioctl" calls to the respective kernel functions, which are present in the
 * IOC kernel module.
 *
 *
 *****************************************************************************/

#include <clCommon.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <errno.h>

#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clEoIpi.h>
#include <clHash.h>
#include <clList.h>
#include <clCksmApi.h>
#include <clRbTree.h>
#include <clJobQueue.h>
#include <clVersionApi.h>
#include <clIocManagementApi.h>
#include <clIocErrors.h>
#include <clIocIpi.h>
#include <clIocParseConfig.h>
#include <clIocMaster.h>
#include <clIocNeighComps.h>
#include <clIocUserApi.h>
#include <clIocSetup.h>
#include <clIocGeneral.h>
#include <clLeakyBucket.h>
#include <clNodeCache.h>
#include <clNetwork.h>
#include <clTimeServer.h>
#include <clTransport.h>
#include <clIocReliableLossList.h>

#ifdef CL_IOC_COMPRESSION

#include <zlib.h>
#define ZLIB_TIME(t) do {                       \
    t = clOsalStopWatchTimeGet();               \
}while(0)                                       \

#endif

extern ClBoolT gIsNodeRepresentative;
extern ClUint32T clEoWithOutCpm;
/*
 * Global leaky bucket overridden on a per-process basis.
 */
ClLeakyBucketHandleT gClLeakyBucket;

#define RELIABLE_IOC
#define CL_IOC_BLOCK_SIZE (1024)
#define CL_IOC_ALIGN_VAL(v,align) (((v) + (align) - 1) & ~((align)-1))
#define CL_IOC_MCAST_VALID(pMcast) (CL_IOC_ADDRESS_TYPE_GET(pMcast) == CL_IOC_MULTICAST_ADDRESS_TYPE)
#define IOC_PHY_CMP(phy1,phy2) (((phy1)->nodeAddress == (phy2)->nodeAddress) ? \
        (ClInt32T) ((phy1)->portId - (phy2)->portId) :                         \
        (ClInt32T)((phy1)->nodeAddress - (phy2)->nodeAddress))

#ifdef CL_IOC_COMPRESSION
#define CL_IOC_COMPRESSION_BOUNDARY (0x1000U) /*mark 4K to be the compression limit*/
#endif

/*
 * Update the reassembly timer after receiving the below length for a reassembly node.
 * Should be a power of 2. 
 */
#define CL_IOC_REASSEMBLY_ADAPTIVE_LENGTH (64 << 20)  /* default at 64 mb */
#define CL_IOC_MAX_PAYLOAD_BITS (16) /* 64k default */
#define CL_IOC_REASSEMBLY_FRAGMENTS ( CL_IOC_REASSEMBLY_ADAPTIVE_LENGTH >> CL_IOC_MAX_PAYLOAD_BITS )
#define CL_IOC_REASSEMBLY_FRAGMENTS_MASK (CL_IOC_REASSEMBLY_FRAGMENTS-1)

/*
 * To use this leaky bucket volume, you are effectively looking at very large traffic or checkpoints of size
 * over 200 megs. Also this entails to increase the /proc/sys/net/core/rmem_{default,max} values to 500 megs or
 * above to avoid ioc skbuffs being dropped by the kernel from queueing on hitting the backlog queue limit.
 */
#define CL_LEAKY_BUCKET_DEFAULT_VOL (50 << 20)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE (25 << 20)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL (500)

extern ClUint32T clAspLocalId;

ClIocNodeAddressT gIocLocalBladeAddress;

#ifdef CL_IOC_COMPRESSION
static ClUint32T gIocCompressionBoundary;
#endif

static ClUint32T gIocInit = CL_FALSE;
static ClOsalMutexIdT gClIocFragMutex;
static ClUint32T currFragId;
static ClIocUserObjectT userObj;
static ClTimerTimeOutT userReassemblyTimerExpiry = { 0 };
ClBoolT gClIocTrafficShaper;
static ClBoolT gClIocReplicast;
static ClUint32T test = 0;

typedef struct {
    ClIocFragHeaderT *header;
    ClBufferHandleT fragPkt;
} ClIocFragNodeT;

static ClRcT clIocReplicastGet(ClIocPortT portId, ClIocAddressT **pAddressList,
        ClUint32T *pNumEntries);

#define IOC_REASSEMBLE_HASH_BITS (12)
#define IOC_REASSEMBLE_HASH_SIZE ( 1 << IOC_REASSEMBLE_HASH_BITS)
#define IOC_REASSEMBLE_HASH_MASK (IOC_REASSEMBLE_HASH_SIZE - 1)

static ClOsalMutexT iocReassembleLock;
static ClOsalMutexT iocAcklock;
static ClJobQueueT iocFragmentJobQueue;
static struct hashStruct *iocReassembleHashTable[IOC_REASSEMBLE_HASH_SIZE];
static ClUint64T iocReassembleCurrentTimerId;
static ClUint64T iocSenderCurrentTimerId = 0;
typedef ClIocReassemblyKeyT ClIocReassembleKeyT;

typedef struct ClIocReassembleTimerKey {
    ClIocReassembleKeyT key;
    ClUint64T timerId;
    ClTimerHandleT reassembleTimer;
} ClIocReassembleTimerKeyT;

typedef struct ClIocReassembleNode {
    ClRbTreeRootT reassembleTree; /*reassembly tree*/
    struct hashStruct hash; /*hash linkage*/
    ClUint32T currentLength;
    ClUint32T expectedLength;
    ClUint32T numFragments; /* number of fragments received*/
    ClIocReassembleTimerKeyT *timerKey;
#ifdef RELIABLE_IOC
    ClUint32T ackSync;
    ClUint32T lossTotal;
    ClUint32T messageId;
    ClUint32T lastAckSync;
    ClBoolT isReliable;
    ClFragmentListHeadT *receiverLossList;
    ClIocPortT commPort;
    ClIocAddressT srcAddress;
    ClTimerHandleT TimerACKHdl;

#endif
} ClIocReassembleNodeT;

typedef struct ClIocFragmentNode {
    ClRbTreeT tree;
    ClUint32T fragOffset;
    ClUint32T fragLength;
    ClUint8T *fragBuffer;
    ClUint32T fragId;
} ClIocFragmentNodeT;

typedef struct ClIocFragmentJob {
    ClCharT xportType[CL_MAX_NAME_LENGTH];
    ClUint8T *buffer;
    ClUint32T length;
    ClIocPortT portId;
    ClIocFragHeaderT fragHeader;
} ClIocFragmentJobT;

typedef struct ClIocFragmentPool {
    ClListHeadT list;
    ClUint8T *buffer;
} ClIocFragmentPoolT;

static CL_LIST_HEAD_DECLARE(iocFragmentPool);
static ClOsalMutexT iocFragmentPoolLock;
static ClUint32T iocFragmentPoolLen;
static ClUint32T iocFragmentPoolSize = 1024 * 1024;
static ClUint32T iocFragmentPoolEntries;
static ClUint32T iocFragmentPoolLimit;

typedef struct ClIocLogicalAddressCtrl {
    ClIocCommPortT *pIocCommPort;
    ClIocLogicalAddressT logicalAddress;
    ClUint32T haState;
    ClListHeadT list;
} ClIocLogicalAddressCtrlT;

typedef struct ClIocMcast {
    ClIocMulticastAddressT mcastAddress;
    struct hashStruct hash;
    ClListHeadT portList;
} ClIocMcastT;

/*port list for the mcast*/
typedef struct ClIocMcastPort {
    ClListHeadT listMcast;
    ClListHeadT listPort;
    ClIocCommPortT *pIocCommPort;
    ClIocMcastT *pMcast;
} ClIocMcastPortT;

typedef struct ClIocNeighborList {
    ClListHeadT neighborList;
    ClOsalMutexT neighborMutex;
    ClUint32T numEntries;
} ClIocNeighborListT;

typedef struct ClIocNeighbor {
    ClListHeadT list;
    ClIocNodeAddressT address;
    ClUint32T status;
} ClIocNeighborT;

/****************
 *  RELIABLE IOC
 ****************/



#ifdef RELIABLE_IOC
static ClOsalMutexT iocSenderLock;
ClUint32T clTriggerFakeDrop;
ClUint32T clFakeDropCount;
#define CL_RETRANMISSION_PRIORITY CL_IOC_HIGH_PRIORITY
#define CL_RETRANMISSION_TIMEOUT 200
static ClTimerTimeOutT ackTimerExpiry = { 0 };
static ClTimerTimeOutT nakTimerExpiry = { 0 };
static struct hashStruct *iocSenderBufferHashTable[IOC_REASSEMBLE_HASH_SIZE];
ClIocNodeAddressT gIocLocalBladeAddress;
static ClTimerTimeOutT userTTLTimerExpiry = { 0 };
static ClTimerTimeOutT userResendTimerExpiry = { 0 };
ClInt32T iocFragmentIdCmp(ClUint32T fragId1, ClUint32T fragId2);
ClRcT receiverDropMsgCallback(ClIocAddressT *srcAddress, ClUint32T messageId,
        ClIocPortT portId);
typedef ClIocReassemblyKeyT ClIocReliableBufferKeyT;
ClIocNodeAddressT lastMessageNode = 0;
ClIocNodeAddressT lastMessageId = 0;

static CL_LIST_HEAD_DECLARE(iocSenderFragmentPool);
static ClOsalMutexT iocSenderFragmentPoolLock;
static ClUint32T iocSenderFragmentPoolLen;
static ClUint32T iocSenderFragmentPoolSize = 1024 * 1024;
static ClUint32T iocSenderFragmentPoolEntries;
static ClUint32T iocSenderFragmentPoolLimit;

typedef struct ClIocTTLTimerKey {
    ClIocReliableBufferKeyT key;
    ClUint64T timerId;
    ClTimerHandleT ttlTimer;
} ClIocTTLTimerKeyT;

typedef struct ClIocReliableFragmentSenderNode {
    ClRbTreeT tree;
    ClUint32T fragmentId;
    ClUint32T fragmentSize; //payload + size of userFragheader
    ClBufferHandleT buffer;

} ClIocReliableFragmentSenderNodeT;

typedef struct ClIocReliableSenderNode {
    ClRbTreeRootT senderBufferTree; /*reassembly tree*/
    struct hashStruct hash; /*hash linkage*/
    struct ClFragmentListHeadT *sendLossList;
    ClUint32T numFragments; /* total fragments to send */
    ClUint32T ackSync;
    ClUint32T totalLost;
    ClUint32T currentFragment;
    ClIocTTLTimerKeyT *ttlTimerKey;
    ClIocAddressT destAddress;
    ClTimerHandleT resendTimer;
    ClBufferHandleT buffer;
    ClUint32T messageLength;
    ClIocCommPortHandleT commPortHandle;

} ClIocReliableSenderNodeT;
static void senderBufferAddFragment(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, struct iovec *target, ClUint32T targetVectors,
        ClIocAddressT *destAddress, ClUint32T fragmentSize,
        ClUint32T fragmentId, ClUint32T totalFragment);
static ClRcT senderBufferRetranmission(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, ClUint32T fragmentId, ClIocAddressT *destAddress,
        ClCharT *xportType, ClBoolT proxy);
ClIocReliableSenderNodeT * getSenderBufferNode(ClUint32T messageId,
        ClIocAddressT *destAddress, ClIocPortT portId);
ClIocReliableSenderNodeT *__iocSenderBufferNodeFind(
        ClIocReliableBufferKeyT *key, ClUint64T timerId);
ClRcT senderBufferACKCallBack(ClUint32T messageId, ClIocAddressT *destAddress,
        ClUint32T fragmentId, ClIocPortT portId);
ClRcT senderBufferNAKCallBack(ClUint32T messageId, ClIocAddressT *destAddress,
        ClIocPortT portId, ClUint32T* losslist, ClUint32T size);
static ClRcT __iocSenderFragmentPoolInitialize(void);
static void __iocSenderFragmentPoolFinalize(void);
ClRcT senderPiggyBackCallback(ClIocFragmentJobT *fragmentJob);
static void senderBufferAddMessage(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, ClBufferHandleT pBuffer, ClUint32T length,
        ClIocAddressT *destAddress, ClUint32T totalFragment);
ClRcT receiverAckSend(ClIocCommPortT *commPort, ClIocAddressT *dstAddress,
        ClUint32T messageId, ClUint32T fragmentId);
static ClRcT receiverACKTrigger(void* key);
ClRcT receiverLastACKTrigger(ClIocReassembleNodeT *node);
static ClRcT receiverNAKTrigger(void* key);
ClRcT receiverNAKTriggerDirect(ClUint32T messageId,ClUint32T fragmentBegin,ClUint32T fragmentEnd,ClIocAddressT* srcAddress,ClIocPortT commPort);


#endif
static ClIocNeighborListT gClIocNeighborList = { .neighborList =
        CL_LIST_HEAD_INITIALIZER(gClIocNeighborList.neighborList), };

#define CL_IOC_MICRO_SLEEP_INTERVAL 1000*2 /* 10 milli second */

#define CL_IOC_DUPLICATE_NODE_TIMEOUT (100)

#define CL_IOC_PORT_EXIT_MESSAGE "QUIT"

#define NULL_CHECK(X)                               \
    do {                                            \
        if((X) == NULL)                             \
        return CL_IOC_RC(CL_ERR_NULL_POINTER);  \
    } while(0)

#define CL_IOC_PORT_BITS (10)

#define CL_IOC_PORT_BUCKETS ( 1 << CL_IOC_PORT_BITS )

#define CL_IOC_PORT_MASK (CL_IOC_PORT_BUCKETS-1)

#define CL_IOC_COMP_BITS (2)

#define CL_IOC_COMP_BUCKETS ( 1 << CL_IOC_COMP_BITS )

#define CL_IOC_COMP_MASK (CL_IOC_COMP_BUCKETS-1)

#define CL_IOC_MCAST_BITS (10)

#define CL_IOC_MCAST_BUCKETS ( 1 << CL_IOC_MCAST_BITS ) 

#define CL_IOC_MCAST_MASK (CL_IOC_MCAST_BUCKETS - 1)

static struct hashStruct *ppIocPortHashTable[CL_IOC_PORT_BUCKETS];
static struct hashStruct *ppIocCompHashTable[CL_IOC_COMP_BUCKETS];
static struct hashStruct *ppIocMcastHashTable[CL_IOC_MCAST_BUCKETS];
static ClOsalMutexT gClIocPortMutex;
static ClOsalMutexT gClIocCompMutex;
static ClOsalMutexT gClIocMcastMutex;

ClRcT clIocNeighborScan(void);
static ClRcT clIocNeighborAdd(ClIocNodeAddressT address, ClUint32T status);
static ClIocNeighborT *clIocNeighborFind(ClIocNodeAddressT address);

/*  
 * When running with asp modified ioc supporting 64k.
 */
#undef CL_IOC_PACKET_SIZE
#define CL_IOC_PACKET_SIZE (64000)

#define longTimeDiff(tm1, tm2) ((tm2.tsSec - tm1.tsSec) * 1000 + (tm2.tsMilliSec - tm1.tsMilliSec))

static ClUint32T gClMaxPayloadSize = 64000;

static ClRcT internalSendSlow(ClIocCommPortT *pIocCommPort,
        ClBufferHandleT message, ClUint32T tempPriority,
        ClIocAddressT *pIocAddress, ClUint32T *pTimeout, ClCharT *xportType,
        ClBoolT proxy);

static ClRcT internalSendSlowReplicast(ClIocCommPortT *pIocCommPort,
        ClBufferHandleT message, ClUint32T tempPriority, ClUint32T *pTimeout,
        ClIocAddressT *replicastList, ClUint32T numReplicasts,
        ClIocHeaderT *userHeader, ClBoolT proxy);

static ClRcT internalSend(ClIocCommPortT *pIocCommPort, struct iovec *target,
        ClUint32T targetVectors, ClUint32T messageLen, ClUint32T tempPriority,
        ClIocAddressT *pIocAddress, ClUint32T *pTimeout, ClCharT *xportType,
        ClBoolT proxy);

static ClRcT internalSendReplicast(ClIocCommPortT *pIocCommPort,
        struct iovec *target, ClUint32T targetVectors, ClUint32T messageLen,
        ClUint32T tempPriority, ClUint32T *pTimeout,
        ClIocAddressT *replicastList, ClUint32T numReplicasts,
        ClIocFragHeaderT *userFragHeader, ClBoolT proxy);

static void __iocFragmentPoolPut(ClUint8T *pBuffer, ClUint32T len) {
    if (len != iocFragmentPoolLen) {
        clHeapFree(pBuffer);
        return;
    }
    if (!iocFragmentPoolLimit) {
        iocFragmentPoolLen = gClMaxPayloadSize;
        CL_ASSERT(iocFragmentPoolLen != 0);
        iocFragmentPoolLimit = iocFragmentPoolSize / iocFragmentPoolLen;
    }
    if (iocFragmentPoolEntries >= iocFragmentPoolLimit) {
        clHeapFree(pBuffer);
    } else {
        ClIocFragmentPoolT *pool = clHeapCalloc(1, sizeof(*pool));
        CL_ASSERT(pool != NULL);
        pool->buffer = pBuffer;
        clOsalMutexLock(&iocFragmentPoolLock);
        clListAddTail(&pool->list, &iocFragmentPool);
        ++iocFragmentPoolEntries;
        clOsalMutexUnlock(&iocFragmentPoolLock);
    }
}

static ClUint8T *__iocFragmentPoolGet(ClUint8T *pBuffer, ClUint32T len) {
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *head = NULL;
    ClUint8T *buffer = NULL;
    clOsalMutexLock(&iocFragmentPoolLock);
    if (len != iocFragmentPoolLen || CL_LIST_HEAD_EMPTY(&iocFragmentPool)) {
        clOsalMutexUnlock(&iocFragmentPoolLock);
        goto alloc;
    }
    head = iocFragmentPool.pNext;
    pool = CL_LIST_ENTRY(head, ClIocFragmentPoolT, list);
    clListDel(head);
    --iocFragmentPoolEntries;
    clLogTrace("IOC", "FRAG-POOL", "Got fragment of len [%d] from pool", len);
    clOsalMutexUnlock(&iocFragmentPoolLock);
    buffer = pool->buffer;
    clHeapFree(pool);
    return buffer;

    alloc: return (ClUint8T*) clHeapAllocate(len);
}

static ClRcT __iocFragmentPoolInitialize(void) {
    ClUint32T currentSize = 0;
    iocFragmentPoolLen = gClMaxPayloadSize;
    CL_ASSERT(iocFragmentPoolLen != 0);
    clOsalMutexInit(&iocFragmentPoolLock);
    while (currentSize + iocFragmentPoolLen < iocFragmentPoolSize) {
        ClIocFragmentPoolT *pool = clHeapCalloc(1, sizeof(*pool));
        ClUint8T *buffer = clHeapAllocate(iocFragmentPoolLen);
        CL_ASSERT(pool != NULL);
        CL_ASSERT(buffer != NULL);
        currentSize += iocFragmentPoolLen;
        pool->buffer = buffer;
        clListAddTail(&pool->list, &iocFragmentPool);
        ++iocFragmentPoolEntries;
        ++iocFragmentPoolLimit;
    }
    return CL_OK;
}

static void __iocFragmentPoolFinalize(void) {
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *iter = NULL;
    while (!CL_LIST_HEAD_EMPTY(&iocFragmentPool)) {
        iter = iocFragmentPool.pNext;
        pool = CL_LIST_ENTRY(iter, ClIocFragmentPoolT, list);
        clListDel(iter);
        if (pool->buffer)
            clHeapFree(pool->buffer);
        clHeapFree(pool);
    }
    iocFragmentPoolEntries = 0;
    iocFragmentPoolLimit = 0;
    clOsalMutexDestroy(&iocFragmentPoolLock);
}

static __inline__ ClUint32T clIocMcastHash(ClIocMulticastAddressT mcastAddress) {
    return (ClUint32T) ((ClUint32T) mcastAddress & CL_IOC_MCAST_MASK);
}

static ClRcT clIocMcastHashAdd(ClIocMcastT *pMcast) {
    ClUint32T key = clIocMcastHash(pMcast->mcastAddress);
    return hashAdd(ppIocMcastHashTable, key, &pMcast->hash);
}

static __inline__ ClRcT clIocMcastHashDel(ClIocMcastT *pMcast) {
    hashDel(&pMcast->hash);
    return CL_OK;
}

static ClIocMcastT *clIocGetMcast(ClIocMulticastAddressT mcastAddress) {
    register struct hashStruct *pTemp;
    ClUint32T key = clIocMcastHash(mcastAddress);
    for (pTemp = ppIocMcastHashTable[key]; pTemp; pTemp = pTemp->pNext) {
        ClIocMcastT *pMcast = hashEntry(pTemp,ClIocMcastT,hash);
        if (pMcast->mcastAddress == mcastAddress) {
            return pMcast;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clIocCompHash(ClUint32T compId) {
    return (compId & CL_IOC_COMP_MASK);
}

static ClRcT clIocCompHashAdd(ClIocCompT *pComp) {
    ClUint32T key = clIocCompHash(pComp->compId);
    return hashAdd(ppIocCompHashTable, key, &pComp->hash);
}

static __inline__ ClRcT clIocCompHashDel(ClIocCompT *pComp) {
    hashDel(&pComp->hash);
    return CL_OK;
}

static ClIocCompT *clIocGetComp(ClUint32T compId) {
    ClUint32T key = clIocCompHash(compId);
    register struct hashStruct *pTemp;
    for (pTemp = ppIocCompHashTable[key]; pTemp; pTemp = pTemp->pNext) {
        ClIocCompT *pComp = hashEntry(pTemp,ClIocCompT,hash);
        if (pComp->compId == compId) {
            return pComp;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clIocPortHash(ClIocPortT portId) {
    return (ClUint32T) (portId & CL_IOC_PORT_MASK);
}

static ClRcT clIocPortHashAdd(ClIocCommPortT *pIocCommPort) {
    ClUint32T key = clIocPortHash(pIocCommPort->portId);
    ClRcT rc;
    rc = hashAdd(ppIocPortHashTable, key, &pIocCommPort->hash);
    return rc;
}

static __inline__ ClRcT clIocPortHashDel(ClIocCommPortT *pIocCommPort) {
    hashDel(&pIocCommPort->hash);
    return CL_OK;
}

static ClIocCommPortT *clIocGetPort(ClIocPortT portId) {
    ClUint32T key = clIocPortHash(portId);
    register struct hashStruct *pTemp;
    for (pTemp = ppIocPortHashTable[key]; pTemp; pTemp = pTemp->pNext) {
        ClIocCommPortT *pIocCommPort = hashEntry(pTemp,ClIocCommPortT,hash);
        if (pIocCommPort->portId == portId) {
            return pIocCommPort;
        }
    }
    return NULL;
}

ClIocNodeAddressT clIocLocalAddressGet() {
    return gIocLocalBladeAddress;
}

ClRcT clIocPortNotification(ClIocPortT port, ClIocNotificationActionT action) {
    ClRcT rc = CL_OK;
    ClIocCommPortT *pIocCommPort = NULL;

    clOsalMutexLock(&gClIocPortMutex);

    pIocCommPort = clIocGetPort(port);
    if (pIocCommPort == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port [0x%x] passed.\n", port));
        rc = CL_IOC_RC(CL_ERR_DOESNT_EXIST);
        goto error_out;
    }

    pIocCommPort->notify = action;

    error_out: clOsalMutexUnlock(&gClIocPortMutex);
    return rc;
}

static ClRcT iocCommPortCreate(ClUint32T portId, ClIocCommPortFlagsT portType,
        ClIocCommPortT *pIocCommPort, const ClCharT *xportType,
        ClBoolT bindFlag) {
    ClIocPhysicalAddressT myAddress;
    ClRcT rc = CL_OK;

    NULL_CHECK(pIocCommPort);

    if (portId >= (CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS)) {
        clDbgCodeError(
                CL_IOC_RC(CL_ERR_INVALID_PARAMETER),
                ("Requested commport [%d] is out of range."
                "OpenClovis ASP ports should be between [1-%d]."
                "Application ports between [%d-%d]", portId, CL_IOC_RESERVED_PORTS, CL_IOC_RESERVED_PORTS+1, CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clIocCheckAndGetPortId(&portId);
    if (rc != CL_OK) {
        goto out;
    }

    CL_LIST_HEAD_INIT(&pIocCommPort->logicalAddressList);
    CL_LIST_HEAD_INIT(&pIocCommPort->multicastAddressList);
    pIocCommPort->portId = portId;

    clOsalMutexLock(&gClIocPortMutex);
    rc = clIocPortHashAdd(pIocCommPort);
    clOsalMutexUnlock(&gClIocPortMutex);

    if (rc != CL_OK) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Port hash add error.rc=0x%x\n",rc));
        goto out_put;
    }

    if (!bindFlag && clTransportBridgeEnabled(gIocLocalBladeAddress)) {
        rc = clTransportListen(xportType, portId);
    } else {
        rc = clTransportBind(xportType, portId);
    }
    if (rc != CL_OK)
        goto out_del;

    myAddress.nodeAddress = gIocLocalBladeAddress;
    myAddress.portId = pIocCommPort->portId;
    clIocCompStatusSet(myAddress, CL_IOC_NODE_UP);

    rc = clOsalMutexInit(&pIocCommPort->unblockMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&pIocCommPort->unblockCond);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&pIocCommPort->recvUnblockCond);
    CL_ASSERT(rc == CL_OK);

    goto out;

    out_del: clIocPortHashDel(pIocCommPort);

    out_put: clIocPutPortId(portId);

    out: return rc;
}

ClRcT clIocCommPortCreateStatic(ClUint32T portId, ClIocCommPortFlagsT portType,
        ClIocCommPortT *pIocCommPort, const ClCharT *xportType) {
    ClRcT rc = iocCommPortCreate(portId, portType, pIocCommPort, xportType,
            CL_TRUE);
    if (rc != CL_OK && CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
        rc = CL_OK;
    return rc;
}

ClRcT clIocCommPortCreate(ClUint32T portId, ClIocCommPortFlagsT portType,
        ClIocCommPortHandleT *pIocCommPortHandle) {
    ClIocCommPortT *pIocCommPort = NULL;
    ClRcT rc = CL_OK;

    NULL_CHECK(pIocCommPortHandle);

    pIocCommPort = clHeapCalloc(1, sizeof(*pIocCommPort));
    CL_ASSERT(pIocCommPort != NULL);

    rc = iocCommPortCreate(portId, portType, pIocCommPort, NULL, CL_FALSE);
    if (rc != CL_OK)
        goto out_free;

    *pIocCommPortHandle = (ClIocCommPortHandleT) pIocCommPort;
    return rc;

    out_free: clHeapFree(pIocCommPort);
    *pIocCommPortHandle = 0;
    return rc;
}

static ClRcT iocCommPortDelete(ClIocCommPortT *pIocCommPort,
        const ClCharT *xportType, ClBoolT bindFlag) {
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead = NULL;
    ClListHeadT *pNext = NULL;
    ClIocCompT *pComp = NULL;

    /*This would withdraw all the binds*/
    if (!bindFlag && clTransportBridgeEnabled(gIocLocalBladeAddress)) {
        clTransportListenStop(xportType, pIocCommPort->portId);
    } else {
        clTransportBindClose(xportType, pIocCommPort->portId);
    }
    clIocPutPortId(pIocCommPort->portId);

    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocCompMutex);
    if ((pComp = pIocCommPort->pComp)) {
        pIocCommPort->pComp = NULL;
        pHead = &pComp->portList;
        CL_LIST_FOR_EACH(pTemp,pHead) {
            ClIocCommPortT *pCommPort =
                    CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
            if (pCommPort == pIocCommPort) {
                clListDel(&pCommPort->listComp);
                break;
            }
        }
        /*Check if component can be unhashed and ripped off*/
        if (CL_LIST_HEAD_EMPTY(&pComp->portList)) {
            clIocCompHashDel(pComp);
            clHeapFree(pComp);
        }
    }
    pHead = &pIocCommPort->logicalAddressList;
    for (pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext) {
        ClIocLogicalAddressCtrlT *pLogicalAddress;
        pNext = pTemp->pNext;
        pLogicalAddress = CL_LIST_ENTRY(pTemp,ClIocLogicalAddressCtrlT,list);
        clHeapFree(pLogicalAddress);
    }

    clOsalMutexLock(&gClIocMcastMutex);
    pHead = &pIocCommPort->multicastAddressList;
    for (pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext) {
        ClIocMcastPortT *pMcastPort;
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listPort);
        CL_ASSERT(pMcastPort->pIocCommPort == pIocCommPort);
        clListDel(&pMcastPort->listMcast);
        clListDel(&pMcastPort->listPort);
        if (CL_LIST_HEAD_EMPTY(&pMcastPort->pMcast->portList)) {
            clIocMcastHashDel(pMcastPort->pMcast);
            clHeapFree(pMcastPort->pMcast);
        }
        clHeapFree(pMcastPort);
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clIocPortHashDel(pIocCommPort);
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    return CL_OK;
}

ClRcT clIocCommPortDeleteStatic(ClIocCommPortT *pIocCommPort,
        const ClCharT *xportType) {
    ClRcT rc = CL_OK;
    NULL_CHECK(pIocCommPort);
    rc = iocCommPortDelete(pIocCommPort, xportType, CL_TRUE);
    clOsalMutexDestroy(&pIocCommPort->unblockMutex);
    clOsalCondDestroy(&pIocCommPort->unblockCond);
    clOsalCondDestroy(&pIocCommPort->recvUnblockCond);
    return rc;
}

ClRcT clIocCommPortDelete(ClIocCommPortHandleT portId) {
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT*) portId;
    ClRcT rc = CL_OK;
    NULL_CHECK(pIocCommPort);
    clIocCommPortReceiverUnblock(portId);
    rc = iocCommPortDelete(pIocCommPort, NULL, CL_FALSE);
    clHeapFree(pIocCommPort);
    return rc;
}

ClRcT clIocCommPortFdGet(ClIocCommPortHandleT portHandle, ClInt32T *pFd) {
    ClIocCommPortT *pPortHandle = (ClIocCommPortT*) portHandle;

    if (pPortHandle == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error : Invalid CommPort handle passed.\n"));
        return CL_IOC_RC(CL_ERR_INVALID_HANDLE);
    }

    if (pFd == NULL) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Error : NULL parameter passed for getting the file descriptor.\n"));
        return CL_IOC_RC(CL_ERR_NULL_POINTER);
    }

    return CL_IOC_RC(CL_ERR_NOT_SUPPORTED);
}

#ifdef CL_IOC_COMPRESSION

static ClRcT doDecompress(ClUint8T *compressedStream, ClUint32T compressedStreamLen,
        ClUint8T **ppDecompressedStream, ClUint32T *pDecompressedStreamLen)
{
    z_stream stream;
    ClUint8T *decompressedStream = NULL;
    ClUint32T decompressedStreams = 0;
    ClInt32T err = 0;
    ClTimeT t1, t2;
    ClUint32T maxPayloadSize = gClMaxPayloadSize;
    ZLIB_TIME(t1);
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.next_in = (Byte*)compressedStream;
    stream.avail_in = compressedStreamLen;
    err = inflateInit(&stream);
    if(err != Z_OK)
    {
        clLogError("IOC", "DECOMPRESS", "Inflate init returned with [%d]", err);
        goto out_free;
    }
    do
    {
        if(!(decompressedStreams & 1))
        {
            decompressedStream = clHeapRealloc(decompressedStream,
                    (decompressedStreams + 2)*maxPayloadSize);
            CL_ASSERT(decompressedStream != NULL);
        }
        stream.next_out = (Byte*) ( decompressedStream + decompressedStreams*maxPayloadSize );
        stream.avail_out = maxPayloadSize;
        err = inflate(&stream, Z_NO_FLUSH);
        if(err == Z_OK || err == Z_STREAM_END)
        {
            ++decompressedStreams;
            if(err == Z_STREAM_END)
            break;
        }
        else
        {
            clLogError("IOC", "DECOMPRESS", "Inflate returned [%d]", err);
            goto out_free;
        }
    }while(1);

    err = inflateEnd(&stream);
    if(err != Z_OK)
    {
        clLogError("IOC", "DECOMPRESS", "Inflate end returned [%d]", err);
        goto out_free;
    }
    ZLIB_TIME(t2);
    *ppDecompressedStream = decompressedStream;
    *pDecompressedStreamLen = stream.total_out;
    clLogNotice("IOC", "DECOMPRESS", "Inflated [%ld] bytes from [%d] bytes, [%d] decompressed streams, "
            "Decompression time [%lld] usecs",
            stream.total_out, compressedStreamLen, decompressedStreams, t2-t1);
#if 0
    assert(!(stream.total_out & 3));
    do
    {
        register ClInt32T i;
        for(i = 0; i < stream.total_out >> 2; ++i)
        {
            if( i && !(i&7)) printf("\n");
            printf("%#x ", ((ClUint32T*)decompressedStream)[i]);
        }
    }while(0);
#endif
    return CL_OK;

    out_free:
    if(decompressedStream) clHeapFree(decompressedStream);
    return CL_IOC_RC(CL_ERR_LIBRARY);
}

static ClRcT doCompress(ClUint8T *uncompressedStream, ClUint32T uncompressedStreamLen,
        ClUint8T **ppCompressedStream, ClUint32T *pCompressedStreamLen)
{
    z_stream stream;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = uncompressedStreamLen/2; /*max. allowable is 50% of the same length*/
    ClInt32T err = 0;
    ClTimeT t1,t2;

    compressedStream = clHeapAllocate(compressedStreamLen);
    CL_ASSERT(compressedStream != NULL);

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    ZLIB_TIME(t1);
    err = deflateInit(&stream, Z_BEST_SPEED);
    if(err != Z_OK)
    {
        clLogError("IOC", "COMPRESS", "Deflate init returned [%d]", err);
        goto out_free;
    }
    stream.next_in = (Byte*)uncompressedStream;
    stream.next_out = (Byte*)compressedStream;
    stream.avail_in = uncompressedStreamLen;
    stream.avail_out = compressedStreamLen;
    err = deflate(&stream, Z_NO_FLUSH);
    if(err != Z_OK)
    {
        clLogError("IOC", "COMPRESS", "Deflate returned [%d]", err);
        goto out_free;
    }
    if(stream.avail_in || !stream.avail_out)
    {
        clLogError("IOC", "COMPRESS", "Deflate didn't work on stream len [%d] to the expected length [%d]",
                uncompressedStreamLen, compressedStreamLen);
        goto out_free;
    }
    err = deflate(&stream, Z_FINISH);
    if(err != Z_STREAM_END)
    {
        clLogError("IOC", "COMPRESS", "Deflate finish needs more output buffer. Returned [%d]", err);
        goto out_free;
    }
    err = deflateEnd(&stream);
    if(err != Z_OK)
    {
        clLogError("IOC", "COMPRESS", "Deflate end returned [%d]", err);
        goto out_free;
    }
    ZLIB_TIME(t2);

    *ppCompressedStream = compressedStream;
    *pCompressedStreamLen = stream.total_out;
    clLogNotice("IOC", "COMPRESS", "Pending avail bytes in the output stream [%d], uncompress len [%d], "
            "compressed len [%ld], "
            "Compression time [%lld] usecs",
            stream.avail_out, uncompressedStreamLen, stream.total_out, t2-t1);

    err = 0;
    return CL_OK;

    out_free:
    clHeapFree(compressedStream);
    return CL_IOC_RC(CL_ERR_LIBRARY);
}
#endif

typedef struct IOVecIterator {
    struct iovec *src;
    struct iovec *target;
    ClUint32T srcVectors;
    ClUint32T maxTargetVectors;
    struct iovec *header;
    struct iovec *curIOVec;
    ClOffsetT offset; /* linear offset within the cur iovec*/
} IOVecIteratorT;

static ClRcT iovecIteratorInit(IOVecIteratorT *iter, struct iovec *src,
        ClUint32T srcVectors, struct iovec *header) {
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if (!srcVectors)
        goto out;
    iter->src = src;
    iter->srcVectors = srcVectors;
    iter->curIOVec = src;
    iter->header = header;
    iter->offset = 0;
    /*
     * Divide by half to accomodate the fragmented vectors in the src.
     */
    iter->maxTargetVectors = 16; /* go in steps of 16 target vectors including header vector */
    iter->target = clHeapCalloc(iter->maxTargetVectors, sizeof(*iter->target));
    CL_ASSERT(iter->target != NULL);
    if (header) {
        iter->target[0].iov_base = header->iov_base;
        iter->target[0].iov_len = header->iov_len;
    }
    rc = CL_OK;
    out: return rc;
}

static ClRcT iovecIteratorNext(IOVecIteratorT *iter, ClUint32T *payload,
        struct iovec **target, ClUint32T *targetVectors) {
    ClUint32T numVectors = 0;
    size_t size;
    struct iovec *curIOVec;
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if (!payload)
        goto out;
    rc = CL_ERR_NO_SPACE;
    if (!(curIOVec = iter->curIOVec))
        goto out;
    size = (size_t) * payload;

    if (iter->header) {
        numVectors = 1;
    }

    while (size > 0 && curIOVec) {
        if (curIOVec->iov_len == 0) {
            ++curIOVec;
            if ((curIOVec - iter->src) >= iter->srcVectors) {
                curIOVec = NULL;
            }
            continue;
        }
        size_t len = curIOVec->iov_len - iter->offset;
        ClUint8T *base = curIOVec->iov_base + iter->offset;
        while (size > 0 && len > 0) {
            size_t tgtLen = CL_MIN(size, len);
            if (numVectors >= iter->maxTargetVectors) {
                iter->target = clHeapRealloc(iter->target,
                        sizeof(*iter->target) * (numVectors + 16));
                CL_ASSERT(iter->target != NULL);
                memset(iter->target + numVectors, 0,
                        sizeof(*iter->target) * 16);
                iter->maxTargetVectors += 16;
            }
            iter->target[numVectors].iov_base = base;
            iter->target[numVectors].iov_len = tgtLen;
            ++numVectors;
            size -= tgtLen;
            len -= tgtLen;
            if (!len) {
                iter->offset = 0;
                ++curIOVec;
                if ((curIOVec - iter->src) >= iter->srcVectors)
                    curIOVec = NULL;
                break;
            }
            base += tgtLen;
            iter->offset += tgtLen;
        }
    }
    iter->curIOVec = curIOVec;
    *target = iter->target;
    *targetVectors = numVectors;
    if (size > 0)
        *payload -= size;
    rc = CL_OK;
    out: return rc;
}

static ClRcT iovecIteratorExit(IOVecIteratorT *iter) {
    if (iter->target) {
        clHeapFree(iter->target);
        iter->target = NULL;
    }
    memset(iter, 0, sizeof(*iter)); /* reset all*/
    return CL_OK;
}

/*
 * Function : clIocSend Description : This function will take the message and
 * enqueue to IOC queues for transmission. 
 */

ClRcT clIocSendWithXportRelayReliable(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType,
        ClIocAddressT *originAddress, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption, ClCharT *xportType, ClBoolT proxy,
        ClBoolT isIOCReliable) {
    ClRcT retCode = CL_OK, rc = CL_OK;
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) commPortHandle;
    ClUint32T timeout = 0;
    ClUint8T priority = 0;
    ClUint32T msgLength;
    ClUint32T maxPayload, fragId, bytesRead = 0;
    ClUint32T totalFragRequired, fraction;
    ClUint32T addrType;
    ClBoolT isBcast = CL_FALSE;
    ClBoolT isReliable = isIOCReliable;
    ClBoolT isPhysicalANotBroadcast = CL_FALSE;
    ClIocAddressT *replicastList = NULL;
    ClIocAddressT srcAddress = { { 0 } };
    ClUint32T numReplicasts = 0;
#ifdef CL_IOC_COMPRESSION
    ClUint8T *decompressedStream = NULL;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = 0;
    ClTimeT pktTime = 0;
    ClTimeValT tv = {0};
#endif

    ClIocAddressT interimDestAddress = { { 0 } };

    if (!pIocCommPort || !destAddress)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    srcAddress.iocPhyAddress.nodeAddress = gIocLocalBladeAddress;
    srcAddress.iocPhyAddress.portId = pIocCommPort->portId;
    if (!originAddress) {
        originAddress = &srcAddress;
    }
    interimDestAddress = *destAddress;
    addrType = CL_IOC_ADDRESS_TYPE_GET(destAddress);
    isBcast = (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE
            && ((ClIocPhysicalAddressT*) destAddress)->nodeAddress
                    == CL_IOC_BROADCAST_ADDRESS);

    isPhysicalANotBroadcast = (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE
            && ((ClIocPhysicalAddressT*) destAddress)->nodeAddress
                    != CL_IOC_BROADCAST_ADDRESS);

    if (isBcast && gClIocReplicast) {
        clIocReplicastGet(((ClIocPhysicalAddressT*) destAddress)->portId,
                &replicastList, &numReplicasts);

    }

    if (isPhysicalANotBroadcast) {
        ClIocNodeAddressT node;
        ClUint8T status;
        /*
         * If proxy is enabled, reset as for unicasts, it would invert the logic.
         * already present to direct the traffic using the right transport
         */
        if (proxy) {
            clLogWarning("PROXY", "SEND",
                    "Disabling proxy sends for unicast traffic");
            proxy = CL_FALSE;
        }
        node = ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;

        if (CL_IOC_RESERVED_ADDRESS == node) {
            clDbgCodeError(
                    CL_IOC_RC(CL_ERR_INVALID_PARAMETER),
                    ("Error : Invalid destination address %x:%x is passed.", ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
        }

        retCode = clIocCompStatusGet(*(ClIocPhysicalAddressT *) destAddress,
                &status);
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(
                    CL_DEBUG_ERROR,
                    ("Error : Failed to get the status of the component. error code 0x%x", retCode));
            return retCode;
        }
        retCode = clFindTransport(
                ((ClIocPhysicalAddressT*) destAddress)->nodeAddress,
                &interimDestAddress, &xportType);
        if (retCode != CL_OK || !interimDestAddress.iocPhyAddress.nodeAddress
                || !xportType) {
            clLogNotice(
                    "XPORT",
                    "LUT",
                    "Not found in destNodeLUT %x:%x is passed.", ((ClIocPhysicalAddressT * )destAddress)->nodeAddress, ((ClIocPhysicalAddressT * )destAddress)->portId);
            return CL_OK;
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress
                && interimDestAddress.iocPhyAddress.nodeAddress != node) {
            if (destAddress->iocPhyAddress.portId >= CL_IOC_XPORT_PORT) {
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            } else {
                interimDestAddress.iocPhyAddress.portId =
                        destAddress->iocPhyAddress.portId;
            }clLogTrace(
                    "PROXY",
                    "SEND",
                    "Destination through the bridge at node [%d], port [%d]", interimDestAddress.iocPhyAddress.nodeAddress, interimDestAddress.iocPhyAddress.portId);
            retCode = clIocCompStatusGet(interimDestAddress.iocPhyAddress,
                    &status);
            if (retCode != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Error : Failed to get the status of the component. error code 0x%x", retCode));
                return retCode;
            }
            if (status == CL_IOC_NODE_DOWN) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Port [0x%x] is trying to reach component [0x%x:0x%x] with [xport: %s]"
                        "but the component is not reachable.", pIocCommPort->portId, ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId, xportType));
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
        } else {
            /*
             * Check status if destination isn't through a bridge.
             */
            if (status == CL_IOC_NODE_DOWN) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Port [0x%x] is trying to reach component [0x%x:0x%x] but the component is not reachable.", pIocCommPort->portId, ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId));
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
            interimDestAddress.iocPhyAddress.nodeAddress =
                    ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
            interimDestAddress.iocPhyAddress.portId =
                    ((ClIocPhysicalAddressT *) destAddress)->portId;
        }
    } else {
        //clLogDebug("IOC", "Rel", "sending broadcast message . Set isReliable to false");
        isReliable = CL_FALSE;
    }

    if (pSendOption) {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_IOC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Failed to get the length of the message. error code 0x%x",retCode));
        retCode = CL_IOC_RC(CL_ERR_INVALID_BUFFER);
        goto out_free;
    }

    maxPayload = gClMaxPayloadSize;

#ifdef CL_IOC_COMPRESSION
    if(protoType != CL_IOC_PORT_NOTIFICATION_PROTO
            &&
            protoType != CL_IOC_PROTO_ARP
            &&
            protoType != CL_IOC_PROTO_CTL
            &&
            msgLength >= gIocCompressionBoundary)
    {
        retCode = clBufferFlatten(message, &decompressedStream);
        CL_ASSERT(retCode == CL_OK);
        retCode = doCompress(decompressedStream, msgLength, &compressedStream, &compressedStreamLen);
        clHeapFree((ClPtrT)decompressedStream);

        if(retCode == CL_OK)
        {
            /*
             * We don't want to use compression for large payloads.
             */
            if(compressedStreamLen &&
                    compressedStreamLen <= maxPayload)
            {
                ClBufferHandleT cmsg = 0;
                msgLength = compressedStreamLen;
                retCode = clBufferCreateAndAllocate(compressedStreamLen, &cmsg);
                CL_ASSERT(retCode == CL_OK);
                retCode = clBufferNBytesWrite(cmsg, compressedStream, compressedStreamLen);
                CL_ASSERT(retCode == CL_OK);
                message = cmsg;
            }
            else
            {
                compressedStreamLen = 0;
            }
            clHeapFree(compressedStream);
            compressedStream = NULL;
        }
    }
#endif
    fragId = currFragId;
#ifdef RELIABLE_IOC
    if (protoType == CL_IOC_SEND_ACK_MESSAGE_PROTO
            || protoType == CL_IOC_SEND_ACK_PROTO
            || protoType == CL_IOC_SEND_NAK_PROTO)
    {
        isReliable = CL_FALSE;
    } else {
        clOsalMutexLock(gClIocFragMutex);
        ++currFragId;
        clOsalMutexUnlock(gClIocFragMutex);
    }
#endif
    if (msgLength > maxPayload) {
        /*
         * Fragment it to 64 K size and return
         */
        ClIocFragHeaderT userFragHeader = { { 0 } };
        struct iovec header = { .iov_base = (void*) &userFragHeader, .iov_len =
                sizeof(userFragHeader) };
        struct iovec *target = NULL;
        IOVecIteratorT iovecIterator = { 0 };
        struct iovec *src = NULL;
        ClInt32T srcVectors = 0;
        ClUint32T targetVectors = 0;
        ClUint32T frags = 0;
        rc = clBufferVectorize(message, &src, &srcVectors);
        CL_ASSERT(rc == CL_OK);
        rc = iovecIteratorInit(&iovecIterator, src, srcVectors, &header);
        CL_ASSERT(rc == CL_OK);
        totalFragRequired = msgLength / maxPayload;
        fraction = msgLength % maxPayload;
        if (fraction)
            totalFragRequired = totalFragRequired + 1;

        userFragHeader.header.version = CL_IOC_HEADER_VERSION;
        userFragHeader.header.protocolType = protoType;
        userFragHeader.header.priority = priority;
        userFragHeader.header.flag = IOC_MORE_FRAG;
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = htonl(
                originAddress->iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = htonl(
                originAddress->iocPhyAddress.portId);
        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->portId);
        userFragHeader.header.reserved = 0;
#ifdef RELIABLE_IOC
        //get piggyback information.
        userFragHeader.header.isReliable = isReliable;
        userFragHeader.header.piggyBackACK = 0;
        userFragHeader.header.piggyBackACKMessageId = 0;
#endif
#ifdef CL_IOC_COMPRESSION
        userFragHeader.header.pktTime = clHtonl64(pktTime);
#endif
        userFragHeader.header.messageId = htonl(fragId);
        userFragHeader.msgId = htonl(fragId);
        userFragHeader.fragOffset = 0;
        userFragHeader.fragLength = htonl(maxPayload);
        userFragHeader.fragId = 1;
        ClIocReliableSenderNodeT *nodeCurrent = NULL;
        while (totalFragRequired > 1) {
#ifdef RELIABLE_IOC
            if (isReliable == CL_TRUE)
            {
                clOsalMutexLock(&iocSenderLock);
                nodeCurrent = getSenderBufferNode(fragId,&interimDestAddress, pIocCommPort->portId);
                if (!nodeCurrent)
                {
                    clLogDebug("IOC", "Rel", "message not found in sender buffer. Create new message");
                }
                else
                {
                    ClUint32T lossFrag=nodeCurrent->totalLost;
                    if(lossFrag>0)
                    {

                        ClUint32T fragmentId = lossListGetFirst(nodeCurrent->sendLossList);
                        //clLogDebug("IOC", "Rel", "send fragId [%d] in total loss [%d] ",fragmentId,lossFrag);
                        ClRcT retcode = senderBufferRetranmission(commPortHandle,
                                fragId, fragmentId,
                                &interimDestAddress, xportType, proxy);
                        if(retcode==CL_OK)
                        {
                            ClBoolT ret = lossListDelete(&nodeCurrent->sendLossList, fragmentId);
                            nodeCurrent->totalLost=lossListCount(&nodeCurrent->sendLossList);
                            //clLogDebug("IOC", "Rel", "sent fragId [%d] .current total loss [%d] ",fragmentId,nodeCurrent->totalLost);

                        }
                    }
                }
                clOsalMutexUnlock(&iocSenderLock);

            }
#endif
            ClUint32T payload = maxPayload;
            retCode = iovecIteratorNext(&iovecIterator, &payload, &target,
                    &targetVectors);
            CL_ASSERT(retCode == CL_OK);
            CL_ASSERT(payload == maxPayload);
            CL_DEBUG_PRINT(
                    CL_DEBUG_TRACE,
                    ("Sending id %d flag 0x%x length %d offset %d\n", ntohl(userFragHeader.msgId), userFragHeader.header.flag, ntohl(userFragHeader.fragLength), ntohl(userFragHeader.fragOffset)));
            /*
             *clLogNotice("FRAG", "SEND", "Sending [%d] bytes with [%d] vectors representing [%d] bytes", 
             msgLength, targetVectors, maxPayload);
             */
            if (replicastList) {
                retCode = internalSendReplicast(pIocCommPort, target,
                        targetVectors, payload + sizeof(userFragHeader),
                        priority, &timeout, replicastList, numReplicasts,
                        &userFragHeader, proxy);
            } else {
                /*clLogTrace(
                 "IOC",
                 "SEND",
                 "Sending to destination [%#x: %#x] with [xport: %s]",
                 interimDestAddress.iocPhyAddress.nodeAddress,
                 interimDestAddress.iocPhyAddress.portId, xportType);*/
                retCode = internalSend(pIocCommPort, target, targetVectors,
                        payload + sizeof(userFragHeader), priority,
                        &interimDestAddress, &timeout, xportType, proxy);
            }
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT)) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Failed to send the message. error code = 0x%x\n", retCode));
                goto frag_error;
            }

#ifdef RELIABLE_IOC
            if (isReliable == CL_TRUE)
            {
                clOsalMutexLock(&iocSenderLock);
                senderBufferAddFragment(commPortHandle, fragId, target,
                        targetVectors, &interimDestAddress,
                        payload + sizeof(userFragHeader), userFragHeader.fragId,
                        totalFragRequired);
                clOsalMutexUnlock(&iocSenderLock);
                userFragHeader.fragId++;
            }

#endif

            bytesRead += payload;
            userFragHeader.fragOffset = htonl(bytesRead); /* updating for the
             * next packet */
            --totalFragRequired;

#ifdef CL_IOC_COMPRESSION            
            if(userFragHeader.header.pktTime)
            userFragHeader.header.pktTime = 0;
#endif
            ++frags;
            if (0 && !(frags & 511))
                sleep(1);
        } /* while */
        /*
         * sending the last fragment
         */
        if (fraction) {
            maxPayload = fraction;
            userFragHeader.fragLength = htonl(maxPayload);
        } else
            fraction = maxPayload;
        userFragHeader.header.flag = IOC_LAST_FRAG;
        retCode = iovecIteratorNext(&iovecIterator, &fraction, &target,
                &targetVectors);
        CL_ASSERT(retCode == CL_OK);
        CL_ASSERT(fraction == maxPayload);
        clLogTrace(
                "FRAG",
                "SEND",
                "Sending last frag at offset [%d], length [%d]", ntohl(userFragHeader.fragOffset), fraction);
        if (replicastList) {
            retCode = internalSendReplicast(pIocCommPort, target, targetVectors,
                    fraction + sizeof(userFragHeader), priority, &timeout,
                    replicastList, numReplicasts, &userFragHeader, proxy);
        } else {
            /*clLogTrace(
             "IOC",
             "SEND",
             "Sending to destination [%#x: %#x] with [xport: %s]",
             interimDestAddress.iocPhyAddress.nodeAddress,
             interimDestAddress.iocPhyAddress.portId, xportType);
             */
            retCode = internalSend(pIocCommPort, target, targetVectors,
                    fraction + sizeof(userFragHeader), priority,
                    &interimDestAddress, &timeout, xportType, proxy);
        }

#ifdef RELIABLE_IOC
#define COUNT_LIMITED 10
#define RESEND_COUNT 3

        if (isReliable == CL_TRUE)
        {
            clOsalMutexLock(&iocSenderLock);
            senderBufferAddFragment(commPortHandle, fragId, target,
                    targetVectors, &interimDestAddress,
                    fraction + sizeof(userFragHeader), userFragHeader.fragId,
                    totalFragRequired);
            clOsalMutexUnlock(&iocSenderLock);
            userFragHeader.fragId++;
            ClUint32T count=0,resend=0;
            ClBoolT exit=CL_TRUE;
            do
            {
                clOsalMutexLock(&iocSenderLock);
                nodeCurrent = getSenderBufferNode(fragId,&interimDestAddress, pIocCommPort->portId);
                if (!nodeCurrent)
                {
                    clLogDebug("IOC", "Rel", "node not found in sender buffer, break");
                    exit=CL_FALSE;
                    clOsalMutexUnlock(&iocSenderLock);
                    break;
                }
                else
                {
                    ClUint32T lossFrag=nodeCurrent->totalLost;
                    if(lossFrag>0)
                    {

                        ClUint32T fragmentId = lossListGetFirst(nodeCurrent->sendLossList);
                        senderBufferRetranmission(commPortHandle,
                                fragId, fragmentId,
                                &interimDestAddress, xportType, proxy);
                        ClBoolT ret=lossListDelete(&nodeCurrent->sendLossList, fragmentId);
                        nodeCurrent->totalLost=lossListCount(&nodeCurrent->sendLossList);
                        clLogDebug("IOC", "Rel", "resent fragId [%d] .current total loss [%d] ",fragmentId,nodeCurrent->totalLost);
                        clOsalMutexUnlock(&iocSenderLock);
                     }
                     else
                     {

                        count++;
                        if(count==COUNT_LIMITED)
                        {
                            if(resend==RESEND_COUNT)
                            {
                                clLogDebug("IOC", "Rel", "message send error. remove message ");
                                //remove node
                                ClRbTreeT *iter, *next = NULL;
                                for (iter = clRbTreeMin(&nodeCurrent->senderBufferTree); iter; iter = next)
                                {
                                    ClIocReliableFragmentSenderNodeT *fragNode = CL_RBTREE_ENTRY(iter, ClIocReliableFragmentSenderNodeT, tree);
                                    next = clRbTreeNext(&nodeCurrent->senderBufferTree, iter);
                                    if (fragNode->fragmentId <= nodeCurrent->numFragments)
                                    {
                                        if (&fragNode->buffer)
                                        {
                                            //clLogDebug("IOC", "Rel","remove fragment [%d]",fragNode->fragmentId);
                                            clBufferDelete(&fragNode->buffer);
                                        }
                                        if(nodeCurrent->sendLossList)
                                        {
                                            lossListDelete(&nodeCurrent->sendLossList, fragNode->fragmentId);
                                        }
                                        clRbTreeDelete(&nodeCurrent->senderBufferTree, iter);
                                        clHeapFree(fragNode);

                                    }
                                }
                                hashDel(&nodeCurrent->hash);
                                if (clTimerCheckAndDelete(&nodeCurrent->ttlTimerKey->ttlTimer) == CL_OK)
                                {
                                   clHeapFree(nodeCurrent->ttlTimerKey);
                                   nodeCurrent->ttlTimerKey = NULL;
                                 }
                                 destroyNodes(&nodeCurrent->sendLossList);
                                 clHeapFree(nodeCurrent);
                                 nodeCurrent=NULL;
                                 clOsalMutexUnlock(&iocSenderLock);
                                 //clLogDebug("IOC", "Rel","removed message in sender buffer");
                                break;
                            }
                            else
                            {
                                //resend unack fragment
                                ClUint32T i = 0;
                                resend++;
                                count=0;

                                for(i=nodeCurrent->ackSync + 1 ; i<=nodeCurrent->numFragments;i++)
                                {
                                    //clLogDebug("IOC", "Rel", "resend fragId [%d]", i);
                                    ClRcT rc = senderBufferRetranmission(commPortHandle,
                                                                    fragId, i,
                                                                    &interimDestAddress, xportType, proxy);
                                    if(rc == CL_ERR_NOT_IMPLEMENTED || rc == CL_ERR_NOT_EXIST )
                                    {
                                        clLogDebug("IOC", "Rel", "resend fragId false. break ");
                                        exit=CL_FALSE;
                                        clOsalMutexUnlock(&iocSenderLock);
                                        break;
                                    }
                                }
                                clOsalMutexUnlock(&iocSenderLock);
                            }
                        }
                        clOsalMutexUnlock(&iocSenderLock);
                        usleep(4000);
                        clLogDebug("IOC", "Rel", "waitting for last fragment ack ...");
                     }
                }
            }while(exit);
        }
#endif

        frag_error: {
            ClRcT rc;
            rc = iovecIteratorExit(&iovecIterator);
            CL_ASSERT(rc == CL_OK);
            if (src)
                clHeapFree(src);
        }
    } else {
        ClIocHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = htonl(
                originAddress->iocPhyAddress.nodeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = htonl(
                originAddress->iocPhyAddress.portId);
        userHeader.dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userHeader.dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->portId);
        userHeader.reserved = 0;
        userHeader.isReliable = isReliable;
        userHeader.messageId = htonl(fragId);

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
        userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif
        retCode = clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                sizeof(ClIocHeaderT));
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            goto out_free;
        }

#ifdef RELIABLE_IOC
        if (isReliable == CL_TRUE) {
            clLogDebug(
                    "IOC",
                    "Rel",
                    "add node with messageId [%d ]and interimDestAddress [%d]  - [%d]", fragId, interimDestAddress.iocPhyAddress.nodeAddress, interimDestAddress.iocPhyAddress.portId);
            ClUint32T len = 0;
            clBufferLengthGet(message, &len);
            senderBufferAddMessage(commPortHandle, fragId, message, len,
                    &interimDestAddress, 1);
        }
#endif
        if (replicastList) {
            //clLogDebug("IOC", "Rel", "send replicast");
            retCode = internalSendSlowReplicast(pIocCommPort, message, priority,
                    &timeout, replicastList, numReplicasts, &userHeader, proxy);
        } else {
            /*clLogDebug(
             "IOC",
             "SEND",
             "Sending to destination [%#x: %#x] with [xport: %s]",
             interimDestAddress.iocPhyAddress.nodeAddress,
             interimDestAddress.iocPhyAddress.portId, xportType);*/
            retCode = internalSendSlow(pIocCommPort, message, priority,
                    &interimDestAddress, &timeout, xportType, proxy);
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if (rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Buffer header trim failed RC = 0x%x\n", rc));

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
        {
            /*
             * delete the compressed message stream.
             */
            clBufferDelete(&message);
        }
#endif
    }

    out_free: if (replicastList)
        clHeapFree(replicastList);

    return retCode;
}

/*
 * Function : clIocSend Description : This function will take the message and
 * enqueue to IOC queues for transmission.
 */
ClRcT clIocSendWithXportRelay(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType,
        ClIocAddressT *originAddress, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption, ClCharT *xportType, ClBoolT proxy) {
    return clIocSendWithXportRelayReliable(commPortHandle, message, protoType,
            originAddress, destAddress, pSendOption, xportType, proxy, CL_FALSE);
}

ClRcT clIocSendWithXport(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption, ClCharT *xportType, ClBoolT proxy) {
    return clIocSendWithXportRelay(commPortHandle, message, protoType, NULL,
            destAddress, pSendOption, xportType, proxy);
}

ClRcT clIocSendWithRelay(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType, ClIocAddressT *srcAddress,
        ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption) {
    return clIocSendWithXportRelay(commPortHandle, message, protoType,
            srcAddress, destAddress, pSendOption, NULL, CL_FALSE);
}

ClRcT clIocSend(ClIocCommPortHandleT commPortHandle, ClBufferHandleT message,
        ClUint8T protoType, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption) {
    return clIocSendWithXportRelay(commPortHandle, message, protoType, NULL,
            destAddress, pSendOption, NULL, CL_FALSE);
}

ClRcT clIocSendReliable(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption) {
    return clIocSendWithXportRelayReliable(commPortHandle, message, protoType,
            NULL, destAddress, pSendOption, NULL, CL_FALSE, CL_TRUE);
}

ClRcT clIocSendSlow(ClIocCommPortHandleT commPortHandle,
        ClBufferHandleT message, ClUint8T protoType, ClIocAddressT *destAddress,
        ClIocSendOptionT *pSendOption) {
    ClRcT retCode, rc;
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) commPortHandle;
    ClUint32T timeout = 0;
    ClUint8T priority = 0;
    ClUint32T msgLength;
    ClUint32T maxPayload, fragId, bytesRead = 0;
    ClUint32T totalFragRequired, fraction;
    ClBufferHandleT tempMsg;
    ClUint32T addrType;
    ClBoolT isPhysicalANotBroadcast = CL_FALSE;
#ifdef CL_IOC_COMPRESSION
    ClUint8T *decompressedStream = NULL;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = 0;
    ClTimeT pktTime = 0;
    ClTimeValT tv = {0};
#endif
    if (!pIocCommPort || !destAddress)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    addrType = CL_IOC_ADDRESS_TYPE_GET(destAddress);
    ClIocAddressT interimDestAddress = { { 0 } };
    ClCharT *xportType = NULL;

    isPhysicalANotBroadcast = (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE
            && ((ClIocPhysicalAddressT *) destAddress)->nodeAddress
                    != CL_IOC_BROADCAST_ADDRESS);
    if (isPhysicalANotBroadcast) {
        ClIocNodeAddressT node;
        ClUint8T status;

        node = ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;

        if (CL_IOC_RESERVED_ADDRESS == node) {
            clDbgCodeError(
                    CL_IOC_RC(CL_ERR_INVALID_PARAMETER),
                    ("Error : Invalid destination address %x:%x is passed.", ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
        }

        retCode = clIocCompStatusGet(*(ClIocPhysicalAddressT *) destAddress,
                &status);
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(
                    CL_DEBUG_ERROR,
                    ("Error : Failed to get the status of the component. error code 0x%x", retCode));
            return retCode;
        }
        retCode = clFindTransport(
                ((ClIocPhysicalAddressT*) destAddress)->nodeAddress,
                &interimDestAddress, &xportType);
        if (retCode != CL_OK || !interimDestAddress.iocPhyAddress.nodeAddress
                || !xportType) {
            clLogNotice(
                    "XPORT",
                    "LUT",
                    "Not found in destNodeLUT %x:%x is passed.", ((ClIocPhysicalAddressT * )destAddress)->nodeAddress, ((ClIocPhysicalAddressT * )destAddress)->portId);
            return CL_OK;
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress
                && interimDestAddress.iocPhyAddress.nodeAddress != node) {
            interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            retCode = clIocCompStatusGet(interimDestAddress.iocPhyAddress,
                    &status);
            if (retCode != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Error : Failed to get the status of the component. error code 0x%x", retCode));
                return retCode;
            }
            if (status == CL_IOC_NODE_DOWN) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("XportType=[%s]: Port [0x%x] is trying to reach component [0x%x:0x%x] "
                        "but the component is not reachable.", xportType, pIocCommPort->portId, ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId));
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
        } else {
            if (status == CL_IOC_NODE_DOWN) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Port [0x%x] is trying to reach component [0x%x:0x%x] but the component is not reachable.", pIocCommPort->portId, ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId));
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
            interimDestAddress.iocPhyAddress.nodeAddress =
                    ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
            interimDestAddress.iocPhyAddress.portId =
                    ((ClIocPhysicalAddressT *) destAddress)->portId;
        }
    }

    if (pSendOption) {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_IOC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Failed to get the lenght of the messege. error code 0x%x",retCode));
        return CL_IOC_RC(CL_ERR_INVALID_BUFFER);
    }

    maxPayload = gClMaxPayloadSize;

#ifdef CL_IOC_COMPRESSION
    if(protoType != CL_IOC_PORT_NOTIFICATION_PROTO
            &&
            protoType != CL_IOC_PROTO_ARP
            &&
            protoType != CL_IOC_PROTO_CTL
            &&
            msgLength >= gIocCompressionBoundary)
    {
        retCode = clBufferFlatten(message, &decompressedStream);
        CL_ASSERT(retCode == CL_OK);
        retCode = doCompress(decompressedStream, msgLength, &compressedStream, &compressedStreamLen);
        clHeapFree((ClPtrT)decompressedStream);

        if(retCode == CL_OK)
        {
            /*
             * We don't want to use compression for large payloads.
             */
            if(compressedStreamLen &&
                    compressedStreamLen <= maxPayload)
            {
                ClBufferHandleT cmsg = 0;
                msgLength = compressedStreamLen;
                retCode = clBufferCreateAndAllocate(compressedStreamLen, &cmsg);
                CL_ASSERT(retCode == CL_OK);
                retCode = clBufferNBytesWrite(cmsg, compressedStream, compressedStreamLen);
                CL_ASSERT(retCode == CL_OK);
                message = cmsg;
            }
            else
            {
                compressedStreamLen = 0;
            }
            clHeapFree(compressedStream);
            compressedStream = NULL;
        }
    }
#endif

    if (msgLength > maxPayload) {
        /*
         * Fragment it to 64 K size and return
         */
        ClIocFragHeaderT userFragHeader = { { 0 } };
        ClUint32T frags = 0;
        retCode = clBufferCreate(&tempMsg);
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(
                    CL_DEBUG_ERROR,
                    ("\nERROR : Message creation failed. errorCode = 0x%x\n", retCode));
            return retCode;
        }

        totalFragRequired = msgLength / maxPayload;
        fraction = msgLength % maxPayload;
        if (fraction)
            totalFragRequired = totalFragRequired + 1;

        clOsalMutexLock(gClIocFragMutex);
        fragId = currFragId;
        currFragId++;
        clOsalMutexUnlock(gClIocFragMutex);

        userFragHeader.header.version = CL_IOC_HEADER_VERSION;
        userFragHeader.header.protocolType = protoType;
        userFragHeader.header.priority = priority;
        userFragHeader.header.flag = IOC_MORE_FRAG;
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = htonl(
                gIocLocalBladeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = htonl(
                pIocCommPort->portId);
        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->portId);
        userFragHeader.header.reserved = 0;
#ifdef CL_IOC_COMPRESSION
        userFragHeader.header.pktTime = clHtonl64(pktTime);
#endif
        userFragHeader.msgId = htonl(fragId);
        userFragHeader.fragOffset = 0;
        userFragHeader.fragLength = htonl(maxPayload);

        while (totalFragRequired > 1) {
            retCode = clBufferToBufferCopy(message, bytesRead, tempMsg,
                    maxPayload);
            if (retCode != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("\nERROR: message to message copy failed. errorCode = 0x%x\n", retCode));
                goto frag_error;
            }

            CL_DEBUG_PRINT(
                    CL_DEBUG_TRACE,
                    ("Sending id %d flag 0x%x length %d offset %d\n", ntohl(userFragHeader.msgId), userFragHeader.header.flag, ntohl(userFragHeader.fragLength), ntohl(userFragHeader.fragOffset)));

            retCode = clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                    sizeof(ClIocFragHeaderT));
            if (retCode != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("\nERROR : Prepend buffer data failed. errorCode = 0x%x\n", retCode));
                goto frag_error;
            }
            if (isPhysicalANotBroadcast) {
                /*clLogTrace(
                 "IOC",
                 "SEND",
                 "Sending to destination [%#x: %#x] with [xport: %s]",
                 interimDestAddress.iocPhyAddress.nodeAddress,
                 interimDestAddress.iocPhyAddress.portId, xportType);*/

                retCode = internalSendSlow(pIocCommPort, tempMsg, priority,
                        &interimDestAddress, &timeout, xportType, CL_FALSE);
            } else {
                retCode = internalSendSlow(pIocCommPort, tempMsg, priority,
                        destAddress, &timeout, xportType, CL_FALSE);
            }
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT)) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Failed to send the message. error code = 0x%x\n", retCode));
                goto frag_error;
            }

            bytesRead = bytesRead + maxPayload;
            userFragHeader.fragOffset = htonl(bytesRead); /* updating for the
             * next packet */
            retCode = clBufferClear(tempMsg);
            if (retCode != CL_OK)
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("\nERROR : Failed clear the buffer. errorCode = 0x%x\n",retCode));

            totalFragRequired--;
#ifdef CL_IOC_COMPRESSION            
            if(userFragHeader.header.pktTime)
            userFragHeader.header.pktTime = 0;
#endif
            ++frags;
            if (!(frags & 511))
                sleep(1);
            /*usleep(CL_IOC_MICRO_SLEEP_INTERVAL);*/
        } /* while */
        /*
         * sending the last fragment
         */
        if (fraction) {
            maxPayload = fraction;
            userFragHeader.fragLength = htonl(maxPayload);
        }
        userFragHeader.header.flag = IOC_LAST_FRAG;

        retCode = clBufferToBufferCopy(message, bytesRead, tempMsg, maxPayload);
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(
                    CL_DEBUG_ERROR,
                    ("Error : message to message copy failed. rc = 0x%x\n", retCode));
            goto frag_error;
        }
        retCode = clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                sizeof(ClIocFragHeaderT));
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            goto frag_error;
        }clLogTrace(
                "FRAG",
                "SEND2",
                "Sending last frag at offset [%d], length [%d]", ntohl(userFragHeader.fragOffset), maxPayload);
        if (isPhysicalANotBroadcast) {
            /*clLogTrace(
             "IOC",
             "SEND",
             "Sending to destination [%#x: %#x] with [xport: %s]",
             interimDestAddress.iocPhyAddress.nodeAddress,
             interimDestAddress.iocPhyAddress.portId, xportType);*/

            retCode = internalSendSlow(pIocCommPort, tempMsg, priority,
                    &interimDestAddress, &timeout, xportType, CL_FALSE);
        } else {
            retCode = internalSendSlow(pIocCommPort, tempMsg, priority,
                    destAddress, &timeout, xportType, CL_FALSE);
        }

        frag_error: {
            ClRcT rc;
            rc = clBufferDelete(&tempMsg);
            CL_ASSERT(rc == CL_OK);
        }
    } else {
        ClIocHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = htonl(
                gIocLocalBladeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = htonl(
                pIocCommPort->portId);
        userHeader.dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userHeader.dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) destAddress)->portId);
        userHeader.reserved = 0;

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
        userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif

        retCode = clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                sizeof(ClIocHeaderT));
        if (retCode != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            return retCode;
        }

        if (isPhysicalANotBroadcast) {
            /*clLogTrace(
             "IOC",
             "SEND",
             "Sending to destination [%#x: %#x] with [xport: %s]",
             interimDestAddress.iocPhyAddress.nodeAddress,
             interimDestAddress.iocPhyAddress.portId, xportType);*/

            retCode = internalSendSlow(pIocCommPort, message, priority,
                    &interimDestAddress, &timeout, xportType, CL_FALSE);
        } else {
            retCode = internalSendSlow(pIocCommPort, message, priority,
                    destAddress, &timeout, xportType, CL_FALSE);
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if (rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Buffer header trim failed RC = 0x%x\n", rc));

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
        {
            /*
             * delete the compressed message stream.
             */
            clBufferDelete(&message);
        }
#endif

    }

    return retCode;
}

static ClRcT internalSend(ClIocCommPortT *pIocCommPort, struct iovec *target,
        ClUint32T targetVectors, ClUint32T messageLen, ClUint32T tempPriority,
        ClIocAddressT *pIocAddress, ClUint32T *pTimeout, ClCharT *xportType,
        ClBoolT proxy) {
    ClRcT rc = CL_OK;
    ClUint32T priority;
    static ClInt32T recordIOCSend = -1;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if (pIocCommPort == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port id\n"));
        goto out;
    }

    priority = CL_IOC_DEFAULT_PRIORITY;
    if (pIocCommPort->portId == CL_IOC_CPM_PORT
            || pIocCommPort->portId == CL_IOC_XPORT_PORT
            || tempPriority == CL_IOC_HIGH_PRIORITY) {
        priority = CL_IOC_HIGH_PRIORITY;
    }

    /*
     * If the leaky bucket is defined, then 
     * respect the traffic signal
     */
    if (gClIocTrafficShaper && gClLeakyBucket) {
        clLeakyBucketFill(gClLeakyBucket, messageLen);
    }

    if (recordIOCSend < 0) {
        ClBoolT record = clParseEnvBoolean("CL_ASP_RECORD_IOC_SEND");
        recordIOCSend = record ? 1 : 0;
    }

    if (recordIOCSend)
        clTaskPoolRecordIOCSend(CL_TRUE);

    rc = clTransportSendProxy(xportType, pIocCommPort->portId, priority,
            pIocAddress, target, targetVectors, 0, proxy);

    if (recordIOCSend)
        clTaskPoolRecordIOCSend(CL_FALSE);

    out: return rc;
}

static ClRcT internalSendReplicast(ClIocCommPortT *pIocCommPort,
        struct iovec *target, ClUint32T targetVectors, ClUint32T messageLen,
        ClUint32T tempPriority, ClUint32T *pTimeout,
        ClIocAddressT *replicastList, ClUint32T numReplicasts,
        ClIocFragHeaderT *userFragHeader, ClBoolT proxy) {
    ClRcT rc;
    ClUint32T i;
    ClUint32T success = 0;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    if (pIocCommPort == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port id\n"));
        goto out;
    }

    for (i = 0; i < numReplicasts; ++i) {
        ClIocAddressT interimDestAddress = { { 0 } };
        ClCharT *xportType = NULL;
        rc = clFindTransport(
                ((ClIocPhysicalAddressT*) &replicastList[i])->nodeAddress,
                &interimDestAddress, &xportType);

        if (rc != CL_OK || !interimDestAddress.iocPhyAddress.nodeAddress
                || !xportType) {
            clLogError(
                    "IOC",
                    "REPLICAST",
                    "Replicast to destination [%#x: %#x] failed with [%#x]", replicastList[i].iocPhyAddress.nodeAddress, replicastList[i].iocPhyAddress.portId, rc);
            continue;
        }

        userFragHeader->header.dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) &replicastList[i])->nodeAddress);
        userFragHeader->header.dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) &replicastList[i])->portId);

        if (interimDestAddress.iocPhyAddress.nodeAddress
                && interimDestAddress.iocPhyAddress.nodeAddress
                        != ((ClIocPhysicalAddressT*) &replicastList[i])->nodeAddress) {
            if (replicastList[i].iocPhyAddress.portId >= CL_IOC_XPORT_PORT)
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            else
                interimDestAddress.iocPhyAddress.portId =
                        replicastList[i].iocPhyAddress.portId;
        } else {
            interimDestAddress.iocPhyAddress.nodeAddress =
                    replicastList[i].iocPhyAddress.nodeAddress;
            interimDestAddress.iocPhyAddress.portId =
                    replicastList[i].iocPhyAddress.portId;
        }

        /*clLogTrace(
         "IOC",
         "REPLICAST",
         "Sending to destination [%#x: %#x] with [xport: %s]",
         interimDestAddress.iocPhyAddress.nodeAddress,
         interimDestAddress.iocPhyAddress.portId, xportType);*/

        rc = internalSend(pIocCommPort, target, targetVectors, messageLen,
                tempPriority, &interimDestAddress, pTimeout, xportType, proxy);

        if (rc != CL_OK) {
            clLogError(
                    "IOC",
                    "REPLICAST",
                    "Replicast to destination [%#x: %#x] failed with [%#x]", replicastList[i].iocPhyAddress.nodeAddress, replicastList[i].iocPhyAddress.portId, rc);
        } else {
            ++success;
        }
    }
    if (success > 0)
        rc = CL_OK;

    out: return rc;
}

static ClRcT internalSendSlow(ClIocCommPortT *pIocCommPort,
        ClBufferHandleT message, ClUint32T tempPriority,
        ClIocAddressT *pIocAddress, ClUint32T *pTimeout, ClCharT *xportType,
        ClBoolT proxy) {
    ClRcT rc = CL_OK;
    struct iovec *pIOVector = NULL;
    ClInt32T ioVectorLen = 0;
    ClUint32T priority;
    static ClInt32T recordIOCSend = -1;
    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if (pIocCommPort == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port id\n"));
        goto out;
    }

    rc = clBufferVectorize(message, &pIOVector, &ioVectorLen);
    if (rc != CL_OK) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error in buffer vectorize.rc=0x%x\n",rc));
        goto out;
    }
    priority = CL_IOC_DEFAULT_PRIORITY;
    if (pIocCommPort->portId == CL_IOC_CPM_PORT
            || pIocCommPort->portId == CL_IOC_XPORT_PORT
            || tempPriority == CL_IOC_HIGH_PRIORITY) {
        priority = CL_IOC_HIGH_PRIORITY;
    }

    /*
     * If the leaky bucket is defined, then 
     * respect the traffic signal
     */
    if (gClIocTrafficShaper && gClLeakyBucket) {
        ClUint32T len = 0;
        clBufferLengthGet(message, &len);
        clLeakyBucketFill(gClLeakyBucket, len);
    }

    if (recordIOCSend < 0) {
        ClBoolT record = clParseEnvBoolean("CL_ASP_RECORD_IOC_SEND");
        recordIOCSend = record ? 1 : 0;
    }

    if (recordIOCSend)
        clTaskPoolRecordIOCSend(CL_TRUE);

    rc = clTransportSendProxy(xportType, pIocCommPort->portId, priority,
            pIocAddress, pIOVector, ioVectorLen, 0, proxy);

    if (recordIOCSend)
        clTaskPoolRecordIOCSend(CL_FALSE);

    clHeapFree(pIOVector);

    out: return rc;
}

static ClRcT internalSendSlowReplicast(ClIocCommPortT *pIocCommPort,
        ClBufferHandleT message, ClUint32T tempPriority, ClUint32T *pTimeout,
        ClIocAddressT *replicastList, ClUint32T numReplicasts,
        ClIocHeaderT *userHeader, ClBoolT proxy) {
    ClRcT rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    ClUint32T i;
    ClUint32T success = 0;
    ClUint32T msgLength;

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK || msgLength == 0) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Failed to get the length of the message. error code 0x%x", rc));
        rc = CL_IOC_RC(CL_ERR_INVALID_BUFFER);
        goto out;
    }

    if (!pIocCommPort) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port\n"));
        goto out;
    }
    for (i = 0; i < numReplicasts; ++i) {
        ClIocAddressT interimDestAddress = { { 0 } };
        ClCharT *xportType = NULL;
        rc = clFindTransport(
                ((ClIocPhysicalAddressT*) &replicastList[i])->nodeAddress,
                &interimDestAddress, &xportType);

        if ((rc != CL_OK) || (!xportType)) {
            clLogError(
                    "IOC",
                    "REPLICAST",
                    "Replicast to destination [%#x: %#x] failed with [%#x]", replicastList[i].iocPhyAddress.nodeAddress, replicastList[i].iocPhyAddress.portId, rc);
            continue;
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if (rc != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nERROR: Buffer header trim failed RC = 0x%x\n", rc));
            continue;
        }

        userHeader->dstAddress.iocPhyAddress.nodeAddress = htonl(
                ((ClIocPhysicalAddressT *) &replicastList[i])->nodeAddress);
        userHeader->dstAddress.iocPhyAddress.portId = htonl(
                ((ClIocPhysicalAddressT *) &replicastList[i])->portId);

        rc = clBufferDataPrepend(message, (ClUint8T *) userHeader,
                sizeof(ClIocHeaderT));
        if (rc != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: Prepend buffer data failed = 0x%x", rc));
            continue;
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress
                && (interimDestAddress.iocPhyAddress.nodeAddress
                        != ((ClIocPhysicalAddressT*) &replicastList[i])->nodeAddress)) {
            if (replicastList[i].iocPhyAddress.portId >= CL_IOC_XPORT_PORT) {
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            } else {
                interimDestAddress.iocPhyAddress.portId =
                        replicastList[i].iocPhyAddress.portId;
            }
        } else {
            interimDestAddress.iocPhyAddress.nodeAddress =
                    replicastList[i].iocPhyAddress.nodeAddress;
            interimDestAddress.iocPhyAddress.portId =
                    replicastList[i].iocPhyAddress.portId;
        }

        /*clLogTrace(
         "IOC",
         "REPLICAST",
         "Sending to destination [%#x: %#x] with [xport: %s]",
         interimDestAddress.iocPhyAddress.nodeAddress,
         interimDestAddress.iocPhyAddress.portId, xportType);*/

        rc = internalSendSlow(pIocCommPort, message, tempPriority,
                &interimDestAddress, pTimeout, xportType, proxy);

        if (rc != CL_OK) {
            clLogError(
                    "IOC",
                    "REPLICAST",
                    "Slow send to destination [%#x: %#x] failed with [%#x]", replicastList[i].iocPhyAddress.nodeAddress, replicastList[i].iocPhyAddress.portId, rc);
        } else {
            ++success;
        }

    }
    if (success > 0)
        rc = CL_OK;

    out: return rc;
}

ClRcT clIocDispatch(const ClCharT *xportType, ClIocCommPortHandleT commPort,
        ClIocDispatchOptionT *pRecvOption, ClUint8T *buffer, ClUint32T bufSize,
        ClBufferHandleT message, ClIocRecvParamT *pRecvParam) {
    ClRcT rc = CL_OK;
    ClIocHeaderT userHeader = { 0 };
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT*) commPort;
    ClUint32T size = sizeof(ClIocHeaderT);
    ClUint8T *pBuffer = buffer;
    ClInt32T bytes = bufSize;
    ClBoolT relay = CL_FALSE;
    ClBoolT syncReassembly = CL_FALSE;
    static ClIocDispatchOptionT recvOption = {
            .timeout = CL_IOC_TIMEOUT_FOREVER, .sync = CL_FALSE, };

#ifdef CL_IOC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    if (pIocCommPort == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid ioc commport\n"));
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    if (!pRecvOption)
        pRecvOption = &recvOption;

    syncReassembly = pRecvOption->sync;

    if (bytes <= size) {
        /*Check for port exit message*/
        if (bytes == sizeof(CL_IOC_PORT_EXIT_MESSAGE)
                && !strncmp((ClCharT*) buffer, CL_IOC_PORT_EXIT_MESSAGE,
                        sizeof(CL_IOC_PORT_EXIT_MESSAGE))) {
            ClTimerTimeOutT waitTime = { .tsSec = 0, .tsMilliSec = 200 };
            CL_DEBUG_PRINT(
                    CL_DEBUG_INFO,
                    ("PORT EXIT MESSAGE received for portid:0x%x,EO [%s]\n",pIocCommPort->portId,CL_EO_NAME));
            rc = CL_IOC_RC(CL_IOC_ERR_RECV_UNBLOCKED);
            clOsalMutexLock(&pIocCommPort->unblockMutex);
            if (!pIocCommPort->blocked) {
                clOsalMutexUnlock(&pIocCommPort->unblockMutex);
                goto out;
            }
            clOsalCondSignal(&pIocCommPort->unblockCond);
            clOsalCondWait(&pIocCommPort->recvUnblockCond,
                    &pIocCommPort->unblockMutex, waitTime);
            clOsalMutexUnlock(&pIocCommPort->unblockMutex);
            goto out;
        }CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
        "The packet is an invalid or a corrupted one. "
        "Packet size if %d, rc = %x\n", bytes, rc));
        goto out;
    }
    memcpy((ClPtrT) &userHeader, (ClPtrT) buffer, sizeof(ClIocHeaderT));
    if (userHeader.version != CL_IOC_HEADER_VERSION) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Dropping received packet of version [%d] messageId [%d] source [%d]. Supported version [%d]\n", userHeader.version,userHeader.messageId,(int)userHeader.srcAddress.iocPhyAddress.nodeAddress,CL_IOC_HEADER_VERSION));
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

#ifdef RELIABLE_IOC
    if (userHeader.isReliable == CL_TRUE)
    {
        userHeader.messageId = ntohl(userHeader.messageId);
        ClUint32T userheaderMessageId = userHeader.messageId;
        //only for testing drop message
        test++;
        if ((test % 6)==0 )
        {
            clLogDebug("IOC","Rel","Dropping a received packet number [%d] for testing reliable", test);
            return CL_IOC_RC(IOC_MSG_QUEUED);
        }
    }
    else
    {
//        clLogDebug("IOC", "Rel", "Receiver IOC message sync without reliable");
    }
#endif

    userHeader.srcAddress.iocPhyAddress.nodeAddress = ntohl(
            userHeader.srcAddress.iocPhyAddress.nodeAddress);
    userHeader.srcAddress.iocPhyAddress.portId = ntohl(
            userHeader.srcAddress.iocPhyAddress.portId);
    userHeader.dstAddress.iocPhyAddress.nodeAddress = ntohl(
            userHeader.dstAddress.iocPhyAddress.nodeAddress);
    userHeader.dstAddress.iocPhyAddress.portId = ntohl(
            userHeader.dstAddress.iocPhyAddress.portId);

    /*
     * Check to forward this message. Switch to synchronous recvs or reassembly of fragments
     */
    if (CL_IOC_ADDRESS_TYPE_GET(&userHeader.dstAddress)
            == CL_IOC_PHYSICAL_ADDRESS_TYPE) {
        if (userHeader.dstAddress.iocPhyAddress.nodeAddress
                != gIocLocalBladeAddress &&
                userHeader.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS) {

relay        = CL_TRUE;
        if(userHeader.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
        {
            if(!clTransportBridgeEnabled(gIocLocalBladeAddress))
            {
                relay = CL_FALSE;
            }
        }
        else
        {
            /*
             * We don't touch the passed structure as user-provided option
             * can be re-used for other packets
             */
            if(!syncReassembly)
            syncReassembly = CL_TRUE;
        }
    }
}

    if (clEoWithOutCpm
            || userHeader.srcAddress.iocPhyAddress.nodeAddress
                    != gIocLocalBladeAddress) {
        if ((rc = clIocCompStatusSet(userHeader.srcAddress.iocPhyAddress,
                CL_IOC_NODE_UP)) != CL_OK)

        {
            ClUint32T packetSize;

            packetSize = bytes
                    - ((userHeader.flag == 0) ?
                            sizeof(ClIocHeaderT) : sizeof(ClIocFragHeaderT));

            CL_DEBUG_PRINT(
                    CL_DEBUG_CRITICAL,
                    ("Dropping a received packet."
                    "Failed to SET the staus of the packet-sender-component "
                    "[node 0x%x : port 0x%x]. Packet size is %d. error code 0x%x ", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, packetSize, rc));
            rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
            goto out;
        }
    }

    if (userHeader.flag == 0) {
#ifdef CL_IOC_COMPRESSION
        ClTimeT pktRecvTime = 0;
        ClUint32T compressionFlag = ntohl(userHeader.reserved);
        ClUint8T *decompressedStream = NULL;
        ClUint32T decompressedStreamLen = 0;
        ClUint32T sentBytes = 0;
#endif
        pBuffer = buffer + sizeof(ClIocHeaderT);
        bytes -= sizeof(ClIocHeaderT);

#ifdef CL_IOC_COMPRESSION
        sentBytes = bytes;
        if(compressionFlag)
        {
            /*
             * decompress. the stream.
             */
            rc = doDecompress(pBuffer, bytes, &decompressedStream, &decompressedStreamLen);
            CL_ASSERT(rc == CL_OK);
            pBuffer = decompressedStream;
            bytes = decompressedStreamLen;
        }
        rc = clBufferNBytesWrite(message, pBuffer, bytes);

        if(pBuffer == decompressedStream)
        {
            clHeapFree(pBuffer);
        }
#else
        if (clBufferAppendHeap(message, buffer,
                bytes + sizeof(ClIocHeaderT)) != CL_OK) {
            rc = clBufferNBytesWrite(message, pBuffer, bytes);
        } else {
            /*
             * Trim the header if the recv buffer is stitched.
             */
            rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));

        }
#endif
        if (rc != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
            "Failed to write to a buffer message. "
            "Packet Size is %d. error code 0x%x", bytes, rc));
            goto out;
        }

#ifdef CL_IOC_COMPRESSION
        pktSendTime = clNtohl64(userHeader.pktTime);
        if(pktSendTime)
        {
            ClTimeValT tv = {0};
            clTimeServerGet(NULL, &tv);
            pktRecvTime = tv.tvSec*1000000LL + tv.tvUsec;
            if(pktRecvTime)
            clLogDebug("IOC", "RECV", "Packet round trip time [%lld] usecs for bytes [%d]",
                    pktRecvTime - pktSendTime, sentBytes);
        }
#endif

        /* 
         * Hoping that the notification packet will not exceed 64K packet size :-). 
         */
        if (pIocCommPort->notify == CL_IOC_NOTIFICATION_DISABLE
                && (userHeader.protocolType == CL_IOC_PORT_NOTIFICATION_PROTO
                        || userHeader.protocolType == CL_IOC_PROTO_ARP)) {
            clBufferClear(message);
            rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
            goto out;
        }
    } else {
        ClIocFragHeaderT userFragHeader;

        memcpy((ClPtrT) &userFragHeader, (ClPtrT) buffer,
                sizeof(ClIocFragHeaderT));

        pBuffer = buffer + sizeof(ClIocFragHeaderT);
        bytes -= sizeof(ClIocFragHeaderT);

        userFragHeader.msgId = ntohl(userFragHeader.msgId);
        userFragHeader.fragOffset = ntohl(userFragHeader.fragOffset);
        userFragHeader.fragLength = ntohl(userFragHeader.fragLength);

        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = ntohl(
                userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = ntohl(
                userFragHeader.header.srcAddress.iocPhyAddress.portId);

        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = ntohl(
                userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = ntohl(
                userFragHeader.header.dstAddress.iocPhyAddress.portId);

        CL_DEBUG_PRINT(
                CL_DEBUG_TRACE,
                ("Got these values fragid %d, frag offset %d, fraglength %d, "
                "flag %x from 0x%x:0x%x at 0x%x:0x%x\n", (userFragHeader.msgId), (userFragHeader.fragOffset), (userFragHeader.fragLength), userFragHeader.header.flag, userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress, userFragHeader.header.srcAddress.iocPhyAddress.portId, gIocLocalBladeAddress, pIocCommPort->portId));

        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if (userFragHeader.header.flag == IOC_LAST_FRAG)
            clLogTrace(
                    "FRAG",
                    "RECV",
                    "Got Last frag at offset [%d], size [%d], received [%d]", userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(xportType, pBuffer, &userFragHeader,
                pIocCommPort->portId, bytes, message, syncReassembly);
        if (rc != CL_OK)
            goto out;
    }

    /*
     * Now that the message is accumulated, forward if required to the actual destination.
     */
    if (relay) {
        ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                .timeout = 0 };
        if (userHeader.dstAddress.iocPhyAddress.nodeAddress
                == CL_IOC_BROADCAST_ADDRESS) {
            ClIocAddressT *bcastList = NULL;
            ClUint32T numBcasts = 0;
            /*
             * Check if we have a proxy broadcast list 
             */
            if (clTransportBroadcastListGet(xportType,
                    &userHeader.srcAddress.iocPhyAddress, &numBcasts,
                    &bcastList) == CL_OK) {
                ClUint32T i;
                for (i = 0; i < numBcasts; ++i) {
                    /*
                     * Broadcast proxy and continue with message processing
                     */
                    clLogDebug(
                            "PROXY",
                            "RELAY",
                            "Broadcast message from node [%d], port [%d] "
                            "to node [%d], port [%d]", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, bcastList[i].iocPhyAddress.nodeAddress, bcastList[i].iocPhyAddress.portId);
                    clIocSendWithXportRelay(commPort, message,
                            userHeader.protocolType, &userHeader.srcAddress,
                            &bcastList[i], &sendOption, (ClCharT*) xportType,
                            CL_FALSE);
                    clBufferReadOffsetSet(message, 0, CL_BUFFER_SEEK_SET);
                }
                if (bcastList)
                    clHeapFree(bcastList);
            }
        } else {
            clLogDebug(
                    "PROXY",
                    "RELAY",
                    "Forward message from node [%d], port [%d] "
                    "to node [%d], port [%d]", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, userHeader.dstAddress.iocPhyAddress.nodeAddress, userHeader.dstAddress.iocPhyAddress.portId);

            clIocSendWithRelay(commPort, message, userHeader.protocolType,
                    &userHeader.srcAddress, &userHeader.dstAddress,
                    &sendOption);
            /*
             * Clear the message buffer for re-use.
             */
            clBufferClear(message);
            rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
            goto out;
        }
    }

    /*
     * Got heartbeat reply from other local components
     */
    if (userHeader.protocolType == CL_IOC_PROTO_ICMP) {
        clIocHearBeatHealthCheckUpdate(
                userHeader.srcAddress.iocPhyAddress.nodeAddress,
                userHeader.srcAddress.iocPhyAddress.portId, NULL);
        clBufferClear(message);
        return CL_IOC_RC(CL_ERR_TRY_AGAIN);
    }

    /*
     * Got heartbeat request from a amf component
     */
    if (userHeader.protocolType == CL_IOC_PROTO_HB) {
        /*
         * Reply HeartBeat message
         */
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

        ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                .timeout = 200 };
        clIocSend(commPort, message, CL_IOC_PROTO_ICMP, &destAddress,
                &sendOption);
        /*
         * Clear the message buffer for re-use.
         */
        clBufferClear(message);
        return CL_IOC_RC(CL_ERR_TRY_AGAIN);
    }

#ifdef RELIABLE_IOC
    //Sender process data control from receiver
    if (userHeader.protocolType == CL_IOC_SEND_ACK_PROTO)
     {
        pBuffer = buffer + sizeof(ClIocHeaderT);
//        clLogDebug("XPORT", "RECV", "Dispatch Received ACK data with size [%d]",bytes);
        if (!pBuffer) {
            clLogDebug("XPORT", "RECV", "ACK fragment data buffer invalid");
            goto out;
        }
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId =
                userHeader.srcAddress.iocPhyAddress.portId;
        ClUint32T portId = userHeader.dstAddress.iocPhyAddress.portId;
        if (bytes > 8) {
            goto out;
            clLogDebug("XPORT", "RECV", "ACK data fragment size invalid");
        }
        ClUint32T messageId;
        ClUint32T fragmentId;
        memcpy(&messageId, pBuffer, sizeof(ClUint32T));
        ClUint8T *pBuffer1 = pBuffer;
        pBuffer1 = pBuffer + sizeof(ClUint32T);
        memcpy(&fragmentId, pBuffer1, sizeof(ClUint32T));
        clOsalMutexLock(&iocSenderLock);
        senderBufferACKCallBack(ntohl(messageId), &destAddress, ntohl(fragmentId),
                portId);
        clOsalMutexUnlock(&iocSenderLock);
        clBufferClear(message);
        return CL_OK;

    }
    if (userHeader.protocolType == CL_IOC_SEND_ACK_MESSAGE_PROTO) {
        pBuffer = buffer + sizeof(ClIocHeaderT);
        clLogDebug("XPORT", "RECV", "Dispatch Received ACK data with size [%d]",bytes);
        if (!pBuffer) {
            clLogDebug("XPORT", "RECV",
                    "iocDispatch : message ACK data buffer invalid");
            goto out;
        }
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId =
                userHeader.srcAddress.iocPhyAddress.portId;
        ClUint32T portId = userHeader.dstAddress.iocPhyAddress.portId;
        if (bytes > 8) {
            goto out;
            clLogDebug("XPORT", "RECV",
                    "iocDispatch :message ACK data size invalid");
        }
        ClUint32T messageId;
        memcpy(&messageId, pBuffer, sizeof(ClUint32T));
        clLogDebug(
                "XPORT",
                "RECV",
                "iocDispatch : Received message ACK of messageId [%d]. Delete from sender buffer", ntohl(messageId));
        rc = senderBufferACKCallBack(ntohl(messageId), &destAddress, 0, portId);
        clBufferClear(message);
        return CL_OK;
    }

    if (userHeader.protocolType == CL_IOC_SEND_NAK_PROTO) {
        //process NAK IOC event
        pBuffer = buffer + sizeof(ClIocHeaderT);
        if (!pBuffer) {
            clLogDebug("XPORT", "RECV", "NAK data buffer invalid");
            goto out;
        }
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId =
                userHeader.srcAddress.iocPhyAddress.portId;
        ClUint32T portId = userHeader.dstAddress.iocPhyAddress.portId;
        ClUint32T messageId;
        memcpy(&messageId, pBuffer, sizeof(ClUint32T));
        ClUint8T *ppBuffer = pBuffer;
        ppBuffer = pBuffer + sizeof(ClUint32T);
        bytes -= sizeof(ClUint32T);
        clLogDebug("XPORT", "RECV", "Dispatch Received NAK data messageId [%d] size [%d]",ntohl(messageId),(ClUint32T)(bytes/sizeof(ClUint32T)));
        senderBufferNAKCallBack(ntohl(messageId), &destAddress, portId,
                (ClUint32T*) ppBuffer, bytes / sizeof(ClUint32T));

        clBufferClear(message);
        return CL_OK;

    }
    if (userHeader.protocolType == CL_IOC_DROP_REQUEST_PROTO) {
        clLogDebug("XPORT", "RECV", "Dispatch Received Drop request");
        pBuffer = buffer + sizeof(ClIocHeaderT);
        bytes -= sizeof(ClIocHeaderT);
        if (!pBuffer) {
            clLogDebug("XPORT", "RECV", "Drop request data buffer invalid");
            goto out;
        }
        ClIocAddressT srcAddress = { { 0 } };
        srcAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        srcAddress.iocPhyAddress.portId =
                userHeader.srcAddress.iocPhyAddress.portId;
        ClUint32T portId = userHeader.dstAddress.iocPhyAddress.portId;
        if (bytes > 4) {
            clLogDebug("XPORT", "RECV", "Drop request data size invalid");
            goto out;
        }
        ClUint32T messageId = ((ClUint32T*) pBuffer)[0];
        clOsalMutexLock(&iocReassembleLock);
        receiverDropMsgCallback(&srcAddress, messageId, pIocCommPort->portId);
        clOsalMutexUnlock(&iocReassembleLock);
        //process NAK IOC event
        return CL_OK;
    }
#endif

    pRecvParam->length = bytes;
    pRecvParam->priority = userHeader.priority;
    pRecvParam->protoType = userHeader.protocolType;
    memcpy(&pRecvParam->srcAddr, &userHeader.srcAddress,
            sizeof(pRecvParam->srcAddr));
#ifdef RELIABLE_IOC
    if (userHeader.isReliable == CL_TRUE && userHeader.flag == 0)
    {
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId =
                userHeader.srcAddress.iocPhyAddress.portId;
        ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                .timeout = 200 };
        clLogDebug("IOC","Rel","Receiver IOC message sync  with message ID [%d], port [%d]", userHeader.messageId, userHeader.srcAddress.iocPhyAddress.portId);
        receiverAckSend(pIocCommPort, &destAddress, userHeader.messageId, 0);
    }
#endif
//    clLogTrace(
//            "XPORT",
//            "RECV",
//            "Received message of size [%d] and protocolType [0x%x] from node [0x%x:0x%x]", bytes, userHeader.protocolType, userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId);
//    clLogDebug("IOC", "Rel", "Received message of size [%d] and protocolType [0x%x] from node [0x%x:0x%x]", bytes, userHeader.protocolType, userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId);

    out: return rc;
}

ClRcT clIocDispatchAsync(const ClCharT *xportType, ClIocPortT port,
        ClUint8T *buffer, ClUint32T bufSize) {
    ClRcT rc = CL_OK;
    ClIocHeaderT userHeader = { 0 };
    ClUint32T size = sizeof(ClIocHeaderT);
    ClUint8T *pBuffer = buffer;
    ClInt32T bytes = bufSize;
    ClIocRecvParamT recvParam = { 0 };
    ClBufferHandleT message = 0;
    ClBoolT relay = CL_FALSE;
#ifdef CL_IOC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    rc = clBufferCreate(&message);
    if (rc != CL_OK)
        goto out;

    if (bytes <= size) {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
        "The packet is an invalid or a corrupted one. "
        "Packet size received [%d]", bytes));
        goto out;
    }
    memcpy((ClPtrT) &userHeader, (ClPtrT) buffer, sizeof(ClIocHeaderT));

    if (userHeader.version != CL_IOC_HEADER_VERSION) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Dropping received packet of version [%d]. Supported version [%d]\n", userHeader.version, CL_IOC_HEADER_VERSION));
        goto out;
    }
#ifdef RELIABLE_IOC
    userHeader.messageId = ntohl(userHeader.messageId);
    if (userHeader.isReliable == CL_TRUE) {
        clLogDebug(
                "IOC",
                "Rel",
                "Receiver IOC message async reliable messageId [%d]", userHeader.messageId);
    } else {
        //clLogDebug("IOC", "Rel", "Receiver IOC message async without reliable");
    }
#endif

    userHeader.srcAddress.iocPhyAddress.nodeAddress = ntohl(
            userHeader.srcAddress.iocPhyAddress.nodeAddress);
    userHeader.srcAddress.iocPhyAddress.portId = ntohl(
            userHeader.srcAddress.iocPhyAddress.portId);
    userHeader.dstAddress.iocPhyAddress.nodeAddress = ntohl(
            userHeader.dstAddress.iocPhyAddress.nodeAddress);
    userHeader.dstAddress.iocPhyAddress.portId = ntohl(
            userHeader.dstAddress.iocPhyAddress.portId);
    userHeader.messageId = ntohl(userHeader.messageId);
    /*
     * Check to forward this message. Switch to synchronous recvs or reassembly of fragments
     */
    if (CL_IOC_ADDRESS_TYPE_GET(&userHeader.dstAddress)
            == CL_IOC_PHYSICAL_ADDRESS_TYPE) {
        if (userHeader.dstAddress.iocPhyAddress.nodeAddress
                != CL_IOC_RESERVED_ADDRESS
                && userHeader.dstAddress.iocPhyAddress.nodeAddress
                        != gIocLocalBladeAddress) {
            relay = CL_TRUE;
            if (userHeader.dstAddress.iocPhyAddress.nodeAddress
                    == CL_IOC_BROADCAST_ADDRESS
                    && !clTransportBridgeEnabled(gIocLocalBladeAddress)) {
                relay = CL_FALSE;
            }
        }
    }

    if (clEoWithOutCpm
            || userHeader.srcAddress.iocPhyAddress.nodeAddress
                    != gIocLocalBladeAddress) {
        if ((rc = clIocCompStatusSet(userHeader.srcAddress.iocPhyAddress,
                CL_IOC_NODE_UP)) != CL_OK)

        {
            ClUint32T packetSize;

            packetSize = bytes
                    - ((userHeader.flag == 0) ?
                            sizeof(ClIocHeaderT) : sizeof(ClIocFragHeaderT));

            CL_DEBUG_PRINT(
                    CL_DEBUG_CRITICAL,
                    ("Dropping a received packet."
                    "Failed to SET the staus of the packet-sender-component "
                    "[node 0x%x : port 0x%x]. Packet size is %d. error code 0x%x ", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, packetSize, rc));
            goto out;
        }
    }

    if (userHeader.flag == 0) {
#ifdef CL_IOC_COMPRESSION
        ClTimeT pktRecvTime = 0;
        ClUint32T compressionFlag = ntohl(userHeader.reserved);
        ClUint8T *decompressedStream = NULL;
        ClUint32T decompressedStreamLen = 0;
        ClUint32T sentBytes = 0;
#endif
        pBuffer = buffer + sizeof(ClIocHeaderT);
        bytes -= sizeof(ClIocHeaderT);

#ifdef CL_IOC_COMPRESSION
        sentBytes = bytes;
        if(compressionFlag)
        {
            /*
             * decompress. the stream.
             */
            rc = doDecompress(pBuffer, bytes, &decompressedStream, &decompressedStreamLen);
            CL_ASSERT(rc == CL_OK);
            pBuffer = decompressedStream;
            bytes = decompressedStreamLen;
        }
#endif
        rc = clBufferNBytesWrite(message, pBuffer, bytes);

#ifdef CL_IOC_COMPRESSION
        if(pBuffer == decompressedStream)
        {
            clHeapFree(pBuffer);
        }
#endif

        if (rc != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
            "Failed to write to a buffer message. "
            "Packet Size is %d. error code 0x%x", bytes, rc));
            goto out;
        }

#ifdef CL_IOC_COMPRESSION
        pktSendTime = clNtohl64(userHeader.pktTime);
        if(pktSendTime)
        {
            ClTimeValT tv = {0};
            clTimeServerGet(NULL, &tv);
            pktRecvTime = tv.tvSec*1000000LL + tv.tvUsec;
            if(pktRecvTime)
            clLogDebug("IOC", "RECV", "Packet round trip time [%lld] usecs for bytes [%d]",
                    pktRecvTime - pktSendTime, sentBytes);
        }
#endif
    } else {
        ClIocFragHeaderT userFragHeader;

        memcpy((ClPtrT) &userFragHeader, (ClPtrT) buffer,
                sizeof(ClIocFragHeaderT));

        pBuffer = buffer + sizeof(ClIocFragHeaderT);
        bytes -= sizeof(ClIocFragHeaderT);

        userFragHeader.msgId = ntohl(userFragHeader.msgId);
        userFragHeader.fragOffset = ntohl(userFragHeader.fragOffset);
        userFragHeader.fragLength = ntohl(userFragHeader.fragLength);

        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = ntohl(
                userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = ntohl(
                userFragHeader.header.srcAddress.iocPhyAddress.portId);

        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = ntohl(
                userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = ntohl(
                userFragHeader.header.dstAddress.iocPhyAddress.portId);

        CL_DEBUG_PRINT(
                CL_DEBUG_TRACE,
                ("Got these values fragid %d, frag offset %d, fraglength %d, "
                "flag %x from 0x%x:0x%x at 0x%x:0x%x\n", (userFragHeader.msgId), (userFragHeader.fragOffset), (userFragHeader.fragLength), userFragHeader.header.flag, userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress, userFragHeader.header.srcAddress.iocPhyAddress.portId, gIocLocalBladeAddress, port));

        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if (userFragHeader.header.flag == IOC_LAST_FRAG)
            clLogTrace(
                    "FRAG",
                    "RECV",
                    "Got Last frag at offset [%d], size [%d], received [%d]", userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(xportType, pBuffer, &userFragHeader, port,
                bytes, message, CL_FALSE);
        /*
         * recalculate timeouts
         */
        if (rc != CL_OK) {
            if (rc == CL_IOC_RC(IOC_MSG_QUEUED))
                rc = CL_OK;
            else {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,
                        ("Dropping a received fragmented-packet. "
                        "Failed to reassemble the packet. Packet size is %d. "
                        "error code 0x%x", bytes, rc));
            }
        }
        goto out;
    }

    /*
     * Now that the message is accumulated, forward if required to the actual destination.
     */
    if (relay) {
        ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                .timeout = 0 };
        ClIocCommPortT *commPort = clIocGetPort(port);
        if (commPort) {
            if (userHeader.dstAddress.iocPhyAddress.nodeAddress
                    == CL_IOC_BROADCAST_ADDRESS) {
                ClIocAddressT *bcastList = NULL;
                ClUint32T numBcasts = 0;
                /*
                 * Check if we have a proxy broadcast list 
                 */
                if (clTransportBroadcastListGet(xportType,
                        &userHeader.srcAddress.iocPhyAddress, &numBcasts,
                        &bcastList) == CL_OK) {
                    ClUint32T i;
                    for (i = 0; i < numBcasts; ++i) {
                        /*
                         * Broadcast proxy and continue with message processing
                         */
                        clLogDebug(
                                "PROXY",
                                "RELAY",
                                "Broadcast message from node [%d], port [%d] "
                                "to node [%d], port [%d], xport [%s]", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, bcastList[i].iocPhyAddress.nodeAddress, bcastList[i].iocPhyAddress.portId, xportType);
                        clIocSendWithXportRelay((ClIocCommPortHandleT) commPort,
                                message, userHeader.protocolType,
                                &userHeader.srcAddress, &bcastList[i],
                                &sendOption, (ClCharT*) xportType, CL_FALSE);
                        clBufferReadOffsetSet(message, 0, CL_BUFFER_SEEK_SET);
                    }
                    if (bcastList)
                        clHeapFree(bcastList);
                }
                goto cont;
            } else {
                clLogDebug(
                        "PROXY",
                        "RELAY",
                        "Forward message from node [%d], port [%d] "
                        "to node [%d], port [%d]", userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, userHeader.dstAddress.iocPhyAddress.nodeAddress, userHeader.dstAddress.iocPhyAddress.portId);
                clIocSendWithRelay((ClIocCommPortHandleT) commPort, message,
                        userHeader.protocolType, &userHeader.srcAddress,
                        &userHeader.dstAddress, &sendOption);
            }
        } else {
            clLogError(
                    "PROXY",
                    "RELAY",
                    "Unable to forward message as comm port [%d] look up failed", port);
        }
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    cont:
    /*
     * Got heartbeat reply from other local components
     */
    if (userHeader.protocolType == CL_IOC_PROTO_ICMP) {
        clIocHearBeatHealthCheckUpdate(
                userHeader.srcAddress.iocPhyAddress.nodeAddress,
                userHeader.srcAddress.iocPhyAddress.portId, NULL);
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    /*
     * Got heartbeat request from a amf component
     */
    if (userHeader.protocolType == CL_IOC_PROTO_HB) {
        /*
         * Reply HeartBeat message
         */
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

        ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                .timeout = 200 };
        ClIocCommPortT *commPort = clIocGetPort(port);
        if (commPort) {
            clIocSend((ClIocCommPortHandleT) commPort, message,
                    CL_IOC_PROTO_ICMP, &destAddress, &sendOption);
        }
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }
    recvParam.length = bytes;
    recvParam.priority = userHeader.priority;
    recvParam.protoType = userHeader.protocolType;
    memcpy(&recvParam.srcAddr, &userHeader.srcAddress,
            sizeof(recvParam.srcAddr));
    clLogTrace(
            "XPORT",
            "RECV",
            "Received message of size [%d] and protocolType [0x%x] from node [0x%x:0x%x]", bytes, userHeader.protocolType, recvParam.srcAddr.iocPhyAddress.nodeAddress, recvParam.srcAddr.iocPhyAddress.portId);
    clEoEnqueueReassembleJob(message, &recvParam);
    message = 0;

    out: if (message) {
        clBufferDelete(&message);
    }
    return rc;
}

ClRcT clIocReceive(ClIocCommPortHandleT commPortHdl,
        ClIocRecvOptionT *pRecvOption, ClBufferHandleT message,
        ClIocRecvParamT *pRecvParam) {
    ClIocDispatchOptionT dispatchOption = { .timeout = CL_IOC_TIMEOUT_FOREVER,
            .sync = CL_TRUE };
    if (pRecvOption) {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, NULL, 0, message,
            pRecvParam);
}

ClRcT clIocReceiveAsync(ClIocCommPortHandleT commPortHdl,
        ClIocRecvOptionT *pRecvOption, ClBufferHandleT message,
        ClIocRecvParamT *pRecvParam) {
    ClIocDispatchOptionT dispatchOption = { .timeout = CL_IOC_TIMEOUT_FOREVER,
            .sync = CL_FALSE };
    if (pRecvOption) {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, NULL, 0, message,
            pRecvParam);
}

ClRcT clIocReceiveWithBuffer(ClIocCommPortHandleT commPortHdl,
        ClIocRecvOptionT *pRecvOption, ClUint8T *buffer, ClUint32T bufSize,
        ClBufferHandleT message, ClIocRecvParamT *pRecvParam) {
    ClIocDispatchOptionT dispatchOption = { .timeout = CL_IOC_TIMEOUT_FOREVER,
            .sync = CL_TRUE };
    if (!buffer)
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if (pRecvOption) {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, buffer, bufSize,
            message, pRecvParam);
}

ClRcT clIocReceiveWithBufferAsync(ClIocCommPortHandleT commPortHdl,
        ClIocRecvOptionT *pRecvOption, ClUint8T *buffer, ClUint32T bufSize,
        ClBufferHandleT message, ClIocRecvParamT *pRecvParam) {
    ClIocDispatchOptionT dispatchOption = { .timeout = CL_IOC_TIMEOUT_FOREVER,
            .sync = CL_FALSE };
    if (!buffer)
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if (pRecvOption) {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, buffer, bufSize,
            message, pRecvParam);
}

ClRcT clIocCommPortModeGet(ClIocCommPortHandleT iocCommPort,
        ClIocCommPortModeT *modeType) {
    NULL_CHECK(modeType);
    *modeType = CL_IOC_BLOCKING_MODE;
    return CL_OK;
}

ClRcT clIocLibFinalize() {
    if (gIocInit == CL_FALSE)
        return CL_OK;
    gIocInit = CL_FALSE;
    clJobQueueDelete(&iocFragmentJobQueue);
    clIocHeartBeatFinalize(gIsNodeRepresentative);
    clTransportFinalize(NULL, gIsNodeRepresentative);
    clNodeCacheFinalize();
    clIocNeighCompsFinalize();
    __iocFragmentPoolFinalize();
    clTransportLayerFinalize();
    clOsalMutexDelete(gClIocFragMutex);
    return CL_OK;
}

static ClRcT clIocLeakyBucketInitialize(void) {
    ClRcT rc = CL_OK;
    gClIocTrafficShaper = clParseEnvBoolean("CL_ASP_TRAFFIC_SHAPER");
    if (gClIocTrafficShaper) {
        char* temp;

        temp = getenv("CL_LEAKY_BUCKET_VOL");
        ClInt64T leakyBucketVol =
                temp ? (ClInt64T) atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_VOL;
        temp = getenv("CL_LEAKY_BUCKET_LEAK_SIZE");
        ClInt64T leakyBucketLeakSize =
                temp ? (ClInt64T) atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE;

        ClTimerTimeOutT leakyBucketInterval;
        temp = getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL");
        leakyBucketInterval.tsSec = 0;
        leakyBucketInterval.tsMilliSec =
                temp ? (ClInt64T) atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL;

        clLogInfo(
                "LKB",
                "INI",
                "Creating a leaky bucket with vol [%lld], leak size [%lld], interval [%d ms]", leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval.tsMilliSec);

        rc = clLeakyBucketCreate(leakyBucketVol, leakyBucketLeakSize,
                leakyBucketInterval, &gClLeakyBucket);
    }
    return rc;
}

ClRcT clIocConfigInitialize(ClIocLibConfigT *pConf) {
    ClRcT retCode;
#ifdef CL_IOC_COMPRESSION
    ClCharT *pIocCompressionBoundary = NULL;
#endif

    NULL_CHECK(pConf);

    if (gIocInit == CL_TRUE)
        return CL_OK;

    if (CL_IOC_PHYSICAL_ADDRESS_TYPE
            != CL_IOC_ADDRESS_TYPE_FROM_NODE_ADDRESS((pConf->nodeAddress))) {
        CL_DEBUG_PRINT(
                CL_DEBUG_CRITICAL,
                ("\nCritical : Invalid IOC address: Node Address [0x%x] is an invalid physical address.\n",pConf->nodeAddress));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }

    if ((CL_IOC_RESERVED_ADDRESS == pConf->nodeAddress)
            || (CL_IOC_BROADCAST_ADDRESS == pConf->nodeAddress)) {
        CL_DEBUG_PRINT(
                CL_DEBUG_CRITICAL,
                ("\nCritical : Invalid IOC address: Node Address [0x%x] is one of the reserved IOC addresses.\n ",pConf->nodeAddress));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }

    gIocLocalBladeAddress = ((ClIocLibConfigT *) pConf)->nodeAddress;

    ASP_NODEADDR = gIocLocalBladeAddress;

    clOsalMutexCreate(&gClIocFragMutex);

    memset(&userObj, 0, sizeof(ClIocUserObjectT));

    userReassemblyTimerExpiry.tsMilliSec = CL_IOC_REASSEMBLY_TIMEOUT % 1000;
    userReassemblyTimerExpiry.tsSec = CL_IOC_REASSEMBLY_TIMEOUT / 1000;
    userTTLTimerExpiry.tsMilliSec = CL_IOC_REASSEMBLY_TIMEOUT % 1000;
    userTTLTimerExpiry.tsSec = CL_IOC_REASSEMBLY_TIMEOUT / 1000;
    userResendTimerExpiry.tsMilliSec = 0;
    userResendTimerExpiry.tsSec = 1;
    ackTimerExpiry.tsSec = 0;
    ackTimerExpiry.tsMilliSec = 6;
    nakTimerExpiry.tsSec = 0;
    nakTimerExpiry.tsMilliSec = 5;
#ifdef RELIABLE_IOC
    clTriggerFakeDrop=20;
    clFakeDropCount=5;
#endif


    retCode = clIocNeighCompsInitialize(gIsNodeRepresentative);
    if (retCode != CL_OK) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error : Failed at neighbor initialize. rc=0x%x\n",retCode));
        goto error_1;
    }

    retCode = clNodeCacheInitialize(gIsNodeRepresentative);
    if (retCode != CL_OK) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Node cache initialize returned [%#x]", retCode));
        goto error_2;
    }

    clIocLeakyBucketInitialize();

    gClIocReplicast = clParseEnvBoolean("CL_ASP_IOC_REPLICAST");

#ifdef CL_IOC_COMPRESSION
    if( (pIocCompressionBoundary = getenv("CL_IOC_COMPRESSION_BOUNDARY") ) )
    {
        gIocCompressionBoundary = atoi(pIocCompressionBoundary);
    }

    if(!gIocCompressionBoundary)
    gIocCompressionBoundary = CL_IOC_COMPRESSION_BOUNDARY;
#endif

    if (gIsNodeRepresentative == CL_TRUE) {
        /*
         * Initialize a debug mode time server to fetch times from remote host.
         * for debugging.
         */
        clTimeServerInitialize();
    }

    retCode = clTransportLayerInitialize();
    if (retCode != CL_OK) {
        goto error_2;
    }

    retCode = clTransportInitialize(NULL, gIsNodeRepresentative);
    if (retCode != CL_OK) {
        goto error_3;
    }

    retCode = clTransportMaxPayloadSizeGet(NULL, &gClMaxPayloadSize);
    if (retCode != CL_OK) {
        goto error_3;
    }

    clIocHeartBeatInitialize(gIsNodeRepresentative);

    gIocInit = CL_TRUE;
    return CL_OK;

    error_3: clTransportLayerFinalize();
    error_2: clIocNeighCompsFinalize();
    error_1: clOsalMutexDelete(gClIocFragMutex);
    return retCode;
}

ClRcT clIocLibInitialize(ClPtrT pConfig) {
    ClIocLibConfigT iocConfig = { 0 };
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;

    clTaskPoolInitialize();

    iocConfig.version = CL_IOC_HEADER_VERSION;
    iocConfig.nodeAddress = clAspLocalId;

    retCode = clIocConfigInitialize(&iocConfig);
    if (retCode != CL_OK) {
        return retCode;
    }

    rc = clOsalMutexInit(&gClIocNeighborList.neighborMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClIocPortMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClIocCompMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClIocMcastMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&iocReassembleLock);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&iocAcklock);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&iocSenderLock);
        CL_ASSERT(rc == CL_OK);
    rc = clJobQueueInit(&iocFragmentJobQueue, 0, 1);
    CL_ASSERT(rc == CL_OK);
    rc = __iocFragmentPoolInitialize();
    CL_ASSERT(rc == CL_OK);
    /*Add ourselves into the neighbor table*/
    gClIocNeighborList.numEntries = 0;
    CL_LIST_HEAD_INIT(&gClIocNeighborList.neighborList);
    rc = clIocNeighborAdd(gIocLocalBladeAddress, CL_IOC_NODE_UP);
    CL_ASSERT(rc == CL_OK);
    return CL_OK;
}

ClRcT clIocCommPortReceiverUnblock(ClIocCommPortHandleT portHandle) {
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) portHandle;
    ClRcT rc = CL_OK;
    ClUint32T portId;
    ClTimerTimeOutT timeout = { .tsSec = 0, .tsMilliSec = 200 };
    ClInt32T tries = 0;
    static struct iovec exitVector = { .iov_base =
            (void*) CL_IOC_PORT_EXIT_MESSAGE, .iov_len =
            sizeof(CL_IOC_PORT_EXIT_MESSAGE), };
    ClIocAddressT destAddress;
    portId = pIocCommPort->portId;
    memset(&destAddress, 0, sizeof(destAddress));
    destAddress.iocPhyAddress.nodeAddress = gIocLocalBladeAddress;
    destAddress.iocPhyAddress.portId = pIocCommPort->portId;

    /*Grab the lock to avoid a race with lost wakeups triggered by the recv.*/
    clOsalMutexLock(&pIocCommPort->unblockMutex);
    ++pIocCommPort->blocked;
    if ((rc = clTransportSend(NULL, pIocCommPort->portId, CL_IOC_HIGH_PRIORITY,
            &destAddress, &exitVector, 1, 0)) != CL_OK) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Error sending port exit message to port:0x%x.errno=%d\n",portId,errno));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        clOsalMutexUnlock(&pIocCommPort->unblockMutex);
        goto out;
    }

    while (tries++ < 3) {
        rc = clOsalCondWait(&pIocCommPort->unblockCond,
                &pIocCommPort->unblockMutex, timeout);
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT) {
            continue;
        }
        break;
    }
    --pIocCommPort->blocked;
    /*
     * we come back with lock held.Now unblock the receiver 
     * irrespective of whether we succeded in cond wait or failed
     */
    clOsalCondSignal(&pIocCommPort->recvUnblockCond);

    clOsalMutexUnlock(&pIocCommPort->unblockMutex);

    out: return rc;
}

ClRcT clIocMaxPayloadSizeGet(ClUint32T *pSize) {
    NULL_CHECK(pSize);
    *pSize = 0xffffffff;
    return CL_OK;
}

ClRcT clIocMaxNumOfPrioritiesGet(ClUint32T *pMaxNumOfPriorities) {
    NULL_CHECK(pMaxNumOfPriorities);
    *pMaxNumOfPriorities = CL_IOC_MAX_PRIORITIES;
    return CL_OK;

}

ClRcT clIocCommPortGet(ClIocCommPortHandleT commPort, ClUint32T *portId) {
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT*) commPort;
    NULL_CHECK(portId);
    NULL_CHECK(pIocCommPort);
    *portId = pIocCommPort->portId;
    return CL_OK;
}

static __inline__ ClUint32T __iocReassembleHashKey(ClIocReassembleKeyT *key) {
    ClUint32T cksum = 0;
    clCksm32bitCompute((ClUint8T*) key, sizeof(*key), &cksum);
    return cksum & IOC_REASSEMBLE_HASH_MASK;
}

static ClIocReassembleNodeT *__iocReassembleNodeFind(ClIocReassembleKeyT *key,
        ClUint64T timerId) {
    ClUint32T hash = __iocReassembleHashKey(key);
    struct hashStruct *iter = NULL;
    for (iter = iocReassembleHashTable[hash]; iter; iter = iter->pNext) {
        ClIocReassembleNodeT *node = hashEntry(iter, ClIocReassembleNodeT, hash);
        if (timerId && node->timerKey->timerId != timerId)
            continue;
        if (!memcmp(&node->timerKey->key, key, sizeof(node->timerKey->key)))
            return node;
    }
    return NULL;
}

static ClInt32T __iocFragmentCmp(ClRbTreeT *node1, ClRbTreeT *node2) {
    ClIocFragmentNodeT *frag1 = CL_RBTREE_ENTRY(node1, ClIocFragmentNodeT, tree);
    ClIocFragmentNodeT *frag2 = CL_RBTREE_ENTRY(node2, ClIocFragmentNodeT, tree);
    return frag1->fragOffset - frag2->fragOffset;
}

static ClRcT __iocReassembleTimer(void *key) {
    ClIocReassembleNodeT *node = NULL;
    ClIocReassembleTimerKeyT *timerKey = key;
    ClRbTreeT *fragHead = NULL;
    ClTimerHandleT timer = NULL;

    clOsalMutexLock(&iocReassembleLock);
    node = __iocReassembleNodeFind(&timerKey->key, timerKey->timerId);
    if (!node) {
        goto out_unlock;
    }

    clLogTrace(
            "FRAG",
            "RECV",
            "Running the reassembly timer for sender node [%#x:%#x] with length [%d] bytes", node->timerKey->key.sendAddr.nodeAddress, node->timerKey->key.sendAddr.portId, node->currentLength);
    while ((fragHead = clRbTreeMin(&node->reassembleTree))) {
        ClIocFragmentNodeT *fragNode =
                CL_RBTREE_ENTRY(fragHead, ClIocFragmentNodeT, tree);
        clRbTreeDelete(&node->reassembleTree, fragHead);
        __iocFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
    node->timerKey = NULL; /* reset even if freeing parent memory for GOD's debugging :) */
#ifdef RELIABLE_IOC
    if(*(&node->isReliable)==CL_TRUE)
    {
        if (clTimerCheckAndDelete(&node->TimerACKHdl) == CL_OK)
        {
            clHeapFree(node->TimerACKHdl);
            node->TimerACKHdl = NULL;
        }
        destroyNodes(&node->receiverLossList);
        //clLogDebug("IOC", "Rel", "destroy receiver loss list ok");

    }
#endif
    clHeapFree(node);
    out_unlock: if ((timer = timerKey->reassembleTimer)) {
        timerKey->reassembleTimer = 0;
    }
    clOsalMutexUnlock(&iocReassembleLock);
    if (timer) {
        clTimerDelete(&timer);
    }

    clHeapFree(timerKey);
    return CL_OK;
}

static ClRcT __iocReassembleDispatch(const ClCharT *xportType,
        ClIocReassembleNodeT *node, ClIocFragHeaderT *fragHeader,
        ClBufferHandleT message, ClBoolT sync) {
    ClBufferHandleT msg = 0;
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_IOC_RC(IOC_MSG_QUEUED);
    ClRbTreeT *iter = NULL;
    ClUint32T len = 0;
    lastMessageId=node->messageId;
    lastMessageNode=node->srcAddress.iocPhyAddress.nodeAddress;
    //clLogDebug("IOC", "Rel", "Enter Ressemble Dispatch");
    if (message) {
        msg = message;
    } else {
        rc = clBufferCreate(&msg);
        CL_ASSERT(rc == CL_OK);
    }
    while ((iter = clRbTreeMin(&node->reassembleTree))) {
        ClIocFragmentNodeT *fragNode =
                CL_RBTREE_ENTRY(iter, ClIocFragmentNodeT, tree);
        if (clBufferAppendHeap(msg, fragNode->fragBuffer,
                fragNode->fragLength) != CL_OK) {
            rc = clBufferNBytesWrite(msg, fragNode->fragBuffer,
                    fragNode->fragLength);
            __iocFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
        } else {
            /*
             * Drop reference instead of recyling the stitched heap.
             */
            clHeapFree(fragNode->fragBuffer);
        }CL_ASSERT(rc == CL_OK);
        clLogTrace(
                "IOC",
                "REASSEMBLE",
                "Reassembled fragment offset [%d], len [%d]", fragNode->fragOffset, fragNode->fragLength);
        clRbTreeDelete(&node->reassembleTree, iter);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
#ifdef RELIABLE_IOC
    if(*(&node->isReliable)==CL_TRUE)
    {
        clOsalMutexLock(&iocAcklock);
        ClIocCommPortT *commPort = clIocGetPort(node->commPort);
        ClInt32T ack = node->ackSync;
        receiverAckSend(commPort, &node->srcAddress, node->messageId, ack);
        if (clTimerCheckAndDelete(&node->TimerACKHdl) == CL_OK)
        {
            clHeapFree(node->TimerACKHdl);
            node->TimerACKHdl = NULL;
        }
        destroyNodes(&node->receiverLossList);
        clOsalMutexUnlock(&iocAcklock);
    }


#endif
    /*
     * Atomically check and delete timer if not running.
     */
    if (clTimerCheckAndDelete(&node->timerKey->reassembleTimer) == CL_OK) {
        //clLogDebug("IOC", "Rel", "Removed reassemble Timer");
        clHeapFree(node->timerKey);
        node->timerKey = NULL;
    }

    clBufferLengthGet(msg, &len);
    if (!len) {
        if (!message) {
            clBufferDelete(&msg);
        }
    } else if (!sync) {
        ClBoolT relay = CL_FALSE;
        if (CL_IOC_ADDRESS_TYPE_GET(&fragHeader->header.dstAddress)
                == CL_IOC_PHYSICAL_ADDRESS_TYPE) {
            if (fragHeader->header.dstAddress.iocPhyAddress.nodeAddress
                    != gIocLocalBladeAddress &&
                    fragHeader->header.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS) {relay = CL_TRUE;
            if(fragHeader->header.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS
                    &&
                    !clTransportBridgeEnabled(gIocLocalBladeAddress))
            {
                relay = CL_FALSE;
            }
        }
    }
        if (relay) {
            ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY,
                    .timeout = 0 };
            ClIocPortT portId =
                    fragHeader->header.dstAddress.iocPhyAddress.portId;
            if (portId >= CL_IOC_XPORT_PORT) {
                portId = CL_IOC_CPM_PORT;
            }
            ClIocCommPortT *commPort = clIocGetPort(portId);
            if (commPort) {
                if (fragHeader->header.dstAddress.iocPhyAddress.nodeAddress
                        == CL_IOC_BROADCAST_ADDRESS) {
                    ClIocAddressT *bcastList = NULL;
                    ClUint32T numBcasts = 0;
                    /*
                     * Check if we have a proxy broadcast list 
                     */
                    if (clTransportBroadcastListGet(xportType,
                            &fragHeader->header.srcAddress.iocPhyAddress,
                            &numBcasts, &bcastList) == CL_OK) {
                        /*
                         * Broadcast proxy and continue with message processing
                         */
                        ClUint32T i;
                        for (i = 0; i < numBcasts; ++i) {
                            clLogDebug(
                                    "PROXY",
                                    "RELAY",
                                    "Broadcast reassembled message from node [%d], port [%d] "
                                    "to node [%d], port [%d]", fragHeader->header.srcAddress.iocPhyAddress.nodeAddress, fragHeader->header.srcAddress.iocPhyAddress.portId, bcastList[i].iocPhyAddress.nodeAddress, bcastList[i].iocPhyAddress.portId);
                            rc = clIocSendWithXportRelay(
                                    (ClIocCommPortHandleT) commPort, msg,
                                    fragHeader->header.protocolType,
                                    &fragHeader->header.srcAddress,
                                    &bcastList[i], &sendOption,
                                    (ClCharT*) xportType, CL_FALSE);
                            clBufferReadOffsetSet(msg, 0, CL_BUFFER_SEEK_SET);
                        }
                        if (bcastList)
                            clHeapFree(bcastList);
                    }
                    relay = CL_FALSE;
                    goto enqueue;
                } else {
                    clLogDebug(
                            "PROXY",
                            "RELAY",
                            "Forward reassembled message from [%d:%d] to node [%d:%d]", fragHeader->header.srcAddress.iocPhyAddress.nodeAddress, fragHeader->header.srcAddress.iocPhyAddress.portId, fragHeader->header.dstAddress.iocPhyAddress.nodeAddress, fragHeader->header.dstAddress.iocPhyAddress.portId);
                    rc = clIocSendWithRelay((ClIocCommPortHandleT) commPort,
                            msg, fragHeader->header.protocolType,
                            &fragHeader->header.srcAddress,
                            &fragHeader->header.dstAddress, &sendOption);
                }
            } else {
                clLogError(
                        "PROXY",
                        "RELAY",
                        "Unable to forward the message from [%d:%d] to node [%d:%d] "
                        "using src port [%d]", fragHeader->header.srcAddress.iocPhyAddress.nodeAddress, fragHeader->header.srcAddress.iocPhyAddress.portId, fragHeader->header.dstAddress.iocPhyAddress.nodeAddress, fragHeader->header.dstAddress.iocPhyAddress.portId, portId);
            }
        } else {
            ClIocRecvParamT recvParam = { 0 };
            enqueue: recvParam.length = len;
            recvParam.priority = fragHeader->header.priority;
            recvParam.protoType = fragHeader->header.protocolType;
            memcpy(&recvParam.srcAddr, &fragHeader->header.srcAddress,
                    sizeof(recvParam.srcAddr));
            rc = clEoEnqueueReassembleJob(msg, &recvParam);
        }

        /*Delete the msg if appropriate*/
        if (relay || (rc != CL_OK)) {
            if (!message) {
                clBufferDelete(&msg);
            }
        }
    } else {
        /* sync. reassemble success */
        retCode = CL_OK;
    }
    clHeapFree(node->timerKey);
    node->timerKey = NULL;
    clHeapFree(node);
    return retCode;
}

static ClRcT __iocFragmentCallback(ClPtrT job, ClBufferHandleT message,
        ClBoolT sync) {
    ClIocReassembleKeyT key = { 0 };
    ClIocReassembleNodeT *node = NULL;
    ClIocFragmentJobT *fragmentJob = job;
    ClIocFragmentNodeT *fragmentNode = NULL;
    ClUint8T flag = 0;
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_IOC_RC(IOC_MSG_QUEUED);

    flag = fragmentJob->fragHeader.header.flag;
    key.fragId = fragmentJob->fragHeader.msgId;
    key.destAddr.nodeAddress = gIocLocalBladeAddress;
    key.destAddr.portId = fragmentJob->portId;
    /*
     * Could be a relay packet.
     */
    if (CL_IOC_ADDRESS_TYPE_GET(&fragmentJob->fragHeader.header.dstAddress)
            == CL_IOC_PHYSICAL_ADDRESS_TYPE
            && fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress
                    != CL_IOC_RESERVED_ADDRESS
            && fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress
                    != CL_IOC_BROADCAST_ADDRESS) {
        key.destAddr.nodeAddress =
                fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress;
        key.destAddr.portId =
                fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.portId;
    }
    key.sendAddr = fragmentJob->fragHeader.header.srcAddress.iocPhyAddress;
    node = __iocReassembleNodeFind(&key, 0);
    if (!node) {
#ifdef RELIABLE_IOC
        if (fragmentJob->fragHeader.header.isReliable == CL_TRUE)
        {
            //clLogDebug("IOC", "Rel", "receive duplicate fragment he he");
            if(fragmentJob->fragHeader.msgId == lastMessageId && fragmentJob->fragHeader.header.srcAddress.iocPhyAddress.nodeAddress == lastMessageNode)
            {
                clLogDebug("IOC", "Rel", "receive duplicate fragment [%d] of last message received. Ignore ",fragmentJob->fragHeader.fragId);
                return CL_IOC_RC(CL_ERR_TRY_AGAIN);
            }
        }
#endif
        /*
         * create a new reassemble node.
         */
        ClUint32T hashKey = __iocReassembleHashKey(&key);
        ClIocReassembleTimerKeyT *timerKey = NULL;
        timerKey = clHeapCalloc(1, sizeof(*timerKey));
        CL_ASSERT(timerKey != NULL);
        node = clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        memcpy(&timerKey->key, &key, sizeof(timerKey->key)); /*safe w.r.t node deletes*/
        node->timerKey = timerKey;
        timerKey->timerId = ++iocReassembleCurrentTimerId;
        clRbTreeInit(&node->reassembleTree, __iocFragmentCmp);
        rc = clTimerCreate(userReassemblyTimerExpiry, CL_TIMER_ONE_SHOT,
                CL_TIMER_SEPARATE_CONTEXT, __iocReassembleTimer,
                (void *) timerKey, &timerKey->reassembleTimer);
        CL_ASSERT(rc == CL_OK);
        node->currentLength = 0;
        node->expectedLength = 0;
        node->receiverLossList=NULL;
        node->numFragments = 0;
        node->lastAckSync = 0;
        node->isReliable = fragmentJob->fragHeader.header.isReliable;
        node->ackSync = 0;
        node->messageId = fragmentJob->fragHeader.msgId;
        node->lossTotal = 0;
        node->commPort = fragmentJob->portId;
#ifdef RELIABLE_IOC
        if (fragmentJob->fragHeader.header.isReliable == CL_TRUE) {
            //clLogDebug("IOC", "Rel", "receiver reliable fragment");
            CL_ASSERT(rc == CL_OK);
            rc = clTimerCreate(ackTimerExpiry, CL_TIMER_REPETITIVE,
                    CL_TIMER_SEPARATE_CONTEXT, receiverACKTrigger,
                    (void *) timerKey, &node->TimerACKHdl);
            if(rc==CL_OK)
            {
                //clLogDebug("IOC", "Rel", "created ack timer successful");
            }
            node->srcAddress.iocPhyAddress.nodeAddress=fragmentJob->fragHeader.header.srcAddress.iocPhyAddress.nodeAddress;
            node->srcAddress.iocPhyAddress.portId=fragmentJob->fragHeader.header.srcAddress.iocPhyAddress.portId;
            //clLogDebug("IOC", "Rel", "start ack node");
            clTimerStart(node->TimerACKHdl);
        }
#endif
        hashAdd(iocReassembleHashTable, hashKey, &node->hash);
        clTimerStart(node->timerKey->reassembleTimer);
    }

#ifdef RELIABLE_IOC
    if (node->isReliable == CL_TRUE)
    {
        clLogDebug("IOC", "Rel", "receive fragment [%d] with acksync [%d] of message id [%d] last message id [%d] source node [%d] last message source node [%d]",fragmentJob->fragHeader.fragId,node->ackSync,node->messageId,lastMessageId,fragmentJob->fragHeader.header.srcAddress.iocPhyAddress.nodeAddress,lastMessageNode);
        ClInt32T loss = iocFragmentIdCmp(fragmentJob->fragHeader.fragId,(node->ackSync + 1));
        if (loss > 0)
        {
            ClUint32T begin=node->ackSync + 1;
            ClUint32T end=fragmentJob->fragHeader.fragId - 1;
            clLogDebug("FRAG", "RECV", "loss fragment detected with [%d] fragment from [%d] to [%d]",loss,begin,end);
            ClUint32T count;
            count=lossListInsertRange(&node->receiverLossList,begin,end);
            clLogDebug("FRAG", "RECV", "debug count [%d] loss total [%d] ",count,node->lossTotal);
            node->lossTotal += count;
            node->ackSync = fragmentJob->fragHeader.fragId;
            //send NAK to Sender
            ClIocAddressT srcAddress = {{0}};
            srcAddress.iocPhyAddress.nodeAddress = node->srcAddress.iocPhyAddress.nodeAddress;
            srcAddress.iocPhyAddress.portId = node->srcAddress.iocPhyAddress.portId;
            receiverNAKTriggerDirect(node->messageId,begin,end,&srcAddress,node->commPort);
        } else if (loss < 0)
        {
            clLogDebug("IOC", "Rel", "receive loss fragment [%d]. Remove fragment out of receiver loss list",fragmentJob->fragHeader.fragId);
            ClBoolT ret= lossListDelete(&node->receiverLossList, fragmentJob->fragHeader.fragId);
            if (ret == CL_TRUE)
            {
                (node->lossTotal)-=1;
            }
            else
            {
                clLogDebug("IOC", "Rel", "receive duplicate fragment [%d] . Ignore ",fragmentJob->fragHeader.fragId);
                return CL_IOC_RC(IOC_MSG_QUEUED);
            }
        }else
        {
            node->ackSync = fragmentJob->fragHeader.fragId;
        }
        if (node->lossTotal>0)
        {
            //clLogDebug("IOC", "Rel", "current loss total [%d]",node->lossTotal);
        }
    }
    else
    {
        //clLogDebug("IOC", "Rel", "Receiver fragment [%d]",fragmentJob->fragHeader.fragId);
    }
#endif

    fragmentNode = clHeapCalloc(1, sizeof(*fragmentNode));
    CL_ASSERT(fragmentNode != NULL);
    fragmentNode->fragOffset = fragmentJob->fragHeader.fragOffset;
    fragmentNode->fragLength = fragmentJob->fragHeader.fragLength;
    fragmentNode->fragBuffer = fragmentJob->buffer;
    fragmentNode->fragId = fragmentJob->fragHeader.fragId;
    node->currentLength += fragmentNode->fragLength;
    ++node->numFragments;
    clRbTreeInsert(&node->reassembleTree, &fragmentNode->tree);

    if (flag == IOC_LAST_FRAG)
    {
        if (fragmentNode->fragOffset + fragmentNode->fragLength
                == node->currentLength) {
            //trigger to send last ack
            if (node->isReliable == CL_TRUE)
            {
                receiverLastACKTrigger(node);
                clTimerStop(node->TimerACKHdl);
            }
            retCode = __iocReassembleDispatch(
                    fragmentJob->xportType[0] ? fragmentJob->xportType : NULL,
                    node, &fragmentJob->fragHeader, message, sync);
        } else {
            /*
             * we update the expected length.
             */
            clLogTrace(
                    "FRAG",
                    "RECV",
                    "Out of order last fragment received for offset [%d], len [%d] bytes, "
                    "current length [%d] bytes", fragmentNode->fragOffset, fragmentNode->fragLength, node->currentLength);
            node->expectedLength = fragmentNode->fragOffset
                    + fragmentNode->fragLength;
        }
    } else if (node->currentLength == node->expectedLength) {
        if (node->isReliable == CL_TRUE)
        {
            receiverLastACKTrigger(node);
            clTimerStop(node->TimerACKHdl);
        }
        retCode = __iocReassembleDispatch(
                fragmentJob->xportType[0] ? fragmentJob->xportType : NULL, node,
                &fragmentJob->fragHeader, message, sync);
    } else {
        /*
         * Now increase the timer based on the number of fragments being received or the node length.
         * Since the sender could be doing flow control, we have an adaptive reassembly timer.
         */
        if (!(node->numFragments & CL_IOC_REASSEMBLY_FRAGMENTS_MASK)) {
            clLogDebug(
                    "FRAG",
                    "RECV",
                    "Updating the reassembly timer to refire after [%d] secs for node length [%d], "
                    "num fragments [%d]", userReassemblyTimerExpiry.tsSec, node->currentLength, node->numFragments);
            clTimerUpdate(node->timerKey->reassembleTimer,
                    userReassemblyTimerExpiry);
        }
    }
    clHeapFree(job);
    return retCode;
}

static ClRcT iocFragmentCallback(ClPtrT job) {
    ClRcT rc = CL_OK;
    clOsalMutexLock(&iocReassembleLock);
    rc = __iocFragmentCallback(job, 0, CL_FALSE);
    clOsalMutexUnlock(&iocReassembleLock);
    return rc;
}

ClRcT __iocUserFragmentReceive(const ClCharT *xportType, ClUint8T *pBuffer,
        ClIocFragHeaderT *userHdr, ClIocPortT portId, ClUint32T length,
        ClBufferHandleT message, ClBoolT sync) {
    ClIocFragmentJobT *job = clHeapCalloc(1, sizeof(*job));
    ClUint8T *buffer = NULL;
    ClRcT rc = CL_OK;
    CL_ASSERT(job != NULL);
    CL_ASSERT(length == userHdr->fragLength);
    buffer = __iocFragmentPoolGet(pBuffer, length);
    memcpy(buffer, pBuffer, length);
    job->buffer = buffer;
    memcpy(&job->fragHeader, userHdr, sizeof(job->fragHeader));
    job->portId = portId;
    job->length = length;
    job->xportType[0] = 0;
    if (xportType) {
        strncat(job->xportType, xportType, sizeof(job->xportType) - 1);
    }
    if (!sync) {
        rc = clJobQueuePush(&iocFragmentJobQueue, iocFragmentCallback,
                (ClPtrT) job);
        if (rc != CL_OK)
            return rc;
        rc = CL_IOC_RC(IOC_MSG_QUEUED);
    } else {
        rc = __iocFragmentCallback((ClPtrT) job, message, CL_TRUE);
    }
    return rc;
}

ClRcT clIocTotalNeighborEntryGet(ClUint32T *pNumberOfEntries) {
    ClUint32T i = 0;
    ClUint8T status = 0;

    NULL_CHECK(pNumberOfEntries);

    for (i = 0; i < CL_IOC_MAX_NODES; i++) {
        clIocRemoteNodeStatusGet(i, &status);
        if (status == CL_IOC_NODE_UP) {
            ++(*pNumberOfEntries);
        }
    }
    return CL_OK;
}

ClRcT clIocNeighborListGet(ClUint32T *pNumberOfEntries,
        ClIocNodeAddressT *pAddrList) {
    ClUint32T i = 0;
    ClUint8T status = 0;
    ClUint32T numEntries = 0;

    NULL_CHECK(pNumberOfEntries);
    NULL_CHECK(pAddrList);

    if (!*pNumberOfEntries)
        return CL_OK;

    for (i = 0; i < CL_IOC_MAX_NODES; i++) {
        clIocRemoteNodeStatusGet(i, &status);
        if (status == CL_IOC_NODE_UP) {
            pAddrList[numEntries++] = i;
            if (numEntries == *pNumberOfEntries)
                break;
        }
    }
    if (numEntries != *pNumberOfEntries)
        *pNumberOfEntries = numEntries;
    return CL_OK;
}

static ClRcT clIocReplicastGet(ClIocPortT portId, ClIocAddressT **pAddressList,
        ClUint32T *pNumEntries) {
    ClUint32T numEntries = 0;
    ClIocAddressT *addressList = NULL;
    ClUint32T i;
    ClUint8T status = 0;
    if (!pAddressList || !pNumEntries)
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    addressList = clHeapCalloc(CL_IOC_MAX_NODES, sizeof(*addressList));
    CL_ASSERT(addressList != NULL);
    for (i = 1; i < CL_IOC_MAX_NODES; ++i) {
        clIocRemoteNodeStatusGet(i, &status);
        if (status == CL_IOC_NODE_UP) {
            addressList[numEntries].iocPhyAddress.nodeAddress = i;
            addressList[numEntries].iocPhyAddress.portId = portId;
            ++numEntries;
        }
    }
    if (!numEntries) {
        clHeapFree(addressList);
        addressList = NULL;
    }
    *pAddressList = addressList;
    *pNumEntries = numEntries;
    return CL_OK;
}

static ClRcT clIocTransparencyBind(ClIocCommPortT *pIocCommPort,
        ClIocLogicalAddressCtrlT *pLogicalAddress) {
    return clTransportTransparencyRegister(NULL, pIocCommPort->portId,
            pLogicalAddress->logicalAddress, pLogicalAddress->haState);

}

ClRcT clIocTransparencyRegister(ClIocTLInfoT *pTLInfo) {
    ClIocCommPortT *pIocCommPort = NULL;
    ClIocPortT portId;
    ClIocCompT *pComp = NULL;
    ClIocLogicalAddressCtrlT *pIocLogicalAddress = NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    ClBoolT found = CL_FALSE;
    ClBoolT portFound = CL_FALSE;

    NULL_CHECK(pTLInfo);

    if (CL_IOC_ADDRESS_TYPE_GET(&pTLInfo->logicalAddr)
            != CL_IOC_LOGICAL_ADDRESS_TYPE) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("\n TL: Invalid logical address:0x%llx \n", pTLInfo->logicalAddr));
        goto out;
    }
    portId = pTLInfo->physicalAddr.portId;

    /*Now lookup the physical address for the ioc comm port*/
    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(portId);
    if (pIocCommPort == NULL) {
        clOsalMutexUnlock(&gClIocPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid physical address portid:0x%x.\n",portId));
        goto out;
    }

    /*Check for an already existing compid mapping the port */
    if (pIocCommPort->pComp && pIocCommPort->pComp->compId != pTLInfo->compId) {
        clOsalMutexUnlock(&gClIocPortMutex);
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("More than 2 components cannot bind to the same port.Comp id 0x%x already bound to the port 0x%x\n",pIocCommPort->pComp->compId,pIocCommPort->portId));
        goto out;
    }

    /*Try getting the compId*/
    clOsalMutexLock(&gClIocCompMutex);
    pComp = clIocGetComp(pTLInfo->compId);
    if (pComp == NULL) {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        pComp = clHeapCalloc(1, sizeof(*pComp));
        if (pComp == NULL) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error allocating memory\n"));
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            goto out;
        }
        pComp->compId = pTLInfo->compId;
        CL_LIST_HEAD_INIT(&pComp->portList);
        clOsalMutexLock(&gClIocPortMutex);
        clOsalMutexLock(&gClIocCompMutex);
    } else {
        register ClListHeadT *pTemp;
        found = CL_TRUE;
        /*Check for same port and LA combination*/
        CL_LIST_FOR_EACH(pTemp,&pComp->portList) {
            ClIocCommPortT *pCommPort =
                    CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
            /*Found the port, check for the logical address*/
            if (pCommPort->portId == pIocCommPort->portId) {
                register ClListHeadT *pAddressList;
                portFound = CL_TRUE;
                CL_LIST_FOR_EACH(pAddressList,&pCommPort->logicalAddressList) {
                    pIocLogicalAddress =
                            CL_LIST_ENTRY(pAddressList,ClIocLogicalAddressCtrlT,list);
                    if (pIocLogicalAddress->logicalAddress
                            == pTLInfo->logicalAddr) {
                        if (pIocLogicalAddress->haState == pTLInfo->haState) {
                            clOsalMutexUnlock(&gClIocCompMutex);
                            clOsalMutexUnlock(&gClIocPortMutex);
                            CL_DEBUG_PRINT(
                                    CL_DEBUG_ERROR,
                                    ("CompId:0x%x has already registered with LA:0x%llx,portId:0x%x\n",pTLInfo->compId,pTLInfo->logicalAddr,pTLInfo->physicalAddr.portId));
                            rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                            goto out;
                        }
                        pIocLogicalAddress->haState = pTLInfo->haState;
                        goto out_bind;
                    }
                }
                break;
            }
        }
    }
    pIocLogicalAddress = clHeapCalloc(1, sizeof(*pIocLogicalAddress));
    if (pIocLogicalAddress == NULL) {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error allocating memory\n"));
        goto out_free;
    }
    /*
     * Pingable for service availability with node address info.
     */
    pIocLogicalAddress->logicalAddress = pTLInfo->logicalAddr;
    pIocLogicalAddress->haState = pTLInfo->haState;
    pIocLogicalAddress->pIocCommPort = pIocCommPort;
    if (portFound == CL_FALSE) {
        pIocCommPort->pComp = pComp;
        /*Add the port to the component list*/
        clListAddTail(&pIocCommPort->listComp, &pComp->portList);
    }

    if (found == CL_FALSE) {
        /*Add the comp to the hash list*/
        clIocCompHashAdd(pComp);
    }
    /*Add to the port list*/
    clListAddTail(&pIocLogicalAddress->list, &pIocCommPort->logicalAddressList);
    pIocCommPort->activeBind = CL_FALSE;
    if (pTLInfo->haState == CL_IOC_TL_STDBY) {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_OK;
        goto out;
    }

    out_bind: rc = clIocTransparencyBind(pIocCommPort, pIocLogicalAddress);
    if (rc != CL_OK) {
        goto out_del;
    }
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    goto out;

    out_del: clListDel(&pIocLogicalAddress->list);
    if (portFound == CL_FALSE) {
        clListDel(&pIocCommPort->listComp);
        pIocCommPort->pComp = NULL;
    }
    if (found == CL_FALSE) {
        clIocCompHashDel(pComp);
    }
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out_free: if (pIocLogicalAddress) {
        clHeapFree(pIocLogicalAddress);
    }
    if (found == CL_FALSE) {
        clHeapFree(pComp);
    }
    out: return rc;
}

ClRcT clIocTransparencyDeregister(ClUint32T compId) {
    ClIocCompT *pComp = NULL;
    ClIocLogicalAddressCtrlT *pIocLogicalAddress = NULL;
    ClIocCommPortT *pCommPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pNext = NULL;
    ClListHeadT *pHead = NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocCompMutex);
    pComp = clIocGetComp(compId);
    if (pComp == NULL) {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        goto out;
    }
    pHead = &pComp->portList;
    for (pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext) {
        register ClListHeadT *pTempLogicalList;
        ClListHeadT *pTempLogicalNext;
        pNext = pTemp->pNext;
        pCommPort = CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
        CL_ASSERT(pCommPort->pComp == pComp);
        pCommPort->pComp = NULL;
        for (pTempLogicalList = pCommPort->logicalAddressList.pNext;
                pTempLogicalList != &pCommPort->logicalAddressList;
                pTempLogicalList = pTempLogicalNext) {
            pTempLogicalNext = pTempLogicalList->pNext;
            pIocLogicalAddress =
                    CL_LIST_ENTRY(pTempLogicalList,ClIocLogicalAddressCtrlT,list);
            clListDel(&pIocLogicalAddress->list);
            CL_ASSERT(
                    pIocLogicalAddress->pIocCommPort->portId == pCommPort->portId);
            rc = clTransportTransparencyDeregister(NULL, pCommPort->portId,
                    pIocLogicalAddress->logicalAddress);
            if (rc != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Logical address deregister for [0x%llx] returned with [%#x]\n", pIocLogicalAddress->logicalAddress, rc));

            }
            clHeapFree(pIocLogicalAddress);
        }
    }
    /*Now unhash*/
    clIocCompHashDel(pComp);
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    clHeapFree(pComp);
    rc = CL_OK;
    out: return rc;
}

ClRcT clIocTransparencyLogicalToPhysicalAddrGet(
        ClIocLogicalAddressT logicalAddr, ClIocTLMappingT **pPhysicalAddr,
        ClUint32T *pNoEntries) {
    NULL_CHECK(pNoEntries);
    NULL_CHECK(pPhysicalAddr);
    *pNoEntries = 0;
    return CL_OK;
}

ClRcT clIocMcastIsRegistered(ClIocMcastUserInfoT *pMcastInfo) {
    ClRcT rc;
    ClIocCommPortT *pIocCommPort = NULL;
    ClIocMcastT *pMcast = NULL;
    ClIocMcastPortT *pMcastPort = NULL;

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if (!CL_IOC_MCAST_VALID(&pMcastInfo->mcastAddr)) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("\nERROR : Invalid multicast address:0x%llx\n",pMcastInfo->mcastAddr));
        goto out;
    }

    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(pMcastInfo->physicalAddr.portId);
    if (pIocCommPort == NULL) {
        clOsalMutexUnlock(&gClIocPortMutex);
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Error : Invalid portid: 0x%x\n",pMcastInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastInfo->mcastAddr);
    if (pMcast == NULL) {
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        goto out_unlock;
    } else {
        register ClListHeadT *pTemp = NULL;
        CL_LIST_FOR_EACH(pTemp,&pMcast->portList) {
            pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
            if (pMcastPort->pIocCommPort->portId
                    == pMcastInfo->physicalAddr.portId) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Error : Port 0x%x is already registered for the multicast address 0x%llx\n", pMcastInfo->physicalAddr.portId, pMcastInfo->mcastAddr));
                rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                goto out_unlock;
            }
        }
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    out_unlock: clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out: return rc;
}

ClRcT clIocMulticastRegister(ClIocMcastUserInfoT *pMcastUserInfo) {
    ClRcT rc = CL_OK;
    ClIocMcastT *pMcast = NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    ClIocCommPortT *pIocCommPort = NULL;
    ClBoolT found = CL_FALSE;

    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if (!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr)) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(pMcastUserInfo->physicalAddr.portId);
    if (pIocCommPort == NULL) {
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Invalid portid: 0x%x\n",pMcastUserInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastUserInfo->mcastAddr);
    if (pMcast == NULL) {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        pMcast = clHeapCalloc(1, sizeof(*pMcast));
        if (pMcast == NULL) {
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error allocating memory\n"));
            goto out;
        }
        pMcast->mcastAddress = pMcastUserInfo->mcastAddr;
        CL_LIST_HEAD_INIT(&pMcast->portList);
        clOsalMutexLock(&gClIocPortMutex);
        clOsalMutexLock(&gClIocMcastMutex);
    } else {
        register ClListHeadT *pTemp = NULL;
        found = CL_TRUE;
        CL_LIST_FOR_EACH(pTemp,&pMcast->portList) {
            pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
            if (pMcastPort->pIocCommPort->portId
                    == pMcastUserInfo->physicalAddr.portId) {
                clOsalMutexUnlock(&gClIocMcastMutex);
                clOsalMutexUnlock(&gClIocPortMutex);
                rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Port 0x%x already exist\n",pMcastUserInfo->physicalAddr.portId));
                goto out;
            }
        }
    }
    pMcastPort = clHeapCalloc(1, sizeof(*pMcastPort));
    if (pMcastPort == NULL) {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error allocating memory\n"));
        goto out_free;
    }
    pMcastPort->pMcast = pMcast;
    pMcastPort->pIocCommPort = pIocCommPort;
    /*Add to the mcasts port list*/
    clListAddTail(&pMcastPort->listMcast, &pMcast->portList);
    /*Add this to the ports mcast list*/
    clListAddTail(&pMcastPort->listPort, &pIocCommPort->multicastAddressList);
    /*Add to the hash linkage*/
    if (found == CL_FALSE) {
        clIocMcastHashAdd(pMcast);
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    /*Fire the bind*/
    rc = clTransportMulticastRegister(NULL, pIocCommPort->portId,
            pMcast->mcastAddress);
    if (rc != CL_OK) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                (" Multicast register for port [%#x] returned [%#x]\n", pIocCommPort->portId, rc));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        goto out_del;
    }
    rc = CL_OK;
    goto out;

    out_del: clListDel(&pMcastPort->listPort);
    clListDel(&pMcastPort->listMcast);
    if (found == CL_FALSE) {
        clIocMcastHashDel(pMcast);
    }
    out_free: if (found == CL_FALSE) {
        clHeapFree(pMcast);
    }
    if (pMcastPort) {
        clHeapFree(pMcastPort);
    }
    out: return rc;
}

ClRcT clIocMulticastDeregister(ClIocMcastUserInfoT *pMcastUserInfo) {
    ClRcT rc = CL_OK;
    ClIocMcastT *pMcast = NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    ClIocCommPortT *pIocCommPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pNext = NULL;
    ClListHeadT *pHead = NULL;
    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if (!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr)) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    clOsalMutexLock(&gClIocPortMutex);

    pIocCommPort = clIocGetPort(pMcastUserInfo->physicalAddr.portId);

    if (pIocCommPort == NULL) {
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Error in getting port: 0x%x\n",pMcastUserInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastUserInfo->mcastAddr);
    if (pMcast == NULL) {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Error in getting mcast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    pHead = &pMcast->portList;
    for (pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext) {
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
        CL_ASSERT(pMcastPort->pMcast == pMcast);
        if (pMcastPort->pIocCommPort == pIocCommPort) {
            rc = clTransportMulticastDeregister(NULL, pIocCommPort->portId,
                    pMcast->mcastAddress);
            if (rc != CL_OK) {
                CL_DEBUG_PRINT(
                        CL_DEBUG_ERROR,
                        ("Multicast deregister for port [%#x] returned with [%#x]\n", pIocCommPort->portId, rc));
                clOsalMutexUnlock(&gClIocMcastMutex);
                clOsalMutexUnlock(&gClIocPortMutex);
                rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
            clListDel(&pMcastPort->listMcast);
            clListDel(&pMcastPort->listPort);
            clHeapFree(pMcastPort);
            goto found;
        }
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Unable to find port:0x%x in mcast list\n",pIocCommPort->portId));
    goto out;

    found: rc = CL_OK;
    if (CL_LIST_HEAD_EMPTY(pHead)) {
        /*Knock off the mcast itself*/
        clIocMcastHashDel(pMcast);
        clHeapFree(pMcast);
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out: return rc;
}

ClRcT clIocMulticastDeregisterAll(ClIocMulticastAddressT *pMcastAddress) {
    ClRcT rc = CL_OK;
    ClIocCommPortT *pIocCommPort = NULL;
    ClIocMcastT *pMcast = NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead = NULL;
    ClListHeadT *pNext = NULL;
    ClIocMulticastAddressT mcastAddress;

    NULL_CHECK(pMcastAddress);
    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if (!CL_IOC_MCAST_VALID(pMcastAddress)) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nERROR: Invalid multicast address:0x%llx\n",*pMcastAddress));
        goto out;
    }
    mcastAddress = *pMcastAddress;
    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(mcastAddress);
    if (pMcast == NULL) {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_OK;
        /*could have been ripped off from a multicast deregister*/
        CL_DEBUG_PRINT(CL_DEBUG_WARN,
                ("Error in mcast get for address:0x%llx\n",mcastAddress));
        goto out;
    }
    pHead = &pMcast->portList;
    for (pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext) {
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
        pIocCommPort = pMcastPort->pIocCommPort;
        if ((rc = clTransportMulticastDeregister(NULL, pIocCommPort->portId,
                pMcast->mcastAddress)) != CL_OK) {
            CL_DEBUG_PRINT(
                    CL_DEBUG_ERROR,
                    ("Multicast deregister for port [%#x] returned [%#x]\n", pIocCommPort->portId, rc));
        }
        clListDel(&pMcastPort->listMcast);
        clListDel(&pMcastPort->listPort);
        clHeapFree(pMcastPort);
    }
    clIocMcastHashDel(pMcast);
    clHeapFree(pMcast);
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    rc = CL_OK;

    out: return rc;
}

static ClVersionT versionsSupported[] = { { 'B', 0x01, 0x01 } };

static ClVersionDatabaseT versionDatabase = { sizeof(versionsSupported)
        / sizeof(ClVersionT), versionsSupported };

ClRcT clIocVersionCheck(ClVersionT *pVersion) {
    ClRcT rc;

    if (pVersion == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    CL_DEBUG_PRINT(
            CL_DEBUG_TRACE,
            ("Passed Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n", pVersion->releaseCode, pVersion->majorVersion, pVersion->minorVersion));
    rc = clVersionVerify(&versionDatabase, pVersion);
    if (rc != CL_OK) {
        CL_DEBUG_PRINT(
                CL_DEBUG_ERROR,
                ("Supported Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n", pVersion->releaseCode, pVersion->majorVersion, pVersion->minorVersion));
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error : Invalid version of application. rc 0x%x\n", rc));
        return rc;
    }
    return rc;
}

ClRcT clIocServerReady(ClIocAddressT *pAddress) {
    return clTransportServerReady(NULL, pAddress);
}

static ClIocNeighborT *clIocNeighborFind(ClIocNodeAddressT address) {
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead = &gClIocNeighborList.neighborList;
    ClIocNeighborT *pNeigh = NULL;
    CL_LIST_FOR_EACH(pTemp,pHead) {
        pNeigh = CL_LIST_ENTRY(pTemp,ClIocNeighborT,list);
        if (pNeigh->address == address) {
            return pNeigh;
        }
    }
    return NULL;
}

ClRcT clIocNeighborAdd(ClIocNodeAddressT address, ClUint32T status) {
    ClRcT rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
    ClIocNeighborT *pNeigh = NULL;

    clOsalMutexLock(&gClIocNeighborList.neighborMutex);
    pNeigh = clIocNeighborFind(address);
    clOsalMutexUnlock(&gClIocNeighborList.neighborMutex);

    if (pNeigh) {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,
                ("Neighbor node:0x%x already exist\n",address));
        rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
        goto out;
    }
    pNeigh = clHeapCalloc(1, sizeof(*pNeigh));
    if (pNeigh == NULL) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error in neigh add\n"));
        goto out;
    }
    pNeigh->address = address;
    pNeigh->status = status;
    clOsalMutexLock(&gClIocNeighborList.neighborMutex);
    clListAddTail(&pNeigh->list, &gClIocNeighborList.neighborList);
    ++gClIocNeighborList.numEntries;
    clOsalMutexUnlock(&gClIocNeighborList.neighborMutex);
    rc = CL_OK;
    out: return rc;
}

void clIocQueueNotificationUnpack(ClIocQueueNotificationT *pSrcQueue,
        ClIocQueueNotificationT *pDestQueue) {
    return;
}

ClRcT clIocCompStatusEnable(ClIocPhysicalAddressT addr) {
    return clIocCompStatusSet(addr, CL_IOC_NODE_UP);
}

void clIocMasterCacheReset(void) {
    ClIocPhysicalAddressT compAddr = { 0 };
    compAddr.portId = CL_IOC_XPORT_PORT;
    clIocMasterSegmentUpdate(compAddr);
}

void clIocMasterCacheSet(ClIocNodeAddressT master) {
    ClIocPhysicalAddressT compAddr = { 0 };
    compAddr.portId = CL_IOC_XPORT_PORT;
    clIocMasterSegmentSet(compAddr, master);
}

static __inline__ ClRcT clIocRangeNodeAddressGet(
        ClIocNodeAddressT *pNodeAddress, ClInt32T start, ClInt32T end) {
    ClInt32T step = start < end ? 1 : -1;

    if (start == end)
        end += step;
    do {
        ClUint8T status = CL_IOC_NODE_DOWN;
        CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Node address: %u \n",start));
        ClRcT rc = clIocRemoteNodeStatusGet((ClIocNodeAddressT) start, &status);
        if (rc == CL_OK && status == CL_IOC_NODE_UP) {
            *pNodeAddress = (ClIocNodeAddressT) start;
            return CL_OK;
        }
        start += step;
    } while (start != end);

    return CL_IOC_RC(CL_ERR_NOT_EXIST);
}

ClRcT clIocHighestNodeAddressGet(ClIocNodeAddressT *pNodeAddress) {
    NULL_CHECK(pNodeAddress);
    return clIocRangeNodeAddressGet(pNodeAddress, CL_IOC_MAX_NODE_ADDRESS - 1,
            -1);
}

ClRcT clIocLowestNodeAddressGet(ClIocNodeAddressT *pNodeAddress) {
    NULL_CHECK(pNodeAddress);
    return clIocRangeNodeAddressGet(pNodeAddress, 0, CL_IOC_MAX_NODE_ADDRESS);
}

#ifdef RELIABLE_IOC
/****************************
 * RELIBALE IOC SENDER
 ****************************/

static ClInt32T __iocSenderFragmentCmp(ClRbTreeT *node1, ClRbTreeT *node2) {
    ClIocReliableFragmentSenderNodeT *frag1 =
            CL_RBTREE_ENTRY(node1, ClIocReliableFragmentSenderNodeT, tree);
    ClIocReliableFragmentSenderNodeT *frag2 =
            CL_RBTREE_ENTRY(node2, ClIocReliableFragmentSenderNodeT, tree);
    return frag1->fragmentId - frag2->fragmentId;
}

static void __iocSenderFragmentPoolPut(ClUint8T *pBuffer, ClUint32T len) {
    if (len != iocSenderFragmentPoolLen) {
        clHeapFree(pBuffer);
        return;
    }
    if (!iocSenderFragmentPoolLimit) {
        iocSenderFragmentPoolLen = gClMaxPayloadSize;
        CL_ASSERT(iocFragmentPoolLen != 0);
        iocSenderFragmentPoolLimit = iocSenderFragmentPoolSize
                / iocSenderFragmentPoolLen;
    }
    if (iocSenderFragmentPoolEntries >= iocSenderFragmentPoolLimit) {
        clHeapFree(pBuffer);
    } else {
        ClIocFragmentPoolT *pool = (ClIocFragmentPoolT*) clHeapCalloc(1,
                sizeof(*pool));
        CL_ASSERT(pool != NULL);
        pool->buffer = pBuffer;
        clOsalMutexLock(&iocSenderFragmentPoolLock);
        clListAddTail(&pool->list, &iocSenderFragmentPool);
        ++iocSenderFragmentPoolEntries;
        clOsalMutexUnlock(&iocSenderFragmentPoolLock);
    }
}

static ClUint8T *__iocSenderFragmentPoolGet(ClUint8T *pBuffer, ClUint32T len) {
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *head = NULL;
    ClUint8T *buffer = NULL;
    clOsalMutexLock(&iocFragmentPoolLock);
    if (len != iocSenderFragmentPoolLen ||
    CL_LIST_HEAD_EMPTY(&iocSenderFragmentPool)) {
        clOsalMutexUnlock(&iocSenderFragmentPoolLock);
        goto alloc;
    }
    head = iocSenderFragmentPool.pNext;
    pool = CL_LIST_ENTRY(head, ClIocFragmentPoolT, list);
    clListDel(head);
    --iocSenderFragmentPoolEntries;
    clLogTrace("IOC", "SENDER-FRAG-POOL",
            "Got fragment of len [%d] from pool", len);
    clOsalMutexUnlock(&iocSenderFragmentPoolLock);
    buffer = pool->buffer;
    clHeapFree(pool);
    return buffer;

    alloc: return (ClUint8T*) clHeapAllocate(len);
}

static ClRcT __iocSenderFragmentPoolInitialize(void) {
    ClUint32T currentSize = 0;
    iocSenderFragmentPoolLen = gClMaxPayloadSize;
    CL_ASSERT(iocSenderFragmentPoolLen != 0);
    clOsalMutexInit(&iocSenderFragmentPoolLock);
    while (currentSize + iocSenderFragmentPoolLen < iocSenderFragmentPoolSize) {
        ClIocFragmentPoolT *pool = (ClIocFragmentPoolT*) clHeapCalloc(1,
                sizeof(*pool));
        ClUint8T *buffer = (ClUint8T*) clHeapAllocate(iocFragmentPoolLen);
        CL_ASSERT(pool != NULL);
        CL_ASSERT(buffer != NULL);
        currentSize += iocSenderFragmentPoolLen;
        pool->buffer = buffer;
        clListAddTail(&pool->list, &iocSenderFragmentPool);
        ++iocSenderFragmentPoolEntries;
        ++iocSenderFragmentPoolLimit;
    }
    return CL_OK;
}

static void __iocSenderFragmentPoolFinalize(void) {
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *iter = NULL;
    while (!CL_LIST_HEAD_EMPTY(&iocSenderFragmentPool)) {
        iter = iocSenderFragmentPool.pNext;
        pool = CL_LIST_ENTRY(iter, ClIocFragmentPoolT, list);
        clListDel(iter);
        if (pool->buffer)
            clHeapFree(pool->buffer);
        clHeapFree(pool);
    }
    iocSenderFragmentPoolEntries = 0;
    iocSenderFragmentPoolLimit = 0;
    clOsalMutexDestroy(&iocSenderFragmentPoolLock);
}

static __inline__ ClUint32T __iocSenderBufferHashKey(
        ClIocReliableBufferKeyT *key) {
    ClUint32T cksum = 0;
    clCksm32bitCompute((ClUint8T*) key, sizeof(*key), &cksum);
    return cksum & IOC_REASSEMBLE_HASH_MASK;
}

static __inline__ ClUint32T __iocReceiverBufferHashKey(
        ClIocReliableBufferKeyT *key) {
    ClUint32T cksum = 0;
    clCksm32bitCompute((ClUint8T*) key, sizeof(*key), &cksum);
    return cksum & IOC_REASSEMBLE_HASH_MASK;
}

ClIocReliableSenderNodeT *__iocSenderBufferNodeFind(
        ClIocReliableBufferKeyT *key, ClUint64T timerId)
{
    ClUint32T hash = __iocSenderBufferHashKey(key);
    struct hashStruct *iter = NULL;
    for (iter = iocSenderBufferHashTable[hash]; iter; iter = iter->pNext) {
        ClIocReliableSenderNodeT *node =
                hashEntry(iter, ClIocReliableSenderNodeT, hash);
        if (timerId && node->ttlTimerKey->timerId != timerId)
            continue;
        if (!memcmp(&node->ttlTimerKey->key, key,
                sizeof(node->ttlTimerKey->key)))
        {
            return node;
        }
    }
    return NULL;
}

static ClRcT senderTTLCallback(void* key) {
    ClRbTreeT *iter = NULL;
    ClIocReliableSenderNodeT *node = NULL;
    ClIocTTLTimerKeyT *timerKey = (ClIocTTLTimerKeyT*) key;
    clLogDebug("IOC","Rel","sender Time to live time out. Send drop drequest to receiver and remove message node entry.");
    clOsalMutexLock(&iocSenderLock);
    node = __iocSenderBufferNodeFind(&timerKey->key, timerKey->timerId);
    if (!node) {
        clLogDebug("IOC", "Rel", "Time to live callback : no node found");
        clOsalMutexUnlock(&iocSenderLock);
        goto error;
    } else
    {
         ClRbTreeT *iter, *next = NULL;
         for (iter = clRbTreeMin(&node->senderBufferTree); iter; iter = next)
         {
             ClIocReliableFragmentSenderNodeT *fragNode =
                     CL_RBTREE_ENTRY(iter, ClIocReliableFragmentSenderNodeT, tree);
             next = clRbTreeNext(&node->senderBufferTree, iter);
             if (fragNode->fragmentId <= node->numFragments)
             {
                 if (&fragNode->buffer)
                 {
                     //clLogDebug("IOC", "Rel","remove fragment [%d]",fragNode->fragmentId);
                     clBufferDelete(&fragNode->buffer);
                 }
                 if(node->sendLossList)
                 {
                     lossListDelete(&node->sendLossList, fragNode->fragmentId);
                 }
                 clRbTreeDelete(&node->senderBufferTree, iter);
                 clHeapFree(fragNode);
             }
         }
        hashDel(&node->hash);
        if (clTimerCheckAndDelete(&node->ttlTimerKey->ttlTimer) == CL_OK)
        {
            clHeapFree(node->ttlTimerKey);
            //clLogDebug("IOC", "Rel", "remove timeToLive Timer");
            node->ttlTimerKey = NULL;
        }
        destroyNodes(&node->sendLossList);
        clHeapFree(node);
        node=NULL;
        clOsalMutexUnlock(&iocSenderLock);
        //clLogDebug("IOC", "Rel","removed message in sender buffer");
    }
    //*************Send Drop message to receiver*************
    error: return 0;
}

static ClRcT senderResendCallback(void* key) {
    ClRbTreeT *iter = NULL;
    ClIocReliableSenderNodeT *node = NULL;
    ClRcT retCode;
    ClIocTTLTimerKeyT *timerKey = (ClIocTTLTimerKeyT*) key;
    //clLogDebug("IOC", "Rel", "sender resend time out. Resend message.");
    node = __iocSenderBufferNodeFind(&timerKey->key, timerKey->timerId);
    if (!node) {
        clLogDebug("IOC", "Rel", "Time to live callback : no node found");
        goto error;
    } else {
        //resend message
        ClIocAddressT interimDestAddress = { { 0 } };
        interimDestAddress = node->destAddress;
        ClIocNodeAddressT destNode = 0;
        destNode = interimDestAddress.iocPhyAddress.nodeAddress;
        ClCharT *xportType = NULL;
        retCode = clFindTransport(destNode, &interimDestAddress, &xportType);
        ClUint32T timeout = CL_RETRANMISSION_TIMEOUT;
        ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) (node->commPortHandle);
        clLogDebug("IOC","Rel","sending loss message with messageId [%d] and interimDestAddress [%d]-[%d]- %s - %d ", timerKey->key.fragId, interimDestAddress.iocPhyAddress.nodeAddress, interimDestAddress.iocPhyAddress.portId, xportType, (int)node->commPortHandle);
        retCode = internalSendSlow(pIocCommPort, node->buffer,
                CL_IOC_DEFAULT_PRIORITY, &interimDestAddress, &timeout,
                xportType, CL_FALSE);
    }
    return retCode;
    error: return 0;
}

ClIocReliableSenderNodeT * getSenderBufferNode(ClUint32T messageId,
        ClIocAddressT *destAddress, ClIocPortT portId) {

    ClIocReliableBufferKeyT key = { 0 };
    ClIocReliableSenderNodeT *node = NULL;
    key.fragId = messageId;
    key.destAddr.nodeAddress =
            ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
    key.destAddr.portId = ((ClIocPhysicalAddressT *) destAddress)->portId;
    key.sendAddr.nodeAddress = gIocLocalBladeAddress;
    key.sendAddr.portId = portId;
    node = __iocSenderBufferNodeFind(&key, 0);
    if (!node) {
        return NULL;
    } else {
        return node;
    }
}


static void senderBufferAddFragment(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, struct iovec *target, ClUint32T targetVectors,
        ClIocAddressT *destAddress, ClUint32T fragmentSize,
        ClUint32T fragmentId, ClUint32T totalFragment) {
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) commPortHandle;
    ClIocReliableBufferKeyT key = { 0 };
    ClIocReliableSenderNodeT *node = NULL;
    ClIocTTLTimerKeyT *timerKey = NULL;
    clLogDebug("IOC","Rel","Send and add fragment [%d] of message [%d] to sender buffer ", fragmentId, messageId);
    node = getSenderBufferNode(messageId, destAddress, pIocCommPort->portId);
    if (!node) {
        /*
         * create a new SenderBuffer node.
         */
        key.fragId = messageId;
        key.destAddr.nodeAddress =
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
        key.destAddr.portId = ((ClIocPhysicalAddressT *) destAddress)->portId;
        key.sendAddr.nodeAddress = gIocLocalBladeAddress;
        key.sendAddr.portId = pIocCommPort->portId;
        //clLogDebug("IOC", "Rel", "add sender buffer [%d] - [%d] - [%d] - [%d] - [%d] - %d",messageId,key.sendAddr.nodeAddress,key.sendAddr.portId,key.destAddr.nodeAddress,key.destAddr.portId,(int)commPortHandle);
        ClUint32T hashKey = __iocSenderBufferHashKey(&key);
        ClIocTTLTimerKeyT *timerKey = NULL;
        timerKey = clHeapCalloc(1, sizeof(*timerKey));
        timerKey = (ClIocTTLTimerKeyT*) clHeapCalloc(1, sizeof(*timerKey));
        CL_ASSERT(timerKey != NULL);
        node = clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        memcpy(&timerKey->key, &key, sizeof(timerKey->key)); /*safe w.r.t node deletes*/
        node->ttlTimerKey = timerKey;
        timerKey->timerId = ++iocSenderCurrentTimerId;
        clRbTreeInit(&node->senderBufferTree, __iocSenderFragmentCmp);
        ClRcT rc;
//        clLogDebug("IOC", "Rel", "Create 1TTL timer");
        rc = clTimerCreate(userTTLTimerExpiry, CL_TIMER_ONE_SHOT,
                CL_TIMER_SEPARATE_CONTEXT, senderTTLCallback, (void *) timerKey,
                &timerKey->ttlTimer);
        hashAdd(iocSenderBufferHashTable, hashKey, &node->hash);
        clTimerStart(node->ttlTimerKey->ttlTimer);
        node->destAddress.iocPhyAddress.nodeAddress =
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
        node->destAddress.iocPhyAddress.portId =
                ((ClIocPhysicalAddressT *) destAddress)->portId;
        node->totalLost = 0;
        node->currentFragment = 0;
        node->numFragments = totalFragment;
        node->sendLossList=NULL;
        node->commPortHandle = commPortHandle;

    }
    ClIocReliableFragmentSenderNodeT *fragmentNode = NULL;
    fragmentNode = (ClIocReliableFragmentSenderNodeT*) clHeapCalloc(1,sizeof(*fragmentNode));
    CL_ASSERT(fragmentNode != NULL);
    ClUint8T* copyStream;
    copyStream = clHeapCalloc(1,fragmentSize);
    memcpy(copyStream, target, fragmentSize);
    ClRcT retCode = clBufferCreateAndAllocate(fragmentSize, &fragmentNode->buffer);
    CL_ASSERT(retCode == CL_OK);
    ClUint32T i;
    for (i = 0; i < targetVectors; i++)
    {
        if (target[i].iov_base)
        {
            ClUint8T* iov_base = NULL;
            iov_base = (ClUint8T *)clHeapAllocate(target[i].iov_len);
            memcpy(iov_base, target[i].iov_base, target[i].iov_len);
            retCode = clBufferNBytesWrite(fragmentNode->buffer, iov_base,target[i].iov_len);
            clHeapFree(iov_base);
            iov_base = NULL;
        }
    }
    clHeapFree(copyStream);
    fragmentNode->fragmentSize = fragmentSize;
    fragmentNode->fragmentId = fragmentId;
    node->currentFragment = fragmentId;
    clRbTreeInsert(&node->senderBufferTree, &fragmentNode->tree);
}

static void senderBufferAddMessage(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, ClBufferHandleT pBuffer, ClUint32T length,
        ClIocAddressT *destAddress, ClUint32T totalFragment) {
    clLogDebug("IOC", "Rel", "Add message [%d] to sender buffer", messageId);
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) commPortHandle;
    ClIocReliableBufferKeyT key = { 0 };
    ClIocReliableSenderNodeT *node = NULL;
    node = getSenderBufferNode(messageId, destAddress, pIocCommPort->portId);
    if (!node){
        /*
         * create a new SenderBuffer node.
         */
        key.fragId = messageId;
        key.destAddr.nodeAddress =
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
        key.destAddr.portId = ((ClIocPhysicalAddressT *) destAddress)->portId;
        key.sendAddr.nodeAddress = gIocLocalBladeAddress;
        key.sendAddr.portId = pIocCommPort->portId;
        //clLogDebug("IOC", "Rel", "add sender buffer [%d] - [%d] - [%d] - [%d] - [%d] - %d",messageId,key.sendAddr.nodeAddress,key.sendAddr.portId,key.destAddr.nodeAddress,key.destAddr.portId,(int)commPortHandle);
        ClUint32T hashKey = __iocSenderBufferHashKey(&key);
        ClIocTTLTimerKeyT *timerKey = NULL;
        timerKey = clHeapCalloc(1, sizeof(*timerKey));
        timerKey = (ClIocTTLTimerKeyT*) clHeapCalloc(1, sizeof(*timerKey));
        CL_ASSERT(timerKey != NULL);
        node = clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        memcpy(&timerKey->key, &key, sizeof(timerKey->key)); /*safe w.r.t node deletes*/
        node->ttlTimerKey = timerKey;
        timerKey->timerId = ++iocSenderCurrentTimerId;
        clRbTreeInit(&node->senderBufferTree, __iocSenderFragmentCmp);
        ClRcT rc;
        clLogDebug("IOC", "Rel", "Create TTL timer");
        rc = clTimerCreate(userTTLTimerExpiry, CL_TIMER_ONE_SHOT,
                CL_TIMER_SEPARATE_CONTEXT, senderTTLCallback, (void *) timerKey,
                &timerKey->ttlTimer);
        rc = clTimerCreate(userResendTimerExpiry, CL_TIMER_REPETITIVE,
                CL_TIMER_SEPARATE_CONTEXT, senderResendCallback,
                (void *) timerKey, &node->resendTimer);
        hashAdd(iocSenderBufferHashTable, hashKey, &node->hash);
        clTimerStart(node->ttlTimerKey->ttlTimer);
        clTimerStart(node->resendTimer);
        node->destAddress.iocPhyAddress.nodeAddress =
                ((ClIocPhysicalAddressT *) destAddress)->nodeAddress;
        node->destAddress.iocPhyAddress.portId =
                ((ClIocPhysicalAddressT *) destAddress)->portId;
        node->totalLost = 0;
        node->currentFragment = 0;
        node->numFragments = totalFragment;
        node->messageLength = length;
        node->commPortHandle = commPortHandle;
        ClUint8T * copyStream = NULL;
        ClRcT retCode = clBufferFlatten(pBuffer, &copyStream);
        CL_ASSERT(retCode == CL_OK);
        retCode = clBufferCreateAndAllocate(length, &node->buffer);
        CL_ASSERT(retCode == CL_OK);
        retCode = clBufferNBytesWrite(node->buffer, copyStream, length);
        CL_ASSERT(retCode == CL_OK);
        clHeapFree(copyStream);
    }
}

/*
 * Functionality: Process ACK package
 * Param :
 * destinationAddress : address of IOC message destination
 * fragmentId : fragment ID
 */
ClRcT senderBufferACKCallBack(ClUint32T messageId, ClIocAddressT *destAddress,
        ClUint32T fragmentId, ClIocPortT portId) {

    ClIocReliableSenderNodeT *node = NULL;
    ClRbTreeT *iter = NULL;
    node = getSenderBufferNode(messageId, destAddress, portId);
    if (!node) {
        clLogDebug("IOC", "Rel", "ACK callback : node not found");
        return CL_FALSE;
    }else
    {
        if (fragmentId == 0)
        {
            clLogDebug(
                    "IOC",
                    "Rel",
                    "Deleting message from sender buffer with message Id [%d]...", messageId);
            hashDel(&node->hash);
            /*
             * Atomically check and delete timer if not running.
             */
            if (clTimerCheckAndDelete(&node->ttlTimerKey->ttlTimer) == CL_OK) {
                clHeapFree(node->ttlTimerKey);
                clLogDebug("IOC", "Rel", "remove timeToLive Timer");
                node->ttlTimerKey = NULL;
            }
            if (clTimerCheckAndDelete(&node->resendTimer) == CL_OK) {
                clHeapFree(node->resendTimer);
                clLogDebug("IOC", "Rel", "remove resend Timer");
                node->resendTimer=NULL;
            }
            destroyNodes(&node->sendLossList);
            clHeapFree(node);
            node=NULL;
        }
        else
        {
            if (node->ackSync == fragmentId)
            {
                //add all un ack fragment to  sender loss list (current send and ackSync)
                clLogDebug("IOC", "Rel","receive previous ack .Not process now ");
                return CL_OK;
            }
            else
            {
                ClRbTreeT *iter, *next = NULL;
                clLogDebug("IOC", "Rel","receiver ack. remove all fragment < [%d]",fragmentId);
                for (iter = clRbTreeMin(&node->senderBufferTree); iter; iter = next)
                {
                    ClIocReliableFragmentSenderNodeT *fragNode =
                            CL_RBTREE_ENTRY(iter, ClIocReliableFragmentSenderNodeT, tree);
                    next = clRbTreeNext(&node->senderBufferTree, iter);
                    if (fragNode->fragmentId <= fragmentId)
                    {
                        if (&fragNode->buffer)
                        {
//                            clLogDebug("IOC", "Rel","remove fragment [%d]",fragNode->fragmentId);
                            clBufferDelete(&fragNode->buffer);
                        }
                        if(node->sendLossList)
                        {
                            lossListDelete(&node->sendLossList, fragNode->fragmentId);
                        }
                        clRbTreeDelete(&node->senderBufferTree, iter);
                        clHeapFree(fragNode);

                    }
                }
                node->ackSync = fragmentId;
            }
            //*********************************************************
            //get the last fragment ack of this message,delete hask and clean
            if (fragmentId == node->numFragments)
            {
                clLogDebug("IOC", "Rel","receive ack of last fragment. Remove message");
                hashDel(&node->hash);
                if (clTimerCheckAndDelete(&node->ttlTimerKey->ttlTimer) == CL_OK) {
                    clHeapFree(node->ttlTimerKey);
                    clLogDebug("IOC", "Rel", "remove timeToLive Timer");
                    node->ttlTimerKey = NULL;
                }
                destroyNodes(&node->sendLossList);
                clHeapFree(node);
                node=NULL;
                clLogDebug("IOC", "Rel","removed message in sender buffer");
            }
        }
    }

    return CL_TRUE;

}

inline static int fragcmp(int32_t seq1, int32_t seq2) {
    return (abs(seq1 - seq2) < 0 ? (seq1 - seq2) : (seq2 - seq1));
}

ClRcT senderBufferNAKCallBack(ClUint32T messageId, ClIocAddressT *destAddress,
        ClIocPortT portId, ClUint32T* losslist, ClUint32T size) {
    ClIocReliableSenderNodeT *node = NULL;
    node = getSenderBufferNode(messageId, destAddress, portId);
    if (!node)
    {
        return CL_FALSE;
        clLogDebug("IOC", "Rel", "node entry not found.");
    }
    else
    {
        ClUint32T i;
        for (i = 0; i < size; ++i)
        {
            ClUint32T lossFragment= ntohl(losslist[i]);
            clLogDebug("IOC", "Rel", "Add fragment [%d] to loss list",lossFragment);
            ClUint32T count;
            count = lossListInsertRange(&node->sendLossList,lossFragment,lossFragment);
        }
        node->totalLost = lossListCount(&node->sendLossList);
    }
    return CL_OK;
}

static ClRcT senderBufferRetranmission(ClIocCommPortHandleT commPortHandle,
        ClUint32T messageId, ClUint32T fragmentId, ClIocAddressT *destAddress,
        ClCharT *xportType, ClBoolT proxy)
{
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *) commPortHandle;
    ClIocReliableSenderNodeT *node = NULL;
    node = getSenderBufferNode(messageId, destAddress, pIocCommPort->portId);
    ClRcT retCode, timeout;
    ClRbTreeT *iter, *next = NULL;
    ClUint32T count =1,fragmentIdTemp;
    clLogDebug("IOC","Rel","retranmission fragment id [%d] of message [%d].", fragmentId, messageId);
    if (!node) {
        clLogDebug("IOC","Rel","retranmission fragment: No node found");
        return CL_ERR_NOT_EXIST;
    } else
    {
        //get fragment via fragment offset and messageId, retransmit this fragment
        for (iter = clRbTreeMin(&node->senderBufferTree); iter; iter = next) {
            ClIocReliableFragmentSenderNodeT *fragNode =
                    CL_RBTREE_ENTRY(iter, ClIocReliableFragmentSenderNodeT, tree);
//            clLogDebug("IOC", "Rel", "loop get fragment [%d] , fragId [%d]",count,fragNode->fragmentId);
            fragmentIdTemp=fragmentId;
            count++;
            if (fragNode->fragmentId == fragmentIdTemp) {
                ClIocAddressT interimDestAddress = { { 0 } };
                interimDestAddress = *destAddress;
                retCode = clFindTransport(
                        ((ClIocPhysicalAddressT*) destAddress)->nodeAddress,
                        &interimDestAddress, &xportType);
                timeout = CL_RETRANMISSION_TIMEOUT;
                //vector buffer here
                struct iovec *pIOVector = NULL;
                ClInt32T ioVectorLen = 0;
                retCode = clBufferVectorize(fragNode->buffer, &pIOVector, &ioVectorLen);
                if (retCode != CL_OK) {
                    clHeapFree(pIOVector);
                    clLogDebug("IOC", "Rel","Error in buffer vectorize.rc=0x%x\n",retCode);
                    clHeapFree(pIOVector);
                    return CL_ERR_NOT_IMPLEMENTED;
                }
                retCode = internalSend(pIocCommPort, pIOVector,
                        ioVectorLen, fragNode->fragmentSize,
                        CL_RETRANMISSION_PRIORITY, &interimDestAddress,
                        &timeout, xportType, proxy);
                clHeapFree(pIOVector);
                return retCode;
            }
            next = clRbTreeNext(&node->senderBufferTree, iter);
        }
    }
    return CL_OK;
}

ClUint32T senderBufferLossListGetFirst(ClIocAddressT *destAddress,
        ClUint32T messageId, ClIocPortT portId)
{
    ClIocReliableSenderNodeT *node = NULL;
    node = getSenderBufferNode(messageId, destAddress, portId);
    if (!node)
    {
        return CL_FALSE;
    }
    else
    {
        return node->sendLossList->fragmentID;
    }
}

ClRcT senderBufferLossListRemoveFirst(ClIocAddressT *destAddress,
        ClUint32T messageId, ClUint32T fragmentId, ClIocPortT portId)
{
    ClIocReliableSenderNodeT *node = NULL;
    node = getSenderBufferNode(messageId, destAddress, portId);
    if (!node)
    {
        return CL_FALSE;
    }
    else
    {
        lossListDelete(&node->sendLossList, fragmentId);
//        node->totalLost--;
        node->totalLost = lossListCount(&node->sendLossList);
    }
    return CL_OK;
}

ClRcT senderPiggyBackCallback(ClIocFragmentJobT *fragmentJob)
{
    ClUint32T ackFragment;
    ClRcT rc;
    ackFragment = fragmentJob->fragHeader.header.piggyBackACK;
    if (ackFragment == 0)
    {
        return CL_OK;
    }
    else
    {
        ClUint32T ackmessageId =
                fragmentJob->fragHeader.header.piggyBackACKMessageId;
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
                fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId =
                fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.portId;
        rc = senderBufferACKCallBack(ackmessageId, &destAddress, ackFragment,
                fragmentJob->portId);
    }
    return rc;
}

/**************************************************************************************************************
 * RELIABLE IOC RECEIVER
 ***************************************************************************************************************/
ClInt32T iocFragmentIdCmp(ClUint32T fragId1, ClUint32T fragId2)
{
    return (fragId1 - fragId2);
}

ClIocReassembleNodeT * getReceiverBufferNode(ClUint32T messageId,
        ClIocAddressT *srcAddress, ClIocPortT portId)
{
    ClIocReassembleKeyT key = { 0 };
    ClIocReassembleNodeT *node = NULL;
    key.fragId = messageId;
    key.sendAddr.nodeAddress =
            ((ClIocPhysicalAddressT *) srcAddress)->nodeAddress;
    key.sendAddr.portId = ((ClIocPhysicalAddressT *) srcAddress)->portId;
    key.destAddr.nodeAddress = gIocLocalBladeAddress;
    key.destAddr.portId = portId;
    node = __iocReassembleNodeFind(&key, 0);
    if (!node)
    {
        return NULL;
    }
    else
    {
        return node;
    }
}

ClRcT receiverDropMsgCallback(ClIocAddressT *srcAddress, ClUint32T messageId,
        ClIocPortT portId)
{
    ClIocReassembleNodeT *node = NULL;
    ClRbTreeT *iter;
    clLogDebug("IOC", "Rel", "process DROP message.");
    node = getReceiverBufferNode(messageId, srcAddress, portId);
    if (!node)
    {
        return CL_FALSE;
    }
    else
    {
        //delete all buffer on node
        clLogDebug("IOC", "Rel",
                "Delete all fragment of message [%d].", messageId);
        while ((iter = clRbTreeMin(&node->reassembleTree))) {
            ClIocFragmentNodeT *fragNode =
                    CL_RBTREE_ENTRY(iter, ClIocFragmentNodeT, tree);
            //__iocSenderFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
            clRbTreeDelete(&node->reassembleTree, iter);
            clHeapFree(fragNode);
        }
        hashDel(&node->hash);

        /*
         * Atomically check and delete timer if not running.
         */
        if (clTimerCheckAndDelete(&node->timerKey->reassembleTimer) == CL_OK) {
            clHeapFree(node->timerKey);
            node->timerKey = NULL;
        }
        clHeapFree(node);
    }
    return CL_OK;
}

ClRcT receiverAckSend(ClIocCommPortT *commPort, ClIocAddressT *dstAddress,
        ClUint32T messageId, ClUint32T fragmentId)
{
    ClRcT rc;
    clLogDebug("IOC", "Rel", "sending ack fragment [%d] to sender address [%d] port [%d]" ,fragmentId,dstAddress->iocPhyAddress.nodeAddress,dstAddress->iocPhyAddress.portId);
    ClBufferHandleT message = 0;
    clBufferCreate(&message);
    ClUint32T msgId = htonl(messageId);
    ClUint32T frgId = htonl(fragmentId);
    rc = clBufferNBytesWrite(message, (ClUint8T *) &msgId, sizeof(ClUint32T));
    if (rc != CL_OK)
    {
        clLogError("IOC", "Rel",
                "clBufferNBytesWrite failed with rc = %#x", rc);
        goto out_delete;
    }
    rc = clBufferNBytesWrite(message, (ClUint8T *) &frgId, sizeof(ClUint32T));
    if (rc != CL_OK)
    {
        clLogError("IOC", "Rel",
                "clBufferNBytesWrite failed with rc = %#x", rc);
        goto out_delete;
    }
    ClIocSendOptionT sendOption;
    sendOption.priority = CL_IOC_HIGH_PRIORITY;
    sendOption.timeout = 200;
    if (fragmentId != 0)
    {
        rc = clIocSend((ClIocCommPortHandleT) commPort, message,
                CL_IOC_SEND_ACK_PROTO, dstAddress, &sendOption);
    }
    else
    {
        clLogDebug("IOC", "Rel","sending ack message [%d] to sender", messageId);
        rc = clIocSend((ClIocCommPortHandleT) commPort, message,
                CL_IOC_SEND_ACK_MESSAGE_PROTO, dstAddress, &sendOption);
    }
    out_delete: clBufferDelete(&message);
    return rc;
}

ClRcT receiverNakSend(ClIocCommPortT *commPort, ClIocAddressT *dstAddress,
        ClBufferHandleT message)
{
    ClRcT rc;
    ClIocSendOptionT sendOption;
    sendOption.priority = CL_IOC_DEFAULT_PRIORITY;
    sendOption.timeout = 200;
    rc = clIocSend((ClIocCommPortHandleT) commPort, message,
            CL_IOC_SEND_NAK_PROTO, dstAddress, &sendOption);
    return rc;
}

ClRcT receiverACKTrigger(void* key)
{
    clOsalMutexLock(&iocAcklock);
    if(key==NULL)
    {
        clLogDebug("IOC", "Rel", "receiverACKTrigger : timer is deleted.");
        clOsalMutexUnlock(&iocReassembleLock);
        return CL_OK;
    }
    ClUint32T ack = 0;
    struct hashStruct *iter = NULL;
    ClIocReassembleNodeT *node = NULL;
    ClRcT retCode;
    ClIocReassembleTimerKeyT *timerKey = key;
    node = __iocReassembleNodeFind(&timerKey->key, timerKey->timerId);
    if (!node)
    {
        clLogDebug("IOC", "Rel", "receiverACKTrigger : no node found");
        goto error;
    }
    else if (node->lossTotal > 0)
    {
        ack = lossListGetFirst(node->receiverLossList)-1;
        if(ack<= (node->ackSync) - 3 && node->lastAckSync==ack)
        {
            //trigger send NAK to sender
            ClBufferHandleT message = 0;
            clBufferCreate(&message);
            ClRcT rc;
            clLogDebug("IOC", "Rel", "check loss list to send loss fragment. ack [%d] las ack [%d] ackSync [%d]", ack, node->lastAckSync, node->ackSync);
            ClFragmentListHeadT* r = node->receiverLossList;
            if (!r)
            {
                clLogDebug("IOC", "Rel","receiver loss list NULL");
                rc = clBufferDelete(&message);
                clOsalMutexUnlock(&iocAcklock);
                return CL_OK;
            }
            ClUint32T msgId = htonl(node->messageId);
            rc = clBufferNBytesWrite(message, (ClUint8T*) &msgId,sizeof(ClUint32T));
            if (rc != CL_OK)
            {
                clLogDebug("IOC", "Rel","clBufferNBytesWrite messageId [%d] failed with rc = %#x",msgId,rc);
                rc = clBufferDelete(&message);
                clOsalMutexUnlock(&iocAcklock);
                return CL_OK;
            }
            while (r != NULL)
            {
                ClUint32T fragment = htonl(r->fragmentID);
                rc = clBufferNBytesWrite(message, (ClUint8T *) &fragment,sizeof(ClUint32T));
                if (rc != CL_OK)
                {
                    clLogDebug("IOC", "Rel", "clBufferNBytesWrite failed with rc = %#x", rc);
                    clOsalMutexUnlock(&iocAcklock);
                    rc = clBufferDelete(&message);
                    return CL_OK;
                }
                break;
                r = r->pNext;
            }
            ClIocCommPortT *commPort = clIocGetPort(node->commPort);
            clLogDebug("IOC", "Rel","There are [%d] fragment loss to send",lossListCount(&node->receiverLossList));
            ClIocAddressT srcAddress = {{0}};
            srcAddress.iocPhyAddress.nodeAddress = node->srcAddress.iocPhyAddress.nodeAddress;
            srcAddress.iocPhyAddress.portId = node->srcAddress.iocPhyAddress.portId;
            receiverNakSend(commPort, &srcAddress, message);
            node->lastAckSync=0;
            clOsalMutexUnlock(&iocAcklock);
            rc = clBufferDelete(&message);
            return CL_OK;
        }

    }
    else
    {
            ack = node->ackSync;
            if(ack==node->lastAckSync)
            {
                clOsalMutexUnlock(&iocAcklock);
                return CL_OK;
            }
    }
    node->lastAckSync=ack;
    ClIocCommPortT *commPort = clIocGetPort(node->commPort);
    ClUint32T messageId=node->messageId;
    ClIocAddressT srcAddress = {{0}};
    srcAddress.iocPhyAddress.nodeAddress = node->srcAddress.iocPhyAddress.nodeAddress;
    srcAddress.iocPhyAddress.portId = node->srcAddress.iocPhyAddress.portId;
    receiverAckSend(commPort, &srcAddress, node->messageId, ack);
    error:
    clOsalMutexUnlock(&iocAcklock);
    return CL_OK;
}

ClRcT receiverLastACKTrigger(ClIocReassembleNodeT *node)
{
    ClUint32T ack = 0;
    struct hashStruct *iter = NULL;
    ClRcT retCode;
    if (!node)
    {
        clLogDebug("IOC", "Rel", "receiverLastACKTrigger : no node found for message id [%d]",node->messageId);
        goto error;
    }
    ack = node->numFragments;
    ClIocCommPortT *commPort = clIocGetPort(node->commPort);
    clLogDebug("IOC", "Rel", "trigger to send last ack");
    ClIocAddressT srcAddress = {{0}};
    srcAddress.iocPhyAddress.nodeAddress = node->srcAddress.iocPhyAddress.nodeAddress;
    srcAddress.iocPhyAddress.portId = node->srcAddress.iocPhyAddress.portId;
    receiverAckSend(commPort, &srcAddress, node->messageId, ack);
    node->lastAckSync=ack;
    error:
    return CL_OK;
}

//ClRcT receiverNAKTrigger(void* key)
//{
//    ClUint32T ack = 0;
//    struct hashStruct *iter = NULL;
//    ClUint32T i,count;
//    clLogDebug("IOC", "Rel", "trigger to send NAK fragment");
//    ClRcT rc;
//    ClIocReassembleNodeT *node = NULL;
//    ClRcT retCode;
//    ClIocTTLTimerKeyT *timerKey = (ClIocTTLTimerKeyT*) key;
//    node = __iocReassembleNodeFind(&timerKey->key, timerKey->timerId);
//    if (!node)
//    {
//        clLogDebug("IOC", "Rel", "receiverNAKTrigger : no node found");
//        goto error;
//    }
//    else
//    {
//        ClBufferHandleT message = 0;
//        clBufferCreate(&message);
//        if (node->lossTotal > 0)
//        {
//            ClFragmentListHeadT* r = node->receiverLossList;
//            if (!r)
//            {
//                clLogDebug("IOC", "Rel","receiver loss list NULL");
//                rc = clBufferDelete(&message);
//                return CL_OK;
//            }
//            ClUint32T msgId = htonl(node->messageId);
//            rc = clBufferNBytesWrite(message, (ClUint8T*) &msgId,
//                    sizeof(ClUint32T));
//            if (rc != CL_OK)
//            {
//                clLogDebug("IOC", "Rel","clBufferNBytesWrite failed with rc = %#x", rc);
//                goto error;
//            }
//            ClUint32T fragment = htonl(r->fragmentID);
//            rc = clBufferNBytesWrite(message, (ClUint8T *) &fragment,sizeof(ClUint32T));
//        }
//        else
//        {
//            clLogDebug("IOC", "Rel", "No loss fragment");
//            return CL_OK;
//        }
//        ClIocCommPortT *commPort = clIocGetPort(node->commPort);
//        clLogDebug("IOC", "Rel","There are [%d] fragment loss",lossListCount(&node->receiverLossList));
//        receiverNakSend(commPort, &node->srcAddress, message);
//        error:
//        clBufferDelete(&message);
//    }
//    return CL_OK;
//}

ClRcT receiverNAKTriggerDirect(ClUint32T messageId,ClUint32T fragmentBegin,ClUint32T fragmentEnd,ClIocAddressT* srcAddress,ClIocPortT commPort)
{
    ClUint32T ack = 0;
    struct hashStruct *iter = NULL;
    ClUint32T i,count;
    clLogDebug("IOC", "Rel", "trigger to send NAK fragment from [%d] to [%d]",fragmentBegin,fragmentEnd);
    ClRcT rc;
    ClRcT retCode;
    ClBufferHandleT message = 0;
    clBufferCreate(&message);
    ClUint32T msgId = htonl(messageId);
    rc = clBufferNBytesWrite(message, (ClUint8T*) &msgId,
               sizeof(ClUint32T));
    if (rc != CL_OK)
    {
        clLogDebug("IOC", "Rel","clBufferNBytesWrite failed with rc = %#x", rc);
        goto error;
    }
    for(i= fragmentBegin;i<=fragmentEnd;i++)
    {
        ClUint32T fragment = htonl(i);
        rc = clBufferNBytesWrite(message, (ClUint8T *) &fragment,
                    sizeof(ClUint32T));
        if (rc != CL_OK)
        {
            clLogDebug("IOC", "Rel", "clBufferNBytesWrite failed with rc = %#x", rc);
            goto error;
        }
    }
    ClIocCommPortT *cPort = clIocGetPort(commPort);
    receiverNakSend(cPort, srcAddress, message);
    error:
    clBufferDelete(&message);
    return CL_OK;
}

#endif

