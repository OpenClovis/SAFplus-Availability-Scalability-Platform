#ifndef _CL_DEBUG_CLI_FUNC_TABLE_H_
#define CL_DEBUG_CLI_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included  from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(debugCliFuncList)[] =
{
    VSYM_EMPTY(NULL, 0),
    VSYM(clDebugInvoke, CL_DEBUG_INVOKE_FUNCTION_FN_ID),
    VSYM(clDebugGetContext, CL_DEBUG_GET_DEBUGINFO_FN_ID),
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, DEBUGCli)[] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_DEBUG_CLIENT_TABLE_ID, debugCliFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
