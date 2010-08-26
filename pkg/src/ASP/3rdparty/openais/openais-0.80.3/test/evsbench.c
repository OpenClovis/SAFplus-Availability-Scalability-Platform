#define _BSD_SOURCE
/*
 * Copyright (c) 2004 MontaVista Software, Inc.
 * Copyright (c) 2006-2007 Red Hat, Inc.
 *
 * All rights reserved.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "saAis.h"
#include "evs.h"

volatile static int alarm_notice = 0;

int outstanding = 0;



void evs_deliver_fn (
	unsigned int nodeid,
	void *msg,
	int msg_len)
{
	outstanding--;
// printf ("Delivering message %s\n", msg);
}

void evs_confchg_fn (
	unsigned int *member_list, int member_list_entries,
	unsigned int *left_list, int left_list_entries,
	unsigned int *joined_list, int joined_list_entries)
{
	int i;

	printf ("CONFIGURATION CHANGE\n");
	printf ("--------------------\n");
	printf ("New configuration\n");
	for (i = 0; i < member_list_entries; i++) {
		printf ("%x\n", member_list[i]);
	}
	printf ("Members Left:\n");
	for (i = 0; i < left_list_entries; i++) {
		printf ("%x\n", left_list[i]);
	}
	printf ("Members Joined:\n");
	for (i = 0; i < joined_list_entries; i++) {
		printf ("%x\n", joined_list[i]);
	}
}

evs_callbacks_t callbacks = {
	evs_deliver_fn,
	evs_confchg_fn
};

struct evs_group groups[3] = {
	{ "key1" },
	{ "key2" },
	{ "key3" }
};

char buffer[200000];

struct iovec iov = {
	.iov_base = buffer,
	.iov_len = sizeof (buffer)
};

void evs_benchmark (evs_handle_t handle,
	int write_size)
{
	struct timeval tv1, tv2, tv_elapsed;
	evs_error_t result;
	int write_count = 0;

	/*
	 * Run benchmark for 10 seconds
	 */
	alarm (10);
	gettimeofday (&tv1, NULL);

	iov.iov_len = write_size;
	do {
		sprintf (buffer, "This is message %d\n", write_count);
		if (outstanding < 10) {
			result = evs_mcast_joined (handle, EVS_TYPE_AGREED, &iov, 1);

			if (result != EVS_ERR_TRY_AGAIN) {
				write_count += 1;
				outstanding++;
			}
		}
		result = evs_dispatch (handle, EVS_DISPATCH_ALL);
	} while (alarm_notice == 0);
	gettimeofday (&tv2, NULL);
	timersub (&tv2, &tv1, &tv_elapsed);

	printf ("%5d Writes ", write_count);
	printf ("%5d bytes per write ", write_size);
	printf ("%7.3f Seconds runtime ", 
		(tv_elapsed.tv_sec + (tv_elapsed.tv_usec / 1000000.0)));
	printf ("%9.3f TP/s ",
		((float)write_count) /  (tv_elapsed.tv_sec + (tv_elapsed.tv_usec / 1000000.0)));
	printf ("%7.3f MB/s.\n", 
		((float)write_count) * ((float)write_size) /  ((tv_elapsed.tv_sec + (tv_elapsed.tv_usec / 1000000.0)) * 1000000.0));

	alarm_notice = 0;
}

void sigalrm_handler (int num)
{
	alarm_notice = 1;
}

void sigintr_handler (int num)
{
	exit (1);
}

int main (void) {
	int size;
	int i;
	evs_error_t result;
	evs_handle_t handle;

	signal (SIGALRM, sigalrm_handler);
	signal (SIGINT, sigintr_handler);


	result = evs_initialize (&handle, &callbacks);
	printf ("Init result %d\n", result);
	result = evs_join (handle, groups, 3);
	printf ("Join result %d\n", result);
	result = evs_leave (handle, &groups[0], 1);
	printf ("Leave result %d\n", result);

	size = 1;

	for (i = 0; i < 225; i++) { /* number of repetitions - up to 50k */
		evs_benchmark (handle, size);
		/*
		 * Adjust count to 95% of previous count
		 * Adjust bytes to write per checkpoint up by 1500
		 */
		size += 1000;
	}
	return (0);
}
