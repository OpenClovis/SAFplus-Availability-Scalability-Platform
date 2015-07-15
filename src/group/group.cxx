/* Standard headers */
#include <string>
/* SAFplus headers */
#include <clGroup.hxx>
#include <clCommon.hxx>
#include <clGroup.hxx>
#include <clNameApi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clGroupIpi.hxx>
#include <clHandleApi.hxx>
#include <clLogApi.hxx>

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
    wakeable = NULL;
    }
 
  Group::Group(Handle groupHandle, int comPort)
    {
    wakeable = NULL;
    groupMsgServer = NULL;
    init(groupHandle, comPort);
    }

  Group::Group(SAFplus::Handle groupHandle,const char* name, int comPort)
    {
    wakeable = NULL;
    groupMsgServer = NULL;
    init(groupHandle, comPort);
    
    if (name && name[0] != 0)
      {
      setName(name);
      }
    }


  Group::~Group()
    {
    if (wakeable)
      {
      wakeable = NULL;
      gsm.deregisterGroupObject(this);
      }

    if (myInformation.id != INVALID_HDL)
      {
      deregister();
      }
    }


  void Group::init(Handle groupHandle, const char* name, int comPort)
    {
    init(groupHandle,comPort);

    if (name && name[0] != 0)
      {
      setName(name);
      }
    }

  void Group::init(Handle groupHandle, int comPort,SAFplus::Wakeable& execSemantics)
    {
    handle = groupHandle;
    if (comPort==0)  // If 0 is passed, that means to use the default port, which is whatever is stored in shared memory
      {
        groupCommunicationPort = gsm.groupHdr->repPort;
      }
    else groupCommunicationPort = comPort;

    if(!groupMsgServer)
      {
      groupMsgServer = &safplusMsgServer;
      }

    GroupShmHashMap::iterator entryPtr;
    do  // waiting for group to be registered into shared memory by the group's node representative.  If this loop never exits, the group node rep is dead.
      {
      entryPtr = gsm.groupMap->find(handle);
      if (entryPtr == gsm.groupMap->end())
        {
        sendGroupAnnounceMessage();  // This will be sent to all nodes, including my own node representative which will then update the shared memory so the group will appear.
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));  // TODO use thread change condition
        // TODO after too many loops, notify fault manager
        }
      } while ((&execSemantics == &BLOCK)&&(entryPtr == gsm.groupMap->end()));
    }

  // Get the current active entity.  If an active entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with active capability
  EntityIdentifier Group::getActive(SAFplus::Wakeable& execSemantics)
    {
    EntityIdentifier ret;
    do
      {
      ret = getRoles().first;
      if (ret == INVALID_HDL)
        {
        if (&execSemantics == &BLOCK) boost::this_thread::sleep(boost::posix_time::milliseconds(100));  // TODO use thread change condition
        else if (&execSemantics != &ABORT)
          { clDbgCodeError(1,("Async call not supported")); }
        }
      } while ((&execSemantics == &BLOCK) && (ret == INVALID_HDL));
    return ret;
    }


  // Get the current standby entity.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
  EntityIdentifier Group::getStandby(SAFplus::Wakeable& execSemantics)
    {
    EntityIdentifier ret;
    do
      {
      ret = getRoles().second;
      if (ret == INVALID_HDL)
        {
        if (&execSemantics == &BLOCK) boost::this_thread::sleep(boost::posix_time::milliseconds(100));  // TODO use thread change condition
        else if (&execSemantics != &ABORT)
          { clDbgCodeError(1,("Async call not supported")); }
        }
      } while ((&execSemantics == &BLOCK) && (ret == INVALID_HDL));
    return ret;
    }

  // Get the current active/standby.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
  std::pair<EntityIdentifier,EntityIdentifier> Group::getRoles()
    {
    std::pair<EntityIdentifier,EntityIdentifier> ret(INVALID_HDL,INVALID_HDL);
    GroupShmHashMap::iterator entryPtr;
    entryPtr = gsm.groupMap->find(handle);
    if (entryPtr == gsm.groupMap->end()) return ret;
    GroupShmEntry *gse = &entryPtr->second;
    if (!gse) return ret;
    ret = gse->getRoles();
    return ret;
    }

  void Group::setName(const char* name)
    {
    GroupShmHashMap::iterator entryPtr;
    entryPtr = gsm.groupMap->find(handle);
    if (entryPtr == gsm.groupMap->end()) return; // TODO: raise exception
    GroupShmEntry *gse = &entryPtr->second;
    assert(gse);  // If this fails, something very wrong with the group data structure in shared memory.  TODO, should probably delete it and fail the node
    if (gse) // Name is not meant to change, and not used except for display purposes.  So just change both of them
      {
      strncpy(gse->name,name,SAFplusI::GroupShmEntry::GROUP_NAME_LEN);
      }
    else
      {
      }
    }

  // Utility functions
  bool Group::isMember(EntityIdentifier id)
    {
    clDbgNotImplemented();
    return false;
    }

#if 0
  // Calls for an election with specified role
  std::pair<EntityIdentifier,EntityIdentifier>  Group::elect(SAFplus::Wakeable& wake)
    {
    std::pair<EntityIdentifier,EntityIdentifier> ret(INVALID_HDL,INVALID_HDL);
    clDbgNotImplemented();
    return ret;
    }
#endif

  // triggers an election if not already running and returns (mostly for internal use)
  void Group::startElection(void)
    {
    //if(!isElectionRunning)  // GAS TODO optimize
      {
      fillAndSendMessage(&myInformation,GroupMessageTypeT::MSG_ELECT_REQUEST,GroupMessageSendMode::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
      }
    }

void Group::setNotification(SAFplus::Wakeable& w)
  {
  if (!wakeable) gsm.registerGroupObject(this);
  wakeable = &w;
  }

  uint64_t Group::lastChange()
    {
      GroupShmHashMap::iterator entryPtr = gsm.groupMap->find(handle);
      if (entryPtr == gsm.groupMap->end())  // Group does not exist
        {
        return 0;  // new group; not even in shared memory yet.
        }
      else
        {
        GroupShmEntry *gse = &entryPtr->second;
        if (!gse) return 0;  // should never happen
        else
          {
          const GroupData& gdr = gse->read();
          return(gdr.lastChanged);
          }
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
        fillAndSendMessage((void *)&gi,GroupMessageTypeT::MSG_ENTITY_JOIN,GroupMessageSendMode::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);
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
            logWarning("GMS", "DER","Attempting to deregister entity that is not registered.  This may legitimately occur if an entity is deregistered right after registration");
            }
          }
        }

      // ok passed all existence checks, so send the deregister message.
      logDebug("GMS", "DER","Deregister entity [%" PRIx64 ":%" PRIx64 "]",me.id[0],me.id[1]);
      fillAndSendMessage((void *)&me,GroupMessageTypeT::MSG_ENTITY_LEAVE,GroupMessageSendMode::SEND_BROADCAST,GroupRoleNotifyTypeT::ROLE_UNDEFINED);

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
    sendNotification((void *)sndMessage,msgLen,GroupMessageSendMode::SEND_BROADCAST);
    }

  void SAFplus::Group::sendGroupAnnounceMessage()
    {
    GroupMessageProtocol sndMessage;
    sndMessage.group = handle;
    sndMessage.messageType = GroupMessageTypeT::MSG_GROUP_ANNOUNCE;
    sndMessage.roleType    = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
    sndMessage.force = 0;
    sndMessage.data[0] = 0;
    sendNotification((void *)&sndMessage,sizeof(GroupMessageProtocol),GroupMessageSendMode::SEND_BROADCAST);
    }



/**
 * Fill information and call message server to send
 */
  void SAFplus::Group::fillAndSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplus::GroupMessageSendMode msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType,bool forcing)
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

  
void SAFplus::Group::send(void* data, int dataLength, SAFplus::GroupMessageSendMode messageMode)
  {
  // TODO Use an advanced Buffer data structure to avoid copies and malloc
  int len = dataLength+sizeof(Handle);
  char* buf = (char*) malloc(len);
  assert(buf);
  memcpy(buf+sizeof(Handle),data,dataLength);

  SAFplus::Handle dest = INVALID_HDL;

  switch(messageMode)
    {
    case GroupMessageSendMode::SEND_BROADCAST:
      {
      for (Iterator i = begin(); i != end(); i++)
        {
        SAFplus::Handle hdl = i->first;
        //ClIocAddress to = getAddress(hdl);        
        memcpy(buf,&hdl,sizeof(Handle));
        groupMsgServer->SendMsg(hdl, (void *)buf, len, SAFplusI::OBJECT_MSG_TYPE);
        }
      }
      break;
    case GroupMessageSendMode::SEND_TO_ACTIVE:
      {
      dest = getActive();
      if (dest == INVALID_HDL)
        {
        assert(0); // TODO throw exception or block
        // throw
        }
      } break;
    case GroupMessageSendMode::SEND_TO_STANDBY:
      {
      dest = getStandby();
      if (dest == INVALID_HDL)
        {
        assert(0); // TODO throw exception or block
        // throw
        }
      } break;
    case GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN:
      {
      roundRobin++;
      if (roundRobin == end()) 
        roundRobin = begin();
      if (roundRobin == end())
        {
        assert(0);  // No members of the group, TODO throw exception or block
        }
      dest = roundRobin->first;
      } break;
    }

  if (dest != INVALID_HDL)
    {
      //ClIocAddress to = getAddress(dest);
      memcpy(buf,&dest,sizeof(Handle));
      //to.iocPhyAddress.nodeAddress = to; // CL_IOC_BROADCAST_ADDRESS;
      groupMsgServer->SendMsg(dest, (void *)buf, len, SAFplusI::OBJECT_MSG_TYPE);
    }

  }

    bool SAFplus::GroupIdentity::override(uint64_t new_credentials,uint new_capabilities)
    {
      bool changed = false;
      if (new_credentials != credentials) {credentials = new_credentials; changed = true; }
      // I only want to change the configured capabilities, not the role
      if (new_capabilities & Group::SOURCE_CAPABILITIES != capabilities & Group::SOURCE_CAPABILITIES) { capabilities = new_capabilities; changed = true; }
      return changed;
    }

  void SAFplus::GroupIdentity::dumpInfo()
  {
    logInfo("GMS", "---","Dumping GroupIdentity at [%p]",this);
    logInfo("GMS", "---","ID: [%" PRIx64 ":%" PRIx64 "]",id.id[0],id.id[1]);
    logInfo("GMS", "---","CREDENTIALS: [0x%" PRIx64 "]",credentials);
    logInfo("GMS", "---","CAPABILITY: [0x%x]",capabilities);
    logInfo("GMS", "---","DATA LENGTH: [0x%x]",dataLen);
  }
/**
 * Actually send message
 */
  void  SAFplus::Group::sendNotification(void* data, int dataLength, GroupMessageSendMode messageMode)
    {
    switch(messageMode)
      {
      case GroupMessageSendMode::SEND_BROADCAST:
      {
      /* Destination is broadcast address */
      Handle broadcastDest = getProcessHandle(groupCommunicationPort,Handle::AllNodes);
      //logInfo("GMS","MSG","Sending broadcast message");
      try
        {
        groupMsgServer->SendMsg(broadcastDest, (void *)data, dataLength, SAFplusI::GRP_MSG_TYPE);
        }
      catch (...) // SAFplus::Error &e)
        {
        //logDebug("GMS","MSG","Failed to send. Error %x",e.rc);
        logDebug("GMS","MSG","Failed to send.");
        }
      break;
      }
      case GroupMessageSendMode::SEND_TO_ACTIVE:
      {
      throw Error(Error::SAFPLUS_ERROR,Error::NOT_IMPLEMENTED);
      break;
      }
      case GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN:
      {
      logInfo("GMS","MSG","Sending message round robin");
      throw Error(Error::SAFPLUS_ERROR,Error::NOT_IMPLEMENTED);
      break;
      }
      default:
      {
      logError("GMS","MSG","Unknown message sending mode");
      assert(0);
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

    int groupInitCount=0;
  void groupInitialize(void)
    {
      groupInitCount++;
      if (groupInitCount > 1) return;
      gsm.init();
    }


  };
