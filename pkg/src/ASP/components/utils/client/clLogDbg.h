/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#ifndef _CL_LOG_DBG_H_
#define _CL_LOG_DBG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/time.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.h>
#include <clLogUtilApi.h>
#include <clOsalApi.h>

/*
 * Array contains all information about log messages
 */
typedef struct
{
    ClHandleT       handle;
    ClLogSeverityT  severity;
    ClUint16T       serviceId;
    ClUint16T       msgId;
    ClCharT         msgHeader[CL_MAX_NAME_LENGTH];
    ClCharT         msg[CL_LOG_MAX_MSG_LEN];
}ClLogDeferredHeaderT;

typedef enum
{
    CL_LOG_NO_MASK    = 0,
    CL_LOG_PART_MASK  = 1,
    CL_LOG_FULL_MASK  = 2,
}ClLogRuleMaskT;

typedef struct
{
    ClCharT        str[CL_MAX_NAME_LENGTH];
    ClUint32T      length;
    ClLogRuleMaskT mask;
} ClLogRuleMemT;

typedef struct
{
    ClLogSeverityT severity;
    ClLogRuleMemT  ruleMems[5];
}ClLogRuleT;

typedef struct
{
    time_t          mt_time;
    ClUint32T       numRules;
    ClLogRuleT      *pRules;  
    ClOsalMutexIdT  mutex;
} ClLogRulesInfoT;

#define CL_LOG_MAX_NAME        (0xff)

typedef struct ClLogFilter
{
    ClCharT        node[CL_LOG_MAX_NAME+1];
    ClCharT        server[CL_LOG_MAX_NAME+1];
    ClCharT        area[CL_LOG_MAX_NAME+1];
    ClCharT        context[CL_LOG_MAX_NAME+1];
    ClCharT        file[CL_LOG_MAX_NAME+1];
    ClLogSeverityT severity;
    ClUint32T      filterMap;
} ClLogRulesFilterT;

typedef struct ClLogRules
{
    ClUint32T         numFilters;
    ClLogRulesFilterT *pFilters;
}ClLogRulesT;


extern ClRcT
clLogDbgFileClose(void);

extern ClRcT
clLogRulesParse(void);

extern ClRcT
clLogTimeGet(ClCharT   *pStrTime, ClUint32T maxBytes);

extern ClRcT
clLogWriteDeferredForceWithHeader(ClHandleT       handle,
                                  ClLogSeverityT  severity,
                                  ClUint16T       serviceId,
                                  ClUint16T       msgId,
                                  ClCharT         *pMsgHeader,
                                  ClCharT         *pFmtStr,
                                  ...);

extern ClRcT
clLogWriteDeferredWithHeader(ClHandleT       handle,
                             ClLogSeverityT  severity,
                             ClUint16T       serviceId,
                             ClUint16T       msgId,
                             ClCharT         *pMsgHeader,
                             ClCharT         *pFmtStr,
                             ...);

#ifdef __cplusplus
}
#endif
#endif
