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

#include "MyRpcChannel.hxx"

namespace SAFplus
{
  namespace Rpc
  {
    namespace rpcTest
    {

      MyRpcChannel::MyRpcChannel(SAFplus::MsgServer *svr, ClIocAddressT *iocDest) : svr(svr), dest(iocDest), msgId(0)
      {
        // TODO Auto-generated constructor stub

      }

      MyRpcChannel::~MyRpcChannel()
      {
        // TODO Auto-generated destructor stub
      }

      void
      MyRpcChannel::CallMethod(const MethodDescriptor* method, RpcController* controller, const Message* request, Message* response,
          Closure* done)
      {
        SAFplus::Rpc::RpcRequest req;
        req.set_type(RequestType::ASYNC);
        req.set_id(msgId++);
        req.set_name(method->name());
        req.set_buffer(request->SerializeAsString());
        svr->SendMsg(*dest, (void *)req.SerializeAsString().c_str(), req.ByteSize(), 111);

      }

      void
      MyRpcChannel::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
      {
        std::cout<<"Handle respone:"<<std::endl;
        SAFplus::Rpc::RpcResponse res;
        res.ParseFromString(std::string((const char*)msg, msglen));
        std::cout<<res.DebugString()<<std::endl;
        msgId--;
      }
    } /* namespace rpcTest */
  } /* namespace Rpc */
} /* namespace SAFplus */
