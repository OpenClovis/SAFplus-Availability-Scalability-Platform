#include "clTimerTest.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TIMERS (0x100)
#define SEC_LIMIT (100)
#define MILLISEC_LIMIT (2000)
#define CL_TIMER_LOG_FILE "clTimerLog.txt"

#ifdef TIMER_DRIFT
#define CL_TIMER_DRIFT_THRESHOLD TIMER_DRIFT
#else
#define CL_TIMER_DRIFT_THRESHOLD (CL_TIMER_USEC_PER_SEC/2)
#endif

typedef struct clTimerArgs
{
    struct timeval start;
    struct timeval end;
    ClTimerTimeOutT timeOut;
    ClInt64T diff;
    ClTimerTypeT type;
    ClTimerContextT context;
    ClInt32T expired;
}ClTimerArgsT;

static ClTimerHandleT *pTimerHandles;
static ClTimerArgsT *pTimerArgs;
static ClInt32T timers = TIMERS;
static ClInt32T expired;
static ClBoolT timerTestDone = CL_FALSE;
static ClBoolT timerRepetitiveTest = CL_FALSE;
static ClInt64T totalShift;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

static __inline__ ClInt64T clTimerTimeDiff(struct timeval *pStart,
                                           struct timeval *pEnd,
                                           ClUint64T thresholdUsecs)
{
    ClInt64T diff = 0;
    pEnd->tv_sec -= pStart->tv_sec;
    pEnd->tv_usec -= pStart->tv_usec;

    /*Possible Time shift*/
    if(pEnd->tv_sec <= 0 )
    {
        if(pEnd->tv_sec < 0 || pEnd->tv_usec <= 0)
        {
            return 0ULL;
        }
    }
    if(pEnd->tv_usec < 0 )
    {
        pEnd->tv_usec += 1000000L;
        --pEnd->tv_sec;
    }
    diff = pEnd->tv_sec*1000000L + pEnd->tv_usec;
    /*
     * Assume forward time shift
     */
    if(thresholdUsecs && diff >= thresholdUsecs)
    {
        fprintf(stderr,"forward drift assumed as diff exceeds threshold:%lld\n",thresholdUsecs);
        diff = 0;
    }
    return diff;
}

static void clTimerCallback(ClPtrT pArg)
{
    ClTimerArgsT *pTimerArg = pArg;
    CL_ASSERT(pTimerArg != NULL);
    if(timerRepetitiveTest == CL_TRUE)
    {
        pTimerArg = pTimerArgs+expired;
    }
    gettimeofday(&pTimerArg->end,NULL);
    ++pTimerArg->expired;
    pTimerArg->diff = clTimerTimeDiff(&pTimerArg->start,&pTimerArg->end,0);
    if(++expired == timers)
    {
        fprintf(stdout,"Timer %d exiting.Signalling main thread.\n",timers);
        pthread_mutex_lock(&mutex);
        timerTestDone = CL_TRUE;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    else if(timerRepetitiveTest == CL_TRUE) 
    {
        gettimeofday(&pTimerArgs[expired].start,NULL);
    }
}

static void clTimerLog(ClInt32T timer,ClInt64T expected,ClTimerArgsT *pTimerArg)
{
    static FILE*fptr;
    if(!fptr && !(fptr = fopen(CL_TIMER_LOG_FILE,"w")))
    {
        CL_TIMER_PRINT("Error opening file:%s for writing\n",CL_TIMER_LOG_FILE);
        CL_ASSERT(0);
    }
    fprintf(fptr,"Timer %d took %lld usecs.Supposed to complete at %lld usecs(%d.%d sec).Deviation of %lld usecs\n",timer+1,pTimerArg->diff,expected,pTimerArg->timeOut.tsSec,pTimerArg->timeOut.tsMilliSec,pTimerArg->diff-expected);
    fflush(fptr);
}

static void clTimerDisplayResult(ClTimerArgsT *pTimerArg)
{
    ClInt64T actual;
    ClInt64T expected;
    actual = pTimerArg->diff;
    expected = pTimerArg->timeOut.tsSec*1000000LL;
    expected += pTimerArg->timeOut.tsMilliSec * 1000L;
    /*round off to frequency base*/
    /*expected += (pTimerArg->timeOut.tsMilliSec/CL_TIMER_MSEC_JIFFIES*CL_TIMER_MSEC_JIFFIES) * CL_TIMER_USEC_PER_MSEC;*/
    if(actual < expected)
    {
#ifdef DEBUG
        fprintf(stderr,"Actual timeout %lld fired %lld usecs before expected timeout %lld (usecs)\n",actual,expected-actual,expected);
#endif
    }
    CL_ASSERT(labs(actual - expected) < CL_TIMER_DRIFT_THRESHOLD);

    clTimerLog(pTimerArg-pTimerArgs,expected,pTimerArg);
    totalShift += (actual - expected);
}

static void clTimerDisplayResults(void)
{
    ClInt32T i;
    for(i = 0;  i < timers; ++i)
    {
        clTimerDisplayResult(pTimerArgs+i);
    }
}

int main(int argc,char **argv)
{
    ClRcT rc;
    register ClInt32T i;
    ClInt32T secLimit = SEC_LIMIT;
    ClInt32T milliSecLimit = MILLISEC_LIMIT;
    rc = clTimerInitialize();
    CL_ASSERT(rc== CL_OK);
    
    if(argc > 1 ) 
    {
        timers = atoi(argv[1]) & 0xfffff;
    }
    if(argc > 2 )
    {
        secLimit = atoi(argv[2]) & 0xfff;
    }
    if(argc > 3 )
    {
        milliSecLimit = atoi(argv[3]) & 0xffff;
    }
    pTimerHandles = calloc(timers,sizeof(ClTimerHandleT));
    CL_ASSERT(pTimerHandles != NULL);

    pTimerArgs = calloc(timers,sizeof(ClTimerArgsT));
    CL_ASSERT(pTimerArgs != NULL);
    for(i = 0;i < timers; ++i)
    {
        ClTimerTimeOutT timeOut = {0};
        ClTimerContextT context = random() % CL_TIMER_MAX_CONTEXT;
        if(secLimit)
        {
            timeOut.tsSec = random() % secLimit;
        }
        if(milliSecLimit)
        {
            timeOut.tsMilliSec = random() % milliSecLimit;
        }
        memcpy(&(pTimerArgs+i)->timeOut,&timeOut,sizeof(timeOut));
        pTimerArgs[i].context=context;
        pTimerArgs[i].type = CL_TIMER_ONE_SHOT;
        CL_ASSERT(clTimerCreate(timeOut,CL_TIMER_ONE_SHOT,context,clTimerCallback,(ClPtrT)&pTimerArgs[i],pTimerHandles+i)==CL_OK);
    }
    for(i = 0; i < timers;++i)
    {
        /*Mark the start time of each guy*/
        gettimeofday(&pTimerArgs[i].start,NULL);
        CL_ASSERT(clTimerStart(pTimerHandles[i])==CL_OK);
    }

    pthread_mutex_lock(&mutex);
    if(timerTestDone == CL_TRUE)
    {
        goto out_result;
    }
    pthread_cond_wait(&cond,&mutex);
    CL_ASSERT(timerTestDone == CL_TRUE);
 out_result:
    pthread_mutex_unlock(&mutex);

    clTimerDisplayResults();

    if(timers)
    {
        fprintf(stderr,"AVG. mean shift for %d timers = %lld usecs\n",timers,totalShift/timers);
    }
#ifdef TEST_REPETITIVE
    expired=0;
    timerTestDone = CL_FALSE;
    totalShift = 0;
    /*Test repetitive*/
    
    pTimerArgs[0].type = CL_TIMER_REPETITIVE;
    pTimerArgs[0].context =CL_TIMER_SEPARATE_CONTEXT;
    pTimerArgs[0].timeOut.tsSec = secLimit & 3;
    pTimerArgs[0].timeOut.tsMilliSec = milliSecLimit & 0xff;
    /*overwrite timeouts for all the remaining*/
    for(i = 1; i < timers; ++i)
    {
        pTimerArgs[i] = pTimerArgs[0];
    }
    CL_ASSERT(clTimerCreate(pTimerArgs[0].timeOut,CL_TIMER_REPETITIVE,CL_TIMER_TASK_CONTEXT,clTimerCallback,pTimerArgs+0,pTimerHandles+0)==CL_OK);

    timerRepetitiveTest = CL_TRUE;
    gettimeofday(&pTimerArgs[0].start,NULL);
    CL_ASSERT(clTimerStart(pTimerHandles[0]) == CL_OK);

    pthread_mutex_lock(&mutex);
    if(timerTestDone == CL_TRUE)
    {
        goto out_result2;
    }
    pthread_cond_wait(&cond,&mutex);
    CL_ASSERT(timerTestDone == CL_TRUE);
 out_result2:
    pthread_mutex_unlock(&mutex);
    clTimerDisplayResults();

    if(timers)
    {
        fprintf(stderr,"AVG. mean shift for %d repetitive timers = %lld usecs\n",timers,totalShift/timers);
    }
#else
    for(i = 0; i < timers; ++i)
    {
        CL_ASSERT(clTimerDelete(pTimerHandles+i) == CL_OK);
    }
#endif
    clTimerFinalize();
    return 0;
}
