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
#ifndef _CL_HEAP_C_
#error "_CL_HEAP_C_ undefined. clHeapDebug.h should be included only from clHeap.c"
#endif

#include "clMemTracker.h"

#if defined(SOLARIS_BUILD) || defined(POSIX_BUILD)
#define CL_HEAP_NATIVE_DEBUG_SET() do {             \
    if(gHeapDebugLevel > 0 )                        \
    {                                               \
        ClInt32T v = 1;                             \
        if(gHeapDebugLevel > 1 )                    \
        {                                           \
            v = 2;                                  \
        }                                           \
        gClHeapAllocate = heapAllocateNativeDebug;  \
        gClHeapRealloc = heapReallocNativeDebug;    \
        gClHeapCalloc = heapCallocNativeDebug;      \
        gClHeapFree = heapFreeNativeDebug;          \
    }                                               \
}while(0)
#else
#define CL_HEAP_NATIVE_DEBUG_SET() do {             \
    if(gHeapDebugLevel > 0 )                        \
    {                                               \
        ClInt32T v = 1;                             \
        if(gHeapDebugLevel > 1 )                    \
        {                                           \
            v = 2;                                  \
        }                                           \
        mallopt(M_CHECK_ACTION, v);                 \
        gClHeapAllocate = heapAllocateNativeDebug;  \
        gClHeapRealloc = heapReallocNativeDebug;    \
        gClHeapCalloc = heapCallocNativeDebug;      \
        gClHeapFree = heapFreeNativeDebug;          \
    }                                               \
}while(0)
#endif

#define CL_HEAP_MEM_TRACKER_INIT() do {                 \
    if(gHeapDebugLevel > 1 )                            \
    {                                                   \
        ClRcT ret = clMemTrackerInitialize();           \
        CL_ASSERT(ret == CL_OK);                        \
    }                                                   \
}while(0)

#define CL_HEAP_PREALLOCATED_DEBUG_SET() do {   \
    if(gHeapDebugLevel > 0 )                    \
    {                                           \
        gClHeapAllocate = heapAllocateDebug;    \
        gClHeapRealloc = heapReallocDebug;      \
        gClHeapCalloc = heapCallocDebug;        \
        gClHeapFree = heapFreeDebug;            \
        CL_HEAP_MEM_TRACKER_INIT();             \
    }                                           \
}while(0)

#define CL_HEAP_MEM_TRACKER_TRACE(pAddress,size) do {   \
    if(gHeapDebugLevel > 1 )                            \
    {                                                   \
        CL_MEM_TRACKER_TRACE("HEAP",pAddress,size);     \
    }                                                   \
}while(0)

#define CL_HEAP_DEBUG_HOOK_RET(fun) do {                \
    if(gHeapDebugLevel > 0 || gHeapHooksDefined == CL_TRUE) \
    {                                                   \
        return fun;                                     \
    }                                                   \
}while(0)

#define CL_HEAP_DEBUG_HOOK_NORET(fun) do {              \
    if(gHeapDebugLevel > 0 || gHeapHooksDefined == CL_TRUE) \
    {                                                   \
        fun;                                            \
        return;                                         \
    }                                                   \
}while(0)

static ClInt32T gHeapDebugLevel = 0;
static ClBoolT  gHeapHooksDefined = CL_FALSE;
static void clHeapDebugLevelSet(void);
static void *heapAllocateDebug(ClUint32T size);
static void *heapReallocDebug(void *pAddress,ClUint32T size);
static void *heapCallocDebug(ClUint32T numChunks,ClUint32T chunkSize);
static void heapFreeDebug(void *pAddress);

static void *heapAllocateNativeDebug(ClUint32T size);
static void *heapReallocNativeDebug(void *pAddress,ClUint32T size);
static void *heapCallocNativeDebug(ClUint32T numChunks,ClUint32T chunkSize);
static void heapFreeNativeDebug(void *pAddress);
