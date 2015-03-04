#pragma once
#include <clCommon.hxx>
#include <clThreadPool.hxx>
#include <clAppEvent.hxx>
#include <clHandleApi.hxx>
#include <clMsgServerIpi.hxx>

#include <boost/thread/thread.hpp>

namespace SAFplus
{
  class MsgServer;
  class MsgTracker;
  class MsgHandler;
  class MsgTransportPlugin_1;
  class MsgSocket;

  //typedef void (*MsgHandler) (ClIocAddressT from, MsgServer* q, ClPtrT msg, uint_t msglen, ClPtrT cookie);

  const uint_t NUM_MSG_TYPES=256;

  class MsgServer:public AppEvent
  {
  public:

    typedef enum
      {
        DEFAULT_OPTIONS = 0,
        AUTO_ACTIVATE = 1
      } Options;
    
    uint_t sendFailureTimeoutMs;
    
    MsgServer();

    /** Constructor

     */
    MsgServer(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads, Options flags=DEFAULT_OPTIONS); 

    /** 2 Phase Constructor.  See MsgServer()*/
    void Init(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads, Options flags=DEFAULT_OPTIONS);
    
    /** Activate when I am assigned active work */
    void AutoActivate();

    /** Activate when an active work assignment (SI/CSI) contains a particular key.
        This is useful when you want to enable/disable functionality based on data within the work assignment */    
    void AutoActivate(const SaUint8T *workKeyName);
    
    /** Returns the redundant address */
    Handle GetSharedAddress() { return handle;  /* TODO */ }

    /** Exactly this msg queue on this node */
    Handle GetAddress() { return handle; }

    /** Handle a particular type of message
        @param type    A number from 0 to 255 indicating the message type
        @param handler Your handler function
        @param cookie  This pointer will be passed to you handler function
     */
    void RegisterHandler(uint_t type,  MsgHandler *handler, void* cookie);

    /** Remove the handler for particular type of message */
    void RemoveHandler(uint_t type);

    /** Send a message
        @param msgtype The destination message handler
        @param destination Address of the destination node/process
        @param buffer Your data
        @param length Your data length

        Raises the "Error" Exception if something goes wrong, or if the destination queue does not
        exist.
    */
    void SendMsg(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype=0);

    /** Start the server */
    void Start();

    /** Stop message processing right away
        Messages waiting in the queue are not dropped
      */
    void Stop();

    /** Stop this server
        This function stops accepting new messages right away, but does not return until all enqueued messages have been processed, and all processing threads are stopped.
    */
    void Quiesce();

    
    /** Direct messages addressed to the shared address to this queue.
        All queues are always accessible via their unique address, but for high availability purposes, the shared address can be switched between message queues.  This function "claims" the shared address.        
     */
    void MakeMePrimary();

    /** this handle references this message server */
    SAFplus::Handle handle;
    uint_t port;

    /*? get a handle to send to all servers of this type in the
     *  cluster.  It may be more efficient to use Groups since this broadcast
     *  will be handled by every node. */
    SAFplus::Handle broadcastAddr()
      {
      return getProcessHandle(port,Handle::AllNodes);
      }

  protected:
    void Shutdown();

    /** Clears this data structure in preparation for Init().
        This function assumes that this object is full of invalid data and has never been used.
        It generally meant for internal use only.
     */
    void Wipe();

    //ClJobQueueT jq;
    SAFplus::ThreadPool jq;
    MakePrimary makePrimaryThunk;
    friend class MakePrimary; 
    friend class MsgTracker;

    uint32_t compId;
    uint_t reliability;
    uint_t commPort;

    SAFplus::Mutex    mutex;
    SAFplus::ThreadCondition       cond;
    bool              receiving;
    boost::thread     receiverThread;
    MsgHandler*       handlers[NUM_MSG_TYPES];
    ClPtrT            cookies[NUM_MSG_TYPES];
    SAFplus::MsgTransportPlugin_1* transport;
    SAFplus::MsgSocket* sock;

    //friend void MsgTrackerHandler(MsgTracker* rm);
    //friend void SynchronousMakePrimary(MsgServer* q);
    friend void ReceiverFunc(MsgServer* q);
    };


};
