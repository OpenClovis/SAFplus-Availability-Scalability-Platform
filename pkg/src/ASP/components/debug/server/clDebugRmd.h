#ifndef _CL_DEBUG_RMD_H_
#define _CL_DEBUG_RMD_H_

#include <clEoApi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CL_DEBUG_INITIALIZE_COMMON_FN_ID = 0,
    /*invoke a function through a debug command*/
    CL_DEBUG_INVOKE_FUNCTION_FN_ID   = CL_EO_GET_FULL_FN_NUM(
        CL_DEBUG_CLIENT_TABLE_ID, 1),
    /*get debug function list for the EO*/
    CL_DEBUG_GET_DEBUGINFO_FN_ID     = CL_EO_GET_FULL_FN_NUM(
        CL_DEBUG_CLIENT_TABLE_ID, 2)
}ClDebugFunctionsIdT;


#ifdef __cplusplus
}
#endif

#endif
