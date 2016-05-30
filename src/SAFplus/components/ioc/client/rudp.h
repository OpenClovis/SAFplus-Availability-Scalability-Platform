#ifndef RUDP_PROTO_H
#define	RUDP_PROTO_H

#include <clCommon.h>
#include <clLogApi.h>


#define RUDP_VERSION	1	/* Protocol version */
#define RUDP_MAXPKTSIZE 1000	/* Number of data bytes that can sent in a packet, RUDP header not included */
#define RUDP_MAXRETRANS 5	/* Max. number of retransmissions */
#define RUDP_TIMEOUT	2000	/* Timeout for the first retransmission in milliseconds */
#define RUDP_WINDOW	3	/* Max. number of unacknowledged packets that can be sent to the network*/

/* Packet types */

#define RUDP_DATA	1
#define RUDP_ACK	2
#define RUDP_SYN	4
#define RUDP_FIN	5

/*
 * Sequence numbers are 32-bit integers operated on with modular arithmetic.
 * These macros can be used to compare sequence numbers.
 */


#define	SEQ_LT(a,b)	((short)((a)-(b)) < 0)
#define	SEQ_LEQ(a,b)	((short)((a)-(b)) <= 0)
#define	SEQ_GT(a,b)	((short)((a)-(b)) > 0)
#define	SEQ_GEQ(a,b)	((short)((a)-(b)) >= 0)

#define DROP 0

typedef enum {SYN_SENT = 0, OPENING, OPEN, FIN_SENT} rudp_state_t; /* RUDP States */

typedef enum { false = 0, true } bool_t;

#define RUDP_MAXPKTSIZE 1000    /* Number of data bytes that can sent in a
                                 * packet, RUDP header not included */

/*
 * Event types for callback notifications
 */

typedef enum {
  RUDP_EVENT_TIMEOUT,
  RUDP_EVENT_CLOSED,
} rudp_event_t;

/*
 * RUDP socket handle
 */

typedef void *rudpSocketT;


/* RUDP packet header */
struct rudp_hdr {
  ClUint16T version;
  ClUint16T type;
  ClUint32T seqno;
}__attribute__ ((packed));

struct RudpPacket {
  struct rudp_hdr header;
  ClInt32T payloadLength;
  char payload[RUDP_MAXPKTSIZE];
};

struct data {
  void *item;
  ClInt32T len;
  struct data *next;
};

struct SenderSession {
  rudp_state_t status; /* Protocol state */
  ClUint32T seqno;
  struct RudpPacket *slidingWindow[RUDP_WINDOW];
  ClInt32T retransmissionAttempts[RUDP_WINDOW];
  struct data *dataQueue; /* Queue of unsent data */
  bool_t sessionFinished; /* Has the FIN we sent been ACKed? */
  void *synTimeout; /* Argument pointer used to delete SYN timeout event */
  void *finTimeout; /* Argument pointer used to delete FIN timeout event */
  void *dataTimeout[RUDP_WINDOW]; /* Argument pointers used to delete DATA timeout events */
  ClInt32T synRetransmitAttempts;
  ClInt32T finRetransmitAttempts;
};

struct ReceiverSession {
  rudp_state_t status; /* Protocol state */
  ClUint32T expected_seqno;
  bool_t sessionFinished; /* Have we received a FIN from the sender? */
};

struct session {
  struct SenderSession *sender;
  struct ReceiverSession *receiver;
  struct sockaddr_in address;
  struct session *next;
};

/* Keeps state for potentially multiple active sockets */
struct RudpSocketList {
  rudpSocketT rsock;
  bool_t closeRequested;
  ClInt32T (*recvHandler)(rudpSocketT, struct sockaddr_in *, char *, ClInt32T);
  ClInt32T (*handler)(rudpSocketT, rudp_event_t, struct sockaddr_in *);
  struct session *sessionsListHead;
  struct RudpSocketList *next;
};

/* Arguments for timeout callback function */
struct TimeoutArgs {
  rudpSocketT fd;
  struct RudpPacket *packet;
  struct sockaddr_in *recipient;
};


struct EventData {
  struct EventData *e_next; /* next in list */
  ClInt32T (*e_fn)(ClInt32T, void*); /* callback function */
  enum {EVENT_FD, EVENT_TIME} e_type; /* type of event */
  ClInt32T e_fd; /* File descriptor */
  struct timeval e_time; /* Timeout */
  void *e_arg; /* function argument */
  char e_string[32]; /* string for identification/debugging */
};

/*
 * Socket creation
 */
rudpSocketT rudpSocket(ClInt32T port);
/*
 * Socket termination
 */
ClInt32T rudpClose(rudpSocketT rsocket);
/*
 * Send a datagram
 */
ClInt32T rudpSendto(rudpSocketT rsocket, void* data, ClInt32T len,struct sockaddr_in* to);
/*
 * Register callback function for packet receiption
 * Note: data and len arguments to callback function
 * are only valid during the call to the handler
 */
ClInt32T rudpRecvfromHandler(rudpSocketT rsocket,ClInt32T (*handler)(rudpSocketT,struct sockaddr_in *,char *, int));
/*
 * Register callback handler for event notifications
 */
ClInt32T rudpEventHandler(rudpSocketT rsocket,ClInt32T (*handler)(rudpSocketT,rudp_event_t, struct sockaddr_in *));

ClInt32T eventTimeout(struct timeval timer,ClInt32T (*callback)(ClInt32T, void*), void *callback_arg, char *idstr);
ClInt32T event_periodic(ClInt32T secs,ClInt32T (*callback)(ClInt32T, void*),void *callback_arg,char *idstr);
ClInt32T eventTimeoutDelete(ClInt32T (*callback)(ClInt32T, void*), void *callback_arg);
ClInt32T eventFdDelete(ClInt32T (*callback)(ClInt32T, void*), void *callback_arg);
ClInt32T eventFd(ClInt32T fd, ClInt32T (*callback)(ClInt32T, void*), void *callback_arg,char *idstr);
ClInt32T eventLoop();
void createSenderSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *to, struct data **data_queue);
void createReceiverSession(struct RudpSocketList *socket, ClUint32T seqno, struct sockaddr_in *addr);
struct RudpPacket *createRudpPacket(ClUint16T type, ClUint32T seqno, ClInt32T len, char *payload);
ClInt32T compareSockaddr(struct sockaddr_in *s1, struct sockaddr_in *s2);
ClInt32T receiveCallback(ClInt32T file, void *arg);
ClInt32T timeoutCallback(ClInt32T retry_attempts, void *args);
ClInt32T sendPacket(bool_t is_ack, rudpSocketT rsocket, struct RudpPacket *p, struct sockaddr_in *recipient);

#endif /* RUDP_PROTO_H */


