#pragma once

#include <clArchHeaders.h>
#include <semaphore.h>

typedef pthread_cond_t      ClOsalCondT;
typedef uint32_t ClUint32T;
typedef int32_t ClInt32T;
typedef void* ClPtrT;

/**
 *  The mutex type to be initialized
 */
typedef enum ClOsalSharedMutexFlags
{
    CL_OSAL_SHARED_INVALID   = 0x0,
    CL_OSAL_SHARED_NORMAL    = 0x1,
    CL_OSAL_SHARED_SYSV_SEM  = 0x2,
    CL_OSAL_SHARED_POSIX_SEM = 0x4,
    CL_OSAL_SHARED_RECURSIVE = 0x8,
    CL_OSAL_SHARED_PROCESS   = 0x10,
    CL_OSAL_SHARED_ERROR_CHECK = 0x20,
}ClOsalSharedMutexFlagsT;

typedef struct ClOsalMutex
{
#ifdef CL_OSAL_DEBUG
#define CL_OSAL_DEBUG_MAGIC 0xDEADBEEF
    ClUint32T magic;
    pthread_t tid;
    const ClCharT *creatorFile;
    ClInt32T creatorLine;
    const ClCharT *ownerFile;
    ClInt32T ownerLine;
    ClListHeadT list; /*entry into the mutex pool*/
#endif
    ClOsalSharedMutexFlagsT flags;
    union shared_lock
    {
        pthread_mutex_t mutex;
        struct ClSem
        {
            sem_t posSem;
            int semId;
            ClInt32T numSems;
        }sem;
    }shared_lock;
}ClOsalMutexT;

  /** Old Clovis return code type. see clCommonErrors.h  DEPRECATED  */
typedef ClUint32T       ClRcT;

#include <clCommonErrors6.h>
