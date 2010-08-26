#ifndef _CL_ERROR_API_H_
#define _CL_ERROR_API_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <saAis.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SaAisErrorT clErrorToSaf(ClRcT errorCode);
extern void clErrorCodeToString(ClRcT errorCode, ClCharT *buf, ClUint32T maxBufSize);
extern ClCharT *clErrorToString(ClRcT errorCode);

#ifdef __cplusplus
}
#endif

#endif
