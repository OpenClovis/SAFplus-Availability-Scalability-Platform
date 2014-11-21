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
#include <clIocErrors.h>
#include <clIocIpi.h>
#include "clTipcNotification.h"
#include <clIocUserApi.h>
#include "clTipcSetup.h"
#include <clTransport.h>
#include <clAmfPluginApi.h>
#include <clList.h>

#define CL_TIPC_HANDLER_MAX_SOCKETS            3

typedef ClIocLogicalAddressT ClIocLocalCompsAddressT;

static ClInt32T handlerFd[CL_TIPC_HANDLER_MAX_SOCKETS];
static ClOsalMutexT gIocEventHandlerSendLock;
static ClTransportListenerHandleT gNotificationListener;
static ClIocCommPortT dummyCommPort, discoveryCommPort;
static ClIocLocalCompsAddressT allLocalComps;
static ClIocAddressT allNodeReps = { .iocPhyAddress = {CL_IOC_BROADCAST_ADDRESS, CL_IOC_XPORT_PORT}};
static ClBoolT shutdownFlag = CL_FALSE;

static ClCharT eventHandlerInited = 0;
static ClRcT tipcEventHandler(ClInt32T fd, ClInt32T events, void *handlerIndex);

static ClUint32T gClTipcSubscrTimeout;

typedef struct ClTipcNodeStatus
{
    ClIocNodeAddressT nodeAddress;
    struct tipc_event event;
    ClListHeadT list;
} ClTipcNodeStatusT;

static CL_LIST_HEAD_DECLARE(gpTipcNodeStatusHandle);

static ClOsalMutexT gTipcNodeStatusMutex;

static void _clTipcNodeStatusAdd(ClTipcNodeStatusT *entry)
{
    clListAddTail(&entry->list, &gpTipcNodeStatusHandle);
}

static ClTipcNodeStatusT* _clTipcNodeStatusFind(ClIocNodeAddressT nodeAddress)
{
    register ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gpTipcNodeStatusHandle)
    {
        ClTipcNodeStatusT *entry = CL_LIST_ENTRY(iter, ClTipcNodeStatusT, list);
        if (entry->nodeAddress == nodeAddress)
        {
            return entry;
        }
    }
    return NULL;
}

static void _clTipcNodeStatusDelete(ClTipcNodeStatusT *entry)
{
    clListDel(&entry->list);
    clHeapFree(entry);
}

static ClRcT tipcSubscribe(ClInt32T fd, ClUint32T portId, 
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

static ClRcT tipcEventDeregister(ClInt32T skipIndex)
{
    ClInt32T i;
    for(i = 0; i < CL_TIPC_HANDLER_MAX_SOCKETS; ++i)
    {
        if(i != skipIndex && handlerFd[i] >= 0)
        {
            clTransportListenerDel(gNotificationListener, handlerFd[i]);
            handlerFd[i] = -1;
        }
    }
    return CL_OK;
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

static ClRcT tipcEventRegister(ClBoolT deregister)
{
    ClRcT rc = CL_OK;

    if(deregister)
    {
        tipcEventDeregister(-1);
    }

    /* Creating a socket for handling the subscription events */
    handlerFd[0] = clTipcSubscriptionSocketCreate();
    if(handlerFd[0] < 0)
        return CL_IOC_RC(CL_ERR_LIBRARY);

    /* SUBSCRIPTION : For getting the node arrival or departure events. */
    tipcSubscribe(handlerFd[0],
                  CL_TIPC_SET_TYPE(CL_IOC_DM_PORT),
                  CL_IOC_MIN_NODE_ADDRESS,
                  CL_IOC_MAX_NODE_ADDRESS,
                  CL_IOC_TIMEOUT_FOREVER);

    tipcSubscribe( handlerFd[0],
                   CL_TIPC_SET_TYPE(CL_IOC_XPORT_PORT),
                   CL_IOC_MIN_NODE_ADDRESS,
                   CL_IOC_MAX_NODE_ADDRESS,
                   CL_IOC_TIMEOUT_FOREVER);

    /* SUBSCRIPTION : For getting the intranode component arrival or departure events. */
    tipcSubscribe( handlerFd[0],
                   CL_IOC_TIPC_ADDRESS_TYPE_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress),
                   CL_TIPC_SET_TYPE(CL_IOC_MIN_COMP_PORT),
                   CL_TIPC_SET_TYPE(CL_IOC_MAX_COMP_PORT),
                   CL_IOC_TIMEOUT_FOREVER);

    rc = clIocCommPortCreateStatic(CL_IOC_DM_PORT, CL_IOC_RELIABLE_MESSAGING, 
                                   &discoveryCommPort, gClTipcXportType);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", "Comm port create for notification port [%#x] returned with [%#x]",
                   CL_IOC_DM_PORT, rc);
        goto out;
    }

    rc = clTipcFdGet(CL_IOC_DM_PORT, &handlerFd[1]);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", "TIPC notification fd for port [%#x] returned with [%#x]",
                   CL_IOC_DM_PORT, rc);
        goto out;
    }

    rc = clIocCommPortCreateStatic(CL_IOC_XPORT_PORT, CL_IOC_RELIABLE_MESSAGING, 
                                   &dummyCommPort, gClTipcXportType);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", "Comm port create for notification port [%#x] returned with [%#x]",
                   CL_IOC_XPORT_PORT, rc);
        goto out;
    }

    rc = clIocPortNotification(CL_IOC_XPORT_PORT, CL_IOC_NOTIFICATION_ENABLE);
    if(rc != CL_OK)
        goto out;

    rc = clTipcFdGet(CL_IOC_XPORT_PORT, &handlerFd[2]);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", "TIPC notification fd for port [%#x] returned with [%#x]",
                   CL_IOC_XPORT_PORT, rc);
        goto out;
    }

    rc = clTransportListenerAdd(gNotificationListener, 
                                handlerFd[0], tipcEventHandler, (void*)&handlerFd[0]);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", 
                   "Listener register for topology socket returned with [%#x]", rc);
        goto out;
    }

    rc = clTransportListenerAdd(gNotificationListener, 
                                handlerFd[1], tipcEventHandler, (void*)&handlerFd[1]);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT", 
                   "Listener register for discovery port returned with [%#x]", rc);
        goto out;
    }
    
    rc = clTransportListenerAdd(gNotificationListener, 
                                handlerFd[2], tipcEventHandler, (void*)&handlerFd[2]);
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "INIT",
                   "Listener register for notification port returned with [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

static ClRcT tipcSubscriptionTimeout(void *data)
{
    ClRcT rc = CL_OK;
    ClTipcNodeStatusT* nodeStatusEntry = (ClTipcNodeStatusT*) data;

    if (nodeStatusEntry)
    {
        if ((nodeStatusEntry->event.event == TIPC_SUBSCR_TIMEOUT)||(nodeStatusEntry->event.event == TIPC_WITHDRAWN))
        {
            rc = clIocNotificationNodeStatusSend((ClIocCommPortHandleT)&dummyCommPort,
                                             CL_IOC_NODE_LEAVE_NOTIFICATION,
                                             nodeStatusEntry->nodeAddress,
                                             (ClIocAddressT*)&allLocalComps,
                                             (ClIocAddressT*)&allNodeReps,
                                             gClTipcXportType);
        }

        clOsalMutexLock(&gTipcNodeStatusMutex);
        _clTipcNodeStatusDelete(nodeStatusEntry);
        clOsalMutexUnlock(&gTipcNodeStatusMutex);
    }

    return rc;
}

static ClRcT clTipcReceivedPacket(ClUint32T socketType, struct msghdr *pMsgHdr)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT compAddr={0};
    ClTimerHandleT timer = {0};
    ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = gClTipcSubscrTimeout};
    ClTipcNodeStatusT* nodeStatusEntry = NULL;

    switch(socketType)
    {
    case 0:
        {
            /* Packet is got from the topology service */
            struct tipc_event event;

            memcpy(&event, pMsgHdr->msg_iov->iov_base, sizeof(event));

            if(event.s.seq.type - CL_TIPC_BASE_TYPE == CL_IOC_DM_PORT)
            {
                /*
                 * Ignore the discovery for now.
                 */
                return rc;
            }

            if(event.s.seq.type - CL_TIPC_BASE_TYPE == CL_IOC_XPORT_PORT)
            {
                ClBoolT handled = CL_FALSE;
                
                /* This is for NODE ARRIVAL/DEPARTURE */

                compAddr.nodeAddress = event.found_lower;
                compAddr.portId = CL_IOC_XPORT_PORT;

                /*
                 * Calling AMF plugin to get status if it is defined
                 */
                if (clAmfPlugin && clAmfPlugin->clAmfNodeStatus)
                {
                    if ((event.event == TIPC_WITHDRAWN || event.event == TIPC_SUBSCR_TIMEOUT)
                       && (clAmfPlugin->clAmfNodeStatus(compAddr.nodeAddress) == ClNodeStatusUp))
                    {
                        /* DO NOT let the rest of the system see the failure */
                        return CL_OK;
                    }
                }

                clLogInfo("TIPC", "NOTIF", "Got node [%s] notification (%d) for node [%d]", event.event == TIPC_PUBLISHED ? "arrival" : "death", event.event, compAddr.nodeAddress);

                /*
                 * Wait a configurable # of seconds to see if the link is "healed"
                 * before letting the event "out" of clTipcNotification.c
  
                 * TIPC_WITHDRAWN happens when a link reset occurs so the delay is necessary on this event.
                 * TIPC_SUBSCR_TIMEOUT may only happen when we get a keepalive timeout so the delay may be optional for it.
                 */
                if ((event.event == TIPC_SUBSCR_TIMEOUT)||(event.event == TIPC_WITHDRAWN))
                {
                    clOsalMutexLock(&gTipcNodeStatusMutex);
                    nodeStatusEntry = _clTipcNodeStatusFind(compAddr.nodeAddress);
                    if (nodeStatusEntry != NULL)
                    {
                        nodeStatusEntry->event.event = event.event;
                    }
                    else
                    {
                        nodeStatusEntry = clHeapAllocate(sizeof(ClTipcNodeStatusT));
                        CL_ASSERT(nodeStatusEntry);

                        nodeStatusEntry->nodeAddress = compAddr.nodeAddress;
                        nodeStatusEntry->event.event = event.event;
                        _clTipcNodeStatusAdd(nodeStatusEntry);

                        rc = clTimerCreateAndStart(timeout, CL_TIMER_VOLATILE, CL_TIMER_SEPARATE_CONTEXT, tipcSubscriptionTimeout, nodeStatusEntry, &timer);
                        if(rc == CL_OK) handled = CL_TRUE;
                    }
                    clOsalMutexUnlock(&gTipcNodeStatusMutex);
                }

                if (!handled)
                {
                    clOsalMutexLock(&gTipcNodeStatusMutex);
                    nodeStatusEntry = _clTipcNodeStatusFind(compAddr.nodeAddress);
                    if (nodeStatusEntry != NULL)
                    {
                        nodeStatusEntry->event.event = event.event;
                    }
                    clOsalMutexUnlock(&gTipcNodeStatusMutex);

                    rc = clIocNotificationNodeStatusSend((ClIocCommPortHandleT)&dummyCommPort,
                                                         event.event == TIPC_PUBLISHED ?
                                                         CL_IOC_NODE_ARRIVAL_NOTIFICATION :
                                                         CL_IOC_NODE_LEAVE_NOTIFICATION,
                                                         compAddr.nodeAddress,
                                                         (ClIocAddressT*)&allLocalComps,
                                                         (ClIocAddressT*)&allNodeReps,
                                                         gClTipcXportType);

                    if(compAddr.nodeAddress == gIocLocalBladeAddress)
                    {
                        if(event.event == TIPC_WITHDRAWN)
                        {
                            shutdownFlag = CL_TRUE;
                        }
                    }
                }
            } 
            else 
            {
                /* This is for LOCAL COMPONENT ARRIVAL/DEPARTURE */

                compAddr.nodeAddress = gIocLocalBladeAddress;
                compAddr.portId = event.found_lower - CL_TIPC_BASE_TYPE;

                clLogInfo("TIPC", "NOTIF", "Got component [%s] notification for node [0x%x] commport [0x%x]",
                          event.event == TIPC_WITHDRAWN ? "death" : "arrival", compAddr.nodeAddress, compAddr.portId); 

                rc = clIocNotificationCompStatusSend((ClIocCommPortHandleT)&dummyCommPort,
                                                     event.event == TIPC_PUBLISHED ? CL_IOC_NODE_UP : CL_IOC_NODE_DOWN,
                                                     compAddr.portId, (ClIocAddressT*)&allLocalComps,
                                                     (ClIocAddressT*)&allNodeReps,
                                                     gClTipcXportType);

                if(event.event == TIPC_WITHDRAWN)
                {
                    if(compAddr.portId == CL_IOC_CPM_PORT)
                    {
                        /*
                         * self shutdown.
                         */
                        shutdownFlag = CL_TRUE;
                    }
                }
            }         
        }
        break;

    case 2:
        {
            /* Packet is received from the other node amfs/NODE-REPS*/
            rc = clIocNotificationPacketRecv((ClIocCommPortHandleT)&dummyCommPort,
                                             (ClUint8T*)pMsgHdr->msg_iov->iov_base, 
                                             (ClUint32T)pMsgHdr->msg_iov->iov_len,
                                             (ClIocAddressT*)&allLocalComps,
                                             (ClIocAddressT*)&allNodeReps,
                                             NULL, NULL, gClTipcXportType);
        }
        break;

    default :
        break;
    }

    return rc;
}

static ClUint8T buffer[0xffff+1];
static ClRcT tipcEventHandler(ClInt32T fd, ClInt32T events, void *handlerIndex)
{
    ClUint32T index = (ClUint32T) ((ClInt32T*)handlerIndex - handlerFd);
    ClRcT rc = CL_OK;
    struct msghdr msgHdr;
    struct iovec ioVector[1];
    struct sockaddr_tipc peerAddress;
    ClInt32T bytes;
    static ClUint32T recvErrors = 0;

    clOsalMutexLock(&gIocEventHandlerSendLock);

    if(!eventHandlerInited)
    {
        rc = CL_ERR_NOT_INITIALIZED;
        goto out_unlock;
    }

    memset((char*)&msgHdr, 0, sizeof(msgHdr));
    memset((char*)ioVector, 0, sizeof(ioVector));
    msgHdr.msg_name = (struct sockaddr_tipc*)&peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector[0].iov_base = (ClPtrT)buffer;
    ioVector[0].iov_len = sizeof(buffer);
    msgHdr.msg_iov = ioVector;
    msgHdr.msg_iovlen = sizeof(ioVector)/sizeof(ioVector[0]);

    bytes = recvmsg(fd, &msgHdr, 0);
    if(bytes <= 0 )
    {
        if(bytes == 0)
            goto out_unlock;

        if( !(recvErrors++ & 255) )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                           ("Recvmsg failed with [%s]\n", strerror(errno)));
            sleep(1);
        }

        if(errno == ENOTCONN)
        {
            if(tipcEventRegister(CL_TRUE) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, 
                               ("TIPC topology subsciption retry failed. "
                                "Shutting down the notification thread and process\n"));
                exit(0); 
            }
        }
        goto out_unlock;
    }

    rc = clTipcReceivedPacket(index, &msgHdr);
    if(shutdownFlag)
    {
        tipcEventDeregister(-1);
    }
    
    out_unlock:
    clOsalMutexUnlock(&gIocEventHandlerSendLock);
    return rc;
}

ClRcT clTipcEventHandlerInitialize(void)
{
    ClRcT rc;

    allLocalComps = CL_IOC_ADDRESS_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress, CL_IOC_BROADCAST_ADDRESS);

    rc = clOsalMutexInit(&gIocEventHandlerSendLock);
    CL_ASSERT(rc == CL_OK);

    rc = clOsalMutexInit(&gTipcNodeStatusMutex);
    CL_ASSERT(rc == CL_OK);

    rc = clIocNotificationInitialize();
    CL_ASSERT(rc == CL_OK);

    rc = clTransportListenerCreate(&gNotificationListener);
    
    if(rc != CL_OK)
        goto out;

    rc = tipcEventRegister(CL_FALSE);

    if(rc != CL_OK)
    {
        goto out_free;
    }

    char* temp;
    temp = getenv("CL_TIPC_SUBSCR_TIMEOUT");
    gClTipcSubscrTimeout = temp ? (ClUint32T)atoi(temp) : CL_TIPC_SUBSCR_TIMEOUT;

    eventHandlerInited = 1;

    out:
    return rc;

    out_free:
    clTransportListenerDestroy(&gNotificationListener);
    return rc;
}

ClRcT clTipcEventHandlerFinalize(void)
{

    /* If service was never initialized, don't finalize */
    if(!eventHandlerInited)
      return CL_OK;


    /* Closing only the data socket so the the event socket can get the event of this and it will comeout of the recview thread */
    clOsalMutexLock(&gIocEventHandlerSendLock);
    clIocNotificationFinalize();
    tipcEventDeregister(-1);
    eventHandlerInited = 0;
    clOsalMutexUnlock(&gIocEventHandlerSendLock);

    clTransportListenerDestroy(&gNotificationListener);

    return CL_OK;
}
