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
        // TODO: the entities in msgRPCs need to be stored with their destination address.  If that entity fails, all msgRPCs going to it need to be removed and the Wakeable called with an error
        // TODO: new MsgRpcEntry(); should be replaced with an allocator that gets (and returns) objects from a list to minimize calls to "new" and "delete"
        //Server
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, SAFplus::Rpc::RpcService *rpcService) :
            svr(svr), service(rpcService)
          {
            dest.iocPhyAddress.nodeAddress = 0;
            dest.iocPhyAddress.portId = 0;
            msgId = 0;
          }

        //Client
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, ClIocAddressT iocDest) :
            svr(svr), dest(iocDest)
          {
            msgId = 0;
            service = NULL;
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

        /*
         * Packed request message and send to server side
         */
        void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, SAFplus::Handle destination,
            const google::protobuf::Message* request, google::protobuf::Message* response, SAFplus::Wakeable &wakeable)
          {
            string strMsgReq;
            SAFplus::Rpc::RpcMessage rpcMsgReq;
            ClIocAddressT overDest;
            ThreadSem blocker(0);

            //Lock sending and record a RPC
            int64_t idx;
            MsgRpcEntry *rpcReqEntry = new MsgRpcEntry();
            if (1)
              {
              ScopedLock<Mutex> lock(mutex);
              idx = msgId++;
              rpcReqEntry->msgId = idx;
              rpcReqEntry->response = response;
              if (&wakeable == &SAFplus::BLOCK)
                rpcReqEntry->callback = &blocker;
              else
                rpcReqEntry->callback = (Wakeable*)&wakeable;
              msgRPCs.insert(std::pair<uint64_t,MsgRpcEntry*>(idx, rpcReqEntry));
              }

            rpcMsgReq.set_type(msgSendType);
            rpcMsgReq.set_id(idx);
            rpcMsgReq.set_name(method->name());

            request->SerializeToString(&strMsgReq);
            rpcMsgReq.set_buffer(strMsgReq);

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
                string output;
                rpcMsgReq.SerializeToString(&output);
                int size = output.size();
                svr->SendMsg(overDest, (void *) output.c_str(), size, msgSendType);
              }
            catch (Error &e)
              {
                logError("RPC", "REQ", "Serialization Error: %s", e.what());
              }
            

            if (&wakeable == &SAFplus::BLOCK)
              {
              blocker.lock(1);  // TODO: Handle dropped packets: use timed_lock with either a message retry or by raising exception? 
              }
          }

        /*
         * Server side: handle request message and reply to client side
         */
        void RpcChannel::HandleRequest(SAFplus::Rpc::RpcMessage *rpcMsgReq, ClIocAddressT iocReq)
          {
            string strMsgRes;
            RpcMessage rpcMsgRes;

            const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsgReq->name());
            google::protobuf::Message* request_pb = service->GetRequestPrototype(method).New();
            google::protobuf::Message* response_pb = service->GetResponsePrototype(method).New();
            request_pb->ParseFromString(rpcMsgReq->buffer());

            service->CallMethod(method, INVALID_HDL, request_pb, response_pb, *((SAFplus::Wakeable*) nullptr));

            rpcMsgRes.set_type(msgReplyType);
            rpcMsgRes.set_id(rpcMsgReq->id());

            response_pb->SerializeToString(&strMsgRes);
            rpcMsgRes.set_buffer(strMsgRes);

            //Check local service
            if (iocReq.iocPhyAddress.nodeAddress == dest.iocPhyAddress.nodeAddress
                && iocReq.iocPhyAddress.portId == dest.iocPhyAddress.portId)
              {
                HandleResponse(&rpcMsgRes);
              }
            else //remote
              {
                //Sending reply to remote
                try
                  {
                    string output;
                    rpcMsgRes.SerializeToString(&output);
                    int size = output.size();
                    svr->SendMsg(iocReq, (void *) output.c_str(), size, msgReplyType);
                  }
                catch (Error &e)
                  {
                    logError("RPC", "REQ", "%s", e.what());
                  }
              }
            delete request_pb;
            delete response_pb;
          }

        /*
         * Client side: handle response message
         */
        void RpcChannel::HandleResponse(SAFplus::Rpc::RpcMessage *rpcMsgRes)
          {
            std::map<uint64_t,MsgRpcEntry*>::iterator it = msgRPCs.find(rpcMsgRes->id());
            if (it != msgRPCs.end())
              {
                MsgRpcEntry *rpcReqEntry = it->second;
                rpcReqEntry->response->ParseFromString(rpcMsgRes->buffer());

                if (rpcReqEntry->callback != NULL)
                  {
                    rpcReqEntry->callback->wake(1, (void*) rpcReqEntry->response);
                  }
                // Lock
                ScopedLock<Mutex> lock(mutex);
                msgRPCs.erase(rpcMsgRes->id());
                // Unlock is done automatically when lock drops out of scope.
                
              }

          }

        /*
         * Message handle for communication
         */
        void RpcChannel::msgHandler(ClIocAddressT srcAddr, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
          {
            SAFplus::Rpc::RpcMessage rpcMsgReqRes;
            rpcMsgReqRes.ParseFromArray(msg, msglen);

            ClWordT msgType = rpcMsgReqRes.type();

            if (msgType == msgSendType)
              {
                HandleRequest(&rpcMsgReqRes, srcAddr);
              }
            else if (msgType == msgReplyType)
              {
                HandleResponse(&rpcMsgReqRes);
              }
            else
              {
                logError("RPC", "HDL", "Received invalid message type [%lu] from [%d.%d]", msgType, srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId);
              }

          }

      } /* namespace Rpc */
  } /* namespace SAFplus */
