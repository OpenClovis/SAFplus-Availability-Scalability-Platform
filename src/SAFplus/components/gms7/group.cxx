/* Standard headers */
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <boost/functional/hash.hpp>

/* SAFplus headers */
#include <clCommon.hxx>
#include <clNameApi.hxx>
#include <clGroup.hxx>

using namespace boost::interprocess;
using namespace SAFplus;
using namespace SAFplusI;

SAFplus::Group::Group(std::string name)
{
  handle  = INVALID_HDL;
  try
  {
    /* TODO: resolve handle from name via NameRegistrar */
    //handle = SAFplus::NameRegistrar.get(name);
    handle = SAFplus::Handle::create();
  }
  catch (SAFplus::NameException& ex)
  {
    return;
  }
  init(handle);
}

void SAFplus::Group::init(SAFplus::Handle groupHandle)
{
  char              sharedMemName[81];
  int               numOfRows = SAFplusI::GROUP_SEGMENT_ROWS;

  /* Check whether handle is valid */
  if (groupHandle == INVALID_HDL)
  {
    /* TODO: Create new handle*/
  }

  /*Initialize shared memory for inter-process communication*/
  /* Name of shared memory*/
  handle.toStr(sharedMemName);

  /* Open or create shared memory */
  msm = managed_shared_memory(open_or_create, sharedMemName, SAFplusI::GROUP_SEGMENT_SIZE);
  try
  {
    hdr = msm.construct<SAFplusI::GroupBufferHeader>("GroupBufferHeader") ();
    hdr->serverPid = getpid();
    hdr->structId = SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7;
  }
  catch(interprocess_exception &e)
  {
    if (e.get_error_code() == already_exists_error)
    {
        hdr = msm.find_or_construct<SAFplusI::GroupBufferHeader>("GroupBufferHeader") ();
        int     retries  =  0;
        while ((hdr->structId != SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        if (retries >= 2)
        {
          hdr->serverPid = getpid();
          hdr->structId = SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7;
        }
    }
    else
    {
      throw;
    }
  }
  /* Create GroupHashMap located in the shared memory */
  map = msm.find_or_construct<GroupHashMap>("GroupHashMap")  ( numOfRows, boost::hash<GroupMapKey>(), BufferPtrContentsEqual(), msm.get_allocator<GroupMapPair>());

  assert(hdr);
}

void SAFplus::Group::registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities, bool wake)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&me));

  if (contents == map->end()) //Not exits
  {
    /* Allocate shared memory */
    SAFplus::Buffer* key = new (msm.allocate(sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(EntityIdentifier)); //Area to store key
    SAFplus::Buffer* val = new (msm.allocate(sizeof(GroupIdentity) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(GroupIdentity));       //Area to store data struct
    SAFplus::Buffer* dataArea = new (msm.allocate(sizeof(SAFplus::Buffer) + dataLength - 1)) SAFplus::Buffer (dataLength);              //Area to store actual data

    /* Create information to store*/
    *(EntityIdentifier *)key = me;
    GroupIdentity* groupIdentity = (GroupIdentity *)val;
    groupIdentity->id = me;
    groupIdentity->credentials = credentials;
    groupIdentity->capabilities = capabilities;
    groupIdentity->data = dataArea;
    groupIdentity->dataLen = dataLength;
    memcpy(groupIdentity->data,(SAFplus::Buffer *)data,dataLength);

    /* Store in the shared memory */
    SAFplusI::BufferPtr kb(key),kv(val);
    SAFplusI::GroupMapPair vt(kb,kv);
    map->insert(vt);

    /*NOTE: the wakeable will send notification to the whole cluster to update the latest data*/
    if (wake)
    {
      wakeable->wake(1);
    }
  }
  else
  {
    /* TODO: handle entity existing case */
  }
}

void SAFplus::Group::deregister(EntityIdentifier me, bool wake)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&me));
  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
  }
  /* Get key and value from iterator */
  SAFplusI::BufferPtr curval = contents->second;
  SAFplusI::BufferPtr curkey = contents->first;
  GroupIdentity *groupIdentity = (GroupIdentity *)&curval;

    /* Delete data from map */
    map->erase(contents);
    /* Delete allocated memory for current EntityIdentifier */
    msm.deallocate(groupIdentity->data);
    msm.deallocate(&curval);
    msm.deallocate(&curkey);

  /*NOTE: the wakeable will send notification to the whole cluster to update the latest data*/
  if (wake)
  {
    wakeable->wake(1);
  }

}

void SAFplus::Group::setCapabilities(uint capabilities, EntityIdentifier me)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&me));
  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
  }

  SAFplusI::BufferPtr curval = contents->second;
  GroupIdentity *groupIdentity = (GroupIdentity *)&curval;
  groupIdentity->capabilities = capabilities;
}

uint SAFplus::Group::getCapabilities(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&id));
  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
    return 0;
  }
  SAFplusI::BufferPtr curval = contents->second;
  GroupIdentity *groupIdentity = (GroupIdentity *)&curval;
  return groupIdentity->capabilities;
}

SAFplus::Buffer& SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&id));
  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */

  }
  SAFplusI::BufferPtr curval = contents->second;
  GroupIdentity *groupIdentity = (GroupIdentity *)&curval;
  return *(groupIdentity->data);
}

// Calls for an election
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect()
{
  /*TODO Implement "bully" election*/

  return std::pair<EntityIdentifier,EntityIdentifier>(active, standby);
}

bool SAFplus::Group::isMember(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr((Buffer*)&id));
  if(contents == map->end())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void SAFplus::Group::setNotification(SAFplus::Wakeable& w)
{
  wakeable = (GroupWakeable *)&w;
}

EntityIdentifier SAFplus::Group::getActive(void) const
{
  return active;
}

EntityIdentifier SAFplus::Group::getStandby(void) const
{
  return standby;
}

/*
 * Implementation for Group::Iterator
 */

SAFplus::Group::Iterator SAFplus::Group::begin()
{
  SAFplus::Group::Iterator i(this);
  assert(this->map);
  i.iter = this->map->begin();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Group::Iterator SAFplus::Group::end()
{
  SAFplus::Group::Iterator i(this);
  assert(this->map);
  i.iter = this->map->end();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Group::Iterator::Iterator(SAFplus::Group* _group):group(_group)
{
  curval = NULL;
}

SAFplus::Group::Iterator::~Iterator()
{
  group = NULL;
  curval = NULL;
}

SAFplus::Group::Iterator& SAFplus::Group::Iterator::operator++()
{
  iter++;
  curval = &(*iter);
  return *this;
}

SAFplus::Group::Iterator& SAFplus::Group::Iterator::operator++(int)
{
  iter++;
  curval = &(*iter);
  return *this;
}

bool SAFplus::Group::Iterator::operator !=(const SAFplus::Group::Iterator& otherValue) const
{
  if (group != otherValue.group) return true;
  if (iter != otherValue.iter) return true;
  return false;
}
