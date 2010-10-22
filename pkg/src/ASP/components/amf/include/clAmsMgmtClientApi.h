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

/**
 *  \file
 *  \brief Header file of AMS related APIs
 *  \ingroup ams_apis
 */

/**
 *  \addtogroup ams_apis
 *  \{
 */

/******************************************************************************
 * TO DO ITEMS
 * - Return values for functions are not comprehensive
 * - Confirm how management API will be RMDized
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_CLIENT_API_H_
#define _CL_AMS_MGMT_CLIENT_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCommonErrors.h>

#include <clAmsTypes.h>
#include <clAmsEntities.h>

// XXX Clean this up later
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtHooks.h>
#include <clLogApi.h>
/******************************************************************************
 * Management API related data structures
 *****************************************************************************/

#define ASP_INSTALL_KEY "ASP_INSTALL_INFO"

typedef struct
{
#if defined (CL_AMS_MGMT_HOOKS)
    ClRcT (*pEntityAdminResponse)
    (ClAmsEntityTypeT type,ClAmsMgmtAdminOperT oper,ClRcT retCode);
#else
    int nothingForNow;
#endif
} ClAmsMgmtCallbacksT;

/******************************************************************************
 * Management API functions summary
 * --------------------------------
 *
 * Functions to manipulate AMS entity
 * ----------------------------------
 * Initialize:          Initialize management api, get a handle
 * Finalize:            Finish use of management api
 *
 * Functions to manipulate all AMF entities
 * ----------------------------------------
 * Unlock:              Set admin state to unlock, ie can provide service
 * Lock:                Set admin state to lock, ie cannot provide service
 * LockAssignment:      Prevent entity from accepting service assignment
 * LockInstantiation:   Prevent entity from being instantiated, same as lock
 * Shutdown:            Set admin state to shutting down
 * Repaired:            Indicate that an entity is repaired and ready
 * Restart:             Restarts the entity
 *
 * Note on Execution Model
 * -----------------------
 * Note that many of these functions result in cascading policy actions
 * that affect a wide range of entities. Some of these operations may
 * require a significant time to be completed. It is not necessary for
 * a function to block until all operations are completed. A delayed
 * callback to the management client informs it of the conclusion of
 * the operation.
 *****************************************************************************/

/******************************************************************************
 * Functions to manipulate AMS
 *****************************************************************************/

/**
 ************************************
 *  \brief Starts the use of the management function library.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  (out) The handle returned by the function. It identifies 
 *  a particular initialization of the Availability Management Framework. This 
 *  handle must be passed as the first input parameter for all further usage of
 *  the function library.
 *
 *  \param amsCallbacks 
 *  (in) Callbacks into management function user.
 *
 *  \param version 
 *  (in/out) In the input parameter, you must pass the current version of AMS 
 *  on the client. In the output parameter, you will receive the supported 
 *  version.
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_VERSION_MISMATCH 
 *  If the current version and the supported version are not the same.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may retry later. 
 *
 *  \retval CL_ERR_INVALID_PARAMETER 
 *  A parameter is not set correctly. 
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory
 *  and so cannot provide the service.
 *
 *  \retval CL_ERR_NULL_POINTER  
 *  Pointer to an invalid memory space.
 *
 *  \par Description:
 *  This function is used to start the use of the management function library. 
 *  It typically registers callbacks and it must be called before invoking any 
 *  other function of the AMS library. In this release there are no callbacks 
 *  to be registered.
 *
 *  \par Library File:
 *  ClAmsMgmtClient
 *
 *  \sa clAmsMgmtFinalize()
 *
 */

extern ClRcT clAmsMgmtInitialize(
        CL_OUT  ClAmsMgmtHandleT  *amsHandle,
        CL_IN  const ClAmsMgmtCallbacksT  *amsMgmtCallbacks,
        CL_INOUT  ClVersionT  *version);


/**
 ************************************
 *  \brief Terminates the use of the management function library.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \retval CL_OK
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may retry later. 
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 

 *  \par Description:
 *   This function is used to terminate the use of the management function 
 *   library.This must be called when the services of the AMS library are no 
 *   longer required. This function frees all resources allocated during 
 *   initialization of the library through the clAmsMgmtInitialize() function.
 *
 *  \par Library File:
 *  ClAmsMgmtClient
 *
 *  \sa clAmsMgmtInitialize()
 *
 */

extern ClRcT clAmsMgmtFinalize(
        CL_IN       ClAmsMgmtHandleT            amsHandle);



/******************************************************************************
 * Generic functions for all AMS entities
 *****************************************************************************/

/**
 ************************************
 *  \brief Changes the administrative state of an AMS entity to lock assigned state. 
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already in lock assigned state 
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  Administrative action is not a valid operation for the logical entity in 
 *  the current administrative state.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \par Description:
 *  The administrative operation is applicable to all Availability Management 
 *  Framework entities that possess an administrative state, namely, service 
 *  unit, service instance, node, and service group. The invocation of this 
 *  administrative operation sets the administrative state of the logical 
 *  entity designated by entity to locked assignment. 
 *  \par
 *  If this operation is invoked by a client on an entity that is already locked
 *  assigned, there is no change in the status of such an entity i.e. it remains
 *  in the locked assigned state, but a benign error value CL_ERR_NO_OP is 
 *  returned to the client conveying that the entity in question, designated by
 *  entity, is already in the locked assigned state.  
 *  \par
 *  If this operation is invoked on an entity that is locked for instantiation, 
 *  there is no change in the status of such an entity, i.e. it remains in the 
 *  locked instantiation state, and the caller is returned a 
 *  CL_ERR_BAD_OPERATION error value.
 *
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa clAmsMgmtEntityLockInstantiation(), clAmsMgmtEntityUnlock()
 *
 */

extern ClRcT clAmsMgmtEntityLockAssignment(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

extern ClRcT clAmsMgmtEntityLockAssignmentExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClBoolT retry);


/**
 ************************************
 *  \brief Changes the administrative state of an AMS entity to lock instantiated 
 *  state. 
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *
 *  \retval CL_ERR_NO_MEMORY 
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already in lock instantiated state.
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  Administrative action is not a valid operation for the logical entity in 
 *  the current administrative state.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \par Description:
 *  The administrative operation is applicable to Availability Management 
 *  Framework entities service unit, node and service group. The invocation of 
 *  this administrative operation sets the administrative state of the logical 
 *  entity designated by entity to locked instantiation.
 *  \par
 *  After successful invocation of this procedure, all components in all 
 *  pertinent service units are terminated. In particular, all processes in 
 *  those components must cease to exist.
 *  \par
 *  Once this operation is invoked on a logical entity, as explained above, all
 *  pertinent service units within its scope become non-instantiable 
 *  ( after being terminated ) and the effect of this operation can be reversed
 *  by applying another administrative operation clAmsMgmtEntityLockAssignment,
 *  which cause the relevent service units to be instantiated in a locked 
 *  assigned state provided that the entity is not locked for instantiation at 
 *  any other level.
 *  \par
 *  If this operation is invoked by a client on an entity that is already locked
 *  instantiated, there is no change in the status of such an entity i.e. it 
 *  remains in the locked instantiated state, but a benign error value 
 *  CL_ERR_NO_OP is returned to the client conveying that the entity in question
 *  , designated by entity, is already in the locked instantiated state.
 *  \par
 *  If this operation is invoked on an entity that is either in the 
 *  shutting-down or unlocked administrative state, there is no change in the 
 *  status of such an entity, i.e. it remains in the respective state, and the 
 *  caller is returned a CL_ERR_BAD_OPERATION error value.
 *
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa clAmsMgmtEntityLockAssignment(), clAmsMgmtEntityUnlock()
 *
 */

extern ClRcT clAmsMgmtEntityLockInstantiation(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

extern ClRcT clAmsMgmtEntityLockInstantiationExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClBoolT retry);


/**
 ************************************
 *  \brief Changes the administrative state of an AMS entity to unlocked state. 
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *  \retval CL_ERR_NO_MEMORY
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already in lock instantiated state.
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  Administrative action is not a valid operation for the logical entity in 
 *  the current administrative state.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \par Description:
 *  The administrative operation is applicable to all Availability Management 
 *  Framework entities that possess an administrative state, namely, service 
 *  unit, service instance, node, and service group. The invocation of this 
 *  administrative operation sets the administrative state of the logical entity
 *  designated by entity to unlocked. If this operation is invoked by a client 
 *  on an entity that is already unlocked, there is no change in the status of 
 *  such an entity i.e. it remains in the unlocked state, but a benign error 
 *  value CL_ERR_NO_OP is returned to the client conveying that the entity in 
 *  question, designated by entity, is already in the unlocked state. If this 
 *  operation is invoked on an entity that is locked for instantiation, there is
 *  no change in the status of such an entity, i.e. it remains in the respective
 *  state, and the caller is returned a CL_ERR_BAD_OPERATION error value.
 *
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa clAmsMgmtEntityLockAssignment(), clAmsMgmtEntityLockInstantiation()
 */

extern ClRcT clAmsMgmtEntityUnlock(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity);

extern ClRcT clAmsMgmtEntityUnlockExtended(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClBoolT retry);

/**
 ************************************
 *  \brief Changes the administrative state of an AMS entity to shutting-down. 
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already in shutting-down state.
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  Administrative action is not a valid operation for the logical entity in 
 *  the current administrative state.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \par Description:
 *  The administrative operation is applicable to all Availability Management 
 *  Framework entities that possess an administrative state, namely, service 
 *  unit, service instance, node, and service group. The invocation of this 
 *  administrative operation sets the administrative state of the logical entity
 *  designated by entity to shutting-down. The administrative operation is 
 *  non-blocking i.e. It does not wait for the logical entity designated by 
 *  entity  to transition to the locked instantiation state, which can possibly
 *  take a very long time. 
 *  \par
 *  If this operation is invoked by a client on an entity that is already in 
 *  shutting-down administrative state, there is no change in the status of such
 *  an entity i.e. it continues shutting-down and the caller is returned a 
 *  benign CL_ERR_NO_OP error value, which means that entity is already 
 *  shutting-down.  
 *  \par
 *  If this operation is invoked on an entity that is in either in locked 
 *  instantiation or lock assignment administrative state, there is no change 
 *  in the status of such an entity, i.e. it remains in the respective state, 
 *  and the caller is returned a CL_ERR_BAD_OPERATION error value.
 *
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa 
 *   clAmsMgmtEntityLockAssignment(),
 *   clAmsMgmtEntityLockInstantiation(),
 *   clAmsMgmtEntityUnlock()
 */

extern ClRcT clAmsMgmtEntityShutdown(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

extern ClRcT clAmsMgmtEntityShutdownExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClBoolT retry);

/**
 ************************************
 *  \brief Restart an AMS entity following termination .
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *  \retval CL_ERR_NO_MEMORY 
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is currently restarting. 
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  The logical entity could not be restarted for various reasons like the 
 *  presence state of the service unit or the component to be re-started was not
 *  instantiated.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \retval CL_AMS_ERR_INVALID_ENTITY 
 *  Administrative operation is not valid for the entity type designated by type
 *  field in entity.
 *
 *  \par Description:
 *  The administrative operation is applicable for component, service unit or 
 *  node. This procedure typically involves a termination action followed by a 
 *  subsequent instantiation of either the concerned entity or logical entities
 *  that belong to the concerned entity.
 *  \par
 *  This administrative operation is applicable to only those service units 
 *  whose presence state is instantiated. The invocation of this administrative
 *  operation on a service unit causes the service unit to be restarted by 
 *  restarting all the components within it.
 *  \par 
 *  If this operation is invoked by a client on an entity that is not 
 *  restartable, there is no change in the status of such an entity, but the 
 *  caller is returned a benign CL_ERR_NO_OP error value, which means that no 
 *  action is being performed.
 *  \par 
 *  When this operation is performed on an individual component, only the 
 *  component implied in the operation is restarted. 
 *  \par 
 *  When this operation is invoked upon a service unit this operation becomes a
 *  composite operation that cause collective restart of all components within 
 *  the service unit. In-order to execute such a collective restart of all the 
 *  components in a particular scope, the AMF first completely terminates all 
 *  pertinent components and does not start instantiating them back until all 
 *  components have been terminated.
 *  \par 
 *  When invoked upon a node, this operation becomes a composite operation that
 *  causes a collective restart of all service units residing within the node. 
 *  In-order to execute such a collective restart of all the service units in a
 *  particular scope, the AMF first completely terminates all pertinent service
 *  units and does not start instantiating them back until all service units 
 *  have been terminated.
 *  \par 
 *  AMF does not proceed with this operation if another administrative operation
 *  or an error recovery initiated by AMF is already engaged on the logical 
 *  entity. In such case, an error value of CL_ERR_TRY_AGAIN is returned 
 *  indicating that the action is feasible but not at this instant.
 *
 *
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa
 *  clAmsMgmtEntityLockAssignment(),
 *  clAmsMgmtEntityLockInstantiation(),
 *  clAmsMgmtEntityUnlock(),
 *  clAmsMgmtEntityShutdown()
 */

extern ClRcT clAmsMgmtEntityRestart(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

extern ClRcT clAmsMgmtEntityRestartExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClBoolT retry);

/**
 ************************************
 *  \brief Marks a previously faulty entity as repaired, so that AMS can use it again 
 *  for work assignment.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already enabled. 
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  The operation could not insure that the presence states of the relevent 
 *  service units and components are either instantiated or uninstantiated. 
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \retval CL_AMS_ERR_INVALID_ENTITY 
 *  Administrative operation is not valid for the entity type designated by type
 *  field in entity.
 *
 *  \par Description:
 *  The administrative operation is applicable for a service unit or a node. 
 *  \par 
 *  This administrative operation is used to clear the disabled operational 
 *  state of a node or a service unit after they have been successfully mended 
 *  to declare them as repaired. The administrator uses this operation to 
 *  indicate the availability of a service unit or a node for providing service
 *  after an externally executed repair action. When invoked on a node this 
 *  operation results enabling the operational state of the constituent service
 *  units and components. When invoked on a service unit, it has a similar 
 *  effect on all the components that make up the service unit.
 *  \par 
 *  AMS might optionally engage in repairing a node or a service unit after a 
 *  successful recovery procedure execution in which case the AMS itself will 
 *  clear the disabled state of the involved node or a service unit, but if a
 *  repair action is undertaken by an external entity outside the scope of the
 *  AMS, or the AMS failed  to successfully repair 
 *  ( and the repair requires intervention by an external entity ), one should 
 *  use this administrative operation to clear the disabled state of the node 
 *  or the service unit to indicate that these entities are repaired and their 
 *  operational state is enabled.
 *  \par 
 *  If this administrative operation is invoked on a logical entity that is 
 *  already enabled, the entity remains in that state, and a benign error value
 *  of CL_ERR_NO_OP is returned to the caller.
 *  
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa
 *  clAmsMgmtEntityLockAssignment(),
 *  clAmsMgmtEntityLockInstantiation(),
 *  clAmsMgmtEntityUnlock(),
 *  clAmsMgmtEntityShutdown(),
 *  clAmsMgmtEntityRestart()
 */


/**
 ************************************
 *  \brief Marks a previously faulty entity as repaired, so that AMS can use it again 
 *  for work assignment.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param entity 
 *  Name and type of the entity on which the administrative action is being 
 *  performed
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_TIMEOUT 
 *  Timeout occured before the call could complete. It is unspecified whether 
 *  the call succeeded or whether it did not. 
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The service can not be provided at this time. The process may try later. 
 *  This error is generally returned in cases where the requested action is 
 *  valid but not currently possible, probably because another operation is 
 *  acting upon the logical entity on which the administrative operation is 
 *  invoked. Such an operation can be another administrative operation or an 
 *  error recovery initiated by the AMF. 
 *
 *
 *  \retval CL_ERR_NO_MEMORY  
 *  Either the AMS library or the provider of service is out of memory and can 
 *  not provide the service. 
 *
 *  \retval CL_ERR_NO_OP 
 *  Invocation of this administrative operation has no effect on the current 
 *  state of the logical entity as it is already enabled. 
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  The operation could not insure that the presence states of the relevent 
 *  service units and components are either instantiated or uninstantiated. 
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \retval CL_AMS_ERR_INVALID_ENTITY 
 *  Administrative operation is not valid for the entity type designated by type
 *  field in entity.
 *
 *  \par Description:
 *  The administrative operation is applicable for a service unit or a node. 
 *  \par 
 *  This administrative operation is used to clear the disabled operational 
 *  state of a node or a service unit after they have been successfully mended 
 *  to declare them as repaired. The administrator uses this operation to 
 *  indicate the availability of a service unit or a node for providing service
 *  after an externally executed repair action. When invoked on a node this 
 *  operation results enabling the operational state of the constituent service
 *  units and components. When invoked on a service unit, it has a similar 
 *  effect on all the components that make up the service unit.
 *  \par 
 *  AMS might optionally engage in repairing a node or a service unit after a 
 *  successful recovery procedure execution in which case the AMS itself will 
 *  clear the disabled state of the involved node or a service unit, but if a
 *  repair action is undertaken by an external entity outside the scope of the
 *  AMS, or the AMS failed  to successfully repair 
 *  ( and the repair requires intervention by an external entity ), one should 
 *  use this administrative operation to clear the disabled state of the node 
 *  or the service unit to indicate that these entities are repaired and their 
 *  operational state is enabled.
 *  \par 
 *  If this administrative operation is invoked on a logical entity that is 
 *  already enabled, the entity remains in that state, and a benign error value
 *  of CL_ERR_NO_OP is returned to the caller.
 *  
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa
 *  clAmsMgmtEntityLockAssignment(),
 *  clAmsMgmtEntityLockInstantiation(),
 *  clAmsMgmtEntityUnlock(),
 *  clAmsMgmtEntityShutdown(),
 *  clAmsMgmtEntityRestart()
 */

extern ClRcT clAmsMgmtEntityRepaired(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

extern ClRcT clAmsMgmtEntityRepairedExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClBoolT retry);


/**
 ************************************
 *  \brief Swaps the HA state of the appropriate CSIs contained within an SI
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param si
 *  Name of the SI thats being swapped
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_BAD_OPERATION 
 *  Swap was attempted on an SI thats not allowed. This is returned whenever SI
 *  doesnt have standby assignments or is not active or in quiescing state. Its also
 *  returned when the parent SG for the SI has a redundancy model of N WAY active or
 *  no redundancy.
 *
 *  \retval CL_ERR_TRY_AGAIN 
 *  The swap cannot be performed at this time. This error is returned when
 *  the constituent service units of the SIs have instantiating/terminating
 *  or restarting presence states.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \retval CL_AMS_ERR_INVALID_ENTITY 
 *  Administrative operation is not valid for the entity type designated by type
 *  field in entity.
 *
 *  \par Description:
 *  This administrative operation is used to swap the ha states of the CSIs within the SI
 *  assigned to service units. It results in the interchange of HA states of the CSIs
 *  assigned to the components within a service unit. If the SI is protected by 2N redundancy
 *  model, it results in swapping of all the active and standby SIs assigned to the service unit
 *  to which the SI getting swapped is assigned. In case of M+N, it also results in swapping
 *  of all the active SIs assigned to the service unit to which the swapped SI is assigned.
 *  This operation is not supported on N-WAY active and no redundancy models. Application of this 
 *  operation may potentially result in modification of the standby assignments of all the other SIs
 *  protected by the same service group but not assigned active to the service unit to which the
 *  swapped SI is assigned active.
 *  
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa
 *  clAmsMgmtEntityLockAssignment(),
 *  clAmsMgmtEntityLockInstantiation(),
 *  clAmsMgmtEntityUnlock(),
 *  clAmsMgmtEntityShutdown(),
 *  clAmsMgmtEntityRestart()
 *  clAmsMgmtEntityRepaired()
 */

extern ClRcT clAmsMgmtSISwap(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClCharT               *si);

extern ClRcT clAmsMgmtSISwapExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClCharT               *si,
        CL_IN       ClBoolT retry);



/**
 ************************************
 *  \brief Restores the SG to the most preferred assignments
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle 
 *  Handle identifying the initialization of the AML by the application 
 *  obtained through earlier invocation of clAmsMgmtInitialize API.
 *
 *  \param sg
 *  Name of the SG thats being adjusted
 *
 *  \param enable
 *  Flag indicating the auto adjustment setting on the SG
 *
 *  \retval CL_OK 
 *  The function executed successfully.
 *
 *  \retval CL_ERR_INVALID_HANDLE 
 *  Handle passed is not a valid handle obtained through an earlier call to 
 *  clAmsMmgmtInitialize API.
 *
 *  \retval CL_ERR_NOT_EXIST 
 *  Entity on which the administrative action is requested is not found in the 
 *  AMS database.
 *
 *  \retval CL_AMS_ERR_INVALID_ENTITY 
 *  Administrative operation is not valid for the entity type designated by type
 *  field in entity.
 *
 *  \par Description:
 *  This administrative operation is used to restore the SG to the most preferred assignments
 *  based on the rank of the associated service units. The need for auto adjustment arises when
 *  the service unit becomes instantiable or when its readiness state is IN-SERVICE or when an
 *  service instance is UNLOCKED. If the service group becomes eligible for auto adjustment after
 *  a fault or recovery, the adjustment procedure is run on the newly repaired service units after
 *  the service groups auto-adjust probation period expires on the repaired service units. But
 *  these service units are used in other operations like switchover and failover.
 *  
 *  \par Library File:
 *  libClAmsMgmt
 *
 *  \sa
 *  clAmsMgmtEntityLockAssignment(),
 *  clAmsMgmtEntityLockInstantiation(),
 *  clAmsMgmtEntityUnlock(),
 *  clAmsMgmtEntityShutdown(),
 *  clAmsMgmtEntityRestart(),
 *  clAmsMgmtEntityRepaired(),
 *  clAmsMgmtSISwap()
 */

extern ClRcT clAmsMgmtSGAdjust(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClCharT               *sg,
        CL_IN       ClBoolT                     enable);

extern ClRcT clAmsMgmtSGAdjustExtended(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClCharT               *sg,
        CL_IN       ClBoolT                     enable,
        CL_IN       ClBoolT retry);


/**
 ************************************
 * 
 *  \brief Enables debugging for AMS entitity(ies).
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API.  
 *
 *  \param entity (in) Name and type of the entity on which the debugFlags have to
 *  be set. NULL value for the entity indicates that debugFlags are being set 
 *  for entire AMS.  
 *
 *  \param debugFlags (in) This parameter defines the type of the debug messages to
 *  be enabled for entity(ies). See the AMS Debug Flags above for description 
 *  of various debugFlags.  AMS debug flags can be  used in conjunction with 
 *  each other. For example, to enable all types of logging messages, the 
 *  debugFlags combination will be : CL_AMS_MGMT_SUB_AREA_MSG | 
 *  CL_AMS_MGMT_SUB_AREA_STATE_CHANGE  | CL_AMS_MGMT_SUB_AREA_FN_CALL | 
 *  CL_AMS_MGMT_SUB_AREA_TIMER 
 *    
 *  \retval CL_OK  The function completed successfully. 
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained 
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It 
 *  is unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is 
 *  out of memory and cannot provide the service.  
 *
 *  \retval CL_ERR_NOT_EXIST Entity on which the action is requested is not found in 
 *  the AMS database.
 *                
 *  \par Description: 
 *  This API enables AMS logging for AMS entity(ies). AMS server logs different 
 *  types of messages. These messages have been divided in 4 categories i.e. 
 *  Important ams events related messages, timer related messages, entity state 
 *  change messages and function entry messages  (see details above). This API can 
 *  be used to enable logging of particular types of messages by AMS server.
 *  \par
 *  If the entity name is NULL, AMS server enables debugFlags for all AMS entities.
 *
 *  \par Library  File: 
 *  libClAmsMgmt
 *
 *  \sa clAmsMgmtDebugDisable(), clAmsMgmtDebugGet()
 * 
 */
 
extern ClRcT
clAmsMgmtDebugEnable(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClUint8T  debugFlags );



/**
 ************************************
 *
 *  \brief Disables debugging for AMS entitity(ies).  
 *
 *  \par Header File: 
 *  clAmsMgmtClientApi.h 
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API.  
 *
 *  \param entity (in) Name and type of the entity on which the debugFlags have to be
 *  set. NULL value for the entity indicates that debugFlags are being set for entire AMS.  
 *
 *  \param debugFlags (in) This parameter defines the type of the debug messages to 
 *  be disabled for entity(ies). See the AMS Debug Flags above for description
 *  of various debugFlags.  
 *  AMS debug flags can be  used in conjunction with each other. For example, to
 *  disble all types of logging messages, the debugFlags combination will be : 
 *  CL_AMS_MGMT_SUB_AREA_MSG | CL_AMS_MGMT_SUB_AREA_STATE_CHANGE  | 
 *  CL_AMS_MGMT_SUB_AREA_FN_CALL | CL_AMS_MGMT_SUB_AREA_TIMER 
 *
 *  \retval CL_OK The function completed successfully.  
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It 
 *  is unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is
 *  out of memory and cannot provide the service.  
 *
 *  \retval CL_ERR_NOT_EXIST Entity on which the administrative action is requested is 
 *  not found in the AMS database.  
 *
 *  \par Description: 
 *  This API disables AMS logging for AMS entity(ies). AMS server logs different
 *  types of messages. These messages have been divided in 4 categories i.e.
 *  Important ams events related messages, timer related messages, entity state 
 *  change messages and function entry messages  (see details above). This API 
 *  can be used to disable logging of particular types of messages by AMS server.
 *  If the entity name is NULL, AMS server disables debugFlags for all AMS 
 *  entities.  
 *
 *  \par Library  File: 
 *  libClAmsMgmt 
 *
 *  \sa clAmsMgmtDebugEnable(), clAmsMgmtDebugGet()
 *
 */

extern ClRcT
clAmsMgmtDebugDisable(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_IN  ClUint8T  debugFlags );


/**
 ************************************
 *
 *  \brief Returns the debugging flags for AMS entitity(ies).
 *
 *  \par Header File: 
 *  clAmsMgmtClientApi.h 
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API. 
 *
 *  \param entity (in) Name and type of the entity on which the debugFlags have to be
 *  set. NULL value for the entity indicates that debugFlags are being set for entire AMS.  
 *
 *  \param debugFlags (out) AML returns the enabled debugFlags for the AMS entity(ies)
 *  in this parameter. See the AMS Debug Flags above for description of various debugFlags.  
 *
 *  \retval CL_OK The function completed successfully.  
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It is
 *  unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is out
 *  of memory and cannot provide the service.  
 *
 *  \retval CL_ERR_NOT_EXIST Entity on which the administrative action is requested is 
 *  not found in the AMS database.  
 *
 *  \par Description: 
 *  This API retrieves the debug flags for AMS entity(ies). AMS server logs
 *  different types of messages. These messages have been divided in 4 categories
 *  i.e. Important ams events related messages, timer related messages, entity
 *  state change messages and function entry messages  (see details above). This
 *  API can be used to retrieve the types of messages, which are currently logged
 *  by AMS server.  
 *  \par
 *  If the entity name is NULL, AMS server returns debugFlags for all AMS
 *  entities.  
 *
 *  \par Library  File: 
 *  libClAmsMgmt 
 *
 *  \sa clAmsMgmtDebugEnable(), clAmsMgmtDebugDisable()
 *
 */

extern ClRcT
clAmsMgmtDebugGet(
        CL_IN  ClAmsMgmtHandleT  amsHandle,
        CL_IN  const ClAmsEntityT  *entity,
        CL_OUT  ClUint8T  *debugFlags );

/**
 ************************************
 *
 *  \brief Enables AMS debugging messages to be displayed on the console.  
 *
 *  \par Header File: 
 *  clAmsMgmtClientApi.h 
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API.  
 *
 *  \retval CL_OK The function completed successfully.  
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It is
 *  unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is 
 *  out of memory and cannot provide the service.  
 *
 *  \par Description: 
 *  This API is used to display AMS messages on the standard output. By default,
 *  AMS messages are sent to log server. This API enables the log messages to be
 *  also seen on the screen. This is a useful API to debug and understand AMS
 *  server behavior.  
 *
 *  \par Library  File: 
 *  libClAmsMgmt 
 *
 *  \sa clAmsMgmtDebugDisableLogToConsole()
 *
 */

extern ClRcT
clAmsMgmtDebugEnableLogToConsole(
        CL_IN  ClAmsMgmtHandleT  amsHandle );


/**
 ************************************
 *
 *  \brief Disables display of AMS debugging messages on the console.  
 *
 *  \par Header File: 
 *  clAmsMgmtClientApi.h 
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API.  
 *
 *  \retval CL_OK The function completed successfully.  
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It is
 *  unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is
 *  out of memory and cannot provide the service.  
 *
 *  \par Description: 
 *  This API is used to disable the display of AMS messages on the standard
 *  output. By default, AMS messages are sent to log server. So AMS messages can
 *  still be seen using ASP logging facility.  
 *
 *  \par Library  File: 
 *  libClAmsMgmt 
 *
 *  \sa clAmsMgmtDebugEnableLogToConsole()
 *
 */

extern ClRcT
clAmsMgmtDebugDisableLogToConsole(
        CL_IN  ClAmsMgmtHandleT  amsHandle );


/**
 ************************************
 *
 *  \brief Changes the alpha factor configured for a given SG
 *
 *  \par Header File: 
 *  clAmsMgmtClientApi.h 
 *
 *  \param amsHandle (in) Handle identifying the initialization of the AML by the
 *  application obtained through earlier invocation of clAmsMgmtInitialize API.  
 *
 *  \param entity (in) Name of the SG and type should be CL_AMS_ENTITY_TYPE_SG 
 *  to get current alpha factor configured for the SG
 *
 *  \param alphaFactor (in) The value for the alphaFactor which should be 
 *  between 0 and 100.
 *
 *  \retval CL_OK The function completed successfully.  
 *
 *  \retval CL_ERR_INVALID_HANDLE Handle passed is not a valid handle obtained
 *  through an earlier call to clAmsMgmtInitialize API.  
 *
 *  \retval CL_ERR_TIMEOUT Timeout occurred before the call could be completed. It is
 *  unspecified whether the call succeeded or whether it did not.  
 *
 *  \retval CL_ERR_NO_MEMORY Either the AMS library or the provider of service is
 *  out of memory and cannot provide the service.  
 *  
 *  \retval CL_ERR_INVALID_PARAMETER The alpha factor provided was invalid.
 *
 *  \retval CL_ERR_NOT_EXIST The SG wasnt found in the AMS DB to change the alpha factor
 *
 *  \par Description: 
 *  This API is used to change the SGs alpha factor configuration. This variable
 *  limits the active number of service units incase the available service units
 *  are less than the preferred active service units. This variable helps to 
 *  dynamically limit the active service units that can be assigned work. 
 *  Judicious usage of this variable would allow dynamic changes to redundancy 
 *  mode at runtime without disrupting service. For example with a 10+1 model, 
 *  one can start with an alpha factor of 10 to get 1+1 incase nodes are going 
 *  to be added dynamically or incase the active service units are supposed to
 *  be restricted. Making it 20, would give 2+1. So one can add the third blade
 *  and then change alpha factor to 20. By default, the alpha factor is set to 
 *  100 which takes into account, all the available service units. The 
 *  acceptable values for alpha factor are between 0 and 100. Providing a value 
 *  of 0 would result in assigning 1 active service unit incase the available 
 *  service units are less than preferred and SG has instantiable service units.
 *
 *  \par Library  File: 
 *  libClAmsMgmt 
 *
 *  \sa clAmsMgmtEntityGetConfig
 *
 */

extern ClRcT
clAmsMgmtEntitySetAlphaFactor( CL_IN ClAmsMgmtHandleT amsHandle,
                               CL_IN ClAmsEntityT *entity,
                               CL_IN ClUint32T alphaFactor );

/**
 ************************************
 *  \brief Initializes the AMS management control library.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param amlHandle (in) The AMS handle returned by the AMS on
 *  invocation of the clAmsMgmtInitialize() API.
 *  \param ccbHandle (out) The AMS management control handle returned
 *  by the library.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE The AMS management handle passed is
 *  invalid.
 *
 *  \par Description:
 *  The clAmsMgmtCCBInitialize() establishes the applications
 *  association with the AMS management control library. Application
 *  must call this function before using any of the clAmsMgmtCCB*
 *  APIs.  The CCB functions are transactional.  Application then call
 *  a number of functions to modify the AMF state, and when complete
 *  call clAmsMgmtCCBCommit() to commit the changes to the AMF database.
 *  The application can then reuse the handle for another group of state
 *  modifications.
 *
 *  However, if the application decides to abort the changes, it must call
 *  clAmsMgmtCCBFinalize(), and then clAmsMgmtCCBInitialize() to get a new
 *  handle.
 *
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBCommit(), clAmsMgmtCCBFinalize()
 *
 */

extern ClRcT
clAmsMgmtCCBInitialize(
        CL_IN   ClAmsMgmtHandleT    amlHandle,
        CL_OUT  ClAmsMgmtCCBHandleT *ccbHandle );


/**
 ************************************
 * \brief Finalizes the AMS management control library.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param ccbHandle (in) The CCB handle returned by the AMS on
 *  invocation of clAmsMgmtCCBInitialize() API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is
 *  invalid.
 *
 *  \par Description:
 *  The clAmsMgmtCCBFinalize() calls removes the applications
 *  association with the AMS management control library, and aborts
 *  any pending operations.  After this call the application should no
 *  longer use this handle to call any AMS management control functions.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBCommit(), clAmsMgmtCCBInitialize()
 *
 */

extern ClRcT
clAmsMgmtCCBFinalize(
        CL_IN   ClAmsMgmtCCBHandleT ccbHandle );


/**
 ************************************
 *  \brief Applies the operation specific to CCB context in AMS DB
 *  atomically.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param ccbHandle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *
 *  \par Description:
 *  The clAmsMgmtCCBCommit() API atomically carries out the previously
 *  specified AMS management control set operation(s) on the AMS
 *  DB. Applications must invoke this call ideally after they have
 *  called all the management control set APIs for the sets to take
 *  effect. 
 *
 *  After calling clAmsMgmtCCBCommit() the handle can be reused for another
 *  transaction.  To abort the transaction instead of committing, use
 *  clAmsMgmtCCBFinalize().
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBInitialize(), clAmsMgmtCCBFinalize()
 *
 */

extern ClRcT
clAmsMgmtCCBCommit(
        CL_IN   ClAmsMgmtCCBHandleT ccbHandle );


/**
 ************************************
 *  \brief Creates a new instance of an AMS entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param entity (in) The name and type of the new AMS entity to be
 *  created.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *
 *  \par Description:
 *  This API allows us to add new entity into an AMS database
 *  dynamically. The newly added entity can be either SG, SI, CSI,
 *  Node, SU or Component, depending on the type specified in ClAmsEntityT.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBEntityDelete()
 *
 */

extern ClRcT clAmsMgmtCCBEntityCreate(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   const ClAmsEntityT  *entity );


/**
 ************************************
 *  \brief Deletes an instance of an AMS entity from AMS database.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param entity (in) The name and type of the new AMS entity to be
 *  deleted.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *
 *  \par Description:
 *  This API allows to delete an AMS entity from the AMS database
 *  dynamically. The entity to be removed can be either SG, SI, CSI,
 *  Node, SU or Component.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBEntityCreate()
 *
 */

extern ClRcT clAmsMgmtCCBEntityDelete(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   const ClAmsEntityT  *entity );

/**
 ************************************
 *  \brief Sets one or more scalar attributes of an AMS entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param entityConfig (in) New configuration values for the AMS
 *  entity attributes(s) whose configuration is being set. AMS entity
 *  name and type is also specified in this structure.
 *  \param bitmask (in) Mask specifying the attribute(s) whose value
 *  is being set to new values.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *
 *  \par Description:
 *  This API allows us to set one or more of the single valued
 *  attribute(s) of an AMS entity. Two set more than one attribute,
 *  the bitMasks representing those attributes can be bitwise
 *  ORed. The entity can be a SG, SI, CSI, Node, SU or
 *  Component. Following paragraphs show what attributes of a
 *  particular entity can be set.
 *
 *  \par SG:
 *  \arg \c SG_CONFIG_ADMIN_STATE
 *
 *  \par SI:
 *  \arg \c SI_CONFIG_NUM_STANDBY_ASSIGNMENTS
 *
 *  \par Component:
 *  \arg \c COMP_CONFIG_TIMEOUTS
 *  \arg \c COMP_CONFIG_CAPABILITY_MODEL
 *  \arg \c COMP_CONFIG_RECOVERY_ON_TIMEOUT
 *  \arg \c COMP_CONFIG_INSTANTIATE_COMMAND
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa None
 *
 */

extern ClRcT clAmsMgmtCCBEntitySetConfig(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityConfigT    *entityConfig,
        CL_IN   ClUint64T   bitMask );


/**
 ************************************
 *  \brief Sets or creates a name value pair for a CSI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param csiName (in) The name of the CSI.
 *  \param nvp (in) The name value pair to be set/created.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *
 *  \par Description:
 *  This API sets a name value pair for the component service
 *  instance. If the name value pair specified does not already exist
 *  for the CSI, then AMS creates the name value pair.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBCSIDeleteNVP()
 *
 */

extern ClRcT clAmsMgmtCCBCSISetNVP(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsCSINVPT    *nvp );


/**
 ************************************
 *  \brief Deletes a name value pair for a CSI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param csiName (in) The name of the CSI.
 *  \param nvp (in) The name value pair for the CSI to be deleted.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_NOT_EXIST The CSI specified by the \e csiName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API deletes a name value pair of the component service
 *  instance.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBCSISetNVP()
 *
 */

extern ClRcT clAmsMgmtCCBCSIDeleteNVP(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsCSINVPT    *nvp );


/**
 ************************************
 *  \brief Adds a node to the node dependencies list of an AMS node.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param nodeName (in) Name of the node whose dependency list is
 *  being modified.
 *  \param dependencyNodeName (in) Name of the dependent node to be
 *  added in the node's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The node specified by the \e nodeName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows adding one node at a time to the dependency list
 *  of a particular node. Node dependency plays roles during CSI
 *  assignment, CSI removal etc and generally means that the dependent
 *  node cannot function properly without the node upon which it
 *  depends.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteNodeDependency()
 *
 */

extern ClRcT clAmsMgmtCCBSetNodeDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *dependencyNodeName );


/**
 ************************************
 *  \brief Deletes a node from the node dependencies list of an AMS
 *  node.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param nodeName (in) Name of the node whose dependency list is
 *  being modified.
 *  \param dependencyNodeName (in) Name of the dependent node to be
 *  removed from the node's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The node specified by the \e nodeName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows removing one node at a time from the dependency list
 *  of a particular node.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetNodeDependency()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteNodeDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *dependencyNodeName );


/**
 ************************************
 *  \brief Adds a SU to the SU list of an AMS node.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param nodeName (in) Name of the node whose SU list is being
 *  modified.
 *  \param suName (in) Name of SU being added to the SU list of the
 *  node.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The node specified by the \e nodeName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one SU at a time to the SU list of a
 *  node. By adding an SU to SU list of a node, we are establishing a
 *  containment relation that the SU represented by \e suName is
 *  contained in the node \e nodeName.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteNodeSUList()
 *
 */

extern ClRcT clAmsMgmtCCBSetNodeSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *suName );


/**
 ************************************
 *  \brief Removes a SU from the SU list of an AMS node.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param nodeName (in) Name of the node whose SU list is being
 *  modified.
 *  \param suName (in) Name of SU to be removed from the SU list of
 *  the node.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The node specified by the \e nodeName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one SU at a time from the su list of
 *  a node. By removing an SU to SU list of a node, we are destroying
 *  a containment relation that the SU represented by \e suName is
 *  contained in the node \e nodeName.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetNodeSUList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteNodeSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *nodeName,
        CL_IN   ClAmsEntityT    *suName );


/**
 ************************************
 *  \brief Adds a SU to the SU list of an AMS SG.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param sgName (in) Name of the SG whose SU list is being modified.
 *  \param suName (in) Name of SU being added to the SU list of the
 *  SG.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SG specified by the \e sgName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one SU at a time to the SU list of a
 *  SG. By adding an SU to SU list of a SG, we are establishing a
 *  containment relation that the SU represented by \e suName is
 *  contained (protected by) in the SG \e sgName for the purposes of
 *  carrying out high availability functions like switchover/failover
 *  etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSGSUList()
 *
 */

extern ClRcT clAmsMgmtCCBSetSGSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *suName );


/**
 ************************************
 *  \brief Deletes a SU from the SU list of an AMS SG.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param sgName (in) Name of the SG whose SU list is being modified.
 *  \param suName (in) Name of SU that is removed from the SU list of
 *  the SG.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SG specified by the \e sgName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one SU at a time from the SU list of
 *  a SG. By removing an SU from SU list of a SG, we are destroying a
 *  containment relation that the SU represented by \e suName is
 *  contained (protected by) in the SG \e sgName for the purposes of
 *  carrying out high availability functions like switchover/failover
 *  etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSGSUList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSGSUList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *suName );


/**
 ************************************
 *  \brief Adds a SI to the SI list of an AMS SG.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param sgName (in) Name of the SG whose SU list is being modified.
 *  \param siName (in) Name of SI being added to the SI list of the
 *  SG.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SG specified by the \e sgName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one SI at a time to the SI list of a
 *  SG. By adding an SI to SI list of a SG, we are establishing a
 *  containment relation that the SI represented by \e siName is
 *  contained (protected by) in the SG \e sgName for the purposes of
 *  carrying out high availability functions.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSGSIList()
 *
 */

extern ClRcT clAmsMgmtCCBSetSGSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *siName );


/**
 ************************************
 *  \brief Deletes a SI from the SI list of an AMS SG.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param sgName (in) Name of the SG whose SI list is being modified.
 *  \param siName (in) Name of SI that is removed from the SI list of
 *  the SG.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SG specified by the \e sgName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one SI at a time from the SI list of
 *  a SG. By removing an SI from SI list of a SG, we are destroying a
 *  containment relation that the SI represented by \e siName is
 *  contained (protected by) in the SG \e sgName for the purposes of
 *  carrying out high availability functions.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSGSIList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSGSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *sgName,
        CL_IN   ClAmsEntityT    *siName );


/**
 ************************************
 *  \brief Adds a component to the component list of an AMS SU.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param suName (in) Name of the SU whose component list is being
 *  modified.
 *  \param compName (in) Name of component being added to the
 *  component list of the SU.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SU specified by the \e suName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one component at a time to the component
 *  list of a SU. By adding an component to component list of a SU, we
 *  are establishing a containment relation that the component \e
 *  compName is contained in the SU \e suName for the purposes of
 *  carrying out high availability functions like fault escalation
 *  etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSUCompList()
 *
 */

extern ClRcT clAmsMgmtCCBSetSUCompList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *suName,
        CL_IN   ClAmsEntityT    *compName );


/**
 ************************************
 *  \brief Removes a component from the component list of an AMS SU.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param suName (in) Name of the SU whose component list is being
 *  modified.
 *  \param compName (in) Name of component that is being removed from
 *  the component list of the SU.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SU specified by the \e suName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one component at a time from the
 *  component list of a SU. By adding an component to component list
 *  of a SU, we are destroying a containment relation that the
 *  component \e compName is contained in the SU \e suName for the
 *  purposes of carrying out high availability functions like fault
 *  escalation etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSUCompList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSUCompList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *suName,
        CL_IN   ClAmsEntityT    *compName );


/**
 ************************************
 *  \brief Adds a SU in the SU rank list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose SU rank list is being
 *  modified.
 *  \param suName (in) Name of SU being added to the SU rank list of
 *  the SI.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SI specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one SU at a time to the SU rank list of
 *  a SI. By adding an SU to SU rank list of a SI, we are establishing
 *  a association relation that the SU \e suName is associated with
 *  the SI \e siName for the purposes of carrying out high
 *  availability functions like work assignment etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSISURankList()
 *
 */

extern ClRcT clAmsMgmtCCBSetSISURankList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *suName);


/**
 ************************************
 *  \brief Removes a SU from the SU rank list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose SU rank list is being
 *  modified.
 *  \param suName (in) Name of SU that is removed from the SU rank
 *  list of the SI.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SI specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one SU at a time from the SU rank
 *  list of a SI. By removing a SU from SU rank list of a SI, we are
 *  destroying the association relation that the SU \e suName is
 *  associated with the SI \e siName for the purposes of carrying out
 *  high availability functions like work assignment etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSISURankList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSISURankList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *suName );

/**
 ************************************
 *  \brief Adds a SI in the SI dependencies list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose dependency list is being
 *  modified.
 *  \param dependencySIName (in) Name of the dependent si to be
 *  added in the SI's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The si specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows adding one SI at a time to the dependency list of
 *  a particular SI. SI dependency plays roles during CSI assignment,
 *  CSI removal etc and generally means that the dependent SI must be
 *  operated upon before operating on the SI that depends on it.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSIDependency()
 *
 */

extern ClRcT clAmsMgmtCCBSetSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *dependencySIName );


/**
 ************************************
 *  \brief Deletes a SI from the SI dependencies list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose dependency list is being
 *  modified.
 *  \param dependencySIName (in) Name of the dependent SI to be
 *  removed from the SI's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SI specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows removing one SI at a time from the dependency list
 *  of a particular SI.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSIDependency()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *dependencySIName );


/**
 ************************************
 *  \brief Adds a CSI in the CSI dependencies list for an AMS CSI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param csiName (in) Name of the CSI whose dependency list is being
 *  modified.
 *  \param dependencyCSIName (in) Name of the dependent csi to be
 *  added in the CSI's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The csi specified by the \e csiName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows adding one CSI at a time to the dependency list of
 *  a particular CSI. CSI dependency plays roles during CSI assignment,
 *  CSI removal etc and generally means that the dependent CSI must be
 *  operated upon before operating on the CSI that depends on it.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteCSIDependency()
 *
 */

extern ClRcT clAmsMgmtCCBSetCSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsEntityT    *dependencyCSIName );

/**
 ************************************
 *  \brief Deletes a CSI from the CSI dependencies list for an AMS CSI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param csiName (in) Name of the CSI whose dependency list is being
 *  modified.
 *  \param dependencyCSIName (in) Name of the dependent CSI to be
 *  removed from the CSI's dependencies list.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The CSI specified by the \e csiName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows removing one CSI at a time from the dependency list
 *  of a particular CSI.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteCSIDependency()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteCSIDependency(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *csiName,
        CL_IN   ClAmsEntityT    *dependencyCSIName );


/**
 ************************************
 *  \brief Adds a CSI in the CSI list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose CSI list is being
 *  modified.
 *  \param csiName (in) Name of CSI being added to the CSI list of the
 *  SI.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SI specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to add one CSI at a time to the CSI list of a
 *  SI. By adding a CSI to CSI list of a SI, we are establishing a
 *  containment relation that the CSI \e csiName is contained in the
 *  SI \e siName for the purposes of carrying out high availability
 *  functions like fault escalation etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBDeleteSICSIList()
 *
 */

extern ClRcT clAmsMgmtCCBSetSICSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *csiName );


/**
 ************************************
 *  \brief Removes a CSI from the CSI list for an AMS SI.
 *
 *  \par Header File:
 *  clAmsMgmtClientApi.h
 *
 *  \param handle (in) The CCB handle returned by the AMS on
 *  invocation of the clAmsMgmtCCBInitialize() API.
 *  \param siName (in) Name of the SI whose CSI list is being
 *  modified.
 *  \param csiName (in) Name of CSI that is being removed from the CSI
 *  list of the SI.
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE AMS management handle passed is invalid.
 *  \retval CL_ERR_DOESNT_EXIST The SI specified by the \e siName
 *  doesn't exist in AMS database.
 *
 *  \par Description:
 *  This API allows us to remove one CSI at a time from the CSI list
 *  of a SI. By adding an CSI to CSI list of a SI, we are destroying a
 *  containment relation that the CSI \e csiName is contained in the
 *  SI \e siName for the purposes of carrying out high availability
 *  functions like fault escalation etc.
 *
 *  \par Library File:
 *  ClAmsMgmt
 *
 *  \sa clAmsMgmtCCBSetSICSIList()
 *
 */

extern ClRcT clAmsMgmtCCBDeleteSICSIList(
        CL_IN   ClAmsMgmtCCBHandleT handle,
        CL_IN   ClAmsEntityT    *siName,
        CL_IN   ClAmsEntityT    *csiName );


/**
 * \brief Returns the configuration and status scalar attributes of an AMS entity
 *
 * \param handle      Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param entityRef   Name of the AMS entity whose configuration and status attributes are queried. The attributes are also returned in this structure.
 *
 *
 * \return OpenClovis return code
 * \retval  CL_OK                     Operation successful
 * \retval  CL_ERR_INVALID_HANDLE     Error: AML CCB Handle is invalid
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */

extern ClRcT clAmsMgmtEntityGet(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_INOUT    ClAmsEntityRefT    *entityRef);




/**
 * \brief Returns the configuration scalar attributes of an AMF entity
 *
 * \param handle       Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param entity       Name of the AMS entity whose configuration attributes are queried
 * \param entityConfig Entity configuration structure returned by AMF
 *
 * \return OpenClovis return code
 * \retval CL_OK                      Operation successful
 * \retval CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 */
extern ClRcT clAmsMgmtEntityGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN   ClAmsEntityT    *entity,
        CL_OUT  ClAmsEntityConfigT  **entityConfig);

/**
 * \brief Returns the configuration information for nodes
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */
extern ClAmsNodeConfigT* clAmsMgmtNodeGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the configuration information for a service group
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsSGConfigT* clAmsMgmtServiceGroupGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the configuration information for a service unit.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsSUConfigT* clAmsMgmtServiceUnitGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the configuration information for a service instance.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */        
extern ClAmsSIConfigT* clAmsMgmtServiceInstanceGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the configuration information for a component service instance.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsCSIConfigT* clAmsMgmtCompServiceInstanceGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);
    
/**
 * \brief Returns the configuration information for a component.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName Null terminated name of the AMS entity.
 *
 * \return NULL entity does not exist, or a pointer to the entity configuration.  You must free (clHeapFree) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current configuration of the entity by
 * calling clAmsMgmtEntityGetConfig with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */     
extern ClAmsCompConfigT* clAmsMgmtCompGetConfig(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);


/**
 * \brief returns the status (transient) scalar attributes of an AMS entity
 *
 * \param handle         Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entity         Name of the AMS entity whose status attributes are queried
 * \param entityStatus   Entity status structure returned by AMF.  It is your responsibility to clHeapFree() the returned object.
 *
 * \return
 * \retval  CL_OK                      - Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      - Error: AML CCB Handle is invalid
 * \retval CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtEntityGetStatus(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *entity,
        CL_OUT  ClAmsEntityStatusT  **entityStatus);

/**
 * \brief Returns the current status information of a node.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */   
extern ClAmsNodeStatusT* clAmsMgmtNodeGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the current status information of a service group.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */      
extern ClAmsSGStatusT* clAmsMgmtServiceGroupGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the current status information of a service unit.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsSUStatusT* clAmsMgmtServiceUnitGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the current status information of a service instance.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsSIStatusT* clAmsMgmtServiceInstanceGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the current status information of a component service instance.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsCSIStatusT* clAmsMgmtCompServiceInstanceGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Returns the current status information of a component.
 *
 * \param handle Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param entName NULL terminated name of the AMS entity.
 *
 * \return A pointer to the entity's status object, or NULL.  You must free (clHeapFree()) this pointer.
 *
 * \par Description:
 * This is a convenience function that returns the current status of the entity by
 * calling clAmsMgmtEntityGetStatus() with the correct ClAmsEntityT structure.
 *
 * \sa clAmsMgmtEntityGetConfig()
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClAmsCompStatusT* clAmsMgmtCompGetStatus(
        CL_IN   ClAmsMgmtHandleT    handle,
        CL_IN const ClCharT *entName);

/**
 * \brief Get all entities of a particular type.
 *
 * \param handle   Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param listName What type of entity to get.
 *
 * \return Success or error code
 *
 * \par Description:
 * The function gets the names of all entities of a particular type.
 * It can be called for each type to get the names of all entities.
 * These names can then be used to retrieve the configuration and status of
 * every entity, giving access to the complete AMF entity database.
 *
 * \sa clAmsMgmtEntityGetConfig(), clAmsMgmtEntityGetStatus()
 *
 * \par Library File:
 *  ClAmsMgmt
 */
extern ClRcT clAmsMgmtGetList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityListTypeT listName,
        CL_OUT  ClAmsEntityBufferT  *buffer);


/**
 * \brief returns the name value pair list for a csi 
 *
 * \param handle       Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param  csi         Name of the csi whose nvp list is queried 
 * \param  nvpBuffer   Buffer containing the nvp list for the csi 
 *
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 */    
extern ClRcT clAmsMgmtGetCSINVPList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *csi,
        CL_OUT  ClAmsCSINVPBufferT  *nvpBuffer);

/**
 * returns the csi-csi dependencies list for a csi 
 *
 * \param handle                     Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param csi                        Name of the csi whose csi-csi dependencies list is queried
 * \param dependenciesCSIBuffer      Buffer containing the csi-csi dependencies list for the csi 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */

extern ClRcT clAmsMgmtGetCSIDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *csi,
        CL_OUT  ClAmsEntityBufferT  *dependenciesCSIBuffer);

/**
 * \brief Get all service group names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 */       
extern ClRcT clAmsMgmtGetSGList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);

/**
 * \brief Get all service instance entity names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */         
extern ClRcT clAmsMgmtGetSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);

/**
 * \brief Get all component service instance entity names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */        
extern ClRcT clAmsMgmtGetCSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);

/**
 * \brief Get all node entity names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */         
extern ClRcT clAmsMgmtGetNodeList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);

/**
 * \brief Get all service unit entity names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */      
extern ClRcT clAmsMgmtGetSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);

/**
 * \brief Get all component entity names
 *
 * \param handle         Handle returned by the AMF on invocation of clAmsMgmtInitialize API.
 * \param entityBuffer   Buffer containing an array of entity names
 *
 * \return  Clovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 *
 * \par Description:
 * This is a type-safe wrapper around clAmsMgmtGetList()
 *
 * \sa clAmsMgmtGetList()
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */     
extern ClRcT clAmsMgmtGetCompList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_OUT  ClAmsEntityBufferT *entityBuffer);


/**
 * \brief returns the node dependencies list for a node 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param node                      Name of the node whose node dependencies list is queried 
 * \param dependencyBuffer          Buffer containing the node dependencies list for the node 
 *
 *
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetNodeDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *dependencyBuffer);



/**
 * \brief returns the node su list for a node 
 *
 * \param handle                  Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param node                    Name of the node whose su list is queried 
 * \param suBuffer                Buffer containing the node su list for the node 
 *
 * \return OpenClovis return code
 * \retval CL_OK                  Operation successful
 * \retval CL_ERR_INVALID_HANDLE  Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetNodeSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *node,
        CL_OUT  ClAmsEntityBufferT  *suBuffer);


    /**
 * returns the sg su list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose su list is queried 
 * \param suBuffer                  Buffer containing the sg su list for the sg 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */


extern ClRcT clAmsMgmtGetSGSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *suBuffer);



    /**
 * returns the sg si list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose si list is queried 
 * \param siBuffer                  Buffer containing the sg si list for the sg 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */


extern ClRcT clAmsMgmtGetSGSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *siBuffer);


    /**
 * returns the component list for a su 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param su                        Name of the su whose component list is queried 
 * \param compBuffer                Buffer containing the component list for the su 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */


extern ClRcT clAmsMgmtGetSUCompList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsEntityBufferT  *compBuffer);



    /**
 * returns the si-su rank list for a si 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param si                        Name of the si whose su rank list is queried 
 * \param suBuffer                  Buffer containing the su rank list for the si 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSISURankList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *suBuffer);

/**
 * returns the si-si dependencies list for a si 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param si                        Name of the si whose si-si dependencies list is queried
 * \param dependenciesSIBuffer      Buffer containing the si-si dependencies list for the si 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSIDependenciesList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *dependenciesSIBuffer);



    /**
 * returns the si-csi list for a si 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param si                        Name of the si whose si-csi list is queried 
 * \param csiBuffer                 Buffer containing the csi list for the si 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSICSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsEntityBufferT  *csiBuffer);



    /**
 * returns the instantiable su list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose instantiable su list is queried 
 * \param instantiableSUBuffer      Buffer containing the instantiable su list for the sg 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSGInstantiableSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *instantiableSUBuffer);


    /**
 * returns the instantiated su list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose instantiated su list is queried 
 * \param instantiatedSUBuffer      Buffer containing the instantiated su list for the sg 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */


extern ClRcT clAmsMgmtGetSGInstantiatedSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *instantiatedSUBuffer);


    /**
 * returns the in service spare su list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose inservice spare su list is queried 
 * \param inserviceSpareSUBuffer    Buffer containing the inservice spare su list for the sg 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSGInServiceSpareSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *inserviceSpareSUBuffer);

    /**
 * returns the assigned su list for a sg 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose assigned su list is queried 
 * \param assignedSUBuffer          Buffer containing the assigned su list for the sg 
 *
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */


extern ClRcT clAmsMgmtGetSGAssignedSUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *assignedSUBuffer);



    /**
 * returns the faulty su list for a sg 
 *
 *
 * @param
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param sg                        Name of the sg whose faulty su list is queried 
 * \param faultySUBuffer            Buffer containing the faulty su list for the sg 
 *
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSGFaultySUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *faultySUBuffer);




    /**
 *  
 * returns the assigned si's list for su 
 *
 *
 * @param
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param su                        Name of the su whose assigned si's list is queried 
 * \param siBuffer                  Buffer containing the si's list for the su 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSUAssignedSIsList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *su,
        CL_OUT  ClAmsSUSIRefBufferT  *siBuffer);


    /**
 * returns the su list for si 
 *
 *
 * @param
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param si                        Name of the si whose su list is queried 
 * \param suBuffer                  Buffer containing the su list for the si 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetSISUList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *si,
        CL_OUT  ClAmsSISURefBufferT  *suBuffer);


/**
 *
 * returns the list of csi's assigned to a component 
 *
 * \param handle                    Handle returned by the AML on invocation of clAmsMgmtInitialize API.
 * \param comp                      Name of the component whose csi list is queried 
 * \param csiBuffer                 Buffer containing the list of csi's assigned to the component 
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_AMS_ERR_INVALID_ENTITY  The entity is not valid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */
extern ClRcT clAmsMgmtGetCompCSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *comp,
        CL_OUT  ClAmsCompCSIRefBufferT  *csiBuffer);


/**
 *
 * Convert an SG from one redundancy model to another, without taking it offline
 *
 * \param handle       Handle returned by the AML on invocation of clAmsMgmtInitialize() API.
 * \param sg           Name of the Service Group that is being modified
 * \param prefix       Newly created entities' names will start with this string 
 * \param activeSUs    Number of service units you want to be active.
 * \param standbySUs   Number of service units you want to be standby.
 * \param migrateList  All entities that were created for you to tweak before committing.
 *
 * \return OpenClovis return code
 * \retval  CL_OK                      Operation successful
 * \retval  CL_ERR_INVALID_HANDLE      Error: AML CCB Handle is invalid
 * \retval  CL_ERR_NOT_EXIST           The entity does not exist
 *
 *
 * \par Description:
 * This function uses all of the other functions defined in this module
 * to implement a major configuration change to an SG.  The SG can be
 * either inservice or out-of-service during this call.  Inservice entities
 * will be disturbed as little as possible during the configuration modification.
 *
 * The function does not give you complete control over all aspects of the
 * configuration change, notably it does not let you choose which SUs to
 * remove (if you are reducing the number of SUs), and it does not let you
 * choose which nodes to deploy new SUs onto (if increasing the SUs).
 * For this reason, it may be more appropriate for you to use this function
 * as an example implementation rather than to call it directly.
 *
 * However, we do attempt to make intelligent decisions about what to remove or
 * add.  For example, the function will prefer to remove entities that are not
 * in service (so if you want to remove particular entities, simply shut those
 * and only those entities down before calling this function), and it will
 * preferentially create entities on existing nodes that are not controllers and
 * do not currently contain this SG.
 *
 * \par Library File:
 *  ClAmsMgmt
 *
 */    
extern ClRcT clAmsMgmtMigrateSG(ClAmsMgmtHandleT handle,
                                const ClCharT *sg,
                                const ClCharT *prefix,
                                ClUint32T activeSUs,
                                ClUint32T standbySUs,
                                ClAmsMgmtMigrateListT *migrateList);

/*
 *Storing persistent entity user data for AMS entities.
 */

/**
 ************************************
 *  \brief Associate arbitrary data with an AMF entity
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity
 *  \param data   (in) Buffer of data to associate
 *  \param len    (in) Length (bytes) of the data in the buffer
 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API persistently associates an arbitrary chunk of data with this entity.
 *  This data can be used by application code for any purpose.
 *
 *  \sa clAmsMgmtEntityUserDataDelete(), clAmsMgmtEntityUserDataGet(), clAmsMgmtEntityUserDataSetKey() 
 *
 */    
extern ClRcT clAmsMgmtEntityUserDataSet(ClAmsMgmtHandleT handle, 
                                        ClAmsEntityT *entity,
                                        ClCharT *data, ClUint32T len);


/**
 ************************************
 *  \brief Associate arbitrary data with an AMF entity and a key
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity
 *  \param data   (in) Buffer of data to associate
 *  \param len    (in) Length (bytes) of the data in the buffer
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_CNT:CL_ERR_NOT_EXIST  The key does not exist
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API persistently associates an arbitrary chunk of data with this entity.
 *  This data can be used by application code for any purpose.  This API allows
 *  multiple applications to store independent data through the use of a key.
 *  This is not meant to be a database!  A fixed and small # of key slots are available
 *  (can be sized by changing a constant and recompilation).
 *
 *  \sa clAmsMgmtEntityUserDataDeleteKey(), clAmsMgmtEntityUserDataSet(), clAmsMgmtEntityUserDataGetKey() 
 *
 */     
extern ClRcT clAmsMgmtEntityUserDataSetKey(ClAmsMgmtHandleT handle, 
                                           ClAmsEntityT *entity,
                                           ClNameT *key,
                                           ClCharT *data, ClUint32T len);
/**
 ************************************
 *  \brief Retrieve arbitrary data associated with an AMF entity
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity
 *  \param data   (out) pointer to buffer of data.  I will malloc.  Free using clHeapFree)
 *  \param len    (out) Length (bytes) of the data in the buffer
 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_CNT:CL_ERR_NOT_EXIST  The key does not exist
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API retrieves the arbitrary chunk of data associated with this entity.
 *  This data can be used by application code for any purpose.
 *
 *  \sa clAmsMgmtEntityUserDataDelete(), clAmsMgmtEntityUserDataSet(), clAmsMgmtEntityUserDataGetKey() 
 *
 */      
extern ClRcT clAmsMgmtEntityUserDataGet(ClAmsMgmtHandleT handle, 
                                        ClAmsEntityT *entity,
                                        ClCharT **data, ClUint32T *len);

/**
 ************************************
 *  \brief Retrieve arbitrary data associated with an AMF entity and a key
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity
 *  \param data   (out) pointer to buffer of data.  I will malloc.  Free using clHeapFree)
 *  \param len    (out) Length (bytes) of the data in the buffer
 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_CNT:CL_ERR_NOT_EXIST  The key does not exist
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API retrieves the arbitrary chunk of data associated with this entity.
 *  This data can be used by application code for any purpose.  This API allows
 *  multiple applications to store independent data through the use of a key.
 *  This is not meant to be a database!  A fixed and small # of key slots are available
 *  (can be sized by changing a constant and recompilation).
 *
 *  \sa clAmsMgmtEntityUserDataDeleteKey(), clAmsMgmtEntityUserDataSet(), clAmsMgmtEntityUserDataGetKey() 
 *
 */      
extern ClRcT clAmsMgmtEntityUserDataGetKey(ClAmsMgmtHandleT handle, 
                                           ClAmsEntityT *entity,
                                           ClNameT *key,
                                           ClCharT **data, ClUint32T *len);
    
/**
 ************************************
 *  \brief Deletes the data for the default key associated with an AMF entity
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API deletes the arbitrary chunk of data for the default key associated with this entity.
 *  This data can be used by application code for any purpose.
 *
 *  \sa clAmsMgmtEntityUserDataDeleteKey(), clAmsMgmtEntityUserDataSet(), clAmsMgmtEntityUserDataGetKey() 
 *
 */      
extern ClRcT clAmsMgmtEntityUserDataDelete(ClAmsMgmtHandleT handle, 
                                           ClAmsEntityT *entity);

/**
 ************************************
 *  \brief Deletes the arbitrary data associated with an AMF entity and a key
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity
 *  \param data   (out) pointer to buffer of data.  I will malloc.  Free using clHeapFree)
 *  \param len    (out) Length (bytes) of the data in the buffer
 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_CNT:CL_ERR_NOT_EXIST  The key does not exist
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API deletes the arbitrary chunk of data associated with this entity.
 *  This data can be used by application code for any purpose.  This API allows
 *  key-specific data to be deleted.
 *  This is not meant to be a database!  A fixed and small # of key slots are available
 *  (can be sized by changing a constant and recompilation).
 *
 *  \sa clAmsMgmtEntityUserDataSetKey(), clAmsMgmtEntityUserDataGetKey() 
 *
 */    
extern ClRcT clAmsMgmtEntityUserDataDeleteKey(ClAmsMgmtHandleT handle, 
                                              ClAmsEntityT *entity,
                                              ClNameT *key);


/**
 ************************************
 *  \brief Deletes all the data associated with an AMF entity for all keys
 *
 *  \param handle (in) The AMS handle returned from clAmsMgmtInitialize() API.
 *  \param entity (in) Name and type of the relevant entity 
 * 
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_CID_AMS:CL_ERR_NOT_EXIST  The entity does not exist
 *
 *  \par Description:
 *  This API deletes the arbitrary chunk of data for all the keys associated with this entity.
 *  This data can be used by application code for any purpose.
 *
 *  \sa clAmsMgmtEntityUserDataDeleteKey(), clAmsMgmtEntityUserDataSet(), clAmsMgmtEntityUserDataGetKey() 
 *
 */      
extern ClRcT clAmsMgmtEntityUserDataDeleteAll(ClAmsMgmtHandleT handle, 
                                              ClAmsEntityT *entity);


extern ClRcT clAmsMgmtSetActive(ClAmsMgmtHandleT handle, ClAmsEntityT *entity, ClAmsEntityT *activeSU);
                     
extern ClRcT clAmsMgmtSIAssignSU(const ClCharT *si, const ClCharT *activeSU, const ClCharT *standbySU);

extern ClRcT clAmsMgmtGetAspInstallInfo(ClAmsMgmtHandleT handle, const ClCharT *nodeName,
                                        ClCharT *aspInstallInfo, ClUint32T len);

#ifdef __cplusplus
}
#endif

#endif							/* _CL_AMS_MGMT_CLIENT_API_H_ */


/** \} */


