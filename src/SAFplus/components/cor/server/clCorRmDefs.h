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
 * ModuleName  : cor
 * File        : clCorRmDefs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Route Manager API's & Definitions
 *
 *
 *****************************************************************************/

#ifndef _INC_ROUTE_MGR_H_
#define _INC_ROUTE_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @pkg cl.cor.rm */

/* INCLUDES */
#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCorServiceId.h>
#include <clCorClient.h>
#include <clBufferApi.h>
#include <clList.h>
#include <clRbTree.h>
#include <clHash.h>
/* Internal Headers*/
#include "clCorHash.h"

/* #include <clCorDmDefs.h> */  /* todo: to be removed */

/* DEFINES */
#define COR_RM_BUF_SZ       (1024 * 63)

/* The following to be replaced by list */
#define COR_MAX_RM_CONFIG     128
#define COR_MAX_ROUTES        64

#define COR_IS_DISABLED(hdr)      ((((hdr).flags)&((ClUint16T)0x8000))==0x8000)
#define COR_DISABLE(hdr)          (hdr).flags|=(ClUint16T)(0x8000)
#define COR_ENABLE(hdr)           (hdr).flags&=(ClUint16T)(0x7FFF)

#define STREAM_IN_COMM(buf,comm)                                        \
do {                                                                    \
  STREAM_IN(buf, &((comm).addr), sizeof((comm).addr));                  \
  STREAM_IN(buf, &((comm).timeout), sizeof((comm).timeout));            \
  STREAM_IN(buf, &((comm).maxRetries), sizeof((comm).maxRetries));      \
  STREAM_IN(buf, &((comm).maxSessions), sizeof((comm).maxSessions));    \
} while(0)

#define STREAM_OUT_COMM(comm,buf)                                       \
do {                                                                    \
  STREAM_OUT(&((comm).addr), buf, sizeof((comm).addr));                 \
  STREAM_OUT(&((comm).timeout), buf, sizeof((comm).timeout));           \
  STREAM_OUT(&((comm).maxRetries), buf, sizeof((comm).maxRetries));     \
  STREAM_OUT(&((comm).maxSessions), buf, sizeof((comm).maxSessions));   \
} while(0)

#define STREAM_IN_COMM_HTON(buf,comm, tmpL, tmpS)                     \
do {                                                                    \
  STREAM_IN_HTONL(buf, &((comm).addr.nodeAddress), tmpL, sizeof((comm).addr.nodeAddress));                  \
  STREAM_IN_HTONL(buf, &((comm).addr.portId), tmpL, sizeof((comm).addr.portId));                  \
  STREAM_IN_HTONL(buf, &((comm).timeout), tmpL, sizeof((comm).timeout));            \
  STREAM_IN_HTONS(buf, &((comm).maxRetries), tmpS, sizeof((comm).maxRetries));      \
  STREAM_IN_HTONS(buf, &((comm).maxSessions), tmpS, sizeof((comm).maxSessions));    \
} while(0)

#define STREAM_OUT_COMM_NTOH(comm,buf)                                       \
do {                                                                    \
  STREAM_OUT_NTOHL(&((comm).addr.nodeAddress), buf, sizeof((comm).addr.nodeAddress));                 \
  STREAM_OUT_NTOHL(&((comm).addr.portId), buf, sizeof((comm).addr.portId));                 \
  STREAM_OUT_NTOHL(&((comm).timeout), buf, sizeof((comm).timeout));           \
  STREAM_OUT_NTOHS(&((comm).maxRetries), buf, sizeof((comm).maxRetries));     \
  STREAM_OUT_NTOHS(&((comm).maxSessions), buf, sizeof((comm).maxSessions));   \
} while(0)

#define COR_ADDR_IS_EQUAL(a,b) \
  (((a).nodeAddress == (b).nodeAddress) && ((a).portId == (b).portId))
#define COR_ADDR_IS_EMPTY(a)  (((a).nodeAddress == 0) && ((a).portId == 0))


#define COR_COMM_PRINT(comm) \
    clOsalPrintf(" [%04x:%04x %xms,retry:%d,%x]", \
           comm.addr.nodeAddress, \
           comm.addr.portId, \
           comm.timeout, \
           comm.maxRetries, \
           comm.maxSessions)
  

/* Forward declaration */
struct CORVector;


/* TYPEDEFS */

typedef ClUint32T  CORFingerPrint_t; /* for time being put the card id here */

/**
 * flags  (MSB-LSB)
 *   16'th bit - 0 = enabled, 1 = disabled
 *   15'th bit - 1 = local storage present, 0 = no local storage
 *   14-1 bits - unused
 */
struct CORInfo 
{
  ClCorCommInfoT     comm;                         /**< COR comm details */
  CORFingerPrint_t  info;                         /**< finger print */
  ClUint32T        lastContactTime;              /**< Last Contacted on */
  ClUint16T        flags;
};

typedef struct CORInfo  CORInfo_t;
typedef CORInfo_t*      CORInfo_h;
struct RouteInfo;
struct CORStation
{
    ClIocNodeAddressT  nodeAddress;
    ClIocPortT         portId;
    ClInt8T            status;
    ClInt8T            isPrimaryOI;
    ClVersionT         version;
    struct RouteInfo   *rtInfo;
};


typedef struct CORStation CORStation_t;

struct CORAppStation
{
    ClIocNodeAddressT nodeAddress;
    ClIocPortT portId;
    struct hashStruct hash;
    ClRbTreeRootT resourceTree;
};

struct CORAppResource
{
    struct RouteInfo *rtInfo;
    CORStation_t *station;
    ClRbTreeT tree;
};

typedef struct CORAppStation CORAppStation_t;
typedef struct CORAppResource CORAppResource_t;
typedef ClIocNodeAddressT  ClCorCpuIdT;

typedef struct ClCorInfoList
{
    ClCorCommInfoT** pSyncList;
    ClUint32T* pCount;
}ClCorInfoListT;

typedef ClCorInfoListT* ClCorInfoListPtrT;

#define COR_RM_ADD_CARD(id, ioc)  if(cardMap!=0) HASH_PUT(cardMap,(id), (ioc))
#define COR_RM_GET_IOC(cid, ioc)  if(cardMap) HASH_GET(cardMap,(cid), ioc); else ioc=0

extern CORHashTable_h  cardMap;


/* prototypes to be removed later */

ClRcT  rmInit(ClCorCpuIdT id, ClIocPhysicalAddressT myIocAddr);
void rmFinalize(void);
ClRcT  rmRouteAdd(ClCorMOIdPtrT moh, ClCorAddrT pid, ClInt8T status);
ClRcT  rmRouteRuleStatusSet(ClCorMOIdPtrT moh, ClUint8T status);
ClRcT  rmFlagsSet(ClCorMOIdPtrT moh, ClUint16T flags);
ClUint16T rmFlagsGet(ClCorMOIdPtrT moh);
ClRcT  rmRouteGet(ClCorMOIdPtrT moh, ClCorCommInfoT* routes, ClUint32T* cnt);
ClRcT  rmRouteCreate(ClCorMOIdPtrT moh, int* idx);
int     rmRouteMatchGet(ClCorMOIdPtrT moh, ClInt32T idxStart);
ClUint32T  rmRoutePack(ClBufferHandleT *pRmBufferH);
ClRcT  rmRouteUnpack(void * buf, ClUint32T size);

/* Cor Route List station Address Related internal apis */
ClRcT  _clCorRmRouteStationStatusOp(corRouteApiInfo_t *pRt);
ClRcT  _clCorRmRouteStationDeleteAll(corRouteApiInfo_t *pRt);

void    rmShow(char**, ClBufferHandleT *pMsgHdl);

ClRcT corListInit(ClCorCpuIdT id, ClIocPhysicalAddressT myIocAddr);
void corListFinalize();
ClRcT corListAdd(ClCorCpuIdT id, ClIocPhysicalAddressT ioc, CORFingerPrint_t inf);  
ClRcT corListDel(ClCorCpuIdT id);  
ClRcT corListCorEnable(ClCorAddrT id);
ClRcT corListCorDisable(ClCorAddrT id);
ClRcT corListIdsGet(ClCorCommInfoT** ppSyncList, ClUint32T* sz);
ClRcT _clCorMoIdToComponentAddressGet(ClCorMOIdPtrT moh, ClCorAddrT* addr);
	
CORInfo_h corListGet(ClCorAddrT id);
ClUint32T corListPack(void * buf);
ClRcT  corListUnpack(void * buf, ClUint32T size);  
ClRcT  corListShow();
ClRcT _clCorStationEnable(SaNameT *compName, ClCorAddrT *stationAdd);
ClRcT _clCorStationDisable(ClCorAddrT stationAdd);
/* In case of Node Shutdown Remove the Peer COR from COR-List and the stations from the Route List */
ClRcT _clCorRemoveStations(ClIocNodeAddressT nodeAddr);

/* Primary OI related definitions */
/**
 * Internal function for registering the component in the OI list.
 */
ClRcT _clCorPrimaryOISet(corRouteApiInfo_t *pRouteInfo);

/**
 * Internal function for deregistering the component form the OI-list.
 */
ClRcT _clCorPrimaryOIClear(corRouteApiInfo_t *pRouteInfo);

/**
 * Internal function to get the primary OI from the OI - list.
 */
ClRcT _clCorPrimaryOIGet(ClCorMOIdPtrT moh, ClCorAddrT *pid);

ClRcT _corRouteRequest(corRouteApiInfo_t *pRt, ClBufferHandleT inMsgHandle, ClBufferHandleT outMsgHandle);

#ifdef __cplusplus
}
#endif

#endif  /*  _INC_DATA_MGR_H_ */
