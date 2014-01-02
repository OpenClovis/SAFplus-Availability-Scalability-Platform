#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
using namespace boost::interprocess;

#include <clCommon.h>
#include <clEoApi.h>

#include <clLogIpi.hpp>
#include <clCommon.hpp>
/* Old code */
#include <clLogApi.h>


static const char* CL_LOG_PRINT_FMT_STR     =         "%-26s [%s:%d] (%s.%d : %s.%3s.%3s:%05d : %s) ";
static const char* CL_LOG_PRINT_FMT_STR_WO_FILE    =   "%-26s (%s.%d : %s.%3s.%3s:%05d : %s) ";

using namespace SAFplus;
using namespace SAFplusI;

uint_t msgIdCnt=0;
char* SAFplus::logCompName=NULL;
bool  logCodeLocationEnable=true;
/* Supported Client Version */
static ClVersionT gLogClntVersionsSupported[] = {CL_LOG_CLIENT_VERSION};

/* Supported Client Version database */
static ClVersionDatabaseT gLogClntVersionDb = {
    sizeof(gLogClntVersionsSupported) / sizeof(ClVersionT),
    gLogClntVersionsSupported
};

static const ClCharT* logSeverityStrGet(SAFplus::LogSeverityT severity);

Logger* SAFplus::logInitialize(ClVersionT* pVersion)
{
    ClRcT       rc          = CL_OK;
    ClBoolT     firstHandle = CL_FALSE;
    ClIocPortT  port        = 0;
    if (pVersion)
      {
        rc = clVersionVerify(&gLogClntVersionDb, pVersion);
        CL_ASSERT(CL_OK != rc);
      }

    logInitializeSharedMem();

    utilsInitialize();  /* Logging uses globals initialized by utils, but is tolerant of uninitialized vals.  Utils may log so this must be run AFTER logging is inited */
}

void SAFplus::logMsgWrite(HandleT streamHdl, LogSeverityT  severity, uint_t serviceId, const char *pArea, const char  *pContext, const char *pFileName, uint_t lineNum, const char *pFmtStr,...)
{
  uint_t            msgStrLen;
  const ClCharT    *pSevName           = NULL;
  ClCharT      timeStr[40]   = {0};
  ClCharT           msg[CL_LOG_MAX_MSG_LEN]; // note this should be able to be removed and directly copied to shared mem.
  va_list vaargs;
  va_start(vaargs, pFmtStr);
  pSevName = logSeverityStrGet(severity);

  SAFplusI::logTimeGet(timeStr, (ClUint32T)sizeof(timeStr));
  // Create the log header
  if(logCodeLocationEnable)        
    {
      msgStrLen = snprintf(msg, CL_LOG_MAX_MSG_LEN - 1, CL_LOG_PRINT_FMT_STR, 
                           timeStr, pFileName, lineNum, SAFplus::ASP_NODENAME, pid,
                              ((logCompName!=NULL) ? logCompName:SAFplus::ASP_COMPNAME), pArea, pContext, msgIdCnt, pSevName);
                
    }
  else
    {
      msgStrLen = snprintf(msg, CL_LOG_MAX_MSG_LEN - 1, CL_LOG_PRINT_FMT_STR_WO_FILE,
                              timeStr, SAFplus::ASP_NODENAME, pid,
                              ((logCompName!=NULL) ? logCompName:SAFplus::ASP_COMPNAME), pArea, pContext,
                              msgIdCnt, pSevName);
    }

  // Now append the log message        
  msgStrLen += vsnprintf(msg + msgStrLen, CL_LOG_MAX_MSG_LEN - msgStrLen, pFmtStr, vaargs);
  if (msgStrLen > CL_LOG_MAX_MSG_LEN-1) msgStrLen=CL_LOG_MAX_MSG_LEN-1;  // if too big, vsnprintf returns the number of bytes that WOULD HAVE BEEN written.
  
  char *base    = (char*) clLogBuffer->get_address();
  
  //clientMutex.wait();
  clientMutex.lock();
  LogBufferEntry* rec = static_cast<LogBufferEntry*>((void*)(((char*)base)+clLogBufferSize-sizeof(LogBufferEntry)));  
  rec -= (clLogHeader->numRecords);

  rec->stream = streamHdl;
  rec->offset = clLogHeader->msgOffset;
  memcpy(base+rec->offset, msg, msgStrLen+1); // length + 1 includes the null terminator

  clLogHeader->numRecords++;
  clLogHeader->msgOffset += msgStrLen+1;

  msgIdCnt++;  // While this is not part of the log header, it needs to be thread-safe so its easiest to put it in here.

  //clLogHeader->clientMutex.post();
  clientMutex.unlock();
  va_end(vaargs);
}

static const ClCharT* logSeverityStrGet(LogSeverityT severity)
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
