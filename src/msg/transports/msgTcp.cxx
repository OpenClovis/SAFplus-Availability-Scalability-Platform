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
    //ThreadCondition cond;
    //bool recvBlocked;
    int sock;
    bool nagleEnabled; 
    NodeIDSocketMap clientSockMap; //TODO in case node failure: if a node got failure, this must be notified so that
    // the item associated with it in the map must be deleted too, if not, the associated socket may be invalid because of its server failure
    int getClientSocket(uint_t nodeID, uint_t port);
    int openClientSocket(uint_t nodeID, uint_t port);    
    static void* acceptClients(void* arg);
    void addToMap(sockaddr_in& client, int socket);
    virtual void switchNagle();
    void sendStream(int sd, short srcPort, mmsghdr* msgvec, int msgCount);
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
    clusterNodes = cn;
    
    config.maxMsgSize = SAFplusI::TcpTransportMaxMsgSize;
    config.maxPort    = SAFplusI::TcpTransportNumPorts;
    config.maxMsgAtOnce = SAFplusI::TcpTransportMaxMsg;
    config.capabilities = SAFplus::MsgTransportConfig::Capabilities::NAGLE_AVAILABLE;
    
    struct in_addr bip = SAFplusI::setNodeNetworkAddr(&nodeMask, clusterNodes);      
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
    //recvBlocked = true;

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
    //pthread_attr_t tattr;
    //pthread_attr_init(&tattr);
    //pthread_attr_setschedpolicy(&tattr, SCHED_RR);
    logTrace("TCP", "CONS","Create thread to acceptClients");
    pthread_create(&tid, NULL, acceptClients, this);
    pthread_detach(tid);
  }

  void* TcpSocket::acceptClients(void* arg)
  {
    logTrace("TCP", "ACPT","Enter acceptClients()");
    TcpSocket* tcp = (TcpSocket*)arg;
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
        logNotice("TCP", "ACPT", "accept error: sd [%d]; errno [%d], errmsg [%s]", tcp->sock, errno, strerror(errno));
        return NULL;
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(2));
    }
    logTrace("TCP", "ACPT", "QUIT the connection accept loop");
    return NULL;
  }

  void TcpSocket::addToMap(sockaddr_in& client, int socket)
  {
    logTrace("TCP", "ADD", "Add socket to map");
    uint_t nodeId = ntohl(client.sin_addr.s_addr) & (((Tcp*)transport)->nodeMask);
    mutex.lock();
    logTrace("TCP", "ADD", "Mutex locked. nodeId [%d]. map size [%ld]", nodeId, clientSockMap.size());
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeId);
    if (contents != clientSockMap.end()) // Socket connected to this nodeId has been established
    {
      SndRecvSock& srsock = contents->second;
      if (srsock.recvSock == 0) // there is no receive socket added to the map
      {
        logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map", socket, nodeId); 
        srsock.recvSock = socket;
      }
      //recvBlocked = false;
      mutex.unlock();
      return;
    }
    // else add this client socket to the map with the specified nodeId   
    logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map ", socket, nodeId); 
    SndRecvSock srsock(0, socket); 
    SockMapPair smp(nodeId, srsock);
    clientSockMap.insert(smp);
    //recvBlocked = false;
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
    mutex.lock();
    logTrace("TCP", "ADD", "Mutex locked. nodeId [%d]", nodeID); 
    int sd = 0;
    NodeIDSocketMap::iterator contents = clientSockMap.find(nodeID);
    if (contents != clientSockMap.end()) // Socket connected to this NodeID has been opened
    {
      SndRecvSock& srsock = contents->second;
#if 0           
      if (srsock.recvSock)
      {        
        sd = srsock.recvSock;
      }
      else if (srsock.sndSock == 0 && node == nodeID) // there is no send socket added to the map and this is same nodeID
      { 
        int newSock = openClientSocket(nodeID, port);
        logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map", newSock, nodeID); 
        srsock.sndSock = newSock;
        sd = newSock;
      }
      else
      {
        sd = srsock.sndSock;
      }
#endif
#if 1
     if (srsock.sndSock == 0 && node == nodeID) // there is no send socket added to the map and this is same nodeID
      {
        if (srsock.recvSock) sd = srsock.recvSock;
        else {
          int newSock = openClientSocket(nodeID, port);
          logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map", newSock, nodeID);
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
#endif
      //cond.notify_one();
      //recvBlocked = false;
      mutex.unlock();
      return sd;
    }
    if (node == nodeID) // there is no send socket added to the map and this is same nodeID
    {
      int newSock = openClientSocket(nodeID, port);
      logTrace("TCP", "ADD", "Add client socket [%d]; nodeId[%d] to map", newSock, nodeID); 
      SndRecvSock srsock(newSock, 0); 
      SockMapPair smp(nodeID, srsock);
      clientSockMap.insert(smp);
      //cond.notify_one();
      //recvBlocked = false;
      mutex.unlock();
      return newSock;
    }
    contents = clientSockMap.find(nodeID);
    if (contents != clientSockMap.end())
    {
      sd = contents->second.recvSock;
      logTrace("TCP", "ADD", "Different nodeID, got the accepted socket [%d]", sd);       
    }
    else
    {
      sd = openClientSocket(nodeID, port);
      logTrace("TCP", "ADD", "Different nodeID, add client socket [%d]; nodeId[%d] to map", sd, nodeID); 
      SndRecvSock srsock(sd, 0); 
      SockMapPair smp(nodeID, srsock);
      clientSockMap.insert(smp);
    }   
    mutex.unlock();    
    return sd;
  }
  
  void TcpSocket::send(Message* origMsg)
  {
/*
    TCP has no concept "message" but bytes (stream) of data instead. So, we cannot send a structure of message 
    over it. We need to send data in bytes subsequently: we send the followings in order: port fragCount,
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
    int msgSize, retval;
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
   
    uint_t pt = msg->port + SAFplusI::TcpTransportStartPort;
    int clientSock;
    if (msg->node == Handle::AllNodes) // Send the message to all nodes
    {
      if (transport->clusterNodes)
      {        
        for (ClusterNodes::Iterator it=transport->clusterNodes->begin();it != transport->clusterNodes->endSentinel;it++)
        {
          clientSock = getClientSocket(it.nodeId(), pt);
          sendStream(clientSock, port, &msgvec[0], msgCount);
        }
      }
    }
    else
    {
      clientSock = getClientSocket(msg->node, pt);
      sendStream(clientSock, port, &msgvec[0], msgCount);
    }
#if 0
    // Send each fragment including fragCount and iovec buffer
    for (int i=0;i<msgCount;i++)
    {
      // First attach the port to stream so that receiver knows where it is from
      short srcPort = htons(port);
      retval = ::send(clientSock, &srcPort, sizeof(short), 0);
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      // Second, send fragment count (number of fragment contained in the message)
      int msg_iovlen = msgvec[i].msg_hdr.msg_iovlen;
      int temp = htonl(msg_iovlen);
      retval = ::send(clientSock, &temp, intSize, 0);
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      for (int j=0;j<msg_iovlen;j++)
      {      
        //logDebug("TCP", "SEND", "Sending msg to socket [%d]", clientSock);
        // Send iov len (the length of each iov element)
        int iovlen = (msgvec[i].msg_hdr.msg_iov+j)->iov_len;
        temp = htonl(iovlen);
        retval = ::send(clientSock, &temp, intSize, 0);
        if (retval == -1)
        {
          int err = errno;
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        // Send each iovec buffer
        retval = ::send(clientSock, (msgvec[i].msg_hdr.msg_iov+j)->iov_base, iovlen, 0);
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
#endif
    MsgPool* temp = origMsg->msgPool;
    temp->free(origMsg);
  }
#if 0
  void TcpSocket::sendStream(int sd, short srcPort, mmsghdr* msgvec, int msgCount)
  {
    logDebug("TCP", "SENDSTR", "Sending msg to socket [%d]", sd);
    for (int i=0;i<msgCount;i++)
    {
      // First attach the port to stream so that receiver knows where it is from
      short pt = htons(srcPort);
      int retval = ::send(sd, &pt, sizeof(pt), 0);

      logDebug("TCP", "SENDSTR", "Send port retval [%d]", retval);
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      // Second, send fragment count (number of fragment contained in the message)
      int msg_iovlen = msgvec[i].msg_hdr.msg_iovlen;
      int temp = htonl(msg_iovlen);
      retval = ::send(sd, &temp, sizeof(temp), 0);

      logDebug("TCP", "SENDSTR", "Send fragCount retval [%d]", retval);
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      for (int j=0;j<msg_iovlen;j++)
      {      
        //logDebug("TCP", "SEND", "Sending msg to socket [%d]", clientSock);
        // Send iov len (the length of each iov element)
        int iovlen = (msgvec[i].msg_hdr.msg_iov+j)->iov_len;
        temp = htonl(iovlen);
        retval = ::send(sd, &temp, sizeof(temp), 0);

        logDebug("TCP", "SENDSTR", "Send fragLen retval [%d]", retval);
        if (retval == -1)
        {
          int err = errno;
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        // Send each iovec buffer
        retval = ::send(sd, (msgvec[i].msg_hdr.msg_iov+j)->iov_base, iovlen, 0);

        logDebug("TCP", "SENDSTR", "Send data retval [%d]", retval);
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
  }
#endif
  void TcpSocket::sendStream(int sd, short srcPort, mmsghdr* msgvec, int msgCount)
  {
    logDebug("TCP", "SENDSTR", "Sending msg to socket [%d]", sd);
    int maxBufSize = SAFplusI::TcpTransportMaxMsgSize*2;
    char buf[maxBufSize];    
    int bufSize, intSize = sizeof(int);
    for (int i=0;i<msgCount;i++)
    {
      bufSize = intSize;
      bzero(buf, maxBufSize);
      // First attach the port to stream so that receiver knows where it is from
      short pt = htons(srcPort);      
      memcpy(buf+bufSize, &pt, sizeof(pt));
      bufSize += sizeof(pt);
      // Second, attach fragment count (number of fragment contained in the message)
      int msg_iovlen = msgvec[i].msg_hdr.msg_iovlen;
      int temp = htonl(msg_iovlen);
      memcpy(buf+bufSize, &temp, intSize);
      bufSize += intSize;
      for (int j=0;j<msg_iovlen;j++)
      {        
        // Attach iov len (the length of each iov element)
        int iovlen = (msgvec[i].msg_hdr.msg_iov+j)->iov_len;
        temp = htonl(iovlen);        
        memcpy(buf+bufSize, &temp, intSize);
        bufSize += intSize;
        // Attach each iovec buffer        
        memcpy(buf+bufSize, (msgvec[i].msg_hdr.msg_iov+j)->iov_base, iovlen);
        bufSize += iovlen;
      }
      // Attach message size at the head of the buffer: msgSize = bufSize - space in byte of bufSize in the buffer
      temp = htonl(bufSize-intSize);
      memcpy(buf, &temp, intSize);
      int retval = ::send(sd, buf, bufSize, 0);
      logDebug("TCP", "SENDSTR", "Sending msg retval [%d]", retval);
      if (retval == -1)
      {
        int err = errno;
        throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
      }
      else
      {
        assert(retval == bufSize); 
      }
    }
  }


  Message* TcpSocket::receive(uint_t maxMsgs, int maxDelay)
  {
      // TODO receive multiple messages
      Message* ret = msgPool->allocMsg();
      MsgFragment* frag = ret->append(SAFplusI::TcpTransportMaxMsgSize);

      mmsghdr msgs[SAFplusI::TcpTransportMaxMsg];  // We are doing this for perf so we certainly don't want to new or malloc it!
      //struct sockaddr_in from[SAFplusI::TcpTransportMaxMsg];
      struct iovec iovecs[SAFplusI::TcpTransportMaxFragments];
      //struct timeval timeoutMem;
      //struct timeval* timeout = &timeoutMem;
      uint_t flags = MSG_DONTWAIT;     

      /*if (maxDelay > 0)
      {        
        timeout->tv_sec = maxDelay/1000;
        timeout->tv_usec = (maxDelay%1000)*1000L;  // convert millisec to microsec, multiply by 1 thousand
      }
      else
      {
        timeout->tv_sec = 0;
        timeout->tv_usec = 0;
      }*/     
      int maxTry;
      if (maxDelay < 0 || !maxDelay)
      {
        maxTry = clientSockMap.size(); // if no delay is needed, then at least performing loop over the socket map
      }
      else
      {
        maxTry = maxDelay*clientSockMap.size();
      }
      
      memset(msgs,0,sizeof(msgs));
      for (int i = 0; i < 1; i++)
      {
        iovecs[i].iov_base = frag->data(0);
        iovecs[i].iov_len          = frag->allocatedLen;
        msgs[i].msg_hdr.msg_iov    = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        //msgs[i].msg_len = 1;
        //msgs[i].msg_hdr.msg_name    = &from[i];
        //msgs[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
      }     
      int maxBufSize = SAFplusI::TcpTransportMaxMsgSize*2;
      char buf[maxBufSize];
      int bufSize = 0, msgSize, intSize = sizeof(int);
      bzero(buf, maxBufSize);

      int fragCount, fragLen, temp, clientSock;
      int retval=-1, tries=0;
      short srcPort, pt;
      int maxTryTillClientIsAccepted = 10;
      //fd_set rfds;      
      // Receive message from any socket from the map     
      int n = 0, tryTillClientIsAccepted; 
      //logDebug("TCP", "RECV", "Loop with maxTry [%d]", maxTry);
      NodeIDSocketMap::iterator iter = clientSockMap.begin();
      while (iter!=clientSockMap.end() && tries<maxTry)
      {       
        n++;
        logDebug("TCP", "RECV", "loop n[%d], tries [%d]", n, tries);
        SndRecvSock& srsock = iter->second; 
        /*FD_ZERO(&rfds);
        FD_SET(clientSock, &rfds);
        retval = select(clientSock+1, &rfds, NULL, NULL, timeout);
        if (retval == -1) 
        {
          int err = errno;
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }
        else if (!retval)
        {
          logNotice("TCP", "RECV", "Msg recv timeout on socket [%d]. Continue other one", clientSock);          
          continue;
        }
        retval = recv(clientSock, &pt, sizeof(short), flags);
        if (retval == -1)
        {
          int err = errno;          
          throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
        }*/
        
        clientSock = srsock.sndSock;

          
          if (!clientSock) goto try_other_sock;
          logDebug("TCP", "RECV", "Try to receive msg on socket [%d]", clientSock);                  
          retval = recv(clientSock, &temp, intSize, flags);          
          if (retval == -1)
          {                   
            logDebug("TCP", "RECV", "errno [%d]", errno);
            if (errno == EAGAIN)  // its ok just no messages received.  This is a "normal" error
            {
try_other_sock:
              logDebug("TCP", "RECV", "Trying other sock");              
              if (maxDelay<=0)
              {
                tryTillClientIsAccepted=0;
                while (!srsock.recvSock && tryTillClientIsAccepted<maxTryTillClientIsAccepted) 
                { 
                  tryTillClientIsAccepted++;
                  logTrace("TCP", "RECV", "Wait till recvSock up to > 0");
                  boost::this_thread::sleep(boost::posix_time::milliseconds(1));                                    
                }               
              }
              clientSock = srsock.recvSock;
              if (clientSock) 
              {
                logDebug("TCP", "RECV", "Try to receive msg on socket [%d]", clientSock); 
                retval = recv(clientSock, &temp, intSize, flags);
                if (retval == -1)
                {
                  if (errno != EAGAIN) 
                  {                 
                    throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
                  }                               
                }
                else goto next_reading;             
              }
              
              boost::this_thread::sleep(boost::posix_time::milliseconds(1));
              goto next_reading;
            }           
            else
              throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
          }
          else
            goto next_reading;
          
        
next_reading:
        logDebug("TCP", "RECV", "next_reading retval [%d]", retval);
        if (retval > 0)
        {          
          for (int i=0;i<1;i++)
          {  
#if 0
            srcPort = ntohs(pt);
            // next, read the fragment count  
            retval = recv(clientSock, &temp, sizeof(temp), flags);
            
            logDebug("TCP", "RECV", "fragCount retval [%d]", retval);
            if (retval == -1) logError("TCP", "RECV", "errno [%d]", errno);
            fragCount = ntohl(temp);  
            msgs[i].msg_hdr.msg_iovlen = fragCount;
            int msgLen = 0;
            for (int j=0;j<fragCount;j++)
            {
              retval = recv(clientSock, &temp, sizeof(temp), flags);

              logDebug("TCP", "RECV", "fragLen retval [%d]", retval);

              if (retval == -1) logError("TCP", "RECV", "errno [%d]", errno);
              if (retval == -1)
              {
                int err = errno;         
                throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
              }
              fragLen = ntohl(temp);
              retval = recv(clientSock, (msgs[i].msg_hdr.msg_iov+j)->iov_base, fragLen, flags);

              logDebug("TCP", "RECV", "data retval [%d]", retval);

            if (retval == -1) logError("TCP", "RECV", "errno [%d]", errno);
              if (retval == -1)
              {
                int err = errno;         
                throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
              }
              (msgs[i].msg_hdr.msg_iov+j)->iov_len = fragLen;
              msgLen+=fragLen;
            }     
            msgs[i].msg_len = msgLen;    
#endif
            msgSize = ntohl(temp);
            retval = recv(clientSock, buf, msgSize, flags);
            logDebug("TCP", "RECV", "Receive msg retval [%d]", retval);
            if (retval == -1)
            {
              int err = errno;          
              throw Error(Error::SYSTEM_ERROR,errno, strerror(errno),__FILE__,__LINE__);
            }
            else
              assert(retval==msgSize);
            memcpy(&pt, buf, sizeof(pt));            
            srcPort = ntohs(pt);
            bufSize += sizeof(pt);
            // next, read the fragment count  
            memcpy(&temp, buf+bufSize, intSize);            
            fragCount = ntohl(temp);            
            msgs[i].msg_hdr.msg_iovlen = fragCount;
            bufSize += intSize;
            int msgLen = 0;
            for (int j=0;j<fragCount;j++)
            {
              memcpy(&temp, buf+bufSize, intSize);            
              bufSize += intSize;
              fragLen = ntohl(temp);
              memcpy((msgs[i].msg_hdr.msg_iov+j)->iov_base, buf+bufSize, fragLen);
              (msgs[i].msg_hdr.msg_iov+j)->iov_len = fragLen;
              bufSize += fragLen;
              msgLen+=fragLen;
            } 
            msgs[i].msg_len = msgLen;
          }

          retval = 1;         
          Message* cur = ret;
          for (int msgIdx = 0; (msgIdx<retval); msgIdx++,cur = cur->nextMsg)          
          {  
            int msgLen = msgs[msgIdx].msg_len;            
            cur->port = srcPort;
            cur->node = iter->first;
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
          return ret;
        }
        
        if (n == clientSockMap.size()) // this is the last element of map
        {
          iter = clientSockMap.begin();
          n = 0;
          logDebug("TCP", "RECV", "Re-loop the map: n[%d], tries [%d]", n, tries);
        }
        else
        {
          iter++;
        }
        tries++;
      }      
      ret->msgPool->free(ret); // clean up this unused message
      //logNotice("TCP", "RECV", "No any msg found on all sockets. Returns NULL");
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
     shutdown(sock, SHUT_RDWR);     
     close(sock);
     sock = -1;
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
