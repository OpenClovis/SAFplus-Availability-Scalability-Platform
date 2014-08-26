#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
using namespace boost::interprocess;

#include <clCommon.h>
#include <clEoApi.h>

#include <clLogIpi.hxx>
#include <clCommon.hxx>

#include <clLogApi.hxx>


static const char* CL_LOG_PRINT_FMT_STR     =         "%-26s [%s:%d] (%s.%d : %s.%3s.%3s:%05d : %s) ";
static const char* CL_LOG_PRINT_FMT_STR_WO_FILE    =   "%-26s (%s.%d : %s.%3s.%3s:%05d : %s) ";

using namespace SAFplus;
using namespace SAFplusI;

uint_t msgIdCnt=0;
const char* SAFplus::logCompName=NULL;
bool  logCodeLocationEnable=true;

/* Supported Client Version */
static ClVersionT gLogClntVersionsSupported[] = {CL_LOG_CLIENT_VERSION};

/* Supported Client Version database */
static ClVersionDatabaseT gLogClntVersionDb = {
    sizeof(gLogClntVersionsSupported) / sizeof(ClVersionT),
    gLogClntVersionsSupported
};

static const ClCharT* logSeverityStrGet(SAFplus::LogSeverity severity);

Logger* SAFplus::logInitialize()
{
    ClRcT       rc          = CL_OK;
    ClBoolT     firstHandle = CL_FALSE;
    ClIocPortT  port        = 0;
#if 0    
    if (pVersion)
      {
        rc = clVersionVerify(&gLogClntVersionDb, pVersion);
        CL_ASSERT(CL_OK != rc);
      }
#endif
    logInitializeSharedMem();

    utilsInitialize();  /* Logging uses globals initialized by utils, but is tolerant of uninitialized vals.  Utils may log so this must be run AFTER logging is inited */
}

uint_t SAFplusI::formatMsgPrefix(char* msg, LogSeverity  severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum)
{
  const ClCharT    *pSevName           = NULL;
  uint_t            msgStrLen;
  ClCharT           timeStr[40]   = {0};
  pSevName = logSeverityStrGet(severity);

  SAFplusI::logTimeGet(timeStr, (ClUint32T)sizeof(timeStr));
  // Create the log header
  if(SAFplus::logCodeLocationEnable)        
    {
      msgStrLen = snprintf(msg, SAFplus::LOG_MAX_MSG_LEN - 1, CL_LOG_PRINT_FMT_STR, timeStr, pFileName, lineNum, SAFplus::ASP_NODENAME, SAFplus::pid,((logCompName!=NULL) ? logCompName:SAFplus::ASP_COMPNAME), pArea, pContext, msgIdCnt, pSevName);              
    }
  else
    {
      msgStrLen = snprintf(msg, SAFplus::LOG_MAX_MSG_LEN - 1, CL_LOG_PRINT_FMT_STR_WO_FILE,timeStr, SAFplus::ASP_NODENAME, SAFplus::pid,((logCompName!=NULL) ? logCompName:SAFplus::ASP_COMPNAME), pArea, pContext, msgIdCnt, pSevName);
    }
  
  return msgStrLen;
}

void SAFplusI::writeToSharedMem(Handle streamHdl,LogSeverity severity, char* msg, int msgStrLen)
{
  char *base    = (char*) clLogBuffer->get_address();  

  clientMutex.lock();
  LogBufferEntry* rec = static_cast<LogBufferEntry*>((void*)(((char*)base)+clLogBufferSize-sizeof(LogBufferEntry)));  
  rec -= (clLogHeader->numRecords);

  rec->stream   = streamHdl;
  rec->offset   = clLogHeader->msgOffset;
  rec->severity = severity;
  if (rec->offset + msgStrLen+1 < SAFplus::CL_LOG_BUFFER_DEFAULT_LENGTH)
    {
    memcpy(base+rec->offset, msg, msgStrLen+1); // length + 1 includes the null terminator
    }
  else
    {
    msgStrLen = sizeof("Log buffer overflow");
    if (rec->offset + msgStrLen +1 < SAFplus::CL_LOG_BUFFER_DEFAULT_LENGTH)
      strcpy(base+rec->offset, "Log buffer overflow");
    else
      {
      clientMutex.unlock();
      return; // No room even to write the overflow message
      }
    }

  clLogHeader->numRecords++;
  clLogHeader->msgOffset += msgStrLen+1;

  msgIdCnt++;  // While this is not part of the log header, it needs to be thread-safe so its easiest to put it in here.

  //clLogHeader->clientMutex.post();
  clientMutex.unlock();
}

static const ClCharT* logSeverityStrGet(LogSeverity severity)
{
  if( severity == LOG_SEV_EMERGENCY )
    {
        return "EMRGN";
    }
    else if( severity == LOG_SEV_ALERT )
    {
        return "ALERT";
    }
    else if( severity == LOG_SEV_CRITICAL )
    {
        return "CRITIC";
    }
    else if( severity == LOG_SEV_ERROR )
    {
        return "ERROR";
    }
    else if( severity == LOG_SEV_WARNING )
    {
        return "WARN";
    }
    else if( severity == LOG_SEV_NOTICE )
    {
        return "NOTICE";
    }
    else if( severity == LOG_SEV_INFO )
    {
        return "INFO";
    }
    else if( severity == LOG_SEV_DEBUG )
    {
        return "DEBUG";
    }
    else if( severity == LOG_SEV_TRACE )
    {
        return "TRACE";
    }
    return "DEBUG";
}

void SAFplus::logMsgWrite(Handle streamHdl, LogSeverity  severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum, const char *pFmtStr,...)
{
  uint_t            msgStrLen;
  const ClCharT    *pSevName           = NULL;
  ClCharT           timeStr[40]   = {0};
  ClCharT           msg[SAFplus::LOG_MAX_MSG_LEN]; // note this should be able to be removed and directly copied to shared mem.
  va_list vaargs;

  if (severity > SAFplus::logSeverity) return;  // don't log if the severity cutoff is lower than that of the log.  Note that the server ALSO does this check...
  
  va_start(vaargs, pFmtStr);

  msgStrLen = formatMsgPrefix(msg,severity, serviceId, pArea, pContext, pFileName, lineNum);
  // Now append the log message        
  msgStrLen += vsnprintf(msg + msgStrLen, SAFplus::LOG_MAX_MSG_LEN - msgStrLen, pFmtStr, vaargs);
  if (msgStrLen > SAFplus::LOG_MAX_MSG_LEN-1) msgStrLen=SAFplus::LOG_MAX_MSG_LEN-1;  // if too big, vsnprintf returns the number of bytes that WOULD HAVE BEEN written.
  va_end(vaargs);
  
  if (logEchoToFd != -1) { write(logEchoToFd,msg,msgStrLen); write(logEchoToFd,"\n",sizeof("\n")-1); }
  writeToSharedMem(streamHdl,severity,msg,msgStrLen);
}

void SAFplus::logStrWrite(Handle streamHdl, LogSeverity  severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum, const char *str)
{
  uint_t            msgStrLen;
  ClCharT           msg[SAFplus::LOG_MAX_MSG_LEN]; // note this should be able to be removed and directly copied to shared mem.
  
  if (severity > logSeverity) return;  // don't log if the severity cutoff is lower than that of the log.  Note that the server ALSO does this check...

  msgStrLen = formatMsgPrefix(msg,severity, serviceId, pArea, pContext, pFileName, lineNum);
  // Now append the log message
  strncat(msg + msgStrLen,str,SAFplus::LOG_MAX_MSG_LEN - msgStrLen);
  msgStrLen = strlen(msg);

  if (logEchoToFd != -1) write(logEchoToFd,msg,msgStrLen);
  writeToSharedMem(streamHdl,severity,msg,msgStrLen);
}
