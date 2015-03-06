#ifndef _CL_TIMERTEST_H_
#define _CL_TIMERTEST_H_

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "clTimerTestCommon.h"
#include "clTimerTestCommonErrors.h"
#include "clTimerTestList.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CL_ASSERT
#define CL_ASSERT(expr) assert(expr)
#endif

#define clHeapAllocate malloc
#define clHeapCalloc calloc
#define clHeapFree free

#define CL_TIMER_RC(rc) CL_RC(CL_CID_TIMER,rc)

#define CL_TIMER_PRINT(fmt,arg...) do {                                 \
        fprintf(stderr,"[%s:%s:%d] "fmt,__FUNCTION__,__FILE__,__LINE__,##arg); \
}while(0)

#define CL_TIMER_MSEC_PER_SEC (1000)
#define CL_TIMER_USEC_PER_SEC (1000000L)
#define CL_TIMER_NSEC_PER_SEC (1000000000LL)
#define CL_TIMER_USEC_PER_MSEC (1000)
#define CL_TIMER_NSEC_PER_USEC (1000)
#define CL_TIMER_NSEC_PER_MSEC (1000000L)

#define CL_TIMER_DELAY(timeOutUsec) do {                        \
    if((timeOutUsec) > 0 )                                      \
    {                                                           \
        struct timespec spec = {0};                             \
        spec.tv_nsec = (timeOutUsec * CL_TIMER_NSEC_PER_USEC);  \
        nanosleep(&spec,NULL);                                  \
    }                                                           \
}while(0)

typedef ClHandleT ClTimerHandleT;

typedef enum ClTimerType
{
    CL_TIMER_ONE_SHOT ,
    CL_TIMER_REPETITIVE,
    CL_TIMER_VOLATILE ,
    CL_TIMER_MAX_TYPE,
}ClTimerTypeT;

typedef enum ClTimerContext
{
    CL_TIMER_TASK_CONTEXT,
    CL_TIMER_SEPARATE_CONTEXT,
    CL_TIMER_MAX_CONTEXT,
} ClTimerContextT;

typedef struct ClTimerTimeOut
{
    ClUint32T tsSec;
    ClUint32T tsMilliSec;
}ClTimerTimeOutT;

typedef void (*ClTimerCallBackT)(ClPtrT pArg);

ClRcT clTimerInitialize(void);

ClRcT clTimerFinalize(void);

ClRcT clTimerCreate(ClTimerTimeOutT timeOut, 
                    ClTimerTypeT timerType,
                    ClTimerContextT timerContext,
                    ClTimerCallBackT timerCallback,
                    ClPtrT timerData,
                    ClTimerHandleT *pTimerHandle);

ClRcT clTimerStart(ClTimerHandleT timerHandle);

ClRcT clTimerStop(ClTimerHandleT timerHandle);

ClRcT clTimerDelete(ClTimerHandleT *pTimerHandle);

static __inline__ ClTimeT clTimerStopWatchTimeGet(void)
{
    ClTimeT usecs;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    usecs = (ClTimeT)t.tv_sec * CL_TIMER_USEC_PER_SEC + t.tv_nsec/1000;
    return usecs;
}

#ifdef __cplusplus
}
#endif

#endif
