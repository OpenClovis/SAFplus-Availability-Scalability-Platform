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

static void leakyBucketIntervalCallback(void *arg)
{
    LeakyBucket *bucket = (LeakyBucket *) arg;
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
}

static void timerLeakThreadFunc(void* arg)
{
    LeakyBucket* bucket = (LeakyBucket*)arg;
    logTrace("LKY","BKT", "Leaky bucket leak thread has started");
    while(!bucket->isStopped)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(bucket->leakInterval));
        bucket->leak();
    }
}

LeakyBucket::LeakyBucket()
{
  waiters=0;
  value = 0;
  volume = 0;
  leakSize = 0;
  isStopped = true;
  lowWMHit = false;
  highWMHit = false;
  leakInterval = 0;
}

bool LeakyBucket::start(long long volume, long long leakSize, long leakInterval,LeakyBucketWaterMark *waterMarkp)
{
    bool rc = true;
    mutex.lock();  // Typically SAFplus expects start and stop to be single threaded and therefore uses non-reentrant code, but we have the mutex so might as well use it.
    assert(isStopped);

    if(!volume || !leakSize || !leakInterval)
        return false;
    this->volume = volume;
    this->leakSize = MIN(leakSize, volume);
    this->lowWMHit = false;
    this->highWMHit = false;
    this->leakInterval=leakInterval;
    this->value=0;
    if(waterMarkp)
    {
      waterMark = *waterMarkp;
      //memcpy(&this->waterMark, waterMark, sizeof(this->waterMark));
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
    isStopped = false;
    filler = boost::thread(timerLeakThreadFunc,this);
    mutex.unlock();
    return true;
}


LeakyBucket::~LeakyBucket()
{
  stop();
}

void LeakyBucket::stop()
{
  bool stopping = false;
  mutex.lock();
  if (!isStopped)
    {
      stopping = true;
      while (this->waiters > 0)
        {
          cond.notify_all();
          mutex.unlock();
          boost::this_thread::sleep(boost::posix_time::milliseconds(50));  // Wake up waiting threads because we are quitting!!!
          mutex.lock();
        }
      this->value = 0;
      isStopped = true;
      /*delete the bucket timer*/
      //clTimerDelete(&bucket->timer);
      //clOsalMutexDestroy(&bucket->mutex);
      //clOsalCondDestroy(&bucket->cond);
      //clHeapFree(bucket);
    }
  mutex.unlock();
  if (stopping) filler.join();  // wait for the filler thread to terminate
}

bool LeakyBucket::fill(long long amt,bool block)
{
    bool rc = true;
    long delay = 0;
    mutex.lock();
    if (isStopped)
      {
        mutex.unlock();
        throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::SERVICE_STOPPED,"Leaky bucket is stopped", __FILE__, __LINE__);
      }
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
          cond.timed_wait(mutex,delay);
          if (isStopped)
            {
              --waiters;
              mutex.unlock();
              throw SAFplus::Error(Error::ErrorFamily::SAFPLUS_ERROR, Error::SERVICE_STOPPED,"Leaky bucket has been stopped", __FILE__, __LINE__);
            }
        } while(value + amt > volume);
        --waiters;
    }
    value += amt;
    mutex.unlock();
    return rc;
}

void LeakyBucket::fill(long long amt)
{
    fill(amt, true);
}

bool LeakyBucket::tryFill(long long amt)
{
    return fill(amt,false);
}

void LeakyBucket::leak()
{
    mutex.lock();
    value -= leakSize;
    if(value < 0) value = 0;
    if(waiters > 0)
    {
        cond.notify_all();
    }
    lowWMHit = false;
    highWMHit = false;
    mutex.unlock();
}


