/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
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
 * Build: 4.2.0
 */
#ifndef __CL_TIPC_USER_API_H__
#define __CL_TIPC_USER_API_H__

#include <clCommon.h>
#include <clOsalApi.h>
#include <clList.h>
#include <clHash.h>
#include <clIocApi.h>
#include <clIocParseConfig.h>


#define BCAST_SOCKET_NEEDED

#define CL_TIPC_SET_TYPE(v) clTipcSetType(v,CL_TRUE)


typedef struct
{
    ClUint8T version;
    ClUint8T protocolType;
    ClUint8T priority;
    ClUint8T flag;
    ClUint32T reserved;
    ClIocAddressT srcAddress;
#ifdef CL_TIPC_COMPRESSION
    ClTimeT       pktTime;
#endif
} ClTipcHeaderT;

typedef struct
{
    ClTipcHeaderT header;
    ClUint32T msgId;
    ClUint32T fragOffset;
    ClUint32T fragLength;
    ClUint32T reserved;
} ClTipcFragHeaderT;
    


typedef struct
{
    ClUint32T compId;
    struct hashStruct hash; /*hash linkage*/
    /*portlist linkage*/
    ClListHeadT portList;
}ClTipcCompT;


typedef struct
{
#define MAX_ADDRESSES (0x3)
    struct hashStruct hash; /*hash linkage*/
    ClInt32T fd;
#ifdef BCAST_SOCKET_NEEDED
    ClInt32T bcastFd;
#endif
    ClIocPortT portId;
    ClIocNotificationActionT notify;
    ClListHeadT logicalAddressList;
    ClListHeadT multicastAddressList;
    /*entry into the comp list for this port*/
    ClListHeadT listComp;
    ClTipcCompT *pComp;
    ClBoolT activeBind;
    ClOsalCondT unblockCond;
    ClOsalCondT recvUnblockCond;
    ClOsalMutexT unblockMutex;
    ClInt32T blocked;
    ClUint32T priority;
}ClTipcCommPortT;




extern ClIocConfigT pAllConfig;
extern ClIocNodeAddressT gIocLocalBladeAddress;

ClUint32T clTipcSetType(ClUint32T portId, ClBoolT setFlag);

#endif
