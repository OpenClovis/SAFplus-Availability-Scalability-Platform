#include <boost/intrusive/list.hpp>
#include "clCommon.hxx"
#include "clMsgBase.hxx"

using namespace boost::intrusive;

#define RUDP_VERSION 1
#define RUDP_HEADER_LEN  6
#define SYN_FLAG   0x80
#define ACK_FLAG   0x40
#define NAK_FLAG   0x20
#define RST_FLAG   0x10
#define NUL_FLAG   0x08
#define CHK_FLAG   0x04
#define FIN_FLAG   0x02
#define LAS_FLAG   0x01
#define SYN_DATA_LEN  13

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
    int controlFlag;           /* Control flags field */
    int fragmentId;         /* Sequence number field */
    int ackNumber;         /* Acknowledgment number field */
    int retransCounter;   /* Retransmission counter */
    bool isLast;

  protected:
    void initFragment(int _flags, int _seqn, int isLastFrag=0);
  public:
    bool isFirst;
    boost::intrusive::list_member_hook<> m_memberHook;
    Handle address;
    Byte* data;
    int dataLen;
    Message* message;
    void setMessage(Message *msg);
    int getFlag();
    int getFragmentId();
    int getAck();
    int getRetxCounter();
    void setLast(bool isLastFragment);
    bool isLastFragment();
    void setAck(int _ackn);
    void setRetxCounter(int _retCounter);
    static ReliableFragment* parse(Byte* bytes, int len);
    static ReliableFragment* parse(Byte* bytes);
    int setHeader(void* ptr);
    virtual int getlength();
    virtual Byte* getData();
    virtual fragmentType getType();
    virtual void parseHeader(const Byte* buffer, int _len);
    virtual Byte* getHeader();
    virtual void parseData(Byte* buffer, int _len);
    virtual ~ReliableFragment()
    {
      if(message!=NULL)
      {
        message->msgPool->free(message);
      }
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
  public:
    DATFragment();
    ~DATFragment();

    DATFragment(int seqn, int ackn,Byte* buffer, int off, int len, bool isLastFrag, bool isFirstSegmentOfFrag=false);

    virtual Byte* getData();
    virtual int getlength();
    virtual fragmentType getType();
    virtual void parseData(Byte* buffer, int _len);
  };

  class SYNFragment: public ReliableFragment
  {
  private:
    int maxFragment;
    int maxFragmentSize;
    int retransInterval;
    int cumAckInterval;
    int emptyInterVal;
    int maxRetrans;
    int maxCumAck;
    int maxOutSeq;
    int maxAutoReset;
  public:
    SYNFragment();
    SYNFragment(int seqn, int maxseg, int maxsegsize, int rettoval,
        int cumacktoval, int niltoval, int maxret,
        int maxcumack, int maxoutseq, int maxautorst);
    int getVersion();
    int getMaxOutstandingFragments();
    int getOptionFlags();
    int getMaxFragmentSize();
    int getRetransmissionIntervel();
    int getCummulativeAckInterval();
    int getNulFragmentInterval();
    int getMaxRetrans();
    int getMaxCumulativeAcks();
    int getMaxOutOfSequence();
    int getMaxAutoReset();
    virtual Byte* getData();
    virtual fragmentType getType();
    virtual void parseData(Byte* buffer, int _len);
    virtual int getlength();

  };


  class ACKFragment : public ReliableFragment
  {
  private:

  public:
    ACKFragment();
    ACKFragment(int seqn, int ackn);
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual int getlength()
    {
      return 0;
    }
  }; // End ACK Fragment Class


  class NAKFragment: public ReliableFragment
  {
  private:
    int* nakData;
    int nakNumber;
  public:
    NAKFragment();
    NAKFragment(int seqn, int ackn,  int* acks, int size);
    int* getNAKs(int* length);
    virtual fragmentType getType();
    virtual Byte* getData();
    virtual void parseData(Byte* buffer, int _len);
    virtual int getlength()
    {
      return nakNumber*2;
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
    virtual int getlength()
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
    virtual int getlength()
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
    virtual int getlength()
    {
      return 0;
    }
  };

  class ReliableSocketProfile
  {
  private:
    int pMaxSndListSize;
    int pMaxRcvListSize;
    int pMaxFragmentSize;
    int pMaxNAKFragments;
    int pMaxRetrans;
    int pMaxCumulativeAcks;
    int pMaxOutOfSequence;
    int pMaxAutoReset;
    int pNullFragmentInterval;
    int pRetransmissionInterval;
    int pCumulativeAckInterval;
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
    int maxSndListSize();

    /**
     * Returns the maximum receive queue size (packets).
     */
    int maxRcvListSize();

    /**
     * Returns the maximum segment size (octets).
     */
    int maxFragmentSize();
    /**
     * Returns the maximum number of outstanding segments.
     */
    int maxNAKFrags();
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
    int getNullFragmentInterval();

    /**
     * Returns the retransmission timeout (ms).
     */
    int getRetransmissionInterval();

    /**
     * Returns the cumulative acknowledge timeout (ms).
     */
    int getCumulativeAckInterval();
  };
}
