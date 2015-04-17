#pragma once
#include <clMsgBase.hxx>
#include <ReliableFragment.hxx>

//TODO
u_int RELIABLE_MSG_TYPE = 0x50;
static u_int MAX_FRAGMENT_NUMBER        = 255;

namespace SAFplus
{

  enum connectionNotification
  {
    OPENED=0,
    REFUSED,
    CLOSE,
    FAILURE,
    RESET
  };

  enum connectionState
  {
    CONN_CLOSED      = 0, /* There is not an active or pending connection */
    CONN_SYN_RCVD, /* Request to connect received, waiting ACK */
    CONN_SYN_SENT, /* Request to connect sent */
    CONN_ESTABLISHED, /* Data transfer state */
    CONN_CLOSE_WAIT4 /* Request to close the connection */
  };

  class queueInfomation
  {
    public:
	  queueInfomation()
      {
      }

      int nextSequenceNumber()
      {
          fragNumber = fragNumber+1;
          return fragNumber;
      }

      int setSequenceNumber(int n)
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

      void incCumulativeAckCounter()
      {
          _cumAckCounter++;
      }

      int getCumulativeAckCounter()
      {
          return _cumAckCounter;
      }

      int getAndResetCumulativeAckCounter()
      {
          int tmp = _cumAckCounter;
          _cumAckCounter = 0;
          return tmp;
      }

      void incOutOfSequenceCounter()
      {
          _outOfSeqCounter++;
      }

      int getOutOfSequenceCounter()
      {
          return _outOfSeqCounter;
      }

      int getAndResetOutOfSequenceCounter()
      {
          int tmp = _outOfSeqCounter;
          _outOfSeqCounter = 0;
          return tmp;
      }

      void incOutstandingSegsCounter()
      {
          nakFragCounter++;
      }

      int getOutstandingSegsCounter()
      {
          return nakFragCounter;
      }

      int getAndResetOutstandingSegsCounter()
      {
          int tmp = nakFragCounter;
          nakFragCounter = 0;
          return tmp;
      }

      void reset()
      {
          _outOfSeqCounter = 0;
          nakFragCounter  = 0;
          _cumAckCounter   = 0;
      }
      int fragNumber;             /* Fragment sequence number */
      int lastFrag;   /* Last in-Fragment received segment */
      /*
       * The receiver maintains a counter of unacknowledged segments received
       * without an acknowledgment being sent to the transmitter. T
       */
      int _cumAckCounter; /* Cumulative acknowledge counter */

      /*
       * The receiver maintains a counter of the number of segments that have
       * arrived out-of-sequence.
       */
      int _outOfSeqCounter; /* Out-of-sequence acknowledgments counter */

      /*
       * The transmitter maintains a counter of the number of segments that
       * have been sent without getting an acknowledgment. This is used
       * by the receiver as a mean of flow control.
       */
      int nakFragCounter; /* Outstanding segments counter */
  };
  enum TimerStatus
  {
    TIMER_RUN =0,
    TIMER_PAUSE
  };
  class SAFplusTimer
  {
    public:
    pthread_t nullFragmentTimer;
    bool isStop;
    TimerStatus status;
    int interval;
  };
  class MsgSocketReliable : public MsgSocketAdvanced
  {
    private:
    /*
    * When this timer expires, the connection is considered broken.
    */
    int _sendQueueSize = 32; /* Maximum number of received segments */
    int _recvQueueSize = 32; /* Maximum number of sent segments */
    int _sendBufferSize;
    int _recvBufferSize;
    //byte[]  _recvbuffer = new byte[65535];
    bool isClosed    = false;
    bool isConnected = false;
    bool isReset     = false;
    bool iskeepAlive = true;
    int  state     = connectionState::CONN_CLOSED;
    int  timeout   = 0; /* (ms) */
    bool  shutIn  = false;
    bool  shutOut = false;
    queueInfomation queueInfo;
    std::list<ReliableFragment> _unackedSentQueue;
    std::list<ReliableFragment> _outSeqRecvQueue;
    std::list<ReliableFragment> _inSeqRecvQueue;
    //private ArrayList _unackedSentQueue = new ArrayList(); /* Unacknowledged segments send queue */
    //private ArrayList _outSeqRecvQueue  = new ArrayList(); /* Out-of-sequence received segments queue */
    //private ArrayList _inSeqRecvQueue   = new ArrayList(); /* In-sequence received segments queue */
    SAFplus::Mutex closeMutex;
    SAFplus::Mutex resetMutex;

    void handleReliableFragment(ReliableFragment frag);
    void handleSYNReliableFragment();
    void handleACKReliableFragment();
    void handleNAKReliableFragment();
    void sendACK();
    void sendUACK();
    void sendSYN();
    void setACK(ReliableFragment frag);
    void getACK(ReliableFragment frag);

    void sendReliableFragment(ReliableFragment frag);
    void queueAndSendReliableFragment(ReliableFragment frag);
    ReliableFragment receiveReliableFragment();
    void retransmitSegment(ReliableFragment frag);
    void setconnection(connectionNotification state);

    public:
    Handle destination;
    uint_t messageType;
    ReliableSocketProfile profile;
    SAFplusTimer nullFragmentTimer;
    SAFplusTimer retransmissionTimer;
    SAFplusTimer cumulativeAckTimer;
    SAFplusTimer keepAliveTimer;
    MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport);
    MsgSocketReliable(MsgSocket* socket);
    virtual ~MsgSocketReliable();
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg,uint_t length);
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);

  };


};
