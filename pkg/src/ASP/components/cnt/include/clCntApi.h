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
 * ModuleName  : cnt
 * File        : clCntApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Clovis Container Library
 *
 *
 *****************************************************************************/


/**********************************************************************************/
/****************************** CONTAINER APIs ************************************/
/**********************************************************************************/
/*                                                                                */
/* clCntLlistCreate                                                               */
/* clCntHashtblCreate                                                             */
/* clCntRbtreeCreate                                                              */
/* clCntNodeAddAndNodeGet                                                         */
/* clCntNodeAdd                                                                   */
/* clCntAllNodesDelete                                                            */
/* clCntAllNodesForKeyDelete                                                      */
/* clCntNodeDelete	                                                              */
/* clCntNodeFind                                                                  */
/* clCntFirstNodeGet                                                              */
/* clCntLastNodeGet                                                               */
/* clCntNextNodeGet	                                                              */
/* clCntPreviousNodeGet                                                           */
/* clCntWalk                                                                      */
/* clCntNodeUserKeyGet                                                            */
/* clCntDataForKeyGet                                                             */
/* clCntNodeUserDataGet                                                           */
/* clCntSizeGet	                                                                  */
/* clCntKeySizeGet                                                                */
/* clCntDelete                                                                    */
/* clCntiThreadSafeLlistCreate                                                    */
/*                                                                                */
/**********************************************************************************/

/**
 *  \file
 *  \brief Header file of Clovis Container Related APIs
 *  \ingroup cnt_apis
 */

/**
 *  \addtogroup cnt_apis
 *  \{
 */
 
#ifndef _CL_CNT_H_
#define _CL_CNT_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <clCommon.h>
#include <clRuleApi.h>

/******************************************************************************
 *  Data Types 
 *****************************************************************************/
 /**
  * Handle of the container. Container could be Linked list, Rbtree,
  * Hashtable.
  */
  typedef ClPtrT    ClCntHandleT;
 
 /**
  * Handle of the container Node.
  */
  typedef ClPtrT    ClCntNodeHandleT;
 /**
  * Handle of the data.
  */
  typedef ClPtrT    ClCntDataHandleT;
 /**
  * Handle of the argument which will be passed to callback functions
  */
  typedef ClPtrT    ClCntArgHandleT;
 /**
  * Handle of the key handle.
  */
  typedef ClPtrT    ClCntKeyHandleT;

  /**
   * This enum describes type of the container.
   */
    typedef enum
    {
        /**
         * Container contains unique key. Duplicate entries are not allowed.
         */
        CL_CNT_UNIQUE_KEY        = 1,
        /**
         * Container contains non unique key. Duplicate entries are allowed. 
         */
        CL_CNT_NON_UNIQUE_KEY    = 2,
        /**
         * Invalid container type.
         */
        CL_CNT_INVALID_KEY_TYPE
    } ClCntKeyTypeT;

/**
 ************************************
 *  \brief Compares the two keys 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param key1 (in) Users passed Key value. 
 *  \param key2 (in) already stored Key value in the container. 
 *
 *  \retval 0 both the keys are equal 
 *  \retval >0 key1 is larger than key2 
 *  \retval <0 key2 is larger than key1
 *  
 *  \par Description:
 *   This compare callback function does the comparison two keys and returns
 *   the result back to the calling function. This will get called, whenever
 *   the container does any key related operations.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet()
 */
typedef ClInt32T  (*ClCntKeyCompareCallbackT)(CL_IN ClCntKeyHandleT key1,
                                              CL_IN ClCntKeyHandleT key2);
/**
 ************************************
 *  \brief Gets called While destroying the node of the container 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param userKey (in) Key of the node is being destroyed.
 *  \param userData (in) Data of the node is being destroyed. 
 *
 *  \retval none
 *  
 *  \par Description:
 *   This delete callback function will get invoked whenever a node is getting
 *   deleted. The user can do their cleanup here.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet()
 */
typedef void (*ClCntDeleteCallbackT)(CL_IN ClCntKeyHandleT userKey,
                                     CL_IN ClCntDataHandleT userData);
/**
 ************************************
 *  \brief Gets called While doing hash table related operations. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param userKey (in) Key of the node is being accessed. 
 *
 *  \retval hashVal will be returned. 
 *  
 *  \par Description:
 *   Whenever any hash table related operations is being performed, this hash
 *   callback will be invoked.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet()
 */
 typedef ClUint32T (*ClCntHashCallbackT)(CL_IN ClCntKeyHandleT userKey);

/**
 ************************************
 *  \brief Gets called While performing container walk operation. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param userKey (in)  Key of the node is being accessed. 
 *  \param userData (in) Data of the node is being accessed.
 *  \param userArg (in)  Userarg whatever they pass to clCntWalk function will
 *  passed.
 *  \param length  (in)  Length of the arg data. 
 *
 *  \retval hashVal will be returned. 
 *  
 *  \par Description:
 *   Whenever a container Walk is performed on any container, this call back
 *   will get invoked. 
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 */
 typedef ClRcT   (*ClCntWalkCallbackT)(CL_IN ClCntKeyHandleT  userKey,
                                       CL_IN ClCntDataHandleT userData, 
                                       CL_IN ClCntArgHandleT  userArg,
                                       CL_IN ClUint32T dataLength);

/**
 ************************************
 *  \brief  Creates the container doubly linked list.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in):  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create a linked list of a Container. It populates all
 *  the functions related with linked list. 
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */

extern ClRcT clCntLlistCreate(CL_IN  ClCntKeyCompareCallbackT fpKeyCompare,
                              CL_IN  ClCntDeleteCallbackT  fpUserDeleteCallback,
                              CL_IN  ClCntDeleteCallbackT  fpUserDestroyCallback,
                              CL_IN  ClCntKeyTypeT         containerKeyType,
                              CL_OUT ClCntHandleT*         pContainerHandle);
/**
 ************************************
 *  \brief  Creates the container doubly linked list with lock variable.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in):  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_MUTEX_ERROR On passing invalid values for creationg mutex. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create a linked list of a Container. It populates all
 *  the functions related with linked list. It provides thread safe container
 *  to the user. At a time only one anylement can be access the container
 *  element.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */
extern ClRcT clCntThreadSafeLlistCreate(CL_IN  ClCntKeyCompareCallbackT fpKeyCompare,
                                        CL_IN  ClCntDeleteCallbackT fpUserDeleteCallback,
                                        CL_IN  ClCntDeleteCallbackT fpUserDestroyCallback,
                                        CL_IN  ClCntKeyTypeT containerKeyType,
                                        CL_OUT ClCntHandleT* pContainerHandle);

extern ClRcT 
clCntOrderedLlistCreate(
        ClCntKeyCompareCallbackT fpKeyCompare,
        ClCntDeleteCallbackT fpUserDeleteCallback,
        ClCntDeleteCallbackT fpUserDestroyCallback,
        ClCntKeyTypeT containerKeyType,
        ClCntHandleT* pContainerHandle);

/**
 ************************************
 *  \brief  Creates the hash table. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param  numberOfBuckets  Number of buckets (table size) in the hash table.
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in)  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_MUTEX_ERROR On passing invalid values for creationg mutex. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create a hash table Container with Numbuckets. It populates all
 *  the function pointers related with hash table. 
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */

extern ClRcT
clCntHashtblCreate(CL_IN ClUint32T numberOfBuckets,
                   CL_IN ClCntKeyCompareCallbackT fpKeyCompare,
                   CL_IN ClCntHashCallbackT fpHashFunction,
                   CL_IN ClCntDeleteCallbackT fpUserDeleteCallback,
                   CL_IN ClCntDeleteCallbackT fpUserDestroyCallback,
                   CL_IN ClCntKeyTypeT containerKeyType,
                   CL_IN ClCntHandleT* pContainerHandle);

/**
 ************************************
 *  \brief  Creates a thread safe hash table. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param  numberOfBuckets  Number of buckets (table size) in the hash table.
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in)  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_MUTEX_ERROR On passing invalid values for creationg mutex. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create a hash table Container with Numbuckets. It populates all
 *  the function pointers related with hash table. It provides thread safe
 *  container, so locking will be provided. 
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */
extern ClRcT
clCntThreadSafeHashtblCreate(CL_IN ClUint32T numberOfBuckets,
                             CL_IN ClCntKeyCompareCallbackT fpKeyCompare,
                             CL_IN ClCntHashCallbackT fpHashFunction,
                             CL_IN ClCntDeleteCallbackT fpUserDeleteCallback,
                             CL_IN ClCntDeleteCallbackT fpUserDestroyCallback,
                             CL_IN ClCntKeyTypeT containerKeyType,
                             CL_OUT ClCntHandleT* pContainerHandle);

/**
 ************************************
 *  \brief  Creates the container red black tree. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in):  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK: The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY: On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER: On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_INVALID_PARAMETER: On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_MUTEX_ERROR: On passing invalid values for creationg mutex. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create and initialize Red Black Tree Container that
 *  supports only unique keys.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */

extern ClRcT clCntRbtreeCreate(CL_IN  ClCntKeyCompareCallbackT fpKeyCompare,
                               CL_IN  ClCntDeleteCallbackT fpUserDeleteCallback,
                               CL_IN  ClCntDeleteCallbackT fpUserDestroyCallback,
                               CL_IN  ClCntKeyTypeT containerKeyType,
                               CL_OUT ClCntHandleT* pContainerHandle);
/**
 ************************************
 *  \brief  Creates the thread safe container red black tree. 
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param fpKeyCompare (in) Pointer to the user's key compare function. It accepts two 
 *  parameters of type \e ClCntKeyHandleT. This function returns the following values:
 *  \arg a negative value: If first key is lesser than the second key. 
 *  \arg zero: If both the keys are equal. 
 *  \arg a positive value: If first key is greater than second key. \n
 *  \b Note: \n
 *  For the APIs where a traversal of the nodes in the container is involved, you must pass a key for 
 *  a node to be found. This callback function is called by the Container library for every node in 
 *  the container with the key passed by you and the key for that node. The implementation of this 
 *  function must compare the keys in the application specific way and return a value as documented below.
 *
 *  \param fpUserDeleteCallback (in):  Pointer to the user's destroy callback function. This function is called 
 *  by the container library whenever either \e clCntNodeDelete or \e clCntNodeAllDelete API is called. The 
 *  delete callback function must free any memory allocated by the application for the node being deleted.
 *
 *  \param  fpUserDestroyCallback (in) Pointer to the user's destroy callback function.
 *  This function is called, whenever you invoke the destroy API. The 
 *  user-key and user-data of each node, which is being deleted is passed as first
 *  and second argument to this callback.
 *
 *  \param  containerKeyType (in)  Enum indicating whether the Container is of a unique
 *  key type or a non-unique key type.
 *
 *  \param  pContainerHandle (out) Pointer to the variable of type ClCntHandleT in
 *  which the function returns a valid Container handle on successful creation of Container.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NULL_POINTER On passing NULL values for either
 *  pContainerHandle or compare call back funtion. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_INVALID_PARAMETER On passing invalid values to container
 *  type. 
 *  \retval CL_ERR_MUTEX_ERROR On passing invalid values for creationg mutex. 
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to create and initialize Red Black Tree Container that
 *  supports only unique keys. It provides thread safe through mutex
 *  variables.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa none 
 */

extern ClRcT clCntThreadSafeRbtreeCreate(CL_IN  ClCntKeyCompareCallbackT fpKeyCompare,
                                         CL_IN  ClCntDeleteCallbackT fpUserDeleteCallback,
                                         CL_IN  ClCntDeleteCallbackT fpUserDestroyCallback,
                                         CL_IN  ClCntKeyTypeT containerKeyType,
                                         CL_OUT ClCntHandleT* pContainerHandle);
/**
 ************************************
 *  \brief Adds a new node to Container and returns the node handle.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.
 *
 *  \param userKey (in) Handle of the user key.
 *
 *  \param userData (in) User specified data. Memory allocation 
 *  for this parameter must be done by you.
 *
 *  \param pExp (in) An RBE Expression to associate with this new node.
 *
 *  \param pNodeHandle (out) Pointer to the variable of type ClCntNodeHandleT 
 *  in which handle of the node is added.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_DUPLICATE If user-key already exists and the Container created supports only unique keys.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to create and insert a new node into the Container as well as to return
 *  the handle of the newly created node.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *
 */
extern ClRcT clCntNodeAddAndNodeGet(CL_IN ClCntHandleT     containerHandle,
                                    CL_IN ClCntKeyHandleT  userKey,
                                    CL_IN ClCntDataHandleT userData,
                                    CL_IN ClRuleExprT      *pExp,
                                    CL_IN ClCntNodeHandleT *pNodeHandle);
/**
 ************************************
 *  \brief Adds a new node to Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.
 *  \param userKey (in) Handle of the user-key.
 *  \param userData (in) User specified data. Memory allocation 
 *   for \e userData must be done by you.
 *  \param rbeExpression (in)  An RBE expression to associate with the new node.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_DUPLICATE If user-key already exists and the Container created supports only unique keys.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to create and insert a new node into the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *
 */
extern ClRcT clCntNodeAdd(CL_IN ClCntHandleT     containerHandle,
                          CL_IN ClCntKeyHandleT  userKey,
                          CL_IN ClCntDataHandleT userData,
                          CL_IN ClRuleExprT      *rbeExpression);
/**
 ************************************
 *  \brief Deletes all the nodes from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.                 
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NOT_EXIST If node does not exist.
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *  
 *  \par Description:
 *  This API is used to delete all the nodes from the Container, but does not affect the user-data. 
 *  You are required to perform memory allocation and de-allocation for data.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *
 */

extern ClRcT clCntAllNodesDelete(CL_IN ClCntHandleT containerHandle);
/**
 ************************************
 *  \par Deletes all the nodes associated with specific key from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param  containerHandle (in) Handle of Container returned by the create API.
 *  \param  userKey (in) User specified key.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NOT_EXIST If node does not exist.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to delete all the nodes associated with the specific user key, but does not 
 *  affect the user-data. You are required to perform memory allocation and de-allocation for data.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet(), clCntNodeUserDataGet(), 
 *      clCntNodeUserKeyGet()
 *
 */

extern ClRcT clCntAllNodesForKeyDelete(CL_IN ClCntHandleT    containerHandle,
                                       CL_IN ClCntKeyHandleT userKey);
/**
 ************************************
 *  \brief Deletes a specific node from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.               
 *  \param nodeHandle (in) Handle of the node to be deleted.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NOT_EXIST If node does not exist.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to delete the specific node from the Container, but it does not affect the user data. 
 *  You are required to perform memory allocation and de-allocation for data.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet(), clCntNodeUserDataGet(), 
 *      clCntNodeUserKeyGet()
 *
 */

extern ClRcT clCntNodeDelete(CL_IN ClCntHandleT containerHandle,
                             CL_IN ClCntNodeHandleT nodeHandle);
/**
 ************************************
 *  \brief Finds a specific node in the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.
 *  \param userKey (in) User specified key.
 *  \param pNodeHandle (out) Pointer to the variable of type ClCntNodeHandleT 
 *  in which handle of the node is found.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *  \retval CL_ERR_NOT_EXIST If user-key does not exist.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to search for a specific node in the Container. It returns the node associated 
 *  with the specific key from the Container. If multiple nodes are associated with the same key,
 *  the node handle of first node associated with the key is returned.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntDataForKeyGet(), clCntNodeUserDataGet(), clCntNodeUserKeyGet()
 *
 */

extern ClRcT clCntNodeFind(CL_IN ClCntHandleT     containerHandle,
                           CL_IN ClCntKeyHandleT  userKey,
                           CL_IN ClCntNodeHandleT *pNodeHandle);
/**
 ************************************
 *  \brief Returns the first node from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.
 *  \param pNodeHandle (out) Pointer to the variable of type ClCntNodeHandleT 
 *  in which handle of the node is found.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to retrieve the first node from the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind() 
 *
 */
extern ClRcT  clCntFirstNodeGet(CL_IN  ClCntHandleT containerHandle,
                                CL_OUT ClCntNodeHandleT* pNodeHandle);
/**
 ************************************
 *  \brief Returns the last node from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in)  Handle of Container returned by the create API.
 *  \param currentNodeHandle (out)  Handle of the current node.
 *  \param pNextNodeHandle  (out) Pointer to the variable of type ClCntNodeHandleT 
 *   in which the handle of the next node is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to return the last node from the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind() 
 *
 */
extern ClRcT clCntLastNodeGet(CL_IN  ClCntHandleT     containerHandle,
                              CL_OUT ClCntNodeHandleT *pNodeHandle);
/**
 ************************************
 *  \brief Returns the next node from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of Container returned by the create API.
 *  \param currentNodeHandle (in) Handle of current node.
 *  \param pNextNodeHandle  (out) Pointer to the variable of type ClCntNodeHandleT 
 *   in which the handle of the next node is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to return the next node after a specific node in the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind() 
 *
 */

extern ClRcT clCntNextNodeGet(CL_IN  ClCntHandleT     containerHandle,
                              CL_IN  ClCntNodeHandleT currentNodeHandle,
                              CL_OUT ClCntNodeHandleT *pNextNodeHandle);
/**
 ************************************
 *  \brief Returns the previous node from the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API.
 *  \param currentNodeHandle (in) Handle of the current node.
 *  \param pPreviousNodeHandle (out) Pointer to the variable of type ClCntNodeHandleT 
 *  in which the handle of the previous node is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to return the node before a specific node in the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind() 
 *
 */

extern ClRcT clCntPreviousNodeGet(CL_IN  ClCntHandleT     containerHandle,
                                  CL_IN  ClCntNodeHandleT currentNodeHandle,
                                  CL_OUT ClCntNodeHandleT *pPreviousNodeHandle);
/**
 ************************************
 *  \brief Walks through the Container.
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API.
 *
 *  \param fpUserWalkCallback (in) Pointer to the user's walk callback function. 
 *  It accepts the following arguments: 
 *  \arg ClCntKeyHandleT   Handle to user-key of the node
 *  \arg ClCntDataHandleT  Handle to user-data of the node.
 *  The \e userKey and \e userData of each node are passed as first and second 
 *  arguments to the callback function. User specified \e userArg is also passed.
 *
 *  \param userArg (in)  User specified argument, which is passed as third parameter
 *  to the user walk callback function. This parameter is also passed to the
 *  \e clRbeExprEvaluate API. 
 *
 *  \param length (in) Length of \e userArg. This parameter is also passed to 
 *  clRbeExprEvaluate API.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to perform a walk through the Container. For each node encountered 
 *  during the walk, if the RBE associated with the node evaluates to: \n
<BR>
 *  \arg TRUE or NULL The user specific callback function is called with the key and data
 *   handles present in the encountered node. 
 *  \arg FALSE: The callback function is not called for the current node. The walk function
 *  continues with the next node.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *  
 */
extern ClRcT clCntWalk(CL_IN ClCntHandleT        containerHandle,
                       CL_IN ClCntWalkCallbackT  fpUserWalkCallback,
                       CL_IN ClCntArgHandleT     userArg,
                       CL_IN ClInt32T            length);

/**
 ************************************
 *  \brief Returns the user-key from the node.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API.
 *  \param nodeHandle (in) Handle of node from which user-key is to be retrieved.
 *  \param pUserKey (out) Pointer to the variable of type ClCntNodeHandleT 
 *  in which the handle of the user-key is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to retrieve the user-key from a specified node.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntDataForKeyGet(), clCntNodeUserDataGet() 
 *
 */
extern ClRcT clCntNodeUserKeyGet(CL_IN  ClCntHandleT     containerHandle,
                                 CL_IN  ClCntNodeHandleT nodeHandle,
                                 CL_OUT ClCntKeyHandleT  *pUserKey);
/**
 ************************************
 *  \brief Returns the user-data associated with a specified key.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in)  Handle of the Container returned by the create API.
 *  \param userKey (in) User-key, for which associated data is to be retrieved. 
 *  \param pUserData (out) Pointer to the variable of type \e ClCntNodeHandleT 
 *  in which the handle of the user-data associated with a specific key is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *  \retval CL_ERR_NOT_EXIST If user-key does not exist.
 *  \retval CL_ERR_NOT_IMPLEMENTED If this API is used with non-unique keys.
 *
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to retrieve the user-data associated with a specified key. It must
 *  be used only if the keys are unique. For non-unique keys, the API returns an error.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntNodeUserKeyGet(), clCntNodeUserDataGet() 
 *
 */
 
extern ClRcT clCntDataForKeyGet(CL_IN  ClCntHandleT     containerHandle,
                                CL_IN  ClCntKeyHandleT  userKey,
                                CL_OUT ClCntDataHandleT *pUserData);
/**
 ************************************
 *  \brief Returns the user-data from the node.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API.
 *  \param nodeHandle (in)  Handle of the node. 
 *  \param pUserDataHandle  (out) Pointer to the variable of type \e ClCntNodeHandleT 
 *  in which the handle of the user-data associated with a specific key is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to retrieve the user-data from a specific node.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate(),
 *      clCntNodeFind(), clCntNodeUserKeyGet(), clCntDataForKeyGet()
 *
 */
 
extern ClRcT clCntNodeUserDataGet(CL_IN  ClCntHandleT     containerHandle,
                                  CL_IN  ClCntNodeHandleT nodeHandle,
                                  CL_OUT ClCntDataHandleT *pUserDataHandle);
/**
 ************************************
 *  \brief Returns the size of the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API. 
 *  \param pSize (out) Pointer to the variable of type \e ClCntNodeHandleT 
 *  in which size of the Container is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to return the total number of nodes in the Container.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate() 
 *
 */

extern ClRcT clCntSizeGet(CL_IN  ClCntHandleT containerHandle,
                          CL_OUT ClUint32T    *pSize);
/**
 ************************************
 *  \brief Returns the number of nodes associated with a user-key.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle (in) Handle of the Container returned by the create API.
 *  \param userKey (in) Handle of the user-key.
 *  \param pSize (out) Pointer to the variable of type \e ClCntNodeHandleT 
 *  in which the number of nodes associated with the specified user-key is returned.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle or if the Container is empty.
 *
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to return the number of nodes associated with a
 *  specific user-key.
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate() 
 *
 */

extern ClRcT clCntKeySizeGet(CL_IN  ClCntHandleT containerHandle,
                             CL_IN  ClCntKeyHandleT userKey,
                             CL_OUT ClUint32T* pSize);
/**
 ************************************
 *  \brief Destroys the Container.
 *
 *  \par Header File: 
 *   clCntApi.h 
 *
 *  \param containerHandle(in) Handle of Container returned by the create API.
 *
 *  \retval CL_OK The API executed successfully. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid Container handle.
 *             
 *  \note
 *  Return value is a combination of the component id and error code. As Return 
 *  values only the Error Codes are listed above. 
 *  Use \c CL_GET_ERROR_CODE(RET_CODE) defined in clCommonErrors.h to retrieve the error code.  
 *
 *  \par Description:
 *  This API is used to delete all the nodes and destroy the Container 
 *  but it does not affect any user-data. 
 *
 *  \par Library File:
 *   ClCnt 
 *
 *  \sa clCntLlistCreate(), clCntHashtblCreate(), clCntRbtreeCreate() 
 *
 */
extern ClRcT clCntDelete(CL_IN ClCntHandleT containerHandle);
/*************************************************************/
/*
  Since the container doesnt have an empty equivalent, we live 
  with this for the time being:
*/
#define CNT_WALK_RC(cnt,rc) do { \
 ClRcT oldRc;\
 if(rc == CL_IOC_STOP_WALK) {\
   rc = CL_OK;\
 }\
 oldRc = rc;\
 if(oldRc != CL_OK) { \
   ClUint32T n = 0;\
   rc = clCntSizeGet((cnt),&n);\
   if(rc != CL_OK || n > 0) { \
     rc = oldRc;\
   } else { \
     rc = CL_OK;\
  }\
 }\
}while(0)

#ifdef __cplusplus
}
#endif

/* _CL_CNT_API_H_ */
#endif 

/** \} */

