#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <clAmsTypes.h>
#include <clAmsErrors.h>
#include <clAmsEntities.h>
#include <clAmsServerUtils.h>
#include <clAmsPolicyEngine.h>
#include <clAmsEntityUserData.h>
#include <clAmsXdrHeaderFiles.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT
clAmsPeSIIsActiveAssignableCustom(CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGFindSIForActiveAssignmentCustom(
        CL_IN ClAmsSGT *sg,
        CL_INOUT ClAmsSIT **targetSI);

extern ClRcT
clAmsPeSGFindSUForActiveAssignmentCustom(CL_IN ClAmsSGT *sg,
                                         CL_IN ClAmsSUT **su,
                                         CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGFindSUForStandbyAssignmentCustom(
                                          CL_IN ClAmsSGT *sg,
                                          CL_IN ClAmsSUT **su,
                                          CL_IN ClAmsSIT *si);

extern ClRcT
clAmsPeSGAssignSUCustom(
                        CL_IN ClAmsSGT *sg
                        );

extern ClRcT 
clAmsPeSIAssignSUCustom(ClAmsSIT *si, ClAmsSUT *activeSU, ClAmsSUT *standbySU);

#ifdef __cplusplus
}
#endif

#endif
