#include <syslog.h>

#include <vector>
using namespace std;

#include <clMgtRoot.hxx>
#include "clMgtApi.hxx"
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clLogIpi.hxx>
#include <clMsgPortsAndTypes.hxx>

#include <ServerConfig.hxx>
#include <StreamConfig.hxx>
#include <FileFullAction.hxx>
#include <Stream.hxx>
#include <StreamScope.hxx>
#include <SAFplusLogModule.hxx>


#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
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

#define Dbg // printf

#include "logcfg.hxx"

using namespace SAFplusLog;

MgtModule dataModule("SAFplusLog");

static SAFplusLog::SAFplusLogModule* logcfg;
extern Stream* sysStreamCfg;
extern Stream* appStreamCfg;

extern void postRecord(LogBufferEntry* rec, char* msg, SAFplusLog::SAFplusLogModule* cfg);
extern void initializeStream(Stream* s);
extern void streamRotationInit(Stream* s);



extern HandleStreamMap hsMap;


void checkAndRotateLog(Stream* s)
{
  
  if ((s->lastUpdate < s->fileName.lastChange) || (s->lastUpdate < s->fileLocation.lastChange) || (s->fp == NULL))
    { // Something changed so close and reopen the files associated with this stream
      if (s->fp) { fclose(s->fp); s->fp = NULL; }
      initializeStream(s);
    }
  long fileSize = 0;
  if (s->fp) fileSize = ftell(s->fp);
  Dbg("checkAndRotateLog(): stream [%s] fileSize [%ld], conf fileSize [%ld]\n", s->tag.c_str(), (long int) fileSize, (long int) s->fileUnitSize.value);
  if (fileSize >= s->fileUnitSize.value)
  {
    if (s->fileFullAction == FileFullAction::ROTATE)
    {
      if (s->maximumFilesRotated < 1) s->maximumFilesRotated=1;  // Correct crazy values that could be set by operator
      std::string fname;
      while (s->numFiles >= s->maximumFilesRotated)
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

void finishLogProcessing(SAFplusLog::SAFplusLogModule* cfg)
  {
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->safplusLog.streamConfig.streamList.end();
  for (iter = cfg->safplusLog.streamConfig.streamList.begin(); iter != end; iter++)
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


void initializeLogRotation(SAFplusLog::SAFplusLogModule* cfg)
{ 
  MgtObject::Iterator iter; 
  MgtObject::Iterator end = cfg->safplusLog.streamConfig.streamList.end();  
  for (iter = cfg->safplusLog.streamConfig.streamList.begin(); iter != end; iter++)
  {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    streamRotationInit(s);    
  }
}

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.

void logInitializeStreams(SAFplusLog::SAFplusLogModule* cfg)
{
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  initializeLogRotation(cfg);  
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->safplusLog.streamConfig.streamList.end();
  for (iter = cfg->safplusLog.streamConfig.streamList.begin(); iter != end; iter++)
  {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    initializeStream(s);
  }
}

// Look at the log configuration and initialize temporary variables, open log files, etc based on the values.
void dumpStreams(SAFplusLog::SAFplusLogModule* cfg)
  {
  // Open FP if needed
  // if rotate
  // determine the current fileIdx by looking at the current files in the directory and adding one.
  MgtObject::Iterator iter;
  MgtObject::Iterator end = cfg->safplusLog.streamConfig.streamList.end();
  for (iter = cfg->safplusLog.streamConfig.streamList.begin(); iter != end; iter++)
    {
    Stream* s = dynamic_cast<Stream*>(iter->second);
    Dbg("Address %p\n", s);
    Dbg("  Stream %s file: %s location: %s\n", s->name.value.c_str(),s->fileName.value.c_str(),s->fileLocation.value.c_str());
    std::string& loc = s->fileLocation.value;
    }
  }
 

void logServerInitialize(SAFplus::MgtDatabase *db)
{
  // Load logging configuration
  
  logcfg = loadLogCfg(db);
  logcfg->bind(safplusMsgServer.handle,logcfg);

  // Initialize
  logInitializeSharedMem();

    // Hard code the well known streams
  appStreamCfg = dynamic_cast<Stream*> (logcfg->safplusLog.streamConfig.streamList["app"]);
  sysStreamCfg = dynamic_cast<Stream*> (logcfg->safplusLog.streamConfig.streamList["sys"]);

  logInitializeStreams(logcfg);
  dumpStreams(logcfg);  

  //SAFplus::logSeverity= LOG_SEV_MAX;  // DEBUG

#if 0 // TODO
  SAFplus::SYSTEM_CONTROLLER=true; // For testing
  IF_CLUSTERWIDE_LOG(LogSpooler logSpooler);
  if (SAFplus::SYSTEM_CONTROLLER) // If this node is system controller, then instantiate LogSpooler obj to listen logs from other nodes
  {     
    //logSpooler.subscribeAllStreams();
    //logSpooler.subscribeStream("app");
  }
#endif  
}


  // Log processing Loop
void logServerProcessing()
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
              //printf("log: %s\n", msg);
              postRecord(rec, msg, logcfg);
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
              //printf("log: %s\n", msg);
              postRecord(rec,msg,logcfg);
              // Forward logs to log subscribers
              IF_CLUSTERWIDE_LOG(logRep.logReplicate(rec, msg));
              rec->offset = 0;
            }

          clLogHeader->numRecords = 0; // No records left in the buffer
          clLogHeader->msgOffset = sizeof(LogBufferHeader); // All log messages consumed, so reset the offset.
          clientMutex.unlock(); // OK let the log clients back in
          finishLogProcessing(logcfg);
        }

      // Wait for more records
      if (serverSem.timed_lock(logcfg->safplusLog.serverConfig.processingInterval))
        {  // kicked awake
          Dbg("timed_wait TRUE\n");
        }
      else  // Timed out
        {
          //Dbg("timed_wait FALSE\n");
        }
    }

