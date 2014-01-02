#include <clHandleApi.hpp>
#include <clGlobals.hpp>

#define CL_LOG_CLIENT_VERSION    {'B', 0x01, 0x01}

namespace SAFplus
{

  /* Log handle */
class Logger
{
public:
};
  
enum
  {
    CL_LOG_BUFFER_DEFAULT_LENGTH = 1024*1024  // use clLogBufferSize in your code for the actual size.
  };
  
  
typedef enum
  {
/**
 * setting severity as EMERGENCY.
 */
    LOG_SEV_EMERGENCY = 0x1,
/**
 * setting severity as ALERT. 
 */
    LOG_SEV_ALERT,
/**
 * setting severity as CRITICAL. 
 */
    LOG_SEV_CRITICAL,
/**
 * setting severity as ERROR. 
 */
    LOG_SEV_ERROR,
/**
 * setting severity as WARNING. 
 */
    LOG_SEV_WARNING,
/**
 * setting severity as NOTICE. 
 */
    LOG_SEV_NOTICE,
/**
 * setting severity as INFORMATION.
 */
    LOG_SEV_INFO,
/**
 * setting severity as DEBUG.
 */
    LOG_SEV_DEBUG,
    LOG_SEV_DEBUG1   =      LOG_SEV_DEBUG,
    LOG_SEV_DEBUG2,
    LOG_SEV_DEBUG3,
    LOG_SEV_DEBUG4,
    LOG_SEV_DEBUG5,
/**
 * setting severity as DEBUG.
 */
    LOG_SEV_TRACE  =    LOG_SEV_DEBUG5,
    LOG_SEV_DEBUG6,
    LOG_SEV_DEBUG7,
    LOG_SEV_DEBUG8,
    LOG_SEV_DEBUG9,
/**
 * Maximum severity level. 
 */
  LOG_SEV_MAX = LOG_SEV_DEBUG9
  } LogSeverityT;

  Logger* logInitialize(ClVersionT* pVersion=0);

void logMsgWrite(HandleT streamHdl, LogSeverityT  severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum, const char *pFmtStr,...) CL_PRINTF_FORMAT(8, 9);

};

#ifndef SAFPLUS_COMPONENT
/*
 * Macros to log messages at different levels of severity
   streamHandle, severity, serviceId, area, context, ...) \
 */
    
#define logEmergency(area, context, ...) appLog(APP_LOG, SAFplus::LOG_SEV_EMERGENCY, 0, area, context, __VA_ARGS__)

#define logAlert(area, context, ...) appLog(APP_LOG,SAFplus::LOG_SEV_ALERT, 0,area, context, __VA_ARGS__)

#define logCritical(area, context, ...) appLog(APP_LOG,SAFplus::LOG_SEV_CRITICAL,0, area, context, __VA_ARGS__)

#define logError(area, context, ...) appLog(APP_LOG,SAFplus::LOG_SEV_ERROR, 0,area, context, __VA_ARGS__)
        
#define logWarning(area, context, ...) appLog(APP_LOG,SAFplus::LOG_SEV_WARNING, 0,area, context, __VA_ARGS__)

#define logNotice(area, context, ...) appLog(APP_LOG,LOG_SEV_NOTICE, 0,area, context, __VA_ARGS__)

#define logInfo(area, context, ...) appLog(APP_LOG,LOG_SEV_INFO, 0,area, context, __VA_ARGS__)

#define logDebug(area, context, ...) appLog(APP_LOG,LOG_SEV_DEBUG,0, area, context, __VA_ARGS__)

#define logTrace(area, context, ...) appLog(APP_LOG,LOG_SEV_TRACE,0, area, context, __VA_ARGS__)
#endif

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
#define appLog(streamHandle, severity, serviceId, area, context, ...) \
do                                                                      \
{                                                                       \
  SAFplus::logMsgWrite(streamHandle, severity, serviceId, area, context, __FILE__, __LINE__, __VA_ARGS__); \
} while(0)
