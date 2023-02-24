#ifndef CLEVENT_SHM_H_
#define CLEVENT_SHM_H_

#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

#include <ctime>
#include <clMsgApi.hxx>
#include <clGroupApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clGroupApi.hxx>

namespace SAFplus
{

// share memory include fault entity information and master event server address
class EventShmHeader
{
public:
	uint64_t structId;  //? Unique number identfying this as fault related data
	SAFplus::Handle activeEventServer;
	uint64_t lastChange; //? monotonically increasing number indicating the last time a change was made to fault
};

enum {
  CL_EVENT_HEADER_STRUCT_ID = 65000,
};

class EventSharedMem
{
public:
	SAFplus::ProcSem mutex;  // protects the shared memory region from simultaneous access
	SAFplus::Mutex eventMutex;
	boost::interprocess::managed_shared_memory eventMsm;
	EventShmHeader* eventHdr;
	//SAFplus::Mutex  localMutex;
	void init(void);
	void setActive(SAFplus::Handle active);
	SAFplus::Handle getActive();
	void clear();
	void changed(void);
	uint64_t lastChange(void);

};

}
#endif
