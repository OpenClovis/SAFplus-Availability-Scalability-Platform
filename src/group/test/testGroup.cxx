// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clLogApi.hxx>
#include <clGroupApi.hxx>
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

  clTestCase(("GRP-FNC-REG.TC001: register and deregister groups"),testRegisterAndDeregister(0));
  clTestCase(("GRP-FNC-REG.TC002: change tracking"),testChanges());
  clTestCase(("GRP-FNC-REG.TC003: group messages"),testSendMessages());
  clTestGroupFinalize();
  safplusFinalize();
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
  logInfo("TEST","GRP", "[%d] Group [%" PRIx64 ":%" PRIx64 "] changed", changeCount, g->handle.id[0],g->handle.id[1]);

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
  printf("[%d] Received msg [%s]\n",id, (char*) msg);
  }

void testSendMessages()
  {
  Handle gh1 = Handle::create();
  Group grp(gh1);

  MyMsgHandler obj(1);
  Handle objHandle = Handle::create();
  SAFplus::objectMessager.insert(objHandle,&obj);

  // Register one entity and verify that it receives the proper messages

  grp.registerEntity(objHandle,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);

  if (1)
    {
    char buf[] = "test message";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_TO_ACTIVE);
    sleep(1);
    int tmp = obj.msgsRcvd;
    clTest(("send to active"), tmp == 1, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "test message 2";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_BROADCAST);
    sleep(1);
    int tmp = obj.msgsRcvd;
    clTest(("broadcast"), tmp == 2, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "test message 3";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    sleep(1);
    int tmp = obj.msgsRcvd;
    clTest(("round-robin"), tmp == 4, ("Got [%d]", tmp)); // Only one entity right now so RR should go to me twice
    }

  obj.msgsRcvd = 0;

  // Register another entity and show that both receive the proper messages

  MyMsgHandler obj2(2);
  Handle obj2Handle = Handle::create();
  SAFplus::objectMessager.insert(obj2Handle,&obj2);
  Group grp2(gh1);
  grp2.registerEntity(obj2Handle,2,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
  int tmp;
  if (1)
    {
    char buf[] = "test message active";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_TO_ACTIVE);
    sleep(1);
    tmp = obj.msgsRcvd;
    clTest(("send to active"), tmp == 1, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "test message standby";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_TO_STANDBY);
    sleep(1);
    tmp = obj2.msgsRcvd;
    clTest(("send to standby"), tmp == 1, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "test message broadcast";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_BROADCAST);
    sleep(1);
    int tmp = obj.msgsRcvd;
    clTest(("broadcast"), tmp == 2, ("Got [%d]", tmp));
    clTest(("broadcast"), (tmp=obj2.msgsRcvd) == 2, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "test message round robin";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    sleep(1);
    clTest(("round-robin"), (tmp=obj.msgsRcvd) == 3, ("Got [%d]", tmp)); // Only one entity right now so RR should go to me twice
    clTest(("round-robin"), (tmp=obj2.msgsRcvd) == 3, ("Got [%d]", tmp)); // Only one entity right now so RR should go to me twice
    }
  
  grp.deregister();  // force a fail over
  sleep(1);
  obj.msgsRcvd = 0;
  obj2.msgsRcvd = 0;

   if (1)
    {
    char buf[] = "post deregister send to active";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_TO_ACTIVE);
    sleep(1);
    clTest(("send to active"), (tmp=obj2.msgsRcvd) == 1, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "post deregister message broadcast";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_BROADCAST);
    sleep(1);
    clTest(("broadcast"), (tmp=obj2.msgsRcvd) == 2, ("Got [%d]", tmp));
    }

  if (1)
    {
    char buf[] = "post deregister round robin";
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    grp.send(buf,sizeof(buf),GroupMessageSendMode::SEND_LOCAL_ROUND_ROBIN);
    sleep(1);
    clTest(("round-robin"), (tmp=obj2.msgsRcvd) == 4, ("Got [%d]", tmp)); // Only one entity right now so RR should go to me twice
    }
   
  clTest(("nothing sent to deregistered entity"), (tmp=obj.msgsRcvd) == 0, ("Got [%d]", tmp));
 
  grp2.deregister();


  SAFplus::objectMessager.remove(objHandle);
  SAFplus::objectMessager.remove(obj2Handle);
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
  boost::this_thread::sleep(boost::posix_time::milliseconds(4000 + SAFplusI::GroupElectionTimeMs));
  int tmp = gch.changeCount;  // use temporary b/c var could be changed in separate thread
  clTest(("registration and election"), tmp == 2, ("Entity registration and election notice not received. Got only [%d] notifications", tmp));

  notifier.deregister();
  sleep(1);
  tmp = gch.changeCount;  // use temporary b/c var could be changed in separate thread
  clTest(("deregistration notification"), tmp == 3, ("Got only [%d] notifications, expected [3]", tmp));

  //assert(gch.changeCount == 2 || gch.changeCount == 3); // register, maybe elect (races with deregister), deregister = 2/3 changes

  gch.changeCount = 0;  // reset for next test

  if (1)
    {
    Group grpa1(gh1);
    grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
    Group grpa2(gh1);
    Handle e2 = Handle::create();
    grpa2.registerEntity(e2,2,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
    boost::this_thread::sleep(boost::posix_time::milliseconds(4000 + SAFplusI::GroupElectionTimeMs));
    // register, register will happen within one change, then elect
    tmp = gch.changeCount;  // use temporary b/c var could be changed in separate thread
    clTest(("double registration and election"), tmp == 2, ("Entity registration and election notice not received. Got only [%d] notifications", tmp));
    }

  }

int testRegisterAndDeregister(int mode)
{
  const std::string s = __FUNCTION__;

  Handle gh1 = Handle::create();

  int baselineGroups = SAFplusI::gsm.dbgCountGroups();
  int baselineEntities = SAFplusI::gsm.dbgCountEntities();
  Group grpa1(gh1);
  //GroupChangeHandler gch;
  //Group notifier(gh1);  // just for monitoring the other group's joining and leaving
  //notifier.setNotification(gch);

  sleep(1);
  int tmp;
  clTest(("register group"), (tmp=SAFplusI::gsm.dbgCountGroups()) == 1 + baselineGroups, ("Group registration miscompare: expected [%d] got [%d]", baselineGroups+1, tmp));
  clTest(("register group -- entities should be unchanged"), (tmp=SAFplusI::gsm.dbgCountEntities()) == baselineEntities, ("Group registration entity miscompare: expected [%d] got [%d]", baselineEntities, tmp));
  SAFplusI::gsm.dbgDump();  // should be just the one group.

  Handle e1 = Handle::create();
  grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  clTest(("register entity"), (tmp=SAFplusI::gsm.dbgCountEntities()) == baselineEntities + 1, ("Entity registration miscompare: expected [%d] got [%d]", baselineEntities + 1, tmp));
  SAFplusI::gsm.dbgDump();  // should be just the one group + one entity.

  grpa1.deregister();
  sleep(1);
  clTest(("deregister entity"), (tmp=SAFplusI::gsm.dbgCountEntities()) == baselineEntities, ("Entity registration miscompare: expected [%d] got [%d]", baselineEntities, tmp));
  SAFplusI::gsm.dbgDump();  // should be just the one group


  Handle e2 = Handle::create();
  grpa1.registerEntity(e1,1,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  Group grpa2(gh1);
  grpa2.registerEntity(e2,2,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  clTest(("reregister & register 2 entities"), (tmp=SAFplusI::gsm.dbgCountEntities()) == baselineEntities + 2, ("Entity registration miscompare: expected [%d] got [%d]", baselineEntities + 2, tmp));
  SAFplusI::gsm.dbgDump();  // should be just the one group + 2 entities.

  Handle gh2 = Handle::create();
  Group grpb1(gh2);
  Group grpb2(gh2);
  grpb1.registerEntity(e1,3,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  grpb2.registerEntity(e2,4,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE);
  sleep(1);
  clTest(("register another group"), (tmp=SAFplusI::gsm.dbgCountGroups()) == 2 + baselineGroups, ("Group registration miscompare: expected [%d] got [%d]", baselineGroups+2, tmp));
  clTest(("register 2 more entities in new group"), (tmp=SAFplusI::gsm.dbgCountEntities()) == baselineEntities + 4, ("Entity registration miscompare: expected [%d] got [%d]", baselineEntities + 4, tmp));

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
