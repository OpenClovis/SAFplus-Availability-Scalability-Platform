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
 */
#include <clLogApi.hxx>
#include <clGlobals.hxx>

#include "clSafplusMsgServer.hxx"
#include "clRpcChannel.hxx"
#include "rpcTest.hxx"
#include "../RpcWakeable.hxx"

using namespace std;
using namespace SAFplus;

#define IOC_PORT_SERVER 65

int main(void)
  {
    logSeverity = LOG_SEV_MAX;

    clMsgInitialize();

    //Msg server listening
    SAFplus::SafplusMsgServer safplusMsgServer(IOC_PORT_SERVER, 10, 10);

    // Handle RPC
    SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, new SAFplus::Rpc::rpcTest::rpcTestImpl());
    channel->setMsgType(100, 101);

    safplusMsgServer.Start();

    // client side
    SAFplus::Rpc::rpcTest::rpcTest_Stub rpcTestService(channel);

    safplusMsgServer.Start();

    //Test RPC
    //Loop forever
    while (1)
      {
        Rpc::RpcWakeable wakeable1(1);
        Rpc::RpcWakeable wakeable2(2);
        Rpc::RpcWakeable wakeable3(3);

        //DATA request
        SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest request1;
        SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse res1;
        request1.set_name("myNameRequest1");

        SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request request2;
        SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response res2;

        request2.set_name("myNameRequest2");

        SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request request3;
        SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response res3;

        request3.set_name("myNameRequest3");

        rpcTestService.testGetRpcMethod(safplusMsgServer.handle, &request1, &res1, wakeable1);
        rpcTestService.testGetRpcMethod2(safplusMsgServer.handle, &request2, &res2, wakeable2);
        rpcTestService.testGetRpcMethod3(safplusMsgServer.handle, &request3, &res3, wakeable3);

        sleep(1);
      }

  }

