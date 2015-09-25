#include <reliableSocket.hxx>
#include "clLogApi.hxx"
#include "clCommon.hxx"
#include <stdio.h>

namespace SAFplus
{
  // Callback function for Null Fragment Timer
  static void * nullFragmentTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
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
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
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
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
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



  // Callback function for Keep alive Timer
  static void* keepAliveTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    logDebug("MSG","RST","start keep alive timer");
    while(tempMsgSocket->keepAliveTimer.isStop)
    {
      usleep(tempMsgSocket->keepAliveTimer.interval*1000);
      if(tempMsgSocket->keepAliveTimer.status==TIMER_RUN)
      {
        tempMsgSocket->keepAliveTimer.timerLock.lock();
        tempMsgSocket->handleKeepAliveTimerFunc();
        tempMsgSocket->keepAliveTimer.timerLock.unlock();
      }
    }
    return NULL;
  }


  // Handle Retransmission function
  void MsgSocketReliable::handleRetransmissionTimerFunc(void)
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
  void MsgSocketReliable::handleNullFragmentTimerFunc(void)
  {
      if (unackedSentQueue.empty())
      {
        try
        {
          // Send a new NULL segment if there is nothing to be retransmitted.
          queueAndSendReliableFragment(new NULLFragment(queueInfo.nextSequenceNumber()));
        }
        catch (...)
        {
          // Hanlde Exception
        }
      }
  }

  // Handle Cumulative Ack function
  void MsgSocketReliable::handleCumulativeAckTimerFunc(void)
  {
    this->sendACK();
  }

  // Handle Cumulative keep alive function
  void MsgSocketReliable::handleKeepAliveTimerFunc(void)
  {
    this->setconnection(connectionNotification::FAILURE);
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

  void MsgSocketReliable::handleReliableFragment(ReliableFragment *frag)
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
    if(compareFragment(frag->seq(), queueInfo.getLastInSequence() ) <= 0 )
    {
      /* Drop packet: duplicate. */
      logDebug("MSG","REL", "Fragment type : DATA, Id : [%d] . Drop duplicate",frag->seq());

    }
    else if(compareFragment(frag->seq(), nextSequenceNumber(queueInfo.getLastInSequence()) ) == 0 )
    {
      //fragment is the next fragment in queue. Add to in-seq queue
      bIsInSequence = true;
      if (inSeqQueue.size() == 0 || (inSeqQueue.size() + outOfSeqQueue.size() < recvQueueSize))
      {
        /* Insert in-sequence segment */
        queueInfo.setLastInSequence(frag->seq());
        if ( fragType == fragmentType::FRAG_DATA ||
            fragType ==  fragmentType::FRAG_RST ||
            fragType == fragmentType::FRAG_FIN)
        {
          logDebug("MSG","REL", "Fragment type : DATA Data, Id [%d] . Add to in sequence queue",frag->seq());
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
        int cmp = compareFragment(frag->seq(), s.seq());
        if (cmp == 0)
        {
          /* Ignore duplicate packet */
          added = true;
          delete frag;
        }
        else if (cmp < 0)
        {
          logDebug("MSG","REL", "Data Fragment [%d] from [%d - %d] . Add to out of seq queue",frag->seq(),destination.getNode(),destination.getPort());
          outOfSeqQueue.insert(iter, *frag);
          added = true;
        }
      }

      if (!added)
      {
        logDebug("MSG","REL", "Data Fragment [%d] from [%d - %d]. Add to out of seq queue",frag->seq(),destination.getNode(),destination.getPort());
        outOfSeqQueue.push_back(*frag);
      }
      //inscreate out of sequence fragment
      queueInfo.increaseOutOfSequenceCounter();
      if (fragType == fragmentType::FRAG_DATA)
      {

      }
    }
    if (bIsInSequence &&
        ( fragType == fragmentType::FRAG_RST ||
            fragType == fragmentType::FRAG_NUL||
            fragType == fragmentType::FRAG_FIN))
    {
      logDebug("MSG","REL", "Send Ack for RST/NUL/FIN fragment");
      sendACK();
    }
    else if ( (queueInfo.getOutOfSequenceCounter() > 0) &&
        (profile->maxOutOfSequence() == 0 || queueInfo.getOutOfSequenceCounter() > profile->maxOutOfSequence()) )
    {
      logDebug("MSG","REL", "OutOfSequence queue not empty. Send NAK fragment");
      sendNAK();
    }
    else if ((queueInfo.getCumulativeAckCounter() > 0) &&
        (profile->maxCumulativeAcks() == 0 || queueInfo.getCumulativeAckCounter() > profile->maxCumulativeAcks()))
    {
      logDebug("MSG","REL", "Send single ACK");
      sendSingleAck();
    }
    else
    {
      cumulativeAckTimer.timerLock.lock();
      {
        if (cumulativeAckTimer.status==TIMER_PAUSE)
        {
          cumulativeAckTimer.interval=profile->cumulativeAckTimeout();
          cumulativeAckTimer.status=TIMER_RUN;
        }
      }
      cumulativeAckTimer.timerLock.unlock();
      cumulativeAckTimer.interval=profile->cumulativeAckTimeout();
      if(!cumulativeAckTimer.started)
      {
        boost::thread(cumulativeAckTimerFunc,this);
        cumulativeAckTimer.started=true;
      }
    }
    ReliableFragmentList::iterator it1 = inSeqQueue.end();
    --it1;
    ReliableFragment &s1 = *it1;
    logDebug("MSG", "REL","Last fragment Id [%d] in in-sequence queue from [%d - %d]  ", s1.seq(),destination.getNode(),destination.getPort());
    if(s1.isLastFragment())
    {
      sendSingleAck();
      logDebug("MSG", "REL","Notify to read message");
      recvQueueCond.notify_one();
    }
    recvQueueLock.unlock();
  }

  void MsgSocketReliable::checkRecvQueues()
  {

    ReliableFragmentList::iterator it = outOfSeqQueue.begin();
    bool haveLastFrag=false;
    while (it != outOfSeqQueue.end() )
    {
      ReliableFragment &s = *it;
      if (compareFragment(s.seq(), nextSequenceNumber(queueInfo.getLastInSequence())) == 0)
      {
        queueInfo.setLastInSequence(s.seq());
        it = outOfSeqQueue.erase(it);
        if (s.getType() == fragmentType::FRAG_DATA ||
            s.getType() == fragmentType::FRAG_RST ||
            s.getType() == fragmentType::FRAG_FIN)
        {
          logDebug("MSG", "REL","Move fragment [%d] to in sequence list ",s.seq());
          inSeqQueue.push_back(s);
          if(s.isLastFragment())
          {
            haveLastFrag=true;
          }
        }
      }
      else
      {
        it++;
      }
    }
    if(haveLastFrag)
    {
      sendSingleAck();
      logDebug("MSG", "REL","Notify to read message");
      recvQueueCond.notify_one();
    }
  }
  void MsgSocketReliable::handleSYNReliableFragment(SYNFragment *frag)
  {
    try
    {
      logDebug("MSG", "REL","Handle SYNC fragment [%d]",frag->seq());
      switch (state)
      {
        case connectionState::CONN_CLOSED:
        {
          queueInfo.setLastInSequence(frag->seq());
          state = connectionState::CONN_SYN_RCVD;
          logDebug("MSG", "RST","Socket([%d]-[%d]) : State CONN_CLOSED. set state to CONN_SYN_RCVD. Send sync frag to [%d] - [%d]",sock->handle().getNode(),sock->handle().getPort(), destination.getNode(),destination.getPort());
          ReliableSocketProfile* _profile = new ReliableSocketProfile(
              sendQueueSize,
              recvQueueSize,
              frag->getMaxFragmentSize(),
              frag->getMaxOutstandingFragments(),
              frag->getMaxRetransmissions(),
              frag->getMaxCumulativeAcks(),
              frag->getMaxOutOfSequence(),
              frag->getMaxAutoReset(),
              frag->getNulFragmentTimeout(),
              frag->getRetransmissionTimeout(),
              frag->getCummulativeAckTimeout());
          profile = _profile;
          SYNFragment* synFrag = new SYNFragment(
              queueInfo.setSequenceNumber(0),
              _profile->maxOutstandingSegs(),
              _profile->maxFragmentSize(),
              _profile->retransmissionTimeout(),
              _profile->cumulativeAckTimeout(),
              _profile->nullFragmentTimeout(),
              _profile->maxRetrans(),
              _profile->maxCumulativeAcks(),
              _profile->maxOutOfSequence(),
              _profile->maxAutoReset());
          synFrag->setAck(frag->seq());
          queueAndSendReliableFragment(synFrag);
          delete _profile;
          break;
        }
        case connectionState::CONN_SYN_SENT:
        {
          logDebug("MSG", "RST","current connection([%d]-[%d]) state : CONN_CLOSED. Set connection to established",sock->handle().getNode(),sock->handle().getPort());
          queueInfo.setLastInSequence(frag->seq());
          state = connectionState::CONN_ESTABLISHED;
          /*
           * Here the client accepts or rejects the parameters sent by the
           * server. For now we will accept them.
           */
          this->sendACK();
          this->setconnection(connectionNotification::OPENED);
          break;
        }
        default:
        {
          logDebug("MSG", "RST","Connection state : Error");
          break;
        }
      }
      delete frag;
    }
    catch (...)
    {
      // TO DO EXCEPTION
    }
  }
  void MsgSocketReliable::handleACKReliableFragment(ReliableFragment *frag)
  {

  }
  void MsgSocketReliable::handleNAKReliableFragment(NAKFragment *frag)
  {
    int length = 0;
    bool bFound = false;
    int* acks = frag->getACKs(&length);
    int lastInSequence = frag->getAck();
    int lastOutSequence = acks[length -1];
    unackedSentQueueLock.lock();
    logDebug("MSG", "RST","Handle NAK fragment [%d] with length [%d]",frag->seq(),length);
    /* Removed acknowledged fragments from unackedSent queue */
    ReliableFragmentList::iterator it = unackedSentQueue.begin();
    while(it != unackedSentQueue.end())
    {
      ReliableFragment& s = *it;
      if ((compareFragment(s.seq(), lastInSequence) <= 0))
      {
        it = unackedSentQueue.erase_and_dispose(it, delete_disposer());
        continue;
      }
      bFound = false;
      for (int i = 0; i < length; i++)
      {
        if ((compareFragment(s.seq(), acks[i]) == 0))
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
    logDebug("MSG", "RST","Handle NAK fragment retransmit Fragment lastInSequence[%d] lastOutSequence[%d]",lastInSequence,lastOutSequence);
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
        iter != unackedSentQueue.end();
        iter++ )
    {
      ReliableFragment& s = *iter;
      logDebug("MSG", "RST","Check fragment [%d] %d %d ",s.seq(),compareFragment(lastInSequence, s.seq()),compareFragment(lastOutSequence, s.seq()));
      if ((compareFragment(lastInSequence, s.seq()) < 0) &&
          (compareFragment(lastOutSequence, s.seq()) > 0))
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

  void MsgSocketReliable::sendACK()
  {
    if (!outOfSeqQueue.empty() )
    {
      sendNAK();
    }
    else
    {
      sendSingleAck();
    }
  }
  void MsgSocketReliable::sendNAK()
  {
    logDebug("MSG", "RST","Send NAK fragment to Node [%d]",destination.getNode());
    if (outOfSeqQueue.empty())
    {
      return;
    }
    queueInfo.getAndResetCumulativeAckCounter();
    queueInfo.getAndResetOutOfSequenceCounter();

    /* Compose list of out-of-sequence sequence numbers */
    int size = outOfSeqQueue.size();
    int* acks = new int[size];
    int nIdx = 0;
    for (ReliableFragmentList::iterator it = outOfSeqQueue.begin();
        it != outOfSeqQueue.end();
        it++)
    {
      ReliableFragment& s = *it;
      acks[nIdx++] = s.seq();
      logDebug("MSG", "RST","NAK fragment [%d]",s.seq());
    }
    try
    {
      int lastInSequence = queueInfo.getLastInSequence();
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

  void MsgSocketReliable::sendSingleAck()
  {
    if (queueInfo.getAndResetCumulativeAckCounter() == 0)
    {
      return;
    }
    try
    {
      int lastInSequence = queueInfo.getLastInSequence();
      logDebug("MSG", "RST","Socket([%d - %d]) : Send single ACK [%d] to Node [%d] port [%d]",sock->handle().getNode(),sock->handle().getPort(),lastInSequence,destination.getNode(),destination.getPort());
      ReliableFragment *fragment=new ACKFragment(nextSequenceNumber(lastInSequence), lastInSequence);
      this->sendReliableFragment(fragment);
      delete fragment;
    }
    catch (...)
    {
      // Handle Exception
    }
  }

  int MsgSocketReliable::nextSequenceNumber(int seqn)
  {
    return (seqn + 1) % MAX_FRAGMENT_NUMBER;
  }

  void MsgSocketReliable::sendSYN()
  {
    //Todo
  }
  void MsgSocketReliable::setACK(ReliableFragment *frag)
  {
    if (queueInfo.getAndResetCumulativeAckCounter() == 0)
    {
      return;
    }
    frag->setAck(queueInfo.getLastInSequence());
  }

  void MsgSocketReliable::getACK(ReliableFragment *frag)
  {
    int ackn = frag->getAck();
    logTrace("MSG","RST","Socket([%d - %d]) : ack : [%d]",sock->handle().getNode(),sock->handle().getPort(),ackn);
    if (ackn < 0)
    {
      logDebug("MSG","RST","Ack < 0. Return ");
      return;
    }
    queueInfo.getAndResetOutstandingFragmentsCounter();
    if (state == connectionState::CONN_SYN_RCVD)
    {
      logDebug("MSG","RST","Socket([%d - %d]) : Current state CONN_SYN_RCVD. set to CONN_ESTABLISHED",sock->handle().getNode(),sock->handle().getPort());
      state = connectionState::CONN_ESTABLISHED;
      setconnection(connectionNotification::OPENED);
    }
    unackedSentQueueLock.lock();
    for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
        iter != unackedSentQueue.end();
    )
    {
      ReliableFragment &fragment = *iter;
      if(compareFragment(fragment.seq(), ackn) <= 0)
      {
        logTrace("MSG","RST","Socket([%d - %d]) : UnackedSentQueue delete Fragment [%d]",sock->handle().getNode(),sock->handle().getPort(),fragment.seq());
        iter = unackedSentQueue.erase_and_dispose(iter, delete_disposer());
      }
      else
      {
        iter++;
      }
    }
    if(unackedSentQueue.empty())
    {
      logTrace("MSG","RST","unackedSentQueue is empty. Pause retransmissionTimer timer ");
      retransmissionTimer.status=TIMER_PAUSE;
    }
    unackedSentQueueCond.notify_all();
    unackedSentQueueLock.unlock();
  }

  //send a reliable fragment
  void MsgSocketReliable::sendReliableFragment(ReliableFragment *frag)
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
    if(sock==NULL)
    {
      return;
    }
    if(sock->msgPool==NULL)
    {
      return;
    }
    Message* reliableFragment;
    reliableFragment=sock->msgPool->allocMsg();
    assert(reliableFragment);
    reliableFragment->setAddress(destination);
    MsgFragment* fragment = reliableFragment->append(0);
    Byte *pBuffer= (Byte*)frag->getBytes();
    fragment->set(pBuffer,frag->length());
    logTrace("MSG","RST","Send Fragment to node [%d] port [%d] Id [%d] ",reliableFragment->getAddress().getNode(),reliableFragment->getAddress().getPort(),frag->seq());
    sock->send(reliableFragment);
    if(pBuffer)
    {
      delete pBuffer;
    }
  }

  // send fragment and store it in uncheck ack
  void MsgSocketReliable::queueAndSendReliableFragment(ReliableFragment* frag)
  {
    unackedSentQueueLock.lock();
    while ((unackedSentQueue.size() >= sendQueueSize) ||
        ( queueInfo.getOutstandingFragmentsCounter() > profile->maxOutstandingSegs()))
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
    queueInfo.incOutstandingFragmentsCounter();
    logTrace("MSG","RST","Socket([%d - %d]) : Store Fragment [%d] to unackedSent Queue",sock->handle().getNode(),sock->handle().getPort(),frag->seq());
    unackedSentQueue.push_back(*frag);
    unackedSentQueueLock.unlock();
    if (isClosed)
    {
      // Hanlde Exception
      //print log and exit
      return;
    }
    /* Re-start retransmission timer */
    if (!(frag->getType() == fragmentType::FRAG_NAK) && !(frag->getType() == fragmentType::FRAG_ACK))
    {
      if (retransmissionTimer.status==TIMER_PAUSE)
      {
        retransmissionTimer.interval=profile->retransmissionTimeout();
        retransmissionTimer.status=TIMER_RUN;
        if(!retransmissionTimer.started)
        {
          logDebug("MSG","RST","Socket : Start retransmission timer [%d]",retransmissionTimer.status);
          boost::thread(retransmissionTimerFunc,this);
          retransmissionTimer.started=true;
        }
      }
    }
    sendReliableFragment(frag);
  }

  Message* MsgSocketReliable::receive(uint_t maxMsgs,int maxDelay)
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
     Byte* buffer = (Byte*)SAFplusHeapAlloc(maxFragmentSize);
     ReliableFragmentList::iterator it = inSeqQueue.begin();
     while(it != inSeqQueue.end())
     {
       ReliableFragment& s = *it;
       buffer=(Byte*)SAFplusHeapRealloc(buffer,totalBytes + ((DATFragment*) &s)->length());
       logDebug("MSG","RST","read the fragment [%d]",s.seq());
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
           return nullptr; /* EOF */
         }
       }
       else if (s.getType() == fragmentType::FRAG_DATA)
       {
         int length = 0;
         Byte* data = ((DATFragment*) &s)->getData();
         length = ((DATFragment*) &s)->length();
         if(s.isLastFragment())
         {
           quit=true;
           address=s.address;
           logDebug("MSG","RST","read the last fragment [%d]",s.seq());
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
       Message* m = sock->msgPool->allocMsg();
       assert(m);
       logTrace("MSG","MSS","Set Address [%d] - [%d] ",address.getNode(),address.getPort());
       m->setAddress(address);
       MsgFragment* frag = m->append(0);
       frag->set(buffer,totalBytes);
       return m;
     }
     recvQueueLock.unlock();
     return nullptr;
  }

  ReliableFragment* MsgSocketReliable::receiveReliableFragment(Handle &handle)
  {
    ReliableFragment* p_Fragment;
    Message* p_Msg = nullptr;
    MsgFragment* p_NextFrag = nullptr;
    int iMaxMsg = 1;
    int iDelay = 1;

    p_Msg = this->sock->receive( iMaxMsg, iDelay);
    if(p_Msg == nullptr)
    {
      return nullptr;

    }
    handle = p_Msg->getAddress();
    p_NextFrag = p_Msg->firstFragment;
    if(p_NextFrag == nullptr)
    {
      return nullptr;
    }
    p_Fragment = ReliableFragment::parse((Byte*)p_NextFrag->read(0), 0, p_NextFrag->len);
    p_Fragment->address=p_Msg->getAddress();
    fragmentType fragType = p_Fragment->getType();
    if(fragType == fragmentType::FRAG_DATA ||
        fragType == fragmentType::FRAG_NUL ||
        fragType == fragmentType::FRAG_RST ||
        fragType == fragmentType::FRAG_FIN ||
        fragType == fragmentType::FRAG_SYN)
    {
      queueInfo.increaseCumulativeAckCounter();
      logTrace("MSG","RST","Inscrease Cumulative counter [%d]",queueInfo.getCumulativeAckCounter());
    }

    if(iskeepAlive)
    {
      // Reset keepAliveTimer
      // keepAliveTimer
    }
    p_Msg->msgPool->free(p_Msg);
    return p_Fragment;
  }

  void MsgSocketReliable::retransmitFragment(ReliableFragment *frag)
  {
    logDebug("MSG","RST","Socket([%d - %d]) : Retransmit Fragment fradId [%d] to node [%d]",sock->handle().getNode(),sock->handle().getPort(), frag->seq(),destination.getNode());
    if (profile->maxRetrans() > 0)
    {
      frag->setRetxCounter(frag->getRetxCounter()+1);
    }
    if (profile->maxRetrans() != 0 && frag->getRetxCounter() > profile->maxRetrans())
    {
      setconnection(connectionNotification::FAILURE);
      return ;
    }
    assert(sock);
    Message* reliableFragment;
    reliableFragment=sock->msgPool->allocMsg();
    assert(reliableFragment);
    reliableFragment->setAddress(destination);
    MsgFragment* fragment = reliableFragment->append(0);
    Byte* buffer = (Byte*)frag->getBytes();
    fragment->set(buffer,frag->length());
    sock->send(reliableFragment);
    delete buffer;
  }

  void MsgSocketReliable::connectionFailure()
  {
    logDebug("MSG","RST","Socket([%d - %d]) : Set connection to Failure",sock->handle().getNode(),sock->handle().getPort());
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
//          recvQueueLock.lock();
//          {
//            recvQueueCond.notify_one();
//          }
//          recvQueueLock.unlock();
          closeImpl();
          break;
        }
      }
      state = connectionState::CONN_CLOSED;
      isClosed = true;
    }
    closeMutex.unlock();
  }


  void MsgSocketReliable::closeImplThread(void* arg)
  {
    MsgSocketReliable* p_this = (MsgSocketReliable*) arg;
    p_this->handleCloseImpl();
  }

  void MsgSocketReliable::handleCloseImpl(void)
  {
    //keepAliveTimer->destroy();
    keepAliveTimer.isStop=true;
    nullFragmentTimer.isStop=true;
    try
    {
      boost::this_thread::sleep_for(boost::chrono::seconds{profile->nullFragmentTimeout() * 2});
    }
    catch (...)
    {
      // Hanlde Exception
    }
    retransmissionTimer.isStop=true;
    cumulativeAckTimer.isStop=true;
    // Close UDP socket.
    // TO DO

    setconnection(connectionNotification::CLOSE);
  }

  void MsgSocketReliable::closeImpl()
  {
    nullFragmentTimer.status=TIMER_PAUSE;
    keepAliveTimer.status=TIMER_PAUSE;
    state = connectionState::CONN_CLOSE_WAIT;
    boost::thread closeThread = boost::thread(MsgSocketReliable::closeImplThread, this);
  }
  void MsgSocketReliable::setconnection(connectionNotification state)
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

  void MsgSocketReliable::connectionOpened(void)
  {
    logDebug("MSG","RST","Socket([%d - %d]) : set connection to Opened",sock->handle().getNode(),sock->handle().getPort());
    if(isConnected)
    {
      nullFragmentTimer.status = TIMER_RUN;
      nullFragmentTimer.interval=profile->nullFragmentTimeout();
      boost::thread(nullFragmentTimerFunc,this);
      if(iskeepAlive)
      {
        keepAliveTimer.status = TIMER_RUN;
        boost::thread(keepAliveTimerFunc,this);
      }
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
    MsgSocketReliable* p_this = (MsgSocketReliable*)arg;
    p_this->handleReliableSocketThread();
  }

  MsgSocketReliable::MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport)
  {
    sock=transport->createSocket(port);
    rcvThread = boost::thread(ReliableSocketThread, this);
  }
  MsgSocketReliable::MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport,Handle destination)
  {
    sock=transport->createSocket(port);
    init();
    connect(destination,0);
  }
  MsgSocketReliable::MsgSocketReliable(MsgSocket* socket)
  {
    sock=socket;
    init();
  }
  void MsgSocketReliable::init()
  {
    queueInfo.fragNumber =0;
    queueInfo.lastFrag=0;
    queueInfo.numberOfCumAck=0;
    queueInfo.numberOfOutOfSeq=0;
    queueInfo.numberOfNakFrag=0; /* Outstanding segments counter */
    nullFragmentTimer.status=TIMER_PAUSE;
    retransmissionTimer.status=TIMER_PAUSE;
    cumulativeAckTimer.status=TIMER_PAUSE;
    keepAliveTimer.status=TIMER_PAUSE;
    nullFragmentTimer.started=false;
    retransmissionTimer.started=false;
    cumulativeAckTimer.started=false;
    keepAliveTimer.started=false;
    profile = new ReliableSocketProfile();
    rcvThread = boost::thread(ReliableSocketThread, this);
  }

  void MsgSocketReliable::handleReliableSocketThread(void)
  {
    fragmentType fragType = fragmentType::FRAG_UDE;
    Handle handle;
    logDebug("MSG","RST"," Socket([%d - %d]) : start thread to receive and handle fragment from [%d] [%d].",sock->handle().getNode(),sock->handle().getPort(),destination.getNode(),destination.getPort());
    int test=0;
    while (1)
    {
      test++;
      ReliableFragment* pFrag =receiveReliableFragment(handle);
      if(pFrag==nullptr)
      {
        logDebug("MSG","RST","Receive NULL fragment . Exit thread");
        return;
      }
      fragType = pFrag->getType();

      if (fragType == fragmentType::FRAG_SYN)
      {
        this->destination = handle;
        handleSYNReliableFragment((SYNFragment*)pFrag);
      }
      else if (fragType == fragmentType::FRAG_NAK)
      {
        handleNAKReliableFragment((NAKFragment*)pFrag);
      }
      else if (fragType == fragmentType::FRAG_ACK)
      {
        logTrace("MSG","RST","Receive ACK fragment . Do nothing");
      }
      else
      {
// For testing
//        if((test % 6)==0)
//        {
//          logDebug("MSG","RST","remove frag to test fragment missing");
//          continue;
//        }
//        else
//        {
//          logDebug("MSG","RST","Socket([%d - %d]) : Receive reliable fragment Id [%d] fragment type [%d] from [%d - %d]",sock->handle().getNode(),sock->handle().getPort(),pFrag->seq(),fragType,handle.getNode(),handle.getPort());
//          handleReliableFragment(pFrag);
//        }
          logDebug("MSG","RST","Socket([%d - %d]) : Receive reliable fragment Id [%d] fragment type [%d] from [%d - %d]",sock->handle().getNode(),sock->handle().getPort(),pFrag->seq(),fragType,handle.getNode(),handle.getPort());
          handleReliableFragment(pFrag);
      }
      getACK(pFrag);
    }
    logDebug("MSG","RST","Receive NULL fragment . Exit thread");
  }

  MsgSocketReliable::~MsgSocketReliable()
  {
    //TODO
  }

  void MsgSocketReliable::send(Message* origMsg)
  {
    Message* msg;
    Message* next = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    int bufferLen;
    do
    {
      msg = next;
      next = msg->nextMsg;  // Save the next message so we use it next
      bufferLen=0;
      nextFrag = msg->firstFragment;
      Byte* buffer = new Byte[msg->getLength()];
      do
      {
        frag = nextFrag;
        nextFrag = frag->nextFragment;
        memcpy(buffer + bufferLen ,(void*)frag->data(),frag->len);
        assert((frag->len > 0) && "The UDP protocol allows sending zero length messages but I think you forgot to set the fragment len field.");
        bufferLen+=frag->len;
      }while(nextFrag);
      Handle destination = getProcessHandle(msg->port,msg->node);
      writeReliable(buffer,0,bufferLen);
      delete buffer;
    } while (next != NULL);
  }
  void MsgSocketReliable::writeReliable(Byte* buffer, int offset, int len)
  {
    logDebug("MSG","RST","Send buffer with len [%d] to  node [%d] in reliable mode ",len,destination.getNode());
    if (isClosed)
    {
      throw new Error("Socket is closed");
    }

    if (!isConnected)
    {
      throw new Error("Connection reset");
    }

    int totalBytes = 0;
    int off = 0;
    int writeBytes = 0;
    while(totalBytes<len)
    {
      writeBytes = MIN(profile->maxFragmentSize() - RUDP_HEADER_LEN,len - totalBytes);
      logTrace("MSG","RST","sending [%d] byte",writeBytes);
      if(totalBytes+writeBytes<len)
      {
        ReliableFragment *frag = new DATFragment(queueInfo.nextSequenceNumber(),
            queueInfo.getLastInSequence(), buffer, off + totalBytes, writeBytes,false);
        logTrace("MSG","RST","Socket([%d - %d]) : Create fragment with seq [%d] type [%d] ack [%d].",sock->handle().getNode(),sock->handle().getPort(),frag->seq(),frag->getType(),frag->getAck());
        queueAndSendReliableFragment(frag);
        totalBytes += writeBytes;
      }
      else
      {
        logDebug("MSG", "REL","send last fragment to  node [%d] in reliable mode ",destination.getNode());
        queueAndSendReliableFragment(new DATFragment(queueInfo.nextSequenceNumber(),
            queueInfo.getLastInSequence(), buffer, off + totalBytes, writeBytes,true));
        totalBytes += writeBytes;
      }
    }
  }

  void MsgSocketReliable::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    // TODO
  }

  Byte* MsgSocketReliable::readReliable(int offset, int &totalBytes, int len)
  {
    logDebug("MSG","RST","receive buffer from [%d - %d] in reliable mode ",destination.getNode(),destination.getPort());
    totalBytes = 0;
    bool quit=false;
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
    Byte* buffer = (Byte*)SAFplusHeapAlloc(maxFragmentSize);
    ReliableFragmentList::iterator it = inSeqQueue.begin();
    while(it != inSeqQueue.end())
    {
      ReliableFragment& s = *it;
      buffer=(Byte*)SAFplusHeapRealloc(buffer,totalBytes + ((DATFragment*) &s)->length());
      logDebug("MSG","RST","read the fragment [%d]",s.seq());
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
          return nullptr; /* EOF */
        }
      }
      else if (s.getType() == fragmentType::FRAG_DATA)
      {
        int length = 0;
        Byte* data = ((DATFragment*) &s)->getData();
        length = ((DATFragment*) &s)->length();
        if(totalBytes + length > len)
        {
          if (totalBytes <= 0)
          {
            throw new Error("insufficient buffer space");
          }
        }
        if(s.isLastFragment())
        {
          quit=true;
          logDebug("MSG","RST","read the last fragment [%d]",s.seq());
        }
        memcpy(buffer + (offset+totalBytes), (void*)data , length);
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
      return buffer;
    }
    recvQueueLock.unlock();
    return buffer;
  }

  void MsgSocketReliable::connect(Handle destination, int timeout)
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
      throw new Error("already connected");
    }
    this->destination = destination;
    // Synchronize sequence numbers
    state = connectionState::CONN_SYN_SENT;
    int sequenceNum=queueInfo.setSequenceNumber(0);
    ReliableFragment *frag = new SYNFragment(sequenceNum,
        profile->maxOutstandingSegs(), profile->maxFragmentSize(),
        profile->retransmissionTimeout(), profile->cumulativeAckTimeout(),
        profile->nullFragmentTimeout(), profile->maxRetrans(),
        profile->maxCumulativeAcks(), profile->maxOutOfSequence(),
        profile->maxAutoReset());

    logTrace("MSG","RST","Socket([%d - %d]) : Create syn fragment with folow parameter [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d].",sock->handle().getNode(),sock->handle().getPort(),sequenceNum,
        profile->maxOutstandingSegs(), profile->maxFragmentSize(),
        profile->retransmissionTimeout(), profile->cumulativeAckTimeout(),
        profile->nullFragmentTimeout(), profile->maxRetrans(),
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
          thisCond.wait(thisMutex);
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

    queueInfo.reset();
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

  void MsgSocketReliable::close()
  {
    closeMutex.lock();
    {
      if (isClosed)
      {
        closeMutex.unlock();
        return;
      }
      try
      {
        // Runtime.getRuntime().removeShutdownHook(_shutdownHook);
      }
      catch (...)
      {
        // Handle Exception
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
          sendReliableFragment(new FINFragment(queueInfo.nextSequenceNumber()));
          closeImpl();
          break;
        }
        case connectionState::CONN_CLOSED:
        {
          retransmissionTimer.isStop = true;
          cumulativeAckTimer.isStop = true;
          keepAliveTimer.isStop = true;
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
  void MsgSocketReliable::flush()
  {
   sock->flush();
  }
  void MsgSocketReliable::connectionClientOpen()
  {
  };

  void MsgSocketServerReliable::addClientSocket(Handle destAddress)
  {
    if (clientSockTable[destAddress] == nullptr)
    {
      try
      {
        MsgSocketClientReliable *sockClient = new MsgSocketClientReliable(sock,destAddress);
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

  void MsgSocketServerReliable::removeClientSocket(Handle destAddress)
  {
    clientSockTable.erase(destAddress);
    if (clientSockTable.empty())
    {
      if (isClosed==true)
      {
        delete sock;
      }
    }
  }

  void MsgSocketServerReliable::handleRcvThread()
  {
    Message* p_Msg = nullptr;
    int iMaxMsg = 1;
    int iDelay = 1;
    Handle destinationAddress;
    MsgFragment* p_NextFrag = nullptr;
    logDebug("MSG","SST","Socket server ([%d - %d]) : Start server receive thread.",sock->handle().getNode(),sock->handle().getPort());
    while (true)
    {
      try
      {
        p_Msg=this->sock->receive(iMaxMsg,iDelay);
        destinationAddress = p_Msg->getAddress();
        ReliableFragment* p_Fragment = nullptr;
        p_NextFrag = p_Msg->firstFragment;
        if(p_NextFrag == nullptr)
        {
          logDebug("MSG","SST","Socket server ([%d - %d]) : Receive fragment null. Return",sock->handle().getNode(),sock->handle().getPort());
          return;
        }
        p_Fragment = ReliableFragment::parse((Byte*)p_NextFrag->read(0), 0, p_NextFrag->len);
        p_Fragment->address=p_Msg->getAddress();
        logDebug("MSG","SST","Socket server ([%d - %d]) : Receive fragment from [%d] - [%d] with seq [%d]",sock->handle().getNode(),sock->handle().getPort(),destinationAddress.getNode(),destinationAddress.getPort(),p_Fragment->seq());
        if(!isClosed)
        {
          if (p_Fragment->getType()==FRAG_SYN)
          {
            logDebug("MSG","SST","Socket server : Receive SYN fragment from [%d] - [%d] .",destinationAddress.getNode(),destinationAddress.getPort());
            if(clientSockTable[destinationAddress]==nullptr)
            {
              logDebug("MSG","SST","Socket server Create socket client and add to client Table");
              addClientSocket(destinationAddress);
            }
          }
        }
        if(clientSockTable[destinationAddress]!=nullptr)
        {
            MsgSocketClientReliable *socket = clientSockTable[destinationAddress];
            socket->receiverFragment(p_Fragment);
        }
        p_Msg->msgPool->free(p_Msg);
      }catch (...)
      {

      }
    }
  }
  void MsgSocketServerReliable::rcvThread(void * arg)
  {
    MsgSocketServerReliable* p_this = (MsgSocketServerReliable*)arg;
    p_this->handleRcvThread();
  }
  MsgSocketServerReliable::MsgSocketServerReliable(uint_t port,MsgTransportPlugin_1* transport)
  {
    logDebug("MSG","SST","Init sock server. ");
    sock=transport->createSocket(port);
    ServerRcvThread = boost::thread(rcvThread, this);
  }
  MsgSocketServerReliable::MsgSocketServerReliable(MsgSocket* socket)
  {
    sock=socket;
    ServerRcvThread = boost::thread(rcvThread, this);
  }
  void MsgSocketServerReliable::init()
  {
    ServerRcvThread.start_thread();
  }

  MsgSocketClientReliable* MsgSocketServerReliable::accept()
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
    MsgSocketClientReliable* sock = (MsgSocketClientReliable*)&(listenSock.front());
    logDebug("MSG","SST", "Get Socket from listenSock list.");
    listenSock.pop_front();
    return sock;
  }

  ReliableFragment* MsgSocketClientReliable::receiveReliableFragment(Handle &handle)
  {
    this->sockServer->fragmentQueueLock.lock();
    while (sockServer->fragmentQueue.empty())
    {
      try
      {
        sockServer->fragmentQueueCond.wait(sockServer->fragmentQueueLock);
      }
      catch(...)
      {
        // Handle Exception
      }
    }

    ReliableFragment* p_Fragment=&sockServer->fragmentQueue.front();
    handle=p_Fragment->address;
    sockServer->fragmentQueue.pop_front();
    sockServer->fragmentQueueLock.unlock();
    if(p_Fragment->getType() == fragmentType::FRAG_DATA ||
        p_Fragment->getType() == fragmentType::FRAG_NUL ||
        p_Fragment->getType() == fragmentType::FRAG_RST ||
        p_Fragment->getType() == fragmentType::FRAG_FIN ||
        p_Fragment->getType() == fragmentType::FRAG_SYN)
    {
      queueInfo.increaseCumulativeAckCounter();
      logDebug("MSG","RST","Inscrease Cumulative counter [%d]",queueInfo.getCumulativeAckCounter());
    }
    return p_Fragment;
  }

  void MsgSocketClientReliable::receiverFragment(ReliableFragment* frag)
  {
    sockServer->fragmentQueueLock.lock();
    sockServer->fragmentQueue.push_back(*frag);
    sockServer->fragmentQueueCond.notify_one();
    sockServer->fragmentQueueLock.unlock();
  }
  void MsgSocketClientReliable::connectionClientOpen()
  {
    logDebug("MSG", "CST","Socket client :  Add socket to listenList");
    sockServer->listenSockMutex.lock();
    sockServer->listenSock.push_back(*this);
    sockServer->listenSockCond.notify_one();
    sockServer->listenSockMutex.unlock();
  }
};
