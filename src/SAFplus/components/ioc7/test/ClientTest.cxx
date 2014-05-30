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
#include "clSafplusMsgServer.hxx"
#include "rpcTest.pb.h"

using namespace std;
using namespace SAFplus;

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

    SAFplusService::rpcTest::TestGetRpcMethodRequest request;

    request.set_name("testRpc_request");

    /*
     * ??? msgClient or safplusMsgServer
     */
    SafplusMsgServer msgClient(IOC_PORT);

    /* Loop receive on loop */

    msgClient.Start();
    int i = 0;
    while (1)
    {
        string data;
        request.SerializeToString(&data);
        MsgReply *msgReply = msgClient.sendReply(iocDest, (void *) data.c_str(), request.ByteSize(), CL_IOC_PROTO_CTL);

        SAFplusService::rpcTest::TestGetRpcMethodResponse res;
        res.ParseFromString(msgReply->buffer);
        std::cout<<"Process:"<<getpid()<<", GOT REPLY:"<<std::endl<<res.DebugString()<<std::endl;
        sleep(3);
    }
}

