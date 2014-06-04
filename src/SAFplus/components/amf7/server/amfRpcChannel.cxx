#include <string>
#include <clLogApi.hxx>
#include <clIocPortList.hxx>
#include "amfRpcChannel.hxx"
#include "amfRpc/amfRpc.pb.h"

using namespace std;
using namespace SAFplus;
using namespace google::protobuf;
namespace SAFplusI
  {

  AmfRpcChannel::AmfRpcChannel(SAFplus::MsgServer *svr) : svr(svr), msgId(0)
    {
    
    svr->RegisterHandler(SAFplusI::AMF_REQ_HANDLER_TYPE, this, NULL);
    }

  AmfRpcChannel::~AmfRpcChannel()
    {
    svr->RemoveHandler(SAFplusI::AMF_REQ_HANDLER_TYPE);
    }


  void AmfRpcChannel::CallMethod(const MethodDescriptor* method, RpcController* controller, const Message* request, google::protobuf::Message* response, Closure* done)
    {
    logInfo("AMF","RPC", "CallMethod");
    #if 0
    SAFplus::Rpc::amfRpc rpcMsg;
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
    #endif
    }


  void AmfRpcChannel::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
        string recMsg((const char*) msg, msglen);
#if 0
        SAFplus::Rpc::rpcTest::TestGetRpcMethodRequest req;

        req.ParseFromString(recMsg);

        cout << "==> Handle for message: "<<endl<< req.DebugString() <<" from [" << std::hex << "0x" << from.iocPhyAddress.nodeAddress << ":"
                << std::hex << "0x" << from.iocPhyAddress.portId << "]" << endl;

        SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse res;

        /* Initialize data response */
        SAFplus::Rpc::rpcTest::DataResult *data = res.mutable_dataresult();
        data->set_name("testRpc_response");
        data->set_status(1);

        /**
         * TODO:
         * Reply, need to check message type to reply
         * Maybe sync queue, Async callback etc
         */
        try
        {
            svr->SendMsg(from, (void *)res.SerializeAsString().c_str(), res.ByteSize(), CL_IOC_SAF_MSG_REPLY_PROTO);
        }
        catch (...)
        {
        }
#endif
    }

  };
