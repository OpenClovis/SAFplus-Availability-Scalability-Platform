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

#include "clThreadApi.hxx"
#include "SAFplusPBExt.pb.h"
#include "SAFplusRpc.pb.h"
#include "clRpcService.hxx"
#include "clRpcChannel.hxx"
#include "clMsgServer.hxx"
#include "clMsgApi.hxx"

using namespace std;

namespace SAFplus
  {
    namespace Rpc
      {

        //Server
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, SAFplus::Rpc::RpcService *rpcService) :
            svr(svr), service(rpcService)
          {
            dest.iocPhyAddress.nodeAddress = 0;
            dest.iocPhyAddress.portId = 0;
            msgId = 0;
//            svr->RegisterHandler(CL_IOC_RMD_SYNC_REQUEST_PROTO, this, NULL);
//            svr->RegisterHandler(CL_IOC_RMD_ASYNC_REQUEST_PROTO, this, NULL);
          }

        //Client
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, ClIocAddressT iocDest) :
            svr(svr), dest(iocDest)
          {
            msgId = 0;
            service = NULL;
//            svr->RegisterHandler(CL_IOC_RMD_SYNC_REPLY_PROTO, this, NULL);
//            svr->RegisterHandler(CL_IOC_RMD_ASYNC_REPLY_PROTO, this, NULL);
          }


      void RpcChannel::setMsgType(ClWordT send,ClWordT reply) 
              { 
              msgSendType = send; msgReplyType = reply; 
              if(msgSendType)  svr->RegisterHandler(msgSendType, this, NULL);
              if(msgReplyType) svr->RegisterHandler(msgReplyType, this, NULL);

              }  // Set the protocol type for underlying transports that require one.

        RpcChannel::~RpcChannel()
          {
            // TODO Auto-generated destructor stub
          }

        void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, SAFplus::Handle destination,
            const google::protobuf::Message* request, google::protobuf::Message* response, SAFplus::Wakeable &wakeable)
          {
            SAFplus::Rpc::RpcMessage rpcMsg;
            ClIocAddressT overDest;

            int64_t idx = msgId++;

            //Lock sending and record a RPC
            ScopedLock<Mutex> lock(mutex);

            rpcMsg.set_type(msgSendType);
            rpcMsg.set_id(idx);
            rpcMsg.set_name(method->name());
            rpcMsg.set_buffer(request->SerializeAsString());

            if (destination != INVALID_HDL)
              {
                overDest = getAddress(destination);
              }
            else
              {
                overDest = dest;
              }

            try
              {
                const char* data = rpcMsg.SerializeAsString().c_str();
                int size = rpcMsg.ByteSize();
                svr->SendMsg(overDest, (void *) data, size, msgSendType);
              }
            catch (...)
              {
              }

            MsgRpcEntry *rpcReqEntry = new MsgRpcEntry();
            rpcReqEntry->msgId = idx;
            rpcReqEntry->response = response;
            rpcReqEntry->callback = (Wakeable*)&wakeable;

            msgRPCs[idx] = rpcReqEntry;
          }

        void RpcChannel::HandleRequest(SAFplus::Rpc::RpcMessage *msg, ClIocAddressT iocReq)
          {
            RpcMessage rpcMsg;

            const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(msg->name());
            google::protobuf::Message* request_pb = service->GetRequestPrototype(method).New();
            google::protobuf::Message* response_pb = service->GetResponsePrototype(method).New();
            request_pb->ParseFromString(msg->buffer());

            //Lock handle request
            ScopedLock<Mutex> lock(mutex);

            service->CallMethod(method, INVALID_HDL, request_pb, response_pb, *((SAFplus::Wakeable*) nullptr));

            rpcMsg.set_type(msgReplyType);
            rpcMsg.set_id(msg->id());
            rpcMsg.set_buffer(response_pb->SerializeAsString());

            //TODO: Check remote to send or field member???

            //Sending reply
            try
              {
                const char* data = rpcMsg.SerializeAsString().c_str();
                int size = rpcMsg.ByteSize();
                svr->SendMsg(iocReq, (void *) data, size, msgReplyType);
              }
            catch (...)
              {
              }

            //TODO:
            //call wakeable->wake(1, (void *)response_pb);

          }

        void RpcChannel::HandleResponse(SAFplus::Rpc::RpcMessage *msg)
          {
            //Lock handle request
            ScopedLock<Mutex> lock(mutex);

            std::map<uint64_t,MsgRpcEntry*>::iterator it = msgRPCs.find(msg->id());
            if (it != msgRPCs.end())
              {
                MsgRpcEntry* rpcReqEntry = (MsgRpcEntry*) it->second;
                rpcReqEntry->response->ParseFromString(msg->buffer());
                if (rpcReqEntry->callback != NULL)
                  {
                    rpcReqEntry->callback->wake(1, (void*) rpcReqEntry->response);
                  }
                msgRPCs.erase(rpcReqEntry->msgId);
                if (msgId > 0)
                  msgId--;
              }

          }

        void RpcChannel::msgHandler(ClIocAddressT srcAddr, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
          {
            SAFplus::Rpc::RpcMessage rpcMsg;

            std::string recMsg((const char*) msg, msglen);
            rpcMsg.ParseFromString(recMsg);

            ClWordT msgType = rpcMsg.type();

            if (msgType == msgSendType)
              {
                HandleRequest(&rpcMsg, srcAddr);
              }
            else if (msgType == msgReplyType)
              {
                HandleResponse(&rpcMsg);
              }
            else
              {
                logError("RPC", "HDL", "Received invalid message type [%u] from [%d.%d]", msgType, srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId);
              }

          }

      } /* namespace Rpc */
  } /* namespace SAFplus */
