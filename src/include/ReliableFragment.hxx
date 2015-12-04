#include <boost/intrusive/list.hpp>
#include "clCommon.hxx"
#include "clMsgBase.hxx"

using namespace boost::intrusive;

#define RUDP_VERSION 1
#define RUDP_HEADER_LEN  5
#define SYN_FLAG   0x80
#define ACK_FLAG   0x40
#define NAK_FLAG   0x20
#define RST_FLAG   0x10
#define NUL_FLAG   0x08
#define CHK_FLAG   0x04
#define FIN_FLAG   0x02
#define LAS_FLAG   0x01
#define SYN_DATA_LEN  16

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
  private:
    int m_nFalgs;           /* Control flags field */
    int m_nLen;          /* Fragment length field */
    int m_nSeqn;         /* Sequence number field */
    int m_nAckn;         /* Acknowledgment number field */
    int m_nRetCounter;   /* Retransmission counter */
    bool isLast;

  protected:
    void init(int _flags, int _seqn, int len, int isLastFrag=0);
  public:
    bool isFirstSegmentofFragment;
    boost::intrusive::list_member_hook<> m_memberHook;
    Handle address;
    Byte* m_pData;
    int dataLen;
    Message* message;
    void parseDataMessage(Message *msg);
    int flags();
    int seq();
    virtual int length();
    int getAck();
    int getRetxCounter();
    virtual Byte* getData();
    void setLast(bool isLastFragment)
    {
      isLast=isLastFragment;
    }
    bool isLastFragment()
    {
      return isLast;
    }
    void setAck(int _ackn);
    void setRetxCounter(int _retCounter);
    static ReliableFragment* parse(Byte* bytes, int len);
    static ReliableFragment* parse(Byte* bytes);
    int setHeader(void* ptr);
    virtual fragmentType getType();
    virtual void parseHeader(const Byte* buffer, int _len);
    virtual Byte* getHeader();
    virtual void parseData(Byte* buffer, int _len);
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
  public:
    DATFragment();
    DATFragment(int seqn, int ackn,Byte* buffer, int off, int len, bool isLastFrag, bool isFirstSegmentOfFrag=false);
    virtual Byte* getData();
    virtual int length();
    virtual fragmentType getType();
    virtual void parseData(Byte* buffer, int _len);
    ~DATFragment();


  };

  class SYNFragment: public ReliableFragment
  {
  private:
    int m_nVersion;
    int m_nMaxseg;
    int m_nOptflags;
    int m_nMaxsegsize;
    int m_nRettoval;
    int m_nCumacktoval;
    int m_nNiltoval;
    int m_nMaxret;
    int m_nMaxcumack;
    int m_nMaxoutseq;
    int m_nMaxautorst;
  public:
    SYNFragment();
    SYNFragment(int seqn, int maxseg, int maxsegsize, int rettoval,
        int cumacktoval, int niltoval, int maxret,
        int maxcumack, int maxoutseq, int maxautorst);
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
    virtual Byte* getData();
    virtual fragmentType getType();
    virtual void parseData(Byte* buffer, int _len);

  };


  class ACKFragment : public ReliableFragment
  {
  private:

  public:
    ACKFragment();
    ACKFragment(int seqn, int ackn);
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual int length()
    {
      return 0;
    }
  }; // End ACK Fragment Class


  class NAKFragment: public ReliableFragment
  {
  private:
    int* m_pArrAcks;
    int m_nNumNak;
  public:
    NAKFragment();
    NAKFragment(int seqn, int ackn,  int* acks, int size);
    int* getACKs(int* length);
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual void parseData(Byte* buffer, int _len);
    virtual int length()
    {
      return m_nNumNak;
    }

  }; // End NAK Fragment class

  class FINFragment: public ReliableFragment
  {
  private:
  protected:

  public:
    FINFragment();
    FINFragment(int seqn);
    virtual fragmentType getType();
    virtual Byte* getData()
    {
      return NULL;
    }
    virtual int length()
    {
      return 0;
    }

  };

  class NULLFragment : public ReliableFragment
  {
  private:
  protected:

  public:
    virtual fragmentType getType();
    NULLFragment();
    NULLFragment(int seqn);
    virtual Byte* getData()
    {
      return NULL;
    }
    virtual int length()
    {
      return 0;
    }
  };

  class RSTFragment : public ReliableFragment
  {
  private:

  protected:
  public:
    virtual fragmentType getType();
    RSTFragment();
    RSTFragment(int seqn);
    virtual Byte* getData()
    {
      return NULL;
    }
    virtual int length()
    {
      return 0;
    }
  };

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
