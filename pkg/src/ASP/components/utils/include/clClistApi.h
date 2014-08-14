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
 * File        : clClistApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains essential definitions for the Circular Linked List
 * implementation.
 *
 *
 *****************************************************************************/


/********************************************************************************/
/******************************** Clist APIs ************************************/
/********************************************************************************/
/*                                                                              */
/*  clClistCreate			                                                    */
/*  clClistFirstNodeAdd		                                                    */
/*  clClistLastNodeAdd		                                                    */
/*  clClistAfterNodeAdd		                                                    */
/*  clClistBeforeNodeAdd	                                                    */
/*  clClistNodeDelete		                                                    */
/*  clClistFirstNodeGet		                                                    */
/*  clClistLastNodeGet		                                                    */
/*  clClistNextNodeGet		                                                    */
/*  clClistPreviousNodeGet	                                                    */
/*  clClistWalk			                                                        */
/*  clClistDataGet			                                                    */
/*  clClistSizeGet			                                                    */
/*  clClistDelete			                                                    */
/*                                                                              */
/********************************************************************************/


/**
 * \file 
 * \brief Header file of Circular List Management related APIs
 * \ingroup clist_apis
 */

/**
 * \addtogroup clist_apis
 * \{
 */

#ifndef _CL_CLIST_API_H_
#define _CL_CLIST_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>


/******************************************************************************
 *  Data Types
 *****************************************************************************/
/**
 *  The type of the handle for the circular list.
 */
typedef ClPtrT ClClistT;

/**
 * The type of the handle for the circular node.
 */
typedef ClPtrT ClClistNodeT;

/**
 * The type of the handle for the user-data.
 */
typedef ClPtrT ClClistDataT;

/**
 * link list with x nodes. It decides the action to be taken when the linked list is full.
 * The values of the ClClistDropPolicyT enumeration type have the following interpretation:
 */
typedef enum
{
/**
 * If the first node not to be dropped even if the list is full.
 */
    CL_NO_DROP=0,
/**
 * Drops the first node if the list is full.
 */
	CL_DROP_FIRST,
/**
 * Adds a new drop policy types before this.
 */
    CL_DROP_MAX_TYPE
}ClClistDropPolicyT;

/**
 ************************************
 *  \brief Delete callback gets called, whenever a Node is getting deleted. 
 *
 *  \par Header File: 
 *   clClistApi.h 
 *
 *  \param userData (in) Data of the node is being deleted. 
 *
 *  \retval none 
 *  
 *  \par Description:
 *   This dequeue callback function gets called, whenever user performs node
 *   deletion on the Circular list. The Data of the node will be exposed to the user
 *   on the callback. This is the place where user can cleanup their data. 
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate(), clQueueDelete() 
 *     
 */
 typedef void  (*ClClistDeleteCallbackT)(CL_IN ClClistDataT userData);

/**
 ************************************
 *  \brief Walk Callback gets called, whenever traverse happens on the
 *  Circular linked list. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param userData (in) Data of the node is being accessed. 
 *  \param userArg (in) User arg of the callback function
 *
 *  \retval none 
 *  
 *  \par Description:
 *   This Walk callback function gets called, whenever user performs traverse
 *   on the Circular Linked List. User can pass any cookie variable as
 *   UserArgument, that will be passed as part of the callback function.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clQueueCreate(), clQueueDelete() 
 *     
 */
typedef ClRcT  (*ClClistWalkCallbackT)(CL_IN ClClistDataT userData,
                                       CL_IN void* userArg);


/************************************/
/**
 ************************************
 *  \brief Creates a Circular Linked List.
 *
 *  \par Header File 
 *   clClistApi.h
 *
 *  \param maxSize (in) Maximum size of the list. It specifies the maximum number of nodes
 *  that can exist at any point of time in the list. This must be an unsigned integer. You can
 *  add nodes into the list until this maximum limit is reached. If you specify this parameter
 *  as 0, then, there is no limit  on the size of the list.
 *
 *  \param dropPolicy (in) Drop policy for the list. Whenever you try to add a
 *  node in the list and the list is full, if this policy is set to  \c CL_DROP_FIRST,
 *  the first node is dropped and the specified node is added. If this parameter
 *  is set to  \c CL_NO_DROP, then an error is returned. This parameter is valid,
 *  only if the list is of fixed size.
 *
 *  \param fpUserDeleteCallBack (in) Pointer to the delete callback function of the user.
 *  This function accepts a parameter of type \e ClClistDataT.
 *  After deleting the specified node, the user data stored in that node is
 *  passed as an argument to the call back function.
 *
 *  \param fpUserDestroyCallBacK (in) Pointer to the user's destroy callback function.
 *  This function accepts a parameter of type \e ClClistDataT.
 *  When destroying, the user data stored in each node is
 *  passed as an argument to the call back function, one by one.
 *
 *  \param pListHead (out) Pointer to the variable of type
 *  ClClistT in which the function returns a
 *  valid list handle on successful creation of the list.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API creates and initializes the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete()
 *
 */

extern ClRcT clClistCreate(CL_IN  ClUint32T                maxSize,
                           CL_IN  ClClistDropPolicyT       dropPolicy,
                           CL_IN  ClClistDeleteCallbackT   fpUserDeleteCallBack,
                           CL_IN  ClClistDeleteCallbackT   fpUserDestroyCallBack,
                           CL_OUT ClClistT                 *pListHead);

/**
 ************************************
 *  \brief Adds a node at the beginning of the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param userData (in) User-data. Memory allocation and de-allocation for user-data must be done by you.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_MAXSIZE_REACHED If the maximum size is reached.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API adds a node at the beginning of the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistCreate(), clClistDelete()
 *
 */

extern ClRcT clClistFirstNodeAdd(CL_IN ClClistT      listHead,
                                 CL_IN ClClistDataT  userData);

/**
 ************************************
 *  \brief Adds a node at the end of the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param userData (in) User-data. Memory allocation and de-allocation for user-data must be done by you.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_MAXSIZE_REACHED If the maximum size is reached.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API adds a node at the end of the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeAdd()
 *
 */

extern ClRcT clClistLastNodeAdd(CL_IN ClClistT     listHead,
                                CL_IN ClClistDataT userData);

/**
 ************************************
 *  \brief Adds a node after a specified node in the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param currentNode (in) Handle of node after which the specified data must be added.
 *  \param userData (in) User-data. Memory allocation and de-allocation for user-data must be done by you.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_MAXSIZE_REACHED If the maximum size is reached.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API adds a node after a specified node in the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistLastNodeAdd(), 
 *      clClistFirstNodeAdd()
 *
 */

extern ClRcT clClistAfterNodeAdd(CL_IN ClClistT      listHead,
                                 CL_IN ClClistNodeT  currentNode,
                                 CL_IN ClClistDataT  userData);
/**
 ************************************
 *  \brief Adds a node before a specified node in the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param currentNode (in) Handle of node before which the specified data must be added.
 *  \param userData (in) User-data. Memory allocation and de-allocation for user-data must be done by you.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_MAXSIZE_REACHED If the maximum size is reached.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API adds a node before a specified node in the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistNextNodeGet()
 *      clClistFirstNodeAdd(), clClistLastNodeAdd()
 *
 */

extern ClRcT clClistBeforeNodeAdd(CL_IN ClClistT      listHead,
                                  CL_IN ClClistNodeT  currentNode,
                                  CL_IN ClClistDataT  userData);

/**
 ************************************
 *  \brief Deletes a node from the list
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param node (in) Handle of node which is to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API deletes the specified node from the Circular Linked list.
 *  The delete callback function, registered during the creation is called with
 *  the data in the node to be deleted.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete()
 *
 */
extern ClRcT clClistNodeDelete(CL_IN ClClistT      listHead,
                               CL_IN ClClistNodeT  node);
/**
 ************************************
 *  \brief Returns the first node from the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param pFirstNode (out) Pointer to variable of type \e ClClistNodeT.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_EXIST If the list is empty (an acceptable way to check
 *                           for length 0 lists).
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API returns the first node from the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete()
 *
 */

extern ClRcT clClistFirstNodeGet(CL_IN ClClistT       listHead,
                                 CL_IN ClClistNodeT*  pFirstNode);
/**
 ************************************
 *  \brief Returns last node from the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param pLastNode (out) Pointer to variable of type \e ClClistNodeT.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_EXIST If the list is empty.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to return the last node from the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistLastNodeGet(CL_IN  ClClistT     listHead,
                                CL_OUT ClClistNodeT *pLastNode);

/**
 ************************************
 *  \brief Returns next node from the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param currentNode (in) Handle of the current node.
 *  \param pNextNode (out) Pointer to variable of type \e ClClistNodeT.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_EXIST If the list is empty.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to return the next node of the specified node from the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistNextNodeGet(CL_IN  ClClistT       listHead,
                                CL_IN  ClClistNodeT   currentNode,
                                CL_OUT ClClistNodeT*  pNextNode);
/**
 ************************************
 *  \brief Returns next node from the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param currentNode (in) Handle of the current node.
 *  \param pPreviousNode (out) Pointer to variable of type \e ClClistNodeT.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NOT_EXIST If the list is empty.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to return the previous node of the specified node from the Circular Linked list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistPreviousNodeGet(CL_IN  ClClistT       listHead,
                                    CL_IN  ClClistNodeT   currentNode,
                                    CL_OUT ClClistNodeT*  pPreviousNode);
/**
 ************************************
 *  \brief Walks through the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *
 *  \param fpUserWalkCallBack (in) Pointer to user callback function. It can have two values:
 *  \arg The first parameter must be of type ClClistDataT.
 *  \arg The second parameter must be of type void *. \n
 *  The user-data stored in each node is passed one-by-one as the
 *  first argument to the callback function.
 *
 *  \param userArg (in) User-specified argument. This variable is passed as the second argument to the callback function.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API performs a walk on the Circular Linked list. The user-specified
 *  callback function is called with every node's data.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistWalk(CL_IN ClClistT              listHead,
                         CL_IN ClClistWalkCallbackT  fpUserWalkCallBack,
                         CL_IN void*                 userArg);
/**
 ************************************
 *  \brief Retrieves data from a node in the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param node (in) Handle of the node.
 *  \param pUserData (out) Pointer to variable of type \e ClClistDataT.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API retrieves data from the specified node in the list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistDataGet(CL_IN  ClClistT      listHead,
                            CL_IN  ClClistNodeT  node,
                            CL_OUT ClClistDataT  *pUserData);
/**
 ************************************
 *  \brief Returns number of data elements (nodes) in the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param listHead (in) Handle of list returned by the create API.
 *  \param pSize (out) Pointer to variable of type \e ClUint32T.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API is used to return the number of data elements (nodes) in the list.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistSizeGet(CL_IN  ClClistT  listHead,
                            CL_OUT ClUint32T *pSize);
/**
 ************************************
 *  \brief Destroys the list.
 *
 *  \par Header File: 
 *   clClistApi.h
 *
 *  \param pListHead (in) Handle of list returned by the create API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \note
 *  Returned error is a combination of the Component Identifier and Error Code.
 *  Here as Return values only the Error Codes are listed. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to get the error code.
 *
 *  \par Description:
 *  This API deletes all the nodes in the list.
 *  The destroy callback function, registered during the creation, is called for
 *  every node in the list with the corresponding data.
 *
 *  \par Library File:
 *   ClUtil 
 *
 *  \sa clClistDelete(), clClistCreate(), clClistFirstNodeGet(), 
 *      clClistLastNodeGet()
 *
 */

extern ClRcT clClistDelete(CL_IN ClClistT* pListHead);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CLIST_API_H_ */


/** \} */
