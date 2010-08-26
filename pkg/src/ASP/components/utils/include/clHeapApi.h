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

/**
 * \file 
 * \brief Header file of Heap Management related APIs
 * \ingroup heap_apis
 */

/**
 *  \addtogroup heap_apis
 *  \{
 */


#ifndef _CL_HEAP_API_H_
#define _CL_HEAP_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clPoolIpi.h>
#include <clMemStats.h>

/**
 * Heap Allocation modes. System mode maps to malloc/free calls. 
 */
typedef enum {
    /**
     * OpenClovis implementation of the memory management library.
     */
    CL_HEAP_PREALLOCATED_MODE, 
    
    /**
     * Native C mode. It maps to the memory APIs provided by \e libc.
     */
    CL_HEAP_NATIVE_MODE, 
    
    /**
     * Custom pools. The application developer can plug in customized 
     * memory management library calls.
     */
    CL_HEAP_CUSTOM_MODE, 
    
} ClHeapModeT;

/**
 * ClHeapConfigT to be fetched by EO and contains the configuration of the heap 
 * library
 */
typedef struct {
    /**
     * Allocation mode. This can be either CL_HEAP_NATIVE_MODE, 
     * CL_HEAP_PREALLOCATED_MODE, or CL_HEAP_CUSTOM_MODE.
     */
    ClHeapModeT mode; 

    /**
     * A pool can grow even after it exhausts its current allocation. 
     * This attribute configures a pool in lazy mode for pool expansion. 
     * There are two modes for configuring the pool:
     *    \arg Lazy mode - The incremented pool does not initialize until an 
     *         allocation is made from the extended portion of the pool. 
     *         Lazy mode speeds up the initialization of the application, 
     *         but shifts the penalty of pool initialization to a later 
     *         allocation.
     *    \arg Normal mode - The incremented pool initializes as soon as it is 
     *         acquired by the memory management library during the creation of 
     *         the pool. 
     */    
    ClBoolT lazy; 

    /**
     * Array of pool configurations
     */    
    ClPoolConfigT *pPoolConfig;

    /**
     * Number of pools in the \e pPoolConfig array
     */    
    ClUint32T numPools;    
} ClHeapConfigT;

/*****************************************************************************
 *  Functions
 *****************************************************************************/

/**
 ************************************
 *  \brief Allocates memory of the requested size.
 *
 *  \par Header File:
 *  clHeapApi.h
 *
 *  \param size (in) Number of memory bytes to be allocated.
 *
 *  \par Return values:
 *  \arg A valid pointer on success. \n
 *  \arg \e NULL On memory allocation failure.
 *
 *  \par Description:
 *  This function allocates memory of a specified size. The returned memory is 
 *  aligned at an 8-byte boundary. When the heap library is configured to 
 *  \e CL_HEAP_PREALLOCATED_MODE, it returns memory of minimum size. This memory 
 *  size is greater than or equal to the requested size. If size is specified 
 *  as 0, the system returns a valid pointer pointing to a chunk of minimum 
 *  size that is previously configured.
 *  If heap library is configured to \e CL_HEAP_PREALLOCATED_MODE, failure
 *  of clHeapAllocate() for one size of memory does not mean that this function
 *  will fail for other sizes. For more information, refer to man page of 
 *  malloc(3). 
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clHeapCalloc(), clHeapFree(), clHeapRealloc(), 
 *
 */
ClPtrT clHeapAllocate(CL_IN ClUint32T size);
 /**
 ************************************
 *  \brief Frees a pre-allocated memory.
 *
 *  \par Header File:
 *  clHeapApi.h
 *
 *  \param  pAddress (in) Block of memory to be freed.
 *
 *  \par Return values:
 *   None.
 *
 *  \par Description:
 *  This function is used to free memory. \e pAddress should be a valid pointer
 *  allocated through a previous call to either clHeapAllocate(), 
 *  clHeapRealloc(), or clHeapCalloc(). \e pAddress should not be used after 
 *  a call to clHeapFree(). NULL is a valid value for \e pAddress. 
 *  For more information, refer to man page of free(3).
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clHeapAllocate(), clHeapCalloc(), clHeapRealloc()
 */
void clHeapFree(CL_IN ClPtrT pAddress);
/**
 ************************************
 *  \brief Allocates memory for an array and initializes it to zero.
 *
 *  \par Header File:
 *  clHeapApi.h
 *
 *  \param numChunks (in) Number of chunks to be allocated.
 *  \param chunkSize (in) Size of each chunk
 *  \par Return values:
 *  \arg A valid pointer on success. \n
 *  \arg \e NULL On memory allocation failure.
 *
 *  \par Description:
 *  This function allocates memory of a specific size. The memory chunk, 
 *  it returns, is aligned at an 8-byte boundary. If \e CL_HEAP_PREALLOCATED_MODE
 *  is selected during heap configuration, failure of clHeapAllocate() for one 
 *  size of memory does not mean that it will fail for other sizes also. For
 *  more information, refer to man page, \e malloc(3). Also refer to man page 
 *  of \e calloc(3).
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clHeapAllocate(), clHeapFree(), clHeapRealloc()
 */
ClPtrT clHeapCalloc(CL_IN ClUint32T numChunks,CL_IN ClUint32T chunkSize);
/*****************************************************************************/

#ifndef __KERNEL__
/*****************************************************************************/
/**
 ************************************
 *  \brief Initializes the heap library.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pHeapConfig (in) Pointer to the configuration to be used by heap library
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER  \e pHeapConfig or \e pPoolConfig
 *   (member of \e pHeapConfig) is NULL.
 *  \retval CL_ERR_INVALID_PARAMETER \e numPools (member of \e pHeapConfig) is 0.
 *  \retval CL_ERR_NO_MEMORY Heap library is out of memory and cannot proceed 
 *   further.
 *
 *  \par Description:
 *  This function is used to initialize the heap library. It is called during 
 *  the initialization of the EO (Execution Object).
 *  The heap library must be initialized before it can be used to allocate 
 *  memory. The caller should allocate and free the \e pHeapConfig parameter. 
 *  Any configuration provided through this function to the heap library 
 *  cannot be changed while the process calling this function is executing.
 *  \par 
 *  During the lifetime of a process, this function must be called only once, 
 *  preferably during process initialization. Subsequent calls to this function
 *  are ignored and it returns \e CL_OK without changing anything.
 *
 *  \par Library File:
 *   libClUtils
 *
 *  \sa clHeapLibFinalize()
 *
 */

ClRcT clHeapLibInitialize(CL_IN const ClHeapConfigT *pHeapConfig);
/*****************************************************************************/

/**
 ************************************
 *  \brief Finalizes the heap library.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \par Parameters:
 *   None
 * 
 *  \retval CL_OK The API executed successfully
 *  \retval CL_ERR_NOT_INITIALIZED heap library is not initialized through a 
 *   previous call to clHeapLibInitialize() or it is finalized through a 
 *   call to clHeapLibFinalize().
 *
 *  \par Description:
 *  This function is used to finalize the heap library. After finalizing
 *  the heap library, it must not be used to allocate any memory or free
 *  previously allocated memory. 
 *
 *  \par Library File:
 *   libClUtils
 *
 *  \sa clHeapLibInitialize()
 *
 */

ClRcT clHeapLibFinalize(void);
/*****************************************************************************/
ClRcT clHeapInit(void);
/*****************************************************************************/
ClRcT clHeapExit(void);
/*****************************************************************************/

/**
 ************************************
 *  \brief Changes the size of the memory block (chunk).
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pAddress (in) Original pointer to the memory block (chunk).
 *  \param size (in) New size of the memory block (chunk). 
 * 
 *  \returns The function returns a valid pointer on success and returns NULL 
 *   on memory allocation failure.
 *
 *  \par Description:
 *  This function is used to change the size of the memory block pointed by 
 *  \e pAddress to \e size in bytes. The new address returned is aligned at 
 *  an 8-byte boundary. The contents of the returned memory block is unchanged 
 *  to the minimum of the old and new sizes. The contents of the memory over 
 *  the size of the previous block of memory is un initialized.
 *  - If \e pAddress is NULL, the call is equivalent to \e clHeapAllocate (size).
 *  - If \e size is zero, the call is equivalent to \e clHeapFree(pAddress) and
 *    the function returns NULL.
 *  - If \e pAddress is NULL and size is zero, it is still equal to 
 *    \e clHeapAllocate (0).
 *  - If \e pAddress is not NULL, it must have been returned by an earlier call
 *    to clHeapAllocate()/clHeapRealloc()/clHeapCalloc().
 *  \par
 *  If clHeapRealloc() fails, the original memory block remains untouched 
 *  (if is not freed or moved). If  clHeapRealloc() succeeds, \e pAddress
 *  should no longer be used. For more information, refer to man page of 
 *  \e realloc(3).
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clHeapAllocate(), clHeapFree(), clHeapCalloc()
 *
 */

ClPtrT clHeapRealloc(CL_IN ClPtrT pAddress,CL_IN ClUint32T size);
/*****************************************************************************/

/**
 ************************************
 *  \brief Shrinks the configured pools of memory.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pShrinkOptions (in) \e shrinkFlags, a member of this structure 
 *  indicates to what extent the existing pools can be shrunk.
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED The heap library is not initialized by a 
 *   previous call to clHeapInitialize()
 *
 *  \par Description:
 *  This function is used to shrink the memory used by the pools of all 
 *  \e chunkSize so that next usages of pools can grow without violating
 *  the configured process wide upper limit of memory. These shrink options
 *  are used for all pools of heap library.
 *
 *  \par Library File:
 *   libClUtils
 *
 */

ClRcT clHeapShrink(CL_IN const ClPoolShrinkOptionsT *pShrinkOptions);
/*****************************************************************************/

/**
 ************************************
 *  \brief Returns the mode set during configuration.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pMode (out) Configuration mode returned by the function
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If this function is called before heap 
 *  initialization through a clHeapIntialize() function.
 *  \retval CL_ERR_NULL_POINTER If \e pMode is passed as NULL.
 *
 *  \par Description:
 *  This function returns the configuration mode of the heap in the current 
 *  process. It can be one of the following values:
 *  CL_HEAP_NATIVE_MODE, 
 *  CL_HEAP_PREALLOCATED_MODE, or 
 *  CL_HEAP_CUSTOM_MODE.
 *
 *  \par Library File:
 *   libClUtils
 *
 */

ClRcT clHeapModeGet(CL_OUT ClHeapModeT *pMode);
/*****************************************************************************/

/**
 ************************************
 *  \brief Returns the statistics collected by heap module.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pHeapStats (out) Pointer to the memory block where heap module will 
 *   copy the statistics.
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED Heap library is not initialized.
 *  \retval CL_ERR_NULL_POINTER The parameter \e pHeapStats is passed as NULL.
 *
 *  \par Description:
 *  This function is used by heap to collect the statistics about its usage. 
 *  Other components can invoke this function to access these statistics.
 *
 *  \par Library File:
 *   libClUtils
 *
 */

ClRcT clHeapStatsGet(CL_OUT ClMemStatsT *pHeapStats);
/*****************************************************************************/

/**
 ************************************
 *  \brief Returns the statistics collected by heap library for an individual 
 *   pool.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param numPools (in) Number of pools for which the statistics are required.
 *   This is used as the size for \e pPoolSize and \e pPoolStats.
 *  \param pPoolSize (out) Pointer to array which contains the sizes of various
 *   pools.
 *  \param pHeapPoolStats (out) Pointer to array which contains the statistics 
 *   of various pools.
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED This function is called before initializing 
 *   the heap library using the clHeapIntialize() function.
 *  \retval CL_ERR_NULL_POINTER Either \e pPoolSize or \e pPoolStats or both 
 *   the parameters is NULL.
 *
 *  \par Description:
 *   This function is used by components to retrieve statistics about usage of 
 *   various pools and their current size. The heap module gathers this information.
 *  \par
 *   If \e numPools is less than the number of pools configured in heap, the 
 *   function returns the size and statistics of the first pool arranged in 
 *   increasing order of chunk sizes. If \e numPools is greater than the number
 *   of pools configured, only first n entries of \e pPoolSize and 
 *   \e pPoolStats are valid, where \e n is the number of pools configured.
 *
 *  \par Library File:
 *   libClUtils
 *
 */

ClRcT clHeapPoolStatsGet(CL_IN ClUint32T numPools,CL_OUT ClUint32T *pPoolSize,CL_OUT ClPoolStatsT *pHeapPoolStats);
/*****************************************************************************/

/**
 ************************************
 *  \brief Customizes the initialization of heap library in CL_HEAP_CUSTOM_MODE.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param pHeapConfig (in) Pointer to configuration information used by heap 
 *  library.
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NULL_POINTER Either \e pHeapConfig or \e pPoolConfig, 
 *   a member of \e pHeapConfig is passed as NULL.
 *  \retval CL_ERR_NO_MEMORY Heap library is out of memory and cannot proceed 
 *   further.
 *
 *  \par Description:
 *  This function is used to customize the initialization of the heap library.
 *  This is an open function and the application developer should implement
 *  this function.This is called only when an application indicates that it
 *  needs to customize heap through CL_HEAP_CUSTOM_MODE. The function is
 *  available in <em> ASP/models/<model-name>/config/clHeapCustom.c.</em>
 *  This function must call clHeapHooksRegister() with the appropriate
 *  function pointers to override the implementation of heap provided by
 *  OpenClovis.
 *
 *  \par Library File:
 *  libClUtils
 *
 *  \sa clHeapLibInitialize(), clHeapLibCustomFinalize()
 *
 */

ClRcT clHeapLibCustomInitialize(const ClHeapConfigT *pHeapConfig);
/*****************************************************************************/

/**
 ************************************
 *  \brief Customizes the finalization of heap library in CL_HEAP_CUSTOM_MODE.
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \par Parameters:
 *   None 
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED Heap library is not initialized through a 
 *   previous call to clHeapLibInitialize() or it is finalized using 
 *   clHeapLibFinalize().
 *
 *  \par Description:
 *   This function is used to customize the finalization of the heap library.
 *   This is an open function and the application developer should implement 
 *   this function. This is called only when an application indicates that 
 *   it needs to customize heap through CL_HEAP_CUSTOM_MODE. The function is
 *   available in <em>ASP/models/<model-name>/config/clHeapCustom.c</em>.
 *   This function must call clHeapHooksRegister() with the appropriate
 *   function pointers to override the implementation of heap provided by
 *   OpenClovis.
 *
 *  \par Library File:
 *   libClUtils
 *
 *  \sa clHeapLibFinalize(), clHeapLibCustomInitialize()
 *
 */

ClRcT clHeapLibCustomFinalize(void);
/*****************************************************************************/

/**
 ************************************
 *  \brief Register functions to be used in CL_HEAP_CUSTOM_MODE
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \param allocHook (in) Function that allocates memory. This function is 
 *   invoked when an application calls clHeapAllocate() function.
 *  \param reallocHook (in) Function that changes the size of the memory. 
 *   This function is invoked when an application calls clHeapRealloc().
 *  \param callocHook (in) Function that allocates memory for an array.
 *   This function is invoked when an application calls clHeapCalloc().
 *  \param freeHook (in) Function that frees the memory. 
 *   This function is invoked when an application calls clHeapFree(). 
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_INITIALIZED Heap library is not initialized or it is 
 *   finalized.
 *  \retval CL_ERR_NULL_POINTER One of the input argument is a NULL pointer.
 *
 *  \par Description:
 *  In CL_HEAP_CUSTOM_MODE, this function is used to register the hooks 
 *  for memory management. The application should override the default
 *  implementation of clHeapLibCustomInitialize() and
 *  clHeapLibCustomFinalize() functions present in
 *  <em> ASP/models/<model-name>/config/clHeapCustom.c</em>. This
 *  function should be called from clHeapLibCustomInitialize(). 
 *  In CL_HEAP_CUSTOM_MODE, no memory allocation should be made before 
 *  registering these functions.
 *  \par
 *  During the life of a process, two different set of hooks for memory
 *  allocation should not be used. Unless due care is taken, memory
 *  allocated by one set of hooks cannot be freed using the other set
 *  of hooks as it leads to corruption of meta data structures.
 *
 *  \par Library File:
 *   libClUtils
 *
 *  \sa clHeapHooksDeregister()
 *
 */

ClRcT clHeapHooksRegister(CL_IN ClPtrT (*allocHook) (ClUint32T),
                     CL_IN ClPtrT (*reallocHook)(ClPtrT ,ClUint32T),
                     CL_IN ClPtrT (*callocHook)(ClUint32T,ClUint32T),
                     CL_IN void (*freeHook)(ClPtrT )
                     );
/*****************************************************************************/

/**
 ************************************
 *  \brief De-registers the hooks registered for CL_HEAP_CUSTOM_MODE
 *
 *  \par Header File:
 *   clHeapApi.h
 *
 *  \par Parameters:
 *   None
 * 
 *  \retval CL_OK Function completed successfully.
 *  \retval CL_ERR_INITIALIZED Heap library is not finalized through a previous
 *   call to the clHeapLibFinalize() function.
 *  \retval CL_ERR_NOT_INITIALIZED Hooks were not registered through a previous
 *   call to clHeapHooksRegister() function.
 *
 *  \par Description:
 *  This function is used to De-register the hooks registered for memory 
 *  management in CL_HEAP_CUSTOM_MODE. After a call to this function, 
 *  the memory related calls cannot be made until another call is made to 
 *  clHeapHooksRegister(). This function should be called during the 
 *  finalization of clHeapLibFinalize() through a call to open function 
 *  clHeapLibCustomFinalize() present in 
 *  <em> ASP/models/<model-name>/config/clHeapCustom.c</em>.
 *  Detailed desciption 
 *
 *  \par Library File:
 *   libClUtils
 *
 *  \sa clHeapHooksRegister()
 *
 */

ClRcT clHeapHooksDeregister(void);
/*****************************************************************************/

#endif

#ifdef __cplusplus
}
#endif

#endif

/** \} */
