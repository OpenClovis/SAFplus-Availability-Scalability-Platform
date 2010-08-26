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
#ifndef _CL_BUFFER_C_
#error _CL_BUFFER_C_ undefined. clBufferDebug.h should be only defined from clBuffer.c
#endif

static void clBufferDebugLevelSet(void);
static void clBufferDebugLevelUnset(void);

static ClRcT clBufferFromPoolAllocateDebug(
                                           ClPoolT pool,
                                           ClUint32T actualLength,
                                           ClUint8T **ppChunk,
                                           void **ppCookie
                                           );

static ClRcT clBufferFromPoolFreeDebug(ClUint8T *pChunk,ClUint32T size,void *pCookie);

#define CL_BUFFER_MEM_TRACKER_INIT() do {           \
    ClRcT rc = clMemTrackerInitialize();            \
    CL_ASSERT(rc == CL_OK);                         \
}while(0)                                           \

#define CL_BUFFER_DEBUG_SET() do {                                  \
    if(gBufferDebugLevel > 0 )                                      \
    {                                                               \
        gClBufferFromPoolAllocate = clBufferFromPoolAllocateDebug;  \
        gClBufferFromPoolFree = clBufferFromPoolFreeDebug;          \
        gClBufferLengthCheck = clBufferLengthCheckDebug;            \
        CL_BUFFER_MEM_TRACKER_INIT();                               \
    }                                                               \
    else                                                            \
    {                                                               \
        gClBufferLengthCheck = clBufferLengthCheck;                 \
    }                                                               \
}while(0)

#if defined(SOLARIS_BUILD) || defined(POSIX_BUILD)
#define CL_BUFFER_NATIVE_DEBUG_SET() do {                       \
        if(gBufferDebugLevel > 0 )                              \
        {                                                       \
            gClBufferLengthCheck = clBufferLengthCheckDebug;    \
        }                                                       \
        else                                                    \
        {                                                       \
            gClBufferLengthCheck = clBufferLengthCheck;         \
        }                                                       \
}while(0)
#else
#define CL_BUFFER_NATIVE_DEBUG_SET() do {                       \
        if(gBufferDebugLevel > 0 )                              \
        {                                                       \
            ClInt32T v = CL_MIN(gBufferDebugLevel, 2) ;         \
            mallopt(M_CHECK_ACTION, v);                         \
            gClBufferLengthCheck = clBufferLengthCheckDebug;    \
        }                                                       \
        else                                                    \
        {                                                       \
            gClBufferLengthCheck = clBufferLengthCheck;         \
        }                                                       \
}while(0)
#endif
