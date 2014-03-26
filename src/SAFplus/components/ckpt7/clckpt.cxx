#include <sys/types.h>
#include <unistd.h>

//#include <boost/interprocess/shared_memory_object.hpp>
//#include <boost/interprocess/mapped_region.hpp>

#include <clCommon.hxx>

#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>

#include <clCustomization.hxx>

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

void SAFplus::Checkpoint::init(const Handle& handle, uint_t _flags,uint_t size, uint_t rows)
{
  flags = _flags;
  char tempStr[81];
  if (handle==INVALID_HDL)
    {
      // Allocate a new handle
    }

  if (size < CkptMinSize) size = CkptDefaultSize;
  size += sizeof(CkptBufferHeader)+BOOST_CKPT_OVERHEAD;
  if (rows < CkptMinRows) rows = CkptDefaultRows;

  //sharedMemHandle = NULL;
  handle.toStr(tempStr);
  //strcpy(tempStr,"test");  // DEBUGGING always uses one segment

  if (flags & EXISTING)  // Try to create it first if the flags don't require that it exists.
    {
        msm = managed_shared_memory(open_only, tempStr);  // will raise something if it does not exist
    }
  else
    {
      msm = managed_shared_memory(open_or_create, tempStr, size);
    }

  try
    {
      hdr = msm.construct<SAFplusI::CkptBufferHeader>("header") ();                                 // Ok it created one so initialize
      //hdr->handle = 0;
      hdr->serverPid = getpid();
      hdr->structId=SAFplusI::CL_CKPT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
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
	      //hdr->handle = 0;
	      hdr->serverPid = getpid();
	      hdr->structId=SAFplusI::CL_CKPT_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
	    }
          
	}
      else throw;
    }

  //map = msm.find_or_construct<CkptHashMap>("table")  ( rows, boost::hash<CkptMapKey>(), std::equal_to<CkptMapValue>(), msm.get_allocator<CkptMapPair>());
  map = msm.find_or_construct<CkptHashMap>("table")  ( rows, boost::hash<CkptMapKey>(), BufferPtrContentsEqual(), msm.get_allocator<CkptMapPair>());

  //assert(sharedMemHandle);
  assert(hdr);
}

const Buffer& SAFplus::Checkpoint::read (const Buffer& key) const
{
  // Will create object if it doesn't exist
  //SAFplusI::BufferPtr& v = (*map)[SAFplusI::BufferPtr((Buffer*)&key)];  // Key is not change, but I have to cast it b/c consts are not carried thru

  CkptHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&key));
  
  if (contents != map->end()) // if (curval)  // record already exists; overwrite
    {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
        {
          return *(curval.get());
        }
    }
  return *((Buffer*) NULL);
}

const Buffer& SAFplus::Checkpoint::read (const uintcw_t key) const
{
  char data[sizeof(Buffer)-1+sizeof(uintcw_t)];
  Buffer* b = new(data) Buffer(sizeof(uintcw_t));
  *((uintcw_t*) b->data) = key;
  return read(*b);
}

const Buffer& SAFplus::Checkpoint::read (const char* key) const
{
  size_t len = strlen(key)+1;  // +1 b/c I'm going to take the /0 so Buffers can be manipulated as strings
  char data[sizeof(Buffer)-1+len]; // -1 because inside Buffer the data field is already length one.
  Buffer* b = new(data) Buffer(len);
  *b = key;
  return read(*b);
}

const Buffer& SAFplus::Checkpoint::read (const std::string& key) const
{
  size_t len = key.length()+1;  // I'm going to take the /0 so Buffer's can be manipulated as strings
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


void SAFplus::Checkpoint::write(const Buffer& key, const Buffer& value,Transaction& t)
{
  //Buffer* existing = read(key);
  uint_t newlen = value.len();

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
                memcpy (curval->data,value.data,newlen);
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
  SAFplus::Buffer* k = new (msm.allocate(key.len()+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (key.len());  // Place a new buffer object into the segment
  *k = key;
  SAFplus::Buffer* v = new (msm.allocate(newlen+sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (newlen);  // Place a new buffer object into the segment
  *v = value;
  SAFplusI::BufferPtr kb(k),kv(v);
  SAFplusI::CkptMapPair vt(kb,kv);
  map->insert(vt);
  
}

void SAFplus::Checkpoint::remove(char* name)
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
       if (t.first) printf("%d:%u ->", t.first->len(),*((uint_t*) t.first->data));
       if (t.second)
	 printf(" %d:%s\n", t.second->len(),t.second->data);
       else printf(" nada\n");
     }
}

void SAFplus::Checkpoint::stats()
{
  char tempStr[81];
  printf("Handle: %s size: %lu, max_size: %lu\n",hdr->handle.toStr(tempStr), map->size(),map->max_size());
}


SAFplus::Checkpoint::Iterator SAFplus::Checkpoint::begin()
{
  SAFplus::Checkpoint::Iterator i(this);
  assert(this->map);
  i.iter = this->map->begin();
  i.curval = &(*i.iter);
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
