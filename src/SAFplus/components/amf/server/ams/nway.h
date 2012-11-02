#ifndef _NWAY_H_
#define _NWAY_H_

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
clAmsPeSGFindSUForActiveAssignmentNway(CL_IN ClAmsSGT *sg,
                                         CL_IN ClAmsSUT **su,
                                         CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGFindSUForStandbyAssignmentNway(
                                          CL_IN ClAmsSGT *sg,
                                          CL_IN ClAmsSUT **su,
                                          CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGAssignSUNway(
                        CL_IN ClAmsSGT *sg
                        );

#ifdef __cplusplus
}
#endif

#endif
