#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <clMsgApi.hxx>
#include <clTestApi.hxx>

uint_t reflectorPort = 10;
using namespace SAFplus;

int main(int argc, char* argv[])
{
  std::string xport("clMsgUdp.so");
  SAFplus::logCompName = "RFL";
  //SAFplus::logSeverity = SAFplus::LOG_SEV_INFO;
  //logEchoToFd = 1; // stdout
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("port", boost::program_options::value<int>(), "reflector port number")
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
  if (vm.count("port")) reflectorPort = vm["port"].as<int>();
  if (vm.count("xport")) xport = vm["xport"].as<std::string>();
  if (vm.count("loglevel")) SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());

  MsgPool msgPool;
  ClPlugin* api = NULL;
#if 0
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
      if (xp) 
        {
        MsgTransportConfig xCfg = xp->initialize(msgPool);
        }
      }

#endif

  SAFplusI::defaultMsgTransport = xport.c_str();
  clMsgInitialize();
  MsgTransportPlugin_1* xp = SAFplusI::defaultMsgPlugin;
  MsgTransportConfig& xCfg = xp->config;
  logNotice("MSG","RFL","Message Reflector: Transport [%s] [%s] mode, node [%u] maxPort [%u] maxMsgSize [%u]", xp->type, xp->clusterNodes ? "Cloud":"LAN",xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize);
  if (xp)
    {
      Message* m;
      ScopedMsgSocket sock(xp,reflectorPort);

      while(1)
        {
          m = sock->receive(1,0);
          if (m) 
            { 
              // for max speed we don't even call the log: logDebug("MSG","RFL", "rcv port [%d] node [%d]\n", m->port, m->node);
              sock->send(m);
            }
        }
    }


}
