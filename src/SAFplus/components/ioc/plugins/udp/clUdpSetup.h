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

#ifndef __CL_UDP_SETUP_H__
#define __CL_UDP_SETUP_H__

#include <clIocConfig.h>
#include <clIocSetup.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_UDP_HIGH_PRIORITY    63
#define CL_UDP_DEFAULT_PRIORITY 0

typedef struct ClIocUdpMap
{
#define __ipv4_addr _addr.sin_addr
#define __ipv6_addr _addr.sin6_addr
    ClIocNodeAddressT slot;
    ClBoolT bridge;
    int family;
    int sendFd;
    union
    {
        struct sockaddr_in sin_addr;
        struct sockaddr_in6 sin6_addr;
    } _addr;
    char addrstr[80];
    struct hashStruct hash; /*hash linkage*/
    ClListHeadT list; /*list linkage*/
    ClUint32T priority;
}ClIocUdpMapT;

typedef struct ClIocUdpAddr
{
    ClIocNodeAddressT slot;
    ClCharT addrstr[80];
}ClIocUdpAddrT;

ClRcT clUdpFdGet(ClIocPortT port, ClInt32T *fd);
ClRcT clIocUdpMapAdd(struct sockaddr *addr, ClIocNodeAddressT slot, ClCharT *retAddrStr);
ClRcT clUdpAddrGet(ClIocNodeAddressT nodeAddress, ClCharT *addrStr);
ClRcT clUdpAddrSet(ClIocNodeAddressT nodeAddress, const ClCharT *addrStr);
ClRcT clIocUdpMapDel(ClIocNodeAddressT slot);
ClRcT clUdpMapWalk(ClRcT (*callback)(ClIocUdpMapT *map, void *cookie), void *cookie, ClInt32T flags);

extern ClBoolT gClSimulationMode;
extern ClInt32T gClUdpXportId;
extern ClCharT gClUdpXportType[CL_MAX_NAME_LENGTH];
extern struct cmsghdr *gClCmsgHdr;
extern ClInt32T gClCmsgHdrLen;
extern ClInt32T gClSockType;
extern ClInt32T gClProtocol;
#ifdef __cplusplus
}
#endif

#endif /* __CL_UDP_SETUP_H__ */
