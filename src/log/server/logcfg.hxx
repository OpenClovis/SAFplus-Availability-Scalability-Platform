#ifndef LOGCFG_HXX_
#define LOGCFG_HXX_

#include <clLogIpi.hxx>

#include <ServerConfig.hxx>
#include <StreamConfig.hxx>
#include <FileFullAction.hxx>
#include <Stream.hxx>
#include <StreamScope.hxx>

#include <SAFplusLogModule.hxx>
#include <boost/unordered_map.hpp>
//ClMgtModule    logCfg("log");


/* Load module configuration from database & create tracking objects */
extern SAFplusLog::SAFplusLogModule* loadLogCfg();
typedef boost::unordered_map<SAFplus::Handle, SAFplusLog::Stream*> HandleStreamMap;

#endif
