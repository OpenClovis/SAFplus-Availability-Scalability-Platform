#include <clMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clTestApi.hxx>
#include <boost/iostreams/stream.hpp>

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
  MsgServer a(2,10,2);
  MsgServer b(1,10,2);

  const char* strMsg = "This is a test of message sending";

  RecvHandler receiver;
  b.RegisterHandler(1,&receiver,NULL);
  b.Start();

  a.SendMsg(b.handle,(void*) strMsg,strlen(strMsg),1);
  receiver.lock();
  printf("Message was: %s\n",receiver.data);
  clTest(("send/recv message ok"), 0 == strncmp((const char*) receiver.data,strMsg,sizeof(strMsg)),("message contents miscompare") );

  // Test buffer interface
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

  // Test Message interface
  val = 2;
  MsgPool& pool = a.getMsgPool();
  Message* msg = pool.allocMsg();
  MsgFragment* frag = msg->append(sizeof(val));
  memcpy(frag->data(),&val,sizeof(val));
  frag->len+=sizeof(val);
  msg->setAddress(b.handle);
  a.SendMsg(msg,2);

  val = 1;
  msg = pool.allocMsg();
  frag = msg->append(sizeof(val));
  memcpy(frag->data(),&val,sizeof(val));
  frag->len+=sizeof(val);
  msg->setAddress(b.handle);
  a.SendMsg(msg,1);

  receiver.lock();
  clTest(("msgType demultiplex 1"), *((uint_t*) receiver.data) == 1, ("data is [%d]",*((uint_t*) receiver.data) ));
  receiver2.lock();
  clTest(("msgType demultiplex 2"), *((uint_t*) receiver2.data) == 2, ("data is [%d]",*((uint_t*) receiver2.data) ));

  // Test message stream interface
  msg = pool.allocMsg();
  if (1)
    {
    boost::iostreams::stream<MessageOStream>  mos(msg);
    mos <<  9 <<  8;
    mos.flush();
    }
  msg->setAddress(b.handle);
  a.SendMsg(msg,1);

  receiver.lock();
  clTest(("msg ostream serialization"), *((char*) receiver.data) == '9', ("data is [%d]",(int) *((char*) receiver.data) ));
  clTest(("msg ostream serialization"), *((char*) receiver.data+1) == '8', ("data is [%d]",(int) *((char*) receiver.data+1) ));
 
  // Verify that sockets are bidirectional
  RecvHandler a_receiver;
  a.RegisterHandler(1,&a_receiver,NULL);
  a.Start();

  b.SendMsg(a.handle,(void*) strMsg,strlen(strMsg),1);
  a_receiver.lock();
  printf("Message was: %s\n",a_receiver.data);
  clTest(("bidirectional send/recv message ok"), 0 == strncmp((const char*) a_receiver.data,strMsg,sizeof(strMsg)),("message contents miscompare") );

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

  //SAFplusI::defaultMsgTransport = "../lib/clMsgUdp.so";
  SAFplusI::defaultMsgTransport = "clMsgSctp.so";

  clMsgInitialize();

  clTestCase(("simple send/recv test"),testSendRecv());
  
  
  clTestGroupFinalize();
}
