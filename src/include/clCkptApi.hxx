//? <section name="Checkpointing">

/*? <desc order="2"><html><p>Checkpointing is the act of preserving critical program data by writing it to persistent storage or to another node.  By checkpointing data, recovery from a failure can proceed from the point of the last checkpoint.  Checkpointing is implemented via a fully-replicated distributed hash table abstraction: write data to the hash table in one process and it can be read in the backup process.</p>
<p>
The SAFplus checkpoint service is implemented by creating a single checkpoint entity in shared memory per node.  This makes data sharing between processes in the same node extremely efficient.  Within each node, programs that have opened the checkpoint elect a "node representative" whose has the job of listening for checkpoint updates from other nodes, and applying them into shared memory.  However "node representative" is not involved in the write process; for speed, every process that writes the checkpoint also sends a checkpoint update message to all other nodes.</p>
<p>
Typically, a single active process writes the checkpoint and other standby entities read it.  If multiple processes need to write the checkpoint, it is the application programmer's task to ensure that these writes do not both write the same data nearly-simultaneously (collide).  In the case of a collision, the contents of that entry in the checkpoint may vary between nodes.  The application programmer can avoid collisions via data sharding or an application level synchronization mechanism.
</p>
</html>
</desc>
*/

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
#include <clCustomization.hxx>
#include <clDbalBase.hxx>


#define	NullTMask 0x800000UL

#define SharedMemPath "/dev/shm/"

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

  //? <class> This class represents a replicated, distributed hash table.  The same checkpoint can be opened on multiple nodes in the cluster to share data.
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
    Checkpoint(const Handle& handle, uint_t flags, uint64_t retentionDuration=SAFplusI::CkptRetentionDurationDefault, uint_t size=0, uint_t rows=0)  
    { 
    init(handle,flags,retentionDuration,size,rows);
    }

    //? <ctor>Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created.</ctor>
    // <arg name="flags">The flavor of checkpoint to open.  See the constants for the options.  All entities opening the checkpoint must use the same flags.</arg>
    // <arg name="retentionDuration">The checkpoint data will not be retained if there isn't any process opening it within this argument. The retention timer will start as soon as the last call to checkpoint close .</arg>
    // <arg name="size">This is the amount of shared memory that will be allocated to hold this checkpoint.  If the checkpoint already exists, this value will be ignored.</arg>
    // <arg name="rows">The maximum number of items to be stored in the checkpoint table</arg>
    Checkpoint(uint_t flags, uint64_t retentionDuration=SAFplusI::CkptRetentionDurationDefault, uint_t size=0, uint_t rows=0)
    { 
    Handle hdl = SAFplus::Handle::create();
    init(hdl,flags,retentionDuration,size,rows);
    }

    //? <ctor>2 step initialization.  You must call init() before using this object</ctor>
    Checkpoint():hdr(NULL),flags(0),sync(NULL) {}  
    ~Checkpoint();

    //? Create a new checkpoint or open an existing one.  If no handle is passed, a new checkpoint will be created.  This function only needs to be called if the default constructor was used to create the object
    // <arg name="flags">The flavor of checkpoint to open.  See the constants for the options.  All entities opening the checkpoint must use the same flags.</arg>
    // <arg name="retentionDuration">The checkpoint data will not be retained if there isn't any process opening it within this argument. The retention timer will start as soon as the last call to checkpoint close .</arg>
    // <arg name="size">This is the amount of shared memory that will be allocated to hold this checkpoint.  If the checkpoint already exists, this value will be ignored.</arg>
    // <arg name="rows">The maximum number of items to be stored in the checkpoint table</arg>
    // <arg name="execSemantics" default="BLOCK">[OPTIONAL] specify blocking or nonblocking execution semantics for this function</arg>
    void init(const Handle& handle, uint_t flags,uint64_t retentionDuration=SAFplusI::CkptRetentionDurationDefault, uint_t size=0, uint_t rows=0,SAFplus::Wakeable& execSemantics = BLOCK);

    // TBD when name service is ready; just resolve name to a handle, or create a name->new handle->new checkpoint object mapping if the name does not exist.
    //Checkpoint(const char* name,uint_t flags);
    //Checkpoint(std::string& name,uint_t flags);

  /*? Read a section of the checkpoint table.  It is best not to hold onto Buffer references for very long; if the record is written and you are holding the Buffer reference, a new buffer will be allocated to avoid modifying your reference.  But if you are not holding it, it will be overwritten (which is more efficient). 
   <arg name="key">The section to read [TODO: what are the release semantics of this buffer]</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   <returns>The data that is read [TODO: what are the release semantics of the returned buffer]</returns>
   */
    const Buffer&  read (const Buffer& key); //const;

   /*? Read a section of the checkpoint table, using an integer key.  It is best not to hold onto Buffer references for very long; if the record is written and you are holding the Buffer reference, a new buffer will be allocated  to avoid modifying your reference.  But if you are not holding it, it will be overwritten (which is more efficient). 
   <arg name="key">The section to read</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   <returns>The data that is read [TODO: what are the release semantics of the returned buffer]</returns>
   */
    const Buffer&  read (const uint64_t key); //const;

   /*? Read a section of the checkpoint table, using a null-terminated string key.  It is best not to hold onto Buffer references for very long; if the record is written and you are holding the Buffer reference, a new buffer will be allocated to avoid modifying your reference.  But if you are not holding it, it will be overwritten (which is more efficient). 
   <arg name="key">The section to read</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   <returns>The data that is read [TODO: what are the release semantics of the returned buffer]</returns>
   */
    const Buffer&  read (const char* key); //const;

   /*? Read a section of the checkpoint table, using a string key.  It is best not to hold onto Buffer references for very long; if the record is written and you are holding the Buffer reference, a new buffer will be allocated to avoid modifying your reference.  But if you are not holding it, it will be overwritten (which is more efficient). 
   <arg name="key">The section to read</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   <returns>The data that is read [TODO: what are the release semantics of the returned buffer]</returns>
   */
    const Buffer&  read (const std::string& key); // const;


  /*? Write a section of the checkpoint table 
   <arg name="key">The section to write</arg>
   <arg name="value">The data to write</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>   
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>   
   */ 
    void write(const Buffer& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);
 
 /*? Write a section of the checkpoint table 
   <arg name="key">The section to write</arg>
   <arg name="value">The data to write</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   */ 
    void write(const uintcw_t key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);

 /*? Write a section of the checkpoint table 
   <arg name="key">The section to write</arg>
   <arg name="value">The data to write</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   */ 
    void write(const char* key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);

 /*? Write a section of the checkpoint table 
   <arg name="key">The section to write</arg>
   <arg name="value">The data to write</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   */ 
    void write(const std::string& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);

 /*? Write a section of the checkpoint table 
   <arg name="key">The section to write</arg>
   <arg name="value">The data to write</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   <exception name="SAFplus::Error">Raised if there is an underlying SAF read error that cannot be automatically handled</exception>
   */ 
    void write(const std::string& key, const std::string& value,Transaction& t=SAFplus::NO_TXN);

 /*? Delete a section from the checkpoint table 
   <arg name="key">The section to delete</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   */ 
    void remove(const Buffer& key,Transaction& t=SAFplus::NO_TXN);

 /*? Delete a section from the checkpoint table 
   <arg name="key">The section to delete</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   */ 
    void remove(const uintcw_t key,Transaction& t=SAFplus::NO_TXN);
 /*? Delete a section from the checkpoint table 
   <arg name="key">The section to delete</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   */ 
    void remove(const char* key, Transaction& t=SAFplus::NO_TXN);
 /*? Delete a section from the checkpoint table 
   <arg name="key">The section to delete</arg>
   <arg name="t" default="NO_TXN">[OPTIONAL, NOT IMPLEMENTED] Write this record as part of a transaction commit, rather than immediately</arg>
   */ 
    void remove(const std::string& key, Transaction& t=SAFplus::NO_TXN);
#if 0    
    void remove(const SAFplusI::BufferPtr& bufPtr, bool isKey=false, Transaction& t=SAFplus::NO_TXN);
    void remove(Buffer* buf, bool isKey=false, Transaction& t=SAFplus::NO_TXN);
#endif    

    //? Get a number which changes whenever the checkpoint changes
    uint_t getChangeNum();
    //? Block for a time or until the checkpoint is changed.  If the checkpoint is closed (destructed), that is considered a change.
    bool wait(uint_t changeNum, uint64_t timeout=ULLONG_MAX);

    /* During replication, changes are received from the remote checkpoint.  This API applies these changes */
    void applySync(const Buffer& key, const Buffer& value,Transaction& t=SAFplus::NO_TXN);

    /* temporarily disallow/allow synchronization */
    // NOT USED: void syncLock() { gate.lock(); }
    // NOT USED: void syncUnlock() { gate.unlock(); }

    //? Reflects all changes from checkpoint data to disk (database)
    void flush();

    typedef SAFplusI::CkptMapPair KeyValuePair;

    //void registerChangeNotification(Callback &c, void* cookie);
 
    //? <class> Iterate through all sections in a checkpoint table.  Like STL iterators, this object acts like a pointer where the obj->first gets the key, and obj->second get the value of the section the iterator is visiting.
    class Iterator
    {
    protected:

    public:
      Iterator(Checkpoint* ckpt);
      ~Iterator();

      //Iterator(Buffer *pData, Buffer *pKey);

      //? comparison
      bool operator !=(const Iterator& otherValue) const;

      //? go to the next section (row) in the checkpoint table
      Iterator& operator++();
      Iterator& operator++(int);

      //? Get the key and value of the currently visited section with (*it).first and (*it).second respectively
      KeyValuePair& operator*() { return *curval; }
      const KeyValuePair& operator*() const { return *curval; }
      //? Get the key and value of the currently visited section with it->first and it->second respectively
      KeyValuePair* operator->() { return curval; }
      const KeyValuePair* operator->() const { return curval; }

      //void getCkptData(void);
      Checkpoint* ckpt;
      SAFplusI::CkptMapPair* curval;

      friend class Checkpoint;
      protected:
      SAFplusI::CkptHashMap::iterator iter;
    }; //? </class>

    //? Similar to the Standard Template Library (STL), this function starts iteration through the members of this group.
    Iterator begin();
    //? Similar to the Standard Template Library (STL), this returns the termination sentinel -- that is, an invalid object that can be compared to an iterator to determine that it is at the end of the list of members.
    Iterator end();
    //? Similar to the Standard Template Library (STL), this returns the iterator of the map designated by the key -- data of the key stored in the map
    Iterator find(const void* key, uint_t len);
    Iterator find(const Buffer& key);
    //? Return the universal identifier for this checkpoint.
    const SAFplus::Handle& handle() { return hdr->handle; } // its read only
    //? Get the name of the checkpoint
    std::string                    name;

    //? [DEBUGGING ONLY]: print out the keys and values in this checkpoint.
    void dump();
    //? [DEBUGGING ONLY]: Unilaterally delete this checkpoint out of shared memory... behavior is undefined if other processes have the checkpoint open.
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
    
    DbalPlugin* pDbal; //Dbal plugin object to manipulate with database. Basically, each persistent checkpoint data needs to be stored as a table on disk, so, if a persistent checkpoint is opened, dbal plugin object will be initialized. If it's not a persistent one, dbal plugin is NULL (not used)
    SAFplusI::CkptOperationMap operMap; /* Holding operation performed on the checkpoint data items identified by key. The purpose is to reflect only changed those items to disk. For those items unchanged, there isn't any operation on disk, too. This decreases the disk I/O operations */

  protected:    
    void syncFromDisk(); // In the case that user opens a non-existing checkpoint but its data is available on disk, so this function copies data from database to checkpoint data to ensure data between them to be synchronized
    void initDB(const char* ckptId, bool isCkptExist); // Initialize dbal and synchronize data from disk with checkpoint if any
    void writeCkpt(ClDBKeyHandleT key, ClUint32T keySize, ClDBRecordHandleT rec, ClUint32T recSize);
    void writeCkptData(ClDBKeyHandleT key, ClUint32T keySize, ClDBRecordHandleT rec, ClUint32T recSize);
    void writeHdrDB(int hdrKey, ClDBRecordT rec, ClUint32T recSize);
    void writeDataDB(Buffer* key, Buffer* val);
    void deleteDataDB(Buffer* key);
    void updateOperMap(Buffer* key, uint_t oper);
   #if 0
   public:
    void dumpHeader();
   #endif
  }; //? </class>
}

#endif
//? </section>
