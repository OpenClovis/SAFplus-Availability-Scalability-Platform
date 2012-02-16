
#include <clAmsMgmtOI.h>

/*
 * Cache for the AMF entities: entity, instance id to AMF entity name map
 */

#define CL_AMS_MGMT_OI_CACHE_BITS (0x8)
#define CL_AMS_MGMT_OI_CACHE_MAX ( 1 << CL_AMS_MGMT_OI_CACHE_BITS )
#define CL_AMS_MGMT_OI_CACHE_MASK (CL_AMS_MGMT_OI_CACHE_MAX-1)

static struct hashStruct *gClAmsMgmtOICacheTable[CL_AMS_ENTITY_TYPE_MAX + 2][CL_AMS_MGMT_OI_CACHE_MAX];

static struct hashStruct *gClAmsMgmtOIExtendedCacheTable[CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX][CL_AMS_MGMT_OI_CACHE_MAX];

static ClCorInstanceIdT gClAmsMgmtOIIndexTable[CL_AMS_ENTITY_TYPE_MAX+2];

static ClCorInstanceIdT gClAmsMgmtOIExtendedIndexTable[CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX];

static ClCorMOIdT  gClChassisMoid;

static ClOsalMutexT gClAmsMgmtOICacheMutex;

static ClBoolT gClAmsMgmtOIInitialized;

typedef struct ClAmsMgmtOICache
{
    struct hashStruct hash;
    ClCorInstanceIdT instance;
    ClAmsEntityT entity;
} ClAmsMgmtOICacheT;

typedef struct ClAmsMgmtOIExtendedCache
{
    struct hashStruct hash;
    ClCorInstanceIdT instance;
    ClAmsMgmtOIExtendedEntityConfigT *pConfig;
    ClUint32T configSize;
}ClAmsMgmtOIExtendedCacheT;

static const ClCharT *const gClAmsMgmtOIExtendedCacheStrTable[CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX] = 
{
    "saAmfSGSIRankTable",
    "saAmfSGSURankTable",
    "saAmfSUsperSIRankTable",    
    "saAmfSISIDepTable",
    "saAmfCSICSIDepTable",
    "saAmfCSINameValueTable",
    "saAmfSUSITable",
};

static __inline__ ClUint32T entityCacheHashKey(ClCorInstanceIdT instance)
{
    return (instance & CL_AMS_MGMT_OI_CACHE_MASK);
}

static ClAmsMgmtOICacheT *clAmsMgmtOICacheFind(struct hashStruct **table, ClCorInstanceIdT instance,
                                               ClAmsEntityT *entity)
{
    struct hashStruct *iter = NULL;
    ClUint32T hashKey = entityCacheHashKey(instance);
    for(iter = table[hashKey]; iter; iter = iter->pNext)
    {
        ClAmsMgmtOICacheT *entry = hashEntry(iter, ClAmsMgmtOICacheT, hash);
        if(entry->instance == instance)
            return entry;
    }
    return NULL;
}

static ClAmsMgmtOIExtendedCacheT *clAmsMgmtOIExtendedCacheFind(struct hashStruct **table, ClCorInstanceIdT instance)
{
    struct hashStruct *iter = NULL;
    ClUint32T hashKey = entityCacheHashKey(instance);
    for(iter = table[hashKey]; iter; iter = iter->pNext)
    {
        ClAmsMgmtOIExtendedCacheT *entry = hashEntry(iter, ClAmsMgmtOIExtendedCacheT, hash);
        if(entry->instance == instance)
            return entry;
    }
    return NULL;
}

static void clAmsMgmtOICacheAdd(ClCorInstanceIdT instance, ClAmsEntityT *entity)
{
    struct hashStruct **table = NULL;
    ClAmsMgmtOICacheT *cacheEntry = NULL;
    if(!entity || entity->type > CL_AMS_ENTITY_TYPE_MAX)
        return;
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    table = gClAmsMgmtOICacheTable[entity->type];
    if( (cacheEntry = clAmsMgmtOICacheFind(table, instance, entity) ) )
    {
        memcpy(&cacheEntry->entity, entity, sizeof(cacheEntry->entity));
    }
    else
    {
        cacheEntry = clHeapCalloc(1, sizeof(*cacheEntry));
        CL_ASSERT(cacheEntry != NULL);
        cacheEntry->instance = instance;
        memcpy(&cacheEntry->entity, entity, sizeof(cacheEntry->entity));
        hashAdd(table, entityCacheHashKey(instance), &cacheEntry->hash);
        clLogNotice("AMF", "MGMT", "Added entity [%s], instance [%d] to the cache",
                    entity->name.value, instance);
    }
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
}

ClRcT clAmsMgmtOIDelete(ClCorInstanceIdT instance, ClAmsEntityT *entity)
{
    ClAmsMgmtOICacheT *entry = NULL;
    if(!entity || entity->type > CL_AMS_ENTITY_TYPE_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    if(!(entry = clAmsMgmtOICacheFind(gClAmsMgmtOICacheTable[entity->type], instance, entity)))
    {
        clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    hashDel(&entry->hash);
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
    clHeapFree(entry);
    return CL_OK;
}

ClRcT clAmsMgmtOIExtendedDelete(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT instance)
{
    ClAmsMgmtOIExtendedCacheT *entry = NULL;

    if(type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    if(!(entry = clAmsMgmtOIExtendedCacheFind(gClAmsMgmtOIExtendedCacheTable[type], instance)))
    {
        clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    hashDel(&entry->hash);
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);

    if(entry->pConfig)
        clHeapFree(entry->pConfig);
    clHeapFree(entry);
    return CL_OK;
}

static void clAmsMgmtOICacheDestroy(ClAmsEntityTypeT type)
{
    struct hashStruct **table = NULL;
    ClUint32T i;

    if(type > CL_AMS_ENTITY_TYPE_MAX) return;
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    table = gClAmsMgmtOICacheTable[type];
    for(i = 0; i < CL_AMS_MGMT_OI_CACHE_MASK; ++i)
    {
        struct hashStruct *iter = NULL;
        struct hashStruct *next = NULL;
        for(iter = table[i]; iter; iter = next)
        {
            ClAmsMgmtOICacheT *entry = hashEntry(iter, ClAmsMgmtOICacheT, hash);
            next = iter->pNext;
            clHeapFree(entry);
        }
        table[i] = NULL;
    }
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
}

static void clAmsMgmtOIExtendedCacheDestroy(ClAmsMgmtOIExtendedClassTypeT type)
{
    struct hashStruct **table = NULL;
    ClUint32T i;

    if(type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX) return;
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    table = gClAmsMgmtOIExtendedCacheTable[type];
    for(i = 0; i < CL_AMS_MGMT_OI_CACHE_MASK; ++i)
    {
        struct hashStruct *iter = NULL;
        struct hashStruct *next = NULL;
        for(iter = table[i]; iter; iter = next)
        {
            ClAmsMgmtOIExtendedCacheT *entry = hashEntry(iter, ClAmsMgmtOIExtendedCacheT, hash);
            next = iter->pNext;
            if(entry->pConfig)
                clHeapFree(entry->pConfig);
            clHeapFree(entry);
        }
        table[i] = NULL;
    }
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
}

static void clAmsMgmtOIExtendedCacheAdd(ClAmsMgmtOIExtendedClassTypeT type,
                                        ClCorInstanceIdT instance, 
                                        ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                        ClUint32T configSize)
{
    struct hashStruct **table = NULL;
    ClAmsMgmtOIExtendedCacheT *cacheEntry = NULL;
    if(type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX || !pConfig || !configSize)
        return;
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    table = gClAmsMgmtOIExtendedCacheTable[type];
    if( (cacheEntry = clAmsMgmtOIExtendedCacheFind(table, instance) ) )
    {
        CL_ASSERT(cacheEntry->pConfig != NULL);
        if(cacheEntry->configSize != configSize)
        {
            clHeapFree(cacheEntry->pConfig);
            cacheEntry->pConfig = clHeapCalloc(1, configSize);
            CL_ASSERT(cacheEntry->pConfig != NULL);
            cacheEntry->configSize = configSize;
        }
        memcpy(cacheEntry->pConfig, pConfig, configSize);
    }
    else
    {
        ClAmsMgmtOIExtendedEntityConfigT *pExtendedConfig = NULL;
        cacheEntry = clHeapCalloc(1, sizeof(*cacheEntry));
        CL_ASSERT(cacheEntry != NULL);
        pExtendedConfig = clHeapCalloc(1, configSize);
        CL_ASSERT(pExtendedConfig != NULL);
        cacheEntry->pConfig = pExtendedConfig;
        cacheEntry->instance = instance;
        memcpy(cacheEntry->pConfig, pConfig, configSize);
        hashAdd(table, entityCacheHashKey(instance), &cacheEntry->hash);
        clLogNotice("AMF", "MGMT", "Added entity [%s], instance [%d] to the extended class [%s]",
                    pConfig->entity.name.value, instance, gClAmsMgmtOIExtendedCacheStrTable[type]);
    }
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
}

static ClRcT clAmsMgmtOIMoIdGet(ClAmsMgmtHandleT handle,
                                ClAmsEntityT *pEntity,
                                ClCorMOIdT *pMoId)
{
    ClCharT *userData = NULL;
    ClUint32T dataSize = 0;
    ClRcT rc = CL_OK;

    if(!pEntity || !pMoId)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    rc = clAmsMgmtEntityUserDataGetKey(handle, pEntity, &pEntity->name,
                                       &userData, &dataSize);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Entity user data get key for [%s] returned [%#x]",
                   pEntity->name.value, rc);
        return rc;
    }

    if(!userData || !dataSize)
        return CL_AMS_RC(CL_ERR_INVALID_STATE);

    rc = clCorMoIdUnpack(userData, dataSize, pMoId);
    clHeapFree(userData);
    return rc;
}

static ClRcT clAmsMgmtOIAttributeSet(ClAmsEntityT *pEntity, ClCorTxnSessionIdT *pSession,
                                     ClCorObjectHandleT objHandle, ClCorAttributeValueListT *pAttrList)
{
    ClRcT rc = CL_OK;
    ClUint32T i;

    if(!pEntity || !objHandle || !pAttrList || !pAttrList->numOfValues || !pAttrList->pAttributeValue)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    for(i = 0; i < pAttrList->numOfValues; ++i)
    {
        ClUint32T attrIndex = pAttrList->pAttributeValue[i].index;

        if(pAttrList->pAttributeValue[i].index == -100)
            attrIndex = CL_COR_INVALID_ATTR_IDX;

        rc = clCorObjectAttributeSet(pSession, objHandle, NULL, 
                                     pAttrList->pAttributeValue[i].attrId, 
                                     attrIndex, pAttrList->pAttributeValue[i].bufferPtr, 
                                     pAttrList->pAttributeValue[i].bufferSize);
        if(pAttrList->pAttributeValue[i].index == -100)
        {
            clHeapFree(pAttrList->pAttributeValue[i].bufferPtr);
            pAttrList->pAttributeValue[i].bufferPtr = NULL;
        }
        if(rc != CL_OK)
        {
            clLogError("AMF", "MGMT", "Attribute set for attribute [%d], entity [%s] returned [%#x]",
                       pAttrList->pAttributeValue[i].attrId, pEntity->name.value, rc);
            while(++i < pAttrList->numOfValues)
            {
                if(pAttrList->pAttributeValue[i].index == -100)
                    clHeapFree(pAttrList->pAttributeValue[i].bufferPtr);
            }
            goto out;
        }
    }

    out:
    return rc;
}

static ClRcT amsMgmtOIExtendedCSINVPAttributeSet(ClCorTxnSessionIdT *pSession,
                                                 ClAmsEntityT *pEntity,
                                                 ClAmsCSINVPBufferT *pNVPBuffer,
                                                 ClRcT (*pClAmsMgmtOIExtendedConfigAttributesGet)
                                                 (ClAmsMgmtOIExtendedClassTypeT type,
                                                  ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                  ClCorClassTypeT *pClassType,
                                                  ClCorAttributeValueListT *pAttrList))
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    ClCorObjectHandleT objHandle = 0;
    ClCorAttributeValueListT attrList = {0};
    ClUint32T index = 0;
    ClCorMOIdT moid;

    if(!pEntity || !pNVPBuffer || !pClAmsMgmtOIExtendedConfigAttributesGet)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    index = gClAmsMgmtOIExtendedIndexTable[CL_AMS_MGMT_OI_CSINAMEVALUE];
    for(i = 0; i < pNVPBuffer->count; ++i)
    {
        ClCorClassTypeT classType = 0;
        ClAmsMgmtOICSINVPConfigT csiNVPConfig = {{{0}}};
        memcpy(&moid, &gClChassisMoid, sizeof(moid));
        memcpy(&csiNVPConfig.config.entity, pEntity, sizeof(csiNVPConfig.config.entity));
        memcpy(&csiNVPConfig.nvp, &pNVPBuffer->nvp[i], sizeof(csiNVPConfig.nvp));
        rc = (*pClAmsMgmtOIExtendedConfigAttributesGet)(CL_AMS_MGMT_OI_CSINAMEVALUE,
                                                        &csiNVPConfig.config,
                                                        &classType, &attrList);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        if(!attrList.numOfValues)
        {
            if(attrList.pAttributeValue)
            {
                clHeapFree(attrList.pAttributeValue);
                attrList.pAttributeValue = NULL;
            }
            goto cache_add;
        }
        rc = clCorMoIdAppend(&moid, classType, index);
        CL_ASSERT(rc == CL_OK);
        rc = clCorMoIdServiceSet(&moid, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        CL_ASSERT(rc == CL_OK);

        rc = clCorMoIdToObjectHandleGet(&moid, &objHandle);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        rc = clAmsMgmtOIAttributeSet(pEntity, pSession, objHandle, &attrList);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        if(attrList.pAttributeValue)
        {
            clHeapFree(attrList.pAttributeValue);
            attrList.pAttributeValue = NULL;
            attrList.numOfValues = 0;
        }
        clCorObjectHandleFree(&objHandle);
        cache_add:
        clAmsMgmtOIExtendedCacheAdd(CL_AMS_MGMT_OI_CSINAMEVALUE, index, 
                                    &csiNVPConfig.config,
                                    (ClUint32T)sizeof(csiNVPConfig));
        ++index;
    }

    out_free:
    gClAmsMgmtOIExtendedIndexTable[CL_AMS_MGMT_OI_CSINAMEVALUE] = index;
    if(attrList.pAttributeValue)
        clHeapFree(attrList.pAttributeValue);
    if(objHandle)
        clCorObjectHandleFree(&objHandle);
    return rc;
}

static ClRcT amsMgmtOIExtendedConfigAttributeSet(ClAmsMgmtOIExtendedClassTypeT type,
                                                 ClCorTxnSessionIdT *pSession,
                                                 ClAmsEntityT *pEntity,
                                                 ClAmsEntityBufferT *pEntityBuffer,
                                                 ClRcT (*pClAmsMgmtOIExtendedConfigAttributesGet)
                                                 (ClAmsMgmtOIExtendedClassTypeT type,
                                                  ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                  ClCorClassTypeT *pClassType,
                                                  ClCorAttributeValueListT *pAttrList))
{
    ClUint32T i;
    ClRcT rc = CL_OK;
    ClCorObjectHandleT objHandle = 0;
    ClCorAttributeValueListT attrList = {0};
    ClUint32T index = 0;
    ClCorMOIdT moid;

    if(!pEntity || !pEntityBuffer || 
       !pClAmsMgmtOIExtendedConfigAttributesGet || type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    index = gClAmsMgmtOIExtendedIndexTable[type];
    for(i = 0; i < pEntityBuffer->count; ++i)
    {
        ClCorClassTypeT classType = 0;
        ClAmsMgmtOIExtendedEntityConfigT config = {{0}};
        memcpy(&moid, &gClChassisMoid, sizeof(moid));
        memcpy(&config.entity, pEntity, sizeof(config.entity));
        memcpy(&config.containedEntity, &pEntityBuffer->entity[i], sizeof(config.containedEntity));
        rc = (*pClAmsMgmtOIExtendedConfigAttributesGet)(type,
                                                        &config,
                                                        &classType, &attrList);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        if(!attrList.numOfValues)
        {
            if(attrList.pAttributeValue)
            {
                clHeapFree(attrList.pAttributeValue);
                attrList.pAttributeValue = NULL;
            }
            goto cache_add;
        }
        rc = clCorMoIdAppend(&moid, classType, index);
        CL_ASSERT(rc == CL_OK);
        rc = clCorMoIdServiceSet(&moid, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
        CL_ASSERT(rc == CL_OK);

        rc = clCorMoIdToObjectHandleGet(&moid, &objHandle);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        rc = clAmsMgmtOIAttributeSet(pEntity, pSession, objHandle, &attrList);
        if(rc != CL_OK)
        {
            goto out_free;
        }
        if(attrList.pAttributeValue)
        {
            clHeapFree(attrList.pAttributeValue);
            attrList.pAttributeValue = NULL;
            attrList.numOfValues = 0;
        }
        clCorObjectHandleFree(&objHandle);
        cache_add:
        clAmsMgmtOIExtendedCacheAdd(type, index, &config, (ClUint32T)sizeof(config));
        ++index;
    }

    out_free:
    gClAmsMgmtOIExtendedIndexTable[type] = index;
    if(attrList.pAttributeValue)
        clHeapFree(attrList.pAttributeValue);
    if(objHandle)
        clCorObjectHandleFree(&objHandle);
    return rc;
}

static ClRcT clAmsMgmtOIExtendedConfigAttributeSet(ClAmsMgmtHandleT handle,
                                                   ClCorTxnSessionIdT *pSession, 
                                                   ClAmsEntityT *pEntity,
                                                   ClRcT (*pClAmsMgmtOIExtendedConfigAttributesGet)
                                                   (ClAmsMgmtOIExtendedClassTypeT type, 
                                                    ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                    ClCorClassTypeT *pClassType,
                                                    ClCorAttributeValueListPtrT))
{
    ClRcT rc = CL_OK;

    if(!handle || !pSession || !pEntity || !pClAmsMgmtOIExtendedConfigAttributesGet)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    switch(pEntity->type)
    {
    case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsEntityBufferT list = {0};
            rc = clAmsMgmtGetSGSUList(handle, pEntity, &list);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "SG [%s] SU list returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }
            rc = amsMgmtOIExtendedConfigAttributeSet(CL_AMS_MGMT_OI_SGSURANK,
                                                     pSession,
                                                     pEntity, 
                                                     &list,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(list.entity)
            {
                clHeapFree(list.entity);
                list.entity = NULL;
                list.count = 0;
            }
            if(rc != CL_OK)
            {
                goto out;
            }
            rc = clAmsMgmtGetSGSIList(handle, pEntity, &list);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "SG [ %s] SI list returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }
            rc = amsMgmtOIExtendedConfigAttributeSet(CL_AMS_MGMT_OI_SGSIRANK,
                                                     pSession,
                                                     pEntity,
                                                     &list,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(list.entity)
                clHeapFree(list.entity);
            if(rc != CL_OK)
                goto out;
        }
        break;

    case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsEntityBufferT list = {0};
            rc = clAmsMgmtGetSIDependenciesList(handle, pEntity, &list);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "SI [%s] dependencies list returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }
            rc = amsMgmtOIExtendedConfigAttributeSet(CL_AMS_MGMT_OI_SISIDEP,
                                                     pSession,
                                                     pEntity,
                                                     &list,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(list.entity)
            {
                clHeapFree(list.entity);
                list.entity = NULL;
                list.count = 0;
            }

            if(rc != CL_OK)
                goto out;

            rc = clAmsMgmtGetSISURankList(handle, pEntity, &list);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "SI [%s] su rank list returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }

            rc = amsMgmtOIExtendedConfigAttributeSet(CL_AMS_MGMT_OI_SUSPERSIRANK,
                                                     pSession,
                                                     pEntity,
                                                     &list,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(list.entity)
                clHeapFree(list.entity);
            if(rc != CL_OK)
                goto out;
        }
        break;

    case CL_AMS_ENTITY_TYPE_CSI:
        {
            ClAmsEntityBufferT list = {0};
            ClAmsCSINVPBufferT buffer = {0};
            rc = clAmsMgmtGetCSIDependenciesList(handle,
                                                 pEntity,
                                                 &list);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "CSI [%s] dependencies list get returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }
            rc = amsMgmtOIExtendedConfigAttributeSet(CL_AMS_MGMT_OI_CSICSIDEP,
                                                     pSession,
                                                     pEntity,
                                                     &list,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(list.entity)
            {
                clHeapFree(list.entity);
                list.entity = NULL;
                list.count = 0;
            }
            if(rc != CL_OK)
            {
                goto out;
            }
            rc = clAmsMgmtGetCSINVPList(handle, pEntity, &buffer);
            if(rc != CL_OK)
            {
                clLogError("AMF", "MGMT", "CSI [%s] name value list get returned [%#x]",
                           pEntity->name.value, rc);
                goto out;
            }
            rc = amsMgmtOIExtendedCSINVPAttributeSet(pSession,
                                                     pEntity,
                                                     &buffer,
                                                     pClAmsMgmtOIExtendedConfigAttributesGet);
            if(buffer.nvp)
                clHeapFree(buffer.nvp);

            if(rc != CL_OK)
                goto out;
        }
        break;

    default:
        break;
    }

    out:
    return rc;
}

static ClRcT clAmsMgmtOIConfigAttributeSet(ClAmsMgmtHandleT handle,
                                           ClCorTxnSessionIdT *pSession, 
                                           ClCorMOIdT *pMoId, 
                                           ClAmsEntityT *pEntity, 
                                           ClRcT (*pClAmsMgmtOIConfigAttributesGet)
                                           (ClAmsEntityConfigT*, ClCorAttributeValueListPtrT ))
{
    ClCorObjectHandleT objHandle = 0;
    ClRcT rc = CL_OK;
    ClCorAttributeValueListT attrList = {0};
    ClAmsEntityConfigT *pEntityConfig = NULL;

    if(!handle || !pSession || !pMoId || !pEntity || !pClAmsMgmtOIConfigAttributesGet)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    rc = clAmsMgmtEntityGetConfig(handle, pEntity, &pEntityConfig);
    if(rc != CL_OK || !pEntityConfig) 
    {
        clLogError("AMF", "MGMT", "Entity [%s] config get returned [%#x]",
                   pEntity->name.value, rc);
        return rc;
    }
    rc = clCorMoIdToObjectHandleGet(pMoId, &objHandle);
    if(rc != CL_OK) 
    {
        goto out_free;
    }
    rc = (*pClAmsMgmtOIConfigAttributesGet)(pEntityConfig, &attrList);
    if(rc != CL_OK)
    {
        goto out_free;
    }
    rc = clAmsMgmtOIAttributeSet(pEntity, pSession, objHandle, &attrList);

    out_free:
    if(pEntityConfig)
        clHeapFree(pEntityConfig);
    if(objHandle)
        clCorObjectHandleFree(&objHandle);
    if(attrList.pAttributeValue)
        clHeapFree(attrList.pAttributeValue);

    return rc;
}

ClRcT clAmsMgmtOIInitialize(ClAmsMgmtHandleT *pHandle, 
                            ClRcT (*pClAmsMgmtOIConfigAttributesGet)
                            (ClAmsEntityConfigT*, ClCorAttributeValueListPtrT pAttrList),
                            ClRcT (*pClAmsMgmtOIExtendedConfigAttributesGet)
                            (ClAmsMgmtOIExtendedClassTypeT type, 
                             ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                             ClCorClassTypeT *pClassType,
                             ClCorAttributeValueListT *pAttrList))
{
    ClRcT rc = CL_OK;
    ClAmsMgmtHandleT handle = 0;
    ClAmsEntityBufferT buffer[CL_AMS_ENTITY_TYPE_MAX+2] = {{0}};
    ClVersionT version = {'B', 0x1, 0x1};
    ClCorTxnSessionIdT txnSession = 0;
    ClUint32T i;
    ClCorAddrT appAddress = {0};
    ClNameT chassisInstance = {0};
    ClUint32T chassisId = 0;

    if(!pClAmsMgmtOIConfigAttributesGet)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    rc = clOsalMutexInit(&gClAmsMgmtOICacheMutex);
    CL_ASSERT(rc == CL_OK);

    rc = clEoMyEoIocPortGet(&appAddress.portId);
    if(rc != CL_OK) 
        return rc;
    appAddress.nodeAddress = clIocLocalAddressGet();

    rc = clAmsMgmtInitialize(&handle, NULL, &version);
    if(rc != CL_OK)
    {
        return rc;
    }
    snprintf(chassisInstance.value, sizeof(chassisInstance.value),
             "%s:%d", "\\Chassis", chassisId);
    chassisInstance.length = strlen(chassisInstance.value);
    rc = clCorMoIdNameToMoIdGet(&chassisInstance, &gClChassisMoid);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "COR moid get for [%s] returned [%#x]",
                   chassisInstance.value, rc);
        goto out_free;
    }
    rc = clAmsMgmtGetSGList(handle, &buffer[CL_AMS_ENTITY_TYPE_SG]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get SG list returned [%#x]", rc);
        goto out_free;
    }
    rc = clAmsMgmtGetSIList(handle, &buffer[CL_AMS_ENTITY_TYPE_SI]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get SI list returned [%#x]", rc);
        goto out_free;
    }
    rc = clAmsMgmtGetCSIList(handle, &buffer[CL_AMS_ENTITY_TYPE_CSI]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get CSI list returned [%#x]", rc);
        goto out_free;
    }
    rc = clAmsMgmtGetNodeList(handle, &buffer[CL_AMS_ENTITY_TYPE_NODE]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get NODE list returned [%#x]", rc);
        goto out_free;
    }
    rc = clAmsMgmtGetSUList(handle, &buffer[CL_AMS_ENTITY_TYPE_SU]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get SU list returned [%#x]", rc);
        goto out_free;
    }
    rc = clAmsMgmtGetCompList(handle, &buffer[CL_AMS_ENTITY_TYPE_COMP]);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "Get COMP list returned [%#x]", rc);
        goto out_free;
    }
    /*
     * Now fetch the moid for each of the entities and build the cache.
     */
    for(i = 0; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        ClAmsEntityBufferT *pBuffer = &buffer[i];
        ClUint32T j;
        if(!pBuffer->count || !pBuffer->entity)
            continue;
        gClAmsMgmtOIIndexTable[i] = pBuffer->count;
        for(j = 0; j < pBuffer->count; ++j)
        {
            ClCorMOIdT moid = {{{0}}};
            rc = clAmsMgmtOIMoIdGet(handle, pBuffer->entity+j, &moid);
            if(rc != CL_OK)
            {
                continue;
            }
            clAmsMgmtOICacheAdd(clCorMoIdToInstanceGet(&moid), pBuffer->entity+j);
            /*
             * Required for instances exceeding the pre-configured limit.
             */
             rc = clCorOIRegister(&moid, &appAddress);
             if(rc != CL_OK)
             {
                 continue;
             }
            rc = clCorPrimaryOISet(&moid, &appAddress);
            if(rc != CL_OK)
            {
                /*
                 * Ignore as it could be already set by the active.
                 */
                continue;
            }
            clAmsMgmtOIConfigAttributeSet(handle, &txnSession, &moid, pBuffer->entity+j, pClAmsMgmtOIConfigAttributesGet);
            clAmsMgmtOIExtendedConfigAttributeSet(handle, &txnSession, pBuffer->entity+j,
                                                  pClAmsMgmtOIExtendedConfigAttributesGet);
        }
    }
    rc = clCorTxnSessionCommit(txnSession);
    if(rc != CL_OK)
    {
        clLogError("AMF", "MGMT", "AMF OI config commit returned [%#x]", rc);
    }
    else
    {
        clLogNotice("AMF", "MGMT", "Entity cache successfully initialized");
        gClAmsMgmtOIInitialized = CL_TRUE;
    }

    out_free:
    for(i = 0; i <= CL_AMS_ENTITY_TYPE_MAX; ++i)
    {
        if(buffer[i].entity) clHeapFree(buffer[i].entity);
        if(rc != CL_OK) clAmsMgmtOICacheDestroy(i);
    }
    if(rc != CL_OK)
    {
        for(i = 0; i < CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX; ++i)
            clAmsMgmtOIExtendedCacheDestroy(i);
    }
    if(rc == CL_OK && pHandle)
        *pHandle = handle;
    else 
        clAmsMgmtFinalize(handle);

    return rc;
}

/*
 * Given a moid, return the entity instance associated with it.
 */
ClRcT clAmsMgmtOIGet(ClCorMOIdT *pMoId, ClAmsEntityT *pEntity)
{
    ClRcT rc = CL_OK;
    ClAmsMgmtOICacheT *entry = NULL;
    if(!pMoId || !pEntity || pEntity->type > CL_AMS_ENTITY_TYPE_MAX) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    entry = clAmsMgmtOICacheFind(gClAmsMgmtOICacheTable[pEntity->type], 
                                 clCorMoIdToInstanceGet(pMoId), pEntity);
    if(!entry)
    {
        clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    memcpy(pEntity, &entry->entity, sizeof(*pEntity));
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
    return rc;
}

ClRcT clAmsMgmtOIExtendedGet(ClCorMOIdT *pMoId, ClAmsMgmtOIExtendedClassTypeT type, 
                             ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                             ClUint32T configSize)
{
    ClRcT rc = CL_OK;
    ClAmsMgmtOIExtendedCacheT *entry = NULL;
    if(!pMoId || !pConfig || type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    clOsalMutexLock(&gClAmsMgmtOICacheMutex);
    entry = clAmsMgmtOIExtendedCacheFind(gClAmsMgmtOIExtendedCacheTable[type], 
                                         clCorMoIdToInstanceGet(pMoId));
    if(!entry || !entry->pConfig || entry->configSize != configSize)
    {
        clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }
    memcpy(pConfig, entry->pConfig, configSize);
    clOsalMutexUnlock(&gClAmsMgmtOICacheMutex);
    return rc;
}

ClRcT clAmsMgmtOIIndexGet(ClAmsEntityTypeT type, ClCorInstanceIdT *pIndex)
{
    if(!pIndex || type > CL_AMS_ENTITY_TYPE_MAX) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    if((ClUint32T)gClAmsMgmtOIIndexTable[type] + 1 < (ClUint32T)gClAmsMgmtOIIndexTable[type])
        return CL_AMS_RC(CL_ERR_OUT_OF_RANGE);
    *pIndex = gClAmsMgmtOIIndexTable[type]++;
    return CL_OK;
}

ClRcT clAmsMgmtOIIndexPut(ClAmsEntityTypeT type, ClCorInstanceIdT index)
{
    if(type > CL_AMS_ENTITY_TYPE_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    /*
     * No op now but wondering if a bitmap is going to be useful because of the
     * way the instance id is laid out and mapped to use as an index.
     */
    return CL_OK;
}

ClRcT clAmsMgmtOIExtendedIndexGet(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT *pIndex)
{
    if(!pIndex || type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized)
        return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    if((ClUint32T)gClAmsMgmtOIExtendedIndexTable[type] + 1 < (ClUint32T)gClAmsMgmtOIExtendedIndexTable[type])
        return CL_AMS_RC(CL_ERR_OUT_OF_RANGE);
    *pIndex = gClAmsMgmtOIExtendedIndexTable[type]++;
    return CL_OK;
}

ClRcT clAmsMgmtOIExtendedIndexPut(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT index)
{
    if(type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!gClAmsMgmtOIInitialized) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    /*
     * No op now but wondering if a bitmap is going to be useful because of the
     * way the instance id is laid out and mapped to use as an index.
     */
    return CL_OK;
}
