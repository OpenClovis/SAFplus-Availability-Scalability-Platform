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
 * File        : clAmsModify.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <string.h>
#include <clCksmApi.h>
#include <clAmsErrors.h>
#include <clAmsServerUtils.h>
#include <clAmsModify.h>
#include <clAmsPolicyEngine.h>
#include <clAmsCkpt.h>
#include <clAmsDBPackUnpack.h>
#include <clAmsSAServerApi.h>
#include <clAmsEntityUserData.h>
#include <clAmsXdrHeaderFiles.h>
#include <clAmsMgmtDebugCli.h>
#include <clCpmExtApi.h>
#include <clList.h>
#include <clNodeCache.h>

/******************************************************************************
 * Global data structures
 *****************************************************************************/
/*
 * This is the global cluster reference. In future this should be replaced by specifc cluster pointer.
 */

extern ClAmsT  gAms;

/*
 */

typedef struct
{
    ClCntKeyCompareCallbackT    cntListKeyCompareCallback;
    ClCntDeleteCallbackT        cntListDeleteCallback;
    ClCntDeleteCallbackT        cntListDestroyCallback;
} ClAmsCntListParamsT;


ClAmsCntListParamsT ClAmsCntListParams[]=

{
    /*
     * CCB Operation List Parameters
     */
    {
        clAmsEntityDbEntityKeyCompareCallback,
        clAmsCCBDeleteCallback,
        clAmsCCBDeleteCallback,
    }

};

typedef struct ClAmsEntityDBWalkArgs
{
    ClBufferHandleT inMsgHdl;
    ClUint32T versionCode;
}ClAmsEntityDBWalkArgsT;

/*
 * All entities in the AMS database are dumped into a XML file. This strcture holds the pointers to
 * the tags for specific entities. This is the root/key to generated XML tree.
 */

extern ClAmsParserPtrT  amfPtr;

/*
 * All invocation entries in the invocation list are dumped into a XML file. This pointer is the
 * key/root for the XML generated for AMS invocations.
 */

extern ClParserPtrT  invocationPtr;

/*
 * Attributes for each AMS entity database
 */

extern ClAmsEntityDbParamsT    gClAmsEntityDbParams[];

typedef struct ClAmsCSIRemoveInvocation
{
    ClNameT *pNodeName;
    ClAmsInvocationT *pInvocations;
    ClInt32T nr;
}ClAmsCSIRemoveInvocationT;

typedef struct ClAmsSISUDeleteRef
{
    ClCntKeyHandleT key;
    ClAmsEntityRefT *suRef;
}ClAmsSISUDeleteRefT;


/******************************************************************************
 * AMS Methods
 *****************************************************************************/

#ifdef POST_RC2

/***************************************************************************
 * clAmsEnable 
 * -----------
 * Enable the instance of ams.
 *
 ***************************************************************************/

ClRcT
clAmsEnable(
        CL_IN  ClAmsT  *ams)
{
    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !ams );

    ams->isEnabled = CL_TRUE;

    rc = clAmsPeClusterInstantiate(ams);

    return CL_AMS_RC (rc);

}

/***************************************************************************
 * clAmsDisable 
 * ------------
 * Disable the instance of the ams.
 *
 ***************************************************************************/

ClRcT
clAmsDisable(
        CL_IN  ClAmsT  *ams )
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !ams );

    ams->isEnabled = CL_FALSE;

    rc = clAmsPeClusterTerminate(ams);

    return CL_AMS_RC (rc);

}

#endif

/***************************************************************************
 * clAmsEntitySetConfig 
 * -------------------
 * Set the config part of the entity.
 *
 ***************************************************************************/

ClRcT
clAmsEntitySetConfig(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  const ClAmsEntityT  *entityName,
        CL_IN  ClAmsEntityConfigT  *entityConfig )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0},0,0};
    ClAmsEntityT  *entity  = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityDb || !entityName || !entityConfig );

    memcpy (&entityRef.entity,entityName,sizeof(ClAmsEntityT));
    entityRef.ptr = NULL;
   
    AMS_CALL( clAmsEntityDbFindEntity(
                entityDb,
                &entityRef) );
    
    /*
     * Set the config portion of the entity
     */

    entity = entityRef.ptr;

    AMS_CHECKPTR ( !entity );

    switch ( entityDb->type )
    {
        case CL_AMS_ENTITY_TYPE_ENTITY:
        {
            /*
             * No config to set
             */
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            /*
             * No config to set
             */
            break;
        }
#endif

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClAmsNodeConfigT  *nodeConfig = &node->config;
            ClAmsEntityListT  nodeDependentsList = {0};
            ClAmsEntityListT  nodeDependenciesList = {0};
            ClAmsEntityListT  suList = {0};


            /* 
             * Get the list part of the entityConfig and save it.
             * We only set the non list part of the node. Copy the list pointers
             * since we will overwrite these pointers using memcpy. Copy back these
             * pointers once the node configuration is updated with the entityConfig values
             */

            memcpy (&nodeDependentsList,&nodeConfig->nodeDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&nodeDependenciesList,&nodeConfig->nodeDependenciesList,sizeof (ClAmsEntityListT));
            memcpy (&suList,&nodeConfig->suList,sizeof (ClAmsEntityListT));

            memcpy (nodeConfig, entityConfig, sizeof (ClAmsNodeConfigT));

            memcpy (&nodeConfig->nodeDependentsList,&nodeDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&nodeConfig->nodeDependenciesList,&nodeDependenciesList,sizeof (ClAmsEntityListT));
            memcpy (&nodeConfig->suList,&suList,sizeof (ClAmsEntityListT));

            break;

        }

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            ClAmsSGConfigT  *sgConfig = &sg->config;
            ClAmsEntityListT  suList = {0};
            ClAmsEntityListT  siList = {0};

            memcpy (&suList,&sgConfig->suList,sizeof (ClAmsEntityListT));
            memcpy (&siList,&sgConfig->siList,sizeof (ClAmsEntityListT));

            memcpy (sgConfig, entityConfig, sizeof (ClAmsSGConfigT));

            memcpy (&sgConfig->suList,&suList,sizeof (ClAmsEntityListT));
            memcpy (&sgConfig->siList,&siList,sizeof (ClAmsEntityListT));

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT  *su = (ClAmsSUT *) entity;
            ClAmsSUConfigT  *suConfig = &su->config;
            ClAmsEntityListT  compList = {0};

            memcpy (&compList,&suConfig->compList,sizeof (ClAmsEntityListT));

            memcpy (suConfig, entityConfig, sizeof (ClAmsSUConfigT));

            memcpy (&suConfig->compList,&compList,sizeof (ClAmsEntityListT));
            
            break;

        }
        case CL_AMS_ENTITY_TYPE_SI:
        {

            ClAmsSIT  *si = (ClAmsSIT *) entity;
            ClAmsSIConfigT  *siConfig = &si->config;
            ClAmsEntityListT  suList = {0};  
            ClAmsEntityListT  siDependentsList = {0};  
            ClAmsEntityListT  siDependenciesList = {0}; 
            ClAmsEntityListT  csiList = {0}; 

            /* 
             * Get the config and save the list pointers
             */

            memcpy (&suList,&siConfig->suList,sizeof (ClAmsEntityListT));
            memcpy (&siDependentsList,&siConfig->siDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&siDependenciesList,&siConfig->siDependenciesList,sizeof (ClAmsEntityListT));
            memcpy (&csiList,&siConfig->csiList,sizeof (ClAmsEntityListT));

            memcpy (siConfig, entityConfig, sizeof (ClAmsSIConfigT));

            memcpy (&siConfig->suList,&suList,sizeof (ClAmsEntityListT));
            memcpy (&siConfig->siDependentsList,&siDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&siConfig->siDependenciesList,&siDependenciesList,sizeof (ClAmsEntityListT));
            memcpy (&siConfig->csiList,&csiList,sizeof (ClAmsEntityListT));

            break;

        }
        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT  *comp = (ClAmsCompT *) entity;
            ClAmsCompConfigT  *compConfig = &comp->config;
            if(compConfig->pSupportedCSITypes)
            {
                clHeapFree(compConfig->pSupportedCSITypes);
                compConfig->pSupportedCSITypes = NULL;
            }
            memcpy (compConfig, entityConfig, (ClUint32T)sizeof(ClAmsCompConfigT) );

            break;

        }
        case CL_AMS_ENTITY_TYPE_CSI:
        {

            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;
            ClAmsCSIConfigT  *csiConfig = &csi->config;
            ClCntHandleT  nameValuePairList = csiConfig->nameValuePairList;
            ClAmsEntityListT csiDependenciesList = {0};
            ClAmsEntityListT csiDependentsList = {0};

            memcpy (&csiDependentsList,&csiConfig->csiDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&csiDependenciesList,&csiConfig->csiDependenciesList,sizeof (ClAmsEntityListT));

            memcpy (csiConfig,entityConfig,sizeof (ClAmsCSIConfigT) );
            csiConfig->nameValuePairList = nameValuePairList;

            memcpy (&csiConfig->csiDependentsList,&csiDependentsList,sizeof (ClAmsEntityListT));
            memcpy (&csiConfig->csiDependenciesList, &csiDependenciesList,sizeof (ClAmsEntityListT));
            
            break;

        }

        default:
        {

            rc = CL_AMS_ERR_INVALID_ENTITY;
            AMS_LOG (CL_DEBUG_ERROR,("Invalid entity type [%d] \n", entityDb->type));
            goto exitfn;

        }
    }

exitfn:

    return CL_AMS_RC (rc);

}

/***************************************************************************
 * clAmsEntitySetAlphaFactor
 * -------------------
 * Set the alpha factor for the SG
 *
 ***************************************************************************/

ClRcT
clAmsEntitySetAlphaFactor( 
                          CL_IN ClAmsEntityT *entity,
                          CL_IN ClUint32T alphaFactor
                           )
{
    ClAmsSGT *sg = (ClAmsSGT*) entity;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR( !sg);

    AMS_FUNC_ENTER(("\n"));

    if(alphaFactor > 100)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Invalid alpha factor value [%d]. "\
                                 "Should be between 0 and 100\n", 
                                 alphaFactor));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * We accept any valid values of alpha factor here. A reduction in 
     * alpha factor would result in active assignments getting reduced as and
     * when the service units are taken down and re-evaluated.
     * An alpha factor of 0 would result in 1 active SU incase there are less
     * number of available SUs than the preferred active SUs.
     */

    sg->config.alpha = alphaFactor;

    clAmsCkptDBWrite();

    if( (rc = clAmsPeSGEvaluateWork( sg ) ) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SG work evaluation returned [%#x]\n", rc));
    }

    return CL_OK;
}

ClRcT
clAmsEntitySetBetaFactor( 
                          CL_IN ClAmsEntityT *entity,
                          CL_IN ClUint32T betaFactor
                           )
{
    ClAmsSGT *sg = (ClAmsSGT*) entity;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR( !sg);

    AMS_FUNC_ENTER(("\n"));

    if(betaFactor > 100)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Invalid beta factor value [%d]. "\
                                 "Should be between 0 and 100\n", 
                                 betaFactor));
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /*
     * We accept any valid values of beta factor here. A reduction in 
     * alpha factor would result in active assignments getting reduced as and
     * when the service units are taken down and re-evaluated.
     * An alpha factor of 0 would result in 1 active SU incase there are less
     * number of available SUs than the preferred active SUs.
     */

    sg->config.beta = betaFactor;

    clAmsCkptDBWrite();

    if( (rc = clAmsPeSGEvaluateWork( sg ) ) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SG work evaluation returned [%#x]\n", rc));
    }

    return CL_OK;
}

static ClRcT clAmsRankUpdate(ClAmsEntityT *entity, ClUint32T newRank)
{
    ClRcT rc = CL_OK;

    switch(entity->type)
    {
    case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT *su = (ClAmsSUT*)entity;
            ClAmsSGT *sg = (ClAmsSGT*)su->config.parentSG.ptr;
            ClUint32T lastRank = su->config.rank;
            ClCntKeyHandleT key = 0;
            ClAmsEntityRefT *ref = NULL;

            if(!sg) 
            {
                su->config.rank = newRank;
                return CL_OK;
            }
            /*
             * Delete from existing SG config list and re-add with the new rank.
             */
            clAmsDeleteFromEntityList((ClAmsEntityT*)sg,
                                      entity,
                                      CL_AMS_SG_CONFIG_SU_LIST);
            su->config.rank = newRank;
            AMS_CHECK_RC_ERROR ( clAmsAddToEntityList((ClAmsEntityT*)sg,
                                 entity,
                                 CL_AMS_SG_CONFIG_SU_LIST) );
            /*
             * Now find the SU in the SI list and delete it from there.
             */
            if(clAmsEntityRefGetKey(
                                    &su->config.entity,
                                    lastRank,
                                    &key,
                                    CL_TRUE) == CL_OK)
            {
                ClAmsEntityRefT entityRef = {{0}};
                memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
                entityRef.ptr = entity;
                /*
                 * Delete from SI su rank list.
                 */
                for(ref = clAmsEntityListGetFirst(&sg->config.siList);
                    ref != NULL;
                    ref = clAmsEntityListGetNext(&sg->config.siList, ref))
                {
                    ClAmsSIT *si = (ClAmsSIT*)ref->ptr;
                    if(clAmsEntityListDeleteEntityRef(&si->config.suList,
                                                      &entityRef,
                                                      key) == CL_OK)
                    {
                        AMS_CHECK_RC_ERROR ( clAmsAddToEntityList(&si->config.entity,
                                             &su->config.entity,
                                             CL_AMS_SI_CONFIG_SU_RANK_LIST) );
                    }
                }
                       
            }
        }
        break;

    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT *si = (ClAmsSIT*)entity;
            ClAmsSGT *sg =  (ClAmsSGT*)si->config.parentSG.ptr;
            if(!sg)
            {
                si->config.rank = newRank;
                return CL_OK;
            }
            clAmsDeleteFromEntityList((ClAmsEntityT*)sg,
                                      entity,
                                      CL_AMS_SG_CONFIG_SI_LIST);
            si->config.rank = newRank;
            AMS_CHECK_RC_ERROR ( clAmsAddToEntityList((ClAmsEntityT*)sg,
                                 entity,
                                 CL_AMS_SG_CONFIG_SI_LIST) );
        }
        break;

    case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *comp = (ClAmsCompT*)entity;
            ClAmsSUT *su = (ClAmsSUT*)comp->config.parentSU.ptr;
            if(!su)
            {
                comp->config.instantiateLevel = newRank;
                return CL_OK;
            }
            clAmsDeleteFromEntityList((ClAmsEntityT*)su, entity,
                                      CL_AMS_SU_CONFIG_COMP_LIST);
            comp->config.instantiateLevel = newRank;
            AMS_CHECK_RC_ERROR ( clAmsAddToEntityList((ClAmsEntityT*)su,
                                 entity,
                                 CL_AMS_SU_CONFIG_COMP_LIST) );

        }
        break;
    default:
        break;
    }

    exitfn:
    return rc;
}

static void
amsUpdateCompTimers(ClAmsCompT *comp, ClAmsCompTimerDurationsT *oldTimeouts)
{
    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_INSTANTIATE,
                           oldTimeouts->instantiate);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_TERMINATE,
                           oldTimeouts->terminate);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_CLEANUP,
                           oldTimeouts->cleanup);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE,
                           oldTimeouts->quiescingComplete);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_CSISET,
                           oldTimeouts->csiSet);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_CSIREMOVE,
                           oldTimeouts->csiRemove);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE,
                           oldTimeouts->proxiedCompInstantiate);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP,
                           oldTimeouts->proxiedCompCleanup);

    clAmsEntityTimerUpdate(&comp->config.entity, CL_AMS_COMP_TIMER_INSTANTIATEDELAY,
                           oldTimeouts->instantiateDelay);
}

/***************************************************************************
 * clAmsEntitySetConfigNew 
 * -------------------
 * Set the config part of the entity.
 *
 ***************************************************************************/

ClRcT
clAmsEntitySetConfigNew(
        CL_IN  ClAmsEntityConfigT  *entityConfig,
        CL_IN  ClUint64T  bitMask )
{

    AMS_FUNC_ENTER (("\n"));

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0},0,0};
    ClAmsEntityT  *entity  = NULL;
    ClAmsAdminStateT lastState = CL_AMS_ADMIN_STATE_NONE;
    ClAmsAdminStateT newState = CL_AMS_ADMIN_STATE_NONE;
    ClBoolT  allAttr = CL_FALSE;


    AMS_CHECKPTR ( !entityConfig );

    memcpy (&entityRef.entity,entityConfig,sizeof(ClAmsEntityT));
    entityRef.ptr = NULL;
   
    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entityConfig->type],
                &entityRef) );
    
    /*
     * Set the config portion of the entity
     */

    entity = entityRef.ptr;

    AMS_CHECKPTR ( !entity );

    if ( bitMask == CL_AMS_CONFIG_ATTR_ALL )
    {
        allAttr = CL_TRUE;

    }

    switch ( entityConfig->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClAmsNodeConfigT  *nodeConfig = &node->config;
            ClAmsNodeConfigT  *newNodeConfig = (ClAmsNodeConfigT *)entityConfig;

            if ( (allAttr) || (bitMask&NODE_CONFIG_ADMIN_STATE))
            {
                lastState = nodeConfig->adminState;
                newState = newNodeConfig->adminState;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_ID))
            {
                nodeConfig->id = newNodeConfig->id;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_CLASS_TYPE))
            {
                nodeConfig->classType = newNodeConfig->classType;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_SUB_CLASS_TYPE))
            {
                memcpy (&nodeConfig->subClassType,&newNodeConfig->subClassType,
                        sizeof(ClNameT));
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_IS_SWAPPABLE))
            {
                nodeConfig->isSwappable = newNodeConfig->isSwappable;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_IS_RESTARTABLE))
            {
                nodeConfig->isRestartable= newNodeConfig->isRestartable;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_AUTO_REPAIR))
            {
                nodeConfig->autoRepair= newNodeConfig->autoRepair;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_IS_ASP_AWARE))
            {
                nodeConfig->isASPAware = newNodeConfig->isASPAware;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_SU_FAILOVER_DURATION))
            {
                nodeConfig->suFailoverDuration = newNodeConfig->suFailoverDuration;
            }

            if ( (allAttr) || (bitMask&NODE_CONFIG_SU_FAILOVER_COUNT_MAX))
            {
                nodeConfig->suFailoverCountMax = newNodeConfig->suFailoverCountMax;
            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            ClAmsSGConfigT  *sgConfig = &sg->config;
            ClAmsSGConfigT  *newSGConfig = (ClAmsSGConfigT *)entityConfig;

            if ( (allAttr) || (bitMask&SG_CONFIG_ADMIN_STATE))
            {
                lastState = sgConfig->adminState;
                newState = newSGConfig->adminState;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_REDUNDANCY_MODEL))
            {
                sgConfig->redundancyModel= newSGConfig->redundancyModel;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_LOADING_STRATEGY))
            {
                sgConfig->loadingStrategy = newSGConfig->loadingStrategy;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_FAILBACK_OPTION))
            {
                sgConfig->failbackOption = newSGConfig->failbackOption;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_AUTO_REPAIR))
            {
                sgConfig->autoRepair = newSGConfig->autoRepair;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_INSTANTIATE_DURATION))
            {
                sgConfig->instantiateDuration = newSGConfig->instantiateDuration;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_NUM_PREF_ACTIVE_SUS))
            {
                sgConfig->numPrefActiveSUs = newSGConfig->numPrefActiveSUs;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_NUM_PREF_STANDBY_SUS))
            {
                sgConfig->numPrefStandbySUs = newSGConfig->numPrefStandbySUs;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_NUM_PREF_INSERVICE_SUS))
            {
                sgConfig->numPrefInserviceSUs = newSGConfig->numPrefInserviceSUs;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_NUM_PREF_ASSIGNED_SUS))
            {
                sgConfig->numPrefAssignedSUs= newSGConfig->numPrefAssignedSUs;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_NUM_PREF_ACTIVE_SUS_PER_SI))
            {
                sgConfig->numPrefActiveSUsPerSI = newSGConfig->numPrefActiveSUsPerSI;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_MAX_ACTIVE_SIS_PER_SU))
            {
                sgConfig->maxActiveSIsPerSU = newSGConfig->maxActiveSIsPerSU;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_MAX_STANDBY_SIS_PER_SU))
            {
                sgConfig->maxStandbySIsPerSU = newSGConfig->maxStandbySIsPerSU;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_COMP_RESTART_DURATION))
            {
                sgConfig->compRestartDuration = newSGConfig->compRestartDuration;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_COMP_RESTART_COUNT_MAX))
            {
                sgConfig->compRestartCountMax = newSGConfig->compRestartCountMax;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_SU_RESTART_DURATION))
            {
                sgConfig->suRestartDuration = newSGConfig->suRestartDuration;
            }

            if ( (allAttr) || (bitMask&SG_CONFIG_SU_RESTART_COUNT_MAX))
            {
                sgConfig->suRestartCountMax= newSGConfig->suRestartCountMax;
            }

            if ( (allAttr) || (bitMask & SG_CONFIG_REDUCTION_PROCEDURE) )
            {
                sgConfig->reductionProcedure = newSGConfig->reductionProcedure;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_COLOCATION_ALLOWED))
            {
                sgConfig->isCollocationAllowed = newSGConfig->isCollocationAllowed;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_AUTO_ADJUST) )
            {
                sgConfig->autoAdjust = newSGConfig->autoAdjust;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_AUTO_ADJUST_PROBATION))
            {
                sgConfig->autoAdjustProbation = newSGConfig->autoAdjustProbation;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_ALPHA_FACTOR) )
            {
                sgConfig->alpha = newSGConfig->alpha;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_BETA_FACTOR) )
            {
                sgConfig->beta = newSGConfig->beta;
            }

            if( (allAttr) || (bitMask & SG_CONFIG_MAX_FAILOVERS) )
            {
                sgConfig->maxFailovers = newSGConfig->maxFailovers;
            }
            
            if( (allAttr) || (bitMask & SG_CONFIG_FAILOVER_DURATION) )
            {
                sgConfig->failoverDuration = newSGConfig->failoverDuration;
            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT  *su = (ClAmsSUT *) entity;
            ClAmsSUConfigT  *suConfig = &su->config;
            ClAmsSUConfigT  *newSUConfig = (ClAmsSUConfigT *)entityConfig;

            if ( (allAttr) || (bitMask&SU_CONFIG_ADMIN_STATE))
            {
                lastState = suConfig->adminState;
                newState = newSUConfig->adminState;
            }

            if ( (allAttr) || (bitMask&SU_CONFIG_RANK))
            {
                if(newSUConfig->rank != suConfig->rank)
                {
                    clAmsRankUpdate(entity, newSUConfig->rank);
                }
            }

            if ( (allAttr) || (bitMask&SU_CONFIG_NUM_COMPONENTS))
            {
                suConfig->numComponents= newSUConfig->numComponents;
            }

            if ( (allAttr) || (bitMask&SU_CONFIG_IS_PREINSTANTIABLE))
            {
                suConfig->isPreinstantiable= newSUConfig->isPreinstantiable;
            }

            if ( (allAttr) || (bitMask&SU_CONFIG_IS_RESTARTABLE))
            {
                suConfig->isRestartable = newSUConfig->isRestartable;
            }

            if ( (allAttr) || (bitMask&SU_CONFIG_IS_CONTAINER_SU))
            {
                suConfig->isContainerSU= newSUConfig->isContainerSU;
            }

            break;

        }
        case CL_AMS_ENTITY_TYPE_SI:
        {

            ClAmsSIT  *si = (ClAmsSIT *) entity;
            ClAmsSIConfigT  *siConfig = &si->config;
            ClAmsSIConfigT  *newSIConfig = (ClAmsSIConfigT *)entityConfig;

            if ( (allAttr) || (bitMask&SI_CONFIG_ADMIN_STATE))
            {
                lastState = siConfig->adminState;
                newState = newSIConfig->adminState;
            }

            if ( (allAttr) || (bitMask&SI_CONFIG_RANK))
            {
                if(siConfig->rank != newSIConfig->rank)
                {
                    clAmsRankUpdate(entity, newSIConfig->rank);
                }
            }

            if ( (allAttr) || (bitMask&SI_CONFIG_NUM_CSIS))
            {
                siConfig->numCSIs = newSIConfig->numCSIs;
            }

            if ( (allAttr) || (bitMask&SI_CONFIG_NUM_STANDBY_ASSIGNMENTS))
            {
                siConfig->numStandbyAssignments = newSIConfig->numStandbyAssignments;
            }
            
            if( (allAttr) || (bitMask & SI_CONFIG_STANDBY_ASSIGNMENT_ORDER))
            {
                siConfig->standbyAssignmentOrder = newSIConfig->standbyAssignmentOrder;
            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT  *comp = (ClAmsCompT *) entity;
            ClAmsCompConfigT  *compConfig = &comp->config;
            ClAmsCompConfigT  *newCompConfig = (ClAmsCompConfigT *)entityConfig;
            if ( (allAttr) || (bitMask&COMP_CONFIG_SUPPORTED_CSI_TYPE) )
            {
                ClInt32T i;
                ClNameT *pSupportedCSITypes = NULL;
                ClUint32T numSupportedCSITypes = 0;
 
               if(!newCompConfig->numSupportedCSITypes
                   || 
                   !newCompConfig->pSupportedCSITypes)
                {
                    if(newCompConfig->pSupportedCSITypes)
                        clAmsFreeMemory(newCompConfig->pSupportedCSITypes);
                    rc = CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
                    AMS_LOG(CL_DEBUG_ERROR,
                            ("Comp config set: Invalid supported csitype: numTypes %d",newCompConfig->numSupportedCSITypes));
                    goto exitfn;
                }
                /*
                 * Make a copy first as we skip invalid csi types
                 */
                pSupportedCSITypes = clHeapCalloc(newCompConfig->numSupportedCSITypes,
                                                  sizeof(*pSupportedCSITypes));
                CL_ASSERT(pSupportedCSITypes != NULL);

                /*
                 * Verify if the supported CSI types are valid
                 */
                for(i = 0; i < newCompConfig->numSupportedCSITypes; ++i)
                {
                    ClAmsEntityRefT entityRef = {{0}};
                    ClNameT *pCSIType = newCompConfig->pSupportedCSITypes+i;
                    pCSIType->length = strlen(pCSIType->value)+1;
                    entityRef.entity.type = CL_AMS_ENTITY_TYPE_CSI;
                    memcpy(&entityRef.entity.name, pCSIType, sizeof(entityRef.entity.name));

                    if(clAmsFindCSIType(NULL, pCSIType) != CL_OK)
                    {
                        clLogError("CCB", "SET", "Supported CSI type [%s] is invalid."
                                   "Skipping this csitype",
                                   pCSIType->value);
                        continue;
                    }
                    memcpy(pSupportedCSITypes+numSupportedCSITypes, pCSIType,
                           sizeof(*pSupportedCSITypes));
                    ++numSupportedCSITypes;
                }

                clAmsFreeMemory(newCompConfig->pSupportedCSITypes);

                if(!numSupportedCSITypes)
                {
                    clAmsFreeMemory(pSupportedCSITypes);
                    clLogError("CCB", "SET", "No valid supported csi types found.");
                    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
                    goto exitfn;
                }
                
                if(compConfig->pSupportedCSITypes)
                    clAmsFreeMemory(compConfig->pSupportedCSITypes);

                compConfig->numSupportedCSITypes =  numSupportedCSITypes;
                compConfig->pSupportedCSITypes = pSupportedCSITypes;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_PROXY_CSI_TYPE) )
            {
                memcpy (&compConfig->proxyCSIType,
                        &newCompConfig->proxyCSIType, sizeof(ClNameT) );
                if(compConfig->proxyCSIType.length == strlen(compConfig->proxyCSIType.value))
                    ++compConfig->proxyCSIType.length;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_CAPABILITY_MODEL) )
            {
                compConfig->capabilityModel = newCompConfig->capabilityModel;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_PROPERTY) )
            {
                compConfig->property = newCompConfig->property;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_IS_RESTARTABLE) )
            {
                compConfig->isRestartable = newCompConfig->isRestartable;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NODE_REBOOT_CLEANUP_FAIL) )
            {
                compConfig->nodeRebootCleanupFail= newCompConfig->nodeRebootCleanupFail;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_INSTANTIATE_LEVEL) )
            {
                ClUint32T newInstantiateLevel = 0;
                if(!(newInstantiateLevel = newCompConfig->instantiateLevel))
                    newInstantiateLevel = 1;
                if(compConfig->instantiateLevel != newInstantiateLevel)
                    clAmsRankUpdate(entity, newInstantiateLevel);
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_INSTANTIATE) )
            {
                compConfig->numMaxInstantiate = newCompConfig->numMaxInstantiate;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_INSTANTIATE_WITH_DELAY) )
            {
                compConfig->numMaxInstantiateWithDelay = newCompConfig->numMaxInstantiateWithDelay;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_TERMINATE ) )
            {
                compConfig->numMaxTerminate = newCompConfig->numMaxTerminate;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_AM_START) )
            {
                compConfig->numMaxAmStart = newCompConfig->numMaxAmStart;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_AM_STOP) )
            {
                compConfig->numMaxAmStop = newCompConfig->numMaxAmStop;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_ACTIVE_CSIS) )
            {
                compConfig->numMaxActiveCSIs = newCompConfig->numMaxActiveCSIs;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_NUM_MAX_STANDBY_CSIS) )
            {
                compConfig->numMaxStandbyCSIs = newCompConfig->numMaxStandbyCSIs;
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_TIMEOUTS) )
            {
                ClAmsCompTimerDurationsT oldConfig = {0};
                memcpy(&oldConfig, &compConfig->timeouts, sizeof(oldConfig));
                memcpy (&compConfig->timeouts, &newCompConfig->timeouts,
                        sizeof(ClAmsCompTimerDurationsT) );
                amsUpdateCompTimers(comp, &oldConfig);
            }

            if ( (allAttr) || (bitMask&COMP_CONFIG_RECOVERY_ON_TIMEOUT))
            {
                compConfig->recoveryOnTimeout= newCompConfig->recoveryOnTimeout;
            }

            if ( allAttr || (bitMask & COMP_CONFIG_PARENT_SU))
            {
                rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SU],
                                             &newCompConfig->parentSU);
                if(rc != CL_OK)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Comp parent SU [%.*s] not found\n",
                                             newCompConfig->parentSU.entity.name.length,
                                             newCompConfig->parentSU.entity.name.value));
                    goto exitfn;
                }
                
                memcpy(&compConfig->parentSU, &newCompConfig->parentSU,
                       sizeof(compConfig->parentSU));
            }

            if( allAttr || (bitMask & COMP_CONFIG_INSTANTIATE_COMMAND))
            {
                memcpy(compConfig->instantiateCommand, newCompConfig->instantiateCommand,
                       sizeof(compConfig->instantiateCommand));
            }
                 
            break;

        }
        case CL_AMS_ENTITY_TYPE_CSI:
        {

            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;
            ClAmsCSIConfigT  *csiConfig = &csi->config;
            ClAmsCSIConfigT  *newCSIConfig = (ClAmsCSIConfigT *)entityConfig;
            if ( (allAttr) || (bitMask&CSI_CONFIG_TYPE) )
            {
                newCSIConfig->type.length = strlen(newCSIConfig->type.value) + 1;
                memcpy (&csiConfig->type, &newCSIConfig->type, sizeof(ClNameT));
            }

            if ( (allAttr) || (bitMask&CSI_CONFIG_IS_PROXY_CSI) )
            {
                csiConfig->isProxyCSI = newCSIConfig->isProxyCSI;
            }

            if ( (allAttr) || (bitMask&CSI_CONFIG_RANK) )
            {
                csiConfig->rank= newCSIConfig->rank;
            }

            break;

        }

        default:
        {

            rc = CL_AMS_ERR_INVALID_ENTITY;
            AMS_LOG (CL_DEBUG_ERROR,("Invalid entity type [%d] \n", entityConfig->type));
            goto exitfn;

        }
    }

    if(lastState != newState)
    {
        rc = clAmsMgmtAdminStateChange(entity, lastState, newState);
    }

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT
clAmsCCBValidateOperationLocked(
                                CL_IN  ClPtrT  req,
                                CL_IN  ClAmsMgmtCCBOperationsT  opId )
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0};

    AMS_CHECKPTR (!req); 
    
    switch (opId)
    {

    case CL_AMS_MGMT_CCB_OPERATION_CREATE :
        {
            clAmsMgmtCCBEntityCreateRequestT *opData = 
                (clAmsMgmtCCBEntityCreateRequestT *) req;

            AMS_CHECK_ENTITY_TYPE_AND_EXIT (opData->entity.type);

            memcpy (&entityRef.entity,&opData->entity,sizeof(ClAmsEntityT));
            rc = clAmsEntityDbFindEntity( 
                                         &gAms.db.entityDb[opData->entity.type], &entityRef ); 

            if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
            {
                rc = CL_OK;
            }

            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_DELETE :
        {
            clAmsMgmtCCBEntityDeleteRequestT *opData = 
                (clAmsMgmtCCBEntityDeleteRequestT *) req;

            AMS_CHECK_ENTITY_TYPE_AND_EXIT (opData->entity.type);

            memcpy (&entityRef.entity,&opData->entity,sizeof(ClAmsEntityT));
            clLogNotice("ENTITY","VALIDATE","Delete validation for entity [%s]", opData->entity.name.value);
            AMS_CHECK_RC_ERROR( clAmsEntityDbFindEntity( 
                                                        &gAms.db.entityDb[opData->entity.type], &entityRef )); 

            AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(&opData->entity));

            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST:
        {
            clAmsMgmtCCBSetNodeSUListRequestT *opData = 
                (clAmsMgmtCCBSetNodeSUListRequestT *) req;

            if ( (opData->nodeName.type != CL_AMS_ENTITY_TYPE_NODE)
                 ||(opData->suName.type != CL_AMS_ENTITY_TYPE_SU))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->suName));
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST:
        {
            clAmsMgmtCCBSetSGSUListRequestT *opData = 
                (clAmsMgmtCCBSetSGSUListRequestT *) req;

            if ( (opData->sgName.type != CL_AMS_ENTITY_TYPE_SG)
                 ||(opData->suName.type != CL_AMS_ENTITY_TYPE_SU))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }
            
            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->suName));
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST:
        {
            clAmsMgmtCCBSetSGSIListRequestT *opData = 
                (clAmsMgmtCCBSetSGSIListRequestT *) req;

            if ( (opData->sgName.type != CL_AMS_ENTITY_TYPE_SG)
                 ||(opData->siName.type != CL_AMS_ENTITY_TYPE_SI))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->siName));
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST:
        {
            clAmsMgmtCCBSetSUCompListRequestT *opData = 
                (clAmsMgmtCCBSetSUCompListRequestT *) req;

            if ( (opData->suName.type != CL_AMS_ENTITY_TYPE_SU)
                 ||(opData->compName.type != CL_AMS_ENTITY_TYPE_COMP))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->compName));
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST:
        {
            clAmsMgmtCCBSetSISURankListRequestT *opData = 
                (clAmsMgmtCCBSetSISURankListRequestT *) req;

            if ( (opData->siName.type != CL_AMS_ENTITY_TYPE_SI)
                 ||(opData->suName.type != CL_AMS_ENTITY_TYPE_SU))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->suName));
            }

            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST:
        {
            clAmsMgmtCCBSetSISIDependencyRequestT *opData = 
                (clAmsMgmtCCBSetSISIDependencyRequestT *) req;
            ClAmsEntityRefT entityRef = {{0}};

            if ( (opData->siName.type != CL_AMS_ENTITY_TYPE_SI)
                 ||(opData->dependencySIName.type != 
                    CL_AMS_ENTITY_TYPE_SI))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if ( !memcmp(&opData->siName,&opData->dependencySIName,
                         sizeof(ClAmsEntityT)) )
            {
                AMS_LOG (CL_DEBUG_ERROR,("ALERT: Trying to add entity " \
                                         "[%s] in its dependency list\n",\
                                         opData->siName.name.value));
                rc = CL_AMS_RC (CL_ERR_INVALID_PARAMETER);
                goto exitfn;
            }
            
            memcpy(&entityRef.entity, &opData->siName, sizeof(entityRef.entity));
            if( (rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                              &entityRef) ) != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("SI [%s] not found in the db",
                                         opData->siName.name.value));
                goto exitfn;
            }

            memset(&entityRef, 0, sizeof(entityRef));
            memcpy(&entityRef.entity, &opData->dependencySIName, sizeof(entityRef.entity));

            if( ( rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                               &entityRef) ) != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("Dependency SI [%s] not found in the db",
                                         opData->dependencySIName.name.value));
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST)
            {
                if ( clAmsCheckIfRefExist(
                                          &opData->siName,
                                          &opData->dependencySIName,
                                          CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST)
                     == CL_TRUE )
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Dependency SI [%s] already present in the dependency list of SI [%s]",
                                             opData->dependencySIName.name.value,
                                             opData->siName.name.value));
                    rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
                    goto exitfn;
                }
                
                if( clAmsCheckIfRefExist(&opData->dependencySIName,
                                         &opData->siName,
                                         CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) 
                    == CL_TRUE )
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("SI [%s] already present in the dependents list of SI [%s]",
                                             opData->siName.name.value,
                                             opData->dependencySIName.name.value));
                    rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
                    goto exitfn;
                }
            }
            else
            {
                if(clAmsCheckIfRefExist(&opData->siName,
                                        &opData->dependencySIName,
                                        CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST) != CL_TRUE)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Dependency SI [%s] not present in the dependency list of SI [%s]",
                                             opData->dependencySIName.name.value,
                                             opData->siName.name.value));
                    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
                    goto exitfn;
                }

                if(clAmsCheckIfRefExist(&opData->dependencySIName,
                                        &opData->siName,
                                        CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) != CL_TRUE)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("SI [%s] not present in the dependents list of SI [%s]",
                                             opData->siName.name.value,
                                             opData->dependencySIName.name.value));
                    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
                    goto exitfn;
                }
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST:
        {
            clAmsMgmtCCBSetCSICSIDependencyRequestT *opData = 
                (clAmsMgmtCCBSetCSICSIDependencyRequestT *) req;
            ClAmsEntityRefT entityRef = {{0}};

            if ( (opData->csiName.type != CL_AMS_ENTITY_TYPE_CSI)
                 ||(opData->dependencyCSIName.type != 
                    CL_AMS_ENTITY_TYPE_CSI))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if ( !memcmp(&opData->csiName,&opData->dependencyCSIName,
                         sizeof(ClAmsEntityT)) )
            {
                AMS_LOG (CL_DEBUG_ERROR,("ALERT: Trying to add entity " \
                                         "[%s] in its dependency list\n",\
                                         opData->csiName.name.value));
                rc = CL_AMS_RC (CL_ERR_INVALID_PARAMETER);
                goto exitfn;
            }
            
            memcpy(&entityRef.entity, &opData->csiName, sizeof(entityRef.entity));
            if( (rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                                              &entityRef) ) != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("CSI [%s] not found in the db",
                                         opData->csiName.name.value));
                goto exitfn;
            }

            memset(&entityRef, 0, sizeof(entityRef));
            memcpy(&entityRef.entity, &opData->dependencyCSIName, sizeof(entityRef.entity));

            if( ( rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                                               &entityRef) ) != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("Dependency CSI [%s] not found in the db",
                                         opData->dependencyCSIName.name.value));
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST)
            {
                if ( clAmsCheckIfRefExist(
                                          &opData->csiName,
                                          &opData->dependencyCSIName,
                                          CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST)
                     == CL_TRUE )
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Dependency CSI [%s] already present in the dependency list of CSI [%s]",
                                             opData->dependencyCSIName.name.value,
                                             opData->csiName.name.value));
                    rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
                    goto exitfn;
                }
                
                if( clAmsCheckIfRefExist(&opData->dependencyCSIName,
                                         &opData->csiName,
                                         CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST) 
                    == CL_TRUE )
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("CSI [%s] already present in the dependents list of CSI [%s]",
                                             opData->csiName.name.value,
                                             opData->dependencyCSIName.name.value));
                    rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
                    goto exitfn;
                }
            }
            else
            {
                if(clAmsCheckIfRefExist(&opData->csiName,
                                        &opData->dependencyCSIName,
                                        CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST) != CL_TRUE)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Dependency CSI [%s] not present in the dependency list of CSI [%s]",
                                             opData->dependencyCSIName.name.value,
                                             opData->csiName.name.value));
                    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
                    goto exitfn;
                }

                if(clAmsCheckIfRefExist(&opData->dependencyCSIName,
                                        &opData->csiName,
                                        CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST) != CL_TRUE)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("CSI [%s] not present in the dependents list of CSI [%s]",
                                             opData->csiName.name.value,
                                             opData->dependencyCSIName.name.value));
                    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
                    goto exitfn;
                }
            }
            break;
        }


    case CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST :
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST :
        {
            clAmsMgmtCCBSetSICSIListRequestT *opData = 
                (clAmsMgmtCCBSetSICSIListRequestT *) req;

            if ( (opData->siName.type != CL_AMS_ENTITY_TYPE_SI)
                 ||(opData->csiName.type != CL_AMS_ENTITY_TYPE_CSI))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if(opId == CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST)
            {
                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState(
                                                              &opData->csiName));
            }
            break;

        }

    case CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG:
        {
            clAmsMgmtCCBEntitySetConfigRequestT *opData = 
                (clAmsMgmtCCBEntitySetConfigRequestT *) req;
            ClAmsEntityConfigT *entityConfig = opData->entityConfig;

            AMS_CHECK_ENTITY_TYPE_AND_EXIT (entityConfig->type);

            if(entityConfig->type == CL_AMS_ENTITY_TYPE_SU 
               ||
               entityConfig->type == CL_AMS_ENTITY_TYPE_SI
               ||
               entityConfig->type == CL_AMS_ENTITY_TYPE_COMP)
            {
                if( (opData->bitmask & SU_CONFIG_RANK)
                    ||
                    (opData->bitmask & SI_CONFIG_RANK)
                    ||
                    (opData->bitmask & COMP_CONFIG_INSTANTIATE_LEVEL)
                    )
                {
                    ClAmsEntityRefT entityRef = {{0}};
                    memcpy(&entityRef.entity, (ClAmsEntityT*)entityConfig, sizeof(entityRef.entity));
                    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityConfig->type],
                                                 &entityRef);
                    
                    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST
                       ||
                       CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)
                    {
                        rc = CL_OK;
                        goto exitfn;
                    }

                    if(entityConfig->type == CL_AMS_ENTITY_TYPE_SU)
                    {
                        ClAmsSUConfigT *newSUConfig = (ClAmsSUConfigT*)entityConfig;
                        ClAmsSUT *su = NULL;
                        su = (ClAmsSUT*)entityRef.ptr;
                        if(su->config.rank != newSUConfig->rank)
                        {
                            AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState((ClAmsEntityT*)entityConfig));
                        }
                    }
                    else if(entityConfig->type == CL_AMS_ENTITY_TYPE_SI)
                    {
                        ClAmsSIConfigT *newSIConfig = (ClAmsSIConfigT*)entityConfig;
                        ClAmsSIT *si = NULL;
                        si = (ClAmsSIT*)entityRef.ptr;
                        if(si->config.rank != newSIConfig->rank)
                        {
                            AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState((ClAmsEntityT*)entityConfig));
                        }
                    }
                    else
                    {
                        ClAmsCompConfigT *newCompConfig = (ClAmsCompConfigT*)entityConfig;
                        ClAmsCompT *comp = NULL;
                        ClUint32T newRank = 0;
                        comp = (ClAmsCompT*)entityRef.ptr;
                        if(! (newRank = newCompConfig->instantiateLevel) )
                            newRank = 1;
                        if(comp->config.instantiateLevel != newRank)
                        {
                            ClAmsSUT *parentSU = (ClAmsSUT*)comp->config.parentSU.ptr;
                            if(parentSU)
                            {
                                AMS_CHECK_RC_ERROR(clAmsCCBValidateAdminState((ClAmsEntityT*)parentSU));
                            }
                        }
                    }
                }
            }
            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP :
        {
            clAmsMgmtCCBCSISetNVPRequestT *opData = 
                (clAmsMgmtCCBCSISetNVPRequestT *) req;

            if ( opData->csiName.type != CL_AMS_ENTITY_TYPE_CSI )
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            break;
        }

    case CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP :
        {
            clAmsMgmtCCBCSIDeleteNVPRequestT *opData = 
                (clAmsMgmtCCBCSIDeleteNVPRequestT *) req;

            if ( opData->csiName.type != CL_AMS_ENTITY_TYPE_CSI )
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            memcpy (&entityRef.entity,&opData->csiName,
                    sizeof(ClAmsEntityT));
                
            AMS_CHECK_RC_ERROR( clAmsEntityDbFindEntity( 
                                                        &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI], 
                                                        &entityRef )); 

            break;
        }


    case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY:
    case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY:
        {
            clAmsMgmtCCBSetNodeDependencyRequestT *opData = 
                (clAmsMgmtCCBSetNodeDependencyRequestT *) req;

            if ( (opData->nodeName.type != CL_AMS_ENTITY_TYPE_NODE)
                 ||(opData->dependencyNodeName.type != 
                    CL_AMS_ENTITY_TYPE_NODE))
            {
                rc = CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);
                goto exitfn;
            }

            if ( !memcmp(&opData->nodeName,&opData->dependencyNodeName,
                         sizeof(ClAmsEntityT)) )
            {
                AMS_LOG (CL_DEBUG_ERROR,("ALERT: Trying to add entity" \
                                         "[%s] in its dependency list\n",
                                         opData->nodeName.name.value));
                rc = CL_AMS_RC (CL_ERR_INVALID_PARAMETER);
                goto exitfn;
            }

            break;
        }

    default: 
        { 
            AMS_LOG(CL_DEBUG_ERROR, ("invalid ccb validate operation id [%d]\n", opId));
            rc =  CL_AMS_RC (CL_AMS_ERR_INVALID_OPERATION);
            break;
        }
    } 
    
    exitfn:

    if(rc != CL_OK)
    {
        clLogError("CCB", "VALIDATE", "AMF validate operation returned [%#x] for operation [%d]",
                   rc, opId);
    }

    return rc;
}

ClRcT
clAmsCCBValidateOperation(
                          CL_IN  ClPtrT  req,
                          CL_IN  ClAmsMgmtCCBOperationsT  opId )
{
    ClRcT  rc = CL_OK;
    clOsalMutexLock(gAms.mutex);
    rc = clAmsCCBValidateOperationLocked(req, opId);
    clOsalMutexUnlock(gAms.mutex);
    return rc;
}

ClRcT
clAmsCCBValidateAdminState(
    CL_IN  ClAmsEntityT  *entity )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0};
    ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;

    AMS_CHECKPTR (!entity);

    memcpy (&entityRef.entity,entity, sizeof(ClAmsEntityT));

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],&entityRef); 

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        return CL_OK;
    }

    if ( rc != CL_OK )
    {
        return rc;
    }

    AMS_CHECKPTR (!entityRef.ptr);
    
    switch ( entity->type )
    {
        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeT  *node = ( ClAmsNodeT *)entityRef.ptr;
                
                if ( node->status.isClusterMember != CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
                {
                    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }
                break;
            }
        case CL_AMS_ENTITY_TYPE_SG:
            {
                ClAmsSGT  *sg = ( ClAmsSGT *)entityRef.ptr;
                clAmsPeSGComputeAdminState(sg, &adminState);

                if(adminState == CL_AMS_ADMIN_STATE_NONE)
                    adminState = sg->config.adminState;

                if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
                {
                    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }
                break;
            }
        case CL_AMS_ENTITY_TYPE_SU:
            {
                ClAmsSUT  *su = ( ClAmsSUT *)entityRef.ptr;
                clAmsPeSUComputeAdminState(su, &adminState);

                if(adminState == CL_AMS_ADMIN_STATE_NONE)
                    adminState = su->config.adminState;

                if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
                {
                    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }
                break;
            }
        case CL_AMS_ENTITY_TYPE_SI:
            {
                ClAmsSIT  *si = ( ClAmsSIT *)entityRef.ptr;
                
                if ( si->config.adminState != CL_AMS_ADMIN_STATE_LOCKED_A )
                {
                    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }
                break;
            }
        case CL_AMS_ENTITY_TYPE_COMP:
            {
                ClAmsCompT  *comp = ( ClAmsCompT *)entityRef.ptr;
                ClAmsSUT  *su = ( ClAmsSUT *)(comp->config.parentSU.ptr);
                if (su)
                {
                    clAmsPeSUComputeAdminState(su, &adminState);
                    if(adminState != CL_AMS_ADMIN_STATE_LOCKED_I)
                        return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }

                break;
            }
        case CL_AMS_ENTITY_TYPE_CSI:
            {
                ClAmsCSIT  *csi = ( ClAmsCSIT *)entityRef.ptr;
                ClAmsSIT  *si = ( ClAmsSIT *)(csi->config.parentSI.ptr);

                if ( si && ( si->config.adminState != CL_AMS_ADMIN_STATE_LOCKED_A ) )
                {
                    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_STATE);
                }

                break;
            }

        default:
            {
                break;
            }

    }

    return CL_OK;
}

/***************************************************************************
 * clAmsEntitySetStatus
 * -------------------
 * Set the status part of the entity.
 *
 ***************************************************************************/

ClRcT
clAmsEntitySetStatus(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  const ClAmsEntityT  *entityName,
        CL_IN  ClAmsEntityStatusT  *entityStatus)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef= {{0},0,0};
    ClAmsEntityT  *entity  = NULL;
    ClAmsEntityStatusT *status = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityDb || !entityName || !entityStatus);

    memcpy (&entityRef.entity,entityName,sizeof(ClAmsEntityT));
    entityRef.ptr = NULL;
   
    AMS_CALL( clAmsEntityDbFindEntity(
                entityDb,
                &entityRef) );
    
    /*
     * Set the status portion of the entity
     */

    entity = entityRef.ptr;

    AMS_CHECKPTR ( !entity )

    switch ( entityDb->type )
    {

        case CL_AMS_ENTITY_TYPE_ENTITY:
        {
            break;
        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClAmsNodeStatusT  *nodeStatus = &node->status;
            memcpy (nodeStatus, entityStatus, sizeof (ClAmsNodeStatusT));
            break;

        }

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            ClAmsSGStatusT  *sgStatus = &sg->status;
            ClAmsEntityListT  instantiableSUList = {0} ;
            ClAmsEntityListT  instantiatedSUList = {0};
            ClAmsEntityListT  inserviceSpareSUList = {0};
            ClAmsEntityListT  assignedSUList = {0};
            ClAmsEntityListT  faultySUList = {0};
            ClListHeadT failoverHistory = {NULL};

            /*
             * Save the lists
             */

            memcpy (&instantiableSUList, &sg->status.instantiableSUList,
                    sizeof(ClAmsEntityListT));
            memcpy (&instantiatedSUList, &sg->status.instantiatedSUList,
                    sizeof(ClAmsEntityListT));
            memcpy (&inserviceSpareSUList, &sg->status.inserviceSpareSUList,
                    sizeof(ClAmsEntityListT));
            memcpy (&assignedSUList, &sg->status.assignedSUList,
                    sizeof(ClAmsEntityListT));
            memcpy (&faultySUList, &sg->status.faultySUList,
                    sizeof(ClAmsEntityListT));
            memcpy(&failoverHistory, &sg->status.failoverHistory, sizeof(failoverHistory));
            memcpy (sgStatus, entityStatus, sizeof (ClAmsSGStatusT));

            memcpy (&sg->status.instantiableSUList,&instantiableSUList, 
                    sizeof(ClAmsEntityListT));
            memcpy (&sg->status.instantiatedSUList,&instantiatedSUList, 
                    sizeof(ClAmsEntityListT));
            memcpy (&sg->status.inserviceSpareSUList,&inserviceSpareSUList, 
                    sizeof(ClAmsEntityListT));
            memcpy (&sg->status.assignedSUList,&assignedSUList, 
                    sizeof(ClAmsEntityListT));
            memcpy (&sg->status.faultySUList,&faultySUList, 
                    sizeof(ClAmsEntityListT));
            memcpy(&sg->status.failoverHistory, &failoverHistory, sizeof(sg->status.failoverHistory));
            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT  *su = (ClAmsSUT *) entity;
            ClAmsSUStatusT  *suStatus = &su->status;
            ClAmsEntityListT  siList = {0};

            memcpy (&siList, &su->status.siList, sizeof(ClAmsEntityListT));
            memcpy (suStatus, entityStatus, sizeof (ClAmsSUStatusT));
            memcpy (&su->status.siList,&siList, sizeof(ClAmsEntityListT));

            break;

        }

        case CL_AMS_ENTITY_TYPE_SI:
        {

            ClAmsSIT  *si = (ClAmsSIT *) entity;
            ClAmsSIStatusT  *siStatus = &si->status;
            ClAmsEntityListT  suList = {0};

            memcpy (&suList, &si->status.suList, sizeof(ClAmsEntityListT));
            memcpy (siStatus, entityStatus, sizeof (ClAmsSIStatusT));
            memcpy (&si->status.suList,&suList,  sizeof(ClAmsEntityListT));

            break;

        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT  *comp = (ClAmsCompT *) entity;
            ClAmsCompStatusT  *compStatus = &comp->status;
            ClAmsEntityListT  csiList = {0};
            ClAmsSAClientCallbacksT clientCallbacks = {0};

            memcpy (&csiList, &comp->status.csiList, sizeof(ClAmsEntityListT));
            memcpy (&clientCallbacks, &comp->status.clientCallbacks, 
                    sizeof(ClAmsSAClientCallbacksT));
            memcpy (compStatus, entityStatus, sizeof (ClAmsCompStatusT));
            memcpy (&comp->status.csiList,&csiList, sizeof(ClAmsEntityListT));
            memcpy ( &comp->status.clientCallbacks, &clientCallbacks,
                    sizeof(ClAmsSAClientCallbacksT));

            break;

        }
        case CL_AMS_ENTITY_TYPE_CSI:
        {

            ClAmsCSIT  *csi = (ClAmsCSIT *) entity;
            ClAmsCSIStatusT  *csiStatus = &csi->status;
            ClAmsEntityListT  pgList = {0};
            ClCntHandleT  pgTrackList = csiStatus->pgTrackList;

            memcpy (&pgList, &csi->status.pgList, sizeof(ClAmsEntityListT));
            memcpy (csiStatus, entityStatus, sizeof (ClAmsCSIStatusT));
            memcpy (&csi->status.pgList,&pgList,  sizeof(ClAmsEntityListT));
            csi->status.pgTrackList = pgTrackList;

            break;

        }

        default:
        {

            rc = CL_AMS_ERR_INVALID_ENTITY;
            AMS_LOG (CL_DEBUG_ERROR,("Invalid entity type [%d] \n", entityDb->type));
            goto exitfn;

        }
    }

    status = clAmsEntityGetStatusStruct(entity);
    CL_LIST_HEAD_INIT(&status->opStack.opList);
    if(status->opStack.numOps)
    {
        clListMoveInit(&entityStatus->opStack.opList,
                       &status->opStack.opList);
    }
exitfn:

    return CL_AMS_RC (rc);

}

#ifdef POST_RC2

/*
 * clAmsEntityListGetListContents
 * -------------------
 * Get the contents of an Entity list 
 *
 * @param
 *   entityList                 - Pointer to an AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityListGetListContents(
        CL_IN  ClAmsEntityListT  *entityList,
        CL_IN  ClUint32T  *numNodes,
        CL_INOUT  ClPtrT  *nodeList )
{

    ClRcT  rc = CL_OK;
    ClCntNodeHandleT  nodeHandle = -1;
    ClPtrT  *buffer = NULL;
    ClAmsEntityRefT  *entityRef = NULL;
    ClUint32T  numEntities = 0;
    ClUint32T  index = 0;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityList || !numNodes || !nodeList );

    *numNodes = 0;
    *nodeList = NULL;

    numEntities = entityList->numEntities;

    if ( !numEntities )
    {
        return CL_OK;
    }

    buffer = clHeapAllocate ((numEntities)*sizeof(ClAmsEntityRefT *));

    AMS_CHECK_NO_MEMORY ( buffer );

    for ( entityRef = (ClAmsEntityRefT *)
            clAmsCntGetFirst( &entityList->list,&nodeHandle);
            entityRef != NULL; 
            entityRef = (ClAmsEntityRefT *)
            clAmsCntGetNext( &entityList->list,&nodeHandle) )
    {

        AMS_CHECKPTR_AND_EXIT (!entityRef);
        buffer[index++] =entityRef;

    }

    *numNodes = index;
    *nodeList = buffer;

    return CL_OK;

exitfn:

    clAmsFreeMemory ( buffer );
    return CL_AMS_RC (rc);

}

#endif

/*
 * clAmsCSISetNVP
 * -------------------
 * Set the CSI NVP list 
 *
 * @param
 *  entityDB                   - DB for the CSI entity 
 *  entity                     - CSI entity name
 *  nvp                        - Name value pair to be added to the CSI's NVP list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsCSISetNVP(
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityDbT  entityDb,
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityT  entity,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSINameValuePairT  nvp )

{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0,0} ;
    ClCntKeyHandleT  entityKeyHandle = 0;
    ClCntHandleT nvpListHandle = 0;
    ClCntNodeHandleT  nvpHandle = 0;
    ClUint32T  entityKey = 0;
    ClAmsCSINameValuePairT  *pNVP = NULL;
    ClUint32T  keyLength = 0;
    ClUint32T  numNodes = 0;


    AMS_FUNC_ENTER (("\n"));

    entityRef.ptr = NULL;

    memcpy (&entityRef.entity, &entity, sizeof (ClAmsEntityT));

    AMS_CALL (clAmsEntityDbFindEntity(
                &entityDb,
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    ClAmsCSIT  *csi = (ClAmsCSIT *)entityRef.ptr;

    AMS_CALL ( clCrc32bitCompute(
                ( ClUint8T *)nvp.paramName.value, 
                nvp.paramName.length,
                &entityKey,
                &keyLength));

    /*
     * Endianesss considerations. entityKey is 16bit but handle is either 32 or
     * 64 depending on flavor of the month. Want to make sure the key value is
     * not mutilated.
     */

    entityKeyHandle = (ClCntKeyHandleT) (ClWordT)entityKey;
    nvpListHandle = csi->config.nameValuePairList;

    /*
     * Check if the name already exists for the CSI
     */

    clCntKeySizeGet( nvpListHandle, entityKeyHandle, &numNodes);

    if  ( numNodes > 0 ) 
    { 

        for ( pNVP = (ClAmsCSINameValuePairT *) clAmsCntGetFirst( 
                    &nvpListHandle,&nvpHandle); pNVP != NULL; pNVP = 
                (ClAmsCSINameValuePairT *) clAmsCntGetNext( &nvpListHandle, 
                                                            &nvpHandle) )
        {
            AMS_CHECKPTR (!pNVP); 
            
            if ( !memcmp(&pNVP->paramName, &nvp.paramName, sizeof(ClNameT)))
            {
                memcpy ( &pNVP->paramValue, &nvp.paramValue, sizeof(ClNameT));
                return CL_OK;
            } 
        }

    }


    /*
     * This is a new NVP, Add it to the nvp list
     */

    pNVP = clHeapAllocate ( sizeof (ClAmsCSINameValuePairT) ); 

    AMS_CHECK_NO_MEMORY (pNVP);

    memcpy (pNVP, &nvp, sizeof (ClAmsCSINameValuePairT));

    AMS_CHECK_RC_ERROR ( clCntNodeAddAndNodeGet(
                nvpListHandle,
                entityKeyHandle, 
                (ClCntDataHandleT)pNVP,
                NULL,
                &entityRef.nodeHandle) );

    return CL_OK;

exitfn:

    clAmsFreeMemory(pNVP);
    return CL_AMS_RC (rc);

}

        
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


ClRcT
clAmsCSIGetNVP(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsCSINVPBufferT  *nvpList )

{
    ClAmsCSIT  *csi = NULL;
    ClAmsCSINameValuePairT  *nvp = NULL;
    ClAmsEntityRefT  entityRef = { {0},0 };
    ClCntNodeHandleT  nodeHandle = 0;
    ClUint32T  numNodes = 0;
    ClUint32T i = 0;
    ClRcT rc;
    

    AMS_CHECKPTR ( !entity || !nvpList );

    memcpy ( &entityRef.entity, entity, sizeof(ClAmsEntityT) );

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI], &entityRef);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) return rc;  /* Not a critical error, so just return */
    AMS_CALL (rc); /* Check for critical errors */
    

    csi = (ClAmsCSIT *)entityRef.ptr;
    AMS_CHECKPTR (!csi);

    /*
     * Go through the nvp list and put it in the nvpList buffer
     */

    AMS_CALL( clCntSizeGet( csi->config.nameValuePairList, &numNodes) );

    nvpList->count = numNodes;
    nvpList->nvp = clHeapAllocate (numNodes*sizeof(ClAmsCSINameValuePairT));

    for ( nvp = (ClAmsCSINameValuePairT *)
            clAmsCntGetFirst( &csi->config.nameValuePairList,&nodeHandle);
            nvp != NULL; 
            nvp = (ClAmsCSINameValuePairT *)
            clAmsCntGetNext( &csi->config.nameValuePairList,&nodeHandle) )
    {

        AMS_CHECKPTR (!nvp);
        memcpy (&nvpList->nvp[i++],nvp,sizeof(ClAmsCSINameValuePairT));

    }

    return CL_OK;

}

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


ClRcT
clAmsCSIDeleteNVP(
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityDbT  entityDb,
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityT  entity,
        // coverity[pass_by_value]
        CL_IN  ClAmsCSINameValuePairT  nvp )

{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0,0} ;
    ClCntKeyHandleT  entityKeyHandle = 0;
    ClCntHandleT nvpListHandle = 0;
    ClCntNodeHandleT  nvpHandle = 0;
    ClUint32T  entityKey = 0;
    ClAmsCSINameValuePairT  *pNVP = NULL;
    ClUint32T  keyLength = 0;
    ClUint32T  numNodes = 0;


    AMS_FUNC_ENTER (("\n"));

    entityRef.ptr = NULL;

    memcpy (&entityRef.entity, &entity, sizeof (ClAmsEntityT));

    AMS_CHECK_RC_ERROR (clAmsEntityDbFindEntity( &entityDb, &entityRef) );

    AMS_CHECKPTR_AND_EXIT ( !entityRef.ptr );

    ClAmsCSIT  *csi = (ClAmsCSIT *)entityRef.ptr;

    AMS_CHECK_RC_ERROR ( clCrc32bitCompute( ( ClUint8T *)nvp.paramName.value, 
                                            nvp.paramName.length, &entityKey, &keyLength));

    /*
     * Endianesss considerations. entityKey is 16bit but handle is either 32 or
     * 64 depending on flavor of the month. Want to make sure the key value is
     * not mutilated.
     */

    entityKeyHandle = (ClCntKeyHandleT) (ClWordT)entityKey;
    nvpListHandle = csi->config.nameValuePairList;

    /*
     * Check if the name exists for the CSI
     */

    clCntKeySizeGet( nvpListHandle, entityKeyHandle, &numNodes);

    if ( numNodes > 0 )
    {

        for ( pNVP = (ClAmsCSINameValuePairT *)
                clAmsCntGetFirst( &nvpListHandle,&nvpHandle); pNVP != NULL; 
                pNVP = (ClAmsCSINameValuePairT *) clAmsCntGetNext( 
                    &nvpListHandle, &nvpHandle) )
        {

            AMS_CHECKPTR (!pNVP);

            if ( !memcmp(&pNVP->paramName, &nvp.paramName, sizeof(ClNameT)))
            {
                AMS_CHECK_RC_ERROR (clCntNodeDelete(nvpListHandle,nvpHandle));
                return CL_OK;
            }

        }

    }

    rc = CL_AMS_RC( CL_ERR_NOT_EXIST ); 

exitfn:

    return rc;

}

static ClRcT clAmsGetEntityListAllCallback(ClAmsEntityT *pEntity, ClPtrT userArg)
{
    ClAmsEntityListT *entityList = userArg;
    ClAmsEntityRefT *pEntityRef = NULL;
    pEntityRef = clHeapCalloc(1, sizeof(*pEntityRef));
    if(!pEntityRef)
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    memcpy(&pEntityRef->entity, pEntity, sizeof(pEntityRef->entity));
    return clAmsEntityListAddEntityRef(entityList, pEntityRef, 0);
}

/*
 * clAmsGetEntityListAll -> fetches all configured entities in the cluster.
 */

ClRcT clAmsGetEntityListAll(ClAmsEntityListT *entityList, ClAmsEntityTypeT type)
{
    ClRcT rc = CL_OK;

    if(type > CL_AMS_ENTITY_TYPE_MAX)
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
    
    entityList->type = type;
    rc = clAmsEntityDbWalkExtended(&gAms.db.entityDb[type], 
                                   clAmsGetEntityListAllCallback, 
                                   (ClPtrT)entityList);
    return rc;
}

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


ClRcT
clAmsGetEntityList(
                   CL_IN  ClAmsEntityT  *entity,
                   CL_IN  ClAmsEntityListTypeT  entityListName,
                   CL_OUT  ClAmsEntityBufferT  *entityListBuffer)

{
    ClAmsEntityRefT  entityRef = { {0},0 };
    ClAmsEntityListT  *entityList = NULL; 
    ClAmsEntityListT list = {0};
    ClAmsEntityRefT *eRef = NULL;
    ClUint32T  i=0;
    ClBoolT terminateList = CL_FALSE;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR ( !entity || !entityListBuffer );
    
    if( entityListName < CL_AMS_ENTITY_LIST_ALL_START
        ||
        entityListName > CL_AMS_ENTITY_LIST_ALL_END)
    {
        memcpy ( &entityRef.entity, entity, sizeof(ClAmsEntityT) );

        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],&entityRef);
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) return rc;  /* Not a critical error, so just return */
        AMS_CALL (rc); /* Check for critical errors */

        AMS_CHECKPTR (!entityRef.ptr);
    }

    switch (entityListName)
    {
    case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *)entityRef.ptr;
            entityList = &node->config.nodeDependenciesList;
            break;
        }

    case CL_AMS_NODE_CONFIG_SU_LIST:
        {
            ClAmsNodeT  *node = (ClAmsNodeT *)entityRef.ptr;
            entityList = &node->config.suList;
            break;
        }

    case CL_AMS_SG_CONFIG_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->config.suList;
            break;
        }

    case CL_AMS_SG_CONFIG_SI_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->config.siList;
            break;
        }

    case CL_AMS_SU_CONFIG_COMP_LIST:
        {
            ClAmsSUT  *su = (ClAmsSUT *)entityRef.ptr;
            entityList = &su->config.compList;
            break;
        }

    case CL_AMS_SI_CONFIG_SU_RANK_LIST:
        {
            ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
            entityList = &si->config.suList;
            break;
        }

    case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
        {
            ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
            entityList = &si->config.siDependenciesList;
            break;
        }

    case CL_AMS_SI_CONFIG_CSI_LIST:
        {
            ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
            entityList = &si->config.csiList;
            break;
        }

    case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
        {
            ClAmsCSIT *csi = (ClAmsCSIT*)entityRef.ptr;
            entityList = &csi->config.csiDependenciesList;
            break;
        }

    case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->status.instantiableSUList;
            break;
        }

    case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->status.instantiatedSUList;
            break;
        }

    case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->status.inserviceSpareSUList;
            break;
        }

    case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->status.assignedSUList;
            break;
        }

    case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
        {
            ClAmsSGT  *sg = (ClAmsSGT *)entityRef.ptr;
            entityList = &sg->status.faultySUList;
            break;
        }

    case CL_AMS_SG_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_SG);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_SG);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("SG entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    case CL_AMS_SI_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_SI);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_SI);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("SI entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    case CL_AMS_CSI_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_CSI);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_CSI);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("CSI entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    case CL_AMS_NODE_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_NODE);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_NODE);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("Node entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    case CL_AMS_SU_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_SU);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_SU);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("SU entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    case CL_AMS_COMP_LIST:
        {
            entityList = &list;
            terminateList = CL_TRUE;
            rc = clAmsEntityListInstantiate(entityList, CL_AMS_ENTITY_TYPE_COMP);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("List instantiate returned [%#x]", rc));
                return rc;
            }

            rc = clAmsGetEntityListAll(entityList, CL_AMS_ENTITY_TYPE_COMP);
            if(rc != CL_OK)
            {
                AMS_LOG(CL_DEBUG_ERROR, ("COMP entity list returned [%#x]", rc));
                AMS_CALL ( clAmsEntityListTerminate(entityList) );
                return rc;
            }
            break;
        }

    default:
        {
            break;
        }

    }

    AMS_CHECKPTR (!entityList);

    entityListBuffer->count = entityList->numEntities;
    entityListBuffer->entity = clHeapAllocate ((entityListBuffer->count)*
                                               sizeof(ClAmsEntityT));

    for ( eRef = clAmsEntityListGetFirst(entityList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(entityList, eRef) )
    {
        memcpy( &entityListBuffer->entity[i++],&eRef->entity,
                sizeof(ClAmsEntityT) );
    }

    if(terminateList == CL_TRUE)
    {
        AMS_CALL ( clAmsEntityListTerminate(entityList) );
    }

    return CL_OK;

}


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


ClRcT
clAmsGetOLEntityList(
                     CL_IN  ClAmsEntityT  *entity,
                     CL_IN  ClAmsEntityListTypeT  entityListName,
                     CL_OUT  ClAmsEntityRefBufferT  *entityListBuffer)

{
    ClAmsEntityRefT  entityRef = { {0},0 };
    ClAmsEntityListT  *entityList = NULL; 
    ClAmsEntityRefT *eRef = NULL;
    ClUint32T  size = 0;
    ClUint32T  i=0;
    ClRcT rc;

    AMS_CHECKPTR ( !entity || !entityListBuffer );

    memcpy ( &entityRef.entity, entity, sizeof(ClAmsEntityT) );

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entity->type],&entityRef);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) return rc;  /* Not a critical error, so just return */
    AMS_CALL (rc); /* Check for critical errors */
    

    AMS_CHECKPTR (!entityRef.ptr);

    switch (entityListName)
    {


    case CL_AMS_SU_STATUS_SI_LIST:
        {
            ClAmsSUT  *su = (ClAmsSUT *)entityRef.ptr;
            entityList = &su->status.siList;
            size = sizeof (ClAmsSUSIRefT);
            break;
        }

    case CL_AMS_SU_STATUS_SI_EXTENDED_LIST:
        {
            ClAmsSUT  *su = (ClAmsSUT *)entityRef.ptr;
            entityList = &su->status.siList;
            size = sizeof (ClAmsSUSIExtendedRefT);
            break;
        }

    case CL_AMS_SI_STATUS_SU_LIST:
        {
            ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
            entityList = &si->status.suList;
            size = sizeof (ClAmsSISURefT);
            break;
        }

    case CL_AMS_SI_STATUS_SU_EXTENDED_LIST:
        {
            ClAmsSIT  *si = (ClAmsSIT *)entityRef.ptr;
            entityList = &si->status.suList;
            size = sizeof (ClAmsSISUExtendedRefT);
            break;
        }

    case CL_AMS_COMP_STATUS_CSI_LIST:
        {
            ClAmsCompT  *comp = (ClAmsCompT *)entityRef.ptr;
            entityList = &comp->status.csiList;
            size = sizeof (ClAmsCompCSIRefT);
            break;
        }

    default:
        {
            break;
        }

    }

    AMS_CHECKPTR (!entityList);

    entityListBuffer->count = entityList->numEntities;
    entityListBuffer->entityRef = clHeapAllocate ((entityListBuffer->count)*
                                                  size);

    for ( eRef = clAmsEntityListGetFirst(entityList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(entityList, eRef) )
    {
        if(entityListName == CL_AMS_SU_STATUS_SI_EXTENDED_LIST)
        {
            ClAmsSUSIExtendedRefT *targetRef = (ClAmsSUSIExtendedRefT*)((ClUint8T*)entityListBuffer->entityRef + i*size);
            ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT*)eRef;
            ClAmsSIT *si = (ClAmsSIT*)eRef->ptr;
            memcpy(&targetRef->entityRef, &suSIRef->entityRef, sizeof(targetRef->entityRef));
            targetRef->haState = suSIRef->haState;
            targetRef->numActiveCSIs = suSIRef->numActiveCSIs;
            targetRef->numStandbyCSIs = suSIRef->numStandbyCSIs;
            targetRef->numQuiescedCSIs = suSIRef->numQuiescedCSIs;
            targetRef->numQuiescingCSIs = suSIRef->numQuiescingCSIs;
            targetRef->numCSIs = si->config.numCSIs;
            targetRef->rank = suSIRef->rank;
            targetRef->pendingInvocations = clAmsInvocationsPendingForSI(si, (ClAmsSUT*)entityRef.ptr);
        }
        else if(entityListName == CL_AMS_SI_STATUS_SU_EXTENDED_LIST)
        {
            ClAmsSISUExtendedRefT *targetRef = (ClAmsSISUExtendedRefT*)((ClUint8T*)entityListBuffer->entityRef + i*size );
            ClAmsSISURefT *siSURef = (ClAmsSISURefT*)eRef;
            memcpy(&targetRef->entityRef, &siSURef->entityRef, sizeof(targetRef->entityRef));
            targetRef->rank = siSURef->rank;
            targetRef->haState = siSURef->haState;
            targetRef->pendingInvocations = clAmsInvocationsPendingForSU((ClAmsSUT*)eRef->ptr);
        }
        else
        {
            memcpy( (ClInt8T *)(entityListBuffer->entityRef) + i*size, eRef, size );
        }
        i++;
    }


    return CL_OK;

}


/*
 * clAmsCSIAddToPGTrackList
 * -------------------
 * Add to CSI PG Track list 
 *
 * @param
 *  entityDB                   - DB for the CSI entity 
 *  entity                     - CSI entity name
 *  pgTrackClient              - PG Client to be added to the CSI's PG track list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsCSIAddToPGTrackList(
        CL_IN  ClAmsEntityDbT  *entityDb,
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsCSIPGTrackClientT  *pgTrackClient,
        CL_INOUT  ClAmsPGNotificationBufferT  *notificationBuffer )

{

    ClAmsEntityRefT  entityRef = {{0},0,0};
    ClAmsCSIPGTrackClientT  *pgClient = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !pgTrackClient );

    entityRef.ptr = NULL;

    memcpy (&entityRef.entity, entity, sizeof (ClAmsEntityT));

    AMS_CALL ( clAmsEntityDbFindEntity(
                entityDb,
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    ClAmsCSIT     *csi = (ClAmsCSIT *)entityRef.ptr;

    if ( pgTrackClient->trackFlags&CL_AMS_PG_TRACK_CURRENT )
    {
        /*
         * Make the notification Buffer and send it to the client
         */
        ClAmsPGNotificationBufferT  *notificationBufferPtr = NULL;
        ClAmsPGChangeT  pgChange = CL_AMS_PG_NO_CHANGE;

        AMS_CALL ( clAmsCSIMarshalPGTrackNotificationBuffer (
                    &notificationBufferPtr,
                    csi,
                    NULL,
                    CL_AMS_PG_TRACK_CURRENT,
                    pgChange) );

        if ( notificationBuffer )
        {

            notificationBuffer->numItems = notificationBufferPtr->numItems;
            notificationBuffer->notification = notificationBufferPtr->notification;
            clAmsFreeMemory (notificationBufferPtr);

        }

        else
        {

            AMS_CALL ( _clAmsSAPGTrackDispatch(
                        pgTrackClient->address,
                        pgTrackClient->cpmHandle,
                        &csi->config.entity.name,
                        notificationBufferPtr) );

            clAmsFreeMemory (notificationBufferPtr->notification);
            clAmsFreeMemory (notificationBufferPtr);

        }

        if ( !( (pgTrackClient->trackFlags&CL_AMS_PG_TRACK_CHANGES) || 
                    (pgTrackClient->trackFlags&CL_AMS_PG_TRACK_CHANGES_ONLY)) )
        {
            return CL_OK;
        }

        pgTrackClient->trackFlags&=~CL_AMS_PG_TRACK_CURRENT;

    }

    /*
     * Verify that the entry does not exist alreday, if the entry exist verify
     * the flags are proper and modify the flags to take care of new  value
     */

    for ( pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetFirst( &csi->status.pgTrackList,&nodeHandle);
            pgClient != NULL; 
            pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetNext( &csi->status.pgTrackList,&nodeHandle) )
    {

        AMS_CHECKPTR (!pgClient);

        if ( !memcmp(&pgClient->address,&pgTrackClient->address,
                    sizeof (ClIocAddressT)) )
        { 

            if (( (pgTrackClient->trackFlags&CL_AMS_PG_TRACK_CHANGES_ONLY) &&
                    (pgClient->trackFlags&CL_AMS_PG_TRACK_CHANGES)) ||
                    ((pgTrackClient->trackFlags&CL_AMS_PG_TRACK_CHANGES) &&
                     (pgClient->trackFlags&CL_AMS_PG_TRACK_CHANGES_ONLY)) )
            { 

                AMS_LOG(CL_DEBUG_ERROR,("CL_AMS_PG_TRACK_CHANGES_ONLY and "
                            "CL_AMS_PG_TRACK_CHANGES are Mutually exclusive track flags \n"));
                return CL_AMS_RC (CL_ERR_INVALID_PARAMETER);

            }

            pgClient->trackFlags |= pgTrackClient->trackFlags;
            clAmsFreeMemory (pgTrackClient);

            return CL_OK;

        }
    
    }

    /*
     * This is a new client interested in PG track for the CSI
     */
    
    AMS_CALL ( clCntNodeAddAndNodeGet(
                csi->status.pgTrackList,
                0, 
                (ClCntDataHandleT)pgTrackClient,
                NULL,
                &entityRef.nodeHandle) );

    return CL_OK;

}

/*
 * clAmsCSIDeleteFromPGTrackList
 * -------------------
 * Add to CSI PG Track list 
 *
 * @param
 *  entityDB                   - DB for the CSI entity 
 *  entity                     - CSI entity name
 *  pgTrackClient              - PG Client to be deleted from the CSI's PG track list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsCSIDeleteFromPGTrackList(
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityDbT  entityDb,
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsCSIPGTrackClientT  *pgTrackClient )

{

    ClAmsEntityRefT  entityRef = {{0},0,0 };
    ClAmsCSIPGTrackClientT  *pgClient = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;


    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!pgTrackClient);

    entityRef.ptr = NULL;
    memcpy (&entityRef.entity, &entity, sizeof (ClAmsEntityT));

    AMS_CALL ( clAmsEntityDbFindEntity(
                &entityDb,
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    ClAmsCSIT     *csi = (ClAmsCSIT *)entityRef.ptr;

    /*
     * Find the actual element to be deleted from the list of all elements
     * with the same key. 
     */ 

    for ( pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetFirst( &csi->status.pgTrackList,&nodeHandle);
            pgClient != NULL; 
            pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetNext( &csi->status.pgTrackList,&nodeHandle) )
    {

        AMS_CHECKPTR (!pgClient);

        if ( !memcmp(&pgClient->address,&pgTrackClient->address,
                    sizeof (ClIocAddressT)) )
        { 

            AMS_CALL ( clCntNodeDelete(
                        csi->status.pgTrackList,
                        nodeHandle) ); 
            
            return CL_OK;

        } 

    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);

}


/*
 * clAmsCSIDispatchPGTrackResponse
 * -------------------
 *  Send response to client interested in change in PG 
 *
 * @param
 *  entityDB                   - DB for the CSI entity 
 *  entity                     - CSI entity name
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsCSIDispatchPGTrackResponse(
        ClAmsCSIT  *csi,
        ClAmsPGNotificationBufferT  *fullNotification,
        ClAmsPGNotificationBufferT  *changeNotification )
{

    AMS_FUNC_ENTER (("\n"));

    ClAmsCSIPGTrackClientT  *pgClient = NULL;
    ClCntNodeHandleT  nodeHandle  = NULL;


    AMS_CHECKPTR ( !fullNotification || !changeNotification );

    AMS_CHECKPTR ( !csi );


    for ( pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetFirst( &csi->status.pgTrackList,&nodeHandle);
            pgClient != NULL; 
            pgClient = (ClAmsCSIPGTrackClientT *)
            clAmsCntGetNext( &csi->status.pgTrackList,&nodeHandle) )
    {

        AMS_CHECKPTR (!pgClient);

        if ( pgClient->trackFlags&CL_AMS_PG_TRACK_CHANGES_ONLY )
        { 

            AMS_CALL ( _clAmsSAPGTrackDispatch(
                        pgClient->address,
                        pgClient->cpmHandle,
                        &csi->config.entity.name,
                        changeNotification) );

        }

        else
        {

            AMS_CALL ( _clAmsSAPGTrackDispatch(
                        pgClient->address,
                        pgClient->cpmHandle,
                        &csi->config.entity.name,
                        fullNotification) );

        }

    }

    return CL_OK;

}


/*
 * clAmsEntityListsCompare
 * -------------------
 * Compare the contents of two entity lists. If there is a common element return 
 * invalid entity. This function also matched the elements whose lists are compared
 * is not present in any of the two lists .
 *
 * @param
 *  entity                      - entity whose lists are compared
 *  dependentEntityList         - Pointer to dependent AMS entity list
 *  dependenciesEntityList      - Pointer to dependencies AMS entity list
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */

ClRcT
clAmsEntityCompareLists(
        // coverity[pass_by_value]
        CL_IN  ClAmsEntityT  entity,
        CL_IN  ClAmsEntityListT  *dependentList,
        CL_IN  ClAmsEntityListT  *dependenciesList )
{

    ClAmsEntityRefT  *dependentEntityRef = NULL;
    ClAmsEntityRefT  *dependenciesEntityRef = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !dependentList || !dependenciesList );

    for ( dependentEntityRef = clAmsEntityListGetFirst(dependentList);
            dependentEntityRef != (ClAmsEntityRefT *) NULL;
            dependentEntityRef = clAmsEntityListGetNext(dependentList, dependentEntityRef) )
    {

        for ( dependenciesEntityRef = clAmsEntityListGetFirst(dependenciesList);
                dependenciesEntityRef != (ClAmsEntityRefT *) NULL;
                dependenciesEntityRef = clAmsEntityListGetNext(dependenciesList,dependenciesEntityRef))
        {

             if ( !memcmp (dependentEntityRef->entity.name.value, 
                         dependenciesEntityRef->entity.name.value,
                         dependentEntityRef->entity.name.length) )
             { 

                 AMS_LOG (CL_DEBUG_ERROR,("\n entity :[%s] found in its dependentList \n",
                             dependenciesEntityRef->entity.name.value));
                 return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);

             }
        }
    }

    /*
     * Go through the other list to verify that the entity dosnt exist in its own list
     */

    for ( dependentEntityRef = clAmsEntityListGetFirst(dependentList);
            dependentEntityRef != (ClAmsEntityRefT *) NULL;
            dependentEntityRef = clAmsEntityListGetNext(dependentList, dependentEntityRef))
    {

         if ( !memcmp(dependentEntityRef->entity.name.value,
                     entity.name.value,dependentEntityRef->entity.name.length))
         { 

             AMS_LOG (CL_DEBUG_ERROR,("\n entity :[%s] found in its dependentList \n",
                     dependentEntityRef->entity.name.value));
             return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);

         } 
    }

    for ( dependenciesEntityRef = clAmsEntityListGetFirst(dependenciesList);
            dependenciesEntityRef != (ClAmsEntityRefT *) NULL;
            dependenciesEntityRef = clAmsEntityListGetNext(dependenciesList,dependenciesEntityRef))
    {

         if ( !memcmp(dependenciesEntityRef->entity.name.value,
                     entity.name.value,
                     dependenciesEntityRef->entity.name.length) )
         { 

             AMS_LOG (CL_DEBUG_ERROR,("\n entity :[%s] found in its dependenciesList \n",
                     dependenciesEntityRef->entity.name.value));
             return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY);

         } 
    }

    return CL_OK;

}

/*
 *clAmsAddToEntityList 
 * -------------------
 */

ClRcT 
clAmsAddGetEntityList(
                      ClAmsEntityT  *sourceEntity,
                      ClAmsEntityT  *targetEntity,
                      ClAmsEntityListTypeT  entityListName,
                      ClAmsEntityT **ppSourceRef,
                      ClAmsEntityT **ppTargetRef)
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  sourceEntityRef = {{0},0,0};
    ClAmsEntityRefT  *targetEntityRef = NULL;
    ClCntKeyHandleT  entityKey = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !sourceEntity || !targetEntity );

    if(ppSourceRef) 
        *ppSourceRef =  NULL;
    if(ppTargetRef)
        *ppTargetRef =  NULL;

    targetEntityRef = clHeapAllocate ( sizeof (ClAmsEntityRefT));

    AMS_CHECK_NO_MEMORY (targetEntityRef);

    memcpy (&sourceEntityRef.entity,sourceEntity,sizeof (ClAmsEntityT));
    memcpy (&targetEntityRef->entity,targetEntity,sizeof (ClAmsEntityT));

    sourceEntityRef.ptr = NULL;
    targetEntityRef->ptr = NULL;

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                                                 &gAms.db.entityDb[sourceEntity->type],
                                                 &sourceEntityRef) );

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                                                 &gAms.db.entityDb[targetEntity->type],
                                                 targetEntityRef) );

    /*
     * Add the targetEntityRef in the sourceEntityRef list
     */

    AMS_CHECKPTR_AND_EXIT ( !sourceEntityRef.ptr );

    switch(entityListName)
    {

    case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
        {
                
            ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

            AMS_CHECK_RC_ERROR ( clAmsEntityRefGetKey(
                                                      &targetEntityRef->entity,
                                                      0,
                                                      &entityKey,
                                                      CL_FALSE) ); 

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                                                             &node->config.nodeDependentsList,
                                                             targetEntityRef,
                                                             entityKey) );

            break;

        }

    case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
        {
                
            ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

            AMS_CHECK_RC_ERROR ( clAmsEntityRefGetKey(
                                                      &targetEntityRef->entity,
                                                      0,
                                                      &entityKey,
                                                      CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                                                             &node->config.nodeDependenciesList,
                                                             targetEntityRef,
                                                             entityKey) );

            break;

        }

    case CL_AMS_NODE_CONFIG_SU_LIST:
        {
               
            ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;
            ClAmsSUT  *su =   (ClAmsSUT *)targetEntityRef->ptr;

            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }


            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &node->config.suList,
                                                            targetEntityRef,
                                                            entityKey) );
                
            break;

        }

    case CL_AMS_SG_CONFIG_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;

            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR (clAmsEntityListAddEntityRef(
                                                            &sg->config.suList,
                                                            targetEntityRef,
                                                            entityKey) ) ;

            break;

        }

    case CL_AMS_SG_CONFIG_SI_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSIT  *si = (ClAmsSIT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     si->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                                                             &sg->config.siList,
                                                             targetEntityRef,
                                                             entityKey) ) ;

            break;

        }

    case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR ( clAmsEntityListAddEntityRef(
                                                             &sg->status.instantiableSUList,
                                                             targetEntityRef,
                                                             entityKey) ) ;

            break;

        }

    case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &sg->status.instantiatedSUList,
                                                            targetEntityRef,
                                                            entityKey) ) ;
            break;

        }

    case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &sg->status.inserviceSpareSUList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &sg->status.assignedSUList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }


            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &sg->status.faultySUList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SU_CONFIG_COMP_LIST:
        {

            ClAmsSUT  *su = (ClAmsSUT *) sourceEntityRef.ptr;
            ClAmsCompT  *comp = (ClAmsCompT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     comp->config.instantiateLevel,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &su->config.compList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SI_CONFIG_SU_RANK_LIST: 
        {

            ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
            ClAmsSUT  *su = (ClAmsSUT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     su->config.rank,
                                                     &entityKey,
                                                     CL_TRUE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &si->config.suList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
        {

            ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
            ClAmsSIT  *targetSI = (ClAmsSIT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     targetSI->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &si->config.siDependentsList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
        {

            ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
            ClAmsSIT  *targetSI= (ClAmsSIT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     targetSI->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &si->config.siDependenciesList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_SI_CONFIG_CSI_LIST:
        {

            ClAmsSIT  *si  = (ClAmsSIT *) sourceEntityRef.ptr;
            ClAmsCSIT  *csi = (ClAmsCSIT *)targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     csi->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &si->config.csiList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
        {

            ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
            ClAmsCSIT  *targetCSI = (ClAmsCSIT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     targetCSI->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &csi->config.csiDependentsList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
        {

            ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
            ClAmsCSIT  *targetCSI= (ClAmsCSIT *) targetEntityRef->ptr;
               
            AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                                                     &targetEntityRef->entity,
                                                     targetCSI->config.rank,
                                                     &entityKey,
                                                     CL_FALSE) );

            if ( clAmsCheckIfRefExist(
                                      sourceEntity,
                                      targetEntity,
                                      entityListName ) 
                 == CL_TRUE )
            {
                rc = CL_ERR_ALREADY_EXIST;
                goto exitfn;
            }

            AMS_CHECK_RC_ERROR( clAmsEntityListAddEntityRef(
                                                            &csi->config.csiDependenciesList,
                                                            targetEntityRef,
                                                            entityKey) );

            break;

        }

    default:
        {

            AMS_LOG (CL_DEBUG_ERROR,("invalid entity list[%d] \n",entityListName));
            rc =  CL_AMS_ERR_INVALID_ENTITY_LIST;
            goto exitfn;
        }

    }

    if(ppSourceRef)
        *ppSourceRef = sourceEntityRef.ptr;
    if(ppTargetRef)
        *ppTargetRef = targetEntityRef->ptr;

    return CL_OK;

    exitfn: 

    clAmsFreeMemory(targetEntityRef);
    return CL_AMS_RC (rc);

}

/*
 *clAmsAddToEntityList 
 * -------------------
 */

ClRcT 
clAmsAddToEntityList(
                      ClAmsEntityT  *sourceEntity,
                      ClAmsEntityT  *targetEntity,
                      ClAmsEntityListTypeT  entityListName)
{
    return clAmsAddGetEntityList(sourceEntity, targetEntity, entityListName, NULL, NULL);
}

ClRcT 
clAmsDeleteFromEntityList(
                       ClAmsEntityT  *sourceEntity,
                       ClAmsEntityT  *targetEntity,
                       ClAmsEntityListTypeT  entityListName )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  sourceEntityRef = {{0},0,0};
    ClAmsEntityRefT  entityRef = {{0}};
    ClAmsEntityRefT  *targetEntityRef = &entityRef;
    ClCntKeyHandleT  entityKey = NULL;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !sourceEntity || !targetEntity );

    memcpy (&sourceEntityRef.entity,sourceEntity,sizeof (ClAmsEntityT));
    memcpy (&targetEntityRef->entity,targetEntity,sizeof (ClAmsEntityT));

    sourceEntityRef.ptr = NULL;
    targetEntityRef->ptr = NULL;

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[sourceEntity->type],
                &sourceEntityRef) );

    AMS_CHECK_RC_ERROR ( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[targetEntity->type],
                targetEntityRef) );

    /*
     * Delete the targetEntityRef from the sourceEntityRef list
     */

    AMS_CHECKPTR_AND_EXIT ( !sourceEntityRef.ptr );

    switch(entityListName)
    {

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

                AMS_CHECK_RC_ERROR ( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            0,
                            &entityKey,
                            CL_FALSE) ); 

                AMS_CHECK_RC_ERROR ( clAmsEntityListDeleteEntityRef(
                            &node->config.nodeDependentsList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

                AMS_CHECK_RC_ERROR ( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            0,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR ( clAmsEntityListDeleteEntityRef(
                            &node->config.nodeDependenciesList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

        case CL_AMS_NODE_CONFIG_SU_LIST:
            {
               
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;
                ClAmsSUT  *su =   (ClAmsSUT *)targetEntityRef->ptr;

                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &node->config.suList,
                            targetEntityRef,
                            entityKey) );
                
                break;

            }

        case CL_AMS_SG_CONFIG_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;

                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR (clAmsEntityListDeleteEntityRef(
                            &sg->config.suList,
                            targetEntityRef,
                            entityKey) ) ;

                break;

            }

        case CL_AMS_SG_CONFIG_SI_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSIT  *si = (ClAmsSIT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            si->config.rank,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR ( clAmsEntityListDeleteEntityRef(
                            &sg->config.siList,
                            targetEntityRef,
                            entityKey) ) ;

                break;

             }

        case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );
 
                AMS_CHECK_RC_ERROR ( clAmsEntityListDeleteEntityRef(
                            &sg->status.instantiableSUList,
                            targetEntityRef,
                            entityKey) ) ;

                break;

            }

        case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );


                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &sg->status.instantiatedSUList,
                            targetEntityRef,
                            entityKey) ) ;
                break;

            }

        case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &sg->status.inserviceSpareSUList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

        case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &sg->status.assignedSUList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

        case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &sg->status.faultySUList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

        case CL_AMS_SU_CONFIG_COMP_LIST:
            {

                ClAmsSUT  *su = (ClAmsSUT *) sourceEntityRef.ptr;
                ClAmsCompT  *comp = (ClAmsCompT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            comp->config.instantiateLevel,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &su->config.compList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_SI_CONFIG_SU_RANK_LIST: 
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            su->config.rank,
                            &entityKey,
                            CL_TRUE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &si->config.suList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSIT  *targetSI = (ClAmsSIT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            targetSI->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &si->config.siDependentsList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSIT  *targetSI= (ClAmsSIT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            targetSI->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &si->config.siDependenciesList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_SI_CONFIG_CSI_LIST:
            {

                ClAmsSIT  *si  = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *csi = (ClAmsCSIT *)targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            csi->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &si->config.csiList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
            {

                ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *targetCSI = (ClAmsCSIT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            targetCSI->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &csi->config.csiDependentsList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
            {

                ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *targetCSI= (ClAmsCSIT *) targetEntityRef->ptr;
               
                AMS_CHECK_RC_ERROR( clAmsEntityRefGetKey(
                            &targetEntityRef->entity,
                            targetCSI->config.rank,
                            &entityKey,
                            CL_FALSE) );

                AMS_CHECK_RC_ERROR( clAmsEntityListDeleteEntityRef(
                            &csi->config.csiDependenciesList,
                            targetEntityRef,
                            entityKey) );

                break;

            }

            default:
            {

                AMS_LOG (CL_DEBUG_ERROR,("invalid entity list[%d] \n",entityListName));
                rc =  CL_AMS_ERR_INVALID_ENTITY_LIST;
                goto exitfn;
            }

        }

        return CL_OK;

exitfn: 

        return CL_AMS_RC (rc);
}


/*
 *clAmsCheckIfRefExist
 * -------------------
 */

ClBoolT 
clAmsCheckIfRefExist(
        ClAmsEntityT  *sourceEntity,
        ClAmsEntityT  *targetEntity,
        ClAmsEntityListTypeT  entityListName )
{

    ClRcT  rc1 = CL_OK;
    ClRcT  rc2 = CL_OK;
    ClAmsEntityRefT  sourceEntityRef = {{0},0,0};
    ClAmsEntityRefT  targetEntityRef = {{0},0,0};
    ClAmsEntityRefT  *foundRef = NULL;
    ClCntKeyHandleT  entityKey = NULL;
    ClAmsEntityListT  *entityList = NULL;

    AMS_FUNC_ENTER (("\n"));

    if ( !sourceEntity || !targetEntity )
    {
        return CL_FALSE;
    }


    memcpy (&sourceEntityRef.entity,sourceEntity,sizeof (ClAmsEntityT));
    memcpy (&targetEntityRef.entity,targetEntity,sizeof (ClAmsEntityT));

    sourceEntityRef.ptr = NULL;
    targetEntityRef.ptr = NULL;

    rc1 = clAmsEntityDbFindEntity(
            &gAms.db.entityDb[sourceEntity->type],
            &sourceEntityRef) ;

    rc2 = clAmsEntityDbFindEntity(
                &gAms.db.entityDb[targetEntity->type],
                &targetEntityRef);

    if ( (rc1 != CL_OK) || (rc2 != CL_OK) )
    {
        return CL_FALSE;
    }

    /*
     * Add the targetEntityRef in the sourceEntityRef list
     */

    if ( !sourceEntityRef.ptr )
    {
        return CL_FALSE;
    }

    switch(entityListName)
    {

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

                clAmsEntityRefGetKey( &targetEntityRef.entity, 0, &entityKey,
                            CL_FALSE); 
                
                entityList = &node->config.nodeDependentsList;
                break;

            }

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;

                clAmsEntityRefGetKey( &targetEntityRef.entity, 0, &entityKey,
                            CL_FALSE);
                entityList = &node->config.nodeDependenciesList;
                break;

            }

        case CL_AMS_NODE_CONFIG_SU_LIST:
            {
               
                ClAmsNodeT  *node = (ClAmsNodeT *) sourceEntityRef.ptr;
                ClAmsSUT  *su =   (ClAmsSUT *)targetEntityRef.ptr;

                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                            &entityKey, CL_FALSE );

                entityList = &node->config.suList;
                break;

            }

        case CL_AMS_SG_CONFIG_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;

                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                            &entityKey, CL_TRUE);

                entityList = &sg->config.suList;
                break;

            }

        case CL_AMS_SG_CONFIG_SI_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSIT  *si = (ClAmsSIT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, si->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &sg->config.siList;
                break;

             }

        case CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &sg->status.instantiableSUList;
                break;

            }

        case CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &sg->status.instantiatedSUList;
                break;

            }

        case CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &sg->status.inserviceSpareSUList;
                break;

            }

        case CL_AMS_SG_STATUS_ASSIGNED_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &sg->status.assignedSUList;
                break;

            }

        case CL_AMS_SG_STATUS_FAULTY_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_FALSE );

                entityList = &sg->status.faultySUList;
                break;

            }

        case CL_AMS_SU_CONFIG_COMP_LIST:
            {

                ClAmsSUT  *su = (ClAmsSUT *) sourceEntityRef.ptr;
                ClAmsCompT  *comp = (ClAmsCompT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, 
                        comp->config.instantiateLevel, &entityKey, CL_TRUE );

                entityList = &su->config.compList;
                break;

            }

            case CL_AMS_SI_CONFIG_SU_RANK_LIST: 
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSUT  *su = (ClAmsSUT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, su->config.rank,
                        &entityKey, CL_TRUE );

                entityList = &si->config.suList;

                break;

            }

            case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSIT  *targetSI = (ClAmsSIT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, 
                        targetSI->config.rank, &entityKey, CL_FALSE );

                entityList = &si->config.siDependentsList;
                break;

            }

            case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
            {

                ClAmsSIT  *si = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsSIT  *targetSI= (ClAmsSIT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, 
                        targetSI->config.rank, &entityKey, CL_FALSE );

                entityList = &si->config.siDependenciesList;
                break;

            }

            case CL_AMS_SI_CONFIG_CSI_LIST:
            {

                ClAmsSIT  *si  = (ClAmsSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *csi = (ClAmsCSIT *)targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, csi->config.rank
                        , &entityKey, CL_FALSE );

                entityList = &si->config.csiList;
                break;

            }

            case CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST:
            {

                ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *targetCSI = (ClAmsCSIT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, 
                        targetCSI->config.rank, &entityKey, CL_FALSE );

                entityList = &csi->config.csiDependentsList;
                break;

            }

            case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
            {

                ClAmsCSIT  *csi = (ClAmsCSIT *) sourceEntityRef.ptr;
                ClAmsCSIT  *targetCSI= (ClAmsCSIT *) targetEntityRef.ptr;
               
                clAmsEntityRefGetKey( &targetEntityRef.entity, 
                        targetCSI->config.rank, &entityKey, CL_FALSE );

                entityList = &csi->config.csiDependenciesList;
                break;

            }

            default:
            {

                AMS_LOG (CL_DEBUG_ERROR,("invalid entity list[%d] \n", \
                            entityListName));
                return CL_FALSE;
            }

        } 
    
    if ( clAmsEntityListFindEntityRef(entityList, &targetEntityRef, entityKey, 
                &foundRef) == CL_OK )
        {
            return CL_TRUE;
        } 
    
    return CL_FALSE;

}



#ifdef POST_RC2

ClRcT 
clAmsGetEntityListContents(
        ClAmsEntityT  *entity,
        ClAmsEntityListTypeT  entityListName, 
        clAmsMgmtEntityGetEntityListResponseT  *res )
{

    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entity || !res );

    memcpy ( &entityRef.entity, entity, sizeof (ClAmsEntityT) );

    /*
     * 1. Find the entity in the database
     * 2. Find the corresponding list 
     * 3. Find the number of elements in the list
     * 4. Allocate memory for those members
     * 5. Copy the elements in the response buffer
     * 6. Update the response with number of total nodes
     */ 
    
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[entityRef.entity.type],&entityRef);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) return rc;  /* Not a critical error, so just return */
    AMS_CALL (rc); /* Check for critical errors */
    
    
    AMS_CHECKPTR ( !entityRef.ptr );

    switch(entityListName)
    {

        case CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &node->config.nodeDependentsList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }
        case CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST:
            {
                
                ClAmsNodeT  *node = (ClAmsNodeT *) entityRef.ptr;

                AMS_CALL ( clAmsEntityListGetListContents(
                            &node->config.nodeDependenciesList,
                            &res->numNodes,
                            &res->nodeList) );

                break;
                
            }
        case CL_AMS_NODE_CONFIG_SU_LIST:
            {
               
                ClAmsNodeT  *node = (ClAmsNodeT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &node->config.suList,
                            &res->numNodes,
                            &res->nodeList) );

                break;
               
            }
            
        case CL_AMS_SG_CONFIG_SU_LIST:
            {

                ClAmsSGT  *sg = (ClAmsSGT *) entityRef.ptr;
               
                AMS_CALL( clAmsEntityListGetListContents(
                            &sg->config.suList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }

        case CL_AMS_SG_CONFIG_SI_LIST:
            {
               
                ClAmsSGT  *sg = (ClAmsSGT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &sg->config.siList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }

        case CL_AMS_SU_CONFIG_COMP_LIST:
            {
               
                ClAmsSUT  *su = (ClAmsSUT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &su->config.compList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }

        case CL_AMS_SI_CONFIG_SU_RANK_LIST: 
            {

                ClAmsSIT  *si = (ClAmsSIT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &si->config.suList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }

        case CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST:
            {
               
                ClAmsSIT  *si = (ClAmsSIT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &si->config.siDependentsList,
                            &res->numNodes,
                            &res->nodeList) );
                
                break;

            }

        case CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST:
            {
            
                ClAmsSIT  *si = (ClAmsSIT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &si->config.siDependenciesList,
                            &res->numNodes,
                            &res->nodeList) );

                break;
               
            }

        case CL_AMS_SI_CONFIG_CSI_LIST:
            {
            
                ClAmsSIT  *si = (ClAmsSIT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &si->config.csiList,
                            &res->numNodes,
                            &res->nodeList) );

                break;
               
            }

        case CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST:
            {
            
                ClAmsCSIT  *csi = (ClAmsCSIT *) entityRef.ptr;

                AMS_CALL( clAmsEntityListGetListContents(
                            &csi->config.csiDependenciesList,
                            &res->numNodes,
                            &res->nodeList) );

                break;
               
            }

        case CL_AMS_SU_STATUS_SI_LIST:
            {

                ClAmsSUT  *su = (ClAmsSUT *) entityRef.ptr;
               
                AMS_CALL( clAmsEntityListGetListContents(
                            &su->status.siList,
                            &res->numNodes,
                            &res->nodeList) );

                break;

            }

        case CL_AMS_COMP_STATUS_CSI_LIST:
           {

               ClAmsCompT  *comp = (ClAmsCompT *) entityRef.ptr; 
               
               AMS_CALL( clAmsEntityListGetListContents(
                           &comp->status.csiList,
                           &res->numNodes,
                           &res->nodeList));

                break;

            }

        default:
           { 
               AMS_LOG (CL_DEBUG_ERROR,("entity reference to invalid list \n"));
               rc =  CL_AMS_ERR_INVALID_ENTITY_LIST;
               goto exitfn;
           } 
    
    }

    return CL_OK;

exitfn:

    /*
     * Delete the contents of the res->nodeList buffer
     */


    clAmsFreeMemory(res->nodeList);
    res->numNodes = 0;

    return CL_AMS_RC(rc);

}

#endif


ClRcT 
clAmsIsValidList (
        ClAmsEntityT  *entity,
        ClAmsEntityListTypeT  entityListName )
{

    AMS_FUNC_ENTER (("\n"));

    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
            {

                if ( (entityListName == CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST) ||
                        (entityListName == CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST) ||
                        (entityListName == CL_AMS_NODE_CONFIG_SU_LIST) )
                {
                    return CL_OK;
                }

                break;

            }

        case CL_AMS_ENTITY_TYPE_SG:
            {

                if ( (entityListName == CL_AMS_SG_CONFIG_SU_LIST) ||
                        (entityListName == CL_AMS_SG_CONFIG_SI_LIST) )
                {
                    return CL_OK;
                }

                break;

            }

        case CL_AMS_ENTITY_TYPE_SU:
            {
               
                if ( (entityListName == CL_AMS_SU_CONFIG_COMP_LIST) ||
                       (entityListName == CL_AMS_SU_STATUS_SI_LIST) )
                {
                    return CL_OK;
                }

                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            {

                if ( (entityListName == CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) ||
                        (entityListName == CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST) ||
                        (entityListName == CL_AMS_SI_CONFIG_SU_RANK_LIST) || 
                        (entityListName == CL_AMS_SI_CONFIG_CSI_LIST) ||
                        (entityListName == CL_AMS_SI_STATUS_SU_LIST) )
                {
                    return CL_OK;
                }

                break;

            }

        case CL_AMS_ENTITY_TYPE_COMP:
            {

                if ( entityListName == CL_AMS_COMP_STATUS_CSI_LIST )
                {
                    return CL_OK;
                }

                break;

            }
        case CL_AMS_ENTITY_TYPE_CSI:
            {

                if ( entityListName == CL_AMS_CSI_CONFIG_NVP_LIST 
                     ||
                     entityListName == CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST
                     ||
                     entityListName == CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST)
                {
                    return CL_OK;
                }

                break;

            }

        default:
            {
                return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_LIST);
            }
    } 

    return CL_AMS_RC (CL_AMS_ERR_INVALID_ENTITY_LIST);
    
}

ClRcT   
clAmsEntitySetRefPtr(
        // coverity[pass_by_value]
        ClAmsEntityRefT  sourceEntityRef,
        // coverity[pass_by_value]
        ClAmsEntityRefT  targetEntityRef )

{
    ClRcT rc;
    ClAmsEntityTypeT  sourceEntityType = {0};
    ClAmsEntityTypeT  targetEntityType = {0};

    AMS_FUNC_ENTER (("\n"));

    sourceEntityType = sourceEntityRef.entity.type;
    targetEntityType = targetEntityRef.entity.type;

    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[sourceEntityType],&sourceEntityRef);
    if (rc != CL_OK) /* ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) */
    {
        AMS_LOG(CL_DEBUG_CRITICAL, ("Error finding source entity [%s %.*s]\n", CL_AMS_STRING_ENTITY_TYPE(sourceEntityType),sourceEntityRef.entity.name.length, sourceEntityRef.entity.name.value));
        return rc;
    }
    AMS_CALL(rc);
    
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[targetEntityType],&targetEntityRef);
    if (rc != CL_OK) /* ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) */
    {
        AMS_LOG(CL_DEBUG_CRITICAL, ("Error finding target entity [%s %.*s]\n", CL_AMS_STRING_ENTITY_TYPE(targetEntityType), targetEntityRef.entity.name.length, targetEntityRef.entity.name.value));
        return rc;
    }
    AMS_CALL(rc);
    

    AMS_CHECKPTR ( !sourceEntityRef.ptr || !targetEntityRef.ptr );

    if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_SG) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_APP) )
    {

        ClAmsSGT      *sg = (ClAmsSGT *)sourceEntityRef.ptr;
        sg->config.parentApp.ptr = targetEntityRef.ptr;
        memcpy ( &sg->config.parentApp.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

    }

    else if ( sourceEntityType == CL_AMS_ENTITY_TYPE_SU )
    {

        if ( targetEntityType == CL_AMS_ENTITY_TYPE_SG )
        {

            ClAmsSUT      *su = (ClAmsSUT *)sourceEntityRef.ptr;
            su->config.parentSG.ptr = targetEntityRef.ptr;
            memcpy ( &su->config.parentSG.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

        }

        else if ( targetEntityType == CL_AMS_ENTITY_TYPE_NODE )
        {

            ClAmsSUT      *su = (ClAmsSUT *)sourceEntityRef.ptr;
            su->config.parentNode.ptr = targetEntityRef.ptr;
            memcpy ( &su->config.parentNode.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

         }

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_SI) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SG) )
    {

        ClAmsSIT      *si = (ClAmsSIT *)sourceEntityRef.ptr;
        si->config.parentSG.ptr = targetEntityRef.ptr;
        memcpy ( &si->config.parentSG.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_COMP) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SU) )
    {

        ClAmsCompT      *comp = (ClAmsCompT *)sourceEntityRef.ptr;
        comp->config.parentSU.ptr = targetEntityRef.ptr;
        memcpy ( &comp->config.parentSU.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_COMP) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_COMP) )
    {

        ClAmsCompT      *comp = (ClAmsCompT *)sourceEntityRef.ptr;
        comp->status.proxyComp = targetEntityRef.ptr;

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_CSI) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SI) )
    {

        ClAmsCSIT     *csi = (ClAmsCSIT *)sourceEntityRef.ptr;
        csi->config.parentSI.ptr = targetEntityRef.ptr;
        memcpy ( &csi->config.parentSI.entity, &targetEntityRef.entity, sizeof (ClAmsEntityT));

    }

    else
    {

        return CL_AMS_RC (CL_AMS_ERR_BAD_CONFIG);

    }

    return CL_OK;

}

ClRcT   
clAmsEntityUnsetRefPtr(
        // coverity[pass_by_value]
        ClAmsEntityRefT  sourceEntityRef,
        // coverity[pass_by_value]
        ClAmsEntityRefT  targetEntityRef )

{

    ClAmsEntityTypeT  sourceEntityType = {0};
    ClAmsEntityTypeT  targetEntityType = {0};

    AMS_FUNC_ENTER (("\n"));

    sourceEntityType = sourceEntityRef.entity.type;
    targetEntityType = targetEntityRef.entity.type;

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[sourceEntityType],
                &sourceEntityRef) );

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[targetEntityType],
                &targetEntityRef) );

    AMS_CHECKPTR ( !sourceEntityRef.ptr || !targetEntityRef.ptr );

    if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_SG) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_APP) )
    {

        ClAmsSGT      *sg = (ClAmsSGT *)sourceEntityRef.ptr;
        sg->config.parentApp.ptr = NULL;
    }

    else if ( sourceEntityType == CL_AMS_ENTITY_TYPE_SU )
    {

        if ( targetEntityType == CL_AMS_ENTITY_TYPE_SG )
        {

            ClAmsSUT      *su = (ClAmsSUT *)sourceEntityRef.ptr;
            su->config.parentSG.ptr = NULL;
        }

        else if ( targetEntityType == CL_AMS_ENTITY_TYPE_NODE )
        {

            ClAmsSUT      *su = (ClAmsSUT *)sourceEntityRef.ptr;
            su->config.parentNode.ptr = NULL;
         }

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_SI) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SG) )
    {

        ClAmsSIT      *si = (ClAmsSIT *)sourceEntityRef.ptr;
        si->config.parentSG.ptr = NULL;
    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_COMP) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SU) )
    {

        ClAmsCompT      *comp = (ClAmsCompT *)sourceEntityRef.ptr;
        comp->config.parentSU.ptr = NULL;
    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_COMP) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_COMP) )
    {

        ClAmsCompT      *comp = (ClAmsCompT *)sourceEntityRef.ptr;
        comp->status.proxyComp = NULL;

    }

    else if ( (sourceEntityType == CL_AMS_ENTITY_TYPE_CSI) &&
            (targetEntityType == CL_AMS_ENTITY_TYPE_SI) )
    {

        ClAmsCSIT     *csi = (ClAmsCSIT *)sourceEntityRef.ptr;
        csi->config.parentSI.ptr = NULL;
    }

    else
    {

        return CL_AMS_RC (CL_AMS_ERR_BAD_CONFIG);

    }

    return CL_OK;

}


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


ClRcT
clAmsValidateConfig(
        CL_IN  ClAmsT  *ams, 
        CL_IN  ClUint32T  mode )
{

    ClAmsEntityDbT  entityDb = {0};
    ClCntNodeHandleT  nodeHandle = NULL;
    ClAmsEntityT  *entity = NULL;
    ClUint32T i = 0;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!ams);

    for ( i=1; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {

        entityDb = ams->db.entityDb[i];

#ifndef POST_RC2

        if ( entityDb.type == CL_AMS_ENTITY_TYPE_APP )
        {
            continue;
        }

#endif

        for ( entity = (ClAmsEntityT *)
                clAmsCntGetFirst( &entityDb.db,&nodeHandle);
                entity != NULL; 
                entity = (ClAmsEntityT *)
                clAmsCntGetNext( &entityDb.db,&nodeHandle) )
        {

            AMS_CALL (clAmsEntityValidate(entity,mode));

        }

    }

    return CL_OK;

}

static ClRcT
clAmsEntityMarshall(ClAmsEntityT *entity, ClPtrT userArg, ClUint32T marshallMask)
{
    ClAmsEntityDBWalkArgsT *arg = userArg;
    ClBufferHandleT inMsgHdl = arg->inMsgHdl;
    ClUint32T versionCode = arg->versionCode;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entity );

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_ENTITY:
        {
            break;
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT    *node = (ClAmsNodeT *) entity;

            AMS_CALL( clAmsDBNodeMarshall(node, inMsgHdl, marshallMask, versionCode));

            break;

        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT    *sg = (ClAmsSGT *) entity;

            AMS_CALL( clAmsDBSGMarshall(sg, inMsgHdl, marshallMask, versionCode) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT    *su = (ClAmsSUT *) entity;
           
            AMS_CALL( clAmsDBSUMarshall(su, inMsgHdl, marshallMask, versionCode) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT    *si = (ClAmsSIT *) entity;

            AMS_CALL( clAmsDBSIMarshall(si, inMsgHdl, marshallMask, versionCode) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT    *comp = (ClAmsCompT *) entity;

            AMS_CALL( clAmsDBCompMarshall(comp, inMsgHdl, marshallMask, versionCode) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {

            ClAmsCSIT    *csi = (ClAmsCSIT *) entity;

            AMS_CALL( clAmsDBCSIMarshall(csi, inMsgHdl, marshallMask, versionCode) );

            break;

        }

        default:
        {

            AMS_LOG(CL_DEBUG_ERROR,("Invalid entity type [%d] \n",entity->type));
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

        }

    } 
    
    return CL_OK;

}

static ClRcT
clAmsEntityConfigMarshall(ClAmsEntityT *entity, ClPtrT userArg)
{

    return clAmsEntityMarshall(entity, userArg, CL_AMS_ENTITY_DB_CONFIG);
}

static ClRcT 
clAmsEntityStatusMarshall(ClAmsEntityT *entity, ClPtrT userArg)
{
    return clAmsEntityMarshall(entity, userArg, CL_AMS_ENTITY_DB_STATUS | CL_AMS_ENTITY_DB_CONFIG_LIST);
}

static ClRcT
clAmsEntityDBMarshall(ClAmsEntityDbT *entityDb, ClBufferHandleT inMsgHdl, ClBoolT marshallConfig, ClUint32T versionCode)
{
    ClRcT  rc = CL_OK;
    ClAmsEntityDBWalkArgsT args = {0};
    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityDb );

#ifndef POST_RC2

    if ( entityDb->type == CL_AMS_ENTITY_TYPE_APP )
    {
        return CL_OK;
    }

#endif

    if ( entityDb->numEntities )
    {
        args.inMsgHdl = inMsgHdl;
        args.versionCode = versionCode;

        if(marshallConfig)
            rc = clAmsEntityDbWalkExtended(entityDb, clAmsEntityConfigMarshall, (ClPtrT)&args); 
        else
            rc = clAmsEntityDbWalkExtended(entityDb, clAmsEntityStatusMarshall, (ClPtrT)&args);
    }

    return rc;

}

static ClRcT
clAmsDBMarshallVersion(ClAmsDbT *amsDb, ClBufferHandleT inMsgHdl, ClUint32T *pVersionCode)
{
    ClUint32T versionCode = CL_VERSION_CURRENT;
    ClVersionT version = {0};

    if(!pVersionCode)
        pVersionCode = &versionCode;
    else
    {
        *pVersionCode = versionCode;
        clNodeCacheMinVersionGet(NULL, pVersionCode);
    }

    version.releaseCode =  CL_VERSION_RELEASE(*pVersionCode);
    version.majorVersion = CL_VERSION_MAJOR(*pVersionCode);
    version.minorVersion = CL_VERSION_MINOR(*pVersionCode);

    return clXdrMarshallClVersionT(&version, inMsgHdl, 0);
}

ClRcT
clAmsDBMarshall(ClAmsDbT *amsDb, ClBufferHandleT inMsgHdl)
{
    ClUint32T i = 0;
    ClUint32T versionCode = 0;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !amsDb );

    AMS_CALL ( clAmsDBMarshallVersion(amsDb, inMsgHdl, &versionCode) );

    /*  
     * First marshall the entity configs.
     */
    for (i=1; i<= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {

        AMS_CALL ( clAmsEntityDBMarshall(&amsDb->entityDb[i], inMsgHdl, CL_TRUE, versionCode) );

    }
    
    /*
     * Second - Marshall the status
     */

    for (i=1; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        AMS_CALL ( clAmsEntityDBMarshall(&amsDb->entityDb[i], inMsgHdl, 0, versionCode) );
    }

    /*
     * Third - Marshall the AMS essentials.
     */

    AMS_CALL ( clAmsServerDataMarshall(&gAms, inMsgHdl, versionCode) );

    /*
     * Fourth - Marshall entity user data. (if present)
     */

    AMS_CALL ( clAmsEntityUserDataMarshall(inMsgHdl, versionCode) );

    /*
     * Write the end marker for the ckpt
     */
    AMS_CALL(clAmsDBMarshallEnd(inMsgHdl, versionCode));

    return CL_OK;

}

static ClRcT
clAmsEntityConfigSerialize(ClAmsEntityT *entity, ClPtrT arg)
{
    return clAmsEntityMarshall(entity, arg, CL_AMS_ENTITY_DB_CONFIG);
}

static ClRcT
clAmsEntityConfigListSerialize(ClAmsEntityT *entity, ClPtrT arg)
{
    return clAmsEntityMarshall(entity, arg, CL_AMS_ENTITY_DB_CONFIG_LIST);
}

static ClRcT
clAmsEntityDBConfigSerialize(ClAmsEntityDbT *entityDb, 
                             ClBufferHandleT inMsgHdl,
                             ClBoolT marshallList,
                             ClUint32T versionCode)
{
    #ifndef POST_RC2
    if(entityDb->type == CL_AMS_ENTITY_TYPE_APP)
    {
        return CL_OK;
    }
    #endif

    if(entityDb->numEntities)
    {
        ClAmsEntityDBWalkArgsT args = {0};
        args.inMsgHdl = inMsgHdl;
        args.versionCode = versionCode;

        if(!marshallList)
            return clAmsEntityDbWalkExtended(entityDb, clAmsEntityConfigSerialize, (ClPtrT)&args);
        else
            return clAmsEntityDbWalkExtended(entityDb, clAmsEntityConfigListSerialize, (ClPtrT)&args);
    }

    return CL_OK;
}

ClRcT clAmsDBConfigSerialize(ClAmsDbT *amsDB, ClBufferHandleT inMsgHdl)
{
    ClUint32T i = 0;
    ClUint32T versionCode = 0;

    /*
     * Marshall the db version.
     */
    AMS_CALL ( clAmsDBMarshallVersion(amsDB, inMsgHdl, &versionCode) );

    /*
     * First pass - Marshall all configuration for the entities.
     */

    for(i = 1; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        AMS_CALL(clAmsEntityDBConfigSerialize(&amsDB->entityDb[i], inMsgHdl, CL_FALSE, versionCode));
    }

    /*
     * Second pass - Marshall lists or relations between entities.
     */
    for(i = 1; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        AMS_CALL(clAmsEntityDBConfigSerialize(&amsDB->entityDb[i], inMsgHdl, CL_TRUE, versionCode));
    }

    /*
     * Write user data.
     */
    AMS_CALL (clAmsEntityUserDataMarshall(inMsgHdl, versionCode));

    /*
     * End of file marker
     */

    AMS_CALL(clAmsDBMarshallEnd(inMsgHdl, versionCode));

    return CL_OK;
}

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

ClRcT
clAmsDBSerializeXML(
        CL_IN  ClAmsDbT  *amsDb )
{
    ClUint32T i = 0;
    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !amsDb );

    for (i=1; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {

        AMS_CALL ( clAmsEntityDBSerializeXML(&amsDb->entityDb[i]) );

    }

    return CL_OK;

}

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

ClRcT
clAmsEntityDBSerializeXML(
        CL_IN  ClAmsEntityDbT  *entityDb)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityDb );

#ifndef POST_RC2

    if ( entityDb->type == CL_AMS_ENTITY_TYPE_APP )
    {
        return CL_OK;
    }

#endif

    if ( entityDb->numEntities )
    {
        rc = clAmsEntityDbWalk(entityDb, clAmsEntitySerializeXML); 
    }

    return rc;

}

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

ClRcT
clAmsEntitySerializeXML(
        CL_IN  ClAmsEntityT  *entity)
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entity );

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_ENTITY:
        {
            break;
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT    *node = (ClAmsNodeT *) entity;

            AMS_CALL( clAmsDBNodeXMLize(node));

            break;

        }
#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            //ClAmsAppT    *app = (ClAmsAppT *) entity;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT    *sg = (ClAmsSGT *) entity;

            AMS_CALL( clAmsDBSGXMLize(sg) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT    *su = (ClAmsSUT *) entity;
           
            AMS_CALL( clAmsDBSUXMLize(su) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT    *si = (ClAmsSIT *) entity;

            AMS_CALL( clAmsDBSIXMLize(si) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT    *comp = (ClAmsCompT *) entity;

            AMS_CALL( clAmsDBCompXMLize(comp) );

            break;

        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {

            ClAmsCSIT    *csi = (ClAmsCSIT *) entity;

            AMS_CALL( clAmsDBCSIXMLize(csi) );

            break;

        }

        default:
        {

            AMS_LOG(CL_DEBUG_ERROR,("Invalid entity type [%d] \n",entity->type));
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

        }

    } 
    
    return CL_OK;

}

ClRcT
clAmsInvocationListMarshall(ClCntHandleT listHandle, ClBufferHandleT msg)
{
    ClAmsInvocationT *invocationData = NULL;
    ClUint32T numInvocations = 0;
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;

    clAmsInvocationLock();

    AMS_CHECK_RC_ERROR( clAmsInvocationMainMarshall(&gAms, msg) );
    AMS_CHECK_RC_ERROR( clCntSizeGet(listHandle, &numInvocations) );
    AMS_CHECK_RC_ERROR ( clXdrMarshallClUint32T(&numInvocations, msg, 0) );

    for ( invocationData = (ClAmsInvocationT *)clAmsCntGetFirst(
                &listHandle,&nodeHandle); invocationData!= NULL;
            invocationData= (ClAmsInvocationT *)clAmsCntGetNext(
                &listHandle,&nodeHandle) )
    {
        VDECL_VER(ClAmsInvocationIDLT, 4, 1, 0) invocationDataIDL = {0};
        AMS_CHECKPTR_AND_EXIT( !invocationData );
        invocationDataIDL.invocation = invocationData->invocation;
        invocationDataIDL.cmd = invocationData->cmd;
        invocationDataIDL.csiTargetOne = invocationData->csiTargetOne;
        invocationDataIDL.reassignCSI = invocationData->reassignCSI;
        clNameCopy(&invocationDataIDL.compName, &invocationData->compName);
        clNameCopy(&invocationDataIDL.csiName, &invocationData->csiName);
        AMS_CHECK_RC_ERROR( VDECL_VER(clXdrMarshallClAmsInvocationIDLT, 4, 1, 0)(&invocationDataIDL, msg, 0));
    }
    
    exitfn:
    clAmsInvocationUnlock();
    return rc;
}

ClRcT
clAmsInvocationListXMLize(
        CL_IN  ClCntHandleT  listHandle )
{ 

    ClAmsInvocationT  *invocationData = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;

    AMS_FUNC_ENTER (("\n")); 
    
    AMS_CALL( clAmsInvocationPackUnpackMain (&gAms) );

    for ( invocationData = (ClAmsInvocationT *)clAmsCntGetFirst(
                &listHandle,&nodeHandle); invocationData!= NULL;
            invocationData= (ClAmsInvocationT *)clAmsCntGetNext(
                &listHandle,&nodeHandle) )
    {

        AMS_CHECKPTR( !invocationData );

        AMS_CALL( clAmsInvocationInstanceXMLize (invocationData) );

    }

    return CL_OK;

}

ClRcT
clAmsSetEntityTimer(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  ClAmsEntityTimerT  *entityTimer )
{

    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!entity || !entityTimer );

    memcpy ( &entityRef.entity, entity, sizeof (ClAmsEntityT));

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entity->type],
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    if ( entityTimer->type == CL_AMS_NODE_TIMER_SUFAILOVER) 
    {

        ClAmsNodeT  *node  = (ClAmsNodeT *)entityRef.ptr;
        memcpy ( &node->status.suFailoverTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));

    }

    else if ( entityTimer->type == CL_AMS_SG_TIMER_INSTANTIATE) 
    {

        ClAmsSGT  *sg  = (ClAmsSGT *)entityRef.ptr;
        memcpy ( &sg->status.instantiateTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));

    }

    else if ( entityTimer->type == CL_AMS_SG_TIMER_ADJUST) 
    {

        ClAmsSGT  *sg  = (ClAmsSGT *)entityRef.ptr;
        memcpy ( &sg->status.adjustTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));
    }

    else if ( entityTimer->type == CL_AMS_SG_TIMER_ADJUST_PROBATION) 
    {

        ClAmsSGT  *sg  = (ClAmsSGT *)entityRef.ptr;
        memcpy ( &sg->status.adjustProbationTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));
    }

    else if ( entityTimer->type == CL_AMS_SU_TIMER_COMPRESTART ) 
    {

        ClAmsSUT  *su  = (ClAmsSUT *)entityRef.ptr;
        memcpy ( &su->status.compRestartTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));

    }

    else if ( entityTimer->type == CL_AMS_SU_TIMER_SURESTART) 
    {

        ClAmsSUT  *su  = (ClAmsSUT *)entityRef.ptr;
        memcpy ( &su->status.suRestartTimer, entityTimer, 
                sizeof (ClAmsEntityTimerT));

    }
    
    else if( entityTimer->type == CL_AMS_SU_TIMER_PROBATION)
    {
        ClAmsSUT *su = (ClAmsSUT*)entityRef.ptr;
        memcpy(&su->status.suProbationTimer, entityTimer,
               sizeof(ClAmsEntityTimerT));
    }

    else if( entityTimer->type == CL_AMS_SU_TIMER_ASSIGNMENT)
    {
        ClAmsSUT *su = (ClAmsSUT*)entityRef.ptr;
        memcpy(&su->status.suAssignmentTimer, entityTimer,
               sizeof(ClAmsEntityTimerT));
    }

    else if (entityTimer->type == CL_AMS_COMP_TIMER_INSTANTIATE)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.instantiate, entityTimer,
               sizeof(ClAmsEntityTimerT));
    }
    
    else if (entityTimer->type == CL_AMS_COMP_TIMER_TERMINATE)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.terminate, entityTimer, sizeof(ClAmsEntityTimerT));
    }
    
    else if(entityTimer->type == CL_AMS_COMP_TIMER_CLEANUP)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.cleanup, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    else if(entityTimer->type == CL_AMS_COMP_TIMER_AMSTART)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.amStart, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    else if(entityTimer->type == CL_AMS_COMP_TIMER_AMSTOP)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.amStop, entityTimer, sizeof(ClAmsEntityTimerT));
    }
    
    else if(entityTimer->type == CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.quiescingComplete, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    else if(entityTimer->type == CL_AMS_COMP_TIMER_CSISET)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.csiSet, entityTimer, sizeof(ClAmsEntityTimerT));
    }
    
    else if(entityTimer->type == CL_AMS_COMP_TIMER_CSIREMOVE)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.csiRemove, entityTimer, sizeof(ClAmsEntityTimerT));
    }
    
    else if(entityTimer->type == CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.proxiedCompInstantiate, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    else if(entityTimer->type == CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.proxiedCompCleanup, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    else if(entityTimer->type == CL_AMS_COMP_TIMER_INSTANTIATEDELAY)
    {
        ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
        memcpy(&comp->status.timers.instantiateDelay, entityTimer, sizeof(ClAmsEntityTimerT));
    }

    return CL_OK;

}

ClRcT   
clAmsXMLizeDB (
               CL_IN  ClAmsT  *ams,
               CL_IN  ClCharT *dbName,
               ClCharT **pData)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !ams );
    AMS_CHECKPTR ( !dbName );
    AMS_CHECKPTR ( !pData );

    AMS_CHECK_RC_ERROR( clAmsDBPackUnpackMain (ams) );
    
    AMS_CHECK_RC_ERROR( clAmsDBSerializeXML(&ams->db) );

    AMS_CHECK_RC_ERROR( clAmsDBGAmsXMLize (ams) );

    *pData = clParserToXml(amfPtr.headPtr);
    
    clParserFree(amfPtr.headPtr);

exitfn:

    return CL_AMS_RC (rc);

}

ClRcT   
clAmsDeXMLizeDB (
                 CL_IN  ClAmsT  *ams,
                 CL_IN  ClCharT *dbName)
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !ams );
    AMS_CHECKPTR ( !dbName );

#ifdef AMS_CKPT_TEST

    AMS_CALL (clAmsDbTerminate(&ams->db));
    AMS_CALL (clAmsDbInstantiate(&ams->db));

#endif

    AMS_CALL(clAmsDBUnpackMain(dbName));

    clParserFree(amfPtr.headPtr);

    AMS_CALL (clAmsDbStartTimers(&ams->db));

    return CL_OK;

}


ClRcT   
clAmsInvocationMarshall (
                       CL_IN  ClAmsT  *ams,
                       CL_IN  ClCharT *invocationName,
                       ClBufferHandleT msg)
{

    AMS_FUNC_ENTER (("\n"));
    AMS_CHECKPTR ( !ams );
    AMS_CHECKPTR ( !invocationName );
    AMS_CALL( clAmsInvocationListMarshall ( ams->invocationList, msg) );
    return CL_OK;
}

ClRcT   
clAmsXMLizeInvocation (
                       CL_IN  ClAmsT  *ams,
                       CL_IN  ClCharT *invocationName,
                       ClCharT **pData)
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !ams );
    AMS_CHECKPTR ( !invocationName );
    AMS_CHECKPTR ( !pData);

    AMS_CHECK_RC_ERROR( clAmsInvocationListXMLize ( ams->invocationList) );

    *pData = clParserToXml(invocationPtr);

    clParserFree(invocationPtr);

exitfn:

    return CL_AMS_RC (rc);
    
}

ClRcT   
clAmsInvocationUnmarshall(ClAmsT *ams, ClCharT *invocationName, ClBufferHandleT msg)
{
    ClUint32T versionCode = 0;
    ClVersionT version = {0};
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!ams);
    AMS_CHECKPTR( !invocationName);

    rc = clXdrUnmarshallClVersionT(msg, &version);
    if(rc != CL_OK) return rc;
    
    versionCode = CL_VERSION_CODE(version.releaseCode, version.majorVersion, version.minorVersion);

    switch(versionCode)
    {
    case CL_VERSION_CODE(CL_RELEASE_VERSION, 1, CL_MINOR_VERSION):
        rc = clAmsInvocationListUnmarshall(msg);
        break;

    /*
     * case CL_VERSION_CODE(4, 1, 0): break;
     */
    default:
        clLogError("INVOCATION", "READ", "Unsupported invocation version [%d.%d.%d]",
                   version.releaseCode, version.majorVersion, version.minorVersion);
        rc = CL_AMS_RC(CL_ERR_VERSION_MISMATCH);
    }

    return rc;
}

ClRcT   
clAmsDeXMLizeInvocation (
                         CL_IN  ClAmsT  *ams,
                         CL_IN  ClCharT *invocationName)
{

    ClParserPtrT invocationFilePtr = NULL;
    const ClCharT *versionPtr = NULL;
    ClUint32T versionCode = 0;
    ClUint32T releaseCode = 0;
    ClUint32T majorVersion = 0;
    ClUint32T minorVersion = 0;
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!ams);
    AMS_CHECKPTR( !invocationName);
    
    invocationFilePtr = clParserParseFile(invocationName);

    if(!invocationFilePtr) return CL_OK;
    
    versionPtr = clParserAttr(invocationFilePtr, "version");
    
    if(!versionPtr)
    {
        /*
         * could be an empty section.
         */
        goto out_free;
    }

    if(sscanf(versionPtr, "%u.%u.%u", &releaseCode, 
              &majorVersion, &minorVersion) != 3)
    {
        clLogError("INVOCATION", "READ", "Invalid version format");
        rc = CL_AMS_RC(CL_ERR_INVALID_STATE);
        goto out_free;
    }

    versionCode = CL_VERSION_CODE(releaseCode, majorVersion, minorVersion);

    switch(versionCode)
    {

    case CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE):
        rc = clAmsInvocationInstanceDeXMLize(invocationFilePtr);
        break;

    default:
        clLogError("INVOCATION", "READ", "Unsupported invocation version [%d.%d.%d]",
                   releaseCode, majorVersion, minorVersion);
        rc = CL_AMS_RC(CL_ERR_VERSION_MISMATCH);
    }

    out_free:
    clParserFree(invocationFilePtr);

    return rc;
}

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


ClCntDataHandleT 
clAmsCntGetFirst(
        CL_IN  ClCntHandleT  *cntHandle,
        CL_INOUT  ClCntNodeHandleT  *nodeHandle )
{

    ClCntDataHandleT  data = NULL;
    ClRcT  rc = CL_OK;
    ClRcT  rc2 = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    if ( !cntHandle)
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("Container handle is Null Pointer. Exiting..\n"));
        return (ClCntDataHandleT) 0;
    }

    rc = clCntFirstNodeGet(
                *cntHandle,
                nodeHandle);

    if ( rc == CL_OK )
    {

         rc2 = clCntNodeUserDataGet(
                *cntHandle,
                *nodeHandle,
                (ClCntDataHandleT *) &data);

         if ( rc2 == CL_OK )
         {
             return data;
         }

    }

    if ( ( CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST ) && ( rc !=CL_OK ) )
    { 

        AMS_LOG(CL_DEBUG_ERROR,
                ("Error[%d] in finding first node of list.  Returning NULL..\n",
                 rc));

    }

    if ( ( CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST ) && ( rc2 != CL_OK ) )
    {

        AMS_LOG(CL_DEBUG_ERROR,
                ("Error[%d] in finding first node data in list. Returning NULL..\n",
                 rc2));
   
    }

    return  0;

}

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
        CL_IN  ClCntNodeHandleT  *nodeHandle)
{

    ClRcT  rc = CL_OK;
    ClCntHandleT  data = 0;

    AMS_FUNC_ENTER (("\n"));

    if ( !cntHandle|| !nodeHandle)
    {

        AMS_LOG(CL_DEBUG_ERROR,
                ("container handle  or node handle is Null Pointer \
                 Exiting..\n"));
        return (ClCntDataHandleT)0 ;

    }

    rc = clCntNextNodeGet(
            *cntHandle,
            *nodeHandle,
            nodeHandle);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClCntHandleT) 0;
    }

    else if ( rc != CL_OK )
    {

        AMS_LOG(CL_DEBUG_ERROR,
                ("Error[%d] in finding next node in container. Returning NULL..\n",
                 rc));
            return (ClCntHandleT) 0;

    }

    rc = clCntNodeUserDataGet(
            *cntHandle,
            *nodeHandle,
            (ClCntDataHandleT *) &data );

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) 
    {
        return (ClCntHandleT) 0;
    }

    else if ( rc != CL_OK )
    {

        AMS_LOG(CL_DEBUG_ERROR,
                ("Error[%d] in finding next node in list. Returning NULL..\n",
                 rc));
        return (ClCntHandleT) 0;

    }

    return data;

}

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

ClRcT
clAmsCSIPGTrackListPrint(
       CL_IN  ClCntHandleT  listHandle )
{

    ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;
    ClUint32T  numNodes = -1;

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( clCntSizeGet( listHandle, &numNodes) );

    CL_AMS_PRINT_TWO_COL("PG Track List","List Count[%u]", numNodes);

    for ( pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetFirst(
                &listHandle,&nodeHandle); pgTrackClient!= NULL;
            pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetNext(
                &listHandle,&nodeHandle))
    {

        AMS_CHECKPTR ( !pgTrackClient );

        CL_AMS_PRINT_TWO_COL("",
                "   NodeAddress:%d", pgTrackClient->address.iocPhyAddress.nodeAddress);
        CL_AMS_PRINT_TWO_COL("",
                "   PortID:%d", pgTrackClient->address.iocPhyAddress.portId);
        CL_AMS_PRINT_TWO_COL("",
                "   TrackFlags:%d", pgTrackClient->trackFlags);

    }

    return CL_OK;

}

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

ClRcT
clAmsCSIPGTrackListXMLPrint(
       CL_IN  ClCntHandleT  listHandle )
{

    ClAmsCSIPGTrackClientT  *pgTrackClient = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;
    ClUint32T  numNodes = -1;

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( clCntSizeGet( listHandle, &numNodes) );

    CL_AMS_PRINT_OPEN_TAG_ATTR("pg_tracks", "%u", numNodes);
    
    for ( pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetFirst(
                &listHandle,&nodeHandle); pgTrackClient!= NULL;
            pgTrackClient = (ClAmsCSIPGTrackClientT *)clAmsCntGetNext(
                &listHandle,&nodeHandle))
    {

        AMS_CHECKPTR ( !pgTrackClient );

        CL_AMS_PRINT_OPEN_TAG("pg_track");
        
        CL_AMS_PRINT_TAG_VALUE("node_address", "%d", 
                               pgTrackClient->address.iocPhyAddress.
                               nodeAddress);
        CL_AMS_PRINT_TAG_VALUE("port", "%#x",
                               pgTrackClient->address.iocPhyAddress.portId);
        CL_AMS_PRINT_TAG_VALUE("track_flags", "%#x",
                               pgTrackClient->trackFlags);

        CL_AMS_PRINT_CLOSE_TAG("pg_track");

    }

    CL_AMS_PRINT_CLOSE_TAG("pg_tracks");

    return CL_OK;

}

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

ClRcT
clAmsCSINVPListPrint(
        CL_IN  ClCntHandleT  listHandle )
{

    ClAmsCSINameValuePairT  *nvp = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;
    ClUint32T  numNodes = -1;

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( clCntSizeGet( listHandle, &numNodes) );

    CL_AMS_PRINT_TWO_COL("CSI NVP List", "List Count[%u]", numNodes);

    for ( nvp = (ClAmsCSINameValuePairT*)clAmsCntGetFirst(
                &listHandle,&nodeHandle); nvp != NULL;
            nvp  = (ClAmsCSINameValuePairT*)clAmsCntGetNext(
                &listHandle,&nodeHandle))
    {

        AMS_CHECKPTR ( !nvp);

        CL_AMS_PRINT_TWO_COL("", "   Name:%s", nvp->paramName.value);

        CL_AMS_PRINT_TWO_COL("", "   Value:%s", nvp->paramValue.value);

    }

    return CL_OK;

}

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

ClRcT
clAmsCSINVPListXMLPrint(
        CL_IN  ClCntHandleT  listHandle )
{

    ClAmsCSINameValuePairT  *nvp = NULL;
    ClCntNodeHandleT  nodeHandle = NULL;
    ClUint32T  numNodes = -1;

    AMS_FUNC_ENTER (("\n"));

    AMS_CALL( clCntSizeGet( listHandle, &numNodes) );

    CL_AMS_PRINT_OPEN_TAG_ATTR("csi_name_value_pairs", "%u", numNodes);
    

    for ( nvp = (ClAmsCSINameValuePairT*)clAmsCntGetFirst(
                &listHandle,&nodeHandle); nvp != NULL;
            nvp  = (ClAmsCSINameValuePairT*)clAmsCntGetNext(
                &listHandle,&nodeHandle))
    {

        AMS_CHECKPTR ( !nvp);

        CL_AMS_PRINT_OPEN_TAG("csi_name_value");
        
        CL_AMS_PRINT_TAG_VALUE("name", "%s", nvp->paramName.value);

        CL_AMS_PRINT_TAG_VALUE("value", "%s", nvp->paramValue.value);

        CL_AMS_PRINT_CLOSE_TAG("csi_name_value");

    }

    CL_AMS_PRINT_CLOSE_TAG("csi_name_value_pairs");

    return CL_OK;

}

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

ClRcT
clAmsDebugEnable(
        CL_IN  clAmsMgmtDebugEnableRequestT  *request )
{

    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !request );

    memcpy (&entityRef.entity,&request->entity,sizeof (ClAmsEntityT));

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[request->entity.type],
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    entityRef.ptr->debugFlags |= request->debugFlags;

    return CL_OK;

}

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

ClRcT
clAmsDebugDisable(
        CL_IN  clAmsMgmtDebugDisableRequestT  *request )
{

    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !request );

    memcpy (&entityRef.entity,&request->entity,sizeof (ClAmsEntityT));

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[request->entity.type],
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    entityRef.ptr->debugFlags &= ~request->debugFlags;

    return CL_OK;

}

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

ClRcT
clAmsDebugGet(
        CL_IN  ClAmsEntityT  *entity,
        CL_IN  clAmsMgmtDebugGetResponseT  *response )
{

    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !response );

    memcpy (&entityRef.entity,entity,sizeof (ClAmsEntityT));

    AMS_CALL( clAmsEntityDbFindEntity(
                &gAms.db.entityDb[entity->type],
                &entityRef) );

    AMS_CHECKPTR ( !entityRef.ptr );

    response->debugFlags = entityRef.ptr->debugFlags;

    return CL_OK;

}

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

ClRcT
clAmsDbStartTimers(
        CL_IN  ClAmsDbT  *amsDb )
{
    ClUint32T i = 0;
    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !amsDb );

    for (i=1; i<= CL_AMS_ENTITY_TYPE_MAX; i++)
    {
        AMS_CALL ( clAmsEntityDbStartTimers(&amsDb->entityDb[i]) );
    }

    return CL_OK;

}

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

ClRcT
clAmsEntityDbStartTimers(
        CL_IN  ClAmsEntityDbT  *entityDb )
{

    ClRcT  rc = CL_OK;

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entityDb );

#ifndef POST_RC2

    if ( entityDb->type == CL_AMS_ENTITY_TYPE_APP )
    {
        return CL_OK;
    }

#endif

    if ( entityDb->numEntities )
    {
        rc = clAmsEntityDbWalk(entityDb, clAmsEntityStartTimers); 
    }

    return rc;
}

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

ClRcT
clAmsEntityStartTimers(
        CL_IN  ClAmsEntityT  *entity )
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR ( !entity );

    switch ( entity->type )
    {

        case CL_AMS_ENTITY_TYPE_ENTITY:
        {
            break;
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {

            ClAmsNodeT  *node = (ClAmsNodeT *) entity;
            ClAmsEntityTimerT  *timer = &node->status.suFailoverTimer;

            AMS_CALL( clAmsTimerResetValues(timer) );

            if (timer->count)
            {

               AMS_CALL( clAmsEntityTimerStart(
                           (ClAmsEntityT *) node,
                           CL_AMS_NODE_TIMER_SUFAILOVER) );

            }

            break;

        }

#ifdef POST_RC2

        case CL_AMS_ENTITY_TYPE_APP:
        {
            break;
        }

#endif

        case CL_AMS_ENTITY_TYPE_SG:
        {

            ClAmsSGT  *sg = (ClAmsSGT *) entity;
            ClAmsEntityTimerT  *timer = &sg->status.instantiateTimer;
            
            /*
             * Kick off failover history timers.
             */
            clAmsSGFailoverHistoryConfigure(sg);

            AMS_CALL( clAmsTimerResetValues(timer) );

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) sg,
                            CL_AMS_SG_TIMER_INSTANTIATE) );

            }
            
            timer = &sg->status.adjustTimer;
            AMS_CALL( clAmsTimerResetValues (timer));
            
            if(timer->count)
            {
                AMS_CALL(clAmsEntityTimerStart((ClAmsEntityT*)sg, 
                                               CL_AMS_SG_TIMER_ADJUST));
            }

            timer = &sg->status.adjustProbationTimer;
            AMS_CALL( clAmsTimerResetValues (timer));
            
            if(timer->count)
            {
                AMS_CALL(clAmsEntityTimerStart((ClAmsEntityT*)sg, 
                                               CL_AMS_SG_TIMER_ADJUST_PROBATION));
            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_SU:
        {

            ClAmsSUT  *su = (ClAmsSUT *) entity;
            ClAmsEntityTimerT  *timer = &su->status.compRestartTimer;

            AMS_CALL( clAmsTimerResetValues(timer) );

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) su,
                            CL_AMS_SU_TIMER_COMPRESTART) );

            }

            timer = &su->status.suRestartTimer;
            AMS_CALL (clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) su,
                            CL_AMS_SU_TIMER_SURESTART) );

            }

            timer = &su->status.suProbationTimer;
            AMS_CALL (clAmsTimerResetValues(timer));
            
            if(timer->count)
            {
                AMS_CALL(clAmsEntityTimerStart(
                                               (ClAmsEntityT*)su,
                                               CL_AMS_SU_TIMER_PROBATION));
            }

            timer = &su->status.suAssignmentTimer;
            AMS_CALL (clAmsTimerResetValues(timer));

            if(timer->count)
            {
                AMS_CALL(clAmsEntityTimerStart(
                                               (ClAmsEntityT*)su,
                                               CL_AMS_SU_TIMER_ASSIGNMENT));
            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_SI:
        {

            break;

        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {

            ClAmsCompT  *comp = (ClAmsCompT *) entity;
            ClAmsEntityTimerT  *timer = &comp->status.timers.instantiate;

            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATE) );

            }

            timer = &comp->status.timers.instantiateDelay;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATEDELAY) );

            }

            timer = &comp->status.timers.terminate;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_TERMINATE) );

            }

            timer = &comp->status.timers.cleanup;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CLEANUP) );

            }

            timer = &comp->status.timers.amStart;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_AMSTART) );

            }

            timer = &comp->status.timers.amStop;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_AMSTOP) );

            }

            timer = &comp->status.timers.quiescingComplete;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

            }

            timer = &comp->status.timers.csiSet;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

            }

            timer = &comp->status.timers.csiRemove;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSIREMOVE) );

            }

            timer = &comp->status.timers.proxiedCompInstantiate;
            AMS_CALL(clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );

            }

            timer = &comp->status.timers.proxiedCompCleanup;
            AMS_CALL (clAmsTimerResetValues(timer));

            if ( timer->count)
            {

                AMS_CALL( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

            }

            break;

        }

        case CL_AMS_ENTITY_TYPE_CSI:
        {

            /*
             * No timers for CSI
             */

            break;

        }

        default:
        {

            AMS_LOG(CL_DEBUG_ERROR,("Invalid entity type [%d]\n",entity->type));
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

        }

    } 
    
    return CL_OK;

}

ClRcT
clAmsTimerResetValues (
        CL_IN  ClAmsEntityTimerT  *timer )
{

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!timer);

    timer->handle = 0;
    timer->entity = 0;

    return CL_OK;

}


ClRcT
clAmsUpdateAlarmHandle(
        CL_INOUT  const ClNameT  *compName,
        CL_IN  ClUint32T  alarmHandle )
{
    ClRcT  rc = CL_OK;
    ClAmsEntityRefT  entityRef = {{0},0,0};

    AMS_FUNC_ENTER (("\n"));

    AMS_CHECKPTR (!compName);

    AMS_CALL ( clOsalMutexLock(gAms.mutex));

    memcpy (&entityRef.entity.name, compName, sizeof (ClNameT));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP; 
    
    if ( (rc = clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                    &entityRef))
            == CL_OK )
    {

        ClAmsCompT  *comp = (ClAmsCompT *)entityRef.ptr;

        AMS_CHECKPTR_AND_UNLOCK ( !comp,gAms.mutex );

        comp->status.alarmHandle = alarmHandle;

    }

    else
    {

        if ( CL_GET_ERROR_CODE (rc) != CL_ERR_NOT_EXIST )
        {
            AMS_CALL ( clOsalMutexUnlock(gAms.mutex));
            goto exitfn;
        }
        
        entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;

        AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX (
                clAmsEntityDbFindEntity(
                    &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                    &entityRef),
                gAms.mutex );

        ClAmsNodeT  *node = (ClAmsNodeT *)entityRef.ptr;

        AMS_CHECKPTR_AND_UNLOCK ( !node,gAms.mutex);

        node->status.alarmHandle = alarmHandle;

     }

    AMS_CALL ( clOsalMutexUnlock(gAms.mutex));

exitfn:

    return CL_AMS_RC (rc);

}

static ClRcT
clAmsAssignCSICallback(ClAmsInvocationT *invocationData,ClPtrT arg)
{
    ClAmsCSIT *csi = NULL;
    ClAmsEntityRefT entityRef = {{0}};
    ClAmsCompT *comp = NULL;
    ClBoolT scFailover = CL_FALSE;
    ClRcT rc = CL_OK;

    AMS_FUNC_ENTER(("\n"));

    AMS_CHECK_CSI ( csi = invocationData->csi );

    if(invocationData->cmd != CL_AMS_CSI_SET_CALLBACK
       &&
       invocationData->cmd != CL_AMS_CSI_QUIESCING_CALLBACK)
    {
        goto out;
    }

    memset(&entityRef,0,sizeof(entityRef));
    memcpy(&entityRef.entity.name,&invocationData->compName,
           sizeof(entityRef.entity.name));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;

    if( (rc = clAmsEntityDbFindEntity
         (&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
          &entityRef)) != CL_OK)
    {
        clLogError("REPLAY", "CSI", "Unable to find Component [%.*s] in entityDB",
                   invocationData->compName.length-1, invocationData->compName.value);
        goto out;
    }

    comp = (ClAmsCompT*)entityRef.ptr;
    /*
     * Make a call to the policy Engine to replay the CSI for this 
     * pending invocation instance
     */
    if(arg)
        scFailover = *(ClBoolT*)arg;

    if( (rc = clAmsPeReplayCSI(comp,invocationData, scFailover)) != CL_OK)
    {
        clLogError("REPLAY", "CSI", "Replaying CSI for Component [%.*s] returned [%#x]",
                   comp->config.entity.name.length-1, comp->config.entity.name.value, rc);
        goto out;
    }

    clLogInfo("REPLAY", "CSI", "CSI [%.*s] Replayed for Component [%.*s]",
              csi->config.entity.name.length-1, csi->config.entity.name.value,
              comp->config.entity.name.length-1, comp->config.entity.name.value);

    out:
    return rc;
}

ClRcT
clAmsReplayAssignCSIInvocation(ClPtrT arg)
{
    AMS_FUNC_ENTER(("\n"));

    AMS_CALL ( clAmsInvocationListWalkAll(clAmsAssignCSICallback,arg,CL_TRUE) );

    return CL_OK;
    
}

static ClRcT
clAmsInvocationCSIRemoveWalkCallback(ClAmsInvocationT *pInvocation,
                                     ClPtrT pArg)
{
    ClRcT rc = CL_OK;
    ClAmsCSIRemoveInvocationT *pCSIRemoveInvocation = pArg;
    ClAmsCSIT *csi = NULL;
    ClAmsCompT *comp = NULL;
    ClAmsSUT *su = NULL;
    ClAmsNodeT *node = NULL;
    ClAmsEntityRefT entityRef;
    ClInt32T currentIndex = pCSIRemoveInvocation->nr;

    if(pInvocation->cmd != CL_AMS_CSI_RMV_CALLBACK )
    {
        return CL_OK;
    }
    
    AMS_CHECK_CSI( csi = pInvocation->csi );

    memset(&entityRef, 0, sizeof(entityRef));
    memcpy(&entityRef.entity.name, &pInvocation->compName,
           sizeof(entityRef.entity.name));
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                 &entityRef);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error finding component [%.*s]\n",
                                 pInvocation->compName.length,
                                 pInvocation->compName.value));
        return rc;
    }

    AMS_CHECK_COMP( comp = (ClAmsCompT*)entityRef.ptr);

    AMS_CHECK_SU( su = (ClAmsSUT*)comp->config.parentSU.ptr );

    AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr);

    if(pCSIRemoveInvocation->pNodeName
       &&
       strncmp(node->config.entity.name.value,
               pCSIRemoveInvocation->pNodeName->value,
               pCSIRemoveInvocation->pNodeName->length))
    {
        return CL_OK;
    }

    if(!(currentIndex & 3))
    {
        pCSIRemoveInvocation->pInvocations = clHeapRealloc(
                                                           pCSIRemoveInvocation->
                                                           pInvocations,
                                                           sizeof(ClAmsInvocationT)
                                                           *
                                                           (currentIndex + 4));
        CL_ASSERT(pCSIRemoveInvocation->pInvocations != NULL);
    }
    clLogDebug("INVOCATION", "RMV", "Invocation [%#llx] pending for component [%s], csi [%s]",
               pInvocation->invocation, pInvocation->compName.value, pInvocation->csiName.value);
    memcpy(pCSIRemoveInvocation->pInvocations + currentIndex,
           pInvocation, sizeof(ClAmsInvocationT));

    ++pCSIRemoveInvocation->nr;

    return CL_OK;
}

ClRcT 
clAmsGetCSIRemoveInvocations(ClNameT *pNodeName,
                             ClAmsInvocationT **ppInvocations,
                             ClInt32T *pNumInvocations)
{
    ClRcT rc = CL_OK;
    ClAmsInvocationT *pInvocations = NULL;
    ClAmsCSIRemoveInvocationT invocation = { 
                                             .nr = 0,
                                             .pInvocations = pInvocations,
                                             .pNodeName = pNodeName,
    };

    rc = clAmsInvocationListWalkAll(clAmsInvocationCSIRemoveWalkCallback,
                                    (ClPtrT)&invocation,
                                    CL_FALSE);

    *ppInvocations = invocation.pInvocations;
    *pNumInvocations = invocation.nr;

    return rc;
                                    
}


ClRcT
clAmsCCBOpListInstantiate(
        CL_OUT  ClCntHandleT  *ccbOpListHandle )

{


    AMS_CHECKPTR (!ccbOpListHandle);

    return clCntLlistCreate(
            ClAmsCntListParams[0].cntListKeyCompareCallback,
            ClAmsCntListParams[0].cntListDeleteCallback,
            ClAmsCntListParams[0].cntListDeleteCallback,
            CL_CNT_NON_UNIQUE_KEY,
            ccbOpListHandle) ;


    return CL_OK;
}

ClRcT
clAmsCCBOpListTerminate(
        CL_IN  ClCntHandleT  *ccbOpListHandle )
{

    ClRcT  rc = CL_OK;

    AMS_CHECKPTR ( !ccbOpListHandle );

    /*
     * First delete each node in the list
     */ 
    
    AMS_CALL (clCntAllNodesDelete(*ccbOpListHandle)) ;

    /*
     * Then deletes the list container
     */ 
    
    clCntDelete(*ccbOpListHandle);

    *ccbOpListHandle = 0;

    return rc;

}


void
clAmsCCBDeleteCallback(
        CL_IN  ClCntKeyHandleT  key,
        CL_IN  ClCntDataHandleT  data )
{

    clAmsMgmtCCBOperationDataT  *opData = NULL;

    /*
     * Delete the memory associated with CCB operations
     */

    opData = (clAmsMgmtCCBOperationDataT *)data;

    if(!opData)
        return;

    if(!opData->payload)
        goto out;

    switch (opData->opId)
    {

        case CL_AMS_MGMT_CCB_OPERATION_CREATE :
        case CL_AMS_MGMT_CCB_OPERATION_DELETE :
        case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST :
        case CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP :
        case CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP :
        case CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY:
        case CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST:
        case CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST:
            {
                break;
            }
    

         case CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG:
            {

                clAmsMgmtCCBEntitySetConfigRequestT  *req = 
                    (clAmsMgmtCCBEntitySetConfigRequestT *)opData->payload;
                clAmsFreeMemory(req->entityConfig);
                break;
            }


      default: 
            { 
                AMS_LOG(CL_DEBUG_ERROR, ("invalid ccb operation id [%d]\n", opData->opId));
                break;
            }
    }

    clAmsFreeMemory (opData->payload);

    out:
    clAmsFreeMemory (opData);
}


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


ClRcT
clAmsCCBAddOperation(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClPtrT  element,
        CL_IN  ClCntKeyHandleT  key )

{

    ClRcT  rc = CL_OK;
    AMS_CHECK_RC_ERROR ( clCntNodeAdd(
                listHandle,
                key, 
                (ClCntDataHandleT)element,
                NULL) );
    exitfn:
    return rc;
}



/*
 * clAmsCCBDeleteOperation
 * -------------------
 *
 *  Deleted a operation from the operation list corresponding to operation list
 *  for the CCB
 *
 *
 * @param
 *  listHandle                 - handle of the entity list 
 *  nodeHandle                 - handle corresponding to the element to be deleted
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClRcT
clAmsCCBDeleteOperation(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClCntNodeHandleT  nodeHandle )

{
    AMS_CALL ( clCntNodeDelete( listHandle, nodeHandle) );

    return CL_OK;
}


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
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClPtrT
clAmsCCBGetFirstElement(
        CL_IN  ClCntHandleT  listHandle,
        CL_OUT  ClCntNodeHandleT  *nodeHandle )

{

    ClRcT  rc = CL_OK;
    ClRcT  rc2 = CL_OK;
    ClPtrT  *element = NULL;

    if ( !listHandle || !nodeHandle ) 
    {
            AMS_LOG(CL_DEBUG_ERROR,                                         
            ("ALERT [%s:%d] : Null Pointer\n",__FUNCTION__, __LINE__));
            return NULL;
    }

    rc = clCntFirstNodeGet( listHandle, nodeHandle);

    if ( rc == CL_OK )
    {
         rc2 = clCntNodeUserDataGet(
                 listHandle,
                 *nodeHandle,
                 (ClCntDataHandleT *)&element);

    }

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node of list. "
                 "Returning NULL..\n", rc));
    }

    if ( (rc2 != CL_OK) && (CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST) ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node data in "
                 "list. Returning NULL..\n", rc2));
    }

    return element;

}


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
 *
 * @returns
 *   CL_OK                      - Operation successful
 *
 */


ClPtrT
clAmsCCBGetNextElement(
        CL_IN  ClCntHandleT  listHandle,
        CL_IN  ClCntNodeHandleT  nodeHandle, 
        CL_OUT  ClCntNodeHandleT *nextNodeHandle )

{

    ClRcT  rc = CL_OK;
    ClRcT  rc2 = CL_OK;
    ClPtrT *element = NULL;

    if ( !listHandle || !nodeHandle || !nextNodeHandle ) 
    {
            AMS_LOG(CL_DEBUG_ERROR,                                         
            ("ALERT [%s:%d] : Null Pointer\n",__FUNCTION__, __LINE__));
            return NULL;
    }

    rc = clCntNextNodeGet(listHandle, nodeHandle, nextNodeHandle);

    if ( rc == CL_OK )
    {
         rc2 = clCntNodeUserDataGet(
                 listHandle,
                 *nextNodeHandle,
                 (ClCntDataHandleT *)&element);

    }

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node of list. "
                 "Returning NULL..\n", rc));
    }

    if ( (rc2 != CL_OK) && (CL_GET_ERROR_CODE(rc2) != CL_ERR_NOT_EXIST) ) 
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("ERROR: Return code [0x%x] in finding first node data in "
                 "list. Returning NULL..\n", rc2));
    }

    return element;

}

ClRcT
clAmsCheckPendingNodeFailFast(
        CL_IN  ClAmsT  *ams )
{

    ClAmsEntityDbT  *nodeDb = NULL;

    AMS_CHECKPTR (!ams);

    nodeDb = &(ams->db.entityDb[CL_AMS_ENTITY_TYPE_NODE]);

    AMS_CHECKPTR (!nodeDb);

    if ( nodeDb->numEntities )
    {
        AMS_CALL( clAmsEntityDbWalk(nodeDb, clAmsInitiateNodeFailFast) ); 
    }

    return CL_OK;
}


ClRcT
clAmsInitiateNodeFailFast(
        CL_IN  ClAmsEntityT  *entity )
{
    ClRcT rc;
    ClAmsNodeT  *node = (ClAmsNodeT *)entity;

    AMS_CHECKPTR (!node);

    if ( ( node->status.isClusterMember == CL_TRUE ) &&
            ( node->status.recovery == CL_AMS_RECOVERY_NODE_FAILFAST) )
    {
        AMS_CALL ( _clAmsSAComponentErrorReport( &(node->config.entity.name), 
                                                 0, CL_AMS_RECOVERY_NODE_FAILFAST, 0, 0) );
    }

    ClIocAddressT  iocAddress;
    ClIocNodeAddressT  nodeAddress;

    clCpmIocAddressForNodeGet(node->config.entity.name,&iocAddress);
    rc = clCpmMasterAddressGet(&nodeAddress);
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Function [clCpmMasterAddressGet] returned \
                    [0x%x]\n",rc));                    
        return rc;
    }

    if ( !memcmp (&iocAddress.iocPhyAddress.nodeAddress,&nodeAddress,
                sizeof(ClIocNodeAddressT)) )
    {
        return CL_OK;
    }

    return CL_OK;
}

ClBoolT
clAmsIsNodeActiveSC(
        CL_IN  ClNameT  *nodeName )
{

    ClIocNodeAddressT  nodeAddress ;
    ClIocAddressT  iocAddress ;
    ClRcT  rc = CL_OK;

    if ( nodeName == NULL )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("ALERT [%s:%d] : nodeName is NULL \n", \
                    __FUNCTION__, __LINE__)); 
        return CL_FALSE;
    }


    rc = clCpmIocAddressForNodeGet( *nodeName, &iocAddress);

    if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Function [clCpmIocAddressForNodeGet] \
                    returned [0x%x]\n",rc));                    
        return CL_FALSE;
    }

    rc = clCpmMasterAddressGet(&nodeAddress);

    if ( rc != CL_OK )
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Function [clCpmMasterAddressGet] returned \
                    [0x%x]\n",rc));                    
        return CL_FALSE;
    }

    if ( !memcmp (&iocAddress.iocPhyAddress.nodeAddress,&nodeAddress,
                sizeof(ClIocNodeAddressT)) )
    {
        return CL_TRUE;
    }

    return CL_FALSE;

}

static ClRcT
clAmsEntityDeleteSISURefs(ClAmsEntityT *entity, ClPtrT arg)
{
    ClAmsSIT *si = (ClAmsSIT*)entity;
    ClAmsSISUDeleteRefT *ref = arg;
    ClAmsEntityRefT *suRef = NULL;
    ClAmsEntityRefT *entityRef =NULL;
    ClAmsSUT *su = NULL;
    ClCntKeyHandleT key = 0;

    if(!arg) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    key = ref->key;
    entityRef =  ref->suRef;
    if(!entityRef) return CL_AMS_RC(CL_ERR_INVALID_STATE);

    su = (ClAmsSUT*)entityRef->ptr;
    if(!su) return CL_AMS_RC(CL_ERR_INVALID_STATE);
        
    if(clAmsEntityListFindEntityRef(&si->config.suList,
                                    entityRef, key,
                                    &suRef) == CL_OK)
    {
        clAmsDeleteFromEntityList(&si->config.entity,
                                  &su->config.entity,
                                  CL_AMS_SI_CONFIG_SU_RANK_LIST);
    }
    return CL_OK;
}

ClRcT
clAmsEntityDeleteRefs(ClAmsEntityRefT *entityRef)
{
    if(!entityRef) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    switch(entityRef->entity.type)
    {
    case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT *sg = (ClAmsSGT*)entityRef->ptr;
            ClAmsEntityRefT *ref = NULL;
            for(ref = clAmsEntityListGetFirst(&sg->config.suList);
                ref != NULL;
                ref = clAmsEntityListGetNext(&sg->config.suList, ref))
            {
                ClAmsSUT *su = (ClAmsSUT*)ref->ptr;
                su->config.parentSG.ptr = NULL;
                clNameSet(&su->config.parentSG.entity.name,
                          "ParentSGUndefined");
                ++su->config.parentSG.entity.name.length;
            }

            for(ref = clAmsEntityListGetFirst(&sg->config.siList);
                ref != NULL;
                ref = clAmsEntityListGetNext(&sg->config.siList, ref))
            {
                ClAmsSIT *si = (ClAmsSIT*)ref->ptr;
                si->config.parentSG.ptr = NULL;
                clNameSet(&si->config.parentSG.entity.name,
                          "ParentSGUndefined");
                ++si->config.parentSG.entity.name.length;
            }
        }
        break;

    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
            ClAmsSGT *sg = (ClAmsSGT*)si->config.parentSG.ptr;
            ClAmsEntityRefT *ref = NULL;

            if(sg)
            {
                /*
                 *Delete the SI from the parent SG list.
                 */
                clAmsDeleteFromEntityList(&sg->config.entity, &si->config.entity,
                                          CL_AMS_SG_CONFIG_SI_LIST);
            }
            /*
             * Unset the CSI references to this SI.
             */
            for(ref = clAmsEntityListGetFirst(&si->config.csiList);
                ref != NULL;
                ref = clAmsEntityListGetNext(&si->config.csiList, ref))
            {
                ClAmsCSIT *csi = (ClAmsCSIT*)ref->ptr;
                csi->config.parentSI.ptr = NULL;
                clNameSet(&csi->config.parentSI.entity.name,
                          "ParentSIUndefined");
                ++csi->config.parentSI.entity.name.length;
            }

            /*
             * Delete this SI from the dependents and dependency list.
             */
            for(ref = clAmsEntityListGetFirst(&si->config.siDependenciesList);
                ref != NULL;
                )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&si->config.siDependenciesList, ref);
                ClAmsSIT *dependencySI = (ClAmsSIT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependencySI->config.entity, &si->config.entity,
                                          CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST);
                ref = nextRef;
            }
            
            for(ref = clAmsEntityListGetFirst(&si->config.siDependentsList);
                ref != NULL;
                )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&si->config.siDependentsList, ref);
                ClAmsSIT *dependentSI = (ClAmsSIT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependentSI->config.entity, &si->config.entity,
                                          CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST);
                ref = nextRef;
            }
        }
        break;

    case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *node = (ClAmsNodeT*)entityRef->ptr;
            ClAmsEntityRefT *ref = NULL;
            /*
             * Unset SU references to this node.
             */
            for(ref = clAmsEntityListGetFirst(&node->config.suList);
                ref != NULL;
                ref = clAmsEntityListGetNext(&node->config.suList, ref))
            {
                ClAmsSUT *su = (ClAmsSUT*)ref->ptr;
                su->config.parentNode.ptr = NULL;
                clNameSet(&su->config.parentNode.entity.name,
                          "ParentNodeUndefined");
                ++su->config.parentNode.entity.name.length;
            }

            /*
             * Delete from node dependency and dependents list.
             */

            for(ref = clAmsEntityListGetFirst(&node->config.nodeDependenciesList);
                ref != NULL; )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&node->config.nodeDependenciesList, ref);
                ClAmsNodeT *dependencyNode = (ClAmsNodeT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependencyNode->config.entity,
                                          &node->config.entity, CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST);
                ref = nextRef;
            }

            for(ref = clAmsEntityListGetFirst(&node->config.nodeDependentsList);
                ref != NULL; )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&node->config.nodeDependentsList, ref);
                ClAmsNodeT *dependentNode = (ClAmsNodeT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependentNode->config.entity, &node->config.entity,
                                          CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST);
                ref = nextRef;
            }
        }
        break;

    case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT *su = (ClAmsSUT*)entityRef->ptr;
            ClAmsSGT *sg = (ClAmsSGT*)su->config.parentSG.ptr;
            ClAmsNodeT *node = (ClAmsNodeT*)su->config.parentNode.ptr;
            ClAmsEntityRefT *ref = NULL;
            ClCntKeyHandleT key = 0;            
            /*
             * Delete from SG list.
             */
            if(sg)
            {
                clAmsDeleteFromEntityList(&sg->config.entity,
                                          &su->config.entity,
                                          CL_AMS_SG_CONFIG_SU_LIST);
            }

            if(clAmsEntityRefGetKey(
                                    &su->config.entity,
                                    su->config.rank,
                                    &key,
                                    CL_TRUE) == CL_OK)
            {
                /*
                 * Delete from SI su rank list.
                 */
                if(sg)
                {
                    for(ref = clAmsEntityListGetFirst(&sg->config.siList);
                        ref != NULL;
                        ref = clAmsEntityListGetNext(&sg->config.siList, ref))
                    {
                        ClAmsSIT *si = (ClAmsSIT*)ref->ptr;
                        ClAmsEntityRefT *suRef = NULL;
    
                        if(clAmsEntityListFindEntityRef(&si->config.suList,
                                                        entityRef, key,
                                                        &suRef) == CL_OK)
                        {
                            clAmsDeleteFromEntityList(&si->config.entity,
                                                      &su->config.entity,
                                                      CL_AMS_SI_CONFIG_SU_RANK_LIST);
                        }
                    }
                }
                else
                {
                    /*
                     * Walk through the SI list and knock the SU off if its in the rank list of the SI.
                     */
                    ClAmsSISUDeleteRefT arg = {0};
                    arg.key = key;
                    arg.suRef = entityRef;
                    clAmsEntityDbWalkExtended(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                              clAmsEntityDeleteSISURefs, (ClPtrT)&arg);
                }
                       
            }
            
            /*
             * Delete from Node list.
             */
            if(node)
            {
                clAmsDeleteFromEntityList(&node->config.entity,
                                          &su->config.entity,
                                          CL_AMS_NODE_CONFIG_SU_LIST);
            }

            /*
             * Unset component references.
             */
            for(ref = clAmsEntityListGetFirst(&su->config.compList);
                ref != NULL; 
                ref = clAmsEntityListGetNext(&su->config.compList, ref))
            {
                ClAmsCompT *comp = (ClAmsCompT*)ref->ptr;
                comp->config.parentSU.ptr = NULL;
                clNameSet(&comp->config.parentSU.entity.name,
                          "ParentSUUndefined");
                ++comp->config.parentSU.entity.name.length;
            }
        }

        break;

    case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT *comp = (ClAmsCompT*)entityRef->ptr;
            ClAmsSUT *su = (ClAmsSUT*)comp->config.parentSU.ptr;
            
            if(su)
            {
                /*
                 *Delete from SU list.
                 */
                clAmsDeleteFromEntityList(&su->config.entity,
                                          &comp->config.entity,
                                          CL_AMS_SU_CONFIG_COMP_LIST);
            }
        }
        break;

    case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsCSIT *csi = (ClAmsCSIT*)entityRef->ptr;
            ClAmsSIT *si = (ClAmsSIT*)csi->config.parentSI.ptr;
            ClAmsEntityRefT *ref = NULL;

            if(si)
            {
                /*
                 *Delete from SI list.  
                 */
                clAmsDeleteFromEntityList(&si->config.entity,
                                          &csi->config.entity,
                                          CL_AMS_SI_CONFIG_CSI_LIST);
            }

            /*
             * Delete from CSI dependent/dependencies list.
             */
            for(ref = clAmsEntityListGetFirst(&csi->config.csiDependenciesList);
                ref != NULL;
                )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&csi->config.csiDependenciesList, ref);
                ClAmsCSIT *dependencyCSI = (ClAmsCSIT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependencyCSI->config.entity, &csi->config.entity,
                                          CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST);
                ref = nextRef;
            }
            
            for(ref = clAmsEntityListGetFirst(&csi->config.csiDependentsList);
                ref != NULL;
                )
            {
                ClAmsEntityRefT *nextRef = clAmsEntityListGetNext(&csi->config.csiDependentsList, ref);
                ClAmsCSIT *dependentCSI = (ClAmsCSIT*)ref->ptr;
                clAmsDeleteFromEntityList(&dependentCSI->config.entity, &csi->config.entity,
                                          CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST);
                ref = nextRef;
            }

        }
        break;

    default:
        break;
    }

    return CL_OK;
}

static __inline__ ClBoolT clAmsMatchCSIType(ClAmsCSIT *csi, 
                                            ClNameT *csiType)
{
    if(csi)
    {
        return (csi->config.type.length == csiType->length &&
                !strncmp(csi->config.type.value, csiType->value, csi->config.type.length)) ?
            CL_TRUE : CL_FALSE;
    }
    return CL_FALSE;
}

static ClRcT
clAmsMatchCSITypeCallback(ClAmsEntityT *entity, ClPtrT userArg)
{
    ClAmsCSIT *csi = (ClAmsCSIT*)entity;
    ClNameT *csiType = userArg;
    if(clAmsMatchCSIType(csi, csiType))
        return CL_AMS_RC(CL_ERR_ALREADY_EXIST);
    return CL_OK;
}

/*
 * Returns true if exists. 
 */
ClRcT clAmsFindCSIType(ClAmsCSIT *csi, ClNameT *csiType)
{
    ClRcT rc = CL_OK;

    if(!csiType) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(csi) return clAmsMatchCSIType(csi, csiType);

    rc = clAmsEntityDbWalkExtended(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_CSI],
                                   clAmsMatchCSITypeCallback, (ClPtrT)csiType);
    
    return CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST ? CL_OK : CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT clAmsSGFailoverHistoryDelete(ClAmsSGT *sg)
{
    while(!CL_LIST_HEAD_EMPTY(&sg->status.failoverHistory))
    {
        ClListHeadT *head = sg->status.failoverHistory.pNext;
        ClAmsSGFailoverHistoryT *history = CL_LIST_ENTRY(head, ClAmsSGFailoverHistoryT, list);
        clListDel(&history->list);
        if(history->timer)
        {
            clTimerStop(history->timer);
            clTimerDelete(&history->timer);
        }
        clHeapFree(history);
    }
    sg->status.failoverHistoryCount = 0;
    sg->status.failoverHistoryIndex = 0;
    return CL_OK;
}

/*
 * Go through the failover history timer unpacked and re-configure them.
 * Must be called only after a failover/unpack on the slave taking over as master
 */

ClRcT clAmsSGFailoverHistoryConfigure(ClAmsSGT *sg)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *next = NULL;
    ClRcT rc = CL_OK;

    for(iter = sg->status.failoverHistory.pNext; iter != &sg->status.failoverHistory; iter = next)
    {
        ClAmsSGFailoverHistoryT *history = CL_LIST_ENTRY(iter, ClAmsSGFailoverHistoryT, list);
        next = iter->pNext;
        if(!history->timer)
        {
            clLogWarning("FAILOVER", "HISTORY", "Skipping unconfigured timer in failover history");
            clListDel(iter);
            clHeapFree(history);
            --sg->status.failoverHistoryCount;
            continue;
        }
        rc = clTimerClusterConfigure(&history->timer);
        if(rc != CL_OK)
        {
            clLogError("FAILOVER", "HISTORY", "Skipping clustered failover timer configure for SG [%s] "
                       "because of error [%#x]", sg->config.entity.name.value, rc);
            clListDel(iter);
            clTimerClusterFree(history->timer);
            clHeapFree(history);           
            --sg->status.failoverHistoryCount;
            continue;
        }
        else
        {
            clLogNotice("FAILOVER", "HISTORY", "Cluster timer [%d] configured for SG [%s]",
                        history->index, sg->config.entity.name.value);
        }
    }

    return CL_OK;
}

static ClRcT clAmsSGFailoverHistoryUpdate(ClAmsSGT *sg, ClAmsSGFailoverHistoryT *history, ClBoolT *pRecover)
{
    ClRcT rc = CL_OK;
    ClBoolT recovery = CL_FALSE;

    if(!sg || !history || !pRecover)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    if(++history->numFailovers >= sg->config.maxFailovers)
    {
        clLogDebug("FAILOVER", "HISTORY", "Recovery enabled for SG [%s] as failover history index [%d] "
                   "exceeds configured max failovers [%d]",
                   sg->config.entity.name.value, history->index, sg->config.maxFailovers);
        recovery = CL_TRUE;
    }
    else
    {
        clLogDebug("FAILOVER", "HISTORY", "Failover count at [%d] for index [%d], SG [%s]",
                   history->numFailovers, history->index, sg->config.entity.name.value);
    }

    /*
     * If we hit the limit for this history/interval, then remove this history
     * from the list.
     */
    if(recovery == CL_TRUE)
    {
        if(history->timer)
        {
            clTimerStop(history->timer);
            clTimerDelete(&history->timer);
        }
        clListDel(&history->list);
        clHeapFree(history);
        *pRecover = recovery;
    }

    return rc;
}

/*
 * Cascade the SGs failover history.
 */
ClRcT clAmsSGFailoverHistoryCascade(ClAmsNodeT *node, ClAmsCompT **ppFaultyComp, ClBoolT *pRecover)
{
    ClListHeadT *iter = NULL;
    ClListHeadT *next = NULL;
    ClAmsSGT *sg = NULL;
    ClAmsSUT *su = NULL;
    ClAmsCompT *faultyComp = NULL;
    ClAmsSGFailoverHistoryT *newHistory = NULL;
    ClAmsSGFailoverHistoryKeyT *historyKey = NULL;
    ClTimerTimeOutT timeout = {0};
    ClBoolT recovery = CL_FALSE;
    ClRcT rc = CL_OK;
    
    if(!ppFaultyComp) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    faultyComp = *ppFaultyComp;

    if(!faultyComp)
    {
        ClAmsEntityRefT *suRef = NULL;
        for(suRef = clAmsEntityListGetFirst(&node->config.suList);
            suRef != NULL;
            suRef = clAmsEntityListGetNext(&node->config.suList, suRef))
        {
            ClAmsEntityRefT *compRef = NULL;
            su = (ClAmsSUT*)suRef->ptr;
            for(compRef = clAmsEntityListGetFirst(&su->config.compList);
                compRef != NULL;
                compRef = clAmsEntityListGetNext(&su->config.compList, compRef))
            {
                ClAmsCompT *comp = (ClAmsCompT*)compRef->ptr;
                if(comp->status.recovery == CL_AMS_RECOVERY_NODE_FAILOVER
                   ||
                   comp->status.recovery == CL_AMS_RECOVERY_NODE_FAILFAST)
                {
                    faultyComp = comp;
                    goto found;
                }
            }
        }
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    found:
    su = (ClAmsSUT*)faultyComp->config.parentSU.ptr;
    if(!su) 
        return CL_AMS_RC(CL_ERR_INVALID_STATE);

    sg = (ClAmsSGT*)su->config.parentSG.ptr;
    if(!sg || !pRecover)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!sg->config.maxFailovers) return rc;

    newHistory = clHeapCalloc(1, sizeof(*newHistory));
    CL_ASSERT(newHistory != NULL);
    /*
     * The history key below would be replicated across cluster with clustered timers.
     */
    historyKey = clHeapCalloc(1, sizeof(*historyKey));
    CL_ASSERT(historyKey != NULL);

    timeout.tsSec = 0;
    timeout.tsMilliSec = sg->config.failoverDuration;
    memcpy(&historyKey->entity, &sg->config.entity, sizeof(historyKey->entity));
    historyKey->index = sg->status.failoverHistoryIndex++;
    memcpy(&newHistory->entity, &sg->config.entity, sizeof(newHistory->entity));
    newHistory->index = historyKey->index;
    rc = clTimerCreateAndStartCluster(timeout, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT,
                                      (void*)historyKey, (ClUint32T)sizeof(*historyKey), &newHistory->timer);
    if(rc != CL_OK)
    {
        goto out_free;
    }

    /*
     * Add to the sgs failover history table.
     */
    clListAddTail(&newHistory->list, &sg->status.failoverHistory);
    ++sg->status.failoverHistoryCount;

    for(iter = sg->status.failoverHistory.pNext; iter != &newHistory->list; iter = next)
    {
        ClAmsSGFailoverHistoryT *history = CL_LIST_ENTRY(iter, ClAmsSGFailoverHistoryT, list);
        next = iter->pNext;
        /*
         * Update the history counters.
         */
        clAmsSGFailoverHistoryUpdate(sg, history, &recovery);
    }

    *pRecover = recovery;
    *ppFaultyComp = faultyComp;
    return rc;

    out_free:
    if(historyKey)
        clHeapFree(historyKey);
    if(newHistory)
        clHeapFree(newHistory);
    return rc;
}

/*
 * Called under the ams db lock.
 */
ClRcT clAmsFailoverHistoryFind(ClAmsEntityT *entity, ClUint32T index,
                               ClAmsSGT **ppSG, ClAmsSGFailoverHistoryT **ppHistory)
{
    ClAmsEntityRefT entityRef = {{0}};
    ClListHeadT *iter = NULL;
    ClAmsSGT *sg = NULL;
    ClRcT rc = CL_OK;

    if(!entity) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    memcpy(&entityRef.entity, entity, sizeof(entityRef.entity));
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SG],
                                 &entityRef);
    if(rc != CL_OK)
    {
        clLogError("FAILOVER", "HISTORY", "Unable to locate sg [%s]", entity->name.value);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    sg = (ClAmsSGT*)entityRef.ptr;
    if(ppSG)
        *ppSG = sg;

    CL_LIST_FOR_EACH(iter, &sg->status.failoverHistory)
    {
        ClAmsSGFailoverHistoryT *failoverHistory = CL_LIST_ENTRY(iter, ClAmsSGFailoverHistoryT, list);
        if(failoverHistory->index == index)
        {
            if(ppHistory)
                *ppHistory = failoverHistory;
            return CL_OK;
        }
    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

static ClRcT clAmsDBGetEntity(ClAmsEntityTypeT type, ClAmsEntityListTypeT listType, ClBufferHandleT msg)
{
    ClRcT rc;
    ClAmsEntityBufferT buffer = {0};
    ClAmsEntityRefT entityRef;
    ClAmsEntityT entity = {0};
    ClUint32T i;

    rc = VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, msg, 0);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    rc = clAmsGetEntityList(&entity, listType, &buffer);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    rc = VDECL_VER(clXdrMarshallClAmsEntityBufferT, 4, 0, 0)(&buffer, msg, 0);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    /*
     * For each node in the list, marshall config and status.
     */
    for(i = 0; i < buffer.count; ++i)
    {
        entityRef.ptr = NULL;
        memcpy(&entityRef.entity, &buffer.entity[i], sizeof(entityRef.entity));
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[type], 
                                     &entityRef);
        if(rc != CL_OK)
        {
            clLogError("GET", "CONFIG", "Unable to get config for %s [%s]",
                       CL_AMS_STRING_ENTITY_TYPE(type) ,buffer.entity[i].name.value);
            goto out_free;
        }

        switch(type)
        {
        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeT *node = (ClAmsNodeT*)entityRef.ptr;
                rc = VDECL_VER(clXdrMarshallClAmsNodeConfigT, 4, 0, 0)(&node->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsNodeStatusT, 4, 0, 0)(&node->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
            }
            break;

        case CL_AMS_ENTITY_TYPE_SU:
            {
                ClAmsEntityRefT *eRef = NULL;
                ClAmsSUT *su = (ClAmsSUT*)entityRef.ptr;
                rc = VDECL_VER(clXdrMarshallClAmsSUConfigT, 4, 0, 0)(&su->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsSUStatusT, 4, 0, 0)(&su->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                listType = CL_AMS_SU_STATUS_SI_EXTENDED_LIST;
                rc = VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = clXdrMarshallClUint32T(&su->status.siList.numEntities, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                for(eRef = clAmsEntityListGetFirst(&su->status.siList); eRef;
                    eRef = clAmsEntityListGetNext(&su->status.siList, eRef))
                {
                    ClAmsSUSIExtendedRefT targetRef = {{{0}}};
                    ClAmsSUSIRefT *suSIRef = (ClAmsSUSIRefT*)eRef;
                    ClAmsSIT *si = (ClAmsSIT*)eRef->ptr;
                    memcpy(&targetRef.entityRef, &suSIRef->entityRef, sizeof(targetRef.entityRef));
                    targetRef.haState = suSIRef->haState;
                    targetRef.numActiveCSIs = suSIRef->numActiveCSIs;
                    targetRef.numStandbyCSIs = suSIRef->numStandbyCSIs;
                    targetRef.numQuiescedCSIs = suSIRef->numQuiescedCSIs;
                    targetRef.numQuiescingCSIs = suSIRef->numQuiescingCSIs;
                    targetRef.numCSIs = si->config.numCSIs;
                    targetRef.rank = suSIRef->rank;
                    targetRef.pendingInvocations = clAmsInvocationsPendingForSI(si, su);
                    rc = VDECL_VER(clXdrMarshallClAmsSUSIExtendedRefT, 4, 0, 0)(&targetRef, msg, 0);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                }

            }
            break;

        case CL_AMS_ENTITY_TYPE_SG:
            {
                ClAmsSGT *sg = (ClAmsSGT*)entityRef.ptr;
                rc = VDECL_VER(clXdrMarshallClAmsSGConfigT, 5, 0, 0)(&sg->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsSGStatusT, 4, 1, 0)(&sg->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
            }
            break;

        case CL_AMS_ENTITY_TYPE_SI:
            {
                ClAmsSIT *si = (ClAmsSIT *)entityRef.ptr;
                ClAmsEntityRefT *eRef = NULL;
                rc = VDECL_VER(clXdrMarshallClAmsSIConfigT, 4, 0, 0)(&si->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsSIStatusT, 4, 0, 0)(&si->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                listType = CL_AMS_SI_STATUS_SU_EXTENDED_LIST;
                rc = VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = clXdrMarshallClUint32T(&si->status.suList.numEntities, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                for(eRef = clAmsEntityListGetFirst(&si->status.suList); eRef;
                    eRef = clAmsEntityListGetNext(&si->status.suList, eRef))
                {
                    ClAmsSISUExtendedRefT targetRef = {{{0}}};
                    ClAmsSISURefT *siSURef = (ClAmsSISURefT*)eRef;
                    memcpy(&targetRef.entityRef, &siSURef->entityRef, sizeof(targetRef.entityRef));
                    targetRef.rank = siSURef->rank;
                    targetRef.haState = siSURef->haState;
                    targetRef.pendingInvocations = clAmsInvocationsPendingForSU((ClAmsSUT*)eRef->ptr);
                    rc = VDECL_VER(clXdrMarshallClAmsSISUExtendedRefT, 4, 0, 0)(&targetRef, msg, 0);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                }
            }
            break;
            
        case CL_AMS_ENTITY_TYPE_CSI:
            {
                ClAmsCSIT *csi = (ClAmsCSIT *)entityRef.ptr;
                rc = VDECL_VER(clXdrMarshallClAmsCSIConfigT, 4, 0, 0)(&csi->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsCSIStatusT, 4, 0, 0)(&csi->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
            }
            break;

        case CL_AMS_ENTITY_TYPE_COMP:
            {
                ClAmsCompT *comp = (ClAmsCompT*)entityRef.ptr;
                ClAmsEntityRefT *eRef;
                ClUint32T numCSIOffset = 0, numCSIs = 0;
                rc = VDECL_VER(clXdrMarshallClAmsCompConfigT, 4, 0, 0)(&comp->config, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                rc = VDECL_VER(clXdrMarshallClAmsCompStatusT, 5, 1, 0)(&comp->status, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                /*
                 * Also stuff the comp status csi list.
                 */
                listType = CL_AMS_COMP_STATUS_CSI_LIST;
                rc = VDECL_VER(clXdrMarshallClAmsEntityListTypeT, 4, 0, 0)(&listType, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                clBufferWriteOffsetGet(msg, &numCSIOffset);
                rc = clXdrMarshallClUint32T(&comp->status.csiList.numEntities, msg, 0);
                if(rc != CL_OK)
                {
                    goto out_free;
                }
                for(eRef = clAmsEntityListGetFirst(&comp->status.csiList); eRef; 
                    eRef = clAmsEntityListGetNext(&comp->status.csiList, eRef))
                {
                    ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT*)eRef;
                    if(!csiRef->activeComp) 
                    {
                        clLogNotice("DBA", "GET", "Skipping a csiref with no active comp "
                                    "for component [%s]", comp->config.entity.name.value);
                        continue;
                    }
                    ++numCSIs;
                    rc = VDECL_VER(clXdrMarshallClAmsCompCSIRefT, 4, 0, 0)(csiRef, msg, 0);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                }
                /*
                 * Unlikely branch;
                 * Patch the num csi refs for the component in case of a mismatch
                 */
                if(numCSIs != comp->status.csiList.numEntities)
                {
                    ClUint32T curOffset = 0;
                    rc = clBufferWriteOffsetGet(msg, &curOffset);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                    rc = clBufferWriteOffsetSet(msg, numCSIOffset, CL_BUFFER_SEEK_SET);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                    rc = clXdrMarshallClUint32T(&numCSIs, msg, 0);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                    rc = clBufferWriteOffsetSet(msg, curOffset, CL_BUFFER_SEEK_SET);
                    if(rc != CL_OK)
                    {
                        goto out_free;
                    }
                }
            }
            break;
            
        default:
            break;
        }
    }

    out_free:
    if(buffer.entity)
        clHeapFree(buffer.entity);

    return rc;
}

/*
 * Get the states of the cluster entities.
 */
ClRcT
clAmsDBGet(ClBufferHandleT msg)
{
    ClRcT rc;
    struct EntityListMap
    {
        ClAmsEntityTypeT type;
        ClAmsEntityListTypeT listType;
    }map[] = { {.type = CL_AMS_ENTITY_TYPE_NODE, .listType = CL_AMS_NODE_LIST},
               {.type = CL_AMS_ENTITY_TYPE_SU,   .listType = CL_AMS_SU_LIST},
               {.type = CL_AMS_ENTITY_TYPE_SG,   .listType = CL_AMS_SG_LIST},
               {.type = CL_AMS_ENTITY_TYPE_SI,   .listType = CL_AMS_SI_LIST},
               {.type = CL_AMS_ENTITY_TYPE_CSI,  .listType = CL_AMS_CSI_LIST},
               {.type = CL_AMS_ENTITY_TYPE_COMP, .listType = CL_AMS_COMP_LIST},
    };
    ClUint32T i;
    for(i = 0; i < sizeof(map)/sizeof(map[0]); ++i)
    {
        rc = clAmsDBGetEntity(map[i].type, map[i].listType, msg);
        if(rc != CL_OK)
        {
            goto out;
        }
    }

    out:
    return rc;
}
