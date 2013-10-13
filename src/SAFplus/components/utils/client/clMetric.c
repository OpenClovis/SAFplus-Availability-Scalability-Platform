#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#ifdef SOLARIS_BUILD
#include <strings.h>
#endif

#include <clCommon.h>
#include <clMetricApi.h>
#include <clDebugApi.h>

void clMetricInit(ClMetric2T* metric, const char* name, int initialValue, void* placement, ClMetricOptionsT options, const char* desc)
{
  ClWordT i;
  bzero((char *)metric,sizeof(ClMetric2T));
  saNameSet(&metric->mgd.cmn.name, name);
  metric->value = initialValue;
  metric->desc  = desc;
  for (i = 0; i < CL_LOG_SEV_DEBUG+1 ; i++)
    {
      metric->lowThreshold[i] = INT_MIN;
      metric->highThreshold[i] = INT_MAX;
    }

}

void clMetricDelete(ClMetric2T* metric)
{
    bzero((char*)metric,sizeof(ClMetric2T));
}

void clMetricSetLowWatermark(ClMetric2T* metric, int logLevel, int value)
{
  CL_ASSERT(logLevel < CL_LOG_SEV_DEBUG);
  metric->lowThreshold[logLevel] = value;
}

void clMetricSetHighWatermark(ClMetric2T* metric, int logLevel, int value)
{
  CL_ASSERT(logLevel < CL_LOG_SEV_DEBUG);
  metric->highThreshold[logLevel] = value;
}


int  clMetricAdjust(ClMetric2T* metric, int amt)
{
  metric->value += amt;

#if 0
  for (i = 0; i < CL_LOG_SEV_DEBUG+1 ; i++)
    {
      if (metric->value < metric->lowThreshold[i])
        {
          ClLog();
          break;
        }
    }

  for (i = 0; i < CL_LOG_SEV_DEBUG+1 ; i++)
    {
      if (metric->value > metric->highThreshold[i])
        {
          ClLog();
          break;
        }
#endif

      return metric->value;
}

int  clMetricSet(ClMetric2T* metric, int amt)
  {
    metric->value = amt;
    return metric->value;
  }
