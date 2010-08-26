#ifndef _CL_AMS_ENTITY_TRIGGER_H_
#define _CL_AMS_ENTITY_TRIGGER_H_

#include <clCommon.h>
#include <clMetricApi.h>
#include <clOsalApi.h>
#include <clAms.h>
#include <clAmsServerUtils.h>
#include <clAmsEntities.h>
#include <clAmsServerEntities.h>

#ifdef __cplusplus

extern "C" {

#endif


typedef enum ClAmsThresholdRecovery
{
    CL_AMS_ENTITY_RECOVERY_NONE,
    CL_AMS_ENTITY_RECOVERY_RESTART,
    CL_AMS_ENTITY_RECOVERY_LOCK,
    CL_AMS_ENTITY_RECOVERY_FAILOVER,
    CL_AMS_ENTITY_RECOVERY_FAILFAST,
    CL_AMS_ENTITY_RECOVERY_MAX,
}ClAmsThresholdRecoveryT;

typedef struct ClAmsThreshold
{
    ClMetricT metric;
    ClAmsThresholdRecoveryT recovery;
    ClBoolT recoveryReset;
}ClAmsThresholdT;

extern ClRcT clAmsEntityTriggerLoad(ClAmsEntityT *pEntity,
                                    ClMetricT *pMetric);

extern ClRcT clAmsEntityTriggerLoadDefault(ClAmsEntityT *pEntity,
                                           ClMetricIdT id);

extern ClRcT clAmsEntityTriggerLoadTrigger(ClAmsEntityT *pEntity,
                                           ClMetricIdT id);

extern ClRcT clAmsEntityTriggerLoadAll(ClMetricT *pMetric);

extern ClRcT clAmsEntityTriggerLoadTriggerAll(ClMetricIdT id);

extern ClRcT clAmsEntityTriggerRecoveryReset(ClAmsEntityT *pEntity,
                                             ClMetricIdT id);

extern ClRcT clAmsEntityTriggerRecoveryResetAll(ClMetricIdT id);

extern ClRcT clAmsEntityTriggerLoadDefaultAll(ClMetricIdT id);

extern ClRcT clAmsEntityTriggerGetMetric(ClAmsEntityT *pEntity,
                                         ClMetricIdT metric,
                                         ClMetricT *pMetric);

extern ClRcT clAmsEntityTriggerGetMetricDefault(ClAmsEntityT *pEntity,
                                                ClMetricIdT metric,
                                                ClMetricT *pMetric);
extern ClRcT clAmsEntityTriggerInitialize(void);

extern  ClRcT clAmsEntityTriggerFinalize(void);

#ifdef __cplusplus
}
#endif

#endif
