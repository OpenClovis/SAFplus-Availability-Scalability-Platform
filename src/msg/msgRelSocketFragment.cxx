#include "clMsgRelSocketFragment.hxx"
namespace SAFplus
{
  ReliableFragment::ReliableFragment()
  {
    ackNumber=-1;
    retransCounter=0;
    data=NULL;
    dataLen=0;
    isLast=false;
    isFirst=false;
    controlFlag=0;
    fragmentId=0;
    int ackNumber=0;
    int retransCounter=0;
  }
  void ReliableFragment::setLast(bool isLastFragment)
  {
    isLast=isLastFragment;
  }
  bool ReliableFragment::isLastFragment()
  {
    return isLast;
  }

  int ReliableFragment::getFlag()
  {
    return controlFlag;
  }
  int ReliableFragment::getFragmentId()
  {
    return fragmentId;
  }
  int ReliableFragment::getAck()
  {
    if((controlFlag & int(ACK_FLAG))== ACK_FLAG)
    {
      return ackNumber;
    }
    return -1;
  }
  void ReliableFragment::parseData(Byte* buffer, int _len)
  {
    return;
  };
  void ReliableFragment::setMessage(Message* msg)
  {
    message=msg;
  };
  Byte* ReliableFragment::getData()
  {
    return NULL;
  }
  int ReliableFragment::setHeader(void* ptr)
  {
    ((Byte*)ptr)[0] = (Byte) (controlFlag & 0xFF);
    ((Byte*)ptr)[1] = (Byte)((fragmentId >> 8) & 0xFF);
    ((Byte*)ptr)[2] = (Byte)((fragmentId >> 0) & 0xFF);
    ((Byte*)ptr)[3] = (Byte)((ackNumber >> 8) & 0xFF);
    ((Byte*)ptr)[4] = (Byte)((ackNumber >> 0) & 0xFF);
    if (isFirst==true)
    {
      ((Byte*)ptr)[5] = (Byte) (1 & 0xFF);
    }
    else
    {
      ((Byte*)ptr)[5] = (Byte) (0 & 0xFF);
    }
    return RUDP_HEADER_LEN;
  }

  int ReliableFragment::getRetxCounter()
  {
    return retransCounter;
  }
  void ReliableFragment::setAck(int _ackn)
  {
    controlFlag = controlFlag | ACK_FLAG;
    ackNumber = _ackn;
  }

  void ReliableFragment::setRetxCounter(int _retCounter)
  {
    retransCounter = _retCounter;
  }

  fragmentType ReliableFragment::getType()
  {
    fragmentType temp;
    switch (controlFlag)
    {
      case SYN_FLAG:
      {
        temp=FRAG_SYN;
        break;
      }
      case ACK_FLAG:
      {
        temp=FRAG_DAT;
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
    Byte *pBuffer = (Byte*)malloc(RUDP_HEADER_LEN);
    pBuffer[0] = (Byte) (controlFlag & 0xFF);
    pBuffer[1] = (Byte)((fragmentId >> 8) & 0xFF);
    pBuffer[2] = (Byte)((fragmentId >> 0) & 0xFF);
    pBuffer[3] = (Byte)((ackNumber >> 8) & 0xFF);
    pBuffer[4] = (Byte)((ackNumber >> 0) & 0xFF);
    if (isFirst==true)
    {
      pBuffer[5] = (Byte) (1 & 0xFF);
    }
    else
    {
      pBuffer[5] = (Byte) (0 & 0xFF);
    }
    return pBuffer;
  }

  void ReliableFragment::initFragment(int _flags, int _seqn, int isLastFrag)
  {
    controlFlag = _flags;
    fragmentId = _seqn;
    message=NULL;
    isLast=isLastFrag;
    isFirst=false;
  }
  int ReliableFragment::getlength()
  {
    return 0;
  }

  void ReliableFragment::parseHeader(const Byte* buffer, int _len)
  {
    controlFlag = int(buffer[0] & 255);
    fragmentId = ((buffer[1] & 0xFF) << 8) | ((buffer[2] & 0xFF) << 0);
    ackNumber  = ((buffer[3] & 0xFF) << 8) | ((buffer[4] & 0xFF) << 0);
    if(int(buffer[5] & 255)==1)
    {
      isFirst=true;
    }
    else
    {
      isFirst=false;
    }
  }

  ReliableFragment* ReliableFragment::parse(Byte* bytes, int len)
  {
    ReliableFragment *fragment = NULL;
    if (len < RUDP_HEADER_LEN)
    {
      // throw new IllegalArgumentException("Invalid segment");
      throw Error("Invalid segment");
    }
    int flags = bytes[0];
    if ((flags & SYN_FLAG) != 0)
    {
      fragment = new SYNFragment();
    }
    else if ((flags & NUL_FLAG) != 0)
    {
      fragment = new NULLFragment();
    }
    else if ((flags & NAK_FLAG) != 0)
    {
      fragment = new NAKFragment();
    }
    else if ((flags & RST_FLAG) != 0)
    {
      fragment = new RSTFragment();
    }
    else if ((flags & FIN_FLAG) != 0)
    {
      //logTrace("MSG","FRT","parse FIN fragment");
      fragment = new FINFragment();
    }
    else if ((flags & LAS_FLAG) != 0)
    {
      fragment = new DATFragment();
      fragment->setLast(true);
    }
    else if ((flags & ACK_FLAG) != 0)
    {
      /* always process ACKs or Data segments last */

      if (len == RUDP_HEADER_LEN)
      {
        fragment = new ACKFragment();
        fragment->setLast(false);
      }
      else
      {
        //logTrace("MSG","FRT","parse DATA fragment");
        fragment = new DATFragment();
        fragment->setLast(false);
      }
    }
    if (fragment == NULL)
    {
      //throw new IllegalArgumentException("Invalid segment");
      throw Error("Invalid segment");
    }
    fragment->parseHeader(bytes, len);
    fragment->message=NULL;
    return fragment;
  }
  ReliableFragment* ReliableFragment::parse(Byte* bytes)
  {
    int length = 0;
    return ReliableFragment::parse(bytes, length);
  }
  //--- End Reliable Fragment

  //------------------------------
  // Data Fragment
  //------------------------------
  DATFragment::DATFragment()
  {

  }
  int DATFragment::getlength()
  {
    return dataLen;
  }
  DATFragment::DATFragment(int seqn, int ackn,Byte* buffer, int off, int len , bool isLastFrag, bool isFirstSegmentOfFrag)  {
    if(isLastFrag==true)
    {
      initFragment(LAS_FLAG, seqn,true);
    }
    else
    {
      initFragment(ACK_FLAG, seqn);
    }
    setAck(ackn);
    this->isFirst=isFirstSegmentOfFrag;
    data=buffer+off;
    dataLen=len;
  }

  Byte* DATFragment::getData()
  {
    return data;
  }

  void DATFragment::parseData(Byte* buffer, int _len)
  {
    dataLen = _len;
    data = buffer;
  }
  fragmentType DATFragment::getType()
  {
    return FRAG_DAT;
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
      int maxcumack, int maxoutseq, int maxautorst)
  {
    initFragment(SYN_FLAG, seqn);
    maxFragment = maxseg;
    maxFragmentSize = maxsegsize;
    retransInterval = rettoval;
    cumAckInterval = cumacktoval;
    emptyInterVal = niltoval;
    maxRetrans = maxret;
    maxCumAck = maxcumack;
    maxOutSeq = maxoutseq;
    maxAutoReset = maxautorst;
  }
  int SYNFragment::getMaxFragment()
  {
    return maxFragment;
  }
  int SYNFragment::getMaxFragmentSize()
  {
    return maxFragmentSize;
  }
  int SYNFragment::getRetransmissionIntervel()
  {
    return retransInterval;
  }
  int SYNFragment::getCummulativeAckInterval()
  {
    return cumAckInterval;
  }
  int SYNFragment::getNulFragmentInterval()
  {
    return emptyInterVal;
  }

  int SYNFragment::getMaxRetrans()
  {
    return maxRetrans;
  }

  int SYNFragment::getMaxCumulativeAcks()
  {
    return maxCumAck;
  }
  int SYNFragment::getMaxOutOfSequence()
  {
    return maxOutSeq;
  }
  int SYNFragment::getMaxAutoReset()
  {
    return maxAutoReset;
  }

  Byte* SYNFragment::getData()
  {
    logDebug("MSG","FRT","get SYNC data");
    Byte *buffer = (Byte*)malloc(SYN_DATA_LEN);
    buffer[0] = (Byte) (maxFragment & 0xFF);
    buffer[1] = (Byte) ((maxFragmentSize >> 8) & 0xFF);
    buffer[2] = (Byte) ((maxFragmentSize >> 0) & 0xFF);
    buffer[3] = (Byte) ((retransInterval >> 8) & 0xFF);
    buffer[4] = (Byte) ((retransInterval >> 0) & 0xFF);
    buffer[5] = (Byte) ((cumAckInterval >> 8) & 0xFF);
    buffer[6] = (Byte) ((cumAckInterval >> 0) & 0xFF);
    buffer[7] = (Byte) ((emptyInterVal >> 8) & 0xFF);
    buffer[8] = (Byte) ((emptyInterVal >> 0) & 0xFF);
    buffer[9] = (Byte) (maxRetrans & 0xFF);
    buffer[10] = (Byte) (maxCumAck & 0xFF);
    buffer[11] = (Byte) (maxOutSeq & 0xFF);
    buffer[12] = (Byte) (maxAutoReset & 0xFF);
    return buffer;
  }
  int SYNFragment::getlength()
  {
    return SYN_DATA_LEN;
  }

  void SYNFragment::parseData(Byte* buffer, int len)
  {
    //logDebug("MSG","FRT","parse SYN fragment data len [%d]",len);
    if(buffer==NULL)
    {
      logDebug("MSG","FRT","SYN fragment is null ");
      return;
    }
    maxFragment = (buffer[0] & 0xFF);
    maxFragmentSize = ((buffer[1] & 0xFF) << 8) | ((buffer[2] & 0xFF) << 0);
    retransInterval = ((buffer[3] & 0xFF) << 8) | ((buffer[4] & 0xFF) << 0);
    cumAckInterval = ((buffer[5] & 0xFF) << 8) | ((buffer[6] & 0xFF) << 0);
    emptyInterVal = ((buffer[7] & 0xFF) << 8) | ((buffer[8] & 0xFF) << 0);
    maxRetrans = (buffer[9] & 0xFF);
    maxCumAck = (buffer[10] & 0xFF);
    maxOutSeq = (buffer[11] & 0xFF);
    maxAutoReset = (buffer[12] & 0xFF);
  }

  fragmentType SYNFragment::getType()
  {
    return FRAG_SYN;
  }

  // End-----Syn Fragment Class


  //--------------------------------------------------
  // ACK Fragment
  //--------------------------------------------------
  ACKFragment::ACKFragment()
  {

  }
  ACKFragment::ACKFragment(int seqn, int ackn)
  {
    initFragment(ACK_FLAG, seqn);
    setAck(ackn);
  }
  fragmentType ACKFragment::getType()
  {
    return FRAG_ACK;
  }

  Byte* ACKFragment::getData()
  {
    return NULL;
  }



  // End ACK Fragment Class

  //-------------------------------------------
  // NAK Fragment
  //-------------------------------------------
  NAKFragment::NAKFragment()
  {
  }

  NAKFragment::NAKFragment(int seqn, int ackn,  int* acks, int size)
  {
    initFragment(NAK_FLAG, seqn);
    setAck(ackn);
    nakNumber=size;
    nakData=acks;
  }

  Byte* NAKFragment::getData()
  {
    Byte *buffer = new Byte[nakNumber*2];
    for (int i = 0; i < nakNumber; i++)
    {
      int nakValue= nakData[i];
      buffer[i*2] = (Byte) ((nakValue >> 8) & 0xFF);
      buffer[i*2+1] = (Byte) ((nakValue >> 0) & 0xFF);
    }
    return buffer;
  }

  void NAKFragment::parseData(Byte* buffer, int len)
  {
    nakNumber = len/2;
    nakData = (int*)malloc(nakNumber*sizeof(int));
    for (int i = 0; i < nakNumber; i++)
    {
      nakData[i]=((buffer[i*2] & 0xFF) << 8) | ((buffer[i*2+1] & 0xFF) << 0);
    }
  }
  fragmentType NAKFragment::getType()
  {
    return FRAG_NAK;
  }
  int* NAKFragment::getNAKs(int* length)
  {
    *length= nakNumber;
    return nakData;
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
    initFragment(FIN_FLAG, seqn);
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
    initFragment(NUL_FLAG, seqn);
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
    initFragment(RST_FLAG, seqn);
  }
  fragmentType RSTFragment::getType()
  {
    return FRAG_RST;
  }
  // Ed RSTFragment Class.

}

