#include <SegmentationSocket.hxx>

//************Advanced socket : MsgSocketSegmentaion************


namespace SAFplus
{

  struct delete_disposer_segment
  {
    void operator()(Segment *delete_this)
    {
      delete delete_this;
    }
  };
  void Segment::init(int _seqn, int len, bool isLastFrag)
  {
    m_nFalgs=MORE_FLAG;
    if(isLastFrag==true)
    {
      m_nFalgs=LAST_FLAG;
    }
    m_nSeqn=_seqn;
    m_nLen= USER_HEADER_LEN;
  }

  int Segment::flags()
  {
    return m_nFalgs;
  }

  int Segment::seq()
  {
    return m_nSeqn;
  }
  int Segment::dataLength()
  {
    return m_nDataLen;
  }
  Segment::Segment()
  {

  }
  void Segment::setLast(bool isLastSeg)
  {
    isLast=isLastSeg;
  }
  bool Segment::isLastSegment()
  {
    return isLast;
  }

  Byte* Segment::getBytes()
  {
//    logTrace("MSG","FRT","Get header data [%d] :  flags [%d] - header len [%d] - Sequence [%d] - MsgId [%d] ",dataLength(),m_nFalgs,m_nLen,m_nSeqn,m_nMsgId);
    Byte *pBuffer = new Byte[dataLength() + USER_HEADER_LEN];
    pBuffer[0] = (Byte) (m_nFalgs & 0xFF);
    pBuffer[1] = (Byte) (m_nLen & 0xFF);
    pBuffer[2] = (Byte) (m_nSeqn & 0xFF);
    pBuffer[3] = (Byte) (m_nMsgId & 0xFF);
    logTrace("MSG","FRT","Get data : [%d] byte",m_nDataLen);
    memcpy(pBuffer+ USER_HEADER_LEN, m_pData , m_nDataLen);
    return pBuffer;
  }

  void Segment::parseBytes(const Byte* buffer, int _off, int _len)
  {
    m_nFalgs = int(buffer[_off] & 255);
    m_nLen   = int(buffer[_off+1] & 255);
    m_nSeqn  = int(buffer[_off+2] & 255);
    m_nMsgId = int(buffer[_off+3] & 255);
    m_nDataLen=_len - USER_HEADER_LEN;
//    logTrace("MSG","FRT","parse header data flags : [%d] - header len : [%d] - Sequence : [%d] - MsgId : [%d]",m_nFalgs,m_nLen,m_nSeqn,m_nMsgId);
    m_pData = new Byte[m_nDataLen];
    memcpy(m_pData, buffer + _off + USER_HEADER_LEN, m_nDataLen);
  }
  Byte* Segment::getData()
  {
    return m_pData;
  }

  void Segment::parse(Byte* bytes, int off, int len)
  {
    int flags = bytes[off];
    setLast(false);
    logTrace("MSG","SMT","Parse segment ...");
    if ((flags & MORE_FLAG) != 0)
    {
      setLast(false);
    }
    else if ((flags & LAST_FLAG) != 0)
    {
      logTrace("MSG","SMT","This is the last segment");
      setLast(true);
    }
    parseBytes(bytes,off,len);
  }

  Segment::Segment(int seqn, const Byte* buffer, int off, int len , bool isLastFrag)
  {
    init(seqn,len,isLastFrag);
    m_pData = new Byte[len];
    memcpy(m_pData, buffer + off, len);
    m_nDataLen=len;
  }


  static void SegmentSocketThread(void * arg)
  {
    MsgSocketSegmentaion* p_this = (MsgSocketSegmentaion*)arg;
    p_this->handleReceiveThread();
  }

  static ClRcT receiveTimeOutCallback(void *arg)
  {
    logDebug("MSG","RST","start clean unused segment list");
    return CL_OK;
  }

  MsgSocketSegmentaion::MsgSocketSegmentaion(uint_t port,MsgTransportPlugin_1* transport)
  {
    sock = transport->createSocket(port);
    msgReceived=0;
    rcvThread = boost::thread(SegmentSocketThread, this);
    ClRcT rc;
//    if ((rc = timerInitialize(NULL)) != CL_OK)
//    {
//      return;
//    }
//    testTimeout.tsMilliSec=0;
//    testTimeout.tsSec=1;
//    receiveTimeOut.timerCreate(testTimeout, TimerTypeT::TIMER_REPETITIVE, TimerContextT::TIMER_SEPARATE_CONTEXT,receiveTimeOutCallback, NULL);
//    //receiveTimeOut.timerStart();
  }

  MsgSocketSegmentaion::MsgSocketSegmentaion(MsgSocket* socket)
  {
    sock = socket;
  }
  MsgSocketSegmentaion::~MsgSocketSegmentaion()
  {
    //TODO
  }
  MsgPool* MsgSocketSegmentaion::getMsgPool()
  {
    logTrace("MSG", "MSS","get msg pool");
    return sock->msgPool;
  };

  void MsgSocketSegmentaion::applySegmentaion(SAFplus::Handle destination, void* buffer, uint_t length)
  {
    int totalBytes = 0;
    int off = 0;
    int writeBytes = 0;
    int seq=0;
    currFragId++;
    while(totalBytes<length)
    {
      seq++;
      writeBytes = MIN(MAX_SEGMENT_SIZE - USER_HEADER_LEN,length - totalBytes);
      if(totalBytes+writeBytes<length)
      {
        Segment *frag = new Segment(seq, (Byte*)buffer, off + totalBytes, writeBytes , false);
        logTrace("MSG","RSS","Socket([%d - %d]) : Create segment with sequence id [%d] ",sock->handle().getNode(),sock->handle().getPort(),frag->seq());
        frag->setMsgId(currFragId);
        sendSegment(destination,frag);
        totalBytes += writeBytes;
        delete frag;
      }
      else
      {
        Segment *frag = new Segment(seq, (Byte*)buffer, off + totalBytes, writeBytes, true);
        logTrace("MSG", "MSS","Create last segment with sequence id [%d]",frag->seq());
        frag->setMsgId(currFragId);
        sendSegment(destination,frag);
        totalBytes += writeBytes;
        delete frag;
      }
      usleep(100);
    }
  }
  void MsgSocketSegmentaion::sendSegment(SAFplus::Handle destination,Segment * frag)
  {
    if(sock==NULL)
    {
      return;
    }
    if(sock->msgPool==NULL)
    {
      return;
    }
    Message* msgSegment;
    msgSegment=sock->msgPool->allocMsg();
    assert(msgSegment);
    msgSegment->setAddress(destination);
    MsgFragment* fragment = msgSegment->append(0);
    Byte *pBuffer= (Byte*)frag->getBytes();
    fragment->set(pBuffer,frag->dataLength() + USER_HEADER_LEN);
    sock->send(msgSegment);
    if(pBuffer)
    {
      delete pBuffer;
    }
  }


  void MsgSocketSegmentaion::applySegmentaion(Message* m)
  {

  }
  void MsgSocketSegmentaion::send(Message* origMsg)
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
      send(destination,buffer,bufferLen);
      delete buffer;
    } while (next != NULL);
  }

  void MsgSocketSegmentaion::send(SAFplus::Handle destination, void* buffer, uint_t length)
  {
    applySegmentaion(destination,buffer,length);
  }
  Message* MsgSocketSegmentaion::receive(uint_t maxMsgs,int maxDelay)
  {
    int totalBytes = 0;
    bool quit=false;
    do
    {
      //logTrace("MSG","MSS","Wait to receive message");
      if(msgReceived>0)
      {
        break;
      }
      usleep(100000);
    }while(1);
    for ( auto it = receiveMap.begin(); it != receiveMap.end();)
    {
      if(it->second->isFull)
      {
        int msgType;
        int length=it->second->length;
        logTrace("MSG","MSS","Wait to receive message length [%d]", length);
        Byte* buffer = new Byte[length];
        SegmentationList::iterator segment_it = it->second->segmentList.begin();
        while(segment_it != it->second->segmentList.end())
        {
          Segment& s = *segment_it;
          int length = 0;
          Byte* data =s.getData();
          length = s.dataLength();
          logTrace("MSG","MSS","Read the segment [%d] Msg Id [%d]  with len [%d]",s.seq(),s.getMsgId(),s.dataLength());
          if(s.isLastSegment())
          {
            quit=true;
          }
          memcpy(buffer + totalBytes, (void*)data , length);
          totalBytes += length;
          segment_it = it->second->segmentList.erase_and_dispose(segment_it, delete_disposer_segment());
          if(quit==true)
          {
            break;
          }
        }
        if(totalBytes > 0)
        {
          msgReceived--;
          logTrace("MSG","MSS","Receive message with [%d] bytes. Update received message to [%d] ",totalBytes,msgReceived);
          Message* m = sock->msgPool->allocMsg();
          assert(m);
          logTrace("MSG","MSS","Set Address [%d] - [%d] ",it->first.sender.getNode(),it->first.sender.getPort());
          m->setAddress(it->first.sender);
          MsgFragment* frag = m->append(0);
          frag->set(buffer,totalBytes);
          if(it->second->segmentList.empty())
          {
            MsgSegments& temp = *(it->second);
            logTrace("MSG","MSS","List empty");
            it = receiveMap.erase(it);
            if(&temp)
            {
              logTrace("MSG","MSS","delete MsgSegments");
              delete &temp;
            }
          }
          return m;
        }
      }
      ++it;
    }
    return nullptr;
  }
  Segment* MsgSocketSegmentaion::receiveSegment(Handle &handle)
  {
    Segment* p_Fragment = new Segment();
    Message* p_Msg = nullptr;
    int iMaxMsg = 1;
    int iDelay = 1;
    p_Msg = this->sock->receive( iMaxMsg, iDelay);
    if(p_Msg == nullptr)
    {
      return nullptr;

    }
    handle = p_Msg->getAddress();
    p_Fragment->parse((Byte*)p_Msg->firstFragment->read(0), 0, p_Msg->firstFragment->len);
    sock->msgPool->free(p_Msg);
    return p_Fragment;
  }



  void MsgSocketSegmentaion::handleReceiveThread(void)
  {
    Handle handle;
    while (1)
    {
      Segment* pFrag =receiveSegment(handle);
      if(pFrag==nullptr)
      {
        logTrace("MSG","MSS","Receive NULL fragment . Exit thread");
        return ;
      }
      MsgKey temp;
      temp.sender=handle;
      temp.msgId=pFrag->getMsgId();
      if(receiveMap[temp]==nullptr)
      {
        logTrace("MSG","MSS","Create new segmentList");
        MsgSegments* msgSegments= new MsgSegments();
        msgSegments->handle=handle;
        logTrace("MSG","MSS","set handle [%d] [%d]",msgSegments->handle.getNode(),msgSegments->handle.getPort());
        receiveMap[temp]=msgSegments;
        logTrace("MSG","MSS","Push segment to segmentList");
        receiveMap[temp]->segmentList.push_back(*pFrag);
        receiveMap[temp]->segmentNum++;
      }
      else
      {
        logTrace("MSG","MSS","Push segment to segmentList");
        receiveMap[temp]->segmentList.push_back(*pFrag);
        receiveMap[temp]->segmentNum++;
      }
      if(pFrag->isLastSegment())
      {
        logTrace("MSG","MSS","Receive last segment with seq [%d] and number of segments [%d]",pFrag->seq(),receiveMap[temp]->segmentNum);
        if(receiveMap[temp]->segmentNum==pFrag->seq())
        {
          receiveMap[temp]->isFull=true;
          msgReceived++;
          logTrace("MSG","MSS","Increase msg received to [%d]",msgReceived);
        }
        else
        {
          logTrace("MSG","MSS","Update expected segments num to [%d]",pFrag->seq());
          receiveMap[temp]->expectedSegments=pFrag->seq();
        }
        receiveMap[temp]->length= (pFrag->seq()-1)*(MAX_SEGMENT_SIZE - USER_HEADER_LEN) + pFrag->dataLength();
      }
      else
      {
        logTrace("MSG","MSS","Receive segments [%d] msgId [%d]",pFrag->seq(),pFrag->getMsgId());
        if(receiveMap[temp]->segmentNum==receiveMap[temp]->expectedSegments)
        {
          logTrace("MSG","MSS","Receive full segments with segment number [%d] and expected segments [%d]",receiveMap[temp]->segmentNum,receiveMap[temp]->expectedSegments);
          receiveMap[temp]->isFull=true;
          msgReceived++;
        }
      }
    }
  }

  int MsgSocketSegmentaion::read(Byte* buffer,int maxlength)
  {
    Handle handle;
    int totalBytes = 0;
    bool quit=false;
    do
    {
      //logTrace("MSG","MSS","Wait to receive message");
      if(msgReceived>0)
      {
        break;
      }
      usleep(100000);
    }while(1);
    for ( auto it = receiveMap.begin(); it != receiveMap.end();)
    {
      if(it->second->isFull)
      {
        it->second->segmentList.sort();
        SegmentationList::iterator segment_it = it->second->segmentList.begin();
        while(segment_it != it->second->segmentList.end())
        {
          Segment& s = *segment_it;
          int length = 0;
          Byte* data =s.getData();
          length = s.dataLength();
          logTrace("MSG","MSS","Read the segment [%d] Msg Id [%d]  with len [%d]",s.seq(),s.getMsgId(),s.dataLength());
          if(totalBytes + length > maxlength)
          {
            if (totalBytes <= 0)
            {
              throw new Error("insufficient buffer space");
            }
          }
          if(s.isLastSegment())
          {
            quit=true;
          }
          memcpy(buffer + totalBytes, (void*)data , length);
          totalBytes += length;
          segment_it = it->second->segmentList.erase_and_dispose(segment_it, delete_disposer_segment());
          if(quit==true)
          {
            break;
          }
        }
        if(totalBytes > 0)
        {
          msgReceived--;
          logTrace("MSG","MSS","Receive message with [%d] byte. Update received message to [%d] ",totalBytes,msgReceived);
          if(it->second->segmentList.empty())
          {
            MsgSegments& temp = *(it->second);
            logTrace("MSG","MSS","List empty");
            it = receiveMap.erase(it);
            if(&temp)
            {
              logTrace("MSG","MSS","delete MsgSegments");
              delete &temp;
            }
          }
          return totalBytes;
        }
      }
      ++it;
    }
    return -1;
  }
  void MsgSocketSegmentaion::flush()
  {
    sock->flush();
  }
}
