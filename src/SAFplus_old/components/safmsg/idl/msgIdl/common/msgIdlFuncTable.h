#ifndef _MSG_IDL_FUNC_TABLE_H_
#define _MSG_IDL_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gMsgIdlFuncList)[] =
{
	VSYM_VER(clMsgInitServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0)),
	VSYM_VER(clMsgFinServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)),
	VSYM_VER(clMsgMessageGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)),
	VSYM_VER(clMsgQueueOpenServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3)),
	VSYM_VER(clMsgQueueRetentionCloseServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4)),
	VSYM_VER(clMsgQueuePersistRedundancyServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 5)),
	VSYM_VER(clMsgQueueGroupCreateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 6)),
	VSYM_VER(clMsgQueueGroupDeleteServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 7)),
	VSYM_VER(clMsgQueueGroupInsertServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 8)),
	VSYM_VER(clMsgQueueGroupRemoveServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 9)),
	VSYM_VER(clMsgQueueGroupTrackServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 10)),
	VSYM_VER(clMsgQueueGroupTrackStopServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 11)),
	VSYM_VER(clMsgQDatabaseUpdateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 12)),
	VSYM_VER(clMsgGroupDatabaseUpdateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 13)),
	VSYM_VER(clMsgGroupMembershipUpdateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 14)),
	VSYM_VER(clMsgQueueAllocateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 15)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, MsgIdl) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, gMsgIdlFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_MSG_IDL_FUNC_TABLE_H_*/
