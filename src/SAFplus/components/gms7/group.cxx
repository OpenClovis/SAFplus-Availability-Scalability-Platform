/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clCommon.hxx>
#include <clGroup.hxx>
#include <clNameApi.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;


/**
 * Static member
 */
bool  Group::isElectionRunning = false;
bool  Group::isElectionFinished = false;
bool  Group::isBootTimeElectionDone = false;
ClTimerHandleT  Group::electionRequestTHandle = NULL;
ClTimerHandleT  Group::roleWaitingTHandle = NULL;
SAFplus::Checkpoint Group::mGroupCkpt(Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);
/**
 * API to create a group membership
 */
SAFplus::Group::Group(std::string handleName,int dataStoreMode, int comPort)
{
  handle  = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  automaticElection    = false;
  minimumElectionTime  = 10;
  wakeable             = (SAFplus::Wakeable *)0;
  dataStoringMode      = dataStoreMode;
  groupMsgServer       = NULL;
  groupCommunicationPort  = comPort;
  needReElect          = false;
  /* Try to register with name service so that callback function can use this group*/
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
SAFplus::Group::Group(int dataStoreMode,int comPort)
{
  handle = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;
  automaticElection    = false;
  minimumElectionTime  = 10;
  dataStoringMode = dataStoreMode;
  groupMsgServer  = NULL;
  groupCommunicationPort = comPort;
  needReElect          = false;
}
/**
 * API to initialize group service
 */
void SAFplus::Group::init(SAFplus::Handle groupHandle)
{
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
  if (!groupMsgServer)
  {
    groupMsgServer = new SAFplus::SafplusMsgServer(groupCommunicationPort);
    groupMsgServer->Start();
  }
  GroupMessageHandler *groupMessageHandler = new GroupMessageHandler(this);
  groupMsgServer->RegisterHandler(CL_IOC_PROTO_MSG, groupMessageHandler, NULL);

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
    /* Get current group handle from name service */
    SAFplus::Handle hdl = *(SAFplus::Handle *)arg;
    Group *instance = (Group *)name.get(hdl);
    std::pair<EntityIdentifier,EntityIdentifier> res = instance->electForRoles(ELECTION_TYPE_BOTH);
    EntityIdentifier activeElected = res.first;
    EntityIdentifier standbyElected = res.second;
    /* Below should never happen, we can't elect a standby with no active */
    if (standbyElected != INVALID_HDL)
    {
      assert(activeElected != INVALID_HDL);
    }
    /* Below can happen if no entity is electable in the group (only watchers) */
    if(activeElected == INVALID_HDL && standbyElected == INVALID_HDL)
    {
      logWarning("GMS","ELECT","Election did not succeed -- no entity can assume a role");
    }
    else
    {
    /* Elected at least standby or active role */
    logInfo("GMS","ELECT","Success with active[%d] and standby[%d]",activeElected.getNode(),standbyElected.getNode());
    }

    /* If I am active member, send notification */
    if(activeElected == instance->myInformation.id)
    {
      instance->setActive(activeElected);
      instance->setStandby(standbyElected);
      instance->fillSendMessage(&activeElected,GroupMessageTypeT::MSG_ROLE_NOTIFY,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_ACTIVE);
      instance->fillSendMessage(&standbyElected,GroupMessageTypeT::MSG_ROLE_NOTIFY,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_STANDBY);
      logInfo("GMS","ELECT","I, Node [%d] took active roles",instance->myInformation.id.getNode());
      isElectionFinished = true;
      if(instance->wakeable)
      {
        instance->wakeable->wake(ELECTION_FINISH_SIG);
      }
      isElectionRunning = false;
    }
    else /*other, wait for notification */
    {
       if(standbyElected == instance->myInformation.id)
       {
         logInfo("GMS","ELECT","I took standby roles");
       }
       else
       {
         logInfo("GMS","ELECT","I am normal member");
       }
       /* If role change message come before I am finished election */
       if(instance->getActive() != activeElected || instance->getStandby() != standbyElected)
       {
         instance->setActive(activeElected);
         instance->setStandby(standbyElected);
         /* Wait 10 seconds to received role changed from active member */
         ClTimerTimeOutT timeOut = { 10, 0 };
         clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, Group::roleChangeRequest, (void* )arg, &Group::roleWaitingTHandle);
       }
       else
       {
         isElectionRunning = false;
         isElectionFinished = true;
         if(instance->wakeable)
         {
           instance->wakeable->wake(ELECTION_FINISH_SIG);
         }
       }
    }
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
  if(isElectionFinished)
  {
    clTimerStop(Group::roleWaitingTHandle);
    clTimerDeleteAsync(&Group::roleWaitingTHandle);
    isElectionRunning = false;
    return CL_OK;
  }
  logError("GMS","ROLE","No role changed message from master. Re-elect");
  /* No role change message from active, need to reelect */
  clTimerStop(Group::roleWaitingTHandle);
  clTimerDeleteAsync(&Group::roleWaitingTHandle);
  isElectionRunning = false;
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
      if(other != getActive())
      {
        logError("GMS","ROLENOTI","Mismatching active role [%d,%d]",getActive().getNode(),other.getNode());
        needReElect          = true;
      }
      break;
    }
    case GroupRoleNotifyTypeT::ROLE_STANDBY:
    {
      EntityIdentifier other = *(EntityIdentifier *)rxMsg->data;
      if(other != getStandby())
      {
        logError("GMS","ROLENOTI","Mismatching standby role [%d,%d]",getStandby().getNode(),other.getNode());
        needReElect          = true;
      }
      clTimerStop(Group::roleWaitingTHandle);
      clTimerDeleteAsync(&Group::roleWaitingTHandle);
      isElectionRunning = false;
      if(needReElect)
      {
        needReElect          = false;
        elect();
        return;
      }
      isElectionFinished = true;
      if(wakeable)
      {
        wakeable->wake(ELECTION_FINISH_SIG);
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
      fillSendMessage(&myInformation,GroupMessageTypeT::MSG_ELECT_REQUEST,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
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
      iocDest.iocPhyAddress.portId      = groupCommunicationPort;
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
      iocDest.iocPhyAddress.portId      = groupCommunicationPort;
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
  /* Find existence of the entity */
  GroupHashMap::iterator contents = groupDataMap.find(me);

  /* The entity was not exits, insert  */
  if (contents == groupDataMap.end())
  {
    /* Keep track of last register entity */
    lastRegisteredEntity = me;

    /* Create key and val to store in database*/
    char vkey[sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    memcpy(key->data,&me,sizeof(EntityIdentifier));

    GroupIdentity *groupIdentity = new GroupIdentity();
    groupIdentity->id = me;
    groupIdentity->credentials = credentials;
    groupIdentity->capabilities = capabilities;
    groupIdentity->dataLen = dataLength;
    writeToDatabase(key,(void *)groupIdentity);

    /* Store associated data */
    SAFplus::GroupMapPair vt(me,const_cast < void * > (data));
    groupDataMap.insert(vt);

    /* Store my node information */
    if(me.getNode() == clIocLocalAddressGet())
    {
      myInformation = *groupIdentity;
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
    memcpy(key->data,&me,sizeof(EntityIdentifier));

    GroupIdentity *groupIdentity = (GroupIdentity *)readFromDatabase(key);
    if(!groupIdentity)
    {
      groupIdentity = new GroupIdentity();
    }
    groupIdentity->credentials = credentials;
    groupIdentity->capabilities = capabilities;
    groupIdentity->dataLen = dataLength;
    writeToDatabase(key,(void *)groupIdentity);

    /* Update my node information */
    if(myInformation.id == me)
    {
      myInformation = *groupIdentity;
    }

    /* Update data associate with the entity */
    /* User should take responsibile for freeing un-used buffer here */
    groupDataMap.erase(contents);
    SAFplus::GroupMapPair vt(me,const_cast < void * > (data));
    groupDataMap.insert(vt);
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
  GroupHashMap::iterator contents = groupDataMap.find(me);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "DEREG","Entity was not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&me,sizeof(EntityIdentifier));

  removeFromDatabase(key);
  /* Delete associated data of entity */
  groupDataMap.erase(contents);
  /* User should remove unused associated data */
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
  if(activeEntity == me && (myInformation.id == standbyEntity || standbyEntity == INVALID_HDL))
  {
    logDebug("GMS","DEREG","Leaving node had active role. Re-elect");
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
  if(standbyEntity == me && myInformation.id == activeEntity)
  {
    logDebug("GMS","DEREG","Leaving node had standby role. Re-elect");
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
  GroupHashMap::iterator contents = groupDataMap.find(me);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "SETCAP","Entity did not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&me,sizeof(EntityIdentifier));

  GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
  grp->capabilities = capabilities;

  /* Update my node information */
  if(myInformation.id == me)
  {
    myInformation = *grp;
  }
  writeToDatabase(key,(void *)grp);
}
/**
 * API to get entity 's capabilities
 */
uint SAFplus::Group::getCapabilities(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = groupDataMap.find(id);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "GETCAP","Entity did not exist");
    return 0;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&id,sizeof(EntityIdentifier));

  GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
  return grp->capabilities;
}
/**
 * API to get entity associated data
 */
void* SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  GroupHashMap::iterator contents = groupDataMap.find(id);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "GETDATA","Entity did not exist");
    return NULL;
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
  for (GroupHashMap::iterator i = groupDataMap.begin();i != groupDataMap.end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;

    GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
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
  for (GroupHashMap::iterator i = groupDataMap.begin();i != groupDataMap.end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;

    GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
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
  isElectionFinished = false;
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
  GroupHashMap::iterator contents = groupDataMap.find(id);
  if(contents == groupDataMap.end())
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
  activeEntity = id;
}
/**
 * API to set group standby entity
 */
void SAFplus::Group::setStandby(EntityIdentifier id)
{
  standbyEntity = id;
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
void* SAFplus::Group::readFromDatabase(Buffer *key)
{
  switch(dataStoringMode)
  {
    case SAFplus::Group::DATA_IN_CHECKPOINT:
    {
      const Buffer& tmp = mGroupCkpt.read(*key);
      return (void *)tmp.data;
    }
    case SAFplus::Group::DATA_IN_MEMORY:
    {
      EntityIdentifier eiKey = *((EntityIdentifier *)key->data);
      GroupHashMap::iterator curItem = mGroupMap.find(eiKey);
      return curItem->second;
    }
    default:
    {
      return NULL;
    }
  }
}
/**
 * Internal group function to write entity information to group database
 */
void SAFplus::Group::writeToDatabase(Buffer *key, void *val)
{
  switch(dataStoringMode)
  {
    case SAFplus::Group::DATA_IN_CHECKPOINT:
    {
      Transaction t;
      char vbuf[sizeof(SAFplus::Buffer) - 1 + sizeof(GroupIdentity)];
      Buffer *buf = new(vbuf) Buffer(sizeof(GroupIdentity));
      memcpy(buf->data,val,sizeof(GroupIdentity));
      mGroupCkpt.write(*key,*buf,t);
      break;
    }
    case SAFplus::Group::DATA_IN_MEMORY:
    {
      EntityIdentifier eiKey = *((EntityIdentifier *)key->data);
      GroupHashMap::iterator curItem = mGroupMap.find(eiKey);
      if(curItem != mGroupMap.end())
      {
        mGroupMap.erase(curItem);
      }
      SAFplus::GroupMapPair vt(eiKey,val);
      mGroupMap.insert(vt);
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
      GroupHashMap::iterator curItem = mGroupMap.find(eiKey);
      if(curItem != mGroupMap.end())
      {
        GroupIdentity *old = (GroupIdentity *)curItem->second;
        if(old)
        {
          delete old;
          old = 0;
        }
        mGroupMap.erase(curItem);
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
  i.iter = this->groupDataMap.begin();
  i.curval = &(*i.iter);
  return i;
}

SAFplus::Group::Iterator SAFplus::Group::end()
{
  SAFplus::Group::Iterator i(this);
  i.iter = this->groupDataMap.end();
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
