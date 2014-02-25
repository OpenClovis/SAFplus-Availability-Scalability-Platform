
#include "logcfg.hxx"
using namespace SAFplusLog;

LogCfg::LogCfg():ClMgtObject("SAFplusLog")
{
  this->addChildObject(&serverConfig, "serverConfig");
  this->addChildObject(&streamConfig, "streamConfig");
}

LogCfg logcfg;


Stream* createStreamCfg(const char* name, const char* filename, const char* location, unsigned long int fileSize, unsigned long int logRecSize, SAFplusLog::FileFullActionOption fullAction, int numFilesRotate, int flushQSize, int flushInterval,bool syslog,SAFplusLog::StreamScopeOption scope)
{
  Stream* s = new Stream();
  s->setName(name);
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
  logcfg.streamConfig.load();  // Load up all children of streamConfig (recursively) from the DB

  Stream* s =  (Stream*)logcfg.streamConfig.getChildObject("sys");
  if (!s)  // The sys log is an Openclovis system log.  So if its config does not exist, or was deleted, recreate the log.
    {
      Stream* s = createStreamCfg("sys","sys",".:var/log",32*1024*1024, 2048, SAFplusLog::ROTATE, 10, 200, 500, false, SAFplusLog::GLOBAL);
      logcfg.streamConfig.addChildObject(s,"sys");
    }

  s =  (Stream*)logcfg.streamConfig.getChildObject("app");
  if (!s)  // The all log is an Openclovis system log.  So if its config does not exist, or was deleted, recreate the log.
    {
      s = createStreamCfg("app","app",".:var/log",32*1024*1024, 2048, SAFplusLog::ROTATE, 10, 200, 500, false, SAFplusLog::GLOBAL);
      logcfg.streamConfig.addChildObject(s,"app");
    }

  return &logcfg;
}
