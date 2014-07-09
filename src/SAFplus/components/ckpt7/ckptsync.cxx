#include <sys/types.h>
#include <unistd.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp> 

#include <boost/unordered_map.hpp>


#include <clCommon.hxx>
#include <clIocPortList.hxx>

#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#include <clMsgApi.hxx>
#include <clGroup.hxx>

#include <clCustomization.hxx>

#include "ckptprotocol.hxx"

using namespace SAFplus;
using namespace SAFplusI;

#define CKPT_GROUP_SUBHANDLE 1

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

  virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
};

static CkptSyncMsgHandler msgHdlr;

// Demultiplex incoming message to the appropriate Checkpoint object
void CkptSyncMsgHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  logInfo("SYNC","MSG","Received checkpoint sync message from %d", from.iocPhyAddress.nodeAddress);
  CkptMsgHdr* hdr = (CkptMsgHdr*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  CkptSynchronization* cs =  msgHdlr.handleMap[hdr->checkpoint];
  if (cs) cs->msgHandler(from, svr, msg, msglen, cookie);
  else
    {
    logInfo("SYNC","MSG","Unable to resolve handle [%lx:%lx] to a communications endpoint", hdr->checkpoint.id[0], hdr->checkpoint.id[1]);

    }
  }

void SAFplusI::CkptSynchronization::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  logInfo("SYNC","MSG","Resolved checkpoint sync message to checkpoint");

  CkptMsgHdr* hdr = (CkptMsgHdr*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  bool endianSwap = false;


  switch(hdr->msgType&0xffff)
    {
    case CKPT_MSG_TYPE_SYNC_REQUEST_1:
      {
      logInfo("SYNC","MSG","Checkpoint sync request message");
       CkptSyncRequest_1* req = (CkptSyncRequest_1*) msg;
       synchronize(req->generation, req->changeNum, req->cookie, req->response);

      } break;

    case CKPT_MSG_TYPE_SYNC_MSG_1:
      logInfo("SYNC","MSG","Received checkpoint sync update message");
      applySyncMsg(msg,msglen, cookie);
      break;

    case CKPT_MSG_TYPE_SYNC_COMPLETE_1:
      {
      CkptSyncCompleteMsg* sc = (CkptSyncCompleteMsg*) msg;
      if (endianSwap) {} // TODO
      while (syncRecvCount < sc->finalCount) // wait for other threads to finish processing
        {
        logInfo("SYNC","MSG","sync complete waiting for msg processing [%d] [%d]...", syncRecvCount, sc->finalCount);
        boost::this_thread::sleep(boost::posix_time::microseconds(100000));
        }
      ckpt->hdr->generation = sc->finalGeneration;
      ckpt->hdr->changeNum = sc->finalChangeNum;
      logInfo("SYNC","MSG","Checkpoint sync complete.  Msg count [%d].", sc->finalCount);

      ckpt->gate.open(); // TODO: I need to open this gate during all kinds of checkpoint failure conditions, sender process death, node death, message lost.
      } break;

    case CKPT_MSG_TYPE_SYNC_RESPONSE_1:
      logInfo("SYNC","MSG","Checkpoint sync response message");
      break;

    default:
      logInfo("SYNC","MSG","Unknown checkpoint update message [%d] length [%u]", hdr->msgType&255,(unsigned int) msglen);
      break;

    }
  }

void SAFplusI::CkptSynchronization::applySyncMsg(ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  CkptSyncMsg* hdr = (CkptSyncMsg*) msg;
  assert((hdr->msgType>>16) == CKPT_MSG_TYPE);  // TODO: endian swap & should never assert on bad incoming message data
  int curpos = sizeof(CkptSyncMsg);

  if (hdr->cookie != syncCookie)
    {
    logError("SYNC","MSG","Sync cookie mismatch.  Expected [0x%x]. Received [0x%x]", syncCookie, hdr->cookie);
    return;
    }

  int count = 0;
  while (curpos < msglen)
    {
    count++;
    Buffer* key = (Buffer*) (((char*)msg)+curpos);
    curpos += sizeof(Buffer) + key->len() - 1;
    Buffer* val = (Buffer*) (((char*)msg)+curpos);
    curpos += sizeof(Buffer) + val->len() - 1;
  
    logInfo("SYNC","APLY","part %d: key(len:%d) %x  val(len:%d) %x", count, key->len(), *((uint32_t*) key->data), val->len(), *((uint32_t*) val->data));
    ckpt->applySync(*key,*val);
    }

  if (syncRecvCount < hdr->count) syncRecvCount = hdr->count;
}

void SAFplusI::CkptSynchronization::synchronize(unsigned int generation, unsigned int lastChange, unsigned int cookie, Handle response)
{
  int count = 0;
  lastChange = 0; // DEBUG to force full sync

  if (generation != ckpt->hdr->generation) lastChange = 0;  // if the generation is wrong, we need ALL changes.
  int bufSize = 200 + SAFplusI::CkptSyncMsgStride;
  char* buf = new char[bufSize];

  CkptSyncMsg* hdr = (CkptSyncMsg*) buf;
  hdr->msgType = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_MSG_1;
  hdr->checkpoint = response;
  hdr->cookie = syncCookie;
  int headerEnds = sizeof(CkptSyncMsg);
  int offset = headerEnds;

  ClIocAddressT to = getAddress(response);

  // TODO: lock writes
  for (Checkpoint::Iterator i=ckpt->begin();i!=ckpt->end();i++)
    {
    SAFplus::Checkpoint::KeyValuePair& item = *i;
    Buffer* key = &(*(item.first));
    Buffer* val = &(*(item.second));

    if (key->changeNum() >= lastChange)
      {
      int dataSize = key->objectSize() + val->objectSize();

      // If we have data in the message and the next kvp would be bigger than the buffer then send the message out.
      if ((offset > headerEnds) && (offset + dataSize > bufSize))
        {
        // TODO unlock writes
        count++;
        logInfo("SYNC","MSG","Sending checkpoint update msg (length [%d]) to handle [%lx:%lx] location [%d:%d] type %d.  key %d %x  val len: %d %x", offset, response.id[0],response.id[1], to.iocPhyAddress.nodeAddress,to.iocPhyAddress.portId, CKPT_SYNC_MSG_TYPE, key->len(), *((uint32_t*) key->data), val->len(), *((uint32_t*) val->data));
        hdr->count = count;
        msgSvr->SendMsg(to,buf,offset,CKPT_SYNC_MSG_TYPE);
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
        offset = headerEnds;
        }
    
      assert(offset + key->objectSize() < bufSize);
      memcpy(&buf[offset],(void*) key, key->objectSize());
      offset += key->objectSize();

      assert(offset + val->objectSize() < bufSize);
      memcpy(&buf[offset],(void*) val, val->objectSize());
      offset += val->objectSize();
      }
    }

  // If we have data in the message and the next kvp would be bigger than the buffer then send the message out.
  if (offset > headerEnds)
      {
      // TODO unlock writes
      logInfo("SYNC","MSG","Sending last checkpoint update msg");
      count++;
      hdr->count = count;
      msgSvr->SendMsg(to,buf,offset,CKPT_SYNC_MSG_TYPE);
      offset = headerEnds;  // Reset the buffer
      // TODO lock writes
      }

  CkptSyncCompleteMsg sc;
  sc.finalCount      = count;  // TODO
  sc.finalGeneration = ckpt->hdr->generation;
  sc.finalChangeNum  = ckpt->hdr->changeNum;
  sc.msgType = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_COMPLETE_1;
  sc.checkpoint = response;
  msgSvr->SendMsg(to,&sc,sizeof(CkptSyncCompleteMsg),CKPT_SYNC_MSG_TYPE);
  // unlock writes

  delete buf;
}

void SAFplusI::CkptSynchronization::init(Checkpoint* c,MsgServer* pmsgSvr)
  {
  ckpt = c;

  if (pmsgSvr == NULL) msgSvr = &safplusMsgServer;
  else msgSvr = pmsgSvr;
  ckpt->hdr->replicaHandle = Handle::create(msgSvr->handle.getPort());
  group = new SAFplus::Group();
  assert(group);
  group->init(ckpt->hdr->handle.getSubHandle(CKPT_GROUP_SUBHANDLE),Group::DATA_IN_MEMORY);
  group->setNotification(*this);
  // The credential is most importantly the change number (so the latest changes becomes the master) and then the node number) 
  group->registerEntity(ckpt->hdr->replicaHandle,(ckpt->hdr->changeNum<<SAFplus::Log2MaxNodes) | SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE, false);

  logInfo("SYNC","TRD","Checkpoint is registered on [%d:%d] type [%d]", msgSvr->handle.getNode(),msgSvr->handle.getPort(),CKPT_SYNC_MSG_TYPE);

  msgHdlr.handleMap[ckpt->hdr->replicaHandle] = this;
  msgHdlr.handleMap[ckpt->hdr->handle] = this;
  msgSvr->RegisterHandler(CKPT_SYNC_MSG_TYPE,&msgHdlr,NULL);

  syncCount = -1;
  syncCookie = rand();  // Generate a random number so an ongoing sync to a failed node does not conflict with it restarting.

  syncThread = boost::thread(boost::ref(*this));
  }

void SAFplusI::CkptSynchronization::wake(int amt,void* cookie)
  {
  unsigned int reason = (unsigned long int) cookie;
  if (1) // reason == GROUP_EVENT)
    {
    logInfo("SYNC","GRP","Checkpoint group membership event");
    }
  else
    {
    assert(0);
    }
  }


void SAFplusI::CkptSynchronization::operator()()
  {
  if (1)
    {
    logInfo("SYNC","TRD","checkpoint synchronization thread");

    // Step one: initial synchronization
    Handle active = group->getActive();

    while (active == INVALID_HDL)
      {
      // TODO what here?  maybe call for election?
      std::pair<EntityIdentifier,EntityIdentifier> activeStandbyPairs = group->elect();
      active = activeStandbyPairs.first;
      }

    if (1)
      {
      CkptSyncRequest msg;
      msg.msgType    = (CKPT_MSG_TYPE << 16) | CKPT_MSG_TYPE_SYNC_REQUEST_1;
      msg.generation = ckpt->hdr->generation;
      msg.changeNum  = ckpt->hdr->changeNum;
      msg.checkpoint = ckpt->hdr->handle;
      msg.response   = ckpt->hdr->replicaHandle;

      syncCookie++;
      msg.cookie = syncCookie;
      syncCount=0;

      ClIocAddressT to = getAddress(active);
      msgSvr->SendMsg(to,&msg,sizeof(msg),CKPT_SYNC_MSG_TYPE);
      logInfo("SYNC","TRD","Sent synchronization request message to [%d:%d] type [%d]", active.getNode(), active.getPort(),CKPT_SYNC_MSG_TYPE);

      }

    boost::this_thread::sleep(boost::posix_time::milliseconds(150000));
    }
  }

bool SAFplus::Checkpoint::electSynchronizationReplica()
{
  return true;
}
