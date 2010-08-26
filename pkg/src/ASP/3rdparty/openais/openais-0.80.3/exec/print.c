/*
 * Copyright (c) 2002-2004 MontaVista Software, Inc.
 * Copyright (c) 2006 Ericsson AB.
 * Copyright (c) 2006-2007 Red Hat, Inc.
 *
 * Author: Steven Dake (sdake@redhat.com)
 * Author: Hans Feldt
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#if defined(OPENAIS_LINUX)
#include <linux/un.h>
#endif
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
#include <sys/un.h>
#endif
#include <syslog.h>
#include <stdlib.h>
#include <pthread.h>

#include "print.h"
#include "totemip.h"
#include "../include/saAis.h"
#include "mainconfig.h"
#include "wthread.h"

struct sockaddr_un syslog_sockaddr = {
	sun_family: AF_UNIX,
	sun_path: "/dev/log"
};

static unsigned int logmode = LOG_MODE_BUFFER | LOG_MODE_STDERR | LOG_MODE_SYSLOG;

static char *logfile = 0;

static int log_setup_called;

static pthread_mutex_t log_mode_mutex;

static struct worker_thread_group log_thread_group;

static unsigned int dropped_log_entries = 0;

#ifndef MAX_LOGGERS
#define MAX_LOGGERS 32
#endif
struct logger loggers[MAX_LOGGERS];

static FILE *log_file_fp = 0;

struct log_entry {
	char *file;
	int line;
	int level;
	char str[128];
	struct log_entry *next;
};

static struct log_entry *head;

static struct log_entry *tail;

struct log_data {
	unsigned int syslog_pos;
	unsigned int level;
	char *log_string;
};

static void log_atexit (void);

static int logger_init (const char *ident, int tags, int level, int mode)
{
	int i;

 	for (i = 0; i < MAX_LOGGERS; i++) {
		if (strcmp (loggers[i].ident, ident) == 0) {
			loggers[i].tags |= tags;
			if (level > loggers[i].level) {
				loggers[i].level = level;
			}
			break;
		}
	}

	if (i == MAX_LOGGERS) {
		for (i = 0; i < MAX_LOGGERS; i++) {
			if (strcmp (loggers[i].ident, "") == 0) {
				strncpy (loggers[i].ident, ident, sizeof(loggers[i].ident)-1);
				loggers[i].tags = tags;
				loggers[i].level = level;
				loggers[i].mode = mode;
				break;
			}
		}
	}

	assert(i < MAX_LOGGERS);

	return i;
}

static void buffered_log_printf (char *file, int line, int level,
								 char *format, va_list ap)
{
	struct log_entry *entry = malloc(sizeof(struct log_entry));

	entry->file = file;
	entry->line = line;
	entry->level = level;
	entry->next = NULL;
	if (head == NULL) {
		head = tail = entry;
	} else {
		tail->next = entry;
		tail = entry;
	}
	vsnprintf(entry->str, sizeof(entry->str), format, ap);
}

static void log_printf_worker_fn (void *thread_data, void *work_item)
{
	struct log_data *log_data = (struct log_data *)work_item;

	/*
	 * Output the log data
	 */
	if (logmode & LOG_MODE_FILE && log_file_fp != 0) {
		fprintf (log_file_fp, "%s", log_data->log_string);
		fflush (log_file_fp);
	}
	if (logmode & LOG_MODE_STDERR) {
		fprintf (stderr, "%s", log_data->log_string);
		fflush (stdout);
	}

	if (logmode & LOG_MODE_SYSLOG) {
		syslog (log_data->level, &log_data->log_string[log_data->syslog_pos]);
	}
	free (log_data->log_string);
}

static void _log_printf (char *file, int line,
	int level, int id,
	char *format, va_list ap)
{
	char newstring[4096];
	char log_string[4096];
	char char_time[512];
	struct timeval tv;
	int i = 0;
	int len;
	struct log_data log_data;
	unsigned int res = 0;

	assert (id < MAX_LOGGERS);

	pthread_mutex_lock (&log_mode_mutex);
	/*
	** Buffer before log_setup() has been called.
	*/
	if (logmode & LOG_MODE_BUFFER) {
		buffered_log_printf(file, line, level, format, ap);
		pthread_mutex_unlock (&log_mode_mutex);
		return;
	}

	if (((logmode & LOG_MODE_FILE) || (logmode & LOG_MODE_STDERR)) &&
		(logmode & LOG_MODE_TIMESTAMP)) {
		gettimeofday (&tv, NULL);
		strftime (char_time, sizeof (char_time), "%b %e %k:%M:%S",
				  localtime (&tv.tv_sec));
		i = snprintf (newstring, sizeof(newstring), "%s.%06ld ", char_time, (long)tv.tv_usec);
	}

	if ((level == LOG_LEVEL_DEBUG) || (logmode & LOG_MODE_FILELINE)) {
		snprintf (&newstring[i], sizeof(newstring)-i, "[%s:%04u] %s", file, line, format);
	} else {	
		snprintf (&newstring[i], sizeof(newstring)-i, "[%-5s] %s", loggers[id].ident, format);
	}
	if (dropped_log_entries) {
		/*
		 * Get rid of \n if there is one
		 */
		if (newstring[strlen (newstring) - 1] == '\n') {
			newstring[strlen (newstring) - 1] = '\0';
		}
		len = snprintf (log_string, sizeof(log_string),
			"%s - prior to this log entry, openais logger dropped '%d' messages because of overflow.", newstring, dropped_log_entries + 1);
	} else {
		len = vsprintf (log_string, newstring, ap);
	}

    // len value can never be negative
    assert (len >= 0);
	/*
	** add line feed if not done yet
	*/
	if (log_string[len - 1] != '\n') {
		log_string[len] = '\n';
		log_string[len + 1] = '\0';
	}

	/*
	 * Create work thread data
	 */
	log_data.syslog_pos = i;
	log_data.level = level;
	log_data.log_string = strdup (log_string);
	if (log_data.log_string == NULL) {
		goto drop_log_msg;
	}
	
	if (log_setup_called) {
		res = worker_thread_group_work_add (&log_thread_group, &log_data);
		if (res == 0) {
			dropped_log_entries = 0;
		} else {
			dropped_log_entries += 1;
		}
	} else {
		log_printf_worker_fn (NULL, &log_data);	
	}

	pthread_mutex_unlock (&log_mode_mutex);
	return;

drop_log_msg:
	dropped_log_entries++;
	pthread_mutex_unlock (&log_mode_mutex);
}

int _log_init (const char *ident)
{
	assert (ident != NULL);

	/*
	** do different things before and after log_setup() has been called
	*/
	if (log_setup_called) {
		return logger_init (ident, TAG_LOG, LOG_LEVEL_INFO, 0);
	} else {
		return logger_init (ident, ~0, LOG_LEVEL_DEBUG, 0);
	}
}

int log_setup (char **error_string, struct main_config *config)
{
	int i;
	static char error_string_response[512];

	if (config->logmode & LOG_MODE_FILE) {
		log_file_fp = fopen (config->logfile, "a+");
		if (log_file_fp == 0) {
			snprintf (error_string_response, sizeof(error_string_response),
				"Can't open logfile '%s' for reason (%s).\n",
					 config->logfile, strerror (errno));
			*error_string = error_string_response;
			return (-1);
		}
	}

	pthread_mutex_lock (&log_mode_mutex);
	logmode = config->logmode;
	pthread_mutex_unlock (&log_mode_mutex);
	logfile = config->logfile;

	if (config->logmode & LOG_MODE_SYSLOG) {
		openlog("openais", LOG_CONS|LOG_PID, config->syslog_facility);
	}

	/*
	** reinit all loggers that has initialised before log_setup() was called.
	*/
	for (i = 0; i < MAX_LOGGERS; i++) {
		loggers[i].tags = TAG_LOG;
		if (config->logmode & LOG_MODE_DEBUG) {
			loggers[i].level = LOG_LEVEL_DEBUG;
		} else {
			loggers[i].level = LOG_LEVEL_INFO;
		}
	}

	/*
	** init all loggers that has configured level and tags
	*/
	for (i = 0; i < config->loggers; i++) {
		if (config->logger[i].level == 0)
			config->logger[i].level = LOG_LEVEL_INFO;
		config->logger[i].tags |= TAG_LOG;
		logger_init (config->logger[i].ident,
			config->logger[i].tags,
			config->logger[i].level,
			config->logger[i].mode);
	}

	worker_thread_group_init (
		&log_thread_group,
		1,
		1024,
		sizeof (struct log_data),
		0,
		NULL,
		log_printf_worker_fn);


	/*
	** Flush what we have buffered
	*/
	log_flush();

	internal_log_printf(__FILE__, __LINE__, LOG_LEVEL_DEBUG, "log setup\n");

	atexit (log_atexit);

	log_setup_called = 1;

	return (0);
}

void internal_log_printf (char *file, int line, int priority,
						  char *format, ...)
{
	int id = LOG_ID(priority);
	int level = LOG_LEVEL(priority);
	va_list ap;

	assert (id < MAX_LOGGERS);

	if (LOG_LEVEL(priority) > loggers[id].level) {
		return;
	}

	va_start (ap, format);
	_log_printf (file, line, level, id, format, ap);
	va_end(ap);
}

void internal_log_printf2 (char *file, int line, int level, int id,
						   char *format, ...)
{
	va_list ap;

	assert (id < MAX_LOGGERS);

	va_start (ap, format);
	_log_printf (file, line, level, id, format, ap);
	va_end(ap);
}

void trace (char *file, int line, int tag, int id, char *format, ...)
{
	assert (id < MAX_LOGGERS);

	if (tag & loggers[id].tags) {
		va_list ap;

		va_start (ap, format);
		_log_printf (file, line, LOG_LEVEL_DEBUG, id, format, ap);
		va_end(ap);
	}
}

static void log_atexit (void)
{
	if (log_setup_called) {
		worker_thread_group_wait (&log_thread_group);
	}
}

void log_flush (void)
{
	if (log_setup_called) {
		log_setup_called = 0;
		worker_thread_group_exit (&log_thread_group);
		worker_thread_group_atsegv (&log_thread_group);
	} else {
		struct log_entry *entry = head;
		struct log_entry *tmp;

		/* do not buffer these printouts */
		logmode &= ~LOG_MODE_BUFFER;

		while (entry) {
			internal_log_printf(entry->file, entry->line,
				entry->level, entry->str);
			tmp = entry;
			entry = entry->next;
			free(tmp);
		}

		head = tail = NULL;
	}
}
