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
#include <iostream>
#include <sstream>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clIocApi.h>
#include "Common/MyRpcChannel.hxx"
#include "clSafplusMsgServer.hxx"
#include "google/protobuf/service.h"
#include "ClientTest.hxx"
#include "rpcTest.pb.h"


using namespace std;
using namespace SAFplus;
using namespace google::protobuf;

//Auto scanning
#define IOC_PORT 0
#define IOC_PORT_SERVER 65

ClUint32T clAspLocalId = 0x1;
ClBoolT gIsNodeRepresentative = CL_FALSE;

int
main(void)
{
    ClIocAddressT iocDest;

    ClRcT rc = CL_OK;

    /*
     * initialize SAFplus libraries 
     */
    if ((rc = clOsalInitialize(NULL)) != CL_OK || (rc = clHeapInit()) != CL_OK || (rc = clTimerInitialize(NULL)) != CL_OK || (rc =
                    clBufferInitialize(NULL)) != CL_OK)
    {

    }

    rc = clIocLibInitialize(NULL);
    assert(rc==CL_OK);

    iocDest.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    iocDest.iocPhyAddress.portId = IOC_PORT_SERVER;
    char helloMsg[] = "Hello world ";

    SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest request;
    SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse res;

    request.set_name("myNameRequest");

    /*
     * ??? msgClient or safplusMsgServer
     */
    SafplusMsgServer msgClient(IOC_PORT);

    /* Loop receive on loop */

//    int i = 0;
//    while (i++<3)
//    {
//        string data;
//        request.SerializeToString(&data);
//        MsgReply *msgReply = msgClient.sendReply(iocDest, (void *) data.c_str(), request.ByteSize(), CL_IOC_PROTO_CTL);
//        res.ParseFromString(msgReply->buffer);
//        std::cout<<"Process:"<<getpid()<<", GOT REPLY:"<<std::endl<<res.DebugString()<<std::endl;
//        sleep(3);
//    }

    SAFplus::Rpc::rpcTest::MyRpcChannel * channel = new SAFplus::Rpc::rpcTest::MyRpcChannel(&msgClient, &iocDest);
    SAFplus::Rpc::rpcTest::rpcTest *service = new SAFplus::Rpc::rpcTest::rpcTest::Stub(channel);

    msgClient.Start();

    //Test RPC
    //Loop forever
    while(1)
    {
      service->testGetRpcMethod(NULL, &request, &res, NULL);
      sleep(3);
    }



}

