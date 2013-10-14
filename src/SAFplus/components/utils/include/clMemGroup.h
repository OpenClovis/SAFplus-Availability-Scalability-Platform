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

#ifndef _CL_MEM_GROUP_H_
#define _CL_MEM_GROUP_H_

#include <clCommon.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
 * \file
 * \brief Memory Groups
 * \ingroup heap_apis
 *
 * \addtogroup heap_apis
 * \{
 */

#define CL_MEM_GRP_ARRAY_SIZE 32

/**
 * Memory Group tracking structure.  A memory group is a set of individually allocated chunks that must all be freed together.
 *
 */    
typedef struct ClMemGroup
{
    /** An array of pointers to memory buffers that memory is served out of.  Each buffer is 2x the size of the prior */
    char*   buffers[CL_MEM_GRP_ARRAY_SIZE];
    /** The beginning of the unallocated area of each buffer */
    ClWordT bufPos[CL_MEM_GRP_ARRAY_SIZE];
    /** The size of the first buffer.  This should be set to some multiple of the size of your typical allocation. */
    ClWordT fundamentalSize;
    /** The last allocated buffer */
    int     curBuffer;    
} ClMemGroupT;

/**
 ************************************
 *  \brief Initialize a memory group.
 *
 *  \par Header File:
 *  clMemGroup.h
 *
 *  \param obj (in) Memory to store the memory group structure in (i.e. the 'this' pointer).  Create on stack or allocate on the heap, sizeof(ClMemGroupT)
 *
 *  \par Description:
 *  Initializes the ClMemGroupT object to handle memory allocation requests.
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clMemGroupInit(), clMemGroupAlloc(), clMemGroupFreeAll(), clMemGroupDelete()
 *
 */
void clMemGroupInit(ClMemGroupT* obj,ClWordT baseSize);

/**
 ************************************
 *  \brief Allocate memory from the group.
 *
 *  \par Header File:
 *  clMemGroup.h
 *
 *  \param obj (in) The memory group (i.e. the 'this' pointer).
 *  \param amt (in) The number of bytes to allocate.
 *
 *  \par Return values:
 *  \arg A valid pointer on success. \n
 *  \arg \e NULL On memory allocation failure.  This will only happen if the system runs out of memory.
 *
 *  \par Description:
 *  This function allocates some memory from within the memory group and returns a pointer to it.
 *  If there is not enough memory in the group, more is allocated to the group using clHeapAllocate.
 *
 *  DO NOT free this memory using clHeapFree or free!!!  You may only free the entire group through the clMemGroupFreeAll(), or clMemGroupDelete() functions.
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clMemGroupInit(), clMemGroupAlloc(), clMemGroupFreeAll(), clMemGroupDelete()
 *
 */    
void* clMemGroupAlloc(ClMemGroupT* obj,unsigned int amt);

/**
 ************************************
 *  \brief Free all memory in the group.
 *
 *  \par Header File:
 *  clMemGroup.h
 *
 *  \param obj (in) The memory group (i.e. the 'this' pointer).
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clMemGroupInit(), clMemGroupAlloc(), clMemGroupFreeAll(), clMemGroupDelete()
 *
 */
void clMemGroupFreeAll(ClMemGroupT* obj);

/**
 ************************************
 *  \brief Delete all allocations in the memory group, and zero the structure.
 *
 *  \par Header File:
 *  clMemGroup.h
 *
 *  \param obj (in) The memory group (i.e. the 'this' pointer).
 *
 *  \par Description:
 *  The pointer cannot be used as a MemGroup after this function is called.  To re-initialize this pointer call clMemGroupInit() again.
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clMemGroupInit(), clMemGroupAlloc(), clMemGroupFreeAll(), clMemGroupDelete()
 *
 */
void clMemGroupDelete(ClMemGroupT* obj);

    
#ifdef __cplusplus
}
#endif

#endif
    

/** \} */
