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
 * File        : clAmsDatabase.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements the AMS database manipulation methods prototyped in
 * clAmsDatabase.h. These methods are AMS internal and invoked typically by
 * functions in the AMS Management API.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clOsalApi.h>
#include <clCksmApi.h>
#include <clIocIpi.h>
#include <clAmsDatabase.h>
#include <clAmsPolicyEngine.h>
#include <clAmsServerUtils.h>
#include <clAmsModify.h>
#include <clAmsInvocation.h>
#include <clAmsCkpt.h>
#include <clCpmCommon.h>
#include <clCpmInternal.h>
#include <clDebugApi.h>
/******************************************************************************
 * Externs
 *****************************************************************************/

extern ClAmsEntityParamsT gClAmsEntityParams[];

/******************************************************************************
 * Attributes for each AMS entity database
 *****************************************************************************/

ClAmsEntityDbParamsT    gClAmsEntityDbParams[] =
{
    /* -- Entity */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbEntityKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- Node */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbNodeKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- Application */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbAppKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- SG */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbSGKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- SU */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbSUKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- SI */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbSIKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- Component */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbCompKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    },
    /* -- CSI */
    {
        CL_AMS_ENTITY_DB_NUMBER_BUCKETS,
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsEntityDbCSIKeyHashCallback,
        clAmsEntityDbDeleteEntityCallback,
        clAmsEntityDbDestroyEntityCallback,
    }
};

/******************************************************************************
 * AMS Database Methods
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

ClRcT
clAmsDbInstantiate(
        CL_IN  ClAmsDbT  *amsDb)
{

    ClUint32T i = 0;
    AMS_CHECKPTR ( !amsDb );

    for (i=0; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {
        AMS_CALL ( clAmsEntityDbInstantiate(&amsDb->entityDb[i], i) );
    }

    return CL_OK;

}

/*
 * clAmsDbTerminate
 * ----------------
 * Terminate an AMS DB
 *
 * @param
 *   amsDb                      - Pointer to an AMS database struct. It is 
 *                                assumed that the caller will free the db 
 *                                struct if so necessary.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsDbTerminate(
        CL_IN  ClAmsDbT  *amsDb)
{

    ClInt32T i = 0;
    AMS_CHECKPTR ( !amsDb );

    for ( i=CL_AMS_ENTITY_TYPE_MAX; i >= 0; i--)
    {
        if ( amsDb->entityDb[i].db )
        {
            AMS_CALL ( clAmsEntityDbTerminate(&amsDb->entityDb[i]) );
        }
    }

    return CL_OK;

}

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

ClRcT
clAmsDbPrint(
        CL_IN  ClAmsDbT  *amsDb)
{

    ClUint32T i = 0;
    ClAmsT *ams = &gAms;

    AMS_CHECKPTR ( !amsDb );

    CL_AMS_PRINT_TWO_COL("AMF", "%s","");

    CL_AMS_PRINT_DELIMITER();

    CL_AMS_PRINT_TWO_COL("Enabled", "%s",CL_AMS_STRING_BOOLEAN(ams->isEnabled));

    CL_AMS_PRINT_TWO_COL("Service State",
            "%s",CL_AMS_STRING_SERVICE_STATE(ams->serviceState));

    CL_AMS_PRINT_TWO_COL("Start Mode",
            "%s",CL_AMS_STRING_INSTANTIATE_MODE(ams->mode));

    /*
    CL_AMS_PRINT_TWO_COL("Cluster Start Uptime",
            "%u",ams->epoch); // XXX macro to print uptime
    CL_AMS_PRINT_TWO_COL("Server Start Uptime",
            "%u",ams->epoch); // XXX macro to print uptime
    */

    CL_AMS_PRINT_TWO_COL("Timers Running", "%u",ams->timerCount);

    CL_AMS_PRINT_TWO_COL("Debug Flags", "0x%x",ams->debugFlags);

    CL_AMS_PRINT_EMPTY_LINE();

    for (i=1; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {
        AMS_CALL ( clAmsEntityDbPrint(&amsDb->entityDb[i]) );
    }

    return CL_OK;

}

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

ClRcT
clAmsDbXMLPrint(
        CL_IN  ClAmsDbT  *amsDb)
{

    ClUint32T i = 0;
    ClAmsT *ams = &gAms;

    AMS_CHECKPTR ( !amsDb );

    CL_AMS_PRINT_OPEN_TAG("ams");

    CL_AMS_PRINT_TAG_ATTR("enabled", "%s",
                          CL_AMS_STRING_BOOLEAN(ams->isEnabled));

    CL_AMS_PRINT_TAG_ATTR("service_state", "%s",
                          CL_AMS_STRING_SERVICE_STATE(ams->serviceState));

    CL_AMS_PRINT_TAG_ATTR("start_mode", "%s",
                          CL_AMS_STRING_INSTANTIATE_MODE(ams->mode));

    CL_AMS_PRINT_TAG_ATTR("timers_running", "%u", ams->timerCount);
    
    CL_AMS_PRINT_TAG_ATTR("debug_flags", "0x%x", ams->debugFlags);
    
    CL_AMS_PRINT_OPEN_TAG("entities");
    
    for (i=1; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {
        ClCharT entity_tag[CL_MAX_NAME_LENGTH] = {0};

        strcpy(entity_tag, CL_AMS_STRING_ENTITY_TYPE(i));
        strcat(entity_tag, "s");
        
        CL_AMS_PRINT_OPEN_TAG(entity_tag);

        AMS_CALL ( clAmsEntityDbXMLPrint(&amsDb->entityDb[i]) );

        CL_AMS_PRINT_CLOSE_TAG(entity_tag);

    }

    CL_AMS_PRINT_CLOSE_TAG("entities");
    
    CL_AMS_PRINT_CLOSE_TAG("ams");
    
    return CL_OK;

}

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

ClRcT
clAmsEntityDbInstantiate(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClUint32T  type)
{

    ClRcT rc = CL_OK;

    AMS_CHECKPTR ( !entityDb );

    AMS_CHECK_ENTITY_TYPE (type);

    if ( (rc = clCntHashtblCreate(
                    gClAmsEntityDbParams[type].numDbBuckets,
                    gClAmsEntityDbParams[type].entityKeyCompareCallback,
                    gClAmsEntityDbParams[type].entityKeyHashCallback,
                    gClAmsEntityDbParams[type].entityDeleteCallback,
                    gClAmsEntityDbParams[type].entityDestroyCallback,
                    CL_CNT_UNIQUE_KEY,
                    &entityDb->db))
            == CL_OK )
    {
        entityDb->isValid = CL_TRUE;
        entityDb->type = type;
        entityDb->numEntities = 0;
    }

    return rc;

}

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

ClRcT
clAmsEntityDbWalk(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClRcT  (*fn)(ClAmsEntityT *) )
{

    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  nodeHandle = 0; 
    ClAmsEntityT  *entity = NULL;
    ClAmsEntityT  *nextEntity = NULL;

    AMS_CHECKPTR ( !entityDb || !fn );

    entity = (ClAmsEntityT *) clAmsCntGetFirst( &entityDb->db,&nodeHandle);

    while ( entity != NULL )
    {

        nextEntity = (ClAmsEntityT *) clAmsCntGetNext(
                &entityDb->db,&nodeHandle);

        rc = fn(entity);

        if ( rc != CL_OK )
        {
            return rc;
        }

        entity = nextEntity;

    }

    return CL_OK;

}

ClRcT
clAmsEntityDbWalkExtended(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClRcT  (*fn)(ClAmsEntityT *, ClPtrT userArg),
        CL_IN  ClPtrT userArg)
{

    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  nodeHandle = 0; 
    ClAmsEntityT  *entity = NULL;
    ClAmsEntityT  *nextEntity = NULL;

    AMS_CHECKPTR ( !entityDb || !fn );

    entity = (ClAmsEntityT *) clAmsCntGetFirst( &entityDb->db,&nodeHandle);

    while ( entity != NULL )
    {

        nextEntity = (ClAmsEntityT *) clAmsCntGetNext(
                &entityDb->db,&nodeHandle);

        rc = fn(entity, userArg);

        if ( rc != CL_OK )
        {
            return rc;
        }

        entity = nextEntity;

    }

    return CL_OK;

}

/*
 * clAmsEntityDbTerminate
 * ------------------
 * Terminate the contents of an Entity DB
 *
 * @param
 *   entityDb                   - Pointer to an AMS entity db.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityDbTerminate(
        CL_IN  ClAmsEntityDbT  *entityDb )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityDb );

    /*
     * First mark the db as not in use..
     */

    entityDb->isValid       = CL_FALSE;
    entityDb->type          = CL_AMS_ENTITY_TYPE_ENTITY;
    entityDb->numEntities   = 0;

    /*
     * then delete each node in the hash table.
     */

    if ( entityDb->db )
    {
        AMS_CALL ( clCntAllNodesDelete(entityDb->db) );
    }

    /*
     * Then delete the hash table container.
     */

    if ( entityDb->db )
    {
        AMS_CALL ( clCntDelete(entityDb->db) );
    }

    entityDb->db = 0;

    return rc;

}


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

ClRcT
clAmsEntityDbPrint(
        CL_IN  ClAmsEntityDbT  *entityDb)
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityDb );

    // RC2: Remove this when application support is added

#ifndef POST_RC2

    if ( entityDb->type == CL_AMS_ENTITY_TYPE_APP )
    {
        return CL_OK;
    }

#endif

    CL_AMS_PRINT_TWO_COL("Entity db of type",
            "%s",gClAmsEntityParams[entityDb->type].typeString.value);
    CL_AMS_PRINT_DELIMITER();

    if ( entityDb->numEntities )
    {
        rc = clAmsEntityDbWalk(entityDb, clAmsEntityPrint); 
    }
    else
    {
        CL_AMS_PRINT_TWO_COL("   <None>","%s","");
        CL_AMS_PRINT_DELIMITER();
    }

    CL_AMS_PRINT_EMPTY_LINE();

    return rc;

}

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

ClRcT
clAmsEntityDbXMLPrint(
        CL_IN  ClAmsEntityDbT  *entityDb)
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityDb );

    // RC2: Remove this when application support is added

#ifndef POST_RC2

    if ( entityDb->type == CL_AMS_ENTITY_TYPE_APP )
    {
        return CL_OK;
    }

#endif

    if ( entityDb->numEntities )
    {
        rc = clAmsEntityDbWalk(entityDb, clAmsEntityXMLPrint); 
    }
#if 0
    else
    {
        CL_AMS_PRINT_TWO_COL("   <None>","%s","");
        CL_AMS_PRINT_DELIMITER();
    }
#endif

    return rc;

}

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

ClRcT
clAmsEntityDbPrintCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data,
        CL_IN  ClCntArgHandleT  arg,
        CL_IN  ClUint32T  dataLength )
{

    ClAmsEntityT  *entity = (ClAmsEntityT *) data;

    AMS_CHECKPTR ( !entity );

    return clAmsEntityPrint(entity);

}

// TODO: both the add and delete functions should just take entity as
// argument. actual malloc should take place outside the add fn.

/*
 * clAmsEntityDbAddEntity
 * ----------------------
 * Instantiate an AMS Entity
 *
 * @param
 *   entityDb                   - Pointer to a valid entity db
 *   entityRef                  - Pointer to entity reference, used as key
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityDbAddEntity(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityRefT  *entityRef )
{

    ClAmsEntityT  *newEntity = NULL;
    ClCntKeyHandleT  entityKeyHandle = 0;

    /*
     * Verify input parameters and get the key for the entity to be
     * added to the db.
     */
 
    AMS_CHECKPTR ( !entityDb || !entityRef );

    if ( ( entityDb->type  != entityRef->entity.type ) ||
         ( entityDb->type < 0 ) ||
         ( entityDb->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    entityKeyHandle = clAmsEntityGetKey(&entityRef->entity);

    /*
     * Allocate a new entity with default values and initialize it appropriately.
     */ 
    
    newEntity = clAmsEntityMalloc( entityRef->entity.type, NULL, NULL );

    AMS_CHECK_NO_MEMORY (newEntity);

    memcpy(newEntity->name.value,
            entityRef->entity.name.value,
            entityRef->entity.name.length);
    newEntity->name.length = entityRef->entity.name.length;

    /*
     * Add the new entity node to the hash table and update
     * the entity db.
     */

    int err = clCntNodeAdd(
                entityDb->db,
                entityKeyHandle, 
                (ClCntDataHandleT) newEntity,
                NULL);
    if (err != CL_OK)
      {
        if (CL_GET_ERROR_CODE(err) == CL_ERR_DUPLICATE)
          {
            clDbgRootCauseError(err, ("Duplicate entry %s in AMS configuration",entityRef->entity.name.value));
          }
        clAmsEntityFree(newEntity);
        return err;
      }

    entityDb->numEntities ++;
    entityRef->ptr = newEntity;

    return CL_OK;

}

/*
 * clAmsEntityDbDeleteEntity
 * -------------------------
 * Terminate/Delete an AMS Entity. Note, any references to this entity made by
 * other entities are not removed by this function and as such it can result
 * in dangling references. 
 *
 * @param
 *   entityDb                   - Pointer to a valid entity db
 *   entityRef                  - Pointer to an entity ref
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityDbDeleteEntity(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityRefT  *entityRef )
{
    ClRcT rc;
    /*
     * Verify the entity db and reference are valid and referring to the
     * same type of entities. Then verify the entity exits in the db.
     */

    AMS_CHECKPTR ( !entityDb || !entityRef );

    if ( ( entityDb->type != entityRef->entity.type ) ||
         ( entityDb->type < 0 ) ||
         ( entityDb->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    rc = clAmsEntityDbFindEntity(entityDb,entityRef);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) return rc;
    else AMS_CALL(rc);

    /*
     * Delete the entity node from the hash table and update
     * the entity db. 
     */

    AMS_CALL ( clCntNodeDelete( entityDb->db, entityRef->nodeHandle) );

    entityDb->numEntities--;
   
    return CL_OK;

}

/*
 * clAmsEntityDbDeleteEntityCallback
 * ---------------------------------
 * Called when an entry is about to be deleted
 *
 * @param
 *   key                        - key for entity
 *   data                       - handle to data in node
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

void
clAmsEntityDbDeleteEntityCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data )
{
    clAmsEntityFree((ClAmsEntityT *)data);
}

/*
 * clAmsEntityDbDestroyEntityCallback
 * ----------------------------------
 * Called when an entry is about to be destroyed
 *
 * @param
 *   key                        - key for entity
 *   data                       - handle to data in node
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

void
clAmsEntityDbDestroyEntityCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data )
{
    clAmsEntityFree((ClAmsEntityT *)data);
}

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

ClRcT
clAmsEntityDbFindEntity(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_INOUT  ClAmsEntityRefT  *entityRef )
{

    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;
    ClCntNodeHandleT  entityNodeHandle = 0;
    ClAmsEntityT  *foundEntity = NULL;
    ClUint32T  numNodes = 0;

    /*
     * Verify the entity db and reference are valid and referring to the
     * same type of entities. Then verify the entity exits in the db.
     */

    AMS_CHECKPTR ( !entityDb || !entityRef );

    if ( ( entityDb->type != entityRef->entity.type ) ||
         ( entityDb->type < 0 ) ||
         ( entityDb->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }


    entityKeyHandle = clAmsEntityGetKey( &entityRef->entity);


    rc = clCntNodeFind( entityDb->db, entityKeyHandle, &entityNodeHandle);

    if ( rc != CL_OK )
    {
        return rc;
    }

    AMS_CALL ( clCntKeySizeGet( entityDb->db, entityKeyHandle, &numNodes) );

    entityRef->ptr = NULL;
    entityRef->nodeHandle = entityNodeHandle;

    clCntNodeUserDataGet(
            entityDb->db,
            entityNodeHandle,
            (ClCntDataHandleT *) &foundEntity);

    while ( (foundEntity != NULL) && (numNodes) )
    {

        if ( !memcmp(entityRef->entity.name.value,
                    foundEntity->name.value,
                    entityRef->entity.name.length) )
        {
            entityRef->ptr = foundEntity;
            entityRef->nodeHandle = entityNodeHandle;
            return CL_OK;
        }

        foundEntity = (ClAmsEntityT *) clAmsCntGetNext(
                &entityDb->db,&entityNodeHandle );

        numNodes--;

    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);

}

/******************************************************************************
 * Entity List Methods
 *****************************************************************************/

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

ClRcT
clAmsEntityListInstantiate(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClUint32T  type )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityList );

    AMS_CHECK_ENTITY_TYPE (type);

    if ( (rc = clCntLlistCreate(
                    gClAmsEntityDbParams[type].entityKeyCompareCallback,
                    clAmsEntityListDeleteCallback,
                    gClAmsEntityDbParams[type].entityDestroyCallback,
                    CL_CNT_NON_UNIQUE_KEY,
                    &entityList->list)) 
            == CL_OK )
    {
        entityList->isRankedList= CL_FALSE;
        entityList->isValid     = CL_TRUE;
        entityList->type        = type;
        entityList->numEntities = 0;
    }

    return rc;

}

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

ClRcT
clAmsEntityOrderedListInstantiate(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClUint32T  type )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityList );

    AMS_CHECK_ENTITY_TYPE (type);

#ifdef CL_AMS_RANK_LIST

    if ( (rc = clCntOrderedLlistCreate(
                    gClAmsEntityDbParams[type].entityKeyCompareCallback,
                    clAmsEntityListDeleteCallback,
                    gClAmsEntityDbParams[type].entityDestroyCallback,
                    CL_CNT_NON_UNIQUE_KEY,
                    &entityList->list)) 
            == CL_OK )
    {
        entityList->isRankedList= CL_TRUE;
        entityList->isValid     = CL_TRUE;
        entityList->type        = type;
        entityList->numEntities = 0;
    }

#else

    if ( (rc = clCntLlistCreate(
                    gClAmsEntityDbParams[type].entityKeyCompareCallback,
                    clAmsEntityListDeleteCallback,
                    gClAmsEntityDbParams[type].entityDestroyCallback,
                    CL_CNT_NON_UNIQUE_KEY,
                    &entityList->list)) 
            == CL_OK )
    {
        entityList->isRankedList= CL_FALSE;
        entityList->isValid     = CL_TRUE;
        entityList->type        = type;
        entityList->numEntities = 0;
    }

#endif

    return rc;

}

/*
 * clAmsEntityNVPListInstantiate
 * --------------------------
 * Instantiate a list of references to AMS entities
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsCSINVPListInstantiate(
        CL_IN  ClCntHandleT  *nameValuePairList, 
        CL_IN  ClUint32T  type )
{

    AMS_CHECK_ENTITY_TYPE (type);

    AMS_CHECKPTR (!nameValuePairList);

    return clCntLlistCreate(
            gClAmsEntityDbParams[type].entityKeyCompareCallback,
            clAmsEntityListDeleteCallback,
            gClAmsEntityDbParams[type].entityDestroyCallback,
            CL_CNT_UNIQUE_KEY,
            nameValuePairList) ;

}


/*
 * clAmsCSIPGTrackListInstantiate 
 * --------------------------
 * Instantiate a list of references to AMS entities
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsCSIPGTrackListInstantiate(
        CL_IN  ClCntHandleT  *pgTrackList, 
        CL_IN  ClUint32T  type )
{

    AMS_CHECK_ENTITY_TYPE (type);

    AMS_CHECKPTR (!pgTrackList);

    return clCntLlistCreate(
            gClAmsEntityDbParams[type].entityKeyCompareCallback,
            clAmsEntityListDeleteCallback,
            gClAmsEntityDbParams[type].entityDestroyCallback,
            CL_CNT_NON_UNIQUE_KEY,
            pgTrackList) ;

}

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
        CL_IN  ClAmsEntityListT  *entityList )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !entityList );

    /*
     * First delete each node in the list
     */ 

    if ( entityList->numEntities )
    {
        AMS_CALL (clCntAllNodesDelete(entityList->list)) ;
    }

    /*
     * Then delete the list container
     */ 
    
    clCntDelete(entityList->list);

    entityList->isValid     = CL_FALSE;
    entityList->type        = CL_AMS_ENTITY_TYPE_ENTITY;
    entityList->numEntities = 0;
    entityList->list        = 0;

    return rc;

}

/*
 * clAmsEntityListAddEntityRef
 * ---------------------------
 * Add a entity reference to a list. It is assumed that the entityRef is
 * fully configured.
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

ClRcT
clAmsEntityListAddEntityRef(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityRefT  *entityRef,
        CL_IN  ClCntKeyHandleT  entityKeyHandle )
{

    /*
     * Verify input parameters. The type of the targetRef, the
     * entityList and the entityDb should be the same. The entity 
     * to which a reference is being made should exist.
     */

    AMS_CHECKPTR ( !entityList || !entityRef );

    if ( ( entityList->type != entityRef->entity.type ) ||
         ( entityList->type < 0 )  ||
         ( entityList->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    /*
     * Compute a key for the reference.
     */

    if ( !entityKeyHandle )
    {
        entityKeyHandle = clAmsEntityGetKey( &entityRef->entity);
    }

    /*
     * Add it to the container.
     */ 
    
    AMS_CALL( clCntNodeAddAndNodeGet(
                entityList->list,
                entityKeyHandle, 
                (ClCntDataHandleT) entityRef,
                NULL,
                &entityRef->nodeHandle) );

    entityList->numEntities ++;

    return CL_OK;

}

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

ClRcT
clAmsEntityListDeleteEntityRef(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityRefT  *entityRef,
        CL_IN  ClCntKeyHandleT  entityKeyHandle)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  *foundRef = NULL;

    AMS_CHECKPTR ( !entityList || !entityRef );

    if ( ( entityList->type != entityRef->entity.type ) ||
         ( entityList->type < 0 ) ||
         ( entityList->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    rc =  clAmsEntityListFindEntityRef( 
            entityList,
            entityRef,
            entityKeyHandle,
            &foundRef) ; 

    if ( rc != CL_OK )
    {
        return rc;
    }

    AMS_CHECKPTR (!foundRef);
    
    AMS_CALL ( clCntNodeDelete( entityList->list, foundRef->nodeHandle) );
    
    entityList->numEntities--;
   
    return CL_OK;

}

void
clAmsEntityListDeleteCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data )
{
    clHeapFree((void *)data);
}

/*
 * clAmsEntityListFindEntityRef
 * ----------------------------
 * Find an entity reference in a list. Note, this function only returns
 * a reference structure if one exists in a list, it does not check if
 * the reference is still valid or dangling.
 *
 * @param
 *   entityList                 - Pointer to a valid list
 *   entityRef                  - Entity being searched for
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListFindEntityRef(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  const ClAmsEntityRefT  *entityRef,
        CL_IN  ClCntKeyHandleT  entityKeyHandle,
        CL_OUT  ClAmsEntityRefT  **foundRef )
{
   
    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  entityNodeHandle = 0;
    ClUint32T  numNodes = 0;

    /*
     * Verify the entity list and reference are valid and referring to the
     * same type of entities. Then verify the entity exits in the list.
     */

    AMS_CHECKPTR ( !entityList || !entityRef || !foundRef );
 
    if ( ( entityList->type !=  entityRef->entity.type ) ||
         ( entityList->type < 0 ) ||
         ( entityList->type > CL_AMS_ENTITY_TYPE_MAX ) )
    {
        clDbgCodeError(CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY),("Passed EntityList parameter is invalid"));
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    *foundRef = NULL;

    /*
     * Compute a key for the reference.
     */

    if ( !entityKeyHandle )
    {
        entityKeyHandle = clAmsEntityGetKey( &entityRef->entity);
    } 
    
    rc = clCntNodeFind( entityList->list, entityKeyHandle, &entityNodeHandle);

    if ( rc != CL_OK )
    {
        return rc;
    }

    AMS_CALL ( clCntKeySizeGet( entityList->list, entityKeyHandle, &numNodes) );

    clCntNodeUserDataGet(
            entityList->list,
            entityNodeHandle,
            (ClCntDataHandleT *)foundRef);

    while ( ((*foundRef) != NULL) && (numNodes) )
    {
        
        if ( !memcmp(entityRef->entity.name.value,
                    (*foundRef)->entity.name.value, 
                    entityRef->entity.name.length) )
        {
            (*foundRef)->nodeHandle = entityNodeHandle;
            return CL_OK;
        }

        (*foundRef) = (ClAmsEntityRefT *) clAmsCntGetNext(
                &entityList->list,&entityNodeHandle );
        numNodes--;

    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);

}

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

ClRcT
clAmsEntityListFindEntityRef2(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClCntKeyHandleT  entityKeyHandle,
        CL_OUT  ClAmsEntityRefT  **foundRef )
{

    ClAmsEntityRefT entityRef = {{0},0,0};

    AMS_CHECKPTR (!entityList || !entity || !foundRef);

    memcpy(&entityRef.entity, entity, sizeof(ClAmsEntityT));

    return clAmsEntityListFindEntityRef(
            entityList,
            &entityRef,
            entityKeyHandle,
            foundRef);
}

/*
 * clAmsEntityListWalk
 * -------------------
 * Walk the contents of an Entity list and call the fn
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *   fn                         - fn to call (arg is entityRef)
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListWalk(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityRefCallbackT  fn,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *entityRef = NULL;
    ClAmsEntityRefT  *nextEntityRef = NULL;

    AMS_CHECKPTR ( !entityList || !fn );

    entityRef = (ClAmsEntityRefT *) clAmsCntGetFirst(
            &entityList->list,&nodeHandle);

    while ( entityRef != NULL )
    {

        nextEntityRef = (ClAmsEntityRefT *) clAmsCntGetNext(
                &entityList->list,&nodeHandle);

        rc = fn(entityRef,entityListName);

        if ( rc != CL_OK )
        {
            return rc;
        }

        entityRef = nextEntityRef;

    }

    return CL_OK;

}

/*
 * returns
 *
 * CL_OK            walked null or non-null list, everything ok
 * other            error in walking list
 */

ClRcT
clAmsEntityListWalkGetEntity(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityCallbackT  fn )
{

    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *entityRef = NULL;
    ClAmsEntityRefT  *nextEntityRef = NULL;

    AMS_CHECKPTR ( !entityList || !fn );

    entityRef = (ClAmsEntityRefT *) clAmsCntGetFirst(
            &entityList->list,&nodeHandle);

    while ( entityRef != NULL )
    {

        nextEntityRef = (ClAmsEntityRefT *) clAmsCntGetNext(
                &entityList->list,&nodeHandle);

        if ( entityRef->ptr )
        {
            rc =  fn(entityRef->ptr);
        }

        if ( rc != CL_OK )
        {
            return rc;
        }

        entityRef = nextEntityRef;

    }

    return CL_OK;

}

/*
 * clAmsEntityListGetFirst
 * -----------------------
 * Find the first element of a list
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClAmsEntityRefT *
clAmsEntityListGetFirst(
        CL_IN  ClAmsEntityListT  *entityList )
{

    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *entityRef = NULL;
    ClRcT  rc = CL_OK, rc2 = CL_OK;

    if ( !entityList )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: EntityList is Null Pointer. Exiting..\n"));
        return (ClAmsEntityRefT *) NULL;
    }

    if ( !entityList->numEntities )
    {
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntFirstNodeGet( entityList->list, &nodeHandle);

    if ( rc == CL_OK )
    {
         rc2 = clCntNodeUserDataGet(
                entityList->list,
                nodeHandle,
                (ClCntDataHandleT *) &entityRef );

         if ( rc2 == CL_OK )
         {
             if ( entityRef )
             {
                 entityRef->nodeHandle = nodeHandle;
             }
             return entityRef;
         }
    }

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node of list. "
                 "Returning NULL..\n", rc));
    }

    if ( CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node data in "
                 "list. Returning NULL..\n", rc2));
    }

    return (ClAmsEntityRefT *) NULL;

}

/*
 * clAmsEntityListGetNext
 * -----------------------
 * Find the next element of a list
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClAmsEntityRefT *
clAmsEntityListGetNext(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityRefT  *entityRef )
{

    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *nextEntityRef = NULL;
    ClRcT  rc = CL_OK;

    if ( !entityList || !entityRef )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: EntityList or EntityRef is Null Pointer, Exiting.\n"));
        return (ClAmsEntityRefT *) NULL;
    }

    if ( !entityList->numEntities )
    {
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntNextNodeGet(entityList->list, entityRef->nodeHandle, &nodeHandle);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClAmsEntityRefT *) NULL;
    }
    else if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding next node in list. "
                 "Returning NULL..\n", rc));
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntNodeUserDataGet(
            entityList->list,
            nodeHandle,
            (ClCntDataHandleT *) &nextEntityRef );

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClAmsEntityRefT *) NULL;
    }
    else if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding next node in list. "
                 "Returning NULL..\n", rc));
        return (ClAmsEntityRefT *) NULL;
    } 
    
    if ( nextEntityRef )
    {
        nextEntityRef->nodeHandle = nodeHandle;
    }

    return nextEntityRef;

}

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

ClAmsEntityRefT *
clAmsEntityListGetLast(
        CL_IN  ClAmsEntityListT  *entityList )
{

    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *entityRef = NULL;
    ClRcT  rc = CL_OK, rc2 = CL_OK;

    if ( !entityList )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: EntityList is Null Pointer. Exiting..\n"));
        return (ClAmsEntityRefT *) NULL;
    }

    if ( !entityList->numEntities )
    {
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntLastNodeGet(
                entityList->list,
                &nodeHandle);

    if ( rc == CL_OK )
    {
         rc2 = clCntNodeUserDataGet(
                entityList->list,
                nodeHandle,
                (ClCntDataHandleT *) &entityRef );

         if ( rc2 == CL_OK )
         {
             if ( entityRef )
             {
                 entityRef->nodeHandle = nodeHandle;
             }
             return entityRef;
         }
    }

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding last node of list. "
                 "Returning NULL..\n", rc));
    }

    if ( CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding last node data in list. "
                 "Returning NULL..\n", rc2));
    }

    return (ClAmsEntityRefT *) NULL;
}

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

ClAmsEntityRefT *
clAmsEntityListGetPrevious(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityRefT  *entityRef )
{

    ClCntNodeHandleT  nodeHandle = 0;
    ClAmsEntityRefT  *prevEntityRef = NULL;
    ClRcT  rc = CL_OK;

    if ( !entityList || !entityRef )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: EntityList or EntityRef is Null Pointer. Exiting.\n"));
        return (ClAmsEntityRefT *) NULL;
    }

    if ( !entityList->numEntities )
    {
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntPreviousNodeGet(
            entityList->list,
            entityRef->nodeHandle,
            &nodeHandle);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClAmsEntityRefT *) NULL;
    }
    else if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding previus node in list. "
                 "Returning NULL..\n", rc));
        return (ClAmsEntityRefT *) NULL;
    }

    rc = clCntNodeUserDataGet(
            entityList->list,
            nodeHandle,
            (ClCntDataHandleT *) &prevEntityRef );

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClAmsEntityRefT *) NULL;
    }
    else if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding previus node in list. "
                 "Returning NULL..\n", rc));
        return (ClAmsEntityRefT *) NULL;
    } 
    
    if ( prevEntityRef )
    {
        prevEntityRef->nodeHandle = nodeHandle;
    }

    return prevEntityRef;

}


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

ClRcT
clAmsEntityListPrint(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    AMS_CHECKPTR ( !entityList );
    
    if ( entityList->numEntities )
    {
        AMS_CALL ( clAmsEntityListWalk(
                        entityList,
                        clAmsEntityListPrintCallback,
                        entityListName ));
    }
    else
    {
        CL_AMS_PRINT_TWO_COL("", "%s", "   <None>");
    }

    return CL_OK;

}

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

ClRcT
clAmsEntityListXMLPrint(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    AMS_CHECKPTR ( !entityList );
    
    if ( entityList->numEntities )
    {
        AMS_CALL ( clAmsEntityListWalk(
                        entityList,
                        clAmsEntityListXMLPrintCallback,
                        entityListName ));
    }
#if 0
    else
    {
        CL_AMS_PRINT_TWO_COL("", "%s", "   <None>");
    }
#endif

    return CL_OK;

}

/*
 * clAmsEntityListPrintCallback
 * --------------------------------
 * Function called while walking the entity list
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListPrintCallback(
        CL_IN  ClAmsEntityRefT  *entityRef,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    return clAmsEntityListPrintEntityRef(
            entityRef,
            CL_AMS_PRINT_SUMMARY,
            entityListName );

}

/*
 * clAmsEntityListXMLPrintCallback
 * --------------------------------
 * Function called while walking the entity list to print it
 * in XML format 
 *
 * @param
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListXMLPrintCallback(
        CL_IN  ClAmsEntityRefT  *entityRef,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    return clAmsEntityListXMLPrintEntityRef(
            entityRef,
            CL_AMS_PRINT_SUMMARY,
            entityListName );

}

/*
 * clAmsEntityListPrintEntityRef
 * -----------------------------
 * Print out the contents of an entity reference
 *
 * @param
 *   entityRef                  - the reference to print
 *   mode                       - print summary or details. when asked
 *                                to print details, it will print out
 *                                info about the target too.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListPrintEntityRef(
        CL_IN  ClAmsEntityRefT  *entityRef, 
        CL_IN  ClUint32T  mode,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    AMS_CHECKPTR ( !entityRef );

    /*
     * Print the list specific data based on the entityListName
     */

    if ( entityListName == CL_AMS_SU_STATUS_SI_LIST )
    {
        ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT *)entityRef;

        CL_AMS_PRINT_TWO_COL("", "   %s", entityRef->entity.name.value);

        clAmsPrintHAState(suSIRef->haState);
    }

    else if ( entityListName == CL_AMS_SI_STATUS_SU_LIST )
    {
        ClAmsSISURefT *siSURef = (ClAmsSISURefT *)entityRef;

        CL_AMS_PRINT_TWO_COL("", "   %s", entityRef->entity.name.value);

        clAmsPrintHAState(siSURef->haState);
    }

    else if ( entityListName == CL_AMS_COMP_STATUS_CSI_LIST )
    {
        ClAmsCompCSIRefT *compCSIRef = (ClAmsCompCSIRefT *)entityRef;

        CL_AMS_PRINT_TWO_COL("", "   %s", entityRef->entity.name.value);

        clAmsPrintHAState(compCSIRef->haState);
    }

    else if ( entityListName == CL_AMS_CSI_STATUS_PG_LIST )
    {
        ClAmsCSICompRefT *csiCompRef = (ClAmsCSICompRefT *)entityRef;

        CL_AMS_PRINT_TWO_COL("", "   %s", entityRef->entity.name.value);

        clAmsPrintHAState(csiCompRef->haState);
    }
    else
    { 
        CL_AMS_PRINT_TWO_COL("", "   %s", entityRef->entity.name.value);
    }

    /*
     * Its unlikely the following will be used, but, whatever..
     */

    if ( mode == CL_AMS_PRINT_DETAILS )
    {
        return clAmsEntityPrint(entityRef->ptr);
    }

    return CL_OK;

}

/*
 * clAmsEntityListXMLPrintEntityRef
 * -----------------------------
 * Print out the contents of an entity reference in XML format
 *
 * @param
 *   entityRef                  - the reference to print
 *   mode                       - print summary or details. when asked
 *                                to print details, it will print out
 *                                info about the target too.
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListXMLPrintEntityRef(
        CL_IN  ClAmsEntityRefT  *entityRef, 
        CL_IN  ClUint32T  mode,
        CL_IN  ClAmsEntityListTypeT  entityListName )
{

    AMS_CHECKPTR ( !entityRef );

    /*
     * Print the list specific data based on the entityListName
     */

    if ( entityListName == CL_AMS_SU_STATUS_SI_LIST )
    {
        ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT *)entityRef;

        CL_AMS_PRINT_OPEN_TAG_ATTR("si", "%s", entityRef->entity.name.value);

        clAmsXMLPrintHAState(suSIRef->haState);

        CL_AMS_PRINT_CLOSE_TAG("si");
    }

    else if ( entityListName == CL_AMS_SI_STATUS_SU_LIST )
    {
        ClAmsSISURefT *siSURef = (ClAmsSISURefT *)entityRef;

        CL_AMS_PRINT_OPEN_TAG_ATTR("su","%s",entityRef->entity.name.value);

        clAmsXMLPrintHAState(siSURef->haState);

        CL_AMS_PRINT_CLOSE_TAG("su");

    }

    else if ( entityListName == CL_AMS_COMP_STATUS_CSI_LIST )
    {
        ClAmsCompCSIRefT *compCSIRef = (ClAmsCompCSIRefT *)entityRef;

        CL_AMS_PRINT_OPEN_TAG_ATTR("csi", "%s", entityRef->entity.name.value);

        clAmsXMLPrintHAState(compCSIRef->haState);

        CL_AMS_PRINT_CLOSE_TAG("csi");

    }

    else if ( entityListName == CL_AMS_CSI_STATUS_PG_LIST )
    {
        ClAmsCSICompRefT *csiCompRef = (ClAmsCSICompRefT *)entityRef;

        CL_AMS_PRINT_OPEN_TAG_ATTR("pg", "%s", entityRef->entity.name.value);

        clAmsXMLPrintHAState(csiCompRef->haState);

        CL_AMS_PRINT_CLOSE_TAG("pg");

    }
    else
    { 
        CL_AMS_PRINT_TAG_VALUE(CL_AMS_STRING_ENTITY_TYPE(entityRef->
                                                         entity.type),
                               "%s", entityRef->entity.name.value);
    }

    /*
     * Its unlikely the following will be used, but, whatever..
     */

    if ( mode == CL_AMS_PRINT_DETAILS )
    {
        return clAmsEntityXMLPrint(entityRef->ptr);
    }

    return CL_OK;

}

/*
 * clAmsPrintHAState 
 * -----------------
 * Print out the HA state of a SI for a given SU
 *
 * @param
 * haState                      - HA state of the SI for the SU
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsPrintHAState(
        CL_IN  ClAmsHAStateT  haState )
{

    if ( haState == CL_AMS_HA_STATE_NONE )
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: NONE");
    }
    else if (  haState == CL_AMS_HA_STATE_ACTIVE )
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: ACTIVE");
    }
    else if (  haState == CL_AMS_HA_STATE_STANDBY)
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: STANDBY");
    }
    else if (  haState == CL_AMS_HA_STATE_QUIESCED)
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: QUIESCED");
    }
    else if (  haState == CL_AMS_HA_STATE_QUIESCING)
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: QUIESCING");
    }
    else
    {
        CL_AMS_PRINT_TWO_COL("", "   %s", "HA State: INVALID");
    }

    return CL_OK;

}

/*
 * clAmsXMLPrintHAState 
 * -----------------
 * Print out the HA state of a SI for a given SU in XML format
 *
 * @param
 * haState                      - HA state of the SI for the SU
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsXMLPrintHAState(
        CL_IN  ClAmsHAStateT  haState )
{

    if ( haState == CL_AMS_HA_STATE_NONE )
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "none");
    }
    else if (  haState == CL_AMS_HA_STATE_ACTIVE )
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "active");
    }
    else if (  haState == CL_AMS_HA_STATE_STANDBY)
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "standby");
    }
    else if (  haState == CL_AMS_HA_STATE_QUIESCED)
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "quiesced");
    }
    else if (  haState == CL_AMS_HA_STATE_QUIESCING)
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "quiescing");
    }
    else
    {
        CL_AMS_PRINT_TAG_VALUE("ha_state", "%s", "invalid");
    }

    return CL_OK;

}

/******************************************************************************
 * Utility functions for specific entities
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Service Group
 *---------------------------------------------------------------------------*/

ClRcT clAmsSGAddSURefToSUList(CL_IN  ClAmsEntityListT  *entityList, CL_IN  ClAmsSUT  *su )
{
    ClAmsEntityRefT  *suRef = NULL, *foundRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SU ( su );
    AMS_CHECKPTR ( !entityList );

    clAmsEntityRefGetKey(&su->config.entity, su->config.rank, &entityKeyHandle, entityList->isRankedList );
    
    /*
     * Make sure the SU is not already on the list
     */
    suRef = clAmsCreateEntityRef(&su->config.entity);    

    rc = clAmsEntityListFindEntityRef(
            entityList,
            suRef,
            entityKeyHandle,
            &foundRef); 
    if (rc == CL_OK) goto cleanup; /* SU is already on the list -- probably NO_OP should be returned but for now keep it as CL_OK for stability */

    
    else if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST )
    {
        clLogWarning("AMS","DB","clAmsSGAddSURefToSUList added SURef to list multiple times");
        goto cleanup;
    }

    rc = clAmsEntityListAddEntityRef( entityList, suRef, entityKeyHandle);
    if ( rc != CL_OK ) goto cleanup;

    return CL_OK;
cleanup:
    if (suRef) clAmsFreeEntityRef(suRef);
    return rc;
}

ClRcT clAmsSGDeleteSURefFromSUList(CL_IN  ClAmsEntityListT  *entityList, CL_IN  ClAmsSUT  *su ) 
{

    ClAmsEntityRefT  suRef = {{0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SU ( su );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL (clAmsEntityRefGetKey(
                &su->config.entity,
                su->config.rank,
                &entityKeyHandle,
                entityList->isRankedList) );
    
    memcpy(&suRef.entity, &su->config.entity, sizeof(ClAmsEntityT) ); 

    suRef.ptr = (ClAmsEntityT *) su;

    rc = clAmsEntityListDeleteEntityRef( entityList, &suRef, entityKeyHandle);

    return rc;
}

#ifdef POST_RC2

ClRcT
clAmsSGAddSIRefToSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSIT  *si )
{

    ClAmsEntityRefT  *siRef = NULL, *foundRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &si->config.entity,
                si->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    siRef = clHeapAllocate(sizeof(ClAmsEntityRefT));

    AMS_CHECK_NO_MEMORY (siRef);

    memcpy(&siRef->entity, &si->config.entity, sizeof(ClAmsEntityT) ); 

    siRef->ptr = (ClAmsEntityT *) si;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            siRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(siRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef( entityList, siRef, entityKeyHandle);

    if ( rc != CL_OK )
    {
        clHeapFree(siRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsSGDeleteSIRefFromSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSIT  *si )
{

    ClAmsEntityRefT  siRef = {{0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &si->config.entity,
                si->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    memcpy(&siRef.entity, &si->config.entity, sizeof(ClAmsEntityT) ); 

    siRef.ptr = (ClAmsEntityT *) si;

    rc = clAmsEntityListDeleteEntityRef( entityList, &siRef, entityKeyHandle);

    return rc;

}

#endif

/*-----------------------------------------------------------------------------
 * Service Unit Functions
 *---------------------------------------------------------------------------*/

ClRcT
clAmsSUAddSIRefToSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSIT  *si,
        CL_IN  ClAmsHAStateT  haState )
{

    ClAmsEntityRefT  *foundRef = NULL;
    ClAmsSUSIRefT  *siRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &si->config.entity,
                si->config.rank,
                &entityKeyHandle,
                entityList->isRankedList));

    siRef = clHeapAllocate(sizeof(ClAmsSUSIRefT)); 

    AMS_CHECK_NO_MEMORY (siRef);

    memcpy(&siRef->entityRef.entity, &si->config.entity, sizeof(ClAmsEntityT)); 

    siRef->entityRef.ptr = (ClAmsEntityT *) si;
    siRef->haState = haState;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            (ClAmsEntityRefT *)siRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(siRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef(
            entityList,
            (ClAmsEntityRefT *)siRef,
            entityKeyHandle);

    if ( rc != CL_OK )
    {
        clHeapFree(siRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsSUDeleteSIRefFromSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSIT  *si )
{

    ClAmsEntityRefT  siRef = {{0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SI ( si );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &si->config.entity,
                si->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    memcpy(&siRef.entity, &si->config.entity, sizeof(ClAmsEntityT) ); 

    siRef.ptr = (ClAmsEntityT *) si;

    rc = clAmsEntityListDeleteEntityRef( entityList, &siRef, entityKeyHandle);

    return rc;

}

/*-----------------------------------------------------------------------------
 * Service Instance Functions
 *---------------------------------------------------------------------------*/

ClRcT
clAmsSIAddSURefToSUList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSUT  *su,
        CL_IN  ClAmsHAStateT  haState )
{

    ClAmsEntityRefT  *foundRef = NULL;
    ClAmsSISURefT  *suRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SU ( su );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &su->config.entity,
                su->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    suRef = clHeapAllocate(sizeof(ClAmsSISURefT) );

    AMS_CHECK_NO_MEMORY (suRef);

    memcpy(&suRef->entityRef.entity, &su->config.entity, sizeof(ClAmsEntityT) );
    suRef->entityRef.ptr = (ClAmsEntityT *) su;
    suRef->haState = haState;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            (ClAmsEntityRefT *)suRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(suRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef(
            entityList,
            (ClAmsEntityRefT *)suRef,
            entityKeyHandle);

    if ( rc != CL_OK )
    {
        clHeapFree(suRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsSIDeleteSURefFromSUList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsSUT  *su )
{

    ClAmsSISURefT suRef = {{{0},0,0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_SU ( su );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &su->config.entity,
                su->config.rank,
                &entityKeyHandle,
                entityList->isRankedList));

    memcpy(&suRef.entityRef.entity, &su->config.entity, sizeof(ClAmsEntityT) ); 

    suRef.entityRef.ptr = (ClAmsEntityT *) su;

    rc = clAmsEntityListDeleteEntityRef(
            entityList,
            (ClAmsEntityRefT *)&suRef,
            entityKeyHandle);

    return rc;

}

#ifdef POST_RC2

ClRcT
clAmsSIAddCSIRefToCSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCSIT  *csi )
{

    ClAmsEntityRefT  *foundRef = NULL;
    ClAmsEntityRefT  *csiRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_CSI ( csi );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &csi->config.entity,
                csi->config.rank,
                &entityKeyHandle,
                entityList->isRankedList )); 
    
    csiRef = clHeapAllocate(sizeof(ClAmsEntityRefT) );

    AMS_CHECK_NO_MEMORY (csiRef);

    memcpy(&csiRef->entity, &csi->config.entity, sizeof(ClAmsEntityT) ); 

    csiRef->ptr = (ClAmsEntityT *) csi;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            csiRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(csiRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef( entityList, csiRef, entityKeyHandle);

    if ( rc != CL_OK )
    {
        clHeapFree(csiRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsSIDeleteCSIRefFromCSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCSIT  *csi )
{

    ClAmsEntityRefT  csiRef = {{0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_CSI ( csi );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &csi->config.entity,
                csi->config.rank,
                &entityKeyHandle,
                entityList->isRankedList));

    memcpy(&csiRef.entity, &csi->config.entity, sizeof(ClAmsEntityT) ); 

    csiRef.ptr = (ClAmsEntityT *) csi;

    rc = clAmsEntityListDeleteEntityRef( entityList, &csiRef, entityKeyHandle);

    return rc;

}

#endif

/*-----------------------------------------------------------------------------
 * Component Functions
 *---------------------------------------------------------------------------*/

ClRcT
clAmsCompAddCSIRefToCSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClAmsHAStateT  haState,
        CL_IN  ClAmsCSITransitionDescriptorT  tdescriptor )
{

    ClAmsEntityRefT  *foundRef = NULL;
    ClAmsCompCSIRefT  *csiRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_CSI ( csi );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &csi->config.entity,
                csi->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    csiRef = clHeapAllocate(sizeof(ClAmsCompCSIRefT));

    AMS_CHECK_NO_MEMORY (csiRef);

    memcpy(&csiRef->entityRef.entity,&csi->config.entity,sizeof(ClAmsEntityT));
    csiRef->entityRef.ptr = (ClAmsEntityT *) csi;
    csiRef->haState = haState;
    csiRef->tdescriptor = tdescriptor;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            (ClAmsEntityRefT *)csiRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(csiRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef(
            entityList,
            (ClAmsEntityRefT *)csiRef,
            entityKeyHandle);

    if ( rc != CL_OK ) 
    {
        clHeapFree(csiRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsCompDeleteCSIRefFromCSIList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCSIT  *csi )
{

    ClAmsCompCSIRefT  csiRef = {{{0},0,0},0,0,0,NULL};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;

    AMS_CHECK_CSI ( csi );
    AMS_CHECKPTR ( !entityList );

    AMS_CALL ( clAmsEntityRefGetKey(
                &csi->config.entity,
                csi->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    memcpy(&csiRef.entityRef.entity, &csi->config.entity, sizeof(ClAmsEntityT)); 
    csiRef.entityRef.ptr = (ClAmsEntityT *) csi;

    rc = clAmsEntityListDeleteEntityRef(
            entityList,
            (ClAmsEntityRefT *)&csiRef,
            entityKeyHandle);

    return rc;

}

/*-----------------------------------------------------------------------------
 * CSI Functions
 *---------------------------------------------------------------------------*/

ClRcT
clAmsCSIAddCompRefToPGList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCompT  *comp,
        CL_IN  ClAmsHAStateT  haState )
{

    ClAmsEntityRefT  *foundRef = NULL;
    ClAmsCSICompRefT  *compRef = NULL;
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;
    ClAmsSUT  *su = NULL;

    AMS_CHECK_COMP ( comp );
    AMS_CHECKPTR ( !entityList );
    AMS_CHECK_SU ( (su = (ClAmsSUT *) comp->config.parentSU.ptr) );

    /*
     * Note, the components that form the protection group for a CSI are
     * ordered by the rank of their constituent SUs.
     */

    AMS_CALL ( clAmsEntityRefGetKey(
                &comp->config.entity,
                su->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    compRef = clHeapAllocate(sizeof(ClAmsCSICompRefT) );

    AMS_CHECK_NO_MEMORY (compRef);

    memcpy(&compRef->entityRef.entity,
            &comp->config.entity,
            sizeof(ClAmsEntityT) ); 
    compRef->entityRef.ptr = (ClAmsEntityT *) comp;
    compRef->haState = haState;

    rc = clAmsEntityListFindEntityRef(
            entityList,
            (ClAmsEntityRefT *)compRef,
            entityKeyHandle,
            &foundRef); 

    if ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) 
    {
        clHeapFree(compRef);
        return rc;
    }

    rc = clAmsEntityListAddEntityRef(
            entityList,
            (ClAmsEntityRefT *)compRef,
            entityKeyHandle);

    if ( rc != CL_OK ) 
    {
        clHeapFree(compRef);
        return rc;
    }

    return CL_OK;

}

ClRcT
clAmsCSIDeleteCompRefFromPGList(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClAmsCompT  *comp )
{

    ClAmsEntityRefT  compRef = {{0},0,0};
    ClRcT  rc = CL_OK;
    ClCntKeyHandleT  entityKeyHandle = 0;
    ClAmsSUT  *su = NULL;

    AMS_CHECK_COMP ( comp );
    AMS_CHECKPTR ( !entityList );
    AMS_CHECK_SU ( (su = (ClAmsSUT *) comp->config.parentSU.ptr) );

    AMS_CALL ( clAmsEntityRefGetKey(
                &comp->config.entity,
                su->config.rank,
                &entityKeyHandle,
                entityList->isRankedList ));

    memcpy(&compRef.entity, &comp->config.entity, sizeof(ClAmsEntityT) ); 

    compRef.ptr = (ClAmsEntityT *) comp;

    rc = clAmsEntityListDeleteEntityRef( entityList, &compRef, entityKeyHandle);

    return rc;

}

/******************************************************************************
 * Support functions for Clovis Containers
 *****************************************************************************/

/*
 * Function to compare two keys for an AMS entity.
 */

ClInt32T
clAmsEntityDbEntityKeyCompareCallback(
        CL_IN  ClCntKeyHandleT  key1,
        CL_IN  ClCntKeyHandleT  key2 )
{
    return (ClWordT)key1 - (ClWordT)key2;
}

/*
 * Hash functions for each AMS entity type. These functions are really
 * redundant. When a hash table is created, the bucket size is presented
 * as an argument. The hash table could compute the hash directly.
 */

ClUint32T
clAmsEntityDbEntityKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_ENTITY].numDbBuckets);
}

ClUint32T
clAmsEntityDbNodeKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_NODE].numDbBuckets);
}

ClUint32T
clAmsEntityDbAppKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_APP].numDbBuckets);
}

ClUint32T
clAmsEntityDbSGKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_SG].numDbBuckets);
}

ClUint32T
clAmsEntityDbSUKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_SU].numDbBuckets);
}

ClUint32T
clAmsEntityDbSIKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_SI].numDbBuckets);
}

ClUint32T
clAmsEntityDbCompKeyHashCallback( CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_COMP].numDbBuckets);
}

ClUint32T
clAmsEntityDbCSIKeyHashCallback(
        CL_IN  ClCntKeyHandleT  key )
{
    return ((ClWordT)key % 
        gClAmsEntityDbParams[CL_AMS_ENTITY_TYPE_CSI].numDbBuckets);
}


/*
 * Verify any inconsistencies in the db
 */

static ClRcT
clAmsAuditComp(ClAmsCompT *comp)
{
    ClAmsEntityRefT  *entityRef;
    ClAmsInvocationCmdT csiOps[CL_AMS_MAX_CALLBACKS];
    ClBoolT allCSIsChecked = CL_FALSE;
    ClRcT rc = CL_OK;

    memset(csiOps, 0, sizeof(csiOps));

    for(entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
        entityRef != NULL;
        entityRef = clAmsEntityListGetNext(&comp->status.csiList,entityRef)
        )
    {
        ClAmsCSIT *csi ;
        ClAmsCompCSIRefT *csiRef;
        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CALL ( clAmsEntityListFindEntityRef2(&comp->status.csiList,
                                                 &csi->config.entity,
                                                 0,
                                                 (ClAmsEntityRefT**)&csiRef
         
                                                 ));
        /*
         * Now check for pending ops against the invocation list.
         * If not found, add an invocation entry against the csi pending op.
         * for the comp.
         */
        if(csiRef->pendingOp)
        {
            ClInvocationT invocation = 0;
            ClAmsInvocationT invocationData = {0};

            if(csiRef->pendingOp < CL_AMS_MAX_CALLBACKS)
                ++csiOps[csiRef->pendingOp];

            if(csiRef->pendingOp == CL_AMS_CSI_SET_CALLBACK
               &&
               !allCSIsChecked)
            {
                allCSIsChecked = CL_TRUE;
                if(clAmsInvocationFindCSI(comp, NULL, CL_AMS_CSI_SET_CALLBACK, &invocationData) == CL_OK)
                {

                    /*
                     * If this has an entry for the reassign all csi invocation, then
                     * remove it and add an invocation entry per csi.
                     */
                    if((rc = clAmsInvocationGetAndDeleteExtended(invocationData.invocation, 
                                                                 &invocationData,
                                                                 CL_FALSE)) != CL_OK)
                    {
                        clLogError("COMP", "AUDIT", "Reassign all invocation entry get for component [%s] "
                                   "returned [%#x] for invocation [%llx]",
                                   comp->config.entity.name.value, rc, invocationData.invocation);
                    }
                    else
                    {
                        clLogNotice("COMP", "AUDIT", "Deleted reassign all entry for component [%s] with invocation [%llx]",
                                    comp->config.entity.name.value, invocationData.invocation);
                    }
                }
            }
            
            if(CL_OK != 
               clAmsInvocationFindCSI(comp,csi,csiRef->pendingOp,NULL))
            {
                if( (rc = clAmsInvocationCreate(csiRef->pendingOp,
                                                comp,csi,&invocation)) != CL_OK )
                {
                    /*
                     * Undo pending operation counter.
                     */
                    if(csiOps[csiRef->pendingOp] > 0)
                        --csiOps[csiRef->pendingOp];
                    clLogError("COMP", "AUDIT", "Adding invocation for Component [%s], csi [%s], cmd [%#x] returned [%#x]",
                               comp->config.entity.name.value, 
                               csi->config.entity.name.value,
                               csiRef->pendingOp, rc);
                }
                else
                {
                    clLogNotice("COMP", "AUDIT",
                                "Added missing invocation entry [%llx] with cmd [%d] for "
                                "Component [%s], CSI [%s]",
                                invocation, csiRef->pendingOp, comp->config.entity.name.value,
                                csi->config.entity.name.value);
                }
            }
        }
    }

    /*
     * Now if there were no CSI sets pending, then reset comp timer values
     * incase the checkpoint for timers went through but the invocation didn't
     * just at the time of failover.
     */
    if(!csiOps[CL_AMS_CSI_SET_CALLBACK])
    {
        if(comp->status.timers.csiSet.count > 0 )
        {
            clLogWarning("COMP", "AUDIT", "No pending CSI set invocations for pending csiset timer. "
                         "Resetting timer count...");
            comp->status.entity.timerCount -= 
                CL_MIN(comp->status.entity.timerCount, comp->status.timers.csiSet.count);
            gAms.timerCount -= CL_MIN(gAms.timerCount, comp->status.timers.csiSet.count);
            comp->status.timers.csiSet.count = 0;
        }
    }

    if(!csiOps[CL_AMS_CSI_QUIESCING_CALLBACK])
    {
        if(comp->status.timers.quiescingComplete.count > 0 )
        {
            clLogWarning("COMP", "AUDIT", "No pending CSI invocations for pending csiquiescing timer. "
                         "Resetting timer count...");
            comp->status.entity.timerCount -= 
                CL_MIN(comp->status.entity.timerCount, comp->status.timers.quiescingComplete.count);
            gAms.timerCount -= CL_MIN(gAms.timerCount, comp->status.timers.quiescingComplete.count);
            comp->status.timers.quiescingComplete.count = 0;
        }
    }

    if(!csiOps[CL_AMS_CSI_RMV_CALLBACK])
    {
        if(comp->status.timers.csiRemove.count > 0)
        {
            clLogWarning("COMP", "AUDIT", "No pending CSI remove invocations for pending csi remove timer. "
                         "Resetting timer count...");
            comp->status.entity.timerCount -=
                CL_MIN(comp->status.entity.timerCount, comp->status.timers.csiRemove.count);
            gAms.timerCount -= CL_MIN(gAms.timerCount, comp->status.timers.csiRemove.count);
            comp->status.timers.csiRemove.count = 0;
        }
    }

    /*
     * If we have an instantiate response pending on the component,
     * add an invocation in CPM for a response replay.
     */
    if(comp->status.timers.instantiate.count > 0
       ||
       comp->status.timers.terminate.count > 0)
    {
        ClAmsSUT *su = NULL;
        ClAmsNodeT *node = NULL;
        su = (ClAmsSUT*)comp->config.parentSU.ptr;
        if(su)
            node = (ClAmsNodeT*)su->config.parentNode.ptr;
        if(su && node)
        {
            if(node->status.isClusterMember == CL_AMS_NODE_IS_CLUSTER_MEMBER)
            {
                ClUint32T cbType = CL_AMS_INSTANTIATE_REPLAY_CALLBACK;
                ClAmsEntityTimerT *pTimer = &comp->status.timers.instantiate;
                ClBoolT responsePending = CL_TRUE;
                if(!comp->status.timers.instantiate.count
                   &&
                   comp->status.timers.terminate.count > 0)
                {
                    cbType = CL_AMS_TERMINATE_REPLAY_CALLBACK;
                    pTimer = &comp->status.timers.terminate;
                }
                if( (rc = cpmReplayInvocationAdd(cbType, 
                                                 comp->config.entity.name.value, 
                                                 node->config.entity.name.value, &responsePending)) != CL_OK)
                {
                    clLogError("COMP", "AUDIT", "CPM [%s] invocation for component [%s] returned [%#x]",
                               cbType == CL_AMS_INSTANTIATE_REPLAY_CALLBACK ? "instantiate":"terminate",
                               comp->config.entity.name.value, rc);
                }
                else
                {
                    if(!responsePending)
                    {
                        clLogNotice("COMP", "AUDIT", "Added [%s] invocation for registered "
                                    "component [%s] belonging to node [%s]",
                                    cbType == CL_AMS_INSTANTIATE_REPLAY_CALLBACK ? "instantiate":"terminate",
                                    comp->config.entity.name.value, node->config.entity.name.value);
                        /*
                         * If there is no response pending, reset timer count.
                         */
                        clLogInfo("COMP", "AUDIT", "Resetting pending [%s] timeout for component [%s]",
                                  cbType == CL_AMS_INSTANTIATE_REPLAY_CALLBACK ? "instantiate":"terminate",
                                  comp->config.entity.name.value);
                        comp->status.entity.timerCount -=
                            CL_MIN(comp->status.entity.timerCount, pTimer->count);
                        gAms.timerCount -= CL_MIN(gAms.timerCount, pTimer->count);
                        pTimer->count = 0;
                    }
                }
            }
        }
    }
    return CL_OK;
}

static ClRcT
clAmsAuditEntityComp(void)
{
    AMS_CALL( clAmsEntityDbWalk(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                (ClAmsEntityCallbackT) clAmsAuditComp
                                ));
    return CL_OK;
}

static ClRcT clAmsAuditNode(ClAmsNodeT *node, ClPtrT arg)
{
    ClBoolT *dirty = (ClBoolT*)arg;
    if(node->status.isClusterMember != CL_AMS_NODE_IS_CLUSTER_MEMBER) // We should audit the 'leaving' node
    {
        /*
         * We should be already within the global cluster mutex while processing    
         * failovers
         */
        ClCpmLT *cpmL = NULL;
        cpmNodeFindLocked((ClCharT*)node->config.entity.name.value, &cpmL);
        if(cpmL && cpmL->pCpmLocalInfo)
        {
            ClIocNodeAddressT nodeId = cpmL->pCpmLocalInfo->nodeId;
            ClUint8T status = CL_IOC_NODE_UP;
            if(clIocRemoteNodeStatusGet(nodeId, &status) == CL_OK
               && 
               status == CL_IOC_NODE_DOWN)
            {
                clLogNotice("NODE", "AUDIT", "Triggering node leave for ejected node [%s:%d] "
                            "whose db state is still present", 
                            node->config.entity.name.value, nodeId);
                *dirty = CL_TRUE;
                clAmsPeNodeHasLeftCluster(node, CL_FALSE);
            }
            else
            {
              clAmsPeNodeIsLeavingClusterCallback_Step1(node, CL_OK);
            }
        }
        else
        {
            /*
             * No CPML entry present. We can treat it as an inconsistent state
             */
            clLogNotice("NODE", "AUDIT", "Triggering node leave for ejected node [%s] "
                        "whose db state is still present",
                        node->config.entity.name.value);
            *dirty = CL_TRUE;
            //clAmsPeNodeHasLeftCluster(node, CL_FALSE);
            // ??? terminate all SUs on this node since switchover done - No CPML entry present
            clAmsPeNodeIsLeavingClusterCallback_Step1(node, CL_OK);
        }
    }
    return CL_OK;
}

static ClRcT
clAmsAuditEntityNode(ClPtrT arg)
{
    AMS_CALL( clAmsEntityDbWalkExtended(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                                        (ClAmsEntityCallbackExtendedT) clAmsAuditNode,
                                        arg) );
    return CL_OK;
}

ClRcT
clAmsAuditEntityDB(void)
{
    AMS_FUNC_ENTER(("\n"));

    /*
     * Add any other entity audits here
     */

    AMS_CALL(clAmsAuditEntityComp());
    
    return CL_OK;

}

ClRcT 
clAmsAuditDb(ClAmsNodeT *pThisNode)
{
    AMS_FUNC_ENTER(("\n"));

    AMS_CALL(clAmsAuditEntityDB());
    
    return CL_OK;
}

ClRcT
clAmsAuditDbEpilogue(ClNameT *nodeName, ClBoolT scFailover)
{
    ClRcT rc = CL_OK;
    ClBoolT dirty = CL_FALSE;
    if(!nodeName) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!scFailover) /* nothing to replay*/
        return CL_OK;

    clLogNotice("AUDIT", "DBA", "Running AMF audit db prologue functions for node [%s]", nodeName->value);
    rc = cpmReplayInvocations(CL_TRUE);
    clOsalMutexLock(gAms.mutex);
    rc |= clAmsAuditEntityNode((ClPtrT)&dirty);
    if(rc == CL_OK && dirty)
    {
        rc = clAmsCkptWrite(&gAms, CL_AMS_CKPT_WRITE_DB);
    }
    clOsalMutexUnlock(gAms.mutex);
    return rc;
}
