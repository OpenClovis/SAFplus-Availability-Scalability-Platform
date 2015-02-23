//#include <clCommon.h>
//#include <clCommonErrors.h>
//#include <clIocApi.h>
//#include <clIocUserApi.h>
//#include <clCpmApi.h>

#include <clCommon.hxx>
#include "clLogApi.hxx"
#include "clMsgHandler.hxx"
#include "clMsgServer.hxx"

namespace SAFplus
{
  class MsgTracker:public Poolable
  {
  public:
    ClIocRecvParam header;
    MsgBuffer msg;
    MsgServer* q;
    virtual void wake(int amt, void* cookie);
  };


  MsgTracker* CreateMsgTracker(MsgBuffer& recv_msg,ClIocRecvParamT& recvParam,MsgServer* q);
  void ReleaseMsgTracker(MsgTracker* rm);
  void MsgTrackerHandler(MsgTracker* rm);

  MsgServer::MsgServer():jq(1,10)
  {
    Wipe();
  }

  MsgServer::MsgServer(uint_t port, uint_t maxPending, uint_t maxThreads, Options flags):jq(1,maxThreads+1)
  {
    /* Created 1 extra thread since it will be used to listen to the IOC port */

    Wipe();
    Init(port, maxPending, maxThreads,flags);
  }

  void MsgServer::Wipe()
  {
    makePrimaryThunk.msgSvr = this;
    reliability = CL_IOC_RELIABLE_MESSAGING; // CL_IOC_UNRELIABLE_MESSAGING
    for (uint_t i=0;i<NUM_MSG_TYPES;i++)
    {
        handlers[i] = NULL;
        cookies[i] = 0;
    }
    sendFailureTimeoutMs = 100;
  }

  void MsgServer::Init(uint_t _port, uint_t maxPending, uint_t maxThreads, Options flags)
  {
    SaNameT myName;

    Wipe();

    port   = _port;

    // TODO: Create the message port

    logInfo("MSG", "SVR","Created message port [%d] for MsgServer object",(unsigned int) port);

    handle = Handle(TransientHandle,Handle::uniqueId(),port,SAFplus::ASP_NODEADDR);

    if (flags & AUTO_ACTIVATE) AutoActivate();
  }

  void MsgServer::AutoActivate()
  {
    // GAS TO DO
  }



void MakePrimary::wake(int amt, void* cookie)
  {
  //
  }


void MsgServer::MakeMePrimary()
  {
    // TODO register primary alias

    //jq.run(&makePrimaryThunk);
  }

  
  void MsgServer::Shutdown()
  {
    // TODO close 

  }

  void MsgServer::SendMsg(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    // TODO
  }

  void ReceiverFunc(MsgServer* q)
  {
    ClIocRecvParamT recvParam;

    logDebug("MSG", "RCV","Message queue receiver started");

    while (q->receiving)
      {
#if 0
        ClBufferHandleT         recv_msg = 0;

        if (worked) 
          {
          logInfo("IOC", "MSG","Rcvd Msg");
          MsgTracker* rm = CreateMsgTracker(recv_msg,recvParam,q);
          recv_msg = 0;  // wipe it so I know to create another
          q->jq.run(rm);
          //clJobQueuePush(&q->jq, (ClCallbackT) MsgTrackerHandler, rm);
          }
#else
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));  // Just sleep until this function is implemented
#endif
      }
  }

  MsgTracker* CreateMsgTracker(MsgBuffer& recv_msg,ClIocRecvParamT& recvParam,MsgServer* q)
  {
    MsgTracker* ret = new MsgTracker();
    ret->header = recvParam;
    ret->q      = q;
    ret->msg    = recv_msg;
    return ret;
  }

  void ReleaseMsgTracker(MsgTracker* rm)
  {
    // TODO delete rm->msg
    //clBufferDelete(&rm->msg);
    rm->q = 0;
    delete rm;
  }
  
  void MsgTracker::wake(int amt, void* cookie)
  {
    MsgHandler *msgHandler = q->handlers[header.protoType];
    if (msgHandler != NULL)
      {
        ClRcT rc;
        uint8_t* buf= NULL; //(uint8_t) msg;
        assert(0);  // not implemented
        //rc = clBufferFlatten(msg,&buf);
        //CL_ASSERT(rc == CL_OK); // Can hit an error if out of memory
        msgHandler->msgHandler(header.srcAddr, q, buf, header.length,q->cookies[header.protoType]);
        SAFplusHeapFree(buf);
      }

    ReleaseMsgTracker(this);
  }
 
  
  void MsgServer::Start(void)
  {
    ClRcT rc;
    logInfo("MSG", "SVR","Starting Message Queue");
    receiving = true;
    receiverThread = boost::thread(ReceiverFunc, this);
    //rc = clJobQueuePush(&jq,(ClCallbackT)ReceiverFunc, (ClPtrT) this);
    //CL_ASSERT(rc == CL_OK);
  }

  void MsgServer::RegisterHandler(uint_t msgtype, MsgHandler *handler, void* cookie)
  {
    if ((msgtype >= NUM_MSG_TYPES)||(msgtype <0))
      {
        logDebug("MSG", "SVR", "message type [%d] out of range [0-255]", (int )msgtype);
        throw Error(Error::SAFPLUS_ERROR, CL_ERR_INVALID_PARAMETER,"message type out of range");
      }
    handlers[msgtype] = handler;
    cookies[msgtype] = cookie;
  }

  void MsgServer::RemoveHandler(uint_t msgtype)
  {
    if ((msgtype >= NUM_MSG_TYPES)||(msgtype <0))
      {
        logDebug("MSG", "SVR", "message type [%d] out of range [0-255]",(int)msgtype);
        throw Error(Error::SAFPLUS_ERROR, CL_ERR_INVALID_PARAMETER,"message type out of range");
      }
    handlers[msgtype] = NULL;
  }


  void MsgServer::Stop(void)
  {
    receiving=false;
    jq.stop();
    receiverThread.detach();
  }

  void MsgServer::Quiesce(void)
  {
    receiving=false;
    jq.stop();
    receiverThread.join();
  }

};
