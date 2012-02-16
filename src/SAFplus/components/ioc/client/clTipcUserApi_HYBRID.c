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
 * File        : clIocTipcUserApi.c
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


#if OS_VERSION_CODE < OS_KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

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
#include <clTipcMaster.h>
#include <clTipcNotification.h>
#include <clTipcNeighComps.h>
#include <clTipcUserApi.h>
#include <clTipcSetup.h>
#include <clTipcGeneral.h>
#include <clLeakyBucket.h>
#include <clNodeCache.h>
#include <clNetwork.h>
#include <clTimeServer.h>
#include <clTransport.h>

#ifdef CL_TIPC_COMPRESSION

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


#define CL_IOC_BLOCK_SIZE (1024)
#define CL_IOC_ALIGN_VAL(v,align) (((v) + (align) - 1) & ~((align)-1))
#define CL_IOC_MCAST_VALID(pMcast) (CL_IOC_ADDRESS_TYPE_GET(pMcast) == CL_IOC_MULTICAST_ADDRESS_TYPE)
#define IOC_PHY_CMP(phy1,phy2) (((phy1)->nodeAddress == (phy2)->nodeAddress) ? \
        (ClInt32T) ((phy1)->portId - (phy2)->portId) :                         \
        (ClInt32T)((phy1)->nodeAddress - (phy2)->nodeAddress))

#ifdef CL_TIPC_COMPRESSION
#define CL_TIPC_COMPRESSION_BOUNDARY (0x1000U) /*mark 4K to be the compression limit*/
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
 * above to avoid tipc skbuffs being dropped by the kernel from queueing on hitting the backlog queue limit.
 */
#define CL_LEAKY_BUCKET_DEFAULT_VOL (50 << 20)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE (25 << 20)
#define CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL (500)

extern ClUint32T clAspLocalId;

ClIocNodeAddressT gIocLocalBladeAddress;

#ifdef CL_TIPC_COMPRESSION
static ClUint32T gTipcCompressionBoundary;
#endif

static ClBoolT tipcPriorityChangePossible = CL_TRUE;  /* Don't attempt to change priority if TIPC does not support, so we don't get tons of error msgs */
static ClUint32T gIocInit = CL_FALSE;
static ClOsalMutexIdT ClIocFragMutex;
static ClUint32T currFragId;
static ClIocUserObjectT userObj;
static ClTimerTimeOutT userReassemblyTimerExpiry = { 0 };
ClBoolT gClIocTrafficShaper;
static ClBoolT gClTipcReplicast;

typedef struct
{
    ClTipcFragHeaderT *header;
    ClBufferHandleT fragPkt;
}ClTipcFragNodeT;


static ClRcT iocUserFragmentReceive(ClBufferHandleT *pOrigMsg,
        ClUint8T *pBuffer,
        ClTipcFragHeaderT *hdr,
                                    ClIocPortT portId, ClUint32T *pLength) __attribute__((unused));
static ClRcT userReassemblyTimer(void *data);
static ClRcT iocUserReassemblePacket(ClBufferHandleT *pOrigMsg,
        ClCntNodeHandleT nodeHandle,
        ClIocReassemblyNodeT *node,
        ClUint32T *pLength);
static ClInt32T iocUserKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void iocUserDeleteCallBack(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData);
static ClInt32T iocUserFragKeyCompare(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2);
static void iocUserFragDeleteCallBack(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData);
static ClRcT userReassemblyTimer(void *data);
static ClRcT clIocReplicastGet(ClIocPortT portId, ClIocAddressT **pAddressList, ClUint32T *pNumEntries);

#define IOC_REASSEMBLE_HASH_BITS (12)
#define IOC_REASSEMBLE_HASH_SIZE ( 1 << IOC_REASSEMBLE_HASH_BITS)
#define IOC_REASSEMBLE_HASH_MASK (IOC_REASSEMBLE_HASH_SIZE - 1)

static ClOsalMutexT iocReassembleLock;
static ClJobQueueT iocFragmentJobQueue;
static struct hashStruct *iocReassembleHashTable[IOC_REASSEMBLE_HASH_SIZE];
static ClUint64T iocReassembleCurrentTimerId;
typedef ClIocReassemblyKeyT ClIocReassembleKeyT;

typedef struct ClIocReassembleNode
{
    ClRbTreeRootT reassembleTree; /*reassembly tree*/
    struct hashStruct hash; /*hash linkage*/
    ClUint32T currentLength;
    ClUint32T expectedLength;
    ClUint32T numFragments; /* number of fragments received*/
    ClIocReassembleKeyT key;
    ClTimerHandleT reassembleTimer;
    ClUint64T timerId;
}ClIocReassembleNodeT;

typedef struct ClIocReassembleTimerKey
{
    ClIocReassembleKeyT key;
    ClUint64T timerId;
}ClIocReassembleTimerKeyT;

typedef struct ClIocFragmentNode
{
    ClRbTreeT tree;
    ClUint32T fragOffset;
    ClUint32T fragLength;
    ClUint8T *fragBuffer;
}ClIocFragmentNodeT;

typedef struct ClIocFragmentJob
{
    ClUint8T *buffer;
    ClUint32T length;
    ClIocPortT portId;
    ClTipcFragHeaderT fragHeader;
}ClIocFragmentJobT;

typedef struct ClIocFragmentPool
{
    ClListHeadT list;
    ClUint8T *buffer;
}ClIocFragmentPoolT;

static CL_LIST_HEAD_DECLARE(iocFragmentPool);
static ClOsalMutexT iocFragmentPoolLock;
static ClUint32T iocFragmentPoolLen;
static ClUint32T iocFragmentPoolSize = 1024*1024;
static ClUint32T iocFragmentPoolEntries;
static ClUint32T iocFragmentPoolLimit;

typedef struct ClTipcLogicalAddress
{
    ClTipcCommPortT *pTipcCommPort;
    ClIocLogicalAddressT logicalAddress;
    ClUint32T haState;
    ClListHeadT list;
}ClTipcLogicalAddressT;

typedef struct ClTipcMcast
{
    ClIocMulticastAddressT mcastAddress;
    struct hashStruct hash;
    ClListHeadT portList;
}ClTipcMcastT;

/*port list for the mcast*/
typedef struct ClTipcMcastPort
{
    ClListHeadT listMcast;
    ClListHeadT listPort;
    ClTipcCommPortT *pTipcCommPort;
    ClTipcMcastT *pMcast;
}ClTipcMcastPortT;

typedef struct ClTipcNeighborList
{
    ClListHeadT neighborList;
    ClOsalMutexT neighborMutex;
    ClUint32T numEntries;
}ClTipcNeighborListT;

typedef struct ClTipcNeighbor
{
    ClListHeadT list;
    ClIocNodeAddressT address;
    ClUint32T status;
}ClTipcNeighborT;

static ClTipcNeighborListT gClTipcNeighborList = {
    .neighborList = CL_LIST_HEAD_INITIALIZER(gClTipcNeighborList.neighborList),
};


#define CL_IOC_MICRO_SLEEP_INTERVAL 1000*10 /* 10 milli second */

#define CL_IOC_TIPC_TYPE(iocAddress) clTipcSetType(((ClUint32T)(((ClUint64T)(iocAddress)>>32)&0xffffffffU)),CL_FALSE)
#define CL_IOC_TIPC_INSTANCE(iocAddress) ((ClUint32T)(((ClUint64T)(iocAddress))&0xffffffffU))


#define CL_TIPC_DUPLICATE_NODE_TIMEOUT (100)

#define CL_TIPC_PORT_EXIT_MESSAGE "QUIT"

#define NULL_CHECK(X)                               \
    do {                                            \
        if((X) == NULL)                             \
        return CL_IOC_RC(CL_ERR_NULL_POINTER);  \
    } while(0)

#define CL_IOC_TIPC_PORT_BITS (10)

#define CL_IOC_TIPC_PORT_BUCKETS ( 1 << CL_IOC_TIPC_PORT_BITS )

#define CL_IOC_TIPC_PORT_MASK (CL_IOC_TIPC_PORT_BUCKETS-1)


#define CL_IOC_TIPC_COMP_BITS (2)

#define CL_IOC_TIPC_COMP_BUCKETS ( 1 << CL_IOC_TIPC_COMP_BITS )

#define CL_IOC_TIPC_COMP_MASK (CL_IOC_TIPC_COMP_BUCKETS-1)


#define CL_IOC_TIPC_MCAST_BITS (10)

#define CL_IOC_TIPC_MCAST_BUCKETS ( 1 << CL_IOC_TIPC_MCAST_BITS ) 

#define CL_IOC_TIPC_MCAST_MASK (CL_IOC_TIPC_MCAST_BUCKETS - 1)


static struct hashStruct *ppTipcPortHashTable[CL_IOC_TIPC_PORT_BUCKETS];
static struct hashStruct *ppTipcCompHashTable[CL_IOC_TIPC_COMP_BUCKETS];
static struct hashStruct *ppTipcMcastHashTable[CL_IOC_TIPC_MCAST_BUCKETS];
static ClOsalMutexT gClTipcPortMutex;
static ClOsalMutexT gClTipcCompMutex;
static ClOsalMutexT gClTipcMcastMutex;


ClRcT clIocNeighborScan(void);
static ClRcT clIocNeighborAdd(ClIocNodeAddressT address,ClUint32T status);
static ClTipcNeighborT *clIocNeighborFind(ClIocNodeAddressT address);

/*  
 * When running with asp modified tipc supporting 64k.
 */
#undef CL_TIPC_PACKET_SIZE
#define CL_TIPC_PACKET_SIZE (64000)

#define longTimeDiff(tm1, tm2) ((tm2.tsSec - tm1.tsSec) * 1000 + (tm2.tsMilliSec - tm1.tsMilliSec))
#define clIocInternalMaxPayloadSizeGet()  (CL_TIPC_PACKET_SIZE - sizeof(ClTipcHeaderT) - sizeof(ClTipcHeaderT) - 1)

static ClRcT internalSendSlow(ClTipcCommPortT *pTipcCommPort,
                          ClBufferHandleT message, 
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout);

static ClRcT internalSendSlowReplicast(ClTipcCommPortT *pTipcCommPort,
                          ClBufferHandleT message, 
                          ClUint32T tempPriority, 
                          ClUint32T *pTimeout,
                          ClIocAddressT *replicastList,
                          ClUint32T numReplicasts);

static ClRcT internalSend(ClTipcCommPortT *pTipcCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout);

static ClRcT internalSendReplicast(ClTipcCommPortT *pTipcCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClUint32T *pTimeout,
                          ClIocAddressT *replicastList,
                          ClUint32T numReplicasts);

static ClUint32T gClTipcPacketSize = CL_TIPC_PACKET_SIZE;

ClUint32T clTipcMcastType(ClUint32T type)
{
    CL_ASSERT((type + CL_TIPC_BASE_TYPE) < 0xffffffff);
    return (type + CL_TIPC_BASE_TYPE);
}
       
static void __iocFragmentPoolPut(ClUint8T *pBuffer, ClUint32T len)
{
    if(len != iocFragmentPoolLen)
    {
        clHeapFree(pBuffer);
        return;
    }
    if(!iocFragmentPoolLimit)
    {
        iocFragmentPoolLen = clIocInternalMaxPayloadSizeGet();
        CL_ASSERT(iocFragmentPoolLen != 0);
        iocFragmentPoolLimit = iocFragmentPoolSize/iocFragmentPoolLen;
    }
    if(iocFragmentPoolEntries >= iocFragmentPoolLimit)
    {
        clHeapFree(pBuffer);
    }
    else
    {
        ClIocFragmentPoolT *pool = clHeapCalloc(1, sizeof(*pool));
        CL_ASSERT(pool != NULL);
        pool->buffer = pBuffer;
        clOsalMutexLock(&iocFragmentPoolLock);
        clListAddTail(&pool->list, &iocFragmentPool);
        ++iocFragmentPoolEntries;
        clOsalMutexUnlock(&iocFragmentPoolLock);
    }
}

static ClUint8T *__iocFragmentPoolGet(ClUint8T *pBuffer, ClUint32T len)
{
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *head = NULL;
    ClUint8T *buffer = NULL;
    clOsalMutexLock(&iocFragmentPoolLock);
    if(len != iocFragmentPoolLen
       ||
       CL_LIST_HEAD_EMPTY(&iocFragmentPool))
    {
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

    alloc:
    return (ClUint8T*)clHeapAllocate(len);
}

static ClRcT __iocFragmentPoolInitialize(void)
{
    ClUint32T currentSize = 0;
    iocFragmentPoolLen = clIocInternalMaxPayloadSizeGet();
    CL_ASSERT(iocFragmentPoolLen != 0);
    clOsalMutexInit(&iocFragmentPoolLock);
    while(currentSize + iocFragmentPoolLen < iocFragmentPoolSize)
    {
        ClIocFragmentPoolT *pool = clHeapCalloc(1, sizeof(*pool));
        ClUint8T *buffer = clHeapAllocate(iocFragmentPoolLen);
        CL_ASSERT(pool !=  NULL);
        CL_ASSERT(buffer != NULL);
        currentSize += iocFragmentPoolLen;
        pool->buffer = buffer;
        clListAddTail(&pool->list, &iocFragmentPool);
        ++iocFragmentPoolEntries;
        ++iocFragmentPoolLimit;
    }
    return CL_OK;
}

ClUint32T clTipcSetType(ClUint32T portId,ClBoolT setFlag)
{
    ClUint32T type = portId;
    if(setFlag == CL_TRUE)
    {
        type += CL_TIPC_BASE_TYPE;

        CL_ASSERT(type <= CL_IOC_COMM_PORT_MASK);
    }
    else
    {
        CL_ASSERT(type > CL_IOC_COMM_PORT_MASK);
    }
    return type;
}

static __inline__ ClUint32T clTipcMcastHash(ClIocMulticastAddressT mcastAddress)
{
    return (ClUint32T)((ClUint32T)mcastAddress & CL_IOC_TIPC_MCAST_MASK);
}

static ClRcT clTipcMcastHashAdd(ClTipcMcastT *pMcast)
{
    ClUint32T key = clTipcMcastHash(pMcast->mcastAddress);
    return hashAdd(ppTipcMcastHashTable,key,&pMcast->hash);
}

static __inline__ ClRcT clTipcMcastHashDel(ClTipcMcastT *pMcast)
{
    hashDel(&pMcast->hash);
    return CL_OK;
}

static ClTipcMcastT *clTipcGetMcast(ClIocMulticastAddressT mcastAddress)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clTipcMcastHash(mcastAddress);
    for(pTemp = ppTipcMcastHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClTipcMcastT *pMcast = hashEntry(pTemp,ClTipcMcastT,hash);
        if(pMcast->mcastAddress == mcastAddress)
        {
            return pMcast;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clTipcCompHash(ClUint32T compId)
{
    return  ( compId & CL_IOC_TIPC_COMP_MASK);
}

static ClRcT clTipcCompHashAdd(ClTipcCompT *pComp)
{
    ClUint32T key = clTipcCompHash(pComp->compId);
    return hashAdd(ppTipcCompHashTable,key,&pComp->hash);
}

static __inline__ ClRcT clTipcCompHashDel(ClTipcCompT *pComp)
{
    hashDel(&pComp->hash);
    return CL_OK;
}

static ClTipcCompT *clTipcGetComp(ClUint32T compId)
{
    ClUint32T key = clTipcCompHash(compId);
    register struct hashStruct *pTemp;
    for(pTemp = ppTipcCompHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClTipcCompT *pComp = hashEntry(pTemp,ClTipcCompT,hash);
        if(pComp->compId == compId)
        {
            return pComp;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clTipcPortHash(ClIocPortT portId)
{
    return (ClUint32T)(portId & CL_IOC_TIPC_PORT_MASK);
}

static ClRcT clTipcPortHashAdd(ClTipcCommPortT *pTipcCommPort)
{
    ClUint32T key = clTipcPortHash(pTipcCommPort->portId);
    ClRcT rc;
    rc = hashAdd(ppTipcPortHashTable,key,&pTipcCommPort->hash);
    return rc;
}

static __inline__ ClRcT clTipcPortHashDel(ClTipcCommPortT *pTipcCommPort)
{
    hashDel(&pTipcCommPort->hash);
    return CL_OK;
}

static ClTipcCommPortT *clTipcGetPort(ClIocPortT portId)
{
    ClUint32T key = clTipcPortHash(portId);
    register struct hashStruct *pTemp;
    for(pTemp = ppTipcPortHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClTipcCommPortT *pTipcCommPort = hashEntry(pTemp,ClTipcCommPortT,hash);
        if(pTipcCommPort->portId == portId)
        {
            return pTipcCommPort;
        }
    }
    return NULL;
}


ClIocNodeAddressT clIocLocalAddressGet()
{
    return gIocLocalBladeAddress;
}


ClRcT clIocPortNotification(ClIocPortT port, ClIocNotificationActionT action)
{
    ClRcT rc = CL_OK;
    ClTipcCommPortT *pTipcCommPort = NULL;

    clOsalMutexLock(&gClTipcPortMutex);

    pTipcCommPort = clTipcGetPort(port);
    if(pTipcCommPort == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid port [0x%x] passed.\n", port));
        rc = CL_IOC_RC(CL_ERR_DOESNT_EXIST);
        goto error_out;
    }

    pTipcCommPort->notify = action;

error_out:
    clOsalMutexUnlock(&gClTipcPortMutex);
    return rc;
}

ClRcT clIocCommPortCreate(ClUint32T portId, ClIocCommPortFlagsT portType,
                          ClIocCommPortHandleT *pIocCommPortHandle)
{
    ClTipcCommPortT *pTipcCommPort = NULL;
    struct sockaddr_tipc address;
    ClRcT rc = CL_OK;
    ClInt32T ret;
    ClUint32T priority = CL_IOC_TIPC_DEFAULT_PRIORITY;

    NULL_CHECK(pIocCommPortHandle);


    if(portId >= (CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS)) {
        clDbgCodeError(CL_IOC_RC(CL_ERR_INVALID_PARAMETER),("Requested commport [%d] is out of range. OpenClovis ASP ports should be between [1-%d].  Application ports between [%d-%d]", portId,CL_IOC_RESERVED_PORTS,CL_IOC_RESERVED_PORTS+1,CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }


    pTipcCommPort = clHeapCalloc(1,sizeof(*pTipcCommPort));
    CL_ASSERT(pTipcCommPort != NULL);

    if(portType == CL_IOC_RELIABLE_MESSAGING)
        pTipcCommPort->fd = socket(AF_TIPC,SOCK_RDM,0);
    else if (portType == CL_IOC_UNRELIABLE_MESSAGING)
        pTipcCommPort->fd = socket(AF_TIPC,SOCK_DGRAM,0);
    else
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(pTipcCommPort->fd < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : socket() failed. system error [%s].\n", strerror(errno)));
        CL_ASSERT(0);
        return CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }
    
    ret = fcntl(pTipcCommPort->fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);

    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = TIPC_ZONE_SCOPE;
    address.addr.name.domain=0;
    address.addr.name.name.instance = gIocLocalBladeAddress;

    
    rc = clTipcCheckAndGetPortId(&portId);
    if(rc != CL_OK)
        goto out_free;

    address.addr.name.name.type = CL_TIPC_SET_TYPE(portId);

    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(struct sockaddr_tipc)) < 0 )
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
        goto out_del;
    }


    CL_LIST_HEAD_INIT(&pTipcCommPort->logicalAddressList);
    CL_LIST_HEAD_INIT(&pTipcCommPort->multicastAddressList);
    pTipcCommPort->activeBind = CL_FALSE;
    pTipcCommPort->portId = portId;

    if(portId == CL_IOC_CPM_PORT)
    {
        /*Let CPM use high priority messaging*/
        priority = CL_IOC_TIPC_HIGH_PRIORITY;
    }
    if(!setsockopt(pTipcCommPort->fd,SOL_TIPC,TIPC_IMPORTANCE, (char *)&priority,sizeof(priority)))
    {
        pTipcCommPort->priority = priority;
    }
    else
    {
        int err = errno;
        if (err == ENOPROTOOPT)
        {
            tipcPriorityChangePossible = CL_FALSE;
            CL_DEBUG_PRINT(CL_DEBUG_WARN,("Message priority not available in this version of TIPC."));
        }            
        else CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in setting TIPC message priority. errno [%d]",err));

        pTipcCommPort->priority = CL_IOC_TIPC_DEFAULT_PRIORITY;
    }
        
    clOsalMutexLock(&gClTipcPortMutex);
    rc = clTipcPortHashAdd(pTipcCommPort);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Port hash add error.rc=0x%x\n",rc));
        goto out_put;
    }


    /* The following bind is for doing the intranode communicatoin */
    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = TIPC_NODE_SCOPE;
    address.addr.name.domain=0;
    address.addr.name.name.type = CL_IOC_TIPC_ADDRESS_TYPE_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress);
    address.addr.name.name.instance = CL_TIPC_SET_TYPE(portId);

    if(bind(pTipcCommPort->fd, (struct sockaddr *)&address, sizeof(struct sockaddr_tipc)) < 0)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : bind failed. errno = %d", errno));
        goto out_del;
    }
    
    {
        ClIocPhysicalAddressT myAddress;
        
        myAddress.nodeAddress = gIocLocalBladeAddress;
        myAddress.portId = pTipcCommPort->portId;
        rc = clIocCompStatusSet(myAddress, CL_IOC_NODE_UP);
        if (rc !=  CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to set component 0x%x status. error code 0x%x", pTipcCommPort->portId, rc));
            return rc;
        }
    }

#ifdef BCAST_SOCKET_NEEDED
    {
        ClInt32T ret;
        pTipcCommPort->bcastFd = socket(AF_TIPC, SOCK_RDM, 0);
        CL_ASSERT(pTipcCommPort->bcastFd >= 0);
        ret = fcntl(pTipcCommPort->bcastFd, F_SETFD, FD_CLOEXEC);
        CL_ASSERT(ret == 0);
    }
#endif

    *pIocCommPortHandle = (ClWordT)pTipcCommPort;

    clOsalMutexUnlock(&gClTipcPortMutex);
    
    rc = clOsalMutexInit(&pTipcCommPort->unblockMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&pTipcCommPort->unblockCond);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&pTipcCommPort->recvUnblockCond);
    CL_ASSERT(rc == CL_OK);

    clTransportListen(NULL, portId);

    goto out;

out_del:
    clTipcPortHashDel(pTipcCommPort);
out_put:
    clTipcPutPortId(portId);
out_free:
    close(pTipcCommPort->fd);
    clHeapFree(pTipcCommPort);

out:
    return rc;
}


ClRcT clIocCommPortDelete(ClIocCommPortHandleT portId)
{
    ClTipcCommPortT *pTipcCommPort  = (ClTipcCommPortT*)portId;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead= NULL;
    ClListHeadT *pNext = NULL;
    ClTipcCompT *pComp = NULL;
    NULL_CHECK(pTipcCommPort);

    clIocCommPortReceiverUnblock(portId);
    /*This would withdraw all the binds*/
    close(pTipcCommPort->fd);
#ifdef BCAST_SOCKET_NEEDED
    close(pTipcCommPort->bcastFd);
#endif
    clTipcPutPortId(pTipcCommPort->portId);

    clOsalMutexLock(&gClTipcPortMutex);
    clOsalMutexLock(&gClTipcCompMutex);
    if((pComp = pTipcCommPort->pComp))
    {
        pTipcCommPort->pComp = NULL;
        pHead = &pComp->portList;
        CL_LIST_FOR_EACH(pTemp,pHead)
        {
            ClTipcCommPortT *pCommPort = CL_LIST_ENTRY(pTemp,ClTipcCommPortT,listComp);
            if(pCommPort == pTipcCommPort)
            {
                clListDel(&pCommPort->listComp);
                break;
            }
        }
        /*Check if component can be unhashed and ripped off*/
        if(CL_LIST_HEAD_EMPTY(&pComp->portList))
        {
            clTipcCompHashDel(pComp);
            clHeapFree(pComp);
        }
    }
    pHead = &pTipcCommPort->logicalAddressList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClTipcLogicalAddressT *pLogicalAddress;
        pNext = pTemp->pNext;
        pLogicalAddress = CL_LIST_ENTRY(pTemp,ClTipcLogicalAddressT,list);
        clHeapFree(pLogicalAddress);
    }

    clOsalMutexLock(&gClTipcMcastMutex);
    pHead = &pTipcCommPort->multicastAddressList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClTipcMcastPortT *pMcastPort;
        pNext = pTemp->pNext;
        pMcastPort= CL_LIST_ENTRY(pTemp,ClTipcMcastPortT,listPort);
        CL_ASSERT(pMcastPort->pTipcCommPort == pTipcCommPort);
        clListDel(&pMcastPort->listMcast);
        clListDel(&pMcastPort->listPort);
        if(CL_LIST_HEAD_EMPTY(&pMcastPort->pMcast->portList))
        {
            clTipcMcastHashDel(pMcastPort->pMcast);
            clHeapFree(pMcastPort->pMcast);
        }
        clHeapFree(pMcastPort);
    }
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clTipcPortHashDel(pTipcCommPort);
    clOsalMutexUnlock(&gClTipcCompMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    clHeapFree(pTipcCommPort);
    return CL_OK;
}


ClRcT clIocCommPortFdGet(ClIocCommPortHandleT portHandle, ClInt32T *pFd)
{
    ClTipcCommPortT *pPortHandle = (ClTipcCommPortT*)portHandle;
    
    if(pPortHandle == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Invalid CommPort handle passed.\n"));
        return CL_IOC_RC(CL_ERR_INVALID_HANDLE);
    }

    if(pFd == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : NULL parameter passed for getting the file descriptor.\n"));
        return CL_IOC_RC(CL_ERR_NULL_POINTER);
    }

    *pFd = pPortHandle->fd;
    
    return CL_OK;
}


/*
 * Contruct the tipc address from the ioc address
 */
#ifdef BCAST_SOCKET_NEEDED
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress,
                       ClIocAddressT *pDestAddress,
                       ClUint32T *pSendFDFlag
                       )
#else
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress,
                       ClIocAddressT *pDestAddress
                       )
#endif
{
    ClInt32T type;
    bzero((char*)pAddress,sizeof(*pAddress));
    pAddress->family = AF_TIPC;
    pAddress->addrtype = TIPC_ADDR_NAME;
    pAddress->scope = TIPC_ZONE_SCOPE;
    type = CL_IOC_ADDRESS_TYPE_GET(pDestAddress);
#ifdef BCAST_SOCKET_NEEDED
    *pSendFDFlag = 1;
#endif

    switch(type)
    {
    case CL_IOC_PHYSICAL_ADDRESS_TYPE:
        if(pDestAddress->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
        {
            goto set_broadcast;
        }
        pAddress->addr.name.name.type = CL_TIPC_SET_TYPE(pDestAddress->iocPhyAddress.portId);
        pAddress->addr.name.name.instance = pDestAddress->iocPhyAddress.nodeAddress;
        pAddress->addr.name.domain=0;
        break;
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        pAddress->addr.name.name.type = CL_IOC_TIPC_TYPE(pDestAddress->iocLogicalAddress);
        pAddress->addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pDestAddress->iocLogicalAddress);
        pAddress->addr.name.domain=0;
        break;
    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_IOC_TIPC_TYPE(pDestAddress->iocMulticastAddress);
        pAddress->addr.nameseq.lower = CL_IOC_TIPC_INSTANCE(pDestAddress->iocMulticastAddress);  
        pAddress->addr.nameseq.upper = CL_IOC_TIPC_INSTANCE(pDestAddress->iocMulticastAddress);
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    case CL_IOC_BROADCAST_ADDRESS_TYPE:
    set_broadcast:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_TIPC_SET_TYPE(pDestAddress->iocPhyAddress.portId);
        pAddress->addr.nameseq.lower = CL_IOC_MIN_NODE_ADDRESS;
        pAddress->addr.nameseq.upper = CL_IOC_MAX_NODE_ADDRESS;
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    case CL_IOC_INTRANODE_ADDRESS_TYPE:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_IOC_TIPC_TYPE(pDestAddress->iocLogicalAddress);
        pAddress->addr.nameseq.lower = CL_TIPC_SET_TYPE(CL_IOC_MIN_COMP_PORT);
        pAddress->addr.nameseq.upper = CL_TIPC_SET_TYPE(CL_IOC_MAX_COMP_PORT);
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    default:
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }
    return CL_OK;
}

#ifdef CL_TIPC_COMPRESSION

static ClRcT doDecompress(ClUint8T *compressedStream, ClUint32T compressedStreamLen,
                          ClUint8T **ppDecompressedStream, ClUint32T *pDecompressedStreamLen)
{
    z_stream stream;
    ClUint8T *decompressedStream = NULL;
    ClUint32T decompressedStreams = 0;
    ClInt32T err = 0;
    ClTimeT t1, t2;
    ClUint32T maxPayloadSize = clIocInternalMaxPayloadSizeGet();
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
    } while(1);

    err = inflateEnd(&stream);
    if(err != Z_OK)
    {
        clLogError("IOC", "DECOMPRESS", "Inflate end returned [%d]", err);
        goto out_free;
    }
    ZLIB_TIME(t2);
    *ppDecompressedStream = decompressedStream;
    *pDecompressedStreamLen = stream.total_out;
    clLogNotice("IOC", "DECOMPRESS", "Inflated [%ld] bytes from [%d] bytes, [%d] decompressed streams, " \
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
            printf("%#x ",  ((ClUint32T*)decompressedStream)[i]);
        }
    } while(0);
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

    stream.zalloc =  Z_NULL;
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
    *pCompressedStreamLen =  stream.total_out;
    clLogNotice("IOC", "COMPRESS", "Pending avail bytes in the output stream [%d], uncompress len [%d], "\
                "compressed len [%ld], " \
                "Compression time [%lld] usecs", 
                stream.avail_out, uncompressedStreamLen, stream.total_out, t2-t1);

    err = 0;
    return CL_OK;

    out_free:
    clHeapFree(compressedStream);
    return CL_IOC_RC(CL_ERR_LIBRARY);
}
#endif

typedef struct IOVecIterator
{
    struct iovec *src;
    struct iovec *target;
    ClUint32T srcVectors;
    ClUint32T maxTargetVectors;
    struct iovec *header;
    struct iovec *curIOVec;
    ClOffsetT offset; /* linear offset within the cur iovec*/
} IOVecIteratorT;

static ClRcT iovecIteratorInit(IOVecIteratorT *iter, 
                               struct iovec *src, ClUint32T srcVectors, 
                               struct iovec *header)
{
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(!srcVectors) 
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
    if(header)
    {
        iter->target[0].iov_base = header->iov_base;
        iter->target[0].iov_len =  header->iov_len;
    }
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT iovecIteratorNext(IOVecIteratorT *iter, ClUint32T *payload, struct iovec **target, ClUint32T *targetVectors)
{
    ClUint32T numVectors = 0;
    size_t size;
    struct iovec *curIOVec;
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(!payload)
        goto out;
    rc = CL_ERR_NO_SPACE;
    if(!(curIOVec = iter->curIOVec))
        goto out;
    size = (size_t)*payload;

    if(iter->header)
        numVectors = 1;

    while(size > 0 && curIOVec)
    {
        size_t len = curIOVec->iov_len - iter->offset;
        ClUint8T *base = curIOVec->iov_base + iter->offset;
        while(size > 0 && len > 0)
        {
            size_t tgtLen = CL_MIN(size, len);
            if(numVectors >= iter->maxTargetVectors)
            {
                iter->target = clHeapRealloc(iter->target, sizeof(*iter->target) * (numVectors + 16));
                CL_ASSERT(iter->target != NULL);
                memset(iter->target + numVectors, 0, sizeof(*iter->target) * 16);
                iter->maxTargetVectors += 16;
            }
            iter->target[numVectors].iov_base = base;
            iter->target[numVectors].iov_len = tgtLen;
            ++numVectors;
            size -= tgtLen;
            len -= tgtLen;
            if(!len)
            {
                iter->offset = 0;
                ++curIOVec;
                if( (curIOVec - iter->src) >= iter->srcVectors )
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
    if(size > 0)
        *payload -= size;
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT iovecIteratorExit(IOVecIteratorT *iter)
{
    if(iter->target) 
    {
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

ClRcT clIocSend(ClIocCommPortHandleT commPortHandle,
                ClBufferHandleT message, ClUint8T protoType,
                ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption)
{
    ClRcT retCode, rc;
    ClTipcCommPortT *pTipcCommPort = (ClTipcCommPortT *)commPortHandle;
    ClUint32T timeout = 0;
    ClUint8T priority = 0;
    ClUint32T msgLength;
    ClUint32T maxPayload, fragId, bytesRead = 0;
    ClUint32T totalFragRequired, fraction;
    ClUint32T addrType;
    ClBoolT isBcast = CL_FALSE;
    ClIocAddressT *replicastList = NULL;
    ClUint32T numReplicasts = 0;
#ifdef CL_TIPC_COMPRESSION
    ClUint8T *decompressedStream = NULL;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = 0;
    ClTimeT pktTime = 0;
    ClTimeValT tv = {0};
#endif
    if (!pTipcCommPort || !destAddress)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    addrType = CL_IOC_ADDRESS_TYPE_GET(destAddress);
    isBcast =  (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE && 
                ( (ClIocPhysicalAddressT*)destAddress)->nodeAddress == CL_IOC_BROADCAST_ADDRESS);
    if(isBcast && gClTipcReplicast)
    {
        clIocReplicastGet( ((ClIocPhysicalAddressT*)destAddress)->portId, &replicastList, &numReplicasts);
    }
    if(addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE
       &&
       !isBcast)
    {
        ClIocNodeAddressT node;
        ClUint8T status;

        node = ((ClIocPhysicalAddressT *)destAddress)->nodeAddress;

        if (CL_IOC_RESERVED_ADDRESS == node) {
            clDbgCodeError(CL_IOC_RC(CL_ERR_INVALID_PARAMETER),("Error : Invalid destination address %x:%x is passed.", 
                                                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                                                ((ClIocPhysicalAddressT *)destAddress)->portId));            
            return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
        }
    
        retCode = clIocCompStatusGet(*(ClIocPhysicalAddressT *)destAddress, &status);
        if (retCode !=  CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to get the status of the component. error code 0x%x", retCode));
            return retCode;
        }
        if(0 && status == CL_IOC_NODE_DOWN)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                           ("Port [0x%x] is trying to reach component [0x%x:0x%x] "
                            "but the component is not reachable.",
                            pTipcCommPort->portId,
                            ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                            ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
        }
            
    }

    if (pSendOption)
    {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_TIPC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("Failed to get the lenght of the messege. error code 0x%x",retCode));
        retCode = CL_IOC_RC(CL_ERR_INVALID_BUFFER);
        goto out_free;
    }
    
    maxPayload = clIocInternalMaxPayloadSizeGet();

#ifdef CL_TIPC_COMPRESSION
    if(protoType != CL_IOC_PORT_NOTIFICATION_PROTO 
       &&
       protoType != CL_IOC_PROTO_CTL
       &&
       msgLength >= gTipcCompressionBoundary)
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

    if (msgLength > maxPayload)
    {
        /*
         * Fragment it to 64 K size and return
         */
        ClTipcFragHeaderT userFragHeader = {{0}};
        struct iovec  header = { .iov_base = (void*)&userFragHeader, .iov_len = sizeof(userFragHeader) };
        struct iovec *target = NULL;
        IOVecIteratorT iovecIterator = {0};
        struct iovec *src= NULL;
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
        
        clOsalMutexLock(ClIocFragMutex);
        fragId = currFragId;
        ++currFragId;
        clOsalMutexUnlock(ClIocFragMutex);

        userFragHeader.header.version = CL_IOC_HEADER_VERSION;
        userFragHeader.header.protocolType = protoType;
        userFragHeader.header.priority = priority;
        userFragHeader.header.flag = IOC_MORE_FRAG;
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = htonl(pTipcCommPort->portId);
        userFragHeader.header.reserved = 0;
#ifdef CL_TIPC_COMPRESSION
        userFragHeader.header.pktTime = clHtonl64(pktTime);
#endif
        userFragHeader.msgId = htonl(fragId);
        userFragHeader.fragOffset = 0;
        userFragHeader.fragLength = htonl(maxPayload);

        while (totalFragRequired > 1)
        {
            ClUint32T payload = maxPayload;
            retCode = iovecIteratorNext(&iovecIterator, &payload, &target, &targetVectors);
            CL_ASSERT(retCode == CL_OK);
            CL_ASSERT(payload == maxPayload);
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                           ("Sending id %d flag 0x%x length %d offset %d\n",
                            ntohl(userFragHeader.msgId), userFragHeader.header.flag,
                            ntohl(userFragHeader.fragLength),
                            ntohl(userFragHeader.fragOffset)));
            /*
             *clLogNotice("FRAG", "SEND", "Sending [%d] bytes with [%d] vectors representing [%d] bytes", 
             msgLength, targetVectors, maxPayload);
            */                                                          
            if(replicastList)
            {
                retCode = internalSendReplicast(pTipcCommPort, target, targetVectors, payload + sizeof(userFragHeader),
                                                priority, &timeout, replicastList, numReplicasts);
            }
            else
            {
                retCode = internalSend(pTipcCommPort, target, targetVectors, payload + sizeof(userFragHeader), 
                                       priority, destAddress, &timeout);
            }
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Failed to send the message. error code = 0x%x\n", retCode));
                goto frag_error;
            }

            bytesRead += payload;
            userFragHeader.fragOffset = htonl(bytesRead);   /* updating for the
                                                             * next packet */
            --totalFragRequired;

#ifdef CL_TIPC_COMPRESSION            
            if(userFragHeader.header.pktTime)
                userFragHeader.header.pktTime = 0;
#endif
            ++frags;
            if(0 && !(frags & 511))
                sleep(1);
        }                       /* while */
        /*
         * sending the last fragment
         */
        if (fraction)
        {
            maxPayload = fraction;
            userFragHeader.fragLength = htonl(maxPayload);
        }
        else fraction = maxPayload;
        userFragHeader.header.flag = IOC_LAST_FRAG;
        retCode = iovecIteratorNext(&iovecIterator, &fraction, &target, &targetVectors);
        CL_ASSERT(retCode == CL_OK);
        CL_ASSERT(fraction == maxPayload);
        clLogTrace("FRAG", "SEND", "Sending last frag at offset [%d], length [%d]",
                   ntohl(userFragHeader.fragOffset), fraction);
        if(replicastList)
        {
            retCode = internalSendReplicast(pTipcCommPort, target, targetVectors, fraction + sizeof(userFragHeader), 
                                            priority, &timeout, replicastList, numReplicasts);
        }
        else
        {
            retCode = internalSend(pTipcCommPort, target, targetVectors, fraction + sizeof(userFragHeader), 
                                   priority, destAddress, &timeout);
        }

        frag_error:
        {
            ClRcT rc;
            rc = iovecIteratorExit(&iovecIterator);
            CL_ASSERT(rc == CL_OK);
            if(src) clHeapFree(src);
        }
    }
    else
    {
        ClTipcHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = htonl(pTipcCommPort->portId);
        userHeader.reserved = 0;

#ifdef CL_TIPC_COMPRESSION
        if(compressedStreamLen)
            userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif

        retCode =
            clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                                sizeof(ClTipcHeaderT));
        if(retCode != CL_OK)	
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            goto out_free;
        }
        
        if(replicastList)
        {
            retCode = internalSendSlowReplicast(pTipcCommPort, message, priority, &timeout, 
                                                replicastList, numReplicasts);
        }
        else
        {
            retCode = internalSendSlow(pTipcCommPort, message, priority, destAddress, &timeout);
        }

        rc = clBufferHeaderTrim(message, sizeof(ClTipcHeaderT));
        if(rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR: Buffer header trim failed RC = 0x%x\n", rc));

#ifdef CL_TIPC_COMPRESSION
        if(compressedStreamLen)
        {
            /*
             * delete the compressed message stream.
             */
            clBufferDelete(&message);
        }
#endif

    }

    out_free:
    if(replicastList)
        clHeapFree(replicastList);

    return retCode;
}

ClRcT clIocSendSlow(ClIocCommPortHandleT commPortHandle,
                    ClBufferHandleT message, ClUint8T protoType,
                    ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption)
{
    ClRcT retCode, rc;
    ClTipcCommPortT *pTipcCommPort = (ClTipcCommPortT *)commPortHandle;
    ClUint32T timeout = 0;
    ClUint8T priority = 0;
    ClUint32T msgLength;
    ClUint32T maxPayload, fragId, bytesRead = 0;
    ClUint32T totalFragRequired, fraction;
    ClBufferHandleT tempMsg;
#ifdef CL_TIPC_COMPRESSION
    ClUint8T *decompressedStream = NULL;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = 0;
    ClTimeT pktTime = 0;
    ClTimeValT tv = {0};
#endif
    if (!pTipcCommPort || !destAddress)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(CL_IOC_ADDRESS_TYPE_GET(destAddress) == CL_IOC_PHYSICAL_ADDRESS_TYPE &&
       ((ClIocPhysicalAddressT *)destAddress)->nodeAddress != CL_IOC_BROADCAST_ADDRESS)
    {
        ClIocNodeAddressT node;
        ClUint8T status;

        node = ((ClIocPhysicalAddressT *)destAddress)->nodeAddress;

        if (CL_IOC_RESERVED_ADDRESS == node) {
            clDbgCodeError(CL_IOC_RC(CL_ERR_INVALID_PARAMETER),("Error : Invalid destination address %x:%x is passed.", 
                                                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                                                ((ClIocPhysicalAddressT *)destAddress)->portId));            
            return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
        }
    
        retCode = clIocCompStatusGet(*(ClIocPhysicalAddressT *)destAddress, &status);
        if (retCode !=  CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to get the status of the component. error code 0x%x", retCode));
            return retCode;
        }
        if(status == CL_IOC_NODE_DOWN)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                           ("Port [0x%x] is trying to reach component [0x%x:0x%x] "
                            "but the component is not reachable.",
                            pTipcCommPort->portId,
                            ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                            ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
        }
            
    }

    if (pSendOption)
    {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_TIPC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("Failed to get the lenght of the messege. error code 0x%x",retCode));
        return CL_IOC_RC(CL_ERR_INVALID_BUFFER);
    }
    
    maxPayload = clIocInternalMaxPayloadSizeGet();

#ifdef CL_TIPC_COMPRESSION
    if(protoType != CL_IOC_PORT_NOTIFICATION_PROTO 
       &&
       protoType != CL_IOC_PROTO_CTL
       &&
       msgLength >= gTipcCompressionBoundary)
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

    if (msgLength > maxPayload)
    {
        /*
         * Fragment it to 64 K size and return
         */
        ClTipcFragHeaderT userFragHeader = {{0}};
        ClUint32T frags = 0;
        retCode = clBufferCreate(&tempMsg);
        if (retCode != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR : Message creation failed. errorCode = 0x%x\n", retCode));
            return retCode;
        }
        
        totalFragRequired = msgLength / maxPayload;
        fraction = msgLength % maxPayload;
        if (fraction)
            totalFragRequired = totalFragRequired + 1;
        
        clOsalMutexLock(ClIocFragMutex);
        fragId = currFragId;
        currFragId++;
        clOsalMutexUnlock(ClIocFragMutex);

        userFragHeader.header.version = CL_IOC_HEADER_VERSION;
        userFragHeader.header.protocolType = protoType;
        userFragHeader.header.priority = priority;
        userFragHeader.header.flag = IOC_MORE_FRAG;
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = htonl(pTipcCommPort->portId);
        userFragHeader.header.reserved = 0;
#ifdef CL_TIPC_COMPRESSION
        userFragHeader.header.pktTime = clHtonl64(pktTime);
#endif
        userFragHeader.msgId = htonl(fragId);
        userFragHeader.fragOffset = 0;
        userFragHeader.fragLength = htonl(maxPayload);

        while (totalFragRequired > 1)
        {
            retCode =
                clBufferToBufferCopy(message, bytesRead, tempMsg,
                                     maxPayload);
            if (retCode != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("\nERROR: message to message copy failed. errorCode = 0x%x\n", retCode));
                goto frag_error;
            }

            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                           ("Sending id %d flag 0x%x length %d offset %d\n",
                            ntohl(userFragHeader.msgId), userFragHeader.header.flag,
                            ntohl(userFragHeader.fragLength),
                            ntohl(userFragHeader.fragOffset)));

            retCode =
                clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                                    sizeof(ClTipcFragHeaderT));
            if(retCode != CL_OK)	
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("\nERROR : Prepend buffer data failed. errorCode = 0x%x\n", retCode));
                goto frag_error;
            }

            retCode = internalSendSlow(pTipcCommPort, tempMsg, priority, destAddress, &timeout);
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Failed to send the message. error code = 0x%x\n", retCode));
                goto frag_error;
            }

            bytesRead = bytesRead + maxPayload;
            userFragHeader.fragOffset = htonl(bytesRead);   /* updating for the
                                                             * next packet */
            retCode = clBufferClear(tempMsg);
            if(retCode != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nERROR : Failed clear the buffer. errorCode = 0x%x\n",retCode));

            totalFragRequired--;
#ifdef CL_TIPC_COMPRESSION            
            if(userFragHeader.header.pktTime)
                userFragHeader.header.pktTime = 0;
#endif
            ++frags;
            if(!(frags & 511))
                sleep(1);
            /*usleep(CL_IOC_MICRO_SLEEP_INTERVAL);*/
        }                       /* while */
        /*
         * sending the last fragment
         */
        if (fraction)
        {
            maxPayload = fraction;
            userFragHeader.fragLength = htonl(maxPayload);
        }
        userFragHeader.header.flag = IOC_LAST_FRAG;

        retCode =
            clBufferToBufferCopy(message, bytesRead, tempMsg,
                                 maxPayload);
        if (retCode != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Error : message to message copy failed. rc = 0x%x\n", retCode));
            goto frag_error;
        }
        retCode =
            clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                                sizeof(ClTipcFragHeaderT));
        if(retCode != CL_OK)	
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            goto frag_error;
        }
        clLogTrace("FRAG", "SEND2", "Sending last frag at offset [%d], length [%d]",
                   ntohl(userFragHeader.fragOffset), maxPayload);
        retCode = internalSendSlow(pTipcCommPort, tempMsg, priority, destAddress, &timeout);

        frag_error:
        {
            ClRcT rc;
            rc = clBufferDelete(&tempMsg);
            CL_ASSERT(rc == CL_OK);
        }
    }
    else
    {
        ClTipcHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = htonl(pTipcCommPort->portId);
        userHeader.reserved = 0;

#ifdef CL_TIPC_COMPRESSION
        if(compressedStreamLen)
            userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif

        retCode =
            clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                                sizeof(ClTipcHeaderT));
        if(retCode != CL_OK)	
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR: Prepend buffer data failed = 0x%x\n", retCode));
            return retCode;
        }

        retCode = internalSendSlow(pTipcCommPort, message, priority, destAddress, &timeout);

        rc = clBufferHeaderTrim(message, sizeof(ClTipcHeaderT));
        if(rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\nERROR: Buffer header trim failed RC = 0x%x\n", rc));

#ifdef CL_TIPC_COMPRESSION
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

static ClRcT internalSend(ClTipcCommPortT *pTipcCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout)
{
    ClRcT rc;
    struct sockaddr_tipc tipcAddress;
    ClInt32T fd ;
    ClInt32T bytes;
    struct msghdr msgHdr;
    ClUint32T priority;
    ClInt32T tries = 0;
    static ClInt32T recordIOCSend = -1;

#ifdef BCAST_SOCKET_NEEDED
    ClUint32T sendFDFlag = 1;
#endif

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(pTipcCommPort == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid port id\n"));
        goto out;
    }

    /*map the address to a TIPC address before sending*/
#ifdef BCAST_SOCKET_NEEDED
    rc = clTipcGetAddress(&tipcAddress,pIocAddress, &sendFDFlag);
#else
    rc = clTipcGetAddress(&tipcAddress,pIocAddress);
#endif
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in tipc get address.rc=0x%x\n",rc));
        goto out;
    }

    fd = pTipcCommPort->fd;

#ifdef BCAST_SOCKET_NEEDED
    if(sendFDFlag == 2)
        fd = pTipcCommPort->bcastFd;
#endif

    priority = CL_IOC_TIPC_DEFAULT_PRIORITY;
    if(pTipcCommPort->portId == CL_IOC_CPM_PORT  ||
       pTipcCommPort->portId == CL_IOC_XPORT_PORT ||
       tempPriority == CL_IOC_HIGH_PRIORITY)
    {
        priority = CL_IOC_TIPC_HIGH_PRIORITY;
    }
    if ((tipcPriorityChangePossible) && (pTipcCommPort->priority != priority))
    {
        if(!setsockopt(fd,SOL_TIPC,TIPC_IMPORTANCE,(char *)&priority,sizeof(priority)))
        {
            pTipcCommPort->priority = priority;
        }
        else
        {
            int err = errno;

            if (err == ENOPROTOOPT)
            {
                tipcPriorityChangePossible = CL_FALSE;
                CL_DEBUG_PRINT(CL_DEBUG_WARN,("Message priority not available in this version of TIPC."));
                tipcPriorityChangePossible = CL_FALSE;
            }            
            else CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in setting TIPC message priority. errno [%d]",err));
        }
    }
    bzero((char*)&msgHdr,sizeof(msgHdr));
    msgHdr.msg_name = (ClPtrT)&tipcAddress;
    msgHdr.msg_namelen = sizeof(tipcAddress);
    msgHdr.msg_iov = target;
    msgHdr.msg_iovlen = targetVectors;
    /*
     * If the leaky bucket is defined, then 
     * respect the traffic signal
     */
    if(gClIocTrafficShaper && gClLeakyBucket)
    {
        clLeakyBucketFill(gClLeakyBucket, messageLen);
    }

    if(recordIOCSend < 0)
    {
        ClBoolT record = clParseEnvBoolean("CL_ASP_RECORD_IOC_SEND");
        recordIOCSend = record ? 1 : 0;
    }
    
    retry:

    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_TRUE);

    /*    bytes = sendmsg(fd,&msgHdr,0);*/
    bytes = messageLen;
    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_FALSE);
    
    clTransportSend(NULL, pTipcCommPort->portId, priority, pIocAddress, target, targetVectors, 0);

    if(bytes <= 0 )
    {
        if(errno == EINTR)
            goto retry;
     
        if(errno == EAGAIN)
        {
            if(++tries < 10)
            {
                usleep(100);
                goto retry;
            }
        }
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed at sendmsg. errno = %d\n",errno));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }

    out:
    return rc;
}

static ClRcT internalSendReplicast(ClTipcCommPortT *pTipcCommPort,
                                   struct iovec *target,
                                   ClUint32T targetVectors,
                                   ClUint32T messageLen,
                                   ClUint32T tempPriority, 
                                   ClUint32T *pTimeout,
                                   ClIocAddressT *replicastList,
                                   ClUint32T numReplicasts)
{
    ClRcT rc;
    ClUint32T i;
    ClUint32T success = 0;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    if(pTipcCommPort == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid port id\n"));
        goto out;
    }

    for(i = 0; i < numReplicasts; ++i)
    {
        rc = internalSend(pTipcCommPort, target, targetVectors, messageLen, tempPriority,
                          &replicastList[i], pTimeout);
        if(rc != CL_OK)
        {
            clLogError("TIPC", "REPLICAST", "Replicast to destination [%d: %#x] failed with [%#x]",
                       replicastList[i].iocPhyAddress.nodeAddress,
                       replicastList[i].iocPhyAddress.portId, rc);
        }
        else
        {
            ++success;
        }
    }        
    if(success > 0)
        rc = CL_OK;

    out:
    return rc;
}

static ClRcT internalSendSlow(ClTipcCommPortT *pTipcCommPort,
                              ClBufferHandleT message, 
                              ClUint32T tempPriority, 
                              ClIocAddressT *pIocAddress,
                              ClUint32T *pTimeout)
{
    ClRcT rc;
    struct sockaddr_tipc tipcAddress;
    ClInt32T fd ;
    ClInt32T bytes;
    struct iovec *pIOVector = NULL;
    ClInt32T ioVectorLen = 0;
    struct msghdr msgHdr;
    ClUint32T priority;
    ClInt32T tries = 0;
    static ClInt32T recordIOCSend = -1;

#ifdef BCAST_SOCKET_NEEDED
    ClUint32T sendFDFlag = 1;
#endif

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(pTipcCommPort == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid port id\n"));
        goto out;
    }

    /*map the address to a TIPC address before sending*/
#ifdef BCAST_SOCKET_NEEDED
    rc = clTipcGetAddress(&tipcAddress,pIocAddress, &sendFDFlag);
#else
    rc = clTipcGetAddress(&tipcAddress,pIocAddress);
#endif
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in tipc get address.rc=0x%x\n",rc));
        goto out;
    }

    fd = pTipcCommPort->fd;

#ifdef BCAST_SOCKET_NEEDED
    if(sendFDFlag == 2)
        fd = pTipcCommPort->bcastFd;
#endif

    rc = clBufferVectorize(message,&pIOVector,&ioVectorLen);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in buffer vectorize.rc=0x%x\n",rc));
        goto out;
    }
    priority = CL_IOC_TIPC_DEFAULT_PRIORITY;
    if(pTipcCommPort->portId == CL_IOC_CPM_PORT  ||
       pTipcCommPort->portId == CL_IOC_XPORT_PORT ||
       tempPriority == CL_IOC_HIGH_PRIORITY)
    {
        priority = CL_IOC_TIPC_HIGH_PRIORITY;
    }
    if ((tipcPriorityChangePossible) && (pTipcCommPort->priority != priority))
    {
        if(!setsockopt(fd,SOL_TIPC,TIPC_IMPORTANCE,(char *)&priority,sizeof(priority)))
        {
            pTipcCommPort->priority = priority;
        }
        else
        {
            int err = errno;

            if (err == ENOPROTOOPT)
            {
                tipcPriorityChangePossible = CL_FALSE;
                CL_DEBUG_PRINT(CL_DEBUG_WARN,("Message priority not available in this version of TIPC."));
                tipcPriorityChangePossible = CL_FALSE;
            }            
            else CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in setting TIPC message priority. errno [%d]",err));
        }
    }
    bzero((char*)&msgHdr,sizeof(msgHdr));
    msgHdr.msg_name = (ClPtrT)&tipcAddress;
    msgHdr.msg_namelen = sizeof(tipcAddress);
    msgHdr.msg_iov = pIOVector;
    msgHdr.msg_iovlen = ioVectorLen;
    /*
     * If the leaky bucket is defined, then 
     * respect the traffic signal
     */
    if(gClLeakyBucket && gClTipcPacketSize <= 1500)
    {
        ClUint32T len = 0;
        clBufferLengthGet(message, &len);
        clLeakyBucketFill(gClLeakyBucket, len);
    }

    if(recordIOCSend < 0)
    {
        ClBoolT record = clParseEnvBoolean("CL_ASP_RECORD_IOC_SEND");
        recordIOCSend = record ? 1 : 0;
    }
    
    retry:

    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_TRUE);

    /*    bytes = sendmsg(fd,&msgHdr,0);*/
    bytes = 10;
    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_FALSE);

    clTransportSend(NULL, pTipcCommPort->portId, priority, pIocAddress, pIOVector, ioVectorLen, 0);

    if(bytes <= 0 )
    {
        if(errno == EINTR)
            goto retry;
     
        if(errno == EAGAIN)
        {
            if(++tries < 10)
            {
                usleep(100);
                goto retry;
            }
        }
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed at sendmsg. errno = %d\n",errno));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }
    clHeapFree(pIOVector);
    out:
    return rc;
}

static ClRcT internalSendSlowReplicast(ClTipcCommPortT *pTipcCommPort,
                                       ClBufferHandleT message, 
                                       ClUint32T tempPriority, 
                                       ClUint32T *pTimeout,
                                       ClIocAddressT *replicastList,
                                       ClUint32T numReplicasts)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    ClUint32T i;
    ClUint32T success = 0;
    if(!pTipcCommPort)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid port\n"));
        goto out;
    }
    for(i = 0; i < numReplicasts; ++i)
    {
        rc = internalSendSlow(pTipcCommPort, message, tempPriority, 
                              &replicastList[i], pTimeout);
        if(rc != CL_OK)
        {
            clLogError("TIPC", "REPLICAST", "Slow send to destination [%d: %#x] failed with [%#x]",
                       replicastList[i].iocPhyAddress.nodeAddress,
                       replicastList[i].iocPhyAddress.portId, rc);
        }
        else
        {
            ++success;
        }
    }
    if(success > 0)
        rc = CL_OK;

    out:
    return rc;
}

static ClRcT __clIocReceive(ClIocCommPortHandleT commPortHdl,
                            ClIocRecvOptionT *pRecvOption,
                            ClUint8T *buffer, ClUint32T bufSize,
                            ClBufferHandleT message, ClIocRecvParamT *pRecvParam,
                            ClBoolT sync)
{
    ClUint32T timeout;
    ClRcT rc = CL_OK;
    ClUint32T lTimeout;
    ClTipcHeaderT userHeader = { 0 };
    ClUint32T size = sizeof(ClTipcHeaderT);
    ClTimeT tml1 = 0;
    ClTipcCommPortT *pTipcCommPort = (ClTipcCommPortT*)commPortHdl;
    struct pollfd pollfds[1];
    ClInt32T fd[1] ;
    ClUint8T *pBuffer = NULL;
    struct msghdr msgHdr;
    struct sockaddr_tipc peerAddress;
    struct iovec ioVector[1];
    ClInt32T bytes;
    ClInt32T pollStatus;
#ifdef CL_TIPC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    if (pRecvParam == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    if (pRecvOption)
        timeout = pRecvOption->recvTimeout;
    else
        timeout = CL_IOC_TIMEOUT_FOREVER;

    if(pTipcCommPort == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid tipc commport\n"));
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    bzero((char*)pollfds,sizeof(pollfds));
    pollfds[0].fd = fd[0] = pTipcCommPort->fd;
    
    bzero((char*)&msgHdr,sizeof(msgHdr));
    bzero((char*)ioVector,sizeof(ioVector));
    msgHdr.msg_name = (struct sockaddr_tipc*)&peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector[0].iov_base = (ClPtrT)buffer;
    ioVector[0].iov_len = bufSize;
    msgHdr.msg_iov = ioVector;
    msgHdr.msg_iovlen = sizeof(ioVector)/sizeof(ioVector[0]);

        
    /*
     * need to check returned payload should not be greater than asked
     */
    retry:
    rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    tml1 = clOsalStopWatchTimeGet();
    for(;;)
    {
        pollfds[0].events = POLLIN|POLLRDNORM;
        pollfds[0].revents = 0;
        pollStatus = poll(pollfds,1,timeout);
        if(pollStatus > 0) {
            if((pollfds[0].revents & (POLLIN|POLLRDNORM)))
            {
                recv:
                bytes = recvmsg(fd[0],&msgHdr,0);
                if(bytes < 0 )
                {
                    if(errno == EINTR)
                        goto recv;
                    perror("Receive : ");
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("recv error. errno = %d\n",errno));
                    goto out;
                }
            } else {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("poll error. errno = %d\n", errno));
                goto out;
            }
        } else if(pollStatus < 0 ) {
            if(errno == EINTR)
                continue;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in poll. errno = %d\n",errno));
            goto out;
        } else {
            rc = CL_IOC_RC(CL_ERR_TIMEOUT);
            goto out;
        }
        break;
    }
    
    if(bytes <= size)
    {
        /*Check for port exit message*/
        if(bytes == sizeof(CL_TIPC_PORT_EXIT_MESSAGE)
           && 
           !strncmp((ClCharT*)buffer,CL_TIPC_PORT_EXIT_MESSAGE,sizeof(CL_TIPC_PORT_EXIT_MESSAGE))
           )
        {
            ClTimerTimeOutT waitTime = {.tsSec=0,.tsMilliSec=200};
            CL_DEBUG_PRINT(CL_DEBUG_INFO,("PORT EXIT MESSAGE received for portid:0x%x,EO [%s]\n",pTipcCommPort->portId,CL_EO_NAME));
            rc = CL_IOC_RC(CL_IOC_ERR_RECV_UNBLOCKED);
            clOsalMutexLock(&pTipcCommPort->unblockMutex);
            if(!pTipcCommPort->blocked)
            {
                clOsalMutexUnlock(&pTipcCommPort->unblockMutex);
                goto out;
            }
            clOsalCondSignal(&pTipcCommPort->unblockCond);
            clOsalCondWait(&pTipcCommPort->recvUnblockCond,&pTipcCommPort->unblockMutex,waitTime);
            clOsalMutexUnlock(&pTipcCommPort->unblockMutex);
            goto out;
        }
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
                                           "The packet is an invalid or a corrupted one. "
                                           "Packet size if %d, rc = %x\n", bytes, rc));
        goto out;
    }


    memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClTipcHeaderT));

    if(userHeader.version != CL_IOC_HEADER_VERSION)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Dropping received packet of version [%d]. Supported version [%d]\n",
                                        userHeader.version, CL_IOC_HEADER_VERSION));
        goto retry;
    }

    userHeader.srcAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
    userHeader.srcAddress.iocPhyAddress.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);

    if(clEoWithOutCpm
       ||
       userHeader.srcAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress)
    {
        if( (rc = clIocCompStatusSet(userHeader.srcAddress.iocPhyAddress, 
                                     CL_IOC_NODE_UP)) != CL_OK)

        {
            ClUint32T packetSize;

            packetSize = bytes - ((userHeader.flag == 0)? sizeof(ClTipcHeaderT): sizeof(ClTipcFragHeaderT));

            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet."
                                               "Failed to SET the staus of the packet-sender-component "
                                               "[node 0x%x : port 0x%x]. Packet size is %d. error code 0x%x ",
                                               userHeader.srcAddress.iocPhyAddress.nodeAddress,
                                               userHeader.srcAddress.iocPhyAddress.portId, 
                                               packetSize, rc));
            goto retry;
        }
    }

    if(userHeader.flag == 0)
    {
#ifdef CL_TIPC_COMPRESSION
        ClTimeT pktRecvTime = 0;
        ClUint32T compressionFlag = ntohl(userHeader.reserved);
        ClUint8T *decompressedStream = NULL;
        ClUint32T decompressedStreamLen = 0;
        ClUint32T sentBytes = 0;
#endif
        pBuffer = buffer + sizeof(ClTipcHeaderT);
        bytes -= sizeof(ClTipcHeaderT);

#ifdef CL_TIPC_COMPRESSION
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
        if(clBufferAppendHeap(message, buffer, bytes + sizeof(ClTipcHeaderT)) != CL_OK)
        {
            rc = clBufferNBytesWrite(message, pBuffer, bytes);
        }
        else
        {
            /*
             * Trim the header if the recv buffer is stitched.
             */
            rc = clBufferHeaderTrim(message, sizeof(ClTipcHeaderT));
        }
#endif

        if(rc != CL_OK) 
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
                                               "Failed to write to a buffer message. "
                                               "Packet Size is %d. error code 0x%x", bytes, rc));
            goto out;
        }

#ifdef CL_TIPC_COMPRESSION
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

        /* Hoping that the notification packet will not exceed 64K packet size :-). */
        if(pTipcCommPort->notify == CL_IOC_NOTIFICATION_DISABLE &&
                userHeader.protocolType == CL_IOC_PORT_NOTIFICATION_PROTO)
        {
            clBufferClear(message);
            goto retry;
        }
    } 
    else 
    {
        ClTipcFragHeaderT userFragHeader;
        
        memcpy((ClPtrT)&userFragHeader,(ClPtrT)buffer, sizeof(ClTipcFragHeaderT));

        pBuffer = buffer + sizeof(ClTipcFragHeaderT);
        bytes -= sizeof(ClTipcFragHeaderT);
               

        userFragHeader.msgId = ntohl(userFragHeader.msgId);
        userFragHeader.fragOffset = ntohl(userFragHeader.fragOffset);
        userFragHeader.fragLength = ntohl(userFragHeader.fragLength);
        
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.portId);
    
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                       ("Got these values fragid %d, frag offset %d, fraglength %d, "
                        "flag %x from 0x%x:0x%x at 0x%x:0x%x\n",
                        (userFragHeader.msgId), (userFragHeader.fragOffset),
                        (userFragHeader.fragLength), userFragHeader.header.flag,
                        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress,
                        userFragHeader.header.srcAddress.iocPhyAddress.portId,
                        gIocLocalBladeAddress, pTipcCommPort->portId));

#if 0
        rc =
            iocUserFragmentReceive(&message, pBuffer, &userFragHeader,
                                   pTipcCommPort->portId,
                                   (ClUint32T*)&bytes);
#else
        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if(userFragHeader.header.flag == IOC_LAST_FRAG)
            clLogTrace("FRAG", "RECV", "Got Last frag at offset [%d], size [%d], received [%d]",
                       userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(pBuffer, &userFragHeader, 
                                      pTipcCommPort->portId, bytes, message, sync);
#endif
        /*
         * recalculate timeouts
         */
        if (rc == CL_IOC_RC(IOC_MSG_QUEUED))
        {
            if(timeout == CL_IOC_TIMEOUT_FOREVER)
                goto retry;
            lTimeout = (clOsalStopWatchTimeGet() - tml1)/1000;
            if (lTimeout < timeout)
            {
                timeout = timeout - lTimeout;
                goto retry;
            }
            else
            {
                rc = CL_IOC_RC(CL_ERR_TIMEOUT);
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Dropping a received fragmented-packet. "
                                                  "Could not receive the complete packet within "
                                                  "the specified timeout. Packet size is %d", bytes));
                goto out;
            }
        }
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Dropping a received fragmented-packet. "
                                              "Failed to reassemble the packet. Packet size is %d. "
                                              "error code 0x%x", bytes, rc));
            goto out;
        }
    }

    pRecvParam->length = bytes;
    pRecvParam->priority = userHeader.priority;
    pRecvParam->protoType = userHeader.protocolType;
    *((ClIocPhysicalAddressT*)&pRecvParam->srcAddr) = userHeader.srcAddress.iocPhyAddress;

    out:
    return rc;
}

ClRcT clIocReceive(ClIocCommPortHandleT commPortHdl,
                   ClIocRecvOptionT *pRecvOption,
                   ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClUint8T buffer[0x10000];
    return __clIocReceive(commPortHdl, pRecvOption, buffer, sizeof(buffer), message, pRecvParam, CL_TRUE);
}

ClRcT clIocReceiveAsync(ClIocCommPortHandleT commPortHdl,
                        ClIocRecvOptionT *pRecvOption,
                        ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClUint8T buffer[0x10000];
    return __clIocReceive(commPortHdl, pRecvOption, buffer, sizeof(buffer), message, pRecvParam, CL_FALSE);
}

ClRcT clIocReceiveWithBuffer(ClIocCommPortHandleT commPortHdl,
                             ClIocRecvOptionT *pRecvOption,
                             ClUint8T *buffer, ClUint32T bufSize,
                             ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    if(!buffer || !bufSize) return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    return __clIocReceive(commPortHdl, pRecvOption, buffer, bufSize, message, pRecvParam, CL_TRUE);
}

ClRcT clIocReceiveWithBufferAsync(ClIocCommPortHandleT commPortHdl,
                                  ClIocRecvOptionT *pRecvOption,
                                  ClUint8T *buffer, ClUint32T bufSize,
                                  ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    if(!buffer || !bufSize) return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    return __clIocReceive(commPortHdl, pRecvOption, buffer, bufSize, message, pRecvParam, CL_FALSE);
}

ClRcT clIocCommPortModeGet(ClIocCommPortHandleT iocCommPort,
        ClIocCommPortModeT *modeType)
{
    NULL_CHECK(modeType);
    *modeType = CL_IOC_BLOCKING_MODE;
    return CL_OK;
}


ClRcT clIocLibFinalize()
{
    if(gIocInit == CL_FALSE)
        return CL_OK;
    gIocInit = CL_FALSE;
    clJobQueueDelete(&iocFragmentJobQueue);
    clTipcEventHandlerFinalize();
    clNodeCacheFinalize();
    clTipcNeighCompsFinalize();
    clOsalMutexLock(userObj.reassemblyMutex);
    clCntDelete(userObj.reassemblyLinkList);
    clOsalMutexUnlock(userObj.reassemblyMutex);
    clOsalMutexDelete(userObj.reassemblyMutex);
    clOsalMutexDelete(ClIocFragMutex);
    return CL_OK;
}

static ClRcT clIocLeakyBucketInitialize(void)
{
    ClRcT rc = CL_OK;
    gClIocTrafficShaper = clParseEnvBoolean("CL_ASP_TRAFFIC_SHAPER");
    if(gClIocTrafficShaper)
    {
        ClInt64T leakyBucketVol = getenv("CL_LEAKY_BUCKET_VOL") ? 
            (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_VOL")) : CL_LEAKY_BUCKET_DEFAULT_VOL;
        ClInt64T leakyBucketLeakSize = getenv("CL_LEAKY_BUCKET_LEAK_SIZE") ?
            (ClInt64T)atoi(getenv("CL_LEAKY_BUCKET_LEAK_SIZE")) : CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE;
        ClTimerTimeOutT leakyBucketInterval = {.tsSec = 0, .tsMilliSec = CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL };
        leakyBucketInterval.tsMilliSec = getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL") ? atoi(getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL")) :
            CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL;
        clLogInfo("LEAKY", "BUCKET-INI", "Creating a leaky bucket with vol [%lld], leak size [%lld], interval [%d ms]",
                  leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval.tsMilliSec);
        rc = clLeakyBucketCreate(leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval, &gClLeakyBucket);
    }
    return rc;
}

ClRcT clIocConfigInitialize(ClIocLibConfigT *pConf)
{
    ClRcT retCode;
    ClRcT rc;
#ifdef CL_TIPC_COMPRESSION
    ClCharT *pTipcCompressionBoundary = NULL;
#endif

    NULL_CHECK(pConf);

    if (gIocInit == CL_TRUE)
        return CL_OK;

    if (CL_IOC_PHYSICAL_ADDRESS_TYPE !=
        CL_IOC_ADDRESS_TYPE_FROM_NODE_ADDRESS((pConf->nodeAddress)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,
                       ("\nCritical : Invalid IOC address: Node Address [0x%x] is an invalid physical address.\n",pConf->nodeAddress));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    if ((CL_IOC_RESERVED_ADDRESS == pConf->nodeAddress) ||
        (CL_IOC_BROADCAST_ADDRESS == pConf->nodeAddress))
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,
                       ("\nCritical : Invalid IOC address: Node Address [0x%x] is one of the reserved IOC addresses.\n ",pConf->nodeAddress));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }

    gIocLocalBladeAddress = ((ClIocLibConfigT *) pConf)->nodeAddress;


    clOsalMutexCreate(&ClIocFragMutex);

    memset(&userObj, 0, sizeof(ClIocUserObjectT));

    retCode = clOsalMutexCreate(&userObj.reassemblyMutex);
    if (retCode != CL_OK)
    {
        goto error_1;
    }
    
    retCode =
        clCntLlistCreate(iocUserKeyCompare, iocUserDeleteCallBack,
                         iocUserDeleteCallBack, CL_CNT_NON_UNIQUE_KEY,
                         &userObj.reassemblyLinkList);
    if (retCode != CL_OK)
    {
        goto error_2;
    }

    userReassemblyTimerExpiry.tsMilliSec = CL_IOC_REASSEMBLY_TIMEOUT % 1000;
    userReassemblyTimerExpiry.tsSec = CL_IOC_REASSEMBLY_TIMEOUT / 1000;

    retCode = clTipcNeighCompsInitialize(gIsNodeRepresentative);
    if(retCode != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed at neighbor initialize. rc=0x%x\n",retCode));
        goto error_4;
    }

    retCode = clNodeCacheInitialize(gIsNodeRepresentative);
    if(retCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Node cache initialize returned [%#x]", retCode));
        goto error_5;
    }

    clIocLeakyBucketInitialize();

    gClTipcReplicast = clParseEnvBoolean("CL_ASP_TIPC_REPLICAST");

#ifdef CL_TIPC_COMPRESSION
    if( (pTipcCompressionBoundary = getenv("CL_TIPC_COMPRESSION_BOUNDARY") ) )
    {
        gTipcCompressionBoundary = atoi(pTipcCompressionBoundary);
    }
    
    if(!gTipcCompressionBoundary)
        gTipcCompressionBoundary = CL_TIPC_COMPRESSION_BOUNDARY;
#endif

    if(gIsNodeRepresentative == CL_TRUE)
    {
        /*
         * Initialize a debug mode time server to fetch times from remote host.
         * for debugging.
         */
        clTimeServerInitialize();
        retCode = clTipcEventHandlerInitialize();
    }

    if(retCode == CL_OK)
    {
        gIocInit = CL_TRUE;
        return CL_OK;
    }

error_5:    
    rc = clTipcNeighCompsFinalize();
    CL_ASSERT(rc == CL_OK);
error_4:
    rc = clCntDelete(userObj.reassemblyLinkList);
    CL_ASSERT(rc == CL_OK);
error_2:
    rc = clOsalMutexDelete(userObj.reassemblyMutex);
    CL_ASSERT(rc == CL_OK);
error_1:
    rc = clOsalMutexDelete(ClIocFragMutex);
    CL_ASSERT(rc == CL_OK);

    return retCode;
}


ClRcT clIocLibInitialize(ClPtrT pConfig)
{
    ClIocLibConfigT iocConfig = {0};
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;

    iocConfig.version = CL_IOC_HEADER_VERSION;
    iocConfig.nodeAddress = clAspLocalId;

    retCode = clIocConfigInitialize(&iocConfig);
    if (retCode != CL_OK)
    {
        return retCode;
    }

    rc = clOsalMutexInit(&gClTipcNeighborList.neighborMutex); 
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClTipcPortMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClTipcCompMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClTipcMcastMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&iocReassembleLock);
    CL_ASSERT(rc == CL_OK);
    rc = clJobQueueInit(&iocFragmentJobQueue, 0, 1);
    CL_ASSERT(rc == CL_OK);
    rc = __iocFragmentPoolInitialize();
    CL_ASSERT(rc == CL_OK);
    /*Add ourselves into the neighbor table*/
    gClTipcNeighborList.numEntries = 0;

    CL_LIST_HEAD_INIT(&gClTipcNeighborList.neighborList);

    rc = clIocNeighborAdd(gIocLocalBladeAddress,CL_IOC_NODE_UP);
    CL_ASSERT(rc == CL_OK);

    clTransportLayerInitialize();
    clTransportInitialize(NULL, gIsNodeRepresentative);
    return CL_OK;
}


ClRcT clIocCommPortReceiverUnblock(ClIocCommPortHandleT portHandle)
{
    ClTipcCommPortT *pTipcCommPort = (ClTipcCommPortT *)portHandle;
    ClRcT rc = CL_OK;
    ClInt32T fd;
    struct sockaddr_tipc destAddress;
    ClUint32T portId;
    ClTimerTimeOutT timeout = {.tsSec=0,.tsMilliSec=200};
    ClInt32T tries=0;
    bzero((char*)&destAddress,sizeof(destAddress));

    portId = pTipcCommPort->portId;
    fd = pTipcCommPort->fd;
    destAddress.family = AF_TIPC;
    destAddress.addrtype = TIPC_ADDR_NAME;
    destAddress.scope = TIPC_NODE_SCOPE;
    destAddress.addr.name.domain=0;
    destAddress.addr.name.name.type = CL_TIPC_SET_TYPE(portId);
    destAddress.addr.name.name.instance = gIocLocalBladeAddress;
    /*Grab the lock to avoid a race with lost wakeups triggered by the recv.*/
    clOsalMutexLock(&pTipcCommPort->unblockMutex);
    ++pTipcCommPort->blocked;
    if(sendto(fd,CL_TIPC_PORT_EXIT_MESSAGE,sizeof(CL_TIPC_PORT_EXIT_MESSAGE),
              0,(struct sockaddr*)&destAddress,sizeof(destAddress))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error sending port exit message to port:0x%x.errno=%d\n",portId,errno));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        clOsalMutexUnlock(&pTipcCommPort->unblockMutex);
        goto out;
    }
    while(tries++ < 3)
    {
        rc = clOsalCondWait(&pTipcCommPort->unblockCond,&pTipcCommPort->unblockMutex,timeout);
        if(CL_GET_ERROR_CODE(rc)==CL_ERR_TIMEOUT)
        {
            continue;
        }
        break;
    }
    --pTipcCommPort->blocked;
    /*
     * we come back with lock held.Now unblock the receiver 
     * irrespective of whether we succeded in cond wait or failed
    */
    clOsalCondSignal(&pTipcCommPort->recvUnblockCond);

    clOsalMutexUnlock(&pTipcCommPort->unblockMutex);

    out:
    return rc;
}



ClRcT clIocMaxPayloadSizeGet(ClUint32T *pSize)
{
    NULL_CHECK(pSize);
    *pSize = 0xffffffff;
    return CL_OK;
}


ClRcT clIocMaxNumOfPrioritiesGet(ClUint32T *pMaxNumOfPriorities)
{
    NULL_CHECK(pMaxNumOfPriorities);
    *pMaxNumOfPriorities = CL_IOC_MAX_PRIORITIES;
    return CL_OK;

}

ClRcT clIocCommPortGet(ClIocCommPortHandleT pIocCommPort, ClUint32T *portId)
{
    ClTipcCommPortT *pTipcCommPort= (ClTipcCommPortT*)pIocCommPort;
    NULL_CHECK(portId);
    NULL_CHECK(pTipcCommPort);
    *portId = pTipcCommPort->portId;
    return CL_OK;
}

static __inline__ ClUint32T __iocReassembleHashKey(ClIocReassembleKeyT *key)
{
    ClUint32T cksum = 0;
    clCksm32bitCompute((ClUint8T*)key, sizeof(*key), &cksum);
    return cksum & IOC_REASSEMBLE_HASH_MASK;
}

static ClIocReassembleNodeT *__iocReassembleNodeFind(ClIocReassembleKeyT *key, ClUint64T timerId)
{
    ClUint32T hash = __iocReassembleHashKey(key);
    struct hashStruct *iter = NULL;
    for(iter = iocReassembleHashTable[hash]; iter; iter = iter->pNext)
    {
        ClIocReassembleNodeT *node = hashEntry(iter, ClIocReassembleNodeT, hash);
        if(timerId 
           &&
           node->timerId != timerId)
            continue;
        if(!memcmp(&node->key, key, sizeof(node->key)))
            return node;
    }
    return NULL;
}

static ClInt32T __iocFragmentCmp(ClRbTreeT *node1, ClRbTreeT *node2)
{
    ClIocFragmentNodeT *frag1 = CL_RBTREE_ENTRY(node1, ClIocFragmentNodeT, tree);
    ClIocFragmentNodeT *frag2 = CL_RBTREE_ENTRY(node2, ClIocFragmentNodeT, tree);
    return frag1->fragOffset - frag2->fragOffset;
}

static ClRcT __iocReassembleTimer(void *key)
{
    ClIocReassembleNodeT *node = NULL;
    ClIocReassembleTimerKeyT *timerKey = key;
    ClRbTreeT *fragHead = NULL;
    clOsalMutexLock(&iocReassembleLock);
    node = __iocReassembleNodeFind(&timerKey->key, timerKey->timerId);
    if(!node)
    {
        clOsalMutexUnlock(&iocReassembleLock);
        goto out;
    }
    clLogTrace("FRAG", "RECV", "Running the reassembly timer for sender node [%#x:%#x] with length [%d] bytes", 
               node->key.sendAddr.nodeAddress, node->key.sendAddr.portId, node->currentLength);
    while( (fragHead = clRbTreeMin(&node->reassembleTree) ) )
    {
        ClIocFragmentNodeT *fragNode = CL_RBTREE_ENTRY(fragHead, ClIocFragmentNodeT, tree);
        clRbTreeDelete(&node->reassembleTree, fragHead);
        __iocFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
    clOsalMutexUnlock(&iocReassembleLock);
    clHeapFree(node);
    out:
    clHeapFree(timerKey);
    return CL_OK;
}

static ClRcT __iocReassembleDispatch(ClIocReassembleNodeT *node, ClTipcFragHeaderT *fragHeader, 
                                     ClBufferHandleT message, ClBoolT sync)
{
    ClBufferHandleT msg = 0;
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_IOC_RC(IOC_MSG_QUEUED);
    ClRbTreeT *iter = NULL;
    ClUint32T len = 0;
    if(message)
    {
        msg = message;
    }
    else
    {
        rc = clBufferCreate(&msg);
        CL_ASSERT(rc == CL_OK);
    }
    while( (iter = clRbTreeMin(&node->reassembleTree)) )
    {
        ClIocFragmentNodeT *fragNode = CL_RBTREE_ENTRY(iter, ClIocFragmentNodeT, tree);
        if(clBufferAppendHeap(msg, fragNode->fragBuffer, fragNode->fragLength) != CL_OK)
        {
            rc = clBufferNBytesWrite(msg, fragNode->fragBuffer, fragNode->fragLength);
            __iocFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
        }
        else
        {
            /*
             * Drop reference instead of recyling the stitched heap.
             */
            clHeapFree(fragNode->fragBuffer);
        }
        CL_ASSERT(rc == CL_OK);
        clLogTrace("IOC", "REASSEMBLE", "Reassembled fragment offset [%d], len [%d]",
                   fragNode->fragOffset, fragNode->fragLength);
        clRbTreeDelete(&node->reassembleTree, iter);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
    clBufferLengthGet(msg, &len);
    if(!len)
    {
        if(!message)
        {
            clBufferDelete(&msg);
        }
    }
    else if(!sync)
    {
        /*
         * Call the EO job queue handler here.
         */
        ClIocRecvParamT recvParam = {0};
        recvParam.length = len;
        recvParam.priority = fragHeader->header.priority;
        recvParam.protoType = fragHeader->header.protocolType;
        memcpy(&recvParam.srcAddr, &fragHeader->header.srcAddress, sizeof(recvParam.srcAddr));
        rc = clEoEnqueueReassembleJob(msg, &recvParam);
        if(rc != CL_OK)
        {
            if(!message)
            {
                clBufferDelete(&msg);
            }
        }
    }
    else
    {
        /* sync. reassemble success */
        retCode = CL_OK; 
    }
    clHeapFree(node);
    return retCode;
}

static ClRcT __iocFragmentCallback(ClPtrT job, ClBufferHandleT message, ClBoolT sync)
{
    ClIocReassembleKeyT key = {0};
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
    key.sendAddr = fragmentJob->fragHeader.header.srcAddress.iocPhyAddress;
    node = __iocReassembleNodeFind(&key, 0);
    if(!node)
    {
        /*
         * create a new reassemble node.
         */
        ClUint32T hashKey = __iocReassembleHashKey(&key);
        ClIocReassembleTimerKeyT *timerKey = clHeapCalloc(1, sizeof(*timerKey));
        CL_ASSERT(timerKey != NULL);
        node = clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        memcpy(&node->key, &key, sizeof(node->key));
        node->timerId = ++iocReassembleCurrentTimerId;
        memcpy(&timerKey->key, &node->key, sizeof(timerKey->key)); /*safe w.r.t node deletes*/
        timerKey->timerId = node->timerId;
        clRbTreeInit(&node->reassembleTree, __iocFragmentCmp);
        rc = clTimerCreate(userReassemblyTimerExpiry, 
                           CL_TIMER_ONE_SHOT | CL_TIMER_VOLATILE,
                           CL_TIMER_SEPARATE_CONTEXT, __iocReassembleTimer,
                           (void *)timerKey, &node->reassembleTimer);
        CL_ASSERT(rc == CL_OK);
        node->currentLength = 0;
        node->expectedLength = 0;
        node->numFragments = 0;
        hashAdd(iocReassembleHashTable, hashKey, &node->hash);
        clTimerStart(node->reassembleTimer);
    }
    fragmentNode = clHeapCalloc(1, sizeof(*fragmentNode));
    CL_ASSERT(fragmentNode != NULL);
    fragmentNode->fragOffset = fragmentJob->fragHeader.fragOffset;
    fragmentNode->fragLength = fragmentJob->fragHeader.fragLength;
    fragmentNode->fragBuffer = fragmentJob->buffer;
    node->currentLength += fragmentNode->fragLength;
    ++node->numFragments;
    clRbTreeInsert(&node->reassembleTree, &fragmentNode->tree);
    if(flag == IOC_LAST_FRAG)
    {
        if(fragmentNode->fragOffset + fragmentNode->fragLength == node->currentLength)
        {
            retCode = __iocReassembleDispatch(node, &fragmentJob->fragHeader, message, sync);
        }
        else
        {
            /*
             * we update the expected length.
             */
            clLogTrace("FRAG", "RECV", "Out of order last fragment received for offset [%d], len [%d] bytes, "
                       "current length [%d] bytes",
                       fragmentNode->fragOffset, fragmentNode->fragLength, node->currentLength);
            node->expectedLength = fragmentNode->fragOffset + fragmentNode->fragLength;
        }
    }
    else if(node->currentLength == node->expectedLength)
    {
        retCode = __iocReassembleDispatch(node, &fragmentJob->fragHeader, message, sync);
    }
    else
    {
        /*
         * Now increase the timer based on the number of fragments being received or the node length.
         * Since the sender could be doing flow control, be have an adaptive reassembly timer.
         */
        if( !( node->numFragments & CL_IOC_REASSEMBLY_FRAGMENTS_MASK ) )
        {
            clLogDebug("FRAG", "RECV", "Updating the reassembly timer to refire after [%d] secs for node length [%d], "
                       "num fragments [%d]",
                       userReassemblyTimerExpiry.tsSec, node->currentLength, node->numFragments);
            clTimerUpdate(node->reassembleTimer, userReassemblyTimerExpiry);
        }
    }
    clHeapFree(job);
    return retCode;
}

static ClRcT iocFragmentCallback(ClPtrT job)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&iocReassembleLock);
    rc = __iocFragmentCallback(job, 0, CL_FALSE);
    clOsalMutexUnlock(&iocReassembleLock);
    return rc;
}

ClRcT __iocUserFragmentReceive(ClUint8T *pBuffer,
                               ClTipcFragHeaderT *userHdr,
                               ClIocPortT portId,
                               ClUint32T length,
                               ClBufferHandleT message,
                               ClBoolT sync)
{
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
    if(!sync)
    {
        rc = clJobQueuePush(&iocFragmentJobQueue, iocFragmentCallback, (ClPtrT)job);
        if(rc != CL_OK)
            return rc;
        rc = CL_IOC_RC(IOC_MSG_QUEUED);
    }
    else
    {
        rc = __iocFragmentCallback((ClPtrT)job, message, CL_TRUE);
    }
    return rc;
}

/*************************************************************************
 *                                                                        
 *  Name: iocUserFragmentReceive                                                 
 *                                                                        
 *  Description: IOC Core fragment receive function. This function is called from
 *               user interface API(North bound).
 *               Call this after reseting the read pointer at userData.
 *               Header is in host order. The buffer is unchanged.
 *                                                                        
 *  Parameters : origMsg -> message Handle in which the received packet is to be copied.
 *             : hdr     -> iocHeader.
 *             : flag    -> flags.
 *  Returns    : ClRcT
 *************************************************************************/
static ClRcT iocUserFragmentReceive(ClBufferHandleT *pOrigMsg,
        ClUint8T *pBuffer,
        ClTipcFragHeaderT *pUserHdr,
        ClIocPortT portId,
        ClUint32T *pLength)
{
    ClRcT rc;
    ClCntNodeHandleT nodeHandle;
    ClIocReassemblyKeyT key;
    ClIocReassemblyNodeT *node;
    ClTipcFragNodeT *fragNode;
    ClRcT retCode = 0;
    ClTipcFragHeaderT *pHdr = NULL;


    key.destAddr.nodeAddress = gIocLocalBladeAddress;
    key.destAddr.portId = portId;
    key.sendAddr = pUserHdr->header.srcAddress.iocPhyAddress;
    key.fragId = pUserHdr->msgId;

    clOsalMutexLock(userObj.reassemblyMutex);

    rc = clCntNodeFind(userObj.reassemblyLinkList, (ClCntKeyHandleT)(&key), &nodeHandle);
    if (rc != CL_OK)
    {
        node = (ClIocReassemblyNodeT *)clHeapAllocate((ClUint32T)
                sizeof(ClIocReassemblyNodeT));
        if (node == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: Memory Allocation failure for size = %zd\n", sizeof(ClIocReassemblyNodeT)));
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            goto err_ret_1;
        }
        memset(node, 0, sizeof(*node));

        rc = clCntLlistCreate(iocUserFragKeyCompare, iocUserFragDeleteCallBack,
                iocUserFragDeleteCallBack, CL_CNT_NON_UNIQUE_KEY,
                &(node->fragList));
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: IOCCoreFragReceive-> Can't create List  RC=0x%x\n", rc));
            goto err_ret_2;
        }

        node->reassemblyKey = key;
#ifdef CL_TIPC_COMPRESSION
        node->pktSendTime = __clNtohl64(pUserHdr->header.pktTime);
#endif
        rc = clCntNodeAdd(userObj.reassemblyLinkList,
                (ClCntKeyHandleT) &node->reassemblyKey,
                (ClCntDataHandleT) node, NULL);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: Failed to add a node to the reassembly container. errorCode = 0x%x\n", rc));
            goto err_ret_3;
        }

        rc = clTimerCreate(userReassemblyTimerExpiry, CL_TIMER_ONE_SHOT,
                CL_TIMER_SEPARATE_CONTEXT, userReassemblyTimer,
                (void *) node, &node->timerID);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: Timer create failed RC=0x%x\n", rc));
            goto err_ret_4;
        }

        rc = clTimerStart(node->timerID);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("\nError :Failed to start timer. error code = 0x%x\n", rc));
            goto err_ret_5;
        }

        goto queue_the_packet;

err_ret_5:
        retCode = clTimerDelete(&(node->timerID));
        CL_ASSERT(retCode == CL_OK);
err_ret_4:
        retCode = clCntNodeFind(userObj.reassemblyLinkList,
                (ClCntKeyHandleT) &key, &nodeHandle);
        CL_ASSERT(retCode == CL_OK);
        retCode = clCntNodeDelete(userObj.reassemblyLinkList, nodeHandle);
        CL_ASSERT(retCode == CL_OK);
err_ret_3:
        retCode = clCntDelete(node->fragList);
        CL_ASSERT(retCode == CL_OK);
err_ret_2:
        clHeapFree(node);
err_ret_1:
        retCode = clOsalMutexUnlock(userObj.reassemblyMutex);
        CL_ASSERT(retCode == CL_OK);

        return rc;
    }
    else
    {
        rc = clCntNodeUserDataGet(userObj.reassemblyLinkList, nodeHandle,
                (ClCntDataHandleT *) &node);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Container Node data get failed. error code [0x%x]\n", rc));
            retCode = clOsalMutexUnlock(userObj.reassemblyMutex);
            CL_ASSERT(retCode == CL_OK);

            return rc;
        }
    }


queue_the_packet:

    fragNode =
        (ClTipcFragNodeT *) clHeapAllocate((ClUint32T)
                sizeof(ClTipcFragNodeT));
    if (fragNode == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nERROR: Memory Allocation failure for size =%d \n",
                 (ClUint32T) sizeof(ClTipcFragNodeT)));
        node->mayDiscard = 1;   /* this can be discarded as we have
                                 * dropped one frag */
        rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
        goto error_ret;
    }

    pHdr = (ClTipcFragHeaderT *)clHeapAllocate(sizeof(ClTipcFragHeaderT));
    if(pHdr == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("ERROR: Memory Allocation failure for size = %zd\n",
                 sizeof(sizeof(ClTipcFragHeaderT))));
        rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
        goto error_ret_1;
    }
    memcpy(pHdr, pUserHdr, sizeof(ClTipcFragHeaderT));

    fragNode->header = pHdr;

    rc = clBufferCreate(&fragNode->fragPkt);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to allocate buffer. error code 0x%x\n",rc));
        node->mayDiscard = 1;   /* this can be discarded as we have
                                 * dropped one frag */
        goto error_ret_2;
    }

    rc = clBufferNBytesWrite(fragNode->fragPkt, pBuffer, *pLength);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Failed to write to the buffer. error code 0x%x\n",rc));
        node->mayDiscard = 1;   /* this can be discarded as we have
                                 * dropped one frag */
        goto error_ret_3;
    }

    rc = clCntNodeAdd(node->fragList,
            (ClCntKeyHandleT)(ClWordT)pHdr->fragOffset,
            (ClCntDataHandleT) fragNode, NULL);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("\nERROR: Failed to add a node to fragment list container. errorCode = 0x%x\n", rc));
        node->mayDiscard = 1;   /* this can be discarded as we have
                                 * dropped one frag */
        goto error_ret_3;
    }

    if (node->mayDiscard == 0)
    {
        if (node->expectedFragOffset != pHdr->fragOffset)
            node->mayDiscard = 1;
        else
            node->expectedFragOffset =
                node->expectedFragOffset + pHdr->fragLength;
    }

    userObj.reassemblyQsize =
        userObj.reassemblyQsize + pHdr->fragLength;

    /* total length of the data received till now. */
    node->totalLength = node->totalLength + pHdr->fragLength;

    /* Total length of the data that must be received to form a complete
     * assembled packet. */
    if(pHdr->header.flag == IOC_LAST_FRAG)
    {
        node->expectedLength = pHdr->fragOffset + pHdr->fragLength;
        node->lastFragSeen = 1;
    }

    rc = CL_IOC_RC(IOC_MSG_QUEUED);

    /* Check if all the fragments are received and if received then call for
     * reassembling, the fragments. */
    if ((node->lastFragSeen) &&
            (node->expectedLength == node->totalLength))
    {
#ifdef CL_TIPC_COMPRESSION
        ClTimeT pktSendTime = node->pktSendTime;
        ClTimeT pktRecvTime = 0;
#endif
        rc = iocUserReassemblePacket(pOrigMsg, nodeHandle, node,
                pLength);
        if(rc != CL_OK)
        {
            clLogDebug("IOC", "REASSEMBLE", "Failed to reassemble the fragments of a packet. error code [0x%x]", rc);
            node->mayDiscard = 1;   /* this can be discarded as we have
                                     * dropped one frag */
            goto error_ret_3;
        }
#ifdef CL_TIPC_COMPRESSION
        if(rc == CL_OK && pktSendTime)
        {
            ClTimeValT tv = {0};
            clTimeServerGet(NULL, &tv);
            pktRecvTime = tv.tvSec*1000000LL + tv.tvUsec;
            if(pktRecvTime)
                clLogDebug("IOC", "REASSEMBLE", "Packet round trip time [%lld] usecs for [%d] bytes",
                        pktRecvTime - pktSendTime, *pLength);
        }
#endif
    }

    goto ok_done;

error_ret_3:
    retCode = clBufferDelete(fragNode->fragPkt);
    CL_ASSERT(retCode == CL_OK);
error_ret_2:
    clHeapFree(pHdr);
error_ret_1:
    clHeapFree(fragNode);
error_ret:
ok_done:
    retCode = clOsalMutexUnlock(userObj.reassemblyMutex);
    CL_ASSERT(retCode == CL_OK);

    return rc;
}


static ClRcT userReassemblyTimer(void *data)
{
    ClCntNodeHandleT nodeHdl;
    ClRcT rc;
    ClRcT retCode = 0;

    /*
     * from the data go and delete the container node whose del call back will
     * call destroy frag list and also will update the recvQsize
     */
    ClIocReassemblyNodeT *node = (ClIocReassemblyNodeT *) data;

    retCode = clOsalMutexLock(userObj.reassemblyMutex);

    rc = clCntNodeFind(userObj.reassemblyLinkList,
            (ClCntKeyHandleT) (&(node->reassemblyKey)), &nodeHdl);
    if (rc != CL_OK)
    {
        retCode = clOsalMutexUnlock(userObj.reassemblyMutex);
        return rc;
    }
    rc = clCntNodeDelete(userObj.reassemblyLinkList, nodeHdl);
    if(retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("\nERROR: Failed to delete fragment list. errorCode = 0x%x\n", retCode));

    retCode = clOsalMutexUnlock(userObj.reassemblyMutex);
    return rc;
}


/*************************************************************************
 *
 *  Name: iocUserReassemblePacket                                                 
 *
 *  Description: iocUserReassemblePacket function. This function reassembles the
 *               fraged packets.
 *
 *  Parameters : nodeHandle -> container node having all the frags of 
 *                             the packet which needs to be reassembled.
 *             : node       -> reassembly node information.
 *  Returns    : ClRcT
 *************************************************************************/
static ClRcT iocUserReassemblePacket(ClBufferHandleT *pOrigMsg,
        ClCntNodeHandleT nodeHandle,
        ClIocReassemblyNodeT *node,
        ClUint32T *pLength)
{
    ClCntNodeHandleT nextHandle;
    ClCntNodeHandleT currHandle;
    ClTipcFragNodeT *fragNode;
    ClRcT rc;
    ClRcT retCode = 0;

    rc = clTimerDelete(&(node->timerID));
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("ERROR: Timer delete failed. error code [0x%x]\n", rc));
        goto error_out;
    }

    rc = clCntFirstNodeGet(node->fragList, &currHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nERROR: First node get failed. error code [0x%x]\n", rc));
        goto error_out;
    }

    do {
        rc = clCntNodeUserDataGet(node->fragList, currHandle,
                (ClCntDataHandleT *) &fragNode);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("ERROR: Container node data get failed. error code [0x%x]\n", rc));
            goto error_out;
        }

        rc = clBufferConcatenate(*pOrigMsg, &fragNode->fragPkt);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("ERROR: Buffer concatenate failed. error code [0x%x]\n", rc));
            goto error_out;
        }

        rc = clCntNextNodeGet(node->fragList, currHandle, &nextHandle);
        if(nextHandle == 0) break;
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("ERROR: Failed to get next fragment from container. error code [0x%x]\n", rc));;
            goto error_out;
        }

        currHandle = nextHandle;
    }while(1);

    *pLength = node->totalLength;
    rc = clBufferReadOffsetSet(*pOrigMsg, 0, CL_BUFFER_SEEK_SET);
    if(rc != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("ERROR: Buffer read offset set is failed. error code [0x%x]\n", rc));

error_out:
    retCode = clCntNodeDelete(userObj.reassemblyLinkList, nodeHandle);
    if (retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("ERROR: Container node delete failed. It will result in memory leak. error code [0x%x]\n", retCode));

    return rc;
}




/*
 * FOR CONTAINERS 
 */

/*************************************************************************
 *
 *  Name: iocUserKeyCompare                                                 
 *
 *  Description: iocKeyCompare function. Reassembly key compare function.
 *               All the params are in host order.
 *
 *  Parameters : key1 -> (frag id, source address, destination address).
 *             : key2 -> (frag id, source address, destination address).
 *  Returns    : ClInt32T
 *************************************************************************/
static ClInt32T iocUserKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    if (key1 == 0 || key2 == 0)
    {
        return CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }
    cmpResult =
        memcmp((void *) key1, (void *) key2, sizeof(ClIocReassemblyKeyT));
    return cmpResult;
}



/*************************************************************************
 *
 *  Name: iocUserDeleteCallBack                                                 
 *
 *  Description: iocUserDeleteCallBack function for reassembly. This function is
 *               called when a node from the reassembly container is deleted.
 *
 *  Parameters : userKey -> (frag id, source address, destination address).
 *             : userData-> ClIocReassemblyNodeT structure.
 *  Returns    : void
 *************************************************************************/
static void iocUserDeleteCallBack(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    ClIocReassemblyNodeT *node = (ClIocReassemblyNodeT *) userData;
    ClRcT retCode = 0;

    CL_FUNC_ENTER();
    /*
     * destroy the internal container list 
     */
    /*
     * it will be called from timer context or by frag receieve after stopping 
     * the timer
     */
    /*
     * decrement the total node count, free the user data
     */
    if (node)
    {
        /*
         * lock is already there it
         */
        if (node->timerID)
        {
            retCode = clTimerDelete(&(node->timerID));
            if(retCode != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("\nERROR: Timer delete failed  RC=0x%x\n", retCode));
        }

        userObj.reassemblyQsize = userObj.reassemblyQsize - node->totalLength;
        retCode = clCntDelete(node->fragList);
        if(retCode != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                    ("\nERROR: Node delete failed  RC=0x%x\n", retCode));

        clHeapFree(node);
    }
    CL_FUNC_EXIT();
    return;
}

/*
 * Container to keep frags for every node in reassmbly
 */

/*************************************************************************
 *
 *  Name: iocUserFragKeyCompare                                                 
 *
 *  Description: iocUserFragKeyCompare function for reassembly. This function
 *               compares the frag offsets.
 *
 *  Parameters : key1 -> frag offset.
 *             : key2 -> frag offset.
 *  Returns    : ClInt32T
 *************************************************************************/
static ClInt32T iocUserFragKeyCompare(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}


/*************************************************************************
 *
 *  Name: iocUserFragDeleteCallBack                                                 
 *
 *  Description: iocUserFragDeleteCallBack function for frag container. This function
 *               is called when a node(frag) from frag container is deleted.
 *
 *  Parameters : userKey -> frag offset.
 *             : userData-> ClTipcFragNodeT node.
 *  Returns    : void
 *************************************************************************/
static void iocUserFragDeleteCallBack(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    ClTipcFragNodeT *node = (ClTipcFragNodeT *) userData;

    ClRcT retCode = 0;
    /*
     * it will be called from timer context or by frag receieve after stopping 
     * the timer
     */
    /*
     * free the userData after freeing the message
     */

    if (node)
    {

        if (node->fragPkt)
        {
            /*
             * it means Reassembly has not happened, it is either coming from
             * checkandmaydiscardpacket or fragrecv or reassembletimer. in
             * iocUserReassemblePacket concatenate deletes the message and make 
             * it NULL
             */
            retCode = clBufferDelete(&node->fragPkt);
            if(retCode != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ("\nERROR: Buffer delete failed  RC=0x%x\n", retCode));
        }
        clHeapFree(node->header);
        clHeapFree(node);
    }
}

ClRcT clIocTotalNeighborEntryGet(ClUint32T *pNumberOfEntries)
{
    ClUint32T   i = 0;
    ClUint8T    status = 0;

    NULL_CHECK(pNumberOfEntries);

    for (i = 0; i < CL_IOC_MAX_NODES; i++)
    {
        clIocRemoteNodeStatusGet(i, &status);
        if (status == CL_IOC_NODE_UP)
        {
            ++(*pNumberOfEntries);
        }
    }
    return CL_OK;
}

ClRcT clIocNeighborListGet(ClUint32T *pNumberOfEntries,
        ClIocNodeAddressT *pAddrList)
{
    ClUint32T   i = 0;
    ClUint8T    status = 0;
    ClUint32T   numEntries = 0;

    NULL_CHECK(pNumberOfEntries);
    NULL_CHECK(pAddrList);

    if(!*pNumberOfEntries) return CL_OK;

    for (i = 0; i < CL_IOC_MAX_NODES; i++)
    {
        clIocRemoteNodeStatusGet(i, &status);
        if (status == CL_IOC_NODE_UP)
        {
            pAddrList[numEntries++] = i;
            if (numEntries == *pNumberOfEntries)
                break;
        }
    }
    if(numEntries != *pNumberOfEntries)
        *pNumberOfEntries = numEntries;
    return CL_OK;
}

static ClRcT clIocReplicastGet(ClIocPortT portId, ClIocAddressT **pAddressList, ClUint32T *pNumEntries)
{
    ClUint32T numEntries = 0;
    ClIocAddressT *addressList = NULL;
    ClUint32T i;
    ClUint8T status = 0;
    if(!pAddressList || !pNumEntries)
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    addressList = clHeapCalloc(CL_IOC_MAX_NODES, sizeof(*addressList));
    CL_ASSERT(addressList != NULL);
    for(i = 1; i < CL_IOC_MAX_NODES; ++i)
    {
        clIocRemoteNodeStatusGet(i, &status);
        if(status == CL_IOC_NODE_UP)
        {
            addressList[numEntries].iocPhyAddress.nodeAddress = i;
            addressList[numEntries].iocPhyAddress.portId = portId;
            ++numEntries;
        }
    }
    if(!numEntries)
    {
        clHeapFree(addressList);
        addressList = NULL;
    }
    *pAddressList = addressList;
    *pNumEntries = numEntries;
    return CL_OK;
}

ClRcT clIocTransparencyBind(ClTipcCommPortT *pTipcCommPort,
                            ClTipcLogicalAddressT *pLogicalAddress
                            )
{
    ClInt32T scope = 1;
    struct sockaddr_tipc address;
    ClBoolT bindFlag = CL_TRUE;
    ClRcT rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);

    if(pLogicalAddress->haState == CL_IOC_TL_STDBY)
    {
        bindFlag = CL_FALSE;
        scope = -1;
    }
    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = scope*TIPC_ZONE_SCOPE;
    address.addr.name.name.type = CL_IOC_TIPC_TYPE(pLogicalAddress->logicalAddress);
    address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pLogicalAddress->logicalAddress);
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
        goto out;
    }
    address.addr.name.name.type = CL_IOC_TIPC_MASTER_TYPE(pTipcCommPort->portId);
    address.addr.name.name.instance = gIocLocalBladeAddress;
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
        goto out_unbind;
    }
    pTipcCommPort->activeBind = bindFlag;
    rc = CL_OK;
    goto out;
    
    out_unbind:
    address.addr.name.name.type = CL_IOC_TIPC_TYPE(pLogicalAddress->logicalAddress);
    address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pLogicalAddress->logicalAddress);
    address.scope = -scope*TIPC_ZONE_SCOPE;
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
    }
    out:
    return rc;
}

ClRcT clIocTransparencyRegister(ClIocTLInfoT *pTLInfo)
{
    ClTipcCommPortT *pTipcCommPort=NULL;
    ClIocPortT portId;
    ClTipcCompT *pComp = NULL;
    ClTipcLogicalAddressT *pTipcLogicalAddress=NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    ClBoolT found=CL_FALSE;
    ClBoolT portFound = CL_FALSE;
    ClInt32T fd;

    NULL_CHECK(pTLInfo);

    if(CL_IOC_ADDRESS_TYPE_GET(&pTLInfo->logicalAddr) != CL_IOC_LOGICAL_ADDRESS_TYPE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ("\n TL: Invalid logical address:0x%llx \n",
                 pTLInfo->logicalAddr));
        goto out;
    }
    portId = pTLInfo->physicalAddr.portId;

    /*Now lookup the physical address for the tipc comm port*/
    clOsalMutexLock(&gClTipcPortMutex);
    pTipcCommPort = clTipcGetPort(portId);
    if(pTipcCommPort == NULL)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid physical address portid:0x%x.\n",portId));
        goto out;
    }

    /*Check for an already existing compid mapping the port */
    if(pTipcCommPort->pComp && pTipcCommPort->pComp->compId != pTLInfo->compId)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("More than 2 components cannot bind to the same port.Comp id 0x%x already bound to the port 0x%x\n",pTipcCommPort->pComp->compId,pTipcCommPort->portId));
        goto out;
    }

    fd = pTipcCommPort->fd;
    /*Try getting the compId*/
    clOsalMutexLock(&gClTipcCompMutex);
    pComp = clTipcGetComp(pTLInfo->compId);
    if(pComp == NULL)
    {
        clOsalMutexUnlock(&gClTipcCompMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        pComp = clHeapCalloc(1,sizeof(*pComp));
        if(pComp == NULL)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            goto out;
        }       
        pComp->compId = pTLInfo->compId;
        CL_LIST_HEAD_INIT(&pComp->portList);
        clOsalMutexLock(&gClTipcPortMutex);
        clOsalMutexLock(&gClTipcCompMutex);
    }
    else
    {
        register ClListHeadT *pTemp;
        found = CL_TRUE;
        /*Check for same port and LA combination*/
        CL_LIST_FOR_EACH(pTemp,&pComp->portList)
        {
            ClTipcCommPortT *pCommPort = CL_LIST_ENTRY(pTemp,ClTipcCommPortT,listComp);
            /*Found the port, check for the logical address*/
            if(pCommPort->portId == pTipcCommPort->portId)
            {
                register ClListHeadT *pAddressList;
                portFound = CL_TRUE;
                CL_LIST_FOR_EACH(pAddressList,&pCommPort->logicalAddressList)
                {
                    pTipcLogicalAddress = CL_LIST_ENTRY(pAddressList,ClTipcLogicalAddressT,list);
                    if(pTipcLogicalAddress->logicalAddress == pTLInfo->logicalAddr)
                    {
                        if(pTipcLogicalAddress->haState == pTLInfo->haState)
                        {
                            clOsalMutexUnlock(&gClTipcCompMutex);
                            clOsalMutexUnlock(&gClTipcPortMutex);
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("CompId:0x%x has already registered with LA:0x%llx,portId:0x%x\n",pTLInfo->compId,pTLInfo->logicalAddr,pTLInfo->physicalAddr.portId));
                            rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                            goto out;
                        }
                        pTipcLogicalAddress->haState = pTLInfo->haState;
                        goto out_bind;
                    }
                }
                break;
            }
        }
    }
    pTipcLogicalAddress = clHeapCalloc(1,sizeof(*pTipcLogicalAddress));
    if(pTipcLogicalAddress == NULL)
    {
        clOsalMutexUnlock(&gClTipcCompMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out_free;
    }
    /*
     * Pingable for service availability with node address info.
     */
    pTipcLogicalAddress->logicalAddress = pTLInfo->logicalAddr;
    pTipcLogicalAddress->haState = pTLInfo->haState;
    pTipcLogicalAddress->pTipcCommPort = pTipcCommPort;
    if(portFound == CL_FALSE)
    {
        pTipcCommPort->pComp = pComp;
        /*Add the port to the component list*/
        clListAddTail(&pTipcCommPort->listComp,&pComp->portList);
    }

    if(found == CL_FALSE)
    {
        /*Add the comp to the hash list*/
        clTipcCompHashAdd(pComp);
    }
    /*Add to the port list*/
    clListAddTail(&pTipcLogicalAddress->list,&pTipcCommPort->logicalAddressList);
    pTipcCommPort->activeBind = CL_FALSE;
    if(pTLInfo->haState == CL_IOC_TL_STDBY)
    {
        clOsalMutexUnlock(&gClTipcCompMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_OK;
        goto out;
    }
    
    out_bind:
    rc = clIocTransparencyBind(pTipcCommPort,pTipcLogicalAddress);
    if(rc != CL_OK)
    {
        goto out_del;
    }
    clOsalMutexUnlock(&gClTipcCompMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    goto out;

    out_del:
    clListDel(&pTipcLogicalAddress->list);
    if(portFound == CL_FALSE)
    {
        clListDel(&pTipcCommPort->listComp);
        pTipcCommPort->pComp = NULL;
    }
    if(found == CL_FALSE)
    {
        clTipcCompHashDel(pComp);
    }
    clOsalMutexUnlock(&gClTipcCompMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);

    out_free:
    if(pTipcLogicalAddress)
    {
        clHeapFree(pTipcLogicalAddress);
    }
    if(found == CL_FALSE)
    {
        clHeapFree(pComp);
    }
    out:
    return rc;
}

ClRcT clIocTransparencyDeregister(ClUint32T compId)
{
    ClTipcCompT *pComp = NULL;
    ClTipcLogicalAddressT *pTipcLogicalAddress=NULL;
    ClTipcCommPortT *pCommPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pNext = NULL;
    ClListHeadT *pHead=NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    clOsalMutexLock(&gClTipcPortMutex);
    clOsalMutexLock(&gClTipcCompMutex);
    pComp = clTipcGetComp(compId);
    if(pComp == NULL)
    {
        clOsalMutexUnlock(&gClTipcCompMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        goto out;
    }
    pHead = &pComp->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClInt32T fd;
        struct sockaddr_tipc address;
        register ClListHeadT *pTempLogicalList;
        ClListHeadT *pTempLogicalNext;
        pNext = pTemp->pNext;
        pCommPort = CL_LIST_ENTRY(pTemp,ClTipcCommPortT,listComp);
        CL_ASSERT(pCommPort->pComp == pComp);
        pCommPort->pComp = NULL;
        for(pTempLogicalList = pCommPort->logicalAddressList.pNext;
                pTempLogicalList != &pCommPort->logicalAddressList;
                pTempLogicalList = pTempLogicalNext)
        {
            pTempLogicalNext = pTempLogicalList->pNext;
            pTipcLogicalAddress = CL_LIST_ENTRY(pTempLogicalList,ClTipcLogicalAddressT,list);
            clListDel(&pTipcLogicalAddress->list);
            bzero((char*)&address,sizeof(address));
            CL_ASSERT(pTipcLogicalAddress->pTipcCommPort->portId == pCommPort->portId);
            fd = pCommPort->fd;
            address.family = AF_TIPC;
            address.addrtype = TIPC_ADDR_NAME;
            address.scope = -TIPC_ZONE_SCOPE;
            address.addr.name.name.type = CL_IOC_TIPC_TYPE(pTipcLogicalAddress->logicalAddress);
            address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pTipcLogicalAddress->logicalAddress);
            address.addr.name.domain=0;
            if(bind(fd,(struct sockaddr*)&address,sizeof(address))<0)
            {
                bzero((char*)&address,sizeof(address));
                fd = pCommPort->fd;
                address.family = AF_TIPC;
                address.addrtype = TIPC_ADDR_NAME;
                address.scope = -TIPC_ZONE_SCOPE;
                address.addr.name.name.type = CL_IOC_TIPC_TYPE(pTipcLogicalAddress->logicalAddress);
                address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pTipcLogicalAddress->logicalAddress);
                address.addr.name.domain=0;
                if(bind(fd,(struct sockaddr*)&address,sizeof(address))<0)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in withdraw bind for logical address:0x%llx.errno=%d\n",pTipcLogicalAddress->logicalAddress,errno));
                }
            }
            clHeapFree(pTipcLogicalAddress);
        }
        /*Unbind the active*/
        if(pCommPort->activeBind == CL_TRUE)
        {
            struct sockaddr_tipc address;
            bzero((char*)&address,sizeof(address));
            address.family = AF_TIPC;
            address.addrtype = TIPC_ADDR_NAME;
            address.scope = -TIPC_ZONE_SCOPE;
            address.addr.name.domain = 0;
            address.addr.name.name.type = CL_IOC_TIPC_MASTER_TYPE(pCommPort->portId);
            address.addr.name.name.instance = gIocLocalBladeAddress;
            if(bind(pCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in withdraw bind for active type:0x%x.errno=%d\n",CL_IOC_TIPC_MASTER_TYPE(pCommPort->portId),
                            errno));
            }
            pCommPort->activeBind = CL_FALSE;
        }
    }
    /*Now unhash*/
    clTipcCompHashDel(pComp);
    clOsalMutexUnlock(&gClTipcCompMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    clHeapFree(pComp);
    rc = CL_OK;
out:
    return rc;
}

ClRcT clIocTransparencyLogicalToPhysicalAddrGet(ClIocLogicalAddressT
        logicalAddr,
        ClIocTLMappingT **pPhysicalAddr,
        ClUint32T *pNoEntries)
{
    NULL_CHECK(pNoEntries);
    NULL_CHECK(pPhysicalAddr);
    *pNoEntries=0;
    return CL_OK;
}


ClRcT clIocMcastIsRegistered(ClIocMcastUserInfoT *pMcastInfo)
{
    ClRcT rc;
    ClTipcCommPortT *pTipcCommPort = NULL;
    ClTipcMcastT *pMcast=NULL;
    ClTipcMcastPortT *pMcastPort = NULL;

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(!CL_IOC_MCAST_VALID(&pMcastInfo->mcastAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nERROR : Invalid multicast address:0x%llx\n",pMcastInfo->mcastAddr));
        goto out;
    }

    clOsalMutexLock(&gClTipcPortMutex);
    pTipcCommPort = clTipcGetPort(pMcastInfo->physicalAddr.portId);
    if(pTipcCommPort == NULL)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Invalid portid: 0x%x\n",pMcastInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClTipcMcastMutex);
    pMcast = clTipcGetMcast(pMcastInfo->mcastAddr);
    if(pMcast == NULL)
    {
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        goto out_unlock;
    }
    else
    {
        register ClListHeadT *pTemp = NULL;
        CL_LIST_FOR_EACH(pTemp,&pMcast->portList)
        {
            pMcastPort = CL_LIST_ENTRY(pTemp,ClTipcMcastPortT,listMcast);
            if(pMcastPort->pTipcCommPort->portId == pMcastInfo->physicalAddr.portId)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Port 0x%x is already registered for the multicast address 0x%llx\n",pMcastInfo->physicalAddr.portId, pMcastInfo->mcastAddr));
                rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                goto out_unlock;
            }
        }
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    out_unlock:
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);

    out:
    return rc;
}
    


ClRcT clIocMulticastRegister(ClIocMcastUserInfoT *pMcastUserInfo)
{
    ClRcT rc = CL_OK;
    ClTipcMcastT *pMcast=NULL;
    ClTipcMcastPortT *pMcastPort = NULL;
    ClTipcCommPortT *pTipcCommPort = NULL;
    ClBoolT found = CL_FALSE;
    struct sockaddr_tipc address;
    ClInt32T fd;

    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    clOsalMutexLock(&gClTipcPortMutex);
    pTipcCommPort = clTipcGetPort(pMcastUserInfo->physicalAddr.portId);
    if(pTipcCommPort == NULL)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid portid: 0x%x\n",pMcastUserInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClTipcMcastMutex);
    pMcast = clTipcGetMcast(pMcastUserInfo->mcastAddr);
    if(pMcast == NULL)
    {
        clOsalMutexUnlock(&gClTipcMcastMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        pMcast = clHeapCalloc(1,sizeof(*pMcast));
        if(pMcast==NULL)
        {
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
            goto out;
        }
        pMcast->mcastAddress = pMcastUserInfo->mcastAddr;
        CL_LIST_HEAD_INIT(&pMcast->portList);
        clOsalMutexLock(&gClTipcPortMutex);
        clOsalMutexLock(&gClTipcMcastMutex);
    }
    else
    {
        register ClListHeadT *pTemp = NULL;
        found = CL_TRUE;
        CL_LIST_FOR_EACH(pTemp,&pMcast->portList)
            {
                pMcastPort = CL_LIST_ENTRY(pTemp,ClTipcMcastPortT,listMcast);
                if(pMcastPort->pTipcCommPort->portId == pMcastUserInfo->physicalAddr.portId)
                {
                    clOsalMutexUnlock(&gClTipcMcastMutex);
                    clOsalMutexUnlock(&gClTipcPortMutex);
                    rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Port 0x%x already exist\n",pMcastUserInfo->physicalAddr.portId));
                    goto out;
                }
            }
    }
    pMcastPort = clHeapCalloc(1,sizeof(*pMcastPort));
    if(pMcastPort == NULL)
    {
        clOsalMutexUnlock(&gClTipcMcastMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out_free;
    }
    pMcastPort->pMcast = pMcast;
    pMcastPort->pTipcCommPort = pTipcCommPort;
    /*Add to the mcasts port list*/
    clListAddTail(&pMcastPort->listMcast,&pMcast->portList);
    /*Add this to the ports mcast list*/
    clListAddTail(&pMcastPort->listPort,&pTipcCommPort->multicastAddressList);
    /*Add to the hash linkage*/
    if(found == CL_FALSE)
    {
        clTipcMcastHashAdd(pMcast);
    }
    fd = pTipcCommPort->fd;
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    
    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = TIPC_ZONE_SCOPE;
    address.addr.name.name.type = CL_IOC_TIPC_TYPE(pMcastUserInfo->mcastAddr);
    address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pMcastUserInfo->mcastAddr);
    address.addr.name.domain = 0;

    /*Fire the bind*/
    if(bind(fd,(struct sockaddr*)&address,sizeof(address)) < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        goto out_del;
    }
    rc = CL_OK;
    goto out;
    
    out_del:
    clListDel(&pMcastPort->listPort);
    clListDel(&pMcastPort->listMcast);
    if(found == CL_FALSE)
    {
        clTipcMcastHashDel(pMcast);
    }
    out_free:
    if(found == CL_FALSE)
    {
        clHeapFree(pMcast);
    }
    if(pMcastPort)
    {
        clHeapFree(pMcastPort);
    }
    out:
    return rc;
}

ClRcT clIocMulticastDeregister(ClIocMcastUserInfoT *pMcastUserInfo)
{
    ClRcT rc = CL_OK;
    ClTipcMcastT *pMcast = NULL;
    ClTipcMcastPortT *pMcastPort = NULL;
    ClTipcCommPortT *pTipcCommPort = NULL;
    register ClListHeadT *pTemp=NULL;
    ClListHeadT *pNext=NULL;
    ClListHeadT *pHead= NULL;
    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    clOsalMutexLock(&gClTipcPortMutex);

    pTipcCommPort = clTipcGetPort(pMcastUserInfo->physicalAddr.portId);

    if(pTipcCommPort==NULL)
    {
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in getting port: 0x%x\n",pMcastUserInfo->physicalAddr.portId));
        goto out;
    }
    clOsalMutexLock(&gClTipcMcastMutex);
    pMcast = clTipcGetMcast(pMcastUserInfo->mcastAddr);
    if(pMcast==NULL)
    {
        clOsalMutexUnlock(&gClTipcMcastMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in getting mcast address:0x%llx\n",pMcastUserInfo->mcastAddr));
        goto out;
    }
    pHead = &pMcast->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClTipcMcastPortT,listMcast);
        CL_ASSERT(pMcastPort->pMcast == pMcast);
        if(pMcastPort->pTipcCommPort == pTipcCommPort)
        {
            ClInt32T fd = pTipcCommPort->fd;
            struct sockaddr_tipc address;
            bzero((char*)&address,sizeof(address));
            address.family = AF_TIPC;
            address.addrtype = TIPC_ADDR_NAME;
            address.scope = -TIPC_ZONE_SCOPE;
            address.addr.name.name.type= CL_IOC_TIPC_TYPE(pMcast->mcastAddress);
            address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(pMcast->mcastAddress);
            address.addr.name.domain = 0;
            if(bind(fd,(struct sockaddr*)&address,sizeof(address)) < 0 )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind.errno=%d\n",errno));
                clOsalMutexUnlock(&gClTipcMcastMutex);
                clOsalMutexUnlock(&gClTipcPortMutex);
                rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
                goto out;
            }
            clListDel(&pMcastPort->listMcast);
            clListDel(&pMcastPort->listPort);
            clHeapFree(pMcastPort);
            goto found;
        }
    }
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Unable to find port:0x%x in mcast list\n",pTipcCommPort->portId));
    goto out;

    found:
    rc = CL_OK;
    if(CL_LIST_HEAD_EMPTY(pHead))
    {
        /*Knock off the mcast itself*/
        clTipcMcastHashDel(pMcast);
        clHeapFree(pMcast);
    }
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);

    out:
    return rc;
}

ClRcT clIocMulticastDeregisterAll(ClIocMulticastAddressT *pMcastAddress)
{
    ClRcT rc = CL_OK;
    ClTipcCommPortT *pTipcCommPort = NULL;
    ClTipcMcastT *pMcast= NULL;
    ClTipcMcastPortT *pMcastPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead = NULL;
    ClListHeadT *pNext = NULL;
    ClIocMulticastAddressT mcastAddress;

    NULL_CHECK(pMcastAddress);
    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(!CL_IOC_MCAST_VALID(pMcastAddress))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nERROR: Invalid multicast address:0x%llx\n",*pMcastAddress));
        goto out;
    }
    mcastAddress = *pMcastAddress;
    clOsalMutexLock(&gClTipcPortMutex);
    clOsalMutexLock(&gClTipcMcastMutex);
    pMcast = clTipcGetMcast(mcastAddress);
    if(pMcast == NULL)
    {
        clOsalMutexUnlock(&gClTipcMcastMutex);
        clOsalMutexUnlock(&gClTipcPortMutex);
        rc = CL_OK;
        /*could have been ripped off from a multicast deregister*/
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in mcast get for address:0x%llx\n",mcastAddress));
        goto out;
    }
    pHead = &pMcast->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClInt32T fd;
        struct sockaddr_tipc address;
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClTipcMcastPortT,listMcast);
        pTipcCommPort = pMcastPort->pTipcCommPort;
        fd = pTipcCommPort->fd;
        bzero((char*)&address,sizeof(address));
        address.family = AF_TIPC;
        address.addrtype = TIPC_ADDR_NAME;
        address.scope = -TIPC_ZONE_SCOPE;
        address.addr.name.domain=0;
        address.addr.name.name.type = CL_IOC_TIPC_TYPE(mcastAddress);
        address.addr.name.name.instance = CL_IOC_TIPC_INSTANCE(mcastAddress);
        if(bind(fd,(struct sockaddr*)&address,sizeof(address))<0)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in bind for port:0x%x.errno=%d\n",pTipcCommPort->portId,errno));
        }
        clListDel(&pMcastPort->listMcast);
        clListDel(&pMcastPort->listPort);
        clHeapFree(pMcastPort);
    }
    clTipcMcastHashDel(pMcast);
    clHeapFree(pMcast);
    clOsalMutexUnlock(&gClTipcMcastMutex);
    clOsalMutexUnlock(&gClTipcPortMutex);
    rc = CL_OK;

    out:
    return rc;
}


static ClVersionT versionsSupported[] = {
    {'B', 0x01, 0x01}
};

static ClVersionDatabaseT versionDatabase = {
    sizeof(versionsSupported) / sizeof(ClVersionT),
    versionsSupported
};


ClRcT clIocVersionCheck(ClVersionT *pVersion)
{
    ClRcT rc;

    if (pVersion == NULL)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
            ("Passed Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n",
             pVersion->releaseCode, pVersion->majorVersion,
             pVersion->minorVersion));
    rc = clVersionVerify(&versionDatabase, pVersion);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Supported Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n",
                 pVersion->releaseCode, pVersion->majorVersion,
                 pVersion->minorVersion));
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Error : Invalid version of application. rc 0x%x\n",
                 rc));
        return rc;
    }
    return rc;
}

ClRcT clIocServerReady(ClIocAddressT *pAddress)
{
    struct sockaddr_tipc topsrv;
    struct tipc_event event;
    struct tipc_subscr subscr;
    ClUint32T type, lower, upper ;
    ClInt32T fd;
    ClInt32T ret;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
 
    NULL_CHECK(pAddress);

    switch(CL_IOC_ADDRESS_TYPE_GET(pAddress))
    {
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        type = CL_IOC_TIPC_TYPE(pAddress->iocLogicalAddress);
        lower = CL_IOC_TIPC_INSTANCE(pAddress->iocLogicalAddress);
        upper = lower;
        break;

    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        type = CL_IOC_TIPC_TYPE(pAddress->iocMulticastAddress);
        lower = CL_IOC_TIPC_INSTANCE(pAddress->iocMulticastAddress);
        upper = lower;
        break;

    default:
        goto out;
    }

    rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    fd = socket(AF_TIPC,SOCK_SEQPACKET,0);
    if(fd < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error creating socket.errno=%d\n",errno));
        goto out;
    }
    ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);
    memset(&topsrv,0,sizeof(topsrv));
    memset(&event,0,sizeof(event));
    memset(&subscr,0,sizeof(subscr));

    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;
    if(connect(fd,(struct sockaddr*)&topsrv,sizeof(topsrv))<0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in connecting to topology server.errno=%d\n",errno));
        goto out_close;
    }
    /*
     * Make a subscription filter with the address first. to see if thats
     * available.
     */
    subscr.seq.type = type;
    subscr.seq.lower = lower;
    subscr.seq.upper = upper;
    subscr.timeout = CL_TIPC_TOP_SRV_READY_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    if(send(fd, (const char *)&subscr,sizeof(subscr),0)!=sizeof(subscr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error send to topology service.errno=%d\n",errno));
        goto out_close;
    }
    if(recv(fd, (char *)&event,sizeof(event),0) != sizeof(event))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error recv from topology.errno=%d\n",errno));
        goto out_close;
    }
    if(event.event != TIPC_PUBLISHED)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Error in recv. event=%d\n",event.event));
        goto out_close;
    }

    rc = CL_OK;

    out_close:
    close(fd);

    out:
    return rc;
}



static void clIocNeighborListDel(ClListHeadT *pNeighborList)
{
    while(!CL_LIST_HEAD_EMPTY(pNeighborList))
    {
        ClListHeadT *pFirst = pNeighborList->pNext;
        ClTipcNeighborT *pNeigh = CL_LIST_ENTRY(pFirst, ClTipcNeighborT, list);
        clListDel(pFirst);
        clHeapFree(pNeigh);
    }
}

/*
 * Locate your neighbors by checking for service availability of CPM
 * in the network. through the topology service.
 */
ClRcT clIocNeighborScan(void)
{
    struct sockaddr_tipc address;
    struct tipc_subscr subscr;
    struct tipc_event event;
    ClIocNodeAddressT *pAddresses = NULL;
    ClInt32T ret;
    ClInt32T fd;
    ClInt32T numEntries=0;
    ClInt32T oldNumEntries=0;
    register ClInt32T i;
    ClRcT rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    ClInt32T batchAlloc = 4;
    ClInt32T batchAllocMask = batchAlloc-1;
    ClListHeadT tempList = CL_LIST_HEAD_INITIALIZER(tempList);
    ClListHeadT *pNext = NULL;


    fd = socket(AF_TIPC,SOCK_SEQPACKET,0);
    if(fd < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in creating socket.errno=%d\n",errno));
        goto out;
    }
    ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);


    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.addr.name.name.type = TIPC_TOP_SRV;
    address.addr.name.name.instance = TIPC_TOP_SRV;
    if(connect(fd,(struct sockaddr*)&address,sizeof(address)) < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in connect.errno=%d\n",errno));
        goto out_close;
    }


    bzero((char*)&subscr,sizeof(subscr));
    subscr.seq.type = CL_TIPC_SET_TYPE(CL_IOC_CPM_PORT);
    /*Get the entire range*/
    subscr.seq.lower = CL_IOC_MIN_NODE_ADDRESS;
    subscr.seq.upper = CL_IOC_MAX_NODE_ADDRESS;
    subscr.timeout = CL_TIPC_TOP_SRV_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    /*Send the subscription information to topology service*/
    if(send(fd, (const char *)&subscr,sizeof(subscr),0) != sizeof(subscr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in send.errno=%d\n",errno));
        goto out_close;
    }

    /*Rip off all the neighbors first*/
    clOsalMutexLock(&gClTipcNeighborList.neighborMutex);
    if(!(oldNumEntries = gClTipcNeighborList.numEntries))
    {
        clOsalMutexUnlock(&gClTipcNeighborList.neighborMutex);
        goto loop;
    }
    pNext = gClTipcNeighborList.neighborList.pNext;
    clListDelInit(&gClTipcNeighborList.neighborList);
    /*Now add back to the temp*/
    clListAddTail(&tempList,pNext);
    gClTipcNeighborList.numEntries = 0;
    clOsalMutexUnlock(&gClTipcNeighborList.neighborMutex);

    loop:
    for(;;)
    {
        ClIocNodeAddressT nodeAddress=0;
        ClInt32T bytes = recv(fd, (char*)&event,sizeof(event),0);
        if(bytes != sizeof(event))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in recv.errno=%d\n",errno));
            goto out_restore;
        }
        if(event.event == TIPC_SUBSCR_TIMEOUT)
        {
            CL_DEBUG_PRINT(CL_DEBUG_INFO,("Top srv. recv timeout\n"));
            break;
        }
        if(event.event != TIPC_PUBLISHED)
        {
            continue;
        }
        CL_ASSERT(event.found_lower == event.found_upper);
        CL_DEBUG_PRINT(CL_DEBUG_INFO,("Received publish event from %u:%u,port:0x%x,node:0x%x\n",event.found_lower,event.found_upper,event.port.ref,tipc_node(event.port.node)));
        /*We have the IOC node addresses embedded in the service instance*/
        nodeAddress = (ClIocNodeAddressT)event.found_lower;
        /*First add before committing*/
        if(!(numEntries & batchAllocMask))
        {
            pAddresses = clHeapRealloc(pAddresses,sizeof(*pAddresses)*(numEntries+batchAlloc));
            if(pAddresses==NULL)
            {
                rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
                goto out_restore;
            }
        }
        pAddresses[numEntries++] = nodeAddress;
    }
    /*Now commit the neighbors*/
    for(i = 0; i < numEntries; ++i)
    {
        clIocNeighborAdd(pAddresses[i],CL_IOC_NODE_UP);
    }
    /* 
     * We now rip off entries from the old neighbor list.
     */
    clIocNeighborListDel(&tempList);
    rc = CL_OK;
    goto out_free;

    out_restore:
    if(oldNumEntries == 0)
    {
        goto out_free;
    }
    clOsalMutexLock(&gClTipcNeighborList.neighborMutex);
    clListDelInit(&tempList);
    clListAddTail(&gClTipcNeighborList.neighborList,pNext);
    gClTipcNeighborList.numEntries = oldNumEntries;
    clOsalMutexUnlock(&gClTipcNeighborList.neighborMutex);
    out_free:
    if(pAddresses)
    {
        clHeapFree(pAddresses);
    }
    out_close:
    close(fd);
    out:
    return rc;
}

static ClTipcNeighborT *clIocNeighborFind(ClIocNodeAddressT address)
{
    register ClListHeadT *pTemp=NULL;
    ClListHeadT *pHead = &gClTipcNeighborList.neighborList;
    ClTipcNeighborT *pNeigh = NULL;
    CL_LIST_FOR_EACH(pTemp,pHead)
        {
            pNeigh = CL_LIST_ENTRY(pTemp,ClTipcNeighborT,list);
            if(pNeigh->address == address)
            {
                return pNeigh;
            }
        }
    return NULL;
}

ClRcT clIocNeighborAdd(ClIocNodeAddressT address,ClUint32T status)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
    ClTipcNeighborT *pNeigh = NULL;

    clOsalMutexLock(&gClTipcNeighborList.neighborMutex);
    pNeigh = clIocNeighborFind(address);
    clOsalMutexUnlock(&gClTipcNeighborList.neighborMutex);

    if(pNeigh)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Neighbor node:0x%x already exist\n",address));
        rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
        goto out;
    }
    pNeigh = clHeapCalloc(1,sizeof(*pNeigh));
    if(pNeigh == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in neigh add\n"));
        goto out;
    }
    pNeigh->address = address;
    pNeigh->status = status;
    clOsalMutexLock(&gClTipcNeighborList.neighborMutex);
    clListAddTail(&pNeigh->list,&gClTipcNeighborList.neighborList);
    ++gClTipcNeighborList.numEntries;
    clOsalMutexUnlock(&gClTipcNeighborList.neighborMutex);
    rc = CL_OK;
    out:
    return rc;
}


void clIocQueueNotificationUnpack(ClIocQueueNotificationT *pSrcQueue,
        ClIocQueueNotificationT *pDestQueue
        )
{
    return;
}

ClRcT clIocCompStatusEnable(ClIocPhysicalAddressT addr)
{
    return clIocCompStatusSet(addr, CL_IOC_NODE_UP);
}

void clIocMasterCacheReset(void)
{
    ClIocPhysicalAddressT compAddr = {0};
    compAddr.portId = CL_IOC_XPORT_PORT;
    clTipcMasterSegmentUpdate(compAddr);
}

#if 0
ClRcT clIocStatisticsPrint(void)
{
    return CL_OK;
}


ClRcT clIocCommPortDebug(ClIocPortT portId, ClCharT *command)
{
    return CL_OK;
}

ClRcT clIocRouteInsert(ClIocRouteParamT *pRouteInfo)
{
    return CL_OK;
}

ClRcT clIocRouteDelete(ClIocNodeAddressT destAddress, ClUint16T prefixLen)
{
    return CL_OK;
}

ClRcT clIocRouteTablePrint()
{
    return CL_OK;
}

ClRcT clIocRoutingTableFlush()
{
    return CL_OK;
}


ClRcT clIocArpTablePrint()
{
    return CL_OK;
}

ClRcT clIocArpInsert(ClIocArpParamT *pArpInfo)
{
    return CL_OK;
}

ClRcT clIocArpDelete(ClIocNodeAddressT myIocAddress, ClCharT *pXportName,
        ClCharT *linkName)
{
    return CL_OK;
}

ClRcT clIocCommPortWaterMarksGet(ClUint32T commPort, ClUint64T *pLowWaterMark,
        ClUint64T *pHighWaterMark)
{
    return CL_OK;
}

ClRcT clIocCommPortWaterMarksSet(ClUint32T commPort, ClUint32T lowWaterMark,
        ClUint32T highWaterMark)
{
    return CL_OK;
}

ClRcT clIocGeoAddressTablePrint(void)
{
    return CL_OK;
}

ClRcT clIocGeographicalAddressGet(ClIocNodeAddressT iocNodeAddr,
        ClCharT *pGeoAddr)
{
    return CL_OK;
}

ClRcT clIocGeographicalAddressSet(ClIocNodeAddressT iocNodeAddr,
        ClCharT *pGeoAddr)
{
    return CL_OK;
}

ClRcT clIocLinkStatusGet(ClCharT *pXportName, ClCharT *pLinkName,
        ClUint8T *pStatus)
{
    return CL_OK;
}

ClRcT clIocLinkStatusSet(ClCharT *pXportName, ClCharT *pLinkName,
        ClUint8T status)
{
    return CL_OK;
}

ClRcT clIocAddressForPhySlotGet(ClUint32T phySlotAddr,
        ClIocNodeAddressT *pIocNodeAddr)
{
    return CL_OK;
}

ClRcT clIocPhySlotForIocAddressGet(ClIocNodeAddressT iocNodeAddr,
        ClUint32T *pPhySlot)
{
    return CL_OK;
}

ClRcT clIocAddressForPhySlotSet(ClUint32T phySlotAddr,
        ClIocNodeAddressT iocNodeAddr)
{
    return CL_OK;
}


ClRcT clIocHeartBeatStop()
{
    return CL_OK;
}


ClRcT clIocHeartBeatStart()
{
    return CL_OK;
}

ClRcT clIocNodeQueueWaterMarksSet(ClIocQueueIdT queueId,ClWaterMarkT *pWM)
{
    return CL_OK;
}

ClRcT clIocNodeQueueSizeSet(ClIocQueueIdT queueId,ClUint32T queueSize)
{
    return CL_OK;
}

ClRcT clIocNodeQueueStatsGet(ClIocQueueStatsT *pSendQStats,
        ClIocQueueStatsT *pRecvQStats
        )
{
    return CL_OK;
}

ClRcT clIocCommPortQueueSizeSet(ClIocCommPortHandleT portId,
        ClUint32T queueSize
        )
{
    return CL_OK;
}

ClRcT clIocCommPortQueueStatsGet(ClIocCommPortHandleT portId,
        ClIocQueueStatsT *pQueueStats
        )
{
    return CL_OK;
}

ClRcT clIocNodeRepresentativeSet(ClIocPortT *pPortId)
{
    return CL_OK;
}

ClRcT clIocSessionReset(ClIocCommPortHandleT iocCommPortHdl,
        ClIocLogicalAddressT *pIocLogicalAddress)
{
    return CL_OK;
}

ClRcT clIocTransparencyDeregisterNode(ClIocNodeAddressT nodeId)
{
    return CL_OK;
}

ClRcT clIocTransparencyLayerBindingsListShow(ClUint32T contextId)
{
    return CL_OK;
}

ClRcT clIocRouteStatusChange(ClIocNodeAddressT destAddress, ClUint16T prefixLen,
        ClUint8T status)
{
    return CL_OK;
}

ClRcT clIocArpEntryStatusChange(ClIocNodeAddressT iocAddr, ClCharT *pXportName,
        ClCharT *pLinkName, ClUint8T status)
{
    return CL_OK;
}


ClRcT clIocLastErrorGet(ClIocCommPortHandleT iocCommPortHdl, ClRcT *pError,
        ClBufferHandleT userMessage)
{
    return CL_OK;
}

ClRcT clIocTransparencyLayerTablePrint()
{
    return CL_OK;
}

ClRcT clIocCommPortBlockRecvSet(ClIocCommPortHandleT portId)
{
    return CL_OK;
}

ClRcT clIocTransportStatsPrint(void)
{
    return CL_OK;
}

ClRcT clIocCommPortModeSet(ClIocCommPortHandleT iocCommPort,
        ClIocCommPortModeT modeType)
{
    return CL_OK;
}

ClRcT clIocXportSwitch(ClUint8T *pXportName, ClUint8T *pLinkName,
        ClUint8T *pLinkAddress)
{
    return CL_OK;
}


ClRcT clIocBind(ClNameT *toName, ClIocToBindHandleT *pToHandle)
{
    return CL_OK;
}


ClRcT clIocGetEntries(ClIocUserOpTypeT mapping, ClIocAddressT *pAddress,
        void **ppAddressList, ClUint32T *pNumEntries)
{
    return CL_OK;
}


/*
 * Display the entries on the screen and copy it also 
 */
ClRcT clIocDisplayEntries(ClIocUserOpTypeT mapping, ClIocAddressT *pAddress,
        void *pAddressList, ClUint32T numEntries,
        ClCharT **ppBuf)
{
    return CL_OK;
}

#endif
