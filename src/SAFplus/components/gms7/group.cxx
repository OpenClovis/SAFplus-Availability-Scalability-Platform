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

/* TODO: Remove all printf, it is used for debugging */

SAFplus::Group::Group(std::string handleName)
{
  handle  = INVALID_HDL;
  try
  {
    /* TODO: resolve handle from name via NameRegistrar, uncomment below line */
    //handle = name.getHandle(handleName);
    /* TODO: remove below line, it is for testing */
    handle = SAFplus::Handle::create();
  }
  catch (SAFplus::NameException& ex)
  {
    /* If handle did not exist, Create it */
    handle = SAFplus::Handle::create();
    /* TODO: uncomment below line. After created handle, register it with Name Service*/
    //name.set(handleName,handle);
  }
  init(handle);
}

void SAFplus::Group::init(SAFplus::Handle groupHandle)
{
  char              sharedMemName[]= "GMS_SHM";
  int               numOfRows = SAFplusI::GROUP_SEGMENT_ROWS;

  /* No entity is registered at this time */
  lastRegisteredEntity = INVALID_HDL;

  /* Check whether handle is valid */
  if (groupHandle == INVALID_HDL)
  {
    /* TODO: Create new handle*/
  }

  /*Initialize shared memory for inter-process communication*/
  /* Name of shared memory*/
  //handle.toStr(sharedMemName);


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
      printf("Error %d \n",e.get_error_code());
      throw;
    }
  }

  /* Create GroupHashMap located in the shared memory */
  map = msm.find_or_construct<GroupHashMap>("GroupHashMap") ( numOfRows, boost::hash<GroupMapKey>(), BufferPtrContentsEqual(), msm.get_allocator<GroupMapPair>());

  assert(hdr);
}

void SAFplus::Group::registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities)
{
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  memcpy((char *)b->data,(char *)&me,sizeof(EntityIdentifier));
  //*((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));


  if (contents == map->end()) //Not exits
  {
    lastRegisteredEntity = me;
    /* Allocate shared memory */
    SAFplus::Buffer* key = new (msm.allocate(sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(EntityIdentifier)); //Area to store key
    SAFplus::Buffer* val = new (msm.allocate(sizeof(GroupIdentity) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(GroupIdentity));       //Area to store data struct

    /* Create information to store*/
    memcpy((char *)key->data,(char *)&me,sizeof(EntityIdentifier));
    //*((EntityIdentifier *) key->data) = me;

    GroupIdentity groupIdentity;
    groupIdentity.id = me;
    groupIdentity.credentials = credentials;
    groupIdentity.capabilities = capabilities;
    groupIdentity.dataLen = dataLength;
    groupIdentity.data = new (msm.allocate(dataLength + sizeof(SAFplus::Buffer) - 1)) SAFplus::Buffer (dataLength);
    memcpy((char *)groupIdentity.data->data,(char *)data, dataLength);
//  *((char *)groupIdentity.data->data) = *(char *)data;
    memcpy((char *)val->data,(char *)&groupIdentity,sizeof(GroupIdentity));
    //*((GroupIdentity *)val->data) = groupIdentity;

    /* Store in the shared memory */
    SAFplusI::BufferPtr kb(key),kv(val);
    SAFplusI::GroupMapPair vt(kb,kv);
    map->insert(vt);

    /* Notify other entities about new entity*/
    wakeable->wake(1);
  }
  else
  {
    /* TODO: handle entity existing case */
    ;
  }
}

void SAFplus::Group::deregister(EntityIdentifier me)
{
  /* Use last registered entity if me is 0 */
  if(me == INVALID_HDL)
  {
    me = lastRegisteredEntity;
  }
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
    return;
  }
  SAFplusI::BufferPtr curval = contents->second;
  SAFplusI::BufferPtr curkey = contents->first;
  /* Delete allocated memory for current EntityIdentifier */

  msm.deallocate(curkey.get());
  /*TODO: Deallocate stored area in Group Identity */
  //
  msm.deallocate(curval.get());

  /* Delete data from map */
  map->erase(contents);
  /* Active or Standby entity leave group. Re-elect */
  if(activeEntity == me || standbyEntity == me)
  {
    elect();
  }
  /*NOTE: the wakeable will send notification to the whole cluster to update the latest data*/
  wakeable->wake(1);
}

void SAFplus::Group::setCapabilities(uint capabilities, EntityIdentifier me)
{
  /* Use last registered entity if me is 0 */
  if(me == INVALID_HDL)
  {
    me = lastRegisteredEntity;
  }
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    /*TODO: EntityIdentifier didn't exist. */
    return;
  }

  SAFplusI::BufferPtr curval = contents->second;
  ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities = capabilities;
}

uint SAFplus::Group::getCapabilities(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = id;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
    return 0;
  }

  SAFplusI::BufferPtr curval = contents->second;
  return ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities;
}

SAFplus::Buffer& SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = id;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));
  if(contents == map->end())
  {
    return *((Buffer*) NULL);
  }
  SAFplusI::BufferPtr curval = contents->second;
  /* TODO: Which data should be returned? Group identity or data member of group identity? */
  //return *((GroupIdentity *)(((Buffer *)curval.get())->data))->data;
  return *(curval.get());
}

  /*
  Called by group member to select a new ACTIVE/STANDBY role
  Return a pair: first is active entity and second is standby entity
  Algorithm:
    * - Loop through entity mapping table
    * - Select entity with highest credentials for active role
    * - Select entity with second highest credential for standby role
    * - Check whether entities are allowed new role, then changing their roles
    * - Update capability of other entities if role changed
    * - Notify to other group member about roles changing
  */
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect()
{
  uint highestCredentials = 0,lowerCredentials = 0, curEntityCredentials = 0;
  EntityIdentifier entityIdenFirst = INVALID_HDL, entityIdenSecond = INVALID_HDL;
  bool isRoleChanged = false;

  for (GroupHashMap::iterator i = map->begin();i != map->end();i++)
  {
    SAFplusI::BufferPtr curval = i->second;
    curEntityCredentials = ((GroupIdentity *)(((Buffer *)curval.get())->data))->credentials;
    if(highestCredentials == 0 || lowerCredentials == 0)
    {
      entityIdenFirst = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      entityIdenSecond = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      highestCredentials = curEntityCredentials;
      lowerCredentials = curEntityCredentials;
    }
    else
    {
      if(curEntityCredentials > lowerCredentials && curEntityCredentials <= highestCredentials)
      {
        entityIdenSecond = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
        lowerCredentials = curEntityCredentials;
      }
      else if(curEntityCredentials > highestCredentials)
      {
        entityIdenSecond = entityIdenFirst;
        lowerCredentials = highestCredentials;
        entityIdenFirst = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
        highestCredentials = curEntityCredentials;
      }
      else
      {
        continue;
      }
    }
  }
  /* Update standby and active entity */
  if(!(entityIdenFirst == INVALID_HDL) && !(entityIdenFirst == activeEntity))
  {
    /* Check if entity is allowed ACTIVE */
    uint capability = getCapabilities(entityIdenFirst);
    if((capability & SAFplus::Group::ACCEPT_ACTIVE) == SAFplus::Group::ACCEPT_ACTIVE)
    {
      isRoleChanged = true;
      activeEntity = entityIdenFirst;
    }

  }
  if(!(entityIdenSecond == INVALID_HDL) && !(entityIdenSecond == standbyEntity))
  {
    /* Check if entity is allowed STANDBY */
    uint capability = getCapabilities(entityIdenSecond);
    if((capability & SAFplus::Group::ACCEPT_STANDBY) == SAFplus::Group::ACCEPT_STANDBY)
    {
      isRoleChanged = true;
      standbyEntity = entityIdenSecond;
    }
  }

  /* Reset state IS_LEADER/IS_STANDBY of other entities */
  for (GroupHashMap::iterator i = map->begin();isRoleChanged && i != map->end();i++)
  {
    SAFplusI::BufferPtr curkey = i->first;
    EntityIdentifier item = *(EntityIdentifier *)(curkey.get()->data);
    uint currentCapability = getCapabilities(item);
    if(item == activeEntity) //This is an active entity
    {
      currentCapability |= SAFplus::Group::IS_ACTIVE;
      currentCapability &= ~SAFplus::Group::IS_STANDBY;
    }
    else if(item == standbyEntity) //This is a standby entity
    {
      currentCapability |= SAFplus::Group::IS_STANDBY;
      currentCapability &= ~SAFplus::Group::IS_ACTIVE;
    }
    else //other members
    {
      currentCapability &= ~SAFplus::Group::IS_STANDBY;
      currentCapability &= ~SAFplus::Group::IS_ACTIVE;
    }
    setCapabilities(currentCapability,item);
  }

  /* TODO: Send notification about role changes */
  if(isRoleChanged)
  {
    wakeable->wake(1);
  }
  return std::pair<EntityIdentifier,EntityIdentifier>(entityIdenFirst, entityIdenSecond);
}

bool SAFplus::Group::isMember(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = id;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

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
  wakeable = (Wakeable *)&w;
}

EntityIdentifier SAFplus::Group::getActive(void) const
{
  return activeEntity;
}

EntityIdentifier SAFplus::Group::getStandby(void) const
{
  return standbyEntity;
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

void SAFplus::Group::receiveNotification()
{
  /* TODO: check notification type and do approriate actions */
  ;
}
