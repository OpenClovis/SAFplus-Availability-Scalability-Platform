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

#ifndef _LEAKY_BUCKET_H_
#define _LEAKY_BUCKET_H_


#include <iostream>
//#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <clCommon.hxx>
#include <clThreadApi.hxx>
#include <boost/thread.hpp>
#include <cltypes.h>
#include <stdio.h>

/*
 * Default global leaky bucket for the process.
 */
using namespace boost::intrusive;
namespace SAFplus
{
    typedef struct
    {
        long long lowWM;
        long long highWM;
        long lowWMDelay;
        long highWMDelay;
    } LeakyBucketWaterMark;

    class LeakyBucket
    {
      protected:
        bool fill(long long amt,bool block);
      boost::thread filler;
      public:
        ~LeakyBucket();
        LeakyBucket();
        SAFplus::Mutex mutex;
        SAFplus::ThreadCondition  cond;
        int     waiters;
        int64_t     value; /*current limit*/
        int64_t     volume; /*bucket size*/
        int64_t     leakSize; /*how much to leak on timer fire*/
        bool isStopped;
        bool lowWMHit;
        bool highWMHit;
        long leakInterval;
        LeakyBucketWaterMark waterMark; /*bucket marks for delay*/
        /* Introduces a delay as the bucket nears empty */
        bool start(long long volume, long long leakSize, long leakInterval,LeakyBucketWaterMark *waterMark=NULL);
        void stop();
        void fill(long long amt);  //? If the leaky bucket is stopped while a thread is waiting for a fill, SAFplus::Error(SAFPLUS_ERROR,SERVICE_STOPPED...) will be raised.
        bool tryFill(long long amt);
        void leak();
    };
}

#endif

