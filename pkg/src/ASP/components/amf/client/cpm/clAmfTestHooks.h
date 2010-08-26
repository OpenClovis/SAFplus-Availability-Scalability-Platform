#ifndef _CL_AMF_TEST_HOOKS_H_
#define _CL_AMF_TEST_HOOKS_H_

#include <saAmf.h>

#ifdef __cplusplus
extern "C" {
#endif

extern SaAisErrorT saAmfInitializeTestHook(SaAmfHandleT *amfHandle,
                                           const SaAmfCallbacksT *amfCallbacks,
                                           SaVersionT *version);

#define CL_AMF_TEST_HOOK_INITIALIZE(amfHandle, amfCallbacks, version) do { \
    if(getenv("CL_AMF_TEST_HOOKS"))                                     \
    {                                                                   \
        if(saAmfInitializeTestHook(amfHandle, amfCallbacks, version) == SA_AIS_OK) \
            return SA_AIS_OK;                                           \
    }                                                                   \
}while(0)

#ifdef __cplusplus
}
#endif

#endif

