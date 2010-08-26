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
#ifndef UTIL_H_DEFINED
#define UTIL_H_DEFINED
#include <sys/time.h>
#include "../include/mar_gen.h"
#include "../include/saAis.h"

/*
 * Get the time of day and convert to nanoseconds
 */
extern SaTimeT clust_time_now(void);

enum e_ais_done {
	AIS_DONE_EXIT = -1,
	AIS_DONE_UID_DETERMINE = -2,
	AIS_DONE_GID_DETERMINE = -3,
	AIS_DONE_MEMPOOL_INIT = -4,
	AIS_DONE_FORK = -5,
	AIS_DONE_LIBAIS_SOCKET = -6,
	AIS_DONE_LIBAIS_BIND = -7,
	AIS_DONE_READKEY = -8,
	AIS_DONE_MAINCONFIGREAD = -9,
	AIS_DONE_LOGSETUP = -10,
	AIS_DONE_AMFCONFIGREAD = -11,
	AIS_DONE_DYNAMICLOAD = -12,
	AIS_DONE_OBJDB = -13,
	AIS_DONE_INIT_SERVICES = -14,
	AIS_DONE_OUT_OF_MEMORY = -15,
	AIS_DONE_FATAL_ERR = -16
};

/*
 * Compare two names.  returns non-zero on match.
 */
extern int name_match(SaNameT *name1, SaNameT *name2);
extern int mar_name_match(mar_name_t *name1, mar_name_t *name2);
void openais_exit_error (enum e_ais_done err);
extern char *getSaNameT (SaNameT *name);
extern char *strstr_rs (const char *haystack, const char *needle);
extern void setSaNameT (SaNameT *name, char *str);
char *get_mar_name_t (mar_name_t *name);
extern int SaNameTisEqual (SaNameT *str1, char *str2);
#endif /* UTIL_H_DEFINED */
