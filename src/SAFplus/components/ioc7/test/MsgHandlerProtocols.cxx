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
#include "MsgHandlerProtocols.hxx"
#include "clMsgServer.hxx"
#include "rpcTest.pb.h"

using namespace std;


namespace SAFplus
{


    MsgHandlerProtocols::MsgHandlerProtocols()
    {
        // TODO Auto-generated constructor stub

    }

    MsgHandlerProtocols::~MsgHandlerProtocols()
    {
        // TODO Auto-generated destructor stub
    }

    void
    MsgHandlerProtocols::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        //char helloMsg[] = "Hello world reply";

        string recMsg((const char*) msg, msglen);

        SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest req;

        req.ParseFromString(recMsg);

        cout << "==> Handle for message: "<<endl<< req.DebugString() <<" from [" << std::hex << "0x" << from.iocPhyAddress.nodeAddress << ":"
                << std::hex << "0x" << from.iocPhyAddress.portId << "]" << endl;

        SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse res;

        /* Initialize data response */
        SAFplus::Rpc::rpcTest::DataResult *data = res.mutable_dataresult();
        data->set_name("testRpc_response");
        data->set_status(1);

        /**
         * TODO:
         * Reply, need to check message type to reply
         * Maybe sync queue, Async callback etc
         */
        try
        {
            svr->SendMsg(from, (void *)res.SerializeAsString().c_str(), res.ByteSize(), CL_IOC_SAF_MSG_REPLY_PROTO);
        }
        catch (...)
        {
        }
    }

} /* namespace SAFplus */
