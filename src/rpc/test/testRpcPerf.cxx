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

sysctl -w net.core.wmem_max=10485760
sysctl -w net.core.rmem_max=10485760
sysctl -w net.core.rmem_default=10485760
sysctl -w net.core.wmem_default=10485760
 */
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <boost/timer.hpp>

#include "clSafplusMsgServer.hxx"
#include "clRpcChannel.hxx"
#include "reflection/msgReflection.hxx"
#include "../RpcWakeable.hxx"

using namespace std;
using namespace boost;
using namespace SAFplus;

#define SERVER_MSG_PORT 65
#define MY_MSG_PORT 66


bool testLatency(Handle dest, int msgLen, int atOnce, int numLoops,const char* testIdentifier,bool verify = false)
{
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
        wakeable.lock(atOnce);  // Bring it back to 0 for the next loop.
        //delete[] resp;
      }
    delete[] resp;
    double elapsed = t.elapsed();

    // Bandwidth measurements are doubled because the program sends AND receives each message
    printf("%s: len [%6d] stride [%6d] Bandwidth [%8.2f msg/s, %8.2f MB/s]\n", testIdentifier, msgLen, atOnce, 2*((double) totalMsgs)/elapsed, 2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0);

    //perfReport("Message Performance", testIdentifier, PerfData("length",msgLen) ("chunk",msgsPerCall), PerfData("msg/s",2*((double) totalMsgs)/elapsed)("MB/s",2*(((double)(totalMsgs*msgLen*8))/elapsed)/1000000.0));

    return true;
}

int main(void)
{
  logSeverity = LOG_SEV_DEBUG;

  clMsgInitialize();

  //Msg server listening
  safplusMsgServer.init(MY_MSG_PORT, 1000, 30);

  // sending to a different port on this node
  Handle msgDest = Handle::create(SERVER_MSG_PORT);
 
  safplusMsgServer.Start();

  testLatency(msgDest, 100, 1, 10000,"100 bytes by 1");
  testLatency(msgDest, 1000, 1, 10000,"1000 bytes by 1");
  testLatency(msgDest, 10000, 1, 10000,"10000 bytes by 1");

  testLatency(msgDest, 100, 10, 1000,"100 bytes by 10");
  testLatency(msgDest, 1000, 10, 1000,"1000 bytes by 10");
  testLatency(msgDest, 10000, 10, 1000,"10000 bytes by 10");

  testLatency(msgDest, 100, 100, 1000,"100 bytes by 10");
  testLatency(msgDest, 1000, 100, 100000,"1000 bytes by 10");
  testLatency(msgDest, 10000, 100, 1000000,"10000 bytes by 10");


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

  }

