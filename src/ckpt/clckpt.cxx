#include <sys/types.h>
#include <unistd.h>
#include <cltypes.h>
//#include <boost/interprocess/shared_memory_object.hpp>
//#include <boost/interprocess/mapped_region.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <clCommon.hxx>

#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>

#include <clCustomization.hxx>

namespace fs = boost::filesystem;

using namespace boost::interprocess;
using namespace SAFplus;
using namespace SAFplusI;

#define BOOST_CKPT_OVERHEAD 0x100


std::size_t SAFplusI::hash_value(BufferPtr const& b)  // Actually hashes the buffer not the pointer of course
    {
      if (!b) return 0; // Hash NULL to 0
      return hash_value(*b);
    }


bool SAFplusI::BufferPtrContentsEqual::operator() (const BufferPtr& x, const BufferPtr& y) const
{
  bool result = (*x==*y);
  SAFplus::Buffer* xb = x.get();
  SAFplus::Buffer* yb = y.get();
  //printf("buffer ptr compare: %p (%d:%d) and %p (%d.%d) -> %d\n", xb, xb->len(), *((uint_t*)xb->data),yb,yb->len(), *((uint_t*)yb->data),result);
  return result;
}

SAFplus::Checkpoint::~Checkpoint()
{
  if (isSyncReplica) { if(sync) delete sync; }
  if (dbHandle) clDbalClose(dbHandle);
  if (flags&PERSISTENT) 
  {
    // deallocate remaining keys if any (in the case flush() is not called)
    gate.lock();
    for(CkptOperationMap::iterator iter = operMap.begin(); iter != operMap.end(); iter++)
    {
      msm.deallocate(iter->first);
    }
    operMap.clear();
    gate.unlock();
  }
}

/*
** This function is getting the existing checkpoint shared memory file for a specified ckpt handle if any.
** it's useful when user wants to open an existing checkpoint: when opening an existing one, some parameters
** used for creating checkpoint such as size, row and retentionDuration time are not counted in (not provided
** and if provided, they'll be ignored).
** Because a shm file is appended with retention duration at the end when creating checkpoint, so, this function
** must be called at init to ensure that the existing shm file (if any) is reused to open instead of building a new one
*/
#if 0
std::string getFullShmFile(const char* shmFilename)
{
  fs::directory_iterator end_iter;
  for( fs::directory_iterator dir_iter(SharedMemPath) ; dir_iter != end_iter ; ++dir_iter)
  {
    if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.
    // Extract just the file name by bracketing it between the preceding / and the file extension
    std::string filePath = dir_iter->path().string();
    // skip ahead to the last / so a name like /dev/shm/ckpt_ works
    int lastSlashPos = filePath.rfind("/");
    int ckptShmPos = filePath.find(shmFilename, lastSlashPos);
    if (ckptShmPos >= 0 && ckptShmPos < filePath.length())
    {
      return filePath.substr(ckptShmPos);
    }
  }  
  return std::string(); // returns empty shm file name
}
#endif

void SAFplus::Checkpoint::init(const Handle& hdl, uint_t _flags, uint64_t retentionDuration, uint_t size, uint_t rows,SAFplus::Wakeable& execSemantics)
{
  logInfo("CKP","INI","Opening checkpoint [%" PRIx64 ":%" PRIx64 "]",hdl.id[0],hdl.id[1]);
  // All constructors funnel through this init routine.
  gate.init(hdl.id[1]);  // 2nd word of the handle should be unique on this node
  gate.close(); // start the gate closed so this process can't access the checkpoint.  But I can't init the gate closed, in case the init opens an existing gate, instead of creating one
  // TODO: gate has to be closed while this process inits but also while replica syncs.  Need 2 gates (thread and interprocess).

  flags = _flags;
  if (flags & REPLICATED)
    if (!(flags & CHANGE_LOG)) flags |= CHANGE_ANNOTATION; // Change annotation is the default delta replication mechanism

  std::string ckptSharedMemoryObjectname = "SAFplusCkpt_";
  char sharedMemFile[256];

  if (hdl==INVALID_HDL)
    {
      // Allocate a new hdl
    }

  if (size < CkptMinSize) size = CkptDefaultSize;
  size += sizeof(CkptBufferHeader)+BOOST_CKPT_OVERHEAD;
  if (rows < CkptMinRows) rows = CkptDefaultRows;

  //sharedMemHandle = NULL;
  hdl.toStr(sharedMemFile);
  ckptSharedMemoryObjectname.append(sharedMemFile);

#if 0
  // check if this checkpoint has existed or not
  std::string ckptShmFile = getFullShmFile(tempStr);
  if (ckptShmFile.length()>0) // there is file matching with the ckpt handle, means ckpt does exist, so use it
  {
    strcpy(tempStr, ckptShmFile.c_str());
  }
  else // there is no any file matching with the ckpt handle, means ckpt doesn't exist, so creating a new one
  {
    std::string strTime = boost::lexical_cast<std::string>(retentionDuration);
    strcat(tempStr, ":");
    strcat(tempStr, strTime.c_str());
  }
#endif
  //strcpy(tempStr,"test");  // DEBUGGING always uses one segment

  bool isCkptExist = false;
  
  if (flags & SHARED)
    {

    if (flags & EXISTING)  // Try to create it first if the flags don't require that it exists.
      {
      msm = managed_shared_memory(open_only, ckptSharedMemoryObjectname.c_str());  // will raise something if it does not exist
      }
    else
      {
      msm = managed_shared_memory(open_or_create, ckptSharedMemoryObjectname.c_str(), size);
      }

    try
      {
      hdr = msm.construct<SAFplusI::CkptBufferHeader>("header") ();                                 // Ok it created one so initialize
      hdr->handle     = hdl;
      hdr->replicaHandle = INVALID_HDL;
      hdr->serverPid  = getpid();
      hdr->generation = 0;
      hdr->changeNum  = 0;
      hdr->structId=SAFplusI::CL_CKPT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
      hdr->retentionDuration = retentionDuration;
      hdr->lastUsed = boost::posix_time::second_clock::universal_time();
      }
    catch (interprocess_exception &e)
      {
      if (e.get_error_code() == already_exists_error)
	{
        hdr = msm.find_or_construct<SAFplusI::CkptBufferHeader>("header") ();                         //allocator instance
        int retries=0;
        while ((hdr->structId != CL_CKPT_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        if (retries>=2)
          {
          hdr->handle     = hdl;
          hdr->replicaHandle = INVALID_HDL;
          hdr->serverPid  = getpid();
          hdr->generation = 0;
          hdr->changeNum  = 0;
          hdr->structId   = SAFplusI::CL_CKPT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)          
          hdr->lastUsed = boost::posix_time::second_clock::universal_time();
          }
        else
          isCkptExist = true;
          
	}
      else throw;
      }
    
    map = msm.find_or_construct<CkptHashMap>("table")  ( rows, boost::hash<CkptMapKey>(), BufferPtrContentsEqual(), msm.get_allocator<CkptMapPair>());
    }
  else
    {
    hdr = new SAFplusI::CkptBufferHeader();
    assert(0);
    //map = new SAFplusI::CkptHashMap(rows, boost::hash<CkptMapKey>(), BufferPtrContentsEqual());
    }
  
  if (flags & REPLICATED)
    {
    sync = new CkptSynchronization(); 
    sync->init(this,NULL,execSemantics); 
    // the sync object will open the gate when synchronization is complete
    }
  else
    {
    gate.open(); // Its the synchronization replica's job to keep the gate closed until sync occurs.
    sync = NULL;
    }

  if (flags & PERSISTENT)
    {
    initDB(ckptSharedMemoryObjectname.c_str(), isCkptExist);
    }
  //assert(sharedMemHandle);
  assert(hdr);
}

const Buffer& SAFplus::Checkpoint::read (const Buffer& key) //const
{
  gate.lock();
  // Will create object if it doesn't exist
  //SAFplusI::BufferPtr& v = (*map)[SAFplusI::BufferPtr((Buffer*)&key)];  // Key is not change, but I have to cast it b/c consts are not carried thru

  CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&key));

  if (contents != map->end()) // if (curval)  // record already exists; overwrite
    {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
        {
          Buffer& ret = *(curval.get());
          hdr->lastUsed = boost::posix_time::second_clock::universal_time();
          gate.unlock();
          return ret;
        }
    }  
  hdr->lastUsed = boost::posix_time::second_clock::universal_time();
  gate.unlock();
  return *((Buffer*) NULL);
}

const Buffer& SAFplus::Checkpoint::read (const uintcw_t key) //const
{
  char data[sizeof(Buffer)-1+sizeof(uintcw_t)];
  Buffer* b = new(data) Buffer(sizeof(uintcw_t));
  *((uintcw_t*) b->data) = key;
  return read(*b);
}

const Buffer& SAFplus::Checkpoint::read (const char* key) //const
{
  size_t len = strlen(key)+1;  // +1 b/c I'm going to take the /0 so Buffers can be manipulated as strings
  char data[sizeof(Buffer)-1+len]; // -1 because inside Buffer the data field is already length one.
  Buffer* b = new(data) Buffer(len);
  *b = key;
  return read(*b);
}

const Buffer& SAFplus::Checkpoint::read (const std::string& key) //const
{
  size_t len = key.length()+1;  // I'm going to take the /0 so Buffers can be manipulated as strings
  char data[sizeof(Buffer)-1+len];
  Buffer* b = new(data) Buffer(len);
  *b = key.data();
  return read(*b);
}


void SAFplus::Checkpoint::write (const uintcw_t key,const Buffer& value,Transaction& t)
{
  char data[sizeof(Buffer)-1+sizeof(uintcw_t)];
  Buffer* b = new(data) Buffer(sizeof(uintcw_t));
  *((uintcw_t*) b->data) = key;
  write(*b,value,t);
}
   
void SAFplus::Checkpoint::write(const char* key, const Buffer& value,Transaction& t)
  {
    // Doing a copy just to create a Buffer object is not efficient and should be fixed with a direct void* interface into the checkpoint system.
    int klen = strlen(key)+1;  // +1 for the null term
    char kmem[sizeof(Buffer)-1+klen];
    Buffer* kb = new(kmem) Buffer(klen);
    memcpy(kb->data,key,klen);
    kb->setNullT(true);
    write(*kb,value,t);
  }

void SAFplus::Checkpoint::write(const std::string& key, const Buffer& value,Transaction& t)
  {
    // Doing a copy just to create a Buffer object is not efficient and should be fixed with a direct void* interface into the checkpoint system.
    int klen = key.length()+1;  // +1 for the null term
    char kmem[sizeof(Buffer)-1+klen];
    Buffer* kb = new(kmem) Buffer(klen);
    memcpy(kb->data,key.c_str(),klen);
    kb->setNullT(true);

    write(*kb,value,t);
  }

void SAFplus::Checkpoint::write(const std::string& key, const std::string& value,Transaction& t)
  {
    // Doing a copy just to create a Buffer object is not efficient and should be fixed with a direct void* interface into the checkpoint system.

    int klen = key.length()+1;  // +1 for the null term
    char kmem[sizeof(Buffer)-1+klen];

    int vlen = value.length()+1;
    char vmem[sizeof(Buffer)-1+vlen];

    Buffer* kb = new(kmem) Buffer(klen);
    memcpy(kb->data,key.c_str(),klen);
    Buffer* vb = new(vmem) Buffer(vlen);
    memcpy(vb->data, value.c_str(),vlen);
    kb->setNullT(true);
    vb->setNullT(true);
    write(*kb,*vb,t);
  }

bool SAFplus::Buffer::isNullT() const { return (refAndLen&NullTMask)>0; }

void SAFplus::Checkpoint::write(const Buffer& key, const Buffer& value,Transaction& t)
{
  gate.lock();
  // All write operations are funneled through this function.

  //Buffer* existing = read(key);
  uint_t newlen = value.len();
  uint32_t change = ++hdr->changeNum;   // TODO transactional write could all have the same changenum...

  //SAFplusI::BufferPtr& curval = (*map)[SAFplusI::BufferPtr((Buffer*)&key)];  // curval is a REFERENCE to the value in the hash table so we can overwrite it to change the value...
  CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&key));
  
  if (contents != map->end()) // if (curval)  // record already exists; overwrite
    {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
        {
          if (curval->ref() == 1)  // This hash table is the only thing using this value right now
            {
              if (curval->len() == newlen) {// lengths are the same, most efficient is to just copy the new data onto the old.
                //memcpy (curval->data,value.data,newlen);
                *curval = value;
                if (flags & CHANGE_ANNOTATION) curval->setChangeNum(change);
                if (sync) sync->sendUpdate(&key,curval.get(), t);
                hdr->lastUsed = boost::posix_time::second_clock::universal_time(); 
                updateOperMap(contents->first.get(), INUP);
                gate.unlock();
                return;
              }
            }
          // Replace the Buffer with a new one
          SAFplus::Buffer* v = new (msm.allocate(newlen+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (newlen);  // Place a new buffer object into the segment, -1 b/c data is a 1 byte char already
          SAFplus::Buffer* old = curval.get();
          *v = value;
          if (flags & CHANGE_ANNOTATION) v->setChangeNum(change);
          curval = v;
          if (old->ref()==1)
            msm.deallocate(old);  // if I'm the last owner, let this go.
          else 
            old->decRef();
          if (sync) sync->sendUpdate(&key,v, t);  // Pass curval not the parameter because curval has the proper change number
          hdr->lastUsed = boost::posix_time::second_clock::universal_time();          
          updateOperMap(contents->first.get(), INUP);
          gate.unlock();          
          return;
        }
    }

  // No record exists, add a new one.
  int klen =key.len()+sizeof(SAFplus::Buffer)-1; 
  SAFplus::Buffer* k = new (msm.allocate(klen)) SAFplus::Buffer (key.len());  // Place a new buffer object into the segment
  *k = key;
  if (flags & CHANGE_ANNOTATION) k->setChangeNum(change);
  SAFplus::Buffer* v = new (msm.allocate(newlen+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (newlen);  // Place a new buffer object into the segment
  *v = value;
  if (flags & CHANGE_ANNOTATION) v->setChangeNum(change);  // TODO: putting the change number in both the key is unnecessary
  SAFplusI::BufferPtr kb(k),kv(v);
  SAFplusI::CkptMapPair vt(kb,kv);
  map->insert(vt);
  if (sync) sync->sendUpdate(&key,v, t);
  hdr->lastUsed = boost::posix_time::second_clock::universal_time();
  updateOperMap(k, INUP);
  gate.unlock();
}

void SAFplus::Checkpoint::applySync(const Buffer& key, const Buffer& value,Transaction& t)
{
  //Buffer* existing = read(key);
  uint_t newlen = value.len();

  //SAFplusI::BufferPtr& curval = (*map)[SAFplusI::BufferPtr((Buffer*)&key)];  // curval is a REFERENCE to the value in the hash table so we can overwrite it to change the value...
  CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&key));
  
  if (contents != map->end()) // if (curval)  // record already exists; overwrite
    {
      SAFplusI::BufferPtr& curkey =  (SAFplusI::BufferPtr&) contents->first; // I need to discard the "const" here to modify the changenum.  Since the change num is not part of the key's hash calculation this should be ok within the context of the map data structure.
      SAFplusI::BufferPtr& curval = contents->second;

      // If this change or a later one has already been applied, skip it.
      if (curval->changeNum() >= value.changeNum())
        {
        logInfo("TEST","GRP", "Checkpoint has more recent change [%d] vs [%d], not applying...",curval->changeNum(),value.changeNum());
        return;
        }

      if (flags & CHANGE_ANNOTATION) curkey->setChangeNum(key.changeNum());  // Apply the proper change number to this buffer -- note, but not using the changenum in the key so should not matter.
      if (curval)
        {
          if (curval->ref() == 1)  // This hash table is the only thing using this value right now
            {
              if (curval->len() == newlen) {// lengths are the same, most efficient is to just copy the new data onto the old.
                *curval = value;  // change num is copied over as part of the assignment
                return;
              }
            }
          // Replace the Buffer with a new one
          SAFplus::Buffer* v = new (msm.allocate(newlen+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (newlen);  // Place a new buffer object into the segment, -1 b/c data is a 1 byte char already
          SAFplus::Buffer* old = curval.get();
          *v = value;
          curval = v;
          if (old->ref()==1)
            msm.deallocate(old);  // if I'm the last owner, let this go.
          else 
            old->decRef();
          return;
        }
    }

  // No record exists, add a new one.
  int klen =key.len()+sizeof(SAFplus::Buffer)-1; 
  SAFplus::Buffer* k = new (msm.allocate(klen)) SAFplus::Buffer (key.len());  // Place a new buffer object into the segment
  *k = key;
  SAFplus::Buffer* v = new (msm.allocate(newlen+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (newlen);  // Place a new buffer object into the segment
  *v = value;
  SAFplusI::BufferPtr kb(k),kv(v);
  SAFplusI::CkptMapPair vt(kb,kv);
  map->insert(vt);
  
  // I don't update hdr->changeNum in case the replication fails.  Then I need to restart it from the original sync location, so I need to preserve that.  
}



void SAFplus::Checkpoint::remove (const uintcw_t key,Transaction& t)
{
  char data[sizeof(Buffer)-1+sizeof(uintcw_t)];
  Buffer* b = new(data) Buffer(sizeof(uintcw_t));
  *((uintcw_t*) b->data) = key;
  remove(*b,t);
}

void SAFplus::Checkpoint::remove(const Buffer& key,Transaction& t)
{
  gate.lock();
  CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&key));

  if (contents != map->end())
    {
      SAFplusI::BufferPtr value = contents->second;  // remove the value
      SAFplusI::BufferPtr curkey   = contents->first;  // remove the key

      map->erase(curkey);
      
      SAFplus::Buffer* val = value.get();
      if (val->ref()==1) 
        msm.deallocate(val);  // if I'm the last owner, let this go.
      else 
        val->decRef();	     

      val = curkey.get();
      if (val->ref()==1)         
        if (flags&PERSISTENT)
          operMap[val] = DEL; // Defer deallocate later after flush() or Checkpoint destructor
        else
          msm.deallocate(val); // if I'm the last owner, let this go. 
      else 
        val->decRef();
    }
  gate.unlock();
}

#if 0
void SAFplus::Checkpoint::remove(const SAFplusI::BufferPtr& bufPtr, bool isKey, Transaction& t)
{
  if (isKey)
  {    
    map->erase(bufPtr);    
  }

  if (bufPtr.get()->ref() == 1)
  {
    msm.deallocate(bufPtr.get());
  }
  else
  {
    bufPtr.get()->decRef();
  }
}

void SAFplus::Checkpoint::remove(Buffer* buf, bool isKey, Transaction& t)
{
  SAFplusI::BufferPtr kb(buf);
  remove(kb, isKey, t);
}
#endif

void SAFplus::Checkpoint::dbgRemove(char* name)
{
  //char tempStr[81];
  //hdr->handle.toStr(tempStr);
  shared_memory_object::remove(name);
}

void SAFplus::Checkpoint::dump()
{
  for(CkptHashMap::const_iterator iter = map->cbegin(); iter != map->cend(); iter++)
     {
       CkptHashMap::value_type t = *iter;
       if (t.first) printf("%d:%s ->", t.first->len(),((char*) t.first->data));
       if (t.second)
	 printf(" %d:%s\n", t.second->len(),t.second->data);
       else printf(" nada\n");
     }
}

void SAFplus::Checkpoint::stats()
{
  char tempStr[81];
  printf("Handle: %s size: %lu, max_size: %lu generation: %d change: %d\n",hdr->handle.toStr(tempStr), (long unsigned int) map->size(),(long unsigned int) map->max_size(),hdr->generation, hdr->changeNum);
}


SAFplus::Checkpoint::Iterator SAFplus::Checkpoint::begin()
{
  gate.lock();
  SAFplus::Checkpoint::Iterator i(this);
  assert(this->map);
  i.iter = this->map->begin();
  i.curval = &(*i.iter);
  gate.unlock();
  return i;
}

SAFplus::Checkpoint::Iterator SAFplus::Checkpoint::end()
{
  SAFplus::Checkpoint::Iterator i(this);
  assert(this->map);
  i.iter = this->map->end();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Checkpoint::Iterator SAFplus::Checkpoint::find(void* key, uint_t len)
{   
  assert(this->map);
  char kdata[sizeof(Buffer)-1+len];
  Buffer* kbuf = new(kdata) Buffer(len);
  memcpy(kbuf->data, key, len);  
  return (find(*kbuf));
}

SAFplus::Checkpoint::Iterator SAFplus::Checkpoint::find(Buffer& key)
{   
  assert(this->map);  
  gate.lock();
  CkptHashMap::iterator it = map->find(SAFplusI::BufferPtr(&key));
  gate.unlock();
  SAFplus::Checkpoint::Iterator i(this);
  i.iter = it;
  i.curval = &(*i.iter);   
  return i;
}

SAFplus::Checkpoint::Iterator::Iterator(SAFplus::Checkpoint* _ckpt):ckpt(_ckpt)
{
  curval=NULL;
}

SAFplus::Checkpoint::Iterator::~Iterator()
{
  ckpt = NULL;
  curval=NULL;
}

SAFplus::Checkpoint::Iterator& SAFplus::Checkpoint::Iterator::operator++()
{
  iter++;
  curval = &(*iter);
  return *this;
}

SAFplus::Checkpoint::Iterator& SAFplus::Checkpoint::Iterator::operator++(int)
{
  iter++;
  curval = &(*iter);
  return *this;
}

bool SAFplus::Checkpoint::Iterator::operator !=(const SAFplus::Checkpoint::Iterator& otherValue) const
{
  if (ckpt != otherValue.ckpt) return true;
  if (iter != otherValue.iter) return true;
  return false;
}

void SAFplus::Checkpoint::initDB(const char* ckptId, bool isCkptExist)
{
  ClRcT rc = CL_OK;
  dbHandle = NULL;
  char dbName[CL_MAX_NAME_LENGTH];
  const char* path=ASP_RUNDIR;
  if (!path || !strlen(ASP_RUNDIR))
  {
    path = ".";
  }
  snprintf (dbName, CL_MAX_NAME_LENGTH-1, "%s/%s.db", path, ckptId);
  rc = clDbalOpen(dbName, dbName, CL_DB_OPEN, CkptMaxKeySize, CkptMaxRecordSize, &dbHandle);
  if (rc == CL_OK)
  {
    if (!isCkptExist)
    {
      syncFromDisk();
    }
  }
  else if (flags&PERSISTENT)
  {
    rc = clDbalOpen(dbName, dbName, CL_DB_CREAT, CkptMaxKeySize, CkptMaxRecordSize, &dbHandle);
    if (rc != CL_OK)
    {
      logWarning("CKPT", "INITDB", "Opening database failed, checkpoint data will not be stored on disk");
    }
  }  
}

void SAFplus::Checkpoint::writeCkpt(ClDBKeyHandleT key, ClUint32T keySize, ClDBRecordHandleT rec, ClUint32T recSize)
{
  if (!memcmp(key, ckptHeader(structId), keySize)) 
    memcpy(&hdr->structId, rec, recSize);
  else if (!memcmp(key, ckptHeader(serverPid), keySize))
    memcpy(&hdr->serverPid, rec, recSize);
  else if (!memcmp(key, ckptHeader(generation), keySize))
    memcpy(&hdr->generation, rec, recSize);
  else if (!memcmp(key, ckptHeader(changeNum), keySize)) 
    memcpy(&hdr->changeNum, rec, recSize);
  else if (!memcmp(key, ckptHeader(hdl), keySize)) 
    memcpy(&hdr->handle, rec, recSize);
  else if (!memcmp(key, ckptHeader(replicaHandle), keySize)) 
    memcpy(&hdr->replicaHandle, rec, recSize);
  else if (!memcmp(key, ckptHeader(retentionDuration), keySize)) 
    memcpy(&hdr->retentionDuration, rec, recSize);
  else if (!memcmp(key, ckptHeader(lastUsed), keySize)) 
    memcpy(&hdr->lastUsed, rec, recSize);
  else 
    writeCkptData(key, keySize, rec, recSize);
  clDbalRecordFree(dbHandle, rec);
}

void SAFplus::Checkpoint::writeCkptData(ClDBKeyHandleT key, ClUint32T keySize, ClDBRecordHandleT rec, ClUint32T recSize)
{
  /* Ckpt write is called comming from a disk synchronization to the ckpt, do not mark the operation to avoid disk write looping */
  uint32_t change = ++hdr->changeNum; 
  SAFplus::Buffer* k = new (msm.allocate(keySize+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (keySize);
  memcpy(k->data, key, keySize);
  if (flags & CHANGE_ANNOTATION) k->setChangeNum(change);
  SAFplus::Buffer* v = new (msm.allocate(recSize+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (recSize);
  memcpy(v->data, rec, recSize);
  if (flags & CHANGE_ANNOTATION) v->setChangeNum(change);
  SAFplusI::BufferPtr kb(k),kv(v);
  SAFplusI::CkptMapPair vt(kb,kv);
  map->insert(vt);
  Transaction t=NO_TXN;
  if (sync) sync->sendUpdate(k,v,t);
  hdr->lastUsed = boost::posix_time::second_clock::universal_time();
}

void SAFplus::Checkpoint::syncFromDisk()
{
  if (!dbHandle)
  {
    logWarning("CKPT", "DSKSYN", "Database handle is NULL, cancel the operation");
    return;
  }
  ClDBKeyHandleT key;
  ClUint32T keySize;
  ClDBRecordHandleT rec;
  ClUint32T recSize; 
  gate.lock();
  ClRcT rc = clDbalFirstRecordGet(dbHandle, &key, &keySize, &rec, &recSize);  
  while (rc == CL_OK)
  {    
    writeCkpt(key, keySize, rec, recSize);
    ClDBKeyHandleT curKey = key;
    ClUint32T curKeySize = keySize;    
    rc = clDbalNextRecordGet(dbHandle, curKey, curKeySize, &key, &keySize, &rec, &recSize);        
    clDbalKeyFree(dbHandle, curKey);
  }
  gate.unlock();
}

void SAFplus::Checkpoint::writeHdrDB(int hdrKey, ClDBRecordT rec, ClUint32T recSize)
{  
  const char* key = ckptHeader(hdrKey);
  ClRcT rc = clDbalRecordReplace(dbHandle, (ClDBKeyT) key, strlen(key)+1, rec, recSize);
  if (rc != CL_OK)
  {
    logWarning("CPKT", "WRTHDR", "add/update key [%s] to database fail", key);
  }
}

void SAFplus::Checkpoint::writeDataDB(Buffer* key, Buffer* val)
{   
  ClRcT rc = clDbalRecordReplace(dbHandle, (ClDBKeyT) key->get(), key->len(), (ClDBRecordT) val->get(), val->len());
  if (rc != CL_OK)
  {
    logWarning("CPKT", "WRTDATA", "add/update key [%s] to database fail", key->get());
  }
}

void SAFplus::Checkpoint::deleteDataDB(Buffer* key)
{   
  ClRcT rc = clDbalRecordDelete(dbHandle, (ClDBKeyT) key->get(), key->len());
  if (rc != CL_OK)
  {
    logWarning("CPKT", "WRTDATA", "delete key [%s] from database fail", key->get());
  }
}

void SAFplus::Checkpoint::flush()
{  
  if (flags&PERSISTENT)
  {
    if (!dbHandle)
    {
      logWarning("CKPT", "FLUSH", "Database handle is NULL, cancel the operation");
      return;
    }
    // write code that reads data from shared memory, then synchronizes those with data from database
    /* writting ckpt header */
    logTrace("CKPT", "FLUSH", "Updating header DB");
    writeHdrDB(structId, (ClDBRecordT) &hdr->structId, sizeof(hdr->structId));
    writeHdrDB(serverPid, (ClDBRecordT) &hdr->serverPid, sizeof(hdr->serverPid));
    writeHdrDB(generation, (ClDBRecordT) &hdr->generation, sizeof(hdr->generation));    
    writeHdrDB(changeNum, (ClDBRecordT) &hdr->changeNum, sizeof(hdr->changeNum));
    writeHdrDB(hdl, (ClDBRecordT) &hdr->handle, sizeof(hdr->handle));
    writeHdrDB(replicaHandle, (ClDBRecordT) &hdr->replicaHandle, sizeof(hdr->replicaHandle));
    writeHdrDB(retentionDuration, (ClDBRecordT) &hdr->retentionDuration, sizeof(hdr->retentionDuration));  
    writeHdrDB(lastUsed, (ClDBRecordT) &hdr->lastUsed, sizeof(hdr->lastUsed));
    /* writing ckpt data */
    logTrace("CKPT", "FLUSH", "Updating data DB");    
    gate.lock();
    for(CkptOperationMap::iterator iter = operMap.begin(); iter != operMap.end(); iter++)  
    {       
      CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr(iter->first));
      if (contents != map->end()) 
      {
        if (iter->second == INUP)
        {
          writeDataDB(contents->first.get(), contents->second.get());
        }
      }
      else if (iter->second == DEL)
      {
        deleteDataDB(iter->first);  
        msm.deallocate(iter->first);
      }      
    }
    operMap.clear();
    gate.unlock();    
  }
  else
  {
    clDbgNotImplemented("%s is not supported with this checkpoint type", __FUNCTION__);
  }
}
void SAFplus::Checkpoint::updateOperMap(Buffer* key, uint_t oper)
{
  if (!(flags&PERSISTENT)) return;
  CkptOperationMap::iterator contents = operMap.find(key);
  if (contents != operMap.end())
  {
    if (contents->second == DEL) // if this key was removed previously and now it's added (rewritten) again, so, it must be deallocated prior to being rewritten
      msm.deallocate(contents->first);    
  }
  operMap[key] = oper;    
}
#if 0
void SAFplus::Checkpoint::dumpHeader()
{
  char tempStr[81];
  printf("Handle: %s serverPid: %lu, retention: %lu, generation: %d, change: %d, lastUsed: %s, structId: 0x%lx\n",hdr->handle.toStr(tempStr), (long unsigned int) hdr->serverPid,(long unsigned int) hdr->retentionDuration,hdr->generation, hdr->changeNum, to_simple_string(hdr->lastUsed).c_str(), hdr->structId);  
}
#endif
