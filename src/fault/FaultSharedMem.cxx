#include <clCustomization.hxx>
#include <FaultSharedMem.hxx>
#include <boost/interprocess/managed_shared_memory.hpp>

using namespace boost::interprocess;


namespace SAFplus
{
void FaultSharedMem::init()
{
    faultSharedMemoryObjectName = "SAFplusFault";
    faultMsm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, faultSharedMemoryObjectName.c_str(), SAFplusI::FaultSharedMemSize);
    faultMap = faultMsm.find_or_construct<SAFplus::FaultShmHashMap>("faults")  (faultMsm.get_allocator<SAFplus::FaultShmMapPair>());

    try
    {
        faultHdr = (SAFplus::FaultShmHeader*) faultMsm.construct<SAFplus::FaultShmHeader>("header") ();                                 // Ok it created one so initialize
    	faultHdr->activeFaultServer = INVALID_HDL;
        faultHdr->structId=SAFplus::CL_FAULT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
    }
    catch (interprocess_exception &e)
    {
        if (e.get_error_code() == already_exists_error)
        {
            faultHdr = faultMsm.find_or_construct<SAFplus::FaultShmHeader>("header") ();                         //allocator instance
            int retries=0;
            while ((faultHdr->structId != CL_FAULT_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; boost::this_thread::sleep(boost::posix_time::milliseconds(250)); }  // If another process just barely beat me to the creation, I could find the memory but it would not be inited.  So I better wait.

            while (faultHdr->structId != CL_FAULT_BUFFER_HEADER_STRUCT_ID_7)
              {
                // That other process should have inited it by now... so that other process must not exist.  Maybe it died, or maybe the shared memory is bad.  I will initialize.
              faultHdr->activeFaultServer = INVALID_HDL;
              faultHdr->structId=SAFplus::CL_FAULT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)   
              boost::this_thread::sleep(boost::posix_time::milliseconds(100));             
              }
        }
        else throw;
    }
}


void FaultSharedMem::setActive(SAFplus::Handle active)
{
  assert(faultHdr);
  faultHdr->activeFaultServer=active;
}

bool FaultSharedMem::createFault(FaultShmEntry* frp,SAFplus::Handle fault)
{
    ScopedLock<Mutex> lock(faultMutex);
    FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end())
    {

        FaultShmEntry* fe = &((*faultMap)[fault]);
        assert(fe);  // TODO: throw out of memory
        fe->init(fault,frp);
        return true;
    }
    else
    {
        //update fault entry
        FaultShmEntry *fe = &entryPtr->second;
        fe->init(fault,frp);
        return false;
    }
}

bool FaultSharedMem::updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault)
{
    ScopedLock<Mutex> lock(faultMutex);
    SAFplus::FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end()) return false; // TODO: raise exception
    FaultShmEntry *fse = &entryPtr->second;
    assert(fse);
    fse->dependecyNum=frp->dependecyNum;
    for(int i=0; i<fse->dependecyNum;i++)
    {
        fse->depends[i]=frp->depends[i];
    }
    fse->state=frp->state;
    return true;
}

  bool FaultSharedMem::updateFaultHandleState(SAFplus::Handle fault, SAFplus::FaultState state)
{
	ScopedLock<Mutex> lock(faultMutex);
    SAFplus::FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end()) return false; // TODO: raise exception
    SAFplus::FaultShmEntry *fse = &entryPtr->second;
    assert(fse);  // TODO: throw out of memory
    fse->state=state;
    return true;
}

void FaultSharedMem::clear()
{
	ScopedLock<Mutex> lock(faultMutex);
    SAFplus::FaultShmHashMap::iterator i;
    if(!faultMap)
    {
        return;
    }
    for (i=faultMap->begin(); i!=faultMap->end();i++)
    {
        SAFplus::Handle faultHdl = i->first;
        FaultShmEntry& fe = i->second;
        fe.state = SAFplus::FaultState::STATE_UNDEFINED;
        fe.dependecyNum=0;
        //fe.name[0] = '\0';
    }
}
void FaultSharedMem::remove(const SAFplus::Handle handle)
{
    if(!faultMap)
    {
        return;
    }
    ScopedLock<Mutex> lock(faultMutex);
    SAFplus::FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(handle);
    if (entryPtr == faultMap->end()) return; // TODO: raise exception
    assert(&handle);  // TODO: throw out of memory
    faultMap->erase(handle);
}

void FaultSharedMem::removeAll()
{
    SAFplus::FaultShmHashMap::iterator i;
    if(!faultMap)
    {
        return;
    }
    ScopedLock<Mutex> lock(faultMutex);
    for (i=faultMap->begin(); i!=faultMap->end();i++)
    {
        SAFplus::Handle faultHdl = i->first;
        assert(&faultHdl);  // TODO: throw out of memory
        faultMap->erase(faultHdl);
	}
}

uint_t FaultSharedMem::getAllFaultClient(char* buf, ClWordT bufSize)
{
    int count = 0;
    int offset = 0;
    SAFplus::FaultShmHashMap::iterator i;
    if(!faultMap)
    {
        return 0;
    }
    ScopedLock<Mutex> lock(faultMutex);
    for (i=faultMap->begin(); i!=faultMap->end();i++)
    {
        Buffer* key = (Buffer*)(&(i->first));
        Buffer* val = (Buffer*)(&(i->second));
        int dataSize = key->objectSize() + val->objectSize();
        if (offset+dataSize >= bufSize) break;  // Sorry can't read any more

        assert(offset + key->objectSize() < SAFplusI::MAX_FAULT_BUFFER_SIZE);
        memcpy(&buf[offset],(void*) key, key->objectSize());
        offset += key->objectSize();

        assert(offset + val->objectSize() < SAFplusI::MAX_FAULT_BUFFER_SIZE);
        memcpy(&buf[offset],(void*) val, val->objectSize());
        offset += val->objectSize();
        count++;
    }
    return count;
}

void FaultSharedMem::applyFaultSync(char* buf, ClWordT bufSize)
{
    int curpos = 0;
    int count = 0;
    while (curpos < bufSize)
    {
        count++;
        Buffer* key = (Buffer*) (((char*)buf)+curpos);
        curpos += sizeof(Buffer) + key->len() - 1;
        Buffer* val = (Buffer*) (((char*)buf)+curpos);
        curpos += sizeof(Buffer) + val->len() - 1;
        createFault((FaultShmEntry*)val, *(Handle*)key);
    }
}
};


