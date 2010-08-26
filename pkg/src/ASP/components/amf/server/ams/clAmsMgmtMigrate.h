#ifndef _CL_AMS_MGMT_MIGRATE_H_
#define _CL_AMS_MGMT_MIGRATE_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clAmsTypes.h>
#include <clAms.h>
#include <clAmsErrors.h>
#include <clAmsServerUtils.h>
#include <clAmsMgmtDebugCli.h>
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtClientApi.h>
#include <clHeapApi.h>
#include <clXdrApi.h>

#ifdef __cplusplus
extern "C" {
#endif

ClRcT clAmsMgmtMigrateSGRedundancy(ClAmsSGRedundancyModelT model, 
                                   const ClCharT *sg,
                                   const ClCharT *prefix,
                                   ClUint32T numActiveSUs,
                                   ClUint32T numStandbySUs,
                                   ClAmsMgmtMigrateListT *migrateList);


#ifdef __cplusplus
}
#endif

#endif
