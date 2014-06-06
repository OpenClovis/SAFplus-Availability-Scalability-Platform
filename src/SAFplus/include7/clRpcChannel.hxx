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

#ifndef CLRPCCHANNEL_HXX_
#define CLRPCCHANNEL_HXX_

#include <google/protobuf/service.h>
#include "SAFplusRpc.pb.h"
#include "clMsgHandler.hxx"
#include <clThreadApi.hxx>

namespace SAFplus
  {
    class MsgServer;
    namespace Rpc
      {
        class MsgRpcEntry
          {
          public:
            uint64_t msgId;
            ClIocAddressT srcAddr;
            google::protobuf::Closure *callback;
            google::protobuf::Message *response;
          };

        /*
         *
         */
        class RpcChannel : public google::protobuf::RpcChannel, public SAFplus::MsgHandler
          {
          public:
            //Client
            explicit RpcChannel(SAFplus::MsgServer *, ClIocAddressT iocDest);

            //Server
            explicit RpcChannel(SAFplus::MsgServer *, google::protobuf::Service *svr);

            virtual ~RpcChannel();

            void CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController* controller,
                const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done);

            //Register with msg server to handle RPC protocol
            void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

            void HandleRequest(SAFplus::Rpc::RpcMessage *msg, ClIocAddressT *iocReq);
            void HandleResponse(SAFplus::Rpc::RpcMessage *msg);
            void RequestComplete(MsgRpcEntry *rpcRequestEntry);

          public:
            /*
             * Id of the next sending message
             */
            uint64_t msgId;
            /*
             * List of IOC sending records
             */
            std::map<uint64_t,MsgRpcEntry*> msgRPCs;

            SAFplus::MsgServer *svr;
            ClIocAddressT dest;
            Mutex mutex;
            google::protobuf::Service *service;  // service to dispatch requests to

          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCCHANNEL_HXX_ */
