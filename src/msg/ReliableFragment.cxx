#include "ReliableFragment.hxx"
namespace SAFplus
{
    ReliableFragment::ReliableFragment()
    {
    // To do
       int a=0;
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
        return m_nRetCounter;
    }
    int ReliableFragment::getAck()
    {
        if(m_nFalgs & ACK_FLAG == ACK_FLAG)
        {
            return m_nAckn;
        }
        return -1;
    }
    Byte* ReliableFragment::getACKs(int* length)
    {
        return nullptr;
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
      return FRAG_UDE;
    }
    Byte* ReliableFragment::getBytes()
    {
        Byte *pBuffer = new Byte[length()];
        pBuffer[0] = (Byte) (m_nFalgs & 0xFF);
        pBuffer[1] = (Byte) (m_nLen & 0xFF);
        pBuffer[2] = (Byte) (m_nSeqn & 0xFF);
        pBuffer[3] = (Byte) (m_nAckn & 0xFF);
        return pBuffer;
    }

    void ReliableFragment::init(int _flags, int _seqn, int len)
    {
        m_nFalgs = _flags;
        m_nSeqn = _seqn;
        m_nLen = len;
    }

    void ReliableFragment::parseBytes(const Byte* buffer, int _off, int _len)
    {
        m_nFalgs = (buffer[_off] & 0xFF);
        m_nLen   = (buffer[_off+1] & 0xFF);
        m_nSeqn  = (buffer[_off+2] & 0xFF);
        m_nAckn  = (buffer[_off+3] & 0xFF);
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
          fragment = new FINFragment();
       }
       else if ((flags & ACK_FLAG) != 0)
       {
           /* always process ACKs or Data segments last */
           if (len == RUDP_HEADER_LEN)
           {
               fragment = new ACKFragment();
           }
           else
           {
               fragment = new DATFragment();
           }
       }

       if (fragment == nullptr)
       {
           //throw new IllegalArgumentException("Invalid segment");
           throw Error("Invalid segment");
       }
       fragment->parseBytes(bytes, off, len);
       return fragment;
    }
   ReliableFragment* ReliableFragment::parse(Byte* bytes)
   {
      int length = 0;
      return ReliableFragment::parse(bytes, 0, length);
   }
	//--- End Reliable Fragment

   //------------------------------
   // Data Fragment
   //------------------------------
   DATFragment::DATFragment()
   {

   }

   DATFragment::DATFragment(int seqn, int ackn,const Byte* buffer, int off, int len)
   {
      init(ACK_FLAG, seqn, RUDP_HEADER_LEN);
      setAck(ackn);
      m_nLen = len;
      m_pData = new Byte[m_nLen];
      memcpy(m_pData, buffer + off, len);
   }

   int DATFragment::length()
   {
      //int dataLength = 0;
      return (m_nLen + ReliableFragment::length() );
   }

   Byte* DATFragment::getData()
   {
      return m_pData;
   }

   Byte* DATFragment::getBytes()
   {
      Byte* buffer = ReliableFragment::getBytes();
      memcpy(buffer, m_pData + RUDP_HEADER_LEN, m_nLen);
      return buffer;
   }

   void DATFragment::parseBytes(const Byte* buffer, int _off, int _len)
   {
      ReliableFragment::parseBytes(buffer, _off, _len);
      m_nLen = _len - RUDP_HEADER_LEN;
      m_pData = new Byte[m_nLen];
      memcpy(m_pData, buffer + _off + RUDP_HEADER_LEN, m_nLen);
      // System.arraycopy(buffer, off+RUDP_HEADER_LEN, _data, 0, _data.length);
   }
   fragmentType DATFragment::getType()
   {
      return FRAG_DATA;
   }
// End DATFragment class

   SYNFragment::SYNFragment()
   {

   }

    SYNFragment::SYNFragment(int seqn, int maxseg, int maxsegsize, int rettoval,
            int cumacktoval, int niltoval, int maxret,
            int maxcumack, int maxoutseq, int maxautorst)
    {
        init(SYN_FLAG, seqn, SYN_HEADER_LEN);
        m_nVersion = RUDP_VERSION;
        m_nMaxseg = maxseg;
        m_nOptflags = 0x01; /* no options */
        m_nMaxsegsize = maxsegsize;
        m_nRettoval = rettoval;
        m_nCumacktoval = cumacktoval;
        m_nNiltoval = niltoval;
        m_nMaxret = maxret;
        m_nMaxcumack = maxcumack;
        m_nMaxoutseq = maxoutseq;
        m_nMaxautorst = maxautorst;
    }
    int SYNFragment::getVersion()
    {
        return m_nVersion;
    }
    int SYNFragment::getMaxOutstandingFragments()
    {
        return m_nMaxseg;
    }

    int SYNFragment::getOptionFlags()
    {
        return m_nOptflags;
    }

    int SYNFragment::getMaxFragmentSize()
    {
        return m_nMaxsegsize;
    }
    int SYNFragment::getRetransmissionTimeout()
    {
        return m_nRettoval;
    }
    int SYNFragment::getCummulativeAckTimeout()
    {
        return m_nCumacktoval;
    }
    int SYNFragment::getNulFragmentTimeout()
    {
        return m_nNiltoval;
    }

    int SYNFragment::getMaxRetransmissions()
    {
        return m_nMaxret;
    }

    int SYNFragment::getMaxCumulativeAcks()
    {
        return m_nMaxcumack;
    }
    int SYNFragment::getMaxOutOfSequence()
    {
        return m_nMaxoutseq;
    }
    int SYNFragment::getMaxAutoReset()
    {
        return m_nMaxautorst;
    }

    Byte* SYNFragment::getBytes()
    {
        Byte *buffer = ReliableFragment::getBytes();
        buffer[4] = (Byte) ((m_nVersion << 4) & 0xFF);
        buffer[5] = (Byte) (m_nMaxseg & 0xFF);
        buffer[6] = (Byte) (m_nOptflags & 0xFF);
        buffer[7] = 0; /* spare */
        buffer[8] = (Byte) ((m_nMaxsegsize >> 8) & 0xFF);
        buffer[9] = (Byte) ((m_nMaxsegsize >> 0) & 0xFF);
        buffer[10] = (Byte) ((m_nRettoval >> 8) & 0xFF);
        buffer[11] = (Byte) ((m_nRettoval >> 0) & 0xFF);
        buffer[12] = (Byte) ((m_nCumacktoval >> 8) & 0xFF);
        buffer[13] = (Byte) ((m_nCumacktoval >> 0) & 0xFF);
        buffer[14] = (Byte) ((m_nNiltoval >> 8) & 0xFF);
        buffer[15] = (Byte) ((m_nNiltoval >> 0) & 0xFF);
        buffer[16] = (Byte) (m_nMaxret & 0xFF);
        buffer[17] = (Byte) (m_nMaxcumack & 0xFF);
        buffer[18] = (Byte) (m_nMaxoutseq & 0xFF);
        buffer[19] = (Byte) (m_nMaxautorst & 0xFF);
        return buffer;
    }

    void SYNFragment::parseBytes(const  Byte* buffer, int off, int len)
    {
        ReliableFragment::parseBytes(buffer, off, len);
        if (len < (SYN_HEADER_LEN) )
        {
            //throw new IllegalArgumentException("Invalid SYN Fragment");
           throw Error("Invalid SYN Fragment");
        }
        m_nVersion = ((buffer[off + 4] & 0xFF) >> 4);
        if (m_nVersion != RUDP_VERSION)
        {
           //throw new IllegalArgumentException("Invalid RUDP version");
           throw Error("Invalid RUDP version");
        }
        m_nMaxseg = (buffer[off + 5] & 0xFF);
        m_nOptflags = (buffer[off + 6] & 0xFF);
        // spare     =  (buffer[off+ 7] & 0xFF);
        m_nMaxsegsize = ((buffer[off + 8] & 0xFF) << 8) | ((buffer[off + 9] & 0xFF) << 0);
        m_nRettoval = ((buffer[off + 10] & 0xFF) << 8) | ((buffer[off + 11] & 0xFF) << 0);
        m_nCumacktoval = ((buffer[off + 12] & 0xFF) << 8) | ((buffer[off + 13] & 0xFF) << 0);
        m_nNiltoval = ((buffer[off + 14] & 0xFF) << 8) | ((buffer[off + 15] & 0xFF) << 0);
        m_nMaxret = (buffer[off + 16] & 0xFF);
        m_nMaxcumack = (buffer[off + 17] & 0xFF);
        m_nMaxoutseq = (buffer[off + 18] & 0xFF);
        m_nMaxautorst = (buffer[off + 19] & 0xFF);
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
       init(ACK_FLAG, seqn, RUDP_HEADER_LEN);
       setAck(ackn);
    }
    fragmentType ACKFragment::getType()
    {
        return FRAG_ACK;
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
       init(NAK_FLAG, seqn, RUDP_HEADER_LEN + size);
       setAck(ackn);
       m_pArrAcks = (Byte*)malloc(size);
       memcpy(m_pArrAcks, acks, size);
    }

    Byte* NAKFragment::getBytes()
    {
       Byte* buffer= ReliableFragment::getBytes();
       for (int i = 0; i < m_nNumNak; i++)
       {
          buffer[4+i] = (Byte) (m_pArrAcks[i] & 0xFF);
       }
       return buffer;
    }

    void NAKFragment::parseBytes(const Byte* buffer, int off, int len)
    {
       ReliableFragment::parseBytes(buffer, off, len);
       m_nNumNak = len - RUDP_HEADER_LEN;
       m_pArrAcks = new Byte[m_nNumNak];
       for (int i = 0; i < m_nNumNak; i++)
       {
          m_pArrAcks[i] = (buffer[off + 4 + i] & 0xFF);
       }
    }
    fragmentType NAKFragment::getType()
    {
       return FRAG_NAK;
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
       init(FIN_FLAG, seqn, RUDP_HEADER_LEN);
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
       init(NUL_FLAG, seqn, RUDP_HEADER_LEN);
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
       init(RST_FLAG, seqn, RUDP_HEADER_LEN);
    }
    fragmentType RSTFragment::getType()
    {
       return FRAG_RST;
    }
    // Ed RSTFragment Class.

}

