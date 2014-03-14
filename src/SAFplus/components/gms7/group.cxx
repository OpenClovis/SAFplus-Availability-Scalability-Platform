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
  char              sharedMemName[]= "ABC";
  int               numOfRows = SAFplusI::GROUP_SEGMENT_ROWS;

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
    printf("MSM OK");
  }
  catch(interprocess_exception &e)
  {
    printf("interprocess_exception \n");
    if (e.get_error_code() == already_exists_error)
    {
        hdr = msm.find_or_construct<SAFplusI::GroupBufferHeader>("GroupBufferHeader") ();
        int     retries  =  0;
        while ((hdr->structId != SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        if (retries >= 2)
        {
          hdr->serverPid = getpid();
          hdr->structId = SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7;
          printf("RETRIES NG \n");
        }
        printf("RETRIES OK \n");
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

void SAFplus::Group::registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities, bool wake)
{
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  memcpy((char *)b->data,(char *)&me,sizeof(EntityIdentifier));
  //*((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  printf("Enter registerEntity \n");
  if (contents == map->end()) //Not exits
  {
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

    /*NOTE: the wakeable will send notification to the whole cluster to update the latest data*/
    if (wake)
    {
      wakeable->wake(1);
    }
    printf("Exit registerEntity. Insertion is ok. \n");
  }
  else
  {
    /* TODO: handle entity existing case */
    printf("Exit registerEntity. Entity existed. \n");
  }
}

void SAFplus::Group::deregister(EntityIdentifier me, bool wake)
{
  printf("Enter deregister \n");

  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    /*TODO: Entity didn't exist. */
    printf("Exit deregister. Entity didn't exist \n");
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

  /*NOTE: the wakeable will send notification to the whole cluster to update the latest data*/
  if (wake)
  {
    wakeable->wake(1);
  }
  printf("Exit deregister. Deregister OK \n");
}

void SAFplus::Group::setCapabilities(uint capabilities, EntityIdentifier me)
{
  printf("Enter setCapabilities \n");
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = me;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    /*TODO: EntityIdentifier didn't exist. */
    printf("Exit setCapabilities. Entity didn't exist \n");
    return;
  }

  SAFplusI::BufferPtr curval = contents->second;
  ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities = capabilities;
  printf("Exit setCapabilities. Updated new capabilities \n");
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
    printf("EntifyIdentifier doen't exits \n");
    return 0;
  }

  SAFplusI::BufferPtr curval = contents->second;
  return ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities;
}

SAFplus::Buffer& SAFplus::Group::getData(EntityIdentifier id)
{
  printf("Enter getData\n");
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = id;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));
  if(contents == map->end())
  {
    printf("Exit getData. Entity didn't exist \n");
    return *((Buffer*) NULL);
  }
  SAFplusI::BufferPtr curval = contents->second;
  printf("Exit getData.\n");
  /* TODO: Why GroupEntity::data did not exist after re-run */
  return *((GroupIdentity *)(((Buffer *)curval.get())->data))->data;
}

// Calls for an election
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect()
{
  /*TODO Implement "bully" election*/
  uint highestCredentials = 0,lowerCredentials = 0, curEntityCredentials = 0;

  active = INVALID_HDL;
  standby = INVALID_HDL;

  for (GroupHashMap::iterator i = map->begin();i != map->end();i++)
  {
    SAFplusI::BufferPtr curval = i->second;
    curEntityCredentials = ((GroupIdentity *)(((Buffer *)curval.get())->data))->credentials;
    if(highestCredentials == 0 || lowerCredentials == 0)
    {
      active = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      standby = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      highestCredentials = curEntityCredentials;
      lowerCredentials = curEntityCredentials;
    }
    else
    {
      if(curEntityCredentials > lowerCredentials && curEntityCredentials <= highestCredentials)
      {
        standby = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
        lowerCredentials = curEntityCredentials;
      }
      else if(curEntityCredentials > highestCredentials)
      {
        standby = active;
        lowerCredentials = highestCredentials;
        active = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
        highestCredentials = curEntityCredentials;
      }
      else
      {
        ;
      }
    }
  }
  return std::pair<EntityIdentifier,EntityIdentifier>(active, standby);
}

bool SAFplus::Group::isMember(EntityIdentifier id)
{
  printf("Enter isMember\n");
  /*Check in share memory if entity exists*/
  char tmpData[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* b = new(tmpData) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) b->data) = id;
  GroupHashMap::iterator contents = map->find(SAFplusI::BufferPtr(b));

  if(contents == map->end())
  {
    printf("Exit isMember. Not found\n");
    return false;
  }
  else
  {
    printf("Exit isMember. OK\n");
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
