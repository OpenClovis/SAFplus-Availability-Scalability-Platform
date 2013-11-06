#ifndef _ALARM_CLOCK_EO_FUNC_TABLE_H_
#define _ALARM_CLOCK_EO_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <clEoApi.h>


CL_EO_CALLBACK_TABLE_DECL(gAlarm_clock_EOFuncList)[] =
{
	VSYM_VER(GetTimeServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 0)),
	VSYM_VER(SetTimeServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 1)),
	VSYM_VER(SetAlarmServer, 4, 0, 0, CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID, 2)),

};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, Alarm_clock_EO) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, gAlarm_clock_EOFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif
#endif /*_ALARM_CLOCK_EO_FUNC_TABLE_H_*/
