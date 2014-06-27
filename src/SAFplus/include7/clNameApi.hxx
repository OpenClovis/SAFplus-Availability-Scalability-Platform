/*? Singleton class that handles all name access
    extern NameRegistrar name;
    inside SAFplus namespace...
*/
#ifndef clNameApi_hxx
#define clNameApi_hxx
// Standard includes
#include <string>
#include <exception>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clCkptApi.hxx>
#include <clCkptIpi.hxx>

//#include <boost/serialization/base_object.hpp>

namespace SAFplus
{
  class Buffer;

  typedef uint64_t U64;
  #define ENDIAN_SWAP_U64(val) ((U64) ( \
    (((U64) (val) & (U64) 0x00000000000000ff) << 56) | \
    (((U64) (val) & (U64) 0x000000000000ff00) << 40) | \
    (((U64) (val) & (U64) 0x0000000000ff0000) << 24) | \
    (((U64) (val) & (U64) 0x00000000ff000000) <<  8) | \
    (((U64) (val) & (U64) 0x000000ff00000000) >>  8) | \
    (((U64) (val) & (U64) 0x0000ff0000000000) >> 24) | \
    (((U64) (val) & (U64) 0x00ff000000000000) >> 40) | \
    (((U64) (val) & (U64) 0xff00000000000000) >> 56)))

  typedef std::pair<const SAFplus::Handle,void*> ObjMapPair; 
  typedef std::pair<const SAFplus::Handle&,void*> RefObjMapPair;
  typedef boost::unordered_map <SAFplus::Handle, void*> ObjHashMap;

  class NameException: public std::exception
  {
  protected:	
     std::string m_errMessage;
  public:
     //NameException(){}
     NameException(const char* errMessage)
     {
	m_errMessage = errMessage;
     }
     void addMsg(const char* msg)
     {
        m_errMessage += msg;
     }
     virtual const char* what()
     {
	return m_errMessage.c_str();
     }
     virtual ~NameException() throw()
     {
     }
  };

  class NameRegistrar
  {
  protected:
     static SAFplus::Checkpoint m_checkpoint;
     //HashMap m_mapData; // keep association between handle and its arbitrary data
     ObjHashMap m_mapObject; // keep association between handle and an object
  private:
     //static NameRegistrar* name;
        
     //NameRegistrar(){}
     
  public:
     typedef enum 
     {
        MODE_REDUNDANCY,
        MODE_ROUND_ROBIN,
        MODE_PREFER_LOCAL,
        MODE_NO_CHANGE
     } MappingMode;
     
     typedef enum 
     {
        FAILURE_PROCESS,
        FAILURE_NODE
     } FailureType;

     enum 
     {
        STRID = 0x1234,
        STRIDEN = 0x4321
     };
      //static NameRegistrar* getInstance() { name = new NameRegistrar(); return name;}
     /* Associate a name with a handle and pointer and associate a handle with a pointer.
      If the name does not exist, it is created.  If it exists, it is overwritten.
      If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
      the void* object pointer is local to this process; it does not need to be part of the checkpoint.
      This association is valid for all SAFplus API name lookups, and for AMF entity names.
      */
     //void setMode(const char* name, MappingMode mode);
     //void setMode(const std::string& name, MappingMode mode);
     void set(const char* name, SAFplus::Handle handle, MappingMode m);
     void set(const std::string& name, SAFplus::Handle handle, MappingMode m);
   
     /* Associate a name with a handle and pointer and associate a handle with a pointer (if object != NULL).
        If the name does not exist, it is created.  If the name exists, this mapping is appended (the original mapping is not removed).
        If there are multiple associations, the first association will always be returned.
        If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
        If the name has more than one mapping another mapping will become the default response for this name. 
        This association is valid for all SAFplus API name lookups, and for AMF entity names.
     */   
     void append(const char* name, SAFplus::Handle handle, MappingMode m=MODE_NO_CHANGE) throw (NameException&);
     void append(const std::string& name, SAFplus::Handle handle, MappingMode m=MODE_NO_CHANGE) throw (NameException&);

     void setLocalObject(const char* name, void* object);
     void setLocalObject(const std::string& name, void* object);
     void setLocalObject(SAFplus::Handle handle, void* object);
     
     // Associate name with arbitrary data. A copy of the data is made.
     void set(const char* name, const void* data, int length) throw (NameException&);
     void set(const std::string& name, const void* data, int length) throw (NameException&); 
   
     // Associate name with arbitrary data. A copy of the data is NOT made; this call transfers the reference count (ownership) to the callee.
     void set(const char* name, SAFplus::Buffer*) throw (NameException&);
     void set(const std::string& name, SAFplus::Buffer*) throw (NameException&);
  
     // Get a handle associated with the data
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     RefObjMapPair get(const char* name) throw(NameException&);
     RefObjMapPair get(const std::string& name) throw(NameException&);
     void* get(const SAFplus::Handle&) throw(NameException&);
   
     SAFplus::Handle& getHandle(const char* name) throw(NameException&);
     SAFplus::Handle& getHandle(const std::string& name) throw(NameException&);
   
       
     // Get a handle associated with the data
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     // Do not free the returned buffer, call Buffer.decRef();
     const SAFplus::Buffer& getData(const char* name) throw(NameException&);
     const SAFplus::Buffer& getData(const std::string& name) throw(NameException&);

     //Failure handling
     void processFailed(const uint32_t pid, const uint32_t amfId);
     void nodeFailed(const uint16_t slotNum, const uint32_t amfId);
     void handleFailure(const FailureType failureType, const uint32_t id, const uint32_t amfId);
     //**** This is the dummy function for testing to get iocNodeAddr ********
     //ClIocNodeAddressT clIocLocalAddressGet() { return 2; }
     void dump();
     void dumpObj();
     //***********************************************************************
     
     virtual ~NameRegistrar();
  };

  typedef struct
  {
  public:
     short structIdAndEndian;
     short numHandles;
     NameRegistrar::MappingMode mappingMode;     
     short extra;
     SAFplus::Handle handles[1];     
  }HandleData;  

  //? Singleton class that handles all name access
  extern NameRegistrar name;  
}

#endif
