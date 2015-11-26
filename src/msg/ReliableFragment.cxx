#include "ReliableFragment.hxx"
namespace SAFplus
{
  ReliableFragment::ReliableFragment()
  {
    // To do
    this->m_nAckn=-1;
    this->m_nRetCounter=0;
  }
  int ReliableFragment::flags()
  {
    return m_nFalgs;
  }
  int ReliableFragment::length()
  {
    return m_nLen;
  }
  int ReliableFragment::seq()
  {
    return m_nSeqn;
  }
  int ReliableFragment::getAck()
  {
    if(m_nFalgs & ACK_FLAG)
    {
      return m_nAckn;
    }
    return -1;
  }


  int ReliableFragment::getRetxCounter()
  {
    return m_nRetCounter;
  }
  void ReliableFragment::setAck(int _ackn)
  {
    m_nFalgs = m_nFalgs | ACK_FLAG;
    m_nAckn = _ackn;
  }

  void ReliableFragment::setRetxCounter(int _retCounter)
  {
    m_nRetCounter = _retCounter;
  }

  fragmentType ReliableFragment::getType()
  {
    fragmentType temp;
    switch (m_nFalgs)
    {
      case SYN_FLAG:
      {
        temp=FRAG_SYN;
        break;
      }
      case ACK_FLAG:
      {
        temp=FRAG_DATA;
        break;
      }
      case NAK_FLAG:
      {
        temp=FRAG_NAK;
        break;
      }
      case RST_FLAG:
      {
        temp=FRAG_RST;
        break;
      }
      case NUL_FLAG:
      {
        temp=FRAG_NUL;
        break;
      }
      case CHK_FLAG:
      {
        temp=FRAG_UDE;
        break;
      }
      case FIN_FLAG:
      {
        temp=FRAG_FIN;
        break;
      }
      default:
      {
        temp=FRAG_UDE;
        break;
      }
    }
    return temp;
  }
  Byte* ReliableFragment::getHeader()
  {
    Byte *pBuffer = new Byte[RUDP_HEADER_LEN];
    pBuffer[0] = (Byte) (m_nFalgs & 0xFF);
    pBuffer[1] = (Byte) (m_nLen & 0xFF);
    pBuffer[2] = (Byte) (m_nSeqn & 0xFF);
    pBuffer[3] = (Byte) (m_nAckn & 0xFF);
    if (isFirstSegmentofFragment==true)
    {
      pBuffer[4] = (Byte) (1 & 0xFF);
    }
    else
    {
      pBuffer[4] = (Byte) (0 & 0xFF);
    }
    return pBuffer;
  }

  int ReliableFragment::setHeader(void* ptr)
  {
    ((Byte*)ptr)[0] = (Byte) (m_nFalgs & 0xFF);
    ((Byte*)ptr)[1] = (Byte) (m_nLen & 0xFF);
    ((Byte*)ptr)[2] = (Byte) (m_nSeqn & 0xFF);
    ((Byte*)ptr)[3] = (Byte) (m_nAckn & 0xFF);
    if (isFirstSegmentofFragment==true)
    {
      ((Byte*)ptr)[4] = (Byte) (1 & 0xFF);
    }
    else
    {
      ((Byte*)ptr)[4] = (Byte) (0 & 0xFF);
    }
    return RUDP_HEADER_LEN;
  }


  void ReliableFragment::init(int _flags, int _seqn, int len, int isLastFrag)
  {
    m_nFalgs = _flags;
    m_nSeqn = _seqn;
    m_nLen = len;
    isFirstSegmentofFragment=false;

  }

  void ReliableFragment::parseHeader(const Byte* buffer, int _off, int _len)
  {
    m_nFalgs = int(buffer[_off] & 255);
    m_nLen   = int(buffer[_off+1] & 255);
    m_nSeqn  = int(buffer[_off+2] & 255);
    m_nAckn  = int(buffer[_off+3] & 255);
    if(int(buffer[_off+4] & 255)==1)
    {
      isFirstSegmentofFragment=true;
    }
    else
    {
      isFirstSegmentofFragment=false;
    }

  }

  ReliableFragment* ReliableFragment::parse(Byte* bytes, int off, int len)
  {
    ReliableFragment *fragment = nullptr;
    if (len < RUDP_HEADER_LEN)
    {
      // throw new IllegalArgumentException("Invalid segment");
      throw Error("Invalid segment");
    }
    int flags = bytes[off];
    if ((flags & SYN_FLAG) != 0)
    {
      logTrace("MSG","FRT","parse SYN fragment");
      fragment = new SYNFragment();
    }
    else if ((flags & NUL_FLAG) != 0)
    {
      logTrace("MSG","FRT","parse NUL fragment");
      fragment = new NULLFragment();
    }
    else if ((flags & NAK_FLAG) != 0)
    {
      logTrace("MSG","FRT","parse NAK fragment");
      fragment = new NAKFragment();
    }
    else if ((flags & RST_FLAG) != 0)
    {
      logTrace("MSG","FRT","parse RST fragment");
      fragment = new RSTFragment();
    }
    else if ((flags & FIN_FLAG) != 0)
    {
      logTrace("MSG","FRT","parse FIN fragment");
      fragment = new FINFragment();
    }
    else if ((flags & ACK_FLAG) != 0)
    {
      /* always process ACKs or Data segments last */
      if ((flags & LAS_FLAG) != 0)
      {
        logTrace("MSG","FRT","parse LAST fragment");
        fragment = new DATFragment();
        fragment->setLast(true);
      }
      else
      {
        logTrace("MSG","FRT","parse ACK fragment");
        if (len == RUDP_HEADER_LEN)
        {
          fragment = new ACKFragment();
          fragment->setLast(false);
        }
        else
        {
          fragment = new DATFragment();
          fragment->setLast(false);
        }
      }
    }
    if (fragment == nullptr)
    {
      //throw new IllegalArgumentException("Invalid segment");
      throw Error("Invalid segment");
    }
    fragment->parseHeader(bytes, off, len);
    return fragment;
  }

  ReliableFragment* ReliableFragment::parse(Byte* bytes)
  {
    int length = 0;
    return ReliableFragment::parse(bytes, 0, length);
  }



  //------------------------------
  // Data Fragment
  //------------------------------
  DATFragment::DATFragment()
  {

  }

  DATFragment::DATFragment(int seqn, int ackn,Byte* buffer, int off, int len ,MsgPool* msgPool, bool isLastFrag, bool isLastSegmentOfFrag)
  {
    if(isLastFrag==true)
    {
      init(LAS_FLAG, seqn, len);
    }
    else
    {
      init(ACK_FLAG, seqn, len);
    }
    if(isLastSegmentOfFrag==true)
    {
      this->isFirstSegmentofFragment=true;
    }
    else
    {
      this->isFirstSegmentofFragment=false;
    }
    setAck(ackn);
    MsgFragment* hdr = msgPool->allocMsgFragment(RUDP_HEADER_LEN);
    uint_t hdrLen = setHeader(hdr->data(0));
    hdr->used(hdrLen);
    MsgFragment* splitFrag = msgPool->allocMsgFragment(0);
    splitFrag->set(buffer,len);
    message->firstFragment = hdr;
    hdr->nextFragment = splitFrag;
    splitFrag->nextFragment = NULL;
    message->lastFragment = NULL;
  }

  int DATFragment::length()
  {
    return m_nLen;
  }

  Byte* DATFragment::getData()
  {
    return (Byte*)message->lastFragment->read(0);
  }

  Byte* DATFragment::getHeader()
  {
    Byte* buffer = ReliableFragment::getHeader();
    return buffer;
  }

  void DATFragment::parseHeader(const Byte* buffer, int _off, int _len)
  {
    ReliableFragment::parseHeader(buffer, _off, _len);

  }
  void DATFragment::parseData(const Byte* buffer, int _off, int _len)
  {

  }
  void DATFragment::parseData(Message *m)
  {
    message = m;
  }

  fragmentType DATFragment::getType()
  {
    return FRAG_DATA;
  }

  DATFragment::~DATFragment()
  {

  };
  // End DATFragment class
  SYNFragment::SYNFragment()
  {

  }

  SYNFragment::SYNFragment(int seqn, int maxseg, int maxsegsize, int rettoval,
      int cumacktoval, int niltoval, int maxret,
      int maxcumack, int maxoutseq, int maxautorst,MsgPool* msgPool)
  {
    init(SYN_FLAG, seqn, SYN_HEADER_LEN);
    Byte *buffer = new Byte(16);
    buffer[0] = (Byte) ((RUDP_VERSION << 4) & 0xFF);
    buffer[1] = (Byte) (maxseg & 0xFF);
    buffer[2] = (Byte) (0x01 & 0xFF);
    buffer[3] = 0; /* spare */
    buffer[4] = (Byte) ((maxsegsize >> 8) & 0xFF);
    buffer[5] = (Byte) ((maxsegsize >> 0) & 0xFF);
    buffer[6] = (Byte) ((rettoval >> 8) & 0xFF);
    buffer[7] = (Byte) ((rettoval >> 0) & 0xFF);
    buffer[8] = (Byte) ((cumacktoval >> 8) & 0xFF);
    buffer[9] = (Byte) ((cumacktoval >> 0) & 0xFF);
    buffer[10] = (Byte) ((niltoval >> 8) & 0xFF);
    buffer[11] = (Byte) ((niltoval >> 0) & 0xFF);
    buffer[12] = (Byte) (maxret & 0xFF);
    buffer[13] = (Byte) (maxcumack & 0xFF);
    buffer[14] = (Byte) (maxoutseq & 0xFF);
    buffer[15] = (Byte) (maxautorst & 0xFF);
    MsgFragment* hdr = msgPool->allocMsgFragment(RUDP_HEADER_LEN);
    uint_t hdrLen = setHeader(hdr->data(0));
    hdr->used(hdrLen);
    MsgFragment* splitFrag = msgPool->allocMsgFragment(0);
    splitFrag->set(buffer,16);
    message->firstFragment = hdr;
    hdr->nextFragment = splitFrag;
    splitFrag->nextFragment = NULL;
    message->lastFragment = NULL;
  }

  int SYNFragment::getSyncValueOneByte(int pos)
  {
    Byte* temp = (Byte*)message->lastFragment->read(0);
    return (temp[pos] & 0xFF);
  }
  int SYNFragment::getSyncValueTwoByte(int pos)
  {
    Byte* buffer = (Byte*)message->lastFragment->read(0);
    return ((buffer[pos] & 0xFF) << 8) | ((buffer[pos+1] & 0xFF) << 0);
  }

  int SYNFragment::getVersion()
  {
    return getSyncValueOneByte(0);
  }
  int SYNFragment::getMaxOutstandingFragments()
  {
    return getSyncValueOneByte(1);
  }

  int SYNFragment::getOptionFlags()
  {
    return getSyncValueOneByte(2);
  }

  int SYNFragment::getMaxFragmentSize()
  {
    return getSyncValueTwoByte(4);
  }
  int SYNFragment::getRetransmissionTimeout()
  {
    return getSyncValueTwoByte(6);
  }
  int SYNFragment::getCummulativeAckTimeout()
  {
    return getSyncValueTwoByte(8);
  }
  int SYNFragment::getNulFragmentTimeout()
  {
    return getSyncValueTwoByte(10);
  }

  int SYNFragment::getMaxRetransmissions()
  {
    return getSyncValueOneByte(12);
  }

  int SYNFragment::getMaxCumulativeAcks()
  {
    return getSyncValueOneByte(13);
  }
  int SYNFragment::getMaxOutOfSequence()
  {
    return getSyncValueOneByte(14);
  }
  int SYNFragment::getMaxAutoReset()
  {
    return getSyncValueOneByte(15);
  }

  Byte* SYNFragment::getHeader()
  {
    Byte *buffer = ReliableFragment::getHeader();
    return buffer;
  }
  Byte* SYNFragment::getData()
  {
    return NULL;
  }
  void SYNFragment::parseHeader(const  Byte* buffer, int off, int len)
  {
    ReliableFragment::parseHeader(buffer, off, len);
  }
  void SYNFragment::parseData(Message *m)
  {
    parseData((Byte*)m->lastFragment->read(0),0,m->lastFragment->len);
  }
  void SYNFragment::parseData(const  Byte* buffer, int off, int len)
  {
    if (len < (0) )
    {
      throw Error("Invalid SYN Fragment");
    }
  }
  fragmentType SYNFragment::getType()
  {
    return FRAG_SYN;
  }


  //--------------------------------------------------
  // ACK Fragment
  //--------------------------------------------------
  ACKFragment::ACKFragment()
  {

  }
  ACKFragment::ACKFragment(int seqn, int ackn,MsgPool* msgPool)
  {
    init(ACK_FLAG, seqn, 0);
    setAck(ackn);
    MsgFragment* hdr = msgPool->allocMsgFragment(RUDP_HEADER_LEN);
    uint_t hdrLen = setHeader(hdr->data(0));
    hdr->used(hdrLen);
    message->firstFragment = hdr;
    hdr->nextFragment = NULL;
  }
  fragmentType ACKFragment::getType()
  {
    return FRAG_ACK;
  }


  //-------------------------------------------
  // NAK Fragment
  //-------------------------------------------
  NAKFragment::NAKFragment()
  {
  }
  NAKFragment::NAKFragment(int seqn, int ackn,  int* acks, int size,MsgPool *msgPool)
  {
    init(NAK_FLAG, seqn, size);
    setAck(ackn);
    m_pArrAcks = (int*)malloc(size);
    m_nNumNak=size;
    m_pArrAcks=acks;
    MsgFragment* hdr = msgPool->allocMsgFragment(RUDP_HEADER_LEN);
    uint_t hdrLen = setHeader(hdr->data(0));
    hdr->used(hdrLen);
    MsgFragment* splitFrag = msgPool->allocMsgFragment(0);
    splitFrag->set(getData(),m_nNumNak);
    message->firstFragment = hdr;
    hdr->nextFragment = splitFrag;
    splitFrag->nextFragment = NULL;
    message->lastFragment = NULL;
  }

  Byte* NAKFragment::getHeader()
  {
    Byte* buffer= ReliableFragment::getHeader();
    return buffer;
  }
  Byte* NAKFragment::getData()
  {
    Byte* buffer = new Byte(m_nNumNak);
    for (int i = 0; i < m_nNumNak; i++)
    {
      buffer[i] = (Byte) (m_pArrAcks[i] & 0xFF);
    }
    return buffer;
  }

  void NAKFragment::parseHeader(const Byte* buffer, int off, int len)
  {
    ReliableFragment::parseHeader(buffer, off, len);
  }
  void NAKFragment::parseData(const Byte* buffer, int off, int len)
  {
    m_nNumNak = len;
    m_pArrAcks = new int[m_nNumNak];
    for (int i = 0; i < m_nNumNak; i++)
    {
      m_pArrAcks[i] = int(buffer[off + i] & 0xFF);
    }
  }
  void NAKFragment::parseData(Message *m)
  {
    parseData((Byte*)m->lastFragment->read(0),0,m->lastFragment->len);
  }

  fragmentType NAKFragment::getType()
  {
    return FRAG_NAK;
  }
  int* NAKFragment::getACKs(int* length)
  {
    *length= m_nNumNak;
    return m_pArrAcks;
  }

  // End NAK Fragment Class

  //-----------------------------------
  //- FINFragment Class
  //-----------------------------------
  //-----------------------------------
  FINFragment::FINFragment()
  {

  }
  FINFragment::FINFragment(int seqn)
  {
    init(FIN_FLAG, seqn, 0);
  }
  // End FIN Fragment
  fragmentType FINFragment::getType()
  {
    return FRAG_FIN;
  }

  NULLFragment::NULLFragment()
  {

  }
  NULLFragment::NULLFragment(int seqn)
  {
    init(NUL_FLAG, seqn, 0);
  }
  fragmentType NULLFragment::getType()
  {
    return FRAG_NUL;
  }
  // End NULL Fragment class

  RSTFragment::RSTFragment()
  {

  }
  RSTFragment::RSTFragment( int seqn)
  {
    init(RST_FLAG, seqn, 0);
  }
  fragmentType RSTFragment::getType()
  {
    return FRAG_RST;
  }
  // Ed RSTFragment Class.

}

