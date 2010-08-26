#ifndef _CL_TXN_AGENT_FUNC_TABLE_H_
#define _CL_TXN_AGENT_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define TXN_AGT_FUNC_ID(n) CL_EO_GET_FULL_FN_NUM(CL_TXN_CLIENT_TABLE_ID, n)    
CL_EO_CALLBACK_TABLE_DECL(clTxnAgentFuncList)[] = 
{
    VSYM_EMPTY(NULL, TXN_AGT_FUNC_ID(0)),                                       /* 0 */
    VSYM_EMPTY(NULL, TXN_AGT_FUNC_ID(1)),                                       /* 1 */
    VSYM(clTxnAgentCntrlMsgReceive, CL_TXN_AGENT_MGR_CMD_RECV),                 /* 2 */ 
    VSYM(clTxnAgentDebugMsgReceive, CL_TXN_AGENT_MGR_DBG_MSG_RECV),             /* 3 */ 
    VSYM_NULL
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, TXNAgent)[] = 
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_TXN_CLIENT_TABLE_ID, clTxnAgentFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
