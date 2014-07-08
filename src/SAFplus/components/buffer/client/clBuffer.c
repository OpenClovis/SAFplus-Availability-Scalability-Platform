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
 * ModuleName  : buffer
 * File        : clBuffer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Clovis BM Buffer Management implementation             
 *****************************************************************************/
#define _CL_BUFFER_C_
#include <stdlib.h>
#ifndef VXWORKS_BUILD
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include <sys/uio.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clOsalApi.h>
#include <clClistApi.h>
#include <clCntApi.h>
#include <clPoolIpi.h>
#include <clHeapApi.h>
#include <clBufferApi.h>
#include <clList.h>
/*
 * We are sharing this with IOC kernel. So some respect
 * being showered through this guy.
 */
#include <clBufferIpi.h>
#include <clEoIpi.h>
#include <clMemStats.h>
#include <clMemTracker.h>
#include <clBinSearchApi.h>


#ifndef DEBUG
#define CL_BUFFER_ASSERT(expr) 
#else
#define CL_BUFFER_ASSERT(expr) CL_ASSERT(expr)
#endif

#define CL_BUFFER_DEPRECATED() do {                                     \
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("%s buffer API is deprecated. "  \
                                       "Please use clBuffer equivalents " \
                                       "or refer to clBufferApi.h",     \
                                       __FUNCTION__));                  \
    }while(0)

#define CL_BUFFER_DEFAULT_INDEX  (0)

#define CL_BUFFER_DEFAULT_SIZE                          \
    gBufferManagementInfo.bufferPoolConfig.pPoolConfig  \
    [CL_BUFFER_DEFAULT_INDEX].chunkSize
                                
#define CL_BUFFER_OVERHEAD ((ClUint32T)sizeof(ClBufferHeaderT))

#define CL_BUFFER_OVERHEAD_CTRL ((ClUint32T)sizeof(ClBufferCtrlHeaderT))

#define CL_BUFFER_OVERHEAD_TOTAL (                                      \
                                  (ClUint32T)(CL_BUFFER_OVERHEAD +      \
                                              CL_BUFFER_OVERHEAD_CTRL + \
                                              PREPEND_SPACE)            \
                                  )

#define CL_BUFFER_MIN_CHUNK_SIZE CL_BUFFER_OVERHEAD_TOTAL

#define CL_BUFFER_LOG(sev,str,...) clEoLibLog(CL_CID_BUFFER,sev,str,__VA_ARGS__)

#define CL_BUFFER_LOG_WRAP(dir)                                         \
    clEoLibLog(CL_CID_BUFFER,CL_LOG_NOTICE,gClBufferLogWrapStrList[(dir)])

/*Buffer stats*/
#define __CL_BUFFER_STATS_UPDATE(size,memDir) do {                      \
        ClBoolT wrapped = CL_FALSE;                                     \
        switch((memDir))                                                \
        {                                                               \
        case CL_MEM_ALLOC:                                              \
            CL_MEM_STATS_UPDATE_ALLOC(gBufferManagementInfo.memStats,   \
                                      size,wrapped);                    \
            break;                                                      \
        case CL_MEM_FREE:                                               \
            CL_MEM_STATS_UPDATE_FREE(gBufferManagementInfo.memStats,    \
                                     size,wrapped);                     \
            break;                                                      \
        default:;                                                       \
        }                                                               \
        if(wrapped == CL_TRUE)                                          \
        {                                                               \
            CL_BUFFER_LOG_WRAP(memDir);                                 \
        }                                                               \
    }while(0)

#define CL_BUFFER_STATS_UPDATE(size,memDir) do {            \
        clOsalMutexLock(&gBufferManagementInfo.mutex);      \
        __CL_BUFFER_STATS_UPDATE(size,memDir);              \
        clOsalMutexUnlock(&gBufferManagementInfo.mutex);    \
    }while(0)

#define CL_BUFFER_WATERMARKS_UPDATE(memDir) do {    \
        clEoMemWaterMarksUpdate(memDir);            \
    }while(0)

#define CL_BUFFER_POOL_FLAGS_SET(config,flags) do { \
        flags = CL_POOL_DEFAULT_FLAG;               \
        if( (config)->lazy == CL_TRUE)              \
        {                                           \
            flags = (ClPoolFlagsT) (flags | CL_POOL_LAZY_FLAG); \
        }                                           \
        if( gBufferDebugLevel > 0 )                 \
        {                                           \
            flags = (ClPoolFlagsT) (flags | CL_POOL_DEBUG_FLAG); \
        }                                           \
    }while(0)

#define CL_POOL_TWO_KB_BUFFER_POOL_SIZE     (128*CL_POOL_TWO_KB_BUFFER)

#define CL_POOL_FOUR_KB_BUFFER_POOL_SIZE    (64*CL_POOL_FOUR_KB_BUFFER)

#define CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE   (64*CL_POOL_EIGHT_KB_BUFFER)

#define NUM_BUFFER_POOL_BLOCKS 4000

typedef struct ClBufferManagementInfo { 
    ClBoolT isInitialized;
    ClBufferPoolConfigT bufferPoolConfig;
    ClPoolT *pPoolHandles;
    ClMemStatsT memStats;
    ClOsalMutexT mutex;
} ClBufferManagementInfoT;

/*****************************************************************************/

#ifndef __cplusplus
static ClBufferManagementInfoT gBufferManagementInfo = {
    .isInitialized = CL_FALSE,
    .pPoolHandles = NULL,
    .bufferPoolConfig = {  .numPools = 0,.pPoolConfig = NULL },
    .memStats = { 0 },    
};
#else
static ClBufferManagementInfoT gBufferManagementInfo = { CL_FALSE,
                                                         { 0, NULL },
                                                         NULL,
                                                         { 0 }
};
#endif

#ifndef __cplusplus
/*The default buffer config*/
static ClPoolConfigT gPoolConfigDefault[] = {
    { 
        .chunkSize = CL_POOL_TWO_KB_BUFFER,
        .incrementPoolSize = (CL_POOL_TWO_KB_BUFFER_POOL_SIZE<<1),
        .initialPoolSize = CL_POOL_TWO_KB_BUFFER_POOL_SIZE << 2,    
        .maxPoolSize = (CL_POOL_TWO_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    },

    { 
        .chunkSize = CL_POOL_FOUR_KB_BUFFER,
        .incrementPoolSize = (CL_POOL_FOUR_KB_BUFFER_POOL_SIZE<<1),
        .initialPoolSize = CL_POOL_FOUR_KB_BUFFER_POOL_SIZE<<2,    
        .maxPoolSize = (CL_POOL_FOUR_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    },

    { 
        .chunkSize = CL_POOL_EIGHT_KB_BUFFER,
        .incrementPoolSize = (CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE<<1),
        .initialPoolSize = CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE<<2,    
        .maxPoolSize = (CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    },
};
#else
static ClPoolConfigT gPoolConfigDefault[] = {
    { 
        CL_POOL_TWO_KB_BUFFER,
        CL_POOL_TWO_KB_BUFFER_POOL_SIZE << 2,    
        (CL_POOL_TWO_KB_BUFFER_POOL_SIZE<<1),
        (CL_POOL_TWO_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    },

    { 
        CL_POOL_FOUR_KB_BUFFER,
        CL_POOL_FOUR_KB_BUFFER_POOL_SIZE<<2,    
        (CL_POOL_FOUR_KB_BUFFER_POOL_SIZE<<1),
        (CL_POOL_FOUR_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    },

    { 
        CL_POOL_EIGHT_KB_BUFFER,
        CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE<<2,    
        (CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE<<1),
        (CL_POOL_EIGHT_KB_BUFFER_POOL_SIZE*NUM_BUFFER_POOL_BLOCKS),
    }
};
#endif

#ifndef __cplusplus
static const ClBufferPoolConfigT gBufferPoolConfigDefault = 
    {
        .numPools = sizeof(gPoolConfigDefault)/sizeof(gPoolConfigDefault[0]),
        .pPoolConfig = gPoolConfigDefault,
        .lazy = CL_FALSE,
        .mode = CL_BUFFER_NATIVE_MODE
    };
#else
static const ClBufferPoolConfigT gBufferPoolConfigDefault = 
    {
        sizeof(gPoolConfigDefault)/sizeof(gPoolConfigDefault[0]),
        gPoolConfigDefault,
        CL_FALSE,
        CL_BUFFER_NATIVE_MODE
    };

#endif

static ClRcT (*gClBufferFromPoolAllocate)(
                                          ClPoolT pool,
                                          ClUint32T actualLength,
                                          ClUint8T **ppChunk,
                                          ClPtrT *ppCookie
                                          );
static ClRcT (*gClBufferFromPoolFree)(ClUint8T *pChunk,
                                      ClUint32T size,
                                      ClPtrT pCookie);

static ClUint32T (*gClBufferLengthCheck)(ClBufferCtrlHeaderT *pCtrlHeader);

static ClUint32T clBufferLengthCheckDebug(ClBufferCtrlHeaderT *pCtrlHeader);

static ClUint32T clBufferLengthCheck(ClBufferCtrlHeaderT *pCtrlHeader);

ClInt32T gBufferDebugLevel;

ClRcT clPoolDestroyForce(ClPoolT pool);

static ClInt32T clBufferCmp(const void *pKey,const void *pEntry)
{
    return  ((ClPoolConfigT *)pKey)->chunkSize - 
        ((ClPoolConfigT*)pEntry)->chunkSize;
}

static ClRcT clBufferValidateConfig(const ClBufferPoolConfigT *pBufferPoolConfig)
{
    ClRcT rc = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
    if(pBufferPoolConfig->numPools == 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Buffer pool count 0\n"));
        goto out;
    }
    if(pBufferPoolConfig->pPoolConfig[0].chunkSize < CL_BUFFER_MIN_CHUNK_SIZE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Min chunk size %d is less than allowed min chunk size %d for buffer\n",pBufferPoolConfig->pPoolConfig[0].chunkSize,CL_BUFFER_MIN_CHUNK_SIZE));
        goto out;
    }
    rc = CL_OK;
    out:
    return rc;
}

/*
 * Normal,debug and purify hooks
 */
#ifndef WITH_PURIFY

/*Include the debugging header stubs*/

#include "clBufferDebug.h"

#ifdef VXWORKS_BUILD
#define CL_BUFFER_POOL_NATIVE_CACHE_SIZE (0x2)
#else
#define CL_BUFFER_POOL_NATIVE_CACHE_SIZE (0x5)
#endif

typedef struct ClBufferPoolNativeCache
{
    ClUint8T *chunk;
    ClUint32T chunkSize;
    ClListHeadT node;
} ClBufferPoolNativeCacheT;

static CL_LIST_HEAD_DECLARE(gBufferPoolNativeCacheList);
static ClUint32T gBufferPoolNativeCacheEntries;
static ClUint32T gBufferPoolNativeCacheLimit;
static ClOsalMutexT gBufferPoolNativeCacheMutex;

static const ClCharT* gClBufferLogWrapStrList[] = {
    "numAlloc statistics wrapped around",
        "numFree statistics wrapped around",
};

static ClRcT
__bufferPoolNativeCacheAdd(ClUint8T *chunk, ClUint32T chunkSize, ClBoolT limit)
{
    ClBufferPoolNativeCacheT *pCache = NULL;
    if(limit && 
       (gBufferPoolNativeCacheEntries >= gBufferPoolNativeCacheLimit))
        return CL_BUFFER_RC(CL_ERR_NO_SPACE);
    pCache = calloc(1, sizeof(*pCache));
    CL_ASSERT(pCache != NULL);
    pCache->chunk = chunk;
    pCache->chunkSize = chunkSize;
    clListAddTail(&pCache->node, &gBufferPoolNativeCacheList);
    ++gBufferPoolNativeCacheEntries;
    return CL_OK;
}

#ifdef VXWORKS_BUILD

static ClUint8T *nativeChunkAlloc(ClUint32T size)
{
    return clHeapCalloc(1, size);
}

static void nativeChunkFree(ClUint8T *chunk)
{
    clHeapFree(chunk);
}

#else

static ClUint8T *nativeChunkAlloc(ClUint32T size)
{
    return calloc(1, size);
}

static void nativeChunkFree(ClUint8T *chunk)
{
    free(chunk);
}

#endif

static ClRcT 
clBufferPoolNativeCacheInitialize(const ClBufferPoolConfigT *pBufferPoolConfig)
{
    ClRcT rc = CL_OK;
    ClInt32T i;
    clOsalMutexInit(&gBufferPoolNativeCacheMutex);
    /*
     * Don't use the cache when running with valgrind
     */
    if(!clParseEnvStr("ASP_VALGRIND_CMD"))
    {
        for(i = 0; i < pBufferPoolConfig->numPools; ++i)
        {
            ClPoolConfigT *pPoolConfig = pBufferPoolConfig->pPoolConfig + i;
            ClInt32T j;
            for(j = 0; j < CL_BUFFER_POOL_NATIVE_CACHE_SIZE; ++j)
            {
                ClUint8T *chunk = nativeChunkAlloc(pPoolConfig->chunkSize);
                __bufferPoolNativeCacheAdd(chunk, pPoolConfig->chunkSize, CL_FALSE);
            }
        }
    }
    gBufferPoolNativeCacheLimit = gBufferPoolNativeCacheEntries;
    clLogNotice("BUFFER", "NATIVE-CACHE", "Cache initialized with [%d] entries", 
                gBufferPoolNativeCacheLimit);
    return rc;
}

static ClRcT
clBufferPoolNativeCacheFinalize(void)
{
    clOsalMutexLock(&gBufferPoolNativeCacheMutex);
    while(!CL_LIST_HEAD_EMPTY(&gBufferPoolNativeCacheList))
    {
        ClListHeadT *next = gBufferPoolNativeCacheList.pNext;
        ClBufferPoolNativeCacheT *cache = CL_LIST_ENTRY(next, ClBufferPoolNativeCacheT, node);
        clListDel(next);
        nativeChunkFree(cache->chunk);
        free(cache);
    }
    gBufferPoolNativeCacheEntries = 0;
    clOsalMutexUnlock(&gBufferPoolNativeCacheMutex);
    return CL_OK;
}

static ClRcT
bufferPoolNativeCacheAdd(ClUint8T *chunk, ClUint32T chunkSize)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gBufferPoolNativeCacheMutex);
    rc = __bufferPoolNativeCacheAdd(chunk, chunkSize, CL_TRUE);
    clOsalMutexUnlock(&gBufferPoolNativeCacheMutex);
    return rc;
}

static ClUint8T *
bufferPoolNativeCacheGet(ClUint32T chunkSize)
{
    ClListHeadT *list = &gBufferPoolNativeCacheList;
    ClListHeadT *temp = NULL;
    clOsalMutexLock(&gBufferPoolNativeCacheMutex);
    CL_LIST_FOR_EACH(temp, list)
    {
        ClBufferPoolNativeCacheT *pCache = CL_LIST_ENTRY(temp, ClBufferPoolNativeCacheT, node);
        if(pCache->chunkSize == chunkSize)
        {
            ClUint8T *chunk = NULL;
            clListDel(temp);
            chunk = pCache->chunk;
            free(pCache);
            --gBufferPoolNativeCacheEntries;
            clOsalMutexUnlock(&gBufferPoolNativeCacheMutex);
            return chunk;
        }
    }
    clOsalMutexUnlock(&gBufferPoolNativeCacheMutex);
    return nativeChunkAlloc(chunkSize);
}

static ClRcT
clBufferFromPoolAllocate (ClPoolT pool,
                          ClUint32T size,
                          ClUint8T **ppChunk,
                          ClPtrT *ppCookie)
{
    ClRcT rc = CL_BUFFER_RC(CL_ERR_NO_MEMORY);
    rc =  clPoolAllocate(pool,ppChunk,ppCookie);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer alloc:Error allocating %d bytes\n",size));
        CL_BUFFER_LOG(CL_LOG_ERROR,
                      "Buffer allocation failed for %d bytes",
                      size);
        goto out;
    }
    CL_BUFFER_STATS_UPDATE(size,CL_MEM_ALLOC);
    CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_ALLOC);
    out:
    return rc;
}

static ClRcT
clBufferFromPoolAllocateNative(ClPoolT pool,
                               ClUint32T size,
                               ClUint8T **ppChunk,
                               ClPtrT *ppCookie)
{
    ClRcT rc = CL_BUFFER_RC(CL_ERR_NO_MEMORY);
    ClUint8T *pChunk = NULL;

    NULL_CHECK(ppChunk);

    pChunk = bufferPoolNativeCacheGet(size);

    if(!pChunk)
        goto out;

    /* The user may not utilise the entire chunk, and if he doesn't errors show up in valgrind */
    /*    memset(pChunk,0,size);  */

    *ppChunk = pChunk;
    if(!gBufferDebugLevel)
    {
#ifndef VXWORKS_BUILD
        CL_BUFFER_STATS_UPDATE(size,CL_MEM_ALLOC);
        CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_ALLOC);
#else
        if(!clParseEnvBoolean("CL_MEM_PARTITION"))
        {
            CL_BUFFER_STATS_UPDATE(size,CL_MEM_ALLOC);
            CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_ALLOC);
        }
#endif
    }
    rc = CL_OK;

    out:
    return rc;
}

static ClRcT clBufferFromPoolFree(ClUint8T *pChunk,ClUint32T size,ClPtrT pCookie)
{
    ClRcT rc;
    CL_BUFFER_STATS_UPDATE(size,CL_MEM_FREE);
    CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_FREE);
    rc =  clPoolFree(pChunk,pCookie);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer free error for %d bytes\n",size));
        CL_BUFFER_LOG(CL_LOG_ERROR,
                      "Buffer free failed for %d bytes",
                      size);
    }
    return rc;
}

static ClRcT clBufferFromPoolFreeNative(ClUint8T *pChunk,ClUint32T size,ClPtrT pCookie)
{
    ClRcT rc = CL_OK;
    if(!gBufferDebugLevel)
    {
#ifndef VXWORKS_BUILD
        CL_BUFFER_STATS_UPDATE(size,CL_MEM_FREE);
        CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_FREE);
#else
        if(!clParseEnvBoolean("CL_MEM_PARTITION"))
        {
            CL_BUFFER_STATS_UPDATE(size,CL_MEM_FREE);
            CL_BUFFER_WATERMARKS_UPDATE(CL_MEM_FREE);
        }
#endif
    }
    rc = bufferPoolNativeCacheAdd(pChunk, size);
    if(rc != CL_OK)
    {
        nativeChunkFree(pChunk);
    }
    return CL_OK;
}

/*Called at init. time*/
static void clBufferModeSet(ClBufferModeT mode)
{
    clBufferDebugLevelSet();
    switch(mode)
    {
    case CL_BUFFER_PREALLOCATED_MODE:
        gClBufferFromPoolAllocate = clBufferFromPoolAllocate;
        gClBufferFromPoolFree = clBufferFromPoolFree;
        CL_BUFFER_DEBUG_SET();
        break;

    case CL_BUFFER_NATIVE_MODE:
    default:
        gClBufferFromPoolAllocate = clBufferFromPoolAllocateNative;
        gClBufferFromPoolFree = clBufferFromPoolFreeNative;
        CL_BUFFER_NATIVE_DEBUG_SET();
        break;

    }
}

/*Called at exit time*/
static void clBufferModeUnset(ClBufferModeT mode)
{
    clBufferDebugLevelUnset();
    if(mode == CL_BUFFER_NATIVE_MODE)
    {
#if !defined(SOLARIS_BUILD) && !defined(POSIX_BUILD)
        mallopt(M_CHECK_ACTION,0);
#endif
    }
    gClBufferFromPoolAllocate = NULL;
    gClBufferFromPoolFree = NULL;
}

static ClRcT clBMBufferPoolStatsGet(ClUint32T numPools,ClUint32T *pPoolSize,ClPoolStatsT *pPoolStats)
{
    ClRcT errorCode = CL_OK;
    ClUint32T currentPoolSize = 0;
    register ClUint32T i;

    numPools = CL_MIN(numPools,gBufferManagementInfo.bufferPoolConfig.numPools);
    for(i = 0; i < numPools;++i)
    {
        errorCode = clPoolStatsGet(gBufferManagementInfo.pPoolHandles[i],
                                   pPoolStats+i);
        if(errorCode != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in getting pool stats "
                                           "for pool size:%d\n",
                                           gBufferManagementInfo.bufferPoolConfig.pPoolConfig[i].chunkSize));
            goto out;
        }
        currentPoolSize += pPoolStats[i].numExtendedPools * pPoolStats[i].poolConfig.incrementPoolSize;
    }

    *pPoolSize = currentPoolSize;

    out:
    return errorCode;
}

static ClRcT
clBufferPoolInitialize(const ClBufferPoolConfigT *pBufferPoolConfigUser)
{
    ClRcT rc = CL_OK;
    register ClUint32T i = 0;
    ClPoolFlagsT flags = CL_POOL_DEFAULT_FLAG;
    ClBufferPoolConfigT *pBufferPoolConfig = &gBufferManagementInfo.bufferPoolConfig;

    if(pBufferPoolConfigUser == NULL)
    {
        pBufferPoolConfigUser = &gBufferPoolConfigDefault;
    }

    rc = CL_BUFFER_RC(CL_ERR_NO_MEMORY);

    memcpy(pBufferPoolConfig,pBufferPoolConfigUser,sizeof(*pBufferPoolConfig));

    if(! (pBufferPoolConfig->pPoolConfig = (ClPoolConfigT*) clHeapAllocate(sizeof(*pBufferPoolConfig->pPoolConfig) * pBufferPoolConfigUser->numPools)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out;
    }

    if(!(gBufferManagementInfo.pPoolHandles = (void**) clHeapAllocate(sizeof(ClPoolT) * pBufferPoolConfigUser->numPools)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating memory\n"));
        goto out_free;
    }

    memset(gBufferManagementInfo.pPoolHandles,0,sizeof(ClPoolT)*pBufferPoolConfigUser->numPools);

    memcpy(pBufferPoolConfig->pPoolConfig,
           pBufferPoolConfigUser->pPoolConfig,
           sizeof(*pBufferPoolConfig->pPoolConfig) * pBufferPoolConfigUser->numPools);

    /*
     * keep the list sorted
     * to accomodate a binary search
     * while allocating a chunk.
     */

    qsort(pBufferPoolConfig->pPoolConfig,pBufferPoolConfig->numPools,
          sizeof(*pBufferPoolConfig->pPoolConfig),clBufferCmp);

    if( (rc = clBufferValidateConfig(pBufferPoolConfig) ) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer config validation failed\n"));
        goto out_free;
    }
    
    clBufferModeSet(pBufferPoolConfig->mode);

    if(pBufferPoolConfig->mode == CL_BUFFER_PREALLOCATED_MODE)
    {

        CL_BUFFER_POOL_FLAGS_SET(pBufferPoolConfig,flags);
        
        for(i = 0; i < pBufferPoolConfig->numPools;++i)
        {
            ClPoolT pool = 0;
            rc = clPoolCreate(&pool,flags,pBufferPoolConfig->pPoolConfig+i);
            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in pool create:0x%x\n",rc));
                goto out_destroy;
            }
            gBufferManagementInfo.pPoolHandles[i] = pool;
        }
    }
    else
    {
        clBufferPoolNativeCacheInitialize((const ClBufferPoolConfigT*)pBufferPoolConfig);
    }
    
    rc = CL_OK;
    goto out;

    out_destroy:
    {
        register ClUint32T j;
        for(j = 0; j < i; ++j)
        {
            if(clPoolDestroy(gBufferManagementInfo.pPoolHandles[j]) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in destroying pool:%d\n",j));
            }
        }
    }
    out_free:
    if(pBufferPoolConfig->pPoolConfig)
    {
        clHeapFree(pBufferPoolConfig->pPoolConfig);
    }
    if(gBufferManagementInfo.pPoolHandles)
    {
        clHeapFree(gBufferManagementInfo.pPoolHandles);
    }
    gBufferManagementInfo.pPoolHandles = 0;
    pBufferPoolConfig->pPoolConfig = NULL;

    out:
    return rc;
}

static ClRcT 
clBufferPoolFinalize(void)
{
    ClBufferPoolConfigT *pBufferPoolConfig;
    ClRcT rc = CL_OK;

    pBufferPoolConfig = &gBufferManagementInfo.bufferPoolConfig;
    if(pBufferPoolConfig->mode == CL_BUFFER_PREALLOCATED_MODE)
    {
        register ClUint32T i;
        for(i = 0; i < pBufferPoolConfig->numPools; ++i)
        {
            ClPoolT pool = gBufferManagementInfo.pPoolHandles[i];
            if(pool && (rc = clPoolDestroyForce(pool)) != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error destroying pool:%d\n",i));
                goto out;
            }
        }
    }
    else
    {
        clBufferPoolNativeCacheFinalize();
    }
    clBufferModeUnset(pBufferPoolConfig->mode);
    clHeapFree(pBufferPoolConfig->pPoolConfig);
    clHeapFree(gBufferManagementInfo.pPoolHandles);
    out:
    return rc;
}

/*Debugging stubs for buffer MGMT*/
#include "clBufferDebug.c"

#else

#include "clBufferPurify.c"

#endif

static ClRcT
clBMChainReset(ClBufferCtrlHeaderT *pCtrlHeader,
               ClBufferHeaderT* pStartBufferHeader)
{
    ClBufferHeaderT* pTemp = NULL;
    ClUint32T prependSpace = PREPEND_SPACE;
    pTemp = pStartBufferHeader;
    while(NULL != pTemp) {
        pTemp->startOffset = CL_BUFFER_OVERHEAD + prependSpace;
        if(prependSpace > 0 )
        {
            prependSpace = 0;
        }
        /*
         * If we had a stitched heap chain, just unlink the heap chain.
         */
        if(pTemp->pool == CL_BUFFER_HEAP_MARKER)
        {
            ClBufferHeaderT *pNext = pTemp->pNextBufferHeader;
            if(pTemp->pPreviousBufferHeader)
            {
                pTemp->pPreviousBufferHeader->pNextBufferHeader = pNext;
            }
            if(pNext)
            {
                pNext->pPreviousBufferHeader = pTemp->pPreviousBufferHeader;
            }
            clHeapFree(pTemp->pCookie);
            /* May prevent double free */
            pTemp->pCookie = NULL;

            clHeapFree(pTemp);
            /* May prevent double free */
            pTemp = NULL;

            pTemp = pNext;
            continue;
        }
        pTemp->readOffset = pTemp->startOffset;
        pTemp->writeOffset = pTemp->startOffset;
        pTemp->dataLength = pTemp->startOffset;
        pTemp = pTemp->pNextBufferHeader;        
    }
    pCtrlHeader->length = 0;
    pCtrlHeader->currentReadOffset = 0;
    pCtrlHeader->currentWriteOffset = 0;
    pCtrlHeader->pCurrentReadBuffer = pStartBufferHeader;
    pCtrlHeader->pCurrentWriteBuffer = pStartBufferHeader;
    return(CL_OK);
}

/*****************************************************************************/

#ifdef POSIX_BUILD

static ClRcT
clBMBufferChainFree(ClBufferCtrlHeaderT *pCtrlHeader,
                    ClBufferHeaderT* pBufferHeader)
{
    ClBufferHeaderT* pTemp = NULL;
    ClBufferHeaderT* pNext = NULL;
    ClBufferHeaderT *pFreeList = NULL;
    ClUint8T* pFreeAddress = NULL;
    ClRcT rc = CL_OK;
    ClUint32T ctrlHeaderSize = 0;
    ClBoolT chainFree = CL_FALSE;
    if(NULL == pBufferHeader) 
    {
        return rc;
    }

    if(pBufferHeader->pPreviousBufferHeader == NULL)
    {
        ctrlHeaderSize = CL_BUFFER_OVERHEAD_CTRL;
        chainFree = CL_TRUE;
    }

    pTemp = pBufferHeader;

    while(pTemp != NULL) {

        pFreeAddress = (ClUint8T*)((ClUint8T*)pTemp - ctrlHeaderSize);
    
        pNext = pTemp->pNextBufferHeader;

        /*
         * update the length if the chain isnt being freed
         * and someone has not reset the length for the header.
         */
        if(chainFree == CL_FALSE)
        {
            pCtrlHeader->length -= CL_MIN(pTemp->dataLength - pTemp->startOffset,
                                          pCtrlHeader->length
                                          );
        }
        if(ctrlHeaderSize > 0 )
        {
            ctrlHeaderSize = 0;
        }
        else
        {
            pTemp->pNextBufferHeader = pFreeList;
            pFreeList = pTemp;
        }

        pTemp = pNext;

    }
    
    for(pNext = NULL, pTemp = pFreeList; pTemp ; pTemp = pNext)
    {
        ClUint32T chunkSize = pTemp->chunkSize;
        ClPtrT cookie = pTemp->pCookie;
        pNext = pTemp->pNextBufferHeader;
        if(pTemp->pool == CL_BUFFER_HEAP_MARKER)
        {
            /*
             * Its a sticky heap chain
             */
            clHeapFree(pTemp->pCookie);
            clHeapFree(pTemp);
        }
        else
        {
            rc = gClBufferFromPoolFree((ClUint8T*)pTemp, chunkSize, cookie);
            if(rc != CL_OK)
            {
                clLogError("CHAIN", "FREE", 
                           "Freeing buffer [%p] of size [%d] returned [%#x]",
                           (ClPtrT)pTemp, chunkSize, rc);
            }
        }
    }

    if(chainFree)
    {
        ClUint32T chunkSize = pBufferHeader->chunkSize;
        ClPtrT cookie = pBufferHeader->pCookie;
        CL_ASSERT(pBufferHeader->pool != CL_BUFFER_HEAP_MARKER);
        rc = gClBufferFromPoolFree((ClUint8T*)pCtrlHeader, chunkSize, cookie);
        if(rc != CL_OK)
        {
            clLogError("CHAIN", "FREE", "Freeing chain [%p] of size [%d] returned [%#x]",
                       (ClPtrT)pCtrlHeader, chunkSize, rc);
        }
    }
    else
    {
        gClBufferLengthCheck(pCtrlHeader);
    }
    return rc;
}

#else

static ClRcT
clBMBufferChainFree(ClBufferCtrlHeaderT *pCtrlHeader,
                    ClBufferHeaderT* pBufferHeader)
{
    ClBufferHeaderT* pTemp = NULL;
    ClBufferHeaderT* pNext = NULL;
    ClUint8T* pFreeAddress = NULL;
    ClRcT rc = CL_OK;
    ClUint32T ctrlHeaderSize = 0;
    ClBoolT chainFree = CL_FALSE;
    if(NULL == pBufferHeader) 
    {
        return rc;
    }

    if(pBufferHeader->pPreviousBufferHeader == NULL)
    {
        ctrlHeaderSize = CL_BUFFER_OVERHEAD_CTRL;
        chainFree = CL_TRUE;
    }

    pTemp = pBufferHeader;

    while(pTemp != NULL) {

        pFreeAddress = (ClUint8T*)((ClUint8T*)pTemp - ctrlHeaderSize);
    
        pNext = pTemp->pNextBufferHeader;

        /*
         * update the length if the chain isnt being freed
         * and someone has not reset the length for the header.
         */
        if(chainFree == CL_FALSE)
        {
            pCtrlHeader->length -= CL_MIN(pTemp->dataLength - pTemp->startOffset,
                                          pCtrlHeader->length
                                          );
        }
        if(pTemp->pool == CL_BUFFER_HEAP_MARKER)
        {
            clHeapFree(pTemp->pCookie);
            /* May prevent double free */
            pTemp->pCookie = NULL;

            clHeapFree(pTemp);
            /* May prevent double free */
            pTemp = NULL;
        }
        else
        {
            /* Free the byte buffer attached to the current buffer header */
            rc = gClBufferFromPoolFree(pFreeAddress,pTemp->chunkSize,pTemp->pCookie);

            if(rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in freeing buffer:%p\n",(ClPtrT)pFreeAddress));
                goto out;
            }
        }
        if(ctrlHeaderSize > 0 )
        {
            ctrlHeaderSize = 0;
        }
        pTemp = pNext;
    }
    out:
    if(chainFree == CL_FALSE)
    {
        gClBufferLengthCheck(pCtrlHeader);
    }
    return rc;
}
#endif

/*****************************************************************************/

static ClRcT
clBMBufferChainAllocate(ClUint32T size, ClBufferHeaderT** ppBuffer)
{
    ClRcT errorCode = CL_OK;
    ClPoolT pool = 0;
    ClPtrT pCookie = NULL;
    ClUint8T *pChunk = NULL;
    ClBufferHeaderT* pNewBuffer = NULL;
    ClBufferHeaderT* pPrevBuffer = NULL;
    ClUint32T startOffset = 0;
    ClUint32T chunkSize = 0;
    ClUint32T poolIndex = 0;
    ClUint32T prependSpace = 0;
    ClUint32T ctrlHeaderSize = 0;
    ClUint32T maxBufferSize = 0;
    ClUint32T overhead = CL_BUFFER_OVERHEAD;
    NULL_CHECK(ppBuffer);

    pPrevBuffer = *ppBuffer;

    size += overhead;

    /*
     * Add the overhead if its the first one.
     */
    if(pPrevBuffer == NULL)
    {
        size += CL_BUFFER_OVERHEAD_CTRL + PREPEND_SPACE;
        overhead += CL_BUFFER_OVERHEAD_CTRL + PREPEND_SPACE;
        prependSpace = PREPEND_SPACE;
        ctrlHeaderSize = CL_BUFFER_OVERHEAD_CTRL;
    }

    poolIndex = gBufferManagementInfo.bufferPoolConfig.numPools - 1;

    maxBufferSize = gBufferManagementInfo.bufferPoolConfig.pPoolConfig[poolIndex].chunkSize;
                                                                       
    if(size < maxBufferSize)
    {
        /*
         * Get the nearest chunk matching the size
         * and grant it. But keep track of the chain length
         * for granting right sizes since we could end up with a
         * large chain causing performance issues with increasing buffers
         */
        ClPoolConfigT *pPoolConfig = NULL;
        ClPoolConfigT chunkKey = {0};

        if(pPrevBuffer != NULL)
        {
            if(!pPrevBuffer->pPreviousBufferHeader)
                size <<= 2;
            else if(!pPrevBuffer->pPreviousBufferHeader->pPreviousBufferHeader)
                size <<= 3;
            else if(!pPrevBuffer->pPreviousBufferHeader->pPreviousBufferHeader->pPreviousBufferHeader)
                size <<= 4;
            else 
                /*expanding chain*/
                size = maxBufferSize;
            
            if(size > maxBufferSize) size = maxBufferSize;
        }
        
        if(size == maxBufferSize)
        {
            chunkSize = maxBufferSize;
        }
        else
        {
            chunkKey.chunkSize = size;
            /*
             * Get the nearest chunk matching the size from our pool
             */
            errorCode  =   clBinarySearch((ClPtrT)&chunkKey,
                                          gBufferManagementInfo.bufferPoolConfig.pPoolConfig,                                     
                                          gBufferManagementInfo.bufferPoolConfig.numPools,
                                          sizeof(ClPoolConfigT),
                                          clBufferCmp,
                                          (ClPtrT*)&pPoolConfig);

            /*This cannot happen*/
            CL_BUFFER_ASSERT(errorCode == CL_OK);

            poolIndex = pPoolConfig -
                gBufferManagementInfo.bufferPoolConfig.pPoolConfig;
            /*Use this chunkSize*/
            size = chunkSize = pPoolConfig->chunkSize;
        }
    }
    else
    {
        /*Use the maximum index*/
        ClUint32T chunks  = 1;
        ClUint32T dataSize;
        chunkSize = gBufferManagementInfo.bufferPoolConfig.pPoolConfig[poolIndex].chunkSize;
        /*
         * Add the first chunk and then estimate chunks needed to 
         * satisfy the request
         */
        dataSize = chunkSize - overhead;
        size -= overhead;
        while(dataSize < size ) 
        {
            ++chunks;
            /*
             * The first could have had variable overhead.
             * subsequent chunks have a constant overhead.
             */
            dataSize += chunkSize - CL_BUFFER_OVERHEAD;
        }
        size = chunkSize * chunks;
    }

    pool = gBufferManagementInfo.pPoolHandles[poolIndex];

    /*
     * We will make sure at config. time that max. chunk Size
     * is always greater than buffer overhead
     */
    while(size > 0)
    {
        errorCode = gClBufferFromPoolAllocate (pool,chunkSize, 
											   &pChunk,&pCookie);
        
        if(errorCode != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating buffer of size "
                                           " [%d]",chunkSize));
            goto out;
        }

        size -= chunkSize;

        pNewBuffer = (ClBufferHeaderT*)((ClCharT*)pChunk + 
                                        ctrlHeaderSize);

        if(NULL == *ppBuffer)
        {
            *ppBuffer = pNewBuffer;
        }

        startOffset = CL_BUFFER_OVERHEAD + prependSpace;

        if(prependSpace)
        {
            prependSpace = 0;
        }

        pNewBuffer->chunkSize = chunkSize;
        pNewBuffer->pool = pool;
        pNewBuffer->pCookie = pCookie;
        pNewBuffer->startOffset = startOffset;
        pNewBuffer->readOffset = startOffset;
        pNewBuffer->writeOffset = startOffset;
        pNewBuffer->dataLength = startOffset;
        pNewBuffer->actualLength = chunkSize - ctrlHeaderSize;
        pNewBuffer->pPreviousBufferHeader = pPrevBuffer;
        pNewBuffer->pNextBufferHeader = NULL;
        
        if(NULL != pPrevBuffer)
        {
            pPrevBuffer->pNextBufferHeader = pNewBuffer;
        }

        pPrevBuffer = pNewBuffer;

        if(ctrlHeaderSize > 0 )
        {
            memset(pChunk,0,ctrlHeaderSize);
            ctrlHeaderSize = 0;
        }

    }
    out:
    return errorCode;
}

/*****************************************************************************/

static ClRcT
clBMBufferChainChecksum16Compute(ClBufferHeaderT* pStartBufferHeader, 
								 ClUint32T offset, ClUint32T length, 
								 ClUint16T* pChecksum)
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClUint32T bytesTraversed = 0;
    ClUint8T* pTemp = NULL;
    ClUint16T sum = 0;
    ClUint16T tempSum = 0;
    ClUint16T checkSum = 0;
    ClUint16T check = 0;
    

    NULL_CHECK(pStartBufferHeader);

    pCurrentBufferHeader = pStartBufferHeader;

    while(pCurrentBufferHeader != NULL) {
    
        bytesTraversed = bytesTraversed + pCurrentBufferHeader->dataLength -
            pCurrentBufferHeader->startOffset;
    
        if(offset <= bytesTraversed) 
        {
            break;
        }    
        
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;        
    }

    if(NULL == pCurrentBufferHeader) 
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        return(errorCode);
    }

    pTemp = (ClUint8T*)((ClUint8T*)pCurrentBufferHeader + 
                        pCurrentBufferHeader->dataLength -
                        (bytesTraversed - offset));
    
    while(length > 0) {
    
        while(pTemp != ((ClUint8T*)pCurrentBufferHeader + 
						pCurrentBufferHeader->dataLength)) {
        
            check += (*pTemp);
        
            /* add one if byte addition carry occurs */
            if (check & 0xFF00)
            {
                check &= 0xFF;
                check++;
            }

            /* sum = (n)D[0] + (n-1)D[1] + ... + (1)D[n-1]    */
    
            tempSum = length * (*pTemp);
        
            /* add carry if byte multiplication carry occurs */
            if (tempSum & 0xFF00)
            {
                tempSum = (tempSum >> 8) + (tempSum & 0xFF);
            }

            sum += tempSum;

            /* add one if byte addition carry occurs */
            if (sum & 0xFF00)
            {
                sum &= 0xFF;
                sum++;
            }
        
            pTemp++;
            length --;
        }
    
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
        if(NULL != pCurrentBufferHeader) 
        {
            pTemp = (ClUint8T*)((ClUint8T*)pCurrentBufferHeader + 
                                pCurrentBufferHeader->startOffset);
        }
        else 
        {
            if(length == 0) 
            {
                break;
            }
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }
    }

    /* Move Check into 2-byte CheckSum */
    checkSum = check;
  
    /* Move Check value to high byte of CheckSum */
    checkSum <<= 8;
  
    /* Or Sum value into low byte of CheckSum */
    checkSum |= sum;
   
    *pChecksum = checkSum;
    
    return(CL_OK);
}

/*****************************************************************************/

static ClRcT
clBMBufferChainChecksum32Compute(ClBufferHeaderT* pStartBufferHeader, 
								 ClUint32T offset, ClUint32T length, 
								 ClUint32T* pChecksum)
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClUint32T bytesTraversed = 0;
    ClUint8T* pTemp = NULL;
    ClUint32T sum = 0;
    ClUint32T tempSum = 0;
    ClUint32T checkSum = 0;
    ClUint32T check = 0;
    

    NULL_CHECK(pStartBufferHeader);

    pCurrentBufferHeader = pStartBufferHeader;

    while(pCurrentBufferHeader != NULL) {
    
        bytesTraversed = bytesTraversed + pCurrentBufferHeader->dataLength - 
            pCurrentBufferHeader->startOffset;
    
        if(offset <= bytesTraversed) 
        {
            break;
        }    
        
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;        
    }

    if(NULL == pCurrentBufferHeader) 
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        return(errorCode);
    }

    pTemp = (ClUint8T*)((ClUint8T*)pCurrentBufferHeader + 
                        pCurrentBufferHeader->dataLength -
                        (bytesTraversed - offset));

    while(length > 0) {
    
        while(pTemp != ((ClUint8T*)pCurrentBufferHeader + 
						pCurrentBufferHeader->dataLength)) {
        
            /* check = D[0] + D[1] + ... + D[n-1] */
        
            check += (*pTemp);
        
            /* add one if byte addition carry occurs */
            if (check & 0xFFFF0000)
            {
                check &= 0xFFFF;
                check++;
            }

            /* sum = (n)D[0] + (n-1)D[1] + ... + (1)D[n-1]    */
    
            tempSum = length * (*pTemp);
        
            /* add carry if byte multiplication carry occurs */
            if (tempSum & 0xFFFF0000)
            {
                tempSum = (tempSum >> 16) + (tempSum & 0xFFFF);
            }

            sum += tempSum;

            /* add one if byte addition carry occurs */
            if (sum & 0xFFFF0000)
            {
                sum &= 0xFFFF;
                sum++;
            }
        
            pTemp++;
            length --;
        }
    
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
        if(NULL != pCurrentBufferHeader) 
        {
            pTemp = (ClUint8T*)((ClUint8T*)pCurrentBufferHeader + 
                                pCurrentBufferHeader->startOffset);
        }
        else 
        {
            if(length == 0)
            {
                break;
            }
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }
    }

    /* Move Check into 4-byte CheckSum */
    checkSum = check;
  
    /* Move Check value to high byte of CheckSum */
    checkSum <<= 16;
  
    /* Or Sum value into low byte of CheckSum */
    checkSum |= sum;
   
    *pChecksum = checkSum;
    
    return(CL_OK);
}

/*****************************************************************************/
static ClRcT
clBMBufferNBytesWrite(ClBufferCtrlHeaderT *pCtrlHeader,
                      ClBufferHeaderT* pBufferHeader, ClUint8T* pInputBuffer, 
                      ClUint32T numberOfBytes)
{
    ClRcT errorCode;
    ClUint8T* pTemp = pInputBuffer;
    ClInt32T remainingBytes = (ClInt32T)numberOfBytes;
    ClUint32T currentWriteOffset = 0;
    ClUint32T length = 0;
    ClUint32T writeOffset = pBufferHeader->writeOffset;

    do
    {

        ClInt32T remainingSpace;
        ClUint8T* pCopyLocation;
        ClUint32T oldLen =      pBufferHeader->dataLength - pBufferHeader->startOffset;

        remainingSpace = pBufferHeader->actualLength - writeOffset;
        remainingSpace = CL_MIN(remainingSpace,remainingBytes);

        if(pBufferHeader->pool == CL_BUFFER_HEAP_MARKER)
        {
            pCopyLocation = pBufferHeader->pCookie;
        }
        else
        {
            pCopyLocation = (ClUint8T*)((ClUint8T*)pBufferHeader + writeOffset);
        }
        memcpy(pCopyLocation, pTemp, remainingSpace);
        writeOffset += remainingSpace;
        /*update the current write offset*/
        currentWriteOffset += remainingSpace;
        pBufferHeader->writeOffset = writeOffset;

        /*update length if data isnt being overwritten*/
        if(pBufferHeader->dataLength < writeOffset)
        {
            pBufferHeader->dataLength = writeOffset;
            /*update length*/
            length += writeOffset - pBufferHeader->startOffset;
            /*reduce the old one*/
            length -= oldLen;
        }

        remainingBytes -= remainingSpace;
        if(remainingBytes <= 0 )
        {
            break;
        }
        pTemp += remainingSpace;

        if(NULL == pBufferHeader->pNextBufferHeader)
        {
            errorCode = clBMBufferChainAllocate((ClUint32T)remainingBytes, 
                                                &pBufferHeader);
            if((CL_OK != errorCode) || 
               (NULL == pBufferHeader->pNextBufferHeader))
            {
                return (CL_ERR_NO_MEMORY); 
            }
        } 

        pBufferHeader = pBufferHeader->pNextBufferHeader;
        writeOffset = pBufferHeader->startOffset; 

    }while(1);

    pCtrlHeader->pCurrentWriteBuffer = pBufferHeader;
    pCtrlHeader->currentWriteOffset += currentWriteOffset;
    /*update length*/
    pCtrlHeader->length += length;
    gClBufferLengthCheck(pCtrlHeader);

    return(CL_OK);
}

/*****************************************************************************/
static ClRcT
clBMBufferNBytesRead(ClBufferCtrlHeaderT *pCtrlHeader,
                     ClBufferHeaderT* pBufferHeader, ClUint8T* pOutputBuffer, 
                     ClUint32T* pNumberOfBytes)
{
    ClUint32T bytesRemaining = *pNumberOfBytes;
    ClUint32T readOffset = pBufferHeader->readOffset;
    ClUint32T currentReadOffset = 0;
    do {
        ClUint8T* pTemp;
        ClInt32T bytesRead;
        
        if(pBufferHeader->pool == CL_BUFFER_HEAP_MARKER)
        {
            pTemp = (ClUint8T*)pBufferHeader->pCookie + readOffset;
        }
        else
        {
            pTemp = (ClUint8T*)pBufferHeader + readOffset;
        }
        bytesRead = pBufferHeader->dataLength - readOffset;
        bytesRead = CL_MIN(bytesRead,bytesRemaining);

        memcpy (pOutputBuffer, pTemp, bytesRead);
        pOutputBuffer += bytesRead;
        readOffset += bytesRead;
        pBufferHeader->readOffset = readOffset;
        bytesRemaining -= bytesRead;

        currentReadOffset += bytesRead;

        if((ClInt32T)bytesRemaining <= 0 )
        {
            break;
        }

        pBufferHeader = pBufferHeader->pNextBufferHeader;

        if(NULL == pBufferHeader)
        {
            break;
        } 

        readOffset = pBufferHeader->startOffset;

    } while (1);

    if(bytesRemaining > 0)
    {
        memset(pOutputBuffer, 0, bytesRemaining);
        *pNumberOfBytes -= bytesRemaining;
    }
    else
    {
        pCtrlHeader->pCurrentReadBuffer = pBufferHeader;
        pCtrlHeader->currentReadOffset += currentReadOffset;
    }
    return(CL_OK);
}

/*****************************************************************************/
static ClRcT
clBMBufferReadOffsetSet(ClBufferCtrlHeaderT *pCtrlHeader,
                        ClBufferHeaderT* pStartBufferHeader, 
						ClInt32T newReadOffset, 
                        ClBufferSeekTypeT seekType
                        )
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pBufferHeader = pStartBufferHeader;
    ClUint32T bytesToSkip = 0;
    
    switch(seekType) {

    case CL_BUFFER_SEEK_SET :   

        if(newReadOffset < 0) 
        {
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }
        
        bytesToSkip = (ClUint32T)newReadOffset;

    seek_set:
        pCtrlHeader->currentReadOffset = bytesToSkip;

        while(pBufferHeader != NULL)
        {
            if(bytesToSkip > (pBufferHeader->dataLength - 
                              pBufferHeader->startOffset))
            {
                bytesToSkip -= (pBufferHeader->dataLength -
                                pBufferHeader->startOffset);
                pBufferHeader = pBufferHeader->pNextBufferHeader;
            }
            else
            {
                pBufferHeader->readOffset = pBufferHeader->startOffset +
                    bytesToSkip;
                pCtrlHeader->pCurrentReadBuffer = pBufferHeader;
                return (CL_OK);
            }
        }

        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        return(errorCode);

    case CL_BUFFER_SEEK_CUR : 

        if(newReadOffset == 0) 
        {
            return(CL_OK);
        }
        bytesToSkip = pCtrlHeader->currentReadOffset;
            
        if(bytesToSkip + newReadOffset < bytesToSkip )
        {
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }

        bytesToSkip += newReadOffset;

        goto seek_set;

        /*unreached*/
        break;

    case CL_BUFFER_SEEK_END :  
        {
            ClInt32T skip = pCtrlHeader->length + newReadOffset;
            if(0 > skip || skip > (ClInt32T) pCtrlHeader->length)
            {
                errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
                return errorCode;
            }
            bytesToSkip = (ClUint32T)skip;
            goto seek_set;
            /*unreached*/
        }
        break;
    case CL_BUFFER_SEEK_MAX : 
        break;
    }

    return CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
}
/*****************************************************************************/
static ClRcT
clBMBufferWriteOffsetSet(ClBufferCtrlHeaderT *pCtrlHeader,
                         ClBufferHeaderT* pStartBufferHeader, 
                         ClInt32T newWriteOffset, 
                         ClBufferSeekTypeT seekType
                         )
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pBufferHeader = pStartBufferHeader;
    ClUint32T bytesToSkip = 0;

    switch(seekType) {
    
    case CL_BUFFER_SEEK_SET :   

        if(newWriteOffset < 0) 
        {
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }

        bytesToSkip = (ClUint32T)newWriteOffset;
            
    seek_set:
        pCtrlHeader->currentWriteOffset = bytesToSkip;

        while(pBufferHeader != NULL) 
        {
            
            if(bytesToSkip > (pBufferHeader->dataLength - 
                              pBufferHeader->startOffset)) 
            {
                
                bytesToSkip = bytesToSkip - (pBufferHeader->dataLength -
                                             pBufferHeader->startOffset);
                pBufferHeader = pBufferHeader->pNextBufferHeader;
                
            }
            else 
            {                                
                pBufferHeader->writeOffset = pBufferHeader->startOffset + 
                    bytesToSkip;
                pCtrlHeader->pCurrentWriteBuffer = pBufferHeader;
                return(CL_OK);
            }
        }
        
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        return(errorCode);
        
    case CL_BUFFER_SEEK_CUR :  

        bytesToSkip =  pCtrlHeader->currentWriteOffset;

        if(bytesToSkip + newWriteOffset < bytesToSkip )
        {
            errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
            return(errorCode);
        }
        bytesToSkip += newWriteOffset;

        goto seek_set;
        /*unreached*/
        break;

    case CL_BUFFER_SEEK_END :  
        {
            ClInt32T skip = pCtrlHeader->length + newWriteOffset;
            if(0 > skip || skip > (ClInt32T) pCtrlHeader->length)
            {
                errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
                return errorCode;
            }
            bytesToSkip = (ClUint32T)skip;
            goto seek_set;
            /*unreached*/
        }
        break;

    case CL_BUFFER_SEEK_MAX : 
        break;
    }
    return CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
}

/*****************************************************************************/
static ClRcT
clBMBufferDataPrepend(ClBufferCtrlHeaderT *pCtrlHeader,
                      ClBufferHeaderT* pFirstBufferHeader, 
                      ClUint8T *pInputBuffer, 
                      ClUint32T numberOfBytes)
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClUint8T* pTemp = NULL;
    ClUint8T* pBufferToCopy = NULL;
    ClUint32T remainingSpace = 0;
       
    /* Dont allow more than what was allocated for the head room */
    remainingSpace = pFirstBufferHeader->startOffset - CL_BUFFER_OVERHEAD;
   
    if(remainingSpace < numberOfBytes)
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        return (errorCode);
    }

    pCurrentBufferHeader = pFirstBufferHeader;
    pTemp = pInputBuffer;
    
    pBufferToCopy = (ClUint8T*)pCurrentBufferHeader + 
        pCurrentBufferHeader->startOffset - numberOfBytes;
  
    memcpy(pBufferToCopy, pTemp, numberOfBytes);

    pCurrentBufferHeader->startOffset -= numberOfBytes;

    /*update the length*/
    pCtrlHeader->length += numberOfBytes;
    gClBufferLengthCheck(pCtrlHeader);

    return(CL_OK);
}

/*****************************************************************************/
static ClRcT
clBMBufferConcatenate(ClBufferCtrlHeaderT *pCtrlHeaderDestination,
                      ClBufferCtrlHeaderT *pCtrlHeaderSource,
                      ClBufferHeaderT* pDestination, 
                      ClBufferHeaderT* pSource
                      )
{
    ClBufferHeaderT* pTempBufferHeader = NULL;
	ClBufferHeaderT* pFreeBufferHeader = NULL;
    ClBufferHeaderT *pDestinationStart = pDestination;
    ClBufferHeaderT *pNext = NULL;
    ClBufferHeaderT *pTemp = NULL;
    ClUint32T nbytes = 0;
    ClRcT errorCode = CL_OK;

    /*
     * Traverse and free empty buffers in the destination    
     * which satisfy dataLength == startOffset
     * and then link the source up.
     */
    pDestination->actualLength = pDestination->dataLength;
	pTempBufferHeader = pDestination;
    pDestination = pDestination->pNextBufferHeader;
       
    while(NULL != pDestination)
    {
        if(pDestination->dataLength != pDestination->startOffset)
        {
            pDestination->actualLength = pDestination->dataLength;
            pTempBufferHeader = pDestination;
            pDestination = pDestination->pNextBufferHeader;
        }
        else
        {
            pFreeBufferHeader = pDestination;
            pDestination = pDestination->pNextBufferHeader;
            /* 
             * UnLink the pFreeBufferHeader from the chain
             *  previous has to exist as we are starting from
             *  second one by skipping the first ctrl header
             *  entry
             */
            pTempBufferHeader = pFreeBufferHeader->pPreviousBufferHeader;
            pTempBufferHeader->pNextBufferHeader = pDestination;                           
            if(pDestination)
            {
                pDestination->pPreviousBufferHeader = pTempBufferHeader;
            }
            pFreeBufferHeader->pNextBufferHeader = NULL;
            /* free the pFreeBufferHeader */
            clBMBufferChainFree(pCtrlHeaderDestination,pFreeBufferHeader);
        }
    }

    /*
     * Now this is tricky.
     * The source is getting sandwitched with the destination.
     * Since our first buffer header has the ctrl data,
     * we have to create a copy of the first one
     * if that has data and then free it.
     * Update the length of the destination also.
     */
    pCtrlHeaderDestination->length += pCtrlHeaderSource->length;

    /*Has to be NULL as we cannot concat. in betweens.*/
    CL_BUFFER_ASSERT(pSource->pPreviousBufferHeader == NULL);

    pTemp = pTempBufferHeader;
    
    nbytes = pSource->dataLength - pSource->startOffset;
    if(nbytes > 0 )
    {
        /*
         * Okay we have data, copy after creating a chain
         * with the required size
         */
        errorCode = clBMBufferChainAllocate(nbytes,&pTempBufferHeader);
        if(errorCode != CL_OK 
           ||
           pTempBufferHeader->pNextBufferHeader == NULL
           )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer concat: "
                                           "Error allocating memory for "
                                           "%d bytes\n",nbytes));
            goto out;
        }

        pTemp = pTempBufferHeader->pNextBufferHeader;

        /*
         * Has to be NULL as we cannot have multilinks
         * for a single chain copy
         */
        CL_BUFFER_ASSERT(pTemp->pNextBufferHeader == NULL);
        /*
         * Copy the data:
         * No need to touch anything as read and write offsets    
         * are reset anyway after the concat.
         */
        memcpy((ClUint8T*)((ClUint8T*)pTemp+pTemp->startOffset),
               (ClUint8T*)( (ClUint8T*)pSource + pSource->startOffset),
               (pSource->dataLength-pSource->startOffset)
               );
        pTemp->dataLength += pSource->dataLength - pSource->startOffset;
    }
    /*Now unlink source*/
    pNext = pSource->pNextBufferHeader;
    pSource->pNextBufferHeader = NULL;
    if(pNext)
    {
        pNext->pPreviousBufferHeader = pTemp;
    }
    pTemp->pNextBufferHeader = pNext;
    clBMBufferChainFree(pCtrlHeaderSource,pSource);

    /*reset write and read offsets*/
    pCtrlHeaderDestination->currentReadOffset = 0;
    pCtrlHeaderDestination->currentWriteOffset = 0;
    pCtrlHeaderDestination->pCurrentReadBuffer = pDestinationStart;
    pCtrlHeaderDestination->pCurrentWriteBuffer = pDestinationStart;

    out:
    gClBufferLengthCheck(pCtrlHeaderDestination);

    return errorCode;
}

/*****************************************************************************/
static ClRcT
clBMBufferHeaderTrim(ClBufferCtrlHeaderT *pCtrlHeader,
                     ClBufferHeaderT* pFirstBufferHeader, 
                     ClUint32T numberOfBytes
                     )
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClBufferHeaderT* pFreeBufferHeader = NULL;
    ClUint32T bytesStripped = 0;
    ClBoolT readBufferDeleted = CL_FALSE;
    ClBoolT writeBufferDeleted = CL_FALSE;

    /*Fast check to reset everything*/
    pCtrlHeader->length -= CL_MIN(numberOfBytes,
                                  pCtrlHeader->length);
    if(pCtrlHeader->length == 0 )
    {
        goto out_reset;
    }

    pCurrentBufferHeader = pFirstBufferHeader;
    
    while(numberOfBytes > 0 && pCurrentBufferHeader != NULL) {

        bytesStripped = CL_MIN(pCurrentBufferHeader->dataLength -
                               pCurrentBufferHeader->startOffset,
                               numberOfBytes
                               );
        numberOfBytes -= bytesStripped;
        pCurrentBufferHeader->startOffset += bytesStripped;
        if(numberOfBytes == 0)
        {
            if(readBufferDeleted == CL_TRUE)
            {
                pCtrlHeader->pCurrentReadBuffer = pCurrentBufferHeader;
            }
            if(writeBufferDeleted == CL_TRUE)
            {
                pCtrlHeader->pCurrentWriteBuffer = pCurrentBufferHeader;
            }
        }
        else 
        {
            if(readBufferDeleted == CL_FALSE)
            {
                if(pCtrlHeader->pCurrentReadBuffer == 
                   pCurrentBufferHeader
                   )
                {
                    readBufferDeleted = CL_TRUE;
                }
            }
            if(writeBufferDeleted == CL_FALSE)
            {
                if(pCtrlHeader->pCurrentWriteBuffer ==
                   pCurrentBufferHeader
                   )
                {
                    writeBufferDeleted = CL_TRUE;
                }
            }
        }
        if(pCurrentBufferHeader->writeOffset < 
           pCurrentBufferHeader->startOffset) 
        {
            pCurrentBufferHeader->writeOffset = 
                pCurrentBufferHeader->startOffset;
        }
        if(pCurrentBufferHeader->readOffset < 
           pCurrentBufferHeader->startOffset) 
        {
            pCurrentBufferHeader->readOffset = 
                pCurrentBufferHeader->startOffset;
        }            
        /*Update current read and write offset*/
        if(pCtrlHeader->currentReadOffset > 0 )
        {
            pCtrlHeader->currentReadOffset -= CL_MIN(bytesStripped,
                                                     pCtrlHeader->currentReadOffset);
        }
        if(pCtrlHeader->currentWriteOffset > 0 )
        {
            pCtrlHeader->currentWriteOffset -= CL_MIN(bytesStripped,
                                                      pCtrlHeader->currentWriteOffset);
        }
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
    }

    /*
     * We are here when the requested bytes are trimmed
     */
    CL_BUFFER_ASSERT(numberOfBytes <= 0 );
    goto out;

    /*reset the buffers. trim every one except the first one*/
    out_reset:    
    pFirstBufferHeader->dataLength = pFirstBufferHeader->startOffset;
    pFirstBufferHeader->writeOffset = pFirstBufferHeader->startOffset;
    pFirstBufferHeader->readOffset = pFirstBufferHeader->startOffset;    
    pCtrlHeader->currentReadOffset = 0;
    pCtrlHeader->currentWriteOffset = 0;
    pCtrlHeader->pCurrentReadBuffer = pFirstBufferHeader;
    pCtrlHeader->pCurrentWriteBuffer = pFirstBufferHeader;
    pFreeBufferHeader = pFirstBufferHeader->pNextBufferHeader;
    pFirstBufferHeader->pNextBufferHeader = NULL;
    if(pFreeBufferHeader)
    {
        errorCode = clBMBufferChainFree(pCtrlHeader,pFreeBufferHeader);
    }
    out:
    gClBufferLengthCheck(pCtrlHeader);
    return errorCode;
}

/*****************************************************************************/
static ClRcT
clBMBufferTrailerTrim(ClBufferCtrlHeaderT *pCtrlHeader,
                      ClBufferHeaderT *pFirstBufferHeader,
                      ClUint32T numberOfBytes
                      )
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClBufferHeaderT* pFreeBufferHeader = NULL;
    ClUint32T bytesStripped = 0;
    ClBoolT readBufferDeleted = CL_FALSE;
    ClBoolT writeBufferDeleted = CL_FALSE;
    
    /*Fast check to reset everything*/
    pCtrlHeader->length -= CL_MIN(numberOfBytes,pCtrlHeader->length);
    if(pCtrlHeader->length == 0 )
    {
        goto out_reset;
    }

    pCurrentBufferHeader = pFirstBufferHeader;
    /* Move to the Last Buffer header */
    while(pCurrentBufferHeader->pNextBufferHeader != NULL) 
    {
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
    }

    while(numberOfBytes > 0 && pCurrentBufferHeader != NULL) 
    {
        bytesStripped = CL_MIN(pCurrentBufferHeader->dataLength -
                               pCurrentBufferHeader->startOffset,
                               numberOfBytes);
        numberOfBytes -= bytesStripped;
        pCurrentBufferHeader->dataLength -= bytesStripped;
        if(numberOfBytes == 0)
        {
            if(readBufferDeleted == CL_TRUE)
            {
                pCtrlHeader->pCurrentReadBuffer = pCurrentBufferHeader;
            }
            if(writeBufferDeleted == CL_TRUE)
            {
                pCtrlHeader->pCurrentWriteBuffer = pCurrentBufferHeader;
            }
        }
        else 
        {
            if(readBufferDeleted == CL_FALSE)
            {
                if(pCtrlHeader->pCurrentReadBuffer == pCurrentBufferHeader)
                {
                    readBufferDeleted = CL_TRUE;
                }
            }
            if(writeBufferDeleted == CL_FALSE)
            {
                if(pCtrlHeader->pCurrentWriteBuffer == pCurrentBufferHeader)
                {
                    writeBufferDeleted = CL_TRUE;
                }
            }
        }
        if(pCurrentBufferHeader->writeOffset > pCurrentBufferHeader->dataLength)
        {
            pCurrentBufferHeader->writeOffset = pCurrentBufferHeader->dataLength;
        }
        if(pCurrentBufferHeader->readOffset > pCurrentBufferHeader->dataLength)
        {
            pCurrentBufferHeader->readOffset = pCurrentBufferHeader->dataLength;
        }
        /*Update current read and write offset*/
        if(pCtrlHeader->currentReadOffset > 0 )
        {
            pCtrlHeader->currentReadOffset -= CL_MIN(bytesStripped,
                                                     pCtrlHeader->currentReadOffset);
        }
        if(pCtrlHeader->currentWriteOffset > 0 )
        {
            pCtrlHeader->currentWriteOffset -= CL_MIN(bytesStripped,
                                                      pCtrlHeader->currentWriteOffset);
        }
        pCurrentBufferHeader = pCurrentBufferHeader->pPreviousBufferHeader;
    }
    CL_BUFFER_ASSERT(numberOfBytes <= 0);
    goto out;
    
    out_reset:
    pFirstBufferHeader->dataLength = pFirstBufferHeader->startOffset;
    pFirstBufferHeader->writeOffset = pFirstBufferHeader->startOffset;
    pFirstBufferHeader->readOffset = pFirstBufferHeader->startOffset;    
    pCtrlHeader->currentReadOffset = 0;
    pCtrlHeader->currentWriteOffset = 0;
    pCtrlHeader->pCurrentReadBuffer = pFirstBufferHeader;
    pCtrlHeader->pCurrentWriteBuffer = pFirstBufferHeader;
    pFreeBufferHeader = pFirstBufferHeader->pNextBufferHeader;
    pFirstBufferHeader->pNextBufferHeader = NULL;
    if(pFreeBufferHeader)
    {
        errorCode = clBMBufferChainFree(pCtrlHeader,pFreeBufferHeader);
    }
    out:
    gClBufferLengthCheck(pCtrlHeader);
    return(errorCode);
}

/*****************************************************************************/
static ClRcT
clBMBufferFlatten(ClBufferCtrlHeaderT *pCtrlHeader,
                  ClBufferHeaderT* pStartBufferHeader, 
                  ClUint8T** ppFlattenBuffer) 
{
    ClUint8T *pCopyToBuffer = NULL;
    ClUint32T len = 0;
    ClUint32T currentReadOffset = pCtrlHeader->currentReadOffset;
    ClUint32T readOffset = pStartBufferHeader->readOffset;
    ClBufferHeaderT *pCurrentReadBuffer = pCtrlHeader->pCurrentReadBuffer;
    ClRcT errorCode = CL_OK;

    NULL_CHECK(ppFlattenBuffer);

    *ppFlattenBuffer =  NULL;
    len = pCtrlHeader->length;

    /* Allocate buffer of required size */
    errorCode = CL_BUFFER_RC(CL_ERR_NO_MEMORY);
    pCopyToBuffer = (ClUint8T*) clHeapAllocate(len);
    if(NULL == pCopyToBuffer)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in allocating memory of size:%d\n",len));
        goto out;
    }
    pStartBufferHeader->readOffset = pStartBufferHeader->startOffset;
    errorCode = clBMBufferNBytesRead(pCtrlHeader,pStartBufferHeader,
                                     pCopyToBuffer,&len);
    if(errorCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in nbytes read of len:%d\n",len));
        clHeapFree(pCopyToBuffer);
        goto out_restore;
    }

    *ppFlattenBuffer = pCopyToBuffer;

    out_restore:
    pStartBufferHeader->readOffset = readOffset;
    pCtrlHeader->currentReadOffset = currentReadOffset;
    pCtrlHeader->pCurrentReadBuffer = pCurrentReadBuffer;
    out:
    return errorCode;
}

/*****************************************************************************/
static ClRcT GetStartChunkAndOffset(ClBufferHeaderT** pBufferHeader, ClUint32T* offset)
{
    int bytesTraversed;

    while(*pBufferHeader != NULL) {
    
        bytesTraversed = (*pBufferHeader)->dataLength - (*pBufferHeader)->startOffset;
        if(((int)*offset) <= bytesTraversed) 
        {
            break;
        }    
        *offset -= bytesTraversed;
        (*pBufferHeader) = (*pBufferHeader)->pNextBufferHeader;        
    }

    if(NULL == *pBufferHeader) 
    {
        ClRcT errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(errorCode, ("Buffer offset exceeds buffer length"));
        return(errorCode);
    }
    return CL_OK;
}


/*****************************************************************************/

static ClRcT
clBMBufferChainCopy(ClBufferHeaderT* pStartBufferHeader, 
                    ClUint32T offset, 
                    ClBufferCtrlHeaderT *pDestination,
                    ClUint32T numberOfBytes)
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClInt32T bytesToWrite = 0;
    ClUint8T* pTemp = NULL;    

    NULL_CHECK(pStartBufferHeader);
    NULL_CHECK(pDestination);

    /* Find the starting chunk, and the offset within that chunk */
    pCurrentBufferHeader = pStartBufferHeader;
    errorCode = GetStartChunkAndOffset(&pCurrentBufferHeader,&offset);
    CL_ASSERT(errorCode == CL_OK);
    
    while(pCurrentBufferHeader != NULL) {

        if(pCurrentBufferHeader->pool == CL_BUFFER_HEAP_MARKER)
        {
            pTemp = (ClUint8T*)pCurrentBufferHeader->pCookie + pCurrentBufferHeader->startOffset + offset;
        }
        else
        {
            pTemp = (ClUint8T*)pCurrentBufferHeader + pCurrentBufferHeader->startOffset + offset;
        }
        bytesToWrite = (ClInt32T)(pCurrentBufferHeader->dataLength - pCurrentBufferHeader->startOffset - offset);
        CL_ASSERT(bytesToWrite >= 0);
        bytesToWrite = CL_MIN(bytesToWrite,numberOfBytes);
        numberOfBytes -= bytesToWrite;
        offset = 0;
        errorCode = clBMBufferNBytesWrite(pDestination,
                                          pDestination->pCurrentWriteBuffer,
                                          pTemp,
                                          bytesToWrite
                                          );
        if(errorCode != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in nbytes write of %d bytes\n",bytesToWrite));
            return errorCode;
        }
        if((ClInt32T)numberOfBytes <= 0 )
        {
            return CL_OK;
        }
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
    }
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in chain copy\n"));
    errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
    clDbgCodeError(errorCode, ("Tried to copy beyond the end of the buffer"));
    return(errorCode);
}

/*****************************************************************************/
static ClRcT
clBMBufferChainDuplicate(ClBufferCtrlHeaderT *pCtrlHeaderSource,
                         ClBufferCtrlHeaderT **ppCtrlHeaderDestination,
                         ClBufferHeaderT* pStartBufferHeader, 
                         ClBufferHeaderT** ppFirstBuffer
                         )
{
    ClRcT errorCode = CL_OK;
    ClBufferHeaderT* pCurrentBufferHeader = NULL;
    ClBufferHeaderT* pPreviousBufferHeader = NULL;
    ClBufferHeaderT* pNewBufferHeader = NULL;
    ClBufferCtrlHeaderT *pCtrlHeaderDestination = NULL;
    ClPtrT pCookie = NULL;
    ClUint32T ctrlHeaderSize = CL_BUFFER_OVERHEAD_CTRL;
    ClUint32T actualLength = 0;
    ClUint8T* pChunk = NULL;
    ClUint8T* pReadBuffer = NULL;
    ClUint8T* pWriteBuffer = NULL;

    if(ppFirstBuffer == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Invalid parameter\n"));
        return CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
    }

    *ppFirstBuffer = NULL;
    pCurrentBufferHeader = pStartBufferHeader;
    
    while(NULL != pCurrentBufferHeader){
        /*get the pool*/
        actualLength = pCurrentBufferHeader->chunkSize;
        errorCode = gClBufferFromPoolAllocate (pCurrentBufferHeader->pool,
                                               pCurrentBufferHeader->chunkSize,
                                               &pChunk,&pCookie);
        if(errorCode != CL_OK)
		{
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error allocating buffer of size "
                                           " %d\n",
                                           pCurrentBufferHeader->chunkSize));
            return errorCode;
		}
		
        pNewBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pChunk + 
                                              ctrlHeaderSize
                                              );
        if(NULL == *ppFirstBuffer)
        {
            *ppFirstBuffer = pNewBufferHeader;
            pCtrlHeaderDestination = *ppCtrlHeaderDestination = 
                (ClBufferCtrlHeaderT*)pChunk;
        }

        pNewBufferHeader->actualLength = pCurrentBufferHeader->actualLength;
        actualLength -= ctrlHeaderSize + CL_BUFFER_OVERHEAD;

        if(ctrlHeaderSize > 0 )
        {
            ctrlHeaderSize = 0;
        }

        pReadBuffer = (ClUint8T*)((ClUint8T*)pCurrentBufferHeader + 
                                  CL_BUFFER_OVERHEAD);
        pWriteBuffer = (ClUint8T*)((ClUint8T*)pNewBufferHeader + 
                                   CL_BUFFER_OVERHEAD);

        /* copy the contents of the byte buffer into the new byte buffer */
        memcpy(pWriteBuffer, pReadBuffer, actualLength);
    
        pNewBufferHeader->startOffset = pCurrentBufferHeader->startOffset ;
        pNewBufferHeader->pCookie = pCookie;
        pNewBufferHeader->pool = pCurrentBufferHeader->pool;
        pNewBufferHeader->chunkSize = pCurrentBufferHeader->chunkSize;
        pNewBufferHeader->readOffset = pCurrentBufferHeader->readOffset;
        pNewBufferHeader->writeOffset = pCurrentBufferHeader->writeOffset;
        pNewBufferHeader->dataLength = pCurrentBufferHeader->dataLength;

        /* link the new buffer header */
        pNewBufferHeader->pNextBufferHeader = NULL;   

        pNewBufferHeader->pPreviousBufferHeader = pPreviousBufferHeader;
    
        if(pPreviousBufferHeader != NULL) 
        {
            pPreviousBufferHeader->pNextBufferHeader = pNewBufferHeader;
        }
    
        pPreviousBufferHeader = pNewBufferHeader;

        if(pCtrlHeaderSource->pCurrentReadBuffer == pCurrentBufferHeader) 
        {
            pCtrlHeaderDestination->pCurrentReadBuffer = pNewBufferHeader;
        }
    
        if(pCtrlHeaderSource->pCurrentWriteBuffer == pCurrentBufferHeader) 
        {
            pCtrlHeaderDestination->pCurrentWriteBuffer = pNewBufferHeader;
        }
    
        pCurrentBufferHeader = pCurrentBufferHeader->pNextBufferHeader;
    }
    /*copy the length*/
    if(pCtrlHeaderDestination)
    {
        pCtrlHeaderDestination->length = pCtrlHeaderSource->length;
        pCtrlHeaderDestination->currentReadOffset = pCtrlHeaderSource->currentReadOffset;
        pCtrlHeaderDestination->currentWriteOffset = pCtrlHeaderSource->currentWriteOffset;
    }
    return(CL_OK);
}

static ClRcT clBMBufferVectorize(ClBufferHeaderT *pBufferHeader,
                                 struct iovec **ppIOVector,
                                 ClInt32T *pNumVectors
                                 )
{
    struct iovec *pIOVector = NULL;
    ClInt32T numVectors=0;
    register ClBufferHeaderT *pTemp;

    for(pTemp = pBufferHeader; pTemp && ++numVectors; pTemp = pTemp->pNextBufferHeader);

    pIOVector = clHeapCalloc(numVectors, sizeof(*pIOVector));
    CL_ASSERT(pIOVector != NULL);
    numVectors = 0;
    for(pTemp = pBufferHeader; pTemp; pTemp = pTemp->pNextBufferHeader)
    {
        if(pTemp->pool == CL_BUFFER_HEAP_MARKER)
        {
            pIOVector[numVectors].iov_base = pTemp->pCookie;
            pIOVector[numVectors].iov_len =  pTemp->chunkSize;
        }
        else
        {
            pIOVector[numVectors].iov_base = (ClPtrT)((ClUint8T*)pTemp + pTemp->startOffset);
            pIOVector[numVectors].iov_len = pTemp->dataLength - pTemp->startOffset;
        }
        if(pIOVector[numVectors].iov_len == 0)
        {
            /*
             * Skip a null vector.
             */
            continue;
        }
        ++numVectors;
    }
    *ppIOVector = pIOVector;
    *pNumVectors = numVectors;

    return CL_OK;
}

/*
 * Interface to app start
 */
/*****************************************************************************/
ClRcT
clBufferInitialize(const ClBufferPoolConfigT *pConfig)
{
    ClRcT errorCode = CL_BUFFER_RC(CL_ERR_INITIALIZED);

    CL_FUNC_ENTER();

    if(gBufferManagementInfo.isInitialized == CL_TRUE) 
    {
        goto out;
    }

    errorCode = clOsalMutexInit(&gBufferManagementInfo.mutex);

    if(errorCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error initializing mutex\n"));
        goto out;
    }

    errorCode = clBufferPoolInitialize(pConfig);

    if(errorCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error initializing buffer pool:0x%x\n",errorCode));
        goto out;
    }

    gBufferManagementInfo.isInitialized = CL_TRUE;

    out:
    CL_FUNC_EXIT();
    return errorCode;
}

/*****************************************************************************/

ClRcT
clBufferFinalize ()
{
    ClRcT errorCode = CL_OK;
 
    CL_FUNC_ENTER();

    errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);

    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer Management is not initialised\n"));
        goto out;
    }

    errorCode = CL_OK;

    gBufferManagementInfo.isInitialized = CL_FALSE;
    
    clOsalMutexDestroy(&gBufferManagementInfo.mutex);

    clBufferPoolFinalize();

    CL_DEBUG_PRINT(CL_DEBUG_INFO,("Buffer Management clean up DONE\n"));

    out:
    CL_FUNC_EXIT();
    return errorCode;
}

/*****************************************************************************/

ClRcT
clBufferCreate (ClBufferHandleT *pBufferHandle)
{
    CL_FUNC_ENTER();
    CL_FUNC_EXIT();
    return clBufferCreateAndAllocate (CL_BUFFER_DEFAULT_SIZE,pBufferHandle);
}

/*****************************************************************************/
ClRcT
clBufferCreateAndAllocate (ClUint32T size, ClBufferHandleT *pBufferHandle)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pFirstBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pBufferHandle);

    /* Allocate default size of 2K if zero is passed */
    if(size == 0)
    {
        size = CL_BUFFER_DEFAULT_SIZE;
    }

    errorCode = clBMBufferChainAllocate (size,&pFirstBufferHeader);
   
    if(CL_OK != errorCode)
    {
        errorCode = CL_BUFFER_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    pCtrlHeader = (ClBufferCtrlHeaderT *) ((char*)pFirstBufferHeader - 
                                           CL_BUFFER_OVERHEAD_CTRL);

    pCtrlHeader->parent = NULL;
    pCtrlHeader->refCnt = 1;
    pCtrlHeader->bufferValidity = BUFFER_VALIDITY;
    pCtrlHeader->pCurrentWriteBuffer = pFirstBufferHeader;
    pCtrlHeader->pCurrentReadBuffer = pFirstBufferHeader;
    pCtrlHeader->currentReadOffset = 0;
    pCtrlHeader->currentWriteOffset = 0;
    *pBufferHandle= pCtrlHeader;
    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferDelete (ClBufferHandleT *pBufferHandle)
{
    ClRcT rc = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer Management is not initialized"));
        rc = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(rc);
    }

    /* Since multiple tasks delete the buffer, it is possible that bad timing will cause both
       to pass this point */
    /*  Need to use a different mutex or make it recursive
      clOsalMutexLock(&gBufferManagementInfo.mutex);
    */
    
    rc = CL_BUFFER_RC(CL_ERR_NULL_POINTER);
    if (pBufferHandle != NULL)
    {
        pCtrlHeader = (ClBufferCtrlHeaderT *)*pBufferHandle;
        if (pCtrlHeader != NULL)
        {
            rc = CL_BUFFER_RC(CL_ERR_INVALID_HANDLE);
            if (pCtrlHeader->bufferValidity == BUFFER_VALIDITY)
            {
                /*
                 * If this buffer has references, don't delete the chain.
                 */
                --pCtrlHeader->refCnt;
                if(pCtrlHeader->refCnt > 0)
                {
                    *pBufferHandle = 0;
                    return CL_OK;
                }
                /*
                 * Check if its a clone. and claim parent or decrement reference.
                 */
                if(pCtrlHeader->parent)
                {
                    --pCtrlHeader->parent->refCnt;
                    if(!pCtrlHeader->parent->refCnt)
                    {
                        pCtrlHeader->parent->bufferValidity = 0;
                        pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader->parent + 
                                                           CL_BUFFER_OVERHEAD_CTRL);
                        pBufferHeader->pPreviousBufferHeader = NULL;
                        rc = clBMBufferChainFree(pCtrlHeader->parent, pBufferHeader);
                    }
                }
                pCtrlHeader->bufferValidity = 0;
                pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader +
                                                   CL_BUFFER_OVERHEAD_CTRL);
                /*
                 * Just an invalid address/value check. before freeing only the ctrl header
                 * for a cloned chunk.
                 */
                if(pCtrlHeader->parent)
                {
                    pCtrlHeader->parent = NULL;
                    pBufferHeader->pNextBufferHeader = NULL;
                    pBufferHeader->pPreviousBufferHeader = NULL;
                }
                rc = clBMBufferChainFree(pCtrlHeader,pBufferHeader);

                *pBufferHandle = 0;
            }
            else
            {
                clDbgCodeError(rc,("Deleting an invalid buffer"));                
            }
            
        }
        else
        {
                clDbgCodeError(rc,("Deleting a null buffer handle"));                
        }        
    }
    else
    {
        clDbgCodeError(rc,("Passed a null buffer handle"));                
    }
    
    /* clOsalMutexUnlock(&gBufferManagementInfo.mutex); */
    CL_FUNC_EXIT();
    return(rc);
}

/*****************************************************************************/

ClRcT
clBufferClear (ClBufferHandleT bufferHandle)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
    
    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader +
                                       CL_BUFFER_OVERHEAD_CTRL);

    CL_FUNC_EXIT();
    return clBMChainReset(pCtrlHeader,pBufferHeader);
}

/*****************************************************************************/
ClRcT
clBufferLengthGet (ClBufferHandleT bufferHandle, ClUint32T *pBufferLength)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pBufferLength);
    
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;

    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    *pBufferLength = pCtrlHeader->length;

    CL_FUNC_EXIT();
    gClBufferLengthCheck(pCtrlHeader);
    return(CL_OK);
}

ClUint32T _clBufferLengthCalc (ClBufferCtrlHeaderT* pCtrlHeader)
{
    int chunkSum, dataSum;
    ClBufferHeaderT *pCurBuf;
    
    CL_FUNC_ENTER();

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    chunkSum=0;
    dataSum=0;

    pCurBuf = (ClBufferHeaderT*)((char*)pCtrlHeader + CL_BUFFER_OVERHEAD_CTRL);
   
    while(NULL != pCurBuf)
    {
        /* ChunkSum is the total available buffer space, dataSum is how much data there is. */
        chunkSum += pCurBuf->actualLength;
        dataSum  += pCurBuf->dataLength - pCurBuf->startOffset;
        
        pCurBuf = pCurBuf->pNextBufferHeader;
    }


    return dataSum;
}

ClRcT clBufferLengthCalc (ClBufferHandleT bufferHandle)
{
    ClBufferCtrlHeaderT* pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    ClUint32T len = _clBufferLengthCalc(pCtrlHeader);
   
    pCtrlHeader->length = len;
    return CL_OK;
}

/*
 * Just a placeholder for the length check pointer.
 * Will compiler inline this! It should.
*/

static ClUint32T clBufferLengthCheck(ClBufferCtrlHeaderT *pCtrlHeader)
{
    return 0U;
}

static ClUint32T clBufferLengthCheckDebug(ClBufferCtrlHeaderT* pCtrlHeader)
{
    ClInt32T chunkSum=0;
    ClUint32T dataSum=0;
    ClBufferHeaderT *pCurBuf;
    
    CL_FUNC_ENTER();

    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    pCurBuf = (ClBufferHeaderT*)((char*)pCtrlHeader + CL_BUFFER_OVERHEAD_CTRL);
   
    while(NULL != pCurBuf)
    {
        /* ChunkSum is the total available buffer space, dataSum is how much data there is. */
        chunkSum += pCurBuf->actualLength;
        dataSum  += pCurBuf->dataLength - pCurBuf->startOffset;
        
        pCurBuf = pCurBuf->pNextBufferHeader;
    }

    if (dataSum != pCtrlHeader->length)
    {
        clDbgCodeError(0, ("Inconsistent total length"));
    }

    return dataSum;
}



/*****************************************************************************/

ClRcT
clBufferNBytesRead (ClBufferHandleT bufferHandle, 
                    ClUint8T *pByteBuffer, 
                    ClUint32T* pNumberOfBytesToRead)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    NULL_CHECK(pByteBuffer);
    NULL_CHECK(pNumberOfBytesToRead);

    errorCode = clBMBufferNBytesRead(pCtrlHeader,
                                     pCtrlHeader->pCurrentReadBuffer,
                                     pByteBuffer, 
                                     pNumberOfBytesToRead
                                     );
    CL_FUNC_EXIT();
    return(errorCode);
}

/*****************************************************************************/

ClRcT clDbgBufferPrint(ClBufferHandleT bufferHdl)
{
    ClUint8T* buf;
    unsigned int len;
    clBufferLengthGet(bufferHdl,&len);
    buf = clHeapAllocate(len+1);
    assert(buf);
    
    clBufferNBytesRead(bufferHdl,buf,&len);
    buf[len]=0;
    CL_DEBUG_PRINT(CL_DEBUG_INFO,("Buffer [%p] length [%d] is [%s]",bufferHdl,len,buf));
    printf("Buffer [%p] length [%d] is [%s]\n",bufferHdl,len,buf);
    clHeapFree(buf);
    return CL_OK;
}

/*****************************************************************************/

ClRcT
clBufferNBytesWrite (ClBufferHandleT bufferHandle, ClUint8T *pByteBuffer, 
                     ClUint32T numberOfBytesToWrite)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        return(errorCode);
    }

    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    NULL_CHECK(pByteBuffer);

    if(pCtrlHeader->parent)
    {
        ClBufferCtrlHeaderT *dupCtrlHeader = NULL;
        ClBufferHeaderT *dupBufferHeader = NULL;
        ClBufferHeaderT *bufferHeader = NULL;
        ClUint32T resetMode = 0;

        /*
         * If the parent is already deleted and has only a single reference, just use the parent buffer.
         * instead of duplicating it.
         */
        if(pCtrlHeader->parent->refCnt > 1)
        {
            errorCode = clBMBufferChainDuplicate(pCtrlHeader->parent, &dupCtrlHeader,
                                                 (ClBufferHeaderT*)
                                                 ((ClUint8T*)pCtrlHeader->parent + CL_BUFFER_OVERHEAD_CTRL),
                                                 &dupBufferHeader);
            if(errorCode != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error [%#x] duplicating a cloned buffer\n", errorCode));
                CL_FUNC_EXIT();
                return errorCode;
            }
        }
        else
        {
            dupCtrlHeader = pCtrlHeader->parent;
            dupBufferHeader = (ClBufferHeaderT*)( (ClUint8T*)pCtrlHeader->parent + CL_BUFFER_OVERHEAD_CTRL );
        }
        bufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader + CL_BUFFER_OVERHEAD_CTRL);
        if(dupCtrlHeader->pCurrentReadBuffer == dupBufferHeader)
        {
            pCtrlHeader->pCurrentReadBuffer = bufferHeader;
        }
        if(dupCtrlHeader->pCurrentWriteBuffer == dupBufferHeader)
        {
            pCtrlHeader->pCurrentWriteBuffer = bufferHeader;
        }
        pCtrlHeader->currentReadOffset = dupCtrlHeader->currentReadOffset;
        pCtrlHeader->currentWriteOffset = dupCtrlHeader->currentWriteOffset;
        pCtrlHeader->length = dupCtrlHeader->length;
        /*
         * Copy the data from the first header.
         */
        memcpy((ClUint8T*)bufferHeader + CL_BUFFER_OVERHEAD,
               (ClUint8T*)dupBufferHeader + CL_BUFFER_OVERHEAD,
               bufferHeader->chunkSize - CL_BUFFER_OVERHEAD_CTRL - CL_BUFFER_OVERHEAD);
        bufferHeader->startOffset = dupBufferHeader->startOffset;
        bufferHeader->dataLength =  dupBufferHeader->dataLength;
        bufferHeader->readOffset =  dupBufferHeader->readOffset;
        bufferHeader->writeOffset = dupBufferHeader->writeOffset;
        bufferHeader->actualLength = dupBufferHeader->actualLength;
        --pCtrlHeader->parent->refCnt;
        pCtrlHeader->parent = NULL;
        pCtrlHeader->refCnt = 1;
        bufferHeader->pPreviousBufferHeader = NULL;
        bufferHeader->pNextBufferHeader = dupBufferHeader->pNextBufferHeader;
        if(bufferHeader->pNextBufferHeader)
            bufferHeader->pNextBufferHeader->pPreviousBufferHeader = bufferHeader;
        dupBufferHeader->pNextBufferHeader = NULL;
        /*  
         * Free the first chain now.
         */
        errorCode = clBMBufferChainFree(dupCtrlHeader, dupBufferHeader);
        if(CL_OK != errorCode)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error [%#x] freeing the cloned duplicate chain error\n", errorCode));
        }
        /*
         * Update the read buffer in the new COW chain.
         */
        if(pCtrlHeader->currentReadOffset <= pCtrlHeader->length)
        {
            errorCode = clBMBufferReadOffsetSet(pCtrlHeader, bufferHeader, 
                                                pCtrlHeader->currentReadOffset, CL_BUFFER_SEEK_SET);
            if(CL_OK != errorCode)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error [%#x] setting read offset to [%d] bytes in the cloned buffer\n",
                                                errorCode, pCtrlHeader->currentReadOffset));
                resetMode |= 1;
            }
        }
        else
        {
            resetMode |= 1;
        }
        if(pCtrlHeader->currentWriteOffset <= pCtrlHeader->length)
        {
            errorCode = clBMBufferWriteOffsetSet(pCtrlHeader, bufferHeader,
                                                 pCtrlHeader->currentWriteOffset, CL_BUFFER_SEEK_SET);
            if(CL_OK != errorCode)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Error [%#x] setting write offset to [%d] bytes in the cloned buffer\n",
                                                errorCode, pCtrlHeader->currentWriteOffset));
                resetMode |= 2;
            }
        }
        else
        {
            resetMode |= 2;
        }
        if( (resetMode & 1) )
        {
            pCtrlHeader->currentReadOffset = 0;
            pCtrlHeader->pCurrentReadBuffer = bufferHeader;
            bufferHeader->readOffset = bufferHeader->startOffset;
        }
        if( (resetMode & 2) )
        {
            pCtrlHeader->currentWriteOffset = 0;
            pCtrlHeader->pCurrentWriteBuffer = bufferHeader;
            bufferHeader->writeOffset = bufferHeader->startOffset;
        }
    }
    errorCode = clBMBufferNBytesWrite (pCtrlHeader,
                                       pCtrlHeader->pCurrentWriteBuffer, 
                                       pByteBuffer, 
                                       numberOfBytesToWrite
                                       );
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}
/*****************************************************************************/

ClRcT
clBufferChecksum16Compute(ClBufferHandleT bufferHandle, 
                          ClUint32T startOffset, 
                          ClUint32T length, 
                          ClUint16T* pChecksum)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
   
    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pChecksum);

    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    CL_FUNC_EXIT();
    return clBMBufferChainChecksum16Compute(pBufferHeader, startOffset, length, pChecksum);
}

/*****************************************************************************/

ClRcT
clBufferChecksum32Compute(ClBufferHandleT bufferHandle, 
                          ClUint32T startOffset, 
                          ClUint32T length, 
                          ClUint32T* pChecksum)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
   
    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pChecksum);

    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    NULL_CHECK(pBufferHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    CL_FUNC_EXIT();
    return clBMBufferChainChecksum32Compute(pBufferHeader, startOffset, 
                                            length, pChecksum);
}

/*****************************************************************************/

ClRcT
clBufferDataPrepend (ClBufferHandleT bufferHandle, ClUint8T *pByteBuffer, 
                     ClUint32T numberOfBytesToWrite)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + 
                                       CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    NULL_CHECK(pBufferHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);
    
    NULL_CHECK(pByteBuffer);
    
    errorCode = clBMBufferDataPrepend(pCtrlHeader,pBufferHeader, 
                                      pByteBuffer, 
                                      numberOfBytesToWrite);
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferConcatenate (ClBufferHandleT destination, ClBufferHandleT *pSource)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pSourceHeader = NULL;
    ClBufferCtrlHeaderT* pDestinationHeader = NULL;
    ClBufferHeaderT* pSourceBuffer = NULL;
    ClBufferHeaderT* pDestinationBuffer = NULL;
   
    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    NULL_CHECK(pSource);
    
    pSourceHeader = (ClBufferCtrlHeaderT *)*pSource;

    pDestinationHeader = (ClBufferCtrlHeaderT *)destination;
    
    NULL_CHECK(pSourceHeader);

    NULL_CHECK(pDestinationHeader);

    BUFFER_VALIDITY_CHECK(pSourceHeader);

    BUFFER_VALIDITY_CHECK(pDestinationHeader);

    if(pSourceHeader->parent || pDestinationHeader->parent)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nCannot concatenate cloned buffers\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return errorCode;
    }

    pSourceBuffer = (ClBufferHeaderT*)((char*)*pSource + CL_BUFFER_OVERHEAD_CTRL);
    pDestinationBuffer = (ClBufferHeaderT*)((char*)destination + CL_BUFFER_OVERHEAD_CTRL);

    errorCode = clBMBufferConcatenate(pDestinationHeader,
                                      pSourceHeader,
                                      pDestinationBuffer, pSourceBuffer);
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    *pSource = 0;
	
	CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferReadOffsetGet (ClBufferHandleT bufferHandle, ClUint32T *pReadOffset)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pReadOffset);

    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;

    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    *pReadOffset = pCtrlHeader->currentReadOffset;

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferWriteOffsetGet (ClBufferHandleT bufferHandle, ClUint32T *pWriteOffset)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    NULL_CHECK(pWriteOffset);

    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;

    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    *pWriteOffset = pCtrlHeader->currentWriteOffset;

    CL_FUNC_EXIT();

    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferReadOffsetSet (ClBufferHandleT bufferHandle, ClInt32T newReadOffset, ClBufferSeekTypeT seekType)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
        
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    NULL_CHECK(pCtrlHeader);
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + CL_BUFFER_OVERHEAD_CTRL);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    if(seekType >= CL_BUFFER_SEEK_MAX) 
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    errorCode = clBMBufferReadOffsetSet(pCtrlHeader,
                                        pBufferHeader, 
                                        newReadOffset, 
                                        seekType
                                        );
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferWriteOffsetSet (ClBufferHandleT bufferHandle, ClInt32T newWriteOffset, ClBufferSeekTypeT seekType)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
        
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + 
                                       CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    NULL_CHECK(pBufferHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    if(seekType >= CL_BUFFER_SEEK_MAX) 
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    errorCode = clBMBufferWriteOffsetSet(pCtrlHeader,
                                         pBufferHeader, 
                                         newWriteOffset, 
                                         seekType
                                         );
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferHeaderTrim (ClBufferHandleT bufferHandle, ClUint32T numberOfBytes)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
    ClUint32T length = 0;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
        
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    length = pCtrlHeader->length;

    if(numberOfBytes > length) 
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    errorCode = clBMBufferHeaderTrim(pCtrlHeader,
                                     pBufferHeader, numberOfBytes
                                     );
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }
    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferTrailerTrim (ClBufferHandleT bufferHandle, ClUint32T numberOfBytes)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
        
    pCtrlHeader = (ClBufferCtrlHeaderT *)bufferHandle;
    pBufferHeader = (ClBufferHeaderT*)((char*)bufferHandle + CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    errorCode = clBMBufferTrailerTrim(pCtrlHeader,pBufferHeader,
                                      numberOfBytes
                                      );

    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferToBufferCopy(ClBufferHandleT source, ClUint32T sourceOffset, ClBufferHandleT destination, ClUint32T numberOfBytes)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pSource = NULL;
    ClBufferCtrlHeaderT *pDestination = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
   
    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
        
    pSource = (ClBufferCtrlHeaderT *)source;
    pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pSource + 
                                       CL_BUFFER_OVERHEAD_CTRL);

    NULL_CHECK(pSource);

    BUFFER_VALIDITY_CHECK(pSource);

    pDestination = (ClBufferCtrlHeaderT*)destination;
    
    NULL_CHECK(pDestination);

    BUFFER_VALIDITY_CHECK(pDestination);

    errorCode = clBMBufferChainCopy(pBufferHeader, sourceOffset,pDestination,numberOfBytes);
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*
 * Just append an existing heap buffer into the chain. This should be a fast way of merging in large
 * buffers during the IDL/XDR marshall step over duplicating large heaps slowing down performance
 * The heap buffer is expected to be a large chunk on the heap to avoid compromising on heap performance
 */
ClRcT
clBufferAppendHeap (ClBufferHandleT source, ClUint8T *buffer, ClUint32T size)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();

    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    if(!buffer)
    {
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return errorCode;
    }

    /*
     * IDL/XDR sometimes passes it. So ignore.
     */
    if(!size)
    {
        CL_FUNC_EXIT();
        return CL_OK;
    }

    pCtrlHeader = (ClBufferCtrlHeaderT *)source;
    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    if(pCtrlHeader->refCnt > 1 || pCtrlHeader->parent)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Can't append a heap chain to a cloned buffer\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_SUPPORTED);
        CL_FUNC_EXIT();
        return errorCode;
    }
    if(!pCtrlHeader->pCurrentWriteBuffer)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Can't append a heap chain without a write buffer\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_INVALID_STATE);
        CL_FUNC_EXIT();
        return errorCode;
    }
    /*
     * Check if the appended heap buffer is a large chunk.
     */
    if( ( errorCode = clHeapReferenceLargeChunk((ClPtrT)buffer, size) ) != CL_OK)
    {
        CL_FUNC_EXIT();
        return errorCode;
    }

    /*
     * Create a new buffer header to hold the heap. reference
     */
    pBufferHeader = (ClBufferHeaderT*)clHeapCalloc(1, sizeof(*pBufferHeader));
    CL_ASSERT(pBufferHeader != NULL);

    pBufferHeader->readOffset = pBufferHeader->startOffset = 0;
    pBufferHeader->dataLength  = size;
    pBufferHeader->actualLength = pBufferHeader->dataLength;
    pBufferHeader->chunkSize = size;
    pBufferHeader->pCookie = (void*)buffer;
    pBufferHeader->pool = CL_BUFFER_HEAP_MARKER; /* note that its a heap chain */
    pBufferHeader->pNextBufferHeader = pCtrlHeader->pCurrentWriteBuffer->pNextBufferHeader;
    pBufferHeader->pPreviousBufferHeader = pCtrlHeader->pCurrentWriteBuffer;
    pCtrlHeader->pCurrentWriteBuffer->pNextBufferHeader = pBufferHeader;
    pCtrlHeader->pCurrentWriteBuffer = pBufferHeader;
    pCtrlHeader->currentWriteOffset += size;
    pCtrlHeader->length += size;
    pBufferHeader->writeOffset = pBufferHeader->dataLength;

    CL_FUNC_EXIT();
    return(CL_OK);
}

ClRcT
clBufferClone (ClBufferHandleT source, ClBufferHandleT *pDuplicate)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferCtrlHeaderT *pCloneHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
    ClBufferHeaderT* pFirstBufferHeader = NULL;
    ClPtrT pCookie = NULL;

    CL_FUNC_ENTER();

    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    NULL_CHECK(pDuplicate);

    pCtrlHeader = (ClBufferCtrlHeaderT *)source;
    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    /*
     * Find the actual parent as we could also clone an already cloned source.
     */
    while(pCtrlHeader->parent)
        pCtrlHeader = pCtrlHeader->parent;

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader +
                                       CL_BUFFER_OVERHEAD_CTRL);
    
    /*
     * Just clone the first and COW the rest.
     */
    errorCode = gClBufferFromPoolAllocate(pBufferHeader->pool,
                                          pBufferHeader->chunkSize,
                                          (ClUint8T**)&pCloneHeader, &pCookie);

    if(errorCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in buffer clone allocate\n"));
        CL_FUNC_EXIT();
        return (errorCode);
    }
    /*
     * Just copy the ctrl header chunk. The cloned chunk can read in the forward direction.
     */
    memcpy(pCloneHeader, pCtrlHeader, pBufferHeader->chunkSize);
    pFirstBufferHeader = (ClBufferHeaderT*) ((ClUint8T*)pCloneHeader + CL_BUFFER_OVERHEAD_CTRL);
    pFirstBufferHeader->pCookie = pCookie;
    pFirstBufferHeader->pPreviousBufferHeader = NULL;
    pCloneHeader->refCnt = 1;
    pCloneHeader->parent = pCtrlHeader; /* point to the parent*/
    ++pCloneHeader->parent->refCnt;
    /*
     * The previous for the first chain would be pointing to the parent and we can leave it untouched.
     */
    *pDuplicate = pCloneHeader;

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/

ClRcT
clBufferDuplicate (ClBufferHandleT source, ClBufferHandleT *pDuplicate)
{
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferCtrlHeaderT* pDuplicateHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;
    ClBufferHeaderT* pFirstBufferHeader = NULL;

    CL_FUNC_ENTER();

    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    NULL_CHECK(pDuplicate);

    pCtrlHeader = (ClBufferCtrlHeaderT *)source;

    NULL_CHECK(pCtrlHeader);

    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader +
                                       CL_BUFFER_OVERHEAD_CTRL);

    pFirstBufferHeader = NULL;

    errorCode = clBMBufferChainDuplicate(pCtrlHeader,
                                         &pDuplicateHeader,
                                         pBufferHeader, 
                                         &pFirstBufferHeader
                                         );
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    if(NULL == pFirstBufferHeader || pDuplicateHeader == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error in duplicate\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return (errorCode);
    }

    pDuplicateHeader->bufferValidity = BUFFER_VALIDITY;
    pDuplicateHeader->refCnt = 1;
    pDuplicateHeader->parent = NULL;
    *pDuplicate = pDuplicateHeader;

    CL_FUNC_EXIT();
    return(CL_OK);
}

/*****************************************************************************/
ClRcT
clBufferFlatten(ClBufferHandleT source, 
                ClUint8T** ppFlattenBuffer) 
{   
    ClRcT errorCode = CL_OK;
    ClBufferCtrlHeaderT* pCtrlHeader = NULL;
    ClBufferHeaderT* pBufferHeader = NULL;

    CL_FUNC_ENTER();
    if(gBufferManagementInfo.isInitialized == CL_FALSE) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nBuffer Management is not initialized\n"));
        errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
        CL_FUNC_EXIT();
        return(errorCode);
    }
    
    NULL_CHECK(ppFlattenBuffer);

    pCtrlHeader = (ClBufferCtrlHeaderT *)source;

    NULL_CHECK(pCtrlHeader);
    BUFFER_VALIDITY_CHECK(pCtrlHeader);

    pBufferHeader = (ClBufferHeaderT*)((ClUint8T*)pCtrlHeader +
                                       CL_BUFFER_OVERHEAD_CTRL);

    errorCode = clBMBufferFlatten(pCtrlHeader,pBufferHeader, ppFlattenBuffer);
    if(CL_OK != errorCode) 
    {
        CL_FUNC_EXIT();
        return(errorCode);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT clBufferStatsGet(ClMemStatsT *pBufferStats)
{
    ClRcT errorCode = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);

    if(gBufferManagementInfo.isInitialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Stats get:buffer not initialized\n"));
        goto out;
    }

    NULL_CHECK(pBufferStats);

    clOsalMutexLock(&gBufferManagementInfo.mutex);
    memcpy(pBufferStats,&gBufferManagementInfo.memStats,sizeof(*pBufferStats));
    clOsalMutexUnlock(&gBufferManagementInfo.mutex);

    pBufferStats->numPools = gBufferManagementInfo.bufferPoolConfig.numPools;
    errorCode = CL_OK;

    out:
    return errorCode;
}

/*****************************************************************************/

ClRcT clBufferPoolStatsGet(ClUint32T numPools,ClUint32T *pPoolSize,ClPoolStatsT *pPoolStats)
{
    ClRcT errorCode = CL_OK;

    if(gBufferManagementInfo.isInitialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Stats get:buffer not initialized\n"));
        goto out;
    }

    NULL_CHECK(pPoolStats);

    NULL_CHECK(pPoolSize);

    if(gBufferManagementInfo.bufferPoolConfig.mode == CL_BUFFER_NATIVE_MODE)
    {
        memset(pPoolStats,0,sizeof(*pPoolStats)*numPools);

        *pPoolSize = 0;
        
        errorCode = CL_OK;
        
        goto out;
    }

    errorCode = clBMBufferPoolStatsGet(numPools,pPoolSize,pPoolStats);

    if(errorCode != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error getting buffer pool stats\n"));
        goto out;
    }

    out:
    return errorCode;
}

ClRcT clBufferShrink(ClPoolShrinkOptionsT *pShrinkOptions)
{
    ClPoolShrinkOptionsT defaultOptions;
    defaultOptions.shrinkFlags = CL_POOL_SHRINK_DEFAULT;

    register ClUint32T i;

    ClRcT rc = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);

    if(gBufferManagementInfo.isInitialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer shrink:Buffer mgmt. uninitialized\n"));
        goto out;
    }

    if(pShrinkOptions == NULL)
    {
        pShrinkOptions = &defaultOptions;
    }

    for(i = 0; i < gBufferManagementInfo.bufferPoolConfig.numPools;++i)
    {
        ClPoolT pool = gBufferManagementInfo.pPoolHandles[i];
        rc = clPoolShrink(pool,pShrinkOptions);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Error shrinking %d sized pool\n",gBufferManagementInfo.bufferPoolConfig.pPoolConfig[i].chunkSize));
            goto out;
        }
    }
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clBufferVectorize(ClBufferHandleT buffer,struct iovec **ppIOVector,ClInt32T *pNumVectors)
{
    ClRcT rc = CL_BUFFER_RC(CL_ERR_NOT_INITIALIZED);
    ClBufferCtrlHeaderT *pCtrlHeader = (ClBufferCtrlHeaderT*)buffer;
    ClBufferHeaderT *pBufferHeader;

    if(gBufferManagementInfo.isInitialized == CL_FALSE)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Buffer not initialized\n"));
        goto out;
    }
    NULL_CHECK(pCtrlHeader);
    NULL_CHECK(ppIOVector);
    NULL_CHECK(pNumVectors);
    pBufferHeader = (ClBufferHeaderT*)
        ((ClUint8T*)pCtrlHeader + CL_BUFFER_OVERHEAD_CTRL);
    rc = clBMBufferVectorize(pBufferHeader,ppIOVector,pNumVectors);
    out:
    return rc;
}

/*****************************************************************************/
/*
 * Deprecated functions start
 */
ClRcT 
clBufferMessageCreate(ClBufferHandleT *pMessageHandle)
{
    CL_BUFFER_DEPRECATED();
    return clBufferCreate(pMessageHandle);
}

ClRcT
clBufferMessageCreateAndAllocate(ClUint32T size, ClBufferHandleT *pMessageHandle)
{
    CL_BUFFER_DEPRECATED();
    return clBufferCreateAndAllocate(size,pMessageHandle);
}

ClRcT
clBufferMessageDelete(ClBufferHandleT *pMessageHandle)
{
    CL_BUFFER_DEPRECATED();
    return clBufferDelete(pMessageHandle);
}

ClRcT
clBufferMessageClear(ClBufferHandleT messageHandle) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferClear(messageHandle);
}

ClRcT
clBufferMessageLengthGet(ClBufferHandleT messageHandle, ClUint32T *pMessageLength) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferLengthGet(messageHandle,pMessageLength);
}

ClRcT
clBufferMessageNBytesRead(ClBufferHandleT messageHandle, 
                          ClUint8T *pByteBuffer, 
                          ClUint32T* pNumberOfBytesToRead)
{
    CL_BUFFER_DEPRECATED();
    return clBufferNBytesRead(messageHandle,pByteBuffer,pNumberOfBytesToRead);
}

ClRcT
clBufferMessageNBytesWrite(ClBufferHandleT messageHandle, 
                           ClUint8T *pByteBuffer, 
                           ClUint32T numberOfBytesToWrite)
{
    CL_BUFFER_DEPRECATED();
    return clBufferNBytesWrite(messageHandle,pByteBuffer,numberOfBytesToWrite);
}

ClRcT
clBufferMessageDataPrepend(ClBufferHandleT messageHandle, 
                           ClUint8T *pByteBuffer, 
                           ClUint32T numberOfBytesToWrite) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferDataPrepend(messageHandle,pByteBuffer,numberOfBytesToWrite);
}

ClRcT
clBufferMessageConcatenate(ClBufferHandleT destination, ClBufferHandleT *pSource) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferConcatenate(destination,pSource);
}

ClRcT
clBufferMessageReadOffsetGet(ClBufferHandleT messageHandle,
                             ClUint32T *pReadOffset) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferReadOffsetGet(messageHandle,pReadOffset);
}

ClRcT
clBufferMessageWriteOffsetGet(ClBufferHandleT messageHandle,
                              ClUint32T *pWriteOffset) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferWriteOffsetGet(messageHandle,pWriteOffset);
}

ClRcT
clBufferMessageReadOffsetSet(ClBufferHandleT messageHandle,
                             ClInt32T newReadOffset,
                             ClBufferSeekTypeT seekType)
{
    CL_BUFFER_DEPRECATED();
    return clBufferReadOffsetSet(messageHandle,newReadOffset,seekType);
}

ClRcT
clBufferMessageWriteOffsetSet(ClBufferHandleT messageHandle,
                              ClInt32T newWriteOffset,
                              ClBufferSeekTypeT seekType) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferWriteOffsetSet(messageHandle,newWriteOffset,seekType);
}

ClRcT
clBufferMessageHeaderTrim(ClBufferHandleT messageHandle,
                          ClUint32T numberOfBytes) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferHeaderTrim(messageHandle,numberOfBytes);
}

ClRcT
clBufferMessageTrailerTrim(ClBufferHandleT messageHandle,
                           ClUint32T numberOfBytes)
{
    CL_BUFFER_DEPRECATED();
    return clBufferTrailerTrim(messageHandle,numberOfBytes);
}

ClRcT
clBufferMessageToMessageCopy(ClBufferHandleT source,
                             ClUint32T sourceOffset,
                             ClBufferHandleT destination,
                             ClUint32T numberOfBytes) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferToBufferCopy(source,sourceOffset,destination,numberOfBytes);
}

ClRcT
clBufferMessageDuplicate(ClBufferHandleT source,
                         ClBufferHandleT *pDuplicate) 
{
    CL_BUFFER_DEPRECATED();
    return clBufferDuplicate(source,pDuplicate);
}

/*
 * Deprecated functions end
 */
#ifdef BUFFER_CLONE_TEST
ClRcT clBufferCloneTest(void)
{
    ClRcT rc;
    ClBufferHandleT buffer;
    ClBufferHandleT cloneBuffers[2];
    ClUint8T buf[4096];
    ClUint8T readBuff[4096];
    ClUint8T readBuff2[4096<<1];
    ClUint32T c;
    ClUint32T i;
    rc = clBufferCreate(&buffer);
    CL_ASSERT(rc == CL_OK);
    memset(buf, 0xa5, sizeof(buf));
    rc = clBufferNBytesWrite(buffer, buf, sizeof(buf));
    CL_ASSERT(rc == CL_OK);
    for(i = 0; i < sizeof(cloneBuffers)/sizeof(cloneBuffers[0]); ++i)
    {
        rc = clBufferClone(buffer, &cloneBuffers[i]);
        CL_ASSERT(rc == CL_OK);
        rc = clBufferReadOffsetSet(cloneBuffers[i], 0, CL_BUFFER_SEEK_SET);
        CL_ASSERT(rc == CL_OK);
        c = sizeof(readBuff);
        rc = clBufferNBytesRead(cloneBuffers[i], readBuff, &c);
        CL_ASSERT(rc == CL_OK);
        CL_ASSERT(c == sizeof(readBuff));
        CL_ASSERT(memcmp(buf, readBuff, c) == 0);
        /* 
         *optional. delete.
         */
        if(i + 1 == sizeof(cloneBuffers)/sizeof(cloneBuffers[0]))
        {
            rc = clBufferDelete(&buffer);
            CL_ASSERT(rc == CL_OK);
        }
        rc = clBufferReadOffsetSet(cloneBuffers[i], 0, CL_BUFFER_SEEK_SET);
        CL_ASSERT(rc == CL_OK);
        rc = clBufferNBytesRead(cloneBuffers[i], readBuff, &c);
        CL_ASSERT(rc == CL_OK);
        CL_ASSERT(c == sizeof(readBuff));
        CL_ASSERT(memcmp(readBuff, buf, c) == 0);
        /*
         * Force a COW on the clone buffer.
         */
        rc = clBufferNBytesWrite(cloneBuffers[i], buf, c);
        CL_ASSERT(rc == CL_OK);
        rc = clBufferWriteOffsetGet(cloneBuffers[i], &c);
        CL_ASSERT(rc == CL_OK);
        clLogNotice("CLONE", "TEST", "Write offset at [%d] for clone buffer [%d]", c, i);
        rc = clBufferLengthGet(cloneBuffers[i], &c);
        CL_ASSERT(rc == CL_OK);
        clLogNotice("CLONE", "TEST", "Len [%d] for clone buffer [%d]", c, i);
        rc = clBufferReadOffsetSet(cloneBuffers[i], 0, CL_BUFFER_SEEK_SET);
        CL_ASSERT(rc == CL_OK);
        c = sizeof(readBuff2);
        rc = clBufferNBytesRead(cloneBuffers[i], readBuff2, &c);
        CL_ASSERT(rc == CL_OK);
        clLogNotice("CLONE", "TEST", "Expected [%d], read [%d] bytes for clone buffer [%d]", 
                    (ClUint32T)sizeof(readBuff2), c, i);
        CL_ASSERT(c == sizeof(readBuff2));
        CL_ASSERT(memcmp(readBuff2, buf, c>>1) == 0);
        CL_ASSERT(memcmp(readBuff2 + (c>>1), buf, c>>1) == 0);
    }
    if(buffer)
    {
        rc = clBufferDelete(&buffer);
        CL_ASSERT(rc == CL_OK);
    }
    for(i = 0; i < sizeof(cloneBuffers)/sizeof(cloneBuffers[0]); ++i)
    {
        rc = clBufferDelete(&cloneBuffers[i]);
        CL_ASSERT(rc == CL_OK);
    }
    clLogNotice("CLONE", "TEST", "Buffer clone success");
    return rc;
}
#endif
