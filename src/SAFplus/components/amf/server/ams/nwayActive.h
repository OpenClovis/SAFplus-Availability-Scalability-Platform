#ifndef _NWAY_ACTIVE_H_
#define _NWAY_ACTIVE_H_

#include <clAmsTypes.h>
#include <clAmsErrors.h>
#include <clAmsEntities.h>
#include <clAmsServerUtils.h>
#include <clAmsPolicyEngine.h>
#include <clAmsModify.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT
clAmsPeSGAssignSUNwayActive(
                            CL_IN ClAmsSGT *sg
                            );

#ifdef __cplusplus
}
#endif

#endif
