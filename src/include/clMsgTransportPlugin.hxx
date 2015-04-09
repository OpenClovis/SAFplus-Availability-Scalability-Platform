#pragma once
#include <clPluginApi.hxx>

namespace SAFplus
  {
  class MsgTransportPlugin_1;
    class MsgSocket;
    class MsgPool;

  enum
    {
    CL_MSG_TRANSPORT_PLUGIN_ID = 0x53846843,
    CL_MSG_TRANSPORT_PLUGIN_VER = 1
    };


  //? How the messaging transport reports to us important information about its capabilities
  class MsgTransportConfig
    {
      public:

      enum Capabilities
        {
          NONE              = 0,
          RELIABLE          = 1,
          NOTIFY_NODE_JOIN  = 2,
          NOTIFY_NODE_LEAVE = 4,
          NOTIFY_PORT_JOIN  = 8,
          NOTIFY_PORT_LEAVE = 0x10,
          NAGLE_AVAILABLE   = 0x20,
        };

      Capabilities capabilities; //? What features does this message transport support?
      uint_t maxMsgSize;  //? Maximum size of messages in bytes
      uint_t maxMsgAtOnce; //? Maximum number of messages that can be sent in a single call
      uint_t maxPort;     //? Maximum message port
      uint_t nodeId;      //? identifies this node uniquely in the cluster
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

    //?  MsgTransportPlugin will always refer to the "latest" revision of the plugin
    typedef MsgTransportPlugin_1 MsgTransportPlugin;

  }

namespace SAFplusI
{
  extern SAFplus::MsgTransportPlugin_1* defaultMsgPlugin;
};
