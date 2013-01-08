#include "clAmfMgmt.h"

ClAmsMgmtHandleT gClAmsMgmtHandle;

#define CHECK_ATTRIBUTE_LIST(numAttrs) do {                             \
        if(!( (numAttrs) & 7))                                          \
        {                                                               \
            pAttrList->pAttributeValue = clHeapRealloc(pAttrList->pAttributeValue, \
                                                       sizeof(*pAttrList->pAttributeValue) * ((numAttrs)+8)); \
            CL_ASSERT(pAttrList->pAttributeValue != NULL);              \
            memset(pAttrList->pAttributeValue + (numAttrs), 0, sizeof(*pAttrList->pAttributeValue)*8); \
        }                                                               \
}while(0)


static ClRcT amfSGExtendedSGSIRankConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                      ClCorClassTypeT *pClassType,
                                                      ClCorAttributeValueListPtrT pAttrList)
{                                                                       
    if(!pConfig || !pClassType || !pAttrList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    *pClassType = CLASS_SAAMFSGSIRANKTABLE_MO;
    return CL_OK;
}

static ClRcT amfSGExtendedSGSURankConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                      ClCorClassTypeT *pClassType,
                                                      ClCorAttributeValueListPtrT pAttrList)
{                                                                       
    if(!pConfig || !pClassType || !pAttrList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    *pClassType = CLASS_SAAMFSGSURANKTABLE_MO;
    return CL_OK;
}

static ClRcT amfSGConfigAttributesGet(ClAmsSGConfigT *pSGConfig, ClCorAttributeValueListPtrT pAttrList)
{                                                                       
    ClRcT rc = CL_OK;
    ClUint32T numAttrs = 0;
    ClUint32T *pValue = NULL;
    
    if(!pSGConfig || !pAttrList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    pAttrList->pAttributeValue = clHeapRealloc(pAttrList->pAttributeValue,
                                               sizeof(*pAttrList->pAttributeValue) * (numAttrs + 8));
    CL_ASSERT(pAttrList->pAttributeValue != NULL);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGNAME;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pSGConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs].bufferSize = pSGConfig->entity.name.length;
    pAttrList->pAttributeValue[numAttrs++].index = 0;

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(pSGConfig->redundancyModel)
        *pValue = pSGConfig->redundancyModel - 1;
    else
        *pValue = 0;
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGREDMODEL;
    pAttrList->pAttributeValue[numAttrs].index = -100;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGNUMPREFACTIVESUS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->numPrefActiveSUs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->numPrefActiveSUs);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGNUMPREFSTANDBYSUS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->numPrefStandbySUs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->numPrefStandbySUs);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGNUMPREFINSERVICESUS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->numPrefInserviceSUs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->numPrefInserviceSUs);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGNUMPREFASSIGNEDSUS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->numPrefAssignedSUs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T) sizeof(pSGConfig->numPrefAssignedSUs);

    CHECK_ATTRIBUTE_LIST(numAttrs);


    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGMAXACTIVESISPERSU;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->maxActiveSIsPerSU;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->maxActiveSIsPerSU);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGMAXSTANDBYSISPERSU;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->maxStandbySIsPerSU;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->maxStandbySIsPerSU);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGCOMPRESTARTPROB;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->compRestartDuration;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->compRestartDuration);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGCOMPRESTARTMAX;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->compRestartCountMax;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->compRestartCountMax);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGSURESTARTPROB;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->suRestartDuration;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->suRestartDuration);
    
    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGSURESTARTMAX;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->suRestartCountMax;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->suRestartCountMax);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    
    if(pSGConfig->autoRepair)
    {
        *pValue = 1;
    }
    else
    {
        *pValue = 2;
    }
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGAUTOREPAIROPTION;
    pAttrList->pAttributeValue[numAttrs].index = -100; /*hint to free the buffer ptr*/
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(pSGConfig->autoAdjust)
    {
        *pValue = 1;
    }
    else
    {
        *pValue = 2;
    }
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGAUTOADJUSTOPTION;
    pAttrList->pAttributeValue[numAttrs].index = -100; /*hint to free bufferptr*/
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSGTABLE_SAAMFSGAUTOADJUSTPROB;
    pAttrList->pAttributeValue[numAttrs].index = 0; 
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSGConfig->autoAdjustProbation;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSGConfig->autoAdjustProbation);
    
    pAttrList->numOfValues = numAttrs;
    return rc;

}

static ClRcT amfSIExtendedSISIDepConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                     ClCorClassTypeT *pClassType,
                                                     ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;

    if(!pConfig || !pClassType || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *pClassType = CLASS_SAAMFSISIDEPTABLE_MO;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSISIDEPTABLE_SAAMFSISIDEPSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSISIDEPTABLE_SAAMFSISIDEPDEPNDSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->containedEntity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->containedEntity.name.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfSIExtendedSUsperSIRankConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                          ClCorClassTypeT *pClassType,
                                                          ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;

    if(!pConfig || !pClassType || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *pClassType = CLASS_SAAMFSUSPERSIRANKTABLE_MO;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUSPERSIRANKTABLE_SAAMFSUSPERSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUSPERSIRANKTABLE_SAAMFSUSPERSISUNAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->containedEntity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->containedEntity.name.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfSIConfigAttributesGet(ClAmsSIConfigT *pSIConfig, ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;
    ClUint32T *pValue = NULL;

    if(!pSIConfig || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    pAttrList->pAttributeValue = clHeapRealloc(pAttrList->pAttributeValue,
                                               sizeof(*pAttrList->pAttributeValue) * (numAttrs + 8));
    CL_ASSERT(pAttrList->pAttributeValue != NULL);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSITABLE_SAAMFSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pSIConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pSIConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(!pSIConfig->rank)
        *pValue = 1;
    else
        *pValue = pSIConfig->rank;
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSITABLE_SAAMFSIRANK;
    pAttrList->pAttributeValue[numAttrs].index = -100;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);
    
    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSITABLE_SAAMFSINUMCSIS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSIConfig->numCSIs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSIConfig->numCSIs);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSITABLE_SAAMFSIPROTECTEDBYSG;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pSIConfig->parentSG.entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pSIConfig->parentSG.entity.name.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfCSIExtendedCSINVPConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                     ClCorClassTypeT *pClassType,
                                                     ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;
    ClAmsMgmtOICSINVPConfigT *pCSINVPConfig = (ClAmsMgmtOICSINVPConfigT *)pConfig;

    if(!pConfig || !pClassType || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *pClassType = CLASS_SAAMFCSINAMEVALUETABLE_MO;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSINAMEVALUETABLE_SAAMFCSINAMEVALUECSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSINAMEVALUETABLE_SAAMFCSINAMEVALUEATTRNAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCSINVPConfig->nvp.paramName.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCSINVPConfig->nvp.paramName.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSINAMEVALUETABLE_SAAMFCSINAMEVALUEATTRVALUE;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCSINVPConfig->nvp.paramValue.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCSINVPConfig->nvp.paramValue.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfCSIExtendedCSICSIDepConfigAttributesGet(ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                        ClCorClassTypeT *pClassType,
                                                        ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;

    if(!pConfig || !pClassType || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    *pClassType = CLASS_SAAMFCSICSIDEPTABLE_MO;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSICSIDEPTABLE_SAAMFCSICSIDEPCSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSICSIDEPTABLE_SAAMFCSICSIDEPDEPNDCSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pConfig->containedEntity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pConfig->containedEntity.name.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfCSIConfigAttributesGet(ClAmsCSIConfigT *pCSIConfig, ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;

    if(!pCSIConfig || !pAttrList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    pAttrList->pAttributeValue = clHeapRealloc(pAttrList->pAttributeValue,
                                               sizeof(*pAttrList->pAttributeValue) * (numAttrs + 8));
    CL_ASSERT(pAttrList->pAttributeValue != NULL);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSITABLE_SAAMFCSINAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCSIConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCSIConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCSITABLE_SAAMFCSTYPE;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCSIConfig->type.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCSIConfig->type.length;
    
    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfNodeConfigAttributesGet(ClAmsNodeConfigT *pNodeConfig, ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;
    ClUint32T *pValue = NULL;

    if(!pNodeConfig || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    pAttrList->pAttributeValue = clHeapRealloc(pAttrList->pAttributeValue,
                                               sizeof(*pAttrList->pAttributeValue) * (numAttrs+8));
    CL_ASSERT(pAttrList->pAttributeValue != NULL);

    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFNODETABLE_SAAMFNODENAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pNodeConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pNodeConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFNODETABLE_SAAMFNODESUFAILOVERMAX;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pNodeConfig->suFailoverCountMax;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pNodeConfig->suFailoverCountMax);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFNODETABLE_SAAMFNODESUFAILOVERPROB;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pNodeConfig->suFailoverDuration;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pNodeConfig->suFailoverDuration);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(pNodeConfig->autoRepair)
        *pValue = 1;
    else
        *pValue = 2;
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFNODETABLE_SAAMFNODEAUTOREPAIROPTION;
    pAttrList->pAttributeValue[numAttrs].index = -100;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfSUConfigAttributesGet(ClAmsSUConfigT *pSUConfig, ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;
    ClInt32T *pValue = NULL;

    if(!pSUConfig || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    
    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUTABLE_SAAMFSUNAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pSUConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pSUConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUTABLE_SAAMFSURANK;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSUConfig->rank;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSUConfig->rank);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUTABLE_SAAMFSUNUMCOMPONENTS;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pSUConfig->numComponents;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pSUConfig->numComponents);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(pSUConfig->isRestartable)
        *pValue = 1;
    else
        *pValue = 0;
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFSUTABLE_SAAMFSURESTARTSTATE;
    pAttrList->pAttributeValue[numAttrs].index = -100;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfCompConfigAttributesGet(ClAmsCompConfigT *pCompConfig, ClCorAttributeValueListPtrT pAttrList)
{
    ClUint32T numAttrs = 0;
    ClUint32T *pValue = NULL;

    if(!pCompConfig || !pAttrList)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPNAME;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCompConfig->entity.name.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCompConfig->entity.name.length;

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPCAPABILITY;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->capabilityModel;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->capabilityModel);
    
    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPCATEGORY;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->property;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->property);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPINSTANTIATIONLEVEL;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->instantiateLevel;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->instantiateLevel);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPRECOVERYONERROR;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->recoveryOnTimeout;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->recoveryOnTimeout);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPNUMMAXINSTANTIATEWITHOUTDELAY;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->numMaxInstantiate;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->numMaxInstantiate);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPNUMMAXINSTANTIATEWITHDELAY;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->numMaxInstantiateWithDelay;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->numMaxInstantiateWithDelay);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pValue = clHeapCalloc(1, sizeof(*pValue));
    CL_ASSERT(pValue != NULL);
    if(pCompConfig->isRestartable)
        *pValue = 2;
    else
        *pValue = 1;
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPDISABLERESTART;
    pAttrList->pAttributeValue[numAttrs].index = -100;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pValue;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(*pValue);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPNUMMAXACTIVECSI;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->numMaxActiveCSIs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->numMaxActiveCSIs);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPNUMMAXSTANDBYCSI;
    pAttrList->pAttributeValue[numAttrs].index = CL_COR_INVALID_ATTR_IDX;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->numMaxStandbyCSIs;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->numMaxStandbyCSIs);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPINSTANTIATECMD;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCompConfig->instantiateCommand;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = strlen(pCompConfig->instantiateCommand);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPINSTANTIATETIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.instantiate;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.instantiate);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPDELAYBETWEENINSTANTIATEATTEMPTS;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.instantiateDelay;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.instantiateDelay);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPTERMINATECALLBACKTIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.terminate;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.terminate);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPCLEANUPTIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.cleanup;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.cleanup);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPCSISETCALLBACKTIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.csiSet;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.csiSet);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPQUIESCINGCOMPLETETIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.quiescingComplete;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.quiescingComplete);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPCSIRMVCALLBACKTIMEOUT;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = &pCompConfig->timeouts.csiRemove;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = (ClUint32T)sizeof(pCompConfig->timeouts.csiRemove);

    CHECK_ATTRIBUTE_LIST(numAttrs);
    pAttrList->pAttributeValue[numAttrs].attrId = SAAMFCOMPTABLE_SAAMFCOMPPROXYCSI;
    pAttrList->pAttributeValue[numAttrs].index = 0;
    pAttrList->pAttributeValue[numAttrs].bufferPtr = pCompConfig->proxyCSIType.value;
    pAttrList->pAttributeValue[numAttrs++].bufferSize = pCompConfig->proxyCSIType.length;

    pAttrList->numOfValues = numAttrs;
    return CL_OK;
}

static ClRcT amfEntityConfigAttributesGet(ClAmsEntityConfigT *pEntityConfig, ClCorAttributeValueListPtrT pAttrList)
{
    if(!pEntityConfig || !pAttrList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(pAttrList->pAttributeValue) 
    {
        clHeapFree(pAttrList->pAttributeValue);
        pAttrList->pAttributeValue = NULL;
    }
    pAttrList->numOfValues = 0;
    switch(pEntityConfig->type)
    {
    case CL_AMS_ENTITY_TYPE_SG:
        return amfSGConfigAttributesGet((ClAmsSGConfigT*)pEntityConfig, pAttrList);
    case CL_AMS_ENTITY_TYPE_SI:
        return amfSIConfigAttributesGet((ClAmsSIConfigT*)pEntityConfig, pAttrList);
    case CL_AMS_ENTITY_TYPE_CSI:
        return amfCSIConfigAttributesGet((ClAmsCSIConfigT*)pEntityConfig, pAttrList);
    case CL_AMS_ENTITY_TYPE_NODE:
        return amfNodeConfigAttributesGet((ClAmsNodeConfigT*)pEntityConfig, pAttrList);
    case CL_AMS_ENTITY_TYPE_SU:
        return amfSUConfigAttributesGet((ClAmsSUConfigT*)pEntityConfig, pAttrList);
    case CL_AMS_ENTITY_TYPE_COMP:
        return amfCompConfigAttributesGet((ClAmsCompConfigT*)pEntityConfig, pAttrList);
    default:
        break;
    }
    return CL_OK;
}

static ClRcT amfEntityExtendedConfigAttributesGet(ClAmsMgmtOIExtendedClassTypeT type,
                                                  ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                                  ClCorClassTypeT *pClassType,
                                                  ClCorAttributeValueListPtrT pAttrList)
{
    if(type >= CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX || !pConfig || !pClassType || !pAttrList) 
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    if(pAttrList->pAttributeValue) 
    {
        clHeapFree(pAttrList->pAttributeValue);
        pAttrList->pAttributeValue = NULL;
    }
    pAttrList->numOfValues = 0;
    switch(type)
    {
    case CL_AMS_MGMT_OI_SGSIRANK:
        return amfSGExtendedSGSIRankConfigAttributesGet(pConfig, pClassType, pAttrList);
    case CL_AMS_MGMT_OI_SGSURANK:
        return amfSGExtendedSGSURankConfigAttributesGet(pConfig, pClassType, pAttrList);
    case CL_AMS_MGMT_OI_SUSPERSIRANK:
        return amfSIExtendedSUsperSIRankConfigAttributesGet(pConfig, pClassType, pAttrList);
    case CL_AMS_MGMT_OI_SISIDEP:
        return amfSIExtendedSISIDepConfigAttributesGet(pConfig, pClassType, pAttrList);
    case CL_AMS_MGMT_OI_CSINAMEVALUE:
        return amfCSIExtendedCSINVPConfigAttributesGet(pConfig, pClassType, pAttrList);
    case CL_AMS_MGMT_OI_CSICSIDEP:
        return amfCSIExtendedCSICSIDepConfigAttributesGet(pConfig, pClassType, pAttrList);
    default:
        break;
    }
    return CL_OK;
}

ClRcT clAmfMgmtCacheInitialize(ClAmsMgmtHandleT *pHandle)
{
    ClRcT rc = clAmsMgmtOIInitialize(pHandle, amfEntityConfigAttributesGet,
                                     amfEntityExtendedConfigAttributesGet);
    if(rc == CL_OK && pHandle)
    {
        if(gClAmsMgmtHandle)
        {
            (void)clAmsMgmtFinalize(gClAmsMgmtHandle);
        }
        gClAmsMgmtHandle = *pHandle;
    }
    return rc;
}
