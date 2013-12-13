
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
#define _CL_HEAP_C_
#include <stdio.h>
#include <stdlib.h>
#ifndef VXWORKS_BUILD
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/mman.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clBinSearchApi.h>
#include <clPoolIpi.h>
#include <clMemStats.h>
#include <clHeapApi.h>
#include <clEoIpi.h>
#include <clUtilsIpi.h>
#include <clRbTree.h>

#ifdef VXWORKS_BUILD
#include <clMemPart.h>
#endif
/*Macro definitions*/

#define CL_HEAP_RC(rc)   (CL_RC(CL_CID_HEAP,(rc)))

#ifdef SOLARIS_BUILD
#define CL_HEAP_OVERHEAD (sizeof (ClUint64T))
#else
#define CL_HEAP_OVERHEAD (sizeof (ClPtrT))
#endif

/*
 * Below size for large chunk should be optimal considering we are 50% of the max. buffer or
 * fragmentation limit.
 */
#define CL_HEAP_LARGE_CHUNK_SIZE (16384)

#define CL_HEAP_SET_ADDRESS(address,overhead)               \
    (address) = (ClPtrT*) ((ClUint8T*)(address) + (overhead))

#define __CL_HEAP_SET_NATIVE_COOKIE(address,cookie) \
    ( (ClUint32T*)(address) )[0] = (cookie)

#define __CL_HEAP_GET_NATIVE_COOKIE(address,cookie) \
    cookie = ((ClUint32T*)(address))[0]

#define __CL_HEAP_SET_COOKIE(address,cookie)                        \
    ((ClPtrT*)(address))[0] = (ClPtrT)(cookie)
    
#define __CL_HEAP_GET_COOKIE(address,cookie)            \
    cookie = (ClPtrT)((ClPtrT*)(address))[0]

#define CL_HEAP_SET_NATIVE_COOKIE(address,cookie) do {  \
    __CL_HEAP_SET_NATIVE_COOKIE(address,cookie);        \
    CL_HEAP_SET_ADDRESS(address,CL_HEAP_OVERHEAD);      \
}while(0)

#define CL_HEAP_GET_NATIVE_COOKIE(address,cookie) do {  \
    CL_HEAP_SET_ADDRESS(address,-CL_HEAP_OVERHEAD);     \
    __CL_HEAP_GET_NATIVE_COOKIE(address,cookie);        \
}while(0)

#define CL_HEAP_SET_COOKIE(pool,address,cookie) do {    \
    __CL_HEAP_SET_COOKIE(address,cookie);               \
    CL_HEAP_SET_ADDRESS(address,CL_HEAP_OVERHEAD);      \
}while(0)

#define CL_HEAP_GET_COOKIE(address,cookie) do {     \
    CL_HEAP_SET_ADDRESS(address,-CL_HEAP_OVERHEAD); \
    __CL_HEAP_GET_COOKIE(address,cookie);           \
}while(0)

#define CL_HEAP_GRANT_SIZE(size) ( clEoMemAdmitAllocate(size) )

#define CL_HEAP_REVOKE_SIZE(size) (clEoMemNotifyFree(size))

#define CL_HEAP_LOG(sev,str,...) clEoLibLog(CL_CID_HEAP,sev,str,__VA_ARGS__)

#define CL_HEAP_LOG_WRAP(dir) clEoLibLog(CL_CID_HEAP,CL_LOG_SEV_TRACE,gClHeapLogWrapStrList[(dir)])

#define __CL_HEAP_STATS_UPDATE(size,memDir) do {        \
    ClBoolT wrapped = CL_FALSE;                         \
    switch((memDir))                                    \
    {                                                   \
    case CL_MEM_ALLOC:                                  \
        CL_MEM_STATS_UPDATE_ALLOC(gHeapList.memStats,   \
                                  size,wrapped);        \
        break;                                          \
    case CL_MEM_FREE:                                   \
        CL_MEM_STATS_UPDATE_FREE(gHeapList.memStats,    \
                                 size,wrapped);         \
        break;                                          \
    default: ;                                          \
    }                                                   \
    if(wrapped == CL_TRUE)                              \
    {                                                   \
        CL_HEAP_LOG_WRAP(memDir);                       \
    }                                                   \
}while(0)

#define CL_HEAP_STATS_UPDATE(size,memDir) do {  \
    clOsalMutexLock(&gHeapList.mutex);          \
    __CL_HEAP_STATS_UPDATE(size,memDir);        \
    clOsalMutexUnlock(&gHeapList.mutex);        \
}while(0)

#define __CL_HEAP_STATS_UPDATE_INCR(memDir) do {                \
    ClBoolT wrapped = CL_FALSE;                                 \
    switch((memDir))                                            \
    {                                                           \
    case CL_MEM_ALLOC:                                          \
        CL_MEM_STATS_UPDATE_ALLOCS(gHeapList.memStats,wrapped); \
        break;                                                  \
    case CL_MEM_FREE:                                           \
        CL_MEM_STATS_UPDATE_FREES(gHeapList.memStats,wrapped);  \
        break;                                                  \
    default:                                                    \
        ;                                                       \
    }                                                           \
    if(wrapped == CL_TRUE)                                      \
    {                                                           \
        CL_HEAP_LOG_WRAP(memDir);                               \
    }                                                           \
}while(0)

#define CL_HEAP_STATS_UPDATE_INCR(memDir) do {  \
    clOsalMutexLock(&gHeapList.mutex);          \
    __CL_HEAP_STATS_UPDATE_INCR(memDir);        \
    clOsalMutexUnlock(&gHeapList.mutex);        \
}while(0)

#define CL_HEAP_WATERMARKS_UPDATE(memDir) do {  \
    clEoMemWaterMarksUpdate(memDir);            \
}while(0)

#define CL_HEAP_POOL_FLAGS_SET(config,flags) do {   \
    flags = CL_POOL_DEFAULT_FLAG;                   \
    if((config).lazy == CL_TRUE)                    \
    {                                               \
        flags = (ClPoolFlagsT) (flags | CL_POOL_LAZY_FLAG); \
    }                                               \
    if(gHeapDebugLevel > 0 )                        \
    {                                               \
        flags = (ClPoolFlagsT) (flags | CL_POOL_DEBUG_FLAG); \
    }                                               \
}while(0)

/*
 * Okay, small games for purify.
 * We go to native debug mode for Purify with stats
 * collection disabled as native mode we collect stats.
 * Thats the only thing we need.
*/
#ifdef WITH_PURIFY

#define CL_HEAP_PURIFY_HOOK(mode) do {          \
    mode = CL_HEAP_NATIVE_MODE;                 \
    if(!gHeapDebugLevel)                        \
    {                                           \
        gHeapDebugLevel = 1;                    \
    }                                           \
}while(0)

#else

#define CL_HEAP_PURIFY_HOOK(mode)

#endif

#define CL_HEAP_ALLOC_EXTERNAL(size) clHeapAllocExternal(size)

#define CL_HEAP_FREE_EXTERNAL(pChunk,size) clHeapFreeExternal(pChunk,size)

#undef NULL_CHECK

#define NULL_CHECK(X) do {                      \
    if(NULL == (X))                             \
    {                                           \
        return CL_HEAP_RC(CL_ERR_NULL_POINTER); \
    }                                           \
}while(0)

static ClBoolT gClHeapCheckerRequired;

#ifndef POSIX_BUILD

static ClBoolT gClHeapCheckerInitialized;
static pthread_mutex_t gClHeapCheckerMutex = PTHREAD_MUTEX_INITIALIZER;
static void clHeapCheckerInitialize(void);

/*
 * static branch prediction hint where likely implies that the branch
 * would be most likely true and unlikely implies that the branch would almost
 * never be taken or condition most likely false.
 */
#define CL_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define CL_UNLIKELY(expr) __builtin_expect(!!(expr), 0)

#define CL_HEAP_CHECKER_INITIALIZE do {         \
    if(CL_UNLIKELY(gClHeapCheckerRequired))     \
        clHeapCheckerInitialize();              \
}while(0)

#define CL_HEAP_CHECKER_ALLOC(size) do {            \
    if(CL_UNLIKELY(gClHeapCheckerRequired))         \
    {                                               \
        if(CL_UNLIKELY(!gClHeapCheckerInitialized)) \
            clHeapCheckerInitialize();              \
        if(strstr(gClEoFilter, CL_EO_NAME))         \
            return __clHeapCheckerAllocate(size);   \
    }                                               \
}while(0)

#define CL_HEAP_CHECKER_REALLOC(ptr, newSize) do {          \
     if(CL_UNLIKELY(gClHeapCheckerRequired))                \
     {                                                      \
         if(CL_UNLIKELY(!gClHeapCheckerInitialized))        \
             clHeapCheckerInitialize();                     \
         if(strstr(gClEoFilter, CL_EO_NAME))                \
             return __clHeapCheckerRealloc(ptr, newSize);   \
     }                                                      \
}while(0)

#define CL_HEAP_CHECKER_CALLOC(numChunks, chunkSize) do {       \
    if(CL_UNLIKELY(gClHeapCheckerRequired))                     \
    {                                                           \
        if(CL_UNLIKELY(!gClHeapCheckerInitialized))             \
            clHeapCheckerInitialize();                          \
        if(strstr(gClEoFilter, CL_EO_NAME))                     \
            return __clHeapCheckerCalloc(numChunks, chunkSize); \
    }                                                           \
}while(0)

#define CL_HEAP_CHECKER_FREE(ptr) do {              \
    if(CL_UNLIKELY(gClHeapCheckerRequired))         \
    {                                               \
        if(CL_UNLIKELY(!gClHeapCheckerInitialized)) \
            clHeapCheckerInitialize();              \
        if(strstr(gClEoFilter, CL_EO_NAME))         \
        {                                           \
            __clHeapCheckerFree(ptr);               \
            return;                                 \
        }                                           \
    }                                               \
}while(0)

#define FREELIST_SIZE (8192)

#define FREELIST_MASK (FREELIST_SIZE - 1)

static ClUint32T gClFreeListIndex;
static ClUint32T gClFreeListIndexMask = FREELIST_MASK;
static ClCharT gClEoFilter[0xff+1];
static ClTimeT gClFreeListTime;
static ClUint32T gClPageSize;

typedef struct ClFreeList
{
    ClPtrT pAddress;
    ClUint32T size;
}ClFreeListT;

static ClFreeListT gClFreeList[FREELIST_SIZE];

static void
clHeapCheckerInitialize(void)
{
    ClCharT *pFreeListSize =  NULL;
    ClCharT *pEoFilter = NULL;
    ClCharT *pFreeListTime = NULL;
    if(gClHeapCheckerInitialized) return;
    gClPageSize = getpagesize();
    if( (pFreeListSize = getenv("CL_FREE_LIST_SIZE")))
    {
        ClUint32T freeListSize = atoi(pFreeListSize);
        if(freeListSize < FREELIST_SIZE)
        {
            ClUint32T shift = 0;
            for(shift = 0; (((ClUint32T)1)<<shift) < freeListSize; ++shift);
            freeListSize = 1 << shift;
            CL_ASSERT(freeListSize <= FREELIST_SIZE);
            gClFreeListIndexMask = freeListSize-1;
        }
    }
    gClFreeListTime = 60;
    if( (pFreeListTime = getenv("CL_FREE_LIST_TIME")))
    {
        gClFreeListTime = atoi(pFreeListTime);
        gClFreeListTime = CL_MIN(gClFreeListTime, 300);
    }
    gClFreeListTime *= 1000;

    if((pEoFilter = getenv("CL_EO_FILTER")))
    {
        strncpy(gClEoFilter, pEoFilter, sizeof(gClEoFilter)-1);
    }
    else
    {
        strncpy(gClEoFilter, "AMF", sizeof(gClEoFilter)-1);
    }
    clLogNotice("HEAP", "CHECKER", 
                "Initializing heap checker with free list mask [%d], eo filter [%s], free list time [%lld]",
                gClFreeListIndexMask, gClEoFilter, gClFreeListTime);
    gClHeapCheckerInitialized = CL_TRUE;
}

static ClPtrT 
__clHeapCheckerAllocate(ClUint32T size)
{
    void *pAddress = NULL;
    ClUint32T origSize = size;
    ClUint32T pageSize = gClPageSize;
    int flags = MAP_ANONYMOUS;
    int err = 0;
    origSize += (sizeof(ClWordT)-1);
    origSize &= ~(sizeof(ClWordT)-1);
    origSize += sizeof(ClWordT);
    size = origSize;
    size += (pageSize - 1);
    size &= ~(pageSize - 1);
#ifndef POSIX_BUILD
    flags |= MAP_PRIVATE;
#endif
    pAddress = mmap(0, size + pageSize, PROT_READ|PROT_WRITE, flags, -1, 0);
    if(pAddress == MAP_FAILED)
        CL_ASSERT(0);
    err = mprotect((ClUint8T*)pAddress + size, pageSize, PROT_NONE);
    if(err)
    {
        clLogError("HEAP", "CHECK", "mprotect returned [%d:%s]", errno, strerror(errno));
        fflush(stdout);
        sleep(1);
        CL_ASSERT(0);
    }
    
    /*
      clLogNotice("HEAP", "ALLOC", "Mmapped/mprotected address [%p:%p], size [%d], origSize [%d]",
                  pAddress, (ClUint8T*)pAddress + size, size, origSize - (ClUint32T)sizeof(long));
    */
    pAddress = (ClUint8T*)pAddress + size;
    pAddress = (ClUint8T*)pAddress - origSize;
    *(ClUint32T*)pAddress = (origSize  - sizeof(ClWordT));
    return (ClPtrT)((ClWordT*)pAddress+1);
}

static ClTimeT
clHeapCheckerTimeGet(void)
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ClTimeT)ts.tv_sec * 1000000 + ts.tv_nsec/1000;
}

static void
__clHeapCheckerFree(ClPtrT pAddress)
{
    ClUint32T pageSize  = gClPageSize;
    static ClTimeT lastFreeTime;
    ClTimeT currentFreeTime = 0;

    /*
     * Disable frees and mprotect boundary;
     */
    pthread_mutex_lock(&gClHeapCheckerMutex);
    currentFreeTime = clHeapCheckerTimeGet();
    if(pAddress)
    {
        ClUint32T index = 0;
        ClUint32T size = *(ClUint32T*)((ClWordT*)pAddress - 1);
        ClUint32T origSize = size;
        ClPtrT origAddress = pAddress;
        int err;
        size = origSize + sizeof(ClWordT);
        size += (pageSize - 1);
        size &= ~(pageSize - 1);
        pAddress = (ClPtrT)((ClUint8T*)pAddress + origSize - size);
        err = mprotect(pAddress, size, PROT_NONE);
        if(err)
        {
            clLogError("FREE", "MAP", "mprotect for address [%p], orig [%p], size [%d], origSize [%d], returned [%d:%s]", 
                       pAddress, origAddress, size, origSize, errno, strerror(errno));
            sleep(1);
            fflush(stdout);
            CL_ASSERT(err == 0);
        }
        /*  
            clLogNotice("FREE", "MAP", "mprotected address [%p], size [%d], origSize [%d]",
            pAddress, size, origSize);
        */
        if(gClFreeListIndex >= gClFreeListIndexMask
           ||
           (
            lastFreeTime && currentFreeTime &&
            (currentFreeTime - lastFreeTime)/1000L > gClFreeListTime)
           )
        {
            ClUint32T i;
            for(i = 0; i < gClFreeListIndex; ++i)
            {
                if(gClFreeList[i].pAddress)
                {
                    err = mprotect(gClFreeList[i].pAddress, gClFreeList[i].size + pageSize, PROT_READ | PROT_WRITE);
                    if(err)
                    {
                        clLogError("FREE", "MAP", "munprotect for address [%p, size [%d] returned [%d:%s]", 
                                   gClFreeList[i].pAddress, gClFreeList[i].size, errno, strerror(errno));
                        fflush(stdout);
                        sleep(1);
                        CL_ASSERT(0);
                    }
                    err = munmap(gClFreeList[i].pAddress, gClFreeList[i].size + pageSize);
                    if(err)
                    {
                        clLogError("FREE", "MAP", "munmap for address [%p], size [%d] returned [%d:%s]",
                                   gClFreeList[i].pAddress, gClFreeList[i].size, errno, strerror(errno));
                        fflush(stdout);
                        sleep(1);
                        CL_ASSERT(err == 0);
                    }
                    gClFreeList[i].pAddress = NULL;
                    gClFreeList[i].size = 0;
                }
            }
            if(gClFreeListIndex >= gClFreeListIndexMask)
                gClFreeListIndex = 0;
            lastFreeTime = clHeapCheckerTimeGet();
        }
        index = gClFreeListIndex++;
        gClFreeListIndex &= gClFreeListIndexMask;
        gClFreeList[index].pAddress = pAddress;
        gClFreeList[index].size = size;
    }
    pthread_mutex_unlock(&gClHeapCheckerMutex);
}

static ClPtrT __clHeapCheckerRealloc(ClPtrT pAddress, ClUint32T newSize)
{
    ClUint32T origSize = 0;
    ClUint32T size = 0;
    ClPtrT pOldAddress = pAddress;
    ClPtrT pNewAddress = NULL;
    ClUint32T pageSize = gClPageSize;
    if(!pAddress)
        return __clHeapCheckerAllocate(newSize);
    origSize = *(ClUint32T*)((ClWordT*)pAddress - 1);
    if(origSize >= newSize)
    {
        return pAddress;
    }
    size = origSize + sizeof(ClWordT);
    size += pageSize -1;
    size &= ~(pageSize - 1);
    pNewAddress = __clHeapCheckerAllocate(newSize);
    memcpy(pNewAddress, pOldAddress, origSize);
    __clHeapCheckerFree(pOldAddress);
    return pNewAddress;
}

static ClPtrT 
__clHeapCheckerCalloc(ClUint32T numChunks, ClUint32T chunkSize)
{
    return __clHeapCheckerAllocate(numChunks*chunkSize);
}

#else

#define CL_HEAP_CHECKER_INITIALIZE 

#define CL_HEAP_CHECKER_ALLOC(size) 

#define CL_HEAP_CHECKER_REALLOC(ptr, newSize)

#define CL_HEAP_CHECKER_CALLOC(numChunks, chunkSize)

#define CL_HEAP_CHECKER_FREE(ptr)

#endif

/*
 * Introduce the DEBUG stubs
*/
#include "clHeapDebug.h"

/*Data definitions*/

/*Heap list*/
typedef struct
{
    ClHeapConfigT heapConfig;/*heap configuration*/
    ClPoolT *pPoolHandles; /*list of pool handles*/
    ClMemStatsT memStats; /*heap stats go here*/
    ClOsalMutexT mutex;/*mutex for heap op.*/
    ClBoolT initialized;/*whether the pool system is initialized or not*/
    ClUint32T numLargeChunks; /* count of large chunks */
    ClRbTreeRootT largeChunkTree; /* heap allocation tracker for large chunks*/
} ClHeapListT;

struct ClHeapLargeChunk
{
    ClRbTreeT tree;
    ClUint32T magic;
    ClPtrT chunk; /* the chunk itself */
    ClUint16T refCnt; /* 16 bit references are fine */
} __attribute__((__aligned__(16)));

#define CL_HEAP_LARGE_CHUNK_OVERHEAD ( sizeof(ClHeapLargeChunkT) )
#define CL_HEAP_LARGE_CHUNK_MAGIC  (0x78563412)

typedef struct ClHeapLargeChunk ClHeapLargeChunkT;

static ClInt32T heapCmpLargeChunk(ClRbTreeT *, ClRbTreeT *);
static ClPtrT (*gClHeapAllocate) ( ClUint32T size );
static void   (*gClHeapFree) (ClPtrT);
static ClPtrT (*gClHeapRealloc) ( ClPtrT,ClUint32T size );
static ClPtrT (*gClHeapCalloc) (ClUint32T,ClUint32T);

static const ClCharT* gClHeapLogWrapStrList[] = {
    "numAlloc statistics wrapped around",
    "numFree statistics wrapped around",
};

/*For test mode, you can override heap with this config.*/

ClHeapConfigT *pHeapConfigUser = NULL;

#ifdef __cplusplus
static ClHeapListT gHeapList = {};
#else
static ClHeapListT gHeapList =
{
    .initialized = CL_FALSE,
};
#endif

#define HEAP_LOG_AREA			"HEP"
#define HEAP_LOG_CTX_INI		"INI"
#define HEAP_LOG_CTX_FINALISE	"FIN"
#define HEAP_LOG_CTX_GET		"GET"
#define HEAP_LOG_CTX_REGISTER	"REG"
#define HEAP_LOG_CTX_DEREGISTER	"DREG"
#define HEAP_LOG_CTX_ALLOCATION	"ALLOC"

/*Function declarations*/
static ClPtrT heapAllocatePreAllocated(ClUint32T size);
static ClPtrT heapReallocPreAllocated(ClPtrT pAddress,ClUint32T size);
static ClPtrT heapCallocPreAllocated(ClUint32T numChunks,ClUint32T chunkSize);
static void heapFreePreAllocated(ClPtrT pAddress);
static ClPtrT heapAllocateNative(ClUint32T size);
static ClPtrT heapReallocNative(ClPtrT pAddress,ClUint32T size);
static ClPtrT heapCallocNative(ClUint32T numChunks,ClUint32T chunkSize);
static void heapFreeNative(ClPtrT pAddress);

#ifdef VXWORKS_BUILD

#define CL_HEAP_MEM_PART_SIZE (4<<20U)
/*
 * Allocate 8 megs
 */
ClMemPartHandleT gClMemPartHandle;

static ClRcT
heapLibVxWorksInitialize(void)
{
    return clMemPartInitialize(&gClMemPartHandle, CL_HEAP_MEM_PART_SIZE);
}

static ClRcT
heapLibVxWorksFinalize(void)
{
    return clMemPartFinalize(&gClMemPartHandle);
}

static ClPtrT
heapAllocateNativeVxWorks(ClUint32T size)
{
    return clMemPartAlloc(gClMemPartHandle, size);
}

static ClPtrT
heapCallocNativeVxWorks(ClUint32T numChunks, ClUint32T chunkSize)
{
    return clMemPartAlloc(gClMemPartHandle, numChunks * chunkSize);
}

static ClPtrT
heapReallocNativeVxWorks(ClPtrT memBase, ClUint32T size)
{
    return clMemPartRealloc(gClMemPartHandle, memBase, size);
}

static void
heapFreeNativeVxWorks(ClPtrT mem)
{
    clMemPartFree(gClMemPartHandle, mem);
}

#endif

/*Call malloc/free as of now before moving to a buddy*/
static
__inline__
ClPtrT
clHeapAllocExternal(
        ClUint32T size)
{
    ClPtrT pAddress = NULL;

    if(CL_HEAP_GRANT_SIZE(size) == CL_TRUE)
    {
        pAddress = malloc(size);
        if (NULL == pAddress)
        {
            CL_HEAP_REVOKE_SIZE(size);
        }
        else
        {
            CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);
        }
    }
    return pAddress;
}

static
__inline__
void
clHeapFreeExternal(
        ClPtrT pAddress,
        ClUint32T size)
{
    CL_HEAP_REVOKE_SIZE(size);
    CL_HEAP_WATERMARKS_UPDATE(CL_MEM_FREE);
    free(pAddress);
}

static
ClInt32T
clPoolCmp(
        const void *a,
        const void *b)
{
    return ((ClPoolConfigT *)a)->chunkSize - ( (ClPoolConfigT*)b )->chunkSize;
}

/*
 * Set the debug level for the heap.
 * > 1 enables mem tracking by logging to process specific file under
 * sandbox/log/mem_logs directory
*/

static
__inline__
void
clHeapModeSet(
              ClHeapModeT mode)
{
    switch(mode)
    {
    case CL_HEAP_NATIVE_MODE:
        gClHeapAllocate = heapAllocateNative;
        gClHeapFree = heapFreeNative;
        gClHeapRealloc = heapReallocNative;
        gClHeapCalloc = heapCallocNative;
        CL_HEAP_NATIVE_DEBUG_SET();
#ifdef VXWORKS_BUILD
        if(clParseEnvBoolean("CL_MEM_PARTITION"))
        {
            gClHeapAllocate = heapAllocateNativeVxWorks;
            gClHeapFree = heapFreeNativeVxWorks;
            gClHeapRealloc = heapReallocNativeVxWorks;
            gClHeapCalloc = heapCallocNativeVxWorks;
        }
#endif
        break;
    case CL_HEAP_PREALLOCATED_MODE:
    default:
        gClHeapAllocate = heapAllocatePreAllocated;
        gClHeapFree = heapFreePreAllocated;
        gClHeapRealloc = heapReallocPreAllocated;
        gClHeapCalloc = heapCallocPreAllocated;
        CL_HEAP_PREALLOCATED_DEBUG_SET();
        break;
    }
}

/*  Constructs pre-defined pools.Wont be around when EO is through. */
ClRcT clHeapInit(void) 
{
    /*100 MB per component right now*/
#define EO_MEM_LIMIT  ( 50 << 20 )
    ClRcT rc = CL_OK;
    extern ClHeapConfigT *pHeapConfigUser;
    ClHeapConfigT *pHeapConfig = pHeapConfigUser;
    static ClEoMemConfigT memConfig = {
        EO_MEM_LIMIT,        
        {30,45},
        {55,70},
        {80,95}
    };

    if(gHeapList.initialized == CL_TRUE)
    {
        
        return rc;
    }
    /*
     * Just do this here for the time being as we are 
     * just working for EO right now
    */
    rc = clMemStatsInitialize(&memConfig);
    CL_ASSERT(rc==CL_OK);

    /*generate if none is specified*/
    if(pHeapConfig == NULL)
    {
        /*
         * increase this as of now if you wanna a success
         * for mem allocations.
        */
#define MAX_CHUNK_SIZE (2<<20)
#define MAX_POOL_SIZE CL_POOL_MAX_SIZE
#define REALLOC_GRANULARITY (0x8)
#define REALLOC_MASK (REALLOC_GRANULARITY-1)
        /*Pool config for allocation*/
        static ClHeapConfigT gHeapConfig;
        gHeapConfig.mode = CL_HEAP_PREALLOCATED_MODE;
        
        ClUint32T chunkSize = 32;
        ClUint32T pageSize = getpagesize();
        ClPoolConfigT *pPoolConfig = NULL;
        ClUint32T numPools = 0;

        pHeapConfig = &gHeapConfig;
        pPoolConfig = (ClPoolConfigT*) realloc(NULL,sizeof(*pPoolConfig) * REALLOC_GRANULARITY);
        if(pPoolConfig == NULL)
        {
            return CL_HEAP_RC(CL_ERR_NO_MEMORY);
        }

        while(chunkSize <= MAX_CHUNK_SIZE)
        {
            ClUint32T initialPoolSize;
            ClUint32T incrementPoolSize;
            ClUint32T maxPoolSize = MAX_POOL_SIZE;

            if(numPools && !(numPools & REALLOC_MASK))
            {
                ClUint32T allocPools = numPools + REALLOC_GRANULARITY;
                pPoolConfig = (ClPoolConfigT*) realloc(pPoolConfig,allocPools*sizeof(*pPoolConfig));
                if(!pPoolConfig)
                {
                    return CL_HEAP_RC(CL_ERR_NO_MEMORY);
                }
            }
            incrementPoolSize = chunkSize * 8;
            
            if(incrementPoolSize < pageSize)
            {
                incrementPoolSize = pageSize;
            }
            initialPoolSize = incrementPoolSize * 2;

            pPoolConfig[numPools].chunkSize = chunkSize;
            pPoolConfig[numPools].initialPoolSize = initialPoolSize;
            pPoolConfig[numPools].incrementPoolSize = incrementPoolSize;
            pPoolConfig[numPools].maxPoolSize = maxPoolSize;
            ++numPools;
            chunkSize <<= 1;
        }
        gHeapConfig.pPoolConfig = pPoolConfig;
        gHeapConfig.numPools = numPools;
        gHeapConfig.lazy = CL_FALSE;
    }
    rc = clHeapLibInitialize(pHeapConfig);
    CL_ASSERT(rc==CL_OK);
    
    return rc;
}

ClRcT clHeapExit(void)
{
    ClRcT rc = CL_OK;
    if(gHeapList.initialized == CL_FALSE)
    {
        return rc;
    }
    rc = clHeapLibFinalize();
    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_FINALISE,"Error in pool finalize");
    }
    clMemStatsFinalize();
    return rc;
}

ClRcT clHeapModeGet(ClHeapModeT *pMode)
{
    ClRcT rc ;
    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);
    CL_ASSERT(gHeapList.initialized);
    NULL_CHECK(pMode);
    rc = CL_OK;
    *pMode = gHeapList.heapConfig.mode;
    return rc;
}

ClRcT clHeapShrink(const ClPoolShrinkOptionsT *pShrinkOptions)
{
    ClRcT rc = CL_OK;
    register ClUint32T i;
    ClPoolShrinkOptionsT defaultShrinkOptions;
    defaultShrinkOptions.shrinkFlags = CL_POOL_SHRINK_DEFAULT;

    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);
    CL_ASSERT(gHeapList.initialized);

    if(pShrinkOptions == NULL)
    {
        pShrinkOptions = &defaultShrinkOptions;
    }
    for(i = 0; i < gHeapList.heapConfig.numPools; ++i)
    {
        ClPoolT pool = gHeapList.pPoolHandles[i];
        rc = clPoolShrink(pool,pShrinkOptions);
        if(rc != CL_OK)
        {
            clLogError(HEAP_LOG_AREA,CL_LOG_CONTEXT_UNSPECIFIED,"Error shrinking %d sized pool\n",gHeapList.heapConfig.pPoolConfig[i].chunkSize);
            goto out;
        }
    }
    rc = CL_OK;
    out:
    return rc;
}

ClRcT clHeapStatsGet(ClMemStatsT *pHeapStats)
{
    ClRcT rc  = CL_OK;

    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);
    CL_ASSERT(gHeapList.initialized);

    NULL_CHECK(pHeapStats);

    clOsalMutexLock(&gHeapList.mutex);
    memcpy(pHeapStats,&gHeapList.memStats,sizeof(*pHeapStats));
    clOsalMutexUnlock(&gHeapList.mutex);

    pHeapStats->numPools = gHeapList.heapConfig.numPools;
    rc = CL_OK;
    
    return rc;
}

ClRcT clHeapPoolStatsGet(ClUint32T numPools,ClUint32T *pPoolSize,ClPoolStatsT *pPoolStats)
{
    ClRcT rc = CL_OK;
    ClUint32T currentPoolSize = 0;
    register ClUint32T i;

    NULL_CHECK(pPoolStats);

    NULL_CHECK(pPoolSize);

    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);
    CL_ASSERT(gHeapList.initialized);

    numPools = CL_MIN(numPools,gHeapList.heapConfig.numPools);

    for(i = 0; i < numPools; ++i)
    {
        rc = clPoolStatsGet(gHeapList.pPoolHandles[i],pPoolStats+i);
        if(rc != CL_OK)
        {
            clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_GET,"Error fetching pool stats for pool:%d\n",gHeapList.heapConfig.pPoolConfig[i].chunkSize);
            goto out;
        }
        currentPoolSize += pPoolStats[i].numExtendedPools *
            pPoolStats[i].poolConfig.incrementPoolSize;
    }
    *pPoolSize = currentPoolSize;
    rc = CL_OK;
    
    out:
    return rc;
}

/*
 * This API is for custom pools if user wants to define their own.
 * The ramifications are too large for allowing this to happen
 * when the clovis heap system is initialised.
 * To prevent/detect cross pool freeing - Pool migration, pool freeze 
 * and unfreeze needs to be tackled for allowing this to happen.
 * Can be done but not required actually.
*/
  
ClRcT clHeapHooksRegister(ClPtrT (*allocHook) (ClUint32T),
                     ClPtrT (*reallocHook)(ClPtrT ,ClUint32T),
                     ClPtrT (*callocHook)(ClUint32T,ClUint32T),
                     void (*freeHook)(ClPtrT)
                     )
{
    ClRcT rc;

    rc = CL_HEAP_RC(CL_ERR_INITIALIZED);

    if(gHeapList.initialized == CL_TRUE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_REGISTER,"Heap system is already running. "
                                      "Please try to shove the register "
                                      "before a heap initialize,preferably "
                                      "through the clHeapCustomInitialize hook"
                                      
                       );
        goto out;
    }

    if(gHeapHooksDefined == CL_TRUE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_REGISTER,"Hooks already defined\n");
        goto out;
    }

    NULL_CHECK(allocHook);
    NULL_CHECK(reallocHook);
    NULL_CHECK(callocHook);
    NULL_CHECK(freeHook);

    gClHeapAllocate = allocHook;
    gClHeapRealloc = reallocHook;
    gClHeapCalloc = callocHook;
    gClHeapFree = freeHook;
    gHeapHooksDefined = CL_TRUE;
    rc = CL_OK;

    out:
    return rc;
}

ClRcT clHeapHooksDeregister(void)
{
    ClRcT rc;
    rc = CL_HEAP_RC(CL_ERR_INITIALIZED);
    if(gHeapList.initialized == CL_TRUE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_DEREGISTER,"Heap system should be finalized "
                                      "before you shove this one through."
                                      "This should be done preferably through "
                                      "a clHeapLibFinalize before the "
                                      "clHeapCustomFinalize hook\n"
                       );
        goto out;
    }
    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);
    if(gHeapHooksDefined == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_DEREGISTER,"Hooks not present\n");
        goto out;
    }
    gHeapHooksDefined = CL_FALSE;
    rc = CL_OK;
    out:
    return rc;
}

/*  Called by EO to initialize the POOL */
ClRcT clHeapLibInitialize(const ClHeapConfigT *pHeapConfig)
{
    ClRcT rc = CL_OK;
    ClPoolFlagsT flags;
    ClHeapModeT mode;
    register ClUint32T i;

    if(gHeapList.initialized == CL_TRUE)
    {
        return CL_OK;
    }

    NULL_CHECK(pHeapConfig);

    rc = clOsalMutexInit(&gHeapList.mutex);

    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Error initialising heap stats mutex\n");
        goto out;
    }

    clRbTreeInit(&gHeapList.largeChunkTree, heapCmpLargeChunk);
    
    gClHeapCheckerRequired = clParseEnvBoolean("CL_HEAP_CHECKER");
    CL_HEAP_CHECKER_INITIALIZE;

    /*
     * Copy the config as we are going to modify certain fields
     * of the config. 
     */

    memcpy(&gHeapList.heapConfig,pHeapConfig,sizeof(gHeapList.heapConfig));

    gHeapList.heapConfig.pPoolConfig = NULL;

    gHeapList.pPoolHandles = NULL;

    /*Check if we have user defined hooks that override our implementation*/

    if(pHeapConfig->mode == CL_HEAP_CUSTOM_MODE)
    {
        rc = clHeapLibCustomInitialize(pHeapConfig);

        if(rc == CL_OK && gHeapHooksDefined == CL_TRUE)
        {
            /*  
             * If hooks are defined, then custom heap is on.
             * So please stay away:-)
             */
            clLogInfo(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Using heap in custom mode\n");
            gHeapList.heapConfig.numPools = 0;
            gHeapList.initialized = CL_TRUE;
            goto out;
        }
        else
        {
            clLogInfo(HEAP_LOG_AREA,HEAP_LOG_CTX_REGISTER,"Falling back to preallocated mode\n");
            gHeapHooksDefined = CL_FALSE;

            gHeapList.heapConfig.mode = CL_HEAP_PREALLOCATED_MODE;
            
        }
    }

    /*Set the debug level now*/
    clHeapDebugLevelSet();

    mode = pHeapConfig->mode;

    /*Purify tricks*/

    CL_HEAP_PURIFY_HOOK(mode);

    /*  Set the pool mode. Return if its a system mode set. */
    clHeapModeSet (mode);

    rc = CL_OK;
    if (mode == CL_HEAP_NATIVE_MODE)
    {
        clLogInfo(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Setting pool mode to native\n");
        /*resetting pool count if given by mistake*/
        gHeapList.heapConfig.numPools = 0;
        gHeapList.initialized = CL_TRUE;
        /*for purify, we would overwrite any other modes here*/
        gHeapList.heapConfig.mode = mode;
#ifdef VXWORKS_BUILD
        if(clParseEnvBoolean("CL_MEM_PARTITION"))
            rc = heapLibVxWorksInitialize();
#endif
        goto out;
    }

    NULL_CHECK(pHeapConfig->pPoolConfig);

    rc = CL_HEAP_RC(CL_ERR_INVALID_PARAMETER);
    if(pHeapConfig->numPools == 0) 
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Pool count 0\n");
        goto out_free;
    }

    rc = CL_HEAP_RC(CL_ERR_NO_MEMORY);
    if ((gHeapList.heapConfig.pPoolConfig = (ClPoolConfigT*)
         CL_HEAP_ALLOC_EXTERNAL (sizeof (ClPoolConfigT)
                                 * pHeapConfig->numPools) ) == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Error allocating memory\n");
        goto out_free;
    }

    if ((gHeapList.pPoolHandles = (void**)
         CL_HEAP_ALLOC_EXTERNAL (sizeof (ClPoolT) * pHeapConfig->numPools))
        == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,"Error allocating memory\n");
        goto out_free;
    }

    memset(gHeapList.pPoolHandles,0,sizeof(ClPoolT) * pHeapConfig->numPools);
    memcpy (gHeapList.heapConfig.pPoolConfig, pHeapConfig->pPoolConfig,
            sizeof (ClPoolConfigT) * pHeapConfig->numPools);

    /* Sort the chunk list on chunk size for faster search to get the pool */
    qsort (gHeapList.heapConfig.pPoolConfig, pHeapConfig->numPools,
           sizeof (*gHeapList.heapConfig.pPoolConfig), clPoolCmp);

    CL_HEAP_POOL_FLAGS_SET(gHeapList.heapConfig,flags);

    for(i = 0; i < pHeapConfig->numPools;++i) 
    {
        ClPoolConfigT *pPoolConfig = gHeapList.heapConfig.pPoolConfig + i;
        ClPoolT handle = 0;
        rc = clPoolCreate(&handle,flags,pPoolConfig);
        if(rc != CL_OK) 
        {
            clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_INI,
                       "Error creating pool of size:%d\n", pPoolConfig->chunkSize);
            goto out_destroy;
        }
        gHeapList.pPoolHandles[i] = handle;
    }

    gHeapList.initialized = CL_TRUE;

    rc = CL_OK;

    goto out;

    out_destroy:
    {
        register ClUint32T j;
        for(j = 0; j < i; ++j)
        {
            clPoolDestroyForce(gHeapList.pPoolHandles[j]);
        }
    }
    out_free:
    if (NULL != gHeapList.heapConfig.pPoolConfig)
    {
        CL_HEAP_FREE_EXTERNAL ((ClPtrT)gHeapList.heapConfig.pPoolConfig,
                               sizeof (ClPoolConfigT) * gHeapList.heapConfig.numPools);
    }
    if (NULL != gHeapList.pPoolHandles)
    {
        CL_HEAP_FREE_EXTERNAL ((ClPtrT)gHeapList.pPoolHandles,
                               sizeof (ClPoolT) * gHeapList.heapConfig.numPools);
    }

    clOsalMutexDestroy(&gHeapList.mutex);

    /*reset the global pool list*/
    memset(&gHeapList,0,sizeof(gHeapList));


    out:
    return rc;
}

/*Free up the all the pools*/
ClRcT
clHeapLibFinalize(void)
{
    ClRcT rc = CL_OK;
    register ClUint32T i;

    rc = CL_HEAP_RC(CL_ERR_NOT_INITIALIZED);

    if(gHeapList.initialized == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_FINALISE,"Pool system isnt initialised\n");
        goto out;
    }

    gHeapList.initialized = CL_FALSE;

    clOsalMutexDestroy(&gHeapList.mutex);

    if(gHeapHooksDefined == CL_TRUE)
    {
        rc = clHeapLibCustomFinalize();
        if(rc != CL_OK)
        {
            clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_FINALISE,"Error deregistering heap hooks\n");
            goto out;
        }
        rc = CL_HEAP_RC(CL_ERR_UNSPECIFIED);
        if(gHeapHooksDefined == CL_TRUE)
        {
            clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_FINALISE,"Hooks still defined after custom finalize\n");
            goto out;
        }
        goto out_reset;
    }

    if(gHeapList.heapConfig.mode == CL_HEAP_NATIVE_MODE)
    {
#ifdef VXWORKS_BUILD
        if(clParseEnvBoolean("CL_MEM_PARTITION"))
            heapLibVxWorksFinalize();
#endif
        goto out_init;
    }

    /*Destroy the pools*/
    for(i = 0 ; i < gHeapList.heapConfig.numPools; ++i)
    {
        if(gHeapList.pPoolHandles[i])
        {
            rc = clPoolDestroy(gHeapList.pPoolHandles[i]);
            if(rc != CL_OK)
            {
                /* lower level logs it.. CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Problem destroying pool handle [%d]\n", i)); */
            }
        }
    }
    CL_HEAP_FREE_EXTERNAL ((ClPtrT)gHeapList.pPoolHandles,
                           sizeof (ClPoolT) * gHeapList.heapConfig.numPools);
    /*Rip off the chunk config*/
    CL_HEAP_FREE_EXTERNAL ((ClPtrT)gHeapList.heapConfig.pPoolConfig,
                           sizeof (ClPoolConfigT) * gHeapList.heapConfig.numPools);

    out_init:
    gClHeapAllocate = NULL;
    gClHeapRealloc =  NULL;
    gClHeapCalloc  = NULL;
    gClHeapFree = NULL;

    out_reset:
    /* Zero off. the global pool list and reset pointers */
    memset(&gHeapList,0,sizeof(gHeapList));
    rc = CL_OK;

    out:
    return rc;
}

/* The heap allocation/free interfaces starts here */

static
__inline__
void
clHeapSizeGet(
        ClPtrT      pAddress,
        ClUint32T*  pSize)
{
    ClUint32T size = 0;

    CL_HEAP_SET_ADDRESS(pAddress,-CL_HEAP_OVERHEAD);
    if(gHeapList.heapConfig.mode == CL_HEAP_NATIVE_MODE)
    {
        __CL_HEAP_GET_NATIVE_COOKIE(pAddress,size);
    }
    else
    {
        ClPtrT pCookie;
        __CL_HEAP_GET_COOKIE(pAddress,pCookie);
        clPoolChunkSizeGet(pCookie,&size);
    }
    *pSize = size;
}

/*Heap allocation/deallocation routines*/
void
clHeapStatsUpdate(
        ClPtrT          pAddress,
        ClUint32T*      pSize,
        ClMemDirectionT memDir)
{
    ClUint32T size = 0;
    clHeapSizeGet(pAddress,&size);
    CL_HEAP_STATS_UPDATE(size,memDir);
    if ( NULL != pSize)
    {
        *pSize = size;
    }
}

#ifdef VXWORKS_BUILD

/*Faster here*/
ClPtrT
clHeapAllocate(
               ClUint32T size)
{
    ClPtrT pAddress = NULL;

    if(gHeapList.initialized == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap isnt initialized\n");
        goto out;
    }

    /*We are here when debugging hooks are not set*/

    pAddress = gClHeapAllocate(size);
    if(pAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,
                    "Error allocating memory for size:%d\n", size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }

    if(!clParseEnvBoolean("CL_MEM_PARTITION"))
    {
        clHeapStatsUpdate(pAddress,NULL,CL_MEM_ALLOC);
        CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);
    }

    out:
    return pAddress;
}

ClPtrT clHeapRealloc(ClPtrT pAddress,ClUint32T size)
{
    ClPtrT pRetAddress = NULL;
    ClUint32T osize = 0;
    ClUint32T nsize = 0;
    ClInt32T  incrementSize = 0;
    ClBoolT memPart = CL_FALSE;

    if(gHeapList.initialized == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap isnt initialized\n");
        goto out;
    }

    /*
     * Here in the normal case:
     */
    
    if(!(memPart = clParseEnvBoolean("CL_MEM_PARTITION"))
       && 
       pAddress != NULL)
    {
        /*fetch the old size of allocation*/
        clHeapSizeGet(pAddress,&osize);
        CL_HEAP_SET_ADDRESS(pAddress,-CL_HEAP_OVERHEAD);
    }

    pRetAddress = gClHeapRealloc(pAddress,size);

    if(pRetAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap realloc failure for size:%d,address:%p\n",size,pAddress);
        goto out;
    }

    if(!memPart)
    {
        /*Get the new size*/
        clHeapSizeGet(pRetAddress,&nsize);

        /*Update this size*/
        incrementSize = nsize - osize;

        if(incrementSize > 0 )
        {
            CL_HEAP_STATS_UPDATE(incrementSize,CL_MEM_ALLOC);

            CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);
        }
    }

    out:
    return pRetAddress;
}

ClPtrT clHeapCalloc(ClUint32T numChunks,ClUint32T chunkSize)
{
    ClPtrT pRetAddress = NULL;
    ClUint32T size ;

    if(gHeapList.initialized == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap isnt initialized\n");
        goto out;
    }

    size = numChunks * chunkSize;

    /*Here in the usual case*/

    pRetAddress = gClHeapCalloc(numChunks,chunkSize);

    if(pRetAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);

        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error in calloc of size:%d - %d chunks,chunksize:%d\n",size,numChunks,chunkSize);
        goto out;
    }

    if(!clParseEnvBoolean("CL_MEM_PARTITION"))
    {
        clHeapStatsUpdate(pRetAddress,NULL,CL_MEM_ALLOC);

        CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);
    }

    out:
    return pRetAddress;
}

void
clHeapFree(
           ClPtrT pAddress)
{
    ClBoolT memPart = CL_FALSE;

    if(gHeapList.initialized == CL_FALSE)
    {
        
		clLogError(HEAP_LOG_AREA,CL_LOG_CONTEXT_UNSPECIFIED,"Heap isnt initialized\n"));
        return ;
    }

    if (NULL == pAddress)
    {
        return ;
    }

    /*Here in the normal case*/
    if(!(memPart = clParseEnvBoolean("CL_MEM_PARTITION")))
    {
        clHeapStatsUpdate(pAddress,NULL,CL_MEM_FREE);

        CL_HEAP_SET_ADDRESS(pAddress,-CL_HEAP_OVERHEAD);
    }

    gClHeapFree(pAddress);

    if(!memPart)
    {
        CL_HEAP_WATERMARKS_UPDATE(CL_MEM_FREE);
    }
}

void *clHeapFenceAllocate(ClUint32T bytes)
{
    return NULL; /*not supported for vxworks */
}

void clHeapFenceFree(void *addr, ClUint32T bytes)
{
    return ;/*not supported for vxworks*/
}

ClRcT clHeapReferenceLargeChunk(ClPtrT addr, ClUint32T size)
{
    return CL_HEAP_RC(CL_ERR_NOT_SUPPORTED);
}
#else

/*Faster here*/
ClPtrT
clHeapAllocate(
        ClUint32T size)
{
    ClPtrT pAddress = NULL;

    CL_HEAP_CHECKER_ALLOC(size);

    if(gHeapList.initialized == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap isnt initialized\n");
        goto out;
    }

    /*Debug hooks*/

    CL_HEAP_DEBUG_HOOK_RET(gClHeapAllocate(size));

    /*We are here when debugging hooks are not set*/

    pAddress = gClHeapAllocate(size);
    if(pAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error allocating memory for size:%d", size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }

    clHeapStatsUpdate(pAddress,NULL,CL_MEM_ALLOC);
    CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);

    out:
    return pAddress;
}

ClPtrT clHeapRealloc(ClPtrT pAddress,ClUint32T size)
{
    ClPtrT pRetAddress = NULL;
    ClUint32T osize = 0;
    ClUint32T nsize = 0;
    ClInt32T  incrementSize = 0;

    CL_HEAP_CHECKER_REALLOC(pAddress, size);
    CL_ASSERT(gHeapList.initialized);


    /*Debug hooks*/

    CL_HEAP_DEBUG_HOOK_RET(gClHeapRealloc(pAddress,size));

    /*
     * Here in the normal case:
    */
    if(pAddress != NULL)
    {
        /*fetch the old size of allocation*/
        clHeapSizeGet(pAddress,&osize);
        CL_HEAP_SET_ADDRESS(pAddress,-CL_HEAP_OVERHEAD);
    }

    pRetAddress = gClHeapRealloc(pAddress,size);

    if(pRetAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Heap realloc failure for size:%d,address:%p\n",size,pAddress);
        goto out;
    }

    /*Get the new size*/
    clHeapSizeGet(pRetAddress,&nsize);

    /*Update this size*/
    incrementSize = nsize - osize;

    if(incrementSize > 0 )
    {
        CL_HEAP_STATS_UPDATE(incrementSize,CL_MEM_ALLOC);

        CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);
    }

    out:
    return pRetAddress;
}

ClPtrT clHeapCalloc(ClUint32T numChunks,ClUint32T chunkSize)
{
    ClPtrT pRetAddress = NULL;
    ClUint32T size ;

    CL_HEAP_CHECKER_CALLOC(numChunks, chunkSize);
    CL_ASSERT(gHeapList.initialized);

    size = numChunks * chunkSize;

    /*Debug hooks*/

    CL_HEAP_DEBUG_HOOK_RET(gClHeapCalloc(numChunks,chunkSize));

    /*Here in the usual case*/

    pRetAddress = gClHeapCalloc(numChunks,chunkSize);

    if(pRetAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);

        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error in calloc of size:%d - %d chunks,chunksize:%d\n",size,numChunks,chunkSize);
        goto out;
    }

    clHeapStatsUpdate(pRetAddress,NULL,CL_MEM_ALLOC);

    CL_HEAP_WATERMARKS_UPDATE(CL_MEM_ALLOC);

    out:
    return pRetAddress;
}

void
clHeapFree(
        ClPtrT pAddress)
{

    CL_HEAP_CHECKER_FREE(pAddress);
    CL_ASSERT(gHeapList.initialized);

    if (NULL == pAddress)
    {
        return ;
    }

    /*Debug hooks*/
    CL_HEAP_DEBUG_HOOK_NORET(gClHeapFree(pAddress));

    /*Here in the normal case*/
    clHeapStatsUpdate(pAddress,NULL,CL_MEM_FREE);

    CL_HEAP_SET_ADDRESS(pAddress,-CL_HEAP_OVERHEAD);

    gClHeapFree(pAddress);

    CL_HEAP_WATERMARKS_UPDATE(CL_MEM_FREE);

}

/*
 * Overrun fencing
 */
void *clHeapFenceAllocate(ClUint32T bytes)
{
    static int pagemask = 0;
    int abytes = bytes;
    char *map;
    if(!pagemask)
        pagemask = getpagesize()-1;
    abytes += sizeof(unsigned long)-1;
    abytes &= ~(sizeof(unsigned long)-1);
    bytes = abytes;
    bytes += pagemask;
    bytes &= ~pagemask;
    map = (char *) mmap(0, bytes + pagemask + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(map == MAP_FAILED)
        return NULL;
    /*
     * protect the adjacent page against overruns.
     */
    CL_ASSERT(mprotect(map+bytes, pagemask+1, PROT_NONE) == 0);
    return (void*)(map + bytes - abytes);
}

void clHeapFenceFree(void *addr, ClUint32T bytes)
{
    static int pagemask;
    int abytes = bytes;
    char *map = (char*) addr;
    if(!pagemask)
        pagemask = getpagesize()-1;
    abytes += sizeof(unsigned long)-1;
    abytes &= ~(sizeof(unsigned long)-1);
    bytes = abytes;
    bytes += pagemask;
    bytes &= ~pagemask;                                                 
    CL_ASSERT(mprotect(map+abytes, pagemask+1, PROT_READ|PROT_WRITE)==0);
    CL_ASSERT(munmap(map+abytes-bytes, bytes+pagemask+1)==0);
}

#endif

/* Allocate from pool: Faster here !  */
static
ClPtrT
heapAllocatePreAllocated(
        ClUint32T size)
{
    ClPtrT pAddress = NULL;
    ClPoolConfigT *pPoolConfig = NULL;
    ClPoolConfigT poolKey = { 0 };
    ClPoolT poolHandle;
    ClPtrT          pCookie     = NULL;
    ClRcT rc;

    /* Add the overhead over here for the cookie */
    size += CL_HEAP_OVERHEAD;

    poolKey.chunkSize = size;
    /*
     * Do a binary search to get an index into the chunk config and hence pool
     * handle to allocate from.
    */
    rc = clBinarySearch ((ClPtrT)&poolKey, gHeapList.heapConfig.pPoolConfig,
            gHeapList.heapConfig.numPools, sizeof (ClPoolConfigT),
            clPoolCmp, (ClPtrT*)&pPoolConfig);
    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error getting pool handle for size:%d", size);
        goto out;
    }

    poolHandle = gHeapList.pPoolHandles [
        pPoolConfig - gHeapList.heapConfig.pPoolConfig ];

    rc = clPoolAllocate(poolHandle,(ClUint8T **) &pAddress,&pCookie);
    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error allocating memory from pool size:%d for input size:%d", pPoolConfig->chunkSize, size);
        goto out;
    }
   
    CL_HEAP_SET_COOKIE(poolHandle,pAddress,pCookie);

    /*
     * Zero of the allocation request as of now till ASP is cured
     * of its disease of expecting malloc to zero off the contents.
     */
    memset(pAddress,0,size-CL_HEAP_OVERHEAD);

    out:
    return (ClPtrT )pAddress;
}

static ClPtrT heapReallocPreAllocated(ClPtrT pAddress,ClUint32T size)
{
    ClPtrT pCookie        = NULL;
    ClPtrT tempAddress    = pAddress;
    ClPtrT pRetAddress = NULL;
    ClUint32T chunkSize   = 0;
    ClUint32T capacity    = 0;
    ClRcT rc;

    if(pAddress == NULL)
    {
        /*call allocate without wasting time*/
        goto allocate;
    }

    __CL_HEAP_GET_COOKIE(tempAddress,pCookie);

    CL_HEAP_SET_ADDRESS(pAddress,CL_HEAP_OVERHEAD);

    clPoolChunkSizeGet(pCookie,&chunkSize);

    capacity = chunkSize - CL_HEAP_OVERHEAD;

    if(0 < size && capacity >= size)
    {
        pRetAddress = pAddress;
        goto out;
    }

    /*
     * Allocate a new chunk. Copy the contents of the old chunk to this guy
     * and release the old chunk
    */

    allocate:
    pRetAddress = heapAllocatePreAllocated(size);

    if(pRetAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"realloc:Error allocating memory for size:%d\n",size);
        goto out;
    }
    /*  
     * Copy the contents of the old chunk to the new chunk and release
     * the old chunk
    */
    if(pAddress != NULL)
    {
        if(0 < size )
        {
            memcpy(pRetAddress,pAddress,capacity);
        }
        /*
         * stats update would be performed implicitly 
         * by clHeapRealloc by adding the difference between
         * the old and the new size.Just doing the numFree update here.
        */
        CL_HEAP_STATS_UPDATE_INCR(CL_MEM_FREE);

        rc = clPoolFree((ClUint8T*)tempAddress,pCookie);

        if(rc != CL_OK)
        {
            clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error freeing the old chunk:%p\n",pAddress);
            /*Never mind this to be slow as its a rare case*/
            clHeapFree(pRetAddress);

            pRetAddress = NULL;

            goto out;
        }
    }
    out:
    return (ClPtrT)pRetAddress;
}

/*Just call pool allocate without wasting time*/
static ClPtrT heapCallocPreAllocated(ClUint32T numChunks,ClUint32T chunkSize)
{
    ClUint32T size;
    ClPtrT pAddress ;
    size = numChunks * chunkSize;
    pAddress = heapAllocatePreAllocated(size);
    if(pAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error in pool calloc for size:%d\n",size);
        goto out;
    }
    memset(pAddress,0,size);
    out:
    return pAddress;
}

/*Free to our pool*/
static void heapFreePreAllocated(ClPtrT pAddress)
{
    ClPtrT pCookie = NULL;
    ClRcT rc;

    /*First get the cookie or extended pool handle*/
    __CL_HEAP_GET_COOKIE(pAddress,pCookie);
    rc = clPoolFree((ClUint8T*)pAddress,pCookie);
    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error freeing memory:%p\n",pAddress);
    }
}

static ClInt32T heapCmpLargeChunk(ClRbTreeT *chunk, ClRbTreeT *entry)
{
    ClHeapLargeChunkT *addedChunk = CL_RBTREE_ENTRY(chunk, ClHeapLargeChunkT, tree);
    ClHeapLargeChunkT *presentChunk = CL_RBTREE_ENTRY(entry, ClHeapLargeChunkT, tree);
    if(addedChunk->chunk > presentChunk->chunk )
        return 1;
    else if(addedChunk->chunk < presentChunk->chunk )
        return -1;
    return 0;
}

static ClPtrT heapAddLargeChunk(ClPtrT pAddress)
{
    ClHeapLargeChunkT *largeChunk = (ClHeapLargeChunkT*)pAddress;
    memset(largeChunk, 0, sizeof(*largeChunk));
    largeChunk->magic = CL_HEAP_LARGE_CHUNK_MAGIC;
    largeChunk->refCnt = 1;
    largeChunk->chunk = pAddress;
    clOsalMutexLock(&gHeapList.mutex);
    clRbTreeInsert(&gHeapList.largeChunkTree, &largeChunk->tree);
    ++gHeapList.numLargeChunks;
    clOsalMutexUnlock(&gHeapList.mutex);
    return (ClPtrT)((ClUint8T*)pAddress + CL_HEAP_LARGE_CHUNK_OVERHEAD);
}

static ClRcT heapDeleteLargeChunk(ClPtrT pAddress)
{
    ClRcT rc = CL_OK;
    ClHeapLargeChunkT *largeChunk = (ClHeapLargeChunkT*)pAddress;
    clOsalMutexLock(&gHeapList.mutex);
    if(largeChunk->magic == CL_HEAP_LARGE_CHUNK_MAGIC)
    {
        /*
         * If the chunk has references, don't delete it yet.
         */
        if(!--largeChunk->refCnt)
        {
            clRbTreeDelete(&gHeapList.largeChunkTree, &largeChunk->tree);
            memset(largeChunk, 0, sizeof(*largeChunk));
            --gHeapList.numLargeChunks;
        }
        else rc = CL_ERR_INUSE;
    }
    else rc = CL_ERR_INVALID_STATE;
    clOsalMutexUnlock(&gHeapList.mutex);
    return rc;
}

ClRcT clHeapReferenceLargeChunk(ClPtrT pAddress, ClUint32T size)
{
    ClRcT rc = CL_OK;
    ClHeapLargeChunkT chunkEntry = {{0}};
    ClHeapLargeChunkT *largeChunk = NULL;
    ClRbTreeT *pTreeChunk;
    /*
     * Only supported for native mode.
     */
    if(gClHeapAllocate != heapAllocateNative)
        return CL_HEAP_RC(CL_ERR_NOT_SUPPORTED);

    if(size < CL_HEAP_LARGE_CHUNK_SIZE)
        return CL_HEAP_RC(CL_ERR_NOT_EXIST);
    /*
     * We trace the large chunk in the tree since we cannot assume that the chunk was allocated from heap.
     */
    chunkEntry.chunk =  ( (ClUint8T*)pAddress - CL_HEAP_OVERHEAD - CL_HEAP_LARGE_CHUNK_OVERHEAD );
    clOsalMutexLock(&gHeapList.mutex);
    pTreeChunk = clRbTreeFind(&gHeapList.largeChunkTree, &chunkEntry.tree);
    if(!pTreeChunk)
    {
        rc = CL_HEAP_RC(CL_ERR_NOT_EXIST);
        goto out_unlock;
    }
    largeChunk = CL_RBTREE_ENTRY(pTreeChunk, ClHeapLargeChunkT, tree);
    if(largeChunk->magic != CL_HEAP_LARGE_CHUNK_MAGIC 
       ||
       !largeChunk->refCnt)
    {
        clLogCritical(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Large chunk at [%p] of size [%d] already freed\n", 
                                           chunkEntry.chunk, size);
        rc = CL_HEAP_RC(CL_ERR_INVALID_STATE);
        goto out_unlock;
    }
    ++largeChunk->refCnt;
    out_unlock:
    clOsalMutexUnlock(&gHeapList.mutex);
    return rc;
}

/*Include the hooks for system mode*/
static
ClPtrT
heapAllocateNative(
        ClUint32T size)
{
    ClPtrT pAddress = NULL;
    ClBoolT largeChunk = CL_FALSE;
    ClUint32T overhead = CL_HEAP_OVERHEAD;

    /* Patch the size for system mode and check if we can allocate.  */
    if(size >= CL_HEAP_LARGE_CHUNK_SIZE)
    {
        largeChunk = CL_TRUE;
        overhead += CL_HEAP_LARGE_CHUNK_OVERHEAD;
    }

    size += overhead;

    if(CL_HEAP_GRANT_SIZE(size) == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,
                   "%d bytes would exceed process upper limit", size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,
                    "Allocation of %d bytes would exceed process upper limit",
                    size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }
    
    pAddress = malloc(size);
    if(pAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error allocating %d bytes",size);
        CL_HEAP_REVOKE_SIZE(size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,"Malloc of %d bytes failed",size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }

    /*
     * Enable tracking/reference the chunk since we could stich this to a buffer
     */
    overhead = 0;
    if(largeChunk)
    {
        pAddress = heapAddLargeChunk(pAddress);
        overhead = CL_HEAP_LARGE_CHUNK_OVERHEAD;
    }

    /*Zero off the contents till ASP is cured of its diseases*/
    memset(pAddress,0,size - overhead);

    /*Set the cookie for system mode here*/
    CL_HEAP_SET_NATIVE_COOKIE(pAddress,size);

    out:
    return pAddress;
}

static ClPtrT heapReallocNative(ClPtrT pAddress,ClUint32T size)
{
    ClPtrT pRetAddress = NULL;
    ClUint32T overhead = CL_HEAP_OVERHEAD;
    ClBoolT largeChunk = CL_FALSE;
    ClHeapLargeChunkT *largeChunkEntry = NULL;
    ClUint8T reallocChunkBackup[CL_HEAP_LARGE_CHUNK_OVERHEAD] = {0};
    ClUint32T osize = 0;

    if(pAddress)
    {
        clHeapSizeGet((ClUint8T*)pAddress + CL_HEAP_OVERHEAD, &osize);
    }
    /*
     * Check if largechunk already exists. and delete it from the tree since if realloc reallocates the chunk,
     * we are fucked !!
     */
    if(osize >= CL_HEAP_LARGE_CHUNK_SIZE + CL_HEAP_LARGE_CHUNK_OVERHEAD + CL_HEAP_OVERHEAD)
    {
        /*
         * Take a backup just incase we are getting truncated and the large chunk to sudden small chunk
         * move has to be adjusted for the mirror since the old chunk could be freed on a migration to small chunk
         */
        memcpy(reallocChunkBackup, pAddress, sizeof(reallocChunkBackup));
        pAddress = (ClPtrT)((ClUint8T*)pAddress - CL_HEAP_LARGE_CHUNK_OVERHEAD);
        largeChunkEntry = (ClHeapLargeChunkT*)pAddress;
        if(largeChunkEntry->magic != CL_HEAP_LARGE_CHUNK_MAGIC)
        {
            clLogCritical(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Trying to realloc an invalid large chunk at [%p] of size [%d]\n",
                                               pAddress, osize);
            goto out;
        }
        if(largeChunkEntry->refCnt > 1)
        {
            clLogCritical(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Trying to realloc a large chunk at [%p] of size [%d] "
                                               "with [%d] references\n", 
                                               pAddress, osize, largeChunkEntry->refCnt);
            goto out;
        }
        clOsalMutexLock(&gHeapList.mutex);
        clRbTreeDelete(&gHeapList.largeChunkTree, &largeChunkEntry->tree);
        memset(largeChunkEntry, 0, sizeof(*largeChunkEntry));
        --gHeapList.numLargeChunks;
        clOsalMutexUnlock(&gHeapList.mutex);
    }

    if(size >= CL_HEAP_LARGE_CHUNK_SIZE)
    {
        overhead += CL_HEAP_LARGE_CHUNK_OVERHEAD;
        largeChunk = CL_TRUE;
    }
    size += overhead;
    if(CL_HEAP_GRANT_SIZE(size) == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"%d bytes realloc would exceed process wide limit\n",size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,
                    "Realloc %d bytes would exceed process upper limit",
                    size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }

    pRetAddress = realloc(pAddress,size);
    if(pRetAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error in realloc of %d bytes\n",size);
        CL_HEAP_REVOKE_SIZE(size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,
                    "Realloc %d bytes failed",
                    size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }
    if(largeChunk)
    {
        /*
         * See if this is a new large chunk addition in which case we have to move existing chunk
         * to accomodate for the large chunk.
         */
        if(pAddress && !largeChunkEntry && osize)
        {
            memmove((ClCharT*)pRetAddress + CL_HEAP_LARGE_CHUNK_OVERHEAD, 
                    pRetAddress, 
                    CL_MIN(osize, size - CL_HEAP_LARGE_CHUNK_OVERHEAD));
        }
        pRetAddress = heapAddLargeChunk(pRetAddress);
    }
    else if(pAddress && largeChunkEntry)
    {
        /*
         * If we are moving from large chunk to small one, then truncate the large chunk overhead
         * in the mirrored chunk
         */
        if(size >= CL_HEAP_LARGE_CHUNK_OVERHEAD)
            memmove(pRetAddress, (ClCharT*)pRetAddress + CL_HEAP_LARGE_CHUNK_OVERHEAD,
                    size - CL_HEAP_LARGE_CHUNK_OVERHEAD);
        else
            memcpy(pRetAddress, reallocChunkBackup, size);
    }

    CL_HEAP_SET_NATIVE_COOKIE(pRetAddress,size);
    out:
    return pRetAddress;
}

static ClPtrT heapCallocNative(ClUint32T chunk,ClUint32T chunkSize)
{
    ClPtrT pAddress = NULL;
    ClUint32T size = chunk*chunkSize;
    ClBoolT largeChunk = CL_FALSE;
    ClUint32T overhead = CL_HEAP_OVERHEAD;
    if(size >= CL_HEAP_LARGE_CHUNK_SIZE)
    {
        overhead += CL_HEAP_LARGE_CHUNK_OVERHEAD;
        largeChunk = CL_TRUE;
    }
    size += overhead;
    if(CL_HEAP_GRANT_SIZE(size) == CL_FALSE)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"%d bytes would exceed process upper limit\n",size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,
                    "calloc %d bytes would exceed process upper limit",
                    size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }

    pAddress = calloc(1,size);
    if(pAddress == NULL)
    {
        clLogError(HEAP_LOG_AREA,HEAP_LOG_CTX_ALLOCATION,"Error in calloc of %d chunks of %d size\n",chunk,chunkSize);
        CL_HEAP_REVOKE_SIZE(size);
        CL_HEAP_LOG(CL_LOG_SEV_ERROR,"calloc %d bytes failed",size);
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        goto out;
    }
    if(largeChunk)
    {
        pAddress = heapAddLargeChunk(pAddress);
    }
    CL_HEAP_SET_NATIVE_COOKIE(pAddress,size);
    out:
    return pAddress;
}

static void heapFreeNative(ClPtrT pAddress)
{
    ClUint32T size = 0;
    if(pAddress != NULL)
    {
        __CL_HEAP_GET_NATIVE_COOKIE(pAddress,size);
        /*
         * Check if its a large chunk.
         */
        if(size >= CL_HEAP_LARGE_CHUNK_SIZE + CL_HEAP_OVERHEAD + CL_HEAP_LARGE_CHUNK_OVERHEAD)
        {
            pAddress = (ClPtrT)((ClUint8T*)pAddress - CL_HEAP_LARGE_CHUNK_OVERHEAD);
            /*
             * If the large chunk has a reference, skip the free.
             */
            if(heapDeleteLargeChunk(pAddress) == CL_ERR_INUSE)
            {
                return;
            }
        }
        CL_HEAP_REVOKE_SIZE(size);
    }
    free(pAddress);
}

/*Now include the default debugging hooks defined for heap*/

#include "clHeapDebug.c"

