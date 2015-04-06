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

#include <clCksmApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clDebugApi.h>
#include <clList.h>
#include <clHash.h>
#include <clParserApi.h>
#include <clLogApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsEntityTrigger.h>
#include <clAmsTriggerRmd.h>
#include <clCpmClient.h>
#include <xdrClErrorReportT.h>

#define CL_AMS_ENTITY_TRIGGER_CONFIG "clAmfEntityTrigger.xml"

typedef struct ClAmsEntityTrigger
{
    ClAmsEntityT entity;
    ClAmsThresholdT thresholds[CL_METRIC_MAX];
    ClMetricIdT recoveryThresholdId;
    ClListHeadT list;
    struct hashStruct hash;
}ClAmsEntityTriggerT;

typedef struct ClAmsListHead
{
    ClInt32T numElements;
    ClListHeadT list;
    ClOsalMutexT mutex;
    ClOsalCondT cond;
} ClAmsListHeadT;

typedef struct ClAmsEntityTriggerList
{
#define CL_AMS_ENTITY_THRESHOLD_BUCKET_BITS (8)
#define CL_AMS_ENTITY_THRESHOLD_BUCKETS (1 << CL_AMS_ENTITY_THRESHOLD_BUCKET_BITS)
#define CL_AMS_ENTITY_THRESHOLD_BUCKET_MASK ( CL_AMS_ENTITY_THRESHOLD_BUCKETS - 1)

    struct hashStruct *hashTable[CL_AMS_ENTITY_THRESHOLD_BUCKETS];
    ClAmsListHeadT list;
}ClAmsEntityTriggerListT;

static ClAmsEntityTriggerListT gClAmsEntityTriggerList = { .list = 
                                                                   { .numElements = 0,
                                                                     .list = 
                                                                         CL_LIST_HEAD_INITIALIZER(
                                                                                                  gClAmsEntityTriggerList.list.list),
                                                                   },
};

static ClAmsMgmtHandleT gClAmsEntityTriggerMgmtHandle;

typedef struct ClAmsEntityTriggerRecoveryCtrl
{
    ClBoolT running;
    ClOsalTaskIdT task;
    ClAmsListHeadT list;
}ClAmsEntityTriggerRecoveryCtrlT;

static ClAmsEntityTriggerRecoveryCtrlT gClAmsEntityTriggerRecoveryCtrl = {
    .running = CL_FALSE,
    .list  = { .numElements = 0,
               .list = CL_LIST_HEAD_INITIALIZER(gClAmsEntityTriggerRecoveryCtrl.list.list),
    },
};

static ClAmsEntityTriggerT gClAmsEntityTriggerDefault[CL_AMS_ENTITY_TYPE_MAX+1];

#define CL_AMS_COMMON_RECOVERY_MAP \
    {\
    CL_AMS_ADMIN_STATE_NONE, CL_AMS_ADMIN_STATE_NONE, CL_AMS_ADMIN_STATE_UNLOCKED,\
        CL_AMS_ADMIN_STATE_UNLOCKED, CL_AMS_ADMIN_STATE_NONE,\
    }

static ClAmsAdminStateT gClAmsEntityTriggerRecoveryResetMap[CL_AMS_ENTITY_TYPE_MAX+1][CL_AMS_ENTITY_RECOVERY_MAX] = {
    [CL_AMS_ENTITY_TYPE_NODE] = { CL_AMS_ADMIN_STATE_NONE, CL_AMS_ADMIN_STATE_NONE, CL_AMS_ADMIN_STATE_UNLOCKED,
                                      CL_AMS_ADMIN_STATE_NONE, CL_AMS_ADMIN_STATE_NONE,
        },
    [CL_AMS_ENTITY_TYPE_SG ] = CL_AMS_COMMON_RECOVERY_MAP,
    [CL_AMS_ENTITY_TYPE_SU] =  CL_AMS_COMMON_RECOVERY_MAP,
    [CL_AMS_ENTITY_TYPE_SI] =  CL_AMS_COMMON_RECOVERY_MAP,
    [CL_AMS_ENTITY_TYPE_COMP] = CL_AMS_COMMON_RECOVERY_MAP,
    [CL_AMS_ENTITY_TYPE_CSI] = CL_AMS_COMMON_RECOVERY_MAP,
};

/*
 * 2D matrix. w.r.t last recovery and current one.
 */
static ClBoolT gClAmsRecoveryAllowedMap[CL_AMS_ENTITY_RECOVERY_MAX][CL_AMS_ENTITY_RECOVERY_MAX] = { 
    {CL_TRUE, CL_TRUE,  CL_TRUE, CL_TRUE, CL_TRUE},
    {CL_TRUE, CL_TRUE,  CL_TRUE, CL_TRUE, CL_TRUE},
    {CL_TRUE, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE},
    {CL_TRUE, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE},
    {CL_TRUE, CL_FALSE, CL_FALSE, CL_FALSE, CL_FALSE},
};

static ClRcT clAmsEntityTriggerCheck(ClAmsEntityTriggerT *pEntityTrigger,
                                     ClMetricT *pMetric);

static ClRcT clAmsEntityTrigger(ClAmsEntityTriggerT *pEntityTrigger,
                                ClMetricIdT id,
                                ClBoolT reset);

static ClRcT clAmsListHeadInit(ClAmsListHeadT *pList)
{
    ClRcT rc;

    clOsalMutexInit(&pList->mutex);
    rc = clOsalCondInit(&pList->cond);
    if(rc != CL_OK)
    {
        clLogError("AMS", "INIT", "Failed to create a condtional variable. error code [0x%x].", rc);
        clOsalMutexDestroy(&pList->mutex);
        return rc;
    }
    pList->numElements = 0;
    CL_LIST_HEAD_INIT(&pList->list);
    return CL_OK;
}

static ClRcT clAmsListHeadDelete(ClAmsListHeadT *pList)
{
    while(!CL_LIST_HEAD_EMPTY(&pList->list))
    {
        ClListHeadT *pFirst = pList->list.pNext;
        ClAmsEntityTriggerT *pEntityTrigger = CL_LIST_ENTRY(pFirst, ClAmsEntityTriggerT, list);
        clListDel(pFirst);
        clHeapFree(pEntityTrigger);
    }
    pList->numElements = 0;
    return CL_OK;
}

/*
 * Load the thresholds with defaults.
 */
static void clAmsEntityTriggerLoadDefaults(ClAmsEntityTriggerT *pEntityTrigger,
                                           ClMetricIdT id)
{
    register ClInt32T i;
    ClMetricIdT start = id;
    ClMetricIdT end = start+1;

    if(!pEntityTrigger || pEntityTrigger->entity.type > CL_AMS_ENTITY_TYPE_MAX) return;
    
    if(start == CL_METRIC_ALL)
    {
        start += 1;
        end = CL_METRIC_MAX;
    }
    for(i = start;  i < end; ++i)
    {
        memcpy(&pEntityTrigger->thresholds[i],
               &gClAmsEntityTriggerDefault[pEntityTrigger->entity.type].thresholds[i],
               sizeof(pEntityTrigger->thresholds[i]));
    }
}
            
static ClAmsEntityTriggerT *clAmsEntityTriggerFind(ClAmsEntityT *pEntity)
{
    ClUint32T hashKey = 0;
    ClRcT rc = CL_OK;
    struct hashStruct *pTemp = NULL;
    ClAmsEntityTriggerT *pEntityTrigger = NULL;

    if(!pEntity)
        return NULL;

    rc = clCksm32bitCompute((ClUint8T*)pEntity->name.value,
                            pEntity->name.length,
                            &hashKey);
    CL_ASSERT(rc == CL_OK);
    hashKey &= CL_AMS_ENTITY_THRESHOLD_BUCKET_MASK;

    for(pTemp = gClAmsEntityTriggerList.hashTable[hashKey];
        pTemp;
        pTemp = pTemp->pNext)
    {
        pEntityTrigger = hashEntry(pTemp, ClAmsEntityTriggerT, hash);
        if(!memcmp(pEntityTrigger->entity.name.value,
                   pEntity->name.value,
                   pEntity->name.length))
            return pEntityTrigger;
    }

    return NULL;
}

static void clAmsEntityTriggerListAdd(ClAmsEntityTriggerT *pEntityTrigger)
{
    ClUint32T hashKey = 0;
    ClRcT rc = clCksm32bitCompute((ClUint8T*)pEntityTrigger->entity.name.value,
                                  pEntityTrigger->entity.name.length,
                                  &hashKey);
    CL_ASSERT(rc == CL_OK);
    hashKey &= CL_AMS_ENTITY_THRESHOLD_BUCKET_MASK;

    hashAdd(gClAmsEntityTriggerList.hashTable, hashKey, &pEntityTrigger->hash);
    clListAddTail(&pEntityTrigger->list, &gClAmsEntityTriggerList.list.list);
    ++gClAmsEntityTriggerList.list.numElements;
    
}

static ClRcT clAmsEntityTriggerUpdate(ClAmsEntityTriggerT *pEntityTrigger,
                                      ClMetricT *pMetric)
{
    ClMetricIdT start = pMetric->id;
    ClMetricIdT end = start+1;
    register ClInt32T i;

    if(start == CL_METRIC_ALL)
    {
        start += 1;
        end = CL_METRIC_MAX;
    }

    for(i = start; i < end; ++i)
    {
        pEntityTrigger->thresholds[i].metric.currentThreshold = pMetric->currentThreshold;
        clLogNotice("TRIGGER", "MODIFY", "Entity [%.*s], Threshold [%d]",
                    pEntityTrigger->entity.name.length-1, pEntityTrigger->entity.name.value, 
                    pMetric->currentThreshold);
    }

    return CL_OK;
}

static ClAmsEntityTriggerT *clAmsEntityTriggerCreate(ClAmsEntityT *pEntity,
                                                     ClMetricT *pMetric)
{
    ClAmsEntityTriggerT *pEntityTrigger = NULL;

    if(!pEntity || !pMetric)
        goto out;
    
    if(pMetric->id >= CL_METRIC_MAX)
    {
        clLogError("TRIGGER", "CREATE",
                   "Invalid metric [%#x] for entity [%.*s]",
                   pMetric->id, pEntity->name.length-1, pEntity->name.value);
        goto out;
    }    

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    pEntityTrigger = clAmsEntityTriggerFind(pEntity);
    if(!pEntityTrigger)
    {
        clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);
        pEntityTrigger = clHeapCalloc(1, sizeof(*pEntityTrigger));
        if(!pEntityTrigger)
        {
            clLogError("TRIGGER", "CREATE", "Memory allocation failure");
            goto out;
        }
        memcpy(&pEntityTrigger->entity, pEntity, sizeof(pEntityTrigger->entity));
        clAmsEntityTriggerLoadDefaults(pEntityTrigger, CL_METRIC_ALL);
        clLogNotice("TRIGGER", "CREATE", "Entity [%.*s], Metric [%#x]",
                    pEntity->name.length-1, pEntity->name.value, pMetric->id);
        clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
        clAmsEntityTriggerListAdd(pEntityTrigger);
    }

    clAmsEntityTriggerUpdate(pEntityTrigger, pMetric);
    clAmsEntityTriggerCheck(pEntityTrigger, pMetric);

    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    out:
    return pEntityTrigger;
}

/*
 * Reset to defaults.
 */
static ClRcT clAmsEntityTriggerReset(ClAmsEntityT *pEntity, ClMetricIdT id)
{
    ClAmsEntityTriggerT *pEntityTrigger = NULL;
    ClRcT rc = CL_OK;
    
    if(!pEntity || pEntity->type > CL_AMS_ENTITY_TYPE_MAX || id >= CL_METRIC_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    pEntityTrigger = clAmsEntityTriggerFind(pEntity);
    if(!pEntityTrigger)
    {
        clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);
        clLogError("TRIGGER", "RESET", "Entity threshold not found for [%.*s], Threshold [%#x]",
                   pEntity->name.length-1, pEntity->name.value, id);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    clAmsEntityTriggerLoadDefaults(pEntityTrigger, id);

    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    return rc;
}

static ClRcT clAmsTriggerFault(ClAmsEntityT *pEntity,
                               ClAmsRecoveryT recommendedRecovery)
{
    ClErrorReportT errorReport = {{0}};
    ClIocNodeAddressT master;
    ClRcT rc = clCpmMasterAddressGet(&master);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RECOVERY", "Master address get returned [%#x]", rc);
        goto out;
    }
    memcpy(&errorReport.compName, &pEntity->name, sizeof(errorReport.compName));
    /*
     *Patch comp name as CPM expects unpatched name before patching it up for AMS :-)
     */
    errorReport.compName.length -= 1;
    errorReport.recommendedRecovery = recommendedRecovery;
    errorReport.handle = 0;
    rc = clCpmClientRMDAsyncNew(master,
                                CPM_COMPONENT_FAILURE_REPORT,
                                (ClUint8T *) &errorReport,
                                sizeof(ClErrorReportT),
                                NULL,
                                NULL,
                                CL_RMD_CALL_ATMOST_ONCE,
                                0,
                                0,
                                0,
                                MARSHALL_FN(ClErrorReportT, 4, 0, 0));
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RECOVERY",
                   "Fault report RMD returned [%#x]", rc);
        goto out;
    }

    out:
    return rc;
}

/*
 * We forward operations on the Comp to the parent SU for failovers.
 */

static ClRcT clAmsCompRecovery(ClAmsEntityT *pEntity,
                               ClAmsThresholdT *pThreshold)
{
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClAmsCompConfigT *pCompConfig = NULL;
    ClRcT rc = CL_OK;

    if(pEntity->type == CL_AMS_ENTITY_TYPE_SU)
    {
        ClAmsEntityBufferT compBuffer = {0};
        rc = clAmsMgmtGetSUCompList(gClAmsEntityTriggerMgmtHandle,
                                    pEntity, &compBuffer);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "RECOVERY",
                       "SU [%.*s] get comp list returned [%#x]",
                       pEntity->name.length-1, pEntity->name.value, rc);
            goto out;
        }
        if(!compBuffer.count)
        {
            clLogNotice("TRIGGER", "RECOVERY",
                        "SU doesnt have components. Locking SU");
            rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                               pEntity);
            if(compBuffer.entity) clHeapFree(compBuffer.entity);
            goto out_free;
        }
        /*
         * Now overwrite the entity with first comp.
         */
        memcpy(pEntity, &compBuffer.entity[0], sizeof(*pEntity));
        clHeapFree(compBuffer.entity);
    }

    clLogNotice("TRIGGER", "RECOVERY",
                "Getting comp config for [%.*s]",
                pEntity->name.length-1, pEntity->name.value);

    rc = clAmsMgmtEntityGetConfig(gClAmsEntityTriggerMgmtHandle,
                                  pEntity, &pEntityConfig);

    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RECOVERY", 
                   "Entity get config returned [%#x]", rc);
        goto out;
    }
    pCompConfig = (ClAmsCompConfigT*)pEntityConfig;

    if(pThreshold->recoveryReset == CL_TRUE)
    {
        if(pThreshold->recovery == CL_AMS_ENTITY_RECOVERY_FAILOVER)
        {
            /*
             *Try repairing.
             */
            rc = clAmsMgmtEntityRepaired(gClAmsEntityTriggerMgmtHandle,
                                         &pCompConfig->parentSU.entity);
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY", 
                           "Repair didn't work. Trying lock assignment first");
                rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                                   &pCompConfig->parentSU.entity);
                if(rc != CL_OK)
                {
                    clLogError("TRIGGER", "RECOVERY",
                               "Lockassignment didn't work. Trying unlock as a last resort");
                }
            }
        }
        
        clLogNotice("TRIGGER", "RECOVERY",
                    "Recoverying SU [%.*s] through unlock",
                    pCompConfig->parentSU.entity.name.length-1,
                    pCompConfig->parentSU.entity.name.value);

        rc = clAmsMgmtEntityUnlock(gClAmsEntityTriggerMgmtHandle,
                                   &pCompConfig->parentSU.entity);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "RECOVERY RESET",
                       "Unlock for entity [%.*s] returned [%#x]",
                       pCompConfig->parentSU.entity.name.length-1,
                       pCompConfig->parentSU.entity.name.value, rc);
        }

        goto out_free;
    }

    switch(pThreshold->recovery)
    {
    case CL_AMS_ENTITY_RECOVERY_RESTART:

        clLogNotice("TRIGGER", "RECOVERY",
                    "Restarting comp [%.*s] as part of [%s] recovery",
                    pEntity->name.length-1, pEntity->name.value,
                    CL_METRIC_STR(pThreshold->metric.id));

        rc = clAmsMgmtEntityRestart(gClAmsEntityTriggerMgmtHandle,
                                    pEntity);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "RECOVERY",
                       "Comp restart returned with [%#x]", rc);
            goto out_free;
        }
        break;

    case CL_AMS_ENTITY_RECOVERY_FAILOVER:
        /*
         * We trigger a fault to AMS with a recommended recovery.
         */
        clAmsTriggerFault(pEntity, CL_AMS_RECOVERY_COMP_FAILOVER);
        break;

    case CL_AMS_ENTITY_RECOVERY_LOCK:
        /*
         * Graceful lock of the SU
         */
        clLogNotice("TRIGGER", "RECOVERY",
                    "Locking work for parent SU [%.*s] as part of [%s] recovery",
                    pCompConfig->parentSU.entity.name.length-1,
                    pCompConfig->parentSU.entity.name.value,
                    CL_METRIC_STR(pThreshold->metric.id));

        rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                           &pCompConfig->parentSU.entity);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "RECOVERY", 
                       "SU work switchover returned [%#x]", rc);
            goto out_free;
        }
        break;

    default:
        break;
    }

    out_free:
    if(pCompConfig->pSupportedCSITypes)
        clHeapFree(pCompConfig->pSupportedCSITypes);
    
    clHeapFree(pCompConfig);

    out:
    return rc;
}

/*
 * We forward operations on the CSI to the parent SI.
 * as operations are defined on the CSI level.
 */

static ClRcT clAmsCSIRecovery(ClAmsEntityT *pEntity,
                              ClAmsThresholdT *pThreshold)
{
    ClRcT rc= CL_OK;
    ClAmsEntityConfigT *pEntityConfig = NULL;
    ClAmsCSIConfigT *pCSIConfig = NULL;

    clLogNotice("TRIGGER", "RECOVERY", 
                "Getting CSI config for [%.*s]",
                pEntity->name.length-1, pEntity->name.value);

    rc = clAmsMgmtEntityGetConfig(gClAmsEntityTriggerMgmtHandle,
                                  pEntity, &pEntityConfig);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RECOVERY", 
                   "Entity get config for CSI returned [%#x]", rc);
        goto out;
    }

    pCSIConfig = (ClAmsCSIConfigT*)pEntityConfig;

    if(pThreshold->recoveryReset == CL_TRUE)
    {
        clLogNotice("RECOVERY", "RESET",
                    "Unlocking SI [%.*s]",
                    pCSIConfig->parentSI.entity.name.length-1,
                    pCSIConfig->parentSI.entity.name.value);

        rc = clAmsMgmtEntityUnlock(gClAmsEntityTriggerMgmtHandle,
                                   &pCSIConfig->parentSI.entity);

        if(rc != CL_OK)
        {
            clLogError("RECOVERY", "RESET",
                       "Unlock returned [%#x]", rc);
        }

        goto out_free;
    }

    switch(pThreshold->recovery)
    {

    case CL_AMS_ENTITY_RECOVERY_FAILOVER:
    case CL_AMS_ENTITY_RECOVERY_LOCK:
        clLogNotice("TRIGGER", "RECOVERY",
                    "Switching over work for SI [%.*s] as part of [%s] threshold recovery",
                    pCSIConfig->parentSI.entity.name.length-1,
                    pCSIConfig->parentSI.entity.name.value,
                    CL_METRIC_STR(pThreshold->metric.id));

        rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                           &pCSIConfig->parentSI.entity);
    
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "RECOVERY",
                       "Switching over SI returned [%#x]", rc);
            goto out_free;
        }

        break;
    
    default:
        break;
    }

    out_free:
    clHeapFree(pEntityConfig);

    out:
    return rc;
}

static ClRcT clAmsEntityRecoveryReset(ClAmsEntityT *pEntity,
                                      ClAmsThresholdT *pThreshold)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT adminState = gClAmsEntityTriggerRecoveryResetMap[pEntity->type][pThreshold->recovery];

    switch(adminState)
    {
        
    case CL_AMS_ADMIN_STATE_NONE:
        break;

    case CL_AMS_ADMIN_STATE_UNLOCKED:

        if(pEntity->type  == CL_AMS_ENTITY_TYPE_CSI)
            return clAmsCSIRecovery(pEntity, pThreshold);

        if(pEntity->type == CL_AMS_ENTITY_TYPE_COMP 
           || 
           pEntity->type == CL_AMS_ENTITY_TYPE_SU)
            return clAmsCompRecovery(pEntity, pThreshold);

        clAmsMgmtEntityRepaired(gClAmsEntityTriggerMgmtHandle,
                                pEntity);

        clLogNotice("RECOVERY", "RESET",
                    "Unlocking [%s] -> [%.*s]",
                    CL_AMS_STRING_ENTITY_TYPE(pEntity->type),
                    pEntity->name.length-1, pEntity->name.value);

        rc = clAmsMgmtEntityUnlock(gClAmsEntityTriggerMgmtHandle,
                                   pEntity);
        
        if(rc != CL_OK)
        {
            clLogError("RECOVERY", "RESET",
                       "Unlock returned [%#x]", rc);
        }
        break;
    default:;
    }

    return rc;
}

/*
 * Carry out per entity recovery based on the configured recovery.
 */
static ClRcT clAmsEntityRecovery(ClAmsEntityT *pEntity,
                                 ClAmsThresholdT *pThreshold)
{
    ClRcT rc = CL_OK;

    /*
     * Special case for undo on the entity that underwent recovery.
     */
    if(pThreshold->recoveryReset == CL_TRUE)
    {
        return clAmsEntityRecoveryReset(pEntity, pThreshold);
    }

    switch(pEntity->type)
    {
    case CL_AMS_ENTITY_TYPE_SI:
     
        switch(pThreshold->recovery)
        {

        case CL_AMS_ENTITY_RECOVERY_FAILOVER:
        case CL_AMS_ENTITY_RECOVERY_LOCK:
            
            /*
             * Switch work for the SI.
             */
            clLogNotice("TRIGGER", "RECOVERY",
                        "Switching over SI [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value,
                        CL_METRIC_STR(pThreshold->metric.id));
            rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                               pEntity);
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Switching over SI returned [%#x]", rc);
            }
            break;

        default:
            break;
        }
        break;

    case CL_AMS_ENTITY_TYPE_CSI:
        clAmsCSIRecovery(pEntity, pThreshold);
        break;

    case CL_AMS_ENTITY_TYPE_SG:

        switch(pThreshold->recovery)
        {
        case CL_AMS_ENTITY_RECOVERY_FAILOVER:
        case CL_AMS_ENTITY_RECOVERY_LOCK:
            /*
             * Lock work for the SG
             */
            clLogNotice("TRIGGER", "RECOVERY",
                        "Removing work for SG [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value, 
                        CL_METRIC_STR(pThreshold->metric.id));
            rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                               pEntity);
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Removing work for SG returned [%#x]", rc);
            }
            break;

        default:
            break;
        }

        break;

    case CL_AMS_ENTITY_TYPE_NODE:

        switch(pThreshold->recovery)
        {
        case CL_AMS_ENTITY_RECOVERY_LOCK:
            
            clLogNotice("TRIGGER", "RECOVERY",
                        "Switching over work for the node SUs [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value, 
                        CL_METRIC_STR(pThreshold->metric.id));
            rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                               pEntity);
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Switching over work for node returned [%#x]", rc);
            }
            break;

        case CL_AMS_ENTITY_RECOVERY_RESTART:
            
            clLogNotice("TRIGGER", "RECOVERY",
                        "Restarting SUs for node [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value,
                        CL_METRIC_STR(pThreshold->metric.id));

            rc = clAmsMgmtEntityRestart(gClAmsEntityTriggerMgmtHandle,
                                        pEntity);

            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Restart node returned [%#x]", rc);
            }

            break;

        case CL_AMS_ENTITY_RECOVERY_FAILOVER:
            clLogNotice("TRIGGER", "RECOVERY",
                        "Failing over node [%.*s] as part [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value,
                        CL_METRIC_STR(pThreshold->metric.id));
            clAmsTriggerFault(pEntity, CL_AMS_RECOVERY_NODE_FAILOVER);
            break;

        case CL_AMS_ENTITY_RECOVERY_FAILFAST:
            clLogNotice("TRIGGER", "RECOVERY",
                        "Failing over node [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value,
                        CL_METRIC_STR(pThreshold->metric.id));
            clAmsTriggerFault(pEntity, CL_AMS_RECOVERY_NODE_FAILFAST);
            break;

        default:
            break;
        }
        break;

    case CL_AMS_ENTITY_TYPE_SU:

        switch(pThreshold->recovery)
        {
        case CL_AMS_ENTITY_RECOVERY_FAILOVER:
                clLogNotice("TRIGGER", "RECOVERY", 
                            "Failover over work for SU [%.*s] as part of [%s] threshold recovery",
                            pEntity->name.length-1, pEntity->name.value, 
                            CL_METRIC_STR(pThreshold->metric.id));
                clAmsCompRecovery(pEntity, pThreshold);
                break;

        case CL_AMS_ENTITY_RECOVERY_LOCK:
            clLogNotice("TRIGGER", "RECOVERY", 
                        "Switching over work for SU [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value,
                        CL_METRIC_STR(pThreshold->metric.id)
                        );

            rc = clAmsMgmtEntityLockAssignment(gClAmsEntityTriggerMgmtHandle,
                                               pEntity);
            
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Switching over work for SU returned [%#x]", rc);
            }
            break;

        case CL_AMS_ENTITY_RECOVERY_RESTART:
            
            clLogNotice("TRIGGER", "RECOVERY",
                        "Restarting components in SU [%.*s] as part of [%s] threshold recovery",
                        pEntity->name.length-1, pEntity->name.value, 
                        CL_METRIC_STR(pThreshold->metric.id));
            
            rc = clAmsMgmtEntityRestart(gClAmsEntityTriggerMgmtHandle,
                                        pEntity);
            if(rc != CL_OK)
            {
                clLogError("TRIGGER", "RECOVERY",
                           "Restarting SU returned [%#x]", rc);
            }

            break;

        default:
            break;
        }
        break;
       
    case CL_AMS_ENTITY_TYPE_COMP:
        clAmsCompRecovery(pEntity, pThreshold);
        break;

    default:
        break;
    }

    return rc;
}

static ClRcT clAmsEntityTriggerRecovery(ClAmsEntityTriggerT *pEntityTrigger)
{
    ClRcT rc = CL_OK;
    ClMetricIdT id = pEntityTrigger->recoveryThresholdId;
    ClMetricIdT start = id;
    ClMetricIdT end = id+1;
    register ClInt32T i;
    ClAmsThresholdRecoveryT lastRecovery = CL_AMS_ENTITY_RECOVERY_NONE;

    if(id == CL_METRIC_ALL)
    {
        start += 1;
        end = CL_METRIC_MAX;
    }
    for(i = start; i < end; ++i)
    {
        ClAmsThresholdT *pThreshold = pEntityTrigger->thresholds+i;
        ClAmsThresholdRecoveryT recovery = pThreshold->recovery;
        ClBoolT recoveryAllowed = gClAmsRecoveryAllowedMap[lastRecovery][recovery];

        if(recoveryAllowed == CL_TRUE)
        {
            clAmsEntityRecovery(&pEntityTrigger->entity, pThreshold);
            if(recovery != CL_AMS_ENTITY_RECOVERY_NONE)
                lastRecovery = recovery;
        }
    }

    return rc;
}


/*
 * The recovery thread for entity threshold.
 */
static ClPtrT clAmsEntityTriggerRecoveryThread(ClPtrT pArg)
{
#define MAX_RECOVERY_MASK (0x3)

    static ClInt32T numRecovery = 0;
    ClTimerTimeOutT timeout = {.tsSec = 0, .tsMilliSec = 0 };
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };

    static ClAmsListHeadT recoveryList = {.numElements = 0,
                                          .list = CL_LIST_HEAD_INITIALIZER(recoveryList.list),
    };
    clOsalMutexLock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
    while(gClAmsEntityTriggerRecoveryCtrl.running == CL_TRUE)
    {
        ClListHeadT *pNext = NULL;

        while(!gClAmsEntityTriggerRecoveryCtrl.list.numElements)
        {
            numRecovery = 0;
            clOsalCondWait(&gClAmsEntityTriggerRecoveryCtrl.list.cond,
                           &gClAmsEntityTriggerRecoveryCtrl.list.mutex,
                           timeout);
            if(gClAmsEntityTriggerRecoveryCtrl.running == CL_FALSE)
                goto out_drain;
        }
        /*
         * We are here when the recovery list isn't empty.
         * We shove the existing recovery batch into the temp list 
         * and reset the main list to release the lock and go lockless.
         */
        recoveryList.numElements = gClAmsEntityTriggerRecoveryCtrl.list.numElements;
        gClAmsEntityTriggerRecoveryCtrl.list.numElements = 0;
        pNext = gClAmsEntityTriggerRecoveryCtrl.list.list.pNext;
        clListDelInit(&gClAmsEntityTriggerRecoveryCtrl.list.list);
        clListAddTail(pNext, &recoveryList.list);
        clOsalMutexUnlock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
        
        while(gClAmsEntityTriggerRecoveryCtrl.running == CL_TRUE
              &&
              recoveryList.numElements > 0
              &&
              !CL_LIST_HEAD_EMPTY(&recoveryList.list))
        {
            ClListHeadT *pFirst = recoveryList.list.pNext;
            ClAmsEntityTriggerT *pEntityTrigger = CL_LIST_ENTRY(pFirst, ClAmsEntityTriggerT, list);
            
            /*
             * Take a break incase you have done some recoveries
             */
            if(numRecovery && !(numRecovery & MAX_RECOVERY_MASK))
            {
                clOsalTaskDelay(delay);
            }
            ++numRecovery;
            numRecovery &= 0xffff;
            /*
             * Unlink the entry from the recovery list.
             */
            clListDel(pFirst);
            clAmsEntityTriggerRecovery(pEntityTrigger);
            clHeapFree(pEntityTrigger);
            --recoveryList.numElements;
        }

        clOsalMutexLock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);

        if(gClAmsEntityTriggerRecoveryCtrl.running == CL_FALSE)
        {
            goto out_drain;
        }

    }

    /*
     * Drain the entries holding the lock.
     */
    out_drain:
    clAmsListHeadDelete(&recoveryList);
    clAmsListHeadDelete(&gClAmsEntityTriggerRecoveryCtrl.list);
    clOsalMutexUnlock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
    
    return NULL;
}

static ClRcT clAmsEntityTriggerRecoveryThreadCreate(void)
{
    ClRcT rc = CL_OK;
    clAmsListHeadInit(&gClAmsEntityTriggerRecoveryCtrl.list);
    gClAmsEntityTriggerRecoveryCtrl.running = CL_TRUE;
    gClAmsEntityTriggerRecoveryCtrl.task = 0;
    rc = clOsalTaskCreateAttached("AMS-RECOVERY-THREAD", CL_OSAL_SCHED_OTHER, 0, 0,
                                  clAmsEntityTriggerRecoveryThread, NULL, 
                                  &gClAmsEntityTriggerRecoveryCtrl.task);
    if(rc != CL_OK)
    {
        gClAmsEntityTriggerRecoveryCtrl.running = CL_FALSE;
    }
    return rc;
}

static ClRcT clAmsEntityTriggerRecoveryThreadDelete(void)
{
    ClRcT rc = CL_OK;
    if(gClAmsEntityTriggerRecoveryCtrl.running &&
       gClAmsEntityTriggerRecoveryCtrl.task)
    {
        clOsalMutexLock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
        gClAmsEntityTriggerRecoveryCtrl.running = CL_FALSE;
        clOsalCondSignal(&gClAmsEntityTriggerRecoveryCtrl.list.cond);
        clOsalMutexUnlock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
        clOsalTaskJoin(gClAmsEntityTriggerRecoveryCtrl.task);
    }
    return rc;
}

static __inline__ ClRcT clAmsEntityTriggerRecoveryListAdd(ClAmsEntityTriggerT *pEntityTrigger)
{
    clOsalMutexLock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
    clListAddTail(&pEntityTrigger->list, 
                  &gClAmsEntityTriggerRecoveryCtrl.list.list);
    ++gClAmsEntityTriggerRecoveryCtrl.list.numElements;
    clOsalCondSignal(&gClAmsEntityTriggerRecoveryCtrl.list.cond);
    clOsalMutexUnlock(&gClAmsEntityTriggerRecoveryCtrl.list.mutex);
    return CL_OK;
}

static ClRcT clAmsEntityTrigger(ClAmsEntityTriggerT *pEntityTrigger,
                                ClMetricIdT id,
                                ClBoolT reset)
{
    ClAmsEntityTriggerT *pEntityTriggerRecovery = NULL;
    ClRcT rc = CL_OK;
    pEntityTriggerRecovery = clHeapCalloc(1, sizeof(*pEntityTriggerRecovery));
    CL_ASSERT(pEntityTriggerRecovery != NULL);
    /*
     *Safe w.r.t deletes behind our back while the trigger is running from the recovery queue.
     */
    memcpy(pEntityTriggerRecovery, pEntityTrigger, sizeof(*pEntityTrigger));
    pEntityTriggerRecovery->recoveryThresholdId = id;
    if(reset == CL_TRUE)
    {
        ClMetricIdT start = id;
        ClMetricIdT end = start+1;
        register ClInt32T i;
        if(start == CL_METRIC_ALL)
        {
            start += 1;
            end = CL_METRIC_MAX;
        }
        for(i = start; i < end; ++i)
        {
            pEntityTriggerRecovery->thresholds[i].recoveryReset = CL_TRUE;
        }
    }

    rc = clAmsEntityTriggerRecoveryListAdd(pEntityTriggerRecovery);
    return rc;
}

static ClRcT clAmsEntityTriggerCheck(ClAmsEntityTriggerT *pEntityTrigger,
                                     ClMetricT *pMetric)
{
    ClMetricIdT start = pMetric->id;
    ClMetricIdT end = start+1;
    register ClInt32T i;

    if(start == CL_METRIC_ALL)
    {
        start += 1;
        end = CL_METRIC_MAX;
    }

    for(i = start; i < end; ++i)
    {
        ClUint32T maxThreshold = pEntityTrigger->thresholds[i].metric.maxThreshold;
        ClUint32T maxOccurences = pEntityTrigger->thresholds[i].metric.maxOccurences;
        if(pMetric->currentThreshold >= maxThreshold 
           &&
           (++pEntityTrigger->thresholds[i].metric.numOccurences >= maxOccurences)
           )
        {
            pEntityTrigger->thresholds[i].metric.numOccurences = 0;
            clAmsEntityTrigger(pEntityTrigger, i, CL_FALSE);
        }
    }
    return CL_OK;
}

static ClRcT clAmsEntityLocate(ClAmsEntityT *pEntity)
{
    ClRcT rc = CL_OK;
    register ClInt32T i;

    for(i = CL_AMS_ENTITY_TYPE_ENTITY + 1;  i < CL_AMS_ENTITY_TYPE_MAX + 1; ++i)
    {
        ClAmsEntityConfigT *pEntityConfig = NULL;
        pEntity->type = i;
        rc = clAmsMgmtEntityGetConfig(gClAmsEntityTriggerMgmtHandle,
                                      pEntity,
                                      &pEntityConfig);
        if(rc != CL_OK)
        {
            if(pEntityConfig)
                clHeapFree(pEntityConfig);

            continue;
        }
        memcpy(pEntity, pEntityConfig, sizeof(*pEntity));
        clHeapFree(pEntityConfig);
        return CL_OK;
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);

}
/*
 * Loads the threshold in the AMS threshold db.
 */

ClRcT clAmsEntityTriggerLoad(ClAmsEntityT *pEntity,
                             ClMetricT *pMetric)
{

    ClRcT rc = CL_OK;
    ClAmsEntityTriggerT *pEntityTrigger = NULL;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "LOAD",
                   "Entity [%.*s] not found", 
                   pEntity->name.length-1, pEntity->name.value);
        return rc;
    }

    pEntityTrigger = clAmsEntityTriggerCreate(pEntity, pMetric);
    if(!pEntityTrigger)
    {
        clLogError("TRIGGER", "LOAD", "Threshold create failed");
        rc = CL_AMS_RC(CL_ERR_UNSPECIFIED);
        goto out;
    }
    out:
    return rc;
    
}

ClRcT clAmsEntityTriggerLoadDefault(ClAmsEntityT *pEntity,
                                    ClMetricIdT id)
{
    ClRcT rc = CL_OK;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RESET",
                   "Entity [%.*s] not found", 
                   pEntity->name.length-1, pEntity->name.value);
        return rc;
    }
    return clAmsEntityTriggerReset(pEntity, id);
}

/*
 * The trigger to be loaded needn't be in the trigger DB.
 */
ClRcT clAmsEntityTriggerLoadTrigger(ClAmsEntityT *pEntity,
                                    ClMetricIdT id)
{
    ClAmsEntityTriggerT entityTrigger = {{0}};
    ClRcT rc = CL_OK;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "LOAD",
                   "Entity [%.*s] not found", 
                   pEntity->name.length-1, pEntity->name.value);
        return rc;
    }

    memcpy(&entityTrigger.entity, pEntity, sizeof(entityTrigger.entity));
    clAmsEntityTriggerLoadDefaults(&entityTrigger, id);
    rc = clAmsEntityTrigger(&entityTrigger, id, CL_FALSE);
    return rc;
}

/*
 * Load all existing entity maps with the current threshold 
 * and check for triggers.
 */

ClRcT clAmsEntityTriggerLoadAll(ClMetricT *pMetric)
{
    ClAmsEntityTriggerT *pEntityTrigger = NULL;
    ClListHeadT *pHead = &gClAmsEntityTriggerList.list.list;
    register ClListHeadT *pTemp ;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    CL_LIST_FOR_EACH(pTemp, pHead)
    {
        pEntityTrigger = CL_LIST_ENTRY(pTemp, ClAmsEntityTriggerT, list);
        clAmsEntityTriggerUpdate(pEntityTrigger, pMetric);
        clAmsEntityTriggerCheck(pEntityTrigger, pMetric);
    }
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);
    return CL_OK;
}

/*
 * Load triggers for all of the entities. for the specified threshold
 */

ClRcT clAmsEntityTriggerLoadTriggerAll(ClMetricIdT id)
{
    ClListHeadT *pHead = &gClAmsEntityTriggerList.list.list;
    register ClListHeadT *pTemp ;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    CL_LIST_FOR_EACH(pTemp, pHead)
    {
        ClAmsEntityTriggerT *pEntityTrigger = CL_LIST_ENTRY(pTemp, ClAmsEntityTriggerT, list);
        clAmsEntityTrigger(pEntityTrigger, id, CL_FALSE);
    }
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    return CL_OK;
}

/*
 * Recovery can be on entities not loaded in the trigger DB
 */
ClRcT clAmsEntityTriggerRecoveryReset(ClAmsEntityT *pEntity,
                                      ClMetricIdT id)
{
    ClAmsEntityTriggerT entityTrigger = {{0}};
    ClRcT rc= CL_OK;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "RECOVERY RESET",
                   "Entity [%.*s] not found", 
                   pEntity->name.length-1, pEntity->name.value);
        return rc;
    }
    memcpy(&entityTrigger.entity, pEntity, sizeof(entityTrigger.entity));
    clAmsEntityTriggerLoadDefaults(&entityTrigger, id);
    clAmsEntityTrigger(&entityTrigger, id, CL_TRUE);

    return rc;
}

ClRcT clAmsEntityTriggerRecoveryResetAll(ClMetricIdT id)
{
    ClAmsEntityTriggerT *pEntityTrigger = NULL;
    ClListHeadT *pHead = &gClAmsEntityTriggerList.list.list;
    register ClListHeadT *pTemp ;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    CL_LIST_FOR_EACH(pTemp, pHead)
    {
        pEntityTrigger = CL_LIST_ENTRY(pTemp, ClAmsEntityTriggerT, list);
        clAmsEntityTrigger(pEntityTrigger, id, CL_TRUE);
    }
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    return CL_OK;
}

ClRcT clAmsEntityTriggerLoadDefaultAll(ClMetricIdT id)
{
    ClAmsEntityTriggerT *pEntityTrigger = NULL;
    ClListHeadT *pHead = &gClAmsEntityTriggerList.list.list;
    register ClListHeadT *pTemp;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    CL_LIST_FOR_EACH(pTemp, pHead)
    {
        pEntityTrigger = CL_LIST_ENTRY(pTemp, ClAmsEntityTriggerT, list);
        clAmsEntityTriggerLoadDefaults(pEntityTrigger, id);
    }
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    return CL_OK;
}

static __inline__ void clAmsEntityTriggerMetrics(ClAmsEntityTriggerT *pEntityTrigger,
                                                 ClMetricIdT id,
                                                 ClMetricT *pMetric)
{
    ClInt32T i = 0;
    if(id == CL_METRIC_ALL)
    {
        for(i = CL_METRIC_ALL+1; i < CL_METRIC_MAX; ++i)
        {
            memcpy(pMetric+i, &pEntityTrigger->thresholds[i].metric, sizeof(*pMetric));
        }
    }
    else
    {
        memcpy(pMetric, &pEntityTrigger->thresholds[id].metric, sizeof(*pMetric));
    }
}

ClRcT clAmsEntityTriggerGetMetric(ClAmsEntityT *pEntity, ClMetricIdT id, ClMetricT *pMetric)
{
    ClRcT rc = CL_OK;
    ClAmsEntityTriggerT *pEntityTrigger = NULL;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET", "Entity [%.*s] locate returned [%#x]",
                   pEntity->name.length-1, pEntity->name.value, rc);
        goto out;
    }
    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    pEntityTrigger = clAmsEntityTriggerFind(pEntity);
    if(!pEntityTrigger)
    {
        clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);
        clLogError("TRIGGER", "GET", "Entity [%.*s] not loaded in trigger db",
                   pEntity->name.length-1, pEntity->name.value);
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out;
    }

    clAmsEntityTriggerMetrics(pEntityTrigger, id, pMetric);
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);

    out:
    return rc;
}

ClRcT clAmsEntityTriggerGetMetricDefault(ClAmsEntityT *pEntity,
                                         ClMetricIdT id,
                                         ClMetricT *pMetric)
{
    ClRcT rc = CL_OK;
    ClAmsEntityTriggerT *pEntityTrigger = NULL;

#ifdef VXWORKS_BUILD
    return CL_AMS_RC(CL_ERR_NOT_SUPPORTED);
#endif

    rc = clAmsEntityLocate(pEntity);
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "GET-DEFAULT", "Entity [%.*s] locate returned [%#x]",
                   pEntity->name.length-1, pEntity->name.value, rc);
        goto out;
    }
    pEntityTrigger = &gClAmsEntityTriggerDefault[pEntity->type];
    clAmsEntityTriggerMetrics(pEntityTrigger, id, pMetric);
    
    out:
    return rc;
}
                                  
static ClRcT clAmsEntityTriggerParseThresholds(ClParserPtrT child,
                                               ClAmsThresholdT *thresholds,
                                               ClAmsThresholdT *defaultThresholds)
                                               
{
    register ClInt32T i;
    for(i = CL_METRIC_ALL+1; i < CL_METRIC_MAX; ++i)
    {
        ClParserPtrT element = 0;
        ClAmsThresholdT *pThreshold = thresholds + i;

        element = clParserChild(child, CL_METRIC_STR(i));
        if(element)
        {
            ClParserPtrT elementNode;
            elementNode = clParserChild(element, "threshold");
            if(elementNode)
            {
                pThreshold->metric.maxThreshold = atoi(elementNode->txt);
            }
            elementNode = clParserChild(element, "maxOccurences");
            if(elementNode)
            {
                pThreshold->metric.maxOccurences = atoi(elementNode->txt);
            }
            elementNode = clParserChild(element, "recovery");
            if(elementNode)
            {
                if(!strncasecmp(elementNode->txt, "FAILOVER", 8))
                {
                    pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_FAILOVER;
                }
                else if(!strncasecmp(elementNode->txt, "FAILFAST", 8))
                {
                    pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_FAILFAST;
                }
                else if(!strncasecmp(elementNode->txt, "RESTART", 7))
                {
                    pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_RESTART;
                }
                else if(!strncasecmp(elementNode->txt, "LOCK", 4))
                {
                    pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_LOCK;
                }
                else
                {
                    pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_NONE;
                }
            }
        }
    }
    return CL_OK;
}

/*
 * Parse the various triggers.
 */

static ClRcT clAmsEntityTriggerParse(ClParserPtrT *pHead)
{
    ClParserPtrT head = NULL;
    ClParserPtrT child = NULL;
    ClCharT *pConfig = getenv("ASP_CONFIG");
    static ClAmsThresholdT defaultThresholds[CL_METRIC_MAX];
    register ClInt32T i;
    ClRcT rc = CL_OK;

    if(!pHead)
    {
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    for(i = CL_METRIC_ALL+1; i < CL_METRIC_MAX; ++i)
    {
        ClAmsThresholdT *pThreshold = defaultThresholds+i;
        pThreshold->metric.id = i;
        pThreshold->metric.pType = CL_METRIC_STR(i);
        pThreshold->metric.numOccurences = 0;
        pThreshold->metric.maxOccurences = 1;
        pThreshold->metric.maxThreshold = 100;
        pThreshold->recovery = CL_AMS_ENTITY_RECOVERY_FAILOVER;
    }


    if(!pConfig)
    {
        clLogError("TRIGGER", "INI", "ASP_CONFIG is not set");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    head = clParserOpenFile(pConfig, CL_AMS_ENTITY_TRIGGER_CONFIG);
    if(!head)
    {
        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);
        goto out_set_default;
    }

    child = clParserChild(head, "defaults");
    if(child)
    {
        rc = clAmsEntityTriggerParseThresholds(child, defaultThresholds,
                                               defaultThresholds);
        if(rc != CL_OK)
        {
            goto out_set_default;
        }
    }

    /*
     * Now parse entities.
     */
    for(i = CL_AMS_ENTITY_TYPE_ENTITY+1; i < CL_AMS_ENTITY_TYPE_MAX + 1;
        ++i)
    {
        ClAmsThresholdT *pThresholds = gClAmsEntityTriggerDefault[i].thresholds;
        memcpy(pThresholds, defaultThresholds, 
               sizeof(gClAmsEntityTriggerDefault[i].thresholds));
        child = clParserChild(head, CL_AMS_STRING_ENTITY_TYPE(i));
        if(child)
        {
            rc = clAmsEntityTriggerParseThresholds(child,
                                                   pThresholds,
                                                   defaultThresholds);
            if(rc != CL_OK)
            {
                goto out_set_default;
            }
        }
    }

    clLogNotice("TRIGGER", "INI", "Trigger parse successful");
    *pHead = head;
    return CL_OK;

    out_set_default:
    for(i = CL_AMS_ENTITY_TYPE_ENTITY+1; i < CL_AMS_ENTITY_TYPE_MAX + 1; ++i)
    {
        ClAmsEntityTriggerT *pEntityTrigger = &gClAmsEntityTriggerDefault[i];
        memcpy(pEntityTrigger->thresholds, defaultThresholds,
               sizeof(pEntityTrigger->thresholds));
    }

    if(head)
        clParserFree(head);

    return rc;
}

/*  
 * Initialize the trigger framework.
 */
ClRcT clAmsEntityTriggerInitialize(void)
{
    ClRcT rc = CL_OK;
    ClVersionT version = {'B', 0x1, 0x1 };
    ClParserPtrT head = NULL;
    register ClInt32T i;

#ifdef VXWORKS_BUILD
    return rc;
#endif
    clAmsListHeadInit(&gClAmsEntityTriggerList.list);

    for(i = 0; i < CL_AMS_ENTITY_THRESHOLD_BUCKETS; ++i)
        gClAmsEntityTriggerList.hashTable[i] = NULL;

    rc = clAmsEntityTriggerParse(&head);
    if(rc != CL_OK)
    {
        clLogDebug("TRIGGER", "INI", "Trigger XML parse error. "  \
                     "Running with defaults");
    }

    rc = clAmsMgmtInitialize(&gClAmsEntityTriggerMgmtHandle,
                             NULL, &version);

    if(rc != CL_OK)
    {
        goto out_free;
    }

    rc = clAmsEntityTriggerRecoveryThreadCreate();
    if(rc != CL_OK)
    {
        clAmsMgmtFinalize(gClAmsEntityTriggerMgmtHandle);
        goto out_free;
    }
    
    rc = clAmsTriggerRmdInitialize();
    if(rc != CL_OK)
    {
        clLogError("TRIGGER", "INI", "Trigger RMD initialize returned [%#x]."
                   "Trigger RMD framework disabled", rc);
    }

    out_free:
    clParserFree(head);

    return rc;
}

ClRcT clAmsEntityTriggerFinalize(void)
{
    ClRcT rc = CL_OK;
    ClListHeadT *pHead = &gClAmsEntityTriggerList.list.list;

#ifdef VXWORKS_BUILD
    return rc;
#endif
    clAmsTriggerRmdFinalize();
    clAmsEntityTriggerRecoveryThreadDelete();

    clOsalMutexLock(&gClAmsEntityTriggerList.list.mutex);
    while(!CL_LIST_HEAD_EMPTY(pHead))
    {
        ClListHeadT *pFirst = pHead->pNext;
        ClAmsEntityTriggerT *pEntityTrigger = CL_LIST_ENTRY(pFirst, ClAmsEntityTriggerT, list);
        clListDel(pFirst);
        hashDel(&pEntityTrigger->hash);
        clHeapFree(pEntityTrigger);
    }
    clOsalMutexUnlock(&gClAmsEntityTriggerList.list.mutex);
    
    if(gClAmsEntityTriggerMgmtHandle)
    {
        rc = clAmsMgmtFinalize(gClAmsEntityTriggerMgmtHandle);
        if(rc != CL_OK)
        {
            clLogError("TRIGGER", "FINI", "Mgmt finalize returned [%#x]", rc);
        }
    }
    
    return rc;
}
