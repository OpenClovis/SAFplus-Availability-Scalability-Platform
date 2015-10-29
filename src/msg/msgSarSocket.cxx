#include <clMsgSarSocket.hxx>

//

namespace SAFplus
  {

    static const unsigned int MSG_SAR_RECV_OVER_ALLOC = 64;

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
      if (xport) xport->transport->deleteSocket(xport);
      xport=NULL;
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

    void MsgSarSocket::setLastChunk(Message* msg)
    {
      uint8_t msgId;
      uint8_t* ptr = (uint8_t*) msg->firstFragment->data(0);
      memcpy(&msgId,ptr,sizeof(uint8_t));
      msgId |= 1;
      memcpy(ptr,&msgId,sizeof(uint8_t));
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

    void MsgSarSocket::addSarHeader(Message* msg, uint msgNum, uint chunkNum)
    {
              if (msg->firstFragment->start >= SAR_HEADER_LEN)  // There is enough unused prefix room for this header
                {                
                  msg->firstFragment->start -= SAR_HEADER_LEN;
                  msg->firstFragment->len += SAR_HEADER_LEN;
                  uint_t hdrLen = setHeader(msg->firstFragment->data(0),(msgNum*2),chunkNum);  
                  assert(hdrLen == SAR_HEADER_LEN);  // for this code to work I need a constant header length
                }
              else // We need to allocate a new fragment
                {
                MsgFragment* sarHeader;
                sarHeader = headerPool->allocMsgFragment(SAR_HEADER_LEN);
                uint_t hdrLen = setHeader(sarHeader->data(0),(msgNum*2),chunkNum);
                sarHeader->used(hdrLen);
                msg->prependFrag(sarHeader);
                }
    }

    void MsgSarSocket::send(Message* origMsg)
    {
      Message* msg;
      Message* nextMsg = origMsg;
      MsgFragment* nextFrag;
      MsgFragment* frag;
      MsgFragment* prevFrag;


      Message* newMsg = NULL;
      
      

      do 
        {
          msg = nextMsg;
          nextMsg = msg->nextMsg;  // Save the next message so we use it next

          uint64_t len = msg->getLength();

          msgNum++;
          int chunkCount=0;  // what packet # (underlying message) is this.

          addSarHeader(msg,msgNum,0);

          if (len + SAR_HEADER_LEN < xport->cap.maxMsgSize)  // This message is smaller than the underlying maximum size so I can just send it
            {
            }
          else
            {
              int chunkSize=0;  // How big this groups of fragments has gotten.
              nextFrag = msg->firstFragment;
              prevFrag = NULL;
              do 
                {
                frag = nextFrag;
                nextFrag = frag->nextFragment;                
                if (chunkSize + frag->len > xport->cap.maxMsgSize)  // At this fragment, the message becomes too big.
                  {
                      chunkCount++;
                      // Create a new message and initialize it to the old message's values
                      Message* split = msgPool->allocMsg();
                      split->node = msg->node;
                      split->port = msg->port;
                      split->msgPool = msg->msgPool;
                      //split->setAddress(msg->getAddress());
                      // allocate a header for it
                      uint_t fillAmt = xport->cap.maxMsgSize-chunkSize;  // This is how much can be left in the original fragment
                      if (fillAmt == 0)  // In this case we just need to move the fragment to a new message not split it.
                        {
                          split->prependFrag(frag);
                          addSarHeader(split,msgNum,chunkCount);
                          assert(prevFrag);  // Its impossible that this if happen in the first fragment so prevFrag MUST have a value
                          msg->lastFragment = prevFrag;
                          prevFrag->nextFragment = NULL;
                          nextFrag = frag;  // I need to recheck this frag in its new position as first in the message
                          frag = split->firstFragment;
                          if (frag == nextFrag) frag = NULL;  // I don't want to add frag->len twice if no header needed to be added
                        }
                      else  // Split the fragment
                        {
                        MsgFragment* hdr = headerPool->allocMsgFragment(SAR_HEADER_LEN);
                        uint_t hdrLen = setHeader(hdr->data(0),msgNum*2,chunkCount);
                        hdr->used(hdrLen);
                        // allocate a fragment that will be used to split the current fragment up
                        MsgFragment* splitFrag = msgPool->allocMsgFragment(0);
                        splitFrag->set(frag->data(fillAmt),frag->len - fillAmt);  // update the buffer of the new fragment.  This frag does NOT manage the memory since we didn't allocate any upon fragment creation.
                        frag->len = fillAmt; // chop the original fragment to the max message size. 

                        // hook the new fragments into the new message
                        split->firstFragment = hdr;
                        hdr->nextFragment = splitFrag;
                        splitFrag->nextFragment = nextFrag;  // hook the rest of the fragments into the new message
                        split->lastFragment = msg->lastFragment;
                        // remove them from the old message
                        msg->lastFragment = frag;
                        frag->nextFragment = NULL;
                        frag = hdr; // Set the iteration variables to the new msg and frag.  The old one is finished off.
                        nextFrag = frag->nextFragment;
                        }

                      // hook the new message into the message list
                      split->nextMsg = nextMsg;
                      msg->nextMsg = split;

                      // Set the iteration variables to the new msg and frag.  The old one is finished off.
                      msg = split;                       
                      chunkSize = 0;  // I can set the chunkSize to 0 here because it will be incremented by the frag length at the bottom of the loop                      
                  }
                prevFrag = frag;
                if (frag) chunkSize += frag->len;                              
                } while(nextFrag);
            }
          setLastChunk(msg);              
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
        Message* m = xport->receive(maxMsgs,curDelay);
        if (!m) return NULL;  // Timeout

        Handle from = m->getAddress();
        uint_t msgId;
        uint_t packetIdx;
        bool last = consumeHeader(m,&msgId, &packetIdx);

        //logDebug("SAR","RCV","msgId [%d] chunk[%d] last [%d]", msgId, packetIdx, last);

        if (last && (packetIdx == 0))  // If this packet was both the last and the first then just return it.
          {
            return m;
          }
        else
          {
            MsgSarIdentifier sarId(from,msgId);
            MsgSarMap::iterator item = received.find(sarId);
            if (item == received.end())
              {
                item = received.insert(MsgSarMap::value_type(sarId,MsgSarTracker())).first;
                //received.insert(MsgSarMap::value_type(sarId,MsgSarTracker()));
              }

            MsgSarTracker& trk = item->second;
            if (last)
              {
                trk.last = packetIdx;
              }
            if (trk.msgs.capacity() <= packetIdx)
              trk.msgs.reserve(packetIdx + MSG_SAR_RECV_OVER_ALLOC);
            // TODO what if I receive the same packet twice?
            trk.msgs[packetIdx] = m;
            trk.count++;

            if ((trk.last > 0) && (trk.count == trk.last+1))  // packetIdx starts a t 0 
              {
                Message* prev = NULL;
                std::vector<Message*>::iterator it;
#if 0  // this is equivalent to the for loop below but not working
                for (it = trk.msgs.begin(); it != trk.msgs.end(); it++)
                  {
                    if (prev) prev->nextMsg = (*it);
                    prev = *it;
                  }
#endif
                for (int i=0; i < trk.count; i++)
                  {
                    Message* im=trk.msgs[i];
                    assert(im);  // All chunks should be received because trk.count == trk.last+1.  Note this will fail if I receive a double packet
                    if (prev) prev->nextMsg = im;
                    prev = im;
                  }
                Message* top = trk.msgs[0];
                MsgFragment* lastFrag = top->lastFragment;
                Message* cur = top->nextMsg;
                top->nextMsg = NULL; // I am combining all chunks into one message
                while(cur != NULL) 
                  {
                    lastFrag->nextFragment = cur->firstFragment;  // attach all these fragments to the top message
                    lastFrag = cur->lastFragment;
                    Message* old = cur;
                    cur=old->nextMsg;
                    old->nextMsg=NULL;  // clear these out so they don't get cleaned up
                    old->firstFragment=NULL;
                    old->lastFragment=NULL;
                    MsgPool* mp = old->msgPool;
                    mp->free(old);  // free the message without freeing its fragments or linked messages
                  }
                received.erase(item);  // remove this message from the lookup table
                return top;
              }
          }
        }  
        
    }
 
    void MsgSarSocket::useNagle(bool value)
    {
      xport->useNagle(value);
    }

  };
