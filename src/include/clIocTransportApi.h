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
 * File        : clIocTransportApi.h
 *******************************************************************************/

#ifndef _CL_IOC_TRANSPORT_API_H_
# define _CL_IOC_TRANSPORT_API_H_

# ifdef __cplusplus
extern "C"
{
# endif

# include <clCommon6.h>
# include <clIocApi.h>
# include <clTimerApi6.h>
# include <clBufferApi6.h>
//# include <clCntApi6.h>


# define CL_TRANS_STAT_INC(value, incr) ((value) = (value) + (incr))
# define CL_IOC_DEF_MAX_ALLOWED_XPORTS (1+4)


# define CL_IOC_MAX_XPORT_STRING_LENGTH   127
# define CL_IOC_MAX_XPORT_ADDR_SIZE       CL_IOC_MAX_XPORT_STRING_LENGTH
# define CL_IOC_MAX_XPORT_NAME_LENGTH     CL_IOC_MAX_XPORT_STRING_LENGTH
# define CL_IOC_MIN_MTU_SIZE              256


    typedef struct ClIocTransportStats
    {
        ClUint32T sendMsgs;
        ClUint32T recvMsgs;
        ClUint32T sendBytes;
        ClUint32T recvBytes;
        ClUint32T badMsgs;
        ClUint32T dropMsgs;
    } ClIocTransportStatsT;

    typedef struct ClIocTransportLinkConfig ClIocTransportLinkConfigT;

    typedef ClRcT( *ClIocCoreFuncT) ( ClBufferHandleT, ClIocTransportLinkConfigT *, ClUint8T *);

    typedef ClRcT( *ClIocTransportFuncT) ( ClIocTransportLinkConfigT *);

    typedef ClRcT( *ClIocTransportSendFuncT) ( ClBufferHandleT, ClIocTransportLinkConfigT *, ClUint8T *);

    typedef ClRcT( *ClIocTransportAddrConvertFuncT) ( ClUint8T * pTransportAddress, ClUint8T * pIocArpAddrBytes);

    typedef ClRcT( *ClIocGroupCreateFuncT) ( ClIocAddressT *, ClUint8T *);

    typedef ClRcT( *ClIocGroupJoinFuncT) ( ClUint8T *);

    typedef ClRcT( *ClIocGroupLeaveFuncT) ( ClUint8T *);

    typedef ClRcT( *ClIocGroupDeleteFuncT) ( ClUint8T *);



    struct ClIocTransportLinkConfig
    {
        ClCharT pXportName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
        ClCharT pXportLinkName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
        ClUint8T xportType;
        ClUint8T isChecksumReqd;
        ClUint8T addressSize;
        ClUint8T isBcastSupported;
        ClUint8T xportBcastAddress[CL_IOC_MAX_XPORT_ADDR_SIZE + 1];
        ClUint8T xportAddress[CL_IOC_MAX_XPORT_ADDR_SIZE + 1];
        ClUint32T mtuSize;
        ClIocTransportStatsT *pIocXportStats;
        ClIocCoreFuncT iocCoreRecvRoutine;
        ClUint8T priority;
        ClUint8T isRegistered;
        void *pXportLinkPrivData;
        ClUint8T status;
    };


    typedef struct ClIocTransportConfig
    {
        ClUint8T version;
        ClCharT pXportName[CL_IOC_MAX_XPORT_NAME_LENGTH + 1];
        ClUint8T priority;
        ClUint8T xportType;
        ClIocTransportFuncT initRoutine;
        ClIocTransportSendFuncT sendRoutine;
        ClIocTransportFuncT closeRoutine;
        ClIocTransportAddrConvertFuncT addrConvertRoutine;
        ClIocTransportAddrConvertFuncT addrExtractRoutine;
    } ClIocTransportConfigT;


    ClRcT clIocTransportRegister(CL_IN ClIocTransportConfigT *pXportObjConfig) CL_DEPRECATED;
    ClRcT clIocTransportDeregister(CL_IN ClCharT *pXportName) CL_DEPRECATED;
    ClRcT clIocLinkRegister( CL_IN ClIocTransportLinkConfigT* pXportLinkConfig) CL_DEPRECATED;
    ClRcT clIocLinkDeregister(CL_IN ClCharT *pXportLinkName, CL_IN ClCharT *pXportName) CL_DEPRECATED;

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_TRANSPORT_API_H_ */
