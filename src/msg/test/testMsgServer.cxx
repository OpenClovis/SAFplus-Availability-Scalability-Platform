#include <clMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;

class xorshf96
  {
public:
  unsigned long x, y, z;
 
  xorshf96(unsigned long seed=123456789)
    {
    x=seed, y=362436069, z=521288629;
    }

  unsigned long operator()(void) 
    {          //period 2^96-1
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z;
    }
  };


#define MAX_MSG 1000

class RecvHandler:public SAFplus::MsgHandler, public SAFplus::ThreadSem
  {
  public:
  virtual void msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
  char data[MAX_MSG];
  int len;
  };

void RecvHandler::msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
  {
  // NOTE: this is a bad example. If more than one message is sent the first might be overwritten by subsequent calls.
  // DO NOT USE in real code.
  assert(msglen < MAX_MSG);
  memcpy(&data,msg,msglen);
  len = msglen;
  unlock(1);
  }

bool testSendRecv()
  {
  MsgServer a(1,10,2);
  MsgServer b(2,10,2);

  const char* strMsg = "This is a test of message sending";

  RecvHandler receiver;
  b.RegisterHandler(1,&receiver,NULL);
  b.Start();

  a.SendMsg(b.handle,(void*) strMsg,strlen(strMsg),1);
  receiver.lock();
  printf("Message was: %s\n",receiver.data);
  clTest(("send/recv message ok"), 0 == strncmp((const char*) receiver.data,strMsg,sizeof(strMsg)),("message contents miscompare") );

  RecvHandler receiver2;
  b.RegisterHandler(2,&receiver2,NULL);
  uint_t val = 2;
  a.SendMsg(b.handle,(void*) &val,sizeof(val),2);
  val = 1;
  a.SendMsg(b.handle,(void*) &val,sizeof(val),1);

  receiver.lock();
  clTest(("msgType demultiplex 1"), *((uint_t*) receiver.data) == 1, ("data is [%d]",*((uint_t*) receiver.data) ));
  receiver2.lock();
  clTest(("msgType demultiplex 2"), *((uint_t*) receiver2.data) == 2, ("data is [%d]",*((uint_t*) receiver2.data) ));

  // Test broadcast
  a.SendMsg(b.broadcastAddr(),(void*) strMsg,strlen(strMsg),1);
  receiver.lock();
  printf("Message was: %s\n",receiver.data);
  clTest(("send/recv message ok"), 0 == strncmp((const char*) receiver.data,strMsg,sizeof(strMsg)),("message contents miscompare") );
  }


int main(int argc, char* argv[])
{
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  //logEchoToFd = 1; // stdout
  clTestGroupInitialize(("Test Message Server"));

  SAFplusI::defaultMsgTransport = "../lib/clMsgUdp.so";

  clMsgInitialize();

  clTestCase(("simple send/recv test"),testSendRecv());
  
  
  clTestGroupFinalize();
}
