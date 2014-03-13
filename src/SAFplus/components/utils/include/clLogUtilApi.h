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
/**
 * \file 
 * \brief Header file of Log Service related Macros
 * \ingroup log_apis
 */

/**
 * \addtogroup log_apis
 * \{
 */

#ifndef _CL_LOG_UTIL_API_H_
#define _CL_LOG_UTIL_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdarg.h>
#include <clCommon.h>
#include <clHeapApi.h>


#define  CL_LOG_MAX_MSG_LEN  1024
#define  CL_LOG_MAX_NUM_MSGS 992

/**
 * It takes one of the above severity level. 
 
 typedef ClUint8T   ClLogSeverityT;
 Moved to clLogApi
*/

/*
 * Initialized
 */
ClRcT
clLogUtilLibInitialize(void);

ClRcT
clLogUtilLibFinalize(ClBoolT logLibInit);

/**
 * Area string for unspecified component area
 */
#define CL_LOG_AREA_UNSPECIFIED "---"

/**
 * Context string for unspecified component context
 */
#define CL_LOG_CONTEXT_UNSPECIFIED "---"

/**
 * Default servie id for SYS components.
 */
#define CL_LOG_DEFAULT_SYS_SERVICE_ID   0x01
  

/**
 * This macro provides the support to log messages by specifying
 * the severity of log message and server information like the 
 * sub-component area and the context of logging.
 *
 * This macro is for ASP components only, since it directs all logs
 * to the OpenClovis system log.
 */
#ifdef SAFplus7
#define clLog(severity, area, context, ...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout); } while(0)
#define clLogConsole
#else
#define clLog(severity, area, context, ...)                     \
do                                                              \
{                                                               \
    const ClCharT  *pArea    = CL_LOG_AREA_UNSPECIFIED;         \
    const ClCharT  *pContext = CL_LOG_CONTEXT_UNSPECIFIED;      \
    if( NULL != area )                                          \
    {                                                           \
        pArea = area;                                           \
    }                                                           \
    if( NULL != context )                                       \
    {                                                           \
        pContext = context;                                     \
    }                                                           \
    clLogMsgWrite(CL_LOG_HANDLE_SYS, (ClLogSeverityT)severity,  \
                  CL_LOG_DEFAULT_SYS_SERVICE_ID,                \
                  pArea, pContext, __FILE__, __LINE__,          \
                  __VA_ARGS__);                                 \
} while(0)
    
#endif

#ifndef SAFplus7

#define clLogDeferred(severity, area, context, ...)                     \
do                                                                      \
{                                                                       \
    const ClCharT  *pArea    = CL_LOG_AREA_UNSPECIFIED;                 \
    const ClCharT  *pContext = CL_LOG_CONTEXT_UNSPECIFIED;              \
    if( NULL != area )                                                  \
    {                                                                   \
        pArea = area;                                                   \
    }                                                                   \
    if( NULL != context )                                               \
    {                                                                   \
        pContext = context;                                             \
    }                                                                   \
    clLogMsgWriteDeferred(CL_LOG_HANDLE_SYS, (ClLogSeverityT)severity,  \
                          CL_LOG_DEFAULT_SYS_SERVICE_ID,                \
                          pArea, pContext, __FILE__, __LINE__,          \
                          __VA_ARGS__);                                 \
} while(0)

#define clLogConsole(severity, area, context, ...)                      \
do                                                                      \
{                                                                       \
    const ClCharT  *pArea    = CL_LOG_AREA_UNSPECIFIED;                 \
    const ClCharT  *pContext = CL_LOG_CONTEXT_UNSPECIFIED;              \
    if( NULL != area )                                                  \
    {                                                                   \
        pArea = area;                                                   \
    }                                                                   \
    if( NULL != context )                                               \
    {                                                                   \
        pContext = context;                                             \
    }                                                                   \
    clLogMsgWriteConsole(CL_LOG_HANDLE_SYS, (ClLogSeverityT)severity,   \
                         CL_LOG_DEFAULT_SYS_SERVICE_ID,                 \
                         pArea, pContext, __FILE__, __LINE__,           \
                          __VA_ARGS__);                                 \
} while(0)

/**
 *  \brief Logging for applications
 *  \par Description
 *  This macro provides the support to log messages by specifying
 *  the severity of log message and server information like the 
 *  sub-component area and the context of logging.
 *  \par
 *  This macro is for applications as opposed to system components. By default
 *  it outputs to "app.log" instead of "sys.log" but of course these outputs
 *  are configurable through the clLog.xml file.
 *
 *  \param streamHandle (in) This handle should be a valid stream handle. It should be 
 *   either default handles(CL_LOG_HANDLE_APP & CL_LOG_HANDLE_SYS) or should 
 *  have obtained from previous clLogStreamOpen() call. The stream handle is CL_LOG_HANDLE_SYS,
 *  then it outputs to "sys.log". It is CL_LOG_HANDLE_APP, then it outputs to "app.log". 
 * 
 *  \param severity (in) This field must be set to one of the values defined 
 *  in this file (CL_LOG_SEV_DEBUG ... CL_LOG_SEV_EMERGENCY).  It defines the 
 *  severity level of the Log Record being written.
 *
 *  \param serviceId (in) This field identifies the module within the process
 *  which is generating this Log Record. If the Log Record message is a generic
 *  one like out of memory, this field can be used to narrow down on the module 
 *  impacted. For ASP client libraries, these values are defined in clCommon.h.
 *  For application modules, it is up-to the application developer to define the
 *  values and scope of those values.
 *
 *  \param area (in) This is a 3 letter string identifying the process (or 
 *  component) that generated this message.
 *
 *  \param context (in) This is a string (3 letters preferred) identifying 
 *  the library that generated this message.
 *
 *  \param ... (in) use a printf style string and arguments for your log message.
 */
#define clAppLog(streamHandle, severity, serviceId, area, context, ...) \
do                                                                      \
{                                                                       \
    const ClCharT *pArea    = CL_LOG_AREA_UNSPECIFIED;                        \
    const ClCharT *pContext = CL_LOG_AREA_UNSPECIFIED;                        \
    if( NULL != area )                                                  \
    {                                                                   \
        pArea = area;                                                   \
    }                                                                   \
    if( NULL != context )                                               \
    {                                                                   \
        pContext = context;                                             \
    }                                                                   \
    clLogMsgWrite(streamHandle, severity, serviceId, pArea, pContext,   \
                  __FILE__, __LINE__, __VA_ARGS__);                     \
} while(0)

/**
 * This macro provides the support to log messages by specifying
 * the severity of log message and server information like the 
 * sub-component aera and the context of logging, here user can provide 
 * multiline messages (separate your lines with \\n).
 *
 * This macro is for ASP components only, since it directs all logs
 * to the OpenClovis system log.
 */
#endif
extern void parseMultiline(ClCharT **ppMsg, const ClCharT *pFmt, ...) CL_PRINTF_FORMAT(2, 3);

#define clLogMultiline(severity, area, context, ...)                \
do {                                                                \
    ClCharT *msg = NULL;                                            \
    ClCharT str[CL_LOG_SLINE_MSG_LEN+10] = {0};                     \
    ClCharT *pTemp    = NULL;                                       \
    ClCharT *pStr     = NULL;                                       \
    ClUint32T length  = 0;                                          \
    parseMultiline(&msg, __VA_ARGS__);                              \
    if(msg) {                                                       \
        pStr = msg;                                                 \
        while( pStr ) {                                             \
            pTemp = pStr;                                           \
            pTemp = strchr(pTemp, '\n');                            \
            if( NULL == pTemp ) {                                   \
                if(*pStr) {                                         \
                    snprintf(str, CL_LOG_SLINE_MSG_LEN+3, "%s%s",   \
                        ((pStr==msg) ? "" : "- "), pStr);           \
                    clLog(severity, area, context, str);            \
                }                                                   \
                break;                                              \
            }                                                       \
            length = pTemp - pStr;                                  \
            ++pTemp;                                                \
            if(length > 0 ) {                                       \
                snprintf(str, CL_LOG_SLINE_MSG_LEN+3,"%s%.*s",      \
                   ((pStr==msg) ? "" : "- "), length, pStr);        \
                clLog(severity, area, context, str);                \
            }                                                       \
            pStr += (pTemp - pStr);                                 \
        }                                                           \
        clHeapFree(msg);                                            \
    }                                                               \
} while(0)


/*
 * Macros to log messages at different levels of severity
 */
    
#define clLogEmergency(area, context, ...) \
        clLog(CL_LOG_SEV_EMERGENCY, area, context, __VA_ARGS__)

#define clLogConsoleEmergency(area, context, ...) \
        clLogConsole(CL_LOG_SEV_EMERGENCY, area, context, __VA_ARGS__)

#define clLogAlert(area, context, ...) \
        clLog(CL_LOG_SEV_ALERT, area, context, __VA_ARGS__)

#define clLogConsoleAlert(area, context, ...) \
        clLogConsole(CL_LOG_SEV_ALERT, area, context, __VA_ARGS__)

#define clLogCritical(area, context, ...) \
        clLog(CL_LOG_SEV_CRITICAL, area, context, __VA_ARGS__)

#define clLogConsoleCritical(area, context, ...) \
        clLogConsole(CL_LOG_SEV_CRITICAL, area, context, __VA_ARGS__)

#define clLogError(area, context, ...) \
        clLog(CL_LOG_SEV_ERROR, area, context, __VA_ARGS__)

#define clLogConsoleError(area, context, ...) \
        clLogConsole(CL_LOG_SEV_ERROR, area, context, __VA_ARGS__)
        
#define clLogWarning(area, context, ...) \
        clLog(CL_LOG_SEV_WARNING, area, context, __VA_ARGS__)

#define clLogConsoleWarning(area, context, ...) \
        clLogConsole(CL_LOG_SEV_WARNING, area, context, __VA_ARGS__)

#define clLogNotice(area, context, ...) \
        clLog(CL_LOG_SEV_NOTICE, area, context, __VA_ARGS__)

#define clLogConsoleNotice(area, context, ...) \
        clLogConsole(CL_LOG_SEV_NOTICE, area, context, __VA_ARGS__)
            
#define clLogInfo(area, context, ...) \
        clLog(CL_LOG_SEV_INFO, area, context, __VA_ARGS__)

#define clLogConsoleInfo(area, context, ...) \
        clLogConsole(CL_LOG_SEV_INFO, area, context, __VA_ARGS__)

#define clLogDebug(area, context, ...) \
        clLog(CL_LOG_SEV_DEBUG, area, context, __VA_ARGS__)

#define clLogConsoleDebug(area, context, ...) \
        clLogConsole(CL_LOG_SEV_DEBUG, area, context, __VA_ARGS__)

#define clLogTrace(area, context, ...) \
        clLog(CL_LOG_SEV_TRACE, area, context, __VA_ARGS__)

#define clLogConsoleTrace(area, context, ...) \
        clLogConsole(CL_LOG_SEV_TRACE, area, context, __VA_ARGS__)

#ifdef __cplusplus
    }
#endif
#endif

/** 
 * \} 
 */
