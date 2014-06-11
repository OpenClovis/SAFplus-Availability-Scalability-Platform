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

#ifndef RPCWAKEABLE_HXX_
#define RPCWAKEABLE_HXX_

#include <iostream>
#include "clCommon.hxx"

namespace SAFplus
  {
    namespace Rpc
      {

        /*
         *
         */
        class RpcWakeable : public SAFplus::Wakeable
          {
          public:
            RpcWakeable();
            virtual ~RpcWakeable();

            void wake(int amt, void* cookie = NULL)
              {
                //TODO:
                //1. Send cookie to source
                //2. Remove an entry in RPCs list

//                 RpcMessage rpcMsg;
//
//                 rpcMsg.set_type(msgReplyType);
//                 rpcMsg.set_id(rpcRequestEntry->msgId);
//                 rpcMsg.set_buffer(rpcRequestEntry->response->SerializePartialAsString());
//
//                 //Sending reply
//                 try
//                 {
//                 svr->SendMsg(rpcRequestEntry->srcAddr, (void *) rpcMsg.SerializeAsString().c_str(), rpcMsg.ByteSize(), msgReplyType);
//                 }
//                 catch (...)
//                 {
//                 }
//
//                 //Remove a RPC request entry
//                 msgRPCs.erase(rpcRequestEntry->msgId);
//                 if (msgId > 0)
//                 msgId--;
//
//                 delete rpcRequestEntry->response;
//                 delete rpcRequestEntry;

                std::cout << "RPC Wakeable test!" << std::endl;
              }
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCWAKEABLE_HXX_ */
