#ifndef _APP_FUNC_TABLE_H_
#define _APP_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gAppFuncList)[] =
{
	VSYM_VER(clLogClientFilterSetNotifyServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_LOG_CLIENT_TABLE_ID, 0)),
	VSYM_VER(clLogClntFileHdlrDataReceiveServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_LOG_CLIENT_TABLE_ID, 1)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, App) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_LOG_CLIENT_TABLE_ID, gAppFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_APP_FUNC_TABLE_H_*/
