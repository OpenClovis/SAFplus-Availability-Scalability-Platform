#ifndef _CL_AMS_ENTITY_TRIGGER_API_H_
#define _CL_AMS_ENTITY_TRIGGER_API_H_

#include <clCommon.h>
#include <clMetricApi.h>
#include <clAmsEntities.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT clAmsTriggerLoad(ClAmsEntityT *pEntity, ClMetricT *pMetric);
extern ClRcT clAmsTriggerLoadAll(ClMetricT *pMetric);
extern ClRcT clAmsTrigger(ClAmsEntityT *pEntity, ClMetricIdT metric);
extern ClRcT clAmsTriggerAll(ClMetricIdT metric);
extern ClRcT clAmsTriggerRecoveryReset(ClAmsEntityT *pEntity, ClMetricIdT metric);
extern ClRcT clAmsTriggerRecoveryResetAll(ClMetricIdT metric);
extern ClRcT clAmsTriggerReset(ClAmsEntityT *pEntity, ClMetricIdT metric);
extern ClRcT clAmsTriggerResetAll(ClMetricIdT metric);
extern ClRcT clAmsTriggerGetMetric(ClAmsEntityT *pEntity, ClMetricIdT id, ClMetricT *pMetric);
extern ClRcT clAmsTriggerGetMetricDefault(ClAmsEntityT *pEntity, ClMetricIdT id, ClMetricT *pMetric);

#ifdef __cplusplus
}
#endif

#endif
