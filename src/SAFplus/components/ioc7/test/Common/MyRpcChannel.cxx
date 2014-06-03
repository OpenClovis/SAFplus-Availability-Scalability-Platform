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
#include "MyRpcChannel.hxx"
#include "Server/RpcTestImpl.hxx"

namespace SAFplus
{
  namespace Rpc
  {
    namespace rpcTest
    {
      MyRpcChannel::MyRpcChannel(SAFplus::MsgServer *svr, ClIocAddressT *iocDest) : svr(svr), dest(iocDest), msgId(0)
      {
        // TODO Auto-generated constructor stub
        // Register with sever handler
        svr->RegisterHandler(CL_IOC_RMD_SYNC_REQUEST_PROTO, this, NULL);
        svr->RegisterHandler(CL_IOC_RMD_SYNC_REPLY_PROTO, this, NULL);
        svr->RegisterHandler(CL_IOC_RMD_ASYNC_REQUEST_PROTO, this, NULL);
        svr->RegisterHandler(CL_IOC_RMD_ASYNC_REPLY_PROTO, this, NULL);
      }

      MyRpcChannel::~MyRpcChannel()
      {
        // TODO Auto-generated destructor stub
      }

      void
      MyRpcChannel::CallMethod(const MethodDescriptor* method, RpcController* controller, const Message* request, google::protobuf::Message* response,
          Closure* done)
      {
        SAFplus::Rpc::RpcMessage rpcMsg;
        rpcMsg.set_type(RequestType::CL_IOC_RMD_SYNC_REQUEST_PROTO);
        rpcMsg.set_id(msgId++);
        rpcMsg.set_name(method->name());
        rpcMsg.set_buffer(request->SerializeAsString());
        try
        {
          svr->SendMsg(*dest, (void *) rpcMsg.SerializeAsString().c_str(), rpcMsg.ByteSize(), CL_IOC_RMD_SYNC_REQUEST_PROTO);
        }
        catch (...)
        {
        }
      }

      void
      MyRpcChannel::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
      {
        SAFplus::Rpc::RpcMessage rpcMsg;
        SAFplus::Rpc::RpcMessage response;

        std::string recMsg((const char*) msg, msglen);
        rpcMsg.ParseFromString(recMsg);

        switch (rpcMsg.type())
        {
          // Handler request sync
          case RequestType::CL_IOC_RMD_SYNC_REQUEST_PROTO:
          {
            response.set_type(RequestType::CL_IOC_RMD_SYNC_REPLY_PROTO);
            response.set_id(rpcMsg.id());
            response.set_name(rpcMsg.name());

            SAFplus::Rpc::rpcTest::rpcTestImpl *service = new SAFplus::Rpc::rpcTest::rpcTestImpl();

            const MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsg.name());
            Message* request = service->GetRequestPrototype(method).New();
            Message* testMethodRes = service->GetResponsePrototype(method).New();
            request->ParseFromString(rpcMsg.buffer());
            service->CallMethod(method, NULL, request, testMethodRes, NULL);

            //Done preparing data
            response.set_buffer(testMethodRes->SerializePartialAsString());

            try
            {
                svr->SendMsg(from, (void *)response.SerializeAsString().c_str(), response.ByteSize(), CL_IOC_RMD_SYNC_REPLY_PROTO);
            }
            catch (...)
            {
            }
            break;
          }

          // Handler request async
          case RequestType::CL_IOC_RMD_ASYNC_REQUEST_PROTO:
          {
            response.set_type(RequestType::CL_IOC_RMD_ASYNC_REPLY_PROTO);
            response.set_id(rpcMsg.id());
            response.set_name(rpcMsg.name());

            SAFplus::Rpc::rpcTest::rpcTestImpl *service = new SAFplus::Rpc::rpcTest::rpcTestImpl();

            const MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsg.name());
            Message* request = service->GetRequestPrototype(method).New();
            Message* testMethodRes = service->GetResponsePrototype(method).New();
            request->ParseFromString(rpcMsg.buffer());
            service->CallMethod(method, NULL, request, testMethodRes, NULL);

            //Done preparing data
            response.set_buffer(testMethodRes->SerializePartialAsString());

            try
            {
                svr->SendMsg(from, (void *)response.SerializeAsString().c_str(), response.ByteSize(), CL_IOC_RMD_ASYNC_REPLY_PROTO);
            }
            catch (...)
            {
            }
            break;
          }

          // Handle response sync
          case RequestType::CL_IOC_RMD_SYNC_REPLY_PROTO:
          {
            //TODO: Handle response data
            // => Calling responeHandler->rpcName => codegen
//            std::cout<<"Got reply CL_IOC_RMD_SYNC_REPLY_PROTO:"<< rpcMsg.DebugString()<<std::endl;
            SAFplus::Rpc::rpcTest::rpcTestImpl *service = new SAFplus::Rpc::rpcTest::rpcTestImpl();

            const MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsg.name());
            Message* request = service->GetRequestPrototype(method).New();
            Message* testMethodRes = service->GetResponsePrototype(method).New();
            testMethodRes->ParseFromString(rpcMsg.buffer());
            service->CallMethod(method, NULL, request, testMethodRes, NULL);
            break;
          }

          // Handle response async
          case RequestType::CL_IOC_RMD_ASYNC_REPLY_PROTO:
          {
            //TODO: Handle response data
            // => Calling responeHandler->rpcName => codegen
//            std::cout<<"Got reply CL_IOC_RMD_ASYNC_REPLY_PROTO:"<< rpcMsg.DebugString()<<std::endl;
            SAFplus::Rpc::rpcTest::rpcTestImpl *service = new SAFplus::Rpc::rpcTest::rpcTestImpl();

            const MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsg.name());
            Message* request = service->GetRequestPrototype(method).New();
            Message* testMethodRes = service->GetResponsePrototype(method).New();
            testMethodRes->ParseFromString(rpcMsg.buffer());
            service->CallMethod(method, NULL, request, testMethodRes, NULL);
            break;
          }
        }
      }
    } /* namespace rpcTest */
  } /* namespace Rpc */
} /* namespace SAFplus */
