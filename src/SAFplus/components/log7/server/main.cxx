#include <syslog.h>

#include <vector>
using namespace std;

#include <clLogIpi.hxx>
#include <clIocPortList.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/filesystem.hpp"
#include <boost/container/map.hpp>
#include <boost/lexical_cast.hpp>
#include <clGlobals.hxx>
using namespace SAFplus;
using namespace SAFplusI;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

#define Dbg printf

#include "logcfg.hxx"

using namespace SAFplusLog;

Stream* sysStreamCfg;
Stream* appStreamCfg;

extern ClBoolT   gIsNodeRepresentative;

typedef boost::container::multimap<std::time_t, fs::path> file_result_set_t;

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
  file_result_set_t file_result_set;
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
    {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    Dbg("Initializing stream %s file: %s location: %s\n", s->name.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str());
    std::string& loc = s->fileLocation.value;

    if ((loc[0] == '.')&&(loc[1] == ':'))  // . for the location means 'this node'
      {
        std::string pathToFile = loc.substr(2,-1);
        if (pathToFile[0] != '/')
        {
          Dbg("path [%s] is invalid. Try to use ASP_LOGDIR\n", pathToFile.c_str());
          pathToFile = SAFplus::ASP_LOGDIR;
        }
        if (!fs::exists(pathToFile))
        {    
          try {
            fs::create_directories(pathToFile);     
          }
          catch (boost::filesystem::filesystem_error ex) {
            Dbg("path [%s] is invalid; ASP_LOGDIR may not be set. System error [%s]\n", pathToFile.c_str(), ex.what());
            return;
          }
        }
        unsigned fileIdx = 0;
        unsigned lastSlashPos,extLogPos,fnIdx;
        std::string strIdx;
        unsigned foundIdx;
        if (s->fileFullAction == FileFullAction::ROTATE || s->fileFullAction == FileFullAction::WRAP)
        {
          if (fs::exists(pathToFile) && fs::is_directory(pathToFile))
          {
            fs::directory_iterator end_iter;
            for( fs::directory_iterator dir_iter(pathToFile) ; dir_iter != end_iter ; ++dir_iter)
            {
              lastSlashPos = dir_iter->path().string().rfind("/");
              extLogPos = dir_iter->path().string().rfind(".log");
              fnIdx = dir_iter->path().string().find(s->fileName.value, lastSlashPos);
              if (fnIdx >= 0 && fnIdx < dir_iter->path().string().length())
              {
                strIdx = dir_iter->path().string().substr(fnIdx+s->fileName.value.length(), extLogPos-(fnIdx+s->fileName.value.length()));
              }
              if (strIdx.length()>0)
              {
                try 
                {
                  fileIdx = boost::lexical_cast<unsigned>(strIdx);
                  if (fs::is_regular_file(dir_iter->status()))
                  {
                    file_result_set.insert(file_result_set_t::value_type(fs::last_write_time(dir_iter->path()), dir_iter->path()));
                  }
                }
                catch(...)
                {
                  Dbg("[%s] is not safplus log, ignore it\n", dir_iter->path().string().c_str());
                }
                strIdx.clear();
              }              
            }
#if 0 // for debugging
            {
             file_result_set_t::iterator it;
             for (it=file_result_set.begin(); it!=file_result_set.end(); ++it)
             std::cout << (*it).first << " => " << (*it).second << '\n';
            }
#endif
            if (s->fileFullAction == FileFullAction::ROTATE)
            {         
              // get the oldest modified file               
              if (file_result_set.size() >= s->maximumFilesRotated)              
              {
                file_result_set_t::iterator it = file_result_set.begin();
                // Delete this oldest file
                Dbg("Deleting file [%s]\n", (*it).second.string().c_str());
                fs::remove((*it).second);                                              
              }             
            }
            // get the last modified file            
            if (file_result_set.size()>0)
            {
              file_result_set_t::reverse_iterator rit = file_result_set.rbegin();
              std::string temp = (*rit).second.string();
              foundIdx = temp.rfind(s->fileName.value);
              if (foundIdx>=0 && foundIdx<temp.length())
              {                
                lastSlashPos = temp.rfind("/");
                extLogPos = temp.rfind(".log");
                fnIdx = temp.find(s->fileName.value, lastSlashPos);
                if (fnIdx >= 0 && fnIdx < temp.length())
                {
                  strIdx = temp.substr(fnIdx+s->fileName.value.length(), extLogPos-(fnIdx+s->fileName.value.length()));
                }
                fileIdx = atoi(strIdx.c_str());  
                if (s->fileFullAction == FileFullAction::ROTATE)
                {
                  fileIdx++;
                }
              }
             }
            }            
          }        
          else if (s->fileFullAction == FileFullAction::HALT)
          {
            // HALT means stopping logging to file => do not open fp => Nothing to do
          }
          s->fileIdx = fileIdx;
          std::string fname(pathToFile +  "/" + s->fileName.value + boost::lexical_cast<std::string>(s->fileIdx) + ".log");
          if (s->fileFullAction == FileFullAction::ROTATE)
          {
            s->fp = fopen(fname.c_str(),"a+");
          }
          else if (s->fileFullAction == FileFullAction::WRAP)
          {
            if (fs::exists(fname))
            {
              s->fp = fopen(fname.c_str(),"r+");
              fseek(s->fp, 0L, SEEK_SET);
            }
            else
            {
              s->fp = fopen(fname.c_str(),"w");
            } 
          }
          Dbg("Opening file: %s %s\n", fname.c_str(), (s->fp) ? "OK":"FAILED");
          if (!s->fp)
          {
            Dbg("Opening file: %s FAILED. Errno [%d]. Error message [%s]\n", fname.c_str(), errno, strerror(errno));
            //exit(1);
          }   
      } 
      // reset the map for processing other log file
      file_result_set.clear();           
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
  gIsNodeRepresentative = CL_TRUE;

  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  SAFplus::ASP_NODEADDR = 0x1;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::LOG_IOC_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 10;

  safplusInitialize(SAFplus::LibDep::MSG, sic);

  /* Initialize mgt database  */
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  db->initializeDB("SAFplusLog");

  MgtModule         dataModule("SAFplusLog");
  dataModule.loadModule();
  dataModule.initialize();

  // Load logging configuration
  LogCfg* cfg = loadLogCfg();
  SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
  cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
  cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");
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
