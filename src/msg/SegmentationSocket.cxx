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
    m_nFalgs=SegmentType::FRAG_MORE;
    if(isLastFrag==true)
    {
      m_nFalgs=SegmentType::FRAG_LAST;
    }
    m_nSeqn=_seqn;
    m_nLen= USER_HEADER_LEN + len;
  }

  int Segment::flags()
  {
    return m_nFalgs;
  }

  int Segment::seq()
  {
    return m_nSeqn;
  }
  int Segment::length()
  {
    return m_nLen;
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
    Byte *pBuffer = new Byte[length()];
    pBuffer[0] = (Byte) (m_nFalgs & 0xFF);
    pBuffer[1] = (Byte) (m_nLen & 0xFF);
    pBuffer[2] = (Byte) (m_nSeqn & 0xFF);
    pBuffer[3] = (Byte) (m_nMsgId & 0xFF);
    logTrace("MSG","FRT","copy [%d] by from mpdata  to buffer",m_nLen - USER_HEADER_LEN);
    memcpy(pBuffer+ USER_HEADER_LEN, m_pData , m_nLen - USER_HEADER_LEN);
    return pBuffer;
  }

  void Segment::parseBytes(const Byte* buffer, int _off, int _len)
  {
    m_nFalgs = int(buffer[_off] & 255);
    m_nLen   = int(buffer[_off+1] & 255);
    m_nSeqn  = int(buffer[_off+2] & 255);
    m_nMsgId = int(buffer[_off+3] & 255);
    m_pData = new Byte[m_nLen];
    logTrace("MSG","FRT","copy [%d] byte from fragment data to mData",m_nLen - USER_HEADER_LEN);
    memcpy(m_pData, buffer + _off + USER_HEADER_LEN, m_nLen-USER_HEADER_LEN);
  }

  Byte* Segment::getData()
  {
    return m_pData;
  }

  void Segment::parse(Byte* bytes, int off, int len)
  {
    int flags = bytes[off];
    if ((flags & MORE_FLAG) != 0)
    {
      setLast(false);
    }
    else if ((flags & LAST_FLAG) != 0)
    {
      setLast(false);
    }
    parseBytes(bytes,off,len);
  }

  Segment::Segment(int seqn, const Byte* buffer, int off, int len , bool isLastFrag)
  {
    init(seqn,len,isLastFrag);
    m_pData = new Byte[len];
    memcpy(m_pData, buffer + off, len);
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

  void MsgSocketSegmentaion::applySegmentaion(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    int totalBytes = 0;
    int off = 0;
    int writeBytes = 0;
    int seq=1;
    currFragId++;
    while(totalBytes<length)
    {
      seq++;
      writeBytes = MIN(MAX_SEGMENT_SIZE - USER_HEADER_LEN,length - totalBytes);
      if(totalBytes+writeBytes<length)
      {
        Segment *frag = new Segment(seq, (Byte*)buffer, off + totalBytes, writeBytes , false);
        logTrace("MSG","RST","Socket([%d - %d]) : Create fragment with seq [%d] ",sock->handle().getNode(),sock->handle().getPort(),frag->seq());
        frag->setMsgId(currFragId);
        sendFragment(destination,frag);
        totalBytes += writeBytes;
      }
      else
      {
        Segment *frag = new Segment(seq, (Byte*)buffer, off + totalBytes, writeBytes, true);
        logDebug("MSG", "REL","send last fragment to  node [%d] in reliable mode ",destination.getNode());
        frag->setMsgId(currFragId);
        sendFragment(destination,frag);
        totalBytes += writeBytes;
      }
    }
  }
  void MsgSocketSegmentaion::sendFragment(SAFplus::Handle destination,Segment * frag)
  {
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
    fragment->set((Byte*)frag->getBytes(),frag->length());
    logTrace("MSG","RST","Send Fragment to node [%d] port [%d] Id [%d] ",reliableFragment->getAddress().getNode(),reliableFragment->getAddress().getPort(),frag->seq());
    sock->send(reliableFragment);
  }


  void MsgSocketSegmentaion::applySegmentaion(Message* m)
  {

  }

  void MsgSocketSegmentaion::send(Message* msg)
  {
    applySegmentaion(msg);
  }
  void MsgSocketSegmentaion::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    applySegmentaion(destination,buffer,length,msgtype);
  }
  Message* MsgSocketSegmentaion::receive(uint_t maxMsgs,int maxDelay)
  {
    return sock->receive(maxMsgs,maxDelay);
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
        logDebug("MSG","SMT","Receive NULL fragment . Exit thread");
        return ;
      }
      MsgKey temp;
      temp.sender=handle;
      temp.msgId=pFrag->getMsgId();
      if(receiveMap[temp]==nullptr)
      {
        MsgSegments* msgSegments= new MsgSegments();
        receiveMap[temp]=msgSegments;
        receiveMap[temp]->segmentNum++;
      }
      else
      {
        receiveMap[temp]->segmentList.push_back(*pFrag);
        receiveMap[temp]->segmentNum++;
      }
      if(pFrag->isLastSegment())
      {
        if(receiveMap[temp]->segmentNum==pFrag->seq())
        {
          receiveMap[temp]->isFull=true;
          msgReceived++;
        }
        else
        {
          receiveMap[temp]->expectedSegments=pFrag->seq();
        }
      }
      else
      {
        if(receiveMap[temp]->segmentNum==receiveMap[temp]->expectedSegments)
        {
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
      logDebug("MSG","SMT","wait to receive message . Exit thread");
      usleep(100);
    }while(msgReceived==0);
    for ( auto it = receiveMap.begin(); it != receiveMap.end(); ++it )
    {
      if(it->second->isFull)
      {
        SegmentationList::iterator segment_it = it->second->segmentList.begin();
        while(segment_it != it->second->segmentList.end())
        {
          Segment& s = *segment_it;
          logDebug("MSG","RST","read the fragment [%d]",s.seq());
          int length = 0;
          Byte* data = ((Segment*) &s)->getData();
          length = ((Segment*) &s)->length();
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
            logDebug("MSG","RST","read the last fragment [%d]",s.seq());
          }
          memcpy(buffer + totalBytes, (void*)data , length);
          totalBytes += length;
          segment_it = it->second->segmentList.erase_and_dispose(segment_it, delete_disposer_segment());
          if(quit==true)
          {
            break;
          }
          else
          {
            it++;
          }
        }
        if(totalBytes > 0)
        {
          msgReceived--;
          return totalBytes;
        }
      }
    }
    return -1;
  }
  void MsgSocketSegmentaion::flush()
  {
    sock->flush();
  }
}
