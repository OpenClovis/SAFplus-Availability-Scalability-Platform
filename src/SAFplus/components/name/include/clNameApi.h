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
 * ModuleName  : name
 * File        : clNameApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains Name Service Client Library related
 * datastructures and APIs.
 *
 *
 *****************************************************************************/

/**
 *  \file
 *  \brief Header file of Name Service Related APIs
 *  \ingroup name_apis
 */

/**
 ************************************
 *  \addtogroup name_apis
 *  \{
 */

#ifndef _CL_NAME_API_H_
#define _CL_NAME_API_H_

#include "clBufferApi.h"
#include <clEoApi.h>
#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/******************************** Defines/constants **************************/
/*****************************************************************************/

/**
 * Maximum number of attributes per entry.
 */
#define CL_NS_MAX_NO_ATTR          5


/**
 * Maximum length of strings used in name service.
 */
#define CL_NS_MAX_STR_LENGTH       20

/**
 * Id of default global context.
 */
#define CL_NS_DEFT_GLOBAL_CONTEXT     0xFFFFFFF


/**
 * Id of default node local context.
 */
#define CL_NS_DEFT_LOCAL_CONTEXT      0xFFFFFFE

/**
 * Context map cookie for default global scope context.
 */
#define CL_NS_DEFT_GLOBAL_MAP_COOKIE  0xFFFFFFF

/**
 * Context map cookie for default local scope context.
 */
#define CL_NS_DEFT_LOCAL_MAP_COOKIE   0xFFFFFFE

/**
 * For getting service from highest priority service provider.
 */
#define CL_NS_DEFT_ATTR_LIST          0x0

/**
 * For getting service from highest priority service provider.
 */
#define CL_NS_GET_OBJ_REF             0xFFFFFFF

/**
 * Name Service event channel name.
 */
#define CL_NAME_PUB_CHANNEL  "NAME_SVC_PUB_CHANNEL"


/**
 * Id of the event published by Name Service.
 */
#define CL_NAME_EVENT_TYPE   0x100

/**
 * Latest Supported Version for Name Service.
 */
# define CL_NAME_VERSION {'B', 0x1, 0x1}
# define CL_NAME_VERSION_SET(version) (version).releaseCode = 'B', \
                                      (version).majorVersion = 0x1,\
                                      (version).minorVersion = 0x1

/********************* Name Service related data structures **********************/
/*********************************************************************************/

/**
 * Type of context to be created.
 */
typedef enum
{

/**
 * User defined node local context.
 */
  CL_NS_USER_NODELOCAL,

/**
 * User defined global context.
 */
    CL_NS_USER_GLOBAL,
}ClNameSvcContextT;


/**
 * Type of operation that invoked event publish.
 */
typedef enum
{
/**
 * Deregistration of a component on an API request.
 */
    CL_NS_COMPONENT_DEREGISTER_OP,

/**
 * Deregistration of service provided by a component.
 */
    CL_NS_SERVICE_DEREGISTER_OP,

 
/**
 * Deregister the entries based on EoID
 */
    CL_NS_EO_DEREGISTER_OP,

/**
 * Deregister the entries based on the component
 * death event
 */
    CL_NS_COMP_DEATH_DEREGISTER_OP,

/**
 * Deregister entries based on the node IOC address.
 */
    CL_NS_NODE_DEREGISTER_OP

}ClNameSvcOpT;


/**
 * Component Priority values.
 */
typedef enum
{
/**
 * Low priority.
 */
    CL_NS_PRIORITY_LOW,

/**
 * Medium priority.
 */
    CL_NS_PRIORITY_MEDIUM,

/**
 * High priority.
 */
    CL_NS_PRIORITY_HIGH,

}ClNameSvcPriorityT;


/**
 * Information passed to Name Service users via events whenever there is a
 * change in Name Service database.
 */
typedef struct
{

/**
 * Name of the serivce.
 */
 SaNameT       name;

/**
 * Object Reference.
 */
    ClUint64T    objReference;

/**
 * Operation type.
 */
    ClNameSvcOpT  operation;

/**
 * Context where service exists.
 */
    ClUint32T     contextMapCookie;
}ClNameSvcEventInfoT;


/**
 * Attribute structure.
 */
typedef struct
{

/**
 * Type field.
 */
ClUint8T        type[CL_NS_MAX_STR_LENGTH];

/**
 * Value field.
 */
ClUint8T        value[CL_NS_MAX_STR_LENGTH];
}ClNameSvcAttrEntryT;

typedef ClNameSvcAttrEntryT* ClNameSvcAttrEntryPtrT;


/**
 * For attribute related search.
 */
typedef struct
{

/**
 * List of attributes.
 */
    ClNameSvcAttrEntryT  attrList[CL_NS_MAX_NO_ATTR];

/**
 * Number of attributes.
 */
    ClUint32T          attrCount;

/**
 * List of opcodes.
 */
    ClUint32T          opCode[CL_NS_MAX_NO_ATTR-1];
}ClNameSvcAttrSearchT;

typedef ClNameSvcAttrSearchT * ClNameSvcAttrSearchPtrT ;


/**
 * The structure ClNameSvcRegisterT contains the name service registration information.
 */
typedef struct
{

/**
 * Name of the service.
 */
    SaNameT           name;

/**
 * Id of the Component.
 */
    ClUint32T         compId;

/**
 * Priority of the Component for the given service.
 */
    ClNameSvcPriorityT priority;

/**
 * Number of attributes associated with the entry.
 */
    ClUint32T         attrCount;

/**
 * List of attributes.
 */
    ClNameSvcAttrEntryT attr[1];
}ClNameSvcRegisterT;

typedef ClNameSvcRegisterT* ClNameSvcRegisterPtrT;


/**
 * List  of components.
 */
typedef struct ClNameSvcCompList
{
   ClUint32T                  dsId;
/**
 * Id of the Component.
 */
  ClUint32T                   compId;

/**
 * EoID of the component
 */
  ClEoIdT                    eoID;
  
/**
 * IOC address of the client
 */
  ClIocNodeAddressT         nodeAddress;
   
/**
 * Client IOC port
 */
  ClIocPortT                clientIocPort;
  
/**
 * Priority of the Component for the service being provided.
 */
  ClNameSvcPriorityT          priority;

/**
 * Pointer to the next element.
 */
    struct ClNameSvcCompList*   pNext;
}ClNameSvcCompListT;

typedef ClNameSvcCompListT* ClNameSvcCompListPtrT;


/**
 * Name Service Entry.
 */
typedef struct
{


/**
 * Name of the component.
 */
    SaNameT             name;

/**
 * Object Reference.
 */
    ClUint64T          objReference;

/**
 * Count of components providing the service.
 */
    ClUint32T           refCount;

/**
 * List of components providing the service.
 */
    ClNameSvcCompListT  compId;

/**
 * Number of attributes associated with the entry.
 */
    ClUint32T         attrCount;

/**
 * List of attributes.
 */
    ClNameSvcAttrEntryT attr[1];
}ClNameSvcEntryT;

typedef ClNameSvcEntryT* ClNameSvcEntryPtrT;

/*********************************************************************************/
/**************************    Client side APIs  *********************************/
/*********************************************************************************/

/*********************************************************************************/
/************************** API to Core Module ***********************************/

/**
 ************************************
 *  \brief Initializes the name service library
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param none
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_INITIALIZED Library already initialized
 *
 *  \par Description:
 *  This API is for initializing the name service library. It must be invoked
 *  before invoking any of the Name service APIs.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameLibFinalize()
 *
 */

ClRcT clNameLibInitialize();


/**
 ************************************
 *  \brief Finalizes the name service library
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param none
 *
 *  \retval CL_OK The API executed successfully.
 *
 *  \par Description:
 *  This API is for initializing the name service library. It must be invoked
 *  before invoking any of the Log service APIs.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameLibInitialize()
 *
 */

ClRcT clNameLibFinalize(void);


/**
 ************************************
 *  \brief Registers name to object reference mapping with Name Service.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \note
 *  For registering to user defined context, first the context must be
 *  created using clNameContextCreate API.
 *
 *  \param contextId Context in which the service will be provided.
 *   It can have the following values:
 *  \arg \c CL_NS_DEFT_GLOBAL_CONTEXT: for registering to default global context.
 *  \arg \c CL_NS_DEFT_LOCAL_CONTEXT: for registering to default node local context.
 *  \arg <em>Id returned by clNameContextCreate API</em>: for user-defined contexts.
 *
 *  \param pNSRegisInfo Registration related information
 *
 *  \param pObjReference This will carry the object reference. If object reference
 *  is known, pObjReference will carry the known value.
 *  If object reference is unknown, pObjReference SHOULD carry CL_NS_GET_OBJ_REF.
 *  The allocated object reference will be returned in pObjReference.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_NS_ERR_CONTEXT_NOT_CREATED On registering, deregistering or querying a context that does not exist.
 *  \retval CL_NS_ERR_LIMIT_EXCEEDED On creating or registering the contexts and entries more than the maximum allowed.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to register a name to object reference mapping with Name Service.
 *  In Clovis High Availability scenario, whenever a component is assigned as an ACTIVE HA
 *  state for a given service, it registers the same with Name Service.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameComponentRegister(), clNameServiceDeregister()
 */

ClRcT clNameRegister(CL_IN ClUint32T contextId,
                     CL_IN ClNameSvcRegisterT* pNSRegisInfo,
                     CL_INOUT ClUint64T *pObjReference);



/**
 ************************************
 *  \brief De-registers all entries of a component with Name Service.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param compId Id of the component.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to deregister entries of all the services being provided
 *  by a given component. In Clovis High Availability scenario, this is
 *  called by Component Manager whenever it detects that a component has died.
 *  This can also be called by a component when it does down gracefully.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameServiceDeregister(), clNameRegister()
 */

ClRcT clNameComponentDeregister(CL_IN ClUint32T compId);



/**
 ************************************
 *  \brief De-registers a particular service provided by a component.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param contextId Context in which the service will be provided.
 *   It can have the following values:
 *  \arg \c CL_NS_DEFT_GLOBAL_CONTEXT: for deregistering to default global context.
 *  \arg \c CL_NS_DEFT_LOCAL_CONTEXT: for deregistering to default node local context.
 *  \arg <em>Id returned by clNameContextCreate API</em>: for user-defined contexts.
 *  \param compId Id of the component.
 *  \param serviceName Name of the service
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_NS_ERR_CONTEXT_NOT_CREATED On registering, deregistering or querying a context that does not exist.
 *  \retval CL_NS_ERR_SERVICE_NOT_REGISTERED Trying to deregister an unregistered service.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is to deregister a service being provided by a given component.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameComponentDeregister(), clNameRegister()
 *
 */

ClRcT clNameServiceDeregister(CL_IN ClUint32T contextId,
                              CL_IN ClUint32T compId,
                              CL_IN SaNameT* serviceName);


/**
 ************************************
 *  \brief Creates a context.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param contextType Whether user-defined global or node local
 *  \param contextMapCookie This will be used during lookup. There is
 *   one-to-one mapping between \e contextMapCookie and \e contextId that
 *   is returned by this API. CL_NS_DEFT_GLOBAL_MAP_COOKIE(0xFFFFFFF) and
 *   CL_NS_DEFT_LOCAL_MAP_COOKIE(0xFFFFFFE) are RESERVED and should not
 *   be used.
 *  \param contextId (out) Id of the context created.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_NS_ERR_LIMIT_EXCEEDED On creating or registering the contexts
 *          and entries more than the maximum allowed.
 *  \retval CL_ERR_NO_MEMORY On memory allocation failure.
 *  \retval CL_ERR_INVALID_PARAMETER On passing an invalid parameter.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_NS_ERR_CONTEXT_ALREADY_CREATED On trying to cretae a context
 *          with a contextMapCookie which is already in use.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to create a user-defined context (both global scope and
 *  node local scope).
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameContextDelete()
 *
 */
ClRcT clNameContextCreate(CL_IN  ClNameSvcContextT contextType,
                          CL_IN  ClUint32T contextMapCookie,
                          CL_OUT ClUint32T *contextId);


/**
 ************************************
 *  \brief Deletes a context.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param contextId Context to be deleted.
 *
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_NS_ERR_CONTEXT_NOT_CREATED On registering, deregistering or
 *          querying a context that does not exist.
 *  \retval CL_NS_ERR_OPERATION_NOT_PERMITTED On trying to delete default
 *          contexts.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to delete a user-defined context (both global scope and
 *  node local scope).
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameContextCreate()
 *
 */
ClRcT clNameContextDelete(CL_IN ClUint32T contextId);

/**********************************************************************************/
/************************** API to Query Module ***********************************/


/**
 ************************************
 *  \brief Returns the object reference for a service.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param pName Name of the service.
 *
 *  \param attrCount Number of attributes being passed in the query. If the number of
 *   attributes is unknown, then this value can be passed as \c CL_NS_DEFT_ATTR_LIST
 *   In such case, the object reference of component with the highest
 *   priority will be returned.
 *
 *  \param pAttr List of attributes. If \c attrCount=CL_NS_DEFT_ATTR_LIST, then this
 *  parameter must be passed as NULL.
 *
 *  \param contextMapCookie Cookie to find the context. There is one-to-one mapping between
 *  \e contextMapCookie and \e context. This parameter can accept the following two values:
 *  \arg \c CL_NS_DEFT_GLOBAL_MAP_COOKIE: for querying the default global context.
 *  \arg \c CL_NS_DEFT_LOCAL_MAP_COOKIE: for querying the default local context.
 *  \param pObjReference (out) Object reference associated with the service.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_NS_ERR_CONTEXT_NOT_CREATED On registering, deregistering or querying a context that does not exist.
 *  \retval CL_NS_ERR_ENTRY_NOT_FOUND On querying an entry that is not present.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to query and retrieve the object reference for a given
 *  service name.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa saNameToObjectMappingGet()
 *
 */
ClRcT saNameToObjectReferenceGet(CL_IN  SaNameT* pName,
                                 CL_IN  ClUint32T attrCount,
                                 CL_IN  ClNameSvcAttrEntryT *pAttr,
                                 CL_IN  ClUint32T contextMapCookie,
                                 CL_OUT ClUint64T* pObjReference);


/**
 ************************************
 *  \brief Returns the entry of the service.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param pName Name of the service.
 *
 *  \param attrCount Number of attributes being passed in the query. If the number of
 *   attributes is unknown, then this value can be passed as \c CL_NS_DEFT_ATTR_LIST
 *   In such case, the object reference of component with the highest
 *   priority will be returned.
 *
 *  \param pAttr List of attributes. If \c attrCount=CL_NS_DEFT_ATTR_LIST, then this
 *  parameter must be passed as NULL.
 *
 *  \param contextMapCookie Cookie to find the context. There is one-to-one mapping between
 *  \e contextMapCookie and \e context. This parameter can accept the following two values:
 *  \arg \c CL_NS_DEFT_GLOBAL_MAP_COOKIE: for querying the default global context.
 *  \arg \c CL_NS_DEFT_LOCAL_MAP_COOKIE: for querying the default local context.
 *
 *  \param pOutBuff (out) Contains the entry. You must free the memory
 *  after successful execution of this API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_NS_ERR_CONTEXT_NOT_CREATED On registering, deregistering or querying a context that does not exist.
 *  \retval CL_NS_ERR_ENTRY_NOT_FOUND On querying an entry that is not present.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_NOT_INITIALIZED NS Library not initialized.
 *
 *  \par Description:
 *  This API is used to query and return the entire entry for a given service name.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa clNameObjectMappingCleanup(), saNameToObjectReferenceGet()
 */
ClRcT saNameToObjectMappingGet(CL_IN  SaNameT* pName,
                               CL_IN  ClUint32T attrCount,
                               CL_IN  ClNameSvcAttrEntryT *pAttr,
                               CL_IN  ClUint32T contextMapCookie,
                               CL_OUT ClNameSvcEntryPtrT* pOutBuff);


/**
 ************************************
 *  \brief Frees the object mapping entry.
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param pObjMapping Pointer to the object mapping being deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to clean up the object mapping returned during the lookup.
 *
 *  \par Library Files:
 *  libClNameClient
 *
 *  \sa saNameToObjectMappingGet()
 *
 */
ClRcT clNameObjectMappingCleanup(CL_IN ClNameSvcEntryPtrT pObjMapping);

/**
 ************************************
 *  \brief Verifies the version with the name service library
 *
 *  \par Header File:
 *  clNameApi.h
 *
 *  \param pVersion Version of NS Library with the user. If version mismatch
 *  happens then this will carry out the version supported by the NS Library.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is for verifing the version with name service library. It must be
 *  invoked after initializing the NS library and before invoking any of the
 *  Name service APIs.
 *
 *  \par Library Files:
 *  libClNameClient
 */

ClRcT clNameLibVersionVerify(CL_INOUT ClVersionT *pVersion);
/**************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _CL_NAME_API_H_ */

/**
 *  \}
 */

