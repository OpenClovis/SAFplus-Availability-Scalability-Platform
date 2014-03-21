#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clCustomization.hxx>

using namespace SAFplus;
using namespace SAFplusI;

//NameRegistrar name; 

Checkpoint NameRegistrar::m_checkpoint(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);

NameRegistrar name;

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/,size_t objlen)
{   
   size_t keyLen = strlen(name)+1;
   char data[sizeof(Buffer)-1+keyLen];
   Buffer* key = new(data) Buffer(keyLen);
   *key = name;
   
   Vector vector;
   vector.push_back(handle);   
   HandleMappingMode hm(m, vector);
   size_t valLen = sizeof(hm);
   char vdata[sizeof(Buffer)-1+valLen];
   Buffer* val = new(vdata) Buffer(valLen);   
   memcpy(val->data, &hm, valLen);
   Transaction t;
   m_checkpoint.write(*key,*val,t);
   if (!object)
   {
      //Don't care the object
      return;
   }
   // Associate this handle to the object
   keyLen = sizeof(handle);
   char* pbuf = new char[keyLen+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (pbuf) SAFplus::Buffer(keyLen);
   memcpy(k->data, &handle, keyLen);
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapObject.find(SAFplusI::BufferPtr(k));  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      delete k; // Release the buffer allocated for the key
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == objlen)
            {
               memcpy (curval->data, object, objlen);               
               return;
            }
            // Replace the Buffer with a new one
            char* pvbuf = new char[objlen+sizeof(SAFplus::Buffer)-1];
            //char pvbuf[objlen+sizeof(SAFplus::Buffer)-1];
            SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer (objlen);
            memcpy (v->data, object, objlen);
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
   char* pvbuf = new char[objlen+sizeof(SAFplus::Buffer)-1];
   //char pvbuf[objlen+sizeof(SAFplus::Buffer)-1];
   SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer(objlen); 
   memcpy(v->data, object, objlen);
   SAFplusI::BufferPtr kb(k),kv(v);
   SAFplusI::CkptMapPair vt(kb,kv);
   m_mapObject.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/,size_t objlen)
{
   set(name.data(), handle, m, object,objlen);
}

void SAFplus::NameRegistrar::append(const char* name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/,size_t objlen)
{
   size_t len = strlen(name)+1;
   char data[sizeof(Buffer)-1+len];
   Buffer* key = new(data) Buffer(len);
   *key = name;
   const Buffer& buf = m_checkpoint.read(*key);
   if (&buf == NULL)
   {
      //There is no any name associated with this handle. Create first
      set(name, handle, m, object, objlen);
   }
   else
   {
      // TODO A name exists, add one more association.
      HandleMappingMode* phm = (HandleMappingMode*) buf.data;
      if (phm != NULL)
      {
         Vector newHandles = phm->getHandles();        
         newHandles.push_back(handle);
         MappingMode mm = phm->getMappingMode();
         if (m != MODE_NO_CHANGE) mm = m;
         //re-write the new buffer
         HandleMappingMode hm(mm, newHandles);
         size_t valLen = sizeof(hm);
         char vdata[sizeof(Buffer)-1+valLen];
         Buffer* val = new(vdata) Buffer(valLen);   
         memcpy(val->data, &hm, valLen);
         Transaction t;
         m_checkpoint.write(*key,*val,t);
      }
   }
   
   if (!object)
   {
      //Don't care the object
      return;
   }
   // TODO Associate this handle to the object
   size_t keyLen = sizeof(handle);
   char* pbuf = new char[keyLen+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (pbuf) SAFplus::Buffer(keyLen);
   memcpy(k->data, &handle, keyLen);
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapObject.find(SAFplusI::BufferPtr(k));  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == objlen)
            {
               memcpy (curval->data, object, objlen);
               delete k; // Release the buffer allocated for the key
               return;
            }
            // Replace the Buffer with a new one
            char* pvbuf = new char[objlen+sizeof(SAFplus::Buffer)-1];
            //char pvbuf[objlen+sizeof(SAFplus::Buffer)-1];
            SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer (objlen);
            memcpy (v->data, object, objlen);
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
            delete k; // Release the buffer allocated for the key
            return;
         }
      }
   }
   char* pvbuf = new char[objlen+sizeof(SAFplus::Buffer)-1];   
   SAFplus::Buffer* v = new (pvbuf) SAFplus::Buffer(objlen); 
   memcpy(v->data, object, objlen);
   SAFplusI::BufferPtr kb(k),kv(v);
   SAFplusI::CkptMapPair vt(kb,kv);
   m_mapObject.insert(vt);
}

void SAFplus::NameRegistrar::append(const std::string& name, SAFplus::Handle handle, MappingMode m, void* object/*=NULL*/,size_t objlen)
{
   append(name.data(), handle, m, object, objlen);
}

void SAFplus::NameRegistrar::set(const char* name, const void* data, int length)
{
   size_t keyLen = strlen(name)+1; //include '\0' for data
   char* pbuf = new char[keyLen+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (pbuf) SAFplus::Buffer(keyLen);
   *k = name;
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(SAFplusI::BufferPtr(k));  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == length)
            {
               memcpy (curval->data, data, length);
               delete k; // Release the buffer allocated for the key
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
   SAFplusI::BufferPtr kb(k),kv(v);
   SAFplusI::CkptMapPair vt(kb,kv);
   m_mapData.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, const void* data, int length)
{
   set(name.data(), data, length);
}

void SAFplus::NameRegistrar::set(const char* name, SAFplus::Buffer* p_buf)
{
   size_t keyLen = strlen(name)+1; //include '\0' for data
   char* pbuf = new char[keyLen+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (pbuf) SAFplus::Buffer(keyLen);
   *k = name;
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(SAFplusI::BufferPtr(k));  
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      SAFplusI::BufferPtr& curval = contents->second;
      if (curval)
      {
         if (curval->ref() == 1)
         {
            if (curval->len() == p_buf->len())
            {
               memcpy (curval->data, p_buf->data, p_buf->len());
               delete k; // Release the buffer allocated for the key
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
   SAFplusI::BufferPtr kb(k),kv(p_buf);
   SAFplusI::CkptMapPair vt(kb,kv);
   p_buf->addRef(); // add 1 referece to the callee
   m_mapData.insert(vt);
}
void SAFplus::NameRegistrar::set(const std::string& name, SAFplus::Buffer* p_buf)
{
   set(name.data(), p_buf);
}

std::pair<SAFplus::Handle&,void*> SAFplus::NameRegistrar::get(const char* name) throw(NameException&)
{
   try
   {
      Handle& handle = getHandle(name);
      //Next: get the void* object associated with the name then make the pair to return
      size_t keyLen = sizeof(handle); //include '\0' for data
      char buf[keyLen+sizeof(SAFplus::Buffer)-1]; 
      SAFplus::Buffer* k = new (buf) SAFplus::Buffer(keyLen);
      memcpy(k->data, &handle, keyLen);
      //Find to see if the key exists?
      HashMap::iterator contents = m_mapObject.find(SAFplusI::BufferPtr(k));  
      if (contents != m_mapObject.end()) // record already exists; return its value
      {
         SAFplusI::BufferPtr& curval = contents->second;
         if (curval)
         {
            if (curval->ref() == 1)
            {           
               return std::pair<SAFplus::Handle&,void*>(handle, curval->data);           
            }
         }
      }
      return (std::pair<SAFplus::Handle&,void*>(handle, NULL));
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
   size_t len = strlen(name)+1;
   char data[sizeof(Buffer)-1+len];
   Buffer* key = new(data) Buffer(len);
   *key = name;
   const Buffer& buf = m_checkpoint.read(*key);
   if (&buf != NULL)
   {
      HandleMappingMode* phm = (HandleMappingMode*) buf.data;
      MappingMode m = phm->getMappingMode();      
      size_t sz = phm->getHandles().size();
      if (m == MODE_REDUNDANCY)
      {
         // first association must be returned
         assert(sz > 0);    
         return phm->getHandles().at(0);
      }
      else if (m == MODE_ROUND_ROBIN)
      {
         srand (time(NULL));
         int idx = rand() % sz;
         assert(idx >=0 && idx < sz);
         return phm->getHandles().at(idx);
      }
      else if (m == MODE_PREFER_LOCAL)
      {
         pid_t thisPid = getpid();
         int i;
         for(i=0;i<sz;i++)
         {
            if (phm->getHandles().at(i).getProcess() == (uint32_t)thisPid)
            {
               return phm->getHandles().at(i);
            }
         }
         //No process match, get handle of THIS NODE
         for(i=0;i<sz;i++)
         {
            ClIocNodeAddressT thisNode = clIocLocalAddressGet();
            if ((uint32_t)phm->getHandles().at(i).getNode() == thisNode)
            {
               return phm->getHandles().at(i);
            }
         }
         // If no any match, get "closer" handle over others. It may latest handle
         return phm->getHandles().at(sz-1);
      }
      else // Other case, REDUNDANCY mode is picked
      {
         assert(sz > 0);
         return phm->getHandles().at(0);
      }
   }
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
   size_t keyLen = strlen(name)+1; //include '\0' for data
   char buf[keyLen+sizeof(SAFplus::Buffer)-1]; 
   SAFplus::Buffer* k = new (buf) SAFplus::Buffer(keyLen);
   *k = name;
   //Find to see if the key exists?
   HashMap::iterator contents = m_mapData.find(SAFplusI::BufferPtr(k));  
   if (contents != m_mapObject.end()) // record already exists; return its value
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
      return getData(name.data());
   }
   catch (NameException ne)
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
       //SAFplusI::BufferPtr& curval = iter.second;       
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
          HandleMappingMode* phm = (HandleMappingMode*) curval->data;          
          Vector v = phm->getHandles();
          size_t sz = v.size();
          for(int i=0;i<sz;i++)
          {
             printf("val [0x%x.0x%x]\n", v[i].id[0],v[i].id[1]);
          }
       }
    }
}

SAFplus::NameRegistrar::~NameRegistrar()
{
   // Free 2 maps
   for(HashMap::iterator iter = m_mapObject.begin(); iter != m_mapObject.end(); iter++)
   {       
       const SAFplusI::BufferPtr& k = iter->first;       
       SAFplusI::BufferPtr& v = iter->second;
       if (k) 
       {
          if  (k->ref() == 1) delete k.get();
          else k->decRef();
       }              
       if (v)
       {
          if  (v->ref() == 1) delete v.get();
          else v->decRef();
       }       
   }
   m_mapObject.clear();
   for(HashMap::iterator iter = m_mapData.begin(); iter != m_mapData.end(); iter++)
   {
       //CkptHashMap::value_type t = *iter;
       const SAFplusI::BufferPtr& k = iter->first;       
       SAFplusI::BufferPtr& v = iter->second;
       if (k)
       {
          if  (k->ref() == 1) delete k.get();
          else k->decRef();
       }       
       if (v)
       {
          if  (v->ref() == 1) delete v.get();
          else v->decRef();
       }       
   }
   m_mapData.clear();
}
