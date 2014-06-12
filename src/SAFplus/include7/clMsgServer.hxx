#pragma once

#include <clIocApi.h>
#include <clJobQueue.h>
#include <clAppEvent.hxx>

namespace SAFplus
{

  class MsgServer;
  class MsgTracker;
  class MsgHandler;

  //typedef void (*MsgHandler) (ClIocAddressT from, MsgServer* q, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

  const ClWordT NUM_MSG_TYPES=256;
  
  class MsgServer:public AppEvent
  {
  public:

    typedef enum
      {
        DEFAULT_OPTIONS = 0,
        AUTO_ACTIVATE = 1
      } Options;
    
    ClWordT sendFailureTimeoutMs;
    
    MsgServer();

    /** Constructor

     */
    MsgServer(ClWordT port, ClWordT maxPendingMsgs, ClWordT maxHandlerThreads, Options flags=DEFAULT_OPTIONS); 

    /** 2 Phase Constructor.  See MsgServer()*/
    void Init(ClWordT port, ClWordT maxPendingMsgs, ClWordT maxHandlerThreads, Options flags=DEFAULT_OPTIONS);
    
    /** Activate when I am assigned active work */
    void AutoActivate();

    /** Activate when an active work assignment (SI/CSI) contains a particular key.
        This is useful when you want to enable/disable functionality based on data within the work assignment */    
    void AutoActivate(const SaUint8T *workKeyName);
    
    /** Returns the redundant address */
    ClIocAddressT GetSharedAddress() { return sharedAddr; }

    /** Exactly this msg queue on this node */
    ClIocAddressT GetAddress() { return uniqueAddr; }

    /** Handle a particular type of message
        @param type    A number from 0 to 255 indicating the message type
        @param handler Your handler function
        @param cookie  This pointer will be passed to you handler function
     */
    void RegisterHandler(ClWordT type,  MsgHandler *handler, ClPtrT cookie);

    /** Remove the handler for particular type of message */
    void RemoveHandler(ClWordT type);

    /** Send a message
        @param msgtype The destination message handler
        @param destination Address of the destination node/process
        @param buffer Your data
        @param length Your data length

        Raises the "Error" Exception if something goes wrong, or if the destination queue does not
        exist.
    */
    void SendMsg(ClIocAddressT destination, void* buffer, ClWordT length,ClWordT msgtype=0);

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

  protected:
    void Shutdown();

    /** Clears this data structure in preparation for Init().
        This function assumes that this object is full of invalid data and has never been used.
        It generally meant for internal use only.
     */
    void Wipe();

    
    ClIocAddressT sharedAddr;
    ClIocAddressT uniqueAddr;
    
    ClJobQueueT jq;
    ClUint32T compId;
    ClWordT port;
    ClWordT reliability;
    ClIocCommPortHandleT commPort;

    ClOsalMutexT      mutex;
    ClOsalCondT       cond;
    ClBoolT           receiving;

    MsgHandler        *handlers[NUM_MSG_TYPES];
    ClPtrT            cookies[NUM_MSG_TYPES];

    friend void MsgTrackerHandler(MsgTracker* rm);
    friend void SynchronousMakePrimary(MsgServer* q);
    friend void ReceiverFunc(MsgServer* q);
    };


};
