#include <reliableSocket.hxx>
#include "clLogApi.hxx"
#include "clCommon.hxx"
#include <stdio.h>
#define MAXSIZE 64000
#define MSG_SEND_TIMEOUT 3000
namespace SAFplus
{
  // Callback function for Null Fragment Timer
  static void * nullFragmentTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*)arg;
    logDebug("MSG","RST","start null fragment timer with interval [%d]",tempMsgSocket->nullFragmentTimer.interval);
    while(tempMsgSocket->nullFragmentTimer.isStop)
    {
      usleep(tempMsgSocket->nullFragmentTimer.interval*1000);
      if(tempMsgSocket->nullFragmentTimer.status==TIMER_RUN)
      {
        tempMsgSocket->nullFragmentTimer.timerLock.lock();
        tempMsgSocket->handleNullFragmentTimerFunc();
        tempMsgSocket->nullFragmentTimer.timerLock.unlock();
      }
    }
    return NULL;
  }

  // Callback function for retransmission Timer
  static void* retransmissionTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*)arg;
    logDebug("MSG","RST","Start retransmission timer with interval [%d] [%d]",tempMsgSocket->retransmissionTimer.interval,tempMsgSocket->retransmissionTimer.status);
    while(tempMsgSocket->retransmissionTimer.isStop)
    {
      usleep(tempMsgSocket->retransmissionTimer.interval*1000);
      if(tempMsgSocket->retransmissionTimer.status==TIMER_RUN)
      {
        tempMsgSocket->retransmissionTimer.timerLock.lock();
        tempMsgSocket->handleRetransmissionTimerFunc();
        tempMsgSocket->retransmissionTimer.timerLock.unlock();
      }
      //call function
    }
    return NULL;
  }

  // Callback function for cumulative Ack Timer
  static void* cumulativeAckTimerFunc(void* arg)
  {
    MsgReliableSocket* tempMsgSocket = (MsgReliableSocket*)arg;
    logDebug("MSG","RST","start cumulativeAck timer with interval [%d]",tempMsgSocket->cumulativeAckTimer.interval);
    while(tempMsgSocket->cumulativeAckTimer.isStop)
    {
      usleep(tempMsgSocket->cumulativeAckTimer.interval*1000);
      if(tempMsgSocket->cumulativeAckTimer.status==TIMER_RUN)
      {
        tempMsgSocket->cumulativeAckTimer.timerLock.lock();
        tempMsgSocket->handleCumulativeAckTimerFunc();
        tempMsgSocket->cumulativeAckTimer.timerLock.unlock();
      }
    }
    return NULL;
  }



  // Handle Retransmission function
  void MsgReliableSocket::handleRetransmissionTimerFunc(void)
  {
    unackedSentQueueLock.lock();
    logTrace("MSG","RST","Handle retransmission timer func");
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
        iter != unackedSentQueue.end();
        iter++)
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
    if (unackedSentQueue.empty())
    {
      try
      {
        // Send a new NULL segment if there is nothing to be retransmitted.
        logDebug("MSG","REL", "send NULL fragment");
        queueAndSendReliableFragment(new NULLFragment(rcvListInfo.nextFragmentId()));
      }
      catch (...)
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
    if (frag1 == frag2)
    {
      return 0;
    }
    else if (((frag1 < frag2) && ((frag2 - frag1) > MAX_FRAGMENT_NUMBER/2)) ||
        ((frag1 > frag2) && ((frag1 - frag2) < MAX_FRAGMENT_NUMBER/2)))
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
      setconnection(connectionNotification::RESET);
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
          state = connectionState::CONN_CLOSE_WAIT;
          break;
        }
      }
    }
    bool bIsInSequence = false;
    recvQueueLock.lock();
    logDebug("MSG","REL", "Receive Fragment **[%d]** . Last in sequence list  [%d] ",frag->getFragmentId(),rcvListInfo.getLastInSequence());
    if(compareFragment(frag->getFragmentId(), rcvListInfo.getLastInSequence() ) <= 0 )
    {
      /* Drop packet: duplicate. */
      logDebug("MSG","REL", "Fragment type : DATA, . Drop duplicate");
      delete frag;
      recvQueueLock.unlock();
      return;
    }
    else if(compareFragment(frag->getFragmentId(), nextSequenceNumber(rcvListInfo.getLastInSequence()) ) == 0 )
    {
      //fragment is the next fragment in queue. Add to in-seq queue
      bIsInSequence = true;
      if (inSeqQueue.size() == 0 || (inSeqQueue.size() + outOfSeqQueue.size() < recvQueueSize))
      {
        /* Insert in-sequence segment */
        rcvListInfo.setLastInSequence(frag->getFragmentId());
        if ( fragType == fragmentType::FRAG_DATA ||
            fragType ==  fragmentType::FRAG_RST ||
            fragType == fragmentType::FRAG_FIN)
        {
          logDebug("MSG","REL", "Fragment type : DATA . Add to in sequence queue");
          inSeqQueue.push_back(*frag);
        }
        checkRecvQueues();
      }
      else
      {
        /* Drop packet: queue is full. */
        logDebug("MSG","REL", "Drop packet: queue is full");

      }
    }
    else if(inSeqQueue.size() + outOfSeqQueue.size() < recvQueueSize)
    {
      //Fragment is not the next fragment in queue. Add it to out of seq queue
      bool added = false;
      for (ReliableFragmentList::iterator iter = outOfSeqQueue.begin();
          iter != outOfSeqQueue.end() && !added;
          iter ++)
      {
        ReliableFragment &s = *iter;
        int cmp = compareFragment(frag->getFragmentId(), s.getFragmentId());
        if (cmp == 0)
        {
          /* Ignore duplicate packet */
          added = true;
          delete frag;
        }
        else if (cmp < 0)
        {
          //logDebug("MSG","REL", "Data Fragment [%d] from [%d - %d] . Add to out of seq queue",frag->getFragmentId(),destination.getNode(),destination.getPort());
          outOfSeqQueue.insert(iter, *frag);
          rcvListInfo.increaseOutOfSequenceFragment();
          added = true;
        }
      }

      if (!added)
      {
        //logDebug("MSG","REL", "Data Fragment [%d] from [%d - %d]. Add to out of seq queue",frag->getFragmentId(),destination.getNode(),destination.getPort());
        outOfSeqQueue.push_back(*frag);
        rcvListInfo.increaseOutOfSequenceFragment();
      }
      //inscreate out of sequence fragment
      if (fragType == fragmentType::FRAG_DATA)
      {

      }
    }
    if (bIsInSequence &&
        ( fragType == fragmentType::FRAG_RST ||
            fragType == fragmentType::FRAG_NUL||
            fragType == fragmentType::FRAG_FIN))
    {
      //logDebug("MSG","REL", "Send Ack for RST/NUL/FIN fragment");
      sendAcknowledged();
    }
    else if ( (rcvListInfo.getOutOfSequenceFragment() > 0) &&
        (profile->maxOutOfSequence() == 0 || rcvListInfo.getOutOfSequenceFragment() > profile->maxOutOfSequence()) )
    {
      //logDebug("MSG","REL", "OutOfSequence queue not empty [%d]. Send NAK fragment",rcvListInfo.getOutOfSequenceFragment());
      sendNAK();
    }
    else if ((rcvListInfo.getCumulativeAckCounter() > 0) &&
        (profile->maxCumulativeAcks() == 0 || rcvListInfo.getCumulativeAckCounter() > profile->maxCumulativeAcks()))
    {
      sendAck();
    }
    else
    {
      cumulativeAckTimer.timerLock.lock();
      {
        if (cumulativeAckTimer.status==TIMER_PAUSE)
        {
          cumulativeAckTimer.interval=profile->getCumulativeAckInterval();
          cumulativeAckTimer.status=TIMER_RUN;
        }
      }
      cumulativeAckTimer.timerLock.unlock();
      cumulativeAckTimer.interval=profile->getCumulativeAckInterval();
      if(!cumulativeAckTimer.started)
      {
        boost::thread(cumulativeAckTimerFunc,this);
        cumulativeAckTimer.started=true;
      }
    }
    ReliableFragmentList::iterator it1 = inSeqQueue.end();
    --it1;
    ReliableFragment &s1 = *it1;
    if(s1.isLastFragment())
    {
      sendAck();
      logDebug("MSG", "REL","Notify to read message");
      recvQueueCond.notify_one();
    }
    recvQueueLock.unlock();
  }

  void MsgReliableSocket::checkRecvQueues()
  {
    int count = 0;
    ReliableFragmentList::iterator it = outOfSeqQueue.begin();
    //    bool haveLastFrag=false;
    while (it != outOfSeqQueue.end() )
    {
      ReliableFragment &s = *it;
      if (compareFragment(s.getFragmentId(), nextSequenceNumber(rcvListInfo.getLastInSequence())) == 0)
      {
        rcvListInfo.setLastInSequence(s.getFragmentId());
        it = outOfSeqQueue.erase(it);
        if (s.getType() == fragmentType::FRAG_DATA ||
            s.getType() == fragmentType::FRAG_RST ||
            s.getType() == fragmentType::FRAG_FIN)
        {
          logDebug("MSG", "REL","Move fragment [%d] to in sequence list ",s.getFragmentId());
          inSeqQueue.push_back(s);
          count++;
          //          if(s.isLastFragment())
          //          {
          //            haveLastFrag=true;
          //          }
        }
      }
      else
      {
        it++;
      }
    }
    if(count>0)
    {
      sendAck();
    }
    //    if(haveLastFrag)
    //    {
    //      logDebug("MSG", "REL","Notify to read message");
    //      recvQueueCond.notify_one();
    //    }
  }
  void MsgReliableSocket::handleSYNReliableFragment(SYNFragment *frag)
  {
    try
    {
      logDebug("MSG", "REL","Handle SYNC fragment [%d]",frag->getFragmentId());
      switch (state)
      {
        case connectionState::CONN_CLOSED:
        {
          rcvListInfo.setLastInSequence(frag->getFragmentId());
          state = connectionState::CONN_SYN_RCVD;
          logDebug("MSG", "RST","Socket([%d]-[%d]) : State CONN_CLOSED. set state to CONN_SYN_RCVD. Send sync frag to [%d] - [%d]",xPort->handle().getNode(),xPort->handle().getPort(), destination.getNode(),destination.getPort());
          logDebug("MSG", "RST","Socket PROFILE [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]",sendQueueSize,
              recvQueueSize,
              frag->getMaxFragmentSize(),
              frag->getMaxOutstandingFragments(),
              frag->getMaxRetrans(),
              frag->getMaxCumulativeAcks(),
              frag->getMaxOutOfSequence(),
              frag->getMaxAutoReset(),
              frag->getNulFragmentInterval(),
              frag->getRetransmissionIntervel(),
              frag->getCummulativeAckInterval());
          profile = new ReliableSocketProfile(
              sendQueueSize,
              recvQueueSize,
              frag->getMaxFragmentSize(),
              frag->getMaxOutstandingFragments(),
              frag->getMaxRetrans(),
              frag->getMaxCumulativeAcks(),
              frag->getMaxOutOfSequence(),
              frag->getMaxAutoReset(),
              frag->getNulFragmentInterval(),
              frag->getRetransmissionIntervel(),
              frag->getCummulativeAckInterval());
          SYNFragment* synFrag = new SYNFragment(
              rcvListInfo.setFragmentId(0),
              profile->maxNAKFrags(),
              profile->maxFragmentSize(),
              profile->getRetransmissionInterval(),
              profile->getCumulativeAckInterval(),
              profile->getNullFragmentInterval(),
              profile->maxRetrans(),
              profile->maxCumulativeAcks(),
              profile->maxOutOfSequence(),
              profile->maxAutoReset());
          rcvListInfo.reset();
          if(!inSeqQueue.empty())
          {
            inSeqQueue.clear_and_dispose(delete_disposer());
          }
          if(!outOfSeqQueue.empty())
          {
            outOfSeqQueue.clear_and_dispose(delete_disposer());
          }
          synFrag->isFirst=false; // sync fragment from receiver
          //logDebug("MSG", "RST","Socket([%d]-[%d]) : Set Ack. Send sync frag to [%d] - [%d]",xPort->handle().getNode(),xPort->handle().getPort(), destination.getNode(),destination.getPort());
          synFrag->setAck(frag->getFragmentId());
          queueAndSendReliableFragment(synFrag);
          break;
        }
        case connectionState::CONN_SYN_SENT:
        {
          logDebug("MSG", "RST","current connection([%d]-[%d]) state : CONN_CLOSED. Set connection to established",xPort->handle().getNode(),xPort->handle().getPort());
          rcvListInfo.setLastInSequence(frag->getFragmentId());
          state = connectionState::CONN_ESTABLISHED;
          /*
           * Here the client accepts or rejects the parameters sent by the
           * server. For now we will accept them.
           */
          this->sendAcknowledged(true);
          this->setconnection(connectionNotification::OPENED);
          break;
        }
        default:
        {
          logDebug("MSG", "RST","Connection state : Error");
          break;
        }
      }
    }
    catch (...)
    {
      // TO DO EXCEPTION
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
    int lastOutSequence = naks[length -1];
    unackedSentQueueLock.lock();
    //logDebug("MSG", "RST","Handle NAK fragment [%d] with length [%d]",frag->getFragmentId(),length);
    /* Removed acknowledged fragments from unackedSent queue */
    ReliableFragmentList::iterator it = unackedSentQueue.begin();
    while(it != unackedSentQueue.end())
    {
      ReliableFragment& s = *it;
      if ((compareFragment(s.getFragmentId(), lastInSequence) <= 0))
      {
        it = unackedSentQueue.erase_and_dispose(it, delete_disposer());
        continue;
      }
      bFound = false;
      for (int i = 0; i < length; i++)
      {
        if ((compareFragment(s.getFragmentId(), naks[i]) == 0))
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
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
        iter != unackedSentQueue.end();
        iter++ )
    {
      ReliableFragment& s = *iter;
      //logDebug("MSG", "RST","Check fragment [%d] %d %d ",s.getFragmentId(),compareFragment(lastInSequence, s.getFragmentId()),compareFragment(lastOutSequence, s.getFragmentId()));
      if ((compareFragment(lastInSequence, s.getFragmentId()) < 0) &&
          (compareFragment(lastOutSequence, s.getFragmentId()) > 0))
      {
        try
        {
          retransmitFragment(&s);
        }
        catch (...)
        {
          // xcp.printStackTrace();
          // Handle Exception
        }
      }
    }
    unackedSentQueueCond.notify_all();
    unackedSentQueueLock.unlock();
  }

  void MsgReliableSocket::sendAcknowledged(bool isFirst)
  {
    if (!outOfSeqQueue.empty() )
    {
      sendNAK();
    }
    else
    {
      sendAck(isFirst);
    }
  }
  void MsgReliableSocket::sendNAK()
  {
    if (outOfSeqQueue.empty())
    {
      return;
    }
    rcvListInfo.resetCumulativeAckFragment();
    rcvListInfo.resetOutOfSequenceFragment();

    /* Compose list of out-of-sequence sequence numbers */
    int size = outOfSeqQueue.size();
    int* acks = new int[size];
    int nIdx = 0;
    for (ReliableFragmentList::iterator it = outOfSeqQueue.begin();
        it != outOfSeqQueue.end();
        it++)
    {
      ReliableFragment& s = *it;
      acks[nIdx++] = s.getFragmentId();
      //logDebug("MSG", "RST","NAK fragment [%d]",s.getFragmentId());
    }
    try
    {
      int lastInSequence = rcvListInfo.getLastInSequence();
      logDebug("MSG", "RST","Send NAK fragment to Node [%d] with size [%d]",destination.getNode(),size);
      NAKFragment* frag = new NAKFragment(nextSequenceNumber(lastInSequence), lastInSequence, acks, size);
      this->sendReliableFragment(frag);
    }
    catch (...)
    {
      // Handle Exception
    }
    delete acks;
  }

  void MsgReliableSocket::sendAck(bool isFirst)
  {
    if (rcvListInfo.resetCumulativeAckFragment() == 0)
    {
      //logDebug("MSG", "RST","CumulativeAck = 0");
      return;
    }
    try
    {
      int lastInSequence = rcvListInfo.getLastInSequence();
      logDebug("MSG", "RST","Socket([%d - %d]) : Send  ACK [%d] to Node [%d] port [%d]",xPort->handle().getNode(),xPort->handle().getPort(),lastInSequence,destination.getNode(),destination.getPort());
      ReliableFragment *fragment=new ACKFragment(nextSequenceNumber(lastInSequence), lastInSequence);
      fragment->isFirst=isFirst;
      this->sendReliableFragment(fragment);
      delete fragment;
    }
    catch (...)
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
    if (rcvListInfo.resetCumulativeAckFragment() == 0)
    {
      return;
    }
    frag->setAck(rcvListInfo.getLastInSequence());
  }

  void MsgReliableSocket::getACK(ReliableFragment *frag)
  {
    int ackn = frag->getAck();
    if (ackn < 0)
    {
      //logDebug("MSG","RST","Ack < 0. Return ");
      return;
    }
    rcvListInfo.resetNAKFragment();
    if (state == connectionState::CONN_SYN_RCVD)
    {
      logDebug("MSG","RST","Socket([%d - %d]) : Current state CONN_SYN_RCVD. set to CONN_ESTABLISHED",xPort->handle().getNode(),xPort->handle().getPort());
      state = connectionState::CONN_ESTABLISHED;
      setconnection(connectionNotification::OPENED);
    }
    unackedSentQueueLock.lock();
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
        iter != unackedSentQueue.end();
    )
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
      if(retransmissionTimer.status==TIMER_RUN)
      {
        logTrace("MSG","RST","unackedSentQueue is empty. Pause retransmissionTimer timer ");
        retransmissionTimer.status=TIMER_PAUSE;
      }
    }
    if(ackn==lastFragmentIdOfMessage)
    {
      sendReliableLock.lock();
      sendReliableCond.notify_one();
      sendReliableLock.unlock();
    }
    unackedSentQueueCond.notify_all();
    unackedSentQueueLock.unlock();
  }

  //send a reliable fragment
  void MsgReliableSocket::sendReliableFragment(ReliableFragment *frag)
  {
    /* Piggyback any pending acknowledgments */
    if (frag->getType()==fragmentType::FRAG_DATA || frag->getType()==fragmentType::FRAG_RST || frag->getType()==fragmentType::FRAG_FIN || frag->getType()==fragmentType::FRAG_NUL)
    {
      setACK(frag);
    }
    /* Reset null segment timer */
    if (frag->getType()==fragmentType::FRAG_DATA || frag->getType()==fragmentType::FRAG_RST || frag->getType()==fragmentType::FRAG_FIN)
    {
      nullFragmentTimer.status=TimerStatus::TIMER_PAUSE;
      nullFragmentTimer.status=TimerStatus::TIMER_RUN;
    }
    //send reliable fragment
    if(xPort==NULL)
    {
      return;
    }
    if(msgPool==NULL)
    {
      return;
    }
    Byte* data;
    Message *message = msgPool->allocMsg();
    message->setAddress(this->destination);
    MsgFragment* hdr = message->append(0);
    Byte* buffer=frag->getHeader();
    hdr->set(buffer,RUDP_HEADER_LEN);
    if(frag->getlength() > 0)
    {
      MsgFragment* splitFrag = message->append(0);
      data =frag->getData();
      splitFrag->set(data,frag->getlength());
    }
    xPort->send(message);
    logTrace("MSG","RST","Send fragment .....");
    free(buffer);
    if(frag->getType()==fragmentType::FRAG_SYN||frag->getType()==fragmentType::FRAG_NAK)
    {
      free(data);
    }
  }

  // send fragment and store it in uncheck ack
  void MsgReliableSocket::queueAndSendReliableFragment(ReliableFragment* frag)
  {
    unackedSentQueueLock.lock();
    while ((unackedSentQueue.size() >= sendQueueSize) ||
        ( rcvListInfo.getNAKFragment() > profile->maxNAKFrags()))
    {
      try
      {
        unackedSentQueueCond.wait(unackedSentQueueLock);
      }
      catch (...)
      {
        // Hanlde Exception
      }
    }
    rcvListInfo.increaseNAKFragment();
    logTrace("MSG","RST","Socket([%d - %d]) : Store Fragment [%d] to unackedSent Queue",xPort->handle().getNode(),xPort->handle().getPort(),frag->getFragmentId());
    unackedSentQueue.push_back(*frag);
    unackedSentQueueLock.unlock();
    if (isClosed)
    {
      logTrace("MSG","RST","status is closed . Return");
      return;
    }
    /* Re-start retransmission timer */
    if (!(frag->getType() == fragmentType::FRAG_NAK) && !(frag->getType() == fragmentType::FRAG_ACK))
    {
      if (retransmissionTimer.status==TIMER_PAUSE)
      {
        logDebug("MSG","RST","Start retransmission timer [%d] interval [%d]",retransmissionTimer.status,retransmissionTimer.interval);
        retransmissionTimer.interval=profile->getRetransmissionInterval();
        retransmissionTimer.status=TIMER_RUN;
        if(!retransmissionTimer.started)
        {
          logDebug("MSG","RST","Socket : Start retransmission timer [%d] interval [%d]",retransmissionTimer.status,retransmissionTimer.interval);
          boost::thread(retransmissionTimerFunc,this);
          retransmissionTimer.started=true;
        }
      }
    }
    sendReliableFragment(frag);
  }

  Message* MsgReliableSocket::receiveOrigin(uint_t maxMsgs,int maxDelay)
  {
    logDebug("MSG","RST","receive buffer from [%d - %d] in reliable mode ",destination.getNode(),destination.getPort());
    int totalBytes = 0;
    bool quit=false;
    Handle address;
    if (isClosed)
    {
      throw Error("Socket is closed");
    }
    if (!isConnected)
    {
      throw Error("Connection reset");
    }
    if (timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {
      recvQueueLock.lock();
      recvQueueCond.timed_wait(recvQueueLock, timeout);
    }
    int maxFragmentSize=profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m = msgPool->allocMsg();
    assert(m);
    MsgFragment *temp;
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      //      logDebug("MSG","RST","Read the fragment id [%d] with length [%d]",s.getFragmentId(),s.getlength());
      if (s.getType() == fragmentType::FRAG_RST)
      {
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_FIN)
      {
        if (totalBytes <= 0)
        {
          it=inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_DATA)
      {
        if(s.isLastFragment())
        {
          quit=true;
          m->setAddress(s.address);
          //logDebug("MSG","RST","read the last fragment [%d]",s.getFragmentId());
        }
        int length = 0;
        Byte* data = s.getData();
        length = s.getlength();
        if(s.isFirst==true)
        {
          totalBytes=0;
          temp=m->append(0);
          buffer = (Byte*)SAFplusHeapAlloc(length);
          temp->set((void*)buffer,length);
          //logDebug("MSG","RST","current len [%d]",temp->len);
        }
        else
        {
          Byte* newBuffer=(Byte*)SAFplusHeapRealloc(buffer,totalBytes + length);
          buffer=newBuffer;
          temp->len+=((DATFragment*) &s)->getlength();
          //logDebug("MSG","RST","current len [%d]",temp->len);
        }
        memcpy(buffer + (totalBytes), (void*)data , length);
        totalBytes += length;
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        if(quit==true)
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

  Message* MsgReliableSocket::receiveFast(uint_t maxMsgs,int maxDelay)
  {
    logDebug("MSG","RST","receive buffer from [%d - %d] in reliable mode ",destination.getNode(),destination.getPort());
    int totalBytes = 0;
    bool quit=false;
    Handle address;
    if (isClosed)
    {
      throw Error("Socket is closed");
    }
    if (!isConnected)
    {
      throw Error("Connection reset");
    }
    if (timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {
      recvQueueLock.lock();
      recvQueueCond.timed_wait(recvQueueLock, timeout);
    }
    int maxFragmentSize=profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m=NULL;
    MsgFragment *temp;
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      //logDebug("MSG","RST","Read the fragment id [%d] with length [%d]",s.seq(),s.length());
      if (s.getType() == fragmentType::FRAG_RST)
      {
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_FIN)
      {
        if (totalBytes <= 0)
        {
          it=inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_DATA)
      {
        if(s.isLastFragment())
        {
          quit=true;
          m->setAddress(s.address);
          logDebug("MSG","RST","read the last fragment [%d]",s.getFragmentId());
        }
        s.message->lastFragment->start += RUDP_HEADER_LEN;
        s.message->lastFragment->len -= RUDP_HEADER_LEN;
        if(m==NULL)
        {
          m=s.message;
          logDebug("MSG","RST","read first last fragment [%d]",s.message->lastFragment->len);
        }
        else
        {
          logDebug("MSG","RST","append  last fragment [%d]",s.message->lastFragment->len);
          m->lastFragment->nextFragment=s.message->lastFragment;
          m->lastFragment=s.message->lastFragment;
          Message* old;
          old=s.message;
          old->nextMsg=NULL;  // clear these out so they don't get cleaned up
          old->firstFragment=NULL;
          old->lastFragment=NULL;
          msgPool->free(old);
        }
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        logDebug("MSG","RST","debug the last fragment len [%d]",m->lastFragment->len);
        if(quit==true)
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

  Message* MsgReliableSocket::receive(uint_t maxMsgs,int maxDelay)
  {
    logDebug("MSG","RST","receive buffer from [%d - %d] in reliable mode ",destination.getNode(),destination.getPort());
    int totalBytes = 0;
    bool quit=false;
    Handle address;
    if (isClosed)
    {
      throw Error("Socket is closed");
    }
    if (!isConnected)
    {
      throw Error("Connection reset");
    }
    if (timeout == 0)
    {
      recvQueueLock.lock();
      recvQueueCond.wait(recvQueueLock);
    }
    else
    {
      recvQueueLock.lock();
      recvQueueCond.timed_wait(recvQueueLock, timeout);
    }
    int maxFragmentSize=profile->maxFragmentSize() - RUDP_HEADER_LEN;
    Byte* buffer=NULL;
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    Message* m = msgPool->allocMsg();
    assert(m);
    MsgFragment *temp;
    temp=m->append(0);
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      logDebug("MSG","RST","Read the fragment id [%d] with length [%d]",s.getFragmentId(),s.getlength());
      if (s.getType() == fragmentType::FRAG_RST)
      {
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_FIN)
      {
        if (totalBytes <= 0)
        {
          it=inSeqQueue.erase_and_dispose(it, delete_disposer());
          recvQueueLock.unlock();
          return NULL; /* EOF */
        }
        it=inSeqQueue.erase_and_dispose(it, delete_disposer());
      }
      else if (s.getType() == fragmentType::FRAG_DATA)
      {
        if(s.isLastFragment())
        {
          quit=true;
          m->setAddress(s.address);
          logDebug("MSG","RST","read the last fragment [%d]",s.getFragmentId());
        }
        int length = 0;
        Byte* data = s.getData();
        length = s.getlength();
        Byte* newBuffer;
        if(buffer==NULL)
        {
          logDebug("MSG","RST","read first fragment [%d]",s.getFragmentId());
          buffer = (Byte*)SAFplusHeapAlloc(length);
        }
        else
        {
          newBuffer=(Byte*)SAFplusHeapRealloc(buffer,totalBytes + length);
          buffer=newBuffer;
        }
        temp->len+=((DATFragment*) &s)->getlength();
        logDebug("MSG","RST","current len [%d]",temp->len);
        memcpy(buffer + (totalBytes), (void*)data , length);
        totalBytes += length;
        it = inSeqQueue.erase_and_dispose(it, delete_disposer());
        if(quit==true)
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
      temp->set((void*)buffer,totalBytes);
      recvQueueLock.unlock();
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

    p_Msg = this->xPort->receive( iMaxMsg, iDelay);
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
    p_Fragment = ReliableFragment::parse((Byte*)p_NextFrag->read(0),p_NextFrag->len);
    p_Fragment->address=p_Msg->getAddress();
    fragmentType fragType = p_Fragment->getType();
    if(fragType == fragmentType::FRAG_DATA ||
        fragType == fragmentType::FRAG_NUL ||
        fragType == fragmentType::FRAG_RST ||
        fragType == fragmentType::FRAG_FIN ||
        fragType == fragmentType::FRAG_SYN)
    {
      rcvListInfo.increaseCumulativeAckCounter();
    }
    if(p_NextFrag->len > RUDP_HEADER_LEN)
    {
      p_Fragment->parseData((Byte*)p_NextFrag->read(RUDP_HEADER_LEN),p_NextFrag->len-RUDP_HEADER_LEN);
    }
    else
    {
      logTrace("MSG","FRT","data is null");
    }
    p_Msg->msgPool->free(p_Msg);
    return p_Fragment;
  }

  void MsgReliableSocket::retransmitFragment(ReliableFragment *frag)
  {
    logDebug("MSG","RST","Socket([%d - %d]) : Retransmit Fragment fradId [%d] ",xPort->handle().getNode(),xPort->handle().getPort(), frag->getFragmentId());
    if (profile->maxRetrans() > 0)
    {
      frag->setRetxCounter(frag->getRetxCounter()+1);
    }
    if (profile->maxRetrans() != 0 && frag->getRetxCounter() > profile->maxRetrans())
    {
      setconnection(connectionNotification::FAILURE);
      return ;
    }
    assert(xPort);
    Byte* data;
    Message *message = msgPool->allocMsg();
    message->setAddress(this->destination);
    MsgFragment* hdr = message->append(0);
    Byte* buffer =frag->getHeader();
    hdr->set(buffer,RUDP_HEADER_LEN);
    //logDebug("MSG","FRT","send message with header length [%d]",message->firstFragment->len);
    if(frag->getlength() > 0)
    {
      MsgFragment* splitFrag = message->append(0);
      data = frag->getData();
      splitFrag->set(data,frag->getlength());
    }
    xPort->send(message);
    free(buffer);
    if(frag->getType()==fragmentType::FRAG_SYN||frag->getType()==fragmentType::FRAG_NAK)
    {
      free(data);
    }
  }

  void MsgReliableSocket::connectionFailure()
  {
    logDebug("MSG","RST","Socket([%d - %d]) : Set connection to Failure",xPort->handle().getNode(),xPort->handle().getPort());
    closeMutex.lock();
    {
      if (isClosed)
      {
        closeMutex.unlock();
        return;
      }
      switch (state)
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
        case connectionState::CONN_CLOSE_WAIT :
        case connectionState::CONN_SYN_RCVD :
        case connectionState::CONN_ESTABLISHED :
        {
          isConnected = false;
          unackedSentQueueLock.lock();
          {
            unackedSentQueueCond.notify_all();
          }
          unackedSentQueueLock.unlock();
          closeImpl();
          break;
        }
      }
      state = connectionState::CONN_CLOSED;
      isClosed = true;
    }
    closeMutex.unlock();
  }

  void MsgReliableSocket::closeImpl()
  {
    nullFragmentTimer.status=TIMER_PAUSE;
    state = connectionState::CONN_CLOSE_WAIT;
    nullFragmentTimer.isStop=true;
    try
    {
      boost::this_thread::sleep_for(boost::chrono::seconds{profile->getNullFragmentInterval() * 2});
    }
    catch (...)
    {
    }
    retransmissionTimer.isStop=true;
    cumulativeAckTimer.isStop=true;
  }
  void MsgReliableSocket::setconnection(connectionNotification state)
  {
    switch(state)
    {
      case connectionNotification::CLOSE :
        break;
      case connectionNotification::FAILURE :
      {
        connectionFailure();
        break;
      }
      case connectionNotification::OPENED :
      {
        connectionOpened();
        break;
      }
      case connectionNotification::REFUSED :
        break;
      case connectionNotification::RESET :
        break;
    }
  }

  void MsgReliableSocket::connectionOpened(void)
  {
    logDebug("MSG","RST","Socket([%d - %d]) : set connection to Opened",xPort->handle().getNode(),xPort->handle().getPort());
    if(isConnected)
    {
      nullFragmentTimer.status = TIMER_RUN;
      nullFragmentTimer.interval=profile->getNullFragmentInterval();
      boost::thread(nullFragmentTimerFunc,this);
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
      connectionClientOpen();

    }
  }
  static void ReliableSocketThread(void * arg)
  {
    MsgReliableSocket* p_this = (MsgReliableSocket*)arg;
    p_this->handleReliableSocketThread();
  }

  MsgReliableSocket::MsgReliableSocket(uint_t port,MsgTransportPlugin_1* transport)
  {
    xPort=transport->createSocket(port);
    msgPool=xPort->getMsgPool();
    init();
  }
  MsgReliableSocket::MsgReliableSocket(uint_t port,MsgTransportPlugin_1* transport,Handle destination)
  {
    xPort=transport->createSocket(port);
    msgPool=xPort->getMsgPool();
    init();
    this->destination=destination;
  }
  MsgReliableSocket::MsgReliableSocket(MsgSocket* socket)
  {
    xPort=socket;
    msgPool=xPort->getMsgPool();
    init();
  }
  void MsgReliableSocket::init()
  {
    rcvListInfo.fragNumber =0;
    rcvListInfo.lastFrag=0;
    rcvListInfo.numberOfCumAck=0;
    rcvListInfo.numberOfOutOfSeq=0;
    rcvListInfo.numberOfNakFrag=0; /* Outstanding segments counter */
    nullFragmentTimer.status=TIMER_PAUSE;
    retransmissionTimer.status=TIMER_PAUSE;
    cumulativeAckTimer.status=TIMER_PAUSE;
    nullFragmentTimer.started=false;
    retransmissionTimer.started=false;
    cumulativeAckTimer.started=false;
    nullFragmentTimer.interval=0;
    retransmissionTimer.interval=0;
    cumulativeAckTimer.interval=0;
    nullFragmentTimer.isStop=true;
    lastFragmentIdOfMessage=0;
    profile=new ReliableSocketProfile();
    rcvThread = boost::thread(ReliableSocketThread, this);
  }

  void MsgReliableSocket::handleReliableSocketThread(void)
  {
    fragmentType fragType = fragmentType::FRAG_UDE;
    Handle handle;
    logDebug("MSG","RST"," Socket([%d - %d]) : start thread to receive and handle fragment from [%d] [%d].",xPort->handle().getNode(),xPort->handle().getPort(),destination.getNode(),destination.getPort());
    int test=0;
    while (1)
    {
      ReliableFragment* pFrag =receiveReliableFragment(handle);
      if(pFrag==NULL)
      {
        logDebug("MSG","RST","Receive NULL fragment . Exit thread");
        return;
      }
      fragType = pFrag->getType();

      if (fragType == fragmentType::FRAG_SYN)
      {
        this->destination = handle;
        logDebug("MSG","RST","set destination 1");
        handleSYNReliableFragment((SYNFragment*)pFrag);
      }
      else if (fragType == fragmentType::FRAG_NAK)
      {
        handleNAKReliableFragment((NAKFragment*)pFrag);
      }
      else if (fragType == fragmentType::FRAG_ACK)
      {
        logTrace("MSG","RST","Receive ACK fragment [%d]",pFrag->getAck());
      }
      else
      {
        //         For testing ............
        test++;
        if(test % 4 == 2)
        {
          logDebug("MSG","RST","force to remove fragment **[%d]**",pFrag->getFragmentId());
          delete pFrag;
          continue;
        }
        else
        {
          handleReliableFragment(pFrag);
        }
//        logDebug("MSG","RST","Socket([%d - %d]) : Receive reliable fragment Id [%d] last [%d] fragment type [%d] from [%d - %d]",xPort->handle().getNode(),xPort->handle().getPort(),pFrag->getFragmentId(),pFrag->isLastFragment(),fragType,handle.getNode(),handle.getPort());
//        handleReliableFragment(pFrag);
      }
      getACK(pFrag);
      if (fragType == fragmentType::FRAG_ACK||fragType == fragmentType::FRAG_SYN||fragType == fragmentType::FRAG_NAK)
      {
        delete pFrag;
      }
    }
    logDebug("MSG","RST","Receive NULL fragment . Exit thread");
  }

  MsgReliableSocket::~MsgReliableSocket()
  {
    //TODO
  }

  void MsgReliableSocket::send(Message* origMsg)
  {

    Message* msg;
    Message* nextMsg = origMsg;
    int chunkCount=0;
    Message* newMsg = NULL;
    do
    {
      msg = nextMsg;
      nextMsg = msg->nextMsg;
      sendOneMsg(msg);
    }while (nextMsg != NULL);
    //sleep(10);
  }


  void MsgReliableSocket::sendOneMsg(Message* origMsg)
  {
    Message* nextMsg = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    nextFrag = origMsg->firstFragment;
    bool lastFrag = false;
    do
    {
      frag = nextFrag;
      nextFrag = frag->nextFragment;
      if(nextFrag==NULL)
      {
        lastFrag = true;
      }
      if (frag->len == MAXSIZE )  // In this case we just need to move the fragment to a new message not split it.
      {
        ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(),
            rcvListInfo.getLastInSequence(), (Byte*)frag->data(0) ,0, frag->len ,lastFrag,true);
        queueAndSendReliableFragment(fragment);
      }
      else if (frag->len > MAXSIZE)
      {
        int totalBytes = 0;
        int writeBytes = 0;
        bool isFirst=true;
        while(totalBytes<frag->len)
        {
          writeBytes = MIN(MAXSIZE - RUDP_HEADER_LEN,frag->len - totalBytes);
          if(totalBytes+writeBytes<frag->len)
          {
            ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(),
                rcvListInfo.getLastInSequence(), (Byte*)frag->data(0),totalBytes, writeBytes,false,isFirst);
            queueAndSendReliableFragment(fragment);
            totalBytes += writeBytes;
          }
          else
          {
            queueAndSendReliableFragment(new DATFragment(rcvListInfo.nextFragmentId(),
                rcvListInfo.getLastInSequence(), (Byte*)frag->data(0), totalBytes, writeBytes,lastFrag,isFirst));
            totalBytes += writeBytes;
          }
          isFirst=false;
        }
      }
      else
      {
        ReliableFragment *fragment = new DATFragment(rcvListInfo.nextFragmentId(),
            rcvListInfo.getLastInSequence(), (Byte*)frag->data(0), 0 , frag->len,lastFrag,true);
        queueAndSendReliableFragment(fragment);
      }
    } while(nextFrag);
    lastFragmentIdOfMessage=rcvListInfo.getFragmentId();
    //logTrace("MSG","RST","update last fragment to [%d]",lastFragmentIdOfMessage);
    sendReliableLock.lock();
    //wait until send successful or timeout
    sendReliableCond.timed_wait(sendReliableLock,MSG_SEND_TIMEOUT);
    sendReliableLock.unlock();
    logTrace("MSG","RST","Send message done");
  }

  void MsgReliableSocket::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    connect(destination,0);
    logDebug("MSG","RST","Send buffer with len [%d] to  node [%d] in reliable mode ",length,destination.getNode());
    if (isClosed)
    {
      throw new Error("Socket is closed");
    }
    if (!isConnected)
    {
      throw new Error("Connection reset");
    }

    int totalBytes = 0;
    int writeBytes = 0;
    while(totalBytes<length)
    {
      writeBytes = MIN(profile->maxFragmentSize() - RUDP_HEADER_LEN,length - totalBytes);
      logTrace("MSG","RST","sending [%d] byte",writeBytes);
      if(totalBytes+writeBytes<length)
      {
        ReliableFragment *frag = new DATFragment(rcvListInfo.nextFragmentId(),
            rcvListInfo.getLastInSequence(), (Byte*)buffer, totalBytes, writeBytes,msgPool,false);
        logTrace("MSG","RST","Socket([%d - %d]) : Create fragment with seq [%d] type [%d] ack [%d].",xPort->handle().getNode(),xPort->handle().getPort(),frag->getFragmentId(),frag->getType(),frag->getAck());
        queueAndSendReliableFragment(frag);
        totalBytes += writeBytes;
      }
      else
      {
        logDebug("MSG", "REL","send last fragment to  node [%d] in reliable mode ",destination.getNode());
        queueAndSendReliableFragment(new DATFragment(rcvListInfo.nextFragmentId(),
            rcvListInfo.getLastInSequence(), (Byte*)buffer, totalBytes, writeBytes,msgPool,true));
        totalBytes += writeBytes;
      }
    }
  }

  void MsgReliableSocket::connect(Handle destination, int timeout)
  {
    logDebug("REL","MSR","Connect to socket server");
    if (timeout < 0)
    {
      throw Error("connect: timeout can't be negative");
    }
    if (isClosed)
    {
      throw Error("Socket is closed");
    }
    if (isConnected)
    {
      return;
      throw new Error("already connected");
    }
    this->destination = destination;
    logDebug("MSG","RST","set destination 2");
    // Synchronize sequence numbers
    state = connectionState::CONN_SYN_SENT;
    if(!unackedSentQueue.empty())
    {
      unackedSentQueue.clear_and_dispose(delete_disposer());
    }
    int sequenceNum=rcvListInfo.setFragmentId(0);
    ReliableFragment *frag = new SYNFragment(sequenceNum,
        profile->maxNAKFrags(), profile->maxFragmentSize(),
        profile->getRetransmissionInterval(), profile->getCumulativeAckInterval(),
        profile->getNullFragmentInterval(), profile->maxRetrans(),
        profile->maxCumulativeAcks(), profile->maxOutOfSequence(),
        profile->maxAutoReset());
    frag->isFirst=true; //sync fragment from sender

    logTrace("MSG","RST","Socket([%d - %d]) : Create syn fragment with folow parameter [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d].",xPort->handle().getNode(),xPort->handle().getPort(),sequenceNum,
        profile->maxNAKFrags(), profile->maxFragmentSize(),
        profile->getRetransmissionInterval(), profile->getCumulativeAckInterval(),
        profile->getNullFragmentInterval(), profile->maxRetrans(),
        profile->maxCumulativeAcks(), profile->maxOutOfSequence(),
        profile->maxAutoReset());
    queueAndSendReliableFragment(frag);
    // Wait for connection establishment (or timeout)
    bool timedout = false;
    thisMutex.lock();
    if (!isConnected)
    {
      try
      {
        if (timeout == 0)
        {
          logDebug("MSG","SST","waiting for sync reply");
          thisCond.wait(thisMutex);
          logDebug("MSG","SST","waiting for sync reply done");
        }
      }
      catch (...)
      {
        // Hanle Exception
      }
    }
    thisMutex.unlock();
    if (state == connectionState::CONN_ESTABLISHED)
    {
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
    switch (state)
    {
      case connectionState::CONN_SYN_SENT:
      {
        setconnection(connectionNotification::REFUSED);
        state = connectionState::CONN_CLOSED;
        if (timedout)
        {
          throw new Error("SocketTimeoutException");
        }
        throw new Error("Connection refused");

      }
      case connectionState::CONN_CLOSED:
      case connectionState::CONN_CLOSE_WAIT:
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
      if (isClosed)
      {
        closeMutex.unlock();
        return;
      }
      switch (state)
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
        case connectionState::CONN_CLOSE_WAIT:
        case connectionState::CONN_SYN_RCVD:
        case connectionState::CONN_ESTABLISHED:
        {
          sendReliableFragment(new FINFragment(rcvListInfo.nextFragmentId()));
          closeImpl();
          break;
        }
        case connectionState::CONN_CLOSED:
        {
          retransmissionTimer.isStop = true;
          cumulativeAckTimer.isStop = true;
          nullFragmentTimer.isStop = true;
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
    xPort->flush();
  }
  void MsgReliableSocket::connectionClientOpen()
  {
  };

  void MsgReliableSocketServer::addClientSocket(Handle destAddress)
  {
    if (clientSockTable[destAddress] == NULL)
    {
      try
      {
        MsgReliableSocketClient *sockClient = new MsgReliableSocketClient(xPort,destAddress);
        sockClient->sockServer=this;
        clientSockTable[destAddress]=sockClient;
      }
      catch (...)
      {
        // Handle Exception
        throw Error("add client socket error");
      }
    }
  }

  void MsgReliableSocketServer::removeClientSocket(Handle destAddress)
  {
    clientSockTable.erase(destAddress);
    if (clientSockTable.empty())
    {
      if (isClosed==true)
      {
        delete xPort;
      }
    }
  }

  void MsgReliableSocketServer::handleRcvThread()
  {
    int iMaxMsg = 1;
    int iDelay = -1;
    Handle destinationAddress;
    logDebug("MSG","SST","Socket server ([%d - %d]) : Start server receive thread.",xPort->handle().getNode(),xPort->handle().getPort());
    while (true)
    {
      try
      {
        Message* p_Msg =this->xPort->receive(iMaxMsg,iDelay);
        assert(p_Msg);
        destinationAddress = p_Msg->getAddress();
        ReliableFragment* p_Fragment = NULL;
        MsgFragment* p_NextFrag = p_Msg->firstFragment;
        if(p_NextFrag == NULL)
        {
          logDebug("MSG","SST","Socket server ([%d - %d]) : Receive fragment null. Return",xPort->handle().getNode(),xPort->handle().getPort());
          return;
        }
        p_Fragment = ReliableFragment::parse((Byte*)p_NextFrag->read(0), p_NextFrag->len);
        p_Fragment->address=p_Msg->getAddress();
        //logDebug("MSG","SST","Socket server ([%d - %d]) : Receive fragment from [%d] - [%d] with seq [%d] len [%d]",xPort->handle().getNode(),xPort->handle().getPort(),destinationAddress.getNode(),destinationAddress.getPort(),p_Fragment->getFragmentId(),p_NextFrag->len);
        if(p_NextFrag->len > RUDP_HEADER_LEN)
        {
          p_Fragment->parseData((Byte*)p_NextFrag->read(RUDP_HEADER_LEN),p_NextFrag->len-RUDP_HEADER_LEN);
        }
        p_Fragment->setMessage(p_Msg);
        if(!isClosed)
        {
          if (p_Fragment->getType()==FRAG_SYN)
          {
            if(p_Fragment->isFirst==true)
            {
              logDebug("MSG","SST","Socket server : Receive SYN fragment from [%d] - [%d] .",destinationAddress.getNode(),destinationAddress.getPort());
              if(clientSockTable[destinationAddress]==NULL)
              {
                logDebug("MSG","SST","Socket server Create socket client and add to client Table");
                addClientSocket(destinationAddress);
              }
            }
            else
            {
              logTrace("MSG","FRT","sync from receiver");
              sendSock->receiverFragment(p_Fragment);
            }
          }
        }
        if((p_Fragment->getType()==FRAG_ACK||p_Fragment->getType()==FRAG_NAK))
        {
          if(p_Fragment->isFirst==false)
          {
            logTrace("MSG","FRT","ACK-NAK from receiver");
            sendSock->receiverFragment(p_Fragment);
          }
          else
          {
            logTrace("MSG","FRT","ACK-NAK from sender");
            MsgReliableSocketClient *socket = clientSockTable[destinationAddress];
            socket->receiverFragment(p_Fragment);
          }
        }
        else
        {
          if(clientSockTable[destinationAddress]!=NULL)
          {
            MsgReliableSocketClient *socket = clientSockTable[destinationAddress];
            //logTrace("MSG","FRT","receive Data fragment ke ke [%d] [%d] [%d]",p_Fragment->getFragmentId(),destinationAddress.getNode(),destinationAddress.getPort());
            socket->receiverFragment(p_Fragment);
          }
        }
        //        if(!isClosed)
        //        {
        //          if (p_Fragment->getType()==FRAG_SYN)
        //          {
        //            logDebug("MSG","SST","Socket server : Receive SYN fragment from [%d] - [%d] .",destinationAddress.getNode(),destinationAddress.getPort());
        //            if(clientSockTable[destinationAddress]==NULL)
        //            {
        //              logDebug("MSG","SST","Socket server Create socket client and add to client Table");
        //              addClientSocket(destinationAddress);
        //            }
        //          }
        //        }
        //        if(clientSockTable[destinationAddress]!=NULL)
        //        {
        //          MsgReliableSocketClient *socket = clientSockTable[destinationAddress];
        //          socket->receiverFragment(p_Fragment);
        //        }
      }catch (...)
      {

      }
    }
  }
  void MsgReliableSocketServer::rcvThread(void * arg)
  {
    MsgReliableSocketServer* p_this = (MsgReliableSocketServer*)arg;
    p_this->handleRcvThread();

  }
  MsgReliableSocketServer::MsgReliableSocketServer(uint_t port,MsgTransportPlugin_1* transport)
  {
    logDebug("MSG","SST","Init sock server. ");
    xPort=transport->createSocket(port);
    ServerRcvThread = boost::thread(rcvThread, this);
    sendSock = new MsgReliableSocketClient(xPort);
    sendSock->sockServer=this;
  }
  MsgReliableSocketServer::MsgReliableSocketServer(MsgSocket* socket)
  {
    xPort=socket;
    ServerRcvThread = boost::thread(rcvThread, this);
  }
  void MsgReliableSocketServer::init()
  {
    ServerRcvThread.start_thread();
  }

  MsgReliableSocketClient* MsgReliableSocketServer::accept()
  {
    listenSockMutex.lock();
    while (listenSock.empty())
    {
      try
      {
        logDebug("REL","INI", "Socket server : ListenSock list is empty. Waiting for socket client.");
        listenSockCond.wait(listenSockMutex);
      }
      catch (...)
      {
        //handle exception
      }
    }
    listenSockMutex.unlock();
    MsgReliableSocketClient* sock = (MsgReliableSocketClient*)&(listenSock.front());
    logDebug("MSG","SST", "Get Socket from listenSock list.");
    listenSock.pop_front();
    return sock;
  }
  void MsgReliableSocketServer::send(Message* msg)
  {
    logDebug("MSG","SST", "Sending message.");
    sendSock->destination=msg->getAddress();
    sendSock->send(msg);
  }
  void MsgReliableSocketServer::connect(Handle destination, int timeout)
  {
    sendSock->connect(destination, timeout);
  }

  ReliableFragment* MsgReliableSocketClient::receiveReliableFragment(Handle &handle)
  {
    fragmentQueueLock.lock();
    while (fragmentQueue.empty())
    {
      try
      {
        fragmentQueueCond.wait(fragmentQueueLock);
      }
      catch(...)
      {
        // Handle Exception
      }
    }

    ReliableFragment* p_Fragment=&fragmentQueue.front();
    handle=p_Fragment->address;
    fragmentQueue.pop_front();
    fragmentQueueLock.unlock();
    if(p_Fragment->getType() == fragmentType::FRAG_DATA ||
        p_Fragment->getType() == fragmentType::FRAG_NUL ||
        p_Fragment->getType() == fragmentType::FRAG_RST ||
        p_Fragment->getType() == fragmentType::FRAG_FIN ||
        p_Fragment->getType() == fragmentType::FRAG_SYN)
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
    logDebug("MSG", "CST","Socket client :  Add socket to listen List");
    sockServer->listenSockMutex.lock();
    sockServer->listenSock.push_back(*this);
    sockServer->listenSockCond.notify_one();
    sockServer->listenSockMutex.unlock();
  }
};
