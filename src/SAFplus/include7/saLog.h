/*******************************************************************************
**
** FILE:
**   saLog.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Logging Service (LOG). It contains all of 
**   the prototypes and type definitions required for LOG. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-LOG-A.02.01
**
** DATE: 
**   Wed Oct 08 2008
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**
** Copyright 2008 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, modify, and distribute this software for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies of any software which is or includes a copy
** or modification of this software and in all copies of the supporting
** documentation for such software.
**
** THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
** WARRANTY.  IN PARTICULAR, THE SERVICE AVAILABILITY FORUM DOES NOT MAKE ANY
** REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
** OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
**
*******************************************************************************/

#ifndef _SA_LOG_H
#define _SA_LOG_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaLogHandleT;
typedef SaUint64T SaLogStreamHandleT;

/* well known cluster wide log streams */
#define SA_LOG_STREAM_SYSTEM            "safLgStr=saLogSystem"
#define SA_LOG_STREAM_NOTIFICATION      "safLgStr=saLogNotification"
#define SA_LOG_STREAM_ALARM             "safLgStr=saLogAlarm"


/* The Log severity level FLAGS */
#define SA_LOG_SEV_EMERGENCY  0
#define SA_LOG_SEV_ALERT      1
#define SA_LOG_SEV_CRITICAL   2
#define SA_LOG_SEV_ERROR      3
#define SA_LOG_SEV_WARNING    4
#define SA_LOG_SEV_NOTICE     5
#define SA_LOG_SEV_INFO       6

typedef SaUint16T SaLogSeverityT;

#define SA_LOG_SEV_FLAG_EMERGENCY 0x0001
#define SA_LOG_SEV_FLAG_ALERT 0x0002
#define SA_LOG_SEV_FLAG_CRITICAL 0x0004
#define SA_LOG_SEV_FLAG_ERROR 0x0008
#define SA_LOG_SEV_FLAG_WARNING 0x0010
#define SA_LOG_SEV_FLAG_NOTICE 0x0020
#define SA_LOG_SEV_FLAG_INFO 0x0040

typedef SaUint16T SaLogSeverityFlagsT;

typedef struct {
    SaSizeT   logBufSize;
    SaUint8T *logBuf;
} SaLogBufferT;

typedef SaUint32T SaLogAckFlagsT;

/* 
 * Indicates if a response to an asynch 
 * log-write is desired.
 */

#define SA_LOG_RECORD_WRITE_ACK   0x1 

typedef SaUint8T  SaLogStreamOpenFlagsT;

/* Application Log Stream creation attribute */

#define SA_LOG_STREAM_CREATE      0x1

/* 
 * SAF Notification Service abstractions are used in the SAF Log service
 * APIs. These data types are resolved by including saNtf.h
 */

#include "saNtf.h"

/* Notification Identifiers for a Log Service produced Notifications */

typedef enum {
    SA_LOG_NTF_LOGFILE_PERCENT_FULL = 1
} SaLogNtfIdentifiersT;
 
/* Log Service Object change notifications deliver these attributes */
typedef enum {
    SA_LOG_NTF_ATTR_LOG_STREAM_NAME = 1,
    SA_LOG_NTF_ATTR_LOGFILE_NAME = 2,
    SA_LOG_NTF_ATTR_LOGFILE_PATH_NAME = 3
} SaLogNtfAttributesT;      

/* There are two types of Log Record Headers */

typedef enum {
    SA_LOG_NTF_HEADER      = 1,
    SA_LOG_GENERIC_HEADER  = 2
} SaLogHeaderTypeT;

/* 
 * This structure represents the fields specific to a notification/alarm
 * log header. The NTF service is the principal user of this kind of logging.
 */

typedef struct {
    SaNtfIdentifierT  notificationId;
    SaNtfEventTypeT  *eventType;
    SaNameT          *notificationObject;
    SaNameT          *notifyingObject;
    SaNtfClassIdT    *notificationClassId;
    SaTimeT           eventTime;
} SaLogNtfLogHeaderT;

/* 
 * This structure illustrates the fields that are specific to the
 * system/application log header.
 */   

typedef struct {
    SaNtfClassIdT        *notificationClassId; /* For Internationalization */
    const SaNameT        *logSvcUsrName;
    SaLogSeverityFlagsT   logSeverity;
} SaLogGenericLogHeaderT;

/* the union of all possible LogHeader types */

typedef union  {
    SaLogNtfLogHeaderT       ntfHdr;
    SaLogGenericLogHeaderT   genericHdr;
} SaLogHeaderT;

/* the data points of a log record at SaLogWriteLog() time */

typedef struct {
    SaTimeT          logTimeStamp; /* SA_TIME_UNKNOWN shall be used if 
                                    * time-stamp can't be determined. This 
                                    * means the Log Svc shall supply the 
                                    * time-stamp.
                                    */
    SaLogHeaderTypeT logHdrType;
    SaLogHeaderT     logHeader;
    SaLogBufferT    *logBuffer;
} SaLogRecordT;

/* Log file full actions */

typedef enum {
    SA_LOG_FILE_FULL_ACTION_WRAP =   1,
    SA_LOG_FILE_FULL_ACTION_HALT =   2,
    SA_LOG_FILE_FULL_ACTION_ROTATE = 3
} SaLogFileFullActionT;

/* Log file creation attributes */
#ifdef SA_LOG_A01
typedef struct {
    SaStringT *logFileName;
    SaStringT *logFilePathName;
    SaUint64T maxLogFileSize;
    SaUint32T maxLogRecordSize;
    SaBoolT haProperty;
    SaLogFileFullActionT logFileFullAction;
    SaUint16T maxFilesRotated;
    SaStringT *logFileFmt;
} SaLogFileCreateAttributesT;
#endif /* SA_LOG_A01 */

typedef struct {
    SaStringT logFileName;
    SaStringT logFilePathName;
    SaUint64T maxLogFileSize;
    SaUint32T maxLogRecordSize;
    SaBoolT haProperty;
    SaLogFileFullActionT logFileFullAction;
    SaUint16T maxFilesRotated;
    SaStringT logFileFmt;
} SaLogFileCreateAttributesT_2;


/* 
 * This callback is invoked by the Log Svc to convey Log filter 
 * information to the Log users. The filter information applies to
 * the particular stream, though it is only legal on the system or
 * one of the application streams.
 */

typedef void 
(*SaLogFilterSetCallbackT)(
    SaLogStreamHandleT  logStreamHandle,
    SaLogSeverityFlagsT logSeverity);

typedef void 
(*SaLogStreamOpenCallbackT)(
    SaInvocationT invocation,
    SaLogStreamHandleT logStreamHandle,
    SaAisErrorT error);

/* 
 * This callback is invoked by the Log Svc in case the asynch 
 * version of the log write function has been invoked to write
 * a log record.
 */

typedef void 
(*SaLogWriteLogCallbackT) (
    SaInvocationT invocation,
    SaAisErrorT error);

typedef struct {
    SaLogFilterSetCallbackT  saLogFilterSetCallback;
    SaLogStreamOpenCallbackT saLogStreamOpenCallback;
    SaLogWriteLogCallbackT   saLogWriteLogCallback;
} SaLogCallbacksT;

typedef enum {
    SA_LOG_MAX_NUM_CLUSTER_APP_LOG_STREAMS_ID = 1
} SaLogLimitIdT;

/*************************************************/
/******** LOG API function declarations **********/
/*************************************************/

extern SaAisErrorT 
saLogInitialize(
    SaLogHandleT *logHandle,
    const SaLogCallbacksT *callbacks,
    SaVersionT *version);

extern SaAisErrorT 
saLogSelectionObjectGet(
    SaLogHandleT logHandle,
    SaSelectionObjectT *selectionObject);

extern SaAisErrorT 
saLogDispatch(
    SaLogHandleT logHandle, 
    SaDispatchFlagsT dispatchFlags);

extern SaAisErrorT 
saLogFinalize(
    SaLogHandleT logHandle);

#ifdef SA_LOG_A01
extern SaAisErrorT 
saLogStreamOpen(
    SaLogHandleT logHandle,
    SaNameT logStreamName,
    SaLogFileCreateAttributesT *logFileCreateAttributes,
    SaLogStreamOpenFlagsT logStreamOpenFlags,
    SaTimeT timeout,
    SaLogStreamHandleT *logStreamHandle);

extern SaAisErrorT 
saLogStreamOpenAsync(
    SaLogHandleT logHandle,
    SaNameT logStreamName,
    SaLogFileCreateAttributesT *logFileCreateAttributes,
    SaLogStreamOpenFlagsT logStreamOpenFlags,
    SaInvocationT invocation);
#endif /* SA_LOG_A01 */

extern SaAisErrorT
saLogStreamOpen_2(
    SaLogHandleT logHandle,
    const SaNameT *logStreamName,
    const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
    SaLogStreamOpenFlagsT logStreamOpenFlags,
    SaTimeT timeout,
    SaLogStreamHandleT *logStreamHandle);

extern SaAisErrorT
saLogStreamOpenAsync_2(
    SaLogHandleT logHandle,
    const SaNameT *logStreamName,
    const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
    SaLogStreamOpenFlagsT logStreamOpenFlags,
    SaInvocationT invocation);

extern SaAisErrorT 
saLogWriteLog(
    SaLogStreamHandleT logStreamHandle,
    SaTimeT timeout,
    const SaLogRecordT *logRecord);

extern SaAisErrorT 
saLogWriteLogAsync(
    SaLogStreamHandleT logStreamHandle,
    SaInvocationT invocation,
    SaLogAckFlagsT ackFlags,
    const SaLogRecordT *logRecord);

extern SaAisErrorT
saLogStreamClose(
    SaLogStreamHandleT logStreamHandle);

extern SaAisErrorT 
saLogLimitGet(
    SaLogHandleT logHandle,
    SaLogLimitIdT limitId,
    SaLimitValueT *limitValue);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_LOG_H */

