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
 * ModuleName  : amf
 * File        : clAmsEntities.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements the generic methods for AMS entities.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <crc.h>
#include <clOsalApi.h>

#include <clAms.h>
#include <clAmsServerUtils.h>
#include <clAmsDatabase.h>
#include <clAmsModify.h>
#include <clAmsPolicyEngine.h>
#include <clAmsEntityUserData.h>
#include <clAmsNotifications.h>
#include <clCpmInternal.h>

/******************************************************************************
 * Global data structures
 *****************************************************************************/

// XXX This is a temporary hack, until all entities have backptrs to
// the cluster entity

extern ClAmsT  gAms;

extern ClAmsEntityConfigT  gClAmsEntityDefaultConfig;
extern ClAmsNodeConfigT  gClAmsNodeDefaultConfig;
extern ClAmsAppConfigT  gClAmsAppDefaultConfig;
extern ClAmsSGConfigT  gClAmsSGDefaultConfig;
extern ClAmsSUConfigT  gClAmsSUDefaultConfig;
extern ClAmsSIConfigT  gClAmsSIDefaultConfig;
extern ClAmsCompConfigT  gClAmsCompDefaultConfig;
extern ClAmsCSIConfigT  gClAmsCSIDefaultConfig;

extern ClAmsEntityMethodsT  gClAmsPeEntityDefaultMethods;
extern ClAmsNodeMethodsT  gClAmsPeNodeDefaultMethods;
extern ClAmsAppMethodsT  gClAmsPeAppDefaultMethods;
extern ClAmsSGMethodsT  gClAmsPeSGDefaultMethods;
extern ClAmsSUMethodsT  gClAmsPeSUDefaultMethods;
extern ClAmsSIMethodsT  gClAmsPeSIDefaultMethods;
extern ClAmsCompMethodsT  gClAmsPeCompDefaultMethods;
extern ClAmsCSIMethodsT  gClAmsPeCSIDefaultMethods;

ClAmsEntityParamsT  gClAmsEntityParams[] =
{
    {
        {
            sizeof("Entity"),
            "Entity"
        },
        &gClAmsEntityDefaultConfig,
        sizeof(ClAmsEntityConfigT),
        &gClAmsPeEntityDefaultMethods,
        sizeof(ClAmsEntityMethodsT),
        sizeof(ClAmsEntityT),
    },
    {
        {
            sizeof("Node"),
            "Node"
        },
        &gClAmsNodeDefaultConfig,
        sizeof(ClAmsNodeConfigT),
        &gClAmsPeNodeDefaultMethods,
        sizeof(ClAmsNodeMethodsT),
        sizeof(ClAmsNodeT),
    },
    {
        {
            sizeof("Application"),
            "Application"
        },
        &gClAmsAppDefaultConfig,
        sizeof(ClAmsAppConfigT),
        &gClAmsPeAppDefaultMethods,
        sizeof(ClAmsAppMethodsT),
        sizeof(ClAmsAppT),
    },
    {
        {
            sizeof("SG"),
            "SG"
        },
        &gClAmsSGDefaultConfig,
        sizeof(ClAmsSGConfigT),
        &gClAmsPeSGDefaultMethods,
        sizeof(ClAmsSGMethodsT),
        sizeof(ClAmsSGT),
    },
    {
        {
            sizeof("SU"),
            "SU"
        },
        &gClAmsSUDefaultConfig,
        sizeof(ClAmsSUConfigT),
        &gClAmsPeSUDefaultMethods,
        sizeof(ClAmsSUMethodsT),
        sizeof(ClAmsSUT),
    },
    {
        {
            sizeof("SI"),
            "SI"
        },
        &gClAmsSIDefaultConfig,
        sizeof(ClAmsSIConfigT),
        &gClAmsPeSIDefaultMethods,
        sizeof(ClAmsSIMethodsT),
        sizeof(ClAmsSIT),
    },
    {
        {
            sizeof("Component"),
            "Component"
        },
        &gClAmsCompDefaultConfig,
        sizeof(ClAmsCompConfigT),
        &gClAmsPeCompDefaultMethods,
        sizeof(ClAmsCompMethodsT),
        sizeof(ClAmsCompT),
    },
    {
        {
            sizeof("CSI"),
            "CSI"
        },
        &gClAmsCSIDefaultConfig,
        sizeof(ClAmsCSIConfigT),
        &gClAmsPeCSIDefaultMethods,
        sizeof(ClAmsCSIMethodsT),
        sizeof(ClAmsCSIT),
    }
};

/******************************************************************************
 * AMS Entity Methods
 *****************************************************************************/

/*
 * clAmsEntityGetDefaults
 * ----------------------
 * Return the default configuration and method pointers for an entity given
 * the type of the entity.
 */

ClRcT
clAmsEntityGetDefaults(
        CL_IN  ClAmsEntityTypeT  type,
        CL_OUT  ClAmsEntityParamsT  **params)
{

    AMS_CHECKPTR( !params );

    AMS_CHECK_ENTITY_TYPE (type);

    *params = (ClAmsEntityParamsT *) &gClAmsEntityParams[type];

    return CL_OK;

}

/*
 * clAmsEntityReset
 * ----------------
 * Reset the status of an entity. Lists and Timers are currently not changed
 * by this fn but that could (should?) be added in the future.
 *
 * Note: Do not use the CL_AMS_SET_<X>_STATE macros in this fn as it gets
 * called when the db is initially populated and inter-entity relationships
 * are not fully set up.
 */

ClRcT
amsEntityReset(
               ClAmsEntityT  *entity,
               ClBoolT init)
{

    ClAmsEntityStatusT  *entityStatus = NULL;
    ClAmsT  *ams = &gAms;
    ClAmsOperStateT lastOperState = CL_AMS_OPER_STATE_NONE;
    ClAmsOperStateT newOperState = CL_AMS_OPER_STATE_NONE;

    AMS_CHECKPTR ( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    clAmsFaultQueueDelete(entity);

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;

            if ( node->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
            {
                node->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
                if(gAms.eventServerReady)
                    clAmsAdminStateNotificationPublish(&node->config.entity, 
                                                       CL_AMS_ADMIN_STATE_SHUTTINGDOWN,
                                                       CL_AMS_ADMIN_STATE_LOCKED_A);
            }

            node->status.presenceState      = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
            lastOperState = node->status.operState;
            newOperState = node->status.operState = CL_AMS_OPER_STATE_DISABLED;
            node->status.isClusterMember    = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;
            node->status.wasMemberBefore    = CL_FALSE;
            node->status.recovery           = CL_AMS_RECOVERY_NONE;
            node->status.suFailoverCount    = 0;
            node->status.numInstantiatedSUs = 0;
            node->status.numAssignedSUs     = 0;
            if(!init 
               &&
               (lastOperState != newOperState))
                clAmsOperStateNotificationPublish((ClAmsEntityT*)&node->config.entity,
                                                  lastOperState, newOperState);
            
            break;
        }

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            if ( sg->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
            {
                sg->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
                if(gAms.eventServerReady)
                    clAmsAdminStateNotificationPublish(&sg->config.entity,
                                                       CL_AMS_ADMIN_STATE_SHUTTINGDOWN,
                                                       CL_AMS_ADMIN_STATE_LOCKED_A);
            }

            sg->status.isStarted           = CL_FALSE;
            sg->status.numCurrActiveSUs    = 0;
            sg->status.numCurrStandbySUs   = 0;
            
            
            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;

            if ( su->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
            {
                su->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
                if(gAms.eventServerReady)
                    clAmsAdminStateNotificationPublish(&su->config.entity,
                                                       CL_AMS_ADMIN_STATE_SHUTTINGDOWN,
                                                       CL_AMS_ADMIN_STATE_LOCKED_A);
            }

            su->status.presenceState       = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
            lastOperState = su->status.operState;
            newOperState = su->status.operState = CL_AMS_OPER_STATE_ENABLED;
            su->status.readinessState      = CL_AMS_READINESS_STATE_OUTOFSERVICE;
            su->status.recovery            = CL_AMS_RECOVERY_NONE;
            su->status.numActiveSIs        = 0;
            su->status.numStandbySIs       = 0;
            su->status.numQuiescedSIs      = 0;
            su->status.compRestartCount    = 0;
            su->status.suRestartCount      = 0;
            su->status.numInstantiatedComp = 0;
            su->status.numDelayAssignments = 0;
            su->status.numWaitAdjustments =  0;
            if(!init 
               && 
               (lastOperState != newOperState))
                clAmsOperStateNotificationPublish(&su->config.entity, lastOperState, newOperState);
            break;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) entity;

            if ( si->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
            {
                si->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
                if(gAms.eventServerReady)
                    clAmsAdminStateNotificationPublish(&si->config.entity,
                                                       CL_AMS_ADMIN_STATE_SHUTTINGDOWN,
                                                       CL_AMS_ADMIN_STATE_LOCKED_A);
            }

            lastOperState = si->status.operState;
            newOperState = si->status.operState = CL_AMS_OPER_STATE_DISABLED;
            si->status.numActiveAssignments  = 0;
            si->status.numStandbyAssignments = 0;
            if(!init 
               &&
               (lastOperState != newOperState))
                clAmsOperStateNotificationPublish(&si->config.entity, lastOperState, newOperState);
            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;

            comp->status.presenceState         = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
            lastOperState = comp->status.operState;
            newOperState = comp->status.operState = CL_AMS_OPER_STATE_ENABLED;
            comp->status.readinessState        = CL_AMS_READINESS_STATE_OUTOFSERVICE;
            comp->status.recovery              = CL_AMS_RECOVERY_NONE;
            comp->status.numActiveCSIs         = 0;
            comp->status.numStandbyCSIs        = 0;
            comp->status.numQuiescingCSIs      = 0;
            comp->status.numQuiescedCSIs       = 0;
            comp->status.restartCount          = 0;
            comp->status.instantiateCount      = 0;
            comp->status.instantiateDelayCount = 0;
            comp->status.amStartCount          = 0;
            comp->status.amStopCount           = 0;

            //comp->status.proxyComp             = NULL;
            
            if(!init
               &&
               (lastOperState != newOperState))
                clAmsOperStateNotificationPublish(&comp->config.entity,
                                                  lastOperState,
                                                  newOperState);
            break;
        }

        default:
        {
            /*
             * This includes entity. cluster, App and CSI. App will be supported in
             * next release. There's nothing to reset for CSI, entity and cluster.
             */
        }

    }

    entityStatus = clAmsEntityGetStatusStruct(entity);

    ams->timerCount -= entityStatus->timerCount;
    entityStatus->timerCount = 0;

    return CL_OK;

}

ClRcT clAmsEntityReset(ClAmsEntityT *pEntity)
{
    return amsEntityReset(pEntity, CL_FALSE);
}

/*
 * clAmsEntityMalloc
 * -----------------
 * Malloc and entity, set up its config, status and methods.
 */

ClAmsEntityT *
clAmsEntityMalloc(
        CL_IN  ClAmsEntityTypeT  type,
        CL_IN  ClAmsEntityConfigT  *config,
        CL_IN  ClAmsEntityMethodsT  *methods )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityT  *newEntity = NULL;
    ClAmsEntityParamsT  *params = NULL;
    ClAmsEntityMethodsT  *useMethods = NULL;
    ClAmsEntityConfigT  *useConfig = NULL;
    ClAmsEntityStatusT *status = NULL;

    /*
     * Allocate a new entity and initialize it appropriately.
     */

    if ( clAmsEntityGetDefaults(type, &params) != CL_OK )
    {
        return (ClAmsEntityT *) NULL;
    }

    useConfig = config ? config : params->defaultConfig;
    useMethods = methods ? methods : params->defaultMethods; 
    
    newEntity = clHeapCalloc(1, params->entitySize);

    if ( newEntity == NULL )
    {
        AMS_LOG (CL_DEBUG_ERROR, ("Malloc failed for operation EntityMalloc\n"));
        return (ClAmsEntityT *)NULL;
    }


    /*
     * Setup default config for entity.
     */

    memcpy(newEntity, useConfig, params->configSize);

    status = clAmsEntityGetStatusStruct(newEntity);
    if(status)
    {
        CL_LIST_HEAD_INIT(&status->opStack.opList);
        status->opStack.numOps = 0;
    }

    /*
     * Setup entity methods and create entity specific lists.
     * 
     * Note: Entity timers are deliberately not created now as they would then
     * pick up the currently set duration values (Clovis OSAL restriction). It 
     * is possible that these will be configured after the entity has been 
     * malloc'd.
     */

    if ( (type < 0) || (type > CL_AMS_ENTITY_TYPE_MAX) )
    { 
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Invalid entity type = %d\n", type));
        goto exitfn;
    }

    switch ( type )
    {
       
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) newEntity;

            /*
             * Methods
             */

            memcpy(&node->methods,
                    (ClAmsNodeMethodsT *) useMethods,
                    sizeof(ClAmsNodeMethodsT));

            /*
             * Config
             */ 
            
            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &node->config.nodeDependentsList,
                        CL_AMS_ENTITY_TYPE_NODE) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &node->config.nodeDependenciesList,
                        CL_AMS_ENTITY_TYPE_NODE) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate( 
                        &node->config.suList,
                        CL_AMS_ENTITY_TYPE_SU) );

            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            ClAmsAppT  *app = (ClAmsAppT *) newEntity;

            /*
             * Methods
             */

            memcpy(&app->methods,
                    (ClAmsAppMethodsT *) useMethods,
                    sizeof(ClAmsAppMethodsT));

            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) newEntity;

            /*
             * Methods
             */

            memcpy(&sg->methods,
                    (ClAmsSGMethodsT *) useMethods,
                    sizeof(ClAmsSGMethodsT));

            /*
             * Config
             */

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->config.suList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->config.siList,
                        CL_AMS_ENTITY_TYPE_SI) );

            /*
             * Status
             */

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->status.instantiableSUList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->status.instantiatedSUList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->status.inserviceSpareSUList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &sg->status.assignedSUList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &sg->status.faultySUList,
                        CL_AMS_ENTITY_TYPE_SU) );

            CL_LIST_HEAD_INIT(&sg->status.failoverHistory);

            sg->status.failoverHistoryIndex = 0;
            sg->status.failoverHistoryCount = 0;

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) newEntity;

            /*
             * Methods
             */

            memcpy(&su->methods,
                    (ClAmsSUMethodsT *) useMethods,
                    sizeof(ClAmsSUMethodsT));

            /*
             * Config
             */

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &su->config.compList,
                        CL_AMS_ENTITY_TYPE_COMP) );

            /*
             * Status
             */

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &su->status.siList,
                        CL_AMS_ENTITY_TYPE_SI) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) newEntity;

            /*
             * Methods
             */

            memcpy(&si->methods,
                    (ClAmsSIMethodsT *) useMethods,
                    sizeof(ClAmsSIMethodsT));

            /*
             * Config
             */

            AMS_CHECK_RC_ERROR( clAmsEntityOrderedListInstantiate(
                        &si->config.suList,
                        CL_AMS_ENTITY_TYPE_SU) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &si->config.siDependentsList,
                        CL_AMS_ENTITY_TYPE_SI) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &si->config.siDependenciesList,
                        CL_AMS_ENTITY_TYPE_SI) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &si->config.csiList,
                        CL_AMS_ENTITY_TYPE_CSI) );

            /*
             * Status
             */

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &si->status.suList,
                        CL_AMS_ENTITY_TYPE_SU) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) newEntity;

            /*
             * Methods
             */

            memcpy(&comp->methods,
                    (ClAmsCompMethodsT *) useMethods,
                    sizeof(ClAmsCompMethodsT));

            /*
             * Status
             */

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &comp->status.csiList,
                        CL_AMS_ENTITY_TYPE_CSI) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT  *csi = (ClAmsCSIT *) newEntity;

            /*
             * Methods
             */

            memcpy(&csi->methods,
                    (ClAmsCSIMethodsT *) useMethods,
                    sizeof(ClAmsCSIMethodsT));

            /*
             * Config
             */

           AMS_CHECK_RC_ERROR( clAmsCSINVPListInstantiate(
                       &csi->config.nameValuePairList,
                       CL_AMS_ENTITY_TYPE_CSI) );

           AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &csi->config.csiDependentsList,
                        CL_AMS_ENTITY_TYPE_CSI) );

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &csi->config.csiDependenciesList,
                        CL_AMS_ENTITY_TYPE_CSI) );

           /*
            * Status
            */

            AMS_CHECK_RC_ERROR( clAmsEntityListInstantiate(
                        &csi->status.pgList,
                        CL_AMS_ENTITY_TYPE_COMP) );

            AMS_CHECK_RC_ERROR( clAmsCSIPGTrackListInstantiate(
                        &csi->status.pgTrackList,
                        CL_AMS_ENTITY_TYPE_CSI) );

            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App and cluster
             */
        }
    }

    /*
     * Setup default status for entity.
     */

    amsEntityReset(newEntity, CL_TRUE);

    return newEntity;

exitfn:

    clHeapFree(newEntity);
    return (ClAmsEntityT *) NULL;

}

/*
 * clAmsEntityFree
 * ---------------
 * Free an entity malloc'd through clAmsEntityMalloc
 */

ClRcT
clAmsEntityFree(
        CL_IN  ClAmsEntityT  *entity )
{
    AMS_CHECKPTR (!entity);

    AMS_CHECK_ENTITY_TYPE (entity->type);

    _clAmsEntityUserDataDelete(&entity->name, CL_TRUE);

    /*
     * Delete entity specific items such as linked lists and timers.
     */


    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;

            AMS_CALL ( clAmsEntityListTerminate(
                            &node->config.nodeDependentsList) );

            AMS_CALL ( clAmsEntityListTerminate(
                            &node->config.nodeDependenciesList) );

            AMS_CALL ( clAmsEntityListTerminate(
                            &node->config.suList) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) node,
                        CL_AMS_NODE_TIMER_SUFAILOVER) );
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;

            /*
             * Nothing to do for now.
             */

            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;

            AMS_CALL ( clAmsEntityListTerminate(&sg->config.suList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->config.siList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->status.instantiableSUList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->status.instantiatedSUList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->status.inserviceSpareSUList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->status.assignedSUList) );
            AMS_CALL ( clAmsEntityListTerminate(&sg->status.faultySUList) );
            AMS_CALL ( clAmsSGFailoverHistoryDelete(sg) );
            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) sg,
                        CL_AMS_SG_TIMER_INSTANTIATE) );
            
            AMS_CALL (clAmsEntityTimerDelete(
                                             (ClAmsEntityT*)sg,
                                             CL_AMS_SG_TIMER_ADJUST) );

            AMS_CALL (clAmsEntityTimerDelete(
                                             (ClAmsEntityT*)sg,
                                             CL_AMS_SG_TIMER_ADJUST_PROBATION) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;
            
            clAmsPeSUSIReassignEntryDelete(su);

            AMS_CALL ( clAmsEntityListTerminate(&su->config.compList) );
            AMS_CALL ( clAmsEntityListTerminate(&su->status.siList) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) su,
                        CL_AMS_SU_TIMER_SURESTART) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) su,
                        CL_AMS_SU_TIMER_COMPRESTART) );

            AMS_CALL ( clAmsEntityTimerDelete(
                                              (ClAmsEntityT*) su,
                                              CL_AMS_SU_TIMER_PROBATION) );

            AMS_CALL ( clAmsEntityTimerDelete(
                                              (ClAmsEntityT*) su,
                                              CL_AMS_SU_TIMER_ASSIGNMENT) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) entity;

            AMS_CALL ( clAmsEntityListTerminate(&si->config.suList) );
            AMS_CALL ( clAmsEntityListTerminate(&si->config.siDependentsList) );
            AMS_CALL ( clAmsEntityListTerminate(&si->config.siDependenciesList) );
            AMS_CALL ( clAmsEntityListTerminate(&si->config.csiList) );
            AMS_CALL ( clAmsEntityListTerminate(&si->status.suList) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;

            AMS_CALL ( clAmsEntityListTerminate(&comp->status.csiList) );

            if(comp->config.pSupportedCSITypes)
            {
                clAmsFreeMemory(comp->config.pSupportedCSITypes);
            }

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_INSTANTIATE) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_TERMINATE) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_CLEANUP) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_AMSTART) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_AMSTOP) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_CSISET) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_CSIREMOVE) );

            AMS_CALL ( clAmsEntityTimerDelete(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_INSTANTIATEDELAY) );

            break;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;

            AMS_CALL ( clAmsEntityListTerminate(&csi->status.pgList) );
            AMS_CALL ( clAmsEntityListTerminate(&csi->config.csiDependentsList) );
            AMS_CALL ( clAmsEntityListTerminate(&csi->config.csiDependenciesList) );
            AMS_CALL ( clCntAllNodesDelete(csi->config.nameValuePairList) );
            AMS_CALL ( clCntDelete(csi->config.nameValuePairList) );
            AMS_CALL ( clCntAllNodesDelete(csi->status.pgTrackList) );
            AMS_CALL ( clCntDelete(csi->status.pgTrackList) );


            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App, cluster
             */
        }
    }

    clAmsEntityOpsClear(entity, NULL);

    clHeapFree((void *)entity);

    return CL_OK;

}

/*
 * clAmsEntityGetStatusStruct
 * --------------------------
 * Given an entity ptr, return a ptr to the status section. This is used by
 * fns to access common data in status without knowing entity type. This fn
 * could be deprecated in the future if the structure of clAmsEntity is
 * changed.
 */

ClAmsEntityStatusT *
clAmsEntityGetStatusStruct(
        ClAmsEntityT  *entity )
{

    if (!entity) 
    {
        return NULL;
    }

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *e = (ClAmsNodeT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            ClAmsAppT *e = (ClAmsAppT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT *e = (ClAmsSGT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT *e = (ClAmsSUT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *e = (ClAmsCompT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT *e = (ClAmsSIT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT *e = (ClAmsCSIT *) entity;
            return (ClAmsEntityStatusT *) &e->status;
        }

        default:
        {
            return NULL;
        }
    }

}

/*
 * clAmsEntityTerminate
 * ----------------
 * Terminate the Entity
 */

ClRcT
clAmsEntityTerminate(
        CL_IN  ClAmsEntityT  *entity )
{

    AMS_CHECKPTR ( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            AMS_CALL (clAmsEntityListTerminate(&node->config.nodeDependentsList));
            AMS_CALL (clAmsEntityListTerminate(&node->config.nodeDependenciesList));
            AMS_CALL (clAmsEntityListTerminate(&node->config.suList));
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;
            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            AMS_CALL (clAmsEntityListTerminate(&sg->config.siList));
            AMS_CALL (clAmsEntityListTerminate(&sg->config.suList));

            AMS_CALL (clAmsEntityListTerminate(&sg->status.instantiableSUList));
            AMS_CALL (clAmsEntityListTerminate(&sg->status.instantiatedSUList));
            AMS_CALL (clAmsEntityListTerminate(&sg->status.inserviceSpareSUList));
            AMS_CALL (clAmsEntityListTerminate(&sg->status.assignedSUList));
            AMS_CALL (clAmsEntityListTerminate(&sg->status.faultySUList));

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;
            AMS_CALL (clAmsEntityListTerminate(&su->config.compList));
            AMS_CALL (clAmsEntityListTerminate(&su->status.siList));
            break;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) entity;
            AMS_CALL (clAmsEntityListTerminate(&si->config.suList));
            AMS_CALL (clAmsEntityListTerminate(&si->config.siDependentsList));
            AMS_CALL (clAmsEntityListTerminate(&si->config.siDependenciesList));
            AMS_CALL (clAmsEntityListTerminate(&si->config.csiList));
            AMS_CALL (clAmsEntityListTerminate(&si->status.suList));

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;
            AMS_CALL (clAmsEntityListTerminate(&comp->status.csiList));
            break;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;
            AMS_CALL (clCntAllNodesDelete(csi->config.nameValuePairList));
            AMS_CALL (clCntDelete(csi->config.nameValuePairList));
            AMS_CALL (clAmsEntityListTerminate(&csi->status.pgList));
            AMS_CALL (clCntAllNodesDelete(csi->status.pgTrackList));
            AMS_CALL (clCntDelete(csi->status.pgTrackList));
            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App, cluster
             */
        }
    }

    return CL_OK;

}

static ClCharT *clAmsEpochString(ClTimeT epoch, ClCharT *pStr)
{
    if(!epoch || !pStr) return NULL;
    ctime_r((const time_t*)&epoch, pStr);
    if(pStr[strlen(pStr)-1] == '\n')
        pStr[strlen(pStr)-1] = 0;
    if(pStr[strlen(pStr)-2] == '\r')
        pStr[strlen(pStr)-2] = 0;
    return pStr;
}

/*
 * clAmsEntityPrint
 * ----------------
 * Print out the contents of an Entity
 */

ClRcT
clAmsEntityPrint(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsAdminStateT  adminState = 0;
    ClAmsEntityStatusT  *entityStatus = NULL;
    ClCharT epochStr[CL_MAX_NAME_LENGTH];
    AMS_CHECKPTR ( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    CL_AMS_PRINT_TWO_COL("Name","%s", entity->name.value);

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClRcT instantiable = (clAmsPeNodeIsInstantiable(node) == CL_OK);
            
            if ( clAmsPeNodeComputeAdminState(node, &adminState) != CL_OK)
            {
                adminState = node->config.adminState;
            }

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                                 "%s","---------------------------");
            if(node->status.entity.epoch)
                CL_AMS_PRINT_TWO_COL("Start time",
                                     "%s", 
                                     clAmsEpochString(node->status.entity.epoch, epochStr));
            CL_AMS_PRINT_TWO_COL("Admin State",
                    "%s",CL_AMS_STRING_A_STATE(node->config.adminState));
            CL_AMS_PRINT_TWO_COL("Computed Admin State",
                    "%s",CL_AMS_STRING_A_STATE(adminState));
            CL_AMS_PRINT_TWO_COL("Id",
                    "%d",node->config.id);
            CL_AMS_PRINT_TWO_COL("Class Type",
                    "%s",CL_AMS_STRING_NODE_CLASSTYPE(node->config.classType));
            CL_AMS_PRINT_TWO_COL("SubClass Type",
                    "%s",node->config.subClassType.value);
            CL_AMS_PRINT_TWO_COL("Is Swappable",
                    "%s",CL_AMS_STRING_BOOLEAN(node->config.isSwappable));
            CL_AMS_PRINT_TWO_COL("Is Restartable",
                    "%s",CL_AMS_STRING_BOOLEAN(node->config.isRestartable));
            CL_AMS_PRINT_TWO_COL("Is ASP Aware",
                    "%s",CL_AMS_STRING_BOOLEAN(node->config.isASPAware));
            CL_AMS_PRINT_TWO_COL("Auto Repair on Join",
                    "%s",CL_AMS_STRING_BOOLEAN(node->config.autoRepair));
            CL_AMS_PRINT_TWO_COL("SU Failover Probation Time",
                    "%lld ms", (ClInt64T) node->config.suFailoverDuration);
            CL_AMS_PRINT_TWO_COL("SU Failover Count",
                    "%d", node->config.suFailoverCountMax);
            CL_AMS_PRINT_TWO_COL("Node Dependents",
                    "Count[%u]",
                    node->config.nodeDependentsList.numEntities);
            clAmsEntityListPrint(
                    &node->config.nodeDependentsList,
                    CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST);

            CL_AMS_PRINT_TWO_COL("Node Dependencies",
                    "Count[%u]",
                    node->config.nodeDependenciesList.numEntities);
            clAmsEntityListPrint(
                    &node->config.nodeDependenciesList,
                    CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST);


            CL_AMS_PRINT_TWO_COL("SUs in Node",
                    "Count[%u]",
                    node->config.suList.numEntities);
            clAmsEntityListPrint(
                    &node->config.suList,
                    CL_AMS_NODE_CONFIG_SU_LIST);

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Presence State",
                    "%s",CL_AMS_STRING_P_STATE(node->status.presenceState));
            CL_AMS_PRINT_TWO_COL("Oper State",
                    "%s",CL_AMS_STRING_O_STATE(node->status.operState));
            CL_AMS_PRINT_TWO_COL("Is Instantiable",
                    "%s",CL_AMS_STRING_BOOLEAN(instantiable));
            CL_AMS_PRINT_TWO_COL("Is Cluster Member",
                    "%s",
                    CL_AMS_STRING_NODE_ISCLUSTERMEMBER(node->status.isClusterMember));
            CL_AMS_PRINT_TWO_COL("Was Cluster Member Before",
                    "%s",CL_AMS_STRING_BOOLEAN(node->status.wasMemberBefore));
            CL_AMS_PRINT_TWO_COL("Last Recovery",
                    "%s", CL_AMS_STRING_RECOVERY(node->status.recovery));
            CL_AMS_PRINT_TWO_COL("Num Instantiated SUs",
                    "%u", node->status.numInstantiatedSUs);
            CL_AMS_PRINT_TWO_COL("Num Assigned SUs",
                    "%u", node->status.numAssignedSUs);
            CL_AMS_PRINT_TWO_COL("SU Failover Probation Timer",
                    "%s", node->status.suFailoverTimer.count ?
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("SU Failover Count",
                    "%d", node->status.suFailoverCount);
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;

            if (clAmsPeSGComputeAdminState(sg, &adminState) != CL_OK)
            {
                adminState = sg->config.adminState;
            }

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");
            if(sg->status.entity.epoch)
                CL_AMS_PRINT_TWO_COL("Start time",
                                     "%s", 
                                     clAmsEpochString(sg->status.entity.epoch, epochStr));
            /*Future
            CL_AMS_PRINT_TWO_COL("Member of Application",
                  "%s",sg->config.parentApp.entity.name.value);
            */
            CL_AMS_PRINT_TWO_COL("Admin State",
                    "%s",CL_AMS_STRING_A_STATE(sg->config.adminState));
            CL_AMS_PRINT_TWO_COL("Computed Admin State",
                    "%s",CL_AMS_STRING_A_STATE(adminState));
            CL_AMS_PRINT_TWO_COL("Redundancy Model",
                    "%s",CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.redundancyModel));
            CL_AMS_PRINT_TWO_COL("Loading Strategy",
                    "%s",CL_AMS_STRING_SG_LOADING_STRATEGY(sg->config.loadingStrategy));
            CL_AMS_PRINT_TWO_COL("Failback Option",
                    "%s",CL_AMS_STRING_BOOLEAN(sg->config.failbackOption));
            CL_AMS_PRINT_TWO_COL("Auto Repair",
                    "%s",CL_AMS_STRING_BOOLEAN(sg->config.autoRepair));
            CL_AMS_PRINT_TWO_COL("SU Instantiate Duration",
                    "%lld ms", sg->config.instantiateDuration);
            CL_AMS_PRINT_TWO_COL("Preferred Active SUs",
                    "%u",sg->config.numPrefActiveSUs);
            CL_AMS_PRINT_TWO_COL("Preferred Standby SUs",
                    "%u",sg->config.numPrefStandbySUs);
            CL_AMS_PRINT_TWO_COL("Preferred Inservice SUs",
                    "%u",sg->config.numPrefInserviceSUs);
            CL_AMS_PRINT_TWO_COL("Preferred Assigned SUs",
                    "%u",sg->config.numPrefAssignedSUs);
            CL_AMS_PRINT_TWO_COL("Preferred Active SUs per SI",
                    "%u",sg->config.numPrefActiveSUsPerSI);
            CL_AMS_PRINT_TWO_COL("Max Active SIs per SU",
                    "%u",sg->config.maxActiveSIsPerSU);
            CL_AMS_PRINT_TWO_COL("Max Standby SIs per SU",
                    "%u",sg->config.maxStandbySIsPerSU);
            CL_AMS_PRINT_TWO_COL("Component Restart Probation Time",
                    "%lld ms", sg->config.compRestartDuration);
            CL_AMS_PRINT_TWO_COL("Component Restart Count",
                    "%d",sg->config.compRestartCountMax);
            CL_AMS_PRINT_TWO_COL("SU Restart Probation Time",
                    "%lld ms", sg->config.suRestartDuration);
            CL_AMS_PRINT_TWO_COL("SU Restart Count",
                    "%d",sg->config.suRestartCountMax);
            CL_AMS_PRINT_TWO_COL("Collocation Allowed",
                    "%s",CL_AMS_STRING_BOOLEAN(sg->config.isCollocationAllowed));
            CL_AMS_PRINT_TWO_COL("Alpha Factor", "%d",sg->config.alpha);

            CL_AMS_PRINT_TWO_COL("Beta Factor", "%d",sg->config.beta);

            CL_AMS_PRINT_TWO_COL("Reduction procedure",
                                 "%s",CL_AMS_STRING_BOOLEAN(sg->config.reductionProcedure));

            CL_AMS_PRINT_TWO_COL("Auto adjust",
                                 "%s",CL_AMS_STRING_BOOLEAN(sg->config.autoAdjust));

            CL_AMS_PRINT_TWO_COL("Auto adjust probation",
                                 "%lld ms", sg->config.autoAdjustProbation);

            CL_AMS_PRINT_TWO_COL("Max failovers",
                                 "%d", sg->config.maxFailovers);

            CL_AMS_PRINT_TWO_COL("Failover duration",
                                 "%lld ms", sg->config.failoverDuration);

            CL_AMS_PRINT_TWO_COL("SU List",
                    "Count[%u]",
                    sg->config.suList.numEntities);
            clAmsEntityListPrint(
                    &sg->config.suList,
                    CL_AMS_SG_CONFIG_SU_LIST);
            CL_AMS_PRINT_TWO_COL("Configured SI List",
                    "Count[%u]",
                    sg->config.siList.numEntities);
            clAmsEntityListPrint(
                    &sg->config.siList,
                    CL_AMS_SG_CONFIG_SI_LIST);

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Is Started",
                    "%s",CL_AMS_STRING_BOOLEAN(sg->status.isStarted));
            CL_AMS_PRINT_TWO_COL("Instantiate Timer",
                    "%s", sg->status.instantiateTimer.count ?
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Instantiable SUs",
                    "Count[%u]",
                    sg->status.instantiableSUList.numEntities);
            clAmsEntityListPrint(
                    &sg->status.instantiableSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);
            CL_AMS_PRINT_TWO_COL("Instantiated SUs",
                    "Count[%u]",
                    sg->status.instantiatedSUList.numEntities);
            clAmsEntityListPrint(
                    &sg->status.instantiatedSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);
            CL_AMS_PRINT_TWO_COL("Inservice Spare SUs",
                    "Count[%u]",
                    sg->status.inserviceSpareSUList.numEntities);
            clAmsEntityListPrint(
                    &sg->status.inserviceSpareSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);
            CL_AMS_PRINT_TWO_COL("Assigned SUs",
                    "Count[%u]",
                    sg->status.assignedSUList.numEntities);
            clAmsEntityListPrint(
                    &sg->status.assignedSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);

            /* Not used anywhere for now..
            CL_AMS_PRINT_TWO_COL("Faulty SU",
                    "%u",sg->status.faultySUList.numEntities);
            */

            CL_AMS_PRINT_TWO_COL("Active SU (M+N, 2N)",
                    "%u",sg->status.numCurrActiveSUs);
            CL_AMS_PRINT_TWO_COL("Standby SU (M+N, 2N)",
                    "%u",sg->status.numCurrStandbySUs);

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;

            if( clAmsPeSUComputeAdminState(su, &adminState) != CL_OK)
            {
                adminState = su->config.adminState;
            }

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");
            if(su->status.entity.epoch)
                CL_AMS_PRINT_TWO_COL("Start time",
                                     "%s",
                                     clAmsEpochString(su->status.entity.epoch, epochStr));
            CL_AMS_PRINT_TWO_COL("Member of SG",
                    "%s",su->config.parentSG.entity.name.value);
            CL_AMS_PRINT_TWO_COL("Member of Node",
                    "%s",su->config.parentNode.entity.name.value);
            CL_AMS_PRINT_TWO_COL("Admin State",
                    "%s",CL_AMS_STRING_A_STATE(su->config.adminState));
            CL_AMS_PRINT_TWO_COL("Computed Admin State",
                    "%s",CL_AMS_STRING_A_STATE(adminState));
            CL_AMS_PRINT_TWO_COL("Rank",
                    "%d",su->config.rank);
            CL_AMS_PRINT_TWO_COL("Number of Components",
                    "%u",su->config.numComponents);
            CL_AMS_PRINT_TWO_COL("Is Preinstantiable",
                    "%s",CL_AMS_STRING_BOOLEAN(su->config.isPreinstantiable));
            CL_AMS_PRINT_TWO_COL("Is Restartable",
                    "%s",CL_AMS_STRING_BOOLEAN(su->config.isRestartable));
            /* Future
            CL_AMS_PRINT_TWO_COL("Is ContainerSU",
                    "%s",su->config.isContainerSU ? "True" : "False");
             */

            CL_AMS_PRINT_TWO_COL("Components List",
                    "Count[%u]",
                    su->config.compList.numEntities);
            clAmsEntityListPrint(
                    &su->config.compList,
                    CL_AMS_SU_CONFIG_COMP_LIST);

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Presence State",
                    "%s",CL_AMS_STRING_P_STATE(su->status.presenceState));
            CL_AMS_PRINT_TWO_COL("Oper State",
                    "%s",CL_AMS_STRING_O_STATE(su->status.operState));
            CL_AMS_PRINT_TWO_COL("Readiness State",
                    "%s",CL_AMS_STRING_R_STATE(su->status.readinessState));
            CL_AMS_PRINT_TWO_COL("Last Recovery",
                    "%s", CL_AMS_STRING_RECOVERY(su->status.recovery));
            CL_AMS_PRINT_TWO_COL("Active SI",
                    "%u",su->status.numActiveSIs);
            CL_AMS_PRINT_TWO_COL("Standby SI",
                    "%u",su->status.numStandbySIs);
            CL_AMS_PRINT_TWO_COL("Quiesced SI",
                    "%u",su->status.numQuiescedSIs);
            CL_AMS_PRINT_TWO_COL("Num Instantiated Components",
                    "%u",su->status.numInstantiatedComp);
            CL_AMS_PRINT_TWO_COL("Num Pre-Instantiated Components",
                    "%u",su->status.numPIComp);
            CL_AMS_PRINT_TWO_COL("Comp Restart Probation Time",
                    "%s", su->status.compRestartTimer.count ?
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Comp Restart Count",
                    "%d", su->status.compRestartCount);
            CL_AMS_PRINT_TWO_COL("SU Restart Probation Time",
                    "%s", su->status.suRestartTimer.count ?
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("SU Restart Count",
                    "%d", su->status.suRestartCount);
            CL_AMS_PRINT_TWO_COL("SU instantiate level",
                    "%d", su->status.instantiateLevel);

            CL_AMS_PRINT_TWO_COL("SI List",
                    "Count[%u]",
                    su->status.siList.numEntities);
            clAmsEntityListPrint(
                    &su->status.siList,
                    CL_AMS_SU_STATUS_SI_LIST);

            break;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) entity;

            if( clAmsPeSIComputeAdminState(si, &adminState) != CL_OK)
            {
                adminState = si->config.adminState;
            }

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Member of SG",
                    "%s",si->config.parentSG.entity.name.value);
            CL_AMS_PRINT_TWO_COL("Admin State",
                    "%s",CL_AMS_STRING_A_STATE(si->config.adminState));
            CL_AMS_PRINT_TWO_COL("Computed Admin State",
                    "%s",CL_AMS_STRING_A_STATE(adminState));
            CL_AMS_PRINT_TWO_COL("Rank",
                    "%d",si->config.rank);
            CL_AMS_PRINT_TWO_COL("Number of CSIs",
                    "%u",si->config.numCSIs);
            CL_AMS_PRINT_TWO_COL("Standby Assignments",
                    "%u",si->config.numStandbyAssignments);

            CL_AMS_PRINT_TWO_COL("SU List",
                    "Count[%u]",
                    si->config.suList.numEntities);
            clAmsEntityListPrint(
                    &si->config.suList,
                    CL_AMS_SI_CONFIG_SU_RANK_LIST);

            CL_AMS_PRINT_TWO_COL("SI Dependents List",
                    "Count[%u]",
                    si->config.siDependentsList.numEntities);
            clAmsEntityListPrint(
                    &si->config.siDependentsList,
                    CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST);

            CL_AMS_PRINT_TWO_COL("SI Dependencies List",
                    "Count[%u]",
                    si->config.siDependenciesList.numEntities);
            clAmsEntityListPrint(
                    &si->config.siDependenciesList,
                    CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST);

            CL_AMS_PRINT_TWO_COL("SI CSI List",
                    "Count[%u]",
                    si->config.csiList.numEntities);
            clAmsEntityListPrint(
                    &si->config.csiList,
                    CL_AMS_SI_CONFIG_CSI_LIST);

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Oper State",
                    "%s",CL_AMS_STRING_O_STATE(si->status.operState));
            CL_AMS_PRINT_TWO_COL("Active Assignments",
                    "%u",si->status.numActiveAssignments);
            CL_AMS_PRINT_TWO_COL("Standby Assignments",
                    "%u",si->status.numStandbyAssignments);

            CL_AMS_PRINT_TWO_COL("SU List",
                    "Count[%u]",
                    si->status.suList.numEntities);
            clAmsEntityListPrint(
                    &si->status.suList,
                    CL_AMS_SI_STATUS_SU_LIST);

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;

            ClUint32T i;

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");
            if(comp->status.entity.epoch)
                CL_AMS_PRINT_TWO_COL("Start time",
                                     "%s",
                                     clAmsEpochString(comp->status.entity.epoch, epochStr));
            CL_AMS_PRINT_TWO_COL("Member of SU",
                    "%s",comp->config.parentSU.entity.name.value);
            
            CL_AMS_PRINT_TWO_COL("Instantiate command",
                                 "%s", comp->config.instantiateCommand);

            CL_AMS_PRINT_TWO_COL("Supported CSI Types",
                                 "Count[%d]",
                                 comp->config.numSupportedCSITypes);
            
            for(i = 0; i < comp->config.numSupportedCSITypes; ++i)
            {
                CL_AMS_PRINT_TWO_COL("","   %s",
                                     comp->config.pSupportedCSITypes[i].value);
            }

            CL_AMS_PRINT_TWO_COL("Proxy CSI Type",
                    "%s",comp->config.proxyCSIType.value);
            CL_AMS_PRINT_TWO_COL("Capability Model",
                    "%s",CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel));
            CL_AMS_PRINT_TWO_COL("Property",
                    "%s",CL_AMS_STRING_COMP_PROPERTY(comp->config.property));
            CL_AMS_PRINT_TWO_COL("Is Restartable",
                    "%s",CL_AMS_STRING_BOOLEAN(comp->config.isRestartable));
            CL_AMS_PRINT_TWO_COL("Reboot Node if cleanup fails",
                    "%s",CL_AMS_STRING_BOOLEAN(comp->config.nodeRebootCleanupFail));
            CL_AMS_PRINT_TWO_COL("Instantiate Level",
                    "%u",comp->config.instantiateLevel);
            CL_AMS_PRINT_TWO_COL("Command line",
                    "%s",comp->config.instantiateCommand);
            CL_AMS_PRINT_TWO_COL("Max Instantiate Attempts",
                    "%u",comp->config.numMaxInstantiate);
            CL_AMS_PRINT_TWO_COL("Max Instantiate Attempts With Delay",
                    "%u",comp->config.numMaxInstantiateWithDelay);
            /*Future
            CL_AMS_PRINT_TWO_COL("Max Terminate Attempts",
                    "%u",comp->config.numMaxTerminate);
            CL_AMS_PRINT_TWO_COL("Max AM Start Attempts",
                    "%u",comp->config.numMaxAmStart);
            CL_AMS_PRINT_TWO_COL("Max AM Stop Attempts",
                    "%u",comp->config.numMaxAmStop);
            */
            CL_AMS_PRINT_TWO_COL("Max Active CSI",
                    "%u",comp->config.numMaxActiveCSIs);
            CL_AMS_PRINT_TWO_COL("Max Standby CSI",
                    "%u",comp->config.numMaxStandbyCSIs);
            CL_AMS_PRINT_TWO_COL("Instantiate Timeout",
                    "%lld ms",comp->config.timeouts.instantiate);
            CL_AMS_PRINT_TWO_COL("Instantiate Delay Timeout",
                    "%lld ms",comp->config.timeouts.instantiateDelay);
            CL_AMS_PRINT_TWO_COL("Terminate Timeout",
                    "%lld ms",comp->config.timeouts.terminate);
            CL_AMS_PRINT_TWO_COL("Cleanup Timeout",
                    "%lld ms",comp->config.timeouts.cleanup);
            /*Future
            CL_AMS_PRINT_TWO_COL("AMStart Timeout",
                    "%lld ms",comp->config.timeouts.amStart);
            CL_AMS_PRINT_TWO_COL("AMStop Timeout",
                    "%lld ms",comp->config.timeouts.amStop);
            */
            CL_AMS_PRINT_TWO_COL("Quiescing Complete Timeout",
                    "%lld ms",comp->config.timeouts.quiescingComplete);
            CL_AMS_PRINT_TWO_COL("CSISet Timeout",
                    "%lld ms",comp->config.timeouts.csiSet);
            CL_AMS_PRINT_TWO_COL("CSIRemove Timeout",
                    "%lld ms",comp->config.timeouts.csiRemove);
            CL_AMS_PRINT_TWO_COL("Proxied Component Instantiate Timeout",
                    "%lld ms",comp->config.timeouts.proxiedCompInstantiate);
            CL_AMS_PRINT_TWO_COL("Proxied Component Cleanup Timeout",
                    "%lld ms",comp->config.timeouts.proxiedCompCleanup);
            CL_AMS_PRINT_TWO_COL("Recovery On Error",
                    "%s",CL_AMS_STRING_RECOVERY(comp->config.recoveryOnTimeout));


            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Proxy for Component",
                    "%s",comp->status.proxyComp ?
                            comp->status.proxyComp->name.value : "None");
            CL_AMS_PRINT_TWO_COL("Presence State",
                    "%s",CL_AMS_STRING_P_STATE(comp->status.presenceState));
            CL_AMS_PRINT_TWO_COL("Oper State",
                    "%s",CL_AMS_STRING_O_STATE(comp->status.operState));
            CL_AMS_PRINT_TWO_COL("Readiness State",
                    "%s",CL_AMS_STRING_R_STATE(comp->status.readinessState));
            CL_AMS_PRINT_TWO_COL("Last Recovery",
                    "%s", CL_AMS_STRING_RECOVERY(comp->status.recovery));
            CL_AMS_PRINT_TWO_COL("Active CSI",
                    "%u",comp->status.numActiveCSIs);
            CL_AMS_PRINT_TWO_COL("Standby CSI",
                    "%u",comp->status.numStandbyCSIs);
            CL_AMS_PRINT_TWO_COL("Quiescing CSI",
                    "%u",comp->status.numQuiescingCSIs);
            CL_AMS_PRINT_TWO_COL("Quiesced CSI",
                    "%u",comp->status.numQuiescedCSIs);
            CL_AMS_PRINT_TWO_COL("Component Restart Count",
                    "%u",comp->status.restartCount);
            CL_AMS_PRINT_TWO_COL("Instantiate Count",
                    "%u",comp->status.instantiateCount);
            CL_AMS_PRINT_TWO_COL("Instantiate with Delay Count",
                    "%u",comp->status.instantiateDelayCount);
            /* Future
            CL_AMS_PRINT_TWO_COL("Active Monitor Start Count",
                    "%u",comp->status.amStartCount);
            CL_AMS_PRINT_TWO_COL("Active Monitor Stop Count",
                    "%u",comp->status.amStopCount);
            */

            CL_AMS_PRINT_TWO_COL("Instantiate Timer",
                    "%s", comp->status.timers.instantiate.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Instantiate Delay Timer",
                    "%s", comp->status.timers.instantiateDelay.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Terminate Timer",
                    "%s", comp->status.timers.terminate.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Cleanup Timer",
                    "%s", comp->status.timers.cleanup.count ? 
                            "Running" : "Inactive");
            /* Future
            CL_AMS_PRINT_TWO_COL("AMStart Timer",
                    "%s", comp->status.timers.amStart.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("AMStop Timer",
                    "%s", comp->status.timers.amStop.count ? 
                            "Running" : "Inactive");
            */
            CL_AMS_PRINT_TWO_COL("Quiescing Complete Timer",
                    "%s", comp->status.timers.quiescingComplete.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("CSISet Timer",
                    "%s", comp->status.timers.csiSet.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("CSIRemove Timer",
                    "%s", comp->status.timers.csiRemove.count ? 
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Proxied Component Instantiate Timer",
                    "%s", comp->status.timers.proxiedCompInstantiate.count ?
                            "Running" : "Inactive");
            CL_AMS_PRINT_TWO_COL("Proxied Component Cleanup Timer",
                    "%s", comp->status.timers.proxiedCompCleanup.count ?
                            "Running" : "Inactive");

            CL_AMS_PRINT_TWO_COL("CSI List",
                    "Count[%u]",
                    comp->status.csiList.numEntities);
            clAmsEntityListPrint(
                    &comp->status.csiList,
                    CL_AMS_COMP_STATUS_CSI_LIST);

            break;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("Member of SI",
                    "%s",csi->config.parentSI.entity.name.value);
            CL_AMS_PRINT_TWO_COL("Type",
                    "%s",csi->config.type.value);
            CL_AMS_PRINT_TWO_COL("Rank",
                    "%d",csi->config.rank);
            CL_AMS_PRINT_TWO_COL("Proxy CSI",
                                 "%s", CL_AMS_STRING_BOOLEAN(csi->config.isProxyCSI));

            CL_AMS_PRINT_TWO_COL("CSI Dependents List",
                    "Count[%u]",
                    csi->config.csiDependentsList.numEntities);
            clAmsEntityListPrint(
                    &csi->config.csiDependentsList,
                    CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST);

            CL_AMS_PRINT_TWO_COL("CSI Dependencies List",
                    "Count[%u]",
                    csi->config.csiDependenciesList.numEntities);
            clAmsEntityListPrint(
                    &csi->config.csiDependenciesList,
                    CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST);

            clAmsCSINVPListPrint(csi->config.nameValuePairList);

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            CL_AMS_PRINT_TWO_COL("PG List",
                    "Count[%u]",
                    csi->status.pgList.numEntities);
            clAmsEntityListPrint(
                    &csi->status.pgList,
                    CL_AMS_CSI_STATUS_PG_LIST);
            clAmsCSIPGTrackListPrint(csi->status.pgTrackList);
            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App, cluster
             */

        }
    }

    entityStatus = clAmsEntityGetStatusStruct(entity);

    CL_AMS_PRINT_TWO_COL("---------------------------------------------",
            "%s","---------------------------");
    /*
    CL_AMS_PRINT_TWO_COL("Uptime",
            "%u",entityStatus->epoch);
    */
    CL_AMS_PRINT_TWO_COL("Timers Running",
            "%u",entityStatus->timerCount);
    CL_AMS_PRINT_TWO_COL("Debug Flags",
            "0x%x",entity->debugFlags);

    CL_AMS_PRINT_DELIMITER();

    return CL_OK;

}

void clAmsCompInfoPrint(const ClAmsCompT *comp)
{
    ClAmsSUT *su = (ClAmsSUT *)comp->config.parentSU.ptr;
    ClAmsNodeT *node = (ClAmsNodeT *)su->config.parentNode.ptr;
    ClIocAddressT iocAddress;
    ClRcT rc = CL_OK;
    ClCpmCompSpecInfoT compInfo;
    ClUint32T i = 0;

    extern ClCpmAmsToCpmCallT *gAmsToCpmCallbackFuncs;

    memset(&iocAddress, 0, sizeof(ClIocAddressT));
    memset(&compInfo, 0 , sizeof(ClCpmCompSpecInfoT));

    rc = (*gAmsToCpmCallbackFuncs->
          cpmIocAddressForNodeGet)(&node->config.entity.name,
                                   &iocAddress);
    if (rc != CL_OK) return;

    rc = clCpmCompInfoGet(&comp->config.entity.name,
                          iocAddress.iocPhyAddress.nodeAddress,
                          &compInfo);
    if (rc != CL_OK) return;
    
    CL_AMS_PRINT_OPEN_TAG("args");
    for (i = 0; i < compInfo.numArgs; ++i)
    {
        CL_AMS_PRINT_TAG_VALUE("arg", "%s", compInfo.args[i]);
    }
    
    CL_AMS_PRINT_CLOSE_TAG("args");

    CL_AMS_PRINT_OPEN_TAG("healthcheck");
    CL_AMS_PRINT_TAG_VALUE("period", "%u", compInfo.period);
    CL_AMS_PRINT_TAG_VALUE("max_duration", "%u", compInfo.maxDuration);
    CL_AMS_PRINT_TAG_VALUE("recovery", "%s",
                           CL_AMS_STRING_RECOVERY(compInfo.recovery));
    CL_AMS_PRINT_CLOSE_TAG("healthcheck");

    for (i = 0; i < compInfo.numArgs; ++i)
    {
        ClCharT **p = compInfo.args;
        clHeapFree(p[i]);
    }
    clHeapFree(compInfo.args);
}

/*
 * clAmsEntityXMLPrint
 * ----------------
 * Print out the contents of an Entity in XML format
 */

ClRcT
clAmsEntityXMLPrint(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsAdminStateT  adminState = 0;

    AMS_CHECKPTR ( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    
    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClRcT instantiable = (clAmsPeNodeIsInstantiable(node) == CL_OK);

            CL_AMS_PRINT_OPEN_TAG_ATTR("node", "%s", entity->name.value);

            if( clAmsPeNodeComputeAdminState(node, &adminState) != CL_OK)
            {
                adminState = node->config.adminState;
            }

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(node->config.
                                                         adminState));

            CL_AMS_PRINT_TAG_VALUE("computed_admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(adminState));

            CL_AMS_PRINT_TAG_VALUE("id", "%d", node->config.id);

            CL_AMS_PRINT_TAG_VALUE("class_type", "%s",
                                   CL_AMS_STRING_NODE_CLASSTYPE(node->config.
                                                                classType));

            CL_AMS_PRINT_TAG_VALUE("sub_class_type", "%s",
                                   node->config.subClassType.value);

            CL_AMS_PRINT_TAG_VALUE("is_swappable", "%s",
                                   CL_AMS_STRING_BOOLEAN(node->config.
                                                         isSwappable));

            CL_AMS_PRINT_TAG_VALUE("is_restartable", "%s",
                                   CL_AMS_STRING_BOOLEAN(node->config.
                                                         isRestartable));

            CL_AMS_PRINT_TAG_VALUE("is_asp_aware", "%s",
                                   CL_AMS_STRING_BOOLEAN(node->config.
                                                         isASPAware));

            CL_AMS_PRINT_TAG_VALUE("autorepair", "%s",
                                   CL_AMS_STRING_BOOLEAN(node->config.
                                                         autoRepair));

            CL_AMS_PRINT_TAG_VALUE("su_failover_duration", "%lld",
                                   node->config.suFailoverDuration);
            CL_AMS_PRINT_TAG_VALUE("su_failover_count_max", "%d",
                                   node->config.suFailoverCountMax);

            CL_AMS_PRINT_CLOSE_TAG("config");
            

            CL_AMS_PRINT_OPEN_TAG_ATTR("node_dependents", "%u",
                                       node->config.nodeDependentsList.
                                       numEntities);

            clAmsEntityListXMLPrint(
                    &node->config.nodeDependentsList,
                    CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST);

            CL_AMS_PRINT_CLOSE_TAG("node_dependents");

            CL_AMS_PRINT_OPEN_TAG_ATTR("node_dependencies", "%u",
                                       node->config.
                                       nodeDependenciesList.numEntities);

            clAmsEntityListXMLPrint(
                    &node->config.nodeDependenciesList,
                    CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST);

            CL_AMS_PRINT_CLOSE_TAG("node_dependencies");

            CL_AMS_PRINT_OPEN_TAG_ATTR("sus", "%u",
                                       node->config.suList.numEntities);

            clAmsEntityListXMLPrint(
                    &node->config.suList,
                    CL_AMS_NODE_CONFIG_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sus");

            CL_AMS_PRINT_OPEN_TAG("status");

            CL_AMS_PRINT_TAG_VALUE("presence_state", "%s",
                                   CL_AMS_STRING_P_STATE(node->status.
                                                         presenceState));
            CL_AMS_PRINT_TAG_VALUE("operational_state", "%s",
                                   CL_AMS_STRING_O_STATE(node->status.
                                                         operState));
            CL_AMS_PRINT_TAG_VALUE("is_instantiable", "%s",
                                   CL_AMS_STRING_BOOLEAN(instantiable));
            CL_AMS_PRINT_TAG_VALUE("is_cluster_member", "%s",
                                   CL_AMS_STRING_NODE_ISCLUSTERMEMBER(node->
                                                                      status.
                                                                      isClusterMember));
            CL_AMS_PRINT_TAG_VALUE("was_cluster_member_before", "%s",
                                   CL_AMS_STRING_BOOLEAN(node->status.
                                                         wasMemberBefore));
            CL_AMS_PRINT_TAG_VALUE("last_recovery", "%s",
                                   CL_AMS_STRING_RECOVERY(node->status.recovery));
            CL_AMS_PRINT_TAG_VALUE("num_instantiated_sus", "%u",
                                   node->status.numInstantiatedSUs);
            CL_AMS_PRINT_TAG_VALUE("num_assigned_sus", "%u",
                                   node->status.numAssignedSUs);
            CL_AMS_PRINT_TAG_VALUE("su_failover_timer", "%s",
                                   node->status.suFailoverTimer.count ?
                                   "Running" : "Inactive");
            CL_AMS_PRINT_TAG_VALUE("su_failover_count", "%d",
                                   node->status.suFailoverCount);

            CL_AMS_PRINT_CLOSE_TAG("status");

            CL_AMS_PRINT_CLOSE_TAG("node");
            
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;

            CL_AMS_PRINT_TWO_COL("Configuration -------------------------------",
                    "%s","---------------------------");

            CL_AMS_PRINT_TWO_COL("Status --------------------------------------",
                    "%s","---------------------------");
            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;

            CL_AMS_PRINT_OPEN_TAG_ATTR("sg", "%s", entity->name.value);

            if( clAmsPeSGComputeAdminState(sg, &adminState) != CL_OK)
            {
                adminState = sg->config.adminState;
            }

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(sg->config.
                                                         adminState));

            CL_AMS_PRINT_TAG_VALUE("computed_admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(adminState));

            CL_AMS_PRINT_TAG_VALUE("redundancy_model", "%s",
                                   CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.
                                                                     redundancyModel));

            CL_AMS_PRINT_TAG_VALUE("loading_strategy", "%s",
                                   CL_AMS_STRING_SG_LOADING_STRATEGY(sg->config.
                                                                     loadingStrategy));

            CL_AMS_PRINT_TAG_VALUE("failback_option", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->config.
                                                         failbackOption));
            
            CL_AMS_PRINT_TAG_VALUE("autorepair", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->config.
                                                         autoRepair));

            CL_AMS_PRINT_TAG_VALUE("instantiate_duration", "%lld",
                                   sg->config.instantiateDuration);

            CL_AMS_PRINT_TAG_VALUE("preferred_active_sus", "%u",
                                   sg->config.numPrefActiveSUs);

            CL_AMS_PRINT_TAG_VALUE("preferred_standby_sus", "%u",
                                   sg->config.numPrefStandbySUs);

            CL_AMS_PRINT_TAG_VALUE("preferred_inservice_sus", "%u",
                                   sg->config.numPrefInserviceSUs);

            CL_AMS_PRINT_TAG_VALUE("preferred_assigned_sus", "%u",
                                   sg->config.numPrefAssignedSUs);

            CL_AMS_PRINT_TAG_VALUE("pref_active_sus_per_si", "%u",
                                   sg->config.numPrefActiveSUsPerSI);

            CL_AMS_PRINT_TAG_VALUE("max_active_sis_per_su", "%u",
                                   sg->config.maxActiveSIsPerSU);

            CL_AMS_PRINT_TAG_VALUE("max_standby_sis_per_su", "%u",
                                   sg->config.maxStandbySIsPerSU);
            
            CL_AMS_PRINT_TAG_VALUE("comp_restart_duration", "%lld",
                                   sg->config.compRestartDuration);

            CL_AMS_PRINT_TAG_VALUE("comp_restart_count_max", "%u",
                                   sg->config.compRestartCountMax);
            
            CL_AMS_PRINT_TAG_VALUE("su_restart_duration", "%lld",
                                   sg->config.suRestartDuration);

            CL_AMS_PRINT_TAG_VALUE("su_restart_count_max", "%d",
                                   sg->config.suRestartCountMax);
            CL_AMS_PRINT_TAG_VALUE("collocation_allowed", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->config.
                                                         isCollocationAllowed));
            CL_AMS_PRINT_TAG_VALUE("alpha_factor", "%d",
                                   sg->config.alpha);

            CL_AMS_PRINT_TAG_VALUE("beta_factor", "%d",
                                   sg->config.beta);

            CL_AMS_PRINT_TAG_VALUE("reduction_procedure", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->config.reductionProcedure));

            CL_AMS_PRINT_TAG_VALUE("auto_adjust", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->config.autoAdjust));

            CL_AMS_PRINT_TAG_VALUE("auto_adjust_probation", "%lld",
                                   sg->config.autoAdjustProbation);

            CL_AMS_PRINT_OPEN_TAG_ATTR("sus", "%u",
                                       sg->config.suList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &sg->config.suList,
                    CL_AMS_SG_CONFIG_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sus");

            CL_AMS_PRINT_OPEN_TAG_ATTR("sis", "%u",
                                       sg->config.siList.numEntities);

            clAmsEntityListXMLPrint(
                    &sg->config.siList,
                    CL_AMS_SG_CONFIG_SI_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sis");

            CL_AMS_PRINT_CLOSE_TAG("config");

            CL_AMS_PRINT_OPEN_TAG("status");
            
            CL_AMS_PRINT_TAG_VALUE("is_started", "%s",
                                   CL_AMS_STRING_BOOLEAN(sg->status.
                                                         isStarted));
            CL_AMS_PRINT_TAG_VALUE("instantiate_timer", "%s",
                                   sg->status.instantiateTimer.count ?
                                   "Running" : "Inactive");
            
            CL_AMS_PRINT_OPEN_TAG_ATTR("instantiable_sus", "%u",
                                       sg->status.instantiableSUList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &sg->status.instantiableSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);
            
            CL_AMS_PRINT_CLOSE_TAG("instantiable_sus");

            CL_AMS_PRINT_OPEN_TAG_ATTR("instantiated_sus", "%u",
                                       sg->status.instantiatedSUList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &sg->status.instantiatedSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("instantiated_sus");

            CL_AMS_PRINT_OPEN_TAG_ATTR("inservice_spare_sus", "%u",
                                       sg->status.inserviceSpareSUList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &sg->status.inserviceSpareSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("inservice_spare_sus");
            
            CL_AMS_PRINT_OPEN_TAG_ATTR("assigned_sus", "%u",
                                       sg->status.assignedSUList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &sg->status.assignedSUList,
                    CL_AMS_SG_CONFIG_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("assigned_sus");

            CL_AMS_PRINT_TAG_VALUE("current_active_sus", "%u",
                                   sg->status.numCurrActiveSUs);
            CL_AMS_PRINT_TAG_VALUE("current_standby_sus", "%u",
                                   sg->status.numCurrStandbySUs);

            CL_AMS_PRINT_CLOSE_TAG("status");

            CL_AMS_PRINT_CLOSE_TAG("sg");

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;

            CL_AMS_PRINT_OPEN_TAG_ATTR("su", "%s", entity->name.value);

            if ( clAmsPeSUComputeAdminState(su, &adminState) != CL_OK)
            {
                adminState = su->config.adminState;
            }

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("member_of_sg", "%s",
                                   su->config.parentSG.entity.name.value);

            CL_AMS_PRINT_TAG_VALUE("member_of_node", "%s",
                                   su->config.parentNode.entity.name.value);

            CL_AMS_PRINT_TAG_VALUE("admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(su->config.
                                                         adminState));
            CL_AMS_PRINT_TAG_VALUE("computed_admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(adminState));
            CL_AMS_PRINT_TAG_VALUE("rank", "%d", su->config.rank);

            CL_AMS_PRINT_TAG_VALUE("num_of_components", "%u",
                                   su->config.numComponents);

            CL_AMS_PRINT_TAG_VALUE("is_preinstantiable", "%s",
                                   CL_AMS_STRING_BOOLEAN(su->config.isPreinstantiable));

            CL_AMS_PRINT_TAG_VALUE("is_restartable", "%s",
                                   CL_AMS_STRING_BOOLEAN(su->config.isRestartable));

            CL_AMS_PRINT_OPEN_TAG_ATTR("components", "%u",
                                       su->config.compList.numEntities);
            clAmsEntityListXMLPrint(
                    &su->config.compList,
                    CL_AMS_SU_CONFIG_COMP_LIST);

            CL_AMS_PRINT_CLOSE_TAG("components");

            CL_AMS_PRINT_CLOSE_TAG("config");
            
            CL_AMS_PRINT_OPEN_TAG("status");
            
            CL_AMS_PRINT_TAG_VALUE("presence_state", "%s",
                                   CL_AMS_STRING_P_STATE(su->status.
                                                         presenceState));
            CL_AMS_PRINT_TAG_VALUE("operational_state", "%s",
                                   CL_AMS_STRING_O_STATE(su->status.
                                                         operState));

            CL_AMS_PRINT_TAG_VALUE("readiness_state", "%s",
                                   CL_AMS_STRING_R_STATE(su->status.
                                                         readinessState));
            CL_AMS_PRINT_TAG_VALUE("last_recovery", "%s",
                                   CL_AMS_STRING_RECOVERY(su->status.
                                                          recovery));
            CL_AMS_PRINT_TAG_VALUE("num_active_sis", "%u",
                                   su->status.numActiveSIs);

            CL_AMS_PRINT_TAG_VALUE("num_standby_sis", "%u",
                                   su->status.numStandbySIs);

            CL_AMS_PRINT_TAG_VALUE("num_quiesced_sis", "%u",
                                   su->status.numQuiescedSIs);

            CL_AMS_PRINT_TAG_VALUE("num_instantiated_comps", "%u",
                                   su->status.numInstantiatedComp);

            CL_AMS_PRINT_TAG_VALUE("num_instantiated_comps", "%u",
                                   su->status.numPIComp);

            CL_AMS_PRINT_TAG_VALUE("comp_restart_timer", "%s",
                                   su->status.compRestartTimer.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("comp_restart_count", "%d",
                                   su->status.compRestartCount);
            CL_AMS_PRINT_TAG_VALUE("su_restart_timer", "%s",
                                   su->status.suRestartTimer.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("su_restart_count", "%d",
                                   su->status.suRestartCount);

            CL_AMS_PRINT_OPEN_TAG_ATTR("sis", "%u",
                                       su->status.siList.numEntities);
            clAmsEntityListXMLPrint(
                    &su->status.siList,
                    CL_AMS_SU_STATUS_SI_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sis");

            CL_AMS_PRINT_CLOSE_TAG("status");

            CL_AMS_PRINT_CLOSE_TAG("su");

            break;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT  *si = (ClAmsSIT *) entity;

            CL_AMS_PRINT_OPEN_TAG_ATTR("si", "%s", entity->name.value);

            if ( clAmsPeSIComputeAdminState(si, &adminState) != CL_OK)
            {
                adminState = si->config.adminState;
            }

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("member_of_sg", "%s",
                                   si->config.parentSG.entity.name.value);

            CL_AMS_PRINT_TAG_VALUE("admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(si->config.
                                                         adminState));

            CL_AMS_PRINT_TAG_VALUE("computed_admin_state", "%s",
                                   CL_AMS_STRING_A_STATE(adminState));

            CL_AMS_PRINT_TAG_VALUE("rank", "%d", si->config.rank);

            CL_AMS_PRINT_TAG_VALUE("num_of_csis", "%u", si->config.numCSIs);

            CL_AMS_PRINT_TAG_VALUE("num_of_standby_assignments", "%u",
                                   si->config.numStandbyAssignments);

            CL_AMS_PRINT_OPEN_TAG_ATTR("sus", "%u",
                                       si->config.suList.numEntities);
            clAmsEntityListXMLPrint(
                    &si->config.suList,
                    CL_AMS_SI_CONFIG_SU_RANK_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sus");
            
            CL_AMS_PRINT_OPEN_TAG_ATTR("si_dependents_list", "%u",
                                       si->config.siDependentsList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &si->config.siDependentsList,
                    CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST);

            CL_AMS_PRINT_CLOSE_TAG("si_dependents_list");

            CL_AMS_PRINT_OPEN_TAG_ATTR("si_dependencies_list", "%u",
                                       si->config.siDependenciesList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &si->config.siDependenciesList,
                    CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST);

            CL_AMS_PRINT_CLOSE_TAG("si_dependencies_list");

            CL_AMS_PRINT_OPEN_TAG_ATTR("csis", "%u", 
                                       si->config.csiList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &si->config.csiList,
                    CL_AMS_SI_CONFIG_CSI_LIST);

            CL_AMS_PRINT_CLOSE_TAG("csis");

            CL_AMS_PRINT_CLOSE_TAG("config");

            CL_AMS_PRINT_OPEN_TAG("status");

            CL_AMS_PRINT_TAG_VALUE("operational_state", "%s",
                                   CL_AMS_STRING_O_STATE(si->status.
                                                         operState));

            CL_AMS_PRINT_TAG_VALUE("num_active_assignments", "%u",
                                   si->status.numActiveAssignments);

            CL_AMS_PRINT_TAG_VALUE("num_standby_assignments", "%u",
                                   si->status.numStandbyAssignments);

            CL_AMS_PRINT_OPEN_TAG_ATTR("sus", "%u",
                                       si->status.suList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &si->status.suList,
                    CL_AMS_SI_STATUS_SU_LIST);

            CL_AMS_PRINT_CLOSE_TAG("sus");

            CL_AMS_PRINT_CLOSE_TAG("status");

            CL_AMS_PRINT_CLOSE_TAG("si");

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;

            ClUint32T i; 

            CL_AMS_PRINT_OPEN_TAG_ATTR("comp", "%s", entity->name.value);

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("member_of_su", "%s",
                                   comp->config.parentSU.entity.name.value);

            CL_AMS_PRINT_TAG_VALUE("proxy_csi_type", "%s",
                                   comp->config.proxyCSIType.value);

            CL_AMS_PRINT_TAG_VALUE("capability_model", "%s",
                                   CL_AMS_STRING_COMP_CAP(comp->config.
                                                          capabilityModel));
            CL_AMS_PRINT_TAG_VALUE("property", "%s",
                                   CL_AMS_STRING_COMP_PROPERTY(comp->config.
                                                               property));

            CL_AMS_PRINT_TAG_VALUE("is_restartable", "%s",
                                   CL_AMS_STRING_BOOLEAN(comp->config.
                                                         isRestartable));

            CL_AMS_PRINT_TAG_VALUE("node_reboot_on_cleanup_fail", "%s",
                                   CL_AMS_STRING_BOOLEAN(comp->
                                                         config.nodeRebootCleanupFail));

            CL_AMS_PRINT_TAG_VALUE("instantiate_level", "%u",
                                   comp->config.instantiateLevel);

            CL_AMS_PRINT_TAG_VALUE("max_instantiate_attempts", "%u",
                                   comp->config.numMaxInstantiate);

            CL_AMS_PRINT_TAG_VALUE("max_instantiate_attempts_with_delay", "%u",
                                   comp->config.numMaxInstantiateWithDelay);

            CL_AMS_PRINT_TAG_VALUE("max_active_csis", "%u",
                                   comp->config.numMaxActiveCSIs);

            CL_AMS_PRINT_TAG_VALUE("max_standby_csis", "%u",
                                   comp->config.numMaxStandbyCSIs);

            CL_AMS_PRINT_TAG_VALUE("instantiate_command", "%s",
                                   comp->config.instantiateCommand);

            if (debugPrintAll)
            {
                clAmsCompInfoPrint(comp);
            }
            
            CL_AMS_PRINT_OPEN_TAG("timeouts");
            
            CL_AMS_PRINT_TAG_VALUE("instantiate_timeout", "%lld",
                                   comp->config.timeouts.instantiate);

            CL_AMS_PRINT_TAG_VALUE("instantiate_delay_timeout", "%lld",
                                   comp->config.timeouts.instantiateDelay);

            CL_AMS_PRINT_TAG_VALUE("terminate_timeout","%lld",
                                   comp->config.timeouts.terminate);

            CL_AMS_PRINT_TAG_VALUE("cleanup_timeout", "%lld",
                                   comp->config.timeouts.cleanup);

            CL_AMS_PRINT_TAG_VALUE("quiescing_complete_timeout", "%lld",
                                   comp->config.timeouts.quiescingComplete);

            CL_AMS_PRINT_TAG_VALUE("csi_set_timeout", "%lld",
                                   comp->config.timeouts.csiSet);

            CL_AMS_PRINT_TAG_VALUE("csi_rmv_timeout", "%lld",
                                   comp->config.timeouts.csiRemove);

            CL_AMS_PRINT_TAG_VALUE("proxied_comp_instantiate_timeout", "%lld",
                                   comp->config.timeouts.proxiedCompInstantiate);

            CL_AMS_PRINT_TAG_VALUE("proxied_comp_cleanup_timeout", "%lld",
                                   comp->config.timeouts.proxiedCompCleanup);

            CL_AMS_PRINT_CLOSE_TAG("timeouts");
                        
            CL_AMS_PRINT_TAG_VALUE("recovery_on_error", "%s",
                                   CL_AMS_STRING_RECOVERY(comp->config.
                                                          recoveryOnTimeout));

            CL_AMS_PRINT_CLOSE_TAG("config");

            CL_AMS_PRINT_OPEN_TAG("status");
            
            CL_AMS_PRINT_TAG_VALUE("proxy_for_component", "%s",
                                   comp->status.proxyComp ?
                                   comp->status.proxyComp->name.value : "None");

            CL_AMS_PRINT_TAG_VALUE("presence_state", "%s",
                                   CL_AMS_STRING_P_STATE(comp->status.
                                                         presenceState));

            CL_AMS_PRINT_TAG_VALUE("operational_state", "%s",
                                   CL_AMS_STRING_O_STATE(comp->status.
                                                         operState));

            CL_AMS_PRINT_TAG_VALUE("readiness_state", "%s",
                                   CL_AMS_STRING_R_STATE(comp->status.
                                                         readinessState));

            CL_AMS_PRINT_TAG_VALUE("last_recovery", "%s",
                                   CL_AMS_STRING_RECOVERY(comp->status.
                                                          recovery));

            CL_AMS_PRINT_TAG_VALUE("num_active_csis", "%u",
                                   comp->status.numActiveCSIs);

            CL_AMS_PRINT_TAG_VALUE("num_standby_csis", "%u",
                                   comp->status.numStandbyCSIs);

            CL_AMS_PRINT_TAG_VALUE("num_quiescing_csis", "%u",
                                   comp->status.numQuiescingCSIs);

            CL_AMS_PRINT_TAG_VALUE("num_quiesced_csis", "%u",
                                   comp->status.numQuiescedCSIs);

            CL_AMS_PRINT_TAG_VALUE("component_restart_count", "%u",
                                   comp->status.restartCount);

            CL_AMS_PRINT_TAG_VALUE("instantiate_count", "%u",
                                   comp->status.instantiateCount);

            CL_AMS_PRINT_TAG_VALUE("instantiate_with_delay_count", "%u",
                                   comp->status.instantiateDelayCount);

            CL_AMS_PRINT_OPEN_TAG("timers");
            
            CL_AMS_PRINT_TAG_VALUE("instantiate_timer", "%s",
                                   comp->status.timers.instantiate.count ? 
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("instantiate_delay_timer", "%s",
                                   comp->status.timers.instantiateDelay.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("terminate_timer", "%s",
                                   comp->status.timers.terminate.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("cleanup_timer", "%s",
                                   comp->status.timers.cleanup.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("quiescing_complete_timer", "%s",
                                   comp->status.timers.quiescingComplete.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("csi_set_timer", "%s",
                                   comp->status.timers.csiSet.count ? 
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("csi_rmv_timer", "%s",
                                   comp->status.timers.csiRemove.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("proxied_comp_instantiate_timer", "%s",
                                   comp->status.timers.
                                   proxiedCompInstantiate.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_TAG_VALUE("proxied_comp_cleanup_timer", "%s",
                                   comp->status.timers.
                                   proxiedCompCleanup.count ?
                                   "Running" : "Inactive");

            CL_AMS_PRINT_CLOSE_TAG("timers");

            CL_AMS_PRINT_OPEN_TAG_ATTR("csis", "%u",
                                       comp->status.csiList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &comp->status.csiList,
                    CL_AMS_COMP_STATUS_CSI_LIST);

            CL_AMS_PRINT_CLOSE_TAG("csis");

            CL_AMS_PRINT_OPEN_TAG_ATTR("supported_csi_types", "%u", 
                                       comp->config.numSupportedCSITypes);

            for(i = 0; i < comp->config.numSupportedCSITypes; ++i)
            {
                CL_AMS_PRINT_TAG_VALUE("csi", "%s",
                                       comp->config.pSupportedCSITypes[i].value);
            }
            
            CL_AMS_PRINT_CLOSE_TAG("supported_csi_types");
            
            CL_AMS_PRINT_CLOSE_TAG("status");
            
            CL_AMS_PRINT_CLOSE_TAG("comp");

            break;
        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;

            CL_AMS_PRINT_OPEN_TAG_ATTR("csi", "%s", entity->name.value);

            CL_AMS_PRINT_OPEN_TAG("config");

            CL_AMS_PRINT_TAG_VALUE("member_of_si", "%s",
                                   csi->config.parentSI.entity.name.value);

            CL_AMS_PRINT_TAG_VALUE("type", "%s",
                                   csi->config.type.value);

            CL_AMS_PRINT_TAG_VALUE("rank", "%d",
                                   csi->config.rank);
 
            clAmsCSINVPListXMLPrint(csi->config.nameValuePairList);

            CL_AMS_PRINT_OPEN_TAG_ATTR("csi_dependents_list", "%u",
                                       csi->config.csiDependentsList.numEntities);
            
            clAmsEntityListXMLPrint(
                    &csi->config.csiDependentsList,
                    CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST);

            CL_AMS_PRINT_CLOSE_TAG("csi_dependents_list");

            CL_AMS_PRINT_OPEN_TAG_ATTR("csi_dependencies_list", "%u",
                                       csi->config.csiDependenciesList.
                                       numEntities);
            clAmsEntityListXMLPrint(
                    &csi->config.csiDependenciesList,
                    CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST);

            CL_AMS_PRINT_CLOSE_TAG("csi_dependencies_list");

            CL_AMS_PRINT_CLOSE_TAG("config");
            
            CL_AMS_PRINT_OPEN_TAG("status");

            CL_AMS_PRINT_OPEN_TAG_ATTR("pgs", "%u",
                                       csi->status.pgList.numEntities);

            clAmsEntityListXMLPrint(
                    &csi->status.pgList,
                    CL_AMS_CSI_STATUS_PG_LIST);
            
            CL_AMS_PRINT_CLOSE_TAG("pgs");

            clAmsCSIPGTrackListXMLPrint(csi->status.pgTrackList);

            CL_AMS_PRINT_CLOSE_TAG("status");

            CL_AMS_PRINT_CLOSE_TAG("csi");

            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App, cluster
             */

        }
    }

 #if 0
    ClAmsEntityStatusT  *entityStatus = NULL;
    entityStatus = clAmsEntityGetStatusStruct(entity);
    CL_AMS_PRINT_TAG_VALUE("timers_running", "%u", entityStatus->timerCount);
    CL_AMS_PRINT_TAG_VALUE("debug_flags", "%#x", entity->debugFlags);
#endif    

    return CL_OK;

}

/*
 * clAmsEntityGetKey
 * -----------------
 * Generate an unique key for each entity based on its name. This is the
 * default key if no other key is provided.
 */

ClRcT
clAmsEntityGetKey(
        CL_IN  ClAmsEntityT  *entity,
        CL_INOUT  ClCntKeyHandleT  *entityKeyHandle )
{

    ClUint32T  entityKey = 0;
    ClUint32T  keyLength = 0;

    AMS_CHECKPTR ( !entity || !entityKeyHandle );

    AMS_CALL ( crc(
                 ( ClUint8T *)entity->name.value, 
                 entity->name.length,
                 &entityKey,
                 &keyLength) );
    /*
     * Endianesss considerations. entityKey is 16bit but handle is either 32 or
     * 64 depending on flavor of the month. Want to make sure the key value is
     * not mutilated.
     */

    *entityKeyHandle = (ClCntKeyHandleT) (ClWordT)entityKey;

    return CL_OK;

}

static ClRcT amsCompClearOps(ClAmsCompT *comp)
{
    /*
     * Clear the invocation list of entries for this component. We are
     * not going to get a response for these invocations.
     */
    AMS_CALL ( clAmsInvocationDeleteAll(comp) );

    /*
     * Stop all possible timers that could be running for the component.
     */
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_INSTANTIATE) );
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_INSTANTIATEDELAY) );
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_TERMINATE) );
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_CLEANUP));
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP));
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_CSISET));
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_CSIREMOVE));
    AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp,
                                   CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE));

    return CL_OK;
}

/*
 * If this is a proxy, then clearops for all the proxied including healthchecks
 */
static ClRcT
clAmsProxiedClearOps(ClAmsCompT *comp)
{
    ClAmsEntityRefT *eRef = NULL;
    ClAmsCSIT *proxyCSI = NULL;
    ClBoolT proxy = CL_FALSE;
    ClAmsEntityDbT *compDb = NULL;
    ClCntNodeHandleT nodeHandle = 0;
    ClCntNodeHandleT nextNodeHandle = 0;
    for(eRef = clAmsEntityListGetFirst(&comp->status.csiList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&comp->status.csiList, eRef))
    {
        ClAmsCSIT *csi = (ClAmsCSIT*)eRef->ptr;
        if(!csi) continue;
        if(csi->config.isProxyCSI) 
        {
            proxyCSI = csi;
            proxy = CL_TRUE;
            break;
        }
    }
    if(!proxy) return CL_OK;
    /*
     * walk through the comp entity db and locate proxied for this csi type.
     */
    compDb = &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP];
    AMS_CALL(clCntFirstNodeGet(compDb->db, &nodeHandle));
    while(nodeHandle)
    {
        ClAmsCompT *compRef = NULL;
        nextNodeHandle = 0;
        clCntNextNodeGet(compDb->db, nodeHandle, &nextNodeHandle);
        AMS_CALL ( clCntNodeUserDataGet(compDb->db, nodeHandle, (ClCntDataHandleT*)&compRef) );
        AMS_CHECK_COMP (compRef);
        if(!memcmp(compRef->config.proxyCSIType.value, proxyCSI->config.type.value,
                   proxyCSI->config.type.length) )
        {
            cpmProxiedHealthcheckStop(&compRef->config.entity.name);
        }
        amsCompClearOps(compRef);
        nodeHandle = nextNodeHandle;
    }
    return CL_OK;
}

/*
 * clAmsEntityClearOps
 * -------------------
 * Clear all timers and invocations for an entity.
 */

ClRcT
clAmsEntityClearOps(
        CL_IN  ClAmsEntityT  *entity)
{
    ClAmsEntityRefT *eRef;

    AMS_CHECKPTR ( !entity );

    AMS_CHECK_ENTITY_TYPE (entity->type);

    switch ( entity->type )
    {
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *) entity;

            clAmsEntityTimerStop( (ClAmsEntityT*)node,
                                  CL_AMS_NODE_TIMER_SUFAILOVER);

            for ( eRef = clAmsEntityListGetFirst(&node->config.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&node->config.suList, eRef) )
            {
                AMS_CALL ( clAmsEntityClearOps(eRef->ptr) );
            }

            break;
        }

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            
            clAmsEntityTimerStop ( (ClAmsEntityT*)sg,
                                   CL_AMS_SG_TIMER_INSTANTIATE);

            clAmsEntityTimerStop ( (ClAmsEntityT*)sg,
                                   CL_AMS_SG_TIMER_ADJUST);

            clAmsEntityTimerStop ( (ClAmsEntityT*)sg,
                                   CL_AMS_SG_TIMER_ADJUST_PROBATION);

            for ( eRef = clAmsEntityListGetFirst(&sg->config.suList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->config.suList, eRef) )
            {
                AMS_CALL ( clAmsEntityClearOps(eRef->ptr) );
            }

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT  *su = (ClAmsSUT *) entity;

            clAmsEntityTimerStop( (ClAmsEntityT*)su,
                                  CL_AMS_SU_TIMER_COMPRESTART);

            clAmsEntityTimerStop( (ClAmsEntityT*)su,
                                  CL_AMS_SU_TIMER_SURESTART);

            clAmsEntityTimerStop ( (ClAmsEntityT*)su,
                                   CL_AMS_SU_TIMER_PROBATION);
            
            clAmsEntityTimerStop ( (ClAmsEntityT*)su,
                                   CL_AMS_SU_TIMER_ASSIGNMENT);

            su->status.numWaitAdjustments = 0;
            su->status.numDelayAssignments = 0;

            for ( eRef = clAmsEntityListGetFirst(&su->config.compList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&su->config.compList, eRef) )
            {
                AMS_CALL ( clAmsEntityClearOps(eRef->ptr) );
            }

            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp = (ClAmsCompT *) entity;

            AMS_CALL( amsCompClearOps(comp) );
            clAmsProxiedClearOps(comp);
            break;
        }

        default:
        {
            /*
             * This includes entity. cluster, App, SI and CSI.
             */
        }

    }

    return CL_OK;

}

/******************************************************************************
 * Functions to validate entity configuration and relationships
 *****************************************************************************/

/*
 * clAmsEntityValidate
 * -------------------
 * Validates an AMS entity. This function can be used by the management
 * API to verify an entity is properly configured as well as by the policy
 * engine, when it needs to do a comprehensive check on an entity. At
 * other times, it will be costly to call this function.
 *
 * Summary validation checks that the entity is properly configured.
 * Detailed validation also checks that the entities that are referred 
 * to by this entity exist. 
 */

ClRcT
clAmsEntityValidate(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClUint32T  mode )
{

    ClRcT rc1 = CL_OK, rc2 = CL_OK;

    AMS_CHECKPTR ( !entity );

    if ( (mode == CL_AMS_ENTITY_VALIDATE_CONFIG) ||
         (mode == CL_AMS_ENTITY_VALIDATE_ALL) )
    {
        rc1 = clAmsEntityValidateConfig(entity);
        if(rc1 != CL_OK)
        {
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                       "Entity validation failed for entity [%s] with [%#02x]",entity->name.value,rc1);
        }
    }

    if ( (mode == CL_AMS_ENTITY_VALIDATE_RELATIONSHIPS) ||
         (mode == CL_AMS_ENTITY_VALIDATE_ALL) )
    {
        rc2 = clAmsEntityValidateRelationships(entity);
        if(rc2 != CL_OK)
        {
            clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                       "Entity validate relationships failed for entity [%s] with [%#02x]",entity->name.value,rc2);
        }

    }

    if ( rc1 || rc2 )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    return CL_OK;

}

ClRcT
clAmsEntityValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsEntityParamsT  *params = NULL;
    ClAmsEntityMethodsT  *methods = NULL;

    AMS_CHECKPTR ( !entity );

    if ( clAmsEntityGetDefaults(entity->type, &params) != CL_OK )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    methods = (ClAmsEntityMethodsT *) params->defaultMethods;

    return methods->validateConfig(entity);

}

ClRcT
clAmsEntityValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsEntityParamsT  *params = NULL;
    ClAmsEntityMethodsT  *methods = NULL;

    AMS_CHECKPTR ( !entity );

    if ( clAmsEntityGetDefaults(entity->type, &params) != CL_OK )
    {
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    methods = (ClAmsEntityMethodsT *) params->defaultMethods;

    return methods->validateRelationships(entity);

}

ClRcT
clAmsValidateReference(
        CL_IN  ClAmsEntityRefT  entityRef,
        CL_IN  ClAmsEntityTypeT  entityType )
{

    AMS_CHECKPTR ( !entityRef.ptr );

    if ( entityRef.ptr->type != entityType )
    {
        return CL_AMS_ERR_INVALID_ENTITY;
    }

    return CL_OK;

}

/*-----------------------------------------------------------------------------
 * Node Validations
 *---------------------------------------------------------------------------*/
 
ClRcT
clAmsNodeValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsNodeT  *node = (ClAmsNodeT  *)entity;

    AMS_CHECK_NODE (node);

    AMS_VALIDATE_ADMINSTATE (node);
    AMS_VALIDATE_NODE_CLASS_TYPE (node);

    AMS_VALIDATE_BOOL_VALUE (node->config.isSwappable);
    AMS_VALIDATE_BOOL_VALUE (node->config.isRestartable);
    AMS_VALIDATE_BOOL_VALUE (node->config.autoRepair);
    AMS_VALIDATE_BOOL_VALUE (node->config.isASPAware);

    return CL_OK;

}

ClRcT
clAmsNodeValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsNodeT  *node = (ClAmsNodeT  *)entity;

    AMS_CHECK_NODE (node);

    /*
     * Validate that there are no circular dependencies in the
     * nodeDependents and nodeDependencies list. Also verify that 
     * the node itself is not there in the dependents or dependencies
     * list
     */

    AMS_CALL( clAmsEntityCompareLists(
                node->config.entity,
                &node->config.nodeDependentsList,
                &node->config.nodeDependenciesList) );
    
    return CL_OK;

}

/*-----------------------------------------------------------------------------
 * SG Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsSGValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsSGT  *sg = (ClAmsSGT *) entity;

    AMS_CHECK_SG (sg);

    AMS_VALIDATE_ADMINSTATE (sg);
    AMS_VALIDATE_BOOL_VALUE (sg->config.failbackOption);
#if 0    
    AMS_VALIDATE_RESTART_COUNT (sg->config.compRestartCountMax);
    AMS_VALIDATE_RESTART_COUNT (sg->config.suRestartCountMax);
    
    AMS_VALIDATE_RESTART_DURATION (sg->config.compRestartDuration);
    AMS_VALIDATE_RESTART_DURATION (sg->config.suRestartDuration);
#endif
    AMS_CALL (clAmsSGValidateRedundancyModel (sg));
    AMS_CALL (clAmsSGValidateLoadingStrategy (sg));

    return CL_OK;

}

ClRcT
clAmsSGValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsSGT  *sg = (ClAmsSGT *) entity;

    AMS_CHECK_SG (sg);

    return CL_OK;

}

ClRcT
clAmsSGValidateLoadingStrategy(
        CL_IN  ClAmsSGT  *sg )
{

    AMS_CHECK_SG (sg);

    if ( (sg->config.loadingStrategy < 1) ||
         (sg->config.loadingStrategy > 5) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: SG [%s] fails loading strategy validation.\n",
                 sg->config.entity.name.value));

        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    return CL_OK;

}

ClRcT
clAmsSGValidateRedundancyModel(
        CL_IN  ClAmsSGT  *sg )
{

    AMS_CHECK_SG (sg);

    if ( (sg->config.redundancyModel <= CL_AMS_SG_REDUNDANCY_MODEL_NONE) ||
         (sg->config.redundancyModel >= CL_AMS_SG_REDUNDANCY_MODEL_MAX) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: SG [%s] fails redundancy model validation.\n",
                 sg->config.entity.name.value));

        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    }

    switch ( sg->config.redundancyModel )
    {

        case CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY:
        {
            if ( (sg->config.numPrefActiveSUs      != 1) ||
                 (sg->config.numPrefStandbySUs     != 0) ||
                 (sg->config.numPrefInserviceSUs   != 1) ||
                 (sg->config.numPrefActiveSUsPerSI != 1) )
            { 
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Valid Values for SG [%s] Redundancy Model [%s] are numPrefActiveSUs[1] (currently %d),"
                         " numPrefStandbySUs[0] (currently %d), numPrefInserviceSUs[1] (currently %d), numPrefActiveSUsPerSI[1] (currently %d)\n", 
                         sg->config.entity.name.value, 
                         CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.redundancyModel),
                         sg->config.numPrefActiveSUs,
                         sg->config.numPrefStandbySUs,
                         sg->config.numPrefInserviceSUs,
                         sg->config.numPrefActiveSUsPerSI));
                goto AMS_VALIDATE_SG_RULES_FAILS;
            }
            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_TWO_N:
        {
            if ( (sg->config.numPrefActiveSUs      != 1) ||
                 (sg->config.numPrefStandbySUs     != 1) ||
                 (sg->config.numPrefInserviceSUs   <  2) ||
                 (sg->config.numPrefAssignedSUs    != 2) ||
                 (sg->config.numPrefActiveSUsPerSI != 1) )
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Valid Values for SG [%s] Redundancy Model [%s] are numPrefActiveSUs[1],"
                         " numPrefStandbySUs[1], numPrefInserviceSUs[>=2], numPrefActiveSUsPerSI[1],"
                         " numPrefAssignedSUs[2]\n",sg->config.entity.name.value,
                         CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.redundancyModel) ));
                goto AMS_VALIDATE_SG_RULES_FAILS;
            }
            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
        {
            if ( (sg->config.numPrefActiveSUs      == 0) ||
                 (sg->config.numPrefStandbySUs     == 0) ||
                 (sg->config.numPrefInserviceSUs   <
                    (sg->config.numPrefActiveSUs   + sg->config.numPrefStandbySUs)) ||
                 (sg->config.numPrefActiveSUsPerSI != 1) )
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Valid Values for SG [%s] Redundancy Model [%s] are numPrefActiveSUs[>0],"
                         " numPrefStandbySUs[>0], numPrefActiveSUsPerSI[1],"
                         " numPrefInserviceSUs [ >= (numPrefActiveSUs + numPrefStandbySUs)],\n",
                         sg->config.entity.name.value,
                         CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.redundancyModel) ));
                goto AMS_VALIDATE_SG_RULES_FAILS;
            }
            break;
        }

    case CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM:
        break; /*user specified - no validation*/

#ifdef POST_RC2

        case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY:
        {
            if ( (sg->config.numPrefInserviceSUs  < sg->config.numPrefAssignedSUs) ||
                 (sg->config.numPrefActiveSUsPerSI != 1) )
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Valid Values for SG [%s] Redundancy Model [%s] are "
                         "numPrefActiveSUs[ >= numPrefAssignedSUs], numPrefActiveSUsPerSI[1]\n",
                         sg->config.entity.name.value,
                         CL_AMS_STRING_SG_REDUNDANCY_MODEL (sg->config.redundancyModel) ));
                goto AMS_VALIDATE_SG_RULES_FAILS;
            }
            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE:
        {
            if ( (sg->config.numPrefInserviceSUs  < sg->config.numPrefAssignedSUs) ) 
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Valid Values for SG [%s] Redundancy Model [%s] are "
                         "numPrefInserviceSUs [< numPrefAssignedSUs] \n",
                         sg->config.entity.name.value,
                         CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->config.redundancyModel) ));
                goto AMS_VALIDATE_SG_RULES_FAILS;
            }
            break;
        }

#endif

        default:
        {
            goto AMS_VALIDATE_SG_RULES_FAILS;
        }

    }

    return CL_OK;

AMS_VALIDATE_SG_RULES_FAILS:
   
    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: SG [%s] fails redundancy model validation.\n",
             sg->config.entity.name.value)); 

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

/*-----------------------------------------------------------------------------
 * Application Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsAppValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{
    return CL_OK;
}

ClRcT
clAmsAppValidateRelationships(
        CL_IN  ClAmsEntityT  *entity)
{
    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * SU Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsSUValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsSUT  *su = (ClAmsSUT *)entity;
    ClAmsSGT  *sg = NULL;

    AMS_CHECK_SU (su);
    AMS_CHECK_SG (sg = (ClAmsSGT *)su->config.parentSG.ptr);

    AMS_VALIDATE_ADMINSTATE (su);

    AMS_VALIDATE_BOOL_VALUE (su->config.isPreinstantiable);
    AMS_VALIDATE_BOOL_VALUE (su->config.isRestartable);
    AMS_VALIDATE_BOOL_VALUE (su->config.isContainerSU);

    if ( (!su->config.isPreinstantiable)  && 
            (sg->config.maxActiveSIsPerSU != 1) &&
            (sg->config.maxStandbySIsPerSU != 0) )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("Valid Values for SG [%s] associated with Non-Preinstantiable SU [%s] are "
                 "maxActiveSIsPerSU [1], maxStandbySIsPerSU [0] \n",
                 sg->config.entity.name.value,su->config.entity.name.value ));
        goto AMS_VALIDATE_SU_RULES_FAILS;
    }

    if ( su->config.numComponents < 1 )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("Valid value of number of components for SU [%s] is [ >1 ]\n",
                 su->config.entity.name.value ));
        goto AMS_VALIDATE_SU_RULES_FAILS;
    }

    return CL_OK;

AMS_VALIDATE_SU_RULES_FAILS:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: SU [%s] fails config validation.\n",
             su->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

ClRcT
clAmsSUValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    ClAmsSUT    *su = (ClAmsSUT *)entity;

    AMS_CHECK_SU (su);

    /*
     * Validate parentSG and parentNode references
     */

    AMS_CHECK_RC_ERROR( clAmsValidateReference(
                su->config.parentSG,
                CL_AMS_ENTITY_TYPE_SG) );

    AMS_CHECK_RC_ERROR( clAmsValidateReference(
                su->config.parentNode,
                CL_AMS_ENTITY_TYPE_NODE) );

    return CL_OK;

exitfn:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: SU [%s] fails relationship validation.\n",
             su->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

/*-----------------------------------------------------------------------------
 * SI Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsSIValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsSIT  *si = (ClAmsSIT *)entity;

    AMS_CHECK_SI (si);

    AMS_VALIDATE_ADMINSTATE (si);

    if ( si->config.numCSIs < 1 )
    {
        AMS_LOG ( CL_DEBUG_ERROR, ("Invalid value of number of CSIs for SI [%s] \n",
                    si->config.entity.name.value ));
        goto AMS_VALIDATE_SI_RULES_FAILS;
    }

    return CL_OK;


AMS_VALIDATE_SI_RULES_FAILS:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: SI [%s] fails config validation.\n",
             si->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

ClRcT
clAmsSIValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    ClAmsSIT    *si = (ClAmsSIT *)entity;

    AMS_CHECK_SI (si);

    /*
     * Validate parentSG reference
     */

    AMS_CHECK_RC_ERROR( clAmsValidateReference(
                si->config.parentSG,
                CL_AMS_ENTITY_TYPE_SG) );

    /*
     * Validate that there are no circular dependencies in the
     * siDependents and siDependencies list. Also verify that 
     * the SI itself is not there in the dependents or dependencies
     * list
     */

    AMS_CHECK_RC_ERROR ( clAmsEntityCompareLists(
                si->config.entity,
                &si->config.siDependentsList,
                &si->config.siDependenciesList) );
    
    /*
     * From the csi list, validate that there is atleast
     * one component in the SU list that has the CSI type in 
     * its supportedCSIType
     */

    ClAmsSGT  *sg = (ClAmsSGT *)si->config.parentSG.ptr;

    AMS_CHECK_RC_ERROR( clAmsValidateCSIType(
                si->config.entity,
                &sg->config.suList,
                &si->config.csiList) );

    return CL_OK;

exitfn:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: SI [%s] fails relationship validation.\n",
             si->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

/*-----------------------------------------------------------------------------
 * Component Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsCompValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsCompT  *comp = (ClAmsCompT *)entity;

    AMS_CHECK_COMP (comp);

    AMS_VALIDATE_COMP_CAPABILITY_MODEL(comp);
    AMS_VALIDATE_COMP_PROPERTY(comp);
    AMS_VALIDATE_COMP_RECOVERY_ON_ERROR(comp);

    AMS_VALIDATE_BOOL_VALUE(comp->config.isRestartable);
    AMS_VALIDATE_BOOL_VALUE(comp->config.nodeRebootCleanupFail);

    if (    comp->config.numMaxInstantiate <= 0 ||
            comp->config.numMaxInstantiateWithDelay <= 0 ||
            comp->config.numMaxTerminate <=0 ||
            comp->config.numMaxAmStart <=0 ||
            comp->config.numMaxAmStop <=0 ||
            comp->config.numMaxActiveCSIs <=0 )
    {
        AMS_LOG ( CL_DEBUG_ERROR,("Component [%s] should have positive values for " 
                "attributes : numMaxInstantiate, numMaxInstantiateWithDelay, " 
                "numMaxTerminate, numMaxAmStart, numMaxAmStop, numMaxActiveCSIs\n", 
                comp->config.entity.name.value) );

        goto AMS_VALIDATE_COMP_RULES_FAILS;
    }

    switch ( comp->config.capabilityModel )
    {

        case CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY:
        {
            if ( comp->config.numMaxActiveCSIs != 1 ) 
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxActiveCSI is 1\n",comp->config.entity.name.value,
                           CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }

            if ( comp->config.numMaxStandbyCSIs != 1 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs is 1\n",comp->config.entity.name.value,
                           CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }

            break;
        }

        case CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY:
        {
            if ( comp->config.numMaxStandbyCSIs == 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs should be non zero positive value\n",
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

        case CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY:
        {
            if ( comp->config.numMaxActiveCSIs != 1 ) 
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxActiveCSIs is 1\n", comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }

            if ( comp->config.numMaxStandbyCSIs == 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs should be non zero positive value \n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

        case CL_AMS_COMP_CAP_X_ACTIVE:
        {

            if ( comp->config.numMaxStandbyCSIs != 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs is zero\n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

        case CL_AMS_COMP_CAP_ONE_ACTIVE:
        {
            if ( comp->config.numMaxActiveCSIs != 1 ) 
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxActiveCSIs is 1\n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }

            if ( comp->config.numMaxStandbyCSIs != 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs is zero \n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

        case CL_AMS_COMP_CAP_NON_PREINSTANTIABLE:
        {
            if ( comp->config.numMaxActiveCSIs != 1 ) 
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxActiveCSIs is 1\n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }

            if ( comp->config.numMaxStandbyCSIs != 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs is zero\n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

#ifdef POST_RC2

        case CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY:
        {

            if ( comp->config.numMaxStandbyCSIs == 0 )
            {
                AMS_LOG ( CL_DEBUG_ERROR, ("Component [%s] Capability Model [%s] : Valid Value of "
                            "numMaxStandbyCSIs should be non zero positive value\n", 
                            comp->config.entity.name.value, 
                            CL_AMS_STRING_COMP_CAP(comp->config.capabilityModel) ));
                goto AMS_VALIDATE_COMP_RULES_FAILS;
            }
            
            break;
        }

#endif

        default:
        {
            goto AMS_VALIDATE_COMP_RULES_FAILS;
        }

    }

    return CL_OK;

AMS_VALIDATE_COMP_RULES_FAILS:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: Component [%s] fails config validation.\n",
             comp->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

ClRcT
clAmsCompValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    ClAmsCompT  *comp = (ClAmsCompT *)entity;

    AMS_CHECK_COMP (comp);

    comp = (ClAmsCompT *)entity;

    /*
     * Validate parentSU reference
     */

    AMS_CHECK_RC_ERROR( clAmsValidateReference(
                comp->config.parentSU,
                CL_AMS_ENTITY_TYPE_SU) );

    return CL_OK;

exitfn:

    AMS_LOG(CL_DEBUG_ERROR,
            ("ERROR: Component [%s] fails relationship validation.\n",
             comp->config.entity.name.value));

    return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

}

/*-----------------------------------------------------------------------------
 * CSI Validations
 *---------------------------------------------------------------------------*/

ClRcT
clAmsCSIValidateConfig(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsCSIT  *csi = (ClAmsCSIT *)entity;

    AMS_CHECK_CSI (csi);

    return CL_OK;

}

ClRcT
clAmsCSIValidateRelationships(
        CL_IN  ClAmsEntityT  *entity )
{

    ClAmsCSIT  *csi = (ClAmsCSIT *)entity;
    ClRcT rc = CL_OK;

    AMS_CHECK_CSI (csi);

    /*
     * Validate the dependent/dependency lists for circular dependencies.
     */
    AMS_CHECK_RC_ERROR ( clAmsEntityCompareLists(
                csi->config.entity,
                &csi->config.csiDependentsList,
                &csi->config.csiDependenciesList) );

    exitfn:
    return rc;

}

/*
 * clAmsValidateCSIType
 * -------------------
 * This function traverses through the list of CSI's for a given SI.
 * For each CSI in the CSI list it tries to find out atleast one component
 * in the SU list for the SI, which has a supported CSI type same as the
 * type for the CSI in the CSI list. If it does not find a match it returns a error.
 *
 * @param
 *  siEntity                    - si Entity which has the csi list 
 * suList                       - su list for the SI entity
 * csiList                      - csi list for the SI entity
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsValidateCSIType(
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityT  siEntity,
        CL_IN  ClAmsEntityListT  *suList,
        CL_IN  ClAmsEntityListT  *csiList )
{

    ClAmsEntityRefT  *csiEntityRef = NULL;
    ClAmsEntityRefT  *suEntityRef = NULL;
    ClAmsEntityRefT  *compEntityRef = NULL;
    ClBoolT  match = CL_FALSE;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !suList || !csiList );

    /*
     * For each CSI in the csi list, find a component in the component list
     * for each SU in the su list, which has a supported CSI type matching the
     * type for the CSI.
     */

    if ( !csiList->numEntities  || !suList->numEntities)
    {
        return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
    }

    for ( csiEntityRef = clAmsEntityListGetFirst(csiList);
            csiEntityRef != (ClAmsEntityRefT *) NULL;
            csiEntityRef = clAmsEntityListGetNext(csiList, csiEntityRef))
    {

        /*
         * Find this csi in the SU's component list
         */ 
         
         match = CL_FALSE;

         AMS_CHECKPTR ( !csiEntityRef->ptr ); 

         ClAmsCSIT  *csi = (ClAmsCSIT *)csiEntityRef->ptr;

         for ( suEntityRef = clAmsEntityListGetFirst(suList);
                 suEntityRef != (ClAmsEntityRefT *) NULL && match != CL_TRUE;
                 suEntityRef = clAmsEntityListGetNext(suList, suEntityRef) )
         { 

             AMS_CHECKPTR ( !suEntityRef->ptr ); 

             ClAmsEntityListT   *compList = &(((ClAmsSUT *)suEntityRef->ptr)->config.compList);

             AMS_CHECKPTR(!compList);

             for ( compEntityRef = clAmsEntityListGetFirst(compList);
                     compEntityRef != (ClAmsEntityRefT *) NULL;
                     compEntityRef = clAmsEntityListGetNext(compList, compEntityRef))
             { 
                 ClUint32T i;

                 AMS_CHECKPTR ( !compEntityRef->ptr ); 

                 ClAmsCompT *comp = (ClAmsCompT *)compEntityRef->ptr;

                 for(i = 0; i < comp->config.numSupportedCSITypes; ++i)
                 {
                     if ( !memcmp(comp->config.pSupportedCSITypes[i].value,
                                  csi->config.type.value, 
                                  csi->config.type.length) )
                     {

                         /*
                          * Found a matching component with same CSI type 
                          */
                         match = CL_TRUE;
                         goto next;
                     }
                 }
             }
         }

         if ( match == CL_FALSE )
         {

             AMS_LOG(CL_DEBUG_ERROR,
                     ("CSI [%s] with CSIType [%s] belonging to SI [%s] can not be assigned to any "
                      "Component\n", csi->config.entity.name.value, csi->config.type.value,
                      siEntity.name.value ));
             return (CL_AMS_ERR_INVALID_ENTITY);

         }
        next: continue;
    }

    return CL_OK;

}

/******************************************************************************
 * Default functions to manage component lifecycle commands
 *****************************************************************************/

/******************************************************************************
 * Functions related to entity timer management
 *****************************************************************************/

ClAmsT  *ams = &gAms;

/*
 * clAmsEntityTimer*
 * -----------------
 * Functions to handle all the timers required by a component.
 * timerCreate, timerSet, timerCancel, timerDelete
 *
 */

ClRcT
clAmsEntityTimerGetValues(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType,
        CL_OUT  ClTimeT  *duration,
        CL_OUT  ClAmsEntityTimerT  **entityTimer,
        CL_OUT  ClTimerCallBackT  *fn )
{

    ClAmsSUT  *su = NULL;
    ClAmsSGT  *sg = NULL;
    ClAmsCompT  *comp = NULL;
    ClAmsNodeT  *node = NULL;
    ClAmsEntityTimerCallbackT  fnx = {0};
    ClAmsSGT defaultSG = {{{0}}};
    ClRcT rc = CL_OK;

    AMS_CHECKPTR ( !entity || !duration || !entityTimer || !fn);

    AMS_CHECK_ENTITY_TYPE (entity->type);

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            node = (ClAmsNodeT *) entity;
            break;
        }

        case CL_AMS_ENTITY_TYPE_SG:
        {
            sg = (ClAmsSGT *) entity;
            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            su = (ClAmsSUT *) entity;
            sg = (ClAmsSGT *) su->config.parentSG.ptr;

            if(!sg) 
            {
                /*
                 * SG is deleted. Flag it as an error but continue.
                 * with default.
                 */
                rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
                sg = &defaultSG;
                defaultSG.config.compRestartDuration = (ClTimeT)~0;
                defaultSG.config.suRestartDuration = (ClTimeT)~0;
                defaultSG.config.autoAdjustProbation = CL_AMS_SG_ADJUST_DURATION;
            }
            break;
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            comp = (ClAmsCompT *) entity;
            break;
        }

        default:
        {
            /*
             * Nothing to be done for entity type entity, App, cluster
             */
            return CL_OK;
        }

    }

    switch ( timerType )
    {

        /*
         * Component Timers
         */

        case CL_AMS_COMP_TIMER_INSTANTIATE:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.instantiate;
            *entityTimer = &comp->status.timers.instantiate;
            fnx = comp->methods.instantiateTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_INSTANTIATEDELAY:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.instantiateDelay;
            *entityTimer = &comp->status.timers.instantiateDelay;
            fnx = comp->methods.instantiateDelayTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_TERMINATE:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.terminate;
            *entityTimer = &comp->status.timers.terminate;
            fnx = comp->methods.terminateTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_CLEANUP:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.cleanup;
            *entityTimer = &comp->status.timers.cleanup;
            fnx = comp->methods.cleanupTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_AMSTART:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.amStart;
            *entityTimer = &comp->status.timers.amStart;
            fnx = comp->methods.amStartTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_AMSTOP:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.amStop;
            *entityTimer = &comp->status.timers.amStop;
            fnx = comp->methods.amStopTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.quiescingComplete;
            *entityTimer = &comp->status.timers.quiescingComplete;
            fnx = comp->methods.quiescingCompleteTimeout;
            break;
        };

        case CL_AMS_COMP_TIMER_CSISET:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.csiSet;
            *entityTimer = &comp->status.timers.csiSet;
            fnx = comp->methods.csiSetTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_CSIREMOVE:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.csiRemove;
            *entityTimer = &comp->status.timers.csiRemove;
            fnx = comp->methods.csiRemoveTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.proxiedCompInstantiate;
            *entityTimer = &comp->status.timers.proxiedCompInstantiate;
            fnx = comp->methods.proxiedCompInstantiateTimeout;
            break;
        }

        case CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP:
        {
            AMS_CHECKPTR( !comp );
            *duration = comp->config.timeouts.proxiedCompCleanup;
            *entityTimer = &comp->status.timers.proxiedCompCleanup;
            fnx = comp->methods.proxiedCompCleanupTimeout;
            break;
        }

        /*
         * SU Timers
         */

        case CL_AMS_SU_TIMER_COMPRESTART:
        {
            AMS_CHECKPTR( !sg || !su );
            *duration = sg->config.compRestartDuration;
            *entityTimer = &su->status.compRestartTimer;
            fnx = su->methods.compRestartTimeout;
            break;
        }

        case CL_AMS_SU_TIMER_SURESTART:
        {
            AMS_CHECKPTR( !sg || !su );
            *duration = sg->config.suRestartDuration;
            *entityTimer = &su->status.suRestartTimer;
            fnx = su->methods.suRestartTimeout;
            break;
        }

        case CL_AMS_SU_TIMER_PROBATION:
        {
            AMS_CHECKPTR( !sg || !su );
            *duration = sg->config.autoAdjustProbation;
            *entityTimer = &su->status.suProbationTimer;
            fnx = su->methods.suProbationTimeout;
            break;
        }

        case CL_AMS_SU_TIMER_ASSIGNMENT:
        {
            AMS_CHECKPTR( !sg || !su );
            *duration = CL_AMS_SU_ASSIGNMENT_DELAY;
            *entityTimer = &su->status.suAssignmentTimer;
            fnx = su->methods.suAssignmentTimeout;
            break;
        }

        /*
         * Node Timers
         */

        case CL_AMS_NODE_TIMER_SUFAILOVER:
        {
            AMS_CHECKPTR( !node );
            *duration = node->config.suFailoverDuration;
            *entityTimer = &node->status.suFailoverTimer;
            fnx = node->methods.suFailoverTimeout;
            break;
        }

        /*
         * SG Timers
         */

        case CL_AMS_SG_TIMER_INSTANTIATE:
        {
            AMS_CHECKPTR( !sg );
            *duration = sg->config.instantiateDuration;
            *entityTimer = &sg->status.instantiateTimer;
            fnx = sg->methods.instantiateTimeout;
            break;
        }

        case CL_AMS_SG_TIMER_ADJUST:
        {
            AMS_CHECKPTR( !sg );
            *duration = CL_AMS_SG_ADJUST_DURATION;
            *entityTimer = &sg->status.adjustTimer;
            fnx = sg->methods.adjustTimeout;
            break;
        }

        case CL_AMS_SG_TIMER_ADJUST_PROBATION:
        {
            AMS_CHECKPTR( !sg );
            *duration = CL_AMS_SG_ADJUST_PROBATION;
            *entityTimer = &sg->status.adjustProbationTimer;
            fnx = sg->methods.adjustProbationTimeout;
            break;
        }

        default:
        {
            AMS_LOG(CL_DEBUG_ERROR, 
                    ("ERROR: Invalid timer type[%d]\n", timerType));

            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }

    } // switch ( timerType )

    *fn = (ClTimerCallBackT) fnx;

    return rc;

}

ClRcT
clAmsEntityTimerCreate(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClTimerCallBackT  fn = {0};
    ClTimerTimeOutT  timeout= {0};
    ClAmsEntityTimerT  *entityTimer = NULL;

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    entityTimer->type = timerType;
    entityTimer->entity = entity;
    entityTimer->count = 0;
    entityTimer->handle = 0;

    /*
     * Duration is in msec and we could probably use it as-is if the two
     * types are the same. But duration is 64 bit and can hold a lot of
     * msec, while tsMilliSec is 32 bit, so we convert it to sec and msec 
     * before setting up a one shot timer.
     *
     * The timeout fn is set to a common fn where we will block on a
     * mutex. fn is not used here.
     */

    CL_AMS_TIMER_CONVERT(duration, timeout);
    
    if ( !duration )
    {
        AMS_ENTITY_LOG(entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Create: Entity [%s] Type [%s] has duration of [0s]. Timer will never pop\n",
                 entity->name.value,
                 CL_AMS_STRING_TIMER(timerType)));
    }

    AMS_CALL ( clTimerCreate (
                timeout,
                CL_TIMER_ONE_SHOT,
                CL_TIMER_SEPARATE_CONTEXT,
                (ClTimerCallBackT) clAmsEntityTimeout,
                (ClPtrT) entityTimer,
                &entityTimer->handle) );

    return CL_OK;

}

ClRcT
clAmsEntityTimerDelete(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClTimerCallBackT  fn = {0};

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    if (entityTimer)
    {
        if(entityTimer->handle)
        {
            AMS_CALL ( clTimerDelete (&entityTimer->handle) );
        }
        entityTimer->handle = 0;
        entityTimer->count = 0;
    }

    return CL_OK;

}

ClRcT
clAmsEntityTimerStart(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClTimerCallBackT  fn = {0};
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClAmsEntityStatusT  *status = NULL;
    ClAmsT  *ams = &gAms;

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    /*
     * If for some reason timer is not yet created, do so now.
     */

    if ( !entityTimer->handle )
    {
        AMS_CALL ( clAmsEntityTimerCreate (entity, timerType) );
    }

    /*
     * If timer is already running, stop currently running timer
     */

    if ( entityTimer->count )
    {
        AMS_CALL ( clTimerStop (entityTimer->handle) );
    }
    else
    {
        status = clAmsEntityGetStatusStruct(entity);
        status->timerCount++;
        ams->timerCount++;
    }

    AMS_CALL ( clTimerStart (entityTimer->handle) );

    AMS_ENTITY_LOG(entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
            ("Timer %s: Entity [%s] Type [%s]\n",
             entityTimer->count ? "Restart" : "Start",
             entity->name.value,
             CL_AMS_STRING_TIMER(timerType)));

    /*
     * Increment timer count. In the case of certain operations, there is only
     * one timer that tracks multiple operations/invocations.
     */

    entityTimer->count++;
    entityTimer->currentOp = ams->ops.currentOp;

    return CL_OK;

}


ClRcT
clAmsEntityTimerStop(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClTimerCallBackT  fn = {0};
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClAmsEntityStatusT  *status = NULL;
    ClAmsT *ams = &gAms;

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    if ( !entityTimer->count )
    {
        return CL_OK;
    }

    entityTimer->count = 0; 

    status = clAmsEntityGetStatusStruct(entity);
    status->timerCount--;
    ams->timerCount--;

    if ( entityTimer->handle )
    {
        AMS_CALL ( clTimerStop (entityTimer->handle) );

        AMS_ENTITY_LOG(entity,CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Stop: Entity [%s] Type [%s]\n",
                 entity->name.value,
                 CL_AMS_STRING_TIMER(timerType)));
    }

    return CL_OK;

}

ClRcT
clAmsEntityTimerStopIfCountZero(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClTimerCallBackT  fn = {0};
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClAmsEntityStatusT  *status = NULL;
    ClAmsT  *ams = &gAms;

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    if ( !entityTimer->count )
    {
        return CL_OK;
    }

    entityTimer->count--;

    if ( entityTimer->count )
    {
        return CL_OK;
    }

    status = clAmsEntityGetStatusStruct(entity);
    status->timerCount--;
    ams->timerCount--;

    if ( entityTimer->handle )
    {
        AMS_CALL ( clTimerStop (entityTimer->handle) );

        AMS_ENTITY_LOG(entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Stop: Entity [%s] Type [%s]\n",
                 entity->name.value,
                 CL_AMS_STRING_TIMER(timerType)));
    }

    return CL_OK;

}

ClRcT
clAmsEntityTimerIsRunning(
        CL_IN  ClAmsEntityT  *entity, 
        CL_IN  ClAmsEntityTimerTypeT  timerType )
{

    ClTimeT  duration = 0;
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClTimerCallBackT  fn = {0};

    AMS_CHECKPTR ( !entity );

    AMS_CALL ( clAmsEntityTimerGetValues(
                entity,
                timerType,
                &duration,
                &entityTimer,
                &fn) );

    if ( entityTimer->count )
    {
        return CL_TRUE;
    }
    else
    {
        return CL_FALSE;
    }

}

ClRcT
clAmsEntityTimeout(
        CL_IN  ClAmsEntityTimerT  *timer ) 
{

    ClRcT  rc = CL_OK;
    ClTimerCallBackT  fn = {0};
    ClTimeT  duration = 0;
    ClAmsEntityTimerT  *entityTimer = NULL;
    ClAmsT  *ams = &gAms;

    AMS_CHECKPTR ( !timer );
    AMS_CHECKPTR ( !timer->entity);

    /*
     * We just want the fn, nothing else
     */

    AMS_CALL ( clAmsEntityTimerGetValues(
                timer->entity,
                timer->type,
                &duration,
                &entityTimer,
                &fn) );

    if ( !fn || (fn == (ClTimerCallBackT) clAmsEntityTimeout) )
    {
        AMS_ENTITY_LOG(timer->entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
            ("ERROR: Timeout fn for timer[%d] is not configured for entity[%s]\n",
             timer->type, timer->entity->name.value));

        return CL_AMS_RC(CL_ERR_TIMEOUT);
    }

    /*
     * Serialize access so timeouts don't collide with each other and with
     * fns invoked via various client APIs. We also check if the timer is
     * still running after acquiring the mutex, because the timer could be
     * cleared in a race condition while it was waiting for the mutex.
     */

    AMS_CALL ( clOsalMutexLock(ams->mutex) );

    if ( !timer->count )
    {
        AMS_ENTITY_LOG(timer->entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Pop/Ignored: Entity [%s] Type [%s] : Ignoring timer as it was cleared while waiting for mutex\n", 
                 timer->entity->name.value,
                 CL_AMS_STRING_TIMER(timer->type)));
    }
    else
    {
        AMS_ENTITY_LOG(timer->entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Pop/Enter: Entity [%s] Type [%s]\n", 
                 timer->entity->name.value,
                 CL_AMS_STRING_TIMER(timer->type)));

        ams->ops.currentOp = timer->currentOp;

        rc = fn(timer);

        AMS_ENTITY_LOG(timer->entity, CL_AMS_MGMT_SUB_AREA_TIMER, CL_DEBUG_TRACE,
                ("Timer Pop/Exit: Entity [%s] Type [%s]\n", 
                 timer->entity->name.value,
                 CL_AMS_STRING_TIMER(timer->type)));
    }

    AMS_CALL ( clOsalMutexUnlock(ams->mutex) );

    if ( !ams->timerCount && (ams->serviceState == CL_AMS_SERVICE_STATE_STOPPED) )
    {
        AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Pending timers have popped. AMF Stopped\n"));

        AMS_CALL ( clOsalMutexLock(ams->terminateMutex) );
        AMS_CALL ( clOsalCondSignal(ams->terminateCond) );
        AMS_CALL ( clOsalMutexUnlock(ams->terminateMutex) );
    }

    return rc;

}

/******************************************************************************
 * EntityRef functions
 *****************************************************************************/

ClRcT
clAmsEntityRefGetKey(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClUint32T  rank,
        CL_OUT  ClCntKeyHandleT  *entityKeyHandle,
        CL_IN  ClBoolT  isRankedList )
{

#ifdef CL_AMS_RANK_LIST 

    if (isRankedList)
    {
        *entityKeyHandle = (ClPtrT)(ClWordT)rank;
        return CL_OK;
    }

#endif

    AMS_CALL ( clAmsEntityGetKey(entity, entityKeyHandle) );

    return CL_OK;

}

/******************************************************************************
 * Entity Specific Functions
 *****************************************************************************/

ClRcT
clAmsCSIMarshalCSIDescriptorExtended(
                                     CL_OUT  ClAmsCSIDescriptorT  **csiDescriptorPtr,
                                     CL_IN  ClAmsCSIT  *csi,
                                     CL_IN  ClAmsCSIFlagsT  csiFlags,
                                     CL_IN  ClAmsHAStateT haState,
                                     // coverity[pass_by_value]
                                     CL_IN  ClNameT  activeCompName,
                                     CL_IN  ClAmsCSITransitionDescriptorT  transitionDescr,
                                     CL_IN  ClUint32T  standbyRank,
                                     CL_IN  ClBoolT    reassignCSI)
{

    ClRcT  rc = CL_OK;
    ClAmsCSIDescriptorT  *csiDescriptor = NULL;
    ClCntNodeHandleT  nodeHandle = 0;
    
    AMS_CHECKPTR ( !csiDescriptorPtr );

    csiDescriptor = clHeapCalloc (1, sizeof (ClAmsCSIDescriptorT));

    AMS_CHECK_NO_MEMORY (csiDescriptor);

    /*
     * Write CSI Flags CSI Name and CSI State Descriptor
     */

    csiDescriptor->csiFlags = csiFlags;

    if (csi && (CL_AMS_CSI_FLAG_TARGET_ALL != csiFlags))
    {
        memcpy ( &csiDescriptor->csiName, &csi->config.entity.name, sizeof (ClNameT));
    }

    if (csi && 
        (
         (CL_AMS_CSI_FLAG_ADD_ONE == csiFlags)
         || 
         reassignCSI))
    {
        /*
         * Make the name value pair list 
         */
        ClUint32T  numAttributes= 0 ;
        ClCntHandleT  nvpList = csi->config.nameValuePairList;
        ClUint32T  index = 0;
        ClAmsCSINameValuePairT  *pNVP = NULL;

        /*
         * Send TARGET_ONE in case of CSI reassignment after component restart
         */
        if(reassignCSI) 
            csiDescriptor->csiFlags = CL_AMS_CSI_FLAG_TARGET_ONE; 

        /*
         * Get the number of NVP's in the NVP list
         */
        AMS_CHECK_RC_ERROR( clCntSizeGet(nvpList,
                                         &numAttributes) );

        csiDescriptor->csiAttributeList.numAttributes=  numAttributes; 
        csiDescriptor->csiAttributeList.attribute = 
            clHeapAllocate(sizeof (ClAmsCSIAttributeT)*numAttributes);

        AMS_CHECK_NO_MEMORY ( csiDescriptor->csiAttributeList.attribute ); 
        
        for ( pNVP = (ClAmsCSINameValuePairT*)clAmsCntGetFirst(
                                                               &nvpList,&nodeHandle); pNVP != NULL;
              pNVP  = (ClAmsCSINameValuePairT*)clAmsCntGetNext(
                                                               &nvpList,&nodeHandle) )
        {

            AMS_CHECKPTR_AND_EXIT ( !pNVP );

            csiDescriptor->csiAttributeList.attribute[index].attributeName = 
                (ClUint8T*) pNVP->paramName.value;
            csiDescriptor->csiAttributeList.attribute[index].attributeValue = 
                (ClUint8T*) pNVP->paramValue.value;
            index++; 
        }
    }

    if (CL_AMS_HA_STATE_ACTIVE == haState)
    {
        csiDescriptor->csiStateDescriptor.activeDescriptor.transitionDescriptor = transitionDescr;
        memcpy ( &csiDescriptor->csiStateDescriptor.activeDescriptor.activeCompName,
                 &activeCompName, sizeof (ClNameT)) ;
    }
    else if (CL_AMS_HA_STATE_STANDBY == haState)
    {
        memcpy ( &csiDescriptor->csiStateDescriptor.standbyDescriptor.activeCompName,
                 &activeCompName, sizeof (ClNameT)) ;
        csiDescriptor->csiStateDescriptor.standbyDescriptor.standbyRank = standbyRank;
    }

    *csiDescriptorPtr = csiDescriptor;

    return CL_OK;

    exitfn:

    if (csi)
    {
        clHeapFree(csiDescriptor->csiAttributeList.attribute);
    }

    clHeapFree(csiDescriptor);
    return CL_AMS_RC (rc);

}

ClRcT
clAmsCSIMarshalCSIDescriptor(
        CL_OUT  ClAmsCSIDescriptorT  **csiDescriptorPtr,
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClAmsCSIFlagsT  csiFlags,
        CL_IN  ClAmsHAStateT haState,
        // coverity[pass_by_value]
        CL_IN  ClNameT  activeCompName,
        CL_IN  ClAmsCSITransitionDescriptorT  transitionDescr,
        CL_IN  ClUint32T  standbyRank )
{
    return clAmsCSIMarshalCSIDescriptorExtended(csiDescriptorPtr, csi, csiFlags,
                                                haState, activeCompName, transitionDescr, 
                                                standbyRank, CL_FALSE);
}

ClRcT
clAmsCSIMarshalPGTrackNotificationBuffer(
        CL_OUT  ClAmsPGNotificationBufferT  **notificationBufferPtr,
        CL_IN  ClAmsCSIT  *csi,
        CL_IN  ClAmsCompT  *changedComp,
        CL_IN  ClAmsPGTrackFlagT  pgTrackFlags,
        CL_IN  ClAmsPGChangeT  pgChange )
{

    ClAmsPGNotificationBufferT  *notificationBuffer = NULL;
    ClAmsEntityRefT  *entityRef = NULL;
    ClUint32T  count = 0;

    AMS_CHECKPTR ( !notificationBufferPtr || !csi );

    notificationBuffer = clHeapAllocate (sizeof (ClAmsPGNotificationBufferT));

    AMS_CHECK_NO_MEMORY ( notificationBuffer );

    /*
     * Go through the csiStatus PG List and pack in the buffer
     */

    if ( pgTrackFlags&CL_AMS_PG_TRACK_CURRENT )
    {

        notificationBuffer->notification = clHeapAllocate(
                sizeof(ClAmsPGNotificationT)*csi->status.pgList.numEntities);

        AMS_CHECK_NO_MEMORY ( notificationBuffer->notification);

        notificationBuffer->numItems = 0;

        for ( count = 0, entityRef = clAmsEntityListGetFirst(
                    &csi->status.pgList); entityRef != (ClAmsEntityRefT *) NULL;
                count++, entityRef = clAmsEntityListGetNext(
                    &csi->status.pgList, entityRef) )
        {

            ClAmsCSICompRefT  *compRef = (ClAmsCSICompRefT *) entityRef;
            ClAmsCompT  *comp = (ClAmsCompT *)compRef->entityRef.ptr;

            memcpy ( &notificationBuffer->notification[count].member.compName,
                    &comp->config.entity.name, sizeof (ClNameT));
            notificationBuffer->notification[count].member.haState = compRef->haState;
            //XXX: notificationBuffer->notification[count].member.rank = comp->status
            notificationBuffer->notification[count].change = CL_AMS_PG_NO_CHANGE; 
            notificationBuffer->numItems++;

        }

    }

    else if ( pgTrackFlags&CL_AMS_PG_TRACK_CHANGES )
    {

        notificationBuffer->notification = 
            clHeapAllocate (sizeof(ClAmsPGNotificationT)*csi->status.pgList.numEntities);

        AMS_CHECK_NO_MEMORY ( notificationBuffer->notification );

        notificationBuffer->numItems = 0;

        for ( count = 0, entityRef = clAmsEntityListGetFirst(
                    &csi->status.pgList); entityRef != (ClAmsEntityRefT *) NULL;
                count++, entityRef = clAmsEntityListGetNext(
                    &csi->status.pgList, entityRef) )
        {

            ClAmsCSICompRefT  *compRef = (ClAmsCSICompRefT *) entityRef;
            ClAmsCompT  *comp    = (ClAmsCompT *)compRef->entityRef.ptr;

            memcpy ( &notificationBuffer->notification[count].member.compName,
                    &comp->config.entity.name, sizeof (ClNameT));
            notificationBuffer->notification[count].member.haState = compRef->haState;
            //XXX: notificationBuffer->notification[count].member.rank = comp->status

            if ( !strcmp(changedComp->config.entity.name.value, comp->config.entity.name.value))
            {
                notificationBuffer->notification[count].change = pgChange;
            }

            else
            {
                notificationBuffer->notification[count].change = CL_AMS_PG_NO_CHANGE; 
            }

            notificationBuffer->numItems++;

        }

    }

    else if ( pgTrackFlags&CL_AMS_PG_TRACK_CHANGES_ONLY )
    {

        notificationBuffer->notification = 
            clHeapAllocate (sizeof(ClAmsPGNotificationT));

        AMS_CHECK_NO_MEMORY ( notificationBuffer->notification );

        notificationBuffer->numItems = 0;

        for ( entityRef = clAmsEntityListGetFirst(&csi->status.pgList);
              entityRef != (ClAmsEntityRefT *) NULL; entityRef = 
              clAmsEntityListGetNext(&csi->status.pgList, entityRef) )
        {

            ClAmsCSICompRefT  *compRef = (ClAmsCSICompRefT *) entityRef;
            ClAmsCompT  *comp = (ClAmsCompT *)compRef->entityRef.ptr;

            if ( !strcmp(changedComp->config.entity.name.value, comp->config.entity.name.value))
            {

                memcpy ( &notificationBuffer->notification[0].member.compName,
                        &comp->config.entity.name, sizeof (ClNameT));
                notificationBuffer->notification[0].member.haState = compRef->haState;
                notificationBuffer->notification[0].change = pgChange;
                notificationBuffer->numItems++;
                //XXX: notificationBuffer->notification[count].member.rank = comp->status
                *notificationBufferPtr = notificationBuffer;

                return CL_OK;

            }

        }

    } 
    
    *notificationBufferPtr = notificationBuffer;

    return CL_OK;

}

static __inline__ ClRcT clAmsEntityOpPushStack(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClAmsEntityOpT *opBlock)
{
    ClAmsEntityOpStackT *opStack = NULL;
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    if(!opStack->opList.pNext) CL_LIST_HEAD_INIT(&opStack->opList);
    ++opStack->numOps;
    clListAdd(&opBlock->list, &opStack->opList);
    return CL_OK;
}

ClRcT clAmsEntityOpPush(ClAmsEntityT *entity,
                        ClAmsEntityStatusT *status,
                        void *data,
                        ClUint32T dataSize,
                        ClUint32T op)
{
    ClAmsEntityOpT *opBlock = NULL;
    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    opBlock = clHeapCalloc(1, sizeof(*opBlock));
    CL_ASSERT(opBlock != NULL);
    opBlock->op = op;
    opBlock->dataSize = dataSize;
    opBlock->data = NULL;
    if(dataSize)
    {
        opBlock->data = clHeapCalloc(1, dataSize);
        CL_ASSERT(opBlock->data != NULL);
        memcpy(opBlock->data, data, dataSize);
    }
    return clAmsEntityOpPushStack(entity, status, opBlock);
}

static __inline__ ClAmsEntityOpT *clAmsEntityOpPopStack(ClAmsEntityT *entity, ClAmsEntityStatusT *status)
{
    ClAmsEntityOpStackT *opStack = NULL;
    ClAmsEntityOpT *opBlock = NULL;
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    if(!opStack->numOps) return NULL;
    opBlock = CL_LIST_ENTRY(opStack->opList.pNext, ClAmsEntityOpT, list);
    clListDel(opStack->opList.pNext);
    --opStack->numOps;
    return opBlock;
}

ClRcT clAmsEntityOpPop(ClAmsEntityT *entity, ClAmsEntityStatusT *status, 
                       void **data, ClUint32T *dataSize, ClUint32T *op)
{
    ClAmsEntityOpT *opBlock = NULL;
    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    opBlock = clAmsEntityOpPopStack(entity, status);
    if(!opBlock) return CL_AMS_RC(CL_ERR_NOT_EXIST);
    if(data)  *data = opBlock->data;
    if(dataSize) *dataSize = opBlock->dataSize;
    if(op) *op = opBlock->op;
    clHeapFree(opBlock);
    return CL_OK;
}

ClBoolT clAmsEntityOpPending(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op)
{
    ClAmsEntityOpStackT *opStack = NULL;
    ClListHeadT *iter = NULL;
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    CL_LIST_FOR_EACH(iter, &opStack->opList)
    {
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(iter, ClAmsEntityOpT, list);
        if( (opBlock->op & op) )  return CL_TRUE;
    }    
    return CL_FALSE;
}

ClRcT clAmsEntityOpGet(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op,
                       void **data, ClUint32T *dataSize)
{
    ClAmsEntityOpStackT *opStack = NULL;
    ClListHeadT *iter = NULL;
    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    CL_LIST_FOR_EACH(iter, &opStack->opList)
    {
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(iter, ClAmsEntityOpT, list);
        if(opBlock->op == op)
        {
            if(data) *data = opBlock->data;
            if(dataSize) *dataSize = opBlock->dataSize;
            return CL_OK;
        }
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT clAmsEntityOpClear(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op, 
                         void **data, ClUint32T *dataSize)
{
    ClAmsEntityOpStackT *opStack = NULL;
    ClListHeadT *iter = NULL;
    if(!entity) return CL_AMS_RC(CL_ERR_NOT_EXIST);
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    CL_LIST_FOR_EACH(iter, &opStack->opList)
    {
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(iter, ClAmsEntityOpT, list);
        if(opBlock->op == op)
        {
            clListDel(iter);
            if(dataSize) *dataSize = opBlock->dataSize;
            if(data) *data = opBlock->data;
            clHeapFree(opBlock);
            --opStack->numOps;
            return CL_OK;
        }
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT clAmsEntityOpsClear(ClAmsEntityT *entity, ClAmsEntityStatusT *status)
{
    ClAmsEntityOpStackT *opStack = NULL;
    if(!status)
        status = clAmsEntityGetStatusStruct(entity);
    opStack = &status->opStack;
    if(!opStack->numOps) return CL_OK;
    while(!CL_LIST_HEAD_EMPTY(&opStack->opList))
    {
        ClListHeadT *entry = opStack->opList.pNext;
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(entry, ClAmsEntityOpT, list);
        clListDel(entry);
        if(opBlock->data) clHeapFree(opBlock->data);
        clHeapFree(opBlock);
    }
    opStack->numOps = 0;
    return CL_OK;
}

#ifdef POST_RC2

/******************************************************************************
 * Entity Stack Operations
 * -- currently unused
 *****************************************************************************/

ClRcT
clAmsEntityCreateOpStack(
        ClAmsEntityT *entity)
{
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    stack->numOps = 0;
    stack->top = NULL;
    stack->bottom = NULL;

    return CL_OK;
}

ClRcT
clAmsEntityPushOpTop(
        ClAmsEntityT *entity,
        ClAmsEntityOpT *operation)
{
    ClAmsEntityOpBlockT *block;
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    AMS_CHECKPTR (!operation);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    block = clHeapAllocate(sizeof(*block));
    block->op = operation;
    block->prev = stack->top;
    block->next = NULL;
    stack->top = block;
    if ( stack->bottom == NULL )
        stack->bottom = block;

    stack->numOps++;

    return CL_OK;
}

ClRcT
clAmsEntityPopOpTop(
        ClAmsEntityT *entity,
        ClAmsEntityOpT *operation)
{
    ClAmsEntityOpBlockT *block;
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    if ( stack->top == NULL )
    {
        operation = NULL;
        return CL_OK;
    }

    block = stack->top;
    operation = block->op;
    stack->top = block->prev;
    if ( stack->top == NULL )
        stack->bottom = NULL;
    clHeapFree(block);

    stack->numOps--;

    return CL_OK;
}

ClRcT
clAmsEntityPushOpBottom(
        ClAmsEntityT *entity,
        ClAmsEntityOpT *operation)
{
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    return CL_OK;
}

ClRcT
clAmsEntityPopOpBottom(
        ClAmsEntityT *entity,
        ClAmsEntityOpT *operation)
{
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    return CL_OK;
}

ClRcT
clAmsEntityDestroyOpStack(
        ClAmsEntityT *entity)
{
    ClAmsEntityOpT *operation;
    ClAmsEntityOpStackT *stack;
    ClAmsEntityStatusT *status;

    AMS_CHECKPTR (!entity);
    
    status = clAmsEntityGetStatusStruct(entity);
    AMS_CHECKPTR (!status);
    stack = &status->stack;

    while ( stack->top ) 
    {
        clAmsEntityPopOpTop(entity, operation);
        clHeapFree(operation);
    }

   return CL_OK;
}

#endif

