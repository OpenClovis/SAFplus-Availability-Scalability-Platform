#include <syslog.h>

#include <vector>
using namespace std;

#include <clMgtRoot.hxx>
#include "MgtMsg.pb.hxx"
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clLogIpi.hxx>
#include <clIocPortList.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/container/map.hpp>
#include <boost/lexical_cast.hpp>
#include <clGlobals.hxx>
#include <Replicate.hxx>
#ifdef SAFPLUS_CLUSTERWIDE_LOG
#include "../rep/clLogRep.hxx"
#include "../rep/clLogSpooler.hxx"
#endif
using namespace SAFplus;
using namespace SAFplusI;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

#define Dbg printf

#include "logcfg.hxx"

using namespace SAFplusLog;

MgtModule dataModule("SAFplusLog");
LogCfg* cfg;
extern Stream* sysStreamCfg;
extern Stream* appStreamCfg;

extern void postRecord(LogBufferEntry* rec, char* msg,LogCfg* cfg);
extern void initializeStream(Stream* s);
extern void streamRotationInit(Stream* s);

class MgtMsgHandler : public SAFplus::MsgHandler
{
  public:
    virtual void msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
      Mgt::Msg::MsgMgt mgtMsgReq;
      mgtMsgReq.ParseFromArray(msg, msglen);
      if (mgtMsgReq.type() == Mgt::Msg::MsgMgt::CL_MGT_MSG_BIND_REQUEST && cfg)
      {
        SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
        cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
        cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");
      }
      else
      {
        MgtRoot::getInstance()->mgtMessageHandler.msgHandler(from, svr, msg, msglen, cookie);
      }
    }

};

extern ClBoolT   gIsNodeRepresentative;
extern HandleStreamMap hsMap;

#if 0
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
#endif
/*
On receiving log from other node, call this function with logRecv=true.
logRecv parameter indicates that this log is received from other nodes (true).
if logRecv==true, log must be written to this node AND do not forward it to others, 
otherwise, write to this node AND forward it to others
*/
#if 0
void postRecord(LogBufferEntry* rec, char* msg,LogCfg* cfg)
{
  if (rec->severity > SAFplus::logSeverity) return;  // don't log if the severity cutoff is lower than that of the log.  Note that the client also does this check.
  //printf("%s\n",msg);

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
      // lookup the handle in the hastable to find the stream
      Dbg("Dynamic stream: handle gotten from hashmap\n");
      strmCfg = hsMap[rec->stream];
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

  if (strmCfg->fp)  // If the file handle is non zero, write the log to that file
    {
      printf("DEBUG: %s\n",msg);  
      printf("DEBUG: msgLen [%d]\n",strlen(msg));  
      strmCfg->fileBuffer += msg;
      strmCfg->fileBuffer += "\n";
    }
  if (strmCfg->sendMsg)
    {
      strmCfg->msgBuffer += msg;
      strmCfg->msgBuffer += "\n";
    }
#if 0
  if ((strmCfg->replicate != Replicate::NONE) && (!logRecv) && (lastRec)) // This log is my own log and there is no record in our processing, so forward them to others
  {
    Dbg("Replicate to other nodes\n");
    logRep.logReplicate(rec->stream);
  }
#endif
}
#endif

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
        // Forward remaining logs to others if any
        IF_CLUSTERWIDE_LOG(logRep.flush(s));
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


void initializeLogRotation(LogCfg* cfg)
{ 
  MgtObject::Iterator iter; 
  MgtObject::Iterator end = cfg->streamConfig.streamList.end();  
  for (iter = cfg->streamConfig.streamList.begin(); iter != end; iter++)
  {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    streamRotationInit(s);    
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
    initializeStream(s);
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
    Dbg("  Stream %s file: %s location: %s\n", s->name.value.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str());
    std::string& loc = s->fileLocation.value;
    }
  }

int main(int argc, char* argv[])
{
  //gIsNodeRepresentative = CL_FALSE;

  SAFplus::ASP_NODEADDR = 0x7;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = SAFplusI::LOG_IOC_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 10;

  safplusInitialize(SAFplus::LibDep::MSG|SAFplus::LibDep::GRP|SAFplus::LibDep::NAME, sic);

  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_MAX;

  /* Initialize mgt database  */
  MgtDatabase *db = MgtDatabase::getInstance();
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

  SAFplus::SYSTEM_CONTROLLER=true; // For testing
  IF_CLUSTERWIDE_LOG(LogSpooler logSpooler);
  if (SAFplus::SYSTEM_CONTROLLER) // If this node is system controller, then instantiate LogSpooler obj to listen logs from other nodes
  {     
    //logSpooler.subscribeAllStreams();
    //logSpooler.subscribeStream("app");
  }
  
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
              char* msg = ((char*) base)+rec->offset;
              postRecord(rec, msg, cfg);
              // Forward logs to log subscribers
              IF_CLUSTERWIDE_LOG(logRep.logReplicate(rec, msg));
              rec->offset = 0;
            }

          // Now handle a few things that can't be done while clients are touching the records.
          clientMutex.lock();

          // Now get the last record and any that may have been added during our processing, 
          numRecords = clLogHeader->numRecords;    
          for (;recnum<numRecords;recnum++, rec--)
            {
              if (rec->offset == 0) continue;  // Bad record
              char* msg = ((char*) base)+rec->offset;
              postRecord(rec,msg,cfg);
              // Forward logs to log subscribers
              IF_CLUSTERWIDE_LOG(logRep.logReplicate(rec, msg));
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
