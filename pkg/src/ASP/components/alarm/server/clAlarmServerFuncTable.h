#ifndef _CL_ALARM_SERVER_FUNC_TABLE_H_
#define _CL_ALARM_SERVER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(gAlarmServerFuncList)[] =
{
    VSYM_EMPTY(NULL, ALARM_SERVER_FUNC_ID(0)), /* 0 */

    VSYM(clAlarmAddProcess, CL_ALARM_SERVER_API_HANDLER), /* 1 */
    VSYM(clAlarmHandleToInfoGet, CL_ALARM_HANDLE_TO_INFO_GET), /* 2 */
    VSYM(clAlarmInfoToAlarmHandleGet, CL_ALARM_HANDLE_FROM_INFO_GET), /* 3 */
    VSYM_VER(clAlarmServerAlarmReset, 4, 1, 0, CL_ALARM_RESET),

    VSYM_NULL, 
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, ALM) [] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_EO_NATIVE_COMPONENT_TABLE_ID, gAlarmServerFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};


#ifdef __cplusplus
}
#endif

#endif
