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
/*This should be included from heap.c only*/
#ifndef _CL_HEAP_C_
#error "_CL_HEAP_C_ undefined.clHeapDebug.c should be only included from clHeap.c"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clCksmApi.h>
#include <clBinSearchApi.h>
#include <clPoolIpi.h>
#include "clMemTracker.h"
#include <clLogUtilApi.h>

/*
 * We rely on cksumming on the chunk underrun part as a previous chunk
 * could run over our meta data. So MAGIC_WORD1 would actually map to a
 * cksum of the meta data/cookie.
 */
#define MAGIC_WORD1 0xdeadbeef
#define MAGIC_WORD2 0xdeadbeef

#define CL_HEAP_MAGIC_WORD1 magic_word1
#define CL_HEAP_MAGIC_WORD2 magic_word2
#define CL_HEAP_SIZE_OVERHEAD (sizeof(ClUint32T))
#define CL_HEAP_WORD_SIZE (sizeof(ClPtrT))

#define CL_HEAP_MEM_TRACKER_ADD(pAddress,size,private) do {         \
    if (gHeapDebugLevel > 1 )                                       \
    {                                                               \
        CL_MEM_TRACKER_ADD ("HEAP", pAddress, size, private,        \
                           gHeapDebugLevel > 2 ? CL_TRUE:CL_FALSE); \
    }                                                               \
}while(0)

#define CL_HEAP_MEM_TRACKER_DELETE(pAddress,size) do {                  \
    if(gHeapDebugLevel > 1 )                                            \
    {                                                                   \
        CL_MEM_TRACKER_DELETE("HEAP",pAddress,size,                     \
                              gHeapDebugLevel > 2 ? CL_TRUE:CL_FALSE);  \
    }                                                                   \
}while(0)

#define CL_HEAP_DETECT_CORRUPTION(pAddress,underrun) do {   \
    if(gHeapDebugLevel > 1 )                                \
    {                                                       \
        clHeapDetectCorruption ((pAddress), (underrun));    \
    }                                                       \
}while(0)

#define __CL_HEAP_OVERHEAD_DEBUG(overhead) do {             \
    overhead = CL_HEAP_SIZE_OVERHEAD + CL_HEAP_OVERHEAD +   \
        sizeof (CL_HEAP_MAGIC_WORD1);                       \
    overhead = CL_ROUNDUP (overhead, CL_HEAP_WORD_SIZE);    \
}while(0)

#define CL_HEAP_OVERHEAD_DEBUG(overhead) do {   \
    __CL_HEAP_OVERHEAD_DEBUG(overhead);         \
    overhead += sizeof (CL_HEAP_MAGIC_WORD2);   \
}while(0)

#define __CL_HEAP_SET_MAGIC_BYTES(address,offset,magic,bytes)   \
    memcpy((ClPtrT)((ClUint8T*)(address) + (offset)),           \
           (ClPtrT)&(magic),bytes)

#define __CL_HEAP_SET_MAGIC(address,offset,magic)       \
    __CL_HEAP_SET_MAGIC_BYTES(address,offset,magic,sizeof((magic)))

#define CL_HEAP_SET_MAGIC(address,offset,magic) do {       \
    __CL_HEAP_SET_MAGIC (address, offset, magic);          \
    CL_HEAP_SET_ADDRESS (address, sizeof ((magic)));       \
}while(0)

#define __CL_HEAP_SET_COOKIE_DEBUG(address,cookie)          \
    memcpy ((ClPtrT)(address), (ClPtrT)&(cookie), sizeof (ClPtrT))

#define CL_HEAP_SET_COOKIE_DEBUG(address,cookie) do {   \
    __CL_HEAP_SET_COOKIE_DEBUG(address,cookie);         \
    CL_HEAP_SET_ADDRESS(address,CL_HEAP_OVERHEAD);      \
}while(0)


#define __CL_HEAP_SET_SIZE(address,size)                            \
    memcpy((ClPtrT)(address),(ClPtrT)&(size),CL_HEAP_SIZE_OVERHEAD)

#define CL_HEAP_SET_SIZE(address,size) do {             \
    __CL_HEAP_SET_SIZE(address,size);                   \
    CL_HEAP_SET_ADDRESS(address,CL_HEAP_SIZE_OVERHEAD); \
}while(0)

#define __CL_HEAP_GET_SIZE(address,size)                            \
    memcpy((ClPtrT)&(size),(ClPtrT)(address),CL_HEAP_SIZE_OVERHEAD)

#define CL_HEAP_GET_SIZE(address,size) do {             \
    __CL_HEAP_GET_SIZE(address,size);                   \
    CL_HEAP_SET_ADDRESS(address,CL_HEAP_SIZE_OVERHEAD); \
}while(0)

#define CL_HEAP_GET_BYTES_MAGIC(chunkSize,size,overhead,bytes) do {     \
    bytes = (chunkSize)-(size)-(overhead)-sizeof(CL_HEAP_MAGIC_WORD2);  \
    bytes = CL_MIN(bytes,sizeof(CL_HEAP_MAGIC_WORD2));                  \
}while(0)

#define CL_HEAP_SET_DEBUG_SIZE(pAddress,chunkSize,size,overhead) do {   \
    ClUint32T __bytes = 0;                                              \
    ClPtrT __tempAddress = (ClPtrT)(pAddress);                          \
    CL_HEAP_SET_ADDRESS(__tempAddress,(ClInt32T)(-                      \
                        sizeof(CL_HEAP_MAGIC_WORD1)-                    \
                        CL_HEAP_OVERHEAD-                               \
                        CL_HEAP_SIZE_OVERHEAD)                          \
                        );                                              \
    __CL_HEAP_SET_SIZE(__tempAddress,(size));                           \
    CL_HEAP_GET_BYTES_MAGIC(chunkSize,size,overhead,__bytes);           \
    __CL_HEAP_SET_MAGIC_BYTES(pAddress,size,                            \
                              CL_HEAP_MAGIC_WORD2,                      \
                              __bytes);                                 \
}while(0)
        
#define CL_HEAP_SET_DEBUG_COOKIE(pAddress,chunkSize,size,cookie) do {   \
    ClUint32T __overhead = 0;                                           \
    ClUint32T __cksum = 0;                                              \
    __CL_HEAP_OVERHEAD_DEBUG (__overhead);                              \
    CL_HEAP_SET_ADDRESS ((pAddress),                                    \
                        __overhead-                                     \
                        sizeof(CL_HEAP_MAGIC_WORD1)-                    \
                         CL_HEAP_OVERHEAD);                             \
    __CL_HEAP_SET_COOKIE_DEBUG (pAddress, cookie);                      \
    clCksm32bitCompute ((ClUint8T*)(pAddress), CL_HEAP_OVERHEAD,        \
                        &__cksum);                                      \
    CL_HEAP_SET_ADDRESS (pAddress, CL_HEAP_OVERHEAD);                   \
    CL_HEAP_SET_MAGIC (pAddress, 0,__cksum);                            \
    CL_HEAP_SET_DEBUG_SIZE(pAddress,chunkSize,size,__overhead);         \
    __CL_HEAP_SET_MAGIC(pAddress,(chunkSize)-__overhead-                \
                        sizeof(CL_HEAP_MAGIC_WORD2),                    \
                        CL_HEAP_MAGIC_WORD2);                           \
}while(0)

#define __CL_HEAP_GET_MAGIC_BYTES(address,offset,magic,bytes)   \
    memcpy ((ClPtrT)&(magic),                                   \
            (ClPtrT)((ClUint8T*)(address) + offset),            \
            (bytes))

#define __CL_HEAP_GET_MAGIC(address,offset,magic)                   \
    __CL_HEAP_GET_MAGIC_BYTES(address,offset,magic,sizeof((magic)))

#define CL_HEAP_GET_MAGIC(pAddress,offset,magic) do {   \
    __CL_HEAP_GET_MAGIC(pAddress,offset,magic);         \
    CL_HEAP_SET_ADDRESS(pAddress,sizeof((magic)));      \
}while(0)

#define __CL_HEAP_GET_COOKIE_DEBUG(address,cookie)          \
    memcpy((ClPtrT)&(cookie),(ClPtrT)(address),sizeof(ClPtrT))

#define CL_HEAP_GET_COOKIE_DEBUG(pAddress,cookie) do {  \
    __CL_HEAP_GET_COOKIE_DEBUG(pAddress,cookie);        \
    CL_HEAP_SET_ADDRESS(pAddress,CL_HEAP_OVERHEAD);     \
}while(0)

/* Given an address,it fetches the cookie,cksum and word1 */
#define CL_HEAP_GET_DEBUG_COOKIE(pAddress,cookie,word1,cksum,size,overhead) \
 do {                                                                   \
    __CL_HEAP_OVERHEAD_DEBUG (overhead);                                \
    CL_HEAP_SET_ADDRESS ((pAddress),                                    \
                        - sizeof (CL_HEAP_MAGIC_WORD1) -                \
                         CL_HEAP_OVERHEAD -                             \
                         CL_HEAP_SIZE_OVERHEAD);                        \
    CL_HEAP_GET_SIZE(pAddress,size);                                    \
    clCksm32bitCompute((ClUint8T*)(pAddress),                           \
                       CL_HEAP_OVERHEAD,                                \
                       &(cksum));                                       \
    CL_HEAP_GET_COOKIE_DEBUG (pAddress, cookie);                        \
    CL_HEAP_GET_MAGIC (pAddress,0,word1);                               \
    CL_HEAP_SET_ADDRESS (pAddress, - (ClInt32T)overhead);               \
}while(0)

#define CL_HEAP_MAGIC_VERIFY_BYTES(word,magic,bytes) (  \
    memcmp ((ClPtrT)&(word), &(magic),(bytes)) == 0     \
    )

#define CL_HEAP_MAGIC_VERIFY(word,magic)                    \
    CL_HEAP_MAGIC_VERIFY_BYTES(word,magic,sizeof((magic)))

#define CL_HEAP_MAGIC_WORD1_VERIFY(word,magic)  \
    CL_HEAP_MAGIC_VERIFY (word, magic)

#define CL_HEAP_MAGIC_WORD2_VERIFY(word)            \
    CL_HEAP_MAGIC_VERIFY (word, CL_HEAP_MAGIC_WORD2)

#define CL_HEAP_OVERRUN_VERIFY(word,bytes)                              \
    ((bytes) ? CL_HEAP_MAGIC_VERIFY_BYTES(word,CL_HEAP_MAGIC_WORD2,bytes):1)
	
#define HEAP_LOG_AREA_HEAP		"HEAP"
#define HEAP_LOG_CTX_DEBUG		"DBG"	

static const ClUint32T magic_word1 = MAGIC_WORD1;
static const ClUint32T magic_word2 = MAGIC_WORD2;
static ClRcT clHeapDetectCorruption( ClPtrT  pAddress, ClBoolT underrun);

static
void
clHeapDebugLevelSet(void)
{
    ClCharT *pLevel = getenv ("CL_HEAP_DEBUG");
    if (NULL != pLevel)
    {
        ClCharT *pTemp = NULL;
        gHeapDebugLevel = (ClInt32T)strtol (pLevel, &pTemp, 10);
        if ('\0' != *pTemp)
        {
            gHeapDebugLevel = 0;
            return;
        }
    }
}

/*Check for the corruption of this chunk*/
static
ClBoolT
clHeapCheckCorruption(
        ClPtrT      pChunk,
        ClUint32T   size)
{
    ClBoolT     corrupt = CL_FALSE;
    ClUint32T   magic2  = 0;

    __CL_HEAP_GET_MAGIC (pChunk, size-sizeof(magic2), magic2);

    if (!CL_HEAP_MAGIC_WORD2_VERIFY (magic2))
    {
        corrupt = CL_TRUE;
        CL_OUTPUT ("Buffer overrun detected for chunk:%p, size:%d\n",
                pChunk, size);
        CL_HEAP_MEM_TRACKER_TRACE (pChunk, size);
    }
    return corrupt;
}

/*Try to detect the corruption of the chunk*/
static
ClRcT
clHeapDetectCorruption(
        ClPtrT  pAddress,
        ClBoolT underrun)
{
    ClPtrT pCookie = NULL;
    ClPtrT pStart = NULL;
    ClUint32T size;
    ClRcT rc;
    CL_OUTPUT ("Trying to detect the cause of the %s for chunk:%p..\n",
           underrun == CL_TRUE ? "underrun" : "overrun", pAddress);

    rc = clMemTrackerGet (pAddress, &size, &pCookie);
    if (rc != CL_OK)
    {
        goto out;
    }

    rc = clPoolStartGet (pCookie, &pStart);
    if (rc == CL_OK)
    {
        /*Check preceding allocated chunks for consecutive corruption*/
        ClBoolT corrupt = CL_TRUE;
        pAddress = (ClPtrT)((ClUint8T*)pAddress - size);
        while (pAddress >= pStart && corrupt == CL_TRUE)
        {
            corrupt = clHeapCheckCorruption (pAddress, size);
            pAddress = (ClPtrT)((ClUint8T*)pAddress - size);
        }
    }
    out:
    return rc;
}

/* The debug versions of the heap routines */
static
ClPtrT
heapAllocateDebug(
        ClUint32T size)
{
    ClPtrT          pAddress    = NULL;
    ClPoolConfigT*  pPoolConfig = NULL;
    ClPoolConfigT   poolKey     = { 0 };
    ClPoolT         poolHandle;
    ClPtrT           pCookie     = NULL;
    ClRcT           rc;
    ClUint32T       overhead    = 0;

    /* Add the overhead over here for debug allocation.  */
    CL_HEAP_OVERHEAD_DEBUG (overhead);

    size += overhead;

    poolKey.chunkSize = size;

    /*
     * Do a binary search to get an index into the chunk config and hence
     * pool handle to allocate from.
     */
    rc = clBinarySearch ((ClPtrT)&poolKey, gHeapList.heapConfig.pPoolConfig,
            gHeapList.heapConfig.numPools, sizeof (ClPoolConfigT),
            clPoolCmp, (ClPtrT*)&pPoolConfig);
    if (rc != CL_OK)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,
                   "Error in getting pool handle for size:%d\n", size);
        goto out;
    }

    poolHandle = gHeapList.pPoolHandles [
        pPoolConfig - gHeapList.heapConfig.pPoolConfig ];

    rc = clPoolAllocate (poolHandle, (ClUint8T**) &pAddress, &pCookie);
    if (rc != CL_OK)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);
        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,
                   "Error allocating memory from pool size:%d for input "
                   "size:%d\n", pPoolConfig->chunkSize, size);
        goto out;
    }

    CL_HEAP_MEM_TRACKER_ADD (pAddress, pPoolConfig->chunkSize, pCookie);

    size -= overhead;

    CL_HEAP_SET_DEBUG_COOKIE (pAddress, pPoolConfig->chunkSize, size, pCookie);

    /*Zero off the contents till ASP gets cured*/
    memset (pAddress, 0, size);

    out:
    return (ClPtrT)pAddress;
}

static ClPtrT heapReallocDebug(ClPtrT pAddress,ClUint32T size)
{
    ClPtrT pCookie = NULL;
    ClPtrT pTempAddress = pAddress;
    ClPtrT pRetAddress = NULL;
    ClUint32T chunkSize = 0;
    ClUint32T capacity = 0;
    ClUint32T oldSize = 0;
    ClUint32T magic1 = 0;
    ClUint32T magic2 = 0;
    ClUint32T cksum = 0;
    ClUint32T overhead = 0;
    ClUint32T bytes = 0;
    ClRcT rc;

    if(pAddress == NULL)
    {
        /*call pool allocate without wasting time*/
        goto allocate;
    }

    CL_HEAP_GET_DEBUG_COOKIE(pTempAddress,pCookie,magic1,cksum,oldSize,overhead);

    if(!CL_HEAP_MAGIC_WORD1_VERIFY(magic1,cksum))
    {
        CL_OUTPUT("Buffer underrun detected for address:%p\n",pTempAddress);
        CL_HEAP_MEM_TRACKER_TRACE(pTempAddress,0);
        CL_HEAP_DETECT_CORRUPTION(pTempAddress,CL_TRUE);
        CL_ASSERT(0);
        goto out;
    }

    clPoolChunkSizeGet(pCookie,&chunkSize);

    CL_HEAP_GET_BYTES_MAGIC(chunkSize,oldSize,overhead,bytes);

    /*Now check for the trailer*/
    
    __CL_HEAP_GET_MAGIC_BYTES(pTempAddress,overhead+oldSize,magic2,bytes);

    if(!CL_HEAP_OVERRUN_VERIFY(magic2,bytes))
    {
        goto out_overrun;
    }

    __CL_HEAP_GET_MAGIC(pTempAddress,
                        chunkSize-sizeof(CL_HEAP_MAGIC_WORD2),
                        magic2);

    if(!CL_HEAP_OVERRUN_VERIFY(magic2,sizeof(magic2)))
    {
        goto out_overrun;
    }

    overhead += sizeof(CL_HEAP_MAGIC_WORD2);

    capacity = chunkSize - overhead;

    if(0 < size && capacity >= size)
    {
        pRetAddress = pAddress;
        /*Set the magic bytes*/
        if(oldSize != size)
        {
            CL_HEAP_SET_DEBUG_SIZE(pRetAddress,chunkSize,size,
                                   overhead - sizeof(CL_HEAP_MAGIC_WORD2)
                                   );
        }
        goto out;
    }

    /*
     * Allocate a new chunk. Copy the contents of the old chunk to this guy
     * and release the old chunk
    */

    allocate:
    pRetAddress = heapAllocateDebug(size);

    if(pRetAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);

        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,"Error allocating memory for size:%d\n",size);
        goto out;
    }
    /*
     * Copy the contents of the old chunk to the new chunk and release
     * the old chunk
    */
    if(pAddress != NULL)
    {
        if(0 < size)
        {
            memcpy(pRetAddress,pAddress,capacity);
        }
        CL_HEAP_MEM_TRACKER_DELETE(pTempAddress,chunkSize);
        rc = clPoolFree((ClUint8T*)pTempAddress,pCookie);
        if(rc != CL_OK)
        {
            clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,"Error freeing the old chunk:%p\n",pAddress);
            heapFreeDebug(pRetAddress);
            pRetAddress = NULL;
            goto out;
        }
    }

    out:
    return pRetAddress;

    out_overrun:

    CL_OUTPUT("Buffer overrun detected for address:%p, chunkSize:%d\n",pTempAddress,chunkSize);

    CL_HEAP_MEM_TRACKER_TRACE(pTempAddress,chunkSize);

    CL_ASSERT(0);

    return NULL;
}

/*Just call pool allocate without wasting time*/
static ClPtrT heapCallocDebug(ClUint32T numChunks,ClUint32T chunkSize)
{
    ClUint32T size;
    ClPtrT pAddress ;
    size = numChunks * chunkSize;
    pAddress = heapAllocateDebug(size);
    if(pAddress == NULL)
    {
        CL_HEAP_MEM_TRACKER_TRACE(NULL,size);

        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,"Error in pool calloc for size:%d\n",size);
        goto out;
    }
    memset(pAddress,0,size);
    out:
    return pAddress;
}

/*Free to our pool*/
static
void
heapFreeDebug(
        ClPtrT pAddress)
{
    ClPtrT      pCookie     = NULL;
    ClUint32T   overhead    = 0;
    ClUint32T   size        = 0;
    ClUint32T   bytes       = 0;
    ClUint32T   magic1      = 0;
    ClUint32T   magic2      = 0;
    ClUint32T   cksum       = 0;
    ClUint32T   chunkSize   = 0;
    ClRcT       rc          = CL_OK;

    /*First get the cookie or extended pool handle*/
    CL_HEAP_GET_DEBUG_COOKIE (pAddress,pCookie,magic1,cksum,size,overhead);

    if (!CL_HEAP_MAGIC_WORD1_VERIFY (magic1, cksum))
    {
        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,
                   "Buffer underrun detected for address:%p\n", pAddress);
        CL_HEAP_MEM_TRACKER_TRACE (pAddress, 0);
        CL_HEAP_DETECT_CORRUPTION (pAddress, CL_TRUE);
        CL_ASSERT(0);
        goto out;
    }

    clPoolChunkSizeGet(pCookie,&chunkSize);

    CL_HEAP_GET_BYTES_MAGIC(chunkSize,size,overhead,bytes);

    __CL_HEAP_GET_MAGIC_BYTES(pAddress,overhead+size,magic2,bytes);

    if(!CL_HEAP_OVERRUN_VERIFY(magic2,bytes))
    {
        goto out_overrun;
    }

    __CL_HEAP_GET_MAGIC(pAddress,
                        chunkSize-sizeof(CL_HEAP_MAGIC_WORD2),
                        magic2);

    if(!CL_HEAP_OVERRUN_VERIFY(magic2,sizeof(magic2)))
    {
        goto out_overrun;
    }
            
    CL_HEAP_MEM_TRACKER_DELETE(pAddress,chunkSize);

    rc = clPoolFree((ClUint8T*)pAddress,pCookie);
    if(rc != CL_OK)
    {
        clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,"Error freeing memory:%p\n",pAddress);
    }
    out:

    return ;

    out_overrun:

    clLogError(HEAP_LOG_AREA_HEAP,HEAP_LOG_CTX_DEBUG,"Buffer overrun detected for "\
                                   "address:%p,size:%d\n",pAddress,\
                                   chunkSize);
    CL_HEAP_MEM_TRACKER_TRACE(pAddress,chunkSize);

    CL_ASSERT(0);

    return;
}

/*
 * Malloc in debug mode would just fall back to native allocations without
 * any statistics gathering
 */
static
ClPtrT
heapAllocateNativeDebug(
        ClUint32T size)
{
    ClPtrT pAddress = NULL;
    pAddress = malloc(size);
    if(pAddress != NULL)
    {
        /*Zero off the contents till ASP is cured of its diseases*/
        memset(pAddress,0,size);
    }
    return pAddress;
}

static ClPtrT heapReallocNativeDebug(ClPtrT pAddress,ClUint32T size)
{
    return realloc(pAddress,size);
}

static ClPtrT heapCallocNativeDebug(ClUint32T chunk,ClUint32T chunkSize)
{
    return calloc(chunk,chunkSize);
}

static
void
heapFreeNativeDebug(
        ClPtrT pAddress)
{
    free (pAddress);
}
