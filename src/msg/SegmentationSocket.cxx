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
    logTrace("MSG","FRT","init segment with length [%d]",len);
    m_nFalgs=MORE_FLAG;
    if(isLastFrag==true)
    {
      m_nFalgs=LAST_FLAG;
    }
    m_nSeqn=_seqn;
    m_nLen= USER_HEADER_LEN;
    logTrace("MSG","FRT","init segment with header length [%d]",m_nLen);
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
    logTrace("MSG","FRT","Get header data :  flags [%d] - header len [%d] - Sequence [%d] - MsgId [%d]",m_nFalgs,m_nLen,m_nSeqn,m_nMsgId);
    Byte *pBuffer = new Byte[dataLength()];
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
    logTrace("MSG","FRT","parse header data flags : [%d] - header len : [%d] - Sequence : [%d] - MsgId : [%d]",m_nFalgs,m_nLen,m_nSeqn,m_nMsgId);
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
    if(!isLastFrag)
    {
      logTrace("MSG","SMT","This is the last segment [%s]",m_pData +90);
    }
  }


  static void SegmentSocketThread(void * arg)
  {
    MsgSocketSegmentaion* p_this = (MsgSocketSegmentaion*)arg;
    p_this->handleReceiveThread();
  }

  MsgSocketSegmentaion::MsgSocketSegmentaion(uint_t port,MsgTransportPlugin_1* transport)
  {
    sock = transport->createSocket(port);
    msgReceived=0;
    rcvThread = boost::thread(SegmentSocketThread, this);
  }

  MsgSocketSegmentaion::MsgSocketSegmentaion(MsgSocket* socket)
  {
    sock = socket;
  }
  MsgSocketSegmentaion::~MsgSocketSegmentaion()
  {
    //TODO
  }

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
    fragment->set(pBuffer,frag->dataLength()+USER_HEADER_LEN);
    logTrace("MSG","MSS","Send segment to node [%d] port [%d] with Sequence Id [%d] ",msgSegment->getAddress().getNode(),msgSegment->getAddress().getPort(),frag->seq());
    sock->send(msgSegment);
    delete pBuffer;
  }


  void MsgSocketSegmentaion::applySegmentaion(Message* m)
  {

  }
  void MsgSocketSegmentaion::send(Message* msg)
  {
    applySegmentaion(msg->getAddress(),(Byte*)msg,msg->getLength());
  }
  void MsgSocketSegmentaion::send(SAFplus::Handle destination, void* buffer, uint_t length)
  {
    applySegmentaion(destination,buffer,length);
  }
  Message* MsgSocketSegmentaion::receive(uint_t maxMsgs,int maxDelay)
  {
    return nullptr;
  }
  Segment* MsgSocketSegmentaion::receiveFragment(Handle &handle)
  {
    Segment* p_Fragment = new Segment();
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
    logTrace("MSG","SMT","Receive fragment from sender");
    p_Fragment->parse((Byte*)p_NextFrag->read(0), 0, p_NextFrag->len);
    return p_Fragment;
  }



  void MsgSocketSegmentaion::handleReceiveThread(void)
  {
    Handle handle;
    while (1)
    {
      Segment* pFrag =receiveFragment(handle);
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
      }
      else
      {
        logTrace("MSG","MSS","Receive segments [%d]",pFrag->seq());
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
          //          if(&s)
          //          {
          //            delete &s;
          //          }
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
