/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clCommon.hxx>
//#include <clNameApi.hxx>
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

  /* No entity is registered at this time */
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;

  /* Check whether handle is valid */
  if (groupHandle == INVALID_HDL)
  {
    /* TODO: Create new handle*/
  }
  initializeSharedMemory(sharedMemName);
}

void SAFplus::Group::initializeSharedMemory(char sharedMemName[])
{
  /* Remove old shared memory which is allocated by me */
  shared_memory_object::remove(sharedMemName);

  /* Create shared memory */
  msm = managed_shared_memory(open_or_create, sharedMemName, SAFplus::Group::GROUP_SEGMENT_SIZE);
  try
  {
    hdr = msm.construct<SAFplusI::GroupBufferHeader>("GroupBufferHeader") ();
    hdr->serverPid = getpid();
    hdr->structId = SAFplus::Group::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7;
  }
  catch(interprocess_exception &e)
  {
    if (e.get_error_code() == already_exists_error)
    {
        hdr = msm.find_or_construct<SAFplusI::GroupBufferHeader>("GroupBufferHeader") ();
        int     retries  =  0;
        while ((hdr->structId != SAFplus::Group::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
        if (retries >= 2)
        {
          hdr->serverPid = getpid();
          hdr->structId = SAFplus::Group::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7;
        }
    }
    else
    {
      printf("Error %d \n",e.get_error_code());
      throw;
    }
  }

  /* Create GroupHashMap located in the shared memory */
  map = msm.find_or_construct<GroupHashMap>("GroupHashMap") ( (int)SAFplus::Group::GROUP_SEGMENT_ROWS, boost::hash<GroupMapKey>(), BufferPtrContentsEqual(), msm.get_allocator<GroupMapPair>());

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


  if (contents == map->end())
  {
    /* Keep track of last register entity */
    lastRegisteredEntity = me;
    /* Allocate shared memory to store data*/
    SAFplus::Buffer* key = new (msm.allocate(sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(EntityIdentifier)); //Area to store key
    SAFplus::Buffer* val = new (msm.allocate(sizeof(GroupIdentity) + sizeof(SAFplus::Buffer)-1)) SAFplus::Buffer (sizeof(GroupIdentity));       //Area to store data struct

    /* Create information to store*/
    memcpy((char *)key->data,(char *)&me,sizeof(EntityIdentifier));

    GroupIdentity groupIdentity;
    groupIdentity.id = me;
    groupIdentity.credentials = credentials;
    groupIdentity.capabilities = capabilities;
    groupIdentity.dataLen = dataLength;
    groupIdentity.data = new (msm.allocate(dataLength + sizeof(SAFplus::Buffer) - 1)) SAFplus::Buffer (dataLength);
    memcpy((char *)groupIdentity.data->data,(char *)data, dataLength);
    memcpy((char *)val->data,(char *)&groupIdentity,sizeof(GroupIdentity));

    /* Store in the shared memory */
    SAFplusI::BufferPtr kb(key),kv(val);
    SAFplusI::GroupMapPair vt(kb,kv);
    map->insert(vt);

    /* Notify other entities about new entity*/
    if(wakeable)
    {
      wakeable->wake(1);
    }

  }
  else
  {
    /* TODO: handle entity existing case */
    ;
  }
}

void SAFplus::Group::registerEntity(GroupIdentity grpIdentity)
{
  registerEntity(grpIdentity.id,grpIdentity.credentials, grpIdentity.data, grpIdentity.dataLen, grpIdentity.capabilities);
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

  /*TODO: Deallocate stored area in Group Identity */
  msm.deallocate(curkey.get());
  msm.deallocate(curval.get());

  /* Delete data from map */
  map->erase(contents);
  /* Notify other entities about new entity*/
  if(wakeable)
  {
    wakeable->wake(2);
  }
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
    /*TODO: EntityIdentifier did not exist. */
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
    /*TODO: Entity did not exist. */
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

void SAFplus::Group::updateGroupRoles()
{
  for (GroupHashMap::iterator i = map->begin(); i != map->end();i++)
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
}

EntityIdentifier SAFplus::Group::electLeader()
{
  uint highestCredentials = 0, curCredentials = 0;
  uint curCapabilities = 0;
  EntityIdentifier leaderEntity = INVALID_HDL;
  for (GroupHashMap::iterator i = map->begin();i != map->end();i++)
  {
    SAFplusI::BufferPtr curval = i->second;
    curCredentials = ((GroupIdentity *)(((Buffer *)curval.get())->data))->credentials;
    curCapabilities = ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities;
    if((curCapabilities & SAFplus::Group::ACCEPT_ACTIVE) != 0)
    {
      if(highestCredentials < curCredentials)
      {
        highestCredentials = curCredentials;
        leaderEntity = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      }
    }
  }
  return leaderEntity;

}
EntityIdentifier SAFplus::Group::electDeputy(EntityIdentifier highestCreEntity)
{
  uint highestCredentials = 0, curCredentials = 0;
  uint curCapabilities = 0;
  EntityIdentifier deputyEntity = INVALID_HDL, curEntity = INVALID_HDL;
  for (GroupHashMap::iterator i = map->begin();i != map->end();i++)
  {
    SAFplusI::BufferPtr curval = i->second;
    curCredentials = ((GroupIdentity *)(((Buffer *)curval.get())->data))->credentials;
    curCapabilities = ((GroupIdentity *)(((Buffer *)curval.get())->data))->capabilities;
    if((curCapabilities & SAFplus::Group::ACCEPT_STANDBY) != 0)
    {
      curEntity = ((GroupIdentity *)(((Buffer *)curval.get())->data))->id;
      if((highestCredentials < curCredentials) && !(curEntity == highestCreEntity))
      {
        highestCredentials = curCredentials;
        deputyEntity = curEntity;
      }
    }
  }
  return deputyEntity;
}

std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::electForRoles(int electionType)
{
  uint highestCredentials = 0,lowerCredentials = 0, curEntityCredentials = 0;
  uint curCapability;
  EntityIdentifier activeCandidate = INVALID_HDL, standbyCandidate = INVALID_HDL;

  if(electionType == SAFplus::Group::ELECTION_TYPE_BOTH)
  {
    activeCandidate = electLeader();
    if(activeCandidate == INVALID_HDL)
    {
      return std::pair<EntityIdentifier,EntityIdentifier>(INVALID_HDL,INVALID_HDL);
    }
    standbyCandidate = electDeputy(activeCandidate);
    return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);
  }
  else if(electionType == SAFplus::Group::ELECTION_TYPE_STANDBY)
  {
    activeCandidate = activeEntity;
    standbyCandidate = electDeputy(activeCandidate);
    return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);
  }
  else if(electionType == SAFplus::Group::ELECTION_TYPE_ACTIVE)
  {
    activeCandidate = electLeader();
    return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);

  }
}

int SAFplus::Group::elect(std::pair<EntityIdentifier,EntityIdentifier> &res, int electionType)
{
  bool isRoleChanged = false;
  std::pair<EntityIdentifier,EntityIdentifier> candidatePair;

  if(electionType == (int)SAFplus::Group::ELECTION_TYPE_BOTH)
  {
    candidatePair = electForRoles(SAFplus::Group::ELECTION_TYPE_BOTH);
      /* Update standby and active entity */
    if(!(candidatePair.first == INVALID_HDL) && !(candidatePair.first == activeEntity))
    {
        isRoleChanged = true;
        activeEntity = candidatePair.first;
    }
    if(!(candidatePair.second == INVALID_HDL) && !(candidatePair.second == standbyEntity))
    {
        isRoleChanged = true;
        standbyEntity = candidatePair.second;
    }
    res.first = activeEntity;
    res.second = standbyEntity;
    updateGroupRoles();
    if(isRoleChanged && wakeable)
    {
      wakeable->wake(3);
    }
    return 0;
  }
  else if(electionType == (int)SAFplus::Group::ELECTION_TYPE_STANDBY)
  {
    candidatePair = electForRoles(SAFplus::Group::ELECTION_TYPE_STANDBY);
    if(!(candidatePair.second == INVALID_HDL) && !(candidatePair.second == standbyEntity))
    {
      isRoleChanged = true;
      standbyEntity = candidatePair.second;
    }
    res.first = activeEntity;
    res.second = standbyEntity;
    updateGroupRoles();
    if(isRoleChanged && wakeable)
    {
      wakeable->wake(3);
    }
    return 1;
  }
  else if(electionType == (int)SAFplus::Group::ELECTION_TYPE_ACTIVE)
  {
    candidatePair = electForRoles(SAFplus::Group::ELECTION_TYPE_ACTIVE);
    if(!(candidatePair.first == INVALID_HDL) && !(candidatePair.first == activeEntity))
    {
        isRoleChanged = true;
        activeEntity = candidatePair.first;
    }
    res.first = activeEntity;
    res.second = standbyEntity;
    updateGroupRoles();
    if(isRoleChanged && wakeable)
    {
      wakeable->wake(3);
    }
    return 2;
  }
  res.first = INVALID_HDL;
  res.second = INVALID_HDL;
  return 0;
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
