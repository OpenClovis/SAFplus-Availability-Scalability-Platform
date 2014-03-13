#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clCustomization.hxx>

using namespace SAFplus;
using namespace SAFplusI;

Checkpoint NameRegistrar::m_checkpoint(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);

/*SAFplus::NameRegistrar::NameRegistrar()
{   
}*/
#if 0
SAFplus::NameRegistrar::NameRegistrar(const char* name, SAFplus::Handle handle, void* object/*=NULL*/)
{
   set(name, handle, object);
}
SAFplus::NameRegistrar::NameRegistrar(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/)
{
   set(name, handle, object);
}
SAFplus::NameRegistrar::NameRegistrar(const char* name, SAFplus::Buffer* buf)
{
   set(name, buf);
}
SAFplus::NameRegistrar::NameRegistrar(const std::string& name, SAFplus::Buffer* buf)
{
   set(name, buf);
}
#endif
void SAFplus::NameRegistrar::set(const char* name, SAFplus::Handle handle, void* object/*=NULL*/,size_t objlen)
{   
   size_t keyLen = sizeof(Handle);
   char data[sizeof(Buffer)-1+keyLen];
   Buffer* key = new(data) Buffer(keyLen);
   memcpy(key->data, &handle, keyLen);
   
   size_t valLen = strlen(name)+1;
   char vdata[sizeof(Buffer)-1+valLen];
   Buffer* val = new(vdata) Buffer(valLen);
   *val = name;

   Transaction t;
   m_checkpoint.write(*key,*val,t);
   if (!object)
   {
      //Don't care the object
      return;
   }
   // TODO Associate this handle to the object
   // ...
   len = strlen(name)+1; //include '\0' for data
   char* pbuf = new char[len+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (pbuf) SAFplus::Buffer(len);
   *k = name;

   char* pvbuf = new char[objlen+sizeof(SAFplus::Buffer)-1];
   SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer(objlen); 
   memcpy(v->data, object, objlen);
   SAFplusI::BufferPtr kb(k),kv(v);
   SAFplusI::CkptMapPair vt(kb,kv);
   m_mapObject.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/,size_t objlen)
{
   set(name.data(), handle, object,objlen);
}

void SAFplus::NameRegistrar::append(const char* name, SAFplus::Handle handle, void* object/*=NULL*/,size_t objlen)
{
   size_t len = sizeof(Handle);
   char data[sizeof(Buffer)-1+len];
   Buffer* key = new(data) Buffer(len);
   memcpy(key->data, &handle, len);
   const Buffer& buf = m_checkpoint.read(*key);
   if (&buf == NULL)
   {
      //There is no any name associated with this handle. Create first
      set(name, handle, object,objlen);
   }
   else
   {
      // TODO A name exists, add one more association. Does ckpt7 support this?
      // ...
   }
   
   if (!object)
   {
      //Don't care the object
      return;
   }
   // TODO Associate this handle to the object
   // ...   
}
void SAFplus::NameRegistrar::append(const std::string& name, SAFplus::Handle handle, void* object/*=NULL*/,size_t objlen)
{
   append(name.data(), handle, object, objlen);
}

void SAFplus::NameRegistrar::set(const char* name, const void* data, int length)
{
}
void SAFplus::NameRegistrar::set(const std::string& name, const void* data, int length)
{
}

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Buffer*)
{
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Buffer*)
{
}

std::pair<SAFplus::Handle&,void*> SAFplus::NameRegistrar::get(const char* name) throw(NameException&)
{
   try
   {
      Handle& handle = getHandle(name);
      //Next: TODO get the void* object associated with the name then make the pair to return
      void* object = NULL;
      return (std::pair<SAFplus::Handle&,void*>(handle, object));
   }                
   catch (NameException ne)
   {
      throw ne;
   }
}
std::pair<SAFplus::Handle&,void*> SAFplus::NameRegistrar::get(const std::string& name) throw(NameException&)
{
   try
   {
      return get(name.data());
   }
   catch(NameException ne)
   {
      throw ne;
   }
}

SAFplus::Handle& SAFplus::NameRegistrar::getHandle(const char* name) throw(NameException&)
{
   //Loop thru the Iterator of checkpoint to find the name
   CkptHashMap::iterator ibegin = m_checkpoint.begin().iter;
   CkptHashMap::iterator iend = m_checkpoint.end().iter;
   //std::pair<SAFplus::Handle&,void*> ret;
   //char errmsg[255];
   for(CkptHashMap::iterator iter = ibegin; iter != iend; iter++)
   {
       //SAFplusI::BufferPtr& curval = iter.second;       
       CkptHashMap::value_type vt = *iter;
       BufferPtr& curval = vt.second;
       if (curval)
       {
          if (strcmp(name, curval->data) == 0) // the name exists
          {
             BufferPtr kname = vt.first;
             assert(kname);
             uint64_t len = kname->len();
             char temp[len];
             memcpy(temp, kname->data, len);
             Handle* handle = new(kname->data) Handle();
             memcpy(kname->data, temp, len);
             return *handle;
          }
       }
    }
#if 0
             else
             {
                strcpy(errmsg, "Format of string handle is not valid");
             }
          }
          else
          {
             strcpy(errmsg, "Name provided doesn't exist");
          }
       }        
       else
       {
          strcpy(errmsg, "The value for the key provided is null");
       }
   }
#endif
   throw NameException("name provided does not exist");
}
SAFplus::Handle& SAFplus::NameRegistrar::getHandle(const std::string& name) throw(NameException&)
{
   try
   {
      return getHandle(name.data());
   }                
   catch (NameException ne)
   {
      throw ne;
   }   
}


SAFplus::Buffer& SAFplus::NameRegistrar::getData(const char* name) throw(NameException&)
{
}
SAFplus::Buffer& SAFplus::NameRegistrar::getData(const std::string& name) throw(NameException&)
{
}
SAFplus::NameRegistrar::~NameRegistrar()
{
}
