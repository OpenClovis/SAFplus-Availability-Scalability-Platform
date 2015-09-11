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
#include <reliableSocket.hxx>

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

char MsgXportTestPfx[4] = {0};
const char* ModeStr = 0;

int main(int argc, char* argv[])
{
  logEchoToFd = 1;  // echo logs to stdout for debugging
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  //SAFplus::logCompName = "TSTTRA";
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
  safplusInitialize(SAFplus::LibDep::LOG);
  ClPlugin* api = NULL;
  if (1)
  {
#ifdef DIRECTLY_LINKED
    api  = clPluginInitialize(SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER);
#else
    ClPluginHandle* plug = clLoadPlugin(SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID,SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER,xport.c_str());
    clTest(("plugin loads"), plug != NULL,(" "));
    if (plug)
    {
      api = plug->pluginApi;
    }
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
      MsgPool msgPool;
      clTestCaseStart(("MXP-%3s-%3s.TC001: initialization",MsgXportTestPfx,ModeStr));
      MsgTransportConfig xCfg = xp->initialize(msgPool,clusterNodes);
      logInfo("TST","MSG","Msg Transport [%s], node [%u] maxPort [%u] maxMsgSize [%u]", xp->type, xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize);
      Handle destination = SAFplus::getProcessHandle(3,122);
      MsgSocketReliable sockclient(4,xp,destination);
      printf("init socket : done \n");
      int i=1;
      do
      {  
      printf("send msg [%d] \n",i);
      unsigned char* buffer = new unsigned char[12345+i*1000];
      memset( buffer, 'c', sizeof(unsigned char)*(12345+i*1000));
      sockclient.writeReliable(buffer,0,12345+i*1000);
      i++;
      }while(i<3);
      do
      {
        sleep(2);
      }while(1);

     }
  }
}
