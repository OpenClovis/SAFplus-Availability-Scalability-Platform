#include <clCorModule.h>
#include <clCorAmf.h>
#include <clCorClient.h>

typedef struct ClCorModuleRegister
{
    const ClCharT *id;
    ClCorModuleFunctionT funPtr;
    ClCorModuleInitFunctionT initFunPtr;
}ClCorModuleRegisterT;

static ClCorModuleRegisterT gClCorModules[] = {
    {
        .id = "AMF",
        .funPtr = clCorAmfModuleHandler,
        .initFunPtr = corAmfMibTreeInitialize,
    },
    {
        .id = NULL,
        .funPtr = NULL,
    },
};

ClRcT clCorModuleClassCount(ClUint32T *pCount)
{
    if(!pCount)
        return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);
    *pCount = (ClUint32T)sizeof(gClCorModules)/sizeof(gClCorModules[0]);
    return CL_OK;
}

ClRcT clCorModuleClassHandler(ClCorModuleInfoT *info, ClInt32T *pModuleId)
{
    ClInt32T i;
    if(pModuleId)
        *pModuleId = -1;
    if(!info) return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
    for(i = 0; gClCorModules[i].id; ++i)
    {
        if(gClCorModules[i].funPtr)
        {
            if(gClCorModules[i].funPtr(info) != CL_OK)
                continue;
            if(pModuleId)
                *pModuleId = i;
            return CL_OK;
        }
    }
    return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
}

ClRcT clCorModuleClassRun(ClBoolT *pPrevMap, ClBoolT *pCurrentMap, ClUint32T maxModules)
{
    ClRcT rc = CL_OK;
    ClInt32T i;
    if(!pCurrentMap || !maxModules) 
        return rc;
    for(i = 0; i < maxModules; ++i)
    {
        if(pCurrentMap[i] && gClCorModules[i].initFunPtr)
        {
            if(pPrevMap)
                pCurrentMap[i] ^= pPrevMap[i];
            if(pCurrentMap[i])
            {
                rc = gClCorModules[i].initFunPtr();
                if(rc != CL_OK)
                    return rc;
            }
                
        }
    }
    return rc;
}

void clCorModuleClassAttrListFree(ClListHeadT *list)
{
    while(!CL_LIST_HEAD_EMPTY(list))
    {
        ClListHeadT *head = list->pNext;
        corAttrIntf_t *attr = CL_LIST_ENTRY(head, corAttrIntf_t, list);
        clListDel(head);
        clHeapFree(attr);
    }
}

void clCorModuleClassAttrFree(ClCorModuleInfoT *info)
{
    clCorModuleClassAttrListFree(&info->simpleAttrList);
    clCorModuleClassAttrListFree(&info->arrayAttrList);
    clCorModuleClassAttrListFree(&info->contAttrList);
    clCorModuleClassAttrListFree(&info->assocAttrList);
}
