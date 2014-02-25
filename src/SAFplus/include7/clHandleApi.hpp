#ifndef clHandleApi_hpp
#define clHandleApi_hpp
#include <cltypes.h>
#include <assert.h>
#include <stdio.h>

#define CL_ASSERT assert
#define SAFplus7  // for integration within the older code

namespace SAFplus
{

  typedef  enum 
    {
      PointerHandle,
      TransientHandle,
      PersistentHandle
    } HandleType;

  class HandleT
  {
  public:
    uint64_t id[2];
  public:
    char* toStr(char* buffer) const
    {
      sprintf(buffer,"%ld.%ld",id[0],id[1]);
      return buffer;
    }

    bool operator == (const HandleT& other) const
    {
      return ((id[0] == other.id[0])&&(id[1]==other.id[1]));
    }
    HandleT() { id[0] = 0; id[1] = 0; }
    HandleT(HandleType t,uint64_t idx, uint16_t process=0xffff,uint16_t node=0xffff,uint_t clusterId=0xfff)
    {
      // TODO: add all handle formats
      if (t==PersistentHandle)
	{
	  id[0]=idx;  // TODO: change to proper handle format as per docs
          id[1]=0;
	}
      else
	{
	  assert(0);
	}
    }
    static HandleT create(void);  // Get a new handle.
  };
  
  class WellKnownHandleT:public HandleT
  {
  public:
    WellKnownHandleT(uint64_t idx,uint16_t process=0xffff,uint16_t node=0xffff,uint_t clusterId=0xfff):HandleT(PersistentHandle,idx)
    {
    }
  };

  // Well Known IDs

  const WellKnownHandleT INVALID_HDL(0,0);
  const WellKnownHandleT SYS_LOG(1,0);
  const WellKnownHandleT APP_LOG(2,0);
  const WellKnownHandleT LOG_STREAM_CKPT(3,0,0);  // The checkpoint that matches log stream names to data
  const WellKnownHandleT GRP_CKPT(4,0,0);         // The checkpoint that matches group names to groups
  const WellKnownHandleT CKPT_CKPT(5,0,0);        // The checkpoint that matches names to checkpoints
  const WellKnownHandleT NAME_CKPT(6,0,0);        // The checkpoint that matches names to arbitrary data

  // Return a handle that refers to the node.  If no argument, use this node.
  HandleT getNodeHandle(int nodeNum=0);

  // Return a handle that refers to the process, if no arg use this process and/or this node.
  HandleT getProcessHandle(int pid=0, int nodeNum=0);

  // Return a handle that refers to a particular object in this process, node, cluster.
  HandleT getObjectHandle(void* object);
};

#endif
