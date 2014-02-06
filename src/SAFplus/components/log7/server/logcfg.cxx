
#include "logcfg.hxx"
using namespace SAFplusLog;

LogCfg::LogCfg():ClMgtObject("log")
{
  this->addChildObject(&serverConfig, "serverConfig");
  this->addChildObject(&streamConfig, "streamConfig");
}

LogCfg logcfg;


Stream* createStreamCfg(const char* name, const char* filename, const char* location, unsigned long int fileSize, unsigned long int logRecSize, const char* fullAction, int numFilesRotate, int flushQSize, int flushInterval,bool syslog,const char* scope)
{
  Stream* s = new Stream();
  s->setNameValue(name);
  s->setFileNameValue(filename);
  s->setFileLocationValue(location);
  s->fileUnitSize = fileSize;
  s->recordSize = logRecSize;
  s->setFileFullActionValue(fullAction);
  s->maximumFilesRotated = numFilesRotate;
  s->flushFreq = flushQSize;
  s->flushInterval = flushInterval;
  s->syslog = syslog;
  s->setStreamScopeValue(scope);
  return s;
}

LogCfg* loadLogCfg()
{
  //ClRcT rc = logCfg.loadModule();
  //logCfg.initialize();


  /* It is a valid and common use case to set (and read) configuration by hand as shown below.
     But, normally initialization code would load the configuration from the database rather than setting it by hand.
   */
  logcfg.serverConfig.maximumStreams = 100U;
  logcfg.serverConfig.maximumStreams = 10;  // ???
  logcfg.serverConfig.maximumSharedMemoryPages = 1000;
  logcfg.serverConfig.maximumRecordsInPacket = 1024*10;
  logcfg.serverConfig.processingInterval = 1000;
  
  Stream* s = createStreamCfg("sys","sys",".:var/log",32*1024*1024, 2048, "ROTATE", 10, 200, 500, false, "GLOBAL");
  logcfg.streamConfig.addChildObject(s,"sys");

  s = createStreamCfg("app","app",".:var/log",32*1024*1024, 2048, "ROTATE", 10, 200, 500, false, "GLOBAL");  
  logcfg.streamConfig.addChildObject(s,"app");
  
  return &logcfg;   
}
