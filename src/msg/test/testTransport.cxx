/*
        sysctl -w net.core.wmem_max=10485760
        sysctl -w net.core.rmem_max=10485760
        sysctl -w net.core.rmem_default=10485760
        sysctl -w net.core.wmem_default=10485760
*/

// If the test's send amount exceeds the kernel's network buffers then packets are dropped.  These packets disappear in unreliable transports like UDP which breaks this unit test.
#define KERNEL_NET_BUF_SIZE 10485760

#include <boost/thread.hpp>
#include <clMsgApi.hxx>
#include <clTestApi.hxx>
#include <boost/program_options.hpp>
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

bool testSendRecv(MsgTransportPlugin_1* xp)
  {
  const char* strMsg = "This is a test of message sending";
  MsgSocket* a = xp->createSocket(1);
  MsgSocket* b = xp->createSocket(2);
  Handle bHdl = b->handle();
  Message* m;
  uint64_t initialAlloc = a->msgPool->allocated;
  uint64_t initialFrag = a->msgPool->fragAllocated;
  m = b->receive(1,0);
  clTest(("receiving nothing, no delay"), m == NULL, (" "));

  //m = b->receive(1,500);
  //clTest(("receiving nothing, with delay"), m == NULL, (" "));

  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  clTest(("message buffer tracking"), ((a->msgPool->allocated - initialAlloc) == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", a->msgPool->allocated));
  clTest(("Frag allocated"), ((a->msgPool->fragAllocated - initialFrag) == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", a->msgPool->fragAllocated));
  MsgFragment* frag = m->append(0);
  clTest(("Frag post-allocate"), (a->msgPool->fragAllocated - initialFrag == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", a->msgPool->fragAllocated));

  frag->set(strMsg);
  m->setAddress(bHdl);
  m->node = b->node;  // Send the message to b
  m->port = b->port;
  a->send(m);
  // msgs can be sent and released asynchronously so we don't actually know it is sent until it is received so we can't test a's msgPool until we receive the response.  But a and b have the same msgPool in this test so we must just delay and hope
  boost::this_thread::sleep(boost::posix_time::milliseconds(100));  
  clTest(("message release"), (a->msgPool->allocated - initialAlloc == 0), ("Allocated msgs is [%" PRIu64 "]. Expected 0", a->msgPool->allocated));
  clTest(("Frag release"), (a->msgPool->fragAllocated - initialFrag == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", a->msgPool->fragAllocated));
  m = b->receive(1);

  if (m)
    {

      // Verify source node port is correct
      clTest(("Verify source node in received msg"), (m->node == a->node), ("Source node is [%d]. Expected [%d]", m->node, a->node));
      clTest(("Verify source port in received msg"), (m->port == a->port), ("Source port is [%d]. Expected [%d]", m->port, a->port));

      clTest(("message rcv pool audit"), (b->msgPool->allocated - initialAlloc == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", b->msgPool->allocated));
      clTest(("msg rcv frag pool audit"), (a->msgPool->fragAllocated - initialFrag == 1), ("Allocated frags is [%" PRIu64 "]. Expected 1", b->msgPool->fragAllocated)); 

      clTest(("recv"),m != NULL,(" "));
      printf("%s\n",(const char*) m->firstFragment->read());
      clTest(("send/recv message ok"), 0 == strncmp((const char*) m->firstFragment->read(),strMsg,sizeof(strMsg)),("message contents miscompare: %s -> %s", strMsg,(const char*) m->firstFragment->read()) );
      b->msgPool->free(m);
      clTest(("message rcv pool audit"), (b->msgPool->allocated - initialAlloc == 0), ("Allocated msgs is [%" PRIu64 "]. Expected 0", b->msgPool->allocated));
      clTest(("msg rcv frag pool audit"), (a->msgPool->fragAllocated - initialFrag == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", b->msgPool->fragAllocated)); 
    }

  // Test min msg size
  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  frag = m->append(0);
  frag->set("1");
  m->setAddress(bHdl);
  a->send(m);
  m = b->receive(1);
  clTest(("recv"),m != NULL,(" "));
  printf("%s\n",(const char*) m->firstFragment->read());
  clTest(("Mininum Msg Size, const char*"), ((const char*)m->firstFragment->read())[0] == '1',("message contents miscompare: %c -> %c", '1',((const char*) m->firstFragment->read())[0]) );
  if (m) b->msgPool->free(m);

  // Test using attached memory buffers
  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  frag = m->append(1);
  ((char*)frag->data(0))[0] = '2';
  frag->len = 1;
  m->setAddress(b->node, b->port);
  a->send(m);
  m = b->receive(1);
  clTest(("recv"),m != NULL,(" "));
  printf("%s\n",(const char*) m->firstFragment->read());
  clTest(("Mininum Msg Size, const char*"), ((const char *)m->firstFragment->read())[0] == '2',("message contents miscompare: %c -> %c", '2',((const char*) m->firstFragment->read())[0] ));
  if (m) b->msgPool->free(m);

  // Test max msg size
  int maxMsgSize = xp->config.maxMsgSize;
  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  frag = m->append(maxMsgSize);
  // TODO fill message with data
  frag->len=maxMsgSize;
  m->setAddress(b->node, b->port);
  a->send(m);
  m = b->receive(1);
  

  xp->deleteSocket(a);
  xp->deleteSocket(b);
  return true;
  }

bool testSendRecvSize(MsgTransportPlugin_1* xp)
  {
  const char* strMsg = "This is a test of message sending";
  //MsgSocket* a = xp->createSocket(1);
  //MsgSocket* b = xp->createSocket(2);
  ScopedMsgSocket a(xp,1);
  ScopedMsgSocket b(xp,2);

  int maxTry = 1;

  Message* m;

  unsigned long seed = 0;

  for (int size = 1; size <= xp->config.maxMsgSize; size+=512)
    {
    seed++;
    printf("%d ", size);
    fflush(stdout);
    m = a->msgPool->allocMsg();
    clTest(("message allocated"), m != NULL,(" "));
    m->setAddress(b->node,b->port);

    MsgFragment* frag = m->append(size);
    clTest(("message frag allocated"), frag != NULL,(" "));
    xorshf96 rnd(seed);
    unsigned char* buf = (unsigned char*) frag->data();
    for (int i = 0; i < size; i++,buf++)
      {
      *buf = rnd();
      }
    frag->len = size;
    a->send(m);
    m = b->receive(1,0);
    int tries = 0;
    while (tries<maxTry && !m)
      {
      boost::this_thread::sleep(boost::posix_time::milliseconds(50));
      m = b->receive(1,0);
      tries++;
      }
    assert(m);
    frag = m->firstFragment;
    assert(frag);
    xorshf96 rnd1(seed);
    const unsigned char* bufr = (const unsigned char*) frag->read();
    int miscompare=-1;
    for (int i = 0; (i < size) && !miscompare; i++,bufr++)
      {
      if (*bufr != rnd())
        {
        miscompare = i;
        }
      }
    clTest(("send/recv message ok"),miscompare==-1,("message size [%d] contents miscompare at position: [%d]",size,miscompare));
    }
  return true;
  }

bool testSendRecvMultiple(MsgTransportPlugin_1* xp)
  {
  const char* strMsg = "This is a test of message sending";
  MsgSocket* a = xp->createSocket(1);
  MsgSocket* b = xp->createSocket(2);

  Message* m;
  
  unsigned long seed = 0;

  for (int atOnce = 1; atOnce < xp->config.maxMsgAtOnce; atOnce+=((rand()%37)+1))
    {
      printf("\nchunk [%d]: ",atOnce);
      for (int size = 1; size <= xp->config.maxMsgSize; size+=((rand()%4096)+1))
      {
       
      // If the test's send amount exceeds the kernel's network buffers then packets are dropped.  These packets disappear in unreliable transports like UDP which breaks this unit test.
      // Experimentally (UDP), it actually needs to be be the network buffer size / 2.  Not sure why that would be...
      if (atOnce*size > KERNEL_NET_BUF_SIZE/2) { size=xp->config.maxMsgSize+1; break; }

      seed++;
      printf("%d ", size);
      fflush(stdout);
      m = a->msgPool->allocMsg();
      clTest(("message allocated"), m != NULL,(" "));
      Message* curm = m;
      int msgCount = 0;
      do
        {
        curm->setAddress(b->node,b->port);

        MsgFragment* frag = curm->append(size);
        clTest(("message frag allocated"), frag != NULL,(" "));
        xorshf96 rnd(seed);
        unsigned char* buf = (unsigned char*) frag->data();
        for (int i = 0; i < size; i++,buf++)
          {
          *buf = rnd();
          }
        frag->len = size;
        msgCount++;
        if (msgCount < atOnce) 
          {
          curm->nextMsg = a->msgPool->allocMsg();
          curm = curm->nextMsg;
          }
        } while (msgCount < atOnce);

      a->send(m);

      msgCount = 0;
      while (msgCount < atOnce)
        {
        m = b->receive(1,0);  // Even though a sent multiples, I may not receive them as multiples
        if (!m)
          {
          boost::this_thread::sleep(boost::posix_time::milliseconds(250));
          m = b->receive(1,0);
          }
        assert(m);  /* If you get this then messages were dropped in
        the kernel.  Use these commands to increase the kernel network buffers:
        sysctl -w net.core.wmem_max=10485760
        sysctl -w net.core.rmem_max=10485760
        sysctl -w net.core.rmem_default=10485760
        sysctl -w net.core.wmem_default=10485760
                    */
        curm = m;
        while(curm)
          {
          MsgFragment* frag = curm->firstFragment;
          assert(frag);
          xorshf96 rnd1(seed);
          const unsigned char* bufr = (const unsigned char*) frag->read();
          int miscompare=-1;
          for (int i = 0; (i < size) && !miscompare; i++,bufr++)
            {
            if (*bufr != rnd1())
              {
              miscompare = i;
              }
            }
          clTest(("send/recv message ok"),miscompare==-1,("message size [%d] contents miscompare at position: [%d]",size,miscompare));
          msgCount++;
          curm=curm->nextMsg;
          }
        b->msgPool->free(m);
        }
      
      }
    }

  return true;
  }



void msgPoolTests(MsgPool& msgPool)
{
  clTest(("message tracking initial state"), (msgPool.allocated == 0), ("Allocated msgs is [%" PRIu64 "]. Expected 0", msgPool.allocated));
  clTest(("Frag allocated"), (msgPool.fragAllocated == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", msgPool.fragAllocated));
  clTest(("Frag allocated bytes"), (msgPool.fragAllocatedBytes == 0), ("Allocated frag bytes is [%" PRIu64 "]. Expected 0", msgPool.fragAllocatedBytes));
  clTest(("Frag cumulative"), (msgPool.fragCumulativeBytes== 0), ("fragCumulativeAllocated [%" PRIu64 "]. Expected 0", msgPool.fragCumulativeBytes));
}

class TestStructure
{
public:
  int8_t  i8;
  int16_t i16;
  int32_t i32;
  int64_t i64;

  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
  TestStructure() {}
  TestStructure(int8_t _i8,int16_t _i16,int32_t _i32,int64_t _i64,uint8_t _u8,uint16_t _u16,uint32_t _u32,uint64_t _u64)
  {
  i8 = _i8;
  i16 = _i16;
  i32 = _i32;
  i64 = _i64;

  u8 = _u8;
  u16 = _u16;
  u32 = _u32;
  u64 = _u64;   
  }

  bool operator == (const TestStructure& ts) const
  {
    if (i8 != ts.i8) return false;
    if (i16 != ts.i16) return false;
    if (i32 != ts.i32) return false;
    if (i64 != ts.i64) return false;
    if (u8 != ts.u8) return false;
    if (u16 != ts.u16) return false;
    if (u32 != ts.u32) return false;
    if (u64 != ts.u64) return false;
    return true;
    //int ret = ::memcmp((void*) this,(void*) &ts,sizeof(TestStructure));
    //return ret==0;
  }
};

MessageOStream& operator << (MessageOStream& mos, TestStructure& val)
{
  mos << val.i8 << val.i16 << val.i32 << val.i64 << val.u8 << val.u16 << val.u32 << val.u64;
  return mos;
}

MessageIStream& operator >> (MessageIStream& mos, TestStructure& val)
{
  mos >> val.i8 >> val.i16 >> val.i32 >> val.i64 >> val.u8 >> val.u16 >> val.u32 >> val.u64;
  return mos;
}


void messageTests(MsgPool& msgPool)
{
  Message*  m = msgPool.allocMsg();
  MsgFragment* frag = m->append(4);
  MsgFragment* frag2 = m->append(7);
  const char* testdata = "0123456789";

  if (1)
    {
    boost::iostreams::stream<MessageOStream>  mos(m);
    mos << testdata;  // Note this does NOT output /0
    }

  if (1)
    {
    boost::iostreams::stream<MessageIStream>  mis(m);
    char test[11];
    mis.read(test,10);
    //clTest(("read"), 10 == mis.read(test,10),("read length incorrect"));
    clTest(("read results"), strncmp(testdata,test,10)==0, ("data miscompare"));
    }

  msgPool.free(m);

  m = msgPool.allocMsg();
  frag = m->append(4);
  frag2 = m->append(7);
  m->append(10);

  uint64_t num = 0x123456789abcdef0ULL;
  uint64_t num2 = 0x123456789abcdef0ULL + 0xa5a5a5a5a5a5a5aULL;
  if (1)
    {
    MessageOStream  mos(m);
    mos.write((char*)&num, sizeof(num));
    mos.write((char*)&num2, sizeof(num2));
    }

  if (1)
    {
    MessageIStream mis(m);
    uint64_t a,b;
    mis.read((char*)&a,sizeof(a));
    mis.read((char*)&b,sizeof(b));
    clTest(("read binary data"), b == num2,("data miscompare"));
    clTest(("read binary data"), a == num, ("data miscompare"));
    }
  
  msgPool.free(m);

  m = msgPool.allocMsg();
  frag = m->append(4);
  frag2 = m->append(7);
  m->append(10);
  m->append(20);

  TestStructure numberData(-1,-2,-3,-4,5,6,7,8);
  if (1)
    {
    MessageOStream  mos(m);
    mos << numberData;    
    }

  if (1)
    {
    MessageIStream mis(m);
    TestStructure tmp;
    mis >> tmp;

    clTest(("read/write binary structure data"), numberData == tmp,("data miscompare"));
    int i = 1;
    }

  msgPool.free(m);  
}


char MsgXportTestPfx[4] = {0};
const char* ModeStr = 0;

int main(int argc, char* argv[])
{
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  SAFplus::logCompName = "TSTTRA";

  std::string xport("clMsgUdp.so");
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "this help message")
    ("xport", boost::program_options::value<std::string>(), "transport plugin filename")
    ("loglevel", boost::program_options::value<std::string>(), "logging cutoff level")
    ("mode", boost::program_options::value<std::string>()->default_value("LAN"), "specify LAN or cloud to set the messaging transport address resolution mode")
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

  // Create a unique test prefix based on the transport name
  strncpy(MsgXportTestPfx,&xport.c_str()[5],3);
  MsgXportTestPfx[3] = 0;
  for (int i=0;i<3;i++) MsgXportTestPfx[i] = toupper(MsgXportTestPfx[i]);

  //logEchoToFd = 1; // stdout
  clTestGroupInitialize(("MXP-XPT-UNT.TG002: Test Message Transport"));


  MsgPool msgPool;

  clTestCase(("MXP-POL-MEM.TC001: Message memory pool"),msgPoolTests(msgPool));
  clTestCase(("MXP-SND-FNC.TC002: Message send recv functional loopback tests"), messageTests(msgPool));

  ClPlugin* api = NULL;
  if (1)
    {
#ifdef DIRECTLY_LINKED
    api  = clPluginInitialize(SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER);
#else
    ClPluginHandle* plug = clLoadPlugin(SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID,SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER,xport.c_str());
    clTest(("plugin loads"), plug != NULL,(" "));
    if (plug) api = plug->pluginApi;
#endif
    }
  if (api)
      {
      MsgTransportPlugin_1* xp = dynamic_cast<MsgTransportPlugin_1*> (api);
      clTest(("plugin casts"), xp != NULL,(" "));
      if (xp) 
        {
          ClusterNodes* clusterNodes = NULL;
          if (vm["mode"].as<std::string>() == "cloud")
            {
              clusterNodes = new ClusterNodes();
              ModeStr = "CLD";
            }
          else ModeStr = "LAN";

        clTestCaseStart(("MXP-%3s-%3s.TC001: initialization",MsgXportTestPfx,ModeStr));
        MsgTransportConfig xCfg = xp->initialize(msgPool,clusterNodes);
        bool abort = false;          
        clTestCaseMalfunction(("Node address is not set properly"), xCfg.nodeId != 0, abort=true);
          
        if (!abort)
          {
          logInfo("TST","MSG","Msg Transport [%s], node [%u] maxPort [%u] maxMsgSize [%u]", xp->type, xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize);
          clTestCaseEnd((" "));
          clTestCase(("MXP-%3s-%3s.TC003: simple send/recv test",MsgXportTestPfx,ModeStr),testSendRecv(xp));
          clTestCase(("MXP-%3s-%3s.TC004: send/recv messages of every allowed length",MsgXportTestPfx,ModeStr),testSendRecvSize(xp));
          clTestCase(("MXP-%3s-%3s.TC005: send/recv multiple simultaneous messages of every allowed length",MsgXportTestPfx,ModeStr),testSendRecvMultiple(xp));
          }
        }
      }

  clTestGroupFinalize();
}
