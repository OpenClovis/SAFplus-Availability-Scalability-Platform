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
* ModuleName  : idl
* File        : clIdlApi.h
*******************************************************************************/

/*******************************************************************************
* Description :
    This module contains common IDL definitions
*******************************************************************************/

/*****************************************************************************/
/******************************** IDL APIs ***********************************/
/*****************************************************************************/
/* pageidl201 : clIdlHandleInitialize                        */
/* pageidl202 : clIdlHandleUpdate                        */
/* pageidl203 : clIdlVersionCheck                        */
/* pageidl204 : clIdlHandleFinalize                      */
/*****************************************************************************/

/**
 *  \defgroup group24 Interface Definition Language (IDL)
 *  \ingroup group4
 */

/**
 *  \file
 *  \ingroup group24
 */

/**
 * \name Interface Definition Language (IDL)
 */

/**
 *  \addtogroup group24
 *  \{
 */




#ifndef _CL_IDL_API_H_
#define _CL_IDL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clRmdApi.h>
#include <clNameApi.h>
#include <clHandleApi.h>
#include <clVersionApi.h>
#include <clXdrApi.h>
#include <clIdlErrors.h>

/**
 *  \page pageIDL Interface Definition Language (IDL) Library
 *
 *
 * \par Overview
 *
 *  OpenClovis ASP is a distributed system and in a typical deployment scenario,
 *  multiple nodes are expected to be part of the system. The different EOs
 *  on the various nodes interact with each other to provide the necessary
 *  services. Therefore, it becomes imperative to provide a simple and efficient
 *  means of communication across EOs. Such a mechanism for communication has
 *  to be agnostic to different platform specifics like endianness issues and
 *  native architecture data formats (32/64 bit architecture). \n
<BR>
 *  The solution to providing such a communication mechanism is to
 *  implement a message passing mechanism with RPC semantics. RPC is used to
 *  provide the ability to make function-calls across remote objects as if the
 *  functions existed in the local address space. Remote Method Dispatch (RMD)
 *  provides this functionality in OpenClovis ASP. \n
<BR>
 *  RMD consists of an engine which provides the ability to pass
 *  messages from source EO to a destination EO. The source EO from where the
 *  call originates is called the Client and the destination EO is called the
 *  Server. The RMD engine provides a generic raw API interface to pass and
 *  receive messages. The IDL generator takes EO interface and relevant
 *  user-defined data type definitions and provides server and client stubs
 *  which have an interface similar to that of a user-defined function call.
 *  IDL in combination with RMD is used to provide a simple and
 *  efficient means of communication across EOs. \n
<BR>
 *  The IDL Library provides the following APIs :
 *
 *  \arg Initialize the handle with RMD parameters.
 *  \arg Update the handle.
 *  \arg Finalize the handle.
 *
 *  When initializing the IDL library, you must specify a structure which
 *  contains the various RMD parameters, such as  RMD options, destination
 *  Address, and flags. This handle must be the first parameter of your function
 *  call. \n
 *
 *  If a component communicates with different components, it needs
 *  to change the destination address. The component can update
 *  the handle using clIdlUpdate(). \n
 *
 *  You can switch Containers without having to rewrite a significant section
 *  of their code, by using the Container abstraction. You only need to change
 *  the Container creation call appropriately. \n
 *
 *  \par Interaction with other components
 *  \arg IDL APIs depend on Operating System Abstraction Layer (OSAL) for memory allocation and free functions.
 *  \arg IDL depends on Handle API for creating and deleting handles.
**/

/************************ IDL PARAMETERS ************************************/

/**
 * IDL call can take two types of addresses - name service address and
 * IOC address.
 * This flag when passed through the \e addressType field in the \e
 * ClIdlAddressT implies that an IOC address is passed.
 */
#define CL_IDL_ADDRESSTYPE_IOC       (1)

/**
 * IDL call can take two types of addresses - name service address and
 * IOC address.
 * This flag when passed through the \e addressType field in the \e
 * ClIdlAddressT implies that a name service address is passed.
 */
#define CL_IDL_ADDRESSTYPE_NAME      (2)


/**
 * If you want to perform Atmost Once Semantics, you must set this bit in
 * the flag passed during handle initialization.
 */
#define CL_IDL_CALL_ATMOST_ONCE         (CL_RMD_CALL_ATMOST_ONCE)

/**
 * If you want to make a non-optimized RMD call, you must set this bit in
 * the flag passed during handle initialization.
 */
#define CL_IDL_CALL_DO_NOT_OPTIMIZE     (CL_RMD_CALL_DO_NOT_OPTIMIZE)

/**
 * If you want to use the services of the same EO instance as in previous call,
 * you must set this bit in the flag passed during handle initialization.
 */
#define CL_IDL_CALL_IN_SESSION          (CL_RMD_CALL_IN_SESSION)


/**
 * Adding the structure for name service address.
 */

typedef struct  ClIdlNameAddressT {

/**
 * This is the context cookie that Name Service expects to be passed.
 */
    ClUint32T      contextCookie;

/**
 * The name of the service requested.
 */
    ClNameT        name;

/**
 * The number of attributes passed.
 */
    ClUint32T             attrCount;

/**
 * The attributes.
 */
    ClNameSvcAttrEntryT   attr[CL_NS_MAX_NO_ATTR];

}ClIdlNameAddressT;

/**
 *  This is the union for IDL Address. IDL accepts either IOC address or
    a name service address.
 */
typedef union  ClIdlAddressTypeT
{

/**
 * IOC address.
 */
    ClIocAddressT      iocAddress;

/**
 * Name service address.
 */
    ClIdlNameAddressT  nameAddress;

}ClIdlAddressTypeT;

/**
 * IDL address structure.
 */
typedef struct ClIdlAddressT
{

/**
 * Type of address being passed. Its values can be:
 * \arg \c CL_IDL_ADDRESSTYPE_IOC: If IOC address is passed
 * \arg \c CL_IDL_ADDRESSTYPE_NAME: If name service address is passed.
 */
    ClUint32T          addressType;

/**
 * This is the address placeholder.
 */
    ClIdlAddressTypeT  address;

}ClIdlAddressT;

/**
 * The structure ClIdlHandleObj represents the IDL handle object. This object contains the details passed by the user during
 * clIdlHandleInitialize. This structure is used for generated code consumption.
 */
typedef struct ClIdlHandleObjT
{
    char*                    objId;  

/**
 * Address which specifies either NAME type or IOC type. This is the destination
 * address where the server side stub is accessed.
 */
    ClIdlAddressT            address;

/**
 * Flags that can be passed to IDL.
 */
    ClUint32T                flags;

/**
 * RMD options. The retries, timeout, and priority are maintained here.
 */
    ClRmdOptionsT            options;

} ClIdlHandleObjT;

/** Handle type */
extern char* clObjId_ClIdlHandleObjT;

/** Handle initializer */
#define CL_IDL_HANDLE_INVALID_VALUE { clObjId_ClIdlHandleObjT, {0},0,{0} }


/**
 *  IDL cookie. This is where IDL maintains the asynchronous callback and related
 *  information. This structure is used for generated code consumption.
 */
typedef struct ClIdlCookieT
{

/**
 * IDL handle.
 */
    ClIdlHandleT  handle;

/**
 * User's callback.
 */
    void (*actualCallback)();

/**
 * User's cookie.
 */
    void*         pCookie;

} ClIdlCookieT;

typedef struct ClIdlClntT
{

/**
 *  Database handle for handles.
 */
ClHandleDatabaseHandleT  idlDbHdl;

/**
 *  Handle count.
 */
ClUint32T                handleCount;
}ClIdlClntT;


/************************ IDL APIs ***************************/
/**
 ************************************
 *  \page pageidl201 clIdlHandleInitialize
 *
 *  \par Synopsis:
 *  Initializes the IDL handle.
 *
 *  \par Header File:
 *  clIdlApi.h
 *
 *  \par Syntax:
 *  \code 	ClRcT clIdlHandleInitialize(
 * 				CL_IN  ClIdlHandleObj *pIdlObj,
 * 				CL_OUT ClIdlHandleT   *pHandle);
 *  \endcode
 *
 *  \param pIdlObj: Object of the IDL handle. This contains information
 *   for communication to the Destination EO.
 *  \param pHandle: (out) Handle of the IDL object. This will be used
 *   in successive IDL calls.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_IDL_RC(CL_ERR_NULL_POINTER): If a NULL pointer is passed.
 *  \retval CL_IDL_RC(CL_ERR_NO_MEMORY): On failure to allocate memory.
 *
 *  \par Description:
 *  This API is used to initialize the IDL handle with various IDL
 *  parameters. Once initialized, the handle will be passed as the first
 *  parameter for all IDL calls.
 *
 *  \par Library File:
 *   libClIdl.a
 *
 *  \par Related Function(s):
 *  \ref pageidl203 "clIdlHandleFinalize" ,
 *  \ref pageidl202 "clIdlHandleUpdate"
 *
 */

ClRcT clIdlHandleInitialize(
       CL_IN  ClIdlHandleObjT*         pIdlObj,     /*Idl object*/
       CL_OUT ClIdlHandleT*           pHandle);    /*handle to the IDL object*/


/**
 ************************************
 *  \page pageidl202 clIdlHandleUpdate
 *
 *  \par Synopsis:
 *  Updates the IDL handle.
 *
 *  \par Header File:
 *  clIdlApi.h
 *
 *  \par Syntax:
 *  \code 	ClRcT clIdlHandleUpdate(
 * 			    CL_IN ClIdlHandleT    handle,
 * 				CL_IN ClIdlHandleObj *pIdlObj);
 *  \endcode
 *
 *  \param pHandle: (out) Handle of the IDL object.
 *  \param pIdlObj: Object of the IDL handle. This contains information
 *   for communication to the Destination EO.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_IDL_RC(CL_ERR_NULL_POINTER): If a NULL pointer is passed.
 *  \retval CL_IDL_RC(CL_ERR_NO_MEMORY): On failure to allocate memory.
 *
 *  \par Description:
 *  This API is used to update the IDL handle with various IDL
 *  parameters.
 *
 *  \par Library File:
 *  libClIdl.a
 *
 *  \par Related Function(s):
 *  \ref pageidl201 "clIdlHandleInitialize" ,
 *  \ref pageidl203 "clIdlHandleFinalize"
 */

ClRcT clIdlHandleUpdate(
        CL_IN ClIdlHandleT            handle,    /*handle to the IDL object*/
        CL_IN ClIdlHandleObjT*         pIdlObj);   /*Idl object*/


/**
 ************************************
 *  \page pageidl203 clIdlVersionCheck
 *
 *  \par Synopsis:
 *  Checks the given version.
 *
 *  \par Header File:
 *  clIdlApi.h
 *
 *  \par Syntax:
 *  \code   ClRcT clIdlHandleInitialize(
 * 			   CL_INOUT	ClVersionT  *pVersion);
 *  \endcode
 *
 *  \param pVersion: Pointer to the version.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_IDL_RC(CL_ERR_NULL_POINTER): If a NULL pointer is passed.
 *
 *  \par Description:
 *  This API is used to check the version of the IDL
 *  library. If it is not valid, it returns the supported
 *  version.
 *
 *  \par Library File:
 *  libClIdl.a
 *
 *  \par Related Function(s):
 *  None.
 */

ClRcT clIdlVersionCheck(
        ClVersionT *         pVersion);   /*pointer to the version*/

/**
 ************************************
 *  \page pageidl204 clIdlHandleFinalize
 *
 *  \par Synopsis:
 *  Removes the IDL handle.
 *
 *  \par Header File:
 *  clIdlApi.h
 *
 *  \par Syntax:
 *  \code 	ClRcT clIdlHandleFinalize(
 * 			  CL_IN	ClIdlHandleT handle );
 *  \endcode
 *
 *  \param handle: IDL handle used for making calls.
 *
 *  \retval CL_OK: The API executed successfully.
 *
 *  \par Description:
 *  This API is used to destroy the IDL handle.
 *  This deletes the handles created by handle library and
 *  frees the resources allocated during initialization.
 *
 *
 *  \par Library File:
 *  libClIdl.a
 *
 *  \par Related Function(s):
 *  \ref pageidl201 "clIdlHandleInitialize"
 *
 */
ClRcT clIdlHandleFinalize(CL_IN ClIdlHandleT   handle );


ClRcT clIdlHandleCheckout(ClIdlHandleT    handle, ClIdlHandleObjT** out);
ClRcT clIdlHandleCheckin(ClIdlHandleT    handle, ClIdlHandleObjT** out);


/*****************************************************************************/



#ifdef __cplusplus
}
#endif

#endif /* _CL_IDL_API_H_ */


/** \} */
