#include <boost/intrusive/list.hpp>
#include "clCommon.hxx"
#include "clMsgBase.hxx"

using namespace boost::intrusive;

#define RUDP_VERSION 1
#define RUDP_HEADER_LEN  5
#define SYN_FLAG   0x80
#define NAK_FLAG   0x20
#define ACK_FLAG   0x40
#define RST_FLAG   0x10
#define NUL_FLAG   0x08
#define CHK_FLAG   0x04
#define FIN_FLAG   0x02
#define LAS_FLAG   0x01
#define SYN_HEADER_LEN  (RUDP_HEADER_LEN + 16)

/*
 *  RUDP Header
 *
 *   0 1 2 3 4 5 6 7 8            15
 *  +-+-+-+-+-+-+-+-+---------------+
 *  |S|A|E|R|N|C| | |    Header     |
 *  |Y|C|A|S|U|H|0|0|    Length     |
 *  |N|K|K|T|L|K| | |               |
 *  +-+-+-+-+-+-+-+-+---------------+
 *  |  Sequence #   +   Ack Number  |
 *  +---------------+---------------+
 *  |            Checksum           |
 *  +---------------+---------------+
 *
 */

namespace SAFplus
{
  static ClUint32T currFragId = 0;
  enum fragmentType
  {
    FRAG_UDE=0,
    FRAG_DATA,
    FRAG_ACK,
    FRAG_NAK,
    FRAG_FIN,
    FRAG_NUL,
    FRAG_RST,
    FRAG_SYN,
    FRAG_LAS
  };

  class ReliableFragment
  {
  protected:
    void init(int _flags, int _seqn, int len, int isLastFrag=0);
    virtual void parseHeader(const Byte* buffer, int _off, int _len);
  public:
    int m_nFalgs;           /* Control flags field */
    int m_nLen;          /* Header length field */
    int m_nSeqn;         /* Sequence number field */
    int m_nAckn;         /* Acknowledgment number field */
    int m_nRetCounter;   /* Retransmission counter */
    bool isFirstSegmentofFragment;
    bool isLast;
    Message *message;
    boost::intrusive::list_member_hook<> m_memberHook;
    Handle address;
    int flags();
    int seq();
    int length();
    int getAck();
    int getRetxCounter();
    void setLast(bool isLastFragment)
    {
      isLast=isLastFragment;
    }
    bool isLastFragment()
    {
      return isLast;
    }
    bool isFirstSegmentOfFrag()
    {
      return isFirstSegmentofFragment;
    }
    void setAck(int _ackn);
    void setRetxCounter(int _retCounter);
    virtual fragmentType getType();
    virtual Byte* getHeader();
    virtual int setHeader(void* ptr);
    virtual Byte* getData()
    {
      return NULL;
    }
    virtual void parseData(const Byte* buffer, int _off, int _len)
    {

    }
    virtual void parseData(Message *message)
    {

    }
    //virtual void parseBytes(const Byte* buffer, int _off, int _len);
    static ReliableFragment* parse(Byte* bytes, int off, int len);
    static ReliableFragment* parse(Byte* bytes);
    virtual ~ReliableFragment()
    {
    }
    ReliableFragment();
  };
  //This option will configure "list" to use the member hook
  typedef member_hook<ReliableFragment, list_member_hook<>,
      &ReliableFragment::m_memberHook> MemberHookOption;
  //This list will use the member hook
  typedef list<ReliableFragment, MemberHookOption> ReliableFragmentList;

  class DATFragment : public ReliableFragment
  {
  private:
  protected:
    virtual void parseHeader(const Byte* buffer, int _off, int _len);
  public:
    DATFragment();
    DATFragment(int seqn, int ackn,Byte* buffer, int off, int len,MsgPool* msgPool,bool isLastFrag=false,bool isLastSegmentOfFrag=false);
    virtual Byte* getData();
    int length();
    virtual Byte* getHeader();
    virtual fragmentType getType();
    virtual void parseData(const Byte* buffer, int _off, int _len);
    virtual void parseData(Message *message);

    ~DATFragment();


  };

  //-----------------------------------------------------
  // SYSFragment
  //-----------------------------------------------------

  class SYNFragment: public ReliableFragment
  {
  private:
//    int m_nVersion;
//    int m_nMaxseg;
//    int m_nOptflags;
//    int m_nMaxsegsize;
//    int m_nRettoval;
//    int m_nCumacktoval;
//    int m_nNiltoval;
//    int m_nMaxret;
//    int m_nMaxcumack;
//    int m_nMaxoutseq;
//    int m_nMaxautorst;
  protected:
    void parseHeader(const Byte* buffer, int off, int len);
  public:
    SYNFragment();
    SYNFragment(int seqn, int maxseg, int maxsegsize, int rettoval,
        int cumacktoval, int niltoval, int maxret,
        int maxcumack, int maxoutseq, int maxautorst,MsgPool* msgPool);
    int getSyncValueOneByte(int pos);
    int getSyncValueTwoByte(int pos);
    int getVersion();
    int getMaxOutstandingFragments();
    int getOptionFlags();
    int getMaxFragmentSize();
    int getRetransmissionTimeout();
    int getCummulativeAckTimeout();
    int getNulFragmentTimeout();
    int getMaxRetransmissions();
    int getMaxCumulativeAcks();
    int getMaxOutOfSequence();
    int getMaxAutoReset();
    virtual Byte* getHeader();
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual void parseData(const Byte* buffer, int _off, int _len);
    virtual void parseData(Message *m);
  };
  // End SysnSegMent
  class ACKFragment : public ReliableFragment
  {
  private:
  protected:

  public:
    ACKFragment();
    ACKFragment(int seqn, int ackn,MsgPool* msgPool);
    virtual fragmentType getType();
  }; // End ACK Fragment Class


  class NAKFragment: public ReliableFragment
  {
  private:
    int* m_pArrAcks;
    int m_nNumNak;
  protected:

    void parseHeader(const Byte* buffer, int off, int len);
  public:
    NAKFragment();
    NAKFragment(int seqn, int ackn,  int* acks, int size,MsgPool *msgPool);
    int* getACKs(int* length);
    virtual Byte* getHeader();
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual void parseData(const Byte* buffer, int _off, int _len);
    virtual void parseData(Message *m);



  }; // End NAK Fragment class

  class FINFragment: public ReliableFragment
  {
  private:
  protected:

  public:
    FINFragment();
    FINFragment(int seqn);
    virtual fragmentType getType();
  }; // End FINFragment Class.

  class NULLFragment : public ReliableFragment
  {
  private:
  protected:

  public:
    virtual fragmentType getType();
    NULLFragment();
    NULLFragment(int seqn);
  }; // End NULL Fragment

  class RSTFragment : public ReliableFragment
  {
  private:

  protected:
  public:
    virtual fragmentType getType();
    RSTFragment();
    RSTFragment(int seqn);
  }; // End RSTFragment Class.

  class ReliableSocketProfile
  {
  private:
    int m_nMaxSendQueueSize;
    int m_nMaxRecvQueueSize;
    int m_nMaxFragmentSize;
    int m_nMaxOutstandingSegs;
    int m_nMaxRetrans;
    int m_nMaxCumulativeAcks;
    int m_nMaxOutOfSequence;
    int m_nMaxAutoReset;
    int m_nNullFragmentTimeout;
    int m_nRetransmissionTimeout;
    int m_nCumulativeAckTimeout;
  protected:
    bool validateValue(const char* param, int value, int minValue, int maxValue);
  public:
    ReliableSocketProfile();
    ReliableSocketProfile(int maxSendQueueSize,
        int maxRecvQueueSize,
        int maxFragmentSize,
        int maxOutstandingSegs,
        int maxRetrans,
        int maxCumulativeAcks,
        int maxOutOfSequence,
        int maxAutoReset,
        int nullFragmentTimeout,
        int retransmissionTimeout,
        int cumulativeAckTimeout);
    /**
     * Returns the maximum send queue size (packets).
     */
    int maxSendQueueSize();

    /**
     * Returns the maximum receive queue size (packets).
     */
    int maxRecvQueueSize();

    /**
     * Returns the maximum segment size (octets).
     */
    int maxFragmentSize();
    /**
     * Returns the maximum number of outstanding segments.
     */
    int maxOutstandingSegs();
    /**
     * Returns the maximum number of consecutive retransmissions (0 means unlimited).
     */
    int maxRetrans();
    /**
     * Returns the maximum number of unacknowledged received segments.
     */
    int maxCumulativeAcks();

    /**
     * Returns the maximum number of out-of-sequence received segments.
     */
    int maxOutOfSequence();

    /**
     * Returns the maximum number of consecutive auto resets.
     */
    int maxAutoReset();

    /**
     * Returns the null segment timeout (ms).
     */
    int nullFragmentTimeout();

    /**
     * Returns the retransmission timeout (ms).
     */
    int retransmissionTimeout();

    /**
     * Returns the cumulative acknowledge timeout (ms).
     */
    int cumulativeAckTimeout();


  };
}
