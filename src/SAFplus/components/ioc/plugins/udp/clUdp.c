#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#ifdef HAVE_SCTP
#include <netinet/sctp.h>
#endif
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
#include "clUdpSetup.h"
#include "clUdpNotification.h"

#define IOC_UDP_MAP_BITS (8)
#define IOC_UDP_MAP_SIZE (1 << IOC_UDP_MAP_BITS)
#define IOC_UDP_MAP_MASK (IOC_UDP_MAP_SIZE-1)
#define __STOP_WALK_ON_ERROR (0x1)

#ifdef __cplusplus
extern "C" {
#endif

ClInt32T gClUdpXportId;
ClCharT  gClUdpXportType[CL_MAX_NAME_LENGTH];
static ClBoolT udpPriorityChangePossible = CL_TRUE;  /* Don't attempt to change priority if UDP does not support, so we don't get tons of error msgs */
static struct hashStruct *gIocUdpMap[IOC_UDP_MAP_SIZE];
static CL_LIST_HEAD_DECLARE(gIocUdpMapList);
extern ClIocNodeAddressT gIocLocalBladeAddress;
static ClPluginHelperVirtualIpAddressT gVirtualIp;
ClBoolT ASP_UDP_USE_EXISTING_IP = CL_FALSE;
static ClBoolT gUdpInit = CL_FALSE;
ClBoolT gClSimulationMode = CL_FALSE;
ClBoolT gClNodeRepresentative = CL_FALSE;
ClInt32T gClProtocol = IPPROTO_UDP;
ClInt32T gClSockType = SOCK_DGRAM;
ClInt32T gClCmsgHdrLen;
struct cmsghdr *gClCmsgHdr;
static ClUint32T gClBindOffset;

typedef struct ClUdpAddrCacheEntry
{
    ClCharT addrStr[INET_ADDRSTRLEN];
}ClUdpAddrCacheEntryT;

#define CL_UDP_ADDR_CACHE_SEGMENT "/CL_UDP_ADDR_CACHE"
#define CL_UDP_ADDR_CACHE_SEGMENT_SIZE  CL_IOC_ALIGN((ClUint32T)(CL_IOC_MAX_NODES*sizeof(ClUdpAddrCacheEntryT)), 8)

#define UDP_LOG_AREA_UDP	"UDP"
#define UDP_LOG_CTX_UDP_MAP	"MAP"
#define	UDP_LOG_CTX_UDP_SEND	"SEND"
#define	UDP_LOG_CTX_UDP_RECV	"RECV"

static ClUint8T *gpClUdpAddrCache;
static ClCharT gClUdpAddrCacheSegment[CL_MAX_NAME_LENGTH+1];
static ClOsalSemIdT gClUdpAddrCacheSem;

typedef struct ClIocUdpArgs
{
    struct iovec *iov;
    int iovlen;
    int flags;
    int port;
    ClUint32T priority;
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

static ClRcT clUdpAddrCacheCreate(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    clOsalShmUnlink(gClUdpAddrCacheSegment);
    rc = clOsalShmOpen(gClUdpAddrCacheSegment, O_RDWR | O_CREAT | O_EXCL, 0666, &fd);
    if (rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache shm open of segment [%s] returned [%#x]", gClUdpAddrCacheSegment, rc);
        return rc;
    }

    rc = clOsalFtruncate(fd, CL_UDP_ADDR_CACHE_SEGMENT_SIZE);
    if (rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache truncate of size [%d] returned [%#x]", (ClUint32T) CL_UDP_ADDR_CACHE_SEGMENT_SIZE,
                rc);
        clOsalShmUnlink(gClUdpAddrCacheSegment);
        close((ClInt32T) fd);
        return rc;
    }

    rc = clOsalMmap(0, CL_UDP_ADDR_CACHE_SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, (ClPtrT*) &gpClUdpAddrCache);
    if (rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache segment mmap returned [%#x]", rc);
        clOsalShmUnlink(gClUdpAddrCacheSegment);
        close((ClInt32T) fd);
        return rc;
    }

    rc = clOsalSemCreate((ClUint8T*) gClUdpAddrCacheSegment, 1, &gClUdpAddrCacheSem);

    if (rc != CL_OK)
    {
        /* Delete existing one and try again */
        ClOsalSemIdT semId = 0;

        if (clOsalSemIdGet((ClUint8T*) gClUdpAddrCacheSegment, &semId) != CL_OK)
        {
            clLogError("NODE", "CACHE", "UDP addresses cache segment sem creation error while fetching sem id");
            clOsalShmUnlink(gClUdpAddrCacheSegment);
            close((ClInt32T) fd);
            return rc;
        }

        if (clOsalSemDelete(semId) != CL_OK)
        {
            clLogError("NODE", "CACHE", "UDP addresses cache segment sem creation error while deleting old sem id");
            clOsalShmUnlink(gClUdpAddrCacheSegment);
            close((ClInt32T) fd);
            return rc;
        }

        rc = clOsalSemCreate((ClUint8T*) gClUdpAddrCacheSegment, 1, &gClUdpAddrCacheSem);
    }

    return rc;
}

static ClRcT clUdpAddrCacheOpen(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    rc = clOsalShmOpen(gClUdpAddrCacheSegment, O_RDWR, 0666, &fd);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache [%s] segment open returned [%#x]",
                gClUdpAddrCacheSegment, rc);
        return rc;
    }

    rc = clOsalMmap(0, CL_UDP_ADDR_CACHE_SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0,
                    (ClPtrT*)&gpClUdpAddrCache);

    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache segment mmap returned [%#x]", rc);
        close((ClInt32T)fd);
        return rc;
    }

    rc = clOsalSemIdGet((ClUint8T*)gClUdpAddrCacheSegment, &gClUdpAddrCacheSem);

    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "UDP addresses cache semid get returned [%#x]", rc);
        close((ClInt32T)fd);
    }

    return rc;
}

static ClRcT clUdpAddrCacheInitialize(ClBoolT createFlag)
{
    ClRcT rc = CL_OK;

    if(gpClUdpAddrCache)
        return rc;

    snprintf(gClUdpAddrCacheSegment, sizeof(gClUdpAddrCacheSegment)-1,
             "%s_%d", CL_UDP_ADDR_CACHE_SEGMENT, clIocLocalAddressGet());

    if (createFlag == CL_TRUE)
    {
        rc = clUdpAddrCacheCreate();
    }
    else
    {
        rc = clUdpAddrCacheOpen();
    }

    if (rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Segment initialize returned [%#x]", rc);
    }

    CL_ASSERT(gpClUdpAddrCache != NULL);

    if (createFlag == CL_TRUE)
    {
        rc = clOsalMsync(gpClUdpAddrCache, CL_UDP_ADDR_CACHE_SEGMENT_SIZE, MS_ASYNC);
        if (rc != CL_OK)
        {
            clLogError("NODE", "CACHE", "UDP addresses cache segment msync returned [%#x]", rc);
        }
    }

    return rc;
}

static ClRcT clUdpAddrCacheFinalize(ClBoolT createFlag)
{
    ClRcT rc = CL_ERR_NOT_INITIALIZED;

    if (gpClUdpAddrCache)
    {
        clOsalSemLock(gClUdpAddrCacheSem);

        if (createFlag)
        {
            clOsalMsync(gpClUdpAddrCache, CL_UDP_ADDR_CACHE_SEGMENT_SIZE, MS_SYNC);
        }

        clOsalMunmap(gpClUdpAddrCache, CL_UDP_ADDR_CACHE_SEGMENT_SIZE);
        gpClUdpAddrCache = NULL;

        clOsalSemUnlock(gClUdpAddrCacheSem);

        if (createFlag)
        {
            clOsalShmUnlink(gClUdpAddrCacheSegment);
            clOsalSemDelete(gClUdpAddrCacheSem);
        }
    }
    return rc;
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
ClRcT clUdpMapWalk(ClRcT (*callback)(ClIocUdpMapT *map, void *cookie), void *cookie, ClInt32T flags)
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
            addrMap = (ClIocUdpMapT*) clHeapRealloc(addrMap, sizeof(*addrMap) * (numEntries + growMask + 1));
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
    ClUint32T priority;
    map = (ClIocUdpMapT*) calloc(1, sizeof(*map));
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
        clPluginHelperConvertHostToInternetAddress(gVirtualIp.ipAddressMask + nodeBridge, addr);
        clLogTrace("UDP", "MAP", "Node [%d] at [%s] uses bridge [%d] through xport [%s]", slot, addr, nodeBridge, xportType);
    }
    map->slot = slot;
    map->bridge = nodeBridge != slot ? CL_TRUE : CL_FALSE;
    map->family = PF_INET;
    map->addrstr[0] = 0;
    strncat(map->addrstr, addr, sizeof(map->addrstr)-1);
    map->__ipv4_addr.sin_family = PF_INET;
    if(inet_pton(PF_INET, addr, (void*)&map->__ipv4_addr.sin_addr) != 1)
    {
        map->family = PF_INET6;
        map->__ipv6_addr.sin6_family = PF_INET6;
        if(inet_pton(PF_INET6, addr, (void*)&map->__ipv6_addr.sin6_addr) != 1)
        {
            clLogError("UDP", "MAP", "Error interpreting address [%s] for node [%d]", addr, slot);
            goto out_free;
        }
    }
    map->sendFd = socket(map->family, gClSockType, gClProtocol);
    if(map->sendFd < 0)
    {
        clLogError("UDP", "MAP", "Socket syscall returned with error [%s] for UDP socket", strerror(errno));
        goto out_free;
    }

    priority = CL_UDP_DEFAULT_PRIORITY;
    if (udpPriorityChangePossible)
    {
        if(!setsockopt(map->sendFd, SOL_IP, IP_TOS, &priority, sizeof(priority)))
        {
            map->priority = priority;
        }
        else
        {
            int err = errno;

            if (err == ENOPROTOOPT)
            {
                udpPriorityChangePossible = CL_FALSE;
                clLogWarning(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_MAP,"Message priority not available in this version of UDP.");
            }
            else clLogWarning(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_MAP,"Error in setting UDP message priority. errno [%d]",err);
            goto out_free;
        }
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

ClRcT clIocUdpMapAdd(struct sockaddr *addr, ClIocNodeAddressT slot, ClCharT *retAddrStr)
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
        rc = CL_OK;
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
        rc = CL_OK;
    }
    else
    {
        rc = CL_ERR_ALREADY_EXIST;
    }
    if(!map) goto out_unlock;

    if (retAddrStr)
    {
        strncat(retAddrStr, addrStr, INET_ADDRSTRLEN - 1);
    }

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
    ClCharT addStr[INET_ADDRSTRLEN] = {0};
    ClIocNotificationIdT notificationId = (ClIocNotificationIdT) ntohl(notification->id);
    ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    clOsalMutexLock(&gXportCtrl.mutex);
    switch (notificationId) 
    {
        case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
        case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
        case CL_IOC_NODE_LINK_UP_NOTIFICATION:
            if (!(map = iocUdpMapFind(nodeAddress)))
            {
                clUdpAddrGet(nodeAddress, addStr);
                iocUdpMapAdd(addStr, nodeAddress);
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

    /* Assign IP address for node, but ONLY if we aren't supposed to use the existing address */
    if (!ASP_UDP_USE_EXISTING_IP)
    {
        clPluginHelperAddRemVirtualAddress("up", &gVirtualIp);

        /* Configure multicast route, but ONLY if route does not exist at route table */
        if (clTransportMcastSupported(NULL))
        {
            ClCharT *mcastAddr = clTransportMcastAddressGet();
            clPluginHelperAddRouteAddress(mcastAddr, gVirtualIp.dev);
        }

    }
    return rc;
}

static ClRcT clUdpGetBackplaneInterface(const ClCharT *xportType, ClCharT *inf)
{
    ClCharT net_addr[CL_MAX_FIELD_LENGTH] = "eth0";
    ClCharT *linkName = NULL;
    ClCharT envlinkNameType[CL_MAX_FIELD_LENGTH] = { 0 };
    if (!xportType)
    {
        return CL_ERR_INVALID_PARAMETER;
    }
    else
    {
        snprintf(envlinkNameType, CL_MAX_FIELD_LENGTH, "ASP_%s_LINK_NAME", xportType);
        linkName = getenv(envlinkNameType);
        if (linkName == NULL)
        {
            // try with default LINK_NAME
            linkName = getenv("LINK_NAME");
            if (linkName == NULL)
            {
                clLogNotice("UDP", "INI", "%s and LINK_NAME environment variable is not exported. Using 'eth0' interface as default", envlinkNameType);
            }
            else
            {
                clLogNotice("UDP", "INI", "LINK_NAME env is exported. Value is %s", linkName);
                net_addr[0] = 0;
                strncat(net_addr, linkName, sizeof(net_addr)-1);
                ClCharT *token = NULL;
                strtok_r(net_addr, ":", &token);
            }
            /* If we are not using the existing IP addr then we need to use a virtual device to make sure we don't overwrite an already-configured address */
            if (!ASP_UDP_USE_EXISTING_IP)
              snprintf(inf, CL_MAX_FIELD_LENGTH, "%s:%d", net_addr, gIocLocalBladeAddress + 10);
            /* If we ARE using the existing IP, then whatever interfaces LINK_NAME is set to IS that address */
            else snprintf(inf, CL_MAX_FIELD_LENGTH, "%s", net_addr);            
        }
        else
        {
            clLogInfo("UDP", "INI", "%s env is exported. Value is %s", envlinkNameType, linkName);
            snprintf(inf, CL_MAX_FIELD_LENGTH, "%s", linkName);
        }
    }
    return CL_OK;
}

static ClRcT clUdpGetNodeIpAddress(const ClCharT *xportType, const ClCharT *devIf, ClCharT *hostAddress, ClCharT *networkMask, ClCharT *broadcast, ClUint32T *ipAddressMask, ClCharT *xportSubnetPrefix)
{
    ClCharT envSubNetType[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT xportSubnet[CL_MAX_FIELD_LENGTH] = { 0 };
    ClCharT *subnetMask = NULL;
    ClCharT *subnetPrefix = NULL;
    ClUint32T CIDR, ipMask, ip, mask;
    ClRcT rc;
    
    if (!xportType)
    {
        return CL_ERR_INVALID_PARAMETER;
    } 
    else 
    {
        snprintf(envSubNetType, CL_MAX_FIELD_LENGTH, "ASP_%s_SUBNET", xportType);
        subnetMask = getenv(envSubNetType);
        if (subnetMask == NULL) 
        {
            subnetMask = (ClCharT*) ASP_PLUGIN_SUBNET_DEFAULT;
        }
        subnetPrefix = strrchr(subnetMask, '/');
        if(!subnetPrefix)
        {
            subnetPrefix = (ClCharT*) ASP_PLUGIN_SUBNET_PREFIX_DEFAULT; 
        }
        else 
        {
            ++subnetPrefix;
        }
        if(! (CIDR = atoi(subnetPrefix) ) )
        {
            clLogInfo("UDP", "INI", "Subnet mask configuration setting (ASP_[transport]_SUBNET) cannot be parsed [%s]", subnetMask);
            return CL_ERR_INVALID_PARAMETER;
        }
        xportSubnetPrefix[0] = 0;
        strncat(xportSubnetPrefix, subnetPrefix, CL_MAX_FIELD_LENGTH-1);
        snprintf(xportSubnet, sizeof(xportSubnet), "%s", subnetMask);
        mask = clPluginHelperBitFillRShift(CIDR);
        clPluginHelperConvertInternetToHostAddress(&ip, xportSubnet);

        /* network address */
        ipMask = (ip & mask);


        /* Try to get address from devif */
        
        rc = CL_OK+1;  /* start with any error condition, so the if below this one will be taken  */
        if (ASP_UDP_USE_EXISTING_IP)
        {
            rc = clPluginHelperDevToIpAddress(devIf, hostAddress);
            if (rc == CL_OK) clLogInfo("UDP","INI","Use existing IP address [%s] as this nodes transport address.", hostAddress);
            else clLogError("UDP","INI","Configured to use an existing IP address for message transport.  But address lookup failed on device [%s] error [0x%x]", devIf, rc);
        }
        
        if (rc != CL_OK)
        {
             /* Automatic assignment of IP address */
            clPluginHelperConvertHostToInternetAddress(ipMask + gIocLocalBladeAddress, hostAddress);
        }

        clPluginHelperConvertHostToInternetAddress(mask, networkMask);
        clPluginHelperConvertHostToInternetAddress(ip | ~mask, broadcast);
        *ipAddressMask = ipMask;
    }
    return CL_OK;
}


#ifdef HAVE_SCTP

void initSctpCtrlSpace(void)
{
    struct sctp_sndrcvinfo *sndrcvInfo;
    static ClUint8T cMsgSpace[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    gClCmsgHdr = (struct cmsghdr *)cMsgSpace;
    gClCmsgHdrLen = CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
    gClCmsgHdr->cmsg_type = SCTP_SNDRCV;
    gClCmsgHdr->cmsg_level = IPPROTO_SCTP;
    gClCmsgHdr->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
    sndrcvInfo = (struct sctp_sndrcvinfo*)CMSG_DATA(gClCmsgHdr);
    memset(sndrcvInfo, 0, sizeof(*sndrcvInfo));
}

static void initSctp(void)
{
    clLogNotice("INIT", "SCTP", "SCTP mode enabled for UDP transport");
    gClSockType = SOCK_SEQPACKET;
    gClProtocol = IPPROTO_SCTP;
    initSctpCtrlSpace();
}

#else

static void initSctp(void)
{
    clLogNotice("INIT", "SCTP", "Not using SCTP mode for UDP as sctp support isn't available");
    clLogNotice("INIT", "SCTP", "Try installing libsctp-dev to enable sctp");
    gClSockType = SOCK_DGRAM;
    gClProtocol = IPPROTO_UDP;
}

#endif

static ClRcT checkInitSctp(void)
{
    ClRcT rc = CL_OK;
    ClParserPtrT parent = NULL, child = NULL;
    ClCharT *config = getenv("ASP_CONFIG");
    const ClCharT *mode;

    if(!config)
        config = (ClCharT*)".";

    parent = clParserOpenFile(config, CL_TRANSPORT_CONFIG_FILE);
    if(!parent)
    {
        clLogWarning("INIT", "SCTP", "Unable to check for sctp mode as config file [%s] is absent at [%s]",
                     CL_TRANSPORT_CONFIG_FILE, config);
        rc = CL_ERR_NOT_EXIST;
        goto out;
    }
    child = clParserChild(parent, "sctp");
    if(!child)
    {
        rc = CL_ERR_NOT_EXIST;
        goto out_free;
    }
    mode = clParserAttr(child, "mode");
    if(!strncasecmp(mode, "on", 2) || !strncasecmp(mode, "enabled", 7))
    {
        initSctp();
    }
    rc = CL_OK;

    out_free:
    if(parent)
        clParserFree(parent);

    out:
    return rc;
}

ClRcT xportInit(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep)
{
    ClRcT rc = CL_OK;
    ClIocUdpMapT *map = NULL;

    /* Initialize plugin global variables */
    if(xportType)
    {
        gClUdpXportType[0] = 0;
        strncat(gClUdpXportType, xportType, sizeof(gClUdpXportType)-1);
    }
    gClNodeRepresentative = nodeRep;
    gClUdpXportId = xportId;
    gClBindOffset = gIocLocalBladeAddress;
    ASP_UDP_USE_EXISTING_IP = clParseEnvBoolean("ASP_UDP_USE_EXISTING_IP");
    gClSimulationMode = clParseEnvBoolean("ASP_MULTINODE");
    if(gClSimulationMode)
    {
        clLogNotice("XPORT", "INIT", "Simulation mode is enabled for the runtime");
        gClBindOffset <<= 10;
    }

    /* check to see if SCTP protocol chosen, and if so, init it */
    checkInitSctp();

    /* Determine the device, IP address and IP network that will be used */
    rc = clUdpGetBackplaneInterface(xportType, gVirtualIp.dev);
    if (rc != CL_OK)
    {
        clLogError("UDP", "INI", "Unable to determine the backplane link: error [%#x]", rc);
        return rc;
    }

    rc = clUdpGetNodeIpAddress(xportType, gVirtualIp.dev,gVirtualIp.ip,gVirtualIp.netmask,gVirtualIp.broadcast,&gVirtualIp.ipAddressMask,gVirtualIp.subnetPrefix);
    if (rc != CL_OK)
    {
        clLogError("UDP", "INI", "Unable to determine the backplane IP Address: error [%#x]", rc);
        return rc;
    }
    
    clLogInfo("UDP", "INI", "Backplane: Link Name: %s, IP Node Address: %s, Network Address: %s, Broadcast: %s, Subnet Prefix: %s, Address Mask: 0x%x", gVirtualIp.dev, gVirtualIp.ip, gVirtualIp.netmask, gVirtualIp.broadcast,gVirtualIp.subnetPrefix,gVirtualIp.ipAddressMask);

    /*
     * To do a fast pass early update node entry table
     */
    clIocNotificationRegister(_clUdpMapUpdateNotification, NULL);

    // TODO: identify from linkName
    gXportCtrl.family = PF_INET;

    rc = clOsalMutexInit(&gXportCtrl.mutex);
    CL_ASSERT(rc == CL_OK);

    rc = clUdpAddrCacheInitialize(nodeRep);
    CL_ASSERT(rc == CL_OK);

    //Add to map with default this node
    iocUdpMapAdd(gVirtualIp.ip, gIocLocalBladeAddress);

    ClUint32T i;
    ClUint32T numNodes = 0;
    ClIocNodeAddressT *pNodes = NULL;
    ClCharT addStr[INET_ADDRSTRLEN] = {0};

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
        if (pNodes[i] != clIocLocalAddressGet() && !(map = iocUdpMapFind(pNodes[i])))
        {
            addStr[0] = 0;
            clUdpAddrGet(pNodes[i], addStr);
            /* Sync IP addresses from shm */
            iocUdpMapAdd(addStr, pNodes[i]);
        }
    }

    clHeapFree(pNodes);

    out:
    gUdpInit = CL_TRUE;
    rc = CL_OK;
    return rc;
}

ClRcT xportFinalize(ClInt32T xportId, ClBoolT nodeRep)
{
    clUdpAddrCacheFinalize(nodeRep);
    return CL_OK;
}

static ClRcT udpDispatchCallback(ClInt32T fd, ClInt32T events, void *cookie)
{
    ClRcT rc = CL_OK;
    ClIocUdpPrivateT *xportPrivate = (ClIocUdpPrivateT*) cookie;
    ClUint8T buffer[0xffff+1];
    struct msghdr msgHdr;
    struct sockaddr peerAddress;
    struct iovec ioVector[1];
    ClInt32T bytes;

    if(!xportPrivate)
    {
        clLogError(UDP_LOG_AREA_UDP,CL_LOG_CONTEXT_UNSPECIFIED,"No private data\n");
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    memset(&msgHdr, 0, sizeof(msgHdr));
    memset(ioVector, 0, sizeof(ioVector));
    msgHdr.msg_name = &peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    msgHdr.msg_control = (ClUint8T*)gClCmsgHdr;
    msgHdr.msg_controllen = gClCmsgHdrLen;
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
        clLogError(UDP_LOG_AREA_UDP,CL_LOG_CONTEXT_UNSPECIFIED,"recv error. errno = %d\n",errno);
        rc = CL_ERR_LIBRARY;
        goto out;
    }

    rc = clIocDispatchAsync(gClUdpXportType, xportPrivate->port, buffer, bytes);

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
        fd = socket(PF_INET6, gClSockType, gClProtocol);
        addr = (struct sockaddr*)&ipv6_addr;
        ipv6_addr.sin6_addr = in6addr_any;
        ipv6_addr.sin6_port = htons(port + CL_TRANSPORT_BASE_PORT + gClBindOffset);
        ipv6_addr.sin6_family = PF_INET6;
        break;
    case PF_INET:
    default:
        fd = socket(PF_INET, gClSockType, gClProtocol);
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
    if(gClCmsgHdr)
    {
        if(listen(fd, CL_IOC_MAX_NODES) < 0)
        {
            clLogError("UDP", "LISTEN", "Listen failed on socket with error [%s]",
                       strerror(errno));
            goto out_close;
        }
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

    xportPrivate = (ClIocUdpPrivateT*) clHeapCalloc(1, sizeof(*xportPrivate));
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
    if(!(xportPrivate = (ClIocUdpPrivateT*) clTransportPrivateDataDelete(gClUdpXportId, port))) 
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
        xportPrivate = (ClIocUdpPrivateT*) clHeapCalloc(1, sizeof(*xportPrivate));
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
    if(!(xportPrivate = (ClIocUdpPrivateT*) clTransportPrivateDataDelete(gClUdpXportId, port)))
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
                    clLogError(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_RECV,"recv error. errno = %d\n",errno);
                    goto out;
                }
            } 
            else 
            {
                clLogError(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_RECV,"poll error. errno = %d\n", errno);
                goto out;
            }
        } 
        else if(pollStatus < 0 ) 
        {
            if(errno == EINTR)
                continue;
            clLogError(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_RECV,"Error in poll. errno = %d\n",errno);
            goto out;
        } 
        else 
        {
            rc = CL_ERR_TIMEOUT;
            goto out;
        }
        break;
    }

    rc = clIocDispatch(gClUdpXportType, commPort, pRecvOption, pBuffer, bytes, message, pRecvParam);

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
            clLogCritical(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_RECV,"Dropping a received fragmented-packet. "
                                              "Could not receive the complete packet within "
                                              "the specified timeout. Packet size is %d", bytes);

        }
    }

    out:
    return rc;

}

static ClRcT iocUdpSend(ClIocUdpMapT *map, void *args)
{
    ClIocUdpArgsT *sendArgs = (ClIocUdpArgsT*) args;
    struct sockaddr *destaddr = NULL;
    socklen_t addrlen = 0;
    struct msghdr msghdr;
    ClRcT rc = CL_OK;
    int portOffset = map->slot;
    ClUint32T priority;

    if ((udpPriorityChangePossible) && (map->priority != sendArgs->priority))
    {
        priority = sendArgs->priority;
        if(!setsockopt(map->sendFd, SOL_IP, IP_TOS, &priority, sizeof(priority)))
        {
            map->priority = priority;
        }
        else
        {
            int err = errno;

            if (err == ENOPROTOOPT)
            {
                udpPriorityChangePossible = CL_FALSE;
                clLogWarning(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_SEND,"Message priority not available in this version of UDP.");
            }
            else clLogWarning(UDP_LOG_AREA_UDP,UDP_LOG_CTX_UDP_SEND,"Error in setting UDP message priority. errno [%d]",err);
        }
    }

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
    msghdr.msg_control = (ClUint8T*)gClCmsgHdr;
    msghdr.msg_controllen = gClCmsgHdrLen;
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
    ClCharT addStr[INET_ADDRSTRLEN] = {0};

    if(!address || !iovlen)
        return CL_OK;
    sendArgs.priority = CL_UDP_DEFAULT_PRIORITY;
    if (priority == CL_IOC_HIGH_PRIORITY)
        sendArgs.priority = CL_UDP_HIGH_PRIORITY;
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
            clUdpAddrGet(address->iocPhyAddress.nodeAddress, addStr);
            map = iocUdpMapAdd(addStr, address->iocPhyAddress.nodeAddress);
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
        rc = clUdpMapWalk(iocUdpSend, &sendArgs, 0);
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
            rc = clUdpNodeNotification(node, event);
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
        rc = clUdpNodeNotification(nodeAddress, event);
    }
    else
    {
        clUdpNotify(nodeAddress, port, CL_IOC_COMP_DEATH_NOTIFICATION);
        clTransportNotifyClose(port);
    }
    return rc;
}

/*
 * Return fd for local port
 */
ClRcT xportFdGet(ClIocCommPortHandleT commPort, ClInt32T *fd)
{
    ClIocCommPortT *pCommPort = (ClIocCommPortT*)commPort;
    return clUdpFdGet(pCommPort->portId, fd);
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

ClRcT clUdpAddrSet(ClIocNodeAddressT nodeAddress, const ClCharT *addrStr)
{
    ClRcT rc = CL_OK;

    rc = clOsalSemLock(gClUdpAddrCacheSem);
    if (rc != CL_OK)
        return rc;

    if (!gpClUdpAddrCache)
    {
        clOsalSemUnlock(gClUdpAddrCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    memset(gpClUdpAddrCache + (sizeof(ClUdpAddrCacheEntryT) * nodeAddress), 0, INET_ADDRSTRLEN);
    memcpy(gpClUdpAddrCache + (sizeof(ClUdpAddrCacheEntryT) * nodeAddress), addrStr, INET_ADDRSTRLEN);

    clOsalSemUnlock(gClUdpAddrCacheSem);

    clLogTrace("UDP", "CACHE", "Setting address cache entry for node [%d: %s]", nodeAddress, addrStr);

    return rc;
}

ClRcT clUdpAddrGet(ClIocNodeAddressT nodeAddress, ClCharT *addrStr)
{
    ClRcT rc = CL_OK;

    rc = clOsalSemLock(gClUdpAddrCacheSem);
    if (rc != CL_OK)
        return rc;

    if (!gpClUdpAddrCache)
    {
        clOsalSemUnlock(gClUdpAddrCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    memcpy(addrStr, gpClUdpAddrCache + (sizeof(ClUdpAddrCacheEntryT) * nodeAddress), INET_ADDRSTRLEN);
    clOsalSemUnlock(gClUdpAddrCacheSem);

    clLogTrace("UDP", "CACHE", "Getting address cache entry for node [%d: %s]", nodeAddress, addrStr);

    return rc;
}

#ifdef __cplusplus
 }
#endif
