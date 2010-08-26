/*
 * Copyright (c) 2003-2005 MontaVista Software, Inc.
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
#ifndef TOTEMSRP_H_DEFINED
#define TOTEMSRP_H_DEFINED

#include "totem.h"
#include "aispoll.h"

typedef unsigned int totemsrp_handle;

/*
 * Totem Single Ring Protocol
 * depends on poll abstraction, POSIX, IPV4
 */

/*
 * Create a protocol instance
 */
int totemsrp_initialize (
	poll_handle poll_handle,
	totemsrp_handle *handle,
	struct totem_config *totem_config,

	void (*deliver_fn) (
		unsigned int nodeid,
		struct iovec *iovec,
		int iov_len,
		int endian_conversion_required),
	void (*confchg_fn) (
		enum totem_configuration_type configuration_type,
		unsigned int *member_list, int member_list_entries,
		unsigned int *left_list, int left_list_entries,
		unsigned int *joined_list, int joined_list_entries,
		struct memb_ring_id *ring_id));

void totemsrp_finalize (totemsrp_handle handle);

/*
 * Multicast a message
 */
int totemsrp_mcast (
	totemsrp_handle handle,
	struct iovec *iovec,
	int iov_len,
	int priority);

/*
 * Return number of available messages that can be queued
 */
int totemsrp_avail (totemsrp_handle handle);

int totemsrp_callback_token_create (
	totemsrp_handle handle,
	void **handle_out,
	enum totem_callback_token_type type,
	int delete,
	int (*callback_fn) (enum totem_callback_token_type type, void *),
	void *data);

void totemsrp_callback_token_destroy (
	totemsrp_handle handle,
	void **handle_out);

int totemsrp_new_msg_signal (totemsrp_handle handle);

extern void totemsrp_net_mtu_adjust (struct totem_config *totem_config);

extern int totemsrp_ifaces_get (
	totemsrp_handle handle,
	unsigned int nodeid,
	struct totem_ip_address *interfaces,
	char ***status,
	unsigned int *iface_count);

extern int totemsrp_my_nodeid_get (
	totemsrp_handle handle);

extern int totemsrp_my_family_get (
	totemsrp_handle handle);

extern int totemsrp_ring_reenable (
	totemsrp_handle handle);

#endif /* TOTEMSRP_H_DEFINED */
