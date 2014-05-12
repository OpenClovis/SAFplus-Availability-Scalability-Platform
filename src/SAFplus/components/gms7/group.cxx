/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clCommon.hxx>
#include <clGroup.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

extern SAFplus::NameRegistrar name;
/**
 * Static member
 */
bool  Group::isElectionRunning = false;
bool  Group::isElectionFinished = false;
bool  Group::isBootTimeElectionDone = false;
ClTimerHandleT  Group::electionRequestTHandle = NULL;
ClTimerHandleT  Group::roleWaitingTHandle = NULL;
SafplusMsgServer *Group::groupMsgServer = NULL;
SAFplus::Checkpoint Group::mGroupCkpt(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);
/**
 * API to create a group membership
 */
SAFplus::Group::Group(std::string handleName,int dataStoreMode)
{
  handle  = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  automaticElection    = false;
  minimumElectionTime  = 10;
  wakeable             = (SAFplus::Wakeable *)0;
  dataStoringMode      = dataStoreMode;
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
/**
 * API to create a group membership (defer initialization)
 */
SAFplus::Group::Group(int dataStoreMode)
{
  handle = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;
  automaticElection    = false;
  minimumElectionTime  = 10;
  dataStoringMode = dataStoreMode;
}
/**
 * API to initialize group service
 */
void SAFplus::Group::init(SAFplus::Handle groupHandle)
{
  /* Group maps to store associated data and entity information */
  mGroupMap = new SAFplus::GroupHashMap;
  groupDataMap = new SAFplus::GroupHashMap;
  /* Store current handle to name service */
  if(handle == INVALID_HDL)
  {
    handle = groupHandle;
    char hdlName[80];
    handle.toStr(hdlName);
    name.set(hdlName,handle,SAFplus::NameRegistrar::MODE_REDUNDANCY,(void*)this);
  }
  /* Initialize neccessary library */
  initializeLibraries();
  /* Start the message communication between groups */
  startMessageServer();
}
/**
 * Initialize necessary libraries
 */
/* FIXME: it should be users' responsible */
ClRcT SAFplus::Group::initializeLibraries()
{
  ClRcT   rc            = CL_OK;
  /* Initialize log service */
  logInitialize();

  /* Initialize necessary libraries */
  if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clBufferInitialize(NULL)) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK)
  {
    return rc;
  }
  clIocLibInitialize(NULL);
  return rc;
}
/**
 * Start the group message server, the main communication method for groups
 */
void SAFplus::Group::startMessageServer()
  {
  if (!Group::groupMsgServer)
    {
    Group::groupMsgServer = new SAFplus::SafplusMsgServer(CL_IOC_GMS_PORT);
    Group::groupMsgServer->Start();
    }

  GroupMessageHandler *groupMessageHandler = new GroupMessageHandler(this);
  Group::groupMsgServer->RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, NULL);

  }
/**
 * Do the real election after timer had expired
 */
ClRcT SAFplus::Group::electionRequest(void *arg)
{
  /* Clear timer and handle */
  clTimerStop(Group::electionRequestTHandle);
  clTimerDeleteAsync(&Group::electionRequestTHandle);
  try
  {
    /* Get and store election result */
    SAFplus::Handle hdl = *(SAFplus::Handle *)arg;
    Group *instance = (Group *)name.get(hdl);
    std::pair<EntityIdentifier,EntityIdentifier> res = instance->electForRoles(ELECTION_TYPE_BOTH);
    EntityIdentifier activeElected = res.first;
    EntityIdentifier standbyElected = res.second;
    instance->setActive(activeElected);
    instance->setStandby(standbyElected);
    /* Below should never happen, no role was elected */
    if(activeElected == INVALID_HDL && standbyElected == INVALID_HDL)
    {
      logError("GMS","ELECT","Election result isn't valid");
      assert(0);
    }
    /* Elected at least standby or active role */
    logInfo("GMS","ELECT","Success with active[%d] and standby[%d]",activeElected.getNode(),standbyElected.getNode());
    if(activeElected == instance->myInformation.id)
    {
      instance->fillSendMessage(&activeElected,GroupMessageTypeT::MSG_ROLE_NOTIFY,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_ACTIVE);
      instance->fillSendMessage(&standbyElected,GroupMessageTypeT::MSG_ROLE_NOTIFY,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_STANDBY);
      logInfo("GMS","ELECT","I, Node [%d] took active roles",instance->myInformation.id.getNode());
      isElectionFinished = true;
      if(instance->wakeable)
      {
        instance->wakeable->wake(ELECTION_FINISH_SIG);
      }
    }
    else
    {
       if(standbyElected == instance->myInformation.id)
       {
         logInfo("GMS","ELECT","I took standby roles");
       }
       else
       {
         logInfo("GMS","ELECT","I am normal member");
       }
      /* Wait 10 seconds to received role changed from active member */
        ClTimerTimeOutT timeOut = { 10, 0 };
        clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, Group::roleChangeRequest, (void* )arg, &Group::roleWaitingTHandle);
    }
    isElectionRunning = false;
    return CL_OK;
  }
  catch(std::exception &ex)
  {
    logError("GMS","...","Exception: %s",ex.what());
    return CL_ERR_INVALID_HANDLE;
  }

}
/**
 * Role change from master timeout
 */
ClRcT SAFplus::Group::roleChangeRequest(void *arg)
{
  /* No role change message from active, need to reelect */
  clTimerStop(Group::roleWaitingTHandle);
  clTimerDeleteAsync(&Group::roleWaitingTHandle);
  /* TODO: report fault */
  try
  {
    SAFplus::Handle hdl = *(SAFplus::Handle *)arg;
    Group *instance = (Group *)name.get(hdl);
    instance->elect();
  }
  catch (SAFplus::NameException& ex)
  {
    logError("GMS","...","Exception: %s",ex.what());
    return CL_ERR_INVALID_HANDLE;
  }

  return CL_OK;
}
/**
 * Node join message from message server
 */
void SAFplus::Group::nodeJoinHandle(SAFplusI::GroupMessageProtocol *rxMsg)
{
  GroupIdentity *rxGrp = (GroupIdentity *)rxMsg->data;
  registerEntity(*rxGrp,false);
}
/**
 * Node leave message from message server
 */
void SAFplus::Group::nodeLeaveHandle(SAFplusI::GroupMessageProtocol *rxMsg)
{
  EntityIdentifier *eId = (EntityIdentifier *)rxMsg->data;
  deregister(*eId,false);
}
/**
 * Role notification message from message server
 */
void SAFplus::Group::roleNotificationHandle(SAFplusI::GroupMessageProtocol *rxMsg)
{
  /* We actually having roles from local election, now...check whether it is consistent */
  switch(rxMsg->roleType)
  {
    case GroupRoleNotifyTypeT::ROLE_ACTIVE:
    {
      EntityIdentifier other = *(EntityIdentifier *)rxMsg->data;
      if(other != getActive() && !isElectionRunning)
      {
        logError("GMS","ROLENOTI","Mismatching active role [%d,%d]",getActive().getNode(),other.getNode());
        elect();
        return;
      }
      break;
    }
    case GroupRoleNotifyTypeT::ROLE_STANDBY:
    {
      EntityIdentifier other = *(EntityIdentifier *)rxMsg->data;
      if(other != getStandby() && !isElectionRunning)
      {
        logError("GMS","ROLENOTI","Mismatching standby role [%d,%d]",getStandby().getNode(),other.getNode());
        elect();
        return;
      }
      if(!isElectionRunning)
      {
        isElectionFinished = true;
        if(wakeable)
        {
          wakeable->wake(ELECTION_FINISH_SIG);
        }
      }
      break;
    }
    default:
    {
      break;
    }
  }
}
/**
 * Election request message
 */
void SAFplus::Group::electionRequestHandle(SAFplusI::GroupMessageProtocol *rxMsg)
{
  if(isElectionRunning == false)
  {
    isElectionRunning = true;
    if(myInformation.id != INVALID_HDL)
    {
      fillSendMessage(&myInformation,GroupMessageTypeT::MSG_NODE_JOIN,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
    }
    /* Wait for 10 seconds before doing election */
    ClTimerTimeOutT timeOut = { 10, 0 };
    clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, Group::electionRequest, (void* )&handle, &electionRequestTHandle);
  }
  if(rxMsg != NULL)
  {
    GroupIdentity *grp = (GroupIdentity *)rxMsg->data;
    registerEntity(*grp,false);
  }
}
/**
 * Fill information and call message server to send
 */
void SAFplus::Group::fillSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplusI::GroupMessageSendModeT msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType)
{
  int msgLen = 0;
  int msgDataLen = 0;
  switch(msgType)
  {
    case GroupMessageTypeT::MSG_NODE_JOIN:
    case GroupMessageTypeT::MSG_ELECT_REQUEST:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(GroupIdentity);
      msgDataLen = sizeof(GroupIdentity);
      break;
    case GroupMessageTypeT::MSG_NODE_LEAVE:
    case GroupMessageTypeT::MSG_ROLE_NOTIFY:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(EntityIdentifier);
      msgDataLen = sizeof(EntityIdentifier);
      break;
    default:
      return;
  }
  char msgPayload[sizeof(Buffer)-1+msgLen];
  Buffer* buff = new(msgPayload) Buffer(msgLen);
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)buff;
  sndMessage->messageType = msgType;
  sndMessage->roleType = roleType;
  memcpy(sndMessage->data,data,msgDataLen);
  sendNotification((void *)sndMessage,msgLen,msgSendMode);
}
/**
 * Actually send message
 */
void  SAFplus::Group::sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode)
{
  switch(messageMode)
  {
    case GroupMessageSendModeT::SEND_BROADCAST:
    {
      /* Destination is broadcast address */
      ClIocAddressT iocDest;
      iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
      iocDest.iocPhyAddress.portId      = CL_IOC_GMS_PORT;
      logInfo("GMS","SendMsg","Sending broadcast message");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
      }
      catch (...)
      {
        logDebug("GMS","SendMsg","Failed to send");
      }
      break;
    }
    case GroupMessageSendModeT::SEND_TO_MASTER:
    {
      /* Destination is Master node address */
      ClIocAddressT iocDest;
      ClIocNodeAddressT masterAddress = 0;
      clCpmMasterAddressGet(&masterAddress);
      iocDest.iocPhyAddress.nodeAddress = masterAddress;
      iocDest.iocPhyAddress.portId      = CL_IOC_GMS_PORT;
      logInfo("GMS","SendMsg","Sending message to Master");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, CL_IOC_PROTO_MSG);
      }
      catch (...)
      {
        logDebug("GMS","SendMsg","Failed to send");
      }
      break;
    }
    case GroupMessageSendModeT::SEND_LOCAL_RR:
    {
      logInfo("GMS","SendMsg","Sending message round robin");
      break;
    }
    default:
    {
      logError("GMS","SendMsg","Unknown message sending mode");
      break;
    }
  }
}
/**
 * API to register an entity to the group
 */
void SAFplus::Group::registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities,bool needNotify)
{
  assert(groupDataMap);

  /* Find existence of the entity */
  GroupHashMap::iterator contents = groupDataMap->find(me);

  /* The entity was not exits, insert  */
  if (contents == groupDataMap->end())
  {
    /* Keep track of last register entity */
    lastRegisteredEntity = me;

    /* Create key and val to store in database*/
    char *vkey = new char[sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1];
    char *vval = new char[sizeof(GroupIdentity) + sizeof(SAFplus::Buffer)-1];
    char *vdat = new char[dataLength + sizeof(SAFplus::Buffer)-1];

    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    SAFplus::Buffer* val = new(vval) Buffer(sizeof(GroupIdentity));
    SAFplus::Buffer* dat = new(vdat) Buffer(dataLength);

    GroupIdentity groupIdentity;
    groupIdentity.id = me;
    groupIdentity.credentials = credentials;
    groupIdentity.capabilities = capabilities;
    groupIdentity.dataLen = dataLength;
    memcpy(val->data,&groupIdentity,sizeof(GroupIdentity));
    memcpy(key->data,&me,sizeof(EntityIdentifier));
    writeToDatabase(key,val);
    memcpy(dat->data,data,dataLength);
    SAFplus::GroupMapPair vt(me,*dat);
    groupDataMap->insert(vt);
    /* Store my node information */
    if(me.getNode() == clIocLocalAddressGet())
    {
      myInformation = groupIdentity;
    }
    logDebug("GMS", "REG","Entity registration successful");
    /* Notify other entities about new entity*/
    if(wakeable)
    {
      wakeable->wake(NODE_JOIN_SIG);
    }

    if(needNotify == true)
    {
      fillSendMessage((void *)&groupIdentity,GroupMessageTypeT::MSG_NODE_JOIN,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
    }
    if(automaticElection)
    {
      logInfo("GMS","REG","Run election based on configuration");
      elect();
    }
  }
  else
  {
    /* Entity exist. Update its information */
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = me;

    Buffer &val = readFromDatabase(key);
    GroupIdentity *groupIdentity = (GroupIdentity *)&(val.data);
    uint oldLen = groupIdentity->dataLen;
    groupIdentity->credentials = credentials;
    groupIdentity->capabilities = capabilities;
    groupIdentity->dataLen = dataLength;
    writeToDatabase(key,&val);
    /* Update my node information */
    if(myInformation.id == me)
    {
      myInformation = *groupIdentity;
    }
    /* Update data associate with the entity */
    if(oldLen == dataLength)
    {
      Buffer &curData = contents->second;
      memcpy(curData.data,data,oldLen);
      SAFplus::GroupMapPair vt(me,curData);
      groupDataMap->insert(vt);
    }
    else
    {
      char vdat[dataLength + sizeof(SAFplus::Buffer)-1];
      SAFplus::Buffer* dat = new(vdat) Buffer(dataLength);
      memcpy(dat->data,data,dataLength);
      groupDataMap->erase(contents);
      SAFplus::GroupMapPair vt(me,*dat);
      groupDataMap->insert(vt);
    }
    logDebug("GMS", "REG","Entity exits. Updated its information");
  }
}
/**
 * API to register an entity to the group
 */
void SAFplus::Group::registerEntity(GroupIdentity grpIdentity, bool needNotify)
{
  registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities,needNotify);
}
/**
 * API to deregister an entity from the group
 */
void SAFplus::Group::deregister(EntityIdentifier me,bool needNotify)
{
  /* Use last registered entity if me is 0 */
  if(me == INVALID_HDL)
  {
    me = lastRegisteredEntity;
  }
  /* Find existence of the entity */
  GroupHashMap::iterator contents = groupDataMap->find(me);

  if(contents == groupDataMap->end())
  {
    logDebug("GMS", "DEREG","Entity was not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = me;

  removeFromDatabase(key);
  /* Delete associated data of entity */
  groupDataMap->erase(contents);
  /* Notify other entities about new entity*/
  if(wakeable)
  {
    wakeable->wake(NODE_LEAVE_SIG);
  }
  if(needNotify == true)
  {
    fillSendMessage((void *)&me,GroupMessageTypeT::MSG_NODE_LEAVE,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
  }
  if(myInformation.id == me) // I am leaving the cluster
  {
    return;
  }
  /* Update if leaving entity is standby/active */
  if(activeEntity == me)
  {
    activeEntity = standbyEntity;
    /* This should not happen */
    if(standbyEntity == INVALID_HDL && isBootTimeElectionDone == true)
    {
      /* TODO: report fault ?? */
    }
    /* Now, elect for the new standby */
    elect();
    return;
  }
  if(standbyEntity == me)
  {
    standbyEntity = INVALID_HDL;
    /* Now, elect for the new standby */
    elect();
    return;
  }
  if(automaticElection)
  {
    logInfo("GMS","REG","Run election based on configuration");
    elect();
  }
}
/**
 * API to set entity 's capabilities
 */
void SAFplus::Group::setCapabilities(uint capabilities, EntityIdentifier me)
{
  /* Use last registered entity if me is 0 */
  if(me == INVALID_HDL)
  {
    me = lastRegisteredEntity;
  }
  /* Find existence of the entity */
  GroupHashMap::iterator contents = groupDataMap->find(me);

  if(contents == groupDataMap->end())
  {
    logDebug("GMS", "SETCAP","Entity did not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = me;
  Buffer &curVal = readFromDatabase(key);
  GroupIdentity *grp = (GroupIdentity *)curVal.data;
  grp->capabilities = capabilities;
  /* Update my node information */
  if(myInformation.id == me)
  {
    myInformation = *grp;
  }
  writeToDatabase(key,&curVal);
}
/**
 * API to get entity 's capabilities
 */
uint SAFplus::Group::getCapabilities(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = groupDataMap->find(id);

  if(contents == groupDataMap->end())
  {
    logDebug("GMS", "GETCAP","Entity did not exist");
    return 0;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  *((EntityIdentifier *) key->data) = id;
  Buffer &curVal = readFromDatabase(key);
  GroupIdentity *grp = (GroupIdentity *)curVal.data;
  return grp->capabilities;
}
/**
 * API to get entity associated data
 */
SAFplus::Buffer& SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = groupDataMap->find(id);

  if(contents == groupDataMap->end())
  {
    logDebug("GMS", "GETDATA","Entity did not exist");
    return *((Buffer*) NULL);
  }
  return contents->second;
}
/**
 * Find the entity with highest credential and ACCEPT_ACTIVE capabilities
 */
EntityIdentifier SAFplus::Group::electLeader()
{
  uint highestCredentials = 0, curCredentials = 0;
  uint curCapabilities = 0;
  EntityIdentifier leaderEntity = INVALID_HDL;
  for (GroupHashMap::iterator i = groupDataMap->begin();i != groupDataMap->end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;
    Buffer &curVal = readFromDatabase(key);
    GroupIdentity *grp = (GroupIdentity *)curVal.data;

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
/**
 * Find the entity with second highest credential and ACCEPT_STANDBY capabilities
 * The highest credential is belong to active entity
 */
EntityIdentifier SAFplus::Group::electDeputy(EntityIdentifier highestCreEntity)
{
  uint highestCredentials = 0, curCredentials = 0;
  uint curCapabilities = 0;
  EntityIdentifier deputyEntity = INVALID_HDL, curEntity = INVALID_HDL;
  for (GroupHashMap::iterator i = groupDataMap->begin();i != groupDataMap->end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;
    Buffer &curVal = readFromDatabase(key);
    GroupIdentity *grp = (GroupIdentity *)curVal.data;
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
/**
 * Do election to find the required roles pairs <active,standby>
 */
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
/**
 * API allow member call for election based on group database
 */
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect()
{
  std::pair<EntityIdentifier,EntityIdentifier> res;
  res.first = INVALID_HDL;
  res.second = INVALID_HDL;
  if(isElectionRunning == true)
  {
    logError("GMS","ELECT","There is an current election running");
    return res;
  }
  electionRequestHandle(NULL);
  // Asynchronous call
  if(wakeable)
  {
    return res;
  }
  else
  {
    while(!isElectionFinished)
    {
      ;
    }
    res.first = getActive();
    res.second = getStandby();
    return res;
  }

}
/**
 * API to check whether an entity is a group member
 */
bool SAFplus::Group::isMember(EntityIdentifier id)
{
  GroupHashMap::iterator contents = groupDataMap->find(id);
  if(contents == groupDataMap->end())
  {
    return false;
  }
  else
  {
    return true;
  }
}
/**
 * API to set notification mechanism
 */
/* FIXME: not complete */
void SAFplus::Group::setNotification(SAFplus::Wakeable& w)
{
  wakeable = (Wakeable *)&w;
  logInfo("GMS", "SETNOTI","Notification had been set");
}
/**
 * API to get current active entity
 */
EntityIdentifier SAFplus::Group::getActive(void) const
{
  return activeEntity;
}
/**
 * API to set group active entity
 */
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
/**
 * API to set group standby entity
 */
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
/**
 * API to get current standby entity
 */
EntityIdentifier SAFplus::Group::getStandby(void) const
{
  return standbyEntity;
}
/**
 * Internal group function to read entity information from group database
 */
Buffer& SAFplus::Group::readFromDatabase(Buffer *key)
{
  switch(dataStoringMode)
  {
    case SAFplus::Group::DATA_IN_CHECKPOINT:
    {
      const Buffer& tmp = mGroupCkpt.read(*key);
      Buffer *res = (Buffer *)&tmp;
      return *res;
    }
    case SAFplus::Group::DATA_IN_MEMORY:
    {
      EntityIdentifier eiKey = *((EntityIdentifier *)key->data);
      GroupHashMap::iterator curItem = mGroupMap->find(eiKey);
      return curItem->second;
    }
    default:
    {
      return *((Buffer*)NULL);
    }
  }
}
/**
 * Internal group function to write entity information to group database
 */
void SAFplus::Group::writeToDatabase(Buffer *key, Buffer *val)
{
  switch(dataStoringMode)
  {
    case SAFplus::Group::DATA_IN_CHECKPOINT:
    {
      Transaction t;
      mGroupCkpt.write(*key,*val,t);
      break;
    }
    case SAFplus::Group::DATA_IN_MEMORY:
    {
      EntityIdentifier eiKey = *((EntityIdentifier *)key->data);
      GroupHashMap::iterator curItem = mGroupMap->find(eiKey);
      if(curItem != mGroupMap->end())
      {
        Buffer &curVal = readFromDatabase(key);
        GroupIdentity *grp = (GroupIdentity *)curVal.data;
        memcpy(curVal.data,val->data,sizeof(GroupIdentity));
        break;
      }
      GroupIdentity *grp = (GroupIdentity *)val->data;
      SAFplus::GroupMapPair vt(eiKey,*val);
      mGroupMap->insert(vt);
      break;
    }
    default:
    {
      logError("GMS","WRITE","Unknown storing mode");
      break;
    }
  }
}
/**
 * Internal group function to remove entity from group database
 */
void SAFplus::Group::removeFromDatabase(Buffer* key)
{
  switch(dataStoringMode)
  {
    case SAFplus::Group::DATA_IN_CHECKPOINT:
    {
      Transaction t;
      mGroupCkpt.remove(*key,t);
      break;
    }
    case SAFplus::Group::DATA_IN_MEMORY:
    {
      EntityIdentifier eiKey = *((EntityIdentifier *)key->data);
      GroupHashMap::iterator curItem = mGroupMap->find(eiKey);
      if(curItem != mGroupMap->end())
      {
        mGroupMap->erase(curItem);
      }
      break;
    }
    default:
    {
      break;
    }
  }
}
/**
 * Group iterator
 */
SAFplus::Group::Iterator SAFplus::Group::begin()
{
  SAFplus::Group::Iterator i(this);
  assert(this->groupDataMap);
  i.iter = this->groupDataMap->begin();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Group::Iterator SAFplus::Group::end()
{
  SAFplus::Group::Iterator i(this);
  assert(this->groupDataMap);
  i.iter = this->groupDataMap->end();
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
