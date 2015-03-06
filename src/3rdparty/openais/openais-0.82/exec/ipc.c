/*
 * Copyright (c) 2002-2006 MontaVista Software, Inc.
 * Copyright (c) 2006-2007 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@mvista.com)
 *
 * This software licensed under BSD license, the text of which follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the MontaVista Software, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <pthread.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#if defined(OPENAIS_SOLARIS) && defined(HAVE_GETPEERUCRED)
#include <ucred.h>
#endif

#include "swab.h"
#include "../include/saAis.h"
#include "../include/list.h"
#include "../include/queue.h"
#include "../lcr/lcr_ifact.h"
#include "poll.h"
#include "totempg.h"
#include "totemsrp.h"
#include "mempool.h"
#include "mainconfig.h"
#include "totemconfig.h"
#include "main.h"
#include "ipc.h"
#include "flow.h"
#include "service.h"
#include "sync.h"
#include "swab.h"
#include "objdb.h"
#include "config.h"
#include "tlist.h"
#define LOG_SERVICE LOG_SERVICE_IPC
#include "logsys.h"

#include "util.h"

LOGSYS_DECLARE_SUBSYS ("IPC", LOG_INFO);

#ifdef OPENAIS_SOLARIS
#define MSG_NOSIGNAL 0
#endif

#define SERVER_BACKLOG 5

/*
 * When there are this many entries left in a queue, turn on flow control
 */
#define FLOW_CONTROL_ENTRIES_ENABLE 400

/*
 * When there are this many entries in a queue, turn off flow control
 */
#define FLOW_CONTROL_ENTRIES_DISABLE 64


static unsigned int g_gid_valid = 0;

static totempg_groups_handle ipc_handle;

DECLARE_LIST_INIT (conn_info_list_head);

static void (*ipc_serialize_lock_fn) (void);

static void (*ipc_serialize_unlock_fn) (void);

struct outq_item {
	void *msg;
	size_t mlen;
};

enum conn_state {
	CONN_STATE_ACTIVE,
	CONN_STATE_SECURITY,
	CONN_STATE_REQUESTED,
	CONN_STATE_CLOSED,
	CONN_STATE_DISCONNECTED
};

struct conn_info {
	int fd;			/* File descriptor  */
	unsigned int events;	/* events polled for by file descriptor */
	enum conn_state state;	/* State of this connection */
	pthread_t thread;	/* thread identifier */
	pthread_attr_t thread_attr;	/* thread attribute */
	char *inb;		/* Input buffer for non-blocking reads */
	int inb_nextheader;	/* Next message header starts here */
	int inb_start;		/* Start location of input buffer */
	int inb_inuse;		/* Bytes currently stored in input buffer */
	struct queue outq;	/* Circular queue for outgoing requests */
	int byte_start;		/* Byte to start sending from in head of queue */
	enum service_types service;/* Type of service so dispatch knows how to route message */
	int authenticated;	/* Is this connection authenticated? */
	void *private_data;	/* library connection private data */
	struct conn_info *conn_info_partner;	/* partner connection dispatch<->response */
	unsigned int flow_control_handle;	/* flow control identifier */
	unsigned int flow_control_enabled;	/* flow control enabled bit */
	unsigned int flow_control_local_count;	/* flow control local count */
	enum openais_flow_control flow_control;	/* Does this service use IPC flow control */
	pthread_mutex_t flow_control_mutex;
        int (*lib_exit_fn) (void *conn);
	struct timerlist timerlist;
	pthread_mutex_t mutex;
	pthread_mutex_t *shared_mutex;
	struct list_head list;
};

static void *prioritized_poll_thread (void *conn);
static int conn_info_outq_flush (struct conn_info *conn_info);
static void libais_deliver (struct conn_info *conn_info);
static void ipc_flow_control (struct conn_info *conn_info);

 /*
  * IPC Initializers
  */

static int response_init_send_response (
	struct conn_info *conn_info,
	void *message);
static int dispatch_init_send_response (
	struct conn_info *conn_info,
	void *message);

static int (*ais_init_service[]) (struct conn_info *conn_info, void *message) = {
	response_init_send_response,
	dispatch_init_send_response
};

static void libais_disconnect_security (struct conn_info *conn_info)
{
	conn_info->state = CONN_STATE_SECURITY;
	close (conn_info->fd);
}

static int response_init_send_response (
	struct conn_info *conn_info,
	void *message)
{
	SaAisErrorT error = SA_AIS_ERR_ACCESS;
	uintptr_t cinfo = (uintptr_t)conn_info;
	mar_req_lib_response_init_t *req_lib_response_init = (mar_req_lib_response_init_t *)message;
	mar_res_lib_response_init_t res_lib_response_init;

	if (conn_info->authenticated) {
		conn_info->service = req_lib_response_init->resdis_header.service;
		error = SA_AIS_OK;
	}
	res_lib_response_init.header.size = sizeof (mar_res_lib_response_init_t);
	res_lib_response_init.header.id = MESSAGE_RES_INIT;
	res_lib_response_init.header.error = error;
	res_lib_response_init.conn_info = (mar_uint64_t)cinfo;

	openais_conn_send_response (
		conn_info,
		&res_lib_response_init,
		sizeof (res_lib_response_init));

	if (error == SA_AIS_ERR_ACCESS) {
		libais_disconnect_security (conn_info);
		return (-1);
	}
	return (0);
}

static int dispatch_init_send_response (
	struct conn_info *conn_info,
	void *message)
{
	SaAisErrorT error = SA_AIS_ERR_ACCESS;
	uintptr_t cinfo;
	mar_req_lib_dispatch_init_t *req_lib_dispatch_init = (mar_req_lib_dispatch_init_t *)message;
	mar_res_lib_dispatch_init_t res_lib_dispatch_init;
	struct conn_info *msg_conn_info;

	if (conn_info->authenticated) {
		conn_info->service = req_lib_dispatch_init->resdis_header.service;
		if (!ais_service[req_lib_dispatch_init->resdis_header.service])
			error = SA_AIS_ERR_NOT_SUPPORTED;
		else
			error = SA_AIS_OK;

		cinfo = (uintptr_t)req_lib_dispatch_init->conn_info;
		conn_info->conn_info_partner = (struct conn_info *)cinfo;

		/* temporary fix for memory leak
		 */
		pthread_mutex_destroy (conn_info->conn_info_partner->shared_mutex);
		free (conn_info->conn_info_partner->shared_mutex);
		
		conn_info->conn_info_partner->shared_mutex = conn_info->shared_mutex;

		list_add (&conn_info_list_head, &conn_info->list);
		list_add (&conn_info_list_head, &conn_info->conn_info_partner->list);

		msg_conn_info = (struct conn_info *)cinfo;
		msg_conn_info->conn_info_partner = conn_info;

		if (error == SA_AIS_OK) {
			int private_data_size;

			private_data_size = ais_service[req_lib_dispatch_init->resdis_header.service]->private_data_size;
			if (private_data_size) {
				conn_info->private_data = malloc (private_data_size);

				conn_info->conn_info_partner->private_data = conn_info->private_data;
				if (conn_info->private_data == NULL) {
					error = SA_AIS_ERR_NO_MEMORY;
				} else {
					memset (conn_info->private_data, 0, private_data_size);
				}
			} else {
				conn_info->private_data = NULL;
				conn_info->conn_info_partner->private_data = NULL;
			}
		}
	}

	res_lib_dispatch_init.header.size = sizeof (mar_res_lib_dispatch_init_t);
	res_lib_dispatch_init.header.id = MESSAGE_RES_INIT;
	res_lib_dispatch_init.header.error = error;

	openais_conn_send_response (
		conn_info,
		&res_lib_dispatch_init,
		sizeof (res_lib_dispatch_init));

	if (error == SA_AIS_ERR_ACCESS) {
		libais_disconnect_security (conn_info);
		return (-1);
	}
	if (error != SA_AIS_OK) {
		return (-1);
	}

	conn_info->state = CONN_STATE_ACTIVE;
	conn_info->conn_info_partner->state = CONN_STATE_ACTIVE;
	conn_info->lib_exit_fn = ais_service[conn_info->service]->lib_exit_fn;
	ais_service[conn_info->service]->lib_init_fn (conn_info);

	conn_info->flow_control = ais_service[conn_info->service]->flow_control;
	conn_info->conn_info_partner->flow_control = ais_service[conn_info->service]->flow_control;
	if (ais_service[conn_info->service]->flow_control == OPENAIS_FLOW_CONTROL_REQUIRED) {
		openais_flow_control_ipc_init (
			&conn_info->flow_control_handle,
			conn_info->service);

	}
	return (0);
}

/*
 * Create a connection data structure
 */
static inline unsigned int conn_info_create (int fd) {
	struct conn_info *conn_info;
	int res;

	conn_info = malloc (sizeof (struct conn_info));
	if (conn_info == 0) {
		return (ENOMEM);
	}

	memset (conn_info, 0, sizeof (struct conn_info));

	res = queue_init (&conn_info->outq, SIZEQUEUE,
		sizeof (struct outq_item));
	if (res != 0) {
		free (conn_info);
		return (ENOMEM);
	}
	conn_info->inb = malloc (sizeof (char) * SIZEINB);
	if (conn_info->inb == NULL) {
		queue_free (&conn_info->outq);
		free (conn_info);
		return (ENOMEM);
	}
	conn_info->shared_mutex = malloc (sizeof (pthread_mutex_t));
	if (conn_info->shared_mutex == NULL) {
		free (conn_info->inb);
		queue_free (&conn_info->outq);
		free (conn_info);
		return (ENOMEM);
	}

	pthread_mutex_init (&conn_info->mutex, NULL);
	pthread_mutex_init (&conn_info->flow_control_mutex, NULL);
	pthread_mutex_init (conn_info->shared_mutex, NULL);

	list_init (&conn_info->list);
	conn_info->state = CONN_STATE_ACTIVE;
	conn_info->fd = fd;
	conn_info->events = POLLIN|POLLNVAL;
	conn_info->service = SOCKET_SERVICE_INIT;

	pthread_attr_init (&conn_info->thread_attr);
/*
 * IA64 needs more stack space then other arches
 */
#if defined(__ia64__)
	pthread_attr_setstacksize (&conn_info->thread_attr, 400000);
#else
	pthread_attr_setstacksize (&conn_info->thread_attr, 200000);
#endif

	pthread_attr_setdetachstate (&conn_info->thread_attr, PTHREAD_CREATE_DETACHED);
	res = pthread_create (&conn_info->thread, &conn_info->thread_attr,
		prioritized_poll_thread, conn_info);
	return (res);
}

static void conn_info_destroy (struct conn_info *conn_info)
{
	struct outq_item *outq_item;

	/*
	 * Free the outq queued items
	 */
	while (!queue_is_empty (&conn_info->outq)) {
		outq_item = queue_item_get (&conn_info->outq);
		free (outq_item->msg);
		queue_item_remove (&conn_info->outq);
	}

	queue_free (&conn_info->outq);
	free (conn_info->inb);
	if (conn_info->conn_info_partner) {
		conn_info->conn_info_partner->conn_info_partner = NULL;
	}
	
	pthread_attr_destroy (&conn_info->thread_attr);
	pthread_mutex_destroy (&conn_info->mutex);
	pthread_mutex_destroy (&conn_info->flow_control_mutex);

	list_del (&conn_info->list);
	free (conn_info);
}

int libais_connection_active (struct conn_info *conn_info);

int libais_connection_active (struct conn_info *conn_info)
{
	return (conn_info->state == CONN_STATE_ACTIVE);
}

static void libais_disconnect_request (struct conn_info *conn_info)
{
	if (conn_info->state == CONN_STATE_ACTIVE) {
		conn_info->state = CONN_STATE_REQUESTED;
		conn_info->conn_info_partner->state = CONN_STATE_REQUESTED;
	}
}

static int libais_disconnect (struct conn_info *conn_info)
{
	int res = 0;

	assert (conn_info->state != CONN_STATE_ACTIVE);

	if (conn_info->state == CONN_STATE_DISCONNECTED) {
		assert (0);
	}

	/*
	 * Close active connections
	 */
	if (conn_info->state == CONN_STATE_ACTIVE || conn_info->state == CONN_STATE_REQUESTED) {
		close (conn_info->fd);
		conn_info->state = CONN_STATE_CLOSED;
		close (conn_info->conn_info_partner->fd);
		conn_info->conn_info_partner->state = CONN_STATE_CLOSED;
	}

	/*
	 * Note we will only call the close operation once on the first time
	 * one of the connections is closed
	 */	
	if (conn_info->state == CONN_STATE_CLOSED) {
		if (conn_info->lib_exit_fn) {
			res = conn_info->lib_exit_fn (conn_info);
		}
		if (res == -1) {
			return (-1);
		}
		if (conn_info->conn_info_partner->lib_exit_fn) {
			res = conn_info->conn_info_partner->lib_exit_fn (conn_info);
		}
		if (res == -1) {
			return (-1);
		}
	}
	conn_info->state = CONN_STATE_DISCONNECTED;
	conn_info->conn_info_partner->state = CONN_STATE_DISCONNECTED;
	if (conn_info->flow_control_enabled == 1) {
		openais_flow_control_disable (conn_info->flow_control_handle);
	}
	return (0);
}

static inline void conn_info_mutex_lock (
	struct conn_info *conn_info,
	unsigned int service)
{
	if (service == SOCKET_SERVICE_INIT) {
		pthread_mutex_lock (&conn_info->mutex);
	} else {
		pthread_mutex_lock (conn_info->shared_mutex);
	}
}
static inline void conn_info_mutex_unlock (
	struct conn_info *conn_info,
	unsigned int service)
{
	if (service == SOCKET_SERVICE_INIT) {
		pthread_mutex_unlock (&conn_info->mutex);
	} else {
		pthread_mutex_unlock (conn_info->shared_mutex);
	}
}

/*
 * This thread runs in a specific thread priority mode to handle
 * I/O requests from the library
 */
static void *prioritized_poll_thread (void *conn)
{
	struct conn_info *conn_info = (struct conn_info *)conn;
	struct pollfd ufd;
	int fds;
	struct sched_param sched_param;
	int res;
	pthread_mutex_t *rel_mutex;
	unsigned int service;
	struct conn_info *cinfo_partner;
	void *private_data;

	sched_param.sched_priority = 1;
	res = pthread_setschedparam (conn_info->thread, SCHED_RR, &sched_param);

	ufd.fd = conn_info->fd;
	for (;;) {
retry_poll:
		service = conn_info->service;
		ufd.events = conn_info->events;
		ufd.revents = 0;
		fds = poll (&ufd, 1, -1);
		
		conn_info_mutex_lock (conn_info, service);
		
		switch (conn_info->state) {
		case CONN_STATE_SECURITY:
			conn_info_mutex_unlock (conn_info, service);
			pthread_mutex_destroy (conn_info->shared_mutex);
			free (conn_info->shared_mutex);
			conn_info_destroy (conn);
			pthread_exit (0);
			break;

		case CONN_STATE_REQUESTED:
		case CONN_STATE_CLOSED:
			res = libais_disconnect (conn);
			if (res != 0) {
				conn_info_mutex_unlock (conn_info, service);
				goto retry_poll;
			}
			break;

		case CONN_STATE_DISCONNECTED:
			rel_mutex = conn_info->shared_mutex;
			private_data = conn_info->private_data;
			cinfo_partner = conn_info->conn_info_partner;
			conn_info_destroy (conn);
			if (service == SOCKET_SERVICE_INIT) {
				pthread_mutex_unlock (&conn_info->mutex);
			} else {
				pthread_mutex_unlock (rel_mutex);
			}
			if (cinfo_partner == NULL) {
				pthread_mutex_destroy (rel_mutex);
				free (rel_mutex);
				free (private_data);
			}
			pthread_exit (0);
			/*
			 * !! NOTE !! this is the exit point for this thread
			 */
			break;

		default:
			break;
		}

		if (fds == -1) {
			conn_info_mutex_unlock (conn_info, service);
			goto retry_poll;
		}

		ipc_serialize_lock_fn ();

		if (fds == 1 && ufd.revents) {
			if (ufd.revents & (POLLERR|POLLHUP)) {

				libais_disconnect_request (conn_info);

				conn_info_mutex_unlock (conn_info, service);
				ipc_serialize_unlock_fn ();
				continue;
			}
			
			if (ufd.revents & POLLOUT) {
				conn_info_outq_flush (conn_info);
			}

			if ((ufd.revents & POLLIN) == POLLIN) {
				libais_deliver (conn_info);
			}

			ipc_flow_control (conn_info);

		}

		ipc_serialize_unlock_fn ();
		conn_info_mutex_unlock (conn_info, service);
	}

	/*
	 * This code never reached
	 */
	return (0);
}

#if defined(OPENAIS_LINUX) || defined(OPENAIS_SOLARIS)
/* SUN_LEN is broken for abstract namespace
 */
#define AIS_SUN_LEN(a) sizeof(*(a))
#else
#define AIS_SUN_LEN(a) SUN_LEN(a)
#endif

#if defined(OPENAIS_LINUX)
char *socketname = "libais.socket";
#else
char *socketname = "/var/run/libais.socket";
#endif


static void ipc_flow_control (struct conn_info *conn_info)
{
	unsigned int entries_used;
	unsigned int entries_usedhw;
	unsigned int flow_control_local_count;
	unsigned int fcc;

	/*
	 * Determine FCC variable and printing variables
	 */
	entries_used = queue_used (&conn_info->outq);
	if (conn_info->conn_info_partner &&
		queue_used (&conn_info->conn_info_partner->outq) > entries_used) {
		entries_used = queue_used (&conn_info->conn_info_partner->outq);
	}
	entries_usedhw = queue_usedhw (&conn_info->outq);
	if (conn_info->conn_info_partner &&
		queue_usedhw (&conn_info->conn_info_partner->outq) > entries_used) {
		entries_usedhw = queue_usedhw (&conn_info->conn_info_partner->outq);
	}
	flow_control_local_count = conn_info->flow_control_local_count;
	if (conn_info->conn_info_partner &&
		conn_info->conn_info_partner->flow_control_local_count > flow_control_local_count) {
		flow_control_local_count = conn_info->conn_info_partner->flow_control_local_count;
	}

	fcc = entries_used;
	if (flow_control_local_count > fcc) {
		fcc = flow_control_local_count;
	}
	/*
	 * IPC group-wide flow control
	 */
	if (conn_info->flow_control == OPENAIS_FLOW_CONTROL_REQUIRED) {
		if (conn_info->flow_control_enabled == 0 &&
			((fcc + FLOW_CONTROL_ENTRIES_ENABLE) > SIZEQUEUE)) {

			log_printf (LOG_LEVEL_NOTICE, "Enabling flow control [%d/%d] - [%d].\n",
				entries_usedhw, SIZEQUEUE,
				flow_control_local_count);
			openais_flow_control_enable (conn_info->flow_control_handle);
			conn_info->flow_control_enabled = 1;
			conn_info->conn_info_partner->flow_control_enabled = 1;
		}
		if (conn_info->flow_control_enabled == 1 &&

			fcc <= FLOW_CONTROL_ENTRIES_DISABLE) {

			log_printf (LOG_LEVEL_NOTICE, "Disabling flow control [%d/%d] - [%d].\n",
				entries_usedhw, SIZEQUEUE,
				flow_control_local_count);
			openais_flow_control_disable (conn_info->flow_control_handle);
			conn_info->flow_control_enabled = 0;
			conn_info->conn_info_partner->flow_control_enabled = 0;
		}
	}
}

static int conn_info_outq_flush (struct conn_info *conn_info) {
	struct queue *outq;
	int res = 0;
	struct outq_item *queue_item;
	struct msghdr msg_send;
	struct iovec iov_send;
	char *msg_addr;

	if (!libais_connection_active (conn_info)) {
		return (-1);
	}
	outq = &conn_info->outq;

	msg_send.msg_iov = &iov_send;
	msg_send.msg_name = 0;
	msg_send.msg_namelen = 0;
	msg_send.msg_iovlen = 1;
#ifndef OPENAIS_SOLARIS
	msg_send.msg_control = 0;
	msg_send.msg_controllen = 0;
	msg_send.msg_flags = 0;
#else
	msg_send.msg_accrights = 0;
	msg_send.msg_accrightslen = 0;
#endif

	while (!queue_is_empty (outq)) {
		queue_item = queue_item_get (outq);
		msg_addr = (char *)queue_item->msg;
		msg_addr = &msg_addr[conn_info->byte_start];

		iov_send.iov_base = msg_addr;
		iov_send.iov_len = queue_item->mlen - conn_info->byte_start;

retry_sendmsg:
		res = sendmsg (conn_info->fd, &msg_send, MSG_NOSIGNAL);
		if (res == -1 && errno == EINTR) {
			goto retry_sendmsg;
		}
		if (res == -1 && errno == EAGAIN) {
			return (0);
		}
		if (res == -1 && errno == EPIPE) {
			libais_disconnect_request (conn_info);
			return (0);
		}
		if (res == -1) {
			printf ("ERRNO is %d\n", errno);
			assert (0); /* some other unhandled error here */
		}
		if (res + conn_info->byte_start != queue_item->mlen) {
			conn_info->byte_start += res;

			return (0);
		}

		/*
		 * Message sent, try sending another message
		 */
		queue_item_remove (outq);
		conn_info->byte_start = 0;
		free (queue_item->msg);
	} /* while queue not empty */

	if (queue_is_empty (outq)) {
		conn_info->events = POLLIN|POLLNVAL;
	}

	return (0);
}



struct res_overlay {
	mar_res_header_t header __attribute((aligned(8)));
	char buf[4096];
};

static void libais_deliver (struct conn_info *conn_info)
{
	int res;
	mar_req_header_t *header;
	int service;
	struct msghdr msg_recv;
	struct iovec iov_recv;
#ifdef OPENAIS_LINUX
	struct cmsghdr *cmsg;
	char cmsg_cred[CMSG_SPACE (sizeof (struct ucred))];
	struct ucred *cred;
	int on = 0;
#endif
	int send_ok = 0;
	int send_ok_joined = 0;
	struct iovec send_ok_joined_iovec;
	struct res_overlay res_overlay;

	msg_recv.msg_iov = &iov_recv;
	msg_recv.msg_iovlen = 1;
	msg_recv.msg_name = 0;
	msg_recv.msg_namelen = 0;
#ifndef OPENAIS_SOLARIS
	msg_recv.msg_flags = 0;

	if (conn_info->authenticated) {
		msg_recv.msg_control = 0;
		msg_recv.msg_controllen = 0;
	} else {
#ifdef OPENAIS_LINUX
		msg_recv.msg_control = (void *)cmsg_cred;
		msg_recv.msg_controllen = sizeof (cmsg_cred);
#else
		uid_t euid = -1;
		gid_t egid = -1;
		if (getpeereid(conn_info->fd, &euid, &egid) != -1 &&
		    (euid == 0 || egid == g_gid_valid)) {
			conn_info->authenticated = 1;
		}
		if (conn_info->authenticated == 0) {
			log_printf (LOG_LEVEL_SECURITY, "Connection not authenticated because gid is %d, expecting %d\n", egid, g_gid_valid);
		}
#endif
	}

#else	/* OPENAIS_SOLARIS */
	msg_recv.msg_accrights = 0;
	msg_recv.msg_accrightslen = 0;

	if (! conn_info->authenticated) {
#ifdef HAVE_GETPEERUCRED
		ucred_t *uc;
		uid_t euid = -1;
		gid_t egid = -1;
		if (getpeerucred(conn_info->fd, &uc) == 0) {
			euid = ucred_geteuid(uc);
			egid = ucred_getegid(uc);
			if ((euid == 0) || (egid == g_gid_valid)) {
				conn_info->authenticated = 1;
			}
			ucred_free(uc);
		}
		if (conn_info->authenticated == 0) {
			log_printf (LOG_LEVEL_SECURITY, "Connection not authenticated because gid is %d, expecting %d\n", (int)egid, g_gid_valid);
		}
#else
		log_printf (LOG_LEVEL_SECURITY, "Connection not authenticated "
				"because platform does not support "
				"authentication with sockets, continuing "
				"with a fake authentication\n");
		conn_info->authenticated = 1;
#endif
	}
#endif

	iov_recv.iov_base = &conn_info->inb[conn_info->inb_start];
	iov_recv.iov_len = (SIZEINB) - conn_info->inb_start;
	if (conn_info->inb_inuse == SIZEINB) {
		return;
	}

retry_recv:
	res = recvmsg (conn_info->fd, &msg_recv, MSG_NOSIGNAL);
	if (res == -1 && errno == EINTR) {
		goto retry_recv;
	} else
	if (res == -1 && errno != EAGAIN) {
		return;
	} else
	if (res == 0) {
#if defined(OPENAIS_SOLARIS) || defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
		/* On many OS poll never return POLLHUP or POLLERR.
		 * EOF is detected when recvmsg return 0.
		 */
  		libais_disconnect_request (conn_info);
#endif
		return;
	}

	/*
	 * Authenticate if this connection has not been authenticated
	 */
#ifdef OPENAIS_LINUX
	if (conn_info->authenticated == 0) {
		cmsg = CMSG_FIRSTHDR (&msg_recv);
		assert (cmsg);
		cred = (struct ucred *)CMSG_DATA (cmsg);
		if (cred) {
			if (cred->uid == 0 || cred->gid == g_gid_valid) {
				setsockopt(conn_info->fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof (on));
				conn_info->authenticated = 1;
			}
		}
		if (conn_info->authenticated == 0) {
			log_printf (LOG_LEVEL_SECURITY, "Connection not authenticated because gid is %d, expecting %d\n", cred->gid, g_gid_valid);
		}
	}
#endif
	/*
	 * Dispatch all messages received in recvmsg that can be dispatched
	 * sizeof (mar_req_header_t) needed at minimum to do any processing
	 */
	conn_info->inb_inuse += res;
	conn_info->inb_start += res;

	while (conn_info->inb_inuse >= sizeof (mar_req_header_t) && res != -1) {
		header = (mar_req_header_t *)&conn_info->inb[conn_info->inb_start - conn_info->inb_inuse];

		if (header->size > conn_info->inb_inuse) {
			break;
		}
		service = conn_info->service;

		/*
		 * If this service is in init phase, initialize service
		 * else handle message using service service
		 */
		if (service == SOCKET_SERVICE_INIT) {
			res = ais_init_service[header->id] (conn_info, header);
		} else  {
			/*
			 * Not an init service, but a standard service
			 */
			if (header->id < 0 || header->id > ais_service[service]->lib_service_count) {
				log_printf (LOG_LEVEL_SECURITY, "Invalid header id is %d min 0 max %d\n",
				header->id, ais_service[service]->lib_service_count);
				return ;
			}

			/*
			 * If flow control is required of the library handle, determine that
			 * openais is not in synchronization and that totempg has room available
			 * to queue a message, otherwise tell the library we are busy and to
			 * try again later
			 */
			send_ok_joined_iovec.iov_base = (char *)header;
			send_ok_joined_iovec.iov_len = header->size;
			send_ok_joined = totempg_groups_send_ok_joined (openais_group_handle,
				&send_ok_joined_iovec, 1);

			send_ok =
				(sync_primary_designated() == 1) && (
				(ais_service[service]->lib_service[header->id].flow_control == OPENAIS_FLOW_CONTROL_NOT_REQUIRED) ||
				((ais_service[service]->lib_service[header->id].flow_control == OPENAIS_FLOW_CONTROL_REQUIRED) &&
				(send_ok_joined) &&
				(sync_in_process() == 0)));

			if (send_ok) {
				ais_service[service]->lib_service[header->id].lib_handler_fn(conn_info, header);
			} else {

				/*
				 * Overload, tell library to retry
				 */
				res_overlay.header.size =
					ais_service[service]->lib_service[header->id].response_size;
				res_overlay.header.id =
					ais_service[service]->lib_service[header->id].response_id;
				res_overlay.header.error = SA_AIS_ERR_TRY_AGAIN;
				openais_conn_send_response (
					conn_info,
					&res_overlay,
					res_overlay.header.size);
			}
		}
		conn_info->inb_inuse -= header->size;
	} /* while */

	if (conn_info->inb_inuse == 0) {
		conn_info->inb_start = 0;
	} else
// BUG	if (connections[conn_info->fd].inb_start + connections[conn_info->fd].inb_inuse >= SIZEINB) {
	if (conn_info->inb_start >= SIZEINB) {
		/*
		 * If in buffer is full, move it back to start
		 */
		memmove (conn_info->inb,
			&conn_info->inb[conn_info->inb_start - conn_info->inb_inuse],
			sizeof (char) * conn_info->inb_inuse);
		conn_info->inb_start = conn_info->inb_inuse;
	}

	return;
}

static int poll_handler_libais_accept (
	poll_handle handle,
	int fd,
	int revent,
	void *data)
{
	socklen_t addrlen;
	struct sockaddr_un un_addr;
	int new_fd;
#ifdef OPENAIS_LINUX
	int on = 1;
#endif
	int res;

	addrlen = sizeof (struct sockaddr_un);

retry_accept:
	new_fd = accept (fd, (struct sockaddr *)&un_addr, &addrlen);
	if (new_fd == -1 && errno == EINTR) {
		goto retry_accept;
	}

	if (new_fd == -1) {
		log_printf (LOG_LEVEL_ERROR, "ERROR: Could not accept Library connection: %s\n", strerror (errno));
		return (0); /* This is an error, but -1 would indicate disconnect from poll loop */
	}

	totemip_nosigpipe(new_fd);
	res = fcntl (new_fd, F_SETFL, O_NONBLOCK);
	if (res == -1) {
		log_printf (LOG_LEVEL_ERROR, "Could not set non-blocking operation on library connection: %s\n", strerror (errno));
		close (new_fd);
		return (0); /* This is an error, but -1 would indicate disconnect from poll loop */
	}

	/*
	 * Valid accept
	 */

	/*
	 * Request credentials of sender provided by kernel
	 */
#ifdef OPENAIS_LINUX
	setsockopt(new_fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof (on));
#endif

	log_printf (LOG_LEVEL_DEBUG, "connection received from libais client %d.\n", new_fd);

	res = conn_info_create (new_fd);
	if (res != 0) {
		close (new_fd);
	}

	return (0);
}
/*
 * Exported functions
 */

int message_source_is_local(mar_message_source_t *source)
{
	int ret = 0;

	assert (source != NULL);
	if (source->nodeid == totempg_my_nodeid_get ()) {
		ret = 1;
	}
	return ret;
}

void message_source_set (
	mar_message_source_t *source,
	void *conn)
{
	assert ((source != NULL) && (conn != NULL));
	memset (source, 0, sizeof (mar_message_source_t));
	source->nodeid = totempg_my_nodeid_get ();
	source->conn = conn;
}

static void ipc_confchg_fn (
	enum totem_configuration_type configuration_type,
	unsigned int *member_list, int member_list_entries,
	unsigned int *left_list, int left_list_entries,
	unsigned int *joined_list, int joined_list_entries,
	struct memb_ring_id *ring_id)
{
}

void openais_ipc_init (
	void (*serialize_lock_fn) (void),
	void (*serialize_unlock_fn) (void),
	unsigned int gid_valid)
{
	int libais_server_fd;
	struct sockaddr_un un_addr;
	int res;

	ipc_serialize_lock_fn = serialize_lock_fn;

	ipc_serialize_unlock_fn = serialize_unlock_fn;

	/*
	 * Create socket for libais clients, name socket, listen for connections
	 */
	libais_server_fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (libais_server_fd == -1) {
		log_printf (LOG_LEVEL_ERROR ,"Cannot create libais client connections socket.\n");
		openais_exit_error (AIS_DONE_LIBAIS_SOCKET);
	};

	totemip_nosigpipe(libais_server_fd);
	res = fcntl (libais_server_fd, F_SETFL, O_NONBLOCK);
	if (res == -1) {
		log_printf (LOG_LEVEL_ERROR, "Could not set non-blocking operation on server socket: %s\n", strerror (errno));
		openais_exit_error (AIS_DONE_LIBAIS_SOCKET);
	}

#if !defined(OPENAIS_LINUX)
	unlink(socketname);
#endif
	memset (&un_addr, 0, sizeof (struct sockaddr_un));
	un_addr.sun_family = AF_UNIX;
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
	un_addr.sun_len = sizeof(struct sockaddr_un);
#endif
#if defined(OPENAIS_LINUX)
	strcpy (un_addr.sun_path + 1, socketname);
#else
	strcpy (un_addr.sun_path, socketname);
#endif

	res = bind (libais_server_fd, (struct sockaddr *)&un_addr, AIS_SUN_LEN(&un_addr));
	if (res) {
		log_printf (LOG_LEVEL_ERROR, "ERROR: Could not bind AF_UNIX: %s.\n", strerror (errno));
		openais_exit_error (AIS_DONE_LIBAIS_BIND);
	}
	listen (libais_server_fd, SERVER_BACKLOG);

        /*
         * Setup libais connection dispatch routine
         */
        poll_dispatch_add (aisexec_poll_handle, libais_server_fd,
                POLLIN, 0, poll_handler_libais_accept);

	g_gid_valid = gid_valid;

	/*
	 * Reset internal state of flow control when
	 * configuration change occurs
	 */
	res = totempg_groups_initialize (
		&ipc_handle,
		NULL,
		ipc_confchg_fn);
}


/*
 * Get the conn info private data
 */
void *openais_conn_private_data_get (void *conn)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	if (conn != NULL) {
		return ((void *)conn_info->private_data);
	} else {
		return NULL;
	}
}

/*
 * Get the conn info partner connection
 */
void *openais_conn_partner_get (void *conn)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	if (conn != NULL) {
		return ((void *)conn_info->conn_info_partner);
	} else {
		return NULL;
	}
}

int openais_conn_send_response (
	void *conn,
	void *msg,
	int mlen)
{
	struct queue *outq;
	char *cmsg;
	int res = 0;
	int queue_empty;
	struct outq_item *queue_item;
	struct outq_item queue_item_out;
	struct msghdr msg_send;
	struct iovec iov_send;
	char *msg_addr;
	struct conn_info *conn_info = (struct conn_info *)conn;

	if (conn_info == NULL) {
		return -1;
	}

	if (!libais_connection_active (conn_info)) {
		return (-1);
	}

	ipc_flow_control (conn_info);

	outq = &conn_info->outq;

	msg_send.msg_iov = &iov_send;
	msg_send.msg_name = 0;
	msg_send.msg_namelen = 0;
	msg_send.msg_iovlen = 1;
#ifndef OPENAIS_SOLARIS
	msg_send.msg_control = 0;
	msg_send.msg_controllen = 0;
	msg_send.msg_flags = 0;
#else
	msg_send.msg_accrights = 0;
	msg_send.msg_accrightslen = 0;
#endif

	if (queue_is_full (outq)) {
		/*
		 * Start a disconnect if we have not already started one
		 * and report that the outgoing queue is full
		 */
		log_printf (LOG_LEVEL_ERROR, "Library queue is full, disconnecting library connection.\n");
		libais_disconnect_request (conn_info);
		return (-1);
	}
	while (!queue_is_empty (outq)) {
		queue_item = queue_item_get (outq);
		msg_addr = (char *)queue_item->msg;
		msg_addr = &msg_addr[conn_info->byte_start];

		iov_send.iov_base = msg_addr;
		iov_send.iov_len = queue_item->mlen - conn_info->byte_start;

retry_sendmsg:
		res = sendmsg (conn_info->fd, &msg_send, MSG_NOSIGNAL);
		if (res == -1 && errno == EINTR) {
			goto retry_sendmsg;
		}
		if (res == -1 && errno == EAGAIN) {
			break; /* outgoing kernel queue full */
		}
		if (res == -1 && errno == EPIPE) {
			libais_disconnect_request (conn_info);
			return (0);
		}
		if (res == -1) {
			assert (0);
			break; /* some other error, stop trying to send message */
		}
		if (res + conn_info->byte_start != queue_item->mlen) {
			conn_info->byte_start += res;
			break;
		}

		/*
		 * Message sent, try sending another message
		 */
		queue_item_remove (outq);
		conn_info->byte_start = 0;
		free (queue_item->msg);
	} /* while queue not empty */

	res = -1;

	queue_empty = queue_is_empty (outq);
	/*
	 * Send request message
	 */
	if (queue_empty) {

		iov_send.iov_base = msg;
		iov_send.iov_len = mlen;
retry_sendmsg_two:
		res = sendmsg (conn_info->fd, &msg_send, MSG_NOSIGNAL);
		if (res == -1 && errno == EINTR) {
			goto retry_sendmsg_two;
		}
		if (res == -1 && errno == EAGAIN) {
			conn_info->byte_start = 0;
			conn_info->events = POLLIN|POLLNVAL;
		}
		if (res != -1) {
			if (res != mlen) {
				conn_info->byte_start += res;
				res = -1;
			} else {
				conn_info->byte_start = 0;
				conn_info->events = POLLIN|POLLNVAL;
			}
		}
	}

	/*
	 * If res == -1 , errrno == EAGAIN which means kernel queue full
	 */
	if (res == -1)  {
		cmsg = malloc (mlen);
		if (cmsg == 0) {
			log_printf (LOG_LEVEL_ERROR, "Library queue couldn't allocate a message, disconnecting library connection.\n");
			libais_disconnect_request (conn_info);
			return (-1);
		}
		queue_item_out.msg = cmsg;
		queue_item_out.mlen = mlen;
		memcpy (cmsg, msg, mlen);
		queue_item_add (outq, &queue_item_out);

		/*
		 * Send a pthread_kill to interrupt the poll syscall
		 * and start a new poll operation in the thread
		 */
		conn_info->events = POLLIN|POLLOUT|POLLNVAL;
		pthread_kill (conn_info->thread, SIGUSR1);
	}
	return (0);
}

void openais_ipc_flow_control_create (
	void *conn,
	unsigned int service,
	char *id,
	int id_len,
	void (*flow_control_state_set_fn) (void *conn, enum openais_flow_control_state),
	void *context)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	openais_flow_control_create (
		conn_info->flow_control_handle,
		service,
		id,
		id_len,
		flow_control_state_set_fn,
		context);	
	conn_info->conn_info_partner->flow_control_handle = conn_info->flow_control_handle;
}

void openais_ipc_flow_control_destroy (
	void *conn,
	unsigned int service,
	unsigned char *id,
	int id_len)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	openais_flow_control_destroy (
		conn_info->flow_control_handle,
		service,
		id,
		id_len);
}

void openais_ipc_flow_control_local_increment (
        void *conn)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	pthread_mutex_lock (&conn_info->flow_control_mutex);

	conn_info->flow_control_local_count++;

	pthread_mutex_unlock (&conn_info->flow_control_mutex);
}

void openais_ipc_flow_control_local_decrement (
        void *conn)
{
	struct conn_info *conn_info = (struct conn_info *)conn;

	pthread_mutex_lock (&conn_info->flow_control_mutex);

	conn_info->flow_control_local_count--;

	pthread_mutex_unlock (&conn_info->flow_control_mutex);
}
