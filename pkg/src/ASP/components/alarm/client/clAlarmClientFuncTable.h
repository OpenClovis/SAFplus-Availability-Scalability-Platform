#ifndef _CL_ALARM_CLIENT_FUNC_TABLE_H_
#define _CL_ALARM_CLIENT_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(gAlarmClientFuncList)[]=
{
    VSYM_EMPTY(NULL, ALARM_CLIENT_FUNC_ID(0)),
    VSYM(clAlarmSvcLibDebugCliAlarmProcess, CL_ALARM_COMP_ALARM_RAISE_IPI),
    VSYM(clAlarmStateGet, CL_ALARM_STATUS_GET_IPI),
    VSYM(clAlarmPendingAlarmsGet, CL_ALARM_PENDING_ALARMS_GET_IPI),
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, ALM_CLIENT) [] =
{
        CL_EO_CALLBACK_TABLE_LIST_DEF(CL_ALARM_CLIENT_TABLE_ID, gAlarmClientFuncList),
        CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
