/*
 * copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
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
#ifndef __CL_TIPC_H__
#define __CL_TIPC_H__

# ifdef __cplusplus
extern "C" {
# endif

ClRcT xportClose(void);
ClRcT xportFdGet(ClIocCommPortHandleT commPort, ClInt32T *fd);
ClRcT clTipcFdGet(ClIocPortT port, ClInt32T *fd);
ClRcT xportListen(ClIocPortT port);
ClUint32T clTipcSetType(ClUint32T portId, ClBoolT setFlag);
ClRcT xportBindClose(ClIocPortT port);
ClUint32T clTipcMcastType(ClUint32T type);
ClRcT xportListenStop(ClIocPortT port);
ClRcT xportNotifyInit(void );
ClRcT xportNotifyOpen(ClIocPortT port);
ClUint32T clTipcOwnAddrGet(void);
#ifdef BCAST_SOCKET_NEEDED
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress, ClIocAddressT *pDestAddress, ClUint32T *pSendFDFlag);
#else
ClRcT clTipcGetAddress(struct sockaddr_tipc *pAddress, ClIocAddressT *pDestAddress);
#endif
ClRcT xportNotifyClose(ClIocNodeAddressT nodeAddress, ClIocPortT port);
ClRcT xportServerReady(ClIocAddressT *pAddress);
ClRcT xportNotifyFinalize(void );
ClRcT xportMasterAddressGet(ClIocLogicalAddressT logicalAddress, ClIocPortT portId, ClIocNodeAddressT *pIocNodeAddress);
ClRcT xportMaxPayloadSizeGet(ClUint32T *pSize);
ClRcT xportMulticastRegister(ClIocPortT port, ClIocMulticastAddressT mcastAddr);
ClRcT xportMulticastDeregister(ClIocPortT port, ClIocMulticastAddressT mcastAddr);
ClRcT xportTransparencyRegister(ClIocPortT port, ClIocLogicalAddressT logicalAddr, ClUint32T haState);
ClRcT xportTransparencyDeregister(ClIocPortT port, ClIocLogicalAddressT logicalAddr);
ClRcT xportBind(ClIocPortT port);
ClRcT xportInit(const ClCharT *xportType, ClInt32T xportId, ClBoolT nodeRep);
ClRcT xportRecv(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption, ClUint8T *pBuffer, ClUint32T bufSize,
                ClBufferHandleT message, ClIocRecvParamT *pRecvParam);
ClRcT xportSend(ClIocPortT port, ClUint32T tempPriority, ClIocAddressT *pIocAddress,
                struct iovec *target, ClUint32T targetVectors, ClInt32T flags);

#ifdef __cplusplus
  }
#endif

#endif
