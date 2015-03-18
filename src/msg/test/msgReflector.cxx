#include <boost/thread.hpp>
#include <clMsgTransportPlugin.hxx>
#include <clTestApi.hxx>

uint_t reflectorPort = 10;
using namespace SAFplus;

int main(int argc, char* argv[])
{
  //SAFplus::logSeverity = SAFplus::LOG_SEV_INFO;
  //logEchoToFd = 1; // stdout

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
      if (xp) 
        {
        MsgTransportConfig xCfg = xp->initialize(msgPool);
        logNotice("MSG","RFL","Message Reflector: Transport [%s], node [%u] maxPort [%u] maxMsgSize [%u]", xp->type, xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize);
        if (1)
          {
          Message* m;
          ScopedMsgSocket sock(xp,reflectorPort);

          while(1)
            {
              m = sock->receive(1,0);
              if (m) 
                { 
                  //logDebug("MSG","RFL", "rcv port [%d] node [%d]\n", m->port, m->node);
                  sock->send(m);
                }
            }
          }
        }
      }

}
