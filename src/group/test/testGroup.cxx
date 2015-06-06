// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clGroup.hxx>
#include <clGlobals.hxx>
#include <clNameApi.hxx>
#include <clGroupIpi.hxx>  // only for debug -- dumping the group shared memory
#include <clObjectMessager.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;
using namespace std;
//int testCapabilities(int mode);
//int testGroupElect(int mode);
//int testGetData(int mode);
//int testGroupIterator(int mode);
int testRegisterAndDeregister(int mode);
void testSendMessages();
void testChanges(void);

static unsigned int MAX_MSGS=25;
static unsigned int MAX_HANDLER_THREADS=2;

namespace SAFplusI
  {
  extern GroupSharedMem gsm;
  };

int main(int argc, char* argv[])
{
  logEchoToFd = 1;  // echo logs to stdout for debugging
  logSeverity = LOG_SEV_DEBUG;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 50;
  
  safplusInitialize(SAFplus::LibDep::GRP,sic);

  //safplusMsgServer.init(50, MAX_MSGS, MAX_HANDLER_THREADS);
  safplusMsgServer.Start();

  clTestGroupInitialize(("group"));

  testRegisterAndDeregister(0);
  //testChanges();
  testSendMessages();
  clTestGroupFinalize();
  return 0;
}

class GroupChangeHandler:public Wakeable
  {
  public:
  int changeCount;
  GroupChangeHandler():changeCount(0) {}
  void wake(int amt,void* cookie=NULL);
  };

void GroupChangeHandler::wake(int amt,void* cookie)
  {
  changeCount++;
  Group* g = (Group*) cookie;
  logInfo("TEST","GRP", "Group [%" PRIx64 ":%" PRIx64 "] changed", g->handle.id[0],g->handle.id[1]);

  Group::Iterator i;
  char buf[100];
  for (i=g->begin(); i != g->end(); i++)
    {
    const GroupIdentity& gid = i->second;
    logInfo("TEST","GRP", "  Entity [%" PRIx64 ":%" PRIx64 "] on node [%d] credentials [%" PRIu64 "] capabilities [%d] %s", gid.id.id[0],gid.id.id[1],gid.id.getNode(),gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
    }
  }

class MyMsgHandler:public SAFplus::MsgHandler
  {
  public:
  int id;
  int msgsRcvd;
  MyMsgHandler(int identity):id(identity),msgsRcvd(0) {}
  virtual void msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
  };

void MyMsgHandler::msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  msgsRcvd++;
  printf("%d: Received msg '%s'\n",id, (char*) msg);
  }

void testSendMessages()
  {
  Handle gh1 = Handle::create();
  Group grp(gh1);  // just for monitoring the other group's joining and leaving

  MyMsgHandler obj(1);
  Handle objHandle = Handle::create();
  SAFplus::objectMessager.insert(objHandle,&obj);

  grp.registerEntity(objHandle,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);

  if (1)
    {
    char buf[] = "test message";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_TO_ACTIVE);
    sleep(2);
    assert(obj.msgsRcvd == 1);
    }

  if (1)
    {
    char buf[] = "test message 2";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_BROADCAST);
    sleep(2);
    assert(obj.msgsRcvd == 2);
    }

  if (1)
    {
    char buf[] = "test message 3";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    sleep(2);
    assert(obj.msgsRcvd == 4);  // Only one entity right now so RR should go to me twice
    }

  SAFplus::objectMessager.remove(objHandle);
  }

void testChanges()
  {
  Handle gh1 = Handle::create();

  GroupChangeHandler gch;
  Group notifier(gh1);  // just for monitoring the other group's joining and leaving
  notifier.setNotification(gch);
  sleep(1);
  // test same group object register/deregister
  Handle e1 = Handle::create();
  notifier.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  notifier.deregister();
  sleep(1);
  assert(gch.changeCount == 2 || gch.changeCount == 3); // register, maybe elect (races with deregister), deregister = 2/3 changes

  gch.changeCount = 0;  // reset for next test

  if (1)
    {
    Group grpa1(gh1);
    grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
    Group grpa2(gh1);
    Handle e2 = Handle::create();
    grpa2.registerEntity(e2,2,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
    sleep(10);

    assert(gch.changeCount == 2); // register, register will happen within one change, then elect
    }

  }

int testRegisterAndDeregister(int mode)
{
  const std::string s = __FUNCTION__;

  Handle gh1 = Handle::create();

  Group grpa1(gh1);
  //GroupChangeHandler gch;
  //Group notifier(gh1);  // just for monitoring the other group's joining and leaving
  //notifier.setNotification(gch);

  sleep(1);
  int tmp;
  clTest(("register"), (tmp=SAFplusI::gsm.dbgCountGroups()) == 1, ("Group registration miscompare: expected [1] got [%d]", tmp));
  SAFplusI::gsm.dbgDump();  // should be just the one group.

  Handle e1 = Handle::create();
  grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  SAFplusI::gsm.dbgDump();  // should be just the one group + one entity.

  grpa1.deregister();
  sleep(1);
  SAFplusI::gsm.dbgDump();  // should be just the one group

  Handle e2 = Handle::create();
  grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  Group grpa2(gh1);
  grpa2.registerEntity(e2,2,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  SAFplusI::gsm.dbgDump();  // should be just the one group + 2 entities.

  Handle gh2 = Handle::create();
  Group grpb1(gh2);
  Group grpb2(gh2);
  grpb1.registerEntity(e1,3,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  grpb2.registerEntity(e2,4,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);

  logInfo("TEST","GRP", "Iterator Test");
  Group::Iterator i;
  char buf[100];
  for (i=grpb1.begin(); i != grpb1.end(); i++)
    {
    const GroupIdentity& gid = i->second;
    logInfo("TEST","GRP", "Entity [%" PRIx64 ":%" PRIx64 "] on node [%d] credentials [%" PRIu64 "] capabilities [%d] %s\n", gid.id.id[0],gid.id.id[1],gid.id.getNode(),gid.credentials, gid.capabilities, Group::capStr(gid.capabilities,buf));
    }

  SAFplusI::gsm.dbgDump();  // should be 2 groups + 2 entities.  The same handles can join multiple groups...

  //assert(0); // force failure without running deregister

  return 0;
}
