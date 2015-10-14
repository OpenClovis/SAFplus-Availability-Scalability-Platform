#include <boost/unordered_map.hpp>

#include <clMsgBase.hxx>

namespace SAFplus
  {

    /*? <class> Segmentation and Reassembly socket
    

    Messages are broken into packets.  All are prefixed with 2 fields: msgNum (8 bits) and index (16 bits) written in network order.
    The msgNum field has a final message indicator (msgNum&1)==1, and 7 bits specifying which message this is.  From the receiver's perspective, source address:msgNum uniquely identifies a "live" message.  Index starts at zero and counts up.  Dropped messages can be identified due to gaps in the index, the initial index not starting a zero (lost first message), or the final message bit not being set (lost last message). 
    */ 
    class MsgSarIdentifier
    {
    public:
      SAFplus::Handle from;
      uint_t msgNum; 
    };


    class MsgSarSocket: public MsgSocket
    {
      public:
      friend class MsgFragment;
      virtual ~MsgSarSocket();

      typedef boost::unordered_map<MsgSarIdentifier,Message*> MsgSarMap;

      MsgSarMap received;

      MsgSocket* xport;
      MsgPool* headerPool;

      //? Construct the socket.  Provide the underlying transport (or additional layer) that this segmentation and reassembly layer will use 
      MsgSarSocket(MsgSocket* underlyingSocket);
      
      //? Send a bunch of messages.  You give up ownership of msg.  The msg object will be released by calling msgPool->free().  This may or may not actually call free depending on settings in the MessageFragment and implementation of the MessagePool.
      virtual void send(Message* msg);
      //? Force all queued messages to be sent.  After this function returns, you can modify any buffers you gave MsgSocket to via the send call, provided that they were not allocated by the msgPool.  In that case, they have been freed.
      virtual void flush();
      //? Receive up to maxMsgs messages.  Wait for no more than maxDelay milliseconds.  If no messages have been received within that time return NULL.  If maxDelay is -1 (default) then wait forever.  If maxDelay is 0 do not wait.
      virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
      //? Enable Nagle's algorithm (delay and batch sending small messages), if the underlying transport supports it.  You should check the transport's capabilities before calling this function.  If the transport does not support NAGLE's algorithm, this function will be a no-op but issue a log.  See <a href="http://en.wikipedia.org/wiki/Nagle%27s_algorithm">Nagle's Algorithm</a> for more details.
      virtual void useNagle(bool value);

    protected:
      int msgNum;
      int setHeader(void* ptr, uint msgNum, uint offset);  // fills the ptr with a valid message header.  Returns the length of the header.
      bool consumeHeader(Message* m, uint* msgNum, uint* offset);  // extracts the header from m.  Returns data in msgNum and offset.  Return value is true if this is the last packet in the message.

      MsgTransportPlugin* transport;
      friend class ScopedMsgSocket;
    }; //? </class>


  };
