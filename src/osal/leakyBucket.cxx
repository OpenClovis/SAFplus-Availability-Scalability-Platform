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

#include <leakyBucket.hxx>
#include <boost/thread.hpp>


using namespace SAFplus;
using namespace SAFplusI;

/** Mininum macro */
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
/** Maximum macro */
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )

#define CL_LEAKY_BUCKET_RC(rc) CL_RC(CL_CID_LEAKY_BUCKET, rc)

static bool leakyBucketIntervalCallback(void *arg)
{
    leakyBucket *bucket = (leakyBucket *) arg;
    bucket->mutex.lock();
    bucket->value -= bucket->leakSize;
    if(bucket->value < 0) bucket->value = 0;
    if(bucket->waiters > 0)
    {
        bucket->cond.notify_all();
    }
    bucket->lowWMHit = false;
    bucket->highWMHit = false;
    bucket->mutex.unlock();
    return true;
}

static void timerLeakThreadFunc(void* arg)
{
    leakyBucket* tempBucket = (leakyBucket*)arg;
    logTrace("THRPOOL","TIMERFUNC", "timerThreadFunc enter");
    while(!tempBucket->isStopped)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(tempBucket->leakInterval));
        leakyBucketIntervalCallback(arg);
    }
}

bool leakyBucket::leakyBucketCreateExtended(long long volume, long long leakSize, long leakInterval,leakyBucketWaterMarkT *waterMark)
{
    bool rc = true;

    if(!volume || !leakSize || !leakInterval)
        return false;
    this->volume = volume;
    this->leakSize = MIN(leakSize, volume);
    this->lowWMHit = false;
    this->highWMHit = false;
    this->leakInterval=leakInterval;
    this->value=0;
    if(waterMark)
    {
        memcpy(&this->waterMark, waterMark, sizeof(this->waterMark));
        if(this->waterMark.lowWM > this->waterMark.highWM)
        {
            this->waterMark.highWM = this->waterMark.lowWM;
            this->waterMark.lowWM /= 2;
        }
        if(!this->waterMark.lowWMDelay)
        {
            /*
             * no delay, hence reset limit
             */
            this->waterMark.lowWM = 0;
        }
        if(!this->waterMark.highWMDelay)
        {
            this->waterMark.highWM = 0;
        }
    }
    boost::thread(timerLeakThreadFunc,this);
    return true;
}

bool leakyBucket::leakyBucketCreate(long long volume, long long leakSize, long leakInterval)
{
    return leakyBucketCreateExtended(volume, leakSize, leakInterval, NULL);
}

bool leakyBucket::leakyBucketCreateSoft(long long volume, long long leakSize, long leakInterval,leakyBucketWaterMarkT *waterMark)
{
    return leakyBucketCreateExtended(volume, leakSize, leakInterval, waterMark);
}

bool leakyBucket::leakyBucketDestroy()
{
    if(!this) return false;
    mutex.lock();
    if(this->waiters > 0)
    {
        mutex.unlock();
        return false;
    }
    this->value = 0;
    isStopped = true;
    /*delete the bucket timer*/
    //clTimerDelete(&bucket->timer);
    //clOsalMutexDestroy(&bucket->mutex);
    //clOsalCondDestroy(&bucket->cond);
    //clHeapFree(bucket);
    mutex.unlock();
    return true;
}

bool leakyBucket::__leakyBucketFill(long long amt,bool block)
{
    ClRcT rc = true;
    long delay = 0;
    mutex.lock();
    amt = MIN(this->volume, amt);  /* If the caller tries to put in more than the bucket will ever hold it would block forever so just reduce the value to fill the bucket fully */
    /* Check for soft watermark limits. */
    if(!this->highWMHit && this->waterMark.highWM && (this->value + amt > this->waterMark.highWM))
    {
        this->highWMHit = true;
        mutex.unlock();
        //clOsalTaskDelay(this->waterMark.highWMDelay);
        boost::this_thread::sleep(boost::posix_time::milliseconds(this->waterMark.highWMDelay));
        mutex.lock();
    }
    if(!this->lowWMHit && this->waterMark.lowWM && (this->value + amt > this->waterMark.lowWM))
    {
        this->lowWMHit = true;
        mutex.unlock();
        //clOsalTaskDelay(this->waterMark.lowWMDelay);
        boost::this_thread::sleep(boost::posix_time::milliseconds(this->waterMark.lowWMDelay));
        mutex.lock();
    }
    /* Now check for hard limits */
    if(value + amt > volume)
    {
        if(!block)
        {
            mutex.unlock();
            return false;
        }

        /* clLogInfo("LEAKY", "BUCKET-FILL", "Leaky bucket blocking caller till there is room in the bucket"); */
        ++waiters; /* Time for me to wait until the bucket is emptied */
        do
        {
            //clOsalCondWait(&bucket->cond, &bucket->mutex, delay);
            cond.timed_wait(mutex,delay);
        } while(value + amt > volume);
        --waiters;
    }
    value += amt;
    mutex.unlock();
    return rc;
}
bool leakyBucket::leakyBucketFill(long long amt)
{
    return __leakyBucketFill(amt, true);
}
bool leakyBucket::leakyBucketTryFill(long long amt)
{
    return __leakyBucketFill(amt,false);
}
bool leakyBucket::leakyBucketLeak()
{
    return leakyBucketIntervalCallback(this);
}


