#include <clLogApi.hxx>
#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clCustomization.hxx>
#include <clGlobals.hxx>

using namespace SAFplus;
using namespace SAFplusI;

#define ENDIAN_SWAP_U64(val) ((uint64_t) ( \
    (((uint64_t) (val) & (uint64_t) 0x00000000000000ff) << 56) | \
    (((uint64_t) (val) & (uint64_t) 0x000000000000ff00) << 40) | \
    (((uint64_t) (val) & (uint64_t) 0x0000000000ff0000) << 24) | \
    (((uint64_t) (val) & (uint64_t) 0x00000000ff000000) <<  8) | \
    (((uint64_t) (val) & (uint64_t) 0x000000ff00000000) >>  8) | \
    (((uint64_t) (val) & (uint64_t) 0x0000ff0000000000) >> 24) | \
    (((uint64_t) (val) & (uint64_t) 0x00ff000000000000) >> 40) | \
    (((uint64_t) (val) & (uint64_t) 0xff00000000000000) >> 56)))


NameRegistrar SAFplus::name;
int nameInitCount=0;
void SAFplus::nameInitialize()
  {
    nameInitCount++;
    if (nameInitCount > 1) return;
    name.init(NAME_CKPT);
  }

void NameRegistrar::init(Handle hdl)
  {
  m_checkpoint.name = "safplusName";
  m_checkpoint.init(hdl,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, CkptDefaultSize, CkptDefaultRows,IGNORE);
  }

void NameRegistrar::set(const char* name, Handle handle, MappingMode m)
{
#if 0
   short numHandles = 1;
   size_t valLen = sizeof(HandleData)+numHandles-1;
   char vdata[sizeof(Buffer)-1+valLen];
   Buffer* val = new(vdata) Buffer(valLen);
   HandleData* data = (HandleData*) val->data;
   data->structIdAndEndian = STRID;
   data->numHandles = numHandles;
   data->mappingMode = m;
   data->handles[i] = handle;
   m_checkpoint.write(name,*val);
#endif
   HandleData data;
   data.structIdAndEndian = NameRegistrar::STRID;
   data.mappingMode = m;
   data.numHandles = 1;
   data.handles[0] = handle;
   set(name, &data, sizeof(data));
}

void NameRegistrar::set(const std::string& name, Handle handle, MappingMode m)
{
   set(name.c_str(), handle, m);
}

void NameRegistrar::append(const char* name, Handle handle, MappingMode m) throw (NameException&)
{
   const Buffer& buf = m_checkpoint.read(name);
   if (&buf == NULL)
   {
      //There is no any name associated with this handle. Create first
      set(name, handle, m);
   }
   else
   {
      // TODO A name exists, add one more association.
      HandleData* data = (HandleData*) buf.data;
      if (data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) // Arbitrary data in this case
      {
         throw NameException("Unable to append the handle because only arbitrary data registered for this name");
      }
      short numHandles = data->numHandles+1;
#if 0
      size_t valLen = sizeof(HandleData)+numHandles-1;
      char vdata[sizeof(Buffer)-1+valLen];
      Buffer* val = new(vdata) Buffer(valLen);
      HandleData* newData = (HandleData*) val->data;
      newData->structIdAndEndian = data->structIdAndEndian;
      newData->numHandles = numHandles;
      if (m == MODE_NO_CHANGE)
      {
         newData->mappingMode = data->mappingMode;
      }
      else
      {
         newData->mappingMode = m;
      }
      for (int i =0;i<numHandles-1;i++)
      {
         newData->handles[i] = data->handles[i];
      }
      newData->handle[numHandles-1] = handle;
      m_checkpoint.write(name,*val);
#endif
      size_t hLen = sizeof(HandleData)+sizeof(Handle)*(numHandles-1);
      char hdata[hLen];
      HandleData* newData = new(hdata) HandleData;
      newData->structIdAndEndian = data->structIdAndEndian;
      newData->numHandles = numHandles;
      if (m == MODE_NO_CHANGE)
      {
         newData->mappingMode = data->mappingMode;
      }
      else
      {
         newData->mappingMode = m;
      }
      for (int i =0;i<numHandles-1;i++)
      {
         newData->handles[i] = data->handles[i];
      }
      newData->handles[numHandles-1] = handle;
      set(name, newData, hLen);
   }
}

void NameRegistrar::append(const std::string& name, Handle handle, MappingMode m) throw (NameException&)
{
   try {
      append(name.c_str(), handle, m);
   }
   catch (NameException ne) {
      logError("NAME","APD","Exception cautch [%s]", ne.what());
      throw ne;
   }
}

void NameRegistrar::setLocalObject(const char* name, void* object)
{
   Handle handle;
   try
   {
      handle = getHandle(name);
   }
   catch(NameException ne)
   {
      logError("NAME","SETOBJ","Exception caught [%s]", ne.what());
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
   ObjMapPair vt(handle,object);
   m_mapObject.insert(vt);
}

void NameRegistrar::setLocalObject(const std::string& name, void* object)
{
   setLocalObject(name.c_str(), object);
}

void NameRegistrar::setLocalObject(Handle handle, void* object)
{
   //Find to see if the key exists?
   ObjHashMap::iterator contents = m_mapObject.find(handle);
   if (contents != m_mapObject.end()) // record already exists; overwrite
   {
      //void* oldObj = contents->second;
      contents->second = object;
      //delete oldObj; Do not delete the object; it must be deleted by owner process
      return;
   }
   ObjMapPair vt(handle,object);
   m_mapObject.insert(vt);
}

void NameRegistrar::set(const char* name, const void* data, int length) throw (NameException&)
{
   size_t valLen = length;
   char vdata[sizeof(Buffer)-1+valLen];
   Buffer* val = new(vdata) Buffer(valLen);
   memcpy(val->data, data, valLen);
   m_checkpoint.write(name,*val);
}

void NameRegistrar::set(const std::string& name, const void* data, int length) throw (NameException&)
{
   try {
      set(name.c_str(), data, length);
   }catch (NameException &ne) {
      throw ne;
   }
}

void NameRegistrar::remove(const char* name) throw (NameException&)
{
   m_checkpoint.remove(name);
}

void NameRegistrar::remove(const std::string& name) throw (NameException&)
{
  m_checkpoint.remove(name);
}


void NameRegistrar::set(const char* name, Buffer* p_buf) throw (NameException&)
{
   m_checkpoint.write(name,*p_buf);
}

void NameRegistrar::set(const std::string& name, Buffer* p_buf) throw (NameException&)
{
   try {
      set(name.c_str(), p_buf);
   }catch (NameException &ne) {
      throw ne;
   }
}

RefObjMapPair NameRegistrar::get(const char* name) throw(NameException&)
{
#if 0
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
#endif
   try
   {
      Handle& handle = getHandle(name);
      try
      {
         void* obj = get(handle);
         return RefObjMapPair(handle, obj);
      }
      catch (NameException &ne)
      { // Not throwing, continue to search below
         logWarning("NAME","GET","Exception caught [%s]", ne.what());
      }
      Checkpoint::Iterator ibegin = m_checkpoint.begin();
      Checkpoint::Iterator iend = m_checkpoint.end();
      for(Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
      {
         BufferPtr curkey = iter->first;
         logDebug("NAME","GET","Get object: key [%s]", curkey.get()->data);

         BufferPtr& curval = iter->second;
         if (curval)
         {
            HandleData* data = (HandleData*) curval->data;
            if ((data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) || // Arbitrary data in this case
                (strcmp(curkey.get()->data, name)))
            {
               continue;
            }
            short sz = data->numHandles;
            for(int i=0;i<sz;i++)
            {
               try
               {
                  void* obj = get(data->handles[i]);
                  return RefObjMapPair(handle, obj);
               }
               catch (NameException &ne)
               { // Not throwing, continue to search
                  logWarning("NAME","GET","Exception caught [%s]", ne.what());
               }
            }
         }
      }
      return RefObjMapPair(handle, NULL);
   }
   catch (NameException &ne)
   {
      throw ne;
   }
}

RefObjMapPair NameRegistrar::get(const std::string& name) throw(NameException&)
{
   try
   {
      //return get(name.c_str());
      RefObjMapPair hobj = get(name.c_str());
      return hobj;
   }
   catch(NameException &ne)
   {
      throw ne;
   }
}

void* NameRegistrar::get(const Handle& handle) throw (NameException&)
{
   ObjHashMap::iterator contents = m_mapObject.find(handle);
   if (contents != m_mapObject.end()) // record already exists; return its value
   {
      return contents->second;
   }
   throw NameException("Handle provided does not exist");
}

std::string NameRegistrar::Iterator::name()
  {
  const Buffer& buf = *(it->first);
  std::string ret = (char*) buf.data;
  return ret;
  }

HandleData& NameRegistrar::Iterator::handle()
  {
  const Buffer& buf = *(it->second);
  HandleData* data = (HandleData*) buf.data;
  return *data;
  }

Handle& NameRegistrar::getHandle(const char* name) throw(NameException&)
{
   const Buffer& buf = m_checkpoint.read(name);
   if (&buf != NULL)
   {
      HandleData* data = (HandleData*) buf.data;
      if (data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) // Arbitrary data in this case
      {
         throw NameException("Unable to get handle because only arbitrary data registered for this name");
      }
      size_t sz = data->numHandles;
      MappingMode m = data->mappingMode;
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
            if (data->handles[i].getProcess() == (uint32_t)thisPid)
            {
               idx = i;
               break;
            }
         }
         //No process match, get handle of THIS NODE
         if (idx == -1)
         {
            uint_t thisNode = SAFplus::ASP_NODEADDR;
            logDebug("NAME","GETHDL","getHandle of name [%s]: thisNode [%d]", name, thisNode);
            for(i=0;i<sz;i++)
	    {
		if ((uint32_t)data->handles[i].getNode() == thisNode)
		{
                   idx = i;
                   break;
		}
            }
         }
         // If no any match, get "closer" handle over others. It may be the latest one
         if (idx == -1)
         {
            idx = sz-1;
         }
      }
      else // Other case, REDUNDANCY mode is picked
      {
         assert(sz > 0);
         idx = 0;
      }
      assert(idx >=0 && idx < sz);
      if (data->structIdAndEndian == STRIDEN) // Need to swap the endian
      {
         data->handles[idx].id[0] = ENDIAN_SWAP_U64(data->handles[idx].id[0]);
         data->handles[idx].id[1] = ENDIAN_SWAP_U64(data->handles[idx].id[1]);
      }
      if (data->handles[idx] == INVALID_HDL) throw NameException("Provided name has invalid handle");
      return data->handles[idx];
   }
   throw NameException("name provided does not exist");
}

Handle& NameRegistrar::getHandle(const std::string& name) throw(NameException&)
{
  Handle& handle = getHandle(name.c_str());
  return handle;
}

char* NameRegistrar::getName(const SAFplus::Handle& handle) throw(NameException&)
{
   Checkpoint::Iterator ibegin = m_checkpoint.begin();
   Checkpoint::Iterator iend = m_checkpoint.end();
   for(Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
   {
      BufferPtr curkey = iter->first;
      // getName CANNOT log because logging uses it: logDebug("NAME","GETNAME","key [%s]\n", curkey.get()->data);

      BufferPtr& curval = iter->second;
      if (curval)
      {
         HandleData* data = (HandleData*) curval->data;
         if (data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) // Arbitrary data in this case
         {
            continue;
         }
         short sz = data->numHandles;
         for(int i=0;i<sz;i++)
         {
            if (data->handles[i] == handle)
            {
               return curkey.get()->data;
            }
         }        
      }
      else
      {
         // getName CANNOT log because logging uses it: logWarning("NAME","GETNAME","name data is empty");
      }
   }
   throw NameException("Handle provided doesn't exist in Name");
}

const Buffer& NameRegistrar::getData(const char* name) throw(NameException&)
{
   const Buffer& buf = m_checkpoint.read(name);
   if (&buf != NULL)
   {
      HandleData* data = (HandleData*) buf.data;
      if (data->structIdAndEndian == STRID || data->structIdAndEndian == STRIDEN) // Handles in this case
      {
         throw NameException("Unable to get arbitrary data because only handle registered for this name");
      }
      return buf;
   }
   throw NameException("Name provided does not exist");
}

const Buffer& NameRegistrar::getData(const std::string& name) throw(NameException&)
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

void NameRegistrar::handleFailure(const FailureType failureType, const uint32_t id, const uint32_t amfId)
{
   Checkpoint::Iterator ibegin = m_checkpoint.begin();
   Checkpoint::Iterator iend = m_checkpoint.end();
   for(Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
   {
      BufferPtr curkey = iter->first;
      logDebug("NAME","HDLFAIL","key [%s]", curkey.get()->data);

      BufferPtr& curval = iter->second;
      if (curval)
      {
         HandleData* data = (HandleData*) curval->data;
         if ((data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) || // Arbitrary data in this case
             (data->mappingMode != NameRegistrar::MODE_PREFER_LOCAL))
         {
            continue;
         }
         short sz = data->numHandles;
         short numHandles = sz;
         unsigned char markedRemoval[sz];
         memset(markedRemoval, 0, sz);
         for(int i=0;i<sz;i++)
         {
            if ((failureType == NameRegistrar::FAILURE_PROCESS && data->handles[i].getProcess() == id) ||
                (failureType == NameRegistrar::FAILURE_NODE && data->handles[i].getNode() == (uint16_t)id))
            {
               if (sz == 1) // There is only one handle registered, then remove the name and its value
               {
                  logDebug("NAME","HDLFAIL","Removing element with key [%s]", curkey.get()->data);
#if 0
                  m_checkpoint.remove(curval);
                  m_checkpoint.remove(curkey, true);
#endif
                  m_checkpoint.remove(*curkey);
               }
               else  //Remove this element and push remaining handles back to checkpoint
               {
                  markedRemoval[i] = 1;
                  numHandles--;
               }
               //return;
            }
         }
         if (numHandles == sz) // No handles is removed. continue
         {
            continue;
         }
         size_t hLen = sizeof(HandleData)+sizeof(Handle)*(numHandles-1);
         char hdata[hLen];
         HandleData* newData = new(hdata) HandleData;
         newData->structIdAndEndian = data->structIdAndEndian;
         newData->numHandles = numHandles;
         newData->mappingMode = data->mappingMode;
         int j=0;
         for (int i=0;i<sz;i++)
         {
            if (!markedRemoval[i])
            {
               newData->handles[j] = data->handles[i];
               j++;
            }
         }
         set(curkey.get()->data, newData, hLen);
      }
      else
      {
         logWarning("NAME","HDLFAIL","name data is empty");
      }
   }
}

void NameRegistrar::processFailed(const uint32_t pid, const uint32_t amfId)
{
   handleFailure(NameRegistrar::FAILURE_PROCESS, pid, amfId);
}

void NameRegistrar::nodeFailed(const uint16_t slotNum, const uint32_t amfId)
{
   handleFailure(NameRegistrar::FAILURE_NODE, (uint32_t)slotNum, amfId);
}

void NameRegistrar::dump()
{
   logInfo("NAME","DUMP","---------------------------------");
   SAFplus::Checkpoint::Iterator ibegin = m_checkpoint.begin();
   SAFplus::Checkpoint::Iterator iend = m_checkpoint.end();
   for(SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
   {
       BufferPtr curkey = iter->first;
       if (curkey)
       {
          logInfo("NAME","DUMP","key [%s]", curkey->data);
       }
       BufferPtr& curval = iter->second;
       if (curval)
       {
          HandleData* data = (HandleData*) curval->data;
          if (data->structIdAndEndian != STRID && data->structIdAndEndian != STRIDEN) // Arbitrary data in this case
          {
             logInfo("NAME","DUMP","Arbitrary data [%s]", curval->data);
          }
          else
          {
             size_t sz = data->numHandles;
             for(int i=0;i<sz;i++)
             {
                logInfo("NAME","DUMP","val [0x%lx.0x%lx]", (long unsigned int) data->handles[i].id[0],(long unsigned int) data->handles[i].id[1]);
             }
          }
       }
    }
   logInfo("NAME","DUMP","---------------------------------");
}

void NameRegistrar::dumpObj()
{
   logInfo("NAME","OBJDUMP","---------------------------------");
   for(ObjHashMap::iterator iter = m_mapObject.begin(); iter != m_mapObject.end(); iter++)
   {
       ObjHashMap::value_type vt = *iter;
       Handle curkey = vt.first;
       logInfo("NAME","OBJDUMP","key [%" PRIx64 ":%" PRIx64 "]", curkey.id[0], curkey.id[1]);
       void* obj = vt.second;
       if (obj)
       {
          logInfo("NAME","OBJDUMP","Obj associated Not NULL");
       }
       else
       {
          logInfo("NAME","OBJDUMP","Obj associated IS NULL");
       }
   }
   logInfo("NAME","OBJDUMP","---------------------------------");
}

NameRegistrar::~NameRegistrar()
{
}
