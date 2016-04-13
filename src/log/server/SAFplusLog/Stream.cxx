/* 
 * File Stream.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "StreamAttributes.hxx"
#include "MgtFactory.hxx"
#include "SAFplusLogCommon.hxx"
#include <string>
#include "StreamStatistics.hxx"
#include <vector>
#include "Stream.hxx"

using namespace  std;
using namespace SAFplusLog;

namespace SAFplusLog
  {

    /* Apply MGT object factory */
    MGT_REGISTER_IMPL(Stream, /SAFplusLog/safplusLog/streamConfig/stream)

    Stream::Stream()
    :fileBuffer(SAFplusI::LogDefaultFileBufferSize),msgBuffer(SAFplusI::LogDefaultMessageBufferSize),fp(NULL),numLogs(0),fileIdx(0),earliestIdx(0),numFiles(0),fileSize(0),sendMsg(false),dirty(false),lastUpdate(0)  // additions
    {
        this->addChildObject(&streamStatistics, "streamStatistics");
        this->tag.assign("stream");
    };

    Stream::Stream(const std::string& nameValue)
      :fileBuffer(SAFplusI::LogDefaultFileBufferSize),msgBuffer(SAFplusI::LogDefaultMessageBufferSize),fp(NULL),numLogs(0),fileIdx(0),earliestIdx(0),numFiles(0),fileSize(0),sendMsg(false),dirty(false),lastUpdate(0)  // additions
    {
        this->name.value =  nameValue;
        this->addChildObject(&streamStatistics, "streamStatistics");
        this->tag.assign("stream");
        // Additions
        addStreamObjMapping(this->name.value.c_str(), this);
    };

    std::vector<std::string> Stream::getKeys()
    {
        std::string keyNames[] = { "name" };
        return std::vector<std::string> (keyNames, keyNames + sizeof(keyNames) / sizeof(keyNames[0]));
    };

    std::vector<std::string>* Stream::getChildNames()
    {
        std::string childNames[] = { "name", "fileName", "replicate", "fileLocation", "fileUnitSize", "recordSize", "fileFullAction", "maximumFilesRotated", "flushFreq", "flushInterval", "syslog", "streamScope", "streamStatistics" };
        return new std::vector<std::string> (childNames, childNames + sizeof(childNames) / sizeof(childNames[0]));
    };

    /*
     * XPATH: /SAFplusLog/safplusLog/streamConfig/stream/streamStatistics
     */
    SAFplusLog::StreamStatistics* Stream::getStreamStatistics()
    {
        return dynamic_cast<StreamStatistics*>(this->getChildObject("streamStatistics"));
    };

    /*
     * XPATH: /SAFplusLog/safplusLog/streamConfig/stream/streamStatistics
     */
    void Stream::addStreamStatistics(SAFplusLog::StreamStatistics *streamStatisticsValue)
    {
        this->addChildObject(streamStatisticsValue, "streamStatistics");
    };

    Stream::~Stream()
    {
    };

}
/* namespace ::SAFplusLog */
