#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTipcUserApi.h>
#include <clTipcGeneral.h>
#include <clTransport.h>
#include <clParserApi.h>
#include <clList.h>
#include <clHash.h>
#include <clDebugApi.h>

#define UDP_CONFIG "clIocUDP.xml"
#define IOC_UDP_MAP_BITS (8)
#define IOC_UDP_MAP_SIZE (1 << IOC_UDP_MAP_BITS)
#define IOC_UDP_MAP_MASK (IOC_UDP_MAP_SIZE-1)
#define __STOP_WALK_ON_ERROR (0x1)

static struct hashStruct *gIocUdpMap[IOC_UDP_MAP_SIZE];
static CL_LIST_HEAD_DECLARE(gIocUdpMapList);
static ClInt32T gClUdpXportId;
extern ClIocNodeAddressT gIocLocalBladeAddress;
typedef struct ClIocUdpMap
{
#define __ipv4_addr _addr.sin_addr
#define __ipv6_addr _addr.sin6_addr
    ClIocNodeAddressT slot;
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

static ClXportCtrlT gXportCtrl;

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

ClRcT iocUdpMapDel(ClIocNodeAddressT slot)
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

ClRcT iocUdpMapWalk(ClRcT (*callback)(ClIocUdpMapT *map, void *cookie), void *cookie, ClInt32T flags)
{
    ClIocUdpMapT *map;
    register ClListHeadT *iter;
    ClListHeadT *next;
    ClRcT rc = CL_OK;
    for(iter = gIocUdpMapList.pNext; iter != &gIocUdpMapList; iter = next)
    {
        next = iter->pNext;
        map = CL_LIST_ENTRY(iter, ClIocUdpMapT, list);
        rc = callback(map, cookie);
        if( (rc != CL_OK) 
            &&
            (flags & __STOP_WALK_ON_ERROR) )
            goto out;
    }
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT iocUdpMapAdd(const ClCharT *addr, ClIocNodeAddressT slot)
{
    ClIocUdpMapT *map = calloc(1, sizeof(*map));
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    CL_ASSERT(map != NULL);
    map->slot = slot;
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
    clLogNotice("UDP", "MAP", "Loaded address [%s] with slot mapped at [%d]", addr, slot);
    rc = CL_OK;
    goto out;

    out_free:
    free(map);

    out:
    return rc;
}

ClRcT xportInit(void)
{
    ClParserPtrT parent;
    ClParserPtrT family;
    ClParserPtrT nodes;
    ClParserPtrT node;
    ClRcT rc = CL_ERR_NOT_EXIST;
    const ClCharT *configPath;

    if(!(configPath = getenv("ASP_CONFIG")))
        configPath = ".";

    parent = clParserOpenFile(configPath, UDP_CONFIG);
    if(!parent)
    {
        clLogError("UDP", "INI", "Unable to load config file [%s]", UDP_CONFIG);
        goto out;
    }
    gXportCtrl.family = PF_INET;
    rc = clOsalMutexInit(&gXportCtrl.mutex);
    CL_ASSERT(rc == CL_OK);
    family = clParserChild(parent, "family");
    if(family)
    {
        ClInt32T len = strlen(family->txt);
        if(len >= 5 && !strncasecmp(&family->txt[len-5], "inet6", 5))
            gXportCtrl.family = PF_INET6;
    }
    nodes = clParserChild(parent, "nodes");
    if(!nodes)
    {
        clLogError("UDP", "INI", "Unable to parse nodes tag in file [%s]", UDP_CONFIG);
        goto out_free;
    }
    node = clParserChild(nodes, "node");
    if(!node)
    {
        clLogError("UDP", "INI", "Unable to parse node tag in file [%s]", UDP_CONFIG);
        goto out_free;
    }
    while(node)
    {
        ClParserPtrT addr = clParserChild(node, "addr");
        ClParserPtrT slot = clParserChild(node, "slot");
        ClIocNodeAddressT iocAddress;
        ClCharT *ip;
        if(!addr || !node) goto next;
        ip = addr->txt;
        iocAddress = atoi(slot->txt);
        if(!iocAddress)
        {
            clLogNotice("UDP", "INI", "Skipping entry with address [%s] as slot is invalid", ip);
            goto next;
        }
        iocUdpMapAdd(ip, iocAddress);
        next: 
        node = node->next;
    }
    gClUdpXportId = clTransportIdGet();
    rc = CL_OK;

    out_free:
    clParserFree(parent);

    out:
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

    rc = clTransportDispatch(xportPrivate->port, buffer, bytes);

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
        ipv6_addr.sin6_port = htons(port + CL_TRANSPORT_BASE_PORT   + (gIocLocalBladeAddress<<10));
        ipv6_addr.sin6_family = PF_INET6;
        break;
    case PF_INET:
    default:
        fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        ipv4_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        ipv4_addr.sin_port = htons(port + CL_TRANSPORT_BASE_PORT  +(gIocLocalBladeAddress <<10));
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
    ClIocUdpPrivateT *xportPrivateLast = NULL;
    if(!port)
    {
        return CL_ERR_INVALID_PARAMETER;
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
    clTransportPrivateDataSet(gClUdpXportId, port, (void*)xportPrivate, (void**)&xportPrivateLast);
    if(xportPrivateLast)
    {
        close(xportPrivateLast->fd);
        clHeapFree(xportPrivateLast);
    }
    goto out;
    
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
    rc = clTransportListenerRegister(fd, udpDispatchCallback, (void*)xportPrivate);
    if(rc != CL_OK)
    {
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
    ClTipcCommPortT *pCommPort = (ClTipcCommPortT*)commPort;
    ClIocUdpPrivateT *pCommPortPrivate = NULL;
    struct msghdr msgHdr;
    struct sockaddr peerAddress;
    struct iovec ioVector;
    struct pollfd pollfd;
    ClTimeT tm;
    ClUint32T timeout;
    ClInt32T bytes;
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

    rc = clIocDispatch(commPort, pRecvOption, pBuffer, bufSize, message, pRecvParam);

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

    if(map->family == PF_INET)
    {
        map->__ipv4_addr.sin_port = htons(CL_TRANSPORT_BASE_PORT + sendArgs->port + (map->slot << 10));
        destaddr = (struct sockaddr*)&map->__ipv4_addr;
        addrlen = sizeof(struct sockaddr_in);
    }
    else
    {
        map->__ipv6_addr.sin6_port = htons(CL_TRANSPORT_BASE_PORT + sendArgs->port  + (map->slot << 10));
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
        clLogError("UDP", "SEND", "UDP send failed with error [%s] for addr [%s], port [%d]", 
                   strerror(errno), map->addrstr, CL_TRANSPORT_BASE_PORT + sendArgs->port);
        rc = CL_ERR_LIBRARY;
    }
    else
    {
        clLogTrace("UDP", "SEND", "UDP send successful for [%d] iovs, addr [%s], port [%d]", 
                   sendArgs->iovlen, map->addrstr, CL_TRANSPORT_BASE_PORT + sendArgs->port);
    }
    return rc;
}

ClRcT xportSend(ClIocPortT port, ClUint32T priority, ClIocAddressT *address, 
                struct iovec *iov, ClInt32T iovlen, ClInt32T flags)
{
    ClIocUdpMapT *map;
    ClIocUdpArgsT sendArgs = {0};
    if(!address || !iovlen)
        return CL_OK;
    sendArgs.iov = iov;
    sendArgs.iovlen = iovlen;
    sendArgs.flags = flags;
    switch(CL_IOC_ADDRESS_TYPE_GET(address))
    {
    case CL_IOC_PHYSICAL_ADDRESS_TYPE:
        if(address->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
            goto bcast_send;
        map = iocUdpMapFind(address->iocPhyAddress.nodeAddress);
        if(!map)
        {
            clLogError("UDP", "SEND", "Unable to find mapping for ioc slot [%d]. "
                       "Please ensure the mapping exists in [%s]", 
                       address->iocPhyAddress.nodeAddress, UDP_CONFIG);
            return CL_ERR_NOT_EXIST;
        }
        sendArgs.port = address->iocPhyAddress.portId;
        iocUdpSend(map, &sendArgs);
        break;
    case CL_IOC_BROADCAST_ADDRESS_TYPE:
    bcast_send:
        sendArgs.port = address->iocPhyAddress.portId;
        iocUdpMapWalk(iocUdpSend, &sendArgs, 0);
        break;
        /*
         * Unhandled till now.
         */
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        clLogWarning("UDP", "SEND", "Ignoring send for logical address type");
        break;
    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        clLogWarning("UDP", "SEND", "Ignoring send for multicast address type");
        break;

    default:
        break;
    }
    return CL_OK;
}
