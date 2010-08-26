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
/*******************************************************************************
 * ModuleName  : include
 * File        : clHandleIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
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

#ifndef _CL_HANDLE_IPI_H_
#define _CL_HANDLE_IPI_H_

# ifdef __cplusplus
extern "C"
{
# endif

typedef ClRcT (*ClHandleDbWalkCallbackT)(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, void *pCookie);

/**
 ******************************************************************************
 *  \par Synopsis:
 *
 *  Creates the specified handle
 *
 *  \par Description:
 *  This API creates the handle as specified by the user if not already used
 *  in the handle database.  It also allocates memory of given size for data 
 *  that the application can use to store any data associated with this handle.
 *
 *  \param databaseHandle: The handle database handle.
 *
 *  \param instanceSize: Size of memory to be allocated.
 *
 *  \param handle: The handle the user desires to use.
 *  
 *  \retval CL_OK: The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_ALREADY_EXIST: Specified handle is already in use.
 *  \retval CL_ERR_INVALID_HANDLE: Invalid handle database handle.
 *  \retval CL_ERR_MUTEX_ERROR: Error in securing mutex on the database.
 *  \retval CL_ERR_NO_RESOURCE: The database is already marked for destroy
 *  (perhaps on another thread), so it is not allowed to create new handles.
 *  \retval CL_ERR_NO_MEMORY: Could not allocated memory for handle or data
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 */
extern ClRcT clHandleCreateSpecifiedHandle(
        CL_IN    ClHandleDatabaseHandleT   databaseHandle,
	    CL_IN    ClInt32T                  instanceSize,
	    CL_IN    ClHandleT                 handle);

/**
 ******************************************************************************
 *  \par Synopsis:
 *
 *  Walks through the handle database
 *
 *  \par Description:
 *  This API traverses the handle database and executes the specified callback
 *  for each of the handles. The user needs to check out the handle before use.
 *
 *  \param databaseHandle: The handle database handle.
 *
 *  \param fpUserWalkCallback: The callback to be executed for each instance.
 *
 *  \param pCookie: User specified argument passed as is to the callback.
 *  
 *  \retval CL_OK: The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_NULL_POINTER: Specified NULL for the callback.
 *  \retval CL_ERR_INVALID_HANDLE: Invalid handle database handle.
 *  \retval CL_ERR_MUTEX_ERROR: Error in securing mutex on the database.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 */
extern ClRcT clHandleWalk (
        CL_IN   ClHandleDatabaseHandleT     databaseHandle,
	    CL_IN   ClHandleDbWalkCallbackT     fpUserWalkCallback,
        CL_IN   void                        *pCookie);

/**
 ******************************************************************************
 *  \par Synopsis:
 *
 *  Validates a Handle
 *
 *  \par Description:
 *  The API checks the validity of a handle thus obviating the need to use
 *  clHandleCheckout()/clHandleCheckin() API pair.
 *
 *  \param databaseHandle: The handle database handle.
 *
 *  \param handle: The handle that needs to be validated.
 *  
 *  \retval CL_OK: The API executed successfully, the returned handle is valid.
 *  \retval CL_ERR_NULL_POINTER: Specified NULL for database handle.
 *  \retval CL_ERR_INVALID_HANDLE: The handle passed is invalid.
 *  \retval CL_ERR_MUTEX_ERROR: Error in securing mutex on the database.
 *
 *  \note
 *  Returned error is a combination of the component id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 */
extern ClRcT clHandleValidate (
        CL_IN ClHandleDatabaseHandleT databaseHandle,
        CL_IN ClHandleT handle);

# ifdef __cplusplus
}
# endif

#endif /* _CL_HANDLE_IPI_H_ */

