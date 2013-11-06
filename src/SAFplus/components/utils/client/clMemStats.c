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
/*The memstats process wide lib.*/
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clLogApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clMemStats.h>
#include <clEoApi.h>
#include <clEoIpi.h>

#define CL_MEM_STATS_LOCK_INIT() clOsalMutexInit(&gClEoMemStats.mutex);

#define CL_MEM_STATS_LOCK() do {                \
        clOsalMutexLock(&gClEoMemStats.mutex);  \
}while(0)

#define CL_MEM_STATS_UNLOCK() do {                  \
        clOsalMutexUnlock(&gClEoMemStats.mutex) ;   \
}while(0)

#define CL_MEM_STATS_LOCK_DESTROY() do {            \
        clOsalMutexDestroy(&gClEoMemStats.mutex);   \
}while(0)

#define CL_MEM_STATS_MEM_LIMIT (gClEoMemStats.memConfig.memLimit)

#define CL_MEM_STATS_ALLOC_SIZE (gClEoMemStats.memStats.currentAllocSize)

#define CL_MEM_STATS_LOG(sev,str,...) clEoLibLog(CL_CID_MEM,sev,str,__VA_ARGS__)

#define CL_MEM_STATS_CHECK_LIMIT(size)                  \
    (CL_MEM_STATS_MEM_LIMIT != CL_MEM_STATS_MAX_LIMIT   \
     &&                                                 \
     CL_MEM_STATS_ALLOC_SIZE + (size) >                 \
     CL_MEM_STATS_MEM_LIMIT                             \
    )

#define CL_MEM_STATS_CONVERT_WM(wm) ((CL_MEM_STATS_MEM_LIMIT*(wm))/100)

#define HIGH_HIGH_WM (gClEoMemStats.memConfig.high_high_wm)
#define HIGH_LOW_WM  (gClEoMemStats.memConfig.high_low_wm)
#define MEDIUM_HIGH_WM (gClEoMemStats.memConfig.medium_high_wm)
#define MEDIUM_LOW_WM  (gClEoMemStats.memConfig.medium_low_wm)
#define LOW_HIGH_WM (gClEoMemStats.memConfig.low_high_wm)
#define LOW_LOW_WM  (gClEoMemStats.memConfig.low_low_wm)

#define LOW_WM_BIT (0x0)
#define MEDIUM_WM_BIT (0x1)
#define HIGH_WM_BIT   (0x2)

#define CHECK_WM_BIT(bit) (gClEoMemStats.waterMarkBits &  (1 << (bit))  )
#define SET_WM_BIT(bit)   (gClEoMemStats.waterMarkBits |= (1 << (bit))  )
#define CLEAR_WM_BIT(bit) (gClEoMemStats.waterMarkBits &= ~(1 << (bit)) ) 

/*
  Check wms. after alloc.
  Cascading style checks.
  Start from high and go down till low.
  for allocations.
  Follow the reverse way for frees.
 */

#define CL_CHECK_WM_ALLOC(wm,label) do {        \
    wm = CL_WM_HIGH;                            \
    if(CL_MEM_STATS_ALLOC_SIZE >                \
       HIGH_HIGH_WM                             \
       &&                                       \
       !CHECK_WM_BIT(HIGH_WM_BIT)               \
       )                                        \
        goto set3;                              \
    wm = CL_WM_MED;                             \
    if(CL_MEM_STATS_ALLOC_SIZE >                \
       MEDIUM_HIGH_WM                           \
       &&                                       \
       !CHECK_WM_BIT(MEDIUM_WM_BIT)             \
       )                                        \
        goto set2;                              \
    wm = CL_WM_LOW;                             \
    if(CL_MEM_STATS_ALLOC_SIZE >                \
       LOW_HIGH_WM                              \
       &&                                       \
       !CHECK_WM_BIT(LOW_WM_BIT)                \
       )                                        \
        goto set1;                              \
    goto label;                                 \
    set3:                                       \
    SET_WM_BIT(HIGH_WM_BIT);                    \
    set2:                                       \
    SET_WM_BIT(MEDIUM_WM_BIT);                  \
    set1:                                       \
    SET_WM_BIT(LOW_WM_BIT);                     \
}while(0)                                       \

#define CL_CHECK_WM_FREE(wm,label) do {         \
    wm = CL_WM_LOW;                             \
    if(CL_MEM_STATS_ALLOC_SIZE <                \
       LOW_LOW_WM                               \
       &&                                       \
       CHECK_WM_BIT(LOW_WM_BIT)                 \
       )                                        \
        goto clear3;                            \
    wm = CL_WM_MED;                             \
    if(CL_MEM_STATS_ALLOC_SIZE <                \
       MEDIUM_LOW_WM                            \
       &&                                       \
       CHECK_WM_BIT(MEDIUM_WM_BIT)              \
       )                                        \
        goto clear2;                            \
    wm = CL_WM_HIGH;                            \
    if(CL_MEM_STATS_ALLOC_SIZE <                \
       HIGH_LOW_WM                              \
       &&                                       \
       CHECK_WM_BIT(HIGH_WM_BIT)                \
       )                                        \
        goto clear1;                            \
    goto label;                                 \
    clear3:                                     \
    CLEAR_WM_BIT(LOW_WM_BIT);                   \
    clear2:                                     \
    CLEAR_WM_BIT(MEDIUM_WM_BIT);                \
    clear1:                                     \
    CLEAR_WM_BIT(HIGH_WM_BIT);                  \
}while(0)

/*
  EOs watermark trigger
*/
#define CL_EO_WM_TRIGGER(wm,wmFlag) do {                       \
    clEoWaterMarkHit(CL_CID_MEM,                            \
                     (wm),                                  \
                     gClEoWaterMarks[(wm)],(wmFlag),NULL);  \
}while(0)
                     
/*global config provided by EO*/
typedef struct ClEoMemStats
{
    ClEoMemConfigT memConfig;
    ClMemStatsT memStats;
    ClOsalMutexT mutex;
    ClBoolT initialized;
    ClCharT waterMarkBits;
} ClEoMemStatsT;

#ifndef __cplusplus
static ClEoMemStatsT gClEoMemStats = { 
    .initialized = CL_FALSE,
    .waterMarkBits = 0,
};
#else
static ClEoMemStatsT gClEoMemStats = {{0},{0},{},0,0};

#endif

static ClWaterMarkT *gClEoWaterMarks[] = {
    &gClEoMemStats.memConfig.memLowWaterMark,
    &gClEoMemStats.memConfig.memHighWaterMark,
    &gClEoMemStats.memConfig.memMediumWaterMark,
};

ClRcT clMemStatsWaterMarksSet(const ClEoMemConfigT *pMemConfig)
{
    ClRcT rc = CL_OK;
    HIGH_HIGH_WM =   CL_MEM_STATS_CONVERT_WM(pMemConfig->high_high_wm);
    HIGH_LOW_WM  =   CL_MEM_STATS_CONVERT_WM(pMemConfig->high_low_wm);
    MEDIUM_HIGH_WM = CL_MEM_STATS_CONVERT_WM(pMemConfig->medium_high_wm);
    MEDIUM_LOW_WM =  CL_MEM_STATS_CONVERT_WM(pMemConfig->medium_low_wm);
    LOW_HIGH_WM =    CL_MEM_STATS_CONVERT_WM(pMemConfig->low_high_wm);
    LOW_LOW_WM =     CL_MEM_STATS_CONVERT_WM(pMemConfig->low_low_wm);
    return rc;
}

static __inline__ ClRcT clMemStatsMemConfigSet(const ClEoMemConfigT *pMemConfig)
{
    memcpy(&gClEoMemStats.memConfig,pMemConfig,sizeof(gClEoMemStats.memConfig));
    return clMemStatsWaterMarksSet(pMemConfig);
}

/*Called by the EO subsystem*/
ClRcT clMemStatsInitialize(const ClEoMemConfigT *pConfig)
{
    ClRcT rc = CL_OK;

    if(gClEoMemStats.initialized == CL_TRUE)
    {
        goto out;
    }

    rc = CL_ERR_INVALID_PARAMETER;
    if(pConfig == NULL)
    {
        clLogError("MEM","INI","Invalid param\n");
        goto out;
    }

    rc = clMemStatsMemConfigSet(pConfig);

    if(rc != CL_OK)
    {
        clLogError("MEM","INI","Error setting watermarks\n");
        goto out;
    }

    rc = CL_MEM_STATS_LOCK_INIT();
    if(rc != CL_OK)
    {
        clLogError("MEM","INI",
                   "CL_MEM_STATS_LOCK_INIT failed, rc=[%#X]\n", rc);
        goto out;
    }


    gClEoMemStats.initialized = CL_TRUE;

    rc = CL_OK;
    out:
    return rc;
}

ClRcT clMemStatsFinalize(void)
{
    ClRcT rc = CL_ERR_NOT_INITIALIZED;
    if(gClEoMemStats.initialized == CL_FALSE)
    {
        goto out;
    }
    gClEoMemStats.initialized = CL_FALSE;
    CL_MEM_STATS_LOCK_DESTROY();
    rc = CL_OK;
    out:
    return rc;
}
/*
  Admit an allocate request.
  Return FALSE if the process wide upper limit is reached.
  Then check for watermarks and trigger events if any.
*/

ClBoolT clEoMemAdmitAllocate(ClUint32T size)
{
    ClBoolT allowRequest = CL_TRUE;
    CL_MEM_STATS_LOCK();
    if(CL_MEM_STATS_CHECK_LIMIT(size))
    {
        allowRequest = CL_FALSE;
        CL_MEM_STATS_LOG(CL_LOG_SEV_ERROR,
                         "Admit allocate failed.Request %d bytes exceeds process upper limit of %d bytes",
                         size,CL_MEM_STATS_MEM_LIMIT);
        goto out_unlock;
    }
    CL_MEM_STATS_ALLOCSIZE_INCR(gClEoMemStats.memStats,size);
    out_unlock:
    CL_MEM_STATS_UNLOCK();
    return allowRequest;
}

void clEoMemNotifyFree(ClUint32T size)
{
    CL_MEM_STATS_LOCK();
    CL_MEM_STATS_ALLOCSIZE_DECR(gClEoMemStats.memStats,size);
    CL_MEM_STATS_UNLOCK();
}

/*Update watermarks on allocations*/
void clEoMemWaterMarksUpdate(ClMemDirectionT memDir)
{
    ClWaterMarkIdT waterMark = (ClWaterMarkIdT) 0;
    ClEoWaterMarkFlagT waterMarkFlag = CL_WM_LOW_LIMIT;

    CL_MEM_STATS_LOCK();

    /*Skipping wm processing for infinite process limit*/
    if(CL_MEM_STATS_MEM_LIMIT == CL_MEM_STATS_MAX_LIMIT)
    {
        goto out_unlock;
    }

    switch(memDir)
    {
    case CL_MEM_ALLOC:
        waterMarkFlag = CL_WM_HIGH_LIMIT;
        CL_CHECK_WM_ALLOC(waterMark,out_unlock);
        break;
    case CL_MEM_FREE:
        waterMarkFlag = CL_WM_LOW_LIMIT;
        CL_CHECK_WM_FREE(waterMark,out_unlock);
        break;
    default:
        goto out_unlock;
    }

    /*we are here when the watermark is triggered*/
    CL_MEM_STATS_UNLOCK();

    CL_EO_WM_TRIGGER(waterMark,waterMarkFlag);
    return ;

    out_unlock:
    CL_MEM_STATS_UNLOCK();
    return ;
}
