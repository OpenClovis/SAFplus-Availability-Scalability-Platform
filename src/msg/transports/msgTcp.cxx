//#define _GNU_SOURCE // needed so that sendmmsg is available

#include <clMsgApi.hxx>
#include <clThreadApi.hxx>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <clCommon.hxx>  // For exceptions
#include <clIocConfig.h>
#include "clPluginHelper.hxx"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>

struct SndRecvSock
{
  int sndSock;
  int recvSock;
  SndRecvSock(int sndsock, int recvsock):sndSock(sndsock), recvSock(recvsock){}
};

typedef std::pair<uint_t, SndRecvSock> SockMapPair; 
typedef boost::unordered_map<uint_t, SndRecvSock> NodeIDSocketMap;


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
    Mutex mutex;
    pthread_t tid;     
    //ThreadCondition cond;
    //bool recvBlocked;
    int sock;
    bool nagleEnabled; 
    NodeIDSocketMap clientSockMap; //TODO in case node failure: if a node got failure, this must be notified so that
    // the item associated with it in the map must be deleted too, if not, the associated socket may be invalid because of its server failure

    bool dataAvailOnSndSock;
    int getClientSocket(uint_t nodeID, uint_t port);
    int openClientSocket(uint_t nodeID, uint_t port);    
    static void* acceptClients(void* arg);
    void addToMap(const sockaddr_in& client, int socket);

    //bool updateBuffer(struct msghdr* msg, int msgLen, int bytes);
    //bool updateBuffer(struct msghdr* msg, int curbytes, int bytes);
    bool updateBuffer(struct msghdr* msg, int bytes);
    int checkDataAvail(int sd, const struct timespec* timeout);
    int getMsgLen(struct msghdr* msg);
    virtual void switchNagle();

    int receiveAllOrThrow(int sock, void* buf, unsigned int amt, int flags);
    void sendMsgs(int sd, struct msghdr* msgvec, int msgCount);
    int getAvailSocket(const SndRecvSock& srsock, bool sndSock);
  };
  static Tcp api;

  Tcp::Tcp()
  {
    msgPool = NULL;
    clusterNodes = NULL;
  }

  MsgTransportConfig::Capabilities Tcp::getCapabilities()
  {
    return SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
  }

  MsgTransportConfig& Tcp::initialize(MsgPool& msgPoolp,ClusterNodes* cn)
  {
    msgPool = &msgPoolp;
    clusterNodes = cn;
    
    config.maxMsgSize = SAFplusI::TcpTransportMaxMsgSize;
    config.maxPort    = SAFplusI::TcpTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::TcpTransportMaxMsg;
    config.capabilities = SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
    std::pair<in_addr,in_addr> ipAndBcast = SAFplusI::setNodeNetworkAddr(&nodeMask,clusterNodes);   
    struct in_addr bip = ipAndBcast.first;          
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
    dataAvailOnSndSock = true; //assume sndSocket for send & receiving data 

    cap.capabilities = (SAFplus::MsgSocketCapabilities::Capabilities) xp->config.capabilities;
    cap.maxMsgSize = xp->config.maxMsgSize;
    cap.maxMsgAtOnce = xp->config.maxMsgAtOnce;

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

    int qack = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (void *)&qack, sizeof(qack)) < 0)
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);

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
    
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    logTrace("TCP", "CONS","Create thread to acceptClients");
    pthread_create(&tid, NULL, acceptClients, this);
    pthread_detach(tid);
  }

  void* TcpSocket::acceptClients(void* arg)
  {
    logTrace("TCP", "ACPT","Enter acceptClients()");
    TcpSocket* tcp = static_cast<TcpSocket*>(arg);
    if (!tcp || tcp->sock < 0) return NULL;
    
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    while (true)
    {
      //logTrace("TCP", "ACPT","In the loop acceptClients()");
      int clientSock = accept(tcp->sock, (struct sockaddr *)&clientAddr, &addrlen);
      if (clientSock > 0)
      {
        logTrace("TCP", "ACPT","accept ok, add client to the map()");
        tcp->addToMap(clientAddr, clientSock);
      }
      else
      {
        logNotice("TCP", "ACPT", "accept error: sd [%d]; ret [%d]; errno [%d], errmsg [%s]", tcp->sock, clientSock, errno, strerror(errno));
        return NULL;
      }
      //boost::this_thread::sleep(boost::posix_time::milliseconds(2));
    }
    logTrace("TCP", "ACPT", "QUIT the connection accept loop");
    return NULL;
  }

  void TcpSocket::addToMap(const sockaddr_in& client, int socket)
  {
    logTrace("TCP", "ADD", "Add socket to map");
    uint_t nodeId;
    if (transport->clusterNodes)
    {
      int temp = ntohl(client.sin_addr.s_addr);
      nodeId = transport->clusterNodes->idOf((void*) &temp,sizeof(uint32_t));
    }
    else
    {
      nodeId = ntohl(client.sin_addr.s_addr) & (((Tcp*)transport)->nodeMask);
    }
    mutex.lock();
    logTrace("TCP", "ADD", "Mutex locked; nodeId [%d]; map size [%u]", nodeId, (unsigned int) clientSockMap.size());
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeId);
    if (contents != clientSockMap.end()) // Socket connected to this nodeId has been established
    {
      SndRecvSock& srsock = contents->second;
      if (srsock.recvSock == 0) // there is no receive socket added to the map
      {
        logTrace("TCP", "ADD", "Add client socket [%d]; nodeId [%d] to map", socket, nodeId); 
        srsock.recvSock = socket;
      }
      mutex.unlock();
      return;
    }
    // else add this client socket to the map with the specified nodeId   
    logTrace("TCP", "ADD", "Add client socket [%d]; nodeId [%d] to map ", socket, nodeId);
    assert(socket>0);
    SndRecvSock srsock(0, socket); 
    SockMapPair smp(nodeId, srsock);
    clientSockMap.insert(smp);    
    mutex.unlock();
  }

  int TcpSocket::openClientSocket(uint_t nodeID, uint_t port)
  {
    int sd,ret;    
    struct sockaddr_in addr;    
    memset(&addr, 0, sizeof(struct sockaddr_in));    
    addr.sin_family = AF_INET;
    if (transport->clusterNodes)
    {
      addr.sin_addr.s_addr = htonl(*((uint32_t*)transport->clusterNodes->transportAddress(nodeID)));
    }
    else
    {      
      addr.sin_addr.s_addr = htonl(((Tcp*)transport)->netAddr | nodeID);
    }
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
    
    int qack = 1;
    if ((ret = setsockopt(sd, IPPROTO_TCP, TCP_QUICKACK, (void *)&qack, sizeof(qack))) < 0)
      throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);

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
    ScopedLock<> sl(mutex);
    //logTrace("TCP", "ADD", "Mutex locked. nodeId [%d]", nodeID); 
    int sd = 0;
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeID);
    if (contents != clientSockMap.end()) // Socket connected to this NodeID has been opened
    {
      SndRecvSock& srsock = contents->second;
      if (srsock.sndSock == 0 && node == nodeID) // there is no send socket added to the map and this is same nodeID
      {
        if (srsock.recvSock) sd = srsock.recvSock;
        else {
          int newSock = openClientSocket(nodeID, port);
          logTrace("TCP", "ADD", "Add client socket [%d]; nodeId [%d] to map", newSock, nodeID);
          srsock.sndSock = newSock;
          sd = newSock;
        }
      }
      else if (srsock.sndSock)
      {
        sd = srsock.sndSock;
      }
      else
      {
        sd = srsock.recvSock;
      }   

      return sd;
    }
    if (node == nodeID) // there is no send socket added to the map and this is same nodeID
    {
      int newSock = openClientSocket(nodeID, port);
      logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map", newSock, nodeID); 
      SndRecvSock srsock(newSock, 0); 
      SockMapPair smp(nodeID, srsock);
      assert(newSock>0);
      clientSockMap.insert(smp);
      return newSock;
    }
    contents = clientSockMap.find(nodeID); // there may client socket has been accepted before
    if (contents != clientSockMap.end())
    {
      sd = contents->second.recvSock;
      logTrace("TCP", "ADD", "Different nodeID, got the accepted socket [%d]", sd);       
    }
    else
    {
      sd = openClientSocket(nodeID, port);
      logTrace("TCP", "ADD", "Different nodeID, add client socket [%d]; nodeId[%d] to map", sd, nodeID); 
      assert(sd>0);
      SndRecvSock srsock(sd, 0); 
      SockMapPair smp(nodeID, srsock);
      clientSockMap.insert(smp);
    }   
    return sd;
  }

  void TcpSocket::send(Message* origMsg)
  {
/*
    TCP has no concept "message" but bytes (stream) of data instead.
    When a sender sends a msghdr, there is not sure that the receiver 
    receives the full msghdr, it may receive 10 bytes first, then 100 bytes,
    then 1500 bytes next and so on... Because of that, we have to attach
    a header consisting of an id, source port and the length of the message.
    The header is an unsigned integer number (32 bits) in which id 8 bits,
    source port 8 bits and the length 16 bits.
    The receiver first read the header first to get id, port and length, then
    it reads the message body until the length bytes is reached (for this, we use the
    updateBuffer function). While reading data, if there is no data available within
    the timeout value, NULL will be returned on this socket.
*/
    msghdr msgvec[SAFplusI::TcpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
    bzero(msgvec,sizeof(msgvec));
    struct iovec iovecBuffer[SAFplusI::TcpTransportMaxFragments];
    msghdr* curvec = msgvec;
    int msgCount = 0;
    int fragCount= 0;  // frags in this message
    int totalFragCount = 0; // total number of fragments
    Message* msg;
    Message* next = origMsg;
    MsgFragment* nextFrag;
    MsgFragment* frag;
    int msgSize, retval;
    //MsgFragment* headerFrag = oriMsg->prepend(4); // reserve 4 bytes (32 bits for the header (8 bits id + 8 bits port + 16 bits msg len)
    uint32_t totalLenPerMsg;
    do 
    { 
      msg = next;
      next = msg->nextMsg;  // Save the next message so we use it next
 
      MsgFragment* headerFrag = msg->prepend(8); // reserve 4 bytes (32 bits) as a header for each msg (8 bits id + 8 bits port + 16 bits msg len)
      headerFrag->len = 8;
      totalLenPerMsg = 0;

      struct iovec *msg_iov = iovecBuffer + totalFragCount;
      struct iovec *curIov = msg_iov;
      fragCount=0;
      nextFrag = msg->firstFragment;      
      do 
      {
        frag = nextFrag;
        nextFrag = frag->nextFragment;

        // Fill the iovector with messages
        curIov->iov_base = frag->data();
        assert((frag->len > 0) && "The Tcp protocol allows sending zero length messages but I think you forgot to set the fragment len field.");
        curIov->iov_len = frag->len;
        totalLenPerMsg += frag->len;
        fragCount++;
        totalFragCount++;
        curIov++;        
      } while(nextFrag);     
      assert(msgCount < SAFplusI::TcpTransportMaxMsg);  // or stack buffer will be exceeded
      curvec->msg_controllen = 0;
      curvec->msg_control = NULL;
      curvec->msg_flags = 0;
      curvec->msg_iov = msg_iov;
      curvec->msg_iovlen = fragCount;      
      // fill the header
      totalLenPerMsg -= headerFrag->len; // exclude the len of the header
      uint32_t hdr = ((node<<16)|(port&(1<<16)-1));  
      memcpy(headerFrag->data(0), &hdr, sizeof(uint32_t));
      memcpy(headerFrag->data(sizeof(uint32_t)), &totalLenPerMsg, sizeof(uint32_t));
      curvec++;
      msgCount++;       
    } while (next != NULL);   
   
    uint_t pt = msg->port + SAFplusI::TcpTransportStartPort;
    int clientSock;
    if (msg->node == Handle::AllNodes) // Send the message to all nodes
    {
      if (transport->clusterNodes)
      {        
        for (ClusterNodes::Iterator it=transport->clusterNodes->begin();it != transport->clusterNodes->endSentinel;it++)
        {
          clientSock = getClientSocket(it.nodeId(), pt);
          sendMsgs(clientSock, msgvec, msgCount);
        }
      }
      else
      {
        logWarning("TCP", "SEND", "Broadcast the message to all nodes but the clusterNodes is NULL");
      }
    }        
    else
    {
      clientSock = getClientSocket(msg->node, pt);
      sendMsgs(clientSock, msgvec, msgCount);
    }

    MsgPool* temp = origMsg->msgPool;
    temp->free(origMsg);
  }

  void TcpSocket::sendMsgs(int sd, struct msghdr* msgvec, int msgCount)
  {
    int retval = -1;
    int bytes, msgLen;
    for (int i=0;i<msgCount;i++)
    {
      bytes = 0;
      msgLen = getMsgLen(msgvec+i);
      do 
      {                
        retval = sendmsg(sd, msgvec+i, 0);  
        if (retval == -1)
        {        
          if (errno != EAGAIN)
            throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        else 
        {
          bytes+=retval;
          if (!updateBuffer(msgvec+i, retval)) break;
        }        
        logDebug("TCP", "SEND", "msgLen [%u]; retval [%d]; bytes [%d]; iov_len [%d]", msgLen, retval, bytes, (int)(msgvec+i)->msg_iov->iov_len);
      } while (true);
      assert(bytes==msgLen);
    }
  }

#if 0
  bool TcpSocket::updateBuffer(struct msghdr* msg, int msgLen, int bytes)
  {
    if (bytes < msgLen) 
    {
      msg->msg_iov->iov_len -= bytes;            
      msg->msg_iov->iov_base = (void*)(((char *) msg->msg_iov->iov_base) + bytes);
      return true;
    }    
    return false;
  }
#endif
#if 0
  bool TcpSocket::updateBuffer(struct msghdr* msg, int curbytes, int bytes)
  {
    if (msg->msg_iov->iov_len) 
    {
      msg->msg_iov->iov_len -= curbytes;            
      msg->msg_iov->iov_base = (void*)(((char *) msg->msg_iov->iov_base) + bytes);
      return true;
    }    
    return false;
  }
#endif

  bool TcpSocket::updateBuffer(struct msghdr* msg, int bytes)
  {
    while (msg->msg_iovlen > 0) 
    {
      if (bytes < msg->msg_iov->iov_len) 
      {
        msg->msg_iov->iov_len -= bytes;
        msg->msg_iov->iov_base = (void*)(((char *) msg->msg_iov->iov_base) + bytes);
        return true;
      }
      bytes -= msg->msg_iov->iov_len;
      ++msg->msg_iov;
      --msg->msg_iovlen;
    }
    return false;
  }


  int TcpSocket::getMsgLen(struct msghdr* msg)
  {
    int msgLen = 0;
    for(int i=0;i<msg->msg_iovlen;i++)
    {
      msgLen+=msg->msg_iov[i].iov_len;     
    }    
    return msgLen;
  }

  int TcpSocket::receiveAllOrThrow(int sock, void* buf, unsigned int amt, int flags)
  {
    unsigned int bytesReceived = 0;
    do 
      {                
        int retval = recv(sock, ((unsigned char*)buf) + bytesReceived, amt-bytesReceived, flags);  
        if (retval == -1)
          {
            int err = errno;
            assert(!(err == EFAULT) || (err == EINVAL) || (err ==  ENOTCONN) || (err == ENOTSOCK));  // These programmatic errors should cause a core dump and fail over
            if ((err != EINTR)&&(err != EAGAIN))  // EWOULDBLOCK?
              {
              throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
              }
          }
        else 
          {
            bytesReceived+=retval;            
          }
      } while (bytesReceived < amt);
    return bytesReceived;
  }
  
  Message* TcpSocket::receive(uint_t maxMsgs, int maxDelay)
  {
      size_t mapSize = clientSockMap.size();
      if (!mapSize) 
      {
        return NULL;
      }
      // TODO receive multiple messages

      struct timespec timeoutMem;
      struct timespec* timeout = &timeoutMem;
      uint_t flags = MSG_DONTWAIT;      
      /* the timeout must be divided for all possible sockets equally */
      // There are 2 sockets per map item, so we must divide 2
      if (maxDelay > 0)
      { 
        timeout->tv_sec = maxDelay/1000;
        timeout->tv_nsec = (maxDelay%1000)*1000000L;                
      }
      else
      {
        timeout->tv_sec = 0;
        timeout->tv_nsec = 0;
      }                 
      
      int clientSock;
      int retval=-1;
      unsigned char srcPort;
      int bytes; 
      unsigned int header;
      //fd_set rfds;
      
      fd_set rfds;
      FD_ZERO(&rfds);
      int max=0;

      if (1)  // TODO maintain the fd_set when fds are added or removed so that we don't have to calculate it every time.
	{
	  ScopedLock<> sl(mutex);

	  for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
	    {       
	      SndRecvSock& srsock = iter->second;
	      if (srsock.sndSock > max) max = srsock.sndSock;
	      if (srsock.recvSock > max) max = srsock.recvSock;

	      if (srsock.sndSock > 0)
		FD_SET(srsock.sndSock, &rfds);
	      if (srsock.recvSock > 0)
		FD_SET(srsock.recvSock, &rfds);       
	    }
	}
      
     logTrace("TCP", "RECV", "Waiting for messages. timeout [%u]s/[%u]ns", (unsigned int)timeout->tv_sec, (unsigned int)timeout->tv_nsec);
     retval = pselect(max+1, &rfds, NULL, NULL, timeout, NULL);
     if (retval==-1)
       {
	 int err = errno;
	 if (err == EINTR) return NULL;
	 if (err == EBADF) return NULL;  // bad file descriptor (socket closed) presumably will be cleaned up for next time
	 else
	   {
	     throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
	   }
       }
     if (retval==0) // nothing updated
       {
	 return NULL;
       }


     for(int sock=0;sock<=max;sock++)
       {
	 if (FD_ISSET(sock, &rfds))
	   {
	     logTrace("TCP", "RECV", "Data found on socket [%d]", sock);        
	     // Reading the header of the message to know how much msg body length can be read next
	     uint32_t header[2]; // 8 bytes reserved for reading msg header
	     int fraglen = receiveAllOrThrow(sock, &header, sizeof(uint32_t)*2, 0);
	     int msgLen = header[1];
	     
	     Message* msg = msgPool->allocMsg();
             // TODO: port, node, (and endian) could be determined by a single hello message on the connection and stored in a lookup table, not passed every time
	     msg->port = header[0]&((1<<16)-1);
	     msg->node = header[0]>>16;
	     MsgFragment* frag = msg->append(msgLen);
             int bytesReceived = receiveAllOrThrow(sock, frag->data(0), msgLen, flags);  
	     logDebug("TCP", "RECV", "msgLen [%u] retval [%d] bytes [%d]", msgLen, retval, bytes);            
	     frag->used(bytesReceived);
	     return msg;
	   }
       }     
      //logNotice("TCP", "RECV", "No any msg found on all sockets. Returns NULL");
      return NULL;
   }
 
   int TcpSocket::getAvailSocket(const SndRecvSock& srsock, bool sndSock)
   {
     if (sndSock && srsock.sndSock) return srsock.sndSock;
#if 0
     if (srsock.recvSock) return srsock.recvSock;
     // Wait until server accepts connections
     fd_set rfds;
     FD_ZERO(&rfds);
     FD_SET(sock, &rfds);
     struct timeval acceptTimeout;
     acceptTimeout.tv_sec = 0;
     acceptTimeout.tv_usec = 5000; // 5ms
     int retval = select(sock+1, &rfds, NULL, NULL, &acceptTimeout);
     if (retval == -1) 
     {            
       throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
     }
     else if (!retval)
     {
       logDebug("TCP", "RECV", "No incoming connection on socket [%d] within 5ms", sock);
       return 0;
     }
     if (!srsock.recvSock)
     {
       logNotice("TCP", "RECV", "There is a connection on socket [%d] within 5ms but recvSock is not updated in the sockMap", sock);
     }
#endif
     return srsock.recvSock;              
   }

   int TcpSocket::checkDataAvail(int sd, const struct timespec* timeout)
   {
     if (sd==0) return 0;
     fd_set rfds;
     FD_ZERO(&rfds);
     FD_SET(sd, &rfds);  
     logTrace("TCP", "RECV", "Waiting in timeout [%u]s/[%u]ns on socket [%d]", (unsigned int)timeout->tv_sec, (unsigned int)timeout->tv_nsec, sd);  
     int retval = pselect(sd+1, &rfds, NULL, NULL, timeout, NULL); 
     return retval;
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
     ScopedLock<> sl(mutex);

     for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
     {      
       socket = iter->second.sndSock?iter->second.sndSock:iter->second.recvSock;       
       if((ret = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay))) != 0)
       {         
         logWarning("TCP", "SWTNGL", "switching nagle algorithm for socket [%d] failed. Errno [%d], errmsg [%s]", socket, errno, strerror(errno));
       }
       else
       {
         logTrace("TCP", "SWTNGL", "switch nagle algorithm for socket successfully");
       }
     }
   }
   
   TcpSocket::~TcpSocket()
   {
     // Close all the sockets: both server socket itself and client sockets in the map     
     pthread_cancel(tid);
     ScopedLock<> sl(mutex);         
     for(NodeIDSocketMap::iterator iter = clientSockMap.begin(); iter != clientSockMap.end(); iter++)
     {       
       int socket = iter->second.sndSock;
       if (socket)
       {
         shutdown(socket, SHUT_RDWR);
         close(socket);       
       }
       socket = iter->second.recvSock;
       if (socket)
       {
         shutdown(socket, SHUT_RDWR);
         close(socket);       
       }
     }
     clientSockMap.clear();
     shutdown(sock, SHUT_RDWR);     
     close(sock);
     sock = -1;
     boost::this_thread::sleep(boost::posix_time::milliseconds(100)); // wait for all sockets releasing in 100ms
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
