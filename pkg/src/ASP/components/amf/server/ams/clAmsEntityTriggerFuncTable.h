#ifndef _CL_AMS_ENTITY_TRIGGER_FUNC_TABLE_H_
#define _CL_AMS_ENTITY_TRIGGER_FUNC_TABLE_H_

#if !defined (__SERVER__) && !defined (__CLIENT__)
#error "This file should be included  from server or client. Define __SERVER__ or __CLIENT__ if you are server or client and then recompile"
#endif

#ifdef __cplusplus
extern "C" {
#endif

CL_EO_CALLBACK_TABLE_DECL(amsEntityTriggerFuncList)[] =
{
    VSYM_EMPTY(NULL, 0),
    VSYM(clAmsEntityTriggerLoadRmd, CL_AMS_TRIGGER_LOAD_RMD),
    VSYM(clAmsEntityTriggerLoadAllRmd, CL_AMS_TRIGGER_LOAD_ALL_RMD),
    VSYM(clAmsEntityTriggerRmd, CL_AMS_TRIGGER_RMD),
    VSYM(clAmsEntityTriggerAllRmd, CL_AMS_TRIGGER_ALL_RMD),
    VSYM(clAmsEntityTriggerRecoveryResetRmd, CL_AMS_TRIGGER_RECOVERY_RESET_RMD),
    VSYM(clAmsEntityTriggerRecoveryResetAllRmd, CL_AMS_TRIGGER_RECOVERY_RESET_ALL_RMD),
    VSYM(clAmsEntityTriggerResetRmd, CL_AMS_TRIGGER_RESET_RMD),
    VSYM(clAmsEntityTriggerResetAllRmd, CL_AMS_TRIGGER_RESET_ALL_RMD),
    VSYM(clAmsEntityTriggerGetMetricRmd, CL_AMS_TRIGGER_GET_METRIC_RMD),
    VSYM(clAmsEntityTriggerGetMetricDefaultRmd, CL_AMS_TRIGGER_GET_METRIC_DEFAULT_RMD),
    VSYM_NULL,
};

CL_EO_CALLBACK_TABLE_LIST_DECL(gAspFuncTable, AMFTrigger)[] =
{
    CL_EO_CALLBACK_TABLE_LIST_DEF(CL_AMS_TRIGGER_CLNT_ID, amsEntityTriggerFuncList),
    CL_EO_CALLBACK_TABLE_LIST_DEF_NULL,
};

#ifdef __cplusplus
}
#endif

#endif
