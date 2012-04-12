#ifndef _MPLUSN_H_
#define _MPLUSN_H_

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
clAmsPeSGFindSUForActiveAssignmentMPlusN(CL_IN ClAmsSGT *sg,
                                         CL_IN ClAmsSUT **su,
                                         CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGFindSUForStandbyAssignmentMPlusN(
                                          CL_IN ClAmsSGT *sg,
                                          CL_IN ClAmsSUT **su,
                                          CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGAssignSUMPlusN(
                        CL_IN ClAmsSGT *sg
                        );

extern ClRcT
clAmsPeSURemoveStandbyMPlusN(ClAmsSGT *sg, ClAmsSUT *su, ClUint32T switchoverMode, ClUint32T error, 
                             ClAmsSUT **activeSU, ClBoolT *reassignWork);

extern ClRcT clAmsPeSUSIDependentsListMPlusN(ClAmsSUT *su, 
                                             ClAmsSUT **activeSU,
                                             ClListHeadT *dependentSIList);

extern ClRcT clAmsPeSGAutoAdjustMPlusN(ClAmsSGT *sg);

extern ClRcT clAmsPeSISwapMPlusN(ClAmsSIT *si, ClAmsSGT *sg);

extern ClRcT
clAmsPeEntityOpSwapRemove(ClAmsEntityT *entity,
                          void *data,
                          ClUint32T dataSize,
                          ClBoolT recovery);

extern ClRcT
clAmsPeEntityOpSwapActive(ClAmsEntityT *entity,
                          void *data,
                          ClUint32T dataSize,
                          ClBoolT recovery);

extern ClRcT
clAmsPeEntityOpReduceRemove(ClAmsEntityT *entity, void *data, ClUint32T dataSize, ClBoolT recovery);

extern ClRcT
clAmsPeEntityOpActiveRemove(ClAmsEntityT *entity,
                            void *data,
                            ClUint32T dataSize,
                            ClBoolT recovery);

extern ClRcT 
clAmsPeEntityOpRemove(ClAmsEntityT *entity,
                      void *data,
                      ClUint32T dataSize,
                      ClBoolT recovery);

#ifdef __cplusplus
}
#endif

#endif
