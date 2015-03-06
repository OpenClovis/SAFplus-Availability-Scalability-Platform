#ifndef _CL_TXN_SERVER_FUNC_TABLE_H_
#define _CL_TXN_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TXN_FUNC_ID(n) CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, n)    
CL_EO_CALLBACK_TABLE_DECL(clTxnServiceFuncList)[] = 
{
    VSYM_EMPTY(NULL, TXN_FUNC_ID(0)),                                       /* 0 */
    VSYM(clTxnServiceClientReqRecv, CL_TXN_SERVICE_CLIENT_REQ_RECV),        /* 1 */ 
    VSYM_NULL
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, TXN)[] = 
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, clTxnServiceFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
