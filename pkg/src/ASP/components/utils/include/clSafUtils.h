#ifndef _CL_SAF_UTILS_H
#define _CL_SAF_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <clCommonErrors.h>
#include <clLogApi.h>

#include <saAis.h>

SaAisErrorT clClovisToSafError(ClRcT clError);
    
ClRcT clSafToClovisError(SaAisErrorT safError);
    
#ifdef __cplusplus
}
#endif

#endif /* _CL_SAF_UTILS_H */
