/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _CL_LEAKY_BUCKET_H_
#define _CL_LEAKY_BUCKET_H_

#include <sys/mman.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clList.h>
#include <clOsalApi.h>
#include <clTimerApi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef ClPtrT ClLeakyBucketHandleT;

typedef ClPtrT ClDequeueTrafficShaperHandleT ;

typedef ClInt64T (*ClDequeueTrafficShaperCallbackT)(ClListHeadT *element);

/*
 * Default global leaky bucket for the process.
 */
extern ClLeakyBucketHandleT gClLeakyBucket;

typedef struct ClLeakyBucketWaterMark
{
    ClInt64T lowWM;
    ClInt64T highWM;
    ClTimerTimeOutT lowWMDelay;
    ClTimerTimeOutT highWMDelay;
}ClLeakyBucketWaterMarkT;

extern ClRcT clLeakyBucketCreate(ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval, ClLeakyBucketHandleT *handle);

extern ClRcT clLeakyBucketCreateSoft(ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval, 
                                     ClLeakyBucketWaterMarkT *waterMark, ClLeakyBucketHandleT *handle);

extern ClRcT clLeakyBucketDestroy(ClLeakyBucketHandleT *handle);

extern ClRcT clLeakyBucketFill(ClLeakyBucketHandleT handle, ClInt64T amt);

extern ClRcT clLeakyBucketTryFill(ClLeakyBucketHandleT handle, ClInt64T amt);

extern ClRcT clLeakyBucketLeak(ClLeakyBucketHandleT handle);

extern ClRcT clDequeueTrafficShaperCreate(ClListHeadT *queue, ClDequeueTrafficShaperCallbackT callback,
                                          ClInt64T volume, ClInt64T leakSize, ClTimerTimeOutT leakInterval, 
                                          ClDequeueTrafficShaperHandleT *handle);

extern ClRcT clDequeueTrafficShaperAdd(ClDequeueTrafficShaperHandleT handle, ClListHeadT *element);

extern ClRcT clDequeueTrafficShaperLeak(ClDequeueTrafficShaperHandleT handle);

extern ClRcT clDequeueTrafficShaperDestroy(ClDequeueTrafficShaperHandleT *handle);


#ifdef __cplusplus
}
#endif

#endif
