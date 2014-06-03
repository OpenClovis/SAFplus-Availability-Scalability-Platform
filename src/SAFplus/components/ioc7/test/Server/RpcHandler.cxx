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

#include "clMsgServer.hxx"
#include "RpcHandler.hxx"
#include <iostream>
#include <string>
#include "SAFplusRpc.pb.h"
#include "google/protobuf/service.h"
#include "rpcTest.pb.h"

namespace SAFplus
{
  namespace Rpc
  {
    namespace rpcTest
    {

      RpcHandler::RpcHandler()
      {
        // TODO Auto-generated constructor stub

      }

      RpcHandler::~RpcHandler()
      {
        // TODO Auto-generated destructor stub
      }

      void
      RpcHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
      {
        std::cout << "Receive RPC call:" << std::endl;

        //Wrapper RPC msg
        SAFplus::Rpc::RpcRequest req;
        std::string recMsg((const char*) msg, msglen);
        req.ParseFromString(recMsg);

        //Header
        std::cout << req.DebugString() << std::endl;

        SAFplus::Rpc::RpcResponse res;

        SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse testMethodRes;
        SAFplus::Rpc::rpcTest::DataResult *data = testMethodRes.mutable_dataresult();
        data->set_name("Get status reply callback");
        data->set_status(1);

        //Wrapper RPC reply msg
        res.set_id(req.id());
        res.set_buffer(testMethodRes.SerializeAsString());

        try
        {
            svr->SendMsg(from, (void *)res.SerializeAsString().c_str(), res.ByteSize(), 111);
        }
        catch (...)
        {
        }
      }

    } /* namespace rpcTest */
  } /* namespace Rpc */
} /* namespace SAFplus */
