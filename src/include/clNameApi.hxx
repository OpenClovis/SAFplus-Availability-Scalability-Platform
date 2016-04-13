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

//? <section name="Name Service">


namespace SAFplus
{
  class Buffer;

  typedef std::pair<const SAFplus::Handle,void*> ObjMapPair; 
  typedef std::pair<const SAFplus::Handle&,void*> RefObjMapPair;
  typedef boost::unordered_map <SAFplus::Handle, void*> ObjHashMap;

  // TODO derive from SAFplus::Error
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


  class HandleData;

  //? <class> The NameRegistrar is a cluster-wide directory that maps names to arbitrary data.  It is typically used to associate a string name with a Handle, so that objects and services can be referred to via a string name.  It is used to discover the handles of SAFplus entities like logging streams, and can be used by the application programmer to discover the implementer of a service.  There is a single instance of this class per application, available in the global variable SAFplus::name.
  class NameRegistrar
  {
  protected:
     SAFplus::Checkpoint m_checkpoint;
     //HashMap m_mapData; // keep association between handle and its arbitrary data
     ObjHashMap m_mapObject; // keep association between handle and an object
  private:
     //static NameRegistrar* name;
        
     //NameRegistrar(){}
     
  public:

     //? <class> Iterate through all the entries in the name table.
     class Iterator
       {
       NameRegistrar* nam;
       SAFplus::Checkpoint::Iterator it;
       public:
       Iterator(NameRegistrar& n):it(&n.m_checkpoint) { nam = &n; }
       friend class NameRegistrar;

       //? comparison
       bool operator != (const Iterator& i) const { return (it != i.it); }

       //? go to the next item in the name table
       Iterator& operator++ (int) { it++; return *this; }
       
       //? Get the name of the currently visited table entry
       std::string name();
       //? Get the handle of the currently visited table entry
       HandleData& handle();
       }; //? </class>
       
     friend class Iterator;

     //? Similar to the Standard Template Library (STL), this function starts iteration through the name table.
     Iterator begin() { Iterator i(*this); i.it = m_checkpoint.begin(); return i; }
     //? Similar to the Standard Template Library (STL), this returns the termination sentinel -- that is, an invalid object that can be compared to an iterator to determine that it is at the end of the list of members.
     Iterator end() { Iterator i(*this); i.it = m_checkpoint.end(); return i; }

     typedef enum 
     {
        MODE_REDUNDANCY, //? If this name has multiple associations, return the valid one.
        MODE_ROUND_ROBIN, //? If this name has multiple associations, return them in succession
        MODE_PREFER_LOCAL, //? If this name has multiple associations, return the local handle if one exists.  Otherwise return the first handle.
        MODE_NO_CHANGE  //? Do not change the already-set mode
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

     /*? Associate a name with a handle and pointer and associate a handle with a pointer.
      If the name does not exist, it is created.  If it exists, it is overwritten.
      If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
      the void* object pointer is local to this process; it does not need to be part of the checkpoint.
      This association is valid for all SAFplus API name lookups, and for AMF entity names.
      */
     void set(const char* name, SAFplus::Handle handle, MappingMode m);

     /*? Associate a name with a handle and pointer and associate a handle with a pointer.
      If the name does not exist, it is created.  If it exists, it is overwritten.
      If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
      the void* object pointer is local to this process; it does not need to be part of the checkpoint.
      This association is valid for all SAFplus API name lookups, and for AMF entity names.
      */
     void set(const std::string& name, SAFplus::Handle handle, MappingMode m);

     //void setMode(const char* name, MappingMode mode);
     //void setMode(const std::string& name, MappingMode mode);
   
     /* Associate a name with a handle and pointer and associate a handle with a pointer (if object != NULL).
        If the name does not exist, it is created.  If the name exists, this mapping is appended (the original mapping is not removed).
        If there are multiple associations, the first association will always be returned.
        If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
        If the name has more than one mapping another mapping will become the default response for this name. 
        This association is valid for all SAFplus API name lookups, and for AMF entity names.
     */   
     void append(const char* name, SAFplus::Handle handle, MappingMode m=MODE_NO_CHANGE) throw (NameException&);

     /* Associate a name with a handle and pointer and associate a handle with a pointer (if object != NULL).
        If the name does not exist, it is created.  If the name exists, this mapping is appended (the original mapping is not removed).
        If there are multiple associations, the first association will always be returned.
        If the handle format contains a node or process designator, then this mapping will be removed when the node/process fails.
        If the name has more than one mapping another mapping will become the default response for this name. 
        This association is valid for all SAFplus API name lookups, and for AMF entity names.
     */   
     void append(const std::string& name, SAFplus::Handle handle, MappingMode m=MODE_NO_CHANGE) throw (NameException&);

     void setLocalObject(const char* name, void* object);
     void setLocalObject(const std::string& name, void* object);
     void setLocalObject(SAFplus::Handle handle, void* object);
     
     //? Associate name with arbitrary data. A copy of the data is made.
     void set(const char* name, const void* data, int length) throw (NameException&);
     //? Associate name with arbitrary data. A copy of the data is made.
     void set(const std::string& name, const void* data, int length) throw (NameException&); 
   
     //? Associate name with arbitrary data. A copy of the data is NOT made; this call transfers the reference count (ownership) to the callee.
     void set(const char* name, SAFplus::Buffer*) throw (NameException&);
     //? Associate name with arbitrary data. A copy of the data is NOT made; this call transfers the reference count (ownership) to the callee.
     void set(const std::string& name, SAFplus::Buffer*) throw (NameException&);

     //? Remove a name
     void remove(const char* name) throw (NameException&);
     //? Remove a name
     void remove(const std::string& name) throw (NameException&);
  
     //? Get a handle associated with the data. The SAFplus APIs use these calls to resolve names to handles or objects.
     RefObjMapPair get(const char* name) throw(NameException&);
     //? Get a handle associated with the data. The SAFplus APIs use these calls to resolve names to handles or objects.
     RefObjMapPair get(const std::string& name) throw(NameException&);
     //? Get a handle associated with the data. The SAFplus APIs use these calls to resolve names to handles or objects.
     void* get(const SAFplus::Handle&) throw(NameException&);
     
     //? This function gets the name associated with the specified handle
     char* getName(const SAFplus::Handle& handle) throw(NameException&); 
   
     //? Gets the handle associated with a name.  Throws exception if the name does not exist or the handle is INVALID_HDL
     SAFplus::Handle& getHandle(const char* name) throw(NameException&);
     //? Gets the handle associated with a name.  Throws exception if the name does not exist or the handle is INVALID_HDL
     SAFplus::Handle& getHandle(const std::string& name) throw(NameException&);
   
       
     //? Get a handle associated with the data.
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     // Do not free the returned buffer, call Buffer.decRef();
     const SAFplus::Buffer& getData(const char* name) throw(NameException&);

     //? Get a handle associated with the data.
     // The SAFplus APIs use these calls to resolve names to handles or objects.
     // Do not free the returned buffer, call Buffer.decRef();
     const SAFplus::Buffer& getData(const std::string& name) throw(NameException&);

     //? Failure handling. This function tells this object that a particular process has failed.  Typically, it is used only internally by SAFplus.
     void processFailed(const uint32_t pid, const uint32_t amfId);
     //? Failure handling. This function tells this object that a particular process has failed.  Typically, it is used only internally by SAFplus.
     void nodeFailed(const uint16_t slotNum, const uint32_t amfId);
     //? Failure handling. This function tells this object that a particular process has failed.  Typically, it is used only internally by SAFplus.
     void handleFailure(const FailureType failureType, const uint32_t id, const uint32_t amfId);

     //**** This is the dummy function for testing to get iocNodeAddr ********
     //ClIocNodeAddressT clIocLocalAddressGet() { return 2; }

     void dump();
     void dumpObj();
     
     virtual ~NameRegistrar();
     void init(SAFplus::Handle hdl);
  }; //? </class>

  class HandleData
    {
    public:
     short structIdAndEndian;
     short numHandles;
     NameRegistrar::MappingMode mappingMode;     
     short extra;
     SAFplus::Handle handles[1];     
    };


  //? Singleton class that handles all name access
  extern NameRegistrar name;  

  //? </section>
}

#endif
