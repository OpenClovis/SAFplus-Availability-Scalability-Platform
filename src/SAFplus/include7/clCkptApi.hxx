#ifndef clCkptApi_hxx
#define clCkptApi_hxx
// Standard includes
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clTransaction.hxx>

#include <clCkptIpi.hxx>

namespace SAFplus
{

  class Buffer
  {
    uint32_t refAndLen; // 8 bits (highest) reference count and 24 bits length combined into one.
  public:
    Buffer() { refAndLen=0;}
    Buffer(uint_t _len) { refAndLen = (1UL<<24) | (_len&0x00ffffff); }
    uint_t len() const { return refAndLen&0x00ffffff; }
    void setLen(uint_t len) { refAndLen = (refAndLen&0xff000000) | len&0x00ffffff; }
    uint_t ref() const { return refAndLen >> 24; }
    void addRef(uint_t amt = 1) { refAndLen += (amt<<24); }
    void decRef(uint_t amt = 1) { if (ref() < amt) refAndLen &= 0x00ffffff; else refAndLen -= (amt<<24); }

    /** The buffer */
    char data[1];  // Not really length 1, this structure will be malloced and placed on a much larger buffer so data is actually len() long

    Buffer& operator = (char c)
    {
      memset(data,c,len());
    }

    Buffer& operator = (const char* s)
    {
      strncpy(data,s,len());
    }
 
    operator char* ()
    {
      return data;
    }
    
    bool operator == (Buffer const& c) const
    {
      uint_t l = len();
      //printf("compare %p (%d:%d)and %p (%d.%d)\n", this, len(), *((uint_t*)data),&c,c.len(), *((uint_t*)c.data));
      if (l != c.len()) return false;  // different sizes must not be =
      return (memcmp(data, c.data,l)==0);
    }

    Buffer& operator=(Buffer const& c)  // Cannot be copied due to size issues, unless lengths are the same
    {
      assert(len() == c.len());
      memcpy(data,c.data,len());
    }

  private:

  };

    std::size_t hash_value(Buffer const& b)
    {
        boost::hash<int> hasher;
        std::size_t seed = 0;
        for (const char* c = b.data; c< b.data+b.len(); c++)  // terribly inefficient
	  {
          boost::hash_combine(seed, *c);
	  }
        //printf("hash %p (%d:%d) = %lu\n", &b, b.len(), *((uint_t*)b.data),seed);
	return seed;
    }    

  class Checkpoint
  {
  public:
    enum
      {
	REPLICATED = 1,  // Replicated efficiently to multiple nodes
	NESTED = 2,      // Checkpoint values can be unique identifiers that resolve to another Checkpoint
	PERSISTENT = 4,  // Checkpoint stores itself to disk
	NOTIFIABLE = 8,  // You can subscribe to get notified of any changes to items in the checkpoint
	SHARED = 0x10,   // A single instance is shared between all processes in this node
	RETAINED = 0x20, // If all instances are closed, this checkpoint is not automatically removed
	LOCAL =    0x40, // This checkpoint exists only on this blade.
        VARIABLE_SIZE = 0x80,  // Pass this into maxSize to dynamically re-allocate when needed

        EXISTING = 0x1000, // This checkpoint must be already in existence.  In this case, no other flags need to be passed since the existing checkpoint's flags will be used. 
      };

    Checkpoint(const Handle& handle, uint_t flags,uint_t size=0, uint_t rows=0)  // Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created and a new handle assigned
    { init(handle,flags,size,rows);}
    
    Checkpoint(uint_t flags,uint_t size=0, uint_t rows=0)  // Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created.
    { init(SAFplus::Handle::create(),flags,size,rows);}

    Checkpoint():hdr(NULL),flags(0) {}  // 2 step initialization
    void init(const Handle& handle, uint_t flags,uint_t size=0, uint_t rows=0);

    // TBD when name service is ready; just resolve name to a handle, or create a name->new handle->new checkpoint object mapping if the name does not exist.
    //Checkpoint(const char* name,uint_t flags);
    //Checkpoint(std::string& name,uint_t flags);

  /**
   \brief Read a section of the checkpoint table  
   \param key The section to read
   \param value The read data is put here
   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF read error
      that cannot be automatically handled
   */
    const Buffer&  read (const Buffer& key) const;
    const Buffer&  read (const uint64_t key) const;
    const Buffer&  read (const char* key) const;
    const Buffer&  read (const std::string& key) const;

  /**
   \brief Write a section of the checkpoint table 
   \param key The section to write
   \param value The data to write
   \par Exceptions
      clAsp::Error is raised if there is an underlying SAF write error
      that cannot be automatically handled
   */    
    void write(const Buffer& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);
    void write(const uintcw_t key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);
    void write(const char* key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);
    void write(const std::string& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);


    typedef SAFplusI::CkptMapPair KeyValuePair;

    //void registerChangeNotification(Callback &c, void* cookie);
 
    class Iterator
    {
    protected:

    public:
      Iterator(Checkpoint* ckpt);
      ~Iterator();

      //Iterator(Buffer *pData, Buffer *pKey);

      // comparison
      bool operator !=(const Iterator& otherValue) const;

      // increment the pointer to the next value
      Iterator& operator++();
      Iterator& operator++(int);

      KeyValuePair& operator*() { return *curval; }
      const KeyValuePair& operator*() const { return *curval; }
      KeyValuePair* operator->() { return curval; }
      const KeyValuePair* operator->() const { return curval; }
      
      //void getCkptData(void);
      Checkpoint* ckpt;
      SAFplusI::CkptMapPair* curval;
//Buffer *pData;
//Buffer *pKey;
      SAFplusI::CkptHashMap::iterator iter;
    };

    // the begin and end of the iterator look up
    Iterator begin();
    Iterator end();

    // debugging
    void dump();
    static void remove(char* name);
    void stats();
  protected:
    //boost::interprocess::shared_memory_object* sharedMemHandle;
    //boost::interprocess::mapped_region       mr;
    boost::interprocess::managed_shared_memory msm;
    SAFplusI::CkptBufferHeader*     hdr;
    SAFplusI::CkptHashMap*          map;
    uint_t                flags;
  };
}

#endif
