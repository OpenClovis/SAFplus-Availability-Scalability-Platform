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
 * File        : clAmsDatabase.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains definitions for the configuration and state
 * database maintained by AMS. While this database is internal to AMS, its
 * contents will be fully exposed via the management API. 
 *
 * This file is Clovis internal if the management API is not directly 
 * exposed to a ASP customer. However, if the management API is exposed
 * to a ASP customer, this file is not Clovis internal.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_DATABASE_H_
#define _CL_AMS_DATABASE_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCntApi.h>
#include <clAmsErrors.h>
#include <clAmsTypes.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>

/******************************************************************************
 * AMS DATABASE
 *****************************************************************************/

/*
 * AMS keeps track of all relevant entities in the AMS DB. The requirements
 * for organizing the AMS DB are as follows:
 *
 * (1) The number of entities in a system can change at run-time. A static
 *     allocation of a fixed size table should be avoided.
 * (2) The key used for refering to an AMS entity in AMS client, management 
 *     and event APIs is the entity name. The entity's db must therefore 
 *     support search at least using the node name. Any other keys such as 
 *     "entity id" are optimizations. 
 * (3) It should be possible to access a specific entity directly without a
 *     search as various entities cross reference each other. However, the 
 *     use of pointers is not recommended as it would be good to have the 
 *     AMS db in a form that can be easily made persistant.
 *     
 *
 * The AMS DB is based on Clovis containers. For now, it is set up as a 
 * per entity hash table with entity name as key. 
 *
 * The size of the various databases maintained by AMS is system designer 
 * configurable and fed to AMS via a deployment config file.  This config
 * is used as a guide to select an appropriate internal storage scheme. The
 * size can only be set when AMS is disabled.
 *
 * XXX: 
 *
 * (1) Need to check how to keep track of handles as per SAF API.
 * (2) Need to separate out class and instance definitions, as class 
 *     information comes from deployment whereas instance comes from 
 *     configuration. Currently everthing is in one flat database.
 */
    
typedef struct
{
    ClUint32T                   numDbBuckets;
    ClCntKeyCompareCallbackT    entityKeyCompareCallback;
    ClCntHashCallbackT          entityKeyHashCallback;
    ClCntDeleteCallbackT        entityDeleteCallback;
    ClCntDeleteCallbackT        entityDestroyCallback;
} ClAmsEntityDbParamsT;

#define CL_AMS_ENTITY_DB_NUMBER_BUCKETS   128

#define CL_AMS_ENTITY_DB_CONFIG 0x1
#define CL_AMS_ENTITY_DB_CONFIG_LIST 0x2
#define CL_AMS_ENTITY_DB_STATUS 0x4

#define CL_AMS_ENTITY_DB_CONFIG_ALL (CL_AMS_ENTITY_DB_CONFIG | CL_AMS_ENTITY_DB_CONFIG_LIST)

typedef struct
{
    ClAmsEntityTypeT        type;
    ClBoolT                 isValid;
    ClUint32T               numEntities;
    ClCntHandleT            db;
} ClAmsEntityDbT;

typedef struct
{
    ClAmsEntityDbT          entityDb [ CL_AMS_ENTITY_TYPE_MAX + 1 ];
} ClAmsDbT;


/******************************************************************************
 * AMS DATABASE METHODS
 *****************************************************************************/

/*
 * clAmsDbInstantiate
 * ------------------
 * Instantiate an AMS DB
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct. It is
 *                                assumed that caller allocates memory for
 *                                the struct.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsDbInstantiate(
        CL_IN       ClAmsDbT            *amsDb);

/*
 * clAmsDbTerminate
 * ----------------
 * Terminate an AMS DB
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct. It is assumed
 *                                that the caller will free the db struct if so
 *                                necessary.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsDbTerminate(
        CL_IN       ClAmsDbT            *amsDb);

/*
 * clAmsDbXMLPrint
 * ------------
 * Print out the contents of a AMS DB in XML format
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsDbXMLPrint(
        CL_IN       ClAmsDbT            *amsDb);

/*
 * clAmsDbPrint
 * ------------
 * Print out the contents of a AMS DB
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsDbPrint(
        CL_IN       ClAmsDbT            *amsDb);
    
/*
 * clAmsEntityDbInstantiate
 * ------------------------
 * Instantiate an AMS Entity DB
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db. It is assumed
 *                                that the caller will allocate the db struct 
 *                                if so necessary.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbInstantiate(
        CL_IN       ClAmsEntityDbT      *entityDb,
        CL_IN       ClUint32T           type);

/*
 * clAmsEntityDbTerminate
 * ----------------------
 * Terminate an AMS Entity DB
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db. It is assumed
 *                                that the caller will free the db struct 
 *                                if so necessary.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbTerminate(
        CL_IN       ClAmsEntityDbT      *entityDb);

/*
 * clAmsEntityDbWalk
 * -----------------
 * Walk an AMS Entity DB and call the function passed as argument
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *   fn                         - fn to callback with *entity as argument
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbWalk(
        CL_IN   ClAmsEntityDbT          *entityDb,
        CL_IN   ClAmsEntityCallbackT    fn);

extern ClRcT clAmsEntityDbWalkExtended(
        CL_IN   ClAmsEntityDbT          *entityDb,
        CL_IN   ClAmsEntityCallbackExtendedT    fn,
        CL_IN   ClPtrT userArg);

/*
 * clAmsEntityDbPrint
 * ------------------
 * Print out the contents of an Entity DB
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbPrint(
        CL_IN       ClAmsEntityDbT      *entityDb);

/*
 * clAmsEntityDbXMLPrint
 * ------------------
 * Print out the contents of an Entity DB in XML format
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbXMLPrint(
        CL_IN       ClAmsEntityDbT      *entityDb);

    
/*
 * clAmsEntityDbPrintCallback
 * --------------------------
 * Function called while walking the entity DB
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbPrintCallback(
        CL_IN       ClCntKeyHandleT key,
        CL_IN       ClCntDataHandleT data,
        CL_IN       ClCntArgHandleT arg,
        CL_IN       ClUint32T dataLength);

/*
 * clAmsEntityDbAddEntity
 * ----------------------
 * Instantiate an AMS Entity and add it to the db.
 *
 * @param
 *   entityDb                   - Pointer to a valid entity db
 *   entity                     - Pointer to new entity
 *   entityRef                  - Ref to new entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbAddEntity(
        CL_IN       ClAmsEntityDbT      *entityDb,
        CL_IN       ClAmsEntityRefT     *entityRef);

/*
 * clAmsEntityDbAddEntity2
 * ----------------------
 * Instantiate an AMS Entity
 *
 * @param
 *   entityDb                   - Pointer to a valid entity db
 *   entity                     - Pointer to entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityDbAddEntity2(
        CL_IN       ClAmsEntityDbT          *entityDb,
        CL_IN       ClAmsEntityT            *entity);

/*
 * clAmsEntityDbDeleteEntity
 * -------------------------
 * Terminate/Delete an AMS Entity
 *
 * @param
 *   entityDb                   - Pointer to a valid entity db
 *   entityRef                  - Pointer to an entity ref
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbDeleteEntity(
        CL_IN       ClAmsEntityDbT          *entityDb,
        CL_IN       ClAmsEntityRefT         *entityRef);

/*
 * clAmsEntityDbFindEntity
 * -----------------------
 * Find an entity in a db
 *
 * @param
 *   entityDb                   - Pointer to a valid db
 *   entityRef                  - Entity being searched for
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityDbFindEntity(
        CL_IN       ClAmsEntityDbT      *entityDb,
        CL_INOUT    ClAmsEntityRefT     *entityRef);

/*
 * clAmsEntityListInstantiate
 * --------------------------
 * Instantiate a list of references to AMS entities
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListInstantiate(
        CL_IN       ClAmsEntityListT    *entityList,
        CL_IN       ClUint32T           type);

/*
 * clAmsEntityOrderedListInstantiate
 * --------------------------
 * Instantiate a ordered list of references to AMS entities
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsEntityOrderedListInstantiate(
        CL_IN       ClAmsEntityListT    *entityList,
        CL_IN       ClUint32T           type);

extern ClRcT clAmsCSINVPListInstantiate(
        CL_IN       ClCntHandleT        *nameValuePairList,
        CL_IN       ClUint32T           type);

extern ClRcT clAmsCSIPGTrackListInstantiate(
        CL_IN       ClCntHandleT        *pgTrackList,
        CL_IN       ClUint32T           type);

/*
 * clAmsEntityListTerminate
 * ------------------------
 * Terminate/Delete an AMS entities list
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListTerminate(
        CL_IN       ClAmsEntityListT    *entityList);

/*
 * clAmsEntityListAddEntityRef
 * ---------------------------
 * Add a entity reference to a list. It is assumed that the entityRef is
 * fully configured. The key is passed as argument, if null, the fn uses
 * the entity name as key.
 *
 * @param
 *   entityList                 - Pointer to a valid entity list
 *   targetRef                  - The entityRef structure that is added to the
 *                                list and which references an entity in the
 *                                entityDb.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListAddEntityRef(
        CL_IN       ClAmsEntityListT        *entityList,
        CL_IN       ClAmsEntityRefT         *entityRef,
        CL_IN       ClCntKeyHandleT         entityKeyHandle);

/*
 * clAmsEntityListDeleteEntityRef
 * ------------------------------
 * Terminate/Delete an entity reference from a list
 *
 * @param
 *   entityList                 - Pointer to a valid entity list
 *   entityRef                  - Pointer to a valid entity reference
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListDeleteEntityRef(
        CL_IN       ClAmsEntityListT        *entityList,
        CL_IN       ClAmsEntityRefT         *entityRef,
        CL_IN       ClCntKeyHandleT         entityKeyHandle);

extern void clAmsEntityListDeleteCallback(
        ClCntKeyHandleT key,
        ClCntDataHandleT data);

/*
 * clAmsEntityListFindEntityRef
 * ----------------------------
 * Find an entity reference in a list
 *
 * @param
 *   entityList                 - Pointer to a valid list
 *   entityRef                  - Entity being searched for
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListFindEntityRef(
        CL_IN       ClAmsEntityListT        *entityList,
        CL_IN       const ClAmsEntityRefT         *entityRef,
        CL_IN       ClCntKeyHandleT         entityKeyHandle,
        CL_IN       ClAmsEntityRefT         **foundRef);


/*
 * clAmsEntityListFindEntityRef2
 * -----------------------------
 * Find an entity reference in a list given the entity ptr. This is a
 * wrapper over the above function, so caller does not have to create
 * a entityRef.
 *
 * @param
 *   entityList                 - Pointer to a valid list
 *   entityRef                  - Entity being searched for
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListFindEntityRef2(
        CL_IN       ClAmsEntityListT        *entityList,
        CL_IN       ClAmsEntityT            *entity,
        CL_IN       ClCntKeyHandleT         entityKeyHandle,
        CL_OUT      ClAmsEntityRefT         **foundRef);

/*
 * clAmsEntityListWalk
 * -------------------
 * Walk the contents of an Entity list and call the fn passing
 * the entityRef as argument.
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *   fn                         - fn to call (arg is entityRef)
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListWalk(
        CL_IN       ClAmsEntityListT       *entityList,
        CL_IN       ClAmsEntityRefCallbackT   fn,
        CL_IN       ClAmsEntityListTypeT    entityListName);

/*
 * clAmsEntityListWalkGetEntity
 * ----------------------------
 * Walk the contents of an Entity list and call the fn passing
 * the entityRef->ptr as argument.
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *   fn                         - fn to call (arg is entity)
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListWalkGetEntity(
        CL_IN       ClAmsEntityListT       *entityList,
        CL_IN       ClAmsEntityCallbackT   fn);

/*
 * clAmsEntityListGetFirst
 * -----------------------
 * Find the first element of a list
 *
 */

extern ClAmsEntityRefT * clAmsEntityListGetFirst(
        CL_IN       ClAmsEntityListT *entityList);

/*
 * clAmsEntityListGetNext
 * ----------------------
 * Find the next element of a list
 *
 */

extern ClAmsEntityRefT * clAmsEntityListGetNext(
        CL_IN       ClAmsEntityListT *entityList,
        CL_IN       ClAmsEntityRefT  *currentRef);

/*
 * clAmsEntityListGetLast
 * -----------------------
 * Find the last element of a list
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClAmsEntityRefT *
clAmsEntityListGetLast(
        CL_IN       ClAmsEntityListT *entityList);

/*
 * clAmsEntityListGetPrevious
 * -----------------------
 * Find the previous element of a list
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClAmsEntityRefT *
clAmsEntityListGetPrevious(
        CL_IN       ClAmsEntityListT *entityList,
        CL_IN       ClAmsEntityRefT  *entityRef);

/*
 * clAmsEntityListPrint
 * --------------------
 * Print out the contents of an Entity list
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListPrint(
        CL_IN       ClAmsEntityListT          *entityList,
        CL_IN       ClAmsEntityListTypeT      entityListName);

/*
 * clAmsEntityListXMLPrint
 * --------------------
 * Print out the contents of an Entity list in XML format
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListXMLPrint(
        CL_IN       ClAmsEntityListT          *entityList,
        CL_IN       ClAmsEntityListTypeT      entityListName);
    
/*
 * clAmsEntityListPrintCallback
 * ----------------------------
 * Function called while walking the entity list to print it.
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListPrintCallback(
        CL_IN       ClAmsEntityRefT     *entityRef,
        CL_IN       ClAmsEntityListTypeT entityListName);

/*
 * clAmsEntityListXMLPrintCallback
 * ----------------------------
 * Function called while walking the entity list to print it
 * in XML format
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListXMLPrintCallback(
        CL_IN       ClAmsEntityRefT     *entityRef,
        CL_IN       ClAmsEntityListTypeT entityListName);
    
/*
 * clAmsEntityListPrintEntityRef
 * -----------------------------
 * Print out the contents of an entity reference
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListPrintEntityRef(
        CL_IN       ClAmsEntityRefT     *entityRef, 
        CL_IN       ClUint32T           mode,
        CL_IN       ClAmsEntityListTypeT    entityListName);

/*
 * clAmsEntityListXMLPrintEntityRef
 * -----------------------------
 * Print out the contents of an entity reference in XML format
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityListXMLPrintEntityRef(
        CL_IN       ClAmsEntityRefT     *entityRef, 
        CL_IN       ClUint32T           mode,
        CL_IN       ClAmsEntityListTypeT    entityListName);
    
/*
 * clAmsPrintHAState 
 * -----------------------------
 * Print out the HA state of a SI for a given SU
 *
 * @param
 * haState                      - HA state of the SI for the SU
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsPrintHAState(
        CL_IN      ClAmsHAStateT        haState );

/*
 * clAmsXMLPrintHAState 
 * -----------------------------
 * Print out the HA state of a SI for a given SU in XML format
 *
 * @param
 * haState                      - HA state of the SI for the SU
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT
clAmsXMLPrintHAState(
        CL_IN      ClAmsHAStateT        haState );


/******************************************************************************
 * Service Group Functions
 *****************************************************************************/

extern ClRcT clAmsSGAddSURefToSUList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSUT     *su);

extern ClRcT clAmsSGDeleteSURefFromSUList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSUT     *su);

extern ClRcT clAmsSGAddSIRefToSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSIT     *si);

extern ClRcT clAmsSGDeleteSIRefFromSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSIT     *si);

/******************************************************************************
 * Service Unit Functions
 *****************************************************************************/

extern ClRcT clAmsSUAddSIRefToSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSIT     *si,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsSUDeleteSIRefFromSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSIT     *si);

/******************************************************************************
 * Service Instance Functions
 *****************************************************************************/

extern ClRcT clAmsSIAddCSIRefToCSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCSIT    *csi);

extern ClRcT clAmsSIDeleteCSIRefFromCSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCSIT    *csi);

extern ClRcT clAmsSIAddSURefToSUList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSUT    *su,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsSIDeleteSURefFromSUList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsSUT    *su);

/******************************************************************************
 * Component Functions
 *****************************************************************************/

extern ClRcT clAmsCompAddCSIRefToCSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsHAStateT haState,
        CL_IN   ClAmsCSITransitionDescriptorT tdescriptor);

extern ClRcT clAmsCompDeleteCSIRefFromCSIList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCSIT *csi);

/******************************************************************************
 * Component Service Instance Functions
 *****************************************************************************/

extern ClRcT clAmsCSIAddCompRefToPGList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT haState);

extern ClRcT clAmsCSIDeleteCompRefFromPGList(
        CL_IN   ClAmsEntityListT *entityList,
        CL_IN   ClAmsCompT *comp);

/******************************************************************************
 * Support functions for Clovis Containers
 *****************************************************************************/

ClInt32T clAmsEntityDbEntityKeyCompareCallback(
            ClCntKeyHandleT key1,
            ClCntKeyHandleT key2);

ClUint32T clAmsEntityDbEntityKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbNodeKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbAppKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbSGKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbSUKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbSUKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbSIKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbCompKeyHashCallback(
            ClCntKeyHandleT key);

ClUint32T clAmsEntityDbCSIKeyHashCallback(
            ClCntKeyHandleT key);

void clAmsEntityDbDeleteEntityCallback(
            ClCntKeyHandleT Key,
            ClCntDataHandleT);

void clAmsEntityDbDestroyEntityCallback(
            ClCntKeyHandleT Key,
            ClCntDataHandleT);

extern ClRcT clAmsEntityDbWalkCallback(
            ClCntKeyHandleT key,
            ClCntDataHandleT data,
            ClCntArgHandleT arg,
            ClUint32T dataLength);

extern ClRcT
clAmsAuditDb(ClAmsNodeT *pThisNode);

extern ClRcT
clAmsAuditDbEpilogue(ClNameT *nodeName, ClBoolT scFailover);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_DATABASE_H_ */
