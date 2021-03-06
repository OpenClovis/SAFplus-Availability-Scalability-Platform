#pragma once
#include <boost/thread.hpp>
#include <clMsgBase.hxx>
#include <clMsgRelSocketFragment.hxx>
#include <boost/unordered_map.hpp>
static u_int MAX_FRAGMENT_NUMBER = 255 * 255;
namespace SAFplus
{

  class MsgReliableSocketServer;
  enum connectionNotification
  {
    OPENED = 0, REFUSED, CLOSE, FAILURE, RESET
  };

  enum connectionState
  {
    CONN_CLOSED = 0, /* There is not an active or pending connection */
    CONN_SYN_RCVD, /* Request to connect received, waiting ACK */
    CONN_SYN_SENT, /* Request to connect sent */
    CONN_ESTABLISHED, /* Data transfer state */
    CONN_CLOSE_REQUEST
  /* Request to close the connection */
  };

  class rcvListInfomation
  {
  public:
    rcvListInfomation()
    {
      fragNumber = 0;
      lastFrag = 0;
      numberOfCumAck = 0;
      numberOfOutOfSeq = 0;
      numberOfNakFrag = 0; /* Outstanding segments counter */
    }

    int nextFragmentId()
    {
      fragNumber = fragNumber + 1;
      return fragNumber;
    }

    int getFragmentId()
    {
      return fragNumber;
    }

    int setFragmentId(int n)
    {
      fragNumber = n;
      return fragNumber;
    }

    int setLastInSequence(int n)
    {
      lastFrag = n;
      return lastFrag;
    }

    int getLastInSequence()
    {
      return lastFrag;
    }

    void increaseCumulativeAckCounter()
    {
      numberOfCumAck++;
    }

    int getCumulativeAckCounter()
    {
      return numberOfCumAck;
    }

    int resetCumulativeAckFragment()
    {
      int tmp = numberOfCumAck;
      numberOfCumAck = 0;
      return tmp;
    }

    void increaseOutOfSequenceFragment()
    {
      numberOfOutOfSeq++;
    }

    int getOutOfSequenceFragment()
    {
      return numberOfOutOfSeq;
    }

    int resetOutOfSequenceFragment()
    {
      int tmp = numberOfOutOfSeq;
      numberOfOutOfSeq = 0;
      return tmp;
    }

    void increaseNAKFragment()
    {
      numberOfNakFrag++;
    }

    int getNAKFragment()
    {
      return numberOfNakFrag;
    }

    int resetNAKFragment()
    {
      int tmp = numberOfNakFrag;
      numberOfNakFrag = 0;
      return tmp;
    }

    void reset()
    {
      fragNumber = 0;
      lastFrag = 0;
      numberOfOutOfSeq = 0;
      numberOfNakFrag = 0;
      numberOfCumAck = 0;
    }
    int fragNumber; /* Fragment sequence number */
    int lastFrag; /* Last in-Fragment received segment */
    /*
     * The receiver maintains a counter of unacknowledged segments received
     * without an acknowledgment being sent to the transmitter. T
     */
    int numberOfCumAck; /* Cumulative acknowledge counter */

    /*
     * The receiver maintains a counter of the number of segments that have
     * arrived out-of-sequence.
     */
    int numberOfOutOfSeq; /* Out-of-sequence acknowledgments counter */

    /*
     * The transmitter maintains a counter of the number of segments that
     * have been sent without getting an acknowledgment. This is used
     * by the receiver as a mean of flow control.
     */
    int numberOfNakFrag; /* Outstanding segments counter */
  };
  enum TimerStatus
  {
    TIMER_RUN = 0, TIMER_PAUSE, TIMER_UNDEFINE
  };
  class SAFplusTimer
  {
  public:
    TimerStatus status;
    //SAFplus::Mutex timerLock;
    int interval;
    bool started;
    bool isRunning;
  };
  class MsgReliableSocket: public MsgSocket //, SAFPlusLockable
  {
  private:
    /*
     * When this timer expires, the connection is considered broken.
     */

    int sendQueueSize = 6400; /* Maximum number of received segments */
    int recvQueueSize = 6400; /* Maximum number of sent segments */
    int sendBufferSize;
    int recvBufferSize;
    bool isClosed = false; //socket status closed
    bool isReset = false; //socket status reset
    int timeout = 10; /* (ms) */ //wait to receive fragment
    ReliableFragmentList unackedSentQueue; // list of fragment sended without receive ACK
    ReliableFragmentList outOfSeqQueue; // list of out-of-sequence fragments
    ReliableFragmentList inSeqQueue; // list of in-sequence fragments
    //Handle close socket
    void handleCloseImpl(void);
    //process fragment : push fragment to in-sequence or out-of-sequence queue
    void handleReliableFragment(ReliableFragment frag);
    // handle SYN fragment : update connection profile, update connection state
    virtual void handleSYNReliableFragment(SYNFragment *frag);
    //Handle ACK fragment
    void handleACKReliableFragment(ReliableFragment *frag);
    //Handle NAK fragment : Removed acknowledged fragments from unacked-sent queue
    void handleNAKReliableFragment(NAKFragment *frag);
    //Checks for in-sequence segments in the out-of-sequence queue that can be moved to the in-sequence queue
    void checkRecvQueues(void);
    // Send acknowledged fragment to sender
    void sendAcknowledged(bool isFirst = false);
    // Send list of received fragment id in out of sequence queue to sender
    void sendNAK();
    // Send the last received fragment id to sender (out-of-sequence is empty)
    void sendAck(bool isFirst = false);
    // no used
    void sendSYN();
    // Piggy back any pending acknowledgments : Sets the ACK flag and number of a segment if there is at least one received segment to be acknowledged.
    void setACK(ReliableFragment *frag);
    // ????
    void getACK(ReliableFragment *frag);
    static int nextSequenceNumber(int seqn);
    //Thread to receive fragment
    //static void ReliableSocketThread(void * arg);
    //send a reliable fragment
    void sendReliableFragment(ReliableFragment *frag);
    // send fragment and store it in uncheck ack
    void queueAndSendReliableFragment(ReliableFragment* frag);
    //Handle Retransmission un-ack fragment
    void retransmitFragment(ReliableFragment* frag);
    // set connection state
    void setconnection(connectionNotification state);
    void connectionOpened(void);
    //initial a reliable socket : start thread to receive fragment
    void init();

  public:
    int state = connectionState::CONN_CLOSED;
    bool rcvFragmentThreadRunning;
    bool getMsgThreadRunning;
    MsgSocket* xport;
    bool isConnected = false; //socket status connected
    MsgReliableSocketServer *sockServer;
    SAFplus::Mutex sendReliableLock;
    int lastFragmentIdOfMessage;
    ThreadCondition sendReliableCond;
    rcvListInfomation rcvListInfo; //socket queue Infomation
    SAFplus::Mutex closeMutex;
    SAFplus::Mutex resetMutex;
    ThreadCondition resetCond;
    SAFplus::Mutex unackedSentQueueLock;
    ThreadCondition unackedSentQueueCond;
    SAFplus::Mutex thisMutex;
    ThreadCondition thisCond;
    SAFplus::Mutex recvQueueLock;
    ThreadCondition recvQueueCond;
    boost::thread rcvFragmentThread; //thread to receive and handle fragment
    Handle destination; //destination of this connection (node id, port)
    uint_t messageType; //type of the message
    ReliableSocketProfile* profile; //socket connection profile
    SAFplusTimer nullFragmentTimer; //If this timer expired , send null fragment if unackedQueue is empty
    SAFplusTimer retransmissionTimer; //If this timer expired, retransmission all un-ack fragment
    SAFplusTimer cumulativeAckTimer; //If this timer expired, send acknowledge fragment to sender
    boost::intrusive::list_member_hook<> m_reliableSocketmemberHook;
    //receive fragment from socket
    void handleReceiveFragmentThread(void);
    virtual ReliableFragment* receiveReliableFragment(Handle &handle);
    MsgReliableSocket(uint_t port, MsgTransportPlugin_1* transport);
    MsgReliableSocket(uint_t port, MsgTransportPlugin_1* transport,
        Handle destination);
    MsgReliableSocket(MsgSocket* socket);
    virtual ~MsgReliableSocket();
    // Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg);
    virtual bool sendOneMsg(Message* msg);
    //Send a buffer data 
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length,
        uint_t msgtype);
    //Receiver a message
    virtual Message* receive(uint_t maxMsgs, int maxDelay = -1);
    //virtual Message* receiveReassemble(uint_t maxMsgs,int maxDelay=-1);
    virtual Message* receiveFast(uint_t maxMsgs, int maxDelay = -1);
    virtual Message* receiveOrigin(uint_t maxMsgs, int maxDelay = -1);
    virtual void flush();
    //Add socket client to socket  server list 
    virtual void connectionClientOpen();
    //read maximum len byte data
    Byte* readReliable(int offset, int &totalBytes, int maxLen);
    //write maximum len byte data
    void writeReliable(Byte* buffer, int offset, int len);
    //connect to destination
    void connect(Handle destination, int timeout);
    //close socket
    void close(void);
    void closeConnection();
    static void closeImplThread(void*);
    //Puts the connection in a closed state and notifies
    void connectionFailure();
    void handleRetransmissionTimerFunc(void);
    void handleNullFragmentTimerFunc(void);
    void handleCumulativeAckTimerFunc(void);
    void handleReliableFragment(ReliableFragment *frag);
    void resetSocket();
  };

  //The disposer object function
  struct delete_disposer
  {
    void operator()(ReliableFragment *delete_this)
    {
      delete delete_this;
    }
  };
  typedef member_hook<MsgReliableSocket, list_member_hook<>,
      &MsgReliableSocket::m_reliableSocketmemberHook> ReliableSocketMemberHookOption;
  typedef list<MsgReliableSocket, ReliableSocketMemberHookOption> ReliableSocketList;

  class MsgReliableSocketClient: public MsgReliableSocket
  {
  public:
    ReliableFragmentList fragmentQueue;
    SAFplus::Mutex fragmentQueueLock;
    SAFplus::Mutex receiveLock;
    ThreadCondition fragmentQueueCond;
    boost::thread getMsgThread;
    MsgReliableSocketClient(MsgSocket* socket) :
        MsgReliableSocket(socket)
    {
    }
    ;
    MsgReliableSocketClient(MsgSocket* socket, Handle destinationAddress) :
        MsgReliableSocket(socket)
    {
      destination = destinationAddress;
    }
    ;
    void startReceiveThread();
    virtual ReliableFragment* receiveReliableFragment(Handle &handle);
    void receiverFragment(ReliableFragment* frag);
    virtual void connectionClientOpen();
    void resetSocketClient();
    static void getMsgThreadFunc(void * arg);
    void handleGetMsgThread();
    virtual ~MsgReliableSocketClient();

  };
  typedef boost::unordered_map<SAFplus::Handle, MsgReliableSocketClient*,
      boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle> > HandleSockMap;
  class MsgReliableSocketServer: public MsgSocket
  {
    MsgSocket* xport;
    int timeout;
    bool isClosed;
    boost::thread readMsgThread;
    bool readMsgThreadRunning;
    void addClientSocket(Handle destAddress);
    void handlereadMsgThread();
    Handle sendDestination;
    MsgReliableSocketClient* sendSock;
  public:
    virtual void flush();
    std::vector<Message*> msgs;
    ThreadCondition readMsgCond;
    SAFplus::Mutex readMsgLock;
    void removeClientSocket(Handle destAddress);
    void close();
    //void init();
    static void readMsgThreadFunc(void * arg);
    MsgReliableSocketServer(uint_t port, MsgTransportPlugin_1* transport);
    MsgReliableSocketServer(MsgSocket* socket);
    ReliableSocketList listenSock;
    ThreadCondition listenSockCond;
    virtual void send(Message* msg);
    void connect(Handle destination, int timeout);
    SAFplus::Mutex listenSockMutex;
    MsgReliableSocketClient* accept();
    HandleSockMap clientSockTable;
    virtual Message* receive(uint_t maxMsgs, int maxDelay = -1);
    virtual ~MsgReliableSocketServer();
  };
}
;
