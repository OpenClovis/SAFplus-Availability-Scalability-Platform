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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#include <clDebugApi.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocServices.h>
#include "clTipcSetup.h"
#include <clIocUserApi.h>
//#include <clIocIpi.h>
//#include <clIocNeighComps.h>

#define CL_TIPC_DUPLICATE_NODE_TIMEOUT (100)

static ClUint32T clTipcOwnAddrGet(void)
{
    struct sockaddr_tipc addr;
    socklen_t sz = sizeof(addr);
    ClInt32T sd;
    ClInt32T rc;

    sd = socket(AF_TIPC, SOCK_RDM, 0);
    if(sd < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : socket() failed. system error [%s].\n", strerror(errno)));
        CL_ASSERT(0);
        return CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }
    
    rc = getsockname(sd, (struct sockaddr *)&addr, &sz);
    CL_ASSERT(rc >= 0);
    
    close(sd);
    return tipc_node(addr.addr.id.node);
}

static ClRcT clTipcIsAddressInUse(ClUint32T type, ClUint32T instance)
{
    ClInt32T topoFd;
    struct sockaddr_tipc topoAddr;
    ClInt32T ret;
    struct tipc_subscr subscr;
    struct tipc_event event;
    ClInt32T bytes;
    ClRcT rc = CL_OK;
    ClInt32T retCode;
    
    topoFd = socket(AF_TIPC, SOCK_SEQPACKET, 0);
    if(topoFd < 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error : socket() failed. system error [%s].\n", strerror(errno)));
        CL_ASSERT(0);
        return CL_IOC_RC(CL_ERR_UNSPECIFIED);
    }
    ret = fcntl(topoFd, F_SETFD, FD_CLOEXEC);
    CL_ASSERT(ret == 0);

    memset(&topoAddr, 0, sizeof(topoAddr));
    topoAddr.family = AF_TIPC;
    topoAddr.addrtype = TIPC_ADDR_NAME;
    topoAddr.addr.name.name.type = TIPC_TOP_SRV;
    topoAddr.addr.name.name.instance = TIPC_TOP_SRV;
    
    ret = connect(topoFd, (struct sockaddr *)&topoAddr, sizeof(topoAddr));
    CL_ASSERT(ret >= 0);

    bzero((char*)&subscr, sizeof(subscr));
    
    subscr.seq.type = type;
    subscr.seq.lower = instance;
    subscr.seq.upper = instance;
    subscr.timeout = CL_TIPC_TOP_SRV_TIMEOUT;
    subscr.filter = TIPC_SUB_SERVICE;
    
    retCode = send(topoFd, (const char *)&subscr, sizeof(subscr),0);
    CL_ASSERT(retCode >= 0);
    
wait_on_recv:
    bzero((char*)&event,sizeof(event));
    bytes = recv(topoFd, (char*)&event, sizeof(event),0);
    if(bytes != sizeof(event))
    {
        rc = CL_IOC_RC(CL_ERR_UNSPECIFIED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error : Failed to receive event. errno=%d\n", errno));
        goto out;
    }
    if(event.event == TIPC_PUBLISHED)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("TIPC duplicate. Lower: %u, Upper: %u, Port: (ref:%u.node:%u) Subscription: (name: (%u.%u.%u), timeout %u, filter: %x)", event.found_lower, event.found_upper, event.port.ref, event.port.node, event.s.seq.type,event.s.seq.lower,event.s.seq.upper, event.s.timeout, event.s.filter));
        rc = CL_IOC_RC(CL_IOC_ERR_NODE_EXISTS);
        goto dup_detected;
    }
    if(event.event != TIPC_SUBSCR_TIMEOUT)
        goto wait_on_recv;

    goto out;

dup_detected:
    /* FIXME : correction to the following print description is needed */
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
            ("Ioc initialization failed. error code = 0x%x", rc));

    if(instance == gIocLocalBladeAddress)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("CAUSE : The ASP Address [0x%x] is already being used by ASP on "
                 "TIPC node [%d.%d.%d] in this network.",gIocLocalBladeAddress,
                 ((event.port.node>>24) & 0xff), ((event.port.node>>12) & 0xfff),
                 (event.port.node & 0xfff)));

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("SOLUTION : If the application is being started through "
                 "\"safplus_amf\" then change the value of \"-l\" command line "
                 "parameter. "
                 "If it is DEBUG-CLI, then change the value of environment "
                 "variable \"ASP_NODEADDR\" in clDebugStart.sh.\n"));
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("CAUSE : A node with TIPC address [%d.%d.%d] already exists. ",
                 ((event.port.node>>24) & 0xff), ((event.port.node>>12) & 0xfff),
                 (event.port.node & 0xfff)));

        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("SOLUTION : Unload and Load the TIPC kernel module. Then "
                 "configure TIPC with a different address."));
    }

out:
    close(topoFd);
    return rc;
}
               
/*
 * Duplicate node detection. Checks for both tipc and IOC address
 * being unique. TIPC node address uniqueness depends on whether
 * we are running multinode or not.
 * If more than 1 node are started at the same time and the detection
 * detects duplicate for both the cases at the same time, then none will start.
 */
ClRcT clTipcDoesNodeAlreadyExist(void)
{
    ClRcT rc = CL_OK;
    ClUint32T tipcAddress = 0;

    if(getenv("ASP_MULTINODE") == NULL)
    {
        /* Check whether another TIPC node with same tipc address <z.c.n>,
         * as of this node exists 
         */
        tipcAddress = clTipcOwnAddrGet();
        rc = clTipcIsAddressInUse(CL_TIPC_SET_TYPE(CL_IOC_XPORT_PORT), tipcAddress);
        if(rc != CL_OK) 
            goto error_detected;
    }

    if(tipcAddress != gIocLocalBladeAddress)
    {
        /* Check whether another ASP-node is using same IOC address as of this ASP */
        rc = clTipcIsAddressInUse(CL_TIPC_SET_TYPE(CL_IOC_XPORT_PORT),
                                  gIocLocalBladeAddress);
    }

    error_detected:
    return rc;
}
