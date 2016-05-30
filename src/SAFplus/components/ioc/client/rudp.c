#include <unistd.h>
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

#include "rudp.h"




/*
 * Internal variables
 */
static struct EventData *ee = NULL;
static struct EventData *ee_timers = NULL;

/*
 * Sort into internal event list
 * Given an absolute timestamp, register function to call.
 */
ClInt32T
eventTimeout(struct timeval t, ClInt32T (*fn)(ClInt32T, void*), void *arg, char *str) {
  struct EventData *e, *e1, **e_prev;

  e = (struct EventData *)malloc(sizeof(struct EventData));
  if (e == NULL) {
    perror("eventTimeout: malloc");
    return -1;
  }
  memset(e, 0, sizeof(struct EventData));
  strcpy(e->e_string, str);
  e->e_fn = fn;
  e->e_arg = arg;
  e->e_type = EVENT_TIME;
  e->e_time = t;

  /* Sort into right place */
  e_prev = &ee_timers;
  for (e1 = ee_timers; e1; e1 =e1->e_next) {
    if (timercmp(&e->e_time, &e1->e_time, <))
      break;
    e_prev = &e1->e_next;
  }
  e->e_next = e1;
  *e_prev = e;
  return 0;
}

/*
 * Deregister a rudp event.
 */
static ClInt32T
eventDelete(struct EventData **firstp, ClInt32T (*fn)(ClInt32T, void*), void *arg) {
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
  /* Not found */
  return -1;
}

/*
 * Deregister a rudp event.
 */
ClInt32T eventTimeoutDelete(ClInt32T (*fn)(ClInt32T, void*), void *arg) {
  return eventDelete(&ee_timers, fn, arg);
}

/*
 * Deregister a file descriptor event.
 */
ClInt32T eventFdDelete(ClInt32T (*fn)(ClInt32T, void*), void *arg)
{
  return eventDelete(&ee, fn, arg);
}

/*
 * Register a callback function when something occurs on a file descriptor.
 * When an input event occurs on file desriptor <fd>,
 * the function <fn> shall be called with argument <arg>.
 * <str> is a debug string for logging.
 */
ClInt32T eventFd(ClInt32T fd, ClInt32T (*fn)(ClInt32T, void*), void *arg, char *str) {
  struct EventData *e;

  e = (struct EventData *)malloc(sizeof(struct EventData));
  if (e==NULL) {
    perror("eventFd: malloc");
    return -1;
  }
  memset(e, 0, sizeof(struct EventData));
  strcpy(e->e_string, str);
  e->e_fd = fd;
  e->e_fn = fn;
  e->e_arg = arg;
  e->e_type = EVENT_FD;
  e->e_next = ee;
  ee = e;
  return 0;
}


/*
 * Rudp event loop.
 * Dispatch file descriptor events (and timeouts) by invoking callbacks.
 */
ClInt32T
eventLoop() {
  struct EventData *e, *e1;
  fd_set fdset;
  ClInt32T n;
  struct timeval t, t0;

  while (ee || ee_timers) {
    FD_ZERO(&fdset);
    for (e=ee; e; e=e->e_next)
      if (e->e_type == EVENT_FD)
        FD_SET(e->e_fd, &fdset);

    if (ee_timers) {
      gettimeofday(&t0, NULL);
      timersub(&ee_timers->e_time, &t0, &t);
      if (t.tv_sec < 0)
        n = 0;
      else
        n = select(FD_SETSIZE, &fdset, NULL, NULL, &t);
    }
    else
      n = select(FD_SETSIZE, &fdset, NULL, NULL, NULL);

    if (n == -1)
      if (errno != EINTR)
        perror("eventLoop: select");
    if (n == 0) { /* Timeout */
      e = ee_timers;
      ee_timers = ee_timers->e_next;
#ifdef DEBUG
      clLogDebug("IOC", "RELIABLE", "eventLoop: timeout : %s[arg: %x]\n",
          e->e_string, (ClInt32T)(long)e->e_arg);
#endif /* DEBUG */
      if ((*e->e_fn)(0, e->e_arg) < 0) {
        return -1;
      }
      switch(e->e_type) {
        case EVENT_TIME:
          free(e);
          break;
        default:
          clLogDebug("IOC", "RELIABLE", "eventLoop: illegal e_type:%d\n", e->e_type);
      }
      continue;
    }
    e = ee;
    while (e) {
      e1 = e->e_next;
      if (e->e_type == EVENT_FD && FD_ISSET(e->e_fd, &fdset)) {
#ifdef DEBUG
        clLogDebug("IOC", "RELIABLE", "eventLoop: socket rcv: %s[fd: %d arg: %x]\n",
            e->e_string, e->e_fd, (ClInt32T)(long)e->e_arg);
#endif /* DEBUG */
        if ((*e->e_fn)(e->e_fd, e->e_arg) < 0) {
          return -1;
        }
      }
      e = e1;
    }
  }
#ifdef DEBUG
  clLogDebug("IOC", "RELIABLE", "eventLoop: returning 0\n");
#endif /* DEBUG */
  return 0;
}

/* Global variables */
bool_t rng_seeded = false;
struct RudpSocketList *socketListHead = NULL;

/* Creates a new sender session and appends it to the socket's session list */
void createSenderSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *to, struct data **dataQueue) {
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL) {
    clLogDebug("IOC", "RELIABLE", "createSenderSession: Error allocating memory\n");
    return;
  }
  newSession->address = *to;
  newSession->next = NULL;
  newSession->receiver = NULL;

  struct SenderSession *new_SenderSession = malloc(sizeof(struct SenderSession));
  if(new_SenderSession == NULL) {
    clLogDebug("IOC", "RELIABLE", "createSenderSession: Error allocating memory\n");
    return;
  }
  new_SenderSession->status = SYN_SENT;
  new_SenderSession->seqno = seqno;
  new_SenderSession->sessionFinished = false;
  /* Add data to the new session's queue */
  new_SenderSession->dataQueue = *dataQueue;
  newSession->sender = new_SenderSession;

  ClInt32T i;
  for(i = 0; i < RUDP_WINDOW; i++) {
    new_SenderSession->retransmissionAttempts[i] = 0;
    new_SenderSession->dataTimeout[i] = 0;
    new_SenderSession->slidingWindow[i] = NULL;
  }    
  new_SenderSession->synRetransmitAttempts = 0;
  new_SenderSession->finRetransmitAttempts = 0;

  if(socket->sessionsListHead == NULL) {
    socket->sessionsListHead = newSession;
  }
  else {
    struct session *curr_session = socket->sessionsListHead;
    while(curr_session->next != NULL) {
      curr_session = curr_session->next;
    }
    curr_session->next = newSession;
  }
}

/* Creates a new receiver session and appends it to the socket's session list */
void createReceiverSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *addr) {
  struct session *newSession = malloc(sizeof(struct session));
  if(newSession == NULL) {
    clLogDebug("IOC", "RELIABLE", "createReceiverSession: Error allocating memory\n");
    return;
  }
  newSession->address = *addr;
  newSession->next = NULL;
  newSession->sender = NULL;

  struct ReceiverSession *new_ReceiverSession = malloc(sizeof(struct ReceiverSession));
  if(new_ReceiverSession == NULL) {
    clLogDebug("IOC", "RELIABLE", "createReceiverSession: Error allocating memory\n");
    return;
  }
  new_ReceiverSession->status = OPENING;
  new_ReceiverSession->sessionFinished = false;
  new_ReceiverSession->expected_seqno = seqno;
  newSession->receiver = new_ReceiverSession
;

  if(socket->sessionsListHead == NULL) {
    socket->sessionsListHead = newSession;
  }
  else {
    struct session *curr_session = socket->sessionsListHead;
    while(curr_session->next != NULL) {
      curr_session = curr_session->next;
    }
    curr_session->next = newSession;
  }
}

/* Allocates a RUDP packet and returns a pointer to it */
struct RudpPacket *createRudpPacket(ClUint16T type, ClUint32T seqno, ClInt32T len, char *payload) {
  struct rudp_hdr header;
  header.version = RUDP_VERSION;
  header.type = type;
  header.seqno = seqno;

  struct RudpPacket *packet = malloc(sizeof(struct RudpPacket));
  if(packet == NULL) {
    clLogDebug("IOC", "RELIABLE", "createRudpPacket: Error allocating memory for packet\n");
    return NULL;
  }
  packet->header = header;
  packet->payloadLength = len;
  memset(&packet->payload, 0, RUDP_MAXPKTSIZE);
  if(payload != NULL)
    memcpy(&packet->payload, payload, len);

  return packet;
}

/* Returns 1 if the two sockaddr_in structs are equal and 0 if not */
ClInt32T compareSockaddr(struct sockaddr_in *s1, struct sockaddr_in *s2) {
  char sender[16];
  char recipient[16];
  strcpy(sender, inet_ntoa(s1->sin_addr));
  strcpy(recipient, inet_ntoa(s2->sin_addr));

  return ((s1->sin_family == s2->sin_family) && (strcmp(sender, recipient) == 0) && (s1->sin_port == s2->sin_port));
}

/* Creates and returns a RUDP socket */
rudpSocketT rudpSocket(ClInt32T port) {
  if(rng_seeded == false) {
    srand(time(NULL));
    rng_seeded = true;
  }
  int sockfd;
  struct sockaddr_in address;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0) {
    perror("socket");
    return(rudpSocketT)NULL;
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if( bind(sockfd, (struct sockaddr *) &address, sizeof(address)) < 0) {
    perror("bind");
    return NULL;
  }

  rudpSocketT socket = (rudpSocketT)sockfd;

  /* Create new socket and add to list of sockets */
  struct RudpSocketList *new_socket = malloc(sizeof(struct RudpSocketList));
  if(new_socket == NULL) {
    clLogDebug("IOC", "RELIABLE", "rudpSocket: Error allocating memory for socket list\n");
    return (rudpSocketT) NULL;
  }
  new_socket->rsock = socket;
  new_socket->closeRequested = false;
  new_socket->sessionsListHead = NULL;
  new_socket->next = NULL;
  new_socket->handler = NULL;
  new_socket->recvHandler = NULL;

  if(socketListHead == NULL) {
    socketListHead = new_socket;
  }
  else {
    struct RudpSocketList *curr = socketListHead;
    while(curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = new_socket;
  }

  /* Register callback event for this socket descriptor */
  if(eventFd(sockfd, receiveCallback, (void*) (long) sockfd, "receiveCallback") < 0) {
    clLogDebug("IOC", "RELIABLE", "Error registering receive callback function");
  }

  return socket;
}

/* Callback function executed when something is received on fd */
ClInt32T receiveCallback(ClInt32T file, void *arg) {
  char buf[sizeof(struct RudpPacket)];
  struct sockaddr_in sender;
  size_t sender_length = sizeof(struct sockaddr_in);
  recvfrom(file, &buf, sizeof(struct RudpPacket), 0, (struct sockaddr *)&sender, &sender_length);

  struct RudpPacket *received_packet = malloc(sizeof(struct RudpPacket));
  if(received_packet == NULL) {
    clLogDebug("IOC", "RELIABLE", "receiveCallback: Error allocating packet\n");
    return -1;
  }
  memcpy(received_packet, &buf, sizeof(struct RudpPacket));

  struct rudp_hdr rudpheader = received_packet->header;
  char type[5];
  short t = rudpheader.type;
  if(t == 1)
    strcpy(type, "DATA");
  else if(t == 2)
    strcpy(type, "ACK");
  else if(t == 4)
    strcpy(type, "SYN");
  else if(t==5)
    strcpy(type, "FIN");
  else
    strcpy(type, "BAD");

  clLogDebug("IOC", "RELIABLE","Received %s packet from %s:%d seq number=%u on socket=%d\n",type,
      inet_ntoa(sender.sin_addr), ntohs(sender.sin_port),rudpheader.seqno,file);

  /* Locate the correct socket in the socket list */
  if(socketListHead == NULL) {
    clLogDebug("IOC", "RELIABLE", "Error: attempt to receive on invalid socket. No sockets in the list\n");
    return -1;
  }
  else {
    /* We have sockets to check */
    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL) {
      if((ClInt32T)(long)curr_socket->rsock == file) {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if((ClInt32T)(long)curr_socket->rsock == file) {
      /* We found the correct socket, now see if a session already exists for this peer */
      if(curr_socket->sessionsListHead == NULL) {
        /* The list is empty, so we check if the sender has initiated the protocol properly (by sending a SYN) */
        if(rudpheader.type == RUDP_SYN) {
          /* SYN Received. Create a new session at the head of the list */
          ClUint32T seqno = rudpheader.seqno + 1;
          createReceiverSession(curr_socket, seqno, &sender);
          /* Respond with an ACK */
          struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);
          sendPacket(true, (rudpSocketT)file, p, &sender);
          free(p);
        }
        else {
          /* No sessions exist and we got a non-SYN, so ignore it */
        }
      }
      else {
        /* Some sessions exist to be checked */
        bool_t session_found = false;
        struct session *curr_session = curr_socket->sessionsListHead;
        struct session *last_session;
        while(curr_session != NULL) {
          if(curr_session->next == NULL) {
            last_session = curr_session;
          }
          if(compareSockaddr(&curr_session->address, &sender) == 1) {
            /* Found an existing session */
            session_found = true;
            break;
          }

          curr_session = curr_session->next;
        }
        if(session_found == false) {
          /* No session was found for this peer */
          if(rudpheader.type == RUDP_SYN) {
            /* SYN Received. Send an ACK and create a new session */
            ClUint32T seqno = rudpheader.seqno + 1;
            createReceiverSession(curr_socket, seqno, &sender);
            struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);
            sendPacket(true, (rudpSocketT)file, p, &sender);
            free(p);
          }
          else {
            /* Session does not exist and non-SYN received - ignore it */
          }
        }
        else {
          /* We found a matching session */ 
          if(rudpheader.type == RUDP_SYN) {
            if(curr_session->receiver == NULL || curr_session->receiver->status == OPENING) {
              /* Create a new receiver session and ACK the SYN*/
              struct ReceiverSession *new_ReceiverSession = malloc(sizeof(struct ReceiverSession));
              if(new_ReceiverSession == NULL) {
                clLogDebug("IOC", "RELIABLE", "receiveCallback: Error allocating receiver session\n");
                return -1;
              }
              new_ReceiverSession->expected_seqno = rudpheader.seqno + 1;
              new_ReceiverSession->status = OPENING;
              new_ReceiverSession->sessionFinished = false;
              curr_session->receiver = new_ReceiverSession;

              ClUint32T seqno = curr_session->receiver->expected_seqno;
              struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);
              sendPacket(true, (rudpSocketT)file, p, &sender);
              free(p);
            }
            else {
              /* Received a SYN when there is already an active receiver session, so we ignore it */
            }
          }
          if(rudpheader.type == RUDP_ACK) {
            ClUint32T ack_sqn = received_packet->header.seqno;
            if(curr_session->sender->status == SYN_SENT) {
              /* This an ACK for a SYN */
              ClUint32T syn_sqn = curr_session->sender->seqno;
              if( (ack_sqn - 1) == syn_sqn) {
                /* Delete the retransmission timeout */
                eventTimeoutDelete(timeoutCallback, curr_session->sender->synTimeout);
                struct TimeoutArgs *t = (struct TimeoutArgs *)curr_session->sender->synTimeout;
                free(t->packet);
                free(t->recipient);
                free(t);
                curr_session->sender->status = OPEN;
                while(curr_session->sender->dataQueue != NULL) {
                  /* Check if the window is already full */
                  if(curr_session->sender->slidingWindow[RUDP_WINDOW-1] != NULL) {
                    break;
                  }
                  else {
                    ClInt32T index;
                    ClInt32T i;
                    /* Find the first unused window slot */
                    for(i = RUDP_WINDOW-1; i >= 0; i--) {
                      if(curr_session->sender->slidingWindow[i] == NULL) {
                        index = i;
                      }
                    }
                    /* Send packet, add to window and remove from queue */
                    ClUint32T seqno = ++syn_sqn;
                    ClInt32T len = curr_session->sender->dataQueue->len;
                    char *payload = curr_session->sender->dataQueue->item;
                    struct RudpPacket *datap = createRudpPacket(RUDP_DATA, seqno, len, payload);
                    curr_session->sender->seqno += 1;
                    curr_session->sender->slidingWindow[index] = datap;
                    curr_session->sender->retransmissionAttempts[index] = 0;
                    struct data *temp = curr_session->sender->dataQueue;
                    curr_session->sender->dataQueue = curr_session->sender->dataQueue->next;
                    free(temp->item);
                    free(temp);

                    sendPacket(false, (rudpSocketT)file, datap, &sender);
                  }
                }
              }
            }
            else if(curr_session->sender->status == OPEN) {
              /* This is an ACK for DATA */
              if(curr_session->sender->slidingWindow[0] != NULL) {
                if(curr_session->sender->slidingWindow[0]->header.seqno == (rudpheader.seqno-1)) {
                  /* Correct ACK received. Remove the first window item and shift the rest left */
                  eventTimeoutDelete(timeoutCallback, curr_session->sender->dataTimeout[0]);
                  struct TimeoutArgs *args = (struct TimeoutArgs *)curr_session->sender->dataTimeout[0];
                  free(args->packet);
                  free(args->recipient);
                  free(args);
                  free(curr_session->sender->slidingWindow[0]);

                  ClInt32T i;
                  if(RUDP_WINDOW == 1) {
                    curr_session->sender->slidingWindow[0] = NULL;
                    curr_session->sender->retransmissionAttempts[0] = 0;
                    curr_session->sender->dataTimeout[0] = NULL;
                  }
                  else {
                    for(i = 0; i < RUDP_WINDOW - 1; i++) {
                      curr_session->sender->slidingWindow[i] = curr_session->sender->slidingWindow[i+1];
                      curr_session->sender->retransmissionAttempts[i] = curr_session->sender->retransmissionAttempts[i+1];
                      curr_session->sender->dataTimeout[i] = curr_session->sender->dataTimeout[i+1];

                      if(i == RUDP_WINDOW-2) {
                        curr_session->sender->slidingWindow[i+1] = NULL;
                        curr_session->sender->retransmissionAttempts[i+1] = 0;
                        curr_session->sender->dataTimeout[i+1] = NULL;
                      }
                    }
                  }

                  while(curr_session->sender->dataQueue != NULL) {
                    if(curr_session->sender->slidingWindow[RUDP_WINDOW-1] != NULL) {
                      break;
                    }
                    else {
                      ClInt32T index;
                      ClInt32T i;
                      /* Find the first unused window slot */
                      for(i = RUDP_WINDOW-1; i >= 0; i--) {
                        if(curr_session->sender->slidingWindow[i] == NULL) {
                          index = i;
                        }
                      }                      
                      /* Send packet, add to window and remove from queue */
                      curr_session->sender->seqno = curr_session->sender->seqno + 1;                      
                      ClUint32T seqno = curr_session->sender->seqno;
                      ClInt32T len = curr_session->sender->dataQueue->len;
                      char *payload = curr_session->sender->dataQueue->item;
                      struct RudpPacket *datap = createRudpPacket(RUDP_DATA, seqno, len, payload);
                      curr_session->sender->slidingWindow[index] = datap;
                      curr_session->sender->retransmissionAttempts[index] = 0;
                      struct data *temp = curr_session->sender->dataQueue;
                      curr_session->sender->dataQueue = curr_session->sender->dataQueue->next;
                      free(temp->item);
                      free(temp);
                      sendPacket(false, (rudpSocketT)file, datap, &sender);
                    }
                  }
                  if(curr_socket->closeRequested) {
                    /* Can the socket be closed? */
                    struct session *head_sessions = curr_socket->sessionsListHead;
                    while(head_sessions != NULL) {
                      if(head_sessions->sender->sessionFinished == false) {
                        if(head_sessions->sender->dataQueue == NULL &&
                            head_sessions->sender->slidingWindow[0] == NULL &&
                            head_sessions->sender->status == OPEN) {
                          head_sessions->sender->seqno += 1;                      
                          struct RudpPacket *p = createRudpPacket(RUDP_FIN, head_sessions->sender->seqno, 0, NULL);
                          sendPacket(false, (rudpSocketT)file, p, &head_sessions->address);
                          free(p);
                          head_sessions->sender->status = FIN_SENT;
                        }
                      }
                      head_sessions = head_sessions->next;
                    }
                  }
                }
              }
            }
            else if(curr_session->sender->status == FIN_SENT) {
              /* Handle ACK for FIN */
              if( (curr_session->sender->seqno + 1) == received_packet->header.seqno) {
                eventTimeoutDelete(timeoutCallback, curr_session->sender->finTimeout);
                struct TimeoutArgs *t = curr_session->sender->finTimeout;
                free(t->packet);
                free(t->recipient);
                free(t);
                curr_session->sender->sessionFinished = true;
                if(curr_socket->closeRequested) {
                  /* See if we can close the socket */
                  struct session *head_sessions = curr_socket->sessionsListHead;
                  bool_t all_done = true;
                  while(head_sessions != NULL) {
                    if(head_sessions->sender->sessionFinished == false) {
                      all_done = false;
                    }
                    else if(head_sessions->receiver != NULL && head_sessions->receiver->sessionFinished == false) {
                      all_done = false;
                    }
                    else {
                      free(head_sessions->sender);
                      if(head_sessions->receiver) {
                        free(head_sessions->receiver);
                      }
                    }

                    struct session *temp = head_sessions;
                    head_sessions = head_sessions->next;
                    free(temp);
                  }
                  if(all_done) {
                    if(curr_socket->handler != NULL) {
                      curr_socket->handler((rudpSocketT)file, RUDP_EVENT_CLOSED, &sender);
                      eventFdDelete(receiveCallback, (rudpSocketT)file);
                      close(file);
                      free(curr_socket);
                    }
                  }
                }
              }
              else {
                /* Received incorrect ACK for FIN - ignore it */
              }
            }
          }
          else if(rudpheader.type == RUDP_DATA) {
            /* Handle DATA packet. If the receiver is OPENING, it can transition to OPEN */
            if(curr_session->receiver->status == OPENING) {
              if(rudpheader.seqno == curr_session->receiver->expected_seqno) {
                curr_session->receiver->status = OPEN;
              }
            }

            if(rudpheader.seqno == curr_session->receiver->expected_seqno) {
              /* Sequence numbers match - ACK the data */
              ClUint32T seqno = rudpheader.seqno + 1;
              curr_session->receiver->expected_seqno = seqno;
              struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);

              sendPacket(true, (rudpSocketT)file, p, &sender);
              free(p);

              /* Pass the data up to the application */
              if(curr_socket->recvHandler != NULL)
                curr_socket->recvHandler((rudpSocketT)file, &sender,
                    (void*) (long)&received_packet->payload, received_packet->payloadLength);
            }
            /* Handle the case where an ACK was lost */
            else if(SEQ_GEQ(rudpheader.seqno, (curr_session->receiver->expected_seqno - RUDP_WINDOW)) &&
                SEQ_LT(rudpheader.seqno, curr_session->receiver->expected_seqno)) {
              ClUint32T seqno = rudpheader.seqno + 1;
              struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);
              sendPacket(true, (rudpSocketT)file, p, &sender);
              free(p);
            }
          }
          else if(rudpheader.type == RUDP_FIN) {
            if(curr_session->receiver->status == OPEN) {
              if(rudpheader.seqno == curr_session->receiver->expected_seqno) {
                /* If the FIN is correct, we can ACK it */
                ClUint32T seqno = curr_session->receiver->expected_seqno + 1;
                struct RudpPacket *p = createRudpPacket(RUDP_ACK, seqno, 0, NULL);
                sendPacket(true, (rudpSocketT)file, p, &sender);
                free(p);
                curr_session->receiver->sessionFinished = true;

                if(curr_socket->closeRequested) {
                  /* Can we close the socket now? */
                  struct session *head_sessions = curr_socket->sessionsListHead;
                  ClInt32T all_done = true;
                  while(head_sessions != NULL) {
                    if(head_sessions->sender->sessionFinished == false) {
                      all_done = false;
                    }
                    else if(head_sessions->receiver != NULL && head_sessions->receiver->sessionFinished == false) {
                      all_done = false;
                    }
                    else {
                      free(head_sessions->sender);
                      if(head_sessions->receiver) {
                        free(head_sessions->receiver);
                      }
                    }

                    struct session *temp = head_sessions;
                    head_sessions = head_sessions->next;
                    free(temp);
                  }
                  if(all_done) {
                    if(curr_socket->handler != NULL) {
                      curr_socket->handler((rudpSocketT)file, RUDP_EVENT_CLOSED, &sender);
                      eventFdDelete(receiveCallback, (rudpSocketT)file);
                      close(file);
                      free(curr_socket);
                    }
                  }
                }
              }
              else {
                /* FIN received with incorrect sequence number - ignore it */
              }
            }
          }
        }
      }
    }
  }

  free(received_packet);
  return 0;
}

/* Close a RUDP socket */
ClInt32T rudpClose(rudpSocketT rsocket) {
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket->next != NULL) {
    if(curr_socket->rsock == rsocket) {
      break;
    }
    curr_socket = curr_socket->next;
  }
  if(curr_socket->rsock == rsocket) {
    curr_socket->closeRequested = true;
    return 0;
  }

  return -1;
}

/* Register receive callback function */ 
ClInt32T rudpRecvfromHandler(rudpSocketT rsocket, ClInt32T (*handler)(rudpSocketT,
    struct sockaddr_in *, char *, int)) {

  if(handler == NULL) {
    clLogDebug("IOC", "RELIABLE", "rudpRecvfromHandler failed: handler callback is null\n");
    return -1;
  }
  /* Find the proper socket from the socket list */
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket->next != NULL) {
    if(curr_socket->rsock == rsocket) {
      break;
    }
    curr_socket = curr_socket->next;
  }
  /* Extra check to handle the case where an invalid rsock is used */
  if(curr_socket->rsock == rsocket) {
    curr_socket->recvHandler = handler;
    return 0;
  }
  return -1;
}

/* Register event handler callback function with a RUDP socket */
ClInt32T rudpEventHandler(rudpSocketT rsocket,
    ClInt32T (*handler)(rudpSocketT, rudp_event_t,
        struct sockaddr_in *)) {

  if(handler == NULL) {
    clLogDebug("IOC", "RELIABLE", "rudpEventHandler failed: handler callback is null\n");
    return -1;
  }

  /* Find the proper socket from the socket list */
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket->next != NULL) {
    if(curr_socket->rsock == rsocket) {
      break;
    }
    curr_socket = curr_socket->next;
  }

  /* Extra check to handle the case where an invalid rsock is used */
  if(curr_socket->rsock == rsocket) {
    curr_socket->handler = handler;
    return 0;
  }
  return -1;
}


/* Sends a block of data to the receiver. Returns 0 on success, -1 on error */
ClInt32T rudpSendto(rudpSocketT rsocket, void* data, ClInt32T len, struct sockaddr_in* to) {

  if(len < 0 || len > RUDP_MAXPKTSIZE) {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send with invalid max packet size\n");
    return -1;
  }

  if(rsocket < 0) {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send on invalid socket\n");
    return -1;
  }

  if(to == NULL) {
    clLogDebug("IOC", "RELIABLE", "rudpSendto Error: attempting to send to an invalid address\n");
    return -1;
  }

  bool_t newSessionCreated = true;
  ClUint32T seqno = 0;
  if(socketListHead == NULL) {
    clLogDebug("IOC", "RELIABLE", "Error: attempt to send on invalid socket. No sockets in the list\n");
    return -1;
  }
  else {
    /* Find the correct socket in our list */
    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL) {
      if(curr_socket->rsock == rsocket) {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if(curr_socket->rsock == rsocket) {
      /* We found the correct socket, now see if a session already exists for this peer */
      struct data *data_item = malloc(sizeof(struct data));
      if(data_item == NULL) {
        clLogDebug("IOC", "RELIABLE", "rudpSendto: Error allocating data queue\n");
        return -1;
      }  
      data_item->item = malloc(len);
      if(data_item->item == NULL) {
        clLogDebug("IOC", "RELIABLE", "rudpSendto: Error allocating data queue item\n");
        return -1;
      }
      memcpy(data_item->item, data, len);
      data_item->len = len;
      data_item->next = NULL;

      if(curr_socket->sessionsListHead == NULL) {
        /* The list is empty, so we create a new sender session at the head of the list */
        seqno = rand();
        createSenderSession(curr_socket, seqno, to, &data_item);
      }
      else {
        bool_t session_found = false;
        struct session *curr_session = curr_socket->sessionsListHead;
        struct session *last_in_list;
        while(curr_session != NULL) {
          if(compareSockaddr(&curr_session->address, to) == 1) {
            bool_t data_is_queued = false;
            bool_t we_must_queue = true;

            if(curr_session->sender==NULL) {
              seqno = rand();
              createSenderSession(curr_socket, seqno, to, &data_item);
              struct RudpPacket *p = createRudpPacket(RUDP_SYN, seqno, 0, NULL);
              sendPacket(false, rsocket, p, to);
              free(p);
              newSessionCreated = false ; /* Dont send the SYN twice */
              break;
            }

            if(curr_session->sender->dataQueue != NULL)
              data_is_queued = true;

            if(curr_session->sender->status == OPEN && !data_is_queued) {
              ClInt32T i;
              for(i = 0; i < RUDP_WINDOW; i++) {
                if(curr_session->sender->slidingWindow[i] == NULL) {
                  curr_session->sender->seqno = curr_session->sender->seqno + 1;
                  struct RudpPacket *datap = createRudpPacket(RUDP_DATA, curr_session->sender->seqno, len, data);
                  curr_session->sender->slidingWindow[i] = datap;
                  curr_session->sender->retransmissionAttempts[i] = 0;
                  sendPacket(false, rsocket, datap, to);
                  we_must_queue = false;
                  break;
                }
              }
            }

            if(we_must_queue == true) {
              if(curr_session->sender->dataQueue == NULL) {
                /* First entry in the data queue */
                curr_session->sender->dataQueue = data_item;
              }
              else {
                /* Add to end of data queue */
                struct data *curr_socket = curr_session->sender->dataQueue;
                while(curr_socket->next != NULL) {
                  curr_socket = curr_socket->next;
                }
                curr_socket->next = data_item;
              }
            }

            session_found = true;
            newSessionCreated = false;
            break;
          }
          if(curr_session->next==NULL)
            last_in_list=curr_session;

          curr_session = curr_session->next;
        }
        if(!session_found) {
          /* If not, create a new session */
          seqno = rand();
          createSenderSession(curr_socket, seqno, to, &data_item);
        }
      }
    }
    else {
      clLogDebug("IOC", "RELIABLE", "Error: attempt to send on invalid socket. Socket not found\n");
      return -1;
    }
  }
  if(newSessionCreated == true) {
    /* Send the SYN for the new session */
    struct RudpPacket *p = createRudpPacket(RUDP_SYN, seqno, 0, NULL);
    sendPacket(false, rsocket, p, to);
    free(p);
  }
  return 0;
}

/* Callback function when a timeout occurs */
ClInt32T timeoutCallback(ClInt32T fd, void *args) {
  struct TimeoutArgs *timeargs=(struct TimeoutArgs*)args;
  struct RudpSocketList *curr_socket = socketListHead;
  while(curr_socket != NULL) {
    if(curr_socket->rsock == timeargs->fd) {
      break;
    }
    curr_socket = curr_socket->next;
  }
  if(curr_socket->rsock == timeargs->fd) {
    bool_t session_found = false;
    /* Check if we already have a session for this peer */
    struct session *curr_session = curr_socket->sessionsListHead;
    while(curr_session != NULL) {
      if(compareSockaddr(&curr_session->address, timeargs->recipient) == 1) {
        /* Found an existing session */
        session_found = true;
        break;
      }
      curr_session = curr_session->next;
    }
    if(session_found == true) {
      if(timeargs->packet->header.type == RUDP_SYN) {
        if(curr_session->sender->synRetransmitAttempts >= RUDP_MAXRETRANS) {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else {
          curr_session->sender->synRetransmitAttempts++;
          sendPacket(false, timeargs->fd, timeargs->packet, timeargs->recipient);
          free(timeargs->packet);
        }
      }
      else if(timeargs->packet->header.type == RUDP_FIN) {
        if(curr_session->sender->finRetransmitAttempts >= RUDP_MAXRETRANS) {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else {
          curr_session->sender->finRetransmitAttempts++;
          sendPacket(false, timeargs->fd, timeargs->packet, timeargs->recipient);
          free(timeargs->packet);
        }
      }
      else {
        ClInt32T i;
        ClInt32T index;
        for(i = 0; i < RUDP_WINDOW; i++) {
          if(curr_session->sender->slidingWindow[i] != NULL &&
              curr_session->sender->slidingWindow[i]->header.seqno == timeargs->packet->header.seqno) {
            index = i;
          }
        }

        if(curr_session->sender->retransmissionAttempts[index] >= RUDP_MAXRETRANS) {
          curr_socket->handler(timeargs->fd, RUDP_EVENT_TIMEOUT, timeargs->recipient);
        }
        else {
          curr_session->sender->retransmissionAttempts[index]++;
          sendPacket(false, timeargs->fd, timeargs->packet, timeargs->recipient);
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
ClInt32T sendPacket(bool_t is_ack, rudpSocketT rsocket, struct RudpPacket *p, struct sockaddr_in *recipient) {
  char type[5];
  short t=p->header.type;
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

  clLogDebug("IOC", "RELIABLE","Sending %s packet to %s:%d seq number=%u on socket=%d\n",type,
      inet_ntoa(recipient->sin_addr), ntohs(recipient->sin_port), p->header.seqno,(ClInt32T)(long))rsocket);

  if (DROP != 0 && rand() % DROP == 1) {
    clLogDebug("IOC", "RELIABLE","Dropped\n");
  }
  else {
    if (sendto(ClInt32T)(long))rsocket, p, sizeof(struct RudpPacket), 0, (struct sockaddr*)recipient, sizeof(struct sockaddr_in)) < 0) {
      clLogDebug("IOC", "RELIABLE", "rudpSendto: sendto failed\n");
      return -1;
    }
  }

  if(!is_ack) {
    /* Set a timeout event if the packet isn't an ACK */
    struct TimeoutArgs *timeargs = malloc(sizeof(struct TimeoutArgs));
    if(timeargs == NULL) {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args\n");
      return -1;
    }
    timeargs->packet = malloc(sizeof(struct RudpPacket));
    if(timeargs->packet == NULL) {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args packet\n");
      return -1;
    }
    timeargs->recipient = malloc(sizeof(struct sockaddr_in));
    if(timeargs->packet == NULL) {
      clLogDebug("IOC", "RELIABLE", "sendPacket: Error allocating timeout args recipient\n");
      return -1;
    }
    timeargs->fd = rsocket;
    memcpy(timeargs->packet, p, sizeof(struct RudpPacket));
    memcpy(timeargs->recipient, recipient, sizeof(struct sockaddr_in));  

    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    struct timeval delay;
    delay.tv_sec = RUDP_TIMEOUT/1000;
    delay.tv_usec= 0;
    struct timeval timeout_time;
    timeradd(&currentTime, &delay, &timeout_time);

    struct RudpSocketList *curr_socket = socketListHead;
    while(curr_socket != NULL) {
      if(curr_socket->rsock == timeargs->fd) {
        break;
      }
      curr_socket = curr_socket->next;
    }
    if(curr_socket->rsock == timeargs->fd) {
      bool_t session_found = false;
      /* Check if we already have a session for this peer */
      struct session *curr_session = curr_socket->sessionsListHead;
      while(curr_session != NULL) {
        if(compareSockaddr(&curr_session->address, timeargs->recipient) == 1) {
          /* Found an existing session */
          session_found = true;
          break;
        }
        curr_session = curr_session->next;
      }
      if(session_found) {
        if(timeargs->packet->header.type == RUDP_SYN) {
          curr_session->sender->synTimeout = timeargs;
        }
        else if(timeargs->packet->header.type == RUDP_FIN) {
          curr_session->sender->finTimeout = timeargs;
        }
        else if(timeargs->packet->header.type == RUDP_DATA) {
          ClInt32T i;
          ClInt32T index;
          for(i = 0; i < RUDP_WINDOW; i++) {
            if(curr_session->sender->slidingWindow[i] != NULL &&
                curr_session->sender->slidingWindow[i]->header.seqno == timeargs->packet->header.seqno) {
              index = i;
            }
          }
          curr_session->sender->dataTimeout[index] = timeargs;
        }
      }
    }
    eventTimeout(timeout_time, timeoutCallback, timeargs, "timeoutCallback");
  }
  return 0;
}
