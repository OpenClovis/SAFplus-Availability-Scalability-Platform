#include <clMsgRelSocket.hxx>
#include "clLogApi.hxx"
#include "clCommon.hxx"
#include <stdio.h>
#define MAXSIZE 64000
#define MSG_SEND_TIMEOUT 10000
namespace SAFplus
{
  // Callback function for Null Fragment Timer
  static void * nullFragmentTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*) arg;
    while(tempMsgSocket->nullFragmentTimer.isRunning)
    {
      usleep(tempMsgSocket->nullFragmentTimer.interval * 1000);
      if(tempMsgSocket->nullFragmentTimer.status == TIMER_RUN)
      {
        tempMsgSocket->handleNullFragmentTimerFunc();
      }
    }
    return NULL;
  }

  // Callback function for retransmission Timer
  static void* retransmissionTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*) arg;
    while(tempMsgSocket->retransmissionTimer.isRunning)
    {
      usleep(tempMsgSocket->retransmissionTimer.interval * 1000);
      if(tempMsgSocket->retransmissionTimer.status == TIMER_RUN)
      {
        tempMsgSocket->handleRetransmissionTimerFunc();
      }
    }
    return NULL;
  }

  // Callback function for cumulative Ack Timer
  static void* cumulativeAckTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*) arg;
    while(tempMsgSocket->cumulativeAckTimer.isRunning)
    {
      usleep(tempMsgSocket->cumulativeAckTimer.interval * 1000);
      if(tempMsgSocket->cumulativeAckTimer.status == TIMER_RUN)
      {
        tempMsgSocket->handleCumulativeAckTimerFunc();
      }
    }
    return NULL;
  }

  // Handle Retransmission function
  void MsgReliableSocket::handleRetransmissionTimerFunc(void)
  {
    unackedSentQueueLock.lock();
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin(); iter != unackedSentQueue.end(); iter++)
    {
      ReliableFragment &frag = *iter;
      try
      {
        retransmitFragment(&frag);
      }
      catch(...)
      {
        // Handle Exception
      }
    }
    unackedSentQueueLock.unlock();
  }

  // Handle Null Fragment function
  void MsgReliableSocket::handleNullFragmentTimerFunc(void)
  {
    if(unackedSentQueue.empty())
    {
      try
      {
        // Send a new NULL segment if there is nothing to be retransmitted.
        logTrace("MSG", "REL", "send NULL fragment");
        queueAndSendReliableFragment(new NULLFragment(rcvListInfo.nextFragmentId()));
      }
      catch(...)
      {
        // Hanlde Exception
      }
    }
  }

  // Handle Cumulative Ack function
  void MsgReliableSocket::handleCumulativeAckTimerFunc(void)
  {
    this->sendAcknowledged();
  }

  // Compare two fragment
  int compareFragment(int frag1, int frag2)
  {
    if(frag1 == frag2)
    {
      return 0;
    }
    else if(((frag1 < frag2) && ((frag2 - frag1) > MAX_FRAGMENT_NUMBER / 2)) || ((frag1 > frag2) && ((frag1 - frag2) < MAX_FRAGMENT_NUMBER / 2)))
    {
      return 1;
    }
    else
    {
      return -1;
    }
  }

  void MsgReliableSocket::handleReliableFragment(ReliableFragment *frag)
  {
    fragmentType fragType = frag->getType();
    if(fragType == fragmentType::FRAG_RST)
    {
      resetMutex.lock();
      isReset = true;
      resetMutex.unlock();
      //setconnection(connectionNotification::RESET);
    }

    /*
     * When a FIN segment is received, no more packets
     * are expected to arrive after this segment.
     */
    if(fragType == fragmentType::FRAG_FIN)
    {
      switch(state)
      {
        case connectionState::CONN_SYN_SENT:
        {
          thisCond.notify_one();
          break;
        }
        case connectionState::CONN_CLOSED:
        {
          break;
        }
        default:
        {
          state = connectionState::CONN_CLOSED;
          logTrace("MSG", "REL", "Set connection state to CLOSED");
          return;
        }
      }
    }
    bool bIsInSequence = false;
    recvQueueLock.lock();
    if(compareFragment(frag->getFragmentId(), rcvListInfo.getLastInSequence()) <= 0)
    {
      /* Drop packet: duplicate. */
      logTrace("MSG", "REL", "Fragment id [%d]. Drop duplicate", frag->getFragmentId());
      delete frag;
      recvQueueLock.unlock();
      return;
    }
    else if(compareFragment(frag->getFragmentId(), nextSequenceNumber(rcvListInfo.getLastInSequence())) == 0)
    {
      //fragment is the next fragment in queue. Add to in-seq queue
      bIsInSequence = true;
      if(inSeqQueue.size() == 0 || (inSeqQueue.size() + outOfSeqQueue.size() < recvQueueSize))
      {
        /* Insert in-sequence segment */
        rcvListInfo.setLastInSequence(frag->getFragmentId());
        if(fragType == fragmentType::FRAG_DAT || fragType == fragmentType::FRAG_RST || fragType == fragmentType::FRAG_FIN)
        {
          logTrace( "MSG", "REL",
              "Add fragment [%d] to in sequence queue. Last in sequence list is [%d]", frag->getFragmentId(), rcvListInfo.getLastInSequence());
          inSeqQueue.push_back(*frag);
        }
        checkRecvQueues();
      }
      else
      {
        /* Drop packet: queue is full. */
        logTrace("MSG", "REL", "Drop packet: queue is full");

      }
    }
    else if(inSeqQueue.size() + outOfSeqQueue.size() < recvQueueSize)
    {
      //Fragment is not the next fragment in queue. Add it to out of seq queue
      bool added = false;
      for(ReliableFragmentList::iterator iter = outOfSeqQueue.begin(); iter != outOfSeqQueue.end() && !added; iter++)
      {
        ReliableFragment &s = *iter;
        int cmp = compareFragment(frag->getFragmentId(), s.getFragmentId());
        if(cmp == 0)
        {
          /* Ignore duplicate packet */
          logTrace("MSG", "REL", "Drop packet: duplicate in out-of-sequence");
          added = true;
          delete frag;
        }
        else if(cmp < 0)
        {
          logTrace("MSG", "REL", "Add fragment [%d] to out of sequence queue. ", frag->getFragmentId());
          outOfSeqQueue.insert(iter, *frag);
          rcvListInfo.increaseOutOfSequenceFragment();
          added = true;
          recvQueueLock.unlock();
          return;
        }
      }

      if(!added)
      {
        //logTrace("MSG","REL", "Data Fragment [%d] from [%d - %d]. Add to out of seq queue",frag->getFragmentId(),destination.getNode(),destination.getPort());
        outOfSeqQueue.push_back(*frag);
        rcvListInfo.increaseOutOfSequenceFragment();
      }
      //inscreate out of sequence fragment
    }
    if(bIsInSequence && (fragType == fragmentType::FRAG_RST || fragType == fragmentType::FRAG_NUL || fragType == fragmentType::FRAG_FIN))
    {
      //logTrace("MSG","REL", "Send Ack for RST/NUL/FIN fragment");
      sendAcknowledged();
    }
    else if((rcvListInfo.getOutOfSequenceFragment() > 0)
        && (profile->maxOutOfSequence() == 0 || rcvListInfo.getOutOfSequenceFragment() > profile->maxOutOfSequence()))
    {
      //logTrace("MSG","REL", "OutOfSequence queue not empty [%d]. Send NAK fragment",rcvListInfo.getOutOfSequenceFragment());
      sendNAK();
    }
    else if((rcvListInfo.getCumulativeAckCounter() > 0)
        && (profile->maxCumulativeAcks() == 0 || rcvListInfo.getCumulativeAckCounter() > profile->maxCumulativeAcks()))
    {
      sendAck();
    }
    else
    {
      if(cumulativeAckTimer.status == TIMER_PAUSE)
      {
        cumulativeAckTimer.interval = profile->getCumulativeAckInterval();
        cumulativeAckTimer.status = TIMER_RUN;
      }
      cumulativeAckTimer.interval = profile->getCumulativeAckInterval();
      if(!cumulativeAckTimer.started)
      {
        boost::thread(cumulativeAckTimerFunc, this);
        cumulativeAckTimer.started = true;
        cumulativeAckTimer.isRunning = true;
      }
    }
    recvQueueLock.unlock();
    ReliableFragmentList::iterator it1 = inSeqQueue.end();
    --it1;
    ReliableFragment &s1 = *it1;
    if(s1.isLastFragment())
    {
      sendAck();
      recvQueueCond.notify_all();
    }
  }

  void MsgReliableSocket::checkRecvQueues()
  {
    int count = 0;
    ReliableFragmentList::iterator it = outOfSeqQueue.begin();
    //    bool haveLastFrag=false;
    while(it != outOfSeqQueue.end())
    {
      ReliableFragment &s = *it;
      if(compareFragment(s.getFragmentId(), nextSequenceNumber(rcvListInfo.getLastInSequence())) == 0)
      {
        rcvListInfo.setLastInSequence(s.getFragmentId());
        it = outOfSeqQueue.erase(it);
        if(s.getType() == fragmentType::FRAG_DAT || s.getType() == fragmentType::FRAG_RST || s.getType() == fragmentType::FRAG_FIN)
        {
          //logTrace("MSG", "REL", "Move fragment [%d] to in sequence list ", s.getFragmentId());
          inSeqQueue.push_back(s);
          count++;
        }
      }
      else
      {
        it++;
      }
    }
    if(count > 0)
    {
      sendAck();
    }
  }
  void MsgReliableSocket::handleSYNReliableFragment(SYNFragment *frag)
  {
    try
    {
      //logTrace("MSG", "REL","Handle SYNC fragment");
      switch(state)
      {
        case connectionState::CONN_CLOSED:
        {
          rcvListInfo.setLastInSequence(frag->getFragmentId());
          state = connectionState::CONN_SYN_RCVD;
          logTrace("MSG","RST",
              "Reliable Socket([%d]-[%d]) : State CONN_CLOSED. set state to CONN_SYN_RCVD. Send sync frag to [%d] - [%d]", xport->handle().getNode(), xport->handle().getPort(), destination.getNode(), destination.getPort());
          logTrace("MSG","RST",
              "Socket PROFILE [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]", sendQueueSize, recvQueueSize, frag->getMaxFragmentSize(), frag->getMaxFragment(), frag->getMaxRetrans(), frag->getMaxCumulativeAcks(), frag->getMaxOutOfSequence(), frag->getMaxAutoReset(), frag->getNulFragmentInterval(), frag->getRetransmissionIntervel(), frag->getCummulativeAckInterval());
          profile = new ReliableSocketProfile(sendQueueSize, recvQueueSize, frag->getMaxFragmentSize(), frag->getMaxFragment(),
              frag->getMaxRetrans(), frag->getMaxCumulativeAcks(), frag->getMaxOutOfSequence(), frag->getMaxAutoReset(), frag->getNulFragmentInterval(),
              frag->getRetransmissionIntervel(), frag->getCummulativeAckInterval());
          SYNFragment* synFrag = new SYNFragment(rcvListInfo.setFragmentId(0), profile->maxNAKFrags(), profile->maxFragmentSize(),
              profile->getRetransmissionInterval(), profile->getCumulativeAckInterval(), profile->getNullFragmentInterval(), profile->maxRetrans(),
              profile->maxCumulativeAcks(), profile->maxOutOfSequence(), profile->maxAutoReset());
          rcvListInfo.reset();
          if(!inSeqQueue.empty())
          {
            inSeqQueue.clear_and_dispose(delete_disposer());
          }
          if(!outOfSeqQueue.empty())
          {
            outOfSeqQueue.clear_and_dispose(delete_disposer());
          }
          synFrag->isFirst = false; // sync fragment from receiver
          //logTrace("MSG", "RST","Socket([%d]-[%d]) : Set Ack. Send sync frag to [%d] - [%d]",xPort->handle().getNode(),xPort->handle().getPort(), destination.getNode(),destination.getPort());
          synFrag->setAck(frag->getFragmentId());
          queueAndSendReliableFragment(synFrag);
          break;
        }
        case connectionState::CONN_SYN_SENT:
        {
          logTrace("MSG","RST","Reliable socket ([%d]-[%d]) current connection state : CONN_CLOSED. Set connection to established", xport->handle().getNode(), xport->handle().getPort());
          rcvListInfo.setLastInSequence(frag->getFragmentId());
          state = connectionState::CONN_ESTABLISHED;
          /*
           * Here the client accepts or rejects the parameters sent by the
           * server. For now we will accept them.
           */
          this->setconnection(connectionNotification::OPENED);
          this->sendAcknowledged(true);
          break;
        }
        default:
        {
          logTrace("MSG", "RST", "Connection state : Error");
          break;
        }
      }
    }
    catch(...)
    {
      // TODO EXCEPTION
    }
  }
  void MsgReliableSocket::handleACKReliableFragment(ReliableFragment *frag)
  {

  }
  void MsgReliableSocket::handleNAKReliableFragment(NAKFragment *frag)
  {
    int length = 0;
    bool bFound = false;
    int* naks = frag->getNAKs(&length);
    int lastInSequence = frag->getAck();
    int lastOutSequence = naks[length - 1];
    unackedSentQueueLock.lock();
    /* Removed acknowledged fragments from unackedSent queue */
    ReliableFragmentList::iterator it = unackedSentQueue.begin();
    while(it != unackedSentQueue.end())
    {
      ReliableFragment& s = *it;
      if((compareFragment(s.getFragmentId(), lastInSequence) <= 0))
      {
        it = unackedSentQueue.erase_and_dispose(it, delete_disposer());
        continue;
      }
      bFound = false;
      for(int i = 0; i < length; i++)
      {
        if((compareFragment(s.getFragmentId(), naks[i]) == 0))
        {
          it = unackedSentQueue.erase_and_dispose(it, delete_disposer());
          bFound = true;
          break;
        }
      }
      if(bFound == false)
      {
        it++;
      }
    }

    /* Retransmit segments */
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin(); iter != unackedSentQueue.end(); iter++)
    {
      ReliableFragment& s = *iter;
      //logTrace("MSG", "RST","Check fragment [%d] %d %d ",s.getFragmentId(),compareFragment(lastInSequence, s.getFragmentId()),compareFragment(lastOutSequence, s.getFragmentId()));
      if((compareFragment(lastInSequence, s.getFragmentId()) < 0) && (compareFragment(lastOutSequence, s.getFragmentId()) > 0))
      {
        try
        {
          retransmitFragment(&s);
        }
        catch(...)
        {
          // TODO
        }
      }
    }
    free(naks);
    unackedSentQueueCond.notify_all();
    unackedSentQueueLock.unlock();
  }

  void MsgReliableSocket::sendAcknowledged(bool isFirst)
  {
    sendAck(isFirst);
  }

  void MsgReliableSocket::sendNAK()
  {
    if(outOfSeqQueue.empty())
    {
      return;
    }
    rcvListInfo.resetCumulativeAckFragment();
    rcvListInfo.resetOutOfSequenceFragment();

    /* Compose list of out-of-sequence sequence numbers */
    int size = outOfSeqQueue.size();
    int *acks = (int*) malloc(size * sizeof(int));
    int nIdx = 0;
    for(ReliableFragmentList::iterator it = outOfSeqQueue.begin(); it != outOfSeqQueue.end(); it++)
    {
      ReliableFragment& s = *it;
      acks[nIdx++] = s.getFragmentId();
    }
    try
    {
      int lastInSequence = rcvListInfo.getLastInSequence();
      //logTrace( "MSG", "RST", "Send NAK fragment to Node [%d] with size [%d]", destination.getNode(), size);
      NAKFragment* frag = new NAKFragment(nextSequenceNumber(lastInSequence), lastInSequence, acks, size);
      this->sendReliableFragment(frag);
      free(acks);
      delete frag;
    }
    catch(...)
    {
      // Handle Exception
    }

  }

  void MsgReliableSocket::sendAck(bool isFirst)
  {
    if(rcvListInfo.resetCumulativeAckFragment() == 0)
    {
      //logTrace("MSG", "RST","CumulativeAck = 0");
      return;
    }
    try
    {
      int lastInSequence = rcvListInfo.getLastInSequence();
      //logTrace( "MSG", "RST", "Send  ACK [%d] to Node [%d] port [%d]", lastInSequence, destination.getNode(), destination.getPort());
      ReliableFragment *fragment = new ACKFragment(nextSequenceNumber(lastInSequence), lastInSequence);
      fragment->isFirst = isFirst;
      this->sendReliableFragment(fragment);
      delete fragment;
    }
    catch(...)
    {
      // Handle Exception
    }
  }

  int MsgReliableSocket::nextSequenceNumber(int seqn)
  {
    return (seqn + 1) % MAX_FRAGMENT_NUMBER;
  }

  void MsgReliableSocket::sendSYN()
  {
    //TODO
  }
  void MsgReliableSocket::setACK(ReliableFragment *frag)
  {
    if(rcvListInfo.resetCumulativeAckFragment() == 0)
    {
      return;
    }
    frag->setAck(rcvListInfo.getLastInSequence());
  }

  void MsgReliableSocket::getACK(ReliableFragment *frag)
  {
    int ackn = frag->getAck();
    if(ackn < 0)
    {
      //logTrace("MSG","RST","Ack < 0. Return ");
      return;
    }
    rcvListInfo.resetNAKFragment();
    if(state == connectionState::CONN_SYN_RCVD)
    {
      logTrace("MSG", "RST", "Current state CONN_SYN_RCVD. set to CONN_ESTABLISHED");
      state = connectionState::CONN_ESTABLISHED;
      setconnection(connectionNotification::OPENED);
    }
    unackedSentQueueLock.lock();
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin(); iter != unackedSentQueue.end();)
    {
      ReliableFragment &fragment = *iter;
      if(compareFragment(fragment.getFragmentId(), ackn) <= 0)
      {
        //logTrace("MSG","RST","Socket([%d - %d]) : UnackedSentQueue delete Fragment [%d]",xPort->handle().getNode(),xPort->handle().getPort(),fragment.getFragmentId());
        iter = unackedSentQueue.erase_and_dispose(iter, delete_disposer());
      }
      else
      {
        iter++;
      }
    }
    if(unackedSentQueue.empty())
    {
      if(retransmissionTimer.status == TIMER_RUN)
      {
        //        logTrace("MSG", "RST",
        //            "unackedSentQueue is empty. Pause retransmissionTimer timer ");
        //          retransmissionTimer.status = TIMER_PAUSE;
      }
    }
    unackedSentQueueCond.notify_all();
    unackedSentQueueLock.unlock();
  }

  //send a reliable fragment
  void MsgReliableSocket::sendReliableFragment(ReliableFragment *frag)
  {
    /* Piggyback any pending acknowledgments */
    if(frag->getType() == fragmentType::FRAG_DAT || frag->getType() == fragmentType::FRAG_RST || frag->getType() == fragmentType::FRAG_FIN
        || frag->getType() == fragmentType::FRAG_NUL)
    {
      setACK(frag);
    }
    /* Reset null segment timer */
    if(frag->getType() == fragmentType::FRAG_DAT || frag->getType() == fragmentType::FRAG_RST || frag->getType() == fragmentType::FRAG_FIN)
    {
      nullFragmentTimer.status = TimerStatus::TIMER_PAUSE;
      nullFragmentTimer.status = TimerStatus::TIMER_RUN;
    }
    //send reliable fragment
    if(xport == NULL)
    {
      return;
    }
    if(msgPool == NULL)
    {
      return;
    }
    bool isDelete = false;
    if(frag->getType() == fragmentType::FRAG_SYN || frag->getType() == fragmentType::FRAG_NAK)
    {
      isDelete = true;
    }
    Byte* data;
    Message *message = msgPool->allocMsg();
    message->setAddress(this->destination);
    MsgFragment* hdr = message->append(0);
    Byte* buffer = frag->getHeader();
    hdr->set(buffer, RUDP_HEADER_LEN);
    if(frag->getlength() > 0)
    {
      data = frag->getData();
      MsgFragment* splitFrag = message->append(0);
      splitFrag->set(data, frag->getlength());
    }
    xport->send(message);
    if(isDelete)
    {
      free(data);
    }
    free(buffer);
  }

  // send fragment and store it in uncheck ack
  void MsgReliableSocket::queueAndSendReliableFragment(ReliableFragment* frag)
  {
    unackedSentQueueLock.lock();
    int unackedSentTimeOut = 5;
    while((unackedSentQueue.size() >= sendQueueSize) || (rcvListInfo.getNAKFragment() > profile->maxNAKFrags()))
    {
      try
      {
        unackedSentQueueCond.timed_wait(unackedSentQueueLock, unackedSentTimeOut);
      }
      catch(...)
      {
        // Hanlde Exception
      }
    }
    rcvListInfo.increaseNAKFragment();
    logTrace("MSG", "RST", "Store Fragment [%d] to unackedSent Queue", frag->getFragmentId());
    unackedSentQueue.push_back(*frag);
    unackedSentQueueLock.unlock();
    if(isClosed)
    {
      logTrace("MSG", "RST", "status is closed . Return");
      return;
    }
    /* Re-start retransmission timer */
    if(!(frag->getType() == fragmentType::FRAG_NAK) && !(frag->getType() == fragmentType::FRAG_ACK))
    {
      if(retransmissionTimer.status == TIMER_PAUSE)
      {
        logTrace( "MSG", "RST", "Start retransmission timer [%d] interval [%d]", retransmissionTimer.status, retransmissionTimer.interval);
        retransmissionTimer.interval = profile->getRetransmissionInterval();
        retransmissionTimer.status = TIMER_RUN;
        if(!retransmissionTimer.started)
        {
          boost::thread(retransmissionTimerFunc, this);
          retransmissionTimer.status = TIMER_RUN;
          retransmissionTimer.isRunning = true;
          retransmissionTimer.started = true;
        }
      }
    }
    sendReliableFragment(frag);
  }

  Message* MsgReliableSocket::receiveOrigin(uint_t maxMsgs, int maxDelay)
  {
    logTrace( "MSG", "RST", "receive buffer from [%d - %d] in reliable mode ", destination.getNode(), destination.getPort());
    int totalBytes = 0;
    bool quit = false;
    Handle address;
    if(isClosed)
    {
      throw Error("Socket is closed");
    }
    if(!isConnected)
    {
      throw Error("Connection reset");
    }
    if(timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {
      recvQueueLock.lock();
      recvQueueCond.timed_wait(recvQueueLock, timeout);
    }
    int maxFragmentSize = profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m = msgPool->allocMsg();
    assert(m);
    MsgFragment *temp;
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      //      logTrace("MSG","RST","Read the fragment id [%d] with length [%d]",s.getFragmentId(),s.getlength());
      if(s.getType() == fragmentType::FRAG_RST)
      {
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_FIN)
      {
        if(totalBytes <= 0)
        {
          it = inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_DAT)
      {
        if(s.isLastFragment())
        {
          quit = true;
          m->setAddress(s.address);
          //logTrace("MSG","RST","read the last fragment [%d]",s.getFragmentId());
        }
        int length = 0;
        Byte* data = s.getData();
        length = s.getlength();
        if(s.isFirst == true)
        {
          totalBytes = 0;
          temp = m->append(0);
          buffer = (Byte*) SAFplusHeapAlloc(length);
          temp->set((void*) buffer, length);
        }
        else
        {
          Byte* newBuffer = (Byte*) SAFplusHeapRealloc(buffer,totalBytes + length);
          buffer = newBuffer;
          temp->len += ((DATFragment*) &s)->getlength();
        }
        memcpy(buffer + (totalBytes), (void*) data, length);
        totalBytes += length;
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        if(quit == true)
        {
          break;
        }
      }
      else
      {
        it++;
      }
    }
    if(totalBytes > 0)
    {
      recvQueueLock.unlock();
      return m;
    }
    recvQueueLock.unlock();
    return NULL;
  }

  Message* MsgReliableSocket::receiveFast(uint_t maxMsgs, int maxDelay)
  {
    logTrace( "MSG", "RST", "receive buffer from [%d - %d] in reliable mode ", destination.getNode(), destination.getPort());
    int totalBytes = 0;
    bool quit = false;
    Handle address;
    if(isClosed)
    {
      throw Error("Socket is closed");
    }
    if(!isConnected)
    {
      throw Error("Connection reset");
    }
    if(timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {
      recvQueueLock.lock();
      recvQueueCond.timed_wait(recvQueueLock, timeout);
    }
    int maxFragmentSize = profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m = NULL;
    MsgFragment *temp;
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      if(s.getType() == fragmentType::FRAG_RST)
      {
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_FIN)
      {
        if(totalBytes <= 0)
        {
          it = inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_DAT)
      {
        if(s.isLastFragment())
        {
          quit = true;
          m->setAddress(s.address);
          logTrace("MSG", "RST", "read the last fragment [%d]", s.getFragmentId());
        }
        s.message->lastFragment->start += RUDP_HEADER_LEN;
        s.message->lastFragment->len -= RUDP_HEADER_LEN;
        if(m == NULL)
        {
          m = s.message;
          logTrace("MSG", "RST", "read first last fragment [%d]", s.message->lastFragment->len);
        }
        else
        {
          logTrace("MSG", "RST", "append  last fragment [%d]", s.message->lastFragment->len);
          m->lastFragment->nextFragment = s.message->lastFragment;
          m->lastFragment = s.message->lastFragment;
          Message* old;
          old = s.message;
          old->nextMsg = NULL; // clear these out so they don't get cleaned up
          old->firstFragment = NULL;
          old->lastFragment = NULL;
          msgPool->free(old);
        }
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        logTrace("MSG", "RST", "debug the last fragment len [%d]", m->lastFragment->len);
        if(quit == true)
        {
          break;
        }

      }
      else
      {
        it++;
      }
    }
    recvQueueLock.unlock();
    return m;
  }

  Message* MsgReliableSocket::receive(uint_t maxMsgs, int maxDelay)
  {
    int totalBytes = 0;
    bool quit = false;
    Handle address;
    if(isClosed)
    {
      throw Error("Socket is closed");
    }
    if(!isConnected)
    {
      throw Error("Connection reset");
    }
    if(timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {

      recvQueueLock.lock();
      if(!recvQueueCond.timed_wait(recvQueueLock, timeout))
      {
        recvQueueLock.unlock();
        return NULL;
      }
    }
    int maxFragmentSize = profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer = NULL;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m = msgPool->allocMsg();
    assert(m);
    MsgFragment *temp;
    temp = m->append(0);
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      if(s.getType() == fragmentType::FRAG_RST)
      {
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_FIN)
      {
        if(totalBytes <= 0)
        {
          it = inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if(s.getType() == fragmentType::FRAG_DAT)
      {
        if(s.isLastFragment())
        {
          quit = true;
          m->setAddress(s.address);
        }
        int length = 0;
        Byte* data = s.getData();
        length = s.getlength();
        Byte* newBuffer;
        if(buffer == NULL)
        {
          buffer = (Byte*) SAFplusHeapAlloc(length);
        }
        else
        {
          newBuffer = (Byte*) SAFplusHeapRealloc(buffer,totalBytes + length);
          buffer = newBuffer;
        }
        temp->len += ((DATFragment*) &s)->getlength();
        memcpy(buffer + (totalBytes), (void*) data, length);
        totalBytes += length;
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        if(quit == true)
        {
          break;
        }
      }
      else
      {
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
    }
    if(totalBytes > 0)
    {
      temp->set((void*) buffer, totalBytes);
      recvQueueLock.unlock();
      //add to message queue
      std::vector<Message*>::iterator it = this->sockServer->msgs.begin();
      sockServer->msgs.insert(it, m);
      sockServer->readMsgLock.lock();
      sockServer->readMsgCond.notify_one();
      sockServer->readMsgLock.unlock();
      return m;
    }
    recvQueueLock.unlock();
    return NULL;
  }

  ReliableFragment* MsgReliableSocket::receiveReliableFragment(Handle &handle)
  {
    ReliableFragment* p_Fragment;
    Message* p_Msg = NULL;
    MsgFragment* p_NextFrag = NULL;
    MsgFragment* p_DataFrag = NULL;

    int iMaxMsg = 1;
    int iDelay = -1;

    p_Msg = this->xport->receive(iMaxMsg, iDelay);
    if(p_Msg == NULL)
    {
      return NULL;
    }
    handle = p_Msg->getAddress();
    p_NextFrag = p_Msg->firstFragment;
    if(p_NextFrag == NULL)
    {
      return NULL;
    }
    p_Fragment = ReliableFragment::parse((Byte*) p_NextFrag->read(0), p_NextFrag->len);
    p_Fragment->address = p_Msg->getAddress();
    fragmentType fragType = p_Fragment->getType();
    if(fragType == fragmentType::FRAG_DAT || fragType == fragmentType::FRAG_NUL || fragType == fragmentType::FRAG_RST || fragType == fragmentType::FRAG_FIN
        || fragType == fragmentType::FRAG_SYN)
    {
      rcvListInfo.increaseCumulativeAckCounter();
    }
    if(p_NextFrag->len > RUDP_HEADER_LEN)
    {
      p_Fragment->parseData((Byte*) p_NextFrag->read(RUDP_HEADER_LEN), p_NextFrag->len - RUDP_HEADER_LEN);
    }
    p_Msg->msgPool->free(p_Msg);
    return p_Fragment;
  }

  void MsgReliableSocket::retransmitFragment(ReliableFragment *frag)
  {
    logTrace("MSG", "RST", "Retransmit Fragment fradId [%d] ", frag->getFragmentId());
    if(profile->maxRetrans() > 0)
    {
      frag->setRetxCounter(frag->getRetxCounter() + 1);
    }
    if(profile->maxRetrans() != 0 && frag->getRetxCounter() > profile->maxRetrans())
    {
      setconnection(connectionNotification::FAILURE);
      return;
    }assert(xport);
    Byte* data;
    Message *message = msgPool->allocMsg();
    message->setAddress(this->destination);
    MsgFragment* hdr = message->append(0);
    Byte* buffer = frag->getHeader();
    hdr->set(buffer, RUDP_HEADER_LEN);
    if(frag->getlength() > 0)
    {
      MsgFragment* splitFrag = message->append(0);
      data = frag->getData();
      splitFrag->set(data, frag->getlength());
    }
    xport->send(message);
    free(buffer);
    if(frag->getType() == fragmentType::FRAG_SYN || frag->getType() == fragmentType::FRAG_NAK)
    {
      free(data);
    }
  }

  void MsgReliableSocket::connectionFailure()
  {
    logTrace( "MSG", "RST", "Socket([%d - %d]) : Set connection to Failure", xport->handle().getNode(), xport->handle().getPort());
    closeMutex.lock();
    {
      if(isClosed)
      {
        closeMutex.unlock();
        return;
      }
      switch(state)
      {
        case connectionState::CONN_SYN_SENT:
        {
          thisMutex.lock();
          {
            thisCond.notify_one();
          }
          thisMutex.unlock();
          break;
        }
        case connectionState::CONN_SYN_RCVD:
        case connectionState::CONN_ESTABLISHED:
        {
          isConnected = false;
          unackedSentQueueLock.lock();
          {
            unackedSentQueueCond.notify_all();
          }
          unackedSentQueueLock.unlock();
          //closeConnection();
          break;
        }
      }
      state = connectionState::CONN_CLOSED;
      isClosed = true;
    }
    closeMutex.unlock();
  }

  void MsgReliableSocket::closeConnection()
  {
    nullFragmentTimer.isRunning = false;
    retransmissionTimer.isRunning = false;
    cumulativeAckTimer.isRunning = false;
    //notify SocketServer to remove this connection.
    this->unackedSentQueue.clear_and_dispose(delete_disposer());
    this->inSeqQueue.clear_and_dispose(delete_disposer());
    this->outOfSeqQueue.clear_and_dispose(delete_disposer());
  }
  void MsgReliableSocket::setconnection(connectionNotification state)
  {
    switch(state)
    {
      case connectionNotification::CLOSE:
        break;
      case connectionNotification::FAILURE:
      {
        connectionFailure();
        break;
      }
      case connectionNotification::OPENED:
      {
        connectionOpened();
        break;
      }
      case connectionNotification::REFUSED:
        break;
      case connectionNotification::RESET:
        break;
    }
  }

  void MsgReliableSocket::connectionOpened(void)
  {
    logTrace( "MSG", "RST", "Socket([%d - %d]) : set connection to Opened", xport->handle().getNode(), xport->handle().getPort());
    if(isConnected)
    {
      nullFragmentTimer.status = TIMER_RUN;
      nullFragmentTimer.interval = profile->getNullFragmentInterval();
      boost::thread(nullFragmentTimerFunc, this);
      resetMutex.lock();
      {
        isReset = false;
        resetCond.notify_one();
      }
      resetMutex.unlock();
    }
    else
    {
      thisMutex.lock();
      {
        isConnected = true;
        state = connectionState::CONN_ESTABLISHED;
      }
      thisCond.notify_one();
      thisMutex.unlock();
    }
  }
  static void rcvFragmentThreadFunc(void * arg)
  {
    MsgReliableSocket* p_this = (MsgReliableSocket*) arg;
    p_this->handleReceiveFragmentThread();
  }

  MsgReliableSocket::MsgReliableSocket(uint_t port, MsgTransportPlugin_1* transport)
  {
    xport = transport->createSocket(port);
    msgPool = xport->getMsgPool();
    node = xport->node;
    port = xport->port;
    init();
  }
  MsgReliableSocket::MsgReliableSocket(uint_t port, MsgTransportPlugin_1* transport, Handle destination)
  {
    xport = transport->createSocket(port);
    msgPool = xport->getMsgPool();
    node = xport->node;
    port = xport->port;
    init();
    this->destination = destination;
  }
  MsgReliableSocket::MsgReliableSocket(MsgSocket* socket)
  {
    xport = socket;
    node = xport->node;
    port = xport->port;
    msgPool = xport->getMsgPool();
    init();
  }
  void MsgReliableSocket::init()
  {
    sockServer = NULL;
    rcvListInfo.fragNumber = 0;
    rcvListInfo.lastFrag = 0;
    rcvListInfo.numberOfCumAck = 0;
    rcvListInfo.numberOfOutOfSeq = 0;
    rcvListInfo.numberOfNakFrag = 0; /* Outstanding segments counter */
    nullFragmentTimer.status = TIMER_PAUSE;
    retransmissionTimer.status = TIMER_PAUSE;
    cumulativeAckTimer.status = TIMER_PAUSE;

    nullFragmentTimer.started = false;
    retransmissionTimer.started = false;
    cumulativeAckTimer.started = false;

    nullFragmentTimer.interval = 0;
    retransmissionTimer.interval = 0;
    cumulativeAckTimer.interval = 0;

    nullFragmentTimer.isRunning = false;
    cumulativeAckTimer.isRunning = false;
    retransmissionTimer.isRunning = false;
    lastFragmentIdOfMessage = 0;
    profile = new ReliableSocketProfile();
    rcvFragmentThreadRunning = true;
    rcvFragmentThread = boost::thread(rcvFragmentThreadFunc, this);
  }

  void MsgReliableSocket::resetSocket()
  {
    rcvListInfo.fragNumber = 0;
    rcvListInfo.lastFrag = 0;
    rcvListInfo.numberOfCumAck = 0;
    rcvListInfo.numberOfOutOfSeq = 0;
    rcvListInfo.numberOfNakFrag = 0; /* Outstanding segments counter */
    nullFragmentTimer.status = TIMER_PAUSE;
    retransmissionTimer.status = TIMER_PAUSE;
    cumulativeAckTimer.status = TIMER_PAUSE;
    nullFragmentTimer.started = false;
    retransmissionTimer.started = false;
    cumulativeAckTimer.started = false;
    nullFragmentTimer.interval = 0;
    retransmissionTimer.interval = 0;
    cumulativeAckTimer.interval = 0;
    nullFragmentTimer.isRunning = false;
    cumulativeAckTimer.isRunning = false;
    retransmissionTimer.isRunning = false;
    state = connectionState::CONN_CLOSED;
    lastFragmentIdOfMessage = 0;
    delete profile;
    if(!inSeqQueue.empty())
    {
      inSeqQueue.clear_and_dispose(delete_disposer());
    }
    if(!outOfSeqQueue.empty())
    {
      outOfSeqQueue.clear_and_dispose(delete_disposer());
    }
  }

  void MsgReliableSocket::handleReceiveFragmentThread(void)
  {
    fragmentType fragType = fragmentType::FRAG_UDE;
    Handle handle;
    logTrace( "MSG", "RST", " Reliable Socket([%d - %d]) : receiver fragment thread started .", xport->handle().getNode(), xport->handle().getPort());
#ifdef TEST_RELIABLE
    int test =0;
#endif
    while(rcvFragmentThreadRunning)
    {
      ReliableFragment* pFrag = receiveReliableFragment(handle);
      if(pFrag == NULL)
      {
        continue;
      }
      fragType = pFrag->getType();
      if(fragType == fragmentType::FRAG_SYN)
      {
        this->destination = handle;
        handleSYNReliableFragment((SYNFragment*) pFrag);
      }
      else if(fragType == fragmentType::FRAG_NAK)
      {
        handleNAKReliableFragment((NAKFragment*) pFrag);
      }
      else if(fragType == fragmentType::FRAG_ACK)
      {
        //        logTrace("MSG", "RST", "Receive ACK fragment [%d]", pFrag->getAck());
        if(pFrag->getAck() == lastFragmentIdOfMessage)
        {
          sendReliableLock.lock();
          sendReliableCond.notify_one();
          sendReliableLock.unlock();
        }
      }
      else
      {
#ifdef TEST_RELIABLE
        //         For testing ............
        test++;
        if(test % 5 == 0)
        {
          logTrace("MSG","RST","Remove fragment [%d] for testing",pFrag->getFragmentId());
          delete pFrag;
          continue;
        }
        else
        {
          handleReliableFragment(pFrag);
        }
#else
        handleReliableFragment(pFrag);
#endif
      }
      getACK(pFrag);
      if(fragType == fragmentType::FRAG_ACK || fragType == fragmentType::FRAG_SYN || fragType == fragmentType::FRAG_NAK)
      {
        delete pFrag;
      }
    }
    logTrace( "MSG", "RST", " Reliable Socket([%d - %d]) : receiver fragment thread stopped.", xport->handle().getNode(), xport->handle().getPort());
  }

  MsgReliableSocket::~MsgReliableSocket()
  {
    nullFragmentTimer.isRunning = false;
    retransmissionTimer.isRunning = false;
    cumulativeAckTimer.isRunning = false;
    //wait for timer stop
    usleep(retransmissionTimer.interval*1000);
    rcvFragmentThread.join();
    if(xport)
      xport = NULL;
    unackedSentQueue.clear_and_dispose(delete_disposer());
    inSeqQueue.clear_and_dispose(delete_disposer());
    outOfSeqQueue.clear_and_dispose(delete_disposer());
    xport = NULL;
    sockServer = NULL;
  }

  void MsgReliableSocket::send(Message* origMsg)
  {

    Message* msg;
    Message* nextMsg = origMsg;
    int chunkCount = 0;
    Message* newMsg = NULL;
    connect(origMsg->getAddress(), 0);
    do
    {
      msg = nextMsg;
      nextMsg = msg->nextMsg;
      sendOneMsg(msg);
    }
    while(nextMsg != NULL);
  }

  bool MsgReliableSocket::sendOneMsg(Message* origMsg)
  {
    Message* nextMsg = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    nextFrag = origMsg->firstFragment;
    bool lastFrag = false;
    bool result=false;
    do
    {
      frag = nextFrag;
      nextFrag = frag->nextFragment;
      if(nextFrag == NULL)
      {
        lastFrag = true;
      }
      if(frag->len == MAXSIZE) // In this case we just need to move the fragment to a new message not split it.
      {
        ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) frag->data(0), 0, frag->len,
            lastFrag, true);
        queueAndSendReliableFragment(fragment);
      }
      else if(frag->len > MAXSIZE)
      {
        int totalBytes = 0;
        int writeBytes = 0;
        bool isFirst = true;
        while(totalBytes < frag->len)
        {
          writeBytes = MIN(MAXSIZE - RUDP_HEADER_LEN,frag->len - totalBytes);
          if(totalBytes + writeBytes < frag->len)
          {
            ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) frag->data(0), totalBytes,
                writeBytes, false, isFirst);
            queueAndSendReliableFragment(fragment);
            totalBytes += writeBytes;
          }
          else
          {
            queueAndSendReliableFragment(
                new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) frag->data(0), totalBytes, writeBytes, lastFrag,
                    isFirst));
            totalBytes += writeBytes;
          }
          isFirst = false;
        }
      }
      else
      {
        ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) frag->data(0), 0, frag->len,
            lastFrag, true);
        queueAndSendReliableFragment(fragment);
      }
    }
    while(nextFrag);
    lastFragmentIdOfMessage = rcvListInfo.getFragmentId();
    //    logTrace("MSG", "RST", "set last Fragment id  %d", lastFragmentIdOfMessage);
    sendReliableLock.lock();
    //wait until send successful or timeout
    result= sendReliableCond.timed_wait(sendReliableLock, MSG_SEND_TIMEOUT);
    sendReliableLock.unlock();
    if(result==false)
    {
      //clear all message in un ack ...
      unackedSentQueueLock.lock();
      {
        unackedSentQueue.clear();
        unackedSentQueueCond.notify_all();
      }
      unackedSentQueueLock.unlock();
    }
    return result;
  }

  void MsgReliableSocket::send(SAFplus::Handle destination, void* buffer, uint_t length, uint_t msgtype)
  {
    connect(destination, 0);
    if(isClosed)
    {
      throw new Error("Socket is closed");
    }
    if(!isConnected)
    {
      throw new Error("Connection reset");
    }

    int totalBytes = 0;
    int writeBytes = 0;
    while(totalBytes < length)
    {
      writeBytes = MIN(profile->maxFragmentSize() - RUDP_HEADER_LEN,length - totalBytes);
      logTrace("MSG", "RST", "sending [%d] byte", writeBytes);
      if(totalBytes + writeBytes < length)
      {
        ReliableFragment *frag = new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) buffer, totalBytes, writeBytes, msgPool,
            false);
        //logTrace("MSG","RST","Socket([%d - %d]) : Create fragment with seq [%d] type [%d] ack [%d].",xPort->handle().getNode(),xPort->handle().getPort(),frag->getFragmentId(),frag->getType(),frag->getAck());
        queueAndSendReliableFragment(frag);
        totalBytes += writeBytes;
      }
      else
      {
        logTrace( "MSG", "REL", "send last fragment to  node [%d] in reliable mode ", destination.getNode());
        queueAndSendReliableFragment(
            new DATFragment(rcvListInfo.nextFragmentId(), rcvListInfo.getLastInSequence(), (Byte*) buffer, totalBytes, writeBytes, msgPool, true));
        totalBytes += writeBytes;
      }
    }
  }

  void MsgReliableSocket::connect(Handle destination, int timeout)
  {
    logTrace(
        "REL",
        "MSR",
        "Reliable Socket([%d - %d]) : Connect to socket server node [%d] port [%d]", xport->handle().getNode(), xport->handle().getPort(), destination.getNode(), destination.getPort());
    if(timeout < 0)
    {
      throw Error("connect: timeout can't be negative");
    }
    if(isClosed)
    {
      throw Error("Socket is closed");
    }
    if(isConnected)
    {
      if(this->destination == destination)
      {
        return;
      }
      else
      {
        close();
      }
    }
    this->destination = destination;
    // Synchronize sequence numbers
    state = connectionState::CONN_SYN_SENT;
    if(!unackedSentQueue.empty())
    {
      unackedSentQueue.clear_and_dispose(delete_disposer());
    }
    int sequenceNum = rcvListInfo.setFragmentId(0);
    ReliableFragment *frag = new SYNFragment(sequenceNum, profile->maxNAKFrags(), profile->maxFragmentSize(), profile->getRetransmissionInterval(),
        profile->getCumulativeAckInterval(), profile->getNullFragmentInterval(), profile->maxRetrans(), profile->maxCumulativeAcks(),
        profile->maxOutOfSequence(), profile->maxAutoReset());
    frag->isFirst = true; //sync fragment from sender

    logTrace(
        "MSG",
        "RST",
        "Socket([%d - %d]) : Create syn fragment with folow parameter [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d].", xport->handle().getNode(), xport->handle().getPort(), sequenceNum, profile->maxNAKFrags(), profile->maxFragmentSize(), profile->getRetransmissionInterval(), profile->getCumulativeAckInterval(), profile->getNullFragmentInterval(), profile->maxRetrans(), profile->maxCumulativeAcks(), profile->maxOutOfSequence(), profile->maxAutoReset());
    queueAndSendReliableFragment(frag);
    // Wait for connection establishment (or timeout)
    bool timedout = false;
    thisMutex.lock();
    if(!isConnected)
    {
      try
      {
        if(timeout == 0)
        {
          thisCond.wait(thisMutex);
        }
      }
      catch(...)
      {
        // Hanle Exception
      }
    }
    thisMutex.unlock();
    if(state == connectionState::CONN_ESTABLISHED)
    {
      usleep(2000);
      return;
    }
    unackedSentQueueLock.lock();
    {
      unackedSentQueue.clear();
      unackedSentQueueCond.notify_all();
    }
    unackedSentQueueLock.unlock();
    rcvListInfo.reset();
    retransmissionTimer.status = TIMER_PAUSE;
    switch(state)
    {
      case connectionState::CONN_SYN_SENT:
      {
        setconnection(connectionNotification::REFUSED);
        state = connectionState::CONN_CLOSED;
        if(timedout)
        {
          throw new Error("SocketTimeoutException");
        }
        throw new Error("Connection refused");

      }
      case connectionState::CONN_CLOSED:
      {
        state = connectionState::CONN_CLOSED;
        throw Error("Socket closed");
      }
    }
  }

  void MsgReliableSocket::close()
  {
    closeMutex.lock();
    {
      if(isClosed)
      {
        closeMutex.unlock();
        return;
      }
      switch(state)
      {
        case connectionState::CONN_SYN_SENT:
        {
          thisMutex.lock();
          {
            thisCond.notify_one();
          }
          thisMutex.unlock();
          break;
        }
        case connectionState::CONN_SYN_RCVD:
        case connectionState::CONN_ESTABLISHED:
        {
          sendReliableFragment(new FINFragment(rcvListInfo.nextFragmentId()));
          closeConnection();
          break;
        }
        case connectionState::CONN_CLOSED:
        {
          retransmissionTimer.isRunning = false;
          cumulativeAckTimer.isRunning = false;
          nullFragmentTimer.isRunning = false;
          break;
        }
      }
      isClosed = true;
      state = connectionState::CONN_CLOSED;
      unackedSentQueueLock.lock();
      {
        unackedSentQueueCond.notify_one();
      }
      unackedSentQueueLock.unlock();
    }
    closeMutex.unlock();
  }
  void MsgReliableSocket::flush()
  {
    xport->flush();
  }
  void MsgReliableSocket::connectionClientOpen()
  {
  }
  ;

  void MsgReliableSocketServer::addClientSocket(Handle destAddress)
  {
    if(clientSockTable[destAddress] == NULL)
    {
      try
      {
        MsgReliableSocketClient *sockClient = new MsgReliableSocketClient(xport, destAddress);
        sockClient->sockServer = this;
        clientSockTable[destAddress] = sockClient;
        sockClient->startReceiveThread();
      }
      catch(...)
      {
        // Handle Exception
        throw Error("add client socket error");
      }
    }
  }

  void MsgReliableSocketServer::removeClientSocket(Handle destAddress)
  {
    // do nothing;
  }

  void MsgReliableSocketServer::close()
  {
    sendSock->close();
  }

  void MsgReliableSocketServer::handlereadMsgThread()
  {
    int iMaxMsg = 1;
    int iDelay = 2;
    Handle destinationAddress;
    logTrace( "MSG", "SST", "Socket server ([%d - %d]) : Read message thread started.", xport->handle().getNode(), xport->handle().getPort());
    while(readMsgThreadRunning)
    {
      try
      {
        Message* p_Msg = this->xport->receive(iMaxMsg, iDelay);
        if(!p_Msg)
        {
          continue;
        }
        destinationAddress = p_Msg->getAddress();
        ReliableFragment* p_Fragment = NULL;
        MsgFragment* p_NextFrag = p_Msg->firstFragment;
        if(p_NextFrag == NULL)
        {
          logTrace( "MSG", "SST", "Socket server ([%d - %d]) : Receive fragment null. Return", xport->handle().getNode(), xport->handle().getPort());
          return;
        }
        p_Fragment = ReliableFragment::parse((Byte*) p_NextFrag->read(0), p_NextFrag->len);
        p_Fragment->address = p_Msg->getAddress();
        //logTrace("MSG","SST","*********************** Receive fragment from [%d] - [%d] with seq [%d] len [%d]",destinationAddress.getNode(),destinationAddress.getPort(),p_Fragment->getFragmentId(),p_NextFrag->len);
        if(p_NextFrag->len > RUDP_HEADER_LEN)
        {
          p_Fragment->parseData((Byte*) p_NextFrag->read(RUDP_HEADER_LEN), p_NextFrag->len - RUDP_HEADER_LEN);
        }
        p_Fragment->setMessage(p_Msg);
        if(!isClosed)
        {
          if(p_Fragment->getType() == FRAG_SYN)
          {
            if(p_Fragment->isFirst == true)
            {
              logTrace( "MSG", "SST", "Socket server : Receive SYN fragment from [%d] - [%d] .", destinationAddress.getNode(), destinationAddress.getPort());
              if(clientSockTable[destinationAddress] == NULL)
              {
                logTrace( "MSG", "SST",
                    "Socket server Create socket client to handle connection from [%d] - [%d]", destinationAddress.getNode(), destinationAddress.getPort());
                addClientSocket(destinationAddress);
              }
              else
              {
                if(p_Fragment->getFragmentId() == 0)
                {
                  clientSockTable[destinationAddress]->resetSocket();
                }
              }
            }
            else
            {
              sendSock->receiverFragment(p_Fragment);
            }
          }
        }
        if((p_Fragment->getType() == FRAG_ACK || p_Fragment->getType() == FRAG_NAK))
        {
          if(p_Fragment->isFirst == false)
          {
            sendSock->receiverFragment(p_Fragment);
            continue;
          }
        }
        if((p_Fragment->getType() == FRAG_FIN))
        {
          if(p_Fragment->isFirst == false)
          {
            logTrace("MSG", "FRT", "Receive FIN fragment from sender");
            MsgReliableSocketClient *socket = clientSockTable[destinationAddress];
            socket->handleReliableFragment(p_Fragment);
            socket->receiveLock.lock();
            socket->resetSocketClient();
            socket->receiveLock.unlock();
            HandleSockMap::iterator it;
            for(it = clientSockTable.begin(); it != clientSockTable.end();)
            {
              if(it->first == destinationAddress)
              {
                // erase() invalidates the iterator but returns a new one
                // that points to the next element in the map.
                it = clientSockTable.erase(it);
              }
              else
              {
                ++it;
              }
            }
            socket->fragmentQueue.clear_and_dispose(delete_disposer());
            delete socket;
            socket = NULL;
          }
        }
        else
        {
          if(clientSockTable[destinationAddress] != NULL)
          {
            MsgReliableSocketClient *socket = clientSockTable[destinationAddress];
            if(compareFragment(p_Fragment->getFragmentId(), socket->rcvListInfo.getLastInSequence()) <= 0 && socket->rcvListInfo.getLastInSequence() != 0)
            {
              logTrace( "MSG", "FRT", "Drop duplicate fragment [%d]", socket->rcvListInfo.getLastInSequence());
            }
            else
            {
              socket->receiverFragment(p_Fragment);
            }
          }
        }
      }
      catch(...)
      {

      }
    }
    logTrace( "MSG", "SST", "Socket server ([%d - %d]) : Read message thread stoped.", xport->handle().getNode(), xport->handle().getPort());
  }
  void MsgReliableSocketServer::readMsgThreadFunc(void * arg)
  {
    MsgReliableSocketServer* p_this = (MsgReliableSocketServer*) arg;
    p_this->handlereadMsgThread();

  }
  MsgReliableSocketServer::MsgReliableSocketServer(uint_t port, MsgTransportPlugin_1* transport)
  {
    logTrace("MSG", "SST", "Init sock server. ");
    xport = transport->createSocket(port);
    readMsgThreadRunning = true;
    readMsgThread = boost::thread(readMsgThreadFunc, this);
    sendSock = new MsgReliableSocketClient(xport);
    node = xport->node;
    port = xport->port;
    sendSock->sockServer = this;
  }
  MsgReliableSocketServer::MsgReliableSocketServer(MsgSocket* socket)
  {
    xport = socket;
    msgPool = xport->getMsgPool();
    readMsgThreadRunning = true;
    readMsgThread = boost::thread(readMsgThreadFunc, this);
    sendSock = new MsgReliableSocketClient(xport);
    sendSock->sockServer = this;
    node = xport->node;
    port = xport->port;
  }

  void MsgReliableSocketServer::flush()
  {
    xport->flush();
  }

  Message* MsgReliableSocketServer::receive(uint_t maxMsgs, int maxDelay)
  {
    Message *m = NULL;
    int readMsgTimeOut = 500;
    if(msgs.empty())
    {
      readMsgLock.lock();
      readMsgCond.timed_wait(readMsgLock, readMsgTimeOut);
      readMsgLock.unlock();
    }
    if(!msgs.empty())
    {
      m = msgs.back();
      msgs.pop_back();
    }
    return m;
  }

#if 0
  void MsgReliableSocketServer::init()
  {
    readMsgThread.start_thread();
  }
#endif  

  MsgReliableSocketClient* MsgReliableSocketServer::accept()
  {
    listenSockMutex.lock();
    while(listenSock.empty())
    {
      try
      {
        logTrace( "REL", "INI", "Socket server : ListenSock list is empty. Waiting for socket client.");
        listenSockCond.wait(listenSockMutex);
      }
      catch(...)
      {
        //handle exception
      }
    }
    listenSockMutex.unlock();
    MsgReliableSocketClient* sock = (MsgReliableSocketClient*) &(listenSock.front());
    logTrace("MSG", "SST", "Get Socket from listenSock list.");
    listenSock.pop_front();
    return sock;
  }

  MsgReliableSocketServer::~MsgReliableSocketServer()
  {
    readMsgThreadRunning = false;
    //usleep(2000);
    readMsgThread.join();
    HandleSockMap::iterator it;
    if(clientSockTable.size() != 0)
    {
      for(it = clientSockTable.begin(); it != clientSockTable.end();)
      {
        MsgReliableSocketClient* & socket = it->second;
        it = clientSockTable.erase(it);
        delete socket;
        socket = NULL;
      }
    }
    sendSock->getMsgThreadRunning = false;
    sendSock->rcvFragmentThreadRunning = false;
    //wait for getMsgThread stopped
    //usleep(2000);
    sendSock->getMsgThread.join();
    sendSock->rcvFragmentThread.join();
    delete sendSock;
    sendSock = NULL;
    if(xport)
      xport->transport->deleteSocket(xport);
    xport = NULL;
  }
  void MsgReliableSocketServer::send(Message* msg)
  {
    sendSock->send(msg);
  }
  void MsgReliableSocketServer::connect(Handle destination, int timeout)
  {
    sendSock->connect(destination, timeout);
  }

  ReliableFragment* MsgReliableSocketClient::receiveReliableFragment(Handle &handle)
  {
    fragmentQueueLock.lock();
    while(fragmentQueue.empty())
    {
      try
      {
        if(!fragmentQueueCond.timed_wait(fragmentQueueLock, 1))
        {
          fragmentQueueLock.unlock();
          return NULL;
        }
      }
      catch(...)
      {
        // Handle Exception
      }
    }
    ReliableFragment* p_Fragment = &fragmentQueue.front();
    handle = p_Fragment->address;
    fragmentQueue.pop_front();
    fragmentQueueLock.unlock();
    if(p_Fragment->getType() == fragmentType::FRAG_DAT || p_Fragment->getType() == fragmentType::FRAG_NUL || p_Fragment->getType() == fragmentType::FRAG_RST
        || p_Fragment->getType() == fragmentType::FRAG_FIN || p_Fragment->getType() == fragmentType::FRAG_SYN)
    {
      rcvListInfo.increaseCumulativeAckCounter();
    }
    return p_Fragment;
  }

  void MsgReliableSocketClient::receiverFragment(ReliableFragment* frag)
  {
    fragmentQueueLock.lock();
    fragmentQueue.push_back(*frag);
    fragmentQueueCond.notify_one();
    fragmentQueueLock.unlock();
  }
  void MsgReliableSocketClient::connectionClientOpen()
  {
    logTrace("MSG", "CST", "Socket client :  Add socket to listen List");
    sockServer->listenSockMutex.lock();
    sockServer->listenSock.push_back(*this);
    sockServer->listenSockCond.notify_one();
    sockServer->listenSockMutex.unlock();
  }
  void MsgReliableSocketClient::resetSocketClient()
  {
    resetSocket();
    if(!fragmentQueue.empty())
    {
      fragmentQueue.clear_and_dispose(delete_disposer());
    }

  }

  void MsgReliableSocketClient::getMsgThreadFunc(void * arg)
  {
    MsgReliableSocketClient* p_this = (MsgReliableSocketClient*) arg;
    p_this->handleGetMsgThread();

  }
  void MsgReliableSocketClient::handleGetMsgThread()
  {
    logTrace("MSG", "CST", "Socket client :  Get Message thread start .");
    while(getMsgThreadRunning)
    {
      if(!isConnected)
      {
        usleep(1);
      }
      else
      {
        receiveLock.lock();
        this->receive(1);
        receiveLock.unlock();
      }
    }
    logTrace("MSG", "CST", "Socket client :  Get Message thread stopped .");
  }

  void MsgReliableSocketClient::startReceiveThread()
  {
    getMsgThreadRunning = true;
    getMsgThread = boost::thread(getMsgThreadFunc, this);
  }

  MsgReliableSocketClient::~MsgReliableSocketClient()
  {
    getMsgThreadRunning = false;
    rcvFragmentThreadRunning = false;
    getMsgThread.join();
    rcvFragmentThread.join();
    usleep(100000);
  }
}
;
