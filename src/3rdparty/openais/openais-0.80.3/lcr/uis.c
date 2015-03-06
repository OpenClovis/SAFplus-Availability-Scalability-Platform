/*
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Author: Steven Dake (sdake@redhat.com)
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
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/types.h>
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
#include <pthread.h>
#include <sys/poll.h>
#include <string.h>

#define SERVER_BACKLOG 5

#if defined(OPENAIS_LINUX)
/* SUN_LEN is broken for abstract namespace 
 */
#define AIS_SUN_LEN(a) sizeof(*(a))

static char *socketname = "lcr.socket";
#else
#define AIS_SUN_LEN(a) SUN_LEN(a)

static char *socketname = "/var/run/lcr.socket";
#endif

static void uis_lcr_bind (int *server_fd)
{
	int fd;
	struct sockaddr_un un_addr;
	int res;

	/*
	 * Create socket for lcr clients, name socket, listen for connections
	 */
	fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		printf ("lcr_bind failed\n");
	};

#if !defined(OPENAIS_LINUX)
	unlink(socketname);
#endif
	memset (&un_addr, 0, sizeof (struct sockaddr_un));
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
	un_addr.sun_len = sizeof(struct sockaddr_un);
#endif
	un_addr.sun_family = AF_UNIX;
#if defined(OPENAIS_LINUX)
	strcpy (un_addr.sun_path + 1, socketname);
#else
	strcpy (un_addr.sun_path, socketname);
#endif

	res = bind (fd, (struct sockaddr *)&un_addr, AIS_SUN_LEN(&un_addr));
	if (res) {
		printf ("Could not bind AF_UNIX: %s\n", strerror (errno));
	}
	listen (fd, SERVER_BACKLOG);
	*server_fd = fd;
}

struct uis_commands {
	char *command;
	void (*cmd_handler) (char *);
};

void cmd1 (char *cmd) {
	printf ("cmd1 executed with cmd line %s\n", cmd);
}

struct uis_commands uis_commands[] = {
	{
		"cmd1", cmd1
	}
};

struct req_msg {
        int len;
        char msg[0];
};

static void lcr_uis_dispatch (int fd)
{
	struct req_msg header;
	char msg_contents[512];
	ssize_t readsize;

	/*
	 * TODO this doesn't handle short reads
	 */
	readsize = read (fd, &header, sizeof (header));
	if (readsize == -1) {
		return;
	}
	readsize = read (fd, msg_contents, sizeof (msg_contents));
	if (readsize == -1) {
		return;
	}

	printf ("msg contents %s\n", msg_contents);
}

static void *lcr_uis_server (void *data)
{
	struct pollfd ufds[2];
	struct sockaddr_un un_addr;
	socklen_t addrlen;
	int nfds = 1;
#ifdef OPENAIS_LINUX
	int on = 1;
#endif
	int res;

	/*
	 * Main acceptance and dispatch loop
	 */
	uis_lcr_bind (&ufds[0].fd);
	printf ("UIS server thread started %d\n", ufds[0].fd);
	ufds[0].events = POLLIN;
	ufds[1].events = POLLOUT;
	for (;;) {
		res = poll (ufds, nfds, -1);
		if (nfds == 1 && ufds[0].revents & POLLIN) {
			ufds[1].fd = accept (ufds[0].fd,
				(struct sockaddr *)&un_addr, &addrlen);
#ifdef OPENAIS_LINUX			
			setsockopt(ufds[1].fd, SOL_SOCKET, SO_PASSCRED,
				&on, sizeof (on));
#endif
			nfds = 2;		
		}
		if (ufds[0].revents & POLLIN) {
			lcr_uis_dispatch (ufds[1].fd);
		}
	}


	return 0;
}

__attribute__ ((constructor)) static int lcr_uis_ctors (void)
{
	pthread_t thread;

	pthread_create (&thread, NULL, lcr_uis_server, NULL);

	return (0);
}

