#ifndef _CL_LOG_API_EXT_H_
#define _CL_LOG_API_EXT_H_

#include <clLogApi.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 *  \param severity (in) This field must be set to one of the values defined 
 *  in .It defines the severity level of the Log Record being written.
 *
 *  \param serviceId (in) This field identifies the module within the process
 *  which is generating this Log Record. If the Log Record message is a generic
 *  one like out of memory, this field can be used to narrow down on the module 
 *  impacted. For ASP client libraries, these values are defined in clCommon.h.
 *  For application modules, it is up-to the application developer to define the
 *  values and scope of those values.
 *
 *  \param msgId (in) This field identifies the actual message to be Logged. 
 *  This is typically an identifier for a string message which the viewer is
 *  aware of through off-line mechanism.
 *
 *  \param list variable length argument list. Rest of the arguments of this
 *  function is passed through this argument. For application Log
 *  Streams, the values and scope of each value is defined by the application
 *  developer. Following two values are pre-defined:
 *  \arg \c #CL_LOG_MSGID_BUFFER
 *  \arg \c #CL_LOG_MSGID_PRINTF_FMT 
 *  
 *  In case, msgId is passed as CL_LOG_MSGID_BUFFER, it is followed by two
 *  parameters:
 *  First one is of type ClUint32T. It is the number of bytes in the buffer
 *  pointed by the second parameter.
 *  Second one is a pointer to a buffer. It is of type ClPtrT. The buffer may
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
 *  \arg \c First one is of type ClUint16T. It is treated as the tag to identify contents
 *  of third argument in this tuple.
 *  \arg \c Second one is of type ClUint16T. It is treated as number of bytes in the
 *  buffer pointed by third argument.
 *  Third one is of type ClPtrT. The buffer may contain binary or ASCII data,
 *  which is defined by the tag (first argument in this tuple). The Log Service
 *  does not interpret this buffer. Only the Log Consumer interprets it and must
 *  be aware of the semantic meaning of its contents. It simply copies this buffer
 *  in the Log Record. In case of ASCII data, if the buffer is NULL terminated,
 *  the length must include the NULL termination byte. This buffer is not freed by
 *  the Log Service.
 *  End of this set of tuple is indicated by a special tag \e CL_LOG_TAG_TERMINATE.
 *  Thus, the variable part of this argument list will always have 3*n + 1, 
 *  where n is the number of parameters to be logged. Following tag values are 
 *  defined by Log Service, rests are defined by the application.
 *  \arg \c CL_LOG_TAG_TERMINATE        
 *  \arg \c CL_LOG_TAG_BASIC_SIGNED    
 *  \arg \c CL_LOG_TAG_BASIC_UNSIGNED 
 *  \arg \c CL_LOG_TAG_STRING        
 *  Log Service also defines following macros to ease the use of this function.
 *  \arg \c #define CL_LOG_TLV_UINT8(var)               \
 *  CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_INT8(var)                \
 *  CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_UINT16(var)              \
 *  CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_INT16(var)               \
 *  CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_UINT32(var)              \
 *  CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_INT32(var)               \
 *  CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_UINT64(var)              \
 *  CL_LOG_TAG_BASIC_UNSIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_INT64(var)               \
 *  CL_LOG_TAG_BASIC_SIGNED, sizeof( var ), &(var)
 *  \arg \c #define CL_LOG_TLV_STRING(var)              \
 *  CL_LOG_TAG_STRING, (strlen( var ) + 1), var
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
clLogVWriteAsync(ClLogStreamHandleT  hStream,
                 ClLogSeverityT      severity,
                 ClUint16T           serviceId,
                 ClUint16T           msgId,
                 va_list             list);

extern ClRcT
clLogVWriteAsyncWithHeader(ClLogStreamHandleT   hStream,
                           ClLogSeverityT       logSeverity,
                           ClUint16T            serviceId,
                           ClUint16T            msgId,
                           ClCharT              *pMsgHeader,
                           va_list              args);

extern ClRcT
clLogWriteWithHeader(ClLogStreamHandleT   hStream,
                     ClLogSeverityT       logSeverity,
                     ClUint16T            serviceId,
                     ClUint16T            msgId,
                     ClCharT              *pMsgHeader,
                     ...);

extern ClRcT
clLogHeaderGet(ClCharT *pMsgHeader, ClUint32T maxHeaderSize);

extern ClRcT
clLogHeaderGetWithContext(const ClCharT *pArea, const ClCharT *pContext, 
                          ClCharT *pMsgHeader, ClUint32T maxHeaderSize);

extern ClCharT * 
clLogSeverityStrGet(ClLogSeverityT severity);

#ifdef __cplusplus
}
#endif

#endif
