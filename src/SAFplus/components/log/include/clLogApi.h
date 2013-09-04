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
 * \brief Header file of Log Service related APIs
 * \ingroup log_apis
 */

/**
 * \addtogroup log_apis
 * \{
 */


#ifndef _CL_LOG_API_H_
#define _CL_LOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h> /* For getpid() */
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <clTimerApi.h>
#include <clCommon.h>
#include <clDbg.h>
#include <clLogUtilApi.h>

/*********************************
    Data Types
*********************************/ 
 
/**
 * The type of handle supplied by Log Service during initialization.
 */
typedef ClHandleT  ClLogHandleT;

/**
 * The type of handle supplied by Log Service during log streamOpen and
 * a process register itself as stream handler for the stream.
 */
typedef ClHandleT  ClLogStreamHandleT;

/**
 * The type of handle supplied by Log Service to a process 
 * who has opened a file for consuming log records.
 */
typedef ClHandleT  ClLogFileHandleT;

/**
 * Maximum length of stream Name.
 */
#define CL_LOG_STREAM_NAME_MAX_LENGTH  128
/**
 * Maximum Single line message length
 */
#define CL_LOG_SLINE_MSG_LEN  256


/**
 * This enumeration is used to specify the scope of the stream.
 * A stream can be local to the node(Local log stream) or global 
 * to the cluster(Global log stream).
 */
typedef enum
{
    /**
     * Flags specifies the streams global to the node. 
     */
    CL_LOG_STREAM_GLOBAL = 0,
    /**
     * Flag specifies the streams local to the node.
     */
    CL_LOG_STREAM_LOCAL  = 1
}ClLogStreamScopeT;

/**
 * This enumeration is used to specify the behavior of Log Service once the Log
 * File into which this Log Stream is going becomes full.
 */
typedef enum
{
 /**
  * It directs the Log Service to create a new Log File Unit when the current 
  * Log File Unit becomes full. The number of maximum Log File Units that can 
  * simultaneously exist is limited by maxFilesRotated attribute of the Log Stream.
  * Once this limit is reached, the oldest Log File Unit is deleted and a new one is
  * created. 
  */
  CL_LOG_FILE_FULL_ACTION_ROTATE = 0,
 /**
  * It makes the Log Service treat the Log File as a circular buffer, i.e., when the Log
  * File becomes full, Log Service starts overwriting oldest records.
  */
  CL_LOG_FILE_FULL_ACTION_WRAP   = 1,
  /**
   * Log Service stops putting more records in the Log File once it becomes full.
   */
  CL_LOG_FILE_FULL_ACTION_HALT   = 2
} ClLogFileFullActionT;

/**
 * This structure describes the attributes of the stream.
 */
typedef struct
{
/**
 * Its the prefix name of file units that are going to be created. 
 */
    ClCharT               *fileName;
/**
 * Its the path where the log file unit(s) will be created.  The path can either be absolute (beginning with a /) or relative. If relative it will be placed in the specified directory under the ASP install directory. Additionally, this field must start with either a period (.) or an asterisk (*) followed by a colon (:). A period indicates that the log file should be written to the local machine. An asterisk indicates that the file should be written to the system controller.
 */
    ClCharT               *fileLocation;
/**
 * Size of the file unit. It will be truncated to multiples of recordSize.
 */
    ClUint32T             fileUnitSize;
/**
 * Size of the log record. 
 */
    ClUint32T             recordSize;
/**
 * Log file replication property. Currently is not supported.
 */
    ClBoolT               haProperty;
/**
 * Action that log service has to take, when the log file unit becomes full.
 */
    ClLogFileFullActionT  fileFullAction;
/**
 * If fileAction is CL_LOG_FILE_FULL_ACTION_ROTATE, the maximum num of log
 * file units that will be created by logService.Otherwise ignored.
 */
    ClUint32T             maxFilesRotated;
/**
 * Num of log records after which the log stream records must be flushed.
 */
    ClUint32T             flushFreq;
/**
 * Time after which the log stream records must be flushed. Denoted in nanoseconds.
 */
    ClTimeT               flushInterval;
/**
 * The water mark for file units.When the size of file reaches this level,
 * the water mark event will be published.
 */
    ClWaterMarkT          waterMark;

    /*
     * Syslog enabler for the stream.
     */
    ClBoolT              syslog;
} ClLogStreamAttributesT;

/**
 * This flag specifies the log service that the particular stream should be 
 * created, if it does not exist already in the cluster.
 */
#define CL_LOG_STREAM_CREATE  0x1

/**
 * This flags to specify the stream should be created or opened.
 */
typedef ClUint8T  ClLogStreamOpenFlagsT;

/**
 * This structure describes all the information about the log stream.
 */
typedef struct
{
/**
 * Name of the log stream.
 */
    ClNameT                 streamName;
/**
 * Scope of the log stream.
 */
    ClLogStreamScopeT       streamScope;
/**
 * The name of the node on which the stream exist.
 */
    ClNameT                 streamScopeNode;
/**
 * The unique id for the stream in the cluster. 
 */
    ClUint16T               streamId;
/**
 * Attributes of the log stream. 
 */
    ClLogStreamAttributesT  streamAttr;
} ClLogStreamInfoT;

/**
 * This structure describes the information about the log stream to 
 * stream Id mapping.
 */
typedef struct 
{
/**
 * Name of the log stream. 
 */
    ClNameT           streamName;
/**
 * Scope of the log stream. 
 */
    ClLogStreamScopeT streamScope;
/**
 * Node name on which the stream exist. 
 */
    ClNameT           nodeName;
/**
 * Unique id of the stream in the cluster.
 */
    ClUint16T         streamId;
} ClLogStreamMapT;

/**
 * This flags informs the log service that the stream handlers will 
 * acknowledge for all the records, it has received.
 */
#define CL_LOG_HANDLER_WILL_ACK  0x1

/**
 * While registering as strean handler, the process should specify 
 * this flag.
 */
typedef ClUint8T  ClLogStreamHandlerFlagsT;

typedef enum
  {
/**
 * setting severity as EMERGENCY.
 */
    CL_LOG_SEV_EMERGENCY = 0x1,
/**
 * setting severity as ALERT. 
 */
    CL_LOG_SEV_ALERT,
/**
 * setting severity as CRITICAL. 
 */
    CL_LOG_SEV_CRITICAL,
/**
 * setting severity as ERROR. 
 */
    CL_LOG_SEV_ERROR,
/**
 * setting severity as WARNING. 
 */
    CL_LOG_SEV_WARNING,
/**
 * setting severity as NOTICE. 
 */
    CL_LOG_SEV_NOTICE,
/**
 * setting severity as INFORMATION.
 */
    CL_LOG_SEV_INFO,
/**
 * setting severity as DEBUG.
 */
    CL_LOG_SEV_DEBUG,
    CL_LOG_SEV_DEBUG1   =      CL_LOG_SEV_DEBUG,
    CL_LOG_SEV_DEBUG2,
    CL_LOG_SEV_DEBUG3,
    CL_LOG_SEV_DEBUG4,
    CL_LOG_SEV_DEBUG5,
/**
 * setting severity as DEBUG.
 */
    CL_LOG_SEV_TRACE  =    CL_LOG_SEV_DEBUG5,
    CL_LOG_SEV_DEBUG6,
    CL_LOG_SEV_DEBUG7,
    CL_LOG_SEV_DEBUG8,
    CL_LOG_SEV_DEBUG9,
/**
 * Maximum severity level. 
 */
  CL_LOG_SEV_MAX    =    CL_LOG_SEV_DEBUG9
  } ClLogSeverityT;

#if 0
/**
 * setting severity as EMERGENCY.
 */
#define CL_LOG_SEV_EMERGENCY  0x1
/**
 * setting severity as ALERT. 
 */
#define CL_LOG_SEV_ALERT      0x2
/**
 * setting severity as CRITICAL. 
 */
#define CL_LOG_SEV_CRITICAL   0x3
/**
 * setting severity as ERROR. 
 */
#define CL_LOG_SEV_ERROR      0x4
/**
 * setting severity as WARNING. 
 */
#define CL_LOG_SEV_WARNING    0x5
/**
 * setting severity as NOTICE. 
 */
#define CL_LOG_SEV_NOTICE     0x6
/**
 * setting severity as INFORMATION.
 */
#define CL_LOG_SEV_INFO       0x7
/**
 * setting severity as DEBUG.
 */
#define CL_LOG_SEV_DEBUG      0x8
/**
 * setting severity as DEBUG.
 */
#define CL_LOG_SEV_TRACE      CL_LOG_TRACE
/**
 * Maximum severity level. 
 */
#define CL_LOG_SEV_MAX        CL_LOG_DEBUG9
#endif

/**
 * Variables of this type is used as a bitmap. Values from 
 * ClLogSeverityT are used to set the individual bits in the
 * bitmap.
 */
typedef ClUint16T  ClLogSeverityFilterT;

/**
 * Discard the old filter settings and use new settings provided. 
 */
#define CL_LOG_FILTER_ASSIGN        0x1

/**
 * Add to the old filter settings as per new filter settings provided.
 */
#define CL_LOG_FILTER_MERGE_ADD     0x2

/**
 * Delete to the old filter settings as per new filter settings provided.
 */
#define CL_LOG_FILTER_MERGE_DELETE  0x3

/**
 * It takes the values of above specifed flags. 
 */
typedef ClUint8T  ClLogFilterFlagsT;


/**
 * This structure describes the filter settings for the stream.
 */
typedef struct
{
/**
 * This field identifies the severity level.  
 */
    ClLogSeverityFilterT  severityFilter;
/**
 * The size of the memory pointed by pMsgIdSet in bytes. 
 */
    ClUint16T             msgIdSetLength;
/**
 * This memory will be treated as bitmap to mask the message ids
 */
    ClUint8T              *pMsgIdSet;
/**
 * The size of the memory pointed by pCompIdSet in bytes. 
 */
    ClUint16T             compIdSetLength;
/**
 * This memory will be treated as bitmap to mask the component ids
 */
    ClUint8T              *pCompIdSet;
} ClLogFilterT;

/**
 ************************************
 *  \brief This function gets called When clLogStreamOpenAsync() call 
 *   returns on the server. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param invocation (in) invocation value of the call. This is basically an
 *  identification about the call.
 *
 *  \param hStream (in) contains handle of the opened stream, if the
 *  clLogStreamOpenAsync() was succesfull. 
 *
 *  \param rc (in) return value of the clLogStreamOpenAsync() call.
 *
 *  \retval  CL_OK The Log stream is opened successfully. 
 *
 *  \retval  CL_ERR_NOT_EXIST \c CL_LOG_STREAM_CREATE flag is not set in streamOpenFlags and
 *  the Stream does not exist.
 *
 *  \retval CL_ERR_ALREADY_EXIST \c CL_LOG_STREAM_CREATE flag is set in streamOpenFlags but
 *  the Stream already exists and was originally created with different
 *  attributes than specified by pStreamAttributes.
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This streamOpen callback function is getting called, when
 *  clLogStreamOpenAsync() call returns from the server. It carries the  
 *  invocation to identify the call and return code for indicating the status
 *  of the call. If the retcode is CL_OK, then hStream will be carrying
 *  the proper handle of the stream which was opened on the particular
 *  invocation.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogOpenStreamAsync()
 *
 */
typedef void (*ClLogStreamOpenCallbackT)(
          CL_IN  ClInvocationT       invocation,
          CL_IN  ClLogStreamHandleT  hStream,
          CL_IN  ClRcT               rc);

/**
 ************************************
 *  \brief Informs the logger about a change in filter settings of a Log
 *  stream opened by this logger. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hStream (in) contains handle of the stream for which the filter
 *  settings have been updated.
 *
 *  \param filter (in) New filter settings of the stream. 
 *
 *  \retval  none 
 *
 *  \par Description:
 *  The Log Service invokes this function when the filter associated with a Log
 *  Stream identified by hStream is changed either by a Logger or north-bound
 *  interface. The Log Stream must be currently opened by this Logger. If either
 *  pLogCallbacks parameter to clLogInitialize call was NULL or
 *  clLogFilterSetCallback of that parameter was NULL, the Logger will not be
 *  informed about change in filter settings.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogFilterSet()
 *
 */
typedef void (*ClLogFilterSetCallbackT)(
              CL_IN  ClLogStreamHandleT  hStream,
              CL_IN  ClLogFilterT        filter);

/**
 ************************************
 *  \brief Callback function to receive log records of Log streams of
 *  interest. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hStream (in) Handle obtained through a previous invocation to
 *  clLogHandlerRegister(). This handle identifies the registration of handler
 *  for a stream for which the Log Records are being delivered.
 *
 *  \param sequenceNumber (in): A monotonically increasing no to detect the duplicate
 *  delivery of a set of Log Records. It includes the node identifier and the
 *  starting sequence number of the records.
 *
 *  \param numRecords (in): Number of records in the buffer pointer by logRecords.
 *
 *  \param pRecords (in): Pointer to a buffer that contains numRecords number of Log
 *   Records from a stream identified by hStream.
 *
 *  \param filter (in) New filter settings of the stream. 
 *
 *  \retval  none 
 *
 *  \par Description:
 *   This callback delivers a set of new Log Records to the handlers. These
 *   records will no more be available in the Log Stream.
 *   This callback may be invoked multiple times for the same set of records,
 *   which can be detected by the same sequenceNumber.
 *   Memory for pRecords is allocated by the Log Service and must be freed by the
 *   Handler.
 *
 *  \par Library File:
 *   ClLogClient
 *
 *  \sa clLogFilterSet()
 *
 */
typedef void (*ClLogRecordDeliveryCallbackT)(
              CL_IN  ClLogStreamHandleT  hStream,
              CL_IN  ClUint64T           seqNum,
              CL_IN  ClUint32T           numRecords,
              CL_IN  ClPtrT              pRecords);

/**
 * This structure describes about the callbacks which can be provided by
 * process while initializing with Log Service.
 */
typedef struct
{
/**
 * Callback for stream open. 
 */
    ClLogStreamOpenCallbackT      clLogStreamOpenCb;
/**
 * Callback for informing filter settings updation. 
 */
    ClLogFilterSetCallbackT       clLogFilterSetCb;
/**
 * Callback for delivering records to stream handlers. 
 */
    ClLogRecordDeliveryCallbackT  clLogRecordDeliveryCb;
/**
 * Callback for async write.Currently ignored. 
 */
#if 0
    ClLogWriteCallbackT           clLogWriteCallback;
#endif
} ClLogCallbacksT;


/**
 * Logging messgae in binary format. 
 */
#define CL_LOG_MSGID_BUFFER      0

/**
 * Logging message in ASCII format. 
 */
#define CL_LOG_MSGID_PRINTF_FMT  1

/**
 * This tag is used for terminating the TLV type.
 */
#define CL_LOG_TAG_TERMINATE       0

/**
 * The following macros will be used while doing TLV log write.
 * The application process can use the following macros while doing TLV log
 * write
 */

/**
 * its basic signed data type.
 */
#define CL_LOG_TAG_BASIC_SIGNED    0x1
/**
 * its basic unsigned data type.
 */
#define CL_LOG_TAG_BASIC_UNSIGNED  0x2
/**
 * its basic string type. 
 */
#define CL_LOG_TAG_STRING    0x3

/**
 * TLV for unsigned byte variable(ClUint8T).
 */
#define CL_LOG_TLV_UINT8(var)                              \
    CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)

/**
 * TLV for signed byte variable(ClInt8T).
 */
#define CL_LOG_TLV_INT8(var)                               \
    CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)

/**
 * TLV for unsigned 16 bit variable(ClUint16T).
 */
#define CL_LOG_TLV_UINT16(var)                             \
    CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)

/**
 * TLV for signed 16 bit varible(ClInt16T).
 */
#define CL_LOG_TLV_INT16(var)                              \
    CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)

/**
 * TLV for unsigned 32 bit varible(ClUint32T).
 */
#define CL_LOG_TLV_UINT32(var)                             \
    CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)

/**
 * TLV for signed 32 bit varible(ClInt32T).
 */
#define CL_LOG_TLV_INT32(var)                              \
    CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)

/**
 * TLV for unsigned 64 bit varible(ClUint64T).
 */
#define CL_LOG_TLV_UINT64(var)                             \
    CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)

/**
 * TLV for signed 64 bit varible(ClInt64T)
 */
#define CL_LOG_TLV_INT64(var)                              \
    CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)

/**
 * TLV for any string. 
 */
#define CL_LOG_TLV_STRING(var)                              \
    CL_LOG_TAG_STRING, (strlen( var ) + 1), (var)


/**
 ************************************
 *  \brief  Initializes the Log service for the calling process and ensures
 *  the version compatability.
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param phLog (out) Handle returned by the Log Service. This handle is used
 *  by the calling process for subsequent invocation LogAPIs. Each invocation of
 *  clLogInitialize() returns the new log handle.
 *
 *  \param pLogCallbacks (in) contains callback functions which can be invoked
 *  on calling process.
 *
 *  \param pVersion (in/out)  As an input parameter, version is a pointer to the required
 *  Log Service version.  As an output parameter, the version actually supported by the
 *  Log Service is delivered.
 *
 *  \retval  CL_OK The Log Service is initialized successfully.
 *
 *  \retval  CL_ERR_VERSION_MISMATCH The supplied version is not supported by current
 *  implementation.
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NULL_POINTER Either phLog or pVersion is passed as NULL.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function initializes the Log Service for the invoking process, performs
 *  version compatibility checks and registers various callbacks provided. This
 *  function must be invoked before any other function of Log Service API. The
 *  handle phLog is returned as the reference to this association of the process
 *  and Log Service. The process uses this handle in subsequent interaction with
 *  Log Service.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogFinalize()
 *
 */
extern ClRcT
clLogInitialize(CL_OUT   ClLogHandleT           *phLog,
                CL_IN    const ClLogCallbacksT  *pLogCallbacks,
                CL_INOUT ClVersionT             *pVersion);

/**
 ************************************
 *  \brief  Finalize the Log service for the calling process and ensures
 *  to release all the resources.
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hLog (in) Handle obtained by the previous invocation
 *  clLogInitialize. This handle identifies the association to be closed
 *  between the calling process and LogService.
 *
 *  \retval  CL_OK The Log Service is finalized successfully.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed handle is either not obtained
 *  through clLogInitialize() or this handle association has been already closed
 *  by another invocation clLogFinalize(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function closes the association between the Log Service and the invoking
 *  process, identified by the handle hLog, and frees up all the resource
 *  acquired by this association. Other functions of Log Service API function
 *  must not be invoked after a successful invocation to this function. Process
 *  must have acquired hLog through a previous successful invocation to
 *  clLogInitialize(). For each successful invocation to clLogInitialize(), the
 *  process must invoke clLogFinalize() before going down gracefully. On
 *  successful completion, this function frees up all the resources acquired by
 *  this association. All opened Log Stream are closed, all registrations for Log
 *  Stream Handlers are de-registered, and all opened Log File handles are
 *  closed. 
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogInitialize()
 *
 */
extern ClRcT clLogFinalize(CL_IN ClLogHandleT  hLog);

/**
 ************************************
 *  \brief Opens the stream for Logging. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hLog (in) Handle obtained by the previous invocation
 *  clLogInitialize. This handle identifies the association
 *  between the calling process and LogService.
 *
 *  \param streamName (in) The name of the Log Stream to be opened 
 *  for logging. It can be one of the pre-defined (modeled) Log Streams
 *  or one being dynamically created.
 *
 *  \param streamScope (in) Scope of the Log Stream. It can have the following
 *  values:
 *  \arg \c CL_LOG_STREAM_GLOBAL
 *  \arg \c CL_LOG_STREAM_LOCAL
 *
 *  \param pStreamAttributes (in) Attributes of the Log Stream to be opened. 
 *  If a pre-defined Log Stream is being opened, then this should be NULL.
 *  If the intent is only to open an existing Log Stream identified by
 *  streamName and streamScope, then this value must be NULL. If the intent 
 *  is to open and create a non-existing Log Stream, then this parameter must
 *  be filled with the attributes of the new Stream and streamOpenFlags must 
 *  be ORed with \c CL_LOG_STREAM_CREATE. If the intent is to open and possibly 
 *  create (if not already created) Stream, then this parameter must be filled 
 *  with the attributes of the new Stream. These attributes must match with 
 *  the attributes of the Stream, if it happened to be already created.
 *
 *  \param streamOpenFlags (in) flags to create/open the stream. 
 *  \c CL_LOG_STREAM_CREATE must be specified if the intention is to open and 
 *  create a non-existent Stream or to open and possibly create (if not
 *  already created) Stream. In case of pre-defined Streams, \c CL_LOG_STREAM_CREATE
 *  must not be specified.
 *
 *  \param timeout (in) This is applicable only for clLogStreamOpen(). If the call does
 *  not complete in this time, the call is considered to have failed. The Log
 *  Stream might have been opened and/or created, but the outcome is
 *  non-deterministic. A value of zero indicates no timeout.
 *
 *  \param Invocation (in) This is applicable only for clLogStreamOpenAsync(). This is
 *  used to co-relate the response received through ClLogStreamOpenCallbackT.
 *
 *  \param phLogStream (out) Pointer to get the Log Service generated handle for this
 *  Log Stream. This handle must be used for subsequent operations on this Log
 *  Stream.
 *
 *  \retval  CL_OK The Log stream is opened successfully. 
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed handle(hLog) is either not obtained
 *  through clLogInitialize() or this handle association has been already closed
 *  by another invocation clLogFinalize(). 
 *
 *  \retval CL_ERR_NULL_POINTER Either pStreamAttributes or phStream is 
 *  passed as NULL.
 *
 *  \retval CL_ERR_INVALID_PARAMETER Some of the parameters passed are not valid.
 *  The conditions are:
 *  \arg \c The streamOpenFlags does not have \c CL_LOG_STREAM_CREATE set and
 *  pStreamAttributes is not NULL.
 *  \arg \c The streamOpenFlags has \c CL_LOG_STREAM_CREATE set and pStreamAttributes
 *  is NULL.
 *  \arg \c Length field of streamName is set to zero.
 *  \arg \c fileLocation member of pStreamAttributes does not follow the pattern 
 *  defined ::ClLogStreamAttributesT. fileName member of pStreamAttributes does 
 *  not follow the pattern defined in ::ClLogStreamAttributesT.
 *  \arg \c fileUnitSize member of pStreamAttributes is the size of the individual file
 *  unit, in bytes. 
 *  \arg \c fileFullAction member of pStreamAttributes does not have a value 
 *  defined in ::ClLogFileFullActionT.
 *  \arg \c Both flushFreq and flushInterval members of pStreamAttributes are set to zero.
 *  \arg \c waterMark member of pStreamAttributes have values outside 0-100 range.
 *  \arg \c streamScope does not have a value defined in ::ClLogStreamScopeT.
 *  \arg \c timeout is specified as a negative value.
 *
 *  \retval  CL_ERR_NOT_EXIST \c CL_LOG_STREAM_CREATE flag is not set in streamOpenFlags and
 *  the Stream does not exist.
 *
 *  \retval  CL_ERR_ALREADY_EXIST \c CL_LOG_STREAM_CREATE flag is set in streamOpenFlags but
 *  the Stream already exists and was originally created with different
 *  attributes than specified by pStreamAttributes.
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function opens a Log Stream for logging. If the Log Stream is not a
 *  pre-defined Stream and application wants to possibly create it, then
 *  pStreamAttributes must be specified and \c CL_LOG_STREAM_CREATE must be set in
 *  streamOpenFlags. If \c CL_LOG_STREAM_CREATE is set in streamOpenFlags, then
 *  pStreamAttributes must be specified, otherwise it must be NULL. Further, if
 *  pStreamAttributes is non-NULL, then at-least one of flushFreq and
 *  flushInterval members of pStreamAttributes must be non-zero.
 *  Invocation to clLogStreamOpen() is blocking. If the Log Stream is
 *  successfully opened, handle to newly opened stream is returned in phStream,
 *  otherwise and error is retured.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogStreamClose(), clLogWriteAsync()
 *
 */
extern ClRcT
clLogStreamOpen(CL_IN  ClLogHandleT            hLog,
                CL_IN  ClNameT                 streamName,
                CL_IN  ClLogStreamScopeT       streamScope,
                CL_IN  ClLogStreamAttributesT  *pStreamAttr,
                CL_IN  ClLogStreamOpenFlagsT   streamOpenFlags,
                CL_IN  ClTimeT                 timeout,
                CL_OUT ClLogStreamHandleT      *phStream);
/**
 ************************************
 *  \brief Close the stream opened for logging. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hStream (in) Handle obtained by the previous invocation
 *  clLogStreamOpen(). This handle identifies the log stream to be closed. 
 *
 *  \retval  CL_OK The Log Stream is closed successfully.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log Stream handle is not valid. 
 *  Either it is not received through a previous invocation to clLogStreamOpen() 
 *  or it has already been closed through an invocation to clLogStreamClose(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function closes a Log Stream identified by \e hStream. \e hStream must have
 *  been obtained through a previous invocation of either clLogStreamOpen() 
 *  or clLogStreamOpenAsync(). After successful completion of this function or a
 *  failure with return value CL_ERR_TIMEOUT, hStream is no longer valid and must
 *  not be used for any other stream related operations. If hStream represents a
 *  Log Stream which is not pre-defined and that Log Stream is not opened by any
 *  process in the cluster, the Log Stream is deleted. When all the Log Streams
 *  being persisted in the same Log File are deleted, the Log File is closed.
 *  Closing a Log Stream releases all the resources allocated by Log Service to
 *  this instance of opening. If a process terminates without closing Log Streams
 *  opened by it, Log Service implicitly closes all such Log Streams.
 *  On successful completion of this call, all pending callbacks referring to
 *  this hStream are cancelled. Since the invocation of callbacks is an
 *  asynchronous operation, some callbacks may still be delivered after this call
 *  returns successfully.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogStreamOpen()
 *
 */
extern ClRcT
clLogStreamClose(CL_IN  ClLogStreamHandleT hStream);

/**
 ************************************
 *  \brief Logs a Log Record in the specified Log Stream. 
 *
 *  \par Header File:
 *  clLogApi.h, clLogErrors.h
 *
 *  \param hStream (in) Handle obtained by the previous invocation
 *  clLogStreamOpen(). This handle identifies the Log Stream on which the log
 *  record to be placed.
 *
 *  \param severity (in) This field must be set to one of the values defined. 
 *  It defines the severity level of the Log Record being written.
 *
 *  \param serviceId (in) This field identifies the module within the process
 *  which is generating this Log Record. If the Log Record message is a generic
 *  one like out of memory, this field can be used to narrow down on the module 
 *  impacted. For ASP client libraries, these values are defined in clCommon.h.
 *  For application modules, it is up-to the application developer to define the
 *  values and scope of those values.
 *
 *  \param msgId (in) This field identifies the actual message to be Logged. 
 *  This is typically an identifier for a string message which the viewer is aware
 *  of through off-line mechanism. Rest of the arguments of this function are
 *  interpreted by the viewer based on this identifier. For application Log
 *  Streams, the values and scope of each value is defined by the application
 *  developer. Following two values are pre-defined:
 *  \arg \c CL_LOG_MSGID_BUFFER   
 *  \arg \c CL_LOG_MSGID_PRINTF_FMT
 *  
 *  In case, msgId is passed as CL_LOG_MSGID_BUFFER, it is followed by two
 *  parameters:
 *  \arg \c First one is of type ClUint32T. It is the number of bytes in the buffer
 *  pointed by the second parameter.
 *  \arg \c Second one is a pointer to a buffer. It is of type ClPtrT. The buffer may
 *  contain binary or ASCII data. The number of bytes of useful data is indicated
 *  by previous parameter. In case the buffer contains ASCII data, if the buffer
 *  is NULL terminated, the length must include the NULL termination byte. This
 *  buffer is not freed by the Log Service.
 *  In case, msgId is passed as \c CL_LOG_MSGID_PRINTF_FMT, the next argument is
 *  treated as format string of printf (3). Rest of the arguments are interpreted
 *  as per this format string.
 *  For other values of msgId, rest of arguments are treated as a set of 3-tuples.
 *  Each of these tuple is of the form <Tag, Length, Value Pointer>. The tuple of
 *  3 arguments is interpreted as follows
 *  First one is of type ClUint16T. It is treated as the tag to identify contents
 *  of third argument in this tuple.
 *  Second one is of type ClUint16T. It is treated as number of bytes in the
 *  buffer pointed by third argument.
 *  Third one is of type ::ClPtrT. The buffer may contain binary or ASCII data,
 *  which is defined by the tag (first argument in this tuple). The Log Service
 *  does not interpret this buffer. Only the Log Consumer interprets it and must
 *  be aware of the semantic meaning of its contents. It simply copies this buffer
 *  in the Log Record. In case of ASCII data, if the buffer is NULL terminated,
 *  the length must include the NULL termination byte. This buffer is not freed by
 *  the Log Service.
 *  End of this set of tuple is indicated by a special tag \c CL_LOG_TAG_TERMINATE.
 *  Thus, the variable part of this argument list will always have 3*n + 1, 
 *  where n is the number of parameters to be logged. Following tag values are 
 *  defined by Log Service, rests are defined by the application.
 *  \arg \c CL_LOG_TAG_TERMINATE        
 *  \arg \c CL_LOG_TAG_BASIC_SIGNED    
 *  \arg \c CL_LOG_TAG_BASIC_UNSIGNED 
 *  \arg \c CL_LOG_TAG_STRING        
 *  Log Service also defines following macros to ease the use of this function.
 *  \arg \c CL_LOG_TLV_UINT8(var)             
 *  \arg \c CL_LOG_TLV_INT8(var)               
 *  \arg \c CL_LOG_TLV_UINT16(var)           
 *  \arg \c CL_LOG_TLV_INT16(var)           
 *  \arg \c CL_LOG_TLV_UINT32(var)             
 *  \arg \c CL_LOG_TLV_INT32(var)             
 *  \arg \c CL_LOG_TLV_UINT64(var)          
 *  \arg \c CL_LOG_TLV_INT64(var)          
 *  \arg \c CL_LOG_TLV_STRING(var)        
 *
 *  \retval  CL_OK The Log Record is recorded successfully. But this does not
 *  mean that the record has been persisted in the Log File.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log Stream handle is not valid. 
 *  Either it is not received through a previous invocation to clLogStreamOpen() 
 *  or it has already been closed through an invocation to clLogStreamClose(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \retval  CL_LOG_ERR_FILE_FULL Log Stream identified by \e hStream was created with
 *  fileFullAction attribute set to CL_LOG_FILE_FULL_ACTION_HALT and the Log File
 *  has become full. Thus no more Log Records can be logged into this stream. The
 *  Stream must be closed.
 *
 *  \par Description:
 *     This function puts a Log Record in the Log Stream identified by hStream.
 *  An invocation to this function is non-blocking. When this function returns,
 *  it is guaranteed that the record has been recorded into Log Stream, but the
 *  record might not be persisted in the Log File. Timing of persistence of this
 *  Log Record in the Log File depends on the flushFreq and flushInterval
 *  attributes of the Log Stream.
 *  \par 
 *      This function accepts variable number of arguments. Actual number of
 *  arguments and their types depend on msgId parameter. If the value of msgId is
 *  CL_LOG_MSGID_BUFFER, then two more arguments are expected which are length of
 *  buffer and pointer to the buffer. If the value of msgId is CL_LOG_MSGID_PRINTF_FMT,
 *  then next argument is treated as a C printf style format string and rest of the
 *  arguments are interpreted as per the format string. 
 *  For all other values, the variable number of arguments are treated as
 *  a set of 3-tuples. They should be 3*n+1, where n is the number of parameters
 *  to be logged along with the msgId. For each such parameter, a tag identifying
 *  the type of the parameter, a length denoting number of bytes in the parameter
 *  and a pointer to the parameter are passed. The last argument must be a
 *  special tag CL_LOG_TAG_TERMINATE. Values of msgId and tags, other than
 *  defined by Log Service are not interpreted and the data is just copied into
 *  the Log Record. It is the responsibility of Log Consumer to get the semantic
 *  meaning of these and interpret the Log Record properly.
 *  \par 
 *     Certain other information like Log Timestamp and Component Id are also
 *  recorded in the Log Record by the Log Service. Log Timestamp is the wall
 *  clock time at the time of invocation to this function on the host where the
 *  Logger is running. Component Id is the unique identifier identifying this
 *  instance of the application. This is issued by Component Manager and remains
 *  unchanged across process restart or cluster restart.The Log Record is written
 *  in the Log Stream in an atomic fashion. Thus,concurrent recording by multiple
 *  threads of the same process or multiple processes in the cluster is properly 
 *  handled.None of the buffers passed to this function are freed by this function.
 *  It is the responsibility of the Logger to free those buffers. Thus, pointers to
 *  stack variables can also be passed as pointers to this function without any
 *  adverse side-effect.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogStreamOpen(), clLogStreamClose()
 *
 */
extern ClRcT
clLogWriteAsync(ClLogStreamHandleT   hStream,
                ClLogSeverityT       severity,
                ClUint16T            serviceId,
                ClUint16T            msgId,
                ...);

extern ClRcT
clLogWriteAsyncWithHeader(ClLogStreamHandleT   hStream,
                          ClLogSeverityT       severity,
                          ClUint16T            serviceId,
                          ClUint16T            msgId,
                          ...);

extern ClRcT
clLogWriteAsyncWithContextHeader(ClLogStreamHandleT   hStream,
                                 ClLogSeverityT       severity,
                                 const ClCharT        *pArea,
                                 const ClCharT        *pContext,
                                 ClUint16T            serviceId,
                                 ClUint16T            msgId,
                                 ...);

/**
 **********************************************************************
 *  \brief Changes the filter settings of a Log Stream.
 *
 *  \par Header File:
 *  clLogApi.h, clLogErrors.h
 *
 *  \param hStream (in) Handle obtained by the previous invocation
 *  clLogStreamOpen(). This handle identifies the Log Stream whose filter
 *  setting have to be changed.
 *
 *  \param logFilterFlags (in) This parameter identifies the way in which 
 *  filter parameter should be used. This filter can overwrite any previous 
 *  filter set or it can be used to modify a previous filter. To clear a 
 *  previous filter, this field should be set to CL_LOG_FILTER_ASSIGN and all
 *  other fields should be set to zero.
 *
 *  \param filter (in) Filter settings to be applied.
 *
 *  \retval  CL_OK Changes in the filter settings have been done successfully.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log Stream handle is not valid. 
 *  Either it is not received through a previous invocation to clLogStreamOpen() 
 *  or it has already been closed through an invocation to clLogStreamClose(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function sets the filter on the stream identified by \e hStream. This
 *  filter can be used to overwrite or clear or modify a previously set filter.
 *  The logFilterFlags parameter is used to identify the correct operation. The
 *  logSeverityFilter field of filter parameter is used to set filter setting
 *  based on severity of the Log Record. All Log Records with severity bits set
 *  in the filter maintained by Log Service on a per Log Stream basis enter the
 *  Log Stream. Similarly, pLogMsgIdSet and pLogCompIdSet is used to specify
 *  filter based on message ID and component ID respectively. Here the msgId and
 *  the compId to be masked, that is, not allowed to enter the Log Stream should
 *  be specified.To clear a previously set filter, all the fields of filter should
 *  be set to zero and logFilterFlags should be set to CL_LOG_FILTER_ASSIGN.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogStreamOpen(), clLogWriteAsync(), clLogStreamClose()
 *
 */
extern ClRcT
clLogFilterSet(CL_IN ClLogStreamHandleT  hStream,
               CL_IN ClLogFilterFlagsT   filterFlags,
               CL_IN ClLogFilterT        filter);

/**
 **********************************************************************
 *  \brief Registers the calling process as handler for the specified stream.
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hLog (in) Handle obtained by the previous invocation
 *  clLogInitialize(). This handle identifies the association between the
 *  calling process and the Log service.
 *
 *  \param streamName (in) Name of the stream for which this process wants
 *  to become a handler.
 *
 *  \param streamScope (in) Scope of the stream identified by streamName.
 *
 *  \param nodeName (in) Name of the node where the Log Stream exists. 
 *  This is valid only if streamScope is set to CL_LOG_SCOPE_LOCAL. 
 *  If streamScope is set to CL_LOG_SCOPE_GLOBAL, this parameter is ignored.
 *
 *  \param handlerFlags (in) It is the bitwise ORed value of flags defined in 
 *  ::ClLogStreamHandlerFlagsT. \c CL_LOG_HANDLER_WILL_ACK must be specified,
 *  if the handler wants to explicitly acknowledge the receipt of Log Records.
 *  Typicaly, Log File Handler, which is part of Log Service, uses this flag.
 *  Other handlers must pass zero as a value of this parameter.
 *
 *  \param phStream (out) Pointer to memory area where the handle to the Log 
 *  Stream is returned. This handle must be used for further operations on 
 *  this stream.
 *
 *  \retval  CL_OK The process has been registered as a handler for the
 *  stream successfully.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed handle \c hLog is either not obtained
 *  through clLogInitialize() or this handle association has been already closed
 *  by another invocation clLogFinalize(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function registers interest of the calling process in handling the Log
 *  Stream. Once the interest has been registered, the calling process will start
 *  getting clLogRecordDeliverCallback as specified during the invocation to
 *  clLogInitialize(). Memory for \e phStream is allocated and freed by the calling
 *  process.In case, this function is called multiple times on the same stream with
 *  the same Log Service handle hLog, every time a new Log Stream handle \e hStream
 *  will be issued. During the delivery of Log Records, the callback will be called
 *  once per such handle issued.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogInitialize(),clLogHandlerDeregister()
 *
 */
extern ClRcT
clLogHandlerRegister(CL_IN   ClLogHandleT              hLog,
                     CL_IN   ClNameT                   streamName, 
                     CL_IN   ClLogStreamScopeT         streamScope,
                     CL_IN   ClNameT                   nodeName,   
                     CL_IN   ClLogStreamHandlerFlagsT  handlerFlags,
                     CL_OUT  ClLogHandleT              *phStream);
/**
 **********************************************************************
 *  \brief Deregisters the calling process as handler for the specified
 *   stream.
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hStream (in) Handle obtained through a previous invocation to
 *  clLogHandlerRegister(). This handle identifies the registration of handler
 *  for a stream which is being deregistered.
 *
 *  \retval  CL_OK The handle has been deregistered successfully.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log Stream handle is not valid.
 *  Either it is not received through a previous invocation to clLogHandlerRegister()
 *  or it has already been deregistered through an invocation to clLogHandlerDeregister()
 *  or hLog supplied to clLogHandlerRegister() during registration of the handler has
 *  been finalized.
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function deregisters interest of the calling process in handling the
 *  Log Stream. Once the interest has been deregistered, the calling process will
 *  stop getting clLogRecordDeliverCallback. This call cancels all the pending
 *  clLogRecordDeliverCallback callbacks. Since callback invocation is
 *  asynchronous, the process may still get some pending callbacks. After this
 *  invocation, hStream is no longer valid. If the handler process terminates
 *  without deregistering its interest, Log Service will implicitly deregisters
 *  the process. If the Log Service handle \e hLog used during clLogHandlerRegister
 *  invocation is finalized without explicitly deregistering the interest in
 *  handling the stream, Log Service will implicitly deregisters the process.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogHandlerRegister()
 *
 */
 extern ClRcT
 clLogHandlerDeregister(CL_IN ClLogStreamHandleT  hStream);

/**
 **********************************************************************
 *  \brief Function to acknowledge, receipt of Log Records, to the sender 
 *  of the Log Records.
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hStream (in) Handle obtained through a previous invocation to
 *  clLogHandlerRegister(). This handle identifies the registration of handler
 *  for a stream which is records are being acknowledged
 *
 *  \param sequenceNumber (in) Number identifying the set of Log Records 
 *  received.This number is obtained from ClLogRecordDeliverCallbackT.
 *
 *  \param numRecords (in) Number of records received successfully by 
 *  the handler
 *
 *  \retval  CL_OK The api has successfully sent the acknowledgement.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log Stream handle is not valid. 
 *  Either it is not received through a previous invocation to
 *  clLogHandlerRegister() or it has already been closed through an invocation
 *  to clLogHandlerDeregister(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \retval CL_ERR_BAD_FLAG \c CL_LOG_HANDLER_WILL_ACK flag was not set while registering
 *  the handler.
 *
 *  \par Description:
 *  This function acknowledges the receipt of Log Records for the Log Stream that
 *  this handler is interested in. Handler should acknowledge only if it
 *  registered with the flag \c CL_LOG_HANDLER_WILL_ACK in the previous invocation
 *  of function, clLogHandlerRegister. The sender of the Log Records treat the
 *  records as lost if the acknowledgement is not received in specified time
 *  limit.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogHandlerRegister()
 *
 */
 extern ClRcT
 clLogHandlerRecordAck(CL_IN ClLogStreamHandleT  hStream,
                       CL_IN ClUint64T           sequenceNumber,
                       CL_IN ClUint32T           numRecords);

/**
 **********************************************************************
 *  \brief Opens the current logical log file for reading the log records. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hLog (in) Handle obtained by the previous invocation
 *  clLogInitialize(). This handle identifies the association between the
 *  calling process and the Log service.
 *
 *  \param fileName (in) Name of the file that the process wants to open for
 *  reading the records.
 *
 *  \param fileLocation (in) Location where the file exists. 
 *   refer ::ClLogStreamAttributesT.
 *
 *  \param isDelete (in) Parmeter that tell the log service to delete the records
 *  read by this file handler from the file for new records.
 *
 *  \param phFile (out) Pointer to memory area where the handle to the Log File
 *  is returned. This handle must be used for further operations on this Log File.
 *
 *  \retval  CL_OK The api has successfully opened file for reading.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed Log handle is not valid. 
 *  Either it is not received through a previous invocation to
 *  clLogInitialize() or it has already been closed through an invocation
 *  to clLogFinalize(). 
 *
 *  \retval CL_ERR_NULL_POINTER \e phFile parameter is passed as NULL.
 *
 *  \retval CL_ERR_NOT_EXITS The Log File identified by filename and fileLocation
 *  does not exist.
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function opens a logical log file for the process to get records and
 *  metadata. Once the file is opened the metadata can be obtained by invoking
 *  clLogFileMetadataGet() and records can be obtained by invocation of
 *  clLogFileRecordsGet() Memory for phFIle is allocated and freed by the 
 *  calling process. Each invocation of this call returns a new handle and will 
 *  start reading the records from the oldest record available.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogInitialize
 *
 */
extern ClRcT
clLogFileOpen(CL_IN ClLogHandleT      hLog,
              CL_IN ClCharT           *fileName,
              CL_IN ClCharT           *fileLocation,
              CL_IN ClBoolT           isDelete, 
              CL_IN ClLogFileHandleT  *phFile);

/**
 **********************************************************************
 *  \brief Close the file which was opened for reading. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hFile (in) Handle obtained through a previous invocation to 
 *  clLogFileOpen(). This handle identifies the Log File opened.
 *
 *  \retval  CL_OK The api has successfully opened file for reading.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed file handle is not valid. 
 *  Either it is not received through a previous invocation to
 *  clLogFileOpen() or it has already been closed through an invocation
 *  to clLogFileClose(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before 
 *  the call could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can 
 *  not be provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  This function closes the Log File opened by the process for reading the log
 *  records, once the file is closed no more records can be read from it.
 *  Metadata of the file cannot be read after that. 
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogFileOpen
 *
 */
extern ClRcT
clLogFileClose(CL_IN ClLogFileHandleT  hFileHdlr);

/**
 **********************************************************************
 *  \brief Gets the metadata of the file. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hFile (in) Handle obtained through a previous invocation to
 *  clLogFileOpen().This handle identifies the Log File whose meta data
 *  is to be obtained.
 *
 *  \param streamAttr (out) Attributes of all the stream that are being
 *  persisted into the file identified by hFile
 *
 *  \param pNumStreams (out) Number of streams whose records are currently
 *  being persisted into the Log File identified by hFile. This indicates 
 *  the number of entries in ppLogStreams. Memory for pNumStreams is allocated
 *  and freed by the calling process.
 *
 *  \param ppLogStreams (out) Pointer to a memory area where pointer to array
 *  of Log Stream information will be stored. Each entry in this array 
 *  corresponds to one Log Stream in the File. Memory for ppLogStreams is 
 *  allocated and freed by the calling process, whereas, memory for 
 *  \e *ppLogStreams is allocated by the Log Service and freed by the calling
 *  process.
 *
 *  \retval  CL_OK The api has been successfully executed.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed file handle is not valid. 
 *  Either it is not received through a previous invocation to
 *  clLogFileOpen() or it has already been closed through an invocation
 *  to clLogFileClose(). 
 *
 *  \retval CL_ERR_TIMEOUT An implementation defined timeout occurred before 
 *  the call could complete.
 *
 *  \retval CL_ERR_NULL_POINTER Either pNumRecords or pLogRecords are passed
 *  as NULL.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can 
 *  not be provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  The function returns the Log Records from the file identified by hFile.
 *  Function returns the set of record starting from the oldest record written in
 *  the Log File. After sending the set, the records are cleaned up from the
 *  file, that is, these records are no longer available, thus creating space for
 *  new records
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogFileOpen
 *
 */

extern ClRcT
clLogFileMetaDataGet(CL_IN  ClLogFileHandleT        hFileHdlr,
                     CL_OUT ClLogStreamAttributesT  *pStreamAttr,
                     CL_OUT ClUint32T               *pNumStreams,
                     CL_OUT ClLogStreamMapT         **ppLogStreams);

/**
 **********************************************************************
 *  \brief Gets the records from the file. 
 *
 *  \par Header File:
 *  clLogApi.h
 *
 *  \param hFile (in) Handle obtained through a previous invocation to 
 *  clLogFileOpen().This handle identifies the Log File whose records are
 *  to be read.
 *
 *  \param pStartTime (out) Its the lowest timestamp in the set of 
 *  records being returned in this call in pLogRecords. It identifies the 
 *  oldest record in the current set. Memory for pStartTime is allocated 
 *  and freed by the calling process.
 *
 *  \param pEndTime (out) Its the highest timestamp in the set of records 
 *  being returned in this call in pLogRecords. It identifies the freshest
 *  record in the current set. Memory for pEndTime is allocated and freed 
 *  by the calling process.
 *
 *  \param pNumRecords (in-out) Caller specifies the number of records it 
 *  wants.Function returns the actual number of records that are read. 
 *  Function always read from the oldest record written into the file, which
 *  is not read till now. It reads till the number of records requested or 
 *  number of records available, whichever is smaller. Memory for \e pNumStreams
 *  is allocated and freed by the calling process.
 *
 *  \param pLogRecords (out) Pointer to a memory area where the Log Records 
 *  read will be stored. Total number of Log Records stored will be specified 
 *  by the value of \e pNumRecords. Memory for pLogRecords is allocated and freed 
 *  by the calling process.
 *
 *  \retval  CL_OK The api has been successfully executed.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed file handle is not valid. 
 *  Either it is not received through a previous invocation to
 *  clLogFileOpen() or it has already been closed through an invocation
 *  to clLogFileClose(). 
 *
 *  \retval CL_ERR_TIMEOUT An implementation defined timeout occurred before 
 *  the call could complete.
 *
 *  \retval CL_ERR_NULL_POINTER Either pNumRecords or pLogRecords are passed
 *  as NULL.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can 
 *  not be provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  The function returns the Log Records from the file identified by hFile.
 *  Function returns the set of record starting from the oldest record written in
 *  the Log File. After sending the set, the records are cleaned up from the
 *  file, that is, these records are no longer available, thus creating space for
 *  new records
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogFileOpen
 *
 */
extern ClRcT
clLogFileRecordsGet(CL_IN  ClLogFileHandleT  hFileHdlr,
                    CL_OUT ClTimeT           *pStarTime,
                    CL_OUT ClTimeT           *pEndTime, 
                    CL_OUT ClUint32T         *pNumRecords,
                    CL_OUT ClPtrT            *pLogRecords);
/**
 ************************************
 *  \brief Gets the list of active streams available in the cluster.
 *
 *  \par Header File:
 *  clLogApi.h, clLogErrors.h
 *
 *  \param hLog (in) Handle obtained by the previous invocation
 *  clLogInitialize(). This handle identifies the association between the
 *  calling process and log service. 
 *
 *  \param pNumStreams (out) Number of streams that are currently open in the
 *  cluster. This indicates the number of entries in \e ppLogStreams. Memory for
 *  \e pNumStreams is allocated and freed by the calling process.
 *
 *  \param ppLogStreams (out) Pointer to a memory area where pointer to array
 *  of Log Stream information will be stored. Each entry in this array corresponds
 *  to one Log Stream in the cluster. Memory for \e ppLogStreams is allocated and 
 *  freed by the calling process, whereas, memory for \e *ppLogStreams is allocated 
 *  by the Log Service and freed by the calling process.
 *
 *  \retval  CL_OK The API has been successfully executed.
 *
 *  \retval  CL_ERR_INVALID_HANDLE The passed handle(hLog) is either not obtained
 *  through clLogInitialize() or this handle association has been already closed
 *  by another invocation clLogFinalize(). 
 *
 *  \retval  CL_ERR_TIMEOUT An implementation defined timeout occurred before the call
 *  could complete.
 *
 *  \retval  CL_ERR_TRY_AGAIN The service could not be provided at this time. 
 *  The process may try later.
 *
 *  \retval  CL_ERR_NULL_POINTER Either pNumStreams or ppLogStreams are passed 
 *  as NULL.
 *
 *  \retval  CL_ERR_NO_MEMORY Either the Log Service library or some other module of 
 *  Log Service is out of memory. Thus service can not be provided at this time. This
 *  may be a transient problem.
 *
 *  \retval  CL_ERR_NO_RESOURCE Either the Log Service library or some other module 
 *  of Log Service is out of resources (other than memory). Thus, service can not be
 *  provided at this time. This may be a transient problem.
 *
 *  \par Description:
 *  The function returns information about all the Log Streams in the cluster.
 *  It is used by Log Handlers to find out Log Streams in which they may be
 *  interested in.
 *
 *  \par Library File:
 *  ClLogClient
 *
 *  \sa clLogStreamOpen(), clLogStreamClose()
 *
 */
extern ClRcT
clLogStreamListGet(CL_IN  ClLogHandleT      hLog,
                   CL_OUT ClUint32T         *pNumStreams,
                   CL_OUT ClLogStreamInfoT  **ppLogStreams);

/**
 *  \brief Logging for applications
 *  \par Description
 *  This macro provides the support to log messages by specifying
 *  the severity of log message and server information like the 
 *  sub-component area and the context of logging.
 *  \par
 *  This macro is for applications as well as system components to output log message
 *  into file sys.0 file. This is default log file which ASP components use.
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
 *  \param libname (in) This is a character pointer specifiying the library name
 *  which is logging. 
 *
 *  \param msg (in) contains format string for the message.
 *
 *  \param ... (in) use a printf style string and arguments for your log message.
 */
 #define clLogWrite(streamHdl, severity, libName, ...)\
 do\
 {\
    clLog(severity, libName, CL_LOG_CONTEXT_UNSPECIFIED, __VA_ARGS__);\
 } while(0)

ClRcT
clLogWriteDeferred(ClHandleT      handle,
                   ClLogSeverityT severity,
                   ClUint16T      servicId,
                   ClUint16T      msgId,
                   ClCharT        *pFmtStr,
                   ...) CL_PRINTF_FORMAT(5, 6);

ClRcT
clLogWriteDeferredForce(ClHandleT      handle,
                        ClLogSeverityT severity,
                        ClUint16T      servicId,
                        ClUint16T      msgId,
                        ClCharT        *pFmtStr,
                        ...) CL_PRINTF_FORMAT(5, 6);

extern ClRcT
clLogMsgWrite(ClHandleT       streamHdl,
              ClLogSeverityT  severity,
              ClUint16T       serviceId,
              const ClCharT         *pArea,
              const ClCharT         *pContext,
              const ClCharT         *pFileName,
              ClUint32T       lineNum,
              const ClCharT   *pFmtStr,
              ...) CL_PRINTF_FORMAT(8, 9);

extern ClRcT
clLogMsgWriteDeferred(ClHandleT       streamHdl,
                      ClLogSeverityT  severity,
                      ClUint16T       serviceId,
                      const ClCharT         *pArea,
                      const ClCharT         *pContext,
                      const ClCharT         *pFileName,
                      ClUint32T       lineNum,
                      const ClCharT   *pFmtStr,
                      ...) CL_PRINTF_FORMAT(8, 9);

extern ClRcT
clLogMsgWriteConsole(ClHandleT       streamHdl,
                     ClLogSeverityT  severity,
                     ClUint16T       serviceId,
                     const ClCharT         *pArea,
                     const ClCharT         *pContext,
                     const ClCharT         *pFileName,
                     ClUint32T       lineNum,
                     const ClCharT   *pFmtStr,
                     ...) CL_PRINTF_FORMAT(8, 9);

ClRcT clLogSeverityFilterToValueGet(ClLogSeverityFilterT filter, ClLogSeverityT* pSeverity);
ClRcT clLogSeverityValueToFilterGet(ClLogSeverityT severity, ClLogSeverityFilterT* pFilter);

ClRcT clLogStreamFilterSet(ClNameT                  *pStreamName, 
                                ClLogStreamScopeT   streamScope,
                                ClNameT             *pStreamScopeNode,
                                ClLogFilterFlagsT   filterFlags,
                                ClLogFilterT        filter);

ClRcT
clLogStreamFilterGet(ClNameT                *pStreamName,
                        ClLogStreamScopeT   streamScope,
                        ClNameT             *pStreamScopeNode,
                        ClLogFilterT        *pFilter);

ClLogSeverityT
clLogSeverityGet(const ClCharT  *pSevName);

#include <ipi/clLogIpiWrap.h> 
#ifdef __cplusplus
}
#endif

#endif /*_CL_LOG_API_H_*/

/** 
 * \} 
 */
