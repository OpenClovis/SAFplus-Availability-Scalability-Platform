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

#include <clLogApi.hxx>
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

typedef struct
{
    ClIocFragHeaderT *header;
    ClBufferHandleT fragPkt;
}ClIocFragNodeT;

static ClRcT clIocReplicastGet(ClIocPortT portId, ClIocAddressT **pAddressList, ClUint32T *pNumEntries);

#define IOC_REASSEMBLE_HASH_BITS (12)
#define IOC_REASSEMBLE_HASH_SIZE ( 1 << IOC_REASSEMBLE_HASH_BITS)
#define IOC_REASSEMBLE_HASH_MASK (IOC_REASSEMBLE_HASH_SIZE - 1)

static ClOsalMutexT iocReassembleLock;
static ClJobQueueT iocFragmentJobQueue;
static struct hashStruct *iocReassembleHashTable[IOC_REASSEMBLE_HASH_SIZE];
static ClUint64T iocReassembleCurrentTimerId;
typedef ClIocReassemblyKeyT ClIocReassembleKeyT;

typedef struct ClIocReassembleTimerKey
{
    ClIocReassembleKeyT key;
    ClUint64T timerId;
    ClTimerHandleT reassembleTimer;
}ClIocReassembleTimerKeyT;

typedef struct ClIocReassembleNode
{
    ClRbTreeRootT reassembleTree; /*reassembly tree*/
    struct hashStruct hash; /*hash linkage*/
    ClUint32T currentLength;
    ClUint32T expectedLength;
    ClUint32T numFragments; /* number of fragments received*/
    ClIocReassembleTimerKeyT *timerKey;
}ClIocReassembleNodeT;

typedef struct ClIocFragmentNode
{
    ClRbTreeT tree;
    ClUint32T fragOffset;
    ClUint32T fragLength;
    ClUint8T *fragBuffer;
}ClIocFragmentNodeT;

typedef struct ClIocFragmentJob
{
    ClCharT xportType[CL_MAX_NAME_LENGTH];
    ClUint8T *buffer;
    ClUint32T length;
    ClIocPortT portId;
    ClIocFragHeaderT fragHeader;
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

typedef struct ClIocLogicalAddressCtrl
{
    ClIocCommPortT *pIocCommPort;
    ClIocLogicalAddressT logicalAddress;
    ClUint32T haState;
    ClListHeadT list;
}ClIocLogicalAddressCtrlT;

typedef struct ClIocMcast
{
    ClIocMulticastAddressT mcastAddress;
    struct hashStruct hash;
    ClListHeadT portList;
}ClIocMcastT;

/*port list for the mcast*/
typedef struct ClIocMcastPort
{
    ClListHeadT listMcast;
    ClListHeadT listPort;
    ClIocCommPortT *pIocCommPort;
    ClIocMcastT *pMcast;
}ClIocMcastPortT;

typedef struct ClIocNeighborList
{
    ClListHeadT neighborList;
    ClOsalMutexT neighborMutex;
    ClUint32T numEntries;
}ClIocNeighborListT;

typedef struct ClIocNeighbor
{
    ClListHeadT list;
    ClIocNodeAddressT address;
    ClUint32T status;
}ClIocNeighborT;

static ClIocNeighborListT gClIocNeighborList = { CL_LIST_HEAD_INITIALIZER(gClIocNeighborList.neighborList) };


#define CL_IOC_MICRO_SLEEP_INTERVAL 1000*10 /* 10 milli second */

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
static ClRcT clIocNeighborAdd(ClIocNodeAddressT address,ClUint32T status);
static ClIocNeighborT *clIocNeighborFind(ClIocNodeAddressT address);

/*  
 * When running with asp modified ioc supporting 64k.
 */
#undef CL_IOC_PACKET_SIZE
#define CL_IOC_PACKET_SIZE (64000)

#define longTimeDiff(tm1, tm2) ((tm2.tsSec - tm1.tsSec) * 1000 + (tm2.tsMilliSec - tm1.tsMilliSec))

#define IOC_LOG_AREA_PORT	"PORT"
#define IOC_LOG_AREA_IOC	"IOC"
#define IOC_LOG_AREA_FRAG	"FRAG"
#define IOC_LOG_AREA_MULTICAST	"MULTICAST"
#define IOC_LOG_AREA_CONFIG	"CONFIG"
#define IOC_LOG_CTX_NOTIF	"NOTIF"
#define IOC_LOG_CTX_CREATE	"CRE"
#define IOC_LOG_CTX_GET		"GET"
#define IOC_LOG_CTX_INI		"INI"
#define IOC_LOG_CTX_SEND	"SEND"
#define IOC_LOG_CTX_REPLICAST	"REPLICAST"
#define IOC_LOG_CTX_VERSION	"VER"
#define IOC_LOG_CTX_ADD		"ADD"
#define IOC_LOG_CTX_RECV	"RECV"
#define IOC_LOG_CTX_REG		"REG"
#define IOC_LOG_CTX_DEREG	"DREG"

static ClUint32T gClMaxPayloadSize = 64000;

static ClRcT internalSendSlow(ClIocCommPortT *pIocCommPort,
                          ClBufferHandleT message, 
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout, ClCharT *xportType, ClBoolT proxy);

static ClRcT internalSendSlowReplicast(ClIocCommPortT *pIocCommPort,
                          ClBufferHandleT message, 
                          ClUint32T tempPriority, 
                          ClUint32T *pTimeout,
                          ClIocAddressT *replicastList,
                          ClUint32T numReplicasts,
                          ClIocHeaderT *userHeader, ClBoolT proxy);

static ClRcT internalSend(ClIocCommPortT *pIocCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout, ClCharT *xportType, ClBoolT proxy);

static ClRcT internalSendReplicast(ClIocCommPortT *pIocCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClUint32T *pTimeout,
                          ClIocAddressT *replicastList,
                          ClUint32T numReplicasts,
                          ClIocFragHeaderT *userFragHeader, ClBoolT proxy);

static void __iocFragmentPoolPut(ClUint8T *pBuffer, ClUint32T len)
{
    if(len != iocFragmentPoolLen)
    {
        clHeapFree(pBuffer);
        return;
    }
    if(!iocFragmentPoolLimit)
    {
        iocFragmentPoolLen = gClMaxPayloadSize;
        CL_ASSERT(iocFragmentPoolLen != 0);
        iocFragmentPoolLimit = iocFragmentPoolSize/iocFragmentPoolLen;
    }
    if(iocFragmentPoolEntries >= iocFragmentPoolLimit)
    {
        clHeapFree(pBuffer);
    }
    else
    {
        ClIocFragmentPoolT *pool = (ClIocFragmentPoolT*) clHeapCalloc(1, sizeof(*pool));
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
    logTrace("IOC", "FRAG-POOL", "Got fragment of len [%d] from pool", len);
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
    iocFragmentPoolLen = gClMaxPayloadSize;
    CL_ASSERT(iocFragmentPoolLen != 0);
    clOsalMutexInit(&iocFragmentPoolLock);
    while(currentSize + iocFragmentPoolLen < iocFragmentPoolSize)
    {
        ClIocFragmentPoolT *pool = (ClIocFragmentPoolT*) clHeapCalloc(1, sizeof(*pool));
        ClUint8T *buffer = (ClUint8T*) clHeapAllocate(iocFragmentPoolLen);
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

static void __iocFragmentPoolFinalize(void)
{
    ClIocFragmentPoolT *pool = NULL;
    ClListHeadT *iter = NULL;
    while(!CL_LIST_HEAD_EMPTY(&iocFragmentPool))
    {
        iter = iocFragmentPool.pNext;
        pool = CL_LIST_ENTRY(iter, ClIocFragmentPoolT, list);
        clListDel(iter);
        if(pool->buffer)
            clHeapFree(pool->buffer);
        clHeapFree(pool);
    }
    iocFragmentPoolEntries = 0;
    iocFragmentPoolLimit = 0;
    clOsalMutexDestroy(&iocFragmentPoolLock);
}

static __inline__ ClUint32T clIocMcastHash(ClIocMulticastAddressT mcastAddress)
{
    return (ClUint32T)((ClUint32T)mcastAddress & CL_IOC_MCAST_MASK);
}

static ClRcT clIocMcastHashAdd(ClIocMcastT *pMcast)
{
    ClUint32T key = clIocMcastHash(pMcast->mcastAddress);
    return hashAdd(ppIocMcastHashTable,key,&pMcast->hash);
}

static __inline__ ClRcT clIocMcastHashDel(ClIocMcastT *pMcast)
{
    hashDel(&pMcast->hash);
    return CL_OK;
}

static ClIocMcastT *clIocGetMcast(ClIocMulticastAddressT mcastAddress)
{
    register struct hashStruct *pTemp;
    ClUint32T key = clIocMcastHash(mcastAddress);
    for(pTemp = ppIocMcastHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClIocMcastT *pMcast = hashEntry(pTemp,ClIocMcastT,hash);
        if(pMcast->mcastAddress == mcastAddress)
        {
            return pMcast;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clIocCompHash(ClUint32T compId)
{
    return  ( compId & CL_IOC_COMP_MASK);
}

static ClRcT clIocCompHashAdd(ClIocCompT *pComp)
{
    ClUint32T key = clIocCompHash(pComp->compId);
    return hashAdd(ppIocCompHashTable,key,&pComp->hash);
}

static __inline__ ClRcT clIocCompHashDel(ClIocCompT *pComp)
{
    hashDel(&pComp->hash);
    return CL_OK;
}

static ClIocCompT *clIocGetComp(ClUint32T compId)
{
    ClUint32T key = clIocCompHash(compId);
    register struct hashStruct *pTemp;
    for(pTemp = ppIocCompHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClIocCompT *pComp = hashEntry(pTemp,ClIocCompT,hash);
        if(pComp->compId == compId)
        {
            return pComp;
        }
    }
    return NULL;
}

static __inline__ ClUint32T clIocPortHash(ClIocPortT portId)
{
    return (ClUint32T)(portId & CL_IOC_PORT_MASK);
}

static ClRcT clIocPortHashAdd(ClIocCommPortT *pIocCommPort)
{
    ClUint32T key = clIocPortHash(pIocCommPort->portId);
    ClRcT rc;
    rc = hashAdd(ppIocPortHashTable,key,&pIocCommPort->hash);
    return rc;
}

static __inline__ ClRcT clIocPortHashDel(ClIocCommPortT *pIocCommPort)
{
    hashDel(&pIocCommPort->hash);
    return CL_OK;
}

static ClIocCommPortT *clIocGetPort(ClIocPortT portId)
{
    ClUint32T key = clIocPortHash(portId);
    register struct hashStruct *pTemp;
    for(pTemp = ppIocPortHashTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClIocCommPortT *pIocCommPort = hashEntry(pTemp,ClIocCommPortT,hash);
        if(pIocCommPort->portId == portId)
        {
            return pIocCommPort;
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
    ClIocCommPortT *pIocCommPort = NULL;

    clOsalMutexLock(&gClIocPortMutex);

    pIocCommPort = clIocGetPort(port);
    if(pIocCommPort == NULL)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_NOTIF,"Invalid port [0x%x] passed.\n", port);
        rc = CL_IOC_RC(CL_ERR_DOESNT_EXIST);
        goto error_out;
    }

    pIocCommPort->notify = action;

error_out:
    clOsalMutexUnlock(&gClIocPortMutex);
    return rc;
}

static ClRcT iocCommPortCreate(ClUint32T portId, ClIocCommPortFlagsT portType,
                               ClIocCommPortT *pIocCommPort, const ClCharT *xportType, 
                               ClBoolT bindFlag)
{
    ClIocPhysicalAddressT myAddress;
    ClRcT rc = CL_OK;

    NULL_CHECK(pIocCommPort);

    if(portId >= (CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS)) 
    {
        clDbgCodeError(CL_IOC_RC(CL_ERR_INVALID_PARAMETER),
                       ("Requested commport [%d] is out of range."
                        "OpenClovis ASP ports should be between [1-%d]."
                        "Application ports between [%d-%d]", 
                        portId, CL_IOC_RESERVED_PORTS, CL_IOC_RESERVED_PORTS+1,
                        CL_IOC_RESERVED_PORTS + CL_IOC_USER_RESERVED_PORTS));
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clIocCheckAndGetPortId(&portId);
    if(rc != CL_OK)
    {
        goto out;
    }

    CL_LIST_HEAD_INIT(&pIocCommPort->logicalAddressList);
    CL_LIST_HEAD_INIT(&pIocCommPort->multicastAddressList);
    pIocCommPort->portId = portId;

    clOsalMutexLock(&gClIocPortMutex);
    rc = clIocPortHashAdd(pIocCommPort);
    clOsalMutexUnlock(&gClIocPortMutex);

    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_CREATE,"Port hash add error.rc=0x%x\n",rc);
        goto out_put;
    }

    if(!bindFlag && clTransportBridgeEnabled(gIocLocalBladeAddress))
    {
        rc = clTransportListen(xportType, portId);
    }
    else
    {
        rc = clTransportBind(xportType, portId);
    }
    if(rc != CL_OK)
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

    out_del:
    clIocPortHashDel(pIocCommPort);

    out_put:
    clIocPutPortId(portId);

    out:
    return rc;
}

ClRcT clIocCommPortCreateStatic(ClUint32T portId, ClIocCommPortFlagsT portType,
                                ClIocCommPortT *pIocCommPort, const ClCharT *xportType)
{
    
    ClRcT rc = iocCommPortCreate(portId, portType, pIocCommPort, xportType, CL_TRUE);
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
        rc = CL_OK;
    return rc;
}

ClRcT clIocCommPortCreate(ClUint32T portId, ClIocCommPortFlagsT portType,
                          ClIocCommPortHandleT *pIocCommPortHandle)
{
    ClIocCommPortT *pIocCommPort = NULL;
    ClRcT rc = CL_OK;

    NULL_CHECK(pIocCommPortHandle);

    pIocCommPort = (ClIocCommPortT*) clHeapCalloc(1,sizeof(*pIocCommPort));
    CL_ASSERT(pIocCommPort != NULL);
    
    rc = iocCommPortCreate(portId, portType, pIocCommPort, NULL, CL_FALSE);
    if(rc != CL_OK)
        goto out_free;

    *pIocCommPortHandle = (ClIocCommPortHandleT)pIocCommPort;
    return rc;

    out_free:
    clHeapFree(pIocCommPort);
    *pIocCommPortHandle = 0;
    return rc;
}

static ClRcT iocCommPortDelete(ClIocCommPortT *pIocCommPort, const ClCharT *xportType,
                               ClBoolT bindFlag)
{
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead= NULL;
    ClListHeadT *pNext = NULL;
    ClIocCompT *pComp = NULL;

    /*This would withdraw all the binds*/
    if(!bindFlag && clTransportBridgeEnabled(gIocLocalBladeAddress))
    {
        clTransportListenStop(xportType, pIocCommPort->portId);
    }
    else
    {
        clTransportBindClose(xportType, pIocCommPort->portId);
    }
    clIocPutPortId(pIocCommPort->portId);

    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocCompMutex);
    if((pComp = pIocCommPort->pComp))
    {
        pIocCommPort->pComp = NULL;
        pHead = &pComp->portList;
        CL_LIST_FOR_EACH(pTemp,pHead)
        {
            ClIocCommPortT *pCommPort = CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
            if(pCommPort == pIocCommPort)
            {
                clListDel(&pCommPort->listComp);
                break;
            }
        }
        /*Check if component can be unhashed and ripped off*/
        if(CL_LIST_HEAD_EMPTY(&pComp->portList))
        {
            clIocCompHashDel(pComp);
            clHeapFree(pComp);
        }
    }
    pHead = &pIocCommPort->logicalAddressList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClIocLogicalAddressCtrlT *pLogicalAddress;
        pNext = pTemp->pNext;
        pLogicalAddress = CL_LIST_ENTRY(pTemp,ClIocLogicalAddressCtrlT,list);
        clHeapFree(pLogicalAddress);
    }

    clOsalMutexLock(&gClIocMcastMutex);
    pHead = &pIocCommPort->multicastAddressList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        ClIocMcastPortT *pMcastPort;
        pNext = pTemp->pNext;
        pMcastPort= CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listPort);
        CL_ASSERT(pMcastPort->pIocCommPort == pIocCommPort);
        clListDel(&pMcastPort->listMcast);
        clListDel(&pMcastPort->listPort);
        if(CL_LIST_HEAD_EMPTY(&pMcastPort->pMcast->portList))
        {
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

ClRcT clIocCommPortDeleteStatic(ClIocCommPortT *pIocCommPort, const ClCharT *xportType)
{
    ClRcT rc = CL_OK;
    NULL_CHECK(pIocCommPort);
    rc = iocCommPortDelete(pIocCommPort, xportType, CL_TRUE);
    clOsalMutexDestroy(&pIocCommPort->unblockMutex);
    clOsalCondDestroy(&pIocCommPort->unblockCond);
    clOsalCondDestroy(&pIocCommPort->recvUnblockCond);
    return rc;
}

ClRcT clIocCommPortDelete(ClIocCommPortHandleT portId)
{
    ClIocCommPortT *pIocCommPort  = (ClIocCommPortT*)portId;
    ClRcT rc = CL_OK;
    NULL_CHECK(pIocCommPort);
    clIocCommPortReceiverUnblock(portId);
    rc = iocCommPortDelete(pIocCommPort, NULL, CL_FALSE);
    clHeapFree(pIocCommPort);
    return rc;
}

ClRcT clIocCommPortFdGet(ClIocCommPortHandleT portHandle, const ClCharT *xportType, ClInt32T *pFd)
{
    ClIocCommPortT *pPortHandle = (ClIocCommPortT*)portHandle;
    
    if(pPortHandle == NULL)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_GET,"Error : Invalid CommPort handle passed.\n");
        return CL_IOC_RC(CL_ERR_INVALID_HANDLE);
    }

    if(pFd == NULL)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_GET,"Error : NULL parameter passed for getting the file descriptor.\n");
        return CL_IOC_RC(CL_ERR_NULL_POINTER);
    }

    return clTransportFdGet(portHandle, xportType, pFd);
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
        logError("IOC", "DECOMPRESS", "Inflate init returned with [%d]", err);
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
            logError("IOC", "DECOMPRESS", "Inflate returned [%d]", err);
            goto out_free;
        }
    } while(1);

    err = inflateEnd(&stream);
    if(err != Z_OK)
    {
        logError("IOC", "DECOMPRESS", "Inflate end returned [%d]", err);
        goto out_free;
    }
    ZLIB_TIME(t2);
    *ppDecompressedStream = decompressedStream;
    *pDecompressedStreamLen = stream.total_out;
    logNotice("IOC", "DECOMPRESS", "Inflated [%ld] bytes from [%d] bytes, [%d] decompressed streams, " \
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
        logError("IOC", "COMPRESS", "Deflate init returned [%d]", err);
        goto out_free;
    }
    stream.next_in = (Byte*)uncompressedStream;
    stream.next_out = (Byte*)compressedStream;
    stream.avail_in = uncompressedStreamLen;
    stream.avail_out = compressedStreamLen;
    err = deflate(&stream, Z_NO_FLUSH);
    if(err != Z_OK)
    {
        logError("IOC", "COMPRESS", "Deflate returned [%d]", err);
        goto out_free;
    }
    if(stream.avail_in || !stream.avail_out)
    {
        logError("IOC", "COMPRESS", "Deflate didn't work on stream len [%d] to the expected length [%d]", 
                   uncompressedStreamLen, compressedStreamLen);
        goto out_free;
    }
    err = deflate(&stream, Z_FINISH);
    if(err != Z_STREAM_END)
    {
        logError("IOC", "COMPRESS", "Deflate finish needs more output buffer. Returned [%d]", err);
        goto out_free;
    }
    err = deflateEnd(&stream);
    if(err != Z_OK)
    {
        logError("IOC", "COMPRESS", "Deflate end returned [%d]", err);
        goto out_free;
    }
    ZLIB_TIME(t2);

    *ppCompressedStream = compressedStream;
    *pCompressedStreamLen =  stream.total_out;
    logNotice("IOC", "COMPRESS", "Pending avail bytes in the output stream [%d], uncompress len [%d], "\
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
    iter->target = (struct iovec*) clHeapCalloc(iter->maxTargetVectors, sizeof(*iter->target)); 
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
    {
        numVectors = 1;
    }

    while(size > 0 && curIOVec)
    {
        if(curIOVec->iov_len == 0)
        {
            ++curIOVec;
            if( (curIOVec - iter->src) >= iter->srcVectors )
            {
                curIOVec = NULL;
            }
            continue;
        }
        size_t len = curIOVec->iov_len - iter->offset;
        ClUint8T *base = (ClUint8T*)((ClUint8T*)curIOVec->iov_base + iter->offset);
        while(size > 0 && len > 0)
        {
            size_t tgtLen = CL_MIN(size, len);
            if(numVectors >= iter->maxTargetVectors)
            {
                iter->target = (struct iovec*) clHeapRealloc(iter->target, sizeof(*iter->target) * (numVectors + 16));
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

ClRcT clIocSendWithXportRelay(ClIocCommPortHandleT commPortHandle,
                              ClBufferHandleT message, ClUint8T protoType,
                              ClIocAddressT *originAddress, ClIocAddressT *destAddress, 
                              ClIocSendOptionT *pSendOption,
                              ClCharT *xportType, ClBoolT proxy)
{
    ClRcT retCode = CL_OK, rc = CL_OK;
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *)commPortHandle;
    ClUint32T timeout = 0;
    ClUint8T priority = 0;
    ClUint32T msgLength;
    ClUint32T maxPayload, fragId, bytesRead = 0;
    ClUint32T totalFragRequired, fraction;
    ClUint32T addrType;
    ClBoolT isBcast = CL_FALSE;
    ClBoolT isPhysicalANotBroadcast = CL_FALSE;
    ClIocAddressT *replicastList = NULL;
    ClIocAddressT srcAddress = {{0}};
    ClUint32T numReplicasts = 0;
#ifdef CL_IOC_COMPRESSION
    ClUint8T *decompressedStream = NULL;
    ClUint8T *compressedStream = NULL;
    ClUint32T compressedStreamLen = 0;
    ClTimeT pktTime = 0;
    ClTimeValT tv = {0};
#endif

    ClIocAddressT interimDestAddress = {{0}};

    if (!pIocCommPort || !destAddress)
        return CL_IOC_RC(CL_ERR_NULL_POINTER);

    srcAddress.iocPhyAddress.nodeAddress = gIocLocalBladeAddress;
    srcAddress.iocPhyAddress.portId = pIocCommPort->portId;
    if(!originAddress)
    {
        originAddress = &srcAddress;
    }
    interimDestAddress = *destAddress;
    addrType = CL_IOC_ADDRESS_TYPE_GET(destAddress);
    isBcast =  (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE && 
                ( (ClIocPhysicalAddressT*)destAddress)->nodeAddress == CL_IOC_BROADCAST_ADDRESS);

    isPhysicalANotBroadcast = (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE &&
                               ( (ClIocPhysicalAddressT*)destAddress)->nodeAddress != CL_IOC_BROADCAST_ADDRESS);

    if (isBcast && gClIocReplicast) 
    {
        clIocReplicastGet(((ClIocPhysicalAddressT*) destAddress)->portId, &replicastList, &numReplicasts);
    }

    if (isPhysicalANotBroadcast) 
    {
        ClIocNodeAddressT node;
        ClUint8T status;
        /*
         * If proxy is enabled, reset as for unicasts, it would invert the logic.
         * already present to direct the traffic using the right transport
         */
        if(proxy) 
        {
            logWarning("PROXY", "SEND", "Disabling proxy sends for unicast traffic");
            proxy = CL_FALSE;
        }
        node = ((ClIocPhysicalAddressT *)destAddress)->nodeAddress;

        if (CL_IOC_RESERVED_ADDRESS == node) 
        {
            clDbgCodeError(CL_IOC_RC(CL_ERR_INVALID_PARAMETER),("Error : Invalid destination address %x:%x is passed.", 
                                                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                                                ((ClIocPhysicalAddressT *)destAddress)->portId));            
            return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
        }
    
        retCode = clIocCompStatusGet(*(ClIocPhysicalAddressT *)destAddress, &status);
        if (retCode !=  CL_OK) 
        {
            logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_SEND,"Error : Failed to get the status of the component. error code 0x%x", retCode);
            return retCode;
        }
        retCode = clFindTransport(((ClIocPhysicalAddressT*)destAddress)->nodeAddress, 
                                  &interimDestAddress, &xportType);
        if (!interimDestAddress.iocPhyAddress.nodeAddress || !xportType)
        {
            clDbgCodeError(CL_IOC_RC(CL_ERR_NOT_EXIST),
                           ("Error : Not found in destNodeLUT %x:%x is passed.",
                            ((ClIocPhysicalAddressT *)destAddress)->nodeAddress,
                            ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_ERR_NOT_EXIST);
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress && 
            interimDestAddress.iocPhyAddress.nodeAddress != node) 
        {
            if(destAddress->iocPhyAddress.portId >= CL_IOC_XPORT_PORT)
            {
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            }
            else
            {
                interimDestAddress.iocPhyAddress.portId = destAddress->iocPhyAddress.portId;
            }
            logTrace("PROXY", "SEND", "Destination through the bridge at node [%d], port [%d]",
                       interimDestAddress.iocPhyAddress.nodeAddress,
                       interimDestAddress.iocPhyAddress.portId);
            retCode = clIocCompStatusGet(interimDestAddress.iocPhyAddress, &status);
            if (retCode !=  CL_OK) 
            {
                logError(IOC_LOG_AREA_PORT,
                           IOC_LOG_CTX_SEND,
                           "Error : Failed to get the status of the component. error code 0x%x", retCode);
                return retCode;
            }
            if (status == CL_IOC_NODE_DOWN)
            {
                logError(IOC_LOG_AREA_PORT,
                           IOC_LOG_CTX_SEND,
                           "Port [0x%x] is trying to reach component [0x%x:0x%x] with [xport: %s]"
                           "but the component is not reachable.", 
                                pIocCommPort->portId,
                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                ((ClIocPhysicalAddressT *)destAddress)->portId, xportType);
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
        } 
        else 
        {
            /*
             * Check status if destination isnt through a bridge.
             */
            if(status == CL_IOC_NODE_DOWN)
            {
                logError(IOC_LOG_AREA_IOC,
                           IOC_LOG_CTX_SEND,
                           "Port [0x%x] is trying to reach component [0x%x:0x%x] "
                           "but the component is not reachable.", 
                                pIocCommPort->portId,
                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                ((ClIocPhysicalAddressT *)destAddress)->portId);
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
            interimDestAddress.iocPhyAddress.nodeAddress = ((ClIocPhysicalAddressT *)destAddress)->nodeAddress;
            interimDestAddress.iocPhyAddress.portId = ((ClIocPhysicalAddressT *)destAddress)->portId;
        }
    }

    if (pSendOption)
    {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_IOC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_SEND, 
                   "Failed to get the length of the message. error code 0x%x",retCode);
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

    if (msgLength > maxPayload)
    {
        /*
         * Fragment it to 64 K size and return
         */
        ClIocFragHeaderT userFragHeader = {{0}};
        struct iovec  header = { (void*)&userFragHeader, sizeof(userFragHeader) };
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
        
        clOsalMutexLock(gClIocFragMutex);
        fragId = currFragId;
        ++currFragId;
        clOsalMutexUnlock(gClIocFragMutex);

        userFragHeader.header.version = CL_IOC_HEADER_VERSION;
        userFragHeader.header.protocolType = protoType;
        userFragHeader.header.priority = priority;
        userFragHeader.header.flag = IOC_MORE_FRAG;
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = 
            htonl(originAddress->iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = 
            htonl(originAddress->iocPhyAddress.portId);
        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *)destAddress)->nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *)destAddress)->portId);
        userFragHeader.header.reserved = 0;
#ifdef CL_IOC_COMPRESSION
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
            logTrace(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                       "Sending id %d flag 0x%x length %d offset %d\n",
                            ntohl(userFragHeader.msgId), userFragHeader.header.flag,
                            ntohl(userFragHeader.fragLength),
                            ntohl(userFragHeader.fragOffset));
            /*
             *logNotice("FRAG", "SEND", "Sending [%d] bytes with [%d] vectors representing [%d] bytes", 
             msgLength, targetVectors, maxPayload);
            */                                                          
            if(replicastList)
            {
                retCode = internalSendReplicast(pIocCommPort, target, targetVectors, 
                                                payload + sizeof(userFragHeader),
                                                priority, &timeout, 
                                                replicastList, numReplicasts, 
                                                &userFragHeader, proxy);
            }
            else
            {
                /*logTrace(
                           "IOC",
                           "SEND",
                           "Sending to destination [%#x: %#x] with [xport: %s]",
                           interimDestAddress.iocPhyAddress.nodeAddress,
                           interimDestAddress.iocPhyAddress.portId, xportType);*/
                retCode = internalSend(pIocCommPort, target, targetVectors, 
                                       payload + sizeof(userFragHeader), priority,
                                       &interimDestAddress, &timeout,
                                       xportType, proxy);
            }
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT))
            {
                logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                           "Failed to send the message. error code = 0x%x\n", retCode);
                goto frag_error;
            }

            bytesRead += payload;
            userFragHeader.fragOffset = htonl(bytesRead);   /* updating for the
                                                             * next packet */
            --totalFragRequired;

#ifdef CL_IOC_COMPRESSION            
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
        logTrace("FRAG", "SEND", "Sending last frag at offset [%d], length [%d]",
                   ntohl(userFragHeader.fragOffset), fraction);
        if(replicastList)
        {
            retCode = internalSendReplicast(pIocCommPort, target, 
                                            targetVectors, fraction + sizeof(userFragHeader), 
                                            priority, &timeout, replicastList, 
                                            numReplicasts, &userFragHeader, proxy);
        }
        else
        {
            /*logTrace(
                  "IOC",
                  "SEND",
                  "Sending to destination [%#x: %#x] with [xport: %s]",
                  interimDestAddress.iocPhyAddress.nodeAddress,
                  interimDestAddress.iocPhyAddress.portId, xportType);
            */
            retCode = internalSend(pIocCommPort, target, targetVectors, 
                                   fraction + sizeof(userFragHeader), priority,
                                   &interimDestAddress, &timeout, 
                                   xportType, proxy);
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
        ClIocHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = 
            htonl(originAddress->iocPhyAddress.nodeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = 
            htonl(originAddress->iocPhyAddress.portId);
        userHeader.dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *)destAddress)->nodeAddress);
        userHeader.dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *)destAddress)->portId);
        userHeader.reserved = 0;

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
            userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif

        retCode =
            clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                                sizeof(ClIocHeaderT));
        if(retCode != CL_OK)	
        {
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                       "\nERROR: Prepend buffer data failed = 0x%x\n", retCode);
            goto out_free;
        }
        
        if(replicastList)
        {
            retCode = internalSendSlowReplicast(pIocCommPort, message, priority, &timeout, 
                                                replicastList, numReplicasts, 
                                                &userHeader, proxy);
        }
        else
        {
            /*logTrace(
                  "IOC",
                  "SEND",
                  "Sending to destination [%#x: %#x] with [xport: %s]",
                  interimDestAddress.iocPhyAddress.nodeAddress,
                  interimDestAddress.iocPhyAddress.portId, xportType);
            */
            retCode = internalSendSlow(pIocCommPort, message, priority, 
                                       &interimDestAddress, &timeout, 
                                       xportType, proxy);
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if(rc != CL_OK)
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                       "\nERROR: Buffer header trim failed RC = 0x%x\n", rc);

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

    out_free:
    if(replicastList)
        clHeapFree(replicastList);

    return retCode;
}

ClRcT clIocSendWithXport(ClIocCommPortHandleT commPortHandle,
                         ClBufferHandleT message, ClUint8T protoType,
                         ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption,
                         ClCharT *xportType, ClBoolT proxy)
{
    return clIocSendWithXportRelay(commPortHandle, message, protoType, NULL, destAddress, pSendOption, xportType, proxy);
}

ClRcT clIocSendWithRelay(ClIocCommPortHandleT commPortHandle,
                         ClBufferHandleT message, ClUint8T protoType,
                         ClIocAddressT *srcAddress, ClIocAddressT *destAddress, 
                         ClIocSendOptionT *pSendOption)
{
    return clIocSendWithXportRelay(commPortHandle, message, protoType, srcAddress, destAddress, pSendOption, NULL, CL_FALSE);
}

ClRcT clIocSend(ClIocCommPortHandleT commPortHandle,
                ClBufferHandleT message, ClUint8T protoType,
                ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption)
{
    return clIocSendWithXportRelay(commPortHandle, message, protoType, NULL, destAddress, pSendOption, NULL, CL_FALSE);
}

ClRcT clIocSendSlow(ClIocCommPortHandleT commPortHandle,
                    ClBufferHandleT message, ClUint8T protoType,
                    ClIocAddressT *destAddress, ClIocSendOptionT *pSendOption)
{
    ClRcT retCode, rc;
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *)commPortHandle;
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
    ClIocAddressT interimDestAddress = {{0}};
    ClCharT *xportType = NULL;

    isPhysicalANotBroadcast = (addrType == CL_IOC_PHYSICAL_ADDRESS_TYPE
                               && ((ClIocPhysicalAddressT *) destAddress)->nodeAddress != CL_IOC_BROADCAST_ADDRESS);
    if (isPhysicalANotBroadcast) {
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
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,"Error : Failed to get the status of the component. error code 0x%x", retCode);
            return retCode;
        }
        retCode = clFindTransport(((ClIocPhysicalAddressT*)destAddress)->nodeAddress, 
                                  &interimDestAddress, &xportType);
        if (!interimDestAddress.iocPhyAddress.nodeAddress || !xportType)
        {
            clDbgCodeError(CL_IOC_RC(CL_ERR_NOT_EXIST),("Error : Not found in destNodeLUT %x:%x is passed.",
                                                        ((ClIocPhysicalAddressT *)destAddress)->nodeAddress,
                                                        ((ClIocPhysicalAddressT *)destAddress)->portId));
            return CL_IOC_RC(CL_ERR_NOT_EXIST);
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress && 
            interimDestAddress.iocPhyAddress.nodeAddress != node) 
        {
            interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            retCode = clIocCompStatusGet(interimDestAddress.iocPhyAddress, &status);
            if (retCode !=  CL_OK) 
            {
                logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,"Error : Failed to get the status of the component. error code 0x%x", retCode);
                return retCode;
            }
            if (status == CL_IOC_NODE_DOWN)
            {
                logError(IOC_LOG_AREA_PORT,
                           CL_LOG_CONTEXT_UNSPECIFIED,
                           "XportType=[%s]: Port [0x%x] is trying to reach component [0x%x:0x%x] "
                           "but the component is not reachable.", xportType, pIocCommPort->portId,
                           ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, ((ClIocPhysicalAddressT *)destAddress)->portId);
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
        } 
        else 
        {
            if(status == CL_IOC_NODE_DOWN)
            {
                logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,
                                "Port [0x%x] is trying to reach component [0x%x:0x%x] "
                                "but the component is not reachable.",
                                pIocCommPort->portId,
                                ((ClIocPhysicalAddressT *)destAddress)->nodeAddress, 
                                ((ClIocPhysicalAddressT *)destAddress)->portId);
                return CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);
            }
            interimDestAddress.iocPhyAddress.nodeAddress = ((ClIocPhysicalAddressT *)destAddress)->nodeAddress;
            interimDestAddress.iocPhyAddress.portId = ((ClIocPhysicalAddressT *)destAddress)->portId;
        }
    }

    if (pSendOption)
    {
        priority = pSendOption->priority;
        timeout = pSendOption->timeout;
    }

#ifdef CL_IOC_COMPRESSION
    clTimeServerGet(NULL, &tv);
    pktTime = tv.tvSec*1000000LL + tv.tvUsec;
#endif

    retCode = clBufferLengthGet(message, &msgLength);
    if (retCode != CL_OK || msgLength == 0)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                   "Failed to get the lenght of the messege. error code 0x%x",retCode);
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

    if (msgLength > maxPayload)
    {
        /*
         * Fragment it to 64 K size and return
         */
        ClIocFragHeaderT userFragHeader = {{0}};
        ClUint32T frags = 0;
        retCode = clBufferCreate(&tempMsg);
        if (retCode != CL_OK)
        {
            logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                       "\nERROR : Message creation failed. errorCode = 0x%x\n", retCode);
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
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = htonl(pIocCommPort->portId);
        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *) destAddress)->portId);
        userFragHeader.header.reserved = 0;
#ifdef CL_IOC_COMPRESSION
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
                logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                           "\nERROR: message to message copy failed. errorCode = 0x%x\n", retCode);
                goto frag_error;
            }

            logTrace(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                      "Sending id %d flag 0x%x length %d offset %d\n",
                       ntohl(userFragHeader.msgId), userFragHeader.header.flag,
                       ntohl(userFragHeader.fragLength),
                       ntohl(userFragHeader.fragOffset));

            retCode =
                clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                                    sizeof(ClIocFragHeaderT));
            if(retCode != CL_OK)	
            {
                logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                           "\nERROR : Prepend buffer data failed. errorCode = 0x%x\n", retCode);
                goto frag_error;
            }
            if (isPhysicalANotBroadcast)
            {
                /*logTrace(
                  "IOC",
                  "SEND",
                  "Sending to destination [%#x: %#x] with [xport: %s]",
                  interimDestAddress.iocPhyAddress.nodeAddress,
                  interimDestAddress.iocPhyAddress.portId, xportType);*/

                retCode = internalSendSlow(pIocCommPort, tempMsg, priority, &interimDestAddress, 
                                           &timeout, xportType, CL_FALSE);
            }
            else
            {
                retCode = internalSendSlow(pIocCommPort, tempMsg, priority, destAddress, 
                                           &timeout, xportType, CL_FALSE);
            }
            if (retCode != CL_OK || retCode == CL_IOC_RC(CL_ERR_TIMEOUT))
            {
                logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                           "Failed to send the message. error code = 0x%x\n", retCode);
                goto frag_error;
            }

            bytesRead = bytesRead + maxPayload;
            userFragHeader.fragOffset = htonl(bytesRead);   /* updating for the
                                                             * next packet */
            retCode = clBufferClear(tempMsg);
            if(retCode != CL_OK)
                logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,"\nERROR : Failed clear the buffer. errorCode = 0x%x\n",retCode);

            totalFragRequired--;
#ifdef CL_IOC_COMPRESSION            
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
            logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                       "Error : message to message copy failed. rc = 0x%x\n", retCode);
            goto frag_error;
        }
        retCode =
            clBufferDataPrepend(tempMsg, (ClUint8T *) &userFragHeader,
                                sizeof(ClIocFragHeaderT));
        if(retCode != CL_OK)	
        {
            logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_SEND,
                       "\nERROR: Prepend buffer data failed = 0x%x\n", retCode);
            goto frag_error;
        }
        logTrace("FRAG", "SEND2", "Sending last frag at offset [%d], length [%d]",
                   ntohl(userFragHeader.fragOffset), maxPayload);
        if (isPhysicalANotBroadcast)
        {
            /*logTrace(
              "IOC",
              "SEND",
              "Sending to destination [%#x: %#x] with [xport: %s]",
              interimDestAddress.iocPhyAddress.nodeAddress,
              interimDestAddress.iocPhyAddress.portId, xportType);*/

            retCode = internalSendSlow(pIocCommPort, tempMsg, priority, &interimDestAddress, 
                                       &timeout, xportType, CL_FALSE);
        }
        else
        {
            retCode = internalSendSlow(pIocCommPort, tempMsg, priority, destAddress, &timeout, 
                                       xportType, CL_FALSE);
        }

        frag_error:
        {
            ClRcT rc;
            rc = clBufferDelete(&tempMsg);
            CL_ASSERT(rc == CL_OK);
        }
    }
    else
    {
        ClIocHeaderT userHeader = { 0 };

        userHeader.version = CL_IOC_HEADER_VERSION;
        userHeader.protocolType = protoType;
        userHeader.priority = priority;
        userHeader.srcAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        userHeader.srcAddress.iocPhyAddress.portId = htonl(pIocCommPort->portId);
        userHeader.dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *) destAddress)->nodeAddress);
        userHeader.dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *) destAddress)->portId);
        userHeader.reserved = 0;

#ifdef CL_IOC_COMPRESSION
        if(compressedStreamLen)
            userHeader.reserved = htonl(1); /*mark compression flag*/
        userHeader.pktTime = clHtonl64(pktTime);
#endif

        retCode = clBufferDataPrepend(message, (ClUint8T *) &userHeader,
                                sizeof(ClIocHeaderT));
        if(retCode != CL_OK)	
        {
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                       "\nERROR: Prepend buffer data failed = 0x%x\n", retCode);
            return retCode;
        }

        if (isPhysicalANotBroadcast)
        {
            /*logTrace(
              "IOC",
              "SEND",
              "Sending to destination [%#x: %#x] with [xport: %s]",
              interimDestAddress.iocPhyAddress.nodeAddress,
              interimDestAddress.iocPhyAddress.portId, xportType);*/

            retCode = internalSendSlow(pIocCommPort, message, priority, &interimDestAddress, 
                                       &timeout, xportType, CL_FALSE);
        } 
        else 
        {
            retCode = internalSendSlow(pIocCommPort, message, priority, destAddress, &timeout, 
                                       xportType, CL_FALSE);
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if(rc != CL_OK)
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,
                       "\nERROR: Buffer header trim failed RC = 0x%x\n", rc);

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

static ClRcT internalSend(ClIocCommPortT *pIocCommPort,
                          struct iovec *target,
                          ClUint32T targetVectors,
                          ClUint32T messageLen,
                          ClUint32T tempPriority, 
                          ClIocAddressT *pIocAddress,
                          ClUint32T *pTimeout, ClCharT *xportType, ClBoolT proxy)
{
    ClRcT rc = CL_OK;
    ClUint32T priority;
    static ClInt32T recordIOCSend = -1;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(pIocCommPort == NULL)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_SEND,"Invalid port id\n");
        goto out;
    }

    priority = CL_IOC_DEFAULT_PRIORITY;
    if(pIocCommPort->portId == CL_IOC_CPM_PORT  ||
       pIocCommPort->portId == CL_IOC_XPORT_PORT ||
       tempPriority == CL_IOC_HIGH_PRIORITY)
    {
        priority = CL_IOC_HIGH_PRIORITY;
    }

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
    
    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_TRUE);

    rc = clTransportSendProxy(xportType, pIocCommPort->portId, priority, pIocAddress,
                              target, targetVectors, 0, proxy);

    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_FALSE);

    out:
    return rc;
}

static ClRcT internalSendReplicast(ClIocCommPortT *pIocCommPort,
                                   struct iovec *target,
                                   ClUint32T targetVectors,
                                   ClUint32T messageLen,
                                   ClUint32T tempPriority, 
                                   ClUint32T *pTimeout,
                                   ClIocAddressT *replicastList,
                                   ClUint32T numReplicasts,
                                   ClIocFragHeaderT *userFragHeader, 
                                   ClBoolT proxy)
{
    ClRcT rc;
    ClUint32T i;
    ClUint32T success = 0;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    if(pIocCommPort == NULL)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_REPLICAST,"Invalid port id\n");
        goto out;
    }

    for(i = 0; i < numReplicasts; ++i)
    {
        ClIocAddressT interimDestAddress = {{0}};
        ClCharT *xportType = NULL;
        rc = clFindTransport(((ClIocPhysicalAddressT*)&replicastList[i])->nodeAddress, 
                             &interimDestAddress, &xportType);

        if (rc != CL_OK || !interimDestAddress.iocPhyAddress.nodeAddress || !xportType) {
            logError("IOC", "REPLICAST", "Replicast to destination [%#x: %#x] failed with [%#x]",
                       replicastList[i].iocPhyAddress.nodeAddress,
                       replicastList[i].iocPhyAddress.portId, rc);
            continue;
        }

        userFragHeader->header.dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *)&replicastList[i])->nodeAddress);
        userFragHeader->header.dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *)&replicastList[i])->portId);

        if (interimDestAddress.iocPhyAddress.nodeAddress
            && 
            interimDestAddress.iocPhyAddress.nodeAddress != 
            ((ClIocPhysicalAddressT*)&replicastList[i])->nodeAddress)
        {
            if(replicastList[i].iocPhyAddress.portId >= CL_IOC_XPORT_PORT)
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            else
                interimDestAddress.iocPhyAddress.portId = 
                    replicastList[i].iocPhyAddress.portId;
        } 
        else 
        {
            interimDestAddress.iocPhyAddress.nodeAddress = replicastList[i].iocPhyAddress.nodeAddress;
            interimDestAddress.iocPhyAddress.portId = replicastList[i].iocPhyAddress.portId;
        }

        /*logTrace(
                "IOC",
                "REPLICAST",
                "Sending to destination [%#x: %#x] with [xport: %s]",
                interimDestAddress.iocPhyAddress.nodeAddress,
                interimDestAddress.iocPhyAddress.portId, xportType);*/

        rc = internalSend(pIocCommPort, target, targetVectors, messageLen, tempPriority, 
                          &interimDestAddress, pTimeout, xportType, proxy);

        if(rc != CL_OK)
        {
            logError("IOC", "REPLICAST", "Replicast to destination [%#x: %#x] failed with [%#x]",
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

static ClRcT internalSendSlow(ClIocCommPortT *pIocCommPort,
                              ClBufferHandleT message, 
                              ClUint32T tempPriority, 
                              ClIocAddressT *pIocAddress,
                              ClUint32T *pTimeout, ClCharT *xportType, 
                              ClBoolT proxy)
{
    ClRcT rc = CL_OK;
    struct iovec *pIOVector = NULL;
    ClInt32T ioVectorLen = 0;
    ClUint32T priority;
    static ClInt32T recordIOCSend = -1;

    rc = CL_IOC_RC(CL_ERR_NULL_POINTER);

    if(pIocCommPort == NULL)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,"Invalid port id\n");
        goto out;
    }

    rc = clBufferVectorize(message,&pIOVector,&ioVectorLen);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_PORT,IOC_LOG_CTX_SEND,"Error in buffer vectorize.rc=0x%x\n",rc);
        goto out;
    }
    priority = CL_IOC_DEFAULT_PRIORITY;
    if(pIocCommPort->portId == CL_IOC_CPM_PORT  ||
       pIocCommPort->portId == CL_IOC_XPORT_PORT ||
       tempPriority == CL_IOC_HIGH_PRIORITY)
    {
        priority = CL_IOC_HIGH_PRIORITY;
    }

    /*
     * If the leaky bucket is defined, then 
     * respect the traffic signal
     */
    if(gClIocTrafficShaper && gClLeakyBucket)
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
    
    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_TRUE);

    rc = clTransportSendProxy(xportType, pIocCommPort->portId, priority, pIocAddress,
                              pIOVector, ioVectorLen, 0, proxy);

    if(recordIOCSend) clTaskPoolRecordIOCSend(CL_FALSE);

    clHeapFree(pIOVector);

    out:
    return rc;
}

static ClRcT internalSendSlowReplicast(ClIocCommPortT *pIocCommPort,
                                       ClBufferHandleT message, 
                                       ClUint32T tempPriority, 
                                       ClUint32T *pTimeout,
                                       ClIocAddressT *replicastList,
                                       ClUint32T numReplicasts, 
                                       ClIocHeaderT *userHeader, ClBoolT proxy)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_NULL_POINTER);
    ClUint32T i;
    ClUint32T success = 0;
    ClUint32T msgLength;

    rc = clBufferLengthGet(message, &msgLength);
    if (rc != CL_OK || msgLength == 0)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_REPLICAST, "Failed to get the length of the message. error code 0x%x", rc);
        rc = CL_IOC_RC(CL_ERR_INVALID_BUFFER);
        goto out;
    }

    if(!pIocCommPort)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_REPLICAST,"Invalid port\n");
        goto out;
    }
    for(i = 0; i < numReplicasts; ++i)
    {
        ClIocAddressT interimDestAddress = {{0}};
        ClCharT *xportType = NULL;
        rc = clFindTransport(((ClIocPhysicalAddressT*)&replicastList[i])->nodeAddress, &interimDestAddress, &xportType);

        if (rc != CL_OK || !interimDestAddress.iocPhyAddress.nodeAddress || !xportType) {
            logError("IOC", "REPLICAST", "Replicast to destination [%#x: %#x] failed with [%#x]",
                       replicastList[i].iocPhyAddress.nodeAddress,
                       replicastList[i].iocPhyAddress.portId, rc);
            continue;
        }

        rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));
        if(rc != CL_OK) {
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_REPLICAST, "\nERROR: Buffer header trim failed RC = 0x%x\n", rc);
            continue;
        }

        userHeader->dstAddress.iocPhyAddress.nodeAddress = 
            htonl(((ClIocPhysicalAddressT *)&replicastList[i])->nodeAddress);
        userHeader->dstAddress.iocPhyAddress.portId = 
            htonl(((ClIocPhysicalAddressT *)&replicastList[i])->portId);

        rc = clBufferDataPrepend(message, (ClUint8T *) userHeader, sizeof(ClIocHeaderT));
        if(rc != CL_OK)
        {
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_REPLICAST, "\nERROR: Prepend buffer data failed = 0x%x\n", rc);
            continue;
        }

        if (interimDestAddress.iocPhyAddress.nodeAddress
            && 
            interimDestAddress.iocPhyAddress.nodeAddress != 
            ((ClIocPhysicalAddressT*) &replicastList[i])->nodeAddress) 
        {
            if(replicastList[i].iocPhyAddress.portId >= CL_IOC_XPORT_PORT)
            {
                interimDestAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;
            }
            else
            {
                interimDestAddress.iocPhyAddress.portId = replicastList[i].iocPhyAddress.portId;
            }
        } 
        else 
        {
            interimDestAddress.iocPhyAddress.nodeAddress = replicastList[i].iocPhyAddress.nodeAddress;
            interimDestAddress.iocPhyAddress.portId = replicastList[i].iocPhyAddress.portId;
        }

        /*logTrace(
                "IOC",
                "REPLICAST",
                "Sending to destination [%#x: %#x] with [xport: %s]",
                interimDestAddress.iocPhyAddress.nodeAddress,
                interimDestAddress.iocPhyAddress.portId, xportType);*/

        rc = internalSendSlow(pIocCommPort, message, tempPriority, &interimDestAddress, pTimeout, xportType, proxy);

        if(rc != CL_OK)
        {
            logError("IOC", "REPLICAST", "Slow send to destination [%#x: %#x] failed with [%#x]",
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

ClRcT clIocDispatch(const ClCharT *xportType, ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, ClUint8T *buffer,
                    ClUint32T bufSize, ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_OK;
    ClIocHeaderT userHeader = { 0 };
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT*)commPort;
    ClUint32T size = sizeof(ClIocHeaderT);
    ClUint8T *pBuffer = buffer;
    ClUint32T bytes = bufSize;
    ClBoolT relay = CL_FALSE;
    ClBoolT syncReassembly = CL_FALSE;
    static ClIocDispatchOptionT recvOption = {  CL_IOC_TIMEOUT_FOREVER, CL_FALSE };
   

#ifdef CL_IOC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    if(pIocCommPort == NULL)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Invalid ioc commport\n");
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    if(!pRecvOption)
        pRecvOption = &recvOption;

    syncReassembly = pRecvOption->sync;

    if(bytes <= size)
    {
        /*Check for port exit message*/
        if(bytes == sizeof(CL_IOC_PORT_EXIT_MESSAGE) && !strncmp((ClCharT*)buffer,CL_IOC_PORT_EXIT_MESSAGE,sizeof(CL_IOC_PORT_EXIT_MESSAGE)))
        {
            ClTimerTimeOutT waitTime = { 0, 200};
            logInfo(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV, "PORT EXIT MESSAGE received for portid:0x%x,EO [%s]\n",pIocCommPort->portId,CL_EO_NAME);
            rc = CL_IOC_RC(CL_IOC_ERR_RECV_UNBLOCKED);
            clOsalMutexLock(&pIocCommPort->unblockMutex);
            if(!pIocCommPort->blocked)
            {
                clOsalMutexUnlock(&pIocCommPort->unblockMutex);
                goto out;
            }
            clOsalCondSignal(&pIocCommPort->unblockCond);
            clOsalCondWait(&pIocCommPort->recvUnblockCond,&pIocCommPort->unblockMutex,waitTime);
            clOsalMutexUnlock(&pIocCommPort->unblockMutex);
            goto out;
        }
        clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet. " "The packet is an invalid or a corrupted one. "
                                           "Packet size if %d, rc = %x\n", bytes, rc);
        goto out;
    }

    memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClIocHeaderT));

    if(userHeader.version != CL_IOC_HEADER_VERSION)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping received packet of version [%d]. Supported version [%d]\n",
                                        userHeader.version, CL_IOC_HEADER_VERSION);
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    userHeader.srcAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
    userHeader.srcAddress.iocPhyAddress.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);
    userHeader.dstAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.dstAddress.iocPhyAddress.nodeAddress);
    userHeader.dstAddress.iocPhyAddress.portId = ntohl(userHeader.dstAddress.iocPhyAddress.portId);

    /*
     * Check to forward this message. Switch to synchronous recvs or reassembly of fragments
     */
    if(CL_IOC_ADDRESS_TYPE_GET(&userHeader.dstAddress) == CL_IOC_PHYSICAL_ADDRESS_TYPE)
    {
        if(userHeader.dstAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress
           &&
           userHeader.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS)
        {
            
            relay = CL_TRUE;
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

    if(clEoWithOutCpm
       ||
       userHeader.srcAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress)
    {
        if( (rc = clIocCompStatusSet(userHeader.srcAddress.iocPhyAddress, 
                                     CL_IOC_NODE_UP)) != CL_OK)

        {
            ClUint32T packetSize;

            packetSize = bytes - ((userHeader.flag == 0)? sizeof(ClIocHeaderT): sizeof(ClIocFragHeaderT));

            clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet."
                                               "Failed to SET the staus of the packet-sender-component "
                                               "[node 0x%x : port 0x%x]. Packet size is %d. error code 0x%x ",
                                               userHeader.srcAddress.iocPhyAddress.nodeAddress,
                                               userHeader.srcAddress.iocPhyAddress.portId, 
                                               packetSize, rc);
            rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
            goto out;
        }
    }

    if(userHeader.flag == 0)
    {
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
        if(clBufferAppendHeap(message, buffer, bytes + sizeof(ClIocHeaderT)) != CL_OK)
        {
            rc = clBufferNBytesWrite(message, pBuffer, bytes);
        }
        else
        {
            /*
             * Trim the header if the recv buffer is stitched.
             */
            rc = clBufferHeaderTrim(message, sizeof(ClIocHeaderT));

        }
#endif
        if(rc != CL_OK) 
        {
            logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet. "
                                               "Failed to write to a buffer message. "
                                               "Packet Size is %d. error code 0x%x", bytes, rc);
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
                logDebug("IOC", "RECV", "Packet round trip time [%lld] usecs for bytes [%d]",
                           pktRecvTime - pktSendTime, sentBytes);
        }
#endif

        /* 
         * Hoping that the notification packet will not exceed 64K packet size :-). 
         */
        if(pIocCommPort->notify == CL_IOC_NOTIFICATION_DISABLE &&
           (userHeader.protocolType == CL_IOC_PORT_NOTIFICATION_PROTO
            ||
            userHeader.protocolType == CL_IOC_PROTO_ARP))
        {
            clBufferClear(message);
            rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
            goto out;
        }
    } 
    else 
    {
        ClIocFragHeaderT userFragHeader;
        
        memcpy((ClPtrT)&userFragHeader,(ClPtrT)buffer, sizeof(ClIocFragHeaderT));

        pBuffer = buffer + sizeof(ClIocFragHeaderT);
        bytes -= sizeof(ClIocFragHeaderT);

        userFragHeader.msgId = ntohl(userFragHeader.msgId);
        userFragHeader.fragOffset = ntohl(userFragHeader.fragOffset);
        userFragHeader.fragLength = ntohl(userFragHeader.fragLength);
        
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.portId);

        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress =
            ntohl(userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId =
            ntohl(userFragHeader.header.dstAddress.iocPhyAddress.portId);

        logError(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_RECV,
                        "Got these values fragid %d, frag offset %d, fraglength %d, "
                        "flag %x from 0x%x:0x%x at 0x%x:0x%x\n",
                        (userFragHeader.msgId), (userFragHeader.fragOffset),
                        (userFragHeader.fragLength), userFragHeader.header.flag,
                        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress,
                        userFragHeader.header.srcAddress.iocPhyAddress.portId,
                        gIocLocalBladeAddress, pIocCommPort->portId);

        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if(userFragHeader.header.flag == IOC_LAST_FRAG)
            logTrace("FRAG", "RECV", "Got Last frag at offset [%d], size [%d], received [%d]",
                       userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(xportType, pBuffer, &userFragHeader, 
                                      pIocCommPort->portId, bytes, message, syncReassembly);
        if(rc != CL_OK)
            goto out;
    }

    /*
     * Now that the message is accumulated, forward if required to the actual destination.
     */
    if(relay)
    {
        ClIocSendOptionT sendOption;
        sendOption.priority = CL_IOC_HIGH_PRIORITY;
        sendOption.timeout = 0;
        if(userHeader.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
        {
            ClIocAddressT *bcastList = NULL;
            ClUint32T numBcasts = 0;
            /*
             * Check if we have a proxy broadcast list 
             */
            if(clTransportBroadcastListGet(xportType, &userHeader.srcAddress.iocPhyAddress,
                                           &numBcasts, &bcastList) == CL_OK)
            {
                ClUint32T i;
                for(i = 0; i < numBcasts; ++i)
                {
                    /*
                     * Broadcast proxy and continue with message processing
                     */
                    logDebug("PROXY", "RELAY", "Broadcast message from node [%d], port [%d] "
                               "to node [%d], port [%d]",
                               userHeader.srcAddress.iocPhyAddress.nodeAddress,
                               userHeader.srcAddress.iocPhyAddress.portId,
                               bcastList[i].iocPhyAddress.nodeAddress,
                               bcastList[i].iocPhyAddress.portId);
                    clIocSendWithXportRelay(commPort, message, userHeader.protocolType,
                                            &userHeader.srcAddress, &bcastList[i],
                                            &sendOption, (ClCharT*)xportType, CL_FALSE);
                    clBufferReadOffsetSet(message, 0, CL_BUFFER_SEEK_SET);
                }
                if(bcastList)
                    clHeapFree(bcastList);
            }
        }
        else
        {
            logDebug("PROXY", "RELAY", "Forward message from node [%d], port [%d] "
                       "to node [%d], port [%d]",
                       userHeader.srcAddress.iocPhyAddress.nodeAddress,
                       userHeader.srcAddress.iocPhyAddress.portId,
                       userHeader.dstAddress.iocPhyAddress.nodeAddress,
                       userHeader.dstAddress.iocPhyAddress.portId);

            clIocSendWithRelay(commPort, message, userHeader.protocolType, 
                               &userHeader.srcAddress, &userHeader.dstAddress, &sendOption);
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
    if (userHeader.protocolType == CL_IOC_PROTO_ICMP)
    {
        clIocHearBeatHealthCheckUpdate(userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, NULL);
        clBufferClear(message);
        return CL_IOC_RC(CL_ERR_TRY_AGAIN);
    }

    /*
     * Got heartbeat request from a amf component
     */
    if (userHeader.protocolType == CL_IOC_PROTO_HB)
    {
        /*
         * Reply HeartBeat message
         */
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
            userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

        ClIocSendOptionT sendOption;
        sendOption.priority = CL_IOC_HIGH_PRIORITY;
        sendOption.timeout = 200;
        clIocSend(commPort, message, CL_IOC_PROTO_ICMP, &destAddress,
                  &sendOption);
        /*
         * Clear the message buffer for re-use.
         */
        clBufferClear(message);
        return CL_IOC_RC(CL_ERR_TRY_AGAIN);
    }

    pRecvParam->length = bytes;
    pRecvParam->priority = userHeader.priority;
    pRecvParam->protoType = userHeader.protocolType;
    memcpy(&pRecvParam->srcAddr, &userHeader.srcAddress, sizeof(pRecvParam->srcAddr));

    logTrace("XPORT", "RECV",
               "Received message of size [%d] and protocolType [0x%x] from node [0x%x:0x%x]", 
               bytes, userHeader.protocolType, userHeader.srcAddress.iocPhyAddress.nodeAddress, 
               userHeader.srcAddress.iocPhyAddress.portId);

    out:
    return rc;
}

ClRcT clIocDispatchAsync(const ClCharT *xportType, ClIocPortT port, ClUint8T *buffer, ClUint32T bufSize)
{
    ClRcT rc = CL_OK;
    ClIocHeaderT userHeader = { 0 };
    ClUint32T size = sizeof(ClIocHeaderT);
    ClUint8T *pBuffer = buffer;
    ClUint32T bytes = bufSize;
    ClIocRecvParamT recvParam = {0};
    ClBufferHandleT message = 0;
    ClBoolT relay = CL_FALSE;
#ifdef CL_IOC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    rc = clBufferCreate(&message);
    if(rc != CL_OK)
        goto out;

    if(bytes <= size)
    {
        clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet. "
                                           "The packet is an invalid or a corrupted one. "
                                           "Packet size received [%d]", bytes);
        goto out;
    }

    memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClIocHeaderT));

    if(userHeader.version != CL_IOC_HEADER_VERSION)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping received packet of version [%d]. Supported version [%d]\n",
                                        userHeader.version, CL_IOC_HEADER_VERSION);
        goto out;
    }

    userHeader.srcAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
    userHeader.srcAddress.iocPhyAddress.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);
    userHeader.dstAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.dstAddress.iocPhyAddress.nodeAddress);
    userHeader.dstAddress.iocPhyAddress.portId = ntohl(userHeader.dstAddress.iocPhyAddress.portId);

    /*
     * Check to forward this message. Switch to synchronous recvs or reassembly of fragments
     */
    if(CL_IOC_ADDRESS_TYPE_GET(&userHeader.dstAddress) == CL_IOC_PHYSICAL_ADDRESS_TYPE)
    {
        if (userHeader.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS 
            &&
            userHeader.dstAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress)
        {
            relay = CL_TRUE;
            if(userHeader.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS
               &&
               !clTransportBridgeEnabled(gIocLocalBladeAddress))
            {
                relay = CL_FALSE;
            }
        }
    }

    if(clEoWithOutCpm
       ||
       userHeader.srcAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress)
    {
        if( (rc = clIocCompStatusSet(userHeader.srcAddress.iocPhyAddress, 
                                     CL_IOC_NODE_UP)) != CL_OK)

        {
            ClUint32T packetSize;

            packetSize = bytes - ((userHeader.flag == 0)? sizeof(ClIocHeaderT): sizeof(ClIocFragHeaderT));

            clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet."
                                               "Failed to SET the staus of the packet-sender-component "
                                               "[node 0x%x : port 0x%x]. Packet size is %d. error code 0x%x ",
                                               userHeader.srcAddress.iocPhyAddress.nodeAddress,
                                               userHeader.srcAddress.iocPhyAddress.portId, 
                                               packetSize, rc);
            goto out;
        }
    }

    if(userHeader.flag == 0)
    {
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

        if(rc != CL_OK) {
            clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received packet. "
                                               "Failed to write to a buffer message. "
                                               "Packet Size is %d. error code 0x%x", bytes, rc);
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
                logDebug("IOC", "RECV", "Packet round trip time [%lld] usecs for bytes [%d]",
                           pktRecvTime - pktSendTime, sentBytes);
        }
#endif
    } 
    else 
    {
        ClIocFragHeaderT userFragHeader;
        
        memcpy((ClPtrT)&userFragHeader,(ClPtrT)buffer, sizeof(ClIocFragHeaderT));

        pBuffer = buffer + sizeof(ClIocFragHeaderT);
        bytes -= sizeof(ClIocFragHeaderT);

        userFragHeader.msgId = ntohl(userFragHeader.msgId);
        userFragHeader.fragOffset = ntohl(userFragHeader.fragOffset);
        userFragHeader.fragLength = ntohl(userFragHeader.fragLength);
        
        userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.srcAddress.iocPhyAddress.portId = 
            ntohl(userFragHeader.header.srcAddress.iocPhyAddress.portId);

        userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress =
            ntohl(userFragHeader.header.dstAddress.iocPhyAddress.nodeAddress);
        userFragHeader.header.dstAddress.iocPhyAddress.portId =
            ntohl(userFragHeader.header.dstAddress.iocPhyAddress.portId);

        logTrace(IOC_LOG_AREA_FRAG,IOC_LOG_CTX_RECV,
                   "Got these values fragid %d, frag offset %d, fraglength %d, "
                   "flag %x from 0x%x:0x%x at 0x%x:0x%x\n",
                   (userFragHeader.msgId), (userFragHeader.fragOffset),
                   (userFragHeader.fragLength), userFragHeader.header.flag,
                   userFragHeader.header.srcAddress.iocPhyAddress.nodeAddress,
                   userFragHeader.header.srcAddress.iocPhyAddress.portId,
                   gIocLocalBladeAddress, port);

        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if(userFragHeader.header.flag == IOC_LAST_FRAG)
            logTrace("FRAG", "RECV", "Got Last frag at offset [%d], size [%d], received [%d]",
                       userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(xportType, pBuffer, &userFragHeader, 
                                      port, bytes, message, CL_FALSE);
        /*
         * recalculate timeouts
         */
        if (rc != CL_OK)
        {
            if(rc == CL_IOC_RC(IOC_MSG_QUEUED))
                rc = CL_OK;
            else
            {
                clLogCritical(IOC_LOG_AREA_IOC,IOC_LOG_CTX_RECV,"Dropping a received fragmented-packet. "
                                                  "Failed to reassemble the packet. Packet size is %d. "
                                                  "error code 0x%x", bytes, rc);
            }
        }
        goto out;
    }

    /*
     * Now that the message is accumulated, forward if required to the actual destination.
     */
    if(relay)
    {
        ClIocSendOptionT sendOption;
        sendOption.priority = CL_IOC_HIGH_PRIORITY;
        sendOption.timeout = 0;
        ClIocCommPortT *commPort = clIocGetPort(port);
        if (commPort) 
        {
            if(userHeader.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
            {
                ClIocAddressT *bcastList = NULL;
                ClUint32T numBcasts = 0;
                /*
                 * Check if we have a proxy broadcast list 
                 */
                if(clTransportBroadcastListGet(xportType, &userHeader.srcAddress.iocPhyAddress,
                                               &numBcasts, &bcastList) == CL_OK)
                {
                    ClUint32T i;
                    for(i = 0; i < numBcasts; ++i)
                    {
                        /*
                         * Broadcast proxy and continue with message processing
                         */
                        logDebug("PROXY", "RELAY", "Broadcast message from node [%d], port [%d] "
                                   "to node [%d], port [%d], xport [%s]",
                                   userHeader.srcAddress.iocPhyAddress.nodeAddress,
                                   userHeader.srcAddress.iocPhyAddress.portId,
                                   bcastList[i].iocPhyAddress.nodeAddress,
                                   bcastList[i].iocPhyAddress.portId, xportType);
                        clIocSendWithXportRelay((ClIocCommPortHandleT)commPort, message, userHeader.protocolType,
                                                &userHeader.srcAddress, &bcastList[i],
                                                &sendOption, (ClCharT*)xportType, CL_FALSE);
                        clBufferReadOffsetSet(message, 0, CL_BUFFER_SEEK_SET);
                    }
                    if(bcastList)
                        clHeapFree(bcastList);
                }
                goto cont;
            }
            else
            {
                logDebug("PROXY", "RELAY", "Forward message from node [%d], port [%d] "
                           "to node [%d], port [%d]",
                           userHeader.srcAddress.iocPhyAddress.nodeAddress,
                           userHeader.srcAddress.iocPhyAddress.portId,
                           userHeader.dstAddress.iocPhyAddress.nodeAddress,
                           userHeader.dstAddress.iocPhyAddress.portId);
                clIocSendWithRelay((ClIocCommPortHandleT)commPort, message, userHeader.protocolType, 
                                   &userHeader.srcAddress, &userHeader.dstAddress, &sendOption);
            }
        }
        else
        {
            logError("PROXY", "RELAY", 
                       "Unable to forward message as comm port [%d] look up failed", port);
        }
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    cont:
    /*
     * Got heartbeat reply from other local components
     */
    if (userHeader.protocolType == CL_IOC_PROTO_ICMP)
    {
        clIocHearBeatHealthCheckUpdate(userHeader.srcAddress.iocPhyAddress.nodeAddress, userHeader.srcAddress.iocPhyAddress.portId, NULL);
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    /*
     * Got heartbeat request from a amf component
     */
    if (userHeader.protocolType == CL_IOC_PROTO_HB)
    {
        /*
         * Reply HeartBeat message
         */
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress =
            userHeader.srcAddress.iocPhyAddress.nodeAddress;
        destAddress.iocPhyAddress.portId = CL_IOC_CPM_PORT;

        ClIocSendOptionT sendOption = {  CL_IOC_HIGH_PRIORITY,0,0,CL_IOC_PERSISTENT_MSG,200 };
        ClIocCommPortT *commPort = clIocGetPort(port);
        if(commPort)
        {
            clIocSend((ClIocCommPortHandleT)commPort, message, CL_IOC_PROTO_ICMP, &destAddress,
                      &sendOption);
        }
        rc = CL_IOC_RC(CL_ERR_TRY_AGAIN);
        goto out;
    }

    recvParam.length = bytes;
    recvParam.priority = userHeader.priority;
    recvParam.protoType = userHeader.protocolType;
    memcpy(&recvParam.srcAddr, &userHeader.srcAddress, sizeof(recvParam.srcAddr));
    logTrace( "XPORT", "RECV",
               "Received message of size [%d] and protocolType [0x%x] from node [0x%x:0x%x]", bytes, userHeader.protocolType, recvParam.srcAddr.iocPhyAddress.nodeAddress, recvParam.srcAddr.iocPhyAddress.portId);
    clEoEnqueueReassembleJob(message, &recvParam);
    message = 0;

    out:
    if(message)
    {
        clBufferDelete(&message);
    }
    return rc;
}

ClRcT clIocReceive(ClIocCommPortHandleT commPortHdl, ClIocRecvOptionT *pRecvOption, ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClIocDispatchOptionT dispatchOption = { CL_IOC_TIMEOUT_FOREVER,  CL_TRUE};
    if(pRecvOption)
    {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, NULL, 0, message, pRecvParam);
}

ClRcT clIocReceiveAsync(ClIocCommPortHandleT commPortHdl, ClIocRecvOptionT *pRecvOption, ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClIocDispatchOptionT dispatchOption = { CL_IOC_TIMEOUT_FOREVER, CL_FALSE};
    if(pRecvOption)
    {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, NULL, 0, message, pRecvParam);
}

ClRcT clIocReceiveWithBuffer(ClIocCommPortHandleT commPortHdl, ClIocRecvOptionT *pRecvOption, ClUint8T *buffer, ClUint32T bufSize,
                             ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClIocDispatchOptionT dispatchOption = { CL_IOC_TIMEOUT_FOREVER,  CL_TRUE };
    if(!buffer) return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(pRecvOption)
    {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, buffer, bufSize, message, pRecvParam);
}

ClRcT clIocReceiveWithBufferAsync(ClIocCommPortHandleT commPortHdl, ClIocRecvOptionT *pRecvOption, ClUint8T *buffer, ClUint32T bufSize,
                                  ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClIocDispatchOptionT dispatchOption = { CL_IOC_TIMEOUT_FOREVER, CL_FALSE };
    if(!buffer) return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(pRecvOption)
    {
        dispatchOption.timeout = pRecvOption->recvTimeout;
    }
    return clTransportRecv(NULL, commPortHdl, &dispatchOption, buffer, bufSize, message, pRecvParam);
}

ClRcT clIocCommPortModeGet(ClIocCommPortHandleT iocCommPort, ClIocCommPortModeT *modeType)
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
    clIocHeartBeatFinalize(gIsNodeRepresentative);
    clTransportFinalize(NULL, gIsNodeRepresentative);
    clNodeCacheFinalize();
    clIocNeighCompsFinalize();
    __iocFragmentPoolFinalize();
    clTransportLayerFinalize();
    clOsalMutexDelete(gClIocFragMutex);
    return CL_OK;
}

static ClRcT clIocLeakyBucketInitialize(void)
{
    ClRcT rc = CL_OK;
    gClIocTrafficShaper = clParseEnvBoolean("CL_ASP_TRAFFIC_SHAPER");
    if(gClIocTrafficShaper)
    {
    char* temp;

    temp = getenv("CL_LEAKY_BUCKET_VOL"); 
    ClInt64T leakyBucketVol = temp ? (ClInt64T)atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_VOL;
    temp = getenv("CL_LEAKY_BUCKET_LEAK_SIZE");
    ClInt64T leakyBucketLeakSize = temp ? (ClInt64T)atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_LEAK_SIZE;

    ClTimerTimeOutT leakyBucketInterval;
    temp = getenv("CL_LEAKY_BUCKET_LEAK_INTERVAL");
    leakyBucketInterval.tsSec = 0;
    leakyBucketInterval.tsMilliSec = temp ? (ClInt64T)atoi(temp) : CL_LEAKY_BUCKET_DEFAULT_LEAK_INTERVAL;
        
        logInfo("LKB", "INI", "Creating a leaky bucket with vol [%lld], leak size [%lld], interval [%d ms]",leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval.tsMilliSec);
        
        rc = clLeakyBucketCreate(leakyBucketVol, leakyBucketLeakSize, leakyBucketInterval, &gClLeakyBucket);
    }
    return rc;
}

ClRcT clIocConfigInitialize(ClIocLibConfigT *pConf)
{
    ClRcT retCode;
#ifdef CL_IOC_COMPRESSION
    ClCharT *pIocCompressionBoundary = NULL;
#endif

    NULL_CHECK(pConf);

    if (gIocInit == CL_TRUE)
        return CL_OK;

    if (CL_IOC_PHYSICAL_ADDRESS_TYPE != CL_IOC_ADDRESS_TYPE_FROM_NODE_ADDRESS((pConf->nodeAddress)))
    {
        clLogCritical(IOC_LOG_AREA_CONFIG,IOC_LOG_CTX_INI,
                      "\nCritical : Invalid IOC address: Node Address [0x%x] is an invalid physical address.\n",pConf->nodeAddress);
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }
    if ((CL_IOC_RESERVED_ADDRESS == pConf->nodeAddress) || (CL_IOC_BROADCAST_ADDRESS == pConf->nodeAddress))
    {
        clLogCritical(IOC_LOG_AREA_CONFIG,IOC_LOG_CTX_INI,
                      "\nCritical : Invalid IOC address: Node Address [0x%x] is one of the reserved IOC addresses.\n ",pConf->nodeAddress);
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }
    gIocLocalBladeAddress = ((ClIocLibConfigT *) pConf)->nodeAddress;

    ASP_NODEADDR = gIocLocalBladeAddress;

    clOsalMutexCreate(&gClIocFragMutex);

    memset(&userObj, 0, sizeof(ClIocUserObjectT));

    userReassemblyTimerExpiry.tsMilliSec = CL_IOC_REASSEMBLY_TIMEOUT % 1000;
    userReassemblyTimerExpiry.tsSec = CL_IOC_REASSEMBLY_TIMEOUT / 1000;

    retCode = clIocNeighCompsInitialize(gIsNodeRepresentative);
    if(retCode != CL_OK)
    {   
        logError(IOC_LOG_AREA_CONFIG,IOC_LOG_CTX_INI,"Error : Failed at neighbor initialize. rc=0x%x\n",retCode);
        goto error_1;
    }

    retCode = clNodeCacheInitialize(gIsNodeRepresentative);
    if(retCode != CL_OK)
    {
        logError(IOC_LOG_AREA_CONFIG,IOC_LOG_CTX_INI,"Node cache initialize returned [%#x]", retCode);
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

    if(gIsNodeRepresentative == CL_TRUE)
    {
        /*
         * Initialize a debug mode time server to fetch times from remote host.
         * for debugging.
         */
        clTimeServerInitialize();
    }

    retCode = clTransportLayerInitialize();
    if(retCode != CL_OK)
    {
        goto error_2;
    }

    retCode = clTransportInitialize(NULL, gIsNodeRepresentative);
    if(retCode != CL_OK)
    {
        goto error_3;
    }

    retCode = clTransportMaxPayloadSizeGet(NULL, &gClMaxPayloadSize);
    if(retCode != CL_OK)
    {
        goto error_3;
    }

    clIocHeartBeatInitialize(gIsNodeRepresentative);

    gIocInit = CL_TRUE;
    return CL_OK;

    error_3:
    clTransportLayerFinalize();
    error_2:    
    clIocNeighCompsFinalize();
    error_1:
    clOsalMutexDelete(gClIocFragMutex);
    return retCode;
}


ClRcT clIocLibInitialize(ClPtrT pConfig)
{
    ClIocLibConfigT iocConfig = {0};
    ClRcT retCode = CL_OK;
    ClRcT rc = CL_OK;

    clTaskPoolInitialize();
    
    iocConfig.version = CL_IOC_HEADER_VERSION;
    iocConfig.nodeAddress = clAspLocalId;

    retCode = clIocConfigInitialize(&iocConfig);
    if (retCode != CL_OK)
    {
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
    rc = clJobQueueInit(&iocFragmentJobQueue, 0, 1);
    CL_ASSERT(rc == CL_OK);
    rc = __iocFragmentPoolInitialize();
    CL_ASSERT(rc == CL_OK);
    /*Add ourselves into the neighbor table*/
    gClIocNeighborList.numEntries = 0;

    CL_LIST_HEAD_INIT(&gClIocNeighborList.neighborList);

    rc = clIocNeighborAdd(gIocLocalBladeAddress,CL_IOC_NODE_UP);
    CL_ASSERT(rc == CL_OK);

    return CL_OK;
}


ClRcT clIocCommPortReceiverUnblock(ClIocCommPortHandleT portHandle)
{
    ClIocCommPortT *pIocCommPort = (ClIocCommPortT *)portHandle;
    ClRcT rc = CL_OK;
    ClUint32T portId;
    ClTimerTimeOutT timeout = { 0, 200};
    ClInt32T tries=0;
    static struct iovec exitVector = { (void*)CL_IOC_PORT_EXIT_MESSAGE,  sizeof(CL_IOC_PORT_EXIT_MESSAGE) };
    ClIocAddressT destAddress;
    portId = pIocCommPort->portId;
    memset(&destAddress, 0, sizeof(destAddress));
    destAddress.iocPhyAddress.nodeAddress = gIocLocalBladeAddress;
    destAddress.iocPhyAddress.portId = pIocCommPort->portId;

    /*Grab the lock to avoid a race with lost wakeups triggered by the recv.*/
    clOsalMutexLock(&pIocCommPort->unblockMutex);
    ++pIocCommPort->blocked;
    if( (rc = clTransportSend(NULL, pIocCommPort->portId, CL_IOC_HIGH_PRIORITY, &destAddress, &exitVector, 1, 0) ) != CL_OK)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_SEND,"Error sending port exit message to port:0x%x.errno=%d\n",portId,errno);
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        clOsalMutexUnlock(&pIocCommPort->unblockMutex);
        goto out;
    }

    while(tries++ < 3)
    {
        rc = clOsalCondWait(&pIocCommPort->unblockCond,&pIocCommPort->unblockMutex,timeout);
        if(CL_GET_ERROR_CODE(rc)==CL_ERR_TIMEOUT)
        {
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

ClRcT clIocCommPortGet(ClIocCommPortHandleT commPort, ClUint32T *portId)
{
    ClIocCommPortT *pIocCommPort= (ClIocCommPortT*)commPort;
    NULL_CHECK(portId);
    NULL_CHECK(pIocCommPort);
    *portId = pIocCommPort->portId;
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
           node->timerKey->timerId != timerId)
            continue;
        if(!memcmp(&node->timerKey->key, key, sizeof(node->timerKey->key)))
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
    ClIocReassembleTimerKeyT *timerKey = (ClIocReassembleTimerKeyT*) key;
    ClRbTreeT *fragHead = NULL;
    ClTimerHandleT timer = NULL;

    clOsalMutexLock(&iocReassembleLock);
    node = __iocReassembleNodeFind(&timerKey->key, timerKey->timerId);
    if(!node)
    {
        goto out_unlock;
    }

    logTrace("FRAG", "RECV", "Running the reassembly timer for sender node [%#x:%#x] with length [%d] bytes", 
               node->timerKey->key.sendAddr.nodeAddress, 
               node->timerKey->key.sendAddr.portId, node->currentLength);
    while( (fragHead = clRbTreeMin(&node->reassembleTree) ) )
    {
        ClIocFragmentNodeT *fragNode = CL_RBTREE_ENTRY(fragHead, ClIocFragmentNodeT, tree);
        clRbTreeDelete(&node->reassembleTree, fragHead);
        __iocFragmentPoolPut(fragNode->fragBuffer, fragNode->fragLength);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
    node->timerKey = NULL; /* reset even if freeing parent memory for GOD's debugging :) */
    clHeapFree(node);

    out_unlock:
    if( (timer = timerKey->reassembleTimer) )
    {
        timerKey->reassembleTimer = 0;
    }
    clOsalMutexUnlock(&iocReassembleLock);
    if(timer)
    {
        clTimerDelete(&timer);
    }

    clHeapFree(timerKey);
    return CL_OK;
}

static ClRcT __iocReassembleDispatch(const ClCharT *xportType, ClIocReassembleNodeT *node, 
                                     ClIocFragHeaderT *fragHeader, 
                                     ClBufferHandleT message, ClBoolT sync)
{
    ClBufferHandleT msg = 0;
    ClRcT rc = CL_OK;
    ClRcT retCode = CL_IOC_RC(IOC_MSG_QUEUED);
    ClRbTreeT *iter = NULL;
    ClUint32T len = 0;
    ClIocRecvParamT recvParam = {0};
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
        logTrace("IOC", "REASSEMBLE", "Reassembled fragment offset [%d], len [%d]",
                   fragNode->fragOffset, fragNode->fragLength);
        clRbTreeDelete(&node->reassembleTree, iter);
        clHeapFree(fragNode);
    }
    hashDel(&node->hash);
    /*
     * Atomically check and delete timer if not running.
     */
    if(clTimerCheckAndDelete(&node->timerKey->reassembleTimer) == CL_OK)
    {
        clHeapFree(node->timerKey);
        node->timerKey = NULL;
    }
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
        ClBoolT relay = CL_FALSE;
        if(CL_IOC_ADDRESS_TYPE_GET(&fragHeader->header.dstAddress) == CL_IOC_PHYSICAL_ADDRESS_TYPE)
        {
            if(fragHeader->header.dstAddress.iocPhyAddress.nodeAddress != gIocLocalBladeAddress
               &&
               fragHeader->header.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS)
            {
                relay = CL_TRUE;
                if(fragHeader->header.dstAddress.iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS
                   &&
                   !clTransportBridgeEnabled(gIocLocalBladeAddress))
                {
                    relay = CL_FALSE;
                }
            }
        }
        if(relay)
        {
            ClIocSendOptionT sendOption = { CL_IOC_HIGH_PRIORITY,0,0,CL_IOC_PERSISTENT_MSG,0 };
            ClIocPortT portId = fragHeader->header.dstAddress.iocPhyAddress.portId;
            if(portId >= CL_IOC_XPORT_PORT)
            {
                portId = CL_IOC_CPM_PORT;
            }
            ClIocCommPortT *commPort = clIocGetPort(portId);
            if (commPort) 
            {
                if(fragHeader->header.dstAddress.iocPhyAddress.nodeAddress ==
                   CL_IOC_BROADCAST_ADDRESS)
                {
                    ClIocAddressT *bcastList = NULL;
                    ClUint32T numBcasts = 0;
                    /*
                     * Check if we have a proxy broadcast list 
                     */
                    if(clTransportBroadcastListGet(xportType, 
                                                   &fragHeader->header.srcAddress.iocPhyAddress,
                                                   &numBcasts, &bcastList) == CL_OK)
                    {
                        /*
                         * Broadcast proxy and continue with message processing
                         */
                        ClUint32T i;
                        for(i = 0; i < numBcasts; ++i)
                        {
                            logDebug("PROXY", "RELAY", 
                                       "Broadcast reassembled message from node [%d], port [%d] "
                                       "to node [%d], port [%d]",
                                       fragHeader->header.srcAddress.iocPhyAddress.nodeAddress,
                                       fragHeader->header.srcAddress.iocPhyAddress.portId,
                                       bcastList[i].iocPhyAddress.nodeAddress,
                                       bcastList[i].iocPhyAddress.portId);
                            rc = clIocSendWithXportRelay((ClIocCommPortHandleT)commPort, msg, 
                                                         fragHeader->header.protocolType, 
                                                         &fragHeader->header.srcAddress, 
                                                         &bcastList[i], &sendOption,
                                                         (ClCharT*)xportType, CL_FALSE);
                            clBufferReadOffsetSet(msg, 0, CL_BUFFER_SEEK_SET);
                        }
                        if(bcastList)
                            clHeapFree(bcastList);
                    }
                    relay = CL_FALSE;
                    goto enqueue;
                }
                else
                {
                    logDebug("PROXY", "RELAY", 
                               "Forward reassembled message from [%d:%d] to node [%d:%d]", 
                               fragHeader->header.srcAddress.iocPhyAddress.nodeAddress,
                               fragHeader->header.srcAddress.iocPhyAddress.portId,
                               fragHeader->header.dstAddress.iocPhyAddress.nodeAddress,
                               fragHeader->header.dstAddress.iocPhyAddress.portId);
                    rc = clIocSendWithRelay((ClIocCommPortHandleT)commPort, msg, 
                                            fragHeader->header.protocolType, 
                                            &fragHeader->header.srcAddress, 
                                            &fragHeader->header.dstAddress, &sendOption);
                }
            }
            else
            {
                logError("PROXY", "RELAY", 
                           "Unable to forward the message from [%d:%d] to node [%d:%d] "
                           "using src port [%d]", 
                           fragHeader->header.srcAddress.iocPhyAddress.nodeAddress,
                           fragHeader->header.srcAddress.iocPhyAddress.portId,
                           fragHeader->header.dstAddress.iocPhyAddress.nodeAddress,
                           fragHeader->header.dstAddress.iocPhyAddress.portId, portId);
            }
        }
        else
        {
            enqueue:
            recvParam.length = len;
            recvParam.priority = fragHeader->header.priority;
            recvParam.protoType = fragHeader->header.protocolType;
            memcpy(&recvParam.srcAddr, &fragHeader->header.srcAddress, sizeof(recvParam.srcAddr));
            rc = clEoEnqueueReassembleJob(msg, &recvParam);
        }

        /*Delete the msg if appropriate*/
        if(relay || (rc != CL_OK))
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
    ClIocFragmentJobT *fragmentJob = (ClIocFragmentJobT*) job;
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
    if(CL_IOC_ADDRESS_TYPE_GET(&fragmentJob->fragHeader.header.dstAddress) == CL_IOC_PHYSICAL_ADDRESS_TYPE
       &&
       fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_RESERVED_ADDRESS
       &&
       fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress != CL_IOC_BROADCAST_ADDRESS)
    {
        key.destAddr.nodeAddress = fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.nodeAddress;
        key.destAddr.portId = fragmentJob->fragHeader.header.dstAddress.iocPhyAddress.portId;
    }
    key.sendAddr = fragmentJob->fragHeader.header.srcAddress.iocPhyAddress;
    node = __iocReassembleNodeFind(&key, 0);
    if(!node)
    {
        /*
         * create a new reassemble node.
         */
        ClUint32T hashKey = __iocReassembleHashKey(&key);
        ClIocReassembleTimerKeyT *timerKey = NULL;
        timerKey = (ClIocReassembleTimerKeyT*) clHeapCalloc(1, sizeof(*timerKey));
        CL_ASSERT(timerKey != NULL);
        node = (ClIocReassembleNodeT*) clHeapCalloc(1, sizeof(*node));
        CL_ASSERT(node != NULL);
        memcpy(&timerKey->key, &key, sizeof(timerKey->key)); /*safe w.r.t node deletes*/
        node->timerKey = timerKey;
        timerKey->timerId = ++iocReassembleCurrentTimerId;
        clRbTreeInit(&node->reassembleTree, __iocFragmentCmp);
        rc = clTimerCreate(userReassemblyTimerExpiry, 
                           CL_TIMER_ONE_SHOT,
                           CL_TIMER_SEPARATE_CONTEXT, __iocReassembleTimer,
                           (void *)timerKey, &timerKey->reassembleTimer);
        CL_ASSERT(rc == CL_OK);
        node->currentLength = 0;
        node->expectedLength = 0;
        node->numFragments = 0;
        hashAdd(iocReassembleHashTable, hashKey, &node->hash);
        clTimerStart(node->timerKey->reassembleTimer);
    }
    fragmentNode = (ClIocFragmentNodeT*) clHeapCalloc(1, sizeof(*fragmentNode));
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
            retCode = __iocReassembleDispatch(fragmentJob->xportType[0] ? 
                                              fragmentJob->xportType : NULL,
                                              node, &fragmentJob->fragHeader, message, sync);
        }
        else
        {
            /*
             * we update the expected length.
             */
            logTrace("FRAG", "RECV", "Out of order last fragment received for offset [%d], len [%d] bytes, "
                       "current length [%d] bytes",
                       fragmentNode->fragOffset, fragmentNode->fragLength, node->currentLength);
            node->expectedLength = fragmentNode->fragOffset + fragmentNode->fragLength;
        }
    }
    else if(node->currentLength == node->expectedLength)
    {
        retCode = __iocReassembleDispatch(fragmentJob->xportType[0] ? 
                                          fragmentJob->xportType : NULL,
                                          node, &fragmentJob->fragHeader, message, sync);
    }
    else
    {
        /*
         * Now increase the timer based on the number of fragments being received or the node length.
         * Since the sender could be doing flow control, we have an adaptive reassembly timer.
         */
        if( !( node->numFragments & CL_IOC_REASSEMBLY_FRAGMENTS_MASK ) )
        {
            logDebug("FRAG", "RECV", "Updating the reassembly timer to refire after [%d] secs for node length [%d], "
                       "num fragments [%d]",
                       userReassemblyTimerExpiry.tsSec, node->currentLength, node->numFragments);
            clTimerUpdate(node->timerKey->reassembleTimer, userReassemblyTimerExpiry);
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

ClRcT __iocUserFragmentReceive(const ClCharT *xportType,
                               ClUint8T *pBuffer,
                               ClIocFragHeaderT *userHdr,
                               ClIocPortT portId,
                               ClUint32T length,
                               ClBufferHandleT message,
                               ClBoolT sync)
{
    ClIocFragmentJobT *job = (ClIocFragmentJobT*) clHeapCalloc(1, sizeof(*job));
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
    if(xportType)
    {
        strncat(job->xportType, xportType, sizeof(job->xportType)-1);
    }
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
    addressList = (ClIocAddressT*) clHeapCalloc(CL_IOC_MAX_NODES, sizeof(*addressList));
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

static ClRcT clIocTransparencyBind(ClIocCommPortT *pIocCommPort,
                                   ClIocLogicalAddressCtrlT *pLogicalAddress
                                   )
{
    return clTransportTransparencyRegister(NULL, pIocCommPort->portId, 
                                           pLogicalAddress->logicalAddress,
                                           pLogicalAddress->haState);

}

ClRcT clIocTransparencyRegister(ClIocTLInfoT *pTLInfo)
{
    ClIocCommPortT *pIocCommPort=NULL;
    ClIocPortT portId;
    ClIocCompT *pComp = NULL;
    ClIocLogicalAddressCtrlT *pIocLogicalAddress=NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    ClBoolT found=CL_FALSE;
    ClBoolT portFound = CL_FALSE;

    NULL_CHECK(pTLInfo);

    if(CL_IOC_ADDRESS_TYPE_GET(&pTLInfo->logicalAddr) != CL_IOC_LOGICAL_ADDRESS_TYPE)
    {
        logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,
                   "\n TL: Invalid logical address:0x%llx \n",
                   pTLInfo->logicalAddr);
        goto out;
    }
    portId = pTLInfo->physicalAddr.portId;

    /*Now lookup the physical address for the ioc comm port*/
    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(portId);
    if(pIocCommPort == NULL)
    {
        clOsalMutexUnlock(&gClIocPortMutex);
        logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Invalid physical address portid:0x%x.\n",portId);
        goto out;
    }

    /*Check for an already existing compid mapping the port */
    if(pIocCommPort->pComp && pIocCommPort->pComp->compId != pTLInfo->compId)
    {
        clOsalMutexUnlock(&gClIocPortMutex);
        logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"More than 2 components cannot bind to the same port.Comp id 0x%x already bound to the port 0x%x\n",pIocCommPort->pComp->compId,pIocCommPort->portId);
        goto out;
    }

    /*Try getting the compId*/
    clOsalMutexLock(&gClIocCompMutex);
    pComp = clIocGetComp(pTLInfo->compId);
    if(pComp == NULL)
    {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        pComp = (ClIocCompT*) clHeapCalloc(1,sizeof(*pComp));
        if(pComp == NULL)
        {
            logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Error allocating memory\n");
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            goto out;
        }       
        pComp->compId = pTLInfo->compId;
        CL_LIST_HEAD_INIT(&pComp->portList);
        clOsalMutexLock(&gClIocPortMutex);
        clOsalMutexLock(&gClIocCompMutex);
    }
    else
    {
        register ClListHeadT *pTemp;
        found = CL_TRUE;
        /*Check for same port and LA combination*/
        CL_LIST_FOR_EACH(pTemp,&pComp->portList)
        {
            ClIocCommPortT *pCommPort = CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
            /*Found the port, check for the logical address*/
            if(pCommPort->portId == pIocCommPort->portId)
            {
                register ClListHeadT *pAddressList;
                portFound = CL_TRUE;
                CL_LIST_FOR_EACH(pAddressList,&pCommPort->logicalAddressList)
                {
                    pIocLogicalAddress = CL_LIST_ENTRY(pAddressList,ClIocLogicalAddressCtrlT,list);
                    if(pIocLogicalAddress->logicalAddress == pTLInfo->logicalAddr)
                    {
                        if(pIocLogicalAddress->haState == pTLInfo->haState)
                        {
                            clOsalMutexUnlock(&gClIocCompMutex);
                            clOsalMutexUnlock(&gClIocPortMutex);
                            logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"CompId:0x%x has already registered with LA:0x%llx,portId:0x%x\n",pTLInfo->compId,pTLInfo->logicalAddr,pTLInfo->physicalAddr.portId);
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
    pIocLogicalAddress = (ClIocLogicalAddressCtrlT*) clHeapCalloc(1,sizeof(*pIocLogicalAddress));
    if(pIocLogicalAddress == NULL)
    {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,"Error allocating memory\n");
        goto out_free;
    }
    /*
     * Pingable for service availability with node address info.
     */
    pIocLogicalAddress->logicalAddress = pTLInfo->logicalAddr;
    pIocLogicalAddress->haState = pTLInfo->haState;
    pIocLogicalAddress->pIocCommPort = pIocCommPort;
    if(portFound == CL_FALSE)
    {
        pIocCommPort->pComp = pComp;
        /*Add the port to the component list*/
        clListAddTail(&pIocCommPort->listComp,&pComp->portList);
    }

    if(found == CL_FALSE)
    {
        /*Add the comp to the hash list*/
        clIocCompHashAdd(pComp);
    }
    /*Add to the port list*/
    clListAddTail(&pIocLogicalAddress->list,&pIocCommPort->logicalAddressList);
    pIocCommPort->activeBind = CL_FALSE;
    if(pTLInfo->haState == CL_IOC_TL_STDBY)
    {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_OK;
        goto out;
    }
    
    out_bind:
    rc = clIocTransparencyBind(pIocCommPort,pIocLogicalAddress);
    if(rc != CL_OK)
    {
        goto out_del;
    }
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    goto out;

    out_del:
    clListDel(&pIocLogicalAddress->list);
    if(portFound == CL_FALSE)
    {
        clListDel(&pIocCommPort->listComp);
        pIocCommPort->pComp = NULL;
    }
    if(found == CL_FALSE)
    {
        clIocCompHashDel(pComp);
    }
    clOsalMutexUnlock(&gClIocCompMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out_free:
    if(pIocLogicalAddress)
    {
        clHeapFree(pIocLogicalAddress);
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
    ClIocCompT *pComp = NULL;
    ClIocLogicalAddressCtrlT *pIocLogicalAddress=NULL;
    ClIocCommPortT *pCommPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pNext = NULL;
    ClListHeadT *pHead=NULL;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocCompMutex);
    pComp = clIocGetComp(compId);
    if(pComp == NULL)
    {
        clOsalMutexUnlock(&gClIocCompMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        goto out;
    }
    pHead = &pComp->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        register ClListHeadT *pTempLogicalList;
        ClListHeadT *pTempLogicalNext;
        pNext = pTemp->pNext;
        pCommPort = CL_LIST_ENTRY(pTemp,ClIocCommPortT,listComp);
        CL_ASSERT(pCommPort->pComp == pComp);
        pCommPort->pComp = NULL;
        for(pTempLogicalList = pCommPort->logicalAddressList.pNext;
            pTempLogicalList != &pCommPort->logicalAddressList;
            pTempLogicalList = pTempLogicalNext)
        {
            pTempLogicalNext = pTempLogicalList->pNext;
            pIocLogicalAddress = CL_LIST_ENTRY(pTempLogicalList,ClIocLogicalAddressCtrlT,list);
            clListDel(&pIocLogicalAddress->list);
            CL_ASSERT(pIocLogicalAddress->pIocCommPort->portId == pCommPort->portId);
            rc = clTransportTransparencyDeregister(NULL, pCommPort->portId, pIocLogicalAddress->logicalAddress);
            if(rc != CL_OK)
            {
                logError(IOC_LOG_AREA_IOC,CL_LOG_CONTEXT_UNSPECIFIED,
                           "Logical address deregister for [0x%llx] returned with [%#x]\n",
                           pIocLogicalAddress->logicalAddress, rc);
                
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
out:
    return rc;
}

ClRcT clIocTransparencyLogicalToPhysicalAddrGet(ClIocLogicalAddressT logicalAddr,
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
    ClIocCommPortT *pIocCommPort = NULL;
    ClIocMcastT *pMcast=NULL;
    ClIocMcastPortT *pMcastPort = NULL;

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(!CL_IOC_MCAST_VALID(&pMcastInfo->mcastAddr))
    {
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"\nERROR : Invalid multicast address:0x%llx\n",pMcastInfo->mcastAddr);
        goto out;
    }

    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(pMcastInfo->physicalAddr.portId);
    if(pIocCommPort == NULL)
    {
        clOsalMutexUnlock(&gClIocPortMutex);
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"Error : Invalid portid: 0x%x\n",pMcastInfo->physicalAddr.portId);
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastInfo->mcastAddr);
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
            pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
            if(pMcastPort->pIocCommPort->portId == pMcastInfo->physicalAddr.portId)
            {
                logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,
                           "Error : Port 0x%x is already registered for the multicast address 0x%llx\n",
                           pMcastInfo->physicalAddr.portId, pMcastInfo->mcastAddr);
                rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                goto out_unlock;
            }
        }
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
    }

    out_unlock:
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out:
    return rc;
}
    
ClRcT clIocMulticastRegister(ClIocMcastUserInfoT *pMcastUserInfo)
{
    ClRcT rc = CL_OK;
    ClIocMcastT *pMcast=NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    ClIocCommPortT *pIocCommPort = NULL;
    ClBoolT found = CL_FALSE;

    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);

    if(!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr))
    {
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr);
        goto out;
    }
    clOsalMutexLock(&gClIocPortMutex);
    pIocCommPort = clIocGetPort(pMcastUserInfo->physicalAddr.portId);
    if(pIocCommPort == NULL)
    {
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"Invalid portid: 0x%x\n",pMcastUserInfo->physicalAddr.portId);
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastUserInfo->mcastAddr);
    if(pMcast == NULL)
    {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        pMcast = (ClIocMcastT*) clHeapCalloc(1,sizeof(*pMcast));
        if(pMcast==NULL)
        {
            rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
            logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"Error allocating memory\n");
            goto out;
        }
        pMcast->mcastAddress = pMcastUserInfo->mcastAddr;
        CL_LIST_HEAD_INIT(&pMcast->portList);
        clOsalMutexLock(&gClIocPortMutex);
        clOsalMutexLock(&gClIocMcastMutex);
    }
    else
    {
        register ClListHeadT *pTemp = NULL;
        found = CL_TRUE;
        CL_LIST_FOR_EACH(pTemp,&pMcast->portList)
            {
                pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
                if(pMcastPort->pIocCommPort->portId == pMcastUserInfo->physicalAddr.portId)
                {
                    clOsalMutexUnlock(&gClIocMcastMutex);
                    clOsalMutexUnlock(&gClIocPortMutex);
                    rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
                    logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"Port 0x%x already exist\n",pMcastUserInfo->physicalAddr.portId);
                    goto out;
                }
            }
    }
    pMcastPort = (ClIocMcastPortT*) clHeapCalloc(1,sizeof(*pMcastPort));
    if(pMcastPort == NULL)
    {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NO_MEMORY);
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG,"Error allocating memory\n");
        goto out_free;
    }
    pMcastPort->pMcast = pMcast;
    pMcastPort->pIocCommPort = pIocCommPort;
    /*Add to the mcasts port list*/
    clListAddTail(&pMcastPort->listMcast,&pMcast->portList);
    /*Add this to the ports mcast list*/
    clListAddTail(&pMcastPort->listPort,&pIocCommPort->multicastAddressList);
    /*Add to the hash linkage*/
    if(found == CL_FALSE)
    {
        clIocMcastHashAdd(pMcast);
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);
    
    /*Fire the bind*/
    rc = clTransportMulticastRegister(NULL, pIocCommPort->portId, pMcast->mcastAddress);
    if(rc != CL_OK)
    {
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_REG, 
                   " Multicast register for port [%#x] returned [%#x]\n",
                   pIocCommPort->portId, rc);
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
        clIocMcastHashDel(pMcast);
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
    ClIocMcastT *pMcast = NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    ClIocCommPortT *pIocCommPort = NULL;
    register ClListHeadT *pTemp=NULL;
    ClListHeadT *pNext=NULL;
    ClListHeadT *pHead= NULL;
    NULL_CHECK(pMcastUserInfo);

    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(!CL_IOC_MCAST_VALID(&pMcastUserInfo->mcastAddr))
    {
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"\nERROR: Invalid multicast address:0x%llx\n",pMcastUserInfo->mcastAddr);
        goto out;
    }
    clOsalMutexLock(&gClIocPortMutex);

    pIocCommPort = clIocGetPort(pMcastUserInfo->physicalAddr.portId);

    if(pIocCommPort==NULL)
    {
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_IOC_RC(CL_ERR_NOT_EXIST);
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Error in getting port: 0x%x\n",pMcastUserInfo->physicalAddr.portId);
        goto out;
    }
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(pMcastUserInfo->mcastAddr);
    if(pMcast==NULL)
    {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Error in getting mcast address:0x%llx\n",pMcastUserInfo->mcastAddr);
        goto out;
    }
    pHead = &pMcast->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
        CL_ASSERT(pMcastPort->pMcast == pMcast);
        if(pMcastPort->pIocCommPort == pIocCommPort)
        {
            rc = clTransportMulticastDeregister(NULL, pIocCommPort->portId, pMcast->mcastAddress);
            if(rc != CL_OK)
            {
                logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Multicast deregister for port [%#x] returned with [%#x]\n",
                                               pIocCommPort->portId, rc);
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
    logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Unable to find port:0x%x in mcast list\n",pIocCommPort->portId);
    goto out;

    found:
    rc = CL_OK;
    if(CL_LIST_HEAD_EMPTY(pHead))
    {
        /*Knock off the mcast itself*/
        clIocMcastHashDel(pMcast);
        clHeapFree(pMcast);
    }
    clOsalMutexUnlock(&gClIocMcastMutex);
    clOsalMutexUnlock(&gClIocPortMutex);

    out:
    return rc;
}

ClRcT clIocMulticastDeregisterAll(ClIocMulticastAddressT *pMcastAddress)
{
    ClRcT rc = CL_OK;
    ClIocCommPortT *pIocCommPort = NULL;
    ClIocMcastT *pMcast= NULL;
    ClIocMcastPortT *pMcastPort = NULL;
    register ClListHeadT *pTemp = NULL;
    ClListHeadT *pHead = NULL;
    ClListHeadT *pNext = NULL;
    ClIocMulticastAddressT mcastAddress;

    NULL_CHECK(pMcastAddress);
    rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    if(!CL_IOC_MCAST_VALID(pMcastAddress))
    {
        logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"\nERROR: Invalid multicast address:0x%llx\n",*pMcastAddress);
        goto out;
    }
    mcastAddress = *pMcastAddress;
    clOsalMutexLock(&gClIocPortMutex);
    clOsalMutexLock(&gClIocMcastMutex);
    pMcast = clIocGetMcast(mcastAddress);
    if(pMcast == NULL)
    {
        clOsalMutexUnlock(&gClIocMcastMutex);
        clOsalMutexUnlock(&gClIocPortMutex);
        rc = CL_OK;
        /*could have been ripped off from a multicast deregister*/
        logWarning(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Error in mcast get for address:0x%llx\n",mcastAddress);
        goto out;
    }
    pHead = &pMcast->portList;
    for(pTemp = pHead->pNext; pTemp != pHead; pTemp = pNext)
    {
        pNext = pTemp->pNext;
        pMcastPort = CL_LIST_ENTRY(pTemp,ClIocMcastPortT,listMcast);
        pIocCommPort = pMcastPort->pIocCommPort;
        if( (rc = clTransportMulticastDeregister(NULL, 
                                                 pIocCommPort->portId,
                                                 pMcast->mcastAddress) ) != CL_OK)
        {
            logError(IOC_LOG_AREA_MULTICAST,IOC_LOG_CTX_DEREG,"Multicast deregister for port [%#x] returned [%#x]\n",
                                           pIocCommPort->portId, rc);
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

    logTrace(IOC_LOG_AREA_IOC,IOC_LOG_CTX_VERSION,
               "Passed Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n",
               pVersion->releaseCode, pVersion->majorVersion,
               pVersion->minorVersion);
    rc = clVersionVerify(&versionDatabase, pVersion);
    if (rc != CL_OK)
    {
        logTrace(IOC_LOG_AREA_IOC,IOC_LOG_CTX_VERSION,
                   "Supported Version : Release = %c ; Major = 0x%x ; Minor = 0x%x\n",
                   pVersion->releaseCode, pVersion->majorVersion,
                   pVersion->minorVersion);
        logTrace(IOC_LOG_AREA_IOC,IOC_LOG_CTX_VERSION,
                   "Error : Invalid version of application. rc 0x%x\n",
                   rc);
        return rc;
    }
    return rc;
}

ClRcT clIocServerReady(ClIocAddressT *pAddress)
{
    return clTransportServerReady(NULL, pAddress);
}

static ClIocNeighborT *clIocNeighborFind(ClIocNodeAddressT address)
{
    register ClListHeadT *pTemp=NULL;
    ClListHeadT *pHead = &gClIocNeighborList.neighborList;
    ClIocNeighborT *pNeigh = NULL;
    CL_LIST_FOR_EACH(pTemp,pHead)
        {
            pNeigh = CL_LIST_ENTRY(pTemp,ClIocNeighborT,list);
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
    ClIocNeighborT *pNeigh = NULL;

    clOsalMutexLock(&gClIocNeighborList.neighborMutex);
    pNeigh = clIocNeighborFind(address);
    clOsalMutexUnlock(&gClIocNeighborList.neighborMutex);

    if(pNeigh)
    {
        logWarning(IOC_LOG_AREA_IOC,IOC_LOG_CTX_ADD,"Neighbor node:0x%x already exist\n",address);
        rc = CL_IOC_RC(CL_ERR_ALREADY_EXIST);
        goto out;
    }
    pNeigh = (ClIocNeighborT*) clHeapCalloc(1,sizeof(*pNeigh));
    if(pNeigh == NULL)
    {
        logError(IOC_LOG_AREA_IOC,IOC_LOG_CTX_ADD,"Error in neigh add\n");
        goto out;
    }
    pNeigh->address = address;
    pNeigh->status = status;
    clOsalMutexLock(&gClIocNeighborList.neighborMutex);
    clListAddTail(&pNeigh->list,&gClIocNeighborList.neighborList);
    ++gClIocNeighborList.numEntries;
    clOsalMutexUnlock(&gClIocNeighborList.neighborMutex);
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
    clIocMasterSegmentUpdate(compAddr);
}

void clIocMasterCacheSet(ClIocNodeAddressT master)
{
    ClIocPhysicalAddressT compAddr = {0};
    compAddr.portId = CL_IOC_XPORT_PORT;
    clIocMasterSegmentSet(compAddr, master);
}

static __inline__ ClRcT clIocRangeNodeAddressGet(ClIocNodeAddressT *pNodeAddress,
                                                ClInt32T start,
                                                ClInt32T end)
{
    ClInt32T step = start < end ? 1: -1;

    if(start == end) end += step;
    do
    {
        ClUint8T status = CL_IOC_NODE_DOWN;
        logInfo(IOC_LOG_AREA_IOC,IOC_LOG_CTX_GET,"Node address: %u \n",start);
        ClRcT rc = clIocRemoteNodeStatusGet((ClIocNodeAddressT)start,
                                            &status);
        if(rc == CL_OK && status == CL_IOC_NODE_UP) 
        {
            *pNodeAddress = (ClIocNodeAddressT) start;
            return CL_OK;
        }
        start += step;
    } while( start != end);

    return CL_IOC_RC(CL_ERR_NOT_EXIST);
}
                                                
ClRcT clIocHighestNodeAddressGet(ClIocNodeAddressT *pNodeAddress)
{
    NULL_CHECK(pNodeAddress);
    return clIocRangeNodeAddressGet(pNodeAddress, CL_IOC_MAX_NODE_ADDRESS-1, -1);
}

ClRcT clIocLowestNodeAddressGet(ClIocNodeAddressT *pNodeAddress)
{
    NULL_CHECK(pNodeAddress);
    return clIocRangeNodeAddressGet(pNodeAddress,  0, CL_IOC_MAX_NODE_ADDRESS);
}
