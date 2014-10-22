#include <syslog.h>

#include <vector>
using namespace std;

#include "client/MgtMsg.pb.hxx"
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
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

MgtModule dataModule("SAFplusLog");
LogCfg* cfg;
Stream* sysStreamCfg;
Stream* appStreamCfg;

class MgtMsgHandler : public SAFplus::MsgHandler
{
  public:
    virtual void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      Mgt::Msg::MsgMgt mgtMsgReq;
      mgtMsgReq.ParseFromArray(msg, msglen);
      if (mgtMsgReq.type() == Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND_REQUEST && cfg)
      {
        SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
        cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
        cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");
      }
    }

};

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

void checkAndRotateLog(Stream* s)
{
  long fileSize = ftell(s->fp);
  Dbg("checkAndRotateLog(): fileSize [%ld], conf fileSize [%ld]\n", fileSize, s->fileUnitSize.value);
  if (fileSize >= s->fileUnitSize.value)
  {
    if (s->fileFullAction == FileFullAction::ROTATE)
    {
      std::string fname;
      if (s->numFiles >= s->maximumFilesRotated)
      {
        fname.assign(s->filePath +  "/" + s->fileName.value + boost::lexical_cast<std::string>(s->earliestIdx) + ".log");
        // Delete the oldest file
        // As a result of test, if the file name doesn't exist, fs::remove doesn't throw any exception or error
        // So, does not need a try catch here
        fs::remove(fname);   
        s->earliestIdx++; // Increasing earliest file index
        s->numFiles--; // Decreasing number of files because one file has been removed            
      }
      // close the current file to open a new file
      fclose(s->fp);    
      s->fp = NULL;
      // open a new file with the newest index
      s->fileIdx++; // Increasing new file index
      fname.assign(s->filePath +  "/" + s->fileName.value + boost::lexical_cast<std::string>(s->fileIdx) + ".log");      
      s->fp = fopen(fname.c_str(),"w");
      Dbg("Opening file: %s %s", fname.c_str(), (s->fp) ? "OK":"FAILED");
      if (s->fp)
      {
        // Increasing the number of files
        s->numFiles++;
        Dbg("\n");
      }
      else
      {
        Dbg("Failure opening file [%s]. Errno [%d]. Error message [%s]\n", fname.c_str(), errno, strerror(errno));
      }
    }  
    else if (s->fileFullAction == FileFullAction::WRAP)
    {
      // Move the file pointer to the beginning of the file
      fseek(s->fp, 0L, SEEK_SET);         
    }
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
        checkAndRotateLog(s);
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

int getFileIdx(std::string filePath, std::string fileName)
{
  int fileIdx = 0;
  int lastSlashPos,extLogPos,fnIdx;
  std::string strIdx;  
  int foundIdx = filePath.rfind(fileName);
  if (foundIdx>=0 && foundIdx<filePath.length())
  {                
    lastSlashPos = filePath.rfind("/");
    extLogPos = filePath.rfind(".log");
    fnIdx = filePath.find(fileName, lastSlashPos);
    if (fnIdx >= 0 && fnIdx < filePath.length())
    {
      strIdx = filePath.substr(fnIdx+fileName.length(), extLogPos-(fnIdx+fileName.length()));
    }
    fileIdx = atoi(strIdx.c_str());      
  }
  return fileIdx;
}

void initializeLogRotation(LogCfg* cfg)
{ 
  MgtObject::Iterator iter; 
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  file_result_set_t file_result_set;
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
  {
    Stream* s = dynamic_cast<Stream*>(iter->second);

    string fpath = s->fileLocation.value;
    if (fpath[0] != '/')  // If the location begins with a /, it is an absolute directory.  
    { // If it is a relative directory, prepend ASP_LOGDIR
      Dbg("path [%s] is invalid. Trying to use ASP_LOGDIR or current directory\n", fpath.c_str());
      if (SAFplus::ASP_LOGDIR[0] != 0)
        s->filePath = SAFplus::ASP_LOGDIR;
      else
        {
        char tmp[1024];
        s->filePath = getcwd(tmp,1024);
        }
      if ((s->filePath.length()>0)&& (s->filePath.at( s->filePath.length() - 1 ) != '/')) s->filePath.append("/");  // Be tolerant of filepath ending in / or not.
      s->filePath.append(fpath);
    }
    else
    {
      s->filePath = fpath;
    }

    // Now make sure that the log directory exists.  If it does not exist, create it.  If it cannot be created, give up logging to this stream.
    std::string& pathToFile = s->filePath;
    if (!fs::exists(pathToFile))
    {
      try
        {
        fs::create_directories(pathToFile);
        }
      catch (boost::filesystem::filesystem_error ex)
        {
        Dbg("path [%s] is invalid; ASP_LOGDIR may not be set. System error [%s]\n", pathToFile.c_str(), ex.what());
        s->fp = NULL;
        s->fileIdx = -1;
        s->earliestIdx = -1;
        s->numFiles = -1;
        continue; // if this filepath is invalid, pass it by and continue handling other streams
        }
    }

    /* The purpose is to find the last modified log file, get its index then calculate the next index for a new file.  

       First we need to separate out the log files that are associated with this stream (verses other streams or random files).  Logs associated with this stream have a name with the following format <pathname><fileName><Index>.log.  We search the directory for all filenames matching this format.  Next, these files are put in the file_result_set.  This map will sort all elements automatically based on modified time.  We use this set to do things like delete the oldest file and create a new one.
    */
    int fileIdx = 0;
    int lastSlashPos,extLogPos,fnIdx;
    std::string strIdx;
    int foundIdx;

    if (s->fileFullAction == FileFullAction::ROTATE || s->fileFullAction == FileFullAction::WRAP)
      {
        fs::directory_iterator end_iter;
        for( fs::directory_iterator dir_iter(pathToFile) ; dir_iter != end_iter ; ++dir_iter)
        {
          if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.  These can't be log files.

          // Extract just the file name by bracketing it between the preceding / and the file extension
          std::string filePath = dir_iter->path().string();
          // skip ahead to the last / so a name like /var/log/log1.log works
          lastSlashPos = filePath.rfind("/");
          fnIdx = filePath.find(s->fileName, lastSlashPos);
          if (fnIdx >= 0 && fnIdx < filePath.length())
          {
            extLogPos = filePath.rfind(".log"); // Find the extension because the file index is bracketed between the fnIdx and the extension.
            strIdx = filePath.substr(fnIdx+s->fileName.value.length(), extLogPos-(fnIdx+s->fileName.value.length()));
          }
          if (strIdx.length()>0)  // The file has an index
          {
            try 
            {
              fileIdx = boost::lexical_cast<unsigned>(strIdx);
              file_result_set.insert(file_result_set_t::value_type(fs::last_write_time(dir_iter->path()), dir_iter->path()));
            }
            catch(...)
            {
              Dbg("[%s] is not safplus log, ignore it\n", filePath.c_str());
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

      s->numFiles = file_result_set.size();
      if (s->numFiles > 0)
        {
        file_result_set_t::iterator it = file_result_set.begin();
        s->earliestIdx = getFileIdx((*it).second.string(), s->fileName);
        file_result_set_t::reverse_iterator rit = file_result_set.rbegin();
        std::string temp = (*rit).second.string();
        s->fileIdx = getFileIdx(temp, s->fileName);
        }
      

#if 0 // REMOVE THIS: Deleting files, etc should be done whenever a new log file needs to be created.  It is not necessary to do it upon initialization
        s->numFiles = file_result_set.size();
        if (s->fileFullAction == FileFullAction::ROTATE && s->numFiles >= s->maximumFilesRotated)
        {
          file_result_set_t::iterator it = file_result_set.begin();
          s->earliestIdx = getFileIdx((*it).second.string(), s->fileName);
          // Delete this oldest file          
          Dbg("Deleting file [%s]\n", (*it).second.string().c_str());
          fs::remove((*it).second);
          s->earliestIdx++;
          s->numFiles--;
        } 
        else if (s->fileFullAction == FileFullAction::HALT)
        {
          // HALT means stopping logging to file => do not open fp => Nothing to do
        }          
        // get the last modified file            
        if (s->numFiles>0)
        {
          file_result_set_t::reverse_iterator rit = file_result_set.rbegin();
          std::string temp = (*rit).second.string();
          s->fileIdx = getFileIdx(temp, s->fileName);
          if (s->fileFullAction == FileFullAction::ROTATE)
          {
            s->fileIdx++;  // NO we don't want to create a new file every time we initialize.  Is that how it works other standard logs?  syslog, apache logs, etc?
          }
        }
        else
        {       
          s->fileIdx = 0;
          s->earliestIdx = 0;
        }
#endif
    }
    // reset the map for processing other log file
    file_result_set.clear();
  }
}

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.
void logInitializeStreams(LogCfg* cfg)
{
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  initializeLogRotation(cfg);  
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
  {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    Dbg("Initializing stream %s file: %s location: %s\n", s->name.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str()); 

    if (s->filePath.length() > 0)
    {
      std::string fname(s->filePath +  "/" + s->fileName.value + boost::lexical_cast<std::string>(s->fileIdx) + ".log");

      // Due to a strange quirk of C, opening a file with mode "a" causes fseek to never work (write pointer is ALWAYS at the end of the file).
      // So we have to open the file using this 2 step mechanism.
      if (fs::exists(fname))
        {
        s->fp = fopen(fname.c_str(),"r+");
        fseek(s->fp, 0L, SEEK_END);  // Append new logs onto the end of the file
        }
      else
        {
        s->fp = fopen(fname.c_str(),"w");
        if (s->fp)
          {
          s->numFiles++;
          }
        }

      Dbg("Opening file: %s %s\n", fname.c_str(), (s->fp) ? "OK":"FAILED");
      if (!s->fp)
      {
        Dbg("Opening file: %s FAILED. Errno [%d]. Error message [%s]\n", fname.c_str(), errno, strerror(errno));
      }
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
  gIsNodeRepresentative = CL_FALSE;

  SAFplus::ASP_NODEADDR = 0x1;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::LOG_IOC_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 10;

  safplusInitialize(SAFplus::LibDep::MSG, sic);

  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  /* Initialize mgt database  */
  ClMgtDatabase *db = ClMgtDatabase::getInstance();
  db->initializeDB("SAFplusLog");

  dataModule.loadModule();
  dataModule.initialize();

  // Load logging configuration
  cfg = loadLogCfg();
  SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
  cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
  cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");

  MgtMsgHandler msghandle;
  SAFplus::SafplusMsgServer* mgtIocInstance = &safplusMsgServer;
  mgtIocInstance->registerHandler(SAFplusI::CL_MGT_MSG_TYPE,&msghandle,NULL);

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
