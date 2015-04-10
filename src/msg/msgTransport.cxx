#include <clMsgApi.hxx>
#include <clLogApi.hxx>
#include <clMsgBase.hxx>

namespace SAFplus
  {

  void Message::setAddress(const Handle& h)
    {
    node = h.getNode();
    port = h.getPort();
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
      len = len -1;
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
       do {
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

  //*****************Advanced socket : MsgSocketReliable***************

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



  //************Advanced socket : MsgSocketSegmentaion************

  MsgSocketSegmentaion::MsgSocketSegmentaion(uint_t port,MsgTransportPlugin_1* transport)
  {
    sock = transport->createSocket(port);
  }
  MsgSocketSegmentaion::MsgSocketSegmentaion(MsgSocket* socket)
  {
    sock = socket;
  }
  MsgSocketSegmentaion::~MsgSocketSegmentaion()
  {
    //TODO
  }
  void MsgSocketSegmentaion::applySegmentaion(Message* m, SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    u_int maxPayload=64000;
    u_int totalFragRequired;
    assert(m);
    m->setAddress(destination);
    MsgFragment* pfx  = m->append(1);
    * ((unsigned char*)pfx->data()) = msgtype;
    pfx->len = 1;
    if(length > maxPayload)
    {
      u_int start =0;
      totalFragRequired = length / maxPayload;
      while (totalFragRequired > 1)
      {
        MsgFragment* frag = m->append(0);
        if(length-start>=maxPayload)
        {
          frag->set((u_int8_t*)buffer+start,maxPayload);
        }
        else
        {
          frag->set((u_int8_t*)buffer+start,maxPayload-start);
        }
        start+=maxPayload;
      }
    }
  }
  void MsgSocketSegmentaion::applySegmentaion(Message* m)
  {
    u_int maxPayload=64000;
    u_int totalFragRequired;
    u_int16_t length = m->lastFragment->len;
    void* bufferData = SAFplusHeapAlloc(length);
    memcpy(bufferData,m->lastFragment->data(),length);
    assert(bufferData);
    if(length > maxPayload)
    {
      m->deleteLastFragment();
      u_int start =0;
      totalFragRequired = length / maxPayload;
      while (totalFragRequired > 1)
      {
        MsgFragment* frag = m->append(0);
        if(length-start>=maxPayload)
        {
          frag->set((u_int8_t*)bufferData+start,maxPayload);
        }
        else
        {
          frag->set((u_int8_t*)bufferData+start,maxPayload-start);
        }
        start+=maxPayload;
      }
    }
    SAFplusHeapFree(bufferData);
  }

  void MsgSocketSegmentaion::send(Message* msg)
  {
    applySegmentaion(msg);
    sock->send(msg);
  }
  void MsgSocketSegmentaion::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    Message* m;
    m = sock->msgPool->allocMsg();
    applySegmentaion(m,destination,buffer,length,msgtype);
    sock->send(m);
  }
  Message* MsgSocketSegmentaion::receive(uint_t maxMsgs,int maxDelay)
  {
    return sock->receive(maxMsgs,maxDelay);
  }

  //*****************Advanced socket : MsgSocketShaping********************
  MsgSocketShaping::MsgSocketShaping(uint_t port,MsgTransportPlugin_1* transport,uint_t volume, uint_t leakSize, uint_t leakInterval)
  {
    bucket.leakyBucketCreate(volume,leakSize,leakInterval);
    sock=transport->createSocket(port);
  };
  MsgSocketShaping::MsgSocketShaping(MsgSocket* socket,uint_t volume, uint_t leakSize, uint_t leakInterval)
  {
    bucket.leakyBucketCreate(volume,leakSize,leakInterval);
    sock=socket;
  };
  MsgSocketShaping::~MsgSocketShaping()
  {
     //TODO
  }
  void MsgSocketShaping::applyShaping(uint_t length)
  {
      bucket.leakyBucketFill(length);
  }
  //? Send a bunch of messages.  You give up ownership of msg.
  void MsgSocketShaping::send(Message* msg)
  {
    applyShaping(msg->getLength());
    sock->send(msg);
  }
  void MsgSocketShaping::send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
  {
    assert(sock);
    Message* m = sock->msgPool->allocMsg();
    assert(m);
    m->setAddress(destination);
    MsgFragment* pfx  = m->append(1);
    * ((unsigned char*)pfx->data()) = msgtype;
    pfx->len = 1;
    MsgFragment* frag = m->append(0);
    frag->set(buffer,length);
    applyShaping(m->getLength());
    sock->send(m);
  }
  Message* MsgSocketShaping::receive(uint_t maxMsgs,int maxDelay)
  {
    return sock->receive(maxMsgs,maxDelay);
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


  };
