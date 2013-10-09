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
/*****************************************************************************
 * ModuleName  : utils
 * File        : clPool.c
 ****************************************************************************/

/*****************************************************************************
 * Description :
 * This file contains Clovis Chunk Pool Management implementation
 ****************************************************************************/

/*uncomment to get debug messages even in non-debug mode*/
/*#define CL_DEBUG*/

#include <string.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clPoolIpi.h>
#include <clEoIpi.h>
#include <clMemStats.h>

/*Macro definitions*/

#define CL_POOL_RC(rc)   (CL_RC(CL_CID_POOL,(rc)))

#define CL_POOL_ALIGNMENT  (0x8)

#define CL_POOL_MIN_CHUNK_SIZE ((ClUint32T)sizeof(ClFreeListHeaderT))

#define CL_POOL_BOUNDS_CHECK(cond,rc,...) do {              \
        if( !(cond) )                                       \
        {                                                   \
            CL_POOL_LOG(CL_LOG_SEV_ERROR,__VA_ARGS__);   \
            return (rc);                                    \
        }                                                   \
}while(0)

/*Pool lock and unlock*/
#define POOL_LOCK_INIT(mutex) (clOsalMutexInit(mutex))
#define POOL_LOCK_DESTROY(mutex) (clOsalMutexDestroy(mutex))
#define POOL_LOCK(mutex) (clOsalMutexLock(mutex))
#define POOL_UNLOCK(mutex) (clOsalMutexUnlock(mutex))

/* Note DO NOT USE normal logging in this module because it is used BY logging, resulting in a deadlock */
#undef clLog
#define clLog(severity, area, context, ...)  ***ERROR Do not use logging in this module.  Use CL_POOL_LOG***
#define CL_POOL_LOG(sev,...) clEoLibLog(CL_CID_POOL,sev,__VA_ARGS__)

#define CL_POOL_ALLOC_EXT(size) malloc(size)

#define CL_POOL_FREE_EXT(address,size) free(address)

#define CL_POOL_ALLOC_EXTERNAL(flags,size) clPoolAllocExternal(flags,size)

#define CL_POOL_FREE_EXTERNAL(flags,pChunk,size)                    \
        clPoolFreeExternal(flags,pChunk,size)

/*we get into lazy for both debug or lazy mode*/
#define CL_POOL_LAZY_MODE(pool)                                     \
    ( (pool)->flags & ( CL_POOL_LAZY_FLAG | CL_POOL_DEBUG_FLAG) )

#define CL_POOL_FREE_LIST_GET_LAZY(chunk,extendedPool,freeList) do {    \
    freeList =                                                          \
    (extendedPool)->pFreeListStart +                                    \
    ( ( (chunk) - (extendedPool)->pExtendedPoolStart ) /                \
    (extendedPool)->pPoolHeader->poolConfig.chunkSize );                \
}while(0)

/* Get the free list from the chunk */

#define CL_POOL_FREE_LIST_GET(chunk,extendedPool,freeList) do {     \
    freeList = (ClFreeListHeaderT*)(chunk);                         \
    if(CL_POOL_LAZY_MODE((extendedPool)->pPoolHeader))              \
    {                                                               \
        CL_POOL_FREE_LIST_GET_LAZY(chunk,extendedPool,freeList);    \
    }                                                               \
}while(0)

#define CL_POOL_MAX_EXTENDED_POOLS(pool) (                                  \
    ((pool)->poolConfig.maxPoolSize != CL_POOL_MAX_SIZE)                    \
     && (((pool)->stats.numExtendedPools *                                  \
        (pool)->poolConfig.incrementPoolSize)                               \
                 >= (pool)->poolConfig.maxPoolSize))

#define CL_POOL_STATS_UPDATE(pool,field,v) do { \
    (pool)->stats.field += (v);                 \
}while(0)

#define CL_POOL_STATS_UPDATE_MAX(pool,field,v) do { \
    if( ( (unsigned int) (v) > (pool)->stats.field ) ) {          \
        (pool)->stats.field = (v);                  \
    }                                               \
}while(0)

#define CL_POOL_CHECK_WRAP_INCR(pool,field) do {            \
    if( ((pool)->stats.field+1) < (pool)->stats.field ) {   \
        CL_POOL_LOG(CL_LOG_TRACE,                           \
                    "%s stats wrapped around",              \
                    #field);                                \
        (pool)->stats.field = 0;                            \
    }                                                       \
    CL_POOL_STATS_UPDATE(pool,field,1);                     \
}while(0)

#define CL_POOL_STATS_UPDATE_EXTENDED_POOLS_INCR(pool) do {     \
    CL_POOL_CHECK_WRAP_INCR(pool,numExtendedPools);             \
    CL_POOL_STATS_UPDATE_MAX(pool,maxNumExtendedPools,          \
                             (pool)->stats.numExtendedPools);   \
}while(0)

#define CL_POOL_STATS_UPDATE_EXTENDED_POOLS_DECR(pool) do { \
    CL_POOL_STATS_UPDATE(pool,numExtendedPools,-1);         \
}while(0)

#define CL_POOL_STATS_UPDATE_ALLOCS(pool) do {  \
    CL_POOL_CHECK_WRAP_INCR(pool,numAllocs);    \
}while(0)

#define CL_POOL_STATS_UPDATE_FREES(pool) do {   \
    CL_POOL_STATS_UPDATE_MAX_NUMALLOCS(pool);   \
    CL_POOL_CHECK_WRAP_INCR(pool,numFrees);     \
}while(0)

#define CL_POOL_STATS_UPDATE_MAX_NUMALLOCS(pool) do {           \
    ClInt32T maxAllocs;                                         \
    maxAllocs =     (pool)->stats.numAllocs -                   \
                    (pool)->stats.numFrees;                     \
    if(maxAllocs > 0 )                                          \
    {                                                           \
        CL_POOL_STATS_UPDATE_MAX(pool,maxNumAllocs,maxAllocs);  \
    }                                                           \
}while(0)

#define CL_POOL_LOCK_INIT(pool) POOL_LOCK_INIT(&(pool)->mutex)

#define CL_POOL_LOCK_DESTROY(pool) POOL_LOCK_DESTROY(&(pool)->mutex)

#define CL_POOL_LOCK(pool) POOL_LOCK(&(pool)->mutex)

#define CL_POOL_UNLOCK(pool) POOL_UNLOCK(&(pool)->mutex)

#define CL_POOL_GRANT_SIZE(flags,size)                                  \
    (((flags) & CL_POOL_DEBUG_FLAG) ? CL_TRUE : clEoMemAdmitAllocate(size))

#define CL_POOL_REVOKE_SIZE(flags,size) do {    \
    if ( ! ( (flags) & CL_POOL_DEBUG_FLAG ) )   \
    {                                           \
        clEoMemNotifyFree(size);                \
    }                                           \
}while(0)

#define CL_POOL_WATERMARKS_UPDATE(flags,dir) do {       \
    if(!( (flags) & CL_POOL_DEBUG_FLAG ) )             \
    {                                                   \
        clEoMemWaterMarksUpdate(dir);                   \
    }                                                   \
}while(0)

/*
 * Extended list manipulation macros.
 * Add in a lifo way to the queue.
 */

#ifndef POOL_QUEUE

#define CL_POOL_EXTENDED_LIST_QUEUE(list,extendedPool) do {                    \
    if(( (extendedPool)->pNext = (list)->pHeadExtendedList) )                  \
    {                                                                          \
        (extendedPool)->pNext->ppPrev = &((extendedPool)->pNext);              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        (list)->ppTailExtendedList = &(extendedPool)->pNext;                   \
    }                                                                          \
    *((extendedPool)->ppPrev = &((list)->pHeadExtendedList)) = (extendedPool); \
    ++(list)->numEntries;                                                      \
} while(0)

#else

#define CL_POOL_EXTENDED_LIST_QUEUE(list,extendedPool) do { \
    *( (extendedPool)->ppPrev =                             \
       (list)->ppTailExtendedList                           \
     ) = (extendedPool);                                    \
    (list)->ppTailExtendedList = &((extendedPool)->pNext);  \
    ++(list)->numEntries;                                   \
}while(0)

#endif

#define CL_POOL_EXTENDED_LIST_DEQUEUE(list,extendedPool) do {       \
    if( (extendedPool)->ppPrev )                                    \
    {                                                               \
        if((extendedPool)->pNext)                                   \
        {                                                           \
            (extendedPool)->pNext->ppPrev = (extendedPool)->ppPrev; \
        }                                                           \
        else                                                        \
        {                                                           \
            (list)->ppTailExtendedList = (extendedPool)->ppPrev;    \
        }                                                           \
        *((extendedPool)->ppPrev) = (extendedPool)->pNext;          \
        (extendedPool)->pNext = NULL;                               \
        (extendedPool)->ppPrev = NULL;                              \
        --(list)->numEntries;                                       \
    }                                                               \
}while(0)

#define CL_POOL_EXTENDED_LIST_POP(list,extendedPool) do {   \
    extendedPool = (list)->pHeadExtendedList;               \
}while(0)

#define CL_POOL_EXTENDED_LIST_POP_DEQUEUE(list,extendedPool) do {   \
    CL_POOL_EXTENDED_LIST_POP(list,extendedPool);                   \
    CL_POOL_EXTENDED_LIST_DEQUEUE(list,extendedPool);               \
}while(0)

#define CL_POOL_EXTENDED_LIST_INIT(list) do {   \
    (list)->numEntries = 0;                     \
    (list)->pHeadExtendedList = NULL;           \
    (list)->ppTailExtendedList =                \
    &(list)->pHeadExtendedList;                 \
}while(0)

#define CL_POOL_EXTENDED_LIST_COUNT(list) ( (list)->numEntries)

#define CL_POOL_EXTENDED_LIST_EMPTY(list)                                   \
        (CL_POOL_EXTENDED_LIST_COUNT(list) == 0)

#define CL_POOL_EXTENDED_FREELIST_INIT(pool)                \
    CL_POOL_EXTENDED_LIST_INIT(&((pool)->freeExtendedList))

#define CL_POOL_EXTENDED_PARTIALLIST_INIT(pool)                 \
    CL_POOL_EXTENDED_LIST_INIT(&(pool)->partialExtendedList)

#define CL_POOL_EXTENDED_FULLLIST_INIT(pool)                \
    CL_POOL_EXTENDED_LIST_INIT(&(pool)->fullExtendedList)

#define CL_POOL_EXTENDED_FREELIST_QUEUE(pool,extendedPool) do {             \
    CL_POOL_EXTENDED_LIST_QUEUE(&(pool)->freeExtendedList,extendedPool);    \
}while(0)

#define CL_POOL_EXTENDED_PARTIALLIST_QUEUE(pool,extendedPool) do {          \
    CL_POOL_EXTENDED_LIST_QUEUE(&(pool)->partialExtendedList,extendedPool); \
}while(0)

#define CL_POOL_EXTENDED_FULLLIST_QUEUE(pool,extendedPool) do {             \
    CL_POOL_EXTENDED_LIST_QUEUE(&(pool)->fullExtendedList,extendedPool);    \
}while(0)

#define CL_POOL_EXTENDED_FREELIST_DEQUEUE(pool,extendedPool) do {           \
    CL_POOL_EXTENDED_LIST_DEQUEUE(&(pool)->freeExtendedList,extendedPool);  \
}while(0)

#define CL_POOL_EXTENDED_PARTIALLIST_DEQUEUE(pool,extendedPool) do {          \
    CL_POOL_EXTENDED_LIST_DEQUEUE(&(pool)->partialExtendedList,extendedPool); \
}while(0)

#define CL_POOL_EXTENDED_FULLLIST_DEQUEUE(pool,extendedPool) do {           \
    CL_POOL_EXTENDED_LIST_DEQUEUE(&(pool)->fullExtendedList,extendedPool);  \
}while(0)

#define CL_POOL_EXTENDED_FREELIST_POP(pool,extendedPool) do {           \
    CL_POOL_EXTENDED_LIST_POP(&(pool)->freeExtendedList,extendedPool);  \
}while(0)

#define CL_POOL_EXTENDED_PARTIALLIST_POP(pool,extendedPool) do {            \
    CL_POOL_EXTENDED_LIST_POP(&(pool)->partialExtendedList,extendedPool);   \
}while(0)

#define CL_POOL_EXTENDED_FULLLIST_POP(pool,extendedPool) do {           \
    CL_POOL_EXTENDED_LIST_POP(&(pop)->fullExtendedList,extendedPool);   \
}while(0)

#define CL_POOL_EXTENDED_FREELIST_POP_DEQUEUE(pool,extendedPool) do {          \
    CL_POOL_EXTENDED_LIST_POP_DEQUEUE(&(pool)->freeExtendedList,extendedPool); \
}while(0)

#define CL_POOL_EXTENDED_PARTIALLIST_POP_DEQUEUE(pool,extendedPool) do {    \
    CL_POOL_EXTENDED_LIST_POP_DEQUEUE(&(pool)->partialExtendedList,         \
        extendedPool);                                                      \
}while(0)

#define CL_POOL_EXTENDED_FULLLIST_POP_DEQUEUE(pool,extendedPool) do {         \
    CL_POOL_EXTENDED_LIST_POP_DEQUEUE(&(pop)->fullExtendedList,extendedPool); \
}while(0)

#define CL_POOL_EXTENDED_FREELIST_COUNT(pool)               \
    CL_POOL_EXTENDED_LIST_COUNT(&(pool)->freeExtendedList)

#define CL_POOL_EXTENDED_PARTIALLIST_COUNT(pool)                \
    CL_POOL_EXTENDED_LIST_COUNT(&(pool)->partialExtendedList)

#define CL_POOL_EXTENDED_FULLLIST_COUNT(pool)               \
    CL_POOL_EXTENDED_LIST_COUNT(&(pool)->fullExtendedList)

#define CL_POOL_EXTENDED_FREELIST_EMPTY(pool)               \
    CL_POOL_EXTENDED_LIST_EMPTY(&(pool)->freeExtendedList)

#define CL_POOL_EXTENDED_PARTIALLIST_EMPTY(pool)                \
    CL_POOL_EXTENDED_LIST_EMPTY(&(pool)->partialExtendedList)

#define CL_POOL_EXTENDED_FULLLIST_EMPTY(pool)               \
    CL_POOL_EXTENDED_LIST_EMPTY(&(pool)->fullExtendedList)

#define CL_POOL_SHRINK_FREELIST(pool,options)                   \
    clPoolShrinkList(pool,&(pool)->freeExtendedList,options)

#define CL_POOL_SHRINK_PARTIALLIST(pool,options)                \
    clPoolShrinkList(pool,&(pool)->partialExtendedList,options)

#define CL_POOL_SHRINK_FULLLIST(pool,options)                   \
    clPoolShrinkList(pool,&(pool)->fullExtendedList,options)

#undef NULL_CHECK

#define NULL_CHECK(X) do {                      \
    if(NULL == (X))                             \
    {                                           \
        return CL_POOL_RC(CL_ERR_NULL_POINTER); \
    }                                           \
}while(0)

/*Type/variable definition follows*/

struct ClExtendedPoolHeader;

typedef struct ClFreeListHeader
{
    ClUint8T *pChunk; /*chunk itself*/
    struct ClFreeListHeader* pNextFreeChunk; /*next list*/
}ClFreeListHeaderT;
/*****************************************************************************/

typedef struct ClExtendedList
{
    struct ClExtendedPoolHeader *pHeadExtendedList;
    struct ClExtendedPoolHeader **ppTailExtendedList;
    ClUint32T numEntries;
} ClExtendedListT;

typedef struct ClPoolHeader
{
    ClPoolConfigT poolConfig;/*pool config is kept here*/
    ClOsalMutexT mutex;  /*Lock for this pool*/
    ClPoolFlagsT flags; /*pool flags*/
    ClPoolStatsT stats; /*pool stats*/
    /*Extended free/full/partial list linkage*/
    ClExtendedListT partialExtendedList;
    ClExtendedListT freeExtendedList;
    ClExtendedListT fullExtendedList;
}ClPoolHeaderT;

/*****************************************************************************/
typedef struct ClExtendedPoolHeader
{
    ClPoolHeaderT *pPoolHeader; /*the pool to which this belongs*/
    ClUint32T numChunks;
    ClUint32T numFreeChunks; /*How many free chunks are there in extended pool*/
    /* pointer to the beginning of the extended pool */
    ClUint8T* pExtendedPoolStart;
    ClFreeListHeaderT *pFirstFreeChunk;
    ClFreeListHeaderT *pFreeListStart;
    struct ClExtendedPoolHeader *pNext;
    struct ClExtendedPoolHeader **ppPrev;
}ClExtendedPoolHeaderT;

static
ClRcT
clExtendedPoolDestroy( ClPoolHeaderT* pPoolHeader,
    ClExtendedPoolHeaderT* pExtendedPoolHeader);

static
ClRcT
clPoolShrinkList( ClPoolHeaderT *pPoolHeader, ClExtendedListT *pList,
    const ClPoolShrinkOptionsT *pShrinkOptions);

/*Call malloc/free as of now. Later can be changed into a buddy*/

#define UTIL_LOG_AREA		"UTL"
#define UTIL_LOG_CTX_POOL	"POOL"

static
__inline__
void*
clPoolAllocExternal(
    ClPoolFlagsT    flags,
    ClUint32T       size)
{
    void    *pAddress = NULL;

    /*Check if this can be granted*/
    if (CL_POOL_GRANT_SIZE(flags,size) == CL_TRUE)
    {
        pAddress = CL_POOL_ALLOC_EXT(size);
        if(!pAddress)
        {
            CL_POOL_REVOKE_SIZE(flags,size);
        }
        else
        {
            CL_POOL_WATERMARKS_UPDATE(flags,CL_MEM_ALLOC);
        }
    }
    return pAddress;
}

static
__inline__
void
clPoolFreeExternal(
    ClPoolFlagsT    flags,
    void            *pAddress,
    ClUint32T       size)
{
    CL_POOL_REVOKE_SIZE (flags, size);
    CL_POOL_WATERMARKS_UPDATE (flags, CL_MEM_FREE);
    CL_POOL_FREE_EXT (pAddress, size);
}

/*Validate the pool config as we dont trust anyone besides us here*/
static
ClRcT
clPoolValidateConfig(const ClPoolConfigT *pPoolConfig)
{
    ClRcT rc = CL_POOL_RC(CL_ERR_INVALID_PARAMETER);

    CL_POOL_BOUNDS_CHECK(pPoolConfig->chunkSize >= CL_POOL_MIN_CHUNK_SIZE, 
                         rc,
                         "Chunk Size too small, Got %d, need at least %d.\n",
                         pPoolConfig->chunkSize,
                         CL_POOL_MIN_CHUNK_SIZE);

    CL_POOL_BOUNDS_CHECK((pPoolConfig->chunkSize  % CL_POOL_ALIGNMENT)==0, 
                         rc,
                         "Chunk Size must be a multiple of %d bytes\n", 
                         CL_POOL_ALIGNMENT);

    CL_POOL_BOUNDS_CHECK(pPoolConfig->incrementPoolSize, 
                         rc, 
                         "Pool increment must not be 0\n");

    CL_POOL_BOUNDS_CHECK((pPoolConfig->incrementPoolSize %
                          pPoolConfig->chunkSize)==0, 
                         rc,
                         "Pool increment %d must be a multiple of the "\
                         "chunk size %d\n", 
                         pPoolConfig->incrementPoolSize, 
                         pPoolConfig->chunkSize);

    CL_POOL_BOUNDS_CHECK((pPoolConfig->initialPoolSize %
                          pPoolConfig->incrementPoolSize)==0, 
                         rc, 
                         "Initial pool size %d must be a multiple "\
                         "of the pool increment %d\n", 
                         pPoolConfig->initialPoolSize,
                         pPoolConfig->incrementPoolSize);

    CL_POOL_BOUNDS_CHECK(
                         !((pPoolConfig->maxPoolSize != CL_POOL_MAX_SIZE)
                           && ((pPoolConfig->maxPoolSize < 
                                pPoolConfig->initialPoolSize)
                               || 
                               (pPoolConfig->maxPoolSize % 
                                pPoolConfig->incrementPoolSize)
                               )
                           ),
                         rc, 
                         "The max pool size %d has to be either the system "\
                         "maximum %d (CL_POOL_MAX_SIZE), or more than the "\
                         "initialPoolSize %d, and also a multiple of "\
                         "the pool increment %d\n", 
                         pPoolConfig->maxPoolSize, 
                         CL_POOL_MAX_SIZE,
                         pPoolConfig->initialPoolSize, 
                         pPoolConfig->incrementPoolSize);
    
    return CL_OK;
}

/*
 * Global free list is kept outside the extended pool in debug mode.
 */

static
ClRcT
clPoolPartitionExtendedPoolDebug(
    ClExtendedPoolHeaderT *pExtendedPoolHeader)
{
    register ClUint32T  i;
    ClFreeListHeaderT   *pFreeList;
    ClFreeListHeaderT   *pCurrentFreeList;
    ClUint8T            *pCurrentChunkStart;

    ClUint32T chunkSize = pExtendedPoolHeader->pPoolHeader->poolConfig.chunkSize;
    ClUint32T numChunks = pExtendedPoolHeader->numChunks;
    ClRcT rc = CL_POOL_RC(CL_ERR_NO_MEMORY);

    pFreeList = (ClFreeListHeaderT*)CL_POOL_ALLOC_EXTERNAL
                    (pExtendedPoolHeader->pPoolHeader->flags,
                    sizeof (*pFreeList) * numChunks);
    if (pFreeList == NULL)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Error allocating memory for freelist for chunk Size:%d",chunkSize);
        goto out;
    }
    pCurrentChunkStart = pExtendedPoolHeader->pExtendedPoolStart;
    pCurrentFreeList = pFreeList;
    pExtendedPoolHeader->pFreeListStart = pFreeList;
    pExtendedPoolHeader->pFirstFreeChunk = pCurrentFreeList;
    pCurrentFreeList->pChunk = pCurrentChunkStart;
    for(i = 0; i < numChunks - 1; ++i)
    {
        pCurrentChunkStart += chunkSize;
        pCurrentFreeList->pNextFreeChunk = pCurrentFreeList + 1;
        pCurrentFreeList->pNextFreeChunk->pChunk = pCurrentChunkStart;
        ++pCurrentFreeList;
    }
    pCurrentFreeList->pNextFreeChunk = NULL;
    rc = CL_OK;
    out:
    return rc;
}

static
ClRcT
clPoolPartitionExtendedPool(
    ClExtendedPoolHeaderT *pExtendedPoolHeader)
{
    ClFreeListHeaderT   *pCurrentFreeList;
    ClUint8T            *pCurrentChunkStart;
    ClUint32T           chunkSize;
    ClRcT               rc = CL_OK;
    register ClUint32T  i;

    if (CL_POOL_LAZY_MODE (pExtendedPoolHeader->pPoolHeader))
    {
        return clPoolPartitionExtendedPoolDebug (pExtendedPoolHeader);
    }

    chunkSize = pExtendedPoolHeader->pPoolHeader->poolConfig.chunkSize;
    pCurrentChunkStart = (ClUint8T*)pExtendedPoolHeader->pExtendedPoolStart;
    pCurrentFreeList = (ClFreeListHeaderT*) pCurrentChunkStart;
    pExtendedPoolHeader->pFirstFreeChunk = pCurrentFreeList;
    pCurrentFreeList->pChunk = pCurrentChunkStart;

    for (i=0; i < pExtendedPoolHeader->numFreeChunks - 1;  i++)
    {
        pCurrentChunkStart += chunkSize;
        pCurrentFreeList->pNextFreeChunk =
            (ClFreeListHeaderT*)pCurrentChunkStart;
        pCurrentFreeList->pNextFreeChunk->pChunk = pCurrentChunkStart;
        pCurrentFreeList = (ClFreeListHeaderT*)pCurrentChunkStart;
    }
    /* Link the last free chunk */
    pCurrentFreeList->pNextFreeChunk = NULL;
    return rc;
}

/*****************************************************************************/
static
ClRcT
clPoolAllocateExtendedPool(
    ClPoolHeaderT*          pPoolHeader,
    ClExtendedPoolHeaderT*  pExtendedPoolHeader)
{
    ClRcT rc = CL_OK;
    ClUint32T chunkSize = pPoolHeader->poolConfig.chunkSize;
    ClUint32T incrementPoolSize = pPoolHeader->poolConfig.incrementPoolSize;

    rc = CL_POOL_RC (CL_ERR_NO_MEMORY);

    if ((pExtendedPoolHeader->pExtendedPoolStart =
        (ClUint8T*)CL_POOL_ALLOC_EXT (incrementPoolSize)) == NULL)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Error allocating memory of size:%d\n", incrementPoolSize);
        goto out;
    }

    pExtendedPoolHeader->pPoolHeader = pPoolHeader;
    pExtendedPoolHeader->numChunks = incrementPoolSize/chunkSize;
    pExtendedPoolHeader->numFreeChunks = pExtendedPoolHeader->numChunks;

    rc = clPoolPartitionExtendedPool (pExtendedPoolHeader);
    if (rc != CL_OK)
    {
        goto out_free;
    }

    /*Add this extended pool to the free list*/
    CL_POOL_LOG(CL_LOG_TRACE,"Adding extended pool %p to free list",(void*)pExtendedPoolHeader);

    CL_POOL_EXTENDED_FREELIST_QUEUE(pPoolHeader,pExtendedPoolHeader);

    goto out;

    out_free:
        CL_POOL_FREE_EXTERNAL (pPoolHeader->flags,
                pExtendedPoolHeader->pExtendedPoolStart, incrementPoolSize);

    out:
    return rc;
}

/****************************************************************************/
ClRcT
clPoolCreate(
    ClPoolT             *pHandle,
    ClPoolFlagsT        flags,
    const ClPoolConfigT *pPoolConfig)
{
    ClRcT rc = CL_OK;
    ClPoolHeaderT* pPoolHeader = NULL;
    ClUint32T chunkSize = 0;
    ClUint32T incrementPoolSize = 0;
    ClUint32T extendedPools = 0;
    ClUint32T i = 0;

    NULL_CHECK (pPoolConfig);
    NULL_CHECK (pHandle);

    *pHandle = 0;

    /* Validate the chunk config*/
    rc = clPoolValidateConfig (pPoolConfig);
    if(rc != CL_OK)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Invalid param");
        goto out;
    }

    chunkSize = pPoolConfig->chunkSize;
    incrementPoolSize = pPoolConfig->incrementPoolSize;
    extendedPools = pPoolConfig->initialPoolSize/incrementPoolSize;

    rc = CL_POOL_RC (CL_ERR_NO_MEMORY);
    if ((pPoolHeader = (ClPoolHeaderT*)CL_POOL_ALLOC_EXTERNAL(flags, sizeof (*pPoolHeader))) == NULL)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Error in allocating memory for pool header, chunk size:%d\n",chunkSize);
        goto out;
    }
    memset (pPoolHeader, 0, sizeof (*pPoolHeader));
    memcpy (&(pPoolHeader->poolConfig), pPoolConfig,
            sizeof (pPoolHeader->poolConfig));
    pPoolHeader->flags = (ClPoolFlagsT) (pPoolHeader->flags | flags);

    rc = CL_POOL_LOCK_INIT (pPoolHeader);
    if(CL_OK != rc)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"CL_POOL_LOCK_INIT failed, rc=[%#X]\n", rc);
        goto out;
    }

    /*Initialise the extended pool free,partial and full list*/
    CL_POOL_EXTENDED_FREELIST_INIT (pPoolHeader);
    CL_POOL_EXTENDED_PARTIALLIST_INIT (pPoolHeader);
    CL_POOL_EXTENDED_FULLLIST_INIT (pPoolHeader);

    if (extendedPools == 0)
    {
        rc = CL_OK;
        goto out;
    }

    /*Create the initial extended pools*/
    for (i = 0; i < extendedPools; ++i)
    {
        ClExtendedPoolHeaderT *pExtendedPoolHeader = NULL;
        if ((pExtendedPoolHeader =
                    (ClExtendedPoolHeaderT*)CL_POOL_ALLOC_EXTERNAL (flags,
                        sizeof (*pExtendedPoolHeader))) == NULL)
        {
            CL_POOL_LOG(CL_LOG_ERROR,"Error allocating memory for extended pool header, chunk size:%d extended pool number:%d\n", chunkSize, i);
            goto out_free;
        }
        memset (pExtendedPoolHeader, 0, sizeof (*pExtendedPoolHeader));
        /*Initialize the extended pool*/
        rc = clPoolAllocateExtendedPool (pPoolHeader, pExtendedPoolHeader);
        if(rc != CL_OK)
        {
            CL_POOL_FREE_EXTERNAL (flags, pExtendedPoolHeader, sizeof (*pExtendedPoolHeader));
            CL_POOL_LOG(CL_LOG_ERROR,"Error allocating extended pool, chunk size:%d extended pool number:%d\n", chunkSize, i);
            goto out_free;
        }
        CL_POOL_STATS_UPDATE_EXTENDED_POOLS_INCR(pPoolHeader);
    }
    *pHandle = (ClPoolT)pPoolHeader;
    CL_POOL_LOG(CL_LOG_TRACE, "%d extended pools created for %d byte pool with chunksize:%d",
                extendedPools,
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize);
    rc = CL_OK;
    goto out;

    out_free:
    if(pPoolHeader)
    {
        clPoolDestroy((ClPoolT)pPoolHeader);
    }
    out:
    return rc;
}

/*****************************************************************************/
ClRcT
clPoolExtend(
    ClPoolT poolHandle)
{
    ClRcT                   rc                  = CL_OK;
    ClPoolHeaderT*          pPoolHeader         = NULL;
    ClExtendedPoolHeaderT*  pExtendedPoolHeader = NULL;

    pPoolHeader = (ClPoolHeaderT*)poolHandle;

    NULL_CHECK (pPoolHeader);

    CL_POOL_LOG(CL_LOG_INFO,"clPoolExtend() :: extendedPoolSize = %d, chunkSize = %d \n",
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize);

    rc = CL_POOL_RC (CL_ERR_NO_MEMORY);
    if (CL_POOL_MAX_EXTENDED_POOLS (pPoolHeader))
    {
        CL_POOL_LOG(CL_LOG_ERROR,"POOL extend of %d byte pool failed.Max pool size :%d, extended pools:%d\n",
                   pPoolHeader->poolConfig.chunkSize,
                   pPoolHeader->poolConfig.maxPoolSize,
                   pPoolHeader->stats.numExtendedPools);
        goto out;
    }

    if((pExtendedPoolHeader =
        (ClExtendedPoolHeaderT*)CL_POOL_ALLOC_EXTERNAL (pPoolHeader->flags,
            sizeof (ClExtendedPoolHeaderT))) == NULL)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Error allocating memory for extended pool header, chunk size:%d extended pool number:%d\n",
                   pPoolHeader->poolConfig.chunkSize,
                   pPoolHeader->stats.numExtendedPools);
        goto out;
    }

    memset (pExtendedPoolHeader, 0, sizeof (*pExtendedPoolHeader));

    CL_POOL_LOG(CL_LOG_TRACE,"Extending %d byte pool of %d chunkSize",
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize);

    /*Initialise the extended pool*/

    rc = clPoolAllocateExtendedPool (pPoolHeader, pExtendedPoolHeader);
    if (rc != CL_OK)
    {
        CL_POOL_LOG(CL_LOG_ERROR,
                    "Extended pool allocation of %d byte pool of %d chunksize failed. pool number:%d",
                    pPoolHeader->poolConfig.incrementPoolSize,
                    pPoolHeader->poolConfig.chunkSize, pPoolHeader->stats.numExtendedPools
                    );
        goto out_free;
    }

    CL_POOL_STATS_UPDATE_EXTENDED_POOLS_INCR(pPoolHeader);
    rc = CL_OK;
    goto out;

    out_free:
    CL_POOL_FREE_EXTERNAL (pPoolHeader->flags, pExtendedPoolHeader,
        sizeof (*pExtendedPoolHeader));

    out:
    return rc;
}

/*****************************************************************************/

static
ClRcT
clPoolDestroyWithForce(
    ClPoolHeaderT   *pPoolHeader,
    ClBoolT         forceFlag)
{
    ClPoolShrinkOptionsT shrinkOptions;
    ClRcT rc = CL_POOL_RC(CL_ERR_OP_NOT_PERMITTED);
    shrinkOptions.shrinkFlags = CL_POOL_SHRINK_ALL;

    if (forceFlag == CL_FALSE
            && (!CL_POOL_EXTENDED_PARTIALLIST_EMPTY (pPoolHeader)
                || !CL_POOL_EXTENDED_FULLLIST_EMPTY (pPoolHeader)))
    {
        CL_POOL_LOG(CL_LOG_WARNING, "Warning!!Destroy called when pool is being used. Chunk size:%d", pPoolHeader->poolConfig.chunkSize);
        goto out;
    }
    rc = CL_POOL_SHRINK_FREELIST (pPoolHeader, &shrinkOptions);
    if (rc != CL_OK)
    {
        CL_POOL_LOG(CL_LOG_ERROR, "Error shrinking pools free list for chunk size:%d", pPoolHeader->poolConfig.chunkSize);
        goto out;
    }

    if (!CL_POOL_EXTENDED_PARTIALLIST_EMPTY(pPoolHeader))
    {
        CL_POOL_LOG(CL_LOG_WARNING,
                     "Warning!!Partial list isnt empty for chunk size:%d Count :%d\n",
                     pPoolHeader->poolConfig.chunkSize,
                     CL_POOL_EXTENDED_PARTIALLIST_COUNT (pPoolHeader));
        rc = CL_POOL_SHRINK_PARTIALLIST (pPoolHeader, &shrinkOptions);
    }

    if (!CL_POOL_EXTENDED_FULLLIST_EMPTY (pPoolHeader))
    {
        CL_POOL_LOG(CL_LOG_WARNING,
                     "Warning !!Full list isnt empty for chunk size:%d Count :%d\n",
                     pPoolHeader->poolConfig.chunkSize,
                     CL_POOL_EXTENDED_FULLLIST_COUNT (pPoolHeader));
        rc = CL_POOL_SHRINK_FULLLIST (pPoolHeader, &shrinkOptions);
    }
    else
    {
        rc = CL_OK;
    }
    out:
    return rc;
}

ClRcT
clPoolDestroyForce(
    ClPoolT poolHandle)
{
    ClRcT           rc              = CL_OK;
    ClPoolHeaderT   *pPoolHeader    = (ClPoolHeaderT*)poolHandle;
    ClPoolFlagsT    flags;

    NULL_CHECK (pPoolHeader);
    flags = pPoolHeader->flags;
    CL_POOL_LOG(CL_LOG_TRACE,
                "Force destroying %d byte pool of %d chunksize",
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize
                );
    CL_POOL_LOCK (pPoolHeader);
    rc = clPoolDestroyWithForce (pPoolHeader, CL_TRUE);
    CL_POOL_UNLOCK (pPoolHeader);
    CL_POOL_LOCK_DESTROY (pPoolHeader);
    CL_POOL_FREE_EXTERNAL (flags, (void*)pPoolHeader, sizeof (*pPoolHeader));
    return rc;
}

ClRcT
clPoolDestroy(
    ClPoolT poolHandle)
{
    ClRcT rc = CL_OK;
    ClPoolHeaderT *pPoolHeader = (ClPoolHeaderT*)poolHandle;
    ClPoolFlagsT flags;

    NULL_CHECK (pPoolHeader);
    flags = pPoolHeader->flags;
    CL_POOL_LOG(CL_LOG_NOTICE,
                "Destroying %d byte pool of %d chunksize",
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize
                );
    CL_POOL_LOCK (pPoolHeader);
    rc = clPoolDestroyWithForce (pPoolHeader, CL_FALSE);
    CL_POOL_UNLOCK (pPoolHeader);

    if (rc != CL_OK)
    {
        CL_POOL_LOG(CL_LOG_ERROR,
                    "Error destroying %d byte pool of %d chunksize",
                    pPoolHeader->poolConfig.incrementPoolSize,
                    pPoolHeader->poolConfig.chunkSize);
        goto out;
    }
    /*
     * This is pretty dangerous and hence any users of this pool
     * go for a toss.
     * But destroy shouldnt be used on pre-defined pools
     * with pools in use anyway.
     */
    CL_POOL_LOCK_DESTROY (pPoolHeader);
    CL_POOL_FREE_EXTERNAL (flags, (void*)pPoolHeader, sizeof (*pPoolHeader));
    out:
    return rc;
}

/*****************************************************************************/

ClRcT
clPoolAllocate(
    ClPoolT     poolHandle,
    ClUint8T**  ppChunk,
    void**      ppCookie)
{
    ClPoolHeaderT*          pPoolHeader         = NULL;
    ClRcT                   rc                  = CL_OK;
    ClFreeListHeaderT*      pFreeChunk          = NULL;
    ClExtendedPoolHeaderT*  pExtendedPoolHeader = NULL;

    pPoolHeader = (ClPoolHeaderT*)poolHandle;
    NULL_CHECK (pPoolHeader);
    NULL_CHECK (ppChunk);
    NULL_CHECK (ppCookie);

    CL_POOL_LOCK (pPoolHeader);

    if(CL_POOL_GRANT_SIZE(pPoolHeader->flags,
                          pPoolHeader->poolConfig.chunkSize) == CL_FALSE)
    {
        rc = CL_POOL_RC(CL_ERR_NO_MEMORY);
        CL_POOL_LOG(CL_LOG_ERROR,"Request of %d bytes would exceed process upper limit\n",pPoolHeader->poolConfig.chunkSize);
        goto out_unlock;
    }

    /*Check for a partially full queue*/
    if (CL_POOL_EXTENDED_PARTIALLIST_EMPTY (pPoolHeader))
    {
        /*Look for an extended pool in the free list*/
        if (CL_POOL_EXTENDED_FREELIST_EMPTY (pPoolHeader))
        {
            /*Grow the extended pool*/
            rc = clPoolExtend(poolHandle);
            if(rc != CL_OK)
            {
                CL_POOL_LOG(CL_LOG_ERROR,"Pool allocate failed to extend %d byte pool of %d chunksize",
                            pPoolHeader->poolConfig.incrementPoolSize,
                            pPoolHeader->poolConfig.chunkSize);
                goto out_unlock;
            }
        }
        CL_POOL_EXTENDED_FREELIST_POP_DEQUEUE (pPoolHeader, pExtendedPoolHeader);
        CL_POOL_EXTENDED_PARTIALLIST_QUEUE (pPoolHeader, pExtendedPoolHeader);
    }
    /*
     * We are here when we have a free chunk:
     * Point of no-return
     */
    CL_POOL_EXTENDED_PARTIALLIST_POP (pPoolHeader, pExtendedPoolHeader);

    pFreeChunk = pExtendedPoolHeader->pFirstFreeChunk;
    pExtendedPoolHeader->pFirstFreeChunk = pFreeChunk->pNextFreeChunk;
    *ppChunk = (ClUint8T*)pFreeChunk->pChunk;
    *ppCookie = (void *)pExtendedPoolHeader;

    CL_POOL_STATS_UPDATE_ALLOCS (pPoolHeader);

    --pExtendedPoolHeader->numFreeChunks;

    CL_POOL_LOG(CL_LOG_TRACE,"pExtendedPoolHeader = %p pExtendedPoolHeader->numFreeChunks = %d pPoolHeader->poolConfig.chunkSize = %d from address = %p\n",
               (void*)pExtendedPoolHeader, pExtendedPoolHeader->numFreeChunks, pPoolHeader->poolConfig.chunkSize, (void*)pFreeChunk->pChunk);

    /*
     * Was free partially but now full.
     * move it to full list.
     */
    if (pExtendedPoolHeader->numFreeChunks == 0)
    {
        CL_POOL_LOG(CL_LOG_TRACE,"Dequeuing extended pool %p from partial list and queueing it to full list\n", (void*)pExtendedPoolHeader);
        CL_POOL_EXTENDED_PARTIALLIST_DEQUEUE (pPoolHeader, pExtendedPoolHeader);
        CL_POOL_EXTENDED_FULLLIST_QUEUE (pPoolHeader, pExtendedPoolHeader);
    }
    out_unlock:
    CL_POOL_UNLOCK (pPoolHeader);
    return rc;
}

/*****************************************************************************/
static
ClRcT
clExtendedPoolDestroy(
    ClPoolHeaderT*          pPoolHeader,
    ClExtendedPoolHeaderT*  pExtendedPoolHeader)
{
    ClRcT rc = CL_OK;
    NULL_CHECK (pPoolHeader);
    NULL_CHECK (pExtendedPoolHeader);

    CL_POOL_FREE_EXT (pExtendedPoolHeader->pExtendedPoolStart,
            pPoolHeader->poolConfig.incrementPoolSize);
    /*
     * For debug mode this would be set to the free list.
     * Rip this off if it exists.
     */
    if (pExtendedPoolHeader->pFreeListStart)
    {
        CL_POOL_FREE_EXTERNAL (pPoolHeader->flags,
                pExtendedPoolHeader->pFreeListStart,
                sizeof (ClFreeListHeaderT) * pExtendedPoolHeader->numChunks);
    }
    CL_POOL_FREE_EXTERNAL (pPoolHeader->flags, (void*)pExtendedPoolHeader,
            sizeof (*pExtendedPoolHeader));
    CL_POOL_STATS_UPDATE_EXTENDED_POOLS_DECR(pPoolHeader);
    return rc;
}

static
ClRcT
clPoolShrinkList(
    ClPoolHeaderT               *pPoolHeader,
    ClExtendedListT             *pList,
    const ClPoolShrinkOptionsT  *pShrinkOptions)
{
    ClExtendedPoolHeaderT *pExtendedPoolHeader = NULL;
    ClExtendedPoolHeaderT *pNext = NULL;
    ClUint32T numExtendedFreePools = 0;
    ClUint32T i;
    ClRcT rc = CL_OK;

    numExtendedFreePools = CL_POOL_EXTENDED_LIST_COUNT (pList);

    switch(pShrinkOptions->shrinkFlags)
    {
        case CL_POOL_SHRINK_DEFAULT:
        /*50 % of the free list*/
            numExtendedFreePools /= 2;
            break;
        case CL_POOL_SHRINK_ONE:
            numExtendedFreePools = CL_MIN (1, numExtendedFreePools);
            break;
        default:
        case CL_POOL_SHRINK_ALL:
            ;
        break;
    }

    if (numExtendedFreePools == 0)
    {
        CL_POOL_LOG(CL_LOG_WARNING,"No free list to shrink");
        goto out;
    }

    for(i = 0, pExtendedPoolHeader = pList->pHeadExtendedList; 
        i < numExtendedFreePools && pExtendedPoolHeader; 
        ++i)
    {
        pNext = pExtendedPoolHeader->pNext;
        CL_POOL_EXTENDED_LIST_DEQUEUE(pList, pExtendedPoolHeader);
        rc = clExtendedPoolDestroy (pPoolHeader, pExtendedPoolHeader);
        if(rc != CL_OK)
        {
            CL_POOL_LOG(CL_LOG_ERROR,"Error in destroying extended pool");
            goto out;
        }
        pExtendedPoolHeader = pNext;
    }

    out:
    return rc;
}

ClRcT
clPoolShrink(
    ClPoolT                     poolHandle,
    const ClPoolShrinkOptionsT* pShrinkOptions)
{
    ClRcT          rc          = CL_OK;
    ClPoolHeaderT* pPoolHeader = (ClPoolHeaderT*)poolHandle;
    ClPoolShrinkOptionsT defaultShrinkOptions;
    defaultShrinkOptions.shrinkFlags = CL_POOL_SHRINK_DEFAULT;

    NULL_CHECK (pPoolHeader);
    if (pShrinkOptions == NULL)
    {
        pShrinkOptions = &defaultShrinkOptions;
    }
    CL_POOL_LOG(CL_LOG_TRACE,
                "Shrinking %d byte pool of %d chunksize",
                pPoolHeader->poolConfig.incrementPoolSize,
                pPoolHeader->poolConfig.chunkSize);

    CL_POOL_LOCK (pPoolHeader);
    rc = CL_POOL_SHRINK_FREELIST (pPoolHeader, pShrinkOptions);
    CL_POOL_UNLOCK(pPoolHeader);

    if(rc != CL_OK)
    {
        CL_POOL_LOG(CL_LOG_ERROR,"Error shrinking %d byte pool of %d chunksize",pPoolHeader->poolConfig.incrementPoolSize,pPoolHeader->poolConfig.chunkSize);
    }
    return rc;
}

/*****************************************************************************/
ClRcT
clPoolFree(
    ClUint8T*   pChunk,
    void*       pCookie)
{
    ClRcT                   rc                  = CL_OK;
    ClPoolHeaderT*          pPoolHeader         = NULL;
    ClExtendedPoolHeaderT*  pExtendedPoolHeader = NULL;
    ClUint32T               freeChunk           = 0;
    ClUint32T               chunkSize           = 0;
    ClFreeListHeaderT*      pCurrentFreeList    = NULL;

    NULL_CHECK (pChunk);
    NULL_CHECK (pCookie);

    pExtendedPoolHeader = (ClExtendedPoolHeaderT*)pCookie;
    pPoolHeader = pExtendedPoolHeader->pPoolHeader;

    CL_POOL_LOG(CL_LOG_TRACE,"freeing chunk  pChunk = %p,extended pool = %p\n", pChunk,(void*)pExtendedPoolHeader);

    CL_POOL_LOCK (pPoolHeader);

    chunkSize = pPoolHeader->poolConfig.chunkSize;

    CL_POOL_REVOKE_SIZE(pPoolHeader->flags,chunkSize);

    CL_POOL_FREE_LIST_GET (pChunk, pExtendedPoolHeader, pCurrentFreeList);

    pCurrentFreeList->pChunk = (ClUint8T*)pChunk;
    pCurrentFreeList->pNextFreeChunk = pExtendedPoolHeader->pFirstFreeChunk;
    pExtendedPoolHeader->pFirstFreeChunk = pCurrentFreeList;

    CL_POOL_STATS_UPDATE_FREES (pPoolHeader);
    freeChunk = ++pExtendedPoolHeader->numFreeChunks;
    if (freeChunk == 1)
    {
        /*Was full before. Move it to partial*/
        CL_POOL_LOG(CL_LOG_TRACE,"Dequeuing extended pool %p from full list and moving the extended pool to partial list\n", (void*)pExtendedPoolHeader);
        CL_POOL_EXTENDED_FULLLIST_DEQUEUE (pPoolHeader, pExtendedPoolHeader);
        CL_POOL_EXTENDED_PARTIALLIST_QUEUE (pPoolHeader, pExtendedPoolHeader);
    }
    if (freeChunk == pExtendedPoolHeader->numChunks)
    {
        /*
         *  Add to the free extended pool list after deleting from the partial
         *  list
         */
        CL_POOL_LOG(CL_LOG_TRACE, "Dequeuing extended pool %p from partial list and moving the extended pool to free list\n", (void*)pExtendedPoolHeader);
        CL_POOL_EXTENDED_PARTIALLIST_DEQUEUE(pPoolHeader,pExtendedPoolHeader);
        CL_POOL_EXTENDED_FREELIST_QUEUE(pPoolHeader,pExtendedPoolHeader);
    }
    CL_POOL_UNLOCK(pPoolHeader);
    rc = CL_OK;
    return rc;
}

/*****************************************************************************/
ClRcT
clPoolChunkSizeGet(
    void*       pCookie,
    ClUint32T*  pSize)
{
    ClPoolHeaderT* pPoolHeader = NULL;
    NULL_CHECK (pCookie);
    NULL_CHECK (pSize);
    pPoolHeader = ((ClExtendedPoolHeaderT*)pCookie)->pPoolHeader;
    *pSize = pPoolHeader->poolConfig.chunkSize;
    return CL_OK;
}

ClRcT
clPoolStatsGet(
    ClPoolT         pool,
    ClPoolStatsT*   pPoolStats)
{
    ClRcT           rc          = CL_OK;
    ClPoolHeaderT*  pPoolHeader = (ClPoolHeaderT*)pool;

    NULL_CHECK (pPoolStats);
    NULL_CHECK (pPoolHeader);

    CL_POOL_LOCK (pPoolHeader);
    memcpy (pPoolStats, &pPoolHeader->stats, sizeof (*pPoolStats));
    /*store the pool config*/
    memcpy(&(pPoolStats->poolConfig), &(pPoolHeader->poolConfig),
        sizeof (pPoolStats->poolConfig));
    CL_POOL_UNLOCK(pPoolHeader);

    return rc;
}

/*Currently used by the heap debug layer*/
ClRcT
clPoolStartGet(
    void*   pCookie,
    void**  ppAddress)
{
    ClRcT rc = CL_OK;
    NULL_CHECK (pCookie);
    NULL_CHECK (ppAddress);
    *ppAddress = ((ClExtendedPoolHeaderT*)pCookie)->pExtendedPoolStart;
    return rc;
}
