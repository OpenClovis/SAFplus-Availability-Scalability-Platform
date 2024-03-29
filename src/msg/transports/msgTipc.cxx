//#define _GNU_SOURCE // needed so that sendmmsg is available

#include <clMsgApi.hxx>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include "clPluginHelper.hxx"
#include <clCommon.hxx>  // For exceptions
#if OS_VERSION_CODE < OS_KERNEL_VERSION(2,6,16)
#include <tipc.h>
#else
#include <linux/tipc.h>
#endif

#if (__GLIBC__ < 2 || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 14)))

#if 0
struct mmsghdr {
    struct msghdr msg_hdr;  /* Message header */
    unsigned int  msg_len;  /* Number of bytes transmitted */
};
#endif

static int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags)
{
  for (int i=0;i<vlen;i++)
    {
      msgvec[i].msg_len = sendmsg(sockfd, &msgvec[i].msg_hdr, flags);
    }
  return vlen;
}
#endif

namespace SAFplus
{
  enum TipcConfig
  {
    MinNodeAddress = 1,
    MaxNodeAddress = 255,
  };

  class Tipc:public MsgTransportPlugin_1
    {
  public:
    Tipc(); 

   //? This needs to be available before initialize()
      virtual MsgTransportConfig::Capabilities getCapabilities();

      virtual MsgTransportConfig& initialize(MsgPool& msgPool,ClusterNodes* cn);

      virtual MsgSocket* createSocket(uint_t port);
      virtual void deleteSocket(MsgSocket* sock);

      void getAddress(void); //? Returns zero if no tipc or node not configured
      uint_t cluster;
      uint_t zone;
      uint_t node;
    };

  class TipcSocket:public MsgSocket
    {
    public:
      TipcSocket(){}
      TipcSocket(uint_t port,MsgPool* pool,MsgTransportPlugin_1* transport);
      virtual ~TipcSocket();
      virtual void send(Message* msg);
      virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
      virtual void flush();
    protected:
      int sock;    
    };
  static Tipc api;


  MsgTransportConfig::Capabilities Tipc::getCapabilities()
  {
  return SAFplus::MsgTransportConfig::Capabilities::BROADCAST;
  }
 
  Tipc::Tipc()
    {
    msgPool = NULL;
    clusterNodes = NULL;
    }

  MsgTransportConfig& Tipc::initialize(MsgPool& msgPoolp,ClusterNodes* cn)
    {
    clusterNodes = cn;
    assert((clusterNodes == 0) && "TIPC only supports LAN mode");
    msgPool = &msgPoolp;

    config.nodeId       = 0;
    config.maxMsgSize   = SAFplusI::TipcTransportMaxMsgSize;
    config.maxPort      = SAFplusI::TipcTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::TipcTransportMaxMsg;
    if (clusterNodes == NULL) // LAN mode
      {
      config.capabilities = SAFplus::MsgTransportConfig::Capabilities::BROADCAST;  // not reliable, can't tell if anything joins or leaves...
      }
    else config.capabilities = SAFplus::MsgTransportConfig::Capabilities::NONE;

    // TODO: get this value from tipc
    getAddress();
    assert((node != 0) && "TIPC address must be configured before running SAFplus"); 
    const char* nodeID = getenv("SAFPLUS_NODE_ID");
    if (nodeID)
      {
      unsigned int envNodeId = boost::lexical_cast<unsigned int>(nodeID);
      assert(envNodeId == node); // SAFPLUS_NODE_ID is optional when using the TIPC transport, but if defined it MUST match the TIPC node address.
      }
    config.nodeId = SAFplus::ASP_NODEADDR = node;

    return config;
    }

  void Tipc::getAddress(void)
    {
	struct sockaddr_tipc addr;
	socklen_t sz = sizeof(addr);
	int sd;
        cluster = 0; zone = 0; node = 0;

	sd = socket(AF_TIPC, SOCK_RDM, 0);
	if (sd < 0) return;
	if (getsockname(sd, (struct sockaddr *)&addr, &sz) < 0) return;
	close(sd);
        cluster = tipc_cluster(addr.addr.id.node);
        zone = tipc_zone(addr.addr.id.node);
        node = tipc_node(addr.addr.id.node);
    }

  MsgSocket* Tipc::createSocket(uint_t port)
    {
    if (port >= SAFplusI::TipcTransportNumPorts) return NULL;
    return new TipcSocket(port, msgPool,this);
    }

  void Tipc::deleteSocket(MsgSocket* sock)
    {
    delete sock;
    }


   TipcSocket::TipcSocket(uint_t pt,MsgPool*  pool,MsgTransportPlugin_1* xp)
     {
     port = pt;
     msgPool = pool;
     transport = xp;
     node = xp->config.nodeId;

     cap.capabilities = (SAFplus::MsgSocketCapabilities::Capabilities) xp->config.capabilities;
     cap.maxMsgSize = xp->config.maxMsgSize;
     cap.maxMsgAtOnce = xp->config.maxMsgAtOnce;

     if ((sock = socket(AF_TIPC, SOCK_RDM, 0)) < 0) 
       {
       int err = errno;
       throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
       }

     if (fcntl(sock, F_SETFD, FD_CLOEXEC) < 0)
       {
       int err = errno;
       throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
       }

     // enable broadcast permissions on this socket
     int broadcastEnable=1;
     if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)))
       {
       int err = errno;
       throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);       
       }

     struct sockaddr_tipc myaddr;
     memset((char *)&myaddr, 0, sizeof(myaddr));
#if 1
     myaddr.family = AF_TIPC;
     myaddr.addrtype = TIPC_ADDR_NAME;  // TODO: bind to the interface specified in env var
     myaddr.scope = TIPC_ZONE_SCOPE;
     myaddr.addr.name.domain = 0;
     myaddr.addr.name.name.instance = node;
     myaddr.addr.name.name.type = (port + SAFplusI::TipcTransportStartPort);
#else
     myaddr.family = AF_TIPC;
     myaddr.addrtype = TIPC_ADDR_ID;  // TODO: bind to the interface specified in env var
     myaddr.scope = 0;
     myaddr.addr.id.ref = (port + SAFplusI::TipcTransportStartPort);
     myaddr.addr.id.node = tipc_addr(((Tipc*)transport)->cluster,((Tipc*)transport)->zone,node);
     //myaddr.addr.name.name.type = (port + SAFplusI::TipcTransportStartPort);
#endif

     if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
       {
       int err = errno;
       throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
       }
     }

    TipcSocket::~TipcSocket()
      {
      close(sock);
      sock = -1;
      }

    void TipcSocket::flush() {} // Nothing to do, messages are not cached

    void TipcSocket::send(Message* origMsg)
      {
      mmsghdr msgvec[SAFplusI::TipcTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
      struct iovec iovecBuffer[SAFplusI::TipcTransportMaxFragments];
      mmsghdr* curvec = &msgvec[0];
      int msgCount = 0;
      int fragCount= 0;  // frags in this message
      int totalFragCount = 0; // total number of fragments
      Message* msg;
      Message* next = origMsg;
      MsgFragment* nextFrag;
      MsgFragment* frag;
      struct sockaddr_tipc to[SAFplusI::TipcTransportMaxMsg];
      unsigned int headers[SAFplusI::TipcTransportMaxMsg];

      do {
        msg = next;
        next = msg->nextMsg;  // Save the next message so we use it next

        struct iovec *msg_iov = iovecBuffer + totalFragCount;
        struct iovec *curIov = msg_iov;
        fragCount=0;

        headers[msgCount] = (node<<16)|port;
        // Fill the first iovector with message header
        curIov->iov_base = &headers[msgCount];
        curIov->iov_len = 4;

        fragCount++;
        totalFragCount++;
        curIov++;

        nextFrag = msg->firstFragment;
        do {
          frag = nextFrag;
          nextFrag = frag->nextFragment;

          // Fill the iovector with messages
          curIov->iov_base = frag->data();     //((char*)frag->buffer)+frag->start;
          assert((frag->len > 0) && "The TIPC protocol allows sending zero length messages but I think you forgot to set the fragment len field.");
          curIov->iov_len = frag->len;

          fragCount++;
          totalFragCount++;
          curIov++;
          } while(nextFrag);
        assert(msgCount < SAFplusI::TipcTransportMaxMsg);  // or stack buffer will be exceeded
        bzero(&to[msgCount],sizeof(struct sockaddr_tipc));
        to[msgCount].family = AF_TIPC;

        //printf("messages to [%d:%d]\n",msg->node, msg->port);

        if (msg->node == Handle::AllNodes)
          {
            to[msgCount].addrtype = TIPC_ADDR_NAMESEQ;
            to[msgCount].addr.nameseq.type = msg->port + SAFplusI::TipcTransportStartPort;
            to[msgCount].addr.nameseq.lower = TipcConfig::MinNodeAddress;
            to[msgCount].addr.nameseq.upper = TipcConfig::MaxNodeAddress;
          }
        else
          {
          if (!transport->clusterNodes)
            {
#if 0  // A thought, but does not work
              to[msgCount].addrtype = TIPC_ADDR_ID;
              to[msgCount].scope    = 0;
              to[msgCount].addr.id.ref = msg->port + SAFplusI::TipcTransportStartPort; 
              to[msgCount].addr.id.node = tipc_addr(((Tipc*)transport)->cluster,((Tipc*)transport)->zone,msg->node);                  
#endif
              to[msgCount].addrtype = TIPC_ADDR_NAME;
              to[msgCount].scope    = TIPC_ZONE_SCOPE;
              to[msgCount].addr.name.name.type = msg->port + SAFplusI::TipcTransportStartPort; 
              to[msgCount].addr.name.name.instance = msg->node;
              to[msgCount].addr.name.domain=0;
            }
          else  
            {
            to[msgCount].addrtype = TIPC_ADDR_NAME;
            to[msgCount].scope    = TIPC_ZONE_SCOPE;
            to[msgCount].addr.name.name.type = msg->port + SAFplusI::TipcTransportStartPort; 
            uint32_t* tipcAddr = (uint32_t*)transport->clusterNodes->transportAddress(msg->node);
            if (!tipcAddr)
              {
              throw Error(Error::SAFPLUS_ERROR,Error::DOES_NOT_EXIST, "Destination node does not exist in cluster node table",__FILE__,__LINE__);
              }
            to[msgCount].addr.name.name.instance = *tipcAddr;
            to[msgCount].addr.name.domain=0;
            }
          }

        curvec->msg_hdr.msg_controllen = 0;
        curvec->msg_hdr.msg_control = NULL;
        curvec->msg_hdr.msg_flags = 0;
        curvec->msg_hdr.msg_name = &to[msgCount];
        curvec->msg_hdr.msg_namelen = sizeof (struct sockaddr_tipc);
        curvec->msg_hdr.msg_iov = msg_iov;
        curvec->msg_hdr.msg_iovlen = fragCount;

        curvec++;
        msgCount++;
        } while (next != NULL);
        int retval = sendmmsg(sock, &msgvec[0], msgCount, 0);  // TODO flags
        if (retval == -1)
          {
            int err = errno;
            //throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
            logError("TIPC","SND","system error number [%d], error message [%s]", err, strerror(err));
          }
        else
          {
            assert(retval == msgCount);  // TODO, retry if all messages not sent
            //printf("%d messages sent\n", retval);
          }
      // Your send routine releases the message whenever you are ready to do so
      MsgPool* temp = origMsg->msgPool;
      temp->free(origMsg);
      }


    Message* TipcSocket::receive(uint_t maxMsgs,int maxDelay)
      {

      // TODO receive multiple messages
      Message* ret = msgPool->allocMsg();
      MsgFragment* frag = ret->append(SAFplusI::TipcTransportMaxMsgSize);
      unsigned int header;

      mmsghdr msgs[SAFplusI::TipcTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
      struct sockaddr_tipc from[SAFplusI::TipcTransportMaxMsg];
      struct iovec iovecs[SAFplusI::TipcTransportMaxFragments];
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
#if 0
        // In an inline fragment the message buffer is located right after the MsgFragment object and shares some bytes with the buffer "pointer" (its not used as a pointer)
        if (frag->flags & SAFplus::MsgFragment::InlineFragment) iovecs[i].iov_base         = (void*) &frag->buffer;
        // If the fragment is not inline then the buffer variable works as a normal pointer.
        else iovecs[i].iov_base         = frag->buffer;
#endif
        // First ioVector is IP header, next is data
        iovecs[0].iov_base = &header;
        iovecs[0].iov_len  = 4;

        iovecs[1].iov_base = frag->data(0);
        iovecs[1].iov_len  = frag->allocatedLen;

        msgs[i].msg_hdr.msg_iov    = iovecs;
        msgs[i].msg_hdr.msg_iovlen = 2;
        msgs[i].msg_hdr.msg_name    = &from[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_tipc);
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
          int msgLen = msgs[msgIdx].msg_len - 4; // 4 bytes reserved for IP header
          struct sockaddr_tipc* srcAddr = (struct sockaddr_tipc*) msgs[msgIdx].msg_hdr.msg_name;
          assert(msgs[msgIdx].msg_hdr.msg_namelen == sizeof(struct sockaddr_tipc));
          assert(srcAddr);

          cur->node = (header & (0xffff<<16))>>16;
          cur->port = header & 0xffff;

          MsgFragment* curFrag = cur->firstFragment;
          for (int fragIdx = 0; (fragIdx < (msgs[msgIdx].msg_hdr.msg_iovlen - 1)) && msgLen; fragIdx++,curFrag=curFrag->nextFragment)
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
};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
  {
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID;
  SAFplus::api.pluginVersion    = SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER;
  SAFplus::api.type = "TIPC";

  // return it
  return (SAFplus::ClPlugin*) &SAFplus::api;
  }

