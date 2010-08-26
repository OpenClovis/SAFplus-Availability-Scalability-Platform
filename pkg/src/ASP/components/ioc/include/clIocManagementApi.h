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
/*******************************************************************************
 * ModuleName  : ioc
 * File        : clIocManagementApi.h
 *******************************************************************************/

#ifndef _CL_IOC_MANAGEMENT_API_H_
# define _CL_IOC_MANAGEMENT_API_H_


# include <clCommon.h>
# include <clBufferApi.h>
# include <clIocTransportApi.h>
# include <clIocApi.h>


# ifdef __cplusplus
extern "C"
{
# endif


# define CL_IOC_ROUTE_FLAGS_SAME_LINK 0xff

# define CL_IOC_ROUTE_UP 1

# define CL_IOC_ROUTE_DOWN 0


# define CL_IOC_MIN_RECV_Q_SIZE      (32*1024)

# define CL_IOC_MAX_RECV_Q_SIZE    (1024*1024)


# define CL_IOC_STATIC_ENTRY    1

# define CL_IOC_DYNAMIC_ENTRY 0


    typedef struct ClIocRouteParam
    {
        ClIocNodeAddressT destAddr;
        ClIocNodeAddressT nextHop;
        ClUint16T prefixLen;
        ClUint16T metrics;
        ClCharT *pXportName;
        ClCharT *pLinkName;
        ClUint8T flags;
        ClUint8T version;
        ClUint8T status;
        ClUint8T entryType;
    } ClIocRouteParamT;



    typedef struct ClIocArpParam
    {
        ClIocNodeAddressT iocAddr;
        ClUint8T *pTransportAddr;
        ClUint32T addrSize;
        ClCharT *pXportName;
        ClCharT *pLinkName;
        ClUint8T status;
        ClUint8T entryType;
    } ClIocArpParamT;

    ClRcT clIocCommPortWaterMarksGet(CL_IN ClUint32T commPort,
            CL_OUT ClUint64T* pLowWaterMark,
            CL_OUT ClUint64T* pHighWaterMark) CL_DEPRECATED;
    ClRcT clIocCommPortWaterMarksSet(CL_IN ClUint32T commPort,
            CL_IN ClUint32T lowWaterMark,
            CL_IN ClUint32T highWaterMark) CL_DEPRECATED;
    ClRcT clIocRouteInsert(CL_IN ClIocRouteParamT*pRouteInfo) CL_DEPRECATED;
    ClRcT clIocRouteDelete(CL_IN ClIocNodeAddressT destAddr,
            CL_IN ClUint16T  prefixLen) CL_DEPRECATED;
    ClRcT clIocRouteTablePrint( void) CL_DEPRECATED;
    ClRcT clIocArpInsert(CL_IN ClIocArpParamT *pArpInfo) CL_DEPRECATED;
    ClRcT clIocArpDelete(CL_IN ClIocNodeAddressT iocAddr,
            CL_IN ClCharT *pXportName,
            CL_IN ClCharT* pLinkName) CL_DEPRECATED;
    ClRcT clIocArpTablePrint(void) CL_DEPRECATED;
    ClRcT clIocLinkStatusGet(CL_IN ClCharT *pXportName,
            CL_IN ClCharT* pLinkName,
            CL_OUT ClUint8T *pStatus) CL_DEPRECATED;
    ClRcT clIocLinkStatusSet(CL_IN ClCharT *pXportName,
            CL_IN ClCharT* pLinkName,
            CL_IN ClUint8T status) CL_DEPRECATED;
    ClRcT clIocHeartBeatStart(void) CL_DEPRECATED;
    ClRcT clIocHeartBeatStop(void) CL_DEPRECATED;
    ClRcT clIocCommPortQueueSizeSet(ClIocCommPortHandleT portId,ClUint32T queueSize) CL_DEPRECATED;
    ClRcT clIocCommPortQueueStatsGet(ClIocCommPortHandleT portId,
            ClIocQueueStatsT *pQueueStats) CL_DEPRECATED;
    ClRcT clIocNodeQueueWaterMarksSet(ClIocQueueIdT queueId,ClWaterMarkT *pWM) CL_DEPRECATED;
    ClRcT clIocNodeQueueSizeSet(ClIocQueueIdT queueId,ClUint32T queueSize) CL_DEPRECATED;
    ClRcT clIocNodeQueueStatsGet(ClIocQueueStatsT *pSendQStats,
            ClIocQueueStatsT *pRecvQStats
            ) CL_DEPRECATED;
# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_MANAGEMENT_API_H_ */
