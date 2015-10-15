#include <clMsgSarSocket.hxx>

//

namespace SAFplus
  {

    MsgSarSocket::MsgSarSocket(MsgSocket* underlyingSocket)
    {
      assert(underlyingSocket);
      xport = underlyingSocket;
      cap = xport->cap;  // This layer is the same as the underlying transport EXCEPT...
      cap.maxMsgSize = UINT_MAX;  // This layer adds unlimited size messages
      headerPool = underlyingSocket->getMsgPool();  // TODO: create a custom header pool optimized for the SAR header size
      msgPool = underlyingSocket->getMsgPool();
      msgNum=0;

      node = xport->node;
      port = xport->port;
    }

    MsgSarSocket::~MsgSarSocket()
    {
    }


    static const unsigned int SAR_HEADER_LEN = sizeof(uint8_t) + sizeof(uint16_t);

    // header is: 8-bits of msgNum.  LSB is 1 if last message.  16-bits offset (network ordered)
    int MsgSarSocket::setHeader(void* ptr, uint msgNum, uint offset)
    {
      uint8_t m = msgNum;
      uint16_t i = offset;
      // store length
      // On the receiving side, this is uniquified by the incoming address so we simply need to send the length and packet count
      // and mutex protect the send code.
      //m = htonb(m);
      i = htons(i);
      memcpy(ptr,&m,sizeof(uint8_t));
      memcpy(((char*)ptr)+1,&i,sizeof(uint16_t));
      return sizeof(uint8_t) + sizeof(uint16_t);
    }

    bool MsgSarSocket::consumeHeader(Message* m, uint* msgNum, uint* offset)
    {
      assert(msgNum); assert(offset);
      MsgFragment* frag = m->firstFragment;
      uint8_t* ptr = (uint8_t*) frag->data(0);
      uint8_t msgId;
      uint16_t i;
      memcpy(&msgId,ptr,sizeof(uint8_t));
      memcpy(&i,ptr+1,sizeof(uint16_t));
      bool last = msgId&1;
      *msgNum = msgId/2;     
      i = ntohs(i);
      *offset = i;
      // Move internal data inside the fragment forward to "consume" the header
      frag->start += sizeof(uint8_t) + sizeof(uint16_t);  
      frag->len -= sizeof(uint8_t) + sizeof(uint16_t);
      return last;
    }



    void MsgSarSocket::send(Message* origMsg)
    {
      Message* msg;
      Message* nextMsg = origMsg;
      MsgFragment* nextFrag;
      MsgFragment* frag;


      Message* outMsg;
      
      

      do 
        {
          msg = nextMsg;
          nextMsg = msg->nextMsg;  // Save the next message so we use it next

          uint64_t len = msg->getLength();

          msgNum++;
            
          if (len + SAR_HEADER_LEN < xport->cap.maxMsgSize)  // This message is smaller than the underlying maximum size so I can just send it
            {
              if (msg->firstFragment->start >= SAR_HEADER_LEN)  // There is enough unused prefix room for this header
                {                
                  msg->firstFragment->start -= SAR_HEADER_LEN;
                  msg->firstFragment->len += SAR_HEADER_LEN;
                  uint_t hdrLen = setHeader(msg->firstFragment->data(0),(msgNum*2)+1,0);  // This is both the first and last message
                  assert(hdrLen == SAR_HEADER_LEN);  // for this code to work I need a constant header length
                }
              else // We need to allocate a new fragment
                {
                MsgFragment* sarHeader;
                sarHeader = headerPool->allocMsgFragment(SAR_HEADER_LEN);
                uint_t hdrLen = setHeader(sarHeader->data(0),(msgNum*2)+1,0);  // This is both the first and last message
                sarHeader->used(hdrLen);
                msg->prependFrag(sarHeader);
                }
            }
          else
            {
              assert(0);  // not implemented
#if 0                 
              fragCount=0;
              nextFrag = msg->firstFragment;
              do {
                frag = nextFrag;
                nextFrag = frag->nextFragment;

          

                fragCount++;
                totalFragCount++;
                curIov++;
              } while(nextFrag);
#endif
            }
          
        } while (nextMsg != NULL);
      xport->send(origMsg);
    }

    void MsgSarSocket::flush()
    {
      xport->flush();
    }

    Message* MsgSarSocket::receive(uint_t maxMsgs,int maxDelay)
    {
      int curDelay=maxDelay;

      while(1)
        {
        Message* m = xport->receive(curDelay);
        if (!m) return NULL;  // Timeout

        Handle from = m->getAddress();
        uint_t msgId;
        uint_t packetIdx;
        bool last = consumeHeader(m,&msgId, &packetIdx);

        if (last && (packetIdx == 0))  // If this packet was both the last and the first then just return it.
          {
            return m;
          }
        else
          {
            assert(0); // Not implemented
          }
        }  
        
    }
 
    void MsgSarSocket::useNagle(bool value)
    {
      xport->useNagle(value);
    }

  };
