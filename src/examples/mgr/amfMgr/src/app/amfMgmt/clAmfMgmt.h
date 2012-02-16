#ifndef _CL_AMF_MGMT_H_
#define _CL_AMF_MGMT_H_

#include <clCorMetaStruct.h>
#include "clAmsMgmtOI.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ClRcT clAmfMgmtCacheInitialize(ClAmsMgmtHandleT *pHandle);
extern ClAmsMgmtHandleT gClAmsMgmtHandle;

#ifdef __cplusplus
}
#endif

#endif
