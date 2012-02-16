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
 * ModuleName  : utils
 * File        : clHandleApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is the interface to a client side handle management service.
 *
 * Part of this code was inspired by the openais code, hence I retained
 * the copyright below.
 *
 *
 *****************************************************************************/

/*
 * Copyright (c) 2002-2003 MontaVista Software, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file 
 * \brief Header file of Handle Management related APIs
 * \ingroup handle_apis
 */

/**
 * \addtogroup handle_apis
 * \{
 */

#ifndef _CL_HANDLE_API_H_
#define _CL_HANDLE_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommon.h>

/**
 * Defines
 */

#define CL_HANDLE_INVALID_VALUE     0x0
#define CL_HDL_IDX_MASK     0x00000000FFFFFFFFULL
#define CL_HDL_IDX(hdl) (ClUint32T)( (hdl) & CL_HDL_IDX_MASK)
/**
 *  Handle database handle.
 */
typedef ClPtrT  ClHandleDatabaseHandleT;

/**
 ******************************************************************************
 *  \brief Creates a handle database.
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param destructor (in) An optional pointer to a destructor function.  Called
 *  when a handle is to be destroyed with its associated data.  If the data
 *  is in one monolithic dynamically allocated block, the destructor is not
 *  needed.  If the data has pointers pointing to separately allocated
 *  memory blocks, the destructrore should be provided and should be
 *  responsible to clean-up the auxiliary data.
 *
 *  \param databaseHandle (out) A pointer to get a unique handle back that can
 *  be used in subsequent calls to the handle database.
 *  
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_NO_MEMORY Memory allocation failure.
 *  
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API creates and initializes a handle database.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 *     
 */
extern ClRcT clHandleDatabaseCreate(
        CL_IN    void                    (*destructor)(void*),
        CL_OUT   ClHandleDatabaseHandleT  *databaseHandle);

/**
 ******************************************************************************
 *  \brief Destroys a handle database.
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param databaseHandle (in) The handle database handle.
 *
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_INVALID_HANDLE Invalid handle.
 *  \retval CL_ERR_MUTEX_ERROR Error in securing mutex on the database.
 *  
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API destroys and frees all data associated with a handle database.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 *     
 */
extern ClRcT clHandleDatabaseDestroy(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle);

/**
 ******************************************************************************
 *  \brief Creates a handle
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param databaseHandle (in) The handle database handle.
 *
 *  \param instanceSize (in) Size of memory to be allocated.
 *
 *  \param handle (out) Pointer to return the unique handle.
 *  
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_INVALID_HANDLE Invalid handle database handle.
 *  \retval CL_ERR_MUTEX_ERROR Error in securing mutex on the database.
 *  \retval CL_ERR_NO_RESOURCE The database is already marked for destroy
 *  (perhaps on another thread), so it is not allowed to create new handles.
 *  \retval CL_ERR_NO_MEMORY Could not allocated memory for handle or data
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API creates a new handle, unique within the context of the given
 *  handle database.  It also allocates memory of given size for data that
 *  the application can use to store any data associated with this handle.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 */
extern ClRcT clHandleCreate(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClInt32T                  instanceSize,
	    CL_OUT   ClHandleT                *handle);

/**
 ******************************************************************************
 *  \brief Destroy a handle
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param databaseHandle (in) The handle database handle.
 *
 *  \param handle (in) The handle.
 *  
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_INVALID_HANDLE Invalid handle database handle or handle.
 *  \retval CL_ERR_MUTEX_ERROR Error in securing mutex on the database.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API destroys (recycles) a given handle, releasing not only the
 *  handle, but also the data (memory) allocated in association with the
 *  handle.  If a desructor function was provided at the time when the
 *  database was created, the destructor function is called before the
 *  handle data is freed.
 *
 *  If the handle is checked out by the application when this API is called,
 *  then the destruction of the handle will be delayed until no further
 *  checkouts are pending.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 *
 */
extern ClRcT clHandleDestroy(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClHandleT                 handle);

/**
 ******************************************************************************
 *  \brief Check out a handle
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param databaseHandle (in) The handle database handle.
 *
 *  \param handle (in) The handle.
 *  
 *  \param instance (out)  A pointer to return the pointer to the data.
 *
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_INVALID_HANDLE Invalid handle database handle, or handle.
 *  \retval CL_ERR_MUTEX_ERROR Error in securing mutex on the database.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API returns with a pointer that point to the data associated
 *  with the handle and increments the check-out counter of the handle.
 *  An application should call this function before making modifications
 *  to the handle data.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 *     
 */
extern ClRcT clHandleCheckout(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClHandleT                 handle,
        CL_OUT   void                    **instance);

/**
 ******************************************************************************
 *  \brief Check in a handle
 *
 *  \par Header File 
 *   clHandleApi.h
 *
 *  \param databaseHandle (in) The handle database handle.
 *
 *  \param handle (in) The handle.
 *  
 *  \retval CL_OK The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_INVALID_HANDLE Invalid handle database handle, or handle.
 *  \retval CL_ERR_MUTEX_ERROR Error in securing mutex on the database.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This API signals the handle database the application is no longer modifying
 *  the data assoicated with the handle.  Each call to this API negates the
 *  effect of exactly one call to the clHandleCheckout() API.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clHandleCreate, clHandleCheckout(), clHandleDestroy() 
 *     
 */
extern ClRcT clHandleCheckin(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClHandleT                 handle);

extern ClRcT clHandleMove(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClHandleT                 oldHandle,
        CL_IN    ClHandleT                 newHandle);

#ifdef __cplusplus
}
#endif

#endif /* _CL_HANDLE_API_H_ */


/** \} */
