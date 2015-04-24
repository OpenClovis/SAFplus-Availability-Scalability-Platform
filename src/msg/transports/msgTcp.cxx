//#define _GNU_SOURCE // needed so that sendmmsg is available

#include <clMsgApi.hxx>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <clCommon.hxx>  // For exceptions
#include <clIocConfig.h>
#include "clPluginHelper.hxx"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>

typedef boost::unordered_map<uint_t, int> NodeIDSocketMap;

namespace SAFplus
{ 
  class Tcp: public MsgTransportPlugin_1
    {
  public:
    in_addr_t netAddr;
    uint32_t nodeMask;
    Tcp(); 

    virtual MsgTransportConfig& initialize(MsgPool& msgPool,ClusterNodes* cn);

    virtual MsgSocket* createSocket(uint_t port);
    virtual void deleteSocket(MsgSocket* sock);
    virtual MsgTransportConfig::Capabilities getCapabilities();
    };

  class TcpSocket: public MsgSocket
  {
  public:    
    TcpSocket(uint_t port,MsgPool* pool,MsgTransportPlugin_1* transport);    
    virtual void send(Message* msg);
    virtual Message* receive(uint_t maxMsgs,int maxDelay=-1);
    virtual void flush();
    virtual void useNagle(bool value);    
    virtual ~TcpSocket();
  protected:
    int sock;
    //uint_t listenPort;
    bool quitting;
    bool nagleEnabled; 
    NodeIDSocketMap clientSockMap; //TODO in case node failure: if a node got failure, this must be notified so that
    // the item associated with it in the map must be deleted too, if not, the associated socket may be invalid because of its server failure
    int getClientSocket(uint_t nodeID, uint_t port);
    int openClientSocket(uint_t nodeID, uint_t port);    
    static void* acceptClients(void* arg);
    void addToMap(sockaddr_in& client, int socket);
    //Message* recvAll(uint_t maxMsgs,int maxDelay=-1);
    virtual void switchNagle();
    void handleError(int retval);
  };
  static Tcp api;

  Tcp::Tcp()
  {
    msgPool = NULL;
  }

  MsgTransportConfig::Capabilities Tcp::getCapabilities()
  {
    return SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
  }

  MsgTransportConfig& Tcp::initialize(MsgPool& msgPoolp,ClusterNodes* cn)
  {
    msgPool = &msgPoolp;
    
    config.maxMsgSize = SAFplusI::TcpTransportMaxMsgSize;
    config.maxPort    = SAFplusI::TcpTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::TcpTransportMaxMsg;
    config.capabilities = SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
    
    struct in_addr bip = SAFplusI::setNodeNetworkAddr(&nodeMask);      
    config.nodeId = SAFplus::ASP_NODEADDR;
    netAddr = ntohl(bip.s_addr)&(~nodeMask);

    return config;
  }

  MsgSocket* Tcp::createSocket(uint_t port)
  {
    if (port >= SAFplusI::TcpTransportNumPorts) return NULL;
    return new TcpSocket(port, msgPool,this); // open a listen socket
  }

  void Tcp::deleteSocket(MsgSocket* sock)
  {
    delete sock;
  }

  TcpSocket::TcpSocket(uint_t pt,MsgPool*  pool,MsgTransportPlugin_1* xp)
  {    
    port = pt;
    msgPool = pool;
    transport = xp;
    node = xp->config.nodeId;
    nagleEnabled = false; // Nagle algorithm is disabled by default
    quitting = false;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
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
      myaddr.sin_port = htons(port + SAFplusI::TcpTransportStartPort);
    }
    
    if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    } 
    if (listen(sock, CL_IOC_MAX_NODES) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    pthread_t tid;
    pthread_create(&tid, NULL, acceptClients, this);
    pthread_detach(tid);
  }

  void* TcpSocket::acceptClients(void* arg)
  {
    TcpSocket* tcp = (TcpSocket*)arg;
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    while (!tcp->quitting)
    {
      int clientSock = accept(tcp->sock, (struct sockaddr *)&clientAddr, &addrlen);
      if (clientSock > 0)
      {
        tcp->addToMap(clientAddr, clientSock);
      }
      else
      {
        logError("TCP", "ACPT", "accept error errno [%d], errmsg [%s]", errno, strerror(errno));
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }
    logDebug("TCP", "ACPT", "QUIT the connection accept loop");
    return NULL;
  }

  void TcpSocket::addToMap(sockaddr_in& client, int socket)
  {
    logDebug("TCP", "ADD", "ADD TO MAP ENTER");
    uint_t nodeId = ntohl(client.sin_addr.s_addr) & (((Tcp*)transport)->nodeMask);
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeId);
    if (contents != clientSockMap.end()) // Socket connected to this nodeId has been established, no-op
    {
      return;
    }
    // else add this client socket to the map with the specified nodeId   
    logDebug("TCP", "ADD", "ADD CLIENT SOCKET [%d] TO MAP", socket); 
    clientSockMap[nodeId] = socket;
  }

  int TcpSocket::openClientSocket(uint_t nodeID, uint_t port)
  {
    int sd,ret;    
    struct sockaddr_in addr;    
    memset(&addr, 0, sizeof(struct sockaddr_in));    
    addr.sin_family = AF_INET;      
    addr.sin_addr.s_addr = htonl(((Tcp*)transport)->netAddr | nodeID);
    addr.sin_port = htons(port);

    if((ret = (sd = socket(AF_INET, SOCK_STREAM, 0))) < 0)    
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    
    if (!nagleEnabled)
    {
      int nodelay = 1;
      if ((ret = setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay))) < 0)
      { 
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
    }

    /*Connect to server*/
    if(((ret = connect(sd, (struct sockaddr*)&addr, sizeof(struct sockaddr)))) < 0)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    return sd;
  }

  int TcpSocket::getClientSocket(uint_t nodeID, uint_t port)
  {
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeID);
    if (contents != clientSockMap.end()) // Socket connected to this NodeID has been opened
    {
      return contents->second;
    }
    int newSock = openClientSocket(nodeID, port);
    logDebug("TCP", "ADD", "ADD CLIENT SOCKET [%d] TO MAP", newSock); 
    clientSockMap[nodeID] = newSock;
    return newSock;
  }
  
  void TcpSocket::send(Message* origMsg)
  {
/*
    TCP has no concept "message" but byte (stream) of data instead. So, we cannot send a structure of message 
    over it. We need to send data in bytes subsequently: we send the followings in order: msgCount, fragCount,
    dataLen(1) (length of actual data), data(1), dataLen(2), data(2),...dataLen(n), data(n).
    The receive side must read in the same way, then fills data to mmsghdr struct
*/
    mmsghdr msgvec[SAFplusI::TcpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
    bzero(msgvec,sizeof(msgvec));
    struct iovec iovecBuffer[SAFplusI::TcpTransportMaxFragments];
    mmsghdr* curvec = &msgvec[0];
    int msgCount = 0;
    int fragCount= 0;  // frags in this message
    int totalFragCount = 0; // total number of fragments
    Message* msg;
    Message* next = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    int msgSize;
    int intSize = sizeof(int);
    int totalMsgSize = SAFplusI::TcpTransportMaxMsgSize+msgSizeLen;
    char buf[totalMsgSize];    
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
        assert((frag->len > 0) && "The Tcp protocol allows sending zero length messages but I think you forgot to set the fragment len field.");
        curIov->iov_len = frag->len;

        fragCount++;
        totalFragCount++;
        curIov++;
      } while(nextFrag);     
      assert(msgCount < SAFplusI::TcpTransportMaxMsg);  // or stack buffer will be exceeded
      curvec->msg_hdr.msg_controllen = 0;
      curvec->msg_hdr.msg_control = NULL;
      curvec->msg_hdr.msg_flags = 0;
      curvec->msg_hdr.msg_iov = msg_iov;
      curvec->msg_hdr.msg_iovlen = fragCount;
      curvec++;
      msgCount++;
    } while (next != NULL);
   
    uint_t port = msg->port + SAFplusI::TcpTransportStartPort;
    int clientSock = getClientSocket(msg->node, port);
    // Send msgCount first
    int temp = htonl(msgCount);
    int retval = ::send(clientSock, &temp, intSize, 0);
    if (ret == -1)
    {
      int err = errno;
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
    }
    // Send each fragment including fragCount and iovec buffer
    for (int i=0;i<msgCount;i++)
    {
      // Send fragCount
      int msg_iovlen = msgs[i].msg_hdr.msg_iovlen;
      temp = htonl(msg_iovlen);
      retval = ::send(clientSock, &temp, intSize, 0);
      if (ret == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      for (int j=0;j<msg_iovlen;j++)
      {      
        //logDebug("TCP", "SEND", "Sending msg to socket [%d]", clientSock);
        // Send iov len (the length of each iov element)
        int iovlen = (msgs[i].msg_hdr.msg_iov+j)->iov_len;
        temp = htonl(iovlen);
        retval = ::send(clientSock, &temp, intSize, 0);
        if (retval == -1)
        {
          int err = errno;
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        // Send each iovec buffer
        retval = ::send(clientSock, (msgs[i].msg_hdr.msg_iov+j)->iov_base, iov_len, 0);
        if (retval == -1)
        {
          int err = errno;
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        else
        {
          assert(retval == iovlen); 
        }
      }
    }
    MsgPool* temp = origMsg->msgPool;
    temp->free(origMsg);
  }

  Message* TcpSocket::receive(uint_t maxMsgs, int maxDelay)
  {
      // TODO receive multiple messages
      Message* ret = msgPool->allocMsg();
      MsgFragment* frag = ret->append(SAFplusI::TcpTransportMaxMsgSize);

      mmsghdr msgs[SAFplusI::TcpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
      //struct sockaddr_in from[SAFplusI::TcpTransportMaxMsg];
      struct iovec iovecs[SAFplusI::TcpTransportMaxFragments];
      struct timespec timeoutMem;
      struct timespec* timeout;
      uint_t flags = MSG_WAITFORONE;

      int intSize = sizeof(int);
      int msgCount, fragCount, fragLen, temp;

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
        iovecs[i].iov_base = frag->data(0);
        iovecs[i].iov_len          = frag->allocatedLen;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_len = 1;
        //msgs[i].msg_hdr.msg_name    = &from[i];
        //msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
      }
     
      int retval, clientSock;
      // Receive message from any socket from the map
      for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
      {       
        NodeIDSocketMap::value_type vt = *iter;
        clientSock = vt.second; 
        retval = recv(clientSock, &temp, intSize, flags);
        if (retval == -1)
        {
          int err = errno;
          //ret->msgPool->free(ret);  // clean up this unused message.  TODO: save it for the next receive call
          if (errno == EAGAIN) 
          {
            logWarning("TCP", "RECV", "Msg not found on socket [%d]. Continue other one", clientSock);
            continue;  // its ok just no messages received on this socket.  This is a "normal" error not an exception, so try on another socket
          }
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        else
        {
          //printf("%d messages received. [%s]\n", retval,(char*) &frag->buffer);
          //logDebug("TCP", "RECV", "Msg received found on socket [%d]", clientSock);
          msgCount = ntohl(temp);
          for (int i=0;i<msgCount;i++)
          {
            retval = recv(clientSock, &temp, intSize, flags);
            if (retval == -1)
            {
              int err = errno;         
              throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
            }
            fragCount = ntohl(temp);
            msgs[i].msg_hdr.msg_iovlen = fragCount;
            
            for (int j=0;j<fragCount;j++)
            {
              retval = recv(clientSock, &temp, intSize, flags);
              if (retval == -1)
              {
                int err = errno;         
                throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
              }
              fragLen = ntohl(temp);
              retval = recv(clientSock, (msgs[i].msg_hdr.msg_iov+j)->iov_base, fragLen, flags);
              if (retval == -1)
              {
                int err = errno;         
                throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
              }
              (msgs[i].msg_hdr.msg_iov+j)->iov_len = fragLen;
            }
            break; // Because we only receive one message per time
          }
          retval = 1;         
          Message* cur = ret;
          for (int msgIdx = 0; (msgIdx<retval); msgIdx++,cur = cur->nextMsg)          
          {  
            int msgLen = msgs[msgIdx].msg_len;
            #if 0
            struct sockaddr_in* srcAddr = (struct sockaddr_in*) msgs[msgIdx].msg_hdr.msg_name;
            assert(msgs[msgIdx].msg_hdr.msg_namelen == sizeof(struct sockaddr_in));
            assert(srcAddr);
            cur->port = ntohs(srcAddr->sin_port) - SAFplusI::TcpTransportStartPort;
            cur->node = ntohl(srcAddr->sin_addr.s_addr) & (((Tcp*)transport)->nodeMask);
            #endif
            MsgFragment* curFrag = cur->firstFragment;
            //cur->port = curFrag->srcPort;        
            //logInfo("TCP", "RECV", "set srcPort [%d] to the received msg", curFrag->srcPort);
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
          return ret;
        }
      }      
      ret->msgPool->free(ret); // clean up this unused message
      logWarning("TCP", "RECV", "No any msg found on all sockets. Returns NULL");
      return NULL;
   }
 
   void TcpSocket::flush() {}

   void TcpSocket::useNagle(bool value)
   {
     if (value == nagleEnabled) 
     {       
       logNotice("TCP", "USENGL", "No-op for using Nagle");
       return; // No-op
     }    
     nagleEnabled = value;
     switchNagle();     
   }

   void TcpSocket::switchNagle()
   {
     int nodelay, ret, socket;
     if (nagleEnabled)
     {
       nodelay = 0;
     }
     else
     {
       nodelay = 1;
     }
     for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
     {
       NodeIDSocketMap::value_type vt = *iter;
       socket = vt.second;       
       if((ret = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay))) != 0)
       {         
         logWarning("TCP", "SWTNGL", "switching nagle algorithm for socket [%d] failed. Errno [%d], errmsg [%s]", socket, errno, strerror(errno));
       }
       else
       {
         logInfo("TCP", "SWTNGL", "switch nagle algorithm for socket successfully");
       }
     }
   }   

   /*void TcpSocket::handleError(int retval)
   {
     if (retval == -1)
     {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
     }
   }*/

   TcpSocket::~TcpSocket()
   {
     // Close all the sockets: both server socket itself and client sockets in the map
     quitting = true;
     int ret = shutdown(sock, SHUT_RDWR);
     if (ret < 0)
     {
       //perror("close server socket");
       logError("TCP","DES","SHUTDOWN SERVER SOCKET ERROR : errno [%d]; errmsg [%s]",errno,strerror(errno));       
     }
     else
       logDebug("TCP","DES","SHUTDOWN SERVER SOCKET OK");
     ret = close(sock);
     if (ret < 0)
     {
       //perror("close server socket");
       logError("TCP","DES","CLOSE SERVER SOCKET ERROR : errno [%d]; errmsg [%s]",errno,strerror(errno));       
     }
     else
       logDebug("TCP","DES","CLOSE SERVER SOCKET OK");
     for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
     {
       NodeIDSocketMap::value_type vt = *iter;
       int socket = vt.second;
       ret = shutdown(socket, SHUT_RDWR);
       if (ret < 0)
       {
         perror("close client socket");
         logError("TCP","DES","SHUTDOWN CLIENT SOCKET ERROR : errno [%d]; errmsg [%s]",errno,strerror(errno));       
       }
       else
         logDebug("TCP","DES","SHUTDOWN CLIENT SOCKET OK");
       ret = close(socket);
       if (ret < 0)
       {
         perror("close client socket");
         logError("TCP","DES","CLOSE CLIENT SOCKET ERROR : errno [%d]; errmsg [%s]",errno,strerror(errno));       
       }
       else
         logDebug("TCP","DES","CLOSE CLIENT SOCKET OK");
     }
     clientSockMap.clear();
   }
};

extern "C" SAFplus::ClPlugin* clPluginInitialize(uint_t preferredPluginVersion)
{
  // We can only provide a single version, so don't bother with the 'preferredPluginVersion' variable.

  // Initialize the pluginData structure
  SAFplus::api.pluginId         = SAFplus::CL_MSG_TRANSPORT_PLUGIN_ID;
  SAFplus::api.pluginVersion    = SAFplus::CL_MSG_TRANSPORT_PLUGIN_VER;
  SAFplus::api.type = "TCP";

  // return it
  return (SAFplus::ClPlugin*) &SAFplus::api;
}
