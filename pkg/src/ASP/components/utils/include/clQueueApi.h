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
 * ModuleName  : utils
 * File        : clQueueApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * A simple queue implementation.
 *
 *
 *****************************************************************************/

/*************************************************************************/
/************************** QUEUE APIs ***********************************/
/*************************************************************************/
/*									                                     */
/* clQueueCreate					                                     */
/* clQueueNodeInsert				                                     */
/* clQueueNodeDelete			                                         */
/* clQueueWalk						                                     */
/* clQueueSizeGet						                                 */
/* clQueueDelete						                                 */
/*									                                     */
/*************************************************************************/

/**
 *  \file 
 *  \brief Header file of Queue Management related APIs
 *  \ingroup queue_apis
 */

/**
 ************************************
 *  \addtogroup queue_apis
 *  \{
 */

#ifndef _CL_QUEUE_API_H_
#define _CL_QUEUE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/**
 * The type of the handle for the queue.
 */
typedef ClPtrT ClQueueT;

/**
 * The type of the handle for the queue node.
 */
typedef ClHandleT ClQueueNodeT; // Mynk NTC

/**
 * The type of the handle for the user-data.
 */
typedef ClPtrT ClQueueDataT;


/**
 ************************************
 *  \brief Walk Callback gets called, whenever traverse happens on the Queue. 
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param userData (in) Data of the node is being accessed. 
 *  \param userArg (in) User arg of the callback function
 *
 *  \retval none 
 *  
 *  \par Description:
 *   This Walk callback function gets called, whenever user performs traverse
 *   on the Queue.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate(), clQueueDelete() 
 *     
 */
typedef void   (*ClQueueWalkCallbackT)(CL_IN ClQueueDataT userData,
                                       CL_IN void         *userArg);
/**
 ************************************
 *  \brief Dequeue callback gets called, whenever a Node is getting deleted. 
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param userData (in) Data of the node is being deleted. 
 *
 *  \retval none 
 *  
 *  \par Description:
 *   This dequeue callback function gets called, whenever user performs node
 *   deletion on the Queue. The Data of the node will be exposed to the user
 *   on the callback. This is the place where user can cleanup their data. 
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate(), clQueueDelete() 
 *     
 */
typedef void   (*ClQueueDequeueCallbackT)(CL_IN ClQueueDataT userData);

/**
 ************************************
 *  \brief Creates a queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param maxSize (in) Maximum size of the Queue. It
 *  specifies the maximum number of elements that can exist at
 *  any point of time in the Queue. This must be an unsigned integer.
 *  You can enqueue elements into the queue until this maximum limit is reached.
 *  If you specify this parameter as 0, then there is no limit
 *  on the size of the Queue.
 *
 *  \param fpUserDequeueCallBack (in) Pointer to the user's dequeue callback function.
 *  This function accepts a parameter of type \e ClQueueDataT.
 *  After dequeueing, the dequeued user-data is passed as an
 *  argument to the callback function.
 *
 *  \param fpUserDestroyCallBack (in) Pointer to the user's destroy callback function.
 *  This function accepts a parameter of type \e ClQueueDataT.
 *  While destroying the queue by clQueueDelete(), for each element in the queue this callback
 *  function is called.
 *
 *  \param pQueueHandle (out) Pointer to the variable of type
 *  \e ClQueueT in which the function returns a valid Queue handle.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \note
 *  Returned error is a combination of the component Id and error code.
 *  Use \c CL_GET_ERROR_CODE(RC) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to create and initialize a queue.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate() 
 *
 */
extern ClRcT clQueueCreate(CL_IN  ClUint32T                 maxSize,
                           CL_IN  ClQueueDequeueCallbackT   fpUserDequeueCallBack,
                           CL_IN  ClQueueDequeueCallbackT   fpUserDestroyCallBack,
                           CL_OUT ClQueueT                  *pQueueHandle);
/**
 ************************************
 *  \brief Enqueues an element (user-data) into the Queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param queueHandle (in) Handle of queue returned by \e clQueueCreate API.
 *  \param userData (in) User-data. Memory allocation and deallocation
 *  for user-data must be done by you. The data of the node to be stored in
 *  the queue. 
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_MAXSIZE_REACHED If the maximum size is reached.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the component Id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to enqueue an element (user-data) into the queue.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate() 
 *
 */
extern ClRcT clQueueNodeInsert(CL_IN ClQueueT     queueHandle,
                               CL_IN ClQueueDataT userData);
/**
 ************************************
 *  \brief Dequeues an element from the queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param queueHandle (in) Handle of queue returned by \e clQueueCreate API.
 *  \param userData (out) Handle of userData, userData of the dequeued node will be returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER On passing null pointer of userData.
 *  \retval CL_ERR_NOT_EXIST If the queue is empty.
 *
 *  \note
 *   Returned error is a combination of the component Id and error code.
 *   Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to dequeue the element from the front of the queue. The
 *  user dequeue callback, registered during the creation time, is called with the element (user-data)
 *  that is dequeued.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate() 
 *
 */
extern ClRcT clQueueNodeDelete(CL_IN  ClQueueT   queueHandle,
                               CL_OUT ClQueueDataT* userData);

/**
 ************************************
 *  \brief Walks through the queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param queueHandle (in) handle of queue returned by \e clQueueCreate Api.
 *
 *  \param fpUserWalkFunction (in) pointer to the callback function.
 *  it accepts the following two parameters:
 *  \arg ClQueueDataT
 *  \arg void * 
 *  each of elements in the queue is passed one by one as the
 *  first argument to the callback function.
 *
 *  \param userArg (in) user-specified argument. this variable is passed as the second
 *  argument to the user's call back function.
 *
 *  \retval CL_OK the api executed successfully.
 *  \retval CL_ERR_NULL_POINTER on passing a null pointer.
 *  \retval CL_ERR_INVALID_HANDLE on passing an invalid handle
 *
 *  \note
 *  returned error is a combination of the component id and error code.
 *  use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description
 *  This Api is used to perform a walk on the queue. The user-specified
 *  callback function is called with every element (user data) in the queue.
 *
 *  \par Library File
 *   ClUtil 
 *
 *  \sa clQueueCreate(), clQueueDelete() 
 *
 */
extern ClRcT clQueueWalk(CL_IN ClQueueT              queueHandle,
                         CL_IN ClQueueWalkCallbackT  fpUserWalkFunction,
                         CL_IN void*                 userArg);
/**
 ************************************
 *  \brief Retrieves the number of data elements in the queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param queueHandle (in) Handle of queue returned by the \e clQueueCreate API.
 *  \param pSize (out) Pointer to variable of type \e ClUint32T, in which the
 *   size of the queue is returned.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL value for callback function.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle
 *
 *  \note
 *  Returned error is a combination of the component Id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par description:
 *  This API is used to retrieve the number of data elements in the queue.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate() 
 *
 */
extern ClRcT clQueueSizeGet(CL_IN  ClQueueT   queueHandle,
                            CL_OUT ClUint32T  *pSize);

/**
 ************************************
 *  \brief Destroys the queue.
 *
 *  \par Header File: 
 *   clQueueApi.h 
 *
 *  \param pQueueHandle(in) Pointer to the queue handle returned by \e clQueueCreate API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle
 *
 *  \note
 *  Returned error is a combination of the component Id and error code.
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par description:
 *  This API is used to delete all the elements in the queue.
 *  The destroy callback function, registered during creation is called for
 *  every element in the queue.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate() 
 *
 */
extern ClRcT clQueueDelete(CL_IN ClQueueT* pQueueHandle);

#ifdef __cplusplus
}
#endif

#endif /* _CL_QUEUE_API_H_ */

/**
 * \}
 */
