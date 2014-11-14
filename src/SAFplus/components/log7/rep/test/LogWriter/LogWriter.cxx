#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <FileFullAction.hxx>
#include <Stream.hxx>
#include <StreamScope.hxx>
#include <clNameApi.hxx>

using namespace SAFplus;
using namespace SAFplusLog;

extern Stream* createStreamCfg(const char* nam, const char* filename, const char* location, unsigned long int fileSize, unsigned long int logRecSize, SAFplusLog::FileFullAction fullAction, int numFilesRotate, int flushQSize, int flushInterval,bool syslog,SAFplusLog::StreamScope scope, Replicate repMode=Replicate::NONE, Handle strmHdl=INVALID_HDL);

void testLogWrite(const char* streamName)
{
  Stream* s = createStreamCfg(streamName, streamName, "/var/log/safplus", 32*1024*1024, 2048, FileFullAction::ROTATE, 10, 200, 500, false, StreamScope::GLOBAL, Replicate::ANY);
  if (!s)
  {
    printf("FAILED to create stream [%s]", streamName);
    return;
  }
  // get the stream handle which has been registered with Name when creating new stream config. Log will be written to this stream later
  Handle streamHdl;
  try
  {
    streamHdl = name.getHandle(streamName);
  }
  catch(SAFplus::NameException& e)
  {
    printf("testLogWrite got exception [%s]\n", e.what());
    return;
  } 
  logSeverity = LOG_SEV_MAX;
  printf("Writing log to stream [%s]; handle [0x.%x.0x%x]\n", streamName, streamHdl.id[0], streamHdl.id[1]);
  while(true)
  {
    // Write log to the user stream
    appLog(streamHdl,SAFplus::LOG_SEV_NOTICE, 0, "APPTEST", "LOGWR", "This is the test line");
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
  }  
}

int main(int argc, char* argv[])
{  
  SAFplus::ASP_NODEADDR = 7;
  safplusInitialize(SAFplus::LibDep::NAME);
  const char* streamName="myapp";
  testLogWrite(streamName);  
}


