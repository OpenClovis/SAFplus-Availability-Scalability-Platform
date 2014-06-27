#include <sys/types.h>
#include <unistd.h>

#include <clCommon.hxx>
#include <clIocPortList.hxx>

#include <clCkptIpi.hxx>
#include <clCkptApi.hxx>
#include <clMsgApi.hxx>
#include <clGroup.hxx>

#include <clCustomization.hxx>

using namespace SAFplus;
using namespace SAFplusI;

#define CKPT_GROUP_SUBHANDLE 1

class CkptSyncMsgHandler: public MsgHandler
{
  public:
  // checkpoint handle to pointer lookup...

  virtual void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
};

static CkptSyncMsgHandler msgHandler;

void CkptSyncMsgHandler::msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  logInfo("SYNC","MSG","Received checkpoint sync message from %d", from.iocPhyAddress.nodeAddress);
  }

void SAFplusI::CkptSynchronization::init(Checkpoint* c,MsgServer* msgSvr)
  {
  ckpt = c;
  if (msgSvr == NULL) msgSvr = &safplusMsgServer;
  msgSvr->RegisterHandler(CKPT_SYNC_MSG_TYPE,&msgHandler,NULL);
#if 0
  group = new SAFplus::Group();
  assert(group);
  group->init(ckpt->hdr->handle.getSubHandle(CKPT_GROUP_SUBHANDLE));
  group->registerEntity(ckpt->hdr->replicaHandle,SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE, false);
#endif
  syncThread = boost::thread(boost::ref(*this));
  }

void SAFplusI::CkptSynchronization::operator()()
  {
  while (1)
    {
    logInfo("SYNC","TRD","checkpoint synchronization thread");

    // Step one: initial synchronization
    

    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    }
  }

bool SAFplus::Checkpoint::electSynchronizationReplica()
{
  return true;
}
