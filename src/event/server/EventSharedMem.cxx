#include <clCustomization.hxx>
#include <EventSharedMem.hxx>
#include <boost/interprocess/managed_shared_memory.hpp>

using namespace boost::interprocess;

namespace SAFplus
{
void EventSharedMem::init(const bool& isServer)
{
	logDebug("AAA","AAA", "db1");
	if(isServer)
	{
	  eventMsm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, eventSharedMemoryObjectName.c_str(), SAFplusI::EventSharedMemSize);
	}else
	{
	  eventMsm = boost::interprocess::managed_shared_memory(boost::interprocess::open_only, eventSharedMemoryObjectName.c_str());
	}
	try
	{
	  if(isServer)
	  {
	    eventHdr = (SAFplus::EventShmHeader*) eventMsm.construct < SAFplus::EventShmHeader > ("header")();                                 // Ok it created one so initialize
            eventHdr->activeEventServer = INVALID_HDL;
            eventHdr->structId = 65000; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
            eventHdr->lastChange = nowMs();
	  }else
	  {
	    eventHdr = (SAFplus::EventShmHeader*) eventMsm.find < SAFplus::EventShmHeader > ("header").first;
	  }
	} catch (interprocess_exception &e)
	{
		if (e.get_error_code() == already_exists_error)
		{
			eventHdr = eventMsm.find_or_construct < SAFplus::EventShmHeader > ("header")();                         //allocator instance
			int retries = 0;
			while ((eventHdr->structId != 650000) && (retries < 2))
			{
				retries++;
				boost::this_thread::sleep(boost::posix_time::milliseconds(250));
			}  // If another process just barely beat me to the creation, I could find the memory but it would not be inited.  So I better wait.

			while (eventHdr->structId != 650000)
			{
				// That other process should have inited it by now... so that other process must not exist.  Maybe it died, or maybe the shared memory is bad.  I will initialize.
				eventHdr->activeEventServer = INVALID_HDL;
				eventHdr->structId = 650000; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
				eventHdr->lastChange = nowMs();
				boost::this_thread::sleep(boost::posix_time::milliseconds(100));
			}
		}
		else
			throw;
	}
}
void EventSharedMem::initWithName(const std::string& objectName,const bool& isServer)
{
  eventSharedMemoryObjectName = objectName;
  init(isServer);
}
SAFplus::Handle EventSharedMem::getActive()
{
    return eventHdr->activeEventServer;
}

uint64_t EventSharedMem::lastChange(void)
{
	assert (eventHdr);
	ScopedLock<Mutex> lock(eventMutex);
	return eventHdr->lastChange;
}

void EventSharedMem::changed(void)
{
	assert (eventHdr);
	eventHdr->lastChange = std::max(eventHdr->lastChange + 1, nowMs()); // Use the current time because its useful, but ensure that the number is monotonically increasing
	logInfo("FLT", "SHM", "Fault DB changed at [%" PRIu64 "]", eventHdr->lastChange);

}

void EventSharedMem::setActive(SAFplus::Handle active)
{
	assert (eventHdr);
	logInfo("EVT", "SHR", "Set Active server to shared mem");
	eventHdr->activeEventServer = active;
}
}
;

