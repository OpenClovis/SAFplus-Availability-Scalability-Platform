/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clCommon.hxx>
#include <clGroup.hxx>

using namespace boost::interprocess;
using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

extern SAFplus::NameRegistrar name;
Checkpoint Group::mCheckpoint(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);

SAFplus::Group::Group(std::string handleName)
{
  handle  = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;
  try
  {
    /* Get handle from name service */
    handle = name.getHandle(handleName);
    name.set(handleName,handle,SAFplus::NameRegistrar::MODE_REDUNDANCY,(void*)this);
  }
  catch (SAFplus::NameException& ex)
  {
    logDebug("GMS", "HDL","Can't get handler from give name %s",handleName.c_str());
    /* If handle did not exist, Create it */
    handle = SAFplus::Handle(PersistentHandle,0,getpid(),clIocLocalAddressGet());
    /* Store to name service */
    name.set(handleName,handle,SAFplus::NameRegistrar::MODE_REDUNDANCY,(void*)this);
  }
  init(handle);
}

SAFplus::Group::Group()
{
  handle = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;
}

void SAFplus::Group::init(SAFplus::Handle groupHandle)
{
  groupMap = new SAFplus::GroupHashMap;
}

void SAFplus::Group::registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities)
{
  assert(groupMap);
  /*Check in share memory if entity exists*/
  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = me;
  GroupHashMap::iterator contents = groupMap->find(me);

  /* The entity was not exits, insert */
  if (contents == groupMap->end())
  {
    /* Keep track of last register entity */
    lastRegisteredEntity = me;
    /* Allocate shared memory to store data*/
    char vval[sizeof(GroupIdentity) + sizeof(SAFplus::Buffer)-1];
    SAFplus::Buffer* val = new(vval) Buffer(sizeof(GroupIdentity));
    GroupIdentity groupIdentity;
    groupIdentity.id = me;
    groupIdentity.credentials = credentials;
    groupIdentity.capabilities = capabilities;
    groupIdentity.dataLen = dataLength;
    memcpy(val->data,&groupIdentity,sizeof(GroupIdentity));
    Transaction t;
    mCheckpoint.write(*key,*val,t);
    char vdat[dataLength + sizeof(SAFplus::Buffer)-1];
    SAFplus::Buffer* dat = new(vdat) Buffer(dataLength);
    memcpy(dat->data,data,dataLength);
    /* Store in the shared memory */
    SAFplus::GroupMapPair vt(me,*dat);
    groupMap->insert(vt);
    logDebug("GMS", "REG","Entity registration successful");
    /* Notify other entities about new entity*/
    if(wakeable)
    {
      wakeable->wake(1);
    }
  }
  else
  {
    uint oldLen = 0;
    Transaction t;
    logDebug("GMS", "REG","Entity exits. Update its information");
    const Buffer& val = mCheckpoint.read(*key);
    GroupIdentity *groupIdentity = (GroupIdentity *)val.data;
    oldLen = groupIdentity->dataLen;
    groupIdentity->credentials = credentials;
    groupIdentity->capabilities = capabilities;
    groupIdentity->dataLen = dataLength;
    mCheckpoint.write(*key,val,t);

    if(oldLen == dataLength)
    {
      Buffer &curData = contents->second;
      memcpy(curData.data,data,oldLen);
    }
    else
    {
      Buffer &oldData = contents->second;
      char vdat[dataLength + sizeof(SAFplus::Buffer)-1];
      SAFplus::Buffer* dat = new(vdat) Buffer(dataLength);
      memcpy(dat->data,data,dataLength);
      groupMap->erase(contents);
      SAFplus::GroupMapPair vt(me,*dat);
      groupMap->insert(vt);
      if(oldData.ref() == 1)
      {
        delete oldData;
      }
      else
      {
        oldData.decRef(1);
      }
    }
  }
}

void SAFplus::Group::registerEntity(GroupIdentity grpIdentity)
{
  registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities);
}

void SAFplus::Group::deregister(EntityIdentifier me)
{
  /* Use last registered entity if me is 0 */
  if(me == INVALID_HDL)
  {
    me = lastRegisteredEntity;
  }
  /*Check in share memory if entity exists*/
  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = me;
  GroupHashMap::iterator contents = groupMap->find(me);

  if(contents == groupMap->end())
  {
    logDebug("GMS", "DEREG","Entity was not exist");
    return;
  }
  Buffer &oldData = contents->second;
#if 0
  if(oldData.ref() == 1)
  {
    delete oldData;
  }
  else
  {
    oldData.decRef(1);
  }
#endif
  /* Remove from checkpoint */
  Transaction t;
  mCheckpoint.remove(*key,t);
  /* Delete data from map */
  groupMap->erase(contents);
  /* Update if leaving entity is standby/active */
  if(activeEntity == me)
  {
    activeEntity = INVALID_HDL;
  }
  if(standbyEntity == me)
  {
    standbyEntity = INVALID_HDL;
  }
  /* Notify other entities about new entity*/
  if(wakeable)
  {
    wakeable->wake(1);
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
  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = me;
  GroupHashMap::iterator contents = groupMap->find(me);

  if(contents == groupMap->end())
  {
    logDebug("GMS", "SETCAP","Entity did not exist");
    return;
  }

  const Buffer &curVal = mCheckpoint.read(*key);
  GroupIdentity *grp = (GroupIdentity *)curVal.data;
  grp->capabilities = capabilities;
  Transaction t;
  mCheckpoint.write(*key,curVal,t);
}

uint SAFplus::Group::getCapabilities(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = id;
  GroupHashMap::iterator contents = groupMap->find(id);

  if(contents == groupMap->end())
  {
    logDebug("GMS", "GETCAP","Entity did not exist");
    return 0;
  }
  const Buffer &curVal = mCheckpoint.read(*key);
  GroupIdentity *grp = (GroupIdentity *)curVal.data;
  return grp->capabilities;
}

SAFplus::Buffer& SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = groupMap->find(id);

  if(contents == groupMap->end())
  {
    logDebug("GMS", "GETDATA","Entity did not exist");
    return *((Buffer*) NULL);
  }
  return contents->second;
}

EntityIdentifier SAFplus::Group::electLeader()
{
  uint highestCredentials = 0, curCredentials = 0;
  uint curCapabilities = 0;
  EntityIdentifier leaderEntity = INVALID_HDL;
  for (GroupHashMap::iterator i = groupMap->begin();i != groupMap->end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;
    const Buffer &val = mCheckpoint.read(*key);
    GroupIdentity *grp = (GroupIdentity *)val.data;

    curCredentials = grp->credentials;
    curCapabilities = grp->capabilities;
    if((curCapabilities & SAFplus::Group::ACCEPT_ACTIVE) != 0)
    {
      if(highestCredentials < curCredentials)
      {
        highestCredentials = curCredentials;
        leaderEntity = grp->id;
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
  for (GroupHashMap::iterator i = groupMap->begin();i != groupMap->end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;

    const Buffer &val = mCheckpoint.read(*key);
    GroupIdentity *grp = (GroupIdentity *)val.data;
    curCredentials = grp->credentials;
    curCapabilities = grp->capabilities;
    if((curCapabilities & SAFplus::Group::ACCEPT_STANDBY) != 0)
    {
      curEntity = grp->id;
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
  else
  {
    return std::pair<EntityIdentifier,EntityIdentifier>(INVALID_HDL,INVALID_HDL);
  }
}

std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect(int electionType)
{
  int ret=0;
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
        logInfo("GMS", "ELECT","Active entity had been elected");
    }
    if(!(candidatePair.second == INVALID_HDL) && !(candidatePair.second == standbyEntity))
    {
        isRoleChanged = true;
        standbyEntity = candidatePair.second;
        logInfo("GMS", "ELECT","Standby entity had been elected");
    }
  }
  else if(electionType == (int)SAFplus::Group::ELECTION_TYPE_STANDBY)
  {
    candidatePair = electForRoles(SAFplus::Group::ELECTION_TYPE_STANDBY);
    if(!(candidatePair.second == INVALID_HDL) && !(candidatePair.second == standbyEntity))
    {
      isRoleChanged = true;
      standbyEntity = candidatePair.second;
      logInfo("GMS", "ELECT","Standby entity had been elected");
    }
  }
  else if(electionType == (int)SAFplus::Group::ELECTION_TYPE_ACTIVE)
  {
    candidatePair = electForRoles(SAFplus::Group::ELECTION_TYPE_ACTIVE);
    if(!(candidatePair.first == INVALID_HDL) && !(candidatePair.first == activeEntity))
    {
        isRoleChanged = true;
        activeEntity = candidatePair.second;
        logInfo("GMS", "ELECT","Active entity had been elected");
    }
  }
  else
  {
    logError("GMS","ELECT","Invalid election type");
    return std::pair<EntityIdentifier,EntityIdentifier>(INVALID_HDL,INVALID_HDL);
  }

  if(isRoleChanged)
  {
    if (wakeable)  wakeable->wake(3);  // Can't wake until this group has updated its internal state
  }
  return candidatePair;
}

bool SAFplus::Group::isMember(EntityIdentifier id)
{
  GroupHashMap::iterator contents = groupMap->find(id);
  if(contents == groupMap->end())
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
  logInfo("GMS", "SETNOTI","Notification had been set");
}

EntityIdentifier SAFplus::Group::getActive(void) const
{
  return activeEntity;
}

void SAFplus::Group::setActive(EntityIdentifier id)
{
  if(isMember(id))
  {
    activeEntity = id;
  }
  else
  {
    logDebug("GMS","SETACT","Node isn't a group member");
  }
}

void SAFplus::Group::setStandby(EntityIdentifier id)
{
  if(isMember(id))
  {
    standbyEntity = id;
  }
  else
  {
    logDebug("GMS","SETACT","Node isn't a group member");
  }
}

EntityIdentifier SAFplus::Group::getStandby(void) const
{
  return standbyEntity;
}

SAFplus::Group::Iterator SAFplus::Group::begin()
{
  SAFplus::Group::Iterator i(this);
  assert(this->groupMap);
  i.iter = this->groupMap->begin();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Group::Iterator SAFplus::Group::end()
{
  SAFplus::Group::Iterator i(this);
  assert(this->groupMap);
  i.iter = this->groupMap->end();
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

std::size_t SAFplus::hash_value(SAFplus::Handle const& h)
{
  boost::hash<uint64_t> hasher;
  return hasher(h.id[0]|h.id[1]);
}
