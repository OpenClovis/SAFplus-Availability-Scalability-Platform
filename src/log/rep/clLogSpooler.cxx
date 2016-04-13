#include <clLogIpi.hxx>
#include <Stream.hxx>
#include "clLogSpooler.hxx"
#include "clLogRep.hxx"
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <Replicate.hxx>

using namespace SAFplusI;
using namespace SAFplus;

extern HandleStreamMap hsMap;
extern void postRecord(LogBufferEntry* rec, char* msg,SAFplusLog::SAFplusLogModule* cfg);
extern SAFplusLog::Stream* loadOrCreateNewStream(const char* streamName, SAFplusLog::Replicate repMode=SAFplusLog::Replicate::NONE, Handle strmHdl=INVALID_HDL);

LogSpooler::LogSpooler()
{
  try 
  {
    logSpoolerHdl = name.getHandle(LOG_SPOOLER_NAME);
  }
  catch(NameException& e) // LOG_SPOOLER_NAME is not registered with a handle, create one
  {
    logSpoolerHdl = Handle::create();
    name.set(LOG_SPOOLER_NAME, logSpoolerHdl, NameRegistrar::MappingMode::MODE_REDUNDANCY);
  }
}

void LogSpooler::subscribeStream(const char* streamName)
{
  Handle streamHdl; 
  try
  {
    streamHdl = name.getHandle(streamName);
  }
  catch (NameException& e)
  {
    printf("subscribeStream got exception [%s]\n", e.what());
    return;
  }
  SAFplusLog::Stream* s = hsMap[streamHdl];  
  if (!s)
  {
    printf("Stream object with stream name [%s] and handle [%" PRIx64 ":%" PRIx64 "] doesn't exist", streamName, streamHdl.id[0], streamHdl.id[1]);
    // if this is the system controller then load or create a new stream with the specified name
    if (SAFplus::SYSTEM_CONTROLLER)
    {
      s = loadOrCreateNewStream(streamName, SAFplusLog::Replicate::ANY, streamHdl);
    }
    else 
      printf("This node is not system controller. So canceling the subscribing\n");
  }
  if (s)
  {
    printf("Subscribing log spooler handle [%" PRIx64 ":%" PRIx64 "] to stream [%s]\n", logSpoolerHdl.id[0], logSpoolerHdl.id[1], streamName);
    objectMessager.insert(logSpoolerHdl, this);
    s->group.registerEntity(logSpoolerHdl, 1, Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  }
}

void LogSpooler::msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{  
  //printf("\n\nLogSpooler received msg from node [%d]; port [%u]\n\n", from.getNode(), from.getPort());
#if 0
  if (from.iocPhyAddress.nodeAddress == SAFplus::ASP_NODEADDR)
  {
    printf("This message was sent by myself. Bypassing it\n\n");
    return;
  }
#endif
  // Parse the message
  unsigned int headerLen = sizeof(LogMsgHeader);
  LogMsgHeader lh;
  memcpy(&lh, msg, headerLen);
  //printf("\n\nlogspooler msgHandler: logCount [%d]\n\n", lh.numLogs);
  char* pMsg = (char*)msg+headerLen;
  int shortLen = sizeof(short);
  for(int i=0;i<lh.numLogs;i++)
  {
    // Parsing the package following format: |header(LogMsgHeader)|logLen(short)|severity|logMsg||logLen(short)|severity|logMsg|...
    short logLen;
    memcpy(&logLen, pMsg, shortLen);
    pMsg+=shortLen;
    short severity;
    memcpy(&severity, pMsg, shortLen);
    pMsg+=shortLen;
    char log[logLen];
    memcpy(log, pMsg, logLen);
    LogBufferEntry rec;
    rec.severity = (LogSeverity) severity;
    rec.stream = lh.streamHandle;
    //printf("\n\n RECEIVED: logLen [%d]; severity [%d]; log [%s]\n\n", logLen, severity, log);
    // After parsing, we call postRecord to write those logs to the local file
    postRecord(&rec, log, NULL);
    pMsg+=logLen;
  }
}
