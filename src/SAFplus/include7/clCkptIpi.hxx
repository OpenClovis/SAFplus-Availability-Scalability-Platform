#ifndef clCkptIpi_hxx
#define clCkptIpi_hxx
// Standard includes

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

namespace SAFplus
{
  class Buffer;
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
 
  
    enum
    {
        CL_CKPT_BUFFER_HEADER_STRUCT_ID_7 = 0x59400070,
    };

    class CkptBufferHeader
    {
    public:
        uint64_t structId;
        pid_t    serverPid;  // This is used to ensure that 2 servers don't fight for the logs...
        SAFplus::Handle handle;
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
