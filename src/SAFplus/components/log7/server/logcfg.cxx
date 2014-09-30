
#include "logcfg.hxx"
using namespace SAFplusLog;
using namespace SAFplus;

LogCfg::LogCfg():MgtContainer("SAFplusLog")
{
  this->addChildObject(&serverConfig, "serverConfig");
  this->addChildObject(&streamConfig, "streamConfig");
}

LogCfg logcfg;


Stream* createStreamCfg(const char* nam, const char* filename, const char* location, unsigned long int fileSize, unsigned long int logRecSize, SAFplusLog::FileFullAction fullAction, int numFilesRotate, int flushQSize, int flushInterval,bool syslog,SAFplusLog::StreamScope scope)
{
  Stream* s = new Stream();
  s->setMyName(nam);
  s->setFileName(filename);
  s->setFileLocation(location);
  s->fileUnitSize = fileSize;
  s->recordSize = logRecSize;
  s->setFileFullAction(fullAction);
  s->maximumFilesRotated = numFilesRotate;
  s->flushFreq = flushQSize;
  s->flushInterval = flushInterval;
  s->syslog = syslog;
  s->setStreamScope(scope);
  return s;
}

/* Initialization code would load the configuration from the database rather than setting it by hand.
 */
LogCfg* loadLogCfg()
{
  logcfg.streamConfig.read();  // Load up all children of streamConfig (recursively) from the DB

  Stream* s =  dynamic_cast<Stream*>(logcfg.streamConfig.streamList.getChildObject("sys"));
  if (!s)  // The sys log is an Openclovis system log.  So if its config does not exist, or was deleted, recreate the log.
    {
      s = createStreamCfg("sys","sys",".:var/log",32*1024*1024, 2048, FileFullAction::ROTATE, 10, 200, 500, false, StreamScope::GLOBAL);
      std::string cfgName("sys");
      logcfg.streamConfig.streamList.addChildObject(s,cfgName);
    }

  s =  dynamic_cast<Stream*>(logcfg.streamConfig.streamList.getChildObject("app"));
  if (!s)  // The all log is an Openclovis system log.  So if its config does not exist, or was deleted, recreate the log.
    {
      s = createStreamCfg("app","app",".:var/log",32*1024*1024, 2048, FileFullAction::ROTATE, 10, 200, 500, false, StreamScope::GLOBAL);
      std::string cfgName("app");
      logcfg.streamConfig.streamList.addChildObject(s,cfgName);
    }

  return &logcfg;
}
