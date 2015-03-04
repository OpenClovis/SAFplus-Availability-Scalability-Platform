#include <clMsgTransportPlugin.hxx>

namespace SAFplus
  {

  class Udp:public MsgTransportPlugin_1
    {
  public:
    MsgPool*  pool;
    Wakeable* wake;

    Udp(); 

    virtual MsgTransportConfig initialize(MsgPool& msgPool, Wakeable* notification);

    virtual MsgSocket* createSocket(uint_t port);
    };

  class UdpSocket:public MsgSocket
    {
    public:
    UdpSocket(uint_t port,MsgPool* pool);
    virtual ~UdpSocket();
    virtual void send(Message* msg);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    };


  Udp::Udp()
    {
    pool = NULL; wake = NULL;
    }

  MsgTransportConfig Udp::initialize(MsgPool& msgPool, Wakeable* notification)
    {
    MsgTransportConfig ret;

    pool = &msgPool;
    wake = notification;

    ret.maxMsgSize = SAFplusI::UdpTransportMaxMsgSize;
    ret.maxPort    = SAFplusI::UdpTransportNumPorts;

    return ret;
    }

  MsgSocket* Udp::createSocket(uint_t port)
    {
    if (port >= SAFplusI::UdpTransportNumPorts) return NULL;
    return new UdpSocket(port,pool);
    }

   UdpSocket::UdpSocket(uint_t pt,MsgPool*  pool)
     {
     port = pt;
     msgPool = pool;
     // TODO: node = ???
      
     }

    UdpSocket::~UdpSocket()
      {
      
      }

    void UdpSocket::send(Message* msg)
      {
      
      // Your send routine releases the message whenever you are ready to do so
      MsgPool* temp = msg->pool;
      temp->free(msg);
      }

    Message* UdpSocket::receive(uint_t maxMsgs,int maxDelay)
      {
      return NULL;
      }

  static Udp api;

  };


extern "C" SAFplus::ClPlugin* clPluginInitialize(ClWordT preferredPluginVersion)
  {
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID;
  SAFplus::api.pluginVersion    = SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER;
  SAFplus::api.type = "UDP";

  // return it
  return (SAFplus::ClPlugin*) &SAFplus::api;
  }
