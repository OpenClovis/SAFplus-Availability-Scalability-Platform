#ifndef LOGREADER_HXX_
#define LOGREADER_HXX_

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <clHandleApi.hxx>
#include <Stream.hxx>
#include <clObjectMessager.hxx>
#include <clNameApi.hxx>
#include <clLogRep.hxx>
#include <Replicate.hxx>

using namespace SAFplus;
using namespace SAFplusLog;

extern SAFplusLog::Stream* loadOrCreateNewStream(const char* streamName, Replicate repMode=Replicate::NONE, Handle strmHdl=INVALID_HDL);

class LogReader: public SAFplus::MsgHandler
{
protected:
  SAFplus::Handle logReaderHdl;
public:
  LogReader()
  {
    logReaderHdl = SAFplus::Handle::create();
  }
  // This function is for user to subscribe a stream (by streamName) so that he can receive logs from this stream
  void subscribeStream(const char* streamName)
  {
    SAFplus::Handle streamHdl = INVALID_HDL;
    // try to get the stream handle if it was registered before. This handle may exist, it might be registered by LogWriter app
    try
    {
      streamHdl = name.getHandle(streamName);
    }
    catch(SAFplus::NameException& e)
    {
      printf("LogReader subscribeStream got exception [%s]. Use a default handle\n", e.what());      
    }
    // Load the stream from MGT database, if it doesn't exist, create new stream with stream name and handle
    SAFplusLog::Stream* s = loadOrCreateNewStream(streamName, Replicate::ANY, streamHdl);
    if (!s)
    {
      printf("Couldn't load or create stream object with streamName [%s]\n", streamName);
      return;
    }
    // Add me to the group so that I can receive logs
    objectMessager.insert(logReaderHdl, this);
    s->group.registerEntity(logReaderHdl, 1, Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  }
  virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
    printf("\n\nLogSpooler received msg from node [%d]; port [%u]\n\n", from.iocPhyAddress.nodeAddress, from.iocPhyAddress.portId);
  // Parse the message
    unsigned int headerLen = sizeof(LogMsgHeader);
    LogMsgHeader lh;
    memcpy(&lh, msg, headerLen);
    printf("\n\nlogspooler msgHandler: logCount [%d]\n\n", lh.numLogs);
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
      char log[logLen+1];
      memcpy(log, pMsg, logLen);
      log[logLen] = 0;
      // print on screen
      printf("\n\n RECEIVED: logLen [%d]; severity [%d]; log [%s]\n\n", logLen, severity, log);      
      pMsg+=logLen;
    }
  }
};
#endif // LOGREADER_HXX_

int main()
{
  SAFplus::ASP_NODEADDR = 7;
  safplusInitialize(SAFplus::LibDep::NAME);
  LogReader logReader;
  logReader.subscribeStream("myapp");
  while(true)
  {
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
  }
}
