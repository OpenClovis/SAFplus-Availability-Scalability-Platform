//? <section name="Messaging">
#pragma once
#include <clDbg.hxx>
#include <leakyBucket.hxx>
#include <clMsgTransportPlugin.hxx>

#include <algorithm>
#include <iosfwd>                          // streamsize
#include <boost/iostreams/categories.hpp>  // sink_tag
#include <boost/iostreams/concepts.hpp>  // sink
typedef unsigned char  Byte;  /* 8 bits */
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

namespace SAFplus
  {

  class MsgSocketCapabilities
  {
  public:
    enum Capabilities  //? This must be the same numbers as MsgTransportConfig::Capabilities
        {
          NONE              = 0,
          RELIABLE          = 1,    //? Reliable messages are supported at this level
          NAGLE_AVAILABLE   = 0x20, //? Layer can delay transmission to attempt to combine short messages
          BROADCAST         = 0x40, //? Layer can broadcast or simulate broadcasts.  Below the typical application layer, this capability may start as false.  Then the messaging initialization my add cluster nodes object.  Since the transport layer can use this object to send a message to every node (simulated broadcast), subsequent calls will return true for this capability.
        };

      Capabilities capabilities; //? What features does this message transport support?
      uint_t maxMsgSize;  //? Maximum size of messages in bytes
      uint_t maxMsgAtOnce; //? Maximum number of messages that can be sent in a single call
  };

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
      uint_t len;           //? length of message from start to the end.  So start+len must be < allocatedLen, if this is an allocated fragment.

      MsgFragment* nextFragment;  //? A pointer to the next fragment in the message.  Should only be used by MsgSocket implementations.
    protected:
      void*        buffer;  //? This must be the LAST object in the class so that the InlineFragment logic works correctly.  It is either a pointer to the memory buffer or is ITSELF the first bytes of the inline memory buffer.  Due to application bugs caused by this optimization, use the @data and @read functions to access the buffer instead of using this directly.
    public:
      void set(const char* buf);  //? Set this fragment's buffer to this null-terminated string (you keep ownership and must free at the appropriate time).  String is NOT copied.
      void set(void* buf, uint_t length);  //? <_> Use the provided buffer as this fragment's data.  You must still clean up the buffer at the appropriate time (after flush() or other technique to ensure the message is actually sent. <arg name='buf'>The fragment will point to this buffer</arg><arg name='length'>length of the buffer</arg> </_>
      int append(const char* s, int n); //? Copy these bytes to the end of the buffer allocated for this fragment.  You keep ownership of s and can free at any time after this function returns.
      void* data(int offset=0);  //? Get a pointer to the data in this fragment at the provided offset.
      const void* read(int offset=0);  //? Get a read-only pointer to the data in this fragment at the provided offset

      void used(uint_t length) //? Indicate that you have used this many bytes beyond the current end of the fragment (typically by setting them directly via the data() API)
      {
        len+=length;
      }

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

  class Message;

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
      void freeFragment(MsgFragment* frag);
    }; //? </class>

  //? <class> Defines a linked list of iovector message buffers
  class Message
    {
    public:
    Message() { initialize(); }
    void initialize() { node=0; port=0; nextMsg=0; firstFragment=nullptr; lastFragment=nullptr; nextMsg=nullptr; msgPool=nullptr;  }
    uint_t node; //? source or destination node, depending on whether this message is being sent or was received.
    uint_t port; //? source or destination port, depending on whether this message is being sent or was received.
    //uint_t lower; //? TIPC lower instance.
    //uint_t upper; //? TIPC upper instance.

      //? Get the source or destination handle (depending on whether this message is being sent or was received) of this message.  This is just a convenience function that constructs a handle from the node and port fields of this object
    Handle getAddress() { return getProcessHandle(port,node); }
    //? Change the address of this message.
    void setAddress(uint_t nodep, uint_t portp) { node=nodep; port=portp; }
    //? Change the address of this message to that of the node and port of the provided handle.
    void setAddress(const Handle& h);
    void deleteLastFragment();
    uint getLength();

      //? Copy the data in the message to the supplied buffer.  Return the number of bytes copied.
    uint_t copy(void* data, uint_t offset, uint_t maxlength);

      //? Return a pointer to a buffer which contains the requested data, contiguous and aligned.  This pointer MAY point to data inside the message or MAY point to data inside the supplied buffer.  The supplied buffer must contain at least length+align bytes (or length bytes and be properly aligned).  If align==0 or 1 data is byte aligned (unaligned).  align==2, 4, 8 means alignment for short, int32 and int64 respectively.  Returns NULL if offset+length is longer than the message
    void* flatten(void* data, uint_t offset, uint_t length,uint_t align=0);

    MsgFragment* prepend(uint_t size); //? Create a message fragment at the beginning of this message and return it
    MsgFragment* append(uint_t size);  //? Create a message fragment at the end of this message and return it

    void prependFrag(MsgFragment* frag); //? Put this fragment in the beginning of the message

    void free(void) { msgPool->free(this); }  //? Return this message, all fragments, and child messages to the message pool

    MsgPool*     msgPool;  //? The message pool that this message and fragments were allocated from
    Message* nextMsg; //? The next message in this send or received groups. 

      // protected:  Should only be used by message transport implementations
    MsgFragment* firstFragment;  //? The first fragment in this message.  Typically only used by the lower layers; upper layers should "hang on" to the MsgFragments returned by the prepend and append functions.
    MsgFragment* lastFragment;  //?  The last fragment in this message.  Typically only used by the lower layers; upper layers should "hang on" to the MsgFragments returned by the prepend and append functions.
    };   //? </class>


  //? <class> A interface defining a portal to send and receive messages.  The application can create or delete a MsgSocket by calling the MsgTransportPlugin_1::createSocket() and MsgTransportPlugin_1::deleteSocket() functions, or by using the @ScopedMsgSocket convenience class for stack-scoped sockets.  Classes derived from MsgSocket can also be layered on top of transport MsgSockets to create additional functionality such as traffic shaping, large message capability, etc.  For more information see [[Messaging#Message_Transport_Layer]]
  class MsgSocket
    {
      public:
      friend class MsgFragment;
      virtual ~MsgSocket()=0;
      MsgSocketCapabilities cap; //? Report the capabilities of this message socket
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
      virtual MsgPool* getMsgPool()
      {
        logTrace("MSG", "MSS","Get message pool");
        return msgPool;
      };

      MsgTransportPlugin* transport;
    protected:
      friend class ScopedMsgSocket;
  }; //? </class>

  class MsgSocketAdvanced:public MsgSocket
  {
    public:
    virtual ~MsgSocketAdvanced()
    {
      //sock->transport->deleteSocket(sock);
    }
    MsgSocket *sock;

    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg)
    {
    }
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1)
    {
      return nullptr;
    }
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype)
    {
    }

    virtual void flush();
  };

  class MsgSocketShaping : public MsgSocketAdvanced
  {
    protected:
      SAFplus::LeakyBucket bucket;
    public:
    MsgSocketShaping(uint_t port,MsgTransportPlugin_1* transport,uint_t volume, uint_t leakSize, uint_t leakInterval);
    MsgSocketShaping(MsgSocket* socket,uint_t volume, uint_t leakSize, uint_t leakInterval);
    virtual ~MsgSocketShaping();  // If you do not want the underlying socket to be returned when this object is destructed, then set sock to NULL before deleting this object
    //? Tell the traffic shaper that a message of the supplied length was sent.  This function is automatically called by send so the application typically never needs to use it.
    void applyShaping(uint_t length);
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* msg);
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length,uint_t msgtype);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    virtual void flush();
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

    //? <class> This class allows you to write data to a Msg using the c++ ostream interface.  Typically no APIs in this class are accessed directly.  Please see the Msg stream example code for a tutorial on use.  It is essential that this object be destroyed before the backing Message object is sent because iostreams use buffering.  So the stream may not actually write to the Message object until it is destroyed.
class MessageOStream:public boost::iostreams::sink
  {
  public:
    Message* msg;
    int fragSize;
    bool eswap;
    std::streamsize write(const char* s, std::streamsize n);

    MessageOStream(Message* _msg, unsigned int _sizeHint=4096, bool endianSwapNeeded=false)
    {
      eswap = endianSwapNeeded;
      msg = _msg;
      fragSize = _sizeHint;
    }

    //? Binary, endian swap serialization operators
    MessageOStream& operator << (uint64_t val) { if (eswap) val = __builtin_bswap64(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (int64_t val) { if (eswap) val = __builtin_bswap64(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (uint32_t val) { if (eswap) val = __builtin_bswap32(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (int32_t val) { if (eswap) val = __builtin_bswap32(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (uint16_t val) { if (eswap) val = __builtin_bswap16(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (int16_t val) { if (eswap) val = __builtin_bswap16(val); write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (uint8_t val) { write((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageOStream& operator << (int8_t val) { write((char*) &val, sizeof(val)); return *this; }
}; //? </class>

    //? <class> This class allows you to write data to a Msg using the c++ ostream interface.  Typically no APIs in this class are accessed directly.  Please see the Msg stream example code for a tutorial on use.  It is essential that this object be destroyed before the backing Message object is sent because iostreams use buffering.  So the stream may not actually write to the Message object until it is destroyed.
class MessageIStream:public boost::iostreams::source
  {
  public:
    Message* msg;
    MsgFragment* curFrag;
    int curOffset;
    bool eswap;
    std::streamsize read(char* s, std::streamsize n);

    MessageIStream(Message* _msg,bool endianSwapNeeded=false)
    {
      eswap = endianSwapNeeded;
      msg = _msg;
      curFrag = msg->firstFragment;
      curOffset=0;
    }

    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (uint64_t& val) { if (eswap) val = __builtin_bswap64(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (int64_t& val) { if (eswap) val = __builtin_bswap64(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (uint32_t& val) { if (eswap) val = __builtin_bswap32(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (int32_t& val) { if (eswap) val = __builtin_bswap32(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (uint16_t& val) { if (eswap) val = __builtin_bswap16(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (int16_t& val) { if (eswap) val = __builtin_bswap16(val); read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (uint8_t& val) { read((char*) &val, sizeof(val)); return *this; }
    //? Binary, endian swap serialization operators
    MessageIStream& operator >> (int8_t& val) { read((char*) &val, sizeof(val)); return *this; }

}; //? </class>


};


//? </section>
