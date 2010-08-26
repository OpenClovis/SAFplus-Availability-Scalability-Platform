#ifndef _CL_COR_AMF_H_
#define _CL_COR_AMF_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clAmsEntities.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCorModule.h>

#ifdef __cplusplus
extern "C" {
#endif


extern ClRcT corAmfTreeInitialize(void);
extern ClRcT corAmfMibTreeInitialize(void);
extern ClRcT corAmfEntityInitialize(void);
extern ClRcT corAmfEntityAttributeListGet(ClUint32T *pEntityId,
                                          ClAmsEntityT *pEntity,
                                          ClAmsEntityConfigT *pEntityConfig,
                                          ClCorAttributeValueListT *pAttrValueList);
extern ClRcT corAmfExtendedClassAttributeListGet(const ClCharT *pExtendedClassName,
                                                 ClCorAttributeValueListT *pAttrValueList,
                                                 ClCorClassTypeT *pClassType);

extern ClRcT corAmfEntityClassGet(ClAmsEntityTypeT type, ClCorClassTypeT *pClassId);
extern ClRcT corAmfMoClassCreate(const ClCharT *entity,
                                 ClCorMOClassPathPtrT moClassPath,
                                 ClCorClassTypeT *pMsoClassId);
extern ClRcT corAmfObjectCreate(ClUint32T entityId, ClAmsEntityT *entity, ClCorMOIdT *pMoId);
extern ClRcT clCorAmfMoIdGet(const ClCharT *name,
                             ClAmsEntityTypeT type,
                             ClCorMOIdT *pMoId);

/*
 * Module prep. handlers.
 */
extern ClRcT clCorAmfModuleHandler(ClCorModuleInfoT *info);

extern ClBoolT gClAmfMibLoaded;

#ifdef __cplusplus
}
#endif

#endif
