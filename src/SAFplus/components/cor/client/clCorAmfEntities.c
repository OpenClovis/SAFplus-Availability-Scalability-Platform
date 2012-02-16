#include <clCorAmf.h>
#include <clCorClient.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsUtils.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCpmApi.h>
#include <xdrClCorMOIdT.h>

#define COR_CLASS_ID_BASE 0x1
#define COR_AMF_BASE_CLASS_ATTR_ID_END (0x2)

ClBoolT  gClAmfMibLoaded;
ClCorClassTypeT gClCorClassIdAlloc;

typedef struct ClAmfEntityAttributeTable
{
    const ClCharT *attrName;
    ClCorAttrTypeT attrType;
    ClUint32T attrFlags;
    ClCorAttrIdT attrId;
    ClCorTypeT attrDataType;
    ClInt64T attrDefaultVal;
    ClInt64T attrMinVal;
    ClInt64T attrMaxVal;
} ClAmfEntityAttributeTableT;

static ClAmfEntityAttributeTableT gClEntityAttributeTable[] = 
{
    {
        .attrName = "entityType",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_ENTITY_TYPE_ENTITY,
        .attrMinVal = 0,
        .attrMaxVal = CL_AMS_ENTITY_TYPE_MAX+1,
    },
    {
        .attrName = "entityName",
        .attrType=CL_COR_ARRAY_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = 0x2,
        .attrDataType = CL_COR_UINT8,
        .attrDefaultVal = 0,
        .attrMinVal = 1,
        .attrMaxVal = CL_MAX_NAME_LENGTH,
    },
    {
        .attrName = NULL,
    },
};

static ClAmfEntityAttributeTableT gClSgAttributeTable[] = 
{
    {
        .attrName="adminState", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_ADMIN_STATE_LOCKED_I,
        .attrMinVal = CL_AMS_ADMIN_STATE_NONE+1,
        .attrMaxVal = CL_AMS_ADMIN_STATE_MAX
    },
    {
        .attrName="redundancyModel", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_SG_REDUNDANCY_MODEL_TWO_N,
        .attrMinVal = CL_AMS_SG_REDUNDANCY_MODEL_NONE+1,
        .attrMaxVal = CL_AMS_SG_REDUNDANCY_MODEL_MAX,
    },
    {
        .attrName="loadingStrategy", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x3,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU,
        .attrMinVal = CL_AMS_SG_LOADING_STRATEGY_NONE+1,
        .attrMaxVal = CL_AMS_SG_LOADING_STRATEGY_MAX,
    },
    {
        .attrName="failbackOption", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x4,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_FALSE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="autoRepair", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x5,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="instantiateDuration", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x6,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 10000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="numPrefActiveSUs", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x7,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numPrefStandbySUs", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x8,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numPrefInserviceSUs", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x9,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 2,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numPrefAssignedSUs", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xa,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 2,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numPrefActiveSUsPerSI", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xb,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="maxActiveSIsPerSU",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xc,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="maxStandbySIsPerSU", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xd,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="compRestartDuration",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xe,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 10000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="compRestartCountMax", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xf,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 5,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },

    {
        .attrName="suRestartDuration",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x10,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="suRestartCountMax", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x11,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 5,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="isCollocationAllowed", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x12,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_FALSE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="alphaFactor",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x13,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 100,
        .attrMinVal = 1,
        .attrMaxVal = 101,
    },
    {
        .attrName="autoAdjust", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x14,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_FALSE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="autoAdjustProbation",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x15,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 10000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="reductionProcedure", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x16,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_FALSE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName = NULL,
    },
};

static ClAmfEntityAttributeTableT gClNodeAttributeTable[] =
{
    {
        .attrName="adminState",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_ADMIN_STATE_LOCKED_I,
        .attrMinVal = CL_AMS_ADMIN_STATE_NONE + 1,
        .attrMaxVal = CL_AMS_ADMIN_STATE_MAX,
    },
    {
        .attrName="id", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="classType", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x3,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_NODE_CLASS_C,
        .attrMinVal = CL_AMS_NODE_CLASS_NONE+1,
        .attrMaxVal = CL_AMS_NODE_CLASS_MAX,
    },
    {
        .attrName="isSwappable", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x4,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="isRestartable", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 5,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="autoRepair", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x6,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="isASPAware",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x7,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="suFailoverDuration",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x8,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 60000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="suFailoverCountMax", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x9,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 10,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName = NULL,
    },
};        

static ClAmfEntityAttributeTableT gClSuAttributeTable[] =
{
    {
        .attrName="adminState",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_ADMIN_STATE_LOCKED_I,
        .attrMinVal = CL_AMS_ADMIN_STATE_NONE + 1,
        .attrMaxVal = CL_AMS_ADMIN_STATE_MAX,
    },
    {
        .attrName="rank", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numComponents", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x3,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="isPreinstantiable", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x4,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="isRestartable", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x5,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName = NULL,
    },
};        

static ClAmfEntityAttributeTableT gClCompAttributeTable[] =
{
    {
        .attrName="numSupportedCSITypes",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="capabilityModel", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY,
        .attrMinVal = 1,
        .attrMaxVal = CL_AMS_COMP_CAP_MAX,
    },
    {
        .attrName="property", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x3,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_COMP_PROPERTY_SA_AWARE,
        .attrMinVal = 1,
        .attrMaxVal = CL_AMS_COMP_PROPERTY_MAX,
    },
    {
        .attrName="isRestartable", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x4,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="nodeRebootCleanupFail", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x5,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_TRUE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE+1,
    },
    {
        .attrName="instantiateLevel",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x6,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxInstantiate",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x7,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 3,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxInstantiateWithDelay", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x8,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 3,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxTerminate", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x9,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxAmStart", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xa,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 3,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxAmStop",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xb,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 3,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxActiveCSIs",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xc,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 1,
        .attrMinVal = 1,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numMaxStandbyCSIs",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xd,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="instantiateTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xe,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="terminateTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0xf,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="cleanupTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x10,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="amStartTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x11,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="amStopTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x12,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="quiescingCompleteTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x13,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="csiSetTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x14,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },

    {
        .attrName="csiRemoveTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x15,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="proxiedInstantiateTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x16,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="proxiedCleanupTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x17,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 20000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="instantiateDelay",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x18,
        .attrDataType = CL_COR_UINT64,
        .attrDefaultVal = 5000,
        .attrMinVal = 1,
        .attrMaxVal = ~0LL>>1,
    },
    {
        .attrName="recoveryOnTimeout",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x19,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_RECOVERY_COMP_FAILOVER,
        .attrMinVal = 1,
        .attrMaxVal = CL_AMS_RECOVERY_INTERNALLY_RECOVERED,
    },
    {
        .attrName = NULL,
    },
};        

static ClAmfEntityAttributeTableT gClSiAttributeTable[] =
{
    {
        .attrName="adminState",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = CL_AMS_ADMIN_STATE_LOCKED_A,
        .attrMinVal = CL_AMS_ADMIN_STATE_NONE + 1,
        .attrMaxVal = CL_AMS_ADMIN_STATE_MAX,
    },
    {
        .attrName="rank", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numCSIs", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x3,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName="numStandbyAssignments", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x4,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName = NULL,
    },
};        

static ClAmfEntityAttributeTableT gClCsiAttributeTable[] =
{
    {
        .attrName="isProxyCSI",
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x1,
        .attrDataType = CL_COR_UINT16,
        .attrDefaultVal = CL_FALSE,
        .attrMinVal = CL_FALSE,
        .attrMaxVal = CL_TRUE + 1,
    },
    {
        .attrName="rank", 
        .attrType=CL_COR_SIMPLE_ATTR,
        .attrFlags = CL_COR_ATTR_CONFIG | CL_COR_ATTR_CACHED | CL_COR_ATTR_PERSISTENT | CL_COR_ATTR_WRITABLE,
        .attrId = COR_AMF_BASE_CLASS_ATTR_ID_END + 0x2,
        .attrDataType = CL_COR_UINT32,
        .attrDefaultVal = 0,
        .attrMinVal = 0,
        .attrMaxVal = ~0U,
    },
    {
        .attrName = NULL,
    },
};        

/*
 *clCorClassAttributeCreate/clCorClassAttributeNameSet/clCorClassAttributeValueSet/clCorClassAttributeUserFlagsSet
*/

typedef struct ClAmfEntityOperationTable
{
    const ClCharT *entityName;
    ClBoolT entitySupported;
    ClAmfEntityAttributeTableT *entityAttrTable;
    ClCorClassTypeT entityClass;
    const ClCorClassTypeT *pEntityParentClass;
    ClCorClassTypeT entityMsoClass;
    ClCorAttrIdT maxAttrId;
    ClBoolT mibAttrTable;
    ClListHeadT list;
}ClAmfEntityOperationTableT;

/*
 * Directly indexable by the entity type.
 */
static ClAmfEntityOperationTableT gClEntityOperationTable[CL_AMS_ENTITY_TYPE_MAX+3] = 
{
    {
        .entityName="AMFEntity",
        .entitySupported = CL_TRUE,
        .entityAttrTable = gClEntityAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = NULL,
    },
    {
        .entityName="AMFNode",
        .entitySupported = CL_TRUE,
        .entityAttrTable= gClNodeAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName = "AMFApp",
        .entitySupported = CL_FALSE,
    },
    {
        .entityName="AMFSg",
        .entitySupported = CL_TRUE,
        .entityAttrTable= gClSgAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName="AMFSu",
        .entitySupported = CL_TRUE,
        .entityAttrTable= gClSuAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName="AMFSi",
        .entitySupported = CL_TRUE,
        .entityAttrTable= gClSiAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName="AMFComp",
        .entitySupported = CL_TRUE,
        .entityAttrTable=gClCompAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName="AMFCsi",
        .entitySupported = CL_TRUE,
        .entityAttrTable=gClCsiAttributeTable,
        .entityClass = 0,
        .pEntityParentClass = &gClEntityOperationTable[0].entityClass,
    },
    {
        .entityName="AMFCluster",
        .entitySupported = CL_FALSE,
    },
    {
        .entityName=NULL,
    },
};

/*
 * Trial and error to get the first free class id.
 */
ClRcT corSetClassIdAllocBase(void)
{
    ClCorClassTypeT start = COR_CLASS_ID_BASE;
    ClRcT rc = CL_OK;
    ClCharT name[CL_COR_MAX_NAME_SZ];
    ClUint32T len = 0;
    do
    {
        rc = clCorClassNameGet(start, name, &len);
    } while(rc == CL_OK && ++start);
    gClCorClassIdAlloc = start;
    clLogNotice("COR", "AMF", "COR class id allocation base [%#x]", gClCorClassIdAlloc);
    return CL_OK;
}

ClRcT corAmfMoClassCreate(const ClCharT *entity,
                          ClCorMOClassPathPtrT moClassPath,
                          ClCorClassTypeT *pMsoClassId)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classId = 0;
    ClCharT msoName[CL_MAX_NAME_LENGTH];

    if((ClUint32T)gClCorClassIdAlloc + 1 < gClCorClassIdAlloc)
    {
        clLogError("COR", "AMF-MSO", "Class  id max limit [%d] reached", gClCorClassIdAlloc);
        return CL_COR_SET_RC(CL_ERR_OUT_OF_RANGE);
    }
    rc = clCorMOClassCreate(moClassPath, 0);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF-MO", "MO class create for [%s] returned [%#x]", 
                   entity, rc);
        goto out;
    }

    clLogNotice("COR", "AMF-MO", "MO class created for [%s]", entity);

    snprintf(msoName, sizeof(msoName), "%s_AMF_MSO", entity);
    classId = gClCorClassIdAlloc++;
    rc = clCorClassCreate(classId, gClEntityOperationTable[0].entityClass);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF-MO", "Class create for MSO class id [%d] returned [%#x]",
                   classId, rc);
        goto out;
    }

    rc = clCorClassNameSet(classId, msoName);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF-MO", "Class name set for class id [%d], [%s] returned [%#x]",
                   classId, msoName, rc);
        goto out;
    }

    /*
     * Create the MSO class
     */
    rc = clCorMSOClassCreate(moClassPath, CL_COR_SVC_ID_AMF_MANAGEMENT, classId);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF-MSO", "MSO class create for class id [%d] returned [%#x]",
                   classId, rc);
        goto out;
    }

    clLogNotice("COR", "AMF-MO", "MSO class created for [%s]", entity);
    
    if(pMsoClassId) *pMsoClassId = classId;

    out:
    return rc;
}

static ClRcT corAmfClassCreate(const ClCharT *entity, ClCorClassTypeT parentClassId, ClCorClassTypeT *pClassId)
{
    ClRcT rc = CL_OK;
    ClCorClassTypeT classId ;

    if((ClUint32T)gClCorClassIdAlloc + 1 < gClCorClassIdAlloc)
    {
        clLogError("COR", "AMF", "Class id max limit [%d] reached", gClCorClassIdAlloc);
        return CL_COR_SET_RC(CL_ERR_OUT_OF_RANGE);
    }
    classId = gClCorClassIdAlloc++;
    rc = clCorClassCreate(classId, parentClassId);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Cor class create failed for entity [%s], class id [%d] with [%#x]",
                   entity, classId, rc);
        goto out;
    }
    rc = clCorClassNameSet(classId, (ClCharT*)entity);
    if(rc != CL_OK)
    {
        clLogError("COR", "AMF", "Cor class name set for [%s] returned [%#x]", 
                   entity, rc);
        goto out;
    }
    
    if(pClassId) *pClassId = classId;

    out:
    return rc;
}

/*
 * Create the entity class attributes for each of the entity classes.
 */
static ClRcT corAmfMibClassExtend(void)
{
    ClRcT rc = CL_OK;
    register ClAmfEntityOperationTableT *pEntityTable = NULL;
    ClAmfEntityAttributeTableT *pEntityAttrTable = NULL;
    ClCorAttrIdT maxAttrId = 0;

    if(!gClEntityOperationTable[0].entityAttrTable)
        return CL_COR_SET_RC(CL_ERR_NOT_SUPPORTED);

    for( pEntityTable = gClEntityOperationTable; pEntityTable->entityName; ++pEntityTable)
    {
        pEntityAttrTable = gClEntityOperationTable[0].entityAttrTable;
        maxAttrId = pEntityTable->maxAttrId + 1;
        if(!pEntityTable->entitySupported
           ||
           !pEntityTable->mibAttrTable) continue;

        for(; pEntityAttrTable->attrName; ++pEntityAttrTable)
        {
            if(pEntityAttrTable->attrType == CL_COR_ARRAY_ATTR)
            {
                rc = clCorClassAttributeArrayCreate(pEntityTable->entityMsoClass,
                                                    maxAttrId,
                                                    pEntityAttrTable->attrDataType,
                                                    (ClUint32T)pEntityAttrTable->attrMaxVal);
            }
            else
            {
                rc = clCorClassAttributeCreate(pEntityTable->entityMsoClass,
                                               maxAttrId,
                                               pEntityAttrTable->attrDataType);
            }
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", 
                           "Cor attribute create for entity [%s], name [%s], class [%#x: %#x] failed with [%#x]",
                           pEntityTable->entityName, pEntityAttrTable->attrName, 
                           pEntityTable->entityMsoClass, 
                           pEntityTable->entityClass,
                           rc);
                goto out_delete;
            }
            rc = clCorClassAttributeNameSet(pEntityTable->entityMsoClass,
                                            maxAttrId,
                                            (char*)pEntityAttrTable->attrName);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Cor attribute name set for entity [%s], name [%s] failed with [%#x]",
                           pEntityTable->entityName, pEntityAttrTable->attrName, rc);
                ++pEntityAttrTable;
                goto out_delete;
            }

            if(pEntityAttrTable->attrType != CL_COR_ARRAY_ATTR)
            {
                rc = clCorClassAttributeValueSet(pEntityTable->entityMsoClass,
                                                 maxAttrId,
                                                 pEntityAttrTable->attrDefaultVal,
                                                 pEntityAttrTable->attrMinVal,
                                                 pEntityAttrTable->attrMaxVal);
                if(rc != CL_OK)
                {
                    clLogError("COR", "AMF", "Cor attribute value set for entity [%s], name [%s] failed with [%#x]",
                               pEntityTable->entityName, pEntityAttrTable->attrName, rc);
                    ++pEntityAttrTable;
                    goto out_delete;
                }
            }

            rc = clCorClassAttributeUserFlagsSet(pEntityTable->entityMsoClass,
                                                 maxAttrId,
                                                 pEntityAttrTable->attrFlags);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Cor attribute user flags set for entity [%s], name [%s] failed with [%#x]",
                           pEntityTable->entityName, pEntityAttrTable->attrName, rc);
                ++pEntityAttrTable;
                goto out_delete;
            }
            clLogDebug("COR", "AMF", "Attribute [%s] created with id [%#x] for class [%#x]",
                       pEntityAttrTable->attrName, maxAttrId, pEntityTable->entityMsoClass);
            ++maxAttrId;
        }
    }
    return rc;
    
    out_delete:
    for(; pEntityTable >= gClEntityOperationTable; --pEntityTable)
    {
        ClCorAttrIdT maxId = maxAttrId;
        ClAmfEntityAttributeTableT *attrIter;
        ClAmfEntityAttributeTableT *attrIterEnd = NULL;
        if(!maxId) 
            maxId = pEntityTable->maxAttrId + 1;
        else
            attrIterEnd = pEntityAttrTable;
        for(attrIter = gClEntityOperationTable[0].entityAttrTable;
            attrIter != attrIterEnd && attrIter->attrName;
            ++attrIter)
        {
            rc = clCorClassAttributeDelete(pEntityTable->entityMsoClass,
                                           maxId);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Cor attribute delete for [%s] failed with [%#x]",
                           attrIter->attrName, rc);
            }
            ++maxId;
        }
        maxAttrId = 0;
    }
    return rc;
}

static ClRcT corAmfAttributeCreate(ClAmfEntityOperationTableT *pEntityTable)
{
    ClRcT rc = CL_OK;
    ClAmfEntityAttributeTableT *pEntityAttrTable = pEntityTable->entityAttrTable;
    if(!pEntityAttrTable) return CL_OK;

    for(; pEntityAttrTable->attrName; ++pEntityAttrTable)
    {
        if(pEntityAttrTable->attrType == CL_COR_ARRAY_ATTR)
        {
            rc = clCorClassAttributeArrayCreate(pEntityTable->entityClass,
                                                pEntityAttrTable->attrId,
                                                pEntityAttrTable->attrDataType,
                                                (ClUint32T)pEntityAttrTable->attrMaxVal);
        }
        else
        {
            rc = clCorClassAttributeCreate(pEntityTable->entityClass,
                                           pEntityAttrTable->attrId,
                                           pEntityAttrTable->attrDataType);
        }
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Cor attribute create for entity [%s], name [%s] failed with [%#x]",
                       pEntityTable->entityName, pEntityAttrTable->attrName, rc);
            goto out_delete;
        }
        rc = clCorClassAttributeNameSet(pEntityTable->entityClass,
                                        pEntityAttrTable->attrId,
                                        (char*)pEntityAttrTable->attrName);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Cor attribute name set for entity [%s], name [%s] failed with [%#x]",
                       pEntityTable->entityName, pEntityAttrTable->attrName, rc);
            ++pEntityAttrTable;
            goto out_delete;
        }

        if(pEntityAttrTable->attrType != CL_COR_ARRAY_ATTR)
        {
            rc = clCorClassAttributeValueSet(pEntityTable->entityClass,
                                             pEntityAttrTable->attrId,
                                             pEntityAttrTable->attrDefaultVal,
                                             pEntityAttrTable->attrMinVal,
                                             pEntityAttrTable->attrMaxVal);
            if(rc != CL_OK)
            {
                clLogError("COR", "AMF", "Cor attribute value set for entity [%s], name [%s] failed with [%#x]",
                           pEntityTable->entityName, pEntityAttrTable->attrName, rc);
                ++pEntityAttrTable;
                goto out_delete;
            }
        }

        rc = clCorClassAttributeUserFlagsSet(pEntityTable->entityClass,
                                             pEntityAttrTable->attrId,
                                             pEntityAttrTable->attrFlags);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Cor attribute user flags set for entity [%s], name [%s] failed with [%#x]",
                       pEntityTable->entityName, pEntityAttrTable->attrName, rc);
            ++pEntityAttrTable;
            goto out_delete;
        }
    }
    return rc;
    
    out_delete:
    for(; --pEntityAttrTable >= pEntityTable->entityAttrTable; )
    {
        rc = clCorClassAttributeDelete(pEntityTable->entityClass,
                                       pEntityAttrTable->attrId);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Cor attribute delete for [%s] failed with [%#x]",
                       pEntityAttrTable->attrName, rc);
        }
    }
    return rc;
}

/*
 * Create all the classes for the AMF entities first.
 */
ClRcT corAmfEntityInitialize(void)
{
    ClRcT rc = CL_OK;
    register ClAmfEntityOperationTableT *entityTable = NULL;

    rc = corSetClassIdAllocBase();
    if(rc != CL_OK) return rc;

    if(gClAmfMibLoaded)
        return corAmfMibClassExtend();

    for(entityTable = gClEntityOperationTable; entityTable->entityName; ++entityTable)
    {
        ClCorClassTypeT classId = 0;
        ClCorClassTypeT parentClassId = 0;

        if(!entityTable->entitySupported) continue;
        parentClassId = entityTable->pEntityParentClass ? *entityTable->pEntityParentClass : 0;

        rc = corAmfClassCreate(entityTable->entityName, parentClassId, &classId);
        if(rc != CL_OK)
        {
            goto out_delete;
        }

        entityTable->entityClass  = classId;
        rc = corAmfAttributeCreate(entityTable);
        if(rc != CL_OK)
        {
            goto out_delete;
        }
    }

    return rc;

    out_delete:
    for(  ; --entityTable >= gClEntityOperationTable ; )
    {
        rc = clCorClassDelete(entityTable->entityClass);
        if(rc != CL_OK)
        {
            clLogError("COR", "AMF", "Class delete for entity [%s], class id [%d] returned [%#x]",
                       entityTable->entityName, entityTable->entityClass, rc);
        }
    }
    return rc;
}

ClRcT corAmfEntityClassGet(ClAmsEntityTypeT type, ClCorClassTypeT *pClassId)
{
    ClRcT rc = CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    if(type > CL_AMS_ENTITY_TYPE_MAX + 1 || !pClassId)
    {
        return rc;
    }

    *pClassId = gClEntityOperationTable[type].entityClass;
    return CL_OK;
}


ClRcT corAmfEntityMibAttributeListGet(ClUint32T *pEntityId,
                                      ClAmsEntityT *pEntity, 
                                      ClAmsEntityConfigT *pEntityConfig,
                                      ClCorAttributeValueListT *pAttrList)
{
    ClRcT rc = CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    typedef struct ClAmfEntityAttributeValueList
    {
        void *pValue;
        ClUint32T length;
    } ClAmfEntityAttributeValueListT;
    ClAmfEntityAttributeValueListT baseAttrValueList[] = { 
        {(void*)&pEntity->type, (ClUint32T)sizeof(pEntity->type)},
        {(void*)pEntity->name.value, pEntity->name.length },
    };
    ClAmfEntityAttributeTableT *attrTable = NULL;
    ClCorAttrIdT entityAttrId;
    ClInt32T baseIndex = 0;
    register ClInt32T i;

    if(pEntity->type > CL_AMS_ENTITY_TYPE_MAX) return rc;
    entityAttrId = gClEntityOperationTable[pEntity->type].maxAttrId + 1;
    pAttrList->numOfValues = (ClUint32T)sizeof(baseAttrValueList)/sizeof(baseAttrValueList[0]);
    pAttrList->pAttributeValue = clHeapCalloc(pAttrList->numOfValues+1, sizeof(*pAttrList->pAttributeValue));
    CL_ASSERT(pAttrList->pAttributeValue != NULL);
    
    /*
     * Check for an initialized attribute in the entity.
     */
    attrTable = gClEntityOperationTable[pEntity->type].entityAttrTable;
    if(!attrTable)
    {
        goto out_free;
    }
    
    for(i = 0; attrTable[i].attrName; ++i)
    {
        if((attrTable[i].attrFlags  & CL_COR_ATTR_INITIALIZED))
        {
            CL_ASSERT(attrTable[i].attrDataType == CL_COR_UINT32);
            if(!pEntityId)
            {
                rc = CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
                goto out_free;
            }
            pAttrList->pAttributeValue[baseIndex].attrId = attrTable[i].attrId;
            pAttrList->pAttributeValue[baseIndex].index = CL_COR_INVALID_ATTR_IDX;
            pAttrList->pAttributeValue[baseIndex].bufferPtr = (ClPtrT)pEntityId;
            pAttrList->pAttributeValue[baseIndex].bufferSize = sizeof(*pEntityId);
            ++baseIndex;
            break;
        }
    }

    attrTable = gClEntityOperationTable[0].entityAttrTable;
    if(!attrTable) 
    {
        goto out_free;
    }

    for( i = 0; i < pAttrList->numOfValues; ++i)
    {
        ClInt32T attrIndex = CL_COR_INVALID_ATTR_IDX;
        if(attrTable[i].attrType == CL_COR_ARRAY_ATTR)
            attrIndex = 0;
        pAttrList->pAttributeValue[baseIndex+i].attrId = entityAttrId;
        pAttrList->pAttributeValue[baseIndex+i].index = attrIndex;
        pAttrList->pAttributeValue[baseIndex+i].bufferPtr = (ClPtrT)baseAttrValueList[i].pValue;
        pAttrList->pAttributeValue[baseIndex+i].bufferSize = baseAttrValueList[i].length;
        ++entityAttrId;
    }

    pAttrList->numOfValues += baseIndex;
    return CL_OK;

    out_free:
    clHeapFree(pAttrList->pAttributeValue);
    pAttrList->pAttributeValue = NULL;
    return rc;
}

/*
 * Set the base class/entity_class attributes
 */
ClRcT corAmfEntityAttributeListGet(ClUint32T *pEntityId,
                                   ClAmsEntityT *pEntity, 
                                   ClAmsEntityConfigT *pEntityConfig,
                                   ClCorAttributeValueListT *pAttrList)
{
    ClRcT rc = CL_OK;
    if(gClAmfMibLoaded)
        rc = corAmfEntityMibAttributeListGet(pEntityId, pEntity, pEntityConfig, pAttrList);
    else
    {
        typedef struct ClAmfEntityAttributeValueList
        {
            void *pValue;
            ClUint32T length;
        } ClAmfEntityAttributeValueListT;
        ClAmfEntityAttributeValueListT baseAttrValueList[] = { 
            {(void*)&pEntity->type, (ClUint32T)sizeof(pEntity->type)},
                {(void*)pEntity->name.value, pEntity->name.length },
        };
        ClAmfEntityAttributeTableT *attrTable = NULL;
        register ClInt32T i;

        pAttrList->numOfValues = (ClUint32T)sizeof(baseAttrValueList)/sizeof(baseAttrValueList[0]);
        pAttrList->pAttributeValue = clHeapCalloc(pAttrList->numOfValues, sizeof(*pAttrList->pAttributeValue));
        CL_ASSERT(pAttrList->pAttributeValue != NULL);

        attrTable = gClEntityOperationTable[0].entityAttrTable;
        if(!attrTable) return CL_COR_SET_RC(CL_ERR_NOT_EXIST);

        for( i = 0; i < pAttrList->numOfValues; ++i)
        {
            ClInt32T attrIndex = CL_COR_INVALID_ATTR_IDX;
            if(attrTable[i].attrType == CL_COR_ARRAY_ATTR)
                attrIndex = 0;
            pAttrList->pAttributeValue[i].attrId = attrTable[i].attrId;
            pAttrList->pAttributeValue[i].index = attrIndex;
            pAttrList->pAttributeValue[i].bufferPtr = (ClPtrT)baseAttrValueList[i].pValue;
            pAttrList->pAttributeValue[i].bufferSize = baseAttrValueList[i].length;
        }
    }
    return rc;
}

static const ClCharT *const gClAmfMoClassStrTable[] = {
    "saAmfEntityTable",
    "saAmfNodeTable",
    "saAmfAppTable",
    "saAmfSGTable",
    "saAmfSUTable",
    "saAmfSITable",
    "saAmfCompTable",
    "saAmfCSITable",
    NULL,
};

static const ClCharT *const gClAmfMsoClassStrTable[] = {
    "CLASS_SAAMFENTITYTABLE",
    "CLASS_SAAMFNODETABLE",
    "CLASS_SAAMFAPPLICATIONTABLE",
    "CLASS_SAAMFSGTABLE",
    "CLASS_SAAMFSUTABLE",
    "CLASS_SAAMFSITABLE",
    "CLASS_SAAMFCOMPTABLE",
    "CLASS_SAAMFCSITABLE",
    NULL,
};

static const ClCharT *const gClAmfExtendedMoClassStrTable[] = { 
    "saAmfScalars",
    "saAmfSUsperSIRankTable",
    "saAmfSGSIRankTable",
    "saAmfSGSURankTable",
    "saAmfSISIDepTable",
    "saAmfCompCSTypeSupportedTable",
    "saAmfCSICSIDepTable",
    "saAmfCSINameValueTable",    
    "saAmfCSTypeAttrNameTable",
    "saAmfSUSITable",
    "saAmfHealthCheckTable",
    "saAmfSCompCsiTable",
    "saAmfProxyProxiedTable",
    "saAmfTrapObject",
    NULL,
};

static const ClCharT *const gClAmfExtendedMsoClassStrTable[] = { 
    "CLASS_SAAMFSCALARS",
    "CLASS_SAAMFSUSPERSIRANKTABLE",
    "CLASS_SAAMFSGSIRANKTABLE",
    "CLASS_SAAMFSGSURANKTABLE",
    "CLASS_SAAMFSISIDEPTABLE",
    "CLASS_SAAMFCOMPCSTYPESUPPORTEDTABLE",
    "CLASS_SAAMFCSICSIDEPTABLE",
    "CLASS_SAAMFCSINAMEVALUETABLE",
    "CLASS_SAAMFCSTYPEATTRNAMETABLE",
    "CLASS_SAAMFSUSITABLE",
    "CLASS_SAAMFHEALTHCHECKTABLE",
    "CLASS_SAAMFSCOMPCSITABLE",
    "CLASS_SAAMFPROXYPROXIEDTABLE",
    "CLASS_SAAMFTRAPOBJECT",
    NULL,
};

#define AMF_MO_CLASS_PREFIX "saAmf"
#define AMF_MSO_CLASS_PREFIX "CLASS_SAAMF"

static CL_LIST_HEAD_DECLARE(gClExtendedEntityOperationTable);

static __inline__ ClBoolT corClassTableLookup(const ClCharT *name,
                                              const ClCharT *const *table,
                                              ClInt32T *index)
{
    register ClInt32T i;
    for(i = 0; table[i]; ++i)
        if(!strncmp(table[i], name, strlen(table[i])))
        {
            if(index)
                *index = i;
            return CL_TRUE;
        }
    return CL_FALSE;
}

static void corAmfExtendedEntityOperationTableAdd(ClCorModuleInfoT *info)
{
    ClAmfEntityOperationTableT *opTable = NULL;
    opTable = clHeapCalloc(1, sizeof(*opTable));
    CL_ASSERT(opTable != NULL);
    opTable->entityClass = info->classId;
    opTable->entityName = clHeapCalloc(1, strlen(info->className)+1);
    CL_ASSERT(opTable->entityName != NULL);
    strcpy((ClCharT*)opTable->entityName, info->className);
    clListAddTail(&opTable->list, &gClExtendedEntityOperationTable);
}

static ClAmfEntityOperationTableT *corAmfExtendedEntityOperationTableFind(const ClCharT *name)
{
    ClListHeadT *iter;
    CL_LIST_FOR_EACH(iter, &gClExtendedEntityOperationTable)
    {
        ClAmfEntityOperationTableT *opTable = CL_LIST_ENTRY(iter, ClAmfEntityOperationTableT, list);
        if(!strncmp(opTable->entityName, name, strlen(opTable->entityName)))
            return opTable;
    }
    return NULL;
}

static ClRcT corAmfMibExtendedClassAttributeListGet(const ClCharT *pExtendedEntityClassName,
                                                    ClCorAttributeValueListT *attrList,
                                                    ClCorClassTypeT *pClassType)
{
    ClUint32T numKeys = 0;
    ClAmfEntityOperationTableT *opTable = NULL;
    ClInt32T i;

    if(!pExtendedEntityClassName || !attrList || !pClassType)
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

    opTable = corAmfExtendedEntityOperationTableFind(pExtendedEntityClassName);
    if(!opTable || !opTable->entityAttrTable)
    {
        if(!opTable)
            clLogError("COR", "AMF", "Class entry [%s] not found in the entity table",
                       pExtendedEntityClassName);
        else
            clLogError("COR", "AMF", "Class entry [%s] has no attribute list",
                       pExtendedEntityClassName);
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    }
    attrList->pAttributeValue = NULL;
    attrList->numOfValues = 0;
    for(i = 0; opTable->entityAttrTable[i].attrName; ++i)
    {
        if((opTable->entityAttrTable[i].attrFlags & CL_COR_ATTR_INITIALIZED))
        {
            if(!(numKeys & 3))
            {
                attrList->pAttributeValue = clHeapRealloc(attrList->pAttributeValue,
                                                          sizeof(*attrList->pAttributeValue) * (numKeys + 4));
                CL_ASSERT(attrList->pAttributeValue != NULL);
                memset(attrList->pAttributeValue + numKeys, 0,
                       sizeof(*attrList->pAttributeValue) * 4);
            }
            CL_ASSERT(opTable->entityAttrTable[i].attrDataType == CL_COR_UINT32);
            attrList->pAttributeValue[numKeys].attrId = opTable->entityAttrTable[i].attrId;
            attrList->pAttributeValue[numKeys].index = CL_COR_INVALID_ATTR_IDX;
            attrList->pAttributeValue[numKeys].bufferPtr = NULL;
            attrList->pAttributeValue[numKeys].bufferSize = 0;
            ++numKeys;
        }
    }
    if(!numKeys)
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    attrList->numOfValues = numKeys;
    *pClassType = opTable->entityClass;
    return CL_OK;
    
}

ClRcT corAmfExtendedClassAttributeListGet(const ClCharT *pExtendedClassName,
                                          ClCorAttributeValueListT *attrList,
                                          ClCorClassTypeT *pClassType)
{
    if(!gClAmfMibLoaded)
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    return corAmfMibExtendedClassAttributeListGet(pExtendedClassName, attrList, pClassType);
}

static void corClassLoadAttrInfo(ClAmfEntityOperationTableT *opTable,
                                 ClCorAttrTypeT attrType,
                                 ClListHeadT *attrList,
                                 ClInt32T *index,
                                 ClCorAttrIdT *maxAttrId)
{
    ClInt32T c = 0;
    ClCorAttrIdT max = 0;
    if(!index || !maxAttrId) return;

    c = *index;
    max = *maxAttrId;
    while(!CL_LIST_HEAD_EMPTY(attrList))
    {
        ClListHeadT *head = attrList->pNext;
        corAttrIntf_t *attr = CL_LIST_ENTRY(head, corAttrIntf_t, list);
        clListDel(head);
        if(!(c & 15))
        {
            opTable->entityAttrTable = clHeapRealloc(opTable->entityAttrTable,
                                                     (c+16) * sizeof(*opTable->entityAttrTable));
            CL_ASSERT(opTable->entityAttrTable != NULL);
        }
        opTable->entityAttrTable[c].attrName = "unused";
        opTable->entityAttrTable[c].attrType = attrType;
        opTable->entityAttrTable[c].attrFlags = attr->flags;
        opTable->entityAttrTable[c].attrId = attr->attrId;
        opTable->entityAttrTable[c].attrDataType = attr->attrType;
        opTable->entityAttrTable[c].attrMinVal = attr->min;
        opTable->entityAttrTable[c].attrMaxVal = attr->max;
        opTable->entityAttrTable[c].attrDefaultVal = attr->init;
        ++c;
        if(attr->attrId > max)
            max = attr->attrId;
        clHeapFree(attr);
    }
    *index = c;
    *maxAttrId = max;
}


/*
 * Initialize the AMF class information.
 */
ClRcT clCorAmfModuleHandler(ClCorModuleInfoT *info)
{
    ClBoolT moPresent = CL_FALSE;
    ClBoolT msoPresent = CL_FALSE;
    ClInt32T moPrefix = 0;
    ClInt32T msoPrefix = 0;
    ClInt32T index = 0;
    if(!info) return CL_COR_SET_RC(CL_ERR_NOT_EXIST);

    if( ((moPrefix = strncmp(info->className, AMF_MO_CLASS_PREFIX, strlen(AMF_MO_CLASS_PREFIX))) != 0)
       &&
        (msoPrefix = strncmp(info->className, AMF_MSO_CLASS_PREFIX, strlen(AMF_MSO_CLASS_PREFIX) )) != 0)
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);

    if(!gClAmfMibLoaded)
        gClAmfMibLoaded = CL_TRUE;

    if(!moPrefix)
    {
        moPresent = corClassTableLookup(info->className, gClAmfMoClassStrTable, &index);
        if(!moPresent)
        {
            moPresent = corClassTableLookup(info->className, gClAmfExtendedMoClassStrTable, &index);
            if(!moPresent)
                return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
            corAmfExtendedEntityOperationTableAdd(info);
        }
        else
        {
            if(!gClEntityOperationTable[index].entitySupported)
                return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
            gClEntityOperationTable[index].entityAttrTable = NULL;
            gClEntityOperationTable[index].mibAttrTable = CL_TRUE;
            gClEntityOperationTable[index].entityClass = info->classId;
        }
        clLogDebug("COR", "AMF", "Module handler for MO class [%s:%#x] added", 
                   info->className, info->classId);
    }
    else
    {
        ClInt32T c = 0;
        ClCorAttrIdT maxAttrId = 0;
        ClAmfEntityOperationTableT *opTable = NULL;
        msoPresent = corClassTableLookup(info->className, gClAmfMsoClassStrTable, &index);
        if(!msoPresent)
        {
            msoPresent = corClassTableLookup(info->className, gClAmfExtendedMsoClassStrTable, &index);
            if(!msoPresent)
                return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
            opTable = corAmfExtendedEntityOperationTableFind(gClAmfExtendedMoClassStrTable[index]);
            if(!opTable)
            {
                clLogError("COR", "AMF", "MO class not found for mso class [%s] in entity op table",
                           info->className);
                return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
            }
        }
        else
        {
            opTable = gClEntityOperationTable + index;
            if(!opTable->entitySupported)
                return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
        }
        opTable->entityAttrTable = NULL;
        if(!CL_LIST_HEAD_EMPTY(&info->simpleAttrList))
            corClassLoadAttrInfo(opTable, CL_COR_SIMPLE_ATTR, &info->simpleAttrList, &c, &maxAttrId);
        if(!CL_LIST_HEAD_EMPTY(&info->arrayAttrList))
            corClassLoadAttrInfo(opTable, CL_COR_ARRAY_ATTR, &info->arrayAttrList, &c, &maxAttrId);
        if(!CL_LIST_HEAD_EMPTY(&info->contAttrList))
            corClassLoadAttrInfo(opTable, CL_COR_CONTAINMENT_ATTR, &info->contAttrList, &c, &maxAttrId);
        if(!CL_LIST_HEAD_EMPTY(&info->assocAttrList))
            corClassLoadAttrInfo(opTable, CL_COR_ASSOCIATION_ATTR, &info->assocAttrList, &c, &maxAttrId);

        if(!(c & 15))
        {
            opTable->entityAttrTable = clHeapRealloc(opTable->entityAttrTable,
                                                     (c+1) * sizeof(*opTable->entityAttrTable));
            CL_ASSERT(opTable->entityAttrTable != NULL);
        }
        opTable->entityAttrTable[c].attrName = NULL;
        opTable->entityMsoClass = info->classId;
        opTable->maxAttrId = maxAttrId;
        clLogDebug("COR", "AMF", "Module handler for MSO class [%s:%#x] added", info->className, info->classId);
    }

    return CL_OK;
}
