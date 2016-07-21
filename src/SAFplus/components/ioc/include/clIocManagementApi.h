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
 * File        : clIocManagementApi.h
 *******************************************************************************/

#ifndef _CL_IOC_MANAGEMENT_API_H_
# define _CL_IOC_MANAGEMENT_API_H_


#include <clCommon.h>
#include <clBufferApi.h>
#include <clIocTransportApi.h>
#include <clIocApi.h>
#include <clHash.h>
#include <clList.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define CL_IOC_ROUTE_FLAGS_SAME_LINK 0xff

#define CL_IOC_ROUTE_UP 1

#define CL_IOC_ROUTE_DOWN 0


#define CL_IOC_MIN_RECV_Q_SIZE      (32*1024)

#define CL_IOC_MAX_RECV_Q_SIZE    (1024*1024)


#define CL_IOC_STATIC_ENTRY    1

#define CL_IOC_DYNAMIC_ENTRY 0

/*
 * This type using for general ClIocNodeAddressT and ClIocPortT
 */
typedef ClUint32T ClIocHeartBeatLinkIndex;

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

/* Heart-beat start global defined */
typedef struct ClIocHeartBeatStatus
{
    ClIocHeartBeatLinkIndex linkIndex;
    ClUint8T status;
    ClUint8T retryCount;
    struct hashStruct hash; /*hash linkage*/
    ClListHeadT list; /*list linkage*/
} ClIocHeartBeatStatusT;


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
ClRcT clIocHeartBeatStart(void);
ClRcT clIocHeartBeatStop(void);
ClRcT clIocCommPortQueueSizeSet(ClIocCommPortHandleT portId,ClUint32T queueSize) CL_DEPRECATED;
ClRcT clIocCommPortQueueStatsGet(ClIocCommPortHandleT portId,
        ClIocQueueStatsT *pQueueStats) CL_DEPRECATED;
ClRcT clIocNodeQueueWaterMarksSet(ClIocQueueIdT queueId,ClWaterMarkT *pWM) CL_DEPRECATED;
ClRcT clIocNodeQueueSizeSet(ClIocQueueIdT queueId,ClUint32T queueSize) CL_DEPRECATED;
ClRcT clIocNodeQueueStatsGet(ClIocQueueStatsT *pSendQStats,
        ClIocQueueStatsT *pRecvQStats
        ) CL_DEPRECATED;

ClRcT clIocHearBeatHealthCheckUpdate(ClIocNodeAddressT nodeAddr, ClUint32T portId, ClCharT *message);
ClRcT clIocHeartBeatInitialize(ClBoolT nodeRep);
ClRcT clIocHeartBeatFinalize(ClBoolT nodeRep);

ClRcT clIocHeartBeatMessageReqRep(ClIocCommPortHandleT commPort, ClIocAddressT *destAddress, ClUint32T reqRep, ClBoolT shutdown);

ClRcT clIocHeartBeatPause(void);

ClRcT clIocHeartBeatUnpause(void);

ClBoolT clIocHeartBeatIsRunning(void);
const ClCharT *clIocHeartBeatStatusGet(void);

# ifdef __cplusplus
}
# endif

#endif                          /* _CL_IOC_MANAGEMENT_API_H_ */
