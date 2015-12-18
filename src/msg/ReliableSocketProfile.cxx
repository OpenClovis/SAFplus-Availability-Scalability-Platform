#include <ReliableFragment.hxx>

#define DEFAULT_SEND_QUEUE_SIZE      320;
#define DEFAULT_RECV_QUEUE_SIZE      320;
#define DEFAULT_SEGMENT_SIZE         64000;
#define DEFAULT_NAK_FRAG     3;
#define DEFAULT_RETRANS              9;
#define DEFAULT_CUMULATIVE_ACKS      2;
#define DEFAULT_OUT_OF_SEQUENCE      9;
#define DEFAULT_AUTO_RESET           3;
#define DEFAULT_NULL_SEGMENT_TIMEOUT     200;
#define DEFAULT_RETRANSMISSION_TIMEOUT   800;
#define DEFAULT_CUMULATIVE_ACK_TIMEOUT   300;

namespace SAFplus
{
  ReliableSocketProfile::ReliableSocketProfile()
  {
    //TODO remove hard code
    pMaxSndListSize = DEFAULT_SEND_QUEUE_SIZE;
    pMaxRcvListSize = DEFAULT_RECV_QUEUE_SIZE;
    pMaxFragmentSize = DEFAULT_SEGMENT_SIZE;
    pMaxNAKFragments = DEFAULT_NAK_FRAG;
    pMaxRetrans = DEFAULT_RETRANS;
    pMaxCumulativeAcks = DEFAULT_CUMULATIVE_ACKS;
    pMaxOutOfSequence = DEFAULT_OUT_OF_SEQUENCE;
    pMaxAutoReset = DEFAULT_AUTO_RESET;
    pNullFragmentInterval = DEFAULT_NULL_SEGMENT_TIMEOUT;
    pRetransmissionInterval = DEFAULT_RETRANSMISSION_TIMEOUT;
    pCumulativeAckInterval = DEFAULT_CUMULATIVE_ACK_TIMEOUT;
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
    validateValue("maxSendQueueSize", maxSendQueueSize, 1, 6401);
    validateValue("maxRecvQueueSize", maxRecvQueueSize, 1, 6401);
    validateValue("maxFragmentSize", maxFragmentSize, 22, 65535);
    validateValue("maxOutstandingSegs", maxOutstandingSegs, 1, 512);
    validateValue("maxRetrans", maxRetrans, 0, 255);
    validateValue("maxCumulativeAcks", maxCumulativeAcks, 0, 512);
    validateValue("maxOutOfSequence", maxOutOfSequence, 0, 512);
    validateValue("maxAutoReset", maxAutoReset, 0, 512);
    validateValue("nullFragmentTimeout", nullFragmentTimeout, 0, 65535);
    validateValue("retransmissionTimeout", retransmissionTimeout, 100, 65535);
    validateValue("cumulativeAckTimeout", cumulativeAckTimeout, 100, 65535);

    pMaxSndListSize = maxSendQueueSize;
    pMaxRcvListSize = maxRecvQueueSize;
    pMaxFragmentSize = maxFragmentSize;
    pMaxNAKFragments = maxOutstandingSegs;
    pMaxRetrans = maxRetrans;
    pMaxCumulativeAcks = maxCumulativeAcks;
    pMaxOutOfSequence = maxOutOfSequence;
    pMaxAutoReset = maxAutoReset;
    pNullFragmentInterval = nullFragmentTimeout;
    pRetransmissionInterval = retransmissionTimeout;
    pCumulativeAckInterval = cumulativeAckTimeout;
  }

  int ReliableSocketProfile::maxSndListSize()
  {
    return pMaxSndListSize;
  }

  int ReliableSocketProfile::maxRcvListSize()
  {
    return pMaxRcvListSize;
  }

  int ReliableSocketProfile::maxFragmentSize()
  {
    return pMaxFragmentSize;
  }

  int ReliableSocketProfile::maxNAKFrags()
  {
    return pMaxNAKFragments;
  }

  int ReliableSocketProfile::maxRetrans()
  {
    return pMaxRetrans;
  }

  int ReliableSocketProfile::maxCumulativeAcks()
  {
    return pMaxCumulativeAcks;
  }

  int ReliableSocketProfile::maxOutOfSequence()
  {
    return pMaxOutOfSequence;
  }

  int ReliableSocketProfile::maxAutoReset() {
    return pMaxAutoReset;
  }

  int ReliableSocketProfile::getNullFragmentInterval()
  {
    return pNullFragmentInterval;
  }

  int ReliableSocketProfile::getRetransmissionInterval()
  {
    return pRetransmissionInterval;
  }

  int ReliableSocketProfile::getCumulativeAckInterval()
  {
    return pCumulativeAckInterval;
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
