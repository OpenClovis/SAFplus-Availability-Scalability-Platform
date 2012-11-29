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

#include <string.h>

#include <clDebugApi.h>
#include <clLogApi.h>

#include <clAmsMgmtDebugCli.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>
#include <clAmsErrors.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsSACommon.h>
#include <clCpmExtApi.h>
#include <clCorUtilityApi.h>
#include <clJobQueue.h>

#undef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE (1024)

#define STREQ(a, b) (strncasecmp((a), (b), strlen((b))) == 0)

#define USAGE_STRING "amsMgmt create|delete|migrate sg|si|csi|node|su|comp "    \
    "entity-name\n"                                                     \
    "\n"                                                                \
    "or\n"                                                              \
    "\n"                                                                \
    "amsMgmt set sg|si|csi|node|su|comp attribute [value1 ]+ entity-name\n" \
    "\n"                                                                \
    "For the list of attributes that can be set please see "            \
    "the OpenClovis SDK documentation."

static ClJobQueueT gAmsMgmtAdminPool;
typedef struct ClAmsMgmtAdminChangeOper
{
    ClAmsEntityT entity;
    ClAmsAdminStateT lastState;
    ClAmsAdminStateT newState;
}ClAmsMgmtAdminChangeOperT;

static void amsCliPrint(ClCharT **retStr, const ClCharT *fmt, ...)
{
    va_list args;
    ClCharT tempStr[2*CL_MAX_NAME_LENGTH] = {0};

    va_start(args, fmt);
    vsnprintf(tempStr, 2*CL_MAX_NAME_LENGTH-1, fmt, args);
    va_end(args);

    *retStr = (ClCharT *) clHeapAllocate(strlen(tempStr) + 1);
    if (!*retStr) 
    {
        clLogError("AMS", "CLI",
                   "Failed to allocate memory, error [%#x]",
                   CL_AMS_RC(CL_ERR_NO_MEMORY));
        return;
    }
    strcpy(*retStr, tempStr);
}

static void usage(ClCharT **ret)
{
    amsCliPrint(ret, USAGE_STRING);
    
    return;
}

static ClRcT amsEntityFromStrGet(const ClCharT *s, ClAmsEntityTypeT *entityType)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(entityType != NULL);

    if (STREQ(s, "sg")) *entityType = CL_AMS_ENTITY_TYPE_SG;
    else if (STREQ(s, "si")) *entityType = CL_AMS_ENTITY_TYPE_SI;
    else if (STREQ(s, "csi")) *entityType = CL_AMS_ENTITY_TYPE_CSI;
    else if (STREQ(s, "node")) *entityType = CL_AMS_ENTITY_TYPE_NODE;
    else if (STREQ(s, "su")) *entityType = CL_AMS_ENTITY_TYPE_SU;
    else if (STREQ(s, "comp")) *entityType = CL_AMS_ENTITY_TYPE_COMP;
    else goto failure;

    return CL_OK;

failure:
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

static ClRcT amsEntityCapModelFromStrGet(const ClCharT *s,
                                        ClAmsCompCapModelT *capModel)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(capModel != NULL);

    if (STREQ(s, "x_and_y")) *capModel = CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY;
    else if (STREQ(s, "x_or_y")) *capModel = CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY;
    else if (STREQ(s, "one_or_x")) *capModel = CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY;
    else if (STREQ(s, "one_or_one")) *capModel = CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY;
    else if (STREQ(s, "x_active")) *capModel = CL_AMS_COMP_CAP_X_ACTIVE;
    else if (STREQ(s, "one_active")) *capModel = CL_AMS_COMP_CAP_ONE_ACTIVE;
    else goto failure;

    return CL_OK;

failure:
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

static ClRcT amsEntityRecoveryFromStrGet(const ClCharT *s,
                                        ClAmsRecoveryT *recovery)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(recovery != NULL);

    if (STREQ(s, "no_rec")) *recovery = CL_AMS_RECOVERY_NO_RECOMMENDATION;
    else if (STREQ(s, "comp_restart")) *recovery = CL_AMS_RECOVERY_COMP_RESTART;
    else if (STREQ(s, "comp_failover")) *recovery = CL_AMS_RECOVERY_COMP_FAILOVER;
    else if (STREQ(s, "node_switchover")) *recovery = CL_AMS_RECOVERY_NODE_SWITCHOVER;
    else if (STREQ(s, "node_failover")) *recovery = CL_AMS_RECOVERY_NODE_FAILOVER;
    else if (STREQ(s, "node_failfast")) *recovery = CL_AMS_RECOVERY_NODE_FAILFAST;
    else goto failure;

    return CL_OK;

failure:
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

static ClRcT amsEntityRedundancyModelFromStrGet(const ClCharT *s,
                                               ClAmsSGRedundancyModelT
                                               *redunModel)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(redunModel != NULL);

    if (STREQ(s, "no_redundancy")) *redunModel = CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY;
    else if (STREQ(s, "twon")) *redunModel = CL_AMS_SG_REDUNDANCY_MODEL_TWO_N;
    else if (STREQ(s, "mplusn")) *redunModel = CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N;
    else goto failure;

    return CL_OK;

failure:
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

static ClRcT amsEntityLoadStrategyFromStrGet(const ClCharT *s,
                                             ClAmsSGLoadingStrategyT
                                             *loadStrategy)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(loadStrategy != NULL);

    if (STREQ(s, "least_si_per_su")) *loadStrategy = CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU;
    else if (STREQ(s, "least_su_assigned")) *loadStrategy = CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED;
    else if (STREQ(s, "least_load_per_su")) *loadStrategy = CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU;
    else if (STREQ(s, "by_si_preference")) *loadStrategy = CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE;
    else goto failure;

    return CL_OK;

failure:
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

static ClRcT amsEntityBoolFromStrGet(const ClCharT *s, ClBoolT *boolValue)
{
    CL_ASSERT(s != NULL);
    CL_ASSERT(boolValue != NULL);

    if (STREQ(s, "true")) *boolValue = CL_TRUE;
    else *boolValue = CL_FALSE;

    return CL_OK;
}

static ClRcT amsMgmtUpdateConfig(ClAmsEntityT *srcEntity,
                                 ClAmsEntityT *targetEntity,
                                 ClInt32T op)
{
    ClAmsEntityBufferT entityBuffer = {0};
    ClAmsEntityConfigT *entityConfig = NULL;
    ClUint64T bitMask = 0;
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    switch(op)
    {
    case CL_AMS_MGMT_CCB_SET_SI_CSI_LIST:
        {
            rc = clAmsMgmtGetSICSIList(gHandle, srcEntity, &entityBuffer);
            if(rc != CL_OK)
                goto out;
            break;
        }

    case CL_AMS_MGMT_CCB_SET_SU_COMP_LIST:
        {
            rc = clAmsMgmtGetSUCompList(gHandle, srcEntity, &entityBuffer);
            if(rc != CL_OK)
                goto out;
            break;
        }

    default:
        goto out;
    }

    /*
     * Check for the matching entity w.r.t the target
     */
    for(i = 0; i < entityBuffer.count; ++i)
    {
        ClAmsEntityT *entity = entityBuffer.entity+i;
        if(!memcmp(entity->name.value, targetEntity->name.value,
                   entity->name.length))
        {
            rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
            goto out_free;
        }
    }

    rc = clAmsMgmtEntityGetConfig(gHandle, srcEntity, &entityConfig);
    if(rc != CL_OK)
        goto out_free;

    switch(op)
    {
    case CL_AMS_MGMT_CCB_SET_SI_CSI_LIST:
        {
            /* 
             * update SI num config csis.
             */
            ++((ClAmsSIConfigT*)entityConfig)->numCSIs;
            bitMask = SI_CONFIG_NUM_CSIS;
            rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle, entityConfig, bitMask);
            break;
        }

    case CL_AMS_MGMT_CCB_SET_SU_COMP_LIST:
        {
            ++((ClAmsSUConfigT*)entityConfig)->numComponents;
            bitMask = SU_CONFIG_NUM_COMPONENTS;
            rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle, entityConfig, bitMask);
            break;
        }
        
    default: 
        break;
    }

    out_free:
    if(entityBuffer.entity)
        clHeapFree(entityBuffer.entity);

    if(entityConfig)
        clHeapFree(entityConfig);

    out:
    return rc;
}

static ClRcT amsMgmtEntity(const ClCharT *op,
                           const ClAmsEntityTypeT entityType,
                           const ClCharT *entityName)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    ClRcT (*fn)(ClAmsMgmtCCBHandleT, const ClAmsEntityT *) = NULL;
    ClAmsEntityT entity;

    memset(&entity, 0, sizeof(ClAmsEntityT));
    
    CL_ASSERT(op != NULL);
    CL_ASSERT(entityName != NULL);

    if (STREQ(op, "create")) fn = clAmsMgmtCCBEntityCreate;
    else if (STREQ(op, "delete")) fn = clAmsMgmtCCBEntityDelete;
    else goto failure;

    entity.type = entityType;

    strncpy(entity.name.value, entityName, CL_MAX_NAME_LENGTH-1);
    entity.name.length = strlen(entity.name.value)+1;

    rc = fn(gCcbHandle, &entity);
    if (CL_OK != rc) goto failure;

    if(STREQ(op, "create") && 
       entityType == CL_AMS_ENTITY_TYPE_CSI)
    {
        ClAmsCSIConfigT csiConfig = {{0}};
        ClUint64T bitMask = CSI_CONFIG_TYPE;
        memcpy(&csiConfig.entity, &entity, sizeof(csiConfig.entity));
        clNameSet(&csiConfig.type, entity.name.value);
        ++csiConfig.type.length;
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle, &csiConfig.entity, bitMask);
        if(rc != CL_OK)
            goto failure;
    }
    else if (STREQ(op, "create") && (entityType == CL_AMS_ENTITY_TYPE_NODE))
    {
        /* 
         *  Not needed as its done on node addition in CPM.
         */
#if 0
        ClCpmNodeConfigT cpmNodeConfig;
        ClCpmSlotInfoT slotInfo;

        memset(&cpmNodeConfig, 0, sizeof(ClCpmNodeConfigT));
        memset(&slotInfo, 0, sizeof(ClCpmSlotInfoT));

        rc = clCpmMasterAddressGet(&slotInfo.slotId);
        if (CL_OK != rc) goto failure;

        rc = clCpmSlotInfoGet(CL_CPM_SLOT_ID, &slotInfo);
        if (CL_OK != rc && CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_COMP_UNREACHABLE) 
            goto failure;

        if(rc == CL_OK)
        {
            rc = clCorMoIdToMoIdNameGet(&slotInfo.nodeMoId,
                                        &cpmNodeConfig.nodeMoIdStr);
            if (CL_OK != rc)
                goto failure;
        }

        strncpy(cpmNodeConfig.nodeName,
                entity.name.value,
                strlen(entity.name.value));
        strncpy(cpmNodeConfig.nodeType.value,
                entity.name.value,
                strlen(entity.name.value));
        cpmNodeConfig.nodeType.length = strlen(cpmNodeConfig.nodeType.value);
        clNameCopy(&cpmNodeConfig.nodeIdentifier, &cpmNodeConfig.nodeType);
        strncpy(cpmNodeConfig.cpmType, "LOCAL", strlen("LOCAL"));

        rc = clAmsMgmtCCBCommit(gCcbHandle);
        if(CL_OK != rc)
            goto failure;
        rc = clCpmNodeConfigSet(&cpmNodeConfig);
        if (CL_OK != rc) goto failure;
        return rc;
#endif
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
    failure:
    return rc;
}

static ClRcT amsMgmtSGSet(ClUint32T argc,
                          ClCharT **argv,
                          ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    const ClCharT *value = argv[4];
    ClAmsEntityT targetEntity;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClUint64T bitMask = 0;
    ClAmsSGConfigT sgConfig;

    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&sgConfig, 0, sizeof(sgConfig));
            
    if (STREQ(attr, "si_list"))
    {
        targetEntity.type = CL_AMS_ENTITY_TYPE_SI;
        
        strncpy(targetEntity.name.value, value, CL_MAX_NAME_LENGTH-1);
        targetEntity.name.length = strlen(targetEntity.name.value)+1;
                
        rc = clAmsMgmtCCBSetSGSIList(gCcbHandle,
                                     entity,
                                     &targetEntity);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "su_list"))
    {
        targetEntity.type = CL_AMS_ENTITY_TYPE_SU;

        strncpy(targetEntity.name.value, value, CL_MAX_NAME_LENGTH-1);
        targetEntity.name.length = strlen(targetEntity.name.value)+1;
                
        rc = clAmsMgmtCCBSetSGSUList(gCcbHandle,
                                     entity,
                                     &targetEntity);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "redundancy_model"))
    {
        ClAmsSGRedundancyModelT redundancyModel =
        CL_AMS_SG_REDUNDANCY_MODEL_NONE;

        rc = amsEntityRedundancyModelFromStrGet(value, &redundancyModel);
        if (CL_OK != rc) goto failure;
                
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.redundancyModel = redundancyModel;
        bitMask |= SG_CONFIG_REDUNDANCY_MODEL;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "loading_strategy"))
    {
        ClAmsSGLoadingStrategyT loadingStrategy =
        CL_AMS_SG_LOADING_STRATEGY_NONE;

        rc = amsEntityLoadStrategyFromStrGet(value, &loadingStrategy);
        if (CL_OK != rc) goto failure;
                
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.loadingStrategy =loadingStrategy;
        bitMask |= SG_CONFIG_LOADING_STRATEGY;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "failback"))
    {
        ClBoolT failbackOption = CL_FALSE;

        rc = amsEntityBoolFromStrGet(value, &failbackOption);
        if (CL_OK != rc) goto failure;

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.failbackOption = failbackOption;
        bitMask |= SG_CONFIG_FAILBACK_OPTION;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "auto_repair"))
    {
        ClBoolT autoRepair = CL_FALSE;

        rc = amsEntityBoolFromStrGet(value, &autoRepair);
        if (CL_OK != rc) goto failure;

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.autoRepair = autoRepair;
        bitMask |= SG_CONFIG_AUTO_REPAIR;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "instantiate_duration"))
    {
        ClTimeT instantiateDuration = strtoll(value, NULL, 0);

        if (!instantiateDuration)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.instantiateDuration = instantiateDuration;
        bitMask |= SG_CONFIG_INSTANTIATE_DURATION;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_pref_active_sus"))
    {
        ClUint32T numPrefActiveSUs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.numPrefActiveSUs = numPrefActiveSUs;
        bitMask |= SG_CONFIG_NUM_PREF_ACTIVE_SUS;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_pref_standby_sus"))
    {
        ClUint32T numPrefStandbySUs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.numPrefStandbySUs = numPrefStandbySUs;
        bitMask |= SG_CONFIG_NUM_PREF_STANDBY_SUS;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_pref_inservice_sus"))
    {
        ClUint32T numPrefInserviceSUs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.numPrefInserviceSUs = numPrefInserviceSUs;
        bitMask |= SG_CONFIG_NUM_PREF_INSERVICE_SUS;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_pref_assigned_sus"))
    {
        ClUint32T numPrefAssignedSUs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.numPrefAssignedSUs = numPrefAssignedSUs;
        bitMask |= SG_CONFIG_NUM_PREF_ASSIGNED_SUS;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
#if 0
    else if (STREQ(attr, "num_pref_active_sus_per_si"))
    {
        ClUint32T numPrefActiveSUsPerSI = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.numPrefActiveSUsPerSI = numPrefActiveSUsPerSI;
        bitMask |= SG_CONFIG_NUM_PREF_ACTIVE_SUS_PER_SI;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
#endif
    else if (STREQ(attr, "max_active_sis_per_su"))
    {
        ClUint32T maxActiveSIsPerSU = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.maxActiveSIsPerSU = maxActiveSIsPerSU;
        bitMask |= SG_CONFIG_MAX_ACTIVE_SIS_PER_SU;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "max_standby_sis_per_su"))
    {
        ClUint32T maxStandbySIsPerSU = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.maxStandbySIsPerSU = maxStandbySIsPerSU;
        bitMask |= SG_CONFIG_MAX_STANDBY_SIS_PER_SU;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "comp_restart_duration"))
    {
        ClTimeT compRestartDuration = strtoll(value, NULL, 0);

        if (!compRestartDuration)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.compRestartDuration = compRestartDuration;
        bitMask |= SG_CONFIG_COMP_RESTART_DURATION;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "comp_restart_count_max"))
    {
        ClUint32T compRestartCountMax = atoi(value);

        if (!compRestartCountMax)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.compRestartCountMax = compRestartCountMax;
        bitMask |= SG_CONFIG_COMP_RESTART_COUNT_MAX;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "su_restart_duration"))
    {
        ClTimeT suRestartDuration = strtoll(value, NULL, 0);

        if (!suRestartDuration)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.suRestartDuration = suRestartDuration;
        bitMask |= SG_CONFIG_SU_RESTART_DURATION;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "su_restart_count_max"))
    {
        ClUint32T suRestartCountMax = atoi(value);

        if (!suRestartCountMax)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
        clHeapFree(pEntityConfig);

        sgConfig.suRestartCountMax = suRestartCountMax;
        bitMask |= SG_CONFIG_SU_RESTART_COUNT_MAX;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &sgConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtSISet(ClUint32T argc,
                          ClCharT **argv,
                          ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    const ClCharT *value = argv[4];
    ClAmsEntityT targetEntity;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClUint64T bitMask = 0;
    ClAmsSIConfigT siConfig;

    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&siConfig, 0, sizeof(siConfig));
            
    if (STREQ(attr, "rank"))
    {
        ClUint32T rank = atoi(value);
        
        if (!rank)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&siConfig, pEntityConfig, sizeof(siConfig));
        clHeapFree(pEntityConfig);

        siConfig.rank = rank;
        bitMask |= SI_CONFIG_RANK;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &siConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_standby_assignments"))
    {
        ClUint32T nStdbyAssign = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&siConfig, pEntityConfig, sizeof(siConfig));
        clHeapFree(pEntityConfig);

        siConfig.numStandbyAssignments = nStdbyAssign;
        bitMask |= SI_CONFIG_NUM_STANDBY_ASSIGNMENTS;
                
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &siConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "csi_list"))
    {
        targetEntity.type = CL_AMS_ENTITY_TYPE_CSI;

        strncpy(targetEntity.name.value, value, CL_MAX_NAME_LENGTH-1);
        targetEntity.name.length = strlen(targetEntity.name.value)+1;
                
        rc = clAmsMgmtCCBSetSICSIList(gCcbHandle,
                                      entity,
                                      &targetEntity);
        if (CL_OK != rc) goto failure;
        if( (rc = amsMgmtUpdateConfig(entity, &targetEntity, 
                                      CL_AMS_MGMT_CCB_SET_SI_CSI_LIST)) != CL_OK)
            goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }
            
    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtCSISet(ClUint32T argc,
                           ClCharT **argv,
                           ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    ClAmsEntityT targetEntity;
    ClAmsCSIConfigT csiConfig;

    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&csiConfig, 0, sizeof(csiConfig));

    if(STREQ(attr, "name_value_pair"))
    {
        ClAmsCSINVPT nvp;

        memset(&nvp, 0, sizeof(nvp));

        if (argc != 7)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        clNameCopy(&nvp.csiName, &entity->name);
        
        strncpy(nvp.paramName.value, argv[4], strlen(argv[4]));
        nvp.paramName.length = strlen(nvp.paramName.value);
        
        strncpy(nvp.paramValue.value, argv[5], strlen(argv[5]));
        nvp.paramValue.length = strlen(nvp.paramValue.value);

        rc = clAmsMgmtCCBCSISetNVP(gCcbHandle, entity, &nvp);
        if (CL_OK != rc) goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }
            
    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtNodeSet(ClUint32T argc,
                            ClCharT **argv,
                            ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    const ClCharT *value = argv[4];
    ClAmsEntityT targetEntity;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClUint64T bitMask = 0;
    ClAmsNodeConfigT nodeConfig;

    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&nodeConfig, 0, sizeof(nodeConfig));

    if (STREQ(attr, "su_list"))
    {
        targetEntity.type = CL_AMS_ENTITY_TYPE_SU;

        strncpy(targetEntity.name.value, value, CL_MAX_NAME_LENGTH-1);
        targetEntity.name.length = strlen(targetEntity.name.value)+1;

        rc = clAmsMgmtCCBSetNodeSUList(gCcbHandle,
                                       entity,
                                       &targetEntity);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "is_restartable"))
    {
        ClBoolT isRestartable = CL_FALSE;
        
        rc = amsEntityBoolFromStrGet(value, &isRestartable);
        if (CL_OK != rc) goto failure;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
        clHeapFree(pEntityConfig);

        nodeConfig.isRestartable = isRestartable;
        bitMask |= NODE_CONFIG_IS_RESTARTABLE;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &nodeConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "auto_repair"))
    {
        ClBoolT autoRepair = CL_FALSE;
        
        rc = amsEntityBoolFromStrGet(value, &autoRepair);
        if (CL_OK != rc) goto failure;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
        clHeapFree(pEntityConfig);

        nodeConfig.autoRepair = autoRepair;
        bitMask |= NODE_CONFIG_AUTO_REPAIR;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &nodeConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "su_failover_duration"))
    {
        ClTimeT suFailoverDuration = strtoll(value, NULL, 0);

        if (!suFailoverDuration)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
        clHeapFree(pEntityConfig);

        nodeConfig.suFailoverDuration = suFailoverDuration;
        bitMask |= NODE_CONFIG_SU_FAILOVER_DURATION;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &nodeConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "su_failover_count_max"))
    {
        ClUint32T suFailoverCountMax = atoi(value);
        
        if (!suFailoverCountMax)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&nodeConfig, pEntityConfig, sizeof(nodeConfig));
        clHeapFree(pEntityConfig);

        nodeConfig.suFailoverCountMax = suFailoverCountMax;
        bitMask |= NODE_CONFIG_SU_FAILOVER_COUNT_MAX;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &nodeConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtSUSet(ClUint32T argc,
                          ClCharT **argv,
                          ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    const ClCharT *value = argv[4];
    ClAmsEntityT targetEntity;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClUint64T bitMask = 0;
    ClAmsSUConfigT suConfig;

    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&suConfig, 0, sizeof(suConfig));

    if (STREQ(attr, "comp_list"))
    {
        targetEntity.type = CL_AMS_ENTITY_TYPE_COMP;

        strncpy(targetEntity.name.value, value, CL_MAX_NAME_LENGTH-1);
        targetEntity.name.length = strlen(targetEntity.name.value)+1;

        rc = clAmsMgmtCCBSetSUCompList(gCcbHandle,
                                       entity,
                                       &targetEntity);
        if (CL_OK != rc) goto failure;
        if((rc = amsMgmtUpdateConfig(entity, &targetEntity,
                                     CL_AMS_MGMT_CCB_SET_SU_COMP_LIST)) != CL_OK)
            goto failure;
    }
    else if (STREQ(attr, "rank"))
    {
        ClUint32T rank = atoi(value);
        
        if (!rank)
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;
        
        memcpy(&suConfig, pEntityConfig, sizeof(suConfig));
        clHeapFree(pEntityConfig);

        suConfig.rank = rank;
        bitMask |= SU_CONFIG_RANK;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &suConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "is_restartable"))
    {
        ClBoolT isRestartable = CL_FALSE;
        
        rc = amsEntityBoolFromStrGet(value, &isRestartable);
        if (CL_OK != rc) goto failure;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&suConfig, pEntityConfig, sizeof(suConfig));
        clHeapFree(pEntityConfig);

        suConfig.isRestartable = isRestartable;
        bitMask |= SU_CONFIG_IS_RESTARTABLE;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &suConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;

    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtCompSet(ClUint32T argc,
                            ClCharT **argv,
                            ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    const ClCharT *attr = argv[3];
    const ClCharT *value = argv[4];
    ClAmsEntityT targetEntity;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClUint64T bitMask = 0;
    ClAmsCompConfigT compConfig;
    ClUint32T i = 0;
    
    memset(&targetEntity, 0, sizeof(targetEntity));
    memset(&compConfig, 0, sizeof(compConfig));

    if (STREQ(attr, "supported_csi_types"))
    {
        ClNameT supportedCSIType;

        memset(&supportedCSIType, 0, sizeof(ClNameT));

        clNameSet(&supportedCSIType, value);
        ++supportedCSIType.length;
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        for(i = 0; i < compConfig.numSupportedCSITypes; ++i)
        {
            if(!memcmp(compConfig.pSupportedCSITypes[i].value,
                       supportedCSIType.value, supportedCSIType.length))
            {
                clHeapFree(compConfig.pSupportedCSITypes);
                rc = CL_AMS_RC(CL_ERR_ALREADY_EXIST);
                goto failure;
            }
        }
        
        compConfig.pSupportedCSITypes = clHeapRealloc(compConfig.pSupportedCSITypes,
                                                      (compConfig.numSupportedCSITypes+1)*
                                                      (ClUint32T)sizeof(*compConfig.pSupportedCSITypes));
        if(!compConfig.pSupportedCSITypes)
        {
            rc = CL_AMS_RC(CL_ERR_NO_MEMORY);
            goto failure;
        }

        memcpy(compConfig.pSupportedCSITypes+compConfig.numSupportedCSITypes,
               &supportedCSIType, sizeof(*compConfig.pSupportedCSITypes));
        ++compConfig.numSupportedCSITypes;

        bitMask |= COMP_CONFIG_SUPPORTED_CSI_TYPE;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);

        clHeapFree(compConfig.pSupportedCSITypes);

        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "capability_model"))
    {
        ClAmsCompCapModelT capModel = 0;
        
        rc = amsEntityCapModelFromStrGet(value, &capModel);
        if (CL_OK != rc) goto failure;

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.capabilityModel = capModel;
        bitMask |= COMP_CONFIG_CAPABILITY_MODEL;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "property"))
    {
    }
    else if (STREQ(attr, "is_restartable"))
    {
        ClBoolT isRestartable = CL_FALSE;
        
        rc = amsEntityBoolFromStrGet(value, &isRestartable);
        if (CL_OK != rc) goto failure;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);
        
        compConfig.isRestartable = isRestartable;
        bitMask |= COMP_CONFIG_IS_RESTARTABLE;
        
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "instantiate_level"))
    {
        bitMask = COMP_CONFIG_INSTANTIATE_LEVEL;
        compConfig.instantiateLevel = atoi(value);
        memcpy(&compConfig.entity, entity, sizeof(compConfig.entity));
        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if(CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_max_instantiate"))
    {
        ClUint32T numMaxInstantiate = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.numMaxInstantiate = numMaxInstantiate;
        bitMask |= COMP_CONFIG_NUM_MAX_INSTANTIATE;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_max_instantiate_with_delay"))
    {
        ClUint32T numMaxInstantiateWithDelay = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.numMaxInstantiateWithDelay = numMaxInstantiateWithDelay;
        bitMask |= COMP_CONFIG_NUM_MAX_INSTANTIATE_WITH_DELAY;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_max_active_csis"))
    {
        ClUint32T numMaxActiveCSIs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.numMaxActiveCSIs = numMaxActiveCSIs;
        bitMask |= COMP_CONFIG_NUM_MAX_ACTIVE_CSIS;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "num_max_standby_csis"))
    {
        ClUint32T numMaxStandbyCSIs = atoi(value);

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.numMaxStandbyCSIs = numMaxStandbyCSIs;
        bitMask |= COMP_CONFIG_NUM_MAX_STANDBY_CSIS;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "timeouts"))
    {
        ClTimeT timeout = strtoll(value, NULL, 0);
        
        if (!timeout) 
        {
            rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.timeouts.instantiate = timeout;
        compConfig.timeouts.terminate = timeout;
        compConfig.timeouts.cleanup = timeout;
        compConfig.timeouts.quiescingComplete = timeout;
        compConfig.timeouts.csiSet = timeout;
        compConfig.timeouts.csiRemove = timeout;
        compConfig.timeouts.instantiateDelay = timeout;
        bitMask |= COMP_CONFIG_TIMEOUTS;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;

    }
    else if (STREQ(attr, "recovery_on_timeout"))
    {
        ClAmsRecoveryT recovery = CL_AMS_RECOVERY_NONE;

        rc = amsEntityRecoveryFromStrGet(value, &recovery);
        if (CL_OK != rc) goto failure;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        compConfig.recoveryOnTimeout = recovery;
        bitMask |= COMP_CONFIG_RECOVERY_ON_TIMEOUT;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else if (STREQ(attr, "instantiate_command"))
    {
        ClUint32T i = 0;
        
        rc = clAmsMgmtEntityGetConfig(gHandle,
                                      entity,
                                      &pEntityConfig);
        if (CL_OK != rc) goto failure;

        memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
        clHeapFree(pEntityConfig);

        strncpy(compConfig.instantiateCommand,
                value,
                CL_MAX_NAME_LENGTH-1);
        for (i = 5; i < argc-1; ++i)
        {
            if (((CL_MAX_NAME_LENGTH-2) -
                 strlen(compConfig.instantiateCommand)) >
                strlen(argv[i]))
            {
                strcat(compConfig.instantiateCommand, " ");
                strncat(compConfig.instantiateCommand, argv[i], strlen(argv[i]));
            }
        }
            
        bitMask |= COMP_CONFIG_INSTANTIATE_COMMAND;

        rc = clAmsMgmtCCBEntitySetConfig(gCcbHandle,
                                         &compConfig.entity,
                                         bitMask);
        if (CL_OK != rc) goto failure;
    }
    else
    {
        rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        goto failure;
    }

    rc = clAmsMgmtCCBCommit(gCcbHandle);
    if (CL_OK != rc) goto failure;
    
    return CL_OK;
failure:
    return rc;
}

static ClRcT amsMgmtEntitySet(const ClAmsEntityTypeT entityType,
                              ClUint32T argc,
                              ClCharT **argv)
{
    ClRcT rc = CL_OK;
    ClAmsEntityT entity;
    ClAmsEntityT targetEntity;
    const ClCharT *targetEntityName = argv[argc-1];

    memset(&entity, 0, sizeof(ClAmsEntityT));
    memset(&targetEntity, 0, sizeof(targetEntity));
    
    entity.type = entityType;

    strncpy(entity.name.value, targetEntityName, CL_MAX_NAME_LENGTH-1);
    entity.name.length = strlen(entity.name.value)+1;

    switch (entityType)
    {
        case CL_AMS_ENTITY_TYPE_SG:
        {
            rc = amsMgmtSGSet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;
            
            break;
        }
        case CL_AMS_ENTITY_TYPE_SI:
        {
            rc = amsMgmtSISet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;

            break;
        }
        case CL_AMS_ENTITY_TYPE_CSI:
        {
            rc = amsMgmtCSISet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;

            break;
        }
        case CL_AMS_ENTITY_TYPE_NODE:
        {
            rc = amsMgmtNodeSet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;

            break;
        }
        case CL_AMS_ENTITY_TYPE_SU:
        {
            rc = amsMgmtSUSet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;

            break;
        }
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            rc = amsMgmtCompSet(argc, argv, &entity);
            if (CL_OK != rc) goto failure;

            break;
        }
        default:
        {
            break;
        }
    }
    
    return CL_OK;
failure:
    return rc;
}


static void amsMgmtMigrateListDisplay(ClAmsMgmtMigrateListT *migrateList,
                                      ClCharT **ret)
{
#define CHECK_CAPACITY(bytes) do {                                      \
    if( (bytes)+1+bytesWritten > currentSize )                          \
    {                                                                   \
        ClInt32T extra = ( ( (bytes) + 1 + bytesWritten ) - currentSize); \
        currentSize += MAX_BUFFER_SIZE + extra;                         \
        *ret = clHeapRealloc(*ret, currentSize);                        \
        CL_ASSERT(*ret != NULL);                                        \
    }                                                                   \
}while(0)

    ClUint32T currentSize = MAX_BUFFER_SIZE;
    ClUint32T bytesWritten = 0;
    if(!migrateList) return;
    if(*ret) 
    {
        clHeapFree(*ret);
        *ret = NULL;
    }
    *ret = clHeapCalloc(1, MAX_BUFFER_SIZE);
    CL_ASSERT(*ret != NULL);

    if(migrateList->si.count)
    {
        ClUint32T i = 0;
        for(i = 0; i < migrateList->si.count; ++i)
        {
            ClAmsEntityT *entity = migrateList->si.entity+i;
            ClCharT c = 0;
            ClUint32T bytes = snprintf(&c, sizeof(c), "Created SI [%.*s]\n",
                                       entity->name.length-1, entity->name.value);
            CHECK_CAPACITY(bytes);
            bytesWritten += snprintf(*ret + bytesWritten,
                                     currentSize - bytesWritten,
                                     "Created SI [%.*s]\n",
                                     entity->name.length-1, entity->name.value);
        }
        clHeapFree(migrateList->si.entity);
    }

    if(migrateList->csi.count)
    {
        ClUint32T i = 0;
        for(i = 0; i < migrateList->csi.count; ++i)
        {
            ClAmsEntityT *entity = migrateList->csi.entity+i;
            ClCharT c = 0;
            ClUint32T bytes = snprintf(&c, sizeof(c),
                                       "Created CSI [%.*s]\n",
                                       entity->name.length-1, entity->name.value);
            CHECK_CAPACITY(bytes);
            bytesWritten += snprintf(*ret + bytesWritten,
                                     currentSize - bytesWritten,
                                     "Created CSI [%.*s]\n",
                                     entity->name.length-1, entity->name.value);
        }
        clHeapFree(migrateList->csi.entity);
    }

    if(migrateList->node.count)
    {
        ClUint32T i = 0;
        for(i = 0; i < migrateList->node.count; ++i)
        {
            ClAmsEntityT *entity = migrateList->node.entity+i;
            ClCharT c = 0;
            ClUint32T bytes = snprintf(&c, sizeof(c), 
                                       "Created Node [%.*s]\n",
                                       entity->name.length-1, entity->name.value);
            CHECK_CAPACITY(bytes);
            bytesWritten += snprintf(*ret + bytesWritten,
                                     currentSize - bytesWritten,
                                     "Created Node [%.*s]\n",
                                     entity->name.length-1, entity->name.value);
        }
        clHeapFree(migrateList->node.entity);
    }

    if(migrateList->su.count)
    {
        ClUint32T i = 0;
        for(i = 0; i < migrateList->su.count; ++i)
        {
            ClAmsEntityT *entity = migrateList->su.entity + i;
            ClCharT c = 0;
            ClUint32T bytes = snprintf(&c, sizeof(c), 
                                       "Created SU [%.*s]\n",
                                       entity->name.length-1, entity->name.value);
            CHECK_CAPACITY(bytes);
            bytesWritten += snprintf(*ret + bytesWritten,
                                     currentSize - bytesWritten,
                                     "Created SU [%.*s]\n",
                                     entity->name.length-1, entity->name.value);
        }
        clHeapFree(migrateList->su.entity);
    }

    if(migrateList->comp.count)
    {
        ClUint32T i = 0;
        for(i = 0; i < migrateList->comp.count; ++i)
        {
            ClAmsEntityT *entity = migrateList->comp.entity + i;
            ClCharT c = 0;
            ClUint32T bytes = snprintf(&c, sizeof(c), "Created Comp [%.*s]\n",
                                       entity->name.length-1, entity->name.value);
            CHECK_CAPACITY(bytes);
            bytesWritten += snprintf(*ret + bytesWritten, currentSize - bytesWritten,
                                     "Created Comp [%.*s]\n",
                                     entity->name.length-1, entity->name.value);
        }
        clHeapFree(migrateList->comp.entity);
    }
    (*ret)[bytesWritten] = 0;
#undef CHECK_CAPACITY
}

static ClRcT amsMgmtEntityMigrate(ClAmsEntityTypeT type,
                                  ClUint32T argc,
                                  ClCharT **argv,
                                  ClCharT **ret)
{
    if(type != CL_AMS_ENTITY_TYPE_SG)
        return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
    if(argc == 7)
    {
        ClUint32T numActiveSUs = 0;
        ClUint32T numStandbySUs = 0;
        ClAmsMgmtMigrateListT migrateList;
        ClRcT rc = CL_OK;
        numActiveSUs = atoi(argv[5]);
        numStandbySUs = atoi(argv[6]);
        if(!numActiveSUs)
        {
            clLogError("AMS", "CLI", "Active SU count cannot be 0");
            return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
        }
        memset(&migrateList, 0, sizeof(migrateList));
        rc = clAmsMgmtMigrateSG(gHandle, argv[3], argv[4], numActiveSUs, numStandbySUs, &migrateList);
        if(rc != CL_OK) return rc;
        amsMgmtMigrateListDisplay(&migrateList, ret);
        return CL_OK;
    }
    return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
}

ClRcT clAmsDebugCliMgmtApi(ClUint32T argc, ClCharT **argv, ClCharT **ret)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    CL_ASSERT(STREQ(argv[0], "amsMgmt"));

    if (4 == argc)
    {
        if (STREQ(argv[1], "create") || STREQ(argv[1], "delete"))
        {
            ClAmsEntityTypeT entityType = CL_AMS_ENTITY_TYPE_ENTITY;
            
            rc = amsEntityFromStrGet(argv[2], &entityType);
            if (CL_OK != rc) goto failure;
            
            rc = amsMgmtEntity(argv[1], entityType, argv[3]);
            if (CL_OK != rc) goto failure;
        }
        else goto failure;
    }
    else if (6 <= argc)
    {
        if(STREQ(argv[1], "migrate"))
        {
            ClAmsEntityTypeT entityType = CL_AMS_ENTITY_TYPE_ENTITY;
            rc = amsEntityFromStrGet(argv[2], &entityType);
            if(CL_OK != rc) goto failure;
            rc = amsMgmtEntityMigrate(entityType, argc, argv, ret);
            if(CL_OK != rc) goto failure;
        }
        else if (STREQ(argv[1], "set"))
        {
            ClAmsEntityTypeT entityType = CL_AMS_ENTITY_TYPE_ENTITY;

            rc = amsEntityFromStrGet(argv[2], &entityType);
            if (CL_OK != rc) goto failure;
            
            rc = amsMgmtEntitySet(entityType, argc, argv);
            if (CL_OK != rc) goto failure;
        }
        else goto failure;
    }
    else goto failure;

    return CL_OK;
    failure:

    if (CL_ERR_INVALID_PARAMETER == CL_GET_ERROR_CODE(rc))
    {
        usage(ret);
    }
    else amsCliPrint(ret, "AMS managment operation failed");

    return rc;
}

static ClRcT amsMgmtAdminStateChangeTask(void *arg)
{
    ClAmsMgmtAdminChangeOperT *oper = arg;
    ClAmsAdminStateT lastState = oper->lastState;
    ClAmsAdminStateT newState = oper->newState;
    ClInt32T tries = 0;
    ClRcT rc = CL_OK;

    switch(lastState)
    {
    case CL_AMS_ADMIN_STATE_LOCKED_I:
        {
            switch(newState)
            {
            case CL_AMS_ADMIN_STATE_LOCKED_A:
                {
                    /*
                     * If the lock assignment fails because of an incorrect 
                     * or incomplete config, then retry
                     */
                    do
                    {
                        rc = clAmsMgmtEntityLockAssignmentExtended(gHandle,
                                                                   &oper->entity,
                                                                   CL_TRUE);
                    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER
                            &&
                            ++tries < 3 && sleep(2) == 0 );
                            
                }
                break;

            case CL_AMS_ADMIN_STATE_UNLOCKED:
                {
                    do
                    {
                        rc = clAmsMgmtEntityLockAssignmentExtended(gHandle,
                                                                   &oper->entity,
                                                                   CL_TRUE);
                    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_NULL_POINTER
                            &&
                            ++tries < 3 && sleep(2) == 0 );

                    if(rc == CL_OK 
                       || 
                       CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP)
                    {
                        rc = clAmsMgmtEntityUnlockExtended(gHandle,
                                                           &oper->entity,
                                                           CL_TRUE);
                    }
                }
                break;
            default: break;
            }
        }
        break;

    case CL_AMS_ADMIN_STATE_LOCKED_A:
        {
            switch(newState)
            {
            case CL_AMS_ADMIN_STATE_LOCKED_I:
                {
                    rc = clAmsMgmtEntityLockInstantiationExtended(gHandle,
                                                                  &oper->entity,
                                                                  CL_TRUE);
                }
                break;
                
            case CL_AMS_ADMIN_STATE_UNLOCKED:
                {
                    rc = clAmsMgmtEntityUnlockExtended(gHandle,
                                                       &oper->entity,
                                                       CL_TRUE);
                }
                break;
            default: break;
            }
        }
        break;

    case CL_AMS_ADMIN_STATE_UNLOCKED:
        {
            switch(newState)
            {
            case CL_AMS_ADMIN_STATE_LOCKED_A:
                {
                    rc = clAmsMgmtEntityLockAssignmentExtended(gHandle,
                                                               &oper->entity,
                                                               CL_TRUE);
                }
                break;

            case CL_AMS_ADMIN_STATE_LOCKED_I:
                {
                    rc = clAmsMgmtEntityLockAssignmentExtended(gHandle, 
                                                               &oper->entity,
                                                               CL_TRUE);
                    if(rc == CL_OK)
                    {
                        rc = clAmsMgmtEntityLockInstantiationExtended(gHandle,
                                                                      &oper->entity,
                                                                      CL_TRUE);
                    }
                }
                break;

            default:break;
            }
        }
        break;

    default: break;
    }

    clHeapFree(oper);
    return rc;
}

ClRcT clAmsMgmtAdminStateChange(ClAmsEntityT *entity,
                                ClAmsAdminStateT lastState,
                                ClAmsAdminStateT newState)
{
    ClRcT rc = CL_OK;
    ClAmsMgmtAdminChangeOperT *oper = NULL;
    if(lastState == newState)
        return CL_OK;

    if(!gAmsMgmtAdminPool.flags)
    {
        rc = clJobQueueInit(&gAmsMgmtAdminPool, 0, 1);
        if(rc != CL_OK)
        {
            clLogError("MGMT", "ADMIN", "Admin pool create failed with [%#x]", rc);
            goto out;
        }
    }
    oper = clHeapCalloc(1, sizeof(*oper));
    CL_ASSERT(oper != NULL);
    memcpy(&oper->entity, entity, sizeof(oper->entity));
    oper->lastState = lastState;
    oper->newState = newState;
    rc = clJobQueuePush(&gAmsMgmtAdminPool, amsMgmtAdminStateChangeTask, oper);

    out:
    return rc;
}
