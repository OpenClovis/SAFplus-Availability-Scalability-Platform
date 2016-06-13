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
//static struct EventData *eData = NULL;
static struct EventData *eventDataTimers = NULL;

ClInt32T eventTimeout(struct timeval t, ClInt32T(*fn)( ClInt32T, void*), void *arg, char *str)
{
  struct EventData *e, *e1, **e_prev;
  e = (struct EventData *) malloc(sizeof(struct EventData));
  if(e == NULL)
  {
    perror("eventTimeout: malloc");
    return -1;
  }
  memset(e, 0, sizeof(struct EventData));
  strcpy(e->e_string, str);
  e->e_fn = fn;
  e->e_arg = arg;
  e->e_type = EVENT_TIME;
  e->e_time = t;
  e_prev = &eventDataTimers;
  for(e1 = eventDataTimers; e1; e1 = e1->e_next)
  {
    if(timercmp(&e->e_time, &e1->e_time, <))
      break;
    e_prev = &e1->e_next;
  }
  e->e_next = e1;
  *e_prev = e;
  return 0;
}
static ClInt32T eventDelete(struct EventData **firstp, ClInt32T (*fn)(ClInt32T, void*), void *arg)
{
  struct EventData *e, **e_prev;
  e_prev = firstp;
  for (e = *firstp; e; e = e->e_next) {
    if (fn == e->e_fn && arg == e->e_arg) {
      *e_prev = e->e_next;
      free(e);
      return 0;
    }
    e_prev = &e->e_next;
  }
  return -1;
}
/*
 * Deregister a rudp event.
 */
ClInt32T eventTimeoutDelete(ClInt32T(*fn)( ClInt32T, void*), void *arg)
{
  return eventDelete(&eventDataTimers, fn, arg);
}

/*
 * Rudp event loop.
 * Dispatch file descriptor events (and timeouts) by invoking callbacks.
 */
ClInt32T eventLoop()
{
  struct EventData *e;
  fd_set fdset;
  ClInt32T n;
  struct timeval t, t0;

  while(eventDataTimers)
  {
    FD_ZERO(&fdset);
    if(eventDataTimers)
    {
      gettimeofday(&t0, NULL);
      timersub(&eventDataTimers->e_time, &t0, &t);
      if(t.tv_sec < 0)
        n = 0;
      else
        n = select(FD_SETSIZE, &fdset, NULL, NULL, &t);
    }
    else
      n = select(FD_SETSIZE, &fdset, NULL, NULL, NULL);

    if(n == -1)
      if(errno != EINTR)
        perror("eventLoop: select");
    if(n == 0)
    { /* Timeout */
      e = eventDataTimers;
      eventDataTimers = eventDataTimers->e_next;
      clLogDebug("IOC", "RELIABLE", "eventLoop: timeout : %s[arg: %x]\n",
          e->e_string, (ClInt32T)(long)e->e_arg);
      if((*e->e_fn)(0, e->e_arg) < 0)
      {
        return -1;
      }
      switch(e->e_type)
      {
        case EVENT_TIME:
          free(e);
          break;
        default:
          clLogDebug("IOC", "RELIABLE", "eventLoop: illegal e_type:%d\n", e->e_type);
      }
      continue;
    }
  }
  clLogDebug("IOC", "RELIABLE", "eventLoop: returning 0\n");
  return 0;
}

/* Global variables */
bool_t rngSeeded = false;
struct RudpSocketList *socketListHead = NULL;
ClOsalMutexT socketListMutex;


/* Creates a new sender session and appends it to the socket's session list */
void createSenderSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *to, struct data **dataQueue)
{
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSession->address = *to;
  newSession->next = NULL;
  newSession->receiver = NULL;

  struct SenderSession *newSenderSession = malloc(sizeof(struct SenderSession));
  if(newSenderSession == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSenderSession->status = SYN_SENT;
  newSenderSession->seqno = seqno;
  newSenderSession->sessionFinished = false;
  /* Add data to the new session's queue */
  newSenderSession->dataQueue = *dataQueue;
  newSession->sender = newSenderSession;

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
}

/* Creates a new receiver session and appends it to the socket's session list */
void createReceiverSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *addr)
{
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "createReceiverSession: Error allocating memory\n");
    return;
  }
  newSession->address = *addr;
  newSession->next = NULL;
  newSession->sender = NULL;

  struct ReceiverSession *newReceiverSession = malloc(sizeof(struct ReceiverSession));
  if(newReceiverSession == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "createReceiverSession: Error allocating memory\n");
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
  struct rudp_hdr header;
  header.version = RUDP_VERSION;
  header.type = type;
  header.seqno = seqno;
  struct RudpPacket *packet = malloc(sizeof(struct RudpPacket));
  if(packet == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "createRudpPacket: Error allocating memory for packet\n");
    return NULL;
  }
  struct iovec *iov =(struct iovec *)payload;
  ClIocHeaderT* iocHeader = (ClIocHeaderT*)iov->iov_base;
  iocHeader->seqno=seqno;
  iocHeader->type=type;
  clLogDebug("TIPC", "SEND", "check value : seqno = %d\n",iocHeader->seqno);
  packet->header = header;
  packet->flag=flag;
  packet->payloadLength = len;
  memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
  if(payload != NULL)
    memcpy(&packet->payload, payload, len);
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

void rudpSocketFromUdpSocket(int sockfd)
{
  if(rngSeeded == false)
  {
    srand(time(NULL));
    rngSeeded = true;
  }
  if(sockfd < 0)
  {
    return;
  }
  clOsalMutexLock(&socketListMutex);

  if(socketListHead == NULL)
  {
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    struct RudpSocketList *newSocket = malloc(sizeof(struct RudpSocketList));
    if(newSocket == NULL)
    {
      clLogDebug("IOC", "RELIABLE", "rudpSocket: Error allocating memory for socket list\n");
      return;
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
        clLogDebug("IOC", "RELIABLE", "create rudpSocket: socket is already exists \n");
        return;
      }
      currentSocket = currentSocket->next;
    }
    rudpSocketT socket = (rudpSocketT) (long) sockfd;
    /* Create new socket and add to list of sockets */
    struct RudpSocketList *newSocket = malloc(sizeof(struct RudpSocketList));
    if(newSocket == NULL)
    {
      clLogDebug("IOC", "RELIABLE", "rudpSocket: Error allocating memory for socket list\n");
      return;
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
  return;
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

//ClInt32T receiveHandlePacket(ClInt32T file,struct RudpPacket* receivedPacket,struct sockaddr_in sender)
//{
//  if(receivedPacket == NULL)
//  {
//    clLogDebug("IOC", "RELIABLE", "receiveCallback: Error allocating packet\n");
//    return -1;
//  }
//  struct rudp_hdr rudpheader = receivedPacket->header;
//  char type[5];
//  short t = rudpheader.type;
//  if(t == 1)
//    strcpy(type, "DATA");
//  else if(t == 2)
//    strcpy(type, "ACK");
//  else if(t == 4)
//    strcpy(type, "SYN");
//  else if(t == 5)
//    strcpy(type, "FIN");
//  else
//    strcpy(type, "BAD");
//
//  clLogDebug("IOC", "RELIABLE", "Received %s packet from %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(sender.sin_addr), ntohs(sender.sin_port),
//      rudpheader.seqno, file);
//
//  /* Locate the correct socket in the socket list */
//  if(socketListHead == NULL)
//  {
//    clLogDebug("IOC", "RELIABLE", "Error: attempt to receive on invalid socket. No sockets in the list\n");
//    return -1;
//  }
//  else
//  {
//    /* We have sockets to check */
//    struct RudpSocketList *curr_socket = socketListHead;
//    while(curr_socket != NULL)
//    {
//      if(curr_socket->socketfd == file)
//      {
//        break;
//      }
//      curr_socket = curr_socket->next;
//    }
//    if(curr_socket->socketfd == file)
//    {
//      /* We found the correct socket, now see if a session already exists for this peer */
//      if(curr_socket->sessionsListHead == NULL)
//      {
//        /* The list is empty, so we check if the sender has initiated the protocol properly (by sending a SYN) */
//        if(rudpheader.type == RUDP_SYN)
//        {
//          /* SYN Received. Create a new session at the head of the list */
//          ClUint32T seqno = rudpheader.seqno + 1;
//          createReceiverSession(curr_socket, seqno, &sender);
//          /* Respond with an ACK */
//          struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//          sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//          free(p);
//        }
//        else
//        {
//          /* No sessions exist and we got a non-SYN, so ignore it */
//        }
//      }
//      else
//      {
//        /* Some sessions exist to be checked */
//        bool_t session_found = false;
//        struct session *currSession = curr_socket->sessionsListHead;
//        //        struct session *last_session;
//        while(currSession != NULL)
//        {
//          //          if(currSession->next == NULL) {
//          //            last_session = currSession;
//          //          }
//          if(compareSockaddr(&currSession->address, &sender) == 1)
//          {
//            /* Found an existing session */
//            session_found = true;
//            break;
//          }
//
//          currSession = currSession->next;
//        }
//        if(session_found == false)
//        {
//          /* No session was found for this peer */
//          if(rudpheader.type == RUDP_SYN)
//          {
//            /* SYN Received. Send an ACK and create a new session */
//            ClUint32T seqno = rudpheader.seqno + 1;
//            createReceiverSession(curr_socket, seqno, &sender);
//            struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//            sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//            free(p);
//          }
//          else
//          {
//            /* Session does not exist and non-SYN received - ignore it */
//          }
//        }
//        else
//        {
//          if(rudpheader.type == RUDP_SYN)
//          {
//            if(currSession->receiver == NULL || currSession->receiver->status == OPENING)
//            {
//              /* Create a new receiver session and ACK the SYN*/
//              struct ReceiverSession *newReceiverSession = malloc(sizeof(struct ReceiverSession));
//              if(newReceiverSession == NULL)
//              {
//                clLogDebug("IOC", "RELIABLE", "receiveCallback: Error allocating receiver session\n");
//                return -1;
//              }
//              newReceiverSession->expected_seqno = rudpheader.seqno + 1;
//              newReceiverSession->status = OPENING;
//              newReceiverSession->sessionFinished = false;
//              currSession->receiver = newReceiverSession;
//
//              ClUint32T seqno = currSession->receiver->expected_seqno;
//              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//              free(p);
//            }
//            else
//            {
//              /* Received a SYN when there is already an active receiver session, so we ignore it */
//            }
//          }
//          if(rudpheader.type == RUDP_ACK)
//          {
//            ClUint32T ack_sqn = receivedPacket->header.seqno;
//            if(currSession->sender->status == SYN_SENT)
//            {
//              /* This an ACK for a SYN */
//              ClUint32T syn_sqn = currSession->sender->seqno;
//              if((ack_sqn - 1) == syn_sqn)
//              {
//                /* Delete the retransmission timeout */
//                eventTimeoutDelete(timeoutCallback, currSession->sender->synTimeout);
//                struct TimeoutArgs *t = (struct TimeoutArgs *) currSession->sender->synTimeout;
//                free(t->packet);
//                free(t->recipient);
//                free(t);
//                currSession->sender->status = OPEN;
//                while(currSession->sender->dataQueue != NULL)
//                {
//                  /* Check if the window is already full */
//                  if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
//                  {
//                    break;
//                  }
//                  else
//                  {
//                    ClInt32T index;
//                    ClInt32T i;
//                    /* Find the first unused window slot */
//                    for(i = RUDP_WINDOW - 1; i >= 0; i--)
//                    {
//                      if(currSession->sender->slidingWindow[i] == NULL)
//                      {
//                        index = i;
//                      }
//                    }
//                    /* Send packet, add to window and remove from queue */
//                    ClUint32T seqno = ++syn_sqn;
//                    ClInt32T len = currSession->sender->dataQueue->len;
//                    char *payload = currSession->sender->dataQueue->item;
//                    struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, len, payload,0);
//                    currSession->sender->seqno += 1;
//                    currSession->sender->slidingWindow[index] = datap;
//                    currSession->sender->retransmissionAttempts[index] = 0;
//                    struct data *temp = currSession->sender->dataQueue;
//                    currSession->sender->dataQueue = currSession->sender->dataQueue->next;
//                    free(temp->item);
//                    free(temp);
//
//                    sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
//                  }
//                }
//              }
//            }
//            else if(currSession->sender->status == OPEN)
//            {
//              /* This is an ACK for DATA */
//              if(currSession->sender->slidingWindow[0] != NULL)
//              {
//                if(currSession->sender->slidingWindow[0]->header.seqno == (rudpheader.seqno - 1))
//                {
//                  /* Correct ACK received. Remove the first window item and shift the rest left */
//                  eventTimeoutDelete(timeoutCallback, currSession->sender->dataTimeout[0]);
//                  struct TimeoutArgs *args = (struct TimeoutArgs *) currSession->sender->dataTimeout[0];
//                  free(args->packet);
//                  free(args->recipient);
//                  free(args);
//                  free(currSession->sender->slidingWindow[0]);
//
//                  ClInt32T i;
//                  if(RUDP_WINDOW == 1)
//                  {
//                    currSession->sender->slidingWindow[0] = NULL;
//                    currSession->sender->retransmissionAttempts[0] = 0;
//                    currSession->sender->dataTimeout[0] = NULL;
//                  }
//                  else
//                  {
//                    for(i = 0; i < RUDP_WINDOW - 1; i++)
//                    {
//                      currSession->sender->slidingWindow[i] = currSession->sender->slidingWindow[i + 1];
//                      currSession->sender->retransmissionAttempts[i] = currSession->sender->retransmissionAttempts[i + 1];
//                      currSession->sender->dataTimeout[i] = currSession->sender->dataTimeout[i + 1];
//
//                      if(i == RUDP_WINDOW - 2)
//                      {
//                        currSession->sender->slidingWindow[i + 1] = NULL;
//                        currSession->sender->retransmissionAttempts[i + 1] = 0;
//                        currSession->sender->dataTimeout[i + 1] = NULL;
//                      }
//                    }
//                  }
//
//                  while(currSession->sender->dataQueue != NULL)
//                  {
//                    if(currSession->sender->slidingWindow[RUDP_WINDOW - 1] != NULL)
//                    {
//                      break;
//                    }
//                    else
//                    {
//                      ClInt32T index;
//                      ClInt32T i;
//                      /* Find the first unused window slot */
//                      for(i = RUDP_WINDOW - 1; i >= 0; i--)
//                      {
//                        if(currSession->sender->slidingWindow[i] == NULL)
//                        {
//                          index = i;
//                        }
//                      }
//                      /* Send packet, add to window and remove from queue */
//                      currSession->sender->seqno = currSession->sender->seqno + 1;
//                      ClUint32T seqno = currSession->sender->seqno;
//                      ClInt32T len = currSession->sender->dataQueue->len;
//                      char *payload = currSession->sender->dataQueue->item;
//                      struct RudpPacket *datap = createRudpPacketNew(RUDP_DATA, seqno, len, payload,0);
//                      currSession->sender->slidingWindow[index] = datap;
//                      currSession->sender->retransmissionAttempts[index] = 0;
//                      struct data *temp = currSession->sender->dataQueue;
//                      currSession->sender->dataQueue = currSession->sender->dataQueue->next;
//                      free(temp->item);
//                      free(temp);
//                      sendPacketNew(false, (rudpSocketT) (long) file, datap, &sender);
//                    }
//                  }
//                  if(curr_socket->closeRequested)
//                  {
//                    /* Can the socket be closed? */
//                    struct session *headSessions = curr_socket->sessionsListHead;
//                    while(headSessions != NULL)
//                    {
//                      if(headSessions->sender->sessionFinished == false)
//                      {
//                        if(headSessions->sender->dataQueue == NULL && headSessions->sender->slidingWindow[0] == NULL && headSessions->sender->status == OPEN)
//                        {
//                          headSessions->sender->seqno += 1;
//                          struct RudpPacket *p = createRudpPacketNew(RUDP_FIN, headSessions->sender->seqno, 0, NULL,0);
//                          sendPacketNew(false, (rudpSocketT) (long) file, p, &headSessions->address);
//                          free(p);
//                          headSessions->sender->status = FIN_SENT;
//                        }
//                      }
//                      headSessions = headSessions->next;
//                    }
//                  }
//                }
//              }
//            }
//            else if(currSession->sender->status == FIN_SENT)
//            {
//              /* Handle ACK for FIN */
//              if((currSession->sender->seqno + 1) == receivedPacket->header.seqno)
//              {
//                eventTimeoutDelete(timeoutCallback, currSession->sender->finTimeout);
//                struct TimeoutArgs *t = currSession->sender->finTimeout;
//                free(t->packet);
//                free(t->recipient);
//                free(t);
//                currSession->sender->sessionFinished = true;
//                if(curr_socket->closeRequested)
//                {
//                  /* See if we can close the socket */
//                  struct session *headSessions = curr_socket->sessionsListHead;
//                  bool_t all_done = true;
//                  while(headSessions != NULL)
//                  {
//                    if(headSessions->sender->sessionFinished == false)
//                    {
//                      all_done = false;
//                    }
//                    else if(headSessions->receiver != NULL && headSessions->receiver->sessionFinished == false)
//                    {
//                      all_done = false;
//                    }
//                    else
//                    {
//                      free(headSessions->sender);
//                      if(headSessions->receiver)
//                      {
//                        free(headSessions->receiver);
//                      }
//                    }
//                    struct session *temp = headSessions;
//                    headSessions = headSessions->next;
//                    free(temp);
//                  }
//                  if(all_done)
//                  {
//                    if(curr_socket->handler != NULL)
//                    {
//                      curr_socket->handler((rudpSocketT) (long) file, RUDP_EVENT_CLOSED, &sender);
//                      close(file);
//                      free(curr_socket);
//                    }
//                  }
//                }
//              }
//              else
//              {
//                /* Received incorrect ACK for FIN - ignore it */
//              }
//            }
//          }
//          else if(rudpheader.type == RUDP_DATA)
//          {
//            /* Handle DATA packet. If the receiver is OPENING, it can transition to OPEN */
//            if(currSession->receiver->status == OPENING)
//            {
//              if(rudpheader.seqno == currSession->receiver->expected_seqno)
//              {
//                currSession->receiver->status = OPEN;
//              }
//            }
//
//            if(rudpheader.seqno == currSession->receiver->expected_seqno)
//            {
//              /* Sequence numbers match - ACK the data */
//              ClUint32T seqno = rudpheader.seqno + 1;
//              currSession->receiver->expected_seqno = seqno;
//              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//
//              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//              free(p);
//
//              /* Pass the data up to the application */
//              if(curr_socket->recvHandler != NULL)
//                curr_socket->recvHandler((rudpSocketT) (long) file, &sender, (void*) (long) &receivedPacket->payload, receivedPacket->payloadLength);
//            }
//            /* Handle the case where an ACK was lost */
//            else if(SEQ_GEQ(rudpheader.seqno, (currSession->receiver->expected_seqno - RUDP_WINDOW))
//                && SEQ_LT(rudpheader.seqno, currSession->receiver->expected_seqno))
//            {
//              ClUint32T seqno = rudpheader.seqno + 1;
//              struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//              sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//              free(p);
//            }
//          }
//          else if(rudpheader.type == RUDP_FIN)
//          {
//            if(currSession->receiver->status == OPEN)
//            {
//              if(rudpheader.seqno == currSession->receiver->expected_seqno)
//              {
//                /* If the FIN is correct, we can ACK it */
//                ClUint32T seqno = currSession->receiver->expected_seqno + 1;
//                struct RudpPacket *p = createRudpPacketNew(RUDP_ACK, seqno, 0, NULL,0);
//                sendPacketNew(true, (rudpSocketT) (long) file, p, &sender);
//                free(p);
//                currSession->receiver->sessionFinished = true;
//
//                if(curr_socket->closeRequested)
//                {
//                  /* Can we close the socket now? */
//                  struct session *headSessions = curr_socket->sessionsListHead;
//                  ClInt32T all_done = true;
//                  while(headSessions != NULL)
//                  {
//                    if(headSessions->sender->sessionFinished == false)
//                    {
//                      all_done = false;
//                    }
//                    else if(headSessions->receiver != NULL && headSessions->receiver->sessionFinished == false)
//                    {
//                      all_done = false;
//                    }
//                    else
//                    {
//                      free(headSessions->sender);
//                      if(headSessions->receiver)
//                      {
//                        free(headSessions->receiver);
//                      }
//                    }
//                    struct session *temp = headSessions;
//                    headSessions = headSessions->next;
//                    free(temp);
//                  }
//                  if(all_done)
//                  {
//                    if(curr_socket->handler != NULL)
//                    {
//                      curr_socket->handler((rudpSocketT) (long) file, RUDP_EVENT_CLOSED, &sender);
//                      close(file);
//                      free(curr_socket);
//                    }
//                  }
//                }
//              }
//              else
//              {
//                /* FIN received with incorrect sequence number - ignore it */
//              }
//            }
//          }
//        }
//      }
//    }
//  }
//
//  free(receivedPacket);
//  return 0;
//}


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

/* Register event handler callback function with a RUDP socket */
ClInt32T rudpEventHandler(rudpSocketT rsocket, ClInt32T(*handler)( rudpSocketT, rudp_event_t, struct sockaddr_in *))
{

  if(handler == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "rudpEventHandler failed: handler callback is null\n");
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
  if(curr_socket->rsock == rsocket)
  {
    curr_socket->handler = handler;
    return 0;
  }
  return -1;
}

ClInt32T rudpSendto(rudpSocketT rsocket, void* data , ClInt32T len, struct sockaddr_in* to,ClInt32T flag)
{

  if(len < 0 || len > RUDP_MAXPKTSIZE)
  {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send with invalid max packet size\n");
    return -1;
  }

  if((long)rsocket < 0)
  {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send on invalid socket\n");
    return -1;
  }

  if(to == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send to an invalid address\n");
    return -1;
  }

  bool_t newSessionCreated = true;
  ClUint32T seqno = 0;
  if(socketListHead == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "Error: attempt to send on invalid socket. No sockets in the list\n");
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
        clLogDebug("IOC", "RELIABLE", "rudpSendto: Error allocating data queue\n");
        return -1;
      }
      data_item->item = malloc(len);
      if(data_item->item == NULL)
      {
        clLogDebug("IOC", "RELIABLE", "rudpSendto: Error allocating data queue item\n");
        return -1;
      }
      memcpy(data_item->item, data, len);
      data_item->len = len;
      data_item->next = NULL;

      if(curr_socket->sessionsListHead == NULL)
      {
        seqno = rand();
        createSenderSession(curr_socket, seqno, to, &data_item);
      }
      else
      {
        bool_t session_found = false;
        struct session *currSession = curr_socket->sessionsListHead;
        //        struct session *last_in_list;
        while(currSession != NULL)
        {
          if(compareSockaddr(&currSession->address, to) == 1)
          {
            bool_t data_is_queued = false;
            bool_t we_must_queue = true;

            if(currSession->sender == NULL)
            {
              seqno = rand();
              createSenderSession(curr_socket, seqno, to, &data_item);
              struct RudpPacket *p = createRudpPacketNew(RUDP_SYN, seqno, 0, NULL,0);
              sendPacketNew(false, rsocket, p, to);
              free(p);
              newSessionCreated = false; /* Dont send the SYN twice */
              break;
            }

            if(currSession->sender->dataQueue != NULL)
              data_is_queued = true;

            if(currSession->sender->status == OPEN && !data_is_queued)
            {
              ClInt32T i;
              for(i = 0; i < RUDP_WINDOW; i++)
              {
                if(currSession->sender->slidingWindow[i] == NULL)
                {
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
              if(currSession->sender->dataQueue == NULL)
              {
                /* First entry in the data queue */
                currSession->sender->dataQueue = data_item;
              }
              else
              {
                /* Add to end of data queue */
                struct data *curr_socket = currSession->sender->dataQueue;
                while(curr_socket->next != NULL)
                {
                  curr_socket = curr_socket->next;
                }
                curr_socket->next = data_item;
              }
            }

            session_found = true;
            newSessionCreated = false;
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
      clLogDebug("IOC", "RELIABLE", "Error: attempt to send on invalid socket. Socket not found\n");
      return -1;
    }
  }
  if(newSessionCreated == true)
  {
    /* Send the SYN for the new session */
    struct RudpPacket *p = createRudpPacketNew(RUDP_SYN, seqno, 0, NULL,0);
    sendPacketNew(false, rsocket, p, to);
    free(p);
  }
  return 0;
}

ClInt32T timeoutCallback(ClInt32T fd, void *args)
{
  struct TimeoutArgs *timeargs = (struct TimeoutArgs*) args;
  clOsalMutexLock(&socketListMutex);
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket != NULL)
  {
    if(curr_socket->rsock == timeargs->fd)
    {
      break;
    }
    curr_socket = curr_socket->next;
  }
  if(curr_socket->rsock == timeargs->fd)
  {
    bool_t session_found = false;
    /* Check if we already have a session for this peer */
    struct session *currSession = curr_socket->sessionsListHead;
    while(currSession != NULL)
    {
      if(compareSockaddr(&currSession->address, timeargs->recipient) == 1)
      {
        /* Found an existing session */
        session_found = true;
        break;
      }
      currSession = currSession->next;
    }
    clOsalMutexUnlock(&socketListMutex);
    if(session_found == true)
    {
      if(timeargs->packet->header.type == RUDP_SYN)
      {
        if(currSession->sender->synRetransmitAttempts >= RUDP_MAXRETRANS)
        {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else
        {
          currSession->sender->synRetransmitAttempts++;
          sendPacketNew(false, timeargs->fd, timeargs->packet, timeargs->recipient);
          free(timeargs->packet);
        }
      }
      else if(timeargs->packet->header.type == RUDP_FIN)
      {
        if(currSession->sender->finRetransmitAttempts >= RUDP_MAXRETRANS)
        {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else
        {
          currSession->sender->finRetransmitAttempts++;
          sendPacketNew(false, timeargs->fd, timeargs->packet, timeargs->recipient);
          free(timeargs->packet);
        }
      }
      else
      {
        ClInt32T i;
        ClInt32T index = 0;
        for(i = 0; i < RUDP_WINDOW; i++)
        {
          if(currSession->sender->slidingWindow[i] != NULL && currSession->sender->slidingWindow[i]->header.seqno == timeargs->packet->header.seqno)
          {
            index = i;
          }
        }

        if(currSession->sender->retransmissionAttempts[index] >= RUDP_MAXRETRANS)
        {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else
        {
          currSession->sender->retransmissionAttempts[index]++;
          sendPacketNew(false, timeargs->fd, timeargs->packet, timeargs->recipient);
          free(timeargs->packet);
        }
      }
    }
  }

  free(timeargs->packet);
  free(timeargs->recipient);
  free(timeargs);
  return 0;
}

/* Transmit a packet via UDP */
//ClInt32T sendPacket(bool_t is_ack, rudpSocketT rsocket, struct RudpPacket *p, struct sockaddr_in *recipient)
//{
//  char type[5];
//  short t = p->header.type;
//  if(t == 1)
//    strcpy(type, "DATA");
//  else if(t == 2)
//    strcpy(type, "ACK");
//  else if(t == 4)
//    strcpy(type, "SYN");
//  else if(t == 5)
//    strcpy(type, "FIN");
//  else
//    strcpy(type, "BAD");
//
//  clLogDebug("IOC", "RELIABLE", "Sending %s packet to %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(recipient->sin_addr), ntohs(recipient->sin_port),
//      p->header.seqno, (ClInt32T) (long) rsocket);
//
//  if(DROP != 0 && rand() % DROP == 1)
//  {
//    clLogDebug("IOC", "RELIABLE", "Dropped\n");
//  }
//  else
//  {
//    if(sendto((ClInt32T) (long) rsocket, p, sizeof(struct RudpPacket), 0, (struct sockaddr*) recipient, sizeof(struct sockaddr_in)) < 0)
//    {
//      clLogDebug("IOC", "RELIABLE", "rudpSendto: sendto failed\n");
//      return -1;
//    }
//  }
//
//  if(!is_ack)
//  {
//    /* Set a timeout event if the packet isn't an ACK */
//    struct TimeoutArgs *timeargs = malloc(sizeof(struct TimeoutArgs));
//    if(timeargs == NULL)
//    {
//      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args\n");
//      return -1;
//    }
//    timeargs->packet = malloc(sizeof(struct RudpPacket));
//    if(timeargs->packet == NULL)
//    {
//      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args packet\n");
//      return -1;
//    }
//    timeargs->recipient = malloc(sizeof(struct sockaddr_in));
//    if(timeargs->packet == NULL)
//    {
//      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args recipient\n");
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
//  }
//  return 0;
//}

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

  clLogDebug("IOC", "RELIABLE", "Sending %s packet to %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(recipient->sin_addr), ntohs(recipient->sin_port),
      p->header.seqno, (ClInt32T) (long) rsocket);

  if(DROP != 0 && rand() % DROP == 1)
  {
    clLogDebug("IOC", "RELIABLE", "Dropped\n");
  }
  else
  {
    struct msghdr msghdr;
    memset(&msghdr, 0, sizeof(msghdr));
    msghdr.msg_name = (struct sockaddr*)recipient;
    msghdr.msg_namelen = sizeof(struct sockaddr_in);
    msghdr.msg_control = (ClUint8T*)gClCmsgHdr;
    msghdr.msg_controllen = gClCmsgHdrLen;
    if(p->payload!=NULL)
    {
      msghdr.msg_iov = (void*)p->payload;
      msghdr.msg_iovlen = p->payloadLength;
    }
    else
    {
      ClIocHeaderT userHeader = { 0 };
      userHeader.version = CL_IOC_HEADER_VERSION;
      userHeader.protocolType = 0;
      userHeader.priority = 0;
      userHeader.type = p->header.type;
      userHeader.isReliable = 1;
      userHeader.seqno = p->header.seqno;
      struct iovec *pIOVector = NULL;
      ClInt32T ioVectorLen = 0;
      ClRcT rc = clBufferVectorize(&userHeader,&pIOVector,&ioVectorLen);
      if(rc != CL_OK)
      {
          clLogDebug("IOC", "RELIABLE", "Error in buffer vectorize.rc=0x%x\n", rc);
          return -1;

       }
      msghdr.msg_iov = (void*)pIOVector;
      msghdr.msg_iovlen = ioVectorLen;
    }
    sendmsg((ClInt32T)(long)rsocket, &msghdr, p->flag);
  }

  if(!is_ack)
  {
    /* Set a timeout event if the packet isn't an ACK */
    struct TimeoutArgs *timeargs = malloc(sizeof(struct TimeoutArgs));
    if(timeargs == NULL)
    {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args\n");
      return -1;
    }
    timeargs->packet = malloc(sizeof(struct RudpPacket));
    if(timeargs->packet == NULL)
    {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args packet\n");
      return -1;
    }
    timeargs->recipient = malloc(sizeof(struct sockaddr_in));
    if(timeargs->packet == NULL)
    {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args recipient\n");
      return -1;
    }
    timeargs->fd = rsocket;
    memcpy(timeargs->packet, p, sizeof(struct RudpPacket));
    memcpy(timeargs->recipient, recipient, sizeof(struct sockaddr_in));

    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    struct timeval delay;
    delay.tv_sec = RUDP_TIMEOUT / 1000;
    delay.tv_usec = 0;
    struct timeval timeout_time;
    timeradd(&currentTime, &delay, &timeout_time);

    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL)
    {
      if(curr_socket->rsock == timeargs->fd)
      {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if(curr_socket->rsock == timeargs->fd)
    {
      bool_t session_found = false;
      /* Check if we already have a session for this peer */
      struct session *currSession = curr_socket->sessionsListHead;
      while(currSession != NULL)
      {
        if(compareSockaddr(&currSession->address, timeargs->recipient) == 1)
        {
          /* Found an existing session */
          session_found = true;
          break;
        }
        currSession = currSession->next;
      }
      if(session_found)
      {
        if(timeargs->packet->header.type == RUDP_SYN)
        {
          currSession->sender->synTimeout = timeargs;
        }
        else if(timeargs->packet->header.type == RUDP_FIN)
        {
          currSession->sender->finTimeout = timeargs;
        }
        else if(timeargs->packet->header.type == RUDP_DATA)
        {
          ClInt32T i;
          ClInt32T index = 0;
          for(i = 0; i < RUDP_WINDOW; i++)
          {
            if(currSession->sender->slidingWindow[i] != NULL && currSession->sender->slidingWindow[i]->header.seqno == timeargs->packet->header.seqno)
            {
              index = i;
            }
          }
          currSession->sender->dataTimeout[index] = timeargs;
        }
      }
    }
    eventTimeout(timeout_time, timeoutCallback, timeargs, "timeoutCallback");
  }
  return 0;
}

ClInt32T receiveHandlePacketNew(ClInt32T file,ClUint8T *buffer,ClUint32T bufSize,struct sockaddr_in sender)
{
  ClIocHeaderT userHeader = { 0 };
  memcpy((ClPtrT)&userHeader,(ClPtrT)buffer,sizeof(ClIocHeaderT));
  if(userHeader.isReliable == 0)
  {
    clLogDebug("IOC", "RELIABLE", "Received message in unreliable mode");
    return 1;
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

  clLogDebug("IOC", "RELIABLE", "Received %s packet from %s:%d seq number=%u on socket=%d\n", type, inet_ntoa(sender.sin_addr), ntohs(sender.sin_port),
      userHeader.seqno, file);

  /* Locate the correct socket in the socket list */
  if(socketListHead == NULL)
  {
    clLogDebug("IOC", "RELIABLE", "Error: attempt to receive on invalid socket. No sockets in the list\n");
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
        //        struct session *last_session;
        while(currSession != NULL)
        {
          //          if(currSession->next == NULL) {
          //            last_session = currSession;
          //          }
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
            if(currSession->receiver == NULL || currSession->receiver->status == OPENING)
            {
              /* Create a new receiver session and ACK the SYN*/
              struct ReceiverSession *newReceiverSession = malloc(sizeof(struct ReceiverSession));
              if(newReceiverSession == NULL)
              {
                clLogDebug("IOC", "RELIABLE", "receiveCallback: Error allocating receiver session\n");
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
            ClUint32T ack_sqn = userHeader.seqno;
            if(currSession->sender->status == SYN_SENT)
            {
              /* This an ACK for a SYN */
              ClUint32T syn_sqn = currSession->sender->seqno;
              if((ack_sqn - 1) == syn_sqn)
              {
                /* Delete the retransmission timeout */
                eventTimeoutDelete(timeoutCallback, currSession->sender->synTimeout);
                struct TimeoutArgs *t = (struct TimeoutArgs *) currSession->sender->synTimeout;
                free(t->packet);
                free(t->recipient);
                free(t);
                currSession->sender->status = OPEN;
                while(currSession->sender->dataQueue != NULL)
                {
                  /* Check if the window is already full */
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
                  }
                }
              }
            }
            else if(currSession->sender->status == OPEN)
            {
              /* This is an ACK for DATA */
              if(currSession->sender->slidingWindow[0] != NULL)
              {
                if(currSession->sender->slidingWindow[0]->header.seqno == (userHeader.seqno - 1))
                {
                  /* Correct ACK received. Remove the first window item and shift the rest left */
                  eventTimeoutDelete(timeoutCallback, currSession->sender->dataTimeout[0]);
                  struct TimeoutArgs *args = (struct TimeoutArgs *) currSession->sender->dataTimeout[0];
                  free(args->packet);
                  free(args->recipient);
                  free(args);
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
              }
            }
            else if(currSession->sender->status == FIN_SENT)
            {
              /* Handle ACK for FIN */
              if((currSession->sender->seqno + 1) == userHeader.seqno)
              {
                eventTimeoutDelete(timeoutCallback, currSession->sender->finTimeout);
                struct TimeoutArgs *t = currSession->sender->finTimeout;
                free(t->packet);
                free(t->recipient);
                free(t);
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
