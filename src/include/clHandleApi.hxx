/*? Handles are the mechanism by which nodes, processes or objects identify eachother within the cluster 
*/
#ifndef clHandleApi_hpp
#define clHandleApi_hpp
#include <cltypes.h>
#include <assert.h>
#include <stdio.h>
#include <string>
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

  #define SUB_HDL_SHIFT                8  // Non-pointer handles can have "well-known" subhandle offsets.  For example, if a checkpoint handle is 0x56, its associated group can always be handle 0x5601 (offset 1 from the base handle).  To accomplish this, "base" handles are incremented by 256 when issued

  //?  Defines the format and semantics of the handle.
  typedef  enum 
    {
      PersistentHandle=0,  //? This handle is the same across cluster, process and node restarts.  It is either well known (defined in the code) or uses AMF entity ids, both of which persist.
      PointerHandle,       //? This transient handle references an object inside a process by pointer.
      TransientHandle,     //? This handle uses slot numbers and message port numbers or process ids so won't be the same across restarts.  Rather than a pointer to an object, it simple increments an integer to make each handle unique.
    } HandleType;

  //? <class> The handle class is the universal mechanism to reference an entity within the cluster.  It is also the address used to send messages within the cluster (<ref type="method">MsgServer.SendMsg</ref>, and with the <ref type="class">ObjectMessager</ref> facility can be used to send messages directly to classes that implement <ref type="class">ObjectMessager</ref>.  A Handle can refer to a cluster, node or process by using 0xffff for the unused fields.  It can refer to an object by pointer, by well-known id, or by automatically assigned ID.  Applications can discover eachother's id via the <ref type="class">Group</ref> service, could pass exchange handles via the message service, or could associate a well-known string with the Handle using the <ref type="class">NameRegistrar</ref> service.
  class Handle
  {
  public:    
    uint64_t id[2];
  public:

    enum  
      {
      AllNodes = 0xffff, //? A handle with this in the node field refers to all nodes (messages will be broadcast, for example).
      AllPorts = 0xffff, //? [TODO] A handle with this in the node field refers to all SAFplus ports (messages will be sent to every process, for example).

      StrLen   = 16*2+2+4, //? To convert to a string representation, allocate at least this many bytes (actually this adds 4 extra "just in case" bytes as padding)

      ThisCluster = 0,  //? Handle refers to the current cluster.  Be careful when doing inter-cluster communications; handle will NOT be "patched" so on the other side it points to the destination cluster.
      ThisNode = 0,     //? Handle refers to the current node.  Be careful when doing inter-node communications; handle will NOT be "patched" so on the other side it points to the destination node.
      ThisProcess = 0,  //? Handle refers to the current process.  Be careful when doing inter-process communications; handle will NOT be "patched" so on the other side it points to the destination process.
      };

    //? Convert this handle to a string representation.  To print handles, it is important to use this function or the PRIx64 macro: printf("%" PRIx64 ":%" PRIx64,handle.id[0],handle.id[1]).  Otherwise your code will not be portable between 32 and 64 bit architectures.
    // <arg name="buffer"></arg>Pass in the buffer to write the handle into.  Should be at least <ref type="const">StrLen</ref> bytes.
    // <returns>The buffer you passed in</returns>
    char* toStr(char* buffer) const
    {
      sprintf(buffer,"%" PRIx64 ":%" PRIx64,id[0],id[1]);
      return buffer;
    }

   //? Convert this string to a handle representation.  The string argument has the format aaaaa:bbbbb
    // for example if strHdl = "23abf777:6700ccd", this is converted to Handle object in which
    // {id[0] = 23abf777,id[1] = 6700ccd}
    // <arg name="strHdl"></arg>Pass
    // <returns>The handle itself</returns>
    Handle& fromStr(const char* strHdl)
    {
      std::string hdl(strHdl);
      return fromStr(hdl);
    }

    Handle& fromStr(const std::string& strHdl)
    {
      int pos = strHdl.find(":");
      if (pos >= 0 && pos < strHdl.length())
      {      
        std::string id0 = strHdl.substr(0, pos);
        std::string id1 = strHdl.substr(pos+1, strHdl.length()-pos);
        sscanf(id0.c_str(), "%" PRIx64, &id[0]);         
        sscanf(id1.c_str(), "%" PRIx64, &id[1]);        
      }      
      return *this;
    }

    //? Are handles equal?
    bool operator == (const Handle& other) const
    {
      return ((id[0] == other.id[0])&&(id[1]==other.id[1]));
    }

    //? Are handles not equal?
    bool operator != (const Handle& other) const
    {
      return ((id[0] != other.id[0])||(id[1]!=other.id[1]));
    }

    bool operator < (const Handle& other) const
    {
      return((id[0] < other.id[0])||((id[0] == other.id[0])&&(id[1] < other.id[1])));
    }

    //? Assign handles
    Handle& operator = (const Handle& other)
    {
      id[0] = other.id[0];
      id[1] = other.id[1];
      return *this;
    }

    //? <ctor> Initializes this handle to <ref type="variable">INVALID_HDL</ref></ctor>
    Handle() { id[0] = 0; id[1] = 0; }

    //? <ctor> Initializes the handle.  This is the detailed initializer.  Typically applications would use the <ref type="function">getNodeHandle()</ref>,  <ref type="function">getProcessHandle()</ref>,<ref type="function">getObjectHandle()</ref>, or <ref type="method">create</ref> functions to create handles.</ctor>
    Handle(HandleType t,uint64_t idx, uint32_t process=0xffffffff,uint16_t node=0xffff,uint_t clusterId=0xfff)
    {
      uint64_t id0 = (( ((uint64_t)t) << HDL_TYPE_ID_SHIFT) & HDL_TYPE_ID_MASK)
                   | (( ((uint64_t)clusterId) << HDL_CLUSTER_ID_SHIFT) & HDL_CLUSTER_ID_MASK)
                   | (( ((uint64_t)node) << HDL_NODE_ID_SHIFT) & HDL_NODE_ID_MASK)
                   | (  ((uint64_t)process) & HDL_PROCESS_ID_MASK);

      id[0]=id0;
      id[1]=idx;
    }

    /*? Normal handles are handed out with gaps (currently 256). Subhandles are simply a handle within the gap.  An application can designate an arbitrary offset to refer to a related handle.  For example, if application's handle is X, it could designate its Checkpoint table as X+1, and its Group as handle X+2.  However it should not be inferred from this example that these entities MUST have different handles.  It is also possible to use the same handle for multiple entities if they can be unambiguously distinguished by context (for example).
    <arg name="offset">the requested sub-handle index</arg>
    */
    Handle getSubHandle(unsigned int offset)
      {
      assert((id[1]&((1<<SUB_HDL_SHIFT)-1))==0);  // Ensure that this is not already a subhandle (or pointer handle)
      assert(offset <  (1<<SUB_HDL_SHIFT));  // Ensure that the subhandle index is valid
      Handle subHdl = *this;
      subHdl.id[1] += offset;
      return subHdl;
      }

    //? Get the handle type
    HandleType getType() const
    {
      HandleType t = (HandleType)((id[0] & HDL_TYPE_ID_MASK) >> HDL_TYPE_ID_SHIFT);
      return t;
    }

    //? Get the index or object pointer portion of this handle.
    uint64_t getIndex() const
    {
      return id[1];
    }

    //? Get the process or port portion of this handle (synonym of <ref type="method">getPort</ref>).
    uint32_t getProcess() const
    {
      uint32_t process = (uint32_t)(id[0] & HDL_PROCESS_ID_MASK);
      return process;
    }

    //? Get the process or port portion of this handle (synonym of <ref type="method">getProcess</ref>).
    uint32_t getPort() const  // Process ID and port have the same function... unique # per node
    {
      uint32_t process = (uint32_t)(id[0] & HDL_PROCESS_ID_MASK);
      return process;
    }

    //? Get the node portion of this handle.
    uint16_t getNode() const
    {
      uint16_t node = (uint16_t)((id[0] & HDL_NODE_ID_MASK) >> HDL_NODE_ID_SHIFT);
      return node;
    }

    //? Get the cluster portion of this handle.
    uint_t getCluster() const
    {
      uint_t clusterId = (uint_t)((id[0] & HDL_CLUSTER_ID_MASK) >> HDL_CLUSTER_ID_SHIFT);
      return clusterId;
    }

    static Handle create(int msgingPort=0);  //? Get a new handle. msgingPort should be the IOC port number if you want to receive messages on this handle, otherwise a unique # (pid).  By default, your SAFplus messaging port is used.
    static Handle createPersistent(int msgingPort=0);  //? Get a new persistent handle. A persistent handle is not removed when the creating process fails.  msgingPort should be the IOC port number if you want to receive messages on this handle, otherwise a unique # (pid).  By default, your SAFplus messaging port is used.
    static Handle create(uint64_t id0,uint64_t id1); //? Translate 2 numbers to a valid handle (for serialization/deserialization)
    static uint64_t uniqueId(void);  //? Get a unique-in-this-process number for making your own handle
  }; 

  // These aren't constructors in the c++ sense, but they are in a real sense.  So let's put them in the Handle's constructor category
  //? <ctor>Return a handle that refers to the node.  If no argument, use this node's actual id.</ctor>
  Handle getNodeHandle(int nodeNum=0);

  //? <ctor>Return a handle that refers to just the cluster/node/process (strip out the object id).</ctor>
  inline Handle getNodeHandle(Handle hdl)
  {
    Handle ret;
    ret.id[0] = hdl.id[0]&(HDL_CLUSTER_ID_MASK | HDL_NODE_ID_MASK);
    ret.id[1] = 0;
    return ret;
  }

  //? <ctor>Return a handle that refers to the process, if no arg use this process's SAFplus port and/or this node's actual id.</ctor>
  Handle getProcessHandle(int pid=0, int nodeNum=0);

  //? <ctor>Return a handle that refers to just the cluster/node/process (strip out the object id).</ctor>
  inline Handle getProcessHandle(Handle hdl)
  {
    Handle ret;
    ret.id[0] = hdl.id[0];
    ret.id[1] = 0;
    return ret;
  }

  //? <ctor>[TODO] Return a handle that refers to a particular object in this process, node, cluster.</ctor>
  Handle getObjectHandle(void* object);

//? </class>
  
  //? Handles can be used as keys in hash tables
  inline std::size_t hash_value(Handle const& h)
  {
     boost::hash<uint64_t> hasher;        
     return hasher(h.id[0]|h.id[1]);
  }     

  //? <class>This class creates "well-known" (i.e. defined in code) handles.  The set of SAFplus well-known handles are also described in this file.
 class WellKnownHandle:public Handle
  {
  public:
    WellKnownHandle(uint64_t idx,uint32_t process=0xffffffff,uint16_t node=0xffff,uint_t clusterId=0xfff):Handle(PersistentHandle,idx<<SUB_HDL_SHIFT,process,node,clusterId)
    {
    }
 }; //? </class>


  // Well Known IDs

  const WellKnownHandle INVALID_HDL(0,0,0,0);  //? This handle does not refer to any entity
  const WellKnownHandle SYS_LOG(1,0);    //? This handle refers to the system log on this cluster/node.
  const WellKnownHandle APP_LOG(2,0);    //? This handle refers to the application log on this cluster/node.
  const WellKnownHandle TEST_LOG(3,0);   //? This handle refers to the test results log on this cluster/node.
  const WellKnownHandle LOG_STREAM_CKPT(4,0,0);  //? The checkpoint that matches log stream names to data
  const WellKnownHandle GRP_CKPT(5,0,0);         //? The checkpoint that matches group names to groups
  // const WellKnownHandle CKPT_CKPT(6,0,0);        //? The checkpoint that matches names to checkpoints
  const WellKnownHandle NAME_CKPT(7,0,0);        //? The checkpoint that matches names to arbitrary data

  const WellKnownHandle CLUSTER_GROUP(8,0,0);    //? This group represents all nodes (AMF instances) running in the cluster
  const WellKnownHandle FAULT_GROUP(9,0,0);      //? This group represents the fault managers
  const WellKnownHandle FAULT_CKPT(10,0,0);      //? This checkpoint is how fault synchronizes data
  const WellKnownHandle MGT_GROUP(11,0,0);       //? This group represents all mgt clients (object implementers)
  const WellKnownHandle MGT_CKPT(15,0,0);       //? This group represents all mgt clients (object implementers)
  const WellKnownHandle EVENT_GROUP(14,0,0);       //? This group represents all event


  const WellKnownHandle CLOUD_DISCOVERY_GROUP(12,0,0); //? This group represents all nodes in the cloud
  const WellKnownHandle EVENT_CKPT(13,0,0);       //? The checkpoint that matches MGT object to arbitrary data

  //? Return a new handle that uses a "well known" id but is qualified by a specific node and port.  By convention, this handle will be used for the entity that implements a well known service on a particular node
  inline Handle wellKnownEntity(const WellKnownHandle& h, int port, int node,uint_t clusterId=0)
  {
    Handle ret(SAFplus::PersistentHandle,h.getIndex(),port,node,0);
    return ret;
  }

  enum
    {
     AppWellKnownIdStart = 1024,  //? Application programmers can start creating well-known handles from this value
    };

};

#endif
