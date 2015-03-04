#include <clFaultIpi.hxx>
#include <boost/interprocess/managed_shared_memory.hpp>

using namespace boost::interprocess;


namespace SAFplus
{
void FaultSharedMem::init(SAFplus::Handle active)
{

    faultMsm = boost::interprocess::managed_shared_memory(boost::interprocess::open_or_create, "SAFplusFaults", SAFplusI::FaultSharedMemSize);
    faultMap = faultMsm.find_or_construct<SAFplus::FaultShmHashMap>("faults")  (faultMsm.get_allocator<SAFplus::FaultShmMapPair>());
    if(active == INVALID_HDL)
    {
    	// fault client
    	faultHdr = faultMsm.find_or_construct<SAFplus::FaultShmHeader>("header") ();
    	return;
    }
    try
    {
        faultHdr = (SAFplus::FaultShmHeader*) faultMsm.construct<SAFplus::FaultShmHeader>("header") ();                                 // Ok it created one so initialize
    	faultHdr->iocFaultServer = active;
        faultHdr->structId=SAFplus::CL_FAULT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
    }
    catch (interprocess_exception &e)
    {
        if (e.get_error_code() == already_exists_error)
        {
        	faultHdr = faultMsm.find_or_construct<SAFplus::FaultShmHeader>("header") ();                         //allocator instance
            int retries=0;
            while ((faultHdr->structId != CL_FAULT_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        }
        else throw;
    }
}


bool FaultSharedMem::createFault(FaultShmEntry* frp,SAFplus::Handle fault)
{
    ScopedLock<Mutex> lock(faultMutex);
    FaultShmHashMap::iterator entryPtr;
    entryPtr = faultMap->find(fault);
    if (entryPtr == faultMap->end())
    {
    	logInfo("FLT","SHR","Create new shared memory entity");
        FaultShmEntry* fe = &((*faultMap)[fault]);
        assert(fe);  // TODO: throw out of memory
        fe->init(fault,frp);
        return true;
    }
    else
    {
    	logInfo("FLT","SHR","Update shared memory entity");
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
    //strncpy(fse->name,frp->name,FAULT_NAME_LEN);
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

void FaultSharedMem::getAllFaultClient(char* buf, ClWordT bufSize)
{
	SAFplus::FaultShmHashMap::iterator i;
	if(!faultMap)
	{
		return;
	}
	ScopedLock<Mutex> lock(faultMutex);
	for (i=faultMap->begin(); i!=faultMap->end();i++)
	{
	    Buffer* key = (Buffer*)(&(i->first));
        Buffer* val = (Buffer*)(&(i->second));
        int dataSize = key->objectSize() + val->objectSize();
        assert(bufSize + key->objectSize() < MAX_FAULT_BUFFER_SIZE);
        memcpy(&buf[bufSize],(void*) key, key->objectSize());
        bufSize += key->objectSize();

        assert(bufSize + val->objectSize() < MAX_FAULT_BUFFER_SIZE);
        memcpy(&buf[bufSize],(void*) val, val->objectSize());
        bufSize += val->objectSize();
	}
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


