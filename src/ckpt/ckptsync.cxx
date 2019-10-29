#include <sys/types.h>
#include <unistd.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 

#include <boost/unordered_map.hpp>

#include <cltypes.h>
#include <clCommon.hxx>
#include <clMsgPortsAndTypes.hxx>

#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#include <clMsgApi.hxx>
#include <clGroupApi.hxx>
#include <clProcessApi.hxx>
#include <clObjectMessager.hxx>

#include <clCustomization.hxx>

#include "ckptprotocol.hxx"

using namespace SAFplus;
using namespace SAFplusI;

#define CKPT_GROUP_SUBHANDLE 1

#define TEMP_CKPT_SYNC_PORT 27 // TODO: this needs to be replaced with a more sophisticated system

enum 
{
  GROUP_EVENT = 0x123
};

typedef boost::unordered_map<SAFplus::Handle, CkptSynchronization*> CkptHandleMap;

class CkptSyncMsgHandler: public MsgHandler
{
  public:
  // checkpoint handle to pointer lookup...
  CkptHandleMap handleMap;

  virtual void msgHandler(Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
};

static CkptSyncMsgHandler msgHdlr;

// Demultiplex incoming message to the appropriate Checkpoint object
void CkptSyncMsgHandler::msgHandler(Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  //logInfo("SYNC","MSG","Received checkpoint sync message from %d", from.iocPhyAddress.nodeAddress);
  CkptMsgHdr* hdr = (CkptMsgHdr*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  CkptSynchronization* cs =  msgHdlr.handleMap[hdr->checkpoint];
  if (cs) cs->msgHandler(from, svr, msg, msglen, cookie);
  else
    {
    logInfo("SYNC","MSG","Unable to resolve handle [%" PRIx64 ":%" PRIx64 "] to a communications endpoint", hdr->checkpoint.id[0], hdr->checkpoint.id[1]);

    }
  }

void SAFplusI::CkptSynchronization::msgHandler(Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  //logInfo("SYNC","MSG","Resolved checkpoint sync message to checkpoint");

  CkptMsgHdr* hdr = (CkptMsgHdr*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  bool endianSwap = false;


  switch(hdr->msgType&0xffff)
    {
    case CKPT_MSG_TYPE_SYNC_REQUEST_1:
      {
        //logInfo("SYNC","MSG","Checkpoint sync request message");
      CkptSyncRequest_1* req = (CkptSyncRequest_1*) msg;
      synchronize(req->generation, req->changeNum, req->cookie, req->response);
      } break;

    case CKPT_MSG_TYPE_SYNC_MSG_1:
      {
      CkptSyncMsg* hdr = (CkptSyncMsg*) msg;
      assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
      logInfo("SYNC","MSG","Received checkpoint sync update message [%d] of length [%d] for [%" PRIx64 ":%" PRIx64"].",hdr->count, (int) msglen, ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1]);

      // The sync cookie verifies that this synchronization message is part of THIS sync operation.  
      // Theoretically, an old sync message might be delayed and then appear from an old sync request.  The sync cookie allows us to detect and ignore those old
      // messages.  I suppose that applying them would not mess anything up, b/c they should be valid data... (except perhaps the sync message count).
      if (hdr->cookie != syncCookie)
        {
        logError("SYNC","MSG","Sync cookie mismatch.  Expected [0x%x]. Received [0x%x]", syncCookie, hdr->cookie);
        }
      else 
        {
        applySyncMsg(msg,msglen, cookie);
        // At this point I do not want to apply the change number to the checkpoint header because the initial synchronization
        // sends the records in any order.  So I might be applying change 100 (say) but have not yet received changes 10-99.
        // Therefore if this process fails, I need to resynchronize starting at the original change number.  So I can't update the change # in shared memory
        // until the entire synchronization process is completed.
        syncRecvCount++;
#if 0
        if (syncRecvCount < hdr->count)
          {
            syncRecvCount = hdr->count;  // TODO... should I be assigning this or just adding one... assignment would cause a dropped packet to be not detected.
          }
#endif
        }

      } break;

    case CKPT_MSG_TYPE_UPDATE_MSG_1:
      if (from.getNode() != SAFplus::ASP_NODEADDR) // No need to handle update messages coming from myself.
        {
        unsigned int change = applySyncMsg(msg,msglen, cookie);
        logInfo("SYNC","MSG","Received checkpoint update message for [%" PRIx64 ":%" PRIx64"].  My last change [%d] incoming [%d]", ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1],ckpt->hdr->changeNum, change);
        if (ckpt->hdr->changeNum < change) ckpt->hdr->changeNum = change;
        } break;

    case CKPT_MSG_TYPE_DELETE_MSG_1:
      if (from.getNode() != SAFplus::ASP_NODEADDR) // No need to handle update messages coming from myself.
        {
        applyDeleteMsg(msg,msglen, cookie);
        logInfo("SYNC","MSG","Received checkpoint delete message for [%" PRIx64 ":%" PRIx64"].", ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1]);
        } break;

    case CKPT_MSG_TYPE_SYNC_COMPLETE_1:
      {
      CkptSyncCompleteMsg* sc = (CkptSyncCompleteMsg*) msg;
      if (endianSwap) {} // TODO
      // Presumably sync complete is sent last.  However other threads could still be processing the incoming messages, or the order
      // could have been mixed up in transit.  So wait for all the synchronization messages to arrive and be processed.
      int loop=0;
      while ((syncRecvCount < sc->finalCount)&&(loop < 20))
        {
        loop++;
        logInfo("SYNC","MSG","sync complete waiting for msg processing received [%d] expected [%d]...", syncRecvCount, sc->finalCount);
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        }
      if (loop == 20)
        {
        logInfo("SYNC","MSG","Checkpoint [%" PRIx64 ":%" PRIx64 "] sync failed.  Msg count [%d]. change [%d.%d]", ckpt->hdr->handle.id[0], ckpt->hdr->handle.id[1], sc->finalCount, ckpt->hdr->generation,ckpt->hdr->changeNum);
        syncRequestActive = INVALID_HDL; // Will trigger a resync request
        }
      else
        {
          ckpt->hdr->generation = sc->finalGeneration;
          if (ckpt->hdr->changeNum < sc->finalChangeNum) ckpt->hdr->changeNum = sc->finalChangeNum;
          logInfo("SYNC","MSG","Checkpoint [%" PRIx64 ":%" PRIx64 "] sync complete.  Msg count [%d].  Record count [%d]. change [%d.%d]", ckpt->hdr->handle.id[0], ckpt->hdr->handle.id[1], sc->finalCount, syncRecordCount, ckpt->hdr->generation,ckpt->hdr->changeNum);
          atomize.lock();
          synchronizing = false;
          ckpt->gate.open(); // TODO: I need to open this gate during all kinds of checkpoint failure conditions, sender process death, node death, message lost.
          atomize.unlock();
        }
      } break;

    case CKPT_MSG_TYPE_SYNC_RESPONSE_1:
      logInfo("SYNC","MSG","Checkpoint sync response message");
      break;

    default:
      logInfo("SYNC","MSG","Unknown checkpoint update message [%d] length [%u]", hdr->msgType&255,(unsigned int) msglen);
      break;

    }
  }

// Apply a synchronization message received either during the initial sync operation OR during a normal sync update (happens when the checkpoint is written)
unsigned int SAFplusI::CkptSynchronization::applySyncMsg(ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  ScopedLock<> lock(atomize);

  bool gated=false;
  CkptSyncMsg* hdr = (CkptSyncMsg*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  int curpos = sizeof(CkptSyncMsg);
  unsigned int lastChange = 0;
  int count = 0;

  while (curpos < msglen)
    {
    gated = false;
    syncRecordCount++;
    count++;
    Buffer* key = (Buffer*) (((char*)msg)+curpos);
    curpos += key->objectSize();
    Buffer* val = (Buffer*) (((char*)msg)+curpos);
    curpos += val->objectSize();
    if (lastChange < val->changeNum()) lastChange = val->changeNum();
    logInfo("SYNC","APLY","[%" PRIx64 ":%" PRIx64"]: part %d: change %d. key(len:%d) %x  val(len:%d) %x", ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1], count, val->changeNum(), key->len(), *((uint32_t*) key->data), val->len(), *((uint32_t*) val->data));
    if (!synchronizing) { gated = true; ckpt->gate.close(); }  // In the initial sync case, the gate is already closed.
    if(ckpt->notificationCallback != NULL)
    {
		//logInfo("SYNC","APPLY", "start ckpt->notificationCallback");
		ckpt->notificationCallback(*key,*val,ckpt->notifCallBackParam);
	}
    ckpt->applySync(*key,*val);
    if (gated) ckpt->gate.open();
    }

  return lastChange;
}

void SAFplusI::CkptSynchronization::applyDeleteMsg(ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  ScopedLock<> lock(atomize);

  bool gated=false;
  CkptSyncMsg* hdr = (CkptSyncMsg*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  int curpos = sizeof(CkptSyncMsg);
  int count = 0;

  while (curpos < msglen)
    {
    gated = false;
    syncRecordCount++;
    count++;
    Buffer* key = (Buffer*) (((char*)msg)+curpos);
    curpos += key->objectSize();
    Buffer* val = (Buffer*) (((char*)msg)+curpos);
    curpos += val->objectSize();
    logInfo("DEL","APLY","[%" PRIx64 ":%" PRIx64"]: part %d: key(len:%d) %x  val(len:%d) %x", ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1], count, key->len(), *((uint32_t*) key->data), val->len(), *((uint32_t*) val->data));
    if (!synchronizing) { gated = true; ckpt->gate.close(); }  // In the initial sync case, the gate is already closed.
    ckpt->applyDelete(*key,*val);
    if (gated) ckpt->gate.open();
    }
}

void SAFplusI::CkptSynchronization::sendUpdate(const Buffer* key,const Buffer* val,int change, Transaction& t)
  {
  // TODO transactions
  //int dataSize = key->objectSize() + val->objectSize();
  int bufSize = sizeof(CkptSyncMsg) + key->objectSize() + val->objectSize();  
  char* buf = new char[bufSize];  // TODO allocate a message buffer

  CkptSyncMsg* hdr = (CkptSyncMsg*) buf;
  memset(hdr,0,sizeof(CkptSyncMsg)); // valgrind
  if (change)
  {
    hdr->msgType     = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_UPDATE_MSG_1;
  }
  else  // deleting ckpt data doesn't take change
  {
    hdr->msgType     = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_DELETE_MSG_1;
  }
  hdr->checkpoint  = ckpt->handle();
  hdr->cookie      = syncCookie;
  hdr->count       = change;
  int offset   = sizeof(CkptSyncMsg);

  memcpy(&buf[offset],(void*) key, key->objectSize());
  offset += key->objectSize();

  assert(offset + val->objectSize() <= bufSize);
  memcpy(&buf[offset],(void*) val, val->objectSize());
  offset += val->objectSize();
  assert(offset <= bufSize);

  group->send(buf,offset,GroupMessageSendMode::SEND_BROADCAST);
  delete [] buf;
#if 0
  Handle to;
  to.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
  to.iocPhyAddress.portId      = TEMP_CKPT_SYNC_PORT;  // This is a problem b/c the syncPort is not known.  TODO: actually use a group API to send to every registered entity in the group.  Inside group, message is sent directly to each "handle" using the object message API (TBD)
  msgSvr->SendMsg(to,buf,offset,CKPT_SYNC_MSG_TYPE);
#endif
  }

// Bring some other replica into sync with this one.
void SAFplusI::CkptSynchronization::synchronize(unsigned int generation, unsigned int lastChange, unsigned int cookie, Handle response)
{
  int count = 0;
  //lastChange = 0; // DEBUG to force full sync

  //Handle to = getAddress(response);

  logInfo("SYNC","MSG","Handling synchronization request from [%" PRIx64 ":%" PRIx64 "] for [%" PRIx64 ":%" PRIx64 "], generation [%d] change [%d] ",response.id[0],response.id[1], ckpt->hdr->handle.id[0],ckpt->hdr->handle.id[1], generation, lastChange);
  if (generation != ckpt->hdr->generation) 
    {
    lastChange = 0;  // if the generation is wrong, we need ALL changes.
    logInfo("SYNC","MSG","Generation mismatch theirs is [%d] vs mine [%d].  Sending the entire checkpoint.",generation, ckpt->hdr->generation);
    }
  int bufSize = 200 + SAFplusI::CkptSyncMsgStride;
  char* buf = new char[bufSize];

  CkptSyncMsg* hdr = (CkptSyncMsg*) buf;
  hdr->msgType = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_MSG_1;
  hdr->checkpoint = response;
  hdr->cookie = cookie;
  int headerEnds = sizeof(CkptSyncMsg);
  int offset = headerEnds;


  // TODO: lock writes
  int itemCount = 0;
  for (Checkpoint::Iterator i=ckpt->begin();i!=ckpt->end();i++)
    {
    itemCount++;
    SAFplus::Checkpoint::KeyValuePair& item = *i;
    Buffer* key = &(*(item.first));
    Buffer* val = &(*(item.second));

    if (val->changeNum() > lastChange)
      {
      int dataSize = key->objectSize() + val->objectSize();

      // If we have data in the message and the next kvp would be bigger than the buffer then send the message out.
      if ((offset > headerEnds) && (offset + dataSize > bufSize))
        {
        // TODO unlock writes
        count++;
        logInfo("SYNC","MSG","Sending checkpoint update msg [%d] (length [%d]) to handle [%" PRIx64 ":%" PRIx64 "] type %d.", count, offset, response.id[0],response.id[1], CKPT_SYNC_MSG_TYPE);
        hdr->count = count;
        msgSvr->SendMsg(response,buf,offset,CKPT_SYNC_MSG_TYPE);
        offset = headerEnds;  // Reset the buffer
        // TODO lock writes
        }

      // Make the buffer big enough if it can't fit.
      if (dataSize + offset > bufSize)
        {
        delete buf;
        bufSize = key->objectSize() + val->objectSize() + offset;
        buf = new char[bufSize];
        assert(buf);
        // Reinitialize the header data
        hdr = (CkptSyncMsg*) buf;
        hdr->msgType = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_MSG_1;
        hdr->checkpoint = response;
        hdr->cookie = cookie;
        offset = headerEnds;
        }

      logInfo("SYNC","MSG", "adding record of key length [%d] (object [%d]) val length [%d] (%d) change [%d]", key->len(), key->objectSize(), val->len(), val->objectSize(),val->changeNum());
      assert(offset + key->objectSize() <= bufSize);
      memcpy(&buf[offset],(void*) key, key->objectSize());
      offset += key->objectSize();

      assert(offset + val->objectSize() <= bufSize);
      memcpy(&buf[offset],(void*) val, val->objectSize());
      offset += val->objectSize();
      }
    else
      {
        logDebug("SYNC","MSG", "skipping record of key length [%d] val length [%d] change [%d]", key->objectSize(), val->objectSize(),val->changeNum());      
      }
    }

  // We have data in the message so send the last one out.
  if (offset > headerEnds)
      {
      // TODO unlock writes
      logInfo("SYNC","MSG","Sending last checkpoint update msg ");
      count++;
      hdr->count = count;
      msgSvr->SendMsg(response,buf,offset,CKPT_SYNC_MSG_TYPE);
      offset = headerEnds;  // Reset the buffer
      // TODO lock writes
      }

  logInfo("SYNC","MSG","Iterated over [%d] items checkpoint size [%d]", itemCount, ckpt->size());

  CkptSyncCompleteMsg sc;
  sc.finalCount      = count;
  sc.finalGeneration = ckpt->hdr->generation;
  sc.finalChangeNum  = ckpt->hdr->changeNum;
  sc.msgType = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_COMPLETE_1;
  sc.checkpoint = response;
  msgSvr->SendMsg(response,&sc,sizeof(CkptSyncCompleteMsg),CKPT_SYNC_MSG_TYPE);
  // unlock writes

  delete buf;
}

void SAFplusI::CkptSynchronization::init(Checkpoint* c,MsgServer* pmsgSvr, SAFplus::Wakeable& execSemantics)
  {
  ckpt = c;
  initialized = true;
  synchronizing = true;  // Checkpoint always starts out in the synchronizing state (access not allowed).  It kicks out of this state when it becomes the active replica or it is synced with the active replica.

  syncReplica = electSynchronizationReplica();

  if (pmsgSvr == NULL) msgSvr = &safplusMsgServer;
  else msgSvr = pmsgSvr;
  ckpt->hdr->replicaHandle = Handle::create(msgSvr->handle.getPort());

  // Install this object as the object message handler so we can handle incoming update messages sent to all members of the group.
  SAFplus::objectMessager.insert(ckpt->hdr->replicaHandle,this);

  // Now join this checkpoint's dedicated group.
  group = new SAFplus::Group();
  assert(group);
  group->init(ckpt->hdr->handle.getSubHandle(CKPT_GROUP_SUBHANDLE),SAFplusI::GMS_IOC_PORT,execSemantics);

  if (syncReplica)  // Only one process per node is in charge of handling replication messages
    {
    if (ckpt->name.size()) group->setName(ckpt->name.c_str());
    group->setNotification(*this);
    // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
    group->registerEntity(ckpt->hdr->replicaHandle,(ckpt->hdr->changeNum<<SAFplus::Log2MaxNodes) | SAFplus::ASP_NODEADDR,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);

    logInfo("SYNC","TRD","Checkpoint is registered on [%d:%d] type [%d]", msgSvr->handle.getNode(),msgSvr->handle.getPort(),CKPT_SYNC_MSG_TYPE);

    msgHdlr.handleMap[ckpt->hdr->replicaHandle] = this;
    msgHdlr.handleMap[ckpt->hdr->handle] = this;
    msgSvr->RegisterHandler(CKPT_SYNC_MSG_TYPE,&msgHdlr,NULL);

    syncCount = -1;
    syncCookie = rand();  // Generate a random number so an ongoing sync to a failed node does not conflict with it restarting.

    syncThread = boost::thread(boost::ref(*this));
    }
  else
    {
    synchronizing = false;
    ckpt->gate.open(); // If this checkpoint becomes the master replica, it is by definition the authorative copy so allow reading/writing
    }
  }

SAFplusI::CkptSynchronization::~CkptSynchronization() { finalize(); }

void SAFplusI::CkptSynchronization::finalize()
{
  if (initialized)
    {
      initialized = false;
      if (synchronizing)
        {
          synchronizing = false;
          syncThread.join();
        }
      delete group;
      group = NULL;
    }
}

void SAFplusI::CkptSynchronization::wake(int amt,void* cookie)
  {
  unsigned int reason = (unsigned long int) cookie;
  if (cookie == group)
    {
    logInfo("SYNC","GRP","Checkpoint group membership event");
    std::pair<EntityIdentifier,EntityIdentifier> roles = group->getRoles();
    if (roles.first == ckpt->hdr->replicaHandle)
      {
      logInfo("SYNC","GRP","Checkpoint is master replica");
      ScopedLock<> lock(atomize);
      if (synchronizing)
        {
        synchronizing = false;
        ckpt->gate.open(); // If this checkpoint becomes the master replica, it is by definition the authorative copy so allow reading/writing
        }
      }
    }
  else
    {
    assert(0);
    }
  }

SAFplusI::CkptSynchronization::CkptSynchronization()
{
  initialized = false;
  synchronizing = false;
  syncReplica = false;
  ckpt = NULL;
  group = NULL;
  msgSvr = NULL;
  syncMsgSize = 0;
  syncCount = 0;
  syncRecvCount= 0;
  syncCookie = 0;
}

void SAFplusI::CkptSynchronization::operator()()
  {

  while(synchronizing)
    {
    logInfo("SYNC","TRD","checkpoint synchronization thread");

    // Step one: initial synchronization
    Handle active;
    do
      {
      assert(group);
      active = group->getActive();
      if (active == INVALID_HDL) // TODO should be a group wake API
        {
        boost::this_thread::sleep(boost::posix_time::milliseconds(250));
        }
      } while (active == INVALID_HDL);
    
    if (active != ckpt->hdr->replicaHandle) // Only request synchronization if I am not active
      {
        if (active != syncRequestActive)  // and the active has changed (or first time)
          {
            syncRequestActive = active;
            CkptSyncRequest msg;
            msg.msgType    = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_REQUEST_1;
            msg.generation = ckpt->hdr->generation;
            msg.changeNum  = ckpt->hdr->changeNum;
            msg.checkpoint = ckpt->hdr->handle;
            msg.response   = ckpt->hdr->replicaHandle;

            syncCookie++;
            msg.cookie = syncCookie;
            syncCount=0;
            syncRecordCount = 0;
            syncRecvCount = 0;
            msgSvr->SendMsg(active,&msg,sizeof(msg),CKPT_SYNC_MSG_TYPE);
            logInfo("SYNC","TRD","Sent synchronization request message to [%d:%d] type [%d] generation [%d] change [%d] sync cookie [%x]", active.getNode(), active.getPort(),CKPT_SYNC_MSG_TYPE,msg.generation, msg.changeNum, syncCookie);
          }
        }
    else 
      {
        ScopedLock<> lock(atomize);
        if (synchronizing)
          {
          synchronizing = false;
          ckpt->gate.open();
          break; // I am active so am the authorative copy
          }
      }

    // Repeat a sync request every 2 seconds in case the checkpoint master fails
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    }
  }

bool SAFplusI::CkptSynchronization::electSynchronizationReplica()
{
  // This code elects a single process within this node to be the entity that handles synchronization messages.
  // It elects a process by checking a shared memory location for a pid.  If that pid is "alive" then that pid is elected.
  // Otherwise this process writes its own pid into the location, waits, and then checks to see if its write was not overwritten.
  // If its write remains valid, it wins the election.
  while (1)
    {
    Process p(ckpt->hdr->serverPid);
    try
      {
      std::string cmd = p.getCmdline();
      return (ckpt->hdr->serverPid == SAFplus::pid);  // If the process is alive we assume it is acting as sync replica... return true if that process is me otherwise false
      // TODO: this method suffers from a chance of a process respawn
      }
    catch (ProcessError& e)  // process in hdr->serverPid is dead, so put myself in there.
      {
      ckpt->hdr->serverPid = SAFplus::pid;
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      if (ckpt->hdr->serverPid == SAFplus::pid) return true;
      // Otherwise I will wrap around, reload the PID and make sure that it is valid.  This will presumably solve theoretical write collisions which corrupt data due to different writes succeeding in different bytes in the number.
      }
    }
}
