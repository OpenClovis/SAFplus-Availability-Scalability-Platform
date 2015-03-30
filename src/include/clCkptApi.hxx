//? <section name="Checkpointing">
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
#include <clThreadApi.hxx>

#include <clCkptIpi.hxx>

#define	NullTMask 0x800000UL

namespace SAFplus
{

  //? <class> This class defines the checkpoint key and data storage unit.  This class defines a buffer whose data directly follows the entities defined in this class.  This allows the buffer contents to be accessed without following any pointers which is important for shared memory (since it can be mapped into different locations in process memory).  Therefore you should not use the traditional new and delete functions to allocate a buffer.  Instead use placement new operators.  See the checkpoint examples for details.
  class Buffer // TODO: rename to CkptBuffer, or integrate with messaging MsgFragment
  {
    // TODO: This is very inefficient for small buffers. The first 2 bits need to select the buffer data format and different formats removes features like the ChangeId, and provides greater or lesser numbers of length bits.
    uint32_t refAndLen; // 8 bits (highest) reference count, 2 bits to indicate whether the key and value are null terminated (strings), and 22 bits length combined into one.
    uint32_t change; 
  public:
    enum 
      {
	LenMask = 0x7fffff,
        NullTShift = 23,
        RefMask = 0xff000000,
        RefShift = 24
      };

    //? <ctor> Default constructor, constructs a zero length buffer</ctor>
    Buffer() { refAndLen=0;}
    //? <ctor> Default constructor, constructs a buffer that reports its length as _len.  But you must 'place' this object on enough memory.</ctor>
    Buffer(uint_t _len) { refAndLen = (1UL<<RefShift) | (_len&LenMask); }
    //? Return the length of the buffer
    uint_t len() const { return refAndLen&LenMask; }
    //? Return the length of this object and the following buffer combined.
    uint_t objectSize() const { return len() + sizeof(Buffer) - 1; }
    //? Set the flag that indicates that this buffer is null terminated
    void setNullT(uint_t val) { refAndLen = refAndLen & (~NullTMask) | (val<<NullTShift); }
    //? Return whether this buffer is null terminated or not.
    bool isNullT() const; // { printf("isnullt %lx %lx\n",refAndLen, refAndLen&NullTMask); return (refAndLen&NullTMask)>0; } // (((refAndLen >> NullTShift)&1) == 1);
    //? set the length of this buffer
    void setLen(uint_t len) { refAndLen = (refAndLen&0xff000000) | len&LenMask; }
    //? return the number of entities using this buffer (reference count)
    uint_t ref() const { return refAndLen >> RefShift; }
    //? Add to the number of entities using this buffer (reference count)
    void addRef(uint_t amt = 1) { refAndLen += (amt<<RefShift); }
    //? Reduce the number of entities using this buffer (reference count)
    void decRef(uint_t amt = 1) { if (ref() < amt) refAndLen &= ~RefMask; else refAndLen -= (amt<<RefShift); }
    //? Get the buffer's change number.  The change number is set every time this buffer changes and so can be used to discover whether the buffer has changed, and in what order.
    //  the current change number is per-table and incremented every time the checkpoint table changes, and is used to synchronize this checkpoint with replicas.
    uint32_t changeNum() const { return change; }
    //? Set the buffer's change number. 
    void setChangeNum(uint32_t num) { change = num; } 
    /** The buffer */
    char data[1];  // Not really length 1, this structure will be malloced and placed on a much larger buffer so data is actually len() long

    //? Clear the buffer to the passed character
    Buffer& operator = (char c)
    {
      memset(data,c,len());
    }

    //? Set the buffer to the passed null terminated string
    Buffer& operator = (const char* s)
    {
      strncpy(data,s,len());
    }

    //? Get the buffer's data
    char* get()
    {
      return data;
    }
 
    //? Get the buffer's data
    operator char* ()
    {
      return data;
    }
    
    //? Compare the contents of 2 buffers.  Return true if contents are the same.  If the buffers are of different lengths then they are not the same.
    bool operator == (Buffer const& c) const
    {
      uint_t l = len();
      //printf("compare %p (%d:%d)and %p (%d.%d)\n", this, len(), *((uint_t*)data),&c,c.len(), *((uint_t*)c.data));
      if (l != c.len()) return false;  // different sizes must not be =
      return (memcmp(data, c.data,l)==0);
    }

    //? Copy the contents of the passed buffer to this one.  Buffers must be the same length.  Flags and change count is also set, but not reference counts.
    Buffer& operator=(Buffer const& c)  
    {
      assert(len() == c.len()); // Cannot be copied due to size issues, unless lengths are the same
      setNullT(c.isNullT());
      //printf("nullt? %d %d\n", c.isNullT(),isNullT());
      memcpy(data,c.data,len());
      change = c.change;
    }

  private:
  }; //? </class>

  //? Buffers can be the key for hash tables
  inline std::size_t hash_value(Buffer const& b)
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

  //? <class> This class represents a replicated, distributed hash table
  class Checkpoint
  {
  public:
    enum
      {
	REPLICATED = 1,  //? Replicated efficiently to multiple nodes
	NESTED = 2,      //? Checkpoint values can be unique identifiers that resolve to another Checkpoint
	PERSISTENT = 4,  //? Checkpoint stores itself to disk
	NOTIFIABLE = 8,  //? You can subscribe to get notified of any changes to items in the checkpoint
	SHARED = 0x10,   //? A single instance is shared between all processes in this node
	RETAINED = 0x20, //? If all instances are closed, this checkpoint is not automatically removed
	LOCAL =    0x40, //? This checkpoint exists only on this blade.
        VARIABLE_SIZE = 0x80,  //? Pass this into maxSize to dynamically re-allocate when needed
        CHANGE_ANNOTATION = 0x100,  //? DEFAULT: Change ids are incorporated into each record.
        CHANGE_LOG        = 0x200,  //? Changes are tracked in a separate log
        EXISTING = 0x1000, //? This checkpoint must be already in existence.  In this case, no other flags need to be passed since the existing checkpoint's flags will be used. 
      };

    //? <ctor>Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created and a new handle assigned</ctor>
    Checkpoint(const Handle& handle, uint_t flags,uint_t size=0, uint_t rows=0)  
    { 
    init(handle,flags,size,rows);
    }

    //? <ctor>Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created.</ctor>
    Checkpoint(uint_t flags,uint_t size=0, uint_t rows=0)
    { 
    Handle hdl = SAFplus::Handle::create();
    init(hdl,flags,size,rows);
    }

    //? <ctor>2 step initialization.  You must call init() before using this object</ctor>
    Checkpoint():hdr(NULL),flags(0),sync(NULL) {}  
    ~Checkpoint();

    //? Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created.  This function only needs to be called if the default constructor was used to create the object
    void init(const Handle& handle, uint_t flags,uint_t size=0, uint_t rows=0,SAFplus::Wakeable& execSemantics = BLOCK);

    // TBD when name service is ready; just resolve name to a handle, or create a name->new handle->new checkpoint object mapping if the name does not exist.
    //Checkpoint(const char* name,uint_t flags);
    //Checkpoint(std::string& name,uint_t flags);

  /*? Read a section of the checkpoint table.  It is best not to hold onto Buffer references for very long; if the record is written and you are holding the Buffer reference, a new buffer will be allocated.  But if you are not holding it, it will be overwritten (which is more efficient). 
   <arg name="key">The section to read [TODO: what are the release semantics of this buffer]</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   <returns>The data that is read [TODO: what are the release semantics of the returned buffer]</returns>
   */
    const Buffer&  read (const Buffer& key); //const;
    const Buffer&  read (const uint64_t key); //const;
    const Buffer&  read (const char* key); //const;
    const Buffer&  read (const std::string& key); // const;

    /* temporarily disallow/allow synchronization */
    void syncLock() { gate.lock(); }
    void syncUnlock() { gate.unlock(); }

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
    void write(const std::string& key, const std::string& value,Transaction& t=SAFplus::NO_TXN);

    void remove(const Buffer& key,Transaction& t=SAFplus::NO_TXN);
    void remove(const uintcw_t key,Transaction& t=SAFplus::NO_TXN);
#if 0    
    void remove(const SAFplusI::BufferPtr& bufPtr, bool isKey=false, Transaction& t=SAFplus::NO_TXN);
    void remove(Buffer* buf, bool isKey=false, Transaction& t=SAFplus::NO_TXN);
#endif    

    /** During replication, changes are received from the remote
    checkpoint.  This API applies these changes */
    void applySync(const Buffer& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);


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

      friend class Checkpoint;
      protected:
      SAFplusI::CkptHashMap::iterator iter;
    };

    // the begin and end of the iterator look up
    Iterator begin();
    Iterator end();

    const SAFplus::Handle& handle() { return hdr->handle; } // its read only
    std::string                    name;

    // debugging
    void dump();
    static void dbgRemove(char* name);
    void stats();
  protected:
    friend class SAFplusI::CkptSynchronization;
    //boost::interprocess::shared_memory_object* sharedMemHandle;
    //boost::interprocess::mapped_region       mr;
    boost::interprocess::managed_shared_memory msm;
    SAFplusI::CkptBufferHeader*     hdr;
    SAFplusI::CkptHashMap*          map;
    uint_t                        flags;
    SAFplus::ProcGate              gate;
    bool isSyncReplica;
    SAFplusI::CkptSynchronization* sync;  // This is a separate object (and pointer) to break the synchronization requirements (messaging and groups) from the core checkpoint
  }; //? </class>
}

#endif
//? </section>
