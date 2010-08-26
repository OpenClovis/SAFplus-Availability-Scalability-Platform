#ifndef __CL_NAME_SERVER_FUNC_LIST_H__
#define __CL_NAME_SERVER_FUNC_LIST_H__


#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(nameNativeFuncList)[] = 
{
    VSYM_EMPTY(NULL, NAME_FUNC_ID(0)),                              /* 0 */
    VSYM(nameSvcRegister, CL_NS_REGISTER),                          /* 1 */
    VSYM(nameSvcComponentDeregister,CL_NS_COMPONENT_DEREGISTER),    /* 2 */
    VSYM(nameSvcServiceDeregister, CL_NS_SERVICE_DEREGISTER),       /* 3 */
    VSYM(nameSvcContextCreate, CL_NS_CONTEXT_CREATE),               /* 4 */
    VSYM(nameSvcContextDelete, CL_NS_CONTEXT_DELETE),               /* 5 */
    VSYM(nameSvcQuery, CL_NS_QUERY),                                /* 6 */
    VSYM(nameSvcNack, CL_NS_NACK),                                  /* 7 */
    VSYM(nameSvcDBEntriesPack, CL_NS_DB_ENTRIES_PACK),              /* 8 */
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, NAM)[] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, nameNativeFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif


#endif
