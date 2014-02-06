#include <syslog.h>

#include <clLogIpi.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <clGlobals.hpp>
using namespace SAFplus;
using namespace SAFplusI;
using namespace boost::posix_time;

#define Dbg printf

#include "logcfg.hxx"

using namespace SAFplusLog;

Stream* sysStreamCfg;
Stream* appStreamCfg;

#if (CL_LOG_SEV_EMERGENCY == LOG_EMERG)
inline int logLevel2SyslogLevel(int ocll)
{
  return ocll;  // Note openclovis severity levels go to LOG_DEBUG+9, but via trial syslog on Linux accepts these higher levels
}
#else
#warning SAFplus and syslog log severity constants are not the same. Using slow translation table to convert them.
int logLevel2SyslogLevel(int ocll)
{
  switch (ocll)
    {
    case CL_LOG_SEV_EMERGENCY: return LOG_EMERG;
    case CL_LOG_SEV_ALERT: return LOG_ALERT;
    case CL_LOG_SEV_CRITICAL: return LOG_CRIT;
    case CL_LOG_SEV_ERROR: return LOG_ERR;
    case CL_LOG_SEV_WARNING: return LOG_WARNING;
    case CL_LOG_SEV_NOTICE: return LOG_NOTICE;
    case CL_LOG_SEV_INFO: return LOG_INFO;
    case CL_LOG_SEV_DEBUG: return LOG_DEBUG:
    default: return LOG_DEBUG;      
    }
}
#endif

void postRecord(LogBufferEntry* rec, char* msg,LogCfg* cfg)
{
  if (rec->severity > SAFplus::logSeverity) return;  // don't log if the severity cutoff is lower than that of the log.  Note that the client also does this check.
  printf("%s\n",msg);

  // Determine the stream
  Stream* strmCfg = NULL;
  // First check well known streams by hand
  if (rec->stream == APP_LOG) strmCfg = appStreamCfg;
  else if (rec->stream == SYS_LOG) strmCfg = sysStreamCfg;
  else  // Find it if its a dynamic stream
    {
      // The Stream configuration uses string names, but the shared memory uses handles.
      // The Name service will hold the name to handle mapping.  During Log service initialization, a handle to Stream* hash table should
      // be created using data in the Name service.  This hash table can be used to look up the data.
      
    }
  if (strmCfg == NULL)  // Stream is not identified
    {
      return;
    }

  if (strmCfg->syslog)  // output this log to syslog
    {
      syslog(logLevel2SyslogLevel(rec->severity),msg);
    }

  if (strmCfg->fp)  // If the file handle is non zero, write the log to that file.
    {
      fputs(msg,strmCfg->fp);
      // TO DO, need to fflush() this file handle after ALL the logs have been processed (do it in main outside of the loop).
    }
}

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.
void logInitializeStreams(LogCfg* cfg)
{
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  ClMgtObjectMap::iterator iter;
  ClMgtObjectMap::iterator end = cfg->streamConfig.end();
  for (iter = cfg->streamConfig.begin(); iter != end; iter++)
    {
        vector<ClMgtObject*> *objs = (vector<ClMgtObject*>*) iter->second;
        int temp = objs->size();
        for(int i = 0; i < temp; i++)
        {
          Stream* s = (Stream*) (*objs)[i];
          Dbg("Initializing stream %s file: %s location: %s\n", s->name.value().c_str(),s->fileName.value().c_str(),s->fileLocation.value().c_str());
          std::string& s = s->fileLocation.value();

          if (s[0] == ".")  // . for the location means 'this node'
            {
              
            }
        }      
    }
  
}

int main(int argc, char* argv[])
{
  // Load logging configuration
  LogCfg* cfg = loadLogCfg();
  // Initialize
  logInitializeSharedMem();

    // Hard code the well known streams
  appStreamCfg = (Stream*) cfg->streamConfig.getChildObject("app");
  sysStreamCfg = (Stream*) cfg->streamConfig.getChildObject("sys");

  logInitializeStreams(cfg);
  
  // Log processing Loop
  while(1)
    {
      int recnum;
      //clLogHeader->clientMutex.wait();
      clientMutex.lock();
      int numRecords = clLogHeader->numRecords;  // move the value out of shared memory into a signed value
      //clLogHeader->clientMutex.post();
      clientMutex.unlock();
      if (numRecords)
        {
          char *base    = (char*) clLogBuffer->get_address();
          LogBufferEntry* rec = static_cast<LogBufferEntry*>((void*)(((char*)base)+clLogBufferSize-sizeof(LogBufferEntry)));  // Records start at the end and go upwards 
          for (recnum=0;recnum<numRecords-1;recnum++, rec--)
            {
              if (rec->offset == 0) continue;  // Bad record
              postRecord(rec,((char*) base)+rec->offset,cfg);
              rec->offset = 0;
            }

          // Now handle a few things that can't be done while clients are touching the records.
          clientMutex.lock();

          // Now get the last record and any that may have been added during our processing, 
          numRecords = clLogHeader->numRecords;    
          for (;recnum<numRecords;recnum++, rec--)
            {
              if (rec->offset == 0) continue;  // Bad record
              postRecord(rec,((char*) base)+rec->offset,cfg);
              rec->offset = 0;
            }

          clLogHeader->numRecords = 0; // No records left in the buffer
          clLogHeader->msgOffset = sizeof(LogBufferHeader); // All log messages consumed, so reset the offset.
          clientMutex.unlock(); // OK let the log clients back in
        }

      // Wait for more records
      if (serverSem.timed_lock(cfg->serverConfig.processingInterval))
        {
          Dbg("timed_wait TRUE\n");
        }
      else
        {
          Dbg("timed_wait FALSE\n");
        }
    }
}
