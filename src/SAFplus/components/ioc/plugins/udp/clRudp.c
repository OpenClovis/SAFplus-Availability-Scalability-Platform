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

ClRcT
clCheckRetransCallback(void *pData)
{
  ClRcT 		 rc		    = CL_OK;
  struct session * currentSession=(struct session*)pData;
  clOsalMutexLock(&currentSession->senderMutex);
  if(currentSession->sender->slidingWindow[0]!=NULL)
  {
    int index;
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    for(index = 0; index < RUDP_WINDOW; index++)
    {
      long sec = ((currentTime.tv_sec * 1000000 + currentTime.tv_usec)- (currentSession->sender->slidingWindow[index]->sendTime.tv_sec * 1000000 + currentSession->sender->slidingWindow[index]->sendTime.tv_usec))/1000;
      if(sec<RUDP_TIMEOUT)
      {
	clLogTrace("UDPRELIABLE", "SEND","sec  : [%ld]",sec);
	clOsalMutexUnlock(&currentSession->senderMutex);
	return CL_OK;
      }
      if(currentSession->sender->retransmissionAttempts[index] >= RUDP_MAXRETRANS)
      {
	//TODO handle socket failue
	clLogTrace("UDPRELIABLE", "SEND","retransmission Exceed RUDP_MAXRETRANS");
      }
      else
      {
	clLogTrace("UDPRELIABLE", "SEND","retransmission packet seqno [%d] timeout [%ld] to dest[%s:%d] using socket [%d]",currentSession->sender->slidingWindow[index]->header.seqno,sec,inet_ntoa(currentSession->address.sin_addr), ntohs(currentSession->address.sin_port),currentSession->socketfd);
	currentSession->sender->retransmissionAttempts[index]++;
	currentSession->sender->slidingWindow[index]->sendTime.tv_sec=currentTime.tv_sec;
	currentSession->sender->slidingWindow[index]->sendTime.tv_usec=currentTime.tv_usec;
	sendPacketNew(false, (rudpSocketT)(long)currentSession->socketfd, currentSession->sender->slidingWindow[index], &currentSession->address);
      }
    }
  }
  else
  {
    clLogTrace("UDPRELIABLE", "SEND", "retransmission on fd [%d] : no packet in sliding windows",currentSession->socketfd);
  }
  clOsalMutexUnlock(&currentSession->senderMutex);
  return rc;
}


/* Creates a new sender session and appends it to the socket's session list */
void createSenderSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *to, struct data **dataQueue)
{
  //clLogTrace("UDPRELIABLE", "SEND", "create sender session");
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL)
  {
    clLogError("UDPRELIABLE", "SEND", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSession->address = *to;
  newSession->next = NULL;
  newSession->receiver = NULL;
  newSession->socketfd = socket->socketfd;
  clOsalMutexInit(&newSession->senderMutex);
  struct SenderSession * newSenderSession = malloc(sizeof(struct SenderSession));
  if(newSenderSession == NULL)
  {
    clLogError("UDPRELIABLE", "SEND", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSenderSession->status = SYN_SENT;
  newSenderSession->seqno = seqno;
  newSenderSession->dataQueueCount=0;
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
  newSenderSession->gRetransTimer=0;
  ClTimerTimeOutT  timeout = {0, 500};
  ClRcT rc = CL_OK;
  clLogTrace("UDPRELIABLE", "SEND", "Create and start timer");
  rc= clTimerCreateAndStart(timeout, CL_TIMER_REPETITIVE,
			     CL_TIMER_SEPARATE_CONTEXT,
			     clCheckRetransCallback,
			     (void*)newSession, &newSenderSession->gRetransTimer);
  if(rc != CL_OK)
  {
      clLogError("UDPRELIABLE", "SEND", "Failed to create a timer for session. error code [0x%x].",rc);
  }

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
  //clLogTrace("UDPRELIABLE", "RECV","Create receive sesion for socket");
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
  //	clLogDebug("IOC", "RELIABLE", "createRudpPacket: Error allocating memory for packet\n");
  //	return NULL;
  //  }
  //  packet->header = header;
  //  packet->flag=flag;
  //  packet->payloadLength = len;
  //  memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
  //  if(payload != NULL)
  //	memcpy(&packet->payload, payload, len);
  //
  //  return packet;
  //}

struct RudpPacket *createRudpPacketNew(ClUint16T type, ClUint32T seqno, ClInt32T len, char *payload,ClInt32T flag)
{
  struct rudp_hdr header;
  header.version = RUDP_VERSION;
  header.type = type;
  header.seqno = seqno;
  struct RudpPacket *packet = malloc(sizeof(struct RudpPacket));
  ClIocHeaderT* iocHeader=NULL;
  if(packet == NULL)
  {
    clLogError("UDPRELIABLE","PACKET", "createRudpPacket: Error allocating memory for packet\n");
    return NULL;
  }
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  packet->sendTime.tv_sec=currentTime.tv_sec;
  packet->sendTime.tv_usec=currentTime.tv_usec;
  packet->payloadLength=len;
  struct iovec *iov=NULL;
  if(type==1)
  {
    memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
    if(payload != NULL)
    {
      memcpy(&packet->payload, payload, len);
    }
    else
    {
      clLogTrace("UDPRELIABLE","PACKET","createRudpPacketNew : Data is NULL");
      return NULL;
    }
    iov =(struct iovec *)packet->payload;
    iocHeader = (ClIocHeaderT*)iov->iov_base;
    int length= iov->iov_len;
    iocHeader->seqno=seqno;
    iocHeader->type=1;
    //clLogTrace("UDPRELIABLE","PACKET","Create DATA packet size [%d] for seqno [%d] done",length,iocHeader->seqno);
    packet->payloadLength = length;
  }
  else
  {
      //clLogTrace("UDPRELIABLE", "PACKET","Create control packet \n");
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
//	if(curr->socketfd==sockfd)
//	{
//	  clOsalMutexUnlock(&socketListMutex);
//	  return CL_TRUE;
//	}
//    }
//  }
//  clOsalMutexUnlock(&socketListMutex);
//  return false;
//}

int rudpSocketFromUdpSocket(int sockfd)
{
  //clLogTrace("UDPRELIABLE", "SOCKET", "Create reliable UDP socket from UDP socket\n");
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
  struct RudpSocketList *newSocket;
  if(socketListHead == NULL)
  {
    newSocket= malloc(sizeof(struct RudpSocketList));
    //clLogTrace("UDPRELIABLE", "SOCKET", "socketListHead is null");
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    if(newSocket == NULL)
    {
      clLogError("UDPRELIABLE", "SOCKET", "rudpSocket: Error allocating memory for socket list");
      clOsalMutexUnlock(&socketListMutex);
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
    //clLogTrace("UDPRELIABLE", "SOCKET", "socketListHead is not null");
    struct RudpSocketList *currentSocket = socketListHead;
    //clLogTrace("UDPRELIABLE", "SOCKET", "check the first socket [%d]",currentSocket->socketfd);
    if(currentSocket->socketfd==sockfd)
    {
      clOsalMutexUnlock(&socketListMutex);
      //clLogTrace("UDPRELIABLE", "SOCKET", "Socket is already exists 1. Return...");
      return 1;
    }
    while(currentSocket->next != NULL)
    {
      clLogTrace("UDPRELIABLE", "SOCKET", "check socket [%d]...",currentSocket->socketfd);
      if(currentSocket->socketfd==sockfd)
      {
	clOsalMutexUnlock(&socketListMutex);
	clLogTrace("UDPRELIABLE", "SOCKET", "Socket is already exists . Return...");
	return 1;
      }
      currentSocket = currentSocket->next;
    }
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    newSocket= malloc(sizeof(struct RudpSocketList));
    if(newSocket == NULL)
    {
      clOsalMutexUnlock(&socketListMutex);
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
  //clLogTrace("UDPRELIABLE", "SOCKET", "Create reliable UDP socket from UDP socket successful with socket [%d]",newSocket->socketfd);
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
  //clLogTrace("UDPRELIABLE","SEND","Sending reliable paket with length [%d]...",len);
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
  if(data==NULL)
  {
      clLogError("UDPRELIABLE","SEND", "rudpSendto Error: Data is null\n");
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
	clLogTrace("UDPRELIABLE","SEND", "No session in current socket. Create sender session. Add data to data queue");
	seqno = rand();
	createSenderSession(curr_socket, seqno, to, &data_item);
      }
      else
      {
	bool_t session_found = false;
	struct session *currSession = curr_socket->sessionsListHead;
	while(currSession != NULL)
	{
	  //clLogTrace("UDPRELIABLE","SEND", "Finding session for destination address...");
	  if(compareSockaddr(&currSession->address, to) == 1)
	  {
	    clOsalMutexLock(&currSession->senderMutex);
	    //clLogTrace("UDPRELIABLE","SEND", "lock send");
	    bool_t data_is_queued = false;
	    bool_t we_must_queue = true;
	    if(currSession->sender == NULL)
	    {
	      clLogTrace("UDPRELIABLE","SEND", "Sender session is null. Send a sync paket ");
	      seqno = rand();
	      createSenderSession(curr_socket, seqno, to, &data_item);
	      struct RudpPacket *p = createRudpPacketNew(RUDP_SYN, seqno, 0, NULL ,0);
	      sendPacketNew(false, rsocket, p, to);
	      free(p);
	      newSessionCreated = false; /* Dont send the SYN twice */
	      break;
	    }
	    if(currSession->sender->dataQueue != NULL)
	    {
	      //clLogTrace("UDPRELIABLE","SEND", "Data is queued in sender session");
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
		  //clLogDebug("IOC", "RELIABLE", "Debug 8\n");
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
	      clLogTrace("UDPRELIABLE","SEND", "Don't send the packet. Add to data queue [%d]",currSession->sender->dataQueueCount);
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
	      currSession->sender->dataQueueCount++;
	      if(currSession->sender->slidingWindow[0] == NULL)
	      {
		  clLogTrace("UDPRELIABLE","SEND", "Fix don't send");
		  currSession->sender->seqno = currSession->sender->seqno + 1;
		  ClUint32T seqno = currSession->sender->seqno;
		  ClInt32T len = currSession->sender->dataQueue->len;
		  char *payload = currSession->sender->dataQueue->item;
		  struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, len, payload,0);
		  currSession->sender->slidingWindow[0] = datap;
		  currSession->sender->retransmissionAttempts[0] = 0;
		  struct data *temp = currSession->sender->dataQueue;
		  currSession->sender->dataQueue = currSession->sender->dataQueue->next;
		  free(temp->item);
		  free(temp);
		  sendPacketNew(false, rsocket, datap,&currSession->address);
	      }
	      //clLogTrace("UDPRELIABLE","SEND", "Packet is added to data queue");
	    }
	    session_found = true;
	    newSessionCreated = false;
	    clOsalMutexUnlock(&currSession->senderMutex);
	    if(we_must_queue == true)
	    {
		do
		{
		  usleep(20000);
		}while(currSession->sender->dataQueueCount>10);
		clLogTrace("UDPRELIABLE","SEND", "queue count [%d]",currSession->sender->dataQueueCount++);
	    }
	    //clLogTrace("UDPRELIABLE","SEND", "unlock send");
	    break;
	  }
	  currSession = currSession->next;
	}
	if(!session_found)
	{
	  seqno = rand();
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
  if(p==NULL)
  {
      clLogTrace("UDPRELIABLE","SEND", "Packet is NULL\n");

  }
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

  if(DROP != 0 && p->header.seqno % DROP == 0)
  {
    clLogError("UDPRELIABLE","SEND", "Dropped seqno %d\n",p->header.seqno);
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
      msghdr.msg_iovlen = 1;
      //clLogTrace("UDPRELIABLE","SEND", "sendPacket: Send packet with Data length %d ", p->payloadLength);
      ClIocHeaderT* iocHeader=NULL;
      struct iovec *iov=NULL;
      iov =(struct iovec *)msghdr.msg_iov;
      iocHeader = (ClIocHeaderT*)iov->iov_base;
      clLogDebug("UDPRELIABLE","SEND", "Sending %s packet to %s:%d seq number=%u on socket=%d\n validation info (%d %d )", type, inet_ntoa(recipient->sin_addr), ntohs(recipient->sin_port),
	  p->header.seqno, (ClInt32T) (long) rsocket,iocHeader->seqno,iocHeader->type);
    }
    else
    {
      //clLogTrace("UDPRELIABLE","SEND", "sendPacket: Send control packet with type %d",t);
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
    if(sendmsg((ClInt32T)(long)rsocket, &msghdr, p->flag) < 0)
    {
	clLogError("UDPRELIABLE","SEND", "UDP send failed ");
    }
    else
    {
	clLogTrace("UDPRELIABLE","SEND", "UDP send using socket ... [%ld] ",(long)rsocket);
    }
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
    clLogTrace("UDPRELIABLE","RECV","Received message inreliable mode with userHeader.type = [%d], [%d] length = %d",userHeader.type, userHeader.seqno, bufSize);
    if(userHeader.type==4)
    {
      clLogTrace("UDPRELIABLE","RECV","Received message SYN inreliable mode. create socket ");
      rudpSocketFromUdpSocket(file);
    }
  }
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
	  clOsalMutexLock(&currSession->senderMutex);
	  //clLogTrace("UDPRELIABLE","SEND", "lock receive");
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
	      clOsalMutexUnlock(&currSession->senderMutex);
	      clLogTrace("UDPRELIABLE","SEND", "unlock receive");
	      return 0;
	    }
	  }
	  if(userHeader.type == RUDP_ACK)
	  {
	    //clLogTrace("UDPRELIABLE","RECV", "handle ACK packet");
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
		currSession->sender->status = OPEN;
		while(currSession->sender->dataQueue != NULL)
		{
		  /* Check if the window is already full */
		  //clLogTrace("UDPRELIABLE","RECV","Data queue is not empty. Check if the window is already full");
		  if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
		  {
		    clLogTrace("UDPRELIABLE","RECV","the window is already full . Exit");
		    break;
		  }
		  else
		  {
		    ClInt32T index=0;
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
		    struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, currSession->sender->dataQueue->len, currSession->sender->dataQueue->item,0);
		    currSession->sender->seqno += 1;
		    currSession->sender->slidingWindow[index] = datap;
		    currSession->sender->retransmissionAttempts[index] = 0;
		    struct data *temp = currSession->sender->dataQueue;
		    currSession->sender->dataQueue = currSession->sender->dataQueue->next;
		    free(temp->item);
		    free(temp);
		    currSession->sender->dataQueueCount--;
		    sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
		  }
		}
	      }
	    }
	    else if(currSession->sender->status == OPEN)
	    {
	      //clLogTrace("UDPRELIABLE","RECV","This is ACK for Data");
	      if(currSession->sender->slidingWindow[0] != NULL)
	      {
		clLogTrace("UDPRELIABLE","RECV","This is ACK for Data for seqno [%d] [%d]",userHeader.seqno-1,currSession->sender->slidingWindow[0]->header.seqno);
		if(currSession->sender->slidingWindow[0]->header.seqno <= (userHeader.seqno - 1))
		{
		  while(currSession->sender->slidingWindow[0]->header.seqno <= (userHeader.seqno - 1))
		  {
		  /* Correct ACK received. Remove the first window item and shift the rest left */
		    clLogTrace("UDPRELIABLE","RECV","remove packet with seqno [%d] in slidingWindow",currSession->sender->slidingWindow[0]->header.seqno);
		    free(currSession->sender->slidingWindow[0]);
		    ClInt32T i;
		    if(RUDP_WINDOW == 1)
		    {
		      currSession->sender->slidingWindow[0] = NULL;
		      currSession->sender->retransmissionAttempts[0] = 0;
		      currSession->sender->dataTimeout[0] = NULL;
		    }
		    else
		    {
		      //clLogTrace("UDPRELIABLE","RECV","remove packet with seqno [%d] in slidingWindow",userHeader.seqno);
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
		    if(currSession->sender->slidingWindow[0]==NULL)
		    {
			break;
		    }
		  }
		  while(currSession->sender->dataQueue != NULL)
		  {
		    if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
		    {
		      clLogTrace("UDPRELIABLE","RECV","sliding windows is full break");
		      break;
		    }
		    else
		    {
		      ClInt32T index=0;
		      ClInt32T i;
		      /* Find the first unused window slot */
		      for(i = RUDP_WINDOW - 1; i >= 0; i--)
		      {
			if(currSession->sender->slidingWindow[i] == NULL)
			{
			  index = i;
			}
		      }
		      clLogTrace("UDPRELIABLE","RECV","sliding windows is not full. Sending package in queue at windows [%d]",index);
		      /* Send packet, add to window and remove from queue */
		      currSession->sender->seqno = currSession->sender->seqno + 1;
		      ClUint32T seqno = currSession->sender->seqno;
		      struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, currSession->sender->dataQueue->len, currSession->sender->dataQueue->item,0);
		      sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
		      currSession->sender->slidingWindow[index] = datap;
		      currSession->sender->retransmissionAttempts[index] = 0;
		      struct data *temp = currSession->sender->dataQueue;
		      currSession->sender->dataQueue = currSession->sender->dataQueue->next;
		      free(temp->item);
		      free(temp);
		      currSession->sender->dataQueueCount--;
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
		  clLogTrace("UDPRELIABLE","RECV","This is ACK for Data for seqno [%d] [%d]. Invalid ACK. ignore",userHeader.seqno,currSession->sender->slidingWindow[0]->header.seqno);
		}
	      }
	    }
	    else if(currSession->sender->status == FIN_SENT)
	    {
	      /* Handle ACK for FIN */
	      if((currSession->sender->seqno + 1) == userHeader.seqno)
	      {
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
	      ClUint32T seqno = userHeader.seqno + 1;
	      currSession->receiver->expected_seqno = seqno;
	      struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
	      sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
	      free(p);
	      /* Pass the data up to the application */
	      return 1;
	    }
	    /* Handle the case where an ACK was lost */
	    else if(SEQ_GEQ(userHeader.seqno, (currSession->receiver->expected_seqno - RUDP_WINDOW))
		&& SEQ_LT(userHeader.seqno, currSession->receiver->expected_seqno))
	    {
	      clLogTrace("UDPRELIABLE","RECV","Handle ACK loss [%d]",userHeader.seqno);
	      ClUint32T seqno = userHeader.seqno + 1;
	      struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
	      sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
	      free(p);
	    }
	    //clOsalMutexUnlock(&currSession->receiver->receiverMutex);
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
	  clOsalMutexUnlock(&currSession->senderMutex);
	  clLogTrace("UDPRELIABLE","SEND", "unlock receiver");
	}
      }
    }
  }
  return 0;
}
