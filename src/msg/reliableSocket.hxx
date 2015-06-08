#pragma once
#include <boost/thread.hpp>
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
    CONN_CLOSE_WAIT /* Request to close the connection */
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

      void increaseCumulativeAckCounter()
      {
          numberOfCumAck++;
      }

      int getCumulativeAckCounter()
      {
          return numberOfCumAck;
      }

      int getAndResetCumulativeAckCounter()
      {
          int tmp = numberOfCumAck;
          numberOfCumAck = 0;
          return tmp;
      }

      void increaseOutOfSequenceCounter()
      {
          numberOfOutOfSeq++;
      }

      int getOutOfSequenceCounter()
      {
          return numberOfOutOfSeq;
      }

      int getAndResetOutOfSequenceCounter()
      {
          int tmp = numberOfOutOfSeq;
          numberOfOutOfSeq = 0;
          return tmp;
      }

      void incOutstandingFragmentsCounter()
      {
          numberOfNakFrag++;
      }

      int getOutstandingFragmentsCounter()
      {
          return numberOfNakFrag;
      }

      int getAndResetOutstandingFragmentsCounter()
      {
          int tmp = numberOfNakFrag;
          numberOfNakFrag = 0;
          return tmp;
      }

      void reset()
      {
          numberOfOutOfSeq = 0;
          numberOfNakFrag  = 0;
          numberOfCumAck   = 0;
      }
      int fragNumber;             /* Fragment sequence number */
      int lastFrag;   /* Last in-Fragment received segment */
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
    TIMER_RUN =0,
    TIMER_PAUSE
  };
  class SAFplusTimer
  {
    public:
    pthread_t nullFragmentTimer;
    bool isStop;
    TimerStatus status;
    SAFplus::Mutex timerLock;
    int interval;
  };

  class MsgSocketReliable : public MsgSocketAdvanced //, SAFPlusLockable
  {
    private:
    /*
    * When this timer expires, the connection is considered broken.
    */
    int sendQueueSize = 32; /* Maximum number of received segments */
    int recvQueueSize = 32; /* Maximum number of sent segments */
    int sendBufferSize;
    int recvBufferSize;
    //byte[]  _recvbuffer = new byte[65535];
    bool isClosed    = false;
    bool isConnected = false;
    bool isReset     = false;
    bool iskeepAlive = true;
    int  state     = connectionState::CONN_CLOSED;
    int  timeout   = 0; /* (ms) */
    bool  shutIn  = false;
    bool  shutOut = false;
    queueInfomation *queueInfo;
    ReliableFragmentList unackedSentQueue;
    ReliableFragmentList outOfSeqQueue;
    ReliableFragmentList inSeqQueue;
    ReliableFragmentList listener;
    SAFplus::Mutex closeMutex;
    SAFplus::Mutex resetMutex;
    ThreadCondition resetCond;
    SAFplus::Mutex recvQueueLock;
    ThreadCondition recvQueueCond;
    SAFplus::Mutex unackedSentQueueLock;
    ThreadCondition unackedSentQueueCond;
    SAFplus::Mutex thisMutex;
    ThreadCondition thisCond;

    void handleCloseImpl(void);
    void handleReliableFragment(ReliableFragment frag);
    void handleSYNReliableFragment(SYNFragment *frag);
    void handleACKReliableFragment(ReliableFragment *frag);
    void handleNAKReliableFragment(NAKFragment *frag);
    void checkRecvQueues(void);
    void sendACK();
    void sendNAK();
    void sendSingleAck();
    void sendSYN();
    void setACK(ReliableFragment *frag);
    void getACK(ReliableFragment *frag);
    static int nextSequenceNumber(int seqn);
    static void ReliableSocketThread(void * arg);
    void handleReliableSocketThread(void);
    void sendReliableFragment(ReliableFragment *frag);
    void queueAndSendReliableFragment(ReliableFragment* frag);
    ReliableFragment* receiveReliableFragment(Handle &);
    void retransmitFragment(ReliableFragment* frag);
    void setconnection(connectionNotification state);
    void connectionOpened(void);

    public:
    Handle destination;
    uint_t messageType;
    ReliableSocketProfile* profile;
    SAFplusTimer nullFragmentTimer;
    SAFplusTimer retransmissionTimer;
    SAFplusTimer cumulativeAckTimer;
    SAFplusTimer keepAliveTimer;
    MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport);
    MsgSocketReliable(MsgSocket* socket);
    virtual ~MsgSocketReliable();
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg);
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    void connect(Handle destination, int timeout);
    void close(void);
    void closeImpl();
    static void closeImplThread(void*);
    void connectionFailure();
    void handleRetransmissionTimerFunc(void);
    void handleNullFragmentTimerFunc(void);
    void handleCumulativeAckTimerFunc(void);
    void handleKeepAliveTimerFunc(void);
    void handleReliableFragment(ReliableFragment *frag);
    virtual void flush();


  };


  //The disposer object function
  struct delete_disposer
  {
     void operator()(ReliableFragment *delete_this)
     {
        delete delete_this;
     }
  };

};
