/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.

To avoid losing messages and causing this test to hang:

sysctl -w net.core.wmem_max=20485760
sysctl -w net.core.rmem_max=20485760
sysctl -w net.core.rmem_default=20485760
sysctl -w net.core.wmem_default=20485760
 */
#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/timer.hpp>

#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include "clTestApi.hxx"
#include "clSafplusMsgServer.hxx"
#include "clRpcChannel.hxx"
#include "reflection/msgReflection.hxx"
#include "../RpcWakeable.hxx"

using namespace std;
using namespace boost;
using namespace SAFplus;

int SERVER_MSG_NODE=0;
int SERVER_MSG_PORT=65;
int MY_MSG_PORT=66;

bool dropIsFailure = true;

int loopCount=200;

boost::program_options::variables_map parseOptions(int argc,char *argv[])
{
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("rnode", boost::program_options::value<int>()->default_value(SERVER_MSG_NODE), "reflector node id")
    ("rport", boost::program_options::value<int>()->default_value(SERVER_MSG_PORT), "reflector port number")
    ("port", boost::program_options::value<int>()->default_value(MY_MSG_PORT), "my port number")
    ("loglevel", boost::program_options::value<std::string>(), "logging cutoff level")
    ("count", boost::program_options::value<int>()->default_value(loopCount))
    ;

  boost::program_options::variables_map vm;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    
  
  if (vm.count("rnode")) SERVER_MSG_NODE = vm["rnode"].as<int>();
  if (vm.count("rport")) SERVER_MSG_PORT = vm["rport"].as<int>();
  if (vm.count("port")) MY_MSG_PORT = vm["port"].as<int>();
  if (vm.count("count")) loopCount = vm["count"].as<int>();
  if (vm.count("loglevel")) SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
    }
  return vm;
}




bool testLatency(Handle dest, int msgLen, int atOnce, int numLoops,const char* testIdentifier,bool verify = false)
{
    uint drops = 0;
    SAFplus::Rpc::RpcChannel channel(&safplusMsgServer, dest);
    channel.setMsgType(100, 101);
    unsigned long int totalMsgs=0;
    SAFplus::Rpc::msgReflection::msgReflection_Stub reflectorService(&channel);

    std::string msg(msgLen,'A');

    timer t;
    SAFplus::Rpc::msgReflection::CallResponse* resp = new SAFplus::Rpc::msgReflection::CallResponse[atOnce];
    for(int loop=0;loop<numLoops;loop++)
      {
        ThreadSem wakeable(0);

        //DATA request
        SAFplus::Rpc::msgReflection::CallRequest req;
        //SAFplus::Rpc::msgReflection::CallResponse* resp = new SAFplus::Rpc::msgReflection::CallResponse[atOnce];
        req.set_data(msg);

        for (int simul=0; simul<atOnce; simul++)
          {
          //reflectorService.call(safplusMsgServer.handle, &req, &resp, wakeable);
          reflectorService.call(dest, &req, &resp[simul], wakeable);
          totalMsgs++;
          }
        bool ok = wakeable.timed_lock(1000, atOnce);  // Bring it back to 0 for the next loop.
        if (!ok)
          {
            if (dropIsFailure) clTestFailed(("Message drop!  Only received %d msgs", atOnce-wakeable.count()));
            drops += atOnce-wakeable.count();
            wakeable.lock(wakeable.count());
            clTestCaseMalfunction(("Message drops exceeded tolerance"),drops > 25, break);
          }
        else clTestSuccess(("received %d msgs", atOnce));
        //delete[] resp;
      }
    delete[] resp;
    double elapsed = t.elapsed();

    // Bandwidth measurements are doubled because the program sends AND receives each message
    printf("%s: len [%6d] stride [%6d] Bandwidth [%8.2f msg/s, %8.2f MB/s]\n", testIdentifier, msgLen, atOnce, 2*((double) totalMsgs)/elapsed, 2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0);

    //perfReport("Message Performance", testIdentifier, PerfData("length",msgLen) ("chunk",msgsPerCall), PerfData("msg/s",2*((double) totalMsgs)/elapsed)("MB/s",2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0));

    return true;
}

int main(int argc, char* argv[])
{
  logSeverity = LOG_SEV_DEBUG;

  boost::program_options::variables_map vm = parseOptions(argc,argv);
  if (vm.count("help")) return 0;  // Help already printed by parseOptions

  clTestGroupInitialize(("remote procedure call"));

  SafplusInitializationConfiguration sic;
  sic.iocPort     = MY_MSG_PORT;
  sic.msgQueueLen = 1000;
  sic.msgThreads  = 30;
  safplusInitialize( SAFplus::LibDep::MSG, sic);

  //Msg server listening
  //safplusMsgServer.init(MY_MSG_PORT, 1000, 30);

  // sending to a different port on this node
  Handle msgDest = getProcessHandle(SERVER_MSG_PORT,SERVER_MSG_NODE);
 
  safplusMsgServer.Start();

  //testLatency(msgDest, 100, 1, 10000,"100 bytes by 1");
  //testLatency(msgDest, 1000, 1, 10000,"1000 bytes by 1");
  clTestCase(("RPC-PRF-TST.TC001: 10000 bytes by 1"), testLatency(msgDest, 10000, 1, loopCount,"10000 bytes by 1"));

  clTestCase(("RPC-PRF-TST.TC002: 100 bytes by 10"),testLatency(msgDest, 100, 10, loopCount,"100 bytes by 10"));
  clTestCase(("RPC-PRF-TST.TC003: 1000 bytes by 10"),testLatency(msgDest, 1000, 10, loopCount,"1000 bytes by 10"));
  clTestCase(("RPC-PRF-TST.TC004: 10000 bytes by 10"),testLatency(msgDest, 10000, 10, loopCount,"10000 bytes by 10"));

  clTestCase(("RPC-PRF-TST.TC005: 100 bytes by 100"),testLatency(msgDest, 100, 100, loopCount,"100 bytes by 100"));
  clTestCase(("RPC-PRF-TST.TC005: 10000 bytes by 100"),testLatency(msgDest, 1000, 100, loopCount,"1000 bytes by 100"));
  clTestCase(("RPC-PRF-TST.TC005: 10000 bytes by 100"),testLatency(msgDest, 10000, 100, loopCount,"10000 bytes by 100"));

  //testLatency(msgDest, 10000, 100, 10000000,"10000 bytes by 10");
  //testLatency(msgDest, 10000, 100, 900000000,"10000 bytes by 10");


#if 0
    //SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, new SAFplus::Rpc::msgReflection::msgReflectionImpl());
    SAFplus::Rpc::RpcChannel* channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, msgDest);
    channel->setMsgType(100, 101);
    // client side
    SAFplus::Rpc::msgReflection::msgReflection_Stub reflectorService(channel);

    //Test RPC
    //Loop forever
    while (1)
      {
        ThreadSem wakeable(0);

        //DATA request
        SAFplus::Rpc::msgReflection::CallRequest req;
        SAFplus::Rpc::msgReflection::CallResponse resp;
        req.set_data();

        //reflectorService.call(safplusMsgServer.handle, &req, &resp, wakeable);
        reflectorService.call(msgDest, &req, &resp, wakeable);
        wakeable.lock();
        
        //        sleep(1);
      }
#endif

  clTestGroupFinalize();
  }

