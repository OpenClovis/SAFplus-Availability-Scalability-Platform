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
#pragma once

#ifndef MYRPCCHANNEL_HXX_
#define MYRPCCHANNEL_HXX_

#include <google/protobuf/service.h>
#include "rpcTest.pb.h"
#include "SAFplusRpc.pb.h"
#include "clSafplusMsgServer.hxx"

using namespace google::protobuf;

namespace SAFplus
{
  namespace Rpc
  {
    namespace rpcTest
    {

      /*
       *
       */
      class MyRpcChannel : public google::protobuf::RpcChannel, public SAFplus::MsgHandler
      {
        public:
          MyRpcChannel(SAFplus::MsgServer *svr, ClIocAddressT *iocDest);
          virtual
          ~MyRpcChannel();
          void CallMethod(const MethodDescriptor* method,
                                    RpcController* controller,
                                    const Message* request,
                                    Message* response,
                                    Closure* done);
          void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
          static void FooDone(google::protobuf::Message* response);
        public:
          SAFplus::MsgServer *svr;
          ClIocAddressT *dest;
          //Msg index
          int msgId;
      };

    } /* namespace rpcTest */
  } /* namespace Rpc */
} /* namespace SAFplus */
#endif /* MYRPCCHANNEL_HXX_ */
