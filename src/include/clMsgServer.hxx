//? <section name="Messaging">

#pragma once
#include <clCommon.hxx>
#include <clThreadPool.hxx>
#include <clAppEvent.hxx>
#include <clHandleApi.hxx>
#include <clMsgServerIpi.hxx>
#include <clMsgBase.hxx>
#include <boost/thread/thread.hpp>

namespace SAFplus
{
  class MsgServer;
  class MsgTracker;
  class MsgHandler;
  class MsgTransportPlugin_1;
  class MsgSocket;
  class Fault;

  //typedef void (*MsgHandler) (ClIocAddressT from, MsgServer* q, ClPtrT msg, uint_t msglen, ClPtrT cookie);

  const uint_t NUM_MSG_TYPES=256;

  //? <class> This class wraps the message transport layer, providing the mechanism to multiplex and demultiplex messages of different types onto a single underlying communications port.
  // This class is exposed so applications can create communications ports independent of SAFplus internal communications.  For most messaging applicatons, you can multiplex your messages within the SAFplus application communications port.  See SafplusMsgServer for details.
  // [TODO] validate and add info about failover
  class MsgServer:public AppEvent
  {
  public:

    typedef enum
      {
        DEFAULT_OPTIONS = 0,
        AUTO_ACTIVATE = 1
      } Options;
      typedef enum
        {
          SOCK_DEFAULT = 0,
          SOCK_SHAPING = 1,
          SOCK_SEGMENTATION = 2,
          SOCK_RELIABLE = 3
        } SocketType;

    
    uint_t sendFailureTimeoutMs;
    
    //? <ctor> This 2 phase constructor requires that you call Init() to actually use the created object </ctor>
    MsgServer();

    /*? <ctor> Single phase constructor.
     <arg name='port'> </arg>
     <arg name='maxPendingMsgs'>This object will queue no more than this many messages from the underlying transport.  This allows the transport to 'push back' on the sender during overload conditions (if the transport supports that feature) </arg>
     <arg name='maxHandlerThreads'>This is the maximum number of threads handling received messages.  Handler threads call application functions via MsgHandler::msgHandler(), so the length of time a thread may be busy is indeterminate.   In particular, a handler thread may itself make a blocking call, waiting to receive another message from this MsgServer.  With only one thread, this will deadlock.  Therefore depending on your application, it may make sense to have multiple handler threads for performance optimization and deadlock avoidance.   </arg>
     <arg name='flags' default="DEFAULT_OPTIONS">Message server options.  Currently, you can choose for this server to remain quiescent (not connected) until the AMF assigns active work to the process.  </arg>
     </ctor>
     */
    MsgServer(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads, Options flags=DEFAULT_OPTIONS, SocketType type = SOCK_DEFAULT);

    ~MsgServer();

    /*? 2 Phase Constructor.  See MsgServer(...) for parameter details */
    void Init(uint_t port, uint_t maxPendingMsgs, uint_t maxHandlerThreads, Options flags=DEFAULT_OPTIONS, SocketType type = SOCK_DEFAULT);
    
    /*? Activate when I am assigned active work. Overrides the options flags. */
    void AutoActivate();

    /*? Activate when an active work assignment (SI/CSI) contains a particular key.
        This is useful when you want to enable/disable functionality based on data within the work assignment */    
    void AutoActivate(const SaUint8T *workKeyName);
    
    /*? [NOT IMPLEMENTED] Returns the redundant address */
    Handle GetSharedAddress() { return handle;  /* TODO */ }

    /*? Return the address of this msg queue on this node */
    Handle GetAddress() { return handle; }

    /*? Handle a particular type of message
        <arg name='type'>A number from 0 to 255 indicating the message type.  A particular message type identifies a particular handler object.  Make sure that you use unique message types!</arg>
        <arg name='handler'>Your handler function</arg>
        <arg name='cookie'>This pointer will be passed back to your derived MsgHandler::msgHandler() handler function.  You can use it to establish context.</arg>
     */
    void RegisterHandler(uint_t type,  MsgHandler *handler, void* cookie);

    /*? Remove the handler for particular type of message 
        <arg name='type'>The type that you passed when RegisterHandler() was called</arg>
     */
    void RemoveHandler(uint_t type);

    /*? Send a message
        <arg name='destination'> Address of the destination node/process</arg>
        <arg name='buffer'> Your data</arg>
        <arg name='length'> Your data length</arg>
        <arg name='msgtype'> The destination message handler's type (that it passed when calling RegisterHandler())</arg>

        Raises the "Error" Exception if something goes wrong.
    */
    void SendMsg(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype=0);

    /*? Send a message
        <arg name='destination'> Address of the destination node/process</arg>
        <arg name='buffer'> Your data</arg>
        <arg name='length'> Your data length</arg>
        <arg name='msgtype'> The destination message handler's type (that it passed when calling RegisterHandler()).  This will be prepended to every message passed in the msg list.</arg>

        Raises the "Error" Exception if something goes wrong.
    */
    void SendMsg(Message* msg,uint_t msgtype=0);

    /*? Start the server listening on the port */
    void Start();

    //? Stop message processing right away.  Messages waiting in the queue are not dropped.
    void Stop();

    //? Stop this server. This function stops accepting new messages right away, but does not return until all enqueued messages have been processed, and all processing threads are stopped.
    void Quiesce();
  
    //? Direct messages addressed to the shared address to this queue.  All queues are always accessible via their unique address, but for high availability purposes, the shared address can be switched between message queues.  This function "claims" the shared address.           
    void MakeMePrimary();

    //? This handle is the address for this message server
    SAFplus::Handle handle;
    //? The port for the message server
    uint_t port;

    //? Get a handle to send to all servers of this type in the cluster.  It may be more efficient to use Groups since this broadcast address will trigger a transport layer broadcast -- that is, this message will be handled by every node.
    SAFplus::Handle broadcastAddr()
      {
      return getProcessHandle(port,Handle::AllNodes);
      }

    //? Get the message buffer pool used by this MsgServer.
    MsgPool& getMsgPool() { return *sock->getMsgPool(); }

    Fault* fault;  //? You need to initialize this if you want the message server to gain knowledge of system faults

    //? Get the underlying transport socket.  This is used to add socket layers to your network stack.  If you use this socket to inject messages, you will confuse the receiving side.
    SAFplus::MsgSocket* getSocket() { return sock; }
    //? Set the underlying transport socket.  This is used to add socket layers to your network stack.  
    void setSocket(SAFplus::MsgSocket* s) { sock = s; }

    //? Get the maximum theoretical message length supported by this server.  This may exceed your available memory.  Sockets that support unlimited sizes will return UINT_MAX.
    uint_t maxMsgLength()
    {
      return sock->cap.maxMsgSize - 1;  // Subtract 1 to account for the message type prefix
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
  }; //? </class>
};

//? </section>
