//#define _GNU_SOURCE // needed so that sendmmsg is available

#include <clMsgApi.hxx>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/sctp.h>
#include <clCommon.hxx>  // For exceptions
#include <clIocConfig.h>
#include "clPluginHelper.hxx"
#include <boost/unordered_map.hpp>

typedef boost::unordered_map<uint_t, int> NodeIDSocketMap;

namespace SAFplus
{ 
  class Sctp: public MsgTransportPlugin_1
    {
  public:
    in_addr_t netAddr;
    uint32_t nodeMask;
    Sctp(); 

    virtual MsgTransportConfig& initialize(MsgPool& msgPool);

    virtual MsgSocket* createSocket(uint_t port);
    virtual void deleteSocket(MsgSocket* sock);
    };

  class SctpSocket: public MsgSocket
  {
  public:    
    SctpSocket(uint_t port,MsgPool* pool,MsgTransportPlugin_1* transport);    
    virtual void send(Message* msg);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    virtual void flush();
    virtual ~SctpSocket();
  protected:
    int sock; 
    NodeIDSocketMap clientSockMap; //TODO in case node failure: if a node got failure, this must be notified so that
    // the item associated with it in the map must be deleted too, if not, the associated socket may be invalid because of its server failure
    int getClientSocket(uint_t nodeID, uint_t port);
    int openClientSocket(uint_t nodeID, uint_t port);    
    
  };
  static Sctp api;

  Sctp::Sctp()
  {
    msgPool = NULL;
  }

  MsgTransportConfig& Sctp::initialize(MsgPool& msgPoolp)
  {
    msgPool = &msgPoolp;
    
    config.maxMsgSize = SAFplusI::SctpTransportMaxMsgSize;
    config.maxPort    = SAFplusI::SctpTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::SctpTransportMaxMsg;
    
    struct in_addr bip = SAFplusI::setNodeNetworkAddr(&nodeMask);      
    config.nodeId = SAFplus::ASP_NODEADDR;
    netAddr = ntohl(bip.s_addr)&(~nodeMask);

    return config;
  }

  MsgSocket* Sctp::createSocket(uint_t port)
  {
    if (port >= SAFplusI::SctpTransportNumPorts) return NULL;
    return new SctpSocket(port, msgPool,this); // open a listen socket
  }

  void Sctp::deleteSocket(MsgSocket* sock)
  {
    delete sock;
  }

  SctpSocket::SctpSocket(uint_t pt,MsgPool*  pool,MsgTransportPlugin_1* xp)
  {    
    port = pt;
    msgPool = pool;
    transport = xp;
    node = xp->config.nodeId;

    if ((sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) 
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    struct sctp_event_subscribe event;  
    struct sctp_paddrparams heartbeat;    
    struct sctp_rtoinfo rtoinfo;
    memset(&event,      1, sizeof(struct sctp_event_subscribe));
    memset(&heartbeat,  0, sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0, sizeof(struct sctp_rtoinfo));
    heartbeat.spp_flags = SPP_HB_ENABLE; // set heartbeat so that the multi-homing enabled
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;
    rtoinfo.srto_max = 2000;
    int reuse = 1;
    if(setsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat)) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
    }

    if(setsockopt(sock, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo)) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
    }

    /*if(setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(struct sctp_event_subscribe)) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
    }*/

    /* set the reuse of address*/
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))< 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
    }

    struct sockaddr_in myaddr;
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (port == 0) // any port
    {
      myaddr.sin_port = htons(0);
    }
    else 
    {    
      myaddr.sin_port = htons(port + SAFplusI::SctpTransportStartPort);
    }

    if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    } 
    if(listen(sock, CL_IOC_MAX_NODES) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
  }

  int SctpSocket::openClientSocket(uint_t nodeID, uint_t port)
  {
    int sock,ret;    
    //char address[16];    
    struct sockaddr_in addr;
    struct sctp_initmsg initmsg;
    struct sctp_event_subscribe events;
    struct sctp_paddrparams heartbeat;
    struct sctp_rtoinfo rtoinfo;

    memset(&initmsg,    0,   sizeof(struct sctp_initmsg));
    memset(&addr,       0,   sizeof(struct sockaddr_in));
    memset(&events,     1,   sizeof(struct sctp_event_subscribe));
    memset(&heartbeat,  0,   sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0,   sizeof(struct sctp_rtoinfo));

    addr.sin_family = AF_INET;    
    //inet_aton(address, &(addr.sin_addr));
    addr.sin_addr.s_addr = htonl(((Sctp*)transport)->netAddr | nodeID);
    addr.sin_port = htons(port);

    initmsg.sinit_num_ostreams = SAFplusI::SctpMaxStream;
    initmsg.sinit_max_instreams = SAFplusI::SctpMaxStream;
    initmsg.sinit_max_attempts = SAFplusI::SctpMaxStream;

    heartbeat.spp_flags = SPP_HB_ENABLE;
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;

    rtoinfo.srto_max = 2000;

    if((ret = (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP))) < 0)    
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    /*Configure Heartbeats*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat))) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    /*Set rto_max*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo))) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    /*Set SCTP Init Message*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg))) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    int nodelay=1;  
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay))) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    /*Enable SCTP Events*/
    /*if((ret = setsockopt(sock, SOL_SCTP, SCTP_EVENTS, (void *)&events, sizeof(events))) != 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }*/

    /*Get And Print Heartbeat Interval*/
    /*i = (sizeof heartbeat);
    getsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &heartbeat, (socklen_t*)&i);

    printf("Heartbeat interval %d\n", heartbeat.spp_hbinterval);*/

    /*Connect to server*/
    if(((ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)))) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    return sock;
  }

  int SctpSocket::getClientSocket(uint_t nodeID, uint_t port)
  {
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeID);
    if (contents != clientSockMap.end()) // Socket connected to this NodeID has been opened
    {
      return contents->second;
    }
    int newSock = openClientSocket(nodeID, port);
    clientSockMap[nodeID] = newSock;
    return newSock;
  }
  
  void SctpSocket::send(Message* origMsg)
  {
    mmsghdr msgvec[SAFplusI::SctpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
    bzero(msgvec,sizeof(msgvec));
    struct iovec iovecBuffer[SAFplusI::SctpTransportMaxFragments];
    mmsghdr* curvec = &msgvec[0];
    int msgCount = 0;
    int fragCount= 0;  // frags in this message
    int totalFragCount = 0; // total number of fragments
    Message* msg;
    Message* next = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    do 
    { 
      msg = next;
      next = msg->nextMsg;  // Save the next message so we use it next

      struct iovec *msg_iov = iovecBuffer + totalFragCount;
      struct iovec *curIov = msg_iov;
      fragCount=0;
      nextFrag = msg->firstFragment;
      do 
      {
        frag = nextFrag;
        nextFrag = frag->nextFragment;

        // Fill the iovector with messages
        curIov->iov_base = frag->data();     //((char*)frag->buffer)+frag->start;
        assert((frag->len > 0) && "The Sctp protocol allows sending zero length messages but I think you forgot to set the fragment len field.");
        curIov->iov_len = frag->len;

        fragCount++;
        totalFragCount++;
        curIov++;
      } while(nextFrag);
      assert(msgCount < SAFplusI::SctpTransportMaxMsg);  // or stack buffer will be exceeded
      curvec->msg_hdr.msg_controllen = 0;
      curvec->msg_hdr.msg_control = NULL;
      curvec->msg_hdr.msg_flags = 0;
      curvec->msg_hdr.msg_iov = msg_iov;
      curvec->msg_hdr.msg_iovlen = fragCount;
      curvec++;
      msgCount++;
    } while (next != NULL);

    int clientSock = -1;
    // Get the client socket connected to server socket whose information is provided in origMsg argument
    if (msg->node == Handle::AllNodes) // Send the message to all nodes: get all the client sockets
    {
      /*
      //Loop thru the socket map
      for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
      {
        NodeIDSocketMap::value_type vt = *iter;
        clientSock = vt.second;
      }*/
      //TODO: get node list
      uint_t nodeList[CL_IOC_MAX_NODES]={0}; // If get node list API returns the dynamic list, we don't need to use the static array like this
      uint_t port = origMsg->port + SAFplusI::SctpTransportStartPort;
      for (int i=0;i<CL_IOC_MAX_NODES;i++)
      {
        if (nodeList[i])
        {
          clientSock = getClientSocket(nodeList[i], port);
          int retval = sendmmsg(clientSock, &msgvec[0], msgCount, 0);  // TODO flags
          if (retval == -1)
          {
            int err = errno;
            throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
          }
          else
          {
            assert(retval == msgCount);  // TODO, retry if all messages not sent
          }    
        }
      }
    }
    else
    {
      clientSock = getClientSocket(origMsg->node, origMsg->port+SAFplusI::SctpTransportStartPort);
      int retval = sendmmsg(clientSock, &msgvec[0], msgCount, 0);  // TODO flags
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      else
      {
        assert(retval == msgCount);  // TODO, retry if all messages not sent
        //printf("%d messages sent\n", retval);
      }
    }
    
    // Your send routine releases the message whenever you are ready to do so
    MsgPool* temp = origMsg->msgPool;
    temp->free(origMsg);
  }

  Message* SctpSocket::receive(uint_t maxMsgs, int maxDelay)
  {
      // TODO receive multiple messages
      Message* ret = msgPool->allocMsg();
      MsgFragment* frag = ret->append(SAFplusI::SctpTransportMaxMsgSize);

      mmsghdr msgs[SAFplusI::SctpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
      struct sockaddr_in from[SAFplusI::SctpTransportMaxMsg];
      struct iovec iovecs[SAFplusI::SctpTransportMaxFragments];
      struct timespec timeoutMem;
      struct timespec* timeout;
      uint_t flags = MSG_WAITFORONE;

      if (maxDelay > 0)
        {
        timeout = &timeoutMem;
        timeout->tv_sec = maxDelay/1000;
        timeout->tv_nsec = (maxDelay%1000)*1000L;  // convert milli to nano, multiply by 1 million
        }
      else if (maxDelay == 0)
        {
        timeout = NULL;
        flags = MSG_DONTWAIT;
        }
      else 
        {
        timeout = NULL;
        }

      memset(msgs,0,sizeof(msgs));
      for (int i = 0; i < 1; i++)
        {
#if 0
        // In an inline fragment the message buffer is located right after the MsgFragment object and shares some bytes with the buffer "pointer" (its not used as a pointer)
        if (frag->flags & SAFplus::MsgFragment::InlineFragment) iovecs[i].iov_base         = (void*) &frag->buffer;
        // If the fragment is not inline then the buffer variable works as a normal pointer.
        else iovecs[i].iov_base         = frag->buffer;
#endif
        iovecs[i].iov_base = frag->data(0);
        iovecs[i].iov_len          = frag->allocatedLen;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name    = &from[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
        }

      int retval = recvmmsg(sock, msgs, 1, flags, timeout);
      if (retval == -1)
        {
        int err = errno;
        ret->msgPool->free(ret);  // clean up this unused message.  TODO: save it for the next receive call
        if (errno == EAGAIN) return NULL;  // its ok just no messages received.  This is a "normal" error not an exception
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
      else
        {
#if 0  // Its possible to send a zero length message!  If that happens you'll receive it!  Did you forget to set the len field of your fragment?
        if ((retval==1)&&(msgs[0].msg_len==0))  // Got one empty message.  Not sure why this happens
          {
          ret->msgPool->free(ret);
          return NULL;
          }
#endif

        //printf("%d messages received. [%s]\n", retval,(char*) &frag->buffer);
        Message* cur = ret;
        for (int msgIdx = 0; (msgIdx<retval); msgIdx++,cur = cur->nextMsg)
          {
          int msgLen = msgs[msgIdx].msg_len;
          struct sockaddr_in* srcAddr = (struct sockaddr_in*) msgs[msgIdx].msg_hdr.msg_name;
          assert(msgs[msgIdx].msg_hdr.msg_namelen == sizeof(struct sockaddr_in));
          assert(srcAddr);
          
          cur->port = ntohs(srcAddr->sin_port) - SAFplusI::SctpTransportStartPort;
          cur->node = ntohl(srcAddr->sin_addr.s_addr) & (((Sctp*)transport)->nodeMask);
          MsgFragment* curFrag = cur->firstFragment;
          for (int fragIdx = 0; (fragIdx < msgs[msgIdx].msg_hdr.msg_iovlen) && msgLen; fragIdx++,curFrag=curFrag->nextFragment)
            {
            // Apply the received size to this fragment.  If the fragment is bigger then the msg length then the entire msg must be in this buffer
            if (curFrag->allocatedLen >= msgLen)
              {
              curFrag->len = msgLen;
              msgLen=0;
              }
            else // If the fragment is smaller, the buffer is full and reduce the size of the msgLen temporary by the amount in this buffer
              {
              curFrag->len = curFrag->allocatedLen;
              msgLen -= curFrag->allocatedLen;
              }
            }
          }
        }

      return ret;
   }
 
   void SctpSocket::flush() {}

   SctpSocket::~SctpSocket()
   {
     // Close all the sockets: both server socket itself and client sockets in the map
     close(sock);
     for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
     {
       NodeIDSocketMap::value_type vt = *iter;
       int socket = vt.second;
       close(socket);
     }
   }
};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID;
  SAFplus::api.pluginVersion    = SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER;
  SAFplus::api.type = "SCTP";

  // return it
  return (SAFplus::ClPlugin*) &SAFplus::api;
}
