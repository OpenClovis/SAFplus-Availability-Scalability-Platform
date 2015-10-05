#pragma once

#include <clMsgApi.hxx>
#include <clLogApi.hxx>
#include <clMsgBase.hxx>
#include <Timer.hxx>
#include <boost/intrusive/list.hpp>
#include <boost/unordered_map.hpp>


using namespace boost::intrusive;
#define MORE_FLAG   0x80
#define LAST_FLAG   0x40
#define USER_HEADER_LEN 6
#define MAX_SEGMENT_SIZE 64000
#define MAX_MSG_SIZE 10000000;
#define CLEAR_QUEUE_INTERVAL 1000;
static ClUint32T currFragId = 0;

namespace SAFplus
{
  enum SegmentType
  {
    FRAG_MORE=0,
    FRAG_LAST
  };

  class MsgKey
  {
  public:
    Handle sender;
    int msgId;
    bool operator == (const MsgKey& other) const
                    {
      return ((sender == other.sender)&&(msgId==other.msgId));
                    }

    bool operator != (const MsgKey& other) const
                    {
      return ((sender != other.sender)||(msgId!=other.msgId));
                    }
    //? Handles can be used as keys in hash tables
  };
  inline std::size_t hash_value(MsgKey const& h)
  {

    boost::hash<uint64_t> hasher;
    std::size_t seed = 0;
    boost::hash_combine(seed,h.msgId);
    boost::hash_combine(seed,h.sender.getNode());
    boost::hash_combine(seed,h.sender.getPort());
    return seed;
  }

  class Segment
  {
  private:
    int m_nFalgs;           /* Control flags field */
    int m_nLen;          /* Header length field */
    int m_nSeqn;         /* Sequence number field */
    int m_nMsgId;
    //    int m_nMsgType;
    int m_nDataLen;
    bool isLast;

  public:
    Byte* m_pData;
    Segment();
    Segment(int seqn, const Byte* buffer, int off, int len , bool isLastFrag);
    void init(int _seqn, int len, bool isLastFrag);
    boost::intrusive::list_member_hook<> m_memberHook;
    int flags();
    int seq();
    int dataLength();
    void setMsgId(int msgId)
    {
      m_nMsgId=msgId;
    }
    int getMsgId()
    {
      return m_nMsgId;
    }
    //    void setMsgType(int msgType)
    //    {
    //      m_nMsgType=msgType;
    //    }
    //    int getMsgType()
    //    {
    //      return m_nMsgType;
    //    }
    void setLast(bool isLastSeg);
    bool isLastSegment();
    Byte* getBytes();
    void parseBytes(const Byte* buffer, int _off, int _len);
    Byte* getData();
    void parse(Byte* bytes, int off, int len);
    friend bool operator< (const Segment &a, const Segment &b)
    {  return a.m_nSeqn < b.m_nSeqn;  }
    friend bool operator> (const Segment &a, const Segment &b)
    {  return a.m_nSeqn > b.m_nSeqn;  }
    friend bool operator== (const Segment &a, const Segment &b)
                               {  return a.m_nSeqn == b.m_nSeqn;  }
    ~Segment()
    {
      delete m_pData;
    }

  };

  typedef member_hook<Segment, list_member_hook<>,&Segment::m_memberHook> MemberHookOption;
  //This list will use the member hook
  typedef list<Segment, MemberHookOption> SegmentationList;

  class MsgSegments
  {
  public:
    SegmentationList    segmentList;
    int expectedSegments;
    int segmentNum;
    bool isFull;
    int length;
    Handle handle;
    MsgSegments()
    {
      expectedSegments=-1;
      segmentNum=0;
      isFull=false;
    };
  };
  typedef boost::unordered_map<SAFplus::MsgKey, SAFplus::MsgSegments*> KeyMsgMap;
  class MsgSocketSegmentaion : public MsgSocketAdvanced
  {
  private:
    KeyMsgMap receiveMap;
    int msgReceived;
    int msgReceiving;
    boost::thread rcvThread; //thread to receive and handle fragment
  public:
//    Timer receiveTimeOut;
//    TimerTimeOutT testTimeout;
    Handle handle;
    MsgSocketSegmentaion(uint_t port,MsgTransportPlugin_1* transport);
    MsgSocketSegmentaion(MsgSocket* socket);
    virtual ~MsgSocketSegmentaion();
    //? Send a bunch of messages.  You give up ownership of msg.
    virtual void send(Message* origMsg);
    virtual void send(SAFplus::Handle destination, void* buffer, uint_t length);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    Segment* receiveSegment(Handle &handle);
    int read(Byte* buffer,int maxlength);
    void handleReceiveThread(void);
    void applySegmentaion(SAFplus::Handle destination, void* buffer, uint_t length);
    void applySegmentaion(Message* m);
    void sendSegment(SAFplus::Handle destination,Segment * frag);
    int getMapsize()
    {
      return receiveMap.size();
    }
    virtual void flush();
    virtual MsgPool* getMsgPool();
  };
};
