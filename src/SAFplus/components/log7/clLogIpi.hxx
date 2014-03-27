#ifndef clLogIpi_hpp
#define clLogIpi_hpp
#define SAFPLUS_COMPONENT

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <clThreadApi.hxx>
#include <clHandleApi.hxx>
#include <clLogApi.hxx>

namespace SAFplusI
{
    enum
    {
        CL_LOG_MAX_MSG_LEN7 = 1024,
        CL_LOG_BUFFER_HEADER_STRUCT_ID_7 = 0x59492348,
        CL_LOG_BUFFER_DEFAULT_LENGTH = 1024,
    };
    

  class LogBufferHeader
  {
  public:
    uint64_t structId;
    pid_t    serverPid;  // This is used to ensure that 2 servers don't fight for the logs...
    uint_t   numRecords;
    uint_t   msgOffset;
    uint_t   extra[12];  // Expansion space so new fields can be added but old versions can still read the shared memory
    LogBufferHeader()
    {
    }
    
  };

    class LogBufferEntry
    {
    public:
        SAFplus::Handle stream;
        uint64_t offset;
      SAFplus::LogSeverity severity;
    };


  extern SAFplus::ProcSem clientMutex;
  extern SAFplus::ProcSem serverSem;
  
  extern boost::interprocess::shared_memory_object* clLogSharedMem;
  extern boost::interprocess::mapped_region* clLogBuffer;
  extern int clLogBufferSize;
  extern LogBufferHeader* clLogHeader;


  void logInitializeSharedMem(void);  
  void logTimeGet(char   *pStrTime, int maxBytes);

  uint_t formatMsgPrefix(char* msg, SAFplus::LogSeverity severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum);
  void writeToSharedMem(SAFplus::Handle streamHdl,SAFplus::LogSeverity severity, char* msg, int msgStrLen);

};



/*
 * Macros to log messages at different levels of severity
 */
    
/*
 * Macros to log messages at different levels of severity
 */
    
#define logEmergency(area, context, ...) clLog(CL_LOG_SEV_EMERGENCY, area, context, __VA_ARGS__)

#define logAlert(area, context, ...) clLog(CL_LOG_SEV_ALERT, area, context, __VA_ARGS__)

#define logCritical(area, context, ...) clLog(CL_LOG_SEV_CRITICAL, area, context, __VA_ARGS__)

#define logError(area, context, ...) clLog(CL_LOG_SEV_ERROR, area, context, __VA_ARGS__)
        
#define logWarning(area, context, ...) clLog(CL_LOG_SEV_WARNING, area, context, __VA_ARGS__)

#define logNotice(area, context, ...) clLog(CL_LOG_SEV_NOTICE, area, context, __VA_ARGS__)

#define logInfo(area, context, ...) clLog(CL_LOG_SEV_INFO, area, context, __VA_ARGS__)

#define logDebug(area, context, ...) clLog(CL_LOG_SEV_DEBUG, area, context, __VA_ARGS__)

#define logTrace(area, context, ...) clLog(CL_LOG_SEV_TRACE, area, context, __VA_ARGS__)

/**
 * This macro provides the support to log messages by specifying
 * the severity of log message and server information like the 
 * sub-component area and the context of logging.
 *
 * This macro is for ASP components only, since it directs all logs
 * to the OpenClovis system log.
 */

#ifdef clLog
#undef clLog
#endif

#define clLog(severity, area, context, ...)                     \
do                                                              \
{ \
  SAFplus::logMsgWrite(CL_LOG_HANDLE_SYS, (ClLogSeverityT)severity, CL_LOG_DEFAULT_SYS_SERVICE_ID, area, context, __FILE__, __LINE__,  __VA_ARGS__); \
} while(0)


#endif
