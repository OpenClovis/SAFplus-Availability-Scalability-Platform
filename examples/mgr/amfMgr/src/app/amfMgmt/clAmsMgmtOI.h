#ifndef _CL_AMS_MGMT_OI_H_
#define _CL_AMS_MGMT_OI_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clHash.h>
#include <clAmsErrors.h>
#include <clAmsMgmtClientApi.h>
#include <clCorApi.h>
#include <clCorTxnApi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ClAmsMgmtOIExtendedClassType
{
    CL_AMS_MGMT_OI_SGSIRANK,
    CL_AMS_MGMT_OI_SGSURANK,
    CL_AMS_MGMT_OI_SUSPERSIRANK,    
    CL_AMS_MGMT_OI_SISIDEP,
    CL_AMS_MGMT_OI_CSICSIDEP,
    CL_AMS_MGMT_OI_CSINAMEVALUE,
    CL_AMS_MGMT_OI_SUSI,
    CL_AMS_MGMT_OI_EXTENDED_CLASS_MAX,
}ClAmsMgmtOIExtendedClassTypeT;

typedef struct ClAmsMgmtOIExtendedEntityConfig
{
    ClAmsEntityT entity;
    ClAmsEntityT containedEntity;
}ClAmsMgmtOIExtendedEntityConfigT;

typedef struct ClAmsMgmtOICSINVPConfig
{
    ClAmsMgmtOIExtendedEntityConfigT config;
    ClAmsCSINVPT nvp;
}ClAmsMgmtOICSINVPConfigT;

extern ClRcT clAmsMgmtOIInitialize(ClAmsMgmtHandleT *pHandle, 
                                   ClRcT (*pClAmsMgmtOIConfigAttributesGet)
                                   (ClAmsEntityConfigT *, ClCorAttributeValueListPtrT pAttrList),
                                   ClRcT (*pClAmsMgmtOIExtendedConfigAttributesGet)
                                   (ClAmsMgmtOIExtendedClassTypeT type,
                                    ClAmsMgmtOIExtendedEntityConfigT *pConfig,
                                    ClCorClassTypeT *pClassType,
                                    ClCorAttributeValueListT *pAttrList));
extern ClRcT clAmsMgmtOIGet(ClCorMOIdT *pMoId, ClAmsEntityT *pEntity);
extern ClRcT clAmsMgmtOIExtendedGet(ClCorMOIdT *pMoId, ClAmsMgmtOIExtendedClassTypeT type,
                                    ClAmsMgmtOIExtendedEntityConfigT *pConfig, ClUint32T configSize);
extern ClRcT clAmsMgmtOIIndexGet(ClAmsEntityTypeT type, ClCorInstanceIdT *pIndex);
extern ClRcT clAmsMgmtOIIndexPut(ClAmsEntityTypeT type, ClCorInstanceIdT index);
extern ClRcT clAmsMgmtOIDelete(ClCorInstanceIdT instance, ClAmsEntityT *entity);
extern ClRcT clAmsMgmtOIExtendedDelete(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT instance);
extern ClRcT clAmsMgmtOIExtendedIndexGet(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT *pIndex);
extern ClRcT clAmsMgmtOIExtendedIndexPut(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT index);
extern ClRcT clAmsMgmtOIExtendedDelete(ClAmsMgmtOIExtendedClassTypeT type, ClCorInstanceIdT instance);

#ifdef __cplusplus
}
#endif

#endif
