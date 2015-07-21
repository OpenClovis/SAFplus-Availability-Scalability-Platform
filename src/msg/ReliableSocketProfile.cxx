#include <ReliableFragment.hxx>
namespace SAFplus
{
#define MAX_SEND_QUEUE_SIZE      32;
#define MAX_RECV_QUEUE_SIZE      32;
#define MAX_SEGMENT_SIZE         128;
#define MAX_OUTSTANDING_SEGS     3;
#define MAX_RETRANS              3;
#define MAX_CUMULATIVE_ACKS      3;
#define MAX_OUT_OF_SEQUENCE      3;
#define MAX_AUTO_RESET           3;
#define NULL_SEGMENT_TIMEOUT     2000;
#define RETRANSMISSION_TIMEOUT   600;
#define CUMULATIVE_ACK_TIMEOUT   300;

  ReliableSocketProfile::ReliableSocketProfile()
  {
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
    checkValue("maxSendQueueSize", maxSendQueueSize, 1, 255);
    checkValue("maxRecvQueueSize", maxRecvQueueSize, 1, 255);
    checkValue("maxFragmentSize", maxFragmentSize, 22, 65535);
    checkValue("maxOutstandingSegs", maxOutstandingSegs, 1, 255);
    checkValue("maxRetrans", maxRetrans, 0, 255);
    checkValue("maxCumulativeAcks", maxCumulativeAcks, 0, 255);
    checkValue("maxOutOfSequence", maxOutOfSequence, 0, 255);
    checkValue("maxAutoReset", maxAutoReset, 0, 255);
    checkValue("nullFragmentTimeout", nullFragmentTimeout, 0, 65535);
    checkValue("retransmissionTimeout", retransmissionTimeout, 100, 65535);
    checkValue("cumulativeAckTimeout", cumulativeAckTimeout, 100, 65535);

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

  bool ReliableSocketProfile::checkValue(const char* param,
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
