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
#include "clRpcChannel.hxx"
#include "RpcWakeable.hxx"
#include "SAFplusRpc.pb.h"
#include "SAFplusPBExt.pb.h"

namespace SAFplus
  {
    namespace Rpc
      {

        RpcWakeable::RpcWakeable()
          {
            channel = NULL;
            rpcRequestEntry = NULL;
          }

        RpcWakeable::~RpcWakeable()
          {
          }

        void RpcWakeable::wake(int amt, void* cookie)
          {
            if (channel != NULL && rpcRequestEntry)
              {
                std::cout<<"Wakeable to send msg!"<<std::endl;
                RequestComplete(rpcRequestEntry);
              }
          }

        void RpcWakeable::RequestComplete(MsgRpcEntry* rpcRequestEntry)
          {
            RpcMessage rpcMsg;

            rpcMsg.set_type(channel->msgReplyType);
            rpcMsg.set_id(rpcRequestEntry->msgId);
            rpcMsg.set_buffer(rpcRequestEntry->response->SerializePartialAsString());

            std::cout<<"DEBUG:"<<rpcMsg.DebugString()<<std::endl;

            //Sending reply
            try
              {
                channel->svr->SendMsg(rpcRequestEntry->srcAddr, (void *) rpcMsg.SerializeAsString().c_str(), rpcMsg.ByteSize(), channel->msgReplyType);
              }
            catch (...)
              {
              }

            //Remove a RPC request entry
            channel->msgRPCs.erase(rpcRequestEntry->msgId);
            if (channel->msgId > 0)
              channel->msgId--;

            delete rpcRequestEntry->response;
            delete rpcRequestEntry;
          }

      } /* namespace Rpc */
  } /* namespace SAFplus */
