#ifndef _CL_CM_SERVER_FUNC_TABLE_H_
#define _CL_CM_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(cmNativeFuncList)[] =
{
    VSYM_EMPTY(NULL, 0),
    VSYM(clCmServerApiFruStateGet, CL_CHASSIS_MANAGER_FRU_STATE_GET),
    VSYM(clCmServerOperationRequest, CL_CHASSIS_MANAGER_OPERATION_REQUEST),
    VSYM(clCmServerCpmResponseHandle, CL_CHASSIS_MANAGER_CPM_RESPONSE_HANDLE),
    VSYM(clCmServerApiThresholdStateGet, CL_CHASSIS_MANAGER_THRESHOLD_STATE_GET),
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, CM)[] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, cmNativeFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
