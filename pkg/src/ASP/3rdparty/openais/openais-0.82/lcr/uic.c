/*
 * Copyright (c) 2006 Steven Dake (sdake@mvista.com)
 * Copyright (c) 2006 Sun Microsystems, Inc.
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

#if defined(OPENAIS_LINUX) || defined(OPENAIS_SOLARIS)
/* SUN_LEN is broken for abstract namespace 
 */
#define AIS_SUN_LEN(a) sizeof(*(a))
#else
#define AIS_SUN_LEN(a) SUN_LEN(a)
#endif

#ifdef OPENAIS_LINUX
static char *socketname = "lcr.socket";
#else
static char *socketname = "/var/run/lcr.socket";
#endif

int uic_connect (int *fd)
{
	int res;
	struct sockaddr_un addr;

	memset (&addr, 0, sizeof (struct sockaddr_un));
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
	addr.sun_len = sizeof(struct sockaddr_un);
#endif
	addr.sun_family = PF_UNIX;
#if defined(OPENAIS_LINUX)
	strcpy (addr.sun_path + 1, socketname);
#else
	strcpy (addr.sun_path, socketname);
#endif
	*fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (*fd == -1) {
		return -errno;
	}
	res = connect (*fd, (struct sockaddr *)&addr, AIS_SUN_LEN(&addr));
	if (res == -1) {
		return -errno;
	}
	return 0;
}

struct req_msg {
	int len;
	char msg[0];
};

int uic_msg_send (int fd, char *msg)
{
	struct msghdr msg_send;
	struct iovec iov_send[2];
	struct req_msg req_msg;
	int res;

	req_msg.len = strlen (msg) + 1;
	iov_send[0].iov_base = (void *)&req_msg;
	iov_send[0].iov_len = sizeof (struct req_msg);
	iov_send[1].iov_base = (void *)msg;
	iov_send[1].iov_len = req_msg.len;

	msg_send.msg_iov = iov_send;
	msg_send.msg_iovlen = 2;
	msg_send.msg_name = 0;
	msg_send.msg_namelen = 0;
#ifndef OPENAIS_SOLARIS
	msg_send.msg_control = 0;
	msg_send.msg_controllen = 0;
	msg_send.msg_flags = 0;
#else
	msg_send.msg_accrights = NULL;
	msg_send.msg_accrightslen = 0;
#endif

	retry_send:
	res = sendmsg (fd, &msg_send, 0);
	if (res == -1 && errno == EINTR) {
                goto retry_send;
        }
	if (res == -1) {
		res = -errno;
	}
	return (res);

}


int main (void)
{
	int client_fd;
	int res;

	res = uic_connect (&client_fd);
	if (res != 0) {
		printf ("Couldn't connect to live replacement service\n");
	}
	uic_msg_send (client_fd, "livereplace ckpt version 2");

	return 0;
}
