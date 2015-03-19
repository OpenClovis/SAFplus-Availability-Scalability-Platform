//#define _GNU_SOURCE // needed so that sendmmsg is available

#include <clMsgTransportPlugin.hxx>
#include <sys/socket.h>
//#include <sys/types.h>
//#include <netinet/ip.h>

namespace SAFplus
  {

  class Udp:public MsgTransportPlugin_1
    {
  public:
    in_addr_t netAddr;
    Udp(); 

    virtual MsgTransportConfig& initialize(MsgPool& msgPool);

    virtual MsgSocket* createSocket(uint_t port);
    virtual void deleteSocket(MsgSocket* sock);
    };

  class UdpSocket:public MsgSocket
    {
    public:
    UdpSocket(){}
    UdpSocket(uint_t port,MsgPool* pool,MsgTransportPlugin_1* transport);
    virtual ~UdpSocket();
    virtual void send(Message* msg);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    virtual void flush();
    protected:
    int sock;    
    };
  static Udp api;
};
