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

#include <clIocProtocols.h>
#include "clSafplusMsgServer.hxx"
#include "rpcTest.pb.h"
#include "clRpcChannel.hxx"
#include "rpcTest.hxx"
#include "../RpcWakeable.hxx"

using namespace SAFplus;

#define IOC_PORT_SERVER 65

ClUint32T clAspLocalId = 0x1;
ClBoolT gIsNodeRepresentative = CL_TRUE;

void FooDone(SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response)
  {
    if (response->has_dataresult())
      {
        const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
        printf("Response is name='%s': status='%d'\n", dr.name().c_str(), dr.status());
      }
  }

void FooDone2(SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response)
  {
    if (response->has_dataresult())
      {
        const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
        printf("Response is name='%s': status='%d'\n", dr.name().c_str(), dr.status());
      }
  }

void FooDone3(SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response)
  {
    if (response->has_dataresult())
      {
        const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
        printf("Response is name='%s': status='%d'\n", dr.name().c_str(), dr.status());
      }
  }

int main(void)
  {
    ClRcT rc = CL_OK;

    /*
     * initialize SAFplus libraries
     */
    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc =
        clBufferInitialize(NULL)) != CL_OK)
      {

      }

    clIocLibInitialize(NULL);

    //Msg server listening
    SAFplus::SafplusMsgServer safplusMsgServer(IOC_PORT_SERVER, 10, 10);

    // Handle RPC
    SAFplus::Rpc::RpcChannel *channelServer = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, new SAFplus::Rpc::rpcTest::rpcTestImpl());
    channelServer->setMsgType(100, 101);

    ClIocAddressT iocDest;
    iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    iocDest.iocPhyAddress.portId = IOC_PORT_SERVER;

    SAFplus::Rpc::RpcChannel *channelClient = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, iocDest);
    channelClient->setMsgType(100, 101);
    SAFplus::Rpc::rpcTest::rpcTest *service = new SAFplus::Rpc::rpcTest::rpcTest::Stub(channelClient);

    safplusMsgServer.Start();

    //DATA request
    SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest request;
    SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse res;
    request.set_name("myNameRequest1");

    SAFplus::Rpc::rpcTest::TestGetRpcMethod2Request request2;
    SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response res2;

    request.set_name("myNameRequest2");

    SAFplus::Rpc::rpcTest::TestGetRpcMethod3Request request3;
    SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response res3;

    request.set_name("myNameRequest3");

    //Test RPC
    //Loop forever
    while (1)
      {
        SAFplus::Rpc::RpcWakeable wakeable;
        SAFplus::Handle hdl = SAFplus::Handle::create();

        service->testGetRpcMethod(hdl, &request, &res, wakeable);
        service->testGetRpcMethod2(hdl, &request2, &res2, wakeable);
        service->testGetRpcMethod3(hdl, &request3, &res3, wakeable);

        sleep(3);
      }

  }

