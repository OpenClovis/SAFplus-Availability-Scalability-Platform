/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <clCommon.h>

#if OS_VERSION_CODE < OS_KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

#include <clOsalApi.h>
#include <clBufferApi.h>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clList.h>
#include <clIocApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocServices.h>
#include <clIocIpi.h>
#include <clTipcNeighComps.h>
#include <clTipcNotification.h>
#include <clTipcUserApi.h>
#include <clTipcSetup.h>
#include <clTipcMaster.h>
#include <clNodeCache.h>
#include <clCpmApi.h>

#define CL_TIPC_HANDLER_MAX_SOCKETS            2

static struct {
    ClOsalMutexT lock;
    ClOsalCondT condVar;
} gIocEventHandlerClose;

typedef struct ClIocNotificationRegister
{
    ClListHeadT list;
    ClIocNotificationRegisterCallbackT callback;
    ClPtrT cookie;
}ClIocNotificationRegisterT;

static CL_LIST_HEAD_DECLARE(gIocNotificationRegisterList);

typedef ClIocLogicalAddressT ClIocLocalCompsAddressT;

static ClCharT pTaskName[] = {"clTipcNoficationHandler"};
static ClUint32T numHandlers = CL_TIPC_HANDLER_MAX_SOCKETS;
static ClInt32T handlerFd[CL_TIPC_HANDLER_MAX_SOCKETS];
static ClOsalMutexT gIocEventHandlerSendLock;

static ClTipcCommPortT dummyCommPort;
static ClIocLocalCompsAddressT allLocalComps;
static ClIocAddressT allNodeReps = { .iocPhyAddress = {CL_IOC_BROADCAST_ADDRESS, CL_IOC_TIPC_PORT}};
static ClIocSendOptionT sendOption = { .priority = CL_IOC_HIGH_PRIORITY, .timeout = 2000 };
static ClUint32T threadContFlag = 1;

static ClCharT eventHandlerInited = 0;
static ClOsalMutexT gIocNotificationRegisterLock;

static ClRcT tipcEventSubscribe(ClBoolT pollThread);

ClRcT clIocDoesNodeAlreadyExist(ClInt32T *pSd);

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

static ClRcT clIocNotificationRegistrants(ClIocNotificationT *notification)
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

static ClRcT clTipcNotificationPacketSend(ClIocNotificationT *pNotificationInfo, 
                                          ClIocAddressT *destAddress, ClBoolT compat)
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
    if(id == CL_IOC_NODE_VERSION_NOTIFICATION
       ||
       id == CL_IOC_NODE_VERSION_REPLY_NOTIFICATION)
    {
        if(!compat)
        {
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
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Nodename marshall for notification version send failed with [%#x]\n", 
                                                retCode));
                goto err_out;
            }
        }
    }

    clOsalMutexLock(&gIocEventHandlerSendLock);
    if(handlerFd[1] != -1)
        retCode = clIocSend((ClIocCommPortHandleT)&dummyCommPort, message, CL_IOC_PORT_NOTIFICATION_PROTO, destAddress, &sendOption);
    clOsalMutexUnlock(&gIocEventHandlerSendLock);
    if(retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send notification. error code 0x%x", retCode));

    err_out:
    clBufferDelete(&message);

    out:
    return retCode;
}

static ClRcT clTipcNodeVersionSend(ClIocAddressT *destAddress, 
                                   ClUint32T nodeVersion, ClUint32T myCapability)
{
    ClIocNotificationT notification = {0};
    notification.id = htonl(CL_IOC_NODE_VERSION_NOTIFICATION);
    notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
    notification.nodeVersion = htonl(nodeVersion);
    notification.nodeAddress.iocPhyAddress.portId = htonl(myCapability);
    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
    clLogNotice("NODE", "VERSION", "Sending node version [%#x], capability [%#x] "
                "to node [%#x], port [%#x]", nodeVersion, myCapability, 
                destAddress->iocPhyAddress.nodeAddress, destAddress->iocPhyAddress.portId);
    return clTipcNotificationPacketSend(&notification, destAddress, CL_FALSE);
}

static ClRcT clTipcReceivedPacket(ClUint32T socketType, struct msghdr *pMsgHdr)
{
    ClRcT rc = CL_OK;
    ClIocNotificationT notification = {0};
    ClIocPhysicalAddressT compAddr={0};
    ClUint8T buff[CL_TIPC_BYTES_FOR_COMPS_PER_NODE];
    ClUint32T nodeVersion = CL_VERSION_CODE(5, 0, 0);
    ClUint32T myCapability = 0;

    clNodeCacheVersionAndCapabilityGet(gIocLocalBladeAddress, &nodeVersion, &myCapability);

    switch(socketType)
    {
    case 0:
        {
            /* Packet is got from the topology service */
            struct tipc_event event;

#ifdef SOLARIS_BUILD
            bcopy(pMsgHdr->msg_iov->iov_base,(ClPtrT)&event, sizeof(event));
#else
            memcpy((ClPtrT)&event, pMsgHdr->msg_iov->iov_base, sizeof(event));
#endif

            if(event.s.seq.type - CL_TIPC_BASE_TYPE == CL_IOC_TIPC_PORT) {
                /* This is for NODE ARRIVAL/DEPARTURE */

                compAddr.nodeAddress = event.found_lower;
                compAddr.portId = CL_IOC_TIPC_PORT;

                clLogInfo("TIPC", "NOTIF", "Got node [%s] notification for node [0x%x]", 
                          event.event == TIPC_PUBLISHED ? "arrival" : "death", compAddr.nodeAddress);

                if(compAddr.nodeAddress == gIocLocalBladeAddress)
                {
                    if(event.event == TIPC_WITHDRAWN) {
                        clNodeCacheReset(gIocLocalBladeAddress);
                        threadContFlag = 0;
                    } else {
                        /*
                         * Send the node version to all node reps.
                         */
                        clIocCompStatusSet(compAddr, event.event);
                        clTipcNodeVersionSend(&allNodeReps, nodeVersion, myCapability);
                    }
                    return CL_OK;
                }

                clIocCompStatusSet(compAddr, event.event);

                if(event.event == TIPC_PUBLISHED) {
                    /* 
                     * Received NODE ARRIVAL notification.
                     * Send back node version again for consistency or link syncup point
                     * and comp bitmap for this node
                     */
                    clTipcNodeVersionSend((ClIocAddressT*)&compAddr,
                                          nodeVersion, myCapability);

                    /* Received NODE ARRIVAL notification. */
                    ClBufferHandleT message;
                    ClRcT retCode = clBufferCreate(&message);
                    CL_ASSERT(retCode == CL_OK);
                    clIocNodeCompsGet(gIocLocalBladeAddress, buff);
                    rc = clBufferNBytesWrite(message, buff, sizeof(buff));
                    if(rc != CL_OK) {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                       ("\nERROR: clBufferNBytesWrite failed with rc = %x\n", rc));
                        goto err_out;
                    }

                    clOsalMutexLock(&gIocEventHandlerSendLock);
                    if(handlerFd[1] != -1)
                        rc = clIocSend((ClIocCommPortHandleT)&dummyCommPort, message, CL_IOC_PROTO_CTL, (ClIocAddressT*)&compAddr, &sendOption);
                    clOsalMutexUnlock(&gIocEventHandlerSendLock);

                    if(rc != CL_OK)
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send notification. error code 0x%x", rc));

                    retCode = clBufferDelete(&message);
                    CL_ASSERT(retCode == CL_OK);
                    err_out:
#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE 
                    return CL_OK;
#else
                    notification.id = htonl(CL_IOC_NODE_ARRIVAL_NOTIFICATION);
#endif
                } else {
                    /* Recieved Node LEAVE notification. */
                    clTipcMasterSegmentUpdate(compAddr);
                    clIocNodeCompsReset(compAddr.nodeAddress);
                    clNodeCacheSoftReset(compAddr.nodeAddress);
                    notification.id = htonl(CL_IOC_NODE_LEAVE_NOTIFICATION);
                }
                notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
                notification.nodeAddress.iocPhyAddress.portId = 0;
                notification.nodeAddress.iocPhyAddress.nodeAddress =  htonl(compAddr.nodeAddress);

                /*
                 * Notify the registrants for this notification who might want to do a fast
                 * pass early than relying on the slightly slower notification proto callback.
                 */
                clIocNotificationRegistrants(&notification);
                /* Need to send a notification packet to all the components on this node */
                clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allLocalComps, CL_FALSE);

            } else {
                /* This is for LOCAL COMPONENT ARRIVAL/DEPARTURE */

                compAddr.nodeAddress = gIocLocalBladeAddress;
                compAddr.portId = event.found_lower - CL_TIPC_BASE_TYPE;

                clLogInfo("TIPC", "NOTIF", "Got component [%s] notification for node [0x%x] commport [0x%x]",
                          event.event == TIPC_WITHDRAWN ? "death" : "arrival", compAddr.nodeAddress, compAddr.portId); 

                clIocCompStatusSet(compAddr, event.event);

                if(event.event == TIPC_WITHDRAWN)
                {
                    if(compAddr.portId == CL_IOC_CPM_PORT)
                    {
                        /*
                         * self shutdown.
                         */
                        threadContFlag = 0;
                        return CL_OK;
                    }
                    clTipcMasterSegmentUpdate(compAddr);
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
                /* Need to send a notification packet to all the asp_amfs's event handlers. */
                clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allNodeReps, CL_FALSE);
            }         
        }
        break;
    case 1:
        {
            /* Packet is received from the other node asp_amfs/NODE-REPS*/
            ClTipcHeaderT userHeader = {0};
            ClUint32T event = 0;
            ClIocPhysicalAddressT srcAddr = {0};

#ifdef SOLARIS_BUILD
            bcopy(pMsgHdr->msg_iov->iov_base,(ClPtrT)&userHeader,sizeof(userHeader));
#else
            memcpy((ClPtrT)&userHeader,pMsgHdr->msg_iov->iov_base,sizeof(userHeader));
#endif
            
            if(userHeader.version != CL_IOC_HEADER_VERSION)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Got version [%d] tipc packet. Supported [%d] version\n",
                                                userHeader.version, CL_IOC_HEADER_VERSION));
                return CL_IOC_RC(CL_ERR_VERSION_MISMATCH);
            }

            srcAddr.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
            srcAddr.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);
            
            clIocCompStatusSet(srcAddr, TIPC_PUBLISHED);

            if(userHeader.protocolType == CL_IOC_PROTO_CTL) {
                clIocNodeCompsSet(srcAddr.nodeAddress, pMsgHdr->msg_iov->iov_base + sizeof(userHeader));
                break;
            }

            memcpy((ClPtrT)&notification, pMsgHdr->msg_iov->iov_base + sizeof(userHeader), sizeof(notification));
            if(ntohl(notification.protoVersion) != CL_IOC_NOTIFICATION_VERSION)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Got version [%d] notification packet. Supported [%d] version\n",
                                                ntohl(notification.protoVersion), 
                                                CL_IOC_NOTIFICATION_VERSION));
                return CL_IOC_RC(CL_ERR_VERSION_MISMATCH);
            }
                
            /*
             * Get the version of the peer node and update the shared memory.
             */
            if(ntohl(notification.id) == CL_IOC_NODE_VERSION_NOTIFICATION ||
               ntohl(notification.id) == CL_IOC_NODE_VERSION_REPLY_NOTIFICATION)
            {
                ClIocAddressT destAddress = {{0}};
                ClUint32T version = ntohl(notification.nodeVersion);
                ClIocNodeAddressT nodeId = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
                ClUint32T theirCapability = ntohl(notification.nodeAddress.iocPhyAddress.portId);
                ClNameT nodeName = {0};
                ClUint8T *nodeInfo = NULL;
                ClUint32T nodeInfoLen = 0;
                ClBoolT compat = CL_FALSE;
                destAddress.iocPhyAddress.nodeAddress = srcAddr.nodeAddress;
                destAddress.iocPhyAddress.portId = srcAddr.portId;
                
                if(destAddress.iocPhyAddress.nodeAddress == gIocLocalBladeAddress)
                {
                    /*
                     * Skip self updates.
                     */
                    break;
                }
                /*
                 * Check for backward compatibility
                 */
                if(version >= CL_VERSION_CODE(5, 0, 0))
                {
                
                    nodeInfo = (ClUint8T*)pMsgHdr->msg_iov->iov_base + sizeof(userHeader) + sizeof(notification);
                    nodeInfoLen = pMsgHdr->msg_iov->iov_len - sizeof(userHeader) - sizeof(notification);
                    if(nodeInfoLen < sizeof(nodeName.length))
                    {
                        clLogError("NOTIF", "GET", "Invalid notification data received with available "
                                   "data length of [%d] bytes", nodeInfoLen);
                        break;
                    }
                    nodeInfoLen -= sizeof(nodeName.length);
                    memcpy(&nodeName.length, nodeInfo, sizeof(nodeName.length));
                    nodeName.length = ntohs(nodeName.length);
                    nodeInfo += sizeof(nodeName.length);
                    if(nodeInfoLen < nodeName.length)
                    {
                        clLogError("NOTIF", "GET", "Invalid notification data received for node version notification. "
                                   "Node length received [%d] with only [%d] bytes of available input data",
                                   nodeName.length, nodeInfoLen);
                        break;
                    }
                    memcpy(nodeName.value, nodeInfo, CL_MIN(sizeof(nodeName.value)-1, nodeName.length));
                    nodeName.value[CL_MIN(sizeof(nodeName.value)-1, nodeName.length)] = 0;
                }
                else
                {
                    compat = CL_TRUE;
                }

                clNodeCacheUpdate(nodeId, version, theirCapability, &nodeName);
                
                /*
                 * Send back node reply to peer for version notifications. with our info.
                 */
                if(ntohl(notification.id) == CL_IOC_NODE_VERSION_NOTIFICATION)
                {
                    notification.id = htonl(CL_IOC_NODE_VERSION_REPLY_NOTIFICATION);
                    notification.nodeVersion = htonl(nodeVersion);
                    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
                    notification.nodeAddress.iocPhyAddress.portId = htonl(myCapability);
                    clTipcNotificationPacketSend(&notification, &destAddress, compat);
                }
                break;
            }

            compAddr.nodeAddress = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
            compAddr.portId = ntohl(notification.nodeAddress.iocPhyAddress.portId);

            if(ntohl(notification.id) == CL_IOC_COMP_ARRIVAL_NOTIFICATION)
                event = TIPC_PUBLISHED;
            else
            {
                event = TIPC_WITHDRAWN;
                clTipcMasterSegmentUpdate(compAddr);
            }
            
            clIocCompStatusSet(compAddr, event);

            clLogInfo ("TIPC", "NOTIF", "Got [%s] notification for node [0x%x] commport [0x%x]",
                       event == TIPC_PUBLISHED ? "arrival":"death",compAddr.nodeAddress, compAddr.portId); 

#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE
            if(event == TIPC_PUBLISHED)
                break;
#endif
                
            /* Need to send the above notification to all the components on this node */
            clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allLocalComps, CL_FALSE);
        }
        break;
    default :
        break;
    }
    return CL_OK;
}



static ClUint8T buffer[0xffff+1];
static void clTipcEventHandler(ClPtrT pArg)
{
    ClUint32T i=0;
    ClInt32T fd[CL_TIPC_HANDLER_MAX_SOCKETS];
    struct pollfd pollfds[CL_TIPC_HANDLER_MAX_SOCKETS];
    struct msghdr msgHdr;
    struct iovec ioVector[1];
    struct sockaddr_tipc peerAddress;
    ClInt32T pollStatus;
    ClInt32T bytes;
    ClUint32T timeout = CL_IOC_TIMEOUT_FOREVER;
    ClUint32T recvErrors = 0;

    retry:
    bzero((char*)pollfds,sizeof(pollfds));
    pollfds[0].fd = fd[0] = handlerFd[0];
    pollfds[1].fd = fd[1] = handlerFd[1];
    pollfds[0].events = pollfds[1].events = POLLIN|POLLRDNORM;


    bzero((char*)&msgHdr,sizeof(msgHdr));
    bzero((char*)ioVector,sizeof(ioVector));
    msgHdr.msg_name = (struct sockaddr_tipc*)&peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector[0].iov_base = (ClPtrT)buffer;
    ioVector[0].iov_len = sizeof(buffer);
    msgHdr.msg_iov = ioVector;
    msgHdr.msg_iovlen = sizeof(ioVector)/sizeof(ioVector[0]);

    while(threadContFlag) {
        pollStatus = poll(pollfds, numHandlers, timeout);
        if(pollStatus > 0) {
            for(i = 0 ; i < numHandlers; i++)
            {
                if((pollfds[i].revents & (POLLIN|POLLRDNORM)))
                {
                    recv:
                    bytes = recvmsg(fd[i],&msgHdr,0);
                    if(bytes < 0 )
                    {
                        if(errno == EINTR)
                            goto recv;

                        if( !(recvErrors++ & 255) )
                        {
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                                           ("Recvmsg failed with [%s]\n", strerror(errno)));
                            sleep(1);
                        }

                        if(errno == ENOTCONN)
                        {
                            if(tipcEventSubscribe(CL_FALSE) != CL_OK)
                            {
                                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("TIPC topology subsciption retry failed. "
                                                                   "Shutting down the notification thread and process\n"));
                                threadContFlag = 0;
                                exit(0); 
                                continue; /*unreached*/
                            }
                            goto retry;
                        }
                        continue;
                    }
                    clTipcReceivedPacket(i, &msgHdr);
                }else if((pollfds[i].revents & (POLLHUP|POLLERR|POLLNVAL))) {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Handler \"poll\" hangup.\n"));
                }
            }
        } else if(pollStatus < 0 ) {
            if (errno != EINTR)  /* If the system call is interrupted just loop, its not an error */
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : poll failed. errno=%d\n",errno));
        }
    }
    close(handlerFd[0]);

    clOsalMutexLock(&gIocEventHandlerClose.lock);
    clOsalCondSignal(&gIocEventHandlerClose.condVar);
    clOsalMutexUnlock(&gIocEventHandlerClose.lock);
}


static ClInt32T clTipcSubscriptionSocketCreate(void)
{
    ClInt32T sd;
    struct sockaddr_tipc topsrv;
    
    sd = socket (AF_TIPC, SOCK_SEQPACKET, 0);
    if(sd < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : socket() failed. system error [%s].\n", strerror(errno)));
        return -1;
    }

    if(fcntl(sd, F_SETFD, FD_CLOEXEC))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error: fcntl on tipc topology socket failed with [%s]\n", 
                                        strerror(errno)));
        close(sd);
        return -1;
    }

    memset(&topsrv,0,sizeof(topsrv));
    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;

    /* Connect to topology server: */
    if(connect(sd,(struct sockaddr*)&topsrv,sizeof(topsrv)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Socket connect for tipc topology socket failed with [%s]\n",
                                        strerror(errno)));
        close(sd);
        return -1;
    }
    
    return sd;
}

static ClRcT clIocSubscribe(ClInt32T fd, ClUint32T portId, 
                            ClUint32T lowerInstance, ClUint32T upperInstance, ClUint32T timeout)
{
    struct tipc_subscr subscr = {{0}};
    ClInt32T rc;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Trace : port 0x%x, lower 0x%x, upper 0x%x, timeout %d, fd=%d",
                portId, lowerInstance, upperInstance, timeout, fd));

    subscr.seq.type  = portId;
    subscr.seq.lower = lowerInstance;
    subscr.seq.upper = upperInstance;
    subscr.timeout   = timeout;
    subscr.filter    = TIPC_SUB_SERVICE;

    rc = send(fd, (const char *)&subscr, sizeof(subscr), 0);
    if(rc < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("send to tipc topology socket failed with [%s]\n", 
                                        strerror(errno)));
        return CL_IOC_RC(CL_ERR_LIBRARY);
    }
    
    return CL_OK;
}

static ClRcT tipcEventSubscribe(ClBoolT pollThread)
{
    ClRcT retCode = CL_OK;

    /* Creating a socket for handling the subscription events */
    handlerFd[0] = clTipcSubscriptionSocketCreate();
    if(handlerFd[0] < 0)
        return CL_IOC_RC(CL_ERR_LIBRARY);

    if(pollThread)
    {
        retCode = clOsalTaskCreateDetached(pTaskName, CL_OSAL_SCHED_OTHER,  CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, (void* (*)(void*))&clTipcEventHandler, NULL);
        if(retCode != CL_OK) {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Event Handle thread did not start. error code 0x%x",retCode));
            goto out;
        }
    }

    /* SUBSCRIPTION : For getting the node arrival or departure events. */
    clIocSubscribe( handlerFd[0],
                    CL_TIPC_SET_TYPE(CL_IOC_TIPC_PORT),
                    CL_IOC_MIN_NODE_ADDRESS,
                    CL_IOC_MAX_NODE_ADDRESS,
                    CL_IOC_TIMEOUT_FOREVER);

    /* SUBSCRIPTION : For getting the intranode component arrival or departure events. */
    clIocSubscribe( handlerFd[0],
                    CL_IOC_TIPC_ADDRESS_TYPE_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress),
                    CL_TIPC_SET_TYPE(CL_IOC_MIN_COMP_PORT),
                    CL_TIPC_SET_TYPE(CL_IOC_MAX_COMP_PORT),
                    CL_IOC_TIMEOUT_FOREVER);

    out:
    return retCode;
}

ClRcT clTipcEventHandlerInitialize(void)
{
    ClRcT retCode = CL_OK;
    ClRcT rc;

    allLocalComps = CL_IOC_ADDRESS_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress, CL_IOC_BROADCAST_ADDRESS);

    rc = clOsalMutexInit(&gIocNotificationRegisterLock);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gIocEventHandlerSendLock);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gIocEventHandlerClose.lock);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalCondInit(&gIocEventHandlerClose.condVar);
    CL_ASSERT(rc == CL_OK);

    /* Creating a socket for handling the data packets sent by other node CPM/asp_amf. */
    retCode  = clIocDoesNodeAlreadyExist(&handlerFd[1]);
    if(retCode != CL_OK)
        goto out;

    dummyCommPort.fd = handlerFd[1];
#ifdef BCAST_SOCKET_NEEDED
    {
        ClInt32T ret;
        dummyCommPort.bcastFd = socket(AF_TIPC, SOCK_RDM, 0);
        CL_ASSERT(dummyCommPort.bcastFd >= 0);
        ret = fcntl(dummyCommPort.bcastFd, F_SETFD, FD_CLOEXEC);
        CL_ASSERT(ret == 0);
    }
#endif
    dummyCommPort.notify = CL_IOC_NOTIFICATION_ENABLE;
    dummyCommPort.priority = CL_IOC_TIPC_DEFAULT_PRIORITY;
    dummyCommPort.portId = CL_IOC_TIPC_PORT;

    retCode = tipcEventSubscribe(CL_TRUE);

    if(retCode != CL_OK)
        goto out;
    
    eventHandlerInited = 1;

    out:
    return retCode;
}

ClRcT clTipcEventHandlerFinalize(void)
{
    ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = CL_TIPC_MAIN_THREAD_WAIT_TIME};

    /* If service was never initialized, don't finalize */
    if(!eventHandlerInited)
      return CL_OK;
    eventHandlerInited = 0;

    /* Closing only the data socket so the the event socket can get the event of this and it will comeout of the recview thread */
    
    clOsalMutexLock(&gIocEventHandlerSendLock);
    close(handlerFd[1]);
    numHandlers = numHandlers - 1;
    handlerFd[1] = -1;
    clOsalMutexUnlock(&gIocEventHandlerSendLock);

    clOsalMutexLock(&gIocEventHandlerClose.lock);
    clOsalCondWait(&gIocEventHandlerClose.condVar, &gIocEventHandlerClose.lock, timeout);
    clOsalMutexUnlock(&gIocEventHandlerClose.lock);

    return CL_OK;
}
