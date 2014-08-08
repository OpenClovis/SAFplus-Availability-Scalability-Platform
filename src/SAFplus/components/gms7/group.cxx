/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clGroup.hxx>
#include <clCommon.hxx>
#include <clGroup.hxx>
#include <clNameApi.hxx>
#include <clIocPortList.hxx>
#include <clGroupIpi.hxx>
#include <clHandleApi.hxx>

using namespace SAFplus;
using namespace SAFplusI;
using namespace std;

namespace SAFplusI
  {
  GroupSharedMem gsm;
  };

namespace SAFplus
  {
  Group::Iterator Group::Iterator::END;

char* Group::capStr(uint cap, char* buf)
  {
  buf[0] = 0;
  if (cap & Group::IS_ACTIVE) strcat(buf,"Active/");
  if (cap & Group::IS_STANDBY) strcat(buf,"Standby/");
  if (cap & Group::ACCEPT_ACTIVE) strcat(buf,"Accepts Active/");
  if (cap & Group::ACCEPT_STANDBY) strcat(buf,"Accepts Standby/");
  if (cap & Group::STICKY) strcat(buf,"Sticky/");
  int tmp = strlen(buf);
  if (tmp) buf[tmp-1] = 0;  // knock off the last / if it exists
  return buf;
  }

//GroupList allGroups;

  Group::Group()
    {
    groupMsgServer = NULL;
    }

  Group::~Group()
    {
    if (myInformation.id != INVALID_HDL)
      {
      deregister();
      }
    }

  Group::Group(Handle groupHandle, int comPort)
    {
    groupMsgServer = NULL;
    init(groupHandle, comPort);
    }

  void Group::init(Handle groupHandle, int comPort)
    {
    handle = groupHandle;
    groupCommunicationPort = comPort;

    if(!groupMsgServer)
      {
      groupMsgServer = &safplusMsgServer;
      }

    GroupShmHashMap::iterator entryPtr;
    do
      {
      entryPtr = gsm.groupMap->find(handle);
      if (entryPtr == gsm.groupMap->end())
        {
        sendGroupAnnounceMessage();  // This will be sent to all nodes, including my own node representative which will then update the shared memory so the group will appear.
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));  // TODO use thread change condition
        // TODO after too many loops, notify fault manager
        }
      } while (entryPtr == gsm.groupMap->end());
    }

  // Get the current active entity.  If an active entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with active capability
  EntityIdentifier Group::getActive(void)
    {
    clDbgNotImplemented();
    return INVALID_HDL;
    }

  // Get the current standby entity.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
  EntityIdentifier Group::getStandby(void)
    {
    clDbgNotImplemented();
    return INVALID_HDL;
    }

  // Get the current active/standby.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
  std::pair<EntityIdentifier,EntityIdentifier> Group::getRoles()
    {
    std::pair<EntityIdentifier,EntityIdentifier> ret(INVALID_HDL,INVALID_HDL);
    clDbgNotImplemented();
    return ret;

    }

  // Utility functions
  bool Group::isMember(EntityIdentifier id)
    {
    clDbgNotImplemented();
    return false;
    }

  // Calls for an election with specified role
  std::pair<EntityIdentifier,EntityIdentifier>  Group::elect(SAFplus::Wakeable& wake)
    {
    std::pair<EntityIdentifier,EntityIdentifier> ret(INVALID_HDL,INVALID_HDL);
    clDbgNotImplemented();
    return ret;
    }

  // triggers an election if not already running and returns (mostly for internal use)
  void Group::startElection(void)
    {
    //if(!isElectionRunning)  // GAS TODO optimize
      {
      fillAndSendMessage(&myInformation,GroupMessageTypeT::MSG_ELECT_REQUEST,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
      }
    }



#if 0
/**
 * Election request message
 */
  void SAFplus::Group::electionRequestHandle(SAFplusI::GroupMessageProtocol *rxMsg)
    {
    if (1)
      {
      ScopedLock<RecursiveMutex> sl(lock);

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
      }

    if(rxMsg != NULL) // Register the information provided by the node that requested election
      {
      GroupIdentity *grp = (GroupIdentity *)rxMsg->data;
      registerEntity(*grp,false);
      }
    }
#endif

/**
 * API to register an entity to the group
 */
  void SAFplus::Group::_registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities)
    {
    bool notify = false;
    if (1)
      {
      //ScopedLock<RecursiveMutex> sl(lock);

      GroupShmHashMap::iterator entryPtr = gsm.groupMap->find(handle);
      if (entryPtr == gsm.groupMap->end())  // Group does not exist
        {
        notify = true;
        }
      else
        {
        GroupShmEntry *gse = &entryPtr->second;
        if (!gse) notify = true;
        else
          {
          const GroupData& gdr = gse->read();
          const GroupIdentity* gi = gdr.find(me);
          if (!gi) notify = true;
          else
            {
            if (gi->credentials != credentials) notify = true;
            if (gi->capabilities&(~(Group::IS_ACTIVE | Group::IS_STANDBY)) != capabilities) notify = true; 
            // TODO if data changed, notify = true
            }
          }
        }
      if (notify)
        {
        GroupIdentity gi;
        gi.id = me;
        gi.credentials = credentials;
        gi.capabilities = capabilities;
        gi.dataLen = 0;   //  TODO: data 
        fillAndSendMessage((void *)&gi,GroupMessageTypeT::MSG_ENTITY_JOIN,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
        }
      }

    // TODO:  Should I wait here until the registration is successful?
    }

#if 0
/**
 * API to register an entity to the group
 */
  void SAFplus::Group::registerEntity(GroupIdentity grpIdentity)
    {
    registerEntity(grpIdentity.id,grpIdentity.credentials, NULL, grpIdentity.dataLen, grpIdentity.capabilities);
    }
#endif

/**
 * API to deregister an entity from the group
 */
  void SAFplus::Group::deregister(EntityIdentifier me)
    {
    if (1)
      {
      //ScopedLock<RecursiveMutex> sl(lock);
      /* Use last registered entity if me is 0 */
      if(me == INVALID_HDL)
        {
        me = myInformation.id;
        myInformation.id = INVALID_HDL; // I deregistered myself so this group object no longer also represents an entity.
        }

      /* Find existence of the entity */
      GroupShmHashMap::iterator entryPtr = gsm.groupMap->find(handle);
      if (entryPtr == gsm.groupMap->end())  // Group does not exist
        {
        logWarning("GMS", "DER","Deregister entity from nonexistant group!");
        return;
        }
      else
        {
        GroupShmEntry *gse = &entryPtr->second;
        if (!gse) 
          {
          assert(0);  // group should never have no data.
          logWarning("GMS", "DER","Deregister entity from group with no data.");
          return;
          }
        else
          {
          const GroupData& gdr = gse->read();
          const GroupIdentity* gi = gdr.find(me);
          if (!gi) 
            {
            logWarning("GMS", "DER","Attempt to deregister entity that is not registered.");
            return;
            }
          }
        }

      // ok passed all existence checks, so send the deregister message.

      fillAndSendMessage((void *)&me,GroupMessageTypeT::MSG_ENTITY_LEAVE,GroupMessageSendModeT::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);

      // TODO wait until the node representative actually receives the message and changes shared memory?
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

  void SAFplus::Group::sendGroupAnnounceMessage()
    {
    GroupMessageProtocol sndMessage;
    sndMessage.group = handle;
    sndMessage.messageType = GroupMessageTypeT::MSG_GROUP_ANNOUNCE;
    sndMessage.roleType    = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
    sndMessage.force = 0;
    sndMessage.data[0] = 0;
    sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocol),GroupMessageSendModeT::SEND_BROADCAST);
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
      case GroupMessageTypeT::MSG_ENTITY_JOIN:
      case GroupMessageTypeT::MSG_ELECT_REQUEST:
      case GroupMessageTypeT::MSG_HELLO:
        msgLen = sizeof(GroupMessageProtocol) + sizeof(GroupIdentity);
        msgDataLen = sizeof(GroupIdentity);
        break;
      case GroupMessageTypeT::MSG_ENTITY_LEAVE:
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


  Group::Iterator::Iterator(Group* _group,bool lock):group(_group),locked(lock)
    {
    if (group==NULL) curIdx=0xffff;
    else
      {
      curIdx = 0;
      if (locked)
        {
        gsm.mutex.lock();  // Currently use a gross lock across all groups
        }
      load();
      }
    }

  Group::Iterator::~Iterator()
    {
    if (locked)
      {
      gsm.mutex.unlock();  // Currently use a gross lock across all groups
      locked = false;
      }
    }

  Group::Iterator& Group::Iterator::operator++()
  {
  if (curIdx != 0xffff) 
    {
    curIdx++;
    load();
    }
  return *this;
  }

  Group::Iterator& Group::Iterator::operator++(int)
  {
  if (curIdx != 0xffff) 
    {
    curIdx++;
    load();
    }
  return *this;
  }

  void Group::Iterator::load(void)
  {
  GroupShmHashMap::iterator entryPtr;
  entryPtr = gsm.groupMap->find(group->handle);
  if (entryPtr != gsm.groupMap->end())  // Group disappeared!
    {
    GroupShmEntry *gse = &entryPtr->second;
    if (gse)
      {
      const GroupData& gdr = gse->read();
      if (curIdx < gdr.numMembers) 
        {
        curval.first = gdr.members[curIdx].id;
        curval.second = gdr.members[curIdx];
        return;
        }
      }
    }

    // If any of the if conditions fail, we are at the end of the list
    curIdx=0xffff; 
    curval.first = INVALID_HDL;
  }

  void groupInitialize(void)
    {
    gsm.init();
    }


  };
