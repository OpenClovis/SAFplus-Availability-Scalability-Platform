/*
 * Copyright (c) 2004-2006 Mark Haverkamp
 * Copyright (c) 2004-2006 Open Source Development Lab
 * Copyright (c) 2006-2007 Red Hat, Inc.  
 * Copyright (c) 2006 Sun Microsystems, Inc.
 *
 * All rights reserved.
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
 * - Neither the name of the Open Source Development Lab nor the names of its
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

#define DUMP_CHAN_INFO
#define RECOVERY_EVENT_DEBUG LOG_LEVEL_DEBUG
#define RECOVERY_DEBUG LOG_LEVEL_DEBUG
#define CHAN_DEL_DEBUG LOG_LEVEL_DEBUG
#define CHAN_OPEN_DEBUG LOG_LEVEL_DEBUG
#define CHAN_UNLINK_DEBUG LOG_LEVEL_DEBUG
#define REMOTE_OP_DEBUG LOG_LEVEL_DEBUG
#define RETENTION_TIME_DEBUG LOG_LEVEL_DEBUG

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/hdb.h"
#include "../include/ipc_evt.h"
#include "../include/list.h"
#include "../include/queue.h"
#include "../lcr/lcr_comp.h"
#include "util.h"
#include "service.h"
#include "mempool.h"
#include "main.h"
#include "ipc.h"
#include "totempg.h"
#include "swab.h"
#include "print.h"
#include "tlist.h"
#include "timer.h"

/*
 * event instance structure. Contains information about the
 * active connection to the API library.
 *
 * esi_version:				Version that the library is running.
 * esi_open_chans:			list of open channels associated with this
 *							instance.  Used to clean up any data left
 *							allocated when the finalize is done.
 *							(event_svr_channel_open.eco_instance_entry)
 * esi_events:				list of pending events to be delivered on this
 *							instance (struct chan_event_list.cel_entry)
 * esi_queue_blocked:		non-zero if the delivery queue got too full
 *							and we're blocking new messages until we
 *							drain some of the queued messages.
 * esi_nevents:				Number of events in events lists to be sent.
 * esi_hdb:					Handle data base for open channels on this
 *							instance.  Used for a quick lookup of
 *							open channel data from a lib api message.
 */
struct libevt_pd {
	SaVersionT				esi_version;
	struct list_head		esi_open_chans;
	struct list_head		esi_events[SA_EVT_LOWEST_PRIORITY+1];
	int						esi_nevents;
	int						esi_queue_blocked;
	struct hdb_handle_database	esi_hdb;
};


enum evt_message_req_types {
	MESSAGE_REQ_EXEC_EVT_EVENTDATA = 0,
	MESSAGE_REQ_EXEC_EVT_CHANCMD = 1,
	MESSAGE_REQ_EXEC_EVT_RECOVERY_EVENTDATA = 2
};

static void lib_evt_open_channel(void *conn, void *message);
static void lib_evt_open_channel_async(void *conn, void *message);
static void lib_evt_close_channel(void *conn, void *message);
static void lib_evt_unlink_channel(void *conn, void *message);
static void lib_evt_event_subscribe(void *conn, void *message);
static void lib_evt_event_unsubscribe(void *conn, void *message);
static void lib_evt_event_publish(void *conn, void *message);
static void lib_evt_event_clear_retentiontime(void *conn, void *message);
static void lib_evt_event_data_get(void *conn, void *message);

static void evt_conf_change(
		enum totem_configuration_type configuration_type,
		unsigned int *member_list, int member_list_entries,
		unsigned int *left_list, int left_list_entries,
		unsigned int *joined_list, int joined_list_entries,
		struct memb_ring_id *ring_id);

static int evt_lib_init(void *conn);
static int evt_lib_exit(void *conn);
static int evt_exec_init(struct objdb_iface_ver0 *objdb);

/*
 * Recovery sync functions
 */
static void evt_sync_init(void);
static int evt_sync_process(void);
static void evt_sync_activate(void);
static void evt_sync_abort(void);

static void convert_event(void *msg);
static void convert_chan_packet(void *msg);

static struct openais_lib_handler evt_lib_service[] = {
	{
	.lib_handler_fn =		lib_evt_open_channel,
	.response_size =		sizeof(struct res_evt_channel_open),
	.response_id =			MESSAGE_RES_EVT_OPEN_CHANNEL,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_open_channel_async,
	.response_size =		sizeof(struct res_evt_channel_open),
	.response_id =			MESSAGE_RES_EVT_OPEN_CHANNEL,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_close_channel,
	.response_size =		sizeof(struct res_evt_channel_close),
	.response_id =			MESSAGE_RES_EVT_CLOSE_CHANNEL,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_unlink_channel,
	.response_size =		sizeof(struct res_evt_channel_unlink),
	.response_id =			MESSAGE_RES_EVT_UNLINK_CHANNEL,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_event_subscribe,
	.response_size =		sizeof(struct res_evt_event_subscribe),
	.response_id =			MESSAGE_RES_EVT_SUBSCRIBE,
	.flow_control =			OPENAIS_FLOW_CONTROL_NOT_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_event_unsubscribe,
	.response_size =		sizeof(struct res_evt_event_unsubscribe),
	.response_id =			MESSAGE_RES_EVT_UNSUBSCRIBE,
	.flow_control =			OPENAIS_FLOW_CONTROL_NOT_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_event_publish,
	.response_size =		sizeof(struct res_evt_event_publish),
	.response_id =			MESSAGE_RES_EVT_PUBLISH,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_event_clear_retentiontime,
	.response_size =		sizeof(struct res_evt_event_clear_retentiontime),
	.response_id =			MESSAGE_RES_EVT_CLEAR_RETENTIONTIME,
	.flow_control =			OPENAIS_FLOW_CONTROL_REQUIRED
	},
	{
	.lib_handler_fn =		lib_evt_event_data_get,
	.response_size =		sizeof(struct lib_event_data),
	.response_id =			MESSAGE_RES_EVT_EVENT_DATA,
	.flow_control =			OPENAIS_FLOW_CONTROL_NOT_REQUIRED
	},
};


static void evt_remote_evt(void *msg, unsigned int nodeid);
static void evt_remote_recovery_evt(void *msg, unsigned int nodeid);
static void evt_remote_chan_op(void *msg, unsigned int nodeid);

static struct openais_exec_handler evt_exec_service[] = {
	{
		.exec_handler_fn		= evt_remote_evt,
		.exec_endian_convert_fn = convert_event
	},
	{
		.exec_handler_fn		= evt_remote_chan_op,
		.exec_endian_convert_fn = convert_chan_packet
	},
	{
		.exec_handler_fn		= evt_remote_recovery_evt,
		.exec_endian_convert_fn = convert_event
	}
};

struct openais_service_handler evt_service_handler = {
	.name						=
								(unsigned char*)"openais event service B.01.01",
	.id							= EVT_SERVICE,
	.private_data_size			= sizeof (struct libevt_pd),
	.flow_control				= OPENAIS_FLOW_CONTROL_NOT_REQUIRED,
	.lib_init_fn				= evt_lib_init,
	.lib_exit_fn				= evt_lib_exit,
	.lib_service				= evt_lib_service,
	.lib_service_count			= sizeof(evt_lib_service) / sizeof(struct openais_lib_handler),
	.exec_init_fn				= evt_exec_init,
	.exec_service				= evt_exec_service,
	.exec_service_count		= sizeof(evt_exec_service) / sizeof(struct openais_exec_handler),
	.exec_dump_fn				= NULL,
	.confchg_fn					= evt_conf_change,
	.sync_init					= evt_sync_init,
	.sync_process				= evt_sync_process,
	.sync_activate				= evt_sync_activate,
	.sync_abort					= evt_sync_abort
};

static struct openais_service_handler *evt_get_handler_ver0(void);

static struct openais_service_handler_iface_ver0 evt_service_handler_iface = {
	.openais_get_service_handler_ver0		= evt_get_handler_ver0
};

static struct lcr_iface openais_evt_ver0[1] = {
	{
		.name					= "openais_evt",
		.version				= 0,
		.versions_replace		= 0,
		.versions_replace_count = 0,
		.dependencies			= 0,
		.dependency_count		= 0,
		.constructor			= NULL,
		.destructor				= NULL,
		.interfaces				= NULL,
	}
};

static struct lcr_comp evt_comp_ver0 = {
	.iface_count			= 1,
	.ifaces					= openais_evt_ver0
};

static struct openais_service_handler *evt_get_handler_ver0(void)
{
	return (&evt_service_handler);
}

__attribute__ ((constructor)) static void evt_comp_register (void) {
	lcr_interfaces_set (&openais_evt_ver0[0], &evt_service_handler_iface);

	lcr_component_register (&evt_comp_ver0);
}

/*
 * MESSAGE_REQ_EXEC_EVT_CHANCMD
 *
 * Used for various event related operations.
 *
 */
enum evt_chan_ops {
	EVT_OPEN_CHAN_OP,		/* chc_chan */
	EVT_CLOSE_CHAN_OP,		/* chc_close_unlink_chan */
	EVT_UNLINK_CHAN_OP,		/* chc_close_unlink_chan */
	EVT_CLEAR_RET_OP,		/* chc_event_id */
	EVT_SET_ID_OP,			/* chc_set_id */
	EVT_CONF_DONE,			/* no data used */
	EVT_OPEN_COUNT,			/* chc_set_opens */
	EVT_OPEN_COUNT_DONE		/* no data used */
};

/*
 * Used during recovery to set the next issued event ID
 * based on the highest ID seen by any of the members
 */
struct evt_set_id {
	mar_uint32_t		chc_nodeid __attribute__((aligned(8)));
	mar_uint64_t		chc_last_id __attribute__((aligned(8)));
};

/*
 * For set open count used during recovery to syncronize all nodes
 *
 *	chc_chan_name:		Channel name.
 *	chc_open_count:		number of local opens of this channel.
 */
struct evt_set_opens {
	mar_name_t		chc_chan_name __attribute__((aligned(8)));
	mar_uint32_t		chc_open_count __attribute__((aligned(8)));
};

/*
 * Used to communicate channel to close or unlink.
 */
#define EVT_CHAN_ACTIVE 0
struct evt_close_unlink_chan {
	mar_name_t		chcu_name __attribute__((aligned(8)));
	mar_uint64_t		chcu_unlink_id __attribute__((aligned(8)));
};

struct open_chan_req {
	mar_name_t		ocr_name __attribute__((aligned(8)));
	mar_uint64_t		ocr_serial_no __attribute__((aligned(8)));
};

/*
 * Sent via MESSAGE_REQ_EXEC_EVT_CHANCMD
 *
 * chc_head:	Request head
 * chc_op:		Channel operation (open, close, clear retentiontime)
 * u:			union of operation options.
 */
struct req_evt_chan_command {
	mar_req_header_t	chc_head __attribute__((aligned(8)));
	mar_uint32_t		chc_op __attribute__((aligned(8)));
	union {
		struct open_chan_req		chc_chan __attribute__((aligned(8)));
		mar_evteventid_t		chc_event_id __attribute__((aligned(8)));
		struct evt_set_id		chc_set_id __attribute__((aligned(8)));
		struct evt_set_opens		chc_set_opens __attribute__((aligned(8)));
		struct evt_close_unlink_chan	chcu __attribute__((aligned(8)));
	} u;
};

/*
 * list of all retained events
 *		struct event_data
 */
static DECLARE_LIST_INIT(retained_list);

/*
 * list of all event channel information
 *		struct event_svr_channel_instance
 */
static DECLARE_LIST_INIT(esc_head);

/*
 * list of all unlinked event channel information
 *		struct event_svr_channel_instance
 */
static DECLARE_LIST_INIT(esc_unlinked_head);

/*
 * Track the state of event service recovery.
 *
 *	evt_recovery_complete:			Normal operational mode
 *
 *	evt_send_event_id:				Node is sending known last
 *									event IDs.
 *
 *	evt_send_open_count:			Node is sending its open
 *									Channel information.
 *
 *	evt_wait_open_count_done:		Node is done sending open channel data and
 *									is waiting for the other nodes to finish.
 *
 *	evt_send_retained_events:		Node is sending retained event data.
 *
 *	evt_send_retained_events_done:	Node is sending done message.
 *
 *	evt_wait_send_retained_events:	Node is waiting for other nodes to
 *									finish sending retained event data.
 */
enum recovery_phases {
	evt_recovery_complete,
	evt_send_event_id,
	evt_send_open_count,
	evt_wait_open_count_done,
	evt_send_retained_events,
	evt_send_retained_events_done,
	evt_wait_send_retained_events
};

/*
 * Global varaibles used by the event service
 *
 * base_id_top:			upper bits of next event ID to assign
 * base_id:				Lower bits of Next event ID to assign
 * my_node_id:			My cluster node id
 * checked_in:			keep track during config change.
 * recovery_node:		True if we're the recovery node. i.e. the
 *						node that sends the retained events.
 * next_retained:		pointer to next retained message to send
 *						during recovery.
 * next_chan:			pointer to next channel to send during recovery.
 * recovery_phase:		Indicates what recovery is taking place.
 * left_member_count:	How many left this configuration.
 * left_member_list:	Members that left this config
 * joined_member_count:	How many joined this configuration.
 * joined_member_list:	Members that joined this config
 * total_member_count:	how many members in this cluster
 * current_member_list:	Total membership this config
 * trans_member_count:	Node count in transitional membership
 * trans_member_list:	Total membership from the transitional membership
 * add_count:			count of joined members used for sending event id
 *						recovery data.
 * add_list:			pointer to joined list used for sending event id
 *						recovery data.
 * processed_open_counts:	Flag used to coordinate clearing open
 *						channel counts for config change recovery.
 *
 */

#define BASE_ID_MASK 0xffffffffLL
static mar_evteventid_t	base_id = 1;
static mar_evteventid_t	base_id_top = 0;
static mar_uint32_t		my_node_id = 0;
static int				checked_in = 0;
static int				recovery_node = 0;
static struct list_head *next_retained = 0;
static struct list_head *next_chan = 0;
static enum recovery_phases recovery_phase = evt_recovery_complete;
static int				left_member_count = 0;
static unsigned int *left_member_list = 0;
static int				joined_member_count = 0;
static unsigned int *joined_member_list = 0;
static int				total_member_count = 0;
static unsigned int *current_member_list = 0;
static int				trans_member_count = 0;
static unsigned int *trans_member_list = 0;
static int				add_count = 0;
static unsigned int *add_list = 0;
static int				processed_open_counts = 0;

/*
 * Structure to track pending channel open requests.
 *	ocp_async:			1 for async open
 *	ocp_invocation:		invocation for async open
 *	ocp_chan_name:		requested channel
 *	ocp_conn:			conn for returning to the library.
 *	ocp_open_flags:		channel open flags
 *	ocp_timer_handle:	timer handle for sync open
 *	ocp_serial_no:		Identifier for the request
 *	ocp_entry:			list entry for pending open list.
 */
struct open_chan_pending {
	int					ocp_async;
	mar_invocation_t	ocp_invocation;
	mar_name_t			ocp_chan_name;
	void				*ocp_conn;
	mar_evtchannelopenflags_t	ocp_open_flag;
	timer_handle		ocp_timer_handle;
	uint64_t			ocp_c_handle;
	uint64_t			ocp_serial_no;
	struct list_head	ocp_entry;
};
static uint64_t	open_serial_no = 0;

/*
 * code to indicate that the open request has timed out. The
 * invocation data element is used for this code since it is
 * only used by the open async call which cannot have a timeout.
 */
#define OPEN_TIMED_OUT 0x5a6b5a6b5a6b5a6bLLU

/*
 * list of pending channel opens
 */
static DECLARE_LIST_INIT(open_pending);
static void chan_open_timeout(void *data);

/*
 * Structure to track pending channel unlink requests.
 * ucp_unlink_id:		unlink ID of unlinked channel.
 * ucp_conn:			conn for returning to the library.
 * ucp_entry:			list entry for pending unlink list.
 */
struct unlink_chan_pending {
	uint64_t			ucp_unlink_id;
	void				*ucp_conn;
	struct list_head	ucp_entry;
};

/*
 * list of pending unlink requests
 */
static DECLARE_LIST_INIT(unlink_pending);

/*
 * Structure to track pending retention time clear requests.
 * rtc_event_id:		event ID to clear.
 * rtc_conn:			conn for returning to the library.
 * rtc_entry:			list entry for pending clear list.
 */
struct retention_time_clear_pending {
	mar_evteventid_t		rtc_event_id;
	void				*rtc_conn;
	struct list_head	rtc_entry;
};

/*
 * list of pending clear requests.
 */
static DECLARE_LIST_INIT(clear_pending);

/*
 * Next unlink ID
 */
static uint64_t	base_unlink_id = 0;
inline uint64_t
next_chan_unlink_id()
{
	uint64_t uid = my_node_id;
	uid = (uid << 32ULL) | base_unlink_id;
	base_unlink_id = (base_unlink_id + 1ULL) & BASE_ID_MASK;
	return uid;
}

#define min(a,b) ((a) < (b) ? (a) : (b))

/*
 * Throttle event delivery to applications to keep
 * the exec from using too much memory if the app is
 * slow to process its events.
 */
#define MAX_EVT_DELIVERY_QUEUE	1000
#define MIN_EVT_QUEUE_RESUME	(MAX_EVT_DELIVERY_QUEUE / 2)

static unsigned int evt_delivery_queue_size = MAX_EVT_DELIVERY_QUEUE;
static unsigned int evt_delivery_queue_resume = MIN_EVT_QUEUE_RESUME;


#define LOST_PUB "EVENT_SERIVCE"
#define LOST_CHAN "LOST EVENT"
/*
 * Event to send when the delivery queue gets too full
 */
char lost_evt[] = SA_EVT_LOST_EVENT;
static int dropped_event_size;
static struct event_data *dropped_event;
struct evt_pattern {
	mar_evt_event_pattern_t	pat;
	char	str[sizeof(lost_evt)];
};
static struct evt_pattern dropped_pattern = {
		.pat	=	{
					sizeof(lost_evt),
					sizeof(lost_evt),
					(SaUint8T *) &dropped_pattern.str[0]},
		.str = {SA_EVT_LOST_EVENT}
};

mar_name_t lost_chan = {
	.value = LOST_CHAN,
	.length = sizeof(LOST_CHAN)
};

mar_name_t dropped_publisher = {
	.value = LOST_PUB,
	.length = sizeof(LOST_PUB)
};

struct event_svr_channel_open;
struct event_svr_channel_subscr;

struct open_count {
	mar_uint32_t	oc_node_id;
	int32_t			oc_open_count;
};

/*
 * Structure to contain global channel releated information
 *
 * esc_channel_name:	The name of this channel.
 * esc_total_opens:		The total number of opens on this channel including
 *						other nodes.
 * esc_local_opens:		The number of opens on this channel for this node.
 * esc_oc_size:			The total number of entries in esc_node_opens;
 * esc_node_opens:		list of node IDs and how many opens are associated.
 * esc_retained_count:	How many retained events for this channel
 * esc_open_chans:		list of opens of this channel.
 *						(event_svr_channel_open.eco_entry)
 * esc_entry:			links to other channels. (used by esc_head)
 * esc_unlink_id:		If non-zero, then the channel has been marked
 *						for unlink.  This unlink ID is used to
 *						mark events still associated with current openings
 *						so they get delivered to the proper recipients.
 */
struct event_svr_channel_instance {
	mar_name_t				esc_channel_name;
	int32_t				esc_total_opens;
	int32_t				esc_local_opens;
	uint32_t			esc_oc_size;
	struct open_count	*esc_node_opens;
	uint32_t			esc_retained_count;
	struct list_head	esc_open_chans;
	struct list_head	esc_entry;
	uint64_t			esc_unlink_id;
};

/*
 * Has the event data in the correct format to send to the library API
 * with aditional field for accounting.
 *
 * ed_ref_count:		how many other strutures are referencing.
 * ed_retained:			retained event list.
 * ed_timer_handle:		Timer handle for retained event expiration.
 * ed_delivered:		arrays of open channel pointers that this event
 *						has been delivered to. (only used for events
 *						with a retention time).
 * ed_delivered_count:	Number of entries available in ed_delivered.
 * ed_delivered_next:	Next free spot in ed_delivered
 * ed_my_chan:			pointer to the global channel instance associated
 *						with this event.
 * ed_event:			The event data formatted to be ready to send.
 */
struct event_data {
	uint32_t							ed_ref_count;
	struct list_head					ed_retained;
	timer_handle						ed_timer_handle;
	struct event_svr_channel_open		**ed_delivered;
	uint32_t							ed_delivered_count;
	uint32_t							ed_delivered_next;
	struct event_svr_channel_instance	*ed_my_chan;
	struct lib_event_data				ed_event;
};

/*
 * Contains a list of pending events to be delivered to a subscribed
 * application.
 *
 * cel_chan_handle:	associated library channel handle
 * cel_sub_id:		associated library subscription ID
 * cel_event:		event structure to deliver.
 * cel_entry:		list of pending events
 *					(struct event_server_instance.esi_events)
 */
struct chan_event_list {
	uint64_t			cel_chan_handle;
	uint32_t			cel_sub_id;
	struct event_data*	cel_event;
	struct list_head	cel_entry;
};

/*
 * Contains information about each open for a given channel
 *
 * eco_flags:			How the channel was opened.
 * eco_lib_handle:		channel handle in the app.  Used for event delivery.
 * eco_my_handle:		the handle used to access this data structure.
 * eco_channel:			Pointer to global channel info.
 * eco_entry:			links to other opeinings of this channel.
 * eco_instance_entry:	links to other channel opeinings for the
 *						associated server instance.
 * eco_subscr:			head of list of sbuscriptions for this channel open.
 *						(event_svr_channel_subscr.ecs_entry)
 * eco_conn:			refrence to EvtInitialize who owns this open.
 */
struct event_svr_channel_open {
	uint8_t								eco_flags;
	uint64_t							eco_lib_handle;
	uint32_t							eco_my_handle;
	struct event_svr_channel_instance	*eco_channel;
	struct list_head					eco_entry;
	struct list_head					eco_instance_entry;
	struct list_head					eco_subscr;
	void								*eco_conn;
};

/*
 * Contains information about each channel subscription
 *
 * ecs_open_chan:		Link to our open channel.
 * ecs_sub_id:			Subscription ID.
 * ecs_filter_count:	number of filters in ecs_filters
 * ecs_filters:			filters for determining event delivery.
 * ecs_entry:			Links to other subscriptions to this channel opening.
 */
struct event_svr_channel_subscr {
	struct event_svr_channel_open	*ecs_open_chan;
	uint32_t						ecs_sub_id;
	mar_evt_event_filter_array_t			*ecs_filters;
	struct list_head				ecs_entry;
};


/*
 * Member node data
 * mn_node_info:		cluster node info from membership
 * mn_last_msg_id:		last seen message ID for this node
 * mn_started:			Indicates that event service has started
 *						on this node.
 * mn_next:				pointer to the next node in the hash chain.
 * mn_entry:			List of all nodes.
 */
struct member_node_data {
	unsigned int		mn_nodeid;
	SaClmClusterNodeT	mn_node_info;
	mar_evteventid_t		mn_last_msg_id;
	mar_uint32_t		mn_started;
	struct member_node_data	*mn_next;
	struct list_head	mn_entry;
};
DECLARE_LIST_INIT(mnd);
/*
 * Take the filters we received from the application via the library and
 * make them into a real mar_evt_event_filter_array_t
 */
static SaAisErrorT evtfilt_to_aisfilt(struct req_evt_event_subscribe *req,
		mar_evt_event_filter_array_t **evtfilters)
{

	mar_evt_event_filter_array_t *filta =
			(mar_evt_event_filter_array_t *)req->ics_filter_data;
	mar_evt_event_filter_array_t *filters;
	mar_evt_event_filter_t *filt = (void *)filta + sizeof(mar_evt_event_filter_array_t);
	SaUint8T *str = (void *)filta + sizeof(mar_evt_event_filter_array_t) +
			(sizeof(mar_evt_event_filter_t) * filta->filters_number);
	int i;
	int j;

	filters = malloc(sizeof(mar_evt_event_filter_array_t));
	if (!filters) {
		return SA_AIS_ERR_NO_MEMORY;
	}

	filters->filters_number = filta->filters_number;
	filters->filters = malloc(sizeof(mar_evt_event_filter_t) *
				filta->filters_number);
	if (!filters->filters) {
			free(filters);
			return SA_AIS_ERR_NO_MEMORY;
	}

	for (i = 0; i < filters->filters_number; i++) {
		filters->filters[i].filter.pattern =
			malloc(filt[i].filter.pattern_size);

		if (!filters->filters[i].filter.pattern) {
				for (j = 0; j < i; j++) {
						free(filters->filters[j].filter.pattern);
				}
				free(filters->filters);
				free(filters);
				return SA_AIS_ERR_NO_MEMORY;
		}
		filters->filters[i].filter.pattern_size =
			filt[i].filter.pattern_size;
		filters->filters[i].filter.allocated_size =
			filt[i].filter.pattern_size;
		memcpy(filters->filters[i].filter.pattern,
				str, filters->filters[i].filter.pattern_size);
		filters->filters[i].filter_type = filt[i].filter_type;
		str += filters->filters[i].filter.pattern_size;
	}

	*evtfilters = filters;

	return SA_AIS_OK;
}

/*
 * Free up filter data
 */
static void free_filters(mar_evt_event_filter_array_t *fp)
{
	int i;

	for (i = 0; i < fp->filters_number; i++) {
		free(fp->filters[i].filter.pattern);
	}

	free(fp->filters);
	free(fp);
}

/*
 * Look up a channel in the global channel list
 */
static struct event_svr_channel_instance *
find_channel(mar_name_t *chan_name, uint64_t unlink_id)
{
	struct list_head *l, *head;
	struct event_svr_channel_instance *eci;

	/*
	 * choose which list to look through
	 */
	if (unlink_id == EVT_CHAN_ACTIVE) {
		head = &esc_head;
	} else {
		head = &esc_unlinked_head;
	}

	for (l = head->next; l != head; l = l->next) {

		eci = list_entry(l, struct event_svr_channel_instance, esc_entry);
		if (!mar_name_match(chan_name, &eci->esc_channel_name)) {
			continue;
		} else if (unlink_id != eci->esc_unlink_id) {
			continue;
		}
		return eci;
	}
	return 0;
}

/*
 * Find the last unlinked version of a channel.
 */
static struct event_svr_channel_instance *
find_last_unlinked_channel(mar_name_t *chan_name)
{
	struct list_head *l;
	struct event_svr_channel_instance *eci;

	/*
	 * unlinked channels are added to the head of the list
	 * so the first one we see is the last one added.
	 */
	for (l = esc_unlinked_head.next; l != &esc_unlinked_head; l = l->next) {

		eci = list_entry(l, struct event_svr_channel_instance, esc_entry);
		if (!mar_name_match(chan_name, &eci->esc_channel_name)) {
			continue;
		}
	}
	return 0;
}

/*
 * Create and initialize a channel instance structure
 */
static struct event_svr_channel_instance *create_channel(mar_name_t *cn)
{
	struct event_svr_channel_instance *eci;
	eci = (struct event_svr_channel_instance *) malloc(sizeof(*eci));
	if (!eci) {
		return (eci);
	}

	memset(eci, 0, sizeof(*eci));
	list_init(&eci->esc_entry);
	list_init(&eci->esc_open_chans);
	eci->esc_oc_size = total_member_count;
	eci->esc_node_opens =
			malloc(sizeof(struct open_count) * total_member_count);
	if (!eci->esc_node_opens) {
		free(eci);
		return 0;
	}
	memset(eci->esc_node_opens, 0,
			sizeof(struct open_count) * total_member_count);
	eci->esc_channel_name = *cn;
	eci->esc_channel_name.value[eci->esc_channel_name.length] = '\0';
	list_add(&eci->esc_entry, &esc_head);

	return eci;
}


/*
 * Make sure that the list of nodes is large enough to hold the whole
 * membership
 */
static int check_open_size(struct event_svr_channel_instance *eci)
{
	if (total_member_count > eci->esc_oc_size) {
		eci->esc_node_opens = realloc(eci->esc_node_opens,
							sizeof(struct open_count) * total_member_count);
		if (!eci->esc_node_opens) {
			log_printf(LOG_LEVEL_WARNING,
					"Memory error realloc of node list\n");
			return -1;
		}
		memset(&eci->esc_node_opens[eci->esc_oc_size], 0,
			sizeof(struct open_count) *
					(total_member_count - eci->esc_oc_size));
		eci->esc_oc_size = total_member_count;
	}
	return 0;
}

/*
 * Find the specified node ID in the node list of the channel.
 * If it's not in the list, add it.
 */
static struct open_count* find_open_count(
		struct event_svr_channel_instance *eci,
		mar_uint32_t node_id)
{
	int i;

	for (i = 0; i < eci->esc_oc_size; i++) {
		if (eci->esc_node_opens[i].oc_node_id == 0) {
			eci->esc_node_opens[i].oc_node_id = node_id;
			eci->esc_node_opens[i].oc_open_count = 0;
		}
		if (eci->esc_node_opens[i].oc_node_id == node_id) {
			return &eci->esc_node_opens[i];
		}
	}
	log_printf(LOG_LEVEL_DEBUG,
			"find_open_count: node id %s not found\n",
			totempg_ifaces_print (node_id));
	return 0;
}

static void dump_chan_opens(struct event_svr_channel_instance *eci)
{
	int i;
	log_printf(LOG_LEVEL_NOTICE,
			"Channel %s, total %d, local %d\n",
			eci->esc_channel_name.value,
			eci->esc_total_opens,
			eci->esc_local_opens);
	for (i = 0; i < eci->esc_oc_size; i++) {
		if (eci->esc_node_opens[i].oc_node_id == 0) {
			break;
		}
		log_printf(LOG_LEVEL_NOTICE, "Node %s, count %d\n",
			totempg_ifaces_print (eci->esc_node_opens[i].oc_node_id),
			eci->esc_node_opens[i].oc_open_count);
	}
}

#ifdef DUMP_CHAN_INFO
/*
 * Scan the list of channels and dump the open count info.
 */
static void dump_all_chans()
{
	struct list_head *l;
	struct event_svr_channel_instance *eci;

	for (l = esc_head.next; l != &esc_head; l = l->next) {
		eci = list_entry(l, struct event_svr_channel_instance, esc_entry);
		dump_chan_opens(eci);

	}
}
#endif

/*
 * Scan the list of channels and zero out the open counts
 */
static void zero_chan_open_counts()
{
	struct list_head *l;
	struct event_svr_channel_instance *eci;
	int i;

	for (l = esc_head.next; l != &esc_head; l = l->next) {
		eci = list_entry(l, struct event_svr_channel_instance, esc_entry);
		for (i = 0; i < eci->esc_oc_size; i++) {
			if (eci->esc_node_opens[i].oc_node_id == 0) {
				break;
			}
			eci->esc_node_opens[i].oc_open_count = 0;
		}
		eci->esc_total_opens = 0;
	}
}
/*
 * Replace the current open count for a node with the specified value.
 */
static int set_open_count(struct event_svr_channel_instance *eci,
		mar_uint32_t node_id, uint32_t open_count)
{
	struct open_count *oc;
	int i;

	if ((i = check_open_size(eci)) != 0) {
		return i;
	}

	oc = find_open_count(eci, node_id);
	if (oc) {
		log_printf(RECOVERY_DEBUG,
			"Set count: Chan %s for node %s, was %d, now %d\n",
			eci->esc_channel_name.value, totempg_ifaces_print (node_id),
			oc->oc_open_count, open_count);

		eci->esc_total_opens -= oc->oc_open_count;
		eci->esc_total_opens += open_count;
		oc->oc_open_count = open_count;
		return 0;
	}
	return -1;
}

/*
 * Increment the open count for the specified node.
 */
static int inc_open_count(struct event_svr_channel_instance *eci,
		mar_uint32_t node_id)
{

	struct open_count *oc;
	int i;

	if ((i = check_open_size(eci)) != 0) {
		return i;
	}

	if (node_id == my_node_id) {
		eci->esc_local_opens++;
	}
	oc = find_open_count(eci, node_id);
	if (oc) {
		eci->esc_total_opens++;
		oc->oc_open_count++;
		return 0;
	}
	return -1;
}

/*
 * Decrement the open count for the specified node in the
 * specified channel.
 */
static int dec_open_count(struct event_svr_channel_instance *eci,
		mar_uint32_t node_id)
{

	struct open_count *oc;
	int i;

	if ((i = check_open_size(eci)) != 0) {
		return i;
	}

	if (node_id == my_node_id) {
		eci->esc_local_opens--;
	}
	oc = find_open_count(eci, node_id);
	if (oc) {
		eci->esc_total_opens--;
		oc->oc_open_count--;
		if ((eci->esc_total_opens < 0) || (oc->oc_open_count < 0)) {
			log_printf(LOG_LEVEL_ERROR, "Channel open decrement error\n");
			dump_chan_opens(eci);
		}
		return 0;
	}
	return -1;
}


/*
 * Remove a channel and free its memory if it's not in use anymore.
 */
static void delete_channel(struct event_svr_channel_instance *eci)
{

	log_printf(CHAN_DEL_DEBUG,
			"Called Delete channel %s t %d, l %d, r %d\n",
			eci->esc_channel_name.value,
			eci->esc_total_opens, eci->esc_local_opens,
			eci->esc_retained_count);
	/*
	 * If no one has the channel open anywhere and there are no unexpired
	 * retained events for this channel, and it has been marked for deletion
	 * by an unlink, then it is OK to delete the data structure.
	 */
	if ((eci->esc_retained_count == 0)  && (eci->esc_total_opens == 0) &&
			(eci->esc_unlink_id != EVT_CHAN_ACTIVE)) {
		log_printf(CHAN_DEL_DEBUG, "Delete channel %s\n",
			eci->esc_channel_name.value);
		log_printf(CHAN_UNLINK_DEBUG, "Delete channel %s, unlink_id %0llx\n",
			eci->esc_channel_name.value, (unsigned long long)eci->esc_unlink_id);

		if (!list_empty(&eci->esc_open_chans)) {
				log_printf(LOG_LEVEL_NOTICE,
					"Last channel close request for %s (still open)\n",
					eci->esc_channel_name.value);
				dump_chan_opens(eci);
				return;
		}

		/*
		 * adjust if we're sending open counts on a config change.
		 */
		if ((recovery_phase != evt_recovery_complete) &&
								(&eci->esc_entry == next_chan)) {
			next_chan = eci->esc_entry.next;
		}

		list_del(&eci->esc_entry);
		if (eci->esc_node_opens) {
			free(eci->esc_node_opens);
		}
		free(eci);
	}
}

/*
 * Free up an event structure if it isn't being used anymore.
 */
static void
free_event_data(struct event_data *edp)
{
	if (--edp->ed_ref_count) {
		return;
	}
	log_printf(LOG_LEVEL_DEBUG, "Freeing event ID: 0x%llx\n",
			(unsigned long long)edp->ed_event.led_event_id);
	if (edp->ed_delivered) {
		free(edp->ed_delivered);
	}

	free(edp);
}

/*
 * Mark a channel for deletion.
 */
static void unlink_channel(struct event_svr_channel_instance *eci,
		uint64_t unlink_id)
{
	struct event_data *edp;
	struct list_head *l, *nxt;

	log_printf(CHAN_UNLINK_DEBUG, "Unlink request: %s, id 0x%llx\n",
			eci->esc_channel_name.value, (unsigned long long)unlink_id);
	/*
	 * The unlink ID is used to note that the channel has been marked
	 * for deletion and is a way to distinguish between multiple
	 * channels of the same name each marked for deletion.
	 */
	eci->esc_unlink_id = unlink_id;

	/*
	 * Move the unlinked channel to the unlinked list.  This way
	 * we don't have to worry about filtering it out when we need to
	 * distribute retained events at recovery time.
	 */
	list_del(&eci->esc_entry);
	list_add(&eci->esc_entry, &esc_unlinked_head);

	/*
	 * Scan the retained event list and remove any retained events.
	 * Since no new opens can occur there won't be any need of sending
	 * retained events on the channel.
	 */
	for (l = retained_list.next; l != &retained_list; l = nxt) {
		nxt = l->next;
		edp = list_entry(l, struct event_data, ed_retained);
		if ((edp->ed_my_chan == eci) &&
				(edp->ed_event.led_chan_unlink_id == EVT_CHAN_ACTIVE)) {
			openais_timer_delete(edp->ed_timer_handle);
			edp->ed_event.led_retention_time = 0;
			list_del(&edp->ed_retained);
			list_init(&edp->ed_retained);
			edp->ed_my_chan->esc_retained_count--;

			log_printf(CHAN_UNLINK_DEBUG,
				"Unlink: Delete retained event id 0x%llx\n",
			(unsigned long long)edp->ed_event.led_event_id);
			free_event_data(edp);
		}
	}

	delete_channel(eci);
}

/*
 * Remove the specified node from the node list in this channel.
 */
static int remove_open_count(
		struct event_svr_channel_instance *eci,
		mar_uint32_t node_id)
{
	int i;
	int j;

	/*
	 * Find the node, remove it and re-pack the array.
	 */
	for (i = 0; i < eci->esc_oc_size; i++) {
		if (eci->esc_node_opens[i].oc_node_id == 0) {
			break;
		}

		log_printf(RECOVERY_DEBUG, "roc: %s/%s, t %d, oc %d\n",
			totempg_ifaces_print (node_id),
			totempg_ifaces_print (eci->esc_node_opens[i].oc_node_id),
			eci->esc_total_opens, eci->esc_node_opens[i].oc_open_count);

		if (eci->esc_node_opens[i].oc_node_id == node_id) {

			eci->esc_total_opens -= eci->esc_node_opens[i].oc_open_count;

			for (j = i+1; j < eci->esc_oc_size; j++, i++) {
				eci->esc_node_opens[i].oc_node_id =
				 eci->esc_node_opens[j].oc_node_id;
				eci->esc_node_opens[i].oc_open_count =
				 eci->esc_node_opens[j].oc_open_count;
			}

			eci->esc_node_opens[eci->esc_oc_size-1].oc_node_id = 0;
			eci->esc_node_opens[eci->esc_oc_size-1].oc_open_count = 0;

			/*
			 * Remove the channel if it's not being used anymore
			 */
			delete_channel(eci);
			return 0;
		}
	}
	return -1;
}


/*
 * Send a request to open a channel to the rest of the cluster.
 */
static SaAisErrorT evt_open_channel(mar_name_t *cn, SaUint8T flgs)
{
	struct req_evt_chan_command cpkt;
	struct event_svr_channel_instance *eci;
	struct iovec chn_iovec;
	int res;
	SaAisErrorT ret;

	ret = SA_AIS_OK;

	eci = find_channel(cn, EVT_CHAN_ACTIVE);

	/*
	 * If the create flag set, and it doesn't exist, we can make the channel.
	 * Otherwise, it's an error since we're notified of channels being created
	 * and opened.
	 */
	if (!eci && !(flgs & SA_EVT_CHANNEL_CREATE)) {
		ret = SA_AIS_ERR_NOT_EXIST;
		goto chan_open_end;
	}

	/*
	 * create the channel packet to send. Tell the the cluster
	 * to create the channel.
	 */
	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.chc_head.id =
		SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
	cpkt.chc_head.size = sizeof(cpkt);
	cpkt.chc_op = EVT_OPEN_CHAN_OP;
	cpkt.u.chc_chan.ocr_name = *cn;
	cpkt.u.chc_chan.ocr_serial_no = ++open_serial_no;
	chn_iovec.iov_base = &cpkt;
	chn_iovec.iov_len = cpkt.chc_head.size;
	log_printf(CHAN_OPEN_DEBUG, "evt_open_channel: Send open mcast\n");
	res = totempg_groups_mcast_joined(openais_group_handle,
			&chn_iovec, 1, TOTEMPG_AGREED);
	log_printf(CHAN_OPEN_DEBUG, "evt_open_channel: Open mcast result: %d\n",
				res);
	if (res != 0) {
			ret = SA_AIS_ERR_LIBRARY;
	}

chan_open_end:
	return ret;

}

/*
 * Send a request to close a channel with the rest of the cluster.
 */
static SaAisErrorT evt_close_channel(mar_name_t *cn, uint64_t unlink_id)
{
	struct req_evt_chan_command cpkt;
	struct iovec chn_iovec;
	int res;
	SaAisErrorT ret;

	ret = SA_AIS_OK;

	/*
	 * create the channel packet to send. Tell the the cluster
	 * to close the channel.
	 */
	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.chc_head.id =
		SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
	cpkt.chc_head.size = sizeof(cpkt);
	cpkt.chc_op = EVT_CLOSE_CHAN_OP;
	cpkt.u.chcu.chcu_name = *cn;
	cpkt.u.chcu.chcu_unlink_id = unlink_id;
	chn_iovec.iov_base = &cpkt;
	chn_iovec.iov_len = cpkt.chc_head.size;
	res = totempg_groups_mcast_joined(openais_group_handle,
			&chn_iovec, 1, TOTEMPG_AGREED);
	if (res != 0) {
			ret = SA_AIS_ERR_LIBRARY;
	}
	return ret;

}

/*
 * Node data access functions.  Used to keep track of event IDs
 * delivery of messages.
 *
 * add_node:	Add a new member node to our list.
 * remove_node:	Remove a node that left membership.
 * find_node:	Given the node ID return a pointer to node information.
 *
 */
#define NODE_HASH_SIZE 256
static struct member_node_data *nl[NODE_HASH_SIZE] = {0};
inline int
hash_sock_addr(unsigned int nodeid)
{
	return nodeid & (NODE_HASH_SIZE - 1);
}

static struct member_node_data **lookup_node(unsigned int nodeid)
{
	int index = hash_sock_addr(nodeid);
	struct member_node_data **nlp;

	nlp = &nl[index];
	for (nlp = &nl[index]; *nlp; nlp = &((*nlp)->mn_next)) {
		if ((*(nlp))->mn_nodeid == nodeid) {
			break;
		}
	}

	return nlp;
}

static struct member_node_data *
evt_find_node(unsigned int nodeid)
{
	struct member_node_data **nlp;

	nlp = lookup_node(nodeid);

	if (!nlp) {
		log_printf(LOG_LEVEL_DEBUG, "find_node: Got NULL nlp?\n");
		return 0;
	}

	return *nlp;
}

static SaAisErrorT
evt_add_node(
	unsigned int nodeid,
	SaClmClusterNodeT *cn)
{
	struct member_node_data **nlp;
	struct member_node_data *nl;
	SaAisErrorT err = SA_AIS_ERR_EXIST;

	nlp = lookup_node(nodeid);

	if (!nlp) {
		log_printf(LOG_LEVEL_DEBUG, "add_node: Got NULL nlp?\n");
		goto an_out;
	}

	if (*nlp) {
		goto an_out;
	}

	*nlp = malloc(sizeof(struct member_node_data));
	if (!(*nlp)) {
			return SA_AIS_ERR_NO_MEMORY;
	}
	nl = *nlp;
	if (nl) {
		memset(nl, 0, sizeof(*nl));
		err = SA_AIS_OK;
		nl->mn_nodeid = nodeid;
		nl->mn_started = 1;
	}
	list_init(&nl->mn_entry);
	list_add(&nl->mn_entry, &mnd);
	nl->mn_node_info = *cn;

an_out:
	return err;
}

/*
 * Find the oldest node in the membership.  This is the one we choose to
 * perform some cluster-wide functions like distributing retained events.
 * We only check nodes that were in our transitional configuration.  In this
 * way there is a recovery node chosen for each original partition in case
 * of a merge.
 */
static struct member_node_data* oldest_node()
{
	struct member_node_data *mn = 0;
	struct member_node_data *oldest = 0;
	int i;

	for (i = 0; i < trans_member_count; i++) {
		mn = evt_find_node(trans_member_list[i]);
		if (!mn || (mn->mn_started == 0)) {
			log_printf(LOG_LEVEL_ERROR,
				"Transitional config Node %s not active\n",
				totempg_ifaces_print (trans_member_list[i]));
			continue;
		}
		if ((oldest == NULL) ||
				(mn->mn_node_info.bootTimestamp <
					 oldest->mn_node_info.bootTimestamp )) {
			oldest = mn;
		} else if (mn->mn_node_info.bootTimestamp ==
					 oldest->mn_node_info.bootTimestamp) {
			if (mn->mn_node_info.nodeId < oldest->mn_node_info.nodeId) {
				oldest = mn;
			}
		}
	}
	return oldest;
}


/*
 * keep track of the last event ID from a node.
 * If we get an event ID less than our last, we've already
 * seen it.  It's probably a retained event being sent to
 * a new node.
 */
static int check_last_event(
	struct lib_event_data *evtpkt,
	unsigned int nodeid)
{
	struct member_node_data *nd;
	SaClmClusterNodeT *cn;


	nd = evt_find_node(nodeid);
	if (!nd) {
		log_printf(LOG_LEVEL_DEBUG,
				"Node ID %s not found for event %llx\n",
				totempg_ifaces_print (evtpkt->led_publisher_node_id),
				(unsigned long long)evtpkt->led_event_id);
		cn = main_clm_get_by_nodeid(nodeid);
		if (!cn) {
			log_printf(LOG_LEVEL_DEBUG,
					"Cluster Node 0x%s not found for event %llx\n",
				totempg_ifaces_print (evtpkt->led_publisher_node_id),
				(unsigned long long)evtpkt->led_event_id);
		} else {
			evt_add_node(nodeid, cn);
			nd = evt_find_node(nodeid);
		}
	}

	if (!nd) {
		return 0;
	}

	if ((nd->mn_last_msg_id < evtpkt->led_msg_id)) {
		nd->mn_last_msg_id = evtpkt->led_msg_id;
		return 0;
	}
	return 1;
}

/*
 * event id generating code.  We use the node ID for this node for the
 * upper 32 bits of the event ID to make sure that we can generate a cluster
 * wide unique event ID for a given event.
 */
SaAisErrorT set_event_id(mar_uint32_t node_id)
{
	SaAisErrorT err = SA_AIS_OK;
	if (base_id_top) {
		err =  SA_AIS_ERR_EXIST;
	}
	base_id_top = (mar_evteventid_t)node_id << 32;
	return err;
}

/*
 * See if an event Id is still in use in the retained event
 * list.
 */
static int id_in_use(uint64_t id, uint64_t base)
{
	struct list_head *l;
	struct event_data *edp;
	mar_evteventid_t evtid = (id << 32) | (base & BASE_ID_MASK);

	for (l = retained_list.next; l != &retained_list; l = l->next) {
		edp = list_entry(l, struct event_data, ed_retained);
		if (edp->ed_event.led_event_id == evtid) {
			return 1;
		}
	}
	return 0;
}

static SaAisErrorT get_event_id(uint64_t *event_id, uint64_t *msg_id)
{
	/*
	 * Don't reuse an event ID if it is still valid because of
	 * a retained event.
	 */
	while (id_in_use(base_id_top, base_id)) {
		base_id++;
	}

	*event_id = base_id_top | (base_id & BASE_ID_MASK) ;
	*msg_id = base_id++;
	return SA_AIS_OK;
}



/*
 * Timer handler to delete expired events.
 *
 */
static void
event_retention_timeout(void *data)
{
	struct event_data *edp = data;
	log_printf(RETENTION_TIME_DEBUG, "Event ID %llx expired\n",
		(unsigned long long)edp->ed_event.led_event_id);
	/*
	 * adjust next_retained if we're in recovery and
	 * were in charge of sending retained events.
	 */
	if (recovery_phase != evt_recovery_complete && recovery_node) {
		if (next_retained == &edp->ed_retained) {
			next_retained = edp->ed_retained.next;
		}
	}
	list_del(&edp->ed_retained);
	list_init(&edp->ed_retained);
	/*
	 * Check to see if the channel isn't in use anymore.
	 */
	edp->ed_my_chan->esc_retained_count--;
	if (edp->ed_my_chan->esc_retained_count == 0) {
		delete_channel(edp->ed_my_chan);
	}
	free_event_data(edp);
}

/*
 * clear a particular event's retention time.
 * This will free the event as long as it isn't being
 * currently used.
 *
 */
static SaAisErrorT
clear_retention_time(mar_evteventid_t event_id)
{
	struct event_data *edp;
	struct list_head *l, *nxt;

	log_printf(RETENTION_TIME_DEBUG, "Search for Event ID %llx\n", 
		(unsigned long long)event_id);
	for (l = retained_list.next; l != &retained_list; l = nxt) {
		nxt = l->next;
		edp = list_entry(l, struct event_data, ed_retained);
		if (edp->ed_event.led_event_id != event_id) {
				continue;
		}

		log_printf(RETENTION_TIME_DEBUG,
							"Clear retention time for Event ID %llx\n",
				(unsigned long long)edp->ed_event.led_event_id);
		openais_timer_delete(edp->ed_timer_handle);
		edp->ed_event.led_retention_time = 0;
		list_del(&edp->ed_retained);
		list_init(&edp->ed_retained);

		/*
		 * Check to see if the channel isn't in use anymore.
		 */
		edp->ed_my_chan->esc_retained_count--;
		if (edp->ed_my_chan->esc_retained_count == 0) {
			delete_channel(edp->ed_my_chan);
		}
		free_event_data(edp);
		return SA_AIS_OK;
	}
	return SA_AIS_ERR_NOT_EXIST;
}

/*
 * Remove specified channel from event delivery list
 */
static void
remove_delivered_channel(struct event_svr_channel_open *eco)
{
	int i;
	struct list_head *l;
	struct event_data *edp;

	for (l = retained_list.next; l != &retained_list; l = l->next) {
		edp = list_entry(l, struct event_data, ed_retained);

		for (i = 0; i < edp->ed_delivered_next; i++) {
			if (edp->ed_delivered[i] == eco) {
				edp->ed_delivered_next--;
				if (edp->ed_delivered_next == i) {
					break;
				}
				memmove(&edp->ed_delivered[i],
					&edp->ed_delivered[i+1],
					&edp->ed_delivered[edp->ed_delivered_next] -
					   &edp->ed_delivered[i]);
				break;
			}
		}
	}
}

/*
 * If there is a retention time, add this open channel to the event so
 * we can check if we've already delivered this message later if a new
 * subscription matches.
 */
#define DELIVER_SIZE 8
static void
evt_delivered(struct event_data *evt, struct event_svr_channel_open *eco)
{
	if (!evt->ed_event.led_retention_time) {
		return;
	}

	log_printf(LOG_LEVEL_DEBUG, "delivered ID %llx to eco %p\n",
			(unsigned long long)evt->ed_event.led_event_id, eco);
	if (evt->ed_delivered_count == evt->ed_delivered_next) {
		evt->ed_delivered = realloc(evt->ed_delivered,
			DELIVER_SIZE * sizeof(struct event_svr_channel_open *));
		memset(evt->ed_delivered + evt->ed_delivered_next, 0,
			DELIVER_SIZE * sizeof(struct event_svr_channel_open *));
		evt->ed_delivered_next = evt->ed_delivered_count;
		evt->ed_delivered_count += DELIVER_SIZE;
	}

	evt->ed_delivered[evt->ed_delivered_next++] = eco;
}

/*
 * Check to see if an event has already been delivered to this open channel
 */
static int
evt_already_delivered(struct event_data *evt,
		struct event_svr_channel_open *eco)
{
	int i;

	if (!evt->ed_event.led_retention_time) {
		return 0;
	}

	log_printf(LOG_LEVEL_DEBUG, "Deliver count: %d deliver_next %d\n",
		evt->ed_delivered_count, evt->ed_delivered_next);
	for (i = 0; i < evt->ed_delivered_next; i++) {
		log_printf(LOG_LEVEL_DEBUG, "Checking ID %llx delivered %p eco %p\n",
			(unsigned long long)evt->ed_event.led_event_id,
			evt->ed_delivered[i], eco);
		if (evt->ed_delivered[i] == eco) {
			return 1;
		}
	}
	return 0;
}

/*
 * Compare a filter to a given pattern.
 * return SA_AIS_OK if the pattern matches a filter
 */
static SaAisErrorT
filter_match(mar_evt_event_pattern_t *ep, mar_evt_event_filter_t *ef)
{
	int ret;
	ret = SA_AIS_ERR_FAILED_OPERATION;

	switch (ef->filter_type) {
	case SA_EVT_PREFIX_FILTER:
		if (ef->filter.pattern_size > ep->pattern_size) {
			break;
		}
		if (strncmp((char *)ef->filter.pattern, (char *)ep->pattern,
					ef->filter.pattern_size) == 0) {
			ret = SA_AIS_OK;
		}
		break;
	case SA_EVT_SUFFIX_FILTER:
		if (ef->filter.pattern_size > ep->pattern_size) {
			break;
		}
		if (strncmp((char *)ef->filter.pattern,
			(char *)&ep->pattern[ep->pattern_size - ef->filter.pattern_size],
					ef->filter.pattern_size) == 0) {
			ret = SA_AIS_OK;
		}

		break;
	case SA_EVT_EXACT_FILTER:
		if (ef->filter.pattern_size != ep->pattern_size) {
			break;
		}
		if (strncmp((char *)ef->filter.pattern, (char *)ep->pattern,
					ef->filter.pattern_size) == 0) {
			ret = SA_AIS_OK;
		}
		break;
	case SA_EVT_PASS_ALL_FILTER:
		ret = SA_AIS_OK;
		break;
	default:
		break;
	}
	return ret;
}

/*
 * compare the event's patterns with the subscription's filter rules.
 * SA_AIS_OK is returned if the event matches the filter rules.
 */
static SaAisErrorT
event_match(struct event_data *evt,
			struct event_svr_channel_subscr *ecs)
{
	mar_evt_event_filter_t *ef;
	mar_evt_event_pattern_t *ep;
	uint32_t filt_count;
	SaAisErrorT ret =  SA_AIS_OK;
	int i;

	ep = (mar_evt_event_pattern_t *)(&evt->ed_event.led_body[0]);
	ef = ecs->ecs_filters->filters;
	filt_count = min(ecs->ecs_filters->filters_number,
			evt->ed_event.led_patterns_number);

	for (i = 0; i < filt_count; i++) {
		ret = filter_match(ep, ef);
		if (ret != SA_AIS_OK) {
			break;
		}
		ep++;
		ef++;
	}
	return ret;
}

/*
 * Scan undelivered pending events and either remove them if no subscription
 * filters match anymore or re-assign them to another matching subscription
 */
static void
filter_undelivered_events(struct event_svr_channel_open *op_chan)
{
	struct event_svr_channel_open *eco;
	struct event_svr_channel_instance *eci;
	struct event_svr_channel_subscr *ecs;
	struct chan_event_list *cel;
	struct libevt_pd *esip;
	struct list_head *l, *nxt;
	struct list_head *l1, *l2;
	int i;

	esip = (struct libevt_pd *)openais_conn_private_data_get(op_chan->eco_conn);
	eci = op_chan->eco_channel;

	/*
	 * Scan each of the priority queues for messages
	 */
	for (i = SA_EVT_HIGHEST_PRIORITY; i <= SA_EVT_LOWEST_PRIORITY; i++) {
		/*
		 * examine each message queued for delivery
		 */
		for (l = esip->esi_events[i].next; l != &esip->esi_events[i]; l = nxt) {
			nxt = l->next;
			cel = list_entry(l, struct chan_event_list, cel_entry);
			/*
			 * Check open channels
			 */
			 for (l1 = eci->esc_open_chans.next;
								l1 != &eci->esc_open_chans; l1 = l1->next) {
				 eco = list_entry(l1, struct event_svr_channel_open, eco_entry);

				 /*
				  * See if this channel open instance belongs
				  * to this evtinitialize instance
				  */
				 if (eco->eco_conn != op_chan->eco_conn) {
					 continue;
				 }

				 /*
				  * See if enabled to receive
				  */
				 if (!(eco->eco_flags & SA_EVT_CHANNEL_SUBSCRIBER)) {
					continue;
				 }

				 /*
				  * Check subscriptions
				  */
				 for (l2 = eco->eco_subscr.next;
									l2 != &eco->eco_subscr; l2 = l2->next) {
					 ecs = list_entry(l2,
								struct event_svr_channel_subscr, ecs_entry);
					 if (event_match(cel->cel_event, ecs) == SA_AIS_OK) {
						 /*
						  * Something still matches.
						  * We'll assign it to
						  * the new subscription.
						  */
						 cel->cel_sub_id = ecs->ecs_sub_id;
						 cel->cel_chan_handle = eco->eco_lib_handle;
						 goto next_event;
					 }
				 }
			 }
			 /*
			  * No subscription filter matches anymore.  We
			  * can delete this event.
			  */
			 list_del(&cel->cel_entry);
			 list_init(&cel->cel_entry);
			 esip->esi_nevents--;

			 free_event_data(cel->cel_event);
			 free(cel);
next_event:
			 continue;
		 }
	}
}

/*
 * Notify the library of a pending event
 */
static void __notify_event(void	*conn)
{
	struct res_evt_event_data res;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);
	log_printf(LOG_LEVEL_DEBUG, "DELIVER: notify\n");
	if (esip->esi_nevents != 0) {
		res.evd_head.size = sizeof(res);
		res.evd_head.id = MESSAGE_RES_EVT_AVAILABLE;
		res.evd_head.error = SA_AIS_OK;
		openais_dispatch_send (
				conn,
				&res,
				sizeof(res));
	}

}
inline void notify_event(void *conn)
{
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);

	/*
	 * Give the library a kick if there aren't already
	 * events queued for delivery.
	 */
	if (esip->esi_nevents++ == 0) {
		__notify_event(conn);
	}
}

/*
 * sends/queues up an event for a subscribed channel.
 */
static void
deliver_event(struct event_data *evt,
		struct event_svr_channel_open *eco,
		struct event_svr_channel_subscr *ecs)
{
	struct chan_event_list *ep;
	SaEvtEventPriorityT evt_prio = evt->ed_event.led_priority;
	struct chan_event_list *cel;
	int do_deliver_event = 0;
	int do_deliver_warning = 0;
	int i;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(eco->eco_conn);

	if (evt_prio > SA_EVT_LOWEST_PRIORITY) {
		evt_prio = SA_EVT_LOWEST_PRIORITY;
	}

	/*
	 * Delivery queue check.
	 * - If the queue is blocked, see if we've sent enough messages to
	 *   unblock it.
	 * - If it isn't blocked, see if this message will put us over the top.
	 * - If we can't deliver this message, see if we can toss some lower
	 *   priority message to make room for this one.
	 * - If we toss any messages, queue up an event of SA_EVT_LOST_EVENT_PATTERN
	 *   to let the application know that we dropped some messages.
	 */
	if (esip->esi_queue_blocked) {
		if (esip->esi_nevents < evt_delivery_queue_resume) {
			esip->esi_queue_blocked = 0;
			log_printf(LOG_LEVEL_DEBUG, "unblock\n");
		}
	}

	assert (esip->esi_nevents >= 0);
	if (!esip->esi_queue_blocked &&
							(esip->esi_nevents >= evt_delivery_queue_size)) {
		log_printf(LOG_LEVEL_DEBUG, "block\n");
		esip->esi_queue_blocked = 1;
		do_deliver_warning = 1;
	}

	if (esip->esi_queue_blocked) {
		do_deliver_event = 0;
		for (i = SA_EVT_LOWEST_PRIORITY; i > evt_prio; i--) {
			if (!list_empty(&esip->esi_events[i])) {
				/*
				 * Get the last item on the list, so we drop the most
				 * recent lowest priority event.
				 */
				cel = list_entry(esip->esi_events[i].prev,
					struct chan_event_list, cel_entry);
				log_printf(LOG_LEVEL_DEBUG, "Drop 0x%0llx\n",
					(unsigned long long)cel->cel_event->ed_event.led_event_id);
				list_del(&cel->cel_entry);
				free_event_data(cel->cel_event);
				free(cel);
				esip->esi_nevents--;
				do_deliver_event = 1;
				break;
			}
		}
	} else {
		do_deliver_event = 1;
	}

	/*
	 * Queue the event for delivery
	 */
	if (do_deliver_event) {
		ep = malloc(sizeof(*ep));
		if (!ep) {
			log_printf(LOG_LEVEL_WARNING,
						"3Memory allocation error, can't deliver event\n");
			return;
		}
		evt->ed_ref_count++;
		ep->cel_chan_handle = eco->eco_lib_handle;
		ep->cel_sub_id = ecs->ecs_sub_id;
		list_init(&ep->cel_entry);
		ep->cel_event = evt;
		list_add_tail(&ep->cel_entry, &esip->esi_events[evt_prio]);
		evt_delivered(evt, eco);
		notify_event(eco->eco_conn);
	}

	/*
	 * If we dropped an event, queue this so that the application knows
	 * what has happened.
	 */
	if (do_deliver_warning) {
		struct event_data *ed;
		ed = malloc(dropped_event_size);
		if (!ed) {
			log_printf(LOG_LEVEL_WARNING,
						"4Memory allocation error, can't deliver event\n");
			return;
		}
		log_printf(LOG_LEVEL_DEBUG, "Warn 0x%0llx\n",
			(unsigned long long)evt->ed_event.led_event_id);
		memcpy(ed, dropped_event, dropped_event_size);
		ed->ed_event.led_publish_time = clust_time_now();
		ed->ed_event.led_event_id = SA_EVT_EVENTID_LOST;
		list_init(&ed->ed_retained);

		ep = malloc(sizeof(*ep));
		if (!ep) {
			log_printf(LOG_LEVEL_WARNING,
						"5Memory allocation error, can't deliver event\n");
			free (ed);
			return;
		}
		ep->cel_chan_handle = eco->eco_lib_handle;
		ep->cel_sub_id = ecs->ecs_sub_id;
		list_init(&ep->cel_entry);
		ep->cel_event = ed;
		list_add_tail(&ep->cel_entry, &esip->esi_events[SA_EVT_HIGHEST_PRIORITY]);
		notify_event(eco->eco_conn);
	}
}

/*
 * Take the event data and swap the elements so they match our architectures
 * word order.
 */
static void
convert_event(void *msg)
{
	struct lib_event_data *evt = (struct lib_event_data *)msg;
	mar_evt_event_pattern_t *eps;
	int i;

	/*
	 * The following elements don't require processing:
	 *
	 * converted in the main deliver_fn:
	 * led_head.id, led_head.size.
	 *
	 * Supplied localy:
	 * source_addr, publisher_node_id, receive_time.
	 *
	 * Used internaly only:
	 * led_svr_channel_handle and led_lib_channel_handle.
	 */

	swab_mar_name_t (&evt->led_chan_name);
	evt->led_chan_unlink_id = swab64(evt->led_chan_unlink_id);
	evt->led_event_id = swab64(evt->led_event_id);
	evt->led_sub_id = swab32(evt->led_sub_id);
	swab_mar_name_t (&evt->led_publisher_name);
	evt->led_retention_time = swab64(evt->led_retention_time);
	evt->led_publish_time = swab64(evt->led_publish_time);
	evt->led_user_data_offset = swab32(evt->led_user_data_offset);
	evt->led_user_data_size = swab32(evt->led_user_data_size);
	evt->led_patterns_number = swab32(evt->led_patterns_number);

	/*
	 * Now we need to go through the led_body and swizzle pattern counts.
	 * We can't do anything about user data since it doesn't have a specified
	 * format.  The application is on its own here.
	 */
	eps = (mar_evt_event_pattern_t *)evt->led_body;
	for (i = 0; i < evt->led_patterns_number; i++) {
		eps->pattern_size = swab32(eps->pattern_size);
		eps->allocated_size = swab32(eps->allocated_size);
		eps++;
	}

}

/*
 * Take an event received from the network and fix it up to be usable.
 * - fix up pointers for pattern list.
 * - fill in some channel info
 */
static struct event_data *
make_local_event(struct lib_event_data *p,
			struct event_svr_channel_instance *eci)
{
	struct event_data *ed;
	mar_evt_event_pattern_t *eps;
	SaUint8T *str;
	uint32_t ed_size;
	int i;

	ed_size = sizeof(*ed) + p->led_user_data_offset + p->led_user_data_size;
	ed = malloc(ed_size);
	if (!ed) {
		log_printf(LOG_LEVEL_WARNING,
			"Failed to allocate %u bytes for event, offset %u, data size %u\n",
				ed_size, p->led_user_data_offset, p->led_user_data_size);
		return 0;
	}
	memset(ed, 0, ed_size);
	list_init(&ed->ed_retained);
	ed->ed_my_chan = eci;

	/*
	 * Fill in lib_event_data and make the pattern pointers valid
	 */
	memcpy(&ed->ed_event, p, sizeof(*p) +
					p->led_user_data_offset + p->led_user_data_size);

	eps = (mar_evt_event_pattern_t *)ed->ed_event.led_body;
	str = ed->ed_event.led_body +
			(ed->ed_event.led_patterns_number * sizeof(mar_evt_event_pattern_t));
	for (i = 0; i < ed->ed_event.led_patterns_number; i++) {
		eps->pattern = str;
		str += eps->pattern_size;
		eps++;
	}

	ed->ed_ref_count++;
	return ed;
}

/*
 * Set an event to be retained.
 */
static void retain_event(struct event_data *evt)
{
	uint32_t ret;
	evt->ed_ref_count++;
	evt->ed_my_chan->esc_retained_count++;
	list_add_tail(&evt->ed_retained, &retained_list);

	ret = openais_timer_add_duration (
		evt->ed_event.led_retention_time,
		evt,
		event_retention_timeout,
		&evt->ed_timer_handle);

	if (ret != 0) {
		log_printf(LOG_LEVEL_ERROR,
				"retention of event id 0x%llx failed\n",
				(unsigned long long)evt->ed_event.led_event_id);
	} else {
		log_printf(RETENTION_TIME_DEBUG, "Retain event ID 0x%llx for %llu ms\n",
			(unsigned long long)evt->ed_event.led_event_id, evt->ed_event.led_retention_time/100000LL);
	}
}

/*
 * Scan the subscription list and look for the specified subsctiption ID.
 * Only look for the ID in subscriptions that are associated with the
 * saEvtInitialize associated with the specified open channel.
 */
static struct event_svr_channel_subscr *find_subscr(
		struct event_svr_channel_open *open_chan, SaEvtSubscriptionIdT sub_id)
{
	struct event_svr_channel_instance *eci;
	struct event_svr_channel_subscr *ecs;
	struct event_svr_channel_open	*eco;
	struct list_head *l, *l1;
	void  *conn = open_chan->eco_conn;

	eci = open_chan->eco_channel;

	/*
	 * Check for subscription id already in use.
	 * Subscriptions are unique within saEvtInitialize (Callback scope).
	 */
    for (l = eci->esc_open_chans.next; l != &eci->esc_open_chans; l = l->next) {
		eco = list_entry(l, struct event_svr_channel_open, eco_entry);
		/*
		 * Don't bother with open channels associated with another
		 * EvtInitialize
		 */
		if (eco->eco_conn != conn) {
			continue;
		}

		for (l1 = eco->eco_subscr.next; l1 != &eco->eco_subscr; l1 = l1->next) {
			ecs = list_entry(l1, struct event_svr_channel_subscr, ecs_entry);
			if (ecs->ecs_sub_id == sub_id) {
				return ecs;
			}
		}
	}
	return 0;
}

/*
 * Handler for saEvtInitialize
 */
static int evt_lib_init(void *conn)
{
	struct libevt_pd *libevt_pd;
	int i;

	libevt_pd = (struct libevt_pd *)openais_conn_private_data_get(conn);


	log_printf(LOG_LEVEL_DEBUG, "saEvtInitialize request.\n");

	/*
	 * Initailze event instance data
	 */
	memset(libevt_pd, 0, sizeof(*libevt_pd));

	/*
	 * Initialize the open channel handle database.
	 */
	hdb_create(&libevt_pd->esi_hdb);

	/*
	 * list of channels open on this instance
	 */
	list_init(&libevt_pd->esi_open_chans);

	/*
	 * pending event lists for each piriority
	 */
	for (i = SA_EVT_HIGHEST_PRIORITY; i <= SA_EVT_LOWEST_PRIORITY; i++) {
		list_init(&libevt_pd->esi_events[i]);
	}

	return 0;
}

/*
 * Handler for saEvtChannelOpen
 */
static void lib_evt_open_channel(void *conn, void *message)
{
	SaAisErrorT error;
	struct req_evt_channel_open *req;
	struct res_evt_channel_open res;
	struct open_chan_pending *ocp;
	int ret;

	req = message;


	log_printf(CHAN_OPEN_DEBUG,
		"saEvtChannelOpen (Open channel request)\n");
	log_printf(CHAN_OPEN_DEBUG,
		"handle 0x%llx, to 0x%llx\n",
		(unsigned long long)req->ico_c_handle,
		(unsigned long long)req->ico_timeout);
	log_printf(CHAN_OPEN_DEBUG, "flags %x, channel name(%d)  %s\n",
		req->ico_open_flag,
		req->ico_channel_name.length,
		req->ico_channel_name.value);
	/*
	 * Open the channel.
	 *
	 */
	error = evt_open_channel(&req->ico_channel_name, req->ico_open_flag);

	if (error != SA_AIS_OK) {
		goto open_return;
	}

	ocp = malloc(sizeof(struct open_chan_pending));
	if (!ocp) {
		error = SA_AIS_ERR_NO_MEMORY;
		goto open_return;
	}

	ocp->ocp_async = 0;
	ocp->ocp_invocation = 0;
	ocp->ocp_chan_name = req->ico_channel_name;
	ocp->ocp_open_flag = req->ico_open_flag;
	ocp->ocp_conn = conn;
	ocp->ocp_c_handle = req->ico_c_handle;
	ocp->ocp_timer_handle = 0;
	ocp->ocp_serial_no = open_serial_no;
	list_init(&ocp->ocp_entry);
	list_add_tail(&ocp->ocp_entry, &open_pending);
	ret = openais_timer_add_duration (
		req->ico_timeout,
		ocp,
		chan_open_timeout,
		&ocp->ocp_timer_handle);
	if (ret != 0) {
		log_printf(LOG_LEVEL_WARNING,
				"Error setting timeout for open channel %s\n",
				req->ico_channel_name.value);
	}
	return;


open_return:
	res.ico_head.size = sizeof(res);
	res.ico_head.id = MESSAGE_RES_EVT_OPEN_CHANNEL;
	res.ico_head.error = error;
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * Handler for saEvtChannelOpen
 */
static void lib_evt_open_channel_async(void *conn, void *message)
{
	SaAisErrorT error;
	struct req_evt_channel_open *req;
	struct res_evt_channel_open res;
	struct open_chan_pending *ocp;

	req = message;


	log_printf(CHAN_OPEN_DEBUG,
		"saEvtChannelOpenAsync (Async Open channel request)\n");
	log_printf(CHAN_OPEN_DEBUG,
		"handle 0x%llx, to 0x%llx\n",
		(unsigned long long)req->ico_c_handle,
		(unsigned long long)req->ico_invocation);
	log_printf(CHAN_OPEN_DEBUG, "flags %x, channel name(%d)  %s\n",
		req->ico_open_flag,
		req->ico_channel_name.length,
		req->ico_channel_name.value);
	/*
	 * Open the channel.
	 *
	 */
	error = evt_open_channel(&req->ico_channel_name, req->ico_open_flag);

	if (error != SA_AIS_OK) {
		goto open_return;
	}

	ocp = malloc(sizeof(struct open_chan_pending));
	if (!ocp) {
		error = SA_AIS_ERR_NO_MEMORY;
		goto open_return;
	}

	ocp->ocp_async = 1;
	ocp->ocp_invocation = req->ico_invocation;
	ocp->ocp_c_handle = req->ico_c_handle;
	ocp->ocp_chan_name = req->ico_channel_name;
	ocp->ocp_open_flag = req->ico_open_flag;
	ocp->ocp_conn = conn;
	ocp->ocp_timer_handle = 0;
	ocp->ocp_serial_no = open_serial_no;
	list_init(&ocp->ocp_entry);
	list_add_tail(&ocp->ocp_entry, &open_pending);

open_return:
	res.ico_head.size = sizeof(res);
	res.ico_head.id = MESSAGE_RES_EVT_OPEN_CHANNEL;
	res.ico_head.error = error;
	openais_dispatch_send(conn, &res, sizeof(res));
}



/*
 * Used by the channel close code and by the implicit close
 * when saEvtFinalize is called with channels open.
 */
static SaAisErrorT
common_chan_close(struct event_svr_channel_open	*eco, struct libevt_pd *esip)
{
	struct event_svr_channel_subscr *ecs;
	struct list_head *l, *nxt;

	log_printf(LOG_LEVEL_DEBUG, "Close channel %s flags 0x%02x\n",
			eco->eco_channel->esc_channel_name.value,
			eco->eco_flags);

	/*
	 * Disconnect the channel open structure.
	 *
	 * Check for subscriptions and deal with them.  In this case
	 * if there are any, we just implicitly unsubscribe.
	 *
	 * When We're done with the channel open data then we can
	 * remove it's handle (this frees the memory too).
	 *
	 */
	list_del(&eco->eco_entry);
	list_del(&eco->eco_instance_entry);

	for (l = eco->eco_subscr.next; l != &eco->eco_subscr; l = nxt) {
		nxt = l->next;
		ecs = list_entry(l, struct event_svr_channel_subscr, ecs_entry);
		log_printf(LOG_LEVEL_DEBUG, "Unsubscribe ID: %x\n",
				ecs->ecs_sub_id);
		list_del(&ecs->ecs_entry);
		free(ecs);
		/*
		 * Purge any pending events associated with this subscription
		 * that don't match another subscription.
		 */
		filter_undelivered_events(eco);
	}

	/*
	 * Remove this channel from the retained event's notion
	 * of who they have been delivered to.
	 */
	remove_delivered_channel(eco);
	return evt_close_channel(&eco->eco_channel->esc_channel_name,
			eco->eco_channel->esc_unlink_id);
}

/*
 * Handler for saEvtChannelClose
 */
static void lib_evt_close_channel(void *conn, void *message)
{
	struct req_evt_channel_close *req;
	struct res_evt_channel_close res;
	struct event_svr_channel_open	*eco;
	unsigned int ret;
	void *ptr;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);

	req = message;

	log_printf(LOG_LEVEL_DEBUG,
			"saEvtChannelClose (Close channel request)\n");
	log_printf(LOG_LEVEL_DEBUG, "handle 0x%x\n", req->icc_channel_handle);

	/*
	 * look up the channel handle
	 */
	ret = hdb_handle_get(&esip->esi_hdb,
					req->icc_channel_handle, &ptr);
	if (ret != 0) {
		goto chan_close_done;
	}
	eco = ptr;

	common_chan_close(eco, esip);
	hdb_handle_destroy(&esip->esi_hdb, req->icc_channel_handle);
	hdb_handle_put(&esip->esi_hdb, req->icc_channel_handle);

chan_close_done:
	res.icc_head.size = sizeof(res);
	res.icc_head.id = MESSAGE_RES_EVT_CLOSE_CHANNEL;
	res.icc_head.error = ((ret == 0) ? SA_AIS_OK : SA_AIS_ERR_BAD_HANDLE);
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * Handler for saEvtChannelUnlink
 */
static void lib_evt_unlink_channel(void *conn, void *message)
{
	struct req_evt_channel_unlink *req;
	struct res_evt_channel_unlink res;
	struct iovec chn_iovec;
	struct unlink_chan_pending *ucp = 0;
	struct req_evt_chan_command cpkt;
	SaAisErrorT error = SA_AIS_ERR_LIBRARY;

	req = message;

	log_printf(CHAN_UNLINK_DEBUG,
			"saEvtChannelUnlink (Unlink channel request)\n");
	log_printf(CHAN_UNLINK_DEBUG, "Channel Name %s\n",
			req->iuc_channel_name.value);

	if (!find_channel(&req->iuc_channel_name, EVT_CHAN_ACTIVE)) {
		log_printf(CHAN_UNLINK_DEBUG, "Channel Name doesn't exist\n");
		error = SA_AIS_ERR_NOT_EXIST;
		goto evt_unlink_err;
	}

	/*
	 * Set up the data structure so that the channel op
	 * mcast handler can complete the unlink comamnd back to the
	 * requestor.
	 */
	ucp = malloc(sizeof(*ucp));
	if (!ucp) {
		log_printf(LOG_LEVEL_ERROR,
				"saEvtChannelUnlink: Memory allocation failure\n");
		error = SA_AIS_ERR_TRY_AGAIN;
		goto evt_unlink_err;
	}

	ucp->ucp_unlink_id = next_chan_unlink_id();
	ucp->ucp_conn = conn;
	list_init(&ucp->ucp_entry);
	list_add_tail(&ucp->ucp_entry, &unlink_pending);

	/*
	 * Put together a mcast packet to notify everyone
	 * of the channel unlink.
	 */
	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.chc_head.id =
		SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
	cpkt.chc_head.size = sizeof(cpkt);
	cpkt.chc_op = EVT_UNLINK_CHAN_OP;
	cpkt.u.chcu.chcu_name = req->iuc_channel_name;
	cpkt.u.chcu.chcu_unlink_id = ucp->ucp_unlink_id;
	chn_iovec.iov_base = &cpkt;
	chn_iovec.iov_len = cpkt.chc_head.size;
	if (totempg_groups_mcast_joined(openais_group_handle,
								&chn_iovec, 1, TOTEMPG_AGREED) == 0) {
		return;
	}

evt_unlink_err:
	if (ucp) {
		list_del(&ucp->ucp_entry);
		free(ucp);
	}
	res.iuc_head.size = sizeof(res);
	res.iuc_head.id = MESSAGE_RES_EVT_UNLINK_CHANNEL;
	res.iuc_head.error = error;
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * Subscribe to an event channel.
 *
 * - First look up the channel to subscribe.
 * - Make sure that the subscription ID is not already in use.
 * - Fill in the subscription data structures and add them to the channels
 *      subscription list.
 * - See if there are any events with retetion times that need to be delivered
 *      because of the new subscription.
 */
static char *filter_types[] = {
	"INVALID FILTER TYPE",
	"SA_EVT_PREFIX_FILTER",
	"SA_EVT_SUFFIX_FILTER",
	"SA_EVT_EXACT_FILTER",
	"SA_EVT_PASS_ALL_FILTER",
};

/*
 * saEvtEventSubscribe Handler
 */
static void lib_evt_event_subscribe(void *conn, void *message)
{
	struct req_evt_event_subscribe *req;
	struct res_evt_event_subscribe res;
	mar_evt_event_filter_array_t *filters;
	SaAisErrorT error;
	struct event_svr_channel_open	*eco;
	struct event_svr_channel_instance *eci;
	struct event_svr_channel_subscr *ecs;
	struct event_data *evt;
	struct list_head *l;
	void *ptr;
	unsigned int ret;
	int i;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);

	req = message;

	log_printf(LOG_LEVEL_DEBUG,
		"saEvtEventSubscribe (Subscribe request)\n");
	log_printf(LOG_LEVEL_DEBUG, "subscription Id: 0x%llx\n",
		(unsigned long long)req->ics_sub_id);

	/*
	 * look up the channel handle
	 */
	ret = hdb_handle_get(&esip->esi_hdb, req->ics_channel_handle, &ptr);
	if (ret != 0) {
		error = SA_AIS_ERR_BAD_HANDLE;
		goto subr_done;
	}
	eco = ptr;

	eci = eco->eco_channel;

	/*
	 * See if the id is already being used
	 */
	ecs = find_subscr(eco, req->ics_sub_id);
	if (ecs) {
		error = SA_AIS_ERR_EXIST;
		goto subr_put;
	}

	error = evtfilt_to_aisfilt(req, &filters);

	if (error == SA_AIS_OK) {
		log_printf(LOG_LEVEL_DEBUG, "Subscribe filters count %d\n",
				(int)filters->filters_number);
		for (i = 0; i < filters->filters_number; i++) {
			log_printf(LOG_LEVEL_DEBUG, "type %s(%d) sz %d, <%s>\n",
					filter_types[filters->filters[i].filter_type],
					filters->filters[i].filter_type,
					(int)filters->filters[i].filter.pattern_size,
					(filters->filters[i].filter.pattern_size)
						? (char *)filters->filters[i].filter.pattern
						: "");
		}
	}

	if (error != SA_AIS_OK) {
		goto subr_put;
	}

	ecs = (struct event_svr_channel_subscr *)malloc(sizeof(*ecs));
	if (!ecs) {
		error = SA_AIS_ERR_NO_MEMORY;
		goto subr_put;
	}
	ecs->ecs_filters = filters;
	ecs->ecs_sub_id = req->ics_sub_id;
	list_init(&ecs->ecs_entry);
	list_add(&ecs->ecs_entry, &eco->eco_subscr);


	res.ics_head.size = sizeof(res);
	res.ics_head.id = MESSAGE_RES_EVT_SUBSCRIBE;
	res.ics_head.error = error;
	openais_response_send(conn, &res, sizeof(res));

	/*
	 * See if an existing event with a retention time
	 * needs to be delivered based on this subscription
	 */
	for (l = retained_list.next; l != &retained_list; l = l->next) {
		evt = list_entry(l, struct event_data, ed_retained);
		log_printf(LOG_LEVEL_DEBUG,
			"Checking event ID %llx chanp %p -- sub chanp %p\n",
			(unsigned long long)evt->ed_event.led_event_id,
			evt->ed_my_chan, eci);
		if (evt->ed_my_chan == eci) {
			if (evt_already_delivered(evt, eco)) {
				continue;
			}
			if (event_match(evt, ecs) == SA_AIS_OK) {
				log_printf(LOG_LEVEL_DEBUG,
					"deliver event ID: 0x%llx\n",
						(unsigned long long)evt->ed_event.led_event_id);
				deliver_event(evt, eco, ecs);
			}
		}
	}
	hdb_handle_put(&esip->esi_hdb, req->ics_channel_handle);
	return;

subr_put:
	hdb_handle_put(&esip->esi_hdb, req->ics_channel_handle);
subr_done:
	res.ics_head.size = sizeof(res);
	res.ics_head.id = MESSAGE_RES_EVT_SUBSCRIBE;
	res.ics_head.error = error;
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * saEvtEventUnsubscribe Handler
 */
static void lib_evt_event_unsubscribe(void *conn, void *message)
{
	struct req_evt_event_unsubscribe *req;
	struct res_evt_event_unsubscribe res;
	struct event_svr_channel_open	*eco;
	struct event_svr_channel_instance *eci;
	struct event_svr_channel_subscr *ecs;
	SaAisErrorT error = SA_AIS_OK;
	unsigned int ret;
	void *ptr;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);

	req = message;

	log_printf(LOG_LEVEL_DEBUG,
		"saEvtEventUnsubscribe (Unsubscribe request)\n");
	log_printf(LOG_LEVEL_DEBUG, "subscription Id: 0x%llx\n",
		(unsigned long long)req->icu_sub_id);

	/*
	 * look up the channel handle, get the open channel
	 * data.
	 */
	ret = hdb_handle_get(&esip->esi_hdb,
						req->icu_channel_handle, &ptr);
	if (ret != 0) {
		error = SA_AIS_ERR_BAD_HANDLE;
		goto unsubr_done;
	}
	eco = ptr;

	eci = eco->eco_channel;

	/*
	 * Make sure that the id exists.
	 */
	ecs = find_subscr(eco, req->icu_sub_id);
	if (!ecs) {
		error = SA_AIS_ERR_NOT_EXIST;
		goto unsubr_put;
	}

	list_del(&ecs->ecs_entry);

	log_printf(LOG_LEVEL_DEBUG,
			"unsubscribe from channel %s subscription ID 0x%x "
			"with %d filters\n",
			eci->esc_channel_name.value,
			ecs->ecs_sub_id, (int)ecs->ecs_filters->filters_number);

	free_filters(ecs->ecs_filters);
	free(ecs);

unsubr_put:
	hdb_handle_put(&esip->esi_hdb, req->icu_channel_handle);
unsubr_done:
	res.icu_head.size = sizeof(res);
	res.icu_head.id = MESSAGE_RES_EVT_UNSUBSCRIBE;
	res.icu_head.error = error;
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * saEvtEventPublish Handler
 */
static void lib_evt_event_publish(void *conn, void *message)
{
	struct lib_event_data *req;
	struct res_evt_event_publish res;
	struct event_svr_channel_open	*eco;
	struct event_svr_channel_instance *eci;
	mar_evteventid_t event_id = 0;
	uint64_t msg_id = 0;
	SaAisErrorT error = SA_AIS_OK;
	struct iovec pub_iovec;
	void *ptr;
	int result;
	unsigned int ret;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);


	req = message;

	log_printf(LOG_LEVEL_DEBUG,
			"saEvtEventPublish (Publish event request)\n");


	/*
	 * look up and validate open channel info
	 */
	ret = hdb_handle_get(&esip->esi_hdb,
				    req->led_svr_channel_handle, &ptr);
	if (ret != 0) {
		error = SA_AIS_ERR_BAD_HANDLE;
		goto pub_done;
	}
	eco = ptr;

	eci = eco->eco_channel;

	/*
	 * modify the request structure for sending event data to subscribed
	 * processes.
	 */
	get_event_id(&event_id, &msg_id);
	req->led_head.id = SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_EVENTDATA);
	req->led_chan_name = eci->esc_channel_name;
	req->led_event_id = event_id;
	req->led_msg_id = msg_id;
	req->led_chan_unlink_id = eci->esc_unlink_id;

	/*
	 * Distribute the event.
	 * The multicasted event will be picked up and delivered
	 * locally by the local network event receiver.
	 */
	pub_iovec.iov_base = req;
	pub_iovec.iov_len = req->led_head.size;
	result = totempg_groups_mcast_joined(openais_group_handle, &pub_iovec, 1, TOTEMPG_AGREED);
	if (result != 0) {
			error = SA_AIS_ERR_LIBRARY;
	}

	hdb_handle_put(&esip->esi_hdb, req->led_svr_channel_handle);
pub_done:
	res.iep_head.size = sizeof(res);
	res.iep_head.id = MESSAGE_RES_EVT_PUBLISH;
	res.iep_head.error = error;
	res.iep_event_id = event_id;
	openais_response_send(conn, &res, sizeof(res));
}

/*
 * saEvtEventRetentionTimeClear handler
 */
static void lib_evt_event_clear_retentiontime(void *conn, void *message)
{
	struct req_evt_event_clear_retentiontime *req;
	struct res_evt_event_clear_retentiontime res;
	struct req_evt_chan_command cpkt;
	struct retention_time_clear_pending *rtc = 0;
	struct iovec rtn_iovec;
	SaAisErrorT error;
	int ret;

	req = message;

	log_printf(RETENTION_TIME_DEBUG,
		"saEvtEventRetentionTimeClear (Clear event retentiontime request)\n");
	log_printf(RETENTION_TIME_DEBUG,
		"event ID 0x%llx, chan handle 0x%x\n",
		(unsigned long long)req->iec_event_id,
		req->iec_channel_handle);

	rtc = malloc(sizeof(*rtc));
	if (!rtc) {
		log_printf(LOG_LEVEL_ERROR,
			"saEvtEventRetentionTimeClear: Memory allocation failure\n");
		error = SA_AIS_ERR_TRY_AGAIN;
		goto evt_ret_clr_err;
	}
	rtc->rtc_event_id = req->iec_event_id;
	rtc->rtc_conn = conn;
	list_init(&rtc->rtc_entry);
	list_add_tail(&rtc->rtc_entry, &clear_pending);

	/*
	 * Send the clear request
	 */
	memset(&cpkt, 0, sizeof(cpkt));
	cpkt.chc_head.id =
		SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
	cpkt.chc_head.size = sizeof(cpkt);
	cpkt.chc_op = EVT_CLEAR_RET_OP;
	cpkt.u.chc_event_id = req->iec_event_id;
	rtn_iovec.iov_base = &cpkt;
	rtn_iovec.iov_len = cpkt.chc_head.size;
	ret = totempg_groups_mcast_joined(openais_group_handle,
			&rtn_iovec, 1, TOTEMPG_AGREED);
	if (ret == 0) {
		// TODO this should really assert
		return;
	}
	error = SA_AIS_ERR_LIBRARY;

evt_ret_clr_err:
	if (rtc) {
		list_del(&rtc->rtc_entry);
		free(rtc);
	}
	res.iec_head.size = sizeof(res);
	res.iec_head.id = MESSAGE_RES_EVT_CLEAR_RETENTIONTIME;
	res.iec_head.error = error;
	openais_response_send(conn, &res, sizeof(res));

}

/*
 * Send requested event data to the application
 */
static void lib_evt_event_data_get(void *conn, void *message)
{
	struct lib_event_data res;
	struct chan_event_list *cel;
	struct event_data *edp;
	int i;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(conn);


	/*
	 * Deliver events in publish order within priority
	 */
	for (i = SA_EVT_HIGHEST_PRIORITY; i <= SA_EVT_LOWEST_PRIORITY; i++) {
		if (!list_empty(&esip->esi_events[i])) {
			cel = list_entry(esip->esi_events[i].next, struct chan_event_list,
						cel_entry);
			list_del(&cel->cel_entry);
			list_init(&cel->cel_entry);
			esip->esi_nevents--;
			if (esip->esi_queue_blocked &&
					(esip->esi_nevents < evt_delivery_queue_resume)) {
				esip->esi_queue_blocked = 0;
				log_printf(LOG_LEVEL_DEBUG, "unblock\n");
			}
			edp = cel->cel_event;
			edp->ed_event.led_lib_channel_handle = cel->cel_chan_handle;
			edp->ed_event.led_sub_id = cel->cel_sub_id;
			edp->ed_event.led_head.id = MESSAGE_RES_EVT_EVENT_DATA;
			edp->ed_event.led_head.error = SA_AIS_OK;
			free(cel);
			openais_response_send(conn, &edp->ed_event,
											edp->ed_event.led_head.size);
			free_event_data(edp);
			goto data_get_done;
		}
	}

	res.led_head.size = sizeof(res.led_head);
	res.led_head.id = MESSAGE_RES_EVT_EVENT_DATA;
	res.led_head.error = SA_AIS_ERR_NOT_EXIST;
	openais_response_send(conn, &res, res.led_head.size);

	/*
	 * See if there are any events that the app doesn't know about
	 * because the notify pipe was full.
	 */
data_get_done:
	if (esip->esi_nevents) {
		__notify_event(conn);
	}
}

/*
 * Scan the list of channels and remove the specified node.
 */
static void remove_chan_open_info(mar_uint32_t node_id)
{
	struct list_head *l, *nxt;
	struct event_svr_channel_instance *eci;

	for (l = esc_head.next; l != &esc_head; l = nxt) {
		nxt = l->next;
		eci = list_entry(l, struct event_svr_channel_instance, esc_entry);
		remove_open_count(eci, node_id);

	}
}


/*
 * Called when there is a configuration change in the cluster.
 * This function looks at any joiners and leavers and updates the evt
 * node list.  The node list is used to keep track of event IDs
 * received for each node for the detection of duplicate events.
 */
static void evt_conf_change(
		enum totem_configuration_type configuration_type,
		unsigned int *member_list, int member_list_entries,
		unsigned int *left_list, int left_list_entries,
		unsigned int *joined_list, int joined_list_entries,
		struct memb_ring_id *ring_id)
{
	log_printf(RECOVERY_DEBUG, "Evt conf change %d\n",
			configuration_type);
	log_printf(RECOVERY_DEBUG, "m %d, j %d, l %d\n",
					member_list_entries,
					joined_list_entries,
					left_list_entries);

	/*
	 * Save the various membership lists for later processing by
	 * the synchronization functions.  The left list is only
	 * valid in the transitional configuration, the joined list is
	 * only valid in the regular configuration.  Other than for the
	 * purposes of delivering retained events from merging partitions,
	 * we only care about the final membership from the regular
	 * configuration.
	 */
	if (configuration_type == TOTEM_CONFIGURATION_TRANSITIONAL) {

		left_member_count = left_list_entries;
		trans_member_count = member_list_entries;

		if (left_member_list) {
			free(left_member_list);
			left_member_list = 0;
		}
		if (left_list_entries) {
			left_member_list =
				malloc(sizeof(unsigned int) * left_list_entries);
			if (!left_member_list) {
				/*
				 * ERROR: No recovery.
				 */
				log_printf(LOG_LEVEL_ERROR,
						"Config change left list allocation error\n");
				assert(0);
			}
			memcpy(left_member_list, left_list,
					sizeof(unsigned int) * left_list_entries);
		}

		if (trans_member_list) {
			free(trans_member_list);
			trans_member_list = 0;
		}
		if (member_list_entries) {
			trans_member_list =
				malloc(sizeof(unsigned int) * member_list_entries);

			if (!trans_member_list) {
				/*
				 * ERROR: No recovery.
				 */
				log_printf(LOG_LEVEL_ERROR,
				  "Config change transitional member list allocation error\n");
				assert(0);
			}
			memcpy(trans_member_list, member_list,
					sizeof(unsigned int) * member_list_entries);
		}
	}

	if (configuration_type == TOTEM_CONFIGURATION_REGULAR) {

		joined_member_count = joined_list_entries;
		total_member_count = member_list_entries;

		if (joined_member_list) {
			free(joined_member_list);
			joined_member_list = 0;
		}
		if (joined_list_entries) {
			joined_member_list =
				malloc(sizeof(unsigned int) * joined_list_entries);
			if (!joined_member_list) {
				/*
				 * ERROR: No recovery.
				 */
				log_printf(LOG_LEVEL_ERROR,
						"Config change joined list allocation error\n");
				assert(0);
			}
			memcpy(joined_member_list, joined_list,
					sizeof(unsigned int) * joined_list_entries);
		}


		if (current_member_list) {
			free(current_member_list);
			current_member_list = 0;
		}
		if (member_list_entries) {
			current_member_list =
				malloc(sizeof(unsigned int) * member_list_entries);

			if (!current_member_list) {
				/*
				 * ERROR: No recovery.
				 */
				log_printf(LOG_LEVEL_ERROR,
						"Config change member list allocation error\n");
				assert(0);
			}
			memcpy(current_member_list, member_list,
					sizeof(unsigned int) * member_list_entries);
		}
	}
}

/*
 * saEvtFinalize Handler
 */
static int evt_lib_exit(void *conn)
{

	struct event_svr_channel_open	*eco;
	struct list_head *l, *nxt;
	struct open_chan_pending *ocp;
	struct unlink_chan_pending *ucp;
	struct retention_time_clear_pending *rtc;
	struct libevt_pd *esip =
		openais_conn_private_data_get(conn);

	log_printf(LOG_LEVEL_DEBUG, "saEvtFinalize (Event exit request)\n");
	log_printf(LOG_LEVEL_DEBUG, "saEvtFinalize %d evts on list\n",
			esip->esi_nevents);

	/*
	 * Clean up any open channels and associated subscriptions.
	 */
	for (l = esip->esi_open_chans.next; l != &esip->esi_open_chans; l = nxt) {
		nxt = l->next;
		eco = list_entry(l, struct event_svr_channel_open, eco_instance_entry);
		common_chan_close(eco, esip);
		hdb_handle_destroy(&esip->esi_hdb, eco->eco_my_handle);
	}

	/*
	 * Clean up any pending async operations
	 */
	for (l = open_pending.next; l != &open_pending; l = nxt) {
		nxt = l->next;
		ocp = list_entry(l, struct open_chan_pending, ocp_entry);
		if (esip == openais_conn_private_data_get(ocp->ocp_conn)) {
			list_del(&ocp->ocp_entry);
			free(ocp);
		}
	}

	for (l = unlink_pending.next; l != &unlink_pending; l = nxt) {
		nxt = l->next;
		ucp = list_entry(l, struct unlink_chan_pending, ucp_entry);
		if (esip == openais_conn_private_data_get(ucp->ucp_conn)) {
			list_del(&ucp->ucp_entry);
			free(ucp);
		}
	}

	for (l = clear_pending.next;
			l != &clear_pending; l = nxt) {
		nxt = l->next;
		rtc = list_entry(l, struct retention_time_clear_pending, rtc_entry);
		if (esip == openais_conn_private_data_get(rtc->rtc_conn)) {
			list_del(&rtc->rtc_entry);
			free(rtc);
		}
	}

	/*
	 * Destroy the open channel handle database
	 */
	hdb_destroy(&esip->esi_hdb);

	return 0;
}

/*
 * Called at service start time.
 */
static int evt_exec_init(struct objdb_iface_ver0 *objdb)
{
	unsigned int object_service_handle;
	char *value;

	log_init ("EVT");
	log_printf(LOG_LEVEL_DEBUG, "Evt exec init request\n");

	objdb->object_find_reset (OBJECT_PARENT_HANDLE);
	if (objdb->object_find (
		    OBJECT_PARENT_HANDLE,
		    "event",
		    strlen ("event"),
		    &object_service_handle) == 0) {

		value = NULL;
		if ( !objdb->object_key_get (object_service_handle,
					     "delivery_queue_size",
					     strlen ("delivery_queue_size"),
					     (void *)&value,
					     NULL) && value) {
			evt_delivery_queue_size = atoi(value);
			log_printf(LOG_LEVEL_NOTICE,
				   "event delivery_queue_size set to %u\n",
				   evt_delivery_queue_size);
		}
		value = NULL;
		if ( !objdb->object_key_get (object_service_handle,
					     "delivery_queue_resume",
					     strlen ("delivery_queue_resume"),
					     (void *)&value,
					     NULL) && value) {
			evt_delivery_queue_resume = atoi(value);
			log_printf(LOG_LEVEL_NOTICE,
				   "event delivery_queue_resume set to %u\n",
				   evt_delivery_queue_size);
		}
	}

	/*
	 * Create an event to be sent when we have to drop messages
	 * for an application.
	 */
	dropped_event_size = sizeof(*dropped_event) + sizeof(dropped_pattern);
	dropped_event = malloc(dropped_event_size);
	if (dropped_event == 0) {
		log_printf(LOG_LEVEL_ERROR,
				"Memory Allocation Failure, event service not started\n");
		errno = ENOMEM;
		return -1;
	}
	memset(dropped_event, 0, sizeof(*dropped_event) + sizeof(dropped_pattern));
	dropped_event->ed_ref_count = 1;
	list_init(&dropped_event->ed_retained);
	dropped_event->ed_event.led_head.size =
			sizeof(*dropped_event) + sizeof(dropped_pattern);
	dropped_event->ed_event.led_head.error = SA_AIS_OK;
	dropped_event->ed_event.led_priority = SA_EVT_HIGHEST_PRIORITY;
	dropped_event->ed_event.led_chan_name = lost_chan;
	dropped_event->ed_event.led_publisher_name = dropped_publisher;
	dropped_event->ed_event.led_patterns_number = 1;
	memcpy(&dropped_event->ed_event.led_body[0],
					&dropped_pattern, sizeof(dropped_pattern));
	return 0;
}

static int
try_deliver_event(struct event_data *evt,
		struct event_svr_channel_instance *eci)
{
	struct list_head *l, *l1;
	struct event_svr_channel_open *eco;
	struct event_svr_channel_subscr *ecs;
	int delivered_event = 0;
	/*
	 * Check open channels
	 */
	for (l = eci->esc_open_chans.next; l != &eci->esc_open_chans; l = l->next) {
		eco = list_entry(l, struct event_svr_channel_open, eco_entry);
		/*
		 * See if enabled to receive
		 */
		if (!(eco->eco_flags & SA_EVT_CHANNEL_SUBSCRIBER)) {
				continue;
		}

		/*
		 * Check subscriptions
		 */
		for (l1 = eco->eco_subscr.next; l1 != &eco->eco_subscr; l1 = l1->next) {
			ecs = list_entry(l1, struct event_svr_channel_subscr, ecs_entry);
			/*
			 * Apply filter rules and deliver if patterns
			 * match filters.
			 * Only deliver one event per open channel
			 */
			if (event_match(evt, ecs) == SA_AIS_OK) {
				deliver_event(evt, eco, ecs);
				delivered_event++;
				break;
			}
		}
	}
	return delivered_event;
}

/*
 * Receive the network event message and distribute it to local subscribers
 */
static void evt_remote_evt(void *msg, unsigned int nodeid)
{
	/*
	 * - retain events that have a retention time
	 * - Find assocated channel
	 * - Scan list of subscribers
	 * - Apply filters
	 * - Deliver events that pass the filter test
	 */
	struct lib_event_data *evtpkt = msg;
	struct event_svr_channel_instance *eci;
	struct event_data *evt;
	SaClmClusterNodeT *cn;

	log_printf(LOG_LEVEL_DEBUG, "Remote event data received from nodeid %s\n",
			totempg_ifaces_print (nodeid));

	/*
	 * See where the message came from so that we can set the
	 * publishing node id in the message before delivery.
	 */
	cn = main_clm_get_by_nodeid(nodeid);
	if (!cn) {
			/*
			 * Not sure how this can happen...
			 */
			log_printf(LOG_LEVEL_DEBUG, "No cluster node data for nodeid %s\n",
				totempg_ifaces_print (nodeid));
			errno = ENXIO;
			return;
	}
	log_printf(LOG_LEVEL_DEBUG, "Cluster node ID %s name %s\n",
					totempg_ifaces_print (cn->nodeId), cn->nodeName.value);

	evtpkt->led_publisher_node_id = nodeid;
	evtpkt->led_nodeid = nodeid;
	evtpkt->led_receive_time = clust_time_now();

	if (evtpkt->led_chan_unlink_id != EVT_CHAN_ACTIVE) {
		log_printf(CHAN_UNLINK_DEBUG,
			"evt_remote_evt(0): chan %s, id 0x%llx\n",
			evtpkt->led_chan_name.value,
			(unsigned long long)evtpkt->led_chan_unlink_id);
	}
	eci = find_channel(&evtpkt->led_chan_name, evtpkt->led_chan_unlink_id);
	/*
	 * We may have had some events that were already queued when an
	 * unlink happened, if we don't find the channel in the active list
	 * look for the last unlinked channel of the same name.  If this channel
	 * is re-opened the messages will still be routed correctly because new
	 * active channel messages will be ordered behind the open.
	 */
	if (!eci && (evtpkt->led_chan_unlink_id == EVT_CHAN_ACTIVE)) {
		log_printf(CHAN_UNLINK_DEBUG,
			"evt_remote_evt(1): chan %s, id 0x%llx\n",
			evtpkt->led_chan_name.value,
			(unsigned long long)evtpkt->led_chan_unlink_id);
		eci = find_last_unlinked_channel(&evtpkt->led_chan_name);
	}

	/*
	 * We shouldn't normally see an event for a channel that we
	 * don't know about.
	 */
	if (!eci) {
		log_printf(LOG_LEVEL_DEBUG, "Channel %s doesn't exist\n",
				evtpkt->led_chan_name.value);
		return;
	}

	if (check_last_event(evtpkt, nodeid)) {
		return;
	}

	evt = make_local_event(evtpkt, eci);
	if (!evt) {
		log_printf(LOG_LEVEL_WARNING,
						"1Memory allocation error, can't deliver event\n");
		return;
	}

	if (evt->ed_event.led_retention_time) {
		retain_event(evt);
	}

	try_deliver_event(evt, eci);
	free_event_data(evt);
}

/*
 * Calculate the remaining retention time of a received event during recovery
 */
inline mar_time_t calc_retention_time(mar_time_t retention,
								mar_time_t received, mar_time_t now)
{
	if ((received < now) && ((now - received) < retention)) {
		return retention - (now - received);
	} else {
		return 0;
	}
}

/*
 * Receive a recovery network event message and save it in the retained list
 */
static void evt_remote_recovery_evt(void *msg, unsigned int nodeid)
{
	/*
	 * - calculate remaining retention time
	 * - Find assocated channel
	 * - Scan list of subscribers
	 * - Apply filters
	 * - Deliver events that pass the filter test
	 */
	struct lib_event_data *evtpkt = msg;
	struct event_svr_channel_instance *eci;
	struct event_data *evt;
	struct member_node_data *md;
	int num_delivered;
	mar_time_t now;

	now = clust_time_now();

	log_printf(RECOVERY_EVENT_DEBUG,
			"Remote recovery event data received from nodeid %d\n", nodeid);

	if (recovery_phase == evt_recovery_complete) {
		log_printf(RECOVERY_EVENT_DEBUG,
			"Received recovery data, not in recovery mode\n");
		return;
	}

	log_printf(RECOVERY_EVENT_DEBUG,
		"Processing recovery of retained events\n");
	if (recovery_node) {
		log_printf(RECOVERY_EVENT_DEBUG, "This node is the recovery node\n");
	}

	log_printf(RECOVERY_EVENT_DEBUG, "(1)EVT ID: %llx, Time: %llx\n",
		(unsigned long long)evtpkt->led_event_id,
		(unsigned long long)evtpkt->led_retention_time);
	/*
	 * Calculate remaining retention time
	 */
	evtpkt->led_retention_time = calc_retention_time(
				evtpkt->led_retention_time,
				evtpkt->led_receive_time,
				now);

	log_printf(RECOVERY_EVENT_DEBUG,
		"(2)EVT ID: %llx, ret: %llx, rec: %llx, now: %llx\n",
		(unsigned long long)evtpkt->led_event_id,
		(unsigned long long)evtpkt->led_retention_time,
		(unsigned long long)evtpkt->led_receive_time,
		(unsigned long long)now);

	/*
	 * If we haven't seen this event yet and it has remaining time, process
	 * the event.
	 */
	if (!check_last_event(evtpkt, evtpkt->led_nodeid) &&
		evtpkt->led_retention_time) {
		/*
		 * See where the message came from so that we can set the
		 * publishing node id in the message before delivery.
		 */
		md = evt_find_node(evtpkt->led_nodeid);
		if (!md) {
				/*
				 * Not sure how this can happen
				 */
				log_printf(LOG_LEVEL_NOTICE, "No node for nodeid %s\n",
						totempg_ifaces_print (evtpkt->led_nodeid));
				return;
		}
		log_printf(LOG_LEVEL_DEBUG, "Cluster node ID %s name %s\n",
						totempg_ifaces_print (md->mn_node_info.nodeId),
						md->mn_node_info.nodeName.value);

		log_printf(CHAN_UNLINK_DEBUG,
			"evt_recovery_event: chan %s, id 0x%llx\n",
			evtpkt->led_chan_name.value,
			(unsigned long long)evtpkt->led_chan_unlink_id);
		eci = find_channel(&evtpkt->led_chan_name, evtpkt->led_chan_unlink_id);

		/*
		 * We shouldn't normally see an event for a channel that we don't
		 * know about.
		 */
		if (!eci) {
			log_printf(RECOVERY_EVENT_DEBUG, "Channel %s doesn't exist\n",
				evtpkt->led_chan_name.value);
			return;
		}

		evt = make_local_event(evtpkt, eci);
		if (!evt) {
			log_printf(LOG_LEVEL_WARNING,
				"2Memory allocation error, can't deliver event\n");
			errno = ENOMEM;
			return;
		}

		retain_event(evt);
		num_delivered = try_deliver_event(evt, eci);
		log_printf(RECOVERY_EVENT_DEBUG, "Delivered to %d subscribers\n",
				num_delivered);
		free_event_data(evt);
	}
}


/*
 * Timeout handler for event channel open.  We flag the structure
 * as timed out.  Then if the open request is ever returned, we can
 * issue a close channel and keep the reference counting correct.
 */
static void chan_open_timeout(void *data)
{
	struct open_chan_pending *ocp = (struct open_chan_pending *)data;
	struct res_evt_channel_open res;

	res.ico_head.size = sizeof(res);
	res.ico_head.id = MESSAGE_RES_EVT_OPEN_CHANNEL;
	res.ico_head.error = SA_AIS_ERR_TIMEOUT;
	ocp->ocp_invocation = OPEN_TIMED_OUT;
	openais_response_send(ocp->ocp_conn, &res, sizeof(res));
}

/*
 * Called by the channel open exec handler to finish the open and
 * respond to the application
 */
static void evt_chan_open_finish(struct open_chan_pending *ocp,
		struct event_svr_channel_instance *eci)
{
	struct event_svr_channel_open *eco;
	SaAisErrorT error = SA_AIS_OK;
	unsigned int ret = 0;
	unsigned int timer_del_status = 0;
	void *ptr = 0;
	uint32_t handle = 0;
	struct libevt_pd *esip;

	esip = (struct libevt_pd *)openais_conn_private_data_get(ocp->ocp_conn);

	log_printf(CHAN_OPEN_DEBUG, "Open channel finish %s\n",
											get_mar_name_t(&ocp->ocp_chan_name));
	if (ocp->ocp_timer_handle) {
		openais_timer_delete (ocp->ocp_timer_handle);
	}

	/*
	 * If this is a finished open for a timed out request, then
	 * send out a close on this channel to clean things up.
	 */
	if (ocp->ocp_invocation == OPEN_TIMED_OUT) {
		log_printf(CHAN_OPEN_DEBUG, "Closing timed out open of %s\n",
				get_mar_name_t(&ocp->ocp_chan_name));
		error = evt_close_channel(&ocp->ocp_chan_name, EVT_CHAN_ACTIVE);
		if (error != SA_AIS_OK) {
			log_printf(CHAN_OPEN_DEBUG,
					"Close of timed out open failed for %s\n",
					get_mar_name_t(&ocp->ocp_chan_name));
		}
		list_del(&ocp->ocp_entry);
		free(ocp);
		return;
	}

	/*
	 * Create a handle to give back to the caller to associate
	 * with this channel open instance.
	 */
	ret = hdb_handle_create(&esip->esi_hdb, sizeof(*eco), &handle);
	if (ret != 0) {
		goto open_return;
	}
	ret = hdb_handle_get(&esip->esi_hdb, handle, &ptr);
	if (ret != 0) {
		goto open_return;
	}
	eco = ptr;

	/*
	 * Initailize and link into the global channel structure.
	 */
	list_init(&eco->eco_subscr);
	list_init(&eco->eco_entry);
	list_init(&eco->eco_instance_entry);
	eco->eco_flags = ocp->ocp_open_flag;
	eco->eco_channel = eci;
	eco->eco_lib_handle = ocp->ocp_c_handle;
	eco->eco_my_handle = handle;
	eco->eco_conn = ocp->ocp_conn;
	list_add_tail(&eco->eco_entry, &eci->esc_open_chans);
	list_add_tail(&eco->eco_instance_entry, &esip->esi_open_chans);

	/*
	 * respond back with a handle to access this channel
	 * open instance for later subscriptions, etc.
	 */
	hdb_handle_put(&esip->esi_hdb, handle);

open_return:
	log_printf(CHAN_OPEN_DEBUG, "Open channel finish %s send response %d\n",
											get_mar_name_t(&ocp->ocp_chan_name),
											error);
	if (ocp->ocp_async) {
		struct res_evt_open_chan_async resa;
		resa.ica_head.size = sizeof(resa);
		resa.ica_head.id = MESSAGE_RES_EVT_CHAN_OPEN_CALLBACK;
		resa.ica_head.error = (ret == 0 ? SA_AIS_OK: SA_AIS_ERR_BAD_HANDLE);
		resa.ica_channel_handle = handle;
		resa.ica_c_handle = ocp->ocp_c_handle;
		resa.ica_invocation = ocp->ocp_invocation;
		openais_dispatch_send(ocp->ocp_conn, &resa, sizeof(resa));
	} else {
		struct res_evt_channel_open res;
		res.ico_head.size = sizeof(res);
		res.ico_head.id = MESSAGE_RES_EVT_OPEN_CHANNEL;
		res.ico_head.error = (ret == 0 ? SA_AIS_OK : SA_AIS_ERR_BAD_HANDLE);
		res.ico_channel_handle = handle;
		openais_response_send(ocp->ocp_conn, &res, sizeof(res));
	}

	if (timer_del_status == 0) {
		list_del(&ocp->ocp_entry);
		free(ocp);
	}
}

/*
 * Called by the channel unlink exec handler to
 * respond to the application.
 */
static void evt_chan_unlink_finish(struct unlink_chan_pending *ucp)
{
	struct res_evt_channel_unlink res;

	log_printf(CHAN_UNLINK_DEBUG, "Unlink channel finish ID 0x%llx\n",
		(unsigned long long)ucp->ucp_unlink_id);

	list_del(&ucp->ucp_entry);

	res.iuc_head.size = sizeof(res);
	res.iuc_head.id = MESSAGE_RES_EVT_UNLINK_CHANNEL;
	res.iuc_head.error = SA_AIS_OK;
	openais_response_send(ucp->ucp_conn, &res, sizeof(res));

	free(ucp);
}

/*
 * Called by the retention time clear exec handler to
 * respond to the application.
 */
static void evt_ret_time_clr_finish(struct retention_time_clear_pending *rtc,
		SaAisErrorT ret)
{
	struct res_evt_event_clear_retentiontime res;

	log_printf(RETENTION_TIME_DEBUG, "Retention Time Clear finish ID 0x%llx\n",
		(unsigned long long)rtc->rtc_event_id);

	res.iec_head.size = sizeof(res);
	res.iec_head.id = MESSAGE_RES_EVT_CLEAR_RETENTIONTIME;
	res.iec_head.error = ret;
	openais_response_send(rtc->rtc_conn, &res, sizeof(res));

	list_del(&rtc->rtc_entry);
	free(rtc);
}

/*
 * Take the channel command data and swap the elements so they match
 * our architectures word order.
 */
static void
convert_chan_packet(void *msg)
{
	struct req_evt_chan_command *cpkt = (struct req_evt_chan_command *)msg;

	/*
	 * converted in the main deliver_fn:
	 * led_head.id, led_head.size.
	 *
	 */

	cpkt->chc_op = swab32(cpkt->chc_op);

	/*
	 * Which elements of the packet that are converted depend
	 * on the operation.
	 */
	switch (cpkt->chc_op) {

	case EVT_OPEN_CHAN_OP:
		swab_mar_name_t (&cpkt->u.chc_chan.ocr_name);
		cpkt->u.chc_chan.ocr_serial_no = swab64(cpkt->u.chc_chan.ocr_serial_no);
		break;

	case EVT_UNLINK_CHAN_OP:
	case EVT_CLOSE_CHAN_OP:
		swab_mar_name_t (&cpkt->u.chcu.chcu_name);
		cpkt->u.chcu.chcu_unlink_id = swab64(cpkt->u.chcu.chcu_unlink_id);
		break;

	case EVT_CLEAR_RET_OP:
		cpkt->u.chc_event_id = swab64(cpkt->u.chc_event_id);
		break;

	case EVT_SET_ID_OP:
		cpkt->u.chc_set_id.chc_nodeid = 
			swab32(cpkt->u.chc_set_id.chc_nodeid);
		cpkt->u.chc_set_id.chc_last_id = swab64(cpkt->u.chc_set_id.chc_last_id);
		break;

	case EVT_OPEN_COUNT:
		swab_mar_name_t (&cpkt->u.chc_set_opens.chc_chan_name);
		cpkt->u.chc_set_opens.chc_open_count = 
			swab32(cpkt->u.chc_set_opens.chc_open_count);
		break;

	/* 
	 * No data assocaited with these ops.
	 */
	case EVT_CONF_DONE:
	case EVT_OPEN_COUNT_DONE:
		break;

	/*
	 * Make sure that this function is updated when new ops are added.
	 */
	default:
		assert(0);
	}
}


/*
 * Receive and process remote event operations.
 * Used to communicate channel opens/closes, clear retention time,
 * config change updates...
 */
static void evt_remote_chan_op(void *msg, unsigned int nodeid)
{
	struct req_evt_chan_command *cpkt = msg;
	unsigned int local_node = {SA_CLM_LOCAL_NODE_ID};
	SaClmClusterNodeT *cn, *my_node;
	struct member_node_data *mn;
	struct event_svr_channel_instance *eci;

	log_printf(REMOTE_OP_DEBUG, "Remote channel operation request\n");
	my_node = main_clm_get_by_nodeid(local_node);
	log_printf(REMOTE_OP_DEBUG, "my node ID: 0x%x\n", my_node->nodeId);

	mn = evt_find_node(nodeid);
	if (mn == NULL) {
		cn = main_clm_get_by_nodeid(nodeid);
		if (cn == NULL) {
			log_printf(LOG_LEVEL_WARNING,
				"Evt remote channel op: Node data for nodeid %d is NULL\n",
				nodeid);
			return;
		} else {
			evt_add_node(nodeid, cn);
			mn = evt_find_node(nodeid);
		}
	}

	switch (cpkt->chc_op) {
		/*
		 * Open channel remote command.  The open channel request is done
		 * in two steps.  When an pplication tries to open, we send an open
		 * channel message to the other nodes. When we receive an open channel
		 * message, we may create the channel structure if it doesn't exist
		 * and also complete applicaiton open requests for the specified
		 * channel.  We keep a counter of total opens for the given channel and
		 * later when it has been completely closed (everywhere in the cluster)
		 * we will free up the assocated channel data.
		 */
	case EVT_OPEN_CHAN_OP: {
		struct open_chan_pending *ocp;
		struct list_head *l, *nxt;

		log_printf(CHAN_OPEN_DEBUG, "Opening channel %s for node %s\n",
						cpkt->u.chc_chan.ocr_name.value,
						totempg_ifaces_print (mn->mn_node_info.nodeId));
		eci = find_channel(&cpkt->u.chc_chan.ocr_name, EVT_CHAN_ACTIVE);

		if (!eci) {
			eci = create_channel(&cpkt->u.chc_chan.ocr_name);
		}
		if (!eci) {
			log_printf(LOG_LEVEL_WARNING, "Could not create channel %s\n",
				   get_mar_name_t(&cpkt->u.chc_chan.ocr_name));
			break;
		}

		inc_open_count(eci, mn->mn_node_info.nodeId);

		if (mn->mn_node_info.nodeId == my_node->nodeId) {
			/*
			 * Complete one of our pending open requests
			 */
			for (l = open_pending.next; l != &open_pending; l = nxt) {
				nxt = l->next;
				ocp = list_entry(l, struct open_chan_pending, ocp_entry);
				log_printf(CHAN_OPEN_DEBUG,
					"Compare channel req no %llu %llu\n",
					(unsigned long long)ocp->ocp_serial_no,
					(unsigned long long)cpkt->u.chc_chan.ocr_serial_no);
				if (ocp->ocp_serial_no == cpkt->u.chc_chan.ocr_serial_no) {
					evt_chan_open_finish(ocp, eci);
					break;
				}
			}
		}
		log_printf(CHAN_OPEN_DEBUG,
				"Open channel %s t %d, l %d, r %d\n",
				get_mar_name_t(&eci->esc_channel_name),
				eci->esc_total_opens, eci->esc_local_opens,
				eci->esc_retained_count);
		break;
	}

	/*
	 * Handle a channel close. We'll decrement the global open counter and
	 * free up channel associated data when all instances are closed.
	 */
	case EVT_CLOSE_CHAN_OP:
		log_printf(LOG_LEVEL_DEBUG, "Closing channel %s for node 0x%x\n",
						cpkt->u.chcu.chcu_name.value, mn->mn_node_info.nodeId);
		eci = find_channel(&cpkt->u.chcu.chcu_name, cpkt->u.chcu.chcu_unlink_id);
		if (!eci) {
			log_printf(LOG_LEVEL_NOTICE,
					"Channel close request for %s not found\n",
				cpkt->u.chcu.chcu_name.value);
			break;
		}

		/*
		 * if last instance, we can free up assocated data.
		 */
		dec_open_count(eci, mn->mn_node_info.nodeId);
		log_printf(LOG_LEVEL_DEBUG,
				"Close channel %s t %d, l %d, r %d\n",
				eci->esc_channel_name.value,
				eci->esc_total_opens, eci->esc_local_opens,
				eci->esc_retained_count);
		delete_channel(eci);
		break;

	/*
	 * Handle a request for channel unlink saEvtChannelUnlink().
	 * We'll look up the channel and mark it as unlinked.  Respond to
	 * local library unlink commands.
	 */
	case EVT_UNLINK_CHAN_OP: {
		struct unlink_chan_pending *ucp;
		struct list_head *l, *nxt;

		log_printf(CHAN_UNLINK_DEBUG,
			"Unlink request channel %s unlink ID 0x%llx from node 0x%x\n",
			cpkt->u.chcu.chcu_name.value,
			(unsigned long long)cpkt->u.chcu.chcu_unlink_id,
			mn->mn_node_info.nodeId);


		/*
		 * look up the channel name and get its assoicated data.
		 */
		eci = find_channel(&cpkt->u.chcu.chcu_name,
				EVT_CHAN_ACTIVE);
		if (!eci) {
			log_printf(LOG_LEVEL_NOTICE,
					"Channel unlink request for %s not found\n",
				cpkt->u.chcu.chcu_name.value);
		}  else {
			/*
			 * Mark channel as unlinked.
			 */
			unlink_channel(eci, cpkt->u.chcu.chcu_unlink_id);
		}

		/*
		 * respond only to local library requests.
		 */
		if (mn->mn_node_info.nodeId == my_node->nodeId) {
			/*
			 * Complete one of our pending unlink requests
			 */
			for (l = unlink_pending.next; l != &unlink_pending; l = nxt) {
				nxt = l->next;
				ucp = list_entry(l, struct unlink_chan_pending, ucp_entry);
				log_printf(CHAN_UNLINK_DEBUG,
					"Compare channel id 0x%llx 0x%llx\n",
					(unsigned long long)ucp->ucp_unlink_id,
					(unsigned long long)cpkt->u.chcu.chcu_unlink_id);
				if (ucp->ucp_unlink_id == cpkt->u.chcu.chcu_unlink_id) {
					evt_chan_unlink_finish(ucp);
					break;
				}
			}
		}
		break;
	}


	/*
	 * saEvtClearRetentionTime handler.
	 */
	case EVT_CLEAR_RET_OP:
	{
		SaAisErrorT did_clear;
		struct retention_time_clear_pending *rtc;
		struct list_head *l, *nxt;

		log_printf(RETENTION_TIME_DEBUG, "Clear retention time request %llx\n",
			(unsigned long long)cpkt->u.chc_event_id);
		did_clear = clear_retention_time(cpkt->u.chc_event_id);

		/*
		 * Respond to local library requests
		 */
		if (mn->mn_node_info.nodeId == my_node->nodeId) {
			/*
			 * Complete pending request
			 */
			for (l = clear_pending.next; l != &clear_pending; l = nxt) {
				nxt = l->next;
				rtc = list_entry(l, struct retention_time_clear_pending,
												rtc_entry);
				if (rtc->rtc_event_id == cpkt->u.chc_event_id) {
					evt_ret_time_clr_finish(rtc, did_clear);
					break;
				}
			}
		}
		break;
	}

	/*
	 * Set our next event ID based on the largest event ID seen
	 * by others in the cluster.  This way, if we've left and re-joined, we'll
	 * start using an event ID that is unique.
	 */
	case EVT_SET_ID_OP: {
		int log_level = LOG_LEVEL_DEBUG;
		if (cpkt->u.chc_set_id.chc_nodeid == my_node->nodeId) {
			log_level = RECOVERY_DEBUG;
		}
		log_printf(log_level,
			"Received Set event ID OP from nodeid %x to %llx for %x my addr %s base %llx\n",
			nodeid,
			(unsigned long long)cpkt->u.chc_set_id.chc_last_id,
			cpkt->u.chc_set_id.chc_nodeid,
			totempg_ifaces_print (my_node->nodeId),
			(unsigned long long)base_id);
		if (cpkt->u.chc_set_id.chc_nodeid == my_node->nodeId) {
			if (cpkt->u.chc_set_id.chc_last_id >= base_id) {
				log_printf(RECOVERY_DEBUG,
					"Set event ID from nodeid %s to %llx\n",
					totempg_ifaces_print (nodeid),
					(unsigned long long)cpkt->u.chc_set_id.chc_last_id);
				base_id = cpkt->u.chc_set_id.chc_last_id + 1;
			}
		}
		break;
	}

	/*
	 * Receive the open count for a particular channel during recovery.
	 * This insures that everyone has the same notion of who has a channel
	 * open so that it can be removed when no one else has it open anymore.
	 */
	case EVT_OPEN_COUNT:
		if (recovery_phase == evt_recovery_complete) {
			log_printf(LOG_LEVEL_ERROR,
				"Evt open count msg from nodeid %s, but not in membership change\n",
				totempg_ifaces_print (nodeid));
		}

		/*
		 * Zero out all open counts because we're setting then based
		 * on each nodes local counts.
		 */
		if (!processed_open_counts) {
			zero_chan_open_counts();
			processed_open_counts = 1;
		}
		log_printf(RECOVERY_DEBUG,
				"Open channel count %s is %d for node %s\n",
				cpkt->u.chc_set_opens.chc_chan_name.value,
				cpkt->u.chc_set_opens.chc_open_count,
				totempg_ifaces_print (mn->mn_node_info.nodeId));

		eci = find_channel(&cpkt->u.chc_set_opens.chc_chan_name,
					EVT_CHAN_ACTIVE);
		if (!eci) {
			eci = create_channel(&cpkt->u.chc_set_opens.chc_chan_name);
		}
		if (!eci) {
			log_printf(LOG_LEVEL_WARNING, "Could not create channel %s\n",
				   get_mar_name_t(&cpkt->u.chc_set_opens.chc_chan_name));
			break;
		}
		if (set_open_count(eci, mn->mn_node_info.nodeId,
			cpkt->u.chc_set_opens.chc_open_count)) {
			log_printf(LOG_LEVEL_ERROR,
				"Error setting Open channel count %s for node %s\n",
				cpkt->u.chc_set_opens.chc_chan_name.value,
				totempg_ifaces_print (mn->mn_node_info.nodeId));
		}
		break;

	/*
	 * Once we get all the messages from
	 * the current membership, determine who delivers any retained events.
	 */
	case EVT_OPEN_COUNT_DONE: {
		if (recovery_phase == evt_recovery_complete) {
			log_printf(LOG_LEVEL_ERROR,
				"Evt config msg from nodeid %s, but not in membership change\n",
				totempg_ifaces_print (nodeid));
		}
		log_printf(RECOVERY_DEBUG,
			"Receive EVT_CONF_CHANGE_DONE from nodeid %s members %d checked in %d\n",
				totempg_ifaces_print (nodeid), total_member_count, checked_in+1);
		if (!mn) {
			log_printf(RECOVERY_DEBUG,
				"NO NODE DATA AVAILABLE FOR nodeid %s\n", totempg_ifaces_print (nodeid));
		}

		if (++checked_in == total_member_count) {
			/*
			 * We're all here, now figure out who should send the
			 * retained events.
			 */
			mn = oldest_node();
			if (mn && mn->mn_node_info.nodeId == my_node_id) {
				log_printf(RECOVERY_DEBUG,
					"I am oldest in my transitional config\n");
				recovery_node = 1;
				recovery_phase = evt_send_retained_events;
			} else {
				recovery_phase = evt_send_retained_events_done;
				recovery_node = 0;
			}
			checked_in = 0;
		}
		break;
	}

	/*
	 * Count up the nodes again, when all the nodes have responded, we've
	 * distributed the retained events and we're done with recovery and can
	 * continue operations.
	 */
	case EVT_CONF_DONE: {
		log_printf(RECOVERY_DEBUG,
				"Receive EVT_CONF_DONE from nodeid %s, members %d checked in %d\n",
				totempg_ifaces_print (nodeid),
				total_member_count, checked_in+1);
		if (++checked_in == total_member_count) {
			/*
			 * All recovery complete, carry on.
			 */
			recovery_phase = evt_recovery_complete;
#ifdef DUMP_CHAN_INFO
			dump_all_chans();
#endif
		}

		break;
	}

	default:
		log_printf(LOG_LEVEL_NOTICE, "Invalid channel operation %d\n",
						cpkt->chc_op);
		break;
	}
}

/*
 * Set up initial conditions for processing event service
 * recovery.
 */
static void evt_sync_init(void)
{
	SaClmClusterNodeT *cn;
	struct member_node_data *md;
	unsigned int my_node = {SA_CLM_LOCAL_NODE_ID};
	int left_list_entries = left_member_count;
	unsigned int *left_list = left_member_list;

	log_printf(RECOVERY_DEBUG, "Evt synchronize initialization\n");

	/*
	 * Set the base event id
	 */
	if (!my_node_id) {
		cn = main_clm_get_by_nodeid(my_node);
		log_printf(RECOVERY_DEBUG, "My node ID %s\n",
			totempg_ifaces_print (cn->nodeId));
		my_node_id = cn->nodeId;
		set_event_id(my_node_id);
	}

	/*
	 * account for nodes that left the membership
	 */
	while (left_list_entries--) {
		md = evt_find_node(*left_list);
		if (md == 0) {
			log_printf(LOG_LEVEL_WARNING,
					"Can't find cluster node at %s\n",
							totempg_ifaces_print (*left_list));
		/*
		 * Mark this one as down.
		 */
		} else {
			log_printf(RECOVERY_DEBUG, "cluster node at %s down\n",
							totempg_ifaces_print(*left_list));
			md->mn_started = 0;
			remove_chan_open_info(md->mn_node_info.nodeId);
		}
		left_list++;
	}

	/*
	 * set up for recovery processing, first phase:
	 */
	recovery_phase = evt_send_event_id;

	/*
	 * List used to distribute last know event IDs.
	 */
	add_list = current_member_list;
	add_count = total_member_count;
	processed_open_counts = 0;

	/*
	 * List used for distributing open channel counts
	 */
	next_chan = esc_head.next;

	/*
	 * List used for distributing retained events
	 */
	next_retained = retained_list.next;

	/*
	 * Member check in counts for open channel counts and
	 * retained events.
	 */
	checked_in = 0;
}

/*
 * Handle event service recovery.  It passes through a number of states to
 * finish the recovery.
 *
 * First, the node broadcasts the highest event ID that it has seen for any
 * joinig node.  This helps to make sure that rejoining nodes don't re-use
 * event IDs that have already been seen.
 *
 * Next, The node broadcasts its open channel information to the other nodes.
 * This makes sure that any joining nodes have complete data on any channels
 * already open.
 *
 * Once done sending open channel information the node waits in a state for
 * the rest of the nodes to finish sending their data.  When the last node
 * has checked in, then the remote channel operation handler selects the next
 * state which is evt_send_retained_events if this is the oldest node in the
 * cluster, or otherwise to evt_wait_send_retained_events to wait for the
 * retained events to be sent.  When the retained events have been sent, the
 * state is changed to evt_recovery_complete and this function exits with
 * zero to inidicate that recovery is done.
 */
static int evt_sync_process(void)
{

	log_printf(RECOVERY_DEBUG, "Process Evt synchronization \n");

	switch (recovery_phase) {

	/*
	 * Send last know event ID to joining nodes to prevent duplicate
	 * event IDs.
	 */
	case evt_send_event_id:
	{
		struct member_node_data *md;
		SaClmClusterNodeT *cn;
		struct req_evt_chan_command cpkt;
		struct iovec chn_iovec;
		int res;

		log_printf(RECOVERY_DEBUG, "Send max event ID updates\n");
		while (add_count) {
			/*
			 * If we've seen this node before, send out the last msg ID
			 * that we've seen from him.  He will set his base ID for
			 * generating event and message IDs to the highest one seen.
			 */
			md = evt_find_node(*add_list);
			if (md != NULL) {
				log_printf(RECOVERY_DEBUG,
					"Send set evt ID %llx to %s\n",
					(unsigned long long)md->mn_last_msg_id,
					totempg_ifaces_print (*add_list));
				md->mn_started = 1;
				memset(&cpkt, 0, sizeof(cpkt));
				cpkt.chc_head.id =
					SERVICE_ID_MAKE (EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
				cpkt.chc_head.size = sizeof(cpkt);
				cpkt.chc_op = EVT_SET_ID_OP;
				cpkt.u.chc_set_id.chc_nodeid = *add_list;
				cpkt.u.chc_set_id.chc_last_id = md->mn_last_msg_id;
				chn_iovec.iov_base = &cpkt;
				chn_iovec.iov_len = cpkt.chc_head.size;
				res = totempg_groups_mcast_joined(openais_group_handle,
						&chn_iovec, 1, TOTEMPG_AGREED);
				if (res != 0) {
					log_printf(RECOVERY_DEBUG,
						"Unable to send event id to %s\n",
						totempg_ifaces_print (*add_list));
					/*
					 * We'll try again later.
					 */
					return 1;
				}

			} else {
				/*
				 * Not seen before, add it to our list of nodes.
				 */
				cn = main_clm_get_by_nodeid(*add_list);
				if (!cn) {
					/*
					 * Error: shouldn't happen
					 */
					log_printf(LOG_LEVEL_ERROR,
							"recovery error node: %s not found\n",
							totempg_ifaces_print (*add_list));
					assert(0);
				} else {
					evt_add_node(*add_list, cn);
				}
			}

			add_list++;
			add_count--;
		}
		recovery_phase = evt_send_open_count;
		return 1;
	}

	/*
	 * Send channel open counts so all members have the same channel open
	 * counts.
	 */
	case evt_send_open_count:
	{
		struct req_evt_chan_command cpkt;
		struct iovec chn_iovec;
		struct event_svr_channel_instance *eci;
		int res;

		log_printf(RECOVERY_DEBUG, "Send open count updates\n");
		/*
		 * Process messages.  When we're done, send the done message
		 * to the nodes.
		 */
		memset(&cpkt, 0, sizeof(cpkt));
		for (;next_chan != &esc_head;
								next_chan = next_chan->next) {
			log_printf(RECOVERY_DEBUG, "Sending next open count\n");
			eci = list_entry(next_chan, struct event_svr_channel_instance,
					esc_entry);
			cpkt.chc_head.id =
				SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
			cpkt.chc_head.size = sizeof(cpkt);
			cpkt.chc_op = EVT_OPEN_COUNT;
			cpkt.u.chc_set_opens.chc_chan_name = eci->esc_channel_name;
			cpkt.u.chc_set_opens.chc_open_count = eci->esc_local_opens;
			chn_iovec.iov_base = &cpkt;
			chn_iovec.iov_len = cpkt.chc_head.size;
			res = totempg_groups_mcast_joined(openais_group_handle,
					&chn_iovec, 1, TOTEMPG_AGREED);

			if (res != 0) {
			/*
			 * Try again later.
			 */
				return 1;
			}
		}
		memset(&cpkt, 0, sizeof(cpkt));
		cpkt.chc_head.id =
			SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
		cpkt.chc_head.size = sizeof(cpkt);
		cpkt.chc_op = EVT_OPEN_COUNT_DONE;
		chn_iovec.iov_base = &cpkt;
		chn_iovec.iov_len = cpkt.chc_head.size;
		res = totempg_groups_mcast_joined(openais_group_handle,
				&chn_iovec, 1,TOTEMPG_AGREED);
		if (res != 0) {
		/*
		 * Try again later.
		 */
			return 1;
		}
		log_printf(RECOVERY_DEBUG, "DONE Sending open counts\n");

		recovery_phase = evt_wait_open_count_done;
		return 1;
	}

	/*
	 * Wait for all nodes to finish sending open updates before proceding.
	 * the EVT_OPEN_COUNT_DONE handler will set the state to
	 * evt_send_retained_events to get us out of this.
	 */
	case evt_wait_open_count_done:
	{
		log_printf(RECOVERY_DEBUG, "Wait for open count done\n");
		return 1;
	}

	/*
	 * If I'm the oldest node, send out retained events so that new nodes
	 * have all the information.
	 */
	case evt_send_retained_events:
	{
		struct iovec chn_iovec;
		struct event_data *evt;
		int res;

		log_printf(RECOVERY_DEBUG, "Send retained event updates\n");

		/*
		 * Process messages.  When we're done, send the done message
		 * to the nodes.
		 */
		for (;next_retained != &retained_list;
								next_retained = next_retained->next) {
			log_printf(LOG_LEVEL_DEBUG, "Sending next retained event\n");
			evt = list_entry(next_retained, struct event_data, ed_retained);
			evt->ed_event.led_head.id =
				SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_RECOVERY_EVENTDATA);
			chn_iovec.iov_base = &evt->ed_event;
			chn_iovec.iov_len = evt->ed_event.led_head.size;
			res = totempg_groups_mcast_joined(openais_group_handle,
					&chn_iovec, 1, TOTEMPG_AGREED);

			if (res != 0) {
			/*
			 * Try again later.
			 */
				return -1;
			}
		}

		recovery_phase = evt_send_retained_events_done;
		return 1;
	}

	case evt_send_retained_events_done:
	{
		struct req_evt_chan_command cpkt;
		struct iovec chn_iovec;
		int res;

		log_printf(RECOVERY_DEBUG, "DONE Sending retained events\n");
		memset(&cpkt, 0, sizeof(cpkt));
		cpkt.chc_head.id =
				SERVICE_ID_MAKE(EVT_SERVICE, MESSAGE_REQ_EXEC_EVT_CHANCMD);
		cpkt.chc_head.size = sizeof(cpkt);
		cpkt.chc_op = EVT_CONF_DONE;
		chn_iovec.iov_base = &cpkt;
		chn_iovec.iov_len = cpkt.chc_head.size;
		res = totempg_groups_mcast_joined(openais_group_handle,
				&chn_iovec, 1, TOTEMPG_AGREED);

		recovery_phase = evt_wait_send_retained_events;
		return 1;
	}

	/*
	 * Wait for send of retained events to finish
	 * the EVT_CONF_DONE handler will set the state to
	 * evt_recovery_complete to get us out of this.
	 */
	case evt_wait_send_retained_events:
	{
		log_printf(RECOVERY_DEBUG, "Wait for retained events\n");
		return 1;
	}

	case evt_recovery_complete:
	{
		log_printf(RECOVERY_DEBUG, "Recovery complete\n");
		return 0;
	}

	default:
		log_printf(LOG_LEVEL_WARNING, "Bad recovery phase state: %u\n",
				recovery_phase);
		recovery_phase = evt_recovery_complete;
		return 0;
	}

	return 0;
}

/*
 * Not used at this time
 */
static void evt_sync_activate(void)
{
	log_printf(RECOVERY_DEBUG, "Evt synchronize activation\n");
}

/*
 * Not used at this time
 */
static void evt_sync_abort(void)
{
	log_printf(RECOVERY_DEBUG, "Abort Evt synchronization\n");
}

/*
 *	vi: set autoindent tabstop=4 shiftwidth=4 :
 */
