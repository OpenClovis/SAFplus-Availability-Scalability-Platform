/*
 * Test program for event service
 */

#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef OPENAIS_SOLARIS
#include <stdint.h>
#include <getopt.h>
#else
#include <sys/types.h>
#endif
#include <stdlib.h>
#include <sys/time.h>
#include "saAis.h"
#include "saEvt.h"

// #define EVENT_SUBSCRIBE

#define PUB_RETRIES 100
#define TRY_WAIT 2

extern int get_sa_error(SaAisErrorT, char *, int);
char result_buf[256];
int result_buf_len = sizeof(result_buf);

static int pub_count = 1;
static int wait_time = -1;

SaVersionT version = { 'B', 0x01, 0x01 };

void event_callback( SaEvtSubscriptionIdT subscriptionId,
		const SaEvtEventHandleT eventHandle,
		const SaSizeT eventDataSize);

SaEvtCallbacksT callbacks = {
	0,
	event_callback
};


char channel[256] = "EVENT_TEST_CHANNEL";
unsigned int subscription_id = 0xfedcba98;
unsigned long long ret_time = 30000000000ULL; /* 30 seconds */
char pubname[256] = "Test Pub Name";

#define _patt1 "Filter pattern 1"
#define patt1 (SaUint8T *) _patt1
#define patt1_size sizeof(_patt1)

#define _patt2 "Filter pattern 2"
#define patt2 (SaUint8T *) _patt2
#define patt2_size sizeof(_patt2)

#define _patt3 "Filter pattern 3"
#define patt3 (SaUint8T *) _patt3
#define patt3_size sizeof(_patt3)

#define _patt4 "Filter pattern 4"
#define patt4 (SaUint8T *) _patt4
#define patt4_size sizeof(_patt4)


SaEvtEventFilterT filters[] = {
	{SA_EVT_PREFIX_FILTER, {patt1_size, patt1_size, patt1}},
	{SA_EVT_SUFFIX_FILTER, {patt2_size, patt2_size, patt2}},
	{SA_EVT_EXACT_FILTER, {patt3_size, patt3_size, patt3}},
	{SA_EVT_PASS_ALL_FILTER, {patt4_size, patt4_size, patt4}}
};

SaEvtEventFilterArrayT subscribe_filters = {
	sizeof(filters)/sizeof(SaEvtEventFilterT),
	filters, 
};


	SaUint8T pat0[100];
	SaUint8T pat1[100];
	SaUint8T pat2[100];
	SaUint8T pat3[100];
	SaUint8T pat4[100];
	SaEvtEventPatternT evt_patts[5] = {
		{100, 100, pat0},
		{100, 100, pat1},
		{100, 100, pat2},
		{100, 100, pat3},
		{100, 100, pat4}};
	SaEvtEventPatternArrayT	evt_pat_get_array = { 5, 0, evt_patts };

SaEvtEventPatternT patterns[] = {
	{patt1_size, patt1_size, patt1},
	{patt2_size, patt2_size, patt2},
	{patt3_size, patt3_size, patt3},
	{patt4_size, patt4_size, patt4}
};
SaNameT test_pub_name;
#define TEST_PRIORITY 2

SaEvtEventPatternArrayT evt_pat_set_array = {
	sizeof(patterns)/sizeof(SaEvtEventPatternT),
	sizeof(patterns)/sizeof(SaEvtEventPatternT),
	patterns
};

char user_data_file[256];
char  user_data[65536];
int user_data_size = 0;

uint64_t clust_time_now(void)
{
	struct timeval tv;
	uint64_t time_now;

	if (gettimeofday(&tv, 0)) {
		return 0ULL;
	}

	time_now = (uint64_t)(tv.tv_sec) * 1000000000ULL;
	time_now += (uint64_t)(tv.tv_usec) * 1000ULL;

	return time_now;
}

int
test_pub()
{
	SaEvtHandleT handle;
	SaEvtChannelHandleT channel_handle;
	SaEvtEventHandleT event_handle;
	SaEvtChannelOpenFlagsT flags;
	SaNameT channel_name;
	uint64_t test_retention;
	SaSelectionObjectT fd;
	int i;

	SaEvtEventIdT event_id;
#ifdef EVENT_SUBSCRIBE
	struct pollfd pfd;
	int nfd;
	int timeout = 1000;
#endif


	
	SaAisErrorT result;
	 
	flags = SA_EVT_CHANNEL_PUBLISHER |
#ifdef EVENT_SUBSCRIBE
		SA_EVT_CHANNEL_SUBSCRIBER |
#endif
		SA_EVT_CHANNEL_CREATE;
	strcpy((char *)channel_name.value, channel);
	channel_name.length = strlen(channel);


	do {
		result = saEvtInitialize (&handle, &callbacks, &version);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("Event Initialize result: %s\n", result_buf);
		return(result);
	}
	do {
		result = saEvtChannelOpen(handle, &channel_name, flags, 
				SA_TIME_MAX, &channel_handle);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("channel open result: %s\n", result_buf);
		return(result);
	}

	/*
	 * Publish with pattens
	 */
	printf("Publish\n");

#ifdef EVENT_SUBSCRIBE
	do {
		result = saEvtEventSubscribe(channel_handle,
			&subscribe_filters,
			subscription_id);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));

	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("event subscribe result: %s\n", result_buf);
		return(result);
	}
#endif
	do {
		result = saEvtEventAllocate(channel_handle, &event_handle);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("event Allocate result: %s\n", result_buf);
		return(result);
	}

	strcpy((char *)test_pub_name.value, pubname);
	test_pub_name.length = strlen(pubname);
	test_retention = ret_time;
	do {
		result = saEvtEventAttributesSet(event_handle,
			&evt_pat_set_array,
			TEST_PRIORITY,
			test_retention,
			&test_pub_name);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("event set attr result(2): %s\n", result_buf);
		return(result);
	}

	for (i = 0; i < pub_count; i++) {
		do {
			result = saEvtEventPublish(event_handle, user_data, 
					user_data_size, &event_id);
		} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
		if (result != SA_AIS_OK) {
			get_sa_error(result, result_buf, result_buf_len);
			printf("event Publish result(2): %s\n", result_buf);
			return(result);
		}
		printf("Published event ID: 0x%llx\n", 
				(unsigned long long)event_id);
	}

	/*
	 * See if we got the event
	 */
	do {
		result = saEvtSelectionObjectGet(handle, &fd);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("saEvtSelectionObject get %s\n", result_buf);
		/* error */
		return(result);
	}
#ifdef EVENT_SUBSCRIBE

	for (i = 0; i < pub_count; i++) {
		pfd.fd = fd;
		pfd.events = POLLIN;
		nfd = poll(&pfd, 1, timeout);
		if (nfd <= 0) {
			printf("poll fds %d\n", nfd);
			if (nfd < 0) {
				perror("poll error");
			}
			goto evt_free;
		}

		printf("Got poll event\n");
		do {
			result = saEvtDispatch(handle, SA_DISPATCH_ONE);
		} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
		if (result != SA_AIS_OK) {
			get_sa_error(result, result_buf, result_buf_len);
			printf("saEvtDispatch %s\n", result_buf);
			return(result);
		}
	}
#endif


	/*
	 * Test cleanup
	 */
	do {
		result = saEvtEventFree(event_handle);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("event free result: %s\n", result_buf);
		return(result);
	}

	do {
		result = saEvtChannelClose(channel_handle);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("channel close result: %s\n", result_buf);
		return(result);
	}
	do {
		result = saEvtFinalize(handle);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));

	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("Event Finalize result: %s\n", result_buf);
		return(result);
	}
	printf("Done\n");
	return SA_AIS_OK;

}

void 
event_callback( SaEvtSubscriptionIdT subscription_id,
		const SaEvtEventHandleT event_handle,
		const SaSizeT event_data_size)
{
	SaAisErrorT result;
	SaUint8T priority;
	SaTimeT retention_time;
	SaNameT publisher_name = {0, {0}};
	SaTimeT publish_time;
	SaEvtEventIdT event_id;
	int i;

	printf("event_callback called\n");
	printf("sub ID: %x\n", subscription_id);
	printf("event_handle %llx\n", 
			(unsigned long long)event_handle);
	printf("event data size %llu\n", (unsigned long long)event_data_size);

	evt_pat_get_array.patternsNumber = 4;
	do {
		result = saEvtEventAttributesGet(event_handle,
			&evt_pat_get_array,	/* patterns */
			&priority,		/* priority */
			&retention_time,	/* retention time */
			&publisher_name,	/* publisher name */
			&publish_time,		/* publish time */
			&event_id		/* event_id */
			);
	} while ((result == SA_AIS_ERR_TRY_AGAIN) && !sleep(TRY_WAIT));
	if (result != SA_AIS_OK) {
		get_sa_error(result, result_buf, result_buf_len);
		printf("event get attr result(2): %s\n", result_buf);
		goto evt_free;
	}
	printf("pattern array count: %llu\n",
		(unsigned long long)evt_pat_get_array.patternsNumber);
	for (i = 0; i < evt_pat_get_array.patternsNumber; i++) {
		printf( "pattern %d =\"%s\"\n", i,
				  evt_pat_get_array.patterns[i].pattern);
	}

	printf("priority: 0x%x\n", priority);
	printf("retention: 0x%llx\n", 
			(unsigned long long)retention_time);
	printf("publisher name content: \"%s\"\n", publisher_name.value); 
	printf("event id: 0x%llx\n", 
			(unsigned long long)event_id);
evt_free:
	result = saEvtEventFree(event_handle);
	get_sa_error(result, result_buf, result_buf_len);
	printf("event free result: %s\n", result_buf);
}


static int err_wait_time = -1;

int main (int argc, char **argv)
{
	static const char opts[] = "c:i:t:n:x:u:w:f:";

	int ret;
	int option;

	while (1) {
		option = getopt(argc, argv, opts);
		if (option == -1) 
			break;

		switch (option) {
		case 'u': {
			int fd;
			int sz;

			strcpy(user_data_file, optarg);
			fd = open(user_data_file, O_RDONLY);
			if (fd < 0) {
				printf("Can't open user data file %s\n",
						user_data_file);
				exit(1);
			}
			sz = read(fd, user_data, 65536);
			if (sz < 0) {
				perror("subscription\n");
				exit(1);
			}
			close(fd);
			user_data_size = sz;
			break;
		}

		case 'c':
			strcpy(channel, optarg);
			break;
		case 'n':
			strcpy(pubname, optarg);
			break;
		case 'f':
			err_wait_time = 
				(unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'i':
			subscription_id = 
				(unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'w':
			wait_time = 
				(unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 't':
			ret_time = strtoull(optarg, NULL, 0);
			ret_time *= 1000000000;
			break;
		case 'x':
			pub_count = strtoul(optarg, NULL, 0);
			break;
		default:
			printf("invalid arg: \"%s\"\n", optarg);
			return 1;
		}
	}
	do {
		ret = test_pub();
		if (ret != SA_AIS_OK) {
			if (err_wait_time < 0) {
				exit(ret);
			} else {
				sleep(err_wait_time);
			}
		} else if (wait_time < 0) {
			break;
		} else {
			sleep(wait_time);
		}
	} while(1);
	return 0;
}
