/* 
 * File StreamScope.cxx
 * This file has been auto-generated by Y2CPP, the
 * plug-in of pyang.
 */ 

#include "MgtEnumType.hxx"
#include <iostream>
#include "SAFplusLogCommon.hxx"
#include "StreamScope.hxx"

using namespace SAFplusLog;

namespace SAFplusLog
  {

    /*
     * Provide an implementation of the en2str_map lookup table.
     */
    const StreamScopeManager::map_t StreamScopeManager::en2str_map = {
            pair_t(StreamScope::GLOBAL, "GLOBAL"),
            pair_t(StreamScope::LOCAL, "LOCAL")
    }; // uses c++11 initializer lists 

    const char* c_str(const SAFplusLog::StreamScope &streamScope)
    {
        return StreamScopeManager::c_str(streamScope);
    };

    std::ostream& operator<<(std::ostream &os, const SAFplusLog::StreamScope &streamScope)
    {
        return os << StreamScopeManager::toString(streamScope);
    };

    std::istream& operator>>(std::istream &is, SAFplusLog::StreamScope &streamScope)
    {
        std::string buf;
        is >> buf;
        streamScope = StreamScopeManager::toEnum(buf);
        return is;
    };

}
/* namespace ::SAFplusLog */
