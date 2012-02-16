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
#ifndef _CL_EO_QUEUE_STATS_H_
#define _CL_EO_QUEUE_STATS_H_

#include <clEoQueue.h>

typedef struct ClEoQueueStats
{
    ClBoolT  used;
    ClUint8T priority;
    ClUint8T proto;
    ClUint32T count;
    struct timeval start;
    struct timeval end;
    ClUint64T minTime;
    ClUint64T maxTime;
    ClUint64T totalTime;
    ClUint32T minQueueSize;
    ClUint32T maxQueueSize;
    ClUint64T totalQueueSize;
}ClEoQueueStatsT;

typedef struct ClEoQueueProtoUsage
{
    ClInt64T bytes;
    ClUint8T proto;
    ClUint32T numMsgs;
}ClEoQueueProtoUsageT;

typedef struct ClEoQueuePriorityUsage
{
    ClInt64T bytes;
    ClUint8T priority;
    ClUint32T numMsgs;
    ClTaskPoolUsageT taskPoolUsage;
}ClEoQueuePriorityUsageT;

typedef struct ClEoQueueDetails
{
    ClEoQueueProtoUsageT *protos; /*protos map*/
    ClUint32T numProtos;
    ClEoQueuePriorityUsageT *priorities; /*priorities map*/
    ClUint32T numPriorities;
}ClEoQueueDetailsT;

#if defined (EO_QUEUE_STATS)
extern void clEoQueueStatsStart(ClUint32T queueSize, ClEoJobT *pJob, ClEoQueueStatsT *pStats);
extern void clEoQueueStatsStop(ClUint32T queueSize, ClEoJobT *pJob, ClEoQueueStatsT *pStats);

#else

#define clEoQueueStatsStart(size, job, stats) do { (void)(stats); (void)(size); } while(0)
#define clEoQueueStatsStop(size, job, stats) do { (void)(stats);  (void)(size); } while(0)

#endif

extern ClRcT clEoQueueStatsGet(ClEoQueueDetailsT *pEoQueueDetails);

#endif
