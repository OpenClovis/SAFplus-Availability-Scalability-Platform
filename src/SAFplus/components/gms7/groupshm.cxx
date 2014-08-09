#include <clCustomization.hxx>
#include <clGroupIpi.hxx>
#include <clIocPortList.hxx>
#include <chrono>

using namespace boost::interprocess;
using namespace SAFplus;
using namespace SAFplusI;

bool SAFplusI::HandleEqual::operator() (const Handle& x, const Handle& y) const
{
  bool result = (x==y);
  return result;
}

namespace SAFplusI
{

ClRcT groupIocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie);

GroupServer::GroupServer()
 { groupCommunicationPort=GMS_IOC_PORT; }




void GroupSharedMem::dbgDump(void)
  {
  char buf[100];
  GroupShmHashMap::iterator i;
  assert(groupMap);
  ScopedLock<ProcSem> lock(mutex);

  for (i=groupMap->begin(); i!=groupMap->end();i++)
    {
    SAFplus::Handle grpHdl = i->first;
    GroupShmEntry& ge = i->second;
    const GroupData& gd = ge.read();
    printf("Group [%lx:%lx]:\n", grpHdl.id[0], grpHdl.id[1]);
    for (int j = 0 ; j<gd.numMembers; j++)
      {
      const GroupIdentity& gid = gd.members[j];
      printf("    Entity [%lx:%lx] on node [%d] credentials [%ld] capabilities [%d] %s\n", gid.id.id[0],gid.id.id[1],gid.id.getNode(),gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
      }
    }
  }

void GroupSharedMem::init()
  {
  mutex.init("GroupSharedMem",1);
  ScopedLock<ProcSem> lock(mutex);
  groupMsm = boost::interprocess::managed_shared_memory(open_or_create, "SAFplusGroups", GroupSharedMemSize);

  try
    {
    groupHdr = (SAFplusI::GroupShmHeader*) groupMsm.construct<SAFplusI::GroupShmHeader>("header") ();                                 // Ok it created one so initialize
    groupHdr->rep  = getpid();
    groupHdr->activeCopy = 0;
    groupHdr->structId=SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
    }
  catch (interprocess_exception &e)
    {
    if (e.get_error_code() == already_exists_error)
      {
      groupHdr = groupMsm.find_or_construct<SAFplusI::GroupShmHeader>("header") ();                         //allocator instance
      int retries=0;
      while ((groupHdr->structId != CL_GROUP_BUFFER_HEADER_STRUCT_ID_7)&&(retries<2)) { retries++; sleep(1); }  // If another process just barely beat me to the creation, I better wait.
      if (retries>=2)  // Another process is supposedly initializing the header but it did not do so.  Something unknown is wrong.  We will try initializing it.
        {
        groupHdr->rep  = getpid();
        groupHdr->activeCopy = 0;
        groupHdr->structId=SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
        }
      }
    else throw;
    }

  //groupMap = new GroupShmHashMap();
  groupMap = groupMsm.find_or_construct<GroupShmHashMap>("groups")  (groupMsm.get_allocator<GroupMapPair>());
  // TODO assocData = groupMsm.find_or_construct<CkptHashMap>("data") ...
  }

void GroupServer::init()
  {
  gsm.init();
  clIocNotificationRegister(groupIocNotificationCallback,this);

  if(!groupMsgServer)
    {
    groupMsgServer = &safplusMsgServer;
    }

  if (1) //groupMsgServer->port == groupCommunicationPort) // If my listening port is the broadcast port then I must be the node representative
    {
    //allGrpMsgHdlr.handleMap[handle] = &groupMessageHandler;  //  Register this object's message handler into the global lookup.
    groupMsgServer->RegisterHandler(SAFplusI::GRP_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
    }

  }


GroupShmEntry* GroupSharedMem::createGroup(SAFplus::Handle grp)
  {
  ScopedLock<ProcSem> lock(mutex);
  GroupShmEntry* ge = &((*groupMap)[grp]);
  assert(ge);  // TODO: throw out of memory
  ge->init(grp);
  return ge;
  }

// Demultiplex incoming message to the appropriate Checkpoint object
void GroupServer::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  logInfo("SYNC","MSG","Received group message from %d", from.iocPhyAddress.nodeAddress);

  /* Parse the message and process if it is valid */
  SAFplusI::GroupMessageProtocol *rxMsg = (SAFplusI::GroupMessageProtocol *)msg;
  //assert((rxMsg->msgType>>16) == SAFplusI::GRP_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data

  SAFplus::Handle grpHandle = rxMsg->group;
  GroupShmHashMap::iterator entryPtr = gsm.groupMap->find(grpHandle);
  GroupShmEntry* ge = NULL;
  if (entryPtr == gsm.groupMap->end())  // We don't know about the group, so create it;
    {
    ge = gsm.createGroup(grpHandle);
    }
  else
    {
    ge = &entryPtr->second; // &(gsm.groupMap->find(grpHandle)->second);
    }

  if(rxMsg == NULL)
  {
    logError("GMS","MSG","Received NULL message. Ignored");
    return;
  }
  logInfo("GMS","MSG","Received message [%x] from node %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);

  switch(rxMsg->messageType)
  {
    case SAFplusI::GroupMessageTypeT::MSG_ENTITY_JOIN:
      if(1) //from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
        logDebug("GMS","MSG","Entity JOIN message");
        GroupIdentity *rxGrp = (GroupIdentity *)rxMsg->data;
        registerEntity(ge, rxGrp->id, rxGrp->credentials, NULL, rxGrp->dataLen, rxGrp->capabilities, false);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_HELLO:
      if(from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
        logDebug("GMS","MSG","Entity HELLO message");
        GroupIdentity *rxGrp = (GroupIdentity *)rxMsg->data;
        registerEntity(ge, rxGrp->id, rxGrp->credentials, NULL, rxGrp->dataLen, rxGrp->capabilities, false);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_ENTITY_LEAVE:
      {
      logDebug("GMS","MSG","Entity LEAVE message");
      //mGroup->nodeLeaveHandle(rxMsg);
      EntityIdentifier *eId = (EntityIdentifier *)rxMsg->data;
      deregisterEntity(ge, *eId,false);
      } break;
    case SAFplusI::GroupMessageTypeT::MSG_ROLE_NOTIFY:
      {
      logDebug("GMS","MSG","Role CHANGE message");
      handleRoleNotification(ge, rxMsg);
      } break;
    case SAFplusI::GroupMessageTypeT::MSG_ELECT_REQUEST:
      if(!(ge->read().flags & GroupData::ELECTION_IN_PROGRESS)) // If we are not already in an election, then start one.  //from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
        logDebug("GMS","MSG","Election REQUEST message");
        handleElectionRequest(grpHandle);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_GROUP_ANNOUNCE:
      {
      // Nothing to do here, we've already created the group if it didn't exist.
      logDebug("GMS","MSG","Group announce message");
      } break;
    default:
      logDebug("GMS","MSG","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
      break;
  }
}

/**
 * Election request message
 */
void GroupServer::handleElectionRequest(SAFplus::Handle grpHandle)
  {
  ScopedLock<ProcSem> lock(gsm.mutex);

  GroupShmHashMap::iterator entryPtr = gsm.groupMap->find(grpHandle);
  GroupShmEntry* ge = NULL;

  if (1)
    {

    if (entryPtr == gsm.groupMap->end())  // We don't know about the group so skip the election
      {
      return;
      }

    ge = &entryPtr->second; // &(gsm.groupMap->find(grpHandle)->second);
    if (1)
      {
      const GroupData& gd = ge->read(); 

      if (gd.flags & GroupData::ELECTION_IN_PROGRESS) // If we are already in an election ignore this call
        {
        return;
        }
      ge->setElectionFlag();  // clear the election flag a bit early so I can react to other nodes' calls for re-elect
      }
    const GroupData& gd = ge->read(); 

    // Now tell other group servers about all members on this node...
    for (int i=0;i<gd.numMembers; i++)
      {
      const GroupIdentity& gi = gd.members[i];
      if (gi.id.getNode() == SAFplus::ASP_NODEADDR)
        {
        sendHelloMessage(grpHandle, gi);  // TODO: create a single message in a buffer and then send it after unlocking the gsm
        }
      }

    }
  uint_t sleepTime = ge->read().electionTimeMs;

  gsm.mutex.unlock();
  // Now wait for election related announcements.
  boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTime/2));
  gsm.mutex.lock();
  // After sleeping, calculate the election winners

  // First reload the shared memory... it may have changed during the sleep!
  if (1)
    {
    entryPtr = gsm.groupMap->find(grpHandle);
    if (entryPtr == gsm.groupMap->end())  // We don't know about the group so skip the election
      {
      return;
      }
    ge = &entryPtr->second;
    const GroupData& gd = ge->read();

    std::pair<EntityIdentifier,EntityIdentifier> results = _electRoles(gd);  // run the election

    // If I don't think there is a valid candidate for active/standby, I don't need to wait around to see if I need to call for another election because I do not think anyone can be voted in.
    if (results.first == INVALID_HDL)
      {
      ge->clearElectionFlag();
      return;
      }

    if (results.first.getNode() == SAFplus::ASP_NODEADDR)  // If an entity on my node wins active, its my job to announce the winners.
      {
      sendRoleAssignmentMessage(grpHandle,results);
      }
    }

  gsm.mutex.unlock();
  // Now wait for the announcement from whichever node owns the winner to take effect
  // Forcing the winning node to announce allows us to validate that the node is alive, and that node could validate that the winning process is alive (TODO)
  boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTime/4));
  ge->clearElectionFlag();  // clear the election flag a bit early so I can react to other nodes' calls for re-elect
  boost::this_thread::sleep(boost::posix_time::milliseconds(sleepTime/4));
  gsm.mutex.lock();

  // After sleeping, make sure someone was elected.  If not, try again.
  if (1)
    {
    // First reload the shared memory... it may have changed during the sleep!
    entryPtr = gsm.groupMap->find(grpHandle);
    if (entryPtr == gsm.groupMap->end())  // We don't know about the group so skip the election
      {
      return;
      }
    ge = &entryPtr->second;
    const GroupData& gd = ge->read();

    if (gd.flags & GroupData::ELECTION_IN_PROGRESS) // somebody beat us to a new election request so no need to check whether we should call for one.
      return;

    if (gd.activeIdx == 0xffff)  // Oops, nobody got elected, but I think there should have been a winner.  Call for a new election
      {
      startElection(grpHandle);  // When I process this message, this function will be called again, so quit instead of looping.
      }
    }
  }

#define ACTIVE_ELECTION_MODIFIER 0x8000000ULL
#define STANDBY_ELECTION_MODIFIER 0x4000000ULL

std::pair<EntityIdentifier,EntityIdentifier> GroupServer::_electRoles(const GroupData& gd)
{
  uint activeCapabilities = 0,curCapabilities = 0;
  uint64_t highestCredentials = 0, curCredentials = 0;
  EntityIdentifier curEntity = INVALID_HDL;

  EntityIdentifier activeCandidate=INVALID_HDL, standbyCandidate=INVALID_HDL;
  uint             activeCredentials=0, standbyCredentials=0;

  for (int i=0;i<gd.numMembers; i++)
    {
    const GroupIdentity& gi = gd.members[i];
    char tempStr[100];

    curCredentials  = gi.credentials;
    curCapabilities = gi.capabilities;
    curEntity       = gi.id;

    // Prefer existing active/standby for the role rather than other nodes -- stops "failback"
    if ((curCapabilities & SAFplus::Group::IS_ACTIVE)&&(curCapabilities & SAFplus::Group::STICKY)) curCredentials |= ACTIVE_ELECTION_MODIFIER;
    if ((curCapabilities & SAFplus::Group::IS_STANDBY)&&(curCapabilities & SAFplus::Group::STICKY)) curCredentials |= STANDBY_ELECTION_MODIFIER;
    logInfo("GRP","ELC", "Member [%lx:%lx], capabilities: (%s) 0x%x, credentials: %lx",curEntity.id[0],curEntity.id[1],Group::capStr(curCapabilities,tempStr), curCapabilities, curCredentials);


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
        // Normally would be a code error but don't kill the server due to a problem with some client
        logWarning("GRP","ELT","This entity's credentials are incorrect.  It cannot have standby capability but be unable to become active.");
      }
  }

  return std::pair<EntityIdentifier,EntityIdentifier>(activeCandidate,standbyCandidate);
}


/**
 * Role notification message from message server
 */
void GroupServer::handleRoleNotification(GroupShmEntry* ge, SAFplusI::GroupMessageProtocol *rxMsg)
{
  bool reelect=false;
  ScopedLock<ProcSem> lock(gsm.mutex);

  SAFplus::Handle grpHandle = rxMsg->group;

  /* We have roles from local election, now...check whether it is consistent */
  switch(rxMsg->roleType)
  {
    case GroupRoleNotifyTypeT::ROLE_ACTIVE:
    {
      std::pair<EntityIdentifier,EntityIdentifier> as = ge->getRoles();
      EntityIdentifier *announce = (EntityIdentifier *)rxMsg->data;
#if 0
      if(!rxMsg->force)  // If we do not want to force, then check if our calculations match the claimant.  If they do not match, we need to re-elect.
      {
        std::pair<int,int> roles = electForRoles(ge->read);
        if (announce[0] != roles.first)
          {
          logError("GMS","ROL","Mismatched active role.  I calculated [%lx:%lx] but received [%lx:%lx]",roles.first.id[0],roles.first.id[1],announce[0].id[0],announce[0].id[1]);
          reelect=true;
          }
      }
#endif

      // But accept the announcement regardless of my election results because the cluster must remain consistent
      if ((as.first != announce[0])||(as.second != announce[1]))
        {
        GroupData* data = &ge->write();
        memcpy(data, &ge->read(), sizeof(GroupData));  // copy the active data over to the new area

        if (as.first != announce[0])  // active entity has changed
          {
          data->setActive(announce[0]);
          }

        if (as.second != announce[1])  // standby entity has changed
          {
          data->setStandby(announce[1]);
          }
        data->lastChanged = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
        ge->flip();
        logInfo("GMS","ROL","Role change announcement active [%lx:%lx] standby [%lx:%lx]", announce[0].id[0],announce[0].id[1],announce[1].id[0],announce[1].id[1]);

        //isElectionRunning = false;

#if 0
        /* If some one are waiting the election, waking up now */
        if(wakeable)
        {
          wakeable->wake(1,(void*)ELECTION_FINISH_SIG);
        }
#endif
      }

#if 0
      if (roleWaitingTHandle)
        {
        clTimerStop(roleWaitingTHandle);
        clTimerDeleteAsync(&roleWaitingTHandle);
        roleWaitingTHandle = 0;
        }
#endif
      break;
    }


    case GroupRoleNotifyTypeT::ROLE_STANDBY:
      assert(0);
      break;

    default:
      break;

  }

  //if (reelect) startElection();  // I didn't like the result for some reason so am calling for another round.
}


ClRcT groupIocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie)
{
  GroupServer* svr = (GroupServer*) cookie;
  ClRcT rc = CL_OK;
  ClIocAddressT address;
  ClIocNotificationIdT eventId = (ClIocNotificationIdT) ntohl(notification->id);
  ClIocNodeAddressT nodeAddress = ntohl(notification->nodeAddress.iocPhyAddress.nodeAddress);
  ClIocPortT portId = ntohl(notification->nodeAddress.iocPhyAddress.portId);
  logDebug("GMS","IOC","Recv notification [%d] for [%d.%d]", eventId, nodeAddress,portId);
  switch(eventId)
    {
    case CL_IOC_COMP_DEATH_NOTIFICATION:
      {
      logDebug("GMS","IOC","Received component leave notification for [%d.%d]", nodeAddress,portId);
      ScopedLock<ProcSem> lock(svr->gsm.mutex);
      GroupShmHashMap::iterator i;

      for (i=svr->gsm.groupMap->begin(); i!=svr->gsm.groupMap->end();i++)
        {
        GroupShmEntry& ge = i->second;
        Handle handle = i->first;
        logDebug("GMS","IOC","  Checking group [%lx:%lx]", handle.id[0],handle.id[1]);
        svr->_deregister(&ge, nodeAddress, portId);
        }

      } break;
    case CL_IOC_NODE_LEAVE_NOTIFICATION:
    case CL_IOC_NODE_LINK_DOWN_NOTIFICATION:
    {
    logDebug("GMS","IOC","Received node leave notification for node [%d]", nodeAddress);

    ScopedLock<ProcSem> lock(svr->gsm.mutex);
    GroupShmHashMap::iterator i;

    for (i=svr->gsm.groupMap->begin(); i!=svr->gsm.groupMap->end();i++)
      {
      GroupShmEntry& ge = i->second;
      svr->_deregister(&ge,nodeAddress);
      }
    } break;
    default:
    {
    logInfo("GMS","IOC","Received event [%d] from IOC notification",eventId);
    break;
    }
    }
  return rc;
}


/**
 * Register an entity to the group
 */
void GroupServer::registerEntity(GroupShmEntry* grp, EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities,bool needNotify)
  {
  bool dirty = false;
  if (1)
    {
    ScopedLock<ProcSem> lock(gsm.mutex);
    GroupIdentity* gi = NULL;
    GroupData* data = &grp->write();
    memcpy(data, &grp->read(), sizeof(GroupData));  // copy the active data over to the new area
    // Now I can use the new data

    // see if the entity already exists & if it does update it.
    int i;
    for (i=0;i<data->numMembers; i++)
      {
      gi = &data->members[i];
      if (gi->id == me)
        {
        dirty |= gi->override(credentials, capabilities);
        break;
        }
      }

    if (i==data->numMembers)  // Entity does not exist, add it
      {
      if (data->numMembers == SAFplusI::GroupMaxMembers)
        {
        assert(0);
        // throw  TODO
        }
      data->numMembers++;
      gi = &data->members[i];
      gi->init(me,credentials,capabilities);
      dirty = true;
      }

    if (dirty)
      {
      data->changeCount++;
      data->lastChanged = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
      grp->flip();
      logDebug("GMS", "REG","Entity [%ld:%ld] registration successful",me.id[0],me.id[1]);
      /* Notify other entities about new entity*/
#if 0 // TODO
      if(wakeable)
        {
        wakeable->wake(1,(void*)NODE_JOIN_SIG);
        }
#endif
      }
    }

  // OPTIMIZE: check internally if the new registration would change anything
  if (dirty && grp->read().flags & GroupData::AUTOMATIC_ELECTION)
    {
    logInfo("GMS","REG","Run election based on configuration");
    startElection(grp->read().hdl);
    }

  }

void GroupServer::startElection(SAFplus::Handle grpHandle)
  {
  char msgPayload[sizeof(GroupMessageProtocol)-1];
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;

  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_ELECT_REQUEST;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
  sndMessage->force        = 0;

  ClIocAddressT iocDest;
  iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  iocDest.iocPhyAddress.portId      = groupCommunicationPort;
  groupMsgServer->SendMsg(iocDest, (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }

void GroupServer::sendHelloMessage(SAFplus::Handle grpHandle,const GroupIdentity& entityData)
  {
  char msgPayload[sizeof(GroupMessageProtocol)-1 + sizeof(GroupIdentity)];
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;

  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_HELLO;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
  sndMessage->force        = 0;
  memcpy(sndMessage->data,(const void*) &entityData,sizeof(GroupIdentity));
  ClIocAddressT iocDest;
  iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  iocDest.iocPhyAddress.portId      = groupCommunicationPort;
  groupMsgServer->SendMsg(iocDest, (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }

void GroupServer::sendRoleAssignmentMessage(SAFplus::Handle grpHandle,std::pair<EntityIdentifier,EntityIdentifier>& results)
  {
  EntityIdentifier data[2] = { results.first, results.second };  // Is this needed or is pair packed?

  char msgPayload[sizeof(GroupMessageProtocol)-1 + sizeof(EntityIdentifier)*2];
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;

  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_ROLE_NOTIFY;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_ACTIVE;
  sndMessage->force        = 1;
  memcpy(sndMessage->data,(const void*) data,sizeof(EntityIdentifier)*2);
  ClIocAddressT iocDest;
  iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  iocDest.iocPhyAddress.portId      = groupCommunicationPort;
  groupMsgServer->SendMsg(iocDest, (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }



/**
 * API to deregister an entity from the group
 */
void GroupServer::_deregister(GroupShmEntry* grp, unsigned int node, unsigned int port)
  {
  SAFplus::Wakeable* w=NULL;
  bool dirty = false;

  if (1)
    {
    if (grp->read().numMembers == 0) 
      {
      return;
      }

    GroupIdentity* gi = NULL;
    GroupData* data = &grp->write();

    memcpy(data, &grp->read(), sizeof(GroupData));  // copy the active data over to the new area
    // Now I can use the new data

    for (int i=0;i<data->numMembers; i++)  // If changing, have a care about the edge condition where there is just 1 member...
      {
      gi = &data->members[i];
      if ((gi->id.getNode() == node)&&((port==0)||(port == gi->id.getPort())))
        {
        logInfo("GMS","DER","Deregistering [%lx:%lx].", gi->id.id[0],gi->id.id[1]);
        dirty=true;
        // removing the active/standby
        if (data->activeIdx == i) { logInfo("GMS","DER","Leaving entity has standby role."); data->activeIdx=0xffff; }
        if (data->standbyIdx == i) { logInfo("GMS","DER","Leaving entity has active role."); data->standbyIdx=0xffff; } 

        if (data->numMembers > 1)  // If there is more than one member, then copy the last member into this one's slot to keep the array compact.
          {
          *gi = data->members[data->numMembers-1];
          if (data->activeIdx == data->numMembers-1) data->activeIdx = i;    // moving the active/standby in the array so have to update the index
          if (data->standbyIdx == data->numMembers-1) data->standbyIdx = i;
          }
        data->members[data->numMembers-1].id = INVALID_HDL;  // defensive programming: clear out the shared memory entry even if not really necessary
        data->numMembers--;
        i--; // Since I moved the last one into this slot, don't advance the loop since I need to evaluate the one I just copied in.  However, note that this loop must terminate because the end condition (numMembers) was reduced by one.
        }
      }

    if (dirty) 
      {
      data->changeCount++;
      data->lastChanged = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
      grp->flip();
      }
    }


  /* We need to other entities about the left node, but only AFTER
   * we have handled it. */
  if(w)
    {
    // TODO: w->wake(1,(void*)SAFplus::NODE_LEAVE_SIG);
    }
  }


/**
 * API to deregister an entity from the group
 */
void GroupServer::deregisterEntity(GroupShmEntry* grp, EntityIdentifier ent,bool needNotify)
  {
  SAFplus::Wakeable* w=NULL;
  bool dirty = false;

  if (1)
    {
    ScopedLock<ProcSem> lock(gsm.mutex);

    if (grp->read().numMembers == 0) 
      {
      return;  // Attempt to deregister a nonexistant entity
      }

    GroupIdentity* gi = NULL;
    GroupData* data = &grp->write();

    memcpy(data, &grp->read(), sizeof(GroupData));  // copy the active data over to the new area
    // Now I can use the new data

    for (int i=0;i<data->numMembers; i++)  // If changing, have a care about the edge condition where there is just 1 member...
      {
      gi = &data->members[i];
      if (gi->id == ent)
        {
        // removing the active/standby
        if (data->activeIdx == i) { logInfo("GMS","DER","Leaving entity has standby role."); data->activeIdx=0xffff; }
        if (data->standbyIdx == i) { logInfo("GMS","DER","Leaving entity has active role."); data->standbyIdx=0xffff; } 

        if (data->numMembers > 1)  // If there is more than one member, then copy the last member into this one's slot to keep the array compact.
          {
          *gi = data->members[data->numMembers-1];
          if (data->activeIdx == data->numMembers-1) data->activeIdx = i;    // moving the active/standby in the array so have to update the index
          if (data->standbyIdx == data->numMembers-1) data->standbyIdx = i;
          }
        data->members[data->numMembers-1].id = INVALID_HDL;  // defensive programming: clear out the shared memory entry even if not really necessary
        data->numMembers--;
        dirty = true;
        break;
        }
      }

    if (dirty) 
      {
      data->changeCount++;
      data->lastChanged = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
      grp->flip();
      }
    }


  /* We need to other entities about the left node, but only AFTER
   * we have handled it. */
  if(w)
    {
    // TODO: w->wake(1,(void*)SAFplus::NODE_LEAVE_SIG);
    }
  }


};
