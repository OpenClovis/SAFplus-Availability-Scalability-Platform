#ifndef _CKPT_CLNT_EO_FUNC_TABLE_H_
#define _CKPT_CLNT_EO_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gCkptClntEoFuncList)[] =
{
	VSYM_VER(clCkptSectionUpdationNotificationServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_CKPT_CLIENT_TABLE_ID, 0)),
	VSYM_VER(clCkptWriteUpdationNotificationServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_CKPT_CLIENT_TABLE_ID, 1)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, CkptClntEo) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_CKPT_CLIENT_TABLE_ID, gCkptClntEoFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_CKPT_CLNT_EO_FUNC_TABLE_H_*/
