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
 * ModuleName  : cor
 * File        : clCorRmMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains Route related functions
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <netinet/in.h>
#include <clCksmApi.h>
/* Internal Headers*/
#include "clCorRt.h"
#include "clCorRmDefs.h" 
#include "clCorTreeDefs.h" 
#include "clCorUtilsProtoType.h" 
#include "clCorClient.h"
#include "clCorLog.h"
#include "clCorDeltaSave.h"
#include "clCorPvt.h"
#include "clCorEO.h"
#include "clCorNiLocal.h"
#include <xdrCorRouteApiInfo_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif
#define COR_RM_ROUTE_PER_BLK   8   /* Some random number for now*/
#define COR_RM_STNS_PER_BLK    8   /* Some random number for now*/

#define COR_RM_ROUTE_STATION_BUCKET_BITS (10)
#define COR_RM_ROUTE_STATION_BUCKETS ( 1 << COR_RM_ROUTE_STATION_BUCKET_BITS )
#define COR_RM_ROUTE_STATION_BUCKET_MASK ( COR_RM_ROUTE_STATION_BUCKETS - 1)

static struct hashStruct *gClCorRouteStationTable[COR_RM_ROUTE_STATION_BUCKETS];

static ClInt32T corRouteInfoCompare(ClRbTreeT *, ClRbTreeT *);
static CL_RBTREE_DECLARE(gClCorRouteInfoTree, corRouteInfoCompare);
ClCorCpuIdT   gMasterCpuId;

extern ClRcT _clCorMoIdToMoIdNameGet(ClCorMOIdPtrT moIdh,  ClNameT *moIdName);

CORVector_t  routeVector;
ClUint32T   nRouteConfig=0;

static  ClRcT rmSyncListGet(ClCorCpuIdT cpuId,
              ClCorObjFlagsT  flags,
			  ClCorCommInfoT **pSyncList,
       		  ClInt32T *pSz);

/* Routine to initialise the route vector */
static ClRcT  corRouteInit();

/* Route Vector Init */ 
ClRcT  corRouteStnInit(RouteInfo_t    *pRt);

/* Internal APIs */
static ClRcT  _routeInfoPack(RouteInfo_h pRt, void ** cookie);
static ClRcT  _routeInfoUnPack(void **cookie);
static ClRcT  _corStationPack(CORStation_t *element, void ** cookie);
static ClRcT _clCorStationEntryCheck(ClCorCommInfoT* stations, 
                ClInt32T count, CORStation_t* newStation);
ClOsalMutexIdT gCorRouteInfoMutex;
extern _ClCorServerMutexT gCorMutexes;
extern ClCorSyncStateT pCorSyncState;
/** 
 * Route Manager Init Routine.
 *
 * API to initialise route manager and its internal data
 * structures. Adds current IOC address to the cor list.
 *
 * @param 
 * 
 * @returns 
 *
 *
 */

static ClInt32T corRouteInfoCompare(ClRbTreeT *node1, ClRbTreeT *node2)
{
    RouteInfo_t *rt1 = CL_RBTREE_ENTRY(node1, RouteInfo_t, tree);
    RouteInfo_t *rt2 = CL_RBTREE_ENTRY(node2, RouteInfo_t, tree);
    return clCorMoIdSortCompare(rt1->moH, rt2->moH);
}

static __inline__ void corRouteInfoTreeAdd(RouteInfo_t *rtInfo)
{
    clRbTreeInsert(&gClCorRouteInfoTree, &rtInfo->tree);
}

static __inline__ void corRouteInfoTreeDel(RouteInfo_t *rtInfo)
{
    clRbTreeDelete(&gClCorRouteInfoTree, &rtInfo->tree);
}

static RouteInfo_t *corRouteInfoGet(ClCorMOIdT *moId)
{
    RouteInfo_t route = {0};
    ClRbTreeT *node = NULL;
    route.moH = moId;
    node = clRbTreeFind(&gClCorRouteInfoTree, &route.tree);
    if(!node) return NULL;
    return CL_RBTREE_ENTRY(node, RouteInfo_t, tree);
}

static void corRouteInfoDestroy(void)
{
    ClRbTreeT *iter = NULL;
    while ( (iter = clRbTreeMin(&gClCorRouteInfoTree) ) )
    {
        clRbTreeDelete(&gClCorRouteInfoTree, iter);
    }
}

static void corAppStationDestroy(void)
{
    struct hashStruct *iter;
    register ClInt32T i;
    for(i = 0; i < COR_RM_ROUTE_STATION_BUCKETS; ++i)
    {
        struct hashStruct *next = NULL;
        for( iter = gClCorRouteStationTable[i]; iter; iter = next)
        {
            CORAppStation_t *appStation = hashEntry(iter, CORAppStation_t, hash);
            next = iter->pNext;
            clHeapFree(appStation);
        }
    }
    memset(gClCorRouteStationTable, 0, sizeof(gClCorRouteStationTable));
}

static __inline__ ClUint32T corAppStationHashKey(ClIocNodeAddressT nodeAddress,
                                                 ClIocPortT portId)
{
    ClUint32T hashKey = 0;
    ClUint32T words[2] = {nodeAddress, portId};
    clCksm32bitCompute((ClUint8T*)words, sizeof(words), &hashKey);
    hashKey &= COR_RM_ROUTE_STATION_BUCKET_MASK;
    return hashKey;
}

static ClInt32T corAppResourceCompare(ClRbTreeT *node1, ClRbTreeT *node2)
{
    CORAppResource_t *res1 = CL_RBTREE_ENTRY(node1, CORAppResource_t, tree);
    CORAppResource_t *res2 = CL_RBTREE_ENTRY(node2, CORAppResource_t, tree);
    ClInt32T cmp = 0;
    cmp = clCorMoIdSortCompare(res1->rtInfo->moH, res2->rtInfo->moH);
    if(cmp) return cmp;
    /*
     * Now compare on the station address: node/port
     */
    if(res1->station->nodeAddress != res2->station->nodeAddress)
        return res1->station->nodeAddress - res2->station->nodeAddress;
    return res1->station->portId - res2->station->portId;
}

static CORAppStation_t *corAppStationGet(ClIocNodeAddressT nodeAddress,
                                         ClIocPortT portId)
{
    struct hashStruct *iter = NULL;
    ClUint32T hashKey = corAppStationHashKey(nodeAddress, portId);
    for(iter = gClCorRouteStationTable[hashKey]; iter; iter = iter->pNext)
    {
        CORAppStation_t *appStation = hashEntry(iter, CORAppStation_t, hash);
        if(appStation->nodeAddress == nodeAddress 
           &&
           appStation->portId == portId)
        {
            return appStation;
        }
    }
    return NULL;
}

static CORAppResource_t *corAppStationResourceGet(ClIocNodeAddressT nodeAddress,
                                                  ClIocPortT portId,
                                                  ClCorMOIdT *pMoid,
                                                  CORAppStation_t **pAppStation)
{
    CORAppStation_t *appStation = NULL;
    CORAppResource_t resource;
    ClRbTreeT *node = NULL;
    RouteInfo_t rtInfo = {0};
    CORStation_t station = {0};
    appStation = corAppStationGet(nodeAddress, portId);
    if(!appStation) return NULL;
    rtInfo.moH = pMoid;
    resource.rtInfo = &rtInfo;
    resource.station = &station;
    station.nodeAddress = nodeAddress;
    station.portId = portId;
    node = clRbTreeFind(&appStation->resourceTree, &resource.tree);
    if(!node) return NULL;
    if(pAppStation) *pAppStation = appStation;
    return CL_RBTREE_ENTRY(node, CORAppResource_t, tree);
}

static __inline__ ClRcT _corAppStationResourceDel(CORAppStation_t *appStation,
                                                  CORAppResource_t *appResource)
{
    if(!appStation || !appResource)
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    clRbTreeDelete(&appStation->resourceTree, &appResource->tree);
    clHeapFree(appResource);
    return CL_OK;
}

ClRcT corAppStationResourceDel(ClIocNodeAddressT nodeAddress,
                               ClIocPortT portId,
                               ClCorMOIdT *pMoid)
{
    CORAppStation_t *appStation = NULL;
    CORAppResource_t *appResource = NULL;
    appResource = corAppStationResourceGet(nodeAddress, portId, pMoid, &appStation);
    if(!appResource) return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    return _corAppStationResourceDel(appStation, appResource);
}

static ClRcT _corAppStationDel(CORAppStation_t *appStation)
{
    ClRbTreeT *iter = NULL;
    if(!appStation) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    while ( (iter = clRbTreeMin(&appStation->resourceTree)) )
    {
        CORAppResource_t *appResource = CL_RBTREE_ENTRY(iter, CORAppResource_t, tree);
        clRbTreeDelete(&appStation->resourceTree, iter);
        clHeapFree(appResource);
    }
    hashDel(&appStation->hash);
    clHeapFree(appStation);
    return CL_OK;
}

ClRcT corAppStationDel(ClIocNodeAddressT nodeAddress, ClIocPortT portId)
{
    CORAppStation_t *appStation = NULL;
    appStation = corAppStationGet(nodeAddress, portId);
    if(!appStation) return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    return _corAppStationDel(appStation);
}

static void _corAppStationAdd(ClIocNodeAddressT nodeAddress,
                              ClIocPortT portId,
                              CORStation_t *station, 
                              RouteInfo_t *rtInfo)
{
    ClUint32T hashKey = 0;
    CORAppStation_t *appStation = NULL;
    CORAppResource_t *appResource = NULL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];
    if( !(appStation = corAppStationGet(nodeAddress, portId)) )
    {
        appStation = clHeapCalloc(1, sizeof(*appStation));
        CL_ASSERT(appStation != NULL);
        appStation->nodeAddress = nodeAddress;
        appStation->portId = portId;
        hashKey = corAppStationHashKey(nodeAddress, portId);
        hashAdd(gClCorRouteStationTable, hashKey, &appStation->hash);
        clRbTreeInit(&appStation->resourceTree, corAppResourceCompare);
    }
    else
    {
        /*
         * Check if the resource is already in the app station.
         */
        CORAppResource_t resource;
        resource.rtInfo = rtInfo;
        resource.station = station;
        if(clRbTreeFind(&appStation->resourceTree, &resource.tree))
            return ;
    }
    appResource = clHeapCalloc(1, sizeof(*appResource));
    CL_ASSERT(appResource != NULL);
    appResource->rtInfo = rtInfo;
    appResource->station = station;
    clRbTreeInsert(&appStation->resourceTree, &appResource->tree);
    clLogTrace("APP", "STATION", "Resource [%s] added to app [%#x: %#x]",
               _clCorMoIdStrGet(rtInfo->moH, moIdName), nodeAddress, portId);
}

static void corAppStationAdd(CORStation_t *station, 
                             RouteInfo_t *rtInfo)
{
    _corAppStationAdd(station->nodeAddress, station->portId, station, rtInfo);
    /*
     * Add an entry for the node.
     */
    _corAppStationAdd(station->nodeAddress, 0, station, rtInfo);
}

ClRcT
rmInit(ClCorCpuIdT id, ClIocPhysicalAddressT myIocAddr)
{
    ClRcT     rc = CL_OK;

    CL_COR_FUNC_ENTER("RMR", "RMI");
    nRouteConfig=0;

#ifdef DEBUG
    /* Add to debug agent */
    dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
#endif
    corListInit(id, myIocAddr);

    /* Initialise the Route structure */
    if (CL_OK != (rc = corRouteInit()))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to initalise Route Module"));
        return rc; /* Pass on the same error */
    }
    
	if((rc = clOsalMutexCreate(&gCorRouteInfoMutex)) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Init Route Mutex. rc [0x%x]",rc));
		return rc;
	}

    CL_COR_FUNC_EXIT("RMR", "RMI");
    return (CL_OK);
}

/* This function deletes the memory allocated for 
route manager */
void rmFinalize(void)
{
	ClUint32T index;
	RouteInfo_t  *pRt = NULL;

	/* Delete entries from COR list */
	corListFinalize();
    corRouteInfoDestroy();
    corAppStationDestroy();

	/* Cleanup the route list */
    for(index=0 ; index < nRouteConfig ; index++)
    {
       if (NULL == ( pRt = (RouteInfo_t *)corVectorGet(&routeVector, index)))
       {
           break;   /* No entry found */
       }
	   clCorMoIdFree(pRt->moH);
       clRuleExprDeallocate(pRt->exprH);
	   corVectorRemoveAll(&pRt->stnVector);
	}
	corVectorRemoveAll(&routeVector);

	if(clOsalMutexDelete(gCorRouteInfoMutex) != CL_OK)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Delete Route Mutex."));
		return;
	}
}

ClRcT _clCorStationEntryCheck(ClCorCommInfoT* stations, ClInt32T count, CORStation_t* newStation)
{
    ClInt32T i = 0;

    CL_FUNC_ENTER();

    for (i=0; i<count; i++)
    {
        if ((stations[i].addr.nodeAddress == newStation->nodeAddress) &&
                (stations[i].addr.portId == newStation->portId))
        {
            /* Station already exists */
            clLogInfo("RMR", "RLT", "The station is already added to the list.");
            CL_FUNC_EXIT();
            return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);
        }
    }

    /* Station entry is not found */
    CL_FUNC_EXIT();
    return (CL_OK);
}

/** 
 * Returns the list of routes for a given ClCorMOId.
 *
 * API to return back the list of stations that needs to be contacted
 * before the cor operation can be committed. This list also includes
 * the syncup list, so that data is in synch in all the CORs that are
 * running in the system (that have the same data) at any given time.
 *
 * @param moh      MO Id Handle
 * @param stations [out] station list
 * @param count    [in/out] no of stations
 * 
 * @returns 
 * 
 * @todo
 *   Yet to figure out if there is only forward entry and
 *   NOT all cor's need to be synced up with the data.
 *
 */

ClRcT 
rmRouteGet (ClCorMOIdPtrT moh,
            ClCorCommInfoT *stations,
            ClUint32T *count)
{
    ClCorCommInfoT* syncList = NULL;
    ClCorCpuIdT     cpuId =  0;
    ClCorObjFlagsT  flags = CL_COR_OBJ_FLAGS_DEFAULT;
    ClInt32T            sz = 0;
    ClInt32T            i  = 0;
    ClInt32T            idx= 0;
    ClInt32T            j  = 0;
    ClRcT               rc = CL_OK;

    CL_COR_FUNC_ENTER("RMR", "RGT");
    
    if( (NULL == moh) || (NULL == count) || (NULL == stations) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    sz = 0; /* No. of stations */

    /* first step is to get the route list and 
     * add all the stations to the st.
     */
    idx = -1;

    while ((idx = rmRouteMatchGet(moh, idx+1)) >= 0)
    {
        RouteInfo_t   *pRt = NULL;
        
        /* copy the routes to the stations */
        pRt = (RouteInfo_t *) corVectorGet(&routeVector, idx);
        if (pRt != NULL)
		{
			sz = sz + pRt->nStations;
        
			/* check if we have enough space, otherwise return back */
			if(sz > *count)
			{
				CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                        ( "Insufficient buffer passed. Size of the buffer : [%d]; No. of route entries found : [%d]."
                          "Couldn't copy all the route lists", *count, sz));
				return CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER);
			}
        
			for(j = 0; j < pRt->nStations; j++)
			{
				CORStation_t    *pStn= NULL;
    
				if (NULL != (pStn =(CORStation_t *)corVectorGet(&pRt->stnVector,j)))
				{
                    if((pStn->status == CL_COR_STATION_DISABLE) || (pStn->status == CL_COR_STATION_DELETE))
                    {
                        /* skip - it. The entry has been disabled */
                        continue;
                    }
                    else
                    {
                        /* Function to check whether the station is already there */
                        rc = _clCorStationEntryCheck(stations, i, pStn);
                        if (rc != CL_COR_SET_RC(CL_COR_ERR_DUPLICATE))
                        {
                            stations[i].addr.nodeAddress = pStn->nodeAddress;
                            stations[i].addr.portId = pStn->portId;
                            stations[i].timeout = CL_COR_DEFAULT_TIMEOUT;
                            stations[i].maxRetries  = CL_COR_DEFAULT_MAX_RETRIES;
                            stations[i].maxSessions = CL_COR_DEFAULT_MAX_SESSIONS;
                    
                            /* increment station count */
                            i++;
                        }
                    }
				}
			}
        
			/* get the flag associated with the moid */
			flags = pRt->flags;
		}
	}
    
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("No. of routes configured for given object : %d", sz));

    /* This function will allocate memory for syncList. 
     * It needs to be freed after the usage. 
     */
    rc = rmSyncListGet (cpuId, flags, &syncList, &sz);
    if (rc != CL_OK)
    {
        clHeapFree(syncList);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the COR List. rc [0x%x]", rc));
        return rc;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("No. of entries found in the COR list : [%d]\n", sz));
 
    if((i+sz) >= *count)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                ( "Insufficient buffer passed. Size of the buffer : [%d]; No. of route entries found : [%d]."
                  "Couldn't copy all the route lists", *count, (i+1+sz)));
        return CL_COR_SET_RC(CL_COR_ERR_INSUFFICIENT_BUFFER);
    }
    
    /*
     * Add to the stuff thats already to station list. Eliminate the
     * COR/IOC Addresses. 
     */
    for(idx=0; syncList && idx < sz && i<(*count); idx++)
    {
            stations[i] = syncList[idx];
            i++;
    }
      
    /* Free the memory allocated by rmSyncListGet() function */
    clHeapFree(syncList);
   
    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "No. of stations found : %d.", i));
    *count = i;
 
    CL_COR_FUNC_EXIT("RMR", "RGT");
    return (CL_OK);
}


/** 
 * Get the COR sync list.
 *
 * API to get the sync list for an moId based on the cpu id
 * and the flags associated with it.
 *
 * @param cpuId     cpuId associated with the ClCorMOId.
 * @param flags     flags associated with the ClCorMOId.
 * @param pSyncList [out] COR sync list.
 * @param pSz       [out] no of CORs in the sync list.
 * 
 * @returns Pointer to the sync list and size (out param).
 *
 */
ClRcT
rmSyncListGet(ClCorCpuIdT cpuId,
              ClCorObjFlagsT  flags,
			  ClCorCommInfoT **pSyncList,
			  ClInt32T *pSz)
{
    /*
    ClInt32T           i  = 0;
    ClInt32T            stIdx = 0;
    */
    
	ClUint32T       sz = 0;
    ClCorCommInfoT* syncList = NULL;
    ClRcT           rc  = CL_OK;

    /*
	 * Get list of all the active COR's.
     */
    rc = corListIdsGet(&syncList, &sz);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to get the COR list. rc [0x%x]\n", rc));
        return rc;
    }
                                
#ifdef DISTRIBUTED_COR
	/* Process the sync list based on the cache flags for the moId */
	switch (flags & CL_COR_OBJ_CACHE_MASK)
	{
	    case CL_COR_OBJ_CACHE_ON_MASTER:
			for (i = 0; i < sz; i++)
			{
                if ((cpuId == syncList[i].addr.nodeAddress) ||
				    (gMasterCpuId == syncList[i].addr.nodeAddress))
				{
				    syncList[stIdx++] = syncList[i];
				}
			}
			sz = stIdx;
			/*
			 * Note that it is possible that the LOCAL and the
			 * MASTER may be same. In that case stIdx will be one.
			 */
            CL_ASSERT ((sz != 0) && (sz < 3));			
			if ((sz == 0) || (sz > 2))
			{
 				CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cannot find local OR master COR list lCpuId 0x%x gCpuId 0x%x",
				                   cpuId, gMasterCpuId));
			    return (CL_COR_SET_RC(CL_COR_INTERNAL_ERR_INVALID_COR_LIST));
			}
		    break;
	    case CL_COR_OBJ_CACHE_LOCAL:
		    /*
			 * keep only the local COR which is same 
			 * as the one with cpuId.
			 */
			for (i = 0; i < sz; i++)
			{
                if(cpuId == syncList[i].addr.nodeAddress)
				{
				    syncList[stIdx++] = syncList[i];
					break;
				}
			}
			sz = stIdx;

            CL_ASSERT (sz == 1);
            
			if (sz != 1)
			{
 				CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cannot find local COR list lCpuId 0x%x",
				                   cpuId));
			    return (CL_COR_SET_RC(CL_COR_INTERNAL_ERR_INVALID_COR_LIST));
			}
		    break;
	    case CL_COR_OBJ_CACHE_GLOBAL:
		    /*
			 * Nothing to be done as all the CORs need
			 * to be in the sync list
			 */
		    break;
	    case CL_COR_OBJ_CACHE_ONLY_ON_MASTER:
		    /*
			 * keep only the master COR 
			 */
			for (i = 0; i < sz; i++)
			{
                if(gMasterCpuId == syncList[i].addr.nodeAddress)
				{
				    syncList[stIdx++] = syncList[i];
					break;
				}
			}
			sz = stIdx;
			
            CL_ASSERT (sz == 1);
            
			if (sz != 1)
			{
 				CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cannot find master COR list gCpuId 0x%x",
				                   gMasterCpuId));
			    return (CL_COR_SET_RC(CL_COR_INTERNAL_ERR_INVALID_COR_LIST));
			}
		    break;
	    default:
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid cache flag 0x%x", flags));
		    return (CL_COR_SET_RC(CL_COR_INTERNAL_ERR_INVALID_RM_FLAGS));
	}
#endif

	*pSz = sz;
	*pSyncList = syncList;
	return (CL_OK);
}

/** 
 * Add station.
 *
 * API to add route to route manager.
 *
 * @param 
 * 
 * @returns 
 *
 * @todo
 *   Need to fix the route from array to list
 *
 *
 */
ClRcT
rmRouteAdd(ClCorMOIdPtrT moh, ClCorAddrT pid, ClInt8T status)
{
    ClRcT       ret = CL_OK;
    ClInt32T i;
    ClCharT moIdStr[CL_MAX_NAME_LENGTH] = {0};

    CL_COR_FUNC_ENTER("RMR", "RAD");
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    ret = clCorMoIdValidate(moh);
    if(CL_OK != ret)
    {
        clLogError("RMR", "RAD", 
                "Failed while validating the moId passed for adding in the route list."
                " Please check the MoId. rc[0x%x]", ret);
        return ret;
    }

    clLogTrace("RMR", "RAD", "Adding the route entry [0x%x:0x%x] for MO [%s] with status as [%d]",
            pid.nodeAddress, pid.portId, _clCorMoIdStrGet(moh, moIdStr), status);

    ret = rmRouteCreate(moh, &i);
    if(ret == CL_OK) 
    {
         RouteInfo_t   *pRt = NULL;
         CORStation_t  *pStn = NULL;
         if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, i)))
         {
            clLogError("RMR", "RAD", "Failed to create Route for the MO[%s]", 
                    moIdStr);
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
         }
        /* just add it to the station list */
        if(pRt->nStations >= COR_MAX_ROUTES) 
        {
            clLogError("RMR", "RAD", 
                    "Too many routes [%d] for given handle, rejecting!", pRt->nStations);
            return CL_COR_SET_RC(CL_COR_ERR_MAX_DEPTH);
        }

        clLogTrace("RMR", "RAD", "The number of route entries for MO [%s] are [%d]",
                moIdStr, pRt->nStations);

        /* Look if the CORStation_t is already added for the resource */

        for(i = 0; i < pRt->nStations; i++)
        {
          pStn = corVectorGet(&pRt->stnVector, i);
          if(pStn->nodeAddress == pid.nodeAddress &&
             pStn->portId == pid.portId)
           { 
              /* we already have a entry for this station.*/
               pStn->status = status;
               /*
                * Add an entry to the app station cache if not present.
                */
               corAppStationAdd(pStn, pRt);
               return (CL_OK);
           }

        }
        if (NULL == (pStn = (CORStation_t *)corVectorExtendedGet(
                                               &pRt->stnVector,pRt->nStations)))
        {
            clLogError( "RMR", "RAD", "Failed to create route station for MO[%s]", moIdStr);
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        pStn->nodeAddress = pid.nodeAddress;
        pStn->portId = pid.portId;
        pStn->status = status;
        pStn->isPrimaryOI = CL_COR_PRIMARY_OI_DISABLED;

        pStn->version.releaseCode = CL_RELEASE_VERSION;
        pStn->version.majorVersion = CL_MAJOR_VERSION;
        pStn->version.minorVersion = CL_MINOR_VERSION;
        /*
         * Add the resource to the app station cache.
         */
        corAppStationAdd(pStn, pRt);
        pRt->nStations++;
    }

    CL_COR_FUNC_EXIT("RMR", "RAD");
    return (ret);
}

/** 
 * This Api perform the operations(set) on the status of the station for a given service id.
 *
 * @param pRtApiInfo [IN] : Pointer to the structure corRouteApiInfo_t.  This structure has
 *                          service Id which will identify the route list. This also has the
 *                          station address which will be deleted from the route list.
 * 
 * @returns CL_OK : When the station set passes.
 *          CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT): When the invalid moId or the station 
                                          address passed is invalid.
 */
ClRcT
_clCorRmRouteStationDeleteAll(corRouteApiInfo_t *pRtApiInfo)
{
    ClRcT       ret = CL_OK;
    ClInt32T   routeIndex = -1;
    ClUint32T  stnIndex = 0;
    RouteInfo_t    *pRt = NULL;
    ClCorAddrT  stationAddr = pRtApiInfo->addr;

    CL_COR_FUNC_ENTER("RMD", "RDA");

    for (routeIndex = 0; routeIndex < nRouteConfig ; routeIndex++) 
    {                                                                                                               
        if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, routeIndex)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Internal Error: Route Info not found for the index. %d", routeIndex));
            CL_ASSERT(pRt != NULL);
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

        if(pRt->moH->svcId != pRtApiInfo->srvcId)
            continue;

        for(stnIndex = 0; stnIndex < pRt->nStations; stnIndex++)
        {
            CORStation_t  *pStn = NULL;
            
            if (NULL == (pStn =(CORStation_t *)corVectorGet(&pRt->stnVector, stnIndex)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nStation vector is NULL.\n"));
                break;
            }

            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Operate on station[0x%x:0x%x] and Op %d, stn status %d", 
                                stationAddr.nodeAddress, stationAddr.portId, pRtApiInfo->reqType, pStn->status));
            if((pStn->nodeAddress == stationAddr.nodeAddress) && (pStn->portId == stationAddr.portId)
                    && (pStn->status != CL_COR_STATION_DELETE))
            {
                
                pStn->status = pRtApiInfo->status;
                if(CL_COR_PRIMARY_OI_ENABLED == pStn->isPrimaryOI)
                    pRt->nPrimaryOI = 0;
                pStn->isPrimaryOI = pRtApiInfo->primaryOI;
                ret = clCorDeltaDbRouteInfoStore(*pRt->moH, *pRtApiInfo, CL_COR_DELTA_ROUTE_DELETE);
                if(ret != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Delete the record from DB. rc[0x%x]",ret));
            }
        }
    }

	CL_COR_FUNC_EXIT("RMR", "RDA");
	return ret;

}

/** 
 * This Api perform the operations(get/set) on the status of the station.
 *
 * @param pRtApiInfo [IN/OUT] : Pointer to the structure corRouteApiInfo_t.  This structure
 *                              is obtained from the EO api _corRouteApi. So for the operation
 *                              involving the status of the station in the route list, this api
 *                              is called.
 * 
 * @returns CL_OK : When the station set passes.
 *          CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT): When the invalid moId or the station 
                                          address passed is invalid.
 */

static ClRcT
_corRmRouteStationStatusOp(corRouteApiInfo_t *pRtApiInfo,
                           RouteInfo_t *pRt,
                           CORStation_t *pStn)
{
    ClRcT ret = CL_OK;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];

    moIdName[0] = 0;

    switch(pRtApiInfo->reqType)
    {
    case COR_SVC_RULE_STATION_DISABLE:
        {
            if(pStn->status != CL_COR_STATION_DELETE)
            {
                /* Removing the station from the MoId */
                clLogInfo("RMR", "RSO", "Disabling the OI [0x%x:0x%x] from the MoId [%s]", 
                          pStn->nodeAddress, pStn->portId, 
                          _clCorMoIdStrGet(&(pRtApiInfo->moId), moIdName));

                pStn->status = pRtApiInfo->status;
                ret = clCorDeltaDbRouteInfoStore(
                                                 pRtApiInfo->moId, *pRtApiInfo, CL_COR_DELTA_ROUTE_STATUS_SET);
                if(ret != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("Failed to Set the status in DB. rc[0x%x]",ret));

                return (CL_OK);
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Can not disable this station address in the route list; "
                                "since it is already disabled"));
                return (CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT));
            }
        }
        break;
    case COR_SVC_RULE_STATION_DELETE:
        {   
            if(pStn->status != CL_COR_STATION_DELETE)
            {
                /* Removing the station from the MoId */
                clLogInfo("RMR", "RSO", "Deleting the OI [0x%x:0x%x] from the MoId [%s]", 
                          pStn->nodeAddress, pStn->portId,
                          _clCorMoIdStrGet(&(pRtApiInfo->moId), moIdName));

                /* If deleting the primary OI then making the number of primary OI as zero.*/
                if(pStn->isPrimaryOI == CL_COR_PRIMARY_OI_ENABLED)
                    pRt->nPrimaryOI = 0;

                pStn->isPrimaryOI = pRtApiInfo->primaryOI;
                pStn->status = pRtApiInfo->status; 

                ret = clCorDeltaDbRouteInfoStore(*pRt->moH, *pRtApiInfo, CL_COR_DELTA_ROUTE_DELETE);
                if(ret != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to remove Route Entry from DB. rc[0x%x]",ret));
                return (CL_OK);
            }
            else
            {
                /* 
                 * Making the debug type from ERROR to WARN. 
                 *  1. PROV client adds itself as OI for MOID*(wildcarded)
                 *  2. MOs are created from CLI.
                 *  3. Prov client dies. In such a case Prov client
                 *     tries to delete MOID1, MOID2.
                 *  4. The route list contains MOID as MOID* (wildcarded).
                 *  5. The deletion for MOID2 will report the error that route has been deleted.
                 *  6. This is misleading, and hence making the error temporarily as WARN.
                 */
                CL_DEBUG_PRINT(CL_DEBUG_WARN,
                               ("Route entry is already deleted. rc [0x%x]", 
                                CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT)));
                CL_COR_FUNC_EXIT("RMR", "RSO");
                return (CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT));
            }
        }
        break;
    case COR_SVC_RULE_STATION_STATUS_GET:
        {   
            pRtApiInfo->status = pStn->status;
            CL_COR_FUNC_EXIT("RMR", "RSO");
            return CL_OK;
        }
        break;
    default:
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nInvalid option for the route station's status\n"));
        break;
    }
        
	CL_COR_FUNC_EXIT("RMR", "RSO");
    return (CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT));

}

ClRcT
_clCorRmRouteStationStatusOp(corRouteApiInfo_t *pRtApiInfo)
{
    RouteInfo_t    *pRt = NULL;
    CORStation_t  *pStn = NULL;
    CORAppResource_t *appResource = NULL;
    ClCorAddrT  stationAddr = pRtApiInfo->addr;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];

    CL_COR_FUNC_ENTER("RMR", "RSO");

    moIdName[0] = 0;

    /* Get the cor route info from the cache.
     * Delete the OI from this exact match. If the exact match is not found
     * then return errror 
     */
    pRt = corRouteInfoGet(&pRtApiInfo->moId);
    if(!pRt)
    {
        clLogError("RM", "STATUS", "Route info not found for moid [%s]",
                   _clCorMoIdStrGet(&pRtApiInfo->moId, moIdName));
        return CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT);
    }

    appResource = corAppStationResourceGet(stationAddr.nodeAddress, stationAddr.portId, &pRtApiInfo->moId,  NULL);
    if(!appResource || !(pStn = appResource->station))
    {
        clLogError("RM", "STATUS", "Route station entry not found for moid [%s], node [%#x], port [%#x]",
                   _clCorMoIdStrGet(&pRtApiInfo->moId, moIdName),
                   stationAddr.nodeAddress, stationAddr.portId);
        return CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT);
    }


    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Operate on station[0x%x:0x%x] and Op %d, stn status %d", 
                                     stationAddr.nodeAddress, stationAddr.portId, pRtApiInfo->reqType, pStn->status));

    return _corRmRouteStationStatusOp(pRtApiInfo, pRt, pStn);
}



/**
 * Add flags to an ClCorMOId.
 *
 * API to add flags to an MO/MSO
 *
 * @param 
 * 
 * @returns 
 *
 * @todo
 *
 */
ClRcT
rmFlagsSet(ClCorMOIdPtrT moh, ClUint16T flags)
{
    ClRcT       ret = CL_OK;
    ClInt32T i;

    CL_FUNC_ENTER();
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* check the validity of the flags */
	if ((flags & ~CL_COR_OBJ_FLAGS_ALL) ||
	    ((flags & CL_COR_OBJ_CACHE_MASK) > CL_COR_OBJ_CACHE_MAX))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Invalid flags"));
        return CL_COR_SET_RC(CL_COR_SVC_ERR_INVALID_FLAGS);
    }
    
    ret = rmRouteCreate(moh, &i);
    if(ret == CL_OK) 
    {
        RouteInfo_t    *pRt = NULL;
		CORObject_h dmh = NULL;
		ObjTreeNode_h obj = NULL;

		ClCorMOServiceIdT svcId;
      
        if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, i))) 
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Create route "));
            return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
        }

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "set flags %04x to route %d", flags, i));
        /* just set the flags here */
        pRt->flags = flags;

		svcId = moh->svcId;
        obj = corMOObjGet(moh);
        if(obj){
            if(svcId != CL_COR_INVALID_SRVC_ID)
            {
                dmh = corMSOObjGet(obj, svcId);
                if(NULL == dmh)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Cound not get MSO Object"));
                    /*Route flag is set, so currently we return with OK*/
                    return CL_OK;
                }
            }
            else
            {
                    dmh = obj->data;
            }

			/* Need to verify if this flag is there, do we need the flag in route info*/
			dmh->flags = flags;
        }
        else
        {
        	CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Cound not get MO Object"));
        	/*Route flag is set, so currently we return with OK*/
        	return CL_OK;
        }
    }

    CL_FUNC_EXIT();
    return (ret);
}

/** 
 * Get flags associated with an ClCorMOId.
 *
 * API to Get flags from an MO/MSO
 *
 * @param 
 * 
 * @returns 
 *
 * @todo
 *
 */
ClUint16T
rmFlagsGet(ClCorMOIdPtrT moh)
{
    ClUint16T   f = CL_COR_OBJ_FLAGS_DEFAULT;
    ClInt32T i = -1;
    ClInt32T count = 0;

    CL_FUNC_ENTER();
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
    }

    /* Get the exact matching entry and return the flags */
    count = 0;
    i = -1;

    while ((i = rmRouteMatchGet(moh, i+1)) >= 0)
    {
        RouteInfo_t   *pRt = NULL;
       
        /* Route entry found */
        count = 1;

        if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, i)))
        {
           CL_DEBUG_PRINT(CL_DEBUG_WARN, ( "No match found for ClCorMOId, returning zero!"));
           return f;
        }

        if (clCorMoIdCompare(pRt->moH, moh) == 0)
        {
            f = pRt->flags;
            break;
        }
        else
        {
            f = pRt->flags;
        }
    }

    if (count == 0)
    {
        clLogTrace("RMR", "RFL", "Route information not found, returning the default flags value.");
    }

    CL_FUNC_EXIT();
    return (f);
}

/**
 * [private]
 * Route create.
 *
 * API to Create route entry if not already present and return back
 * the index.
 *
 * @param moh  MO Id Handle
 */
ClRcT
rmRouteCreate(ClCorMOIdPtrT moh, ClInt32T* idx)
{
    ClRuleExprT*    expr;
    ClRcT       ret = CL_OK;
    RouteInfo_t  *pRt = NULL;

    CL_COR_FUNC_ENTER("RMR", "RCT");
    
    if( ( NULL == moh ) || ( NULL == idx ) )
    {
        clLogError("RMR", "RCT", "The input parameter to the function is NULL. ");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* 
     * first verify and retrieve the config if already present
     */
    if( (pRt = corRouteInfoGet(moh) ) )
    {
        *idx = pRt->index;
        CL_COR_FUNC_EXIT("RMR", "RCT");
        return CL_OK;
    }

    /* no match, so need to add this as a new entry to the table */

    if (NULL == ( pRt = (RouteInfo_t *)corVectorExtendedGet(&routeVector,
                                                            nRouteConfig)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to extend the vector!"));
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
   
    /* create the rbe expression */
    ret = corMOIdExprCreate(moh, &expr);
    if(ret == CL_OK) 
    {
        /* clone the ClCorMOId */
        ret = clCorMoIdClone(moh, &pRt->moH);
        if(ret == CL_OK) 
        {
            pRt->exprH = expr;

            /* init the stations */
            if (CL_OK != (ret = corRouteStnInit(pRt)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Station initialisation failed!"));
                return ret;
            }

            *idx = pRt->index = nRouteConfig++;

            /* Initailizing the no. of primary OIs*/
            pRt->nPrimaryOI = 0;
            pRt->flags = CL_COR_OBJ_FLAGS_DEFAULT;

            /* Include the version information */
            pRt->version.releaseCode = CL_RELEASE_VERSION;
            pRt->version.majorVersion = CL_MAJOR_VERSION;
            pRt->version.minorVersion = CL_MINOR_VERSION;
            /*
             * Add to the route info cache.
             */
            corRouteInfoTreeAdd(pRt);
        }
        else 
        {
            /* free the expression */
            clRuleExprDeallocate(expr);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "ClCorMOId clone failed!"));
        }
    }
  
    CL_COR_FUNC_EXIT("RMR", "RCT");
    return (ret);
}

/** 
 * [private]
 * Get Route Match.
 *
 * API to Get Route index that match the given ClCorMOId.  This routine
 * will try to do an exact match first if present, if not would narrow
 * down the match to the approximate close MO Id that matches it.
 *
 * @param moh        MO Id Handle
 * 
 * @returns 
 *
 * @todo   need to figure out the best match
 *
 */
ClInt32T
rmRouteMatchGet(ClCorMOIdPtrT moh, ClInt32T idxStart)
{
    ClInt32T        i = 0;
    RouteInfo_t     *pRt = NULL;

    CL_COR_FUNC_ENTER("RMR", "RMG");
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return -1;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Get Route"));

    i = idxStart;

    /* verify the following in the route configuration.
     *   a) look for an exact match, if present, then return back
     *      right over there
     *   b) find which is the closest match
     */
    for(; i<nRouteConfig; i++)
    {
        pRt = (RouteInfo_t *)corVectorGet(&routeVector, i);
        if (pRt == NULL)
        {
           return -1;
        }

        if(clCorMoIdCompare(moh, pRt->moH)==0)
        {
            /* just add it to the station list */
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Got an exact match!"));
            CL_COR_FUNC_EXIT("RMR", "RMG");
            return (i);
        }

        /* see if the rbe expr matches */
        if(clRuleExprEvaluate(pRt->exprH, 
                           (ClUint32T*) moh,
                           sizeof(ClCorMOIdT)) == CL_RULE_TRUE)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "Matched index %d", i));

            /* Return this best match */
            return (i);
        }
    }  

    CL_COR_FUNC_EXIT("RMR", "RMG");
    /* No match found */
    return (-1);
}

/** 
 * Pack all routes.
 *
 * API to pack all the route configuration defined. It takes
 * buffer-pointer and returns size of buffer used.
 *
 * @param           Pointer to buffer which will be packed.
 * 
 * @returns         Size of data packed/used
 *
 */
ClRcT
rmRoutePack(ClBufferHandleT *pBufH)
{
    ClRcT   rc = CL_OK;
    ClInt32T i = 0;
    RouteInfoStream_h   packInfo = NULL;
	ClUint32T tmpNRouteConfig = 0;
  
    CL_COR_FUNC_ENTER("RMR", "RPK");
    /*
       Scheme for packing :
       Packing for Route Configuration shall follow structure
       <routeVector.nRouteConfig>
       <RouteEntry-1.flags>
       <RouteEntry-1.moId>
       <RouteEntry-1.nStations>
       <RouteEntry-1.Station-1>
       ....
       <RouteEntry-1.Station-n>
       <RouteEntry-2.flags>
       ....

       Regular expression is not packed as it is derived from ClCorMOId
    */

    /* walk thru all the objects and dump them */
    
    if( NULL == pBufH )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    packInfo = clHeapAllocate(sizeof(RouteInfoStream_t));
    if(packInfo == NULL)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
	return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    
    packInfo->pBufH = pBufH;
    /* Update the pointer and remaining-size of packInfo */

    STREAM_IN_BOUNDS_HTONL(packInfo->pBufH, &(nRouteConfig), tmpNRouteConfig, sizeof(nRouteConfig));
    /* Since head is initialized with default entry, this check is required */
    for ( ; i < nRouteConfig; i++)
    {
        RouteInfo_h pRt;
        if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, i)))
        {
            break;
        }
        _routeInfoPack(pRt, (void **) &packInfo);
    }

    clHeapFree(packInfo);
    CL_COR_FUNC_EXIT("RMR", "RPK");
    return rc;
}

/** 
 * [private]
 * Pack a given RouteEntry
 *
 * This internal API shall pack contents of a RouteEntry into
 * the given buffer. 
 *
 * @param           Index of the RouteInfo_t in the route-vector
 * @param           Pointer to RouteInfo_t to be packed.
 * @param           Internal cookie (buffer)
 * 
 * @returns         Returns CL_OK on success
 */
ClRcT
_routeInfoPack(RouteInfo_h this, void ** cookie)
{
    ClRcT retCode = CL_OK;
    RouteInfoStream_h   context = (RouteInfoStream_h) *cookie;
	ClUint16T tmpPackS;
	ClUint32T tmpPackL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];
    CL_COR_FUNC_ENTER("RMR", "RIP");
    
    
    if ( ( NULL == this ) || ( NULL == context ) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL arguments"));
        retCode = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else 
    {
        ClInt32T i = 0;
        STREAM_IN_BOUNDS_HTONS(context->pBufH, &(this->flags), tmpPackS, sizeof( this->flags));
		for(i = 0; i< CL_COR_HANDLE_MAX_DEPTH; i++)
		{
        	STREAM_IN_BOUNDS_HTONL( context->pBufH, &(this->moH->node[i].type), tmpPackL, sizeof(ClCorClassTypeT));
        	STREAM_IN_BOUNDS_HTONL( context->pBufH, &(this->moH->node[i].instance), tmpPackL, sizeof(ClCorInstanceIdT));
		}
		
       	STREAM_IN_BOUNDS_HTONS(context->pBufH, &(this->moH->svcId), tmpPackS, sizeof(this->moH->svcId));
       	STREAM_IN_BOUNDS_HTONS(context->pBufH, &(this->moH->depth), tmpPackS, sizeof(this->moH->depth));
        clLogNotice("ROUTE", "PACK", "Packing moid [%s] with depth [%d]", 
                    _clCorMoIdStrGet(this->moH, moIdName),
                    this->moH->depth);
       	STREAM_IN_BOUNDS_HTONL(context->pBufH, &(this->moH->qualifier), tmpPackL, sizeof(this->moH->qualifier));
        STREAM_IN_BOUNDS_HTONS(context->pBufH, &(this->nStations), tmpPackS ,sizeof( this->nStations));
        STREAM_IN_BOUNDS_HTONS(context->pBufH, &(this->nPrimaryOI), tmpPackS ,sizeof( this->nPrimaryOI));
        STREAM_IN_BUFFER(context->pBufH, &(this->version.releaseCode), sizeof(this->version.releaseCode));
        STREAM_IN_BUFFER(context->pBufH, &(this->version.majorVersion), sizeof(this->version.majorVersion));
        STREAM_IN_BUFFER(context->pBufH, &(this->version.minorVersion), sizeof(this->version.minorVersion));

        for (i = 0; (i < this->nStations) && (retCode == CL_OK); i++)
        { 
            CORStation_t   *pStn; 
            if (NULL == (pStn = (CORStation_t *)corVectorGet(&this->stnVector, i)))
            {
                break;
            }
            retCode = _corStationPack(pStn, cookie);
        }
        if (CL_OK != retCode)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Error during packing stations"));
            return retCode;
        }
    }
    CL_COR_FUNC_EXIT("RMR", "RIP");
    return retCode;
}

/**
 * [private]
 * Pack routine for a given CORStation structure.
 *
 * @param       Index of the CORStation entry in the vector
 * @param       Pointer to CORStation definition
 * @param       Internal cookie pointer.
 *
 * @return      CL_OK on success
 */
ClRcT
_corStationPack(CORStation_t* stn, void ** cookie)
{
    ClRcT retCode = CL_OK;
    RouteInfoStream_h   context = (RouteInfoStream_h) *cookie;
	ClUint32T tmpPackL;

    CL_COR_FUNC_ENTER("RMR", "RSP");
    
    
    if ( (NULL == stn) || (NULL == context))
    {
        retCode = CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else
    {
         STREAM_IN_BOUNDS_HTONL(context->pBufH, &(stn->nodeAddress), tmpPackL, sizeof(ClIocNodeAddressT)); 
         STREAM_IN_BOUNDS_HTONL(context->pBufH, &(stn->portId), tmpPackL, sizeof(ClIocPortT)); 
         STREAM_IN_BUFFER(context->pBufH, &(stn->status), sizeof(stn->status)); 
         STREAM_IN_BUFFER(context->pBufH, &(stn->isPrimaryOI), sizeof(stn->isPrimaryOI)); 
        
         STREAM_IN_BUFFER(context->pBufH, &(stn->version.releaseCode), sizeof(stn->version.releaseCode));
         STREAM_IN_BUFFER(context->pBufH, &(stn->version.majorVersion), sizeof(stn->version.majorVersion));
         STREAM_IN_BUFFER(context->pBufH, &(stn->version.minorVersion), sizeof(stn->version.minorVersion));
    }
    CL_COR_FUNC_EXIT("RMR", "RSP");
    return retCode;
}

/**
 * Unpack routes from the buffer
 *
 * API to unpack route list from the buffer.
 *
 * @param       Pointer to buffer containing route-list
 * @param       Size of buffer
 *
 * @return      CL_OK on success
 */
ClRcT  
rmRouteUnpack(void * buf, ClUint32T size)
{
    ClRcT              retCode = CL_OK;
    RouteInfoStream_h  unPackInfo = NULL;
    ClUint32T          routeCfgCount;
    ClInt32T           i = 0;

    CL_COR_FUNC_ENTER("RMR", "RUP");
    
    if ( NULL == buf )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Null argument passed"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    unPackInfo = clHeapAllocate(sizeof(RouteInfoStream_t));
    if(unPackInfo == NULL)
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
	CL_DEBUG_PRINT(CL_DEBUG_ERROR,(CL_COR_ERR_STR(CL_COR_ERR_NO_MEM)));
	return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    unPackInfo->buf = (Byte_h) buf;
    unPackInfo->size = size;
    STREAM_OUT_BOUNDS_NTOHL( &(routeCfgCount), unPackInfo->buf, 
                       sizeof(routeCfgCount), unPackInfo->size);
    clLogInfo("ROUTE", "UNPACK", "Unpacking [%d] route entries of size [%d] bytes",
              routeCfgCount, unPackInfo->size);
    for (; (i < routeCfgCount) && (retCode == CL_OK) ; i++)
    {
        retCode = _routeInfoUnPack((void **) &unPackInfo);
    }
    if(buf)
    {
#ifdef DEBUG
        dmBinaryShow("Route Rule List", buf, size);
#endif
    }
    clHeapFree(unPackInfo);

    CL_COR_FUNC_EXIT("RMR", "RUP");
    return retCode;
}

/**
 * [private]
 * Unpack routine to extract single route-info entry from the buffer
 *
 * @param           Internal cookie 
 *
 * @return          CL_OK on success
 */
ClRcT
_routeInfoUnPack(void **cookie)
{
    ClRcT retCode = CL_OK;
    RouteInfoStream_h   context = (RouteInfoStream_h) *cookie;

    CL_COR_FUNC_ENTER("RMR", "RIU");
    if (NULL == context)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Null arguement"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    else
    {
        ClInt32T i = 0;
        RouteInfo_t         routeEntry;
        clCorMoIdAlloc(&(routeEntry.moH));

       STREAM_OUT_BOUNDS_NTOHS( &(routeEntry.flags), context->buf, 
                            sizeof(routeEntry.flags), context->size);
		for(; i <CL_COR_HANDLE_MAX_DEPTH; i++)
		{
        	STREAM_OUT_BOUNDS_NTOHL( &(routeEntry.moH->node[i].type), context->buf, sizeof(ClCorClassTypeT), context->size);
        	STREAM_OUT_BOUNDS_NTOHL( &(routeEntry.moH->node[i].instance), context->buf, sizeof(ClCorInstanceIdT), context->size);
		}

       	STREAM_OUT_BOUNDS_NTOHS( &(routeEntry.moH->svcId), context->buf, sizeof(routeEntry.moH->svcId), context->size);
       	STREAM_OUT_BOUNDS_NTOHS( &(routeEntry.moH->depth), context->buf, sizeof(routeEntry.moH->depth), context->size);
       	STREAM_OUT_BOUNDS_NTOHL( &(routeEntry.moH->qualifier), context->buf, sizeof(routeEntry.moH->qualifier), context->size);
        STREAM_OUT_BOUNDS_NTOHS( &(routeEntry.nStations), context->buf, sizeof(routeEntry.nStations), context->size); 
        STREAM_OUT_BOUNDS_NTOHS( &(routeEntry.nPrimaryOI), context->buf, sizeof(routeEntry.nStations), context->size); 

        STREAM_OUT_BOUNDS( &(routeEntry.version.releaseCode), context->buf, sizeof(routeEntry.version.releaseCode), context->size);
        STREAM_OUT_BOUNDS( &(routeEntry.version.majorVersion), context->buf, sizeof(routeEntry.version.majorVersion), context->size);
        STREAM_OUT_BOUNDS( &(routeEntry.version.minorVersion), context->buf, sizeof(routeEntry.version.minorVersion), context->size);

        for(i = 0; i < routeEntry.nStations; i++)
        {
            CORStation_t    stn;
		    ClCorAddrT      addr = {0};	
            STREAM_OUT_BOUNDS_NTOHL(&stn.nodeAddress, context->buf, sizeof(stn.nodeAddress), context->size); 
            STREAM_OUT_BOUNDS_NTOHL(&stn.portId, context->buf, sizeof(stn.portId), context->size); 
            STREAM_OUT_BOUNDS(&stn.status, context->buf, sizeof(stn.status), context->size); 
            STREAM_OUT_BOUNDS(&stn.isPrimaryOI, context->buf, sizeof(stn.isPrimaryOI), context->size); 

            STREAM_OUT_BOUNDS(&(stn.version.releaseCode), context->buf, sizeof(stn.version.releaseCode), context->size);
            STREAM_OUT_BOUNDS(&(stn.version.majorVersion), context->buf, sizeof(stn.version.majorVersion), context->size);
            STREAM_OUT_BOUNDS(&(stn.version.minorVersion), context->buf, sizeof(stn.version.minorVersion), context->size);
         
            addr.nodeAddress = stn.nodeAddress;
            addr.portId = stn.portId;
            retCode = rmRouteAdd(routeEntry.moH, addr, stn.status);
            if(retCode != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to add the station \
                                [0x%x:0x%x]. rc[0x%x]", addr.nodeAddress, addr.portId, retCode));
            if(CL_COR_PRIMARY_OI_ENABLED == stn.isPrimaryOI)
            {
                corRouteApiInfo_t routeInfo = {{0}};
                routeInfo.moId = *(routeEntry.moH);
                routeInfo.addr = addr;
                routeInfo.status = stn.status;
                routeInfo.primaryOI = stn.isPrimaryOI;
                retCode = _clCorPrimaryOISet(&routeInfo);
                if(retCode != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to add the station \
                                    [0x%x:0x%x]. rc[0x%x]", addr.nodeAddress, addr.portId, retCode));
            }
        }
        rmFlagsSet(routeEntry.moH, routeEntry.flags);
        clHeapFree(routeEntry.moH);
    }

    CL_COR_FUNC_EXIT("RMR", "RIU");
    return retCode;
}

/** 
 * Display route manager configuration.
 *
 * API to Display route manager configuration including, cor List
 * and route configuration rules.
 *
 *
 */
void
rmShow(char** params, ClBufferHandleT* pMsg)
{
    ClInt32T i;  
    ClInt32T j;
    ClCharT corStr[CL_COR_CLI_STR_LEN];
    ClUint16T nRoute = 0;


	clOsalMutexLock(gCorRouteInfoMutex);
    corListShow(pMsg);
  

    corStr[0]='\0';
    sprintf(corStr, "%15c -- Route Configuration (rules) --\n"
      "===========================================================\n",32);
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

    for(i=0;i<nRouteConfig;i++)
    {
        RouteInfo_t   *pRt = NULL;
        
        if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, i)))
        {
            break;
        }

        corStr[0]='\0';
        sprintf(corStr, "Rule %02d)",nRoute++);
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

        clCorMoIdGetInMsg(pRt->moH, pMsg);

        corStr[0]='\0';
        sprintf(corStr, " Expr:%p Flags:%04x Routes: ",
                 (void*)pRt->exprH,
                 pRt->flags);

        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));
        for(j=0; j< pRt->nStations; j++)
        {
            CORStation_t  *pStn = NULL;
            
            if (NULL == (pStn =(CORStation_t *)corVectorGet(&pRt->stnVector,j)))
            {
                break;
            }

            if((pStn->nodeAddress != 0) && (pStn->portId != 0))
            {
                corStr[0]='\0';
                sprintf(corStr,"[STATUS: %d, STN: %x:%x, PRIMARY:%d ] ",
                           pStn->status,
                           pStn->nodeAddress,
                           pStn->portId,
                           pStn->isPrimaryOI);
                clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                    strlen(corStr));
            }
        }
        corStr[0]='\0';
        sprintf(corStr,"\n");
        clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                    strlen(corStr));
    }
    corStr[0]='\0';
    sprintf(corStr,"===========================================================\n");
    clBufferNBytesWrite (*pMsg, (ClUint8T*)corStr,
                                strlen(corStr));

	clOsalMutexUnlock(gCorRouteInfoMutex);
}


/* 
 *   Route Vector Init
 */ 
ClRcT  corRouteInit()
{
   ClRcT   rc = CL_OK;
    
   /* Start with clean slate */
   memset(&routeVector, '\0', sizeof(routeVector));

   /* Initialise the vector */
   if (CL_OK != (rc = corVectorInit(&routeVector,
                                       sizeof(RouteInfo_t),
                                       COR_RM_ROUTE_PER_BLK)))
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to initalise Route vector"));
        return rc; /* Pass on the same error */
   }
   return CL_OK;
}

ClRcT _clCorMoIdToComponentAddressGet(ClCorMOIdPtrT moh, ClCorAddrT* addr)
{
	ClInt32T idx = 0;
    ClUint32T index = 0;
	RouteInfo_t* pRt = NULL;
	CORStation_t * component = 0;
    ClInt32T i = 0;

    idx = -1;
    for (i=0; i < nRouteConfig; i++)
    {
        pRt = (RouteInfo_t *) corVectorGet(&routeVector, i);
        if (pRt == NULL)
        {
            break;
        }

        if (clCorMoIdCompare(moh, pRt->moH) == 0)
        {
            /* Exact match, return this index */
            idx = i;
            break;
        }

        if (clRuleExprEvaluate(pRt->exprH,
                        (ClUint32T *) moh,
                        sizeof(ClCorMOIdT)) == CL_RULE_TRUE)
        {
            idx = i;
        }
    }

    if(idx >= 0)
    {        
        /* copy the routes to the stations */
        pRt = (RouteInfo_t *) corVectorGet(&routeVector, idx);
        
        if (pRt == NULL)
        {
            /* No entry found for the MoId in the Route Vector */
            CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, 
                    "\nNo entry found for the given MoId in the Route List.", CL_ERR_DOESNT_EXIST);
        }
    }
	else
	{
        /* No entry found the MoId in the Route Vector */
		CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, 
                "\nNo entry found for the given MoId in the Route List.", CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT));
	}

    if(pRt->nStations < 1)
	{
		/* No stations found. Just return error */
		CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, 
                "\nNo stations found for the given MoId", CL_ERR_DOESNT_EXIST);
	}

	/* Get the first station and return the address */
    for(index = 0; index < pRt->nStations; index++)
    {
	    component = corVectorGet(&pRt->stnVector, index);

	    if(NULL == component)
		{
		    CL_COR_RETURN_ERROR(CL_DEBUG_ERROR, 
                    "\nCould not get component address from the Route List", CL_ERR_DOESNT_EXIST);
		}

        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ("Station Info found : [0x%x:0x%x]", component->nodeAddress, component->portId));

        if(component &&  (component->status == CL_COR_STATION_DISABLE))
        { 
	        addr->nodeAddress = component->nodeAddress;
            addr->portId = component->portId;
            CL_FUNC_EXIT();
            return CL_OK;
        }
    }
	
    CL_FUNC_EXIT();
	return CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT);
}

/* 
 *   Route Vector Init
 */ 
ClRcT  corRouteStnInit(RouteInfo_t    *pRt)
{
   ClRcT   rc = CL_OK;

   /* Start with clean slate */
   memset(&pRt->stnVector, '\0', sizeof(CORVector_t));

   /* Initialise the vector */
   if (CL_OK != (rc = corVectorInit(&pRt->stnVector,
                                       sizeof(CORStation_t),
                                       COR_RM_STNS_PER_BLK)))
   {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "Failed to initalise Stattion vector"));
        return rc; /* Pass on the same error */
   }
    return rc;
}

ClRcT _clCorStationEnable(ClNameT *pCompName, ClCorAddrT *pStationAddr)
{
    ClRcT rc = CL_OK;
    ClOampRtResourceArrayT *pResourceArray = NULL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];
    ClBufferHandleT inMsgHandle = 0;
    register ClInt32T i;

    if(!(pResourceArray = corComponentResourceGet(pCompName)))
    {
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    }

    rc = clBufferCreate(&inMsgHandle);
    CL_ASSERT(rc == CL_OK);

    for(i = 0; i < pResourceArray->noOfResources; ++i)
    {
        ClCorMOIdT moid;
        ClCorMOClassPathT moClassPath;
        CORMOClass_h moClassHandle;
        CORMSOClass_h msoClassHandle;
        ClOampRtResourceT *pResource = pResourceArray->pResources + i;
        corRouteApiInfo_t routeInfo = {{0}}; 
        rc = corXlateMOPath((ClCharT*)pResource->resourceName.value, &moid);
        if(rc != CL_OK)
        {
            clLogError("STN", "ENABLE", "COR name to moid get for resource [%s] returned [%#x]",
                       pResource->resourceName.value, rc);
            continue;
        }
        clCorMoIdToMoClassPathGet(&moid, &moClassPath);
        rc = corMOClassHandleGet(&moClassPath, &moClassHandle);
        if(rc != CL_OK)
        {
            continue;
        }

        rc = corMSOClassHandleGet(moClassHandle, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
                                  &msoClassHandle);
        if(rc != CL_OK)
        {
            continue;
        }
        clCorMoIdServiceSet(&moid, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        CL_COR_VERSION_SET(routeInfo.version);
        routeInfo.reqType = COR_SVC_RULE_STATION_ENABLE;
        routeInfo.moId = moid;
        routeInfo.addr = *pStationAddr;
        routeInfo.status = CL_COR_STATION_ENABLE;
        if(i)
            clBufferClear(inMsgHandle);
        rc = VDECL_VER(clXdrMarshallcorRouteApiInfo_t, 4, 0, 0)(&routeInfo, inMsgHandle, 0);
        CL_ASSERT(rc == CL_OK);
        rc = _corRouteRequest(&routeInfo, inMsgHandle, 0);
        if(rc != CL_OK)
        {
            moIdName[0] = 0;
            clLogError("STN", "ENABLE", "Failed to add route for resource [%s], node [%#x], port [%#x]",
                       _clCorMoIdStrGet(&moid, moIdName), pStationAddr->nodeAddress, 
                       pStationAddr->portId);
                       
        }
    }

    clBufferDelete(&inMsgHandle);
    return rc;
}

/*
 *  Function to disable all the rules, which are defined for stationAdd
 */
ClRcT _clCorStationDisable(ClCorAddrT stationAdd)
{

    /* Disable Cor List */
    clOsalMutexLock(gCorRouteInfoMutex);
    if(stationAdd.portId == CL_IOC_COR_PORT)
    {
        corListCorDisable(stationAdd);
   
        /* In the case of COR standby going down reset the sycn state variable.*/
        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
        pCorSyncState = CL_COR_SYNC_STATE_INVALID; 
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
    }
    else
    {
        /* Look for all the stations in route list */
        RouteInfo_t   *pRt = NULL;
        CORStation_t  *pStn = NULL;
        CORAppStation_t *appStation =  NULL;
        ClRbTreeT *iter = NULL;

        appStation = corAppStationGet(stationAdd.nodeAddress, stationAdd.portId);
        if(!appStation)
        {
            clOsalMutexUnlock(gCorRouteInfoMutex);
            clLogError("STN", "DIS", "Station not found for node [%#x], port [%#x]",
                       stationAdd.nodeAddress, stationAdd.portId);
            return CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT);
        }
        /*
         * Walk through the resources in the app station
         */
        while ( (iter = clRbTreeMin(&appStation->resourceTree) ) )
        {
            CORAppResource_t *appResource = NULL;
            appResource = CL_RBTREE_ENTRY(iter, CORAppResource_t, tree);
            /*
             * Find the route entry for the resource.
             */
            pRt = appResource->rtInfo;
            pStn = appResource->station;
            CL_ASSERT(pStn 
                      && 
                      pStn->nodeAddress == stationAdd.nodeAddress 
                      && 
                      pStn->portId == stationAdd.portId);

            if (pStn->status != CL_COR_STATION_DELETE)
            { 
                corRouteApiInfo_t routeInfo;
                ClRcT           rc = CL_OK;

                routeInfo.moId = *pRt->moH;
                routeInfo.addr.nodeAddress = pStn->nodeAddress;
                routeInfo.addr.portId = pStn->portId;
                routeInfo.status = CL_COR_STATION_DELETE;
                routeInfo.reqType = COR_SVC_RULE_STATION_DELETE;
                routeInfo.primaryOI  = CL_COR_PRIMARY_OI_DISABLED; 
                rc = _corRmRouteStationStatusOp(&routeInfo, pRt, pStn);
                if(rc != CL_OK)
                {
                    ClNameT name = {0};
                    _clCorMoIdToMoIdNameGet(pRt->moH, &name);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Delete the Route Info for MoId [%s] and Station [0x%x:0x%x] rc[0x%x]", 
                                                   name.value, 
                                                   pStn->nodeAddress,
                                                   pStn->portId,
                                                   rc));
                }
            }
            _corAppStationResourceDel(appStation, appResource);
        }
        _corAppStationDel(appStation);
    }
    clOsalMutexUnlock(gCorRouteInfoMutex);
    return (CL_OK);
}

/*
 *  Function to disable all the rules, which are defined for stationAdd
 */
ClRcT _clCorRemoveStations(ClIocNodeAddressT nodeAddr)
{

    ClCorAddrT    stationAdd;
    RouteInfo_t   *pRt = NULL;
    CORStation_t  *pStn = NULL;
    CORAppStation_t *appStation = NULL;
    ClRbTreeT *iter = NULL;
    corRouteApiInfo_t routeInfo;
    ClRcT rc  = CL_OK;

    clOsalMutexLock(gCorRouteInfoMutex);
    /* Disable Cor List */
    stationAdd.portId = CL_IOC_COR_PORT;
    stationAdd.nodeAddress = nodeAddr;
    rc = corListCorDisable(stationAdd);
    if(rc == CL_OK)
    {
        /* In the case of COR standby going down reset the sycn state variable.*/
        clOsalMutexLock(gCorMutexes.gCorSyncStateMutex);
        pCorSyncState = CL_COR_SYNC_STATE_INVALID; 
        clOsalMutexUnlock(gCorMutexes.gCorSyncStateMutex);
    }

    /*
     * Get the station entry for this node. to get the node resource map.
     */
    appStation = corAppStationGet(nodeAddr, 0);

    if(!appStation)
    {
        clOsalMutexUnlock(gCorRouteInfoMutex);
        clLogError("REM", "STN", "Couldn't find route station entry for node [%#x]", nodeAddr);
        return CL_COR_SET_RC(CL_COR_ERR_ROUTE_NOT_PRESENT);
    }

    while ( (iter = clRbTreeMin(&appStation->resourceTree) ) )
    {
        CORAppResource_t *appResource = NULL;
        appResource = CL_RBTREE_ENTRY(iter, CORAppResource_t, tree);
        pRt = appResource->rtInfo;
        pStn = appResource->station;
        CL_ASSERT(pStn && pStn->nodeAddress == nodeAddr);
        clLogDebug("STN", "REM", "Processing station [%#x:%#x] with status [%d]", pStn->nodeAddress, pStn->portId,
                   pStn->status);
        if(pStn->status != CL_COR_STATION_DELETE)
        { 
            routeInfo.moId = *pRt->moH;
            routeInfo.addr.nodeAddress = pStn->nodeAddress;
            routeInfo.addr.portId = pStn->portId;
            routeInfo.status = CL_COR_STATION_DELETE;
            routeInfo.reqType = COR_SVC_RULE_STATION_DELETE;
            routeInfo.primaryOI  = CL_COR_PRIMARY_OI_DISABLED; 
            rc = _clCorRmRouteStationStatusOp(&routeInfo);
            if(rc != CL_OK)
            {
                ClNameT name = {0};
                _clCorMoIdToMoIdNameGet(pRt->moH, &name);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Delete the Route Info for MoId [%s] and Station [0x%x:0x%x] rc[0x%x]", 
                                               name.value, 
                                               pStn->nodeAddress,
                                               pStn->portId,
                                               rc));
            }
        }
        _corAppStationResourceDel(appStation, appResource);
    }
    _corAppStationDel(appStation);
    clOsalMutexUnlock(gCorRouteInfoMutex);
    return CL_OK;
}

/** 
 * Add OIs.
 *
 * API to add OI to read OI list.
 *
 * @param 
 * 
 * @returns 
 */

ClRcT
_clCorPrimaryOISet(corRouteApiInfo_t *pRouteInfo)
{
    ClRcT           rc = CL_OK;
    ClInt32T       index = 0;
    RouteInfo_t     *pRt = NULL;
    CORStation_t    *pStn = NULL;
    CORAppResource_t *appResource = NULL;
    ClCorMOIdPtrT   pMoId = NULL;
    ClCorAddrPtrT   pCompAddr = NULL;
    ClNameT         moIdName = {0};
    ClCharT         moName[CL_MAX_NAME_LENGTH];
    ClInt32T        count = 0;

    CL_COR_FUNC_ENTER("RMR","POS");
    
    /* Checking for the NULL */
    clDbgIfNullReturn ( pRouteInfo, CL_CID_COR );

    pMoId = &pRouteInfo->moId;
    pCompAddr = &pRouteInfo->addr;

    /* Checking for the NULL */
    clDbgIfNullReturn ( pMoId, CL_CID_COR );
    
    /* Checking for the NULL */
    clDbgIfNullReturn ( pCompAddr, CL_CID_COR );

    clLog ( CL_LOG_SEV_DEBUG, "RMR", "POS", "Setting the OI as primary OI. [0x%x: 0x%x]", 
            pCompAddr->nodeAddress, pCompAddr->portId );    

    /* Walk through the route list and check if primary OI is set for any moId matching with this
     * moId using wild-card match
     */
    for (index = 0; index < corVectorSizeGet(routeVector); index++)
    {
        pRt = ( RouteInfo_t * )corVectorGet(&routeVector, index);
        if ( NULL == pRt )
        {
            clLog ( CL_LOG_SEV_ERROR, "RMR", "POS", "Failed to get the route entry." );
            CL_COR_FUNC_EXIT("RMR", "POS");
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
        }

        if (clCorMoIdCompare(pRt->moH, pMoId) >= 0) /* If it is exact or wild card match */
        {
            /* Check if primary OI already exists */
            clLog ( CL_LOG_SEV_TRACE, "RMR", "POS", "Checking whether there is already a primary OI exists." );

            count++;

            /* If there is already a read OI registered.*/
            if(pRt->nPrimaryOI == 1)
            {
                clLog( CL_LOG_SEV_DEBUG,"RMR", "POS", " Primary OI exist. Checking if it is duplicate addition." );

                rc = _clCorPrimaryOIEntryCheck(pRt, pCompAddr, pMoId);
                if(rc != CL_OK)
                {
                    if ( _clCorMoIdToMoIdNameGet(pMoId, &moIdName) != CL_OK)
                        clLog ( CL_LOG_SEV_ERROR, "RMR", "POS", "The MO already have a primary OI. rc[0x%x]", rc );
                    else
                        clLog ( CL_LOG_SEV_ERROR, "RMR", "POS", 
                                "The MO already have a primary OI. [MO : %s]. rc[0x%x]", 
                                moIdName.value, rc );
                }    

                CL_COR_FUNC_EXIT("RMR", "POS");
                return rc;
            }
        }
    }

    if (count == 0)
    {
        clLog(CL_LOG_SEV_ERROR, "RMR", "POS", "The component address donot exist in the route list" );
        CL_FUNC_EXIT();
        return (CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED));    
    }

    clLog ( CL_LOG_SEV_DEBUG, "RMR", "POS", "No primary OI configured, so adding [0x%x:0x%x] as the Primary OI.",
            pCompAddr->nodeAddress, pCompAddr->portId );

    pRt = corRouteInfoGet(pMoId);
    if(!pRt)
    {
        moName[0] = 0;
        clLogError("RMR", "POS", "No route info found for resource [%s]",
                   _clCorMoIdStrGet(pMoId, moName));
        return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);    
    }
    
    appResource = corAppStationResourceGet(pCompAddr->nodeAddress, pCompAddr->portId, pMoId, NULL);
    if(!appResource || !(pStn = appResource->station))
    {
        moName[0] = 0;
        clLogError("RMR", "POS", "Component address [%#x: %#x] not present in the route list for MO [%s]",
                   pCompAddr->nodeAddress, pCompAddr->portId, _clCorMoIdStrGet(pMoId, moName));
        return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);
    }

    clLog ( CL_LOG_SEV_DEBUG, "RMR", "POS", "The component [0x%x:0x%x] is set as primary OI.",
            pCompAddr->nodeAddress, pCompAddr->portId );

    pStn->isPrimaryOI = pRouteInfo->primaryOI;
    pRt->nPrimaryOI = 1;
    pRouteInfo->status = pStn->status;
    return CL_OK;
}



/**
 * Function to unset the primary OI.
 */
ClRcT
_clCorPrimaryOIClear(corRouteApiInfo_t *pRouteInfo)
{
    RouteInfo_t     *pRt = NULL;
    CORStation_t    *pStn = NULL;
    CORAppResource_t *appResource = NULL;
    ClCorMOIdPtrT   pMoId = NULL;
    ClCorAddrPtrT   pCompAddr = NULL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];

    CL_COR_FUNC_ENTER("RMR", "POC");
    
    if(NULL == pRouteInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("NULL pointer passed. "));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    pMoId = &pRouteInfo->moId;
    pCompAddr = &pRouteInfo->addr;
    
    if( (NULL == pMoId) || (NULL == pCompAddr))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( " NULL pointer passed. "));
        CL_COR_FUNC_EXIT("RMR", "POC");
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Unsetting the OI as read OI. [0x%x: 0x%x]", 
                                   pCompAddr->nodeAddress, pCompAddr->portId));    

    /* Initialize the index and count */
    
    pRt = corRouteInfoGet(pMoId);
    if(!pRt)
    {
        moIdName[0] = 0;
        clLogError("OI", "CLEAR", "No entry for MO [%s]",
                   _clCorMoIdStrGet(pMoId, moIdName));
        return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);
    }

    if(0 == pRt->nStations)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("No station present in the OI list. "));
        CL_COR_FUNC_EXIT("RMR", "POC");
        return CL_COR_SET_RC(CL_COR_ERR_NO_RESOURCE);
    }
            
    if(pRt->nPrimaryOI == 0)
    {
        clLogDebug("RMR", "POICLEAR", "No primary OI registered.");
        CL_COR_FUNC_EXIT("RMR", "POC");
        return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);
    }

    appResource = corAppStationResourceGet(pCompAddr->nodeAddress, pCompAddr->portId, pMoId, NULL);

    if(!appResource || !(pStn = appResource->station))
    {
        moIdName[0] = 0;
        clLogError("OI", "CLEAR", "No resource found for node [%#x], port [%#x], moid [%s]",
                   pCompAddr->nodeAddress, pCompAddr->portId, 
                   _clCorMoIdStrGet(pMoId, moIdName));
        return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);
    }

    if(CL_COR_PRIMARY_OI_ENABLED == pStn->isPrimaryOI)
    {
        pStn->isPrimaryOI = pRouteInfo->primaryOI;
        pRt->nPrimaryOI = 0;
        pRouteInfo->status = pStn->status;
        CL_COR_FUNC_EXIT("RMR", "POC");
        return CL_OK;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Component is not primary OI."));
    CL_COR_FUNC_EXIT("RMR", "POC");
    return CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED);
}

/** 
 * Add OIs.
 *
 * API to add OI to read OI list.
 *
 * @param 
 * 
 * @returns 
 */

static ClRcT
_corPrimaryOIGetWildCard(ClCorMOIdPtrT moh, ClCorAddrT *pid)
{
    ClInt32T       index = -1;
    RouteInfo_t     *pRt = NULL;
    CORStation_t    *pStn = NULL;

    CL_COR_FUNC_ENTER("RMR", "POG");
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /* Initialize count and index */
    index = -1;

    while ( (index = rmRouteMatchGet(moh, index+1)) >= 0)
    {
         if (NULL == (pRt = (RouteInfo_t *)corVectorGet(&routeVector, index)))
         {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to get the route entry."));
            return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
         }

        /* If there is no read OI registered.*/
        if(pRt->nPrimaryOI == 1)
        {
            ClUint32T idx = 0;

            for(idx = 0; idx < pRt->nStations; idx++)
            {
                pStn = corVectorGet(&pRt->stnVector, idx);

                if ((CL_COR_PRIMARY_OI_ENABLED == pStn->isPrimaryOI) 
                        && (CL_COR_STATION_ENABLE == pStn->status))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                            ("Got the read OI [0x%x:0x%x] for this MOId.", pStn->nodeAddress, pStn->portId));
                    memcpy(&pid->nodeAddress, &pStn->nodeAddress, sizeof(ClUint32T));
                    memcpy(&pid->portId, &pStn->portId, sizeof(ClUint32T));
                    return CL_OK;
                }
            } 
        }
    }

    CL_COR_FUNC_EXIT("RMR", "POG");
    return (CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED));
}

ClRcT
_clCorPrimaryOIGet(ClCorMOIdPtrT moh, ClCorAddrT *pid)
{
    ClInt32T       index = -1;
    RouteInfo_t     *pRt = NULL;
    CORStation_t    *pStn = NULL;
    ClCharT moIdName[CL_MAX_NAME_LENGTH];

    CL_COR_FUNC_ENTER("RMR", "POG");
    
    if( NULL == moh )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Input variable is NULL"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }

    /*
     * First check for an exact match.
     */
    pRt = corRouteInfoGet(moh);

    if(!pRt)
    {
        moIdName[0] = 0;
        clLogDebug("OI", "GET", "No primary OI found for MO [%s]. Scanning for wildcard match",
                   _clCorMoIdStrGet(moh, moIdName));
        return _corPrimaryOIGetWildCard(moh, pid);
    }

    if(pRt->nPrimaryOI == 1)
    {
        for(index = 0; index < pRt->nStations; index++)
        {
            pStn = corVectorGet(&pRt->stnVector, index);

            if ((CL_COR_PRIMARY_OI_ENABLED == pStn->isPrimaryOI) 
                && (CL_COR_STATION_ENABLE == pStn->status))
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                               ("Got the read OI [0x%x:0x%x] for this MOId.", pStn->nodeAddress, pStn->portId));
                memcpy(&pid->nodeAddress, &pStn->nodeAddress, sizeof(ClUint32T));
                memcpy(&pid->portId, &pStn->portId, sizeof(ClUint32T));
                return CL_OK;
            }
        } 
    }


    CL_COR_FUNC_EXIT("RMR", "POG");
    return (CL_COR_SET_RC(CL_COR_ERR_OI_NOT_REGISTERED));
}


/**
 * Function to validate that the entry is existing in the route list as a primary OI.
 */  
ClRcT
_clCorPrimaryOIEntryCheck( RouteInfo_t* pRt, ClCorAddrT *pCompAddr, ClCorMOIdPtrT pMoId )
{
    ClUint32T       index = 0;
    ClNameT         moIdName = {0};
    CORStation_t    *pStn = NULL;

    CL_COR_FUNC_ENTER("RMR", "POS");

    /* Checking for NULL */
    clDbgIfNullReturn( pRt, CL_CID_COR );

    /* Checking for NULL */
    clDbgIfNullReturn( pCompAddr, CL_CID_COR );

    clLog( CL_LOG_SEV_TRACE, "RMR", "POS", "Verifying the Primary OI flag for [0x%x:0x%x]",
            pCompAddr->nodeAddress, pCompAddr->portId );

    if(_clCorMoIdToMoIdNameGet(pRt->moH, &moIdName) == CL_OK )
        clLog( CL_LOG_SEV_DEBUG, "RMR", "POS", "The MO [%s] is having [%d] OI(s)", moIdName.value, pRt->nStations );
    else
        clLog( CL_LOG_SEV_DEBUG, "RMR", "POS", "The MO is having [%d] OI(s)", pRt->nStations );

    for( index = 0; index < pRt->nStations; index++ )
    {
        pStn = corVectorGet( &pRt->stnVector, index );
        if( pStn->isPrimaryOI == CL_COR_PRIMARY_OI_ENABLED )
        {
            clLog( CL_LOG_SEV_DEBUG, "RMR", "POS", "The component is [0x%x:0x%x] a primary OI.", 
                    pStn->nodeAddress, pStn->portId );

            if( pStn->nodeAddress == pCompAddr->nodeAddress &&
                pStn->portId == pCompAddr->portId )
            { 
                /* Check for the exact match of the MoId.
                 * If it is then return CL_OK;
                 * Otherwise return CL_COR_ERR_ROUTE_PRESENT
                 */
                if (clCorMoIdCompare(pRt->moH, pMoId) == 0)
                {
                    clLog( CL_LOG_SEV_DEBUG, "RMR", "POS", "The component [0x%x:0x%x] is already the primary OI. ",
                        pStn->nodeAddress, pStn->portId );

                    CL_COR_FUNC_EXIT("RMR", "POS");
                    return CL_OK;
                }
                else
                {
                    clLogError("RMR", "POS", "This OI has already registered for a MoId"
                            " which is matching with the current MoId in wild card match");
                    CL_FUNC_EXIT();
                    return CL_COR_SET_RC(CL_COR_ERR_ROUTE_PRESENT);
                }
            }
            else
            {
                clLog( CL_LOG_SEV_ERROR, "RMR", "POS", "There is already a Primary OI [0x%x:0x%x] for MO[%s] ",
                        pStn->nodeAddress, pStn->portId, moIdName.value );

                CL_COR_FUNC_EXIT("RMR", "POS");
                return CL_COR_SET_RC(CL_COR_ERR_ROUTE_PRESENT);
            }
        }
        else
        {
            clLog( CL_LOG_SEV_TRACE, "RMR", "POS", "The OI is [0x%x:0x%x] not a primary OI.", 
                    pCompAddr->nodeAddress, pCompAddr->portId );
        }
    } 

    clLog( CL_LOG_SEV_ERROR,"RMR", "POS", "The number of primary OI for the MO[%s] is not proper.", moIdName.value );

    CL_COR_FUNC_EXIT("RMR", "POS");
    return CL_OK;

}
