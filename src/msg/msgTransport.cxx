#include <clMsgApi.hxx>
#include <clLogApi.hxx>
#include <clMsgBase.hxx>
#include <boost/tokenizer.hpp>

namespace SAFplus
  {

  void Message::setAddress(const Handle& h)
    {
    node = h.getNode();
    port = h.getPort();
    }

  void Message::prependFrag(MsgFragment* frag)
  {
      frag->nextFragment = firstFragment;
      firstFragment = frag;
      if (!lastFragment) lastFragment = frag;  // If this is the very first message fragment
  }

    uint_t Message::copy(void* data, uint_t offset, uint_t maxlen)
    {
      int maxlength = (maxlen > INT_MAX) ? INT_MAX: (int) maxlen;  // numbers higher than INT_MAX basically mean "the whole thing"
      uint8_t* ptr = (uint8_t*) data;
      assert(ptr);
      int loc = offset;
      uint_t amtCopied = 0;
      // advance to the position
      MsgFragment* frag;
      for (frag = firstFragment; frag != NULL; frag = frag->nextFragment)
        {
          loc -= frag->len;
          if (loc <= 0) break;
        }

      if (frag)
        {
          if (loc < 0)  // Copy the portion in this fragment.
            {            
              int amt2copy = std::min(-loc, maxlength); 
              memcpy(ptr,frag->read(frag->len + loc),amt2copy);  // actually subtracting, loc is negative
              maxlength -= amt2copy; 
              amtCopied += amt2copy;
              ptr+=amt2copy;
            }
          frag = frag->nextFragment;

          // Copy the rest
          for (; (frag != NULL) && (maxlength>0); frag = frag->nextFragment)
            {
              int amt2copy = std::min((int) frag->len, maxlength); 
              memcpy(ptr,frag->read(0),amt2copy);
              maxlength -= amt2copy; 
              amtCopied += amt2copy;            
              ptr+=amt2copy;
            }
        }
      
      return amtCopied;
    }

    void* Message::flatten(void* data, uint_t offset, uint_t length,uint_t align)
    {
      int loc = offset;
      uint_t amtCopied = 0;

      // advance to the position
      MsgFragment* frag;
      for (frag = firstFragment; frag != NULL; frag = frag->nextFragment)
        {
          loc -= frag->len;
          if (loc <= 0) break;
        }
      if (!frag) return NULL; // offset is beyond the end of the message

      uint8_t* out = (uint8_t*) data;
      if (align>1)  // Align the out pointer
        {
          out = (uint8_t*) ((((uintptr_t)out)+(align-1))&(~(align-1)));
        }
      uint8_t* ret = out;

      if (loc < 0)  // Copy the portion in this fragment.
        {            
          int amt2copy = std::min(-loc, (int) length); 
          if (amt2copy <= -loc) // All requested data is in one fragment
            {
              uint8_t* ptr = (uint8_t*) frag->read(frag->len + loc);
              if (align==0 || align==1 || (((uintptr_t)ptr)&(align-1)==0)) return ptr; // The pointer is properly aligned (and contains all requested data) so just return it
            }

          memcpy(out,frag->read(frag->len + loc),amt2copy);
          length -= amt2copy; 
          out+=amt2copy;
        }

      // Copy the rest
      for (; (frag != NULL)&&length; frag = frag->nextFragment)
        {
          int amt2copy = std::min(frag->len, length); 
          memcpy(out,frag->read(0),amt2copy);
          length -= amt2copy; 
          out+=amt2copy;
        }      

      if (length) return NULL;  // unable to copy the whole thing
      return ret;
    }

  
  MsgFragment* Message::prepend(uint_t size)
    {
      MsgFragment* f = msgPool->allocMsgFragment(size);
      assert(f);
      f->nextFragment = firstFragment;
      firstFragment = f;
      if (!lastFragment) lastFragment = f;  // If this is the very first message fragment
      return f;
    }

  MsgFragment* Message::append(uint_t size)
    {
      MsgFragment* f = msgPool->allocMsgFragment(size);
      assert(f);
      if (!lastFragment) { lastFragment = f; firstFragment = f; }  // This is the first fragment
      else
        {
        lastFragment->nextFragment = f;
        lastFragment = f;
        }
      return f;
    }
   void Message::deleteLastFragment()
    {
      MsgFragment* cur = firstFragment;
      assert(cur);
      MsgFragment* next= cur->nextFragment;
      assert(next);
      while (next->nextFragment != NULL);
      {
         cur = next;
         next = cur->nextFragment;
      }
      cur->nextFragment=NULL;
      lastFragment=cur;
      // First clean up the buffer if needed
      msgPool->freeFragment(next);
    }
  u_int Message::getLength()
    {
	  u_int len=0;
      MsgFragment* nextFrag;
      MsgFragment* frag;
      nextFrag = firstFragment;
      do
      {
        frag = nextFrag;
        nextFrag = frag->nextFragment;
        len+=frag->len;
      }while(nextFrag);
      // msg type
      //len = len -1;
      return len;
    }

  MsgFragment* MsgPool::allocMsgFragment(uint_t size)
    {
      fragAllocated++;
      MsgFragment* ret = NULL;
      if (size == 0)
        {
        ret = (MsgFragment*) SAFplusHeapAlloc(sizeof(MsgFragment));
        assert(ret);
        ret->constructPointerFrag();
        }
      else
        {
        ret = (MsgFragment*) SAFplusHeapAlloc(sizeof(MsgFragment) + size);  // actually could be size - sizeof(void*) because we'll use the buffer variable data itself
        assert(ret);
        ret->constructInlineFrag(size);
        fragAllocatedBytes += size;
        fragCumulativeBytes += size;
        }
      return ret;
    }

  void MsgFragment::set(const char* buf)
    {
    if (flags & PointerFragment)
      {
      start  = 0;
      len    = strlen(buf)+1;  // Include the NULL termination 
      buffer = (void*) buf;
      }
    else if (flags & InlineFragment)
      {
      clDbgNotImplemented("assignment into an inline frag");
      }
    else
      {
      assert(!"Message fragment corrupt");
      }
    }

  void MsgFragment::set(void* buf,uint_t length)
    {
    if (flags & PointerFragment)
      {
      start  = 0;
      len    = length;  // Include the NULL termination 
      buffer = (void*) buf;
      }
    else if (flags & InlineFragment)
      {
      clDbgNotImplemented("assignment into an inline frag");
      }
    else
      {
      assert(!"Message fragment corrupt");
      }
    }



  void* MsgFragment::data(int offset)
    {
    if (flags & PointerFragment)
      {
      return (void*) (((char*) buffer)+offset+start);
      }
    else if (flags & InlineFragment)
      {
      assert(offset+start < allocatedLen);
      return (void*) (((char*) &buffer)+offset+start);
      }
    }

    int MsgFragment::append(const char* s, int n)
    {
      //assert(size+offset+start < allocatedLen);
      int availableSpace = allocatedLen - (start+len);
      if (availableSpace==0) return 0;
      int writeAmt = std::min(n,availableSpace);
      char* d = (char*) data(len);  // I'm adding to the end so I want the pointer offset by len
      memcpy(d,s,writeAmt);
      len += writeAmt;
      return writeAmt;
    }


  const void* MsgFragment::read(int offset)
    {
    if (offset == len) return NULL; // especially needed for 0 length fragments
    assert(offset < len);  // not offset+start because the len does not include start
    if (flags & PointerFragment)
      {
      return (void*) (((char*) buffer)+offset+start);
      }
    else if (flags & InlineFragment)
      {
      return (void*) (((char*) &buffer)+offset+start);
      }
    }




  Message* MsgPool::allocMsg()
    {
    Message* msg = (Message*) SAFplusHeapAlloc(sizeof(Message));
    assert(msg);
    msg->initialize();
    msg->msgPool = this;
    allocated++;
    return msg;
    }

  void MsgPool::free(Message* next)
    {
    Message* msg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    do {
       msg = next;
       next = msg->nextMsg;  // Save the next message so we can delete it next

       nextFrag = msg->firstFragment;
       if (nextFrag) do {
          frag = nextFrag;
          nextFrag = frag->nextFragment;

          // First clean up the buffer if needed
          if (frag->flags & MsgFragment::Flags::DataSAFplusFree) SAFplusHeapFree(frag->buffer);
          else if (frag->flags & MsgFragment::Flags::DataMsgPoolFree) SAFplusHeapFree(frag->buffer);  // TODO: hold the fragments in lists in the message pool
          else if (frag->flags & MsgFragment::Flags::DataCustomFree) { clDbgNotImplemented("custom free logic"); }  // Should the custom free function be per fragment or per message?

          if (frag->flags & MsgFragment::Flags::InlineFragment) fragAllocatedBytes -= frag->allocatedLen;

          // Next clean up the fragment itself
          if (frag->flags & MsgFragment::Flags::SAFplusFree) SAFplusHeapFree(frag);
          else if (frag->flags & MsgFragment::Flags::MsgPoolFree) SAFplusHeapFree(frag);  // TODO: hold the fragments in lists in the message pool
          else if (frag->flags & MsgFragment::Flags::CustomFree) { clDbgNotImplemented("custom fragment free logic"); }
          fragAllocated--;
          } while(nextFrag);

       // Ok all frags cleaned up so delete this message
       SAFplusHeapFree(msg);
       allocated--;
       } while (next != NULL);
    }

  void MsgPool::freeFragment(MsgFragment* frag)
  {
    // First clean up the buffer if needed
    if (frag->flags & MsgFragment::Flags::DataSAFplusFree) SAFplusHeapFree(frag->buffer);
    else if (frag->flags & MsgFragment::Flags::DataMsgPoolFree) SAFplusHeapFree(frag->buffer);  // TODO: hold the fragments in lists in the message pool
    else if (frag->flags & MsgFragment::Flags::DataCustomFree) { clDbgNotImplemented("custom free logic"); }  // Should the custom free function be per fragment or per message?

    if (frag->flags & MsgFragment::Flags::InlineFragment) fragAllocatedBytes -= frag->allocatedLen;

    // Next clean up the fragment itself
    if (frag->flags & MsgFragment::Flags::SAFplusFree) SAFplusHeapFree(frag);
    else if (frag->flags & MsgFragment::Flags::MsgPoolFree) SAFplusHeapFree(frag);  // TODO: hold the fragments in lists in the message pool
    else if (frag->flags & MsgFragment::Flags::CustomFree) { clDbgNotImplemented("custom fragment free logic"); }
    fragAllocated--;
  }


  MsgSocket::~MsgSocket()
  {}

  void MsgSocket::useNagle(bool value)
    {
      if (value) logDebug("MSG","SCK","Nagle algorithm turned on but not supported by socket.");
    }

  void MsgTransportPlugin_1::registerWatcher(Wakeable* watcher)
    {
    // This is implemented as an expanding array of pointers.
    // registration and unregistration occurs rarely so performance is not an issue.
    // But speed iterating thru the notification list is.

    for(int i=0;i<numWatchers;i++)
      {
      if (watchers[i] == nullptr) 
        {
        watchers[i] = watcher;
        return;
        }
      if (watchers[i] == watcher)
        {
        return;
        }
      }
    uint_t newNum = numWatchers + 4;
    Wakeable** tmp = (Wakeable**) SAFplusHeapAlloc(newNum * sizeof(Wakeable*));
    assert(tmp);
    memcpy(tmp,watchers,numWatchers *sizeof(Wakeable**));
    tmp[numWatchers] = watcher;
    for(int i=numWatchers+1;i<newNum;i++)
      {
      tmp[i] = nullptr;
      }
    SAFplusHeapFree(watchers);
    watchers = tmp;
    numWatchers = newNum;
    }

  void MsgTransportPlugin_1::unregisterWatcher(Wakeable* watcher)
    {
    for(int i=0;i<numWatchers;i++)
      {
      if (watchers[i] == watcher)
        {
        watchers[i] = nullptr;
        // It should be impossible for the same watcher to be in the list more than once but I'll look thru the whole list just to be safe.
        }
      }
    }

  std::string MsgTransportPlugin_1::transportAddress2String(const void* transportAddr)
    {
      if (transportAddr == NULL) return std::string();

      std::ostringstream s;
      uint32_t val1, val2, val3, val4;
      uint32_t addr = *((uint32_t*) transportAddr);
      uint16_t port;
      memcpy(&port, ((char*)transportAddr)+sizeof(uint32_t),sizeof(uint16_t));

      val1 = addr & 0xff000000;
      val1 = val1 >> 24;
      val2 = addr & 0x00ff0000;
      val2 = val2 >> 16;
      val3 = addr & 0x0000ff00;
      val3 = val3 >> 8;
      val4 = addr & 0x000000ff;

      if (port)
        {
          s << val1 << '.' << val2 << '.' << val3 << '.' << val4 << ':' << port;
        }
      else
        {
          s << val1 << '.' << val2 << '.' << val3 << '.' << val4;
        }

      return s.str();    
    }

#if 0
  void MsgTransportPlugin_1::setClusterTracker(ClusterNodes* cn)
  {
    clusterNodes = cn;
    if (clusterNodes)
      {
        config.capabilities = (MsgTransportConfig::Capabilities) (config.capabilities | MsgTransportConfig::BROADCAST);  // Your plugin should be able to simulate a broadcast now...
      }
    else
      {
        config.capabilities = (MsgTransportConfig::Capabilities) (config.capabilities & ~MsgTransportConfig::BROADCAST);
      }
  }
#endif

  void MsgTransportPlugin_1::string2TransportAddress(const std::string& str, void* transportAddr, int* transportAddrLen)
    {
      assert(*transportAddrLen >= sizeof(uint32_t));
      boost::tokenizer<> tok(str);
      uint32_t addr=0;
      uint16_t port=0;
      int count=0;
      for(boost::tokenizer<>::iterator it=tok.begin(); it!=tok.end();it++,count++)
        {
          if (count < 4)
            {
            addr <<= 8;
            addr |= std::stoi(*it);
            }
          else
            {
            port = std::stoi(*it);
            }
        }
      *transportAddrLen=sizeof(uint32_t) + sizeof(uint16_t);
      memcpy(transportAddr,&addr,sizeof(uint32_t));
      memcpy(((char*)transportAddr)+sizeof(uint32_t),&port,sizeof(uint16_t));
    }

#if 0
    ClUint32T val[4] = { 0 };
    const char *token = NULL;
    const char *nextToken = NULL;
    ClInt32T n = 0;

    assert(*transportAddrLen >= 4);

    const char* ipAddress = str.c_str();

    token = strtok_r(ipAddress, ".", &nextToken);
    while (token && (n<4)) {
        val[n++] = atoi(token);
        token = strtok_r(NULL, ".", &nextToken);
    }


    uint32_t* addr=(uint32_t*) transportAddr;
    *addr |= val[3];
    val[2] <<= 8;
    *addr |= val[2];
    val[1] <<= 16;
    *addr |= val[1];
    val[0] <<= 24;
    *addr |= val[0];

    *transportAddrLen=4;
    //return true;   
    }
#endif

  //*****************Advanced socket : MsgSocketShaping********************
MsgSocketShaping::MsgSocketShaping(uint_t port,MsgTransportPlugin_1* transportp,uint_t volume, uint_t leakSize, uint_t leakInterval)
  {
    transport = transportp;
    xport=transport->createSocket(port);
    msgPool = xport->getMsgPool();
    assert(xport);
    bucket.start(volume,leakSize,leakInterval);
    node = xport->node;
    port = xport->port;
  };

MsgSocketShaping::MsgSocketShaping(MsgSocket* socket,uint_t volume, uint_t leakSize, uint_t leakInterval)
  {
    transport= socket->transport;
    xport=socket;
    msgPool = xport->getMsgPool();
    bucket.start(volume,leakSize,leakInterval);
    node = xport->node;
    port = xport->port;
  };

  MsgSocketShaping::~MsgSocketShaping()
  {
    bucket.stop();
    if (xport&&transport) transport->deleteSocket(xport);
    xport = NULL;
    transport = NULL;
  }
  void MsgSocketShaping::applyShaping(uint_t length)
  {
      bucket.fill(length);
  }
  //? Send a bunch of messages.  You give up ownership of msg.
  void MsgSocketShaping::send(Message* msg)
  {
    logDebug("MSG","SCK","apply leaky bucket with msg lenght %d to %d",msg->getLength(),msg->node);
    uint_t length = msg->getLength();
    applyShaping(length);
    xport->send(msg);
  }
  void MsgSocketShaping::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    assert(xport);
    Message* m = xport->msgPool->allocMsg();
    assert(m);
    m->setAddress(destination);
    MsgFragment* pfx  = m->append(1);
    * ((unsigned char*)pfx->data()) = msgtype;
    pfx->len = 1;
    MsgFragment* frag = m->append(0);
    frag->set(buffer,length);
    applyShaping(m->getLength());
    xport->send(m);
  }
  Message* MsgSocketShaping::receive(uint_t maxMsgs,int maxDelay)
  {
    return xport->receive(maxMsgs,maxDelay);
  }

  void MsgSocketShaping::flush()
  {
    xport->flush();
  }
  void MsgSocketAdvanced::flush()
  {
    xPort->flush();
  }
  std::streamsize MessageOStream::write(const char* s, std::streamsize n)
    {
      // Write up to n characters to the underlying 
      // data sink from the buffer s, returning the 
      // number of characters written
      int leftover = n;
      MsgFragment* frag = msg->lastFragment;

      while(leftover)
        {
          if (frag)
            {
              int written = frag->append(s,leftover);
              s += written;
              leftover -= written;
            }
          if (leftover)
            {
              int newFragSize = std::max(leftover,fragSize);
              frag = msg->append(newFragSize);
            }
        }
      return n;    
    }


    std::streamsize MessageIStream::read(char* s, std::streamsize n)
    {
      // Read up to n characters from the underlying data source
      // into the buffer s, returning the number of characters
      // read; return -1 to indicate EOF
      int leftover = n;

      if (!curFrag) return -1;  // at the end.

      while(curFrag && leftover)
        {
          int readable = curFrag->len - curOffset;
          int amt2Read = std::min(readable, leftover);
          if (amt2Read > 0)
            {
              memcpy(s,curFrag->read(curOffset),amt2Read);
              s += amt2Read;
              leftover -= amt2Read;
              curOffset += amt2Read;
            }
          if (leftover)
            {
              curFrag = curFrag->nextFragment;
              curOffset = 0;
            }
        }
       
      return n-leftover;    
    }


  };
