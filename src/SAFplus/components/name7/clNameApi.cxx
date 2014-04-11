#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clCustomization.hxx>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

using namespace SAFplus;
using namespace SAFplusI;

Checkpoint NameRegistrar::m_checkpoint(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);

NameRegistrar name;

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/)
{   
   size_t keyLen = strlen(name)+1;
   char data[sizeof(Buffer)-1+keyLen];
   Buffer* key = new(data) Buffer(keyLen);
   *key = name;
   
   Vector vector;
   vector.push_back(handle);   
   HandleMappingMode hm(m, vector);

   std::string serial_str;
   boost::iostreams::back_insert_device<std::string> inserter(serial_str);
   boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
   boost::archive::binary_oarchive oa(s);
   oa << hm;   
   s.flush();
   size_t valLen = serial_str.size();
   char vdata[sizeof(Buffer)-1+valLen];
   Buffer* val = new(vdata) Buffer(valLen);   
   memcpy(val->data, serial_str.data(), valLen);

   Transaction t;
   m_checkpoint.write(*key,*val,t);
   if (!object)
   {
      //Don't care the object
      return;
   }
   //Find to see if the key exists?
   ObjHashMap::iterator contents = m_mapObject.find(handle);  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   { 
      //void* oldObj = contents->second;
      contents->second = object;
      //delete oldObj; Do not delete the object; it must be deleted by owner process
      return;
   }
   SAFplus::ObjMapPair vt(handle,object);
   m_mapObject.insert(vt);  
}

void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/)
{
   set(name.c_str(), handle, m, object);
}

void SAFplus::NameRegistrar::append(const char* name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/)
{
   size_t len = strlen(name)+1;
   char data[sizeof(Buffer)-1+len];
   Buffer* key = new(data) Buffer(len);
   *key = name;
   const Buffer& buf = m_checkpoint.read(*key);
   if (&buf == NULL)
   {
      //There is no any name associated with this handle. Create first
      set(name, handle, m, object);
   }
   else
   {
      // TODO A name exists, add one more association.
      size_t sz = buf.len();      
      boost::iostreams::basic_array_source<char> device(buf.data, sz);
      boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
      boost::archive::binary_iarchive ia(s);
      HandleMappingMode hm;  
      ia >> hm;
      Vector newHandles = hm.getHandles();        
      newHandles.push_back(handle);
      MappingMode mm = hm.getMappingMode();
      if (m != MODE_NO_CHANGE) mm = m;
      //re-write the new buffer
      HandleMappingMode hm2(mm, newHandles);
      std::string serial_str;
      boost::iostreams::back_insert_device<std::string> inserter(serial_str);
      boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s2(inserter);
      boost::archive::binary_oarchive oa(s2);
      oa << hm2;
      s2.flush();
      size_t valLen = serial_str.size();
      char vdata[sizeof(Buffer)-1+valLen];
      Buffer* val = new(vdata) Buffer(valLen);   
      memcpy(val->data, serial_str.data(), valLen);
      Transaction t;
      m_checkpoint.write(*key,*val,t);
   }
   
   if (!object)
   {
      //Don't care the object
      return;
   }
   // TODO Associate this handle to the object
   //Find to see if the key exists?
   ObjHashMap::iterator contents = m_mapObject.find(handle);  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      //void* oldObj = contents->second;
      contents->second = object;
      //delete oldObj; Do not delete the object; it must be deleted by owner process
      return;
   }   
   SAFplus::ObjMapPair vt(handle,object);
   m_mapObject.insert(vt);
}

void SAFplus::NameRegistrar::append(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/)
{
   append(name.c_str(), handle, m, object);
}

void SAFplus::NameRegistrar::set(const char* name, const void* data, int length) throw (NameException&)
{
   Handle handle;
   try {
      handle = getHandle(name);
   }catch (NameException &ne) {
      ne.addMsg(". The name may be not registered with any handle");
      throw ne;
   }   
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(handle);  
   if (contents != m_mapData.end()) // record already exists; overwrite
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == length)
            {
               memcpy (curval->data, data, length);
               return;
            }
            // Replace the Buffer with a new one
            char* pvbuf = new char[length+sizeof(SAFplus::Buffer)-1];
            SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer (length);
            memcpy (v->data, data, length);
            SAFplus::Buffer* old = curval.get();
            curval = v;
            if (old->ref() == 1)
            {
               delete old;
            }
            else
            {
               old->decRef();	
            }
            return;
         }
      }
   }
   char* pvbuf = new char[length+sizeof(SAFplus::Buffer)-1];
   SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer(length); 
   memcpy(v->data, data, length);
   SAFplusI::BufferPtr kv(v);
   MapPair vt(handle,kv);
   m_mapData.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, const void* data, int length) throw (NameException&)
{
   try {
      set(name.c_str(), data, length);
   }catch (NameException &ne) {
      throw ne;
   }
}

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Buffer* p_buf) throw (NameException&)
{
   Handle handle;
   try {
      handle = getHandle(name);
   }catch (NameException &ne) {
      ne.addMsg(". The name may be not registered with any handle");
      throw ne;
   } 
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(handle);  
   if (contents != m_mapData.end()) // record already exists; overwrite
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == p_buf->len())
            {
               memcpy (curval->data, p_buf->data, p_buf->len());
               return;
            }
            // Replace the Buffer with a new one
            SAFplus::Buffer* old = curval.get();
            curval = p_buf;
            if (old->ref() == 1)
            {
               delete old;
            }
            else
            {
               old->decRef();	
            }
            return;
         }
      }
   }
   SAFplusI::BufferPtr kv(p_buf);
   MapPair vt(handle,kv);
   //p_buf->addRef(); // add 1 reference to the callee
   m_mapData.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Buffer* p_buf) throw (NameException&)
{
   try {
      set(name.c_str(), p_buf);
   }catch (NameException &ne) {
      throw ne;
   }
}

ObjMapPair SAFplus::NameRegistrar::get(const char* name) throw(NameException&)
{
   try
   {
      Handle handle = getHandle(name);
      //Find to see if the key exists?
      ObjHashMap::iterator contents = m_mapObject.find(handle);  
      if (contents != m_mapObject.end()) // record already exists; return its value
      {
         void* curObj = contents->second;
         if (curObj)
         {                   
            return ObjMapPair(handle, curObj);                     
         }
      }
      return ObjMapPair(handle, NULL);
   }                
   catch (NameException &ne)
   {
      throw ne;
   }
}
ObjMapPair SAFplus::NameRegistrar::get(const std::string& name) throw(NameException&)
{
   try
   {
      return get(name.c_str());
   }
   catch(NameException &ne)
   {
      throw ne;
   }
}

void* SAFplus::NameRegistrar::get(const SAFplus::Handle& handle) throw (NameException&)
{
   ObjHashMap::iterator contents = m_mapObject.find(handle);  
   if (contents != m_mapObject.end()) // record already exists; return its value
   {   
      return contents->second;      
   }
   throw NameException("Handle provided does not exist");
}

SAFplus::Handle SAFplus::NameRegistrar::getHandle(const char* name) throw(NameException&)
{
   size_t len = strlen(name)+1;
   char data[sizeof(Buffer)-1+len];
   Buffer* key = new(data) Buffer(len);
   *key = name;
   const Buffer& buf = m_checkpoint.read(*key);
   if (&buf != NULL)
   {      
      boost::iostreams::basic_array_source<char> device(buf.data, buf.len());
      boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
      boost::archive::binary_iarchive ia(s);
      HandleMappingMode hm;  
      ia >> hm;      
      MappingMode m = hm.getMappingMode();      
      size_t sz = hm.getHandles().size();
      int idx = -1;
      if (m == MODE_REDUNDANCY)
      {
         // first association must be returned
         assert(sz > 0);    
         idx = 0;
      }
      else if (m == MODE_ROUND_ROBIN)
      {
         srand (time(NULL));
         idx = rand() % sz;         
      }
      else if (m == MODE_PREFER_LOCAL)
      {
         pid_t thisPid = getpid();
         int i;
         for(i=0;i<sz;i++)
         {
            if (hm.getHandles().at(i).getProcess() == (uint32_t)thisPid)
            {
               idx = i;
            }
         }
         //No process match, get handle of THIS NODE
         if (idx == -1)
         {
            for(i=0;i<sz;i++)
	    {
		ClIocNodeAddressT thisNode = clIocLocalAddressGet();
		if ((uint32_t)hm.getHandles().at(i).getNode() == thisNode)
		{		
                   idx = i;
		}
            }
         }
         // If no any match, get "closer" handle over others. It may be the latest one
         if (idx == -1) idx = sz-1;
      }
      else // Other case, REDUNDANCY mode is picked
      {
         assert(sz > 0);
         idx = 0;
      }
      assert(idx >=0 && idx < sz);
      return hm.getHandles().at(idx);
   }
   throw NameException("name provided does not exist");
}
SAFplus::Handle SAFplus::NameRegistrar::getHandle(const std::string& name) throw(NameException&)
{
   try
   {
      return getHandle(name.c_str());
   }                
   catch (NameException &ne)
   {
      throw ne;
   }   
}


SAFplus::Buffer& SAFplus::NameRegistrar::getData(const char* name) throw(NameException&)
{         
   Handle handle;
   try {
      handle = getHandle(name);
   }catch (NameException &ne) {
      throw ne;
   } 
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(handle);  
   if (contents != m_mapData.end()) // record already exists; return its value
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() >= 1)
         {           
            return *curval.get();        
         }
       }
    }
    throw NameException("Name provided does not exist");   
}
SAFplus::Buffer& SAFplus::NameRegistrar::getData(const std::string& name) throw(NameException&)
{
   try 
   {
      return getData(name.c_str());
   }
   catch (NameException &ne)
   {
      throw ne;
   }
}

void SAFplus::NameRegistrar::dump()
{
   CkptHashMap::iterator ibegin = m_checkpoint.begin().iter;
   CkptHashMap::iterator iend = m_checkpoint.end().iter;  
   for(CkptHashMap::iterator iter = ibegin; iter != iend; iter++)
   {
       CkptHashMap::value_type vt = *iter;
       BufferPtr curkey = vt.first;
       printf("---------------------------------\n");      
       if (curkey)
       {
          printf("key [%s]\n", curkey->data);
       }       
       BufferPtr& curval = vt.second;
       if (curval)
       {
          boost::iostreams::basic_array_source<char> device(curval->data, curval->len());
          boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s(device);
          boost::archive::binary_iarchive ia(s);
          HandleMappingMode hm;  
          ia >> hm;
          Vector v = hm.getHandles();
          size_t sz = v.size();
          for(int i=0;i<sz;i++)
          {
             printf("val [0x%x.0x%x]\n", v[i].id[0],v[i].id[1]);
          }
       }
    }
}

void SAFplus::NameRegistrar::dumpObj()
{
   for(ObjHashMap::iterator iter = m_mapObject.begin(); iter != m_mapObject.end(); iter++)
   {
       ObjHashMap::value_type vt = *iter;
       Handle curkey = vt.first;
       printf("---------------------------------\n");      
       printf("key [0x%x.0x%x]\n", curkey.id[0], curkey.id[1]);              
       void* obj = vt.second;
       if (obj)
       {
          printf("Obj associated Not NULL\n");
       }
       else       
       {
          printf("Obj associated IS NULL\n");
       }
    }
}

SAFplus::NameRegistrar::~NameRegistrar()
{
   // Free 2 maps
#if 0
   for(ObjHashMap::iterator iter = m_mapObject.begin(); iter != m_mapObject.end(); iter++)
   {       
       const SAFplusI::BufferPtr& k = iter->first;       
       //SAFplusI::BufferPtr& v = iter->second;
       if (k) 
       {
          if  (k->ref() == 1) delete k.get();
          else k->decRef();
       }
       /*if (v)
       {
          if  (v->ref() == 1) delete v.get();
          else v->decRef();
       }*/       
   }

   m_mapObject.clear();
#endif
   for(HashMap::iterator iter = m_mapData.begin(); iter != m_mapData.end(); iter++)
   {
       //CkptHashMap::value_type t = *iter;
       //const SAFplusI::BufferPtr& k = iter->first;       
       SAFplusI::BufferPtr& v = iter->second;
       /*if (k)
       {
          if  (k->ref() == 1) delete k.get();
          else k->decRef();
       }*/       
       if (v)
       {
          if  (v->ref() == 1) delete v.get();
          else v->decRef();
       }       
   }
   m_mapData.clear();
}
