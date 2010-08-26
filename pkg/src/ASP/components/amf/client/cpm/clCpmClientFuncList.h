#ifndef _CL_CPM_CLIENT_FUNC_LIST_H_

#define _CL_CPM_CLIENT_FUNC_LIST_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(clCpmMgmtClientFuncList)[] = 
{
    VSYM_EMPTY(NULL, CPM_INITIALIZE_FN_ID),
    VSYM(_clCpmClientCompHealthCheck, CPM_HEALTH_CHECK_FN_ID),            
    VSYM(_clCpmClientCompTerminate, CPM_TERMINATE_FN_ID),                 
    VSYM(_clCpmClientCompCSISet, CPM_CSI_SET_FN_ID),
    VSYM(_clCpmClientCompCSIRmv, CPM_CSI_RMV_FN_ID),
    VSYM(_clCpmClientCompProtectionGroupTrack, CPM_PROTECTION_GROUP_TRACK_FN_ID),
    VSYM(_clCpmClientCompProxiedComponentInstantiate, CPM_PROXIED_COMP_INSTANTIATION_FN_ID),
    VSYM(_clCpmClientCompProxiedComponentCleanup, CPM_PROXIED_CL_CPM_CLEANUP_FN_ID),
    VSYM_NULL
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, CPMMgmtClient)[] = 
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_CPM_MGR_CLIENT_TABLE_ID, clCpmMgmtClientFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
