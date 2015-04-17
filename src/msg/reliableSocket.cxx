#include <reliableSocket.hxx>
#include <stdio.h>
namespace SAFplus
{
  static void* nullFragmentTimerFunc(void* arg)
  {
    MsgSocketReliable* tempMsgSocket = (MsgSocketReliable*)arg;
    while(tempMsgSocket->nullFragmentTimer.isStop)
    {
      sleep(tempMsgSocket->nullFragmentTimer.interval);
      if(tempMsgSocket->nullFragmentTimer.status==TIMER_RUN)
      {
        //call function
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
        //call function
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
        //call function
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
        //call function
      }
    }
    return NULL;
  }

  int compareFragment(int frag1, int frag2)
  {
    if (frag1 == frag2) {
        return 0;
    }
    else if (((frag1 < frag2) && ((frag2 - frag1) > MAX_FRAGMENT_NUMBER/2)) ||
             ((frag1 > frag2) && ((frag1 - frag2) < MAX_FRAGMENT_NUMBER/2))) {
        return 1;
    }
    else {
        return -1;
    }
  }
  void MsgSocketReliable::handleReliableFragment(ReliableFragment frag)
  {

  }

  void MsgSocketReliable::handleSYNReliableFragment()
  {

  }
  void MsgSocketReliable::handleACKReliableFragment()
  {

  }
  void MsgSocketReliable::handleNAKReliableFragment()
  {

  }
  void MsgSocketReliable::sendACK()
  {

  }
  void MsgSocketReliable::sendUACK()
  {

  }
  void MsgSocketReliable::sendSYN()
  {

  }
  void MsgSocketReliable::setACK(ReliableFragment frag)
  {
    if (queueInfo.getAndResetCumulativeAckCounter() == 0)
    {
      return;
    }
    frag.setAck(queueInfo.getLastInSequence());
  }

  void MsgSocketReliable::getACK(ReliableFragment frag)
  {
    int ackn = frag.getAck();
    if (ackn < 0)
    {
      return;
    }
    queueInfo.getAndResetOutstandingSegsCounter();
    if (state == connectionState::CONN_SYN_RCVD)
    {
      state = connectionState::CONN_ESTABLISHED;
      setconnection(connectionNotification::OPENED);
    }
//    std::list <ReliableFragment>::Iterator it = _unackedSentQueue.begin();
//    while (it!=_unackedSentQueue.end())
//    {
//      if (compareFragment(frag.seq(), ackn) <= 0)
//      {
//        //remove frag
//      }
//      if (_unackedSentQueue.empty())
//      {
//        retransmissionTimer.status=TIMER_PAUSE;
//      }
//    }
//      //notify all
  }

  void MsgSocketReliable::sendReliableFragment(ReliableFragment frag)
  {
    /* Piggyback any pending acknowledgments */
    if (frag.getType()==fragmentType::FRAG_DATA || frag.getType()==fragmentType::FRAG_RST || frag.getType()==fragmentType::FRAG_FIN || frag.getType()==fragmentType::FRAG_NUL)
    {
    //TODO checkAndSetAck(s);
        setACK(frag);
    }
    /* Reset null segment timer */
    if (frag.getType()==fragmentType::FRAG_DATA || frag.getType()==fragmentType::FRAG_RST || frag.getType()==fragmentType::FRAG_FIN)
    {
      //TODO nullSegmentTimer.reset();
    }
    //send reliable fragment
    Message* reliableReliableFragment = sock->msgPool->allocMsg();
    assert(reliableReliableFragment);
    reliableReliableFragment->setAddress(destination);
    MsgFragment* pfx  = reliableReliableFragment->append(1);
    * ((unsigned char*)pfx->data()) = messageType;
    pfx->len = 1;
    MsgFragment* fragment = reliableReliableFragment->append(0);
    fragment->set((char*)&frag,frag.length());
    sock->send(reliableReliableFragment);
  }
  void MsgSocketReliable::queueAndSendReliableFragment(ReliableFragment frag)
  {

  }
//  ReliableFragment MsgSocketReliable::receiveReliableFragment()
//  {
//
//  }
  void MsgSocketReliable::retransmitSegment(ReliableFragment frag)
  {
    if (profile.maxRetrans() > 0)
    {
      frag.setRetxCounter(frag.getRetxCounter()+1);
    }
    if (profile.maxRetrans() != 0 && frag.getRetxCounter() > profile.maxRetrans())
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
    fragment->set((char*)&frag,frag.length());
    sock->send(reliableReliableFragment);
    if (frag.getType()== FRAG_DATA)
    {
    //TODO
    }
  }
  void MsgSocketReliable::setconnection(connectionNotification state)
  {
    switch(state)
    {
      case connectionNotification::CLOSE :
      break;
      case connectionNotification::FAILURE :
      break;
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
