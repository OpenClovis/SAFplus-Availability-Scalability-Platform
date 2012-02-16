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
 * ModuleName  : amf                                                           
 * File        : clAmsMgmtClientIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :  
 *
 * This header file contains AMS management function related definitions. The
 * management function is used by a management client (or an intermediatory
 * called by a management client to manipulate the AMS config database.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/


/**************************************************************************************
*********************************AMS Management Client Functions***********************
***************************************************************************************/
/*  pageams101 : clAmsMgmtInitialize                                                  */
/*  pageams102 : clAmsMgmtFinalize                                                    */
/*  pageams103 : clAmsMgmtAMSEnable                                                   */
/*  pageams104 : clAmsMgmtAMSDisable                                                  */
/*  pageams105 : clAmsMgmtEntityCreate                                                */
/*  pageams106 : clAmsMgmtEntityDelete                                                */
/*  pageams107 : clAmsMgmtEntityFindByName                                            */
/*  pageams108 : clAmsMgmtEntityFindByAttribute                                       */
/*  pageams109 : clAmsMgmtEntitySetConfig                                             */
/*  pageams110 : clAmsMgmtEntityLockAssignment                                        */
/*  pageams111 : clAmsMgmtEntityLockInstantiation                                     */
/*  pageams112 : clAmsMgmtEntityUnlock                                                */
/*  pageams113 : clAmsMgmtEntityShutdown                                              */
/*  pageams114 : clAmsMgmtEntityRestart                                               */
/*  pageams115 : clAmsMgmtEntityRepaired                                              */
/*  pageams116 : clAmsMgmtEntityStartMonitoringEntity                                 */
/*  pageams117 : clAmsMgmtEntityStopMonitoringEntity                                  */
/*  pageams118 : clAmsMgmtSGAdjustPreference                                          */
/*  pageams119 : clAmsMgmtSGAssignSUtoSG                                              */
/*  pageams120 : clAmsMgmtSISwap                                                      */
/*  pageams121 : clAmsMgmtEntityListEntityRefAdd                                      */
/*  pageams122 : clAmsMgmtEntityGetEntityList                                         */
/*  pageams123 : clAmsMgmtEntitySetRef                                                */
/*  pageams124 : clAmsMgmtCSISetNVP                                                   */
/*  pageams125 : clAmsMgmtDebugEnable                                                 */
/*  pageams126 : clAmsMgmtDebugDisable                                                */
/*  pageams127 : clAmsMgmtDebugGet                                                    */
/*                                                                                    */
/**************************************************************************************/


/******************************************************************************
 * TO DO ITEMS
 * - Return values for functions are not comprehensive
 * - Confirm how management API will be RMDized
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_CLIENT_IPI_H_
#define _CL_AMS_MGMT_CLIENT_IPI_H_

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
#include <clLogApi.h>
/******************************************************************************
 * Management API related data structures
 *****************************************************************************/

/******************************************************************************
 * Generic functions for all AMS entities
 *****************************************************************************/
/*
 ************************************
 *  \page pageams105  clAmsMgmtEntityCreate
 *
 *  \par Synopsis:
 *  Creates a new instance of an AMS entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntityCreate(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       const ClAmsEntityT          *entity);
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param entity: Entity type and name.
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid.
 *
 *  \par Description:
 *  Create a new instance of an AMS entity such as Node, Application, SG,
 *  SU, SI, Component or CSI in the AMS entity database.
 *
 *  The entity is created with a default config and only the entity name
 *  and type is set by default. All other entity attributes must be
 *  configured by calling the clAmsMgmtEntitySetConfig function. All
 *  entities are created with a management state of disabled and must be
 *  explicitly added into the active entity database.
 *
 *  This function wraps equivalent AMS database function functions.
 *
 *  \par Library File:
 *
 *  \par Related function(s):
 */

extern ClRcT clAmsMgmtEntityCreate(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);

/*
 ************************************
 *  \page pageams106  clAmsMgmtEntityDelete
 *
 *  \par Synopsis:
 *  Removes an instance of an AMS entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntityDelete(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       const ClAmsEntityT          *entity);
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param entity: Entity type and name.
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid.
 *
 *  \par Description:
 *  This function is used to remove an instance of an AMS entity such as Node, Application, SG, SU,
 *  SI, Component or CSI from the AMS entity database.
 *
 *  The entity is identified either by an handle, or if its null, then by
 *  the entity type and name.
 *
 *  For all entities with an admin state (node, application, SG, SU, SI),
 *  the entity is first brought into a locked state, before being removed
 *  from the database. The state transitions will trigger the evaluation
 *  of necessary AMS policies. If state transitions are locked for an entity,
 *  the locks are overridden. Entities without an admin state (component
 *  and CSI) are removed from the appropriate data bases after removing
 *  the entities from service.
 *
 *  This function wraps equivalent AMS database and client function functions.
 *
 *  \par Library File:
 *
 *  \par Related function(s):
 */

extern ClRcT clAmsMgmtEntityDelete(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity);
/*
 ************************************
 *  \page pageams107  clAmsMgmtEntityFindByName
 *
 *  \par Synopsis:
 *  Finds an instance of an AMS entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntityFindByName(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_INOUT    ClAmsEntityRefT             *entityRef);
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param entity: (in/out) Entity type and name.
 *
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid.
 *  \retval CL_ERR_NOT_EXIST: Error: Resource/Entity does not exist.
 *
 *  \par Description:
 *  This function is used to find an instance of an AMS entity in the appropriate AMS entity
 *  database. The entity is identified by the entity type and name
 *  and returns the entity handle.
 *
 *  This function wraps equivalent AMS database functions.
 *
 *  \par Library File:
 *
 *   \par Related function(s):
 */

extern ClRcT clAmsMgmtEntityFindByName(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_INOUT    ClAmsEntityRefT             *entityRef);

/*
 ************************************
 *  \page pageams109 clAmsMgmtEntitySetConfig
 *
 *  \par Synopsis:
 *  Configures attributes for AMS entities.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntitySetConfig(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       const ClAmsEntityT          *entity,
 *                               CL_IN       ClAmsEntityConfigT          *entityConfig,
 *                               CL_IN       ClUint32T                   peInstantiateFlag );
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param entity: Entity type and name.
 *  \param attributeChangeList: Structure indicating which attributes to change and values.
 *
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid.
 *  \retval CL_ERR_NOT_EXIST: Error: Resource/Entity does not exist.
 *  \retval CL_ERR_INVALID_STATE:  Error: The operation is not valid in the current state.
 *
 *  \par Description:
 *  This function is used to configure attributes for AMS entities.
 *  The entity is identified either by an handle, or if its null, then by
 *  the entity type and name.
 *
 *  \par Library File:
 *
 *  \par Related function(s):
 */
// XXX Need to fix how attributes are indicated in the fn call

extern ClRcT clAmsMgmtEntitySetConfig(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *entity,
        CL_IN       ClAmsEntityConfigT          *entityConfig,
        CL_IN       ClUint32T                   peInstantiateFlag );

extern ClRcT clAmsMgmtSGAssignSUtoSG(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *suEntity,
        CL_IN       const ClAmsEntityT          *sgEntity);

/*
 ************************************
 *  \page pageams121  clAmsMgmtEntityListEntityRefAdd
 *
 *  \par Synopsis:
 *  Adds reference of an entity to another entity's list.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntityListEntityRefAdd(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       ClAmsEntityT                *sourceEntity,
 *                               CL_IN       ClAmsEntityT                *targetEntity,
 *                               CL_IN       ClAmsEntityListTypeT        entityListName);
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param sourceEntity: Entity type and name whose list has to be modified.
 *  \param targetEntity: Entity type and name whose reference has to be added in the list.
 *  \param entityListName: Name of the list in which the target entity reference will be added.
 *
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid
 *  \retval CL_ERR_INVALID_STATE: Error: The operation is not valid in the current state,
 *  \retval CL_ERR_NOT_EXIST: Error: Resource/Entity does not exist.
 *
 *  \par Description:
 *  This function is used to add reference of an entity to another entity's list
 *  The entity is identified either by an handle, or if its null, then by
 *  the entity type and name.
 *
 *  \par Library File:
 *  ClAmsMgmtClient
 *  \par Related function(s):
 *
 *
 */

extern ClRcT clAmsMgmtEntityListEntityRefAdd(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       ClAmsEntityT                *sourceEntity,
        CL_IN       ClAmsEntityT                *targetEntity,
        CL_IN       ClAmsEntityListTypeT        entityListName);

/*
 ************************************
 *  \page pageams123  clAmsMgmtEntitySetRef
 *
 *  \par Synopsis:
 *  Sets a reference of an entity for a given entity.
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtEntitySetRef(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       const ClAmsEntityT          *sourceEntity,
 *                               CL_IN       const ClAmsEntityT          *targetEntity );
 *  \endcode
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param sourceEntity: Entity type and name which needs the reference to be added.
 *  \param targetEntity: Entity type and name whose reference has to be added.
 *
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid
 *  \retval CL_ERR_INVALID_STATE: Error: The operation is not valid in the current state,
 *  \retval CL_ERR_NOT_EXIST: Error: Resource/Entity does not exist.
 *
 *  \par Description:
 *
 *  \par Library File:
 *  ClAmsMgmtClient
 *  \par Related function(s):
 *
 *
 */

extern ClRcT
clAmsMgmtEntitySetRef(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *sourceEntity,
        CL_IN       const ClAmsEntityT          *targetEntity );

/*
 ************************************
 *  \page pageams124  clAmsMgmtCSISetNVP
 *
 *  \par Synopsis:
 *  Adds a name value pair to a CSIs name value pair list
 *
 *  \par Header File:
 *  clAmsMgmtClientIpi.h
 *
 *
 *  \par Syntax:
 *  \code   ClRcT clAmsMgmtCSISetNVP(
 *                               CL_IN       ClAmsMgmtHandleT            amsHandle,
 *                               CL_IN       const ClAmsEntityT          *sourceEntity,
 *                               CL_IN       ClAmsCSINameValuePairT       nvp );
 *  \endcode[-=]=[-]
 *
 *  \param amsHandle: Handle assigned to function user.
 *  \param sourceEntity: Entity type and name of the CSI whose NVP list has to be added with new NVP.
 *  \param nvp: Name value pair which needs to be added to the CSI's NVP list.
 *
 *
 *  \retval CL_OK: The function executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE: Error: Handle was invalid
 *  \retval CL_ERR_INVALID_STATE: Error: The operation is not valid in the current state,
 *  \retval CL_ERR_NOT_EXIST: Error: Resource/Entity does not exist.
 *
 *  \par Description:
 *
 *  \par Library File:
 *  ClAmsMgmtClient
 *  \par Related function(s):
 *
 *
 */

extern ClRcT
clAmsMgmtCSISetNVP(
        CL_IN       ClAmsMgmtHandleT            amsHandle,
        CL_IN       const ClAmsEntityT          *sourceEntity,
        CL_IN       ClAmsCSINameValuePairT       nvp );


#ifdef __cplusplus
}
#endif

#endif // #ifndef _CL_AMS_MGMT_CLIENT_IPI_H_


