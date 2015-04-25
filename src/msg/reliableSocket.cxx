#include <reliableSocket.hxx>

#include <stdio.h>
namespace SAFplus
{
  static void * nullFragmentTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    while(tempMsgSocket->nullFragmentTimer.isStop)
    {
      sleep(tempMsgSocket->nullFragmentTimer.interval);
      if(tempMsgSocket->nullFragmentTimer.status==TIMER_RUN)
      {
          tempMsgSocket->nullFragmentTimer.timerLock.lock();
          tempMsgSocket->handleNullFragmentTimerFunc();
          tempMsgSocket->nullFragmentTimer.timerLock.unlock();
      }
    }
    return NULL;
  }

  static void* retransmissionTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    while(tempMsgSocket->retransmissionTimer.isStop)
    {
      sleep(tempMsgSocket->retransmissionTimer.interval);
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

  static void* cumulativeAckTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    while(tempMsgSocket->cumulativeAckTimer.isStop)
    {
      sleep(tempMsgSocket->cumulativeAckTimer.interval);
      if(tempMsgSocket->cumulativeAckTimer.status==TIMER_RUN)
      {
        tempMsgSocket->cumulativeAckTimer.timerLock.lock();
        tempMsgSocket->handleCumulativeAckTimerFunc();
        tempMsgSocket->cumulativeAckTimer.timerLock.unlock();
      }
    }
    return NULL;
  }




  static void* keepAliveTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    while(tempMsgSocket->keepAliveTimer.isStop)
    {
      sleep(tempMsgSocket->keepAliveTimer.interval);
      if(tempMsgSocket->keepAliveTimer.status==TIMER_RUN)
      {
        tempMsgSocket->keepAliveTimer.timerLock.lock();
        tempMsgSocket->handleKeepAliveTimerFunc();
        tempMsgSocket->keepAliveTimer.timerLock.unlock();
      }
    }
    return NULL;
  }



  void MsgSocketReliable::handleRetransmissionTimerFunc(void)
  {
     unackedSentQueueLock.lock();
     {
        for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
              iter != unackedSentQueue.end();
              iter++)
        {
           ReliableFragment &frag = *iter;
           try
           {
              retransmitSegment(&frag);
           }
           catch(...)
           {
              // Handle Exception
           }
        }
     }
  }

  void MsgSocketReliable::handleNullFragmentTimerFunc(void)
  {
     unackedSentQueueLock.lock();
     {
        if (unackedSentQueue.empty())
        {
           try
           {
              queueAndSendReliableFragment(new NULLFragment(queueInfo->nextSequenceNumber()));
           }
           catch (...)
           {
              // Hanlde Exception
           }
        }
     }
     unackedSentQueueLock.unlock();
  }

  void MsgSocketReliable::handleCumulativeAckTimerFunc(void)
  {
    this->sendACK();
  }

  void MsgSocketReliable::handleKeepAliveTimerFunc(void)
  {
     this->setconnection(connectionNotification::FAILURE);
  }

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
     /*
      * When a RST segment is received, the sender must stop
      * sending new packets, but most continue to attempt
      * delivery of packets already accepted from the application.
      */
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
//              this->lock();
//              {
//                 this->notify_one();
//              }
//              this->unlock();
              break;
           }
           case connectionState::CONN_CLOSED:
           {
              break;
           }
           default:
           {
              state = connectionState::CONN_CLOSE_WAIT;
           }
        }
     }
     bool bIsInSequence = false;
     recvQueueLock.lock();
     if(compareFragment(frag->seq(), queueInfo->getLastInSequence() ) <= 0 )
     {
        /* Drop packet: duplicate. */
     }
     else if(compareFragment(frag->seq(), nextSequenceNumber(queueInfo->getLastInSequence()) ) <= 0 )
     {
        bIsInSequence = true;
        if (inSeqRecvQueue.size() == 0 ||
              (inSeqRecvQueue.size() + outSeqRecvQueue.size() < recvQueueSize))
        {
           /* Insert in-sequence segment */
           queueInfo->setLastInSequence(frag->seq());
           if ( fragType == fragmentType::FRAG_DATA ||
                 fragType ==  fragmentType::FRAG_RST ||
                 fragType == fragmentType::FRAG_FIN)
           {
              inSeqRecvQueue.push_back(*frag);
           }
           checkRecvQueues();
        }
        else
        {
           /* Drop packet: queue is full. */
        }
     }
     else if(inSeqRecvQueue.size() + outSeqRecvQueue.size() < recvQueueSize)
     {
        /* Insert out-of-sequence segment, in order */
        bool added = false;
        for (ReliableFragmentList::iterator iter = outSeqRecvQueue.begin();
              iter != outSeqRecvQueue.end() && !added;
              iter ++)
        {
            ReliableFragment &s = *iter;
            int cmp = compareFragment(frag->seq(), s.seq());
            if (cmp == 0)
            {
                /* Ignore duplicate packet */
                added = true;
            }
            else if (cmp < 0)
            {
                outSeqRecvQueue.insert(iter, *frag);
                added = true;
            }
        }

        if (!added)
        {
            outSeqRecvQueue.push_back(*frag);
        }

        queueInfo->incOutOfSequenceCounter();
     }

     if (bIsInSequence &&
           ( fragType == fragmentType::FRAG_RST ||
                 fragType == fragmentType::FRAG_NUL||
                 fragType == fragmentType::FRAG_FIN))
     {
        sendACK();
     }
     else if ( (queueInfo->getOutOfSequenceCounter() > 0) &&
           (profile->maxOutOfSequence() == 0 || queueInfo->getOutOfSequenceCounter() > profile->maxOutOfSequence()) )
     {
        sendNAK();
     }
     else if ((queueInfo->getCumulativeAckCounter() > 0) &&
           (profile->maxCumulativeAcks() == 0 || queueInfo->getCumulativeAckCounter() > profile->maxCumulativeAcks()))
     {
        sendSingleAck();
     }
     else
     {
        cumulativeAckTimer.timerLock.lock();
        {
           if (cumulativeAckTimer.status==TIMER_PAUSE)
           {
              cumulativeAckTimer.interval=profile->cumulativeAckTimeout();
           }
        }
        cumulativeAckTimer.timerLock.lock();
     }
     recvQueueLock.unlock();
  }

  void MsgSocketReliable::checkRecvQueues()
  {
     recvQueueLock.lock();
     {
        ReliableFragmentList::iterator it = outSeqRecvQueue.begin();
        while (it != outSeqRecvQueue.end() )
        {
           ReliableFragment &s = *it;
           if (compareFragment(s.seq(), nextSequenceNumber(queueInfo->getLastInSequence())) == 0)
           {
              queueInfo->setLastInSequence(s.seq());
              if (s.getType() == fragmentType::FRAG_DATA ||
                    s.getType() == fragmentType::FRAG_RST ||
                    s.getType() == fragmentType::FRAG_FIN)
              {
                 inSeqRecvQueue.push_back(s);
              }
              it = outSeqRecvQueue.erase_and_dispose(it, delete_disposer());
           }
           else
           {
              it++;
           }
        }

        recvQueueCond.notify_one();
     }
     recvQueueLock.unlock();
  }

  void MsgSocketReliable::handleSYNReliableFragment(SYNFragment *frag)
  {
     try
     {
        switch (state)
        {
           case connectionState::CONN_CLOSED:
           {
              queueInfo->setLastInSequence(frag->seq());
              state = connectionState::CONN_SYN_RCVD;
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
                    queueInfo->setSequenceNumber(0),
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
              break;
           }
           case connectionState::CONN_SYN_SENT:
           {
              queueInfo->setLastInSequence(frag->seq());
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
              break;
           }
         }
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
     Byte* acks = frag->getACKs(&length);
     int lastInSequence = frag->getAck();
     int lastOutSequence = acks[length -1];
     unackedSentQueueLock.lock();
     /* Removed acknowledged segments from sent queue */
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
     for(ReliableFragmentList::iterator iter = unackedSentQueue.begin();
           iter != unackedSentQueue.end();
           iter++ )
     {
        ReliableFragment& s = *iter;
        if ((compareFragment(lastInSequence, s.seq()) < 0) &&
              (compareFragment(lastOutSequence, s.seq()) > 0))
        {
           try
           {
              retransmitSegment(&s);
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
     recvQueueLock.lock();
     if (outSeqRecvQueue.empty() )
     {
        sendNAK();
     }
     else
     {
        sendSingleAck();
     }
     recvQueueLock.unlock();
  }
  void MsgSocketReliable::sendNAK()
  {
     recvQueueLock.lock();
     if (outSeqRecvQueue.empty())
     {
        return;
     }
     queueInfo->getAndResetCumulativeAckCounter();
     queueInfo->getAndResetOutOfSequenceCounter();

     /* Compose list of out-of-sequence sequence numbers */
     int size = outSeqRecvQueue.size();
     int* acks = new int[size];
     int nIdx = 0;
     for (ReliableFragmentList::iterator it = outSeqRecvQueue.begin();
           it != outSeqRecvQueue.end();
           it++)
     {
       ReliableFragment& s = *it;
       acks[nIdx++] = s.seq();
     }
     try
     {
       int lastInSequence = queueInfo->getLastInSequence();
       this->sendReliableFragment(new NAKFragment(nextSequenceNumber(lastInSequence), lastInSequence, acks, size));
     }
     catch (...)
     {
         // Handle Exception
     }
     recvQueueLock.unlock();
  }

  void MsgSocketReliable::sendSingleAck()
  {
    if (queueInfo->getAndResetCumulativeAckCounter() == 0)
    {
       return;
    }
    try
    {
        int lastInSequence = queueInfo->getLastInSequence();
        this->sendReliableFragment(new ACKFragment(nextSequenceNumber(lastInSequence), lastInSequence));
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

  }
  void MsgSocketReliable::setACK(ReliableFragment *frag)
  {
    if (queueInfo->getAndResetCumulativeAckCounter() == 0)
    {
      return;
    }
    frag->setAck(queueInfo->getLastInSequence());
  }

  void MsgSocketReliable::getACK(ReliableFragment *frag)
  {
     int ackn = frag->getAck();
     if (ackn < 0)
     {
        return;
     }
     queueInfo->getAndResetOutstandingSegsCounter();
     if (state == connectionState::CONN_SYN_RCVD)
     {
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
           iter = unackedSentQueue.erase_and_dispose(iter, delete_disposer());
        }
        else
        {
           iter++;
        }
     }
     if(unackedSentQueue.empty())
     {
        // Cancel Retransmisstion Timer
        retransmissionTimer.status==TIMER_PAUSE;
     }
     unackedSentQueueCond.notify_all();
     unackedSentQueueLock.unlock();
  }
  // reviewed
  void MsgSocketReliable::sendReliableFragment(ReliableFragment *frag)
  {
    /* Piggyback any pending acknowledgments */
    if (frag->getType()==fragmentType::FRAG_DATA || frag->getType()==fragmentType::FRAG_RST || frag->getType()==fragmentType::FRAG_FIN || frag->getType()==fragmentType::FRAG_NUL)
    {
    //TODO checkAndSetAck(s);
        setACK(frag);
    }
    /* Reset null segment timer */
    if (frag->getType()==fragmentType::FRAG_DATA || frag->getType()==fragmentType::FRAG_RST || frag->getType()==fragmentType::FRAG_FIN)
    {
      //TODO nullSegmentTimer.reset();
      nullFragmentTimer.status=TimerStatus::TIMER_PAUSE;
    }
    //send reliable fragment
    Message* reliableReliableFragment = sock->msgPool->allocMsg();
    assert(reliableReliableFragment);
    reliableReliableFragment->setAddress(destination);
    MsgFragment* pfx  = reliableReliableFragment->append(1);
    * ((unsigned char*)pfx->data()) = messageType;
    pfx->len = 1;
    MsgFragment* fragment = reliableReliableFragment->append(0);
    fragment->set((char*)frag,frag->length());
    sock->send(reliableReliableFragment);
  }
  void MsgSocketReliable::queueAndSendReliableFragment(ReliableFragment* frag)
  {
     unackedSentQueueLock.lock();
     while ((unackedSentQueue.size() >= sendQueueSize) ||
           ( queueInfo->getOutstandingSegsCounter() > profile->maxOutstandingSegs()))
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
     queueInfo->incOutstandingSegsCounter();
     unackedSentQueue.push_back(*frag);
     unackedSentQueueLock.unlock();

     if (isClosed)
     {
         // Hanlde Exception
         //throw new SocketException("Socket is closed");
     }
     /* Re-start retransmission timer */
     if (!(frag->getType() == fragmentType::FRAG_NAK) && !(frag->getType() == fragmentType::FRAG_ACK))
     {
        retransmissionTimer.timerLock.lock();
        if (retransmissionTimer.status==TIMER_PAUSE)
        {
           retransmissionTimer.interval=profile->retransmissionTimeout();
        }
        retransmissionTimer.timerLock.lock();
     }
     this->sendReliableFragment(frag);

     if (frag->getType() ==  fragmentType::FRAG_DATA)
     {
        // Listening data
     }
  }
  ReliableFragment MsgSocketReliable::receiveReliableFragment()
  {

  }
  void MsgSocketReliable::retransmitSegment(ReliableFragment *frag)
  {
    if (profile->maxRetrans() > 0)
    {
      frag->setRetxCounter(frag->getRetxCounter()+1);
    }
    if (profile->maxRetrans() != 0 && frag->getRetxCounter() > profile->maxRetrans())
    {
      setconnection(connectionNotification::FAILURE);
      return ;
    }

    Message* reliableReliableFragment = sock->msgPool->allocMsg();
    assert(reliableReliableFragment);
    reliableReliableFragment->setAddress(destination);
    MsgFragment* pfx  = reliableReliableFragment->append(1);
    * ((unsigned char*)pfx->data()) = messageType;
    pfx->len = 1;
    MsgFragment* fragment = reliableReliableFragment->append(0);
    fragment->set((char*)frag,frag->length());
    sock->send(reliableReliableFragment);
    if (frag->getType()== FRAG_DATA)
    {
    //TODO
    }
  }

  void MsgSocketReliable::connectionFailure()
  {
     closeMutex.lock();
     {
        if (isClosed)
        {
           return;
        }
        switch (state)
        {
           case connectionState::CONN_SYN_SENT:
           {
//             this->lock();
//              {
//                 this->notify_one();
//              }
//              this->unlock();
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
              recvQueueLock.lock();
              {
                 recvQueueCond.notify_one();
              }
              recvQueueLock.unlock();
              closeImpl();
              break;
           }
        }
        state = connectionState::CONN_CLOSED;
        isClosed = true;
     }
     closeMutex.unlock();
  }

  void MsgSocketReliable::closeImplThread(void* para)
  {
     MsgSocketReliable* _THIS = (MsgSocketReliable*) para;
     _THIS->handleCloseImpl();
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
     //boost::thread clossThread = boost::thread(MsgSocketReliable::closeImplThread, this); ???
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
      break;
      case connectionNotification::REFUSED :
      break;
      case connectionNotification::RESET :
      break;
    }
  }

  MsgSocketReliable::MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport)
  {
    sock=transport->createSocket(port);
  }
  MsgSocketReliable::MsgSocketReliable(MsgSocket* socket)
  {
    sock=socket;
  }
  MsgSocketReliable::~MsgSocketReliable()
  {
	  //TODO
  }

  void MsgSocketReliable::send(Message* msg,uint_t length)
  {
    //Apply reliable algorithm
    sock->send(msg);
  }

  void MsgSocketReliable::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
  // TODO
  }

  Message* MsgSocketReliable::receive(uint_t maxMsgs,int maxDelay)
  {
    return sock->receive(maxMsgs,maxDelay);
  }

};
