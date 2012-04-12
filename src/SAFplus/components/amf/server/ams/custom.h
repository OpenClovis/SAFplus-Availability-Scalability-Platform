#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <clAmsTypes.h>
#include <clAmsErrors.h>
#include <clAmsEntities.h>
#include <clAmsServerUtils.h>
#include <clAmsPolicyEngine.h>
#include <clAmsModify.h>
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
        CL_INOUT ClAmsSIT **targetSI,
        CL_OUT ClAmsSUT **targetSU);

extern ClRcT
clAmsPeSGFindSIForStandbyAssignmentCustom(
                                          CL_IN ClAmsSGT *sg,
                                          CL_OUT ClAmsSIT **targetSI,
                                          CL_OUT ClAmsSUT **targetSU,
                                          CL_IN ClAmsSIT **scannedSIList,
                                          CL_IN ClUint32T numScannedSIs);

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

extern ClRcT clAmsPeSGAutoAdjustCustom(ClAmsSGT *sg);

#ifdef __cplusplus
}
#endif

#endif
