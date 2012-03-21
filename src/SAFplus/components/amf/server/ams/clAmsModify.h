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
 * File        : clAmsModify.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains AMS internal definitions.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_MODIFY_H_
#define _CL_AMS_MODIFY_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clOsalApi.h>
#include <clAmsTypes.h>
#include <clAmsDatabase.h>
#include <clAms.h>
#include <clAmsMgmtCommon.h>
#include <clAmsPolicyEngine.h>
#include <clAmsInvocation.h>

/************************************************************************
Function decalration for functions used to modify the database content
************************************************************************/

/* Enable the ams */

extern ClRcT
clAmsEnable(
        CL_IN  ClAmsT  *ams);

/**********************************************************************/

/* Disable the ams */

extern ClRcT
clAmsDisable(
        CL_IN  ClAmsT  *ams );

/**********************************************************************/

/* print the ams */
extern ClRcT
clAmsPrint(
        CL_IN  ClAmsT  *ams );
/**********************************************************************/
extern ClRcT
clAmsEntityGetStatus(
            CL_IN  ClAmsEntityDbT  *entityDb,
            CL_IN  ClAmsEntityT  *entity,
            CL_OUT  ClAmsEntityStatusT  **entityStatus );
/**********************************************************************/

extern ClRcT
clAmsEntitySetConfig(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  const ClAmsEntityT  *entityName,
        CL_IN  ClAmsEntityConfigT  *entityConfig );

extern ClRcT
clAmsEntitySetAlphaFactor(
        CL_IN  ClAmsEntityT *entity,
        CL_IN  ClUint32T alphaFactor );

extern ClRcT
clAmsEntitySetBetaFactor(
        CL_IN  ClAmsEntityT *entity,
        CL_IN  ClUint32T betaFactor );

/**********************************************************************/
extern ClRcT
clAmsEntitySetAdminState(
            CL_IN  ClAmsEntityDbT  *entityDb,
            CL_IN  ClAmsEntityT  *entity,
            CL_IN  ClAmsAdminStateT  adminState );

/**********************************************************************/
extern ClRcT
clAmsEntitySetPresenceStateToRestart(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityRefT  *entityRef,
        CL_IN  ClAmsPresenceStateT  presenceState );
/**********************************************************************/
extern ClRcT
clAmsEntityListGetListContents(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClUint32T  *numNodes,
        CL_INOUT  ClPtrT  *nodeList );

/**********************************************************************/
extern ClRcT
clAmsEntityGetAdminState(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsAdminStateT  *adminState);
/*********************************************************************/

extern ClRcT
clAmsCSISetNVP(
        CL_IN  ClAmsEntityDbT  entityDb,
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsCSINameValuePairT  nvp );

/*********************************************************************/

extern ClRcT
clAmsEntityCompareLists(
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsEntityListT  *dependentList,
        CL_IN  ClAmsEntityListT  *dependenciesList );

/*********************************************************************/

extern ClRcT 
clAmsAddToEntityList(
        ClAmsEntityT  *sourceEntity,
        ClAmsEntityT  *targetEntity,
        ClAmsEntityListTypeT  entityListName );

extern ClRcT 
clAmsAddGetEntityList(
        ClAmsEntityT  *sourceEntity,
        ClAmsEntityT  *targetEntity,
        ClAmsEntityListTypeT  entityListName,
        ClAmsEntityT **ppSourceRef,
        ClAmsEntityT **ppTargetRef);

/*********************************************************************/

extern ClRcT 
clAmsDeleteFromEntityList(
        ClAmsEntityT  *sourceEntity,
        ClAmsEntityT  *targetEntity,
        ClAmsEntityListTypeT  entityListName );

/*********************************************************************/

extern ClBoolT 
clAmsCheckIfRefExist(
        ClAmsEntityT  *sourceEntity,
        ClAmsEntityT  *targetEntity,
        ClAmsEntityListTypeT  entityListName );

/*********************************************************************/

extern ClRcT
clAmsGetEntityListContents(
        ClAmsEntityT  *entity,
        ClAmsEntityListTypeT  entityListName, 
        clAmsMgmtEntityGetEntityListResponseT  *res );

/*********************************************************************/

extern ClRcT 
clAmsIsValidList (
        ClAmsEntityT  *entity,
        ClAmsEntityListTypeT  entityListName );

/*********************************************************************/

extern ClRcT   
clAmsEntitySetRefPtr(
        ClAmsEntityRefT  sourceEntityRef,
        ClAmsEntityRefT  targetEntityRef );

/*********************************************************************/

extern ClRcT   
clAmsEntityUnsetRefPtr(
        ClAmsEntityRefT  sourceEntityRef,
        ClAmsEntityRefT  targetEntityRef );

/*********************************************************************/

extern ClRcT
clAmsCSIAddToPGTrackList(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsCSIPGTrackClientT  *pgTrackClient,
        CL_INOUT  ClAmsPGNotificationBufferT  *notificationBuffer );

/*********************************************************************/

extern ClRcT
clAmsCSIDeleteFromPGTrackList(
        CL_IN  ClAmsEntityDbT  entityDb,
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsCSIPGTrackClientT  *pgTrackClient );

/*********************************************************************/

extern ClRcT
clAmsCSIDispatchPGTrackResponse(
        ClAmsCSIT  *csi,
        ClAmsPGNotificationBufferT  *fullNotification,
        ClAmsPGNotificationBufferT  *changeNotification );

/*********************************************************************/
/*
 * clAmsValidateConfig
 * -------------------
 * Validates ams configuration eneterd through the XML. This function will validate the 
 * configuration and relationships for all the ams entities. 
 * @param
 *   amsDb                      - pointer of an ams database 
 *   mode                       - summary or detailed. Detailed validation
 *                                checks that the entities that are referred
 *                                to by this entity exist. Summary validation
 *                                simply checks that the entity is properly
 *                                configured.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsValidateConfig(
        CL_IN  ClAmsT  *ams, 
        CL_IN  ClUint32T  mode );

/*
 * clAmsCSIPGTrackListPrint 
 * -----------------------
 * Print the contents of the PG track list 
 *
 * @param
 *   cntHandle                - handle to the clovis container
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
clAmsCSIPGTrackListPrint(
        ClCntHandleT  listHandle );

/*
 * clAmsCSIPGTrackListXMLPrint 
 * -----------------------
 * Print the contents of the PG track list in XML format
 *
 * @param
 *   cntHandle                - handle to the clovis container
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

extern ClRcT
clAmsCSIPGTrackListXMLPrint(
        ClCntHandleT  listHandle );
    
/*
 * clAmsCSINVPListPrint
 * -------------------
 * Print the NVP list for a CSI
 *
 * @param
 *   nameValuePairList          - clovis container handle for csi's NVP
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsCSINVPListPrint(
        CL_IN  ClCntHandleT  nameValuePairList );

/*
 * clAmsCSINVPListXMLPrint
 * -------------------
 * Print the NVP list for a CSI in XML format
 *
 * @param
 *   nameValuePairList          - clovis container handle for csi's NVP
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsCSINVPListXMLPrint(
        CL_IN  ClCntHandleT  nameValuePairList );

/*
 * clAmsDebugEnable
 * -------------------
 * Enable logging for a subArea
 *
 * @param
 *  request                     - request attribute identifying the entity,
                                  subArea and log level for logging
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsDebugEnable(
        CL_IN  clAmsMgmtDebugEnableRequestT  *request );

/*
 * clAmsDebugDisable
 * -------------------
 * Disable logging for a subArea
 *
 * @param
 *  request                     - request attribute identifying the entity,
                                  subArea and log level for logging
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsDebugDisable(
        CL_IN  clAmsMgmtDebugDisableRequestT  *request);

/*
 * clAmsDebugGet
 * -------------------
 * Get the logging flags for the entity 
 *
 * @param
 *  entity                      - entity name and type  
 *  response                    - response attribute identifying the entity,
                                  subArea 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsDebugGet(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  clAmsMgmtDebugGetResponseT  *response);


/*
 * clAmsDBSerializeXML
 * ------------
 * Serialize the contents of a AMS DB
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsDBSerializeXML(
        CL_IN  ClAmsDbT  *amsDb);


/*
 * clAmsEntityDBSerializeXML
 * ------------------
 * Serialize the contents of an Entity DB
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsEntityDBSerializeXML(
        CL_IN  ClAmsEntityDbT  *entityDb);


/*
 * clAmsEntitySerializeXML
 * ----------------
 * Serialize the contents of an Entity
 *
 * @param
 *   entity                     - Pointer to an AMS entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsEntitySerializeXML(
        CL_IN  ClAmsEntityT  *entity);
/***************************************************************************
 * clAmsEntitySetStatus
 * -------------------
 * Set the status part of the entity.
 *
 ***************************************************************************/

extern ClRcT
clAmsEntitySetStatus(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  const ClAmsEntityT  *entityName,
        CL_IN  ClAmsEntityStatusT  *entityStatus);

extern ClRcT
clAmsSetEntityTimer(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsEntityTimerT  *entityTimer );

/*
 * clAmsCntGetFirst
 * -----------------------
 * Find the first element of a container 
 *
 * @param
 *   cntHandle                - handle to the clovis container
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */


ClCntDataHandleT clAmsCntGetFirst(
        CL_IN  ClCntHandleT  *cntHandle,
        CL_INOUT  ClCntNodeHandleT  *nodeHandle );

/*
 * clAmsCntGetNext
 * -----------------------
 * Find the next element of a container 
 *
 * @param
 *   cntHandle                - handle to the clovis container
 *   nodeHandle               - current node in the container 
 *
 * @returns
 *   CL_OK                    - Operation successful
 *
 */

ClCntDataHandleT 
clAmsCntGetNext(
        CL_IN  ClCntHandleT  *cntHandle,
        CL_IN  ClCntNodeHandleT  *nodeHandle);


extern ClRcT
clAmsInvocationInstanceXMLize (
        ClAmsInvocationT  *invocationData );

extern ClRcT
clAmsInvocationListXMLize(
        CL_IN  ClCntHandleT  listHandle);

/*
 * clAmsDbStartTimers
 * ------------
 * Start the timers which were running at the time of crash 
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsDbStartTimers(
        CL_IN  ClAmsDbT  *amsDb);

/*
 * clAmsEntityDbStartTimers
 * ------------------
 *
 * Start the timers of the entity which were running at the time of crash 
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsEntityDbStartTimers(
        CL_IN  ClAmsEntityDbT  *entityDb);

/*
 * clAmsEntityStartTimers
 * ----------------
 * Start the timers for an Entity
 *
 * @param
 *   entity                     - Pointer to an AMS entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsEntityStartTimers(
        CL_IN  ClAmsEntityT  *entity);

extern ClRcT
clAmsTimerResetValues (
        CL_INOUT  ClAmsEntityTimerT  *timer);

extern ClRcT   
clAmsXMLizeInvocation (
                       CL_IN  ClAmsT  *ams,
                       CL_IN  ClCharT *invocationName,
                       CL_OUT ClCharT **ppData);

extern ClRcT   
clAmsXMLizeDB (
               CL_IN  ClAmsT  *ams,
               CL_IN  ClCharT *dbName,
               CL_OUT ClCharT **ppData);

extern ClRcT   
clAmsDeXMLizeInvocation (
                         CL_INOUT  ClAmsT  *ams,
                         CL_IN  ClCharT *invocationName);

extern ClRcT   
clAmsDeXMLizeDB (
                 CL_INOUT  ClAmsT  *ams,
                 CL_IN  ClCharT *dbName);

extern ClRcT
clAmsUpdateAlarmHandle(
        CL_INOUT  const ClNameT  *compName,
        CL_IN  ClUint32T  alarmHandle );

extern ClRcT
clAmsReplayAssignCSIInvocation(ClPtrT arg);

extern ClRcT
clAmsGetCSIRemoveInvocations(ClNameT *pNodeName,
                             ClAmsInvocationT **ppInvocations,
                             ClInt32T *pNumInvocations);

extern ClRcT
clAmsCCBOpListInstantiate(
        CL_OUT  ClCntHandleT  *ccbOpListHandle );

extern ClRcT
clAmsCCBOpListTerminate(
        CL_IN  ClCntHandleT  *ccbOpListHandle );
extern void
clAmsCCBDeleteCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data );

/*
 * clAmsCCBAddOperation
 * -------------------
 *
 *  Add a new operation in the operation list corresponding to operation list
 *  for the CCB
 *
 *
 * @param
 *  listHandle                 - handle of the entity list 
 *  element                    - new element to be added to the list
 *  key                        - key for the new element
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


extern ClRcT
clAmsCCBAddOperation(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClPtrT  element,
        CL_IN  ClCntKeyHandleT  key );


/*
 * clAmsCCBGetFirstElement
 * -------------------
 *
 * returns the first operation from the operation list corresponding to CCB
 *
 *
 * @param
 *  listHandle                 - handle of the entity list 
 *
 *  nodeHandle                 - handle corresponding to the first node which
 *                             - is returned to the caller of this function
 *
 * @returns
 *
 * return the first element in the list, returns NULL in case of errors or empty list.
 *
 */


extern ClPtrT
clAmsCCBGetFirstElement(
        CL_IN  ClCntHandleT  listHandle,
        CL_OUT  ClCntNodeHandleT  *nodeHandle );



/*
 * clAmsCCBGetNextElement
 * -------------------
 *
 * returns the next operation from the operation list corresponding to CCB
 *
 *
 * @param
 *  listHandle                 - handle of the entity list 
 *
 *  nodeHandle                 - handle corresponding to the previous node which
 *                             - is used to find the next node in the list
 *
 *  nextNodeHandle             - handle corresponding to the next node in the list
 *                             - which is returned to the caller of this function
 *
 * @returns
 *
 * return the first element in the list, returns NULL in case of errors or empty list.
 *
 */


extern ClPtrT
clAmsCCBGetNextElement(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClCntNodeHandleT  nodeHandle, 
        CL_OUT  ClCntNodeHandleT  *nextNodeHandle );


/***************************************************************************
 * clAmsEntitySetConfigNew 
 * -------------------
 * Set the config part of the entity.
 *
 ***************************************************************************/

extern ClRcT
clAmsEntitySetConfigNew(
        CL_IN  ClAmsEntityConfigT  *entityConfig,
        CL_IN  ClUint64T  bitMask );

/*
 * clAmsCSIGetNVP
 * -------------------
 * Get the CSI NVP list 
 *
 * @param
 *  entity                     - CSI entity name
 *  nvpList                    - Buffer containing the nvp list 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


extern ClRcT
clAmsCSIGetNVP(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsCSINVPBufferT  *nvpList );

/*
 * clAmsCSIDeleteNVP
 * -------------------
 * Delete the CSI NVP from NVP list 
 *
 * @param
 *  entityDB                   - DB for the CSI entity 
 *  entity                     - CSI entity name
 *  nvp                        - Name value pair to be deleted from the CSI's NVP list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


extern ClRcT
clAmsCSIDeleteNVP(
        CL_IN  ClAmsEntityDbT  entityDb,
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsCSINameValuePairT  nvp );

/*
 * clAmsGetEntityList
 * -------------------
 * Get the entity list 
 *
 * @param
 *  entity                     - entity name
 *  entityListName             - Name of the entity list  
 *  entityList                 - Buffer containing the entity list 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


extern ClRcT
clAmsGetEntityList(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsEntityListTypeT  entityListName,
        CL_OUT  ClAmsEntityBufferT  *entityList);

extern ClRcT
clAmsGetEntityListAll(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityTypeT entityType);

/*
 * clAmsGetOLEntityList
 * -------------------
 * Get the overloaded entity list 
 *
 * @param
 *  entity                     - entity name
 *  entityListName             - Name of the entity list  
 *  entityList                 - Buffer containing the entity list 
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


extern ClRcT
clAmsGetOLEntityList(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsEntityListTypeT  entityListName,
        CL_OUT  ClAmsEntityRefBufferT  *entityListBuffer);

typedef struct
{
    ClAmsMgmtCCBOperationsT  opId;
    ClPtrT  *payload;
}clAmsMgmtCCBOperationDataT;

extern ClRcT
clAmsCCBValidateOperation(
        CL_IN  ClPtrT  req,
        CL_IN  ClAmsMgmtCCBOperationsT  opId );

extern ClRcT
clAmsCCBValidateOperationLocked(
        CL_IN  ClPtrT  req,
        CL_IN  ClAmsMgmtCCBOperationsT  opId );

extern ClRcT
clAmsCCBValidateAdminState(
        CL_IN  ClAmsEntityT  *entity );


extern ClRcT
clAmsCheckPendingNodeFailFast(
        CL_IN  ClAmsT  *ams );

extern ClRcT
clAmsInitiateNodeFailFast(
        CL_IN  ClAmsEntityT  *entity );

extern ClRcT
clAmsEntityDeleteRefs(ClAmsEntityRefT *eRef);

extern ClRcT
clAmsFindCSIType(ClAmsCSIT *csi, ClNameT *csiType);

extern ClRcT clAmsSGFailoverHistoryConfigure(ClAmsSGT *sg);

extern ClRcT clAmsSGFailoverHistoryCascade(ClAmsNodeT *node, ClAmsCompT **ppFaultyComp, ClBoolT *pRecover);

extern ClRcT clAmsFailoverHistoryFind(ClAmsEntityT *entity, ClUint32T index, 
                                      ClAmsSGT **ppSG, ClAmsSGFailoverHistoryT **ppHistory);

extern ClRcT clAmsSGFailoverHistoryDelete(ClAmsSGT *sg);

extern ClRcT
clAmsDBGet(ClBufferHandleT msg);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_MODIFY_H_ */
