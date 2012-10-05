#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocUserApi.h>
#include <clIocGeneral.h>
#include <clTransport.h>
#include <clParserApi.h>
#include <clList.h>
#include <clHash.h>
#include <clDebugApi.h>
#include <clPluginHelper.h>
#include <clIocNeighComps.h>
#include <clJobQueue.h>
#include "clUdpSetup.h"
#include "clUdpNotification.h"

#define IOC_UDP_MAP_BITS (8)
#define IOC_UDP_MAP_SIZE (1 << IOC_UDP_MAP_BITS)
#define IOC_UDP_MAP_MASK (IOC_UDP_MAP_SIZE-1)
#define __STOP_WALK_ON_ERROR (0x1)

ClInt32T gClUdpXportId;
ClCharT  gClUdpXportType[CL_MAX_NAME_LENGTH];
static struct hashStruct *gIocUdpMap[IOC_UDP_MAP_SIZE];
static CL_LIST_HEAD_DECLARE(gIocUdpMapList);
extern ClIocNodeAddressT gIocLocalBladeAddress;
static ClPluginHelperVirtualIpAddressT gVirtualIp;
static ClBoolT gUdpInit = CL_FALSE;
ClBoolT gClSimulationMode = CL_FALSE;
static ClUint32T gClBindOffset;

typedef struct ClIocUdpMap
{
#define __ipv4_addr _addr.sin_addr
#define __ipv6_addr _addr.sin6_addr
    ClIocNodeAddressT slot;
    ClBoolT bridge;
    int family;
    int sendFd;
    union 
    {
        struct sockaddr_in sin_addr;
        struct sockaddr_in6 sin6_addr;
    } _addr;
    char addrstr[80];
    struct hashStruct hash; /*hash linkage*/
    ClListHeadT list; /*list linkage*/
}ClIocUdpMapT;

typedef struct ClIocUdpArgs
{
    struct iovec *iov;
    int iovlen;
    int flags;
    int port;
}ClIocUdpArgsT;

typedef struct ClIocUdpPrivate
{
    ClIocPortT port;
    ClInt32T fd;
}ClIocUdpPrivateT;

typedef struct ClXportCtrl
{
    ClOsalMutexT mutex;
    ClInt32T family;
}ClXportCtrlT;

typedef struct ClLinkNotificationArgs
{
    ClIocNodeAddressT node;
    ClIocNotificationIdT event;
} ClLinkNotificationArgsT;

static ClXportCtrlT gXportCtrl;
static ClJobQueueT gXportLinkNotifyQueue;

#define UDP_MAP_HASH(addr) ( (addr) & IOC_UDP_MAP_MASK )

static __inline__ void udpMapAdd(ClIocUdpMapT *map)
{
    ClUint32T hash = UDP_MAP_HASH(map->slot);
    hashAdd(gIocUdpMap, hash, &map->hash);
    clListAddTail(&map->list, &gIocUdpMapList);
}

static __inline__ void udpMapDel(ClIocUdpMapT *map)
{
    hashDel(&map->hash);
    clListDel(&map->list);
}

ClIocUdpMapT *iocUdpMapFind(ClIocNodeAddressT slot)
{
    ClUint32T hash = UDP_MAP_HASH(slot);
    register struct hashStruct *iter;
    for(iter = gIocUdpMap[hash]; iter; iter = iter->pNext)
    {
        ClIocUdpMapT *entry = hashEntry(iter, ClIocUdpMapT, hash);
        if(entry->slot == slot)
            return entry;
    }
    return NULL;
}

ClRcT clIocUdpMapDel(ClIocNodeAddressT slot)
{
    ClRcT rc = CL_OK;
    ClIocUdpMapT *map = NULL;
    if((map = iocUdpMapFind(slot)))
    {
        udpMapDel(map);
        close(map->sendFd);
        free(map);
    }
    else rc = CL_ERR_NOT_EXIST;
    return rc;
}

/*
 * Called with xport lock held. Returns with lock released.
 */
ClRcT iocUdpMapWalk(ClRcT (*callback)(ClIocUdpMapT *map, void *cookie), void *cookie, ClInt32T flags)
{
    ClIocUdpMapT *map;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK, retCode = CL_OK;
    ClIocUdpMapT *addrMap = NULL;
    ClUint32T numEntries = 0, i;
    static ClUint32T growMask = 7;

    /*
     * Accumulate the map first and then send all lockless. This would be faster
     * then playing lock/unlock futex wait/wakeup games.
     */
    for(iter = gIocUdpMapList.pNext; iter != &gIocUdpMapList; iter = iter->pNext)
    {
        map = CL_LIST_ENTRY(iter, ClIocUdpMapT, list);
        /*
         * If the entry is through a bridge, the broadcast should be proxied
         * by the bridge node.
         */
        if(map->bridge)
            continue;

        if(!(numEntries & growMask))
        {
            addrMap = clHeapRealloc(addrMap, sizeof(*addrMap) * (numEntries + growMask + 1));
            CL_ASSERT(addrMap != NULL);
        }
        memcpy(addrMap+numEntries, map, sizeof(*map));
        ++numEntries;
    }
    clOsalMutexUnlock(&gXportCtrl.mutex);

    for(i = 0; i < numEntries; ++i)
    {
        rc = callback(addrMap+i, cookie);
        if(rc != CL_OK)
        {
            retCode = rc;
            if( (flags & __STOP_WALK_ON_ERROR) )
                goto out_free;
        }
    }

    out_free:
    if(addrMap)
        clHeapFree(addrMap);

    return retCode;
}

static ClIocUdpMapT *iocUdpMapAdd(ClCharT *addr, ClIocNodeAddressT slot)
{
    ClIocUdpMapT *map = NULL;
    ClCharT addrStr[CL_MAX_NAME_LENGTH];
    ClIocNodeAddressT nodeBridge = slot;
    map = calloc(1, sizeof(*map));
    CL_ASSERT(map != NULL);
    if(!addr)
    {
        ClCharT *xportType = NULL;
        ClIocAddressT dstBridge = {{0}};
        addr = addrStr;
        /*
         * Find the bridge slot for the target and add the ip for that slot
         * as the target
         */
        if(slot != gIocLocalBladeAddress && 
           clFindTransport(slot, &dstBridge, &xportType) == CL_OK)
        {
            /*
             * Add this to the UDP map only the node transport type
             * matches ours. Else its a hybrid/multi-xport setup where
             * the node should be accessible through another transport.
             */
            if(strcmp(gClUdpXportType, xportType))
            {
                clLogInfo("UDP", "MAP", "Skipping map update as "
                          "node [%d] is through bridge [%d] and xport [%s]. "
                          "Native xport [%s]",
                          slot, dstBridge.iocPhyAddress.nodeAddress, 
                          xportType, gClUdpXportType);
                goto out_free;
            }
            nodeBridge = dstBridge.iocPhyAddress.nodeAddress;
        }
        clPluginHelperGetIpAddress(gVirtualIp.ipAddressMask, nodeBridge, addr);
        clLogTrace("UDP", "MAP", "Node [%d] at [%s] uses bridge [%d] through xport [%s]", 
                   slot, addr, nodeBridge, xportType);
    }
    map->slot = slot;
    map->bridge = nodeBridge != slot ? CL_TRUE : CL_FALSE;
    map->family = PF_INET;
    map->addrstr[0] = 0;
    strncat(map->addrstr, addr, sizeof(map->addrstr)-1);
    if(inet_pton(PF_INET, addr, (void*)&map->__ipv4_addr.sin_addr) != 1)
    {
        map->family = PF_INET6;
        if(inet_pton(PF_INET6, addr, (void*)&map->__ipv6_addr.sin6_addr) != 1)
        {
            clLogError("UDP", "MAP", "Error interpreting address [%s] for node [%d]", addr, slot);
            goto out_free;
        }
    }
    map->sendFd = socket(map->family, SOCK_DGRAM, IPPROTO_UDP);
    if(map->sendFd < 0)
    {
        clLogError("UDP", "MAP", "Socket syscall returned with error [%s] for UDP socket", strerror(errno));
        goto out_free;
    }

    udpMapAdd(map);
    clLogTrace("UDP", "MAP", "Loaded address [%s] with slot mapped at [%d]", addr, slot);
    goto out;

    out_free:
    free(map);
    map = NULL;

    out:
    return map;
}

ClRcT clIocUdpMapAdd(struct sockaddr *addr, ClIocNodeAddressT slot)
{
    ClRcT rc = CL_ERR_UNSPECIFIED;
    ClIocUdpMapT *map = NULL;
    ClCharT addrStr[INET_ADDRSTRLEN];
    struct in_addr *in4_addr = &((struct sockaddr_in*)addr)->sin_addr;
    if(!inet_ntop(PF_INET, (const void *)in4_addr, addrStr, sizeof(addrStr)))
    {
        struct in6_addr *in6_addr = &((struct sockaddr_in6*)addr)->sin6_addr;
        if(!inet_ntop(PF_INET6, (const void*)in6_addr, addrStr, sizeof(addrStr)))
            goto out;
    }

    clOsalMutexLock(&gXportCtrl.mutex);
    map = iocUdpMapFind(slot);
    if(!map)
    {
        clLogNotice("MAP", "ADD", "Adding address [%s] for slot [%d]", addrStr, slot);
        map = iocUdpMapAdd(addrStr, slot);
    }
    /*
     * Update if slot address changed.
     */
    else if(strcmp(map->addrstr, addrStr))
    {
        udpMapDel(map);
        close(map->sendFd);
        free(map);
        clLogNotice("MAP", "ADD", "Updating address [%s] for SLOT [%d]", addrStr, slot);
        map = iocUdpMapAdd(addrStr, slot);
    }
    if(!map) goto out_unlock;

    rc = CL_OK;

    out_unlock:
    clOsalMutexUnlock(&gXportCtrl.mutex);

    out:
    return rc;
}

/*
 * Got notification node/comp arrival and update hash map
 */
static ClRcT _clUdpMapUpdateNotification(ClIocNotificationT *notification, ClPtrT cookie)
{
    ClIocUdpMapT *map = NULL;
    ClIocNotificationIdT notificationId = ntohl(notification->id);
    ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    clOsalMutexLock(&gXportCtrl.mutex);
    switch (notificationId) 
    {
        case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
        case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
        case CL_IOC_NODE_LINK_UP_NOTIFICATION:
            if (!(map = iocUdpMapFind(nodeAddress)))
            {
                iocUdpMapAdd(NULL, nodeAddress);
            }
            break;
        case CL_IOC_NODE_LEAVE_NOTIFICATION:
        case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
            if (nodeAddress != gIocLocalBladeAddress)
            {
                clIocUdpMapDel(nodeAddress);
            }
            break;
        default:
            break;
    }
    clOsalMutexUnlock(&gXportCtrl.mutex);
    return CL_OK;
}

ClRcT xportAddressAssign(void)
{
    ClRcT rc = CL_OK;

    // Assigned IP address for node
    clPluginHelperAddRemVirtualAddress("up", &gVirtualIp);
    return rc;
}

ClRcT xportInit(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep)
{
    ClRcT rc = CL_OK;

    if(xportType)
    {
        gClUdpXportType[0] = 0;
        strncat(gClUdpXportType, xportType, sizeof(gClUdpXportType)-1);
    }
    gClUdpXportId = xportId;
    gClBindOffset = gIocLocalBladeAddress;
    gClSimulationMode = clParseEnvBoolean("ASP_MULTINODE");
    if(gClSimulationMode)
    {
        clLogNotice("XPORT", "INIT", "Simulation mode is enabled for the runtime");
        gClBindOffset <<= 10;
    }
    clPluginHelperGetVirtualAddressInfo("UDP", &gVirtualIp);

    clLogDebug(
            "UDP",
            "INI",
            "Link Name: %s, IP Node Address: %s, Network Address: %s, Broadcast: %s",
            gVirtualIp.dev, gVirtualIp.ip, gVirtualIp.netmask, gVirtualIp.broadcast);

    /*
     * To do a fast pass early update node entry table
     */
    rc = clJobQueueInit(&gXportLinkNotifyQueue, 0, 1);
    if(rc != CL_OK)
    {
        clLogError("UDP", "INIT", "Link notify job queue creation failed with [%#x]. "
                   "Link level or split brain recovery wont function", rc);
    }

    clIocNotificationRegister(_clUdpMapUpdateNotification, NULL);

    // TODO: identify from linkName
    gXportCtrl.family = PF_INET;

    rc = clOsalMutexInit(&gXportCtrl.mutex);
    CL_ASSERT(rc == CL_OK);

    //Add to map with default this node
    iocUdpMapAdd(gVirtualIp.ip, gIocLocalBladeAddress);

    ClUint32T numNodes = 0, i = 0;
    ClIocNodeAddressT *pNodes = NULL;

    rc = clIocTotalNeighborEntryGet(&numNodes);
    if(rc != CL_OK)
    {
        clLogWarning("UDP", "INI", "Failed to get the number of neighbors in ASP system. error code [0x%x].", rc);
        goto out;
    }

    pNodes = (ClIocNodeAddressT *)clHeapAllocate(sizeof(ClIocNodeAddressT) * numNodes);
    if(pNodes == NULL)
    {
        clLogWarning("UDP", "INI", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(ClIocNodeAddressT) * numNodes, rc);
        goto out;
    }

    rc = clIocNeighborListGet(&numNodes, pNodes);
    if (rc != CL_OK)
    {
        clLogWarning("UDP", "INI", "Failed to get the neighbor node addresses. error code [0x%x].", rc);
        goto out;
    }

    /* insert into udp map */
    for (i = 0; i < numNodes; i++)
    {
        if (pNodes[i] != clIocLocalAddressGet())
        {
            iocUdpMapAdd(NULL, pNodes[i]);
        }
    }

    clHeapFree(pNodes);

    out:
    gUdpInit = CL_TRUE;
    rc = CL_OK;
    return rc;
}

static ClRcT udpDispatchCallback(ClInt32T fd, ClInt32T events, void *cookie)
{
    ClRcT rc = CL_OK;
    ClIocUdpPrivateT *xportPrivate = cookie;
    ClUint8T buffer[0xffff+1];
    struct msghdr msgHdr;
    struct sockaddr peerAddress;
    struct iovec ioVector[1];
    ClInt32T bytes;

    if(!xportPrivate)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No private data\n"));
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    memset(&msgHdr, 0, sizeof(msgHdr));
    memset(ioVector, 0, sizeof(ioVector));
    msgHdr.msg_name = &peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector[0].iov_base = (ClPtrT)buffer;
    ioVector[0].iov_len = sizeof(buffer);
    msgHdr.msg_iov = ioVector;
    msgHdr.msg_iovlen = sizeof(ioVector)/sizeof(ioVector[0]);
    /*
     * need to check returned payload should not be greater than asked
     */
    recv:
    bytes = recvmsg(fd, &msgHdr, 0);
    if(bytes < 0 )
    {
        if(errno == EINTR)
            goto recv;
        perror("Receive : ");
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("recv error. errno = %d\n",errno));
        rc = CL_ERR_LIBRARY;
        goto out;
    }

    rc = clIocDispatchAsync(xportPrivate->port, buffer, bytes);

    out:
    return rc;
}

static ClRcT __xportBind(ClIocPortT port, ClInt32T *pFd)
{
    ClRcT rc = CL_ERR_LIBRARY;
    struct sockaddr_in6 ipv6_addr;
    struct sockaddr_in ipv4_addr;
    struct sockaddr *addr = (struct sockaddr*)&ipv4_addr;
    int fd = -1;
    int flag = 1;

    switch(gXportCtrl.family)
    {
    case PF_INET6:
        fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        addr = (struct sockaddr*)&ipv6_addr;
        ipv6_addr.sin6_addr = in6addr_any;
        ipv6_addr.sin6_port = htons(port + CL_TRANSPORT_BASE_PORT + gClBindOffset);
        ipv6_addr.sin6_family = PF_INET6;
        break;
    case PF_INET:
    default:
        fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        ipv4_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        ipv4_addr.sin_port = htons(port + CL_TRANSPORT_BASE_PORT + gClBindOffset);
        ipv4_addr.sin_family = PF_INET;
        break;
    }
    if(fd < 0)
        goto out;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
    {
        clLogError("UDP", "BIND", "setsockopt for SO_REUSEADDR failed with error [%s]", strerror(errno));
        goto out_close;
    }

    if(bind(fd, addr, sizeof(*addr)) < 0)
    {
        clLogError("UDP", "BIND", "Bind failed with error [%s]", strerror(errno));
        goto out_close;
    }
    *pFd = fd;
    rc = CL_OK;
    goto out;

    out_close:
    close(fd);

    out:
    return rc;
}

ClRcT xportBind(ClIocPortT port)
{
    ClRcT rc = CL_OK;
    ClInt32T fd = -1;
    ClIocUdpPrivateT *xportPrivate = NULL;
    if(!port)
    {
        return CL_ERR_INVALID_PARAMETER;
    }
    /*
     * Check if already bound
     */
    xportPrivate = (ClIocUdpPrivateT*) clTransportPrivateDataGet(gClUdpXportId, port);
    if(xportPrivate)
    {
        goto out;
    }
    rc = __xportBind(port, &fd);
    if(rc != CL_OK)
    {
        clLogError("UDP", "BIND", "Bind failed for port [%#x] with error [%#x]", port, rc);
        goto out;
    }

    xportPrivate = clHeapCalloc(1, sizeof(*xportPrivate));
    CL_ASSERT(xportPrivate != NULL);
    xportPrivate->port = port;
    xportPrivate->fd = fd;
    clTransportPrivateDataSet(gClUdpXportId, port, (void*)xportPrivate, NULL);

    out:
    return rc;
}

ClRcT xportBindClose(ClIocPortT port)
{
    ClIocUdpPrivateT *xportPrivate = NULL;
    ClRcT rc = CL_OK;
    if(!(xportPrivate = clTransportPrivateDataDelete(gClUdpXportId, port))) 
        return CL_ERR_NOT_EXIST;
    if(xportPrivate->port != port)
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    close(xportPrivate->fd);
    clHeapFree(xportPrivate);
    return rc;
}

ClRcT xportListen(ClIocPortT port)
{
    ClRcT rc = CL_OK;
    ClInt32T fd = -1;
    ClIocUdpPrivateT *xportPrivate = NULL;
    void *xportPrivateLast = NULL;
    if(!port)
    {
        return CL_ERR_INVALID_PARAMETER;
    }
    xportPrivate = (ClIocUdpPrivateT*) clTransportPrivateDataGet(gClUdpXportId, port);
    if(!xportPrivate)
    {
        rc = __xportBind(port, &fd);
        if(rc != CL_OK)
        {
            clLogError("UDP", "LISTENER", "Bind failed for port [%#x] with error [%#x]", port, rc);
            goto out;
        }
        xportPrivate = clHeapCalloc(1, sizeof(*xportPrivate));
        CL_ASSERT(xportPrivate != NULL);
        xportPrivate->port = port;
        xportPrivate->fd = fd;
    }
    else
    {
        xportPrivateLast = xportPrivate;
    }
    rc = clTransportListenerRegister(xportPrivate->fd, udpDispatchCallback, (void*)xportPrivate);
    if(rc != CL_OK)
    {
        clLogError("UDP", "LISTENER", "Register failed for port [%#x] with error [%#x]", port, rc);
        goto out_free;
    }
    if(!xportPrivateLast)
    {
        clTransportPrivateDataSet(gClUdpXportId, port, (void*)xportPrivate, NULL);
    }

    goto out;
    
    out_free:
    if(!xportPrivateLast)
    {
        close(xportPrivate->fd);
        clHeapFree(xportPrivate);
    }

    out:
    return rc;
}

ClRcT xportListenStop(ClIocPortT port)
{
    ClIocUdpPrivateT *xportPrivate = NULL;
    ClRcT rc = CL_OK;
    if(!(xportPrivate = clTransportPrivateDataDelete(gClUdpXportId, port)))
        return CL_ERR_INVALID_PARAMETER;
    if(xportPrivate->port != port)
    {
        rc = CL_ERR_NOT_EXIST;
        goto out_free;
    }
    rc = clTransportListenerDeregister(xportPrivate->fd);
    out_free:
    clHeapFree(xportPrivate);
    return rc;
}

ClRcT xportRecv(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption,
                ClUint8T *pBuffer, ClUint32T bufSize, 
                ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_OK;
    ClUint8T buffer[0xffff+1];
    ClIocCommPortT *pCommPort = (ClIocCommPortT*)commPort;
    ClIocUdpPrivateT *pCommPortPrivate = NULL;
    struct msghdr msgHdr;
    struct sockaddr peerAddress;
    struct iovec ioVector;
    struct pollfd pollfd;
    ClTimeT tm;
    ClUint32T timeout;
    ClInt32T bytes = 0;
    ClInt32T pollStatus;

    if(!pCommPort || !pRecvOption || !message || !pRecvParam)
    {
        rc = CL_ERR_INVALID_PARAMETER;
        goto out;
    }

    if(! (pCommPortPrivate = (ClIocUdpPrivateT*)clTransportPrivateDataGet(gClUdpXportId, pCommPort->portId)) )
    {
        rc = CL_ERR_NOT_EXIST;
        goto out;
    }

    if(!pBuffer)
    {
        pBuffer = buffer;
        bufSize = (ClUint32T)sizeof(buffer);
    }
    
    memset(&pollfd, 0, sizeof(pollfd));

    memset(&msgHdr, 0, sizeof(msgHdr));
    memset(&ioVector, 0, sizeof(ioVector));
    msgHdr.msg_name = &peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector.iov_base = (ClPtrT)pBuffer;
    ioVector.iov_len = bufSize;
    msgHdr.msg_iov = &ioVector;
    msgHdr.msg_iovlen = 1;
    timeout = pRecvOption->timeout;

    retry:
    for(;;)
    {
        pollfd.fd = pCommPortPrivate->fd;
        pollfd.events = POLLIN|POLLRDNORM;
        pollfd.revents = 0;
        tm = clOsalStopWatchTimeGet();
        pollStatus = poll(&pollfd, 1, timeout);
        if(pollStatus > 0) 
        {
            if((pollfd.revents & (POLLIN|POLLRDNORM)))
            {
                recv:
                bytes = recvmsg(pCommPortPrivate->fd, &msgHdr, 0);
                if(bytes < 0 )
                {
                    if(errno == EINTR)
                        goto recv;
                    perror("Receive : ");
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("recv error. errno = %d\n",errno));
                    goto out;
                }
            } 
            else 
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("poll error. errno = %d\n", errno));
                goto out;
            }
        } 
        else if(pollStatus < 0 ) 
        {
            if(errno == EINTR)
                continue;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in poll. errno = %d\n",errno));
            goto out;
        } 
        else 
        {
            rc = CL_ERR_TIMEOUT;
            goto out;
        }
        break;
    }

    rc = clIocDispatch(commPort, pRecvOption, pBuffer, bytes, message, pRecvParam);

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN)
        goto retry;

    if(CL_GET_ERROR_CODE(rc) == IOC_MSG_QUEUED)
    {
        ClUint32T elapsedTm;
        if(timeout == CL_IOC_TIMEOUT_FOREVER)
            goto retry;
        elapsedTm = (clOsalStopWatchTimeGet() - tm)/1000;
        if(elapsedTm < timeout)
        {
            timeout -= elapsedTm;
            goto retry;
        }
        else
        {
            rc = CL_ERR_TIMEOUT;
            CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Dropping a received fragmented-packet. "
                                              "Could not receive the complete packet within "
                                              "the specified timeout. Packet size is %d", bytes));

        }
    }

    out:
    return rc;

}

static ClRcT iocUdpSend(ClIocUdpMapT *map, void *args)
{
    ClIocUdpArgsT *sendArgs = args;
    struct sockaddr *destaddr = NULL;
    socklen_t addrlen = 0;
    struct msghdr msghdr;
    ClRcT rc = CL_OK;
    int portOffset = map->slot;

    if(gClSimulationMode)
    {
        portOffset <<= 10;
    }
    if(map->family == PF_INET)
    {
        map->__ipv4_addr.sin_port = htons(CL_TRANSPORT_BASE_PORT + sendArgs->port + portOffset);
        destaddr = (struct sockaddr*)&map->__ipv4_addr;
        addrlen = sizeof(struct sockaddr_in);
    }
    else
    {
        map->__ipv6_addr.sin6_port = htons(CL_TRANSPORT_BASE_PORT + sendArgs->port  + portOffset);
        destaddr = (struct sockaddr*)&map->__ipv6_addr;
        addrlen = sizeof(struct sockaddr_in6);
    }
    memset(&msghdr, 0, sizeof(msghdr));
    msghdr.msg_name = destaddr;
    msghdr.msg_namelen = addrlen;
    msghdr.msg_iov = sendArgs->iov;
    msghdr.msg_iovlen = sendArgs->iovlen;
    if(sendmsg(map->sendFd, &msghdr, sendArgs->flags) < 0)
    {
        clLogError("UDP", "SEND", "UDP send failed with error [%s] for addr [%s], port [0x%x:%d]",
                   strerror(errno), map->addrstr, sendArgs->port, 
                   CL_TRANSPORT_BASE_PORT + sendArgs->port + portOffset);
        rc = CL_ERR_LIBRARY;
    }
    else
    {
        clLogTrace("UDP", "SEND", "UDP send successful for [%d] iovs, addr [%s], port [0x%x:%d]",
                   sendArgs->iovlen, map->addrstr, sendArgs->port, 
                   CL_TRANSPORT_BASE_PORT + sendArgs->port + portOffset);
    }
    return rc;
}

ClRcT xportSend(ClIocPortT port, ClUint32T priority, ClIocAddressT *address, 
                struct iovec *iov, ClInt32T iovlen, ClInt32T flags)
{
    ClIocUdpMapT *map;
    ClIocUdpMapT addrMap = {0};
    ClIocUdpArgsT sendArgs = {0};
    ClUint8T buff[CL_IOC_BYTES_FOR_COMPS_PER_NODE];
    ClUint32T i = 0;
    ClRcT rc = CL_OK;

    if(!address || !iovlen)
        return CL_OK;
    sendArgs.iov = iov;
    sendArgs.iovlen = iovlen;
    sendArgs.flags = flags;

    clOsalMutexLock(&gXportCtrl.mutex);

    switch(CL_IOC_ADDRESS_TYPE_GET(address))
    {
    case CL_IOC_PHYSICAL_ADDRESS_TYPE:
        if(address->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
            goto bcast_send;
        map = iocUdpMapFind(address->iocPhyAddress.nodeAddress);
        if(!map)
        {
            map = iocUdpMapAdd(NULL, address->iocPhyAddress.nodeAddress);
            if(!map)
            {
                clOsalMutexUnlock(&gXportCtrl.mutex);
                clLogError(
                        "UDP",
                        "SEND",
                        "Unable to add mapping for ioc slot [%d]. ", address->iocPhyAddress.nodeAddress);
                rc = CL_ERR_NOT_EXIST;
                goto out;
            }
        }
        sendArgs.port = address->iocPhyAddress.portId;
        memcpy(&addrMap, map, sizeof(addrMap));
        clOsalMutexUnlock(&gXportCtrl.mutex);
        rc = iocUdpSend(&addrMap, &sendArgs);
        goto out;

    case CL_IOC_BROADCAST_ADDRESS_TYPE:
    bcast_send:
        sendArgs.port = address->iocPhyAddress.portId;
        rc = iocUdpMapWalk(iocUdpSend, &sendArgs, 0);
        goto out;
        /*
         * Unhandled till now.
         */
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        clLogWarning("UDP", "SEND", "Ignoring send for logical address type");
        break;
    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        clLogWarning("UDP", "SEND", "Ignoring send for multicast address type");
        break;

    case CL_IOC_INTRANODE_ADDRESS_TYPE:
        clIocNodeCompsGet(clIocLocalAddressGet(), buff);
        map = iocUdpMapFind(clIocLocalAddressGet());
        if (map)
        {
            memcpy(&addrMap, map, sizeof(addrMap));
            clOsalMutexUnlock(&gXportCtrl.mutex);
            for (i=0; i< CL_IOC_MAX_COMP_PORT; i++)
            {
                if (i != port && (buff[i>>3] & (1 << (i&7))))
                {
                    sendArgs.port = i;
                    ClRcT retCode = iocUdpSend(&addrMap, &sendArgs);
                    if(retCode != CL_OK)
                    {
                        rc = retCode;
                    }
                }
            }
            goto out;
        }
        break;
    default:
        break;
    }
    clOsalMutexUnlock(&gXportCtrl.mutex);

    out:
    return rc;
}

ClRcT xportNotifyInit(void)
{
    if(!gUdpInit)
        return CL_ERR_NOT_INITIALIZED;
    return clUdpEventHandlerInitialize();
}

ClRcT xportNotifyFinalize(void)
{
    if(!gUdpInit)
        return CL_ERR_NOT_INITIALIZED;

    return clUdpEventHandlerFinalize();
}

static ClRcT linkNotify(ClPtrT args)
{
    ClLinkNotificationArgsT *linkNotification = args;
    ClRcT rc = clUdpNodeNotification(linkNotification->node, linkNotification->event);
    clHeapFree(linkNotification);
    return rc;
}

static ClRcT xportLinkNotify(ClIocNodeAddressT node, ClIocNotificationIdT id)
{
    ClRcT rc = CL_OK;
    ClLinkNotificationArgsT *linkNotification = clHeapCalloc(1, sizeof(*linkNotification));
    CL_ASSERT(linkNotification != NULL);
    linkNotification->node = node;
    linkNotification->event = id;
    rc = clJobQueuePush(&gXportLinkNotifyQueue, linkNotify, linkNotification);
    if(rc != CL_OK)
    {
        clHeapFree(linkNotification);
    }
    return rc;
}

/*
 * Notify the port used
 */
ClRcT xportNotifyOpen(ClIocNodeAddressT node, ClIocPortT port, 
                      ClIocNotificationIdT event)
{
    ClRcT rc = CL_OK;
    if(port != CL_IOC_XPORT_PORT)
    {
        rc = clUdpNotify(gIocLocalBladeAddress, port, CL_IOC_COMP_ARRIVAL_NOTIFICATION);
        clTransportNotifyOpen(port);
    }
    else if(node)
    {
        if(node != gIocLocalBladeAddress && event == CL_IOC_NODE_LINK_UP_NOTIFICATION)
        {
            rc = xportLinkNotify(node, event);
        }
        else
        {
            rc = clUdpNotify(node, port, CL_IOC_COMP_ARRIVAL_NOTIFICATION);
        }
    }
    return rc;
}

/*
 * Notify the port unused
 */
ClRcT xportNotifyClose(ClIocNodeAddressT nodeAddress, ClIocPortT port, 
                       ClIocNotificationIdT event)
{
    ClRcT rc = CL_OK;
    if(nodeAddress != gIocLocalBladeAddress && 
       port == CL_IOC_XPORT_PORT && 
       event == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
    {
        rc = xportLinkNotify(nodeAddress, event);
    }
    else
    {
        clUdpNotify(nodeAddress, port, CL_IOC_COMP_DEATH_NOTIFICATION);
        clTransportNotifyClose(port);
    }
    return rc;
}

// Get socket created associate portId
ClRcT clUdpFdGet(ClIocPortT port, ClInt32T *fd)
{
    ClIocUdpPrivateT *xportPrivate;
    if(!fd) return CL_ERR_INVALID_PARAMETER;
    if(! (xportPrivate = (ClIocUdpPrivateT*)clTransportPrivateDataGet(gClUdpXportId, port)))
        return CL_ERR_NOT_EXIST;
    *fd = xportPrivate->fd;
    return CL_OK;
}
