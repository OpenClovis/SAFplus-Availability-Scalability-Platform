#include <syslog.h>

#include <vector>
using namespace std;

#include <clMgtRoot.hxx>
#include "clMgtApi.hxx"
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clLogIpi.hxx>
#include <clMsgPortsAndTypes.hxx>
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

SAFplusLog::SAFplusLogRoot* cfg;
extern Stream* sysStreamCfg;
extern Stream* appStreamCfg;

extern void postRecord(LogBufferEntry* rec, char* msg, SAFplusLog::SAFplusLogRoot* cfg);
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
        assert(0); // Nobody should call this because of the checkpoint
//        SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
//        cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
//        cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");
      }
      else
      {
        MgtRoot::getInstance()->mgtMessageHandler.msgHandler(from, svr, msg, msglen, cookie);
      }
    }

};

extern HandleStreamMap hsMap;


void checkAndRotateLog(Stream* s)
{
  long fileSize = ftell(s->fp);
  Dbg("checkAndRotateLog(): fileSize [%ld], conf fileSize [%ld]\n", (long int) fileSize, (long int) s->fileUnitSize.value);
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

void finishLogProcessing(SAFplusLog::SAFplusLogRoot* cfg)
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


void initializeLogRotation(SAFplusLog::SAFplusLogRoot* cfg)
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

void logInitializeStreams(SAFplusLog::SAFplusLogRoot* cfg)
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
void dumpStreams(SAFplusLog::SAFplusLogRoot* cfg)
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

  // Load logging configuration
  cfg = loadLogCfg();
  SAFplus::Handle hdl = SAFplus::Handle::create(SAFplusI::LOG_IOC_PORT);
  cfg->bind(hdl);

//  cfg->streamConfig.bind(hdl, "SAFplusLog", "/StreamConfig");
//  cfg->serverConfig.bind(hdl, "SAFplusLog", "/ServerConfig");

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

#if 0 // TODO
  SAFplus::SYSTEM_CONTROLLER=true; // For testing
  IF_CLUSTERWIDE_LOG(LogSpooler logSpooler);
  if (SAFplus::SYSTEM_CONTROLLER) // If this node is system controller, then instantiate LogSpooler obj to listen logs from other nodes
  {     
    //logSpooler.subscribeAllStreams();
    //logSpooler.subscribeStream("app");
  }
#endif  

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
