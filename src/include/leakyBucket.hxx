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
#include <cltypes.h>
#include <stdio.h>

/*
 * Default global leaky bucket for the process.
 */
using namespace boost::intrusive;
namespace SAFplus
{
    typedef struct leakyBucketWaterMark
    {
        long long lowWM;
        long long highWM;
        long lowWMDelay;
        long highWMDelay;
    }leakyBucketWaterMarkT;

    class leakyBucket
    {
      protected:
        bool __leakyBucketFill(long long amt,bool block);
      public:
        SAFplus::Mutex mutex;
        SAFplus::ThreadCondition  cond;
        int     waiters;
        long long     value; /*current limit*/
        long long     volume; /*bucket size*/
        long long     leakSize; /*how much to leak on timer fire*/
        leakyBucketWaterMarkT waterMark; /*bucket marks for delay*/
        bool isStopped;
        bool lowWMHit;
        bool highWMHit;
        long leakInterval;
        bool leakyBucketCreateExtended(long long volume, long long leakSize, long leakInterval,leakyBucketWaterMarkT *waterMark);
        bool leakyBucketCreate(long long volume, long long leakSize, long leakInterval);
        /* Introduces a delay as the bucket nears empty */
        bool leakyBucketCreateSoft(long long volume, long long leakSize, long leakInterval,leakyBucketWaterMarkT *waterMark);
        bool leakyBucketDestroy();
        bool leakyBucketFill(long long amt);
        bool leakyBucketTryFill(long long amt);
        bool leakyBucketLeak();
    };
}

#endif

