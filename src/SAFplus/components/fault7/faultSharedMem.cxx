
#include <clFaultIpi.hxx>
#include <clFaultApi.hxx>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace SAFplus
{
void FaultSharedMem::init()
{
    mutex.init("FaultSharedMem",1);
    ScopedLock<ProcSem> lock(mutex);
    faultMsm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, "SAFplusFaults", SAFplusI::FaultSharedMemSize);
    faultMap = faultMsm.find_or_construct<SAFplus::FaultShmHashMap>("faults")  (faultMsm.get_allocator<SAFplus::FaultShmMapPair>());
}

bool FaultSharedMem::createFault(FaultShmEntry* frp,SAFplus::Handle fault)
{
    //ScopedLock<ProcSem> lock(mutex);
    FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end())
    {
        ScopedLock<ProcSem> lock(mutex);
        FaultShmEntry* fe = &((*faultMap)[fault]);
        assert(fe);  // TODO: throw out of memory
        fe->init(fault,frp);
        return true;
    }
    else
    {
        FaultShmEntry *fe = &entryPtr->second;
        fe->init(fault,frp);
        //update fault entry
        return false;
    }
}

bool FaultSharedMem::updateFaultHandle(FaultShmEntry* frp,SAFplus::Handle fault)
{
    ScopedLock<ProcSem> lock(mutex);
    SAFplus::FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end()) return false; // TODO: raise exception
    FaultShmEntry *fse = &entryPtr->second;
    assert(fse);  // TODO: throw out of memory
    strncpy(fse->name,frp->name,FAULT_NAME_LEN);
    fse->dependecyNum=frp->dependecyNum;
    for(int i=0; i<fse->dependecyNum;i++)
    {
        fse->depends[i]=frp->depends[i];
    }
    fse->state=frp->state;
    return true;
}

bool FaultSharedMem::updateFaultHandleState(SAFplus::FaultState state,SAFplus::Handle fault)
{
    ScopedLock<ProcSem> lock(mutex);
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
    ScopedLock<ProcSem> lock(mutex);
    SAFplus::FaultShmHashMap::iterator i;
    for (i=faultMap->begin(); i!=faultMap->end();i++)
    {
        SAFplus::Handle faultHdl = i->first;
        FaultShmEntry& fe = i->second;
        fe.state = SAFplus::FaultState::STATE_UNDEFINED;
        fe.dependecyNum=0;
        fe.name[0] = '\0';
    }
}
void FaultSharedMem::remove(const SAFplus::Handle handle)
{
	ScopedLock<ProcSem> lock(mutex);
	SAFplus::FaultShmHashMap::iterator entryPtr;
	entryPtr = faultMap->find(handle);
	if (entryPtr == faultMap->end()) return; // TODO: raise exception
	assert(&handle);  // TODO: throw out of memory
	faultMap->erase(handle);
}

};


