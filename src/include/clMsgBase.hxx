//? <section name="Messaging">
#pragma once
#include <clDbg.hxx>
#include <clMsgHandler.hxx>
#include <leakyBucket.hxx>
#include <clMsgTransportPlugin.hxx>

namespace SAFplus
  {
    //? <class> A message fragment is a buffer that contains part or all of a message.
  class MsgFragment
    {
    public:
      enum Flags
        {
          //? <_> Fragment contains a pointer to a buffer </_>
          PointerFragment = 1,           
          InlineFragment  = 2,           //? Fragment's buffer starts at &buffer and extends beyond the end of this object
          DoNotFree       = 0x10,        //? Do not free this MsgFragment, the application will reuse it
          SAFplusFree     = 0x20,        //? This fragment was allocated outside of the message pool. Free it using the @SAFplusHeapFree API
          MsgPoolFree     = 0x40,        //? This fragment was allocated in the message pool. Free it back into the message pool
          CustomFree      = 0x80,        //? Use an application supplied free function (TODO)
          DataDoNotFree       = 0x100,   //? Do not free the data referenced by this MsgFragment, either the application will reuse it, or this is an @InlineFragment
          DataSAFplusFree     = 0x200,   //? This fragment's data was allocated outside of the message pool. Free it using the @SAFplusHeapFree API
          DataMsgPoolFree     = 0x400,   //? This fragment's data was allocated in the message pool. Free it back into the message pool
          DataCustomFree      = 0x800,   //? Use an application supplied free function (TODO)
        };

      Flags flags;          //? Describes metadata about this fragment, bit meanings are defined as constants
      uint_t allocatedLen;  //? How much data is allocated in this fragment.  This will be 0 if the fragment was not allocated (it could be a const char* buffer, or the application does not want the messaging layer to free this memory).
      uint_t start;         //? Where does the message actually start in the allocated data (offset).
      uint_t len;           //? length of message.  So start+len must be < allocatedLen, if this is an allocated fragment.

      MsgFragment* nextFragment;  //? A pointer to the next fragment in the message.  Should only be used by MsgSocket implementations.
    protected:
      void*        buffer;  //? This must be the LAST object in the class so that the InlineFragment logic works correctly.  It is either a pointer to the memory buffer or is ITSELF the first bytes of the inline memory buffer.  Due to application bugs caused by this optimization, use the @data and @read functions to access the buffer instead of using this directly.
    public:
      void set(const char* buf);  //? Set this fragment to a null-terminated string buffer (you keep ownership)
      void set(void* buf, uint_t length);  //? <_> Set this fragment to the provided buffer (you keep ownership) <arg name='buf'>The data in this buffer will be copied into the fragment</arg><arg name='length'>length of the passed buffer</arg> </_>
      void* data(int offset=0);  //? Get a pointer to the data in this fragment at the provided offset.
      const void* read(int offset=0);  //? Get a read-only pointer to the data in this fragment at the provided offset

      // Internal interface: initializes this fragment as one that points to an external buffer
      void constructPointerFrag()
        {
        flags = (SAFplus::MsgFragment::Flags) (PointerFragment | MsgPoolFree | DataDoNotFree);  // By default the fragment data is constructed to not be freed, but user can change this if his buffer needs to be freed
        allocatedLen = 0;
        start = 0;
        len = 0;
        nextFragment = nullptr;
        buffer = nullptr;
        }

      // Internal interface: initializes this fragment as one whose buffer is located at &buffer
      void constructInlineFrag(uint size)
        {
        flags = (SAFplus::MsgFragment::Flags) (InlineFragment | MsgPoolFree | DataDoNotFree); // Data is part of this object so no need to independently free it
        allocatedLen = size;
        start = 0;
        len = 0;
        nextFragment = nullptr;
        }

      friend class MsgPool;
  };  //? </class>

  class MsgPool;

  //? <class> Defines a linked list of iovector message buffers
  class Message
    {
    public:
    Message() { initialize(); }
    void initialize() { node=0; port=0; nextMsg=0; firstFragment=nullptr; lastFragment=nullptr; nextMsg=nullptr; msgPool=nullptr;  }
    uint_t node; //? source or destination node, depending on whether this message is being sent or was received.
    uint_t port; //? source or destination port, depending on whether this message is being sent or was received.

    //? Change the address of this message.
    void setAddress(uint_t nodep, uint_t portp) { node=nodep; port=portp; }
    //? Change the address of this message to that of the node and port of the provided handle.
    void setAddress(const Handle& h);

    MsgFragment* prepend(uint_t size); //? Create a message fragment at the beginning of this message and return it
    MsgFragment* append(uint_t size);  //? Create a message fragment at the end of this message and return it

    MsgPool*     msgPool;  //? The message pool that this message and fragments were allocated from
    Message* nextMsg; //? The next message in this send or received groups. 

      // protected:  Should only be used by message transport implementations
    MsgFragment* firstFragment;  //? The first fragment in this message.  Typically only used by the lower layers; upper layers should "hang on" to the MsgFragments returned by the prepend and append functions.
    MsgFragment* lastFragment;  //?  The last fragment in this message.  Typically only used by the lower layers; upper layers should "hang on" to the MsgFragments returned by the prepend and append functions.
    };   //? </class>

  //? <class> A pool of message buffers so we don't have to keep freeing/allocating.
  class MsgPool
    {
      public:
      MsgPool():allocated(0),fragAllocated(0),fragAllocatedBytes(0),fragCumulativeBytes(0) {}
      //? Number of messages currently allocated
      int64_t allocated;
      //? Number of fragments currently allocated
      int64_t fragAllocated;
      //? Number of bytes currently allocated
      int64_t fragAllocatedBytes;
      //? Total number of bytes in every message allocated.  That is, 2 sequential allocations of size N that are mapped to the same physical memory increase this value by 2N
      int64_t fragCumulativeBytes;
      // These are virtual so that a plugin does not have to link with libmsg to call them.
      //? Allocate a message fragment with an inline buffer of @size.  If @size is zero, this allocates a pointer fragment.
      virtual MsgFragment* allocMsgFragment(uint_t size);
      //? Allocate a new message
      virtual Message* allocMsg();
      //? Free an entire chain of messages and message fragments.
      virtual void free(Message* msg);
    }; //? </class>

  //? <class> A interface defining a portal to send and receive messages.  The application can create or delete a MsgSocket by calling the MsgTransportPlugin_1::createSocket() and MsgTransportPlugin_1::deleteSocket() functions, or by using the @ScopedMsgSocket convenience class for stack-scoped sockets.  Classes derived from MsgSocket can also be layered on top of transport MsgSockets to create additional functionality such as traffic shaping, large message capability, etc.  For more information see [[Messaging#Message_Transport_Layer]]
  class MsgSocket
    {
      public:
      friend class MsgFragment;
      virtual ~MsgSocket()=0;
      MsgPool* msgPool;  //? The message pool that will be used when allocating messages and fragments during receive() and freeing messages and fragments during send()
      uint_t node; //? node address of this socket -- that is, the address of this node
      uint_t port; //? port this socket is listening to.  This may not directly correspond to underlying transport port numbers. Instead, SAFplus ports will be within a specific range in the underlying transport, so to get the underlying port number, you'd use "port + <start port constant defined in clCustomization.hxx>"
      
      //? Return a handle that refers to this message socket.
      SAFplus::Handle handle() { return SAFplus::getProcessHandle(port,node); }
      //? Send a bunch of messages.  You give up ownership of msg.  The msg object will be released by calling msgPool->free().  This may or may not actually call free depending on settings in the MessageFragment and implementation of the MessagePool.
      virtual void send(Message* msg)=0;
      //? Force all queued messages to be sent.  After this function returns, you can modify any buffers you gave MsgSocket to via the send call, provided that they were not allocated by the msgPool.  In that case, they have been freed.
      virtual void flush()=0;

      //? Receive up to maxMsgs messages.  Wait for no more than maxDelay milliseconds.  If no messages have been received within that time return NULL.  If maxDelay is -1 (default) then wait forever.  If maxDelay is 0 do not wait.
      virtual Message* receive(uint_t maxMsgs,int maxDelay=-1)=0;

      //? Enable Nagle's algorithm (delay and batch sending small messages), if the underlying transport supports it.  You should check the transport's capabilities before calling this function.  If the transport does not support NAGLE's algorithm, this function will be a no-op but issue a log.  See <a href="http://en.wikipedia.org/wiki/Nagle%27s_algorithm">Nagle's Algorithm</a> for more details.
      virtual void useNagle(bool value);

    protected:
      MsgTransportPlugin* transport;
      friend class ScopedMsgSocket;
  }; //? </class>

  class MsgSocketAdvance
  {
    public:
    virtual ~MsgSocketAdvance()
    {
      //sock->transport->deleteSocket(sock);
    }
    MsgSocket *sock;
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg,uint_t length)=0;
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1)=0;
    MsgSocket* operator->() {return sock;}
  };

  class MsgSocketShaping : public MsgSocketAdvance
  {
    private:
      SAFplus::leakyBucket bucket;
    public:
    MsgSocketShaping(uint_t port,MsgTransportPlugin_1* transport,uint_t volume, uint_t leakSize, uint_t leakInterval);
    virtual ~MsgSocketShaping();
    //? Tell the traffic shaper that a message of the supplied length was sent.  This function is automatically called by send so the application typically never needs to use it.
    void applyShaping(uint_t length);
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg,uint_t length);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
  };

  class MsgSocketReliable : public MsgSocketAdvance
  {
    private:

    public:
    MsgSocketReliable(uint_t port,MsgTransportPlugin_1* transport);
    virtual ~MsgSocketReliable();
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg,uint_t length);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
  };

  class MsgSocketSegmentaion : public MsgSocketAdvance
  {
    private:
    public:
    MsgSocketSegmentaion(uint_t port,MsgTransportPlugin_1* transport);
    virtual ~MsgSocketSegmentaion();
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg,uint_t length);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
  };

  //? <class> A message socket whose lifetime rules follow lexical scoping
  class ScopedMsgSocket
    {
    public:
      //? <ctor> Pass the message transport plugin and port to create your message socket </ctor>
    ScopedMsgSocket(MsgTransportPlugin* xp, uint_t port) { sock=xp->createSocket(port); }
    ~ScopedMsgSocket() { sock->transport->deleteSocket(sock); }
      //? Access the MsgSocket object directly
    MsgSocket* sock;
      //? A convenience function that makes the ScopedMsgSocket object behave like a MsgSocket* object.
    MsgSocket* operator->() {return sock;}
  }; //? </class>


};


//? </section>
