#include <clCommon.hxx>
#include <clMsgTransportPlugin.hxx>

namespace SAFplusI
{
SAFplus::MsgTransportPlugin_1* defaultMsgPlugin = NULL;
};

namespace SAFplus
{
MsgPool msgPool;

void clMsgInitialize(void)
  {
  const char* xportFile = getenv("SAFPLUS_MSG_TRANSPORT");
  if (!xportFile)
    {
    xportFile = SAFplusI::defaultMsgTransport;
    }

  ClPlugin* api = NULL;

#ifdef SAFPLUS_MSG_DIRECTLY_LINKED
  api  = clPluginInitialize(SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER);
#else
  ClPluginHandle* plug = clLoadPlugin(SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID,SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER,xportFile);
  if (plug) api = plug->pluginApi;
  else
    {
    assert(!"Cannot load message transport plugin");
    }
#endif

  if (api)
    {
    MsgTransportPlugin_1* xp = dynamic_cast<MsgTransportPlugin_1*> (api);
    if (xp) 
      {
      xp->initialize(msgPool);
      SAFplusI::defaultMsgPlugin = xp;
      if (SAFplus::ASP_NODEADDR == ~((ClWordT) 0))  // not initialized
        SAFplus::ASP_NODEADDR = xp->config.nodeId;
      else
        {
        // Multi transport or environment variable case.  The discovered node address and the configured one need to be the same.  And the node address needs to be the same across all transports.
        assert(SAFplus::ASP_NODEADDR == xp->config.nodeId);
        }
      }
    else
      {
      assert(!"bad plugin");
      }
    }
  else
    {
    assert(!"bad plugin");
    }

  }
};
