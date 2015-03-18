#include <boost/thread.hpp>
#include <clMsgTransportPlugin.hxx>
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

bool testSendRecv(MsgTransportPlugin_1* xp)
  {
  const char* strMsg = "This is a test of message sending";
  MsgSocket* a = xp->createSocket(1);
  MsgSocket* b = xp->createSocket(2);
  
  Message* m;

  m = b->receive(1,0);
  clTest(("receiving nothing, no delay"), m == NULL, (" "));

  //m = b->receive(1,500);
  //clTest(("receiving nothing, with delay"), m == NULL, (" "));

  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  MsgFragment* frag = m->append(0);
  frag->set(strMsg);
  m->node = b->node;  // Send the message to b
  m->port = b->port;

  a->send(m);
  m = b->receive(1,0);
  clTest(("recv"),m != NULL,(" "));
  printf("%s\n",(const char*) m->firstFragment->read());
  clTest(("send/recv message ok"), 0 == strncmp((const char*) m->firstFragment->read(),strMsg,sizeof(strMsg)),("message contents miscompare: %s -> %s", strMsg,(const char*) m->firstFragment->read()) );
  if (m) b->msgPool->free(m);


  // Test min msg size
  // Test max msg size
  // Verify source node port is correct

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
    if (!m)
      {
      boost::this_thread::sleep(boost::posix_time::milliseconds(50));
      m = b->receive(1,0);
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

  for (int atOnce = 1; atOnce < xp->config.maxMsgAtOnce; atOnce++)
    {
    for (int size = 1; size <= xp->config.maxMsgSize/10; size+=512)
      {
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
           MsgFragment* frag = m->firstFragment;
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
          curm=m->nextMsg;
          }
        }
      
      }
    }

  return true;
  }




int main(int argc, char* argv[])
{
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  //logEchoToFd = 1; // stdout
  clTestGroupInitialize(("Test Message Transport"));


  MsgPool msgPool;
  ClPlugin* api = NULL;
  if (1)
    {
#ifdef DIRECTLY_LINKED
    api  = clPluginInitialize(SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER);
#else
    ClPluginHandle* plug = clLoadPlugin(SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID,SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER,"../lib/clMsgUdp.so");
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
