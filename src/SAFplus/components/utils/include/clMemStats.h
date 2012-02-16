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
/*
  Memstats module for updating mem.stats
*/
#ifndef _CL_MEM_STATS_H_
#define _CL_MEM_STATS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clPoolIpi.h>

/*For infinite process wide limits*/
#define CL_MEM_STATS_MAX_LIMIT (0x0)

/*Allocation and frees */
typedef enum ClMemDirection
{
    CL_MEM_ALLOC = 0x0,
    CL_MEM_FREE  = 0x1,
} ClMemDirectionT;

typedef struct ClMemStats
{
    ClUint32T numAllocs;
    ClUint32T numFrees;
    ClUint32T currentAllocSize;/*current total alloc size*/
    ClUint32T maxAllocSize;/*max alloc. size*/
    ClUint32T numPools; /*number of pools for this config*/
} ClMemStatsT;

typedef struct ClEoMemConfig
{
    ClUint32T     memLimit;
    ClWaterMarkT memLowWaterMark;
    ClWaterMarkT memHighWaterMark;
    ClWaterMarkT memMediumWaterMark;

#define low_low_wm  memLowWaterMark.lowLimit
#define low_high_wm memLowWaterMark.highLimit
#define medium_low_wm  memMediumWaterMark.lowLimit
#define medium_high_wm  memMediumWaterMark.highLimit
#define high_low_wm memHighWaterMark.lowLimit
#define high_high_wm memHighWaterMark.highLimit

} ClEoMemConfigT;


#define CL_MEM_STATS_CHECK_WRAP_INCR(v,incr,wrapped) do {   \
    if( ( (v) + (incr) ) < (v) )                            \
    {                                                       \
        wrapped = CL_TRUE;                                  \
        (v) = 0;                                            \
    }                                                       \
    (v) += (incr);                                          \
}while(0)

#define CL_MEM_STATS_CHECK_WRAP_DECR(v,decr,wrapped) do {   \
    (v) -= (decr);                                          \
    if( (ClInt32T)(v) < 0 )                                 \
    {                                                       \
        wrapped = CL_TRUE;                                  \
        (v) = 0;                                            \
    }                                                       \
}while(0)

#define CL_MEM_STATS_UPDATE_MAX(v,max) do {     \
    if ( (v) > (max) )                          \
    {                                           \
        (max) = (v) ;                           \
    }                                           \
}while(0)

#define CL_MEM_STATS_UPDATE_ALLOCS(stats,wrapped) do {          \
    CL_MEM_STATS_CHECK_WRAP_INCR((stats).numAllocs,1,wrapped);  \
}while(0)

#define CL_MEM_STATS_UPDATE_FREES(stats,wrapped) do {           \
    CL_MEM_STATS_CHECK_WRAP_INCR((stats).numFrees,1,wrapped);   \
}while(0)

#define CL_MEM_STATS_ALLOCSIZE_INCR(stats,size) do {    \
    (stats).currentAllocSize += (size);                 \
}while(0)

#define CL_MEM_STATS_ALLOCSIZE_DECR(stats,size) do {    \
    (stats).currentAllocSize -= (size);                 \
}while(0)

#define CL_MEM_STATS_UPDATE_ALLOC(stats,size,wrapped) do {                  \
    CL_MEM_STATS_CHECK_WRAP_INCR((stats).currentAllocSize,size,wrapped);    \
    CL_MEM_STATS_UPDATE_ALLOCS(stats,wrapped);                              \
}while(0)

#define CL_MEM_STATS_UPDATE_FREE(stats,size,wrapped) do {   \
    CL_MEM_STATS_UPDATE_MAX((stats).currentAllocSize,       \
                            (stats).maxAllocSize);          \
    CL_MEM_STATS_CHECK_WRAP_DECR((stats).currentAllocSize,  \
                                 size,wrapped);             \
    CL_MEM_STATS_UPDATE_FREES(stats,wrapped);               \
}while(0)

ClRcT clMemStatsInitialize(const ClEoMemConfigT *pConfig);
ClRcT clMemStatsFinalize(void);
ClRcT clMemStatsWaterMarksSet(const ClEoMemConfigT *pMemConfig);
void clEoMemWaterMarksUpdate(ClMemDirectionT memDir);
ClBoolT clEoMemAdmitAllocate(ClUint32T size);
void clEoMemNotifyFree(ClUint32T size);

#ifdef __cplusplus
}
#endif

#endif
    

    
