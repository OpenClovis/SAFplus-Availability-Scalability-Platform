#ifndef __CL_OSAL_COMMON_H__
#define __CL_OSAL_COMMON_H__


#ifndef SEMVMX
#define SEMVMX (32767)
#endif

#define STACK_DEPTH 16
#define LOG_BUFFER_SIZE 256

#ifndef CL_OSAL_DEBUG

#define nullChkRet(__ptr) clDbgIfNullReturn(__ptr,CL_CID_OSAL)

#define sysErrnoChkRet(__ret) do { if (-1 == (__ret)) \
    { \
        int __err = errno;\
        int __retCode; \
        if (__err == EINVAL) __retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER); \
        else __retCode = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR); \
        clDbgRootCauseError(__retCode, ("When running function [%s], system returned error [%s] code [%d].  System call or error variable is [" #__ret "].", __FUNCTION__, strerror(__err), __err)); \
        CL_FUNC_EXIT(); \
        return(__retCode); \
    } } while(0)

#define sysRetErrChkRet(__predicate) do {  \
        int __result = (int) (__predicate);  \
        if (0 != __result) { \
        int __retCode;       \
        if (__result == EINVAL) __retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER); \
        else __retCode = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR); \
        clDbgRootCauseError(__retCode, ("When running function [%s], system returned error [%s] code [%d].  System call or error variable is [" #__predicate "].", __FUNCTION__, strerror(__result), __result)); \
        CL_FUNC_EXIT(); \
        return(__retCode); \
                           } \
  } while(0)

#else

#define nullChkRet(__ptr) do { if(!__ptr) { return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER); } }while(0)

#define sysErrnoChkRet(__ret) do { if (-1 == (__ret))                   \
    {                                                                   \
        int __err = errno;                                              \
        int __retCode;                                                  \
        if (__err == EINVAL) __retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER); \
        else __retCode = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);              \
        printf("When running function [%s], system returned error [%s] code [%d].  System call or error variable is [" #__ret "].\n", __FUNCTION__, strerror(__ret), __err); \
        CL_FUNC_EXIT();                                                 \
        return(__retCode);                                              \
    } } while(0)

#define sysRetErrChkRet(__predicate) do {                               \
        int __result = (int) (__predicate);                             \
        if (0 != __result)                                              \
        {                                                               \
        int __retCode;                                                  \
        if (__result == EINVAL) __retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER); \
        else __retCode = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);              \
        printf("When running function [%s], system returned error [%s] code [%d].  System call or error variable is [" #__predicate "].\n", __FUNCTION__, strerror(__result), __result); \
        CL_FUNC_EXIT();                                                 \
        return(__retCode);                                              \
        }                                                               \
  } while(0)

#endif

#endif
