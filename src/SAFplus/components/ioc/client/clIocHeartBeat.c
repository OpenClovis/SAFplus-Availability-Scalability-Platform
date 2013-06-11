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
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <clParserApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clHash.h>
#include <clList.h>
#include <clVersionApi.h>
#include <clIocErrors.h>
#include <clIocIpi.h>
#include <clNodeCache.h>
#include <clTransport.h>
#include <clIocSetup.h>
#include <clIocManagementApi.h>
#include <clIocNeighComps.h>
#include <clIocUserApi.h>
#include <clClmTmsCommon.h>
#include <clCpmExtApi.h>

#define CL_IOC_HB_INTERVAL_DEFAULT 3000
#define CL_IOC_MAX_HB_RETRIES_DEFAULT 5
#define CL_IOC_HB_METHOD "HeartBeatPlugin"
#define CL_IOC_HB_HELLO_MESSAGE "HELLO"
#define CL_IOC_HB_EXIT_MESSAGE "EXIT"

#define __CL_IOC_HB_STATUS_ACTIVE (0x1)
#define __CL_IOC_HB_STATUS_PAUSED (0x2)

#define CL_IOC_HB_STATUS_ACTIVE() (gHeartBeatStatus & __CL_IOC_HB_STATUS_ACTIVE)
#define CL_IOC_HB_STATUS_PAUSED() (gHeartBeatStatus & __CL_IOC_HB_STATUS_PAUSED)

#define __CL_IOC_HB_STATUS_CLEAR(flag) do { gHeartBeatStatus &= ~(flag); } while(0)
#define __CL_IOC_HB_STATUS_SET(flag) do { gHeartBeatStatus |= (flag); } while(0)

#define CL_IOC_HB_CLEAR_STATUS_ACTIVE() __CL_IOC_HB_STATUS_CLEAR(__CL_IOC_HB_STATUS_ACTIVE)
#define CL_IOC_HB_CLEAR_STATUS_PAUSED() __CL_IOC_HB_STATUS_CLEAR(__CL_IOC_HB_STATUS_PAUSED)
#define CL_IOC_HB_SET_STATUS_ACTIVE() __CL_IOC_HB_STATUS_SET(__CL_IOC_HB_STATUS_ACTIVE)
#define CL_IOC_HB_SET_STATUS_PAUSED() __CL_IOC_HB_STATUS_SET(__CL_IOC_HB_STATUS_PAUSED)

typedef struct ClIocHeartBeatNotification
{
    ClIocPhysicalAddressT addr;
    ClIocNotificationIdT event;
}ClIocHeartBeatNotificationT;

typedef ClRcT (*ClIocHeartBeatPluginT)(ClUint32T interval, ClUint32T retries);

extern ClIocNodeAddressT gIocLocalBladeAddress;

static ClOsalMutexT gIocHeartBeatTableLock;

static ClTimerHandleT gHeartBeatTimer = CL_HANDLE_INVALID_VALUE;

static ClUint32T gGmsInitDone = CL_FALSE;
static ClTimerHandleT gHandleGmsTimer = CL_HANDLE_INVALID_VALUE;
static ClGmsHandleT gHeartBeatHandleGms = CL_HANDLE_INVALID_VALUE;

/*
 * Store configuration of heart-beat
 */
static ClInt32T gClHeartBeatInterval;
static ClInt32T gClHeartBeatIntervalLocal;
static ClInt32T gClHeartBeatRetries;

static ClUint32T gHeartBeatStatus;
/*
 * HashMap for peer nodes status.
 */
static struct hashStruct *gIocHeartBeatMap[CL_IOC_MAX_NODES];
/*
 * LinkList for peer node status.
 */
static CL_LIST_HEAD_DECLARE(gIocHeartBeatList);

static ClIocHeartBeatPluginT gClHeartBeatPlugin = NULL;

static ClPtrT gClPluginHandle;

static ClOsalMutexT gIocHeartBeatLocalTableLock;
static ClTimerHandleT gHeartBeatLocalTimer = CL_HANDLE_INVALID_VALUE;
/*
 * HashMap for local components status.
 * Have to store separate two map and list for faster lookup.
 */
static struct hashStruct *gIocHeartBeatLocalMap[CL_IOC_MAX_COMP_PORT+1];
/*
 * LinkList for local components status.
 */
static CL_LIST_HEAD_DECLARE(gIocHeartBeatLocalList);

/*
 * Adding an entry of peer node's status into linkList and hashMap.
 */
static ClRcT _clIocHeartBeatEntryMapAdd(ClIocHeartBeatStatusT *entry) 
{
    ClUint32T hash = 0;
    ClCharT *xportType = NULL;
    ClIocAddressT dstSlot = {{0}};
    /*
     * Before adding the entry, find if we have a direct path to the slot
     * without going through the bridge in which case the bridge is anyway 
     * proxying the healthcheck
     */
    if(entry->linkIndex != gIocLocalBladeAddress)
    {
        clFindTransport(entry->linkIndex, &dstSlot, &xportType);
        if(xportType && 
           dstSlot.iocPhyAddress.nodeAddress != entry->linkIndex)
        {
            clLogNotice("IOC", "HBT", "Skipping healthcheck for node [%d] "
                        "as it would be proxied by the bridge at slot [%d]",
                        entry->linkIndex, dstSlot.iocPhyAddress.nodeAddress);
            return CL_ERR_NO_OP;
        }
    }
    hash = entry->linkIndex & CL_IOC_MAX_NODE_ADDRESS;
    hashAdd(gIocHeartBeatMap, hash, &entry->hash);
    clListAddTail(&entry->list, &gIocHeartBeatList);
    clLogDebug("IOC", "HBT", "Added heartbeat entry for node [%d]", entry->linkIndex);
    return CL_OK;
}

/*
 * Find an entry peer node's status.
 *
 */
static ClIocHeartBeatStatusT *_clIocHeartBeatMapFind(ClIocHeartBeatLinkIndex linkIndex) {
    ClUint32T hash = linkIndex & CL_IOC_MAX_NODE_ADDRESS;
    register struct hashStruct *iter;
    for (iter = gIocHeartBeatMap[hash]; iter; iter = iter->pNext) {
        ClIocHeartBeatStatusT *entry =
                hashEntry(iter, ClIocHeartBeatStatusT, hash);
        if (entry->linkIndex == linkIndex) {
            return entry;
        }
    }
    return NULL;
}

/*
 * Adding an entry of local component's status into linkList and hashMap.
 */
static __inline__ void _clIocHeartBeatLocalEntryMapAdd(ClIocHeartBeatStatusT *entry) {
    ClUint32T hash = entry->linkIndex & CL_IOC_MAX_COMP_PORT;
    hashAdd(gIocHeartBeatLocalMap, hash, &entry->hash);
    clListAddTail(&entry->list, &gIocHeartBeatLocalList);
    clLogTrace("IOC", "HBT", "Added heartbeat entry for comp [%d]", entry->linkIndex);
}

/*
 * Find an entry local component's status.
 *
 */
static ClIocHeartBeatStatusT *_clIocHeartBeatLocalMapFind(ClIocHeartBeatLinkIndex linkIndex) {
    ClUint32T hash = linkIndex & CL_IOC_MAX_COMP_PORT;
    register struct hashStruct *iter;
    for (iter = gIocHeartBeatLocalMap[hash]; iter; iter = iter->pNext) {
        ClIocHeartBeatStatusT *entry =
                hashEntry(iter, ClIocHeartBeatStatusT, hash);
        if (entry->linkIndex == linkIndex) {
            return entry;
        }
    }
    return NULL;
}

/*
 * Free an entry peer node/local component's status out of linkList and hashMap.
 */
static __inline__ void _clIocHeartBeatEntryMapDel(ClIocHeartBeatStatusT *entry) {
    hashDel(&entry->hash);
    clListDel(&entry->list);
}

/*
 * Got hearbeat reply from local component and peer nodes
 */
ClRcT clIocHearBeatHealthCheckUpdate(ClIocNodeAddressT nodeAddress, ClUint32T portId, ClCharT *message) {
    ClRcT rc = CL_OK;
    if (nodeAddress == gIocLocalBladeAddress)
    {
        if (message != NULL
                && !strncmp(message, CL_IOC_HB_EXIT_MESSAGE,
                        sizeof(CL_IOC_HB_EXIT_MESSAGE)))
        {
            CL_IOC_HB_CLEAR_STATUS_ACTIVE();
            if(gHeartBeatTimer != CL_HANDLE_INVALID_VALUE)
            {
                clTimerDelete(&gHeartBeatTimer);
            }
            if (gHeartBeatLocalTimer != CL_HANDLE_INVALID_VALUE)
            {
                clTimerDelete(&gHeartBeatLocalTimer);
            }
            return rc;
        }
        clOsalMutexLock(&gIocHeartBeatLocalTableLock);
        ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatLocalMapFind(portId);
        if (!hbStatus) {
            hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
            hbStatus->linkIndex = portId;
            hbStatus->status = CL_IOC_NODE_UP;
            hbStatus->retryCount = 0;
            _clIocHeartBeatLocalEntryMapAdd(hbStatus);
        } else {
             hbStatus->retryCount = 0;
        }
        clOsalMutexUnlock(&gIocHeartBeatLocalTableLock);
    }
    else
    {
        if (!CL_IOC_HB_STATUS_ACTIVE())
        {
            return rc;
        }
        clOsalMutexLock(&gIocHeartBeatTableLock);
        ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatMapFind(nodeAddress);
        if (!hbStatus)
        {
            hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
            hbStatus->linkIndex = nodeAddress;
            hbStatus->status = CL_IOC_NODE_UP;
            hbStatus->retryCount = 0;
            if(_clIocHeartBeatEntryMapAdd(hbStatus) != CL_OK)
            {
                clHeapFree(hbStatus);
                hbStatus = NULL;
            }
        }
        else
        {
            hbStatus->status = CL_IOC_NODE_UP;
            hbStatus->retryCount = 0;
        }
        clOsalMutexUnlock(&gIocHeartBeatTableLock);
    }
    return rc;
}

#define __ALLOC_HB_NOTIFICATIONS            do {                        \
        if(!(numNotifications & 15))                                    \
        {                                                               \
            hbNotifications = clHeapRealloc(hbNotifications,            \
                                            sizeof(*hbNotifications) * (numNotifications + 16)); \
            CL_ASSERT(hbNotifications != NULL);                         \
            memset(hbNotifications + numNotifications, 0,               \
                   sizeof(*hbNotifications) * 16);                      \
        }                                                               \
}while(0)

/*
 * Calling clIocSend and to health check peer nodes
*/
static ClRcT _clIocHeartBeatSend(void) 
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT compAddr;
    ClEoExecutionObjT *eoObj = NULL;
    ClIocHeartBeatNotificationT *hbNotifications = NULL;
    ClUint32T i, numNotifications = 0;

    clEoMyEoObjectGet(&eoObj);

    if(!eoObj)
    {
        clLogWarning("IOC", "HBT", "EO still uninitialized during heartbeat phase. Will retry again");
        return CL_ERR_NOT_INITIALIZED;
    }

    if(CL_IOC_HB_STATUS_PAUSED())
    {
        return CL_OK;
    }

    /*
     * Peer node heartbeat interval sending
     */
    compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    compAddr.portId = CL_IOC_XPORT_PORT;

    rc = clIocHeartBeatMessageReqRep(eoObj->commObj, (ClIocAddressT *) &compAddr, CL_IOC_PROTO_HB, CL_FALSE);

    clOsalMutexLock(&gIocHeartBeatTableLock);
    if (rc == CL_OK) 
    {
        register ClListHeadT *iter;
        ClListHeadT *next = NULL;
        /*
         * Peer nodes health check status
         */
        for(iter = gIocHeartBeatList.pNext; iter != &gIocHeartBeatList; iter = next)
        {
            next = iter->pNext;
            ClIocHeartBeatStatusT *entry =
                CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
            if (entry->status == CL_IOC_NODE_UP) 
            {
                if(entry->linkIndex == gIocLocalBladeAddress)
                {
                    if(entry->retryCount > 0)
                    {
                        entry->retryCount = 0;
                    }
                    continue;
                }

                /*
                 * Get current status in case node already updated
                 */
                ClUint8T status;
                clIocRemoteNodeStatusGet(entry->linkIndex, &status);
                if (status == CL_IOC_NODE_DOWN)
                {
                    entry->status = CL_IOC_NODE_DOWN;
                    entry->retryCount = 0;
                }
                else 
                {
                    if (entry->retryCount > gClHeartBeatRetries)
                    {
                        clLogNotice("IOC", "HBT", "No heartbeats from node [0x%x] for %d retries.  Marking node failed.", entry->linkIndex, entry->retryCount);
                        entry->status = CL_IOC_NODE_DOWN;
                        /*
                         * Notify node leave close and release the entry to avoid false heartbeat
                         * again on the same entry.
                         */
                        entry->retryCount = 0;
                        __ALLOC_HB_NOTIFICATIONS;
                        hbNotifications[numNotifications].addr.nodeAddress = entry->linkIndex;
                        hbNotifications[numNotifications].addr.portId = CL_IOC_XPORT_PORT;
                        hbNotifications[numNotifications].event = CL_IOC_NODE_LEAVE_NOTIFICATION;
                        ++numNotifications;
                        _clIocHeartBeatEntryMapDel(entry);
                        clHeapFree(entry);

                    } 
                    else
                    {
                        entry->retryCount++;
                    }
                }
            }
            else if(entry->status == CL_IOC_LINK_DOWN)
            {
                entry->retryCount = 0;
                entry->status = CL_IOC_NODE_UP;
                /*
                 *Reassign the interface addresses back on link up for available transports
                 */
                clTransportAddressAssign(NULL);
                __ALLOC_HB_NOTIFICATIONS;
                hbNotifications[numNotifications].addr.nodeAddress = entry->linkIndex;
                hbNotifications[numNotifications].addr.portId = CL_IOC_XPORT_PORT;
                hbNotifications[numNotifications].event = CL_IOC_NODE_LINK_UP_NOTIFICATION;
                ++numNotifications;
            }
        }
    }
    else
    {
        register ClListHeadT *iter;
        ClListHeadT *next = NULL;
        /*
         * Peer nodes health check status
         */
        for(iter = gIocHeartBeatList.pNext; iter != &gIocHeartBeatList; iter = next)
        {
            next = iter->pNext;
            ClIocHeartBeatStatusT *entry =
                CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);

	    if(entry->linkIndex == gIocLocalBladeAddress)
	    {
	        continue;
	    }

            if (entry->status == CL_IOC_NODE_UP) 
            {
                /*
                 * Get current status in case node already updated
                 */
                ClUint8T status = 0;
                clIocRemoteNodeStatusGet(entry->linkIndex, &status);

                if (status == CL_IOC_NODE_DOWN)
                {
                    entry->status = CL_IOC_NODE_DOWN;
                    entry->retryCount = 0;
                }
                else 
                {
                    if(entry->retryCount > gClHeartBeatRetries)
                    {
                        entry->status = CL_IOC_LINK_DOWN;
                        /*
                         * Notify node leave close and retain the entry as link could be restored
                         * later for the node
                         */
                        entry->retryCount = 0;
                        if(entry->linkIndex != gIocLocalBladeAddress)
                        {
                            __ALLOC_HB_NOTIFICATIONS;
                            hbNotifications[numNotifications].addr.nodeAddress = entry->linkIndex;
                            hbNotifications[numNotifications].addr.portId = CL_IOC_XPORT_PORT;
                            hbNotifications[numNotifications].event = CL_IOC_NODE_LINK_DOWN_NOTIFICATION;
                            ++numNotifications;
                        }
                    } 
                    else 
                    {
                        entry->retryCount++;
                    }
                }
            }
        }
    }
    clOsalMutexUnlock(&gIocHeartBeatTableLock);

    for(i = 0; i < numNotifications; ++i)
    {
        switch(hbNotifications[i].event)
        {
        case CL_IOC_NODE_LEAVE_NOTIFICATION:
            clTransportNotificationClose(NULL, hbNotifications[i].addr.nodeAddress,
                                         hbNotifications[i].addr.portId,
                                         CL_IOC_NODE_LEAVE_NOTIFICATION);
            break;

        case CL_IOC_NODE_LINK_UP_NOTIFICATION:
            clLogNotice("SPLIT", "CLUSTER", "Sending node arrival for slot [%d]", 
                        hbNotifications[i].addr.nodeAddress);
            if((rc = clTransportNotificationOpen(NULL, hbNotifications[i].addr.nodeAddress,
                                                 hbNotifications[i].addr.portId,
                                                 CL_IOC_NODE_LINK_UP_NOTIFICATION)) != CL_OK)
            {
                ClIocHeartBeatStatusT *entry = NULL;
                clOsalMutexLock(&gIocHeartBeatTableLock);
                entry = _clIocHeartBeatMapFind(hbNotifications[i].addr.nodeAddress);
                if(entry)
                {
                    clLogNotice("SPLIT", "CLUSTER", 
                                "Setting node [%d] status back to link down "
                                "as notification open failed with [%#x]",
                                entry->linkIndex, rc);
                    entry->status = CL_IOC_LINK_DOWN;
                }
                clOsalMutexUnlock(&gIocHeartBeatTableLock);
            }
            break;

        case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
            clLogNotice("SPLIT", "CLUSTER",
                        "HeartBeat node [%#x]'s status death as link appears to be down", 
                        hbNotifications[i].addr.nodeAddress);
            clTransportNotificationClose(NULL, hbNotifications[i].addr.nodeAddress,
                                         hbNotifications[i].addr.portId,
                                         CL_IOC_NODE_LINK_DOWN_NOTIFICATION);
            break;

        default: break;
        }
    }
    
    if(hbNotifications)
        clHeapFree(hbNotifications);

    return CL_OK;

}


/*
 * Calling clIocSend and to health check local components
 */
static ClRcT _clIocHeartBeatCompsSend(void) 
{
    ClRcT rc = CL_OK;
    ClEoExecutionObjT *eoObj = NULL;
    ClListHeadT *iter, *next;
    ClIocPhysicalAddressT compAddr = { 0 };
    ClIocHeartBeatNotificationT *hbNotifications = NULL;
    ClUint32T i, numNotifications = 0;

    clEoMyEoObjectGet(&eoObj);
    if(!eoObj)
    {
        clLogWarning("IOC", "HBT", "EO still uninitialized during heartbeat comp. Will retry again");
        return CL_ERR_NOT_INITIALIZED;
    }
    /*
     * Return if heartbeat is temporarily paused. (could be under debugger)
     */
    if(CL_IOC_HB_STATUS_PAUSED())
    {
        return CL_OK; 
    }

    compAddr.nodeAddress = gIocLocalBladeAddress;

    /*
     * Local component heartbeat interval sending
     */
    clOsalMutexLock(&gIocHeartBeatLocalTableLock);

    for(iter = gIocHeartBeatLocalList.pNext; iter != &gIocHeartBeatLocalList; iter = next) 
    {
        next = iter->pNext;
        ClIocHeartBeatStatusT *entry =
            CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
        /*
         * Local components health check status
         */
        compAddr.portId = entry->linkIndex;
        rc = clIocHeartBeatMessageReqRep(eoObj->commObj,
                (ClIocAddressT *) &compAddr, CL_IOC_PROTO_HB, CL_FALSE);
        if (rc == CL_OK) 
        {
            if (entry->status == CL_IOC_NODE_UP
                && entry->linkIndex != CL_IOC_XPORT_PORT
                && entry->linkIndex != CL_IOC_CPM_PORT) 
            {
                if (entry->retryCount > gClHeartBeatRetries) 
                {
                    clLogNotice(
                            "IOC",
                            "HBT",
                            "HeartBeat component [0x%x]'s status death", entry->linkIndex);
                    entry->status = CL_IOC_NODE_DOWN;
                    entry->retryCount = 0;
                    __ALLOC_HB_NOTIFICATIONS;
                    hbNotifications[numNotifications].addr.nodeAddress = gIocLocalBladeAddress;
                    hbNotifications[numNotifications].addr.portId = entry->linkIndex;
                    hbNotifications[numNotifications].event = CL_IOC_COMP_DEATH_NOTIFICATION;
                    ++numNotifications;
                    _clIocHeartBeatEntryMapDel(entry);
                    clHeapFree(entry);
                } 
                else 
                {
                    entry->retryCount++;
                }
            }
        }
    }
    clOsalMutexUnlock(&gIocHeartBeatLocalTableLock);

    for(i = 0; i < numNotifications; ++i)
    {
        switch(hbNotifications[i].event)
        {
        case CL_IOC_COMP_DEATH_NOTIFICATION:
            /*
             * Notify port close
             */
            clTransportNotificationClose(NULL, 
                                         hbNotifications[i].addr.nodeAddress,
                                         hbNotifications[i].addr.portId,
                                         CL_IOC_COMP_DEATH_NOTIFICATION);
            break;
        default: break;
        }
    }
    
    if(hbNotifications)
        clHeapFree(hbNotifications);

    return rc;
}

/*
 * Got notification node/comp arrival and update hash map
 */
static ClRcT _clHeartBeatUpdateNotification(ClIocNotificationT *notification, ClPtrT cookie)
{
    ClIocNotificationIdT notificationId = ntohl(notification->id);
    ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
    switch (notificationId) 
    {
    case CL_IOC_COMP_ARRIVAL_NOTIFICATION: 
        {
            if (nodeAddress == gIocLocalBladeAddress) 
            {
                clOsalMutexLock(&gIocHeartBeatLocalTableLock);
                ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatLocalMapFind(
                                                                              portId);
                if (!hbStatus) 
                {
                    hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
                    hbStatus->linkIndex = portId;
                    hbStatus->status = CL_IOC_NODE_UP;
                    hbStatus->retryCount = 0;
                    _clIocHeartBeatLocalEntryMapAdd(hbStatus);
                }
                clOsalMutexUnlock(&gIocHeartBeatLocalTableLock);
            } 
            else 
            {
                clOsalMutexLock(&gIocHeartBeatTableLock);
                ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatMapFind(
                                                                         nodeAddress);
                if (!hbStatus) 
                {
                    hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
                    hbStatus->linkIndex = nodeAddress;
                    hbStatus->status = CL_IOC_NODE_UP;
                    hbStatus->retryCount = 0;
                    if(_clIocHeartBeatEntryMapAdd(hbStatus) != CL_OK)
                    {
                        clHeapFree(hbStatus);
                        hbStatus = NULL;
                    }
                } 
                else 
                {
                    hbStatus->status = CL_IOC_NODE_UP;
                    hbStatus->retryCount = 0;
                }
                clOsalMutexUnlock(&gIocHeartBeatTableLock);
            }
            break;
        }
    case CL_IOC_NODE_ARRIVAL_NOTIFICATION: 
        {
            clOsalMutexLock(&gIocHeartBeatTableLock);
            ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatMapFind(nodeAddress);
            if (!hbStatus) 
            {
                hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
                hbStatus->linkIndex = nodeAddress;
                hbStatus->status = CL_IOC_NODE_UP;
                hbStatus->retryCount = 0;
                if(_clIocHeartBeatEntryMapAdd(hbStatus) != CL_OK)
                {
                    clHeapFree(hbStatus);
                    hbStatus = NULL;
                }
            } 
            else 
            {
                hbStatus->status = CL_IOC_NODE_UP;
                hbStatus->retryCount = 0;
            }
            clOsalMutexUnlock(&gIocHeartBeatTableLock);
            break;
        }
    case CL_IOC_NODE_LEAVE_NOTIFICATION: 
        {
            clOsalMutexLock(&gIocHeartBeatTableLock);
            ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatMapFind(nodeAddress);
            if (hbStatus) 
            {
                _clIocHeartBeatEntryMapDel(hbStatus);
            }
            clOsalMutexUnlock(&gIocHeartBeatTableLock);
            if(hbStatus)
            {
                clHeapFree(hbStatus);
            }
            break;
        }
    case CL_IOC_COMP_DEATH_NOTIFICATION: 
        {
            if (nodeAddress == gIocLocalBladeAddress) 
            {
                clOsalMutexLock(&gIocHeartBeatLocalTableLock);
                ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatLocalMapFind(portId);
                if (hbStatus) 
                {
                    _clIocHeartBeatEntryMapDel(hbStatus);
                }
                clOsalMutexUnlock(&gIocHeartBeatLocalTableLock);
                if(hbStatus)
                {
                    clHeapFree(hbStatus);
                }
            }
            break;
        }

    default: 
        {
            break;
        }
    }
    return CL_OK;
}

/*
 * The method for network heartbeat running on master node
 * Step 1
 *      Get neighbors node list, insert into hashtable
 * Step 2
 *      Create timer for interval sending to all peer nodes
 *
 */
ClRcT clIocHeartBeatStart() 
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeOut = {
            .tsSec = gClHeartBeatInterval / 1000,
            .tsMilliSec = gClHeartBeatInterval % 1000
    };

    if (gHeartBeatTimer != CL_HANDLE_INVALID_VALUE) {
        clLogError("IOC", "HBT", "Timer already initialized!");
        return rc;
    }

    ClUint32T numNodes = 0, i = 0;
    ClIocNodeAddressT *pNodes = NULL;

    rc = clIocTotalNeighborEntryGet(&numNodes);
    if(rc != CL_OK)
    {
        clLogError("IOC", "HBT", "Failed to get the number of neighbors in ASP system. error code [0x%x].", rc);
        goto out;
    }

    pNodes = (ClIocNodeAddressT *)clHeapAllocate(sizeof(ClIocNodeAddressT) * numNodes);
    if(pNodes == NULL)
    {
        clLogError("IOC", "HBT", "Failed to allocate [%zd] bytes of memory. error code [0x%x].", sizeof(ClIocNodeAddressT) * numNodes, rc);
        goto out;
    }

    rc = clIocNeighborListGet(&numNodes, pNodes);
    if(rc != CL_OK)
    {
        clLogError("IOC", "HBT", "Failed to get the neighbor node addresses. error code [0x%x].", rc);
        goto out;
    }

    clOsalMutexLock(&gIocHeartBeatTableLock);

    /* insert into heart beat */
    for (i = 0; i < numNodes; i++) 
    {
        ClIocHeartBeatStatusT *hbStatus = _clIocHeartBeatMapFind(pNodes[i]);
        if (!hbStatus) 
        {
            hbStatus = clHeapCalloc(1, sizeof(*hbStatus));
            hbStatus->linkIndex = pNodes[i];
            hbStatus->status = CL_IOC_NODE_UP;
            hbStatus->retryCount = 0;
            if(_clIocHeartBeatEntryMapAdd(hbStatus) != CL_OK)
            {
                clHeapFree(hbStatus);
            }
        }
    }

    clOsalMutexUnlock(&gIocHeartBeatTableLock);

    clLogDebug("IOC", "HBT", "Starting heartbeat on node [%d] with [%d] peers",
               gIocLocalBladeAddress, numNodes > 1 ? numNodes - 1 : 0);

    rc = clTimerCreateAndStart(timeOut, CL_TIMER_REPETITIVE,
            CL_TIMER_SEPARATE_CONTEXT,
            (ClTimerCallBackT) _clIocHeartBeatSend, (void *) NULL,
            &gHeartBeatTimer);

    clHeapFree(pNodes);
    rc = CL_OK;

    out:
    return rc;
}

/*
 * Callback method to register TRACK_CHANGE
 */
static
void clHeartBeatTrackCallback(ClGmsClusterNotificationBufferT *notificationBuffer,
                              ClUint32T                       numberOfMembers,
                              ClRcT                           rc)
{

    clLogDebug("IOC", "HBT", "Received trackcallback with leader [%d]",
               notificationBuffer->leader);

    if (notificationBuffer->leader == -1)
    {
        /*
         * If going leaderless, disable hb.
         */
        if (CL_IOC_HB_STATUS_ACTIVE())
        {
            clLogDebug("IOC", "HBT", "Shutting down heartbeat for node [%d]", 
                       gIocLocalBladeAddress);
            CL_IOC_HB_CLEAR_STATUS_ACTIVE();
            if(gHeartBeatTimer != CL_HANDLE_INVALID_VALUE)
            {
                clTimerDelete(&gHeartBeatTimer);
            }

            register ClListHeadT *iter;

            // Free peer node's status entry
            clOsalMutexLock(&gIocHeartBeatTableLock);
            while(!CL_LIST_HEAD_EMPTY(&gIocHeartBeatList)) 
            {
                iter = gIocHeartBeatList.pNext;
                ClIocHeartBeatStatusT *entry =
                    CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
                _clIocHeartBeatEntryMapDel(entry);
                clHeapFree(entry);
            }
            clOsalMutexUnlock(&gIocHeartBeatTableLock);
        }
        /*
         * Go out if I'm not leader
         */
        return;
    }

    if (!CL_IOC_HB_STATUS_ACTIVE())
    {
        ClRcT rc = clIocHeartBeatStart();
        if (rc == CL_OK)
        {
            CL_IOC_HB_SET_STATUS_ACTIVE();
        }
    } 
    else 
    {
        clOsalMutexLock(&gIocHeartBeatTableLock);
        register ClListHeadT *iter;
        CL_LIST_FOR_EACH(iter, &gIocHeartBeatList) 
        {
            ClIocHeartBeatStatusT *entry =
                CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
            if(entry->status != CL_IOC_LINK_DOWN
               &&
               entry->status != CL_IOC_LINK_UP)
            {
                entry->retryCount = 0;
                entry->status = CL_IOC_NODE_UP;
            }
        }
        clOsalMutexUnlock(&gIocHeartBeatTableLock);
    }
}

/*
 * Create timer to initialize gms
 */
static ClRcT _clHeartBeatGmsTimerInitCallback() {
    ClRcT rc = CL_OK;
    ClVersionT version = { 'B', 0x1, 0x1 };
    ClGmsCallbacksT gGmsCallbacks = {
            NULL,
            (ClGmsClusterTrackCallbackT) clHeartBeatTrackCallback,
            NULL,
            NULL,
    };
    ClTimerTimeOutT timeout = { 0, 1000 };
    ClBoolT *pData = NULL;

    /*
     * Contact GMS to get master addresses.
     */
    ClGmsClusterNotificationBufferT notBuffer;
    memset((void*)&notBuffer , 0, sizeof(notBuffer));
    memset( &notBuffer , '\0' ,sizeof(ClGmsClusterNotificationBufferT));

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
        rc = clGmsInitialize(&gHeartBeatHandleGms, &gGmsCallbacks, &version);
        if (CL_OK != rc) {
            retry:
            if (CL_HANDLE_INVALID_VALUE != gHandleGmsTimer) {
                clTimerDelete(&gHandleGmsTimer);
                gHandleGmsTimer = CL_HANDLE_INVALID_VALUE;
            }
            rc = clTimerCreateAndStart(timeout, CL_TIMER_ONE_SHOT,
                    CL_TIMER_SEPARATE_CONTEXT, _clHeartBeatGmsTimerInitCallback,
                    pData, &gHandleGmsTimer);
            if (CL_OK != rc) 
            {
                clLogError("IOC", "HBT",
                           "Failed to do GMS initialization, error [%#x]", rc);
            }
            return CL_OK;
        }
        clTimerDelete(&gHandleGmsTimer);

        /*
         * Register for track changes.
         */
        rc = clGmsClusterTrack(gHeartBeatHandleGms,
                               CL_GMS_TRACK_CHANGES,
                               &notBuffer);
        if (CL_OK != rc)
        {
            clLogError("IOC", "HBT",
                       "The GMS cluster track function failed, error [%#x]",
                       rc);
        }

        gGmsInitDone = CL_TRUE;
    }
    return rc;
}

static ClRcT iocCompFastNotifyCallback(ClIocPhysicalAddressT *compAddr, 
                                       ClUint32T status, ClPtrT arg)
{
    if(status == CL_IOC_NODE_DOWN)
    {
        clTransportNotificationClose(NULL, gIocLocalBladeAddress, 
                                     compAddr->portId, CL_IOC_COMP_DEATH_NOTIFICATION);
    }
    return CL_OK;
}

/*
 * The function use for local component heartbeat running on CPM (amf)
 * Step:
 * Create timer for interval sending heartbeat to all components
 * only if the underlying system doesn't support a much faster passive failure
 * detection that uses inotify for detecting component failovers.
 *
 */
static ClRcT HeartBeatPluginDefault(ClUint32T interval, ClUint32T retires) {
    ClRcT rc = CL_OK;    
    rc = clTransportNotifyInitialize();
    if(rc == CL_OK)
    {
        rc = clTransportNotifyRegister(iocCompFastNotifyCallback, NULL);
        if(rc != CL_OK)
        {
            clTransportNotifyFinalize();
        }
        else
        {
            clLogNotice("IOC", "HBT", 
                        "Heartbeat set to fast failure detection for local components");
            goto out;
        }
    }

    ClTimerTimeOutT timeOut = { .tsSec = interval
            / 1000, .tsMilliSec = interval % 1000 };
    
    rc = CL_ERR_INVALID_PARAMETER;

    if (gHeartBeatLocalTimer != CL_HANDLE_INVALID_VALUE) {
        clLogError("IOC", "HBT", "Timer already initialized!");
        return rc;
    }
    /*
     * TODO
     * Using plugin for easy replace with other hearbeat algorithm.
     */
    rc = clTimerCreateAndStart(timeOut, CL_TIMER_REPETITIVE,
            CL_TIMER_SEPARATE_CONTEXT,
            (ClTimerCallBackT) _clIocHeartBeatCompsSend, (void *) NULL,
            &gHeartBeatLocalTimer);

    out:
    return rc;
}

ClRcT _clIocSetHeartBeatConfig()
{

    ClParserPtrT   parent     = NULL ;   /*parent*/
    ClRcT          rc         = CL_OK;
    ClCharT       *configPath = NULL;

    configPath = getenv("ASP_CONFIG");
    if (configPath != NULL)
    {
        parent = clParserOpenFile(configPath, CL_TRANSPORT_CONFIG_FILE);
        if (parent == NULL)
        {
            clLogWarning("IOC", "HBT", "Transport configuration file does not exist. Should default to tipc");
            return CL_ERR_NULL_POINTER;
        }
    }
    else
    {
        rc = CL_ERR_NULL_POINTER;
        clLogError("IOC", "HBT", "ASP_CONFIG path is not set in the environment rc[%#x]", rc);
    }

    ClParserPtrT heartbeatPtr = clParserChild(parent, "heartbeat");
    if (!heartbeatPtr)
    {
        gClHeartBeatInterval = gClHeartBeatIntervalLocal = CL_IOC_HB_INTERVAL_DEFAULT;
        gClHeartBeatRetries = CL_IOC_MAX_HB_RETRIES_DEFAULT;
        /*
         * Uncomment to test local heartbeat without heartbeat tag in the config file
         * albeit I would prefer a heartbeat enable/disable flag in the config file.
         */
        /*gClHeartBeatPlugin = HeartBeatPluginDefault;*/
        goto out;
    }
    
    /*
     * Set the default plugin if heartbeat tag is set.
     */
    gClHeartBeatPlugin = HeartBeatPluginDefault;

    ClParserPtrT hearbeatIntervalPtr = clParserChild(heartbeatPtr, "interval");
    ClParserPtrT hearbeatIntervalLocalPtr = clParserChild(heartbeatPtr, "intervalLocal");
    ClParserPtrT hearbeatRetriesPtr = clParserChild(heartbeatPtr, "retries");
    ClParserPtrT hearbeatPluginPtr = clParserChild(heartbeatPtr, "plugin");
    
    if (!hearbeatIntervalPtr)
    {
        gClHeartBeatInterval = CL_IOC_HB_INTERVAL_DEFAULT;
    }
    else
    {
        gClHeartBeatInterval = atoi(hearbeatIntervalPtr->txt);
    }

    if (!hearbeatIntervalLocalPtr)
    {
        gClHeartBeatIntervalLocal = CL_IOC_HB_INTERVAL_DEFAULT;
    }
    else
    {
        gClHeartBeatIntervalLocal = atoi(hearbeatIntervalLocalPtr->txt);
    }

    if (!hearbeatRetriesPtr)
    {
        gClHeartBeatRetries = CL_IOC_MAX_HB_RETRIES_DEFAULT;
    }
    else
    {
        gClHeartBeatRetries = atoi(hearbeatRetriesPtr->txt);
    }

    if (hearbeatPluginPtr)
    {
        gClPluginHandle = dlopen(hearbeatPluginPtr->txt, RTLD_GLOBAL | RTLD_NOW);
        if (!gClPluginHandle) {
            clLogWarning("IOC","HBT", "HeartBeat is not running for local component");
            gClHeartBeatPlugin = HeartBeatPluginDefault;
        } else {
            *(void**) &gClHeartBeatPlugin = dlsym(gClPluginHandle, CL_IOC_HB_METHOD);
            if (!gClHeartBeatPlugin)
            {
                clLogWarning(
                        "IOC",
                        "HBT",
                        "Unable to find the heartbeat method in plugin : %s", dlerror());
                *(void**) &gClHeartBeatPlugin = HeartBeatPluginDefault;
                dlclose(gClPluginHandle);
                gClPluginHandle = NULL;
            }
        }
    }

    out:
    if (parent != NULL)
        clParserFree(parent);
    return rc;
}

ClRcT clIocHeartBeatInitialize(ClBoolT nodeRep) {
    ClRcT rc = CL_OK;

    if (nodeRep) {

        _clIocSetHeartBeatConfig();

        clOsalMutexInit(&gIocHeartBeatLocalTableLock);
        clOsalMutexInit(&gIocHeartBeatTableLock);

        /*
         * Don't enable hearbeating unless the hearbeat tag is set in the config file
         */
        if (gClHeartBeatPlugin != NULL)
        {
            /*
             * Register current track to running heartbeat on master only
             */
            rc = _clHeartBeatGmsTimerInitCallback();
            gClHeartBeatPlugin(gClHeartBeatIntervalLocal, gClHeartBeatRetries);
        }

        /*
         * To do a fast pass early update node entry table
         */
        clIocNotificationRegister(_clHeartBeatUpdateNotification, NULL);
    }
    return rc;
}

/*
 * Pause or temporarily disable the hearbeating
 */
ClRcT clIocHeartBeatPause(void)
{
    CL_IOC_HB_SET_STATUS_PAUSED();
    return CL_OK;
}

ClRcT clIocHeartBeatUnpause(void)
{
    CL_IOC_HB_CLEAR_STATUS_PAUSED();
    return CL_OK;
}

/*
 * Active/paused/inactive: returns constant (.rodata)
 */
const ClCharT *clIocHeartBeatStatusGet(void)
{
    const ClCharT *status = "inactive";

    if(CL_IOC_HB_STATUS_ACTIVE())
    {
        status = "active/running";
        if(CL_IOC_HB_STATUS_PAUSED())
            status = "active/paused";
    }
    else if(CL_IOC_HB_STATUS_PAUSED())
    {
        status = "inactive/paused";
    }
    
    return status;
}

ClRcT clIocHeartBeatFinalize(ClBoolT nodeRep) {
    ClRcT rc = CL_OK;
    if (nodeRep)
    {
        CL_IOC_HB_CLEAR_STATUS_ACTIVE();

        clIocNotificationDeregister(_clHeartBeatUpdateNotification);
        clTransportNotifyFinalize();
        /*
         * Free peer node's status entry
         */
        if (gHeartBeatTimer != CL_HANDLE_INVALID_VALUE)
        {
            clTimerDelete(&gHeartBeatTimer);
        }
        if (gHeartBeatLocalTimer != CL_HANDLE_INVALID_VALUE)
        {
            clTimerDelete(&gHeartBeatLocalTimer);
        }
        if(gClPluginHandle)
        {
            dlclose(gClPluginHandle);
        }
        register ClListHeadT *iter;
        clOsalMutexLock(&gIocHeartBeatTableLock);
        while(!CL_LIST_HEAD_EMPTY(&gIocHeartBeatList))
        {
            iter = gIocHeartBeatList.pNext;
            ClIocHeartBeatStatusT *entry =
                    CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
            _clIocHeartBeatEntryMapDel(entry);
            clHeapFree(entry);
        }
        clOsalMutexUnlock(&gIocHeartBeatTableLock);

        // Free local component's status entry
        clOsalMutexLock(&gIocHeartBeatLocalTableLock);
        while(!CL_LIST_HEAD_EMPTY(&gIocHeartBeatLocalList)) 
        {
            iter = gIocHeartBeatLocalList.pNext;
            ClIocHeartBeatStatusT *entry =
                    CL_LIST_ENTRY(iter, ClIocHeartBeatStatusT, list);
            _clIocHeartBeatEntryMapDel(entry);
            clHeapFree(entry);
        }
        clOsalMutexUnlock(&gIocHeartBeatLocalTableLock);
        if (gHeartBeatHandleGms)
        {
            clGmsFinalize(gHeartBeatHandleGms);
        }
    }
    return rc;
}

/*
 * Format the hello message and send request/reply
 */
ClRcT clIocHeartBeatMessageReqRep(ClIocCommPortHandleT commPort,
        ClIocAddressT *destAddress, ClUint32T reqRep, ClBoolT shutdown) {
    ClRcT rc = CL_OK;

    ClBufferHandleT message = 0;
    clBufferCreate(&message);
    if (!shutdown)
    {
        rc = clBufferNBytesWrite(message, (ClUint8T *) &CL_IOC_HB_HELLO_MESSAGE,
                sizeof(CL_IOC_HB_HELLO_MESSAGE));
    }
    else
    {
        rc = clBufferNBytesWrite(message, (ClUint8T *) &CL_IOC_HB_EXIT_MESSAGE,
                sizeof(CL_IOC_HB_EXIT_MESSAGE));
    }

    if (rc != CL_OK) 
    {
        clLogError("IOC", "HBT", "clBufferNBytesWrite failed with rc = %#x", rc);
        goto out_delete;
    }

    ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY, .timeout =
            200 };

    rc = clIocSend(commPort, message, reqRep, destAddress, &sendOption);

    out_delete:
    clBufferDelete(&message);

    return rc;
}
