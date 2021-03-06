#ifndef clCkptIpi_hxx
#define clCkptIpi_hxx
// Standard includes
#include <boost/thread.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash
#include <string>


// SAFplus includes
#include <clCommon.hxx>
#include <clHandleApi.hxx>
#include <clTransaction.hxx>
#include <clMsgHandler.hxx>

namespace SAFplus
{
  class Buffer;
  class Checkpoint;
  class Group;
  class MsgServer;
}

namespace SAFplusI
{
  // Predeclare internal classes so we can refer to pointers to them.
  class CkptBufferHeader;
  class BufferPtr;
  struct BufferPtrContentsEqual;

  typedef SAFplusI::BufferPtr  CkptMapKey;
  typedef SAFplusI::BufferPtr  CkptMapValue;
  //typedef SAFplus::Buffer CkptMapKey;
  //typedef SAFplus::Buffer  CkptMapValue;
  typedef std::pair<const CkptMapKey,CkptMapValue> CkptMapPair;
  typedef boost::interprocess::allocator<CkptMapValue, boost::interprocess::managed_shared_memory::segment_manager> CkptAllocator;
  //typedef boost::unordered_map < CkptMapKey, CkptMapValue, boost::hash<CkptMapKey>, std::equal_to<CkptMapValue>, CkptAllocator> CkptHashMap;
  typedef boost::unordered_map < CkptMapKey, CkptMapValue, boost::hash<CkptMapKey>, BufferPtrContentsEqual, CkptAllocator> CkptHashMap;

  typedef boost::unordered_map < SAFplus::Buffer*, uint_t> CkptOperationMap;

  class CkptSynchronization:public SAFplus::MsgHandler, public SAFplus::Wakeable
    {
    public:
    SAFplus::Mutex       atomize;
    bool                 initialized;
    bool                 synchronizing;
    bool                 syncReplica;
    SAFplus::Checkpoint* ckpt;
    SAFplus::Group*      group;  // Needs to be a pointer to break circular includes
    SAFplus::MsgServer*  msgSvr;
    int                  syncMsgSize;   // What's the preferred synchronization message size.  Messages will be < this amount OR contain only 1 key/value pair.
    int                  syncCount;
      int                  syncRecordCount;
    volatile int         syncRecvCount;
    unsigned int         syncCookie;
    boost::thread        syncThread;
    SAFplus::Handle      syncRequestActive;  // Who did we send a sync request to?


    void init(SAFplus::Checkpoint* c,SAFplus::MsgServer* pmsgSvr=NULL,SAFplus::Wakeable& execSemantics = SAFplus::BLOCK);
    void finalize();
    CkptSynchronization();
    ~CkptSynchronization();
    bool electSynchronizationReplica();  // Returns true if this process is the synchronization replica.

    void operator()();  // Thread thunk

    void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

    virtual void wake(int amt,void* cookie=NULL);

    // Send synchronization delta data to the handle
    void synchronize(unsigned int generation, unsigned int lastChange, unsigned int cookie, SAFplus::Handle response);

    // Send an update to all replicas to keep them in sync
    void sendUpdate(const SAFplus::Buffer* key,const SAFplus::Buffer* value, int change, SAFplus::Transaction& t);

    // Apply a particular received synchronization message to the checkpoint.  Returns the largest change number referenced in the message.
    unsigned int applySyncMsg(ClPtrT msg, ClWordT msglen, ClPtrT cookie);

    // Apply a particular received delete message to the checkpoint
    void applyDeleteMsg(ClPtrT msg, ClWordT msglen, ClPtrT cookie);
    };

    enum
    {
        CL_CKPT_BUFFER_HEADER_STRUCT_ID_7 = 0x59400070,
    };

    class CkptBufferHeader
    {
    public:
        uint64_t structId;
        pid_t    serverPid;     // This is the synchronization replica...
        uint32_t generation;    // If this replica's generation does not match the master, this replica's data is too old to do anything.
        uint32_t changeNum;     // This monotonically increasing value indicates when changes were made for delta synchronization.
        SAFplus::Handle handle; // The Checkpoint table handle -- identifies this checkpoint
        SAFplus::Handle replicaHandle;  // Identifies this instance of the checkpoint -- only needed if this is the synchronization replica.  Otherwise will be INVALID_HDL
        uint64_t retentionDuration; // how long (in seconds) a checkpoint data should be retained on disk if there is no call to checkpoint open
        boost::posix_time::ptime lastUsed; // the last time (in seconds) a checkpoint is used by a process
    };

   enum CkptHeader{ structId, serverPid, generation, changeNum, hdl, replicaHandle, retentionDuration, lastUsed } ;
   static const char * ckptHeaderStrings[] = {"structId", "serverPid", "generation", "changeNum", "handle", "replicaHandle", "retentionDuration", "lastUsed"};

   inline const char* ckptHeader( int val )
   {
     return ckptHeaderStrings[val];
   }

   /* This enum defines what operation performed on the checkpoint data. The purpose is to reflect only changed those items to disk. For those items unchanged, there isn't any operation on disk, too. This decreases the disk I/O operations */
   enum 
   { 
     INUP, // Insert or update checkpoint data
     DEL // Delete checkpoint data
   };

  class BufferPtr:public boost::interprocess::offset_ptr<SAFplus::Buffer>
  {
  public:
    BufferPtr(SAFplus::Buffer* b):boost::interprocess::offset_ptr<SAFplus::Buffer>(b) {}
    BufferPtr():boost::interprocess::offset_ptr<SAFplus::Buffer>() {}
  };

  struct BufferPtrContentsEqual
  {
    bool operator() (const BufferPtr& x, const BufferPtr& y) const;
    typedef BufferPtr first_argument_type;
    typedef BufferPtr second_argument_type;
    typedef bool result_type;
  };

  //BufferPtr 

  std::size_t hash_value(BufferPtr const& b);  // Actually hashes the buffer not the pointer of course

};

#endif
