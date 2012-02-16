#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#include <sys/stat.h>
#endif
#include <sys/epoll.h>
#include <unistd.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHash.h>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clIocApi.h>
#include <clTransport.h>
#include <clList.h>
#include "clIocNeighComps.h"

#ifdef HAVE_INOTIFY

#define TRANSPORT_NOTIFY_BITS (8)
#define TRANSPORT_NOTIFY_SIZE ( 1 << TRANSPORT_NOTIFY_BITS)
#define TRANSPORT_NOTIFY_MASK ( TRANSPORT_NOTIFY_SIZE-1)
#define TRANSPORT_NOTIFY_HASH(fd) ( (fd) & TRANSPORT_NOTIFY_MASK)
#define TRANSPORT_NOTIFY_DIRNAME "notify"

static ClInt32T gNotifyFd = -1;
static ClInt32T gWatchFd = -1;
static struct hashStruct *gNotifyMap[TRANSPORT_NOTIFY_SIZE];
static CL_LIST_HEAD_DECLARE(gClXportNotifyRegistrants);

typedef struct ClTransportNotifyMap
{
    ClInt32T port;
    ClInt32T watchFd;
    struct hashStruct hash;
}ClTransportNotifyMapT;

typedef struct ClTransportNotifyRegistrant
{
    ClTransportNotifyCallbackT callback;
    ClPtrT arg;
    ClListHeadT list;
}ClTransportNotifyRegistrantT;

static ClRcT transportNotifyRegistrants(ClIocPhysicalAddressT *compAddr, ClUint32T status);
static ClRcT transportNotifyCallback(ClInt32T fd, ClInt32T events, void *unused);

static ClTransportNotifyMapT *findNotifyMap(ClInt32T watchFd, ClInt32T port)
{
    ClUint32T hash = 0;
    if(port < 0) return NULL;
    hash = TRANSPORT_NOTIFY_HASH(port);
    struct hashStruct *iter;
    for(iter = gNotifyMap[hash]; iter; iter = iter->pNext)
    {
        ClTransportNotifyMapT *notify = hashEntry(iter, ClTransportNotifyMapT, hash);
        ClInt32T cmp = (notify->port == port);
        if(watchFd >= 0)
            cmp &= (notify->watchFd == watchFd);
        if(cmp) return notify;
    }
    return NULL;
}

static ClTransportNotifyMapT *addNotifyMap(ClInt32T watchFd, ClInt32T port)
{
    ClTransportNotifyMapT *notify;
    if(port < 0) return NULL;
    if( !(notify = findNotifyMap(watchFd, port)))
    {
        ClUint32T hash = TRANSPORT_NOTIFY_HASH(port);
        notify = clHeapCalloc(1, sizeof(*notify));
        CL_ASSERT(notify != NULL);
        hashAdd(gNotifyMap, hash, &notify->hash);
    }
    notify->watchFd = watchFd;
    notify->port = port;
    return notify;
}

static ClRcT delNotifyMap(ClInt32T watchFd, ClInt32T port)
{
    ClTransportNotifyMapT *notify;
    if( (notify = findNotifyMap(watchFd, port) ) )
    {
        hashDel(&notify->hash);
        clHeapFree(notify);
        return CL_OK;
    }
    return CL_ERR_NOT_EXIST;
}

static ClCharT *transportNotifyLocGet(void)
{
    static ClCharT pathname[FILENAME_MAX];
    if(!pathname[0])
    {
        ClCharT *runDir = getenv("ASP_RUNDIR");
        if(!runDir) runDir = ".";
        snprintf(pathname, sizeof(pathname), "%s/%s", runDir, TRANSPORT_NOTIFY_DIRNAME);
    }
    return pathname;
}

static void transportNotifyLocRemove(const ClCharT *compName)
{
    ClCharT filename[FILENAME_MAX];
    snprintf(filename, sizeof(filename), "%s/%s", transportNotifyLocGet(), compName);
    unlink(filename);
}

ClRcT clTransportNotifyInitialize(void)
{
    ClCharT *pathname = transportNotifyLocGet();
    if(mkdir(pathname, 0777) < 0 && errno != EEXIST)
    {
        clLogError("XPORT", "NOTIFY", "Notify create directory [%s] failed with error [%s]",
                   pathname, strerror(errno));
        return CL_ERR_LIBRARY;
    }
                   
    gNotifyFd = inotify_init();
    if(gNotifyFd < 0)
    {
        perror("inotify_init:");
        return CL_ERR_LIBRARY;
    }
    fcntl(gNotifyFd, F_SETFD, FD_CLOEXEC);
    gWatchFd = inotify_add_watch(gNotifyFd, pathname, IN_CREATE | IN_CLOSE_NOWRITE);
    if(gWatchFd < 0)
    {
        perror("inotify_add_watch:");
        close(gNotifyFd);
        gNotifyFd = -1;
    }
    return clTransportListenerRegister(gNotifyFd, transportNotifyCallback, NULL);
}

ClRcT clTransportNotifyFinalize(void)
{
    if(gWatchFd >= 0)
        close(gWatchFd);
    if(gNotifyFd >= 0) 
        clTransportListenerDeregister(gNotifyFd);
    return CL_OK;
}

static void __compNotify(ClInt32T wd, const ClCharT *compName, ClIocNotificationIdT notification)
{
    ClTransportNotifyMapT *notify;
    ClInt32T port = -1;
    const ClCharT *s = compName;
    ClIocPhysicalAddressT compAddr;
    if(s && ( s = strrchr(s, '_')))
        port = atoi(s+1);
    if(notification == CL_IOC_COMP_ARRIVAL_NOTIFICATION)
    {
        notify = addNotifyMap(wd, port);
        if(notify)
        {
            compAddr.nodeAddress = clIocLocalAddressGet();
            compAddr.portId = port;
            clIocCompStatusSet(compAddr, CL_IOC_NODE_UP);
            /*
             * Invoke all registrants with the event.
             */
            transportNotifyRegistrants(&compAddr, CL_IOC_NODE_UP);
            clLogDebug("XPORT", "NOTIFY", "Comp arrival marked for port [%#x], id [%s]", port, compName);
        }
    }
    else
    {
        if(delNotifyMap(wd, port) == CL_OK)
        {
            compAddr.nodeAddress = clIocLocalAddressGet();
            compAddr.portId = port;
            if(compName)
            {
                transportNotifyLocRemove(compName);
            }
            clIocCompStatusSet(compAddr, CL_IOC_NODE_DOWN);
            /*
             * Invoke all registrants with the event.
             */
            transportNotifyRegistrants(&compAddr, CL_IOC_NODE_DOWN);
            clLogDebug("XPORT", "NOTIFY", "Comp death marked for port [%#x], id [%s]", port, compName);
        }
    }
}

static ClRcT transportNotifyCallback(ClInt32T fd, ClInt32T events, void *unused)
{
    ClCharT buff[ 16 * (sizeof(struct inotify_event) + FILENAME_MAX) ];
    ClInt32T bytes;
    ClCharT *iter = buff;
    struct inotify_event *event;
    if(!(events & EPOLLIN))
        return CL_OK;
    bytes = read(fd, buff, sizeof(buff));
    while(bytes >= (ClInt32T)sizeof(struct inotify_event))
    {
        event = (struct inotify_event*)iter;
        if((event->mask & IN_CREATE))
        {
            __compNotify(event->wd, event->len ? event->name : NULL,
                               CL_IOC_COMP_ARRIVAL_NOTIFICATION);
        }
        else if ((event->mask & IN_CLOSE_NOWRITE))
        {
            __compNotify(event->wd, event->len ? event->name : NULL,
                               CL_IOC_COMP_DEATH_NOTIFICATION);
        }
        iter = iter + sizeof(struct inotify_event) + event->len;
        bytes -= (sizeof(struct inotify_event) + event->len);
    }
    return CL_OK;
}

static ClRcT transportNotifyRegistrants(ClIocPhysicalAddressT *compAddr, ClUint32T status)
{
    ClListHeadT *iter, *next = NULL;
    for(iter = gClXportNotifyRegistrants.pNext; iter != &gClXportNotifyRegistrants;
        iter = next)
    {
        ClTransportNotifyRegistrantT *registrant;
        next = iter->pNext;
        registrant = CL_LIST_ENTRY(iter, ClTransportNotifyRegistrantT, list);
        registrant->callback(compAddr, status, registrant->arg);
    }
    return CL_OK;
}

ClRcT clTransportNotifyOpen(ClIocPortT port)
{
    ClCharT pathname[FILENAME_MAX];
    ClCharT *compName = getenv("ASP_COMPNAME");
    ClInt32T fd;
    if(!compName) compName = "COMPNAME";
    snprintf(pathname, sizeof(pathname), "%s/%s_%d", transportNotifyLocGet(), compName, port);
    unlink(pathname);
    fd = open(pathname, O_CREAT | O_RDONLY, 0777);
    if(fd < 0)
    {
        clLogError("XPORT", "NOTIFY", "Xport notify open for path [%s] failed with [%s]",
                   pathname, strerror(errno));
        return CL_ERR_LIBRARY;
    }
    else
        clLogDebug("XPORT", "NOTIFY", "Xport notify opened for path [%s]", pathname);
    return CL_OK;
}

ClRcT clTransportNotifyClose(ClIocPortT port)
{
    return CL_OK;
}

ClRcT clTransportNotifyRegister(ClTransportNotifyCallbackT callback, ClPtrT arg)
{
    ClTransportNotifyRegistrantT *registrant = NULL;
    if(!callback) return CL_ERR_INVALID_PARAMETER;
    registrant = clHeapCalloc(1, sizeof(*registrant));
    CL_ASSERT(registrant != NULL);
    registrant->callback = callback;
    registrant->arg = arg;
    clListAddTail(&registrant->list, &gClXportNotifyRegistrants);
    return CL_OK;
}

ClRcT clTransportNotifyDeregister(ClTransportNotifyCallbackT callback)
{
    ClListHeadT *iter, *next = NULL;
    ClTransportNotifyRegistrantT *registrant;

    for(iter = gClXportNotifyRegistrants.pNext; iter != &gClXportNotifyRegistrants;
        iter = next)
    {
        next = iter->pNext;
        registrant = CL_LIST_ENTRY(iter, ClTransportNotifyRegistrantT, list);
        if(registrant->callback == callback)
        {
            clListDel(&registrant->list);
            clHeapFree(registrant);
            return CL_OK;
        }
    }

    return CL_ERR_NOT_EXIST;
}

#else

ClRcT clTransportNotifyInitialize(void)
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT clTransportNotifyFinalize(void)
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT clTransportNotifyOpen(ClIocPortT port)
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT clTransportNotifyClose(ClIocPortT port)
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT clTransportNotifyRegister(ClTransportNotifyCallbackT callback, ClPtrT arg)
{
    return CL_ERR_NOT_SUPPORTED;
}

ClRcT clTransportNotifyDeregister(ClTransportNotifyCallbackT callback)
{
    return CL_ERR_NOT_SUPPORTED;
}

#endif

