#ifndef clHandleApi_hpp
#define clHandleApi_hpp
#include <cltypes.h>
#include <assert.h>
#include <stdio.h>

#include <boost/functional/hash.hpp> 

#define CL_ASSERT assert
#ifndef SAFplus7
#define SAFplus7  // for integration within the older code
#endif

namespace SAFplus
{

  #define HDL_TYPE_ID_MASK            0xF000000000000000ULL
  #define HDL_CLUSTER_ID_MASK         0x0FFF000000000000ULL
  #define HDL_NODE_ID_MASK            0x0000FFFF00000000ULL
  #define HDL_PROCESS_ID_MASK         0x00000000FFFFFFFFULL

  #define HDL_TYPE_ID_SHIFT            60
  #define HDL_CLUSTER_ID_SHIFT         48
  #define HDL_NODE_ID_SHIFT            32

  typedef  enum 
    {
      PersistentHandle=0, // this handle uses AMF entity ids so persists.
      PointerHandle,
      TransientHandle, // this handle uses slot numbers and process ids so won't be the same across restarts
    } HandleType;

  class Handle
  {
  public:    
    uint64_t id[2];
  public:
    char* toStr(char* buffer) const
    {
      sprintf(buffer,"%lx.%lx",id[0],id[1]);
      return buffer;
    }

    bool operator == (const Handle& other) const
    {
      return ((id[0] == other.id[0])&&(id[1]==other.id[1]));
    }    

    bool operator != (const Handle& other) const
    {
      return ((id[0] != other.id[0])||(id[1]!=other.id[1]));
    }

    Handle() { id[0] = 0; id[1] = 0; }
    Handle(HandleType t,uint64_t idx, uint32_t process=0xffffffff,uint16_t node=0xffff,uint_t clusterId=0xfff)
    {
      uint64_t id0 = (( ((uint64_t)t) << HDL_TYPE_ID_SHIFT) & HDL_TYPE_ID_MASK)
                   | (( ((uint64_t)clusterId) << HDL_CLUSTER_ID_SHIFT) & HDL_CLUSTER_ID_MASK)
                   | (( ((uint64_t)node) << HDL_NODE_ID_SHIFT) & HDL_NODE_ID_MASK)
                   | (  ((uint64_t)process) & HDL_PROCESS_ID_MASK);

      id[0]=id0;
      id[1]=idx;
    }

    HandleType getType()
    {
      HandleType t = (HandleType)((id[0] & HDL_TYPE_ID_MASK) >> HDL_TYPE_ID_SHIFT);
      return t;
    }

    uint64_t getIndex()
    {
      return id[1];
    }

    uint32_t getProcess()
    {
      uint32_t process = (uint32_t)(id[0] & HDL_PROCESS_ID_MASK);
      return process;
    }

    uint16_t getNode()
    {
      uint16_t node = (uint16_t)((id[0] & HDL_NODE_ID_MASK) >> HDL_NODE_ID_SHIFT);
      return node;
    }

    uint_t getCluster()
    {
      uint_t clusterId = (uint_t)((id[0] & HDL_CLUSTER_ID_MASK) >> HDL_CLUSTER_ID_SHIFT);
      return clusterId;
    }

    static Handle create(void);  // Get a new handle.
  };
  
  inline std::size_t hash_value(Handle const& h)
  {
     boost::hash<uint64_t> hasher;        
     return hasher(h.id[0]|h.id[1]);
  }     

 class WellKnownHandle:public Handle
  {
  public:
    WellKnownHandle(uint64_t idx,uint32_t process=0xffffffff,uint16_t node=0xffff,uint_t clusterId=0xfff):Handle(PersistentHandle,idx,process,node,clusterId)
    {
    }
  };

  // Well Known IDs

  const WellKnownHandle INVALID_HDL(0,0,0,0);
  const WellKnownHandle SYS_LOG(1,0);
  const WellKnownHandle APP_LOG(2,0);
  const WellKnownHandle TEST_LOG(3,0);
  const WellKnownHandle LOG_STREAM_CKPT(4,0,0);  // The checkpoint that matches log stream names to data
  const WellKnownHandle GRP_CKPT(5,0,0);         // The checkpoint that matches group names to groups
  const WellKnownHandle CKPT_CKPT(6,0,0);        // The checkpoint that matches names to checkpoints
  const WellKnownHandle NAME_CKPT(7,0,0);        // The checkpoint that matches names to arbitrary data

  const WellKnownHandle CLUSTER_GROUP(8,0,0);    // This group represents nodes (AMF instances) running in the cluster

  // Return a handle that refers to the node.  If no argument, use this node.
  Handle getNodeHandle(int nodeNum=0);

  // Return a handle that refers to the process, if no arg use this process and/or this node.
  Handle getProcessHandle(int pid=0, int nodeNum=0);

  // Return a handle that refers to a particular object in this process, node, cluster.
  Handle getObjectHandle(void* object);
};

#endif
