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
#ifndef __CL_IOC_USER_API_H__
#define __CL_IOC_USER_API_H__

#include <clCommon.h>
#include <clOsalApi.h>
#include <clList.h>
#include <clHash.h>
#include <clIocApi.h>
#include <clIocParseConfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BCAST_SOCKET_NEEDED


#define RELIABLE_IOC
typedef struct
{
    ClUint8T version;
    ClUint8T protocolType;
    ClUint8T priority;
    ClUint8T flag;
    ClUint32T reserved;
    ClIocAddressT srcAddress;
    ClIocAddressT dstAddress;
#ifdef RELIABLE_IOC
    ClUint32T messageId;
    ClBoolT isReliable;
#endif
#ifdef CL_IOC_COMPRESSION
    ClTimeT       pktTime;
#endif
} ClIocHeaderT;

typedef struct
{
    ClIocHeaderT header;
    ClUint32T msgId;
    ClUint32T fragOffset;
    ClUint32T fragLength;
    ClUint32T reserved;
    ClUint32T fragId;
} ClIocFragHeaderT;
    


typedef struct
{
    ClUint32T compId;
    struct hashStruct hash; /*hash linkage*/
    /*portlist linkage*/
    ClListHeadT portList;
}ClIocCompT;


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
    ClIocCompT *pComp;
    ClBoolT activeBind;
    ClOsalCondT unblockCond;
    ClOsalCondT recvUnblockCond;
    ClOsalMutexT unblockMutex;
    ClInt32T blocked;
    ClUint32T priority;
}ClIocCommPortT;




extern ClIocConfigT pAllConfig;
extern ClIocNodeAddressT gIocLocalBladeAddress;
extern ClUint32T
clIocSetType(ClUint32T portId, ClBoolT setFlag);
extern ClRcT 
__iocUserFragmentReceive(const ClCharT *xportType,
                         ClUint8T *pBuffer,
                         ClIocFragHeaderT *userHdr,
                         ClIocPortT portId,
                         ClUint32T length,
                         ClBufferHandleT msg,
                         ClBoolT sync);

extern ClRcT 
clIocCommPortCreateStatic(ClUint32T portId, ClIocCommPortFlagsT portType,
                          ClIocCommPortT *pIocCommPort, const ClCharT *xportType);

extern ClRcT 
clIocCommPortDeleteStatic(ClIocCommPortT *pIocCommPort, const ClCharT *xportType);

#ifdef __cplusplus
}
#endif

#endif
