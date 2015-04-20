#include <clClusterNodes.hxx>
#include <clCustomization.hxx>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <clMsgBase.hxx>

using namespace boost::interprocess;
using namespace boost;

namespace SAFplus
{
  const char* ClusterNodesSharedMemoryName = "SAFplusClusterNodes";

  class ClusterNodeArray
  {
  public:

    class Entry
    {
    public:
      uint64_t since;  //? When status or generation last changed.  Not really needed, may be useful for statistics.
      uint64_t generation;  //? The generation field differentiates reboots of a node.
      SAFplus::NodeStatus  status;
      char     transport;  //? an ID describing the message transport used to contact this node. 0 = default transport.  This transport is used to decode the address
      char     address[SAFplus::MsgTransportAddressMaxLen];  //? The underlying (IP, TIPC) address of the node.  Format is defined by the transport
    };

    enum 
      {
        Version_1 = 0xa63b2891,
        UnknownGeneration = 0xffffffffffffffff,
      };
    uint32_t structIdAndVersion;
    //uint32_t numLiveNodes;
    uint32_t maxNodeId;
    Entry nodes[SAFplus::MaxNodes];  //? Array of known nodes.  The nodeId is the index into this array.

    void clean(void);
    int findEmpty(void); //? returns 0 if nothing is empty
  };


  int ClusterNodeArray::findEmpty(void)
  {
    for (int i=1;i<SAFplus::MaxNodes; i++)
      {
        if (nodes[i].status != NodeStatus::Alive) return i;
      }
    return 0;
  }


  void ClusterNodeArray::clean(void)
  {
    // Now initialize the shared memory since this is the server (TODO: in the future, use what already exists)
    structIdAndVersion =    ClusterNodeArray::Version_1;
    //numNodes = 0;
    maxNodeId = 0;
    int generation = 0;
    for (int i=0;i< SAFplus::MaxNodes; i++)
      {
        nodes[i].generation = generation;
        nodes[i].since = 0;
        nodes[i].status = NodeStatus::NonExistent;
        memset(nodes[i].address,0,MsgTransportAddressMaxLen);
      }
  
  }
  

  ClusterNodes::ClusterNodes(bool writable)
  {
    if (writable)
      {
        mem = boost::interprocess::shared_memory_object(boost::interprocess::open_or_create, ClusterNodesSharedMemoryName, boost::interprocess::read_write);
        // Set up access to the shared memory
        mem.truncate(sizeof(ClusterNodeArray));
        memRegion = mapped_region(mem,read_write,0,sizeof(ClusterNodeArray));
       }
    else
      {
        mem = boost::interprocess::shared_memory_object(boost::interprocess::open_only, ClusterNodesSharedMemoryName, boost::interprocess::read_only);
      memRegion = mapped_region(mem,read_only,0,sizeof(ClusterNodeArray));
      }

    data = (SAFplus::ClusterNodeArray*) memRegion.get_address();
    assert(data);

    if (writable)
      {
        if (data->structIdAndVersion != ClusterNodeArray::Version_1) data->clean();
      }

  }

  NodeStatus ClusterNodes::status(int nodeId) const
  {
    assert(data);
    assert(nodeId < SAFplus::MaxNodes);
    if (nodeId > data->maxNodeId) return NodeStatus::NonExistent;
    return data->nodes[nodeId].status;
  }

  const char* ClusterNodes::transportAddress(int nodeId) const
  {
    assert(data);
    assert(nodeId < SAFplus::MaxNodes);
    if (nodeId > data->maxNodeId) return NULL;
    return data->nodes[nodeId].address;
  }

  uint64_t ClusterNodes::generation(int nodeId) const
  {
    assert(data);
    assert(nodeId < SAFplus::MaxNodes);
    if (nodeId > data->maxNodeId) return 0;
    return data->nodes[nodeId].generation;
  }

  uint ClusterNodes::add(const std::string& address,uint64_t generation,uint preferredNodeId)
  {
    bool existing = false;
    if (preferredNodeId >= SAFplus::MaxNodes) preferredNodeId = 0; // Reject crazy preference
    if (preferredNodeId)  // let's see if we can use his preference.
      {      
        if (data->nodes[preferredNodeId].status == NodeStatus::Alive) // not a good sign, unless the node is being added twice.
          {
            if (address != data->nodes[preferredNodeId].address) preferredNodeId = 0;  // Nope, different nodes.
            else
              {
                existing = true;
              }
          }
      }
    if (!preferredNodeId) preferredNodeId = data->findEmpty();

    if (!preferredNodeId) return 0;  // no available slots

    // Ok, now fill in the slot with the new node's data

    //data->nodes[preferredNodeId].status = NodeStatus::Dead;  // Briefly mark as dead because I'm changing stuff.  Note: commenting out b/c only thing changing is generation.
    //strncpy(data->nodes[preferredNodeId].address,address.c_str(),MsgTransportAddressMaxLen);
    int len = MsgTransportAddressMaxLen;
    SAFplusI::defaultMsgPlugin->string2TransportAddress(address,(void*) data->nodes[preferredNodeId].address,&len);
    if (generation == ClusterNodeArray::UnknownGeneration)
      {
        if (data->nodes[preferredNodeId].generation == ClusterNodeArray::UnknownGeneration) data->nodes[preferredNodeId].generation = 1;
      else data->nodes[preferredNodeId].generation++;
      }
    else data->nodes[preferredNodeId].generation = generation;

    // Change the status last so readers do not use this entry until it is properly filled out.
    data->nodes[preferredNodeId].status = NodeStatus::Alive;

    // update the last node in the array so we don't have to look thru the entire thing during searches.
    data->maxNodeId = std::max(data->maxNodeId, preferredNodeId);
    //if (!existing) numLiveNodes++;
    return preferredNodeId;
  }

  void ClusterNodes::set(uint nodeId, const std::string& address,uint64_t generation,NodeStatus status)
  {
    assert(nodeId <= SAFplus::MaxNodes); // Reject crazy id

    //if ((data->nodes[nodeId].status != NodeStatus::Alive)&&(status == NodeStatus::Alive)) numLiveNodes++;

    //strncpy(data->nodes[nodeId].address,address.c_str(),MsgTransportAddressMaxLen);
    int len = MsgTransportAddressMaxLen;
    SAFplusI::defaultMsgPlugin->string2TransportAddress(address,data->nodes[nodeId].address,&len);
    if (generation == ClusterNodeArray::UnknownGeneration)
      {
        if (data->nodes[nodeId].generation == ClusterNodeArray::UnknownGeneration) data->nodes[nodeId].generation = 1;
      else data->nodes[nodeId].generation++;
      }
    else data->nodes[nodeId].generation = generation;

    // Change the status last so readers do not use this entry until it is properly filled out.
    data->nodes[nodeId].status = NodeStatus::Alive;

    // update the last node in the array so we don't have to look thru the entire thing during searches.
    data->maxNodeId = std::max(data->maxNodeId, nodeId);
  }

  void ClusterNodes::mark(int nodeId,NodeStatus status)
  {
    assert(nodeId <= SAFplus::MaxNodes); // Reject crazy id
    data->nodes[nodeId].status = status;

    // update the last node in the array so we don't have to look thru the entire thing during searches.  But only if we are marking this node as something
    if (status != NodeStatus::NonExistent)
      {
        data->maxNodeId = std::max(data->maxNodeId, (uint32_t) nodeId);
      }
  }


  std::string ClusterNodes::Iterator::transportAddressString() const
  {
    const void* tmp = clusterNodes->transportAddress(curIdx);
    if (!tmp) return std::string();
    return SAFplusI::defaultMsgPlugin->transportAddress2String(tmp);
  }

  ClusterNodes::Iterator& ClusterNodes::Iterator::operator++()
  {
  do
    {
    curIdx++;    
    } while((clusterNodes->status(curIdx) == SAFplus::NodeStatus::NonExistent)&&(curIdx <= clusterNodes->data->maxNodeId));

  if (curIdx > clusterNodes->data->maxNodeId) { clusterNodes = NULL;  curIdx = 0; }
  return *this;
  }

  ClusterNodes::Iterator& ClusterNodes::Iterator::operator++(int)
  {
  do
    {
    curIdx++;    
    } while((clusterNodes->status(curIdx) == SAFplus::NodeStatus::NonExistent)&&(curIdx <= clusterNodes->data->maxNodeId));

  if (curIdx > clusterNodes->data->maxNodeId) { clusterNodes = NULL; curIdx = 0; }
  return *this;
  }

  

  ClusterNodes::Iterator ClusterNodes::endSentinel(NULL,0);

  ClusterNodes* defaultClusterNodes = NULL;

  void ClusterNodes::deleteSharedMemory()
  {
  shared_memory_object::remove(ClusterNodesSharedMemoryName);
  }

  int ClusterNodes::idOf(const void* transportAddress,int addrSize) const // TODO: use a hash table to do the reverse lookup.
  {
    assert(addrSize <= SAFplus::MsgTransportAddressMaxLen);
    for(int curIdx=1;curIdx<=data->maxNodeId;curIdx++)
      {
        ClusterNodeArray::Entry* entry = &data->nodes[curIdx];
        if ((entry->status != SAFplus::NodeStatus::NonExistent)&&(entry->status != SAFplus::NodeStatus::Dead))
          {
            if (memcmp(transportAddress,entry->address,addrSize)==0) return curIdx;
          }
      }
    return 0;
  }

};
