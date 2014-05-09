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
        char helloMsg[] = "Hello world reply";

        string recMsg((const char*) msg, msglen);

        testprotobuf::Person *person = new testprotobuf::Person;

        person->ParseFromString(recMsg);

        std::cout<<person->name()<<" - "<<person->id()<<std::endl;

        cout << "==> Handle for message: " << person->DebugString() << " from [" << std::hex << "0x" << from.iocPhyAddress.nodeAddress << ":"
                << std::hex << "0x" << from.iocPhyAddress.portId << "]" << endl;

        /**
         * TODO:
         * Reply, need to check message type to reply
         * Maybe sync queue, Async callback etc
         */
        recMsg.append(":").append(helloMsg);
        try
        {
            svr->SendMsg(from, (void *)recMsg.c_str(), recMsg.length(), CL_IOC_SAF_MSG_REPLY_PROTO);
        }
        catch (...)
        {
        }
    }

} /* namespace SAFplus */
