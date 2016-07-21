#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <clIocUserApi.h>
#include "clUdpSetup.h"
#include "clRudp.h"
#define CL_SOCKLEN_T socklen_t

/* Global variables */
bool_t rngSeeded = false;
struct RudpSocketList *socketListHead = NULL;
ClOsalMutexT socketListMutex;


/* Creates a new sender session and appends it to the socket's session list */
void createSenderSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *to, struct data **dataQueue)
{
  clLogTrace("UDPRELIABLE", "SEND", "create sender session");
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL)
  {
    clLogError("UDPRELIABLE", "SEND", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSession->address = *to;
  newSession->next = NULL;
  newSession->receiver = NULL;
  struct SenderSession * newSenderSession = malloc(sizeof(struct SenderSession));
  if(newSenderSession == NULL)
  {
    clLogError("UDPRELIABLE", "SEND", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSenderSession->status = SYN_SENT;
  newSenderSession->seqno = seqno;
  newSenderSession->sessionFinished = false;
  /* Add data to the new session's queue */
  newSenderSession->dataQueue = *dataQueue;
  newSession->sender=newSenderSession;
  ClInt32T i;
  for(i = 0; i < RUDP_WINDOW; i++)
  {

	  newSenderSession->retransmissionAttempts[i] = 0;
	  newSenderSession->dataTimeout[i] = 0;
	  newSenderSession->slidingWindow[i] = NULL;
  }
  newSenderSession->synRetransmitAttempts = 0;
  newSenderSession->finRetransmitAttempts = 0;

  if(socket->sessionsListHead == NULL)
  {
    socket->sessionsListHead = newSession;
    clLogTrace("UDPRELIABLE", "SEND", "Add sender session to session list. First Session");

  }
  else
  {
    struct session *currSession = socket->sessionsListHead;
    while(currSession->next != NULL)
    {
      currSession = currSession->next;
    }
    currSession->next = newSession;
    clLogTrace("UDPRELIABLE", "SEND", "Add sender session to session list");
  }
}

/* Creates a new receiver session and appends it to the socket's session list */
void createReceiverSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *addr)
{
  clLogTrace("UDPRELIABLE", "RECV","Create receive sesion for socket");
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL)
  {
    clLogError("UDPRELIABLE", "RECV", "createReceiverSession: Error allocating memory\n");
    return;
  }
  newSession->address = *addr;
  newSession->next = NULL;
  newSession->sender = NULL;

  struct ReceiverSession *newReceiverSession = malloc(sizeof(struct ReceiverSession));
  if(newReceiverSession == NULL)
  {
    clLogError("UDPRELIABLE", "RECV", "createReceiverSession: Error allocating memory\n");
    return;
  }
  newReceiverSession->status = OPENING;
  newReceiverSession->sessionFinished = false;
  newReceiverSession->expected_seqno = seqno;
  newSession->receiver = newReceiverSession;

  if(socket->sessionsListHead == NULL)
  {
    socket->sessionsListHead = newSession;
  }
  else
  {
    struct session *currSession = socket->sessionsListHead;
    while(currSession->next != NULL)
    {
      currSession = currSession->next;
    }
    currSession->next = newSession;
  }
  clLogTrace("UDPRELIABLE", "RECV","Create receive sesion successful");
}

/* Allocates a RUDP packet and returns a pointer to it */
  //struct RudpPacket *createRudpPacket(ClUint16T type, ClUint32T seqno, ClInt32T len, char *payload,ClInt32T flag)
  //{
  //  struct rudp_hdr header;
  //  header.version = RUDP_VERSION;
  //  header.type = type;
  //  header.seqno = seqno;
  //
  //  struct RudpPacket *packet = malloc(sizeof(struct RudpPacket));
  //  if(packet == NULL)
  //  {
  //    clLogDebug("IOC", "RELIABLE", "createRudpPacket: Error allocating memory for packet\n");
  //    return NULL;
  //  }
  //  packet->header = header;
  //  packet->flag=flag;
  //  packet->payloadLength = len;
  //  memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
  //  if(payload != NULL)
  //    memcpy(&packet->payload, payload, len);
  //
  //  return packet;
  //}

struct RudpPacket *createRudpPacketNew(ClUint16T type, ClUint32T seqno, ClInt32T len, char *payload,ClInt32T flag)
{
  clLogTrace("UDPRELIABLE","PACKET","Create UPD Packet");
  struct rudp_hdr header;
  header.version = RUDP_VERSION;
  header.type = type;
  header.seqno = seqno;
  struct RudpPacket *packet = malloc(sizeof(struct RudpPacket));
  if(packet == NULL)
  {
    clLogError("UDPRELIABLE","PACKET", "createRudpPacket: Error allocating memory for packet\n");
    return NULL;
  }
  packet->payloadLength=len;
  struct iovec *iov=NULL;
  if(type==1)
  {
    if(payload==NULL)
    {
      clLogTrace("UDPRELIABLE","PACKET","Payload is null");
    }
    else
    {
      clLogTrace("UDPRELIABLE","PACKET","Create DATA packet");
    }
    iov =(struct iovec *)payload;
    ClIocHeaderT* iocHeader = (ClIocHeaderT*)iov->iov_base;
    int length= iov->iov_len;
    iocHeader->seqno=seqno;
    iocHeader->type=type;
    memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
    clLogTrace("UDPRELIABLE","PACKET","memcopy payload to packet with length [%d]",length);
    if(payload != NULL)
      memcpy(&packet->payload, payload, length);
    clLogTrace("UDPRELIABLE","PACKET","Create DATA packet with length [%d]",length);
    packet->payloadLength = len;
  }
  else
  {
      clLogTrace("UDPRELIABLE", "PACKET","Create control packet \n");
  }
  packet->header = header;
  packet->flag=flag;
  return packet;
}


/* Returns 1 if the two sockaddr_in structs are equal and 0 if not */
ClInt32T compareSockaddr(struct sockaddr_in *s1, struct sockaddr_in *s2)
{
  char sender[16];
  char recipient[16];
  strcpy(sender, inet_ntoa(s1->sin_addr));
  strcpy(recipient, inet_ntoa(s2->sin_addr));
  return ((s1->sin_family == s2->sin_family) && (strcmp(sender, recipient) == 0) && (s1->sin_port == s2->sin_port));
}


//ClBoolT isSessionDuplicate(int sockfd)
//{
//  if(socketListHead == NULL)
//  {
//    return CL_FALSE;
//  }
//  else
//  {
//    struct RudpSocketList *curr = socketListHead;
//    while(curr->next != NULL)
//    {
//      if(curr->socketfd==sockfd)
//      {
//        clOsalMutexUnlock(&socketListMutex);
//        return CL_TRUE;
//      }
//    }
//  }
//  clOsalMutexUnlock(&socketListMutex);
//  return false;
//}

int rudpSocketFromUdpSocket(int sockfd)
{
  clLogTrace("UDPRELIABLE", "SOCKET", "Create reliable UDP socket from UDP socket\n");
  if(rngSeeded == false)
  {
    srand(time(NULL));
    rngSeeded = true;
  }
  if(sockfd < 0)
  {
    return -1;
  }
  clOsalMutexLock(&socketListMutex);

  if(socketListHead == NULL)
  {
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    struct RudpSocketList *newSocket = malloc(sizeof(struct RudpSocketList));
    if(newSocket == NULL)
    {
      clLogError("UDPRELIABLE", "SOCKET", "rudpSocket: Error allocating memory for socket list");
      return -1;
    }
    newSocket->socketfd=sockfd;
    newSocket->rsock = socket;
    newSocket->closeRequested = false;
    newSocket->sessionsListHead = NULL;
    newSocket->next = NULL;
    newSocket->handler = NULL;
    newSocket->recvHandler = NULL;
    socketListHead = newSocket;
  }
  else
  {
    struct RudpSocketList *currentSocket = socketListHead;
    while(currentSocket->next != NULL)
    {
      if(currentSocket->socketfd==sockfd)
      {
        clOsalMutexUnlock(&socketListMutex);
        clLogTrace("UDPRELIABLE", "SOCKET", "Socket is already exists. Return...");
        return 1;
      }
      currentSocket = currentSocket->next;
    }
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    struct RudpSocketList *newSocket = malloc(sizeof(struct RudpSocketList));
    if(newSocket == NULL)
    {
      clLogError("UDPRELIABLE", "SOCKET","RudpSocket: Error allocating memory for socket list");
      return -1;
    }
    newSocket->socketfd=sockfd;
    newSocket->rsock = socket;
    newSocket->closeRequested = false;
    newSocket->sessionsListHead = NULL;
    newSocket->next = NULL;
    newSocket->handler = NULL;
    newSocket->recvHandler = NULL;
    currentSocket->next = newSocket;
  }
  clOsalMutexUnlock(&socketListMutex);
  clLogTrace("UDPRELIABLE", "SOCKET", "Create reliable UDP socket from UDP socket successful");
  return 0;
}

ClInt32T receivePacket(ClInt32T file, struct RudpPacket* receivedPacket,struct sockaddr_in* sender)
{
  char buf[sizeof(struct RudpPacket)];
  ClInt32T bytes;
  CL_SOCKLEN_T sender_length = sizeof(struct sockaddr_in);
  bytes=recvfrom(file, &buf, sizeof(struct RudpPacket), 0, (struct sockaddr *)sender, &sender_length);
  memcpy(receivedPacket, &buf, sizeof(struct RudpPacket));
  return bytes;
}

/* Close a RUDP socket */
ClInt32T rudpClose(rudpSocketT rsocket)
{
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket->next != NULL)
  {
    if(curr_socket->rsock == rsocket)
    {
      break;
    }
    curr_socket = curr_socket->next;
  }
  if(curr_socket->rsock == rsocket)
  {
    curr_socket->closeRequested = true;
    return 0;
  }
  return -1;
}

/* Register receive callback function */
ClInt32T rudpRecvfromHandler(rudpSocketT rsocket, ClInt32T(*handler)( rudpSocketT, struct sockaddr_in *, char *, int))
{
  if(handler == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "rudpRecvfromHandler failed: handler callback is null\n");
    return -1;
  }
  /* Find the proper socket from the socket list */
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket->next != NULL)
  {
    if(curr_socket->rsock == rsocket)
    {
      break;
    }
    curr_socket = curr_socket->next;
  }
  /* Extra check to handle the case where an invalid rsock is used */
  if(curr_socket->rsock == rsocket)
  {
    curr_socket->recvHandler = handler;
    return 0;
  }
  return -1;
}

ClInt32T rudpSendto(rudpSocketT rsocket, void* data , ClInt32T len, struct sockaddr_in* to,ClInt32T flag)
{
  clLogTrace("UDPRELIABLE","SEND","Sending reliable paket...");
  if(len < 0 || len > RUDP_MAXPKTSIZE)
  {
    clLogError("UDPRELIABLE","SEND", "rudpSendto Error: attempting to send with invalid max packet size\n");
    return -1;
  }

  if((long)rsocket < 0)
  {
      clLogError("UDPRELIABLE","SEND", "rudpSendto Error: attempting to send on invalid socket\n");
    return -1;
  }

  if(to == NULL)
  {
      clLogError("UDPRELIABLE","SEND", "rudpSendto Error: attempting to send to an invalid address\n");
    return -1;
  }

  bool_t newSessionCreated = true;
  ClUint32T seqno = 0;
  if(socketListHead == NULL)
  {
      clLogError("UDPRELIABLE","SEND", "Error: attempt to send on invalid socket. No sockets in the list\n");
    return -1;
  }
  else
  {
    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL)
    {
      if(curr_socket->rsock == rsocket)
      {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if(curr_socket->rsock == rsocket)
    {
      /* We found the correct socket, now see if a session already exists for this peer */
      struct data *data_item = malloc(sizeof(struct data));
      if(data_item == NULL)
      {
        clLogDebug("UDPRELIABLE","SEND", "rudpSendto: Error allocating data queue\n");
        return -1;
      }
      data_item->item = malloc(len);
      if(data_item->item == NULL)
      {
        clLogDebug("UDPRELIABLE","SEND", "rudpSendto: Error allocating data queue item\n");
        return -1;
      }
      memcpy(data_item->item, data, len);
      data_item->len = len;
      data_item->next = NULL;
      if(curr_socket->sessionsListHead == NULL)
      {
        clLogTrace("UDPRELIABLE","SEND", "No session in current socket. Create sender socket. Add data to data queue");
        seqno = rand();
        createSenderSession(curr_socket, seqno, to, &data_item);
      }
      else
      {
        bool_t session_found = false;
        struct session *currSession = curr_socket->sessionsListHead;
        while(currSession != NULL)
        {
          clLogTrace("UDPRELIABLE","SEND", "Finding session for destination address...");
          if(compareSockaddr(&currSession->address, to) == 1)
          {
            bool_t data_is_queued = false;
            bool_t we_must_queue = true;
            if(currSession->sender == NULL)
            {
              clLogTrace("UDPRELIABLE","SEND", "Sender session is null. Send a sync paket ");
              seqno = rand()%65535;
              createSenderSession(curr_socket, seqno, to, &data_item);
              struct RudpPacket *p = createRudpPacketNew(RUDP_SYN, seqno, 0, NULL ,0);
              sendPacketNew(false, rsocket, p, to);
              free(p);
              newSessionCreated = false; /* Dont send the SYN twice */
              break;
            }
            if(currSession->sender->dataQueue != NULL)
            {
              clLogTrace("UDPRELIABLE","SEND", "Data is queued in sender session");
              data_is_queued = true;
            }
            if(currSession->sender->status == OPEN && !data_is_queued)
            {
              clLogTrace("UDPRELIABLE","SEND", "Data queue is empty . Send Data packet");
              ClInt32T i;
              for(i = 0; i < RUDP_WINDOW; i++)
              {
                if(currSession->sender->slidingWindow[i] == NULL)
                {
                  clLogDebug("IOC", "RELIABLE", "Debug 8\n");
                  currSession->sender->seqno = currSession->sender->seqno + 1;
                  struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, currSession->sender->seqno, len, data,flag);
                  currSession->sender->slidingWindow[i] = datap;
                  currSession->sender->retransmissionAttempts[i] = 0;
                  sendPacketNew(false, rsocket, datap, to);
                  we_must_queue = false;
                  break;
                }
              }
            }
            if(we_must_queue == true)
            {
              clLogTrace("UDPRELIABLE","SEND", "Don't send the packet. Add to data queue");
              if(currSession->sender->dataQueue == NULL)
              {
                /* First entry in the data queue */
                currSession->sender->dataQueue = data_item;
              }
              else
              {
                /* Add to end of data queue */
                struct data *curr_data = currSession->sender->dataQueue;
                while(curr_data->next != NULL)
                {
                   curr_data = curr_data->next;
                }
                curr_data->next = data_item;
              }
              clLogTrace("UDPRELIABLE","SEND", "Packet is added to data queue");
            }
            session_found = true;
            //newSessionCreated = false;
            break;
          }
          currSession = currSession->next;
        }
        if(!session_found)
        {
          seqno = rand()%65535;
          createSenderSession(curr_socket, seqno, to, &data_item);
        }
      }
    }
    else
    {
      clLogError("UDPRELIABLE","SEND", "Error: attempt to send on invalid socket. Socket not found\n");
      return -1;
    }
  }
  if(newSessionCreated == true)
  {
    /* Send the SYN for the new session */
    clLogTrace("UDPRELIABLE","SEND", "New Session created. Send SYN packet...");
    struct RudpPacket *p = createRudpPacketNew(RUDP_SYN, seqno, 0, NULL,0);
    sendPacketNew(false, rsocket, p, to);
    free(p);
  }
  return 0;
}


ClInt32T sendPacketNew(bool_t is_ack, rudpSocketT rsocket, struct RudpPacket *p, struct sockaddr_in *recipient)
{
  char type[5];
  short t = p->header.type;
  if(t == 1)
    strcpy(type, "DATA");
  else if(t == 2)
    strcpy(type, "ACK");
  else if(t == 4)
    strcpy(type, "SYN");
  else if(t == 5)
    strcpy(type, "FIN");
  else
    strcpy(type, "BAD");

  clLogDebug("UDPRELIABLE","SEND", "Sending %s packet to %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(recipient->sin_addr), ntohs(recipient->sin_port),
      p->header.seqno, (ClInt32T) (long) rsocket);

  if(DROP != 0 && rand() % DROP == 1)
  {
    clLogError("UDPRELIABLE","SEND", "Dropped\n");
  }
  else
  {
    struct msghdr msghdr;
    ClIocHeaderT userHeader = { 0 };
    struct iovec controlMessage;
    memset(&msghdr, 0, sizeof(msghdr));
    msghdr.msg_name = (struct sockaddr*)recipient;
    msghdr.msg_namelen = sizeof(struct sockaddr_in);
    msghdr.msg_control = (ClUint8T*)gClCmsgHdr;
    msghdr.msg_controllen = gClCmsgHdrLen;
    if(t==1)
    {
      msghdr.msg_iov = (void*)p->payload;
      msghdr.msg_iovlen = p->payloadLength;
      clLogTrace("UDPRELIABLE","SEND", "sendPacket: Send packet with Data length %d ", p->payloadLength);
    }
    else
    {
      clLogTrace("UDPRELIABLE","SEND", "sendPacket: Send control packet with type %d",t);
      userHeader.version = CL_IOC_HEADER_VERSION;
      userHeader.protocolType = 0;
      userHeader.priority = 0;
      userHeader.type = t;
      userHeader.isReliable = 1;
      userHeader.seqno = p->header.seqno;
      userHeader.reserved = htonl(0x2);
      controlMessage.iov_base = (void*)&userHeader;
      controlMessage.iov_len = sizeof(userHeader);
      msghdr.msg_iov = (void*)&controlMessage;
      msghdr.msg_iovlen = 1;
    }
    int length = msghdr.msg_iov->iov_len;
    if(sendmsg((ClInt32T)(long)rsocket, &msghdr, p->flag) < 0)
    {
        clLogError("UDPRELIABLE","SEND", "UDP send failed with error");
    }
    else
    {
        clLogTrace("UDPRELIABLE","SEND", "UDP send [%d] byte successful using socket [%ld] ",length,(long)rsocket);
    }
  }
  if(!is_ack)
  {
    /* Set a timeout event if the packet isn't an ACK */
      clLogTrace("UDPRELIABLE","SEND", "Set time out event for this packet");
//    struct TimeoutArgs *timeargs = malloc(sizeof(struct TimeoutArgs));
//    if(timeargs == NULL)
//    {
//      clLogError("UDPRELIABLE","SEND", "sendPacket: Error allocating timeout args\n");
//      return -1;
//    }
//    timeargs->packet = malloc(sizeof(struct RudpPacket));
//    if(timeargs->packet == NULL)
//    {
//      clLogError("UDPRELIABLE","SEND", "sendPacket: Error allocating timeout args packet\n");
//      return -1;
//    }
//    timeargs->recipient = malloc(sizeof(struct sockaddr_in));
//    if(timeargs->packet == NULL)
//    {
//      clLogError("UDPRELIABLE","SEND", "sendPacket: Error allocating timeout args recipient\n");
//      return -1;
//    }
//    timeargs->fd = rsocket;
//    memcpy(timeargs->packet, p, sizeof(struct RudpPacket));
//    memcpy(timeargs->recipient, recipient, sizeof(struct sockaddr_in));
//
//    struct timeval currentTime;
//    gettimeofday(&currentTime, NULL);
//    struct timeval delay;
//    delay.tv_sec = RUDP_TIMEOUT / 1000;
//    delay.tv_usec = 0;
//    struct timeval timeout_time;
//    timeradd(&currentTime, &delay, &timeout_time);
//
//    struct RudpSocketList *curr_socket = socketListHead;
//    while(curr_socket != NULL)
//    {
//      if(curr_socket->rsock == timeargs->fd)
//      {
//        break;
//      }
//      curr_socket = curr_socket->next;
//    }
//    if(curr_socket->rsock == timeargs->fd)
//    {
//      bool_t session_found = false;
//      /* Check if we already have a session for this peer */
//      struct session *currSession = curr_socket->sessionsListHead;
//      while(currSession != NULL)
//      {
//        if(compareSockaddr(&currSession->address, timeargs->recipient) == 1)
//        {
//          /* Found an existing session */
//          session_found = true;
//          break;
//        }
//        currSession = currSession->next;
//      }
//      if(session_found)
//      {
//        if(timeargs->packet->header.type == RUDP_SYN)
//        {
//          currSession->sender->synTimeout = timeargs;
//        }
//        else if(timeargs->packet->header.type == RUDP_FIN)
//        {
//          currSession->sender->finTimeout = timeargs;
//        }
//        else if(timeargs->packet->header.type == RUDP_DATA)
//        {
//          ClInt32T i;
//          ClInt32T index = 0;
//          for(i = 0; i < RUDP_WINDOW; i++)
//          {
//            if(currSession->sender->slidingWindow[i] != NULL && currSession->sender->slidingWindow[i]->header.seqno == timeargs->packet->header.seqno)
//            {
//              index = i;
//            }
//          }
//          currSession->sender->dataTimeout[index] = timeargs;
//        }
//      }
//    }
//    eventTimeout(timeout_time, timeoutCallback, timeargs, "timeoutCallback");
  }
  return 0;
}

ClInt32T receiveHandlePacketNew(ClInt32T file,ClUint8T *buffer,ClUint32T bufSize,struct sockaddr_in sender)
{
  ClIocHeaderT userHeader = { 0 };
  memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClIocHeaderT));
  if(userHeader.isReliable == 0)
  {
    clLogTrace("UDPRELIABLE","RECV", "Received message in unreliable mode size %d",bufSize);
    return 1;
  }
  else
  {
    clLogTrace("UDPRELIABLE","RECV","Received message inreliable mode with userHeader.type = %d, %d",userHeader.type, bufSize);
  }
  rudpSocketFromUdpSocket(file);
  char type[5];
  short t = userHeader.type;
  if(t == 1)
    strcpy(type, "DATA");
  else if(t == 2)
    strcpy(type, "ACK");
  else if(t == 4)
    strcpy(type, "SYN");
  else if(t == 5)
    strcpy(type, "FIN");
  else
    strcpy(type, "BAD");

  clLogTrace("UDPRELIABLE","RECV","Received %s packet from %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(sender.sin_addr), ntohs(sender.sin_port),
      userHeader.seqno, file);
  /* Locate the correct socket in the socket list */
  if(socketListHead == NULL)
  {
    clLogError("UDPRELIABLE","RECV", "Error: attempt to receive on invalid socket. No sockets in the list\n");
    return -1;
  }
  else
  {
    /* We have sockets to check */
    clOsalMutexLock(&socketListMutex);
    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL)
    {
      if((ClInt32T) (long) curr_socket->rsock == file)
      {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if((ClInt32T) (long) curr_socket->rsock == file)
    {
      /* We found the correct socket, now see if a session already exists for this peer */
      if(curr_socket->sessionsListHead == NULL)
      {
        /* The list is empty, so we check if the sender has initiated the protocol properly (by sending a SYN) */
        if(userHeader.type == RUDP_SYN)
        {
          /* SYN Received. Create a new session at the head of the list */
          clLogTrace("UDPRELIABLE","RECV","Process SYN packet . Create Session and send ACK");
          ClUint32T seqno = userHeader.seqno + 1;
          createReceiverSession(curr_socket, seqno, &sender);
          /* Respond with an ACK */
          struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
          sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
          free(p);
        }
        else
        {
          /* No sessions exist and we got a non-SYN, so ignore it */
        }
      }
      else
      {
        /* Some sessions exist to be checked */
        bool_t session_found = false;
        struct session *currSession = curr_socket->sessionsListHead;
        while(currSession != NULL)
        {
          if(compareSockaddr(&currSession->address, &sender) == 1)
          {
            /* Found an existing session */
            session_found = true;
            break;
          }
          currSession = currSession->next;
        }
        if(session_found == false)
        {
          /* No session was found for this peer */
          if(userHeader.type == RUDP_SYN)
          {
            /* SYN Received. Send an ACK and create a new session */
            clLogTrace("UDPRELIABLE","RECV","Handle SYN packet . Create Session and send ACK");
            ClUint32T seqno = userHeader.seqno + 1;
            createReceiverSession(curr_socket, seqno, &sender);
            struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
            sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
            free(p);
          }
          else
          {
            /* Session does not exist and non-SYN received - ignore it */
          }
        }
        else
        {
          if(userHeader.type == RUDP_SYN)
          {
            clLogTrace("UDPRELIABLE","RECV","Handle SYN packet.");
            if(currSession->receiver == NULL || currSession->receiver->status == OPENING)
            {
              /* Create a new receiver session and ACK the SYN*/
              struct ReceiverSession *newReceiverSession = malloc(sizeof(struct ReceiverSession));
              if(newReceiverSession == NULL)
              {
                clLogTrace("UDPRELIABLE","RECV", "receiveCallback: Error allocating receiver session\n");
                clOsalMutexUnlock(&socketListMutex);
                return -1;
              }
              newReceiverSession->expected_seqno = userHeader.seqno + 1;
              newReceiverSession->status = OPENING;
              newReceiverSession->sessionFinished = false;
              currSession->receiver = newReceiverSession;
              ClUint32T seqno = currSession->receiver->expected_seqno;
              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
              free(p);
            }
            else
            {
              /* Received a SYN when there is already an active receiver session, so we ignore it */
              clOsalMutexUnlock(&socketListMutex);
              return 0;
            }
          }
          if(userHeader.type == RUDP_ACK)
          {
            clLogTrace("UDPRELIABLE","RECV", "handle ACK packet");
            ClUint32T ack_sqn = userHeader.seqno;
            if(currSession->sender->status == SYN_SENT)
            {
               clLogTrace("UDPRELIABLE","RECV", "This an ACK for a SYN. Sender status SYN_SENT");
              /* This an ACK for a SYN */
              ClUint32T syn_sqn = currSession->sender->seqno;
              if((ack_sqn - 1) == syn_sqn)
              {
                 clLogTrace("UDPRELIABLE","RECV","Update sender status to OPEN");
                /* Delete the retransmission timeout */
//                eventTimeoutDelete(timeoutCallback, currSession->sender->synTimeout);
//                struct TimeoutArgs *t = (struct TimeoutArgs *) currSession->sender->synTimeout;
//                free(t->packet);
//                free(t->recipient);
//                free(t);
                currSession->sender->status = OPEN;
                while(currSession->sender->dataQueue != NULL)
                {
                  /* Check if the window is already full */
                  clLogTrace("UDPRELIABLE","RECV","Data queue is not empty. Check if the window is already full");
                  if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
                  {
                    clLogTrace("UDPRELIABLE","RECV","the window is already full . Exit");
                    break;
                  }
                  else
                  {
                    ClInt32T index;
                    ClInt32T i;
                    /* Find the first unused window slot */
                    for(i = RUDP_WINDOW - 1; i >= 0; i--)
                    {
                      if(currSession->sender->slidingWindow[i] == NULL)
                      {
                        index = i;
                      }
                    }
                    clLogTrace("UDPRELIABLE","RECV","Send packet, add to window and remove from queue");
                    /* Send packet, add to window and remove from queue */
                    ClUint32T seqno = ++syn_sqn;
                    ClInt32T len = currSession->sender->dataQueue->len;
                    char *payload = currSession->sender->dataQueue->item;
                    struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, len, payload,0);
                    currSession->sender->seqno += 1;
                    currSession->sender->slidingWindow[index] = datap;
                    currSession->sender->retransmissionAttempts[index] = 0;
                    struct data *temp = currSession->sender->dataQueue;
                    currSession->sender->dataQueue = currSession->sender->dataQueue->next;
                    free(temp->item);
                    free(temp);
                    sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
                    clLogTrace("UDPRELIABLE","RECV","Send packet done");

                  }
                }
              }
            }
            else if(currSession->sender->status == OPEN)
            {
              clLogTrace("UDPRELIABLE","RECV","This is ACK for Data");
              if(currSession->sender->slidingWindow[0] != NULL)
              {
                if(currSession->sender->slidingWindow[0]->header.seqno == (userHeader.seqno - 1))
                {
                  /* Correct ACK received. Remove the first window item and shift the rest left */
                  clLogTrace("UDPRELIABLE","RECV","This is ACK for Data for seqno [%d]",userHeader.seqno);
//                  eventTimeoutDelete(timeoutCallback, currSession->sender->dataTimeout[0]);
//                  struct TimeoutArgs *args = (struct TimeoutArgs *) currSession->sender->dataTimeout[0];
//                  clLogDebug("IOC", "RELIABLE", "free package [%d]",args->packet->header.seqno);
//                  free(args->packet);
//                  free(args->recipient);
//                  free(args);
//                  free(currSession->sender->slidingWindow[0]);
                  ClInt32T i;
                  if(RUDP_WINDOW == 1)
                  {
                    currSession->sender->slidingWindow[0] = NULL;
                    currSession->sender->retransmissionAttempts[0] = 0;
                    currSession->sender->dataTimeout[0] = NULL;
                  }
                  else
                  {
                    clLogTrace("UDPRELIABLE","RECV","remove packet with seqno [%d] in slidingWindow",userHeader.seqno);
                    for(i = 0; i < RUDP_WINDOW - 1; i++)
                    {
                      currSession->sender->slidingWindow[i] = currSession->sender->slidingWindow[i + 1];
                      currSession->sender->retransmissionAttempts[i] = currSession->sender->retransmissionAttempts[i + 1];
                      currSession->sender->dataTimeout[i] = currSession->sender->dataTimeout[i + 1];
                      if(i == RUDP_WINDOW - 2)
                      {
                        currSession->sender->slidingWindow[i + 1] = NULL;
                        currSession->sender->retransmissionAttempts[i + 1] = 0;
                        currSession->sender->dataTimeout[i + 1] = NULL;
                      }
                    }
                  }
                  while(currSession->sender->dataQueue != NULL)
                  {
                    if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
                    {
                      break;
                    }
                    else
                    {
                      ClInt32T index;
                      ClInt32T i;
                      /* Find the first unused window slot */
                      for(i = RUDP_WINDOW - 1; i >= 0; i--)
                      {
                        if(currSession->sender->slidingWindow[i] == NULL)
                        {
                          index = i;
                        }
                      }
                      /* Send packet, add to window and remove from queue */
                      currSession->sender->seqno = currSession->sender->seqno + 1;
                      ClUint32T seqno = currSession->sender->seqno;
                      ClInt32T len = currSession->sender->dataQueue->len;
                      char *payload = currSession->sender->dataQueue->item;
                      struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, len, payload,0);
                      currSession->sender->slidingWindow[index] = datap;
                      currSession->sender->retransmissionAttempts[index] = 0;
                      struct data *temp = currSession->sender->dataQueue;
                      currSession->sender->dataQueue = currSession->sender->dataQueue->next;
                      free(temp->item);
                      free(temp);
                      sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
                    }
                  }
                  if(curr_socket->closeRequested)
                  {
                    /* Can the socket be closed? */
                    struct session *headSessions = curr_socket->sessionsListHead;
                    while(headSessions != NULL)
                    {
                      if(headSessions->sender->sessionFinished == false)
                      {
                        if(headSessions->sender->dataQueue == NULL && headSessions->sender->slidingWindow[0] == NULL && headSessions->sender->status == OPEN)
                        {
                          headSessions->sender->seqno += 1;
                          struct RudpPacket *p = createRudpPacketNew(RUDP_FIN, headSessions->sender->seqno, 0, NULL,0);
                          sendPacketNew(false, (rudpSocketT) (long) file, p, &headSessions->address);
                          free(p);
                          headSessions->sender->status = FIN_SENT;
                        }
                      }
                      headSessions = headSessions->next;
                    }
                  }
                }
                else
                {
                  clLogTrace("UDPRELIABLE","RECV","This is ACK for Data for seqno [%d] . Invalid ACK",userHeader.seqno);
                }
              }
            }
            else if(currSession->sender->status == FIN_SENT)
            {
              /* Handle ACK for FIN */
              if((currSession->sender->seqno + 1) == userHeader.seqno)
              {
//                eventTimeoutDelete(timeoutCallback, currSession->sender->finTimeout);
//                struct TimeoutArgs *t = currSession->sender->finTimeout;
//                free(t->packet);
//                free(t->recipient);
//                free(t);
                currSession->sender->sessionFinished = true;
                if(curr_socket->closeRequested)
                {
                  /* See if we can close the socket */
                  struct session *headSessions = curr_socket->sessionsListHead;
                  bool_t all_done = true;
                  while(headSessions != NULL)
                  {
                    if(headSessions->sender->sessionFinished == false)
                    {
                      all_done = false;
                    }
                    else if(headSessions->receiver != NULL && headSessions->receiver->sessionFinished == false)
                    {
                      all_done = false;
                    }
                    else
                    {
                      free(headSessions->sender);
                      if(headSessions->receiver)
                      {
                        free(headSessions->receiver);
                      }
                    }
                    struct session *temp = headSessions;
                    headSessions = headSessions->next;
                    free(temp);
                  }
                  if(all_done)
                  {
                    if(curr_socket->handler != NULL)
                    {
                      curr_socket->handler((rudpSocketT) (long) file, RUDP_EVENT_CLOSED, &sender);
                      close(file);
                      free(curr_socket);
                    }
                  }
                }
              }
              else
              {
                /* Received incorrect ACK for FIN - ignore it */
              }
            }
          }
          else if(userHeader.type == RUDP_DATA)
          {
            /* Handle DATA packet. If the receiver is OPENING, it can transition to OPEN */
            clLogTrace("UDPRELIABLE","RECV","This is DATA packet");
            if(currSession->receiver->status == OPENING)
            {
              if(userHeader.seqno == currSession->receiver->expected_seqno)
              {
                currSession->receiver->status = OPEN;
                clLogTrace("UDPRELIABLE","RECV","receive status is OPENING. Update receive status");
              }
              else
              {
                  clLogTrace("UDPRELIABLE","RECV","receive status is OPENING. Invalid seqno");
              }
            }

            if(userHeader.seqno == currSession->receiver->expected_seqno)
            {
              /* Sequence numbers match - ACK the data */
              clLogTrace("UDPRELIABLE","RECV","receive status is OPENING. Update receive status");
              ClUint32T seqno = userHeader.seqno + 1;
              currSession->receiver->expected_seqno = seqno;
              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
              free(p);
              /* Pass the data up to the application */
              clOsalMutexUnlock(&socketListMutex);
              return 1;
            }
            /* Handle the case where an ACK was lost */
            else if(SEQ_GEQ(userHeader.seqno, (currSession->receiver->expected_seqno - RUDP_WINDOW))
                && SEQ_LT(userHeader.seqno, currSession->receiver->expected_seqno))
            {
              ClUint32T seqno = userHeader.seqno + 1;
              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
              free(p);
            }
          }
          else if(userHeader.type == RUDP_FIN)
          {
            if(currSession->receiver->status == OPEN)
            {
              if(userHeader.seqno == currSession->receiver->expected_seqno)
              {
                /* If the FIN is correct, we can ACK it */
                ClUint32T seqno = currSession->receiver->expected_seqno + 1;
                struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
                sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
                free(p);
                currSession->receiver->sessionFinished = true;

                if(curr_socket->closeRequested)
                {
                  /* Can we close the socket now? */
                  struct session *headSessions = curr_socket->sessionsListHead;
                  ClInt32T all_done = true;
                  while(headSessions != NULL)
                  {
                    if(headSessions->sender->sessionFinished == false)
                    {
                      all_done = false;
                    }
                    else if(headSessions->receiver != NULL && headSessions->receiver->sessionFinished == false)
                    {
                      all_done = false;
                    }
                    else
                    {
                      free(headSessions->sender);
                      if(headSessions->receiver)
                      {
                        free(headSessions->receiver);
                      }
                    }
                    struct session *temp = headSessions;
                    headSessions = headSessions->next;
                    free(temp);
                  }
                  if(all_done)
                  {
                    if(curr_socket->handler != NULL)
                    {
                      curr_socket->handler((rudpSocketT) (long) file, RUDP_EVENT_CLOSED, &sender);
                      close(file);
                      free(curr_socket);
                    }
                  }
                }
              }
              else
              {
                /* FIN received with incorrect sequence number - ignore it */
              }
            }
          }
        }
      }
    }
  }
  clOsalMutexUnlock(&socketListMutex);
  return 0;
}
