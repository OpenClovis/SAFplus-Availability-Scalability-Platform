#ifndef CLLOGREP_HXX_
#define CLLOGREP_HXX_

#include <boost/unordered_map.hpp>
#include <clHandleApi.hxx>
#include <clLogIpi.hxx>
#include <ServerConfig.hxx>
#include "../server/logcfg.hxx"

extern LogCfg logcfg;

namespace SAFplus {

/* This struct holds a message header */
struct LogMsgHeader {
  short idAndEndian;  // This is a well-known number
  char  version;
  char  extra;
  SAFplus::Handle streamHandle;  // One receiver can subscribe multiple streams.
  short numLogs;
  LogMsgHeader(){}
  LogMsgHeader(short p_idAndEndian, char p_version, char p_extra, SAFplus::Handle p_streamHandle, short p_numLogs):
    idAndEndian(p_idAndEndian), version(p_version), extra(p_extra), streamHandle(p_streamHandle), numLogs(p_numLogs)
  {
  }
};

class LogRep
{
protected:
  void sendBuffer(SAFplusLog::Stream* s, SAFplus::Handle& streamHdl);
public:
  enum 
  {
    CLSS_ID = 0x160e
  };
  LogRep();
  void logReplicate(SAFplusI::LogBufferEntry* rec, char* msg);
  void flush(SAFplusLog::Stream* s);
};

extern LogRep logRep;

}
#endif // CLLOGREP
