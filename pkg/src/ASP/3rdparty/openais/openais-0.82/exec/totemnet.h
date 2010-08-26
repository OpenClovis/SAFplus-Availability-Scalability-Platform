/*
 * Copyright (c) 2005 MontaVista Software, Inc.
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
#ifndef TOTEMNET_H_DEFINED
#define TOTEMNET_H_DEFINED

#include <sys/types.h>
#include <sys/socket.h>

#include "swab.h"
#include "totem.h"
#include "aispoll.h"

typedef unsigned int totemnet_handle;

#define TOTEMNET_NOFLUSH	0
#define TOTEMNET_FLUSH		1
/*
 * Totem Network interface - also does encryption/decryption
 * depends on poll abstraction, POSIX, IPV4
 */

/*
 * Create an instance
 */
extern int totemnet_initialize (
	poll_handle poll_handle,
	totemnet_handle *handle,
	struct totem_config *totem_config,
	int interface_no,
	void *context,

	void (*deliver_fn) (
		void *context,
		void *msg,
		int msg_len),

	void (*iface_change_fn) (
		void *context,
		struct totem_ip_address *iface_address));

extern int totemnet_processor_count_set (
	totemnet_handle handle,
	int processor_count);

extern int totemnet_token_send (
	totemnet_handle handle,
	struct iovec *iovec,
	int iov_len);

extern int totemnet_mcast_flush_send (
	totemnet_handle handle,
	struct iovec *iovec,
	unsigned int iov_len);

extern int totemnet_mcast_noflush_send (
	totemnet_handle handle,
	struct iovec *iovec,
	unsigned int iov_len);

extern int totemnet_recv_flush (totemnet_handle handle);

extern int totemnet_send_flush (totemnet_handle handle);

extern int totemnet_iface_check (totemnet_handle handle);

extern int totemnet_finalize (totemnet_handle handle);

extern void totemnet_net_mtu_adjust (struct totem_config *totem_config);

extern char *totemnet_iface_print (totemnet_handle handle);

extern int totemnet_iface_get (
	totemnet_handle handle,
	struct totem_ip_address *addr);

extern int totemnet_token_target_set (
	totemnet_handle handle,
	struct totem_ip_address *token_target);

extern struct totem_ip_address my_ip;

#endif /* TOTEMNET_H_DEFINED */
