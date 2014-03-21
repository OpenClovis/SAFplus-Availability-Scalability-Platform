#ifndef clNameApi_hxx
#define clNameApi_hxx
// Standard includes
#include <string>
#include <exception>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clCkptApi.hxx>
#include <clCkptIpi.hxx>

namespace SAFplus
{

  typedef boost::unordered_map <SAFplusI::CkptMapKey, SAFplusI::CkptMapValue> HashMap;
  typedef std::vector<SAFplus::Handle> Vector;
  

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
     HashMap m_mapData; // keep association between name and arbitrary data
     HashMap m_mapObject; // keep association between handle and an object
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
      //static NameRegistrar* getInstance() { name = new NameRegistrar(); return name;}
     /* Associate a name with a handle and pointer and associate a handle with a pointer.
      If the name does not exist, it is created.  If it exists, it is overwritten.
      If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
      the void* object pointer is local to this process; it does not need to be part of the checkpoint.
      This association is valid for all SAFplus API name lookups, and for AMF entity names.
      */
     //void setMode(const char* name, MappingMode mode);
     //void setMode(const std::string& name, MappingMode mode);
     void set(const char* name, SAFplus::Handle handle, MappingMode m, void* object=NULL, size_t objlen=0);
     void set(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object=NULL, size_t objlen=0);
   
     /* Associate a name with a handle and pointer and associate a handle with a pointer (if object != NULL).
        If the name does not exist, it is created.  If the name exists, this mapping is appended (the original mapping is not removed).
        If there are multiple associations, the first association will always be returned.
        If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
        If the name has more than one mapping another mapping will become the default response for this name. 
        This association is valid for all SAFplus API name lookups, and for AMF entity names.
     */   
     void append(const char* name, SAFplus::Handle handle, MappingMode m, void* object=NULL,size_t objlen=0);
     void append(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object=NULL,size_t objlen=0);
     
     // Associate name with arbitrary data. A copy of the data is made.
     void set(const char* name, const void* data, int length);
     void set(const std::string& name, const void* data, int length);
   
     // Associate name with arbitrary data. A copy of the data is NOT made; this call transfers the reference count (ownership) to the callee.
     void set(const char* name, SAFplus::Buffer*);
     void set(const std::string& name, SAFplus::Buffer*);
  
     // Get a handle associated with the data
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     std::pair<SAFplus::Handle&,void*> get(const char* name) throw(NameException&);
     std::pair<SAFplus::Handle&,void*> get(const std::string& name) throw(NameException&);
   
     SAFplus::Handle& getHandle(const char* name) throw(NameException&);
     SAFplus::Handle& getHandle(const std::string& name) throw(NameException&);
   
       
     // Get a handle associated with the data
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     // Do not free the returned buffer, call Buffer.decRef();
     SAFplus::Buffer& getData(const char* name) throw(NameException&);
     SAFplus::Buffer& getData(const std::string& name) throw(NameException&);

     //**** This is the dummy function for testing to get iocNodeAddr ********
     ClIocNodeAddressT clIocLocalAddressGet() { return 2; }
     void dump();
     
     virtual ~NameRegistrar();
  };

  class HandleMappingMode
  {
  protected:
     NameRegistrar::MappingMode m_mode;
     Vector m_handles;
  public:
     HandleMappingMode(NameRegistrar::MappingMode mode, Vector handles): m_mode(mode), m_handles(handles)
     {        
     }
     NameRegistrar::MappingMode getMappingMode()
     {
        return m_mode;
     }
     Vector getHandles()
     {
        return m_handles;
     }
  };
}
#endif
