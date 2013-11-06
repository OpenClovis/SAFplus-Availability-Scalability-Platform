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
#ifndef _CL_BUFFER_C_
#error "_CL_BUFFER_C_ undefined. clBufferDebug.c should be included from clBuffer.c"
#endif

#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clMemTracker.h>

/*Mem tracking for buffer mgmt.*/

#define CL_BUFFER_MEM_TRACKER_ADD(pAddress,size,private) do {       \
    CL_MEM_TRACKER_ADD("BUFFER",pAddress,size,private,              \
                       gBufferDebugLevel > 1 ? CL_TRUE:CL_FALSE);   \
}while(0)

#define CL_BUFFER_MEM_TRACKER_DELETE(pAddress,size) do {                \
    CL_MEM_TRACKER_DELETE("BUFFER",pAddress,size,                       \
                          gBufferDebugLevel > 1 ? CL_TRUE:CL_FALSE);    \
}while(0)


static void clBufferDebugLevelSet(void)
{
    ClCharT *pLevel = getenv("CL_BUFFER_DEBUG");
    if(pLevel)
    {
        ClCharT *pTemp = NULL;
        while(isspace(*pLevel))
            ++pLevel;
        gBufferDebugLevel = (ClInt32T)strtol(pLevel,&pTemp,10);
        if(*pTemp)
        {
            gBufferDebugLevel = 0;
        }
    }
}

static void clBufferDebugLevelUnset(void)
{
    gBufferDebugLevel = 0;
#ifndef SOLARIS_BUILD
    unsetenv("CL_BUFFER_DEBUG");
#endif
}

static ClRcT clBufferFromPoolAllocateDebug(
                                           ClPoolT pool,
                                           ClUint32T actualLength,
                                           ClUint8T **ppChunk,
                                           void **ppCookie
                                           )
{

    ClRcT rc;
    rc = clPoolAllocate(pool,ppChunk,ppCookie);
    if(rc == CL_OK)
    {
        CL_BUFFER_MEM_TRACKER_ADD(*ppChunk,actualLength,*ppCookie);
    }
    else
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Error allocating buffer chunk from " 
                                      " pool of size: %d\n",actualLength);
    }
    return rc;
}

static ClRcT clBufferFromPoolFreeDebug(ClUint8T *pChunk,ClUint32T size,void *pCookie)
{
    ClRcT rc;
    CL_BUFFER_MEM_TRACKER_DELETE(pChunk,size);
    rc = clPoolFree(pChunk,pCookie);
    if(rc != CL_OK)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Error freeing buffer chunk:%p "
                                       "of size:%d to the pool\n",pChunk,size);
    }
    return rc;
}
