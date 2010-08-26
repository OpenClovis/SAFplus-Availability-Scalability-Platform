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

static ClRcT clTipcNotificationPacketSend(ClIocNotificationT *pNotificationInfo, ClIocAddressT *destAddress)
{
    ClRcT retCode;
    ClBufferHandleT message;

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

    clOsalMutexLock(&gIocEventHandlerSendLock);
    if(handlerFd[1] != -1)
        retCode = clIocSend((ClIocCommPortHandleT)&dummyCommPort, message, CL_IOC_PORT_NOTIFICATION_PROTO, destAddress, &sendOption);
    clOsalMutexUnlock(&gIocEventHandlerSendLock);
    if(retCode != CL_OK)
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to send notification. error code 0x%x", retCode));

err_out:
    {
        ClRcT rc = clBufferDelete(&message);
        CL_ASSERT(rc == CL_OK);
    }

out:
    return retCode;
}

static ClRcT clTipcReceivedPacket(ClUint32T socketType, struct msghdr *pMsgHdr)
{
    ClRcT rc = CL_OK;
    ClIocNotificationT notification = {0};
    ClIocPhysicalAddressT compAddr={0};
    ClUint8T buff[CL_TIPC_BYTES_FOR_COMPS_PER_NODE];
    ClUint32T nodeVersion = CL_VERSION_CODE(CL_RELEASE_VERSION, 1, CL_MINOR_VERSION);

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

                CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Got [%d] notification for node [0x%x].", event.event, compAddr.nodeAddress)); 

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
                        notification.id = htonl(CL_IOC_NODE_VERSION_NOTIFICATION);
                        notification.protoVersion = htonl(CL_IOC_NOTIFICATION_VERSION);
                        notification.nodeVersion = htonl(nodeVersion);
                        notification.nodeAddress.iocPhyAddress.portId = 0;
                        notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
                        clTipcNotificationPacketSend(&notification, &allNodeReps);
                    }
                    return CL_OK;
                }

                clIocCompStatusSet(compAddr, event.event);

                if(event.event == TIPC_PUBLISHED) {
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
                    clNodeCacheReset(compAddr.nodeAddress);
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
                clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allLocalComps);

            } else {
                /* This is for LOCAL COMPONENT ARRIVAL/DEPARTURE */

                compAddr.nodeAddress = gIocLocalBladeAddress;
                compAddr.portId = event.found_lower - CL_TIPC_BASE_TYPE;

                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Trace : Got %d notification for node [0x%x] commport [0x%x]",
                                                event.event, compAddr.nodeAddress, compAddr.portId)); 

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
                clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allNodeReps);
            }         
        }
        break;
    case 1:
        {
            /* Packet is received from the other node asp_amfs/NODE-REPS*/
            ClTipcHeaderT userHeader = {0};
            ClUint32T event = 0;
                
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

            if(userHeader.protocolType == CL_IOC_PROTO_CTL) {
                clIocNodeCompsSet(ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress), pMsgHdr->msg_iov->iov_base + sizeof(userHeader));
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
                destAddress.iocPhyAddress.nodeAddress = ntohl(userHeader.srcAddress.iocPhyAddress.nodeAddress);
                destAddress.iocPhyAddress.portId = ntohl(userHeader.srcAddress.iocPhyAddress.portId);
                
                if(destAddress.iocPhyAddress.nodeAddress == gIocLocalBladeAddress)
                {
                    /*
                     * Skip self updates.
                     */
                    break;
                }
             
                clNodeCacheUpdate(nodeId, version);
                
                /*
                 * Send back node reply to peer for version notifications. with our info.
                 */
                if(ntohl(notification.id) == CL_IOC_NODE_VERSION_NOTIFICATION)
                {
                    notification.id = htonl(CL_IOC_NODE_VERSION_REPLY_NOTIFICATION);
                    notification.nodeVersion = htonl(nodeVersion);
                    notification.nodeAddress.iocPhyAddress.nodeAddress = htonl(gIocLocalBladeAddress);
                    clTipcNotificationPacketSend(&notification, &destAddress);
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

            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Trace : Got %d notification for node [0x%x] commport [0x%x]",
                                            event,compAddr.nodeAddress, compAddr.portId)); 

#ifdef CL_IOC_COMP_ARRIVAL_NOTIFICATION_DISABLE
            if(event == TIPC_PUBLISHED)
                break;
#endif
                
            /* Need to send the above notification to all the components on this node */
            clTipcNotificationPacketSend(&notification, (ClIocAddressT *)&allLocalComps);
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
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : recvmsg failed. errno=%d\n",errno));
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
    ClInt32T rc;
    struct sockaddr_tipc topsrv;
    
    sd = socket (AF_TIPC, SOCK_SEQPACKET, 0);
    if(sd < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : socket() failed. system error [%s].\n", strerror(errno)));
        CL_ASSERT(0);
        return CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }

    rc = fcntl(sd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(rc == 0);

    memset(&topsrv,0,sizeof(topsrv));
    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;

    /* Connect to topology server: */
    rc = connect(sd,(struct sockaddr*)&topsrv,sizeof(topsrv));
    CL_ASSERT(rc == 0);
    
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
    CL_ASSERT(rc >= 0);
    
    return CL_OK;
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

    /* Creating a socket for handling the subscription events */
    handlerFd[0] = clTipcSubscriptionSocketCreate();


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
    
    retCode = clOsalTaskCreateDetached(pTaskName, CL_OSAL_SCHED_OTHER,  CL_OSAL_THREAD_PRI_NOT_APPLICABLE, 0, (void* (*)(void*))&clTipcEventHandler, NULL);
    if(retCode != CL_OK) {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : Event Handle thread did not start. error code 0x%x",retCode));
        goto out;
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
