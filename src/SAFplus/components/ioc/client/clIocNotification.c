#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clList.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocServices.h>
#include <clIocIpi.h>
#include <clNodeCache.h>
#include <clTransport.h>
#include <clCpmApi.h>
#include "clIocUserApi.h"
#include "clIocSetup.h"
#include "clIocNeighComps.h"
#include "clIocMaster.h"

typedef struct ClIocNotificationRegister
{
    ClListHeadT list;
    ClIocNotificationRegisterCallbackT callback;
    ClPtrT cookie;
}ClIocNotificationRegisterT;

static CL_LIST_HEAD_DECLARE(gIocNotificationRegisterList);
static ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY, .timeout = 2000 };
static ClOsalMutexT gIocNotificationRegisterLock;
static ClBoolT gIocNotificationInitialized;

static ClIocNotificationRegisterT* clIocNotificationRegisterFind(ClIocNotificationRegisterCallbackT callback)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *list = &gIocNotificationRegisterList;
    CL_LIST_FOR_EACH(iter, list)
    {
        ClIocNotificationRegisterT *entry = CL_LIST_ENTRY(iter, ClIocNotificationRegisterT, list);
        if(entry->callback == callback)
            return entry;
    }
    return NULL;
}

ClRcT clIocNotificationRegister(ClIocNotificationRegisterCallbackT callback, ClPtrT cookie)
{
    ClIocNotificationRegisterT *registrant = NULL;
    if(!callback) return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    registrant = clHeapCalloc(1, sizeof(*registrant));
    CL_ASSERT(registrant != NULL);
    registrant->callback = callback;
    registrant->cookie = cookie;

    clOsalMutexLock(&gIocNotificationRegisterLock);
    clListAddTail(&registrant->list, &gIocNotificationRegisterList);
    clOsalMutexUnlock(&gIocNotificationRegisterLock);
    return CL_OK;
}

ClRcT clIocNotificationDeregister(ClIocNotificationRegisterCallbackT callback)
{
    ClIocNotificationRegisterT *entry = NULL;
    clOsalMutexLock(&gIocNotificationRegisterLock);
    entry = clIocNotificationRegisterFind(callback);
    if(entry)
    {
        clListDel(&entry->list);
    }
    clOsalMutexUnlock(&gIocNotificationRegisterLock);
    if(entry) clHeapFree(entry);
    return CL_OK;
}

ClRcT clIocNotificationRegistrants(ClIocNotificationT *notification)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *list = &gIocNotificationRegisterList;

    clOsalMutexLock(&gIocNotificationRegisterLock);
    CL_LIST_FOR_EACH(iter, list)
    {
        ClIocNotificationRegisterT *entry = CL_LIST_ENTRY(iter, ClIocNotificationRegisterT, list);
        if(entry->callback)
        {
            entry->callback(notification, entry->cookie);
        }
    }
    clOsalMutexUnlock(&gIocNotificationRegisterLock);
    return CL_OK;
}

ClRcT clIocNotificationRegistrantsDelete(void)
{
    clOsalMutexLock(&gIocNotificationRegisterLock);
    while(!CL_LIST_HEAD_EMPTY(&gIocNotificationRegisterList))
    {
        ClListHeadT *head = gIocNotificationRegisterList.pNext;
        ClIocNotificationRegisterT *entry = CL_LIST_ENTRY(head, ClIocNotificationRegisterT, list);
        clListDel(&entry->list);
        clHeapFree(entry);
    }
    clOsalMutexUnlock(&gIocNotificationRegisterLock);
    return CL_OK;
}

ClRcT clIocNotificationProxySend(ClIocCommPortHandleT commPort,
                                 ClIocNotificationT *notification,
                                 ClIocPhysicalAddressT *srcAddr, 
                                 ClIocAddressT *destAddr,
                                 ClCharT *xportType)
{
    ClRcT retCode = CL_OK;
    ClBufferHandleT message = 0;

    retCode = clBufferCreate(&message);
    if(retCode != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Buffer creation failed. rc=0x%x\n",retCode));
        goto out;
    }   

    retCode = clBufferNBytesWrite(message,(ClUint8T *)&notification, sizeof(notification));
    if (CL_OK != retCode)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nERROR: clBufferNBytesWrite failed with rc = %x\n",
                        retCode));
        goto err_out;
    }   

    clLogTrace("PROXY", "SEND", "Proxying [%d] notification to the peer node reps "
               "for node [%d:%d]", notification->id, srcAddr->nodeAddress, srcAddr->portId);

    retCode = clIocSendWithXport(commPort, message, CL_IOC_PROTO_ARP,
                                 destAddr, &sendOption, xportType, CL_TRUE);
    if(retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send proxy notification. "
                                       "Error [%#x]", retCode));

    err_out:
    clBufferDelete(&message);

    out:
    return retCode;
}

static ClRcT clIocNotificationNodeNamePack(ClBufferHandleT message) 
{
    ClRcT retCode = CL_OK;
    static ClNameT nodeName = {0};
    static ClUint16T len;

    if(!len)
    {
        clCpmLocalNodeNameGet(&nodeName);
        len = htons(nodeName.length);
    }
    retCode = clBufferNBytesWrite(message, (ClUint8T*)&len, sizeof(len));
    retCode |= clBufferNBytesWrite(message, (ClUint8T*)nodeName.value, (ClUint32T)nodeName.length);
    if(retCode != CL_OK)
    {
        clLogError("NOTIF", "PACK", 
                   "Nodename marshall for notification version send failed with [%#x]", 
                   retCode);
        goto out;
    }

    out:
    return retCode;
}

ClRcT clIocNotificationDiscoveryPack(ClBufferHandleT message,
                                     ClIocNotificationIdT id,
                                     ClIocNotificationT *pNotification, 
                                     ClBoolT compat)
{
    ClRcT retCode = CL_OK;
    static ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
    ClUint32T myCapability = 0;

    clNodeCacheVersionAndCapabilityGet(gIocLocalBladeAddress, &nodeVersion, &myCapability);
    pNotification->id = htonl(id);
    pNotification->protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    pNotification->nodeVersion = htonl(nodeVersion);
    pNotification->nodeAddress.iocPhyAddress.portId = htonl(myCapability);
    pNotification->nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
    retCode = clBufferNBytesWrite(message,(ClUint8T *)pNotification, sizeof(*pNotification));
    if (CL_OK != retCode)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nERROR: clBufferNBytesWrite failed with rc = %x\n",
                        retCode));
        goto out;
    }   

    if(id == CL_IOC_NODE_VERSION_NOTIFICATION
       ||
       id == CL_IOC_NODE_VERSION_REPLY_NOTIFICATION)
    {
        if(!compat)
        {
            retCode = clIocNotificationNodeNamePack(message);
        }
    }

    out:
    return retCode;
}

ClRcT clIocNotificationPacketSend(ClIocCommPortHandleT commPort, 
                                  ClIocNotificationT *pNotificationInfo, 
                                  ClIocAddressT *destAddress, ClBoolT compat,
                                  ClCharT *xportType)
{
    ClRcT retCode = CL_OK;
    ClBufferHandleT message = 0;
    ClIocNotificationIdT id = ntohl(pNotificationInfo->id);

    retCode = clBufferCreate(&message);
    if(retCode != CL_OK)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Buffer creation failed. rc=0x%x\n",retCode));
        goto out;
    }   

    retCode = clBufferNBytesWrite(message,(ClUint8T *)pNotificationInfo, sizeof(*pNotificationInfo));
    if (CL_OK != retCode)
    {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nERROR: clBufferNBytesWrite failed with rc = %x\n",
                        retCode));
        goto err_out;
    }   
    if(id == CL_IOC_NODE_VERSION_NOTIFICATION || id == CL_IOC_NODE_VERSION_REPLY_NOTIFICATION)
    {
        if(!compat)
        {
            retCode = clIocNotificationNodeNamePack(message);
            if(retCode != CL_OK)
                goto err_out;
        }
    }

    /* clLogInfo("Sending discovery notification for myself []"); */
    retCode = clIocSendWithXport(commPort, message, CL_IOC_PORT_NOTIFICATION_PROTO, destAddress, &sendOption, xportType, CL_FALSE);
    if(retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send notification. error code [0x%x]", retCode));

    err_out:
    clBufferDelete(&message);

    out:
    return retCode;
}

static ClRcT clIocNotificationDiscoveryUnpack(ClUint8T *recvBuff,
                                              ClUint32T recvLen,
                                              ClIocPhysicalAddressT *srcAddr, 
                                              ClIocNotificationT *notification,
                                              ClIocCommPortHandleT commPort,
                                              ClCharT *xportType)
{
    ClRcT rc = CL_OK;
    ClIocAddressT destAddress = {{0}};
    ClUint32T version = ntohl(notification->nodeVersion);
    ClIocNodeAddressT nodeId = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
    ClUint32T theirCapability = ntohl(notification->nodeAddress.iocPhyAddress.portId);
    ClIocNotificationIdT id = ntohl(notification->id);
    ClNameT nodeName = {0};
    ClUint8T *nodeInfo = NULL;
    ClUint32T nodeInfoLen = 0;
    ClBoolT compat = CL_FALSE;

    destAddress.iocPhyAddress.nodeAddress = srcAddr->nodeAddress;
    destAddress.iocPhyAddress.portId = srcAddr->portId;

    if(id == CL_IOC_NODE_LINK_UP_NOTIFICATION)
    {
        clLogDebug("IOC", "NTF", "Discovery notification of node id [%d] LINK UP",nodeId);
        clNodeCacheUpdate(nodeId, version, 0, NULL);
        clIocNotificationRegistrants(notification);
        goto out;
    }

    /*
     * Check for backward compatibility
     */
    if(version >= CL_VERSION_CODE(5, 0, 0))
    {
        nodeInfo = recvBuff + sizeof(*notification);
        nodeInfoLen = recvLen - sizeof(*notification);
        if(nodeInfoLen < sizeof(nodeName.length))
        {
            clLogError("NOTIF", "GET", "Invalid discovery data received with available "
                       "data length of [%d] bytes", nodeInfoLen);
            rc = CL_IOC_RC(CL_ERR_NO_SPACE);
            goto out;
        }
        nodeInfoLen -= sizeof(nodeName.length);
        memcpy(&nodeName.length, nodeInfo, sizeof(nodeName.length));
        nodeName.length = ntohs(nodeName.length);
        nodeInfo += sizeof(nodeName.length);
        if(nodeInfoLen < nodeName.length)
        {
            clLogError("NOTIF", "GET", "Invalid discovery data received for node version notification."
                       "Node length received [%d] with only [%d] bytes of available input data",
                       nodeName.length, nodeInfoLen);
            rc = CL_IOC_RC(CL_ERR_NO_SPACE);
            goto out;
        }
        memcpy(nodeName.value, nodeInfo, CL_MIN(sizeof(nodeName.value)-1, nodeName.length));
        nodeName.value[CL_MIN(sizeof(nodeName.value)-1, nodeName.length)] = 0;
    }
    else
    {
        compat = CL_TRUE;
    }

    clLogDebug("IOC", "NTF", "Discovery notification of node [%s] id [%d], capability [0x%x]", nodeName.value, nodeId, theirCapability);
    clNodeCacheUpdate(nodeId, version, theirCapability, &nodeName);
                
    /*
     * Send back node reply to peer for version notifications. with our info.
     */
    if(id == CL_IOC_NODE_VERSION_NOTIFICATION)
    {
        static ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
        ClUint32T myCapability = 0;
        clNodeCacheVersionAndCapabilityGet(gIocLocalBladeAddress, &nodeVersion, &myCapability);
        notification->id = htonl(CL_IOC_NODE_VERSION_REPLY_NOTIFICATION);
        notification->nodeVersion = htonl(nodeVersion);
        notification->nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
        notification->nodeAddress.iocPhyAddress.portId = htonl(myCapability);

        clLogDebug("IOC", "NTF", "Sending return discovery notification for myself at [%d], capability [0x%x]", gIocLocalBladeAddress,myCapability);
        rc = clIocNotificationPacketSend(commPort, notification, &destAddress, compat, xportType);
    }
    

    out:
    return rc;
}

/*
 * Send the node bitmap for this send
 */
static ClRcT clIocNotificationNodeMapSend(ClIocCommPortHandleT commPort, 
                                          ClIocAddressT *compAddress, ClCharT *xportType)
{
    ClUint8T buff[CL_IOC_BYTES_FOR_COMPS_PER_NODE];
    ClBufferHandleT message = 0;
    ClRcT rc = CL_OK;

    clBufferCreate(&message);
    clIocNodeCompsGet(gIocLocalBladeAddress, buff);
    rc = clBufferNBytesWrite(message, buff, sizeof(buff));
    if(rc != CL_OK) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("\nERROR: clBufferNBytesWrite failed with rc = %x\n", rc));
        goto out_delete;
    }

    rc = clIocSendWithXport(commPort, message, CL_IOC_PROTO_CTL, 
                            compAddress, &sendOption, xportType, CL_FALSE);

    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send notification node map. error code 0x%x", rc));
    }

    out_delete:
    clBufferDelete(&message);
    return rc;
}

/* 
 * This is for NODE ARRIVAL/DEPARTURE 
*/

static ClRcT clIocNodeVersionSend(ClIocCommPortHandleT commPort, 
                                  ClIocAddressT *destAddress, ClCharT *xportType,
                                  ClIocNotificationIdT id)
{
    ClIocNotificationT notification = {0};
    static ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
    ClUint32T myCapability = 0;
    clNodeCacheVersionAndCapabilityGet(gIocLocalBladeAddress, &nodeVersion, &myCapability);
    if(id != CL_IOC_NODE_LINK_UP_NOTIFICATION)
    {
        id = CL_IOC_NODE_VERSION_NOTIFICATION;
    }
    else
    {
        /*
         * No need to sync capabilities again on link up
         */
        // GAS, but never a bad idea!: myCapability = 0;
    }
    notification.id = htonl(id);
    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    notification.nodeVersion = htonl(nodeVersion);
    notification.nodeAddress.iocPhyAddress.portId = htonl(myCapability);
    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
    clLogNotice("NODE", "VERSION", "Sending node version [%#x], capability [%#x] to node [%u], port [%#x]", nodeVersion, myCapability, destAddress->iocPhyAddress.nodeAddress, destAddress->iocPhyAddress.portId);
    return clIocNotificationPacketSend(commPort, &notification, destAddress, CL_FALSE, xportType);
}

static ClRcT clIocNotificationProxyRecv(ClIocCommPortHandleT commPort, ClUint8T *buff,
                                        ClIocAddressT *allLocalComps,
                                        ClIocAddressT *allNodeReps, ClCharT *xportType)
{
    ClIocNotificationT notification = {0};
    ClIocNotificationT notificationBuffer = {0};
    ClUint32T status = 0;
    memcpy(&notificationBuffer, buff, sizeof(notificationBuffer));
    notification.id = ntohl(notificationBuffer.id);
    notification.protoVersion = ntohl(notificationBuffer.protoVersion);
    notification.nodeAddress.iocPhyAddress.nodeAddress = 
        ntohl(notificationBuffer.nodeAddress.iocPhyAddress.nodeAddress);
    notification.nodeAddress.iocPhyAddress.portId = 
        ntohl(notificationBuffer.nodeAddress.iocPhyAddress.portId);
    if(notification.protoVersion != CL_IOC_NOTIFICATION_VERSION)
    {
        clLogError("PROXY", "RECV", "Invalid proxy notification packet received "
                   "with version [%d]", notification.protoVersion);
        return CL_ERR_VERSION_MISMATCH;
    }
    clLogTrace("PROXY", "RECV", "Received proxy notification [%d] for node [%d:%d]",
               notification.id, notification.nodeAddress.iocPhyAddress.nodeAddress,
               notification.nodeAddress.iocPhyAddress.portId);
    if(notification.id == CL_IOC_NODE_ARRIVAL_NOTIFICATION
       ||
       notification.id == CL_IOC_COMP_ARRIVAL_NOTIFICATION)
    {
        status = CL_IOC_NODE_UP;
    }
    else
    {
        status = CL_IOC_NODE_DOWN;
    }
    clIocCompStatusSet(notification.nodeAddress.iocPhyAddress, status);

    if(status == CL_IOC_NODE_UP)
    {
        /* 
         * Received NODE ARRIVAL notification.
         * Send back node version again for consistency or link syncup point
         * and comp bitmap for this node
         */
        if(notification.id == CL_IOC_NODE_ARRIVAL_NOTIFICATION)
        {
            clIocNodeVersionSend(commPort, 
                                 (ClIocAddressT*)&notification.nodeAddress, xportType,
                                 notification.id);
            clIocNotificationNodeMapSend(commPort, 
                                         (ClIocAddressT*)&notification.nodeAddress, 
                                         xportType);
        }
#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE 
        return CL_OK;
#endif
    }
    else 
    {
        /* Received Node/comp LEAVE notification. */
        clIocMasterSegmentUpdate(notification.nodeAddress.iocPhyAddress);
        if(notification.id == CL_IOC_NODE_LEAVE_NOTIFICATION)
        {
            notificationBuffer.nodeAddress.iocPhyAddress.portId = 0;
            clIocNodeCompsReset(notification.nodeAddress.iocPhyAddress.nodeAddress);
            clNodeCacheSoftReset(notification.nodeAddress.iocPhyAddress.nodeAddress);
        }
    }
    
    /*
     * Notify the registrants for this notification who might want to do a fast
     * pass early than relying on the slightly slower notification proto callback.
     */
    if(notification.id == CL_IOC_NODE_ARRIVAL_NOTIFICATION
       ||
       notification.id == CL_IOC_NODE_LEAVE_NOTIFICATION)
    {
        clIocNotificationRegistrants(&notificationBuffer);
    }

    /* Need to send a notification packet to all the components on this node */
    return clIocNotificationPacketSend(commPort, &notificationBuffer, allLocalComps,CL_FALSE, xportType);
}

ClRcT clIocNotificationNodeStatusSend(ClIocCommPortHandleT commPort, 
                                      ClIocNotificationIdT id,
                                      ClIocNodeAddressT notificationNodeAddr,
                                      ClIocAddressT *allLocalComps, 
                                      ClIocAddressT *allNodeReps, ClCharT *xportType)
{
    ClIocPhysicalAddressT notificationCompAddr = {.nodeAddress = notificationNodeAddr,
                                                  .portId = CL_IOC_XPORT_PORT
    };
    ClIocNotificationT notification = {0};
    ClRcT rc = CL_OK;
    ClUint32T status;
 
    if(id == CL_IOC_COMP_ARRIVAL_NOTIFICATION)
        id = CL_IOC_NODE_ARRIVAL_NOTIFICATION;

    if(id == CL_IOC_COMP_DEATH_NOTIFICATION)
        id = CL_IOC_NODE_LEAVE_NOTIFICATION;

    if( id == CL_IOC_NODE_ARRIVAL_NOTIFICATION ||
        id == CL_IOC_NODE_LINK_UP_NOTIFICATION)
    {
        status = CL_IOC_NODE_UP;
    }
    else
    {
        status = CL_IOC_NODE_DOWN;
    }

    if(notificationNodeAddr == gIocLocalBladeAddress)
    {
        if(status == CL_IOC_NODE_DOWN)
        {
            clNodeCacheReset(gIocLocalBladeAddress);
        } 
        else 
        {
            /*
             * Send the node version to all node reps.
             */
            clIocCompStatusSet(notificationCompAddr, CL_IOC_NODE_UP);
            if(allNodeReps)
            {
                rc = clIocNodeVersionSend(commPort, allNodeReps, xportType, id);
            }
        }
        return rc;
    }

    clIocCompStatusSet(notificationCompAddr, status);

    if(status == CL_IOC_NODE_UP)
    {
        /* 
         * Received NODE ARRIVAL notification.
         * Send back node version again for consistency or link syncup point
         * and comp bitmap for this node
        */
        rc = clIocNodeVersionSend(commPort, (ClIocAddressT*)&notificationCompAddr, 
                                  xportType, id);
        if(rc != CL_OK)
        {
            if(id == CL_IOC_NODE_LINK_UP_NOTIFICATION)
            {
                clIocCompStatusSet(notificationCompAddr, CL_IOC_NODE_DOWN);
                goto out;
            }
        }

        rc = clIocNotificationNodeMapSend(commPort, (ClIocAddressT*)&notificationCompAddr, xportType);
        if(rc != CL_OK)
        {
            if(id == CL_IOC_NODE_LINK_UP_NOTIFICATION)
            {
                clIocCompStatusSet(notificationCompAddr, CL_IOC_NODE_DOWN);
                goto out;
            }
        }

#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE 
        return CL_OK;
#else
        notification.id = htonl(id);
#endif
    } 
    else 
    {
        /* Received Node LEAVE notification. */
        clIocMasterSegmentUpdate(notificationCompAddr);
        clIocNodeCompsReset(notificationNodeAddr);
        clNodeCacheSoftReset(notificationNodeAddr);
        notification.id = htonl(id);
    }

    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    notification.nodeAddress.iocPhyAddress.portId = 0;
    notification.nodeAddress.iocPhyAddress.nodeAddress =  htonl(notificationNodeAddr);

    if(clTransportBridgeEnabled(gIocLocalBladeAddress) && allNodeReps)
    {
        clIocNotificationProxySend(commPort, &notification, &notificationCompAddr, 
                                   allNodeReps, xportType);
    }

    /*
     * Notify the registrants for this notification who might want to do a fast
     * pass early than relying on the slightly slower notification proto callback.
     */
    clIocNotificationRegistrants(&notification);
    /* Need to send a notification packet to all the components on this node */
    rc = clIocNotificationPacketSend(commPort, &notification, allLocalComps, 
                                     CL_FALSE, xportType);
    out:
    return rc;
}

/* 
 * This is for LOCAL COMPONENT ARRIVAL/DEPARTURE 
*/

ClRcT clIocNotificationCompStatusSend(ClIocCommPortHandleT commPort, ClUint32T status,
                                      ClIocPortT portId,
                                      ClIocAddressT *allLocalComps, 
                                      ClIocAddressT *allNodeReps, ClCharT *xportType)
{
    ClIocPhysicalAddressT compAddr = {.nodeAddress = gIocLocalBladeAddress,
                                      .portId = portId
    };
    ClIocNotificationT notification = {0};

    clIocCompStatusSet(compAddr, status);

    if(status == CL_IOC_NODE_DOWN)
    {
        if(portId == CL_IOC_CPM_PORT)
        {
            /*
             * self shutdown.
             */
            return CL_OK;
        }
        clIocMasterSegmentUpdate(compAddr);
        notification.id = htonl(CL_IOC_COMP_DEATH_NOTIFICATION);
    }
    else
    {
        notification.id = htonl(CL_IOC_COMP_ARRIVAL_NOTIFICATION);
    }
    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    notification.nodeAddress.iocPhyAddress.portId = htonl(compAddr.portId);
    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(compAddr.nodeAddress);
    /*
     * Notify the registrants for this notification for faster processing
     */
    clIocNotificationRegistrants(&notification);

    /* Need to send a notification packet to all the Amfs's event handlers. */
    return clIocNotificationPacketSend(commPort, &notification, allNodeReps, CL_FALSE, xportType);
}         

/* 
 * Packet is received from the other node amfs/NODE-REPS
*/

ClRcT clIocNotificationPacketRecv(ClIocCommPortHandleT commPort, ClUint8T *recvBuff, ClUint32T recvLen,
                                  ClIocAddressT *allLocalComps, ClIocAddressT *allNodeReps,
                                  void (*syncCallback)(ClIocPhysicalAddressT *srcAddr, ClPtrT syncArg), 
                                  ClPtrT syncArg, ClCharT *xportType)
{
    ClIocHeaderT userHeader = {0};
    ClUint32T event = 0;
    ClUint8T *pRecvBase = recvBuff;
    ClIocPhysicalAddressT compAddr = {0};
    ClIocPhysicalAddressT srcAddr = {0};
    ClIocNotificationT notification = {0};
    ClIocNotificationIdT id = 0;
    ClRcT rc = CL_OK;
    ClUint8T protocolType;
    ClUint8T version;
    ClUint32T sizeHeader = 0;
#ifdef COMPAT_5
    ClTipcHeaderT userTipcHeader = {0};
    ClBoolT isBackward = CL_FALSE;

    if ((recvLen <= (ClUint32T)sizeof(userHeader)) && (recvLen <= (ClUint32T)sizeof(userTipcHeader)))
    {
        return CL_IOC_RC(CL_ERR_NO_SPACE);
    }

    sizeHeader = (ClUint32T)sizeof(userTipcHeader);
    memcpy((ClPtrT)&userTipcHeader, pRecvBase, sizeHeader);

    /*
     * Issue: 1 SC running with 5.0 and another running 6.0, then IOC header have a different.
     * As result, 2 above SCs can not communication
     * Work-around to mark new version (since 6.0) by marked .reserved field to 0x2 because
     * old version (5.0) .reserved field = 0x0, 0x1 => CL_IOC_COMPRESSION
     * But it is not right if old one (5.0) defined CL_IOC_COMPRESSION, then it should be patched
     * for 5.0 as well since if defined compression flag, this solution may not work
     */
    isBackward = (!(ntohl(userTipcHeader.reserved) & 0x2));

    if (isBackward)
      {
        protocolType = userTipcHeader.protocolType;
        version = userTipcHeader.version;

        srcAddr.nodeAddress = ntohl(userTipcHeader.srcAddress.iocPhyAddress.nodeAddress);
        srcAddr.portId = ntohl(userTipcHeader.srcAddress.iocPhyAddress.portId);
        // Backward compatible
        if (srcAddr.nodeAddress == gIocLocalBladeAddress)
          {
            goto out;
          }
        else
          {
            //0x2 mean this is release 5.0 (userHeader.reserved = 0)
            clIocSetNodeCompat(srcAddr.nodeAddress, 0x2);
          }
      }
    else
      {
#else
        if (recvLen <= (ClUint32T)sizeof(userHeader))
        {
            return CL_IOC_RC(CL_ERR_NO_SPACE);
        }
#endif
        sizeHeader = (ClUint32T)sizeof(userHeader);
        memcpy((ClPtrT)&userHeader, pRecvBase, sizeHeader);
        protocolType = userHeader.protocolType;
        version = userHeader.version;

        srcAddr.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
        srcAddr.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);
#ifdef COMPAT_5
        if (srcAddr.nodeAddress != gIocLocalBladeAddress)
          clIocSetNodeCompat(srcAddr.nodeAddress, 0x1); //0x1 mean this is release 6.0 (userHeader.reserved = 0x2)
      }
#endif

    if (version != CL_IOC_HEADER_VERSION)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Got version [%d] tipc packet. Supported [%d] version\n", version, CL_IOC_HEADER_VERSION));
        return CL_IOC_RC(CL_ERR_VERSION_MISMATCH);
    }

    if(protocolType == CL_IOC_PROTO_ARP)
    {
        /*
         * Ignore self notification.
         */
        if(srcAddr.nodeAddress == gIocLocalBladeAddress)
        {
            return CL_OK;
        }
        recvLen -= sizeHeader;
        if(recvLen < sizeof(ClIocNotificationT))
        {
            clLogError("PROXY", "RECV", "Invalid proxy notification packet received "
                       "of size [%d]. Expected [%d] bytes", 
                       recvLen, (ClUint32T)sizeof(ClIocNotificationT));
            return CL_ERR_NO_SPACE;
        }
        return clIocNotificationProxyRecv(commPort, pRecvBase + sizeHeader,
                                          allLocalComps, allNodeReps, xportType);
    }

    /*
     * Enable the destination status bit when a notification packet is received from a peer node.
     */
    clIocCompStatusSet(srcAddr, CL_IOC_NODE_UP);

    /*
     * Got heartbeat reply from other nodes/local components
     */
    if (protocolType == CL_IOC_PROTO_ICMP)
    {
        /*
         * Expecting "EXIT" or "HELLO" heartbeat message
         */
        ClCharT message[6] = {0};
        memcpy(message, pRecvBase + sizeHeader, 6);
        clIocHearBeatHealthCheckUpdate(srcAddr.nodeAddress, srcAddr.portId, message);
        return CL_OK;
    }

    /*
     * Got heartbeat request from a node
     */
    if (protocolType == CL_IOC_PROTO_HB) {
        /*
         * Reply HeartBeat message
         */
        ClIocAddressT destAddress = { { 0 } };
        destAddress.iocPhyAddress.nodeAddress = srcAddr.nodeAddress; 
        destAddress.iocPhyAddress.portId = CL_IOC_XPORT_PORT;

        if (destAddress.iocPhyAddress.nodeAddress == gIocLocalBladeAddress)
        {
            goto out;
        }
        clIocHeartBeatMessageReqRep(commPort, &destAddress, CL_IOC_PROTO_ICMP, CL_FALSE);
        return CL_OK;
    }

    if(protocolType == CL_IOC_PROTO_CTL)
    {
        clIocNodeCompsSet(srcAddr.nodeAddress, pRecvBase + sizeHeader);
        return CL_OK;
    }

    memcpy(&notification, pRecvBase + sizeHeader, sizeof(notification));
    if(ntohl(notification.protoVersion) != CL_IOC_NOTIFICATION_VERSION)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                       ("Got version [%d] notification packet. Supported [%d] version\n",
                        ntohl(notification.protoVersion), 
                        CL_IOC_NOTIFICATION_VERSION));
        return CL_IOC_RC(CL_ERR_VERSION_MISMATCH);
    }

    id = ntohl(notification.id);
    if(id == CL_IOC_NODE_DISCOVER_NOTIFICATION)
    {
        if(syncCallback)
            syncCallback(&srcAddr, syncArg);
        return CL_OK;
    }

    /*
     * Get the version of the peer node and update the shared memory.
     */
    if(id == CL_IOC_NODE_VERSION_NOTIFICATION ||
       id == CL_IOC_NODE_VERSION_REPLY_NOTIFICATION ||
       id == CL_IOC_NODE_LINK_UP_NOTIFICATION)
    {
        if(srcAddr.nodeAddress == gIocLocalBladeAddress)
            goto out;
        
        rc = clIocNotificationDiscoveryUnpack(pRecvBase + sizeHeader,
                                              recvLen - sizeHeader, &srcAddr,
                                              &notification, commPort, xportType);

        if(id == CL_IOC_NODE_LINK_UP_NOTIFICATION
           &&
           rc == CL_OK)
            goto out_notify;

        goto out;
    }

    compAddr.nodeAddress = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
    compAddr.portId = ntohl(notification.nodeAddress.iocPhyAddress.portId);

    if(id == CL_IOC_COMP_ARRIVAL_NOTIFICATION)
    {
        event = CL_IOC_NODE_UP;
    }
    else
    {
        event = CL_IOC_NODE_DOWN;
        clIocMasterSegmentUpdate(compAddr);
    }

    clIocCompStatusSet(compAddr, event);

    clLogInfo ("IOC", "NOTIF", "Got [%s] notification [0x%x] for node [%d] commport [0x%x]",
               event == CL_IOC_NODE_UP ? "arrival": "death", id, compAddr.nodeAddress, compAddr.portId);

#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE
    if(event == CL_IOC_NODE_UP)
        goto out;
#endif

    /* Need to send the above notification to all the components on this node */
    out_notify:
    rc = clIocNotificationPacketSend(commPort, &notification, allLocalComps, CL_FALSE, xportType);

    if(clTransportBridgeEnabled(gIocLocalBladeAddress) && allNodeReps && srcAddr.nodeAddress != gIocLocalBladeAddress)
    {
        clIocNotificationProxySend(commPort, &notification, &srcAddr, allNodeReps, xportType);
    }

    if ((compAddr.portId == 0) && (event == CL_IOC_NODE_DOWN) && (compAddr.nodeAddress == gIocLocalBladeAddress))
    {
        ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0 };
        clLogCritical ("IOC", "NOT", "Controller has kicked this node out of the cluster -- quitting in 1 second.");
        clOsalTaskDelay(delay);
        exit(0);
    }

    out:
    return rc;
}

/*
 * Inform to all nodes about node leaving
 */
ClRcT clIocNotificationNodeLeave(ClIocCommPortHandleT commPort, ClIocNodeAddressT nodeAddr)
{
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = CL_IOC_XPORT_PORT;
    static ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
    ClUint32T myCapability = 0;
    ClIocNotificationT notification;
    notification.id = htonl(CL_IOC_NODE_LEAVE_NOTIFICATION);
    notification.nodeVersion = htonl(nodeVersion);
    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(nodeAddr);
    notification.nodeAddress.iocPhyAddress.portId = htonl(myCapability);
    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    return clIocNotificationPacketSend(commPort, &notification, &allNodeReps, CL_FALSE, NULL);
}

ClRcT clIocNotificationInitialize(void)
{
    ClRcT rc = CL_OK;
    if(gIocNotificationInitialized)
    {
        return rc;
    }
    rc = clOsalMutexInit(&gIocNotificationRegisterLock);
    if(rc != CL_OK)
        return rc;
    gIocNotificationInitialized = CL_TRUE;
    return rc;
}

ClRcT clIocNotificationFinalize(void)
{
    return CL_OK;
}

