/*
 * Sample code to enable reading the AMF db using the amf management cache apis
 * The clAmsMgmtDB prefixed apis first fetch the DB using clAmsMgmtDBGet and then
 * overlay a client side cache referenced by the db handle.
 * The db handle returned can then be used to read the amf db entities using the various
 * apis in the code below.
 * This is in contrast to the clAmsMgmt apis (without DB prefix) which make a call to AMF
 * master to fetch the amf db entities for every request.
 *
 * The example usage of more or less all the amf mgmt DB (/cache) apis are in
 * clAmsDBTest() function below
 */

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clAmsMgmtClientApi.h>

static void dumpBuffer(ClAmsEntityBufferT *buffer);
static void dumpCSIRefBuffer(ClAmsCompCSIRefBufferT *csiRefBuffer);
static void dumpSISURefBuffer(ClAmsSISUExtendedRefBufferT *sisuRefBuffer);
static void dumpSUSIRefBuffer(ClAmsSUSIExtendedRefBufferT *susiRefBuffer);

#define __ENTITY_CONFIG_DUMP(e) dump##e##Config

#define __ENTITY_STATUS_DUMP(e) dump##e##Status

#define DECLARE_ENTITY_CONFIG_DUMP(e) \
static void __ENTITY_CONFIG_DUMP(e) (ClAmsEntityConfigT *config)

#define DECLARE_ENTITY_STATUS_DUMP(e) \
static void __ENTITY_STATUS_DUMP(e) (ClAmsEntityStatusT *status)

DECLARE_ENTITY_CONFIG_DUMP(SG);
DECLARE_ENTITY_STATUS_DUMP(SG);

DECLARE_ENTITY_CONFIG_DUMP(SI);
DECLARE_ENTITY_STATUS_DUMP(SI);

DECLARE_ENTITY_CONFIG_DUMP(CSI);
DECLARE_ENTITY_STATUS_DUMP(CSI);

DECLARE_ENTITY_CONFIG_DUMP(Node);
DECLARE_ENTITY_STATUS_DUMP(Node);

DECLARE_ENTITY_CONFIG_DUMP(SU);
DECLARE_ENTITY_STATUS_DUMP(SU);

DECLARE_ENTITY_CONFIG_DUMP(Comp);
DECLARE_ENTITY_STATUS_DUMP(Comp);

static ClRcT amsDBGetList(ClAmsMgmtDBHandleT db, ClAmsEntityTypeT type,
                          ClAmsEntityBufferT *buffer);
static ClRcT amsDBGetChildList(ClAmsMgmtDBHandleT db,
                               ClAmsEntityT *entity,
                               ClAmsEntityTypeT subType,
                               ClAmsEntityBufferT *buffer);

/*
 * Read the AMF db and cache contents to the db handle in the client side
 */
ClRcT clAmsDBRead(ClAmsMgmtDBHandleT *db)
{
    return clAmsMgmtDBGet(db);
}

ClRcT clAmsDBFinalize(ClAmsMgmtDBHandleT *db)
{
    return clAmsMgmtDBFinalize(db);
}

/*
 * Read or dump the amf db list for sg/si/csi/node/su/comp given the db/cache handle
 * The contents would be logged if the buffer passed is NULL
 */
ClRcT clAmsDBGetSGList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *sgBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_SG, sgBuffer);
}

ClRcT clAmsDBGetSIList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *siBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_SI, siBuffer);
}

ClRcT clAmsDBGetCSIList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *csiBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_CSI, csiBuffer);
}

ClRcT clAmsDBGetNodeList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *nodeBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_NODE, nodeBuffer);
}

ClRcT clAmsDBGetSUList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *suBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_SU, suBuffer);
}

ClRcT clAmsDBGetCompList(ClAmsMgmtDBHandleT db, ClAmsEntityBufferT *compBuffer)
{
    return amsDBGetList(db, CL_AMS_ENTITY_TYPE_COMP, compBuffer);
}

ClRcT clAmsDBGetSGSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                         ClAmsEntityBufferT *sgsiBuffer)
{
    return amsDBGetChildList(db, entity, CL_AMS_ENTITY_TYPE_SI, sgsiBuffer);
}

ClRcT clAmsDBGetSGSUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                         ClAmsEntityBufferT *sgsuBuffer)
{
    return amsDBGetChildList(db, entity, CL_AMS_ENTITY_TYPE_SU, sgsuBuffer);
}

ClRcT clAmsDBGetSICSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                          ClAmsEntityBufferT *sicsiBuffer)
{
    return amsDBGetChildList(db, entity, CL_AMS_ENTITY_TYPE_CSI, sicsiBuffer);
}

ClRcT clAmsDBGetNodeSUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                           ClAmsEntityBufferT *nodesuBuffer)
{
    return amsDBGetChildList(db, entity, CL_AMS_ENTITY_TYPE_SU, nodesuBuffer);
}

ClRcT clAmsDBGetSUCompList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                           ClAmsEntityBufferT *sucompBuffer)
{
    return amsDBGetChildList(db, entity, CL_AMS_ENTITY_TYPE_COMP, sucompBuffer);
}

ClRcT clAmsDBGetCompCSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                            ClAmsCompCSIRefBufferT *csiRefBuffer)
{
    ClAmsCompCSIRefBufferT buffer = {0};
    ClRcT rc = CL_OK;

    if(!csiRefBuffer)
        csiRefBuffer = &buffer;

    rc = clAmsMgmtDBGetCompCSIList(db, entity, csiRefBuffer);
    if(rc != CL_OK)
        return rc;

    if(csiRefBuffer == &buffer)
    {
        clLogNotice("DBA", "READ", "Getting comp csi list");
        dumpCSIRefBuffer(csiRefBuffer);
        clLogNotice("DBA", "READ", "Done getting comp csi list");
        clAmsMgmtFreeCompCSIRefBuffer(csiRefBuffer);
    }

    return rc;
}

ClRcT clAmsDBGetSISUList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                         ClAmsSISUExtendedRefBufferT *sisuRefBuffer)
{
    ClRcT rc = CL_OK;
    ClAmsSISUExtendedRefBufferT buffer = {0};
    if(!sisuRefBuffer)
        sisuRefBuffer = &buffer;
    rc = clAmsMgmtDBGetSISUList(db, entity, sisuRefBuffer);
    if(rc != CL_OK)
        return rc;
    if(sisuRefBuffer == &buffer)
    {
        clLogNotice("DBA", "READ", "Getting SI [%s] su list", entity->name.value);
        dumpSISURefBuffer(sisuRefBuffer);
        clLogNotice("DBA", "READ", "Done getting SI [%s] su list", entity->name.value);
        if(sisuRefBuffer->entityRef)
            clHeapFree(sisuRefBuffer->entityRef);
    }
    return rc;
}

ClRcT clAmsDBGetSUSIList(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                         ClAmsSUSIExtendedRefBufferT *susiRefBuffer)
{
    ClRcT rc = CL_OK;
    ClAmsSUSIExtendedRefBufferT buffer = {0};
    if(!susiRefBuffer)
        susiRefBuffer = &buffer;
    rc = clAmsMgmtDBGetSUAssignedSIsList(db, entity, susiRefBuffer);
    if(rc != CL_OK)
        return rc;
    if(susiRefBuffer == &buffer)
    {
        clLogNotice("DBA", "READ", "Getting SU [%s] si list", entity->name.value);
        dumpSUSIRefBuffer(susiRefBuffer);
        clLogNotice("DBA", "READ", "Done getting SU [%s] si list", entity->name.value);
        if(susiRefBuffer->entityRef)
            clHeapFree(susiRefBuffer->entityRef);
    }
    return rc;
}

ClRcT clAmsDBGetEntityConfig(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                             ClAmsEntityConfigT **config)
{
    ClRcT rc = CL_OK;
    static void (*dbConfigMap[CL_AMS_ENTITY_TYPE_MAX+1])(ClAmsEntityConfigT*) = {
        [CL_AMS_ENTITY_TYPE_SG] = __ENTITY_CONFIG_DUMP(SG),
        [CL_AMS_ENTITY_TYPE_SI] = __ENTITY_CONFIG_DUMP(SI),
        [CL_AMS_ENTITY_TYPE_CSI] = __ENTITY_CONFIG_DUMP(CSI),
        [CL_AMS_ENTITY_TYPE_NODE] = __ENTITY_CONFIG_DUMP(Node),
        [CL_AMS_ENTITY_TYPE_SU] = __ENTITY_CONFIG_DUMP(SU),
        [CL_AMS_ENTITY_TYPE_COMP] = __ENTITY_CONFIG_DUMP(Comp),
    };
    ClAmsEntityConfigT *myConfig = NULL;
    if(!entity 
       || 
       entity->type > CL_AMS_ENTITY_TYPE_MAX 
       ||
       !dbConfigMap[entity->type])
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!config)
    {
        config = &myConfig;
    }

    rc = clAmsMgmtDBGetEntityConfig(db, entity, config);
    if(rc != CL_OK)
        return rc;
    
    if(myConfig)
    {
        dbConfigMap[entity->type](myConfig);
        clHeapFree(myConfig);
    }

    return rc;
}

ClRcT clAmsDBGetEntityStatus(ClAmsMgmtDBHandleT db, ClAmsEntityT *entity,
                             ClAmsEntityStatusT **status)
{
    ClRcT rc = CL_OK;
    static void (*dbStatusMap[CL_AMS_ENTITY_TYPE_MAX+1])(ClAmsEntityStatusT*) = {
        [CL_AMS_ENTITY_TYPE_SG] = __ENTITY_STATUS_DUMP(SG),
        [CL_AMS_ENTITY_TYPE_SI] = __ENTITY_STATUS_DUMP(SI),
        [CL_AMS_ENTITY_TYPE_CSI] = __ENTITY_STATUS_DUMP(CSI),
        [CL_AMS_ENTITY_TYPE_NODE] = __ENTITY_STATUS_DUMP(Node),
        [CL_AMS_ENTITY_TYPE_SU] = __ENTITY_STATUS_DUMP(SU),
        [CL_AMS_ENTITY_TYPE_COMP] = __ENTITY_STATUS_DUMP(Comp),
    };
    ClAmsEntityStatusT *myStatus = NULL;
    if(!entity 
       || 
       entity->type > CL_AMS_ENTITY_TYPE_MAX 
       ||
       !dbStatusMap[entity->type])
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!status)
    {
        status = &myStatus;
    }

    rc = clAmsMgmtDBGetEntityStatus(db, entity, status);
    if(rc != CL_OK)
        return rc;
    
    if(myStatus)
    {
        dbStatusMap[entity->type](myStatus);
        clHeapFree(myStatus);
    }

    return rc;
}

static void dumpBuffer(ClAmsEntityBufferT *buffer)
{
    for(ClUint32T i = 0; i < buffer->count; ++i)
    {
        clLogNotice("DBA", "READ", "%s name [%s]", 
                    CL_AMS_STRING_ENTITY_TYPE(buffer->entity[i].type),
                    buffer->entity[i].name.value);
    }
}

static void dumpCSIRefBuffer(ClAmsCompCSIRefBufferT *csiRefBuffer)
{
    for(ClUint32T i = 0; i < csiRefBuffer->count; ++i)
    {
        if(csiRefBuffer->entityRef[i].activeComp)
        {
            clLogNotice("DBA", "READ", "CSI [%s], state [%s], active comp [%s]",
                        csiRefBuffer->entityRef[i].entityRef.entity.name.value,
                        CL_AMS_STRING_H_STATE(csiRefBuffer->entityRef[i].haState),
                        csiRefBuffer->entityRef[i].activeComp->name.value);
        }
        else
        {
            clLogNotice("DBA", "READ", "CSI [%s], state [%s]",
                        csiRefBuffer->entityRef[i].entityRef.entity.name.value,
                        CL_AMS_STRING_H_STATE(csiRefBuffer->entityRef[i].haState));
        }
    }
}

static void dumpSISURefBuffer(ClAmsSISUExtendedRefBufferT *sisuRefBuffer)
{
    for(ClUint32T i = 0; i < sisuRefBuffer->count; ++i)
    {
        clLogNotice("DBA", "READ", "SI has SU [%s] assigned with ha state [%s], invocations [%d]",
                    sisuRefBuffer->entityRef[i].entityRef.entity.name.value,
                    CL_AMS_STRING_H_STATE(sisuRefBuffer->entityRef[i].haState),
                    sisuRefBuffer->entityRef[i].pendingInvocations);
    }
}

static void dumpSUSIRefBuffer(ClAmsSUSIExtendedRefBufferT *susiRefBuffer)
{
    for(ClUint32T i = 0; i < susiRefBuffer->count; ++i)
    {
        clLogNotice("DBA", "READ", "SU has SI [%s] assigned with ha state [%s], active csis [%d], "
                    "standby csis [%d], quiesced csis [%d], quiescing csis [%d], numcsis [%d], "
                    "pending invocations [%d]",
                    susiRefBuffer->entityRef[i].entityRef.entity.name.value,
                    CL_AMS_STRING_H_STATE(susiRefBuffer->entityRef[i].haState),
                    susiRefBuffer->entityRef[i].numActiveCSIs, 
                    susiRefBuffer->entityRef[i].numStandbyCSIs,
                    susiRefBuffer->entityRef[i].numQuiescedCSIs,
                    susiRefBuffer->entityRef[i].numQuiescingCSIs,
                    susiRefBuffer->entityRef[i].numCSIs,
                    susiRefBuffer->entityRef[i].pendingInvocations);
    }
}

static void dumpSGConfig(ClAmsEntityConfigT *config)
{
    ClAmsSGConfigT *sg = (ClAmsSGConfigT*)config;
    clLogNotice("DBA", "READ", "SG config [%s], redundancy [%s], state [%s], "
                "autoRepair [%s], prefActiveSUs [%d], "
                "prefStandbySUs [%d], inserviceSUs [%d], prefActiveSUsPerSI [%d], "
                "maxActiveSIsPerSU [%d], maxStandbySIsPerSU [%d]",
                sg->entity.name.value, CL_AMS_STRING_SG_REDUNDANCY_MODEL(sg->redundancyModel),
                CL_AMS_STRING_A_STATE(sg->adminState),
                sg->autoRepair ? "yes" : "no", sg->numPrefActiveSUs, sg->numPrefStandbySUs,
                sg->numPrefInserviceSUs, sg->numPrefActiveSUsPerSI,
                sg->maxActiveSIsPerSU, sg->maxStandbySIsPerSU);
}

static void dumpSGStatus(ClAmsEntityStatusT *status)
{
    ClAmsSGStatusT *sg = (ClAmsSGStatusT*)status;
    clLogNotice("DBA", "READ", "SG status curr activeSUs [%d], curr standbySUs [%d]",
                sg->numCurrActiveSUs, sg->numCurrStandbySUs);
}

static void dumpSIConfig(ClAmsEntityConfigT *config)
{
    ClAmsSIConfigT *si = (ClAmsSIConfigT*)config;
    clLogNotice("DBA", "READ", "SI config [%s], state [%s], numCSIs [%d], "
                "numStandbyAssignments [%d], parent SG [%s]",
                si->entity.name.value, CL_AMS_STRING_A_STATE(si->adminState),
                si->numCSIs, si->numStandbyAssignments, si->parentSG.entity.name.value);
}

static void dumpSIStatus(ClAmsEntityStatusT *status)
{
    ClAmsSIStatusT *si = (ClAmsSIStatusT*)status;
    clLogNotice("DBA", "READ", "SI status oper state [%s], num active assignments [%d], "
                "num standby assignments [%d]", CL_AMS_STRING_O_STATE(si->operState),
                si->numActiveAssignments, si->numStandbyAssignments);
}

static void dumpCSIConfig(ClAmsEntityConfigT *config)
{
    ClAmsCSIConfigT *csi = (ClAmsCSIConfigT*)config;
    clLogNotice("DBA", "READ", "CSI config [%s], type [%.*s], parent SI [%s]",
                csi->entity.name.value, csi->type.length, csi->type.value, 
                csi->parentSI.entity.name.value);
}

static void dumpCSIStatus(ClAmsEntityStatusT *status)
{
    ClAmsCSIStatusT *csi = (ClAmsCSIStatusT*)status;
    clLogNotice("DBA", "READ", "CSI status timer count [%d]", csi->entity.timerCount);
}

static void dumpNodeConfig(ClAmsEntityConfigT *config)
{
    ClAmsNodeConfigT *node = (ClAmsNodeConfigT*)config;
    clLogNotice("DBA", "READ", "Node config [%s], state [%s], autoRepair [%s]",
                node->entity.name.value, CL_AMS_STRING_A_STATE(node->adminState),
                node->autoRepair ? "yes" : "no");
}

static void dumpNodeStatus(ClAmsEntityStatusT *status)
{
    ClAmsNodeStatusT *node = (ClAmsNodeStatusT*)status;
    clLogNotice("DBA", "READ", "Node status oper state [%s], presence state [%s], "
                "isClusterMember [%s], instantiated SUs [%d], assigned SUs [%d]",
                CL_AMS_STRING_O_STATE(node->operState),
                CL_AMS_STRING_P_STATE(node->presenceState),
                CL_AMS_STRING_NODE_ISCLUSTERMEMBER(node->isClusterMember),
                node->numInstantiatedSUs, node->numAssignedSUs);
}

static void dumpSUConfig(ClAmsEntityConfigT *config)
{
    ClAmsSUConfigT *su = (ClAmsSUConfigT*)config;
    clLogNotice("DBA", "READ", "SU config [%s], state [%s], components [%d], "
                "parent SG [%s], parent Node [%s]",
                su->entity.name.value, CL_AMS_STRING_A_STATE(su->adminState),
                su->numComponents, su->parentSG.entity.name.value,
                su->parentNode.entity.name.value);
}

static void dumpSUStatus(ClAmsEntityStatusT *status)
{
    ClAmsSUStatusT *su = (ClAmsSUStatusT*)status;
    clLogNotice("DBA", "READ", "SU status oper state [%s], presence state [%s], "
                "readiness state [%s], active SIs [%d], standby SIs [%d], quiesced SIs [%d], "
                "instantiated comp [%d]", 
                CL_AMS_STRING_O_STATE(su->operState), CL_AMS_STRING_P_STATE(su->presenceState),
                CL_AMS_STRING_R_STATE(su->readinessState), su->numActiveSIs,
                su->numStandbySIs, su->numQuiescedSIs, su->numInstantiatedComp);
}

static void dumpCompConfig(ClAmsEntityConfigT *config)
{
    ClAmsCompConfigT *comp = (ClAmsCompConfigT*)config;
    clLogNotice("DBA", "READ", "Comp config [%s], supported csitypes [%d], "
                "capability model [%s], instantiate level [%d], "
                "max active csis [%d], max standby csis [%d], recovery [%s], "
                "parent SU [%s]", comp->entity.name.value,
                comp->numSupportedCSITypes, 
                CL_AMS_STRING_COMP_CAP(comp->capabilityModel),
                comp->instantiateLevel, comp->numMaxActiveCSIs, 
                comp->numMaxStandbyCSIs, 
                CL_AMS_STRING_RECOVERY(comp->recoveryOnTimeout),
                comp->parentSU.entity.name.value);
}

static void dumpCompStatus(ClAmsEntityStatusT *status)
{
    ClAmsCompStatusT *comp = (ClAmsCompStatusT*)status;
    clLogNotice("DBA", "READ", "Comp status oper state [%s], presence state [%s], "
                "readiness state [%s], active csis [%d], standby csis [%d], "
                "quiesced csis [%d], quiescing csis [%d]",
                CL_AMS_STRING_O_STATE(comp->operState), CL_AMS_STRING_P_STATE(comp->presenceState),
                CL_AMS_STRING_R_STATE(comp->readinessState), comp->numActiveCSIs,
                comp->numStandbyCSIs, comp->numQuiescedCSIs, comp->numQuiescingCSIs);
}

static ClRcT amsDBGetList(ClAmsMgmtDBHandleT db, ClAmsEntityTypeT type,
                          ClAmsEntityBufferT *buffer)
{
    ClRcT rc = CL_OK;
    ClAmsEntityBufferT list = {0};
    static ClRcT (*dbGetList[CL_AMS_ENTITY_TYPE_MAX+1])(ClAmsMgmtDBHandleT, ClAmsEntityBufferT *) = {
        [CL_AMS_ENTITY_TYPE_SG] = clAmsMgmtDBGetSGList,
        [CL_AMS_ENTITY_TYPE_SI] = clAmsMgmtDBGetSIList,
        [CL_AMS_ENTITY_TYPE_CSI] = clAmsMgmtDBGetCSIList,
        [CL_AMS_ENTITY_TYPE_NODE] = clAmsMgmtDBGetNodeList,
        [CL_AMS_ENTITY_TYPE_SU] = clAmsMgmtDBGetSUList,
        [CL_AMS_ENTITY_TYPE_COMP] = clAmsMgmtDBGetCompList,
    };

    if(!db) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    if(!buffer) buffer = &list;

    if(type > CL_AMS_ENTITY_TYPE_MAX
       ||
       !dbGetList[type])
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    rc = dbGetList[type](db, buffer);
    if(rc != CL_OK)
        goto out;

    if(buffer == &list)
    {
        dumpBuffer(buffer);
        if(buffer->entity)
            clHeapFree(buffer->entity);
    }

    out:
    return rc;
}

static ClRcT amsDBGetChildList(ClAmsMgmtDBHandleT db,
                               ClAmsEntityT *entity,
                               ClAmsEntityTypeT subType,
                               ClAmsEntityBufferT *buffer)
{
    ClRcT rc = CL_OK;
    ClAmsEntityBufferT list = {0};
    static ClRcT (*dbGetChildList[CL_AMS_ENTITY_TYPE_MAX + 1][CL_AMS_ENTITY_TYPE_MAX+1])
        (ClAmsMgmtDBHandleT, ClAmsEntityT*, ClAmsEntityBufferT *) =
        {
            [CL_AMS_ENTITY_TYPE_SG][CL_AMS_ENTITY_TYPE_SI] = clAmsMgmtDBGetSGSIList,
            [CL_AMS_ENTITY_TYPE_SG][CL_AMS_ENTITY_TYPE_SU] = clAmsMgmtDBGetSGSUList,
            [CL_AMS_ENTITY_TYPE_SI][CL_AMS_ENTITY_TYPE_CSI] = clAmsMgmtDBGetSICSIList,
            [CL_AMS_ENTITY_TYPE_NODE][CL_AMS_ENTITY_TYPE_SU] = clAmsMgmtDBGetNodeSUList,
            [CL_AMS_ENTITY_TYPE_SU][CL_AMS_ENTITY_TYPE_COMP] = clAmsMgmtDBGetSUCompList,
        };

    if(!entity
       ||
       entity->type > CL_AMS_ENTITY_TYPE_MAX
       ||
       subType > CL_AMS_ENTITY_TYPE_MAX
       ||
       !dbGetChildList[entity->type] 
       ||
       !dbGetChildList[entity->type][subType])
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    if(!buffer)
        buffer = &list;

    rc = dbGetChildList[entity->type][subType](db, entity, buffer);
    if(rc != CL_OK)
        return rc;

    if(buffer == &list)
    {
        clLogNotice("DBA", "READ", "Getting [%s] [%s] list start",
                    CL_AMS_STRING_ENTITY_TYPE(entity->type),
                    CL_AMS_STRING_ENTITY_TYPE(subType));
        dumpBuffer(buffer);
        clLogNotice("DBA", "READ", "Getting [%s] [%s] list end",
                    CL_AMS_STRING_ENTITY_TYPE(entity->type),
                    CL_AMS_STRING_ENTITY_TYPE(subType));
        if(buffer->entity)
            clHeapFree(buffer->entity);
    }

    return rc;
}

static ClAmsMgmtDBHandleT testDb;

ClRcT clAmsDBTest(void)
{
#define __RESET_BUFFER(buffer) do {                                     \
        if((buffer).entity) { clHeapFree((buffer).entity); (buffer).entity = NULL; } \
        (buffer).count = 0;                                             \
}while(0)

#define __CHECK_ERROR(rc) do {                  \
        if( (rc) != CL_OK) goto out;            \
} while(0)
#define __CALL_LIST_TEST(E) do {                \
        rc = clAmsDBGet##E##List(testDb, NULL); \
        __CHECK_ERROR(rc);                      \
}while(0)

    ClRcT rc = CL_OK;
    ClAmsEntityBufferT buffer = {0};

    rc = clAmsDBRead(&testDb);
    __CHECK_ERROR(rc);

    __CALL_LIST_TEST(SG);
    __CALL_LIST_TEST(SI);
    __CALL_LIST_TEST(CSI);
    __CALL_LIST_TEST(Node);
    __CALL_LIST_TEST(SU);
    __CALL_LIST_TEST(Comp);

    rc = clAmsDBGetSGList(testDb, &buffer);
    __CHECK_ERROR(rc);
    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *sg = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, sg, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, sg, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSGSUList(testDb, sg, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSGSIList(testDb, sg, NULL);
        __CHECK_ERROR(rc);
    }

    __RESET_BUFFER(buffer);
    
    rc = clAmsDBGetSIList(testDb, &buffer);
    __CHECK_ERROR(rc);

    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *si = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, si, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, si, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSICSIList(testDb, si, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSISUList(testDb, si, NULL);
        __CHECK_ERROR(rc);
    }
    
    __RESET_BUFFER(buffer);
    rc = clAmsDBGetCSIList(testDb, &buffer);
    __CHECK_ERROR(rc);
    
    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *csi = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, csi, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, csi, NULL);
        __CHECK_ERROR(rc);
    }

    __RESET_BUFFER(buffer);
    rc = clAmsDBGetNodeList(testDb, &buffer);
    __CHECK_ERROR(rc);

    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *node = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, node, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, node, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetNodeSUList(testDb, node, NULL);
        __CHECK_ERROR(rc);
    }
    
    __RESET_BUFFER(buffer);
    rc = clAmsDBGetSUList(testDb, &buffer);
    __CHECK_ERROR(rc);
    
    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *su = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, su, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, su, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSUCompList(testDb, su, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetSUSIList(testDb, su, NULL);
        __CHECK_ERROR(rc);
    }

    __RESET_BUFFER(buffer);
    rc = clAmsDBGetCompList(testDb, &buffer);
    __CHECK_ERROR(rc);
    
    for(ClUint32T i = 0; i < buffer.count; ++i)
    {
        ClAmsEntityT *comp = buffer.entity + i;
        rc = clAmsDBGetEntityConfig(testDb, comp, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetEntityStatus(testDb, comp, NULL);
        __CHECK_ERROR(rc);
        rc = clAmsDBGetCompCSIList(testDb, comp, NULL);
        __CHECK_ERROR(rc);
    }

    out:
    __RESET_BUFFER(buffer);

    if(testDb)
        clAmsDBFinalize(&testDb);

    return rc;
}

/*
 * Get component names for the node
 */
ClRcT clAmsDBGetNodeCompList(ClAmsMgmtDBHandleT cache,
                             const ClCharT *nodeName, 
                             ClAmsEntityBufferT *compList)
{
    ClRcT rc = CL_OK;
    ClAmsEntityBufferT compBuffer = {0};
    ClAmsEntityBufferT suBuffer = {0};
    ClAmsEntityT entity = {.type = CL_AMS_ENTITY_TYPE_NODE};

    if(!cache || !nodeName) return CL_OK;

    if(compList)
    {
        compList->entity = NULL;
        compList->count = 0;
    }

    clNameSet(&entity.name, nodeName);

    rc = clAmsMgmtDBGetNodeSUList(cache, &entity, &suBuffer);
    if(rc != CL_OK)
        goto out;

    for(ClUint32T i = 0; i < suBuffer.count; ++i)
    {
        rc = clAmsMgmtDBGetSUCompList(cache, &suBuffer.entity[i], &compBuffer);
        if(rc != CL_OK)
            goto out_free;
        if(compBuffer.count == 0)
        {
            if(compBuffer.entity)
            {
                clHeapFree(compBuffer.entity);
                compBuffer.entity = NULL;
            }
            continue;
        }
        if(compList)
        {
            compList->entity = clHeapRealloc(compList->entity,
                                            sizeof(*compList->entity) * 
                                             (compList->count + compBuffer.count));
            CL_ASSERT(compList->entity != NULL);
            memcpy(compList->entity + compList->count, compBuffer.entity,
                   sizeof(*compBuffer.entity) * compBuffer.count);
            compList->count += compBuffer.count;
        }

        if(compBuffer.entity)
        {
            clHeapFree(compBuffer.entity);
            compBuffer.entity = NULL;
        }
        compBuffer.count = 0;
    }

    out_free:
    if(suBuffer.entity)
        clHeapFree(suBuffer.entity);

    out:
    return rc;
}

/*
 * Just an example usage to get comp names given a nodename
 */
ClRcT amsDBGetNodeCompList(const ClCharT *nodeName)
{
    ClRcT rc = CL_OK;
    static ClAmsMgmtDBHandleT cache;
    if(!nodeName) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(!cache)
    {
        rc = clAmsMgmtDBGet(&cache);
        CL_ASSERT(rc == CL_OK);
    }
    ClAmsEntityBufferT compBuffer = {0};
    rc = clAmsDBGetNodeCompList(cache, nodeName, &compBuffer);
    if(compBuffer.count)
        clLogNotice("NODE", "COMP", "Node [%s] has [%d] components",
                    nodeName, compBuffer.count);
    for(ClUint32T i = 0; i < compBuffer.count; ++i)
    {
        clLogNotice("NODE", "COMP", "Node [%s] has component [%s]",
                    nodeName, compBuffer.entity[i].name.value);
    }
    if(compBuffer.entity)
        clHeapFree(compBuffer.entity);
    
    /*
     * not finalizing the static cache, as its just an example routine
     */
    return rc;
}

