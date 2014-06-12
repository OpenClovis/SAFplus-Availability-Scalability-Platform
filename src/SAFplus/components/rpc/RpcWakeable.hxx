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

namespace SAFplus
  {
    namespace Rpc
      {

      class RpcChannel;
      class MsgRpcEntry;

        /*
         *
         */
        class RpcWakeable : public SAFplus::Wakeable
          {
          public:
            RpcWakeable();
            RpcWakeable(RpcChannel *ch, MsgRpcEntry *rpcRequestEntry): channel(ch), rpcRequestEntry(rpcRequestEntry) {};
            virtual ~RpcWakeable();
            void RequestComplete(MsgRpcEntry *rpcRequestEntry);
            void wake(int amt, void* cookie = NULL);

          public:
            RpcChannel *channel;
            MsgRpcEntry *rpcRequestEntry;
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCWAKEABLE_HXX_ */
