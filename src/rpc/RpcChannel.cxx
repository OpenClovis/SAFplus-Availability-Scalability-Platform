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
#include <inttypes.h>
#include <clFaultApi.hxx>

#include <boost/iostreams/stream.hpp>

using namespace std;

namespace SAFplus
  {
    namespace Rpc
      {
        const std::string RpcChannel::NO_RESPONSE = "NO_RESPONSE";

        // TODO: the entities in msgRPCs need to be stored with their destination address.  If that entity fails, all msgRPCs going to it need to be removed and the Wakeable called with an error
        // TODO: new MsgRpcEntry(); should be replaced with an allocator that gets (and returns) objects from a list to minimize calls to "new" and "delete"
        //Server
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, SAFplus::Rpc::RpcService *rpcService) :
            svr(svr), service(rpcService)
          {
            // Assign to compare with local service
            dest = svr->handle;
            msgId = 0;
          }

        //Client
        RpcChannel::RpcChannel(SAFplus::MsgServer *svr, SAFplus::Handle iocDest) :
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
            SAFplus::Handle overDest;
            ThreadSem blocker(0);
            bool isRequestWithNoResponse = false;
            int64_t idx;

            /*
             * Request with response
             */
            if (!(method->output_type()->name().compare(NO_RESPONSE)))
              {
                isRequestWithNoResponse = true;
              }
            else
              {
                //Lock sending and record a RPC
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
                      rpcReqEntry->callback = (Wakeable*) &wakeable;
                    msgRPCs.insert(std::pair<uint64_t,MsgRpcEntry*>(idx, rpcReqEntry));
                  }
              }

            rpcMsgReq.set_type(msgSendType);
            rpcMsgReq.set_id(idx);
            rpcMsgReq.set_name(method->name());

            // Attach handle into rpc msg (this would make independent from transport layer plugins)
            SAFplus::Rpc::Handle *hdl = rpcMsgReq.mutable_handle();
            hdl->set_id0(svr->handle.id[0]);
            hdl->set_id1(svr->handle.id[1]);

            request->SerializeToString(&strMsgReq);
            rpcMsgReq.set_buffer(strMsgReq);

            if (destination != INVALID_HDL)
              {
                overDest = destination;
              }
            else
              {
                overDest = dest;
                destination = dest;
              }

            bool retry=true;
            while (retry)
              {
                try
                  {
                    //string output;
                    //rpcMsgReq.SerializeToString(&output);
                    //int size = output.size();
                    //svr->SendMsg(overDest, (void *) output.c_str(), size, msgSendType);
                    MsgPool& pool = svr->getMsgPool();
                    Message* msg = pool.allocMsg();
                    msg->setAddress(overDest);
                    if (1)
                      {
                        boost::iostreams::stream<MessageOStream>  mos(msg);
                        rpcMsgReq.SerializeToOstream(&mos);
                      }
                    svr->SendMsg(msg, msgSendType);
                  }
                catch (Error &e)
                  {
                    logError("RPC", "REQ", "Serialization Error: %s", e.what());
                  }

                /*
                 * Do not handle block on receiving response
                 */
                if (isRequestWithNoResponse)
                  return;

                if (&wakeable == &SAFplus::BLOCK)
                  {
                    if (!blocker.timed_lock(SAFplusI::RpcRetryInterval,1))
                      {
                        logWarning("RPC", "REQ", "RPC reply timeout.  Waited [%d] ms",SAFplusI::RpcRetryInterval);
                        SAFplus::Handle h = destination;
                        
                        if (!svr->fault) // A fault manager wasn't installed in this message server so I can't do discovery of node/process failures
                          {
                            assert(0);  // TODO: raise exception
                          }
                    
                        for (int i=0;i<3;i++)  // Loop around 3 times checking this handle, then node/process, and finally just the node
                          {
                            FaultState fs = svr->fault->getFaultState(h);
                            if (fs==FaultState::STATE_UP)
                              {
                                svr->fault->notifyNoResponse(h);  // Tell the fault manager that I'm having a problem.
                                retry=true;
                                break;
                              }
                            else if (fs==FaultState::STATE_UNDEFINED)
                              {
                                // Look at fault state of containing entity?
                                if (i==0) h = getProcessHandle(destination.getProcess(), destination.getNode());                      
                                if (i==1) h = getNodeHandle(destination.getNode());
                                if (i==2)  // No information is stored in the fault manager... should never happen, unless old data in shared mem.
                                  {
                                    //            throw Error(Error::SAFPLUS_ERROR, Error::MISCONFIGURATION, "Invalid messaging port [0]", __FILE__, __LINE__);
                                    logCritical("MSG","SVR", "Fault manager has no information about this destination [%" PRIx64 ":%" PRIx64 "]", destination.id[0],destination.id[1]);
                                    throw Error(Error::SAFPLUS_ERROR,Error::CONTAINER_DOES_NOT_EXIST, "Fault manager has no information about destination",__FILE__,__LINE__);
                                 }
                              }
                            else if (fs==FaultState::STATE_DOWN)
                              {
                                   throw Error(Error::SAFPLUS_ERROR,Error::DOES_NOT_EXIST, "Fault manager has no information about destination",__FILE__,__LINE__);                          
                              }
                            else 
                              {
                                assert(!"Impossible fault state");
                              }
                          }
                           retry = true;

                      }
                    else retry = false;
                  }
                else retry = false; // If the function is not blocking, then the app will handle any needed retries
              }
          }

        /*
         * Server side: handle request message and reply to client side
         */
        void RpcChannel::HandleRequest(SAFplus::Rpc::RpcMessage *rpcMsgReq, SAFplus::Handle iocReq)
          {
            string strMsgRes;
            RpcMessage rpcMsgRes;
            google::protobuf::Message* request_pb = NULL;
            google::protobuf::Message* response_pb = NULL;
            bool isRequestWithNoResponse = false;
            if (!service)
              {
                clDbgCodeError(0, "Entity [%" PRIx64 ":%" PRIx64 "] from node.port [%d.%d] made a RPC call to a client-only entity", iocReq.id[0], iocReq.id[1], iocReq.getNode(), iocReq.getPort());
                if (request_pb) delete request_pb;
              return;
              }
            const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(rpcMsgReq->name());
            request_pb  = service->GetRequestPrototype(method).New();

            /*
             * Request with response
             */
            if (!(method->output_type()->name().compare(NO_RESPONSE)))
              {
                isRequestWithNoResponse = true;
              }
            else
              {
                response_pb = service->GetResponsePrototype(method).New();
              }

            request_pb->ParseFromString(rpcMsgReq->buffer());

            service->CallMethod(method, INVALID_HDL, request_pb, response_pb, *((SAFplus::Wakeable*) nullptr));

            if (isRequestWithNoResponse)
              {
                delete request_pb;
                return;
              }

            /*
             * Pack response msg
             */
            rpcMsgRes.set_type(msgReplyType);
            rpcMsgRes.set_id(rpcMsgReq->id());

            // Attach handle into rpc msg (this would make independent from transport layer plugins)
            SAFplus::Rpc::Handle *hdl = rpcMsgRes.mutable_handle();
            hdl->set_id0(svr->handle.id[0]);
            hdl->set_id1(svr->handle.id[1]);

            response_pb->SerializeToString(&strMsgRes);
            rpcMsgRes.set_buffer(strMsgRes);

            //Check local service
            if (iocReq.getNode() == dest.getNode()
                && iocReq.getPort() == dest.getPort())
              {
                HandleResponse(&rpcMsgRes);
              }
            else //remote
              {
                //Sending reply to remote
                try
                  {
                    //string output;
                    //rpcMsgRes.SerializeToString(&output);
                    //int size = output.size();
                    //svr->SendMsg(iocReq, (void *) output.c_str(), size, msgReplyType);
                    MsgPool& pool = svr->getMsgPool();
                    Message* msg = pool.allocMsg();
                    msg->setAddress(iocReq);
                    if (1)
                      {
                        boost::iostreams::stream<MessageOStream>  mos(msg);
                        rpcMsgRes.SerializeToOstream(&mos);
                      }
                    svr->SendMsg(msg, msgReplyType);
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
            mutex.lock();
            std::map<uint64_t,MsgRpcEntry*>::iterator it = msgRPCs.find(rpcMsgRes->id());
            if (it != msgRPCs.end())
              {
                MsgRpcEntry* rpcReqEntry = it->second;
                msgRPCs.erase(it);
                mutex.unlock();

                rpcReqEntry->response->ParseFromString(rpcMsgRes->buffer());

                if (rpcReqEntry->callback != NULL)
                  {
                    rpcReqEntry->callback->wake(1, (void*) rpcReqEntry->response);
                  }
                else
                  {
                    logWarning("RPC", "RSP","RPC has no callback");
                  }
                delete rpcReqEntry;
              }
            else
              {
                mutex.unlock();
                logWarning("RPC", "RSP","RPC response [%d] has no request tracker", (unsigned int) rpcMsgRes->id());
              }

          }

        /*
         * Message handle for communication
         */
        void RpcChannel::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
        {
          Message* msg = msgHead;
          while(msg)
            {
              assert(msg->firstFragment == msg->lastFragment);  // TODO: This code is only written to handle one fragment.
              MsgFragment* frag = msg->firstFragment;
              SAFplus::Rpc::RpcMessage rpcMsgReqRes;
              rpcMsgReqRes.ParseFromArray(frag->read(0),frag->len);

              ClWordT msgType = rpcMsgReqRes.type();

              // transport layer handle only provides process and node.  RPC response handle needs to be the requesting object
              SAFplus::Handle from;
              from.id[0] = rpcMsgReqRes.handle().id0();
              from.id[1] = rpcMsgReqRes.handle().id1();

              if (msgType == msgSendType)
                {
                  HandleRequest(&rpcMsgReqRes, from);
                }
              else if (msgType == msgReplyType)
                {
                  HandleResponse(&rpcMsgReqRes);
                }
              else
                {
                  SAFplus::Handle srcAddr = msg->getAddress();
                  logError("RPC", "HDL", "Received invalid message type [%lu] from [%" PRIx64 ":%" PRIx64 "]", msgType, srcAddr.id[0], srcAddr.id[1]);
                }
              msg = msg->nextMsg;
            }
          msgHead->msgPool->free(msgHead);
        }
#if 0
        void RpcChannel::msgHandler(SAFplus::Handle srcAddr, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
          {
            SAFplus::Rpc::RpcMessage rpcMsgReqRes;
            rpcMsgReqRes.ParseFromArray(msg, msglen);

            ClWordT msgType = rpcMsgReqRes.type();

            // Using this handle to independent with transport layer plugins
            SAFplus::Handle from;
            from.id[0] = rpcMsgReqRes.handle().id0();
            from.id[1] = rpcMsgReqRes.handle().id1();

            if (msgType == msgSendType)
              {
                HandleRequest(&rpcMsgReqRes, from);
              }
            else if (msgType == msgReplyType)
              {
                HandleResponse(&rpcMsgReqRes);
              }
            else
              {
                logError("RPC", "HDL", "Received invalid message type [%lu] from [%" PRIx64 ":%" PRIx64 "]", msgType, srcAddr.id[0], srcAddr.id[1]);
              }

          }
#endif

      } /* namespace Rpc */
  } /* namespace SAFplus */
