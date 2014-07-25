/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clCommon.hxx>
#include <clGroup.hxx>
#include <clNameApi.hxx>
#include <clIocPortList.hxx>
#include <clGroup.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;
using namespace boost::intrusive;
//Define a list that will store MyClass using the public member hook
typedef boost::intrusive::list< Group, member_hook< Group, list_member_hook<>, &Group::member_hook_> > GroupList;

GroupList allGroups;
/**
 * Static member
 */
//bool  Group::isElectionRunning = false;
//bool  Group::isElectionFinished = false;
bool  Group::isBootTimeElectionDone = false;
//ClTimerHandleT  Group::electionRequestTHandle = NULL;
//ClTimerHandleT  Group::roleWaitingTHandle = NULL;
SAFplus::Checkpoint Group::mGroupCkpt;
bool groupCkptInited = false;


typedef boost::unordered_map<SAFplus::Handle, SAFplus::Group::GroupMessageHandler*> GroupHandleMap;
class GroupSyncMsgHandler: public MsgHandler
{
  public:
  // checkpoint handle to pointer lookup...
  GroupHandleMap handleMap;

  virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
};
static GroupSyncMsgHandler allGrpMsgHdlr;

/**
 * API to create a group membership
 */
SAFplus::Group::Group(const std::string& handleName,int dataStoreMode, int comPort):groupMessageHandler()
{
  roleWaitingTHandle = NULL;
  electionRequestTHandle = NULL;
  isElectionRunning = false;
  isElectionFinished = false;

  handle  = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  automaticElection    = true;
  //minimumElectionTime  = 10;
  electionTimeMs       = 5000;
  wakeable             = (SAFplus::Wakeable *)0;
  dataStoringMode      = dataStoreMode;
  groupMsgServer       = NULL;
  groupCommunicationPort  = comPort;
  needReElect          = false;
  stickyMode           = true;
  /* Try to register with name service so that callback function can use this group*/
  try
  {
    /* Get handle from name service */
    handle = name.getHandle(handleName);
  }
  catch (SAFplus::NameException& ex)
  {
    logDebug("GMS", "HDL","Can't get handler from give name %s",handleName.c_str());
    /* If handle did not exist, Create it */
    handle = SAFplus::Handle::create();
    /* Store to name service. inside the "catch" because the name
     * already exists if the try succeeded. */
    name.set(handleName,handle,SAFplus::NameRegistrar::MODE_REDUNDANCY);
  }

  name.setLocalObject(handleName,(void*)this);
  init(handle,dataStoringMode, groupCommunicationPort);
}
/**
 * API to create a group membership (defer initialization)
 */
SAFplus::Group::Group(int dataStoreMode,int comPort):groupMessageHandler()
{
  roleWaitingTHandle = NULL;
  electionRequestTHandle = NULL;
  isElectionRunning = false;
  isElectionFinished = false;

  handle = INVALID_HDL;
  activeEntity = INVALID_HDL;
  standbyEntity = INVALID_HDL;
  lastRegisteredEntity = INVALID_HDL;
  wakeable             = (SAFplus::Wakeable *)0;
  automaticElection    = true;
  //minimumElectionTime  = 10;
  electionTimeMs       = 5000;
  dataStoringMode = dataStoreMode;
  groupMsgServer  = NULL;
  groupCommunicationPort = comPort;
  needReElect          = false;
  stickyMode           = true;
}

SAFplus::Group::~Group()
{
  allGroups.remove(*this);
  allGrpMsgHdlr.handleMap[handle] = NULL;
}

/**
 * API to initialize group service
 */
void SAFplus::Group::init(SAFplus::Handle groupHandle,int dataStoreMode, int comPort)
{
  logInfo("GRP","INI","Opening group [%lx:%lx]",groupHandle.id[0],groupHandle.id[1]);

  dataStoringMode      = dataStoreMode;
  groupCommunicationPort = comPort;

  if ((dataStoringMode == SAFplus::Group::DATA_IN_CHECKPOINT)&&(!groupCkptInited))
  {
    groupCkptInited=1;
    Group::mGroupCkpt.init(GRP_CKPT, Checkpoint::REPLICATED|Checkpoint::SHARED, CkptDefaultSize, CkptDefaultRows);
  }

  /* Store current handle to name service */
  if(handle == INVALID_HDL)
  {
    handle = groupHandle;
    char hdlName[80];
    handle.toStr(hdlName);
    name.set(hdlName,handle,SAFplus::NameRegistrar::MODE_REDUNDANCY);
    name.setLocalObject(hdlName, (void*) this);
  }
  /* Initialize neccessary library */
  //initializeLibraries();
  /* Start the message communication between groups */
  startMessageServer();
  allGroups.push_back(*this);
}
/**
 * IOC notification to detect node leave
 */
ClRcT SAFplus::Group::iocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie)
{
  ClRcT rc = CL_OK;
  ClIocAddressT address;
  ClIocNotificationIdT eventId = (ClIocNotificationIdT) ntohl(notification->id);
  ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
  ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
  switch(eventId)
  {
    case CL_IOC_NODE_LEAVE_NOTIFICATION:
    case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
    {
      logDebug("GMS","IOC","Received node leave notification for node [%d]", nodeAddress);
      GroupList::iterator git;
      for (git=allGroups.begin(); git!=allGroups.end(); git++)
        {
        logDebug("GMS","IOC","  Checking group [%lx:%lx]", (*git).myInformation.id.id[0],(*git).myInformation.id.id[1]);
        try
          {
          Group *instance = &(*git);
          for (SAFplus::DataHashMap::iterator i = instance->groupDataMap.begin();i != instance->groupDataMap.end();i++)
            {
            EntityIdentifier curIter = i->first;
            if(curIter.getNode() == nodeAddress)
              {
              logDebug("GMS","IOC","    Removing Entity [%lx:%lx]", curIter.id[0],curIter.id[1]);
              instance->deregister(curIter,false);
              }
            }
          }
        catch(std::exception &ex)
          {
          logError("GMS","---","Exception: %s",ex.what());
          }
        }
      break;
    }
    default:
    {
      logInfo("GMS","IOC","Received event [%d] from IOC notification",eventId);
      break;
    }
  }
  return rc;
}

void SAFplus::groupInitialize()
  {
  clIocNotificationRegister(Group::iocNotificationCallback,NULL);
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


// Demultiplex incoming message to the appropriate Checkpoint object
void GroupSyncMsgHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  //logInfo("SYNC","MSG","Received group message from %d", from.iocPhyAddress.nodeAddress);
  GroupMessageProtocol* hdr = (GroupMessageProtocol*) msg;
  //assert((hdr->msgType>>16) == SAFplusI::GRP_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  SAFplus::Group::GroupMessageHandler* cs =  allGrpMsgHdlr.handleMap[hdr->group];
  if (cs) cs->msgHandler(from, svr, msg, msglen, cookie);
  else
    {
    logInfo("SYNC","MSG","Unable to resolve handle [%lx:%lx] to a communications endpoint", hdr->group.id[0], hdr->group.id[1]);
    }
  }


/**
 * Start the group message server, the main communication method for groups
 */
void SAFplus::Group::startMessageServer()
{
  if(!groupMsgServer)
  {
    groupMsgServer = &safplusMsgServer;
  }

  groupMessageHandler.init(this);  //  Initialize this object's message handler.
  allGrpMsgHdlr.handleMap[handle] = &groupMessageHandler;  //  Register this object's message handler into the global lookup.
  groupMsgServer->RegisterHandler(SAFplusI::GRP_MSG_TYPE, &allGrpMsgHdlr, NULL);  //  Register the main message handler (no-op if already registered)
}


ClRcT electionRequestDispatch(void *arg)
{
  Group* g = (Group*) arg;
  return g->electionRequest();
}

/**
 * Role change from master timeout
 */
ClRcT roleChangeRequestDispatch(void *arg)
{
  Group *instance = (Group *)arg;
  return instance->roleChangeRequest();
}

/**
 * Do the real election after timer had expired
 */
ClRcT SAFplus::Group::electionRequest()
{
  /* Clear timer and handle */
  clTimerStop(electionRequestTHandle);
  clTimerDeleteAsync(&electionRequestTHandle);
  try
  {
    std::pair<EntityIdentifier,EntityIdentifier> res = electForRoles();
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
      logWarning("GMS","ELE","Election did not succeed -- no entity can assume a role");
    }
    else
    {
    /* Elected at least standby or active role */
    logInfo("GMS","ELE","Success with active [%d] and standby [%d]",activeElected.getNode(),standbyElected.getNode());
    }

    /* If I want to make myself active member, send notification */
    if(activeElected == myInformation.id)
    {
      assert(activeElected != standbyElected);
      sendRoleMessage(activeElected,standbyElected,false);
      logInfo("GMS","ELE","I, Node [%d] am claiming active",myInformation.id.getNode());
    }
    /* Note that I don't set myself to active here even though I elected
     * myself.  The reason is that I want to hear my own message to:
     * 1. make sure that the message was sent across the cluster, and
     * 2. synchronize (as much as possible) my becoming active clusterwide in one atomic packet.
     */

    /* If we don't receive the ROLE announcement withing this time,
     * restart the election. */
    ClTimerTimeOutT timeOut = { 0, electionTimeMs/2 };
    clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, roleChangeRequestDispatch, (void*) this, &roleWaitingTHandle);

    return CL_OK;
  }
  catch(std::exception &ex)
  {
    logError("GMS","---","Exception: %s",ex.what());
    return CL_ERR_INVALID_HANDLE;
  }

}


ClRcT SAFplus::Group::roleChangeRequest()
{
  if (roleWaitingTHandle)
    {
  clTimerStop(roleWaitingTHandle);
  clTimerDeleteAsync(&roleWaitingTHandle);
  roleWaitingTHandle = 0;
    };

  if(isElectionFinished)
  {
    isElectionRunning = false;
    return CL_OK;
  }
  logError("GMS","ROL","No role changed message from master. Re-elect");
  /* No role change message from active, need to reelect */
  isElectionRunning = false;
  /* TODO: report fault */
  try
  {
    elect();
  }
  catch (SAFplus::NameException& ex)
  {
    logError("GMS","---","Exception: %s",ex.what());
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
      EntityIdentifier *announce = (EntityIdentifier *)rxMsg->data;

      if(!rxMsg->force)  // If we do not want to force, then check if our calculations match the claimant.  If they do not match, we need to re-elect.
      {
        std::pair<EntityIdentifier,EntityIdentifier> roles = electForRoles();
        if (announce[0] != roles.first)
          {
          logError("GMS","ROL","Mismatched active role.  I calculated [%lx:%lx] but received [%lx:%lx]",roles.first.id[0],roles.first.id[1],announce[0].id[0],announce[0].id[1]);
          needReElect          = true;
          }
      }

      // But accept the announcement regardless of my election results because the cluster must remain consistent
      if(1) //(rxMsg->force)
      {
        isElectionFinished = true;
        if (activeEntity != announce[0])  // active entity has changed
          {
          if (activeEntity != INVALID_HDL)
            removeCapabilities(activeEntity,SAFplus::Group::IS_ACTIVE);
          activeEntity = announce[0];
          if (announce[0] != INVALID_HDL)
            {
            removeCapabilities(activeEntity,SAFplus::Group::IS_STANDBY);
            addCapabilities(announce[0],SAFplus::Group::IS_ACTIVE);
            }
          }

        if (standbyEntity != announce[1])  // active entity has changed
          {
          if (standbyEntity != INVALID_HDL)
            removeCapabilities(standbyEntity,SAFplus::Group::IS_STANDBY);
          standbyEntity = announce[1];
          if (announce[1] != INVALID_HDL)
            {
            removeCapabilities(announce[1],SAFplus::Group::IS_ACTIVE);  // could be the case in a double active situation...
            addCapabilities(announce[1],SAFplus::Group::IS_STANDBY);
            }
          }

        logInfo("GMS","ROL","Role change announcement active [%lx:%lx] standby [%lx:%lx]", activeEntity.id[0],activeEntity.id[1],standbyEntity.id[0],standbyEntity.id[1]);

          isElectionRunning = false;
        /* If some one are waiting the election, waking up now */
        if(wakeable)
        {
          wakeable->wake(1,(void*)ELECTION_FINISH_SIG);
        }
      }

      if (roleWaitingTHandle)
        {
      clTimerStop(roleWaitingTHandle);
      clTimerDeleteAsync(&roleWaitingTHandle);
      roleWaitingTHandle = 0;
        }

      if(needReElect)
      {
        assert(0);
        needReElect          = false;
        elect();
      }
      break;
    }


    case GroupRoleNotifyTypeT::ROLE_STANDBY:
    {
      assert(0);
      EntityIdentifier other = *(EntityIdentifier *)rxMsg->data;
      if(other != standbyEntity)
      {
        logError("GMS","ROL","Mismatching standby role [%d,%d]",standbyEntity.getNode(),other.getNode());
        needReElect          = true;
      }
  if (roleWaitingTHandle)
    {
      clTimerStop(roleWaitingTHandle);
      clTimerDeleteAsync(&roleWaitingTHandle);
      roleWaitingTHandle = 0;
    }
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
        wakeable->wake(1,(void*)ELECTION_FINISH_SIG);
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
  if(myInformation.id != INVALID_HDL)
    {
      fillAndSendMessage(&myInformation,GroupMessageTypeT::MSG_HELLO,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
    }

  if(isElectionRunning == false)
  {
    isElectionRunning = true;
    ClTimerTimeOutT timeOut = { 0, electionTimeMs };
    clTimerCreateAndStart( timeOut, CL_TIMER_ONE_SHOT, CL_TIMER_TASK_CONTEXT, electionRequestDispatch, (void* ) this, &electionRequestTHandle);
  }
  if(rxMsg != NULL) // Register the information provided by the node that requested election
  {
    GroupIdentity *grp = (GroupIdentity *)rxMsg->data;
    registerEntity(*grp,false);
  }
}

void SAFplus::Group::sendRoleMessage(EntityIdentifier active,EntityIdentifier standby,bool forcing)
{
  EntityIdentifier data[2] = { active, standby };
  int msgLen = 0;
  int msgDataLen = 0;

  msgDataLen = sizeof(EntityIdentifier)*2;
  msgLen = sizeof(GroupMessageProtocol) + msgDataLen;

  char msgPayload[sizeof(Buffer)-1+msgLen];
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;
  sndMessage->group = handle;
  sndMessage->messageType = GroupMessageTypeT::MSG_ROLE_NOTIFY;
  sndMessage->roleType    = GroupRoleNotifyTypeT::ROLE_ACTIVE;
  sndMessage->force = forcing;
  memcpy(sndMessage->data,data,msgDataLen);
  sendNotification((void *)sndMessage,msgLen,GroupMessageSendModeT::SEND_BROADCAST);
}

/**
 * Fill information and call message server to send
 */
void SAFplus::Group::fillAndSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplusI::GroupMessageSendModeT msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType,bool forcing)
{
  int msgLen = 0;
  int msgDataLen = 0;
  switch(msgType)
  {
    case GroupMessageTypeT::MSG_NODE_JOIN:
    case GroupMessageTypeT::MSG_ELECT_REQUEST:
    case GroupMessageTypeT::MSG_HELLO:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(GroupIdentity);
      msgDataLen = sizeof(GroupIdentity);
      break;
    case GroupMessageTypeT::MSG_NODE_LEAVE:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(EntityIdentifier);
      msgDataLen = sizeof(EntityIdentifier);
      break;
    case GroupMessageTypeT::MSG_ROLE_NOTIFY:
      msgLen = sizeof(GroupMessageProtocol) + sizeof(EntityIdentifier)*2;
      msgDataLen = sizeof(EntityIdentifier);
      break;
    default:
      return;
  }
  char msgPayload[sizeof(Buffer)-1+msgLen];
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;
  sndMessage->group = handle;
  sndMessage->messageType = msgType;
  sndMessage->roleType = roleType;
  sndMessage->force = forcing;
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
      //logInfo("GMS","MSG","Sending broadcast message");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, SAFplusI::GRP_MSG_TYPE);
      }
      catch (...)
      {
        logDebug("GMS","MSG","Failed to send");
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
      //logInfo("GMS","MSG","Sending message to Master");
      try
      {
        groupMsgServer->SendMsg(iocDest, (void *)data, dataLength, SAFplusI::GRP_MSG_TYPE);
      }
      catch (...)
      {
        logDebug("GMS","MSG","Failed to send");
      }
      break;
    }
    case GroupMessageSendModeT::SEND_LOCAL_RR:
    {
      logInfo("GMS","MSG","Sending message round robin");
      break;
    }
    default:
    {
      logError("GMS","MSG","Unknown message sending mode");
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
  DataHashMap::iterator contents = groupDataMap.find(me);

  /* The entity was not exits, insert  */
  if (contents == groupDataMap.end())
  {
    /* Keep track of last register entity */
    lastRegisteredEntity = me;

    /* Create key and val to store in database*/
    char vkey[sizeof(EntityIdentifier) + sizeof(SAFplus::Buffer)-1];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    memcpy(key->data,&me,sizeof(EntityIdentifier));
    GroupIdentity groupIdentity;
    groupIdentity.id = me;
    groupIdentity.credentials = credentials;
    groupIdentity.capabilities = capabilities;
    groupIdentity.dataLen = dataLength;
    writeToDatabase(key,(void *)&groupIdentity);
    /* Store associated data */
    SAFplus::DataMapPair vt(me,const_cast < void * > (data));
    groupDataMap.insert(vt);

    /* Store my node information */
    if(me.getNode() == clIocLocalAddressGet())
    {
      myInformation = groupIdentity;
    }
    logDebug("GMS", "REG","Entity registration successful");
    /* Notify other entities about new entity*/
    if(wakeable)
    {
      wakeable->wake(1,(void*)NODE_JOIN_SIG);
    }

    if(needNotify == true)
    {
      fillAndSendMessage((void *)&groupIdentity,GroupMessageTypeT::MSG_NODE_JOIN,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
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
      /* Found entity but can't read its information. Memory corruption occured */
      logError("GMS","REG","Invalid entity information");
      assert(0);
    }
    /* Update entity information */
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
    SAFplus::DataMapPair vt(me,const_cast < void * > (data));
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
  DataHashMap::iterator contents = groupDataMap.find(me);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "DER","Entity was not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&me,sizeof(EntityIdentifier));

  removeFromDatabase(key);
  /* Delete associated data of entity */
  groupDataMap.erase(contents);
  /* User should remove unused associated data */

  if(needNotify == true)
  {
    fillAndSendMessage((void *)&me,GroupMessageTypeT::MSG_NODE_LEAVE,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
  }
  if(myInformation.id == me) // I am leaving the cluster
  {
    return;
  }
  /* Update if leaving entity is standby/active */
  if(activeEntity == me && (myInformation.id == standbyEntity || standbyEntity == INVALID_HDL))
  {
    if (standbyEntity == INVALID_HDL) logInfo("GMS","DER","Leaving entity had active role.");
    else logInfo("GMS","DER","Leaving entity had active role. Standby is now Active.");

    // Promote the standby to active in a manner which NEVER allows other threads to see both the standby and active as the same node
    Handle tmp = standbyEntity;
    standbyEntity = INVALID_HDL;
    activeEntity = tmp;
    addCapabilities(activeEntity,SAFplus::Group::IS_ACTIVE);
    removeCapabilities(activeEntity,SAFplus::Group::IS_STANDBY);
  }
  else if(standbyEntity == me && myInformation.id == activeEntity)
  {
    logInfo("GMS","DER","Leaving entity had standby role.");
    standbyEntity = INVALID_HDL;
    /* Now, elect for the new standby */
  }
  if(automaticElection)  // since something failed, run a re-election automatically if that is the configured behavior
  {
    logInfo("GMS","DER","Run election based on configuration");
    elect();
  }

   /* We need to other entities about the left node, but only AFTER
    * we have handled it. */
  if(wakeable)
  {
    wakeable->wake(1,(void*)NODE_LEAVE_SIG);
  }

}

void SAFplus::Group::addCapabilities(EntityIdentifier id,uint capabilities)
  {
  uint cap = getCapabilities(id);
  cap |= capabilities;
  setCapabilities(cap,id);
  }

void SAFplus::Group::removeCapabilities(EntityIdentifier id,uint capabilities)
  {
  uint cap = getCapabilities(id);
  cap &= ~capabilities;
  setCapabilities(cap,id);
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
  DataHashMap::iterator contents = groupDataMap.find(me);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "CAP","Entity did not exist");
    return;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&me,sizeof(EntityIdentifier));

  GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
  if(grp == NULL)
  {
    /* Found entity but can't read its information. Memory corruption occured */
    logError("GMS","CAP","Invalid entity information");
    assert(0);
  }
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
  DataHashMap::iterator contents = groupDataMap.find(id);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "CAP","Entity did not exist");
    return 0;
  }

  char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
  SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
  memcpy(key->data,&id,sizeof(EntityIdentifier));

  GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
  if(grp == NULL)
  {
    /* Found entity but can't read its information. Memory corruption occured */
    logError("GMS","CAP","Invalid entity information");
    assert(0);
  }
  return grp->capabilities;
}
/**
 * API to get entity associated data
 */
void* SAFplus::Group::getData(EntityIdentifier id)
{
  /*Check in share memory if entity exists*/
  DataHashMap::iterator contents = groupDataMap.find(id);

  if(contents == groupDataMap.end())
  {
    logDebug("GMS", "DAT","Entity did not exist");
    return NULL;
  }
  return contents->second;
}

#if 0
/**
 * Election for leader/deputy
 */
EntityIdentifier SAFplus::Group::electARole(EntityIdentifier ignoreMe)
{
  uint requiredCapabilities = 0,curCapabilities = 0;
  uint highestCredentials = 0, curCredentials = 0;
  EntityIdentifier curEntity = INVALID_HDL, electedEntity = INVALID_HDL;

  if(ignoreMe == INVALID_HDL) // Required capabilities to become an active
  {
    requiredCapabilities = SAFplus::Group::ACCEPT_ACTIVE;
  }
  else // // Required capabilities to become a standby
  {
    requiredCapabilities = SAFplus::Group::ACCEPT_STANDBY;
  }
  for (DataHashMap::iterator i = groupDataMap.begin();i != groupDataMap.end();i++)
  {
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;

    GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
    curCredentials  = grp->credentials;
    curCapabilities = grp->capabilities;
    curEntity       = grp->id;
    if(((curCapabilities & requiredCapabilities) != 0) && (curEntity !=  ignoreMe))
    {
      if(highestCredentials < curCredentials)
      {
        highestCredentials = curCredentials;
        electedEntity = curEntity;
      }
    }
  }
  return electedEntity;
}

/**
 * Do election to find the required roles pairs <active,standby>
 */
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::electForRoles()
{
  EntityIdentifier activeCandidate = INVALID_HDL, standbyCandidate = INVALID_HDL;

  activeCandidate = electARole();
  if(activeCandidate == INVALID_HDL)
  {
    return std::pair<EntityIdentifier,EntityIdentifier>(INVALID_HDL,INVALID_HDL);
  }
  standbyCandidate = electARole(activeCandidate);
  return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);
}
#else

#define ACTIVE_ELECTION_MODIFIER 0x8000000ULL
#define STANDBY_ELECTION_MODIFIER 0x4000000ULL

std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::electForRoles()
{
  uint activeCapabilities = 0,curCapabilities = 0;
  uint64_t highestCredentials = 0, curCredentials = 0;
  EntityIdentifier curEntity = INVALID_HDL;

  EntityIdentifier activeCandidate=INVALID_HDL, standbyCandidate=INVALID_HDL;
  uint             activeCredentials=0, standbyCredentials=0;

  for (DataHashMap::iterator i = groupDataMap.begin();i != groupDataMap.end();i++)
  {
    char tempStr[100];
    char vkey[sizeof(SAFplus::Buffer)-1+sizeof(EntityIdentifier)];
    SAFplus::Buffer* key = new(vkey) Buffer(sizeof(EntityIdentifier));
    *((EntityIdentifier *) key->data) = i->first;
    tempStr[0] = 0;
    GroupIdentity *grp = (GroupIdentity *)readFromDatabase(key);
    curCredentials  = grp->credentials;
    curCapabilities = grp->capabilities;
    curEntity       = grp->id;

    // Prefer existing active/standby for the role rather than other nodes -- stops "failback"
    if ((curCapabilities & SAFplus::Group::IS_ACTIVE)&&(curCapabilities & SAFplus::Group::STICKY)) curCredentials |= ACTIVE_ELECTION_MODIFIER;
    if ((curCapabilities & SAFplus::Group::IS_STANDBY)&&(curCapabilities & SAFplus::Group::STICKY)) curCredentials |= STANDBY_ELECTION_MODIFIER;
    logInfo("GRP","ELC", "Member [%lx:%lx], capabilities: (%s) 0x%x, credentials: %lx",curEntity.id[0],curEntity.id[1],capStr(curCapabilities,tempStr), curCapabilities, curCredentials);


    // Obviously the anything that can accept the standby role must be able to be active too.
    // But it is possible to not be able to accept the standby role (transition directly to active).
    if ((curCapabilities & (SAFplus::Group::ACCEPT_ACTIVE |  SAFplus::Group::ACCEPT_STANDBY)) != 0)  
    {
      if(activeCredentials < curCredentials)
      {
        // If the active candidate is changing, see if the current candidate is a better match for standby
        if ((standbyCredentials < activeCredentials)&&(activeCapabilities&SAFplus::Group::ACCEPT_STANDBY))
          {
          standbyCredentials = activeCredentials;
          standbyCandidate   = activeCandidate;
          }
        activeCredentials = curCredentials;
        activeCandidate = curEntity;
        activeCapabilities = curCapabilities;  
      }
      else if ((standbyCredentials < curCredentials)&&(curCapabilities&SAFplus::Group::ACCEPT_STANDBY))
        {
        standbyCredentials = curCredentials;
        standbyCandidate = curEntity;
        }
    }
    else if ((curCapabilities & SAFplus::Group::ACCEPT_STANDBY) != 0)
      {
        clDbgCodeError(1, ("This entity's credentials are incorrect.  It cannot have standby capability but be unable to become active.") );
      }
  }

  return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);
}

#endif

char* SAFplus::Group::capStr(uint cap, char* buf)
  {
  if (cap & IS_ACTIVE) strcat(buf,"Active/");
  if (cap & IS_STANDBY) strcat(buf,"Standby/");
  if (cap & ACCEPT_ACTIVE) strcat(buf,"Accepts Active/");
  if (cap & ACCEPT_STANDBY) strcat(buf,"Accepts Standby/");
  if (cap & STICKY) strcat(buf,"Sticky/");
  int tmp = strlen(buf);
  if (tmp) buf[tmp-1] = 0;  // knock off the last / if it exists
  return buf;
  }

/**
 * API allow member call for election based on group database
 */
std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::elect(SAFplus::Wakeable& wake)
{
  std::pair<EntityIdentifier,EntityIdentifier> res;
  res.first = INVALID_HDL;
  res.second = INVALID_HDL;
  if(!isElectionRunning)
    {
    isElectionFinished = false;
    fillAndSendMessage(&myInformation,GroupMessageTypeT::MSG_ELECT_REQUEST,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
    electionRequestHandle(NULL);
    }

  // Asynchronous call
  if(&wake != NULL)
  {
    clDbgNotImplemented("");  // TODO, I need to put the wake object into a list so I can call it when the election is complete
    return res;
  }
  else
  {
    while(!isElectionFinished)  // TODO: should be a thread condition
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(50));
    }
    res.first = activeEntity;
    res.second = standbyEntity;
    return res;
  }

}
/**
 * API to check whether an entity is a group member
 */
bool SAFplus::Group::isMember(EntityIdentifier id)
{
  DataHashMap::iterator contents = groupDataMap.find(id);
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
  logInfo("GMS", "---","Notification had been set");
}
/**
 * API to get current active entity
 */
EntityIdentifier SAFplus::Group::getActive(void)
{
  if (activeEntity == INVALID_HDL) elect();  // TODO: We don't want to keep re-electing if there is no capable candidate, so this should really re-elect only if something changed.
  return activeEntity;
}
/**
 * API to set group active entity
 */
void SAFplus::Group::setActive(EntityIdentifier id)
{
  /* Stop all running election */
  isElectionRunning = false;
  isElectionFinished = true;
  /* Set the active entity as user expectation */
  activeEntity = id;
  if(stickyMode)
  {
    uint capabilities = 0;
    if(activeEntity != INVALID_HDL)
    {
      capabilities = getCapabilities(activeEntity);
      capabilities |= SAFplus::Group::IS_ACTIVE;
      capabilities &= (~SAFplus::Group::IS_STANDBY);
      setCapabilities(capabilities,activeEntity);
    }
  }
  /* Announce role change */
  fillAndSendMessage(&activeEntity,GroupMessageTypeT::MSG_ROLE_NOTIFY,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_ACTIVE, true);
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
EntityIdentifier SAFplus::Group::getStandby(void)
{
  if (standbyEntity == INVALID_HDL) elect(); // TODO: We don't want to keep re-electing if there is no capable candidate, so this should really re-elect only if something changed.
  return standbyEntity;
}

std::pair<EntityIdentifier,EntityIdentifier> SAFplus::Group::getRoles(void)
{
  if ((standbyEntity == INVALID_HDL)||(activeEntity == INVALID_HDL)) elect(); // TODO: We don't want to keep re-electing if there is no capable candidate, so this should really re-elect only if something changed.
  std::pair<EntityIdentifier,EntityIdentifier> ret(activeEntity,standbyEntity);
  return ret;
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
      return (void *)&(curItem->second);
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
      GroupIdentity *grp = (GroupIdentity *)val;
      SAFplus::GroupMapPair vt(eiKey,*grp);
      mGroupMap.insert(vt);
      break;
    }
    default:
    {
      logError("GMS","---","Unknown storing mode");
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

SAFplus::Group::GroupMessageHandler::GroupMessageHandler(SAFplus::Group* _group):mGroup(_group)
{

}

void SAFplus::Group::GroupMessageHandler::init(SAFplus::Group* _group)
{
  mGroup = _group;
}

void SAFplus::Group::GroupMessageHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  /* If no communication needed, just ignore */
  if(mGroup == NULL)
  {
    return;
  }

  /* Parse the message and process if it is valid */
  SAFplusI::GroupMessageProtocol *rxMsg = (SAFplusI::GroupMessageProtocol *)msg;
  if(rxMsg == NULL)
  {
    logError("GMS","MSG","Received NULL message. Ignored");
    return;
  }
  logInfo("GMS","MSG","Received message [%x] from node %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);

  switch(rxMsg->messageType)
  {
    case SAFplusI::GroupMessageTypeT::MSG_NODE_JOIN:
      if(from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
      logDebug("GMS","MSG","Node JOIN message");
      mGroup->nodeJoinHandle(rxMsg);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_HELLO:
      if(from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
      logDebug("GMS","MSG","Node HELLO message");
      mGroup->nodeJoinHandle(rxMsg);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_NODE_LEAVE:
      logDebug("GMS","MSG","Node LEAVE message");
      mGroup->nodeLeaveHandle(rxMsg);
      break;
    case SAFplusI::GroupMessageTypeT::MSG_ROLE_NOTIFY:
      logDebug("GMS","MSG","Role CHANGE message");
      mGroup->roleNotificationHandle(rxMsg);
      break;
    case SAFplusI::GroupMessageTypeT::MSG_ELECT_REQUEST:
      if(from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
        logDebug("GMS","MSG","Election REQUEST message");
        mGroup->electionRequestHandle(rxMsg);
        } break;
    default:
      logDebug("GMS","MSG","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
      break;
  }
}
