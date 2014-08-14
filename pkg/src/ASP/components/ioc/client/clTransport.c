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
#include <clTipcUserApi.h>
#include <clTipcGeneral.h>
#include <clTipcNeighComps.h>
#include <clEoIpi.h>

#define CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT (512)

typedef struct ClTransportLayer
{
#define XPORT_STATE_INITIALIZED  (0x1)
    ClCharT *xportType;
    ClCharT *xportPlugin;
    ClPtrT xportPluginHandle;
    ClInt32T xportState;
    ClRcT (*xportInit)(void);
    ClRcT (*xportFinalize)(void);
    ClRcT (*xportNotifyInit)(void);
    ClRcT (*xportNotifyFinalize)(void);
    ClRcT (*xportNotifyOpen)(ClIocPortT port);
    ClRcT (*xportNotifyClose)(ClIocPortT port);
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
extern ClUint32T clEoWithOutCpm;

#define XPORT_CONFIG_FILE "clTransport.xml"

static ClRcT xportInitFake(void) 
{ 
    clLogNotice("XPORT", "INIT", "Inside fake transport initialize");
    return CL_OK; 
}

static ClRcT xportFinalizeFake(void) 
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

static ClRcT xportNotifyCloseFake(ClIocPortT port)
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

static ClRcT xportMaxPayloadSizeGetDefault(ClUint32T *size)
{
    if(size)
        *size = CL_XPORT_MAX_PAYLOAD_SIZE_DEFAULT - 512;
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
        clLogError("XPORT", "LOAD", "Unable to load plugin [%s]. Error [%s]", plugin, dlerror() ? dlerror() : "unknown");
        goto out_free;
    }
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
    *(void**)&xport->xportSend = dlsym(xport->xportPluginHandle, "xportSend");
    *(void**)&xport->xportRecv = dlsym(xport->xportPluginHandle, "xportRecv");
    *(void**)&xport->xportMaxPayloadSizeGet = dlsym(xport->xportPluginHandle, "xportMaxPayloadSizeGet");
    *(void**)&xport->xportTransparencyRegister = dlsym(xport->xportPluginHandle, "xportTransparencyRegister");
    *(void**)&xport->xportTransparencyDeregister = dlsym(xport->xportPluginHandle, "xportTransparencyDeregister");
    *(void**)&xport->xportMulticastRegister = dlsym(xport->xportPluginHandle, "xportMulticastRegister");
    *(void**)&xport->xportMulticastDeregister = dlsym(xport->xportPluginHandle, "xportMulticastDeregister");

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
    if(!xport->xportSend) xport->xportSend = xportSendFake;
    if(!xport->xportRecv) xport->xportRecv = xportRecvFake;
    if(!xport->xportMaxPayloadSizeGet) xport->xportMaxPayloadSizeGet = xportMaxPayloadSizeGetDefault;
    if(!xport->xportTransparencyRegister) xport->xportTransparencyRegister = xportTransparencyRegisterFake;
    if(!xport->xportTransparencyDeregister) xport->xportTransparencyDeregister = xportTransparencyDeregisterFake;
    if(!xport->xportMulticastRegister) xport->xportMulticastRegister = xportMulticastRegisterFake;
    if(!xport->xportMulticastDeregister) xport->xportMulticastDeregister = xportMulticastDeregisterFake;

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
    if(gXportCtrl.running && gXportCtrl.breaker[1] >= 0)
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

static ClRcT __listenerEventDeregister(ClXportListenerT *listener)
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

static ClRcT listenerEventDeregister(ClXportListenerT *listener)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gXportCtrl.mutex);
    rc = __listenerEventDeregister(listener);
    clOsalMutexUnlock(&gXportCtrl.mutex);
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
    /*
     * Create the breaker.
     */
    gXportCtrl.breaker[0] = gXportCtrl.breaker[1] = -1;
    if(pipe(gXportCtrl.breaker) < 0)
    {
        clLogError("XPORT", "LISTENER", "Breaker pipe creation failed with error [%s]",
                   strerror(errno));
        return CL_ERR_LIBRARY;
    }
    fcntl(gXportCtrl.breaker[0], F_SETFD, FD_CLOEXEC);
    fcntl(gXportCtrl.breaker[1], F_SETFD, FD_CLOEXEC);
    rc = clTransportListenerRegister(gXportCtrl.breaker[0], transportBreaker, NULL);
    if(rc != CL_OK)
    {
        goto out_deregister;
    }
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
    clTransportListenerDeregister(gXportCtrl.breaker[0]);
    close(gXportCtrl.breaker[1]);
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

static void setDefaultXport(ClParserPtrT parent)
{
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
    rc = CL_ERR_NOT_EXIST;
    if(!parent)
    {
        clLogError("XPORT", "INIT", "Unable to open transport config [%s] from path [%s]", XPORT_CONFIG_FILE, configPath);
        goto out;
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
    setDefaultXport(parent);

    rc = transportInit();
    if(rc != CL_OK)
    {
        clTransportLayerFinalize();
    }
    
    out_free:
    clParserFree(parent);
    out:
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
            rc = xport->xportInit();
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
           (err = xport->xportInit()) != CL_OK)
        {
            clLogError("XPORT", "INI", "Unable to initialize transport [%s]. Error [%#x]", xport->xportType, err);
            rc |= err;
        }
        else xport->xportState |= XPORT_STATE_INITIALIZED;
    }

    out_notify:
    if(nodeRep && rc == CL_OK)
    {
        rc = clTransportNotificationInitialize(type);
    }

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
            rc = xport->xportFinalize();
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
           (err = xport->xportFinalize()) != CL_OK)
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
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            rc = xport->xportNotifyInit();
            if(rc == CL_OK) break;
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

ClRcT clTransportNotificationClose(const ClCharT *type, ClIocPortT port)
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
        rc = xport->xportNotifyClose(port);
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
            rc = xport->xportNotifyClose(port);
            if(rc == CL_OK) break;
        }
    }
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_SUPPORTED)
    {
        rc = clTransportNotifyClose(port);
    }
    clLogError("XPORT", "NOTIFY", "Transport notify close failed with [%#x]", rc);
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
    rc = clTransportNotificationClose(type, port);

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
    rc = clTransportNotificationClose(type, port);

    return rc;
}

ClRcT clTransportDispatch(ClIocPortT port, ClUint8T *buffer, ClUint32T bufSize)
{
    ClRcT rc = CL_OK;
    ClTipcHeaderT userHeader = { 0 };
    ClUint32T size = sizeof(ClTipcHeaderT);
    ClUint8T *pBuffer = buffer;
    ClInt32T bytes = bufSize;
    ClIocRecvParamT recvParam = {0};
    ClBufferHandleT message = 0;
#ifdef CL_TIPC_COMPRESSION
    ClTimeT pktSendTime = 0;
#endif

    rc = clBufferCreate(&message);
    if(rc != CL_OK)
        goto out;

    if(bytes <= size)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("Dropping a received packet. "
                                           "The packet is an invalid or a corrupted one. "
                                           "Packet size received [%d]", bytes));
        goto out;
    }

    memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClTipcHeaderT));

    if(userHeader.version != CL_IOC_HEADER_VERSION)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Dropping received packet of version [%d]. Supported version [%d]\n",
                                        userHeader.version, CL_IOC_HEADER_VERSION));
        goto out;
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
            goto out;
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
#endif
        rc = clBufferNBytesWrite(message, pBuffer, bytes);

#ifdef CL_TIPC_COMPRESSION
        if(pBuffer == decompressedStream)
        {
            clHeapFree(pBuffer);
        }
#endif

        if(rc != CL_OK) {
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
                        gIocLocalBladeAddress, port));

        /*
         * Will be used once fully tested as its faster than earlier method
         */
        if(userFragHeader.header.flag == IOC_LAST_FRAG)
            clLogTrace("FRAG", "RECV", "Got Last frag at offset [%d], size [%d], received [%d]",
                       userFragHeader.fragOffset, userFragHeader.fragLength, bytes);

        rc = __iocUserFragmentReceive(pBuffer, &userFragHeader, 
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
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL,("Dropping a received fragmented-packet. "
                                                  "Failed to reassemble the packet. Packet size is %d. "
                                                  "error code 0x%x", bytes, rc));
            }
        }
        goto out;
    }

    recvParam.length = bytes;
    recvParam.priority = userHeader.priority;
    recvParam.protoType = userHeader.protocolType;
    memcpy(&recvParam.srcAddr, &userHeader.srcAddress, sizeof(recvParam.srcAddr));
    clLogTrace("XPORT", "RECV", "Received message of size [%d]", bytes);
    clEoEnqueueReassembleJob(message, &recvParam);
    message = 0;

    out:
    if(message)
    {
        clBufferDelete(&message);
    }
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
                    clLogError("XPORT", "RECV", "Transport [%s] recv. failed with [%#x]", xport->xportType, err);
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
    clListMoveInit(&gClTransportList, &transportBatch);
    for(iter = transportBatch.pNext; iter != &transportBatch; iter = next)
    {
        next = iter->pNext;
        xport = CL_LIST_ENTRY(iter, ClTransportLayerT, xportList);
        if(xport->xportState & XPORT_STATE_INITIALIZED)
        {
            xport->xportState &= ~XPORT_STATE_INITIALIZED;
            xport->xportFinalize();
        }
        clListDel(&xport->xportList);
        transportFree(xport);
    }
    transportListenerFinalize();
    return CL_OK;
}

