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

#include "clRpcChannel.hxx"
#include "clMsgServer.hxx"

namespace SAFplus
  {
    namespace Rpc
      {

        //Server
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, google::protobuf::Service *service) :
            svr(svr), service(service)
          {
            dest = NULL;
            msgId = 0;
            svr->RegisterHandler(CL_IOC_RMD_SYNC_REQUEST_PROTO, this, NULL);
            svr->RegisterHandler(CL_IOC_RMD_ASYNC_REQUEST_PROTO, this, NULL);
          }

        //Client
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, ClIocAddressT *iocDest) :
            svr(svr), dest(iocDest)
          {
            msgId = 0;
            service = NULL;
            svr->RegisterHandler(CL_IOC_RMD_SYNC_REPLY_PROTO, this, NULL);
            svr->RegisterHandler(CL_IOC_RMD_ASYNC_REPLY_PROTO, this, NULL);
          }

        RpcChannel::~RpcChannel()
          {
            // TODO Auto-generated destructor stub
          }

        void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController* controller,
            const google::protobuf::Message* request, google::protobuf::Message* response, google::protobuf::Closure* done)
          {
            SAFplus::Rpc::RpcMessage rpcMsg;
            int64_t idx = msgId++;

            //Lock sending and record a RPC
//            ScopedLock<Mutex> lock(mutex);

            rpcMsg.set_type(RequestType::CL_IOC_RMD_ASYNC_REQUEST_PROTO);
            rpcMsg.set_id(idx);
            rpcMsg.set_name(method->name());
            rpcMsg.set_buffer(request->SerializeAsString());

            try
              {
                svr->SendMsg(*dest, (void *) rpcMsg.SerializeAsString().c_str(), rpcMsg.ByteSize(), CL_IOC_RMD_ASYNC_REQUEST_PROTO);
              }
            catch (...)
              {
              }

            MsgRpcEntry *rpcReqEntry = new MsgRpcEntry();
            rpcReqEntry->msgId = idx;
            rpcReqEntry->response = response;
            rpcReqEntry->callback = done;
            msgRPCs[idx] = rpcReqEntry;
          }

        void RpcChannel::RequestComplete(MsgRpcEntry *rpcRequestEntry)
          {
            RpcMessage rpcMsg;

            std::cout<<"DONE3"<<std::endl;
            //Lock handle request complete
//            ScopedLock<Mutex> lock(mutex);

            rpcMsg.set_type(RequestType::CL_IOC_RMD_ASYNC_REPLY_PROTO);
            rpcMsg.set_id(rpcRequestEntry->msgId);
            rpcMsg.set_buffer(rpcRequestEntry->response->SerializePartialAsString());

            //Sending reply
            try
              {
                svr->SendMsg(rpcRequestEntry->srcAddr, (void *) rpcMsg.SerializeAsString().c_str(), rpcMsg.ByteSize(),
                    CL_IOC_RMD_ASYNC_REPLY_PROTO);
              }
            catch (...)
              {
              }

            //Remove a RPC request entry
            msgRPCs.erase(rpcRequestEntry->msgId);
            delete rpcRequestEntry->response;
            delete rpcRequestEntry;

            std::cout<<"DONE4"<<std::endl;
          }

        void RpcChannel::HandleRequest(SAFplus::Rpc::RpcMessage *msg, ClIocAddressT *iocReq)
          {
            const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(msg->name());
            google::protobuf::Message* request_pb = service->GetRequestPrototype(method).New();
            google::protobuf::Message* response_pb = service->GetResponsePrototype(method).New();
            request_pb->ParseFromString(msg->buffer());

            //Lock handle request
//            ScopedLock<Mutex> lock(mutex);

            MsgRpcEntry *rpcReqEntry = new MsgRpcEntry();
            rpcReqEntry->msgId = msg->id();
            rpcReqEntry->response = response_pb;
            rpcReqEntry->srcAddr = *iocReq;
            msgRPCs[msg->id()] = rpcReqEntry;

            google::protobuf::Closure *callback = google::protobuf::NewCallback(this, &RpcChannel::RequestComplete, rpcReqEntry);
            service->CallMethod(method, NULL, request_pb, response_pb, callback);
          }

        void RpcChannel::HandleResponse(SAFplus::Rpc::RpcMessage *msg)
          {
            std::cout<<"DONE5"<<std::endl;
            //Lock handle request
//            ScopedLock<Mutex> lock(mutex);

            std::map<uint64_t,MsgRpcEntry*>::iterator it = msgRPCs.find(msg->id());
            if (it != msgRPCs.end())
              {
                std::cout<<"DONE2"<<std::endl;
                MsgRpcEntry* rpcReqEntry = (MsgRpcEntry*) it->second;
                rpcReqEntry->response->ParseFromString(msg->buffer());
                rpcReqEntry->callback->Run();

//                msgRPCs.erase(rpcReqEntry->msgId);
                //delete rpcReqEntry->response;
//                delete rpcReqEntry;
              }

          }

        void RpcChannel::msgHandler(ClIocAddressT srcAddr, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
          {
            SAFplus::Rpc::RpcMessage rpcMsg;

            std::string recMsg((const char*) msg, msglen);
            rpcMsg.ParseFromString(recMsg);

            switch (rpcMsg.type())
              {
              // Handler request sync/async
              case RequestType::CL_IOC_RMD_SYNC_REQUEST_PROTO:
              case RequestType::CL_IOC_RMD_ASYNC_REQUEST_PROTO:
                {
                  HandleRequest(&rpcMsg, &srcAddr);
                  break;
                }

                // Handle response sync/async
              case RequestType::CL_IOC_RMD_SYNC_REPLY_PROTO:
              case RequestType::CL_IOC_RMD_ASYNC_REPLY_PROTO:
                {
                  HandleResponse(&rpcMsg);
                  break;
                }
              }
          }

      } /* namespace Rpc */
  } /* namespace SAFplus */
