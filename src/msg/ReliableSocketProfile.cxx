#include <ReliableFragment.hxx>
namespace SAFplus
{
#define DEFAULT_SEND_QUEUE_SIZE      32;
#define DEFAULT_RECV_QUEUE_SIZE      32;
#define DEFAULT_SEGMENT_SIZE         128;
#define DEFAULT_OUTSTANDING_SEGS     3;
#define DEFAULT_RETRANS              3;
#define DEFAULT_CUMULATIVE_ACKS      3;
#define DEFAULT_OUT_OF_SEQUENCE      3;
#define DEFAULT_AUTO_RESET           3;
#define DEFAULT_NULL_SEGMENT_TIMEOUT     2000;
#define DEFAULT_RETRANSMISSION_TIMEOUT   600;
#define DEFAULT_CUMULATIVE_ACK_TIMEOUT   300;

  ReliableSocketProfile::ReliableSocketProfile()
  {
    //TODO remove hard code
    m_nMaxSendQueueSize = DEFAULT_SEND_QUEUE_SIZE;
    m_nMaxRecvQueueSize = DEFAULT_RECV_QUEUE_SIZE;
    m_nMaxFragmentSize = DEFAULT_SEGMENT_SIZE;
    m_nMaxOutstandingSegs = DEFAULT_OUTSTANDING_SEGS;
    m_nMaxRetrans = DEFAULT_RETRANS;
    m_nMaxCumulativeAcks = DEFAULT_CUMULATIVE_ACKS;
    m_nMaxOutOfSequence = DEFAULT_OUT_OF_SEQUENCE;
    m_nMaxAutoReset = DEFAULT_AUTO_RESET;
    m_nNullFragmentTimeout = DEFAULT_NULL_SEGMENT_TIMEOUT;
    m_nRetransmissionTimeout = DEFAULT_RETRANSMISSION_TIMEOUT;
    m_nCumulativeAckTimeout = DEFAULT_CUMULATIVE_ACK_TIMEOUT;
  }
  ReliableSocketProfile::ReliableSocketProfile(int maxSendQueueSize,
      int maxRecvQueueSize,
      int maxFragmentSize,
      int maxOutstandingSegs,
      int maxRetrans,
      int maxCumulativeAcks,
      int maxOutOfSequence,
      int maxAutoReset,
      int nullFragmentTimeout,
      int retransmissionTimeout,
      int cumulativeAckTimeout)
  {
    validateValue("maxSendQueueSize", maxSendQueueSize, 1, 255);
    validateValue("maxRecvQueueSize", maxRecvQueueSize, 1, 255);
    validateValue("maxFragmentSize", maxFragmentSize, 22, 65535);
    validateValue("maxOutstandingSegs", maxOutstandingSegs, 1, 255);
    validateValue("maxRetrans", maxRetrans, 0, 255);
    validateValue("maxCumulativeAcks", maxCumulativeAcks, 0, 255);
    validateValue("maxOutOfSequence", maxOutOfSequence, 0, 255);
    validateValue("maxAutoReset", maxAutoReset, 0, 255);
    validateValue("nullFragmentTimeout", nullFragmentTimeout, 0, 65535);
    validateValue("retransmissionTimeout", retransmissionTimeout, 100, 65535);
    validateValue("cumulativeAckTimeout", cumulativeAckTimeout, 100, 65535);

    m_nMaxSendQueueSize = maxSendQueueSize;
    m_nMaxRecvQueueSize = maxRecvQueueSize;
    m_nMaxFragmentSize = maxFragmentSize;
    m_nMaxOutstandingSegs = maxOutstandingSegs;
    m_nMaxRetrans = maxRetrans;
    m_nMaxCumulativeAcks = maxCumulativeAcks;
    m_nMaxOutOfSequence = maxOutOfSequence;
    m_nMaxAutoReset = maxAutoReset;
    m_nNullFragmentTimeout = nullFragmentTimeout;
    m_nRetransmissionTimeout = retransmissionTimeout;
    m_nCumulativeAckTimeout = cumulativeAckTimeout;
  }

  int ReliableSocketProfile::maxSendQueueSize()
  {
    return m_nMaxSendQueueSize;
  }

  int ReliableSocketProfile::maxRecvQueueSize()
  {
    return m_nMaxRecvQueueSize;
  }

  int ReliableSocketProfile::maxFragmentSize()
  {
    return m_nMaxFragmentSize;
  }

  int ReliableSocketProfile::maxOutstandingSegs()
  {
    return m_nMaxOutstandingSegs;
  }

  int ReliableSocketProfile::maxRetrans()
  {
    return m_nMaxRetrans;
  }

  int ReliableSocketProfile::maxCumulativeAcks()
  {
    return m_nMaxCumulativeAcks;
  }

  int ReliableSocketProfile::maxOutOfSequence()
  {
    return m_nMaxOutOfSequence;
  }

  int ReliableSocketProfile::maxAutoReset() {
    return m_nMaxAutoReset;
  }

  int ReliableSocketProfile::nullFragmentTimeout()
  {
    return m_nNullFragmentTimeout;
  }

  int ReliableSocketProfile::retransmissionTimeout()
  {
    return m_nRetransmissionTimeout;
  }

  int ReliableSocketProfile::cumulativeAckTimeout()
  {
    return m_nCumulativeAckTimeout;
  }

  bool ReliableSocketProfile::validateValue(const char* param,
      int value,
      int minValue,
      int maxValue)
  {
    if (value < minValue || value > maxValue)
    {
      // throw new IllegalArgumentException(param);
      throw Error(param);
      return false;
    }
    return true;
  }

}
