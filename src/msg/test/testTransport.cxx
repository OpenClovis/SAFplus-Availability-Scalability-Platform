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

  m = b->receive(1,0);
  clTest(("receiving nothing, no delay"), m == NULL, (" "));

  //m = b->receive(1,500);
  //clTest(("receiving nothing, with delay"), m == NULL, (" "));

  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  clTest(("message buffer tracking"), (a->msgPool->allocated == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", a->msgPool->allocated));
  clTest(("Frag allocated"), (a->msgPool->fragAllocated == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", a->msgPool->fragAllocated));
  MsgFragment* frag = m->append(0);
  clTest(("Frag post-allocate"), (a->msgPool->fragAllocated == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", a->msgPool->fragAllocated));

  frag->set(strMsg);
  m->setAddress(bHdl);
  m->node = b->node;  // Send the message to b
  m->port = b->port;
  a->send(m);
  // msgs can be sent and released asynchronously so we don't actually know it is sent until it is received so we can't test a's msgPool until we receive the response.  But a and b have the same msgPool in this test so we must just delay and hope
  boost::this_thread::sleep(boost::posix_time::milliseconds(100));  
  clTest(("message release"), (a->msgPool->allocated == 0), ("Allocated msgs is [%" PRIu64 "]. Expected 0", a->msgPool->allocated));
  clTest(("Frag release"), (a->msgPool->fragAllocated == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", a->msgPool->fragAllocated));
  m = b->receive(1);

  if (m)
    {

      // Verify source node port is correct
      clTest(("Verify source node in received msg"), (m->node == a->node), ("Source node is [%d]. Expected [%d]", m->node, a->node));
      clTest(("Verify source port in received msg"), (m->port == a->port), ("Source port is [%d]. Expected [%d]", m->port, a->port));

      clTest(("message rcv pool audit"), (b->msgPool->allocated == 1), ("Allocated msgs is [%" PRIu64 "]. Expected 1", b->msgPool->allocated));
      clTest(("msg rcv frag pool audit"), (a->msgPool->fragAllocated == 1), ("Allocated frags is [%" PRIu64 "]. Expected 1", b->msgPool->fragAllocated)); 

      clTest(("recv"),m != NULL,(" "));
      printf("%s\n",(const char*) m->firstFragment->read());
      clTest(("send/recv message ok"), 0 == strncmp((const char*) m->firstFragment->read(),strMsg,sizeof(strMsg)),("message contents miscompare: %s -> %s", strMsg,(const char*) m->firstFragment->read()) );
      b->msgPool->free(m);
      clTest(("message rcv pool audit"), (b->msgPool->allocated == 0), ("Allocated msgs is [%" PRIu64 "]. Expected 0", b->msgPool->allocated));
      clTest(("msg rcv frag pool audit"), (a->msgPool->fragAllocated == 0), ("Allocated frags is [%" PRIu64 "]. Expected 0", b->msgPool->fragAllocated)); 
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

  //logEchoToFd = 1; // stdout
  clTestGroupInitialize(("Test Message Transport"));


  MsgPool msgPool;

  msgPoolTests(msgPool);


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
        MsgTransportConfig xCfg = xp->initialize(msgPool);
        logInfo("TST","MSG","Msg Transport [%s], node [%u] maxPort [%u] maxMsgSize [%u]", xp->type, xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize);
        clTestCase(("simple send/recv test"),testSendRecv(xp));
        clTestCase(("send/recv messages of every allowed length"),testSendRecvSize(xp));
        clTestCase(("send/recv messages of every allowed length"),testSendRecvMultiple(xp));
        }
      }

  clTestGroupFinalize();
}
