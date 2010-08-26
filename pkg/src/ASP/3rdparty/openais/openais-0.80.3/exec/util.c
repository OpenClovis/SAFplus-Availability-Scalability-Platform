/*
 * Copyright (c) 2002-2004 MontaVista Software, Inc.
 * Copyright (c) 2004 Open Source Development Lab
 * Copyright (c) 2006-2007 Red Hat, Inc.
 *
 * All rights reserved.
 *
 * Author: Steven Dake (sdake@redhat.com)
 * Author: Mark Haverkamp (markh@osdl.org)
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "../include/saAis.h"
#include "../include/list.h"
#include "util.h"
#include "print.h"

/*
 * Compare two names.  returns non-zero on match.
 */
int name_match(SaNameT *name1, SaNameT *name2) 
{
	if (name1->length == name2->length) {
		return ((strncmp ((char *)name1->value, (char *)name2->value,
			name1->length)) == 0);
	} 
	return 0;
}

int mar_name_match(mar_name_t *name1, mar_name_t *name2) 
{
	if (name1->length == name2->length) {
		return ((strncmp ((char *)name1->value, (char *)name2->value,
			name1->length)) == 0);
	} 
	return 0;
}

/*
 * Get the time of day and convert to nanoseconds
 */
SaTimeT clust_time_now(void)
{
	struct timeval tv;
	SaTimeT time_now;

	if (gettimeofday(&tv, 0)) {
		return 0ULL;
	}

	time_now = (SaTimeT)(tv.tv_sec) * 1000000000ULL;
	time_now += (SaTimeT)(tv.tv_usec) * 1000ULL;

	return time_now;
}

struct error_code_entry {
	enum e_ais_done code;
	char *string;
};

static struct error_code_entry error_code_map[] = {
        {
		.code = AIS_DONE_EXIT, 
		.string = "finished, exiting normally"
	},
        {
		.code = AIS_DONE_UID_DETERMINE,
		.string = "could not determine the process UID"
	},
        {
		.code = AIS_DONE_GID_DETERMINE,
		.string = "could not determine the process GID"
	},
        {
		.code = AIS_DONE_MEMPOOL_INIT,
		.string = "could not initialize the memory pools"
	},
        {
		.code = AIS_DONE_FORK,
		.string = "could not fork"
	},
        {
		.code = AIS_DONE_LIBAIS_SOCKET,
		.string = "could not create a socket"
	},
        {
		.code = AIS_DONE_LIBAIS_BIND,
		.string = "could not bind to an address"
	},
        {
		.code = AIS_DONE_READKEY,
		.string = "could not read the security key"
	},
        {
		.code = AIS_DONE_MAINCONFIGREAD,
		.string = "could not read the main configuration file"
	},
        {
		.code = AIS_DONE_LOGSETUP,
		.string = "could not setup the logging system"
	},
        {
		.code = AIS_DONE_AMFCONFIGREAD,
		.string = "could not read the AMF configuration"
	},
        {
		.code = AIS_DONE_DYNAMICLOAD,
		.string = "could not load a dynamic object"
	},
        {	.code = AIS_DONE_OBJDB,
		.string = "could not use the object database"
	},
        {
		.code = AIS_DONE_INIT_SERVICES,
		.string = "could not initlalize services"
	},
        {
		.code = AIS_DONE_OUT_OF_MEMORY,
		.string = "Out of memory"
	},
        {
		.code =  AIS_DONE_FATAL_ERR,
		.string = "Unknown fatal error"
	},
};

void openais_exit_error (enum e_ais_done err)
{
	char *error_string = "Error code not available";
	int i;

	for (i = 0; i < (sizeof (error_code_map) / sizeof (struct error_code_entry)); i++) {
		if (err == error_code_map[i].code) {
			error_string = error_code_map[i].string;
		}
	}
	log_printf (LOG_LEVEL_ERROR, "AIS Executive exiting (reason: %s).\n", error_string);
	log_flush();
	exit (err);
}

char *getSaNameT (SaNameT *name)
{
	static char ret_name[SA_MAX_NAME_LENGTH + 1];

	memset (ret_name, 0, sizeof (ret_name));
	memcpy (ret_name, name->value, name->length);

	return (ret_name);
}

char *get_mar_name_t (mar_name_t *name) {
	static char ret_name[SA_MAX_NAME_LENGTH + 1];

	memset (ret_name, 0, sizeof (ret_name));
	memcpy (ret_name, name->value, name->length);

	return (ret_name);
}

char *strstr_rs (const char *haystack, const char *needle)
{
	char *end_address;
	char *new_needle;

	new_needle = (char *)strdup (needle);
	new_needle[strlen (new_needle) - 1] = '\0';

	end_address = strstr (haystack, new_needle);
	if (end_address) {
		end_address += strlen (new_needle);
		end_address = strstr (end_address, needle + strlen (new_needle));
	}
	if (end_address) {
		end_address += 1; /* skip past { or = */
		do {
			if (*end_address == '\t' || *end_address == ' ') {
				end_address++;
			} else {
				break;
			}
		} while (*end_address != '\0');
	}

	free (new_needle);
	return (end_address);
}

void setSaNameT (SaNameT *name, char *str) {
	strncpy ((char *)name->value, str, SA_MAX_NAME_LENGTH-1);
	if (strlen ((char *)name->value) > SA_MAX_NAME_LENGTH) {
		name->length = SA_MAX_NAME_LENGTH;
	} else {
		name->length = strlen (str);
	}
}

int SaNameTisEqual (SaNameT *str1, char *str2) {
	if (str1->length == strlen (str2)) {
		return ((strncmp ((char *)str1->value, (char *)str2,
			str1->length)) == 0);
	} else {
		return 0;
	}
}

