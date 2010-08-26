/*
 * include/net/tipc/tipc_plugin_if.h: Include file for interface access by TIPC plugins
 * 
 * Copyright (c) 2003-2006, Ericsson AB
 * Copyright (c) 2005-2006, Wind River Systems
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
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
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NET_TIPC_BEARER_H_
#define _NET_TIPC_BEARER_H_

#ifdef __KERNEL__

#ifndef SOLARIS
#include <linux/tipc_config.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#else
#include <solaris/tipc_solaris.h>
#include <solaris/tipc_config.h>
#endif /* SOLARIS */


#define TIPC_MAX_BEARERS	8

#define TIPC_MEDIA_ID_INVALID	0
#define TIPC_MEDIA_ID_ETH	1

/**
 * struct tipc_media_addr - destination address used by TIPC bearers
 * @value: media-specific address info (opaque to TIPC) 
 * @media_id: TIPC media identifier
 * @broadcast: non-zero if address is a broadcast address
 */

struct tipc_media_addr {
        u8 value[20];
	u8 media_id;
        u8 broadcast;
};

/**
 * struct tipc_bearer - TIPC bearer info available to privileged users
 * @usr_handle: pointer to additional user-defined information about bearer
 * @mtu: max packet size bearer can support
 * @blocked: non-zero if bearer is blocked
 * @addr: media-specific address associated with bearer
 * @name: bearer name (format = media:interface)
 * @lock: spinlock for controlling access to bearer
 * 
 * Note: TIPC initializes "name" and "lock" fields; user is responsible for
 * initialization all other fields when a bearer is enabled.
 */


struct tipc_bearer {
	void *usr_handle;
	u32 mtu;
	int blocked;
	struct tipc_media_addr addr;
	char name[TIPC_MAX_BEARER_NAME];
	spinlock_t lock;
};

/**
 * struct tipc_media - TIPC media information available to privileged users
 * @type_id: TIPC media identifier
 * @name: media name
 * @priority: default link (and bearer) priority
 * @tolerance: default time (in ms) before declaring link failure
 * @window: default window (in packets) before declaring link congestion
 * @send_msg: routine which handles buffer transmission
 * @enable_bearer: routine which enables a bearer
 * @disable_bearer: routine which disables a bearer
 * @addr2str: routine which converts media address to string form
 * @str2addr: routine which converts media address from string form
 * @msg2addr: routine which converts media address to message header form
 * @addr2msg: routine which converts media address from message header form
 * @bcast_addr: media address used in broadcasting
 */
 
struct tipc_media {
	u8 media_id;
	char name[TIPC_MAX_MEDIA_NAME];
	u32 priority;
	u32 tolerance;
	u32 window;
	int (*send_msg)(struct sk_buff *buf, 
			struct tipc_bearer *b_ptr,
			struct tipc_media_addr *dest);
	int (*enable_bearer)(struct tipc_bearer *b_ptr);
	void (*disable_bearer)(struct tipc_bearer *b_ptr);
	int (*addr2str)(struct tipc_media_addr *a, char *str_buf, int str_size);
	int (*str2addr)(struct tipc_media_addr *a, char *str_buf);
        int (*msg2addr)(struct tipc_media_addr *a, u32 *msg_area);
        int (*addr2msg)(struct tipc_media_addr *a, u32 *msg_area);
	struct tipc_media_addr bcast_addr;
};


int  tipc_register_media(struct tipc_media *m_ptr);
void tipc_recv_msg(struct sk_buff *buf, struct tipc_bearer *tb_ptr);

int  tipc_block_bearer(const char *name);
void tipc_continue(struct tipc_bearer *tb_ptr); 

int tipc_enable_bearer(const char *bearer_name, u32 disc_domain, u32 priority);
int tipc_disable_bearer(const char *name);


#endif

#endif
