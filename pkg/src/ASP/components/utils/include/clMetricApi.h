/*
 * Copyright (C) 2002-2007 by OpenClovis Inc. All  Rights Reserved.
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

#ifndef _CL_METRIC_API_H_
#define _CL_METRIC_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clLogApi.h>
#include <clMgdApi.h>

/**
 * \file 
 * \brief Header file of Log Service related APIs
 * \ingroup util_apis
 */

/**
 * \addtogroup util_apis
 * \{
 */


struct ClMetricTstruct;
/**
 * \brief Metric watermark exceeded callback definition
 */

typedef void (* ClMetricWatermarkCallbackT) (
  CL_IN   struct ClMetricTstruct* metric,    /**< The metric whose watermark was exceeded */
  CL_IN   int        value,     /**< The current value of the metric */
  CL_IN   int        logLevel,  /**< What severity level was exceeded */
  CL_IN   ClBoolT    exceeded,  /**< CL_TRUE if cond is active, else cleared */
  CL_IN   ClBoolT    high      /**< CL_TRUE if high watermark, else low */
);

  /** Metric options */
typedef enum
  {
    CL_METRIC_NO_OPTIONS  = 0,

    /** Always issue all conditions, even if I just issued a clear. */
    CL_METRIC_NO_DEBOUNCE = 1,
    /** Do not issue condition cleared messages when the metric moves back into
        a safe zone. */
    CL_METRIC_NO_CLEAR    = 2,

    /** If the metric moves from level A to C, issue both B and C, not just C */
    CL_METRIC_NO_SKIP     = 4,

  } ClMetricOptionsT;


typedef enum ClMetricType
{
  CL_METRIC_PERCENT,
  CL_METRIC_MILLISEC,
  CL_METRIC_BYTES,
  CL_METRIC_MEGABYTES,
  CL_METRIC_UNITS
}ClMetricTypeT;

#define CL_METRIC_STR(id) \
    ((id) == CL_METRIC_ALL) ? "all" :           \
    ((id) == CL_METRIC_CPU) ? "cpu" :           \
    ((id) == CL_METRIC_MEM) ? "mem" :           \
    "unknown"



/**
 * \brief Metric definition
 * \par Description:
 * A metric is a data structure that is used to track a value.  It is 
 * essentially a thread-safe integer.  However, the value is accessible through
 * various debugging APIs (TBD), and if the value exceeds certain 
 * user-definable thresholds (low or high) then notification actions will be
 * taken.  These include logging and (TBD) SNMP traps.  These thresholds are
 * optionally debounced -- that is, if a value is constantly crossing and
 * recrossing a threshold, notification will be sent only once.

 * The user can also install a callback to implement unique behavior when a 
 * threshold is exceeded.
 *
 */
typedef struct clMetricTstruct
{
  ClMgdLeafT mgd;
  
  /** \brief Description of this metric
   */
  ClCharT*  desc;

  /** \brief What type of data is stored.  That is, what is a unit of? Is it a measure of time, bytes, percentage, etc */
  ClMetricTypeT type;

  /** \brief What is the current value of the metric. READ ONLY use API to write
   *  \sa ClMetricAdjust(), ClMetricSet()
   */
  ClInt32T  value; 

  /** behavioral modification */ 
  ClMetricOptionsT flags;

  /** Watermark tracking info -- Do not use
   * \sa ClMetricSetLowWatermark(), ClMetricSetHighWatermark()
   */
  ClInt32T lowThreshold[CL_LOG_SEV_DEBUG+1];
  ClInt32T highThreshold[CL_LOG_SEV_DEBUG+1];

  ClMetricWatermarkCallbackT callback;
  
} ClMetric2T;


/* Another metric definition */

typedef enum ClMetricId
{
    CL_METRIC_ALL,
    CL_METRIC_CPU,
    CL_METRIC_MEM,
    CL_METRIC_MAX,
}ClMetricIdT;

typedef struct ClMetric
{
    const ClCharT *pType;
    ClMetricIdT id;
    ClUint32T maxThreshold;
    ClUint32T currentThreshold;
    ClUint32T maxOccurences;
    ClUint32T numOccurences;
}ClMetricT;

#define CL_METRIC_STR(id) \
    ((id) == CL_METRIC_ALL) ? "all" :           \
    ((id) == CL_METRIC_CPU) ? "cpu" :           \
    ((id) == CL_METRIC_MEM) ? "mem" :           \
    "unknown"


/**
 ************************************
 *  \brief Initialize a metric
 *
 *  \par Description:
 *   This Walk callback function gets called, whenever user performs traverse
 *   on the Queue.
 *
 *  \param metric       (in) Pointer to allocated memory
 *  \param name         (in) Identifies the purpose of this metric to the user
 *  \param initialValue (in) Starting value of the metric
 *  \param placement    (in) NULL (not currently used)
 *  \param options      (in) 0 (not currently used)
 *  \param desc         (in) Description.  Should be a const string (I will use this string, not copy it)
 *
 *  \sa clMetricDelete()
 */
void clMetricInit(ClMetric2T* metric, char* name, int initialValue, void* placement, ClMetricOptionsT options, char* desc);

/**
 ************************************
 *  \brief Delete a metric
 *
 *  \param metric       (in) Pointer to initialized metric
 *
 *  \sa ClMetricInit()
 */
void clMetricDelete(ClMetric2T* metric);

/**
 *  \brief Sets a watermark
 *
 *  \par Description:
 *  If the metric descends below this watermark, action is taken
 *
 *  \param metric       (in) Pointer to initialized metric
 *  \param loglevel     (in) Severity level of this watermark
 *  \param value        (in) Level of the watermark
 *  \sa clMetricSetHighWatermark
 */
void clMetricSetLowWatermark(ClMetric2T* metric, int logLevel, int value);

/**
 *  \brief Sets a watermark
 *
 *  \par Description:
 *  If the metric ascends above this watermark, action is taken
 *
 *  \sa 
 */
void clMetricSetHighWatermark(ClMetric2T* metric, int logLevel, int value);

/**
 *  \brief change the value of a metric by some amount (+ or -)
 *
 *  \par Description:
 *  This function adds an amount to a metric allowing it to be changed
 *  positively or negatively.  If a threshold is exceeded, action is taken.
 *  
 *  \sa ClMetricSet(),
 */
int  clMetricAdjust(ClMetric2T* metric, int amt);

/**
 *  \brief 
 *
 *  \par Description:
 *  This function sets the metric's value to the amt. If a threshold is 
 *  exceeded, action is taken.
 *
 *  \sa clMetricAdjust()
 */
int  clMetricSet(ClMetric2T* metric, int amt);


/** 
 * \} 
 */
#ifdef __cplusplus
}
#endif

#endif


