#ifndef _CL_COR_MODULE_H_
#define _CL_COR_MODULE_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clHeapApi.h>
#include <clCorErrors.h>
#include <clCorMetaData.h>
#include <clList.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClCorModuleInfo
{
    ClCorClassTypeT classId;
    ClCorClassTypeT superClassId;
    ClCharT className[CL_MAX_NAME_LENGTH];
    ClCharT superClassName[CL_MAX_NAME_LENGTH];
    ClListHeadT simpleAttrList;
    ClListHeadT arrayAttrList;
    ClListHeadT contAttrList;
    ClListHeadT assocAttrList;
}ClCorModuleInfoT;

typedef ClRcT (*ClCorModuleFunctionT)(ClCorModuleInfoT *info);
typedef ClRcT (*ClCorModuleInitFunctionT)(void);
extern ClRcT clCorModuleClassHandler(ClCorModuleInfoT *info, ClInt32T *pModuleId);
extern ClRcT clCorModuleClassCount(ClUint32T *pCount);
extern ClRcT clCorModuleClassRun(ClBoolT *pPrevMap, ClBoolT *pCurrentMap, ClUint32T maxModules);
extern void  clCorModuleClassAttrFree(ClCorModuleInfoT *info);
extern void  clCorModuleClassAttrListFree(ClListHeadT *attrList);


#ifdef __cplusplus
}
#endif

#endif

