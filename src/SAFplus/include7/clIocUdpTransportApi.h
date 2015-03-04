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
/*******************************************************************************
 * ModuleName  : ioc
 * File        : clIocUdpTransportApi.h
 *******************************************************************************/


#ifndef _CL_IOC_UDP_TRANSPORT_API_H_
# define _CL_IOC_UDP_TRANSPORT_API_H_
# ifdef __cplusplus
extern "C"
{
# endif

# include <clIocTransportApi.h>

# define CL_IOC_UDP_XPORT_NAME "UDP"

#define CL_IOC_IP_ADDRESS_LENGTH     15

#define CL_IOC_MCAST_ADDRESS_LENGTH  CL_IOC_IP_ADDRESS_LENGTH

# define CL_IOC_UDP_MTU_SIZE (32*1024)




    typedef struct
    {
        ClIocNodeAddressT slotNum;
        ClCharT pInterfaceAddress[CL_IOC_MAX_XPORT_ADDR_SIZE + 1];
    } ClIocLocationInfoT;


    typedef struct ClIocLinkCfg
    {

        ClCharT pName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
        ClUint8T priority;
        ClCharT pInterface[CL_IOC_MAX_XPORT_ADDR_SIZE + 1];
        ClUint8T pMcastAddress[CL_IOC_MCAST_ADDRESS_LENGTH + 1];
        ClUint32T mtuSize;
        ClBoolT isCksumSupported;
        ClBoolT isMcastSupported;
        ClUint32T numOfNodes;
        ClIocLocationInfoT *pNode;
    } ClIocUserLinkCfgT;


    typedef struct ClIocXportConfig
    {
        ClCharT pName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
        ClUint8T priority;
        ClUint32T id;
        ClUint32T numOfLinks;
        ClIocUserLinkCfgT *pLink;
    } ClIocUserTransportConfigT;




    ClRcT clIocUdpXportConfigInitialize(
            CL_IN ClIocUserTransportConfigT *pXportConfig) CL_DEPRECATED;


    ClRcT clIocTcpXportConfigInitialize(
            CL_IN ClIocUserTransportConfigT * pXportConfig
            ) CL_DEPRECATED;


    ClRcT clIocUdpXportFinalize(
            void
            ) CL_DEPRECATED;

    ClRcT clIocTcpXportFinalize(
            void
            ) CL_DEPRECATED;

# ifdef __cplusplus
}
# endif
#endif                          /* _CL_IOC_UDP_TRANSPORT_API_H_ */
