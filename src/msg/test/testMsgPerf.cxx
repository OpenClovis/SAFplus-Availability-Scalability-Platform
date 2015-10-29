#include <boost/thread.hpp>
#include <boost/timer.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <clMsgApi.hxx>
#include <clTestApi.hxx>

uint_t reflectorPort = 10;
uint_t reflectorNode = 102;
uint_t repeat = 1;
bool sar = false;
using namespace SAFplus;
using namespace boost;
namespace po = boost::program_options;

std::ostringstream performanceReport;

const char* suiteName = "MSG";
const char* testName = "TST";

class PerfData
{
  //std::string data;
  std::ostringstream data;
public:
  PerfData(const char* identifier, int val)
  {   add(identifier, val); }
  PerfData& operator()(const char* identifier, int val)
  {  data << ","; add(identifier, val); return *this; }
  void add(const char* identifier, int val)
  {
    data << "('" << identifier << "'," << val << ")";
  }

  PerfData(const char* identifier, double val)
  {  add(identifier, val); }
  PerfData& operator()(const char* identifier, double val)
  { data << ",";  add(identifier, val);  return *this; }
  void add(const char* identifier, double val)
  {
    data << "('" << identifier << "'," << val << ")";
  }

  const std::string str() const { return data.str(); }

  /* void copy(std::string& s)
  {
    std::copy(data.begin(),data.end(),s.begin());
  }
  */
  //void finish() { data << "\0"; }

};

void perfReport(const char* graph,const char* series, PerfData& parameters, PerfData& outputs)
{
  //parameters.finish();
  //printf("perfReport ('%s','%s',%s,%s)\n",graph,series,parameters.str().c_str(),outputs.str().c_str());
  std::string ps = parameters.str();
  std::string os = outputs.str();
  //printf("%s", ps.c_str());
  //printf("%s", os.c_str());
  performanceReport << "('" << graph << "','" << series << "',(" << ps.c_str() << "),(" << os.c_str() << "))\n";
}


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


bool testChunkingPerf(MsgSocket* src, MsgSocket* sink,Handle dest, int msgLen, int msgsPerCall, int numCalls, int numLoops,const char* testIdentifier,bool verify = false)
{
  Message* m;
  unsigned int totalMsgs=0;
  unsigned long seed = 0;
  timer t;

  for (int loop = 0; loop<numLoops; loop++)
    {
      for (int atOnce = msgsPerCall; atOnce <= msgsPerCall; atOnce++)  // for loop just executes once
        {
          for (int size = msgLen; size <= msgLen; size++) // for loop just executes once
            {
              seed++;
              m = src->msgPool->allocMsg();
              assert(m); //clTest(("message allocated"), m != NULL,(" "));
              Message* curm = m;
              int msgCount = 0;
              do
                {
                  curm->setAddress(dest);

                  MsgFragment* frag = curm->append(size);
                  assert(frag); //clTest(("message frag allocated"), frag != NULL,(" "));
                  if (verify)
                    {
                      xorshf96 rnd(seed);
                      unsigned char* buf = (unsigned char*) frag->data();
                      for (int i = 0; i < size; i++,buf++)
                        {
                          *buf = rnd();
                        }
                    }
                  frag->len = size;
                  msgCount++;
                  totalMsgs++;
                  if (msgCount < atOnce) 
                    {
                      curm->nextMsg = src->msgPool->allocMsg();
                      curm = curm->nextMsg;
                    }
                } while (msgCount < atOnce);

              src->send(m);

              msgCount = 0;
              while (msgCount < atOnce)
                {
                  m = sink->receive(1,10);  // Even though a sent multiples, I may not receive them as multiples
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
                      if (verify)
                        {
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
                        }
                      msgCount++;
                      curm=m->nextMsg;
                    }
                  sink->msgPool->free(m);
                }
            }
        }
    }

  double elapsed = t.elapsed();
  // Bandwidth measurements are doubled because the program sends AND receives each message
  printf("%s: len [%6d] stride [%6d] Bandwidth [%8.2f msg/s, %8.2f MB/s]\n", testIdentifier, msgLen, msgsPerCall, 2*((double) totalMsgs)/elapsed, 2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0);

  perfReport("Message Performance", testIdentifier, PerfData("length",msgLen) ("chunk",msgsPerCall), PerfData("msg/s",2*((double) totalMsgs)/elapsed)("MB/s",2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0));

#if 0
  printf("      : frag allocations: [%" PRIi64 "->%" PRIi64 "] cur allocated: msgs [%" PRIi64 "->%" PRIi64 "] frags[%" PRIi64 "->%" PRIi64 "]\n", 
         src->msgPool->fragCumulativeAllocated,sink->msgPool->fragCumulativeAllocated, 
         src->msgPool->allocated,sink->msgPool->allocated, 
         src->msgPool->fragAllocated,sink->msgPool->fragAllocated); 
#endif
  assert(src->msgPool->fragAllocated == 0);
  assert(sink->msgPool->fragAllocated == 0);
  assert(src->msgPool->allocated == 0);
  assert(sink->msgPool->allocated == 0);
  return true;
}


bool testLatency(MsgSocket* src, MsgSocket* sink,Handle dest, int msgLen, int numLoops,const char* testIdentifier,bool verify = false)
{
  Message* m;
  unsigned int totalMsgs=0;
  unsigned long seed = 0;

  seed++;
  m = src->msgPool->allocMsg();
  assert(m); //clTest(("message allocated"), m != NULL,(" "));
  Message* curm = m;
  int msgCount = 0;

  curm->setAddress(dest);

  MsgFragment* frag = curm->append(msgLen);
  assert(frag); //clTest(("message frag allocated"), frag != NULL,(" "));
  if (verify)
    {
      xorshf96 rnd(seed);
      unsigned char* buf = (unsigned char*) frag->data();
      for (int i = 0; i < msgLen; i++,buf++)
        {
          *buf = rnd();
        }
    }
  frag->len = msgLen;
  msgCount++;

  timer t;
  for (int loop = 0; loop<numLoops; loop++)
    {
      Handle d = m->getAddress();
      assert(d == dest);
      src->send(m);
      m = sink->receive(1,1000);
      assert(m);  /* If you get this then messages were dropped in
                     the kernel.  Use these commands to increase the kernel network buffers:
                     sysctl -w net.core.wmem_max=10485760
                     sysctl -w net.core.rmem_max=10485760
                     sysctl -w net.core.rmem_default=10485760
                     sysctl -w net.core.wmem_default=10485760
                  */
    }
  double elapsed = t.elapsed();

  frag = m->firstFragment;
  assert(frag);
  if (verify)
    {
      xorshf96 rnd1(seed);
      const unsigned char* bufr = (const unsigned char*) frag->read();
      int miscompare=-1;
      for (int i = 0; (i < msgLen) && !miscompare; i++,bufr++)
        {
          if (*bufr != rnd1())
            {
              miscompare = i;
            }
        }
      clTest(("send/recv message ok"),miscompare==-1,("message size [%d] contents miscompare at position: [%d]",msgLen,miscompare));
    }

  sink->msgPool->free(m);

  printf("%s: len [%6d] Latency [%8.6f ms]\n", testIdentifier, msgLen, ((elapsed*1000.0)/(double) numLoops));
#if 0
  printf("      : frag allocations: [%" PRIi64 "->%" PRIi64 "] cur allocated: msgs [%" PRIi64 "->%" PRIi64 "] frags[%" PRIi64 "->%" PRIi64 "]\n", 
         src->msgPool->fragCumulativeAllocated,sink->msgPool->fragCumulativeAllocated, 
         src->msgPool->allocated,sink->msgPool->allocated, 
         src->msgPool->fragAllocated,sink->msgPool->fragAllocated); 
#endif
  assert(src->msgPool->fragAllocated == 0);
  assert(sink->msgPool->fragAllocated == 0);
  assert(src->msgPool->allocated == 0);
  assert(sink->msgPool->allocated == 0);
  return true;
}


void testGroup(MsgSocket* src, MsgSocket* sink,Handle dest,const char* desc)
{
#if 0
  testLatency(src,sink,dest,1,10000 * repeat, desc,true);
  testLatency(src,sink,dest,16,10000 * repeat, desc,true);
  testLatency(src,sink,dest,100,10000 * repeat, desc,true);
  testLatency(src,sink,dest,1000,10000 * repeat, desc,true);
  testLatency(src,sink,dest,10000,10000 * repeat, desc,true);

  testChunkingPerf(src, sink,dest,1, 1 , 1 , 10000 * repeat,desc);
  testChunkingPerf(src, sink,dest,16, 1 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,100, 1 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,1000, 1 , 1 , 2000 * repeat,desc);
  testChunkingPerf(src, sink,dest,10000, 1 , 1 , 1000 * repeat,desc);
  testChunkingPerf(src, sink,dest,50000, 1 , 1 , 1000 * repeat,desc);
#endif
  if (src->cap.maxMsgSize > 200000) testChunkingPerf(src, sink,dest,200000, 1 , 1 , 500 * repeat,desc);

#if 1
  testChunkingPerf(src, sink,dest,1, 10 , 1 , 10000 * repeat,desc);
  testChunkingPerf(src, sink,dest,16, 10 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,100, 10 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,1000, 10 , 1 , 2000 * repeat,desc);
  testChunkingPerf(src, sink,dest,10000, 10, 1 , 1000 * repeat,desc);
  testChunkingPerf(src, sink,dest,50000, 10 , 1 , 1000 * repeat,desc);
  if (src->cap.maxMsgSize > 100000) testChunkingPerf(src, sink,dest,100000, 10 , 1 , 300 * repeat,desc);

  testChunkingPerf(src, sink,dest,1, 50 , 1 , 10000 * repeat,desc);
  testChunkingPerf(src, sink,dest,16, 50 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,100, 50 , 1 , 5000 * repeat,desc);
  testChunkingPerf(src, sink,dest,1000, 50, 1 , 2000 * repeat,desc);
  testChunkingPerf(src, sink,dest,10000, 50, 1 , 1000 * repeat,desc);
  testChunkingPerf(src, sink,dest,50000, 50 , 1 , 1000 * repeat,desc);
  if (src->cap.maxMsgSize > 130000) testChunkingPerf(src, sink,dest,130000, 50 , 1 , 100 * repeat,desc);

  // high perf RPC: This test simulates a server being hit by tons of small RPC calls
  testChunkingPerf(src, sink,dest,32, 500 , 1 , 1000 * repeat,desc); 
#endif
}

class Sock
{
  public:
  //? <ctor> Pass the message transport plugin and port to create your message socket </ctor>
  Sock(MsgTransportPlugin* xp, ::uint_t port) 
    { 
    xport=xp->createSocket(port); 
    if (sar) 
      {
        sarSock = new MsgSarSocket(xport);
        sock = sarSock;
      }
    else
      {
        sarSock = NULL;
        sock = xport;
      }
    }

  ~Sock() 
    { 
      // done by sarSock: xport->transport->deleteSocket(xport);
    if (sarSock) delete sarSock; 
    }
  MsgSocket* xport;
  MsgSocket* sarSock;

      //? Access the MsgSocket object directly
  MsgSocket* sock;
      //? A convenience function that makes the ScopedMsgSocket object behave like a MsgSocket* object.
  MsgSocket* operator->() {return sock;}
};

int main(int argc, char* argv[])
{
  bool testProcess = false;
  bool testNode    = false;
  bool testCluster = false;
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  std::string xport("clMsgUdp.so");
  SAFplus::logCompName = "TSTPRF";

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("rnode", po::value<int>(), "reflector node id")
    ("rport", po::value<int>(), "reflector port number")
    ("xport", boost::program_options::value<std::string>(), "transport plugin filename")
    ("sar", boost::program_options::value<bool>()->default_value("false"), "Use segmentation and reassembly layer")
    ("loglevel", boost::program_options::value<std::string>(), "logging cutoff level")
    ("repeat", boost::program_options::value<int>(), "repeat factor: bigger number means longer test")
    ("variant", po::value<std::string>()->default_value("all"), "use 'process','node', or 'cluster' to select the testing scope.")
    ;

  po::variables_map vm;        
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
      return 0;
    }
  if (vm.count("rnode")) reflectorNode = vm["rnode"].as<int>();
  if (vm.count("rport")) reflectorPort = vm["rport"].as<int>();
  if (vm.count("xport")) xport = vm["xport"].as<std::string>();
  if (vm.count("repeat")) repeat = vm["repeat"].as<int>();
  if (vm.count("loglevel")) SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());
  if (vm.count("sar"))
    {
      sar = vm["sar"].as<bool>();
      if (sar) suiteName = "SAR";
    }
  if (vm.count("variant")) 
    {
    std::string s = vm["variant"].as<std::string>();
    if (s == "process") testProcess = true;
    if (s == "node") testNode = true;
    if (s == "cluster") testCluster = true;
    if (s == "all") testProcess = testNode = testCluster = true; 
    }
  else
   {
   testProcess = testNode = true;
   }

  //logEchoToFd = 1; // stdout

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

  logInfo(testName,suiteName,"Msg Transport [%s], node [%u] maxPort [%u] maxMsgSize [%u] reflector node [%d] reflector port [%d]", xp->type, xCfg.nodeId, xCfg.maxPort, xCfg.maxMsgSize, reflectorNode, reflectorPort);
  if (testProcess)
    {
      logInfo(testName,suiteName,"Loopback to same process");
      Sock a(xp,1);
      testGroup(a.sock,a.sock,a->handle(),"same process");
    }

  if (testNode)
    {
      logInfo(testName,suiteName,"Different process, same node: run the msgReflector program on this node");

      Sock a(xp,1);
      testGroup(a.sock,a.sock,getProcessHandle(reflectorPort,a->node),"inter-process");
    }

  if (testCluster)
    {
      logInfo(testName,suiteName,"Different process, different node: run the msgReflector program on node %d", reflectorNode);
      Sock a(xp,1);
      testGroup(a.sock,a.sock,getProcessHandle(reflectorPort,reflectorNode),"inter-node");
    }

  std::cout << "\n<metrics>\n" << performanceReport.str() << "\n</metrics>\n";
}
