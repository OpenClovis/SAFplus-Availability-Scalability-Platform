/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */

#include <clLeakyBucket.h>

typedef struct ClLeakyBucket
{
    ClOsalMutexT mutex;
    ClOsalCondT  cond;
    ClInt32T     waiters;  
    ClInt64T     value; /*current limit*/
    ClInt64T     volume; /*bucket size*/
    ClInt64T     leakSize; /*how much to leak on timer fire*/
    ClLeakyBucketWaterMarkT waterMark; /*bucket marks for delay*/
    ClBoolT lowWMHit;
    ClBoolT highWMHit;
    ClTimerHandleT timer;
}ClLeakyBucketT;

typedef struct ClDequeueTrafficShaper
{
    ClLeakyBucketT *bucket;
    ClListHeadT *queue;
    ClDequeueTrafficShaperCallbackT callback;
}ClDequeueTrafficShaperT;

#define CL_LEAKY_BUCKET_RC(rc) CL_RC(CL_CID_LEAKY_BUCKET, rc)

static ClRcT clLeakyBucketIntervalCallback(void *arg)
{
    ClLeakyBucketT *bucket = (ClLeakyBucketT *) arg;
    clOsalMutexLock(&bucket->mutex);
    bucket->value -= bucket->leakSize;
    if(bucket->value < 0) bucket->value = 0;
    if(bucket->waiters > 0)
        clOsalCondBroadcast(&bucket->cond);
    bucket->lowWMHit = CL_FALSE;
    bucket->highWMHit = CL_FALSE;
    clOsalMutexUnlock(&bucket->mutex);
    return CL_OK;
}

static ClRcT clLeakyBucketCreateExtended(ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval,ClLeakyBucketWaterMarkT *waterMark,ClLeakyBucketHandleT *handle)
{
    ClLeakyBucketT *bucket = NULL;
    ClRcT rc = CL_OK;

    if(!handle || !volume || !leakSize || (!leakInterval.tsSec && !leakInterval.tsMilliSec)) 
        return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);

    bucket = (ClLeakyBucketT *) clHeapCalloc(1, sizeof(*bucket));
    CL_ASSERT(bucket != NULL);

    rc = clOsalMutexInit(&bucket->mutex);
    CL_ASSERT (rc == CL_OK);
    rc = clOsalCondInit(&bucket->cond);
    CL_ASSERT (rc == CL_OK);
    bucket->volume = volume;
    bucket->leakSize = CL_MIN(leakSize, volume);
    bucket->lowWMHit = CL_FALSE;
    bucket->highWMHit = CL_FALSE;
    if(waterMark)
    {
        memcpy(&bucket->waterMark, waterMark, sizeof(bucket->waterMark));
        if(bucket->waterMark.lowWM > bucket->waterMark.highWM)
        {
            bucket->waterMark.highWM = bucket->waterMark.lowWM;
            bucket->waterMark.lowWM /= 2;
        }
        if(!bucket->waterMark.lowWMDelay.tsSec &&
           !bucket->waterMark.lowWMDelay.tsMilliSec)
        {
            /* 
             * no delay, hence reset limit
             */
            bucket->waterMark.lowWM = 0;
        }
        if(!bucket->waterMark.highWMDelay.tsSec &&
           !bucket->waterMark.highWMDelay.tsMilliSec)
        {
            bucket->waterMark.highWM = 0;
        }
    }

    rc = clTimerCreateAndStart(leakInterval,
                               CL_TIMER_REPETITIVE,
                               CL_TIMER_TASK_CONTEXT,
                               clLeakyBucketIntervalCallback,
                               (ClPtrT)bucket,
                               &bucket->timer);
    CL_ASSERT(rc==CL_OK);

    *handle = (ClLeakyBucketHandleT)bucket;
    return CL_OK;
}

ClRcT clLeakyBucketCreate(ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval,  ClLeakyBucketHandleT *handle)
{
    return clLeakyBucketCreateExtended(volume, leakSize, leakInterval, NULL, handle);
}

ClRcT clLeakyBucketCreateSoft(ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval,ClLeakyBucketWaterMarkT *waterMark, ClLeakyBucketHandleT *handle)
{
    return clLeakyBucketCreateExtended(volume, leakSize, leakInterval, waterMark, handle);
}

ClRcT clLeakyBucketDestroy(ClLeakyBucketHandleT *handle)
{
    ClLeakyBucketT *bucket;
    if(!handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    bucket = (ClLeakyBucketT*)*handle;
    if(!bucket) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    clOsalMutexLock(&bucket->mutex);
    if(bucket->waiters > 0)
    {
        clOsalMutexUnlock(&bucket->mutex);
        return CL_LEAKY_BUCKET_RC(CL_ERR_INUSE);
    }
    bucket->value = 0;
    /*delete the bucket timer*/
    clTimerDelete(&bucket->timer);
    *handle = 0;
    clOsalMutexUnlock(&bucket->mutex);
    clOsalMutexDestroy(&bucket->mutex);
    clOsalCondDestroy(&bucket->cond);
    clHeapFree(bucket);
    return CL_OK;
}

static ClRcT __clLeakyBucketFill(ClLeakyBucketT *bucket,
                                 ClInt64T amt,
                                 ClBoolT block)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT delay = { 0, 0};

    clOsalMutexLock(&bucket->mutex);

    amt = CL_MIN(bucket->volume, amt);  /* If the caller tries to put in more than the bucket will ever hold it would block forever so just reduce the value to fill the bucket fully */

    /* Check for soft watermark limits. */
    if(!bucket->highWMHit && bucket->waterMark.highWM && (bucket->value + amt > bucket->waterMark.highWM))
    {
        bucket->highWMHit = CL_TRUE;
        clOsalMutexUnlock(&bucket->mutex);
        clOsalTaskDelay(bucket->waterMark.highWMDelay);
        clOsalMutexLock(&bucket->mutex);
    }

    if(!bucket->lowWMHit && bucket->waterMark.lowWM && (bucket->value + amt > bucket->waterMark.lowWM))
    {
        bucket->lowWMHit = CL_TRUE;
        clOsalMutexUnlock(&bucket->mutex);
        clOsalTaskDelay(bucket->waterMark.lowWMDelay);
        clOsalMutexLock(&bucket->mutex);
    }
    
    /* Now check for hard limits */
    if(bucket->value + amt > bucket->volume)
    {
        if(!block)
        {
            clOsalMutexUnlock(&bucket->mutex);
            return CL_LEAKY_BUCKET_RC(CL_ERR_NO_SPACE);
        }
        
        /* clLogInfo("LEAKY", "BUCKET-FILL", "Leaky bucket blocking caller till there is room in the bucket"); */
        ++bucket->waiters; /* Time for me to wait until the bucket is emptied */
        do
        {
            clOsalCondWait(&bucket->cond, &bucket->mutex, delay);
        } while(bucket->value + amt > bucket->volume);
        --bucket->waiters;
    }
    
    bucket->value += amt;
    clOsalMutexUnlock(&bucket->mutex);
    return rc;
}

ClRcT clLeakyBucketFill(ClLeakyBucketHandleT handle, ClInt64T amt)                           
{
    if(!handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    return __clLeakyBucketFill((ClLeakyBucketT*)handle, amt, CL_TRUE);
}

ClRcT clLeakyBucketTryFill(ClLeakyBucketHandleT handle, ClInt64T amt)
{
    if(!handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    return __clLeakyBucketFill((ClLeakyBucketT*)handle, amt, CL_FALSE);
}

ClRcT clLeakyBucketLeak(ClLeakyBucketHandleT handle)
{
    if(!handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    return clLeakyBucketIntervalCallback((void*)handle);
}

static ClRcT clLeakyBucketTrafficShaperCallbackLocked(ClDequeueTrafficShaperT *shaper)
{
    ClInt64T currentLeaked = 0;
    CL_ASSERT(shaper->callback != NULL);
    while((!shaper->bucket->leakSize || 
           currentLeaked < shaper->bucket->leakSize)
          &&
          !CL_LIST_HEAD_EMPTY(shaper->queue))
    {
        ClListHeadT *pElement = shaper->queue->pNext;
        ClInt64T leaked = 0;
        clListDel(pElement);
        clOsalMutexUnlock(&shaper->bucket->mutex);
        leaked = shaper->callback(pElement);
        clOsalMutexLock(&shaper->bucket->mutex);
        if(leaked > 0)
            currentLeaked += leaked;
    }
    return CL_OK;
}

static ClRcT clLeakyBucketTrafficShaperCallback(void *arg)
{
    ClDequeueTrafficShaperT *shaper = (ClDequeueTrafficShaperT *) arg;
    if(!shaper || !shaper->bucket) return CL_OK;
    clOsalMutexLock(&shaper->bucket->mutex);
    clLeakyBucketTrafficShaperCallbackLocked(shaper);
    clOsalMutexUnlock(&shaper->bucket->mutex);
    return CL_OK;
}

ClRcT clDequeueTrafficShaperCreate(ClListHeadT *queue, ClDequeueTrafficShaperCallbackT callback,
                                   ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval, 
                                   ClDequeueTrafficShaperHandleT *handle)
{
    ClRcT rc = CL_OK;
    ClDequeueTrafficShaperT *shaper = NULL;
    ClLeakyBucketT *bucket = NULL;

    if(!queue || !callback || !handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);

    if(!leakSize || (!leakInterval.tsSec && !leakInterval.tsMilliSec)) 
        return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    
    shaper = (ClDequeueTrafficShaperT *) clHeapCalloc(1, sizeof(*shaper));
    CL_ASSERT(shaper != NULL);

    bucket = (ClLeakyBucketT *) clHeapCalloc(1, sizeof(*bucket));
    CL_ASSERT(bucket != NULL);
    rc = clOsalMutexInit(&bucket->mutex);
    CL_ASSERT (rc == CL_OK);
    rc = clOsalCondInit(&bucket->cond);
    CL_ASSERT(rc == CL_OK);
    bucket->volume = volume;
    bucket->leakSize = leakSize;
    shaper->bucket = bucket;
    shaper->queue = queue;
    shaper->callback = callback;

    rc = clTimerCreateAndStart(leakInterval,
                               CL_TIMER_REPETITIVE,
                               CL_TIMER_SEPARATE_CONTEXT,
                               clLeakyBucketTrafficShaperCallback,
                               (ClPtrT)shaper,
                               &bucket->timer);
    if(rc != CL_OK)
        goto out_free;

    *handle = (ClDequeueTrafficShaperHandleT)shaper;
    return rc;

    out_free:
    clHeapFree(shaper);
    if(bucket)
    {
        clOsalMutexDestroy(&bucket->mutex);
        clOsalCondDestroy(&bucket->cond);
        clHeapFree(bucket);
    }
    return rc;
}

/*
 * Safe adds to the shaper controlled by the leaky bucket
 */
ClRcT clDequeueTrafficShaperAdd(ClDequeueTrafficShaperHandleT handle, ClListHeadT *element)
{
    ClRcT rc = CL_OK;
    ClDequeueTrafficShaperT *shaper = (ClDequeueTrafficShaperT*)handle;

    if(!handle || !element) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    if(!shaper->bucket) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_STATE);

    clOsalMutexLock(&shaper->bucket->mutex);
    clListAddTail(element, shaper->queue);
    clOsalMutexUnlock(&shaper->bucket->mutex);

    return rc;
}

ClRcT clDequeueTrafficShaperLeak(ClDequeueTrafficShaperHandleT handle)
{
    return clLeakyBucketTrafficShaperCallback((void*)handle);
}

ClRcT clDequeueTrafficShaperDestroy(ClDequeueTrafficShaperHandleT *handle)
{
    ClRcT rc = CL_OK;
    ClDequeueTrafficShaperT *shaper = NULL;
    if(!handle || !*handle) return CL_LEAKY_BUCKET_RC(CL_ERR_INVALID_PARAMETER);
    shaper = (ClDequeueTrafficShaperT*)*handle;
    if(!shaper || !shaper->bucket) return CL_ERR_INVALID_PARAMETER;

    clOsalMutexLock(&shaper->bucket->mutex);
    shaper->bucket->leakSize = 0;
    clLeakyBucketTrafficShaperCallbackLocked(shaper);
    *handle = 0;
    clOsalMutexUnlock(&shaper->bucket->mutex);

    clTimerDelete(&shaper->bucket->timer);
    
    clOsalMutexDestroy(&shaper->bucket->mutex);
    clOsalCondDestroy(&shaper->bucket->cond);

    clHeapFree(shaper->bucket);
    clHeapFree(shaper);

    return rc;
}
