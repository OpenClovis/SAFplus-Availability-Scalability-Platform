#include <clMsgTransportPlugin.hxx>
#include <clTestApi.hxx>

using namespace SAFplus;

bool testSendRecv(MsgTransportPlugin_1* xp)
  {

  MsgSocket* a = xp->createSocket(1);
  MsgSocket* b = xp->createSocket(1);
  
  Message* m;

  m = b->receive(1,0);
  clTest(("receiving nothing, no delay"), m == NULL, (" "));

  m = b->receive(1,500);
  clTest(("receiving nothing, with delay"), m == NULL, (" "));

  m = a->msgPool->allocMsg();
  clTest(("message allocated"), m != NULL,(" "));
  MsgFragment* frag = m->append(0);
  frag->set("This is a test of message sending\n");
  m->node = b->node;  // Send the message to b
  m->port = b->port;

  a->send(m);
  m = b->receive(1,500);
  clTest(("recv"),m != NULL,(" "));
   
  b->msgPool->free(m);
  }



int main(int argc, char* argv[])
{
  //logEchoToFd = 1; // stdout
  clTestGroupInitialize(("Test Message Transport"));


  MsgPool msgPool;

  if (1)
    {
    ClPluginHandle* plug = clLoadPlugin(SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID,SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER,"clMsgUdp.so");
    clTest(("plugin loads"), plug != NULL,(" "));
    if (plug)
      {
      MsgTransportPlugin_1* xp = dynamic_cast<MsgTransportPlugin_1*> (plug->pluginApi);
      clTest(("plugin casts"), xp != NULL,(" "));
      if (xp) 
        {
        xp->initialize(msgPool,NULL);
        clTestCase(("simple send/recv test"),testSendRecv(xp));
        }
      }
    }

  clTestGroupFinalize();
}
