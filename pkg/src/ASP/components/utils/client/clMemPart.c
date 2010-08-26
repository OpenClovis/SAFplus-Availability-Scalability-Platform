#ifndef VXWORKS_BUILD
#error "This file is specific to VxWorks build. and shouldn't be included for anything else."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clMemPart.h>
#include <memPartLib.h>

#define CL_MEM_PART_SIZE  (4<<20U)
#define CL_MEM_PART_EXPANSION_SLOTS (0x7)
#define CL_MEM_PART_EXPANSION_SIZE  (1 << 20U)

typedef struct ClMemPart
{
    ClOsalMutexT mutex;
    PART_ID partId;
    ClCharT *partitions[CL_MEM_PART_EXPANSION_SLOTS];
    int index;
}ClMemPartT;

ClRcT clMemPartInitialize(ClMemPartHandleT *pMemPartHandle, ClUint32T memPartSize)
{
    ClCharT *pool = NULL;
    ClInt32T i;
    ClRcT rc = CL_OK;
    ClMemPartT *pMemPart = NULL;

    if(!pMemPartHandle)
        return CL_ERR_INVALID_PARAMETER;

    if(!memPartSize)
        memPartSize = CL_MEM_PART_SIZE;

    pMemPart = calloc(1, sizeof(*pMemPart));
    CL_ASSERT(pMemPart !=  NULL);

    rc= clOsalMutexInit(&pMemPart->mutex);
    CL_ASSERT(rc == CL_OK);

    for(i = 0; i < CL_MEM_PART_EXPANSION_SLOTS; ++i)
        pMemPart->partitions[i] = NULL;

    pMemPart->index = 0;
    pool = malloc(memPartSize);
    if(!pool)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CALLOC failed for size [%d] while trying to create MEM partition\n", 
                                        memPartSize));
        return CL_ERR_NO_MEMORY;
    }
    pMemPart->partId = memPartCreate(pool, memPartSize);
    if(!pMemPart->partId)
    {
        free(pool);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("memPartCreate for size [%d] failed\n", memPartSize));
        return CL_ERR_NO_MEMORY;
    }
    pMemPart->partitions[pMemPart->index++] = pool;
    *pMemPartHandle = (ClMemPartHandleT)pMemPart;
    return CL_OK;
}

ClRcT clMemPartFinalize(ClMemPartHandleT *pHandle)
{
    STATUS rc = 0;
    ClMemPartT *pMemPart = NULL;
    if(!pHandle || !(pMemPart = (ClMemPartT*)*pHandle) )
        return CL_ERR_INVALID_PARAMETER;
    clOsalMutexLock(&pMemPart->mutex);
    rc = memPartDelete(pMemPart->partId);
    clOsalMutexUnlock(&pMemPart->mutex);
    clOsalMutexDestroy(&pMemPart->mutex);
    free(pMemPart);
    *pHandle = 0;
    if(rc == ERROR)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("memPartDelete returned [%s]\n", strerror(errno)));
        return CL_ERR_UNSPECIFIED;
    }
    return CL_OK;
}


static ClRcT clMemPartExpand(ClMemPartT *pMemPart, ClUint32T size)
{
    STATUS rc = 0;
    if(pMemPart->index >= CL_MEM_PART_EXPANSION_SLOTS)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mem part out of expansion slots trying to expand for size [%d]\n", size));
        return CL_ERR_NO_SPACE;
    }
    if(pMemPart->partitions[pMemPart->index])
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mem part already expanded\n"));
        return CL_ERR_UNSPECIFIED;
    }
    pMemPart->partitions[pMemPart->index] = malloc(size);
    if(!pMemPart->partitions[pMemPart->index])
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mem part expand calloc failure for size [%d]\n", size));
        return CL_ERR_NO_MEMORY;
    }
    if( (rc = memPartAddToPool(pMemPart->partId, pMemPart->partitions[pMemPart->index], size) ) == ERROR )
    {
        free(pMemPart->partitions[pMemPart->index]);
        pMemPart->partitions[pMemPart->index] = NULL;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mem part add to pool for size [%d] failed with [%s]\n",
                                        size, strerror(errno)));
        return CL_ERR_NO_MEMORY;
    }
    ++pMemPart->index;
    return CL_OK;
}

ClPtrT clMemPartAlloc(ClMemPartHandleT handle, ClUint32T size)
{
    ClPtrT mem = NULL;
    ClMemPartT *pMemPart = NULL;
    MEM_PART_STATS memStats;
    ClRcT rc = CL_OK;

    if(!(pMemPart = (ClMemPartT*)handle))
        return NULL;

    if(!size)
        size = 16;/*min size to alloc*/

    clOsalMutexLock(&pMemPart->mutex);
    if(memPartInfoGet(pMemPart->partId, &memStats) == ERROR)
    {
        clOsalMutexUnlock(&pMemPart->mutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("memPartInfoGet failed with [%s]\n", strerror(errno)));
        return mem;
    }
    /*
     * Check if we are hittin the roof. Expand
     */
    if(size >= memStats.numBytesFree)
    {
        if((rc = clMemPartExpand(pMemPart, CL_MEM_PART_EXPANSION_SIZE)) != CL_OK)
        {
            /* ignore and fail on memPartAlloc failure*/
        }
    }
    mem = memPartAlloc(pMemPart->partId, size);
    clOsalMutexUnlock(&pMemPart->mutex);
    if(!mem)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Critical memPartAlloc error for size [%d]\n", size));
        CL_ASSERT(0);
    }
    memset(mem, 0, size);
    return mem;
}

ClPtrT clMemPartRealloc(ClMemPartHandleT handle, ClPtrT memBase, ClUint32T size)
{
    MEM_PART_STATS memPartStats;
    ClPtrT mem = NULL;
    ClMemPartT *pMemPart = NULL;
    ClRcT rc = CL_OK;

    if(!(pMemPart = (ClMemPartT*)handle) )
        return NULL;

    if(!size) size = 16;

    clOsalMutexLock(&pMemPart->mutex);
    if(memPartInfoGet(pMemPart->partId, &memPartStats) == ERROR)
    {
        clOsalMutexUnlock(&pMemPart->mutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("memPartInfoGet for size [%d] failed with [%s]\n", size, strerror(errno)));
        return mem;
    }
    if(size >= memPartStats.numBytesFree)
    {
        if( (rc = clMemPartExpand(pMemPart, CL_MEM_PART_EXPANSION_SIZE) ) != CL_OK)
        {
            /* do nothing now and fall back to realloc.*/
        }
    }
    mem = memPartRealloc(pMemPart->partId, memBase, size);
    clOsalMutexUnlock(&pMemPart->mutex);
    if(!mem)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mem part realloc failure for size [%d]\n", size));
        CL_ASSERT(0);
    }
    return mem;
}

void clMemPartFree(ClMemPartHandleT handle, ClPtrT mem)
{
    STATUS rc = 0;
    ClMemPartT *pMemPart = NULL;
    if( !(pMemPart = (ClMemPartT*)handle) )
        return;
    if(!mem) return;
    clOsalMutexLock(&pMemPart->mutex);
    if( (rc = memPartFree(pMemPart->partId, mem) ) == ERROR )
    {
        /*
         * Ignore
         */
        clOsalMutexUnlock(&pMemPart->mutex);
        return;
    }
    clOsalMutexUnlock(&pMemPart->mutex);
}

