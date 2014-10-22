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
/*******************************************************************************
 * ModuleName  : ioc                                                           
 * File        : clIocTipcUserApi.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file contains all the API definations. These functions do the
 * "ioctl" calls to the respective kernel functions, which are present in the
 * IOC kernel module.
 *
 *
 *****************************************************************************/


#include <clCommon.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <errno.h>


#if OS_VERSION_CODE < OS_KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

#include <clLogApi.hxx>
#include <clOsalApi.h>
#include <clEoIpi.h>
#include <clHash.h>
#include <clList.h>
#include <clCksmApi.h>
#include <clRbTree.h>
#include <clJobQueue.h>
#include <clVersionApi.h>
#include <clIocManagementApi.h>
#include <clIocErrors.h>
#include <clIocIpi.h>
#include <clIocParseConfig.h>
#include <clIocUserApi.h>
#include <clIocGeneral.h>
#include <clIocMaster.h>
#include "clTipcNotification.h"
#include "clTipcSetup.h"
#include <clLeakyBucket.h>
#include <clNodeCache.h>
#include <clNetwork.h>
#include <clTimeServer.h>
#include <clTransport.h>
#include <clTipc.h>

#define TIPC_LOG_AREA_TIPC		"TIPC"
#define TIPC_LOG_CTX_TIPC_DISPATCH	"DISP"
#define TIPC_LOG_CTX_TIPC_BIND		"BIND"
#define TIPC_LOG_CTX_TIPC_SEND		"SEND"
#define TIPC_LOG_CTX_TIPC_GET		"GET"
#define TIPC_LOG_CTX_TIPC_RECV		"RECV"
#define TIPC_LOG_CTX_TIPC_READY		"RDY"

#ifdef __cplusplus
extern "C" {
#endif

extern ClIocNodeAddressT gIocLocalBladeAddress;
#define MAX_MESSAGE_LENGTH 0xffff
ClInt32T gClTipcXportId;
ClCharT gClTipcXportType[CL_MAX_NAME_LENGTH];
static ClBoolT tipcPriorityChangePossible = CL_TRUE;  /* Don't attempt to change priority if TIPC does not support, so we don't get tons of error msgs */
static ClUint32T gTipcInit = CL_FALSE;

typedef struct ClTipcCommPortPrivate
{
    ClIocPortT portId;
    ClInt32T fd;
    ClInt32T bcastFd;
    ClUint32T priority;
    ClBoolT activeBind;
}ClTipcCommPortPrivateT;

#define CL_TIPC_TYPE(iocAddress) clTipcSetType(((ClUint32T)(((ClUint64T)(iocAddress)>>32)&0xffffffffU)),CL_FALSE)
#define CL_TIPC_INSTANCE(iocAddress) ((ClUint32T)(((ClUint64T)(iocAddress))&0xffffffffU))

#define CL_TIPC_PORT_EXIT_MESSAGE "QUIT"
#undef NULL_CHECK
#define NULL_CHECK(X)                               \
    do {                                            \
        if((X) == NULL)                             \
        return CL_IOC_RC(CL_ERR_NULL_POINTER);  \
    } while(0)

static ClOsalMutexT gClTipcPortMutex;
static ClOsalMutexT gClTipcCompMutex;
static ClOsalMutexT gClTipcMcastMutex;

/*  
 * When running with asp modified tipc supporting 64k.
 */
#ifdef POSIX_BUILD
#undef CL_TIPC_PACKET_SIZE
#define CL_TIPC_PACKET_SIZE (0XFFFF)
#endif

#define clIocInternalMaxPayloadSizeGet()  (CL_TIPC_PACKET_SIZE - sizeof(ClIocHeaderT) - sizeof(ClIocHeaderT) - 1)

static ClBoolT gClTipcReplicast;

ClUint32T clTipcMcastType(ClUint32T type)
{
    CL_ASSERT((type + CL_TIPC_BASE_TYPE) < 0xffffffff);
    return (type + CL_TIPC_BASE_TYPE);
}
       
ClUint32T clTipcSetType(ClUint32T portId, ClBoolT setFlag)
{
    ClUint32T type = portId;
    if(setFlag == CL_TRUE)
    {
        type += CL_TIPC_BASE_TYPE;

        CL_ASSERT(type <= CL_IOC_COMM_PORT_MASK);
    }
    else
    {
        CL_ASSERT(type > CL_IOC_COMM_PORT_MASK);
    }
    return type;
}

static ClRcT tipcDispatchCallback(ClInt32T fd, ClInt32T events, void *cookie)
{
    ClRcT rc = CL_OK;
    ClTipcCommPortPrivateT *xportPrivate = (ClTipcCommPortPrivateT*) cookie;
    struct msghdr msgHdr;
    struct sockaddr_tipc peerAddress;
    struct iovec ioVector[1];
    ClUint8T *buffer = NULL;
    buffer = __iocMessagePoolGet();
    ClInt32T bytes;

    if(!xportPrivate)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_DISPATCH,"No private data\n");
        rc = CL_IOC_RC(CL_ERR_INVALID_HANDLE);
        goto out;
    }

    memset(&msgHdr, 0, sizeof(msgHdr));
    memset(ioVector, 0, sizeof(ioVector));
    memset(&peerAddress, 0, sizeof(peerAddress));
    msgHdr.msg_name = &peerAddress;
    msgHdr.msg_namelen = sizeof(peerAddress);
    ioVector[0].iov_base = (ClPtrT)buffer;
    ioVector[0].iov_len = MAX_MESSAGE_LENGTH;
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
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_DISPATCH,"recv error. errno = %d\n",errno);
        rc = CL_ERR_LIBRARY;
        goto out;
    }
    rc = clIocDispatchAsync(gClTipcXportType, xportPrivate->portId, buffer, bytes);
    if(((ClIocHeaderT*)buffer)->flag == 0)
    {
        __iocMessagePoolPut(buffer);
    }
    out:
    return rc;
}

static ClRcT __xportBind(ClIocPortT portId, ClBoolT listen)
{
    ClTipcCommPortPrivateT *pTipcCommPort = NULL;
    ClTipcCommPortPrivateT *pTipcCommPortLast = NULL;
    struct sockaddr_tipc address;
    ClRcT rc = CL_OK;
    ClUint32T priority = CL_TIPC_DEFAULT_PRIORITY;

    pTipcCommPort = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, portId);
    if(pTipcCommPort)
    {
        pTipcCommPortLast = pTipcCommPort;
        if(listen)
        {
            goto out_listen;
        }
        return CL_OK;
    }
        
    pTipcCommPort = (ClTipcCommPortPrivateT*) clHeapCalloc(1,sizeof(*pTipcCommPort));
    CL_ASSERT(pTipcCommPort != NULL);
    
    pTipcCommPort->fd = socket(AF_TIPC, SOCK_RDM, 0);

    rc = CL_ERR_UNSPECIFIED;

    if(pTipcCommPort->fd < 0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND, 
                   "Error : socket() failed. system error [%s].\n", strerror(errno));
        goto out_free;
    }
    
    if(fcntl(pTipcCommPort->fd, F_SETFD, FD_CLOEXEC) < 0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error: socket fcntl failed with error [%s]\n",
                                        strerror(errno));
        goto out_close;
    }

    memset((char*)&address,0, sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = TIPC_ZONE_SCOPE;
    address.addr.name.domain=0;
    address.addr.name.name.instance = gIocLocalBladeAddress;
    address.addr.name.name.type = CL_TIPC_SET_TYPE(portId);

    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(struct sockaddr_tipc)) < 0 )
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in bind.errno=%d\n",errno);
        goto out_close;
    }

    pTipcCommPort->activeBind = CL_FALSE;
    pTipcCommPort->portId = portId;

    if(portId == CL_IOC_CPM_PORT)
    {
        /*Let CPM use high priority messaging*/
        priority = CL_TIPC_HIGH_PRIORITY;
    }
    if(!setsockopt(pTipcCommPort->fd,SOL_TIPC,TIPC_IMPORTANCE, (char *)&priority,sizeof(priority)))
    {
        pTipcCommPort->priority = priority;
    }
    else
    {
        int err = errno;
        if (err == ENOPROTOOPT)
        {
            tipcPriorityChangePossible = CL_FALSE;
            logWarning(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Message priority not available in this version of TIPC.");
        }            
        else logWarning(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in setting TIPC message priority. errno [%d]",err);

        pTipcCommPort->priority = CL_TIPC_DEFAULT_PRIORITY;
    }
        
    if(portId != CL_IOC_XPORT_PORT)
    {
        /* The following bind is for doing the intranode communication */
        bzero((char*)&address,sizeof(address));
        address.family = AF_TIPC;
        address.addrtype = TIPC_ADDR_NAME;
        address.scope = TIPC_NODE_SCOPE;
        address.addr.name.domain=0;
        address.addr.name.name.type = CL_IOC_TIPC_ADDRESS_TYPE_FORM(CL_IOC_INTRANODE_ADDRESS_TYPE, gIocLocalBladeAddress);
        address.addr.name.name.instance = CL_TIPC_SET_TYPE(portId);

        if(bind(pTipcCommPort->fd, (struct sockaddr *)&address, sizeof(struct sockaddr_tipc)) < 0)
        {
            logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error : bind failed. errno = %d", errno);
            goto out_close;
        }
    }

#ifdef BCAST_SOCKET_NEEDED
    {
        pTipcCommPort->bcastFd = socket(AF_TIPC, SOCK_RDM, 0);
        if(pTipcCommPort->bcastFd < 0)
            goto out_close;

        if(fcntl(pTipcCommPort->bcastFd, F_SETFD, FD_CLOEXEC) < 0)
        {
            close(pTipcCommPort->bcastFd);
            pTipcCommPort->bcastFd = -1;
            goto out_close;
        }
    }
#endif

    if(listen)
    {
        out_listen:
        rc = clTransportListenerRegister(pTipcCommPort->fd, tipcDispatchCallback, (void*)pTipcCommPort);
        if(rc != CL_OK)
        {
            goto out_close;
        }
    }
    if(!pTipcCommPortLast)
    {
        clTransportPrivateDataSet(gClTipcXportId, portId, (void*)pTipcCommPort, NULL);
    }

    rc = CL_OK;
    goto out;

    out_close:
    if(!pTipcCommPortLast)
        close(pTipcCommPort->fd);

    out_free:
    if(!pTipcCommPortLast)
        clHeapFree(pTipcCommPort);

    out:
    return rc;
}

static ClRcT __xportBindClose(ClIocPortT port, ClBoolT listen)
{
    ClTipcCommPortPrivateT *pTipcCommPort  = NULL;
    ClRcT rc = CL_OK;

    if( !(pTipcCommPort = (ClTipcCommPortPrivateT*)clTransportPrivateDataDelete(gClTipcXportId, port) ) )
        return CL_ERR_NOT_EXIST;

    if(pTipcCommPort->portId != port)
    {
        rc = CL_ERR_NOT_EXIST;
        goto out_free;
    }

    /*This would withdraw all the binds*/
    if(listen)
    {
        clTransportListenerDeregister(pTipcCommPort->fd);
    }
    else
    {
        close(pTipcCommPort->fd);
    }

#ifdef BCAST_SOCKET_NEEDED
    close(pTipcCommPort->bcastFd);
#endif

    out_free:
    clHeapFree(pTipcCommPort);
    return rc;
}

ClRcT xportBind(ClIocPortT port)
{
    return __xportBind(port, CL_FALSE);
}

ClRcT xportBindClose(ClIocPortT port)
{
    return __xportBindClose(port, CL_FALSE);
}

ClRcT xportListen(ClIocPortT port)
{
    return __xportBind(port, CL_TRUE);
}

ClRcT xportListenStop(ClIocPortT port)
{
    return __xportBindClose(port, CL_TRUE);
}

/*
 * Contruct the tipc address from the ioc address
 */
#ifdef BCAST_SOCKET_NEEDED
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress,
                       ClIocAddressT *pDestAddress,
                       ClUint32T *pSendFDFlag
                       )
#else
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress,
                       ClIocAddressT *pDestAddress
                       )
#endif
{
    ClInt32T type;
    memset((char*)pAddress, 0, sizeof(*pAddress));
    pAddress->family = AF_TIPC;
    pAddress->addrtype = TIPC_ADDR_NAME;
    pAddress->scope = TIPC_ZONE_SCOPE;
    type = CL_IOC_ADDRESS_TYPE_GET(pDestAddress);
#ifdef BCAST_SOCKET_NEEDED
    *pSendFDFlag = 1;
#endif

    switch(type)
    {
    case CL_IOC_PHYSICAL_ADDRESS_TYPE:
        if(pDestAddress->iocPhyAddress.nodeAddress == CL_IOC_BROADCAST_ADDRESS)
        {
            goto set_broadcast;
        }
        pAddress->addr.name.name.type = CL_TIPC_SET_TYPE(pDestAddress->iocPhyAddress.portId);
        pAddress->addr.name.name.instance = pDestAddress->iocPhyAddress.nodeAddress;
        pAddress->addr.name.domain=0;
        break;
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        pAddress->addr.name.name.type = CL_TIPC_TYPE(pDestAddress->iocLogicalAddress);
        pAddress->addr.name.name.instance = CL_TIPC_INSTANCE(pDestAddress->iocLogicalAddress);
        pAddress->addr.name.domain=0;
        break;
    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_TIPC_TYPE(pDestAddress->iocMulticastAddress);
        pAddress->addr.nameseq.lower = CL_TIPC_INSTANCE(pDestAddress->iocMulticastAddress);  
        pAddress->addr.nameseq.upper = CL_TIPC_INSTANCE(pDestAddress->iocMulticastAddress);
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    case CL_IOC_BROADCAST_ADDRESS_TYPE:
    set_broadcast:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_TIPC_SET_TYPE(pDestAddress->iocPhyAddress.portId);
        pAddress->addr.nameseq.lower = CL_IOC_MIN_NODE_ADDRESS;
        pAddress->addr.nameseq.upper = CL_IOC_MAX_NODE_ADDRESS;
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    case CL_IOC_INTRANODE_ADDRESS_TYPE:
        pAddress->addrtype = TIPC_ADDR_NAMESEQ;
        pAddress->addr.nameseq.type = CL_TIPC_TYPE(pDestAddress->iocLogicalAddress);
        pAddress->addr.nameseq.lower = CL_TIPC_SET_TYPE(CL_IOC_MIN_COMP_PORT);
        pAddress->addr.nameseq.upper = CL_TIPC_SET_TYPE(CL_IOC_MAX_COMP_PORT);
#ifdef BCAST_SOCKET_NEEDED
        *pSendFDFlag = 2;
#endif
        break;
    default:
        return CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    }
    return CL_OK;
}

ClRcT xportRecv(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, ClUint8T *pBuffer, ClUint32T bufSize, 
                ClBufferHandleT message, ClIocRecvParamT *pRecvParam)
{
    ClRcT rc = CL_OK;
    ClUint8T *poolBuffer =NULL;
    ClIocCommPortT *pCommPort = (ClIocCommPortT*)commPort;
    ClTipcCommPortPrivateT *pCommPortPrivate = NULL;
    struct msghdr msgHdr;
    struct sockaddr_tipc peerAddress;
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

    if(! (pCommPortPrivate = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, pCommPort->portId)) )
    {
        rc = CL_ERR_NOT_EXIST;
        goto out;
    }
    poolBuffer= __iocMessagePoolGet();
    bufSize = MAX_MESSAGE_LENGTH;
    if(!pBuffer)
    {
        pBuffer= poolBuffer;
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
                    logDebug("TIPC", "RECV", "recv error. errno = %d\n",errno);
                    goto out;
                }
            } 
            else 
            {
                logDebug("TIPC", "RECV", "poll error. errno = %d\n", errno);
                goto out;
            }
        } 
        else if(pollStatus < 0 ) 
        {
            if(errno == EINTR)
                continue;
            logDebug("TIPC", "RECV", "Error in poll. errno = %d\n",errno);
            goto out;
        } 
        else 
        {
            rc = CL_ERR_TIMEOUT;
            goto out;
        }
        break;
    }
    rc = clIocDispatch(gClTipcXportType, commPort, pRecvOption, poolBuffer, bytes, message, pRecvParam);

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN)
        goto retry;

    if(CL_GET_ERROR_CODE(rc) == IOC_MSG_QUEUED)
    {
        ClUint32T elapsedTm;
        if(timeout == CL_IOC_TIMEOUT_FOREVER)
        {
            poolBuffer= __iocMessagePoolGet();
            ioVector.iov_base = (ClPtrT)poolBuffer;
            msgHdr.msg_iov = &ioVector;
            goto retry;
        }
        elapsedTm = (clOsalStopWatchTimeGet() - tm)/1000;
        if(elapsedTm < timeout)
        {
            timeout -= elapsedTm;
            poolBuffer= __iocMessagePoolGet();
            ioVector.iov_base = (ClPtrT)poolBuffer;
            msgHdr.msg_iov = &ioVector;
            goto retry;
        }
        else
        {
            rc = CL_ERR_TIMEOUT;
            logDebug("TIPC", "RECV", "Dropping a received fragmented-packet. "
                                              "Could not receive the complete packet within "
                                              "the specified timeout. Packet size is %d", bytes);

        }
    }

    out:
    if(((ClIocHeaderT*)poolBuffer)->flag == 0)
    {
        __iocMessagePoolPut(poolBuffer);
    }
    return rc;

}

ClRcT xportSend(ClIocPortT port, ClUint32T tempPriority, ClIocAddressT *pIocAddress,
                struct iovec *target, ClUint32T targetVectors, ClInt32T flags)
{
    ClRcT rc = CL_OK;
    struct sockaddr_tipc tipcAddress;
    ClInt32T fd ;
    ClInt32T bytes;
    struct msghdr msgHdr;
    ClUint32T priority;
#ifdef BCAST_SOCKET_NEEDED
    ClUint32T sendFDFlag = 1;
#endif
    ClInt32T tries = 0;
    ClTipcCommPortPrivateT *pTipcCommPort = NULL;

    if(!pIocAddress || !target || !targetVectors)
        return CL_OK;

    if( !(pTipcCommPort = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, port) ) )
    {
        rc = CL_ERR_NOT_EXIST;
        goto out;
    }

    /*map the address to a TIPC address before sending*/
#ifdef BCAST_SOCKET_NEEDED
    rc = clTipcGetAddress(&tipcAddress, pIocAddress, &sendFDFlag);
#else
    rc = clTipcGetAddress(&tipcAddress, pIocAddress);
#endif

    if(rc != CL_OK)
    {
        logDebug("TIPC", "SEND", "Error in tipc get address.rc=0x%x\n",rc);
        goto out;
    }

    fd = pTipcCommPort->fd;

#ifdef BCAST_SOCKET_NEEDED
    if(sendFDFlag == 2)
        fd = pTipcCommPort->bcastFd;
#endif

    priority = CL_TIPC_DEFAULT_PRIORITY;
    if(pTipcCommPort->portId == CL_IOC_CPM_PORT  ||
       pTipcCommPort->portId == CL_IOC_XPORT_PORT ||
       tempPriority == CL_IOC_HIGH_PRIORITY)
    {
        priority = CL_TIPC_HIGH_PRIORITY;
    }
    if ((tipcPriorityChangePossible) && (pTipcCommPort->priority != priority))
    {
        if(!setsockopt(fd,SOL_TIPC,TIPC_IMPORTANCE,(char *)&priority,sizeof(priority)))
        {
            pTipcCommPort->priority = priority;
        }
        else
        {
            int err = errno;

            if (err == ENOPROTOOPT)
            {
                tipcPriorityChangePossible = CL_FALSE;
                logWarning(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_SEND,"Message priority not available in this version of TIPC.");
            }            
            else logWarning(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in setting TIPC message priority. errno [%d]",err);
        }
    }
    memset((char*)&msgHdr, 0, sizeof(msgHdr));
    msgHdr.msg_name = (ClPtrT)&tipcAddress;
    msgHdr.msg_namelen = sizeof(tipcAddress);
    msgHdr.msg_iov = target;
    msgHdr.msg_iovlen = targetVectors;

    retry:
    bytes = sendmsg(fd,&msgHdr,0);

    if(bytes <= 0 )
    {
        if(errno == EINTR)
            goto retry;
     
        if(errno == EAGAIN)
        {
            if(++tries < 10)
            {
                usleep(100);
                goto retry;
            }
        }
        logDebug("TIPC", "SEND", "Error : Failed at sendmsg. errno = %d\n",errno);
        rc = CL_ERR_UNSPECIFIED;
    }

    out:
    return rc;
}

ClRcT xportClose(void)
{
    if(gTipcInit == CL_FALSE)
        return CL_OK;
    gTipcInit = CL_FALSE;
    return CL_OK;
}

ClRcT xportNotifyInit(void)
{
    if(!gTipcInit)
        return CL_ERR_NOT_INITIALIZED;
    return clTipcEventHandlerInitialize();
}

ClRcT xportNotifyFinalize(void)
{
    if(!gTipcInit)
        return CL_ERR_NOT_INITIALIZED;
    return clTipcEventHandlerFinalize();
}

ClRcT xportNodeAddrGet(void)
{
    return clTipcOwnAddrGet();
}
/*
 * Already handled by the tipc notification interface.
 */
ClRcT xportNotifyOpen(ClIocPortT port)
{
    return CL_OK;
}

ClRcT xportNotifyClose(ClIocNodeAddressT nodeAddress, ClIocPortT port)
{
    return CL_OK;
}

static ClRcT tipcConfigInitialize(ClBoolT nodeRep)
{
    gClTipcReplicast = clParseEnvBoolean("CL_ASP_TIPC_REPLICAST");
    return CL_OK;
}


ClRcT xportInit(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep)
{
    ClRcT rc = CL_OK;
    if (gTipcInit == CL_TRUE)
        goto out;
    gClTipcXportId = xportId;
    if(xportType)
    {
        gClTipcXportType[0] = 0;
        strncat(gClTipcXportType, xportType, sizeof(gClTipcXportType)-1);
    }
    rc = clOsalMutexInit(&gClTipcPortMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClTipcCompMutex);
    CL_ASSERT(rc == CL_OK);
    rc = clOsalMutexInit(&gClTipcMcastMutex);
    CL_ASSERT(rc == CL_OK);
    /* 
     * Check if the tipc address or asp address is in use
     */
    if(nodeRep)
    {
        rc  = clTipcDoesNodeAlreadyExist();
        if(rc != CL_OK)
            goto out;
    }
    rc = tipcConfigInitialize(nodeRep);
    if(rc != CL_OK)
        goto out;
    gTipcInit = CL_TRUE;

    out:
    return rc;
}

ClRcT xportMaxPayloadSizeGet(ClUint32T *pSize)
{
    NULL_CHECK(pSize);
    *pSize = clIocInternalMaxPayloadSizeGet();
    return CL_OK;
}

ClRcT xportTransparencyRegister(ClIocPortT port, ClIocLogicalAddressT logicalAddr, ClUint32T haState)
{
    ClInt32T scope = 1;
    struct sockaddr_tipc address;
    ClBoolT bindFlag = CL_TRUE;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    ClTipcCommPortPrivateT *pTipcCommPort = NULL;

    if( !(pTipcCommPort = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, port)))
        return rc;

    if(haState == CL_IOC_TL_STDBY)
    {
        if(!pTipcCommPort->activeBind)
        {
            rc = CL_OK;
            goto out;
        }
        /*
         * unbind
         */
        bindFlag = CL_FALSE;
        scope = -1;
    }
    bzero((char*)&address,sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = scope*TIPC_ZONE_SCOPE;
    address.addr.name.name.type = CL_TIPC_TYPE(logicalAddr);
    address.addr.name.name.instance = CL_TIPC_INSTANCE(logicalAddr);
    rc = CL_IOC_RC(CL_ERR_LIBRARY);
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in bind.errno=%d\n",errno);
        goto out;
    }
    address.addr.name.name.type = CL_IOC_MASTER_TYPE(pTipcCommPort->portId);
    address.addr.name.name.instance = gIocLocalBladeAddress;
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in bind.errno=%d\n",errno);
        goto out_unbind;
    }
    if(bindFlag)
    {
        pTipcCommPort->activeBind = CL_TRUE;
    }
    rc = CL_OK;
    goto out;
    
    out_unbind:
    address.addr.name.name.type = CL_TIPC_TYPE(logicalAddr);
    address.addr.name.name.instance = CL_TIPC_INSTANCE(logicalAddr);
    address.scope = -scope*TIPC_ZONE_SCOPE;
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address))<0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in bind.errno=%d\n",errno);
    }
    out:
    return rc;
}

ClRcT xportTransparencyDeregister(ClIocPortT port, ClIocLogicalAddressT logicalAddr)
{
    return xportTransparencyRegister(port, logicalAddr, CL_IOC_TL_STDBY);
}

static ClRcT __xportMulticastBind(ClIocPortT port, ClIocMulticastAddressT mcastAddr,
                                  ClInt32T bindScope)
{
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
    struct sockaddr_tipc address;
    ClTipcCommPortPrivateT *pTipcCommPort = NULL;

    if(!(pTipcCommPort = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, port)))
        goto out;

    memset(&address, 0, sizeof(address));
    address.family = AF_TIPC;
    address.addrtype = TIPC_ADDR_NAME;
    address.scope = bindScope * TIPC_ZONE_SCOPE;
    address.addr.name.name.type = CL_TIPC_TYPE(mcastAddr);
    address.addr.name.name.instance = CL_TIPC_INSTANCE(mcastAddr);
    address.addr.name.domain = 0;

    rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);

    /*Fire the bind*/
    if(bind(pTipcCommPort->fd,(struct sockaddr*)&address,sizeof(address)) < 0 )
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_BIND,"Error in bind.errno=%d\n",errno);
        goto out;
    }
    rc = CL_OK;

    out:
    return rc;
}

ClRcT xportMulticastRegister(ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{
    return __xportMulticastBind(port, mcastAddr, 1);
}

ClRcT xportMulticastDeregister(ClIocPortT port, ClIocMulticastAddressT mcastAddr)
{
    return __xportMulticastBind(port, mcastAddr, -1);
}
    
ClRcT xportServerReady(ClIocAddressT *pAddress)
{
    struct sockaddr_tipc topsrv;
    struct tipc_event event;
    struct tipc_subscr subscr;
    ClUint32T type, lower, upper ;
    ClInt32T fd;
    ClInt32T ret;
    ClRcT rc = CL_IOC_RC(CL_ERR_INVALID_PARAMETER);
 
    NULL_CHECK(pAddress);

    switch(CL_IOC_ADDRESS_TYPE_GET(pAddress))
    {
    case CL_IOC_LOGICAL_ADDRESS_TYPE:
        type = CL_TIPC_TYPE(pAddress->iocLogicalAddress);
        lower = CL_TIPC_INSTANCE(pAddress->iocLogicalAddress);
        upper = lower;
        break;

    case CL_IOC_MULTICAST_ADDRESS_TYPE:
        type = CL_TIPC_TYPE(pAddress->iocMulticastAddress);
        lower = CL_TIPC_INSTANCE(pAddress->iocMulticastAddress);
        upper = lower;
        break;

    default:
        goto out;
    }

    rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
    fd = socket(AF_TIPC,SOCK_SEQPACKET,0);
    if(fd < 0 )
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_READY,"Error creating socket.errno=%d\n",errno);
        goto out;
    }
    ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);
    memset(&topsrv,0,sizeof(topsrv));
    memset(&event,0,sizeof(event));
    memset(&subscr,0,sizeof(subscr));

    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;
    if(connect(fd,(struct sockaddr*)&topsrv,sizeof(topsrv))<0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_READY,"Error in connecting to topology server.errno=%d\n",errno);
        goto out_close;
    }
    /*
     * Make a subscription filter with the address first. to see if thats
     * available.
     */
    subscr.seq.type = type;
    subscr.seq.lower = lower;
    subscr.seq.upper = upper;
    subscr.timeout = CL_TIPC_TOP_SRV_READY_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    if(send(fd, (const char *)&subscr,sizeof(subscr),0)!=sizeof(subscr))
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_SEND,"Error send to topology service.errno=%d\n",errno);
        goto out_close;
    }
    if(recv(fd, (char *)&event,sizeof(event),0) != sizeof(event))
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_RECV,"Error recv from topology.errno=%d\n",errno);
        goto out_close;
    }
    if(event.event != TIPC_PUBLISHED)
    {
        logWarning(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_READY,"Error in recv. event=%d\n",event.event);
        goto out_close;
    }

    rc = CL_OK;

    out_close:
    close(fd);

    out:
    return rc;
}

ClRcT xportMasterAddressGet(ClIocLogicalAddressT logicalAddress, ClIocPortT portId, ClIocNodeAddressT *pIocNodeAddress)
{
    struct sockaddr_tipc topsrv;
    ClInt32T fd;
    ClInt32T ret;
    struct tipc_event event;
    struct tipc_subscr subscr;
    ClRcT rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);

    fd = socket(AF_TIPC,SOCK_SEQPACKET,0);
    if(fd < 0 )
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_GET,"Error creating socket.errno=%d\n",errno);
        goto out;
    }
    ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);
    memset(&topsrv,0,sizeof(topsrv));
    memset(&event,0,sizeof(event));
    memset(&subscr,0,sizeof(subscr));

    topsrv.family = AF_TIPC;
    topsrv.addrtype = TIPC_ADDR_NAME;
    topsrv.addr.name.name.type = TIPC_TOP_SRV;
    topsrv.addr.name.name.instance = TIPC_TOP_SRV;
    if(connect(fd,(struct sockaddr*)&topsrv,sizeof(topsrv))<0)
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_GET,"Error in connecting to topology server.errno=%d\n",errno);
        goto out_close;
    }

    /*
     *Make a subscription filter:
     *The master is expected to have bound
     *on a master address type.
     */
    subscr.seq.type = CL_IOC_MASTER_TYPE(portId);
    subscr.seq.lower = CL_IOC_MIN_NODE_ADDRESS;
    subscr.seq.upper = CL_IOC_MAX_NODE_ADDRESS;
    subscr.timeout = CL_TIPC_TOP_SRV_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    if(send(fd, (const char*)&subscr,sizeof(subscr),0) != sizeof(subscr))
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_SEND,"Error in send. errno=%d\n",errno);
        goto out_close;
    }
    if(recv(fd, (char*)&event,sizeof(event),0) != sizeof(event))
    {
        logError(TIPC_LOG_AREA_TIPC,TIPC_LOG_CTX_TIPC_RECV,"Error in recv. errno=%d\n",errno);
        goto out_close;
    }
    if(event.event != TIPC_PUBLISHED)
    {
        goto out_close;
    }
    CL_ASSERT(event.found_lower == event.found_upper);
    *pIocNodeAddress = (ClIocNodeAddressT)event.found_lower;
    rc = CL_OK;

out_close:
    close(fd);
out:
    return rc;
}

/*
 * Return fd for local port
 */
ClRcT xportFdGet(ClIocCommPortHandleT commPort, ClInt32T *fd)
{
    ClIocCommPortT *pCommPort = (ClIocCommPortT*)commPort;
    return clTipcFdGet(pCommPort->portId, fd);
}

ClRcT clTipcFdGet(ClIocPortT port, ClInt32T *fd)
{
    ClTipcCommPortPrivateT *xportPrivate;
    if(!fd) return CL_ERR_INVALID_PARAMETER;
    if(! (xportPrivate = (ClTipcCommPortPrivateT*)clTransportPrivateDataGet(gClTipcXportId, port)))
        return CL_ERR_NOT_EXIST;
    *fd = xportPrivate->fd;
    return CL_OK;
}

#ifdef __cplusplus
 }
#endif
