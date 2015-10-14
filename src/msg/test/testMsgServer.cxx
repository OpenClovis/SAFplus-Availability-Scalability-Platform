#include <clMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clMsgSarSocket.hxx>
#include <clTestApi.hxx>
#include <boost/program_options.hpp>
#include <boost/iostreams/stream.hpp>

using namespace SAFplus;

// add segmentation and reassembly layer if this is true
bool sar = false;

const char* suiteName = "SVR";  // this is the name of the suite of tests.  It changes based on the parameters supplied

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
  //MsgServer a(2,10,2,SAFplus::MsgServer::Options::DEFAULT_OPTIONS,SAFplus::MsgServer::SocketType::SOCK_SEGMENTATION);
  //MsgServer b(1,10,2,SAFplus::MsgServer::Options::DEFAULT_OPTIONS,SAFplus::MsgServer::SocketType::SOCK_SEGMENTATION);
  MsgServer a(2,10,2);
  MsgServer b(1,10,2);
  if (sar)
    {
      // TODO memory leak
      a.setSocket(new MsgSarSocket(a.getSocket()));
      b.setSocket(new MsgSarSocket(b.getSocket()));
    }


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
  clTest(("broadcast send/recv message ok"), 0 == strncmp((const char*) receiver.data,strMsg,sizeof(strMsg)),("message contents miscompare") );
  }


int main(int argc, char* argv[])
{
  SAFplus::  logSeverity = LOG_SEV_MAX;

  logEchoToFd = 1; // stdout

  std::string xport("clMsgUdp.so");
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "this help message")
    ("xport", boost::program_options::value<std::string>(), "transport plugin filename")
    ("loglevel", boost::program_options::value<std::string>(), "logging cutoff level")
    ("mode", boost::program_options::value<std::string>()->default_value("LAN"), "specify 'LAN' or 'cloud' to set the messaging transport address resolution mode")
    ("sar", boost::program_options::value<bool>()->default_value("false"), "Use segmentation and reassembly layer")
    ;

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
      return 0;
    }
  if (vm.count("xport")) xport = vm["xport"].as<std::string>();
  if (vm.count("loglevel")) SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());

  if (vm.count("sar"))
    {
      sar=true;
      suiteName = "SAR";
    }

  clTestGroupInitialize(("MSG-%s-UNT.TG003: Test MsgServer class",suiteName));

  if (vm["mode"].as<std::string>() == "cloud")
    {
    // Force cloud mode (you need to have set it up using the "node" command line)
    SAFplus::defaultClusterNodes = new ClusterNodes(false);
    }

  SAFplusI::defaultMsgTransport = xport.c_str();  // only works bc the context of xport is the full main() function
  //SAFplusI::defaultMsgTransport = "clMsgSctp.so";

  clMsgInitialize();

  clTestCase(("MSG-%s-UNT.TC001: simple send/recv test",suiteName),testSendRecv());

  clTestGroupFinalize();
}
