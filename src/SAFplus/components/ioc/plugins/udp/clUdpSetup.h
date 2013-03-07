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

ClRcT clUdpFdGet(ClIocPortT port, ClInt32T *fd);
ClRcT clIocUdpMapAdd(struct sockaddr *addr, ClIocNodeAddressT slot);
ClRcT clIocUdpMapDel(ClIocNodeAddressT slot);
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
