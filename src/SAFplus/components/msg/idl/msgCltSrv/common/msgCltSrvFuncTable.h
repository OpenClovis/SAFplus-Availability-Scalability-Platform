#ifndef _MSG_CLT_SRV_FUNC_TABLE_H_
#define _MSG_CLT_SRV_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gMsgCltSrvFuncList)[] =
{
	VSYM_VER(clMsgQueueStatusGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_MSG_CLIENT_SERVER_TABLE_ID, 0)),
	VSYM_VER(clMsgQueueUnlinkServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_MSG_CLIENT_SERVER_TABLE_ID, 1)),
	VSYM_VER(clMsgQueueInfoGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_MSG_CLIENT_SERVER_TABLE_ID, 2)),
	VSYM_VER(clMsgQueueMoveMessagesServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_MSG_CLIENT_SERVER_TABLE_ID, 3)),
	VSYM_VER(clMsgMessageReceivedServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_MSG_CLIENT_SERVER_TABLE_ID, 4)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, MsgCltSrv) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_MSG_CLIENT_SERVER_TABLE_ID, gMsgCltSrvFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_MSG_CLT_SRV_FUNC_TABLE_H_*/
