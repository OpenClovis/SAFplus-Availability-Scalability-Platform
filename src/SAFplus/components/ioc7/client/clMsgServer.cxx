#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clIocUserApi.h>
#include <clCpmApi.h>

#include "clLogApi.hxx"
#include "clMsgHandler.hxx"
#include "clMsgServer.hxx"
#include "clCommon.hxx"

namespace SAFplus
{
  class MsgTracker
  {
  public:
    ClIocRecvParam header;
    ClBufferHandleT msg;
    MsgServer* q;
  };


  MsgTracker* CreateMsgTracker(ClBufferHandleT recv_msg,ClIocRecvParamT& recvParam,MsgServer* q);
  void ReleaseMsgTracker(MsgTracker* rm);
  void MsgTrackerHandler(MsgTracker* rm);

  
  MsgServer::MsgServer()
  {
    Wipe();
  }

  MsgServer::MsgServer(ClWordT port, ClWordT maxPending, ClWordT maxThreads, Options flags)
  {
    Wipe();
    Init(port, maxPending, maxThreads,flags);
  }

  
  void MsgServer::Wipe()
  {
    reliability = CL_IOC_RELIABLE_MESSAGING; // CL_IOC_UNRELIABLE_MESSAGING
    for (ClWordT i=0;i<NUM_MSG_TYPES;i++)
    {
        handlers[i] = NULL;
        cookies[i] = 0;
    }
    sendFailureTimeoutMs = 100;
  }

  void MsgServer::Init(ClWordT _port, ClWordT maxPending, ClWordT maxThreads, Options flags)
  {
    SaNameT myName;
    ClRcT rc;

    Wipe();

    /* Get the component ID */
    /*
    rc = clCpmComponentNameGet(clCpmHandle,&myName);
    CL_ASSERT(rc==CL_OK);
    rc = clCpmComponentIdGet(clCpmHandle,&myName,&compId);
    CL_ASSERT(rc==CL_OK);
     */

    port   = _port;
        
    rc = clIocCommPortCreate( port, reliability, &commPort);
    if (rc != CL_OK)
      {
        clDbgPause();
        throw Error(Error::SAFPLUS_ERROR,rc,"Cannot create communications port");
      }
    
    // TODO: when SAFplus6 and 7 are separated, remove this code the peeks inside what is supposed to be an opaque handle
    ClIocCommPortT* pdata = (ClIocCommPortT*) commPort;
    port = pdata->portId;

    logInfo("IOC", "MSG","Created message port [%d] for MsgServer object",port);

    int node = clIocLocalAddressGet();
    uniqueAddr.iocPhyAddress.nodeAddress   = node;
    uniqueAddr.iocPhyAddress.portId        = port;

    handle = Handle(TransientHandle,Handle::uniqueId(),port,node);

    /* Create 1 extra thread since it will be used to listen to the IOC port */
    clJobQueueInit(&jq, maxPending, maxThreads+1);

    if (flags & AUTO_ACTIVATE) AutoActivate();
  }

  void MsgServer::AutoActivate()
  {
    // GAS TO DO
  }

  void SynchronousMakePrimary(MsgServer* q)
  {
    ClRcT rc;
    /* Create a Logical Address CL_IOC_LOGICAL_ADDRESS_FORM*/
    ClIocTLInfoT  tl;
    q->sharedAddr.iocLogicalAddress = tl.logicalAddr                = CL_IOC_ADDRESS_FORM(CL_IOC_LOGICAL_ADDRESS_TYPE, q->port, q->compId);
    tl.compId                     = q->compId;
    tl.repliSemantics             = CL_IOC_TL_NO_REPLICATION;
    tl.contextType                = CL_IOC_TL_GLOBAL_SCOPE;
    tl.haState                    = CL_IOC_TL_ACTIVE;
    tl.physicalAddr               = q->uniqueAddr.iocPhyAddress;
   
    rc = clIocTransparencyRegister(&tl);
    if (rc != CL_OK)
      {
        clDbgPause();
        throw Error(Error::SAFPLUS_ERROR,rc,"Failed to create shared address");
      }
  }
  
  void MsgServer::MakeMePrimary()
  {
    /* Defer the activate -- synchronous causes EO lockup */
    ClRcT rc = clJobQueuePush(&jq,(ClCallbackT)SynchronousMakePrimary, (ClPtrT) this);
    CL_ASSERT(rc == CL_OK);
  }

  
  void MsgServer::Shutdown()
  {
    ClRcT rc;
    
    receiving = false;
    rc =	clIocCommPortDelete (commPort);
    CL_ASSERT(rc == CL_OK);
    commPort = 0;

    /*
    ClRcT 	clIocTransparencyDeregister (CL_IN ClUint32T compId)
 	De-registers the application with Transparency Layer.
    */
  }

  void MsgServer::SendMsg(ClIocAddressT destination, void* buffer, ClWordT length,ClWordT msgtype)
  {
    ClRcT rc;
    ClBufferHandleT         send_msg = 0;
    ClIocSendOptionT        sendOptions = {0};

    sendOptions.msgOption   = CL_IOC_PERSISTENT_MSG;

    rc = clBufferCreate(&send_msg);
    if (rc != CL_OK)
      {
        clDbgPause();
        throw Error(Error::SAFPLUS_ERROR,rc,"Failed to create send buffer");
      }
    clBufferNBytesWrite(send_msg, (ClUint8T*)buffer, length);

    ClWordT retry = 0;
    do
      {
        retry +=1;
        rc = clIocSend(commPort, send_msg, msgtype, &destination, &sendOptions );
        if (rc != CL_OK)
          {
            if ((CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_HOST_UNREACHABLE)&&(CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
              {
                clDbgPause(); /* unreachable is an "expected" error during a board failure */
                throw Error(Error::SAFPLUS_ERROR,rc,"Failed to send");
              }
            else /* Give a moment for the failover to occur and retry the send */
              {
                ClTimerTimeOutT timeOut = {0, (ClUint32T)sendFailureTimeoutMs};
                clOsalTaskDelay (timeOut);
              }
          }
    
      } while((retry < 3)&&(rc!=CL_OK));

    if (send_msg)
    {
        clBufferDelete(&send_msg);
    }

    if (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE)
      {
        /* Well, maybe no one is able to take over for this IOC address, so I've got to pass the
        error up...*/
        throw Error(Error::SAFPLUS_ERROR,rc,"Failed to send");
      }
  }

  void ReceiverFunc(MsgServer* q)
  {
    ClIocRecvParamT recvParam;

    //logInfo("IOC", "MSG","Message queue receiver function");

    while (q->receiving)
      {
        ClRcT                   rc = CL_OK; 
        ClIocRecvOptionT        recvOptions = {0};
        ClBufferHandleT         recv_msg = 0;
        recvOptions.recvTimeout = 5000;  /* Wake up every second to see if we should still be receiving */
        if (!recv_msg) clBufferCreate(&recv_msg);
        //Log(CL_LOG_SEV_INFO,"Calling IocReceive.");
        rc = clIocReceive(q->commPort, &recvOptions, recv_msg, &recvParam);
        if ((CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_TRY_AGAIN)||(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT))
          {
            /* Pass: we just got a timeout and so will retry the recv */
          }
        else if (rc != CL_OK)
          {
            ClTimerTimeOutT timeOut = {1,0};
            // I can't throw here because I'm in my own thread and so there's noone to catch
            logError("IOC", "MSG", "Error [0x%x] receiving message, retrying...",rc);
            clOsalTaskDelay (timeOut); // delay to make sure a continuous error doesn't chew up CPU.
          }
        else //(rc == CL_OK)
          {
          logInfo("IOC", "MSG","Rcvd Msg");
          MsgTracker* rm = CreateMsgTracker(recv_msg,recvParam,q);
          recv_msg = 0;  // wipe it so I know to create another
          clJobQueuePush(&q->jq, (ClCallbackT) MsgTrackerHandler, rm);
          }
      }
  }

  MsgTracker* CreateMsgTracker(ClBufferHandleT recv_msg,ClIocRecvParamT& recvParam,MsgServer* q)
  {
    MsgTracker* ret = new MsgTracker();
    ret->header = recvParam;
    ret->q      = q;
    ret->msg    = recv_msg;
    return ret;
  }

  void ReleaseMsgTracker(MsgTracker* rm)
  {
    clBufferDelete(&rm->msg);
    rm->q = 0;
    delete rm;
  }
  
  void MsgTrackerHandler(MsgTracker* rm)
  {
    MsgServer* q = rm->q;
    MsgHandler *msgHandler = q->handlers[rm->header.protoType];
    if (msgHandler != NULL)
      {
        ClRcT rc;
        ClUint8T* buf=0;
        rc = clBufferFlatten(rm->msg,&buf);
        CL_ASSERT(rc == CL_OK); // Can hit an error if out of memory
        msgHandler->msgHandler(rm->header.srcAddr, q, buf, rm->header.length,q->cookies[rm->header.protoType]);
        clHeapFree(buf);
      }

    ReleaseMsgTracker(rm);
  }
 
  
  void MsgServer::Start(void)
  {
    ClRcT rc;
    logInfo("IOC", "MSG","Starting Message Queue");
    receiving = true;
    rc = clJobQueuePush(&jq,(ClCallbackT)ReceiverFunc, (ClPtrT) this);
    CL_ASSERT(rc == CL_OK);
  }


  void MsgServer::Stop(void)
  {
    receiving=false;
    clJobQueueStop(&jq);
  }

  void MsgServer::Quiesce(void)
  {
    receiving=false;
    clJobQueueQuiesce(&jq);
  }

  void MsgServer::RegisterHandler(ClWordT msgtype, MsgHandler *handler, ClPtrT cookie)
  {
    if ((msgtype >= NUM_MSG_TYPES)||(msgtype <0))
      {
        logDebug("IOC", "MSG", "message type [%d] out of range [0-255]", (int )msgtype);
        throw Error(Error::SAFPLUS_ERROR, CL_ERR_INVALID_PARAMETER,"message type out of range");
      }
    handlers[msgtype] = handler;
    cookies[msgtype] = cookie;
  }

  void MsgServer::RemoveHandler(ClWordT msgtype)
  {
    if ((msgtype >= NUM_MSG_TYPES)||(msgtype <0))
      {
        logDebug("IOC", "MSG", "message type [%d] out of range [0-255]",(int)msgtype);
        throw Error(Error::SAFPLUS_ERROR, CL_ERR_INVALID_PARAMETER,"message type out of range");
      }
    handlers[msgtype] = NULL;
  }
};
