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

int sctp_sendmsg(int s, const void *msg, size_t len, struct sockaddr *to,
	     socklen_t tolen, uint32_t ppid, uint32_t flags,
                 uint16_t stream_no, uint32_t timetolive, uint32_t context);

typedef boost::unordered_map<uint_t, int> NodeIDSocketMap;

namespace SAFplus
{ 
  class Sctp: public MsgTransportPlugin_1
    {
  public:
    in_addr_t netAddr;
    uint32_t nodeMask;
    Sctp(); 

    virtual MsgTransportConfig& initialize(MsgPool& msgPool,ClusterNodes* cn);

    virtual MsgSocket* createSocket(uint_t port);
    virtual void deleteSocket(MsgSocket* sock);

    //? This needs to be available before initialize()
      virtual MsgTransportConfig::Capabilities getCapabilities();

    };

  class SctpSocket: public MsgSocket
  {
  public:    
    SctpSocket(uint_t port,MsgPool* pool,MsgTransportPlugin_1* transport);    
    virtual void send(Message* msg);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    virtual void flush();
    virtual void useNagle(bool value);    
    virtual ~SctpSocket();
  protected:
    int sock;
    bool nagleEnabled; 
    //NodeIDSocketMap clientSockMap; //TODO in case node failure: if a node got failure, this must be notified so that
    // the item associated with it in the map must be deleted too, if not, the associated socket may be invalid because of its server failure
    //int getClientSocket(uint_t nodeID, uint_t port);
    //int openClientSocket(uint_t nodeID, uint_t port);    
    virtual void switchNagle();
  };
  static Sctp api;

  Sctp::Sctp()
  {
    msgPool = NULL;
    clusterNodes = NULL;
  }

  MsgTransportConfig::Capabilities Sctp::getCapabilities()
  {
  return SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
  }


  MsgTransportConfig& Sctp::initialize(MsgPool& msgPoolp,ClusterNodes* cn)
  {
    clusterNodes = cn;
    msgPool = &msgPoolp;
    
    config.maxMsgSize = SAFplusI::SctpTransportMaxMsgSize;
    config.maxPort    = SAFplusI::SctpTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::SctpTransportMaxMsg;
    config.capabilities = SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
    std::pair<in_addr,in_addr> ipAndBcast = SAFplusI::setNodeNetworkAddr(&nodeMask,clusterNodes);   
    struct in_addr bip = ipAndBcast.first;    
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
    nagleEnabled = false; // Nagle algorithm is disabled by default

    cap.capabilities = (SAFplus::MsgSocketCapabilities::Capabilities) xp->config.capabilities;
    cap.maxMsgSize = xp->config.maxMsgSize;
    cap.maxMsgAtOnce = xp->config.maxMsgAtOnce;

    if ((sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0) 
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }

    struct sctp_event_subscribe event;  
    struct sctp_paddrparams heartbeat;    
    struct sctp_rtoinfo rtoinfo;
    memset(&event,      0, sizeof(struct sctp_event_subscribe));
    memset(&heartbeat,  0, sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0, sizeof(struct sctp_rtoinfo));


    heartbeat.spp_flags = SPP_HB_ENABLE; // set heartbeat so that the multi-homing enabled
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;
    rtoinfo.srto_max = 2000;
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

#if 0 // Note, if you enable events, you must make sure that the received event is NOT passed up to the application layer as an empty message
    event.sctp_data_io_event = 1;
    event.sctp_association_event = 1;
    event.sctp_send_failure_event = 1;

    if(setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(struct sctp_event_subscribe)) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
    }
#endif

    if (!nagleEnabled)
    {
      int nodelay=1;  
      if(setsockopt(sock, SOL_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay)) != 0)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
    }

    /* set the reuse of address*/
    int reuse = 1;
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


  void SctpSocket::send(Message* origMsg)
  {
    mmsghdr msgvec[SAFplusI::SctpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
    struct sockaddr_in to[SAFplusI::SctpTransportMaxMsg];
    bzero(msgvec,sizeof(msgvec));
    struct iovec iovecBuffer[SAFplusI::SctpTransportMaxFragments];
    char ctrldata[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    mmsghdr* curvec = &msgvec[0];
    int msgCount = 0;
    int fragCount= 0;  // frags in this message
    int totalFragCount = 0; // total number of fragments
    Message* msg;
    Message* next = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;


    // set up the first one so that CMSG_XXX macros will work
    curvec->msg_hdr.msg_controllen = sizeof(ctrldata);
    curvec->msg_hdr.msg_control = ctrldata;

    // TODO is the same control data per message OK?
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&curvec->msg_hdr);
    cmsg->cmsg_level = IPPROTO_SCTP;
    cmsg->cmsg_type = SCTP_SNDRCV;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

    curvec->msg_hdr.msg_controllen = cmsg->cmsg_len;
    struct sctp_sndrcvinfo *sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
    memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
    sinfo->sinfo_ppid = 1; // ppid;
    sinfo->sinfo_flags = 0; // SCTP_ADDR_OVER; // flags;
    sinfo->sinfo_stream = 1; // stream_no;
    sinfo->sinfo_timetolive = 0; // timetolive;
    sinfo->sinfo_context = 0; // context;
    
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

      bzero(&to[msgCount],sizeof(struct sockaddr_in));
      to[msgCount].sin_family = AF_INET;
      // right now, can't mix broadcast and single destination messages. TODO: split broadcast messages out so they are mixable.
      if (msg->node == Handle::AllNodes)
          to[msgCount].sin_addr.s_addr= 0;  // TODO: this will broadcast simply by messaging all known nodes.  I need to fix this down in the AllNodes handler
      else
        {
          if (transport->clusterNodes)
            {
              uint32_t* addr = (uint32_t*) transport->clusterNodes->transportAddress(msg->node);
              if (addr)
                to[msgCount].sin_addr.s_addr = htonl(*((uint32_t*)transport->clusterNodes->transportAddress(msg->node)));
              else
                {
                  throw Error(Error::SAFPLUS_ERROR,Error::DOES_NOT_EXIST, "Destination node does not exist in cluster node table",__FILE__,__LINE__);
                }
            }
          else
            {
            to[msgCount].sin_addr.s_addr = htonl(((Sctp*)transport)->netAddr | msg->node);
            }
        }
      to[msgCount].sin_port=htons(msg->port + SAFplusI::SctpTransportStartPort);

      curvec->msg_hdr.msg_controllen = cmsg->cmsg_len;
      curvec->msg_hdr.msg_control = ctrldata;
      curvec->msg_hdr.msg_name = &to[msgCount];
      curvec->msg_hdr.msg_namelen = sizeof (struct sockaddr_in);
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
      //uint_t nodeList[CL_IOC_MAX_NODES]={0}; // If get node list API returns the dynamic list, we don't need to use the static array like this
      if (transport->clusterNodes)
        {
          uint_t port = origMsg->port + SAFplusI::SctpTransportStartPort;
          for (ClusterNodes::Iterator it=transport->clusterNodes->begin();it != transport->clusterNodes->endSentinel;it++)
            {
              for (int i=0;i<msgCount;i++)  // Fix up the destination addresses
                {
                  //assert(to[i].sin_addr.s_addr== 0);
                  in_addr_t* t = (in_addr_t*) it.transportAddress();
		  if (!t)
		    throw Error(Error::SAFPLUS_ERROR,Error::DOES_NOT_EXIST,"Destination does not exist",__FILE__,__LINE__);
                  to[i].sin_addr.s_addr = htonl(*t);
                }

              int retval = sendmmsg(sock, &msgvec[0], msgCount, 0);  // TODO flags
              if (retval == -1)
                {
                  int err = errno;
                  //throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
                  logError("SCTP","SND","system error number [%d], error message [%s]", err, strerror(err));
                  break;
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
      int retval = sendmmsg(sock, &msgvec[0], msgCount, 0);  // TODO flags

      if (retval == -1)
      {
        int err = errno;
        //char* errstr = strerror(errno);
        //throw Error(Error::SYSTEM_ERROR,errno,errstr,__FILE__,__LINE__);
        logError("SCTP","SND","system error number [%d], error message [%s]", err, strerror(err));
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
      struct sockaddr_in from[SAFplusI::SctpTransportMaxMsg] = {0};
      struct iovec iovecs[SAFplusI::SctpTransportMaxFragments];
      struct timespec timeoutMem;
      struct timeval timeout4sockopt;
      struct timespec* timeout;
      uint_t flags = MSG_WAITFORONE;

      if (maxDelay > 0)
        {
        timeout = &timeoutMem;
        timeout->tv_sec = maxDelay/1000;
        timeout->tv_nsec = (maxDelay%1000)*1000000L;  // convert milli to nano, multiply by 1 million

        timeout4sockopt.tv_sec = maxDelay/1000;
        timeout4sockopt.tv_usec = (maxDelay%1000)*1000L;  // convert milli to micro, multiply by 1 thousand
        }
      else if (maxDelay == 0)
        {
        timeout = NULL;
        flags = MSG_DONTWAIT;
        }
      else 
        {
        timeout = NULL;
        timeout4sockopt.tv_sec = INT_MAX;  // basically forever
        timeout4sockopt.tv_usec = 0;  // convert milli to micro, multiply by 1 thousand
        }

      memset(msgs,0,sizeof(msgs));
      for (int i = 0; i < 1; i++)
        {
        iovecs[i].iov_base = frag->data(0);
        iovecs[i].iov_len          = frag->allocatedLen;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name    = &from[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
        }

      if (timeout)
        {
          // For Linux only.  For Windows, pass a 32 bit integer in milliseconds.
          if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout4sockopt, sizeof(timeout4sockopt)) < 0)
            {
              assert(0);
            }
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
          if (transport->clusterNodes)  // Cloud mode
            {
              int tmp = ntohl(srcAddr->sin_addr.s_addr);
              cur->node = transport->clusterNodes->idOf((void*) &tmp,sizeof(uint32_t));
            }
          else  // LAN mode
            {
            cur->node = ntohl(srcAddr->sin_addr.s_addr) & (((Sctp*)transport)->nodeMask);
            }

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

   void SctpSocket::useNagle(bool value)
   {
     if (value == nagleEnabled) 
     {       
       logNotice("SCTP", "USENGL", "No-op for using Nagle");
       return; // No-op
     }    
     nagleEnabled = value;
     switchNagle();     
   }

   void SctpSocket::switchNagle()
   {
     int nodelay,ret;
     if (nagleEnabled)
     {
       nodelay = 0;
     }
     else
     {
       nodelay = 1;
     }

     if((ret = setsockopt(sock, SOL_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay))) != 0)
       {         
         logWarning("SCTP", "SWTNGL", "Switching nagle algorithm for socket [%d] failed. Errno [%d], errmsg [%s]", sock, errno, strerror(errno));
       }
       else
       {
         logDebug("SCTP", "SWTNGL", "Switched nagle algorithm for socket successfully.");
       }
     
   }   

   SctpSocket::~SctpSocket()
   {
     // Close all the sockets: both server socket itself and client sockets in the map
     close(sock);
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
