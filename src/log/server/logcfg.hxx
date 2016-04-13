#ifndef LOGCFG_HXX_
#define LOGCFG_HXX_

#include <clLogIpi.hxx>

#if 0
#include <ServerConfig.hxx>
#include <StreamConfig.hxx>
#include <FileFullAction.hxx>
#include <Stream.hxx>
#include <StreamScope.hxx>
#include <SAFplusLogModule.hxx>
#endif

#include <boost/unordered_map.hpp>

namespace SAFplusLog
{
  class Stream;
  class SAFplusLogModule;
};

/* Load module configuration from database & create tracking objects */
extern SAFplusLog::SAFplusLogModule* loadLogCfg();
typedef boost::unordered_map<SAFplus::Handle, SAFplusLog::Stream*> HandleStreamMap;

void addStreamObjMapping(const char* streamName, SAFplusLog::Stream* s, SAFplus::Handle strmHdl=SAFplus::INVALID_HDL);

#endif
