#ifndef _CL_NODE_CACHE_H_
#define _CL_NODE_CACHE_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clOsalApi.h>
#include <clDebugApi.h>

#ifdef __cplusplus
extern "C" {
#endif

ClRcT clNodeCacheInitialize(ClBoolT createFlag);
ClRcT clNodeCacheFinalize(void);
ClRcT clNodeCacheUpdate(ClIocNodeAddressT nodeAddress, ClUint32T version);
ClRcT clNodeCacheReset(ClIocNodeAddressT nodeAddress);
ClRcT clNodeCacheVersionGet(ClIocNodeAddressT nodeAddress, ClUint32T *pVersion);
ClRcT clNodeCacheMinVersionGet(ClIocNodeAddressT *pNodeAddress, ClUint32T *pVersion);

#ifdef __cplusplus
}
#endif

#endif
