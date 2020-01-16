#include <clCustomization.hxx>
#include <clHandleApi.hxx>
#include <clProcessApi.hxx>
#include <clFaultApi.hxx>
#include <clGroupIpi.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <chrono>
#include <inttypes.h>

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

//ClRcT groupIocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie);
char groupSharedMemoryObjectName[256] = {0};

GroupServer::GroupServer()
 { 
 quit = false;
 groupCommunicationPort=GMS_IOC_PORT; 
 groupMsgServer = NULL;
 }

void GroupSharedMem::deleteSharedMemory()
  {
  shared_memory_object::remove(groupSharedMemoryObjectName);
  }

void GroupSharedMem::registerGroupObject(Group* grp)
  {
  ScopedLock<Mutex> lock2(localMutex);
  GroupChangeMap::iterator val = lastChange.find(grp->handle);

  // If the group does not exist in the local hash table, we need to look up the lastChange in shared memory
  if (val == lastChange.end())
    {
    lastChange[grp->handle].first = grp->lastChange();
    }

  lastChange[grp->handle].second.push_back(grp);
  }

void GroupSharedMem::deregisterGroupObject(Group* grp)
  {
  //GroupChangeMap::iterator val = lastChange.find(grp->handle);
  ScopedLock<Mutex> lock2(localMutex);
  lastChange[grp->handle].second.remove(grp);
  }

static void runDispatcher(void * gsm)
  {
  ((GroupSharedMem*) gsm)->dispatcher();
  }

// This thread wakes whenever any group changes and then wakes the interested groups
void GroupSharedMem::dispatcher()
  {
  while(!quit)
    {
    boost::this_thread::sleep(boost::posix_time::milliseconds(250));  // TODO: should be woken via a global sem...

    if (!quit)
      {
      ScopedLock<Mutex> lock2(localMutex);
      ScopedLock<ProcSem> lock(mutex);
      SAFplusI::GroupShmHashMap::iterator i;
      for (i=groupMap->begin(); i!=groupMap->end();i++)
        {
        SAFplus::Handle grpHdl = i->first;
        GroupShmEntry& ge = i->second;
        const GroupData& gd = ge.read();
      
        GroupChangeMap::iterator val = lastChange.find(grpHdl);  // Get our local last change
        if (val != lastChange.end()) // I am interested in changes in this group
          {
          if (gd.lastChanged != val->second.first)
            {
            val->second.first = gd.lastChanged;  // Ok will process all the callbacks for this change so set my local change count to the current one
            std::list<Group*>::iterator git = val->second.second.begin();
            for (; git != val->second.second.end(); git++)
              {
              if ((*git)->wakeable) 
                {
                mutex.unlock();
                (*git)->wakeable->wake(1,(void*) *git);
                mutex.lock();
                }
              }
            }
          }
        }
      }
    }
  }


unsigned int GroupSharedMem::dbgCountGroups(void)
{
  GroupShmHashMap::iterator i;
  assert(groupMap);
  ScopedLock<ProcSem> lock(mutex);

  int ret =0;
  for (i=groupMap->begin(); i!=groupMap->end();i++)
    {
      ret++;
    }
  return ret;
}

unsigned int GroupSharedMem::dbgCountEntities(void)
{
  GroupShmHashMap::iterator i;
  assert(groupMap);
  ScopedLock<ProcSem> lock(mutex);

  int ret =0;
  for (i=groupMap->begin(); i!=groupMap->end();i++)
    {
    SAFplus::Handle grpHdl = i->first;
    GroupShmEntry& ge = i->second;
    const GroupData& gd = ge.read();
    ret += gd.numMembers; 
    }
  return ret;
}



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
    printf("Group %s [%" PRIx64 ":%" PRIx64 "]:\n", ge.name,grpHdl.id[0], grpHdl.id[1]);
    for (int j = 0 ; j<gd.numMembers; j++)
      {
      const GroupIdentity& gid = gd.members[j];
      printf("    Entity [%" PRIx64 ":%" PRIx64 "] on node [%d] credentials [%ld] capabilities [%d] %s\n", gid.id.id[0],gid.id.id[1],gid.id.getNode(),(unsigned long int) gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
      }
    }
  }

  void GroupSharedMem::claim(int pid, int port)
  {
  ScopedLock<ProcSem> lock(mutex);
  groupHdr->rep  = pid;
  groupHdr->repPort = port;
  groupHdr->structId=SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
  
  }


void GroupSharedMem::clear()
  {
  ScopedLock<Mutex> lock2(localMutex);
  ScopedLock<ProcSem> lock(mutex);
  SAFplusI::GroupShmHashMap::iterator i;
  for (i=groupMap->begin(); i!=groupMap->end();i++)
    {
    SAFplus::Handle grpHdl = i->first;
    GroupShmEntry& ge = i->second;
    GroupData& gw = ge.write();
    gw.activeIdx = 0xffff;
    gw.standbyIdx = 0xffff;
    gw.numMembers = 0;
    ge.clearElectionFlag();
    ge.flip();
    }
  
  }

GroupSharedMem::~GroupSharedMem()
  {
    if (!quit) finalize();
  }

void GroupSharedMem::finalize()
  {
    quit = true;
    grpDispatchThread.join();
  }

void GroupSharedMem::init()
  {
  quit = false;
  mutex.init("GroupSharedMem",1);

  groupSharedMemoryObjectName[0] = 0;
  strncat(groupSharedMemoryObjectName, "SAFplusGroup", sizeof(groupSharedMemoryObjectName) - 1);
  logInfo("GRP", "INI", "Opening shared memory [%s]", groupSharedMemoryObjectName);


  ScopedLock<ProcSem> lock(mutex);
  groupMsm = boost::interprocess::managed_shared_memory(open_or_create, groupSharedMemoryObjectName, GroupSharedMemSize);
//  ScopedLock<ProcSem> lock(mutex);
  try
    {
    groupHdr = (SAFplusI::GroupShmHeader*) groupMsm.construct<SAFplusI::GroupShmHeader>("header") ();                                 // Ok it created one so initialize
    groupHdr->rep  = 0;
    groupHdr->repPort = 0;
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
          groupHdr->rep      = 0;
          groupHdr->repPort  = 0;
          groupHdr->structId = SAFplusI::CL_GROUP_BUFFER_HEADER_STRUCT_ID_7; // Initialize this last.  It indicates that the header is properly initialized (and acts as a structure version number)
        }
      }
    else throw;
    }
//  try
  //groupMap = new GroupShmHashMap();
  groupMap = groupMsm.find_or_construct<GroupShmHashMap>("groups")  (groupMsm.get_allocator<GroupMapPair>());
  // TODO assocData = groupMsm.find_or_construct<CkptHashMap>("data") ...
//    {
    //start dispatcher thread
    grpDispatchThread = boost::thread(runDispatcher, this);
  }

GroupServer::~GroupServer()
  {
    quit = true;
    if (groupMsgServer)
      groupMsgServer->removeHandler(SAFplusI::GRP_MSG_TYPE);  //  Register the main message handler (no-op if already registered)
    faultHandler.join();
  }

void GroupServer::init()
  {
    // TODO: use the already inited global GroupSharedMem rather than doing it twice. 
  gsm.init();

#if 0
      // If the data is valid and the controlling process is alive, this process is in conflict.  There can be only one GroupServer per node.
  if ((gsm.groupHdr->structId == CL_GROUP_BUFFER_HEADER_STRUCT_ID_7)&&(SAFplus::Process(gsm.groupHdr->rep).alive()))
        {
          throw SAFplus::Error(SAFplus::Error::SAFPLUS_ERROR, SAFplus::Error::EXISTS, "Group Node Representative already exists", __FILE__, __LINE__);
        }
#endif

  if(!groupMsgServer)
    {
    groupMsgServer = &safplusMsgServer;
    }
  groupCommunicationPort=groupMsgServer->port;

  if (electNodeRepresentative())
    {
      // I am node representative

      // Elect node representative also claims so this is not necessary
      //gsm.claim(SAFplus::pid,groupMsgServer->port);  // Make me the node representative

      gsm.clear();  // I am the node representative just starting up, so members may have fallen out while I was gone.  So I must delete everything I knew.
      //clIocNotificationRegister(groupIocNotificationCallback,this);

      if (1) //groupMsgServer->port == groupCommunicationPort) // If my listening port is the broadcast port then I must be the node representative
        {
          //allGrpMsgHdlr.handleMap[handle] = &groupMessageHandler;  //  Register this object's message handler into the global lookup.
          groupMsgServer->registerHandler(SAFplusI::GRP_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
        }
      faultHandler = boost::thread(boost::ref(*this));
    }
  else
    {
      // TODO: instead of raising an exception, I could start monitoring the shared memory to ensure that the group rep does not die (but may be redundant with the AMF).
      throw SAFplus::Error(SAFplus::Error::SAFPLUS_ERROR, SAFplus::Error::EXISTS, "Group Node Representative already exists", __FILE__, __LINE__);
    }

  }

  void GroupServer::operator() (void)
  {
    uint64_t faultChange=0;
    SAFplus::Fault fault;

    while(!quit)
      {
        boost::this_thread::sleep(boost::posix_time::milliseconds(250));  // TODO: should be woken via a global sem...
        // TODO this polling & searching should be replaced with explicit notification of what changed
        uint64_t tmp = fault.lastChange();
        if ((!quit)&&(faultChange != tmp))  // Fault manager changed; update groups
          {
            printf ("FAULT CHANGE\n");
            faultChange = tmp;
            //ScopedLock<Mutex> lock2(localMutex);
            //ScopedLock<ProcSem> lock(mutex);
            SAFplusI::GroupShmHashMap::iterator i;
            for (i=gsm.groupMap->begin(); i!=gsm.groupMap->end();i++)
              {
                GroupShmEntry& ge = i->second;
                const GroupData& gd = ge.read();
                for (int j=0;j<::SAFplusI::GroupMaxMembers;j++)
                  {
                    if (gd.members[j].id != INVALID_HDL)
                      {
                        FaultState fs = fault.getFaultState(gd.members[j].id);
                        if (fs == FaultState::STATE_DOWN)
                          {
                            //logInfo("FLT","MSG","Removing faulted [%d.%d.%" PRIx64 "] from group", gd.members[j].id.getNode(),gd.members[j].id.getProcess(), gd.members[j].id.getIndex());
                            printf("Removing faulted [%d.%d.%" PRIx64 "] from group", gd.members[j].id.getNode(),gd.members[j].id.getProcess(), gd.members[j].id.getIndex());
                            deregisterEntity(&ge,gd.members[j].id,true);
                            j=::SAFplusI::GroupMaxMembers;  // Not going to find a double
                          }
                      }
                  }
              }
          }
      }
  }

void GroupServer::removeEntities(SAFplus::Handle faultEntity)
{
  SAFplusI::GroupShmHashMap::iterator i;
  for (i=gsm.groupMap->begin(); i!=gsm.groupMap->end();i++)
   {
        GroupShmEntry& ge = i->second;
        const GroupData& gd = ge.read();
        for (int j=0;j<::SAFplusI::GroupMaxMembers;j++)
         {
             const SAFplus::Handle& ent = gd.members[j].id;
             if (ent != INVALID_HDL && ent.getNode() == faultEntity.getNode())
             {
                 logInfo("FLT","RMV","Removing faulted [%" PRIx64 ":%" PRIx64 "] from group", ent.id[0],ent.id[1]);
                 deregisterEntity(&ge,ent,true);
                 //logInfo("FLT","SET","set fault state of entity [%" PRIx64 ":%" PRIx64 "] as DOWN", ent.id[0],ent.id[1]);
                 //fault.setFaultState(ent,FaultState::STATE_DOWN);
                 j=::SAFplusI::GroupMaxMembers;
             }
        }
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
void GroupServer::msgHandler(Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  uint_t fromNode = from.getNode();
  //logInfo("SYNC","MSG","Received group message from node %d", fromNode);

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
  logInfo("GMS","MSG","Received message [0x%x] from node [%d]",(unsigned int) rxMsg->messageType,fromNode);

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
      if(fromNode != SAFplus::ASP_NODEADDR)
        {
        logDebug("GMS","MSG","Entity HELLO message");
        GroupIdentity *rxGrp = (GroupIdentity *)rxMsg->data;
        registerEntity(ge, rxGrp->id, rxGrp->credentials, NULL, rxGrp->dataLen, rxGrp->capabilities, false);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_ENTITY_LEAVE:
      {
      //mGroup->nodeLeaveHandle(rxMsg);
      EntityIdentifier *eId = (EntityIdentifier *)rxMsg->data;
      logDebug("GMS","MSG","Entity LEAVE message");
      deregisterEntity(ge, *eId,false);
      } break;
    case SAFplusI::GroupMessageTypeT::MSG_ROLE_NOTIFY:
      {
        //logDebug("GMS","MSG","Role CHANGE message");
      handleRoleNotification(ge, rxMsg);
      } break;
    case SAFplusI::GroupMessageTypeT::MSG_ELECT_REQUEST:
      if(!(ge->read().flags & GroupData::ELECTION_IN_PROGRESS)) // If we are not already in an election, then start one.  //from.iocPhyAddress.nodeAddress != clIocLocalAddressGet())
        {
          //logDebug("GMS","MSG","Election REQUEST message");
          handleElectionRequest(grpHandle);
        } break;
    case SAFplusI::GroupMessageTypeT::MSG_GROUP_ANNOUNCE:
      {
      // Nothing to do here, we've already created the group if it didn't exist.
      logDebug("GMS","MSG","Group announce message");
      } break;
    default:
      logDebug("GMS","MSG","Unknown message type [0x%x] from node [%d]",(unsigned int) rxMsg->messageType,fromNode);
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
      logDebug("GMS","MSG","Election request for group [%" PRIx64 ":%" PRIx64 "] named [%s]",grpHandle.id[0],grpHandle.id[1],ge->name);

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
    logInfo("GRP","ELC", "Member [%" PRIx64 ":%" PRIx64 "], capabilities: (%s) 0x%x, credentials: %" PRIu64,curEntity.id[0],curEntity.id[1],Group::capStr(curCapabilities,tempStr), curCapabilities, curCredentials);


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
          logError("GMS","ROL","Mismatched active role.  I calculated [%" PRIx64 ":%" PRIx64 "] but received [%" PRIx64 ":%" PRIx64 "]",roles.first.id[0],roles.first.id[1],announce[0].id[0],announce[0].id[1]);
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

        // Make data->lastChanged a monotonically increasing clock by generally using the time, but adding 1 in case the time is 
        // the same as the prior change time
        uint64_t temp = (uint64_t) std::chrono::steady_clock::now().time_since_epoch().count();
        if (temp <= data->lastChanged) data->lastChanged = temp+1;
        else data->lastChanged = temp;

        ge->flip();
        logInfo("GMS","ROL","Group [%s] [%" PRIx64 ":%" PRIx64 "] Role change announcement active [%" PRIx64 ":%" PRIx64 "] standby [%" PRIx64 ":%" PRIx64 "]", ge->name, grpHandle.id[0], grpHandle.id[1], announce[0].id[0],announce[0].id[1],announce[1].id[0],announce[1].id[1]);

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


#if 0 // TODO: component and node death to be reported by fault manager
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
        logDebug("GMS","IOC","  Checking group [%" PRIx64 ":%" PRIx64 "]", handle.id[0],handle.id[1]);
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
#endif


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
      logDebug("GMS", "REG","Entity [%" PRIx64 ":%" PRIx64 "] registration successful",me.id[0],me.id[1]);
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
  memset(msgPayload,0,sizeof(msgPayload)); // valgrind
  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_ELECT_REQUEST;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
  sndMessage->force        = 0;

  groupMsgServer->SendMsg(getProcessHandle(groupCommunicationPort,Handle::AllNodes), (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }

void GroupServer::sendHelloMessage(SAFplus::Handle grpHandle,const GroupIdentity& entityData)
  {
  char msgPayload[sizeof(GroupMessageProtocol)-1 + sizeof(GroupIdentity)];
  memset(&msgPayload,0,sizeof(GroupMessageProtocol)-1 + sizeof(GroupIdentity));  // valgrind
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;

  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_HELLO;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
  sndMessage->force        = 0;
  memcpy(sndMessage->data,(const void*) &entityData,sizeof(GroupIdentity));

  groupMsgServer->SendMsg(getProcessHandle(groupCommunicationPort,Handle::AllNodes), (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }

void GroupServer::sendRoleAssignmentMessage(SAFplus::Handle grpHandle,const std::pair<EntityIdentifier,EntityIdentifier>& results)
  {
  EntityIdentifier data[2] = { results.first, results.second };  // Is this needed or is pair packed?

  char msgPayload[sizeof(GroupMessageProtocol)-1 + sizeof(EntityIdentifier)*2];
  memset(&msgPayload,0,sizeof(GroupMessageProtocol)-1 + sizeof(EntityIdentifier)*2);  // valgrind
  GroupMessageProtocol *sndMessage = (GroupMessageProtocol *)&msgPayload;

  sndMessage->group        = grpHandle;
  sndMessage->messageType  = GroupMessageTypeT::MSG_ROLE_NOTIFY;
  sndMessage->roleType     = GroupRoleNotifyTypeT::ROLE_ACTIVE;
  sndMessage->force        = 1;
  memcpy(sndMessage->data,(const void*) data,sizeof(EntityIdentifier)*2);

  groupMsgServer->SendMsg(getProcessHandle(groupCommunicationPort,Handle::AllNodes), (void *)msgPayload, sizeof(msgPayload), SAFplusI::GRP_MSG_TYPE);
  }



/**
 * API to deregister an entity from the group
 */
void GroupServer::_deregister(GroupShmEntry* grp, unsigned int node, unsigned int port)
  {
  SAFplus::Wakeable* w=NULL;
  bool dirty = false;
  bool reelect = false;

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
      if ((gi->id.getNode() == node)&&((port==0)||(port == gi->id.getPort())))  // Ok we are deregistering this entity.  It matches the node or the node & port.
        {
        logInfo("GMS","DER","Deregistering [%" PRIx64 ":%" PRIx64 "].", gi->id.id[0],gi->id.id[1]);
        dirty=true;
        // removing the active/standby
        if (data->activeIdx == i) 
          { 
          logInfo("GMS","DER","Leaving entity had active role."); 
          data->activeIdx=data->standbyIdx;
          data->standbyIdx=0xffff; 
          reelect = true; // elect a new standby
          // set the capability on the new active to active  -- I don't have to remove cap from old active because it is deregistered
          data->members[data->activeIdx].capabilities = (data->members[data->activeIdx].capabilities & ~SAFplus::Group::IS_STANDBY) | SAFplus::Group::IS_ACTIVE;
          sendRoleAssignmentMessage(data->hdl,std::pair<EntityIdentifier,EntityIdentifier>(data->members[data->activeIdx].id, INVALID_HDL));
          }
        if (data->standbyIdx == i) { logInfo("GMS","DER","Leaving entity had standby role."); data->standbyIdx=0xffff; reelect = true; } 
        // TODO: if active fails should I promote the standby to active right here for rapid standby to active handling... what about notification of the change?

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

  const GroupData& gd = grp->read();
  // TODO: If the election is in progress, is it possible that the deregistered entity will be elected?  Check this...
  if (reelect&&(gd.flags&GroupData::AUTOMATIC_ELECTION)&&(!(gd.flags&GroupData::ELECTION_IN_PROGRESS)))
    {
    startElection(gd.hdl);
    }
  /* We need to inform other entities about the left node, but only AFTER we have handled it. */
  if(w)
    {
    // TODO: w->wake(1,(void*)SAFplus::NODE_LEAVE_SIG);
    }
  }

bool GroupServer::electNodeRepresentative(void)
  {
    SAFplusI::GroupShmHeader* hdr = this->gsm.groupHdr;
    assert(hdr);  // It must be nonzero because I would have created it if it did not exist.

  // This code elects a single process within this node to be the entity that handles synchronization messages.
  // It elects a process by checking a shared memory location for a pid.  If that pid is "alive" then that pid is elected.
  // Otherwise this process writes its own pid into the location, waits, and then checks to see if its write was not overwritten.
  // If its write remains valid, it wins the election.
  while (1)
    {
    Process p(hdr->rep);
    try
      {
      std::string cmd = p.getCmdline();
      return (hdr->rep == SAFplus::pid);  // If the process is alive we assume it is acting as sync replica... return true if that process is me otherwise false
      // TODO: this method suffers from a chance of a process respawn
      }
    catch (ProcessError& e)  // process in hdr->serverPid is dead, so put myself in there.
      {
      hdr->rep = SAFplus::pid;
      hdr->repPort = groupCommunicationPort;
      hdr->repWatchdog = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      logAlert("GMS","REP","This process is claiming group node representative, on port %d at beat %u",hdr->repPort, (unsigned int) hdr->repWatchdog); 
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      if (hdr->rep == SAFplus::pid) return true;
      // Otherwise I will wrap around, reload the PID and make sure that it is valid.  This will presumably solve theoretical write collisions which corrupt data due to different writes succeeding in different bytes in the number.
      }
    }  
  }

/**
 * API to deregister an entity from the group
 */
void GroupServer::deregisterEntity(GroupShmEntry* grp, EntityIdentifier ent,bool needNotify)
  {
  SAFplus::Wakeable* w=NULL;
  bool dirty = false;
  bool reelect = false;

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
        if (data->activeIdx == i) 
          { 
          logInfo("GMS","DER","Leaving entity has active role."); 
          data->activeIdx=data->standbyIdx;
          data->standbyIdx=0xffff; 
          reelect = true; // elect a new standby
          // set the capability on the new active to active  -- I don't have to remove cap from old active because it is deregistered
          data->members[data->activeIdx].capabilities = (data->members[data->activeIdx].capabilities & ~SAFplus::Group::IS_STANDBY) | SAFplus::Group::IS_ACTIVE;
          sendRoleAssignmentMessage(data->hdl,std::pair<EntityIdentifier,EntityIdentifier>(data->members[data->activeIdx].id, INVALID_HDL));
          reelect = true;
          }

        if (data->standbyIdx == i) { logInfo("GMS","DER","Leaving entity has standby role."); data->standbyIdx=0xffff; reelect=true; } 

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

  const GroupData& gd = grp->read();
  // TODO: If the election is in progress, is it possible that the deregistered entity will be elected?  Check this...
  if (reelect&&(gd.flags&GroupData::AUTOMATIC_ELECTION)&&(!(gd.flags&GroupData::ELECTION_IN_PROGRESS)))
    {
    startElection(gd.hdl);
    }

  /* We need to other entities about the left node, but only AFTER
   * we have handled it. */
  if(w)
    {
    // TODO: w->wake(1,(void*)SAFplus::NODE_LEAVE_SIG);
    }
  }


};
