#ifndef LOGCFG_HXX_
#define LOGCFG_HXX_

#include <clLogIpi.hxx>

#include <ServerConfig.hxx>
#include <StreamConfig.hxx>
#include <FileFullAction.hxx>
#include <Stream.hxx>
#include <StreamScope.hxx>

#include <clMgtModule.hxx>
#include <boost/unordered_map.hpp>
//ClMgtModule    logCfg("log");


// Top level class the represents the log.yang file.  I think that this should be auto-generated.
class LogCfg : public SAFplus::MgtContainer
{
public:
  SAFplusLog::ServerConfig serverConfig;
  SAFplusLog::StreamConfig streamConfig;
  LogCfg();
};

/* Load module configuration from database & create tracking objects */
extern LogCfg* loadLogCfg();
typedef boost::unordered_map<SAFplus::Handle, SAFplusLog::Stream*> HandleStreamMap;

#endif
