#include <syslog.h>

#include <vector>
using namespace std;

#include <clLogIpi.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <clGlobals.hxx>
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

  strmCfg->dirty = true;  // We wrote something to this stream
  
  if (strmCfg->syslog)  // output this log to syslog
    {
      syslog(logLevel2SyslogLevel(rec->severity),msg);
    }

  if (strmCfg->fp)  // If the file handle is non zero, write the log to that file.
    {
      strmCfg->fileBuffer += msg;
      strmCfg->fileBuffer += "\n";
    }
  if (strmCfg->sendMsg)
    {
      strmCfg->msgBuffer += msg;
      strmCfg->msgBuffer += "\n";
    }
}

void finishLogProcessing(LogCfg* cfg)
  {
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
    {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    if (s->dirty)
      {
      if (s->fp)
        {
        //int sz = s->fileBuffer.size();
        //boost::asio::streambuf::const_buffers_type bufs = s->fileBuffer.data();
        //assert(sz == boost::asio::buffer_size(bufs));  // If the whole size is not the same as this one buffer I am confused about the API
        //fwrite(boost::asio::buffer_cast<const char*>(bufs),sizeof(char),boost::asio::buffer_size(bufs),s->fp);
        s->fileBuffer.fwrite(s->fp);                    
        fflush(s->fp);
        s->fileBuffer.consume();
        }
      if (s->sendMsg)
        {
        // TODO: read checkpoint to determine who wants to hear about these logs and send the log message buffer to them.
        s->msgBuffer.consume();
        }
  
      s->dirty = false;
      }
          
    }  
  }

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.
void logInitializeStreams(LogCfg* cfg)
  {
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
    {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    Dbg("Initializing stream %s file: %s location: %s\n", s->name.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str());
    std::string& loc = s->fileLocation.value;

    if ((loc[0] == '.')&&(loc[1] == ':'))  // . for the location means 'this node'
      {
      s->fileIdx = 0;  // TODO:  this index should be initialized by looking at log files currently in the directory and their modification dates.
      std::string fname(loc.substr(2,-1) +  "/" + s->fileName.value + boost::lexical_cast<std::string>(s->fileIdx) + ".log");
      s->fp = fopen(fname.c_str(),"a");
      Dbg("Opening file: %s %s\n", fname.c_str(), (s->fp) ? "OK":"FAILED");
              
      }
            
    }
  
  }

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.
void dumpStreams(LogCfg* cfg)
  {
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
    {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    Dbg("Address %p\n", s);
    Dbg("  Stream %s file: %s location: %s\n", s->name.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str());
    std::string& loc = s->fileLocation.value;
           
    }
  
  }

int main(int argc, char* argv[])
{
  /* Initialize mgt database  */
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  db->initializeDB("SAFplusLog");

  // Load logging configuration
  LogCfg* cfg = loadLogCfg();
  // Initialize
  logInitializeSharedMem();

    // Hard code the well known streams
  appStreamCfg = dynamic_cast<Stream*> (cfg->streamConfig.streamList["app"]);
  sysStreamCfg = dynamic_cast<Stream*> (cfg->streamConfig.streamList["sys"]);

  logInitializeStreams(cfg);
  dumpStreams(cfg);  

  SAFplus::logSeverity= LOG_SEV_MAX;  // DEBUG

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
          finishLogProcessing(cfg);
        }

      // Wait for more records
      if (serverSem.timed_lock(cfg->serverConfig.processingInterval))
        {  // kicked awake
          Dbg("timed_wait TRUE\n");
        }
      else  // Timed out
        {
          //Dbg("timed_wait FALSE\n");
        }
    }
}
