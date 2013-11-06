#ifndef _LOG_FUNC_TABLE_H_
#define _LOG_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gLogFuncList)[] =
{
	VSYM_VER(clLogMasterAttrVerifyNGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0)),
	VSYM_VER(clLogMasterStreamCloseNotifyServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)),
	VSYM_VER(clLogMasterStreamListGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)),
	VSYM_VER(clLogMasterCompIdChkNGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 3)),
	VSYM_VER(clLogMasterCompListGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 4)),
	VSYM_VER(clLogMasterCompListNotifyServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 5)),
	VSYM_VER(clLogStreamOwnerStreamOpenServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 6)),
	VSYM_VER(clLogStreamOwnerStreamCloseServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 7)),
	VSYM_VER(clLogStreamOwnerFilterSetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 8)),
	VSYM_VER(clLogStreamOwnerHandlerRegisterServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 9)),
	VSYM_VER(clLogStreamOwnerStreamMcastGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 10)),
	VSYM_VER(clLogStreamOwnerHandlerDeregisterServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 11)),
	VSYM_VER(clLogStreamOwnerFilterGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 12)),
	VSYM_VER(clLogSvrStreamOpenServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 13)),
	VSYM_VER(clLogSvrStreamCloseServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 14)),
	VSYM_VER(clLogSvrFilterSetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 15)),
	VSYM_VER(clLogSvrStreamHandleFlagsUpdateServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 16)),
	VSYM_VER(clLogHandlerSvrAckSendServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 17)),
	VSYM_VER(clLogFileHdlrFileOpenServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 18)),
	VSYM_VER(clLogFileHdlrFileRecordsGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 19)),
	VSYM_VER(clLogFileHdlrFileMetaDataGetServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 20)),
	VSYM_VER(clLogExternalSendServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 21)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, Log) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, gLogFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_LOG_FUNC_TABLE_H_*/
