#include <clCommon.hxx>
#include <clMsgApi.hxx>

namespace SAFplusI
{
SAFplus::MsgTransportPlugin_1* defaultMsgPlugin = NULL;
};

namespace SAFplus
{
MsgPool msgPool;
  int msgInitCount=0;
void clMsgInitialize(void)
  {
    msgInitCount++;
    if (msgInitCount > 1) return;
    //? <cfg name="SAFPLUS_MSG_TRANSPORT">This environment variable specifies which message transport plugin your cluster should use.  example: export SAFPLUS_MSG_TRANSPORT=clMsgSctp.so</cfg>
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
      logCritical("MSG", "INI", "Cannot load message transport plugin [%s] library path is [%s]",xportFile, getenv("LD_LIBRARY_PATH"));
      assert(!"Cannot load message transport plugin");
    }
#endif

  if (api)
    {
    MsgTransportPlugin_1* xp = dynamic_cast<MsgTransportPlugin_1*> (api);
    if (xp) 
      {
      MsgTransportConfig::Capabilities cap = xp->getCapabilities();
      //? <cfg name="SAFPLUS_CLOUD_NODES">Specifies to use cloud mode and the address/base port of other cluster nodes (or just 1 if you do not want to specify any cluster nodes).  Running safplus_cloud --reload will use this variable to load nodes.  Nodes IDs will be assigned in order, starting with 1.  Therefore, it is important that this environment variable be the same for all nodes in the cluster.  Example: export SAFPLUS_CLOUD_NODES=192.168.45.32,192.168.45.35:10000.</cfg>
      char* cnVar = std::getenv("SAFPLUS_CLOUD_NODES");
      if (cnVar)
        {
          logInfo("MSG", "INI", "Using cloud mode node addressing because SAFPLUS_CLOUD_NODES is defined.");
        }
      if (cnVar || (!(cap & MsgTransportConfig::BROADCAST))) // can't do broadcast so we need to use a cluster nodes tracker
        {
        if (!SAFplus::defaultClusterNodes)  SAFplus::defaultClusterNodes = new ClusterNodes(false);
        }       

      SAFplusI::defaultMsgPlugin = xp;
      MsgTransportConfig cfg = xp->initialize(msgPool,SAFplus::defaultClusterNodes);
      logInfo("MSG","INI","Message Transport [%s] [%s] mode initialized.  Max Size [%d], Max Chunk [%d].", xp->type, xp->clusterNodes ? "Cloud": "LAN", xp->config.maxMsgSize, xp->config.maxMsgAtOnce);

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
