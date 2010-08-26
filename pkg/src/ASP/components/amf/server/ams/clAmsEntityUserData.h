#ifndef _CL_AMS_ENTITY_USER_DATA_H_
#define _CL_AMS_ENTITY_USER_DATA_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clAmsErrors.h>
#include <clAmsMgmtCommon.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsServerUtils.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clBufferApi.h>
#include <clList.h>
#include <clHash.h>
#include <clCksmApi.h>
#include <clXdrApi.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT clAmsEntityUserDataInitialize(void);
extern ClRcT clAmsEntityUserDataFinalize(void);
extern ClRcT _clAmsEntityUserDataSet(ClNameT *entity, ClCharT *data, ClUint32T len);
extern ClRcT _clAmsEntityUserDataSetKey(ClNameT *entity, ClNameT *key, ClCharT *data, ClUint32T len);
extern ClRcT _clAmsEntityUserDataGet(ClNameT *entity, ClCharT **data, ClUint32T *length);
extern ClRcT _clAmsEntityUserDataGetKey(ClNameT *entity, ClNameT *key, ClCharT **data, ClUint32T *length);
extern ClRcT _clAmsEntityUserDataDelete(ClNameT *entity, ClBoolT clear);
extern ClRcT _clAmsEntityUserDataDeleteKey(ClNameT *entity, ClNameT *key);
extern ClRcT clAmsEntityUserDataDestroy(void);
extern ClRcT clAmsEntityUserDataPackAll (ClBufferHandleT inMsgHdl);
extern ClRcT clAmsEntityUserDataUnpackAll(ClBufferHandleT inMsgHdl);

#ifdef __cplusplus
}
#endif

#endif
