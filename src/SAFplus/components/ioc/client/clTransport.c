#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <clTransport.h>
#include <clLogApi.h>
#include <clParserApi.h>
#include <clDebugApi.h>
#include <clList.h>
#include <clParserApi.h>
#include <clHash.h>
#include <clDebugApi.h>
#include <clTaskPool.h>
#include "clIocUserApi.h"
#include "clIocGeneral.h"
#include "clIocNeighComps.h"
#include <clEoIpi.h>
#include <clCpmApi.h>
#include <clTimerApi.h>
#include <clClmTmsCommon.h>
#include <clNodeCache.h>

#define CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT_HEADROOM (100)
#define CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT (64000 - CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT_HEADROOM)
#define CL_XPORT_DEFAULT_TYPE "tipc"
#define CL_XPORT_DEFAULT_PLUGIN "libClTIPC.so"
typedef struct ClTransportLayer
{
#define XPORT_STATE_INITIALIZED  (0x1)
    ClInt32T xportId;
    ClCharT *xportType;
    ClCharT *xportPlugin;
    ClPtrT xportPluginHandle;
    ClInt32T xportState;
    ClRcT (*xportIpAddressAssign)(void);
    ClRcT (*xportInit)(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep);
    ClRcT (*xportFinalize)(ClInt32T xportId, ClBoolT nodeRep);
    ClRcT (*xportNotifyInit)(void);
    ClRcT (*xportNotifyFinalize)(void);
    ClRcT (*xportNotifyOpen)(ClIocPortT port);
    ClRcT (*xportNotifyClose)(ClIocNodeAddressT nodeAddress, ClIocPortT port);
    /*
     * Use bind and bindclose if you want to do a synchronous xport recv. 
     * over an async listen.
     */
    ClRcT (*xportBind)(ClIocPortT port);
    ClRcT (*xportBindClose)(ClIocPortT port);
    ClRcT (*xportListen)(ClIocPortT port);
    ClRcT (*xportListenStop)(ClIocPortT port);
    ClRcT (*xportSend)(ClIocPortT port, ClUint32T priority, ClIocAddressT *, 
                       struct iovec *iov, ClInt32T iovlen, ClInt32T flags);
    ClRcT (*xportRecv)(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, 
                       ClUint8T *buffer, ClUint32T bufSize, ClBufferHandleT message,
                       ClIocRecvParamT *pRecvParam);
    ClRcT (*xportMaxPayloadSizeGet)(ClUint32T *pSize);
    ClRcT (*xportServerReady)(ClIocAddressT *pAddress);
    ClRcT (*xportMasterAddressGet)(ClIocLogicalAddressT la, ClIocPortT port, 
                                   ClIocNodeAddressT *masterAddress);
    ClRcT (*xportTransparencyRegister)(ClIocPortT port, ClIocLogicalAddressT logicalAddr, ClUint32T haState);
    ClRcT (*xportTransparencyDeregister)(ClIocPortT port, ClIocLogicalAddressT logicalAddr);
    ClRcT (*xportMulticastRegister)(ClIocPortT port, ClIocMulticastAddressT mcastAddr);
    ClRcT (*xportMulticastDeregister)(ClIocPortT port, ClIocMulticastAddressT mcastAddr);
    ClListHeadT xportList; 
} ClTransportLayerT;

typedef struct ClEventData
{
    ClInt32T fd;
    ClInt32T index;
}ClEventDataT;

typedef struct ClXportListener
{
    ClInt32T fd;
    ClListHeadT list;
}ClXportListenerT;

typedef struct ClPollEvent
{
    ClInt32T fd;
    ClInt32T events;
    void *cookie;
    ClRcT (*dispatch)(ClInt32T fd, ClInt32T events, void *cookie);
}ClPollEventT;

typedef struct ClXportCtrl
{
    ClBoolT running;
    ClListHeadT listenerList;
    ClOsalMutexT mutex;
    ClOsalCondT cond;
    ClTaskPoolHandleT pool;
    ClInt32T eventFd;
    ClPollEventT *pollfds;
    ClUint32T numfds;
    ClInt32T breaker[2];
}ClXportCtrlT;

#define CL_XPORT_PRIVATE_TABLE_BITS (12)
#define CL_XPORT_PRIVATE_TABLE_MAX (1 << CL_XPORT_PRIVATE_TABLE_BITS)
#define CL_XPORT_PRIVATE_TABLE_MASK (CL_XPORT_PRIVATE_TABLE_MAX - 1)
#define __TRANSPORT_PRIVATE_KEY(fd, port) ( ((fd) << 8) | ( (port) & 0xff ) )
#define __TRANSPORT_PRIVATE_HASH(fd, port) (__TRANSPORT_PRIVATE_KEY(fd, port) & CL_XPORT_PRIVATE_TABLE_MASK )
static struct hashStruct *gXportPrivateTable[CL_XPORT_PRIVATE_TABLE_MAX];

#define CL_XPORT_NODEADDR_TABLE_BITS (8)
#define CL_XPORT_NODEADDR_TABLE_MAX (1 << CL_XPORT_NODEADDR_TABLE_BITS)
#define CL_XPORT_NODEADDR_TABLE_MASK (CL_XPORT_NODEADDR_TABLE_MAX - 1)
#define CL_XPORT_NODEADDR_TABLE_HASH(addr) ( (addr) & CL_XPORT_NODEADDR_TABLE_MASK )

typedef struct ClXportNodeAddrData
{
    ClIocNodeAddressT slot;
    ClCharT **xports;
    ClUint32T numXport;
    ClBoolT bridge;
    struct hashStruct hash; /*hash linkage*/
    ClListHeadT list; /*list linkage*/
} ClXportNodeAddrDataT;

/* This struct using for lookup a destination node address */
typedef struct ClXportDestNodeLUTData
{
    ClIocNodeAddressT destIocNodeAddress;       /* Destination node address */
    ClIocNodeAddressT bridgeIocNodeAddress;     /* Interim destination node address */
    ClCharT *xportType;                         /* Protocol to send */
    struct hashStruct hash;                     /* hash linkage */
    ClListHeadT list;                           /* list linkage */
} ClXportDestNodeLUTDataT;

/*
 * Global hash table which will have all the xport to ioc node address mapping
 * It will update when node arrive/death.
 * If arrive but not configure, update xport as default
 */
static struct hashStruct *gClXportNodeAddrHashTable[CL_XPORT_NODEADDR_TABLE_MAX];
static CL_LIST_HEAD_DECLARE(gClXportNodeAddrList);

/*
 * Global hash table to store destination node address
 */
static struct hashStruct *gClXportDestNodeLUTHashTable[CL_XPORT_NODEADDR_TABLE_MAX];
static CL_LIST_HEAD_DECLARE(gClXportDestNodeLUTList);

static ClOsalMutexT gXportNodeAddrListmutex;

static void _clSetupDestNodeLUTData(void);

static ClXportNodeAddrDataT gClXportDefaultNodeName;
/*
 * Global xports ID
 */
ClInt32T gClTransportId;

#define MULTICAST_ADDR_DEFAULT "224.1.1.1"
#define MULTICAST_PORT_DEFAULT 5678

/* Multicast address specified in the config file */
static ClCharT gClTransportMcastAddr[CL_MAX_NAME_LENGTH];
/* Multicast port number specified in the config file */
static ClInt32T gClTransportMcastPort;
static ClUint32T gGmsInitDone = CL_FALSE;
static ClBoolT gLocalNodeBridge = CL_FALSE;
static ClTimerHandleT gHandleGmsTimer = CL_HANDLE_INVALID_VALUE;
static ClGmsHandleT gXportHandleGms = CL_HANDLE_INVALID_VALUE;

typedef struct ClXportPrivateData
{
    ClInt32T fd;
    ClIocPortT port;
    void *private;
    struct hashStruct hash;
}ClXportPrivateDataT;

static ClXportCtrlT gXportCtrl = {
    .listenerList = CL_LIST_HEAD_INITIALIZER(gXportCtrl.listenerList),
    .pollfds = NULL,
    .numfds = 0,
};

static CL_LIST_HEAD_DECLARE(gClTransportList);
static ClCharT gClXportDefaultType[CL_MAX_NAME_LENGTH];
static ClTransportLayerT *gClXportDefault;
static ClBoolT gXportNodeRep;

#define XPORT_CONFIG_FILE "clTransport.xml"

static ClRcT xportIpAddressAssignFake(void)
{
    clLogNotice("XPORT", "IP_ASSIGN", "Inside fake transport initialize");
    return CL_OK;
}

static ClRcT xportInitFake(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep)
{ 
    clLogNotice("XPORT", "INIT", "Inside fake transport initialize");
    return CL_OK; 
}

static ClRcT xportFinalizeFake(ClInt32T xportId, ClBoolT nodeRep)
{ 
    clLogNotice("XPORT", "FIN", "Inside fake transport finalize");
    return CL_OK; 
}

static ClRcT xportBindFake(ClIocPortT port)
{
    clLogNotice("XPORT", "BIND", "Inside fake transport bind for port [%d]", port);
    return CL_OK;
}

static ClRcT xportBindCloseFake(ClIocPortT port)
{
    clLogNotice("XPORT", "BIND", "Inside fake transport bind close for port [%d]", port);
    return CL_OK;
}

static ClRcT xportListenFake(ClIocPortT port)
{
    clLogNotice("XPORT", "LISTEN", "Inside fake transport listen for port [%d]", port);
    return CL_OK; 
}

static ClRcT xportListenStopFake(ClIocPortT port)
{
    clLogNotice("XPORT", "LISTEN-STOP", "Inside fake transport listen stop");
    return CL_OK;
}

static ClRcT xportNotifyOpenFake(ClIocPortT port)
{
    return CL_ERR_NOT_SUPPORTED;
}

static ClRcT xportNotifyCloseFake(ClIocNodeAddressT nodeAddress, ClIocPortT port)
{
    return CL_ERR_NOT_SUPPORTED;
}

static ClRcT xportNotifyInitFake(void)
{
    return CL_ERR_NOT_SUPPORTED;
}

static ClRcT xportNotifyFinalizeFake(void)
{
    return CL_ERR_NOT_SUPPORTED;
}

static ClRcT xportSendFake(ClIocPortT port, ClUint32T priority, ClIocAddressT *addr, 
                           struct iovec *iov, ClInt32T iovlen, ClInt32T flags) 
{ 
    if(!addr) return CL_OK;
    switch(CL_IOC_ADDRESS_TYPE_GET(addr))
    {
    case CL_IOC_PHYSICAL_ADDRESS_TYPE:
        clLogNotice("XPORT", "SEND", "Inside fake transport send for node [%d], port [%#x]",
                    addr->iocPhyAddress.nodeAddress, addr->iocPhyAddress.portId);
        break;
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        clLogNotice("XPORT", "SEND", "Inside fake transport send for logical address [%#llx]",
                    addr->iocLogicalAddress);
        break;
    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        clLogNotice("XPORT", "SEND", "Inside fake transport send for multicast address [%#llx]",
                    addr->iocMulticastAddress);
        break;
    default:
        break;
    }
    return CL_OK; 
}

static ClRcT xportRecvFake(ClIocCommPortHandleT port, ClIocDispatchOptionT *pRecvOption, 
                           ClUint8T *buffer, ClUint32T bufSize,
                           ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{ 
    clLogNotice("XPORT", "RECV", "Inside fake transport recv");
    return CL_OK; 
}

static ClRcT xportServerReadyFake(ClIocAddressT *pAddress)
{ 
    clLogNotice("XPORT", "READY", "Inside fake xport servery ready");
    return CL_OK; 
}

static ClRcT xportMasterAddressGetFake(ClIocLogicalAddressT la, ClIocPortT port, ClIocNodeAddressT *masterAddress)
{ 
    clLogNotice("XPORT", "MASTER", "Inside fake master address get");
    return CL_ERR_NOT_SUPPORTED;
}

static ClRcT xportTransparencyRegisterFake(ClIocPortT port, ClIocLogicalAddressT logicalAddr, ClUint32T haState)
{ 
    clLogNotice("XPORT", "TL", "Inside fake transport transparency register for LA [%#llx], "
                "haState [%s]",
                logicalAddr, haState == CL_IOC_TL_ACTIVE ? "active" : "standby");
    return CL_OK; 
}

static ClRcT xportTransparencyDeregisterFake(ClIocPortT port, ClIocLogicalAddressT logicalAddr)
{ 
    clLogNotice("XPORT", "TL", "Inside fake transport transparency deregister for LA [%#llx]", 
                logicalAddr);
    return CL_OK; 
}

static ClRcT xportMulticastRegisterFake(ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{ 
    clLogNotice("XPORT", "MCAST", "Inside fake transport multicast register for multicast [%#llx]",
                mcastAddr);
    return CL_OK; 
}

static ClRcT xportMulticastDeregisterFake(ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{ 
    clLogNotice("XPORT", "MCAST", "Inside fake transport multicast deregister for multicast [%#llx]",
                mcastAddr);
    return CL_OK; 
}

static __inline__ void _clXportNodeAddrMapAdd(ClXportNodeAddrDataT *entry)
{
    ClUint32T hash = CL_XPORT_NODEADDR_TABLE_HASH(entry->slot);
    hashAdd(gClXportNodeAddrHashTable, hash, &entry->hash);
    clListAddTail(&entry->list, &gClXportNodeAddrList);
}

static __inline__ void _clXportNodeAddrMapDel(ClXportNodeAddrDataT *entry)
{
    int indexXport = 0;
    for (indexXport = 0; indexXport < entry->numXport; indexXport++)
    {
        clHeapFree(entry->xports[indexXport]);
    }
    hashDel(&entry->hash);
    clListDel(&entry->list);
    clHeapFree(entry->xports);
    clHeapFree(entry);
}

static __inline__ ClXportNodeAddrDataT *_clXportNodeAddrMapFind(ClIocNodeAddressT slot)
{
    ClUint32T hash = CL_XPORT_NODEADDR_TABLE_HASH(slot);
    register struct hashStruct *iter;
    for(iter = gClXportNodeAddrHashTable[hash]; iter; iter = iter->pNext)
    {
        ClXportNodeAddrDataT *entry = hashEntry(iter, ClXportNodeAddrDataT, hash);
        if (entry->slot == slot)
        {
            return entry;
        }
    }
    return NULL;
}

static void _clXportNodeAddrMapFree()
{
    register ClListHeadT *iter;
    // Free default xport type
    int i = 0;
    for (i = 0; i < gClXportDefaultNodeName.numXport; i++)
    {
        clHeapFree(gClXportDefaultNodeName.xports[i]);
    }

    clHeapFree(gClXportDefaultNodeName.xports);

    // Free each entry on xport node address mapping
    while(!CL_LIST_HEAD_EMPTY(&gClXportNodeAddrList))
    {
        iter = gClXportNodeAddrList.pNext;
        ClXportNodeAddrDataT *entry = CL_LIST_ENTRY(iter, ClXportNodeAddrDataT, list);
        _clXportNodeAddrMapDel(entry);
    }
}

/*
 * Add an entry data of destination node into hash table
 */
static __inline__ void _clXportDestNodeLUTMapAdd(ClXportDestNodeLUTDataT *entry)
{
    clLogInfo(
               "IOC",
               "LUT",
               "Add new entry for LUT, dest node [%#x] bridge [%#x] protocol [%s]", 
               entry->destIocNodeAddress, entry->bridgeIocNodeAddress, entry->xportType);
    ClUint32T hash = CL_XPORT_NODEADDR_TABLE_HASH(entry->destIocNodeAddress);
    hashAdd(gClXportDestNodeLUTHashTable, hash, &entry->hash);
    clListAddTail(&entry->list, &gClXportDestNodeLUTList);
}

/*
 * Remove and entry data of destination node out of hash table
 */
static __inline__ void _clXportDestNodeLUTMapDel(ClXportDestNodeLUTDataT *entry)
{
    clHeapFree(entry->xportType);
    hashDel(&entry->hash);
    clListDel(&entry->list);
    clHeapFree(entry);
}

/*
 * Terminal all entry in Dest Node LUT Data
 */
static ClRcT _clXportDestNodeLUTMapFree()
{
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    while(!CL_LIST_HEAD_EMPTY(&gClXportDestNodeLUTList))
    {
        iter = gClXportDestNodeLUTList.pNext;
        ClXportDestNodeLUTDataT *map = CL_LIST_ENTRY(iter, ClXportDestNodeLUTDataT, list);
        _clXportDestNodeLUTMapDel(map);
    }
    return rc;
}

static __inline__ ClXportDestNodeLUTDataT *_clXportDestNodeLUTMapFind(ClIocNodeAddressT iocAddress)
{
    ClUint32T hash = CL_XPORT_NODEADDR_TABLE_HASH(iocAddress);
    register struct hashStruct *iter;
    for(iter = gClXportDestNodeLUTHashTable[hash]; iter; iter = iter->pNext)
    {
        ClXportDestNodeLUTDataT *entry = hashEntry(iter, ClXportDestNodeLUTDataT, hash);
        if (entry->destIocNodeAddress == iocAddress)
        {
            return entry;
        }
    }
    return NULL;
}

static void _clXportUpdateNodeConfig(ClIocNodeAddressT node)
{
    ClUint32T i = 0;

    ClXportNodeAddrDataT *nodeAddrConfig = clHeapCalloc(1, sizeof(*nodeAddrConfig));
    CL_ASSERT(nodeAddrConfig != NULL);
    nodeAddrConfig->slot = node;
    nodeAddrConfig->numXport = gClXportDefaultNodeName.numXport;
    nodeAddrConfig->bridge = gClXportDefaultNodeName.bridge;
    nodeAddrConfig->xports = (ClCharT **) clHeapCalloc(nodeAddrConfig->numXport, sizeof(ClCharT *));
    CL_ASSERT(nodeAddrConfig->xports != NULL);
    for (i = 0; i < nodeAddrConfig->numXport; i++) {
        nodeAddrConfig->xports[i] = clStrdup(gClXportDefaultNodeName.xports[i]);
    }
    clLogDebug("IOC", "LUT", "Add Ioc node address config [%#x]", node);
    _clXportNodeAddrMapAdd(nodeAddrConfig);
}

static ClRcT _clTransportGmsTimerInitCallback() {
    ClRcT rc = CL_OK;
    ClVersionT version = { 'B', 0x1, 0x1 };
    ClGmsCallbacksT gGmsCallbacks = { NULL, NULL, NULL, NULL, };
    ClTimerTimeOutT timeout = { 0, 1000 };
    ClBoolT *pData = NULL;

    if (gGmsInitDone == CL_FALSE) {
        ClUint8T tempStatus;
        ClIocPhysicalAddressT compAddr = { 0 };
        compAddr.nodeAddress = gIocLocalBladeAddress;
        compAddr.portId = CL_IOC_GMS_PORT;
        rc = clIocCompStatusGet(compAddr, &tempStatus);
        if (rc != CL_OK || tempStatus == CL_IOC_NODE_DOWN)
        {
            goto retry;
        }
        rc = clGmsInitialize(&gXportHandleGms, &gGmsCallbacks, &version);
        if (CL_OK != rc) {
            retry:
            if (CL_HANDLE_INVALID_VALUE != gHandleGmsTimer) {
                clTimerDelete(&gHandleGmsTimer);
                gHandleGmsTimer = CL_HANDLE_INVALID_VALUE;
            }
            rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT,
                    CL_TIMER_SEPARATE_CONTEXT, _clTransportGmsTimerInitCallback,
                    pData, &gHandleGmsTimer);
            if (CL_OK != rc) {
                clLogError("IOC", "INIT",
                        "Failed to do GMS initialization, error [%#x]", rc);
            }
            return CL_OK;
        }
        clTimerDelete(&gHandleGmsTimer);
        gGmsInitDone = CL_TRUE;
    }
    return rc;
}

/* 
 * Update node lookup table from notification
 */
static ClRcT clTransportDestNodeLUTUpdate(ClIocNotificationIdT notificationId, ClIocNodeAddressT nodeAddr)
{
    register ClListHeadT *iter;
    clOsalMutexLock(&gXportNodeAddrListmutex);
    switch (notificationId)
    {
        case CL_IOC_NODE_ARRIVAL_NOTIFICATION:
        case CL_IOC_COMP_ARRIVAL_NOTIFICATION:
        {
            clLogDebug("IOC", "LUT", "Triggering node join for node [0x%x]", nodeAddr);
            if (!_clXportNodeAddrMapFind(nodeAddr)) {
                _clXportUpdateNodeConfig(nodeAddr);
                _clSetupDestNodeLUTData();
            } else {
                clLogDebug("IOC", "LUT", "Nothing update for node [%#x]", nodeAddr);
            }
            break;
        }
        case CL_IOC_NODE_LEAVE_NOTIFICATION:
        {
            clLogNotice("IOC", "LUT", "Triggering node leave for node [0x%x]", nodeAddr);
            CL_LIST_FOR_EACH(iter, &gClXportDestNodeLUTList) {
                ClXportDestNodeLUTDataT *map = CL_LIST_ENTRY(iter, ClXportDestNodeLUTDataT, list);
                if (map->destIocNodeAddress == nodeAddr || map->bridgeIocNodeAddress == nodeAddr) {
                    clLogDebug("IOC", "LUT", "Remove entry out of DestNodeLUT for node [%#x]", nodeAddr);
                    _clXportDestNodeLUTMapDel(map);
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
    clOsalMutexUnlock(&gXportNodeAddrListmutex);
    return CL_OK;
}

static ClRcT _clXportDestNodeLUTUpdateNotification(ClIocNotificationT *notification, ClPtrT cookie)
{
    ClIocNotificationIdT notificationId = ntohl(notification->id);
    ClIocNodeAddressT iocAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
    if (!portId || portId == CL_IOC_CPM_PORT || portId == CL_IOC_XPORT_PORT)
    {
        clTransportDestNodeLUTUpdate(notificationId, iocAddress);
    }
    return CL_OK;
}

static ClRcT xportMaxPayloadSizeGetDefault(ClUint32T *size)
{
    if(size)
        *size = CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT - 100;
    return CL_OK;
}

static ClTransportLayerT *findTransport(const ClCharT *type)
{
    register ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClTransportLayerT *entry = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(!strncasecmp(entry->xportType, type, strlen(type))) return entry;
    }
    return NULL;
}

static void addTransport(const ClCharT *type, const ClCharT *plugin)
{
    ClTransportLayerT *xport = clHeapCalloc(1, sizeof(*xport));
    CL_ASSERT(xport != NULL);
    xport->xportType = clStrdup(type);
    CL_ASSERT(xport->xportType != NULL);
    xport->xportPlugin = clStrdup(plugin);
    CL_ASSERT(xport->xportPlugin != NULL);
    xport->xportPluginHandle = dlopen(plugin, RTLD_GLOBAL | RTLD_NOW);
    if(!xport->xportPluginHandle)
    {
        char* error = dlerror();
        clLogError("XPORT", "LOAD", "Unable to load plugin [%s]. Error [%s]", plugin, error ? error : "unknown");
        goto out_free;
    }
    *(void**)&xport->xportIpAddressAssign = dlsym(xport->xportPluginHandle, "xportIpAddressAssign");
    *(void**)&xport->xportInit = dlsym(xport->xportPluginHandle, "xportInit");
    *(void**)&xport->xportFinalize = dlsym(xport->xportPluginHandle, "xportFinalize");

    *(void**)&xport->xportNotifyInit = dlsym(xport->xportPluginHandle, "xportNotifyInit");
    *(void**)&xport->xportNotifyFinalize = dlsym(xport->xportPluginHandle, "xportNotifyFinalize");
    *(void**)&xport->xportNotifyOpen = dlsym(xport->xportPluginHandle, "xportNotifyOpen");
    *(void**)&xport->xportNotifyClose = dlsym(xport->xportPluginHandle, "xportNotifyClose");

    *(void **)&xport->xportBind = dlsym(xport->xportPluginHandle, "xportBind");
    *(void **)&xport->xportBindClose = dlsym(xport->xportPluginHandle, "xportBindClose");
    *(void **)&xport->xportListen = dlsym(xport->xportPluginHandle, "xportListen");
    *(void **)&xport->xportListenStop = dlsym(xport->xportPluginHandle, "xportListenStop");
    *(void **)&xport->xportServerReady = dlsym(xport->xportPluginHandle, "xportServerReady");
    *(void **)&xport->xportMasterAddressGet = dlsym(xport->xportPluginHandle, "xportMasterAddressGet");
    *(void**)&xport->xportSend = dlsym(xport->xportPluginHandle, "xportSend");
    *(void**)&xport->xportRecv = dlsym(xport->xportPluginHandle, "xportRecv");
    *(void**)&xport->xportMaxPayloadSizeGet = dlsym(xport->xportPluginHandle, "xportMaxPayloadSizeGet");
    *(void**)&xport->xportTransparencyRegister = dlsym(xport->xportPluginHandle, "xportTransparencyRegister");
    *(void**)&xport->xportTransparencyDeregister = dlsym(xport->xportPluginHandle, "xportTransparencyDeregister");
    *(void**)&xport->xportMulticastRegister = dlsym(xport->xportPluginHandle, "xportMulticastRegister");
    *(void**)&xport->xportMulticastDeregister = dlsym(xport->xportPluginHandle, "xportMulticastDeregister");

    if(!xport->xportIpAddressAssign) xport->xportIpAddressAssign = xportIpAddressAssignFake;
    if(!xport->xportInit) xport->xportInit = xportInitFake;
    if(!xport->xportFinalize) xport->xportFinalize = xportFinalizeFake;
    if(!xport->xportNotifyInit) xport->xportNotifyInit = xportNotifyInitFake;
    if(!xport->xportNotifyFinalize) xport->xportNotifyFinalize = xportNotifyFinalizeFake;
    if(!xport->xportNotifyOpen) xport->xportNotifyOpen = xportNotifyOpenFake;
    if(!xport->xportNotifyClose) xport->xportNotifyClose = xportNotifyCloseFake;
    if(!xport->xportBind) xport->xportBind = xportBindFake;
    if(!xport->xportBindClose) xport->xportBindClose = xportBindCloseFake;
    if(!xport->xportListen) xport->xportListen = xportListenFake;
    if(!xport->xportListenStop) xport->xportListenStop = xportListenStopFake;
    if(!xport->xportServerReady) xport->xportServerReady = xportServerReadyFake;
    if(!xport->xportMasterAddressGet) xport->xportMasterAddressGet = xportMasterAddressGetFake;
    if(!xport->xportSend) xport->xportSend = xportSendFake;
    if(!xport->xportRecv) xport->xportRecv = xportRecvFake;
    if(!xport->xportMaxPayloadSizeGet) xport->xportMaxPayloadSizeGet = xportMaxPayloadSizeGetDefault;
    if(!xport->xportTransparencyRegister) xport->xportTransparencyRegister = xportTransparencyRegisterFake;
    if(!xport->xportTransparencyDeregister) xport->xportTransparencyDeregister = xportTransparencyDeregisterFake;
    if(!xport->xportMulticastRegister) xport->xportMulticastRegister = xportMulticastRegisterFake;
    if(!xport->xportMulticastDeregister) xport->xportMulticastDeregister = xportMulticastDeregisterFake;
    xport->xportId = clTransportIdGet();
    clListAddTail(&xport->xportList, &gClTransportList);
    clLogNotice("XPORT", "LOAD", "Loaded transport [%s] with plugin [%s]", type, plugin);

    goto out;

    out_free:
    if(xport->xportPlugin) clHeapFree(xport->xportPlugin);
    if(xport->xportType) clHeapFree(xport->xportType);
    clHeapFree(xport);

    out:
    return;
}

static ClInt32T listenerEventFind(ClInt32T fd, ClInt32T index)
{
    register ClInt32T i;
    if(index >= 0 && index < gXportCtrl.numfds && gXportCtrl.pollfds[index].fd == fd)
        return index;
    for(i = 0; i < gXportCtrl.numfds; ++i)
    {
        if(gXportCtrl.pollfds[i].fd == fd) return i;
    }
    return -1;
}

static ClXportListenerT *listenerFind(ClInt32T fd)
{
    ClXportListenerT *listener;
    ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gXportCtrl.listenerList)
    {
        listener = CL_LIST_ENTRY(iter, ClXportListenerT, list);
        if(listener->fd == fd) 
            return listener;
    }
    return NULL;
}

/*
 * Wake up the poll or listener thread.
 */
static __inline__ void transportBreakerWakeup(void)
{
    if(gXportCtrl.breaker[1] >= 0)
    {
        int c = 'c';
        if(write(gXportCtrl.breaker[1], &c, 1) != 1)
        {
            clLogError("XPORT", "BREAKER", "Xport breaker wakeup returned with [%s]", strerror(errno));
        }
    }
}

static ClRcT listenerEventRegister(ClXportListenerT *listener, 
                                   ClInt32T events,
                                   ClRcT (*dispatchCallback)(ClInt32T fd, ClInt32T events, void *cookie), 
                                   void *cookie)
{
    ClPollEventT event = {0};
    ClEventDataT *eventData;
    struct epoll_event epoll_event = {0};
    ClInt32T err;
    ClRcT rc = CL_ERR_ALREADY_EXIST;
    if(listenerEventFind(listener->fd, -1) >= 0)
    {
        clLogError("EVENT", "REGISTER", "Listener at fd [%d] already exists", listener->fd);
        goto out;
    }
    if(!events)
        events = EPOLLPRI | EPOLLIN;
    epoll_event.events = events;
    eventData = (ClEventDataT*)&epoll_event.data;
    eventData->fd = listener->fd;
    eventData->index = gXportCtrl.numfds;
    rc = CL_ERR_LIBRARY;
    clOsalMutexLock(&gXportCtrl.mutex);
    err = epoll_ctl(gXportCtrl.eventFd, EPOLL_CTL_ADD, listener->fd, &epoll_event);
    if(err < 0)
    {
        clLogError("EVENT", "REGISTER", "epoll_ctl add returned with error [%s] for fd [%d]",
                   strerror(errno), listener->fd);
        goto out_unlock;
    }
    gXportCtrl.pollfds = realloc(gXportCtrl.pollfds, sizeof(*gXportCtrl.pollfds) * 
                                         (gXportCtrl.numfds + 1));
    CL_ASSERT(gXportCtrl.pollfds != NULL);
    event.fd = listener->fd;
    event.events = events;
    event.dispatch = dispatchCallback;
    event.cookie = cookie;
    memcpy(gXportCtrl.pollfds + gXportCtrl.numfds, &event, sizeof(event));
    ++gXportCtrl.numfds;
    rc = CL_OK;
    clListAddTail(&listener->list, &gXportCtrl.listenerList);
    transportBreakerWakeup();
    clOsalCondSignal(&gXportCtrl.cond);
    
    out_unlock:
    clOsalMutexUnlock(&gXportCtrl.mutex);
    out:
    return rc;
}

static ClRcT listenerEventDeregister(ClXportListenerT *listener)
{
    struct epoll_event epoll_event = {0};
    ClInt32T index = 0;
    ClInt32T err;
    ClRcT rc = CL_ERR_LIBRARY;
    err = epoll_ctl(gXportCtrl.eventFd, EPOLL_CTL_DEL, listener->fd, &epoll_event);
    if(err < 0)
    {
        clLogError("EVENT", "DEREGISTER", "epoll delete failed for fd [%d] with error [%s]", listener->fd,
                   strerror(errno));
        goto out;
    }
    if((index = listenerEventFind(listener->fd, -1)) >= 0)
    {
        --gXportCtrl.numfds;
        memmove(gXportCtrl.pollfds + index, gXportCtrl.pollfds + index + 1,
                sizeof(*gXportCtrl.pollfds) * ( gXportCtrl.numfds - index));
    }
    clListDel(&listener->list);
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT transportListenerFinalize(void)
{
    clOsalMutexLock(&gXportCtrl.mutex);
    while(!CL_LIST_HEAD_EMPTY(&gXportCtrl.listenerList))
    {
        ClXportListenerT *listener = CL_LIST_ENTRY(gXportCtrl.listenerList.pNext, 
                                                   ClXportListenerT, list);
        clListDel(&listener->list);
        if(listener->fd != gXportCtrl.breaker[0])
            close(listener->fd);
        free(listener);
    }
    if(gXportCtrl.running)
    {
        /*
         * Signal the listener thread to wind up and wait for it to exit.
         */
        ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0};
        gXportCtrl.running = CL_FALSE;
        transportBreakerWakeup();
        clOsalCondSignal(&gXportCtrl.cond);
        clOsalCondWait(&gXportCtrl.cond, &gXportCtrl.mutex, delay);
    }
    if(gXportCtrl.breaker[0] >= 0)
        close(gXportCtrl.breaker[0]);
    if(gXportCtrl.breaker[1] >= 0)
        close(gXportCtrl.breaker[1]);

    close(gXportCtrl.eventFd);
    if(gXportCtrl.pollfds)
    {
        free(gXportCtrl.pollfds);
        gXportCtrl.pollfds = NULL;
    }
    gXportCtrl.numfds = 0;
    clOsalMutexUnlock(&gXportCtrl.mutex);
    return CL_OK;
}

static ClRcT transportListener(ClPtrT unused)
{
    struct epoll_event *epoll_events = NULL;
    ClInt32T epoll_fds = 0;
    ClInt32T epoll_cur_fds = 0;
    ClInt32T ret = 0;
    ClInt32T i;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 0};
    clOsalMutexLock(&gXportCtrl.mutex);
    while(gXportCtrl.running)
    {
        while ( gXportCtrl.running && !(epoll_fds = gXportCtrl.numfds))
            clOsalCondWait(&gXportCtrl.cond, &gXportCtrl.mutex, delay);
        if(!gXportCtrl.running)
            break;
        clOsalMutexUnlock(&gXportCtrl.mutex);
        if(epoll_cur_fds < epoll_fds)
        {
            epoll_events = realloc(epoll_events, sizeof(*epoll_events) * epoll_fds);
            CL_ASSERT(epoll_events !=  NULL);
            epoll_cur_fds = epoll_fds;
        }
        memset(epoll_events, 0, sizeof(*epoll_events) * epoll_fds);
        ret = epoll_wait(gXportCtrl.eventFd, epoll_events, epoll_fds, -1);
        clOsalMutexLock(&gXportCtrl.mutex);
        if(ret <= 0)
            continue;
        for(i = 0; i < ret; ++i)
        {
            ClEventDataT *data;
            ClInt32T index;
            ClPollEventT event = {0};
            if(!epoll_events[i].events) continue;
            data = (void*)&epoll_events[i].data;
            index = listenerEventFind(data->fd, data->index);
            if(index < 0)
            {
                clLogWarning("XPORT", "LISTENER", "Listener not found for fd [%d] registered at index [%d]", 
                             data->fd, data->index);
                continue;
            }
            memcpy(&event, gXportCtrl.pollfds+index, sizeof(event));
            if( !(epoll_events[i].events & event.events) )
                continue;
            clOsalMutexUnlock(&gXportCtrl.mutex);
            if(event.dispatch)
                event.dispatch(event.fd, epoll_events[i].events, event.cookie);
            clOsalMutexLock(&gXportCtrl.mutex);
        }
    }
    clOsalCondSignal(&gXportCtrl.cond);
    clOsalMutexUnlock(&gXportCtrl.mutex);
    free(epoll_events);
    return CL_OK;
}

static ClRcT transportBreaker(ClInt32T fd, ClInt32T events, void *cookie)
{
    char buf[80];
    int bytes = read(fd, buf, sizeof(buf));
    return bytes > 0 ? CL_OK : CL_ERR_LIBRARY;
}

/*
 * Usage to get the list node config in clTransport.xml
 */
ClRcT clTransportNodeAddrGet(ClUint32T *pNumberOfEntries,
        ClIocNodeAddressT *pAddrList) {
    ClUint32T   numEntries = 0;
    register ClListHeadT *iterNodeAddr;

    CL_LIST_FOR_EACH(iterNodeAddr, &gClXportNodeAddrList)
    {
        ClXportNodeAddrDataT *entry = CL_LIST_ENTRY(iterNodeAddr, ClXportNodeAddrDataT, list);
        pAddrList[numEntries++] = entry->slot;
    }

    if(numEntries != *pNumberOfEntries)
        *pNumberOfEntries = numEntries;

    return CL_OK;
}

ClRcT clTransportListenerRegister(ClInt32T fd, ClRcT (*dispatchCallback)(ClInt32T fd, ClInt32T events, void *cookie),
                                  void *cookie)
{
    ClXportListenerT *listener = NULL;
    ClRcT rc = CL_OK;
    listener = calloc(1, sizeof(*listener));
    CL_ASSERT(listener != NULL);
    listener->fd = fd;
    rc = listenerEventRegister(listener, EPOLLIN | EPOLLPRI, dispatchCallback, cookie);
    if(rc != CL_OK)
    {
        clLogError("XPORT", "LISTENER", "Xport listener register for fd [%d] returned with [%#x]", fd, rc);
        free(listener);
        return rc;
    }
    return rc;
}

ClRcT clTransportListenerDeregister(ClInt32T fd)
{
    ClRcT rc = CL_OK;
    ClXportListenerT *listener = NULL;
    clOsalMutexLock(&gXportCtrl.mutex);
    listener = listenerFind(fd);
    if(!listener)
    {
        clOsalMutexUnlock(&gXportCtrl.mutex);
        return CL_ERR_NOT_EXIST;
    }
    rc = listenerEventDeregister(listener);
    clOsalMutexUnlock(&gXportCtrl.mutex);
    if(rc != CL_OK)
    {
        return rc;
    }
    close(listener->fd);
    free(listener);
    return rc;
}

static void *transportPrivateDataGet(ClInt32T fd, ClIocPortT port, ClXportPrivateDataT **xportPrivate)
{
    ClUint32T hash = __TRANSPORT_PRIVATE_HASH(fd, port);
    struct hashStruct *iter;
    for(iter = gXportPrivateTable[hash]; iter; iter = iter->pNext)
    {
        ClXportPrivateDataT *private = hashEntry(iter, ClXportPrivateDataT, hash);
        if(private->fd == fd && private->port == port)
        {
            if(xportPrivate) *xportPrivate = private;
            return private->private;
        }
    }
    return NULL;
}

void *clTransportPrivateDataGet(ClInt32T fd, ClIocPortT port)
{
    return transportPrivateDataGet(fd, port, NULL);
}

void clTransportPrivateDataSet(ClInt32T fd, ClIocPortT port, void *private, void **privateLast)
{
    ClXportPrivateDataT *curXportPrivate = NULL;
    void *curPrivate;
    ClUint32T hash = 0;
    if(privateLast)
        *privateLast = NULL;
    if( (curPrivate = transportPrivateDataGet(fd, port, &curXportPrivate)) )
    {
        if(privateLast) *privateLast = curPrivate;
        curXportPrivate->private = private;
        return;
    }
    curXportPrivate = clHeapCalloc(1, sizeof(*curXportPrivate));
    CL_ASSERT(curXportPrivate != NULL);
    curXportPrivate->private = private;
    curXportPrivate->fd = fd;
    curXportPrivate->port = port;
    hash = __TRANSPORT_PRIVATE_HASH(fd, port);
    hashAdd(gXportPrivateTable, hash, &curXportPrivate->hash);
}

void *clTransportPrivateDataDelete(ClInt32T fd, ClIocPortT port)
{
    ClXportPrivateDataT *xportPrivate = NULL;
    void *curPrivate;
    curPrivate = transportPrivateDataGet(fd, port, &xportPrivate);
    if(!xportPrivate) return NULL;
    hashDel(&xportPrivate->hash);
    clHeapFree(xportPrivate);
    return curPrivate;
}

static ClRcT transportListenerInitialize(void)
{
    ClRcT rc = CL_OK;
    ClInt32T breaker[2] = {-1,-1};
    /*
     * Create the breaker.
     */
    gXportCtrl.breaker[0] = gXportCtrl.breaker[1] = -1;
    if(pipe(breaker) < 0)
    {
        clLogError("XPORT", "LISTENER", "Breaker pipe creation failed with error [%s]",
                   strerror(errno));
        return CL_ERR_LIBRARY;
    }
    fcntl(breaker[0], F_SETFD, FD_CLOEXEC);
    fcntl(breaker[1], F_SETFD, FD_CLOEXEC);
    rc = clTransportListenerRegister(breaker[0], transportBreaker, NULL);
    if(rc != CL_OK)
    {
        goto out_deregister;
    }
    gXportCtrl.breaker[0] = breaker[0];
    gXportCtrl.breaker[1] = breaker[1];
    /*
     * Create the listener pool on the first create.
     */
    if(!gXportCtrl.pool)
    {
        gXportCtrl.running = CL_TRUE;
        rc = clTaskPoolCreate(&gXportCtrl.pool, 1, NULL, NULL);
        if(rc != CL_OK)
        {
            clLogError("UDP", "LISTENER", "Task pool create failed with error [%#x]", rc);
            goto out_deregister;
        }
        rc = clTaskPoolRun(gXportCtrl.pool, transportListener, NULL);
        if(rc != CL_OK)
        {
            clLogError("UDP", "LISTENER", "Task pool run failed with error [%#x]", rc);
            goto out_delete;
        }
    }
    goto out;

    out_delete:
    clTaskPoolDelete(gXportCtrl.pool);
    gXportCtrl.pool = 0;
    
    out_deregister:
    clTransportListenerDeregister(breaker[0]);
    close(breaker[1]);
    gXportCtrl.breaker[0] = gXportCtrl.breaker[1] = -1;

    out:
    return rc;
}

static ClRcT transportInit(void)
{
    ClRcT rc = CL_OK;

    rc = clOsalMutexInit(&gXportCtrl.mutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&gXportCtrl.cond);
    CL_ASSERT(rc == CL_OK);

    if( (gXportCtrl.eventFd = epoll_create(16) ) < 0 )
    {
        clLogError("UDP", "INI", "epoll create returned with error [%s]", strerror(errno));
        goto out;
    }
    
    rc = transportListenerInitialize();

    out:
    return rc;
}

static ClRcT setDefaultXport(ClParserPtrT parent)
{
    ClRcT rc = CL_OK;
    ClParserPtrT xportConfig = clParserChild(parent, "config");
    if(xportConfig)
    {
        ClParserPtrT xportConfigDefault = clParserChild(xportConfig, "default");
        if(xportConfigDefault && xportConfigDefault->txt)
        {
            gClXportDefaultType[0] = 0;
            strncat(gClXportDefaultType, xportConfigDefault->txt, sizeof(gClXportDefaultType)-1);
        }
    }
    if(gClXportDefaultType[0])
    {
        gClXportDefault = findTransport(gClXportDefaultType);
    }
    /*
     * If nothing present, use the first one registered.
     */
    if(!gClXportDefault)
    {
        if(!CL_LIST_HEAD_EMPTY(&gClTransportList))
        {
            gClXportDefault = CL_LIST_ENTRY(gClTransportList.pNext, ClTransportLayerT, xportList);
        }
    }

    if(gClXportDefault)
    {
        clLogNotice("XPORT", "INIT", "Default transport set to [%s]", gClXportDefault->xportType);
    }
    else
    {
        clLogCritical("XPORT", "INIT", "Not found any transport!");
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

/*
 * Check the node can be handled as a bridge or not
 * Mark as CL_TRUE if its protocols contain at least one available protocol and
 * more than one protocol available on that node (bridge = CL_TRUE)
 *
 */
static ClBoolT _clNodeCanBeBridge(ClCharT *srcProto, ClCharT *dstProto,
                                  ClXportNodeAddrDataT *entry) {
    int j;
    int matches = 0;

    if(!entry->bridge) return CL_FALSE;

    for (j = 0; j < entry->numXport && matches < 2; j++)
    {
        if(!findTransport(entry->xports[j]))
            continue;
        if(!strcmp(srcProto, entry->xports[j]))
            ++matches;
        if(!strcmp(dstProto, entry->xports[j]))
            ++matches;
    }

    return matches < 2 ? CL_FALSE : CL_TRUE;
}

/*
 * Find the node address that can be handled as a bridge
 */
static void _clFindNodeBridge(ClCharT *srcProto, ClCharT *dstProto, ClIocNodeAddressT *iocAddress) 
{
    ClXportNodeAddrDataT *map;
    register ClListHeadT *iterNodeAddr;
    CL_LIST_FOR_EACH(iterNodeAddr, &gClXportNodeAddrList)
    {
        map = CL_LIST_ENTRY(iterNodeAddr, ClXportNodeAddrDataT, list);
        if (_clNodeCanBeBridge(srcProto, dstProto, map))
        {
            *iocAddress = map->slot;
            return;
        }
    }
}

/*
 * Initialize table for node destination lookup
 */
static void _clSetupDestNodeLUTData(void)
{
    register ClListHeadT *iter;
    register ClListHeadT *iterNodeAddr;
    int i;

    CL_LIST_FOR_EACH(iterNodeAddr, &gClXportNodeAddrList)
    {
        ClXportNodeAddrDataT *map = CL_LIST_ENTRY(iterNodeAddr, ClXportNodeAddrDataT, list);
        ClXportDestNodeLUTDataT *entryLUTData = clHeapCalloc(1, sizeof(*entryLUTData));
        CL_ASSERT(entryLUTData != NULL);

        // Don't update if it exits
        if (_clXportDestNodeLUTMapFind(map->slot)) {
            clHeapFree(entryLUTData);
            goto loop;
        }

        // The order protocol base on the config instead of available protocol
        if (map->slot == gIocLocalBladeAddress) {
            entryLUTData->bridgeIocNodeAddress = map->slot;
            entryLUTData->destIocNodeAddress = map->slot;
            entryLUTData->xportType = clStrdup(map->xports[0]);
            _clXportDestNodeLUTMapAdd(entryLUTData);
            goto loop;
        }

        CL_LIST_FOR_EACH(iter, &gClTransportList)
        {
            ClTransportLayerT *entry = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
            for (i = 0; i < map->numXport; i++)
            {
                // Source and destination share a protocol
                if (!strcmp(entry->xportType, map->xports[i]))
                {
                    entryLUTData->bridgeIocNodeAddress = map->slot;
                    entryLUTData->destIocNodeAddress = map->slot;
                    if(entryLUTData->xportType)
                    {
                        clHeapFree(entryLUTData->xportType);
                    }
                    entryLUTData->xportType = clStrdup(entry->xportType);
                    _clXportDestNodeLUTMapAdd(entryLUTData);
                    goto loop;
                } else if (!entryLUTData->bridgeIocNodeAddress) {
                    ClIocNodeAddressT bridgeNode = 0;
                    _clFindNodeBridge(entry->xportType, map->xports[i], &bridgeNode);
                    if (bridgeNode)
                    {
                        entryLUTData->bridgeIocNodeAddress = bridgeNode;
                        entryLUTData->xportType = clStrdup(entry->xportType);
                    }
                }
            }
        }

        if (entryLUTData->bridgeIocNodeAddress != entryLUTData->destIocNodeAddress) 
        {
            entryLUTData->destIocNodeAddress = map->slot;
            _clXportDestNodeLUTMapAdd(entryLUTData);
        }

        loop:
        continue;
    }
}

/*
 * Lookup in hash table to get node address destination and xport type to send/receive
  */
ClRcT clFindTransport(ClIocNodeAddressT dstIocAddress, ClIocAddressT *rdstIocAddress,
                      ClCharT **typeXport) 
{
    ClCharT *preferredXport = NULL;
    ClXportDestNodeLUTDataT *destNodeLUTData = NULL;

    if(!typeXport) return CL_ERR_INVALID_PARAMETER;
    preferredXport = *typeXport;

    clOsalMutexLock(&gXportNodeAddrListmutex);
    if (! (destNodeLUTData = _clXportDestNodeLUTMapFind(dstIocAddress) ) )
    {
        if(!_clXportNodeAddrMapFind(dstIocAddress))
        {
            _clXportUpdateNodeConfig(dstIocAddress);
        }
        _clSetupDestNodeLUTData();
        destNodeLUTData = _clXportDestNodeLUTMapFind(dstIocAddress);
    }

    if (!destNodeLUTData) 
    {
        clOsalMutexUnlock(&gXportNodeAddrListmutex);
        return CL_ERR_NOT_EXIST;
    }

    ((ClIocPhysicalAddressT *)rdstIocAddress)->nodeAddress = destNodeLUTData->bridgeIocNodeAddress;
    *typeXport = destNodeLUTData->xportType;
    /*
     * If a preferred xport was specified and there is a mismatch for 
     * local node destination xport on the preferred, check if we can route
     * through preferred
     */
    if(preferredXport && 
       dstIocAddress == gIocLocalBladeAddress &&
       strcmp(*typeXport, preferredXport))
    {
        static ClCharT preferredXportCache[CL_MAX_NAME_LENGTH];
        /*
         * Lookup the fast cache.
         */
        if(preferredXportCache[0] &&
           !strcmp(preferredXportCache, preferredXport))
        {
            *typeXport = preferredXport;
        }
        else
        {
            ClXportNodeAddrDataT *nodeAddrConfig;
            nodeAddrConfig = _clXportNodeAddrMapFind(gIocLocalBladeAddress);
            if(!nodeAddrConfig)
            {
                clLogWarning("LUT", "FIND", "Node entry for this node not found");
            }
            else
            {
                ClUint32T i;
                for(i = 0; i < nodeAddrConfig->numXport; ++i)
                {
                    if(!strcmp(nodeAddrConfig->xports[i], preferredXport))
                    {
                        *typeXport = preferredXport;
                        preferredXportCache[0] = 0;
                        strncat(preferredXportCache, nodeAddrConfig->xports[i],
                                sizeof(preferredXportCache)-1);
                        break;
                    }
                }
            }
        }
    }
    clOsalMutexUnlock(&gXportNodeAddrListmutex);
#if 0 /* debug */
    if(dstIocAddress != gIocLocalBladeAddress)
    {
        clLogNotice("LUT", "FIND", "Using transport [%s] for destination node [%d]",
                    *typeXport, dstIocAddress);
    }
    else
    {
        static int c;
        if(!c)
        {
            clLogNotice("LUT", "FIND", "Using transport [%s] for self", *typeXport);
            c=1;
        }
    }
#endif
    return CL_OK;
}

static void _setDefaultXportForNode(ClParserPtrT parent)
{
#define MAX_XPORTS_PER_SLOT (8)
    ClParserPtrT protocol = clParserChild(parent, "protocol");
    ClParserPtrT nodes;
    ClParserPtrT node;

    ClCharT xportType[CL_MAX_NAME_LENGTH] = { 0 };
    ClCharT *token = NULL;
    ClCharT *nextToken = NULL;
    ClCharT *xports[MAX_XPORTS_PER_SLOT+1]; /* +1 for paranoia in case some blighter changes max xports to 0 */
    ClXportNodeAddrDataT *nodeAddrConfig = NULL;
    int numxn = 0;
    int i = 0;

    if (!protocol)
    {
        clLogNotice("XPORT", "INIT", "Protocol for node is not set!");
        goto default_xport;
    }

    const ClCharT *xportDefault = clParserAttr(protocol, "default");

    nodes = clParserChild(protocol, "nodes");
    if (!nodes)
    {
        goto default_xport_node;
    }

    node = clParserChild(nodes, "node");
    if (!node)
    {
        goto default_xport_node;
    }

    while (node)
    {
        numxn = 0;
        const ClCharT *slot = clParserAttr(node, "slot");
        const ClCharT *protocols = clParserAttr(node, "protocol");
        const ClCharT *bridge = clParserAttr(node, "bridge");

        if (!slot || !protocols)
        {
               goto next;
        }

        ClIocNodeAddressT iocAddress = atoi(slot);
        clLogDebug("LUT", "MAP", "Adding entry for slot [%d]", iocAddress);
        // Split the protocol into xports
        xportType[0] = 0;
        strncat(xportType, protocols, sizeof(xportType)-1);

        token = strtok_r(xportType, " ", &nextToken);
        while (token && numxn < MAX_XPORTS_PER_SLOT)
        {
            ClTransportLayerT *xport = findTransport(token);

            /* On destination node there maybe existing protocol which not available this blade */
            if (xport || iocAddress != gIocLocalBladeAddress)
            {
                xports[numxn++] = token;
            }
            token = strtok_r(NULL, " ", &nextToken);
        }

        ClXportNodeAddrDataT *entry = clHeapCalloc(1, sizeof(*entry));
        CL_ASSERT(entry != NULL);
        entry->slot = iocAddress;
        entry->numXport = numxn;
        if (bridge && numxn > 1 && (!strncmp(bridge, "1", 1) ||
                        !strncasecmp(bridge, "yes", 3) ||
                        !strncasecmp(bridge, "y", 1) ||
                        !strncasecmp(bridge, "true", 4) ||
                        !strncasecmp(bridge, "t", 1) ))
        {
            entry->bridge = CL_TRUE;
        }
        else
        {
            entry->bridge = CL_FALSE;
        }
        entry->xports = (ClCharT **) clHeapCalloc(entry->numXport, sizeof(ClCharT *));
        CL_ASSERT(entry->xports != NULL);
        for (i = 0; i < entry->numXport; i++)
        {
            entry->xports[i] = clStrdup(xports[i]);
        }

        _clXportNodeAddrMapAdd(entry);
        next:
        node = node->next;
    }

    default_xport_node:
    numxn = 0;
    // If xport default type from file config
    if (xportDefault)
    {
        // Split the default protocol into xports
        xportType[0] = 0;
        strncat(xportType, xportDefault, sizeof(xportType)-1);
        token = strtok_r(xportType, " ", &nextToken);
        while (token && numxn < MAX_XPORTS_PER_SLOT)
        {
            xports[numxn++] = token;
            token = strtok_r(NULL, " ", &nextToken);
        }
    }

    default_xport:
    if (!numxn) 
    {
        xports[numxn++] = gClXportDefault->xportType;
    }

    gClXportDefaultNodeName.numXport = numxn;
    gClXportDefaultNodeName.xports = (ClCharT **) clHeapCalloc(gClXportDefaultNodeName.numXport, sizeof(ClCharT *));
    CL_ASSERT(gClXportDefaultNodeName.xports != NULL);
    if (numxn > 1) 
    {
        gClXportDefaultNodeName.bridge = CL_TRUE;
    } 
    else 
    {
        gClXportDefaultNodeName.bridge = CL_FALSE;
    }

    for (i = 0; i < gClXportDefaultNodeName.numXport; i++)
    {
        gClXportDefaultNodeName.xports[i] = clStrdup(xports[i]);
    }

    /*
     * Add local node into LUT
     */
    if (!(nodeAddrConfig = _clXportNodeAddrMapFind(gIocLocalBladeAddress))) 
    {
        ClListHeadT *iter;
        i = 0;
        nodeAddrConfig = clHeapCalloc(1, sizeof(*nodeAddrConfig));
        CL_ASSERT(nodeAddrConfig != NULL);
        nodeAddrConfig->slot = gIocLocalBladeAddress;
        nodeAddrConfig->numXport = 0;
        nodeAddrConfig->bridge = CL_FALSE;
        CL_LIST_FOR_EACH(iter, &gClTransportList) 
        {
            ClTransportLayerT *entry = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
            nodeAddrConfig->numXport++;
            nodeAddrConfig->xports = clHeapRealloc(nodeAddrConfig->xports, sizeof(ClCharT *) * nodeAddrConfig->numXport);
            nodeAddrConfig->xports[i] = clStrdup(entry->xportType);
            i++;
        }
        if (i > 1) 
        {
            nodeAddrConfig->bridge = CL_TRUE;
        }
        clLogDebug("IOC", "LUT", "Add Ioc local node address config [%#x]", gIocLocalBladeAddress);
        _clXportNodeAddrMapAdd(nodeAddrConfig);
    }
    gLocalNodeBridge = nodeAddrConfig->bridge;
    if(gLocalNodeBridge)
    {
        clLogNotice("IOC", "LUT", "Local node bridge enabled with multiple transports");
    }
    return;

#undef MAX_XPORTS_PER_SLOT
}

ClRcT _clIocSetMulticastConfig(ClParserPtrT parent)
{
    ClRcT rc = CL_OK;
    ClParserPtrT multicastPtr = clParserChild(parent, "multicast");
    if (!multicastPtr)
    {
        strncat(gClTransportMcastAddr, MULTICAST_ADDR_DEFAULT, sizeof(gClTransportMcastAddr)-1);
        gClTransportMcastPort = MULTICAST_PORT_DEFAULT;
        goto out;
    }

    const ClCharT *mcastAddressAttr = clParserAttr(multicastPtr, "address");
    const ClCharT *mcastPortAttr = clParserAttr(multicastPtr, "port");

    if (!mcastAddressAttr || !mcastPortAttr)
    {
        strncat(gClTransportMcastAddr, MULTICAST_ADDR_DEFAULT, sizeof(gClTransportMcastAddr)-1);
        gClTransportMcastPort = MULTICAST_PORT_DEFAULT;
        goto out;
    }

    strncat(gClTransportMcastAddr, mcastAddressAttr, sizeof(gClTransportMcastAddr)-1);
    gClTransportMcastPort = atoi(mcastPortAttr);

    out:
    return rc;
}

ClRcT clTransportLayerInitialize(void)
{
    ClRcT rc = CL_OK;
    ClCharT *configPath;
    ClParserPtrT parent;
    ClParserPtrT xports;
    ClParserPtrT xport;

    configPath = getenv("ASP_CONFIG");
    if(!configPath) configPath = ".";
    parent = clParserOpenFile(configPath, XPORT_CONFIG_FILE);
    if(!parent)
    {
        clLogWarning("XPORT", "INIT", 
                     "Unable to open transport config [%s] from path [%s]." 
                     "Would be loading default transport [%s] with plugin [%s]", 
                     XPORT_CONFIG_FILE, configPath, CL_XPORT_DEFAULT_TYPE, CL_XPORT_DEFAULT_PLUGIN);
        goto set_default;
    }
    xports = clParserChild(parent, "xports");
    if(!xports)
    {
        clLogError("XPORT", "INIT", "No xports tag found for transport initialize in [%s]", XPORT_CONFIG_FILE);
        goto out_free;
    }
    xport = clParserChild(xports, "xport");
    if(!xport)
    {
        clLogError("XPORT", "INIT", "No xport tag found for transport initialize in [%s]", XPORT_CONFIG_FILE);
        goto out_free;
    }
    while(xport)
    {
        ClParserPtrT type = clParserChild(xport, "type");
        ClParserPtrT plugin = clParserChild(xport, "plugin");
        if(!type || !plugin) goto next;
        addTransport(type->txt, plugin->txt);
        next:
        xport = xport->next;
    }
    if(CL_LIST_HEAD_EMPTY(&gClTransportList))
    {
        set_default:
        addTransport(CL_XPORT_DEFAULT_TYPE, CL_XPORT_DEFAULT_PLUGIN);
    }

    clOsalMutexInit(&gXportNodeAddrListmutex);

    rc = setDefaultXport(parent);
    if(rc != CL_OK)
    {
        goto out_free;
    }

    _setDefaultXportForNode(parent);

    /*
     * Loading config of multicast
     */
    _clIocSetMulticastConfig(parent);

    _clSetupDestNodeLUTData();

    _clTransportGmsTimerInitCallback();

    rc = transportInit();
    if(rc != CL_OK)
    {
        clTransportLayerFinalize();
    }
    
    out_free:
    if(parent)
        clParserFree(parent);

    return rc;
}

/*
 * Initialize the particular transport type or all transports.
 */
ClRcT clTransportInitialize(const ClCharT *type, ClBoolT nodeRep)
{
    ClTransportLayerT *xport = NULL;
    ClRcT rc = CL_OK;
    register ClListHeadT *iter;

    if(nodeRep)
    {
        gXportNodeRep = nodeRep;
    }

    /*
     * To do a fast pass early update destNodeLUT
     */
    clIocNotificationRegister(_clXportDestNodeLUTUpdateNotification, NULL);

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "INI", "Unable to initialize transport [%s]. Transport not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            rc = xport->xportInit(type, xport->xportId, nodeRep);
            if(rc != CL_OK)
            {
                clLogError("XPORT", "INIT", "Transport [%s] initialize failed with [%#x]", type, rc);
            }
            else
            { 
                xport->xportState |= XPORT_STATE_INITIALIZED;
                goto out_notify;
            }
        }
        return rc;
    }
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(!(xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportInit(xport->xportType, xport->xportId, nodeRep)) != CL_OK)
        {
            clLogError("XPORT", "INI", "Unable to initialize transport [%s]. Error [%#x]", 
                       xport->xportType, err);
            rc = err;
            goto out;
        }
        else xport->xportState |= XPORT_STATE_INITIALIZED;
    }

    out_notify:
    if (nodeRep) 
    {
        rc = clTransportIpAddressAssign(type);
    }

    if(nodeRep && rc == CL_OK)
    {
        rc = clTransportNotificationInitialize(type);
    }

    out:
    return rc;
}

/*
 * Close the transport
 */

ClRcT clTransportFinalize(const ClCharT *type, ClBoolT nodeRep)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;

    if(nodeRep)
    {
        rc = clTransportNotificationFinalize(type);
        if(rc != CL_OK)
            return rc;
    }

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "CLOSE", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if((xport->xportState & XPORT_STATE_INITIALIZED))
        {
            rc = xport->xportFinalize(xport->xportId, nodeRep);
            xport->xportState &= ~XPORT_STATE_INITIALIZED;
        }
        return rc;
    }
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportFinalize(xport->xportId, nodeRep)) != CL_OK)
        {
            clLogError("XPORT", "CLOSE", "Transport [%s] close failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
        xport->xportState &= ~XPORT_STATE_INITIALIZED;
    }
    return rc;
}

ClRcT clTransportServerReady(const ClCharT *type, ClIocAddressT *pAddress)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "READY", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "READY", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportServerReady(pAddress);
        return rc;
    }
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportServerReady(pAddress)) != CL_OK)
        {
            clLogError("XPORT", "READY", "Transport [%s] server ready returned with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }

    return rc;
}

static ClRcT transportMasterAddressGet(ClTransportLayerT *xport, ClIocLogicalAddressT la,
                                       ClIocPortT port, ClIocNodeAddressT *masterAddress)
{
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(xport)
    {
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "MASTER", "Transport [%s] not initialized", xport->xportType);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportMasterAddressGet(la, port, masterAddress);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            if( (rc = xport->xportMasterAddressGet(la, port, masterAddress)) != CL_OK)
            {
                if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
                {
                    rc = CL_OK;
                }
                else
                {
                    clLogError("XPORT", "MASTER",
                               "Transport [%s] master address returned with [%#x]", 
                               xport->xportType, rc);
                    goto out;
                }
            }
            else
            {
                return CL_OK;
            }
        }
    }
    rc = CL_ERR_NOT_EXIST;

    out:
    return rc;
}

ClRcT clTransportMasterAddressGet(const ClCharT *type, ClIocLogicalAddressT la,
                                  ClIocPortT port, ClIocNodeAddressT *masterAddress)
{
    ClTransportLayerT *xport = NULL;
    if(type)
    {
        xport = findTransport(type);
        if(!xport) return CL_ERR_NOT_EXIST;
    }
    return transportMasterAddressGet(xport, la, port, masterAddress);
}

ClRcT clTransportMasterAddressGetDefault(ClIocLogicalAddressT la,
        ClIocPortT port, ClIocNodeAddressT *masterAddress) {
    ClRcT rc = CL_OK;
    if (clAspNativeLeaderElection()) {
        rc = clNodeCacheLeaderGet(masterAddress);
        return rc;
    }

    if (gGmsInitDone)
    {
        ClGmsClusterNotificationBufferT notBuffer = { 0 };
        rc = clGmsClusterTrack(gXportHandleGms, CL_GMS_TRACK_CURRENT, &notBuffer);
        if (rc == CL_OK)
        {
            if (-1 == notBuffer.leader)
            {
                return CL_IOC_RC(CL_ERR_TRY_AGAIN);
            }
            *masterAddress = notBuffer.leader;
            if (notBuffer.notification)
            {
                clHeapFree(notBuffer.notification);
            }
            return rc;
        }
    }
    return transportMasterAddressGet(gClXportDefault, la, port, masterAddress);
}

/*
 * Node representative specific api.
 */
ClRcT clTransportNotificationInitialize(const ClCharT *type)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportNotifyInit();
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
        {
            rc = clTransportNotifyInitialize();
        }
        if(rc != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] notify initialize failed with [%#x]", 
                       type, rc);
        }
        return rc;
    }
    rc = CL_ERR_NOT_SUPPORTED;
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT rc2 = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            rc2 = xport->xportNotifyInit();
            if(rc != CL_OK)
            {
                rc = rc2;
            }
        }
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
    {
        if( (rc = clTransportNotifyInitialize()) != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport notify initialize failed with [%#x]", rc);
        }
    }
    return rc;
}

/*
 * Node representative specific api.
 */
ClRcT clTransportIpAddressAssign(const ClCharT *type)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if (type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "IP_ASSIGN", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        rc = xport->xportIpAddressAssign();
        if(rc != CL_OK)
        {
            clLogError("XPORT", "IP_ASSIGN", "Transport [%s] ip address assign failed with [%#x]",
                       type, rc);
        }
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        rc = xport->xportIpAddressAssign();
        if(rc != CL_OK)
        {
            clLogError("XPORT", "IP_ASSIGN", "Transport [%s] ip address assign failed with [%#x]",
                       type, rc);
        }
    }

    return rc;
}

ClRcT clTransportNotificationFinalize(const ClCharT *type)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportNotifyFinalize();
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
        {
            rc = clTransportNotifyFinalize();
        }
        if(rc != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] notify finalize failed with [%#x]", 
                       type, rc);
        }
        return rc;
    }
    rc = CL_ERR_NOT_SUPPORTED;
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            rc = xport->xportNotifyFinalize();
            if(rc == CL_OK) break;
        }
    }

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
    {
        if( (rc = clTransportNotifyFinalize()) != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport notify finalize failed with [%#x]", rc);
        }

    }
    return rc;
}

ClRcT clTransportNotificationOpen(const ClCharT *type, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportNotifyOpen(port);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
        {
            rc = clTransportNotifyOpen(port);
        }
        if(rc != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] notify open failed with [%#x]", 
                       type, rc);
        }
        return rc;
    }
    rc = CL_ERR_NOT_SUPPORTED;
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            rc = xport->xportNotifyOpen(port);
            if(rc == CL_OK) break;
        }
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
    {
        if( (rc = clTransportNotifyOpen(port)) != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport notify open failed with [%#x]", rc);
        }
    }
    return rc;
}

ClRcT clTransportNotificationClose(const ClCharT *type, ClIocNodeAddressT nodeAddress, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportNotifyClose(nodeAddress, port);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
        {
            rc = clTransportNotifyClose(port);
        }
        if(rc != CL_OK)
        {
            clLogError("XPORT", "NOTIFY", "Transport [%s] notify close failed with [%#x]", 
                       type, rc);
        }
        return rc;
    }
    rc = CL_ERR_NOT_SUPPORTED;
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            rc = xport->xportNotifyClose(nodeAddress, port);
            if(rc == CL_OK) break;
        }
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
    {
        rc = clTransportNotifyClose(port);
    }
    if(rc != CL_OK)
    {
        clLogError("XPORT", "NOTIFY", "Transport notify close failed with [%#x]", rc);
    }
    return rc;
}

ClRcT clTransportBind(const ClCharT *type, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;

    rc = clTransportNotificationOpen(type, port);
    if(rc != CL_OK)
        return rc;

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "BIND", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "BIND", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        return xport->xportBind(port);
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportBind(port)) != CL_OK)
        {
            clLogError("XPORT", "BIND", "Transport [%s] bind failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }
    return rc;
}

ClRcT clTransportBindClose(const ClCharT *type, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "BIND", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "BIND", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportBindClose(port);
        if(rc != CL_OK)
            return rc;
        goto out_close;
    }
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportBindClose(port)) != CL_OK)
        {
            clLogError("XPORT", "BIND", 
                       "Transport [%s] bind close failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }
    if(rc != CL_OK)
        return rc;

    out_close:
    rc = clTransportNotificationClose(type, gIocLocalBladeAddress, port);

    return rc;
}

ClRcT clTransportListen(const ClCharT *type, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;

    rc = clTransportNotificationOpen(type, port);
    if(rc != CL_OK)
        return rc;

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportListen(port);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportListen(port)) != CL_OK)
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] listen failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }
    return rc;
}

ClRcT clTransportListenStop(const ClCharT *type, ClIocPortT port)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;

    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportListenStop(port);
        if(rc != CL_OK)
            return rc;
        goto out_close;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportListenStop(port)) != CL_OK)
        {
            clLogError("XPORT", "LISTEN", "Transport [%s] listenstop failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }
    if(rc != CL_OK)
        return rc;
    
    out_close:
    rc = clTransportNotificationClose(type, gIocLocalBladeAddress, port);

    return rc;
}

ClRcT clTransportTransparencyRegister(const ClCharT *type, ClIocPortT port, 
                                      ClIocLogicalAddressT logicalAddr, ClUint32T haState)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "TL", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "TL", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportTransparencyRegister(port, logicalAddr, haState);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportTransparencyRegister(port, logicalAddr, haState)) != CL_OK)
        {
            clLogError("XPORT", "TL", "Transport [%s] tl register failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }

    return rc;
}

ClRcT clTransportTransparencyDeregister(const ClCharT *type, ClIocPortT port, ClIocLogicalAddressT logicalAddr)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "TL", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "TL", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportTransparencyDeregister(port, logicalAddr);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportTransparencyDeregister(port, logicalAddr)) != CL_OK)
        {
            clLogError("XPORT", "TL", "Transport [%s] TL deregister failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }

    return rc;
}

ClRcT clTransportMulticastRegister(const ClCharT *type, ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "MCAST", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "MCAST", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportMulticastRegister(port, mcastAddr);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportMulticastRegister(port, mcastAddr)) != CL_OK)
        {
            clLogError("XPORT", "MCAST", "Transport [%s] mcast register failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }

    return rc;
}

ClRcT clTransportMulticastDeregister(const ClCharT *type, ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "MCAST", "Transport [%s] not registered", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "MCAST", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportMulticastDeregister(port, mcastAddr);
        return rc;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportMulticastDeregister(port, mcastAddr)) != CL_OK)
        {
            clLogError("XPORT", "MCAST", "Transport [%s] mcast deregister failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }

    return rc;
}

ClRcT clTransportSend(const ClCharT *type, ClIocPortT port, ClUint32T priority, ClIocAddressT *address, 
                      struct iovec *iov, ClInt32T iovlen, ClInt32T flags)
{
    ClTransportLayerT *xport = NULL;
    register ClListHeadT *iter;
    ClRcT rc = CL_OK;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "SEND", "Transport [%s] not registered.", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "SEND", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        return xport->xportSend(port, priority, address, iov, iovlen, flags);
    }
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
           &&
           (err = xport->xportSend(port, priority, address, iov, iovlen, flags)) != CL_OK)
        {
            clLogError("XPORT", "SEND", "Transport [%s] send failed with [%#x]", xport->xportType, err);
            rc |= err;
        }
    }
    return rc;
}

static ClRcT transportRecv(ClTransportLayerT *xport, ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, 
                           ClUint8T *buffer, ClUint32T bufSize,
                           ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_OK;
    register ClListHeadT *iter;

    if(xport)
    {
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "RECV", "Transport [%s] not initialized", xport->xportType);
            return CL_ERR_NOT_INITIALIZED;
        }
        rc = xport->xportRecv(commPort, pRecvOption, buffer, bufSize, message, pRecvParam);
        goto out;
    }

    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            if((err = xport->xportRecv(commPort, pRecvOption, 
                                       buffer, bufSize, message, pRecvParam)) != CL_OK)
            {
                if(CL_GET_ERROR_CODE(err) != CL_IOC_ERR_RECV_UNBLOCKED)
                {
                    clLogError("XPORT", "RECV", "Transport [%s] recv. failed with [%#x]", xport->xportType, err);
                }
                rc |= err;
            }
            else
            {
                rc = CL_OK;
                goto out;
            }
        }
    }

    out:
    return rc;
}

ClRcT clTransportRecv(const ClCharT *type, ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, 
                      ClUint8T *buffer, ClUint32T bufSize,
                      ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClTransportLayerT *xport = NULL;
    if(type)
    {
        xport = findTransport(type);
        if(!xport) return CL_ERR_NOT_EXIST;
    }
    return transportRecv(xport, commPort, pRecvOption, buffer, bufSize, message, pRecvParam);
}

ClRcT clTransportRecvDefault(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption,
                             ClUint8T *buffer, ClUint32T bufSize,
                             ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    return transportRecv(gClXportDefault, commPort, pRecvOption, buffer, bufSize, message, pRecvParam);
}

ClRcT clTransportMaxPayloadSizeGet(const ClCharT *type, ClUint32T *pSize)
{
    ClTransportLayerT *xport;
    ClRcT rc = CL_ERR_NOT_EXIST;
    register ClListHeadT *iter;
    ClUint32T minPayloadSize = ~0U; 
    ClUint32T size = 0;
    if(!pSize) 
        return CL_ERR_INVALID_PARAMETER;
    if(type)
    {
        xport = findTransport(type);
        if(!xport)
        {
            clLogError("XPORT", "PAYLOAD", "Transport [%s] not registered.", type);
            return CL_ERR_NOT_EXIST;
        }
        if(!(xport->xportState & XPORT_STATE_INITIALIZED))
        {
            clLogError("XPORT", "PAYLOAD", "Transport [%s] not initialized", type);
            return CL_ERR_NOT_INITIALIZED;
        }
        return xport->xportMaxPayloadSizeGet(pSize);
    }
    /*
     * Find the minimum payload size across all transports when
     * sending across all
     */
    CL_LIST_FOR_EACH(iter, &gClTransportList)
    {
        ClRcT err = CL_OK;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if((xport->xportState & XPORT_STATE_INITIALIZED)
            &&
           (err = xport->xportMaxPayloadSizeGet(&size)) == CL_OK)
        {
            rc = CL_OK;
            if(size < minPayloadSize) 
            {
                minPayloadSize = size;
            }
        }
    }
    if(rc == CL_OK)
    {
        *pSize = minPayloadSize;
    }
    return rc;
}

static __inline__ void transportFree(ClTransportLayerT *xport)
{
    if(xport->xportType) clHeapFree(xport->xportType);
    if(xport->xportPlugin) clHeapFree(xport->xportPlugin);
    if(xport->xportPluginHandle) dlclose(xport->xportPluginHandle);
    clHeapFree(xport);
}

ClRcT clTransportLayerFinalize(void)
{
    register ClListHeadT *iter;
    ClListHeadT *next = NULL;
    ClTransportLayerT *xport;
    CL_LIST_HEAD_DECLARE(transportBatch);

    clOsalMutexLock(&gXportNodeAddrListmutex);
    _clXportNodeAddrMapFree();
    _clXportDestNodeLUTMapFree();
    clOsalMutexUnlock(&gXportNodeAddrListmutex);

    if (gXportHandleGms)
    {
        clGmsFinalize(gXportHandleGms);
    }
    clListMoveInit(&gClTransportList, &transportBatch);
    for(iter = transportBatch.pNext; iter != &transportBatch; iter = next)
    {
        next = iter->pNext;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            xport->xportState &= ~XPORT_STATE_INITIALIZED;
            xport->xportFinalize(xport->xportId, gXportNodeRep);
        }
        clListDel(&xport->xportList);
        transportFree(xport);
    }
    transportListenerFinalize();
    return CL_OK;
}

ClCharT *clTransportMcastAddressGet(void)
{
    return gClTransportMcastAddr;
}

ClUint32T clTransportMcastPortGet(void)
{
    return gClTransportMcastPort;
}

ClBoolT clTransportBridgeEnabled(ClIocNodeAddressT node)
{
    ClXportNodeAddrDataT *nodeAddrConfig = NULL;
    ClBoolT bridge = CL_FALSE;
    if(node == gIocLocalBladeAddress)
        return gLocalNodeBridge;
    clOsalMutexLock(&gXportNodeAddrListmutex);
    nodeAddrConfig = _clXportNodeAddrMapFind(node);
    if(nodeAddrConfig)
        bridge = nodeAddrConfig->bridge;
    clOsalMutexUnlock(&gXportNodeAddrListmutex);
    return bridge;
}
