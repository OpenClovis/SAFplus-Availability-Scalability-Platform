#include <clPluginApi.hxx>

namespace SAFplus
  {
  class MsgTransportPlugin_1;

  enum
    {
    CL_MSG_TRANSPORT_PLUGIN_ID = 0x53846843,
    CL_MSG_TRANSPORT_PLUGIN_VER = 1
    };

  class MsgFragment
    {
    public:
      enum Flags
        {
          PointerFragment = 1,  //? Fragment contains a pointer to a buffer
          InlineFragment  = 2,   //? Fragment's buffer starts at &buffer.
          DoNotFree       = 0x10,
          SAFplusFree     = 0x20,
          MsgPoolFree     = 0x40,
          CustomFree      = 0x80,
          DataDoNotFree       = 0x100,
          DataSAFplusFree     = 0x200,
          DataMsgPoolFree     = 0x400,
          DataCustomFree      = 0x800,
        };

      Flags flags;
      uint_t allocatedLen;  //? How much data is allocated in this fragment
      uint_t start;         //? Where does the message actually start in the allocated data (offset)
      uint_t len;           //? length of message
      MsgFragment* nextFragment;
      void*        buffer;
 
      void set(const char* buf);  //? Set this fragment to a null-terminated string buffer (you keep ownership)
      void set(void* buf, uint_t length);  //? Set this fragment to buffer (you keep ownership)
      void* data(int offset=0);
      const void* read(int offset=0);

      void constructPointerFrag()
        {
        flags = (SAFplus::MsgFragment::Flags) (PointerFragment | MsgPoolFree | DataDoNotFree);  // By default the fragment data is constructed to not be freed, but user can change this if his buffer needs to be freed
        allocatedLen = 0;
        start = 0;
        len = 0;
        nextFragment = nullptr;
        buffer = nullptr;
        }

      void constructInlineFrag(uint size)
        {
        flags = (SAFplus::MsgFragment::Flags) (InlineFragment | MsgPoolFree | DataDoNotFree); // Data is part of this object so no need to independently free it
        allocatedLen = size;
        start = 0;
        len = 0;
        nextFragment = nullptr;
        }

    };

  class MsgPool;

  //? Defines a linked list of iovector message buffers
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
    MsgPool*     msgPool;
    MsgFragment* firstFragment;
    MsgFragment* lastFragment;
    Message* nextMsg;
    };

  //? A pool of message buffers so we don't have to keep freeing/allocating.
  class MsgPool
    {
      public:
      // These are virtual so that a plugin does not have to link with libmsg to call them.
      virtual MsgFragment* allocMsgFragment(uint_t size);
      virtual Message* allocMsg();
      virtual void free(Message*);
    };

  //? How the messaging transport reports to us important information about its capabilities
  class MsgTransportConfig
    {
      public:
      uint_t maxMsgSize;  //? Maximum size of messages in bytes
      uint_t maxMsgAtOnce; //? Maximum number of messages that can be sent in a single call
      uint_t maxPort;     //? Maximum message port
      uint_t nodeId;      //? identifies this node uniquely in the cluster
    };

  //? A portal to send and receive messages
  class MsgSocket
    {
      public:
      virtual ~MsgSocket()=0;
      MsgPool* msgPool;
      MsgTransportPlugin_1* transport;
      uint_t node; //? source or destination node, depending on whether this message is being sent or was received.
      uint_t port; //? source or destination port, depending on whether this message is being sent or was received.

      //? Send a bunch of messages.  You give up ownership of msg.
      virtual void send(Message* msg)=0;
      //? Force all queued messages to be sent (you can reuse any non msgpool buffers you gave me)
      virtual void flush()=0;

      //? Receive up to maxMsgs messages.  Wait for no more than maxDelay milliseconds.  If no messages have been received within that time return NULL.  If maxDelay is -1 (default) then wait forever.  If maxDelay is 0 do not wait.
      virtual Message* receive(uint_t maxMsgs,int maxDelay=-1)=0;

    };


  class MsgNotificationData:public WakeableCookie
    {
      public:
      enum  // cookie subtypes
      {
        NodeJoin  = 1,
        NodeLeave = 2,  // Note a single NodeLeave squelches all PortLeave notifications on that node.
        PortJoin  = 3,
        PortLeave = 4,
      };
    
    };


  class MsgTransportPlugin_1:public ClPlugin
    {
  public:
    const char* type;  //? transport plugin type (i.e. UDP, TIPC)
    MsgTransportConfig config;
    MsgPool* msgPool;  //? This is the memory pool to use for this transport.
    Wakeable** watchers;
    uint_t numWatchers;

    //? Do any transport related initialization, including setting the "config" and "msgPool" variables to the appropriate values
    virtual MsgTransportConfig& initialize(MsgPool& msgPool)=0;

    //? Create a communications channel
    virtual MsgSocket* createSocket(uint_t port)=0;
    //? Delete and close the communication channel
    virtual void deleteSocket(MsgSocket* sock)=0;

    //? Register for any events coming from this plugin
    virtual void registerWatcher(Wakeable* notification);
    //? No longer receive events coming from this plugin
    virtual void unregisterWatcher(Wakeable* notification);

    // The copy constructor is disabled to ensure that the only copy of this
    // class exists in the shared memory lib.
    // This will help allow policies to be unloaded and updated by discouraging
    // them from being copied willy-nilly.
    MsgTransportPlugin_1(MsgTransportPlugin_1 const&) = delete; 
    MsgTransportPlugin_1& operator=( MsgTransportPlugin_1 const&) = delete;
  protected:  // Only constructable from your derived class from within the .so
    MsgTransportPlugin_1():type(nullptr),msgPool(nullptr),watchers(nullptr),numWatchers(0) {};
    };

  class ScopedMsgSocket
    {
    public:
    MsgSocket* sock;
    ScopedMsgSocket(MsgTransportPlugin_1* xp, uint_t port) { sock=xp->createSocket(port); }
    ~ScopedMsgSocket() { sock->transport->deleteSocket(sock); }

    MsgSocket* operator->() {return sock;}
    };

  }

namespace SAFplusI
{
  extern SAFplus::MsgTransportPlugin_1* defaultMsgPlugin;
};
