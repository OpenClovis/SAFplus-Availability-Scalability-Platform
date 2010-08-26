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

/**
 * This file implements node version cache for ASP nodes.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <clNodeCache.h>

#define CL_NODE_CACHE_SEGMENT "/CL_NODE_CACHE"

#define CL_CACHE_ALIGN(v, a)  ( ((v)+(a)-1) & ~((a)-1) )
#define CL_NODE_CACHE_ALIGN(v)  CL_CACHE_ALIGN(v, 8)

#define CL_NODE_CACHE_SEGMENT_SIZE  CL_NODE_CACHE_ALIGN((ClUint32T)(sizeof(ClNodeCacheHeaderT)+CL_IOC_MAX_NODES*sizeof(ClNodeCacheEntryT)))

#define CL_NODE_MAP_SIZE  (CL_CACHE_ALIGN(CL_IOC_MAX_NODES, 8) >> 3 )
#define CL_NODE_MAP_WORDS (CL_CACHE_ALIGN(CL_NODE_MAP_SIZE, 4) >> 2 )

/*
 *Faster to operate on words than bytes.
 */
#define CL_NODE_MAP_SET(map, bit)  do {         \
    (map)[ (bit) >> 5] |= ( 1 << ((bit) & 31) ) ;	\
}while(0)

#define CL_NODE_MAP_CLEAR(map, bit) do {        \
    (map)[ (bit) >> 5 ] &= ~(1 << ((bit) & 31));	\
}while(0)

#define CL_NODE_MAP_TEST(map, bit) \
  ( (map)[(bit) >> 5] & ( 1 << ((bit) & 31) ) ) ? 1 : 0

#define CL_NODE_CACHE_HEADER_BASE(base) ( (ClNodeCacheHeaderT*)(base) )
#define CL_NODE_CACHE_ENTRY_BASE(base)  ( (ClNodeCacheEntryT*) ( (ClNodeCacheHeaderT*)base + 1 ) )

static ClUint8T *gpClNodeCache;
static ClCharT gClNodeCacheSegment[CL_MAX_NAME_LENGTH+1];
static ClFdT gClNodeCacheFd;
static ClOsalSemIdT gClNodeCacheSem;
static ClBoolT gClNodeCacheOwner;

typedef struct ClNodeCacheHeader
{
    ClInt32T maxNodes;
    ClInt32T currentNodes;
    ClIocNodeAddressT minVersionNode;
    ClUint32T minVersion;
    ClUint32T nodeMap[CL_NODE_MAP_WORDS];
}ClNodeCacheHeaderT;

typedef struct ClNodeCacheEntry
{
    ClUint32T version; /*entry address is the index into the segment.*/
}ClNodeCacheEntryT;


static ClRcT clNodeCacheCreate(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;

    clOsalShmUnlink(gClNodeCacheSegment);
    rc = clOsalShmOpen(gClNodeCacheSegment, O_RDWR | O_CREAT | O_EXCL, 0666, &fd);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache shm open of segment [%s] returned [%#x]",
                   gClNodeCacheSegment, rc);
        goto out;
    }

    rc = clOsalFtruncate(fd, CL_NODE_CACHE_SEGMENT_SIZE);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache segment truncate of size [%d] returned [%#x]",
                   (ClUint32T) CL_NODE_CACHE_SEGMENT_SIZE, rc);
        goto out_unlink;
    }

    rc = clOsalMmap(0, CL_NODE_CACHE_SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, 
                    (ClPtrT*)&gpClNodeCache);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache segment mmap returned [%#x]", rc);
        goto out_unlink;
    }

    gClNodeCacheFd = fd;

    rc = clOsalSemCreate((ClUint8T*)gClNodeCacheSegment, 1, &gClNodeCacheSem);

    if(rc != CL_OK)
    {
        ClOsalSemIdT semId = 0;

        if(clOsalSemIdGet((ClUint8T*)gClNodeCacheSegment, &semId) != CL_OK)
        {
            clLogError("NODE", "CACHE", "Node cache segment sem creation error while fetching sem id");
            goto out_unlink;
        }

        if(clOsalSemDelete(semId) != CL_OK)
        {
            clLogError("NODE", "CACHE", "Node cache segment sem creation error while deleting old sem id");
            goto out_unlink;
        }

        rc = clOsalSemCreate((ClUint8T*)gClNodeCacheSegment, 1, &gClNodeCacheSem);
    }

    return rc;

    out_unlink:
    clOsalShmUnlink(gClNodeCacheSegment);
    close((ClInt32T)fd);

    out:
    return rc;
}
    

static ClRcT clNodeCacheOpen(void)
{
    ClRcT rc = CL_OK;
    ClFdT fd;
    
    rc = clOsalShmOpen(gClNodeCacheSegment, O_RDWR, 0666, &fd);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache [%s] segment open returned [%#x]",
                   gClNodeCacheSegment, rc);
        goto out;
    }

    rc = clOsalMmap(0, CL_NODE_CACHE_SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, 
                    (ClPtrT*)&gpClNodeCache);

    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache segment mmap returned [%#x]", rc);
        close((ClInt32T)fd);
        goto out;
    }

    gClNodeCacheFd = fd;

    rc = clOsalSemIdGet((ClUint8T*)gClNodeCacheSegment, &gClNodeCacheSem);

    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache semid get returned [%#x]", rc);
        close((ClInt32T)fd);
    }

    out:
    return rc;
}

ClRcT clNodeCacheInitialize(ClBoolT createFlag)
{
    ClRcT rc = CL_OK;

    if(gpClNodeCache)
        goto out;

    snprintf(gClNodeCacheSegment, sizeof(gClNodeCacheSegment)-1,
             "%s_%d", CL_NODE_CACHE_SEGMENT, clIocLocalAddressGet());

    if(createFlag == CL_TRUE)
    {
        rc = clNodeCacheCreate();
    }
    else
    {
        rc = clNodeCacheOpen();
    }
    
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Segment initialize returned [%#x]", rc);
        goto out;
    }

    CL_ASSERT(gpClNodeCache != NULL);

    if(!createFlag)
    {
        goto out;
    }

    CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->maxNodes = CL_IOC_MAX_NODES;
    CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->currentNodes = 0;
    memset(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, 0, 
           sizeof(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap));

    clNodeCacheUpdate(clIocLocalAddressGet(), 
                      CL_VERSION_CODE(CL_RELEASE_VERSION, 1, CL_MINOR_VERSION));

    rc = clOsalMsync(gpClNodeCache, CL_NODE_CACHE_SEGMENT_SIZE, MS_ASYNC);
    if(rc != CL_OK)
    {
        clLogError("NODE", "CACHE", "Node cache segment msync returned [%#x]", rc);
        goto out;
    }

    gClNodeCacheOwner = CL_TRUE;

    out:
    return rc;
}

ClRcT clNodeCacheFinalize(void)
{
    ClRcT rc = CL_ERR_NOT_INITIALIZED;
    ClBoolT cacheOwner = CL_FALSE;

    if(!gpClNodeCache)
        goto out;

    clOsalSemLock(gClNodeCacheSem);
    if( (cacheOwner = gClNodeCacheOwner) )
    {
        rc = clOsalMsync(gpClNodeCache, CL_NODE_CACHE_SEGMENT_SIZE, MS_SYNC);
        CL_ASSERT(rc == CL_OK);
    }

    rc = clOsalMunmap(gpClNodeCache, CL_NODE_CACHE_SEGMENT_SIZE);
    CL_ASSERT(rc == CL_OK);

    gpClNodeCache = NULL;

    clOsalSemUnlock(gClNodeCacheSem);

    if(cacheOwner)
    {
        clOsalShmUnlink(gClNodeCacheSegment);
        clOsalSemDelete(gClNodeCacheSem);
    }

    out:    
    return rc;
}

/*
 * Do a bitmap scan to update the cache header used for min. version gets on
 * resets of min nodeaddress.
 */

static void nodeCacheMinVersionSet(void)
{
    ClInt32T i;
    ClInt32T j;
    ClIocNodeAddressT minVersionNode = CL_IOC_BROADCAST_ADDRESS;
    ClUint32T minVersion = (ClUint32T)-1;

    for(i = 0; i < CL_NODE_MAP_WORDS; ++i)
    {
        ClUint32T mask = CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap[i];
        if(!mask) continue;
        for(j = 0; j < 32; ++j)
        {
            ClIocNodeAddressT node = (i << 5) + j;
            if( (mask & ( 1 << j)) )
            {
                ClUint32T version = CL_NODE_CACHE_ENTRY_BASE(gpClNodeCache)[node].version;
                if(version && version < minVersion)
                {
                    minVersion = version;
                    minVersionNode = node;
                }
            }
        }
    }

    if(minVersion != (ClUint32T)-1)
    {
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode = minVersionNode;
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion = minVersion;
    }
    else
    {
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode = 0;
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion = 0;
    }

}

ClRcT clNodeCacheUpdate(ClIocNodeAddressT nodeAddress, ClUint32T version)
{
    ClRcT   rc = CL_OK;
    if(nodeAddress >= CL_IOC_MAX_NODES) return CL_ERR_INVALID_PARAMETER;

    rc = clOsalSemLock(gClNodeCacheSem);
    if (rc != CL_OK)
        return rc;

    if(!gpClNodeCache)
    {
        clOsalSemUnlock(gClNodeCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    if(!CL_NODE_MAP_TEST(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, nodeAddress))
    {
        CL_NODE_MAP_SET(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, nodeAddress);
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->currentNodes += 1;
    }

    CL_NODE_CACHE_ENTRY_BASE(gpClNodeCache)[nodeAddress].version = version;

    /*
     * Update node min version/address.
     */
    if(!CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion || 
            version < CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion)
    {
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion = version;
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode = nodeAddress;
    }

    clOsalSemUnlock(gClNodeCacheSem);

    return CL_OK;
}

ClRcT clNodeCacheReset(ClIocNodeAddressT nodeAddress)
{
    ClRcT   rc = CL_OK;

    if(nodeAddress >= CL_IOC_MAX_NODES) return CL_ERR_INVALID_PARAMETER;

    rc = clOsalSemLock(gClNodeCacheSem);
    if (rc != CL_OK)
        return rc;

    if(!gpClNodeCache)
    {
        clOsalSemUnlock(gClNodeCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    if(CL_NODE_MAP_TEST(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, nodeAddress))
    {
        CL_NODE_MAP_CLEAR(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, nodeAddress);
        CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->currentNodes -= 1;
        CL_NODE_CACHE_ENTRY_BASE(gpClNodeCache)[nodeAddress].version = 0;

        if(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode == nodeAddress)
        {
            CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode = 0;
            CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion = 0;

            /*
             *Update nodes min version/address.
             */
            nodeCacheMinVersionSet();
        }
    }
    clOsalSemUnlock(gClNodeCacheSem);

    return CL_OK;
}

ClRcT clNodeCacheVersionGet(ClIocNodeAddressT nodeAddress, ClUint32T *pVersion)
{
    ClRcT rc = CL_OK;
    if(nodeAddress >= CL_IOC_MAX_NODES || !pVersion) return CL_ERR_INVALID_PARAMETER;

    *pVersion = 0;
    rc = clOsalSemLock(gClNodeCacheSem);
    if (rc != CL_OK)
        return rc;

    if(!gpClNodeCache)
    {
        clOsalSemUnlock(gClNodeCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    if(CL_NODE_MAP_TEST(CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->nodeMap, nodeAddress))
    {
        *pVersion = CL_NODE_CACHE_ENTRY_BASE(gpClNodeCache)[nodeAddress].version;
    }
    else
    {
        rc = CL_ERR_NOT_EXIST;
    }
    clOsalSemUnlock(gClNodeCacheSem);
    
    return rc;
}


ClRcT clNodeCacheMinVersionGet(ClIocNodeAddressT *pNodeAddress, ClUint32T *pVersion)
{
    ClRcT   rc = CL_OK;

    if(!pVersion) return CL_ERR_INVALID_PARAMETER;

    rc = clOsalSemLock(gClNodeCacheSem);
    if (rc != CL_OK)
        return rc;

    if(!gpClNodeCache)
    {
        clOsalSemUnlock(gClNodeCacheSem);
        return CL_ERR_NOT_INITIALIZED;
    }

    *pVersion = CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersion;
    if(pNodeAddress)
        *pNodeAddress = CL_NODE_CACHE_HEADER_BASE(gpClNodeCache)->minVersionNode;
    clOsalSemUnlock(gClNodeCacheSem);

    return CL_OK;
}
