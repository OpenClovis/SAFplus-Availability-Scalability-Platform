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
#ifndef CLRPCAPI_HXX_
#define CLRPCAPI_HXX_

#include <map>
#include "clThreadApi.hxx"
#include "clMsgHandler.hxx"

//? <section name="Remote Procedure Calls">

namespace google {
  namespace protobuf
    {
      class MethodDescriptor;
      class Message;
    }
}

namespace SAFplus
  {
    class MsgServer;
    class Wakeable;
    class MsgHandler;
    class Handle;

    namespace Rpc
      {

        class RpcService;
        class RpcMessage;

        class MsgRpcEntry
          {
          public:
            uint64_t msgId;
            SAFplus::Handle srcAddr;
            SAFplus::Wakeable *callback;
            google::protobuf::Message *response;
          };

        //? <class> This class sends and receives remote procedure call messages.  It sits under the classes auto-generated by SAFplus that handle your RPC calls.  Typically this class instantiated but then not directly used by the application.
        class RpcChannel : public SAFplus::MsgHandler
          {
          public:
            //? <ctor> Create a RPC channel for client or combined client/server operation.
            // For combined operation, manually set .service to the SAFplus::Rpc::RpcService object after construction 
            // <arg name="svr">The messaging transport to use when sending/receiving messages</arg>
            // <arg name="destination">Where RPC requests will be sent</arg>
            // </ctor>
            explicit RpcChannel(SAFplus::MsgServer* svr, SAFplus::Handle destination);

            //? <ctor> Create a RPC channel for the server side.
            // <arg name="svr">The message server to use when sending/receiving messages</arg>
            // <arg name="rpcService">The incoming message handler (generally auto-generated from your .proto or YANG RPC definition file)</arg> </ctor>
            explicit RpcChannel(SAFplus::MsgServer* svr, SAFplus::Rpc::RpcService *rpcService);

            virtual ~RpcChannel();

            void CallMethod(const google::protobuf::MethodDescriptor* method, SAFplus::Handle destination,
                const google::protobuf::Message* request, google::protobuf::Message* response, SAFplus::Wakeable &wakeable);

            // Register with msg server to handle RPC protocol
            void msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie);

            void HandleRequest(SAFplus::Rpc::RpcMessage *msg, SAFplus::Handle from);
            void HandleResponse(SAFplus::Rpc::RpcMessage *msg);

            void setMsgType(ClWordT send, ClWordT reply); //? Set the SAFplus protocol type for underlying transports that require one.  See <ref type="file">clMsgPortsAndTypes.hxx</ref>.
            void setMsgSendType(ClWordT send);
            void setMsgReplyType(ClWordT reply);

          public:
            ClWordT msgSendType;
            ClWordT msgReplyType;
            /*
             * Id of the next sending message
             */
            uint64_t msgId;
            /*
             * List of IOC sending records
             */
            std::map<uint64_t,MsgRpcEntry*> msgRPCs;

            SAFplus::MsgServer *svr;
            SAFplus::Handle dest;
            Mutex mutex;
            SAFplus::Rpc::RpcService *service; // service to dispatch requests to

            static const std::string NO_RESPONSE;
        }; //? </class>

      } /* namespace Rpc */
  } /* namespace SAFplus */

//? </section>

#endif /* CLRPCCHANNEL_HXX_ */

