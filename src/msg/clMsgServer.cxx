#include <clCommon.hxx>
#include <clLogApi.hxx>
#include <clMsgHandler.hxx>
#include <clMsgApi.hxx>
#include <clMsgSarSocket.hxx>

namespace SAFplus
{
  class MsgTracker:public Poolable
  {
  public:
    Message*   msg;
    MsgServer* q;
    virtual void wake(int amt, void* cookie);
    virtual void complete(void);  // derived class free function
    virtual ~MsgTracker();
  };


  MsgTracker* CreateMsgTracker(Message* recv_msg,MsgServer* q);
  void ReleaseMsgTracker(MsgTracker* rm);
  void MsgTrackerHandler(MsgTracker* rm);

  MsgTracker::~MsgTracker()
  {
    msg = NULL;
    q = NULL;
    structId = 0xdeadbee1;
  }

  void MsgTracker::complete()
  {
    assert(deleteWhenComplete == false);
    ReleaseMsgTracker(this);
  }

  MsgServer::MsgServer():jq()
  {
    Wipe();
  }

  MsgServer::MsgServer(uint_t port, uint_t maxPending, uint_t maxThreads, Options flags, SocketType type):jq()
  {
    /* Created 1 extra thread since it will be used to listen to the IOC port */

    Wipe();
    Init(port, maxPending, maxThreads,flags,type);
  }

  MsgServer::~MsgServer()
  {
    Shutdown();
    if (sock) delete sock;
    sock = NULL;
    transport = NULL;
  }

  void MsgServer::Wipe()
  {
    transport = NULL;
    sock = NULL;
    makePrimaryThunk.msgSvr = this;
    reliability = 0;  // TODO: new messaging CL_IOC_RELIABLE_MESSAGING; // CL_IOC_UNRELIABLE_MESSAGING
    for (uint_t i=0;i<NUM_MSG_TYPES;i++)
    {
        handlers[i] = NULL;
        cookies[i] = 0;
    }
    sendFailureTimeoutMs = 100;
  }

  void MsgServer::Init(uint_t _port, uint_t maxPending, uint_t maxThreads, Options flags, SocketType type)
  {
    SaNameT myName;
    assert(_port != 0);  // TODO if port==0, use a shared memory bitmap to allocate a port number
    Wipe();
    jq.init(1,maxThreads);
    port   = _port;

    // Create the message port
    if (!transport) transport = SAFplusI::defaultMsgPlugin;
    switch(type)
    {
      case SOCK_DEFAULT:
      {
        sock = transport->createSocket(port);
        break;
      }
      case SOCK_SHAPING:
      {
        break;
      }
      case SOCK_SEGMENTATION:
      {
        MsgSocket* xport = transport->createSocket(port);
        sock = new MsgSarSocket(xport);
        //sock= new MsgSocketSegmentation(port,transport);
        break;
      }
      case SOCK_RELIABLE:
      {
        break;
      }
    }

    logInfo("MSG", "SVR","Created message port [%d] for MsgServer object",(unsigned int) port);

    handle = Handle(TransientHandle,Handle::uniqueId(),port,sock->node);

    if (flags & AUTO_ACTIVATE) AutoActivate();

    fault = NULL;
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
    receiving = false;
    jq.stop();
  }

  void  MsgServer::SendMsg(Message* msg,uint_t msgtype)
  {
    Message* cur;
    for (cur = msg; cur != nullptr; cur=cur->nextMsg)
      {
        MsgFragment* pfx = msg->prepend(1);
        * ((unsigned char*)pfx->data()) = msgtype;
        pfx->len = 1; 
      }

    sock->send(msg);
  }

  void MsgServer::SendMsg(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    // Note msgtype is ignored at this level.  It is used at the SAFplusMsgServer level.
    assert(sock);
    Message* m = transport->msgPool->allocMsg();
    assert(m);
    m->setAddress(destination);
    MsgFragment* pfx  = m->append(1);
    * ((unsigned char*)pfx->data()) = msgtype;
    pfx->len = 1;
    MsgFragment* frag = m->append(0);
    frag->set(buffer,length);
    sock->send(m);
    // I have to flush to make sure that my use of buffer is complete
    sock->flush();
  }

  void ReceiverFunc(MsgServer* q)
  {
    logDebug("MSG", "RCV","Message queue receiver started on [%" PRIx64 ":%" PRIx64 "] port [%d]", q->handle.id[0], q->handle.id[1],q->port);

    while (q->receiving)
      {
        Message* m = NULL;
        try
          {
          m = q->sock->receive(1,1000);  // block for one second or until a message is received
          }
        catch(Error &e)
          {
            if ((e.osError == EBADF)&&(q->receiving == false))  // we closed the FD because all done
              {
                break;
              }
            else throw;
          }

        if (m) 
          {
            // logDebug("IOC", "MSG","Rcvd Msg");
          MsgTracker* rm = CreateMsgTracker(m,q);
          m = 0;  // wipe it so I know to create another
          q->jq.run(rm);

          }
        //boost::this_thread::sleep(boost::posix_time::milliseconds(1000));  // Just sleep until this function is implemented

      }
  }

  int outstandingMsgTrackers=0;

  MsgTracker* CreateMsgTracker(Message* recv_msg,MsgServer* q)
  {
    MsgTracker* ret = new MsgTracker();
    ret->q      = q;
    ret->msg    = recv_msg;
    assert(ret->deleteWhenComplete == false);
    outstandingMsgTrackers++;
    return ret;
  }

  void ReleaseMsgTracker(MsgTracker* rm)
  {
    if (rm->msg)
      {
        assert(0);
        //rm->msg->msgPool->free(rm->msg);
      }
    rm->q = 0;
    delete rm;
    outstandingMsgTrackers--;

  }
  
  void MsgTracker::wake(int amt, void* cookie)
  {
    assert(msg->firstFragment == msg->lastFragment);  // This code is only written to handle one fragment.
    MsgFragment* frag =  msg->firstFragment;
    // TODO: what would the performance be to put the header into its own buffer during the recvmmsg?
    int msgType = *((char*)frag->read(0));
    frag->start++;
    frag->len--;

    MsgHandler *msgHandler = q->handlers[msgType];
    if (msgHandler != NULL)
      {
        // logDebug("MSG", "SVR", "Received message of type [%d]", (int )msgType);

      msgHandler->msgHandler(q, msg,q->cookies[msgType]);
      msg=nullptr;  // ownership is given to the msgHandler
#if 0
        ClRcT rc;
        //rc = clBufferFlatten(msg,&buf);
        Handle srcAddr = getProcessHandle(msg->port,msg->node);
        msgHandler->msgHandler(srcAddr, q, (char*)frag->read(0), frag->len,q->cookies[msgType]);
        msg->msgPool->free(msg);  // ownership will be given to the msgHandler for now we free
        
#endif
      }
    else
      {
      logWarning("MSG", "SVR", "Received message of unregistered type [%d]", (int )msgType);
      msg->msgPool->free(msg);
      msg=nullptr;
      }
    // this message tracker must be released by the Poolable class after we return
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
