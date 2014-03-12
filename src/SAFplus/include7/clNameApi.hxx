#ifndef clNameApi_hxx
#define clNameApi_hxx
// Standard includes
#include <string>
#include <exception>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clCkptApi.hxx>


namespace SAFplus
{

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

  public:
     NameRegistrar();
     NameRegistrar(const char* name, SAFplus::Handle handle, void* object=NULL);
     NameRegistrar(const std::string& name, SAFplus::Handle handle, void* object=NULL);
     NameRegistrar(const char* name, SAFplus::Buffer*);
     NameRegistrar(const std::string& name, SAFplus::Buffer*);     
     //NameRegistrar(std::string name, SAFplus::Handle handle);
     /*void nameInitialize();
     void nameSet(string name, HandleT handle);
     void nameSet(string name, void* data, int dataLen);
     HandleT nameGet(string name);
     int nameGet(string name, void* data, int maxDataLen);
     */
     /* Associate a name with a handle and pointer and associate a handle with a pointer.
      If the name does not exist, it is created.  If it exists, it is overwritten.
      If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
      the void* object pointer is local to this process; it does not need to be part of the checkpoint.
      This association is valid for all SAFplus API name lookups, and for AMF entity names.
      */
     void set(const char* name, SAFplus::Handle handle, void* object=NULL);
     void set(const std::string& name, SAFplus::Handle handle, void* object=NULL);
   
     /* Associate a name with a handle and pointer and associate a handle with a pointer (if object != NULL).
        If the name does not exist, it is created.  If the name exists, this mapping is appended (the original mapping is not removed).
        If there are multiple associations, the first association will always be returned.
        If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
        If the name has more than one mapping another mapping will become the default response for this name. 
        This association is valid for all SAFplus API name lookups, and for AMF entity names.
     */   
     void append(const char* name, SAFplus::Handle handle, void* object=NULL);
     void append(const std::string& name, SAFplus::Handle handle, void* object=NULL);
     
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
     virtual ~NameRegistrar();
  };
  
}

#endif
