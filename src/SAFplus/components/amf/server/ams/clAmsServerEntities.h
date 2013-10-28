#ifndef _CL_AMS_SERVER_ENTITIES_H_
#define _CL_AMS_SERVER_ENTITIES_H_

#include <clAmsEntities.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * AMS Entity Methods
 *****************************************************************************/

/*
 * clAmsEntityGetDefaults
 * ----------------------
 * Get the default params associated with an entity. Not directly accessing
 * global data structures means, the defaults can be different for different
 * entities if necessary.
 *
 * @param
 *
 * @returns
 *
 */

extern ClRcT
clAmsEntityGetDefaults(
        CL_IN   ClAmsEntityTypeT type,
        CL_OUT  ClAmsEntityParamsT **params);

/*
 * clAmsEntityReset
 * ----------------
 * Reset the status elements of an entity.
 *
 * @param
 *
 * @returns
 *
 */

extern ClRcT
clAmsEntityReset(
        CL_IN   ClAmsEntityT *entity);

/*
 * clAmsEntityMalloc
 * -----------------
 * Create a new AMS Entity and configure it appropriately with the config
 * and methods passed in. Thus each new entity can have a different config
 * as well as methods.
 *
 * @param
 *   type                       - type of entity
 *   config                     - config of new entity
 *   methods                    - methods to set for new entity
 *
 * @returns
 *   ClAmsEntityT *             - pointer to new entity
 *
 */

extern ClAmsEntityT * clAmsEntityMalloc(
        CL_IN   ClAmsEntityTypeT type,
        CL_IN   ClAmsEntityConfigT *config,
        CL_IN   ClAmsEntityMethodsT *methods);

/*
 * clAmsEntityFree
 * ---------------
 * Free an AMS Entity
 *
 * @param
 *   ClAmsEntityT *             - pointer to entity
 *
 * @returns
 *   CL_OK                      - ok
 */

extern ClRcT clAmsEntityFree(
        CL_IN   ClAmsEntityT *entity);

/*
 * clAmsEntityGetStatusStruct
 * --------------------------
 * Given an entity ptr, return ptr to the status portion
 *
 * @param
 *   ClAmsEntityT *             - pointer to entity
 *
 * @returns
 *   ClAmsEntityStatusT *       - pointer to entity status struct
 */

extern ClAmsEntityStatusT * clAmsEntityGetStatusStruct(
        CL_IN   ClAmsEntityT *entity);

/*
 * clAmsEntityPrint
 * ----------------
 * Print contents of an AMS Entity
 *
 * @param
 *   entity                     - Pointer to a valid entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityPrint(
        CL_IN       ClAmsEntityT            *entity);

/*
 * clAmsEntityXMLPrint
 * ----------------
 * Print contents of an AMS Entity in XML format
 *
 * @param
 *   entity                     - Pointer to a valid entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

extern ClRcT clAmsEntityXMLPrint(
        CL_IN       ClAmsEntityT            *entity);

/*
 * clAmsEntityGetKey
 * -----------------
 * Generate an unique key for each entity based on its name
 *
 * @param
 *   entity                     - Entity whose key is to be generated
 *   entityKeyHandle            - The key handle to return
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClCntKeyHandleT clAmsEntityGetKey(CL_IN       const ClAmsEntityT        *entity);

/*
 * clAmsEntityClearOps
 * -------------------
 * Clear all pending invocations and timers for an entity
 *
 * @param
 *
 * @returns
 *
 */

extern ClRcT
clAmsEntityClearOps(
        CL_IN   ClAmsEntityT *entity);

/******************************************************************************
 * Functions related to managing timers for entities
 *****************************************************************************/

/*
 * clAmsEntityTimer*
 * -------------------
 * Functions to handle all the timers required by a component.
 * timerCreate, timerSet, timerCancel, timerDelete
 *
 */

extern ClRcT clAmsEntityTimerGetValues(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType,
        CL_OUT      ClTimeT *duration,
        CL_OUT      ClAmsEntityTimerT **entityTimer,
        CL_OUT      ClTimerCallBackT *fn);

extern ClRcT clAmsEntityTimerCreate(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT clAmsEntityTimerStart(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT
clAmsEntityTimerUpdate(
                       CL_IN  ClAmsEntityT  *entity, 
                       CL_IN  ClAmsEntityTimerTypeT  timerType,
                       CL_IN  ClTimeT oldDuration);

extern ClRcT clAmsEntityTimerStop(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT clAmsEntityTimerStopIfCountZero(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT clAmsEntityTimerDelete(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT clAmsEntityTimerIsRunning(
        CL_IN       ClAmsEntityT *entity, 
        CL_IN       ClAmsEntityTimerTypeT timerType);

extern ClRcT clAmsEntityTimeout(
        CL_IN       ClAmsEntityTimerT *timer);


/*
 * clAmsEntityValidate
 * -------------------
 * Validates an AMS entity. This function can be used by the management
 * API to verify an entity is properly configured as well as by the policy
 * engine when it needs to do a comprehensive check on an entity.
 *
 * @param
 *   entity                     - the entity to validate
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

#define CL_AMS_ENTITY_VALIDATE_CONFIG           1
#define CL_AMS_ENTITY_VALIDATE_RELATIONSHIPS    2
#define CL_AMS_ENTITY_VALIDATE_ALL              3

extern ClRcT clAmsEntityValidate(
        CL_IN       ClAmsEntityT        *entity, 
        CL_IN       ClUint32T           mode);

extern ClRcT clAmsEntityValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsEntityValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsNodeValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsNodeValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSGValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSGValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSUValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSUValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSIValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSIValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsCompValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsCompValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsCSIValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsCSIValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsValidateCSIType(
        CL_IN  ClAmsEntityT  siEntity,
        CL_IN  ClAmsEntityListT  *suList,
        CL_IN  ClAmsEntityListT  *csiList );

extern ClRcT clAmsAppValidateConfig(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsAppValidateRelationships(
        CL_IN ClAmsEntityT *entity);

extern ClRcT clAmsSGValidateLoadingStrategy(
        CL_IN ClAmsSGT *sg);

extern ClRcT clAmsSGValidateRedundancyModel(
        CL_IN ClAmsSGT *sg);

extern ClRcT clAmsValidateReference(
        CL_IN       ClAmsEntityRefT        entityRef,
        CL_IN       ClAmsEntityTypeT       entityType);

/******************************************************************************
 * EntityRef functions
 *****************************************************************************/


extern ClRcT clAmsEntityRefGetKey(
        CL_IN   ClAmsEntityT *entity,
        CL_IN   ClUint32T    rank,
        CL_OUT  ClCntKeyHandleT *entityKeyHandle,
        CL_IN   ClBoolT isRankedList );

/******************************************************************************
 * Entity Specific Functions
 *****************************************************************************/

extern ClRcT clAmsCSIMarshalCSIDescriptor(
        ClAmsCSIDescriptorT             **csiDescriptorPtr,
        ClAmsCSIT                       *csi,
        ClAmsCSIFlagsT                  csiFlags,
        ClAmsHAStateT                   haState,
        ClNameT                         activeCompName,
        ClAmsCSITransitionDescriptorT   transitionDescr,
        ClUint32T                       standbyRank);

extern ClRcT clAmsCSIMarshalCSIDescriptorExtended(
        ClAmsCSIDescriptorT             **csiDescriptorPtr,
        ClAmsCSIT                       *csi,
        ClAmsCSIFlagsT                  csiFlags,
        ClAmsHAStateT                   haState,
        ClNameT                         activeCompName,
        ClAmsCSITransitionDescriptorT   transitionDescr,
        ClUint32T                       standbyRank,
        ClBoolT                         reassignCSI);

extern ClRcT
clAmsCSIMarshalPGTrackNotificationBuffer(
        ClAmsPGNotificationBufferT      **notificationBufferPtr,
        ClAmsCSIT                       *csi,
        ClAmsCompT                      *changedComp,
        ClAmsPGTrackFlagT               pgTrackFlags,
        ClAmsPGChangeT                  pgChange );

/*
 * clAmsEntityTerminate
 * ----------------
 * Terminate the Entity
 */

extern ClRcT
clAmsEntityTerminate(
        CL_IN       ClAmsEntityT            *entity);

/*
 * Entity stack operations.
*/

extern ClRcT
clAmsEntityOpPush(ClAmsEntityT *entity, ClAmsEntityStatusT *status, void *data, ClUint32T dataSize, ClUint32T op);

extern ClRcT
clAmsEntityOpPop(ClAmsEntityT *entity, ClAmsEntityStatusT *status, void **data, ClUint32T *dataSize, ClUint32T *op);

extern ClBoolT
clAmsEntityOpPending(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op);

extern ClRcT
clAmsEntityOpGet(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op,
                 void **data, ClUint32T *dataSize);

extern ClRcT
clAmsEntityOpClear(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op,
                   void **data, ClUint32T *dataSize);

extern ClRcT
clAmsEntityOpsClear(ClAmsEntityT *entity, ClAmsEntityStatusT *status);

extern ClRcT
clAmsEntityOpsClearNode(ClAmsEntityT *entity, ClAmsEntityStatusT *status);

#ifdef __cplusplus
}
#endif

#endif
